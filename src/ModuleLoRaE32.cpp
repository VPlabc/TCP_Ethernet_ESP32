#include <Arduino.h>
#include "ModuleLoRaE32.h"
M_LoRa_E32 loraE32;
#include "LoRa_E32.h"
#include <Arduino_JSON.h>

// LoRa E32 Configuration Pins
#define E32_M0_PIN    15
#define E32_M1_PIN    16
#define E32_TX_PIN    17
#define E32_RX_PIN    18
#define E32_AUX_PIN   -1

M_LoRa_E32::M_LoRa_E32() {}

void sendDebugMessage(const String& message) {
    Serial.println(message); // Gửi thông điệp debug qua Serial
}
#define LOG(s) sendDebugMessage(s)

// LoRa E32 instance
LoRa_E32 e32ttl100(&Serial1, E32_AUX_PIN, E32_M0_PIN, E32_M1_PIN);

void printParameters(struct Configuration configuration);

void M_LoRa_E32::CMND(const String& cmd) {

    JSONVar json = JSON.parse(cmd);
    
    if (JSON.typeof(json) == "undefined") {
      Serial.println("Invalid JSON received");
      sendDebugMessage("Invalid JSON received");
      return;
    }
    String action = JSON.stringify(json["action"]);
    action.replace("\"", "");

     if (action == "set_lora_e32_config") {
      // static bool configUpdatePending = false;
      if (!json.hasOwnProperty("addh") || !json.hasOwnProperty("addl") || !json.hasOwnProperty("chan") ||
          !json.hasOwnProperty("uartParity") || !json.hasOwnProperty("uartBaudRate") ||
          !json.hasOwnProperty("airDataRate") || !json.hasOwnProperty("fixedTransmission") ||
          !json.hasOwnProperty("ioDriveMode") || !json.hasOwnProperty("wirelessWakeupTime") ||
          !json.hasOwnProperty("fec") || !json.hasOwnProperty("transmissionPower")) {
        sendDebugMessage("Invalid LoRa E32 configuration data");
        return;
      }
      // configUpdatePending = true;
      LoRaE32Config *config = new LoRaE32Config();
      config->addh = (int)json["addh"];
      config->addl = (int)json["addl"];
      config->chan = (int)json["chan"];
      config->uartParity = (int)json["uartParity"];
      config->uartBaudRate = (int)json["uartBaudRate"];
      config->airDataRate = (int)json["airDataRate"];
      config->fixedTransmission = (int)json["fixedTransmission"];
      config->ioDriveMode = (int)json["ioDriveMode"];
      config->wirelessWakeupTime = (int)json["wirelessWakeupTime"];
      config->fec = (int)json["fec"];
      config->transmissionPower = (int)json["transmissionPower"];
      // config->operatingMode = systemStatus.loraE32.operatingMode;
      

    }
    else if (action == "set_lora_operating_mode") {
      uint8_t mode = (int)json["mode"];
      setLoRaOperatingMode(mode);
      // sendSystemStatus();
    }
}

// Initialize LoRa E32
void M_LoRa_E32::initLoRaE32() {
  Serial.println("=== Initializing LoRa E32 ===");
  
  // Initialize chân M0, M1 là OUTPUT
  pinMode(E32_M0_PIN, OUTPUT);
  pinMode(E32_M1_PIN, OUTPUT);
  
  // Khởi tạo Serial1 với các chân đã định nghĩa
  Serial1.begin(9600, SERIAL_8N1, E32_RX_PIN, E32_TX_PIN);
  
  // Khởi tạo LoRa E32
  e32ttl100.begin();
  
  // Đặt chế độ mặc định là Normal Mode
  setLoRaOperatingMode(0); // Normal mode
  
  vTaskDelay(pdMS_TO_TICKS(1000)); // Tăng delay để module ổn định
  
  Serial.println("Reading module information...");
  
  // Đọc thông tin module với error handling
  ResponseStructContainer c;
  c = e32ttl100.getModuleInformation();
  
  if (c.status.code == 1 && c.data != nullptr) {
    ModuleInformation mi = *(ModuleInformation*)c.data;
    // printModuleInformation(mi);
    
    // Lưu thông tin module vào system status
    // systemStatus.loraE32.moduleInfo = "HEAD: " + String(mi.HEAD, HEX) + 
    //                                   ", Freq: " + String(mi.frequency, HEX) + 
    //                                   ", Ver: " + String(mi.version, HEX) + 
    //                                   ", Features: " + String(mi.features, HEX);
    // systemStatus.loraE32.initialized = true;
    
    sendDebugMessage("LoRa E32 module information read successfully");
  } else {
    Serial.print("Error reading module information: ");
    Serial.println(c.status.getResponseDescription());
    sendDebugMessage("Error reading LoRa E32 module information: " + String(c.status.getResponseDescription()));
    // systemStatus.loraE32.initialized = false;
  }
  
  if (c.data != nullptr) {
    c.close();
  }
  
  vTaskDelay(pdMS_TO_TICKS(1000));
  
    
  //   // Đọc cấu hình hiện tại
    ResponseStructContainer configContainer;
    configContainer = e32ttl100.getConfiguration();
    
    if (configContainer.status.code == 1 && configContainer.data != nullptr) {
      Configuration configuration = *(Configuration*)configContainer.data;
      Serial.println("Configuration read successfully!");
      
      printParameters(configuration);
      
  //     // // Lưu cấu hình vào system status
  //     // systemStatus.loraE32.addh = configuration.ADDH;
  //     // systemStatus.loraE32.addl = configuration.ADDL;
  //     // systemStatus.loraE32.chan = configuration.CHAN;
  //     // systemStatus.loraE32.uartParity = configuration.SPED.uartParity;
  //     // systemStatus.loraE32.uartBaudRate = configuration.SPED.uartBaudRate;
  //     // systemStatus.loraE32.airDataRate = configuration.SPED.airDataRate;
  //     // systemStatus.loraE32.fixedTransmission = configuration.OPTION.fixedTransmission;
  //     // systemStatus.loraE32.ioDriveMode = configuration.OPTION.ioDriveMode;
  //     // systemStatus.loraE32.wirelessWakeupTime = configuration.OPTION.wirelessWakeupTime;
  //     // systemStatus.loraE32.fec = configuration.OPTION.fec;
  //     // systemStatus.loraE32.transmissionPower = configuration.OPTION.transmissionPower;
  //     // systemStatus.loraE32.frequency = configuration.getChannelDescription();
  //     // systemStatus.loraE32.parityBit = configuration.SPED.getUARTParityDescription();
  //     // systemStatus.loraE32.airDataRateStr = configuration.SPED.getAirDataRate();
  //     // systemStatus.loraE32.uartBaudRateStr = configuration.SPED.getUARTBaudRate();
  //     // systemStatus.loraE32.fixedTransmissionStr = configuration.OPTION.getFixedTransmissionDescription();
  //     // systemStatus.loraE32.ioDriveModeStr = configuration.OPTION.getIODroveModeDescription();
  //     // systemStatus.loraE32.wirelessWakeupTimeStr = configuration.OPTION.getWirelessWakeUPTimeDescription();
  //     // systemStatus.loraE32.fecStr = configuration.OPTION.getFECDescription();
  //     // systemStatus.loraE32.transmissionPowerStr = configuration.OPTION.getTransmissionPowerDescription();
      
      sendDebugMessage("LoRa E32 configuration read successfully");
    } else {
      Serial.print("Error reading configuration: ");
      Serial.println(configContainer.status.getResponseDescription());
      sendDebugMessage("Error reading LoRa E32 configuration: " + String(configContainer.status.getResponseDescription()));
    }
    
    if (configContainer.data != nullptr) {
      configContainer.close();
    }
  // }
  
  Serial.println("=== LoRa E32 Initialization Complete ===");
}


// Set LoRa operating mode
void M_LoRa_E32::setLoRaOperatingMode(uint8_t mode) {
  switch (mode) {
    case 0: // Normal Mode (M0=0, M1=0)
      digitalWrite(E32_M0_PIN, LOW);
      digitalWrite(E32_M1_PIN, LOW);
      e32ttl100.setMode(MODE_0_NORMAL);
      // systemStatus.loraE32.operatingMode = 0;
      sendDebugMessage("LoRa E32 set to Normal Mode");
      break;
    case 1: // Wake-Up Mode (M0=1, M1=0)
      digitalWrite(E32_M0_PIN, HIGH);
      digitalWrite(E32_M1_PIN, LOW);
      e32ttl100.setMode(MODE_1_WAKE_UP);
      // systemStatus.loraE32.operatingMode = 1;
      sendDebugMessage("LoRa E32 set to Wake-Up Mode");
      break;
    case 2: // Power-Saving Mode (M0=0, M1=1)
      digitalWrite(E32_M0_PIN, LOW);
      digitalWrite(E32_M1_PIN, HIGH);
      e32ttl100.setMode(MODE_2_POWER_SAVING);
      // systemStatus.loraE32.operatingMode = 2;
      sendDebugMessage("LoRa E32 set to Power-Saving Mode");
      break;
    case 3: // Sleep Mode (M0=1, M1=1)
      digitalWrite(E32_M0_PIN, HIGH);
      digitalWrite(E32_M1_PIN, HIGH);
      e32ttl100.setMode(MODE_3_PROGRAM);
      // systemStatus.loraE32.operatingMode = 3;
      sendDebugMessage("LoRa E32 set to Sleep Mode");
      break;
    default:
      sendDebugMessage("Invalid LoRa E32 operating mode");
      return;
  }
  // saveLoRaConfig(); // Lưu chế độ vào LittleFS
  vTaskDelay(pdMS_TO_TICKS(100));// Đợi module ổn định
}



// Set LoRa E32 configuration
void M_LoRa_E32::setLoRaConfig() {
    loraE32.setLoRaOperatingMode(3); // MODE_3_PROGRAM
    LoRaE32Config config;
    Configuration loraConfig;
    loraConfig.HEAD = 0xC0;
    loraConfig.ADDH = config.addh;
    loraConfig.ADDL = config.addl;
    loraConfig.CHAN = config.chan;
    loraConfig.SPED.uartParity = config.uartParity;
    loraConfig.SPED.uartBaudRate = config.uartBaudRate;
    loraConfig.SPED.airDataRate = config.airDataRate;
    loraConfig.OPTION.fixedTransmission = config.fixedTransmission;
    loraConfig.OPTION.ioDriveMode = config.ioDriveMode;
    loraConfig.OPTION.wirelessWakeupTime = config.wirelessWakeupTime;
    loraConfig.OPTION.fec = config.fec;
    loraConfig.OPTION.transmissionPower = config.transmissionPower;

    ResponseStatus rs = e32ttl100.setConfiguration(loraConfig, WRITE_CFG_PWR_DWN_SAVE);
    if (rs.code == SUCCESS) {
      Serial.println("LoRa E32 configuration set successfully");
      sendDebugMessage("LoRa E32 configuration set successfully");
    }else {
      Serial.print("Error setting LoRa E32 configuration: ");
      Serial.println(rs.getResponseDescription());
      sendDebugMessage("Error setting LoRa E32 configuration: " + rs.getResponseDescription());
    }
    loraE32.setLoRaOperatingMode(0); // Quay lại chế độ Normal
}


String M_LoRa_E32::GetJsonConfig(){
    loraE32.setLoRaOperatingMode(3); // MODE_3_PROGRAM

    ResponseStructContainer configContainer = e32ttl100.getConfiguration();
    String result = "";
    if (configContainer.status.code == 1 && configContainer.data != nullptr) {
        Configuration newConfig = *(Configuration*)configContainer.data;

        JSONVar jsonConfig;
        jsonConfig["addh"] = newConfig.ADDH;
        jsonConfig["addl"] = newConfig.ADDL;
        jsonConfig["chan"] = newConfig.CHAN;
        jsonConfig["uartParity"] = newConfig.SPED.uartParity;
        jsonConfig["uartBaudRate"] = newConfig.SPED.uartBaudRate;
        jsonConfig["airDataRate"] = newConfig.SPED.airDataRate;
        jsonConfig["fixedTransmission"] = newConfig.OPTION.fixedTransmission;
        jsonConfig["ioDriveMode"] = newConfig.OPTION.ioDriveMode;
        jsonConfig["wirelessWakeupTime"] = newConfig.OPTION.wirelessWakeupTime;
        jsonConfig["fec"] = newConfig.OPTION.fec;
        jsonConfig["transmissionPower"] = newConfig.OPTION.transmissionPower;

        result = JSON.stringify(jsonConfig);
    } else {
        Serial.print("Error reading configuration: ");
        Serial.println(configContainer.status.getResponseDescription());
        sendDebugMessage("Error reading LoRa E32 configuration: " + String(configContainer.status.getResponseDescription()));
    }

    if (configContainer.data != nullptr) {
        configContainer.close();
    }
    loraE32.setLoRaOperatingMode(0); // Quay lại chế độ Normal
    return result;
}

void M_LoRa_E32::SetConfigFromJson(String jsonString) {
  JSONVar json = JSON.parse(jsonString);
  
  if (JSON.typeof(json) == "undefined") {
    Serial.println("Invalid JSON received");
    sendDebugMessage("Invalid JSON received");
    return;
  }
  
  if (!json.hasOwnProperty("addh") || !json.hasOwnProperty("addl") || !json.hasOwnProperty("chan") ||
      !json.hasOwnProperty("uartParity") || !json.hasOwnProperty("uartBaudRate") ||
      !json.hasOwnProperty("airDataRate") || !json.hasOwnProperty("fixedTransmission") ||
      !json.hasOwnProperty("ioDriveMode") || !json.hasOwnProperty("wirelessWakeupTime") ||
      !json.hasOwnProperty("fec") || !json.hasOwnProperty("transmissionPower")) {
    sendDebugMessage("Invalid LoRa E32 configuration data");
    return;
  }
  
  LoRaE32Config config;
  config.addh = (int)json["addh"];
  config.addl = (int)json["addl"];
  config.chan = (int)json["chan"];
  config.uartParity = (int)json["uartParity"];
  config.uartBaudRate = (int)json["uartBaudRate"];
  config.airDataRate = (int)json["airDataRate"];
  config.fixedTransmission = (int)json["fixedTransmission"];
  config.ioDriveMode = (int)json["ioDriveMode"];
  config.wirelessWakeupTime = (int)json["wirelessWakeupTime"];
  config.fec = (int)json["fec"];
  config.transmissionPower = (int)json["transmissionPower"];

  // Gọi hàm để thiết lập cấu hình LoRa E32
  setLoRaConfig();
}
// Print configuration parameters
void printParameters(struct Configuration configuration) {
  Serial.println("======== LORA E32 CONFIGURATION ========");
  
  Serial.print("HEAD: ");
  Serial.print(configuration.HEAD, BIN);
  Serial.print(" (");
  Serial.print(configuration.HEAD, DEC);
  Serial.print(") 0x");
  Serial.println(configuration.HEAD, HEX);
  
  Serial.println();
  
  Serial.print("High Address (ADDH): ");
  Serial.print(configuration.ADDH, DEC);
  Serial.print(" (0x");
  Serial.print(configuration.ADDH, HEX);
  Serial.print(", ");
  Serial.print(configuration.ADDH, BIN);
  Serial.println(")");
  
  Serial.print("Low Address (ADDL): ");
  Serial.print(configuration.ADDL, DEC);
  Serial.print(" (0x");
  Serial.print(configuration.ADDL, HEX);
  Serial.print(", ");
  Serial.print(configuration.ADDL, BIN);
  Serial.println(")");
  
  Serial.print("Channel (CHAN): ");
  Serial.print(configuration.CHAN, DEC);
  Serial.print(" → ");
  Serial.println(configuration.getChannelDescription());
  
  Serial.println();
  Serial.println("=== SPEED CONFIGURATION (SPED) ===");
  
  Serial.print("UART Parity: ");
  Serial.print(configuration.SPED.uartParity, BIN);
  Serial.print(" → ");
  Serial.println(configuration.SPED.getUARTParityDescription());
  
  Serial.print("UART Baud Rate: ");
  Serial.print(configuration.SPED.uartBaudRate, BIN);
  Serial.print(" → ");
  Serial.println(configuration.SPED.getUARTBaudRate());
  
  Serial.print("Air Data Rate: ");
  Serial.print(configuration.SPED.airDataRate, BIN);
  Serial.print(" → ");
  Serial.println(configuration.SPED.getAirDataRate());
  
  Serial.println();
  Serial.println("=== OPTIONS ===");
  
  Serial.print("Fixed Transmission: ");
  Serial.print(configuration.OPTION.fixedTransmission, BIN);
  Serial.print(" → ");
  Serial.println(configuration.OPTION.getFixedTransmissionDescription());
  
  Serial.print("IO Drive Mode: ");
  Serial.print(configuration.OPTION.ioDriveMode, BIN);
  Serial.print(" → ");
  Serial.println(configuration.OPTION.getIODroveModeDescription());
  
  Serial.print("Wireless Wakeup Time: ");
  Serial.print(configuration.OPTION.wirelessWakeupTime, BIN);
  Serial.print(" → ");
  Serial.println(configuration.OPTION.getWirelessWakeUPTimeDescription());
  
  Serial.print("FEC (Forward Error Correction): ");
  Serial.print(configuration.OPTION.fec, BIN);
  Serial.print(" → ");
  Serial.println(configuration.OPTION.getFECDescription());
  
  Serial.print("Transmission Power: ");
  Serial.print(configuration.OPTION.transmissionPower, BIN);
  Serial.print(" → ");
  Serial.println(configuration.OPTION.getTransmissionPowerDescription());
  
  Serial.println("===================================");
}

