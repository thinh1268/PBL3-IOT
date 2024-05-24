#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#include <WiFiMulti.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include "FirebaseESP32.h"
#include <ESP8266WiFiMulti.h>
#endif
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>


LiquidCrystal_I2C lcd(0x27, 16, 2); // Địa chỉ I2C của LCD và kích thước màn hình
const int buttonPin = 4; // Chân nút nhấn
bool lastButtonState = HIGH;
bool currentButtonState = HIGH;
bool isLcdOn = false;
#define IR_SENSOR_PIN 2
#define SOUND_SENSOR_PIN 39
#define ALARM_PIN 12
#define BUTTON_PIN 4

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define API_KEY "AIzaSyCIKfBR07ejF-PBrijlbjdqCdUjy9N42jk"
#define DATABASE_URL "pbl3-da653-default-rtdb.firebaseio.com"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;
bool alarmSystemActive = false;
bool previousAlarmSystemActive = false;
unsigned long lastAttemptTime = 0;
WiFiMulti wifiMulti;


void connectToWiFiAndFirebase() {
  wifiMulti.addAP("THINH NGUYEN - 2.4G", "10072001");
  wifiMulti.addAP("Khoa 20dtclc1", "aaaaaaaa");
  wifiMulti.addAP("Redmi Note 11", "1234567890");

  Serial.print("Connecting to Wi-Fi");
  unsigned long startTime = millis();
  while (wifiMulti.run() != WL_CONNECTED){
    if (millis() - startTime > 10000) {
      Serial.println("Failed to connect to WiFi");
      Serial.println("______________________________");
      Serial.println(" OFFLINE MODE ");
      return;
    }
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void setup(){
  pinMode(IR_SENSOR_PIN, INPUT);
  pinMode(SOUND_SENSOR_PIN, INPUT);
  pinMode(ALARM_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  Wire.begin(21,22);               //Thiết lập chân kết nối I2C (SDA,SCL);
  lcd.init(); // Khởi tạo LCD
  lcd.backlight();                 //Bật đèn nền
  lcd.setCursor(0, 0); // Đặt con trỏ vị trí trái trên cùng của LCD
  lcd.print("WELCOME GROUP 2"); // In GIOI THIEU 

  pinMode(buttonPin, INPUT_PULLUP); // Đặt chân nút nhấn là INPUT_PULLUP (nút mở và nối đất)
  connectToWiFiAndFirebase();
}

void loop(){
  
  delay(500);
  if (digitalRead(BUTTON_PIN) == LOW) {
    alarmSystemActive = !alarmSystemActive;
    delay(200);
  }

  if (alarmSystemActive) {
    int irValue = digitalRead(IR_SENSOR_PIN);
    int soundValue = analogRead(SOUND_SENSOR_PIN);
     // Display sensor values on Serial Monitor
    Serial.print("IR Value: ");
    Serial.print(irValue);
    Serial.print("\tSound Value: ");
    Serial.println(soundValue);
     if (isLcdOn) {
      lcd.setCursor(0, 1); // Set cursor to the first column of the second line
      lcd.print("IR: ");
      lcd.print(irValue);
      lcd.print(" Sound: "); // Adding some spacing
      lcd.print(soundValue);
      
    }
    if (irValue == 0 || soundValue > 100) {
    digitalWrite(ALARM_PIN, HIGH); // Turn on the relay (light)
  } else {
    digitalWrite(ALARM_PIN, LOW); // Turn off the relay (light)
  }
} else {
  digitalWrite(ALARM_PIN, LOW); // Turn off the relay (light) if the alarm system is not active
   // Check conditions and control the relay

currentButtonState = digitalRead(buttonPin);

  if (currentButtonState == LOW && lastButtonState == HIGH) {
    isLcdOn = !isLcdOn;
    if (isLcdOn) { 
      lcd.clear(); // Xóa màn hình
      lcd.setCursor(0, 0);
      lcd.print("BAT CANH BAO");
 
    } else {
      lcd.clear(); // Xóa màn hình
      lcd.setCursor(0, 0);
      lcd.print("TAT CANH BAO");
      lcd.setCursor(0, 1); 
      lcd.print(" OFFLINE MODE "); // trang thai ofline
    }
    delay(200); // Đợi một chút để tránh đọc nút nhấn liên tục
  }

  lastButtonState = currentButtonState;
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastAttemptTime > 300000) { // 5 minutes
      lastAttemptTime = currentMillis;
      Serial.println("Lost WiFi connection. Trying to reconnect...");
      connectToWiFiAndFirebase();
    }
  } else {
    int irValue = digitalRead(IR_SENSOR_PIN);
    int soundValue = analogRead(SOUND_SENSOR_PIN);
  
    if (Firebase.ready() && signupOK ) {
      if (Firebase.RTDB.setInt(&fbdo, "IRSensor/value",irValue)){
        Serial.print("IRSensor: ");
        Serial.println(irValue);
        // Display irValue on the second line of the LCD
        lcd.setCursor(0, 1); // Set cursor to the first column of the second line
        lcd.print("IR: ");
        lcd.print(irValue);
      }
      else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
      }
    
      if (Firebase.RTDB.setInt(&fbdo, "SoundSensor/value", soundValue)){
        Serial.print("SoundSensor: ");
        Serial.println(soundValue);
        // Display soundValue next to irValue on the second line of the LCD
       lcd.print(" Sound: "); // Adding some spacing
       lcd.print(soundValue);
      }
      else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
      }

      if (alarmSystemActive != previousAlarmSystemActive) {
        if (Firebase.RTDB.setInt(&fbdo, "ButtonState/value", alarmSystemActive)){
          Serial.print("ButtonState: ");
          Serial.println(alarmSystemActive);
        }
        else {
          Serial.println("FAILED");
          Serial.println("REASON: " + fbdo.errorReason());
        }
        previousAlarmSystemActive = alarmSystemActive;
      }

      if (Firebase.RTDB.getInt(&fbdo, "ButtonState/value")){
        alarmSystemActive = fbdo.intData();
        Serial.print("Updated ButtonState: ");
        Serial.println(alarmSystemActive);
          if (alarmSystemActive == 1) {
           lcd.clear(); // Clear the LCD screen
           lcd.setCursor(0, 0);
           lcd.print("BAT CANH BAO"); // Display "BAT CANH BAO" when value is 1
           lcd.setCursor(0,1);

           } else {
           lcd.clear(); // Clear the LCD screen
           lcd.setCursor(0, 0);
           lcd.print("TAT CANH BAO"); // Display "TAT CANH BAO" when value is 0
    }
      }
      else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
      }

    }
  }
  Serial.println("______________________________");
}
