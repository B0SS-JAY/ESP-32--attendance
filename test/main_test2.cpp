#include <Adafruit_Fingerprint.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "RTClib.h"

// ----------------------
// HARDWARE SETUP
// ----------------------
HardwareSerial fpSerial(2);    // UART2 for fingerprint
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;

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
// FUNCTION DECLARATIONS
int getFingerprintID();
void displayUser(uint8_t id);

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

  // If RTC lost power → set time automatically
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Fingerprint sensor
  fpSerial.begin(57600, SERIAL_8N1, 16, 17);
  finger.begin(57600);

  if (finger.verifyPassword()) {
    lcd.clear();
    lcd.print("Sensor Ready");
  } else {
    lcd.clear();
    lcd.print("Sensor Error!");
    while (1);
  }

  delay(1000);
  lcd.clear();
}

// ----------------------
// MAIN LOOP
// ----------------------
void loop() {
  getFingerprintID();
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

  uint8_t id = finger.fingerID;
  displayUser(id);
  return id;
}

// ----------------------
// LCD DISPLAY FUNCTION
// ----------------------
void displayUser(uint8_t id) {
  DateTime now = rtc.now();  // Get current time
  char timeStr[10];
  sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

  for (int i = 0; i < totalUsers; i++) {
    if (users[i].id == id) {
      lcd.clear();
      lcd.print(users[i].name);  // Line 1

      lcd.setCursor(0, 1);
      lcd.print(users[i].grade); // Grade left
      lcd.setCursor(10, 1);
      lcd.print(timeStr);        // Time on right

      return;
    }
  }

  lcd.clear();
  lcd.print("Unknown ID:");
  lcd.print(id);
}
