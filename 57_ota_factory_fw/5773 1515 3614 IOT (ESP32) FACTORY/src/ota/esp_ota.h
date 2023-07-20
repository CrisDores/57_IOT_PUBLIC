#ifndef ESP_OTA_H
#define ESP_OTA_H

//----------------------------//

#include <Arduino.h>
#include <HTTPClient.h>
#include <Update.h>
#include <certs/ota_cert.h>

class manager; // forward declaration

class esp_ota
{
    public:
        const char *ota_fw_url = "https://raw.githubusercontent.com/CrisDores/57_IOT_PUBLIC/main/57_ota_fw/firmware.bin"; // PUBLIC REP
        const char *ota_factory_fw_url = "https://raw.githubusercontent.com/CrisDores/57_IOT_PUBLIC/main/57_ota_factory_fw/firmware.bin"; // FACTORY

        //const char *ota_fw_url = "https://raw.githubusercontent.com/CrisDores/IntelligentGasESP32/main/57_ota_fw/firmware.bin"; // PRIVATE REP
        //String gh_access_token = "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"; // my personal access token // CrisDores

    public:

    HTTPClient http;
    WiFiClientSecure client;
    ota_cert Cert;

    uint8_t run_update(const char* ota_url);
};

//----------------------------//

#endif