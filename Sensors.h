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

constexpr uint8_t SHT30_ADDR_CABINET = 0x45;
constexpr uint32_t SENSOR_READ_INTERVAL_MS = 1000;
constexpr uint8_t ACS712_AVERAGE_SAMPLES = 16;
constexpr int32_t ACS712_ZERO_OFFSET_MV = 2500;
constexpr uint16_t ACS712_SENSITIVITY_MV_PER_A = 185;
constexpr uint16_t ACS712_MAX_VALID_CURRENT_MA = 5000;

extern EnvironmentData coopEnvironment;
extern EnvironmentData cabinetEnvironment;
extern ElectricalData electricalData;

extern OneWireTemperature externalTemperature;
extern CurrentMeasurement currentSensors;

void sensors_init();
void sensors_update();

bool sensors_readSHT30(uint8_t address, EnvironmentData* data);
bool sensors_readDS18B20(OneWireTemperature* temp);

EnvironmentData* sensors_getCoopEnvironment();
EnvironmentData* sensors_getCabinetEnvironment();
ElectricalData* sensors_getElectricalData();

float sensors_calculateDewPoint(float temperatureC, float humidityPercent);
const char* sensors_getStatus();

#endif // SENSORS_H
