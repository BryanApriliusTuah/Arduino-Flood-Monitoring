#include <HTTPClient.h>
#include <ArduinoJson.h>

// Rain Gauge Configuration
const int pin_interrupt = 3;
volatile int tip_count = 0;
float curah_hujan = 0.00;
float milimeter_per_tip = 0.30;
volatile unsigned long last_interrupt_time = 0;

unsigned long last_reset_time = 0;
const unsigned long RESET_INTERVAL = 60UL * 60UL * 1000UL;  // 1 jam dalam ms

const char* API_URL = "http://192.168.1.100:3000/api/rainfall-hourly"; 
const int HARDWARE_ID = 1;  // Change to your hardware ID

void ICACHE_RAM_ATTR tipping(){
  unsigned long now = millis();
  if (now - last_interrupt_time > 200) {
    tip_count++;
    last_interrupt_time = now;
  }
}

void init_rain_gauge(){
  pinMode(pin_interrupt, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pin_interrupt), tipping, FALLING);
}

float read_rainfall(){
  noInterrupts();
  int tips = tip_count;
  tip_count = 0;
  interrupts();
  curah_hujan += tips * milimeter_per_tip;
  return curah_hujan;
}

// Function to send hourly rainfall data to API
bool sendHourlyRainfallToAPI(float rainfallMm) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[API] WiFi not connected, cannot send data");
    return false;
  }

  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");

  // Create JSON payload
  StaticJsonDocument<200> doc;
  doc["hardwareId"] = HARDWARE_ID;
  doc["rainfall_mm"] = rainfallMm;

  String jsonString;
  serializeJson(doc, jsonString);

  Serial.println("[API] Sending hourly rainfall data:");
  Serial.println(jsonString);

  // Send POST request
  int httpResponseCode = http.POST(jsonString);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("[API] Response code: ");
    Serial.println(httpResponseCode);
    Serial.print("[API] Response: ");
    Serial.println(response);
    http.end();
    return (httpResponseCode == 201 || httpResponseCode == 200);
  } else {
    Serial.print("[API] Error sending data. Error code: ");
    Serial.println(httpResponseCode);
    http.end();
    return false;
  }
}

void check_and_reset_rainfall() {
  unsigned long now = millis();
  if (now - last_reset_time >= RESET_INTERVAL) {

    // SEND DATA TO API BEFORE RESETTING
    Serial.print("[RainGauge] Sending hourly data: ");
    Serial.print(curah_hujan);
    Serial.println(" mm");

    bool success = sendHourlyRainfallToAPI(curah_hujan);

    if (success) {
      Serial.println("[RainGauge] ✓ Data sent successfully to API");
    } else {
      Serial.println("[RainGauge] ✗ Failed to send data to API");
    }

    // Reset the counter
    noInterrupts();
    curah_hujan = 0.00;
    tip_count = 0;
    interrupts();
    last_reset_time = now;
    Serial.println("[RainGauge] Curah hujan telah direset setelah 1 jam");
  }
}

