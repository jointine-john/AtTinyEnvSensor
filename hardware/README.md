# Hardware

Schematic of the sensor node PCB in EAGLE and KiCad formats, for importing into
EasyEDA Pro. Both match the drawing in [Future plans](../docs/future-plans.md#schematic-draft).

| File | What it is |
|---|---|
| `sensor-node.sch` | EAGLE schematic, with footprints for the simple parts |
| `sensor-node.kicad_sch` | Schematic (KiCad 7 format, symbols embedded) |
| `sensor-node.kicad_pro` | Minimal KiCad project file |
| `sensor-node-kicad.zip` | The two files above, zipped for import |
| `gen_schematic.py` | Generates the KiCad schematic. Its `PARTS` table is the circuit |
| `gen_eagle.py` | Generates the EAGLE schematic from the same table |

## Import the EAGLE file (try this first)

1. In EasyEDA Pro: **File → Import → EAGLE**, and choose `sensor-node.sch`.
2. Each net lists its pins explicitly, so the connections should come across
   even where the KiCad import lost them.
3. Footprints are embedded for R1–R3, C1, C2, C4 (0603/0805), U1 (SOIC-8) and
   the headers U3, J1, J2. Assign BT1, C3 and U2 from the table below.
4. **Design → Update/Convert Schematic to PCB**, then lay out the board.

## Import the KiCad file

1. In EasyEDA Pro: **File → Import → KiCad**, and choose
   `sensor-node-kicad.zip`.
2. Assign real parts. The symbols are simple boxes and the footprints are
   KiCad library names, so for each part pick the LCSC part. The `LCSC`
   field is filled in for the resistors and ceramic capacitors.
3. **Design → Update/Convert Schematic to PCB**, then lay out the board.

Each pin has a short wire with a net label on its end. Labels with the same
name are connected.

## Footprints

Picking the LCSC part in EasyEDA brings its footprint with it, so use the LCSC
number where there is one.

| Ref | Part | Footprint | In EasyEDA |
|---|---|---|---|
| U1 | ATtiny85V-10SU | SOIC-8, 150 mil body (3.9 × 4.9 mm), 1.27 mm pitch | Search `ATTINY85V-10SU`. The ATtiny85-20SU has the same footprint but is only rated down to 2.7 V |
| U2 | SHT40-AD1B | DFN-4, 1.5 × 1.5 mm, 0.8 mm pitch | Search `SHT40-AD1B`. Pins: 1 SDA, 2 SCL, 3 VDD, 4 VSS |
| U3 | STX882 module | 1×4 pin header, 2.54 mm, through-hole | No LCSC part. Check the pin order and pitch on your module |
| J1 | ISP header | 2×3 pin header, 2.54 mm, vertical, through-hole | Generic header, or 6 SMD pads for pogo pins |
| J2 | Antenna pad | Single through-hole pad, about 1.0 mm drill, 2 mm pad | For the 17.3 cm wire |
| BT1 | CR2450 holder | SMD CR2450 holder | Search `CR2450` and pick an SMD holder. Footprints differ between makers |
| C3 | 470 µF low-leakage | SMD aluminium can 8 × 10 mm, or polymer 7343 (D case) | Choose after the coin-cell bench test |
| C2 | 10 µF ceramic | 0805 | C15850 |
| C1, C4 | 100 nF ceramic | 0603 | C14663 |
| R1, R2, R3 | 10 kΩ | 0603 | C25804 |

C15850, C14663 and C25804 are JLCPCB basic parts, so assembly adds no extra
feeder fee for them.

## Changing the circuit

Edit `PARTS` in `gen_schematic.py`, then run both generators:

    python3 hardware/gen_schematic.py
    python3 hardware/gen_eagle.py

The first prints the netlist so you can check it. Rebuild
`sensor-node-kicad.zip` afterwards.
