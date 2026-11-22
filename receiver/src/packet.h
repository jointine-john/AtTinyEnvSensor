// Parses a decoded packet (see common/protocol.h) into its readings.
// No Arduino dependencies, so it can be unit-tested on a PC.
#pragma once

#include <stdint.h>
#include "protocol.h"

struct SensorPacket {
    // Bits for `present`.
    enum : uint16_t {
        TEMPERATURE    = 1 << 0,
        HUMIDITY       = 1 << 1,
        PRESSURE       = 1 << 2,
        WIND_SPEED     = 1 << 3,
        WIND_GUST      = 1 << 4,
        WIND_DIRECTION = 1 << 5,
        RAIN           = 1 << 6,
    };

    uint8_t node;
    uint8_t seq;
    uint16_t batteryMv;     // 0 = not battery powered
    uint8_t status;         // PROTO_STATUS_* flags, 0 if none sent
    uint16_t present;       // which readings below are valid
    uint8_t unknown;        // readings of types this receiver doesn't know (skipped)

    int16_t temperature;    // centi-degC
    uint16_t humidity;      // centi-%RH
    uint16_t pressure;      // deci-hPa
    uint16_t windSpeed;     // centi-m/s
    uint16_t windGust;      // centi-m/s
    uint16_t windDirection; // deci-degrees
    uint16_t rainTips;      // running count, wraps
    uint16_t rainUmPerTip;  // micrometres per tip

    bool has(uint16_t bits) const { return (present & bits) == bits; }
};

// `pkt` is the decoder's packet (len byte, body, CRC) of `len` bytes, with a
// valid CRC. Returns false if the body is malformed (a reading runs past the
// end, or uses the reserved size code); `out` is then not meaningful.
bool parsePacket(const uint8_t *pkt, uint8_t len, SensorPacket &out);
