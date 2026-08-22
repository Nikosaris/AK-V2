#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include "Globals.h"

// ============================================================================
// SENSOR DATA STRUCTURE
// ============================================================================

struct EnvironmentData {
  bool isValid = false;          // True if sensor has read valid data
  float temperatureC = 0.0f;    // Temperature in degrees Celsius
  float humidityPct = 0.0f;     // Relative humidity in percent
  float dewPointC = 0.0f;       // Dew point in degrees Celsius
};

// ============================================================================
// GLOBAL SENSOR INSTANCES
// ============================================================================

extern EnvironmentData coopEnvironment;     // Temperature sensor inside the coop
extern EnvironmentData cabinetEnvironment;  // Temperature/humidity sensor in the cabinet

// ============================================================================
// SENSOR FUNCTIONS
// ============================================================================

/**
 * Initialize sensor hardware
 */
void sensors_init();

/**
 * Update sensor readings
 * Should be called periodically in main loop
 */
void sensors_update();

#endif // SENSORS_H
