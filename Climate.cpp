#include "Climate.h"
#include "Sensors.h"
#include "RTC.h"

// ============================================================================
// CLIMATE DATA
// ============================================================================

ClimateData climateData;
static ClimateConfig climateConfig;

// External motor instances (from Motor.cpp)
extern Motor doorMotor;
extern Motor windowMotor;

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
  motor_setCommand(&doorMotor, MotorCommand::OPEN);
  climateData.doorOpen = true;
}

void climate_doorClose() {
  motor_setCommand(&doorMotor, MotorCommand::CLOSE);
  climateData.doorOpen = false;
}

void climate_windowOpen() {
  motor_setCommand(&windowMotor, MotorCommand::OPEN);
  climateData.windowOpen = true;
}

void climate_windowClose() {
  motor_setCommand(&windowMotor, MotorCommand::CLOSE);
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

// Returns true when current time is within the day window (sunrise..sunset).
// Uses minute-precision comparison and falls back to "day" (door stays open)
// if the RTC time is not yet valid, ensuring the door is never stuck closed.
static bool climate_isDay(const TimeData* time) {
  if (time == nullptr) return true; // Safe fallback: keep door open

  uint16_t nowMins     = (uint16_t)time->hour * 60 + time->minute;
  uint16_t sunriseMins = (uint16_t)climateConfig.sunriseHour * 60 + climateConfig.sunriseMinute;
  uint16_t sunsetMins  = (uint16_t)climateConfig.sunsetHour  * 60 + climateConfig.sunsetMinute;

  // Validate sunrise < sunset (defensive guard)
  if (sunriseMins >= sunsetMins) {
    return true; // Configuration error — default to day so door stays open
  }

  return (nowMins >= sunriseMins && nowMins < sunsetMins);
}

void climate_update() {
  // Update current temperature from sensors
  if (coopEnvironment.isValid) {
    climateData.currentTempC = coopEnvironment.temperatureC;
  }

  // Get current time from RTC — automation runs regardless of rtc_isValid()
  // because DS3231 or millis() always provides a plausible time.
  TimeData* time = rtc_getTime();

  bool isDay = climate_isDay(time);
  climateData.isNight = !isDay;

  // Handle different climate modes
  switch (climateConfig.mode) {
    // ========================================================================
    case ClimateMode::MANUAL: {
      // Manual mode — no automatic control
      break;
    }

    // ========================================================================
    case ClimateMode::SCHEDULE: {
      // Scheduled mode — open/close at sunrise/sunset
      bool shouldBeOpen = isDay;

      if (shouldBeOpen && !climateData.doorOpen) {
        climate_doorOpen();
      } else if (!shouldBeOpen && climateData.doorOpen) {
        climate_doorClose();
      }

      // Window control based on temperature
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

    // ========================================================================
    case ClimateMode::SMART: {
      // Smart mode — temperature and weather aware
      bool shouldBeOpen = isDay && climateData.currentTempC < climateConfig.overTempC;

      if (shouldBeOpen && !climateData.doorOpen) {
        climate_doorOpen();
      } else if (!shouldBeOpen && climateData.doorOpen) {
        climate_doorClose();
      }

      // Window control with hysteresis
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
