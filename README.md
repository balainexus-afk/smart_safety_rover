
# Smart Safety Rover for Inspection

A LoRa controlled rover that drives into hazardous areas, such as coal mines, to check gas levels and avoid obstacles, while an operator watches live readings on a display and a web dashboard.

## 📌 Overview

Sending a person into a mine or a closed space to check for dangerous gas is risky. This project places the sensors on a small rover instead. The operator stays at a safe distance, drives the rover with four buttons on a handheld transmitter, and reads gas levels on an OLED display and on a web dashboard. If gas crosses the alert level, a buzzer sounds.

The system has two ESP32 units that talk to each other over LoRa radio.

## ✨ Features

* **Wireless control:** Forward, backward, left and right commands travel over 433 MHz LoRa.
* **Gas monitoring:** An MQ6 sensor on the rover and an MQ9 sensor on the transmitter side, with an alert level of 500 ppm.
* **Obstacle stop:** A VL53L0X laser distance sensor stops the rover when something is closer than 10 cm.
* **Safety stop:** The rover stops if no button is pressed or if more than one button is pressed.
* **Live OLED display:** Shows gas readings, gas status and GPS status.
* **Buzzer alert:** Sounds in short beeps when gas is high.
* **Web dashboard:** A page called Coal Mine Safety Dashboard with live graphs for both sensors, a 500 ppm threshold line, a safety status box and a full screen emergency alert when both sensors are high.
* **GPS module:** Added on the transmitter for location data.
* 


## 🧰 Components Used

**Rover (receiver)**
* ESP32 DevKit
* LoRa module (433 MHz, SX127x family)
* L298N motor driver
* Four wheel acrylic chassis with DC gear motors
* VL53L0X laser distance sensor
* MQ6 gas sensor, connected through a 22k and 22k voltage divider
* Two 18650 lithium cells with holder
* DC to DC converter module
* Breadboards and jumper wires

**Transmitter (operator unit)**
* ESP32 DevKit, powered by USB
* LoRa module (433 MHz, SX127x family)
* Four push buttons (forward, backward, left, right)
* SH1106 128x64 OLED display
* MQ9 gas sensor
* GPS module
* Buzzer
* Breadboard, resistors and jumper wires

## 🔌 Pin Connections

**Transmitter**
* LoRa: SCK 18, MISO 19, MOSI 23, SS 5, RST 14, DIO0 26
* Buttons: forward 32, backward 33, left 25, right 27
* OLED: SDA 21, SCL 22
* Buzzer: 13
* GPS: RX pin 34, TX pin 15
* MQ9: 35

**Rover**
* LoRa: SCK 18, MISO 19, MOSI 23, SS 5, RST 14, DIO0 26
* L298N: IN1 25, IN2 27, IN3 32, IN4 33
* VL53L0X: SDA 21, SCL 22
* MQ6: pin 34 through the voltage divider

## ⚙️ How It Works

1. The operator presses a button on the transmitter. It sends a command over LoRa.
2. The rover receives the command and drives, unless the distance sensor sees an obstacle closer than 10 cm.
3. After each command, the rover sends back a gas packet with the estimated ppm, the sensor voltage and an alert flag.
4. The transmitter shows the readings on the OLED, sounds the buzzer when gas is high and serves a live web dashboard over WiFi.
5. The MQ9 sensor is calibrated in clean air for 30 seconds each time the transmitter starts.

**Please note:** the ppm values are estimates from a basic sensor formula, not a calibrated gas measurement. Do not use this prototype as a certified safety device.

## 🚀 Getting Started

1. Clone this repository.
2. Install the ESP32 board package and these libraries in Arduino IDE: **LoRa** (by Sandeep Mistry), **U8g2**, **TinyGPSPlus** and **Adafruit VL53L0X**.
3. In `code/transmitter/transmitter.ino`, enter your own WiFi name and password in the `WIFI_SSID` and `WIFI_PASSWORD` lines.
4. Upload `transmitter.ino` to the transmitter ESP32 and `rover_receiver.ino` to the rover ESP32.
5. Make sure both units use the same LoRa settings, which are already set in the code.
6. Power both units, open the Serial Monitor at 115200 baud on the transmitter to find the dashboard address, and open it in a browser on the same WiFi network.

## 📁 Folder Structure

```
code/transmitter/       Transmitter sketch
code/rover_receiver/    Rover sketch
images/                 Photos and diagrams
README.md
```

## 📈 Results

Add what you tested here, for example the LoRa range, how the gas sensors responded and how well the obstacle stop worked.

## 🔮 Future Improvements

* Calibrate the gas sensors for accurate ppm readings
* Add a camera for live video from the rover
* Add speed control for smoother driving
* Replace the breadboards with a soldered board and a protective enclosure
* Log gas data to the cloud

## 👤 Author

B Shyam Sunder
Electronics and Communication Engineering, SRM Institute of Science and Technology, Ramapuram
