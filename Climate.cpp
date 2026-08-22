#include "Climate.h"
#include <math.h>
#include "RTC.h"
#include "Sensors.h"

ClimateData climateData;
static ClimateConfig climateConfig;

static constexpr double D2R = M_PI / 180.0;

static double jdn(uint16_t year, uint8_t month, uint8_t day) {
  if (month <= 2) { year -= 1; month += 12; }
  const int A = year / 100;
  const int B = 2 - A + A / 4;
  return floor(365.25 * (year + 4716)) + floor(30.6001 * (month + 1)) + day + B - 1524.5;
}

static int dayOfWeek(uint16_t year, uint8_t month, uint8_t day) {
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (month < 3) year -= 1;
  return (year + year / 4 - year / 100 + year / 400 + t[month - 1] + day) % 7;
}

static bool isDST(uint16_t year, uint8_t month, uint8_t day) {
  if (month < 3 || month > 10) return false;
  if (month > 3 && month < 10) return true;
  const int dow31 = dayOfWeek(year, month, 31);
  const uint8_t lastSunday = static_cast<uint8_t>(31 - dow31);
  if (month == 3) return day >= lastSunday;
  return day < lastSunday;
}

static int sunEvent(uint16_t year, uint8_t month, uint8_t day, double latitude, double longitude, int timezoneOffsetH, bool sunrise, double zenith) {
  const int tz = timezoneOffsetH + (isDST(year, month, day) ? 1 : 0);
  const double JD = jdn(year, month, day);
  const double T = (JD - 2451545.0) / 36525.0;
  double L0 = fmod(280.46646 + T * (36000.76983 + T * 0.0003032), 360.0); if (L0 < 0.0) L0 += 360.0;
  double M = fmod(357.52911 + T * (35999.05029 - T * 0.0001537), 360.0); if (M < 0.0) M += 360.0;
  const double C = sin(M * D2R) * (1.914602 - T * (0.004817 + 0.000014 * T)) + sin(2.0 * M * D2R) * (0.019993 - 0.000101 * T) + sin(3.0 * M * D2R) * 0.000289;
  const double sunLon = L0 + C;
  const double omega = 125.04 - 1934.136 * T;
  const double appLon = sunLon - 0.00569 - 0.00478 * sin(omega * D2R);
  const double meanObl = 23.0 + (26.0 + (21.448 - T * (46.8150 + T * (0.00059 - T * 0.001813))) / 60.0) / 60.0;
  const double oblCorr = meanObl + 0.00256 * cos(omega * D2R);
  const double sinDecl = sin(oblCorr * D2R) * sin(appLon * D2R);
  const double cosDecl = cos(asin(sinDecl));
  const double e = 0.016708634 - T * (0.000042037 + 0.0000001267 * T);
  const double y = pow(tan(oblCorr / 2.0 * D2R), 2.0);
  const double eqTime = 4.0 * (y * sin(2.0 * L0 * D2R) - 2.0 * e * sin(M * D2R) + 4.0 * e * y * sin(M * D2R) * cos(2.0 * L0 * D2R) - 0.5 * y * y * sin(4.0 * L0 * D2R) - 1.25 * e * e * sin(2.0 * M * D2R)) * (180.0 / M_PI);
  const double latitudeRad = latitude * D2R;
  const double cosHA = (cos(zenith * D2R) - sinDecl * sin(latitudeRad)) / (cosDecl * cos(latitudeRad));
  if (cosHA < -1.0 || cosHA > 1.0) return -1;
  const double HA = acos(cosHA) / D2R;
  const double solarNoonUTC = 720.0 - 4.0 * longitude - eqTime;
  const double eventUTC = sunrise ? solarNoonUTC - 4.0 * HA : solarNoonUTC + 4.0 * HA;
  double eventLocal = eventUTC + static_cast<double>(tz * 60);
  while (eventLocal < 0.0) eventLocal += 1440.0;
  while (eventLocal >= 1440.0) eventLocal -= 1440.0;
  return static_cast<int>(round(eventLocal));
}

static int16_t normalizeMinutes(int32_t minutes) { while (minutes < 0) minutes += 1440; while (minutes >= 1440) minutes -= 1440; return static_cast<int16_t>(minutes); }

void climate_init() {
  climateData.currentMode = ClimateMode::MANUAL;
  climateData.doorOpen = (door_getState() == DoorState::OPEN);
  climateData.windowOpen = (window_getState() == WindowState::OPEN);
  climateData.doorClosed = (door_getState() == DoorState::CLOSED);
  climateData.windowClosed = (window_getState() == WindowState::CLOSED);
  climateData.currentTempC = 20.0f;
  climateData.targetTempC = 20.0f;
  climateData.isNight = false;
  climateData.isSunrise = false;
  climateData.lastUpdateMs = millis();
  climateConfig.mode = ClimateMode::MANUAL;
  climateConfig.sunriseHour = 6; climateConfig.sunriseMinute = 0; climateConfig.sunsetHour = 20; climateConfig.sunsetMinute = 0;
  climateConfig.latitude = 50.149041f; climateConfig.longitude = 15.550867f; climateConfig.timezoneOffsetH = 1;
  climateConfig.minTempC = 10.0f; climateConfig.maxTempC = 28.0f; climateConfig.overTempC = 30.0f; climateConfig.openWindowAboveTempC = 25.0f; climateConfig.closeWindowBelowTempC = 20.0f; climateConfig.windowHysteresisC = 2.0f;
  climateConfig.doorOpenOffsetMin = 15; climateConfig.doorCloseOffsetMin = 20; climateConfig.cameraOnOffsetMin = -30; climateConfig.cameraOffOffsetMin = 0; climateConfig.useCivilTwilight = false;
  TimeData* t = rtc_getTime();
  climate_recalculateSunTimes(t->year, t->month, t->day);
}

void climate_doorOpen() { door_open(); }
void climate_doorClose() { door_close(); }
void climate_windowOpen() { window_open(); climateData.windowOpen = true; climateData.windowClosed = false; }
void climate_windowClose() { window_close(); climateData.windowOpen = false; climateData.windowClosed = true; }
void climate_setMode(ClimateMode mode) { climateConfig.mode = mode; climateData.currentMode = mode; }
ClimateMode climate_getMode() { return climateData.currentMode; }

void climate_recalculateSunTimes(uint16_t year, uint8_t month, uint8_t day) {
  const double latitude = climateConfig.latitude;
  const double longitude = climateConfig.longitude;
  const int timezone = climateConfig.timezoneOffsetH;
  climateData.sunriseTotalMin = sunEvent(year, month, day, latitude, longitude, timezone, true, 90.833);
  climateData.sunsetTotalMin = sunEvent(year, month, day, latitude, longitude, timezone, false, 90.833);
  climateData.civilDawnMin = sunEvent(year, month, day, latitude, longitude, timezone, true, 96.0);
  climateData.civilDuskMin = sunEvent(year, month, day, latitude, longitude, timezone, false, 96.0);
  if (climateData.sunriseTotalMin >= 0 && climateData.sunsetTotalMin >= 0) { int16_t length = climateData.sunsetTotalMin - climateData.sunriseTotalMin; if (length < 0) length += 1440; climateData.dayLengthMin = static_cast<uint16_t>(length); } else { climateData.dayLengthMin = 0; }
  int16_t baseDoorOpen = climateData.sunriseTotalMin;
  int16_t baseDoorClose = climateData.sunsetTotalMin;
  if (climateConfig.useCivilTwilight) { if (climateData.civilDawnMin >= 0) baseDoorOpen = climateData.civilDawnMin; if (climateData.civilDuskMin >= 0) baseDoorClose = climateData.civilDuskMin; }
  climateData.doorOpenEffectiveMin = normalizeMinutes(static_cast<int32_t>(baseDoorOpen) + climateConfig.doorOpenOffsetMin);
  climateData.doorCloseEffectiveMin = normalizeMinutes(static_cast<int32_t>(baseDoorClose) + climateConfig.doorCloseOffsetMin);
  constexpr int16_t CAMERA_START_LEAD_MIN = 3;
  climateData.cameraOnEffectiveMin = normalizeMinutes(static_cast<int32_t>(climateData.doorCloseEffectiveMin) - CAMERA_START_LEAD_MIN);
  climateData.cameraOffEffectiveMin = normalizeMinutes(static_cast<int32_t>(climateData.doorCloseEffectiveMin) + climateConfig.cameraOffOffsetMin);
  climateData.lastCalcDay = day; climateData.lastCalcMonth = month; climateData.lastCalcYear = year;
}

void climate_update() {
  if (coopEnvironment.isValid) climateData.currentTempC = coopEnvironment.temperatureC;
  TimeData* time = rtc_getTime();
  if (time->day != climateData.lastCalcDay || time->month != climateData.lastCalcMonth || time->year != climateData.lastCalcYear) climate_recalculateSunTimes(time->year, time->month, time->day);
  const int16_t nowMin = static_cast<int16_t>(static_cast<uint16_t>(time->hour) * 60u + time->minute);
  const bool isDay = (climateData.sunriseTotalMin >= 0 && climateData.sunsetTotalMin >= 0 && nowMin >= climateData.sunriseTotalMin && nowMin < climateData.sunsetTotalMin);
  climateData.isNight = !isDay; climateData.isSunrise = (nowMin >= climateData.sunriseTotalMin && nowMin < (climateData.sunriseTotalMin + 30));
  if (door_getState() == DoorState::OPEN) climateData.doorOpen = true;
  if (door_getState() == DoorState::CLOSED) climateData.doorClosed = true;
  if (window_getState() == WindowState::OPEN) climateData.windowOpen = true;
  if (window_getState() == WindowState::CLOSED) climateData.windowClosed = true;

  bool doorShouldBeOpen = false;
  if (climateData.sunriseTotalMin >= 0 && climateData.sunsetTotalMin >= 0) {
    if (climateData.doorOpenEffectiveMin <= climateData.doorCloseEffectiveMin) doorShouldBeOpen = (nowMin >= climateData.doorOpenEffectiveMin && nowMin < climateData.doorCloseEffectiveMin);
    else doorShouldBeOpen = (nowMin >= climateData.doorOpenEffectiveMin || nowMin < climateData.doorCloseEffectiveMin);
  }

  static bool windowShouldBeOpen = false;
  switch (climateConfig.mode) {
    case ClimateMode::MANUAL: break;
    case ClimateMode::SCHEDULE:
    case ClimateMode::SMART:
      if (doorShouldBeOpen && door_getState() != DoorState::OPEN && door_getState() != DoorState::OPENING) climate_doorOpen();
      else if (!doorShouldBeOpen && door_getState() == DoorState::OPEN) climate_doorClose();
      if (climateData.currentTempC > climateConfig.openWindowAboveTempC) windowShouldBeOpen = true;
      else if (climateData.currentTempC < climateConfig.closeWindowBelowTempC) windowShouldBeOpen = false;
      if (windowShouldBeOpen && !climateData.windowOpen) climate_windowOpen();
      else if (!windowShouldBeOpen && climateData.windowOpen) climate_windowClose();
      break;
  }
  climateData.lastUpdateMs = millis();
}

ClimateConfig* climate_getConfig() { return &climateConfig; }
ClimateData* climate_getData() { return &climateData; }
const char* climate_getModeName(ClimateMode mode) { switch (mode) { case ClimateMode::MANUAL: return "MANUAL"; case ClimateMode::SCHEDULE: return "SCHEDULE"; case ClimateMode::SMART: return "SMART"; default: return "UNKNOWN"; } }
