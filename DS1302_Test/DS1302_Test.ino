/*
  MH-Real-Time Clock Module 2 - DS1302 RTC Module
  
  This sketch displays the current time from the DS1302 RTC module in the serial terminal.

  Pin connections for Arduino Uno:
  - VCC: 5V
  - GND: GND
  - CLK: Digital Pin 6
  - DAT: Digital Pin 7
  - RST: Digital Pin 8
*/

#include <virtuabotixRTC.h>

// Define the pins used for the RTC module
#define CLK_PIN 6
#define DAT_PIN 7
#define RST_PIN 8

// Create an RTC object
virtuabotixRTC myRTC(CLK_PIN, DAT_PIN, RST_PIN);

void setup() {
  Serial.begin(9600);

  // The line below was used to set the initial time. It is now commented out
  // to prevent resetting the time every time the Arduino starts.
  // myRTC.setDS1302Time(0, 48, 9, 4, 28, 1, 2026); 
}

void loop() {
  // Get the current time from the RTC module
  myRTC.updateTime();

  // Print the time to the serial monitor
  Serial.print("Current Time: ");
  Serial.print(myRTC.year);
  Serial.print("/");
  Serial.print(myRTC.month);
  Serial.print("/");
  Serial.print(myRTC.dayofmonth);
  Serial.print(" ");
  Serial.print(myRTC.hours);
  Serial.print(":");
  Serial.print(myRTC.minutes);
  Serial.print(":");
  Serial.println(myRTC.seconds);

  delay(1000); // Wait for a second
}
