// ============================================================================
// AK-V2: Professional Chicken Coop Automation Firmware
// Platform: ESP32-WROOM-32
// ============================================================================

#include "Globals.h"
#include "Hardware.h"
#include "Door.h"
#include "Window.h"
#include "Settings.h"

// ============================================================================
// SETUP - Initialization
// ============================================================================

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(100);

  Serial.println("\n\n=================================================================================");
  Serial.println("AK-V2: Professional Chicken Coop Automation Firmware");
  Serial.println("Platform: ESP32-WROOM-32");
  Serial.println("Version: 0.1.0");
  Serial.println("=================================================================================");

  globals_init();
  Serial.println("[INIT] Globals initialized");

  hardware_init();
  Serial.println("[INIT] Hardware initialized");

  settings_init();
  Serial.println("[INIT] Settings loaded");

  door_init(PWM_CHANNEL_DOOR_IN1, PWM_CHANNEL_DOOR_IN2);
  door_setConfig(*settings_getDoorConfig());
  Serial.println("[INIT] Door initialized");

  window_init(PWM_CHANNEL_WINDOW_IN1, PWM_CHANNEL_WINDOW_IN2);
  window_setConfig(*settings_getWindowConfig());
  Serial.println("[INIT] Window initialized");

  Serial.println("\n[INIT] System startup complete. Ready to operate.\n");
}

// ============================================================================
// LOOP - Main control loop
// ============================================================================

void loop() {
  unsigned long loopStartTime = millis();

  globals_update();
  hardware_update();

  door_update();
  window_update();

  unsigned long currentTime = millis();
  if (currentTime - lastSerialLogTime >= SERIAL_LOG_INTERVAL_MS) {
    lastSerialLogTime = currentTime;
    logSystemStatus();
  }

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
  Serial.print(door_getStateName());
  Serial.print(" (I=");
  Serial.print(door_getCurrentMA());
  Serial.print("mA) | Window: ");
  Serial.print(window_getStateName());
  Serial.print(" (I=");
  Serial.print(window_getCurrentMA());
  Serial.println("mA)");
}
