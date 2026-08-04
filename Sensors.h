#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Wire.h>
#include "Globals.h"

struct EnvironmentData {
  float temperatureC = 0.0f;
  float humidityPercent = 0.0f;
  float dewPointC = 0.0f;
  bool isValid = false;
  unsigned long lastReadMs = 0;
  unsigned long lastUpdateMs = 0;
};

struct ElectricalData {
  uint16_t doorCurrentMA = 0;
  uint16_t windowCurrentMA = 0;
  float systemVoltageV = 0.0f;
  bool isValid = false;
  unsigned long lastUpdateMs = 0;
};

struct OneWireTemperature {
  float temperatureC = 0.0f;
  bool isValid = false;
  unsigned long lastReadMs = 0;
};

struct CurrentMeasurement {
  uint16_t currentMA = 0;
  bool isValid = false;
  unsigned long lastReadMs = 0;
};

constexpr uint8_t SHT30_ADDR_COOP = 0x44;
constexpr uint8_t SHT30_ADDR_CABINET = 0x45;

extern EnvironmentData coopEnvironment;
extern EnvironmentData cabinetEnvironment;
extern ElectricalData electricalData;

extern OneWireTemperature externalTemperature;
extern OneWireTemperature cabinetTemperature;
extern OneWireTemperature heaterTemperature;
extern CurrentMeasurement currentSensors;

void sensors_init();
void sensors_update();

bool sensors_readSHT30(uint8_t address, EnvironmentData* data);
bool sensors_readDS18B20(uint8_t index, OneWireTemperature* temp);

EnvironmentData* sensors_getCoopEnvironment();
EnvironmentData* sensors_getCabinetEnvironment();
ElectricalData* sensors_getElectricalData();

float sensors_calculateDewPoint(float temperatureC, float humidityPercent);
const char* sensors_getStatus();

#endif // SENSORS_H
