#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include <LittleFS.h>
#include <AsyncElegantOTA.h>

void APIRecallSetup(AsyncWebServer* server){
    
  server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html);
  });

  server->on("/set_tcp_ip", HTTP_POST, [](AsyncWebServerRequest *request){
    TCPenabled = request->getParam("enabled", true)->value();
    tcpConfigServerIP = request->getParam("tcp_ip", true)->value();
    tcpConfigPort = request->getParam("tcp_port", true)->value().toInt();
    tcpRole = request->getParam("tcp_role", true)->value();
    #ifdef USE_ETHERNET
    myIP.fromString(request->getParam("my_ip", true)->value());
    myGW.fromString(request->getParam("my_gw", true)->value());
    mySN.fromString(request->getParam("my_sn", true)->value());
    myDNS.fromString(request->getParam("my_dns", true)->value());
    #endif // USE_ETHERNET
    ClientBaudrate = request->getParam("baudrate", true)->value().toInt();
    saveTCPConfig(); //Lưu cấu hình vào file
    #ifdef USE_LEDPIXEL
    pixels.setPixelColor(0, pixels.Color(150, 150, 150));
    pixels.show();   // Send the updated pixel colors to the hardware.
    #endif// USE_LEDPIXEL
    #ifdef USE_INFO
      updateInfoSocket(tcpRole, tcpConfigPort, GetServerPort(), myIP, myGW, mySN, myDNS, ClientBaudrate);
    #endif//USE_INFO
    if (tcpRole == "server") {
      // if (!serverStarted) {
      //     consolePrintln("Starting TCP server on port " + String(tcpConfigPort));          
      //     tcpServer->close(); // Đóng server cũ nếu có
      //     delete tcpServer;   // Giải phóng bộ nhớ
      //     tcpServer->begin(tcpConfigPort);
      //     serverStarted = true;
      //     #ifdef USE_LEDPIXEL
      //     pixels.setPixelColor(0, pixels.Color(0, 150, 0));
      //     pixels.show();
      //     #endif // USE_LEDPIXEL
      // }
    } else if (tcpRole == "client") {
      consolePrintln("Cau hinh moi: " + tcpConfigServerIP + ":" + String(tcpConfigPort));
      if (tcpClient.connect(tcpConfigServerIP.c_str(), tcpConfigPort)) {
          consolePrintln("Connected to server at " + tcpConfigServerIP + ":" + String(tcpConfigPort));
      } else {
          consolePrintln("Connection failed");
      }
    }
    request->send(200, "text/plain", "Da nhan cau hinh: " + tcpConfigServerIP + ":" + String(tcpConfigPort));
  });
  server->on("/console", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_console);
  });
// Gửi API reset sau 5s
  server->on("/api/reset", HTTP_GET, [](AsyncWebServerRequest *req){
    req->send(200, "text/plain", "ESP will reset in 5 seconds");
    // Đặt hẹn giờ reset sau 5s
    static bool resetScheduled = false;
    if (!resetScheduled) {
      resetScheduled = true;
      // Dùng task để delay không block
      xTaskCreate([](void*){
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    ESP.restart();
      }, "ResetTask", 2048, nullptr, 1, nullptr);
    }
  });
  server->on("/loadTCPconfig", HTTP_GET, [](AsyncWebServerRequest *request){
    StaticJsonDocument<256> doc;
    doc["enabled"] = TCPenabled;
    doc["tcp_ip"] = tcpConfigServerIP;
    doc["tcp_port"] = tcpConfigPort;
    doc["tcp_role"] = tcpRole;
    #ifdef USE_ETHERNET
    doc["my_ip"] = myIP.toString();
    doc["my_gw"] = myGW.toString();
    doc["my_sn"] = mySN.toString();
    doc["my_dns"] = myDNS.toString();
    #endif // USE_ETHERNET
    doc["baudrate"] = ClientBaudrate;
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
  });
}