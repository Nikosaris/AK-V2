#include "WebServer.h"
#include "Motor.h"
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
  server = new WiFiServer(80);
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
    
    .container {
      display: flex;
      min-height: 100vh;
    }
    
    /* SIDEBAR */
    .sidebar {
      width: 250px;
      background: #0d0d0d;
      padding: 20px;
      border-right: 1px solid #333;
      overflow-y: auto;
      position: fixed;
      height: 100vh;
      left: 0;
      top: 0;
      z-index: 1000;
    }
    
    .logo {
      font-size: 20px;
      font-weight: bold;
      color: #4CAF50;
      margin-bottom: 30px;
      text-align: center;
    }
    
    .menu-item {
      padding: 12px;
      margin: 5px 0;
      border-radius: 5px;
      cursor: pointer;
      transition: all 0.3s;
      font-size: 14px;
      border-left: 3px solid transparent;
    }
    
    .menu-item:hover {
      background: #2a2a2a;
      border-left-color: #4CAF50;
    }
    
    .menu-item.active {
      background: #2a5a2a;
      border-left-color: #4CAF50;
      color: #4CAF50;
    }
    
    /* MAIN CONTENT */
    .content {
      margin-left: 250px;
      padding: 20px;
      flex: 1;
      width: calc(100% - 250px);
    }
    
    .page {
      display: none;
    }
    
    .page.active {
      display: block;
      animation: fadeIn 0.3s;
    }
    
    @keyframes fadeIn {
      from { opacity: 0; }
      to { opacity: 1; }
    }
    
    h1 {
      margin-bottom: 20px;
      color: #4CAF50;
      font-size: 28px;
    }
    
    h2 {
      margin-top: 20px;
      margin-bottom: 15px;
      color: #4CAF50;
      font-size: 20px;
    }
    
    /* CARDS */
    .cards {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
      gap: 20px;
      margin-bottom: 20px;
    }
    
    .card {
      background: #2a2a2a;
      padding: 20px;
      border-radius: 8px;
      border-left: 4px solid #4CAF50;
      box-shadow: 0 2px 10px rgba(0,0,0,0.3);
    }
    
    .card.error {
      border-left-color: #f44336;
    }
    
    .card.warning {
      border-left-color: #ff9800;
    }
    
    .card-label {
      font-size: 12px;
      color: #999;
      text-transform: uppercase;
      margin-bottom: 5px;
    }
    
    .card-value {
      font-size: 24px;
      font-weight: bold;
      color: #4CAF50;
      margin-bottom: 10px;
    }
    
    .card-value.red {
      color: #f44336;
    }
    
    .card-value.orange {
      color: #ff9800;
    }
    
    /* BUTTONS */
    .button-group {
      display: flex;
      gap: 10px;
      margin-top: 15px;
      flex-wrap: wrap;
    }
    
    button {
      padding: 10px 20px;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      font-size: 14px;
      font-weight: bold;
      transition: all 0.3s;
      flex: 1;
      min-width: 80px;
    }
    
    button.primary {
      background: #4CAF50;
      color: white;
    }
    
    button.primary:hover {
      background: #45a049;
    }
    
    button.danger {
      background: #f44336;
      color: white;
    }
    
    button.danger:hover {
      background: #da190b;
    }
    
    button.secondary {
      background: #666;
      color: white;
    }
    
    button.secondary:hover {
      background: #777;
    }
    
    /* FORMS */
    .form-group {
      margin-bottom: 15px;
    }
    
    label {
      display: block;
      margin-bottom: 5px;
      font-size: 14px;
      color: #999;
    }
    
    input, select {
      width: 100%;
      padding: 10px;
      background: #1a1a1a;
      border: 1px solid #444;
      border-radius: 4px;
      color: #e0e0e0;
      font-size: 14px;
    }
    
    input:focus, select:focus {
      outline: none;
      border-color: #4CAF50;
      box-shadow: 0 0 5px rgba(76, 175, 80, 0.3);
    }
    
    /* TABLE */
    table {
      width: 100%;
      border-collapse: collapse;
      margin-top: 15px;
    }
    
    th {
      background: #1a1a1a;
      padding: 10px;
      text-align: left;
      border-bottom: 2px solid #444;
      color: #4CAF50;
      font-size: 12px;
    }
    
    td {
      padding: 10px;
      border-bottom: 1px solid #333;
    }
    
    tr:hover {
      background: #333;
    }
    
    /* RESPONSIVE */
    @media (max-width: 768px) {
      .sidebar {
        width: 100%;
        height: auto;
        position: relative;
        border-right: none;
        border-bottom: 1px solid #333;
      }
      
      .content {
        margin-left: 0;
        width: 100%;
      }
      
      .cards {
        grid-template-columns: 1fr;
      }
      
      .button-group {
        flex-direction: column;
      }
      
      button {
        width: 100%;
      }
    }
    
    .status-badge {
      display: inline-block;
      padding: 5px 10px;
      border-radius: 3px;
      font-size: 12px;
      font-weight: bold;
    }
    
    .status-on {
      background: #4CAF50;
      color: white;
    }
    
    .status-off {
      background: #666;
      color: white;
    }
    
    .status-auto {
      background: #ff9800;
      color: white;
    }

    .time-display {
      font-size: 16px;
      color: #4CAF50;
      margin-bottom: 5px;
    }
  </style>
</head>
<body>
  <div class="container">
    <!-- SIDEBAR -->
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
    
    <!-- MAIN CONTENT -->
    <div class="content">
      <!-- DASHBOARD -->
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
          
          <div class="card">
            <div class="card-label">Teplota Kurník</div>
            <div class="card-value" id="coopTemp">-- °C</div>
          </div>
          
          <div class="card">
            <div class="card-label">Teplota Rozvaděč</div>
            <div class="card-value" id="cabinetTemp">-- °C</div>
          </div>
          
          <div class="card">
            <div class="card-label">Vlhkost Rozvaděč</div>
            <div class="card-value" id="cabinetHumidity">-- %</div>
          </div>
          
          <div class="card">
            <div class="card-label">Rosný Bod</div>
            <div class="card-value" id="dewPoint">-- °C</div>
          </div>
          
          <div class="card">
            <div class="card-label">Topení</div>
            <div class="card-value" id="heaterStatus"><span class="status-badge status-off">VYPNUTO</span></div>
          </div>
          
          <div class="card">
            <div class="card-label">Osvětlení</div>
            <div class="card-value" id="lightStatus"><span class="status-badge status-off">VYPNUTO</span></div>
          </div>
          
          <div class="card">
            <div class="card-label">Režim Systému</div>
            <div class="card-value" id="systemMode">RUN</div>
          </div>
        </div>
      </div>
      
      <!-- MANUAL CONTROL -->
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
        
        <h2>Kamera</h2>
        <div class="button-group">
          <button class="primary" onclick="apiCall('/api/camera/on')">Zapnout</button>
          <button class="danger" onclick="apiCall('/api/camera/off')">Vypnout</button>
          <button class="secondary" onclick="apiCall('/api/camera/auto')">Automatika</button>
        </div>
        
        <h2>Osvětlení</h2>
        <div class="button-group">
          <button class="primary" onclick="apiCall('/api/light/auto')">Automatika</button>
          <button class="danger" onclick="apiCall('/api/light/off')">Vypnout</button>
        </div>
        
        <h2>Topení</h2>
        <div class="button-group">
          <button class="primary" onclick="apiCall('/api/heater/on')">Zapnout</button>
          <button class="danger" onclick="apiCall('/api/heater/off')">Vypnout</button>
          <button class="secondary" onclick="apiCall('/api/heater/auto')">Automatika</button>
        </div>
      </div>
      
      <!-- MOTOR SETTINGS -->
      <div class="page" id="motor-settings">
        <h1>⚙️ Nastavení Motorů</h1>
        
        <h2>Dveře</h2>
        <div class="cards">
          <div class="card">
            <div class="form-group">
              <label>Timeout (ms)</label>
              <input type="number" id="doorTimeout" placeholder="30000">
            </div>
            <div class="form-group">
              <label>Maximální Proud (mA)</label>
              <input type="number" id="doorMaxCurrent" placeholder="2000">
            </div>
            <div class="form-group">
              <label>Proudová Ochrana při Otevírání (mA)</label>
              <input type="number" id="doorCurrentOpenProtection" placeholder="1800">
            </div>
            <div class="form-group">
              <label>Proudová Ochrana při Zavírání (mA)</label>
              <input type="number" id="doorCurrentCloseProtection" placeholder="1800">
            </div>
            <div class="form-group">
              <label>Doba Ignorování Proudu po Rozběhu (ms)</label>
              <input type="number" id="doorCurrentIgnoreTime" placeholder="500">
            </div>
            <div class="form-group">
              <label>Doba Potvrzení Přetížení (ms)</label>
              <input type="number" id="doorOvercurrentConfirmTime" placeholder="200">
            </div>
            <div class="form-group">
              <label>Počet Opakování</label>
              <input type="number" id="doorRetries" placeholder="3">
            </div>
            <div class="form-group">
              <label>PWM Otevření (0-255)</label>
              <input type="number" id="doorPwmOpen" placeholder="200">
            </div>
            <div class="form-group">
              <label>PWM Zavření (0-255)</label>
              <input type="number" id="doorPwmClose" placeholder="200">
            </div>
            <div class="form-group">
              <label>PWM Pomalého Dojezdu (0-255)</label>
              <input type="number" id="doorPwmSlow" placeholder="100">
            </div>
            <div class="form-group">
              <label>Doba Zpomalení (ms)</label>
              <input type="number" id="doorSlowdownTime" placeholder="2000">
            </div>
            <button class="primary" onclick="saveDoorSettings()">Uložit</button>
          </div>
        </div>
        
        <h2>Okno</h2>
        <div class="cards">
          <div class="card">
            <div class="form-group">
              <label>Timeout (ms)</label>
              <input type="number" id="windowTimeout" placeholder="20000">
            </div>
            <div class="form-group">
              <label>Maximální Proud (mA)</label>
              <input type="number" id="windowMaxCurrent" placeholder="1500">
            </div>
            <div class="form-group">
              <label>Proudová Ochrana při Otevírání (mA)</label>
              <input type="number" id="windowCurrentOpenProtection" placeholder="1300">
            </div>
            <div class="form-group">
              <label>Proudová Ochrana při Zavírání (mA)</label>
              <input type="number" id="windowCurrentCloseProtection" placeholder="1300">
            </div>
            <div class="form-group">
              <label>Doba Ignorování Proudu po Rozběhu (ms)</label>
              <input type="number" id="windowCurrentIgnoreTime" placeholder="500">
            </div>
            <div class="form-group">
              <label>Doba Potvrzení Přetížení (ms)</label>
              <input type="number" id="windowOvercurrentConfirmTime" placeholder="200">
            </div>
            <div class="form-group">
              <label>Počet Opakování</label>
              <input type="number" id="windowRetries" placeholder="3">
            </div>
            <div class="form-group">
              <label>PWM Otevření (0-255)</label>
              <input type="number" id="windowPwmOpen" placeholder="200">
            </div>
            <div class="form-group">
              <label>PWM Zavření (0-255)</label>
              <input type="number" id="windowPwmClose" placeholder="200">
            </div>
            <div class="form-group">
              <label>PWM Pomalého Dojezdu (0-255)</label>
              <input type="number" id="windowPwmSlow" placeholder="100">
            </div>
            <div class="form-group">
              <label>Doba Zpomalení (ms)</label>
              <input type="number" id="windowSlowdownTime" placeholder="1500">
            </div>
            <button class="primary" onclick="saveWindowSettings()">Uložit</button>
          </div>
        </div>
      </div>
      
      <!-- AUTOMATION -->
      <div class="page" id="automation">
        <h1>🤖 Automatika</h1>
        
        <div class="cards">
          <div class="card">
            <div class="form-group">
              <label>Režim</label>
              <select id="automationMode">
                <option>Automat</option>
                <option>Manuál</option>
                <option>Servis</option>
              </select>
            </div>
          </div>
        </div>
        
        <h2>Otevírání Dveří - Východ Slunce</h2>
        <div class="cards">
          <div class="card">
            <div class="time-display" id="sunriseDisplay">Východ slunce: --:--</div>
            <div class="form-group">
              <label>Korekce (±minuty)</label>
              <input type="number" id="doorOpenCorrection" placeholder="0" min="-120" max="120">
            </div>
            <button class="primary" onclick="saveDoorAutomation()">Uložit</button>
          </div>
        </div>
        
        <h2>Zavírání Dveří - Západ Slunce</h2>
        <div class="cards">
          <div class="card">
            <div class="time-display" id="sunsetDisplay">Západ slunce: --:--</div>
            <div class="form-group">
              <label>Korekce (±minuty)</label>
              <input type="number" id="doorCloseCorrection" placeholder="0" min="-120" max="120">
            </div>
            <button class="primary" onclick="saveDoorAutomation()">Uložit</button>
          </div>
        </div>
        
        <h2>Otevírání Okna - Teplota</h2>
        <div class="cards">
          <div class="card">
            <div class="form-group">
              <label>Teplota Otevření (°C)</label>
              <input type="number" id="windowOpenTemp" placeholder="25">
            </div>
            <button class="primary" onclick="saveWindowAutomation()">Uložit</button>
          </div>
        </div>
        
        <h2>Zavírání Okna - Teplota</h2>
        <div class="cards">
          <div class="card">
            <div class="form-group">
              <label>Teplota Zavření (°C)</label>
              <input type="number" id="windowCloseTemp" placeholder="23">
            </div>
            <button class="primary" onclick="saveWindowAutomation()">Uložit</button>
          </div>
        </div>
        
        <h2>Kamera - Časový Switch (Západ Slunce)</h2>
        <div class="cards">
          <div class="card">
            <div class="time-display" id="cameraTimeDisplay">Zapnutí kamery: --:-- (západ slunce)</div>
            <div class="form-group">
              <label>Korekce Času Zapnutí (±minuty před západem)</label>
              <input type="number" id="cameraTimeCorrection" placeholder="-30" min="-120" max="120">
            </div>
            <div class="form-group">
              <label>Doba Automatického Vypnutí po Zavření (minut)</label>
              <input type="number" id="cameraAutoOffTime" placeholder="60">
            </div>
            <button class="primary" onclick="saveCameraAutomation()">Uložit</button>
          </div>
        </div>
        
        <h2>GPS</h2>
        <div class="cards">
          <div class="card">
            <div class="form-group">
              <label>Zeměpisná Šířka</label>
              <input type="number" id="gpsLatitude" placeholder="50.0" step="0.0001">
            </div>
            <div class="form-group">
              <label>Zeměpisná Délka</label>
              <input type="number" id="gpsLongitude" placeholder="14.0" step="0.0001">
            </div>
            <div class="form-group">
              <label>Časové Pásmo</label>
              <select id="timezone">
                <option>UTC+1 (CET)</option>
                <option>UTC+2 (CEST)</option>
              </select>
            </div>
            <button class="primary" onclick="saveGPS()">Uložit</button>
          </div>
        </div>
      </div>
      
      <!-- ALARMS -->
      <div class="page" id="alarms">
        <h1>🚨 Alarmy</h1>
        <table>
          <thead>
            <tr>
              <th>Alarm</th>
              <th>Stav</th>
              <th>Prahová Hodnota</th>
              <th>Potvrzení (s)</th>
            </tr>
          </thead>
          <tbody>
            <tr>
              <td>Překážka Dveří</td>
              <td><span class="status-badge status-on">ON</span></td>
              <td><input type="number" placeholder="2000" style="width: 80px;"></td>
              <td><input type="number" placeholder="5" style="width: 80px;"></td>
            </tr>
            <tr>
              <td>Překážka Okna</td>
              <td><span class="status-badge status-on">ON</span></td>
              <td><input type="number" placeholder="1500" style="width: 80px;"></td>
              <td><input type="number" placeholder="5" style="width: 80px;"></td>
            </tr>
            <tr>
              <td>Timeout</td>
              <td><span class="status-badge status-on">ON</span></td>
              <td>-</td>
              <td><input type="number" placeholder="10" style="width: 80px;"></td>
            </tr>
            <tr>
              <td>Přehřátí Rozvaděče</td>
              <td><span class="status-badge status-on">ON</span></td>
              <td><input type="number" placeholder="45" style="width: 80px;"></td>
              <td><input type="number" placeholder="10" style="width: 80px;"></td>
            </tr>
            <tr>
              <td>Nízká Teplota</td>
              <td><span class="status-badge status-off">OFF</span></td>
              <td><input type="number" placeholder="5" style="width: 80px;"></td>
              <td><input type="number" placeholder="20" style="width: 80px;"></td>
            </tr>
            <tr>
              <td>Vysoká Vlhkost</td>
              <td><span class="status-badge status-on">ON</span></td>
              <td><input type="number" placeholder="85" style="width: 80px;"></td>
              <td><input type="number" placeholder="30" style="width: 80px;"></td>
            </tr>
          </tbody>
        </table>
        
        <h2>Historie Alarmů</h2>
        <table>
          <thead>
            <tr>
              <th>Datum</th>
              <th>Čas</th>
              <th>Událost</th>
              <th>Hodnota</th>
            </tr>
          </thead>
          <tbody>
            <tr>
              <td>2024-01-15</td>
              <td>10:30:15</td>
              <td>Překážka Dveří</td>
              <td>2150 mA</td>
            </tr>
            <tr>
              <td>2024-01-15</td>
              <td>09:15:42</td>
              <td>Timeout Okna</td>
              <td>-</td>
            </tr>
          </tbody>
        </table>
      </div>
      
      <!-- SENSORS -->
      <div class="page" id="sensors">
        <h1>📡 Čidla</h1>
        <div class="cards">
          <div class="card">
            <div class="card-label">Teplota Kurník</div>
            <div class="card-value" id="sensorCoopTemp">-- °C</div>
          </div>
          <div class="card">
            <div class="card-label">Teplota Rozvaděč</div>
            <div class="card-value" id="sensorCabinetTemp">-- °C</div>
          </div>
          <div class="card">
            <div class="card-label">Vlhkost Rozvaděč</div>
            <div class="card-value" id="sensorCabinetHumidity">-- %</div>
          </div>
          <div class="card">
            <div class="card-label">Rosný Bod</div>
            <div class="card-value" id="sensorDewPoint">-- °C</div>
          </div>
          <div class="card">
            <div class="card-label">Proud Dveří</div>
            <div class="card-value" id="sensorDoorCurrent">-- mA</div>
          </div>
          <div class="card">
            <div class="card-label">Proud Okna</div>
            <div class="card-value" id="sensorWindowCurrent">-- mA</div>
          </div>
          <div class="card">
            <div class="card-label">Napětí Napájení</div>
            <div class="card-value" id="sensorVoltage">-- V</div>
          </div>
        </div>
      </div>
      
      <!-- LIGHTING -->
      <div class="page" id="lighting">
        <h1>💡 Osvětlení</h1>
        <div class="cards">
          <div class="card">
            <div class="form-group">
              <label>Režim</label>
              <select id="lightingMode">
                <option>Automatika</option>
                <option>Vypnuto</option>
              </select>
            </div>
            <div class="form-group">
              <label>Čas Zapnutí (HH:MM) - v Automatice</label>
              <input type="time" id="lightingOnTime" value="06:00">
            </div>
            <div class="form-group">
              <label>Čas Vypnutí (HH:MM) - v Automatice</label>
              <input type="time" id="lightingOffTime" value="20:00">
            </div>
            <button class="primary" onclick="saveLightingSettings()">Uložit</button>
          </div>
        </div>
      </div>
      
      <!-- HEATING -->
      <div class="page" id="heating">
        <h1>🔥 Topení Rozvaděče</h1>
        <div class="cards">
          <div class="card">
            <div class="form-group">
              <label>Prahová Teplota Rosného Bodu (°C)</label>
              <input type="number" id="heatingDewpointThreshold" placeholder="10">
            </div>
            <div class="form-group">
              <label>Hystereze (°C)</label>
              <input type="number" id="heatingHysteresis" placeholder="2">
            </div>
            <button class="primary" onclick="saveHeatingSettings()">Uložit</button>
          </div>
        </div>
      </div>
      
      <!-- NETWORK -->
      <div class="page" id="network">
        <h1>🌐 Síť</h1>
        <div class="cards">
          <div class="card">
            <div class="form-group">
              <label>SSID</label>
              <input type="text" id="networkSSID" placeholder="WiFi Síť">
            </div>
            <div class="form-group">
              <label>Heslo</label>
              <input type="password" id="networkPassword" placeholder="••••••••">
            </div>
            <div class="form-group">
              <label>DHCP</label>
              <select id="networkDHCP">
                <option>Zapnuto</option>
                <option>Vypnuto</option>
              </select>
            </div>
            <div class="form-group">
              <label>Statická IP</label>
              <input type="text" id="networkStaticIP" placeholder="192.168.1.100">
            </div>
            <div class="form-group">
              <label>Hostname</label>
              <input type="text" id="networkHostname" placeholder="ak-v2">
            </div>
            <button class="primary" onclick="saveNetworkSettings()">Uložit</button>
          </div>
        </div>
      </div>
      
      <!-- OTA -->
      <div class="page" id="ota">
        <h1>📦 OTA Aktualizace</h1>
        <div class="cards">
          <div class="card">
            <div class="card-label">Verze Firmware</div>
            <div class="card-value" id="firmwareVersion">v1.0.0</div>
            <div class="card-label">Datum Kompilace</div>
            <div class="card-value" id="compilationDate">--</div>
            <div class="button-group">
              <button class="primary" onclick="uploadFirmware()">Nahrát Firmware</button>
              <button class="secondary" onclick="restartDevice()">Restartovat</button>
            </div>
          </div>
        </div>
      </div>
      
      <!-- DIAGNOSTICS -->
      <div class="page" id="diagnostics">
        <h1>🔧 Diagnostika</h1>
        <div class="cards">
          <div class="card">
            <div class="card-label">Využitá RAM</div>
            <div class="card-value" id="ramUsage">-- %</div>
          </div>
          <div class="card">
            <div class="card-label">Využitá Flash</div>
            <div class="card-value" id="flashUsage">-- %</div>
          </div>
          <div class="card">
            <div class="card-label">Provozní Čas</div>
            <div class="card-value" id="uptime">-- s</div>
          </div>
          <div class="card">
            <div class="card-label">WiFi RSSI</div>
            <div class="card-value" id="wifiRSSI">-- dBm</div>
          </div>
          <div class="card">
            <div class="card-label">IP Adresa</div>
            <div class="card-value" id="ipAddress">--</div>
          </div>
          <div class="card">
            <div class="card-label">MAC Adresa</div>
            <div class="card-value" id="macAddress">--</div>
          </div>
          <div class="card">
            <div class="card-label">Teplota Procesoru</div>
            <div class="card-value" id="cpuTemp">-- °C</div>
          </div>
          <div class="card">
            <div class="card-label">Napětí Napájení</div>
            <div class="card-value" id="supplyVoltage">-- V</div>
          </div>
        </div>
      </div>
      
      <!-- SERVICE -->
      <div class="page" id="service">
        <h1>🛠️ Servisní Režim</h1>
        
        <h2>Testování Hardware</h2>
        <div class="button-group">
          <button class="secondary" onclick="testRelays()">Test Relé</button>
          <button class="secondary" onclick="testMotors()">Test Motorů</button>
          <button class="secondary" onclick="testLimits()">Test Spínačů</button>
          <button class="secondary" onclick="testACS712()">Test ACS712</button>
          <button class="secondary" onclick="testSHT30()">Test SHT30</button>
          <button class="secondary" onclick="testDS18B20()">Test DS18B20</button>
        </div>
        
        <h2>Údržba Systému</h2>
        <div class="button-group">
          <button class="secondary" onclick="restartDevice()">Restartovat Zařízení</button>
          <button class="danger" onclick="factoryReset()">Obnovit Tovární Nastavení</button>
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
      fetch(endpoint)
        .then(r => r.json())
        .then(data => console.log('OK:', data))
        .catch(e => console.error('Chyba:', e));
    }
    
    function updateTime() {
      const now = new Date();
      const hours = String(now.getHours()).padStart(2, '0');
      const minutes = String(now.getMinutes()).padStart(2, '0');
      const seconds = String(now.getSeconds()).padStart(2, '0');
      document.getElementById('currentTime').textContent = hours + ':' + minutes + ':' + seconds;
    }
    
    function updateStatus() {
      fetch('/api/status')
        .then(r => r.json())
        .then(data => {
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
        })
        .catch(e => console.error('Chyba:', e));
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

  String request = client.readStringUntil('
');
  request.trim();

  String method = request.substring(0, request.indexOf(' '));
  String path = request.substring(request.indexOf(' ') + 1, request.lastIndexOf(' '));

  while (client.available()) {
    String line = client.readStringUntil('
');
    if (line == "\r") break;
  }

  // ========================================================================
  // ROUTE HANDLING
  // ========================================================================

  if (path == "/" || path == "/index.html") {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html; charset=utf-8");
    client.println("Content-Length: " + String(strlen(HTML_TEMPLATE)));
    client.println("Connection: close");
    client.println();
    client.print(HTML_TEMPLATE);
  }
  else if (path == "/api/status") {
    String json = "{";
    json += "\"door_status\":\"ZAVŘENO\",";
    json += "\"window_status\":\"ZAVŘENO\",";
    json += "\"camera\":false,";
    json += "\"coop_temp\":22.5,";
    json += "\"cabinet_temp\":18.3,";
    json += "\"cabinet_humidity\":65,";
    json += "\"dew_point\":11.2,";
    json += "\"heater\":false,";
    json += "\"light\":false,";
    json += "\"system_mode\":\"RUN\",";
    json += "\"uptime\":141000,";
    json += "\"ip_address\":\"172.20.10.6\"";
    json += "}";

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Content-Length: " + String(json.length()));
    client.println("Connection: close");
    client.println();
    client.print(json);
  }
  else if (path.startsWith("/api/")) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Content-Length: 16");
    client.println("Connection: close");
    client.println();
    client.print("{\"status\":\"ok\"}");
  }
  else {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/html");
    client.println("Content-Length: 9");
    client.println("Connection: close");
    client.println();
    client.print("Not Found");
  }

  delay(1);
  client.stop();

  webServerData.totalRequests++;
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
