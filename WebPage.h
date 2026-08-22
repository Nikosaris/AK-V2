#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <Arduino.h>

const char* HTML_TEMPLATE = R"rawliteral(
<!DOCTYPE html>
<html lang="cs">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>AK-V2 Kurník Kontrola</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: #1a1a1a; color: #e0e0e0; overflow-x: hidden; }
    .container { display: flex; min-height: 100vh; }
    .sidebar { width: 250px; background: #0d0d0d; padding: 20px; border-right: 1px solid #333; overflow-y: auto; position: fixed; height: 100vh; left: 0; top: 0; z-index: 1000; }
    .logo { font-size: 20px; font-weight: bold; color: #4CAF50; margin-bottom: 30px; text-align: center; }
    .menu-item { padding: 12px; margin: 5px 0; border-radius: 5px; cursor: pointer; transition: all 0.3s; font-size: 14px; border-left: 3px solid transparent; }
    .menu-item:hover { background: #2a2a2a; border-left-color: #4CAF50; }
    .menu-item.active { background: #2a5a2a; border-left-color: #4CAF50; color: #4CAF50; }
    .content { margin-left: 250px; padding: 20px; flex: 1; width: calc(100% - 250px); }
    .page { display: none; }
    .page.active { display: block; animation: fadeIn 0.3s; }
    @keyframes fadeIn { from { opacity: 0; } to { opacity: 1; } }
    h1 { margin-bottom: 20px; color: #4CAF50; font-size: 28px; }
    h2 { margin-top: 20px; margin-bottom: 15px; color: #4CAF50; font-size: 20px; }
    .cards { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin-bottom: 20px; }
    .card { background: #2a2a2a; padding: 20px; border-radius: 8px; border-left: 4px solid #4CAF50; box-shadow: 0 2px 10px rgba(0,0,0,0.3); }
    .card-label { font-size: 12px; color: #999; text-transform: uppercase; margin-bottom: 5px; }
    .card-value { font-size: 24px; font-weight: bold; color: #4CAF50; margin-bottom: 10px; }
    .button-group { display: flex; gap: 10px; margin-top: 15px; flex-wrap: wrap; }
    button { padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; font-size: 14px; font-weight: bold; transition: all 0.3s; flex: 1; min-width: 80px; }
    button.primary { background: #4CAF50; color: white; } button.danger { background: #f44336; color: white; } button.secondary { background: #666; color: white; }
    .form-group { margin-bottom: 15px; }
    label { display: block; margin-bottom: 5px; font-size: 14px; color: #999; }
    input, select { width: 100%; padding: 10px; background: #1a1a1a; border: 1px solid #444; border-radius: 4px; color: #e0e0e0; font-size: 14px; }
    table { width: 100%; border-collapse: collapse; margin-top: 15px; }
    th { background: #1a1a1a; padding: 10px; text-align: left; border-bottom: 2px solid #444; color: #4CAF50; font-size: 12px; }
    td { padding: 10px; border-bottom: 1px solid #333; }
    .status-badge { display: inline-block; padding: 5px 10px; border-radius: 3px; font-size: 12px; font-weight: bold; }
    .status-on { background: #4CAF50; color: white; } .status-off { background: #666; color: white; }
    .time-display { font-size: 16px; color: #4CAF50; margin-bottom: 5px; }
    @media (max-width: 768px) { .sidebar { width: 100%; height: auto; position: relative; border-right: none; border-bottom: 1px solid #333; } .content { margin-left: 0; width: 100%; } .cards { grid-template-columns: 1fr; } .button-group { flex-direction: column; } button { width: 100%; } }
  </style>
</head>
<body>
  <div class="container"><div class="sidebar"><div class="logo">🐔 AK-V2</div><div class="menu-item active" onclick="showPage('dashboard', this)">📊 Dashboard</div></div><div class="content"><div class="page active" id="dashboard"><h1>📊 Dashboard</h1><div class="time-display">Čas: <span id="currentTime">--:--:--</span></div><div class="cards"><div class="card"><div class="card-label">Dveře</div><div class="card-value" id="doorStatus">NAČÍTÁNÍ…</div><div class="card-label">Proud: <span id="doorCurrent">0 mA</span></div><div class="card-label">Opakování: <span id="doorRetries">0</span></div><div class="button-group"><button class="primary" onclick="apiCall('/api/door/open')">Otevřít</button><button class="danger" onclick="apiCall('/api/door/close')">Zavřít</button><button class="secondary" onclick="apiCall('/api/door/stop')">STOP</button></div></div><div class="card"><div class="card-label">Okno</div><div class="card-value" id="windowStatus">NAČÍTÁNÍ…</div><div class="card-label">Proud: <span id="windowCurrent">0 mA</span></div><div class="card-label">Opakování: <span id="windowRetries">0</span></div><div class="button-group"><button class="primary" onclick="apiCall('/api/window/open')">Otevřít</button><button class="danger" onclick="apiCall('/api/window/close')">Zavřít</button><button class="secondary" onclick="apiCall('/api/window/stop')">STOP</button></div></div></div></div></div></div>
  <script>
    function showPage() {}
    function apiCall(endpoint) { fetch(endpoint, { method: 'POST' }); }
    function updateStatus() { fetch('/api/status').then(r => r.json()).then(data => { if (document.getElementById('doorStatus')) document.getElementById('doorStatus').innerText = data.door_status || '--'; if (document.getElementById('doorCurrent')) document.getElementById('doorCurrent').innerText = (data.door_current || 0) + ' mA'; if (document.getElementById('doorRetries')) document.getElementById('doorRetries').innerText = data.door_retries || 0; if (document.getElementById('windowStatus')) document.getElementById('windowStatus').innerText = data.window_status || '--'; if (document.getElementById('windowCurrent')) document.getElementById('windowCurrent').innerText = (data.window_current || 0) + ' mA'; if (document.getElementById('windowRetries')) document.getElementById('windowRetries').innerText = data.window_retries || 0; }); }
    function updateTime() { const now = new Date(); if (document.getElementById('currentTime')) document.getElementById('currentTime').innerText = now.toLocaleTimeString('cs-CZ'); }
    setInterval(updateStatus, 3000); setInterval(updateTime, 1000); updateStatus(); updateTime();
  </script>
</body>
</html>
)rawliteral";

#endif // WEBPAGE_H
