#include <Arduino.h>  //not needed in the arduino ide

// Captive Portal
#include <AsyncTCP.h>  
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>	
#include <esp_wifi.h>			//Used for mpdu_rx_disable android workaround
#include "history.h"  // Include history functions for logging

const char *APssid = "ALC_Master";  // FYI The SSID can't have a space in it.
// const char * password = "12345678"; //Atleast 8 chars
const char *APpassword = NULL;  // no password

#define MAX_CLIENTS 4	
#define WIFI_CHANNEL 6	// 2.4ghz channel 6 https://en.wikipedia.org/wiki/List_of_WLAN_channels#2.4_GHz_(802.11b/g/n/ax)

const IPAddress localIP(4, 3, 2, 1);		   // the IP address the web server, Samsung requires the IP to be in public space
const IPAddress gatewayIP(4, 3, 2, 1);		  
const IPAddress subnetMask(255, 255, 255, 0);  // no need to change: https://avinetworks.com/glossary/subnet-mask/

const String localIPURL = "http://4.3.2.1";	 


const char index_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html>
<head>
  <title>HRS232 TCP Config</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    body {
      background: #f4f6fb;
      font-family: 'Segoe UI', Arial, sans-serif;
      margin: 0; padding: 0;
    }
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
  <script>
    // Lấy config từ server và cập nhật form
    document.addEventListener('DOMContentLoaded', function() {
      fetch('/loadTCPconfig')
        .then(r => r.json())
        .then(cfg => {
          document.querySelector('input[name="tcp_ip"]').value = cfg.tcp_ip || '';
          document.querySelector('input[name="tcp_port"]').value = cfg.tcp_port || '';
          document.querySelector('select[name="tcp_role"]').value = cfg.tcp_role || 'client';
          document.querySelector('input[name="my_ip"]').value = cfg.my_ip || '';
          document.querySelector('input[name="my_gw"]').value = cfg.my_gw || '';
          document.querySelector('input[name="my_sn"]').value = cfg.my_sn || '';
          document.querySelector('input[name="my_dns"]').value = cfg.my_dns || '';
          document.querySelector('input[name="baudrate"]').value = cfg.baudrate || 115200;
          // Hiển thị thông tin hiện tại
          document.getElementById('currentInfo').innerHTML =
            '<b>TCP Role:</b> ' + (cfg.tcp_role || '') + '<br>' +
            '<b>TCP Server:</b> ' + (cfg.tcp_ip || '') + ':' + (cfg.tcp_port || '') + '<br>' +
            '<b>My IP:</b> ' + (cfg.my_ip || '') + '<br>' +
            '<b>Gateway:</b> ' + (cfg.my_gw || '') + '<br>' +
            '<b>Subnet:</b> ' + (cfg.my_sn || '') + '<br>' +
            '<b>DNS:</b> ' + (cfg.my_dns || '') + '<br>' +
            '<b>Baudrate:</b> ' + (cfg.baudrate || 115200);
        });
    });
  </script>
</head>
<body>
  <div class="container">
    <h1>HRS232 TCP Config</h1>
    <p>Cấu hình TCP/IP cho HRS232</p>
    <form action="/set_tcp_ip" method="POST">
      <label>TCP Server IP:</label>
      <input type="text" name="tcp_ip" required>
      <label>TCP Server Port:</label>
      <input type="number" name="tcp_port" min="1" max="65535" required>
      <label>TCP Role:</label>
      <select name="tcp_role">
        <option value="server">Server</option>
        <option value="client">Client</option>
      </select>
      <label>My IP:</label>
      <input type="text" name="my_ip" required>
      <label>My Gateway:</label>
      <input type="text" name="my_gw" required>
      <label>My Subnet:</label>
      <input type="text" name="my_sn" required>
      <label>My DNS:</label>
      <input type="text" name="my_dns" required>
      <label>Client Baudrate:</label>
      <input type="number" name="baudrate" value="115200" min="300" max="1000000" required>
      <button type="submit">Lưu cấu hình</button>
    </form>
    <div class="info" id="currentInfo">
      Đang tải cấu hình...
    </div>
    <button type="button" onclick="fetch('/api/reset').then(r=>r.text()).then(alert)">Reset ESP (API)</button>
    <button type="button" onclick="fetch('/console').then(r=>r.text()).then(alert)">Xem Console</button>
  </div>
</body>
</html>
)=====";


// Trang HTML console
const char index_console[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>HRS232 Console</title>
  <meta charset="utf-8">
  <style>
    body { background: #222; color: #eee; font-family: monospace; }
    #console { width: 100%; height: 90vh; background: #111; color: #0f0; padding: 10px; overflow-y: scroll; border: 1px solid #444; }
  </style>
</head>
<body>
  <h2>HRS232 Console</h2>
  <div id="console"></div>
  <input type="text" id="input" placeholder="Type here..." autofocus
         onkeydown="if(event.key === 'Enter') { ws.send(this.value); this.value=''; }">
  <script>
    var ws = new WebSocket('ws://' + location.host + '/ws_console');
    var consoleDiv = document.getElementById('console');
    ws.onmessage = function(event) {
      consoleDiv.innerText += event.data;
      consoleDiv.scrollTop = consoleDiv.scrollHeight;
    };
    ws.onopen = function() { consoleDiv.textContent += '[Console connected]\n'; };
    ws.onclose = function() { consoleDiv.textContent += '[Console disconnected]\n'; };
  </script>
</body>
</html>
)rawliteral";



DNSServer dnsServer;
AsyncWebServer server(80);

void setUpDNSServer(DNSServer &dnsServer, const IPAddress &localIP) {
// Define the DNS interval in milliseconds between processing DNS requests
#define DNS_INTERVAL 30

	// Set the TTL for DNS response and start the DNS server
	dnsServer.setTTL(3600);
	dnsServer.start(53, "*", localIP);
}

void startSoftAccessPoint(const char *ssid, const char *password, const IPAddress &localIP, const IPAddress &gatewayIP) {
// Define the maximum number of clients that can connect to the server
#define MAX_CLIENTS 4
// Define the WiFi channel to be used (channel 6 in this case)
#define WIFI_CHANNEL 6

	// Set the WiFi mode to access point and station
	WiFi.mode(WIFI_MODE_AP);

	// Define the subnet mask for the WiFi network
	const IPAddress subnetMask(255, 255, 255, 0);

	// Configure the soft access point with a specific IP and subnet mask
	WiFi.softAPConfig(localIP, gatewayIP, subnetMask);

	// Start the soft access point with the given ssid, password, channel, max number of clients
	WiFi.softAP(ssid, password, WIFI_CHANNEL, 0, MAX_CLIENTS);

	// Disable AMPDU RX on the HRS232 WiFi to fix a bug on Android
	esp_wifi_stop();
	esp_wifi_deinit();
	wifi_init_config_t my_config = WIFI_INIT_CONFIG_DEFAULT();
	my_config.ampdu_rx_enable = false;
	esp_wifi_init(&my_config);
	esp_wifi_start();
	vTaskDelay(100 / portTICK_PERIOD_MS);  // Add a small delay
}

void setUpWebserver(AsyncWebServer &server, const IPAddress &localIP) {
	//======================== Webserver ========================
	// WARNING IOS (and maybe macos) WILL NOT POP UP IF IT CONTAINS THE WORD "Success" https://www.esp8266.com/viewtopic.php?f=34&t=4398
	// SAFARI (IOS) IS STUPID, G-ZIPPED FILES CAN'T END IN .GZ https://github.com/homieiot/homie-esp8266/issues/476 this is fixed by the webserver serve static function.
	// SAFARI (IOS) there is a 128KB limit to the size of the HTML. The HTML can reference external resources/images that bring the total over 128KB
	// SAFARI (IOS) popup browser has some severe limitations (javascript disabled, cookies disabled)

	// Required
	server.on("/connecttest.txt", [](AsyncWebServerRequest *request) { request->redirect("http://logout.net"); });	// windows 11 captive portal workaround
	server.on("/wpad.dat", [](AsyncWebServerRequest *request) { request->send(404); });								// Honestly don't understand what this is but a 404 stops win 10 keep calling this repeatedly and panicking the esp32 :)

	// Background responses: Probably not all are Required, but some are. Others might speed things up?
	// A Tier (commonly used by modern systems)
	server.on("/generate_204", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });		   // android captive portal redirect
	server.on("/redirect", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });			   // microsoft redirect
	server.on("/hotspot-detect.html", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });  // apple call home
	server.on("/canonical.html", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });	   // firefox captive portal call home
	server.on("/success.txt", [](AsyncWebServerRequest *request) { request->send(200); });					   // firefox captive portal call home
	server.on("/ncsi.txt", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });			   // windows call home

	// B Tier (uncommon)
	//  server.on("/chrome-variations/seed",[](AsyncWebServerRequest *request){request->send(200);}); //chrome captive portal call home
	//  server.on("/service/update2/json",[](AsyncWebServerRequest *request){request->send(200);}); //firefox?
	//  server.on("/chat",[](AsyncWebServerRequest *request){request->send(404);}); //No stop asking Whatsapp, there is no internet connection
	//  server.on("/startpage",[](AsyncWebServerRequest *request){request->redirect(localIPURL);});

	// return 404 to webpage icon
	server.on("/favicon.ico", [](AsyncWebServerRequest *request) { request->send(404); });	// webpage icon

	// Serve Basic HTML Page
	server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse(200, "text/html", index_html);
		response->addHeader("Cache-Control", "public,max-age=31536000");  // save this file to cache for 1 year (unless you refresh)
		request->send(response);
		consolePrintln("Served Basic HTML Page");
	});
  server.on("/history", HTTP_GET, [](AsyncWebServerRequest *request) {
    handleHistory(request);
  });
	// the catch all
	server.onNotFound([](AsyncWebServerRequest *request) {
		request->redirect(localIPURL);
		Serial.print("onnotfound ");
		Serial.print(request->host());	// This gives some insight into whatever was being requested on the serial monitor
		Serial.print(" ");
		Serial.print(request->url());
		Serial.print(" sent redirect to " + localIPURL + "\n");
	});
}

void WiFi_AP_setup() {
	// Set the transmit buffer size for the Serial object and start it with a baud rate of 115200.
	// Serial.setTxBufferSize(1024);
	Serial.begin(115200);

	// Wait for the Serial object to become available.
	// while (!Serial)
	// 	;

	// Print a welcome message to the Serial port.
	consolePrintln("\n\nHRS232, V0.1.0 compiled " __DATE__ " " __TIME__ " by VinhPhat");  //__DATE__ is provided by the platformio ide
	Serial.printf("%s-%d\n\r", ESP.getChipModel(), ESP.getChipRevision());

  // Khởi tạo WiFi AP trước
   startSoftAccessPoint(APssid, APpassword, localIP, gatewayIP);delay(200);
  // Thêm MAC vào APssid để tránh trùng SSID
    // Lấy MAC sau khi WiFi đã sẵn sàng
    uint8_t mac[6];
    if (esp_wifi_get_mac(WIFI_IF_AP, mac) == ESP_OK) {
        char ssidWithMac[32];
        snprintf(ssidWithMac, sizeof(ssidWithMac), "%s_%02X%02X%02X", APssid, mac[3], mac[4], mac[5]);
        // Đổi SSID bằng WiFi.softAP, KHÔNG gọi lại startSoftAccessPoint
        WiFi.softAP(ssidWithMac, APpassword, WIFI_CHANNEL, 0, MAX_CLIENTS);
    } else {
        Serial.println("Failed to get AP MAC!");
    }
	setUpDNSServer(dnsServer, localIP);
  // ALC_setup(server); // Khởi tạo ALC Project
	setUpWebserver(server, localIP);
	server.begin();

	Serial.print("\n");
	Serial.print("Startup Time:");	// should be somewhere between 270-350 for Generic HRS232 (D0WDQ6 chip, can have a higher startup time on first boot)
	Serial.println(millis());
  consolePrintln("Startup Time:" + String(millis()));
	Serial.print("\n");
}

void WiFi_AP_loop() {
	dnsServer.processNextRequest();	 // I call this atleast every 10ms in my other projects (can be higher but I haven't tested it for stability)
	delay(DNS_INTERVAL);			 // seems to help with stability, if you are doing other things in the loop this may not be needed
}

