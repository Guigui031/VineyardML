# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a vehicle detection system using ESP32-based Heltec LoRa boards with ultrasonic sensors and SD card logging. The system consists of three main components:

- **LoRa Transmitter**: Sensor node with ultrasonic distance sensor that detects vehicles and transmits data
- **LoRa Receiver**: Base station that receives sensor data and sends Telegram notifications
- **ParkingNotifier**: Enhanced system with multiple sensor types and modular architecture

## Hardware Architecture

The system uses Heltec ESP32 LoRa V3 boards with:
- **LoRa communication**: 866.3 MHz (Europe), 250 kHz bandwidth, spreading factor 9
- **Ultrasonic sensor**: HC-SR04 for distance measurement (trigger pin 2, echo pin 4)
- **SD card storage**: SPI interface for data logging (SCK=18, MISO=19, MOSI=23, CS=5)
- **OLED display**: 128x64 SSD1306 for status information
- **Photoresistor**: Pin 33 for light level monitoring
- **WiFi connectivity**: For Telegram bot integration

## Development Commands

### Building and Uploading
```bash
# Build transmitter (sensor node)
pio run -e transmitter

# Build receiver (base station)
pio run -e receiver

# Upload to transmitter board
pio run -e transmitter --target upload

# Upload to receiver board
pio run -e receiver --target upload

# Monitor serial output
pio device monitor -p [COM_PORT]
```

### Environment Configuration
The ParkingNotifier project uses conditional compilation with two build environments:
- **transmitter**: COM11, sensor node with ultrasonic/light sensors and SD logging
- **receiver**: COM6, base station with WiFi, Telegram bot, and LoRa reception

Build flags automatically configure the code:
- `TRANSMITTER_MODE`: Enables sensor functions, SD card logging
- `RECEIVER_MODE`: Enables WiFi, Telegram notifications, reception handling

## Code Architecture

### LoRa Communication Protocol
- **Frequency**: 866.3 MHz (configurable for region)
- **Data format**: JSON packets with ID, battery voltage, RSSI, timestamp
- **Duty cycle**: 1% compliance with automatic timing control
- **Range optimization**: SF9, 250kHz bandwidth for good range/speed balance

### ParkingNotifier Modular Architecture
The refactored system uses a clean modular design:

**Core Files:**
- `common.h`: Shared declarations, constants, and global variables
- `variables.cpp`: Global variable definitions with conditional compilation
- `transmitter.cpp`: Main transmitter application (sensor node)
- `receiver.cpp`: Main receiver application (base station)

**Module Headers & Implementation:**
- `radio.h/.cpp`: LoRa communication, packet handling, transmission/reception
- `display.h/.cpp`: OLED display management for both modes
- `sensors.h/.cpp`: Ultrasonic distance and light sensor functions (transmitter only)
- `sd_card.h/.cpp`: Data logging and file management (transmitter only)
- `notifications.h/.cpp`: WiFi and Telegram bot integration (receiver only)

**Build Configuration:**
- Conditional compilation ensures only relevant code is included in each build
- Source filtering in `platformio.ini` optimizes build size and dependencies

### Data Logging System
- **Parking events**: Logged to `/parking_data/YYYY-MM-DD.csv`
- **Light measurements**: Logged to `/light_data/YYYY-MM-DD.csv`
- **CSV format**: Includes timestamps, sensor values, system state
- **Real-time clock**: NTP synchronization for accurate timestamps

### Telegram Bot Integration
- **Commands**: `/status`, `/log`, `/data`, `/on`, `/off`, `/help`
- **Notifications**: Automatic alerts when vehicles detected
- **Remote control**: Enable/disable notifications remotely
- **Data queries**: Request current sensor readings

## Important Configuration

### Network Settings (Receiver Only)
- WiFi credentials are defined in `variables.cpp` (update before deployment)
- Telegram bot token and chat ID must be configured in `variables.cpp`
- NTP server for time synchronization: `pool.ntp.org`
- Timezone: GMT-5 with daylight savings adjustment

### Transmitter Settings
- Sensor pins: Trigger=2, Echo=4, LDR=33
- SD card SPI: SCK=18, MISO=19, MOSI=23, CS=5
- Detection threshold: < 30cm distance for vehicle presence
- Logging interval: 30 seconds for light data

### Sensor Calibration
- **Distance trigger**: < 30cm and > 0cm for valid detection
- **Detection cooldown**: 15 seconds to prevent duplicate triggers
- **Light logging**: Every 30 seconds (configurable via `logInterval`)
- **Battery monitoring**: Built-in voltage measurement and percentage calculation

### Security Considerations
- Telegram bot tokens are embedded in code (should be externalized)
- WiFi credentials are hardcoded (consider secure storage)
- SD card data is unencrypted (consider encryption for sensitive deployments)

## Testing and Deployment

### Field Testing
- Verify LoRa range and signal quality in deployment environment
- Test ultrasonic sensor accuracy at various distances and angles
- Validate SD card logging under different weather conditions
- Confirm Telegram notifications work reliably

### Power Management
- Battery voltage monitoring for low-power alerts
- Deep sleep modes available but not currently implemented
- Solar charging considerations for long-term deployment

## Troubleshooting

### Common Issues
- **LoRa communication fails**: Check frequency, spreading factor, and antenna connections
- **SD card errors**: Verify SPI wiring and card formatting (FAT32)
- **Telegram connectivity**: Confirm WiFi credentials and bot token validity
- **Sensor readings**: Check ultrasonic sensor wiring and mounting position

### Debug Commands
```bash
# Monitor with specific baud rate
pio device monitor -b 115200

# Clean build
pio run --target clean

# List available devices
pio device list
```