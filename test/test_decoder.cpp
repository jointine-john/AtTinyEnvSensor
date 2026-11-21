// Host-side test: encodes random packets exactly as the sensor node does,
// distorts the pulse train like a cheap OOK receiver would (clock error,
// high/low skew, jitter, noise before and after) and checks the decoder.
//
//   make -C test
#include <algorithm>
#include <cstdio>
#include <random>
#include <vector>
#include "decoder.h"

struct P { uint8_t level; double us; };

static std::vector<P> encode(const uint8_t *pkt)
{
    std::vector<uint8_t> halves;
    auto bit = [&](uint8_t b) { halves.push_back(b); halves.push_back(!b); };
    auto byte = [&](uint8_t v) { for (int i = 7; i >= 0; i--) bit((v >> i) & 1); };
    for (int i = 0; i < PROTO_PREAMBLE_LEN; i++)
        byte(PROTO_PREAMBLE_BYTE);
    byte(PROTO_SYNC_BYTE);
    for (int i = 0; i < PROTO_PACKET_LEN; i++)
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

int main()
{
    std::mt19937 rng(1234);
    auto uni = [&](double a, double b) { return std::uniform_real_distribution<double>(a, b)(rng); };
    auto irand = [&](int a, int b) { return std::uniform_int_distribution<int>(a, b)(rng); };

    int decoded = 0, clean = 0, falseAccepts = 0, corrupted = 0;
    for (int trial = 0; trial < 20000; trial++) {
        uint8_t pkt[PROTO_PACKET_LEN];
        for (int i = 0; i < PROTO_PAYLOAD_LEN; i++)
            pkt[i] = irand(0, 255);
        pkt[PROTO_PAYLOAD_LEN] = crc8(pkt, PROTO_PAYLOAD_LEN);

        double clk = uni(0.9, 1.1);
        double skew = uni(-80, 80);
        bool corrupt = trial % 10 == 0;

        std::vector<P> sig = encode(pkt);
        if (corrupt) {
            // Flip a payload bit by stretching or shrinking one pulse.
            size_t i = sig.size() - irand(5, 60);
            sig[i].us = sig[i].us == PROTO_HALF_BIT_US ? sig[i].us * 2 : sig[i].us / 2;
        }

        std::vector<P> pulses;
        for (int i = irand(0, 40); i > 0; i--)
            pulses.push_back({(uint8_t)irand(0, 1), (double)irand(30, 3000)});
        for (auto &p : sig)
            pulses.push_back({p.level, p.us * clk + (p.level ? skew : -skew) + uni(-60, 60)});
        pulses.push_back({0, 5000});

        ManchesterDecoder dec;
        bool got = false, match = false;
        for (auto &p : pulses) {
            if (dec.feedPulse(p.level, (uint32_t)p.us)) {
                got = true;
                match = std::equal(pkt, pkt + PROTO_PACKET_LEN, dec.packet());
            }
        }

        if (corrupt) {
            corrupted++;
            falseAccepts += got;
        } else {
            clean++;
            decoded += got && match;
        }
    }

    printf("clean packets decoded:      %d / %d\n", decoded, clean);
    printf("corrupted packets accepted: %d / %d\n", falseAccepts, corrupted);
    bool pass = decoded == clean && falseAccepts == 0;
    puts(pass ? "PASS" : "FAIL");
    return pass ? 0 : 1;
}
