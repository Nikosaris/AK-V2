#include "Settings.h"
#include "Climate.h"
#include <EEPROM.h>

// ============================================================================
// EEPROM CONFIGURATION
// ============================================================================

const uint16_t EEPROM_SIZE = 4096;      // Total EEPROM size
const uint16_t EEPROM_MAGIC = 0xAA55;   // Magic number to identify valid data
const uint16_t EEPROM_VERSION = 2;      // Settings version (bumped for GPS addition)

const uint16_t EEPROM_ADDR_MAGIC = 0;   // Magic number
const uint16_t EEPROM_ADDR_VERSION = 2; // Version number
const uint16_t EEPROM_ADDR_DOOR_CONFIG = 10;    // Door configuration
const uint16_t EEPROM_ADDR_WINDOW_CONFIG = 100; // Window configuration
const uint16_t EEPROM_ADDR_GPS = 200;           // GPS: latitude(float) + longitude(float) + timezone(int8_t)

// ============================================================================
// GLOBAL SETTINGS
// ============================================================================

static MotorConfig doorConfig;
static MotorConfig windowConfig;

// GPS config is stored in/retrieved from climateConfig directly via Climate.h

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
    EEPROM.get(EEPROM_ADDR_DOOR_CONFIG, doorConfig);
    EEPROM.get(EEPROM_ADDR_WINDOW_CONFIG, windowConfig);

    // Load GPS settings into climateConfig
    ClimateConfig* cc = climate_getConfig();
    EEPROM.get(EEPROM_ADDR_GPS, cc->latitude);
    EEPROM.get(EEPROM_ADDR_GPS + sizeof(float), cc->longitude);
    EEPROM.get(EEPROM_ADDR_GPS + 2 * sizeof(float), cc->timezoneOffsetH);
  } else {
    // Use default values and persist them
    settings_reset();
  }
}

void settings_save() {
  // Write magic number and version
  EEPROM.write(EEPROM_ADDR_MAGIC, EEPROM_MAGIC & 0xFF);
  EEPROM.write(EEPROM_ADDR_MAGIC + 1, (EEPROM_MAGIC >> 8) & 0xFF);
  EEPROM.write(EEPROM_ADDR_VERSION, EEPROM_VERSION & 0xFF);
  EEPROM.write(EEPROM_ADDR_VERSION + 1, (EEPROM_VERSION >> 8) & 0xFF);

  EEPROM.put(EEPROM_ADDR_DOOR_CONFIG, doorConfig);
  EEPROM.put(EEPROM_ADDR_WINDOW_CONFIG, windowConfig);

  // Save GPS settings from climateConfig
  ClimateConfig* cc = climate_getConfig();
  EEPROM.put(EEPROM_ADDR_GPS, cc->latitude);
  EEPROM.put(EEPROM_ADDR_GPS + sizeof(float), cc->longitude);
  EEPROM.put(EEPROM_ADDR_GPS + 2 * sizeof(float), cc->timezoneOffsetH);

  EEPROM.commit();
}

void settings_reset() {
  // Door configuration defaults (heavier motor, higher current, longer timeout)
  doorConfig.timeoutMs = 30000;
  doorConfig.maxCurrent = 2000;
  doorConfig.currentIgnoreTimeMs = 500;
  doorConfig.overCurrentTimeMs = 1000;
  doorConfig.maxRetries = 3;
  doorConfig.pwmOpen = 200;
  doorConfig.pwmClose = 200;
  doorConfig.pwmSlow = 100;
  doorConfig.slowApproachDistanceMs = 2000;

  // Window (ventilation) configuration defaults (lighter motor, lower current, shorter timeout)
  windowConfig.timeoutMs = 20000;
  windowConfig.maxCurrent = 1500;
  windowConfig.currentIgnoreTimeMs = 500;
  windowConfig.overCurrentTimeMs = 800;
  windowConfig.maxRetries = 3;
  windowConfig.pwmOpen = 180;
  windowConfig.pwmClose = 180;
  windowConfig.pwmSlow = 90;
  windowConfig.slowApproachDistanceMs = 1500;

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

// ============================================================================
// APPLY HELPERS - update config and persist immediately
// ============================================================================

void settings_applyDoorConfig(const MotorConfig& cfg) {
  doorConfig = cfg;
  settings_save();
}

void settings_applyWindowConfig(const MotorConfig& cfg) {
  windowConfig = cfg;
  settings_save();
}

// ============================================================================
// GPS HELPERS
// ============================================================================

ClimateConfig* settings_getClimateConfig() {
  return climate_getConfig();
}

void settings_applyClimateGPS(float latitude, float longitude, int8_t timezoneOffsetH) {
  ClimateConfig* cc = climate_getConfig();
  cc->latitude         = latitude;
  cc->longitude        = longitude;
  cc->timezoneOffsetH  = timezoneOffsetH;
  settings_save();
}
