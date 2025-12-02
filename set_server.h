String LevelUrl = "http://192.168.43.218:3000/api/level";

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
  http.setTimeout(10000);  // 10 second timeout
  http.addHeader("Content-Type", "application/json");

  Serial.println("[WiFi] Sending POST request...");
  int httpResponseCode = http.POST(jsonPayload);

  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Server response:");
    Serial.println(response);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
  // ============ HTTP POST JSON ============

#else
  Serial.println("[GSM] Starting push_server...");
  
  byte _retry = 0;
  if (!modem->isGprsConnected()) {
    Serial.println("[GSM] GPRS not connected, attempting reconnect...");
    while (_retry++ < 3) {  // Reduced from 5 to 3 retries
      Serial.print("[GSM] Retry GPRS "); Serial.print(_retry); Serial.println("/3");
      if (modem->gprsConnect(apn, gprsUser, gprsPass)) {
        Serial.println("[GSM] GPRS reconnected");
        break;
      }
      delay(2000);  // Reduced from 1000
    }
    if (_retry >= 3) {
      Serial.println("[GSM] GPRS failed after retries, skipping push");
      return;  // Don't restart, just skip this push
    }
  }

  Serial.println("[GSM] Connecting to server...");
  if (!client->connect(server, port)) {
    Serial.println("[GSM] Failed to connect to server");
    modem->gprsDisconnect();
    return;
  }

  Serial.println("[GSM] Sending HTTP POST request...");

  http->setTimeout(15000);  // 15 second timeout for GSM
  http->beginRequest();
  http->post(API);
  http->sendHeader("Content-Type", "application/json");
  http->sendHeader("Content-Length", jsonPayload.length());
  http->beginBody();
  http->print(jsonPayload);
  http->endRequest();

  int statusCode = http->responseStatusCode();
  String response = http->responseBody();

  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);

  http->stop();
  modem->gprsDisconnect();
  Serial.println("[GSM] GPRS disconnected");
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

  // ============ HTTP PUT JSON ============
  http.begin(LevelUrl);
  http.setTimeout(10000);  // 10 second timeout
  http.addHeader("Content-Type", "application/json");
  
  Serial.println("[WiFi] Sending PUT request to update levels...");
  int responseCode = http.PUT(jsonPayload);
  String response = http.getString();

  Serial.print("HTTP PUT Response code: "); Serial.println(responseCode);
  Serial.print("Response body PUT: "); Serial.println(response);

  http.end();
  // ============ HTTP PUT JSON ============

#else
  Serial.println("[GSM] Starting push_level...");
  
  byte _retry = 0;
  if (!modem->isGprsConnected()) {
    Serial.println("[GSM] GPRS not connected, attempting reconnect...");
    while (_retry++ < 3) {  // Reduced from 5 to 3 retries
      Serial.print("[GSM] Retry GPRS "); Serial.print(_retry); Serial.println("/3");
      if (modem->gprsConnect(apn, gprsUser, gprsPass)) {
        Serial.println("[GSM] GPRS reconnected");
        break;
      }
      delay(2000);  // Reduced from 1000
    }
    if (_retry >= 3) {
      Serial.println("[GSM] GPRS failed after retries, skipping push_level");
      return;  // Don't restart, just skip
    }
  }

  Serial.println("[GSM] Connecting to server...");
  if (!client->connect(server, port)) {
    Serial.println("[GSM] Failed to connect to server");
    modem->gprsDisconnect();
    return;
  }

  Serial.println("[GSM] Sending HTTP PUT request...");

  http->setTimeout(15000);  // 15 second timeout for GSM
  http->beginRequest();
  http->put("/api/level");
  http->sendHeader("Content-Type", "application/json");
  http->sendHeader("Content-Length", jsonPayload.length());
  http->beginBody();
  http->print(jsonPayload);
  http->endRequest();

  int statusCode = http->responseStatusCode();
  String response = http->responseBody();

  Serial.print("Status code: "); Serial.println(statusCode);
  Serial.print("Response: "); Serial.println(response);

  http->stop();
  modem->gprsDisconnect();
  Serial.println("[GSM] GPRS disconnected");
#endif
}