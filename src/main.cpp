#include <WebServer_ESP32_SC_W5500.h>
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include <LittleFS.h>

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
IPAddress myGW(192, 168, 0, 1);
IPAddress mySN(255, 255, 255, 0);

// Google DNS Server IP
IPAddress myDNS(8, 8, 8, 8);

// Khai báo đối tượng UART
HardwareSerial MySerial1(0); // UART1

const char *ssid = "I-Soft";          // Replace with your network SSID
const char *password = "i-soft@2023"; // Replace with your network password
WiFiServer tcpServer(8080);
WiFiClient tcpClient;   // dùng khi ESP là client
bool wasConnectedToServer = false;
String tcpConfigServerIP = "192.168.3.165";
uint16_t tcpConfigPort = 80;
String tcpRole = "client";

AsyncWebServer server(80);
// AsyncWebSocket ws("/ws");// nếu dùng WebSocket

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>ESP32 Web Server</title>
  <meta charset="UTF-8">
  <link rel="icon" href="data:,">
  <script></script>
</head>
<body>
  <h1>Hello from ESP32!</h1>
  <p>Trang web này chạy trên ESP32.</p>
  <div class="container">
    <form action="/set_tcp_ip" method="POST">
    <label>TCP Server IP:</label>
    <input type="text" name="tcp_ip">
    <label>TCP Server Port:</label>
    <input type="number" name="tcp_port">
    <label>TCP Role:</label>
    <select name="tcp_role">
      <option value="server">Server</option>
      <option value="client">Client</option>
    </select>
    <label>My IP:</label>
    <input type="text" name="my_ip">
    <label>My Gateway:</label>
    <input type="text" name="my_gw">
    <label>My Subnet:</label>
    <input type="text" name="my_sn">
    <label>My DNS:</label>
    <input type="text" name="my_dns">
    <button type="submit">Lưu cấu hình</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

// void ConsoleLog(const String &message) {
//   ws.textAll(message); // Send message to all connected WebSocket clients
//   Serial.println(message); // Send message to all connected WebSocket clients
// } 

// void ConsoleLog(const int8_t &message) {
//   ws.textAll(String(message)); // Send message to all connected WebSocket clients
//   Serial.println(String(message)); // Send message to all connected WebSocket clients
// }  

// void ConsoleLogln(const String &message) {
//   ws.textAll(message); // Send message to all connected WebSocket clients
//   Serial.println(message); // Send message to all connected WebSocket clients
// }

// void ConsoleLogln(const int &message) {
//   Serial.println(String(message)); // Send message to all connected WebSocket clients
//   ws.textAll(String(message)); // Send message to all connected WebSocket clients
// }
// void ConsoleLogf(const char *format, ...) {
//   va_list args;
//   va_start(args, format);
//   char buffer[256];
//   vsnprintf(buffer, sizeof(buffer), format, args);
//   ws.textAll(buffer); // Send message to all connected WebSocket clients
//   Serial.println(buffer); // Send message to all connected WebSocket clients
//   va_end(args);
// }

// String wsMsg = "";
// void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
//   AwsFrameInfo *info = (AwsFrameInfo*)arg;
//   if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
//     for (size_t i = 0; i < len; i++) {
//       wsMsg += (char) data[i];
//     }
//   }
//   Serial.println("Received WebSocket message: " + wsMsg);
//   ws.textAll("Received: " + wsMsg); // Gửi lại dữ liệu đã nhận
//   serialHandler();
// }

// void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
//   switch (type) {
//     case WS_EVT_CONNECT:
//       ConsoleLogf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
//       break;
//     case WS_EVT_DISCONNECT:
//       ConsoleLogf("WebSocket client #%u disconnected\n", client->id());
//       break;
//     case WS_EVT_DATA:
//       handleWebSocketMessage(arg, data, len);
//       break;
//     case WS_EVT_PONG:
//     case WS_EVT_ERROR:
//       break;
//   }
// }

// void initWebSocket() {
//   ConsoleLogln("Initializing WebSocket ...");
//   ws.onEvent(onEvent);
//   server.addHandler(&ws);
// } 

// StaticJsonDocument<200> parseClientJson(const String& msg) {
//   StaticJsonDocument<200> doc;
//   DeserializationError error = deserializeJson(doc, msg);
//   StaticJsonDocument<200> result;
//   if (!error) {
//     result["role"] = doc["role"] | "";
//     result["status"] = doc["status"] | "";
//     result["data"] = doc["data"] | "";
//     result["ip"] = doc["ip"] | "";
//     result["port"] = doc["port"] | "";
//   } else {
//     result["role"] = "";
//     result["status"] = "";
//     result["data"] = "";
//     result["ip"] = "";
//     result["port"] = "";
//   }
//   return result;
// }

// void handleTCPServerResponse() {
//   if (isTCPClientConnected && tcpClient.connected()) {
//     while (tcpClient.available()) {
//       String serverMsg = tcpClient.readStringUntil('\n');
//       Serial.println("Received from server: " + serverMsg);
//       StaticJsonDocument<200> doc1 = parseClientJson(serverMsg);
//       String role = doc1["role"] | "";
//       String status = doc1["status"] | "";
//       String data = doc1["data"] | "";
//       if (role == "server") {
//         Serial.println("Server role confirmed");
//         if (status == "disconnect") {
//           Serial.println("Server requested disconnect");
//           tcpClient.stop();
//           isTCPClientConnected = false;
//           Serial.println("Disconnected from server");
//           return; // Thoát khỏi hàm nếu server yêu cầu ngắt kết nối
//         } else if (data.length() > 0) {
//           Serial.println("Server data: " + data);
//         } else {
//           Serial.println("No data received from server");
//         }
//       } else {
//         Serial.println("Invalid role from server: " + role);
//       }
//       // Xử lý logic với serverMsg tại đây nếu muốn
//     }
//   }
// }

// void handleTCPclient() {//đọc tin nhắnn từ client
//     if (!tcpServer) return; // Nếu server chưa được khởi tạo
//     WiFiClient client = tcpServer->available(); // Kiểm tra xem có client nào kết nối hay không
//     if (client) {
//         Serial.println("New client connected");
//         // Xử lý client kết nối
//         while (client.connected()) {
//             if (client.available()) {
//               String msg = client.readStringUntil('\n');
//               Serial.println("Received from client: " + msg);
//               StaticJsonDocument<200> doc1 = parseClientJson(msg);
//               String role = doc1["role"] | "";
//               String status = doc1["status"] | "";
//               String data = doc1["data"] | "";
//               client.flush(); // Đảm bảo không còn dữ liệu trong buffer
//               client.println("Received: " + msg); // Gửi lại dữ liệu đã nhận
//               if (role == "client") {
//                 if (status == "disconnect")
//                 {
//                   Serial.println("Client requested disconnect");
//                   client.println("Disconnected by server");
//                   break; // Thoát khỏi vòng lặp để ngắt kết nối client
//                 }
//                 else if (data.length() > 0)
//                 {
//                   Serial.println("Client data: " + data);
//                   client.println("Server received: " + data);
//                 }
//                 else {
//                   client.println("No data received");
//                 }
//               }
//               else {
//                 client.println("Invalid role");
//               }
//           }
//         }
//         if(!client) return;   
//         Serial.println("Client disconnected");
//         client.stop();     
//     }
//   }

// void stopAndDeleteTCPServer() {
//   if (tcpServer) {
//     tcpServer->stop();
//     delete tcpServer;
//     tcpServer = nullptr;
//     Serial.println("TCP Server stopped and deleted");
//   }
// }

String msgSerial;
String currentRole;

// Nhận trạng thái từ Serial để xử lý role của tcp 
void serialHandler() {
  // Ưu tiên xử lý tin nhắn từ WebSocket nếu có
  // if (wsMsg.length() > 0) {
  //   msgSerial = wsMsg;
  //   wsMsg = ""; // Đã xử lý xong, xóa đi
  //   Serial.println("Received from WebSocket: " + msgSerial);
  //   // ws.textAll("Received from WebSocket: " + msgSerial);
  // } else {
    msgSerial = Serial.readStringUntil('\n');
    Serial.println("Received from Serial: " + msgSerial);

    if (tcpRole == "client") {
      if (tcpClient.connected()) {
      tcpClient.println(msgSerial); // Gửi tin nhắn đến server TCP
      Serial.println("Sent to TCP server: " + msgSerial);
      MySerial1.println(msgSerial); // Gửi tin nhắn đến UART1
      } else {
      Serial.println("Not connected to any TCP server.");
      }
    } else if (tcpRole == "server") {
      if (tcpClient.connected()) {
        tcpClient.println(msgSerial); // Gửi tin nhắn đến client TCP
        Serial.println("Sent to TCP client: " + msgSerial);
        MySerial1.println(msgSerial); // Gửi tin nhắn đến UART1
      } else {
        Serial.println("No TCP client connected.");
      }
    }
    // ws.textAll("Received from Serial: " + msgSerial);
  // } 
  // if (msgSerial.startsWith("{") && msgSerial.endsWith("}")) {
  //     // Đơn giản dùng ArduinoJson để parse (nên thêm thư viện ArduinoJson vào project)
  //     StaticJsonDocument<200> obj = parseClientJson(msgSerial);       
  //       String ip = obj["ip"] | "";
  //       // String role = obj["role"] | "";
  //       String role = "server"; // "server" hoặc "client"
  //       // String status = obj["status"] | "";
  //       String status = "connect"; // "connect" hoặc "disconnect"
  //       // int port = String(obj["port"]| "").toInt() ;
  //       int port = 8080; // Cổng kết nối, có thể thay đổi tùy ý
  //       String data = obj["data"] | "";
  //       Serial.println("Parsed JSON: role=" + role + ", status=" + status + ", ip=" + ip + ", port=" + String(port) + ", data=" + data);
  //       if (role == "server") {
  //         currentRole = "server";
  //         Serial.println("Server IP: " + ip);
  //         Serial.println("Server Port: " + port);
  //         // stopAndDeleteTCPServer(); // Dừng và xóa server cũ nếu có
  //         if(status == "connect"){
  //           // Tạo server mới với cổng 8080
  //           tcpServer = new WiFiServer(8080);
  //           tcpServer->begin();
  //           // Thiết lập client TCP
  //           WiFiClient client = tcpServer->available();
  //           Serial.println("i am TCP Server with port " + String(port != 0 ? port : 8080));
  //           Serial.println("Server started");
  //           Serial.println("Client received: " + msgSerial); // Gửi lại dữ liệu đã nhận
  //           client.println("Client received: " + msgSerial); // Gửi lại dữ liệu đã nhận
  //           if (client) {
  //               client.println("Client received: " + msgSerial);
  //           }
  //         }
  //         if(status == "disconnect") {
  //           stopAndDeleteTCPServer(); // Dừng và xóa server cũ nếu có
  //         }
  //       } else if (role == "client") {
  //       if (status == "connect") {
  //         currentRole = "client";
  //         Serial.println("Server IP: " + ip);
  //         Serial.println("Server Port: " + port);
  //         // Thiết lập client TCP
  //         WiFiClient client;
  //         // if (tcpClient.connect(ip.c_str(), port)) {
  //         if (tcpClient.connect("191,168,1,9", 8080)) {
  //           isTCPClientConnected = true;
  //           Serial.println("Connected to server at " + ip + ":" + String(port));
  //           client.println("i am TCP client");// Gửi thông điệp đến server
  //         } else {
  //           Serial.println("Connection failed");
  //         }
  //       } 
  //       else if (status == "disconnect") {
  //         if (tcpClient.connected()) {
  //           tcpClient.stop();
  //           isTCPClientConnected = false;
  //           Serial.println("Disconnecting from server");
  //         }
  //       }      
  //   }
  // }
// }
}

void checkNewClient() {
    WiFiClient newClient = tcpServer.available();
    if (newClient) {
        Serial.println("New client connected");
        tcpClient = newClient;
    }
}

void checkServerConnection() {
  if (tcpClient.connected()) {
    if (!wasConnectedToServer) {
      Serial.println("Connected to server");
      wasConnectedToServer = true;
    }
  } else {
    if (wasConnectedToServer) {
      Serial.println("Lost connection to server, reconnecting...");
      wasConnectedToServer = false;
    }
    if (tcpClient.connect(tcpConfigServerIP.c_str(), tcpConfigPort)) {
      Serial.println("Reconnected to server");
      wasConnectedToServer = true;
    } else {
      // Chỉ in thông báo thất bại khi vừa mất kết nối
      static unsigned long lastPrint = 0;
      if (millis() - lastPrint > 3000) {
        Serial.println("Reconnect failed, will retry...");
        lastPrint = millis();
      }
    }
    delay(500); // Tránh spam kết nối quá nhanh
  }
}

void saveConfig() {
  File file = LittleFS.open("/config.txt", "w");
  if (!file) {
    Serial.println("Failed to open config file for writing");
    return;
  }
  file.println(tcpConfigServerIP);
  file.println(tcpConfigPort);
  file.println(tcpRole);
  file.println(myIP);
  file.println(myGW);
  file.println(mySN);
  file.println(myDNS);
  file.close();
}

void loadConfig() {
  File file = LittleFS.open("/config.txt", "r");
  if (!file) {
    Serial.println("No config file found, using defaults");
    return;
  }
  tcpConfigServerIP = file.readStringUntil('\n');
  tcpConfigServerIP.trim();
  tcpConfigPort = file.readStringUntil('\n').toInt();
  tcpRole = file.readStringUntil('\n');
  tcpRole.trim();
  String ipStr = file.readStringUntil('\n');
  myIP.fromString(ipStr);
  String gwStr = file.readStringUntil('\n');
  myGW.fromString(gwStr);
  String snStr = file.readStringUntil('\n');
  mySN.fromString(snStr);
  String dnsStr = file.readStringUntil('\n');
  myDNS.fromString(dnsStr);
  file.close();
}

void setup() {
  if (!LittleFS.begin()) {
    Serial.println("LittleFS Mount Failed");
  }
  loadConfig(); //đọc cấu hình từ file

  delay(5000);

  Serial.begin(115200);
  MySerial1.begin(9600, SERIAL_8N1, 2, 7);

  Serial.println(tcpConfigServerIP);
  Serial.println(tcpConfigPort);
  Serial.println(tcpRole);
  Serial.println(myIP);
  Serial.println(myGW);
  Serial.println(mySN);
  Serial.println(myDNS);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  // ws.textAll("Connected to WiFi");
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html);
  });

  server.on("/set_tcp_ip", HTTP_POST, [](AsyncWebServerRequest *request){
    tcpConfigServerIP = request->getParam("tcp_ip", true)->value();
    tcpConfigPort = request->getParam("tcp_port", true)->value().toInt();
    tcpRole = request->getParam("tcp_role", true)->value();
    myIP.fromString(request->getParam("my_ip", true)->value());
    myGW.fromString(request->getParam("my_gw", true)->value());
    mySN.fromString(request->getParam("my_sn", true)->value());
    myDNS.fromString(request->getParam("my_dns", true)->value());
    saveConfig(); //Lưu cấu hình vào file
    if (tcpRole == "server") {
      tcpServer.begin();
      Serial.println("TCP Server started at port " + String(tcpConfigPort));
    } else if (tcpRole == "client") {
      Serial.println("Cau hinh moi: " + tcpConfigServerIP + ":" + String(tcpConfigPort));
      if (tcpClient.connect(tcpConfigServerIP.c_str(), tcpConfigPort)) {
          Serial.println("Connected to server at " + tcpConfigServerIP + ":" + String(tcpConfigPort));
      } else {
          Serial.println("Connection failed");
      }
      request->send(200, "text/plain", "Da nhan cau hinh: " + tcpConfigServerIP + ":" + String(tcpConfigPort));
    }
  });

  server.begin();

  // tcpServer = new WiFiServer(8080);
  // tcpServer->begin();
  // To be called before ETH.begin()
  ESP32_W5500_onEvent();

  // start the ethernet connection and the server:
  // Use DHCP dynamic IP and random mac
  uint16_t index = millis() % NUMBER_OF_MAC;

  //bool begin(int MISO_GPIO, int MOSI_GPIO, int SCLK_GPIO, int CS_GPIO, int INT_GPIO, int SPI_CLOCK_MHZ,
  //           int SPI_HOST, uint8_t *W5500_Mac = W5500_Default_Mac);
  //ETH.begin( MISO_GPIO, MOSI_GPIO, SCK_GPIO, CS_GPIO, INT_GPIO, SPI_CLOCK_MHZ, ETH_SPI_HOST );
  bool ethStatus = false;
  while (!ethStatus) {
    Serial.print("Connecting to Ethernet: ");
    ethStatus = ETH.begin( MISO_GPIO, MOSI_GPIO, SCK_GPIO, CS_GPIO, INT_GPIO, SPI_CLOCK_MHZ, ETH_SPI_HOST, mac[index] );
    delay(1000);
  }
  
  // Static IP, leave without this line to get IP via DHCP
  //bool config(IPAddress local_ip, IPAddress gateway, IPAddress subnet, IPAddress dns1 = 0, IPAddress dns2 = 0);
  ETH.config(myIP, myGW, mySN, myDNS);

  // Serial.print(F("HTTP EthernetWebServer is @ IP : "));
  // Serial.println(ETH.localIP());
  ESP32_W5500_waitForConnect();

  ///////////////////////////////////
  // ETHserver.on(F("/"), handleRoot);
  // ETHserver.on(F("/test.svg"), drawGraph);
  // ETHserver.on(F("/inline"), []()
  // {`
  //   ETHserver.send(200, F("text/plain"), F("This works as well"));
  // });
  // // ETHserver.onNotFound(handleNotFound);

  if (tcpRole == "server") {
    tcpServer.begin();
  } else if (tcpRole == "client") {
    // Thiết lập client TCP
    if (tcpClient.connect(tcpConfigServerIP.c_str(), tcpConfigPort)) {
      Serial.println("Connected to server at " + tcpConfigServerIP + ":" + String(tcpConfigPort));
    } else {
      Serial.println("Connection failed");
    }
  }

  // Serial.print(F("HTTP EthernetWebServer is @ IP : "));
  // Serial.println(ETH.localIP());
}

void loop() {
  if (tcpRole == "server") {
    checkNewClient();
  } else {
    checkServerConnection(); 
  }

  if (Serial.available() > 0) {
    serialHandler();
  }

  if (MySerial1.available()) {
    int c = MySerial1.read();
    MySerial1.write(c); // Gửi sang UART1
    Serial.write(c); // Gửi sang Serial
    if (tcpRole == "server" && tcpClient.connected()) {
      tcpClient.write(c); // Gửi sang client TCP nếu có kết nối
    } else if (tcpRole == "client" && wasConnectedToServer) {
      tcpClient.write(c); // Gửi sang server TCP nếu có kết nối
    }
  }

  if(tcpClient.available()) {
    int c = tcpClient.read();
    Serial.write(c); // Gửi sang Serial
    MySerial1.write(c); // Gửi sang UART1
  }
}

  // handleTCPclient();
  // handleTCPServerResponse();