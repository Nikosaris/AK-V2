// ============================================================================
// AK-V2: Professional Chicken Coop Automation Firmware
// Platform: ESP32-WROOM-32
// ============================================================================

#include "Globals.h"
#include "Hardware.h"
#include "Motor.h"
#include "Settings.h"
#include "RTC.h"
#include "EthernetNTP.h"
#include "WiFi.h"

// ============================================================================
// MOTOR INSTANCES
// ============================================================================

static Motor doorMotor;
static Motor windowMotor;

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

  // ---- Ethernet init (highest priority NTP source) ----
  Serial.println("\n[INIT] === Network initialization ===");
  ethernet_init();
  Serial.println("[INIT] Ethernet W5500 initialized (RESET: GPIO2)");

  // ---- RTC init (DS3231 + EEPROM fallback) ----
  Serial.println("[INIT] === Time initialization ===");
  rtc_init();
  Serial.println("[INIT] RTC initialized");

  // ---- WiFi init (secondary NTP fallback) ----
  wifi_init();
  Serial.println("[INIT] WiFi initialized (NTP fallback)");

  // ---- Motor init ----
  motor_init(&doorMotor, "Door", PWM_CHANNEL_DOOR_IN1, PWM_CHANNEL_DOOR_IN2);
  doorMotor.config = *settings_getDoorConfig();
  Serial.println("[INIT] Door motor initialized");

  motor_init(&windowMotor, "Window", PWM_CHANNEL_WINDOW_IN1, PWM_CHANNEL_WINDOW_IN2);
  windowMotor.config = *settings_getWindowConfig();
  Serial.println("[INIT] Window motor initialized");

  Serial.println("\n[INIT] System startup complete. Ready to operate.\n");
}

// ============================================================================
// LOOP - Main control loop
// ============================================================================

void loop() {
  unsigned long loopStartTime = millis();

  globals_update();
  hardware_update();

  // ---- Network + Time updates (priority order) ----
  ethernet_update();
  rtc_update();
  wifi_update();

  // ---- Motor state machines ----
  motor_update(&doorMotor);
  motor_update(&windowMotor);

  // ---- EEPROM backup every minute ----
  static unsigned long lastEEPROMSaveMs = 0;
  unsigned long now = millis();
  if (now - lastEEPROMSaveMs >= 60000) {
    lastEEPROMSaveMs = now;
    rtc_saveToEEPROM();
  }

  // ---- Periodic NTP sync every 24h (if Ethernet offline, try WiFi) ----
  static unsigned long lastNTPCheckMs = 0;
  if (now - lastNTPCheckMs >= NTP_SYNC_INTERVAL_MS) {
    lastNTPCheckMs = now;
    if (!ethernet_isConnected()) {
      if (wifi_isConnected()) {
        Serial.println("[RTC] Fallback to WiFi NTP...");
        // WiFi NTP sync handled separately; DS3231 maintains time locally
      } else {
        Serial.println("[RTC] \xe2\x9a\xa0\xef\xb8\x8f  All networks offline - running on DS3231");
      }
    }
  }

  // ---- Periodic serial logging (every 1 second) ----
  if (now - lastSerialLogTime >= SERIAL_LOG_INTERVAL_MS) {
    lastSerialLogTime = now;
    logSystemStatus();
  }

  // ---- Frame rate limiting (~20 Hz) ----
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
  char timeBuf[20];
  snprintf(timeBuf, sizeof(timeBuf), "20%02d-%02d-%02d %02d:%02d:%02d",
           t->year, t->month, t->day, t->hour, t->minute, t->second);

  Serial.print("[LOG-");
  Serial.print(logCounter);
  Serial.print("] ");
  Serial.print(timeBuf);
  Serial.print(" [");
  Serial.print(rtc_getSourceName(rtc_getCurrentSource()));
  Serial.print("] | Door: ");
  Serial.print(motor_getStateName(doorMotor.data.state));
  Serial.print(" | Window: ");
  Serial.println(motor_getStateName(windowMotor.data.state));
}
