#include <manager/manager.h> // class definition completed
#include <ble/esp_ble.h>

// static manager &m = manager::getInstance(); // invoke singleton instance
// static bc I want only one "local - global" declaration

// no uso static instance porque baja mucho la velocidad de respuesta en el backlight

uint8_t esp_ble::begin()
{
    manager &m = manager::getInstance();

    NimBLEDevice::init("Detector" + m.keys[0]);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */ // ESP only

    // NimBLEDevice::setSecurityAuth(true, true, true);
    // NimBLEDevice::setSecurityPasskey(123456);
    // NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);

    pServer = NimBLEDevice::createServer();

    pServer->setCallbacks(new server_cb());
    pService = pServer->createService(SERVICE_CH_UUID);

    // NEW:
    tools_p = pService->createCharacteristic(TOOLS_CH_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ);
    bl_p = pService->createCharacteristic(BACKLIGHT_CH_UUID, NIMBLE_PROPERTY::WRITE);
    app_read_p = pService->createCharacteristic(APP_READ_CH_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

    // NEW:
    tools_p->setCallbacks(new tools_cb());
    bl_p->setCallbacks(new backlight_cb());
    app_read_p->setCallbacks(new app_read_cb());

    pService->start();
    pServer->startAdvertising();

    pServer->getAdvertising()->setMinPreferred(0x06); // exp
    pServer->getAdvertising()->setMinPreferred(0x12); // exp

    return EXIT_SUCCESS;
}
uint8_t esp_ble::stop()
{
    disconnect(); // disconnect all

    delay(10);
    NimBLEDevice::getServer()->stopAdvertising();
    NimBLEDevice::getServer()->removeService(pService);
    delay(10);

    NimBLEDevice::deinit(true); // true = clear all

    return EXIT_SUCCESS;
}
uint8_t esp_ble::disconnect()
{
    if (NimBLEDevice::getServer()->getConnectedCount() != 0x00)
    {
        std::vector<uint16_t> dev_arr = NimBLEDevice::getServer()->getPeerDevices();

        for (std::vector<uint16_t>::size_type i = 0; i != dev_arr.size(); i++)
            NimBLEDevice::getServer()->disconnect(dev_arr[i]); // disconnect all the devs
    }

    return EXIT_SUCCESS;
}
uint8_t esp_ble::disconnect(ble_gap_conn_desc *desc)
{
    if (NimBLEDevice::getServer()->getConnectedCount() != 0x00)
    {
        std::vector<uint16_t> dev_arr = NimBLEDevice::getServer()->getPeerDevices();

        for (std::vector<uint16_t>::size_type i = 0x00; i != dev_arr.size(); i++)
        {
            if (dev_arr[i] == desc->conn_handle)
                continue;
            NimBLEDevice::getServer()->disconnect(dev_arr[i]); // disconnect all the devs
        }

        NimBLEDevice::getServer()->disconnect(desc->conn_handle); // disconnect host
    }

    return EXIT_SUCCESS;
}

//-----------------------------------------------//

void server_cb::onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc)
{
    manager &m = manager::getInstance();

    m.cstats[1] = true;
    m.wf.end(); // disconnect

    pServer->updateConnParams(desc->conn_handle, 24, 48, 0, 60);
    NimBLEDevice::startAdvertising(); // multiconnect support
}
void server_cb::onDisconnect(NimBLEServer *pServer)
{
    manager &m = manager::getInstance();

    if (pServer->getConnectedCount() == 0x00)
    {
        m.cstats[1] = false;
        m.work_flag = true; // back to work in case of disconnection

        m.wf.begin(); // connect to wifi

        NimBLEDevice::startAdvertising(); // multiconnect support
                                          // pIntelligentGas->bcs_disconnected(); // doy aviso en el loop
        // WiFi.begin();
    }
}
void server_cb::onAuthenticationComplete(ble_gap_conn_desc *desc)
{
    NimBLEDevice::getServer()->disconnect(desc->conn_handle); // original
}
uint32_t server_cb::onPassKeyRequest()
{
    return 123456;
}
bool server_cb::onConfirmPIN(uint32_t pass_key)
{
    return true;
}

//-----------------------------------------------//

void tools_cb::onRead(NimBLECharacteristic *p)
{
    manager &m = manager::getInstance();

    p->setValue(m.keys[0] + ":" + m.keys[1] + ":" + m.keys[2]); // send string to app
}
void tools_cb::onWrite(NimBLECharacteristic *p, ble_gap_conn_desc *desc)
{
    manager &m = manager::getInstance();

    using namespace std;

    string request = p->getValue();

    if (request.find("57_IOT") == string::npos)                                 // SYS PROTECTION
        return;                                                                 // password not found
                                                                                //--------------------------//
    if (request.find("[") == string::npos || request.find("]") == string::npos) // SYS PROTECTION
        return;                                                                 // cmd braces not found
                                                                                //--------------------------//
    if (request.find("(") == string::npos || request.find(")") == string::npos) // SYS PROTECTION
        return;                                                                 // data braces not found
                                                                                //--------------------------//
    if (request.find("[") + 1 == request.find("]"))                             // SYS PROTECTION
        return;                                                                 // empty cmd
                                                                                //--------------------------//
    if (request.find("(") + 1 == request.find(")"))                             // SYS PROTECTION
        return;                                                                 // empty data

    string cmd = request.substr((request.find("[") + 1), (request.find("]") - request.find("[") - 1)); // extract []

    for (uint8_t i = 0; i < cmd.length(); i++)
        if (!isdigit(cmd.at(i))) // non digit system protection // abort on core 0 protection
            return;

    //----------------------------------------//----------------------------------------// SAFE CMD

    switch (stoi(cmd)) // this switch needs the braces
    {
    case 0x01: // NEW LABEL ARRIVED: "57_IOT[1](new_label)"
    {
        m.keys[2] = request.substr((request.find("(") + 1), (request.find(")") - request.find("(") - 1)); // extract ()

        m.prefs.begin("57_IOT", false);
        m.prefs.putString("NICK", m.keys[2].c_str());
        m.prefs.end();

        break;
    }
    case 0x02: // WIFI initialization from app: "57_IOT[2](ssid#pass)"
    {
        if (request.find("#") == string::npos)          // SYS PROTECTION
            return;                                     // no data divisor
                                                        //--------------------------//
        if (request.find("(") + 1 == request.find("#")) // SYS PROTECTION
            return;                                     // empty ssid
                                                        //--------------------------//
        if (request.find("#") + 1 == request.find(")")) // SYS PROTECTION
            return;                                     // empty pass

        //----------------------------------------//----------------------------------------// SAFE DATA

        string ssid = request.substr((request.find("(") + 1), (request.find("#") - request.find("(") - 1)); // extract (#
        string pass = request.substr((request.find("#") + 1), (request.find(")") - request.find("#") - 1)); // extract #)

        m.wf.new_wifi(ssid.c_str(), pass.c_str());

        m.ble.disconnect();

        break;
    }
    case 0x03: // OTA trigger from app: "57_IOT[3](nvs_status)" // "DELETE" will erase the nvs, "KEEP" will store the nvs
    {
        string nvs_status = request.substr((request.find("(") + 1), (request.find(")") - request.find("(") - 1)); // extract ()

        // IntelligentGas *pIntelligentGas = IntelligentGas::getInstance();
        // pIntelligentGas->ota_update(nvs_status); // make the update

        break;
    }
    case 0x04: // REBOOT from app: "57_IOT[4](0)" // 1 to erase nvs // 0 to mantain
    {
        string nvs_status = request.substr((request.find("(") + 1), (request.find(")") - request.find("(") - 1)); // extract ()

        for (uint8_t i = 0; i < nvs_status.length(); i++)
            if (!isdigit(nvs_status.at(i))) // non digit system protection // abort on core 0 protection
                return;

        if (stoi(nvs_status))
        {
            if (nvs_flash_erase() != ESP_OK) // erase nvs & deinitialize
            {
                esp_restart();
                delay(100);
            }
        }

        //----------------------------------------//----------------------------------------// DEVICE READY

        m.restart_flag = true;
        m.ble.disconnect();

        break;
    }
    case 0x05: // change PIC SERIAL NUMBER: "57_IOT[5](nsn)" // CHANGE the sn on PIC
    {
        string nsn = request.substr((request.find("(") + 1), (request.find(")") - request.find("(") - 1)); // extract ()

        if (nsn.length() != 0x08)
            return; // the serial number is not 8 digits

        for (uint8_t i = 0; i < nsn.length(); i++)
            if (!isdigit(nsn.at(i))) // non digit system protection // abort on core 0 protection
                return;

        uint8_t scode = m.uart.set_pic_serial_number(nsn);
        while (scode != EXIT_SUCCESS)
        {
            scode = m.uart.set_pic_serial_number(nsn);

            static uint8_t aux;
            aux++;
            if (aux >= m.retry_count)
                return;
        }

        m.prefs.begin("57_IOT", false);
        m.prefs.putString("SN", nsn.c_str());
        m.prefs.end();

        m.restart_flag = true; // restart request
        m.ble.disconnect();    // disconnect all ble devs

        break;
    }
    case 0x06: // disconnect the device // "57_IOT[6](0)" // 1 to disconnect all // 0 to disconnect one
    {
        string dis_status = request.substr((request.find("(") + 1), (request.find(")") - request.find("(") - 1)); // extract ()

        for (uint8_t i = 0; i < dis_status.length(); i++)
            if (!isdigit(dis_status.at(i))) // non digit system protection // abort on core 0 protection
                return;

        if (stoi(dis_status))
            m.ble.disconnect(desc); // disconnect all
        else
            NimBLEDevice::getServer()->disconnect(desc->conn_handle);

        // disconnect BLE
        break;
    }
    case 0x07: // start - stop working on loop // "57_IOT[6](1)" // 1 to start // 0 to stop
    {
        string work_status = request.substr((request.find("(") + 1), (request.find(")") - request.find("(") - 1)); // extract ()

        for (uint8_t i = 0; i < work_status.length(); i++)
            if (!isdigit(work_status.at(i))) // non digit system protection // abort on core 0 protection
                return;

        if (stoi(work_status))
            m.work_flag = true;

        else
            m.work_flag = false;

        break;
    }
    }
}

//-----------------------------------------------//

void app_read_cb::onRead(NimBLECharacteristic *p)
{
    manager &m = manager::getInstance();

    uint8_t scode = m.uart.get_pic_work_data(&m.work_data[0], m.work_data_len);
    while (scode != EXIT_SUCCESS)
    {
        scode = m.uart.get_pic_work_data(&m.work_data[0], m.work_data_len);

        static uint8_t aux;
        aux++;
        if (aux >= m.retry_count)
            return; // error
    }

    //   // SUCCESS

    p->setValue(&m.work_data[0], m.work_data_len);
}

//-----------------------------------------------//

void backlight_cb::onWrite(NimBLECharacteristic *p)
{
    const uint8_t *buffer = p->getValue();
    uint16_t bl = buffer[0] + (buffer[1] << 8); // reassembly 16 bytes

    uart_comms uart; // creo objeto porque es mucho mas rapido que llamar a manager
    uint8_t rounds = 0;
    while (uart.set_backlight(bl) != 0x00 && rounds < 10) // hardcoded
        rounds++;                                         // wait till success with limiter
}