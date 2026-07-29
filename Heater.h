#ifndef HEATER_H
#define HEATER_H

#include <Arduino.h>
#include "Globals.h"
#include "Sensors.h"

// ============================================================================
// HEATER STATE ENUM
// ============================================================================

enum class HeaterState : uint8_t {
  OFF = 0,           // Heater is off
  ON = 1,            // Heater is on
  AUTO = 2,          // Automatic mode based on dew point
  ERROR = 3          // Heater error
};

// ============================================================================
// HEATER CONFIGURATION
// ============================================================================

struct HeaterConfig {
  HeaterState mode = HeaterState::OFF;     // Current operating mode
  float dewPointThresholdC = 15.0f;        // Turn on at this dew point
  float hysteresisC = 2.0f;                // Hysteresis to prevent chatter
  uint32_t debounceTimeMs = 5000;          // Debounce time before state change
  uint8_t maxRetries = 3;                  // Max retry attempts
  uint32_t retryDelayMs = 10000;           // Delay between retries
};

// ============================================================================
// HEATER DATA - RUNTIME STATE
// ============================================================================

struct HeaterData {
  HeaterState currentState = HeaterState::OFF;
  bool isActive = false;                   // Relay is powered
  bool hasError = false;                   // Error flag
  uint8_t retryCount = 0;                  // Current retry count
  unsigned long lastStateChangeMs = 0;     // When state last changed
  unsigned long lastActivationMs = 0;      // When last turned on
  uint32_t runTimeMs = 0;                  // Total time running
};

// ============================================================================
// HEATER INSTANCE
// ============================================================================

extern HeaterData heaterData;

// ============================================================================
// HEATER CONTROL FUNCTIONS
// ============================================================================

/**
 * Initialize heater system
 */
void heater_init();

/**
 * Update heater state machine
 * Should be called in main loop
 */
void heater_update();

/**
 * Turn heater on
 */
void heater_on();

/**
 * Turn heater off
 */
void heater_off();

/**
 * Set heater mode (OFF, ON, AUTO, ERROR)
 */
void heater_setMode(HeaterState mode);

/**
 * Get heater state
 */
HeaterState heater_getState();

/**
 * Check if heater is running
 */
bool heater_isActive();

/**
 * Get heater configuration
 */
HeaterConfig* heater_getConfig();

/**
 * Get human-readable state name
 */
const char* heater_getStateName(HeaterState state);

#endif // HEATER_H
