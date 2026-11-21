/* Temperature/humidity sensor interface. Exactly one driver is compiled in,
 * selected by the PlatformIO environment (sht40.c or htu21d.c). */
#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>

/* Provided by main.c: power-down sleep for one ~16 ms watchdog period, so
 * drivers can wait for conversions without burning current. */
void sleep_16ms(void);

void sensor_init(void);
/* Takes one reading: temperature in centi-degC, humidity in centi-%RH.
 * Returns 1 on success (ACKs and CRCs all valid). */
uint8_t sensor_read(int16_t *temp_c100, uint16_t *rh_c100);

#endif
