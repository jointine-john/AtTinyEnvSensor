/* HTU21D / SHT21 / Si7021 driver (e.g. the GY-21 breakout). All three share
 * the address, commands and data format. */
#include <avr/io.h>
#include "i2c.h"
#include "sensor.h"
#include "protocol.h"

#define HTU21D_ADDR     0x40
#define CMD_TRIGGER_T   0xF3    /* no-hold master: sensor NACKs reads until done */
#define CMD_TRIGGER_RH  0xF5
#define CMD_SOFT_RESET  0xFE

/* Temperature (14 bit) takes up to 50 ms, humidity (12 bit) up to 16 ms.
 * Poll every ~16 ms; give up after ~130 ms. */
#define POLL_TRIES      8

static uint8_t send_command(uint8_t cmd)
{
    i2c_start();
    uint8_t ok = i2c_write(HTU21D_ADDR << 1) && i2c_write(cmd);
    i2c_stop();
    return ok;
}

static uint8_t measure(uint8_t cmd, uint16_t *raw)
{
    if (!send_command(cmd))
        return 0;

    for (uint8_t i = 0; i < POLL_TRIES; i++) {
        sleep_16ms();
        i2c_start();
        if (!i2c_write((HTU21D_ADDR << 1) | 1)) {
            i2c_stop();     /* still converting */
            continue;
        }
        uint8_t buf[3];
        buf[0] = i2c_read(1);
        buf[1] = i2c_read(1);
        buf[2] = i2c_read(0);
        i2c_stop();

        if (crc8_seed(0x00, buf, 2) != buf[2])
            return 0;
        /* The bottom two bits are status, not data. */
        *raw = (((uint16_t)buf[0] << 8) | buf[1]) & 0xFFFC;
        return 1;
    }
    return 0;
}

void sensor_init(void)
{
    send_command(CMD_SOFT_RESET);
    sleep_16ms();   /* reset takes up to 15 ms */
}

uint8_t sensor_read(int16_t *temp_c100, uint16_t *rh_c100)
{
    uint16_t raw_t, raw_rh;

    if (!measure(CMD_TRIGGER_T, &raw_t) || !measure(CMD_TRIGGER_RH, &raw_rh))
        return 0;

    /* Datasheet: T = -46.85 + 175.72 * raw / 65536, RH = -6 + 125 * raw / 65536 */
    *temp_c100 = (int16_t)(((int32_t)17572 * raw_t) >> 16) - 4685;
    int16_t rh = (int16_t)(((int32_t)12500 * raw_rh) >> 16) - 600;
    if (rh < 0)
        rh = 0;
    if (rh > 10000)
        rh = 10000;
    *rh_c100 = (uint16_t)rh;
    return 1;
}
