#ifndef LIGHT_H
#define LIGHT_H

#include <Arduino.h>
#include "Globals.h"

// ============================================================================
// LIGHT STATE ENUM
// ============================================================================

enum class LightState : uint8_t {
  OFF = 0,           // Light is off
  ON = 1,            // Light is on
  AUTO = 2,          // Automatic mode (time-based)
  ERROR = 3          // Light error
};

// ============================================================================
// LIGHT CONFIGURATION
// ============================================================================

struct LightConfig {
  LightState mode = LightState::OFF;       // Current operating mode
  uint8_t autoOnHour = 6;                  // Auto turn on at this hour
  uint8_t autoOnMinute = 0;                // Auto turn on at this minute
  uint8_t autoOffHour = 20;                // Auto turn off at this hour
  uint8_t autoOffMinute = 0;               // Auto turn off at this minute
  uint8_t minOnTimeMs = 100;               // Minimum time light must be on
};

// ============================================================================
// LIGHT DATA - RUNTIME STATE
// ============================================================================

struct LightData {
  LightState currentState = LightState::OFF;
  bool isActive = false;                   // Relay is powered
  bool hasError = false;                   // Error flag
  unsigned long lastStateChangeMs = 0;     // When state last changed
  unsigned long lastActivationMs = 0;      // When last turned on
  uint32_t runTimeMs = 0;                  // Total time light has been on
};

// ============================================================================
// LIGHT INSTANCE
// ============================================================================

extern LightData lightData;

// ============================================================================
// LIGHT CONTROL FUNCTIONS
// ============================================================================

/**
 * Initialize light system
 */
void light_init();

/**
 * Update light state machine
 * Should be called in main loop
 */
void light_update();

/**
 * Turn light on
 */
void light_on();

/**
 * Turn light off
 */
void light_off();

/**
 * Set light mode (OFF, ON, AUTO)
 */
void light_setMode(LightState mode);

/**
 * Get light state
 */
LightState light_getState();

/**
 * Check if light is on
 */
bool light_isActive();

/**
 * Get light configuration
 */
LightConfig* light_getConfig();

/**
 * Get human-readable state name
 */
const char* light_getStateName(LightState state);

#endif // LIGHT_H
