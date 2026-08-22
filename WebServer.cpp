#include "WebServer.h"
#include "Door.h"
#include "Window.h"
#include "Sensors.h"
#include "Heater.h"
#include "Light.h"
#include "Globals.h"
#include <WiFi.h>

// ============================================================================
// WEB SERVER INSTANCE - GLOBAL DATA
// ============================================================================

WebServerData webServerData;
static WebServerConfig webServerConfig;
static WiFiServer* server = nullptr;

// ============================================================================
// SMALL HELPERS
// ============================================================================

static String boolToJson(bool value) {
  return value ? "true" : "false";
}

static const char* doorStateToCzech(DoorState state) {
  switch (state) {
    case DoorState::UNKNOWN: return "NEZNÁMÁ POLOHA";
    case DoorState::STOPPED: return "ZASTAVENO";
    case DoorState::OPENING: return "OTEVÍRÁNÍ";
    case DoorState::OPENING_SLOW: return "OTEVÍRÁNÍ – POMALÝ DOBĚH";
    case DoorState::OPEN: return "OTEVŘENO";
    case DoorState::CLOSING: return "ZAVÍRÁNÍ";
    case DoorState::CLOSING_SLOW: return "ZAVÍRÁNÍ – POMALÝ DOBĚH";
    case DoorState::OBSTACLE: return "PŘEKÁŽKA";
    case DoorState::TIMEOUT: return "TIMEOUT";
    case DoorState::ERROR: return "CHYBA";
    default: return "NEZNÁMÝ STAV";
  }
}

static const char* windowStateToCzech(WindowState state) {
  switch (state) {
    case WindowState::UNKNOWN: return "NEZNÁMÁ POLOHA";
    case WindowState::STOPPED: return "ZASTAVENO";
    case WindowState::OPENING: return "OTEVÍRÁNÍ";
    case WindowState::OPENING_SLOW: return "OTEVÍRÁNÍ – POMALÝ DOBĚH";
    case WindowState::OPEN: return "OTEVŘENO";
    case WindowState::CLOSING: return "ZAVÍRÁNÍ";
    case WindowState::CLOSING_SLOW: return "ZAVÍRÁNÍ – POMALÝ DOBĚH";
    case WindowState::OBSTACLE: return "PŘEKÁŽKA";
    case WindowState::TIMEOUT: return "TIMEOUT";
    case WindowState::ERROR: return "CHYBA";
    default: return "NEZNÁMÝ STAV";
  }
}

static String doorConfigToJson(const DoorConfig& cfg) {
  String json = "{";
  json += "\"timeoutMs\":" + String(cfg.timeoutMs) + ",";
  json += "\"openStartPower\":" + String(cfg.openStartPower) + ",";
  json += "\"openRunPower\":" + String(cfg.openRunPower) + ",";
  json += "\"closeStartPower\":" + String(cfg.closeStartPower) + ",";
  json += "\"closeRunPower\":" + String(cfg.closeRunPower) + ",";
  json += "\"slowPower\":" + String(cfg.slowPower) + ",";
  json += "\"openStartTimeMs\":" + String(cfg.openStartTimeMs) + ",";
  json += "\"closeStartTimeMs\":" + String(cfg.closeStartTimeMs) + ",";
  json += "\"closeExtraTimeMs\":" + String(cfg.closeExtraTimeMs) + ",";
  json += "\"obstacleCurrentMA\":" + String(cfg.obstacleCurrentMA) + ",";
  json += "\"currentIgnoreStartMs\":" + String(cfg.currentIgnoreStartMs) + ",";
  json += "\"maxRetries\":" + String(cfg.maxRetries);
  json += "}";
  return json;
}

static String windowConfigToJson(const WindowConfig& cfg) {
  String json = "{";
  json += "\"timeoutMs\":" + String(cfg.timeoutMs) + ",";
  json += "\"openStartPower\":" + String(cfg.openStartPower) + ",";
  json += "\"openRunPower\":" + String(cfg.openRunPower) + ",";
  json += "\"closeStartPower\":" + String(cfg.closeStartPower) + ",";
  json += "\"closeRunPower\":" + String(cfg.closeRunPower) + ",";
  json += "\"slowPower\":" + String(cfg.slowPower) + ",";
  json += "\"openStartTimeMs\":" + String(cfg.openStartTimeMs) + ",";
  json += "\"closeStartTimeMs\":" + String(cfg.closeStartTimeMs) + ",";
  json += "\"closeExtraTimeMs\":" + String(cfg.closeExtraTimeMs) + ",";
  json += "\"obstacleCurrentMA\":" + String(cfg.obstacleCurrentMA) + ",";
  json += "\"currentIgnoreStartMs\":" + String(cfg.currentIgnoreStartMs) + ",";
  json += "\"maxRetries\":" + String(cfg.maxRetries);
  json += "}";
  return json;
}

static int jsonFindNumber(const String& json, const char* key, int fallback) {
  String needle = String("\"") + key + "\":";
  int idx = json.indexOf(needle);
  if (idx < 0) return fallback;
  idx += needle.length();
  int end = idx;
  while (end < (int)json.length()) {
    char c = json[end];
    if (!((c >= '0' && c <= '9') || c == '-' )) break;
    end++;
  }
  if (end == idx) return fallback;
  return json.substring(idx, end).toInt();
}

static void jsonToDoorConfig(const String& json, DoorConfig& cfg) {
  cfg.timeoutMs = (uint16_t)jsonFindNumber(json, "timeoutMs", cfg.timeoutMs);
  cfg.openStartPower = (uint8_t)jsonFindNumber(json, "openStartPower", cfg.openStartPower);
  cfg.openRunPower = (uint8_t)jsonFindNumber(json, "openRunPower", cfg.openRunPower);
  cfg.closeStartPower = (uint8_t)jsonFindNumber(json, "closeStartPower", cfg.closeStartPower);
  cfg.closeRunPower = (uint8_t)jsonFindNumber(json, "closeRunPower", cfg.closeRunPower);
  cfg.slowPower = (uint8_t)jsonFindNumber(json, "slowPower", cfg.slowPower);
  cfg.openStartTimeMs = (uint32_t)jsonFindNumber(json, "openStartTimeMs", cfg.openStartTimeMs);
  cfg.closeStartTimeMs = (uint32_t)jsonFindNumber(json, "closeStartTimeMs", cfg.closeStartTimeMs);
  cfg.closeExtraTimeMs = (uint32_t)jsonFindNumber(json, "closeExtraTimeMs", cfg.closeExtraTimeMs);
  cfg.obstacleCurrentMA = (uint16_t)jsonFindNumber(json, "obstacleCurrentMA", cfg.obstacleCurrentMA);
  cfg.currentIgnoreStartMs = (uint32_t)jsonFindNumber(json, "currentIgnoreStartMs", cfg.currentIgnoreStartMs);
  cfg.maxRetries = (uint8_t)jsonFindNumber(json, "maxRetries", cfg.maxRetries);
}

static void jsonToWindowConfig(const String& json, WindowConfig& cfg) {
  cfg.timeoutMs = (uint16_t)jsonFindNumber(json, "timeoutMs", cfg.timeoutMs);
  cfg.openStartPower = (uint8_t)jsonFindNumber(json, "openStartPower", cfg.openStartPower);
  cfg.openRunPower = (uint8_t)jsonFindNumber(json, "openRunPower", cfg.openRunPower);
  cfg.closeStartPower = (uint8_t)jsonFindNumber(json, "closeStartPower", cfg.closeStartPower);
  cfg.closeRunPower = (uint8_t)jsonFindNumber(json, "closeRunPower", cfg.closeRunPower);
  cfg.slowPower = (uint8_t)jsonFindNumber(json, "slowPower", cfg.slowPower);
  cfg.openStartTimeMs = (uint32_t)jsonFindNumber(json, "openStartTimeMs", cfg.openStartTimeMs);
  cfg.closeStartTimeMs = (uint32_t)jsonFindNumber(json, "closeStartTimeMs", cfg.closeStartTimeMs);
  cfg.closeExtraTimeMs = (uint32_t)jsonFindNumber(json, "closeExtraTimeMs", cfg.closeExtraTimeMs);
  cfg.obstacleCurrentMA = (uint16_t)jsonFindNumber(json, "obstacleCurrentMA", cfg.obstacleCurrentMA);
  cfg.currentIgnoreStartMs = (uint32_t)jsonFindNumber(json, "currentIgnoreStartMs", cfg.currentIgnoreStartMs);
  cfg.maxRetries = (uint8_t)jsonFindNumber(json, "maxRetries", cfg.maxRetries);
}

static void sendJson(WiFiClient& client, int code, const String& body) {
  if (code == 200) client.println("HTTP/1.1 200 OK");
  else if (code == 400) client.println("HTTP/1.1 400 Bad Request");
  else if (code == 404) client.println("HTTP/1.1 404 Not Found");
  else client.println("HTTP/1.1 500 Internal Server Error");

  client.println("Content-Type: application/json; charset=utf-8");
  client.println("Content-Length: " + String(body.length()));
  client.println("Connection: close");
  client.println();
  client.print(body);
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void webserver_init() {
  webServerData.currentState = WebServerState::STOPPED;
  webServerData.isRunning = false;
  webServerData.totalRequests = 0;
  webServerData.totalClients = 0;
  webServerData.lastRequestMs = 0;
  webServerData.hasError = false;

  webServerConfig.port = 80;
  webServerConfig.enableSSL = false;
  webServerConfig.sslPort = 443;
  webServerConfig.requireAuthentication = false;
  strcpy(webServerConfig.adminUsername, "admin");
  strcpy(webServerConfig.adminPassword, "");

  server = new WiFiServer(webServerConfig.port);
}

// ============================================================================
// WEB SERVER CONTROL
// ============================================================================

bool webserver_start() {
  if (webServerData.isRunning || server == nullptr) {
    return false;
  }

  server->begin();
  webServerData.currentState = WebServerState::RUNNING;
  webServerData.isRunning = true;
  webServerData.totalRequests = 0;
  webServerData.totalClients = 0;

  Serial.println("Web server started on port 80");
  return true;
}

void webserver_stop() {
  if (!webServerData.isRunning || server == nullptr) {
    return;
  }

  server->stop();
  webServerData.currentState = WebServerState::STOPPED;
  webServerData.isRunning = false;

  Serial.println("Web server stopped");
}

// ============================================================================
// HTML TEMPLATE
// ============================================================================

const char* HTML_TEMPLATE = R"rawliteral(
<!DOCTYPE html>
<html lang="cs">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>AK-V2 Kurník Kontrola</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: #1a1a1a;
      color: #e0e0e0;
      overflow-x: hidden;
    }
    .container { display: flex; min-height: 100vh; }
    .sidebar {
      width: 250px; background: #0d0d0d; padding: 20px;
      border-right: 1px solid #333; overflow-y: auto;
      position: fixed; height: 100vh; left: 0; top: 0; z-index: 1000;
    }
    .logo {
      font-size: 20px; font-weight: bold; color: #4CAF50;
      margin-bottom: 30px; text-align: center;
    }
    .menu-item {
      padding: 12px; margin: 5px 0; border-radius: 5px; cursor: pointer;
      transition: all 0.3s; font-size: 14px; border-left: 3px solid transparent;
    }
    .menu-item:hover { background: #2a2a2a; border-left-color: #4CAF50; }
    .menu-item.active { background: #2a5a2a; border-left-color: #4CAF50; color: #4CAF50; }
    .content { margin-left: 250px; padding: 20px; flex: 1; width: calc(100% - 250px); }
    .page { display: none; }
    .page.active { display: block; animation: fadeIn 0.3s; }
    @keyframes fadeIn { from { opacity: 0; } to { opacity: 1; } }
    h1 { margin-bottom: 20px; color: #4CAF50; font-size: 28px; }
    h2 { margin-top: 20px; margin-bottom: 15px; color: #4CAF50; font-size: 20px; }
    .cards { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin-bottom: 20px; }
    .card {
      background: #2a2a2a; padding: 20px; border-radius: 8px;
      border-left: 4px solid #4CAF50; box-shadow: 0 2px 10px rgba(0,0,0,0.3);
    }
    .card.error { border-left-color: #f44336; }
    .card.warning { border-left-color: #ff9800; }
    .card-label { font-size: 12px; color: #999; text-transform: uppercase; margin-bottom: 5px; }
    .card-value { font-size: 24px; font-weight: bold; color: #4CAF50; margin-bottom: 10px; }
    .button-group { display: flex; gap: 10px; margin-top: 15px; flex-wrap: wrap; }
    button {
      padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer;
      font-size: 14px; font-weight: bold; transition: all 0.3s; flex: 1; min-width: 80px;
    }
    button.primary { background: #4CAF50; color: white; }
    button.primary:hover { background: #45a049; }
    button.danger { background: #f44336; color: white; }
    button.danger:hover { background: #da190b; }
    button.secondary { background: #666; color: white; }
    button.secondary:hover { background: #777; }
    .form-group { margin-bottom: 15px; }
    label { display: block; margin-bottom: 5px; font-size: 14px; color: #999; }
    input, select {
      width: 100%; padding: 10px; background: #1a1a1a; border: 1px solid #444;
      border-radius: 4px; color: #e0e0e0; font-size: 14px;
    }
    input:focus, select:focus { outline: none; border-color: #4CAF50; box-shadow: 0 0 5px rgba(76,175,80,0.3); }
    table { width: 100%; border-collapse: collapse; margin-top: 15px; }
    th { background: #1a1a1a; padding: 10px; text-align: left; border-bottom: 2px solid #444; color: #4CAF50; font-size: 12px; }
    td { padding: 10px; border-bottom: 1px solid #333; }
    tr:hover { background: #333; }
    @media (max-width: 768px) {
      .sidebar { width: 100%; height: auto; position: relative; border-right: none; border-bottom: 1px solid #333; }
      .content { margin-left: 0; width: 100%; }
      .cards { grid-template-columns: 1fr; }
      .button-group { flex-direction: column; }
      button { width: 100%; }
    }
    .status-badge { display: inline-block; padding: 5px 10px; border-radius: 3px; font-size: 12px; font-weight: bold; }
    .status-on { background: #4CAF50; color: white; }
    .status-off { background: #666; color: white; }
    .status-auto { background: #ff9800; color: white; }
    .time-display { font-size: 16px; color: #4CAF50; margin-bottom: 5px; }
  </style>
</head>
<body>
  <div class="container">
    <div class="sidebar">
      <div class="logo">🐔 AK-V2</div>
      <div class="menu-item active" onclick="showPage('dashboard')">📊 Dashboard</div>
      <div class="menu-item" onclick="showPage('manual')">🎮 Manuální Ovládání</div>
      <div class="menu-item" onclick="showPage('motor-settings')">⚙️ Nastavení Motorů</div>
      <div class="menu-item" onclick="showPage('automation')">🤖 Automatika</div>
      <div class="menu-item" onclick="showPage('alarms')">🚨 Alarmy</div>
      <div class="menu-item" onclick="showPage('sensors')">📡 Čidla</div>
      <div class="menu-item" onclick="showPage('lighting')">💡 Osvětlení</div>
      <div class="menu-item" onclick="showPage('heating')">🔥 Topení</div>
      <div class="menu-item" onclick="showPage('network')">🌐 Síť</div>
      <div class="menu-item" onclick="showPage('ota')">📦 OTA</div>
      <div class="menu-item" onclick="showPage('diagnostics')">🔧 Diagnostika</div>
      <div class="menu-item" onclick="showPage('service')">🛠️ Servis</div>
    </div>
    <div class="content">
      <div class="page active" id="dashboard">
        <h1>📊 Dashboard</h1>
        <div class="time-display">Čas: <span id="currentTime">--:--:--</span></div>
        <div class="cards">
          <div class="card">
            <div class="card-label">Dveře</div>
            <div class="card-value" id="doorStatus">ZAVŘENO</div>
            <div class="card-label">Proud: <span id="doorCurrent">0 mA</span></div>
            <div class="card-label">Opakování: <span id="doorRetries">0</span></div>
            <div class="button-group">
              <button class="primary" onclick="apiCall('/api/door/open')">Otevřít</button>
              <button class="danger" onclick="apiCall('/api/door/close')">Zavřít</button>
            </div>
          </div>
          <div class="card">
            <div class="card-label">Okno</div>
            <div class="card-value" id="windowStatus">ZAVŘENO</div>
            <div class="card-label">Proud: <span id="windowCurrent">0 mA</span></div>
            <div class="card-label">Opakování: <span id="windowRetries">0</span></div>
            <div class="button-group">
              <button class="primary" onclick="apiCall('/api/window/open')">Otevřít</button>
              <button class="danger" onclick="apiCall('/api/window/close')">Zavřít</button>
            </div>
          </div>
          <div class="card">
            <div class="card-label">Kamera</div>
            <div class="card-value" id="cameraStatus"><span class="status-badge status-off">VYPNUTO</span></div>
            <div class="button-group">
              <button class="primary" onclick="apiCall('/api/camera/on')">Zapnout</button>
              <button class="danger" onclick="apiCall('/api/camera/off')">Vypnout</button>
            </div>
          </div>
          <div class="card"><div class="card-label">Teplota Kurník</div><div class="card-value" id="coopTemp">-- °C</div></div>
          <div class="card"><div class="card-label">Teplota Rozvaděč</div><div class="card-value" id="cabinetTemp">-- °C</div></div>
          <div class="card"><div class="card-label">Vlhkost Rozvaděč</div><div class="card-value" id="cabinetHumidity">-- %</div></div>
          <div class="card"><div class="card-label">Rosný Bod</div><div class="card-value" id="dewPoint">-- °C</div></div>
          <div class="card"><div class="card-label">Topení</div><div class="card-value" id="heaterStatus"><span class="status-badge status-off">VYPNUTO</span></div></div>
          <div class="card"><div class="card-label">Osvětlení</div><div class="card-value" id="lightStatus"><span class="status-badge status-off">VYPNUTO</span></div></div>
          <div class="card"><div class="card-label">Režim Systému</div><div class="card-value" id="systemMode">RUN</div></div>
        </div>
      </div>
      <div class="page" id="manual">
        <h1>🎮 Manuální Ovládání</h1>
        <h2>Dveře</h2>
        <div class="button-group">
          <button class="primary" onclick="apiCall('/api/door/open')">Otevřít</button>
          <button class="danger" onclick="apiCall('/api/door/close')">Zavřít</button>
          <button class="secondary" onclick="apiCall('/api/door/stop')">Stop</button>
          <button class="secondary" onclick="apiCall('/api/door/reset')">Reset Chyby</button>
        </div>
        <h2>Okno</h2>
        <div class="button-group">
          <button class="primary" onclick="apiCall('/api/window/open')">Otevřít</button>
          <button class="danger" onclick="apiCall('/api/window/close')">Zavřít</button>
          <button class="secondary" onclick="apiCall('/api/window/stop')">Stop</button>
          <button class="secondary" onclick="apiCall('/api/window/reset')">Reset Chyby</button>
        </div>
      </div>
    </div>
  </div>
  <script>
    function showPage(pageId) {
      document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
      document.querySelectorAll('.menu-item').forEach(m => m.classList.remove('active'));
      document.getElementById(pageId).classList.add('active');
      event.target.classList.add('active');
    }
    function apiCall(endpoint) {
      fetch(endpoint).then(r => r.json()).then(data => console.log('OK:', data)).catch(e => console.error('Chyba:', e));
    }
    function updateTime() {
      const now = new Date();
      const hours = String(now.getHours()).padStart(2, '0');
      const minutes = String(now.getMinutes()).padStart(2, '0');
      const seconds = String(now.getSeconds()).padStart(2, '0');
      document.getElementById('currentTime').textContent = hours + ':' + minutes + ':' + seconds;
    }
    function updateStatus() {
      fetch('/api/status').then(r => r.json()).then(data => {
        document.getElementById('doorStatus').textContent = data.door_status || '--';
        document.getElementById('windowStatus').textContent = data.window_status || '--';
        document.getElementById('cameraStatus').innerHTML = '<span class="status-badge ' + (data.camera ? 'status-on' : 'status-off') + '">' + (data.camera ? 'ZAPNUTO' : 'VYPNUTO') + '</span>';
        document.getElementById('coopTemp').textContent = (data.coop_temp || 0).toFixed(1) + ' °C';
        document.getElementById('cabinetTemp').textContent = (data.cabinet_temp || 0).toFixed(1) + ' °C';
        document.getElementById('cabinetHumidity').textContent = (data.cabinet_humidity || 0).toFixed(0) + ' %';
        document.getElementById('dewPoint').textContent = (data.dew_point || 0).toFixed(1) + ' °C';
        document.getElementById('heaterStatus').innerHTML = '<span class="status-badge ' + (data.heater ? 'status-on' : 'status-off') + '">' + (data.heater ? 'ZAPNUTO' : 'VYPNUTO') + '</span>';
        document.getElementById('lightStatus').innerHTML = '<span class="status-badge ' + (data.light ? 'status-on' : 'status-off') + '">' + (data.light ? 'ZAPNUTO' : 'VYPNUTO') + '</span>';
        document.getElementById('systemMode').textContent = data.system_mode || 'RUN';
        document.getElementById('uptime').textContent = Math.floor((data.uptime || 0) / 1000) + ' s';
        document.getElementById('ipAddress').textContent = data.ip_address || '--';
        document.getElementById('sensorCoopTemp').textContent = (data.coop_temp || 0).toFixed(1) + ' °C';
        document.getElementById('sensorCabinetTemp').textContent = (data.cabinet_temp || 0).toFixed(1) + ' °C';
        document.getElementById('sensorCabinetHumidity').textContent = (data.cabinet_humidity || 0).toFixed(0) + ' %';
        document.getElementById('sensorDewPoint').textContent = (data.dew_point || 0).toFixed(1) + ' °C';
        document.getElementById('doorCurrent').textContent = (data.door_current || 0) + ' mA';
        document.getElementById('windowCurrent').textContent = (data.window_current || 0) + ' mA';
        document.getElementById('doorRetries').textContent = data.door_retries || 0;
        document.getElementById('windowRetries').textContent = data.window_retries || 0;
      }).catch(e => console.error('Chyba:', e));
    }
    function saveDoorSettings() { alert('Nastavení dveří uloženo'); }
    function saveWindowSettings() { alert('Nastavení okna uloženo'); }
    function saveDoorAutomation() { alert('Automatika dveří uložena'); }
    function saveWindowAutomation() { alert('Automatika okna uložena'); }
    function saveCameraAutomation() { alert('Automatika kamery uložena'); }
    function saveGPS() { alert('GPS parametry uloženy'); }
    function saveLightingSettings() { alert('Nastavení osvětlení uloženo'); }
    function saveHeatingSettings() { alert('Nastavení topení uloženo'); }
    function saveNetworkSettings() { alert('Nastavení sítě uloženo'); }
    function uploadFirmware() { alert('Funkce nahrávání firmware není implementována'); }
    function restartDevice() { if (confirm('Chcete restartovat zařízení?')) apiCall('/api/restart'); }
    function factoryReset() { if (confirm('Chcete obnovit tovární nastavení?')) apiCall('/api/factory-reset'); }
    function testRelays() { apiCall('/api/test/relays'); }
    function testMotors() { apiCall('/api/test/motors'); }
    function testLimits() { apiCall('/api/test/limits'); }
    function testACS712() { apiCall('/api/test/acs712'); }
    function testSHT30() { apiCall('/api/test/sht30'); }
    function testDS18B20() { apiCall('/api/test/ds18b20'); }
    setInterval(updateTime, 1000);
    setInterval(updateStatus, 2000);
    updateTime();
    updateStatus();
  </script>
</body>
</html>
)rawliteral";

// ============================================================================
// WEB SERVER UPDATE - HANDLE CLIENT REQUESTS
// ============================================================================

void webserver_update() {
  if (!webServerData.isRunning || server == nullptr) {
    return;
  }

  WiFiClient client = server->available();
  if (!client) {
    return;
  }

  unsigned long timeout = millis() + 1000;
  while (client.connected() && !client.available() && millis() < timeout) {
    delay(1);
  }

  if (!client.available()) {
    client.stop();
    return;
  }

  String request = client.readStringUntil('\n');
  request.trim();

  int firstSpace = request.indexOf(' ');
  int lastSpace = request.lastIndexOf(' ');
  if (firstSpace < 0 || lastSpace < 0 || lastSpace <= firstSpace) {
    client.stop();
    return;
  }

  String method = request.substring(0, firstSpace);
  String path = request.substring(firstSpace + 1, lastSpace);

  String body = "";
  while (client.available()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }
  while (client.available()) {
    body += (char)client.read();
  }
  body.trim();

  webServerData.totalRequests++;
  webServerData.lastRequestMs = millis();

  if (path == "/" || path == "/index.html") {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html; charset=utf-8");
    client.println("Connection: close");
    client.println();
    client.print(HTML_TEMPLATE);
  }
  else if (path == "/api/status") {
    String json = "{";
    json += "\"door_status\":\"" + String(doorStateToCzech(door_getState())) + "\",";
    json += "\"door_state_raw\":\"" + String(door_getStateName()) + "\",";
    json += "\"door_open\":" + boolToJson(door_isOpen()) + ",";
    json += "\"door_closed\":" + boolToJson(door_isClosed()) + ",";
    json += "\"door_error\":" + boolToJson(door_hasError()) + ",";
    json += "\"door_current\":" + String(door_getCurrentMA()) + ",";
    json += "\"door_retries\":" + String(door_getRetryCount()) + ",";
    json += "\"window_status\":\"" + String(windowStateToCzech(window_getState())) + "\",";
    json += "\"window_state_raw\":\"" + String(window_getStateName()) + "\",";
    json += "\"window_open\":" + boolToJson(window_isOpen()) + ",";
    json += "\"window_closed\":" + boolToJson(window_isClosed()) + ",";
    json += "\"window_error\":" + boolToJson(window_hasError()) + ",";
    json += "\"window_current\":" + String(window_getCurrentMA()) + ",";
    json += "\"window_retries\":" + String(window_getRetryCount()) + ",";
    json += "\"camera\":" + boolToJson(camera_isActive()) + ",";
    json += "\"coop_temp\":" + String(coopEnvironment.temperatureC, 1) + ",";
    json += "\"cabinet_temp\":" + String(cabinetEnvironment.temperatureC, 1) + ",";
    json += "\"cabinet_humidity\":" + String(cabinetEnvironment.humidityPct, 0) + ",";
    json += "\"dew_point\":" + String(cabinetEnvironment.dewPointC, 1) + ",";
    json += "\"heater\":" + boolToJson(heater_isActive()) + ",";
    json += "\"light\":" + boolToJson(light_isActive()) + ",";
    json += "\"system_mode\":\"RUN\",";
    json += "\"uptime\":" + String(systemUptime) + ",";
    json += "\"ip_address\":\"" + WiFi.localIP().toString() + "\"";
    json += "}";
    sendJson(client, 200, json);
  }
  else if (path == "/api/door/settings" || path == "/api/window/settings") {
    bool isDoor = path.startsWith("/api/door/");
    bool isPost = method == "POST";

    if (!isPost) {
      if (isDoor) sendJson(client, 200, doorConfigToJson(*door_getConfig()));
      else sendJson(client, 200, windowConfigToJson(*window_getConfig()));
    } else {
      if (isDoor) {
        DoorConfig cfg = *door_getConfig();
        jsonToDoorConfig(body, cfg);
        door_setConfig(cfg);
        sendJson(client, 200, "{\"status\":\"ok\"}");
      } else {
        WindowConfig cfg = *window_getConfig();
        jsonToWindowConfig(body, cfg);
        window_setConfig(cfg);
        sendJson(client, 200, "{\"status\":\"ok\"}");
      }
    }
  }
  else if (path == "/api/door/open") {
    door_open();
    sendJson(client, 200, "{\"status\":\"ok\"}");
  }
  else if (path == "/api/door/close") {
    door_close();
    sendJson(client, 200, "{\"status\":\"ok\"}");
  }
  else if (path == "/api/door/stop") {
    door_stop();
    sendJson(client, 200, "{\"status\":\"ok\"}");
  }
  else if (path == "/api/door/reset") {
    door_resetError();
    sendJson(client, 200, "{\"status\":\"ok\"}");
  }
  else if (path == "/api/window/open") {
    window_open();
    sendJson(client, 200, "{\"status\":\"ok\"}");
  }
  else if (path == "/api/window/close") {
    window_close();
    sendJson(client, 200, "{\"status\":\"ok\"}");
  }
  else if (path == "/api/window/stop") {
    window_stop();
    sendJson(client, 200, "{\"status\":\"ok\"}");
  }
  else if (path == "/api/window/reset") {
    window_resetError();
    sendJson(client, 200, "{\"status\":\"ok\"}");
  }
  else if (path == "/api/camera/on") {
    camera_on();
    sendJson(client, 200, "{\"status\":\"ok\"}");
  }
  else if (path == "/api/camera/off") {
    camera_off();
    sendJson(client, 200, "{\"status\":\"ok\"}");
  }
  else if (path == "/api/camera/auto") {
    camera_setAutoMode();
    sendJson(client, 200, "{\"status\":\"ok\"}");
  }
  else if (path == "/api/heater/on") {
    heater_setMode(HeaterState::ON);
    sendJson(client, 200, "{\"status\":\"ok\"}");
  }
  else if (path == "/api/heater/off") {
    heater_setMode(HeaterState::OFF);
    sendJson(client, 200, "{\"status\":\"ok\"}");
  }
  else if (path == "/api/heater/auto") {
    heater_setMode(HeaterState::AUTO);
    sendJson(client, 200, "{\"status\":\"ok\"}");
  }
  else if (path == "/api/light/auto") {
    light_setMode(LightMode::AUTO);
    sendJson(client, 200, "{\"status\":\"ok\"}");
  }
  else if (path == "/api/light/off") {
    light_off();
    sendJson(client, 200, "{\"status\":\"ok\"}");
  }
  else if (path == "/api/restart") {
    sendJson(client, 200, "{\"status\":\"ok\"}");
    delay(100);
    ESP.restart();
  }
  else if (path == "/api/factory-reset") {
    settings_reset();
    sendJson(client, 200, "{\"status\":\"ok\"}");
  }
  else {
    sendJson(client, 404, "{\"status\":\"not_found\"}");
  }

  delay(1);
  client.stop();
}

// ============================================================================
// PUBLIC API
// ============================================================================

bool webserver_isRunning() { return webServerData.isRunning; }
WebServerState webserver_getState() { return webServerData.currentState; }
WebServerConfig* webserver_getConfig() { return &webServerConfig; }
WebServerData* webserver_getData() { return &webServerData; }

const char* webserver_getStateName(WebServerState state) {
  switch (state) {
    case WebServerState::STOPPED: return "STOPPED";
    case WebServerState::RUNNING: return "RUNNING";
    case WebServerState::ERROR: return "ERROR";
    default: return "UNKNOWN";
  }
}
