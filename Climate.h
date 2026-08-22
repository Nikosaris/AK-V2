#ifndef CLIMATE_H
#define CLIMATE_H

#include <Arduino.h>
#include "Globals.h"
#include "Door.h"
#include "Window.h"
#include "Heater.h"
#include "Light.h"

// ============================================================================
// CLIMATE AUTOMATION MODES
// ============================================================================

enum class ClimateMode : uint8_t {
  MANUAL = 0,
  SCHEDULE = 1,
  SMART = 2
};

// ============================================================================
// CLIMATE CONFIGURATION
// ============================================================================

struct ClimateConfig {
  ClimateMode mode = ClimateMode::MANUAL;
  uint8_t sunriseHour = 6;
  uint8_t sunriseMinute = 0;
  uint8_t sunsetHour = 20;
  uint8_t sunsetMinute = 0;
  float minTempC = 10.0f;
  float maxTempC = 28.0f;
  float overTempC = 30.0f;
  float openWindowAboveTempC = 25.0f;
  float closeWindowBelowTempC = 20.0f;
  float windowHysteresisC = 2.0f;
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

extern ClimateData climateData;

void climate_init();
void climate_update();
void climate_setMode(ClimateMode mode);
ClimateMode climate_getMode();
void climate_doorOpen();
void climate_doorClose();
void climate_windowOpen();
void climate_windowClose();
ClimateConfig* climate_getConfig();
ClimateData* climate_getData();
const char* climate_getModeName(ClimateMode mode);

#endif // CLIMATE_H
