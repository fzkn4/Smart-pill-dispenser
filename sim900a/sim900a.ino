#include <SoftwareSerial.h>

// Configure SoftwareSerial using Pin 7 (RX) and Pin 8 (TX)
// Connect SIM900A TX to Arduino Pin 7
// Connect SIM900A RX to Arduino Pin 8
SoftwareSerial sim900(7, 8); 

void setup() {
  // Initialize Serial Monitor for debugging
  Serial.begin(9600);
  Serial.println("System Started...");

  // Initialize GSM Module
  sim900.begin(9600);
  
  // Give time for the GSM module to stabilize/connect to network
  Serial.println("Initializing GSM module (Waiting 5s)...");
  delay(5000); 

  // Send the SMS once
  sendSMS();
}

void loop() {
  // Use loop to read responses from SIM900A or send manual AT commands from Serial Monitor
  if (sim900.available()) {
    Serial.write(sim900.read());
  }
  if (Serial.available()) {
    sim900.write(Serial.read());
  }
}

void sendSMS() {
  Serial.println("Sending SMS...");

  // Set SMS mode to ASCII
  sim900.println("AT+CMGF=1"); 
  delay(1000);
  
  // REPLACE THE NUMBER BELOW WITH THE ACTUAL PH NUMBER
  sim900.println("AT+CMGS=\"+639753483020\""); 
  delay(1000);
  
  // The SMS content
  sim900.println("Hello from Arduino Pill Dispenser!"); 
  delay(100);
  
  // End command with CTRL+Z (ASCII 26)
  sim900.println((char)26); 
  delay(1000);

  Serial.println("SMS Sent!");
}
