#include "Sensors.h"

#include <math.h>

EnvironmentData coopEnvironment;
EnvironmentData cabinetEnvironment;
ElectricalData electricalData;

static uint16_t sensors_readCurrentMA() {
  const int32_t millivolts = static_cast<int32_t>(analogRead(ACS712_PIN)) * 3300 / 4095;
  const int32_t deltaMillivolts = millivolts - 2500;
  const int32_t currentMA = abs(deltaMillivolts) * 1000 / 185;

  if (currentMA > 5000) {
    return 0;
  }

  return static_cast<uint16_t>(currentMA);
}

float sensors_calculateDewPoint(float temperatureC, float humidityPercent) {
  if (humidityPercent <= 0.0f || humidityPercent > 100.0f) {
    return temperatureC;
  }

  const float a = 17.62f;
  const float b = 243.12f;
  const float gamma = logf(humidityPercent / 100.0f) + ((a * temperatureC) / (b + temperatureC));
  return (b * gamma) / (a - gamma);
}

void sensors_init() {
  coopEnvironment = {};
  cabinetEnvironment = {};
  electricalData = {};
}

void sensors_update() {
  const unsigned long currentTime = millis();
  const uint16_t currentMA = sensors_readCurrentMA();

  electricalData.doorCurrentMA = currentMA;
  electricalData.windowCurrentMA = currentMA;
  electricalData.systemVoltageV = 3.3f;
  electricalData.isValid = true;
  electricalData.lastUpdateMs = currentTime;

  coopEnvironment.lastUpdateMs = currentTime;
  cabinetEnvironment.lastUpdateMs = currentTime;
}

EnvironmentData* sensors_getCoopEnvironment() {
  return &coopEnvironment;
}

EnvironmentData* sensors_getCabinetEnvironment() {
  return &cabinetEnvironment;
}

ElectricalData* sensors_getElectricalData() {
  return &electricalData;
}
