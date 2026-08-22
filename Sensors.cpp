#include "Sensors.h"

// ============================================================================
// GLOBAL SENSOR INSTANCES
// ============================================================================

EnvironmentData coopEnvironment;
EnvironmentData cabinetEnvironment;

// ============================================================================
// INITIALIZATION
// ============================================================================

void sensors_init() {
  // EnvironmentData fields are zero-initialized by default member initializers;
  // nothing additional to configure at startup.
}

// ============================================================================
// UPDATE - Read sensors
// ============================================================================

void sensors_update() {
  // TODO: Implement actual sensor reading from hardware
  // Coop temperature: Dallas DS18B20 on ONEWIRE_PIN
  // Cabinet temperature/humidity: DHT22 or similar
}
