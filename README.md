# ESP32 Wi-Fi Control and Monitoring System

A real-time embedded control project built for the ESP32 using the ESP-IDF framework. The system exposes a web dashboard for wireless network configuration, sensor monitoring, firmware updates, UART control, and actuator management, including servo-based window control and RGB status signaling.

This project was designed as a professional control and monitoring platform for embedded systems, with modular drivers and an intuitive browser-based interface.

## Overview

The firmware integrates multiple peripherals and services into a single application:

- Wi-Fi station and access point configuration
- Embedded HTTP server with a web front-end
- OTA firmware update support
- Temperature reading from an NTC sensor
- Time synchronization via SNTP
- UART communication control
- Servo-controlled window automation
- RGB LED signaling and status feedback
- Persistence of Wi-Fi credentials and scheduled commands

## Key Features

- Wi-Fi management
  - Connect/disconnect from a local wireless network
  - Save and restore credentials in NVS
  - Start an ESP32 access point for local configuration

- Embedded web interface
  - Real-time temperature monitoring
  - Time and date display
  - Manual open/close actions for a window mechanism
  - Firmware update through a browser upload
  - UART enable/disable controls

- Sensor acquisition
  - NTC analog temperature reading
  - Potentiometer signal acquisition

- Automation and control
  - Servo-based actuation for window movement
  - RGB LED control using configurable thresholds
  - Time-based register comparison and scheduled actions

- OTA support
  - Dual-partition layout for firmware upgrade
  - Web-based upload flow with status reporting

## System Architecture

The firmware is organized into a few logical modules:

- `main.c`: application entry point and task orchestration
- `Wifi_lib`: Wi-Fi initialization, credentials handling, and connection logic
- `Http_lib`: embedded HTTP server and OTA logic
- `Adc_lib`: NTC and potentiometer measurement drivers
- `Uart_lib`: UART interface and command handling
- `RGB_lib`: RGB LED control and thresholds
- `Servo_lib`: servo motor control for the window mechanism

## Project Structure

```text
Project_RTS_2024-2/
├── README.md
├── project_change_wifi_info/
│   ├── CMakeLists.txt
│   ├── sdkconfig
│   ├── partitions_two_ota.csv
│   ├── main/
│   │   ├── CMakeLists.txt
│   │   ├── main.c
│   │   ├── tasks_common.h
│   │   ├── HTTP_Server.pdf
│   │   ├── webpage/
│   │   │   ├── index.html
│   │   │   ├── app.css
│   │   │   ├── app.js
│   │   │   ├── favicon.ico
│   │   │   └── jquery-3.3.1.min.js
│   │   └── Librerias/
│   │       ├── Adc_lib/
│   │       ├── Http_lib/
│   │       ├── RGB_lib/
│   │       ├── Servo_lib/
│   │       ├── Uart_lib/
│   │       └── Wifi_lib/
│   └── build/
└── .git/
```

## Hardware Requirements

This project is intended for an ESP32 development board with the following peripherals connected:

- NTC temperature sensor
- Potentiometer input
- RGB LED
- Servo motor or actuator for the window mechanism
- UART interface for communication or debugging
- Power supply and stable ground reference

## Software Requirements

- ESP-IDF v5.x
- CMake
- Python 3
- USB-to-UART driver for flashing the board
- A serial terminal such as idf.py monitor or any compatible serial monitor

## Getting Started

1. Install ESP-IDF and configure the environment variables.
2. Open a terminal in the project folder:

```bash
cd project_change_wifi_info
```

3. Set the target device:

```bash
idf.py set-target esp32
```

4. Build the firmware:

```bash
idf.py build
```

5. Flash the firmware to the ESP32:

```bash
idf.py -p <PORT> flash
```

6. Open the serial monitor:

```bash
idf.py monitor
```

## Default Network Configuration

The application is configured with a default access point profile in the source code:

- SSID: `ESP32_JAVIER`
- Password: `12345678`
- IP: `192.168.0.1`

After startup, connect to this network and access the embedded web page through the device IP or the configured local network route.

## Usage

After the device starts:

1. Connect to the ESP32 access point or ensure the board is on the same local network.
2. Open a browser and go to the ESP32 web interface.
3. Configure Wi-Fi settings if needed.
4. Monitor real-time temperature and time information.
5. Upload new firmware using the OTA interface.
6. Control the UART, RGB LED, or window mechanism from the dashboard.

## Web Application Features

The frontend included in the project enables users to:

- View the currently saved Wi-Fi network
- Establish a Wi-Fi connection
- Remove saved Wi-Fi credentials
- Read current temperature values from the NTC sensor
- Trigger manual window open/close actions
- Toggle UART communication
- Update firmware from a `.bin` file
- Check OTA status and reboot workflow

## Notes

- The project uses a custom partition table with OTA support, which enables safe firmware upgrades.
- Configuration values such as SSID, password, and GPIO mappings can be adjusted in the library headers and source files.
- For a production deployment, it is recommended to secure the access point credentials, validate input data, and adapt the actuator and sensor logic to the target hardware.

## License

This project is intended for academic and embedded systems development use. Please review the repository and source files for any additional constraints before commercial deployment.

## Contributing

Contributions are welcome. If you want to extend the project with new sensors, protocols, or dashboard features, consider maintaining the same modular structure and documenting the operational changes clearly.
