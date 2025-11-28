// Example sketch using FingerprintGSM library with LCD and RTC
// Save the library code as FingerprintGSM.h in your sketch folder
// Required Libraries (install from Arduino Library Manager):
// - Adafruit Fingerprint Sensor
// - LiquidCrystal_I2C
// - RTClib by Adafruit

#include "Fingerprint_GSM.h"

// Create serial objects for sensors
HardwareSerial fingerprintSerial(2);  // Use Serial2 for fingerprint
HardwareSerial gsmSerial(1);          // Use Serial1 for GSM

// Create library instance
FingerprintGSM fpGSM(&fingerprintSerial, &gsmSerial);

// Configuration
const char* ADMIN_PHONE = "+639562352443";  // Change to your phone number
bool autoNotify = true;
bool autoVerify = false;  // Enable continuous fingerprint scanning
unsigned long lastScanTime = 0;
const unsigned long SCAN_INTERVAL = 1000;  // Scan every 1 second

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== Fingerprint GSM Security System ===\n");
  
  // Initialize LCD first (0x27 is common I2C address, try 0x3F if it doesn't work)
  // For 16x2 LCD: beginLCD(0x27, 16, 2)
  // For 20x4 LCD: beginLCD(0x27, 20, 4)
  if (!fpGSM.beginLCD(0x27, 16, 2)) {
    Serial.println("Failed to initialize LCD!");
    Serial.println("Check I2C address (try 0x3F if 0x27 doesn't work)");
  }
  
  // Initialize RTC (DS3231 uses I2C address 0x68)
  if (!fpGSM.beginRTC()) {
    Serial.println("Failed to initialize RTC!");
    Serial.println("System will work without timestamps");
  } else {
    // Enable time display on LCD
    fpGSM.setShowTimeOnLCD(false); // Set to true to show clock on LCD
  }
  
  // Initialize Fingerprint Sensor
  // RX=16, TX=17 for fingerprint
  if (!fpGSM.beginFingerprint(57600, 16, 17)) {
    Serial.println("Failed to initialize fingerprint sensor!");
    while (1) delay(1);
  }
  
  // Initialize GSM Module
  // RX=26, TX=27 for SIM800L
  if (!fpGSM.beginGSM(9600, 26, 27)) {
    Serial.println("Failed to initialize GSM module!");
    Serial.println("System will work without SMS notifications");
  }
  
  // Set admin phone number
  fpGSM.setAdminPhone(String(ADMIN_PHONE));
  
  // Print sensor information
  fpGSM.printSensorInfo();
  
  // Print current time
  fpGSM.printCurrentTime();
  
  // Show ready message on LCD
  fpGSM.lcdShowStatus("System Ready", "Place Finger");
  
  // Example: Add some users (optional)
  // fpGSM.addUser(1, "John Doe", "+1234567890", true);
  // fpGSM.addUser(2, "Jane Smith", "+0987654321", false);
  
  printMenu();
}

void loop() {
  // Update time display on LCD if enabled
  fpGSM.lcdUpdateTime();
  
  // Check for commands from Serial Monitor
  if (Serial.available()) {
    char cmd = Serial.read();
    handleCommand(cmd);
  }
  
  // Auto-verify mode - continuously scan for fingerprints
  if (autoVerify && (millis() - lastScanTime > SCAN_INTERVAL)) {
    lastScanTime = millis();
    
    fpGSM.lcdShowStatus("Ready", "Place Finger");
    
    int id = fpGSM.verifyFingerprint();
    
    if (id > 0) {
      // Access granted
      UserData* user = fpGSM.getUser(id);
      
      if (user != nullptr) {
        Serial.print("\n✓ Access Granted - Welcome ");
        Serial.println(user->name);
        fpGSM.lcdShowAccessGranted(user->name);
      } else {
        Serial.println("\n✓ Access Granted - ID #" + String(id));
        fpGSM.lcdShowStatus("ACCESS GRANTED", "ID #" + String(id));
      }
      
      // Print timestamp
      DateTime now = fpGSM.getCurrentTime();
      Serial.print("Time: ");
      Serial.println(fpGSM.getDateTimeString(now));
      
      if (autoNotify) {
        fpGSM.sendAccessNotification(id, true);
      }
      
      delay(3000);
      fpGSM.lcdShowStatus("System Ready", "Place Finger");
      
    } else if (id == -2) {
      // Access denied
      Serial.println("\n✗ Access Denied - Unknown fingerprint!");
      fpGSM.lcdShowAccessDenied();
      
      // Print timestamp
      DateTime now = fpGSM.getCurrentTime();
      Serial.print("Time: ");
      Serial.println(fpGSM.getDateTimeString(now));
      
      if (autoNotify) {
        fpGSM.sendAccessNotification(0, false);
      }
      
      delay(3000);
      fpGSM.lcdShowStatus("System Ready", "Place Finger");
    }
  }
  
  delay(50);
}

void handleCommand(char cmd) {
  Serial.println();
  
  switch (cmd) {
    case 'e':  // Enroll fingerprint
      enrollNewUser();
      break;
      
    case 'v':  // Verify fingerprint
      verifyUser();
      break;
      
    case 'd':  // Delete fingerprint
      deleteUser();
      break;
      
    case 'l':  // List users
      fpGSM.listUsers();
      break;
      
    case 'i':  // Sensor info
      fpGSM.printSensorInfo();
      break;
      
    case 's':  // Send test SMS
      sendTestSMS();
      break;
      
    case 'c':  // Make test call
      makeTestCall();
      break;
      
    case 'n':  // Toggle auto-notify
      autoNotify = !autoNotify;
      Serial.print("Auto-notify: ");
      Serial.println(autoNotify ? "ON" : "OFF");
      fpGSM.lcdShowStatus("Auto-Notify", autoNotify ? "ON" : "OFF");
      delay(1500);
      break;
      
    case 'a':  // Toggle auto-verify
      autoVerify = !autoVerify;
      Serial.print("Auto-verify: ");
      Serial.println(autoVerify ? "ON" : "OFF");
      fpGSM.lcdShowStatus("Auto-Verify", autoVerify ? "ON" : "OFF");
      delay(1500);
      if (autoVerify) {
        fpGSM.lcdShowStatus("System Ready", "Place Finger");
      } else {
        fpGSM.lcdShowStatus("Standby Mode");
      }
      break;
      
    case 'b':  // Toggle LCD backlight
      {
        static bool backlightOn = true;
        backlightOn = !backlightOn;
        fpGSM.lcdBacklight(backlightOn);
        Serial.print("LCD Backlight: ");
        Serial.println(backlightOn ? "ON" : "OFF");
      }
      break;
      
    case 't':  // Show time and date
      showTimeDate();
      break;
      
    case 'r':  // Set RTC time
      setRTCTime();
      break;
      
    case 'p':  // Print current time
      fpGSM.printCurrentTime();
      break;
      
    case 'w':  // Toggle time display on LCD
      {
        static bool showTime = false;
        showTime = !showTime;
        fpGSM.setShowTimeOnLCD(showTime);
        Serial.print("Time display on LCD: ");
        Serial.println(showTime ? "ON" : "OFF");
        if (!showTime) {
          if (autoVerify) {
            fpGSM.lcdShowStatus("System Ready", "Place Finger");
          } else {
            fpGSM.lcdShowStatus("Standby Mode");
          }
        }
      }
      break;
      
    case 'x':  // LCD test
      testLCD();
      break;
      
    case 'h':  // Help
      printMenu();
      break;
      
    default:
      Serial.println("Unknown command. Press 'h' for help.");
  }
}

void enrollNewUser() {
  Serial.println("=== Enroll New User ===");
  fpGSM.lcdShowStatus("ENROLLMENT", "Enter ID");
  
  Serial.print("Enter fingerprint ID (1-127): ");
  
  while (!Serial.available()) delay(10);
  uint8_t id = Serial.parseInt();
  Serial.println(id);
  
  if (id < 1 || id > 127) {
    Serial.println("ERROR: Invalid ID!");
    fpGSM.lcdShowStatus("ERROR:", "Invalid ID!");
    delay(2000);
    return;
  }
  
  // Enroll fingerprint
  if (!fpGSM.enrollFingerprint(id)) {
    Serial.println("ERROR: Enrollment failed!");
    return;
  }
  
  // Get user details
  fpGSM.lcdShowStatus("Enter Details", "Via Serial");
  
  Serial.print("Enter name: ");
  while (!Serial.available()) delay(10);
  String name = Serial.readStringUntil('\n');
  name.trim();
  
  Serial.print("Enter phone number (or leave empty): ");
  while (!Serial.available()) delay(10);
  String phone = Serial.readStringUntil('\n');
  phone.trim();
  
  Serial.print("Send SMS on access? (y/n): ");
  while (!Serial.available()) delay(10);
  char notify = Serial.read();
  bool sendNotifications = (notify == 'y' || notify == 'Y');
  
  // Add user to system
  fpGSM.addUser(id, name.c_str(), phone.c_str(), sendNotifications);
  
  // Send enrollment notification
  fpGSM.sendEnrollmentNotification(id, name.c_str());
  
  Serial.println("\n✓ User enrolled successfully!");
  
  // Show success with timestamp
  DateTime now = fpGSM.getCurrentTime();
  Serial.print("Enrollment time: ");
  Serial.println(fpGSM.getDateTimeString(now));
  
  fpGSM.lcdShowStatus("Success!", name, "ID #" + String(id));
  delay(3000);
  
  if (autoVerify) {
    fpGSM.lcdShowStatus("System Ready", "Place Finger");
  } else {
    fpGSM.lcdShowStatus("Standby Mode");
  }
  
  printMenu();
}

void verifyUser() {
  Serial.println("=== Verify Fingerprint ===");
  fpGSM.lcdShowStatus("VERIFY", "Place Finger");
  
  Serial.println("Place finger on sensor...");
  
  // Wait for finger
  unsigned long startTime = millis();
  while (millis() - startTime < 10000) {  // 10 second timeout
    int id = fpGSM.verifyFingerprint();
    
    if (id > 0) {
      Serial.println("\n✓ Access Granted!");
      
      DateTime now = fpGSM.getCurrentTime();
      Serial.print("Time: ");
      Serial.println(fpGSM.getDateTimeString(now));
      
      UserData* user = fpGSM.getUser(id);
      if (user != nullptr) {
        Serial.print("Welcome, ");
        Serial.println(user->name);
        fpGSM.lcdShowAccessGranted(user->name);
      } else {
        Serial.print("ID #");
        Serial.println(id);
        fpGSM.lcdShowStatus("ACCESS GRANTED", "ID #" + String(id));
      }
      
      if (autoNotify) {
        fpGSM.sendAccessNotification(id, true);
      }
      
      delay(3000);
      break;
      
    } else if (id == -2) {
      Serial.println("\n✗ Access Denied - Unknown fingerprint!");
      
      DateTime now = fpGSM.getCurrentTime();
      Serial.print("Time: ");
      Serial.println(fpGSM.getDateTimeString(now));
      
      fpGSM.lcdShowAccessDenied();
      
      if (autoNotify) {
        fpGSM.sendAccessNotification(0, false);
      }
      
      delay(3000);
      break;
    }
    
    delay(100);
  }
  
  if (autoVerify) {
    fpGSM.lcdShowStatus("System Ready", "Place Finger");
  } else {
    fpGSM.lcdShowStatus("Standby Mode");
  }
  
  printMenu();
}

void deleteUser() {
  Serial.println("=== Delete User ===");
  fpGSM.lcdShowStatus("DELETE USER", "Enter ID");
  
  Serial.print("Enter fingerprint ID to delete (1-127): ");
  
  while (!Serial.available()) delay(10);
  uint8_t id = Serial.parseInt();
  Serial.println(id);
  
  if (id < 1 || id > 127) {
    Serial.println("ERROR: Invalid ID!");
    fpGSM.lcdShowStatus("ERROR:", "Invalid ID!");
    delay(2000);
    return;
  }
  
  // Get user info before deleting
  UserData* user = fpGSM.getUser(id);
  
  if (fpGSM.deleteFingerprint(id)) {
    fpGSM.removeUser(id);
    Serial.println("✓ User deleted successfully!");
    
    DateTime now = fpGSM.getCurrentTime();
    Serial.print("Deletion time: ");
    Serial.println(fpGSM.getDateTimeString(now));
    
    if (user != nullptr) {
      Serial.print("Deleted: ");
      Serial.println(user->name);
      fpGSM.lcdShowStatus("Deleted:", user->name);
    } else {
      fpGSM.lcdShowStatus("Deleted", "ID #" + String(id));
    }
    delay(2000);
  } else {
    Serial.println("ERROR: Failed to delete user");
  }
  
  if (autoVerify) {
    fpGSM.lcdShowStatus("System Ready", "Place Finger");
  } else {
    fpGSM.lcdShowStatus("Standby Mode");
  }
  
  printMenu();
}

void sendTestSMS() {
  Serial.println("=== Send Test SMS ===");
  fpGSM.lcdShowStatus("TEST SMS");
  
  Serial.print("Enter phone number: ");
  
  while (!Serial.available()) delay(10);
  String phone = Serial.readStringUntil('\n');
  phone.trim();
  
  Serial.print("Enter message: ");
  while (!Serial.available()) delay(10);
  String message = Serial.readStringUntil('\n');
  message.trim();
  
  // Add timestamp to message
  DateTime now = fpGSM.getCurrentTime();
  message += "\nSent: " + fpGSM.getDateTimeString(now);
  
  fpGSM.sendSMS(phone, message);
  
  if (autoVerify) {
    fpGSM.lcdShowStatus("System Ready", "Place Finger");
  } else {
    fpGSM.lcdShowStatus("Standby Mode");
  }
  
  printMenu();
}

void makeTestCall() {
  Serial.println("=== Make Test Call ===");
  fpGSM.lcdShowStatus("TEST CALL");
  
  Serial.print("Enter phone number: ");
  
  while (!Serial.available()) delay(10);
  String phone = Serial.readStringUntil('\n');
  phone.trim();
  
  DateTime now = fpGSM.getCurrentTime();
  Serial.print("Call initiated at: ");
  Serial.println(fpGSM.getDateTimeString(now));
  
  fpGSM.makeCall(phone);
  Serial.println("Call initiated. Press any key to continue.");
  
  while (!Serial.available()) delay(100);
  Serial.read();
  
  if (autoVerify) {
    fpGSM.lcdShowStatus("System Ready", "Place Finger");
  } else {
    fpGSM.lcdShowStatus("Standby Mode");
  }
  
  printMenu();
}

void showTimeDate() {
  Serial.println("=== Current Time & Date ===");
  fpGSM.printCurrentTime();
  fpGSM.lcdShowTimeDate();
  
  Serial.println("Press any key to continue...");
  while (!Serial.available()) delay(100);
  Serial.read();
  
  if (autoVerify) {
    fpGSM.lcdShowStatus("System Ready", "Place Finger");
  } else {
    fpGSM.lcdShowStatus("Standby Mode");
  }
  
  printMenu();
}

void setRTCTime() {
  Serial.println("=== Set RTC Time ===");
  fpGSM.lcdShowStatus("SET TIME", "Via Serial");
  
  Serial.println("Enter time in format: YYYY MM DD HH MM SS");
  Serial.println("Example: 2024 11 28 14 30 00");
  Serial.print("Enter: ");
  
  while (!Serial.available()) delay(10);
  
  uint16_t year = Serial.parseInt();
  uint8_t month = Serial.parseInt();
  uint8_t day = Serial.parseInt();
  uint8_t hour = Serial.parseInt();
  uint8_t minute = Serial.parseInt();
  uint8_t second = Serial.parseInt();
  
  Serial.println();
  Serial.print("Setting time to: ");
  Serial.print(year); Serial.print("-");
  Serial.print(month); Serial.print("-");
  Serial.print(day); Serial.print(" ");
  Serial.print(hour); Serial.print(":");
  Serial.print(minute); Serial.print(":");
  Serial.println(second);
  
  if (fpGSM.setTime(year, month, day, hour, minute, second)) {
    Serial.println("✓ Time set successfully!");
  } else {
    Serial.println("✗ Failed to set time!");
  }
  
  delay(2000);
  
  if (autoVerify) {
    fpGSM.lcdShowStatus("System Ready", "Place Finger");
  } else {
    fpGSM.lcdShowStatus("Standby Mode");
  }
  
  printMenu();
}

void testLCD() {
  Serial.println("=== LCD Test ===");
  
  fpGSM.lcdShowStatus("LCD Test", "Line 2", "Line 3", "Line 4");
  delay(2000);
  
  fpGSM.lcdShowTimeDate();
  delay(3000);
  
  fpGSM.lcdShowStatus("Scrolling Text", "This is a very long text that will scroll");
  delay(3000);
  
  fpGSM.lcdShowStatus("Backlight Test");
  for (int i = 0; i < 3; i++) {
    fpGSM.lcdBacklight(false);
    delay(300);
    fpGSM.lcdBacklight(true);
    delay(300);
  }
  
  Serial.println("LCD test complete");
  
  if (autoVerify) {
    fpGSM.lcdShowStatus("System Ready", "Place Finger");
  } else {
    fpGSM.lcdShowStatus("Standby Mode");
  }
  
  printMenu();
}

void printMenu() {
  Serial.println("\n========== MENU ==========");
  Serial.println("e - Enroll new user");
  Serial.println("v - Verify fingerprint");
  Serial.println("d - Delete user");
  Serial.println("l - List all users");
  Serial.println("i - Sensor information");
  Serial.println("s - Send test SMS");
  Serial.println("c - Make test call");
  Serial.println("n - Toggle auto-notify");
  Serial.println("a - Toggle auto-verify");
  Serial.println("b - Toggle LCD backlight");
  Serial.println("t - Show time and date");
  Serial.println("r - Set RTC time");
  Serial.println("p - Print current time");
  Serial.println("w - Toggle time on LCD");
  Serial.println("x - Test LCD display");
  Serial.println("h - Show this menu");
  Serial.println("==========================\n");
}