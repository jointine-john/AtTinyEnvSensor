/* Sensirion SHT40 driver. Measurement is split in two so the caller can sleep
 * through the ~9 ms conversion. */
#ifndef SHT40_H
#define SHT40_H

#include <stdint.h>

void sht40_reset(void);
/* Starts a high-precision measurement. Returns 1 on ACK. */
uint8_t sht40_start(void);
/* Reads the result: temperature in centi-degC, humidity in centi-%RH.
 * Returns 1 on success (ACK + both CRCs valid). */
uint8_t sht40_read(int16_t *temp_c100, uint16_t *rh_c100);

#endif
