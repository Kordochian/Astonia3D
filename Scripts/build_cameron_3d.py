"""Builds the whole surface of Cameron (area 1) as textured 3D placeholder architecture from the original
cameron.map: an AstoniaBuilding "Surface_Cameron" that replaces the zone actor's tile blocks.

From each tile of the map:
- ground sprite -> textured floor (grass, dirt, cobbled roads, forest floor, planks, tiles, flagstones);
- blocking foreground sprite -> walls (stone, dark stone, plaster, timber by sprite family; full height
  next to rooms, lower on their own), trees (forest edges get a blocker so they still close the forest),
  bushes, boulders, or furniture/crates; non-blocking grass tufts and flowers -> small plants;
- items -> doors (open, with lintels), wall torches and lamp posts with lights, chests, chairs, signs,
  flowers, area exits (marked with a glowing ring);
- rooms (MF_INDOORS tiles with a floor sprite) -> gable roofs over their walls, split into rectangles.
Hand-built parts (the fortress, the Thieves' Guild; the zone's other replaced regions) are left out, holes
stay open where the zone's lift shafts come up, and the map spots of the sections the zone moved
underground get forest floor. NPCs and the underground levels stay with the zone actor.

Run after build_zone.py (and import_textures.py once):
  UnrealEditor-Cmd.exe AstoniaUE5.uproject -run=pythonscript -script="<project>/Scripts/build_cameron_3d.py"
"""
import math
import os
import sys
from collections import deque
import unreal

sys.path.insert(0, os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Scripts"))
from astonia_kit import *  # noqa: E402,F401,F403

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
MAP_FILE = os.path.normpath(os.path.join(PROJECT, "../../source_codes/astonia_community_server35-main/zones/1/cameron.map"))
WORLD = "/Game/Maps/L_World"
LABEL = "Surface_Cameron"
ZONE_LABEL = "Zone_01_Cameron"
SIZE = 256

WALL_H = 400.0       # walls round rooms
LOW_WALL_H = 300.0   # walls on their own (garden walls, ruins)
FOREST_H = 650.0     # blocker height along forest edges

ROAD = (0.42, 0.40, 0.37)
TILE_FLOOR = (0.52, 0.49, 0.44)
PLANK_FLOOR = (0.34, 0.23, 0.14)
FOREST_FLOOR = (0.17, 0.22, 0.10)
DUNGEON_FLOOR = (0.36, 0.34, 0.31)
ROOF_COLORS = [(0.42, 0.18, 0.11), (0.36, 0.22, 0.14), (0.30, 0.30, 0.33), (0.45, 0.24, 0.12), (0.26, 0.20, 0.17)]
FLOWER_COLORS = [(0.80, 0.15, 0.15), (0.85, 0.75, 0.15), (0.55, 0.25, 0.75), (0.90, 0.90, 0.85), (0.25, 0.40, 0.85)]


def fam_in(v, *ranges):
    return any(lo <= v <= hi for lo, hi in ranges)


def ground_kind(t):
    g = t["g"]
    indoor = "MF_INDOORS" in t["fl"]
    if fam_in(g, (12000, 12009)):
        return K.GRASS, GRASS, False
    if fam_in(g, (12010, 12019)):
        return K.DIRT, EARTH, False
    if fam_in(g, (12030, 12039), (30000, 30069)):
        return K.COBBLE, ROAD, False
    if fam_in(g, (59740, 59769)):
        return K.FOREST, FOREST_FLOOR, False
    if fam_in(g, (12020, 12029), (12120, 12159), (14060, 14069)):
        return K.TILES, TILE_FLOOR, True
    if fam_in(g, (12100, 12109), (20220, 20249)):
        return K.PLANKS, PLANK_FLOOR, True
    if fam_in(g, (59100, 59139), (20290, 20339), (21220, 21239)):
        return K.FLAGSTONE, DUNGEON_FLOOR, True
    if g == 0:
        return K.DIRT, EARTH, False
    return (K.FLAGSTONE, DUNGEON_FLOOR, True) if indoor else (K.GRASS, GRASS, False)


def wall_style(f):
    if fam_in(f, (14000, 14029)):
        return K.PLASTER, (0.60, 0.56, 0.48)
    if fam_in(f, (14030, 14059), (16130, 16399)):
        return K.STONE, STONE
    if fam_in(f, (14060, 14089)):
        return K.STONE, (0.56, 0.53, 0.48)
    if fam_in(f, (14090, 14199)):
        return K.STONE, (0.42, 0.40, 0.38)
    if fam_in(f, (59150, 59179)):
        return K.STONE, (0.38, 0.37, 0.36)
    if fam_in(f, (21000, 21009)):
        return K.TIMBER, (0.40, 0.30, 0.20)
    return None


def object_kind(t):
    """What a blocking tile becomes: wall, forest, tree, bush, rock or prop."""
    f = t["f"]
    if wall_style(f):
        return "wall"
    if fam_in(f, (16020, 16029)):
        return "forest"
    if fam_in(f, (21410, 21429)):
        return "tree"
    if fam_in(f, (16000, 16009), (21010, 21019)):
        return "bush"
    if f == 0 or fam_in(f, (59760, 59779)):
        return "rock"
    return "prop"


def hval(x, y, salt=0):
    return ((x * 73856093) ^ (y * 19349663) ^ (salt * 83492791)) & 0xFFFFFFFF


def runs(cells):
    """Horizontal runs (y, x0, x1) of a set of tiles."""
    out = []
    by_row = {}
    for x, y in cells:
        by_row.setdefault(y, []).append(x)
    for y, xs in sorted(by_row.items()):
        xs.sort()
        start = prev = xs[0]
        for x in xs[1:]:
            if x != prev + 1:
                out.append((y, start, prev))
                start = x
            prev = x
        out.append((y, start, prev))
    return out


def rectangles(cells):
    """Splits a tile set into rectangles (x0, y0, x1, y1), inclusive, largest first, so big rooms get
    one roof instead of strips."""
    left = set(cells)
    out = []
    while left:
        xs = [c[0] for c in left]
        ys = [c[1] for c in left]
        x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
        heights = [0] * (x1 - x0 + 1)
        best = (0, None)
        for y in range(y0, y1 + 1):
            for i in range(len(heights)):
                heights[i] = heights[i] + 1 if (x0 + i, y) in left else 0
            stack = []   # largest rectangle in a histogram
            for i in range(len(heights) + 1):
                hgt = heights[i] if i < len(heights) else 0
                start = i
                while stack and stack[-1][1] >= hgt:
                    start, sh = stack.pop()
                    area = sh * (i - start)
                    if area > best[0]:
                        best = (area, (x0 + start, y - sh + 1, x0 + i - 1, y))
                stack.append((start, hgt))
        r = best[1]
        for i in range(r[0], r[2] + 1):
            for j in range(r[1], r[3] + 1):
                left.discard((i, j))
        out.append(r)
    return out


class Surface:
    def __init__(self, tiles, zone):
        self.t = tiles
        self.excluded = set()
        self.holes = set()
        self.placed_sections = set()
        regions = zone.get_editor_property("replaced_regions")
        rects = []
        ground_rects = []
        for r in regions:
            lo, hi = r.get_editor_property("min"), r.get_editor_property("max")
            if lo.x <= 0 and lo.y <= 0 and hi.x >= SIZE - 1 and hi.y >= SIZE - 1:
                continue   # the region that hands the whole surface to this script
            (ground_rects if r.get_editor_property("keep_ground") else rects).append(((lo.x, lo.y), (hi.x, hi.y)))
        # Regions whose structure keeps the ground: this script lays the ground there, nothing else.
        self.ground_only = set()
        for x in range(SIZE):
            for y in range(SIZE):
                if in_rects(x, y, rects):
                    self.excluded.add((x, y))
                elif in_rects(x, y, ground_rects):
                    self.ground_only.add((x, y))
        for s in zone.get_editor_property("shafts"):
            top = s.get_editor_property("top")
            if abs(top.z) < 1:
                self.holes.add((int(top.x // T), int(top.y // T)))
        self.entries = [(p.get_editor_property("entry_tile").x, p.get_editor_property("entry_tile").y) for p in zone.get_editor_property("placements")]
        self.surface = self.split_levels()

    def at(self, p):
        return self.t.get(p)

    def blocked(self, p):
        t = self.at(p)
        return t is not None and "MF_MOVEBLOCK" in t["fl"]

    def walkable(self, p):
        t = self.at(p)
        return t is not None and t["g"] and "MF_MOVEBLOCK" not in t["fl"]

    def split_levels(self):
        """Tiles that belong to the surface: walkable sections and the tiles nearest to them, minus the
        sections the zone moved underground (like AstoniaZoneActor's level assignment)."""
        section = {}
        n = 0
        for start in self.t:
            if start in section or not self.walkable(start):
                continue
            queue = deque([start])
            section[start] = n
            while queue:
                x, y = queue.popleft()
                for d in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    q = (x + d[0], y + d[1])
                    if q not in section and self.walkable(q):
                        section[q] = n
                        queue.append(q)
            n += 1
        placed = {section[e] for e in self.entries if e in section}
        self.placed_sections = placed
        near = {p: {s} for p, s in section.items()}
        dist = {p: 0 for p in section}
        queue = deque(section)
        while queue:
            x, y = queue.popleft()
            for dx in (-1, 0, 1):
                for dy in (-1, 0, 1):
                    q = (x + dx, y + dy)
                    if q not in self.t:
                        continue
                    if q not in dist:
                        dist[q] = dist[(x, y)] + 1
                        near[q] = set(near[(x, y)])
                        queue.append(q)
                    elif dist[q] == dist[(x, y)] + 1:
                        near[q] |= near[(x, y)]
        surface = set()
        self.footprint = set()
        for p in self.t:
            if p in self.excluded:
                continue
            ns = near.get(p)
            if not ns or ns - placed:
                surface.add(p)
            else:
                self.footprint.add(p)
        return surface


def build_ground(b, s):
    by_kind = {}
    for p in s.surface | s.ground_only:
        if p in s.holes:
            continue
        kit, color, _ = ground_kind(s.at(p))
        by_kind.setdefault((kit, color), set()).add(p)
    for p in s.footprint:
        by_kind.setdefault((K.FOREST, FOREST_FLOOR), set()).add(p)
    count = 0
    for (kit, color), cells in by_kind.items():
        for y, x0, x1 in runs(cells):
            b.box(kit, x0 * T, y * T, -20, (x1 + 1) * T, (y + 1) * T, 0, color, vary=0.04)
            count += 1
    return count


def tree(b, x, y, h, big=False):
    kit_tree(b, x, y, h, "forest" if big else "tree")


def bush(b, x, y, h):
    kit_tree(b, x, y, h, "bush")
    b.box(K.BLOCKER, x - 80, y - 80, 0, x + 80, y + 80, 120, LEAVES, vary=0)


def rock(b, x, y, h, small=False):
    r = (90 if small else 150) + h % 70
    b.piece(K.ROCK, (x, y, r * 0.25), (r * 1.4, r * 1.1, r * 0.9), (0.24, 0.23, 0.21), yaw=h % 180, vary=0.1)
    b.piece(K.ROCK, (x + r * 0.35, y - r * 0.2, r * 0.15), (r * 0.7, r * 0.6, r * 0.5), (0.22, 0.21, 0.19), yaw=(h >> 4) % 180, vary=0.1)


def prop(b, x, y, h):
    kind = h % 3
    if kind == 0:   # crates
        b.box(K.WOOD, x - 60, y - 50, 0, x + 40, y + 50, 100, WOOD)
        b.box(K.WOOD, x - 20, y - 40, 100, x + 55, y + 35, 170, WOOD)
    elif kind == 1:   # barrels
        for dx, dy in ((-30, -25), (30, -25), (0, 30)):
            b.piece(K.TRUNK, (x + dx, y + dy, 55), (64, 64, 110), WOOD, vary=0.1)
            b.piece(K.DECOR_ROUND, (x + dx, y + dy, 80), (68, 68, 8), METAL, vary=0)
    else:   # shelf / cupboard
        b.box(K.WOOD, x - 80, y - 35, 0, x + 80, y + 35, 190, WOOD_DARK)
        for z in (60, 120):
            b.box(K.DECOR, x - 76, y - 38, z, x + 76, y + 38, z + 6, WOOD, vary=0)


def plant(b, x, y, h, color=None):
    for i in range(3):
        a = (h >> (i * 4)) % 360
        px, py = x + 35 * math.cos(math.radians(a)), y + 35 * math.sin(math.radians(a))
        b.piece(K.LEAVES, (px, py, 18), (55, 55, 40), LEAVES, vary=0.2)
        if color:
            b.piece(K.DECOR_ROUND, (px, py, 38), (14, 14, 6), color, vary=0)


def build_objects(b, s):
    counts = {"wall tiles": 0, "trees": 0, "forest": 0, "bushes": 0, "rocks": 0, "props": 0, "plants": 0}
    floors = {p for p in s.surface - s.ground_only if not s.blocked(p) and "MF_INDOORS" in s.at(p)["fl"] and ground_kind(s.at(p))[2]}
    walls = {}
    forest = set()
    for p in s.surface - s.ground_only:
        t = s.at(p)
        x, y = p
        cx, cy = (x + 0.5) * T, (y + 0.5) * T
        h = hval(x, y)
        if not s.blocked(p):
            if fam_in(t["f"], (20820, 20849)):
                plant(b, cx, cy, h, FLOWER_COLORS[h % len(FLOWER_COLORS)] if h % 4 == 0 else None)
                counts["plants"] += 1
            continue
        kind = object_kind(t)
        if kind == "wall":
            style = wall_style(t["f"])
            room = any((x + dx, y + dy) in floors for dx in (-1, 0, 1) for dy in (-1, 0, 1))
            walls.setdefault((style, WALL_H if room else LOW_WALL_H), set()).add(p)
        elif kind == "forest":
            tree(b, cx, cy, h, big=True)
            forest.add(p)
            counts["forest"] += 1
        elif kind == "tree":
            tree(b, cx, cy, h)
            counts["trees"] += 1
        elif kind == "bush":
            bush(b, cx, cy, h)
            counts["bushes"] += 1
        elif kind == "rock":
            rock(b, cx, cy, h, small=t["f"] == 0)
            counts["rocks"] += 1
        else:
            prop(b, cx, cy, h)
            counts["props"] += 1
    # Modular walls (Blender kit), 0.5 m thick on the tile's centre line: a straight piece where the
    # wall runs on through the tile, else a post with half-length arms towards each connected side.
    # Door tiles count as wall for this, so walls run straight up to the door frames. Collision follows
    # the thin wall (blockers 0.6 m wide).
    every = set()
    for cells in walls.values():
        every |= cells
    walls_all = set(every)
    every |= {p for p in s.surface - s.ground_only if "door" in s.at(p)["it"] and "teleport" not in s.at(p)["it"]}
    half = 30.0
    for ((kit, color), height), cells in walls.items():
        straight, window, post = WALL_PIECES[kit]
        zs = height / 400.0 * 100.0
        for (x, y) in cells:
            e, w = (x + 1, y) in every, (x - 1, y) in every
            n, so = (x, y - 1) in every, (x, y + 1) in every
            cx, cy = (x + 0.5) * T, (y + 0.5) * T
            X0, Y0, X1, Y1 = x * T, y * T, (x + 1) * T, (y + 1) * T
            # Walls the map draws two or more tiles thick stay solid: a full-tile block.
            solid = any(all((x + i, y + j) in walls_all for i in (0, dx) for j in (0, dy)) for dx in (-1, 1) for dy in (-1, 1))
            if solid:
                b.piece(straight, (cx, cy, 0), (100, 100 * T / (2 * half), zs), color, vary=0.04)
                b.box(K.BLOCKER, X0, Y0, 0, X1, Y1, height, color, vary=0)
            elif e and w and not (n or so):
                piece = window if height >= WALL_H and (x * 7 + y * 3) % 4 == 0 else straight
                b.piece(piece, (cx, cy, 0), (100, 100, zs), color, vary=0.04)
                b.box(K.BLOCKER, X0, cy - half, 0, X1, cy + half, height, color, vary=0)
            elif n and so and not (e or w):
                piece = window if height >= WALL_H and (x * 7 + y * 3) % 4 == 0 else straight
                b.piece(piece, (cx, cy, 0), (100, 100, zs), color, yaw=90.0, vary=0.04)
                b.box(K.BLOCKER, cx - half, Y0, 0, cx + half, Y1, height, color, vary=0)
            else:
                b.piece(post, (cx, cy, 0), (100, 100, zs), color, vary=0.04)
                b.box(K.BLOCKER, cx - 36, cy - 36, 0, cx + 36, cy + 36, height, color, vary=0)
                for on, dx, dy in ((e, 1, 0), (w, -1, 0), (so, 0, 1), (n, 0, -1)):
                    if not on:
                        continue
                    b.piece(straight, (cx + dx * 50, cy + dy * 50, 0), (50, 100, zs), color, yaw=0.0 if dx else 90.0, vary=0.04)
                    if dx:
                        b.box(K.BLOCKER, min(cx, cx + dx * 100), cy - half, 0, max(cx, cx + dx * 100), cy + half, height, color, vary=0)
                    else:
                        b.box(K.BLOCKER, cx - half, min(cy, cy + dy * 100), 0, cx + half, max(cy, cy + dy * 100), height, color, vary=0)
        counts["wall tiles"] += len(cells)
    for y, x0, x1 in runs(forest):
        b.box(K.BLOCKER, x0 * T, y * T, 0, (x1 + 1) * T, (y + 1) * T, FOREST_H, LEAVES, vary=0)
    return floors, walls, counts


def gable(b, x0, y0, x1, y1, base, color):
    """Gable roof over a rectangle of tiles from the Blender kit: shingle panels down both slopes,
    ridge tiles along the top and a plaster gable at each end."""
    X0, Y0, X1, Y1 = x0 * T, y0 * T, (x1 + 1) * T, (y1 + 1) * T
    over = 50.0
    along_x = (X1 - X0) >= (Y1 - Y0)
    short = (Y1 - Y0) if along_x else (X1 - X0)
    half = short / 2 + over
    rise = min(short * 0.42, 520.0)
    angle = math.atan2(rise, half)
    slope = math.hypot(half, rise) + 12
    mx, my = (X0 + X1) / 2, (Y0 + Y1) / 2
    ridge_z = base + rise + 6
    length = (X1 - X0 if along_x else Y1 - Y0) + 2 * over
    start = (X0 if along_x else Y0) - over
    n = max(1, int(math.ceil(length / 200.0)))
    seg_len = length / n
    m = max(1, int(math.ceil(slope / 200.0)))
    seg = slope / m
    ridge_dir = unreal.Vector(1, 0, 0) if along_x else unreal.Vector(0, 1, 0)
    for sgn in (-1, 1):
        across = unreal.Vector(0, sgn, 0) if along_x else unreal.Vector(sgn, 0, 0)
        down = unreal.Vector(across.x * math.cos(angle), across.y * math.cos(angle), -math.sin(angle))
        # The imported panel runs along local -Y down the slope: local +Y = up the slope.
        up_slope = unreal.Vector(-down.x, -down.y, -down.z)
        x_axis = ridge_dir
        if x_axis.x * up_slope.y - x_axis.y * up_slope.x < 0:   # keep the panel's face (X cross Y) pointing up
            x_axis = unreal.Vector(-ridge_dir.x, -ridge_dir.y, 0)
        rot = unreal.MathLibrary.make_rot_from_xy(x_axis, up_slope)
        for i in range(n):
            a = start + (i + 0.5) * seg_len
            for j in range(m):
                d = j * seg
                px = (a if along_x else mx) + down.x * d
                py = (my if along_x else a) + down.y * d
                pz = ridge_z + down.z * d
                b.piece(K.ROOF_PANEL, (px, py, pz), (seg_len / 2.0, seg / 2.0, 100), color, yaw=rot.yaw, pitch=rot.pitch, roll=rot.roll, vary=0.04)
    for i in range(n):
        a = start + (i + 0.5) * seg_len
        b.piece(K.ROOF_RIDGE, ((a if along_x else mx), (my if along_x else a), ridge_z - 4), (seg_len / 2.0, 100, 100), color,
                yaw=0.0 if along_x else 90.0, vary=0.03)
    for e in ((X0 + 14, X1 - 14) if along_x else (Y0 + 14, Y1 - 14)):
        b.piece(K.GABLE, ((e if along_x else mx), (my if along_x else e), base), (short, 100, rise), (0.60, 0.56, 0.48),
                yaw=90.0 if along_x else 0.0, vary=0.02)


WALL_PIECES = {
    K.STONE: (K.STONE_WALL, K.STONE_WINDOW, K.STONE_POST),
    K.PLASTER: (K.PLASTER_WALL, K.PLASTER_WINDOW, K.PLASTER_POST),
    K.TIMBER: (K.TIMBER_WALL, K.TIMBER_WINDOW, K.TIMBER_POST),
}


def build_roofs(b, s, floors, walls):
    wall_cells = set()
    for (_, height), cells in walls.items():
        if height >= WALL_H:
            wall_cells |= cells
    cells = floors | {p for p in wall_cells if any((p[0] + dx, p[1] + dy) in floors for dx in (-1, 0, 1) for dy in (-1, 0, 1))}
    cells -= s.holes
    seen = set()
    buildings = roofs = 0
    for start in sorted(cells):
        if start in seen:
            continue
        comp = set()
        queue = deque([start])
        seen.add(start)
        while queue:
            x, y = queue.popleft()
            comp.add((x, y))
            for d in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                q = (x + d[0], y + d[1])
                if q in cells and q not in seen:
                    seen.add(q)
                    queue.append(q)
        if len(comp) < 4:
            continue
        color = ROOF_COLORS[hval(*start) % len(ROOF_COLORS)]
        for r in rectangles(comp):
            if (r[2] - r[0] + 1) * (r[3] - r[1] + 1) < 2:
                continue
            gable(b, *r, WALL_H, color)
            roofs += 1
        buildings += 1
    return buildings, roofs


def door(b, s, p, style, item):
    x, y = p
    along_x = s.blocked((x - 1, y)) and s.blocked((x + 1, y))
    kit_door(b, (x + 0.5) * T, (y + 0.5) * T, 0.0, along_x, item, style[0], style[1], WALL_H, hval(x, y, 11))


def lamp_post(b, x, y):
    b.piece(K.TRUNK, (x, y, 140), (16, 16, 280), METAL)
    b.box(K.DECOR, x - 22, y - 22, 280, x + 22, y + 22, 290, METAL, vary=0)
    b.piece(K.GLOW, (x, y, 305), (30, 30, 34), FLAME, vary=0)
    b.box(K.DECOR, x - 24, y - 24, 325, x + 24, y + 24, 335, METAL, vary=0)
    b.light(x, y, 300, 2200.0, 1000.0)


def build_items(b, s):
    counts = {"doors": 0, "torches": 0, "lamps": 0, "chests": 0, "chairs": 0, "signs": 0, "flowers": 0, "exits": 0}
    for p in s.surface - s.ground_only:
        t = s.at(p)
        it = t["it"]
        if not it:
            continue
        x, y = p
        cx, cy = (x + 0.5) * T, (y + 0.5) * T
        h = hval(x, y, 7)
        if "door" in it and "teleport" not in it:
            neighbour = next((s.at(q) for q in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)) if s.blocked(q)), None)
            style = wall_style(neighbour["f"]) if neighbour else None
            door(b, s, p, style or (K.STONE, STONE), it)
            counts["doors"] += 1
        elif "walltorch" in it:
            if s.blocked(p):
                d = next((d for d in ((0, 1), (1, 0), (0, -1), (-1, 0)) if s.walkable((x + d[0], y + d[1]))), (0, 1))
                sconce(b, cx + d[0] * 37, cy + d[1] * 37, 250)    # on the face of this (0.5 m) wall
            else:
                d = next((d for d in ((0, -1), (-1, 0), (1, 0), (0, 1)) if s.blocked((x + d[0], y + d[1]))), (0, -1))
                sconce(b, cx + d[0] * 163, cy + d[1] * 163, 250)  # on the face of the next tile's wall
            counts["torches"] += 1
        elif "light" in it:
            lamp_post(b, cx, cy)
            counts["lamps"] += 1
        elif "chest" in it or "depot" in it:
            b.box(K.WOOD, cx - 55, cy - 33, 0, cx + 55, cy + 33, 60, WOOD)
            b.box(K.WOOD, cx - 58, cy - 36, 60, cx + 58, cy + 36, 78, WOOD_DARK)
            b.box(K.DECOR, cx - 10, cy - 40, 45, cx + 10, cy - 34, 70, GOLD, vary=0)
            counts["chests"] += 1
        elif "chair" in it:
            b.box(K.WOOD, cx - 30, cy - 30, 0, cx + 30, cy + 30, 45, WOOD)
            b.box(K.WOOD, cx - 30, cy - 34, 45, cx + 30, cy - 24, 110, WOOD_DARK)
            counts["chairs"] += 1
        elif "sign" in it:
            b.piece(K.TRUNK, (cx, cy, 110), (14, 14, 220), WOOD_DARK)
            b.box(K.WOOD, cx - 60, cy - 6, 150, cx + 60, cy + 6, 220, WOOD)
            counts["signs"] += 1
        elif ("flower" in it or "berry" in it or "mushroom" in it):
            color = (0.70, 0.10, 0.12) if "berry" in it else (0.80, 0.70, 0.60) if "mushroom" in it else FLOWER_COLORS[h % len(FLOWER_COLORS)]
            plant(b, cx, cy, h, color)
            counts["flowers"] += 1
        elif "city_exit" in it:
            b.piece(K.TRIM, (cx, cy, 5), (190, 190, 10), TRIM)
            b.piece(K.DECOR_ROUND, (cx, cy, 11), (150, 150, 2), (0.15, 0.35, 0.90), vary=0)
            counts["exits"] += 1
    return counts


def main():
    tiles = read_map(MAP_FILE)
    unreal.EditorLoadingAndSavingUtils.load_map(WORLD)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    zone = next((a for a in actors.get_all_level_actors() if a.get_actor_label() == ZONE_LABEL), None)
    if zone is None:
        raise RuntimeError("build_cameron_3d: run build_zone.py first (no %s)" % ZONE_LABEL)
    actor = next((a for a in actors.get_all_level_actors() if a.get_actor_label() == LABEL), None)
    if actor is None:
        actor = actors.spawn_actor_from_class(unreal.AstoniaBuilding, unreal.Vector(0, 0, 0))
        actor.set_actor_label(LABEL)
        actor.set_folder_path("Astonia/Buildings")
    actor.set_editor_property("is_spatially_loaded", False)
    actor.clear_pieces()
    b = Builder(actor)
    s = Surface(tiles, zone)
    ground = build_ground(b, s)
    floors, walls, objects = build_objects(b, s)
    buildings, roofs = build_roofs(b, s, floors, walls)
    items = build_items(b, s)
    actor.finish_pieces()
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    unreal.log("build_cameron_3d: %s built: %d pieces; %d surface tiles (%d excluded, %d underground footprint, %d shaft holes); "
               "%d ground runs; %s; %d buildings with %d roofs; %s"
               % (LABEL, actor.get_piece_count(), len(s.surface), len(s.excluded), len(s.footprint), len(s.holes),
                  ground, objects, buildings, roofs, items))


main()
