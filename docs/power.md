# Power budget

| Item | Current | Per year |
|---|---|---|
| Sleep: ATtiny85 power-down + watchdog | ~4.5 µA | ~40 mAh |
| SHT40 idle | 0.08 µA | <1 mAh |
| MCU awake for measure + TX (~0.45 s at ~0.4 mA, every 5 min) | | ~20 mAh |
| Transmitter: 3 × 120 ms frames, ~50% duty at ~15 mA, every 5 min | | ~80 mAh |
| **Total** | | **~140 mAh/year** |

These are estimates from datasheet figures, not measurements.

With a **GY-21 (HTU21D)** board instead of an SHT40, add about 9 mAh a year
for the board's 3.3 V regulator (~1 µA). Its longer conversions (~70 ms, slept
through) add under 1 mAh. The total is still roughly 150 mAh a year.

2×AA alkaline give ~2000 mAh usable down to 2.0 V. The battery outlasts any
target you're likely to set; shelf life is the real limit. If you wanted a
CR2032 (~220 mAh) instead, the same firmware would run for about 1.5 years at best
(add a 100 µF low-leakage capacitor for the TX bursts).

## Tuning

To trade range for battery, reduce `PROTO_REPEATS` (in `common/protocol.h`) or
raise `INTERVAL_S` (in `sensor-node/platformio.ini`).

## Measuring sleep current

Put a multimeter in µA mode in series with the battery. Bridge the meter during
the transmit burst, or the meter's burden voltage may brown the circuit out.

Expect about 5 µA. Anything over ~10 µA usually means a breakout board LED or
regulator, or a floating pin.
