#include "WiFi.h"

//C0:49:EF:C9:69:64 (ESP32v1)
//30:C6:F7:04:E5:80 (ESP32v4)

void setup(){
  Serial.begin(115200);
  WiFi.mode(WIFI_MODE_STA);
}
 
void loop(){
  Serial.println(WiFi.macAddress());
  delay(1000);
}
