
#include <EEPROM.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#include <ESPAsyncWebServer.h>
// #include <ETH.h>
#include <WiFi.h>
#include <time.h>


// Add Info page
const char index_info[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Info</title>
  <meta charset="utf-8">
  <style>
    body { font-family: Arial,sans-serif; background: #f4f6fb; margin:0; }
    .card { background:#fff; border-radius:10px; box-shadow:0 2px 8px #0001; margin:24px auto; max-width:420px; padding:24px; }
    .gauge-bg { width:100%; height:24px; background:#eee; border-radius:12px; margin-bottom:8px; }
    .gauge-bar { height:24px; background:#0078d7; border-radius:12px; transition:width 0.5s; }
    .label { font-weight:bold; margin-bottom:4px; }
    .value { float:right; }
    .section-title { font-size:1.1em; margin:18px 0 8px 0; color:#0078d7; }
    .row { margin-bottom:8px; }
    .container {
      max-width: 420px;
      margin: 40px auto;
      background: #fff;
      border-radius: 12px;
      box-shadow: 0 2px 12px rgba(0,0,0,0.08);
      padding: 32px 28px 24px 28px;
    }
    h1 {
      text-align: center;
      color: #2c3e50;
      margin-bottom: 12px;
    }
    p {
      text-align: center;
      color: #555;
      margin-bottom: 28px;
    }
    label {
      display: block;
      margin-top: 16px;
      margin-bottom: 6px;
      color: #34495e;
      font-weight: 500;
    }
    input[type="text"], input[type="number"], select {
      width: 100%;
      padding: 8px 10px;
      border: 1px solid #d0d7de;
      border-radius: 5px;
      font-size: 1em;
      background: #f8fafc;
      margin-bottom: 2px;
      box-sizing: border-box;
    }
    button {
      margin-top: 22px;
      width: 100%;
      padding: 10px 0;
      background: #0078d7;
      color: #fff;
      border: none;
      border-radius: 6px;
      font-size: 1.1em;
      font-weight: 600;
      cursor: pointer;
      transition: background 0.2s;
    }
    button:hover {
      background: #005fa3;
    }
    .info {
      margin-top: 18px;
      padding: 10px;
      background: #eaf6ff;
      border-left: 4px solid #0078d7;
      color: #333;
      font-size: 0.97em;
      border-radius: 4px;
    }
  </style>
  </style>
</head>
<body>
    <div class="card">
        <h1>Hub RS232 Info</h1>
        <p>Current status and configuration</p>
    </div>
  <div class="card">
    <div class="section-title">Free RAM</div>
    <div class="gauge-bg">
      <div id="ramGauge" class="gauge-bar" style="width:0%"></div>
    </div>
    <div class="row">
      <span class="label">Free:</span>
      <span id="freeRam" class="value"></span>
    </div>
    <div class="row">
      <span class="label">Total:</span>
      <span id="totalRam" class="value"></span>
    </div>
    <div class="section-title">Run Info</div>
    <div class="row"><span class="label">Uptime:</span><span id="uptime" class="value"></span></div>
    <div class="row"><span class="label">Reset Count:</span><span id="resetCount" class="value"></span></div>
    <div class="section-title">Hub RS232</div>
    <div class="row"><span class="label">Baudrate:</span><span id="rs232Baud" class="value"></span></div>
    <div class="row"><span class="label">Mode:</span><span id="tcpMode" class="value"></span></div>
    <div class="section-title">Ethernet</div>
    <div class="row"><span class="label">TCP Port:</span><span id="tcpPort" class="value"></span></div>
    <div class="row"><span class="label">IP Address:</span><span id="ethIP" class="value"></span></div>
    <div class="row"><span class="label">Gateway:</span><span id="ethGW" class="value"></span></div>
    <div class="row"><span class="label">Subnet:</span><span id="ethSN" class="value"></span></div>
    <div class="row"><span class="label">DNS1:</span><span id="ethDNS1" class="value"></span></div>
    <div class="row"><span class="label">DNS2:</span><span id="ethDNS2" class="value"></span></div>
  </div>
  <div class="container">
    <h1>Control Panel</h1>
        <button onclick="location.href='/console'">View Console</button>
        <button onclick="location.href='/history'">View History</button>
        <button onclick="fetch('/api/reset').then(r=>r.text()).then(alert)">Reset</button> 
        <input type="number" id="cycleTime" placeholder="Cycle time (ms)" value="1000" min="100" max="10000">
        <input type="button" value="Cycle time" onclick="CycleTime()">
    </div>  
  <script>
    
    function onload(event) {
        handleChangeShapeType();

        initWebSocket();
    }

    function initWebSocket() {
        console.log('Trying to open a WebSocket connection…');
        websocket = new WebSocket('ws://' + location.host + '/ws_info');
        websocket.onopen = onOpen;
        websocket.onclose = onClose;
        websocket.onmessage = onMessage;
    }
    function reconnectWebSocket() {
        if (websocket) {
            websocket.close();
        }
        initWebSocket();
    }
    function onOpen() {
        console.log("WebSocket opened!");
        alert("WebSocket opened!");    
    }
    function onClose() {
        console.log("WebSocket closed!");
        // Có thể thử reconnect nếu cần
        setTimeout(initWebSocket, 1000); // Thử reconnect sau 1 giây
    }
    function onMessage() {
        const info = JSON.parse(event.data);
        document.getElementById('ramGauge').style.width = (info.free_ram / info.total_ram * 100) + '%';
        document.getElementById('freeRam').textContent = info.free_ram + ' bytes';
        document.getElementById('totalRam').textContent = info.total_ram + ' bytes';
        document.getElementById('uptime').textContent = info.uptime;
        document.getElementById('resetCount').textContent = info.reset_count;
        document.getElementById('rs232Baud').textContent = info.rs232_baudrate;
        document.getElementById('tcpMode').textContent = info.tcp_mode;
        document.getElementById('tcpPort').textContent = info.tcp_port;
        document.getElementById('ethIP').textContent = info.eth_ip;
        document.getElementById('ethGW').textContent = info.eth_gw;
        document.getElementById('ethSN').textContent = info.eth_sn;
        document.getElementById('ethDNS1').textContent = info.eth_dns1;
        document.getElementById('ethDNS2').textContent = info.eth_dns2;
    };
    
  </script>
</body>
</html>
)rawliteral";


// EEPROM address for reset counter
#define EEPROM_SIZE 8
#define RESET_COUNT_ADDR 0

// Store start time
unsigned long startMillis = 0;
uint32_t resetCount = 0;
uint16_t BaudRate = 115200; // Default baudrate
enum TCPConnectionStatus {
  TCP_DISCONNECTED,
  TCP_CONNECTED
};

enum EthernetStatus {
  ETH_DISCONNECTED,
  ETH_CONNECTED
};

TCPConnectionStatus tcpStatus = TCP_DISCONNECTED;
EthernetStatus ethStatus = ETH_DISCONNECTED;
uint16_t webserverPort = 80;


// Gửi thông tin qua socket (WebSocket)
AsyncWebSocket wsInfo("/ws_info");

// Gọi trong loop:
void infoSocketLoop();
// Gọi trong setup:
void setupInfoSocketAndTimer();
// Call this in setup
void initResetCount();
void sendInfoJson();
// Call this in setup
void initResetCount();
// Helper to format uptime as "dd | hh:mm:ss"
String formatUptime(unsigned long ms) {
  unsigned long s = ms / 1000;
  unsigned long d = s / 86400;
  s %= 86400;
  unsigned long h = s / 3600;
  s %= 3600;
  unsigned long m = s / 60;
  s %= 60;
  char buf[32];
  snprintf(buf, sizeof(buf), "%lu | %02lu:%02lu:%02lu", d, h, m, s);
  return String(buf);
}
bool TCPMode = false; // true for server, false for client
uint16_t TCPport = 5000; // Default TCP port
IPAddress ethIP(0, 0, 0, 0); // Default IP
IPAddress ethGW(0, 0, 0, 0); // Default Gateway
IPAddress ethSN(255, 255, 255, 0); // Default Subnet Mask
IPAddress ethDNS1(8, 8, 8, 8); // Default DNS
IPAddress ethDNS2(8, 8, 4, 4); // Default DNS2

void updateInfoSocket(bool tcpMode, uint16_t tcpPort,uint16_t webPort = 80, 
                     IPAddress ethIP = IPAddress(0,0,0,0), 
                     IPAddress ethGW = IPAddress(0,0,0,0), 
                     IPAddress ethSN = IPAddress(255,255,255,0), 
                     IPAddress ethDNS1 = IPAddress(8,8,8,8), 
                     IPAddress ethDNS2 = IPAddress(8,8,4,4),
                    uint16_t baudRate = 115200) {
    TCPMode = tcpMode;
    TCPport = tcpPort;
    webserverPort = webPort;
    ethIP = ethIP;
    ethGW = ethGW;
    ethSN = ethSN;
    ethDNS1 = ethDNS1;
    ethDNS2 = ethDNS2;
    BaudRate = baudRate;
}
// Helper to get free heap
uint32_t getFreeHeap() {
  return ESP.getFreeHeap();
}
uint32_t getTotalHeap() {
  return ESP.getHeapSize();
}

// Helper to get RS232 baudrate (assuming Serial)
uint32_t getRS232Baudrate() {
  return BaudRate;
}

// Helper to get TCP mode
String getTCPMode() {
    if (TCPMode) {
        return "Server";
    }
    else {
        return "Client";
    }
}

// Helper to get Ethernet info
IPAddress getEthIP() {
  return ethIP;
}
IPAddress getEthGW() {
  return ethGW;
}
IPAddress getEthSN() {
  return ethSN;
}
IPAddress getEthDNS1() {
  return ethDNS1;
}
IPAddress getEthDNS2() {
  return ethDNS2;
}
uint16_t getTCPPort() {
  return TCPport;
}

// Call this when TCP connects/disconnects
void setTCPStatus(bool connected) {
  tcpStatus = connected ? TCP_CONNECTED : TCP_DISCONNECTED;
}

// Call this when Ethernet connects/disconnects
void setEthStatus(bool connected) {
  ethStatus = connected ? ETH_CONNECTED : ETH_DISCONNECTED;
}

// Helper to get TCP status as string
String getTCPStatusStr() {
  return (tcpStatus == TCP_CONNECTED) ? "Connected" : "Disconnected";
}

// Helper to get Ethernet status as string
String getEthStatusStr() {
  return (ethStatus == ETH_CONNECTED) ? "Connected" : "Disconnected";
}

// Helper to get webserver port
uint16_t getWebserverPort() {
  return webserverPort;
}
// EEPROM: Read/Write reset count
void loadResetCount() {
  EEPROM.begin(EEPROM_SIZE);
  resetCount = EEPROM.readUInt(RESET_COUNT_ADDR);
}
void saveResetCount() {
  EEPROM.writeUInt(RESET_COUNT_ADDR, resetCount);
  EEPROM.commit();
}

// Call this in setup
void initResetCount() {
  loadResetCount();
  resetCount++;
  saveResetCount();
}

// Info API with cycletime setting and socket broadcast

uint32_t infoCycleTime = 1000; // default 1000ms
#define INFO_CYCLETIME_ADDR 4 // EEPROM address for cycletime

void loadInfoCycleTime() {
  EEPROM.begin(EEPROM_SIZE);
  infoCycleTime = EEPROM.readUInt(INFO_CYCLETIME_ADDR);
  if (infoCycleTime == 0xFFFFFFFF || infoCycleTime < 100) infoCycleTime = 1000; // fallback
}
void saveInfoCycleTime() {
  EEPROM.writeUInt(INFO_CYCLETIME_ADDR, infoCycleTime);
  EEPROM.commit();
}


void sendInfoJson() {
  String json = "{";
  json += "\"free_ram\":" + String(getFreeHeap()) + ",";
  json += "\"total_ram\":" + String(getTotalHeap()) + ",";
  json += "\"uptime\":\"" + formatUptime(millis() - startMillis) + "\",";
  json += "\"reset_count\":" + String(resetCount) + ",";
  json += "\"rs232_baudrate\":" + String(getRS232Baudrate()) + ",";
  json += "\"tcp_mode\":\"" + getTCPMode() + "\",";
  json += "\"tcp_port\":" + String(getTCPPort()) + ",";
  json += "\"eth_ip\":\"" + getEthIP().toString() + "\",";
  json += "\"eth_gw\":\"" + getEthGW().toString() + "\",";
  json += "\"eth_sn\":\"" + getEthSN().toString() + "\",";
  json += "\"eth_dns1\":\"" + getEthDNS1().toString() + "\",";
  json += "\"eth_dns2\":\"" + getEthDNS2().toString() + "\"";
  json += "}";
  wsInfo.textAll(json);
}
void InfoPage(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", index_info);
}
// API REST: trả về info và cho phép set cycletime
void handleInfo(AsyncWebServerRequest *request) {
  if (request->hasParam("cycletime", true)) {
    String val = request->getParam("cycletime", true)->value();
    uint32_t t = val.toInt();
    if (t >= 100 && t <= 60000) {
      infoCycleTime = t;
      saveInfoCycleTime();
    }
  }
  String json = "{";
  json += "\"free_ram\":" + String(getFreeHeap()) + ",";
  json += "\"total_ram\":" + String(getTotalHeap()) + ",";
  json += "\"uptime\":\"" + formatUptime(millis() - startMillis) + "\",";
  json += "\"reset_count\":" + String(resetCount) + ",";
  json += "\"rs232_baudrate\":" + String(getRS232Baudrate()) + ",";
  json += "\"tcp_mode\":\"" + getTCPMode() + "\",";
  json += "\"tcp_port\":" + String(getTCPPort()) + ",";
  json += "\"eth_ip\":\"" + getEthIP().toString() + "\",";
  json += "\"eth_gw\":\"" + getEthGW().toString() + "\",";
  json += "\"eth_sn\":\"" + getEthSN().toString() + "\",";
  json += "\"eth_dns1\":\"" + getEthDNS1().toString() + "\",";
  json += "\"eth_dns2\":\"" + getEthDNS2().toString() + "\",";
  json += "\"cycletime\":" + String(infoCycleTime);
  json += "}";
  request->send(200, "application/json", json);
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
  if (type == WS_EVT_DATA && len > 0) {
    String msg = String((char*)data).substring(0, len);
    if (msg.startsWith("cycletime:")) {
      uint32_t t = msg.substring(10).toInt();
      if (t >= 100 && t <= 60000) {
        infoCycleTime = t;
        saveInfoCycleTime();
      }
    }
  }
}


void setupInfoSocketAndTimer(AsyncWebServer *server) {
  loadInfoCycleTime();
  wsInfo.onEvent(onWsEvent);

    server->addHandler(&wsInfo);
    // Đăng ký API REST
    server->on("/api/info", HTTP_GET, handleInfo);
    // Đăng ký trang Info
    server->on("/info", HTTP_GET, InfoPage);
    // Call this in setup
     initResetCount();
     sendInfoJson();
    // Call this in setup
     initResetCount();
    
    startMillis = millis();
}

unsigned long lastInfoSend = 0;
void infoSocketLoop() {
  if (millis() - lastInfoSend >= infoCycleTime) {
    lastInfoSend = millis();
    sendInfoJson();
  }
}