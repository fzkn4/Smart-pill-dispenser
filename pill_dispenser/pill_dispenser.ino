#include <Wire.h>
#include <U8g2lib.h>
#include <virtuabotixRTC.h>

// RTC Pin definitions for the DS1302 module
#define CLK_PIN 6
#define DAT_PIN 7
#define RST_PIN 8

// Create an RTC object for the DS1302 with a unique name to avoid library conflicts
virtuabotixRTC myClock(CLK_PIN, DAT_PIN, RST_PIN);

// --- OLED Display Setup ---
// U8g2 Constructor for SSD1306
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2_ssd1306(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
// U8g2 Constructor for SH1106
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2_sh1106(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Pointer to the active U8g2 display object
U8G2 *u8g2 = nullptr;

void setup() {
  Serial.begin(115200);
  Serial.println("Pill Dispenser: Starting...");

  // --- RTC Setup ---
  // This block is for setting the time on the DS1302. To set the time:
  // 1. Uncomment the following line.
  // 2. Replace the values with the current date and time (seconds, minutes, hours, day_of_week, day_of_month, month, year).
  // 3. Upload the sketch to your Arduino.
  // 4. IMPORTANT: Comment the line out again and re-upload the sketch.
  // myClock.setDS1302Time(0, 50, 11, 3, 28, 1, 2026); // S, M, H, DoW, DoM, M, Y

  // --- OLED Setup ---
  Wire.begin();
  delay(500);

  // Try initializing with SSD1306 controller
  if (u8g2_ssd1306.begin()) {
    u8g2 = &u8g2_ssd1306;
    Serial.println("✓ SUCCESS: SSD1306 controller initialized.");
  } 
  // If SSD1306 fails, try SH1106
  else if (u8g2_sh1106.begin()) {
    u8g2 = &u8g2_sh1106;
    Serial.println("✓ SUCCESS: SH1106 controller initialized.");
  } else {
    u8g2 = nullptr;
    Serial.println("✗ FAILED: Both SSD1306 and SH1106 controllers failed to initialize.");
    Serial.println("Please check OLED wiring and I2C address.");
  }

  if (u8g2 != nullptr) {
    Serial.println("Display initialized. Starting main loop...");
  }
}

void loop() {
  // Get the current time from the RTC module
  myClock.updateTime();

  // Serial printing (for debugging)
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
  
  // OLED Display Update
  if (u8g2 != nullptr) {
    char timeStr[9]; // HH:MM:SS
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", myClock.hours, myClock.minutes, myClock.seconds);

    char dateStr[11]; // YYYY/MM/DD
    snprintf(dateStr, sizeof(dateStr), "%04d/%02d/%02d", myClock.year, myClock.month, myClock.dayofmonth);

    u8g2->firstPage();
    do {
      // Draw Time
      u8g2->setFont(u8g2_font_ncenB12_tr); // A good font for the time
      u8g2_uint_t timeWidth = u8g2->getStrWidth(timeStr);
      u8g2_uint_t timeX = (u8g2->getDisplayWidth() - timeWidth) / 2;
      u8g2->drawStr(timeX, 28, timeStr);

      // Draw Date
      u8g2->setFont(u8g2_font_7x13B_tr); // A smaller font for the date
      u8g2_uint_t dateWidth = u8g2->getStrWidth(dateStr);
      u8g2_uint_t dateX = (u8g2->getDisplayWidth() - dateWidth) / 2;
      u8g2->drawStr(dateX, 52, dateStr);

    } while (u8g2->nextPage());
  }
  
  delay(1000);
}
