# ATtiny85 Temperature / Humidity Sensors

Battery-powered sensor nodes (ATtiny85 + SHT40 + 433 MHz transmitter on 2×AA)
reporting every ~5 minutes to an ESP32 receiver that publishes to MQTT with
Home Assistant auto-discovery.

```
 [ATtiny85 + SHT40 + STX882] ))) 433.92 MHz OOK ))) [RXB6 + ESP32] --WiFi--> MQTT --> Home Assistant
```

## Layout

| Path | What |
|---|---|
| `common/protocol.h` | Over-the-air packet format, shared by both ends |
| `sensor-node/` | ATtiny85 firmware (bare avr-gcc via PlatformIO, no Arduino core) |
| `receiver/` | ESP32 firmware (Arduino via PlatformIO) |
| `test/` | PC test of the Manchester decoder against noisy, jittered signals |

## Bill of materials (per sensor)

| Part | Notes |
|---|---|
| ATtiny85-20PU (DIP-8) or ATtiny85V | Runs at 1 MHz, fine down to 1.8 V |
| SHT40 breakout | Must have SDA/SCL pull-ups (~10 kΩ); Adafruit/generic boards do. **Remove or cut any power LED and regulator** |
| STX882 433 MHz transmitter | Rated 1.2–6 V, works well at 3 V. FS1000A also works but loses range below ~3 V |
| 17.3 cm wire | Quarter-wave antenna |
| 100 nF ceramic | Decoupling, across ATtiny VCC/GND |
| 10 µF ceramic | Across the battery, helps during TX bursts |
| 2×AA holder | Lithium AA (Energizer L91) recommended outdoors/cold |

Receiver: any ESP32 dev board + RXB6 (superheterodyne, much better than the
XY-MK-5V) + 17.3 cm antenna.

## Wiring (sensor)

```
                 ATtiny85
              +-----\/-----+
   (ISP RST) -| 1 PB5  VCC 8|- Battery +  --- 100nF --- GND
     unused  -| 2 PB3  PB2 7|- SHT40 SCL
     unused  -| 3 PB4  PB1 6|- STX882 DATA
        GND  -| 4 GND  PB0 5|- SHT40 SDA
              +-------------+

 SHT40 VIN -> Battery +      STX882 VCC -> Battery +
 SHT40 GND -> GND            STX882 GND -> GND, ANT -> 17.3 cm wire
```

No voltage regulator: everything runs straight off the cells (3.2 V fresh → ~2.0 V flat).
The transmitter draws nothing measurable while its DATA pin is low.

PB0/PB1/PB2 double as the ISP MOSI/MISO/SCK pins, so you can reprogram in-circuit
(the transmitter will chirp during upload, which is harmless).

## Power budget

| Item | Current | Per year |
|---|---|---|
| Sleep: ATtiny85 power-down + watchdog | ~4.5 µA | ~40 mAh |
| SHT40 idle | 0.08 µA | <1 mAh |
| MCU awake for measure + TX (~0.45 s at ~0.4 mA, every 5 min) | | ~20 mAh |
| Transmitter: 3 × 120 ms frames, ~50% duty at ~15 mA, every 5 min | | ~80 mAh |
| **Total** | | **~140 mAh/year** |

2×AA alkaline give ~2000 mAh usable down to 2.0 V. The battery outlasts any
target you're likely to set; shelf life is the real limit. If you wanted a
CR2032 (~220 mAh) instead, the same firmware would run for about 1.5 years at best
(add a 100 µF low-leakage capacitor for the TX bursts).

To trade range for battery, reduce `PROTO_REPEATS` or raise `INTERVAL_S`.

**Check your build's sleep current** with a multimeter in µA mode in series with the
battery (bridge the meter during the transmit burst, or it may brown out). Anything over
~10 µA usually means a breakout board LED/regulator or a floating pin.

## Building

Install [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html).

### Sensor node

Programmer: USBasp, or an Arduino Uno running the ArduinoISP sketch (see
`sensor-node/platformio.ini`).

```sh
cd sensor-node
pio run -t fuses                                   # once per new chip
PLATFORMIO_BUILD_FLAGS="-DNODE_ID=1" pio run -t upload
PLATFORMIO_BUILD_FLAGS="-DNODE_ID=2" pio run -t upload   # next sensor...
```

Give each sensor a unique `NODE_ID` (1–255).

**Calibrating the battery reading:** the ATtiny's internal 1.1 V reference is only
±10%. Measure VCC with a meter, compare with the value the receiver reports, and
rebuild with `-DBANDGAP_MV=<1100 × meter / reported>`.

### Receiver

```sh
cd receiver
cp src/secrets.example.h src/secrets.h   # fill in WiFi + MQTT
pio run -t upload && pio device monitor
```

Each sensor appears in Home Assistant as a device "ATtiny sensor N" with
Temperature, Humidity and Battery (V) entities, which become unavailable after 20
minutes without data. Raw JSON is on `attiny_th/<id>/state`.

### Decoder test (PC)

```sh
make -C test
```

## Protocol

See `common/protocol.h`. 1000 bit/s Manchester, 4-byte preamble, sync byte
`0x2D`, 8-byte payload (node, seq, temp, RH, battery mV), CRC-8. Each reading
is sent 3 times, 20 ms apart. The receiver de-duplicates on (node, seq). Nodes
add random jitter to their interval so they don't keep colliding.

433.92 MHz at well under 1% duty cycle is within UK/EU SRD (IR2030) limits.
