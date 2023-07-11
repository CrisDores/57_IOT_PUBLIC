#include <manager/manager.h>
#include <ota/esp_ota.h>

static manager &m = manager::getInstance(); // invoke singleton instance

uint8_t esp_ota::make_update()
{
    if (m.cstats[0] != true)
        return 0xEE; // no wifi

    // client.setCACert(Cert.rootCACert);
    // http.begin(ota_fw_url, client);
    http.begin(ota_fw_url);

    // http.addHeader("authorization", "token " + gh_access_token);

    int scode = http.GET();

    if (scode == HTTP_CODE_MOVED_PERMANENTLY || scode == HTTP_CODE_FOUND)
    {
        String newUrl = http.getLocation(); // Obtén la nueva URL a partir de la cabecera "Location"
        http.end();         // Cierra la conexión actual

        http.begin(newUrl); // Inicia una nueva conexión con la nueva URL
        scode = http.GET(); // Intenta la petición de nuevo
    }

    if (scode != HTTP_CODE_OK) // 302 bc https not set // 200 is ok with http
        return 0xEE; // error

    /*
            Update.begin()

            params:
                - size of new code
                - command (the place where the ota goes)
                (0 for flash and 100 for SPIFFS) default 0
    */

    if (!Update.begin(http.getSize()))
        return 0xEE; // not enough space for new code

    m.ble.stop();
    delay(1000);

    size_t written = Update.writeStream(http.getStream());

    if (Update.end())
    {
        if (Update.isFinished())
        {
            m.restart_flag = true; // restart request
            delay(200);
        }
    }

    return EXIT_SUCCESS;
}