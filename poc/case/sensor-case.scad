// Proof-of-concept case for the stripboard sensor node
// (veroboard + CR2032 holder + STX882 + GY-21/SHT40 breakout).
//
// Two parts: a base tray and a push-fit lid. The sensor sits in its own
// vented chamber, separated from the electronics by a wall, so heat from the
// board and the transmitter doesn't skew the readings.
//
// Every size below is a GUESS for typical parts. Measure yours and edit the
// numbers before printing (see poc/README.md for what to measure).
//
// Render a single part for export by setting `part`:
//   "base", "lid", "both" (side by side for printing) or "assembly" (preview
//   with ghosted parts, lid lifted).

part = "assembly";

/* [Print] */
wall      = 2.0;   // side wall thickness
floor_t   = 1.6;   // base floor thickness
lid_t     = 1.6;   // lid plate thickness
r_out     = 3.0;   // outer corner radius (inner = r_out - wall)
fit       = 0.25;  // gap per side between lid lip and base wall (push fit)
clr       = 0.5;   // clearance around parts in their pockets

/* [Stripboard: 8 strips x 24 holes] */
vero_l     = 62;   // length along the strips (24 x 2.54 = 61, plus edge)
vero_w     = 21;   // width across the strips (8 x 2.54 = 20.3, plus edge)
vero_t     = 1.6;  // board thickness
vero_under = 2.5;  // standoff height, clears solder joints and cut tracks
vero_top   = 12;   // tallest part above the board (socketed ATtiny85 ~9 mm)

/* [CR2032 holder, loose in a side bay] */
cell_l = 35;       // generic wired CR2032 box with switch
cell_w = 27;
cell_h = 8;

/* [STX882 transmitter, loose in the side bay] */
tx_l = 22;
tx_w = 10;
tx_h = 5;

/* [Sensor breakout in the vented chamber] */
sens_l   = 15;     // GY-21 ~ 15 x 12 (Adafruit SHT40 is 25.4 x 17.8)
sens_w   = 12;
sens_air = 6;      // extra free air around the sensor along the case length

/* [Lid] */
lip_h = 3.0;       // depth of the lid lip that pushes into the base
lip_t = 1.2;       // lip wall thickness

/* [Vents] */
vent_w   = 1.6;    // slot width (keep >= 2 nozzle widths)
vent_gap = 2.0;    // web between slots

/* [Antenna and mounting] */
ant_d   = 1.8;     // slot for the 17.3 cm antenna wire
ears    = true;    // screw ears on the ends for wall mounting
ear_hole = 3.4;    // M3 / No.4 wood screw

$fn = 48;

// ---------------------------------------------------------------- layout --
// X runs along the case: sensor chamber | divider | stripboard compartment.
// Y: stripboard against the front wall, then a rib, then the side bay with
// the cell holder and transmitter in a row.

sens_ch = sens_l + 2*clr + sens_air;            // sensor chamber length
main_x0 = sens_ch + wall;                       // start of main compartment
main_l  = max(vero_l, cell_l + tx_l + 2) + 2*clr;
rib_t   = 1.2;                                  // board / bay separator
bay_w   = max(cell_w, tx_w) + 2*clr;
bay_y0  = vero_w + 2*clr + rib_t;

in_x = main_x0 + main_l;
in_y = max(bay_y0 + bay_w, sens_w + 2*clr + 6);
in_z = max(vero_under + vero_t + vero_top, cell_h, tx_h) + 1;
r_in = max(r_out - wall, 0.5);

out_x = in_x + 2*wall;
out_y = in_y + 2*wall;
out_z = floor_t + in_z;

wire_notch_w = 6;   // gap in the lid rib for the four sensor wires
wire_notch_h = 2;

echo(str("Outer size (base + lid): ", out_x, " x ", out_y, " x ",
         out_z + lid_t, " mm"));

// --------------------------------------------------------------- helpers --
module rbox(size, r) {
    hull() for (x = [r, size[0] - r], y = [r, size[1] - r])
        translate([x, y, 0]) cylinder(r = r, h = size[2]);
}

// Row of vertical slots through a wall that faces Y, spread over [x0, x1].
module y_slots(x0, x1, y, z0, z1) {
    n = floor((x1 - x0 + vent_gap) / (vent_w + vent_gap));
    span = n * vent_w + (n - 1) * vent_gap;
    if (n > 0) for (i = [0 : n - 1])
        translate([x0 + (x1 - x0 - span) / 2 + i * (vent_w + vent_gap),
                   y - 1, z0])
            cube([vent_w, wall + 2, z1 - z0]);
}

// Row of slots through a wall that faces X, spread over [y0, y1].
module x_slots(y0, y1, x, z0, z1) {
    n = floor((y1 - y0 + vent_gap) / (vent_w + vent_gap));
    span = n * vent_w + (n - 1) * vent_gap;
    if (n > 0) for (i = [0 : n - 1])
        translate([x - 1, y0 + (y1 - y0 - span) / 2 + i * (vent_w + vent_gap),
                   z0])
            cube([wall + 2, vent_w, z1 - z0]);
}

// Slots through a horizontal plate, running along Y, spread over x0..x1.
module plate_slots(x0, x1, y0, y1, z, t) {
    n = floor((x1 - x0 + vent_gap) / (vent_w + vent_gap));
    span = n * vent_w + (n - 1) * vent_gap;
    if (n > 0) for (i = [0 : n - 1])
        translate([x0 + (x1 - x0 - span) / 2 + i * (vent_w + vent_gap),
                   y0, z - 1])
            cube([vent_w, y1 - y0, t + 2]);
}

// ------------------------------------------------------------------ base --
// Origin is the inner front-left corner of the cavity at floor level, so the
// shell spans x -wall..in_x+wall, y -wall..in_y+wall, z -floor_t..in_z.
module base() {
    vz0 = 2;                    // vents start this far above the floor
    vz1 = in_z - lip_h - 1;     // and stop below the lid lip

    difference() {
        union() {
            translate([-wall, -wall, -floor_t]) rbox([out_x, out_y, out_z], r_out);
            if (ears) for (end = [0, 1]) ear(end);
        }

        // cavity
        rbox([in_x, in_y, in_z + 1], r_in);

        // sensor chamber vents: both side walls, the end wall, the floor
        y_slots(2, sens_ch - 2, -wall, vz0, vz1);
        y_slots(2, sens_ch - 2, in_y, vz0, vz1);
        x_slots(r_in + 2, in_y - r_in - 2, -wall, vz0, vz1);
        plate_slots(2, sens_ch - 2, 3, in_y - 3, -floor_t, floor_t);

        // antenna slot, open from the top, in the far end wall beside the
        // transmitter; the wire sits below the lid lip
        translate([in_x - 1, bay_y0 + bay_w / 2 - ant_d / 2, in_z - lip_h - 3])
            cube([wall + 2, ant_d, lip_h + 4]);
    }

    // divider between sensor chamber and electronics; stops lip_h short of
    // the rim, and the lid's rib closes the gap (with a notch for the wires)
    translate([sens_ch, 0, 0]) cube([wall, in_y, in_z - lip_h]);

    // stripboard: four corner standoffs, trapped by the front wall, the
    // divider, the far end wall and the rib
    for (x = [0, vero_l - 4], y = [0, vero_w - 4])
        translate([main_x0 + clr + x, clr + y, 0]) cube([4, 4, vero_under]);

    // rib between stripboard and side bay
    translate([main_x0, vero_w + 2*clr, 0])
        cube([main_l, rib_t, vero_under + vero_t + 2]);

    // low stop between cell holder and transmitter
    translate([main_x0 + clr + cell_l + clr, bay_y0, 0])
        cube([1.2, bay_w, 3]);

    // two rails to lift the sensor off the vented floor
    for (y = [in_y / 2 - sens_w / 2 + 1, in_y / 2 + sens_w / 2 - 2])
        translate([2, y, 0]) cube([sens_ch - 4, 1, 3]);
}

module ear(end) {
    x = end == 0 ? -wall : in_x + wall;
    dir = end == 0 ? -1 : 1;
    difference() {
        hull() {
            translate([x - 0.5, in_y / 2 - 6, -floor_t])
                cube([1, 12, 3]);
            translate([x + dir * 7, in_y / 2, -floor_t]) cylinder(r = 5, h = 3);
        }
        translate([x + dir * 7, in_y / 2, -floor_t - 1])
            cylinder(d = ear_hole, h = 5);
    }
}

// ------------------------------------------------------------------- lid --
// Modelled in print orientation: plate on the bed, lip and rib pointing up.
// Flipping it over to fit mirrors Y only, and the lid is symmetric in Y.
module lid() {
    difference() {
        translate([-wall, -wall, 0]) rbox([out_x, out_y, lid_t], r_out);
        plate_slots(2, sens_ch - 2, 3, in_y - 3, 0, lid_t);
    }

    // lip
    translate([0, 0, lid_t]) difference() {
        translate([fit, fit, 0])
            rbox([in_x - 2*fit, in_y - 2*fit, lip_h], max(r_in - fit, 0.3));
        translate([fit + lip_t, fit + lip_t, -1])
            rbox([in_x - 2*(fit + lip_t), in_y - 2*(fit + lip_t), lip_h + 2],
                 max(r_in - fit - lip_t, 0.3));
    }

    // rib that meets the top of the divider, with a notch for the wires
    translate([0, 0, lid_t]) difference() {
        translate([sens_ch, fit + lip_t, 0])
            cube([wall, in_y - 2*(fit + lip_t), lip_h]);
        translate([sens_ch - 1, in_y / 2 - wire_notch_w / 2, lip_h - wire_notch_h])
            cube([wall + 2, wire_notch_w, wire_notch_h + 1]);
    }
}

// ---------------------------------------------------------- placeholders --
module parts_ghost() {
    // stripboard and its tallest part
    translate([main_x0 + clr, clr, vero_under]) {
        color("peru") cube([vero_l, vero_w, vero_t]);
        color("dimgray") translate([vero_l * 0.15, vero_w * 0.25, vero_t])
            cube([10, 10, vero_top]);
    }
    color("silver") translate([main_x0 + clr, bay_y0 + clr, 0])
        cube([cell_l, cell_w, cell_h]);
    color("green") translate([main_x0 + clr + cell_l + 2*clr + 1.2, bay_y0 + clr, 0])
        cube([tx_l, tx_w, tx_h]);
    color("purple") translate([clr + sens_air / 2, in_y / 2 - sens_w / 2, 3])
        cube([sens_l, sens_w, 1.6]);
}

// ---------------------------------------------------------------- output --
if (part == "base") {
    translate([0, 0, floor_t]) base();
} else if (part == "lid") {
    lid();
} else if (part == "both") {
    translate([0, 0, floor_t]) base();
    translate([0, out_y + 10, 0]) lid();
} else {
    translate([0, 0, floor_t]) {
        base();
        %parts_ghost();
    }
    // lid flipped into place and lifted so the inside shows
    %translate([0, in_y, floor_t + in_z + lid_t + 15]) rotate([180, 0, 0]) lid();
}
