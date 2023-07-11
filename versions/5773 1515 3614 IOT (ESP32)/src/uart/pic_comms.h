#ifndef PIC_COMMS_H
#define PIC_COMMS_H

#include <Arduino.h>

class manager; // forward declaration

#define HEADERS 0xF5
#define pic Serial2

class uart_comms
{
private:
    const uint16_t uart_timeout = 10; // 40 good // 10 original

    uint8_t uart_byte_comm(uint8_t *, uint8_t *, size_t);
    uint8_t do_chksum(uint8_t *, size_t = 8); // default 64 bytes
    uint8_t wait_bytes(uint8_t);

public:
    uint8_t begin();

    uint8_t set_pic_serial_number(std::string new_sn);

    uint8_t get_pic_work_data(uint8_t *data, uint8_t data_len); // DATA ARR: 28
    uint8_t set_pic_connection_status(uint8_t wcs, uint8_t bcs, uint8_t gcs);

    uint8_t set_backlight(uint16_t); // NOT USED
};

#endif