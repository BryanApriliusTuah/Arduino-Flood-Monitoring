String LevelUrl = "http://192.168.1.9:3000/api/level";

void push_server(String level ,String lat, String lng) {
  String jsonPayload = "{\"elevation\":" + level + ",\"latitude\":\"" + lat + "\",\"longitude\":\"" + lng + "\",\"hardwareId\": 1}";

#ifdef USE_WIFI
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Tidak tersambung.");
    return;
  }
  HTTPClient http;
  String url = "http://" + String(server) + ":" + String(port) + API;

  // ============ HTTP POST JSON ============

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(jsonPayload);

  Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Server response:");
      Serial.println(response);
    }

    http.end();

  // ============ HTTP POST JSON ============

#else
  byte _retry = 0;
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    while (_retry++ < 5) {
      Serial.println("[GSM] Retry GPRS...");
      if (modem.gprsConnect(apn, gprsUser, gprsPass)) break;
      delay(1000);
    }
    if (_retry >= 5) {
      Serial.println("[GSM] Gagal GPRS, restart...");
      ESP.restart();
    }
  }

  if (!client.connect(server, port)) {
    Serial.println("[GSM] Gagal konek ke server");
    return;
  }

  SerialMon.println("Sending HTTP POST request...");

  // ============ HTTP POST JSON GSM ============

  http.beginRequest();
  http.post(API);
  http.sendHeader("Content-Type", "application/json");
  http.sendHeader("Content-Length", jsonPayload.length());
  http.beginBody();
  http.print(jsonPayload);
  http.endRequest();

  int statusCode = http.responseStatusCode();
  String response = http.responseBody();

  SerialMon.print("Status code: ");
  SerialMon.println(statusCode);
  SerialMon.print("Response: ");
  SerialMon.println(response);

  http.stop();
  modem.gprsDisconnect();
  SerialMon.println("GPRS disconnected");

  // ============ HTTP POST JSON GSM ============
#endif
}

void push_level(String normal, String banjir){
  String jsonPayload = "{\"normal\":" + normal + ",\"banjir\":" + banjir + "}";

#ifdef USE_WIFI
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Tidak tersambung.");
    return;
  }

  HTTPClient http;

  // ============ HTTP POST JSON ============

  http.begin(LevelUrl);
  http.addHeader("Content-Type", "application/json");
  int responseCode = http.PUT(jsonPayload);
  String response = http.getString();

  Serial.print("HTTP PUT Response code: "); Serial.print(responseCode); Serial.println("");
  Serial.print("Response body PUT: ");Serial.print(response); Serial.println("");

  http.end();
  // ============ HTTP POST JSON ============

#else
  byte _retry = 0;
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    while (_retry++ < 5) {
      Serial.println("[GSM] Retry GPRS...");
      if (modem.gprsConnect(apn, gprsUser, gprsPass)) break;
      delay(1000);
    }
    if (_retry >= 5) {
      Serial.println("[GSM] Gagal GPRS, restart...");
      ESP.restart();
    }
  }

  if (!client.connect(server, port)) {
    Serial.println("[GSM] Gagal konek ke server");
    return;
  }

  SerialMon.println("Sending HTTP PUT request...");

  // ============ HTTP PUT JSON GSM ============

  http.beginRequest();
  http.put("/api/level");
  http.sendHeader("Content-Type", "application/json");
  http.sendHeader("Content-Length", jsonPayload.length());
  http.beginBody();
  http.print(jsonPayload);
  http.endRequest();

  int statusCode = http.responseStatusCode();
  String response = http.responseBody();

  SerialMon.print("Status code: "); SerialMon.print(statusCode); Serial.println("");
  SerialMon.print("Response: "); SerialMon.print(response); Serial.println("");

  http.stop();
  modem.gprsDisconnect();
  SerialMon.println("GPRS disconnected");

  // ============ HTTP PUT JSON GSM ============
#endif
}