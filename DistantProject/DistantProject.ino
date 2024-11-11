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
const unsigned long notifyInterval = 5000; // 5 วินาที
const float MAX_DISTANCE = 700; // ระยะทางสูงสุดที่เซ็นเซอร์สามารถวัดได้ (เซนติเมตร)

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
  // ตรวจสอบการกดปุ่มเพื่อเปลี่ยนหน่วย
  checkButton();

  // อ่านและแสดงค่าระยะทาง
  float distance = readDistance();
  displayDistance(distance);

  // ตรวจสอบเวลาผ่านไปแล้ว 10 วินาทีหรือไม่ก่อนส่งแจ้งเตือน
  if (distance < 10 && millis() - lastNotifyTime >= notifyInterval) {
    // แปลงระยะทางให้เป็น String ก่อนส่งไปยังฟังก์ชัน
    String distanceStr = String(distance);
    sendLineNotification(distanceStr);
    lastNotifyTime = millis(); // บันทึกเวลาที่แจ้งเตือนล่าสุด
  }

  delay(1000); // หน่วงเวลา 1 วินาทีสำหรับการอ่านค่าระยะห่าง
}

// ฟังก์ชันเชื่อมต่อ WiFi
void connectWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

// ฟังก์ชันตรวจสอบการกดปุ่ม
void checkButton() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    isMetric = !isMetric;  // สลับหน่วยการวัด
    delay(300);  // หน่วงเวลาเล็กน้อยเพื่อป้องกันการอ่านหลายครั้ง
  }
}

// ฟังก์ชันอ่านค่าระยะทางจาก Ultrasonic Sensor
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

  // ตรวจสอบว่าเกิน 700 เซนติเมตรหรือไม่
  if (distance > MAX_DISTANCE) {
    distance = MAX_DISTANCE; // ตั้งค่าระยะทางเป็น 700 cm ถ้าเกิน
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Over 700 Cm");
    sendLineNotification("Over 700 Cm");
  }

  // แปลงหน่วยถ้าจำเป็น
  return isMetric ? distance : distance * 0.393701;
}

// ฟังก์ชันแสดงค่าระยะทางบนจอ LCD
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

// ฟังก์ชันส่งแจ้งเตือนผ่าน LINE
void sendLineNotification(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://notify-api.line.me/api/notify");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Authorization", "Bearer " + lineToken);

    String payload = "message=" + message;
    http.POST(payload);
    http.end();
  }
}
