/**
 * @file Fingerprint_GSM.cpp
 * @author Jayrold Langcay, Angelo Corpuz
 * @brief 
 * @version 0.1
 * @date 2025-11-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "Fingerprint_GSM.h"
FingerprintGSM::FingerprintGSM(HardwareSerial* fpSerial, HardwareSerial* gsmSerial) {
  this->fingerprintSerial = fpSerial;
  this->gsmSerial = gsmSerial;
  this->finger = new Adafruit_Fingerprint(fpSerial);
  this->lcd = nullptr;
  this->rtc = nullptr;
  this->userCount = 0;
  this->gsmReady = false;
  this->lcdEnabled = false;
  this->rtcEnabled = false;
  this->lcdCols = 16;
  this->lcdRows = 2;
  this->showTimeOnLCD = false;
  this->lastTimeUpdate = 0;
  
  // Initialize user array
  for (int i = 0; i < 127; i++) {
    users[i].id = 0;
    users[i].name[0] = '\0';
    users[i].phoneNumber[0] = '\0';
    users[i].notifyOnAccess = false;
  }
}

bool FingerprintGSM::beginFingerprint(long baudRate, uint8_t rxPin, uint8_t txPin) {
  fingerprintSerial->begin(baudRate, SERIAL_8N1, rxPin, txPin);
  delay(100);
  
  if (finger->verifyPassword()) {
    Serial.println("[FP] Fingerprint sensor initialized");
    if (lcdEnabled) {
      lcdShowStatus("Fingerprint", "Ready!");
      delay(1500);
    }
    finger->getParameters();
    return true;
  } else {
    Serial.println("[FP] ERROR: Fingerprint sensor not found");
    if (lcdEnabled) {
      lcdShowStatus("ERROR:", "FP Sensor", "Not Found!");
      delay(2000);
    }
    return false;
  }
}

bool FingerprintGSM::beginGSM(long baudRate, uint8_t rxPin, uint8_t txPin) {
  gsmSerial->begin(baudRate, SERIAL_8N1, rxPin, txPin);
  delay(1000);
  
  Serial.println("[GSM] Initializing SIM800L...");
  if (lcdEnabled) {
    lcdShowStatus("GSM Module", "Initializing...");
  }
  
  // Test AT command
  if (!sendATCommand("AT", "OK", 1000)) {
    Serial.println("[GSM] ERROR: No response from SIM800L");
    if (lcdEnabled) {
      lcdShowStatus("ERROR:", "GSM No Response");
      delay(2000);
    }
    return false;
  }
  
  // Check signal quality
  gsmSerial->println("AT+CSQ");
  delay(500);
  Serial.print("[GSM] Signal: ");
  while (gsmSerial->available()) {
    Serial.write(gsmSerial->read());
  }
  
  // Set SMS mode to text
  if (!sendATCommand("AT+CMGF=1", "OK", 1000)) {
    Serial.println("[GSM] ERROR: Failed to set SMS text mode");
    return false;
  }
  
  // Configure SMS parameters
  sendATCommand("AT+CNMI=2,2,0,0,0", "OK", 1000);
  
  Serial.println("[GSM] SIM800L initialized successfully");
  if (lcdEnabled) {
    lcdShowStatus("GSM Module", "Ready!");
    delay(1500);
  }
  
  gsmReady = true;
  return true;
}

bool FingerprintGSM::beginLCD(uint8_t address, uint8_t cols, uint8_t rows) {
  lcdCols = cols;
  lcdRows = rows;
  
  lcd = new LiquidCrystal_I2C(address, cols, rows);
  lcd->init();
  lcd->backlight();
  
  lcdEnabled = true;
  
  Serial.print("[LCD] Initialized (");
  Serial.print(cols);
  Serial.print("x");
  Serial.print(rows);
  Serial.println(")");
  
  lcdShowWelcome();
  return true;
}

bool FingerprintGSM::beginRTC() {
  rtc = new RTC_DS3231();
  
  if (!rtc->begin()) {
    Serial.println("[RTC] ERROR: RTC not found");
    if (lcdEnabled) {
      lcdShowStatus("ERROR:", "RTC Not Found");
      delay(2000);
    }
    return false;
  }
  
  if (rtc->lostPower()) {
    Serial.println("[RTC] RTC lost power, setting time to compile time");
    rtc->adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  rtcEnabled = true;
  Serial.println("[RTC] Real-Time Clock initialized");
  
  DateTime now = rtc->now();
  Serial.print("[RTC] Current time: ");
  Serial.println(getDateTimeString(now));
  
  if (lcdEnabled) {
    lcdShowStatus("RTC Ready", getTimeString(now), getDateString(now));
    delay(2000);
  }
  
  return true;
}

void FingerprintGSM::setAdminPhone(String phone) {
  adminPhone = phone;
  Serial.print("[GSM] Admin phone set to: ");
  Serial.println(adminPhone);
}

bool FingerprintGSM::sendATCommand(String cmd, String expectedResponse, unsigned long timeout) {
  gsmSerial->println(cmd);
  unsigned long start = millis();
  String response = "";
  
  while (millis() - start < timeout) {
    while (gsmSerial->available()) {
      char c = gsmSerial->read();
      response += c;
    }
    if (response.indexOf(expectedResponse) != -1) {
      return true;
    }
  }
  return false;
}

bool FingerprintGSM::sendSMS(String phoneNumber, String message) {
  if (!gsmReady) {
    Serial.println("[GSM] ERROR: GSM not initialized");
    return false;
  }
  
  Serial.println("[GSM] Sending SMS to: " + phoneNumber);
  if (lcdEnabled) {
    lcdShowStatus("Sending SMS...");
  }
  
  gsmSerial->print("AT+CMGS=\"");
  gsmSerial->print(phoneNumber);
  gsmSerial->println("\"");
  delay(500);
  
  gsmSerial->print(message);
  delay(100);
  gsmSerial->write(26);  // Ctrl+Z to send
  
  unsigned long start = millis();
  bool sent = false;
  
  while (millis() - start < 10000) {
    if (gsmSerial->available()) {
      String response = gsmSerial->readString();
      if (response.indexOf("OK") != -1) {
        sent = true;
        break;
      }
    }
  }
  
  if (sent) {
    Serial.println("[GSM] SMS sent successfully");
    if (lcdEnabled) {
      lcdShowStatus("SMS Sent!");
      delay(1500);
    }
  } else {
    Serial.println("[GSM] ERROR: Failed to send SMS");
    if (lcdEnabled) {
      lcdShowStatus("SMS Failed!");
      delay(1500);
    }
  }
  
  return sent;
}

bool FingerprintGSM::enrollFingerprint(uint8_t id) {
  Serial.print("[FP] Enrolling fingerprint ID #");
  Serial.println(id);
  
  if (lcdEnabled) {
    lcdShowEnrolling(1);
  }
  
  int p = -1;
  Serial.println("[FP] Place finger...");
  
  while (p != FINGERPRINT_OK) {
    p = finger->getImage();
    if (p == FINGERPRINT_OK) {
      Serial.println("[FP] Image captured");
    } else if (p != FINGERPRINT_NOFINGER) {
      Serial.println("[FP] ERROR: Image capture failed");
      if (lcdEnabled) {
        lcdShowStatus("ERROR:", "Capture Failed");
        delay(2000);
      }
      return false;
    }
  }
  
  p = finger->image2Tz(1);
  if (p != FINGERPRINT_OK) {
    Serial.println("[FP] ERROR: Image conversion failed");
    if (lcdEnabled) {
      lcdShowStatus("ERROR:", "Convert Failed");
      delay(2000);
    }
    return false;
  }
  
  if (lcdEnabled) {
    lcdShowEnrolling(2);
  }
  Serial.println("[FP] Remove finger");
  delay(2000);
  
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger->getImage();
  }
  
  if (lcdEnabled) {
    lcdShowEnrolling(3);
  }
  Serial.println("[FP] Place same finger again");
  
  while (p != FINGERPRINT_OK) {
    p = finger->getImage();
    if (p == FINGERPRINT_OK) {
      Serial.println("[FP] Image captured");
    } else if (p != FINGERPRINT_NOFINGER) {
      Serial.println("[FP] ERROR: Image capture failed");
      if (lcdEnabled) {
        lcdShowStatus("ERROR:", "Capture Failed");
        delay(2000);
      }
      return false;
    }
  }
  
  p = finger->image2Tz(2);
  if (p != FINGERPRINT_OK) {
    Serial.println("[FP] ERROR: Image conversion failed");
    if (lcdEnabled) {
      lcdShowStatus("ERROR:", "Convert Failed");
      delay(2000);
    }
    return false;
  }
  
  p = finger->createModel();
  if (p != FINGERPRINT_OK) {
    Serial.println("[FP] ERROR: Fingerprints did not match");
    if (lcdEnabled) {
      lcdShowStatus("ERROR:", "Prints Don't", "Match!");
      delay(2000);
    }
    return false;
  }
  
  p = finger->storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("[FP] Fingerprint enrolled successfully!");
    if (lcdEnabled) {
      lcdShowStatus("Success!", "ID #" + String(id), "Enrolled!");
      delay(2000);
    }
    return true;
  } else {
    Serial.println("[FP] ERROR: Failed to store fingerprint");
    if (lcdEnabled) {
      lcdShowStatus("ERROR:", "Store Failed");
      delay(2000);
    }
    return false;
  }
}

int FingerprintGSM::verifyFingerprint() {
  uint8_t p = finger->getImage();
  if (p != FINGERPRINT_OK) return -1;
  
  p = finger->image2Tz();
  if (p != FINGERPRINT_OK) return -1;
  
  p = finger->fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.print("[FP] Match found! ID #");
    Serial.print(finger->fingerID);
    Serial.print(" Confidence: ");
    Serial.println(finger->confidence);
    return finger->fingerID;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("[FP] No match found");
    return -2;
  }
  
  return -1;
}

bool FingerprintGSM::deleteFingerprint(uint8_t id) {
  uint8_t p = finger->deleteModel(id);
  
  if (p == FINGERPRINT_OK) {
    Serial.print("[FP] Deleted fingerprint ID #");
    Serial.println(id);
    if (lcdEnabled) {
      lcdShowStatus("Deleted", "ID #" + String(id));
      delay(1500);
    }
    return true;
  } else {
    Serial.println("[FP] ERROR: Failed to delete fingerprint");
    if (lcdEnabled) {
      lcdShowStatus("ERROR:", "Delete Failed");
      delay(1500);
    }
    return false;
  }
}

uint8_t FingerprintGSM::getTemplateCount() {
  finger->getTemplateCount();
  return finger->templateCount;
}

void FingerprintGSM::printSensorInfo() {
  finger->getParameters();
  Serial.println("\n[FP] === Sensor Information ===");
  Serial.print("Capacity: "); Serial.println(finger->capacity);
  Serial.print("Security Level: "); Serial.println(finger->security_level);
  Serial.print("Packet Length: "); Serial.println(finger->packet_len);
  Serial.print("Baud Rate: "); Serial.println(finger->baud_rate);
  Serial.print("Templates Stored: "); Serial.println(finger->templateCount);
  Serial.println("============================\n");
  
  if (lcdEnabled) {
    lcdShowStatus("Templates: " + String(finger->templateCount), 
                  "Capacity: " + String(finger->capacity));
    delay(3000);
  }
}

bool FingerprintGSM::addUser(uint8_t id, const char* name, const char* phoneNumber, bool notify) {
  if (id < 1 || id > 127) {
    Serial.println("[USER] ERROR: Invalid ID");
    return false;
  }
  
  users[id - 1].id = id;
  strncpy(users[id - 1].name, name, 31);
  users[id - 1].name[31] = '\0';
  strncpy(users[id - 1].phoneNumber, phoneNumber, 15);
  users[id - 1].phoneNumber[15] = '\0';
  users[id - 1].notifyOnAccess = notify;
  
  Serial.print("[USER] Added user: ");
  Serial.print(name);
  Serial.print(" (ID #");
  Serial.print(id);
  Serial.println(")");
  
  if (lcdEnabled) {
    lcdShowStatus("User Added:", String(name));
    delay(1500);
  }
  
  return true;
}

bool FingerprintGSM::removeUser(uint8_t id) {
  if (id < 1 || id > 127) return false;
  
  users[id - 1].id = 0;
  users[id - 1].name[0] = '\0';
  users[id - 1].phoneNumber[0] = '\0';
  users[id - 1].notifyOnAccess = false;
  
  return true;
}

UserData* FingerprintGSM::getUser(uint8_t id) {
  if (id < 1 || id > 127) return nullptr;
  if (users[id - 1].id == 0) return nullptr;
  return &users[id - 1];
}

void FingerprintGSM::listUsers() {
  Serial.println("\n[USER] === Registered Users ===");
  int count = 0;
  for (int i = 0; i < 127; i++) {
    if (users[i].id != 0) {
      Serial.print("ID #");
      Serial.print(users[i].id);
      Serial.print(": ");
      Serial.print(users[i].name);
      Serial.print(" | Phone: ");
      Serial.print(users[i].phoneNumber);
      Serial.print(" | Notify: ");
      Serial.println(users[i].notifyOnAccess ? "Yes" : "No");
      count++;
    }
  }
  Serial.print("Total: ");
  Serial.print(count);
  Serial.println(" users");
  Serial.println("==============================\n");
  
  if (lcdEnabled) {
    lcdShowStatus("Total Users:", String(count));
    delay(2000);
  }
}

bool FingerprintGSM::sendAccessNotification(uint8_t fingerprintID, bool granted) {
  if (!gsmReady || adminPhone.length() == 0) return false;
  
  String message = "";
  UserData* user = getUser(fingerprintID);
  
  // Get current time if RTC is available
  String timeStamp = "";
  if (rtcEnabled) {
    DateTime now = rtc->now();
    timeStamp = getDateTimeString(now);
  }
  
  if (granted && user != nullptr) {
    message = "ACCESS GRANTED\n";
    message += "User: " + String(user->name) + "\n";
    message += "ID: " + String(fingerprintID) + "\n";
    if (rtcEnabled) {
      message += "Time: " + timeStamp;
    } else {
      message += "Time: " + String(millis() / 1000) + "s";
    }
    
    // Send to admin
    sendSMS(adminPhone, message);
    
    // Send to user if they want notifications
    if (user->notifyOnAccess && strlen(user->phoneNumber) > 0) {
      String userMsg = "Hello " + String(user->name) + ", you accessed the system";
      if (rtcEnabled) {
        userMsg += " at " + getTimeString(rtc->now());
      }
      userMsg += ".";
      sendSMS(String(user->phoneNumber), userMsg);
    }
  } else {
    message = "ACCESS DENIED\n";
    message += "Unknown fingerprint detected!\n";
    if (rtcEnabled) {
      message += "Time: " + timeStamp;
    } else {
      message += "Time: " + String(millis() / 1000) + "s";
    }
    sendSMS(adminPhone, message);
  }
  
  return true;
}

bool FingerprintGSM::sendEnrollmentNotification(uint8_t fingerprintID, const char* name) {
  if (!gsmReady || adminPhone.length() == 0) return false;
  
  String message = "NEW ENROLLMENT\n";
  message += "User: " + String(name) + "\n";
  message += "ID: " + String(fingerprintID);
  
  if (rtcEnabled) {
    DateTime now = rtc->now();
    message += "\nTime: " + getDateTimeString(now);
  }
  
  return sendSMS(adminPhone, message);
}

bool FingerprintGSM::makeCall(String phoneNumber) {
  if (!gsmReady) return false;
  
  Serial.println("[GSM] Making call to: " + phoneNumber);
  if (lcdEnabled) {
    lcdShowStatus("Calling...", phoneNumber);
  }
  
  gsmSerial->print("ATD");
  gsmSerial->print(phoneNumber);
  gsmSerial->println(";");
  
  delay(1000);
  return true;
}

int FingerprintGSM::getFingerprintID() {
  return verifyFingerprint();
}

// LCD Functions
void FingerprintGSM::lcdClear() {
  if (!lcdEnabled) return;
  lcd->clear();
}

void FingerprintGSM::lcdPrint(String text, uint8_t col, uint8_t row) {
  if (!lcdEnabled) return;
  lcd->setCursor(col, row);
  lcd->print(text);
}

void FingerprintGSM::lcdPrintCenter(String text, uint8_t row) {
  if (!lcdEnabled) return;
  
  uint8_t len = text.length();
  if (len >= lcdCols) {
    lcd->setCursor(0, row);
    lcd->print(text.substring(0, lcdCols));
  } else {
    uint8_t pos = (lcdCols - len) / 2;
    lcd->setCursor(pos, row);
    lcd->print(text);
  }
}

void FingerprintGSM::lcdScrollText(String text, uint8_t row, uint16_t delayMs) {
  if (!lcdEnabled) return;
  
  if (text.length() <= lcdCols) {
    lcdPrintCenter(text, row);
    return;
  }
  
  // Add spaces for smooth scrolling
  text = String("  ") + text + String("  ");
  
  for (int i = 0; i <= text.length() - lcdCols; i++) {
    lcd->setCursor(0, row);
    lcd->print(text.substring(i, i + lcdCols));
    delay(delayMs);
  }
}

void FingerprintGSM::lcdShowStatus(String line1, String line2, String line3, String line4) {
  if (!lcdEnabled) return;
  
  lcd->clear();
  
  if (line1.length() > 0) lcdPrintCenter(line1, 0);
  if (lcdRows >= 2 && line2.length() > 0) lcdPrintCenter(line2, 1);
  if (lcdRows >= 3 && line3.length() > 0) lcdPrintCenter(line3, 2);
  if (lcdRows >= 4 && line4.length() > 0) lcdPrintCenter(line4, 3);
}

void FingerprintGSM::lcdShowWelcome() {
  if (!lcdEnabled) return;
  
  lcd->clear();
  lcdPrintCenter("Security System", 0);
  if (lcdRows >= 2) {
    lcdPrintCenter("Initializing...", 1);
  }
  delay(2000);
}

void FingerprintGSM::lcdShowAccessGranted(const char* name) {
  if (!lcdEnabled) return;
  
  lcd->clear();
  lcdPrintCenter("ACCESS GRANTED", 0);
  
  if (lcdRows >= 2) {
    if (strlen(name) > lcdCols) {
      lcdScrollText(String(name), 1, 250);
    } else {
      lcdPrintCenter(String(name), 1);
    }
  }
  
  if (lcdRows >= 3 && rtcEnabled) {
    DateTime now = rtc->now();
    lcdPrintCenter(getTimeString(now), 2);
  } else if (lcdRows >= 3) {
    lcdPrintCenter("Welcome!", 2);
  }
}

void FingerprintGSM::lcdShowAccessDenied() {
  if (!lcdEnabled) return;
  
  lcd->clear();
  lcdPrintCenter("ACCESS DENIED", 0);
  
  if (lcdRows >= 2) {
    lcdPrintCenter("Unknown User", 1);
  }
  
  if (lcdRows >= 3 && rtcEnabled) {
    DateTime now = rtc->now();
    lcdPrintCenter(getTimeString(now), 2);
  }
  
  // Flash backlight
  for (int i = 0; i < 3; i++) {
    lcd->noBacklight();
    delay(200);
    lcd->backlight();
    delay(200);
  }
}

void FingerprintGSM::lcdShowEnrolling(uint8_t step) {
  if (!lcdEnabled) return;
  
  lcd->clear();
  lcdPrintCenter("ENROLLING", 0);
  
  if (lcdRows >= 2) {
    switch (step) {
      case 1:
        lcdPrintCenter("Place Finger", 1);
        break;
      case 2:
        lcdPrintCenter("Remove Finger", 1);
        break;
      case 3:
        lcdPrintCenter("Place Again", 1);
        break;
    }
  }
}

void FingerprintGSM::lcdBacklight(bool on) {
  if (!lcdEnabled) return;
  
  if (on) {
    lcd->backlight();
  } else {
    lcd->noBacklight();
  }
}

void FingerprintGSM::lcdUpdateTime() {
  if (!lcdEnabled || !rtcEnabled || !showTimeOnLCD) return;
  
  if (millis() - lastTimeUpdate >= TIME_UPDATE_INTERVAL) {
    lastTimeUpdate = millis();
    DateTime now = rtc->now();
    
    // Update time on first row
    lcd->setCursor(0, 0);
    String timeStr = getTimeString(now);
    
    // Center the time
    if (timeStr.length() < lcdCols) {
      uint8_t padding = (lcdCols - timeStr.length()) / 2;
      for (uint8_t i = 0; i < padding; i++) {
        lcd->print(" ");
      }
    }
    lcd->print(timeStr);
    
    // Fill remaining space
    for (uint8_t i = timeStr.length() + ((lcdCols - timeStr.length()) / 2); i < lcdCols; i++) {
      lcd->print(" ");
    }
  }
}

void FingerprintGSM::lcdShowTimeDate() {
  if (!lcdEnabled || !rtcEnabled) return;
  
  DateTime now = rtc->now();
  lcd->clear();
  lcdPrintCenter(getTimeString(now), 0);
  if (lcdRows >= 2) {
    lcdPrintCenter(getDateString(now), 1);
  }
  if (lcdRows >= 3) {
    lcdPrintCenter("Temp: " + String(getTemperature(), 1) + "C", 2);
  }
}

void FingerprintGSM::setShowTimeOnLCD(bool show) {
  showTimeOnLCD = show;
  if (show && lcdEnabled && rtcEnabled) {
    lcdUpdateTime();
  }
}

// RTC Functions
DateTime FingerprintGSM::getCurrentTime() {
  if (!rtcEnabled) {
    return DateTime(2000, 1, 1, 0, 0, 0); // Return default if RTC not available
  }
  return rtc->now();
}

bool FingerprintGSM::setTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
  if (!rtcEnabled) {
    Serial.println("[RTC] ERROR: RTC not initialized");
    return false;
  }
  
  rtc->adjust(DateTime(year, month, day, hour, minute, second));
  Serial.print("[RTC] Time set to: ");
  Serial.println(getDateTimeString(rtc->now()));
  
  if (lcdEnabled) {
    lcdShowStatus("Time Set!", getDateTimeString(rtc->now()));
    delay(2000);
  }
  
  return true;
}

void FingerprintGSM::printCurrentTime() {
  if (!rtcEnabled) {
    Serial.println("[RTC] ERROR: RTC not initialized");
    return;
  }
  
  DateTime now = rtc->now();
  Serial.print("[RTC] Current time: ");
  Serial.println(getDateTimeString(now));
  Serial.print("[RTC] Temperature: ");
  Serial.print(getTemperature());
  Serial.println("°C");
}

float FingerprintGSM::getTemperature() {
  if (!rtcEnabled) return 0.0;
  return rtc->getTemperature();
}

// Helper functions for time formatting
String FingerprintGSM::getTimeString(DateTime dt) {
  char buffer[9];
  sprintf(buffer, "%02d:%02d:%02d", dt.hour(), dt.minute(), dt.second());
  return String(buffer);
}

String FingerprintGSM::getDateString(DateTime dt) {
  char buffer[11];
  sprintf(buffer, "%04d-%02d-%02d", dt.year(), dt.month(), dt.day());
  return String(buffer);
}

String FingerprintGSM::getDateTimeString(DateTime dt) {
  char buffer[20];
  sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", 
          dt.year(), dt.month(), dt.day(),
          dt.hour(), dt.minute(), dt.second());
  return String(buffer);
}