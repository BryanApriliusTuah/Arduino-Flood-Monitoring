uint8_t e_mea = 2; // Seberapa kita percaya bahwa sensor punya noise (biasanya ±2 cm untuk ultrasonic)
uint8_t e_est = 2; // Seberapa yakin kita sama tebakan awal (samakan dengan e_mea, nilai otomatis melakukan penyesuaian seiring waktu)
uint8_t q = 0.8; // Seberapa cepat dunia nyata bisa berubah, dan filter harus mengikutinya (0.001 s.d 1)

#include <SimpleKalmanFilter.h>
SimpleKalmanFilter simpleKalmanFilter(e_mea, e_est, q);

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
      vTaskDelay(1);
    }
  }

  return estimated_level;
}

float level_siaga(){
  float banjir = read_level();
  return banjir + 10;
}

float level_banjir(){
  float banjir = read_level();
  return banjir;
}

String check_ultrasonic(){
  float jarak = read_level();
  return "Jarak Sensor Ultrasonik: " + String(jarak) + " cm";
}