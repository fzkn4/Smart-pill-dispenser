#include <Wire.h>
#include <U8g2lib.h>
#include <virtuabotixRTC.h>
#include <Servo.h>

// --- Pin definitions ---
#define CLK_PIN 6
#define DAT_PIN 7
#define RST_PIN 8
const int servoPin = 9;
const int buttonPin = 10;

// --- Component Objects ---
virtuabotixRTC myClock(CLK_PIN, DAT_PIN, RST_PIN);
Servo myservo;
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2_ssd1306(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2_sh1106(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
U8G2 *u8g2 = nullptr;

// --- Servo settings ---
const int naturalPosition = 10;
const int dispensePosition = 180;

// --- State Machine ---
enum DispenseState { IDLE, DISPENSING, WAITING_FOR_ACK, ACKNOWLEDGED, TIMED_OUT };
DispenseState currentState = IDLE;
unsigned long stateChangeTime = 0; // Generic timer for state transitions

// --- Timing Constants (in milliseconds) ---
const unsigned long dispenseMessageDuration = 5000; // 5s for "Dispensed" msg
const unsigned long ackWindowDuration = 180000;     // 3 minutes to acknowledge
const unsigned long feedbackMessageDuration = 5000; // 5s for "Taken"/"Not Taken"

// --- Timed Dispense Logic ---
const int dispenseInterval = 5; // How many minutes between dispensing
int lastMinuteDispensed = -1;   // Tracks the last minute to avoid repeats
unsigned long lastSerialPrintTime = 0; // Timer for serial print

void setup() {
  Serial.begin(115200);
  Serial.println("Pill Dispenser: Starting...");

  pinMode(buttonPin, INPUT_PULLUP);

  // myClock.setDS1302Time(0, 0, 12, 1, 1, 2024); // S, M, H, DoW, DoM, M, Y

  myservo.attach(servoPin);
  myservo.write(naturalPosition);
  Serial.println("Servo initialized.");

  Wire.begin();
  if (u8g2_ssd1306.begin()) {
    u8g2 = &u8g2_ssd1306;
    Serial.println("✓ SUCCESS: SSD1306 controller initialized.");
  } else if (u8g2_sh1106.begin()) {
    u8g2 = &u8g2_sh1106;
    Serial.println("✓ SUCCESS: SH1106 controller initialized.");
  } else {
    u8g2 = nullptr;
    Serial.println("✗ FAILED: OLED controller failed.");
  }

  if (u8g2 != nullptr) {
    u8g2->enableUTF8Print();
    Serial.println("Display initialized. Starting main loop...");
  }
}

// --- Drawing Functions ---
void drawMessage(const char* msg1, const char* msg2 = "") {
  if (u8g2 == nullptr) return;
  u8g2->firstPage();
  do {
    u8g2->setFont(u8g2_font_ncenB10_tr);
    u8g2_uint_t msg1Width = u8g2->getUTF8Width(msg1);
    u8g2_uint_t msg1X = (u8g2->getDisplayWidth() - msg1Width) / 2;
    u8g2->drawUTF8(msg1X, 28, msg1);

    if (strlen(msg2) > 0) {
      u8g2_uint_t msg2Width = u8g2->getUTF8Width(msg2);
      u8g2_uint_t msg2X = (u8g2->getDisplayWidth() - msg2Width) / 2;
      u8g2->drawUTF8(msg2X, 52, msg2);
    }
  } while (u8g2->nextPage());
}

void drawClock() {
  if (u8g2 == nullptr) return;
  char timeStr[9];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", myClock.hours, myClock.minutes, myClock.seconds);
  char dateStr[11];
  snprintf(dateStr, sizeof(dateStr), "%04d/%02d/%02d", myClock.year, myClock.month, myClock.dayofmonth);
  drawMessage(timeStr, dateStr);
}

// --- Main State Machine and Loop ---
void loop() {
  myClock.updateTime();

  // --- State Machine Logic ---
  switch (currentState) {
    case IDLE:
      // Check if it's time to dispense
      if ((myClock.minutes % dispenseInterval == 0) && (myClock.minutes != lastMinuteDispensed)) {
        Serial.println("--- DISPENSING ---");
        myservo.write(dispensePosition);
        delay(1000); // Block for 1s for servo action
        myservo.write(naturalPosition);
        
        lastMinuteDispensed = myClock.minutes;
        currentState = DISPENSING;
        stateChangeTime = millis();
      }
      break;

    case DISPENSING:
      // Show "Dispensed" message for 5 seconds
      if (millis() - stateChangeTime >= dispenseMessageDuration) {
        currentState = WAITING_FOR_ACK;
        stateChangeTime = millis(); // Reset timer for the next state
      }
      break;

    case WAITING_FOR_ACK:
      // Check if button was pressed
      if (digitalRead(buttonPin) == LOW) {
        Serial.println("--- Meds Taken ---");
        currentState = ACKNOWLEDGED;
        stateChangeTime = millis();
      }
      // Check if 3-minute window has passed
      else if (millis() - stateChangeTime >= ackWindowDuration) {
        Serial.println("--- Meds Not Taken (Timeout) ---");
        currentState = TIMED_OUT;
        stateChangeTime = millis();
      }
      break;

    case ACKNOWLEDGED:
      // Show "Meds Taken" message for 5 seconds
      if (millis() - stateChangeTime >= feedbackMessageDuration) {
        currentState = IDLE;
      }
      break;

    case TIMED_OUT:
      // Show "Meds Not Taken" message for 5 seconds
      if (millis() - stateChangeTime >= feedbackMessageDuration) {
        currentState = IDLE;
      }
      break;
  }

  // --- Display Drawing Logic ---
  switch (currentState) {
    case IDLE:
    case WAITING_FOR_ACK: // Show clock while waiting
      drawClock();
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

  // --- Serial printing (for debugging, once per second) ---
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
