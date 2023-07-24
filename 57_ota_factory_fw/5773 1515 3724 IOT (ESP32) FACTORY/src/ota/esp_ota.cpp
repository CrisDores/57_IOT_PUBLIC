#include <manager/manager.h>
#include <ota/esp_ota.h>

static manager &m = manager::getInstance(); // invoke singleton instance

void esp_ota::ota_progress_cb(size_t written, size_t total)
{
    uint8_t progress = (written * 100) / total;

    Serial.print("PROGRESS: ");
    Serial.println(progress);

    if (NimBLEDevice::getServer()->getConnectedCount() != 0x00)
    {
        m.ble.ota_p->setValue("57_IOT_OTAPR:" + std::to_string(progress));
        m.ble.ota_p->notify();
    }

    delay(30); // critical
}

uint8_t esp_ota::run_update(Stream &stream, size_t total_bytes)
{
    if (!Update.begin(total_bytes))
    {
        Serial.println("NOT ENOUGH SPACE FOR NEW CODE");
        return 0xEE; // not enough space for new code
    }

    delay(50);

    if (NimBLEDevice::getServer()->getConnectedCount() != 0x00)
    {
        m.ble.ota_p->setValue("57_IOT_OTA:START");
        m.ble.ota_p->notify();
    }

    Update.onProgress(
        [this](size_t written, size_t total)
        {
            this->ota_progress_cb(written, total);
        });

    Serial.println("Update.writeStream iniciando!!");
    size_t written = Update.writeStream(stream);
    Serial.println("OPERACION COMPLETA");

    bool success = Update.end(); // writes to eBoot // critical

    delay(500);

    if (success)
    {
        Serial.println("OTA SUCCESS");

        if (NimBLEDevice::getServer()->getConnectedCount() != 0x00)
        {
            m.ble.ota_p->setValue("57_IOT_OTA:SUCCESS");
            m.ble.ota_p->notify();
        }
    }

    else
    {
        Serial.println("OTA_FAIL");

        if (NimBLEDevice::getServer()->getConnectedCount() != 0x00)
        {
            m.ble.ota_p->setValue("57_IOT_OTA:FAIL");
            m.ble.ota_p->notify();
        }
    }

    return EXIT_SUCCESS;
}

uint8_t esp_ota::request_update(uint8_t *res, std::string ota_url)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("NO WIFI");
        return 0xEE; // no wifi
    }

    if (!http.begin(ota_url.c_str()))
    {
        Serial.println("NO SE PUDO INICIAR UNA PETICION HTTP");
        return 0XEE; // http fail
    }

    // http.addHeader("authorization", "token " + gh_access_token);

    int scode = http.GET();

    //

    delay(30);

    Serial.print("HTTP_CODE: ");
    Serial.println(scode);

    if (NimBLEDevice::getServer()->getConnectedCount() != 0x00)
    {
        m.ble.ota_p->setValue("57_IOT_OTA_HTTP:" + std::to_string(scode));
        m.ble.ota_p->notify();
    }

    switch (scode)
    {
    case HTTP_CODE_OK:
        run_update(http.getStream(), http.getSize());

        break;

    case HTTP_CODE_FOUND:

        if (ota_url == ota_fw_url) // work ota
            ota_fw_url = http.getLocation().c_str();

        else if (ota_url == ota_factory_fw_url) // factory ota
            ota_factory_fw_url = http.getLocation().c_str();

        *res = 0xE5; // try again

        break;

    case HTTP_CODE_NOT_FOUND:

        break;
    }

    //

    return EXIT_SUCCESS;
}