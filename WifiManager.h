#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>
#include "Globals.h"

// ============================================================================
// WiFi STATE ENUM
// ============================================================================

enum class WiFiState : uint8_t {
  DISCONNECTED = 0,   // Not connected
  CONNECTING = 1,     // Attempting to connect
  CONNECTED = 2,      // Connected to WiFi
  ERROR = 3           // Connection error
};

// ============================================================================
// WiFi CONFIGURATION
// ============================================================================

struct WiFiConfig {
  char ssid[32] = "";
  char password[64] = "";
  uint8_t maxRetries = 10;
  uint32_t retryDelayMs = 5000;
  bool autoConnect = true;
  uint32_t connectionTimeoutMs = 30000;  // 30 seconds
};

// ============================================================================
// WiFi DATA - RUNTIME STATE
// ============================================================================

struct WiFiData {
  WiFiState currentState = WiFiState::DISCONNECTED;
  bool isConnected = false;
  uint8_t retryCount = 0;
  unsigned long lastConnectionAttemptMs = 0;
  unsigned long connectionDurationMs = 0;
  char localIP[16] = "";
  int8_t signalStrength = 0;  // RSSI in dBm
};

// ============================================================================
// WiFi INSTANCE
// ============================================================================

extern WiFiData wifiData;

// ============================================================================
// WiFi CONTROL FUNCTIONS
// ============================================================================

/**
 * Initialize WiFi system
 */
void wifi_init();

/**
 * Update WiFi state machine
 * Should be called in main loop
 */
void wifi_update();

/**
 * Connect to WiFi network
 * @param ssid - network SSID
 * @param password - network password
 * @return - true if connection initiated
 */
bool wifi_connect(const char* ssid, const char* password);

/**
 * Disconnect from WiFi
 */
void wifi_disconnect();

/**
 * Get current WiFi state
 */
WiFiState wifi_getState();

/**
 * Check if WiFi is connected
 */
bool wifi_isConnected();

/**
 * Get WiFi configuration
 */
WiFiConfig* wifi_getConfig();

/**
 * Get WiFi data
 */
WiFiData* wifi_getData();

/**
 * Get signal strength in dBm
 */
int8_t wifi_getSignalStrength();

/**
 * Get local IP address
 */
const char* wifi_getLocalIP();

/**
 * Get human-readable state name
 */
const char* wifi_getStateName(WiFiState state);

/**
 * Synchronize time via NTP over WiFi
 * Fills provided TimeData struct and calls rtc_syncFromNTP()
 * @return true if sync succeeded
 */
bool wifi_ntpSync();

#endif // WIFIMANAGER_H
