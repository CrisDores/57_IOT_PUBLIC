#include <manager/manager.h>
#include <uart/pic_comms.h>

// using ref_class = uart_comms; // FUNCIONA

uint8_t uart_comms::begin()
{
    manager &m = manager::getInstance(); // invoke singleton instance
    uint8_t scode = 0x00;

    pic.begin(500000); // ciudado con las altas velocidades

    scode = set_pic_connection_status(m.cstats[0], m.cstats[1], m.cstats[2]);
    while (scode != EXIT_SUCCESS)
    {
        scode = set_pic_connection_status(m.cstats[0], m.cstats[1], m.cstats[2]);

        static uint8_t aux;
        aux++;
        if (aux >= m.retry_count)
            return 0x06;
    }

    scode = get_pic_work_data(&m.work_data[0], m.work_data_len);
    while (scode != EXIT_SUCCESS)
    {
        scode = get_pic_work_data(&m.work_data[0], m.work_data_len);

        static uint8_t aux;
        aux++;
        if (aux >= m.retry_count)
            return 0x06;
    }

    return EXIT_SUCCESS;
}
uint8_t uart_comms::do_chksum(uint8_t *arr, size_t arr_len)
{
    uint8_t chksum_res = 0;
    for (uint8_t i = 1; i < arr_len - 2; i++)
        chksum_res ^= arr[i];
    return chksum_res;
}
uint8_t uart_comms::wait_bytes(uint8_t t_bytes)
{
    unsigned long ELT = millis();
    while (pic.available() < t_bytes)
    {
        if (millis() - ELT > uart_timeout) // timeout
            return 0xE2;
        // return a timeout
    }
    return 0x00; // success // all bytes in
}
uint8_t uart_comms::uart_byte_comm(uint8_t *cmd, uint8_t *rply, size_t len)
{
    while (pic.available())
        pic.read(); // clear buff

    pic.write(cmd, len); // SENDING TO PIC

    uint8_t scode = wait_bytes(len); // loop here till all bytes are in
    if (scode != 0x00)
    {
        for (uint8_t i = 0; i < len; i++)
            rply[i] = scode;
        return scode;
    }

    for (uint8_t i = 0; i < len; i++)
        rply[i] = pic.read(); // RECEIVING FROM PIC

    //////

    return 0x00; // return the info
}

uint8_t uart_comms::set_pic_serial_number(std::string new_sn)
{
    uint8_t cmd[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // to PIC
    uint8_t cmd_len = sizeof(cmd) / sizeof(cmd[0]);

    uint8_t rply[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // from PIC
    uint8_t rply_len = sizeof(rply) / sizeof(rply[0]);

    uint8_t scode = 0x00; // status code // handles errors

    cmd[0] = HEADERS;
    cmd[1] = 0xA0;                      // set serial number command
    cmd[2] = stoi(new_sn.substr(0, 2)); // year
    cmd[3] = stoi(new_sn.substr(2, 2)); // month
    cmd[4] = stoi(new_sn.substr(4, 2)); // day
    cmd[5] = stoi(new_sn.substr(6, 2)); // position
    cmd[7] = HEADERS;

    cmd[6] = do_chksum(cmd, cmd_len);

    scode = uart_byte_comm(&cmd[0], &rply[0], cmd_len); // UART COMMS

    if (scode != 0x00)
        return scode; // comms error code

    if (rply[0] != HEADERS || rply[rply_len - 1] != HEADERS)
        return 0xE1; // wrong data

    if (rply[rply_len - 2] != do_chksum(rply, rply_len))
        return 0xE1; // wrong data

    if (rply[rply_len - 3] != 0x00)
        return rply[rply_len - 3]; // pic error

    return 0x00; // SUCCESS
}

uint8_t uart_comms::get_pic_work_data(uint8_t *data, uint8_t data_len)
{
    manager &m = manager::getInstance(); // invoke singleton instance

    uint8_t cmd[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // to PIC
    uint8_t cmd_len = sizeof(cmd) / sizeof(cmd[0]);

    uint8_t rply[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // from PIC
    uint8_t rply_len = sizeof(rply) / sizeof(rply[0]);

    uint8_t scode = 0x00; // status code // handles errors

    cmd[0] = HEADERS;
    cmd[1] = 0xC1; // get work data command
    cmd[2] = 0x00; // empty
    cmd[3] = 0x00; // empty
    cmd[4] = 0x00; // empty
    cmd[5] = 0x00;
    cmd[7] = HEADERS;

    cmd[6] = do_chksum(cmd, cmd_len);

    scode = uart_byte_comm(&cmd[0], &rply[0], cmd_len); // UART COMMS

    if (scode != 0x00)
        return scode; // comms error code

    if (rply[0] != HEADERS || rply[rply_len - 1] != HEADERS)
    {
        for (uint8_t i = 0; i < data_len; i++)
            data[i] = 0xE1; // wrong data
        return 0xE1;        // wrong data
    }

    if (rply[rply_len - 2] != do_chksum(rply, rply_len))
    {
        for (uint8_t i = 0; i < data_len; i++)
            data[i] = 0xE1; // wrong data
        return 0xE1;        // wrong data
    }

    if (rply[rply_len - 3] != 0x00)
    {
        for (uint8_t i = 0; i < data_len; i++)
            data[i] = 0xE1;        // wrong data
        return rply[rply_len - 3]; // pic error
    }

    uint8_t payload[rply[2]]; // VLA seems to work fine
    uint8_t payload_len = rply[2];

    scode = wait_bytes(payload_len); // loop here till all bytes are in
    if (scode != 0x00)
    {
        for (uint8_t i = 0; i < payload_len; i++)
            rply[i] = scode;
        return scode;
    }

    for (uint8_t i = 0; i < payload_len; i++)
        payload[i] = pic.read(); // get the payload of PIC

    if (payload[0] != HEADERS || payload[payload_len - 1] != HEADERS)
    {
        for (uint8_t i = 0; i < data_len; i++)
            data[i] = 0xE3; // wrong data
        return 0xE3;        // wrong data
    }

    if (payload[payload_len - 2] != do_chksum(payload, payload_len))
    {
        for (uint8_t i = 0; i < data_len; i++)
            data[i] = 0xE3; // wrong data
        return 0xE3;        // wrong data
    }

    ///////////////////////////////////////////
    for (uint8_t i = 0; i < data_len; i++)
        data[i] = payload[i + 2]; // charge data

    ///////////////////////////////////////////////////////

    return 0x00; // SUCCESS
}
uint8_t uart_comms::set_pic_connection_status(uint8_t wcs, uint8_t bcs, uint8_t gcs) // WCS // BCS // GCS
{
    manager &m = manager::getInstance(); // invoke singleton instance

    uint8_t cmd[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // to PIC
    uint8_t cmd_len = sizeof(cmd) / sizeof(cmd[0]);

    uint8_t rply[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // from PIC
    uint8_t rply_len = sizeof(rply) / sizeof(rply[0]);

    uint8_t scode = 0x00; // status code // handles errors

    cmd[0] = HEADERS;
    cmd[1] = 0xC2; // set reg cmd
    cmd[2] = wcs;  // wcs
    cmd[3] = bcs;  // bcs
    cmd[4] = gcs;  // gcs
    cmd[5] = 0x00;
    cmd[7] = HEADERS;

    cmd[6] = do_chksum(cmd, cmd_len);

    scode = uart_byte_comm(&cmd[0], &rply[0], cmd_len); // UART COMMS

    if (scode != 0x00)
        return scode; // comms error code

    if (rply[0] != HEADERS || rply[rply_len - 1] != HEADERS)
        return 0xE1; // wrong data

    if (rply[rply_len - 2] != do_chksum(rply, rply_len))
        return 0xE1; // wrong data

    if (rply[rply_len - 3] != 0x00)
        return rply[rply_len - 3]; // pic error

    return 0x00; // SUCCESS
}

uint8_t uart_comms::set_backlight(uint16_t nbl)
{
    uint8_t nbl_l = (uint8_t)nbl; // LOW PART // HIGH PART DISCARDED
    uint8_t nbl_h = nbl >> 8;     // HIGH PART // MSB SHIFTED TO LSB, SO LOW PART DISCARDED

    uint8_t cmd[8] = {HEADERS, 0xC3, nbl_l, nbl_h, 0x00, 0x00, 0x00, HEADERS}; // to PIC
    uint8_t cmd_len = sizeof(cmd) / sizeof(cmd[0]);

    uint8_t rply[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // from PIC
    uint8_t rply_len = sizeof(rply) / sizeof(rply[0]);

    cmd[cmd_len - 2] = do_chksum(cmd, cmd_len); // ASSIGN THE CHKSUM

    uint8_t scode = uart_byte_comm(&cmd[0], &rply[0], cmd_len);
    if (scode != 0x00)
        return scode; // comms error code

    if (rply[0] != 0xF5 || rply[7] != 0xF5 || rply[6] != do_chksum(rply, sizeof(rply)))
        return 0xE1; // wrong data in this case

    if (rply[5] != 0x00)
        return rply[rply_len - 3]; // return pic error

    return 0x00; // SUCESS AND NO PIC ERRORS
}

/////////////////////////////////////////////////////////////////////////////////////////