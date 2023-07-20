#include <manager/manager.h>
#include <ota/esp_ota.h>

static manager &m = manager::getInstance(); // invoke singleton instance

void ota_progress_cb(size_t written, size_t total)
{
    Serial.print("TOTAL DE BYTES: ");
    Serial.println(total);

    Serial.print("BYTES ESCRITOS: ");
    Serial.println(written);
    Serial.println();

    // m.ble.tools_p->setValue("57_IOT_OTAPR(" + std::to_string(written) + ")");
    // m.ble.tools_p->notify(true);

    // delay(5);
}

uint8_t esp_ota::run_update(const char *ota_url)
{
    if (m.cstats[0] != true)
    {
        Serial.println("NO WIFI");
        return 0xEE; // no wifi
    }

    // client.setCACert(Cert.rootCACert);
    // http.begin(ota_fw_url, client);

    if (!http.begin(ota_url))
    {
        Serial.println("NO SE PUDO INICIAR UNA PETICION HTTP");
        return 0XEE; // http fail
    }

    // http.addHeader("authorization", "token " + gh_access_token);

    delay(5000);

    Serial.print("MEMORIA HEAP (RAM) DISPONIBLE: ");
    Serial.println(ESP.getFreeHeap());
    Serial.print("MEMORIA PSRAM DISPONIBLE: ");
    Serial.println(ESP.getFreePsram());
    Serial.println();

    int scode = http.GET();

    if (scode == HTTP_CODE_MOVED_PERMANENTLY || scode == HTTP_CODE_FOUND)
    {
        String newUrl = http.getLocation(); // Obtén la nueva URL a partir de la cabecera "Location"
        Serial.println("NUEVA UBICACION DE LA URL: ");
        Serial.println(newUrl);
        http.end(); // Cierra la conexión actual

        http.begin(newUrl); // Inicia una nueva conexión con la nueva URL
        scode = http.GET(); // Intenta la petición de nuevo
    }

    if (scode != HTTP_CODE_OK) // 302 bc https not set // 200 is ok with http
    {
        Serial.print("STATUS CODE FAILED: ");
        Serial.println(scode);
        return 0xEE; // error
    }

    Serial.println("ARRANCANDO ACTUALIZACION");

    /*
            Update.begin()

            params:
                - size of new code
                - command (the place where the ota goes)
                (0 for flash and 100 for SPIFFS) default 0
    */

    if (!Update.begin(http.getSize()))
    {
        Serial.println("NOT ENOUGH SPACE FOR NEW CODE");
        return 0xEE; // not enough space for new code
    }

    //m.ble.stop();
    
    delay(1000);

    //m.ble.tools_p->setValue("57_IOT_RUNNING_OTA");
    //m.ble.tools_p->notify(true);

    Update.onProgress(ota_progress_cb);

    Serial.println("Update.writeStream iniciando!!");
    size_t written = Update.writeStream(http.getStream());

    Serial.println("OPERACION COMPLETA");

    if (Update.end())
    {
        Serial.println("Actualización completada");
        if (Update.isFinished())
        {
            //m.ble.tools_p->setValue("57_IOT_OTA_SUCCESS");
            //m.ble.tools_p->notify(true);

            Serial.println("Solicitando reinicio...");
            m.restart_flag = true; // restart request
            delay(200);
        }
        else
        {
            Serial.println("La actualización no se completó. Algo salió mal.");
        }
    }
    else
    {
        Serial.printf("Error de actualización: %s\n", Update.errorString());
    }

    return EXIT_SUCCESS;
}