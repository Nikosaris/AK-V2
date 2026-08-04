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
CurrentMeasurement currentSensors;

// ============================================================================
// INTERNÍ OBJEKTY
// ============================================================================

static OneWire oneWire(ONEWIRE_PIN);
static DallasTemperature ds18b20(&oneWire);
static unsigned long lastSensorReadMs = 0;

// ============================================================================
// INTERNÍ FUNKCE
// ============================================================================

static uint16_t sensors_readCurrentMA() {
  int32_t adcSum = 0;
  for (uint8_t sample = 0; sample < ACS712_AVERAGE_SAMPLES; ++sample) {
    adcSum += analogRead(ACS712_PIN);
  }

  const int32_t averageAdc = adcSum / ACS712_AVERAGE_SAMPLES;
  const int32_t millivolts = averageAdc * 3300 / 4095;
  const int32_t deltaMV = millivolts - ACS712_ZERO_OFFSET_MV;
  const int32_t currentMA = abs(deltaMV) * 1000 / ACS712_SENSITIVITY_MV_PER_A;
  return (currentMA > ACS712_MAX_VALID_CURRENT_MA) ? 0 : static_cast<uint16_t>(currentMA);
}

// ============================================================================
// INICIALIZACE
// ============================================================================

void sensors_init() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  ds18b20.begin();
  ds18b20.setWaitForConversion(false);

  coopEnvironment = {};
  cabinetEnvironment = {};
  electricalData = {};
  externalTemperature = {};
  currentSensors = {};
  lastSensorReadMs = 0;
}

// ============================================================================
// AKTUALIZACE - volat v hlavní smyčce
// ============================================================================

void sensors_update() {
  const unsigned long now = millis();
  if ((now - lastSensorReadMs) < SENSOR_READ_INTERVAL_MS) {
    return;
  }
  lastSensorReadMs = now;

  sensors_readSHT30(SHT30_ADDR_CABINET, &cabinetEnvironment);

  if (cabinetEnvironment.isValid) {
    cabinetEnvironment.dewPointC = sensors_calculateDewPoint(
      cabinetEnvironment.temperatureC,
      cabinetEnvironment.humidityPercent
    );
  }

  ds18b20.requestTemperatures();
  sensors_readDS18B20(&externalTemperature);

  if (externalTemperature.isValid) {
    coopEnvironment.temperatureC = externalTemperature.temperatureC;
    coopEnvironment.isValid = true;
    coopEnvironment.lastReadMs = externalTemperature.lastReadMs;
    coopEnvironment.lastUpdateMs = externalTemperature.lastReadMs;
  } else {
    coopEnvironment.isValid = false;
  }

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

bool sensors_readDS18B20(OneWireTemperature* temp) {
  if (temp == nullptr) return false;

  float t = ds18b20.getTempCByIndex(0);

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
