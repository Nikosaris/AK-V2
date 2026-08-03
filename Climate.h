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
  
  // Sunrise/Sunset times (for SCHEDULE mode) - auto-computed from GPS
  uint8_t sunriseHour = 6;
  uint8_t sunriseMinute = 0;
  uint8_t sunsetHour = 20;
  uint8_t sunsetMinute = 0;
  
  // GPS coordinates for automatic sunrise/sunset calculation
  float latitude = 50.0f;             // Degrees North (e.g. 50.08 for Prague)
  float longitude = 14.42f;           // Degrees East  (e.g. 14.42 for Prague)
  int8_t timezoneOffsetH = 1;         // Base UTC offset WITHOUT DST (1 = CET). DST (+1 h) is added automatically.

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

/**
 * Recalculate sunrise and sunset times from GPS coordinates and current date.
 * Updates climateConfig.sunriseHour/Minute and sunsetHour/Minute.
 * Uses a simplified NOAA solar algorithm accurate to ±1 minute.
 * @param year  - full year (e.g. 2025)
 * @param month - month 1-12
 * @param day   - day 1-31
 */
void climate_recalculateSunTimes(uint16_t year, uint8_t month, uint8_t day);

#endif // CLIMATE_H
