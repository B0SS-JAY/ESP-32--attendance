#include <SoftwareSerial.h>
SoftwareSerial sim(1, 0); // RX, TX
int _timeout;
String _buffer;
String number = "+69562352443";  // change to your number

void setup() {
  Serial.begin(9600);
  _buffer.reserve(100);
  Serial.println("System Started...");
  sim.begin(9600);
  delay(1000);

  Serial.println("Type s to send an SMS, r to receive an SMS, and c to make a call");
  sim.println("AT");
  delay(500);
  sim.println("AT+CMGF=1");  // Text mode
  delay(500);
  sim.println("AT+CNMI=1,2,0,0,0");  // Auto show new SMS directly
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
  sim.println("AT+CMGF=1"); // text mode
  delay(500);
  sim.print("AT+CMGS=\"");
  sim.print(number);
  sim.println("\"");
  delay(500);

  String SMS = "Hello, how are you Julius?";
  sim.println(SMS);
  delay(500);
  sim.write(26);  // CTRL+Z to send
  Serial.println("Waiting for modem response...");

  String response = _readSerial(10000); // wait up to 10s for response

  // Check if the response indicates success
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
