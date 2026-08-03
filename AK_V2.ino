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
#include "Climate.h"
#include "Heater.h"
#include "Light.h"
#include "Alarm.h"
#include "Logger.h"
#include "OTA.h"
#include "WebServer.h"
#include "Ethernet.h"
#include "WiFi.h"
#include <cstring>

// Compatibility declarations for Arduino builds where local headers differ
extern Motor doorMotor;
extern Motor windowMotor;

void wifi_init();
void wifi_update();
bool wifi_connect(const char* ssid, const char* password);
bool wifi_isConnected();
const char* wifi_getLocalIP();

#ifndef NETWORK_USE_ETHERNET_PRIMARY
constexpr bool NETWORK_USE_ETHERNET_PRIMARY = true;
#endif

#ifndef NETWORK_USE_WIFI_FALLBACK
constexpr bool NETWORK_USE_WIFI_FALLBACK = true;
#endif

#ifndef WIFI_DEBUG_SSID
constexpr const char* WIFI_DEBUG_SSID = "";
#endif

#ifndef WIFI_DEBUG_PASSWORD
constexpr const char* WIFI_DEBUG_PASSWORD = "";
#endif

#ifndef ETHERNET_WIFI_FALLBACK_DELAY_MS
constexpr uint32_t ETHERNET_WIFI_FALLBACK_DELAY_MS = 10000;
#endif

// ============================================================================
// MOTOR INSTANCES
// ============================================================================

static unsigned long networkInitTimeMs = 0;
static bool webServerActive = false;

static void tryWiFiFallback() {
  if (!NETWORK_USE_WIFI_FALLBACK) {
    return;
  }

  if (strlen(WIFI_DEBUG_SSID) == 0) {
    return;
  }

  if (!wifi_isConnected()) {
    wifi_connect(WIFI_DEBUG_SSID, WIFI_DEBUG_PASSWORD);
    Serial.println("[NET] WiFi fallback connect requested");
  }
}

static bool networkHasConnectivity() {
  return wifi_isConnected() || (NETWORK_USE_ETHERNET_PRIMARY && ethernet_isConnected());
}

static void updateNetworkServices() {
  if (networkHasConnectivity()) {
    if (!webServerActive && webserver_start()) {
      webServerActive = true;
      Serial.println("[NET] Web server started");
    }
    return;
  }

  if (webServerActive) {
    webserver_stop();
    webServerActive = false;
    Serial.println("[NET] Web server stopped");
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

  // Initialize logging
  logger_init();
  Serial.println("[INIT] Logger initialized");

  // Initialize hardware (GPIO, PWM, etc.)
  hardware_init();
  Serial.println("[INIT] Hardware initialized");

  // Initialize settings system
  settings_init();
  Serial.println("[INIT] Settings loaded");

  // Initialize sensor layer
  sensors_init();
  Serial.println("[INIT] Sensors initialized");

  // Initialize timekeeping and automation support modules
  rtc_init();
  Serial.println("[INIT] RTC initialized");

  // Initialize networking (Ethernet primary, WiFi fallback for debugging)
  webserver_init();
  Serial.println("[INIT] Web server initialized");

  ota_init();
  Serial.println("[INIT] OTA initialized");

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

  alarm_init();
  Serial.println("[INIT] Alarm system initialized");

  heater_init();
  Serial.println("[INIT] Heater initialized");

  light_init();
  Serial.println("[INIT] Light initialized");

  climate_init();
  Serial.println("[INIT] Climate automation initialized");

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

  // Update timekeeping
  rtc_update();

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

  updateNetworkServices();
  webserver_update();
  ota_update();

  // Update automation/support modules
  alarm_update();
  heater_update();
  light_update();
  climate_update();

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
  Serial.print("mA) | Temp: ");

  if (coopEnvironment.isValid) {
    Serial.print(coopEnvironment.temperatureC, 1);
    Serial.print("C");
  } else {
    Serial.print("N/A");
  }

  Serial.print(" | Net: ");
  if (NETWORK_USE_ETHERNET_PRIMARY && ethernet_isConnected()) {
    Serial.print("ETH ");
    Serial.print(ethernet_getLocalIP());
  } else if (wifi_isConnected()) {
    Serial.print("WIFI ");
    Serial.print(wifi_getLocalIP());
  } else {
    Serial.print("OFFLINE");
  }

  Serial.print(" | Climate: ");
  Serial.print(climate_getModeName(climate_getMode()));
  Serial.print(" | Alarms: ");
  Serial.println(alarm_getActiveCount());
}
