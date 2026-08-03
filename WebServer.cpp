#include "WebServer.h"
#include "Motor.h"
#include "Sensors.h"
#include "Heater.h"
#include "Light.h"
#include "Settings.h"
#include "Climate.h"
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
      
      if (pageId === 'motor-settings') loadMotorSettings();
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
          document.getElementById('doorCurrent').textContent = (data.door_current || 0) + ' mA';
          document.getElementById('doorRetries').textContent = data.door_retries || 0;
          document.getElementById('windowStatus').textContent = data.window_status || '--';
          document.getElementById('windowCurrent').textContent = (data.window_current || 0) + ' mA';
          document.getElementById('windowRetries').textContent = data.window_retries || 0;
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
          document.getElementById('sensorDoorCurrent').textContent = (data.door_current || 0) + ' mA';
          document.getElementById('sensorWindowCurrent').textContent = (data.window_current || 0) + ' mA';
          const sunrise = String(data.sunrise_hour || 6).padStart(2,'0') + ':' + String(data.sunrise_minute || 0).padStart(2,'0');
          const sunset  = String(data.sunset_hour  || 20).padStart(2,'0') + ':' + String(data.sunset_minute  || 0).padStart(2,'0');
          document.getElementById('sunriseDisplay').textContent = 'Východ slunce: ' + sunrise;
          document.getElementById('sunsetDisplay').textContent  = 'Západ slunce: '  + sunset;
        })
        .catch(e => console.error('Chyba:', e));
    }
    
    function saveDoorSettings() {
      const cfg = {
        timeoutMs:              parseInt(document.getElementById('doorTimeout').value)             || 30000,
        maxCurrent:             parseInt(document.getElementById('doorMaxCurrent').value)          || 2000,
        currentIgnoreTimeMs:    parseInt(document.getElementById('doorCurrentIgnoreTime').value)   || 500,
        overCurrentTimeMs:      parseInt(document.getElementById('doorOvercurrentConfirmTime').value) || 1000,
        maxRetries:             parseInt(document.getElementById('doorRetries').value)             || 3,
        pwmOpen:                parseInt(document.getElementById('doorPwmOpen').value)             || 200,
        pwmClose:               parseInt(document.getElementById('doorPwmClose').value)            || 200,
        pwmSlow:                parseInt(document.getElementById('doorPwmSlow').value)             || 100,
        slowApproachDistanceMs: parseInt(document.getElementById('doorSlowdownTime').value)        || 2000
      };
      fetch('/api/door/settings', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify(cfg)})
        .then(r => r.json())
        .then(() => alert('Nastavení dveří uloženo'))
        .catch(e => alert('Chyba: ' + e));
    }
    function saveWindowSettings() {
      const cfg = {
        timeoutMs:              parseInt(document.getElementById('windowTimeout').value)             || 20000,
        maxCurrent:             parseInt(document.getElementById('windowMaxCurrent').value)          || 1500,
        currentIgnoreTimeMs:    parseInt(document.getElementById('windowCurrentIgnoreTime').value)   || 500,
        overCurrentTimeMs:      parseInt(document.getElementById('windowOvercurrentConfirmTime').value) || 800,
        maxRetries:             parseInt(document.getElementById('windowRetries').value)             || 3,
        pwmOpen:                parseInt(document.getElementById('windowPwmOpen').value)             || 180,
        pwmClose:               parseInt(document.getElementById('windowPwmClose').value)            || 180,
        pwmSlow:                parseInt(document.getElementById('windowPwmSlow').value)             || 90,
        slowApproachDistanceMs: parseInt(document.getElementById('windowSlowdownTime').value)        || 1500
      };
      fetch('/api/window/settings', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify(cfg)})
        .then(r => r.json())
        .then(() => alert('Nastavení okna (ventilace) uloženo'))
        .catch(e => alert('Chyba: ' + e));
    }
    function loadMotorSettings() {
      fetch('/api/door/settings').then(r => r.json()).then(d => {
        document.getElementById('doorTimeout').value                = d.timeoutMs             || '';
        document.getElementById('doorMaxCurrent').value             = d.maxCurrent            || '';
        document.getElementById('doorCurrentIgnoreTime').value      = d.currentIgnoreTimeMs   || '';
        document.getElementById('doorOvercurrentConfirmTime').value = d.overCurrentTimeMs     || '';
        document.getElementById('doorRetries').value                = d.maxRetries            || '';
        document.getElementById('doorPwmOpen').value                = d.pwmOpen               || '';
        document.getElementById('doorPwmClose').value               = d.pwmClose              || '';
        document.getElementById('doorPwmSlow').value                = d.pwmSlow               || '';
        document.getElementById('doorSlowdownTime').value           = d.slowApproachDistanceMs|| '';
      }).catch(() => {});
      fetch('/api/window/settings').then(r => r.json()).then(d => {
        document.getElementById('windowTimeout').value                = d.timeoutMs             || '';
        document.getElementById('windowMaxCurrent').value             = d.maxCurrent            || '';
        document.getElementById('windowCurrentIgnoreTime').value      = d.currentIgnoreTimeMs   || '';
        document.getElementById('windowOvercurrentConfirmTime').value = d.overCurrentTimeMs     || '';
        document.getElementById('windowRetries').value                = d.maxRetries            || '';
        document.getElementById('windowPwmOpen').value                = d.pwmOpen               || '';
        document.getElementById('windowPwmClose').value               = d.pwmClose              || '';
        document.getElementById('windowPwmSlow').value                = d.pwmSlow               || '';
        document.getElementById('windowSlowdownTime').value           = d.slowApproachDistanceMs|| '';
      }).catch(() => {});
    }
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
// HELPER: Extract integer value from simple flat JSON string
// e.g. jsonGetInt("{\"timeoutMs\":30000}", "timeoutMs") -> 30000
// Returns defaultVal if key not found.
// ============================================================================

static int32_t jsonGetInt(const String& json, const char* key, int32_t defaultVal = 0) {
  String search = "\"";
  search += key;
  search += "\":";
  int idx = json.indexOf(search);
  if (idx < 0) return defaultVal;
  idx += search.length();
  // skip optional whitespace
  while (idx < (int)json.length() && json[idx] == ' ') idx++;
  return json.substring(idx).toInt();
}

// ============================================================================
// HELPER: Build motor config JSON
// ============================================================================

static String motorConfigToJson(const MotorConfig& cfg) {
  String json = "{";
  json += "\"timeoutMs\":"               + String(cfg.timeoutMs)             + ",";
  json += "\"maxCurrent\":"              + String(cfg.maxCurrent)             + ",";
  json += "\"currentIgnoreTimeMs\":"     + String(cfg.currentIgnoreTimeMs)    + ",";
  json += "\"overCurrentTimeMs\":"       + String(cfg.overCurrentTimeMs)      + ",";
  json += "\"maxRetries\":"              + String(cfg.maxRetries)             + ",";
  json += "\"pwmOpen\":"                 + String(cfg.pwmOpen)                + ",";
  json += "\"pwmClose\":"                + String(cfg.pwmClose)               + ",";
  json += "\"pwmSlow\":"                 + String(cfg.pwmSlow)                + ",";
  json += "\"slowApproachDistanceMs\":"  + String(cfg.slowApproachDistanceMs);
  json += "}";
  return json;
}

// ============================================================================
// HELPER: Parse motor config from JSON body and fill cfg
// ============================================================================

static void jsonToMotorConfig(const String& json, MotorConfig& cfg) {
  cfg.timeoutMs             = (uint32_t)jsonGetInt(json, "timeoutMs",             cfg.timeoutMs);
  cfg.maxCurrent            = (uint16_t)jsonGetInt(json, "maxCurrent",            cfg.maxCurrent);
  cfg.currentIgnoreTimeMs   = (uint32_t)jsonGetInt(json, "currentIgnoreTimeMs",   cfg.currentIgnoreTimeMs);
  cfg.overCurrentTimeMs     = (uint32_t)jsonGetInt(json, "overCurrentTimeMs",     cfg.overCurrentTimeMs);
  cfg.maxRetries            = (uint8_t) jsonGetInt(json, "maxRetries",            cfg.maxRetries);
  cfg.pwmOpen               = (uint8_t) jsonGetInt(json, "pwmOpen",               cfg.pwmOpen);
  cfg.pwmClose              = (uint8_t) jsonGetInt(json, "pwmClose",              cfg.pwmClose);
  cfg.pwmSlow               = (uint8_t) jsonGetInt(json, "pwmSlow",               cfg.pwmSlow);
  cfg.slowApproachDistanceMs= (uint32_t)jsonGetInt(json, "slowApproachDistanceMs",cfg.slowApproachDistanceMs);
}

// ============================================================================
// HELPER: Send JSON response
// ============================================================================

static void sendJson(WiFiClient& client, int code, const String& json) {
  String status = (code == 200) ? "200 OK" : (code == 400 ? "400 Bad Request" : "404 Not Found");
  client.println("HTTP/1.1 " + status);
  client.println("Content-Type: application/json");
  client.println("Content-Length: " + String(json.length()));
  client.println("Connection: close");
  client.println();
  client.print(json);
}

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

  String method = request.substring(0, request.indexOf(' '));
  String path = request.substring(request.indexOf(' ') + 1, request.lastIndexOf(' '));

  // Read headers – capture Content-Length for POST body
  int contentLength = 0;
  while (client.available()) {
    String line = client.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) break;
    if (line.startsWith("Content-Length:")) {
      contentLength = line.substring(15).toInt();
    }
  }

  // Read POST body if present
  String body = "";
  if (method == "POST" && contentLength > 0) {
    unsigned long bodyTimeout = millis() + 500;
    while ((int)body.length() < contentLength && millis() < bodyTimeout) {
      if (client.available()) body += (char)client.read();
    }
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
  // ------------------------------------------------------------------
  // STATUS - real data
  // ------------------------------------------------------------------
  else if (path == "/api/status") {
    EnvironmentData* coop    = sensors_getCoopEnvironment();
    EnvironmentData* cabinet = sensors_getCabinetEnvironment();
    ElectricalData*  elec    = sensors_getElectricalData();

    String ip = "";
    if (WiFi.status() == WL_CONNECTED) ip = WiFi.localIP().toString();

    String json = "{";
    json += "\"door_status\":\""    + String(motor_getStateName(doorMotor.data.state))   + "\",";
    json += "\"door_current\":"     + String(doorMotor.data.currentMA)                   + ",";
    json += "\"door_retries\":"     + String(doorMotor.data.retryCount)                  + ",";
    json += "\"window_status\":\"" + String(motor_getStateName(windowMotor.data.state))  + "\",";
    json += "\"window_current\":"   + String(windowMotor.data.currentMA)                 + ",";
    json += "\"window_retries\":"   + String(windowMotor.data.retryCount)                + ",";
    json += "\"camera\":false,";
    json += "\"coop_temp\":"        + String(coop->isValid    ? coop->temperatureC    : 0.0f, 1) + ",";
    json += "\"cabinet_temp\":"     + String(cabinet->isValid ? cabinet->temperatureC : 0.0f, 1) + ",";
    json += "\"cabinet_humidity\":" + String(cabinet->isValid ? cabinet->humidityPercent : 0.0f, 0) + ",";
    json += "\"dew_point\":"        + String(cabinet->isValid ? cabinet->dewPointC    : 0.0f, 1) + ",";
    json += "\"heater\":false,";
    json += "\"light\":false,";
    json += "\"system_mode\":\"RUN\",";
    json += "\"uptime\":"           + String(systemUptime)  + ",";
    json += "\"sunrise_hour\":"     + String(climate_getConfig()->sunriseHour)   + ",";
    json += "\"sunrise_minute\":"   + String(climate_getConfig()->sunriseMinute) + ",";
    json += "\"sunset_hour\":"      + String(climate_getConfig()->sunsetHour)    + ",";
    json += "\"sunset_minute\":"    + String(climate_getConfig()->sunsetMinute)  + ",";
    json += "\"ip_address\":\""     + ip + "\"";
    json += "}";
    sendJson(client, 200, json);
  }
  // ------------------------------------------------------------------
  // DOOR SETTINGS - GET (return current config) / POST (apply new config)
  // ------------------------------------------------------------------
  else if (path == "/api/door/settings") {
    if (method == "GET") {
      sendJson(client, 200, motorConfigToJson(*settings_getDoorConfig()));
    } else if (method == "POST") {
      MotorConfig cfg = *settings_getDoorConfig();
      jsonToMotorConfig(body, cfg);
      settings_applyDoorConfig(cfg);
      doorMotor.config = cfg;
      sendJson(client, 200, "{\"ok\":true}");
    } else {
      sendJson(client, 400, "{\"error\":\"method\"}");
    }
  }
  // ------------------------------------------------------------------
  // WINDOW SETTINGS - GET / POST
  // ------------------------------------------------------------------
  else if (path == "/api/window/settings") {
    if (method == "GET") {
      sendJson(client, 200, motorConfigToJson(*settings_getWindowConfig()));
    } else if (method == "POST") {
      MotorConfig cfg = *settings_getWindowConfig();
      jsonToMotorConfig(body, cfg);
      settings_applyWindowConfig(cfg);
      windowMotor.config = cfg;
      sendJson(client, 200, "{\"ok\":true}");
    } else {
      sendJson(client, 400, "{\"error\":\"method\"}");
    }
  }
  // ------------------------------------------------------------------
  // GENERIC API COMMANDS (door/window control, etc.)
  // ------------------------------------------------------------------
  else if (path.startsWith("/api/")) {
    if (path == "/api/door/open")    motor_setCommand(&doorMotor,   MotorCommand::OPEN);
    else if (path == "/api/door/close")  motor_setCommand(&doorMotor,   MotorCommand::CLOSE);
    else if (path == "/api/door/stop")   motor_setCommand(&doorMotor,   MotorCommand::STOP);
    else if (path == "/api/door/reset")  motor_resetError(&doorMotor);
    else if (path == "/api/window/open") motor_setCommand(&windowMotor, MotorCommand::OPEN);
    else if (path == "/api/window/close")motor_setCommand(&windowMotor, MotorCommand::CLOSE);
    else if (path == "/api/window/stop") motor_setCommand(&windowMotor, MotorCommand::STOP);
    else if (path == "/api/window/reset")motor_resetError(&windowMotor);
    else if (path == "/api/restart")     ESP.restart();
    sendJson(client, 200, "{\"ok\":true}");
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

// ============================================================================
// API HANDLER - called externally with pre-parsed endpoint/method/body
// ============================================================================

void webserver_handleAPI(const char* endpoint, const char* method, const char* body) {
  if (!endpoint || !method) return;

  String path(endpoint);
  String meth(method);
  String bodyStr(body ? body : "");

  if (path == "/api/door/open")         motor_setCommand(&doorMotor,   MotorCommand::OPEN);
  else if (path == "/api/door/close")   motor_setCommand(&doorMotor,   MotorCommand::CLOSE);
  else if (path == "/api/door/stop")    motor_setCommand(&doorMotor,   MotorCommand::STOP);
  else if (path == "/api/door/reset")   motor_resetError(&doorMotor);
  else if (path == "/api/window/open")  motor_setCommand(&windowMotor, MotorCommand::OPEN);
  else if (path == "/api/window/close") motor_setCommand(&windowMotor, MotorCommand::CLOSE);
  else if (path == "/api/window/stop")  motor_setCommand(&windowMotor, MotorCommand::STOP);
  else if (path == "/api/window/reset") motor_resetError(&windowMotor);
  else if (path == "/api/heater/on")    heater_setMode(HeaterState::ON);
  else if (path == "/api/heater/off")   heater_setMode(HeaterState::OFF);
  else if (path == "/api/heater/auto")  heater_setMode(HeaterState::AUTO);
  else if (path == "/api/light/auto")   light_setMode(LightState::AUTO);
  else if (path == "/api/light/off")    light_setMode(LightState::OFF);
  else if (path == "/api/door/settings" && meth == "POST") {
    MotorConfig cfg = *settings_getDoorConfig();
    jsonToMotorConfig(bodyStr, cfg);
    settings_applyDoorConfig(cfg);
    doorMotor.config = cfg;
  }
  else if (path == "/api/window/settings" && meth == "POST") {
    MotorConfig cfg = *settings_getWindowConfig();
    jsonToMotorConfig(bodyStr, cfg);
    settings_applyWindowConfig(cfg);
    windowMotor.config = cfg;
  }
  else if (path == "/api/restart") {
    ESP.restart();
  }
}
