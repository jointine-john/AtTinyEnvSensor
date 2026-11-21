#include "decoder.h"
#include <string.h>

void ManchesterDecoder::reset()
{
    resetPhase(phases_[0]);
    resetPhase(phases_[1]);
    parity_ = 0;
}

void ManchesterDecoder::resetPhase(Phase &p)
{
    p.sync = 0;
    p.haveFirst = false;
    p.inPayload = false;
}

bool ManchesterDecoder::feedPulse(uint8_t level, uint32_t durationUs)
{
    const uint32_t T = PROTO_HALF_BIT_US;

    // Anything that isn't one or two half-bits long is noise or a gap.
    if (durationUs < T / 2 || durationUs > T * 5 / 2) {
        reset();
        return false;
    }
    uint8_t halves = durationUs < T * 3 / 2 ? 1 : 2;

    bool done = false;
    while (halves--)
        done |= pushHalf(level ? 1 : 0);
    return done;
}

bool ManchesterDecoder::pushHalf(uint8_t level)
{
    bool done = false;
    for (uint8_t k = 0; k < 2; k++) {
        Phase &p = phases_[k];
        if (((parity_ + k) & 1) == 0) {
            p.first = level;
            p.haveFirst = true;
        } else if (p.haveFirst) {
            p.haveFirst = false;
            if (p.first == level)
                resetPhase(p);      // no mid-bit transition: wrong alignment or noise
            else
                done |= pushBit(p, p.first);
        }
    }
    parity_ ^= 1;
    return done;
}

bool ManchesterDecoder::pushBit(Phase &p, uint8_t bit)
{
    if (!p.inPayload) {
        p.sync = (uint16_t)((p.sync << 1) | bit);
        if (p.sync == PROTO_SYNC_WORD) {
            p.inPayload = true;
            p.cur = 0;
            p.bits = 0;
            p.len = 0;
        }
        return false;
    }

    p.cur = (uint8_t)((p.cur << 1) | bit);
    if (++p.bits < 8)
        return false;
    p.buf[p.len++] = p.cur;
    p.bits = 0;
    p.cur = 0;
    if (p.len < PROTO_PACKET_LEN)
        return false;

    bool ok = crc8(p.buf, PROTO_PAYLOAD_LEN) == p.buf[PROTO_PAYLOAD_LEN];
    if (ok)
        memcpy(packet_, p.buf, PROTO_PACKET_LEN);
    resetPhase(p);
    return ok;
}
