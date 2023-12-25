// Communication protocol
#include <esp_now.h>
#include <WiFi.h>

// OLED Display
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define joystick_1_x 35
#define joystick_1_y 32
#define joystick_1_button 34
#define button_1 14
#define button_2 27
#define button_3 26
#define button_4 25

int cali_joy_1_x, cali_joy_1_y, cali_joy_2_x, cali_joy_2_y;
int input_offset_x, input_offset_y,
    input_max_x, input_min_x,
    input_max_y, input_min_y;

uint8_t broadcastAddress[] = {0x3C, 0xE9, 0x0E, 0x8A, 0x4B, 0x00}; //BOAT

typedef struct MessageStruct {
  int data_x;
  int data_y;
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

void displayText(String text) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println(text);
  display.display();
}
 
void setup() {
  //joystick
  pinMode(joystick_1_x, INPUT);
  pinMode(joystick_1_y, INPUT);
  pinMode(joystick_1_button, INPUT);
  //button module
  pinMode(button_1, INPUT_PULLUP);
  pinMode(button_2, INPUT_PULLUP);
  pinMode(button_3, INPUT_PULLUP);
  pinMode(button_4, INPUT_PULLUP);

  // Init Serial Monitor
  Serial.begin(115200);
  
  Serial.println("Starting up...");
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  else {
    Serial.println(F("SSD1306 allocation success"));
  }
  display.display();
  delay(2000); // Pause for 2 seconds
  display.clearDisplay(); // Clear the buffer
//  displayText("Loading...");
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println("Loading...");
  display.display();
  delay(2000);
 
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

  // Calibrate joystick
  int temp_x, temp_y;
  for(int i = 0; i < 5; i++){
    temp_x += analogRead(joystick_1_x);
    temp_y += analogRead(joystick_1_y);
    delay(20);
  }
  cali_joy_1_x = temp_x / 5;
  cali_joy_1_y = temp_y / 5;
  
  input_offset_x = cali_joy_1_x, input_offset_y = cali_joy_1_y,
  input_max_x = 4095-cali_joy_1_x, input_min_x = cali_joy_1_x,
  input_max_y = 4095-cali_joy_1_y, input_min_y = cali_joy_1_y;
}

void formatPWMs(int &data_x, int &data_y){
  float temp_x = data_x, temp_y = data_y;
  
  temp_x -= input_offset_x;
  temp_y -= input_offset_y;

  if(temp_x < 0) temp_x = temp_x / input_min_x * 255;
  else temp_x = temp_x / input_max_x * 255;
  if(temp_y < 0) temp_y = temp_y / input_min_y * 255;
  else temp_y = temp_y / input_max_y * 255;

  data_x = temp_x;
  data_y = temp_y;
}

esp_err_t sendData(int data_x, int data_y, bool boost, bool lights){
  controlData.data_x = data_x;
  controlData.data_y = data_y;
  controlData.boost = boost;
  controlData.lights = lights;
  Serial.print("PWM X: ");
  Serial.println(controlData.data_x);
  Serial.print("PWM Y: ");
  Serial.println(controlData.data_y);
  Serial.print("BOOST: ");
  Serial.println(controlData.boost ? "True" : "False");
  Serial.print("LIGHTS: ");
  Serial.println(controlData.lights ? "True" : "False");
  return esp_now_send(broadcastAddress, (uint8_t *) &controlData, sizeof(controlData));
}

void inputStabilizer(int &data_x, int &data_y){
  int temp_x, temp_y;
  for(int i = 0; i < 5; i++){
    temp_x += analogRead(joystick_1_x);
    temp_y += analogRead(joystick_1_y);
    delay(20);
  }
  data_x = temp_x / 5;
  data_y = temp_y / 5;
//  Serial.print("RAW X: ");
//  Serial.println(data_x);
//  Serial.print("RAW Y: ");
//  Serial.println(data_y);
  formatPWMs(data_x, data_y);
}

void loop() {
  int data_x, data_y;
  bool  boost = digitalRead(button_1) == LOW, 
        lights = digitalRead(button_2) == LOW;
  inputStabilizer(data_x, data_y);
  bool sendStatus = sendData(data_x, data_y, boost, lights) == ESP_OK;
  Serial.print("Status: ");
  Serial.println(sendStatus ? "Data sent" : "Error sending the data");
  delay(100);
}
