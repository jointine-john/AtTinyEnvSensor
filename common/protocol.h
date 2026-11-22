/*
 * Over-the-air protocol shared by the sensor nodes (ATtiny) and the receiver (ESP32).
 * See docs/protocol.md for the full description.
 *
 * Physical layer: 433.92 MHz OOK, Manchester encoded (G.E. Thomas convention:
 * the first half-bit carries the bit value, the second half its inverse),
 * MSB first.
 *
 * Frame:  [preamble 0xAA x4][sync 0xD4][len][body: len bytes][crc8][trailer bit 0]
 *
 * len counts the body only. The CRC covers len and the body.
 *
 * Body (big-endian):
 *   0     node id
 *   1     sequence number (wraps)
 *   2..3  battery voltage, uint16, millivolts (0 = not battery powered)
 *   4..   zero or more readings, each [type][value]
 *
 * The top two bits of a reading's type give its value size (1, 2 or 4 bytes),
 * so a receiver can skip types it doesn't know. Size code 3 is reserved.
 *
 * Each frame is sent PROTO_REPEATS times; the receiver de-duplicates on (node, seq).
 */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define PROTO_HALF_BIT_US     500     /* 1000 bit/s */
#define PROTO_PREAMBLE_BYTE   0xAA
#define PROTO_PREAMBLE_LEN    4
/* 0xD4 rather than the original format's 0x2D, so receivers ignore nodes
 * still running the old fixed-length firmware instead of misreading them. */
#define PROTO_SYNC_BYTE       0xD4
#define PROTO_SYNC_WORD       ((PROTO_PREAMBLE_BYTE << 8) | PROTO_SYNC_BYTE)
#define PROTO_REPEATS         3
#define PROTO_REPEAT_GAP_MS   20

#define PROTO_HEADER_LEN      4       /* node, seq, battery */
#define PROTO_MAX_BODY_LEN    48
/* len byte + body + CRC */
#define PROTO_MAX_PACKET_LEN  (1 + PROTO_MAX_BODY_LEN + 1)

/* Reading types: size code in bits 7-6, id in bits 5-0. */
#define PROTO_SIZE_1          0x00
#define PROTO_SIZE_2          0x40
#define PROTO_SIZE_4          0x80

/* uint8 status flags, sent only when something is wrong. */
#define PROTO_STATUS          (PROTO_SIZE_1 | 0x01)
#define PROTO_STATUS_SENSOR_ERROR  0x01   /* T/H sensor didn't answer or failed CRC */

/* int16, centi-degrees C */
#define PROTO_TEMPERATURE     (PROTO_SIZE_2 | 0x01)
/* uint16, centi-percent RH */
#define PROTO_HUMIDITY        (PROTO_SIZE_2 | 0x02)
/* uint16, deci-hPa (station pressure, not sea-level corrected) */
#define PROTO_PRESSURE        (PROTO_SIZE_2 | 0x03)
/* uint16, centi-m/s, average over the reporting interval */
#define PROTO_WIND_SPEED      (PROTO_SIZE_2 | 0x04)
/* uint16, centi-m/s, highest short-window average in the interval */
#define PROTO_WIND_GUST       (PROTO_SIZE_2 | 0x05)
/* uint16, deci-degrees from north, 0-3599 */
#define PROTO_WIND_DIRECTION  (PROTO_SIZE_2 | 0x06)
/* uint16 tip count since the node started (wraps), then uint16 rainfall per
 * tip in micrometres. A running count means a lost packet loses no rain. */
#define PROTO_RAIN            (PROTO_SIZE_4 | 0x07)

/* Value size in bytes for a reading type, or 0 for the reserved size code. */
static inline uint8_t proto_value_size(uint8_t type)
{
    uint8_t code = type >> 6;
    return code == 3 ? 0 : (uint8_t)(1 << code);
}

/* CRC-8, polynomial 0x31. The packet and the SHT4x start from 0xFF,
 * the HTU21D/SHT21/Si7021 from 0x00. */
static inline uint8_t crc8_seed(uint8_t crc, const uint8_t *data, uint8_t len)
{
    while (len--) {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8; i++)
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
    }
    return crc;
}

static inline uint8_t crc8(const uint8_t *data, uint8_t len)
{
    return crc8_seed(0xFF, data, len);
}

/* --- Building a packet (used by the nodes, the loopback test and the PC test) ---
 *
 *   uint8_t pkt[PROTO_MAX_PACKET_LEN];
 *   uint8_t n = proto_begin(pkt, node, seq, battery_mv);
 *   n = proto_put16(pkt, n, PROTO_TEMPERATURE, (uint16_t)temp);
 *   n = proto_end(pkt, n);          // n is now the number of bytes to send
 *
 * The caller must not add more than PROTO_MAX_BODY_LEN bytes of body.
 */
static inline uint8_t proto_begin(uint8_t *pkt, uint8_t node, uint8_t seq, uint16_t battery_mv)
{
    pkt[1] = node;
    pkt[2] = seq;
    pkt[3] = (uint8_t)(battery_mv >> 8);
    pkt[4] = (uint8_t)battery_mv;
    return 1 + PROTO_HEADER_LEN;
}

static inline uint8_t proto_put8(uint8_t *pkt, uint8_t n, uint8_t type, uint8_t value)
{
    pkt[n] = type;
    pkt[n + 1] = value;
    return (uint8_t)(n + 2);
}

static inline uint8_t proto_put16(uint8_t *pkt, uint8_t n, uint8_t type, uint16_t value)
{
    pkt[n] = type;
    pkt[n + 1] = (uint8_t)(value >> 8);
    pkt[n + 2] = (uint8_t)value;
    return (uint8_t)(n + 3);
}

static inline uint8_t proto_put32(uint8_t *pkt, uint8_t n, uint8_t type, uint32_t value)
{
    pkt[n] = type;
    pkt[n + 1] = (uint8_t)(value >> 24);
    pkt[n + 2] = (uint8_t)(value >> 16);
    pkt[n + 3] = (uint8_t)(value >> 8);
    pkt[n + 4] = (uint8_t)value;
    return (uint8_t)(n + 5);
}

/* Fills in len and the CRC; returns the packet length (len byte to CRC). */
static inline uint8_t proto_end(uint8_t *pkt, uint8_t n)
{
    pkt[0] = (uint8_t)(n - 1);
    pkt[n] = crc8(pkt, n);
    return (uint8_t)(n + 1);
}

#endif
