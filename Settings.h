#ifndef SETTINGS_H
#define SETTINGS_H

#include "Globals.h"
#include "Motor.h"

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

#endif // SETTINGS_H
