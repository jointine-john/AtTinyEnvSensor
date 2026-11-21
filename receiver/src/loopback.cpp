#ifdef LOOPBACK_TEST

#include <Arduino.h>
#include "esp_timer.h"
#include "driver/gpio.h"

#include "loopback.h"
#include "protocol.h"

constexpr gpio_num_t TX_PIN = GPIO_NUM_5;
constexpr uint8_t NODE_ID = 250;
constexpr uint32_t INTERVAL_MS = 30000;

constexpr uint16_t FRAME_HALVES = ((PROTO_PREAMBLE_LEN + 1 + PROTO_PACKET_LEN) * 8 + 1) * 2;
constexpr uint16_t GAP_HALVES = PROTO_REPEAT_GAP_MS * 1000 / PROTO_HALF_BIT_US;
constexpr uint16_t MAX_HALVES = PROTO_REPEATS * (FRAME_HALVES + GAP_HALVES);

// The whole burst is pre-encoded as half-bit levels and clocked out by a
// periodic esp_timer, which runs on the other core from loop() and the
// receive interrupt.
static uint8_t halves[MAX_HALVES];
static uint16_t halfCount = 0;
static volatile uint16_t halfIdx = 0;
static esp_timer_handle_t timer;
static uint8_t seq = 0;
static uint32_t lastSendMs;

static void onTick(void *)
{
    uint16_t i = halfIdx;
    if (i < halfCount) {
        gpio_set_level(TX_PIN, halves[i]);
        halfIdx = i + 1;
        return;
    }
    gpio_set_level(TX_PIN, 0);
    esp_timer_stop(timer);
}

static void pushBit(uint16_t &n, uint8_t bit)
{
    halves[n++] = bit;
    halves[n++] = !bit;
}

static void pushByte(uint16_t &n, uint8_t byte)
{
    for (uint8_t m = 0x80; m; m >>= 1)
        pushBit(n, (byte & m) ? 1 : 0);
}

static void sendPacket()
{
    // Sawtooth 20.00 -> 21.90 C so successive readings are visibly different.
    int16_t temp = 2000 + (seq % 20) * 10;
    uint16_t rh = 5000;
    uint16_t vbat = 3000;

    uint8_t pkt[PROTO_PACKET_LEN];
    pkt[0] = NODE_ID;
    pkt[1] = seq;
    pkt[2] = (uint16_t)temp >> 8;
    pkt[3] = (uint16_t)temp & 0xFF;
    pkt[4] = rh >> 8;
    pkt[5] = rh & 0xFF;
    pkt[6] = vbat >> 8;
    pkt[7] = vbat & 0xFF;
    pkt[8] = crc8(pkt, PROTO_PAYLOAD_LEN);

    uint16_t n = 0;
    for (uint8_t r = 0; r < PROTO_REPEATS; r++) {
        for (uint8_t i = 0; i < PROTO_PREAMBLE_LEN; i++)
            pushByte(n, PROTO_PREAMBLE_BYTE);
        pushByte(n, PROTO_SYNC_BYTE);
        for (uint8_t i = 0; i < PROTO_PACKET_LEN; i++)
            pushByte(n, pkt[i]);
        pushBit(n, 0);
        for (uint16_t i = 0; i < GAP_HALVES; i++)
            halves[n++] = 0;
    }

    halfIdx = 0;
    halfCount = n;
    esp_timer_start_periodic(timer, PROTO_HALF_BIT_US);
    Serial.printf("loopback: sent node %u seq %u: %.2f C, %.2f %%RH, %u mV\n",
                  NODE_ID, seq, temp / 100.0, rh / 100.0, vbat);
    seq++;
}

void loopbackBegin()
{
    gpio_reset_pin(TX_PIN);
    gpio_set_direction(TX_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(TX_PIN, 0);

    esp_timer_create_args_t args = {};
    args.callback = onTick;
    args.name = "loopback";
    esp_timer_create(&args, &timer);

    lastSendMs = millis() - INTERVAL_MS + 5000;     // first packet after ~5 s
    Serial.println("LOOPBACK TEST MODE: jumper GPIO5 -> GPIO4");
}

void loopbackLoop()
{
    if (halfIdx < halfCount)
        return;     // still transmitting
    if (millis() - lastSendMs < INTERVAL_MS)
        return;
    lastSendMs = millis();
    sendPacket();
}

#endif
