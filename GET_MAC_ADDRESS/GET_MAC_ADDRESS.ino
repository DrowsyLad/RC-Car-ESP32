#include "WiFi.h"

//3C:E9:0E:8A:4B:00 (ESP32v1)
//3C:E9:0E:4C:A0:8C (ESP32v4)

void setup(){
  Serial.begin(115200);
  WiFi.mode(WIFI_MODE_STA);
}
 
void loop(){
  Serial.println(WiFi.macAddress());
  delay(1000);
}
