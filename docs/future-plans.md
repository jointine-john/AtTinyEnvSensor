# Future plans

Nothing on this page is built yet. It records where the sensor node is headed
after the stripboard proof of concept: a custom PCB, an enclosure, and coin-cell
power. It also covers a sensor on the ESP32 receiver itself.

Wind and rain recording (on the ATtiny84) moved to the separate
AtTinyWeatherStation project.

## Goals

1. Run the sensor node from a single **CR2450** coin cell instead of 2×AA.
2. Replace the stripboard with a **PCB** designed in EasyEDA and ordered from
   JLCPCB (optionally with assembly).
3. Design a **case** that fits the PCB and cell, and suits indoor and sheltered
   outdoor use.
4. Have the **ESP32 receiver** record its own temperature, humidity and
   **pressure**, published to Home Assistant alongside the nodes.

The cell is a **CR2450** (24.5 mm × 5.0 mm, ~620 mAh, 3 V lithium).

## 1. Coin-cell power

### Budget

From [Power budget](power.md), the node uses about 140 mAh a year with an
SHT40. On a CR2450:

| Cell | Capacity | Estimated life |
|---|---|---|
| CR2450 | ~620 mAh | ~4 years on paper, less in the cold |
| CR2032 (for comparison) | ~220 mAh | ~1.5 years |

The ~92 mAh a year spent on the transmitter is the biggest item, so it is the
first thing to cut if life falls short.

### The pulse-current problem

The STX882 draws about 15 mA for around 360 ms per report (3 frames). A coin
cell's internal resistance (tens of ohms, rising with age and cold) makes the
voltage sag under that load, which can reset the ATtiny or corrupt a frame.

Mitigations to evaluate:

- **Reservoir capacitor** across the cell, e.g. 220–470 µF. It must be
  low-leakage (polymer or low-leakage tantalum/electrolytic); a leaky capacitor
  can draw more than the whole sleeping circuit.
- **Fewer repeats:** lower `PROTO_REPEATS` in `common/protocol.h` from 3 to 2
  (cuts TX energy by a third, at some cost to reliability).
- **Longer interval:** raise `INTERVAL_S` in `sensor-node/platformio.ini`.
- **Gap between frames:** sleep briefly between repeats so the capacitor can
  recharge.
- **Brown-out detection:** confirm the BOD fuse setting is sensible for a
  ~2.0 V end point.

### Bench test before the PCB

1. Power the current stripboard build from a CR2450 in a holder.
2. Scope or log the supply rail during a transmit burst, with and without the
   reservoir capacitor.
3. Check the receiver still decodes every report, including with a part-used
   cell and in a fridge (cold test).
4. Measure sleep current (expect about 5 µA, see [Power budget](power.md)).

Exit criterion: no resets or lost reports with a part-used cell and the
chosen capacitor.

## 2. PCB (EasyEDA → JLCPCB)

### Design choices

- **Sensor on the board:** a bare SHT40 (DFN-4) instead of a breakout. This
  drops the breakout's LED and regulator, and the HTU21D/GY-21 build stays
  stripboard-only.
- **MCU:** ATtiny85 in SOIC-8 (ATtiny85V preferred, rated to 1.8 V), or keep a
  DIP socket for easy reflashing.
- **Transmitter:** keep the STX882 module, soldered flat or on headers, rather
  than designing an RF stage.
- **Antenna:** 17.3 cm wire from a pad, or a PCB/helical antenna if the case
  is too small (expect some range loss).
- **Programming:** a 6-pin ISP footprint (or pogo-pin pads) so boards can be
  reflashed in place.
- **Cell holder:** SMD CR2450 holder on the back of the board.
- **Passives:** 100 nF decoupling, the reservoir capacitor from the bench test,
  and 10 kΩ I²C pull-ups (the breakouts provided these before).
- **Thermal isolation:** keep the SHT40 at the board edge, away from the
  transmitter, with slots in the board to reduce heat coupling.
- **Parts:** pick from the JLCPCB/LCSC basic parts list where possible to keep
  assembly cheap.

### Schematic (draft)

![Sensor node PCB schematic](images/pcb-schematic.svg)

The pin-out is the same as the stripboard build, so the `sht40` firmware runs
unchanged. Draft parts list:

| Ref | Part | Package (suggested) |
|---|---|---|
| BT1 | CR2450 holder | SMD, from the LCSC catalogue |
| C1, C4 | 100 nF ceramic | 0603 |
| C2 | 10 µF ceramic | 0805 |
| C3 | 220–470 µF low-leakage, value from the bench test | Polymer or electrolytic, SMD |
| R1, R2, R3 | 10 kΩ | 0603 |
| U1 | ATtiny85V-10SU | SOIC-8 |
| U2 | SHT40-AD1B | DFN-4 (1.5 × 1.5 mm) |
| U3 | STX882 module | 4-pin header or castellated pads |
| J1 | ISP header | 2×3, 2.54 mm, or pogo-pin pads |

### Steps

1. Enter this schematic in EasyEDA.
2. Lay out the board, sized to fit the cell holder.
3. Order a small batch of bare boards first, hand-assemble one and repeat the
   bench test.
4. Add the schematic and Gerbers to the repo (e.g. `hardware/`).

## 3. Case

- Sized around the PCB, cell holder and antenna.
- Vents or a membrane so the sensor sees outside air, with the electronics
  shielded from direct rain and sun (a Stevenson-screen style louvred design
  for outdoor nodes).
- Easy access to change the cell.
- 3D-printed (PETG or ASA for outdoor UV resistance).
- Store the model files in the repo alongside the PCB files.

## 4. Receiver sensor (temperature, humidity, pressure)

The receiver is mains-powered, so power use doesn't matter here.

### Sensor choice

- **BME280** (recommended): temperature, humidity and pressure in one I²C
  chip, on cheap breakout boards. Buy from a reputable seller: many boards
  sold as "BME280" carry a BMP280, which has no humidity sensor. The firmware
  should check the chip ID (0x60 for BME280, 0x58 for BMP280) and log a clear
  error.
- **Alternative:** SHT40 for temperature and humidity plus a BMP280/BMP390 for
  pressure, on the same I²C bus. More accurate humidity (±1.8% vs ±3% RH), and
  it matches the nodes' sensor, at the cost of a second board.

### Wiring

- I²C on the ESP32 defaults: **SDA → GPIO21, SCL → GPIO22**, VCC → 3V3,
  GND → GND. GPIO4 stays the 433 MHz receiver input.
- Most BME280 boards already have I²C pull-ups.
- **Self-heating:** the ESP32 and its WiFi radio warm the board by a few °C.
  Mount the sensor on a short lead outside or below the receiver's case, away
  from the ESP32 and any regulator.

### Firmware

- Read the sensor every 60 s, in forced mode so it sleeps between readings
  (less self-heating).
- Publish over the existing MQTT connection as its own Home Assistant device
  (e.g. "Receiver"), with Home Assistant auto-discovery like the nodes:
  temperature (°C), humidity (%) and pressure (hPa, device class
  `atmospheric_pressure`).
- Optionally report sea-level pressure as well, using a configured altitude
  (in `secrets.h`), so readings match weather forecasts.
- Use an existing library (e.g. Adafruit BME280) via `lib_deps` in
  `receiver/platformio.ini`.
- Keep it optional: if no sensor answers at start-up, log it and carry on
  receiving, so receivers without a sensor still work.

## Open questions

- Target battery life: what is the minimum acceptable?
- Indoor only, or outdoor too? This drives the case design and whether cold
  performance matters.
- Hand-solder or JLCPCB assembly? This decides SOIC/DFN vs through-hole parts.
- Wire antenna or PCB antenna?
- Receiver sensor: BME280 alone, or SHT40 + BMP280/BMP390? Indoors with the
  receiver, or on a lead outside?
