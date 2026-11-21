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

## Shopping list (UK)

Links checked November 2026. Stock and prices change, so check before ordering.
Generic modules are linked as eBay UK searches because individual listings expire.

### Per sensor

| Part | Qty | Where |
|---|---|---|
| ATtiny85-20PU (DIP-8) | 1 | [RS 133-1672](https://uk.rs-online.com/web/p/microcontrollers/1331672) (pack of 5, ~£1.25 each), [Farnell 1455162](https://uk.farnell.com/microchip/attiny85-20pu/mcu-8bit-attiny-20mhz-dip-8/dp/1455162) |
| …or ATtiny85V-10PU (rated down to 1.8 V) | 1 | [RS 133-1676](https://uk.rs-online.com/web/p/microcontrollers/1331676) |
| 8-pin DIP socket | 1 | [eBay UK](https://www.ebay.co.uk/sch/i.html?_nkw=8+pin+DIP+IC+socket) |
| SHT40 breakout | 1 | [The Pi Hut – Adafruit 4885](https://thepihut.com/products/adafruit-sensirion-sht40-temperature-humidity-sensor-stemma-qt-qwiic), [Pimoroni – Adafruit 4885](https://shop.pimoroni.com/products/adafruit-sensirion-sht40-temperature-humidity-sensor-stemma-qt-qwiic), [The Pi Hut – DFRobot Fermion](https://thepihut.com/products/fermion-sht40-humidity-temperature-sensor-breakout) |
| STX882 433 MHz transmitter | 1 | [eBay UK](https://www.ebay.co.uk/sch/i.html?_nkw=STX882), [Tindie – STX882 + SRX882 kit](https://www.tindie.com/products/icstation/stx882srx882-433mhz-transceiver8251/) |
| 2×AA holder with switch and leads | 1 | [CPC BT06555](https://cpc.farnell.com/multicomp-pro/mp000375/battery-box-2x-aa-switched-wired/dp/BT06555), [CPC BT06560 (waterproof)](https://cpc.farnell.com/pro-power/pp002109/battery-box-2x-aa-waterproof-wired/dp/BT06560), [The Pi Hut](https://thepihut.com/products/battery-holder-2xaa-with-cover-and-switch) |
| 100 nF ceramic capacitor | 1 | [eBay UK](https://www.ebay.co.uk/sch/i.html?_nkw=100nF+ceramic+capacitor+through+hole) |
| 10 µF ceramic capacitor | 1 | [eBay UK](https://www.ebay.co.uk/sch/i.html?_nkw=10uF+ceramic+capacitor+through+hole) |
| Lithium AA (Energizer L91), for outdoor/cold use | 2 | Any supermarket or [eBay UK](https://www.ebay.co.uk/sch/i.html?_nkw=Energizer+L91+lithium+AA) |
| Stripboard, solid-core wire (also used for the 17.3 cm antenna) | – | [eBay UK](https://www.ebay.co.uk/sch/i.html?_nkw=stripboard+veroboard) |

The Adafruit SHT40 board has a power LED. Cut its jumper or remove it, or it
will dominate the sleep current.

### Receiver (one)

| Part | Where |
|---|---|
| ESP32 dev board (classic ESP32-WROOM-32, matches `board = esp32dev`) | [The Pi Hut – Espressif ESP32 Developer Edition](https://thepihut.com/products/espressif-esp32-development-board-developer-edition), [The Pi Hut – Olimex ESP32-DevKit-LiPo](https://thepihut.com/products/olimex-esp32-devkit-lipo-esp32-development-board), or a generic board from [eBay UK](https://www.ebay.co.uk/sch/i.html?_nkw=ESP32+WROOM+32+devkit) |
| RXB6 433 MHz superheterodyne receiver | [eBay UK](https://www.ebay.co.uk/sch/i.html?_nkw=RXB6+433MHz) |

An ESP32-S2, S3 or C3 board also works if you change `board` in
`receiver/platformio.ini`. The SRX882 from the STX882 kit works in place of
the RXB6, but its CS (enable) pin must be tied high.

### Programmer (one)

| Part | Where |
|---|---|
| USBasp | [38-3d.co.uk](https://38-3d.co.uk/products/usbasp-isp-programmer), [eBay UK](https://www.ebay.co.uk/sch/i.html?_nkw=USBasp+programmer) (choose one with the slow-clock JP3 jumper or recent firmware) |
| 10-pin to 6-pin ISP adapter | [eBay UK](https://www.ebay.co.uk/sch/i.html?_nkw=10+pin+to+6+pin+ISP+adapter) |
| …or SparkFun Tiny AVR Programmer (ATtiny85 socket, no wiring needed; set `upload_protocol = usbtiny`) | [The Pi Hut](https://thepihut.com/products/sparkfun-tiny-avr-programmer) |

You don't need a programmer if you already have an Arduino Uno or Nano. See
[Programmer wiring](#programmer-wiring).

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

The ATtiny85 is programmed over ISP with a USBasp or an Arduino running
ArduinoISP. See [Programmer wiring](#programmer-wiring) below.

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

If the programmer can't connect, check the wiring first. The chip runs at 1 MHz,
so the ISP clock must stay below 250 kHz. The USBasp is already slowed to about
30 kHz (`-B 32`), and ArduinoISP's default of about 166 kHz is fine.

### Programmer wiring

ISP uses six pins on the ATtiny85:

```
              +-----\/-----+
     RESET  -| 1        8 |-  VCC
            -| 2        7 |-  SCK
            -| 3        6 |-  MISO
       GND  -| 4        5 |-  MOSI
              +------------+
```

Easiest is a bare chip on a breadboard with a 100 nF capacitor across pins 8 and 4.

#### USBasp (default in `platformio.ini`)

10-pin header, looking into the connector with the notch at the top:

```
  MOSI  1 ● ● 2  VCC
   NC   3 ● ● 4  GND
 RESET  5 ● ● 6  GND
  SCK   7 ● ● 8  GND
  MISO  9 ● ● 10 GND
```

| USBasp | ATtiny85 |
|---|---|
| 1 MOSI | 5 (PB0) |
| 5 RESET | 1 (PB5) |
| 7 SCK | 7 (PB2) |
| 9 MISO | 6 (PB1) |
| 2 VCC | 8 |
| 4/6/8/10 GND | 4 |

- **Voltage jumper:** 5 V is fine for a bare chip. Use 3.3 V if a bare SHT40 is
  already connected, as it is only rated to 3.6 V.
- **"cannot set sck period" warning:** older USBasp firmware prints this. It is harmless.
- **Linux USB permissions:** install PlatformIO's udev rules once:
  ```sh
  curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core/develop/platformio/assets/system/99-platformio-udev.rules | sudo tee /etc/udev/rules.d/99-platformio-udev.rules
  sudo udevadm control --reload-rules && sudo udevadm trigger
  ```

#### Arduino Uno/Nano as ISP

1. Upload **File → Examples → 11.ArduinoISP → ArduinoISP** to the Arduino.
2. Wire:

   | Arduino | ATtiny85 |
   |---|---|
   | D10 | 1 (RESET) |
   | D11 | 5 (MOSI) |
   | D12 | 6 (MISO) |
   | D13 | 7 (SCK) |
   | 5V | 8 |
   | GND | 4 |

3. Put a **10 µF capacitor between the Arduino's RESET and GND** (negative leg
   to GND) so avrdude doesn't reset the Arduino. Fit it after uploading
   ArduinoISP.
4. In `sensor-node/platformio.ini`, replace the USBasp `upload_protocol` /
   `upload_flags` lines with:
   ```ini
   upload_protocol = stk500v1
   upload_port = /dev/ttyACM0      ; Uno; a Nano is usually /dev/ttyUSB0
   upload_speed = 19200
   upload_flags =
       -P$UPLOAD_PORT
       -b$UPLOAD_SPEED
   ```

#### Reprogramming a finished sensor in place

PB0/PB1/PB2 are shared with the SHT40 and transmitter. Neither interferes with
programming, so a 6-pin ISP header on the sensor board lets you reflash
without removing the chip.

**Remove the batteries first**, or set the programmer to 3.3 V and leave its
VCC disconnected. Otherwise the programmer's 5 V back-feeds the AA cells,
which is unsafe for alkalines.

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
