/*
 * Over-the-air protocol shared by the sensor node (ATtiny85) and the receiver (ESP32).
 *
 * Physical layer: 433.92 MHz OOK, Manchester encoded (G.E. Thomas convention:
 * the first half-bit carries the bit value, the second half its inverse),
 * MSB first.
 *
 * Frame:  [preamble 0xAA x4][sync 0x2D][payload 8 bytes][crc8][trailer bit 0]
 *
 * Payload (big-endian):
 *   0     node id
 *   1     sequence number (wraps)
 *   2..3  temperature, int16, centi-degrees C (PROTO_TEMP_INVALID on sensor error)
 *   4..5  relative humidity, uint16, centi-percent
 *   6..7  battery voltage, uint16, millivolts
 *
 * Each frame is sent PROTO_REPEATS times; the receiver de-duplicates on (node, seq).
 */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define PROTO_HALF_BIT_US     500     /* 1000 bit/s */
#define PROTO_PREAMBLE_BYTE   0xAA
#define PROTO_PREAMBLE_LEN    4
#define PROTO_SYNC_BYTE       0x2D
#define PROTO_SYNC_WORD       ((PROTO_PREAMBLE_BYTE << 8) | PROTO_SYNC_BYTE)
#define PROTO_PAYLOAD_LEN     8
#define PROTO_PACKET_LEN      (PROTO_PAYLOAD_LEN + 1)
#define PROTO_REPEATS         3
#define PROTO_REPEAT_GAP_MS   20

#define PROTO_TEMP_INVALID    0x7FFF

/* CRC-8, polynomial 0x31, init 0xFF (same CRC the SHT4x uses). */
static inline uint8_t crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;
    while (len--) {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8; i++)
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
    }
    return crc;
}

#endif
