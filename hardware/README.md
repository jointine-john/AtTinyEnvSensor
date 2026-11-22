# Hardware

KiCad schematic of the sensor node PCB, ready to import into EasyEDA Pro. It
matches the drawing in [Future plans](../docs/future-plans.md#schematic-draft).

| File | What it is |
|---|---|
| `sensor-node.kicad_sch` | Schematic (KiCad 7 format, symbols embedded) |
| `sensor-node.kicad_pro` | Minimal KiCad project file |
| `gen_schematic.py` | Generates the schematic from its net table |

## Import into EasyEDA

1. Zip `sensor-node.kicad_pro` and `sensor-node.kicad_sch` together.
2. In EasyEDA Pro: **File → Import → KiCad**, and choose the zip.
3. Assign real parts. The symbols are simple boxes and the footprints are
   KiCad library names, so for each part pick the LCSC part. The `LCSC`
   field is filled in for the resistors and ceramic capacitors.
4. **Design → Update/Convert Schematic to PCB**, then lay out the board.

Every connection is a net label on a pin, with no wires. Labels with the same
name are connected.

## Changing the circuit

Edit `PARTS` in `gen_schematic.py`, then run:

    python3 hardware/gen_schematic.py

It prints the netlist so you can check it.
