#ifndef LORA_H
#define LORA_H

#include <Arduino.h>
#include <HardwareSerial.h>

// LoRa E32 433 1W default parameters
#define LORA_DEFAULT_CHANNEL   0x17  // Example: channel 23
#define LORA_DEFAULT_NETID     0x01
#define LORA_DEFAULT_ID        0x01
#define LORA_DEFAULT_BAUDRATE  9600
#define LORA_DEFAULT_DATARATE  2     // Example: 2 = 2.4kbps

// enum LoRaRole {
//     LORA_ROLE_MASTER,
//     LORA_ROLE_NODE
// };

// struct LoRaPacket {
//     uint8_t id;
//     uint8_t netID;
//     char data[32]; // Plan/Result
// };
enum Role { ROLE_MASTER = 0, ROLE_NODE = 1 };

struct LoRaPacket {
    uint8_t id;
    uint8_t netID;
    uint8_t type; // 0: Plan, 1: Result
    uint8_t data[32];
    uint8_t dataLen;
};

class LORAE32 {
public:

    // LORAE32(HardwareSerial& serial, uint8_t m0, uint8_t m1, uint8_t aux);
    void PinSetup(HardwareSerial& serial, uint8_t m0, uint8_t m1, uint8_t aux);
    void setConfig(uint8_t channel, uint8_t id, uint8_t netID, uint8_t dataSpeed, uint32_t baudrate);
    void restoreDefault();
    void bindButton(uint8_t pin);
    
    void bind();
    void setDefaultConfig();
    void saveRoleAndID(Role role, uint8_t id, uint8_t netID);
    void loadRoleAndID();
    
    void send(const LoRaPacket& packet);
    void setRole(Role role);
    Role getRole() const;

    void setNodeID(uint8_t id);
    uint8_t getNodeID() const;

    void setNetID(uint8_t netID);
    uint8_t getNetID() const;

    // bool send(const LoRaPacket& packet);
    bool available();
    bool receive(LoRaPacket& packet);

    void onReceive(void (*callback)(const LoRaPacket&));
    void loop();

private:
    HardwareSerial& _serial;
    uint8_t _m0, _m1, _aux;
    Role _role;
    uint8_t _id, _netID, _channel, _dataSpeed;
    uint32_t _baudrate;
    void (*_onReceive)(const LoRaPacket&);
    uint8_t _bindPin;
    unsigned long _buttonPressTime;
    bool _buttonPressed;

    void enterConfigMode();
    void exitConfigMode();
    void saveConfig();
    void loadConfig();
    void checkBindButton();
};

#endif // LORA_H