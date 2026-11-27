# Proof of concept

Throwaway work for the stripboard prototype. It is kept apart from the real
design (the round PCB in `hardware/`), which will get its own case.

## Case for the stripboard node

`case/sensor-case.scad` is a parametric OpenSCAD model of a two-part case
(base and push-fit lid) for:

- the 8 × 24 hole stripboard ([layout](../docs/wiring.md#stripboard-layout-sensor-node))
- a wired CR2032 holder
- the STX882 transmitter on wires
- a GY-21 or SHT40 breakout on wires

```
  sensor chamber   divider            electronics
 +--------------+--+------------------------------------------+
 | vents        |  |  stripboard on 4 standoffs              ||  <- front
 |  [sensor]    |  |------------------ rib -------------------|
 | vents        |  |  [CR2032 holder]  | [STX882]          ---|-- antenna slot
 +--------------+--+------------------------------------------+
  ear                                                       ear
```

- The sensor has its own chamber, vented through the walls, floor and lid.
  A wall (closed off at the top by a rib on the lid) keeps it away from heat
  from the board and transmitter. The four sensor wires pass through a notch
  in that rib.
- The stripboard sits on four corner standoffs that clear the solder joints.
  The walls, the divider and a low rib hold it in place.
- The cell holder and transmitter sit loose in a side bay.
- The antenna leaves through a slot in the far end wall. The slot is open at
  the top, so you can drop the wire in without unsoldering it.
- Ears on each end take M3 or No.4 screws for wall mounting (`ears = false`
  removes them).
- The lid is a push fit. If it is loose, add a strip of tape or reduce `fit`.

With the default sizes the outside is about 91 × 55 × 20 mm, plus the ears.

## Measure your parts first

The default sizes are guesses for typical parts. Measure yours with calipers
and set these at the top of the file:

| Parameter | What to measure | Default |
|---|---|---|
| `vero_l`, `vero_w` | Cut stripboard, length along the strips × width | 62 × 21 |
| `vero_under` | Height of the solder joints and wire links under the board, plus ~1 mm | 2.5 |
| `vero_top` | Tallest part above the board (socketed chip, capacitor, header) | 12 |
| `cell_l`, `cell_w`, `cell_h` | CR2032 holder, including the switch | 35 × 27 × 8 |
| `tx_l`, `tx_w`, `tx_h` | STX882 module, including pins and wires | 22 × 10 × 5 |
| `sens_l`, `sens_w` | Sensor breakout (Adafruit SHT40 is 25.4 × 17.8) | 15 × 12 |

If your CR2032 holder is soldered to the stripboard rather than on wires, add
its height to `vero_top` and set `cell_l` and `cell_w` to 0.

The console prints the outside size on every render.

## Rendering and printing

1. Open the file in OpenSCAD. `part = "assembly"` shows the base with ghosted
   parts and the lid lifted above it, so you can check the fit.
2. Set `part = "base"`, render (F6), and export an STL. Do the same with
   `part = "lid"`. Or use `part = "both"` to put both on one plate.
3. Print the base upright and the lid plate-down, as modelled. Neither needs
   supports.

Suggested settings: PLA or PETG, 0.2 mm layers, 0.4 mm nozzle, 3 walls, 20 %
infill.

From the command line:

```sh
openscad -D 'part="base"' -o base.stl case/sensor-case.scad
openscad -D 'part="lid"'  -o lid.stl  case/sensor-case.scad
```

This is for indoor or sheltered use only. The vents are open, so rain will get
in. The outdoor case belongs with the PCB (see
[Future plans](../docs/future-plans.md#3-case)).
