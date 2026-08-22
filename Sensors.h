#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include "Globals.h"

struct EnvironmentData {
  bool isValid = false;
  float temperatureC = 0.0f;
  float humidityPct = 0.0f;
  float dewPointC = 0.0f;
};

extern EnvironmentData coopEnvironment;
extern EnvironmentData cabinetEnvironment;

void sensors_init();
void sensors_update();

#endif // SENSORS_H
