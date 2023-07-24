#include <manager/manager.h>
#include <firebase/esp_firebase.h>

static manager &m = manager::getInstance(); // invoke singleton instance

uint8_t esp_firebase::begin()
{
    push_fb_url = std::string(FB_HEADER_URL) + m.keys[0] + ".json?auth=" + std::string(FB_API_KEY);

    client.setCACert(Cert.rootCACert); // SSL cert
    http.setReuse(true);               // keep alive connection

    configTime(0, 0, m.fb.ntp_server);

    return EXIT_SUCCESS;
}

uint8_t esp_firebase::push_timestamp()
{
    if (WiFi.status() != WL_CONNECTED)
        return 0xE3; // no wifi

    StaticJsonDocument<64> json_timestamp;
    if (!getLocalTime(&timeinfo)) // get timestamp
        return 0xE4;
    time(&timestamp);
    json_timestamp["timestamp"] = timestamp;

    //******//

    http.begin(client, push_fb_url.c_str());

    http.addHeader("Content-Type", "application/json");
    int response_code = http.sendRequest("PATCH", json_timestamp.as<String>());

    // String response_body = http.getString(); // download the response

    http.end();

    if (response_code != 200)
        return 0xE5; // wrong status code

    return EXIT_SUCCESS;
}
uint8_t esp_firebase::push_data()
{
    if (WiFi.status() != WL_CONNECTED)
        return 0xEE; // no wifi

    StaticJsonDocument<128> json_data;
    if (!getLocalTime(&timeinfo)) // get timestamp
        return 0xEE;
    time(&timestamp);
    json_data["timestamp"] = timestamp;

    //******//

    json_data["event"] = m.work_data[4];                       // event
    json_data["CO"] = m.work_data[5] + (m.work_data[6] << 8);  // monoxide
    json_data["GAS"] = m.work_data[7] + (m.work_data[8] << 8); // methane
    json_data["version"] = m.keys[1];
    json_data["label"] = m.keys[2]; // device nickname

    //******//

    http.begin(client, push_fb_url.c_str());

    http.addHeader("Content-Type", "application/json");
    int response_code = http.sendRequest("PATCH", json_data.as<String>());

    // String response_body = http.getString(); // download the response

    http.end();

    if (response_code != 200)
        return 0xEE;

    return EXIT_SUCCESS;
}