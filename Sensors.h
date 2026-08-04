#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include "Globals.h"

struct EnvironmentData {
  float temperatureC = 0.0f;
  float humidityPercent = 0.0f;
  float dewPointC = 0.0f;
  bool isValid = false;
  unsigned long lastUpdateMs = 0;
};

struct ElectricalData {
  uint16_t doorCurrentMA = 0;
  uint16_t windowCurrentMA = 0;
  float systemVoltageV = 0.0f;
  bool isValid = false;
  unsigned long lastUpdateMs = 0;
};

extern EnvironmentData coopEnvironment;
extern EnvironmentData cabinetEnvironment;
extern ElectricalData electricalData;

void sensors_init();
void sensors_update();
EnvironmentData* sensors_getCoopEnvironment();
EnvironmentData* sensors_getCabinetEnvironment();
ElectricalData* sensors_getElectricalData();
float sensors_calculateDewPoint(float temperatureC, float humidityPercent);

#endif // SENSORS_H
