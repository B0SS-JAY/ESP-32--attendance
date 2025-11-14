#include <Adafruit_Fingerprint.h>

HardwareSerial FingerSerial(2);  // UART2 (GPIO16/17)
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&FingerSerial);

void setup() {
  Serial.begin(9600);
  delay(1000);

  FingerSerial.begin(57600, SERIAL_8N1, 16, 17);

  Serial.println("Searching for fingerprint sensor...");

  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor detected!");
  } else {
    Serial.println("No fingerprint sensor found.");
    while (1);
  }
}

void loop() {
  Serial.println("\n--- Fingerprint Enrollment ---");
  Serial.print("Enter ID # (1 - 127): ");

int enrollID = -1;
while (enrollID <= 0 || enrollID > 127) {
  Serial.println("Enter ID # (1–127): ");

  while (!Serial.available());  
  enrollID = Serial.parseInt();

  Serial.print("You entered: ");
  Serial.println(enrollID);
}

  Serial.print("Enrolling ID: ");
  Serial.println(enrollID);

  enrollFingerprint(enrollID);
}

void enrollFingerprint(int id) {
  int p = -1;
  Serial.println("Place finger on sensor...");

  // STEP 1: Wait for first image
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }

  // Convert first image
  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    Serial.println("Error converting first image.");
    return;
  }

  Serial.println("Remove finger...");
  delay(2000);

  // STEP 2: Wait for second scan
  Serial.println("Place the same finger again...");
  p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }

  // Convert second image
  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    Serial.println("Error converting second image.");
    return;
  }

  // STEP 3: Create model (combine two scans)
  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    Serial.println("Fingerprints do not match! Try again.");
    return;
  }

  // STEP 4: Store template in flash memory
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Enrollment success!");
  } else {
    Serial.println("Failed to store fingerprint.");
  }
}


