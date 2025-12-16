#include <Adafruit_Fingerprint.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "RTClib.h"
#include <HardwareSerial.h>

// ----------------------
// HARDWARE SETUP
// ----------------------
HardwareSerial fpSerial(2);    // UART2 for fingerprint
HardwareSerial sim(1);         // UART1 for SIM800L

LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;

// SIM800L UART pins
#define SIM_RX 25   // SIM800L TX
#define SIM_TX 26   // SIM800L RX

// ----------------------
// USER DATABASE
// ----------------------
struct User {
  uint8_t id;
  String name;
  String grade;
};

User users[] = {
  {1, "Jella Rosales", "Grade 11"},
  {2, "Mica Gunsat",   "Grade 11"},
  {3, "Pearl Abad",    "Grade 11"},
  {4, "George Maralli","Grade 11"},
  {5, "Kathleen Mira", "Grade 11"}
};

uint8_t totalUsers = sizeof(users) / sizeof(users[0]);

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fpSerial);

// ----------------------
// SMS CONTROL
// ----------------------
String phoneNumber = "+639176215111";
bool smsSent = false;
unsigned long lastScanTime = 0;

// ----------------------
// FUNCTION DECLARATIONS
// ----------------------
int getFingerprintID();
void displayUser(uint8_t id);
void sendSMS(String message);
String readSIM(unsigned long timeout);

// ----------------------
// SETUP
// ----------------------
void setup() {
  Serial.begin(9600);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Initializing...");

  // RTC
  Wire.begin(21, 22);
  if (!rtc.begin()) {
    lcd.clear();
    lcd.print("RTC Error!");
    while (1);
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Fingerprint
  fpSerial.begin(57600, SERIAL_8N1, 16, 17);
  finger.begin(57600);

  if (!finger.verifyPassword()) {
    lcd.clear();
    lcd.print("Sensor Error!");
    while (1);
  }

  // SIM800L
  sim.begin(9600, SERIAL_8N1, SIM_RX, SIM_TX);
  delay(1000);
  sim.println("AT");
  delay(500);
  sim.println("AT+CMGF=1");
  delay(500);
  sim.println("AT+CNMI=1,2,0,0,0");
  delay(500);

  lcd.clear();
  lcd.print("System Ready");
  delay(1000);
  lcd.clear();
}

// ----------------------
// MAIN LOOP
// ----------------------
void loop() {
  int id = getFingerprintID();

  if (id == -1) {
    smsSent = false;  // finger removed
  }

  delay(300);
}

// ----------------------
// FINGERPRINT FUNCTIONS
// ----------------------
int getFingerprintID() {
  int r = finger.getImage();
  if (r != FINGERPRINT_OK) return -1;

  r = finger.image2Tz();
  if (r != FINGERPRINT_OK) return -1;

  r = finger.fingerFastSearch();
  if (r != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("No Match");
    return -1;
  }

  displayUser(finger.fingerID);
  return finger.fingerID;
}

// ----------------------
// LCD + SMS FUNCTION
// ----------------------
void displayUser(uint8_t id) {
  DateTime now = rtc.now();

  char timeStr[10];
  char dateStr[12];
  sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  sprintf(dateStr, "%02d/%02d/%04d", now.month(), now.day(), now.year());

  for (int i = 0; i < totalUsers; i++) {
    if (users[i].id == id) {

      lcd.clear();
      lcd.print(users[i].name);
      lcd.setCursor(0, 1);
      lcd.print(users[i].grade);
      lcd.setCursor(10, 1);
      lcd.print(timeStr);

      if (!smsSent || millis() - lastScanTime > 10000) {
        String sms =
          "Attendance Alert\n"
          "Name: " + users[i].name + "\n" +
          "Grade: " + users[i].grade + "\n" +
          "Date: " + String(dateStr) + "\n" +
          "Time: " + String(timeStr);

        sendSMS(sms);
        smsSent = true;
        lastScanTime = millis();
      }
      return;
    }
  }

  lcd.clear();
  lcd.print("Unknown ID:");
  lcd.print(id);
}

// ----------------------
// SMS FUNCTIONS
// ----------------------
void sendSMS(String message) {
  sim.println("AT+CMGF=1");
  delay(300);

  sim.print("AT+CMGS=\"");
  sim.print(phoneNumber);
  sim.println("\"");
  delay(300);

  sim.print(message);
  delay(300);
  sim.write(26); // CTRL+Z

  readSIM(8000);
}

String readSIM(unsigned long timeout) {
  unsigned long start = millis();
  String response = "";
  while (millis() - start < timeout) {
    while (sim.available()) {
      response += char(sim.read());
    }
  }
  return response;
}
