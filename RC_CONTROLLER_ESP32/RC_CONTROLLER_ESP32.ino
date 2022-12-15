#include <esp_now.h>
#include <WiFi.h>

#define joystick_x 33
#define joystick_y 32
#define joystick_button 35
#define button_1 14
#define button_2 27
#define button_3 26
#define button_4 25

uint8_t broadcastAddress[] = {0xC0, 0x49, 0xEF, 0xC9, 0x69, 0x64}; //ESP32v1

typedef struct MessageStruct {
  int pwm_x;
  int pwm_y;
  bool boost;
  bool lights;
};

MessageStruct controlData;

esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t statusDelivery) {
  Serial.print("Last Packet Send Status: ");
  Serial.println(statusDelivery == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
 
void setup() {
  //joystick
  pinMode(joystick_x, INPUT);
  pinMode(joystick_y, INPUT);
  pinMode(joystick_button, INPUT);
  //button module
  pinMode(button_1, INPUT_PULLUP);
  pinMode(button_2, INPUT_PULLUP);
  pinMode(button_3, INPUT_PULLUP);
  pinMode(button_4, INPUT_PULLUP);

  // Init Serial Monitor
  Serial.begin(115200);
 
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}

void formatPWMs(int* pwm_x, int* pwm_y){
  float temp_x = *pwm_x, temp_y = *pwm_y;
  const int 
    input_max_x = 1465, input_min_x = 2630,
    input_max_y = 1345, input_min_y = 2750,
    input_offset_x = 2630, input_offset_y = 2750;
  
  temp_x -= input_offset_x;
  temp_y -= input_offset_y;

  if(temp_x < 0) temp_x = temp_x / input_min_x * 255;
  else temp_x = temp_x / input_max_x * 255;
  if(temp_y < 0) temp_y = temp_y / input_min_y * 255;
  else temp_y = temp_y / input_max_y * 255;

  *pwm_x = temp_x;
  *pwm_y = temp_y;
}

esp_err_t sendData(int pwm_x, int pwm_y, bool boost, bool lights){
  controlData.pwm_x = pwm_x;
  controlData.pwm_y = pwm_y;
  controlData.boost = boost;
  controlData.lights = lights;
  Serial.print("PWM X: ");
  Serial.println(controlData.pwm_x);
  Serial.print("PWM Y: ");
  Serial.println(controlData.pwm_y);
  Serial.print("BOOST: ");
  Serial.println(controlData.boost ? "True" : "False");
  Serial.print("LIGHTS: ");
  Serial.println(controlData.lights ? "True" : "False");
  return esp_now_send(broadcastAddress, (uint8_t *) &controlData, sizeof(controlData));
}

void inputStabilizer(int* pwm_x, int* pwm_y){
  int temp_x, temp_y;
  for(int i = 0; i < 5; i++){
    temp_x += analogRead(joystick_x);
    temp_y += analogRead(joystick_y);
    delay(20);
  }
  *pwm_x = temp_x / 5;
  *pwm_y = temp_y / 5;
  formatPWMs(pwm_x, pwm_y);
}

void loop() {
  int pwm_x, pwm_y;
  bool  boost = digitalRead(button_1) == LOW, 
        lights = digitalRead(button_2) == LOW;
  inputStabilizer(&pwm_x, &pwm_y);
  bool sendStatus = sendData(pwm_x, pwm_y, boost, lights) == ESP_OK;
  Serial.print("Status: ");
  Serial.println(sendStatus ? "Data sent" : "Error sending the data");
  delay(100);
}
