# Sensors

The firmware supports two families of I2C temperature/humidity sensor. Both
wire up identically, so the choice only affects which firmware you build.

| | SHT40 | HTU21D / SHT21 / Si7021 |
|---|---|---|
| Example boards | Adafruit 4885, DFRobot Fermion | GY-21, GY-213V |
| PlatformIO environment | `sht40` | `htu21d` (default) |
| I2C address | 0x44 | 0x40 |
| Accuracy | ±0.2 °C, ±1.8 %RH | ±0.3 °C, ±2 %RH |
| Supply range | 1.08–3.6 V | 1.5–3.6 V |
| Time awake per reading | ~16 ms | ~70 ms (slept through) |
| Extra battery use (board) | none if LED removed | ~9 mAh/year (regulator) |
| Damp/outdoor use | Good | Can stick near 100 %RH after condensation |
| Status | Recommended | Proof of concept |

## Which to use

- **SHT40** for the finished sensors, especially outdoors or anywhere damp.
- **GY-21 (HTU21D)** is what the proof of concept uses. It's fine indoors and
  works without modification.

## Wiring

Both boards use the pins **VIN, GND, SCL, SDA**. On the stripboard:

| Sensor pin | Stripboard hole | ATtiny85 |
|---|---|---|
| VIN | C15 | VCC (pin 8) |
| SCL | D15 | PB2 (pin 7) |
| SDA | F15 | PB0 (pin 5) |
| GND | G15 | GND (pin 4) |

See [Wiring](wiring.md) for the full schematic and stripboard layout.

## Building for your sensor

```sh
cd sensor-node
pio run -t upload               # HTU21D / SHT21 / Si7021 (GY-21)
pio run -e sht40 -t upload      # SHT40
```

The default is set by `default_envs` in `sensor-node/platformio.ini`. To make
the SHT40 the default, change it to `sht40`.

## Board notes

### GY-21 (HTU21D / SHT21)

The GY-21 was designed for 5 V Arduinos, so the back of the board carries:

- a 3.3 V regulator (marked 662K), which draws about 1 µA
- two level-shifting transistors on SDA and SCL, which draw nothing when idle
- the I2C pull-up resistors, so no external ones are needed

There is **no power LED**, so it can be used as-is on 2×AA. From a 3 V battery
the regulator passes slightly less than the battery voltage through, and the
sensor works down to 1.5 V, so it keeps working as the cells drain.

To save the last ~1 µA, remove the regulator and bridge its input and output
pads. It isn't worth it for the proof of concept.

The front of the board has two footprints. The populated one decides the chip
(HTU21D or SHT21), and the empty one is for an Si7021. All three use the same
commands and the same `htu21d` firmware.

### SHT40 breakouts

- **Adafruit 4885** has a power LED. Cut its jumper or remove the LED, or it
  will drain the batteries in weeks.
- Bare SHT40 boards with no regulator must not see more than 3.6 V. Set a
  USBasp to 3.3 V when programming in-circuit (see
  [Programming](programming.md)).

## Troubleshooting

If the firmware doesn't match the board, or SDA/SCL are swapped, the sensor
doesn't answer. The node still transmits, and the receiver logs:

```
node 1 seq 4: SENSOR ERROR, battery 3012 mV
```

A sensible battery voltage there means the ATtiny85 and radio are working, and
the problem is on the sensor side:

1. Check the build environment matches the board.
2. Check SDA and SCL aren't swapped.
3. Check the board has power between VIN and GND.

## Adding another sensor

Drivers implement the two functions in `sensor-node/src/sensor.h`:

```c
void sensor_init(void);
uint8_t sensor_read(int16_t *temp_c100, uint16_t *rh_c100);
```

Use `sleep_16ms()` to wait for conversions in power-down sleep. Add a new
environment to `sensor-node/platformio.ini` whose `build_src_filter` excludes
the other drivers.
