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
//
// NOTE: All intermediate JD arithmetic is done in double to avoid float32
// precision loss. A 32-bit float has only ~7 significant digits; JD values
// near 2,451,545 lose the fractional part to rounding steps of 0.25
// (= 6 h), producing errors of ±90 minutes. Using double fixes this.
// ============================================================================

static const double DEG2RAD_D = M_PI / 180.0;

// Return 1 if EU summer time (DST) is in effect on the given date, else 0.
// EU rule: DST starts last Sunday of March at 01:00 UTC,
//          ends   last Sunday of October at 01:00 UTC.
static int climate_isDST_EU(uint16_t year, uint8_t month, uint8_t day) {
  if (month < 3 || month > 10) return 0;
  if (month > 3 && month < 10) return 1;

  // Find day-of-week of the 31st (or 30th for April/Sept) of the month.
  // Zeller's formula for day-of-week (0=Sun … 6=Sat):
  uint8_t checkDay = 31; // both March and October have 31 days
  // Tomohiko Sakamoto's algorithm
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  uint16_t y = year - (checkDay < 3 ? 1 : 0);
  uint8_t dow31 = (y + y/4 - y/100 + y/400 + t[checkDay-1] + checkDay) % 7;
  // Last Sunday of month = 31 - ((dow31 == 0) ? 0 : dow31)
  // But for March dow of day 31; last Sunday = 31 - dow31 (if dow31>0) else 31
  uint8_t lastSun = (uint8_t)(31 - dow31);

  if (month == 3)  return (day >= lastSun) ? 1 : 0;
  else             return (day <  lastSun) ? 1 : 0;
}

// Compute Julian Day Number from calendar date (Gregorian calendar).
// Returns a double to preserve the fractional part needed for time calculation.
static double climate_julianDay(uint16_t year, uint8_t month, uint8_t day) {
  if (month <= 2) {
    year -= 1;
    month += 12;
  }
  int A = (int)(year / 100);
  int B = 2 - A + (int)(A / 4);
  return (long)(365.25 * (year + 4716)) + (int)(30.6001 * (month + 1)) + day + B - 1524.5;
}

// Calculate sunrise or sunset in minutes from midnight (local time).
// isSunrise = true → sunrise, false → sunset.
// Returns -1 on failure (polar).
static int climate_sunEvent(uint16_t year, uint8_t month, uint8_t day, bool isSunrise) {
  double lat = (double)climateConfig.latitude;
  double lon = (double)climateConfig.longitude;
  // Apply DST on top of the base UTC offset stored in config
  int tz = (int)climateConfig.timezoneOffsetH + climate_isDST_EU(year, month, day);

  // All JD arithmetic in double to avoid float32 precision loss (~0.25 JD steps)
  double jd = climate_julianDay(year, month, day);
  // Work relative to J2000 epoch to keep numbers small
  double n  = jd - 2451545.0 + 0.0008;

  double Js     = n - lon / 360.0;
  double M      = fmod(357.5291 + 0.98560028 * Js, 360.0);
  double C      = 1.9148 * sin(M * DEG2RAD_D)
                + 0.0200 * sin(2.0 * M * DEG2RAD_D)
                + 0.0003 * sin(3.0 * M * DEG2RAD_D);
  double lambda = fmod(M + C + 180.0 + 102.9372, 360.0);

  double Jt    = 2451545.0 + Js + 0.0053 * sin(M * DEG2RAD_D) - 0.0069 * sin(2.0 * lambda * DEG2RAD_D);
  double sinD  = sin(lambda * DEG2RAD_D) * sin(23.4397 * DEG2RAD_D);
  double cosD  = cos(asin(sinD));

  double cosH  = (sin(-0.8333 * DEG2RAD_D) - sin(lat * DEG2RAD_D) * sinD)
                 / (cos(lat * DEG2RAD_D) * cosD);

  if (cosH < -1.0 || cosH > 1.0) {
    return -1; // polar day or night
  }

  double H      = acos(cosH) * (180.0 / M_PI);
  double Jevent = isSunrise ? (Jt - H / 360.0) : (Jt + H / 360.0);

  // Extract fractional day relative to J2000 epoch (keeps value small → no precision loss)
  double fracFromEpoch = Jevent - 2451545.0;
  // Convert to minutes-from-midnight UTC: only the sub-day fraction matters
  double dayFrac = fracFromEpoch - floor(fracFromEpoch);
  int minutesUTC = (int)round(dayFrac * 1440.0) % 1440;
  if (minutesUTC < 0) minutesUTC += 1440;

  int minutesLocal = minutesUTC + tz * 60;
  if (minutesLocal < 0)    minutesLocal += 1440;
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
