#include "WiFi.h"
#include <WiFi.h>

// ============================================================================
// WiFi INSTANCE - GLOBAL DATA
// ============================================================================

WiFiData wifiData;
static WiFiConfig wifiConfig;

// ============================================================================
// INITIALIZATION
// ============================================================================

void wifi_init() {
  wifiData.currentState = WiFiState::DISCONNECTED;
  wifiData.isConnected = false;
  wifiData.retryCount = 0;
  wifiData.lastConnectionAttemptMs = 0;
  wifiData.connectionDurationMs = 0;
  strcpy(wifiData.localIP, "0.0.0.0");
  wifiData.signalStrength = 0;

  // Initialize WiFi config with defaults
  strcpy(wifiConfig.ssid, "");
  strcpy(wifiConfig.password, "");
  wifiConfig.maxRetries = 10;
  wifiConfig.retryDelayMs = 5000;
  wifiConfig.autoConnect = true;
  wifiConfig.connectionTimeoutMs = 30000;

  // Set WiFi mode to Station
  WiFi.mode(WIFI_STA);
}

// ============================================================================
// WiFi CONTROL
// ============================================================================

bool wifi_connect(const char* ssid, const char* password) {
  if (ssid == nullptr || password == nullptr) {
    return false;
  }

  strncpy(wifiConfig.ssid, ssid, sizeof(wifiConfig.ssid) - 1);
  strncpy(wifiConfig.password, password, sizeof(wifiConfig.password) - 1);

  wifiData.currentState = WiFiState::CONNECTING;
  wifiData.retryCount = 0;
  wifiData.lastConnectionAttemptMs = millis();

  // Initiate WiFi connection
  WiFi.begin(ssid, password);

  return true;
}

void wifi_disconnect() {
  WiFi.disconnect(true); // Turn off WiFi radio
  wifiData.currentState = WiFiState::DISCONNECTED;
  wifiData.isConnected = false;
  strcpy(wifiData.localIP, "0.0.0.0");
}

// ============================================================================
// STATE MACHINE UPDATE
// ============================================================================

void wifi_update() {
  unsigned long currentTime = millis();

  switch (wifiData.currentState) {
    // ========================================================================
    case WiFiState::DISCONNECTED: {
      // Check if we should attempt to reconnect
      if (wifiConfig.autoConnect && strlen(wifiConfig.ssid) > 0) {
        unsigned long timeSinceLastAttempt = currentTime - wifiData.lastConnectionAttemptMs;
        if (timeSinceLastAttempt > wifiConfig.retryDelayMs) {
          wifi_connect(wifiConfig.ssid, wifiConfig.password);
        }
      }
      break;
    }

    // ========================================================================
    case WiFiState::CONNECTING: {
      wl_status_t status = WiFi.status();

      if (status == WL_CONNECTED) {
        // Successfully connected
        wifiData.currentState = WiFiState::CONNECTED;
        wifiData.isConnected = true;
        wifiData.retryCount = 0;

        // Get and store local IP
        IPAddress ip = WiFi.localIP();
        snprintf(wifiData.localIP, sizeof(wifiData.localIP), "%d.%d.%d.%d",
                 ip[0], ip[1], ip[2], ip[3]);

        Serial.print("WiFi connected! IP: ");
        Serial.println(wifiData.localIP);
      } else if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL) {
        // Connection failed
        wifiData.retryCount++;
        if (wifiData.retryCount >= wifiConfig.maxRetries) {
          wifiData.currentState = WiFiState::ERROR;
          wifiData.isConnected = false;
        } else {
          wifiData.currentState = WiFiState::DISCONNECTED;
          wifiData.lastConnectionAttemptMs = currentTime;
        }
      } else if ((currentTime - wifiData.lastConnectionAttemptMs) > wifiConfig.connectionTimeoutMs) {
        // Connection timeout
        wifiData.currentState = WiFiState::ERROR;
        wifiData.isConnected = false;
      }
      break;
    }

    // ========================================================================
    case WiFiState::CONNECTED: {
      wl_status_t status = WiFi.status();

      if (status == WL_CONNECTED) {
        // Still connected - update signal strength and duration
        wifiData.signalStrength = WiFi.RSSI();
        wifiData.connectionDurationMs = currentTime - wifiData.lastConnectionAttemptMs;
      } else {
        // Connection lost
        wifiData.currentState = WiFiState::DISCONNECTED;
        wifiData.isConnected = false;
        wifiData.retryCount = 0;
        wifiData.lastConnectionAttemptMs = currentTime;
      }
      break;
    }

    // ========================================================================
    case WiFiState::ERROR: {
      // Wait before retrying
      unsigned long timeSinceError = currentTime - wifiData.lastConnectionAttemptMs;
      if (timeSinceError > (wifiConfig.retryDelayMs * 2)) {
        wifiData.currentState = WiFiState::DISCONNECTED;
        wifiData.retryCount = 0;
        wifiData.lastConnectionAttemptMs = currentTime;
      }
      break;
    }
  }
}

// ============================================================================
// PUBLIC API
// ============================================================================

WiFiState wifi_getState() {
  return wifiData.currentState;
}

bool wifi_isConnected() {
  return wifiData.isConnected;
}

WiFiConfig* wifi_getConfig() {
  return &wifiConfig;
}

WiFiData* wifi_getData() {
  return &wifiData;
}

int8_t wifi_getSignalStrength() {
  return wifiData.signalStrength;
}

const char* wifi_getLocalIP() {
  return wifiData.localIP;
}

const char* wifi_getStateName(WiFiState state) {
  switch (state) {
    case WiFiState::DISCONNECTED: return "DISCONNECTED";
    case WiFiState::CONNECTING:   return "CONNECTING";
    case WiFiState::CONNECTED:    return "CONNECTED";
    case WiFiState::ERROR:        return "ERROR";
    default:                      return "UNKNOWN";
  }
}
