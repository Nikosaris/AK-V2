// ============================================================================
// AK-V2: Professional Chicken Coop Automation Firmware
// Platform: ESP32-WROOM-32
// ============================================================================

#include "Globals.h"
#include "Hardware.h"
#include "Motor.h"
#include "Settings.h"
#include "RTC.h"
#include "WifiManager.h"
#include "Climate.h"

// ============================================================================
// MOTOR INSTANCES
// ============================================================================

static Motor doorMotor;
static Motor windowMotor;

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
  Serial.println("Version: 0.1.0");
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

  // Initialize RTC — DS3231 attempted first, falls back to millis()
  rtc_init();
  Serial.println("[INIT] RTC initialized");

  // Initialize motor structures
  motor_init(&doorMotor, "Door", PWM_CHANNEL_DOOR_IN1, PWM_CHANNEL_DOOR_IN2);
  doorMotor.config = *settings_getDoorConfig();
  Serial.println("[INIT] Door motor initialized");

  motor_init(&windowMotor, "Window", PWM_CHANNEL_WINDOW_IN1, PWM_CHANNEL_WINDOW_IN2);
  windowMotor.config = *settings_getWindowConfig();
  Serial.println("[INIT] Window motor initialized");

  // Initialize climate automation
  climate_init();
  Serial.println("[INIT] Climate automation initialized");

  // Initialize WiFi (non-blocking — NTP sync happens in the loop)
  wifi_init();
  // Provide credentials here or load from settings/EEPROM in production.
  // wifi_connect("YOUR_SSID", "YOUR_PASSWORD");
  Serial.println("[INIT] WiFi initialized");

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

  // Update RTC — advances software clock and re-reads DS3231 every second
  rtc_update();

  // Update WiFi — handles connect/reconnect and NTP sync (non-blocking)
  wifi_update();

  // Core motor control - STATE MACHINE UPDATES
  motor_update(&doorMotor);
  motor_update(&windowMotor);

  // Climate automation — always runs, regardless of network availability
  climate_update();

  // Periodic serial logging (every 1 second)
  unsigned long currentMillis = millis();
  if (currentMillis - lastSerialLogTime >= SERIAL_LOG_INTERVAL_MS) {
    lastSerialLogTime = currentMillis;
    logSystemStatus();
  }

  // Frame rate limiting — ensure loop runs at ~20 Hz (50ms)
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

  TimeData* t = rtc_getTime();

  Serial.print("[LOG-");
  Serial.print(logCounter);
  Serial.print("] Uptime: ");
  Serial.print(systemUptime / 1000);
  Serial.print("s | Time: ");
  Serial.printf("%04d-%02d-%02d %02d:%02d:%02d",
                2000 + t->year, t->month, t->day,
                t->hour, t->minute, t->second);
  Serial.print(" | RTC: ");
  switch (rtc_getSource()) {
    case RTCSource::DS3231: Serial.print("DS3231"); break;
    case RTCSource::NTP:    Serial.print("NTP");    break;
    default:                Serial.print("MILLIS"); break;
  }
  Serial.print(" | WiFi: ");
  Serial.print(wifi_getStateName(wifi_getState()));
  Serial.print(" | Door: ");
  Serial.print(motor_getStateName(doorMotor.data.state));
  Serial.print(" (I=");
  Serial.print(doorMotor.data.currentMA);
  Serial.print("mA) | Window: ");
  Serial.print(motor_getStateName(windowMotor.data.state));
  Serial.print(" (I=");
  Serial.print(windowMotor.data.currentMA);
  Serial.print("mA) | Climate: ");
  Serial.println(climate_getModeName(climate_getMode()));
}
