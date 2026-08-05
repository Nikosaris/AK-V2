#include "WifiManager.h"
#include "RTC.h"
#include <WiFi.h>
#include <WiFiUdp.h>

// ============================================================================
// WiFi INSTANCE - GLOBAL DATA
// ============================================================================

WiFiData wifiData;
static WiFiConfig wifiConfig;

// ============================================================================
// NTP IMPLEMENTATION (lightweight, no external library)
// ============================================================================

static const uint16_t NTP_PORT         = 123;
static const uint16_t NTP_LOCAL_PORT   = 2390; // Ephemeral local UDP port for NTP client
static const uint16_t NTP_PACKET_SIZE  = 48;
static const unsigned long NTP_TIMEOUT_MS = 5000;
// NTP epoch starts 1900-01-01; Unix epoch starts 1970-01-01.
static const unsigned long NTP_TO_UNIX_OFFSET = 2208988800UL;

static WiFiUDP ntpUdp;
static uint8_t ntpPacketBuf[NTP_PACKET_SIZE];

static bool ntp_requestTime(const char* server) {
  ntpUdp.begin(NTP_LOCAL_PORT);
  memset(ntpPacketBuf, 0, NTP_PACKET_SIZE);

  // Build NTP request (LI=0, VN=3, Mode=3 = client)
  ntpPacketBuf[0] = 0b00011011;

  ntpUdp.beginPacket(server, NTP_PORT);
  ntpUdp.write(ntpPacketBuf, NTP_PACKET_SIZE);
  ntpUdp.endPacket();

  // Wait for response
  unsigned long start = millis();
  while (millis() - start < NTP_TIMEOUT_MS) {
    int size = ntpUdp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      ntpUdp.read(ntpPacketBuf, NTP_PACKET_SIZE);
      ntpUdp.stop();

      // Extract transmit timestamp (bytes 40-43)
      unsigned long secsSince1900 =
          ((unsigned long)ntpPacketBuf[40] << 24) |
          ((unsigned long)ntpPacketBuf[41] << 16) |
          ((unsigned long)ntpPacketBuf[42] <<  8) |
          ((unsigned long)ntpPacketBuf[43]);

      if (secsSince1900 == 0) return false;

      unsigned long unixTime = secsSince1900 - NTP_TO_UNIX_OFFSET;
      bool ok = rtc_syncFromNTP(unixTime, wifiConfig.timezoneOffsetSeconds);
      if (ok) {
        Serial.print("[WiFi-NTP] Time synchronised: ");
        TimeData* t = rtc_getTime();
        Serial.printf("%04d-%02d-%02d %02d:%02d:%02d\n",
                      2000 + t->year, t->month, t->day,
                      t->hour, t->minute, t->second);
      }
      return ok;
    }
    delay(10);
  }

  ntpUdp.stop();
  Serial.println("[WiFi-NTP] NTP request timed out");
  return false;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void wifi_init() {
  wifiData.currentState = WiFiState::DISCONNECTED;
  wifiData.isConnected = false;
  wifiData.retryCount = 0;
  wifiData.lastConnectionAttemptMs = 0;
  wifiData.connectionDurationMs = 0;
  wifiData.ntpSynced = false;
  wifiData.lastNtpSyncMs = 0;
  strcpy(wifiData.localIP, "0.0.0.0");
  wifiData.signalStrength = 0;

  // Initialize WiFi config with defaults
  strcpy(wifiConfig.ssid, "");
  strcpy(wifiConfig.password, "");
  wifiConfig.maxRetries = 10;
  wifiConfig.retryDelayMs = 5000;
  wifiConfig.autoConnect = true;
  wifiConfig.connectionTimeoutMs = 30000;
  strcpy(wifiConfig.ntpServer, "pool.ntp.org");
  wifiConfig.timezoneOffsetSeconds = 3600;
  wifiConfig.ntpSyncIntervalMs = 3600000;

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

  WiFi.begin(ssid, password);
  return true;
}

void wifi_disconnect() {
  WiFi.disconnect(true);
  wifiData.currentState = WiFiState::DISCONNECTED;
  wifiData.isConnected = false;
  strcpy(wifiData.localIP, "0.0.0.0");
}

// ============================================================================
// STATE MACHINE UPDATE
// ============================================================================

void wifi_update() {
  unsigned long currentMillis = millis();

  switch (wifiData.currentState) {
    // ========================================================================
    case WiFiState::DISCONNECTED: {
      if (wifiConfig.autoConnect && strlen(wifiConfig.ssid) > 0) {
        unsigned long timeSinceLastAttempt = currentMillis - wifiData.lastConnectionAttemptMs;
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
        wifiData.isConnected = true;
        wifiData.retryCount = 0;

        IPAddress ip = WiFi.localIP();
        snprintf(wifiData.localIP, sizeof(wifiData.localIP), "%d.%d.%d.%d",
                 ip[0], ip[1], ip[2], ip[3]);

        Serial.print("[WiFi] Connected! IP: ");
        Serial.println(wifiData.localIP);

        // Immediately attempt NTP sync
        wifiData.currentState = WiFiState::NTP_SYNC;

      } else if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL) {
        wifiData.retryCount++;
        if (wifiData.retryCount >= wifiConfig.maxRetries) {
          wifiData.currentState = WiFiState::ERROR;
          wifiData.isConnected = false;
        } else {
          wifiData.currentState = WiFiState::DISCONNECTED;
          wifiData.lastConnectionAttemptMs = currentMillis;
        }
      } else if ((currentMillis - wifiData.lastConnectionAttemptMs) > wifiConfig.connectionTimeoutMs) {
        wifiData.currentState = WiFiState::ERROR;
        wifiData.isConnected = false;
      }
      break;
    }

    // ========================================================================
    case WiFiState::NTP_SYNC: {
      // Attempt NTP; always proceed to CONNECTED afterwards so automation
      // is never blocked by an NTP failure.
      ntp_requestTime(wifiConfig.ntpServer);
      wifiData.ntpSynced = rtc_isValid();
      wifiData.lastNtpSyncMs = currentMillis;
      wifiData.currentState = WiFiState::CONNECTED;
      break;
    }

    // ========================================================================
    case WiFiState::CONNECTED: {
      wl_status_t status = WiFi.status();

      if (status == WL_CONNECTED) {
        wifiData.signalStrength = WiFi.RSSI();
        wifiData.connectionDurationMs = currentMillis - wifiData.lastConnectionAttemptMs;

        // Periodic NTP re-sync
        if (currentMillis - wifiData.lastNtpSyncMs >= wifiConfig.ntpSyncIntervalMs) {
          Serial.println("[WiFi] Periodic NTP re-sync...");
          ntp_requestTime(wifiConfig.ntpServer);
          wifiData.lastNtpSyncMs = currentMillis;
        }
      } else {
        wifiData.currentState = WiFiState::DISCONNECTED;
        wifiData.isConnected = false;
        wifiData.retryCount = 0;
        wifiData.lastConnectionAttemptMs = currentMillis;
        Serial.println("[WiFi] Connection lost — will retry");
      }
      break;
    }

    // ========================================================================
    case WiFiState::ERROR: {
      unsigned long timeSinceError = currentMillis - wifiData.lastConnectionAttemptMs;
      if (timeSinceError > (wifiConfig.retryDelayMs * 2)) {
        wifiData.currentState = WiFiState::DISCONNECTED;
        wifiData.retryCount = 0;
        wifiData.lastConnectionAttemptMs = currentMillis;
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
    case WiFiState::NTP_SYNC:     return "NTP_SYNC";
    case WiFiState::ERROR:        return "ERROR";
    default:                      return "UNKNOWN";
  }
}
