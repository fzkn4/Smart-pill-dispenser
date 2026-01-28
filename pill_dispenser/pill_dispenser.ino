#include <Wire.h>
#include <U8g2lib.h>
#include <virtuabotixRTC.h>
#include <Servo.h>

// --- RTC Pin definitions ---
#define CLK_PIN 6
#define DAT_PIN 7
#define RST_PIN 8
virtuabotixRTC myClock(CLK_PIN, DAT_PIN, RST_PIN);

// --- Servo definitions ---
Servo myservo;
const int servoPin = 9;
const int naturalPosition = 10;
const int dispensePosition = 180;

// --- Timed Dispense definitions ---
const int dispenseInterval = 5; // How many minutes between dispensing
int lastMinuteDispensed = -1;   // Tracks the last minute we dispensed to avoid repeats

// --- OLED Display Setup ---
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2_ssd1306(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2_sh1106(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
U8G2 *u8g2 = nullptr;

void setup() {
  Serial.begin(115200);
  Serial.println("Pill Dispenser: Starting...");

  // --- RTC Setup ---
  // myClock.setDS1302Time(0, 50, 11, 3, 28, 1, 2026); // S, M, H, DoW, DoM, M, Y

  // --- Servo Setup ---
  myservo.attach(servoPin);
  myservo.write(naturalPosition); // Start at the natural position
  Serial.println("Servo initialized.");

  // --- OLED Setup ---
  Wire.begin();
  delay(500);

  if (u8g2_ssd1306.begin()) {
    u8g2 = &u8g2_ssd1306;
    Serial.println("✓ SUCCESS: SSD1306 controller initialized.");
  } else if (u8g2_sh1106.begin()) {
    u8g2 = &u8g2_sh1106;
    Serial.println("✓ SUCCESS: SH1106 controller initialized.");
  } else {
    u8g2 = nullptr;
    Serial.println("✗ FAILED: Both SSD1306 and SH1106 controllers failed to initialize.");
  }

  if (u8g2 != nullptr) {
    Serial.println("Display initialized. Starting main loop...");
  }
}

void dispensePill() {
  Serial.println("--- DISPENSING ---");
  myservo.write(dispensePosition);
  delay(1000); // Keep it open for 1 second
  myservo.write(naturalPosition);
  Serial.println("--- Servo returned to natural position ---");
}

void loop() {
  myClock.updateTime();

  // --- Timed Dispense Check ---
  // Check if the current minute is a multiple of the interval and we haven't already dispensed this minute
  if ((myClock.minutes % dispenseInterval == 0) && (myClock.minutes != lastMinuteDispensed)) {
    dispensePill();
    lastMinuteDispensed = myClock.minutes; // Record that we dispensed this minute
  }

  // --- Serial printing (for debugging) ---
  Serial.print("Current Time: ");
  Serial.print(myClock.year);
  Serial.print("/");
  Serial.print(myClock.month);
  Serial.print("/");
  Serial.print(myClock.dayofmonth);
  Serial.print(" ");
  Serial.print(myClock.hours);
  Serial.print(":");
  if(myClock.minutes < 10) Serial.print('0');
  Serial.print(myClock.minutes);
  Serial.print(":");
  if(myClock.seconds < 10) Serial.print('0');
  Serial.print(myClock.seconds);
  Serial.println();
  
  // --- OLED Display Update ---
  if (u8g2 != nullptr) {
    char timeStr[9]; // HH:MM:SS
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", myClock.hours, myClock.minutes, myClock.seconds);
    char dateStr[11]; // YYYY/MM/DD
    snprintf(dateStr, sizeof(dateStr), "%04d/%02d/%02d", myClock.year, myClock.month, myClock.dayofmonth);

    u8g2->firstPage();
    do {
      u8g2->setFont(u8g2_font_ncenB12_tr);
      u8g2_uint_t timeWidth = u8g2->getStrWidth(timeStr);
      u8g2_uint_t timeX = (u8g2->getDisplayWidth() - timeWidth) / 2;
      u8g2->drawStr(timeX, 28, timeStr);

      u8g2->setFont(u8g2_font_7x13B_tr);
      u8g2_uint_t dateWidth = u8g2->getStrWidth(dateStr);
      u8g2_uint_t dateX = (u8g2->getDisplayWidth() - dateWidth) / 2;
      u8g2->drawStr(dateX, 52, dateStr);
    } while (u8g2->nextPage());
  }
  
  delay(1000);
}
