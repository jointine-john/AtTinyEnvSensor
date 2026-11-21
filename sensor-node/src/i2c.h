/* Bit-banged I2C master (open-drain via DDR, relies on external pull-ups). */
#ifndef I2C_H
#define I2C_H

#include <stdint.h>

#define I2C_SDA PB0
#define I2C_SCL PB2

void i2c_init(void);
void i2c_start(void);
void i2c_stop(void);
/* Returns 1 if the slave ACKed. */
uint8_t i2c_write(uint8_t byte);
uint8_t i2c_read(uint8_t ack);

#endif
