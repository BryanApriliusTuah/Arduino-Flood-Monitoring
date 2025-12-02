#pragma once

// ====== PILIH MODE ======
#define USE_WIFI // Comment baris ini jika hanya ingin pakai GSM
// =========================

const char server[] = "192.168.43.218";
const int port = 3000;
String API = "/api/elevations";

#ifdef USE_WIFI // --------- WIFI + WiFiManager -----------

#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>

WiFiManager wm;

// Prioritas WiFi - akan dicoba berurutan
struct WiFiCredentials {
  const char* ssid;
  const char* password;
};

const WiFiCredentials priorityWiFi[] = {
  {"Ayen", "aleapril123"},
  {"Redmi Note 8 Pro", "AyenAyen"}
};
const int numPriorityWiFi = 2;

void init_wifi() {
  // Coba koneksi ke WiFi prioritas satu per satu
  for (int i = 0; i < numPriorityWiFi; i++) {
    Serial.print("[WiFi] Mencoba: ");
    Serial.println(priorityWiFi[i].ssid);
    
    WiFi.begin(priorityWiFi[i].ssid, priorityWiFi[i].password);
    
    // Tunggu maksimal 10 detik untuk setiap WiFi
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.print("[WiFi] Tersambung ke: ");
      Serial.println(priorityWiFi[i].ssid);
      Serial.print("[WiFi] IP Address: ");
      Serial.println(WiFi.localIP());
      return;
    }
    
    Serial.println(" Gagal!");
    WiFi.disconnect();
  }
  
  // Jika semua WiFi prioritas gagal, gunakan WiFiManager
  Serial.println("[WiFi] Semua jaringan prioritas gagal.");
  Serial.println("[WiFi] Menggunakan WiFiManager untuk setup manual...");
  
  bool res = wm.autoConnect("ESP32-Setup");
  if (!res) {
    Serial.println("[WiFi] Gagal konek. Restart...");
    delay(3000);
    ESP.restart();
  } else {
    Serial.println("[WiFi] Tersambung ke jaringan!");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.localIP());
  }
}

#else // --------- GSM SIM7600 -----------

#define RXD2 8
#define TXD2 9
#define PKEY 47
#define RST 48
#define SerialMon Serial
#define SerialAT Serial2

// #define TINY_GSM_DEBUG SerialMon 
#define TINY_GSM_MODEM_SIM7600

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

const char apn[]      = "internet"; // ganti sesuai provider
const char gprsUser[] = "";
const char gprsPass[] = "";

TinyGsm* modem = NULL;
TinyGsmClient* client = NULL;
HttpClient* http = NULL;

void init_gsm() {
  Serial.println("[GSM] Step 1: Setting up pins...");
  pinMode(RST, OUTPUT);
  pinMode(PKEY, OUTPUT);
  
  // Ensure pins start in known state
  digitalWrite(RST, HIGH);
  digitalWrite(PKEY, HIGH);
  delay(100);
  
  Serial.println("[GSM] Step 2: Power on sequence...");
  // PKEY pulse to power on (if modem is off)
  // Pull PKEY low for 1-2 seconds to turn on
  digitalWrite(PKEY, LOW);
  delay(1500);
  digitalWrite(PKEY, HIGH);
  delay(100);
  
  Serial.println("[GSM] Step 3: Waiting for modem boot (5s)...");
  delay(5000); // SIM7600 needs time to boot up
  
  Serial.println("[GSM] Step 4: Starting Serial...");
  SerialAT.begin(115200, SERIAL_8N1, RXD2, TXD2);
  delay(1000); // Give serial time to stabilize

  Serial.println("[GSM] Step 5: Creating modem object...");
  modem  = new TinyGsm(SerialAT);
  
  Serial.println("[GSM] Step 6: Creating client objects...");
  client = new TinyGsmClient(*modem);
  http   = new HttpClient(*client, server, port);

  Serial.println("[GSM] Init done");
}

bool init_gsm_network() {
  Serial.println("[GSM] === Starting Network Init ===");
  
  Serial.println("[GSM] Step 1: Restarting modem...");
  Serial.flush(); // Ensure message is printed
  delay(100);
  
  // Try a softer approach first - don't restart if modem is already on
  String modemInfo = modem->getModemInfo();
  Serial.print("[GSM] Modem Info: ");
  Serial.println(modemInfo);
  
  if (modemInfo.length() > 0) {
    Serial.println("[GSM] Modem already responding, skipping restart");
  } else {
    Serial.println("[GSM] Modem not responding, attempting restart...");
    
    // Hardware reset using RST pin
    digitalWrite(RST, LOW);
    delay(100);
    digitalWrite(RST, HIGH);
    delay(3000); // Wait for modem to boot
    
    Serial.println("[GSM] Hardware reset complete");
  }
  
  Serial.println("[GSM] Step 2: Initializing modem...");
  Serial.flush();
  delay(100);
  
  // Test basic communication first
  Serial.println("[GSM] Testing AT communication...");
  SerialAT.println("AT");
  delay(1000);
  
  if (SerialAT.available()) {
    String response = SerialAT.readString();
    Serial.print("[GSM] AT Response: ");
    Serial.println(response);
  } else {
    Serial.println("[GSM] WARNING: No AT response - modem may not be ready");
    delay(3000); // Give more time
  }
  
  Serial.println("[GSM] Calling modem.init()...");
  bool initResult = modem->init();
  
  if (!initResult) {
    Serial.println("[GSM] ERROR: Modem init returned false!");
    return false;
  }
  Serial.println("[GSM] Modem init OK");

  Serial.println("[GSM] Step 3: Waiting for network (60s timeout)...");
  Serial.flush();
  
  // Wait with periodic status updates
  unsigned long start = millis();
  while (!modem->isNetworkConnected() && (millis() - start < 60000)) {
    Serial.print(".");
    delay(2000);
  }
  Serial.println();
  
  if (!modem->isNetworkConnected()) {
    Serial.println("[GSM] ERROR: Network not found after 60s");
    return false;
  }
  
  Serial.println("[GSM] Network OK");
  Serial.print("[GSM] Signal quality: ");
  Serial.println(modem->getSignalQuality());

  Serial.println("[GSM] Step 4: Connecting GPRS...");
  for (int i=0; i<5; i++) {
    Serial.print("[GSM] GPRS attempt ");
    Serial.print(i+1);
    Serial.println("/5");
    Serial.flush();
    
    if (modem->gprsConnect(apn, gprsUser, gprsPass)) {
      Serial.println("[GSM] GPRS connected successfully!");
      Serial.print("[GSM] Local IP: ");
      Serial.println(modem->localIP());
      return true;
    }
    
    Serial.println("[GSM] GPRS retry failed, waiting...");
    delay(5000);
  }
  
  Serial.println("[GSM] ERROR: GPRS connection failed after 5 attempts");
  return false;
}

void check_gsm_network() {
  if (!modem->isGprsConnected()) {
    SerialMon.println("[GSM] GPRS dropped. Reconnecting...");
    modem->gprsConnect(apn, gprsUser, gprsPass);
  }
}

#endif