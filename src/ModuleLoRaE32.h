#ifndef ModuleLoRaE32_H
#define ModuleLoRaE32_H
#include <Arduino.h>

struct LoRaPacket {
    uint8_t id;         // Node ID
    uint8_t netID;     // Network ID
    uint8_t type;      // 0: Plan, 1: Result
    uint8_t data[32];  // Data payload
    uint8_t dataLen;   // Length of data
};

// Structure for LoRa E32 configuration
struct LoRaE32Config {
  bool initialized = false;
  String moduleInfo = "";
  String configInfo = "";
  uint8_t addh = 0;
  uint8_t addl = 0;
  uint8_t chan = 0;
  String frequency = "";
  String airDataRateStr = "";
  String uartBaudRateStr = "";
  String transmissionPowerStr = "";
  String parityBit = "";
  String wirelessWakeupTimeStr = "";
  String fecStr = "";
  String fixedTransmissionStr = "";
  String ioDriveModeStr = "";
  uint8_t uartParity = 0;
  uint8_t uartBaudRate = 0b011;
  uint8_t airDataRate = 0b010;
  uint8_t fixedTransmission = 0;
  uint8_t ioDriveMode = 1;
  uint8_t wirelessWakeupTime = 0;
  uint8_t fec = 1;
  uint8_t transmissionPower = 0b11;
  uint8_t operatingMode = 0;
};


class M_LoRa_E32 {
    public:
        M_LoRa_E32();
        void initLoRaE32();
        // void begin(uint8_t channel, uint8_t id, uint8_t netID, uint8_t dataSpeed, uint32_t baudrate);
        
        // void bind();
        // void send(const LoRaPacket& packet);
        // void onReceive(LoRaReceiveCallback cb);
        // void loop();

        void setLoRaOperatingMode(uint8_t mode);
        void CMND(const String& cmd);
        uint8_t id;
        uint8_t netID;
        void setLoRaConfig();
        String GetJsonConfig();
        void SetConfigFromJson(String jsonString);
    private:
        // void saveLoRaConfig();
        // LoRaE32Config loadLoRaConfig();
        
        // LoRa E32 pins
        static const uint8_t E32_M0_PIN = 15;
        static const uint8_t E32_M1_PIN = 16;
        static const uint8_t E32_TX_PIN = 17;
        static const uint8_t E32_RX_PIN = 18;
        static const uint8_t E32_AUX_PIN = -1; // Not used
};
#endif // ModuleLoRaE32_H