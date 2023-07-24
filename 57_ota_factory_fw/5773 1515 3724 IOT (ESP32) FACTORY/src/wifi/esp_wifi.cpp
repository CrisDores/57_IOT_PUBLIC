#include <manager/manager.h>
#include <wifi/esp_wifi.h>

static manager &m = manager::getInstance(); // invoke singleton instance

uint8_t esp_wifi::begin()
{
    WiFi.mode(WIFI_MODE_STA);

    wcs_connected_id = WiFi.onEvent(
        [this](WiFiEvent_t event, WiFiEventInfo_t info)
        {
            this->wcs_connected(event, info);
        },
        ARDUINO_EVENT_WIFI_STA_CONNECTED);
    wcs_disconnected_id = WiFi.onEvent(
        [this](WiFiEvent_t event, WiFiEventInfo_t info)
        {
            this->wcs_disconnected(event, info);
        },
        ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

    WiFi.begin();

    return EXIT_SUCCESS;
}
uint8_t esp_wifi::end()
{
    WiFi.removeEvent(wcs_connected_id);
    WiFi.removeEvent(wcs_disconnected_id);

    WiFi.disconnect();
    return EXIT_SUCCESS;
}

void esp_wifi::wcs_connected(arduino_event_id_t event, arduino_event_info_t info)
{
    Serial.println("(ISR)WIFI CONNECTED");
    m.cstats[0] = true; // wifi connected

    delay(1000);

    if (NimBLEDevice::getServer()->getConnectedCount() != 0x00) // ble connected
    {
        m.ble.tools_p->setValue("WCS_CONNECTED:");
        m.ble.tools_p->notify();
    }

    m.send_fb_timestamp_flag = true; // send timestamp
}
void esp_wifi::wcs_disconnected(arduino_event_id_t event, arduino_event_info_t info)
{
    Serial.println("(ISR)WIFI DISCONNECTED");

    uint8_t desc_reason = info.wifi_sta_disconnected.reason;

    m.cstats[0] = false; // wifi disconnected
    m.cstats[2] = false; // firebase not working (obvious)

    Serial.print("(ISR)DISCONNECTED REASON: ");
    Serial.println(std::to_string(desc_reason).c_str());

    delay(1000);

    if (NimBLEDevice::getServer()->getConnectedCount() != 0x00) // ble connected
    {
        m.ble.tools_p->setValue("WCS_DISCONNECTED:" + std::to_string(desc_reason));
        m.ble.tools_p->notify();
    }
}