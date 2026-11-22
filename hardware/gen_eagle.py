#!/usr/bin/env python3
"""Generate hardware/sensor-node.sch, an EAGLE (XML) schematic.

Uses the same PARTS and SYMBOLS tables as gen_schematic.py, so the KiCad and
EAGLE files describe the same circuit. Unlike KiCad, EAGLE lists each net's
pins explicitly (<pinref>), so an importer doesn't have to work out
connections from where wires and labels touch.

Footprints for the simple parts (0603, 0805, SOIC-8, pin headers) are embedded.
BT1, C3 and U2 have no footprint: assign them in EasyEDA.

    python3 hardware/gen_eagle.py
"""

import pathlib
import re
from xml.sax.saxutils import escape, quoteattr

from gen_schematic import NOTES, PARTS, SYMBOLS, G

OUT = pathlib.Path(__file__).with_name("sensor-node.sch")
LIB = "envsensor"
GATE = "G$1"
BASE_Y = 200.0  # KiCad sheet Y (down) -> EAGLE Y (up)
STUB = G

DIRECTION = {"bidirectional": "io", "input": "in", "power_in": "pas",
             "passive": "pas"}
# KiCad pin angle -> EAGLE rotation, and the outward direction (EAGLE, Y up).
ROT = {0: "R0", 180: "R180", 90: "R90", 270: "R270"}
OUTWARD = {0: (-1, 0), 180: (1, 0), 270: (0, 1), 90: (0, -1)}


def f(v):
    return f"{round(v, 4):g}"


def pin_name(name):
    return re.sub(r"~\{([^}]*)\}", r"\1", name)


# ---------------------------------------------------------------- packages
def smd_2pad(name, pitch, dx, dy, body_w, body_h):
    h = pitch / 2
    return (name, [f'<smd name="1" x="{f(-h)}" y="0" dx="{f(dx)}" dy="{f(dy)}" layer="1"/>',
                   f'<smd name="2" x="{f(h)}" y="0" dx="{f(dx)}" dy="{f(dy)}" layer="1"/>']
            + outline(body_w, body_h, 51) + names(body_h / 2 + 0.6))


def outline(w, h, layer):
    x, y = w / 2, h / 2
    pts = [(-x, -y), (x, -y), (x, y), (-x, y), (-x, -y)]
    return [f'<wire x1="{f(a[0])}" y1="{f(a[1])}" x2="{f(b[0])}" y2="{f(b[1])}" '
            f'width="0.127" layer="{layer}"/>' for a, b in zip(pts, pts[1:])]


def names(y):
    return [f'<text x="0" y="{f(y)}" size="0.8" layer="25" align="bottom-center">&gt;NAME</text>',
            f'<text x="0" y="{f(-y)}" size="0.8" layer="27" align="top-center">&gt;VALUE</text>']


def soic8():
    pads = []
    for i in range(4):
        y = 1.905 - i * 1.27
        pads.append(f'<smd name="{i + 1}" x="-2.475" y="{f(y)}" dx="1.95" dy="0.6" layer="1"/>')
        pads.append(f'<smd name="{8 - i}" x="2.475" y="{f(y)}" dx="1.95" dy="0.6" layer="1"/>')
    dot = '<circle x="-3.6" y="2.6" radius="0.15" width="0.3" layer="21"/>'
    return ("SOIC-8_3.9X4.9_P1.27", pads + outline(3.9, 4.9, 21) + [dot] + names(3.1))


def header(name, cols, rows):
    """Pin 1 square. Numbered across each row: 1 2 / 3 4 / ... (AVR ISP order)."""
    pads = []
    n = 1
    for r in range(rows):
        for c in range(cols):
            shape = ' shape="square"' if n == 1 else ""
            pads.append(f'<pad name="{n}" x="{f(c * G)}" y="{f(-r * G)}" '
                        f'drill="1" diameter="1.7"{shape}/>')
            n += 1
    w, h = cols * G, rows * G
    body = [f'<wire x1="{f(a)}" y1="{f(b)}" x2="{f(c)}" y2="{f(d)}" width="0.127" layer="21"/>'
            for a, b, c, d in ((-G / 2, G / 2, w - G / 2, G / 2),
                               (w - G / 2, G / 2, w - G / 2, G / 2 - h),
                               (w - G / 2, G / 2 - h, -G / 2, G / 2 - h),
                               (-G / 2, G / 2 - h, -G / 2, G / 2))]
    return (name, pads + body +
            [f'<text x="{f(-G / 2)}" y="{f(G / 2 + 0.4)}" size="0.8" layer="25">&gt;NAME</text>'])


PACKAGES = dict([
    smd_2pad("R0603", 1.65, 0.95, 1.0, 1.6, 0.8),
    smd_2pad("C0603", 1.65, 0.95, 1.0, 1.6, 0.8),
    smd_2pad("C0805", 1.9, 1.0, 1.45, 2.0, 1.25),
    soic8(),
    header("HDR_1X01_P2.54", 1, 1),
    header("HDR_1X04_P2.54", 1, 4),
    header("HDR_2X03_P2.54", 2, 3),
])

# ref -> package (None: no footprint, assign in EasyEDA)
PACKAGE_OF = {
    "BT1": None, "C3": None, "U2": None,
    "C1": "C0603", "C4": "C0603", "C2": "C0805",
    "R1": "R0603", "R2": "R0603", "R3": "R0603",
    "U1": "SOIC-8_3.9X4.9_P1.27",
    "U3": "HDR_1X04_P2.54", "J2": "HDR_1X01_P2.54", "J1": "HDR_2X03_P2.54",
}


# ---------------------------------------------------------------- symbols
NUM = r"(-?[\d.]+)"


def symbol_graphics(sym):
    """Convert the KiCad graphics strings in SYMBOLS to EAGLE elements."""
    out = []
    for g in sym["gfx"]:
        width = float(m.group(1)) if (m := re.search(r"\(width " + NUM, g)) else 0.254
        width = width or 0.254
        if g.startswith("(rectangle"):
            x1, y1, x2, y2 = map(float, re.search(
                rf"\(start {NUM} {NUM}\) \(end {NUM} {NUM}\)", g).groups())
            if "(type outline)" in g:
                out.append(f'<rectangle x1="{f(min(x1, x2))}" y1="{f(min(y1, y2))}" '
                           f'x2="{f(max(x1, x2))}" y2="{f(max(y1, y2))}" layer="94"/>')
                continue
            pts = [(x1, y1), (x2, y1), (x2, y2), (x1, y2), (x1, y1)]
        elif g.startswith("(polyline"):
            pts = [tuple(map(float, p)) for p in re.findall(rf"\(xy {NUM} {NUM}\)", g)]
        elif g.startswith("(circle"):
            cx, cy, rad = map(float, re.search(
                rf"\(center {NUM} {NUM}\) \(radius {NUM}\)", g).groups())
            out.append(f'<circle x="{f(cx)}" y="{f(cy)}" radius="{f(rad)}" '
                       f'width="{f(width)}" layer="94"/>')
            continue
        else:
            raise ValueError(g)
        out += [f'<wire x1="{f(a[0])}" y1="{f(a[1])}" x2="{f(b[0])}" y2="{f(b[1])}" '
                f'width="{f(width)}" layer="94"/>' for a, b in zip(pts, pts[1:])]
    return out


def eagle_symbol(sym):
    els = symbol_graphics(sym)
    for num, name, x, y, ang, ln, et in sym["pins"]:
        attrs = f'x="{f(x)}" y="{f(y)}" direction="{DIRECTION[et]}" rot="{ROT[ang]}"'
        if sym["names"] and abs(ln - G) < 1e-6:  # box symbols: named 2.54 mm pins
            els.append(f'<pin name={quoteattr(pin_name(name))} {attrs} '
                       f'length="short" visible="pin"/>')
        else:  # other pins: draw the lead, connection point only
            dx, dy = {0: (1, 0), 180: (-1, 0), 90: (0, 1), 270: (0, -1)}[ang]
            els.append(f'<wire x1="{f(x)}" y1="{f(y)}" x2="{f(x + dx * ln)}" '
                       f'y2="{f(y + dy * ln)}" width="0.254" layer="94"/>')
            els.append(f'<pin name={quoteattr(pin_name(name))} {attrs} '
                       f'length="point" visible="off"/>')
    ys = [p[3] for p in sym["pins"]]
    top, bot = max(ys) + 2.54, min(ys) - 2.54
    if sym["names"]:
        els.append(f'<text x="0" y="{f(top)}" size="1.778" layer="95" align="bottom-center">&gt;NAME</text>')
        els.append(f'<text x="0" y="{f(bot)}" size="1.778" layer="96" align="top-center">&gt;VALUE</text>')
    else:
        els.append('<text x="2.54" y="0.635" size="1.778" layer="95">&gt;NAME</text>')
        els.append('<text x="2.54" y="-0.635" size="1.778" layer="96" align="top-left">&gt;VALUE</text>')
    return f'<symbol name="{sym["name"]}">' + "".join(els) + "</symbol>"


def deviceset(ref, sname, value):
    """One deviceset per symbol+package pairing, named after both."""
    sym = SYMBOLS[sname]
    pkg = PACKAGE_OF[ref]
    ds_name = sname
    if pkg:
        pads = re.findall(r'(?:smd|pad) name="([^"]+)"', "".join(PACKAGES[pkg]))
        connects = "".join(
            f'<connect gate="{GATE}" pin={quoteattr(pin_name(p[1]))} pad="{p[0]}"/>'
            for p in sym["pins"])
        missing = {p[0] for p in sym["pins"]} - set(pads)
        assert not missing, f"{ref}: package {pkg} has no pads {missing}"
        device = (f'<device name="{pkg}" package="{pkg}"><connects>{connects}</connects>'
                  '<technologies><technology name=""/></technologies></device>')
    else:
        device = '<device name=""><technologies><technology name=""/></technologies></device>'
    return ds_name, sym["ref"], device


LAYERS = [(1, "Top", 4), (16, "Bottom", 1), (17, "Pads", 2), (18, "Vias", 2),
          (19, "Unrouted", 6), (20, "Dimension", 15), (21, "tPlace", 7),
          (22, "bPlace", 7), (25, "tNames", 7), (26, "bNames", 7),
          (27, "tValues", 7), (28, "bValues", 7), (29, "tStop", 7),
          (30, "bStop", 7), (31, "tCream", 7), (32, "bCream", 7),
          (39, "tKeepout", 4), (40, "bKeepout", 1), (44, "Drills", 7),
          (45, "Holes", 7), (51, "tDocu", 7), (52, "bDocu", 7),
          (91, "Nets", 2), (92, "Busses", 1), (93, "Pins", 2), (94, "Symbols", 4),
          (95, "Names", 7), (96, "Values", 7), (97, "Info", 7), (98, "Guide", 6)]


def main():
    # devicesets: group by (symbol, package)
    devsets = {}
    part_ds = {}
    for ref, (sname, value, *_rest) in PARTS.items():
        ds_name, prefix, device = deviceset(ref, sname, value)
        ds = devsets.setdefault(ds_name, {"prefix": prefix, "devices": {}})
        dev_name = PACKAGE_OF[ref] or ""
        ds["devices"][dev_name] = device
        part_ds[ref] = (ds_name, dev_name)

    used_pkgs = sorted({p for p in PACKAGE_OF.values() if p})
    packages = "".join(f'<package name="{p}">' + "".join(PACKAGES[p]) + "</package>"
                       for p in used_pkgs)
    symbols = "".join(eagle_symbol(SYMBOLS[s]) for s in sorted(devsets))
    dsets = "".join(
        f'<deviceset name="{n}" prefix="{d["prefix"]}"><gates>'
        f'<gate name="{GATE}" symbol="{n}" x="0" y="0"/></gates>'
        f'<devices>{"".join(d["devices"].values())}</devices></deviceset>'
        for n, d in sorted(devsets.items()))

    parts, instances = [], []
    nets = {}
    for ref, (sname, value, _kfp, lcsc, gx, gy, conns) in PARTS.items():
        ds_name, dev_name = part_ds[ref]
        attr = (f'<attribute name="LCSC" value="{lcsc}"/>' if lcsc else "")
        parts.append(f'<part name="{ref}" library="{LIB}" deviceset="{ds_name}" '
                     f'device="{dev_name}" value={quoteattr(value)}>{attr}</part>')
        X, Y = gx * G, BASE_Y - gy * G
        instances.append(f'<instance part="{ref}" gate="{GATE}" x="{f(X)}" y="{f(Y)}"/>')
        sym = SYMBOLS[sname]
        for num, name, px, py, ang, _ln, _et in sym["pins"]:
            net = conns[num]
            if net is None:
                continue
            tx, ty = X + px, Y + py
            dx, dy = OUTWARD[ang]
            ex, ey = tx + dx * STUB, ty + dy * STUB
            lrot = {(-1, 0): "R180", (1, 0): "R0", (0, 1): "R90", (0, -1): "R270"}[(dx, dy)]
            nets.setdefault(net, []).append(
                f'<segment><pinref part="{ref}" gate="{GATE}" pin={quoteattr(pin_name(name))}/>'
                f'<wire x1="{f(tx)}" y1="{f(ty)}" x2="{f(ex)}" y2="{f(ey)}" width="0.1524" layer="91"/>'
                f'<label x="{f(ex)}" y="{f(ey)}" size="1.778" layer="95" rot="{lrot}"/></segment>')

    net_xml = "".join(f'<net name="{n}" class="0">' + "".join(segs) + "</net>"
                      for n, segs in sorted(nets.items()))
    plain = "".join(f'<text x="{f(10 * G)}" y="{f(BASE_Y - (64 + 2 * i) * G)}" '
                    f'size="1.778" layer="97">{escape(t)}</text>'
                    for i, t in enumerate(NOTES))
    layers = "".join(f'<layer number="{n}" name="{name}" color="{c}" fill="1" '
                     f'visible="yes" active="yes"/>' for n, name, c in LAYERS)

    doc = f"""<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE eagle SYSTEM "eagle.dtd">
<eagle version="9.6.2">
<drawing>
<settings><setting alwaysvectorfont="no"/><setting verticaltext="up"/></settings>
<grid distance="0.1" unitdist="inch" unit="inch" style="lines" multiple="1" display="no" altdistance="0.01" altunitdist="inch" altunit="inch"/>
<layers>{layers}</layers>
<schematic xreflabel="%F%N/%S.%C%R" xrefpart="/%S.%C%R">
<description>Sensor node PCB (CR2450). Generated by hardware/gen_eagle.py</description>
<libraries><library name="{LIB}">
<packages>{packages}</packages>
<symbols>{symbols}</symbols>
<devicesets>{dsets}</devicesets>
</library></libraries>
<attributes/>
<variantdefs/>
<classes><class number="0" name="default" width="0" drill="0"/></classes>
<parts>{"".join(parts)}</parts>
<sheets><sheet>
<plain>{plain}</plain>
<instances>{"".join(instances)}</instances>
<busses/>
<nets>{net_xml}</nets>
</sheet></sheets>
</schematic>
</drawing>
</eagle>
"""
    OUT.write_text(pretty(doc))


def pretty(doc):
    """Put each element on its own line so diffs stay readable."""
    import xml.dom.minidom
    dom = xml.dom.minidom.parseString(doc.encode())
    text = dom.toprettyxml(indent=" ", encoding="utf-8").decode()
    # minidom rewrites the DOCTYPE over two lines; put EAGLE's usual one back
    text = re.sub(r"<!DOCTYPE[^>]*>", '<!DOCTYPE eagle SYSTEM "eagle.dtd">', text)
    return "\n".join(ln for ln in text.splitlines() if ln.strip()) + "\n"


if __name__ == "__main__":
    main()
