#include <set_internet.h>
#include <set_gps.h>
#include <set_level.h>
#include <set_socket.h>
#include <set_server.h>
#include <set_rain_gauge.h>
#include <esp_task_wdt.h>

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#define ANALOG_INPUT_PIN A0
#define WDT_TIMEOUT 90  // 30 second watchdog timeout

void TaskSocket( void *pvParameters );
void TaskServer( void *pvParameters );
TaskHandle_t socket_task_handle = NULL;
TaskHandle_t server_task_handle = NULL;

String siaga = "0.00";
String banjir = "0.00";

uint8_t max_attempts = 20;
uint8_t attempts = 0;
uint32_t blink_delay = 1000;

void setup() {
  Serial.begin(115200);
  delay(1000);  // Give serial time to initialize

  esp_reset_reason_t reason = esp_reset_reason();
  Serial.print("Reset reason: ");
  Serial.println(reason);

  // Print detailed reset reason
  switch(reason) {
    case ESP_RST_POWERON: Serial.println("Power-on reset"); break;
    case ESP_RST_SW: Serial.println("Software reset"); break;
    case ESP_RST_PANIC: Serial.println("Exception/panic"); break;
    case ESP_RST_INT_WDT: Serial.println("Interrupt watchdog"); break;
    case ESP_RST_TASK_WDT: Serial.println("Task watchdog"); break;
    case ESP_RST_WDT: Serial.println("Other watchdog"); break;
    case ESP_RST_BROWNOUT: Serial.println("Brownout reset"); break;
    default: Serial.println("Unknown"); break;
  }

#ifdef USE_WIFI
  init_wifi();
#else
  init_gsm();
  if (!init_gsm_network()) {
    Serial.println("[Main] GSM init failed. Restart...");
    delay(2000);
    ESP.restart();
  }
#endif

  // Initialize WDT first, before creating tasks
  esp_task_wdt_deinit(); // Deinit first to ensure clean state
  
  // Initialize WDT with configuration
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = WDT_TIMEOUT * 1000,
    .idle_core_mask = 0, // Don't watch IDLE tasks
    .trigger_panic = true
  };
  esp_task_wdt_init(&wdt_config);

  // Create tasks
  xTaskCreate(TaskSocket, "TaskSocket", 20000, (void*) &blink_delay, 2, &socket_task_handle);
  xTaskCreatePinnedToCore(TaskServer, "TaskServer", 20000, NULL, 1, &server_task_handle, ARDUINO_RUNNING_CORE);
  
  // Add tasks to WDT after they are created
  if (socket_task_handle != NULL) {
    esp_task_wdt_add(socket_task_handle);
    Serial.println("[Setup] TaskSocket added to WDT");
  }
  
  if (server_task_handle != NULL) {
    esp_task_wdt_add(server_task_handle);
    Serial.println("[Setup] TaskServer added to WDT");
  }
  
  Serial.println("[Setup] Complete!");
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));  // Keep loop alive
}

void TaskSocket(void *pvParameters) {
  uint32_t blink_delay = *((uint32_t*)pvParameters);
  int state_led = LOW;
  const int ledPin = 38;
  const int wdtPin = 15;

  unsigned long cur_time_run = 0, old_time_run = 0;
  unsigned long restart_time = millis();  // Track actual restart time
  
  Serial.println("[TaskSocket] Starting...");
  vTaskDelay(pdMS_TO_TICKS(250));

  pinMode(ledPin, OUTPUT);
  pinMode(wdtPin, OUTPUT);

  // Reset WDT for this task
  esp_task_wdt_reset();

  Serial.println("[TaskSocket] Initializing sensors...");
  init_level();
  vTaskDelay(pdMS_TO_TICKS(100));
  esp_task_wdt_reset();
  
  init_gps();
  vTaskDelay(pdMS_TO_TICKS(100));
  esp_task_wdt_reset();
  
  init_rain_gauge();
  vTaskDelay(pdMS_TO_TICKS(100));
  esp_task_wdt_reset();
  
  Serial.println("[TaskSocket] Initializing WebSocket...");
  init_websocket();

  vTaskDelay(pdMS_TO_TICKS(500));
  esp_task_wdt_reset();

  Serial.println("[TaskSocket] Reading initial levels...");
  while ((siaga == "0.00" || banjir == "0.00") && attempts < max_attempts) {
    esp_task_wdt_reset();  // Feed watchdog during initialization
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    siaga = String(level_siaga());
    banjir = String(level_banjir());
    
    Serial.print("[Retry "); Serial.print(attempts + 1); Serial.print("/"); Serial.print(max_attempts);
    Serial.print("] siaga: "); Serial.print(siaga);
    Serial.print(" | Banjir: "); Serial.println(banjir);
    
    attempts++;
  }

  if (attempts >= max_attempts) {
    Serial.println("[WARNING] Level values not valid after retries.");
  } else {
    Serial.println("[TaskSocket] Pushing initial levels...");
    esp_task_wdt_reset();  // Feed watchdog before HTTP call
    
    // Try to push with timeout protection
    unsigned long push_start = millis();
    push_level(siaga, banjir);
    unsigned long push_duration = millis() - push_start;
    
    Serial.print("[TaskSocket] Push completed in ");
    Serial.print(push_duration);
    Serial.println(" ms");
    
    esp_task_wdt_reset();  // Feed again after push
    vTaskDelay(pdMS_TO_TICKS(1000));  // Wait after push
  }

  // ADD THIS WAITING LOOP
  Serial.println("[TaskSocket] Waiting for WebSocket connection...");
  unsigned long wait_start = millis();
  while (!websocketConnected && (millis() - wait_start < 10000)) { // Wait up to 10 seconds
    webSocket.loop(); // Must call loop() to process events
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_task_wdt_reset(); // Feed the watchdog while waiting
  }

  if (websocketConnected) {
    Serial.println("[TaskSocket] WebSocket connected!");
  } else {
    Serial.println("[TaskSocket] WebSocket connection failed!");
  }

  // Countdown with watchdog feed
  Serial.println("[TaskSocket] Starting countdown...");
  char buffer[64];
  for (int i = 10; i > 0; i--) {
    esp_task_wdt_reset();  // Feed watchdog during countdown
    sprintf(buffer, "[INFO] Device will be activated in %d seconds", i);
    Serial.println(buffer);
    push_timeReadySocket(buffer);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  Serial.println("[TaskSocket] Entering main loop...");
  old_time_run = millis();

  for (;;) {
    esp_task_wdt_reset();  // Feed watchdog at start of each loop
    
    webSocket.loop();
    check_and_reset_rainfall();

    // Check for 6-hour restart (6 hours = 21,600,000 milliseconds)
    if (millis() - restart_time > 21600000UL) {
      Serial.println("[TaskSocket] 6-hour restart triggered");
      vTaskDelay(pdMS_TO_TICKS(1000));
      ESP.restart();
    }

    cur_time_run = millis();
    if (cur_time_run - old_time_run > 1000) {
      Serial.print("[Ultrasonic Sensor] "); 
      Serial.print(read_level()); 
      Serial.println(" cm");
      
      read_gps(); 
      
      Serial.println("[Task Socket] Send data to website...");
      Serial.print("Rainfall: "); 
      Serial.print(read_rainfall());
      Serial.println(" mm");
      
      push_websocket(String(read_level()), String(read_rainfall()), siaga, banjir, getLat(), getLng());

      state_led = !state_led;
      digitalWrite(ledPin, state_led);
      digitalWrite(wdtPin, state_led);
      old_time_run = millis();
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void TaskServer(void *pvParameters) {
  (void) pvParameters;
  unsigned long cur_time_eth = 0, old_time_eth = 0;

  Serial.println("[TaskServer] Starting...");
  
  // Reset WDT for this task
  esp_task_wdt_reset();
  
  vTaskDelay(pdMS_TO_TICKS(5000));  // Wait 5 seconds before first push
  
  old_time_eth = millis();

  for (;;) {
    esp_task_wdt_reset();  // Feed watchdog at start of loop
    
    cur_time_eth = millis();
    if (cur_time_eth - old_time_eth >= 60000UL) {  // 60 seconds
      Serial.println("[TaskServer] Push Server initiated");
      
      #ifndef USE_WIFI
      if (!modem->isGprsConnected()) {
        Serial.println("[GSM] Not connected, skipping push");
        old_time_eth = millis();
        vTaskDelay(pdMS_TO_TICKS(1000));
        continue;
      }
      Serial.println("[GSM] Connection OK");
      #endif
      
      String lat = getLat();
      String lng = getLng();
      
      if(lat != "" && lng != "") {
        Serial.print("[TaskServer] Lat: "); Serial.print(lat);
        Serial.print(" Lng: "); Serial.println(lng);
        
        // Feed WDT before potentially long operation
        esp_task_wdt_reset();
        push_server(String(read_level()), lat, lng);
        esp_task_wdt_reset();  // Feed again after push
      } else {
        Serial.println("[TaskServer] No GPS data, skipping push");
      }
      
      old_time_eth = millis();
      Serial.println("[TaskServer] Push completed");
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}