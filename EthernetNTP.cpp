#include "EthernetNTP.h"
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

// ============================================================================
// PRIVATE DATA
// ============================================================================

static EthernetConfig ethConfig;
static EthernetData   ethData;
static EthernetUDP    ntpUDP;

static const uint32_t NTP_EPOCH_OFFSET = 2208988800UL;  // seconds 1900→1970
static const uint32_t UNIX_OFFSET_2000 = 946684800UL;   // seconds 1970→2000

// ============================================================================
// NTP HELPERS
// ============================================================================

static void buildNTPPacket(uint8_t* buf) {
  memset(buf, 0, NTP_PACKET_SIZE);
  buf[0] = 0b11100011;  // LI=3, Version=4, Mode=3 (client)
  buf[1] = 0;           // Stratum
  buf[2] = 6;           // Polling interval
  buf[3] = 0xEC;        // Precision
  // Root delay and dispersion: 8 bytes zeros
  buf[12] = 49;
  buf[13] = 0x4E;
  buf[14] = 49;
  buf[15] = 52;
}

static bool sendNTPRequest(const char* server) {
  uint8_t buf[NTP_PACKET_SIZE];
  buildNTPPacket(buf);

  ntpUDP.beginPacket(server, NTP_PORT);
  ntpUDP.write(buf, NTP_PACKET_SIZE);
  return ntpUDP.endPacket() == 1;
}

static bool receiveNTPResponse(TimeData* result) {
  uint8_t buf[NTP_PACKET_SIZE];
  unsigned long startMs = millis();

  while (millis() - startMs < ethConfig.ntpTimeoutMs) {
    if (ntpUDP.parsePacket() >= NTP_PACKET_SIZE) {
      ntpUDP.read(buf, NTP_PACKET_SIZE);

      // Extract transmit timestamp (bytes 40-43)
      uint32_t secsSince1900 = ((uint32_t)buf[40] << 24)
                             | ((uint32_t)buf[41] << 16)
                             | ((uint32_t)buf[42] << 8)
                             |  (uint32_t)buf[43];

      if (secsSince1900 == 0) return false;

      uint32_t unixTime    = secsSince1900 - NTP_EPOCH_OFFSET;
      uint32_t secsSince2k = unixTime - UNIX_OFFSET_2000;

      // Convert to TimeData
      uint32_t remaining = secsSince2k;

      uint32_t daysSince2k = remaining / 86400;
      remaining %= 86400;

      result->hour   = remaining / 3600;
      remaining     %= 3600;
      result->minute = remaining / 60;
      result->second = remaining % 60;

      // Year calculation (approx, ignores leap years for simplicity)
      uint32_t year = 0;
      while (daysSince2k >= 365) {
        bool leap = ((2000 + year) % 4 == 0 &&
                    ((2000 + year) % 100 != 0 || (2000 + year) % 400 == 0));
        uint32_t daysInYear = leap ? 366 : 365;
        if (daysSince2k < daysInYear) break;
        daysSince2k -= daysInYear;
        year++;
      }
      result->year = (uint8_t)year;

      // Month calculation
      static const uint8_t daysInMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};
      uint8_t month = 0;
      while (month < 12) {
        uint8_t dim = daysInMonth[month];
        bool leap = ((2000 + year) % 4 == 0 &&
                    ((2000 + year) % 100 != 0 || (2000 + year) % 400 == 0));
        if (month == 1 && leap) dim = 29;
        if (daysSince2k < dim) break;
        daysSince2k -= dim;
        month++;
      }
      result->month     = month + 1;
      result->day       = (uint8_t)(daysSince2k + 1);
      result->dayOfWeek = 0;  // Not critical for operation

      return true;
    }
    delay(10);
  }
  return false;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void ethernet_init() {
  Serial.println("[ETH] === Inicializace W5500 ===");

  // Config defaults
  ethConfig.csPin              = ETHERNET_CS_PIN;
  ethConfig.resetPin           = ETHERNET_RESET_PIN;
  ethConfig.ntpSyncIntervalMs  = NTP_SYNC_INTERVAL_MS;
  ethConfig.ntpTimeoutMs       = NTP_TIMEOUT_MS;
  strncpy(ethConfig.ntpServer,   "pool.ntp.org",   sizeof(ethConfig.ntpServer) - 1);
  strncpy(ethConfig.ntpFallback, "time.nist.gov",  sizeof(ethConfig.ntpFallback) - 1);
  // Random MAC based on fixed OUI (locally administered)
  ethConfig.mac[0] = 0xDE;
  ethConfig.mac[1] = 0xAD;
  ethConfig.mac[2] = 0xBE;
  ethConfig.mac[3] = 0xEF;
  ethConfig.mac[4] = 0xFE;
  ethConfig.mac[5] = 0x01;

  // Data defaults
  ethData.state          = EthernetState::ETH_INITIALIZING;
  ethData.isConnected    = false;
  ethData.ntpSynced      = false;
  ethData.lastNTPSyncMs  = 0;
  ethData.lastConnectedMs = 0;
  strcpy(ethData.localIP,    "0.0.0.0");
  strcpy(ethData.gatewayIP,  "0.0.0.0");
  strcpy(ethData.subnetMask, "0.0.0.0");

  // Hardware reset W5500
  pinMode(ethConfig.resetPin, OUTPUT);
  digitalWrite(ethConfig.resetPin, LOW);
  delay(20);
  digitalWrite(ethConfig.resetPin, HIGH);
  delay(150);

  // SPI init
  SPI.begin(ETHERNET_SPI_CLK, ETHERNET_SPI_MISO, ETHERNET_SPI_MOSI, ETHERNET_CS_PIN);
  Serial.print("[ETH] \xe2\x9c\x93 SPI initialized (RESET: GPIO");
  Serial.print(ethConfig.resetPin);
  Serial.println(")");

  Ethernet.init(ethConfig.csPin);

  // Attempt DHCP connect
  ethernet_connect();
}

bool ethernet_connect() {
  Serial.println("[ETH] Connecting via DHCP...");
  if (Ethernet.begin(ethConfig.mac, 10000, 4000) == 0) {
    // Check hardware
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("[ETH] W5500 not found");
      ethData.state       = EthernetState::ETH_ERROR;
      ethData.isConnected = false;
      return false;
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("[ETH] Ethernet cable not connected");
      ethData.state       = EthernetState::ETH_DISCONNECTED;
      ethData.isConnected = false;
      return false;
    }
    Serial.println("[ETH] DHCP failed");
    ethData.state       = EthernetState::ETH_ERROR;
    ethData.isConnected = false;
    return false;
  }

  ethData.isConnected    = true;
  ethData.state          = EthernetState::ETH_CONNECTED;
  ethData.lastConnectedMs = millis();

  IPAddress ip  = Ethernet.localIP();
  IPAddress gw  = Ethernet.gatewayIP();
  IPAddress sub = Ethernet.subnetMask();
  snprintf(ethData.localIP,    sizeof(ethData.localIP),    "%d.%d.%d.%d", ip[0],  ip[1],  ip[2],  ip[3]);
  snprintf(ethData.gatewayIP,  sizeof(ethData.gatewayIP),  "%d.%d.%d.%d", gw[0],  gw[1],  gw[2],  gw[3]);
  snprintf(ethData.subnetMask, sizeof(ethData.subnetMask), "%d.%d.%d.%d", sub[0], sub[1], sub[2], sub[3]);

  Serial.print("[ETH] \xe2\x9c\x93 Ethernet connected\n[ETH] DHCP: ");
  Serial.println(ethData.localIP);
  Serial.print("[ETH] Gateway: ");
  Serial.println(ethData.gatewayIP);
  Serial.print("[ETH] Subnet: ");
  Serial.println(ethData.subnetMask);

  // Immediate NTP sync on first connect
  ethernet_ntpSyncWithFallback();
  return true;
}

void ethernet_disconnect() {
  Ethernet.maintain();
  ethData.isConnected = false;
  ethData.state       = EthernetState::ETH_DISCONNECTED;
}

// ============================================================================
// NTP SYNC
// ============================================================================

bool ethernet_ntpSync() {
  if (!ethData.isConnected) return false;

  Serial.println("[ETH] === NTP Synchronizace ===");
  Serial.print("[ETH] NTP Server: ");
  Serial.println(ethConfig.ntpServer);

  ntpUDP.begin(NTP_PORT);

  if (!sendNTPRequest(ethConfig.ntpServer)) {
    ntpUDP.stop();
    Serial.println("[ETH] NTP request failed");
    return false;
  }

  TimeData ntpTime;
  if (!receiveNTPResponse(&ntpTime)) {
    ntpUDP.stop();
    Serial.println("[ETH] NTP timeout");
    return false;
  }

  ntpUDP.stop();

  char buf[48];
  snprintf(buf, sizeof(buf), "[ETH] NTP time: 20%02d-%02d-%02d %02d:%02d:%02d",
           ntpTime.year, ntpTime.month, ntpTime.day,
           ntpTime.hour, ntpTime.minute, ntpTime.second);
  Serial.println(buf);

  rtc_syncFromNTP(&ntpTime, true);

  ethData.ntpSynced     = true;
  ethData.lastNTPSyncMs = millis();
  return true;
}

bool ethernet_ntpSyncWithFallback() {
  if (ethernet_ntpSync()) return true;

  // Try fallback server
  Serial.print("[ETH] Trying fallback NTP: ");
  Serial.println(ethConfig.ntpFallback);

  ntpUDP.begin(NTP_PORT);
  if (!sendNTPRequest(ethConfig.ntpFallback)) {
    ntpUDP.stop();
    return false;
  }

  TimeData ntpTime;
  if (!receiveNTPResponse(&ntpTime)) {
    ntpUDP.stop();
    Serial.println("[ETH] Fallback NTP also failed");
    return false;
  }

  ntpUDP.stop();
  rtc_syncFromNTP(&ntpTime, true);
  ethData.ntpSynced     = true;
  ethData.lastNTPSyncMs = millis();
  return true;
}

// ============================================================================
// UPDATE
// ============================================================================

void ethernet_update() {
  // Maintain DHCP lease
  if (ethData.isConnected) {
    Ethernet.maintain();

    // Check physical link
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("[ETH] Ethernet is offline");
      ethData.isConnected = false;
      ethData.state       = EthernetState::ETH_DISCONNECTED;
      return;
    }

    // Periodic NTP sync
    unsigned long now = millis();
    if (ethData.ntpSynced && (now - ethData.lastNTPSyncMs >= ethConfig.ntpSyncIntervalMs)) {
      Serial.println("[ETH] === NTP Synchronizace (periodic) ===");
      ethernet_ntpSyncWithFallback();
    }
  } else {
    // Attempt reconnect every 30 seconds
    static unsigned long lastReconnectMs = 0;
    unsigned long now = millis();
    if (now - lastReconnectMs >= 30000) {
      lastReconnectMs = now;
      ethernet_connect();
    }
  }
}

// ============================================================================
// PUBLIC API
// ============================================================================

bool ethernet_isConnected() {
  return ethData.isConnected;
}

EthernetState ethernet_getState() {
  return ethData.state;
}

EthernetData* ethernet_getData() {
  return &ethData;
}

const char* ethernet_getStateName(EthernetState state) {
  switch (state) {
    case EthernetState::ETH_DISABLED:      return "DISABLED";
    case EthernetState::ETH_INITIALIZING:  return "INITIALIZING";
    case EthernetState::ETH_CONNECTING:    return "CONNECTING";
    case EthernetState::ETH_CONNECTED:     return "CONNECTED";
    case EthernetState::ETH_DISCONNECTED:  return "DISCONNECTED";
    case EthernetState::ETH_ERROR:         return "ERROR";
    default:                           return "UNKNOWN";
  }
}
