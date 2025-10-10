#pragma once

#include <WebSocketsClient.h> 
#include "set_internet.h" 

const char* websocket_host = "srv1036121.hstgr.cloud";
const int websocket_port = 8001;
const char* websocket_path = "/ws";

WebSocketsClient webSocket;
bool websocketConnected = false;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("[Websocket] Connection closed");
      websocketConnected = false;
      break;
    case WStype_CONNECTED:
      Serial.println("[Websocket] Connection opened");
      websocketConnected = true;
      break;
    case WStype_TEXT:
      Serial.printf("[Websocket] Message received: %s\n", payload);
      websocketConnected = true;
      break;
    case WStype_ERROR:
      Serial.println("[Websocket] Error occurred");
      websocketConnected = false;
      break;
    default:
      break;
  }
}

void connect_websocket() {
  Serial.println("[Websocket] Attempting to connect...");
  webSocket.begin(websocket_host, websocket_port, websocket_path);
}

void init_websocket(){
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);  // Reconnect every 5 seconds if disconnected
  webSocket.enableHeartbeat(15000, 3000, 2);  // ping interval, pong timeout, disconnect after N failures
  connect_websocket();
}

void push_timeReadySocket(String message){
  if (!websocketConnected) {
    Serial.println("[Websocket] Not connected. Skipping time message...");
    return;
  }
  String jsonWebsocket = "{\"type\": \"time\" , \"timeReady\":\"" + message + "\"}";
  bool sent = webSocket.sendTXT(jsonWebsocket);
  if (!sent) {
    Serial.println("[Websocket] Failed to send time message");
  }
}

void push_websocket(String level, String rainfall, String siaga, String banjir, String lat, String lng){
  if (!websocketConnected) {
    Serial.println("[Websocket] Not connected. Skipping data send...");
    return;
  }
  
  String jsonWebsocket = 
    "{\"type\": \"data\" ,\"hardwareId\": 1 ,\"elevation\":" + level +
    ",\"level_siaga\":" + siaga +
    ",\"level_banjir\":" + banjir +
    ",\"curah_hujan\":" + rainfall +
    ",\"latitude\":\"" + lat +
    "\",\"longitude\":\"" + lng + "\"}";
  
  bool sent = webSocket.sendTXT(jsonWebsocket);
  if (sent) {
    Serial.println("[Websocket] Data sent successfully");
  } else {
    Serial.println("[Websocket] Failed to send data");
  }
}