//motor 1 (Moving)
const int EN_A = 13;
const int IN_1 = 12;
const int IN_2 = 14;
//motor 2 (Steering)
const int EN_B = 25;
const int IN_3 = 27;
const int IN_4 = 26;

//setup pwm pins
const int pwm_freq = 25000;
const int pwm_channel_1 = 0;
const int pwm_channel_2 = 1;
const int pwm_resolution = 8;

void pwmLimiter(int* pwm_1, int* pwm_2){
  int pwm_1_max = 128, pwm_2_max = 64;
  if(*pwm_1 > pwm_1_max) *pwm_1 = pwm_1_max;
  if(*pwm_2 > pwm_2_max) *pwm_2 = pwm_2_max;
}

void moveMotor(int pwm_1, int pwm_2){
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
  pwmLimiter(&pwm_1, &pwm_2);
  Serial.print("PWM Value: ");
  Serial.print(pwm_1);
  Serial.print("|");
  Serial.print(pwm_2);
  Serial.println();
  ledcWrite(pwm_channel_1, pwm_1);
  ledcWrite(pwm_channel_2, pwm_2);
}

void setup() {
  Serial.begin(9600);
  Serial.println("ESP32 Initialized..");
  
  pinMode(EN_A, OUTPUT);
  pinMode(EN_B, OUTPUT);
  pinMode(IN_1, OUTPUT);
  pinMode(IN_2, OUTPUT);
  pinMode(IN_3, OUTPUT);
  pinMode(IN_4, OUTPUT);

  int freq_1 = ledcSetup(pwm_channel_1, pwm_freq, pwm_resolution);
  if(freq_1 == 0){
    Serial.println("Error occured in configuring pwm channel 1! Returning..");
    return;
  }
  else{
    Serial.print("PWM Freq: ");
    Serial.println(freq_1);
  }
  ledcAttachPin(EN_A, pwm_channel_1);
  int freq_2 = ledcSetup(pwm_channel_2, pwm_freq, pwm_resolution);
  if(freq_2 == 0){
    Serial.println("Error occured in configuring pwm channel 2! Returning..");
    return;
  }
  else{
    Serial.print("PWM Freq: ");
    Serial.println(freq_2);
  }
  ledcAttachPin(EN_B, pwm_channel_2);
}

void loop() {
  moveMotor(255, 255);
  delay(2000);
  moveMotor(255, -255);
  delay(2000);
  moveMotor(-255, 255);
  delay(2000);
  moveMotor(-255, -255);
  delay(2000);
}
