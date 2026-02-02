# Arduino-based Automated Pill Dispenser

This project is an automated pill dispenser built using an Arduino Uno. It is designed to dispense medication at a regular, user-defined interval. The device features an OLED screen to display the current time and status notifications, a real-time clock (RTC) to maintain time even when powered off, and a confirmation button to acknowledge that the medication has been taken.

## Features

- **Automated Dispensing:** A servo motor activates at a set interval to dispense pills.
- **Real-Time Clock:** A DS1302 RTC module keeps accurate time.
- **OLED Display:** Shows the current time, date, and status messages like "MEDICINE DISPENSED", "Meds Taken", and "Meds Not Taken".
- **Audio-Visual Feedback:** 
    - **LEDs:** Yellow (Pending), Green (Taken), Red (Missed).
    - **Buzzer:** Distinct sounds for dispensing, success, and timeout events.
- **Confirmation System:** After dispensing, the user has a 3-minute window to press a button to confirm they have taken their medication.
- **User Feedback:** The system provides clear visual feedback on the OLED screen based on the user's actions.

## Components Required

- Arduino Uno
- DS1302 Real-Time Clock (RTC) Module
- 128x64 I2C OLED Display (SSD1306 or SH1106)
- Micro Servo (e.g., SG90)
- Piezo Buzzer
- 3x LEDs (Red, Yellow, Green)
- 3x Resistors (220Ω recommended for LEDs)
- Tactile Push Button
- Breadboard
- Jumper Wires

## Wiring and Pinout

Connect the components to the Arduino Uno as follows:

| Component             | Pin on Component | Connection on Arduino Uno |
| --------------------- | :--------------: | :-----------------------: |
| **DS1302 RTC Module** |       VCC        |            5V             |
|                       |       GND        |            GND            |
|                       |       CLK        |         Digital 6         |
|                       |       DAT        |         Digital 7         |
|                       |       RST        |         Digital 8         |
| **OLED Display**      |       VCC        |            5V             |
|                       |       GND        |            GND            |
|                       |       SDA        |         Analog A4         |
|                       |       SCL        |         Analog A5         |
| **Micro Servo**       |   VCC (Red)      |            5V             |
|                       |  GND (Brown)     |            GND            |
|                       | Signal (Orange)  |         Digital 9         |
| **Push Button**       |      Leg 1       |         Digital 10        |
|                       |      Leg 2       |            GND            |
| **LEDs**              |    Red Anode (+) |         Digital 2         |
|                       | Yellow Anode (+) |         Digital 3         |
|                       |  Green Anode (+) |         Digital 4         |
|                       |   Cathodes (-)   |            GND            |
| **Buzzer**            |   Positive (+)   |         Digital 5         |
|                       |   Negative (-)   |            GND            |

## Setup and Installation

1.  **Assemble Hardware:** Connect all the components according to the wiring table above.
2.  **Install Arduino IDE:** If you haven't already, download and install the [Arduino IDE](https://www.arduino.cc/en/software).
3.  **Install Libraries:** You will need to install two libraries.
    - **U8g2:** In the Arduino IDE, go to `Sketch` > `Include Library` > `Manage Libraries...`. Search for `U8g2` and install the library by `oliver`.
    - **virtuabotixRTC:** This library must be installed manually.
        - [Download the library as a ZIP file here.](https://github.com/chrisfryer78/ArduinoRTClibrary/archive/refs/heads/master.zip)
        - In the Arduino IDE, go to `Sketch` > `Include Library` > `Add .ZIP Library...` and select the `ArduinoRTClibrary-master.zip` file you just downloaded.
4.  **Load the Sketch:** Open the `pill_dispenser/pill_dispenser.ino` file in the Arduino IDE.
5.  **Set the Time:**
    - In the `setup()` section of the code, find the line `// myClock.setDS1302Time(...);`.
    - **Uncomment** this line and replace the numbers with the current time and date in the format: `(seconds, minutes, hours, day_of_week, day_of_month, month, year)`.
    - **Upload** the sketch to your Arduino.
    - **IMPORTANT:** Immediately **comment the line out again** and **re-upload** the sketch. This prevents the clock from being reset every time the device powers on.
6.  **Configure Interval (Optional):**
    - Near the top of the sketch, you can change the `dispenseInterval` variable from `5` to any other number to change the minutes between dispenses.
7.  **Upload Final Code:** Upload the final version of the sketch (with the time-setting line commented out).

## How It Works

The dispenser operates on a simple state machine controlled by `millis()` timers to avoid blocking the main loop.

1.  **IDLE:** The device displays the clock. LEDs are OFF.
2.  **DISPENSING:** The servo activates, Yellow LED turns ON, and the Buzzer beeps 3 times. OLED shows "MEDICINE DISPENSED".
3.  **WAITING FOR ACKNOWLEDGEMENT:** The system enters a 3-minute window. Yellow LED remains ON.
4.  **FEEDBACK:**
    - **Success:** If the button is pressed, Green LED turns ON, Buzzer plays a happy melody, and OLED shows "Meds Taken".
    - **Timeout:** If time runs out, Red LED turns ON, Buzzer plays a sad melody, and OLED shows "Meds Not Taken".
