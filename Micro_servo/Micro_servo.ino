#include <Servo.h>

Servo myservo;  // Create servo object
int pos = 10;   // Starting position

void setup() {
  myservo.attach(9);  // Attach to pin 9
  myservo.write(pos); // Center initially
}

void loop() {
  // Move to the 90-degree position
  myservo.write(180);
  delay(1000);       // Wait for 1 second

  // Move back to the starting position
  myservo.write(10);
  delay(1000);       // Wait for 1 second
}
