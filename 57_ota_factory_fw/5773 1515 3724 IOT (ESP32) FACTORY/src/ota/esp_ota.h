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
    //std::string ota_fw_url = "https://raw.githubusercontent.com/CrisDores/57_IOT_PUBLIC/main/57_ota_fw/firmware.bin";                 // PUBLIC REP
    //std::string ota_factory_fw_url = "https://raw.githubusercontent.com/CrisDores/57_IOT_PUBLIC/main/57_ota_factory_fw/firmware.bin"; // FACTORY

    std::string ota_fw_url = "https://github.com/CrisDores/57_IOT_PUBLIC/raw/main/57_ota_fw/firmware.bin";                 // PUBLIC REP
    std::string ota_factory_fw_url = "https://github.com/CrisDores/57_IOT_PUBLIC/raw/main/57_ota_factory_fw/firmware.bin"; // FACTORY

    // const char *ota_fw_url = "https://raw.githubusercontent.com/CrisDores/IntelligentGasESP32/main/57_ota_fw/firmware.bin"; // PRIVATE REP
    // String gh_access_token = "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"; // my personal access token // CrisDores

public:
    HTTPClient http;
    WiFiClientSecure client;

    uint8_t run_update(Stream &stream, size_t total_bytes);
    uint8_t request_update(uint8_t *res, std::string ota_url);

    void ota_progress_cb(size_t written, size_t total);
};

//----------------------------//

#endif