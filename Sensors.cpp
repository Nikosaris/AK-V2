#include "Sensors.h"
#include <math.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ============================================================================
// GLOBÁLNÍ INSTANCE
// ============================================================================

EnvironmentData  coopEnvironment;
EnvironmentData  cabinetEnvironment;
ElectricalData   electricalData;

OneWireTemperature externalTemperature;
OneWireTemperature cabinetTemperature;
OneWireTemperature heaterTemperature;
CurrentMeasurement currentSensors;

// ============================================================================
// INTERNÍ OBJEKTY
// ============================================================================

static OneWire oneWire(ONEWIRE_PIN);
static DallasTemperature ds18b20(&oneWire);

// ============================================================================
// INTERNÍ FUNKCE
// ============================================================================

static uint16_t sensors_readCurrentMA() {
  const int32_t millivolts = static_cast<int32_t>(analogRead(ACS712_PIN)) * 3300 / 4095;
  const int32_t deltaMV = millivolts - 2500;
  const int32_t currentMA = abs(deltaMV) * 1000 / 185;
  return (currentMA > 5000) ? 0 : static_cast<uint16_t>(currentMA);
}

// ============================================================================
// INICIALIZACE
// ============================================================================

void sensors_init() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  ds18b20.begin();

  coopEnvironment = {};
  cabinetEnvironment = {};
  electricalData = {};
  externalTemperature = {};
  cabinetTemperature = {};
  heaterTemperature = {};
  currentSensors = {};
}

// ============================================================================
// AKTUALIZACE - volat v hlavní smyčce
// ============================================================================

void sensors_update() {
  const unsigned long now = millis();

  sensors_readSHT30(SHT30_ADDR_COOP, &coopEnvironment);
  sensors_readSHT30(SHT30_ADDR_CABINET, &cabinetEnvironment);

  if (cabinetEnvironment.isValid) {
    cabinetEnvironment.dewPointC = sensors_calculateDewPoint(
      cabinetEnvironment.temperatureC,
      cabinetEnvironment.humidityPercent
    );
  }

  sensors_readDS18B20(0, &externalTemperature);
  sensors_readDS18B20(1, &cabinetTemperature);
  sensors_readDS18B20(2, &heaterTemperature);

  const uint16_t currentMA = sensors_readCurrentMA();
  currentSensors.currentMA = currentMA;
  currentSensors.isValid = true;
  currentSensors.lastReadMs = now;

  electricalData.doorCurrentMA = currentMA;
  electricalData.windowCurrentMA = currentMA;
  electricalData.systemVoltageV = 3.3f;
  electricalData.isValid = true;
  electricalData.lastUpdateMs = now;
}

// ============================================================================
// ČTENÍ SHT30 (I2C)
// ============================================================================

bool sensors_readSHT30(uint8_t address, EnvironmentData* data) {
  if (data == nullptr) return false;

  Wire.beginTransmission(address);
  Wire.write(0x2C);
  Wire.write(0x06);
  if (Wire.endTransmission() != 0) {
    data->isValid = false;
    return false;
  }

  delay(20);

  if (Wire.requestFrom((int)address, 6) != 6) {
    data->isValid = false;
    return false;
  }

  uint8_t buf[6];
  for (uint8_t i = 0; i < 6; i++) buf[i] = Wire.read();

  uint16_t rawTemp = ((uint16_t)buf[0] << 8) | buf[1];
  uint16_t rawHum = ((uint16_t)buf[3] << 8) | buf[4];

  data->temperatureC = -45.0f + 175.0f * ((float)rawTemp / 65535.0f);
  data->humidityPercent = 100.0f * ((float)rawHum / 65535.0f);
  data->isValid = true;
  data->lastReadMs = millis();
  data->lastUpdateMs = data->lastReadMs;

  return true;
}

// ============================================================================
// ČTENÍ DS18B20 (OneWire)
// ============================================================================

bool sensors_readDS18B20(uint8_t index, OneWireTemperature* temp) {
  if (temp == nullptr) return false;

  ds18b20.requestTemperatures();
  float t = ds18b20.getTempCByIndex(index);

  if (t == DEVICE_DISCONNECTED_C || t < -55.0f || t > 125.0f) {
    temp->isValid = false;
    return false;
  }

  temp->temperatureC = t;
  temp->isValid = true;
  temp->lastReadMs = millis();
  return true;
}

// ============================================================================
// ACCESSORY FUNKCE PRO WEBSERVER
// ============================================================================

EnvironmentData* sensors_getCoopEnvironment() {
  return &coopEnvironment;
}

EnvironmentData* sensors_getCabinetEnvironment() {
  return &cabinetEnvironment;
}

ElectricalData* sensors_getElectricalData() {
  return &electricalData;
}

// ============================================================================
// VÝPOČET ROSNÉHO BODU
// ============================================================================

float sensors_calculateDewPoint(float temperatureC, float humidityPercent) {
  if (humidityPercent <= 0.0f || humidityPercent > 100.0f) return temperatureC;
  const float a = 17.62f;
  const float b = 243.12f;
  const float gamma = logf(humidityPercent / 100.0f) + (a * temperatureC) / (b + temperatureC);
  return (b * gamma) / (a - gamma);
}

// ============================================================================
// STATUS
// ============================================================================

const char* sensors_getStatus() {
  if (!externalTemperature.isValid && !currentSensors.isValid) return "ERROR";
  if (!externalTemperature.isValid || !currentSensors.isValid) return "PARTIAL";
  return "OK";
}
