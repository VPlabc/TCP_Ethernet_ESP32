#include <WiFi.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <vector>


#define SERVER_HOST_NAME "esp_server"

#define TCP_PORT 7050
#define DNS_PORT 53
DNSServer DNS;
std::vector<AsyncClient*> clients;

 /* clients events */
static void handleError(void* arg, AsyncClient* client, int8_t error) {
	Serial.printf("\n connection error %s from client %s \n", client->errorToString(error), client->remoteIP().toString().c_str());
    consolePrintln("\n connection error " + String(client->errorToString(error)) + " from client " + client->remoteIP().toString() + " \n");
    WARNING = true;
}

static void handleData(void* arg, AsyncClient* client, void *data, size_t len) {
	// Serial.printf("\n data received from client %s \n", client->remoteIP().toString().c_str());
	Serial.write((uint8_t*)data, len);
    MySerial1.write((uint8_t*)data, len); // Ghi dữ liệu nhận được từ client vào Serial1
	// reply to client
	// if (client->space() > 32 && client->canSend()) {
	// 	char reply[32];
	// 	sprintf(reply, "this is from %s", SERVER_HOST_NAME);
	// 	client->add(reply, strlen(reply));
	// 	client->send();
	// }
}

static void handleDisconnect(void* arg, AsyncClient* client) {
	Serial.printf("\n client %s disconnected \n", client->remoteIP().toString().c_str());
    consolePrintln("\n client " + client->remoteIP().toString() + " disconnected \n");
    WARNING = true;

}

static void handleTimeOut(void* arg, AsyncClient* client, uint32_t time) {
	Serial.printf("\n client ACK timeout ip: %s \n", client->remoteIP().toString().c_str());
    consolePrintln("\n client ACK timeout ip: " + client->remoteIP().toString());
}


/* server events */
static void handleNewClient(void* arg, AsyncClient* client) {
	Serial.printf("\n new client has been connected to server, ip: %s", client->remoteIP().toString().c_str());
    consolePrintln("\n new client has been connected to server, ip: " + client->remoteIP().toString());
	// add to list
	clients.push_back(client);
	
	// register events
	client->onData(&handleData, NULL);
	client->onError(&handleError, NULL);
	client->onDisconnect(&handleDisconnect, NULL);
	client->onTimeout(&handleTimeOut, NULL);
    WARNING = false;

}

void TCP_setup(IPAddress local_ip, uint16_t tcp_port) {
	Serial.begin(115200);
	delay(20);
	
	// create access point
	// while (!WiFi.softAP(SSID, PASSWORD, 6, false, 15)) {
	// 	delay(500);
	// }

	// start dns server
	if (!DNS.start(DNS_PORT, SERVER_HOST_NAME, local_ip))
		Serial.printf("\n failed to start dns service \n");
        consolePrintln("\n failed to start dns service \n");

	AsyncServer* server = new AsyncServer(tcp_port); // start listening on tcp port 7050
	server->onClient(&handleNewClient, server);
	server->begin();
}

void TCP_loop() {
	DNS.processNextRequest();
    // long lastTimes = millis();
    // if (millis() - lastTimes > 1000) {
    //     lastTimes = millis();
        // Serial.println("Tick");
        // Gửi dữ liệu từ Serial1 tới tất cả client TCP
    if (MySerial1.available()) {
        uint8_t b = MySerial1.read();
        for (auto client : clients) {
            if (client->canSend()) {
                client->write((const char*)&b, 1);
            }
        }
    }
    // }
}