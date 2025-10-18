#define USE_ETHERNET
#define USE_LEDPIXEL
#define USE_TCP
#define USE_WIFI_AP
#define USE_INFO

#ifdef USE_ETHERNET
#include <WebServer_ESP32_SC_W5500.h>
#endif// USE_ETHERNET

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include <LittleFS.h>
#include <AsyncElegantOTA.h>

#ifdef USE_LEDPIXEL
#include <Adafruit_NeoPixel.h>
#define PIN       48 
#define NUMPIXELS 2 
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#endif// USE_LEDPIXEL

bool WARNING = false; // Biến để theo dõi trạng thái cảnh báo
bool ERROR = false;
bool NETWORK = false;

bool TCPenabled = false;

#ifdef USE_ETHERNET
#define ETH_SPI_HOST        SPI3_HOST
#define SPI_CLOCK_MHZ       25
// Must connect INT to GPIOxx or not working
#define INT_GPIO            10

#define MISO_GPIO           9
#define MOSI_GPIO           13
#define SCK_GPIO            12
#define CS_GPIO             11

// Enter a MAC address and IP address for your controller below.
#define NUMBER_OF_MAC      20
byte mac[][NUMBER_OF_MAC] =
{
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x01 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x02 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x03 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x04 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x05 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x06 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x07 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x08 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x09 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x0A },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x0B },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x0C },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x0D },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x0E },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x0F },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x10 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x11 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x12 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x13 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x14 },
};

IPAddress myIP(192, 168, 3, 163);
IPAddress myGW(192, 168, 3, 1);
IPAddress mySN(255, 255, 255, 0);

// Google DNS Server IP
IPAddress myDNS(8, 8, 8, 8);

#endif// USE_ETHERNET

// WebSocket cho console
AsyncWebSocket wsConsole("/ws_console");
#include <vector>

bool serverStarted = false;
bool socketConnected = false;
// Lưu trữ các dòng console
std::vector<String> consoleBuffer;
const size_t MAX_CONSOLE_LINES = 200;
#include "esp_heap_caps.h"


// Khai báo đối tượng UART
#ifdef USE_TCP
  HardwareSerial MySerial1(1); // UART1
#endif//USE_TCP

const char *ssid = "I-Soft";          // Replace with your network SSID
const char *password = "i-soft@2023"; // Replace with your network password

WiFiServer *tcpServer = nullptr;
WiFiClient tcpClient;   // dùng khi ESP là client
bool wasConnectedToServer = false;
bool wasConnectedToClient = false;
String tcpConfigServerIP = "192.168.3.165";
uint16_t tcpConfigPort = 8080;
String tcpRole = "client";

void consolePrintln(const String& line);
void consolePrintf(const char* format, ...);

// Hàm kiểm tra và khởi tạo PSRAM
void initPSRAM() {
  if (psramInit()) {
    consolePrintln("PSRAM initialized successfully");
    consolePrintf("Total PSRAM: %u bytes\n", ESP.getPsramSize());
    consolePrintf("Free PSRAM: %u bytes\n", ESP.getFreePsram());
  } else {
    consolePrintln("PSRAM initialization failed");
    WARNING = true;
  }
}

// Gọi hàm này trong setup() nếu cần
// Hàm thêm dòng vào consoleBuffer
void addConsoleLine(const String& line) {
  if (consoleBuffer.size() >= MAX_CONSOLE_LINES) {
    consoleBuffer.erase(consoleBuffer.begin());
  }
  consoleBuffer.push_back(line);
  // Gửi dòng mới đến WebSocket console
  if (socketConnected) {
    wsConsole.textAll(line);
  }
}

void consolePrintln(const String &msg) {
  Serial.println(msg); // Hoặc thêm timestamp, màu, ... nếu cần
  addConsoleLine(String(msg));
}

void consolePrintf(const char* format, ...) {
  char buf[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);
  Serial.print(buf);
  addConsoleLine(String(buf));
}

// Gửi toàn bộ buffer cho client mới
void sendConsoleBuffer(AsyncWebSocketClient *client) {
  String all;
  for (const auto& l : consoleBuffer) {
    // Lọc bỏ ký tự không phải ASCII hoặc UTF-8 hợp lệ
    String safeLine;
    for (size_t i = 0; i < l.length(); ++i) {
      char c = l[i];
      if ((c >= 32 && c <= 126) || c == '\n' || c == '\r') { // chỉ gửi ký tự in được
        safeLine += c;
      }
    }
    all += safeLine + "\n";
  }
  client->text(all);
}

// Khi có client mới kết nối WebSocket
void onWsConsoleEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                      void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    sendConsoleBuffer(client);
    socketConnected = true;
  }
  else if (type == WS_EVT_DISCONNECT) {
    consolePrintln("Client disconnected from console WebSocket");
    socketConnected = false;
  } else if (type == WS_EVT_DATA) {
    // Không cần xử lý dữ liệu từ client, chỉ gửi dữ liệu console
    Serial1.write(data, len); // Ghi dữ liệu nhận được từ client vào Serial1
    consolePrintln("Received from console WebSocket: " + String((char*)data, len));
    tcpClient.write(data, len); // Ghi dữ liệu nhận được từ client vào TCP client

  }
}

String msgSerial;
String currentRole;
int ClientBaudrate = 9600;


#ifdef USE_INFO
#include "Info.h" // Thư viện Info để quản lý thông tin hệ thống
#endif//USE_INFO

#include "WifiPostal.h" // Thư viện WifiPostal để tạo captive portal
#include "TCPServer.h" // Thư viện TCPServer để quản lý server TCP

void Network_event(WiFiEvent_t event)
{
  switch (event)
  {
    Serial.println("Network Event: " + String(event));
    case ARDUINO_EVENT_ETH_START:
      consolePrintln(F("\nETH Started"));
      //set eth hostname here
      #ifdef USE_ETHERNET
      ETH.setHostname("ESP32_SC_W5500");
      #endif// USE_ETHERNET
      break;

    case ARDUINO_EVENT_ETH_CONNECTED:
      consolePrintln(F("ETH Connected"));
      break;

    case ARDUINO_EVENT_ETH_GOT_IP:
    #ifdef USE_ETHERNET
      if (!ESP32_W5500_eth_connected)
      {
        consolePrintf("ETH MAC: %s , IPv4: %s", ETH.macAddress(),  ETH.localIP().toString());

        if (ETH.fullDuplex())
        {
          consolePrintln(F("FULL_DUPLEX, "));
        }
        else
        {
          consolePrintln(F("HALF_DUPLEX, "));
        }
        char ETHSpeed[16];
        snprintf(ETHSpeed, sizeof(ETHSpeed), "%dMbps", ETH.linkSpeed());
        consolePrintln(ETHSpeed);

        ESP32_W5500_eth_connected = true;
      }
    #endif// USE_ETHERNET
      break;

    case ARDUINO_EVENT_ETH_DISCONNECTED:
      consolePrintln("ETH Disconnected");
      #ifdef USE_ETHERNET
      ESP32_W5500_eth_connected = false;
      #endif// USE_ETHERNET
      break;

    case ARDUINO_EVENT_ETH_STOP:
      consolePrintln("\ETH Stopped");
      #ifdef USE_ETHERNET
      ESP32_W5500_eth_connected = false;
      #endif// USE_ETHERNET
      break;

    default:
      break;
  }
}

// Nhận trạng thái từ Serial để xử lý role của tcp 
void serialHandler() {
    msgSerial = Serial.readStringUntil('\n');
    consolePrintln("Received from Serial: " + msgSerial);
#ifdef USE_TCP
    if (tcpRole == "client") {
      if (tcpClient.connected()) {
      tcpClient.println(msgSerial); // Gửi tin nhắn đến server TCP
      consolePrintln("Sent to TCP server: " + msgSerial);
      MySerial1.println(msgSerial); // Gửi tin nhắn đến UART1
      } else {
      consolePrintln("Not connected to any TCP server.");
      WARNING = true; // Đánh dấu có lỗi
      }
    } else if (tcpRole == "server") {
      // if (tcpClient.connected()) {
      //   tcpClient.println(msgSerial); // Gửi tin nhắn đến client TCP
      //   consolePrintln("Sent to TCP client: " + msgSerial);
      //   MySerial1.println(msgSerial); // Gửi tin nhắn đến UART1
      // } else {
      //   consolePrintln("No TCP client connected.");
      //   WARNING = true; // Đánh dấu có lỗi
      // }
    }
  #endif//USE_TCP
}

void checkNewClient() {
  #ifdef USE_TCP
      // WiFiClient newClient = tcpServer->available();
      // if (newClient) {
      //   tcpClient = newClient;
      //   wasConnectedToClient = true;
      //   consolePrintln("New client connected");
      //   Serial.println("New client connected");
      //   WARNING = false; // Reset lỗi khi có client mới
      // } else {
      //   wasConnectedToClient = false;
      // }
  #endif//USE_TCP
}

void checkServerConnection() {
  #ifdef USE_TCP
  if (tcpClient.connected()) {
    if (!wasConnectedToServer) {
      consolePrintln("Connected to server");
      wasConnectedToServer = true;
    }
  } else {
    if (wasConnectedToServer) {
      consolePrintln("Lost connection to server, reconnecting...");
      wasConnectedToServer = false;
    }
    if (tcpClient.connect(tcpConfigServerIP.c_str(), tcpConfigPort)) {
      consolePrintln("Reconnected to server");
      wasConnectedToServer = true;
    } else {
      // Chỉ in thông báo thất bại khi vừa mất kết nối
      static unsigned long lastPrint = 0;
      if (millis() - lastPrint > 3000) {
        consolePrintln("Reconnect failed, will retry...");
        lastPrint = millis();
        // Nháy LED 2 lần để báo hiệu
        // Blink LED 2 times without delay using millis()
        static int blinkCount = 0;
        static unsigned long lastBlink = 0;
        static bool blinking = false;
        if (!blinking) {
          blinking = true;
          blinkCount = 0;
          lastBlink = millis();
        }
        if (blinking) {
          if (millis() - lastBlink >= 200) {
            if (blinkCount % 2 == 0) {
              #ifdef USE_LEDPIXEL
              pixels.setPixelColor(0, pixels.Color(0, 150, 100));
              #endif// USE_LEDPIXEL
            } else {
              #ifdef USE_LEDPIXEL
              pixels.setPixelColor(0, pixels.Color(0, 150, 0));
              #endif// USE_LEDPIXEL
            }
            pixels.show();
            lastBlink = millis();
            blinkCount++;
            if (blinkCount >= 4) {
              blinking = false;
            }
          }
        }
      }
    }
    delay(500); // Tránh spam kết nối quá nhanh
  }
  #endif//USE_TCP
}

void saveTCPConfig() {
  File file = LittleFS.open("/TCPconfig.json", "w");
  if (!file) {
    consolePrintln("Failed to open config file for writing");
    return;
  }

  StaticJsonDocument<500> doc;
  doc["enabled"] = TCPenabled;
  doc["tcp_ip"] = tcpConfigServerIP;
  doc["tcp_port"] = tcpConfigPort;
  doc["tcp_role"] = tcpRole;
  #ifdef USE_ETHERNET
  doc["my_ip"] = myIP.toString();
  doc["my_gw"] = myGW.toString();
  doc["my_sn"] = mySN.toString();
  doc["my_dns"] = myDNS.toString();
  #endif// USE_ETHERNET
  doc["baudrate"] = ClientBaudrate;

  Serial.println("[TCP CONFIG SAVE]");
  serializeJsonPretty(doc, Serial);
  Serial.println();
  
  if (serializeJson(doc, file) == 0) {
    consolePrintln("Failed to write JSON to file");
  } else {
    consolePrintln("Config saved to /TCPconfig.json");
  }
  file.close();
}

void loadTCPConfig() {
  File file = LittleFS.open("/TCPconfig.json", "r");
  if (!file) {
    consolePrintln("No config file found, using defaults");
    // Set default values
    TCPenabled = true; // Mặc định bật TCP
    tcpConfigServerIP = "192.168.1.254";
    tcpConfigPort = 8080;
    tcpRole = "client";
    #ifdef USE_ETHERNET
    myIP.fromString("192.168.1.253");
    myGW.fromString("192.168.1.0");
    mySN.fromString("255.255.255.0");
    myDNS.fromString("8.8.8.8");
    #endif// USE_ETHERNET
    ClientBaudrate = 9600; // Default baudrate
    return;
  }
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    consolePrintln("Failed to read config file, using defaults");
    WARNING = true; // Đánh dấu có lỗi
    file.close();
    return;
  }

  if(doc.containsKey("enabled")) TCPenabled = doc["enabled"].as<bool>();
  if (doc.containsKey("tcp_ip")) tcpConfigServerIP = doc["tcp_ip"].as<String>();
  if (doc.containsKey("tcp_port")) tcpConfigPort = doc["tcp_port"].as<uint16_t>();
  if (doc.containsKey("tcp_role")) tcpRole = doc["tcp_role"].as<String>();
  #ifdef USE_ETHERNET
  if (doc.containsKey("my_ip")) myIP.fromString(doc["my_ip"].as<String>());
  if (doc.containsKey("my_gw")) myGW.fromString(doc["my_gw"].as<String>());
  if (doc.containsKey("my_sn")) mySN.fromString(doc["my_sn"].as<String>());
  if (doc.containsKey("my_dns")) myDNS.fromString(doc["my_dns"].as<String>());
  #endif// USE_ETHERNET
  if (doc.containsKey("baudrate")) {ClientBaudrate = doc["baudrate"].as<int>();}
  consolePrintln("TCPenb:" + String(TCPenabled) + ", IP: " + tcpConfigServerIP + 
               ", Port: " + String(tcpConfigPort) + ", Role: " + tcpRole + " | TCP IP: " + myIP.toString() +
               ", GW: " + myGW.toString() + ", SN: " + mySN + " | Baudrate: " + String(ClientBaudrate));
  consolePrintln("Loaded TCP config from /TCPconfig.json");
  file.close();
}

void saveConfig() {
  File file = LittleFS.open("/config.json", "w");
  if (!file) {
    consolePrintln("Failed to open config file for writing");
    return;
  }

  DynamicJsonDocument doc(1024);
  ssid = "I-Soft"; // Replace with your network SSID
  password = "i-soft@2023"; // Replace with your network password
  doc["ssid"] = ssid;
  doc["password"] = password;
  
  if (serializeJson(doc, file) == 0) {
    consolePrintln("Failed to write JSON to file");
  } else {
    consolePrintln("Config saved to /config.json");
  }
  file.close();
}

void loadConfig() {
  File file = LittleFS.open("/config.json", "r");
  if (!file) {
    consolePrintln("No config file found, using defaults");
    return;
  }

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    consolePrintln("Failed to read config file, using defaults");
    WARNING = true; // Đánh dấu có lỗi
    file.close();
    return;
  }

  // Load your configuration parameters here
  // Example: myParameter = doc["myParameter"].as<String>();

  file.close();
}



// AsyncWebServer server(80);
// AsyncWebSocket ws("/ws");// nếu dùng WebSocket


uint16_t GetServerPort(){
  #ifdef USE_ETHERNET
  return tcpConfigPort;
  #else
  return 80; // Default port for HTTP server
  #endif// USE_ETHERNET
}

#include "APIRecall.h" // Thư viện APIRecall để quản lý cấu hình qua API


void setup() {
  // delay(10000);
  #ifdef USE_LEDPIXEL
  pixels.begin(); 
  pixels.clear();
  #endif// USE_LEDPIXEL

  Serial.begin(115200);
  WiFi.onEvent(Network_event);

  if (!LittleFS.begin()) {
    consolePrintln("LittleFS Mount Failed");
    consolePrintln("Formatting LittleFS...");
      #ifdef USE_LEDPIXEL 
      pixels.setPixelColor(0, pixels.Color(255, 200, 0));pixels.show();
      #endif// USE_LEDPIXEL
    if (!LittleFS.format()) {
      consolePrintln("Failed to format LittleFS");
      #ifdef USE_LEDPIXEL
      pixels.setPixelColor(0, pixels.Color(255, 0, 0));pixels.show();  
      #endif// USE_LEDPIXEL
      ERROR = true; // Đánh dấu có lỗi nghiêm trọng
      return;
    } else {
      consolePrintln("LittleFS formatted successfully");
      #ifdef USE_LEDPIXEL
      pixels.setPixelColor(0, pixels.Color(10, 200, 0));pixels.show();
      #endif// USE_LEDPIXEL
      ERROR = WARNING = false; // Reset lỗi
    }
  }

#ifdef USE_TCP
  loadTCPConfig();
#endif//USE_TCP 

  #ifdef USE_TCP
  MySerial1.begin(ClientBaudrate, SERIAL_8N1, 2, 7);
  #endif// UART1
  tcpServer = new WiFiServer(tcpConfigPort);

  #ifdef USE_LEDPIXEL
  pixels.setPixelColor(0, pixels.Color(0, 150, 0));
  pixels.show();   
  #endif// USE_LEDPIXEL
#ifdef USE_ETHERNET
  consolePrintln("======================================");
  consolePrintln("Server IP: " + tcpConfigServerIP);
  consolePrintln("Server Port: " + String(tcpConfigPort));
  consolePrintln("TCP Role: " + tcpRole);
  consolePrintln("My IP: " + myIP.toString());
  consolePrintln("My Gateway: " + myGW.toString());
  consolePrintln("My Subnet: " + mySN.toString());
  consolePrintln("My DNS: " + myDNS.toString());
  consolePrintln("Client Baudrate: " + String(ClientBaudrate));
  consolePrintln("======================================");
#endif//USE_ETHERNET
#ifndef USE_WIFI_AP
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    consolePrintln("Connecting to WiFi...");
  }
  // ws.textAll("Connected to WiFi");
  consolePrintln("Connected to WiFi");
  consolePrintln(WiFi.localIP().toString());
  #endif//USE_WIFI_AP
  
  #ifdef USE_LEDPIXEL
  pixels.setPixelColor(0, pixels.Color(0, 0, 150));
  pixels.show();   // Send the updated pixel colors to the hardware.
  #endif// USE_LEDPIXEL
  // WiFiUDP udp;
  // udp.begin(5000); 

  #ifdef USE_WIFI_AP
  WiFi_AP_setup();
  NETWORK = true;

  #endif//USE_WIFI_AP  
  #ifdef USE_INFO
  // Khởi tạo WebSocket cho thông tin hệ thống
  setupInfoSocketAndTimer(&server);

  #endif//USE_INFO
  APIRecallSetup(&server); // Khởi tạo API để gọi lại cấu hình
    
  // Khởi tạo WebSocket cho console
  wsConsole.onEvent(onWsConsoleEvent);
  server.addHandler(&wsConsole);
  server.begin();
  #ifdef USE_INFO
  //update Information
  updateInfoSocket(tcpRole, tcpConfigPort, GetServerPort(), myIP, myGW, mySN, myDNS, ClientBaudrate);
  #endif // USE_INFO

  AsyncElegantOTA.begin(&server);
  consolePrintln("OTA Update enabled at /update");

  #ifdef USE_ETHERNET
    ESP32_W5500_onEvent();
    // start the ethernet connection and the server:
    // Use DHCP dynamic IP and random mac
    uint16_t index = millis() % NUMBER_OF_MAC;
    consolePrintln("Connecting to Ethernet: ");
    if(ETH.begin( MISO_GPIO, MOSI_GPIO, SCK_GPIO, CS_GPIO, INT_GPIO, SPI_CLOCK_MHZ, ETH_SPI_HOST, mac[index] )){
      consolePrintln("Ethernet connected with MAC: " + ETH.macAddress());
      #ifdef USE_LEDPIXEL
        pixels.setPixelColor(0, pixels.Color(0, 150, 0));pixels.show(); 
      #endif//USE_LEDPIXEL 
    }else{
      ERROR = true; // Đánh dấu có lỗi nghiêm trọng
      WARNING = false;
    }
    delay(1000);
    // }

    ETH.config(myIP, myGW, mySN, myDNS);
    // In ra địa chỉ IP Ethernet lên Serial và lưu vào consoleBuffer
    consolePrintln("Ethernet IP: " + ETH.localIP().toString());
    // ESP32_W5500_waitForConnect();
  #endif // USE_ETHERNET
  ///////////////////////////////////
  #ifdef USE_TCP
  if(TCPenabled) {
    consolePrintln("TCP enabled, starting configuration...");
    if (tcpRole == "server") {
      // if (!serverStarted) {
      //     consolePrintln("Starting TCP server on port " + String(tcpConfigPort));
      //     tcpServer->begin(tcpConfigPort);
      //     serverStarted = true;
      //     #ifdef USE_LEDPIXEL
      //     pixels.setPixelColor(0, pixels.Color(0, 150, 0));
      //     pixels.show();
      //     #endif // USE_LEDPIXEL
      // }
      consolePrintln("Starting TCP server on port " + String(tcpConfigPort) + " | IP: " + myIP.toString());
      TCP_setup(myIP, tcpConfigPort);
    } else if (tcpRole == "client") {
      // Thiết lập client TCP
      consolePrintln("Connecting to server at " + tcpConfigServerIP + ":" + String(tcpConfigPort));
      int retry = 0;
      const int maxRetry = 5;
      while (!tcpClient.connected() && retry < maxRetry) {
        if (tcpClient.connect(tcpConfigServerIP.c_str(), tcpConfigPort)) {
          consolePrintln("Connected to server at " + tcpConfigServerIP + ":" + String(tcpConfigPort));
          #ifdef USE_LEDPIXEL
          pixels.setPixelColor(0, pixels.Color(0, 150, 150));
          pixels.show();   // Send the updated pixel colors to the hardware.
          #endif // USE_LEDPIXEL
          break;
        } else {
          consolePrintln("Connection failed, retrying...");
          #ifdef USE_LEDPIXEL
          pixels.setPixelColor(0, pixels.Color(0, 150, 100));
          pixels.show();   // Send the updated pixel colors to the hardware.
          #endif// USE_LEDPIXEL
          delay(500);
          retry++;
          #ifdef USE_LEDPIXEL
          pixels.setPixelColor(0, pixels.Color(0, 150, 0));
          pixels.show();   // Send the updated pixel colors to the hardware.
          #endif // USE_LEDPIXEL
          delay(500);
        }
      }
      if (!tcpClient.connected()) {
        consolePrintln("Could not connect to server after retries.");
      }
    }

  }
  #endif // USE_TCP
  ///////////////////////////////////
  Serial.println("Ram :" + String(ESP.getFreeHeap()/1024) + " KB");
    consolePrintln("Setup completed");
    initPSRAM(); // Khởi tạo PSRAM nếu có
    #ifdef USE_LEDPIXEL
    pixels.setPixelColor(0, pixels.Color(0, 150, 0));pixels.show();   // Send the updated pixel colors to the hardware.
    #endif // USE_LEDPIXEL
}//setup

uint8_t wheelval = 0;
bool once_setup = true;
void loop() {
    // Blink green LED (pixel 0) without delay using millis() nếu không lỗi
    static unsigned long lastBlinkTime = 0;
    static bool ledOn = false;
    const unsigned long blinkInterval = 200; // ms
    
      if (millis() - lastBlinkTime >= blinkInterval) {
      lastBlinkTime = millis();
      // Serial.println("Ram :" + String(ESP.getFreeHeap()/1024) + " KB");

      ledOn = !ledOn;
      if (ledOn) {
        #ifdef USE_LEDPIXEL
        if (!WARNING & !ERROR) {
          pixels.setPixelColor(0, pixels.Color(0, 150, 0)); // LED xanh lá
        } else if (WARNING) {
          pixels.setPixelColor(0, pixels.Color(255, 200, 0)); // LED cam nếu có lỗi
        } else if (ERROR) { WARNING = false;
          pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // LED đỏ nếu có lỗi
        } else if (NETWORK) {
          pixels.setPixelColor(0, pixels.Color(0, 100, 150)); // LED xanh dương nếu có lỗi mạng
        } else {
          pixels.setPixelColor(0, pixels.Color(0, 150, 0)); 
        }
        #endif // USE_LEDPIXEL
      } else {
        #ifdef USE_LEDPIXEL
        pixels.setPixelColor(0, 0, 0, 0);
        #endif // USE_LEDPIXEL
      }
        #ifdef USE_LEDPIXEL
        pixels.show();
        #endif // USE_LEDPIXEL
      }

      static unsigned long wifiStartTime = millis();
      // if (WiFi.status() == WL_CONNECTED) {
        if (millis() - wifiStartTime > 180000) { // 3 phút = 180000 ms
          consolePrintln("WiFi connected for over 3 minutes, disconnecting...");
          WiFi.disconnect(true);\
          NETWORK = false; // Đánh dấu có lỗi mạng
        }
      // } else {
        // wifiStartTime = millis(); // Reset timer nếu mất kết nối
      // }
    // delay(200); 
  #ifdef USE_TCP
  if(TCPenabled){
    if (tcpRole == "server") { 
      TCP_loop();
    //   if (once_setup){
    //     once_setup = false;
    //     tcpServer = new WiFiServer(tcpConfigPort);
    //   }
      // checkNewClient();

    } else {
      checkServerConnection(); 
    }

    if (MySerial1.available()) {
      int c = MySerial1.read();
      // MySerial1.write(c); // Gửi sang UART1

      if (tcpRole == "server") {
        // Serial.write(c); // Gửi sang Serial
        // if (clients.space() > 32 && clients.canSend()) {
        // char reply[32];
        // sprintf(reply, "this is from %s", SERVER_HOST_NAME);
        // client->add(reply, strlen(reply));
        // client->send();

      //   tcpServer->write(c); // Gửi sang client TCP nếu có kết nối
      } else if (tcpRole == "client" && wasConnectedToServer) {
      //   tcpClient.write(c); // Gửi sang server TCP nếu có kết nối
      }
    }
    
    // if (Serial.available() > 0) {

    //     tcpClient.write(c); // Gửi sang server TCP nếu có kết nối
    //   }

    // if(tcpClient.connected() && tcpClient.available()) {
    //   int c = tcpClient.read();
    //   // Serial.write(c); // Gửi sang Serial
    //   MySerial1.write(c); // Gửi sang UART1
    //   if (socketConnected) {
    //     // wsConsole.textAll(String((char)c)); // Gửi dữ liệu đến WebSocket console
    //     consolePrintln(">> TCP: " + String((char)c));
    //   }
    // }

      // }
    // }
  }
  #endif//USE_TCP

  // Tạo task riêng cho WiFi_AP_loop nếu chưa tạo
  #ifdef USE_WIFI_AP
  static bool wifiTaskStarted = false;
  if (!wifiTaskStarted) {
    wifiTaskStarted = true;
    xTaskCreate([](void*) {
      while (true) {
        WiFi_AP_loop();
        vTaskDelay(10 / portTICK_PERIOD_MS); // tránh chiếm CPU
      }
    }, "WiFiAPLoopTask", 8096, nullptr, 1, nullptr);
  }
  #endif//USE_WIFI_AP
}

  // handleTCPclient();
  // handleTCPServerResponse();