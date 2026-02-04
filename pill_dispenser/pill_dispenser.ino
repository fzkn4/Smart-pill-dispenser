/**
 * Pill Dispenser Project
 * 
 * This sketch controls an automated pill dispenser using:
 * - DS1302 Real Time Clock (RTC) for accurate timing.
 * - Servo Motor for dispensing mechanism.
 * - OLED Display (SSD1306/SH1106) for user feedback.
 * - Button for user acknowledgement.
 * - LEDs and Buzzer for alerts.
 */

#include <Wire.h>               // I2C communication for OLED
#include <U8g2lib.h>            // Graphics library for OLED
#include <virtuabotixRTC.h>     // Library to interact with DS1302 RTC
#include <Servo.h>              // Library to control the Servo motor

// --- Pin Definitions ---
// Connect these pins to the corresponding components on the Arduino
#define CLK_PIN 6               // RTC Clock
#define DAT_PIN 7               // RTC Data
#define RST_PIN 8               // RTC Reset
const int servoPin = 9;         // Servo control signal
const int buttonPin = 10;       // Push button for acknowledgement
const int redLedPin = 2;        // Red LED (Missed/Timeout)
const int yellowLedPin = 3;     // Yellow LED (Pending/Dispensing)
const int greenLedPin = 4;      // Green LED (Success/Taken)
const int buzzerPin = 5;        // Piezo buzzer

// --- Component Objects ---
// Initialize the RTC module
virtuabotixRTC myClock(CLK_PIN, DAT_PIN, RST_PIN);

// Initialize the Servo object
Servo myservo;

// Initialize OLED Display - trying both common driver types (SSD1306 and SH1106)
// The correct one will be selected in setup()
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2_ssd1306(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2_sh1106(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
U8G2 *u8g2 = nullptr; // Pointer to the active display object

// --- Servo Settings ---
const int naturalPosition = 0;      // Resting position of the servo (Closed)
const int dispensePosition = 180;   // Dispensing position of the servo (Open)
const int servoDelay = 15;          // Delay in ms per degree for smooth movement animation

// --- State Machine ---
// The system operates in one of these states to manage the flow logically
enum DispenseState { 
  IDLE,             // Waiting for the next scheduled time
  DISPENSING,       // Currently moving the motor to dispense
  WAITING_FOR_ACK,  // Meds dispensed, waiting for user to press button
  ACKNOWLEDGED,     // User pressed button (Success)
  TIMED_OUT         // User did not press button in time (Failure)
};
DispenseState currentState = IDLE;  // Start in IDLE state
unsigned long stateChangeTime = 0;  // Tracks when the last state change happened (for timers)

// --- Timing Constants (in milliseconds) ---
const unsigned long dispenseMessageDuration = 5000; // How long to show "Dispensed" message
const unsigned long ackWindowDuration = 30000;      // How long to wait for user input (30 seconds)
const unsigned long feedbackMessageDuration = 5000; // How long to show Success/Fail message

// --- Timed Dispense Logic ---
const int dispenseInterval = 1;     // Interval in Minutes (e.g., 1 = every minute, 60 = every hour)
int lastMinuteDispensed = -1;       // Memory to ensure we only dispense once per interval
unsigned long lastSerialPrintTime = 0; // Timer for debug output


void setup() {
  Serial.begin(115200);
  Serial.println("Pill Dispenser: Starting...");

  // Initialize Input/Output Pins
  pinMode(buttonPin, INPUT_PULLUP); // Use internal pullup resistor for button
  pinMode(redLedPin, OUTPUT);
  pinMode(yellowLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  // --- RTC Configuration ---
  // To set the time, uncomment the line below, upload, then comment it out and re-upload.
  // Format: seconds, minutes, hours, day_of_week(1=Sun), day_of_month, month, year
  // myClock.setDS1302Time(00, 48, 6, 3, 3, 2, 2026); 

  // --- Servo Initialization ---
  myservo.attach(servoPin);
  myservo.write(naturalPosition); // Reset to closed position start
  Serial.println("Servo initialized.");

  // --- OLED Initialization ---
  Wire.begin();
  // Try to initialize SSD1306 driver first
  if (u8g2_ssd1306.begin()) {
    u8g2 = &u8g2_ssd1306;
    Serial.println("✓ SUCCESS: SSD1306 controller initialized.");
  } 
  // Fallback to SH1106 driver if the first one fails
  else if (u8g2_sh1106.begin()) {
    u8g2 = &u8g2_sh1106;
    Serial.println("✓ SUCCESS: SH1106 controller initialized.");
  } else {
    u8g2 = nullptr;
    Serial.println("✗ FAILED: OLED controller failed.");
  }

  // Enable UTF8 support if display is working
  if (u8g2 != nullptr) {
    u8g2->enableUTF8Print();
    Serial.println("Display initialized. Starting main loop...");
  }
}

// --- Drawing Functions ---
// --- Drawing Helper: Displays two lines of centered text ---
void drawMessage(const char* msg1, const char* msg2 = "") {
  if (u8g2 == nullptr) return;
  u8g2->firstPage(); // Begin OLED picture loop
  do {
    u8g2->setFont(u8g2_font_ncenB10_tr); // Set a font
    
    // Calculate position to center the first line
    u8g2_uint_t msg1Width = u8g2->getUTF8Width(msg1);
    u8g2_uint_t msg1X = (u8g2->getDisplayWidth() - msg1Width) / 2;
    u8g2->drawUTF8(msg1X, 28, msg1);

    // If a second line exists, calculate center and draw it
    if (strlen(msg2) > 0) {
      u8g2_uint_t msg2Width = u8g2->getUTF8Width(msg2);
      u8g2_uint_t msg2X = (u8g2->getDisplayWidth() - msg2Width) / 2;
      u8g2->drawUTF8(msg2X, 52, msg2);
    }
  } while (u8g2->nextPage());
}

// --- Drawing Helper: Displays current Date and Time ---
void drawClock() {
  if (u8g2 == nullptr) return;
  
  // Format Time String: HH:MM:SS
  char timeStr[9];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", myClock.hours, myClock.minutes, myClock.seconds);
  
  // Format Date String: YYYY/MM/DD
  char dateStr[11];
  snprintf(dateStr, sizeof(dateStr), "%04d/%02d/%02d", myClock.year, myClock.month, myClock.dayofmonth);
  
  drawMessage(timeStr, dateStr);
}

// --- Sound Helper: Plays different tones based on event type ---
void playSound(int type) {
  // Type 1: Dispensing Alert (Three beeps)
  if (type == 1) {
    for (int i = 0; i < 3; i++) {
      tone(buzzerPin, 1000); // 1kHz tone
      delay(200);
      noTone(buzzerPin);
      delay(100);
    }
  }
  // Type 2: Taken / Success (Ascending happy melody)
  else if (type == 2) {
    tone(buzzerPin, 1000); delay(150);
    tone(buzzerPin, 1500); delay(150);
    tone(buzzerPin, 2000); delay(300);
    noTone(buzzerPin);
  }
  // Type 3: Timeout / Failure (Descending sad melody)
  else if (type == 3) {
    tone(buzzerPin, 500); delay(200);
    tone(buzzerPin, 400); delay(200);
    tone(buzzerPin, 300); delay(400);
    noTone(buzzerPin);
  }
}

// --- Servo Helper: Moves the servo slowly to avoid jarring ---
void moveSmooth(int start, int end) {
  if (start < end) {
    for (int pos = start; pos <= end; pos++) {
      myservo.write(pos);
      delay(servoDelay); // Small delay creates smooth motion
    }
  } else {
    for (int pos = start; pos >= end; pos--) {
      myservo.write(pos);
      delay(servoDelay);
    }
  }
}

// --- Main State Machine and Loop ---
// --- Main State Machine and Loop ---
void loop() {
  myClock.updateTime(); // Read the latest time from RTC

  // --- State Machine Logic ---
  // Controls the flow of the application based on 'currentState'
  switch (currentState) {
    
    // State 1: IDLE - Waiting for the right time
    case IDLE:
      // Check if the current minute is a multiple of the interval AND we haven't dispensed yet this minute
      if ((myClock.minutes % dispenseInterval == 0) && (myClock.minutes != lastMinuteDispensed)) {
        Serial.println("--- DISPENSING ROUTINE STARTED ---");
        
        playSound(1); // Play alert sound
        
        // Dispense Pill: Move Servo Open
        moveSmooth(naturalPosition, dispensePosition);
        
        delay(1000); // Wait 1 second for pills to fall
        
        // Return Servo to Closed
        moveSmooth(dispensePosition, naturalPosition);
        
        // Update tracking to prevent double dispensing in the same minute
        lastMinuteDispensed = myClock.minutes;
        
        // Transition to next state
        currentState = DISPENSING;
        stateChangeTime = millis();
      }
      break;

    // State 2: DISPENSING - Showing "Dispensed" message
    case DISPENSING:
      // Keep message for set duration, then move to waiting
      if (millis() - stateChangeTime >= dispenseMessageDuration) {
        currentState = WAITING_FOR_ACK;
        stateChangeTime = millis(); // Reset timer for the next state
      }
      break;

    // State 3: WAITING FOR ACKNOWLEDGEMENT - Waiting for user to press button
    case WAITING_FOR_ACK:
      // Condition A: User Presses Button
      if (digitalRead(buttonPin) == LOW) {
        Serial.println("--- Meds Taken (User Confirmed) ---");
        myservo.write(naturalPosition); // Safety check: ensure servo is closed
        
        currentState = ACKNOWLEDGED; // Move to Success state
        stateChangeTime = millis();
        playSound(2); // Play Success Sound
      }
      // Condition B: Time Runs Out
      else if (millis() - stateChangeTime >= ackWindowDuration) {
        Serial.println("--- Meds Not Taken (Timeout Reached) ---");
        
        currentState = TIMED_OUT; // Move to Failure state
        stateChangeTime = millis();
        playSound(3); // Play Failure Sound
      }
      break;

    // State 4: ACKNOWLEDGED - Success Feedback
    case ACKNOWLEDGED:
      // Show "Meds Taken" for a few seconds, then go back to IDLE
      if (millis() - stateChangeTime >= feedbackMessageDuration) {
        currentState = IDLE;
      }
      break;

    // State 5: TIMED OUT - Failure Feedback
    case TIMED_OUT:
      // Show "Meds Not Taken" for a few seconds, then go back to IDLE
      if (millis() - stateChangeTime >= feedbackMessageDuration) {
        currentState = IDLE;
      }
      break;
  }

  // --- Display Drawing Logic ---
  // Updates the OLED based on the current state
  switch (currentState) {
    case IDLE:
    case WAITING_FOR_ACK: 
      drawClock(); // Show Date/Time when idle or waiting
      break;
    case DISPENSING:
      drawMessage("MEDICINE", "DISPENSED 💊");
      break;
    case ACKNOWLEDGED:
      drawMessage("Meds Taken", "👍");
      break;
    case TIMED_OUT:
      drawMessage("Meds Not Taken", "!");
      break;
  }

  // --- LED Logic ---
  // Updates status LEDs based on the current state
  switch (currentState) {
    case IDLE:
      // All LEDs OFF
      digitalWrite(redLedPin, LOW);
      digitalWrite(yellowLedPin, LOW);
      digitalWrite(greenLedPin, LOW);
      break;
      
    case DISPENSING:
    case WAITING_FOR_ACK:
      // Yellow LED ON (Pending)
      digitalWrite(redLedPin, LOW);
      digitalWrite(yellowLedPin, HIGH);
      digitalWrite(greenLedPin, LOW);
      break;
      
    case ACKNOWLEDGED:
      // Green LED ON (Success)
      digitalWrite(redLedPin, LOW);
      digitalWrite(yellowLedPin, LOW);
      digitalWrite(greenLedPin, HIGH);
      break;
      
    case TIMED_OUT:
      // Red LED ON (Failure)
      digitalWrite(redLedPin, HIGH);
      digitalWrite(yellowLedPin, LOW);
      digitalWrite(greenLedPin, LOW);
      break;
  }

  // --- Serial printing (Debug) ---
  // Prints status once per second to the Serial Monitor
  if (millis() - lastSerialPrintTime >= 1000) {
    lastSerialPrintTime = millis();
    Serial.print("State: ");
    Serial.print(currentState);
    Serial.print(" | Time: ");
    Serial.print(myClock.hours);
    Serial.print(":");
    Serial.println(myClock.minutes);
  }
}
