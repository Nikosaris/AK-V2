#include "Climate.h"
#include "Sensors.h"
#include "RTC.h"
#include <math.h>

// ============================================================================
// CLIMATE DATA
// ============================================================================

ClimateData climateData;
static ClimateConfig climateConfig;

// External motor instances (from Motor.cpp)
extern Motor doorMotor;
extern Motor windowMotor;

// ============================================================================
// SUN TIME CALCULATION (Simplified NOAA algorithm)
// Accurate to ±1-2 minutes for mid-latitudes.
// Returns time in minutes from midnight (local time).
// Returns -1 if sun never rises/sets (polar day/night).
// ============================================================================

static const float DEG2RAD = M_PI / 180.0f;
static const float RAD2DEG = 180.0f / M_PI;

// Compute Julian Day Number from calendar date (Gregorian calendar)
static float climate_julianDay(uint16_t year, uint8_t month, uint8_t day) {
  if (month <= 2) {
    year -= 1;
    month += 12;
  }
  int A = (int)(year / 100);
  int B = 2 - A + (int)(A / 4);
  return (int)(365.25f * (year + 4716)) + (int)(30.6001f * (month + 1)) + day + B - 1524.5f;
}

// Calculate sunrise or sunset in minutes from midnight (local time).
// isSunrise = true → sunrise, false → sunset.
// Returns -1 on failure (polar).
static int climate_sunEvent(uint16_t year, uint8_t month, uint8_t day, bool isSunrise) {
  float lat = climateConfig.latitude;
  float lon = climateConfig.longitude;
  int8_t tz  = climateConfig.timezoneOffsetH;

  float jd = climate_julianDay(year, month, day);
  float n  = jd - 2451545.0f + 0.0008f;

  float Js = n - lon / 360.0f;
  float M  = fmodf(357.5291f + 0.98560028f * Js, 360.0f);
  float C  = 1.9148f * sinf(M * DEG2RAD)
           + 0.0200f * sinf(2.0f * M * DEG2RAD)
           + 0.0003f * sinf(3.0f * M * DEG2RAD);
  float lambda = fmodf(M + C + 180.0f + 102.9372f, 360.0f);

  float Jt   = 2451545.0f + Js + 0.0053f * sinf(M * DEG2RAD) - 0.0069f * sinf(2.0f * lambda * DEG2RAD);
  float sinD = sinf(lambda * DEG2RAD) * sinf(23.4397f * DEG2RAD);
  float cosD = cosf(asinf(sinD));

  float cosH = (sinf(-0.8333f * DEG2RAD) - sinf(lat * DEG2RAD) * sinD)
               / (cosf(lat * DEG2RAD) * cosD);

  if (cosH < -1.0f || cosH > 1.0f) {
    return -1; // polar day or night
  }

  float H = acosf(cosH) * RAD2DEG;
  float Jrise = Jt - H / 360.0f;
  float Jset  = Jt + H / 360.0f;

  float Jevent = isSunrise ? Jrise : Jset;
  // Convert Julian day fraction to minutes from midnight UTC
  float fractDay = Jevent - (int)Jevent;
  int minutesUTC = (int)roundf(fractDay * 1440.0f) % 1440;
  if (minutesUTC < 0) minutesUTC += 1440;

  int minutesLocal = minutesUTC + tz * 60;
  if (minutesLocal < 0) minutesLocal += 1440;
  if (minutesLocal >= 1440) minutesLocal -= 1440;

  return minutesLocal;
}

void climate_recalculateSunTimes(uint16_t year, uint8_t month, uint8_t day) {
  // RTC year is 0-99 offset from 2000
  uint16_t fullYear = (year < 100) ? (2000 + year) : year;

  int riseMin = climate_sunEvent(fullYear, month, day, true);
  int setMin  = climate_sunEvent(fullYear, month, day, false);

  if (riseMin >= 0) {
    climateConfig.sunriseHour   = (uint8_t)(riseMin / 60);
    climateConfig.sunriseMinute = (uint8_t)(riseMin % 60);
  }
  if (setMin >= 0) {
    climateConfig.sunsetHour   = (uint8_t)(setMin / 60);
    climateConfig.sunsetMinute = (uint8_t)(setMin % 60);
  }

  Serial.print("[CLIMATE] Sunrise: ");
  Serial.print(climateConfig.sunriseHour);
  Serial.print(":");
  if (climateConfig.sunriseMinute < 10) Serial.print("0");
  Serial.print(climateConfig.sunriseMinute);
  Serial.print("  Sunset: ");
  Serial.print(climateConfig.sunsetHour);
  Serial.print(":");
  if (climateConfig.sunsetMinute < 10) Serial.print("0");
  Serial.println(climateConfig.sunsetMinute);
}

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

  // Set default configuration (GPS defaults = Prague, CET)
  climateConfig.mode = ClimateMode::MANUAL;
  climateConfig.sunriseHour = 6;
  climateConfig.sunriseMinute = 0;
  climateConfig.sunsetHour = 20;
  climateConfig.sunsetMinute = 0;
  climateConfig.latitude = 50.0f;
  climateConfig.longitude = 14.42f;
  climateConfig.timezoneOffsetH = 1;
  climateConfig.minTempC = 10.0f;
  climateConfig.maxTempC = 28.0f;
  climateConfig.overTempC = 30.0f;
  climateConfig.openWindowAboveTempC = 25.0f;
  climateConfig.closeWindowBelowTempC = 20.0f;
  climateConfig.windowHysteresisC = 2.0f;

  // Initial sun time calculation using current RTC date
  TimeData* t = rtc_getTime();
  climate_recalculateSunTimes(t->year, t->month, t->day);
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
static uint8_t lastSunCalcDay = 0;        // Day of last sun time recalculation

void climate_update() {
  unsigned long currentTime = millis();

  // Update current temperature from sensors
  if (coopEnvironment.isValid) {
    climateData.currentTempC = coopEnvironment.temperatureC;
  }

  // Get current time from RTC
  TimeData* time = rtc_getTime();

  // Recalculate sunrise/sunset once per day at midnight (when day changes)
  if (time->day != lastSunCalcDay) {
    lastSunCalcDay = time->day;
    climate_recalculateSunTimes(time->year, time->month, time->day);
  }

  // Determine if it's day or night (compare minutes-from-midnight)
  uint16_t nowMin     = (uint16_t)time->hour * 60 + time->minute;
  uint16_t sunriseMin = (uint16_t)climateConfig.sunriseHour * 60 + climateConfig.sunriseMinute;
  uint16_t sunsetMin  = (uint16_t)climateConfig.sunsetHour  * 60 + climateConfig.sunsetMinute;

  bool isDay = (nowMin >= sunriseMin && nowMin < sunsetMin);
  climateData.isNight = !isDay;

  // Handle different climate modes
  switch (climateConfig.mode) {
    // ========================================================================
    case ClimateMode::MANUAL: {
      // Manual mode - no automatic control
      // User commands via climate_doorOpen(), climate_doorClose(), etc.
      break;
    }

    // ========================================================================
    case ClimateMode::SCHEDULE: {
      // Scheduled mode - open/close at fixed times
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
      // Smart mode - temperature and weather aware
      // Combines scheduled and temperature-based control

      // Door control: open during day, close at night
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
