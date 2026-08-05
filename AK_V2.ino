// ============================================================================
// AK-V2: Professional Chicken Coop Automation Firmware
// Platform: ESP32-WROOM-32
// ============================================================================

#include "Globals.h"
#include "Hardware.h"
#include "Motor.h"
#include "Settings.h"
#include "Sensors.h"
#include "RTC.h"
#include "EthernetNTP.h"
#include "WifiManager.h"
#include "Climate.h"
#include "Heater.h"
#include "Light.h"
#include "Alarm.h"
#include "Logger.h"
#include "OTA.h"
#include "WebServer.h"

// ============================================================================
// MOTOR INSTANCES
// ============================================================================

Motor doorMotor;
Motor windowMotor;

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(100);

  Serial.println("\n\n=================================================================================");
  Serial.println("AK-V2: Professional Chicken Coop Automation Firmware");
  Serial.println("Platform: ESP32-WROOM-32");
  Serial.println("Version: 0.1.0");
  Serial.println("=================================================================================");

  // Core system
  globals_init();
  Serial.println("[INIT] Globals initialized");

  hardware_init();
  Serial.println("[INIT] Hardware initialized");

  settings_init();
  Serial.println("[INIT] Settings loaded");

  sensors_init();
  Serial.println("[INIT] Sensors initialized");

  // Ethernet (highest priority NTP source)
  Serial.println("\n[INIT] === Network initialization ===");
  ethernet_init();
  Serial.println("[INIT] Ethernet W5500 initialized (RESET: GPIO2)");

  // RTC (DS3231 + EEPROM fallback)
  Serial.println("[INIT] === Time initialization ===");
  rtc_init();
  Serial.println("[INIT] RTC initialized");

  // WiFi (secondary NTP fallback)
  wifi_init();
  wifi_connect("iPhone (2)", "12345678");
  Serial.println("[INIT] WiFi initialized (NTP fallback)");
  // Wait briefly for WiFi to connect, then sync NTP
  {
    unsigned long wifiWaitStart = millis();
    Serial.print("[INIT] Cekam na WiFi pripojeni...");
    while (!wifi_isConnected() && millis() - wifiWaitStart < 15000) {
      wifi_update();
      delay(200);
      Serial.print(".");
    }
    Serial.println();
    if (wifi_isConnected()) {

    } else {
      Serial.println("[INIT] WiFi nepripojeno - NTP sync preskocen");
    }
  }

  // Motors
  motor_init(&doorMotor, "Door", PWM_CHANNEL_DOOR_IN1, PWM_CHANNEL_DOOR_IN2);
  doorMotor.config = *settings_getDoorConfig();
  Serial.println("[INIT] Door motor initialized");

  motor_init(&windowMotor, "Window", PWM_CHANNEL_WINDOW_IN1, PWM_CHANNEL_WINDOW_IN2);
  windowMotor.config = *settings_getWindowConfig();
  Serial.println("[INIT] Window motor initialized");

  // Automation modules
  climate_init();
  Serial.println("[INIT] Climate initialized");

  heater_init();
  Serial.println("[INIT] Heater initialized");

  light_init();
  Serial.println("[INIT] Light initialized");

  alarm_init();
  Serial.println("[INIT] Alarm initialized");

  // Services
  logger_init();
  Serial.println("[INIT] Logger initialized");

  ota_init();
  Serial.println("[INIT] OTA initialized");

  webserver_init();
  webserver_start();
  Serial.println("[INIT] WebServer initialized");

  Serial.println("\n[INIT] System startup complete. Ready to operate.\n");
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
  unsigned long loopStartTime = millis();

  globals_update();
  hardware_update();
  sensors_update();

  // Network + Time (priority order)
  ethernet_update();
  rtc_update();
  wifi_update();

  // Motor state machines
  motor_update(&doorMotor);
  motor_update(&windowMotor);

  // Automation modules
  climate_update();
  heater_update();
  light_update();
  alarm_update();

  // Services
  ota_update();
  webserver_update();

  unsigned long now = millis();

  // EEPROM backup every minute
  static unsigned long lastEEPROMSaveMs = 0;
  if (now - lastEEPROMSaveMs >= 60000) {
    lastEEPROMSaveMs = now;
    rtc_saveToEEPROM();
  }

  // NTP fallback check every 24h
  static unsigned long lastNTPCheckMs = 0;
  if (now - lastNTPCheckMs >= NTP_SYNC_INTERVAL_MS) {
    lastNTPCheckMs = now;
    if (!ethernet_isConnected()) {
      if (wifi_isConnected()) {
        Serial.println("[RTC] Fallback to WiFi NTP...");

      } else {
        Serial.println("[RTC] ⚠️  All networks offline - running on DS3231");
      }
    }
  }

  // Serial log every 1 second
  if (now - lastSerialLogTime >= SERIAL_LOG_INTERVAL_MS) {
    lastSerialLogTime = now;
    logSystemStatus();
  }

  // Frame rate ~20 Hz
  unsigned long loopDuration = millis() - loopStartTime;
  if (loopDuration < LOOP_INTERVAL_MS) {
    delayMicroseconds((LOOP_INTERVAL_MS - loopDuration) * 1000);
  }
}

// ============================================================================
// LOGGING
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
