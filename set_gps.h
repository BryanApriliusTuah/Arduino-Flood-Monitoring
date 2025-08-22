#include <TinyGPS++.h>
TinyGPSPlus gps;

#define ss Serial1
#define RXD1 21
#define TXD1 39
String str_buf_lat, str_buf_lng, str_buf_date;

void displayInfo(){
  if (gps.location.isValid())
  {
    str_buf_lat = String(gps.location.lat(), 6);
    str_buf_lng = String(gps.location.lng(), 6);
    Serial.print("[Location] ");
    Serial.print(str_buf_lat); Serial.print(", "); Serial.print(str_buf_lng);
  }
  else
  {
    Serial.print(F("INVALID"));
  }
  Serial.println("     ");
  
  if (gps.date.isValid())
  {
    str_buf_date = String(gps.date.month()) + "/" + String(gps.date.day()) + "/" + String(gps.date.year());
    Serial.print("[Datetime] "); Serial.print(str_buf_date);

  }
  else
  {
    Serial.print(F("INVALID"));
  }
  Serial.println();
}

void init_gps() {
  ss.begin(9600, SERIAL_8N1, RXD1, TXD1);
  delay(250);
}

void read_gps() {
  bool printed = false;
  while (ss.available() > 0)
    if (gps.encode(ss.read()) && !printed){
      displayInfo();
      printed = true;
    }
}

String getLat(){
  return str_buf_lat;
}

String getLng(){
  return str_buf_lng;
}

String getDate(){
  return str_buf_date;
}