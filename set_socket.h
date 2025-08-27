#include <Arduino.h>
#include <ArduinoWebsockets.h>

using namespace websockets;
WebsocketsClient websocket;

bool websocketConnected = false;
String socketUrl = "ws://192.168.1.9:8001/ws";

void init_websocket(){
  Serial.println("[Websocket] Connecting to server...");

  websocket.onEvent([](WebsocketsEvent event, String data) {
    if (event == WebsocketsEvent::ConnectionOpened) {
      Serial.println("[Websocket] Connection opened");
      websocketConnected = true;
    } else if (event == WebsocketsEvent::ConnectionClosed) {
      Serial.println("[Websocket] Connection closed");
      websocketConnected = false;
    } else if (event == WebsocketsEvent::GotPing) {
      Serial.println("[Websocket] Received Ping");
    } else if (event == WebsocketsEvent::GotPong) {
      Serial.println("[Websocket] Received Pong");
    }
  });

  websocket.onMessage([](WebsocketsMessage message) {
    Serial.print("[Websocket] Message received: ");
    Serial.println(message.data());
  });

  websocket.connect(socketUrl);
}

void push_websocket(String level, String rainfall, String lat, String lng){
  if (!websocketConnected) {
    websocket.connect(socketUrl);
  }

  if (!websocket.available()) {
    Serial.println("[Websocket] Not connected. Skipping send...");
    return;
  }

  Serial.println("[Websocket] Sending data...");
  String jsonWebsocket = "{\"hardwareId\": 1 ,\"elevation\":" + level + ",\"curah_hujan\":" + rainfall + ",\"latitude\":\"" + lat + "\",\"longitude\":\"" + lng + "\"}";
  websocket.send(jsonWebsocket);
}