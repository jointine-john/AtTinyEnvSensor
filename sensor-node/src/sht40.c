#include <avr/io.h>
#include <util/delay.h>
#include "i2c.h"
#include "sht40.h"
#include "protocol.h"

#define SHT40_ADDR        0x44
#define CMD_MEASURE_HIGH  0xFD
#define CMD_SOFT_RESET    0x94

static uint8_t send_command(uint8_t cmd)
{
    i2c_start();
    uint8_t ok = i2c_write(SHT40_ADDR << 1) && i2c_write(cmd);
    i2c_stop();
    return ok;
}

void sht40_reset(void)
{
    send_command(CMD_SOFT_RESET);
    _delay_ms(1);
}

uint8_t sht40_start(void)
{
    return send_command(CMD_MEASURE_HIGH);
}

uint8_t sht40_read(int16_t *temp_c100, uint16_t *rh_c100)
{
    uint8_t buf[6];

    i2c_start();
    if (!i2c_write((SHT40_ADDR << 1) | 1)) {
        i2c_stop();
        return 0;
    }
    for (uint8_t i = 0; i < 6; i++)
        buf[i] = i2c_read(i < 5);
    i2c_stop();

    if (crc8(&buf[0], 2) != buf[2] || crc8(&buf[3], 2) != buf[5])
        return 0;

    uint16_t raw_t = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t raw_rh = ((uint16_t)buf[3] << 8) | buf[4];

    /* Datasheet: T = -45 + 175 * raw / 65535, RH = -6 + 125 * raw / 65535 */
    *temp_c100 = (int16_t)(((int32_t)17500 * raw_t) >> 16) - 4500;
    int16_t rh = (int16_t)(((int32_t)12500 * raw_rh) >> 16) - 600;
    if (rh < 0)
        rh = 0;
    if (rh > 10000)
        rh = 10000;
    *rh_c100 = (uint16_t)rh;
    return 1;
}
