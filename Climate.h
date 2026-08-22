#ifndef CLIMATE_H
#define CLIMATE_H

#include <Arduino.h>
#include "Globals.h"
#include "Door.h"
#include "Window.h"
#include "Heater.h"
#include "Light.h"

enum class ClimateMode : uint8_t {
  MANUAL = 0,
  SCHEDULE = 1,
  SMART = 2
};

struct ClimateConfig {
  ClimateMode mode = ClimateMode::MANUAL;

  uint8_t sunriseHour = 6;
  uint8_t sunriseMinute = 0;
  uint8_t sunsetHour = 20;
  uint8_t sunsetMinute = 0;

  float latitude = 50.0f;
  float longitude = 14.42f;
  int8_t timezoneOffsetH = 1;

  float minTempC = 10.0f;
  float maxTempC = 28.0f;
  float overTempC = 30.0f;

  float openWindowAboveTempC = 25.0f;
  float closeWindowBelowTempC = 20.0f;
  float windowHysteresisC = 2.0f;

  int16_t doorOpenOffsetMin = 15;
  int16_t doorCloseOffsetMin = 20;

  int16_t cameraOnOffsetMin = -30;
  int16_t cameraOffOffsetMin = 0;

  bool useCivilTwilight = false;
};

struct ClimateData {
  ClimateMode currentMode = ClimateMode::MANUAL;
  bool doorOpen = false;
  bool doorClosed = false;
  bool windowOpen = false;
  bool windowClosed = false;
  float currentTempC = 0.0f;
  float targetTempC = 20.0f;
  bool isNight = false;
  bool isSunrise = false;
  unsigned long lastUpdateMs = 0;

  int16_t sunriseTotalMin = 360;
  int16_t sunsetTotalMin = 1200;
  int16_t civilDawnMin = 330;
  int16_t civilDuskMin = 1230;
  uint16_t dayLengthMin = 840;

  int16_t doorOpenEffectiveMin = 375;
  int16_t doorCloseEffectiveMin = 1220;
  int16_t cameraOnEffectiveMin = 1170;
  int16_t cameraOffEffectiveMin = 1220;

  uint8_t lastCalcDay = 0;
  uint8_t lastCalcMonth = 0;
  uint16_t lastCalcYear = 0;
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
void climate_recalculateSunTimes(uint16_t year, uint8_t month, uint8_t day);
const char* climate_getModeName(ClimateMode mode);

#endif // CLIMATE_H
