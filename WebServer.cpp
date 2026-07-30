#include "WebServer.h"
#include "Motor.h"
#include "Sensors.h"
#include "Heater.h"
#include "Light.h"
#include "Globals.h"
#include <WebServer.h>

// ============================================================================
// WEB SERVER INSTANCE - GLOBAL DATA
// ============================================================================

WebServerData webServerData;
static WebServerConfig webServerConfig;
static WebServer* server = nullptr;

// Forward declarations for external motor instances
extern Motor doorMotor;
extern Motor windowMotor;

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

  // Initialize config
  webServerConfig.port = 80;
  webServerConfig.enableSSL = false;
  webServerConfig.sslPort = 443;
  webServerConfig.requireAuthentication = false;
  strcpy(webServerConfig.adminUsername, "admin");
  strcpy(webServerConfig.adminPassword, "");

  // Create web server instance
  server = new WebServer(80);
}

// ============================================================================
// WEB SERVER CONTROL
// ============================================================================

bool webserver_start() {
  if (webServerData.isRunning || server == nullptr) {
    return false;
  }

  // ========================================================================
  // MAIN STATUS PAGE
  // ========================================================================

  server->on("/", HTTP_GET, []() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>AK-V2 Kurin Kontrola</title>
  <meta charset="UTF-8">
  <style>
    body { font-family: Arial; margin: 20px; background: #f5f5f5; }
    .container { max-width: 1000px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
    h1 { color: #333; }
    .status { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; }
    .card { background: #f9f9f9; padding: 15px; border-radius: 5px; border-left: 4px solid #4CAF50; }
    .card.error { border-left-color: #f44336; }
    .card.warning { border-left-color: #ff9800; }
    .label { font-weight: bold; color: #555; font-size: 14px; }
    .value { color: #333; font-size: 18px; margin-top: 5px; margin-bottom: 10px; }
    button { background: #4CAF50; color: white; padding: 8px 16px; border: none; border-radius: 4px; cursor: pointer; margin: 3px; font-size: 12px; }
    button:hover { background: #45a049; }
    button.danger { background: #f44336; }
    button.danger:hover { background: #da190b; }
    .refresh { color: #999; font-size: 12px; }
  </style>
</head>
<body>
  <div class="container">
    <h1>AK-V2 Kurin Kontrola</h1>
    <p class="refresh">Aktualizace kazde 2 sekundy</p>
    
    <h2>Motory</h2>
    <div class="status">
      <div class="card">
        <div class="label">Dvierka</div>
        <div class="value" id="doorStatus">ZAVRENA</div>
        <button onclick="send('/api/door/open')">Otevrit</button>
        <button class="danger" onclick="send('/api/door/close')">Zavrit</button>
      </div>
      <div class="card">
        <div class="label">Okno</div>
        <div class="value" id="windowStatus">ZAVRENO</div>
        <button onclick="send('/api/window/open')">Otevrit</button>
        <button class="danger" onclick="send('/api/window/close')">Zavrit</button>
      </div>
    </div>

    <h2>Senzory</h2>
    <div class="status">
      <div class="card">
        <div class="label">Teplota</div>
        <div class="value" id="temperature">-- C</div>
      </div>
      <div class="card">
        <div class="label">Vlhkost</div>
        <div class="value" id="humidity">-- %</div>
      </div>
    </div>

    <h2>Autonomie</h2>
    <div class="status">
      <div class="card">
        <div class="label">Topeni</div>
        <div class="value" id="heaterStatus">VYPNUTO</div>
        <button onclick="send('/api/heater/on')">Zapnout</button>
        <button class="danger" onclick="send('/api/heater/off')">Vypnout</button>
      </div>
      <div class="card">
        <div class="label">Osvetleni</div>
        <div class="value" id="lightStatus">VYPNUTO</div>
        <button onclick="send('/api/light/on')">Zapnout</button>
        <button class="danger" onclick="send('/api/light/off')">Vypnout</button>
      </div>
    </div>

    <h2>System</h2>
    <div class="status">
      <div class="card">
        <div class="label">Provozni cas</div>
        <div class="value" id="uptime">-- s</div>
      </div>
      <div class="card">
        <div class="label">WiFi</div>
        <div class="value" id="wifiStatus">PRIPOJENO</div>
      </div>
    </div>
  </div>

  <script>
    function send(url) {
      fetch(url).catch(e => console.error('Chyba:', e));
    }
    
    setInterval(() => {
      fetch('/api/status')
        .then(r => r.json())
        .then(data => {
          document.getElementById('doorStatus').textContent = data.door_status;
          document.getElementById('windowStatus').textContent = data.window_status;
          document.getElementById('temperature').textContent = data.temperature + ' C';
          document.getElementById('humidity').textContent = data.humidity + ' %';
          document.getElementById('heaterStatus').textContent = data.heater_status;
          document.getElementById('lightStatus').textContent = data.light_status;
          document.getElementById('uptime').textContent = data.uptime + ' s';
        })
        .catch(e => console.error('Chyba:', e));
    }, 2000);
  </script>
</body>
</html>
)rawliteral";
    server->send(200, "text/html; charset=utf-8", html);
  });

  // ========================================================================
  // API STATUS ENDPOINT
  // ========================================================================

  server->on("/api/status", HTTP_GET, []() {
    String json = "{";
    json += "\"door_status\":\"" + String(motor_getStateName(doorMotor.data.state)) + "\",";
    json += "\"window_status\":\"" + String(motor_getStateName(windowMotor.data.state)) + "\",";
    json += "\"temperature\":" + String(coopEnvironment.temperatureC, 1) + ",";
    json += "\"humidity\":" + String(coopEnvironment.humidityPercent, 0) + ",";
    json += "\"heater_status\":\"" + String(heater_getStateName(heater_getState())) + "\",";
    json += "\"light_status\":\"" + String(light_getStateName(light_getState())) + "\",";
    json += "\"uptime\":" + String(systemUptime / 1000);
    json += "}";
    server->send(200, "application/json", json);
  });

  // ========================================================================
  // DOOR CONTROL ENDPOINTS
  // ========================================================================

  server->on("/api/door/open", HTTP_GET, []() {
    motor_setCommand(&doorMotor, MotorCommand::OPEN);
    server->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server->on("/api/door/close", HTTP_GET, []() {
    motor_setCommand(&doorMotor, MotorCommand::CLOSE);
    server->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server->on("/api/door/stop", HTTP_GET, []() {
    motor_setCommand(&doorMotor, MotorCommand::STOP);
    server->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // ========================================================================
  // WINDOW CONTROL ENDPOINTS
  // ========================================================================

  server->on("/api/window/open", HTTP_GET, []() {
    motor_setCommand(&windowMotor, MotorCommand::OPEN);
    server->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server->on("/api/window/close", HTTP_GET, []() {
    motor_setCommand(&windowMotor, MotorCommand::CLOSE);
    server->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server->on("/api/window/stop", HTTP_GET, []() {
    motor_setCommand(&windowMotor, MotorCommand::STOP);
    server->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // ========================================================================
  // HEATER CONTROL ENDPOINTS
  // ========================================================================

  server->on("/api/heater/on", HTTP_GET, []() {
    heater_on();
    server->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server->on("/api/heater/off", HTTP_GET, []() {
    heater_off();
    server->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // ========================================================================
  // LIGHT CONTROL ENDPOINTS
  // ========================================================================

  server->on("/api/light/on", HTTP_GET, []() {
    light_on();
    server->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server->on("/api/light/off", HTTP_GET, []() {
    light_off();
    server->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // Start server
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
// WEB SERVER UPDATE
// ============================================================================

void webserver_update() {
  if (!webServerData.isRunning || server == nullptr) {
    return;
  }

  server->handleClient();
}

// ============================================================================
// API ENDPOINT HANDLERS
// ============================================================================

void webserver_handleAPI(const char* endpoint, const char* method, const char* body) {
  // Handled by WebServer callbacks above
}

// ============================================================================
// PUBLIC API
// ============================================================================

bool webserver_isRunning() {
  return webServerData.isRunning;
}

WebServerState webserver_getState() {
  return webServerData.currentState;
}

WebServerConfig* webserver_getConfig() {
  return &webServerConfig;
}

WebServerData* webserver_getData() {
  return &webServerData;
}

const char* webserver_getStateName(WebServerState state) {
  switch (state) {
    case WebServerState::STOPPED: return "STOPPED";
    case WebServerState::RUNNING: return "RUNNING";
    case WebServerState::ERROR:   return "ERROR";
    default:                      return "UNKNOWN";
  }
}
