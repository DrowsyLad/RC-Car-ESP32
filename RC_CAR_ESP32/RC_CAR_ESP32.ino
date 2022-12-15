// #define BluetoothSerial
#define ESP_NOW
#define SUMO

#ifdef ESP_NOW
#include <esp_now.h>
#include <WiFi.h>
#endif

#ifdef BluetoothSerial
#include <BluetoothSerial.h>
#endif

//---------------------------------------- Pins Setup ------------------------------------------

enum ESP32_pins{
  //motor 1 (Moving)
  EN_A = 13,
  IN_1 = 12,
  IN_2 = 14,
  //motor 2 (Steering)
  EN_B = 25,
  IN_3 = 27,
  IN_4 = 26,
  //lights
  RED_LIGHT = 33,
  FRONT_LIGHT = 32
};

//setup pwm pins
const int pwm_freq = 25000;
const int pwm_channel_1 = 0;
const int pwm_channel_2 = 1;
const int pwm_resolution = 8;

//---------------------------------------- Pins Setup ------------------------------------------
//----------------------------------------------------------------------------------------------
//------------------------------------- Global Variables ---------------------------------------

const int pwm_1_max = 216, pwm_2_max = 128;
int command_x = 0, command_y = 0;
bool print_status = false;

//------------------------------------- Global Variables ---------------------------------------
//----------------------------------------------------------------------------------------------
//--------------------------------------- Car Control ------------------------------------------

void pwmLimiter(int* pwm_1, int* pwm_2, bool boost){
  if(*pwm_1 > pwm_1_max and boost == false) *pwm_1 = pwm_1_max;
  else if(*pwm_1 < -pwm_1_max and boost == false) *pwm_1 = -pwm_1_max;
  if(*pwm_2 > pwm_2_max) *pwm_2 = pwm_2_max;
  else if(*pwm_2 < -pwm_2_max) *pwm_2 = -pwm_2_max;
}

void moveMotor(int pwm_1, int pwm_2, bool boost = false, bool lights = false){
  #ifdef SUMO
  int temp_1 = pwm_1, temp_2 = pwm_2, pwm_limit;
  boost ? pwm_limit = 255 : pwm_limit = 216;
  pwm_1 = temp_1 + temp_2;
  pwm_2 = temp_1 - temp_2;
  if(pwm_1 > pwm_limit) pwm_1 = pwm_limit;
  else if(pwm_1 < -pwm_limit) pwm_1 = -pwm_limit;
  if(pwm_2 > pwm_limit) pwm_2 = pwm_limit;
  else if(pwm_2 < -pwm_limit) pwm_2 = -pwm_limit;
  #else
  pwmLimiter(&pwm_1, &pwm_2, boost);
  #endif
  
  Serial.print("PWM 1: ");
  Serial.println(pwm_1);
  Serial.print("PWM 2: ");
  Serial.println(pwm_2);
  Serial.print("BOOST: ");
  Serial.println(boost ? "True" : "False");
  Serial.print("LIGHTS: ");
  Serial.println(lights ? "True" : "False");
  
  if(pwm_1 < 0){
    pwm_1 *= -1;
    digitalWrite(IN_1, HIGH);
    digitalWrite(IN_2, LOW);
  }
  else{
    digitalWrite(IN_1, LOW);
    digitalWrite(IN_2, HIGH);
  }
  if(pwm_2 < 0){
    pwm_2 *= -1;
    digitalWrite(IN_3, HIGH);
    digitalWrite(IN_4, LOW);
  }
  else{
    digitalWrite(IN_3, LOW);
    digitalWrite(IN_4, HIGH);
  }

  ledcWrite(pwm_channel_1, pwm_1);
  ledcWrite(pwm_channel_2, pwm_2);
}

//--------------------------------------- Car Control ------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------- ESP NOW --------------------------------------------

#ifdef ESP_NOW

typedef struct MessageStruct {
  int pwm_x;
  int pwm_y;
  bool boost;
  bool lights;
};

MessageStruct controlData;

void OnDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len) {
  memcpy(&controlData, incomingData, sizeof(controlData));
  if(print_status){
    Serial.println("Incoming Data..");
    Serial.print("Bytes received: ");
    Serial.print(len);
    Serial.println();
    print_status = false;
  }
}

void ESP_NOW_mainProcess(){
  moveMotor(controlData.pwm_x, controlData.pwm_y, controlData.boost, controlData.lights);
}

#endif

//----------------------------------------- ESP NOW --------------------------------------------
//----------------------------------------------------------------------------------------------
//------------------------------------- Bluetooth Serial ---------------------------------------

#ifdef BluetoothSerial

BluetoothSerial SerialBT;

void receiveCommand(){
  String command = "None";
  if(SerialBT.available()){
    command = SerialBT.readString();
    processCommand(&command);
  }
  Serial.print(command);
  Serial.print("|");
  Serial.print(command_x);
  Serial.print("|");
  Serial.print(command_y);
  Serial.println();
}

void processCommand(String* command){
  if(*command == "x-") command_x = -1;
  else if(*command == "x+") command_x = 1;
  else if(*command == "x") command_x = 0;
  
  else if(*command == "y-") command_y = -1;
  else if(*command == "y+") command_y = 1;
  else if(*command == "y") command_y = 0;

  else{
    command_x = 0;
    command_y = 0;
  }
}

void bluetoothSerial_mainProcess() {
  receiveCommand();
  moveMotor(command_x * pwm_1_max, command_y * pwm_2_max);
}

#endif

//------------------------------------- Bluetooth Serial ---------------------------------------
//----------------------------------------------------------------------------------------------
//--------------------------------------- Main Process -----------------------------------------

void setup() {
  //Serial init
  Serial.begin(115200);

  //BluetoothSerial init
  #ifdef BluetoothSerial
  SerialBT.begin("ESP32_Mus");
  #endif

  //ESP NOW init
  #ifdef ESP_NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  #endif

  //Pin init
  pinMode(EN_A, OUTPUT);
  pinMode(EN_B, OUTPUT);
  pinMode(IN_1, OUTPUT);
  pinMode(IN_2, OUTPUT);
  pinMode(IN_3, OUTPUT);
  pinMode(IN_4, OUTPUT);

  //PWM init
  ledcSetup(pwm_channel_1, pwm_freq, pwm_resolution);
  ledcAttachPin(EN_A, pwm_channel_1);
  ledcSetup(pwm_channel_2, pwm_freq, pwm_resolution);
  ledcAttachPin(EN_B, pwm_channel_2);

  Serial.println("ESP32 Initialized..");
}

void loop(){
  print_status = true;
  #ifdef BluetoothSerial
  bluetoothSerial_mainProcess();
  #endif
  #ifdef ESP_NOW
  ESP_NOW_mainProcess();
  #endif
  delay(100);
}

//--------------------------------------- Main Process -----------------------------------------
//----------------------------------------------------------------------------------------------
