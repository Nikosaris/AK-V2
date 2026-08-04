#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// GPIO PIN DEFINITIONS - All hardware pins defined in one place
// ============================================================================

// Digital Inputs - Limit Switches
constexpr uint8_t DOOR_TOP_LIMIT_PIN = 16;
constexpr uint8_t DOOR_BOTTOM_LIMIT_PIN = 17;
constexpr uint8_t WINDOW_TOP_LIMIT_PIN = 19;
constexpr uint8_t WINDOW_BOTTOM_LIMIT_PIN = 23;
constexpr uint8_t LOCAL_BUTTON_PIN = 25;

// Analog Input - Current Measurement
constexpr uint8_t ACS712_PIN = 35;

// I2C Bus
constexpr uint8_t I2C_SDA_PIN = 21;
constexpr uint8_t I2C_SCL_PIN = 22;

// OneWire Bus
constexpr uint8_t ONEWIRE_PIN = 4;

// Motor H-Bridge Outputs - Door
constexpr uint8_t DOOR_IN1_PIN = 5;
constexpr uint8_t DOOR_IN2_PIN = 18;

// Motor H-Bridge Outputs - Window
constexpr uint8_t WINDOW_IN1_PIN = 32;
constexpr uint8_t WINDOW_IN2_PIN = 33;

// Relay Outputs
constexpr uint8_t CAMERA_RELAY_PIN = 26;
constexpr uint8_t HEATER_RELAY_PIN = 27;
constexpr uint8_t LIGHT_RELAY_PIN = 14;

// ============================================================================
// SYSTEM CONSTANTS
// ============================================================================

// Serial communication
constexpr uint32_t SERIAL_BAUD_RATE = 115200;

// PWM Configuration
constexpr uint8_t PWM_FREQUENCY = 1;          // 1 kHz
constexpr uint8_t PWM_RESOLUTION = 8;         // 8-bit (0-255)
constexpr uint8_t PWM_CHANNEL_DOOR_IN1 = 0;
constexpr uint8_t PWM_CHANNEL_DOOR_IN2 = 1;
constexpr uint8_t PWM_CHANNEL_WINDOW_IN1 = 2;
constexpr uint8_t PWM_CHANNEL_WINDOW_IN2 = 3;

// Timeouts and timing
constexpr uint32_t LOOP_INTERVAL_MS = 50;     // Main loop runs every 50ms (20Hz)
constexpr uint32_t SERIAL_LOG_INTERVAL_MS = 1000; // Serial logging every 1 second

// ============================================================================
// EEPROM Time Backup
// ============================================================================

constexpr uint16_t EEPROM_ADDR_TIME_BACKUP    = 200;  // 7 bytes (Time)
constexpr uint16_t EEPROM_ADDR_TIME_CHECKSUM  = 207;  // 2 bytes (CRC16)
constexpr uint16_t EEPROM_ADDR_LAST_SYNC_TIME = 209;  // 4 bytes (Unix)

// ============================================================================
// RTC DS3231
// ============================================================================

constexpr uint8_t  DS3231_I2C_ADDRESS = 0x68;
constexpr uint32_t DS3231_I2C_CLOCK   = 100000;  // 100 kHz

// ============================================================================
// Ethernet W5500
// ============================================================================

constexpr uint8_t ETHERNET_CS_PIN    = 5;
constexpr uint8_t ETHERNET_RESET_PIN = 2;
constexpr uint8_t ETHERNET_SPI_MOSI  = 23;
constexpr uint8_t ETHERNET_SPI_MISO  = 19;
constexpr uint8_t ETHERNET_SPI_CLK   = 18;

// ============================================================================
// NTP Configuration
// ============================================================================

constexpr uint32_t NTP_SYNC_INTERVAL_MS = 86400000UL;  // 24 hours
constexpr uint32_t NTP_TIMEOUT_MS       = 5000;
constexpr uint16_t NTP_PORT             = 123;
constexpr uint8_t  NTP_PACKET_SIZE      = 48;

// ============================================================================
// GLOBAL VARIABLES (defined in Globals.cpp)
// ============================================================================

extern unsigned long systemUptime;
extern unsigned long lastLoopTime;
extern unsigned long lastSerialLogTime;

// ============================================================================
// FUNCTION DECLARATIONS (defined in Globals.cpp)
// ============================================================================

void globals_init();
void globals_update();

#endif // GLOBALS_H
