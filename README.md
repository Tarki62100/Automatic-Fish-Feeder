
### Automated/Manual Fish Feeder (ESP32)

A dual-mode automated fish feeding system built with an ESP32. This project features a physical TFT display menu, hardware PWM servo control for precise feeding mechanics, and a local embedded web server for remote control over WiFi.
    **(WARNING:You can adjust line 69 to make sure the amount of feed dropped is adequate for your fish)**

## Key Features

* **Dual Operating Modes:** 
  * **WiFi Mode:** Connects to your local network and hosts an embedded web server. Allows you to feed the fish remotely via a browser and view the time elapsed since the last feeding.
  * **Offline (Physical) Mode:** Functions entirely without WiFi. Set feeding intervals (from 15 seconds to 48 hours) directly using the physical buttons and TFT interface.
* **State Machine Architecture:** Menu navigation and screen rendering are handled via a non-blocking state machine, ensuring the web server and hardware interrupts run smoothly in the background.
* **Custom Homing System:** Designed to work with a physical "penny wall" limit switch mechanism to ensure the gravity-fed container always returns to the perfect center axis.

## Hardware Requirements

* **Microcontroller:** ESP32 Development Board
* **Display:** TFT Display (SPI) compatible with `TFT_eSPI` library
* **Motor:** MSG90 Micro Servo (or similar PWM servo)
* **Inputs:** 2x Tactile Push Buttons (Select & Confirm)
* **Misc:** Breadboard, Jumper Wires, Custom 3D Printed / DIY Container

##  Pin Configuration

| Component | ESP32 Pin | Note |
| :--- | :--- | :--- |
| **Servo Motor** | `GPIO 27` | Controlled via 50Hz PWM | (Pressing feed multiple times in a row might result in the servo overheating so avoid it)
| **Select Button** | `GPIO 0` | Input Pullup (Avoid pressing during boot) |
| **Confirm Button** | `GPIO 35` | Input Pullup |
| **TFT Backlight** | `GPIO 4` | Output High |

*(Note: Standard SPI pins for the TFT display are configured inside the `TFT_eSPI` library's `User_Setup.h` file).*

## Software Dependencies

Ensure you have the following libraries installed in your Arduino IDE:
* [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) by Bodmer (Requires correct `User_Setup.h` configuration for your specific screen)
* Native ESP32 Libraries: `WiFi.h`, `WebServer.h`, `FS.h`, `SPI.h`

## Installation & Setup

1. Clone this repository:
   ```bash
   git clone https://github.com/Tarki62100/Automatic-Fish-Feeder
   
2.Open the fish_feeder.ino file in the arduino ide

3.Update the Wi-Fi credentials (ssid and password) and the web server login details at the top of the file.

4.Upload into your esp32 microcontroller

