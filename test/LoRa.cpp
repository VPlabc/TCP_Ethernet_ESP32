#include "LoRa.h"
#include <HardwareSerial.h>
#include <EEPROM.h>

#define LORA_SERIAL Serial2
uint8_t LORA_RX_PIN = -1;
uint8_t LORA_TX_PIN = -1;
uint8_t LORA_M0_PIN = -1; // Example pin, adjust as needed
uint8_t LORA_M1_PIN = -1; // Example pin, adjust as needed
uint8_t LORA_AUX_PIN = -1; // Example pin, adjust as needed

#define LORA_DEFAULT_CHANNEL 0x17
#define LORA_DEFAULT_ID 0x01
#define LORA_DEFAULT_NETID 0x01
#define LORA_DEFAULT_BAUDRATE 9600
#define LORA_DEFAULT_DATARATE 2 // 2: 2.4kbps

#define EEPROM_ROLE_ADDR 0
#define EEPROM_ID_ADDR 1
#define EEPROM_NETID_ADDR 2

#define BIND_BUTTON_PIN 22
#define BIND_HOLD_TIME 10000 // 10s



typedef void (*LoRaReceiveCallback)(const LoRaPacket&);
LoRaReceiveCallback receiveCb;
bool bindBtnPressed;
unsigned long bindBtnPressTime;
    Role role;
    uint8_t id;
    uint8_t _netID;
// class LORAE32 {
// public:
//     LORAE32();
//     void begin(uint8_t channel, uint8_t id, uint8_t netID, uint8_t dataSpeed, uint32_t baudrate);
//     void bind();
//     void saveRoleAndID(Role role, uint8_t id, uint8_t netID);
//     void loadRoleAndID();
//     void send(const LoRaPacket& packet);
//     void onReceive(LoRaReceiveCallback cb);
//     void loop();

//     Role role;
//     uint8_t id;
//     uint8_t netID;

// private:
//     LoRaReceiveCallback receiveCb;
//     unsigned long bindBtnPressTime;
//     bool bindBtnPressed;
//     void setDefaultConfig();
//     void enterConfigMode();
//     void exitConfigMode();
//     void writeConfig(uint8_t channel, uint8_t id, uint8_t netID, uint8_t dataSpeed, uint32_t baudrate);
// };

// LORAE32::LORAE32() : receiveCb(nullptr), bindBtnPressTime(0), bindBtnPressed(false) {}

void writeConfig(uint8_t channel, uint8_t id, uint8_t netID, uint8_t dataSpeed, uint32_t baudrate);
void exitConfigMode();

void LORAE32::PinSetup(HardwareSerial& serial, uint8_t m0, uint8_t m1, uint8_t aux) {
    LORA_SERIAL = serial;
    LORA_M0_PIN = m0;
    LORA_M1_PIN = m1;
    LORA_AUX_PIN = aux;

    pinMode(LORA_M0_PIN, OUTPUT);
    pinMode(LORA_M1_PIN, OUTPUT);
    pinMode(LORA_AUX_PIN, INPUT);
    
    digitalWrite(LORA_M0_PIN, LOW);
    digitalWrite(LORA_M1_PIN, LOW);
}

void LORAE32::setConfig(uint8_t channel, uint8_t id, uint8_t netID, uint8_t dataSpeed, uint32_t baudrate) {

    pinMode(BIND_BUTTON_PIN, INPUT_PULLUP);

    LORA_SERIAL.begin(baudrate, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);

    // loadRoleAndID();
    // writeConfig(channel, id, netID, dataSpeed, baudrate);
}

void LORAE32::bind() {
    if (digitalRead(BIND_BUTTON_PIN) == LOW) {
        if (!bindBtnPressed) {
            bindBtnPressTime = millis();
            bindBtnPressed = true;
        } else if (millis() - bindBtnPressTime > BIND_HOLD_TIME) {
            setDefaultConfig();
            saveRoleAndID(ROLE_NODE, LORA_DEFAULT_ID, LORA_DEFAULT_NETID);
        }
    } else {
        bindBtnPressed = false;
    }
}

void LORAE32::setDefaultConfig() {
    writeConfig(LORA_DEFAULT_CHANNEL, LORA_DEFAULT_ID, LORA_DEFAULT_NETID, LORA_DEFAULT_DATARATE, LORA_DEFAULT_BAUDRATE);
}

void LORAE32::saveRoleAndID(Role role, uint8_t id, uint8_t netID) {
    EEPROM.write(EEPROM_ROLE_ADDR, role);
    EEPROM.write(EEPROM_ID_ADDR, id);
    EEPROM.write(EEPROM_NETID_ADDR, netID);
    EEPROM.commit();
    role = role;
    id = id;
    _netID = netID;
}

void LORAE32::loadRoleAndID() {
    EEPROM.begin(8);
    role = (Role)EEPROM.read(EEPROM_ROLE_ADDR);
    id = EEPROM.read(EEPROM_ID_ADDR);
    _netID = EEPROM.read(EEPROM_NETID_ADDR);
    if (id == 0xFF || _netID == 0xFF) {
        id = LORA_DEFAULT_ID;
        _netID = LORA_DEFAULT_NETID;
        role = ROLE_NODE;
    }
}

void enterConfigMode() {
    digitalWrite(LORA_M0_PIN, HIGH);
    digitalWrite(LORA_M1_PIN, HIGH);
    delay(50);
}

void exitConfigMode() {
    digitalWrite(LORA_M0_PIN, LOW);
    digitalWrite(LORA_M1_PIN, LOW);
    delay(50);
}

void writeConfig(uint8_t channel, uint8_t id, uint8_t netID, uint8_t dataSpeed, uint32_t baudrate) {
    enterConfigMode();
    uint8_t config[6] = {0xC0, 0x00, 0x00, channel, netID, id};
    // Set dataSpeed and baudrate if needed (simplified)
    LORA_SERIAL.write(config, 6);
    delay(100);
    exitConfigMode();
}

void LORAE32::send(const LoRaPacket& packet) {
    uint8_t buf[36];
    buf[0] = packet.id;
    buf[1] = packet.netID;
    buf[2] = packet.type;
    buf[3] = packet.dataLen;
    memcpy(&buf[4], packet.data, packet.dataLen);
    LORA_SERIAL.write(buf, 4 + packet.dataLen);
}

void LORAE32::onReceive(void (*callback)(const LoRaPacket&)) { 
    receiveCb = cb;
}

void LORAE32::loop() {
    bind();
    if (LORA_SERIAL.available() >= 4) {
        LoRaPacket pkt;
        pkt.id = LORA_SERIAL.read();
        pkt.netID = LORA_SERIAL.read();
        pkt.type = LORA_SERIAL.read();
        pkt.dataLen = LORA_SERIAL.read();
        if (pkt.dataLen > 0 && pkt.dataLen <= 32) {
            LORA_SERIAL.readBytes(pkt.data, pkt.dataLen);
        }
        if (receiveCb) receiveCb(pkt);
    }
}

// // Example usage
// LORAE32 lora;

// void onLoraReceived(const LoRaPacket& pkt) {
//     // handle received packet
// }

// void setup() {
//     lora.begin(0x17, 0x01, 0x01, 2, 9600);
//     lora.onReceive(onLoraReceived);
// }

// void loop() {
//     lora.loop();
// }