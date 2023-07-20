#ifndef ESP_BLE_H
#define ESP_BLE_H

//----------------------------//

#include <Arduino.h>
#include <NimBLEDevice.h>

class manager; // forward declaration

//--//

class esp_ble
{
    const char *SERVICE_CH_UUID = "dd249079-0ce8-4d11-8aa9-53de4040aec6";
    const char *TOOLS_CH_UUID = "89925840-3d11-4676-bf9b-62961456b570";
    const char *KEYS_CH_UUID = "db34bdb0-c04e-4c02-8401-078fb22ef46a";
    const char *BACKLIGHT_CH_UUID = "12d3c6a1-f86e-4d5b-89b5-22dc3f5c831f";
    const char *APP_READ_CH_UUID = "6869fe94-c4a2-422a-ac41-b2a7a82803e9";
    const char *APP_CAL_CH_UUID = "0147ab2a-3987-4bb8-802b-315a664eadd6";
    const char *APP_REG_CH_UUID = "961d1cdd-028f-47d0-aa2a-e0095e387f55";

    NimBLEServer *pServer;   // BLE VARS
    NimBLEService *pService; // BLE VARS

public:
    NimBLECharacteristic *tools_p; // tools pointer
    NimBLECharacteristic *keys_p;
    NimBLECharacteristic *bl_p;       // backlight pointer
    NimBLECharacteristic *app_read_p; // app read pointer
    NimBLECharacteristic *app_cal_p;  // app calibration pointer
    NimBLECharacteristic *app_reg_p;  // app regulation pointer

    uint8_t begin();
    uint8_t stop();

    uint8_t disconnect();                        // disconnect all
    uint8_t disconnect(ble_gap_conn_desc *desc); // disconnect all // connection handler last
};

class server_cb : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc);
    void onDisconnect(NimBLEServer *pServer);
    void onAuthenticationComplete(ble_gap_conn_desc *desc);
    uint32_t onPassKeyRequest();
    bool onConfirmPIN(uint32_t pass_key);
};

class tools_cb : public NimBLECharacteristicCallbacks
{
    void onRead(NimBLECharacteristic *p);
    void onWrite(NimBLECharacteristic *p, ble_gap_conn_desc *desc);
};

class keys_cb : public NimBLECharacteristicCallbacks
{
    void onRead(NimBLECharacteristic *p);
};

class app_read_cb : public NimBLECharacteristicCallbacks
{
    void onRead(NimBLECharacteristic *p);
    void onSubscribe(NimBLECharacteristic *p, ble_gap_conn_desc *desc, uint16_t sub_val);
};

class backlight_cb : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *p);
};

class app_cal_cb : public NimBLECharacteristicCallbacks
{
    void onRead(NimBLECharacteristic *p);
    void onWrite(NimBLECharacteristic *p);
};

class app_reg_cb : public NimBLECharacteristicCallbacks
{
    void onRead(NimBLECharacteristic *p);
    void onWrite(NimBLECharacteristic *p);
};

//----------------------------//

#endif