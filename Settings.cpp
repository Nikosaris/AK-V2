#include "Settings.h"
#include <EEPROM.h>

// ============================================================================
// EEPROM CONFIGURATION
// ============================================================================

const uint16_t EEPROM_SIZE = 4096;      // Total EEPROM size
const uint16_t EEPROM_MAGIC = 0xAA55;   // Magic number to identify valid data
const uint16_t EEPROM_VERSION = 1;      // Settings version

const uint16_t EEPROM_ADDR_MAGIC = 0;   // Magic number
const uint16_t EEPROM_ADDR_VERSION = 2; // Version number
const uint16_t EEPROM_ADDR_DOOR_CONFIG = 10;   // Door configuration
const uint16_t EEPROM_ADDR_WINDOW_CONFIG = 100; // Window configuration

// ============================================================================
// GLOBAL SETTINGS
// ============================================================================

static MotorConfig doorConfig;
static MotorConfig windowConfig;

// ============================================================================
// INITIALIZATION
// ============================================================================

void settings_init() {
  EEPROM.begin(EEPROM_SIZE);
  settings_load();
}

// ============================================================================
// LOAD / SAVE / RESET
// ============================================================================

void settings_load() {
  // Check if EEPROM contains valid data
  uint16_t magic = EEPROM.read(EEPROM_ADDR_MAGIC) | (EEPROM.read(EEPROM_ADDR_MAGIC + 1) << 8);
  uint16_t version = EEPROM.read(EEPROM_ADDR_VERSION) | (EEPROM.read(EEPROM_ADDR_VERSION + 1) << 8);

  if (magic == EEPROM_MAGIC && version == EEPROM_VERSION) {
    // Load from EEPROM
    // For now, using default values
    // TODO: Implement EEPROM reading
  } else {
    // Use default values
    settings_reset();
  }
}

void settings_save() {
  // Write magic number and version
  EEPROM.write(EEPROM_ADDR_MAGIC, EEPROM_MAGIC & 0xFF);
  EEPROM.write(EEPROM_ADDR_MAGIC + 1, (EEPROM_MAGIC >> 8) & 0xFF);
  EEPROM.write(EEPROM_ADDR_VERSION, EEPROM_VERSION & 0xFF);
  EEPROM.write(EEPROM_ADDR_VERSION + 1, (EEPROM_VERSION >> 8) & 0xFF);

  // TODO: Save motor configurations

  EEPROM.commit();
}

void settings_reset() {
  // Door configuration defaults
  doorConfig.timeoutMs = 30000;
  doorConfig.maxCurrent = 1500;
  doorConfig.currentIgnoreTimeMs = 500;
  doorConfig.overCurrentTimeMs = 1000;
  doorConfig.maxRetries = 3;
  doorConfig.pwmOpen = 200;
  doorConfig.pwmClose = 200;
  doorConfig.pwmSlow = 100;
  doorConfig.slowApproachDistanceMs = 2000;

  // Window configuration defaults (same as door for now)
  windowConfig = doorConfig;

  settings_save();
}

// ============================================================================
// GETTERS
// ============================================================================

MotorConfig* settings_getDoorConfig() {
  return &doorConfig;
}

MotorConfig* settings_getWindowConfig() {
  return &windowConfig;
}
