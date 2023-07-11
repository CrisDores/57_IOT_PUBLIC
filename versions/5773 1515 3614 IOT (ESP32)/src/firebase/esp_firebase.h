#ifndef ESP_FIREBASE_H
#define ESP_FIREBASE_H

//----------------------------//

#include <Arduino.h>
#include "certs/firebase_cert.h"
#include <HTTPClient.h>
#include <time.h>
#include <ArduinoJson.h>

class manager; // forward declaration

//--//

class esp_firebase
{
    WiFiClientSecure client;
    HTTPClient http;
    firebase_cert Cert;
    StaticJsonDocument<128> data;

    // timestamp things
    time_t timestamp;
    struct tm timeinfo;

    std::string push_fb_url;
    std::string get_ota_fb_url = "https://eiot-fe00c-default-rtdb.firebaseio.com/VERSION.json";

private:
    const char *ntp_server = "pool.ntp.org";
    const char *FB_HEADER_URL = "https://eiot-fe00c-default-rtdb.firebaseio.com/";
    const char *FB_API_KEY = "AIzaSyCRxbQiRUMlNnnuwqAKVDilsXfHojitRSk";

    //--//

public:
    uint8_t begin();

    uint8_t push_timestamp();
    uint8_t push_data();
    uint8_t get_ota_status();
};

//----------------------------//

#endif