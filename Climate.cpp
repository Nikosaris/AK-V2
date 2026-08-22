#include "Climate.h"
#include "Sensors.h"
#include "RTC.h"
#include "Door.h"
#include "Window.h"

// ============================================================================
// CLIMATE DATA
// ============================================================================

ClimateData climateData;
static ClimateConfig climateConfig;

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

  // Set default configuration
  climateConfig.mode = ClimateMode::MANUAL;
  climateConfig.sunriseHour = 6;
  climateConfig.sunriseMinute = 0;
  climateConfig.sunsetHour = 20;
  climateConfig.sunsetMinute = 0;
  climateConfig.minTempC = 10.0f;
  climateConfig.maxTempC = 28.0f;
  climateConfig.overTempC = 30.0f;
  climateConfig.openWindowAboveTempC = 25.0f;
  climateConfig.closeWindowBelowTempC = 20.0f;
  climateConfig.windowHysteresisC = 2.0f;
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

static bool windowShouldBeOpen = false;  // Hysteresis tracking

void climate_update() {
  unsigned long currentTime = millis();

  // Update current temperature from sensors
  if (coopEnvironment.isValid) {
    climateData.currentTempC = coopEnvironment.temperatureC;
  }

  // Get current time from RTC
  TimeData* time = rtc_getTime();

  // Determine if it's day or night
  uint8_t currentHour = time->hour;
  uint8_t currentMinute = time->minute;

  bool isDay = (currentHour >= climateConfig.sunriseHour && 
                currentHour < climateConfig.sunsetHour);
  climateData.isNight = !isDay;

  // Handle different climate modes
  switch (climateConfig.mode) {
    case ClimateMode::MANUAL: {
      break;
    }

    case ClimateMode::SCHEDULE: {
      bool shouldBeOpen = isDay;

      if (shouldBeOpen && !climateData.doorOpen) {
        climate_doorOpen();
      } else if (!shouldBeOpen && climateData.doorOpen) {
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
    }

    case ClimateMode::SMART: {
      bool shouldBeOpen = isDay && climateData.currentTempC < climateConfig.overTempC;

      if (shouldBeOpen && !climateData.doorOpen) {
        climate_doorOpen();
      } else if (!shouldBeOpen && climateData.doorOpen) {
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
    case ClimateMode::MANUAL:   return "MANUAL";
    case ClimateMode::SCHEDULE: return "SCHEDULE";
    case ClimateMode::SMART:    return "SMART";
    default:                    return "UNKNOWN";
  }
}
