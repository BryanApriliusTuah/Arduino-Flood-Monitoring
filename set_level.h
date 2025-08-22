#include <SimpleKalmanFilter.h>
SimpleKalmanFilter simpleKalmanFilter(2, 2, 0.01);

#define MAX_RXD 18
#define MAX_TXD 17
#define MAX_EN 16

#define mySerial Serial2

unsigned char data[4] = {};

float estimated_level;

void init_level() {
  mySerial.begin(9600, SERIAL_8N1, MAX_RXD, MAX_TXD);
  delay(250);
}

float read_level() {
  float distance_cm;

  while (mySerial.available() >= 4) {
    if (mySerial.peek() == 0xFF) {
      // Read 4 bytes
      for (int i = 0; i < 4; i++) {
        data[i] = mySerial.read();
      }

      // Check checksum
      int sum = (data[0] + data[1] + data[2]) & 0xFF;
      if (sum == data[3]) {
        int distance_mm = (data[1] << 8) | data[2];
        if (distance_mm > 280) { // Lower limit filter
          distance_cm = distance_mm / 10.0;
          estimated_level = distance_cm;
          float filtered = simpleKalmanFilter.updateEstimate(distance_cm);

          if (filtered != 0.0) {
            estimated_level = filtered;
          }

        } else {
          Serial.println("Below minimum range (28cm)");
          estimated_level = 0.00;
        }
      } else {
        Serial.println("Checksum failed");
      }
    } else {
      // Discard garbage byte
      mySerial.read();
    }
  }

  return estimated_level;
}

float level_normal(){
  float normal = read_level();
  return normal + 10;
}

float level_banjir(){
  float normal = read_level();
  return normal;
}