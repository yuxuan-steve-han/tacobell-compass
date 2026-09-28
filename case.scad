// Taco Bell compass, round body + front lobe (v6: v5 + keychain lug)
// Parts: Waveshare ESP32-S3-Touch-LCD-1.28, ATGM336H GPS (separate antenna),
//        QMC5883L (GY-271), 103035 LiPo, passive piezo buzzer
//
// Top view (+Y = front / pointing direction):
//   Round body: display on top, battery underneath, USB-C at the back,
//               piezo buzzer standing on the -X side behind sound holes.
//   Lobe:       GPS antenna taped under the lid (only plastic above it);
//               below it, side by side on the floor: ATGM336H module (+X)
//               and QMC5883L (-X), the compass pushed forward away from the battery.
//
// Board sizes come from Waveshare's drawing plus caliper checks; battery, GPS module,
// buzzer, and screws are assumed with extra leeway. Update when measured.

/* [What to render] */
part = "assembly"; // [assembly, base, lid, print_layout]
show_components = true;

/* [Case] */
case_d      = 54;   // round body outer diameter
lobe_hw     = 17;   // lobe half-width (outer)
lobe_end    = 41.5; // lobe tip distance from center (outer)
lobe_corner = 4;
wall        = 2.0;
floor_t     = 1.6;
lid_t       = 2.6;
ledge_t     = 1.0;
chamfer     = 0.8;
tol         = 0.3;

/* [Waveshare ESP32-S3-Touch-LCD-1.28] */
disp_d        = 38.51; // display module outer diameter (drawing; measured ~38.5)
view_d        = 33.40; // visible area (drawing)
board_r       = 18.75; // PCB circle radius (drawing)
tab_w         = 25.28; // USB tab width (drawing)
tab_r         = 2.0;   // USB tab corner radius (drawing)
board_total_h = 39.49; // top of PCB circle to bottom of USB tab (drawing; measured ~39.5)
board_y       = -0.75; // display center; sets the USB-C gap to the back wall
disp_stack_t  = 5.0;   // measured: glass face to PCB back surface
board_back_h  = 3.5;   // measured 8 mm from the glass face = 3 mm past the PCB, +0.5 leeway
usb_below_pcb = 1.5;   // measured 6.5 mm from the glass face = 1.5 mm past the PCB
usbc_w        = 12;
usbc_h        = 7;
usbc_relief   = 1.0;

/* [Battery 103035] */
batt_w = 35.5; // along X (assumed 35 + leeway)
batt_l = 30.5; // along Y (assumed 30 + leeway)
batt_h = 10.5; // assumed 10 + leeway
batt_y = -1;

/* [ATGM336H antenna (under the lid)] */
ant_l      = 26.5; // measured 26 + leeway, along X
ant_w      = 13.5; // measured 13 + leeway, along Y
ant_h      = 7.5;  // measured 7 + leeway
ant_y      = 26.75;
ant_pocket = 0.8;

/* [ATGM336H module (flat on the lobe floor, +X side)] */
gps_w = 13.5;  // assumed, along X
gps_l = 16;    // assumed, along Y
gps_h = 3.5;   // assumed, thickness incl. parts and wires
gps_x = 7.75;
gps_y = 24;

/* [QMC5883L (GY-271), flat on the lobe floor] */
qmc_w = 14.0;  // measured, along X
qmc_l = 18.0;  // measured, along Y
qmc_h = 4.0;   // measured, thickest part
qmc_x = -7.5;
qmc_y = 29;
qmc_fence_h = 1.5;

/* [Piezo buzzer (stands upright on the -X side)] */
buz_d = 12.5;  // assumed 12 + leeway
buz_t = 4.0;   // assumed 3.5 + leeway
buz_x = -21;
buz_y = 0;
buz_hole_d = 1.5;

/* [Screws (M2 self-tapping) and front tongue] */
boss_pts      = [[21.5, -9], [-21.5, -9]];
boss_d        = 4.4;
pilot_d       = 1.9;   // slightly loose for PLA so bosses don't crack
pilot_depth   = 8;
screw_clear_d = 2.4;
screw_head_d  = 4.2;
screw_head_h  = 1.6;   // DIN 7981 M2 pan head

/* [Board support arms] */
arms    = true;
arm_tip = 15;    // |X| where each arm ends under the PCB edge
arm_w   = 5;
arm_t   = 1.6;

/* [Keychain lug] */
keychain   = true;
key_angle  = 225;   // position around the round body (0 = +X, 90 = front); avoid 250-290 (USB-C)
key_w      = 12;    // lug width (leaves ~3.75 mm of plastic around the hole; sized for PLA)
key_t      = 5;     // lug thickness (prints flat on the bed; sized for PLA)
key_reach  = 6;     // hole center distance past the outer wall
key_hole   = 4.5;   // fits typical split rings and lanyard loops

/* [Optional wall switch hole] */
switch_hole  = true; // set true once you have the switch; adjust size to its knob/body
switch_angle = 0;     // +X side: clear of the bosses, buzzer, USB-C, and lobe; free space beside the battery
switch_w     = 8;
switch_h     = 4;
switch_z     = 6;

$fn = 96;
eps = 0.02;   // small overlap so touching parts fuse cleanly (avoids non-manifold warnings)

// ---------- derived ----------
inner_r    = case_d / 2 - wall;
lobe_inner = lobe_end - wall;
batt_top   = floor_t + batt_h;
pcb_bottom = batt_top + 0.5 + board_back_h;
glass_top  = pcb_bottom + disp_stack_t;
base_h     = glass_top - (lid_t - ledge_t);
tab_bottom = board_y + board_r - board_total_h;
usbc_z     = pcb_bottom - usb_below_pcb;
ant_bottom = base_h + ant_pocket - ant_h;
qmc_top    = floor_t + qmc_h;
gps_top    = floor_t + gps_h;

echo(str("Case: ", case_d, " x ", case_d / 2 + lobe_end, " x ", base_h + lid_t, " mm"));
echo(str("USB tab edge to inner wall: ", inner_r + tab_bottom, " mm"));
echo(str("USB tab corner reach vs inner radius: ",
         sqrt(pow(tab_w / 2 - tab_r, 2) + pow(tab_bottom + tab_r, 2)) + tab_r, " / ", inner_r));
echo(str("Battery corner reach vs inner radius: ",
         sqrt(pow(batt_w / 2 + tol, 2) + pow(batt_y - batt_l / 2 - tol, 2)), " / ", inner_r));
echo(str("Display edge to antenna: ", (ant_y - ant_w / 2) - (board_y + disp_d / 2), " mm"));
echo(str("Battery front edge to QMC edge: ", (qmc_y - qmc_l / 2) - (batt_y + batt_l / 2), " mm"));
echo(str("GPS module / compass top to antenna bottom: ", ant_bottom - max(gps_top, qmc_top), " mm"));
echo(str("Gap between GPS module and compass: ", (gps_x - gps_w / 2) - (qmc_x + qmc_w / 2) - 2 * tol, " mm"));
echo(str("Lobe inner half-width vs parts: ", lobe_hw - wall, " / ", max(gps_x + gps_w / 2, -(qmc_x - qmc_w / 2), ant_l / 2) + tol));
echo(str("Arm support under PCB edge: ",
         sqrt(pow(board_r, 2) - pow(boss_pts[0][1] - board_y, 2)) - arm_tip, " mm"));

// ---------- helpers ----------
module outline(o = 0) {
    offset(delta = o) hull() {
        circle(d = case_d);
        translate([-lobe_hw, -1])
            offset(r = lobe_corner) offset(delta = -lobe_corner)
                square([2 * lobe_hw, lobe_end + 1]);
    }
}

module shell_solid(h, cb = 0, ct = 0) {
    hull() {
        translate([0, 0, cb]) linear_extrude(h - cb - ct) outline();
        linear_extrude(0.01) outline(-cb);
        translate([0, 0, h - 0.01]) linear_extrude(0.01) outline(-ct);
    }
}

module inner_volume(h = 100) { translate([0, 0, floor_t - eps]) linear_extrude(h) outline(-wall + 0.01); }

module board2d(o = 0) {
    offset(delta = o) union() {
        translate([0, board_y]) circle(r = board_r);
        translate([-tab_w / 2, tab_bottom])
            offset(r = tab_r) offset(delta = -tab_r)
                square([tab_w, board_y - tab_bottom + 2 * tab_r]);
    }
}

module slot_z(w, h, depth) {
    hull() for (x = [-(w - h) / 2, (w - h) / 2]) translate([x, 0, 0]) cylinder(d = h, h = depth);
}

// flat lug with a hole, sticking out from the round body at floor level
module keychain_lug() {
    rotate([0, 0, key_angle]) difference() {
        hull() {
            translate([case_d / 2 - wall + 0.5, -key_w / 2, 0]) cube([0.1, key_w, key_t]);
            translate([case_d / 2 + key_reach, 0, 0]) cylinder(d = key_w, h = key_t);
        }
        translate([case_d / 2 + key_reach, 0, -1]) cylinder(d = key_hole, h = key_t + 2, $fn = 48);
        // soften the hole edges so the ring doesn't bite into the plastic
        translate([case_d / 2 + key_reach, 0, key_t - 0.5]) cylinder(d1 = key_hole, d2 = key_hole + 1.2, h = 0.51, $fn = 48);
        translate([case_d / 2 + key_reach, 0, -0.01]) cylinder(d1 = key_hole + 1.2, d2 = key_hole, h = 0.51, $fn = 48);
    }
}

module battery_keepout() {
    translate([-(batt_w / 2 + tol + 0.3), batt_y - batt_l / 2 - tol - 0.3, -1])
        cube([batt_w + 2 * tol + 0.6, batt_l + 2 * tol + 0.6, batt_top + 0.5 + 1]);
}

// ---------- base ----------
module base() {
    difference() {
        union() {
            difference() {
                shell_solid(base_h, cb = chamfer);
                translate([0, 0, floor_t]) linear_extrude(base_h) outline(-wall);
            }
            if (keychain) keychain_lug();
            intersection() {
                union() {
                    for (p = boss_pts) translate([p[0], p[1], 0]) cylinder(d = boss_d, h = base_h);
                    if (arms) difference() { for (s = [0, 1]) arm(boss_pts[s]); battery_keepout(); }
                    battery_clips();
                    flat_fence(gps_x, gps_y, gps_w, gps_l);
                    upright_holder(buz_x, buz_y, buz_t, buz_d, 4, 1.0);
                    flat_fence(qmc_x, qmc_y, qmc_w, qmc_l);
                }
                union() { inner_volume(base_h); translate([0, 0, -1]) linear_extrude(base_h + 2) outline(-0.01); }
            }
        }
        // screw pilots
        for (p = boss_pts) translate([p[0], p[1], base_h - pilot_depth]) cylinder(d = pilot_d, h = pilot_depth + 1);
        // USB-C at the back, with outer relief
        translate([0, -case_d / 2 - 1, usbc_z]) rotate([-90, 0, 0]) {
            slot_z(usbc_w, usbc_h, wall + 4);
            slot_z(usbc_w + 2, usbc_h + 2, 1 + usbc_relief);
        }
        // buzzer sound holes through the -X wall
        for (dy = [-3, 0, 3], dz = [-3, 0, 3])
            if (abs(dy) + abs(dz) <= 3)
                translate([-case_d / 2 - 1, buz_y + dy, floor_t + buz_d / 2 + dz])
                    rotate([0, 90, 0]) cylinder(d = buz_hole_d, h = wall + 3, $fn = 24);
        // slot for the lid tongue at the lobe tip
        translate([-4.2, lobe_inner - 0.01, base_h - 2.4]) cube([8.4, 1.2, 1.6]);
        // optional switch
        if (switch_hole)
            rotate([0, 0, switch_angle]) translate([inner_r - 1, -switch_w / 2, switch_z - switch_h / 2])
                cube([wall + 2, switch_w, switch_h]);
    }
}

// support arm from a boss to under the PCB edge; 45 degree underside
module arm(p) {
    s = p[0] > 0 ? 1 : -1;
    bx = abs(p[0]);
    drop = bx - arm_tip;
    hull() {
        translate([s > 0 ? arm_tip : -bx, p[1] - arm_w / 2, pcb_bottom - arm_t])
            cube([bx - arm_tip, arm_w, arm_t]);
        translate([s > 0 ? bx - 0.01 : -bx, p[1] - arm_w / 2, pcb_bottom - arm_t - drop])
            cube([0.01, arm_w, drop + arm_t]);
    }
}

module battery_clips() {
    arm_len = 5; t = 1.2; h = 4;
    for (sx = [-1, 1], sy = [-1, 1])
        translate([sx * (batt_w / 2 + tol), batt_y + sy * (batt_l / 2 + tol), floor_t - eps])
            scale([sx, sy, 1]) {
                translate([0, -arm_len, 0]) cube([t, arm_len + t, h + eps]);
                translate([-arm_len, 0, 0]) cube([arm_len + t, t, h + eps]);
            }
}

// two rails holding a flat part upright; thickness along X, length along Y
module upright_holder(cx, cy, th, len, h, rail) {
    for (sx = [-1, 1])
        translate([cx + sx * (th / 2 + tol) + (sx < 0 ? -rail : 0), cy - len / 2 + 1, floor_t - eps])
            cube([rail, len - 2, h + eps]);
}

// low corner guides for a flat part taped to the floor; back side left open for wires
module flat_fence(cx, cy, w, l) {
    t = 1.0; arm_len = 4;
    for (sx = [-1, 1])   // front corners
        translate([cx + sx * (w / 2 + tol), cy + (l / 2 + tol), floor_t - eps])
            scale([sx, 1, 1]) {
                translate([0, -arm_len, 0]) cube([t, arm_len + t, qmc_fence_h + eps]);
                translate([-arm_len, 0, 0]) cube([arm_len + t, t, qmc_fence_h + eps]);
            }
    for (sx = [-1, 1])   // back side guides
        translate([cx + sx * (w / 2 + tol) + (sx < 0 ? -t : 0), cy - l / 2, floor_t - eps])
            cube([t, 3, qmc_fence_h + eps]);
}

// ---------- lid (modeled top-up; printed flipped) ----------
module lid() {
    difference() {
        shell_solid(lid_t, ct = chamfer);
        translate([0, board_y, -1]) cylinder(d = view_d + 1, h = lid_t + 2);
        translate([0, board_y, lid_t - 0.8]) cylinder(d1 = view_d + 1, d2 = view_d + 2.6, h = 0.81);
        translate([0, board_y, -1]) cylinder(d = disp_d + 2 * tol, h = lid_t - ledge_t + 1);
        translate([-(ant_l / 2 + tol), ant_y - (ant_w / 2 + tol), -1])
            cube([ant_l + 2 * tol, ant_w + 2 * tol, ant_pocket + 1]);
        for (p = boss_pts) translate([p[0], p[1], 0]) {
            translate([0, 0, -1]) cylinder(d = screw_clear_d, h = lid_t + 2);
            translate([0, 0, lid_t - screw_head_h]) cylinder(d = screw_head_d, h = screw_head_h + 1);
        }
    }
    // tongue: hook the tip in first, press the lid down, then screw
    stem_y = lobe_inner - tol - 1.2;
    translate([-3.8, stem_y, -2.2]) cube([7.6, 1.2, 2.2]);
    translate([-3.8, stem_y, -2.2]) cube([7.6, 1.2 + tol + 1.0, 1.2]);
}

// ---------- ghost components ----------
module components() {
    color("SteelBlue", 0.5)
        translate([-batt_w / 2, batt_y - batt_l / 2, floor_t - eps]) cube([batt_w, batt_l, batt_h]);
    color("DarkBlue", 0.6)
        translate([0, 0, pcb_bottom]) linear_extrude(1.6) board2d();
    color("Silver", 0.7)
        translate([-usbc_w / 2 + 1, tab_bottom, pcb_bottom - 3.2]) cube([usbc_w - 2, 7, 3.2]);
    color("Black", 0.8)
        translate([0, board_y, pcb_bottom + 1.6]) cylinder(d = disp_d, h = glass_top - pcb_bottom - 1.6);
    color("Goldenrod", 0.7)
        translate([-ant_l / 2, ant_y - ant_w / 2, ant_bottom]) cube([ant_l, ant_w, ant_h]);
    color("DarkGreen", 0.7)
        translate([gps_x - gps_w / 2, gps_y - gps_l / 2, floor_t - eps]) cube([gps_w, gps_l, gps_h]);
    color("Purple", 0.7)
        translate([qmc_x - qmc_w / 2, qmc_y - qmc_l / 2, floor_t - eps]) cube([qmc_w, qmc_l, qmc_h]);
    color("Gold", 0.7)
        translate([buz_x, buz_y, floor_t + buz_d / 2]) rotate([0, 90, 0])
            cylinder(d = buz_d, h = buz_t, center = true);
}

// ---------- output ----------
if (part == "base") base();
else if (part == "lid") translate([0, 0, lid_t]) rotate([180, 0, 0]) lid();
else if (part == "print_layout") {
    base();
    translate([case_d + 8, 0, lid_t]) rotate([180, 0, 0]) lid();
}
else {
    base();
    translate([0, 0, base_h + 0.01]) color("WhiteSmoke", 0.6) lid();
    if (show_components) components();
}
