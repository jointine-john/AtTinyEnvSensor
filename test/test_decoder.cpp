// Host-side test: encodes random packets exactly as the sensor node does,
// distorts the pulse train like a cheap OOK receiver would (clock error,
// high/low skew, jitter, noise before and after) and checks the decoder.
// Then checks the packet parser against the readings that were sent.
//
//   make -C test
#include <algorithm>
#include <cstdio>
#include <random>
#include <vector>
#include "decoder.h"
#include "packet.h"

struct P { uint8_t level; double us; };

static std::vector<P> encode(const uint8_t *pkt, int len)
{
    std::vector<uint8_t> halves;
    auto bit = [&](uint8_t b) { halves.push_back(b); halves.push_back(!b); };
    auto byte = [&](uint8_t v) { for (int i = 7; i >= 0; i--) bit((v >> i) & 1); };
    for (int i = 0; i < PROTO_PREAMBLE_LEN; i++)
        byte(PROTO_PREAMBLE_BYTE);
    byte(PROTO_SYNC_BYTE);
    for (int i = 0; i < len; i++)
        byte(pkt[i]);
    bit(0);
    halves.push_back(0);

    std::vector<P> pulses;
    for (uint8_t h : halves) {
        if (!pulses.empty() && pulses.back().level == h)
            pulses.back().us += PROTO_HALF_BIT_US;
        else
            pulses.push_back({h, PROTO_HALF_BIT_US});
    }
    return pulses;
}

// A random packet with a random selection of readings (sometimes including
// an unknown type, which the parser must skip). `expect` gets what the parser
// should report.
template <class Rng>
static int randomPacket(Rng &irand, uint8_t *pkt, SensorPacket &expect)
{
    expect = SensorPacket{};
    expect.node = irand(0, 255);
    expect.seq = irand(0, 255);
    expect.batteryMv = irand(0, 3600);
    int n = proto_begin(pkt, expect.node, expect.seq, expect.batteryMv);

    auto maybe16 = [&](uint8_t type, uint16_t bit, uint16_t &field) {
        if (irand(0, 1)) {
            field = irand(0, 65535);
            n = proto_put16(pkt, n, type, field);
            expect.present |= bit;
        }
    };
    if (irand(0, 3) == 0) {
        expect.status = PROTO_STATUS_SENSOR_ERROR;
        n = proto_put8(pkt, n, PROTO_STATUS, expect.status);
    }
    uint16_t temp = 0;
    maybe16(PROTO_TEMPERATURE, SensorPacket::TEMPERATURE, temp);
    expect.temperature = (int16_t)temp;
    maybe16(PROTO_HUMIDITY, SensorPacket::HUMIDITY, expect.humidity);
    maybe16(PROTO_PRESSURE, SensorPacket::PRESSURE, expect.pressure);
    maybe16(PROTO_WIND_SPEED, SensorPacket::WIND_SPEED, expect.windSpeed);
    maybe16(PROTO_WIND_GUST, SensorPacket::WIND_GUST, expect.windGust);
    maybe16(PROTO_WIND_DIRECTION, SensorPacket::WIND_DIRECTION, expect.windDirection);
    if (irand(0, 1)) {
        expect.rainTips = irand(0, 65535);
        expect.rainUmPerTip = 279;
        n = proto_put32(pkt, n, PROTO_RAIN, ((uint32_t)expect.rainTips << 16) | expect.rainUmPerTip);
        expect.present |= SensorPacket::RAIN;
    }
    if (irand(0, 4) == 0) {
        n = proto_put32(pkt, n, PROTO_SIZE_4 | 0x3E, 0xDEADBEEF);   // a future type
        expect.unknown++;
    }
    return proto_end(pkt, n);
}

static bool samePacket(const SensorPacket &a, const SensorPacket &b)
{
    return a.node == b.node && a.seq == b.seq && a.batteryMv == b.batteryMv &&
           a.status == b.status && a.present == b.present && a.unknown == b.unknown &&
           (!a.has(SensorPacket::TEMPERATURE) || a.temperature == b.temperature) &&
           (!a.has(SensorPacket::HUMIDITY) || a.humidity == b.humidity) &&
           (!a.has(SensorPacket::PRESSURE) || a.pressure == b.pressure) &&
           (!a.has(SensorPacket::WIND_SPEED) || a.windSpeed == b.windSpeed) &&
           (!a.has(SensorPacket::WIND_GUST) || a.windGust == b.windGust) &&
           (!a.has(SensorPacket::WIND_DIRECTION) || a.windDirection == b.windDirection) &&
           (!a.has(SensorPacket::RAIN) || (a.rainTips == b.rainTips && a.rainUmPerTip == b.rainUmPerTip));
}

// Packets with a valid CRC but a broken body must be rejected, not misread.
static bool malformedRejected()
{
    uint8_t pkt[PROTO_MAX_PACKET_LEN];
    SensorPacket sp;

    // Reading truncated: a 2-byte value with only 1 byte left in the body.
    int n = proto_begin(pkt, 1, 2, 3000);
    pkt[n++] = PROTO_TEMPERATURE;
    pkt[n++] = 0x12;
    int len = proto_end(pkt, n);
    if (parsePacket(pkt, len, sp))
        return false;

    // Reserved size code.
    n = proto_begin(pkt, 1, 2, 3000);
    n = proto_put8(pkt, n, 0xC1, 0);
    len = proto_end(pkt, n);
    if (parsePacket(pkt, len, sp))
        return false;

    // Header only is fine.
    n = proto_begin(pkt, 1, 2, 3000);
    len = proto_end(pkt, n);
    return parsePacket(pkt, len, sp) && sp.present == 0 && sp.batteryMv == 3000;
}

int main()
{
    std::mt19937 rng(1234);
    auto uni = [&](double a, double b) { return std::uniform_real_distribution<double>(a, b)(rng); };
    auto irand = [&](int a, int b) { return std::uniform_int_distribution<int>(a, b)(rng); };

    int decoded = 0, clean = 0, falseAccepts = 0, corrupted = 0, parsed = 0;
    for (int trial = 0; trial < 20000; trial++) {
        uint8_t pkt[PROTO_MAX_PACKET_LEN];
        SensorPacket expect;
        int len = randomPacket(irand, pkt, expect);

        double clk = uni(0.9, 1.1);
        double skew = uni(-80, 80);
        bool corrupt = trial % 10 == 0;

        std::vector<P> sig = encode(pkt, len);
        if (corrupt) {
            // Flip a bit after the sync word by stretching or shrinking one pulse.
            size_t i = sig.size() - irand(5, (int)sig.size() - 45);
            sig[i].us = sig[i].us == PROTO_HALF_BIT_US ? sig[i].us * 2 : sig[i].us / 2;
        }

        std::vector<P> pulses;
        for (int i = irand(0, 40); i > 0; i--)
            pulses.push_back({(uint8_t)irand(0, 1), (double)irand(30, 3000)});
        for (auto &p : sig)
            pulses.push_back({p.level, p.us * clk + (p.level ? skew : -skew) + uni(-60, 60)});
        pulses.push_back({0, 5000});

        ManchesterDecoder dec;
        bool got = false, match = false, parseOk = false;
        for (auto &p : pulses) {
            if (dec.feedPulse(p.level, (uint32_t)p.us)) {
                got = true;
                match = dec.packetLen() == len && std::equal(pkt, pkt + len, dec.packet());
                SensorPacket sp;
                parseOk = parsePacket(dec.packet(), dec.packetLen(), sp) && samePacket(expect, sp);
            }
        }

        if (corrupt) {
            corrupted++;
            falseAccepts += got;
        } else {
            clean++;
            decoded += got && match;
            parsed += got && match && parseOk;
        }
    }

    bool malformedOk = malformedRejected();
    printf("clean packets decoded:      %d / %d\n", decoded, clean);
    printf("decoded packets parsed:     %d / %d\n", parsed, clean);
    printf("corrupted packets accepted: %d / %d\n", falseAccepts, corrupted);
    printf("malformed bodies rejected:  %s\n", malformedOk ? "yes" : "NO");
    bool pass = decoded == clean && parsed == clean && falseAccepts == 0 && malformedOk;
    puts(pass ? "PASS" : "FAIL");
    return pass ? 0 : 1;
}
