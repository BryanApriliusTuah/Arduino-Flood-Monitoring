// set_gsm.h with WiFiManager and GSM fallback
#pragma once

// ====== PILIH MODE ======
#define USE_WIFI // Comment baris ini jika hanya ingin pakai GSM
// =========================

const char server[] = "192.168.1.2";
const int port = 3000;
String API = "/api/elevations";

#ifdef USE_WIFI // --------- WIFI + WiFiManager -----------

#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>

WiFiManager wm;

void init_wifi() {
  Serial.println("[WiFi] Mencoba konek otomatis...");
  bool res = wm.autoConnect("ESP32-Setup");
  if (!res) {
    Serial.println("[WiFi] Gagal konek. Restart...");
    delay(3000);
    ESP.restart();
  } else {
    Serial.println("[WiFi] Tersambung ke jaringan!");
  }
}

#else // --------- GSM SIM7600 -----------

#define RXD2 8
#define TXD2 9
#define PKEY 47
#define RST 48
#define SerialMon Serial
#define SerialAT Serial2

#define TINY_GSM_MODEM_SIM7600
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

const char apn[]      = "Internet";
const char gprsUser[] = "";
const char gprsPass[] = "";

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
HttpClient http(client, server, port);

void init_gsm() {
  pinMode(RST, OUTPUT);
  pinMode(PKEY, OUTPUT);
  SerialAT.begin(38400, SERIAL_8N1, RXD2, TXD2);
}

void wakeup_sim() {
  SerialMon.println("[GSM] Wakeup SIM7600...");
  digitalWrite(PKEY, LOW); digitalWrite(RST, LOW); delay(1000);
  digitalWrite(PKEY, HIGH); digitalWrite(RST, HIGH); delay(1000);
  digitalWrite(PKEY, LOW); digitalWrite(RST, LOW); delay(1000);

  byte _retry = 0;
  Serial.println("[GSM] Menunggu AT Response...");
  while (!_at_ok("AT", "OK", 1000) && _retry < 20) {
    Serial.println("Retry: " + String(_retry++));
  }
  String info = modem.getModemInfo();
  SerialMon.println("Modem Info: " + info);
}

bool _at_ok(const char* cmd, const char* expected, uint16_t timeout) {
  SerialAT.println(cmd);
  unsigned long start = millis();
  String res = "";
  while (millis() - start < timeout) {
    if (SerialAT.available()) res += char(SerialAT.read());
    if (res.indexOf(expected) >= 0) return true;
  }
  return false;
}
#endif