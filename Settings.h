#ifndef SETTINGS_H
#define SETTINGS_H

#include "Globals.h"
#include "Motor.h"
#include "Climate.h"

// ============================================================================
// SETTINGS MANAGEMENT
// ============================================================================

/**
 * Initialize settings system
 * Load configuration from EEPROM or use defaults
 */
void settings_init();

/**
 * Load all settings from EEPROM
 */
void settings_load();

/**
 * Save all settings to EEPROM
 */
void settings_save();

/**
 * Reset to factory defaults
 */
void settings_reset();

/**
 * Get door motor configuration
 */
MotorConfig* settings_getDoorConfig();

/**
 * Get window motor configuration
 */
MotorConfig* settings_getWindowConfig();

/**
 * Apply and persist door motor configuration
 */
void settings_applyDoorConfig(const MotorConfig& cfg);

/**
 * Apply and persist window motor configuration
 */
void settings_applyWindowConfig(const MotorConfig& cfg);

/**
 * Get GPS/climate configuration
 */
ClimateConfig* settings_getClimateConfig();

/**
 * Apply and persist GPS coordinates and timezone
 */
void settings_applyClimateGPS(float latitude, float longitude, int8_t timezoneOffsetH);

#endif // SETTINGS_H
