/*
 * ATtiny85 temperature/humidity sensor node.
 *
 * Wakes every INTERVAL_S seconds (via the watchdog), reads the SHT40 and the
 * supply voltage, transmits one packet over 433 MHz and goes back to
 * power-down sleep. Sleep current is dominated by the watchdog (~4-5 uA).
 *
 * Pinout (DIP-8):
 *   1 PB5 RESET  - ISP only
 *   2 PB3        - unused (driven low)
 *   3 PB4        - unused (driven low)
 *   4 GND
 *   5 PB0        - SHT40 SDA
 *   6 PB1        - transmitter DATA
 *   7 PB2        - SHT40 SCL
 *   8 VCC        - 2x AA direct, no regulator
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "protocol.h"
#include "i2c.h"
#include "sht40.h"
#include "radio.h"

#ifndef NODE_ID
#define NODE_ID 1
#endif

#ifndef INTERVAL_S
#define INTERVAL_S 300
#endif

/* Internal 1.1 V bandgap, nominal. Each chip is within +/-10%; measure VCC
 * with a meter once and adjust this to calibrate the battery reading. */
#ifndef BANDGAP_MV
#define BANDGAP_MV 1100
#endif

#define WDT_16MS 0
#define WDT_8S   (_BV(WDP3) | _BV(WDP0))

EMPTY_INTERRUPT(WDT_vect);

/* Power-down sleep for one watchdog period, then stop the watchdog so it
 * can't fire while we're transmitting. */
static void sleep_wdt(uint8_t prescale)
{
    cli();
    wdt_reset();
    MCUSR &= ~_BV(WDRF);
    WDTCR = _BV(WDCE) | _BV(WDE);
    WDTCR = _BV(WDIE) | prescale;

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
#if defined(BODS) && defined(BODSE)
    sleep_bod_disable();
#endif
    sei();
    sleep_cpu();
    sleep_disable();

    cli();
    WDTCR = _BV(WDCE) | _BV(WDE);
    WDTCR = 0;
    sei();
}

static uint16_t read_vcc_mv(void)
{
    power_adc_enable();
    ADMUX = _BV(MUX3) | _BV(MUX2);                 /* measure Vbg, VCC as reference */
    ADCSRA = _BV(ADEN) | _BV(ADPS1) | _BV(ADPS0);  /* 1 MHz / 8 = 125 kHz */
    _delay_ms(2);                                  /* bandgap settling */

    ADCSRA |= _BV(ADSC);                           /* first result is discarded */
    while (ADCSRA & _BV(ADSC))
        ;
    ADCSRA |= _BV(ADSC);
    while (ADCSRA & _BV(ADSC))
        ;
    uint16_t adc = ADC;

    ADCSRA = 0;
    power_adc_disable();
    return adc ? (uint16_t)(((uint32_t)BANDGAP_MV * 1024) / adc) : 0;
}

static uint16_t lfsr_next(uint16_t lfsr)
{
    return (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u);
}

int main(void)
{
    /* Run at 1 MHz from the 8 MHz RC oscillator regardless of the CKDIV8 fuse:
     * safe down to 1.8 V and low active current. */
    clock_prescale_set(clock_div_8);

    ACSR |= _BV(ACD);       /* analog comparator off */
    ADCSRA = 0;
    power_all_disable();    /* timers, USI, ADC */

    /* Unused pins as outputs low so they never float. */
    DDRB = _BV(PB3) | _BV(PB4);
    PORTB = 0;

    radio_init();
    i2c_init();
    sht40_reset();

    uint8_t seq = 0;
    uint16_t lfsr = 0xACE1u ^ ((uint16_t)NODE_ID << 8);
    uint8_t packet[PROTO_PACKET_LEN];

    for (;;) {
        int16_t temp = PROTO_TEMP_INVALID;
        uint16_t rh = 0;

        if (sht40_start()) {
            sleep_wdt(WDT_16MS);    /* conversion takes up to 8.3 ms */
            if (!sht40_read(&temp, &rh)) {
                temp = PROTO_TEMP_INVALID;
                rh = 0;
            }
        }

        uint16_t vcc = read_vcc_mv();

        packet[0] = NODE_ID;
        packet[1] = seq++;
        packet[2] = (uint16_t)temp >> 8;
        packet[3] = (uint16_t)temp & 0xFF;
        packet[4] = rh >> 8;
        packet[5] = rh & 0xFF;
        packet[6] = vcc >> 8;
        packet[7] = vcc & 0xFF;
        packet[8] = crc8(packet, PROTO_PAYLOAD_LEN);
        radio_send(packet);

        /* Randomly add one extra 8 s period so nodes that started together
         * drift apart instead of colliding every time. */
        lfsr = lfsr_next(lfsr);
        uint16_t wakes = INTERVAL_S / 8 + (lfsr & 1);
        while (wakes--)
            sleep_wdt(WDT_8S);
    }
}
