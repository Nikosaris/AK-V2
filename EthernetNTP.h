#ifndef ETHERNETNTP_H
#define ETHERNETNTP_H

#include <Arduino.h>
#include "Globals.h"
#include "RTC.h"

// ============================================================================
// ETHERNET STATE
// ============================================================================

enum class EthernetState : uint8_t {
  DISABLED    = 0,
  INITIALIZING = 1,
  CONNECTING  = 2,
  CONNECTED   = 3,
  DISCONNECTED = 4,
  ERROR       = 5
};

// ============================================================================
// ETHERNET CONFIG
// ============================================================================

struct EthernetConfig {
  uint8_t  csPin;
  uint8_t  resetPin;
  uint8_t  mac[6];
  uint32_t ntpSyncIntervalMs;
  uint32_t ntpTimeoutMs;
  char     ntpServer[32];
  char     ntpFallback[32];
};

// ============================================================================
// ETHERNET RUNTIME DATA
// ============================================================================

struct EthernetData {
  EthernetState state;
  bool          isConnected;
  char          localIP[16];
  char          gatewayIP[16];
  char          subnetMask[16];
  unsigned long lastNTPSyncMs;
  unsigned long lastConnectedMs;
  bool          ntpSynced;
};

// ============================================================================
// ETHERNET NTP FUNCTIONS
// ============================================================================

void ethernet_init();
void ethernet_update();
bool ethernet_connect();
void ethernet_disconnect();
bool ethernet_isConnected();

bool ethernet_ntpSync();
bool ethernet_ntpSyncWithFallback();

EthernetState ethernet_getState();
const char*   ethernet_getStateName(EthernetState state);
EthernetData* ethernet_getData();

#endif // ETHERNETNTP_H
