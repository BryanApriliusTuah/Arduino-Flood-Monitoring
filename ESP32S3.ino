#include <set_internet.h>
#include <set_gps.h>
#include <set_level.h>
#include <set_socket.h>
#include <set_server.h>
#include <set_rain_gauge.h>

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#define ANALOG_INPUT_PIN A0

void TaskSocket( void *pvParameters );
void TaskServer( void *pvParameters );
TaskHandle_t analog_read_task_handle;

String siaga = "0.00";
String banjir = "0.00";

uint8_t max_attempts = 20;
uint8_t attempts = 0;
uint32_t blink_delay = 1000;

void setup() {
  Serial.begin(115200);

#ifdef USE_WIFI
  init_wifi();
#else
  init_gsm();
  wakeup_sim();
#endif

  xTaskCreate(TaskSocket, "Task Socket", 10000, (void*) &blink_delay, 2, NULL);
  xTaskCreatePinnedToCore(TaskServer, "Task Server", 10000, NULL, 1, &analog_read_task_handle, ARDUINO_RUNNING_CORE);
}

void loop() {}

void TaskSocket(void *pvParameters) {
  uint32_t blink_delay = *((uint32_t*)pvParameters);
  int state_led = LOW;
  const int ledPin =  38;
  const int wdtPin =  15;

  unsigned cur_time_run, old_time_run;
  unsigned cur_time_rst;
  delay(250);

  pinMode(ledPin, OUTPUT);
  pinMode(wdtPin, OUTPUT);

  init_level();
  init_gps();
  init_rain_gauge();
  init_websocket();

  while ((siaga == "0.00" || banjir == "0.00") && attempts < max_attempts) {
    delay(1000);
    siaga = String(level_siaga());
    banjir = String(level_banjir());
    Serial.print("[Retry] siaga: "); Serial.print(siaga);
    Serial.print(" | Banjir: "); Serial.println(banjir);
    attempts++;
  }

  if (attempts >= max_attempts) {
    Serial.println("[WARNING] Level values not valid after retries.");
  } else {
    push_level(siaga, banjir);
  }


  // Delay 10 seconds after pushing level, with countdown
  char buffer[64];
  for (int i = 10; i > 0; i--) {
    sprintf(buffer, "[INFO] Device will be activated in %d seconds", i);
    push_timeReadySocket(buffer);
    vTaskDelay(pdMS_TO_TICKS(1000)); // wait 1 second
  }

  int counter = 0;

  for (;;) {
    websocket.poll();

    check_and_reset_rainfall();

    cur_time_rst = millis() / 1000;
    if (cur_time_rst > 21600) {  // restart setiap 6 jam
      ESP.restart();
    }

    cur_time_run = millis();
    if (cur_time_run - old_time_run > 1000) {
      Serial.print("[Ultrasonic Sensor] "); Serial.print(read_level()); Serial.println("");
      read_gps(); 
      Serial.println("[Task Socket] Send data to website...");
      Serial.print("Rainfall: "); Serial.println(read_rainfall());
      push_websocket(String(read_level()), String(read_rainfall()), siaga, banjir, getLat(), getLng()); Serial.println("");

      state_led = !state_led;
      digitalWrite(ledPin, state_led);
      digitalWrite(wdtPin, state_led);
      old_time_run = millis();
    }

    vTaskDelay(100);
  }
}

void TaskServer(void *pvParameters) {
  (void) pvParameters;
  unsigned cur_time_eth, old_time_eth;

  for (;;) {

    cur_time_eth = millis();
    if (cur_time_eth - old_time_eth >= 60 * 1000) {
      Serial.println("[TaskServer] Push Server");
      if(getLat() != "" || getLng() != ""){
        push_server(String(read_level()), getLat(), getLng());
      }
      old_time_eth = millis();
    }

    vTaskDelay(100);
  }
}
