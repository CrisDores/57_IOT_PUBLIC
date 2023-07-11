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
        return scode;

    // init uart
    if (scode = uart.begin() != EXIT_SUCCESS)
        return scode;

    // init ble
    if (scode = ble.begin() != EXIT_SUCCESS)
        return scode;

    // init wifi
    if (scode = wf.begin() != EXIT_SUCCESS)
        return scode;

    // init firebase config // no wifi needed
    if (scode = fb.begin() != EXIT_SUCCESS)
        return scode;

    return EXIT_SUCCESS;
}

//*************************************//

uint8_t manager::work_handler()
{
    // CHECK ISR FLAGS:
    if (restart_flag)
    {
        restart_flag = false; // reset var

        wf.end();
        ble.disconnect();
        ble.stop();
        delay(200); // end delay

        esp_restart();
    }

    if (make_ota_flag)
    {
        if (cstats[0] && !cstats[1]) // wifi on ble off
        {
            if (ota.make_update() != EXIT_SUCCESS)
                restart_flag = true; // fail in OTA
            else
                return EXIT_SUCCESS; // anything to restart
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

        if (cstats[0] && !cstats[1]) // wifi on ble off
        {
            send_fb_timestamp_flag = false; // reset

            delay(100);

            scode = fb.push_timestamp();
            while (scode != EXIT_SUCCESS)
            {
                scode = fb.push_timestamp();

                static uint8_t aux;
                aux++;

                if (aux >= retry_count)
                {
                    cstats[2] = false;   // firebase not working
                    return EXIT_SUCCESS; // return anything
                }
            }

            delay(100);

            scode = fb.get_ota_status();
            while (scode != EXIT_SUCCESS)
            {
                scode = fb.get_ota_status();

                static uint8_t aux;
                aux++;

                if (aux >= retry_count)
                {
                    cstats[2] = false;   // firebase not working
                    return EXIT_SUCCESS; // return anything
                }
            }

            delay(100);

            time_collector = millis(); // reset after call
        }
    }

    if (send_fb_data_flag)
    {
        if (cstats[0] && !cstats[1]) // wifi on ble off
        {
            send_fb_data_flag = false; // reset

            delay(100);

            uint8_t scode = fb.push_data();
            while (scode != EXIT_SUCCESS)
            {
                scode = fb.push_data();

                static uint8_t aux;
                aux++;

                if (aux >= retry_count)
                {
                    cstats[2] = false;   // firebase not working
                    return EXIT_SUCCESS; // return anything
                }
            }

            delay(100);

            scode = fb.get_ota_status();
            while (scode != EXIT_SUCCESS)
            {
                scode = fb.get_ota_status();

                static uint8_t aux;
                aux++;

                if (aux >= retry_count)
                {
                    cstats[2] = false;   // firebase not working
                    return EXIT_SUCCESS; // return anything
                }
            }

            delay(100);

            time_collector = millis(); // reset the timer
        }
    }

    //***************************************************//

    uint8_t scode = 0x00; // error handler
    static uint8_t retry; // retry counter

    uint8_t work_data_aux[28];
    uint8_t work_data_aux_len = sizeof(work_data_aux) / sizeof(work_data_aux[0]);

    uint8_t calibration_data_aux[6];
    uint8_t calibration_data_aux_len = sizeof(calibration_data_aux) / sizeof(calibration_data_aux[0]);

    //--------------------------------//

    delay(5);
    scode = uart.get_pic_work_data(&work_data_aux[0], work_data_aux_len);
    delay(5);
    while (scode != EXIT_SUCCESS)
    {
        delay(5);
        scode = uart.get_pic_work_data(&work_data_aux[0], work_data_aux_len);
        delay(5);

        retry++;
        if (retry >= retry_count)
        {
            restart_flag = true;
            return EXIT_SUCCESS; // return anything
        }
    }
    retry = 0x00; // reset

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

        restart_flag = true;
        return EXIT_SUCCESS; // return anything
    }

    //--------------------------------//

    delay(5);
    scode = uart.set_pic_connection_status(cstats[0], cstats[1], cstats[2]);
    delay(5);
    while (scode != EXIT_SUCCESS)
    {
        delay(5);
        scode = uart.set_pic_connection_status(cstats[0], cstats[1], cstats[2]);
        delay(5);

        retry++;
        if (retry >= retry_count)
        {
            restart_flag = true;
            return EXIT_SUCCESS; // return anything
        }
    }
    retry = 0x00; // reset

    //--------------------------------//

    // ALL DATA CHARGED //

    for (uint8_t i = 0x00; i < work_data_len; i++)
    {
        if (work_data[i] != work_data_aux[i]) // something has changed!!
        {
            for (uint8_t j = 0x00; j < work_data_len; j++)
                work_data[j] = work_data_aux[j];

            if (cstats[1]) // ble on
            {
                ble.app_read_p->setValue(&work_data[0], work_data_len);
                ble.app_read_p->notify(true);
            }
            send_fb_data_flag = true;
        }
    }

    //--------------------------------//

    if (millis() - time_collector > TIMESTAMP_TIMING)
        send_fb_timestamp_flag = true;

    return EXIT_SUCCESS;
}