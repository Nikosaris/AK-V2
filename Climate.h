#ifndef CLIMATE_H
#define CLIMATE_H

#include <Arduino.h>
#include "Globals.h"
#include "Motor.h"
#include "Heater.h"
#include "Light.h"

// ============================================================================
// CLIMATE AUTOMATION MODES
// ============================================================================

enum class ClimateMode : uint8_t {
  MANUAL = 0,        // Manual control - user commands only
  SCHEDULE = 1,      // Scheduled automation (sunrise/sunset)
  SMART = 2          // Smart mode (weather-dependent)
};

// ============================================================================
// CLIMATE CONFIGURATION
// ============================================================================

struct ClimateConfig {
  ClimateMode mode = ClimateMode::MANUAL;
  
  // Sunrise/Sunset times (for SCHEDULE mode)
  uint8_t sunriseHour = 6;
  uint8_t sunriseMinute = 0;
  uint8_t sunsetHour = 20;
  uint8_t sunsetMinute = 0;
  
  // Temperature thresholds
  float minTempC = 10.0f;             // Minimum comfortable temperature
  float maxTempC = 28.0f;             // Maximum comfortable temperature
  float overTempC = 30.0f;            // Emergency over-temperature
  
  // Window opening thresholds
  float openWindowAboveTempC = 25.0f; // Open window if temp above this
  float closeWindowBelowTempC = 20.0f; // Close window if temp below this
  
  // Hysteresis for window control
  float windowHysteresisC = 2.0f;     // Prevent chattering
};

// ============================================================================
// CLIMATE DATA - RUNTIME STATE
// ============================================================================

struct ClimateData {
  ClimateMode currentMode = ClimateMode::MANUAL;
  bool doorOpen = false;
  bool windowOpen = false;
  float currentTempC = 0.0f;
  float targetTempC = 20.0f;
  bool isNight = false;
  bool isSunrise = false;
  unsigned long lastUpdateMs = 0;
};

// ============================================================================
// CLIMATE INSTANCE
// ============================================================================

extern ClimateData climateData;

// ============================================================================
// CLIMATE CONTROL FUNCTIONS
// ============================================================================

/**
 * Initialize climate automation system
 */
void climate_init();

/**
 * Update climate automation
 * Should be called in main loop
 */
void climate_update();

/**
 * Set climate mode
 */
void climate_setMode(ClimateMode mode);

/**
 * Get current climate mode
 */
ClimateMode climate_getMode();

/**
 * Manual door open command
 */
void climate_doorOpen();

/**
 * Manual door close command
 */
void climate_doorClose();

/**
 * Manual window open command
 */
void climate_windowOpen();

/**
 * Manual window close command
 */
void climate_windowClose();

/**
 * Get climate configuration
 */
ClimateConfig* climate_getConfig();

/**
 * Get climate data
 */
ClimateData* climate_getData();

/**
 * Get mode name
 */
const char* climate_getModeName(ClimateMode mode);

#endif // CLIMATE_H
