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
constexpr uint8_t WINDOW_TOP_LIMIT_PIN = 36;
constexpr uint8_t WINDOW_BOTTOM_LIMIT_PIN = 39;
constexpr uint8_t LOCAL_BUTTON_PIN = 25;

// Analog Input - Current Measurement
constexpr uint8_t ACS712_PIN = 35;

// I2C Bus
constexpr uint8_t I2C_SDA_PIN = 21;
constexpr uint8_t I2C_SCL_PIN = 22;

// OneWire Bus
constexpr uint8_t ONEWIRE_PIN = 4;

// Motor H-Bridge Outputs - Door
constexpr uint8_t DOOR_IN1_PIN = 13;
constexpr uint8_t DOOR_IN2_PIN = 15;

// Motor H-Bridge Outputs - Window
constexpr uint8_t WINDOW_IN1_PIN = 32;
constexpr uint8_t WINDOW_IN2_PIN = 33;

// Relay Outputs
constexpr uint8_t CAMERA_RELAY_PIN = 26;
constexpr uint8_t HEATER_RELAY_PIN = 27;
constexpr uint8_t LIGHT_RELAY_PIN = 14;

// Ethernet W5500 SPI Bus (primary network interface)
constexpr uint8_t ETH_SCLK_PIN = 18;
constexpr uint8_t ETH_MISO_PIN = 19;
constexpr uint8_t ETH_MOSI_PIN = 23;
constexpr uint8_t ETH_CS_PIN = 5;
constexpr int8_t ETH_IRQ_PIN = -1;
constexpr int8_t ETH_RST_PIN = -1;
constexpr uint8_t ETH_PHY_ADDR = 1;

// Dual-network configuration
constexpr bool NETWORK_USE_ETHERNET_PRIMARY = true;
constexpr bool NETWORK_USE_WIFI_FALLBACK = true;

// Debug WiFi fallback (set SSID/PASSWORD for phone debugging)
constexpr const char* WIFI_DEBUG_SSID = "";
constexpr const char* WIFI_DEBUG_PASSWORD = "";
constexpr uint32_t ETHERNET_WIFI_FALLBACK_DELAY_MS = 10000;

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
