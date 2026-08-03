#ifndef ETHERNET_MANAGER_H
#define ETHERNET_MANAGER_H

#include <Arduino.h>
#include "Globals.h"

enum class EthernetState : uint8_t {
  DISCONNECTED = 0,
  CONNECTING = 1,
  CONNECTED = 2,
  ERROR = 3
};

struct EthernetConfig {
  bool autoConnect = true;
  uint8_t maxRetries = 5;
  uint32_t retryDelayMs = 5000;
};

struct EthernetData {
  EthernetState currentState = EthernetState::DISCONNECTED;
  bool isConnected = false;
  bool linkUp = false;
  uint8_t retryCount = 0;
  unsigned long lastConnectionAttemptMs = 0;
  char localIP[16] = "";
};

extern EthernetData ethernetData;

void ethernet_init();
bool ethernet_connect();
void ethernet_disconnect();
void ethernet_update();
EthernetState ethernet_getState();
bool ethernet_isConnected();
bool ethernet_isLinkUp();
const char* ethernet_getLocalIP();
EthernetConfig* ethernet_getConfig();
EthernetData* ethernet_getData();
const char* ethernet_getStateName(EthernetState state);

#endif // ETHERNET_MANAGER_H
