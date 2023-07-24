#ifndef ESP_WIFI_H
#define ESP_WIFI_H

#include <Arduino.h>
#include <WiFi.h>

class manager; // forward declaration

class esp_wifi
{
private:
    wifi_event_id_t wcs_connected_id;
    wifi_event_id_t wcs_disconnected_id;

    void wcs_connected(arduino_event_id_t event, arduino_event_info_t info);
    void wcs_disconnected(arduino_event_id_t event, arduino_event_info_t info);

public:
    uint8_t begin();
    uint8_t end();
};

#endif