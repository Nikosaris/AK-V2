#include "Ethernet.h"
#include <ETH.h>
#include <SPI.h>

EthernetData ethernetData;
static EthernetConfig ethernetConfig;
static bool ethernetStarted = false;

void ethernet_init() {
  ethernetData.currentState = EthernetState::DISCONNECTED;
  ethernetData.isConnected = false;
  ethernetData.linkUp = false;
  ethernetData.retryCount = 0;
  ethernetData.lastConnectionAttemptMs = 0;
  strcpy(ethernetData.localIP, "0.0.0.0");

  ethernetConfig.autoConnect = true;
  ethernetConfig.maxRetries = 5;
  ethernetConfig.retryDelayMs = 5000;
}

bool ethernet_connect() {
  if (ethernetStarted) {
    return true;
  }

  SPI.begin(ETH_SCLK_PIN, ETH_MISO_PIN, ETH_MOSI_PIN, ETH_CS_PIN);
  ethernetData.currentState = EthernetState::CONNECTING;
  ethernetData.lastConnectionAttemptMs = millis();

  ethernetStarted = ETH.begin(ETH_PHY_W5500, ETH_PHY_ADDR, ETH_CS_PIN, ETH_IRQ_PIN, ETH_RST_PIN, SPI);
  if (!ethernetStarted) {
    ethernetData.currentState = EthernetState::ERROR;
    ethernetData.isConnected = false;
    return false;
  }

  return true;
}

void ethernet_disconnect() {
  ETH.end();
  ethernetStarted = false;
  ethernetData.currentState = EthernetState::DISCONNECTED;
  ethernetData.isConnected = false;
  ethernetData.linkUp = false;
  strcpy(ethernetData.localIP, "0.0.0.0");
}

void ethernet_update() {
  const unsigned long currentTime = millis();

  if (!ethernetStarted) {
    if (ethernetConfig.autoConnect &&
        (currentTime - ethernetData.lastConnectionAttemptMs) > ethernetConfig.retryDelayMs &&
        ethernetData.retryCount < ethernetConfig.maxRetries) {
      ethernetData.retryCount++;
      ethernet_connect();
    }
    return;
  }

  ethernetData.linkUp = ETH.linkUp();
  if (ethernetData.linkUp) {
    IPAddress ip = ETH.localIP();
    ethernetData.isConnected = (ip[0] != 0 || ip[1] != 0 || ip[2] != 0 || ip[3] != 0);
    if (ethernetData.isConnected) {
      ethernetData.currentState = EthernetState::CONNECTED;
      snprintf(ethernetData.localIP, sizeof(ethernetData.localIP), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    } else {
      ethernetData.currentState = EthernetState::CONNECTING;
      strcpy(ethernetData.localIP, "0.0.0.0");
    }
  } else {
    ethernetData.currentState = EthernetState::DISCONNECTED;
    ethernetData.isConnected = false;
    strcpy(ethernetData.localIP, "0.0.0.0");
  }
}

EthernetState ethernet_getState() {
  return ethernetData.currentState;
}

bool ethernet_isConnected() {
  return ethernetData.isConnected;
}

bool ethernet_isLinkUp() {
  return ethernetData.linkUp;
}

const char* ethernet_getLocalIP() {
  return ethernetData.localIP;
}

EthernetConfig* ethernet_getConfig() {
  return &ethernetConfig;
}

EthernetData* ethernet_getData() {
  return &ethernetData;
}

const char* ethernet_getStateName(EthernetState state) {
  switch (state) {
    case EthernetState::DISCONNECTED: return "DISCONNECTED";
    case EthernetState::CONNECTING:   return "CONNECTING";
    case EthernetState::CONNECTED:    return "CONNECTED";
    case EthernetState::ERROR:        return "ERROR";
    default:                          return "UNKNOWN";
  }
}
