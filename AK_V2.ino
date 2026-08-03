// ============================================================================
// AK-V2: Professional Chicken Coop Automation Firmware
// Platform: ESP32-WROOM-32
// ============================================================================

#include "Globals.h"
#include "Hardware.h"
#include "Motor.h"
#include "Sensors.h"
#include "Settings.h"
#include "Ethernet.h"
#include "WiFi.h"
#include <cstring>

// ============================================================================
// MOTOR INSTANCES
// ============================================================================

static unsigned long networkInitTimeMs = 0;

static void tryWiFiFallback() {
  if (!NETWORK_USE_WIFI_FALLBACK) {
    return;
  }

  if (strlen(WIFI_DEBUG_SSID) == 0) {
    return;
  }

  if (!wifi_isConnected() && wifi_getState() != WiFiState::CONNECTING) {
    wifi_connect(WIFI_DEBUG_SSID, WIFI_DEBUG_PASSWORD);
    Serial.println("[NET] WiFi fallback connect requested");
  }
}

// ============================================================================
// SETUP - Initialization
// ============================================================================

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(SERIAL_BAUD_RATE);
  delay(100);

  Serial.println("\n\n=================================================================================");
  Serial.println("AK-V2: Professional Chicken Coop Automation Firmware");
  Serial.println("Platform: ESP32-WROOM-32");
  Serial.println("Version: 0.2.0");
  Serial.println("=================================================================================");

  // Initialize globals
  globals_init();
  Serial.println("[INIT] Globals initialized");

  // Initialize hardware (GPIO, PWM, etc.)
  hardware_init();
  Serial.println("[INIT] Hardware initialized");

  // Initialize settings system
  settings_init();
  Serial.println("[INIT] Settings loaded");

  // Initialize sensor layer
  sensors_init();
  Serial.println("[INIT] Sensors initialized");

  // Initialize networking (Ethernet primary, WiFi fallback for debugging)
  wifi_init();
  if (NETWORK_USE_ETHERNET_PRIMARY) {
    ethernet_init();
  }
  networkInitTimeMs = millis();
  if (NETWORK_USE_ETHERNET_PRIMARY && ethernet_connect()) {
    Serial.println("[NET] Ethernet W5500 initialization started (primary)");
  } else {
    Serial.println("[NET] Ethernet init failed, trying WiFi fallback");
    tryWiFiFallback();
  }

  // Initialize motor structures
  motor_init(&doorMotor, "Door", DOOR_IN1_PIN, DOOR_IN2_PIN);
  doorMotor.config = *settings_getDoorConfig();
  Serial.println("[INIT] Door motor initialized");

  motor_init(&windowMotor, "Window", WINDOW_IN1_PIN, WINDOW_IN2_PIN);
  windowMotor.config = *settings_getWindowConfig();
  Serial.println("[INIT] Window motor initialized");

  Serial.println("\n[INIT] System startup complete. Ready to operate.\n");
}

// ============================================================================
// LOOP - Main control loop
// ============================================================================

void loop() {
  unsigned long loopStartTime = millis();

  // Update global timing
  globals_update();

  // Update hardware state
  hardware_update();

  // Update sensor layer
  sensors_update();

  // Update network managers
  if (NETWORK_USE_ETHERNET_PRIMARY) {
    ethernet_update();
  }
  wifi_update();

  // Ethernet is primary; if unavailable for some time, allow WiFi fallback
  if ((!NETWORK_USE_ETHERNET_PRIMARY || !ethernet_isConnected()) &&
      (millis() - networkInitTimeMs) > ETHERNET_WIFI_FALLBACK_DELAY_MS) {
    tryWiFiFallback();
  }

  // Core motor control - STATE MACHINE UPDATES
  motor_update(&doorMotor);
  motor_update(&windowMotor);

  // Periodic serial logging (every 1 second)
  unsigned long currentTime = millis();
  if (currentTime - lastSerialLogTime >= SERIAL_LOG_INTERVAL_MS) {
    lastSerialLogTime = currentTime;
    logSystemStatus();
  }

  // Frame rate limiting - ensure loop runs at ~20 Hz (50ms)
  unsigned long loopDuration = millis() - loopStartTime;
  if (loopDuration < LOOP_INTERVAL_MS) {
    delayMicroseconds((LOOP_INTERVAL_MS - loopDuration) * 1000);
  }
}

// ============================================================================
// LOGGING AND DIAGNOSTICS
// ============================================================================

void logSystemStatus() {
  static uint32_t logCounter = 0;
  logCounter++;

  Serial.print("[LOG-");
  Serial.print(logCounter);
  Serial.print("] Uptime: ");
  Serial.print(systemUptime / 1000);
  Serial.print("s | Door: ");
  Serial.print(motor_getStateName(doorMotor.data.state));
  Serial.print(" (I=");
  Serial.print(doorMotor.data.currentMA);
  Serial.print("mA) | Window: ");
  Serial.print(motor_getStateName(windowMotor.data.state));
  Serial.print(" (I=");
  Serial.print(windowMotor.data.currentMA);
  Serial.println("mA)");
}
