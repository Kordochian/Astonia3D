"""Builds Cameron's fortress (the start area, server "fort" 126,179) into the open world map as an
AstoniaBuilding made of placeholder kit pieces, laid out from the original cameron.map.

What comes from the map (tile = 200 cm, tile x,y -> UE X,Y, zone actor at the world origin):
- Curtain walls on the map's wall lines: north y168-169, west x114-115, south y190-191, east x144-145
  (two tiles: stone 14030-14033 outside, wall-face sprites 16175-16190 inside), the east side of the
  north yard x134 and the step y176 (one tile on the map, built two tiles thick like the rest).
- The gate in the north wall between the jambs at x120 and x126 (passage x121-125).
- Covered galleries wherever the map marks the strip inside a wall as MF_INDOORS.
- Buttresses where wall decorations 14052-14055 sit just outside a wall.
- Trees (21000-21499, forest edge 16020-16025), camp props (16152-16174), braziers (nightlight items).
- Floors from the ground sprites: paved courtyard 59108-59116, the start mosaic 50027-50036 around
  126,179, earth 12010-12018 and path 12030-12038 in the north yard.
Added for the 3D world: round corner towers, the watch tower where the north yard meets the
courtyard, square gate towers with a bridge over the gate, stairs up to a wall walk that runs all
the way round through the towers, banners and torches.

The zone actor leaves these tiles to the fortress (LAYOUTS[...]["replaced"] in build_zone.py), so run
build_zone.py first, then this:
  UnrealEditor-Cmd.exe AstoniaUE5.uproject -run=pythonscript -script="<project>/Scripts/build_fortress.py"
Re-running rebuilds in place.
"""
import math
import os
import sys
import unreal

sys.path.insert(0, os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Scripts"))
from astonia_kit import *  # noqa: E402,F401,F403  (T, K, colours, read_map, in_rects, ensure_materials, Builder, sconce)

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
MAP_FILE = os.path.normpath(os.path.join(PROJECT, "../../source_codes/astonia_community_server35-main/zones/1/cameron.map"))
WORLD = "/Game/Maps/L_World"
LABEL = "Fortress_Cameron"
WALK = 800.0       # wall-walk floor height
THICK = 400.0      # curtain wall thickness (two map tiles)
PARAPET = 50.0
GALLERY = 400.0    # depth of the covered galleries (two map tiles)

# --- Layout (cm, world space). Tower and wall lines sit on the map's wall tiles. ---
# Towers: label, centre x, y, radius, door directions (yaw: 0 = +X east, 90 = +Y south).
TOWERS = [
    ("North-west tower", 23000, 33800, 450, [0, 90]),
    ("North tower", 26800, 33800, 450, [180, 90]),
    ("Watch tower", 26800, 35400, 560, [270, 0]),
    ("North-east tower", 29000, 35400, 450, [180, 90]),
    ("South-east tower", 29000, 38200, 450, [270, 180]),
    ("South-west tower", 23000, 38200, 450, [0, 270]),
]
WATCH = "Watch tower"
TOWER_TOP = 1400.0     # top of the walk-level room in the corner towers
WATCH_TOP = 2300.0

# Gate: square towers either side of the original passage (x121-125) and a bridge over it.
GATE_Y = (33300, 34100)
GATE_TOWERS = [(23600, 24200), (25200, 25800)]
GATE_PASSAGE = (24200, 25200)
GATE_TOP = 1350.0
BRIDGE = (33400, 34000, 650)   # y0, y1, underside

# Curtain walls: label, along axis, centre line, from, to (along), outward sign, outer map tile line.
WALLS = [
    ("North west", "x", 33800, 23000, 23600, -1, 168),
    ("North east", "x", 33800, 25800, 26800, -1, 168),
    ("North yard east", "y", 26800, 33800, 35400, +1, 134),
    ("Step", "x", 35400, 26800, 29000, -1, 176),
    ("East", "y", 29000, 35400, 38200, +1, 145),
    ("South", "x", 38200, 23000, 29000, +1, 191),
    ("West", "y", 23000, 33800, 38200, -1, 114),
]

# Stairs up to the wall walk: label, along axis, across centre, bottom (along), direction, width.
STAIRS = [
    ("West stair", "y", 23300, 37500, -1, 200),
    ("East stair", "y", 28700, 37500, -1, 200),
]
STEP_RISE = 25.0
STEP_RUN = 35.0

START = (126, 179)   # server: new characters start here (the mosaic)

# Tiles the fortress builds (the zone actor skips them): inside the walls, and the ring round them.
FLOOR_RECTS = [((114, 168), (134, 191)), ((134, 176), (145, 191))]
AREA_RECTS = [((112, 165), (142, 193)), ((143, 170), (148, 193))]




def in_masonry(x, y, margin=60.0):
    """True if the point (cm) is inside a wall, tower or gate tower (plus margin)."""
    for _, cx, cy, r, _ in TOWERS:
        if math.hypot(x - cx, y - cy) < r + margin:
            return True
    for x0, x1 in GATE_TOWERS:
        if x0 - margin < x < x1 + margin and GATE_Y[0] - margin < y < GATE_Y[1] + margin:
            return True
    for _, axis, line, a0, a1, _, _ in WALLS:
        along, perp = (x, y) if axis == "x" else (y, x)
        if min(a0, a1) - margin < along < max(a0, a1) + margin and abs(perp - line) < THICK / 2 + margin:
            return True
    return False


def tower(b, label, cx, cy, r, doors):
    watch = label == WATCH
    top = TOWER_TOP
    b.piece(K.STONE_ROUND, (cx, cy, 80), (2 * r + 70, 2 * r + 70, 160), STONE_DARK)
    b.piece(K.STONE_ROUND, (cx, cy, WALK / 2), (2 * r, 2 * r, WALK), STONE)
    b.piece(K.STONE_ROUND, (cx, cy, WALK - 20), (2 * r + 30, 2 * r + 30, 40), TRIM)
    # Walk-level room: a ring of wall segments, open where the wall walks come in.
    n, thick, door_h = 16, 60.0, 350.0
    step = 360.0 / n
    seg = 2 * r * math.tan(math.radians(step / 2)) + 8
    rr = r - thick / 2
    for k in range(n):
        a = k * step
        x, y = cx + rr * math.cos(math.radians(a)), cy + rr * math.sin(math.radians(a))
        if any(abs((a - d + 180) % 360 - 180) < 1 for d in doors):
            b.piece(K.STONE, (x, y, (WALK + door_h + top) / 2), (thick, seg, top - WALK - door_h), STONE, yaw=a)
        else:
            b.piece(K.STONE, (x, y, (WALK + top) / 2), (thick, seg, top - WALK), STONE, yaw=a)
            if k % 2 == 1:
                ox, oy = cx + (r + 2) * math.cos(math.radians(a)), cy + (r + 2) * math.sin(math.radians(a))
                b.piece(K.DECOR, (ox, oy, WALK + 330), (14, 22, 130), SLIT, yaw=a, vary=0)
    # Arrow slits in the lower body.
    for a in (45, 135, 225, 315):
        ox, oy = cx + (r + 2) * math.cos(math.radians(a)), cy + (r + 2) * math.sin(math.radians(a))
        for z in (320, 600):
            b.piece(K.DECOR, (ox, oy, z), (14, 20, 110), SLIT, yaw=a, vary=0)
    if watch:
        # Taller keep-like tower: a second string course, a closed upper body with windows, a door to the yard.
        b.piece(K.STONE_ROUND, (cx, cy, top + 20), (2 * r + 30, 2 * r + 30, 40), TRIM)
        b.piece(K.STONE_ROUND, (cx, cy, (top + WATCH_TOP) / 2), (2 * r - 20, 2 * r - 20, WATCH_TOP - top), STONE)
        for a in range(0, 360, 45):
            ox, oy = cx + (r - 8) * math.cos(math.radians(a)), cy + (r - 8) * math.sin(math.radians(a))
            b.piece(K.DECOR, (ox, oy, WATCH_TOP - 380), (14, 40, 170), SLIT, yaw=a, vary=0)
        top = WATCH_TOP
        dx, dy = cx, cy + r + 4
        b.box(K.WOOD, dx - 90, dy - 8, 0, dx + 90, dy + 8, 330, WOOD_DARK)
        b.box(K.TRIM, dx - 120, dy - 4, 330, dx + 120, dy + 24, 380, TRIM)
        b.box(K.DECOR, dx - 92, dy + 8, 90, dx + 92, dy + 12, 110, METAL, vary=0)
        b.box(K.DECOR, dx - 92, dy + 8, 240, dx + 92, dy + 12, 260, METAL, vary=0)
        sconce(b, dx + 150, dy + 10, 300)
        banner(b, cx, cy + r + 2, 1500, 200, 600, "y")
    # Eave and slate cone roof.
    b.piece(K.STONE_ROUND, (cx, cy, top + 20), (2 * r + 40, 2 * r + 40, 40), TRIM)
    roof_h = (1300.0 if watch else 1.9 * r)
    b.piece(K.ROOF_CONE, (cx, cy, top + 40 + roof_h / 2), (2 * r + 130, 2 * r + 130, roof_h), SLATE)
    b.piece(K.DECOR_ROUND, (cx, cy, top + 40 + roof_h + 60), (10, 10, 120), METAL, vary=0)
    b.piece(K.DECOR, (cx, cy, top + 40 + roof_h + 125), (24, 24, 24), GOLD, vary=0)


def banner(b, x, y, z_top, width, height, facing_axis):
    """Hangs a banner flat against a face; facing_axis 'y' = the face is perpendicular to Y."""
    if facing_axis == "y":
        b.box(K.CLOTH, x - width / 2, y - 5, z_top - height, x + width / 2, y + 5, z_top, BANNER, vary=0)
        b.box(K.DECOR, x - width / 2 - 15, y - 9, z_top - 10, x + width / 2 + 15, y + 9, z_top + 10, GOLD, vary=0)
        b.box(K.DECOR, x - width * 0.18, y - 8, z_top - height * 0.45 - width * 0.18, x + width * 0.18, y + 8, z_top - height * 0.45 + width * 0.18, GOLD, vary=0)
    else:
        b.box(K.CLOTH, x - 5, y - width / 2, z_top - height, x + 5, y + width / 2, z_top, BANNER, vary=0)
        b.box(K.DECOR, x - 9, y - width / 2 - 15, z_top - 10, x + 9, y + width / 2 + 15, z_top + 10, GOLD, vary=0)
        b.box(K.DECOR, x - 8, y - width * 0.18, z_top - height * 0.45 - width * 0.18, x + 8, y + width * 0.18, z_top - height * 0.45 + width * 0.18, GOLD, vary=0)


def merlons(b, axis, line, a0, a1, z0, depth):
    length = a1 - a0
    if length < 100:
        return
    count = max(1, int((length + 70) // 160))
    pitch = length / count
    for i in range(count):
        c = a0 + pitch * (i + 0.5)
        if axis == "x":
            b.piece(K.STONE, (c, line, z0 + 65), (90, depth, 130), STONE)
        else:
            b.piece(K.STONE, (line, c, z0 + 65), (depth, 90, 130), STONE)


def tower_at(x, y):
    for label, cx, cy, r, _ in TOWERS:
        if abs(cx - x) < 1 and abs(cy - y) < 1:
            return r
    return 0.0


def wall(b, axis, line, a0, a1, out):
    lo, hi = min(a0, a1), max(a0, a1)

    def span(off0, off1, z0, z1, kit, color, trim0=0.0, trim1=0.0):
        p0, p1 = lo + trim0, hi - trim1
        q0, q1 = line + off0, line + off1
        if axis == "x":
            b.box(kit, p0, min(q0, q1), z0, p1, max(q0, q1), z1, color)
        else:
            b.box(kit, min(q0, q1), p0, z0, max(q0, q1), p1, z1, color)

    h = THICK / 2
    span(-h, h, 0, WALK, K.STONE, STONE)
    span(out * h, out * (h + 50), 0, 160, K.STONE, STONE_DARK)
    span(out * h, out * (h + 20), WALK - 40, WALK, K.TRIM, TRIM)
    r0, r1 = tower_at(*((lo, line) if axis == "x" else (line, lo))), tower_at(*((hi, line) if axis == "x" else (line, hi)))
    t0, t1 = max(r0 - 30, 0), max(r1 - 30, 0)
    span(out * h, out * (h - PARAPET), WALK, WALK + 110, K.STONE, STONE, t0, t1)
    merlons(b, axis, line + out * (h - PARAPET / 2), lo + t0, hi - t1, WALK + 110, PARAPET)


def gallery(b, axis, line, s0, s1, inward):
    """Covered timber gallery along the inner face of a wall, from s0 to s1 (along)."""
    face = line + inward * THICK / 2
    post_d, roof_hi, roof_lo, over = GALLERY - 30, 580.0, 395.0, 40.0
    n = max(1, int(round((s1 - s0) / 400.0)))
    for i in range(n + 1):
        s = s0 + (s1 - s0) * i / n
        d = face + inward * post_d
        if axis == "x":
            b.box(K.WOOD, s - 12, d - 12, 0, s + 12, d + 12, roof_lo + 10, WOOD_DARK)
        else:
            b.box(K.WOOD, d - 12, s - 12, 0, d + 12, s + 12, roof_lo + 10, WOOD_DARK)
    d = face + inward * post_d
    if axis == "x":
        b.box(K.WOOD, s0 - 12, d - 13, roof_lo, s1 + 12, d + 13, roof_lo + 28, WOOD)
    else:
        b.box(K.WOOD, d - 13, s0 - 12, roof_lo, d + 13, s1 + 12, roof_lo + 28, WOOD)
    run = GALLERY + over
    drop = roof_hi - roof_lo - over * (roof_hi - roof_lo) / GALLERY
    angle = math.degrees(math.atan2(roof_hi - roof_lo, GALLERY))
    width = math.hypot(run, drop)
    mid_d = face + inward * run / 2
    mid_z = roof_hi - drop / 2 + 8
    if axis == "x":
        b.piece(K.ROOF, ((s0 + s1) / 2, mid_d, mid_z), (s1 - s0 + 60, width, 14), SHINGLE, roll=inward * angle)
    else:
        b.piece(K.ROOF, (mid_d, (s0 + s1) / 2, mid_z), (width, s1 - s0 + 60, 14), SHINGLE, pitch=-inward * angle)


def gate(b):
    y0, y1 = GATE_Y
    for x0, x1 in GATE_TOWERS:
        b.box(K.STONE, x0 - 30, y0 - 30, 0, x1 + 30, y1 + 30, 160, STONE_DARK)
        b.box(K.STONE, x0, y0, 0, x1, y1, WALK, STONE)
        b.box(K.TRIM, x0 - 20, y0 - 20, WALK - 40, x1 + 20, y1 + 20, WALK, TRIM)
        # Room at walk level with doors east and west on the wall line.
        th, door_w, door_h = 60.0, 180.0, 350.0
        b.box(K.STONE, x0, y0, WALK, x1, y0 + th, GATE_TOP, STONE)
        b.box(K.STONE, x0, y1 - th, WALK, x1, y1, GATE_TOP, STONE)
        for x in (x0, x1 - th):
            b.box(K.STONE, x, y0 + th, WALK, x + th, 33800 - door_w / 2, GATE_TOP, STONE)
            b.box(K.STONE, x, 33800 + door_w / 2, WALK, x + th, y1 - th, GATE_TOP, STONE)
            b.box(K.STONE, x, 33800 - door_w / 2, WALK + door_h, x + th, 33800 + door_w / 2, GATE_TOP, STONE)
        b.box(K.STONE, x0 - 10, y0 - 10, GATE_TOP, x1 + 10, y1 + 10, GATE_TOP + 50, STONE)
        merlons(b, "x", y0 + 15, x0 - 10, x1 + 10, GATE_TOP + 50, 50)
        merlons(b, "x", y1 - 15, x0 - 10, x1 + 10, GATE_TOP + 50, 50)
        merlons(b, "y", x0 + 15, y0 + 60, y1 - 60, GATE_TOP + 50, 50)
        merlons(b, "y", x1 - 15, y0 + 60, y1 - 60, GATE_TOP + 50, 50)
        for z in (350, 600, WALK + 330):
            b.box(K.DECOR, (x0 + x1) / 2 - 10, y0 - 3, z - 60, (x0 + x1) / 2 + 10, y0 + 2, z + 60, SLIT, vary=0)
        banner(b, (x0 + x1) / 2, y0 - 8, 1150, 170, 420, "y")
        banner(b, (x0 + x1) / 2, y1 + 8, 1150, 170, 420, "y")
    # Bridge over the passage carries the wall walk from one gate tower to the other.
    g0, g1 = GATE_PASSAGE
    by0, by1, under = BRIDGE
    b.box(K.STONE, g0, by0, under, g1, by1, WALK, STONE)
    b.box(K.TRIM, g0, by0 - 25, under - 50, g1, by0 + 10, under + 10, TRIM)
    b.box(K.STONE, g0, by0, WALK, g1, by0 + PARAPET, WALK + 110, STONE)
    merlons(b, "x", by0 + PARAPET / 2, g0, g1, WALK + 110, PARAPET)
    b.box(K.STONE, g0, by1 - 40, WALK, g1, by1, WALK + 95, STONE)
    # Raised portcullis: its spikes show under the bridge.
    x = g0 + 40
    while x < g1 - 20:
        b.box(K.DECOR, x - 5, by0 + 70, under - 70, x + 5, by0 + 80, under, METAL, vary=0)
        x += 60
    b.box(K.DECOR, g0, by0 + 68, under - 14, g1, by0 + 82, under, METAL, vary=0)
    # Gate leaves stand open against the inside of the gate towers.
    for hinge, sign in ((g0, 1), (g1, -1)):
        xl = hinge + sign * 18
        b.box(K.WOOD, xl - 14, by1, 0, xl + 14, by1 + 480, 620, WOOD_DARK)
        for z in (120, 320, 520):
            b.box(K.DECOR, xl - 18, by1 + 10, z - 12, xl + 18, by1 + 470, z + 12, METAL, vary=0)
    # Road through the passage and torches on both sides.
    b.box(K.COBBLE, g0, GATE_Y[0] - 100, -20, g1, by1, 2, THRESHOLD)
    for x in (g0 - 30, g1 + 30):
        sconce(b, x, GATE_Y[0] - 30, 420)
        sconce(b, x, GATE_Y[1] + 30, 420)


def stair(b, axis, across, bottom, direction, width):
    n = int(WALK / STEP_RISE)
    for i in range(n):
        s0 = bottom + direction * i * STEP_RUN
        s1 = s0 + direction * STEP_RUN
        z1 = (i + 1) * STEP_RISE
        if axis == "y":
            b.box(K.TRIM, across - width / 2, min(s0, s1), 0, across + width / 2, max(s0, s1), z1, TRIM)
        else:
            b.box(K.TRIM, min(s0, s1), across - width / 2, 0, max(s0, s1), across + width / 2, z1, TRIM)
    return bottom, bottom + direction * n * STEP_RUN


def tree(b, x, y, key, small):
    h = (key * 2654435761) & 0xFFFFFFFF
    trunk = (160 if small else 260) + (h % 140)
    crown = (150 if small else 260) + ((h >> 8) % 90)
    b.piece(K.TRUNK, (x, y, trunk / 2), (30 if small else 40, 30 if small else 40, trunk), BARK)
    b.piece(K.LEAVES, (x, y, trunk + crown * 0.3), (crown, crown, crown * 0.9), LEAVES, vary=0.12)
    b.piece(K.LEAVES, (x + crown * 0.2, y - crown * 0.15, trunk + crown * 0.7), (crown * 0.7, crown * 0.7, crown * 0.65), LEAVES, vary=0.12)


def prop(b, x, y, sprite):
    if 16152 <= sprite <= 16159:
        for dx, dy in ((-30, -25), (30, -25), (0, 30)):
            b.piece(K.TRUNK, (x + dx, y + dy, 55), (64, 64, 110), WOOD, vary=0.1)
            b.piece(K.DECOR_ROUND, (x + dx, y + dy, 80), (68, 68, 8), METAL, vary=0)
    elif 16168 <= sprite <= 16171:
        b.box(K.WOOD, x - 60, y - 50, 0, x + 40, y + 50, 100, WOOD)
        b.box(K.WOOD, x - 20, y - 40, 100, x + 55, y + 35, 175, WOOD)
    else:
        b.box(K.WOOD, x - 90, y - 40, 70, x + 90, y + 40, 82, WOOD)
        for dx in (-80, 80):
            for dy in (-30, 30):
                b.box(K.WOOD, x + dx - 5, y + dy - 5, 0, x + dx + 5, y + dy + 5, 70, WOOD_DARK)
        b.box(K.WOOD, x - 80, y - 75, 40, x + 80, y - 55, 48, WOOD_DARK)
        b.box(K.WOOD, x - 80, y + 55, 40, x + 80, y + 75, 48, WOOD_DARK)


def brazier(b, x, y):
    b.piece(K.STONE_ROUND, (x, y, 55), (50, 50, 110), TRIM)
    b.piece(K.DECOR_ROUND, (x, y, 120), (80, 80, 20), METAL, vary=0)
    b.piece(K.GLOW, (x, y, 140), (50, 50, 45), FLAME, vary=0)
    b.light(x, y, 190, 3000.0, 1500.0)


def mosaic(b, x, y):
    for dia, z, color in ((620, 1.0, GOLD), (560, 1.5, BANNER), (300, 2.0, GOLD), (240, 2.5, TRIM), (80, 3.0, GOLD)):
        b.piece(K.DECOR_ROUND, (x, y, z), (dia, dia, 2), color, vary=0)
    for a in range(0, 180, 45):
        b.piece(K.DECOR, (x, y, 2.2), (540, 26, 2), GOLD, yaw=a, vary=0)


def main():
    if not os.path.exists(MAP_FILE):
        raise RuntimeError("build_fortress: map not found: %s" % MAP_FILE)
    tiles = read_map(MAP_FILE)
    ensure_materials()
    unreal.EditorLoadingAndSavingUtils.load_map(WORLD)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    fort = None
    for a in actors.get_all_level_actors():
        if a.get_actor_label() == LABEL:
            fort = a
    if fort is None:
        fort = actors.spawn_actor_from_class(unreal.AstoniaBuilding, unreal.Vector(0, 0, 0))
        fort.set_actor_label(LABEL)
        fort.set_folder_path("Astonia/Buildings")
    fort.set_editor_property("is_spatially_loaded", False)
    fort.clear_pieces()
    b = Builder(fort)

    # Floors.
    for x in range(110, 150):
        for y in range(160, 196):
            if not in_rects(x, y, FLOOR_RECTS):
                continue
            g = tiles.get((x, y), {}).get("g", 0)
            x0, y0 = x * T, y * T
            if 12010 <= g <= 12018:
                b.box(K.DIRT, x0, y0, -20, x0 + T, y0 + T, 0, EARTH, vary=0.08)
            elif 12030 <= g <= 12038:
                b.box(K.DIRT, x0, y0, -20, x0 + T, y0 + T, 0, PATH, vary=0.08)
            elif 12000 <= g <= 12008:
                b.box(K.GRASS, x0, y0, -20, x0 + T, y0 + T, 0, GRASS, vary=0.08)
            elif 30000 <= g <= 30020:
                b.box(K.COBBLE, x0, y0, -20, x0 + T, y0 + T, 0, THRESHOLD)
            else:
                b.box(K.FLAGSTONE, x0, y0, -20, x0 + T, y0 + T, 0, PAVING, vary=0.03)
    mosaic(b, (START[0] + 0.5) * T, (START[1] + 0.5) * T)

    # Walls, towers, gate.
    for label, axis, line, a0, a1, out, _ in WALLS:
        wall(b, axis, line, a0, a1, out)
    for label, cx, cy, r, doors in TOWERS:
        tower(b, label, cx, cy, r, doors)
    gate(b)

    # Stairs, then galleries where the map marks the strip inside a wall as indoors (not where stairs are).
    stair_spans = []
    for label, axis, across, bottom, direction, width in STAIRS:
        s0, s1 = stair(b, axis, across, bottom, direction, width)
        stair_spans.append((axis, across, min(s0, s1) - 100, max(s0, s1) + 100))
    galleries = 0
    for label, axis, line, a0, a1, out, map_line in WALLS:
        inward = -out
        lo, hi = min(a0, a1), max(a0, a1)
        r0 = tower_at(*((lo, line) if axis == "x" else (line, lo)))
        r1 = tower_at(*((hi, line) if axis == "x" else (line, hi)))
        lo, hi = lo + r0 + 20, hi - r1 - 20
        covered = []
        for t in range(int(lo // T), int(hi // T) + 1):
            c = (t + 0.5) * T
            if c < lo or c > hi:
                continue
            strip = [(t, map_line + inward * d) if axis == "x" else (map_line + inward * d, t) for d in (2, 3)]
            if any("MF_INDOORS" in tiles.get(p, {}).get("fl", ()) for p in strip):
                covered.append(t)
        runs = []
        for t in covered:
            if runs and runs[-1][1] == t - 1:
                runs[-1][1] = t
            else:
                runs.append([t, t])
        gallery_across = line + inward * (THICK / 2 + GALLERY / 2)
        for t0, t1 in runs:
            s0, s1 = max(t0 * T, lo), min((t1 + 1) * T, hi)
            pieces = [(s0, s1)]
            for s_axis, s_across, c0, c1 in stair_spans:
                if s_axis != axis or abs(s_across - gallery_across) > GALLERY:
                    continue
                pieces = [p for q0, q1 in pieces for p in ((q0, min(q1, c0)), (max(q0, c1), q1))]
            for q0, q1 in pieces:
                if q1 - q0 >= 2 * T:
                    gallery(b, axis, line, q0, q1, inward)
                    galleries += 1

    # Buttresses on the outer faces where the map has wall decorations outside a wall.
    buttresses = trees = props = lights = 0
    for (x, y), t in tiles.items():
        if not in_rects(x, y, AREA_RECTS):
            continue
        f = t["f"]
        cx, cy = (x + 0.5) * T, (y + 0.5) * T
        if 14052 <= f <= 14055:
            for label, axis, line, a0, a1, out, map_line in WALLS:
                perp, along = (y, cx) if axis == "x" else (x, cy)
                if (perp - map_line) * out not in (1, 2) or not (min(a0, a1) < along < max(a0, a1)):
                    continue
                if any(math.hypot(tx - (along if axis == "x" else line), ty - (line if axis == "x" else along)) < r + 150 for _, tx, ty, r, _ in TOWERS):
                    break
                if axis == "x" and GATE_TOWERS[0][0] - 200 < along < GATE_TOWERS[1][1] + 200:
                    break
                face = line + out * THICK / 2
                for depth, z1, w in ((110, 600, 150), (60, 720, 120)):
                    q0, q1 = sorted((face, face + out * depth))
                    if axis == "x":
                        b.box(K.STONE, along - w / 2, q0, 0, along + w / 2, q1, z1, STONE_DARK if depth > 100 else STONE)
                    else:
                        b.box(K.STONE, q0, along - w / 2, 0, q1, along + w / 2, z1, STONE_DARK if depth > 100 else STONE)
                buttresses += 1
                break
        elif in_masonry(cx, cy):
            pass  # sprites drawn on wall tiles (wall faces, things leaning on walls)
        elif 21000 <= f <= 21499 or 16020 <= f <= 16025:
            tree(b, cx, cy, x * 256 + y, in_rects(x, y, FLOOR_RECTS))
            trees += 1
        elif 16152 <= f <= 16174 and in_rects(x, y, FLOOR_RECTS):
            prop(b, cx, cy, f)
            props += 1
        if t["it"] == "nightlight" and in_rects(x, y, FLOOR_RECTS):
            brazier(b, cx, cy)
            lights += 1

    fort.finish_pieces()
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    unreal.log("build_fortress: %s built: %d pieces (%d galleries, %d buttresses, %d trees, %d props, %d braziers)"
               % (LABEL, fort.get_piece_count(), galleries, buttresses, trees, props, lights))


main()
