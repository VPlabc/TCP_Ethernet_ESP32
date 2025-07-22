#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "LoRaConfig.h"
#include "LoRa.h"
LORAE32* lora = nullptr;

// Helper: Convert LoRa config to JSON
String getLoRaConfigJson() {
    String json = "{";
    json += "\"id\":" + String(lora.getNodeID()) + ",";
    json += "\"netID\":" + String(lora.getNetID()) + ",";
    json += "\"channel\":" + String(LORA_DEFAULT_CHANNEL) + ",";
    json += "\"baudrate\":" + String(LORA_DEFAULT_BAUDRATE) + ",";
    json += "\"dataSpeed\":" + String(LORA_DEFAULT_DATARATE) + ",";
    json += "\"role\":" + String(lora.getRole());
    json += "}";
    return json;
}

// API: Get LoRa config
void handleGetConfig(AsyncWebServerRequest *request) {
    request->send(200, "application/json", getLoRaConfigJson());
}

// API: Set LoRa config
void handleSetConfig(AsyncWebServerRequest *request) {
    if (request->hasParam("id", true) && request->hasParam("netID", true) &&
        request->hasParam("channel", true) && request->hasParam("baudrate", true) &&
        request->hasParam("dataSpeed", true) && request->hasParam("role", true)) {
        uint8_t id = request->getParam("id", true)->value().toInt();
        uint8_t netID = request->getParam("netID", true)->value().toInt();
        uint8_t channel = request->getParam("channel", true)->value().toInt();
        uint32_t baudrate = request->getParam("baudrate", true)->value().toInt();
        uint8_t dataSpeed = request->getParam("dataSpeed", true)->value().toInt();
        uint8_t role = request->getParam("role", true)->value().toInt();

        lora.setConfig(channel, id, netID, dataSpeed, baudrate);
        lora.setRole((Role)role);

        request->send(200, "application/json", "{\"result\":\"ok\"}");
    } else {
        request->send(400, "application/json", "{\"error\":\"Missing parameters\"}");
    }
}

// API: Ping (test data)
void handlePing(AsyncWebServerRequest *request) {
    LoRaPacket pkt;
    pkt.id = lora.getNodeID();
    pkt.netID = lora.getNetID();
    strncpy(pkt.data, "ping", sizeof(pkt.data));
    bool sent = lora.send(pkt);
    request->send(200, "application/json", String("{\"sent\":") + (sent ? "true" : "false") + "}");
}

// Web UI (simple)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>LoRa E32 Config</title>
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <style>
    body { font-family: Arial; margin: 2em; }
    label { display: block; margin-top: 1em; }
    input, select { width: 100px; }
    button { margin-top: 1em; }
    #pingResult { margin-top: 1em; color: green; }
  </style>
</head>
<body>
  <h2>LoRa E32 Configuration</h2>
  <form id="configForm">
    <label>ID: <input type="number" id="id" min="1" max="255"></label>
    <label>NetID: <input type="number" id="netID" min="1" max="255"></label>
    <label>Channel: <input type="number" id="channel" min="0" max="31"></label>
    <label>Baudrate: <input type="number" id="baudrate" value="9600"></label>
    <label>DataSpeed: <input type="number" id="dataSpeed" min="0" max="7"></label>
    <label>Role:
      <select id="role">
        <option value="0">Master</option>
        <option value="1">Node</option>
      </select>
    </label>
    <button type="submit">Save Config</button>
  </form>
  <button id="pingBtn">Ping</button>
  <div id="pingResult"></div>
  <script>
    function loadConfig() {
      fetch('/api/config').then(r=>r.json()).then(cfg=>{
        document.getElementById('id').value = cfg.id;
        document.getElementById('netID').value = cfg.netID;
        document.getElementById('channel').value = cfg.channel;
        document.getElementById('baudrate').value = cfg.baudrate;
        document.getElementById('dataSpeed').value = cfg.dataSpeed;
        document.getElementById('role').value = cfg.role;
      });
    }
    document.getElementById('configForm').onsubmit = function(e) {
      e.preventDefault();
      const data = new URLSearchParams();
      ['id','netID','channel','baudrate','dataSpeed','role'].forEach(k=>{
        data.append(k, document.getElementById(k).value);
      });
      fetch('/api/config', {method:'POST', body:data})
        .then(r=>r.json()).then(j=>{
          alert(j.result ? "Saved!" : "Error");
        });
    };
    document.getElementById('pingBtn').onclick = function() {
      fetch('/api/ping').then(r=>r.json()).then(j=>{
        document.getElementById('pingResult').textContent = j.sent ? "Ping sent!" : "Ping failed!";
      });
    };
    loadConfig();
  </script>
</body>
</html>
)rawliteral";

// Setup web server and routes
void setupLoRaWeb(AsyncWebServer& server) {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });
    server.on("/api/config", HTTP_GET, handleGetConfig);
    server.on("/api/config", HTTP_POST, handleSetConfig);
    server.on("/api/ping", HTTP_GET, handlePing);
    server.begin();
}


// Call this in your setup()
void LoRaSetup(HardwareSerial &serial , uint8_t m0, uint8_t m1, uint8_t aux ,  uint8_t chanel, uint8_t id, uint8_t netID, uint8_t dataSpeed, uint32_t baudrate) {
    lora.PinSetup(serial, m0, m1, aux);
    lora.setConfig(chanel, id, netID, dataSpeed, baudrate);
    lora = new LORAE32(Serial2, m0, m1, aux, chanel, id, netID, dataSpeed, baudrate);
    lora->setRole(LORAE32::Role::Master); // Hoặc Node
    lora->setConfig(chanel, id, netID, dataSpeed, baudrate);
}