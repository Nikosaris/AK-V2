#include "Settings.h"
#include <EEPROM.h>

const uint16_t EEPROM_SIZE = 4096;
const uint16_t EEPROM_MAGIC = 0xAA55;
const uint16_t EEPROM_VERSION = 1;

const uint16_t EEPROM_ADDR_MAGIC = 0;
const uint16_t EEPROM_ADDR_VERSION = 2;
const uint16_t EEPROM_ADDR_DOOR_CONFIG = 10;
const uint16_t EEPROM_ADDR_WINDOW_CONFIG = 100;

static DoorConfig doorConfig;
static WindowConfig windowConfig;

void settings_init() {
  EEPROM.begin(EEPROM_SIZE);
  settings_load();
}

void settings_load() {
  uint16_t magic = EEPROM.read(EEPROM_ADDR_MAGIC) | (EEPROM.read(EEPROM_ADDR_MAGIC + 1) << 8);
  uint16_t version = EEPROM.read(EEPROM_ADDR_VERSION) | (EEPROM.read(EEPROM_ADDR_VERSION + 1) << 8);

  if (magic == EEPROM_MAGIC && version == EEPROM_VERSION) {
    EEPROM.get(EEPROM_ADDR_DOOR_CONFIG, doorConfig);
    EEPROM.get(EEPROM_ADDR_WINDOW_CONFIG, windowConfig);
  } else {
    settings_reset();
  }
}

void settings_save() {
  EEPROM.write(EEPROM_ADDR_MAGIC, EEPROM_MAGIC & 0xFF);
  EEPROM.write(EEPROM_ADDR_MAGIC + 1, (EEPROM_MAGIC >> 8) & 0xFF);
  EEPROM.write(EEPROM_ADDR_VERSION, EEPROM_VERSION & 0xFF);
  EEPROM.write(EEPROM_ADDR_VERSION + 1, (EEPROM_VERSION >> 8) & 0xFF);

  EEPROM.put(EEPROM_ADDR_DOOR_CONFIG, doorConfig);
  EEPROM.put(EEPROM_ADDR_WINDOW_CONFIG, windowConfig);
  EEPROM.commit();
}

void settings_reset() {
  doorConfig = DoorConfig();
  windowConfig = WindowConfig();
  settings_save();
}

DoorConfig* settings_getDoorConfig() {
  return &doorConfig;
}

WindowConfig* settings_getWindowConfig() {
  return &windowConfig;
}

void settings_applyDoorConfig(const DoorConfig& cfg) {
  doorConfig = cfg;
  settings_save();
}

void settings_applyWindowConfig(const WindowConfig& cfg) {
  windowConfig = cfg;
  settings_save();
}
