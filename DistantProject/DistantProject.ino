#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ข้อมูลการเชื่อมต่อ WiFi และ LINE Token
const char* ssid = "Boyratchapon_2.4G";
const char* password = "boat27092549";
String lineToken = "7PrvwwNIbvcAEwzCTYTMOxz6rg5m1sKRfgUW2mDrXsM";

// กำหนดพินต่างๆ
#define TRIG_PIN 5       // พิน TRIG ของ Ultrasonic Sensor
#define ECHO_PIN 18      // พิน ECHO ของ Ultrasonic Sensor
#define BUTTON_PIN 19    // พินของปุ่มกด

// กำหนด LCD ที่อยู่ I2C, คอลัมน์ และแถว
LiquidCrystal_I2C lcd(0x27, 16, 2);  
bool isMetric = true; // ค่าเริ่มต้นเป็นเซนติเมตร

// ตั้งเวลาสำหรับการแจ้งเตือน
unsigned long lastNotifyTime = 0;
const unsigned long notifyInterval = 10000; // 10 วินาที
const float MAX_DISTANCE = 700; // ระยะทางสูงสุดที่เซ็นเซอร์สามารถวัดได้ (เซนติเมตร)
bool wifiConnected = false;

void setup() {
  Serial.begin(115200);

  // เริ่มการทำงานของจอ LCD
  lcd.init();
  lcd.backlight();

  // ตั้งค่าพินต่างๆ
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // เชื่อมต่อ WiFi
  connectWiFi();
}

void loop() {
  checkButton();
  float distance = readDistance();
  displayDistance(distance);

  if (distance < MAX_DISTANCE && millis() - lastNotifyTime >= notifyInterval) {
    String distanceStr = String(distance);
    sendLineNotification(distanceStr);
    lastNotifyTime = millis();
  }
  delay(1000);
}

void connectWiFi() {
  WiFi.begin(ssid, password);
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 20) { 
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");
    wifiConnected = true;
  } else {
    Serial.println("\nWiFi Failed");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed");
    wifiConnected = false;
  }
  delay(2000);
}

void checkButton() {
  static bool lastButtonState = HIGH;
  bool buttonState = digitalRead(BUTTON_PIN);

  if (lastButtonState == HIGH && buttonState == LOW) {
    isMetric = !isMetric;
    Serial.print("Unit changed: ");
    Serial.println(isMetric ? "cm" : "in");
    delay(300);
  }
  lastButtonState = buttonState;
}

float readDistance() {
  long duration;
  float distance;

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration * 0.034 / 2;

  if (distance > MAX_DISTANCE) {
    distance = MAX_DISTANCE;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Over 700 Cm");
    sendLineNotification("Over 700 Cm");
  }

  return isMetric ? distance : distance * 0.393701;
}

void displayDistance(float distance) {
  String unit = isMetric ? "cm" : "in";
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Distance:");
  lcd.setCursor(0, 1);
  lcd.print(distance);
  lcd.print(" ");
  lcd.print(unit);
}

void sendLineNotification(String message) {
  if (wifiConnected) {
    HTTPClient http;
    http.begin("https://notify-api.line.me/api/notify");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Authorization", "Bearer " + lineToken);

    String unit = isMetric ? "cm" : "in";
    message += " " + unit;
    String payload = "message=" + message;

    int httpResponseCode = http.POST(payload);
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    http.end();
  } else {
    Serial.println("WiFi not connected, skipping LINE notification.");
  }
}
