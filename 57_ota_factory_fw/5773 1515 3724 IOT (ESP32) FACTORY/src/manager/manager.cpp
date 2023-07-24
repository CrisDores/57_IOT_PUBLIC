#include <manager/manager.h>

//------------------------------------------//

uint8_t manager::begin_nvs()
{
    prefs.begin("57_IOT", true); // RO
    bool nvs_status = prefs.isKey("nvs_status");

    if (!nvs_status) // si NO existe la partición
    {
        prefs.end();

        if (nvs_flash_erase() != ESP_OK)
            return 0xE1;

        if (nvs_flash_init() != ESP_OK)
            return 0xE2;

        //----------------------------------------//----------------------------------------// NVS READY

        prefs.begin("57_IOT", false); // RW

        prefs.putBool("nvs_status", true); // confirmo creacion de particion
        prefs.putString("SN", "00000000");
        prefs.putString("NICK", "_");
        prefs.end();

        esp_restart(); // FIRST INIT
        delay(500);
    }
    else // ya existe la particion, levanto datos
    {
        keys[0] = prefs.getString("SN").c_str();   // serial number from PIC
        keys[2] = prefs.getString("NICK").c_str(); // nickname
        prefs.end();
    }

    return EXIT_SUCCESS;
}

//*************************************//

uint8_t manager::begin()
{
    uint8_t scode;

    // init nvs
    if (scode = begin_nvs() != EXIT_SUCCESS)
    {
        Serial.println("NVS NOT WORKING");
        return scode;
    }

    // init uart
    if (scode = uart.begin() != EXIT_SUCCESS)
    {
        Serial.println("UART NOT WORKING");
        return scode;
    }

    // init ble
    if (scode = ble.begin() != EXIT_SUCCESS)
    {
        Serial.println("BLE NOT WORKING");
        return scode;
    }

    // init wifi
    if (scode = wf.begin() != EXIT_SUCCESS)
    {
        Serial.println("WIFI NOT WORKING");
        return scode;
    }

    // init firebase config // no wifi needed
    if (scode = fb.begin() != EXIT_SUCCESS)
    {
        Serial.println("FIREBASE NOT WORKING");
        return scode;
    }

    return EXIT_SUCCESS;
}

//*************************************//

uint8_t manager::work_handler()
{
    // CHECK ISR FLAGS:
    if (restart_flag)
    {
        restart_flag = false; // reset var
        Serial.println("RESTARTING... DISCONNECTING PROTOCOLS!");

        wf.end();
        ble.disconnect();
        ble.stop();

        delay(200); // end delay

        esp_restart();
    }

    if (make_ota_flag)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("MAKING OTA!");
            Serial.println();

            // if (NimBLEDevice::getInitialized())
            //     ble.stop();

            delay(3000);

            uint8_t res = 0x00;

            for (uint8_t attempts = 0x00; attempts < retry_count; attempts++) // GET PIC WORK DATA
            {
                uint8_t scode = ota.request_update(&res, ota.ota_fw_url);
                delay(5);

                if (scode != EXIT_SUCCESS && attempts >= retry_count)
                {
                    restart_flag = true; // OTA NOT WORKING
                    return scode;
                }
                else if (scode == EXIT_SUCCESS)
                    break; // SUCCESS
            }

            if (res != 0xE5)
            {
                Serial.println("REQUESTING RESTARTING AFTER WORK OTA ATTEMPT");
                restart_flag = true;
            }

            return EXIT_SUCCESS;
        }
    }

    if (make_factory_ota_flag)
    {
        if (WiFi.status() == WL_CONNECTED) // wifi on
        {
            Serial.println("MAKING FACTORY OTA!");
            Serial.println();

            // if (NimBLEDevice::getInitialized())
            //     ble.stop();

            delay(3000);

            uint8_t res = 0x00;

            for (uint8_t attempts = 0x00; attempts < retry_count; attempts++) // GET PIC WORK DATA
            {
                uint8_t scode = ota.request_update(&res, ota.ota_factory_fw_url);
                delay(5);

                if (scode != EXIT_SUCCESS && attempts >= retry_count)
                {
                    restart_flag = true; // OTA NOT WORKING
                    return scode;
                }
                else if (scode == EXIT_SUCCESS)
                    break; // SUCCESS
            }

            if (res != 0xE5)
            {
                Serial.println("REQUESTING RESTARTING AFTER WORK OTA ATTEMPT");
                restart_flag = true;
            }

            return EXIT_SUCCESS; // return anything
        }

        // MAKE OTA MOTHERFUCJER
    }

    if (!work_flag) // ISR ONLY
    {
        return EXIT_SUCCESS;
    }

    if (send_fb_timestamp_flag)
    {
        uint8_t scode = 0x00;

        if (WiFi.status() == WL_CONNECTED) // wifi on ble off
        {
            if (NimBLEDevice::getServer()->getConnectedCount() == 0x00)
            {
                send_fb_timestamp_flag = false; // reset

                delay(1000);

                for (uint8_t attempts = 0x00; attempts < retry_count; attempts++) // GET PIC WORK DATA
                {
                    uint8_t scode = fb.push_timestamp();
                    delay(5);

                    if (scode != EXIT_SUCCESS && attempts >= retry_count)
                    {
                        cstats[2] = false; // firebase not working
                        return scode;
                    }
                    else if (scode == EXIT_SUCCESS)
                    {
                        cstats[2] = true; // firebase working
                        break;            // SUCCESS
                    }
                }

                delay(100);

                time_collector = millis(); // reset after call
            }
        }
    }

    if (send_fb_data_flag)
    {
        if (WiFi.status() == WL_CONNECTED) // wifi on ble off
        {
            if (NimBLEDevice::getServer()->getConnectedCount() == 0x00)
            {
                send_fb_data_flag = false; // reset

                delay(1000);

                for (uint8_t attempts = 0x00; attempts < retry_count; attempts++) // GET PIC WORK DATA
                {
                    uint8_t scode = fb.push_data();
                    delay(5);

                    if (scode != EXIT_SUCCESS && attempts >= retry_count)
                    {
                        cstats[2] = false; // firebase not working
                        return scode;
                    }
                    else if (scode == EXIT_SUCCESS)
                    {
                        cstats[2] = true; // firebase working
                        break;            // SUCCESS
                    }
                }

                Serial.println("DATA PUSHED!!");

                delay(100);

                time_collector = millis(); // reset the timer
            }
        }
    }

    //***************************************************//

    uint8_t work_data_aux[28];
    uint8_t work_data_aux_len = sizeof(work_data_aux) / sizeof(work_data_aux[0]);

    uint8_t calibration_data_aux[10];
    uint8_t calibration_data_aux_len = sizeof(calibration_data_aux) / sizeof(calibration_data_aux[0]);

    uint8_t regulation_data_aux[15];
    uint8_t regulation_data_aux_len = sizeof(regulation_data_aux) / sizeof(regulation_data_aux[0]);

    //--------------------------------//

    for (uint8_t attempts = 0x00; attempts < retry_count; attempts++) // GET PIC WORK DATA
    {
        uint8_t scode = uart.get_pic_work_data(&work_data_aux[0], work_data_aux_len);
        delay(5);

        if (scode != EXIT_SUCCESS && attempts >= retry_count)
            return scode;
        else if (scode == EXIT_SUCCESS)
            break; // SUCCESS
    }

    //--------------------------------//

    std::string nsn;
    for (uint8_t i = 0; i < 4; i++) // 0 -- 3 SN
    {
        if (std::to_string(work_data_aux[i]).length() == 0x01)
            nsn.append("0" + std::to_string(work_data_aux[i]));
        else
            nsn.append(std::to_string(work_data_aux[i]));
    }

    if (nsn != keys[0])
    {
        prefs.begin("57_IOT", false);
        prefs.putString("SN", nsn.c_str());
        prefs.end();

        Serial.print("CHANGE OF SERIAL NUMBER: ");
        Serial.println(nsn.c_str());

        restart_flag = true;
        return EXIT_SUCCESS; // return anything
    }

    //--------------------------------//

    for (uint8_t attempts = 0x00; attempts < retry_count; attempts++) // GET PIC WORK DATA
    {
        uint8_t scode = uart.get_pic_calibration_data(&calibration_data_aux[0], calibration_data_aux_len);
        delay(5);

        if (scode != EXIT_SUCCESS && attempts >= retry_count)
            return scode;
        else if (scode == EXIT_SUCCESS)
            break; // SUCCESS
    }

    //--------------------------------//

    for (uint8_t attempts = 0x00; attempts < retry_count; attempts++) // GET PIC REG DATA
    {
        uint8_t scode = uart.get_pic_regulation_data(&regulation_data_aux[0], regulation_data_aux_len);
        delay(5);

        if (scode != EXIT_SUCCESS && attempts >= retry_count)
            return scode;
        else if (scode == EXIT_SUCCESS)
            break; // SUCCESS
    }

    //--------------------------------//

    for (uint8_t attempts = 0x00; attempts < retry_count; attempts++) // SET CONNECTION STATUS
    {
        uint8_t scode = uart.set_pic_connection_status(cstats[0], cstats[1], cstats[2]);
        delay(5);

        if (scode != EXIT_SUCCESS && attempts >= retry_count)
            return scode;
        else if (scode == EXIT_SUCCESS)
            break; // SUCCESS
    }

    //--------------------------------//

    // ALL DATA CHARGED //

    for (uint8_t i = 0x00; i < work_data_len; i++)
    {
        if (work_data[i] != work_data_aux[i]) // something has changed!!
        {
            for (uint8_t j = 0x00; j < work_data_len; j++)
                work_data[j] = work_data_aux[j];

            if (NimBLEDevice::getServer()->getConnectedCount() != 0x00) // ble on
            {
                ble.app_read_p->setValue(&work_data[0], work_data_len);
                ble.app_read_p->notify(true);
            }

            Serial.println("WORK INFO CHANGED!");
            send_fb_data_flag = true;

            break;
        }
    }

    //--------------------------------//

    for (uint8_t i = 0x00; i < calibration_data_aux_len; i++)
    {
        if (calibration_data[i] != calibration_data_aux[i]) // something has changed!!
        {
            for (uint8_t j = 0x00; j < calibration_data_len; j++)
                calibration_data[j] = calibration_data_aux[j];

            if (NimBLEDevice::getServer()->getConnectedCount() != 0x00) // ble connected
            {
                ble.app_cal_p->setValue(&calibration_data[0], calibration_data_len);
                ble.app_cal_p->notify(true);
            }

            break;
        }
    }

    //--------------------------------//

    for (uint8_t i = 0x00; i < regulation_data_aux_len; i++)
    {
        if (regulation_data[i] != regulation_data_aux[i]) // something has changed!!
        {
            Serial.println("REGULATION VALUES CHANGED!");

            for (uint8_t j = 0x00; j < regulation_data_len; j++)
                regulation_data[j] = regulation_data_aux[j];

            if (NimBLEDevice::getServer()->getConnectedCount() != 0x00)
            {
                ble.app_reg_p->setValue(&regulation_data[0], regulation_data_len);
                ble.app_reg_p->notify(true);
            }

            break;
        }
    }

    //--------------------------------//

    if (millis() - time_collector > TIMESTAMP_TIMING)
        send_fb_timestamp_flag = true;

    // Serial.println("LOOP WORKING");
    return EXIT_SUCCESS;
}