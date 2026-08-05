#include "EthernetNTP.h"
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

// ============================================================================
// ETHERNET W5500 CONFIGURATION
// ============================================================================

// MAC address for W5500 (locally administered, change if multiple devices)
static byte ethernetMAC[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// W5500 reset pin (GPIO2 as noted in INIT log)
static const uint8_t W5500_RESET_PIN = 2;

// NTP configuration
static const uint16_t NTP_PORT        = 123;
static const uint16_t NTP_LOCAL_PORT  = 2391;
static const uint16_t NTP_PACKET_SIZE = 48;
static const uint32_t NTP_TIMEOUT_MS  = 5000;
static const unsigned long NTP_TO_UNIX_OFFSET = 2208988800UL;

static const char* NTP_SERVER = "pool.ntp.org";

// ============================================================================
// RUNTIME STATE
// ============================================================================

static bool     ethernetConnected = false;
static char     localIPStr[16]    = "0.0.0.0";
static EthernetUDP ntpUdp;
static uint8_t  ntpBuf[NTP_PACKET_SIZE];

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

static bool ntp_sendRequest(const char* server) {
  memset(ntpBuf, 0, NTP_PACKET_SIZE);
  ntpBuf[0] = 0b00011011; // LI=0, VN=3, Mode=3 (client)

  ntpUdp.beginPacket(server, NTP_PORT);
  ntpUdp.write(ntpBuf, NTP_PACKET_SIZE);
  return (ntpUdp.endPacket() == 1);
}

static bool ntp_receiveAndSync() {
  unsigned long start = millis();
  while (millis() - start < NTP_TIMEOUT_MS) {
    int size = ntpUdp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      ntpUdp.read(ntpBuf, NTP_PACKET_SIZE);

      // Extract transmit timestamp (bytes 40-43)
      unsigned long secsSince1900 =
          ((unsigned long)ntpBuf[40] << 24) |
          ((unsigned long)ntpBuf[41] << 16) |
          ((unsigned long)ntpBuf[42] <<  8) |
          ((unsigned long)ntpBuf[43]);

      if (secsSince1900 == 0) return false;

      unsigned long unixTime = secsSince1900 - NTP_TO_UNIX_OFFSET;
      // Use unix-time overload (timezone offset = 0; handled by RTC layer)
      return rtc_syncFromNTP(unixTime, 3600); // UTC+1 default (CET)
    }
    delay(10);
  }
  Serial.println("[ETH-NTP] NTP response timed out");
  return false;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void ethernet_init() {
  // Hardware reset W5500
  pinMode(W5500_RESET_PIN, OUTPUT);
  digitalWrite(W5500_RESET_PIN, LOW);
  delay(10);
  digitalWrite(W5500_RESET_PIN, HIGH);
  delay(200);

  // Start Ethernet with DHCP
  if (Ethernet.begin(ethernetMAC) == 0) {
    Serial.println("[ETH] DHCP failed — Ethernet not available");
    ethernetConnected = false;
    return;
  }

  ethernetConnected = true;
  IPAddress ip = Ethernet.localIP();
  snprintf(localIPStr, sizeof(localIPStr), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  Serial.print("[ETH] Connected, IP: ");
  Serial.println(localIPStr);

  // Start UDP for NTP
  ntpUdp.begin(NTP_LOCAL_PORT);

  // Initial NTP sync
  ethernet_ntpSync();
}

// ============================================================================
// UPDATE (call every loop)
// ============================================================================

void ethernet_update() {
  if (!ethernetConnected) return;

  // Maintain DHCP lease
  Ethernet.maintain();
}

// ============================================================================
// NTP SYNC
// ============================================================================

bool ethernet_ntpSync() {
  if (!ethernetConnected) return false;

  if (!ntp_sendRequest(NTP_SERVER)) {
    Serial.println("[ETH-NTP] Failed to send NTP request");
    return false;
  }

  bool ok = ntp_receiveAndSync();
  if (ok) {
    Serial.println("[ETH-NTP] Time synchronised via Ethernet");
  }
  return ok;
}

bool ethernet_ntpSyncWithFallback() {
  const char* servers[] = { "pool.ntp.org", "time.google.com", "time.cloudflare.com" };
  for (uint8_t i = 0; i < 3; i++) {
    if (ntp_sendRequest(servers[i]) && ntp_receiveAndSync()) {
      Serial.print("[ETH-NTP] Synced from ");
      Serial.println(servers[i]);
      return true;
    }
  }
  Serial.println("[ETH-NTP] All NTP servers failed");
  return false;
}

// ============================================================================
// PUBLIC API
// ============================================================================

bool ethernet_isConnected() {
  return ethernetConnected;
}

const char* ethernet_getLocalIP() {
  return localIPStr;
}
