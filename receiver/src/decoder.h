// Manchester decoder for the sensor protocol (see common/protocol.h).
//
// Feed it the received signal as a sequence of pulses (level + duration).
// It has no Arduino dependencies so it can be unit-tested on a PC.
#pragma once

#include <stdint.h>
#include "protocol.h"

class ManchesterDecoder {
public:
    ManchesterDecoder() { reset(); }

    // Returns true when this pulse completed a packet with a valid CRC.
    // The packet (len byte, body, CRC) is then available from packet(),
    // packetLen() bytes long.
    bool feedPulse(uint8_t level, uint32_t durationUs);
    const uint8_t *packet() const { return packet_; }
    uint8_t packetLen() const { return packetLen_; }
    void reset();

private:
    // We don't know whether a given half-bit is the first or second half of a
    // bit, so two decoders run in parallel, one per alignment.
    struct Phase {
        uint16_t sync;
        uint8_t first;
        bool haveFirst;
        bool inPayload;
        uint8_t cur;
        uint8_t bits;
        uint8_t len;
        uint8_t buf[PROTO_MAX_PACKET_LEN];
    };

    bool pushHalf(uint8_t level);
    bool pushBit(Phase &p, uint8_t bit);
    static void resetPhase(Phase &p);

    Phase phases_[2];
    uint8_t parity_;
    uint8_t packet_[PROTO_MAX_PACKET_LEN];
    uint8_t packetLen_ = 0;
};
