# ATtiny85 Temperature / Humidity Sensors

Battery-powered sensor nodes (ATtiny85 + SHT40 or HTU21D + 433 MHz transmitter on 2×AA)
reporting every ~5 minutes to an ESP32 receiver that publishes to MQTT with
Home Assistant auto-discovery. Estimated battery use is ~140 mAh a year, so a
pair of AAs lasts several years.

```
 [ATtiny85 + sensor + STX882] ))) 433.92 MHz OOK ))) [RXB6/SRX882 + ESP32] --WiFi--> MQTT --> Home Assistant
```

## Documentation

| | Page | Contents |
|---|---|---|
| 1 | [Parts](docs/parts.md) | Bill of materials and UK shopping list |
| 2 | [Wiring](docs/wiring.md) | Sensor node and receiver wiring, antennas |
| 3 | [Programming the ATtiny85](docs/programming.md) | USBasp, ArduinoISP and SparkFun programmer wiring, troubleshooting |
| 4 | [Building and flashing](docs/building.md) | Build options, node IDs, battery calibration, receiver setup, Home Assistant, decoder test |
| 5 | [Power budget](docs/power.md) | Battery life estimate, tuning, measuring sleep current |
| 6 | [Radio protocol](docs/protocol.md) | Frame format, payload, reliability |

## Quick start

```sh
# Sensor node (ATtiny85 on a USBasp; add "-e sht40" for an SHT40 board)
cd sensor-node
pio run -t fuses
PLATFORMIO_BUILD_FLAGS="-DNODE_ID=1" pio run -t upload

# Receiver (ESP32)
cd ../receiver
cp src/secrets.example.h src/secrets.h   # fill in WiFi + MQTT
pio run -t upload && pio device monitor
```

## Repository layout

| Path | What |
|---|---|
| `common/protocol.h` | Over-the-air packet format, shared by both ends |
| `sensor-node/` | ATtiny85 firmware (bare avr-gcc via PlatformIO, no Arduino core) |
| `receiver/` | ESP32 firmware (Arduino via PlatformIO) |
| `test/` | PC test of the Manchester decoder against noisy, jittered signals |
| `docs/` | Documentation |
