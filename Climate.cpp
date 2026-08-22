#include "Climate.h"
#include "Sensors.h"
#include "RTC.h"

// ============================================================================
// CLIMATE DATA
// ============================================================================

ClimateData climateData;
static ClimateConfig climateConfig;

// External motor instances
extern Door doorMotor;
extern Window windowMotor;

// ============================================================================
// INITIALIZATION
// ============================================================================

void climate_init() {
  climateData.currentMode = ClimateMode::MANUAL;
  climateData.doorOpen = false;
  climateData.windowOpen = false;
  climateData.currentTempC = 20.0f;
  climateData.targetTempC = 20.0f;
  climateData.isNight = false;
  climateData.lastUpdateMs = millis();
}

// ============================================================================
// MANUAL COMMANDS
// ============================================================================

void climate_doorOpen() {
  door_open();
  climateData.doorOpen = true;
}

void climate_doorClose() {
  door_close();
  climateData.doorOpen = false;
}

void climate_windowOpen() {
  window_open();
  climateData.windowOpen = true;
}

void climate_windowClose() {
  window_close();
  climateData.windowOpen = false;
}

void climate_setMode(ClimateMode mode) {
  climateConfig.mode = mode;
  climateData.currentMode = mode;
}

// ============================================================================
// AUTOMATION LOGIC
// ============================================================================

static bool windowShouldBeOpen = false;

void climate_update() {
  if (coopEnvironment.isValid) {
    climateData.currentTempC = coopEnvironment.temperatureC;
  }

  TimeData* time = rtc_getTime();
  if (time == nullptr) {
    return;
  }

  const uint8_t currentHour = time->hour;
  const bool isDay = (currentHour >= climateConfig.sunriseHour && currentHour < climateConfig.sunsetHour);
  climateData.isNight = !isDay;

  switch (climateConfig.mode) {
    case ClimateMode::MANUAL:
      break;

    case ClimateMode::SCHEDULE:
      if (isDay && !climateData.doorOpen) {
        climate_doorOpen();
      } else if (!isDay && climateData.doorOpen) {
        climate_doorClose();
      }
      if (climateData.currentTempC > climateConfig.openWindowAboveTempC) {
        windowShouldBeOpen = true;
      } else if (climateData.currentTempC < climateConfig.closeWindowBelowTempC) {
        windowShouldBeOpen = false;
      }
      if (windowShouldBeOpen && !climateData.windowOpen) {
        climate_windowOpen();
      } else if (!windowShouldBeOpen && climateData.windowOpen) {
        climate_windowClose();
      }
      break;

    case ClimateMode::SMART:
      if (isDay && climateData.currentTempC < climateConfig.overTempC && !climateData.doorOpen) {
        climate_doorOpen();
      } else if (!isDay && climateData.doorOpen) {
        climate_doorClose();
      }
      if (climateData.currentTempC > climateConfig.openWindowAboveTempC) {
        windowShouldBeOpen = true;
      } else if (climateData.currentTempC < (climateConfig.closeWindowBelowTempC - climateConfig.windowHysteresisC)) {
        windowShouldBeOpen = false;
      }
      if (windowShouldBeOpen && !climateData.windowOpen) {
        climate_windowOpen();
      } else if (!windowShouldBeOpen && climateData.windowOpen) {
        climate_windowClose();
      }
      break;
  }
}

// ============================================================================
// PUBLIC API
// ============================================================================

ClimateMode climate_getMode() {
  return climateData.currentMode;
}

ClimateConfig* climate_getConfig() {
  return &climateConfig;
}

ClimateData* climate_getData() {
  return &climateData;
}

const char* climate_getModeName(ClimateMode mode) {
  switch (mode) {
    case ClimateMode::MANUAL: return "MANUAL";
    case ClimateMode::SCHEDULE: return "SCHEDULE";
    case ClimateMode::SMART: return "SMART";
    default: return "UNKNOWN";
  }
}
