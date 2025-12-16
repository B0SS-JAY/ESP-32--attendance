#include <HardwareSerial.h>
#include <Arduino.h>

// Functions
void SendMessage();
void RecieveMessage();
void callNumber();
String _readSerial(unsigned long timeout);

// Use UART1 - MUST remap pins (default 9/10 are for flash)
HardwareSerial sim(1);

// Choose available GPIO pins for UART1 remapping
#define RX_PIN 25  // GPIO25 as RX (connect to SIM800L TX)
#define TX_PIN 26  // GPIO26 as TX (connect to SIM800L RX)

String _buffer;
String number = "+639562352443";

void setup() {
  Serial.begin(9600);
  _buffer.reserve(100);
  Serial.println("System Started...");
  
  // Initialize UART1 with remapped pins: begin(baud, config, RX, TX)
  sim.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(1000);
  
  Serial.println("Type s to send an SMS, r to receive an SMS, and c to make a call");
  
  // Initialize SIM800L
  sim.println("AT");
  delay(500);
  sim.println("AT+CMGF=1");
  delay(500);
  sim.println("AT+CNMI=1,2,0,0,0");
  delay(500);
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    switch (cmd) {
      case 's':
        SendMessage();
        break;
      case 'r':
        RecieveMessage();
        break;
      case 'c':
        callNumber();
        break;
    }
  }
  
  if (sim.available()) {
    Serial.write(sim.read());
  }
}

void SendMessage() {
  Serial.println("Sending Message...");
  sim.println("AT+CMGF=1");
  delay(500);
  sim.print("AT+CMGS=\"");
  sim.print(number);
  sim.println("\"");
  delay(500);
  String SMS = "Hello, how are you?";
  sim.println(SMS);
  delay(500);
  sim.write(26);
  
  Serial.println("Waiting for modem response...");
  String response = _readSerial(10000);
  
  if (response.indexOf("+CMGS:") != -1 && response.indexOf("OK") != -1) {
    Serial.println("✅ Message sent successfully!");
  } else if (response.indexOf("ERROR") != -1) {
    Serial.println("❌ Failed to send message!");
  } else {
    Serial.println("⚠️ No clear response from module.");
  }
  Serial.println("Modem reply:");
  Serial.println(response);
}

void RecieveMessage() {
  Serial.println("SIM800L Read an SMS");
  sim.println("AT+CMGF=1");
  delay(200);
  sim.println("AT+CNMI=1,2,0,0,0");
  delay(200);
  Serial.println("Ready to receive SMS...");
}

void callNumber() {
  Serial.println("Calling...");
  sim.print("ATD");
  sim.print(number);
  sim.println(";");
  String response = _readSerial(5000);
  Serial.println("Response:");
  Serial.println(response);
}

String _readSerial(unsigned long timeout) {
  unsigned long startTime = millis();
  String buffer = "";
  while (millis() - startTime < timeout) {
    while (sim.available()) {
      char c = sim.read();
      buffer += c;
    }
  }
  return buffer;
}