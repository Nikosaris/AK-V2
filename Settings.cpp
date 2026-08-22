#include "Settings.h"
#include <EEPROM.h>

// ============================================================================
// EEPROM CONFIGURATION
// ============================================================================

const uint16_t EEPROM_SIZE = 4096;
const uint16_t EEPROM_MAGIC = 0xAA55;
const uint16_t EEPROM_VERSION = 1;

const uint16_t EEPROM_ADDR_MAGIC = 0;
const uint16_t EEPROM_ADDR_VERSION = 2;
const uint16_t EEPROM_ADDR_DOOR_CONFIG = 10;
const uint16_t EEPROM_ADDR_WINDOW_CONFIG = 100;

// ============================================================================
// GLOBAL SETTINGS
// ============================================================================

static DoorConfig doorConfig;
static WindowConfig windowConfig;

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
  uint16_t magic = EEPROM.read(EEPROM_ADDR_MAGIC) | (EEPROM.read(EEPROM_ADDR_MAGIC + 1) << 8);
  uint16_t version = EEPROM.read(EEPROM_ADDR_VERSION) | (EEPROM.read(EEPROM_ADDR_VERSION + 1) << 8);

  if (magic == EEPROM_MAGIC && version == EEPROM_VERSION) {
    // Keep defaults for now; migrate persistence later.
  } else {
    settings_reset();
  }
}

void settings_save() {
  EEPROM.write(EEPROM_ADDR_MAGIC, EEPROM_MAGIC & 0xFF);
  EEPROM.write(EEPROM_ADDR_MAGIC + 1, (EEPROM_MAGIC >> 8) & 0xFF);
  EEPROM.write(EEPROM_ADDR_VERSION, EEPROM_VERSION & 0xFF);
  EEPROM.write(EEPROM_ADDR_VERSION + 1, (EEPROM_VERSION >> 8) & 0xFF);
  EEPROM.commit();
}

void settings_reset() {
  doorConfig = DoorConfig();
  windowConfig = WindowConfig();
  settings_save();
}

// ============================================================================
// GETTERS
// ============================================================================

DoorConfig* settings_getDoorConfig() {
  return &doorConfig;
}

WindowConfig* settings_getWindowConfig() {
  return &windowConfig;
}
