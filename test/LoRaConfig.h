#ifndef LORA_CONFIG_H
#define LORA_CONFIG_H

#include <Arduino.h>

// Default LoRa parameters (adjust as needed)
#define LORA_DEFAULT_CHANNEL   10
#define LORA_DEFAULT_ID        1
#define LORA_DEFAULT_NETID     1
#define LORA_DEFAULT_BAUDRATE  9600
#define LORA_DEFAULT_DATARATE  2

// LoRa roles
enum LoRaRole {
    LORA_ROLE_MASTER = 0,
    LORA_ROLE_NODE = 1
};

// LoRa packet structure (example)
struct LoRaPacket {
    uint8_t id;
    uint8_t netID;
    char data[32];
};

void setupLoRaWeb(AsyncWebServer& server);
void LoRaSetup(HardwareSerial &serial , uint8_t m0, uint8_t m1, uint8_t aux ,  uint8_t chanel, uint8_t id, uint8_t netID, uint8_t dataSpeed, uint32_t baudrate);

#endif // LORA_CONFIG_H