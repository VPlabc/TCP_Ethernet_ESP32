#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "ALC_Project.h"

#include <ESPAsyncWebServer.h>
// #include "LoRaConfig.h"
// AsyncWebServer server(80);


// Web: Monitor page
const char monitor_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Monitor</title>
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <style>
    body{font-family:sans-serif;}
    .card{border:1px solid #ccc;padding:16px;margin:8px;border-radius:8px;}
    .card-title{font-weight:bold;}
  </style>
</head>
<body>
  <h2>Pin State Monitor</h2>
  <div id="pinCards"></div>
  <h2>Counter Monitor</h2>
  <div id="counterCards"></div>
  <script>
    function update() {
      fetch('/status').then(r=>r.json()).then(data=>{
        let pinHtml = '';
        data.pins.forEach(p=>{
          pinHtml += `<div class="card"><div class="card-title">Pin ${p.pin}</div>State: ${p.state}</div>`;
        });
        document.getElementById('pinCards').innerHTML = pinHtml;
        let counterHtml = '';
        data.counters.forEach(c=>{
          counterHtml += `<div class="card"><div class="card-title">Counter ${c.idx}</div>Value: ${c.value} / Plan: ${c.plan}</div>`;
        });
        document.getElementById('counterCards').innerHTML = counterHtml;
      });
    }
    setInterval(update, 1000); update();
  </script>
  <a href="/config">Config</a>
</body>
</html>
)rawliteral";

// Web: Config page
const char config_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Config</title>
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <style>
    body{font-family:sans-serif;}
    .card{border:1px solid #ccc;padding:16px;margin:8px;border-radius:8px;}
    label{display:block;margin-top:8px;}
  </style>
</head>
<body>
  <h2>ESP32 Counter Config</h2>
  <form id="cfgForm">
    <div class="card">
      <label>Function Pin: <input name="funcPin" type="number" min="0" max="39" required></label>
    </div>
    <div id="counterCfg"></div>
    <button type="submit">Save</button>
  </form>
  <script>
    let NUM_COUNTERS = 4;
    function loadConfig() {
      fetch('/getcfg').then(r=>r.json()).then(cfg=>{
        document.querySelector('[name=funcPin]').value = cfg.funcPin;
        let html = '';
        for(let i=0;i<NUM_COUNTERS;i++){
          html += `<div class="card">
            <div class="card-title">Counter ${i}</div>
            <label>Pin: <input name="pin${i}" type="number" min="0" max="39" value="${cfg.counters[i].pin}" required></label>
            <label>Filter(ms): <input name="filter${i}" type="number" min="0" max="1000" value="${cfg.counters[i].filter}" required></label>
            <label>Multiple: <input name="multiple${i}" type="number" min="1" max="100" value="${cfg.counters[i].multiple}" required></label>
            <label>Plan: <input name="plan${i}" type="number" min="0" max="9999" value="${cfg.plan[i]}" required></label>
            <label>Result: <input name="result${i}" type="number" min="0" max="9999" value="${cfg.result[i]}" disabled></label> 
          </div>
          <div class="card">
            <div class="card-title">Main Config</div>
            <input type="hidden" name="id" value="${cfg.id}">
            <input type="hidden" name="netID" value="${cfg.netID}">
           </div>`;
        }
        document.getElementById('counterCfg').innerHTML = html;
      });
    }
    document.getElementById('cfgForm').onsubmit = function(e){
      e.preventDefault();
      let fd = new FormData(this);
      fetch('/setcfg', {method:'POST', body:fd}).then(r=>r.text()).then(t=>{
        alert(t);
        loadConfig();
      });
    };
    loadConfig();
  </script>
  <a href="/">Monitor</a>
</body>
</html>
)rawliteral";

const char lora_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>LoRa E32 Config</title>
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <style>
    body{font-family:sans-serif;}
    .card{border:1px solid #ccc;padding:16px;margin:8px;border-radius:8px;}
    label{display:block;margin-top:8px;}
  </style>
</head>
<body>
  <h2>LoRa E32 Config</h2>
  <form id="loraForm">
    <div class="card">
      <label>ADDH: <input name="addh" type="number" min="0" max="255" required></label>
      <label>ADDL: <input name="addl" type="number" min="0" max="255" required></label>
      <label>Channel: <input name="chan" type="number" min="0" max="83" required></label>
      <label>UART Parity: <input name="uartParity" type="number" min="0" max="2" required></label>
      <label>UART BaudRate: <input name="uartBaudRate" type="number" min="0" max="7" required></label>
      <label>Air Data Rate: <input name="airDataRate" type="number" min="0" max="7" required></label>
      <label>Fixed Transmission: <input name="fixedTransmission" type="number" min="0" max="1" required></label>
      <label>IO Drive Mode: <input name="ioDriveMode" type="number" min="0" max="1" required></label>
      <label>Wireless Wakeup Time: <input name="wirelessWakeupTime" type="number" min="0" max="7" required></label>
      <label>FEC: <input name="fec" type="number" min="0" max="1" required></label>
      <label>Transmission Power: <input name="transmissionPower" type="number" min="0" max="3" required></label>
    </div>
    <button type="submit">Save</button>
  </form>
  <script>

    function loadLoraCfg() {
      fetch('/api/get_lora_config')
        .then(res => res.json())
        .then(cfg => {
          for(let k in cfg) {
            let el = document.querySelector('[name='+k+']');
            if(el) el.value = cfg[k];
          }
        })
        .catch(err => console.error(err));
    }

    document.getElementById('loraForm').onsubmit = function(e){
      e.preventDefault();

      let fd = new FormData(this);
      fetch('/lora/setcfg', {method:'POST', body:fd}).then(r=>r.text()).then(t=>{
        alert(t);
        loadLoraCfg();
      });
    };
    loadLoraCfg();
  </script>
  <a href="/">Monitor</a>
</body>
</html>
)rawliteral";


String html = R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <title>LMD Shape Editor</title>
        <meta name="viewport" content="width=device-width,initial-scale=1">
        <style>
        body{font-family:sans-serif;}
        .card{border:1px solid #ccc;padding:16px;margin:8px;border-radius:8px;}
        label{display:block;margin-top:8px;}
        #canvasWrap{overflow:auto;}
        canvas{background:#222;display:block;}
        .btn{margin:4px;}
        </style>
      </head>
      <body>
        <h2>LMD Shape & Text Editor</h2>
        <div class="card">
        <label>Rows: <input id="rows" type="number" min="1" max="8" value="1"></label>
        <label>Cols: <input id="cols" type="number" min="1" max="8" value="1"></label>
        <button class="btn" onclick="resizeCanvas()">Apply</button>
        </div>
        <div class="card">
        <button class="btn" onclick="addShape('line')">Add Line</button>
        <button class="btn" onclick="addShape('rect')">Add Rect</button>
        <button class="btn" onclick="addShape('round')">Add Round</button>
        <button class="btn" onclick="addShape('triangle')">Add Triangle</button>
        <button class="btn" onclick="addText()">Add Text</button>
        </div>
        <div class="card">
      <label>Color: <input id="color" type="color" value="#00ff00"></label>
      <label>Radius (for round): <input id="radius" type="number" min="1" max="32" value="5"></label>
        </div>
        <div id="canvasWrap">
        <canvas id="lmdCanvas" width="64" height="32"></canvas>
        </div>
        <div class="card">
        <button class="btn" onclick="saveLMD()">Save</button>
        <button class="btn" onclick="loadLMD()">Load</button>
        </div>
        <script>
        let shapes = [];
        let rows = 1, cols = 1;
        let moduleW = 64, moduleH = 32;
        let canvas = document.getElementById('lmdCanvas');
        let ctx = canvas.getContext('2d');

        function getColor() {
      return document.getElementById('color').value || "#0f0";
        }
        function getRadius() {
      return parseInt(document.getElementById('radius').value) || 5;
        }

        function resizeCanvas() {
      rows = parseInt(document.getElementById('rows').value);
      cols = parseInt(document.getElementById('cols').value);
      canvas.width = cols * moduleW;
      canvas.height = rows * moduleH;
      drawAll();
        }

        function addShape(type) {
      let s = {type:type, color:getColor()};
      if(type=='line') {
        s.x=10; s.y=10; s.w=40; s.h=0;
      } else if(type=='rect') {
        s.x=5; s.y=5; s.w=20; s.h=10;
      } else if(type=='round') {
        s.x=32; s.y=16; s.radius=getRadius();
      } else if(type=='triangle') {
        s.x=10; s.y=30; s.x1=32; s.y1=2; s.x2=54; s.y2=30;
      }
      shapes.push(s);
      drawAll();
        }

        function addText() {
      let x = prompt("Text X?", "10");
      let y = prompt("Text Y?", "20");
      let content = prompt("Text Content?", "Hello");
      if(x && y && content) {
        shapes.push({type:'text', x:parseInt(x), y:parseInt(y), content:content, color:getColor()});
        drawAll();
      }
        }

        function drawAll() {
      ctx.clearRect(0,0,canvas.width,canvas.height);
      for(let s of shapes) {
        ctx.save();
        ctx.strokeStyle = s.color || "#0f0";
        ctx.fillStyle = s.color || "#0f0";
        if(s.type=='line') {
          ctx.beginPath();
          ctx.moveTo(s.x, s.y);
          ctx.lineTo(s.x+s.w, s.y+s.h);
          ctx.stroke();
        } else if(s.type=='rect') {
          ctx.strokeRect(s.x, s.y, s.w, s.h);
        } else if(s.type=='round') {
          ctx.beginPath();
          ctx.arc(s.x, s.y, s.radius || getRadius(), 0, 2*Math.PI);
          ctx.stroke();
        } else if(s.type=='triangle') {
          ctx.beginPath();
          ctx.moveTo(s.x, s.y);
          ctx.lineTo(s.x1, s.y1);
          ctx.lineTo(s.x2, s.y2);
          ctx.closePath();
          ctx.stroke();
        } else if(s.type=='text') {
          ctx.font = "12px sans-serif";
          ctx.fillText(s.content, s.x, s.y);
        }
        ctx.restore();
      }
        }

        function saveLMD() {
      fetch('/lmd/save', {
        method:'POST',
        headers:{'Content-Type':'application/json'},
        body:JSON.stringify({rows,cols,shapes})
      }).then(r=>r.text()).then(t=>alert(t));
        }

        function loadLMD() {
      fetch('/lmd/load').then(r=>r.json()).then(data=>{
        rows = data.rows; cols = data.cols;
        document.getElementById('rows').value = rows;
        document.getElementById('cols').value = cols;
        shapes = data.shapes || [];
        resizeCanvas();
      });
        }

        resizeCanvas();
        </script>
        <a href="/">Monitor</a>
      </body>
      </html>
      )rawliteral";
      
#include "ModuleLoRaE32.h"
M_LoRa_E32 loraE32s;

// #include "LMD.h"

// ==== Global Variables ====
ProjectConfig gConfig;

// Helper: Get JSON status for all pins/counters
String getStatusJson() {
    StaticJsonDocument<512> doc;
    JsonArray pins = doc.createNestedArray("pins");
    for (int i = 0; i < NUM_COUNTERS; i++) {
        JsonObject p = pins.createNestedObject();
        p["pin"] = gConfig.counters[i].pin;
        p["state"] = digitalRead(gConfig.counters[i].pin);
    }
    JsonArray counters = doc.createNestedArray("counters");
    for (int i = 0; i < NUM_COUNTERS; i++) {
        JsonObject c = counters.createNestedObject();
        c["idx"] = i;
        c["value"] = gConfig.result[i];
        c["plan"] = gConfig.plan[i];
    }
    String out;
    serializeJson(doc, out);
    return out;
}

// Web: API handlers
void setupWebServer(AsyncWebServer &server) {
    server.on("/monitor", HTTP_GET, [](AsyncWebServerRequest *req) {
        req->send_P(200, "text/html", monitor_html);
    });
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *req) {
        req->send(200, "application/json", getStatusJson());
    });
    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *req) {
        req->send_P(200, "text/html", config_html);
    });
    server.on("/getcfg", HTTP_GET, [](AsyncWebServerRequest *req) {
        StaticJsonDocument<1024> doc;
        doc["funcPin"] = gConfig.funcPin;
        for (int i = 0; i < NUM_COUNTERS; i++) {
            doc["result"][i] = gConfig.result[i];
            doc["plan"][i] = gConfig.plan[i];
            doc["id"] = gConfig.id;
            doc["netID"] = gConfig.netID;
            doc["channel"] = gConfig.channel;
            doc["counters"][i]["pin"] = gConfig.counters[i].pin;
            doc["counters"][i]["filter"] = gConfig.counters[i].filter;
            doc["counters"][i]["multiple"] = gConfig.counters[i].multiple;
        }
        String out;
        serializeJson(doc, out);
        req->send(200, "application/json", out);
    });
    server.on("/setcfg", HTTP_POST, [](AsyncWebServerRequest *req) {
        if (req->hasParam("funcPin", true)) {
            gConfig.funcPin = req->getParam("funcPin", true)->value().toInt();
            for (int i = 0; i < NUM_COUNTERS; i++) {
                gConfig.counters[i].pin = req->getParam("pin" + String(i), true)->value().toInt();
                gConfig.counters[i].filter = req->getParam("filter" + String(i), true)->value().toInt();
                gConfig.counters[i].multiple = req->getParam("multiple" + String(i), true)->value().toInt();
                gConfig.plan[i] = req->getParam("plan" + String(i), true)->value().toInt();
                gConfig.result[i] = 0; // Reset result on config change
                gConfig.id = req->getParam("id", true)->value().toInt();
                gConfig.netID = req->getParam("netID", true)->value().toInt();
                gConfig.channel = req->getParam("channel", true)->value().toInt();
            }
            saveProjectConfig(gConfig);
            req->send(200, "text/plain", "Config saved. Reboot ESP32 for pin changes to take effect.");
        } else {
            req->send(400, "text/plain", "Invalid config");
        }
    });
    
    // Web: Lora E32 config page
    server.on("/lora", HTTP_GET, [](AsyncWebServerRequest *req) {
      req->send_P(200, "text/html", lora_html);
    });

    // API: Save LoRa E32 config
    server.on("/lora/setcfg", HTTP_POST, [](AsyncWebServerRequest *req) {
      if (
        req->hasParam("addh", true) && req->hasParam("addl", true) &&
        req->hasParam("chan", true) && req->hasParam("uartParity", true) &&
        req->hasParam("uartBaudRate", true) && req->hasParam("airDataRate", true) &&
        req->hasParam("fixedTransmission", true) && req->hasParam("ioDriveMode", true) &&
        req->hasParam("wirelessWakeupTime", true) && req->hasParam("fec", true) &&
        req->hasParam("transmissionPower", true)
      ) {
            StaticJsonDocument<256> json;
            json["addh"] = req->getParam("addh", true)->value().toInt();
            json["addl"] = req->getParam("addl", true)->value().toInt();
            json["chan"] = req->getParam("chan", true)->value().toInt();
            json["uartParity"] = req->getParam("uartParity", true)->value().toInt();
            json["uartBaudRate"] = req->getParam("uartBaudRate", true)->value().toInt();
            json["airDataRate"] = req->getParam("airDataRate", true)->value().toInt();
            json["fixedTransmission"] = req->getParam("fixedTransmission", true)->value().toInt();
            json["ioDriveMode"] = req->getParam("ioDriveMode", true)->value().toInt();
            json["wirelessWakeupTime"] = req->getParam("wirelessWakeupTime", true)->value().toInt();
            json["fec"] = req->getParam("fec", true)->value().toInt();
            json["transmissionPower"] = req->getParam("transmissionPower", true)->value().toInt();

            String jsonString;
            serializeJson(json, jsonString);

        // Save config to LoRa module
        loraE32s.SetConfigFromJson(jsonString);

        req->send(200, "text/plain", "LoRa config saved.");
      } else {
        req->send(400, "text/plain", "Invalid LoRa config");
      }
    });
    // API: Load LoRa E32 config page
    server.on("/api/get_lora_config", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = loraE32s.GetJsonConfig();
        request->send(200, "application/json", json);
    });

    // LoRa command endpoint
    server.on("/lora/cmnd", HTTP_GET, [](AsyncWebServerRequest *req) {
        if (req->hasParam("cmd")) {
            String cmd = req->getParam("cmd")->value();
            loraE32s.CMND(cmd);
            req->send(200, "text/plain", "Command sent: " + cmd);
        } else {
            req->send(400, "text/plain", "Missing command parameter");
        }
    });

    /////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////// LMD \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
    // LMD: Shape Editor Page
    server.on("/lmd", HTTP_GET, [](AsyncWebServerRequest *req) {
      req->send(200, "text/html", html);
    });

    // API: Save LMD shape/text config
    server.on("/lmd/save", HTTP_POST, [](AsyncWebServerRequest *req) {
      String body;
      if (req->hasParam("plain", true)) {
        body = req->getParam("plain", true)->value();
      } else {
        req->send(400, "text/plain", "Missing body");
        return;
      }
      File f = LittleFS.open("/lmd.json", "w");
      if (!f) {
        req->send(500, "text/plain", "Save failed");
        return;
      }
      Serial.println("Saving LMD config: " + body);
      f.print(body);
      f.close();
      req->send(200, "text/plain", "LMD config saved");
    });

    // API: Load LMD shape/text config
    server.on("/lmd/load", HTTP_GET, [](AsyncWebServerRequest *req) {
      File f = LittleFS.open("/lmd.json", "r");
      if (!f) {
        req->send(404, "application/json", "{\"rows\":1,\"cols\":1,\"shapes\":[]}");
        return;
      }
      String json = f.readString();
      f.close();
      req->send(200, "application/json", json);
    });
#ifdef USE_LMD
    // API: Draw LMD shapes from file
    server.on("/lmd/draw", HTTP_POST, [](AsyncWebServerRequest *req) {
      File f = LittleFS.open("/lmd.json", "r");
      if (!f) {
      req->send(404, "text/plain", "No LMD config found");
      return;
      }
      String json = f.readString();
      f.close();

      StaticJsonDocument<2048> doc;
      DeserializationError err = deserializeJson(doc, json);
      if (err) {
      req->send(400, "text/plain", "Invalid LMD config");
      return;
      }

      JsonArray shapes = doc["shapes"].as<JsonArray>();
      for (JsonVariant s : shapes) {
      String type = s["type"] | "";
      if (type == "line") {
        DrawLine(
          static_cast<int>(s["x"] | 0),
          static_cast<int>(s["y"] | 0),
          static_cast<int>(s["x"] | 0) + static_cast<int>(s["w"] | 0),
          static_cast<int>(s["y"] | 0) + static_cast<int>(s["h"] | 0),
          myWHITE // thickness or color, adjust as needed
        );
      } else if (type == "rect") {
        DrawRect(
          static_cast<int>(s["x"] | 0),
          static_cast<int>(s["y"] | 0),
          static_cast<int>(s["x"] | 0) + static_cast<int>(s["w"] | 0),
          static_cast<int>(s["y"] | 0) + static_cast<int>(s["h"] | 0),
          myWHITE // thickness or color, adjust as needed
        );
      } else if (type == "round") {
        DrawRound(
          static_cast<int>(s["x"] | 0),
          static_cast<int>(s["y"] | 0),
          static_cast<int>(s["x"] | 0) + static_cast<int>(s["w"] | 0),
          static_cast<int>(s["y"] | 0) + static_cast<int>(s["h"] | 0),
          1, // radius or size, adjust as needed
          myWHITE // thickness or color, adjust as needed
        );
      } else if (type == "triangle") {
        DrawTriangle(
          static_cast<int>(s["x"] | 0),
          static_cast<int>(s["y"] | 0),
          static_cast<int>(s["x1"] | 0),
          static_cast<int>(s["y1"] | 0),
          static_cast<int>(s["x2"] | 0),
          static_cast<int>(s["y2"] | 0),
          myWHITE // thickness or color, adjust as needed
        );
      }
      else if (type == "text") {
        DrawText(
          static_cast<int>(s["x"] | 0),
          static_cast<int>(s["y"] | 0),
          myWHITE, // font or style, adjust as needed
          s["content"].as<const char*>() // ensure correct type
        );
      }
      // You can add text drawing here if needed
      }
      req->send(200, "text/plain", "LMD shapes drawn");
    });
    #endif // USE_LMD
    server.begin();
}
// ==== Save/Load Functions ====
bool saveProjectConfig(const ProjectConfig &cfg) {
    File file = LittleFS.open(CONFIG_FILE, "w");
    if (!file) return false;

    StaticJsonDocument<1024> doc;
    for (int i = 0; i < NUM_COUNTERS; i++) {
        doc["plan"][i] = cfg.plan[i];
        doc["result"][i] = cfg.result[i];
        doc["counters"][i]["pin"] = cfg.counters[i].pin;
        doc["counters"][i]["filter"] = cfg.counters[i].filter;
        doc["counters"][i]["multiple"] = cfg.counters[i].multiple;
    }
    doc["funcPin"] = cfg.funcPin;
    doc["id"] = cfg.id;
    doc["netID"] = cfg.netID;  
    doc["channel"] = cfg.channel;
    doc["LoRaPin"][0] = cfg.LoRaPin[0];
    doc["LoRaPin"][1] = cfg.LoRaPin[1];
    doc["LoRaPin"][2] = cfg.LoRaPin[2];
    doc["LoRaPin"][3] = cfg.LoRaPin[3];
    doc["LoRaPin"][4] = cfg.LoRaPin[4];

    bool ok = serializeJson(doc, file) > 0;
    file.close();
    return ok;
}

bool loadProjectConfig(ProjectConfig &cfg) {
    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file) return false;

    StaticJsonDocument<1024> doc;
    DeserializationError err = deserializeJson(doc, file);
    file.close();
    if (err) return false;

    for (int i = 0; i < NUM_COUNTERS; i++) {
        cfg.plan[i] = doc["plan"][i] | 0;
        cfg.result[i] = doc["result"][i] | 0;
        cfg.counters[i].pin = doc["counters"][i]["pin"] | 0;
        cfg.counters[i].filter = doc["counters"][i]["filter"] | 0;
        cfg.counters[i].multiple = doc["counters"][i]["multiple"] | 1;
    }
    cfg.funcPin = doc["funcPin"] | 0;
    cfg.id = doc["id"] | 0;
    cfg.netID = doc["netID"] | 0;
    cfg.channel = doc["channel"] | 0;
    cfg.LoRaPin[0] = doc["LoRaPin"][0] | 0;
    cfg.LoRaPin[1] = doc["LoRaPin"][1] | 0;
    cfg.LoRaPin[2] = doc["LoRaPin"][2] | 0;
    cfg.LoRaPin[3] = doc["LoRaPin"][3] | 0;
    cfg.LoRaPin[4] = doc["LoRaPin"][4] | 0;
    // LoRaSetup()
    return true;
}

// ==== Example Counter Handler ====
volatile uint32_t counterValue[NUM_COUNTERS] = {0};
unsigned long lastTrigger[NUM_COUNTERS] = {0};

// void IRAM_ATTR counterISR(int idx) {
//     unsigned long now = millis();
//     if (now - lastTrigger[idx] >= gConfig.counters[idx].filter) {
//         lastTrigger[idx] = now;
//         counterValue[idx] += gConfig.counters[idx].multiple;
//         gConfig.result[idx] = min((uint16_t)counterValue[idx], (uint16_t)9999);
//     }
// }

void setupCounters() {
    // for (int i = 0; i < NUM_COUNTERS; i++) {
    //     pinMode(gConfig.counters[i].pin, INPUT_PULLUP);
    //     attachInterruptArg(
    //         gConfig.counters[i].pin,
    //         [](void *arg) { counterISR((int)(intptr_t)arg); },
    //         (void *)(intptr_t)i,
    //         FALLING
    //     );
    // }
}

// ==== Setup/Loop Example ====
void ALC_setup(AsyncWebServer &server) {
    Serial.begin(115200);
    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed!");
        return;
    }
    if (!loadProjectConfig(gConfig)) {
        Serial.println("Load config failed, using defaults.");
        memset(&gConfig, 0, sizeof(gConfig));
        for (int i = 0; i < NUM_COUNTERS; i++) {
            gConfig.counters[i].pin = 2 + i;
            gConfig.counters[i].filter = 50;
            gConfig.counters[i].multiple = 1;
        }
        gConfig.funcPin = 15;
        saveProjectConfig(gConfig);
    }
    // setupCounters();
    Serial.println("Ram :" + String(ESP.getFreeHeap()/1024 )+ " KB");
    setupWebServer(server);
    Serial.println("Ram :" + String(ESP.getFreeHeap()/1024 )+ " KB");
    

    // Initialize LoRa
    // loraE32s.initLoRaE32();
    
    Serial.println("Ram :" + String(ESP.getFreeHeap()/1024 )+ " KB");
    
    // setup_ledPanel();
    // if (!dma_display || !virtualDisp) {
    // Serial.println("Panel not initialized!");
    // return;
    // }
    
    // Serial.println("Ram :" + String(ESP.getFreeHeap()/1024 )+ " KB");
}
void ALC_loop() {
    // Your main logic here (web config, admin monitor, etc.)
    // delay(1000);
}