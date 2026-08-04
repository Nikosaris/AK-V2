#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>

#if __has_include(<OneWire.h>) && __has_include(<DallasTemperature.h>)
  #include <OneWire.h>
  #include <DallasTemperature.h>
  #define AKV2_DS18B20_AVAILABLE 1
#else
  #define AKV2_DS18B20_AVAILABLE 0
#endif

constexpr const char* AP_SSID = "AKV2-SENSORS-TEST";
constexpr const char* AP_PASSWORD = "12345678";

constexpr uint8_t ONEWIRE_PIN = 4;       // Coop temperature sensor (DS18B20)
constexpr uint8_t I2C_SDA_PIN = 21;      // Cabinet SHT30 SDA
constexpr uint8_t I2C_SCL_PIN = 22;      // Cabinet SHT30 SCL
constexpr uint8_t SHT30_ADDRESS = 0x44;

float coopOffsetC = 0.0f;
float cabinetTempOffsetC = 0.0f;
float cabinetHumidityOffsetPct = 0.0f;

WebServer server(80);
#if AKV2_DS18B20_AVAILABLE
OneWire oneWire(ONEWIRE_PIN);
DallasTemperature ds18b20(&oneWire);
#endif

static bool readSHT30(float& tC, float& hPct) {
  Wire.beginTransmission(SHT30_ADDRESS);
  Wire.write(0x2C);
  Wire.write(0x06);
  if (Wire.endTransmission() != 0) return false;

  delay(20);
  if (Wire.requestFrom((int)SHT30_ADDRESS, 6) != 6) return false;

  uint16_t rawT = (Wire.read() << 8) | Wire.read();
  Wire.read();
  uint16_t rawH = (Wire.read() << 8) | Wire.read();
  Wire.read();

  tC = -45.0f + 175.0f * ((float)rawT / 65535.0f);
  hPct = 100.0f * ((float)rawH / 65535.0f);
  return true;
}

static float readCoopTempC(bool& ok) {
#if AKV2_DS18B20_AVAILABLE
  ds18b20.requestTemperatures();
  float t = ds18b20.getTempCByIndex(0);
  ok = (t > -55.0f && t < 125.0f);
  return t;
#else
  ok = false;
  return 0.0f;
#endif
}

static String sensorJson() {
  bool coopOk = false;
  float coopRaw = readCoopTempC(coopOk);

  float cabinetRawT = 0.0f, cabinetRawH = 0.0f;
  bool cabinetOk = readSHT30(cabinetRawT, cabinetRawH);

  float coopFinal = coopRaw + coopOffsetC;
  float cabFinalT = cabinetRawT + cabinetTempOffsetC;
  float cabFinalH = cabinetRawH + cabinetHumidityOffsetPct;

  String json = "{";
  json += "\"coop_ok\":" + String(coopOk ? "true" : "false") + ",";
  json += "\"coop_raw\":" + String(coopRaw, 2) + ",";
  json += "\"coop_final\":" + String(coopFinal, 2) + ",";
  json += "\"cabinet_ok\":" + String(cabinetOk ? "true" : "false") + ",";
  json += "\"cabinet_raw_temp\":" + String(cabinetRawT, 2) + ",";
  json += "\"cabinet_raw_humidity\":" + String(cabinetRawH, 2) + ",";
  json += "\"cabinet_final_temp\":" + String(cabFinalT, 2) + ",";
  json += "\"cabinet_final_humidity\":" + String(cabFinalH, 2) + ",";
  json += "\"coop_offset\":" + String(coopOffsetC, 2) + ",";
  json += "\"cabinet_temp_offset\":" + String(cabinetTempOffsetC, 2) + ",";
  json += "\"cabinet_humidity_offset\":" + String(cabinetHumidityOffsetPct, 2) + ",";
  json += "\"ds18b20_lib\":" + String(AKV2_DS18B20_AVAILABLE ? "true" : "false");
  json += "}";
  return json;
}

static const char* HTML = R"html(
<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>
<title>AK-V2 Sensor Test</title><style>body{font-family:Arial;background:#111;color:#eee;padding:16px}button{padding:10px;margin:4px}pre{background:#222;padding:12px}</style></head>
<body><h2>AK-V2: Test čidel</h2>
<p>Koop teplota + rozvaděč teplota/vlhkost + korekce.</p>
<div>
<button onclick="adj('coop',0.1)">Coop +0.1°C</button><button onclick="adj('coop',-0.1)">Coop -0.1°C</button>
<button onclick="adj('cabt',0.1)">Cab T +0.1°C</button><button onclick="adj('cabt',-0.1)">Cab T -0.1°C</button>
<button onclick="adj('cabh',1)">Cab H +1%</button><button onclick="adj('cabh',-1)">Cab H -1%</button>
</div><pre id='out'>--</pre>
<script>
async function load(){const r=await fetch('/api/status');document.getElementById('out').textContent=JSON.stringify(await r.json(),null,2)}
async function adj(k,v){await fetch('/api/offset?key='+k+'&delta='+v);load()}
setInterval(load,1500);load();
</script></body></html>
)html";

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
#if AKV2_DS18B20_AVAILABLE
  ds18b20.begin();
#endif

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  server.on("/", []() { server.send(200, "text/html", HTML); });
  server.on("/api/status", []() { server.send(200, "application/json", sensorJson()); });
  server.on("/api/offset", []() {
    String key = server.arg("key");
    float delta = server.arg("delta").toFloat();
    if (key == "coop") coopOffsetC += delta;
    else if (key == "cabt") cabinetTempOffsetC += delta;
    else if (key == "cabh") cabinetHumidityOffsetPct += delta;
    server.send(200, "application/json", "{\"ok\":true}");
  });
  server.begin();

  Serial.print("[SENSORS] AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void loop() { server.handleClient(); }
