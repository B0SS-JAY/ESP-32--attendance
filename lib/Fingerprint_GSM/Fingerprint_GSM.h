/**
 * @file Fingerprint_GSM.h
 * @author Jayrold Langcay, Angelo Corpuz
 * @brief 
 * @version 0.1
 * @date 2025-11-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef FINGERPRINT_GSM_H
#define FINGERPRINT_GSM_H

#include <Arduino.h>
#include <Adafruit_Fingerprint.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>

// User data structure
struct UserData {
  uint8_t id;
  char name[32];
  char phoneNumber[16];
  bool notifyOnAccess;
};

// Access log structure
struct AccessLog {
  uint8_t userId;
  DateTime timestamp;
  bool granted;
};

class FingerprintGSM {
  private:
    HardwareSerial* fingerprintSerial;
    HardwareSerial* gsmSerial;
    Adafruit_Fingerprint* finger;
    LiquidCrystal_I2C* lcd;
    RTC_DS3231* rtc;
    
    UserData users[127];  // Store user data for IDs 1-127
    uint8_t userCount;
    
    String adminPhone;
    bool gsmReady;
    bool lcdEnabled;
    bool rtcEnabled;
    uint8_t lcdCols;
    uint8_t lcdRows;
    
    // Time display settings
    bool showTimeOnLCD;
    unsigned long lastTimeUpdate;
    const unsigned long TIME_UPDATE_INTERVAL = 1000; // Update every second
    
    // GSM helper functions
    bool sendATCommand(String cmd, String expectedResponse, unsigned long timeout);
    void waitForGSM();
    
    // LCD helper functions
    void lcdPrintCenter(String text, uint8_t row);
    void lcdScrollText(String text, uint8_t row, uint16_t delayMs = 300);
    
    // RTC helper functions
    String getTimeString(DateTime dt);
    String getDateString(DateTime dt);
    String getDateTimeString(DateTime dt);
    
  public:
    // Constructor
    FingerprintGSM(HardwareSerial* fpSerial, HardwareSerial* gsmSerial);
    
    // Initialization
    bool beginFingerprint(long baudRate = 57600, uint8_t rxPin = 16, uint8_t txPin = 17);
    bool beginGSM(long baudRate = 9600, uint8_t rxPin = 26, uint8_t txPin = 27);
    bool beginLCD(uint8_t address = 0x27, uint8_t cols = 16, uint8_t rows = 2);
    bool beginRTC();
    void setAdminPhone(String phone);
    
    // Fingerprint operations
    bool enrollFingerprint(uint8_t id);
    int verifyFingerprint();
    bool deleteFingerprint(uint8_t id);
    uint8_t getTemplateCount();
    void printSensorInfo();
    
    // User management
    bool addUser(uint8_t id, const char* name, const char* phoneNumber, bool notify = true);
    bool removeUser(uint8_t id);
    UserData* getUser(uint8_t id);
    void listUsers();
    
    // GSM operations
    bool sendSMS(String phoneNumber, String message);
    bool sendAccessNotification(uint8_t fingerprintID, bool granted);
    bool sendEnrollmentNotification(uint8_t fingerprintID, const char* name);
    String readSMS();
    bool makeCall(String phoneNumber);
    
    // LCD operations
    void lcdClear();
    void lcdPrint(String text, uint8_t col = 0, uint8_t row = 0);
    void lcdShowStatus(String line1, String line2 = "", String line3 = "", String line4 = "");
    void lcdShowWelcome();
    void lcdShowAccessGranted(const char* name);
    void lcdShowAccessDenied();
    void lcdShowEnrolling(uint8_t step);
    void lcdBacklight(bool on);
    void lcdUpdateTime();
    void lcdShowTimeDate();
    void setShowTimeOnLCD(bool show);
    
    // RTC operations
    DateTime getCurrentTime();
    bool setTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
    void printCurrentTime();
    float getTemperature(); // DS3231 has built-in temperature sensor
    
    // Utility
    int getFingerprintID();
    uint8_t captureFingerprint(uint8_t slot);
};

#endif