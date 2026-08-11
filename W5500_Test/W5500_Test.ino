// ============================================================================
// W5500_Test: Standalone test sketch for WIZnet W5500 Ethernet module
// Platform : ESP32-WROOM-32
// Build    : Arduino IDE  (install "Ethernet" library by Arduino, v2.x)
//
// SPI wiring (HSPI bus – does NOT conflict with AK-V2 main firmware pins)
// ┌──────────────┬───────────┬────────────────────────────────────────┐
// │ W5500 pin    │ ESP32 pin │ Note                                   │
// ├──────────────┼───────────┼────────────────────────────────────────┤
// │ MOSI         │ GPIO 13   │ HSPI MOSI                              │
// │ MISO         │ GPIO 12   │ HSPI MISO                              │
// │ SCLK         │ GPIO 2    │ HSPI SCK  (boot: keep LOW or floating) │
// │ SCS / CS     │ GPIO 15   │ Chip-select (active LOW)               │
// │ RESET        │ GPIO 0    │ Optional – tie HIGH if not used        │
// │ 3.3 V        │ 3V3       │                                        │
// │ GND          │ GND       │                                        │
// └──────────────┴───────────┴────────────────────────────────────────┘
//
// Fixed IP configuration – edit the constants below to suit your network.
// The sketch:
//   1. Initialises the W5500 over HSPI with a fixed IP address.
//   2. Opens a TCP server on port 80.
//   3. Replies to any incoming connection with a plain-text status page.
//   4. Prints diagnostics to the Serial Monitor (115200 baud) every second.
// ============================================================================

#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>

// ============================================================================
// USER-CONFIGURABLE NETWORK SETTINGS
// ============================================================================

// MAC address – must be unique on your LAN.
// W5500 modules often have a sticker with a MAC; use that if available.
static const uint8_t MAC_ADDR[6] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x01 };

static const IPAddress STATIC_IP      (192, 168, 1, 200);
static const IPAddress SUBNET_MASK    (255, 255, 255,   0);
static const IPAddress GATEWAY_IP     (192, 168, 1,   1);
static const IPAddress DNS_SERVER     (  8,   8,   8,   8);

constexpr uint16_t SERVER_PORT = 80;

// ============================================================================
// SPI / W5500 PIN DEFINITIONS  (HSPI bus)
// ============================================================================

constexpr uint8_t ETH_CS_PIN   = 15;   // Chip-select
constexpr uint8_t ETH_MOSI_PIN = 13;   // HSPI MOSI
constexpr uint8_t ETH_MISO_PIN = 12;   // HSPI MISO
constexpr uint8_t ETH_SCK_PIN  =  2;   // HSPI SCK
constexpr uint8_t ETH_RST_PIN  =  0;   // Hardware reset (optional)

constexpr uint32_t SPI_CLOCK_HZ = 20000000UL;  // 20 MHz – safe for W5500

// ============================================================================
// TIMING CONSTANTS
// ============================================================================

constexpr uint32_t SERIAL_BAUD_RATE       = 115200;
constexpr uint32_t SERIAL_LOG_INTERVAL_MS = 1000;
constexpr uint32_t LOOP_INTERVAL_MS       =   50;

// ============================================================================
// ETHERNET STATE MACHINE
// ============================================================================

enum class EthState : uint8_t {
  INIT        = 0,
  RUNNING     = 1,
  LINK_DOWN   = 2,
  ERROR       = 3
};

struct EthData {
  EthState state          = EthState::INIT;
  bool     linkUp         = false;
  uint32_t totalRequests  = 0;
  unsigned long initMs    = 0;
};

static EthData   ethData;
static SPIClass  hSPI(HSPI);           // Dedicated HSPI instance
static EthernetServer tcpServer(SERVER_PORT);

// ============================================================================
// HELPER: human-readable state name
// ============================================================================

static const char* eth_stateName(EthState s) {
  switch (s) {
    case EthState::INIT:      return "INIT";
    case EthState::RUNNING:   return "RUNNING";
    case EthState::LINK_DOWN: return "LINK_DOWN";
    case EthState::ERROR:     return "ERROR";
    default:                  return "UNKNOWN";
  }
}

// ============================================================================
// ETH_INIT
// ============================================================================

static void eth_init() {
  ethData.initMs = millis();

  // Optional hardware reset pulse
  if (ETH_RST_PIN != 0) {
    pinMode(ETH_RST_PIN, OUTPUT);
    digitalWrite(ETH_RST_PIN, LOW);
    delay(10);
    digitalWrite(ETH_RST_PIN, HIGH);
    delay(100);
  }

  // Start HSPI bus on custom pins
  hSPI.begin(ETH_SCK_PIN, ETH_MISO_PIN, ETH_MOSI_PIN, ETH_CS_PIN);
  hSPI.setFrequency(SPI_CLOCK_HZ);

  // Tell the Ethernet library which CS pin and SPI bus to use
  Ethernet.init(ETH_CS_PIN);

  // Begin with fixed IP (no DHCP)
  Ethernet.begin(
    const_cast<uint8_t*>(MAC_ADDR),
    STATIC_IP,
    DNS_SERVER,
    GATEWAY_IP,
    SUBNET_MASK
  );

  // Verify hardware is detected
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("[ETH] ERROR: W5500 not found – check wiring!");
    ethData.state = EthState::ERROR;
    return;
  }

  Serial.print("[ETH] Hardware: ");
  Serial.println(
    Ethernet.hardwareStatus() == EthernetW5500 ? "W5500" :
    Ethernet.hardwareStatus() == EthernetW5200 ? "W5200" : "W5100"
  );

  // Start TCP server
  tcpServer.begin();

  ethData.state = EthState::RUNNING;
  Serial.print("[ETH] IP address : ");
  Serial.println(Ethernet.localIP());
  Serial.print("[ETH] TCP server : port ");
  Serial.println(SERVER_PORT);
}

// ============================================================================
// ETH_UPDATE  – called every loop iteration
// ============================================================================

static void eth_update() {
  // Track link state
  EthernetLinkStatus link = Ethernet.linkStatus();
  ethData.linkUp = (link == LinkON);

  if (ethData.state == EthState::RUNNING && !ethData.linkUp) {
    ethData.state = EthState::LINK_DOWN;
    Serial.println("[ETH] Link DOWN");
    return;
  }

  if (ethData.state == EthState::LINK_DOWN && ethData.linkUp) {
    ethData.state = EthState::RUNNING;
    Serial.println("[ETH] Link UP");
  }

  if (ethData.state != EthState::RUNNING) {
    return;
  }

  // Handle incoming TCP clients
  EthernetClient client = tcpServer.available();
  if (!client) {
    return;
  }

  ethData.totalRequests++;

  // Drain the HTTP request (we don't parse it for this test)
  unsigned long requestStart = millis();
  while (client.connected() && client.available()) {
    client.read();
    if (millis() - requestStart > 500) {
      break;  // safety timeout
    }
  }

  // Send a minimal HTTP/1.0 response
  client.println("HTTP/1.0 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println("=== W5500 Test - AK-V2 ===");
  client.print  ("IP      : ");
  client.println(Ethernet.localIP());
  client.print  ("Uptime  : ");
  client.print  (millis() / 1000);
  client.println(" s");
  client.print  ("Requests: ");
  client.println(ethData.totalRequests);

  client.flush();
  delay(1);
  client.stop();
}

// ============================================================================
// SERIAL DIAGNOSTICS
// ============================================================================

static void printStatus() {
  static uint32_t counter = 0;
  counter++;

  Serial.print("[LOG-");
  Serial.print(counter);
  Serial.print("] State: ");
  Serial.print(eth_stateName(ethData.state));
  Serial.print(" | Link: ");
  Serial.print(ethData.linkUp ? "UP" : "DOWN");
  Serial.print(" | Uptime: ");
  Serial.print(millis() / 1000);
  Serial.print("s | Requests: ");
  Serial.println(ethData.totalRequests);
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(100);

  Serial.println();
  Serial.println("=================================================================");
  Serial.println("W5500_Test: WIZnet W5500 Ethernet module test");
  Serial.println("Platform : ESP32-WROOM-32");
  Serial.println("=================================================================");
  Serial.println();

  eth_init();
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
  unsigned long loopStart = millis();

  eth_update();

  // Periodic serial log
  static unsigned long lastLogMs = 0;
  if (millis() - lastLogMs >= SERIAL_LOG_INTERVAL_MS) {
    lastLogMs = millis();
    printStatus();
  }

  // Frame-rate limiting – ~20 Hz
  unsigned long elapsed = millis() - loopStart;
  if (elapsed < LOOP_INTERVAL_MS) {
    delayMicroseconds((LOOP_INTERVAL_MS - elapsed) * 1000);
  }
}
