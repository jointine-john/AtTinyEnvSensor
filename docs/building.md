# Building and flashing

Install [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html).

## Sensor node

The ATtiny85 is programmed over ISP with a USBasp or an Arduino running
ArduinoISP. See [Programming the ATtiny85](programming.md) for wiring.

Choose the PlatformIO environment that matches your sensor board:

| Environment | Sensor | Example boards |
|---|---|---|
| `htu21d` (default) | HTU21D, SHT21 or Si7021 | GY-21, GY-213V |
| `sht40` | Sensirion SHT40 | Adafruit 4885, DFRobot Fermion |

```sh
cd sensor-node
pio run -t fuses                                   # once per new chip
PLATFORMIO_BUILD_FLAGS="-DNODE_ID=1" pio run -t upload            # HTU21D/GY-21
PLATFORMIO_BUILD_FLAGS="-DNODE_ID=2" pio run -e sht40 -t upload   # SHT40
```

Give each sensor a unique `NODE_ID` (1–255). If the build doesn't match the
board, the receiver logs `SENSOR ERROR` for that node but still reports its
battery voltage.

### Build options

| Flag | Default | Meaning |
|---|---|---|
| `NODE_ID` | 1 | Sensor ID reported to the receiver |
| `INTERVAL_S` | 300 | Seconds between readings (rounded to 8 s steps) |
| `BANDGAP_MV` | 1100 | ATtiny85 internal reference, for battery calibration |

### Calibrating the battery reading

The ATtiny's internal 1.1 V reference is only ±10%. Measure VCC with a meter,
compare with the value the receiver reports, and rebuild with
`-DBANDGAP_MV=<1100 × meter / reported>`.

## Receiver

```sh
cd receiver
cp src/secrets.example.h src/secrets.h   # fill in WiFi + MQTT
pio run -t upload && pio device monitor
```

`secrets.h` is git-ignored.

### Home Assistant

Each sensor appears in Home Assistant through MQTT discovery as a device
"ATtiny sensor N" with Temperature, Humidity and Battery (V) entities. They
become unavailable after 20 minutes without data. Raw JSON is published on
`attiny_th/<id>/state`:

```json
{"temperature":21.53,"humidity":45.20,"battery":2.981,"seq":12}
```

The serial monitor logs every decoded packet, which is the quickest way to
check the radio link.

## Decoder test (PC)

Needs `g++` and `make`.

```sh
make -C test
```

It feeds the receiver's decoder 20,000 simulated packets with clock error,
pulse distortion, jitter and noise, and checks that clean packets decode and
corrupted ones are rejected.
