# espace-team

# ESP-NOW Multi Unit Demo

This project extends the DroneBot Workshop's ESP-NOW Multi Unit Demo to support enhanced communication and control functionalities across multiple ESP32 devices using the ESP-NOW protocol. This version introduces advanced game mechanics, including multiple levels, dynamic command generation, and command broadcasting with real-time feedback on command status.

## Features

- **Multiple Levels**: Four levels of progression, each with increasing difficulty based on tighter time constraints.
- **Dynamic Command System**: Random generation of commands, consisting of verbs and two-part nouns, which are displayed on the device and need to be executed before time runs out.
- **Real-Time Communication**: Use of ESP-NOW for efficient, real-time communication across devices without the need for Wi-Fi connectivity.
- **Progress and Timeout Management**: Real-time progress updates and timeout handling based on user interactions.
- **Visual Feedback**: Commands and progress levels are displayed using the TFT display, providing immediate visual feedback to the user.

## Hardware Requirements

- ESP32 Development Board
- ST7735 TFT Display
- General-purpose pushbuttons (2x)
- Miscellaneous: wires, breadboard, etc.

## Software Dependencies

- `TFT_eSPI` library
- `WiFi.h`
- `esp_now.h`
- `SPI.h`

## Setup

### Hardware Configuration:

1. Connect the TFT display and buttons to the ESP32 according to the pin configurations specified in the code.
2. Ensure that the display and buttons are correctly wired and functional.

### Software Installation:

1. Download and install the Arduino IDE.
2. Install the required libraries using the Library Manager in the Arduino IDE:
   - `TFT_eSPI`
   - `WiFi`
   - ESP-NOW (part of the ESP32 core for Arduino)
3. Load the script onto your ESP32 device.

### ESP-NOW Configuration:

- The ESP32 devices should be configured to operate in STA mode.
- Ensure ESP-NOW peers are properly configured to communicate with each other.

### Running the Demo:

- Power on the ESP32 devices.
- The devices should automatically connect and start the ESP-NOW communication.
- Follow the on-screen commands to progress through the levels.

## Modifications from Original Code

- Introduced multi-level gameplay with specific time limits for each level.
- Enhanced command generation logic for more varied and interesting gameplay.
- Added handling for different types of received messages (ASK, DONE, PROGRESS).
- Implemented visual feedback mechanisms for command execution and progress updates.

## Documentation

Further details and tutorials can be found on the original creator's website: [DroneBot Workshop](https://dronebotworkshop.com).
