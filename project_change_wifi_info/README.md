| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- | -------- | -------- |

# PROJECT_CHANGE_WIFI_INFO

This project implements a web control panel to manage different functionalities of the ESP32, such as WiFi connection, firmware update, UART control and system temperature and time monitoring.

### Features
- WiFi connection: Allows connecting and disconnecting the ESP32 from wireless networks.

- Firmware Upgrade: Upload .bin files to upgrade the ESP32 firmware.

- Temperature Monitoring: Displays real-time temperature measured by an NTC sensor.

- Time Display: Synchronisation and display of the system time.

- UART Control: Enables and disables sending data over UART.

- Register: Save, delete, and display registers on the web, with an update that allows the first five registers to be    fixed in the open position and the last five in the closed position.

- Servo Control : Library update enabling servo control.


## How to use example
We encourage the users to use the example as a template for the new projects.
A recommended way is to follow the instructions on a [docs page](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html#start-a-new-project).

- Connect to the ESP32's WiFi network or make sure it is on the same local network.

- Open a browser and access the ESP32's IP address.

Use the interface to:

- Configure WiFi settings.

- Monitor temperature and time.

- Update firmware.

- Control UART.

- Control the window manually.

- Control the LED.

- Control the window with registers.

## Example folder contents

The project **PROJECT_CHANGE_WIFI_INFO** contains one source file in C language [main.c](main/main.c). The file is located in folder [main](main).

ESP-IDF projects are built using CMake. The project build configuration is contained in `CMakeLists.txt`
files that provide set of directives and instructions describing the project's source files and targets
(executable, library, or both). 

Below is short explanation of remaining files in the project folder.

```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   └── main.c
├── Librerias
│   ├── Adc_lib
│   │   ├── ADC_NTC_POT.c
│   │   └── ADC_NTC_POT.h
│   ├── Http_lib
│   │   ├── http_server.c
│   │   └── http_server.h
│   ├── RGB_lib
│   │   ├── LED_RGB.c
│   │   └── LED_RGB.h
│   ├── Uart_lib
│   │   ├── COMANDS_UART.c
│   │   └── COMANDS_UART.h
│   ├── Wifi_lib
│   │   ├── wifi_app.c
│   │   └── wifi_app.h
│   ├── Servo_lib
│   │   ├── servo_lib.c
│   │   └── servo_lib.h
├── webpage
│   ├── app.css
│   ├── app.js
│   ├── favicon.ico
│   ├── index.html
│   ├── jquery-3.3.1.min.js
└── Readme.md                       This file reading.
```
Additionally, the sample project contains Makefile and component.mk files, used for the legacy Make based build system. 
They are not used or needed when building with CMake and idf.py.
