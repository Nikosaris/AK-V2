# AK-V2 - Professional Chicken Coop Automation Firmware

## Overview
AK-V2 is a professional-grade firmware for automated chicken coop control based on ESP32 platform.

## Features
- Stable, modular, and easily extensible
- Non-blocking operations (no delay())
- State machine-based motor control
- Professional web interface
- Complete climate control automation

## Development Environment
- **Platform:** ESP32-WROOM-32
- **IDE:** Arduino IDE
- **Language:** Arduino C++

## Project Structure
```
AK-V2/
├── AK_V2.ino          # Main sketch
├── Globals.h/cpp      # Global variables, pin mapping and network constants
├── Hardware.h/cpp     # Hardware initialization and pin definitions
├── Motor.h/cpp        # Motor state machine and control
├── Sensors.h/cpp      # Sensor and electrical telemetry layer
├── Settings.h/cpp     # Configuration management
├── Climate.h/cpp      # Automation logic (sunrise, sunset, temperature)
├── Logger.h/cpp       # Event logging
├── WebServer.h/cpp    # Web interface and REST API
├── OTA.h/cpp          # Over-the-air updates
├── RTC.h/cpp          # Real-time clock management
└── WiFi.h/cpp         # WiFi connectivity
```

## GPIO Map
### Digital Inputs
- GPIO16: DOOR_TOP_LIMIT
- GPIO17: DOOR_BOTTOM_LIMIT
- GPIO36: WINDOW_TOP_LIMIT
- GPIO39: WINDOW_BOTTOM_LIMIT
- GPIO25: LOCAL_BUTTON

### Analog Inputs
- GPIO35: ACS712 (Current Measurement)

### I²C Bus
- GPIO21: SDA
- GPIO22: SCL

### OneWire Bus
- GPIO4: DS18B20 (Temperature Sensors)

### H-Bridge Outputs (Motors)
- GPIO13, GPIO15: Door (IN1, IN2)
- GPIO32, GPIO33: Window (IN1, IN2)

### Ethernet W5500 (SPI, primary network)
- GPIO18: SCLK
- GPIO19: MISO
- GPIO23: MOSI
- GPIO5: CS
- Ethernet is primary link, WiFi is fallback/debug

### Relay Outputs
- GPIO26: Camera Relay
- GPIO27: Heater Relay
- GPIO14: Light Relay

## Coding Guidelines
✅ Use: struct, enum class, constexpr, modular files
❌ Avoid: class, OOP, dynamic memory, String

## State Machine
```
STOPPED → OPENING/CLOSING → (OBSTACLE/TIMEOUT) → ERROR/RETRY
```

## Version
v0.2.0 - W5500 Ethernet primary, WiFi fallback and updated pin map

## Standalone Module Test Sketches

For module-by-module debugging through web buttons, use these standalone sketches:

- `module_tests/AKV2_Test_Sensors/AKV2_Test_Sensors.ino`
  - Coop temperature + cabinet temperature/humidity display
  - Web calibration offsets for each measured value
- `module_tests/AKV2_Test_Relays/AKV2_Test_Relays.ino`
  - Manual ON/OFF relay testing only (camera/heater/light)
- `module_tests/AKV2_Test_Limits/AKV2_Test_Limits.ino`
  - Door/window limit sensor state evaluation
- `module_tests/AKV2_Test_DoorManual/AKV2_Test_DoorManual.ino`
  - Manual door drive OPEN/CLOSE/STOP over web buttons
- `module_tests/AKV2_Test_VentManual/AKV2_Test_VentManual.ino`
  - Manual ventilation drive OPEN/CLOSE/STOP over web buttons

Each test sketch starts its own AP and serves a minimal local web UI.
