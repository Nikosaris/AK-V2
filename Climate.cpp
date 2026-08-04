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
// SUN TIME CALCULATION
//
// Algorithm based on Dusk2Dawn library (R.J. Salmon), which implements the
// NOAA Solar Calculator equations:
//   https://www.esrl.noaa.gov/gmd/grad/solcalc/
//
// Accuracy: ±1 minute for mid-latitudes (matches project requirement).
//
// All intermediate Julian Day (JD) arithmetic is performed in double to
// avoid float32 precision loss.  A 32-bit float has only ~7 significant
// digits; JD values near 2,451,545 lose sub-day resolution in steps of
// ~0.25 JD (= 6 h), causing errors up to ±90 minutes.
// ============================================================================

static const double D2R = M_PI / 180.0; // degrees → radians

// ---------------------------------------------------------------------------
// Gregorian calendar → Julian Day Number (double precision)
// ---------------------------------------------------------------------------
static double jdn(uint16_t year, uint8_t month, uint8_t day) {
  if (month <= 2) { year -= 1; month += 12; }
  int A = (int)(year / 100);
  int B = 2 - A + (int)(A / 4);
  return floor(365.25 * (year + 4716)) + floor(30.6001 * (month + 1)) + day + B - 1524.5;
}

// ---------------------------------------------------------------------------
// EU DST detection
// Returns 1 if summer time (CEST) is active on the given date, else 0.
// Rule: starts last Sunday of March 01:00 UTC, ends last Sunday of October 01:00 UTC.
// ---------------------------------------------------------------------------
static int isDST(uint16_t year, uint8_t month, uint8_t day) {
  if (month < 3 || month > 10) return 0;
  if (month > 3 && month < 10) return 1;
  // Tomohiko Sakamoto's weekday algorithm (0 = Sunday).
  // We need the day-of-week of the 31st of the month (both March and October have 31 days).
  // t[] offset table indexed by month-1.
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  uint16_t y = year; // day 31 >= 3, so no year adjustment needed
  uint8_t dow31 = (uint8_t)((y + y/4 - y/100 + y/400 + t[month - 1] + 31) % 7);
  uint8_t lastSun = (uint8_t)(31 - dow31); // last Sunday of the month (dow31==0 → 31st is Sunday)
  if (month == 3)  return (day >= lastSun) ? 1 : 0;
  else             return (day <  lastSun) ? 1 : 0;
}

// ---------------------------------------------------------------------------
// Core solar calculation (Dusk2Dawn / NOAA method)
//
// zenith:
//   90.833  – standard sunrise/sunset (upper limb, atmospheric refraction)
//    96.0   – civil twilight (6° below geometric horizon)
//
// Returns minutes from midnight (local time), or -1 on polar day/night.
// ---------------------------------------------------------------------------
static int sunEvent(uint16_t year, uint8_t month, uint8_t day,
                    bool isSunrise, double zenith) {
  double lat = (double)climateConfig.latitude;
  double lon = (double)climateConfig.longitude;
  int    tz  = (int)climateConfig.timezoneOffsetH + isDST(year, month, day);

  double JD = jdn(year, month, day);

  // Julian century relative to J2000.0
  double t = (JD - 2451545.0) / 36525.0;

  // Geometric mean longitude of the Sun (degrees)
  double L0 = fmod(280.46646 + t * (36000.76983 + t * 0.0003032), 360.0);

  // Geometric mean anomaly of the Sun (degrees)
  double M  = fmod(357.52911 + t * (35999.05029 - t * 0.0001537), 360.0);

  // Equation of the centre
  double C  = sin(M * D2R) * (1.914602 - t * (0.004817 + 0.000014 * t))
            + sin(2.0 * M * D2R) * (0.019993 - 0.000101 * t)
            + sin(3.0 * M * D2R) * 0.000289;

  // Sun's true longitude and anomaly (degrees)
  double sunLon = L0 + C;

  // Apparent longitude (correct for nutation and aberration)
  double omega   = 125.04 - 1934.136 * t;
  double appLon  = sunLon - 0.00569 - 0.00478 * sin(omega * D2R);

  // Mean obliquity of the ecliptic
  double meanObl = 23.0 + (26.0 + (21.448 - t * (46.8150 + t * (0.00059 - t * 0.001813))) / 60.0) / 60.0;

  // Corrected obliquity
  double oblCorr = meanObl + 0.00256 * cos(omega * D2R);

  // Sun's right ascension (degrees) – unused but kept for completeness
  // double RA = atan2(cos(oblCorr * D2R) * sin(appLon * D2R), cos(appLon * D2R)) / D2R;

  // Sun's declination (degrees)
  double sinDecl = sin(oblCorr * D2R) * sin(appLon * D2R);
  double cosDecl = cos(asin(sinDecl));

  // Equation of time (minutes)
  double e     = 0.016708634 - t * (0.000042037 + 0.0000001267 * t);
  double y_val = pow(tan(oblCorr / 2.0 * D2R), 2.0);
  double eqTime = 4.0 * (y_val * sin(2.0 * L0 * D2R)
                - 2.0 * e * sin(M * D2R)
                + 4.0 * e * y_val * sin(M * D2R) * cos(2.0 * L0 * D2R)
                - 0.5 * y_val * y_val * sin(4.0 * L0 * D2R)
                - 1.25 * e * e * sin(2.0 * M * D2R)) * (180.0 / M_PI);

  // Hour angle of sunrise/sunset (degrees)
  double cosHA = (cos(zenith * D2R) - sinDecl * sin(lat * D2R))
                 / (cosDecl * cos(lat * D2R));

  if (cosHA < -1.0) return -1; // polar day  (sun never sets)
  if (cosHA >  1.0) return -1; // polar night (sun never rises)

  double HA = acos(cosHA) / D2R; // 0–180°

  // Solar noon in minutes (UTC)
  double solarNoonUTC = 720.0 - 4.0 * lon - eqTime;

  // Event time (UTC minutes)
  double eventUTC = isSunrise ? (solarNoonUTC - 4.0 * HA)
                              : (solarNoonUTC + 4.0 * HA);

  // Convert to local time
  double eventLocal = eventUTC + tz * 60.0;
  // Wrap to [0, 1440)
  while (eventLocal <    0.0) eventLocal += 1440.0;
  while (eventLocal >= 1440.0) eventLocal -= 1440.0;

  return (int)round(eventLocal);
}

// ---------------------------------------------------------------------------
// Public: recalculate all sun times and effective action times
// ---------------------------------------------------------------------------
void climate_recalculateSunTimes(uint16_t year, uint8_t month, uint8_t day) {
  uint16_t fullYear = (year < 100) ? (uint16_t)(2000 + year) : year;

  // Standard sunrise / sunset  (zenith = 90.833°)
  int riseMin = sunEvent(fullYear, month, day, true,  90.833);
  int setMin  = sunEvent(fullYear, month, day, false, 90.833);

  // Civil dawn / dusk  (zenith = 96.0°)
  int dawnMin = sunEvent(fullYear, month, day, true,  96.0);
  int duskMin = sunEvent(fullYear, month, day, false, 96.0);

  if (riseMin >= 0) {
    climateData.sunriseTotalMin = (int16_t)riseMin;
    climateConfig.sunriseHour   = (uint8_t)(riseMin / 60);
    climateConfig.sunriseMinute = (uint8_t)(riseMin % 60);
  }
  if (setMin >= 0) {
    climateData.sunsetTotalMin  = (int16_t)setMin;
    climateConfig.sunsetHour    = (uint8_t)(setMin / 60);
    climateConfig.sunsetMinute  = (uint8_t)(setMin % 60);
  }
  if (dawnMin >= 0) climateData.civilDawnMin = (int16_t)dawnMin;
  if (duskMin >= 0) climateData.civilDuskMin = (int16_t)duskMin;

  if (riseMin >= 0 && setMin >= 0)
    climateData.dayLengthMin = (uint16_t)(setMin - riseMin);

  // Effective action times (wrap into [0, 1440))
  auto wrapMin = [](int v) -> int16_t {
    while (v <    0) v += 1440;
    while (v >= 1440) v -= 1440;
    return (int16_t)v;
  };

  int baseOpen  = climateConfig.useCivilTwilight ? (int)climateData.civilDawnMin : riseMin;
  int baseClose = climateConfig.useCivilTwilight ? (int)climateData.civilDuskMin : setMin;

  climateData.doorOpenEffectiveMin  = wrapMin(baseOpen  + (int)climateConfig.doorOpenOffsetMin);
  climateData.doorCloseEffectiveMin = wrapMin(baseClose + (int)climateConfig.doorCloseOffsetMin);
  climateData.cameraOnEffectiveMin  = wrapMin(setMin    + (int)climateConfig.cameraOnOffsetMin);
  climateData.cameraOffEffectiveMin = wrapMin((int)climateData.doorCloseEffectiveMin
                                              + (int)climateConfig.cameraOffOffsetMin);

  // Record last calculation date
  climateData.lastCalcDay   = day;
  climateData.lastCalcMonth = month;
  climateData.lastCalcYear  = fullYear;

  // Serial log
  Serial.printf("[CLIMATE] %04u-%02u-%02u  Sunrise: %02u:%02u  Sunset: %02u:%02u"
                "  Day: %u min  CivilDawn: %02d:%02d  CivilDusk: %02d:%02d\n",
                fullYear, month, day,
                climateConfig.sunriseHour, climateConfig.sunriseMinute,
                climateConfig.sunsetHour,  climateConfig.sunsetMinute,
                (unsigned)climateData.dayLengthMin,
                climateData.civilDawnMin / 60, climateData.civilDawnMin % 60,
                climateData.civilDuskMin / 60, climateData.civilDuskMin % 60);
  Serial.printf("[CLIMATE] Door open: %02d:%02d  Door close: %02d:%02d"
                "  Camera on: %02d:%02d  Camera off: %02d:%02d\n",
                climateData.doorOpenEffectiveMin  / 60, climateData.doorOpenEffectiveMin  % 60,
                climateData.doorCloseEffectiveMin / 60, climateData.doorCloseEffectiveMin % 60,
                climateData.cameraOnEffectiveMin  / 60, climateData.cameraOnEffectiveMin  % 60,
                climateData.cameraOffEffectiveMin / 60, climateData.cameraOffEffectiveMin % 60);
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

  // Default configuration – adjust coordinates and timezone via web interface
  climateConfig.mode = ClimateMode::MANUAL;
  climateConfig.sunriseHour = 6;
  climateConfig.sunriseMinute = 0;
  climateConfig.sunsetHour = 20;
  climateConfig.sunsetMinute = 0;
  climateConfig.latitude  = 50.149041f;  // Hradec Králové, CZ
  climateConfig.longitude = 15.550867f;
  climateConfig.timezoneOffsetH = 1;     // CET (DST added automatically)
  climateConfig.minTempC = 10.0f;
  climateConfig.maxTempC = 28.0f;
  climateConfig.overTempC = 30.0f;
  climateConfig.openWindowAboveTempC  = 25.0f;
  climateConfig.closeWindowBelowTempC = 20.0f;
  climateConfig.windowHysteresisC = 2.0f;
  climateConfig.doorOpenOffsetMin   =  15;
  climateConfig.doorCloseOffsetMin  =  20;
  climateConfig.cameraOnOffsetMin   = -30;
  climateConfig.cameraOffOffsetMin  =   0;
  climateConfig.useCivilTwilight    = false;

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

void climate_update() {
  // Update current temperature from sensors
  if (coopEnvironment.isValid) {
    climateData.currentTempC = coopEnvironment.temperatureC;
  }

  // Get current time from RTC
  TimeData* time = rtc_getTime();

  // Recalculate sunrise/sunset once per day when day changes
  if (time->day != climateData.lastCalcDay) {
    climate_recalculateSunTimes(time->year, time->month, time->day);
  }

  // Current time in minutes from midnight
  int16_t nowMin = (int16_t)((uint16_t)time->hour * 60u + time->minute);

  // Day / night based on actual sunrise/sunset (not effective times)
  bool isDay = (nowMin >= climateData.sunriseTotalMin && nowMin < climateData.sunsetTotalMin);
  climateData.isNight = !isDay;

  // Door should be open window: doorOpenEffective → doorCloseEffective
  bool doorShouldBeOpen;
  if (climateData.doorOpenEffectiveMin <= climateData.doorCloseEffectiveMin) {
    doorShouldBeOpen = (nowMin >= climateData.doorOpenEffectiveMin
                     && nowMin <  climateData.doorCloseEffectiveMin);
  } else {
    // Wraps midnight (edge case for extreme offsets)
    doorShouldBeOpen = (nowMin >= climateData.doorOpenEffectiveMin
                     || nowMin <  climateData.doorCloseEffectiveMin);
  }

  switch (climateConfig.mode) {
    // ========================================================================
    case ClimateMode::MANUAL: {
      // Manual mode – no automatic control
      break;
    }

    // ========================================================================
    case ClimateMode::SCHEDULE: {
      // Scheduled mode: door follows sunrise/sunset with configured offsets
      if (doorShouldBeOpen && !climateData.doorOpen) {
        climate_doorOpen();
      } else if (!doorShouldBeOpen && climateData.doorOpen) {
        climate_doorClose();
      }

      // Window: temperature-based, no hysteresis
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
      // Smart mode: door follows offset times AND temperature safety
      bool shouldOpen = doorShouldBeOpen
                     && climateData.currentTempC < climateConfig.overTempC;

      if (shouldOpen && !climateData.doorOpen) {
        climate_doorOpen();
      } else if (!shouldOpen && climateData.doorOpen) {
        climate_doorClose();
      }

      // Window: temperature-based with hysteresis
      if (climateData.currentTempC > climateConfig.openWindowAboveTempC) {
        windowShouldBeOpen = true;
      } else if (climateData.currentTempC
                   < (climateConfig.closeWindowBelowTempC - climateConfig.windowHysteresisC)) {
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

  climateData.lastUpdateMs = millis();
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
