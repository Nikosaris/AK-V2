#include <WiFi.h>
#include <WebServer.h>

constexpr const char* AP_SSID = "AKV2-DOOR-TEST";
constexpr const char* AP_PASSWORD = "12345678";

constexpr uint8_t DOOR_IN1_PIN = 13;
constexpr uint8_t DOOR_IN2_PIN = 15;
constexpr uint32_t SAFETY_STOP_MS = 12000;

WebServer server(80);
unsigned long motionStartMs = 0;
String motionState = "STOP";

static void doorStop() {
  digitalWrite(DOOR_IN1_PIN, LOW);
  digitalWrite(DOOR_IN2_PIN, LOW);
  motionState = "STOP";
  motionStartMs = 0;
}

static void doorOpen() {
  digitalWrite(DOOR_IN1_PIN, HIGH);
  digitalWrite(DOOR_IN2_PIN, LOW);
  motionState = "OPEN";
  motionStartMs = millis();
}

static void doorClose() {
  digitalWrite(DOOR_IN1_PIN, LOW);
  digitalWrite(DOOR_IN2_PIN, HIGH);
  motionState = "CLOSE";
  motionStartMs = millis();
}

static String statusJson() {
  String json = "{";
  json += "\"state\":\"" + motionState + "\",";
  json += "\"runtime_ms\":" + String(motionStartMs == 0 ? 0 : millis() - motionStartMs);
  json += "}";
  return json;
}

static const char* HTML = R"html(
<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>
<title>AK-V2 Door Manual Test</title><style>body{font-family:Arial;background:#111;color:#eee;padding:16px}button{padding:12px;margin:4px;min-width:90px}pre{background:#222;padding:12px}</style></head>
<body><h2>AK-V2: Ruční test dveří</h2><p>Pouze OPEN/CLOSE/STOP přes web tlačítka.</p>
<div><button onclick="api('/api/door/open')">OPEN</button><button onclick="api('/api/door/close')">CLOSE</button><button onclick="api('/api/door/stop')">STOP</button></div>
<pre id='out'>--</pre>
<script>
async function api(u){await fetch(u);load()}
async function load(){const r=await fetch('/api/status');document.getElementById('out').textContent=JSON.stringify(await r.json(),null,2)}
setInterval(load,500);load();
</script></body></html>
)html";

void setup() {
  Serial.begin(115200);
  pinMode(DOOR_IN1_PIN, OUTPUT);
  pinMode(DOOR_IN2_PIN, OUTPUT);
  doorStop();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  server.on("/", []() { server.send(200, "text/html", HTML); });
  server.on("/api/status", []() { server.send(200, "application/json", statusJson()); });
  server.on("/api/door/open", []() { doorOpen(); server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/api/door/close", []() { doorClose(); server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/api/door/stop", []() { doorStop(); server.send(200, "application/json", "{\"ok\":true}"); });
  server.begin();

  Serial.print("[DOOR] AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();
  if (motionStartMs > 0 && (millis() - motionStartMs) > SAFETY_STOP_MS) {
    doorStop();
  }
}
