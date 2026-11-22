#!/usr/bin/env python3
"""Generate hardware/sensor-node.kicad_sch from the PARTS table below.

Matches docs/images/pcb-schematic.svg. Each pin gets a short wire with a net
label on its end, so the PARTS table below is the whole circuit.

    python3 hardware/gen_schematic.py
"""

import pathlib
import uuid

OUT = pathlib.Path(__file__).with_name("sensor-node.kicad_sch")
PROJECT = "sensor-node"
NS = uuid.UUID("6f1c2b0e-8d1a-4a57-9a3e-1f2d3c4b5a69")  # stable UUIDs


def uid(*parts):
    return str(uuid.uuid5(NS, "/".join(map(str, parts))))


ROOT = uid("root")
G = 2.54  # grid
FONT = "(effects (font (size 1.27 1.27)))"
FONT_HIDE = "(effects (font (size 1.27 1.27)) hide)"

# ---------------------------------------------------------------- symbols
# Each symbol: graphics (list of s-exprs, lib coords, Y up) and pins
# (number, name, x, y, angle, length, etype).


def ic(name, left, right, half_w, ref="U"):
    """Box symbol. left/right: lists of (number, name, etype), top to bottom."""
    rows = max(len(left), len(right))
    half_h = (rows * G) / 2 + G / 2
    pins = []
    for side, plist in ((0, left), (180, right)):
        for i, (num, pname, et) in enumerate(plist):
            y = (rows - 1) * G / 2 - i * G
            x = -(half_w + G) if side == 0 else half_w + G
            pins.append((num, pname, x, y, side, G, et))
    gfx = [f"(rectangle (start {-half_w} {half_h}) (end {half_w} {-half_h}) "
           "(stroke (width 0.254) (type default)) (fill (type background)))"]
    return dict(name=name, ref=ref, gfx=gfx, pins=pins, names=True,
                ref_y=half_h + 1.27, val_y=-half_h - 1.27)


def two_pin(name, ref, gfx, length, top_name="1", bot_name="2"):
    pins = [("1", top_name, 0, 3.81, 270, length, "passive"),
            ("2", bot_name, 0, -3.81, 90, length, "passive")]
    return dict(name=name, ref=ref, gfx=gfx, pins=pins, names=False,
                ref_y=None, val_y=None)


def poly(*pts, w=0.254):
    p = " ".join(f"(xy {x} {y})" for x, y in pts)
    return f"(polyline (pts {p}) (stroke (width {w}) (type default)) (fill (type none)))"


SYMBOLS = {
    "ATtiny85": ic("ATtiny85",
                   [("1", "PB5/~{RESET}", "bidirectional"),
                    ("2", "PB3", "bidirectional"),
                    ("3", "PB4", "bidirectional"),
                    ("4", "GND", "power_in")],
                   [("8", "VCC", "power_in"),
                    ("7", "PB2/SCK", "bidirectional"),
                    ("6", "PB1/MISO", "bidirectional"),
                    ("5", "PB0/MOSI", "bidirectional")], 7.62),
    "SHT40": ic("SHT40",
                [("1", "SDA", "bidirectional"), ("2", "SCL", "input")],
                [("3", "VDD", "power_in"), ("4", "VSS", "power_in"),
                 ("5", "EP", "passive")], 5.08),
    "STX882": ic("STX882",
                 [("1", "VCC", "power_in"), ("2", "DATA", "input"),
                  ("3", "GND", "power_in"), ("4", "ANT", "passive")], [], 6.35),
    "AVR_ISP": ic("AVR_ISP",
                  [("1", "MISO", "passive"), ("3", "SCK", "passive"),
                   ("5", "~{RST}", "passive")],
                  [("2", "VCC", "passive"), ("4", "MOSI", "passive"),
                   ("6", "GND", "passive")], 5.08, ref="J"),
    "AntennaPad": dict(name="AntennaPad", ref="J", names=True,
                       gfx=["(circle (center 0 0) (radius 0.762) "
                            "(stroke (width 0.254) (type default)) (fill (type none)))"],
                       pins=[("1", "ANT", -3.81, 0, 0, 3.048, "passive")],
                       ref_y=2.54, val_y=-2.54),
    "R": two_pin("R", "R", ["(rectangle (start -1.016 2.54) (end 1.016 -2.54) "
                            "(stroke (width 0.254) (type default)) (fill (type none)))"], 1.27),
    "C": two_pin("C", "C", [poly((-2.032, 0.762), (2.032, 0.762), w=0.508),
                            poly((-2.032, -0.762), (2.032, -0.762), w=0.508)], 2.794),
    "C_Polarized": two_pin("C_Polarized", "C", [
        "(rectangle (start -2.286 0.508) (end 2.286 1.016) "
        "(stroke (width 0) (type default)) (fill (type none)))",
        "(rectangle (start 2.286 -0.508) (end -2.286 -1.016) "
        "(stroke (width 0) (type default)) (fill (type outline)))",
        poly((-1.778, 2.286), (-0.762, 2.286), w=0),
        poly((-1.27, 2.794), (-1.27, 1.778), w=0)], 2.794, "+", "-"),
    "Battery_Cell": two_pin("Battery_Cell", "BT", [
        "(rectangle (start -2.286 1.778) (end 2.286 1.524) "
        "(stroke (width 0) (type default)) (fill (type outline)))",
        "(rectangle (start -1.524 1.016) (end 1.524 0.508) "
        "(stroke (width 0) (type default)) (fill (type outline)))",
        poly((0.762, 3.302), (1.778, 3.302), w=0.254),
        poly((1.27, 3.81), (1.27, 2.794), w=0.254),
        poly((0, 1.778), (0, 2.54)), poly((0, 0), (0, 0.762))], 1.27, "+", "-"),
}

# ---------------------------------------------------------------- parts
# ref: (symbol, value, footprint, LCSC, x, y in grid units, {pin: net})
# Footprints are KiCad library names; EasyEDA may ask you to remap them.
FP_0603C = "Capacitor_SMD:C_0603_1608Metric"
FP_0603R = "Resistor_SMD:R_0603_1608Metric"
PARTS = {
    "BT1": ("Battery_Cell", "CR2450", "Battery:BatteryHolder_Keystone_3008_1x2450", "",
            10, 20, {"1": "VCC", "2": "GND"}),
    "C3": ("C_Polarized", "470uF low-leak", "Capacitor_SMD:CP_Elec_8x10", "",
           16, 20, {"1": "VCC", "2": "GND"}),
    "C2": ("C", "10uF", "Capacitor_SMD:C_0805_2012Metric", "C15850",
           22, 20, {"1": "VCC", "2": "GND"}),
    "C1": ("C", "100nF", FP_0603C, "C14663", 28, 20, {"1": "VCC", "2": "GND"}),
    "C4": ("C", "100nF", FP_0603C, "C14663", 34, 20, {"1": "VCC", "2": "GND"}),
    "U1": ("ATtiny85", "ATtiny85V-10SU", "Package_SO:SOIC-8_3.9x4.9mm_P1.27mm", "",
           56, 22, {"1": "RESET", "2": None, "3": None, "4": "GND",
                    "8": "VCC", "7": "SCL", "6": "DATA", "5": "SDA"}),
    "R3": ("R", "10k", FP_0603R, "C25804", 46, 40, {"1": "VCC", "2": "RESET"}),
    "R2": ("R", "10k", FP_0603R, "C25804", 52, 40, {"1": "VCC", "2": "SCL"}),
    "R1": ("R", "10k", FP_0603R, "C25804", 58, 40, {"1": "VCC", "2": "SDA"}),
    "U2": ("SHT40", "SHT40-AD1B",
           "", "",  # footprint: pick the LCSC SHT40 part (DFN-4 + centre pad)
           86, 22, {"1": "SDA", "2": "SCL", "3": "VCC", "4": "GND",
                    "5": "GND"}),  # 5: exposed centre pad, tied to VSS inside
    "U3": ("STX882", "STX882", "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical", "",
           86, 40, {"1": "VCC", "2": "DATA", "3": "GND", "4": "ANT"}),
    "J2": ("AntennaPad", "Antenna 17.3cm wire",
           "Connector_PinHeader_2.54mm:PinHeader_1x01_P2.54mm_Vertical", "",
           100, 46, {"1": "ANT"}),
    "J1": ("AVR_ISP", "ISP 2x3", "Connector_PinHeader_2.54mm:PinHeader_2x03_P2.54mm_Vertical", "",
           86, 58, {"1": "DATA", "3": "SCL", "5": "RESET",
                    "2": "VCC", "4": "SDA", "6": "GND"}),
}

NOTES = [
    "ATtiny85 coin-cell T/H sensor node. Same pin-out as the stripboard build.",
    "No regulator: everything runs from the CR2450 (3.0 V fresh to ~2.0 V).",
    "C3 value from the coin-cell bench test (220-470 uF), low-leakage, near U3.",
    "C1 at U1 pin 8 and C4 at U2 VDD, as close to the pins as possible.",
    "Keep U2 at a board edge away from U3; slot the PCB around it.",
    "U3 pin order varies between STX882 modules: check yours.",
]

# ---------------------------------------------------------------- emit


def r(v):
    return f"{round(v, 3):g}"


def lib_symbol(s):
    n = s["name"]
    out = [f'(symbol "EnvSensor:{n}"']
    if s["names"]:
        out.append("(pin_names (offset 1.016))")
    else:
        out.append("(pin_numbers hide) (pin_names (offset 0) hide)")
    out.append("(in_bom yes) (on_board yes)")
    out.append(f'(property "Reference" "{s["ref"]}" (at 0 0 0) {FONT})')
    out.append(f'(property "Value" "{n}" (at 0 0 0) {FONT})')
    out.append(f'(property "Footprint" "" (at 0 0 0) {FONT_HIDE})')
    out.append(f'(property "Datasheet" "" (at 0 0 0) {FONT_HIDE})')
    out.append(f'(symbol "{n}_0_1" ' + " ".join(s["gfx"]) + ")")
    pins = []
    for num, pname, x, y, ang, ln, et in s["pins"]:
        pins.append(f'(pin {et} line (at {r(x)} {r(y)} {ang}) (length {r(ln)}) '
                    f'(name "{pname}" {FONT}) (number "{num}" {FONT}))')
    out.append(f'(symbol "{n}_1_1" ' + " ".join(pins) + ")")
    out.append(")")
    return " ".join(out)


def pin_tip(sym, num, X, Y):
    for pnum, _, x, y, ang, _, _ in sym["pins"]:
        if pnum == num:
            return X + x, Y - y, ang
    raise KeyError(num)


# Label angle/justify for a pin tip, so the text points away from the body.
LABEL_DIR = {0: (180, "right bottom"), 180: (0, "left bottom"),
             270: (90, "left bottom"), 90: (270, "right bottom")}
# Sheet direction (Y down) pointing away from the body, per pin angle.
STUB_DIR = {0: (-1, 0), 180: (1, 0), 270: (0, -1), 90: (0, 1)}
STUB = G


def main():
    body = []
    nets = {}
    for ref, (sname, value, fp, lcsc, gx, gy, conns) in PARTS.items():
        sym = SYMBOLS[sname]
        X, Y = gx * G, gy * G
        if sym["ref_y"] is None:  # two-pin parts: text to the right
            ref_at, val_at = (X + 2.54, Y - 1.27, 0), (X + 2.54, Y + 1.27, 0)
            just = " (justify left)"
        else:
            ref_at, val_at = (X, Y - sym["ref_y"], 0), (X, Y - sym["val_y"], 0)
            just = ""
        props = [("Reference", ref, ref_at, False), ("Value", value, val_at, False),
                 ("Footprint", fp, (X, Y, 0), True), ("Datasheet", "", (X, Y, 0), True),
                 ("LCSC", lcsc, (X, Y, 0), True)]
        p_str = []
        for k, v, (px, py, pa), hide in props:
            eff = (f"(effects (font (size 1.27 1.27)){just}{' hide' if hide else ''})")
            p_str.append(f'(property "{k}" "{v}" (at {r(px)} {r(py)} {pa}) {eff})')
        pin_uuids = " ".join(f'(pin "{p[0]}" (uuid {uid(ref, "pin", p[0])}))'
                             for p in sym["pins"])
        body.append(
            f'(symbol (lib_id "EnvSensor:{sname}") (at {r(X)} {r(Y)} 0) (unit 1) '
            f'(in_bom yes) (on_board yes) (dnp no) (uuid {uid(ref)}) '
            + " ".join(p_str) + " " + pin_uuids +
            f' (instances (project "{PROJECT}" (path "/{ROOT}" '
            f'(reference "{ref}") (unit 1)))))')
        for num, net in conns.items():
            tx, ty, ang = pin_tip(sym, num, X, Y)
            if net is None:
                body.append(f"(no_connect (at {r(tx)} {r(ty)}) (uuid {uid(ref, 'nc', num)}))")
                continue
            nets.setdefault(net, []).append(f"{ref}.{num}")
            # Short wire out from the pin, label on its far end. Importers
            # such as EasyEDA only attach labels to wires, not to bare pins.
            dx, dy = STUB_DIR[ang]
            ex, ey = tx + dx * STUB, ty + dy * STUB
            body.append(f"(wire (pts (xy {r(tx)} {r(ty)}) (xy {r(ex)} {r(ey)})) "
                        f"(stroke (width 0) (type default)) (uuid {uid(ref, 'wire', num)}))")
            la, lj = LABEL_DIR[ang]
            body.append(f'(label "{net}" (at {r(ex)} {r(ey)} {la}) '
                        f'(effects (font (size 1.27 1.27)) (justify {lj})) '
                        f'(uuid {uid(ref, "label", num)}))')
        # every pin must be connected or marked no-connect
        missing = {p[0] for p in sym["pins"]} - set(conns)
        assert not missing, f"{ref}: unassigned pins {missing}"

    for i, line in enumerate(NOTES):
        body.append(f'(text "{line}" (at {r(10 * G)} {r((64 + 2 * i) * G)} 0) '
                    f'(effects (font (size 1.27 1.27)) (justify left bottom)) '
                    f'(uuid {uid("note", i)}))')

    libs = " ".join(lib_symbol(SYMBOLS[n]) for n in
                    sorted({p[0] for p in PARTS.values()}))
    doc = (f'(kicad_sch (version 20230121) (generator eeschema) (uuid {ROOT}) '
           f'(paper "A4") (title_block (title "Sensor node PCB (CR2450)") '
           f'(rev "0.1") (comment 1 "Generated by hardware/gen_schematic.py")) '
           f'(lib_symbols {libs}) ' + " ".join(body) +
           ' (sheet_instances (path "/" (page "1"))))')
    OUT.write_text(pretty(doc))

    for net in sorted(nets):
        print(f"{net:6} {' '.join(nets[net])}")


def pretty(s):
    """Indent the s-expression one node per line (strings kept intact)."""
    out, depth, i, in_str = [], 0, 0, False
    line = ""
    while i < len(s):
        c = s[i]
        if in_str:
            line += c
            if c == "\\":
                line += s[i + 1]
                i += 1
            elif c == '"':
                in_str = False
        elif c == '"':
            in_str = True
            line += c
        elif c == "(":
            if depth and line.strip():
                out.append(line.rstrip())
                line = ""
            line = "  " * depth + "(" if not line.strip() else line + "("
            depth += 1
        elif c == ")":
            depth -= 1
            line += ")"
        elif c == " " and i + 1 < len(s) and s[i + 1] == "(":
            pass
        else:
            line += c
        i += 1
    out.append(line.rstrip())
    assert depth == 0 and not in_str, "unbalanced s-expression"
    return "\n".join(out) + "\n"


if __name__ == "__main__":
    main()
