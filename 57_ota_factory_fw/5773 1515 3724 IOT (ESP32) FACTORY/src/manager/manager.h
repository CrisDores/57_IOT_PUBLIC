#ifndef MANAGER_H
#define MANAGER_H

//----------------------------//

#include <Preferences.h> // nvs manage
#include <nvs_flash.h>   // nvs manage

#include <ble/esp_ble.h>
#include <uart/pic_comms.h>
#include <wifi/esp_wifi.h>
#include <firebase/esp_firebase.h>
#include <ota/esp_ota.h>

class manager
{
    //****************************************************************// SINGLETON
private:
    manager() {} // ensure only one instance
public:
    manager(manager const &) = delete;
    void operator=(manager const &) = delete;

    // Método para obtener la instancia del Singleton
    static manager &getInstance()
    {
        static manager instance; // Guaranteed to be destroyed.
                                 // Instantiated on first use.
        return instance;
    }

    //****************************************************************// SINGLETON

private:
    uint8_t begin_nvs();

public:
    Preferences prefs;

    esp_ble ble;
    uart_comms uart;
    esp_wifi wf;
    esp_firebase fb;
    esp_ota ota;

    const uint8_t retry_count = 10;
    const uint32_t TIMESTAMP_TIMING = 900000; // 15 minutos 900000
    uint64_t time_collector = 0x00;

    volatile bool cstats[3] = {false, false, false}; // [wcs, bcs, gcs]
    uint8_t cstats_len = sizeof(cstats) / sizeof(cstats[0]);

    std::string keys[3] = {"", "230724D_F", ""}; // [serial number, code version, nickname]
    //std::string keys[3] = {"", "000000", ""}; // [serial number, code version, nickname]

    uint8_t work_data[28];
    uint8_t work_data_len = sizeof(work_data) / sizeof(work_data[0]);

    uint8_t calibration_data[10];
    uint8_t calibration_data_len = sizeof(calibration_data) / sizeof(calibration_data[0]);

    uint8_t regulation_data[15];
    uint8_t regulation_data_len = sizeof(regulation_data) / sizeof(regulation_data[0]);

    // ISR FLAGS:
    volatile bool restart_flag = false;
    volatile bool work_flag = true;
    volatile bool send_fb_timestamp_flag = false;
    volatile bool send_fb_data_flag = false;
    volatile bool make_ota_flag = false;
    volatile bool make_factory_ota_flag = false;

    //**************//

    uint8_t begin();
    uint8_t work_handler();
};

//----------------------------//

#endif