"""Builds Cameron's Thieves' Guild as an AstoniaBuilding: a guildhouse against the fortress's east wall
and, under it, the guild's rooms rebuilt from the original cameron.map.

On the map the guild (entry 160,212, 30 rooms of 5x5 tiles, the master's vault at the bottom) lies in the
walled-off wolf forest south-east of the fort, and the 2D game reached it through a hole in the north
village (city_teleport5 at 108,92). Here the whole section is moved GUILD_OFFSET tiles and GUILD_DEPTH
under a guildhouse on the free ground east of the fort (x148-155, y177-185); a stair inside the house
goes down through the guild's north wall into the entry room. From the map: floors (walkable tiles of the
section), walls (blocking tiles round it), doors (left open), wall torches, chests, armchairs. The
guild's thieves, assassins and master come from the zone's spawns, which build_zone.py records at the
moved spot (LAYOUTS["1"]["replaced"]). Forest floor covers the old spot.

Run after build_zone.py (any order with build_fortress.py):
  UnrealEditor-Cmd.exe AstoniaUE5.uproject -run=pythonscript -script="<project>/Scripts/build_thieves_guild.py"
Re-running rebuilds in place.
"""
import math
import os
import sys
from collections import deque
import unreal

sys.path.insert(0, os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Scripts"))
from astonia_kit import *  # noqa: E402,F401,F403  (T, K, colours, read_map, ensure_materials, Builder, sconce)

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
MAP_FILE = os.path.normpath(os.path.join(PROJECT, "../../source_codes/astonia_community_server35-main/zones/1/cameron.map"))
WORLD = "/Game/Maps/L_World"
LABEL = "ThievesGuild_Cameron"

ENTRY = (160, 212)              # original teleport target inside the guild
GUILD_RECT = ((148, 208), (183, 254))
GUILD_OFFSET = (-9, -29)        # keep in step with build_zone.py LAYOUTS["1"]["replaced"]
GUILD_DEPTH = 600.0
ROOM_H = 400.0

# Guildhouse (cm) and the stair from its floor down into the guild's entry room.
HOUSE = (29600, 35400, 31200, 37200)
STAIR_X = (30200, 30400)        # world tile 151 = original x160
STAIR_TOP_Y = 35560
STAIR_BOTTOM_Y = 36400          # world row 182 = original y211, the entry room's north edge
STEP_RISE, STEP_RUN = 25.0, 35.0
OPENING_Y = 35960               # house floor covers the stair beyond this (enough headroom below)

GUILD_STONE = (0.27, 0.25, 0.23)
GUILD_FLOOR = (0.22, 0.20, 0.18)
GUILD_CEILING = (0.17, 0.16, 0.15)
PLASTER = (0.62, 0.57, 0.48)
HOUSE_STONE = (0.38, 0.35, 0.31)
DARK_SHINGLE = (0.16, 0.14, 0.15)
PLANKS = (0.26, 0.17, 0.10)
VELVET = (0.32, 0.05, 0.06)
FOREST_FLOOR = (0.18, 0.25, 0.11)


def walkable(t):
    return t is not None and t["g"] and "MF_MOVEBLOCK" not in t["fl"]


def guild_section(tiles):
    seen = {ENTRY}
    queue = deque([ENTRY])
    while queue:
        x, y = queue.popleft()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            n = (x + dx, y + dy)
            if n not in seen and walkable(tiles.get(n)):
                seen.add(n)
                queue.append(n)
    walls = set()
    for x, y in seen:
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                n = (x + dx, y + dy)
                if n not in seen and n in tiles:
                    walls.add(n)
    return seen, walls


def world_tile(p):
    return p[0] + GUILD_OFFSET[0], p[1] + GUILD_OFFSET[1]


def rows(cells):
    """Merges tiles into horizontal runs: (y, x0, x1) inclusive."""
    out = []
    for y in sorted({c[1] for c in cells}):
        xs = sorted(c[0] for c in cells if c[1] == y)
        start = prev = xs[0]
        for x in xs[1:]:
            if x != prev + 1:
                out.append((y, start, prev))
                start = x
            prev = x
        out.append((y, start, prev))
    return out


def wall_dir(tiles, p, section):
    """Direction (dx, dy) from tile p to a neighbouring wall, preferring north, west, east, south."""
    for d in ((0, -1), (-1, 0), (1, 0), (0, 1)):
        if (p[0] + d[0], p[1] + d[1]) not in section:
            return d
    return (0, -1)


def chest(b, x, y, z, d):
    along_x = d[1] != 0     # wall to the north/south: the chest's long side runs along X
    sx, sy = (110, 65) if along_x else (65, 110)
    cx, cy = x + d[0] * 55, y + d[1] * 55
    b.box(K.WOOD, cx - sx / 2, cy - sy / 2, z, cx + sx / 2, cy + sy / 2, z + 60, WOOD)
    b.box(K.WOOD, cx - sx / 2 - 3, cy - sy / 2 - 3, z + 60, cx + sx / 2 + 3, cy + sy / 2 + 3, z + 78, WOOD_DARK)
    for f in (-0.3, 0.3):
        if along_x:
            b.box(K.DECOR, cx + f * sx - 5, cy - sy / 2 - 4, z, cx + f * sx + 5, cy + sy / 2 + 4, z + 80, METAL, vary=0)
        else:
            b.box(K.DECOR, cx - sx / 2 - 4, cy + f * sy - 5, z, cx + sx / 2 + 4, cy + f * sy + 5, z + 80, METAL, vary=0)
    fx, fy = cx - d[0] * (sx / 2 + 4), cy - d[1] * (sy / 2 + 4)   # lock on the side away from the wall
    b.box(K.DECOR, fx - 10, fy - 10, z + 45, fx + 10, fy + 10, z + 70, GOLD, vary=0)


def armchair(b, x, y, z, d):
    cx, cy = x + d[0] * 40, y + d[1] * 40
    b.box(K.WOOD, cx - 40, cy - 40, z, cx + 40, cy + 40, z + 45, WOOD_DARK)
    b.box(K.CLOTH, cx - 36, cy - 36, z + 45, cx + 36, cy + 36, z + 55, VELVET, vary=0)
    bx, by = cx + d[0] * 35, cy + d[1] * 35
    if d[1] != 0:
        b.box(K.WOOD, cx - 40, by - 8, z, cx + 40, by + 8, z + 120, WOOD_DARK)
        for sx in (-36, 36):
            b.box(K.WOOD, cx + sx - 6, cy - 40, z + 45, cx + sx + 6, cy + 40, z + 75, WOOD_DARK)
    else:
        b.box(K.WOOD, bx - 8, cy - 40, z, bx + 8, cy + 40, z + 120, WOOD_DARK)
        for sy in (-36, 36):
            b.box(K.WOOD, cx - 40, cy + sy - 6, z + 45, cx + 40, cy + sy + 6, z + 75, WOOD_DARK)


def guild(b, tiles):
    section, walls = guild_section(tiles)
    z0 = -GUILD_DEPTH
    # Tiles the stair tunnel passes through (the guild's north wall above the entry room).
    tunnel = {(STAIR_X[0] // T - GUILD_OFFSET[0], y) for y in range(ENTRY[1] - 3, ENTRY[1] - 1)}

    def cm(x, y):
        wx, wy = world_tile((x, y))
        return wx * T, wy * T

    for y, x0, x1 in rows(section):
        ax, ay = cm(x0, y)
        bx, _ = cm(x1 + 1, y)
        b.box(K.FLAGSTONE, ax, ay, z0 - 20, bx, ay + T, z0, GUILD_FLOOR, vary=0.04)
    for y, x0, x1 in rows(walls - tunnel):
        ax, ay = cm(x0, y)
        bx, _ = cm(x1 + 1, y)
        b.box(K.STONE, ax, ay, z0, bx, ay + T, z0 + ROOM_H, GUILD_STONE, vary=0.04)
    for y, x0, x1 in rows((section | walls) - tunnel):
        ax, ay = cm(x0, y)
        bx, _ = cm(x1 + 1, y)
        b.box(K.STONE, ax, ay, z0 + ROOM_H, bx, ay + T, z0 + ROOM_H + 40, GUILD_CEILING, vary=0.03)

    counts = {"doors": 0, "torches": 0, "chests": 0, "chairs": 0}
    for p in section:
        item = tiles[p]["it"]
        if not item:
            continue
        wx, wy = world_tile(p)
        cx, cy = (wx + 0.5) * T, (wy + 0.5) * T
        if "door" in item:
            # Doorway in a wall line: lintel above, the leaf stands open against one jamb.
            along_x = (p[0] - 1, p[1]) not in section and (p[0] + 1, p[1]) not in section
            b.box(K.TRIM, cx - 100, cy - 100, z0 + 260, cx + 100, cy + 100, z0 + ROOM_H, GUILD_STONE)
            if along_x:
                b.box(K.WOOD, cx - 98, cy - 90, z0, cx - 86, cy + 90, z0 + 255, WOOD_DARK)
                b.box(K.TRIM, cx - 100, cy - 100, z0 + 250, cx + 100, cy - 80, z0 + 270, WOOD)
            else:
                b.box(K.WOOD, cx - 90, cy - 98, z0, cx + 90, cy - 86, z0 + 255, WOOD_DARK)
                b.box(K.TRIM, cx - 100, cy - 100, z0 + 250, cx - 80, cy + 100, z0 + 270, WOOD)
            counts["doors"] += 1
        elif "walltorch" in item:
            d = wall_dir(tiles, p, section)
            sconce(b, cx + d[0] * 88, cy + d[1] * 88, z0 + 250)
            counts["torches"] += 1
        elif "chest" in item:
            chest(b, cx, cy, z0, wall_dir(tiles, p, section))
            counts["chests"] += 1
        elif "armchair" in item:
            armchair(b, cx, cy, z0, wall_dir(tiles, p, section))
            counts["chairs"] += 1
    # A rug where the 2D game's teleport dropped you.
    ex, ey = world_tile(ENTRY)
    b.box(K.DECOR, ex * T + 15, ey * T + 15, z0, ex * T + T - 15, ey * T + T - 15, z0 + 2, VELVET, vary=0)
    return len(section), len(walls), counts


def stair(b):
    x0, x1 = STAIR_X
    z0 = -GUILD_DEPTH
    n = int(GUILD_DEPTH / STEP_RISE)
    for i in range(n):
        y = STAIR_TOP_Y + i * STEP_RUN
        b.box(K.TRIM, x0, y, z0, x1, y + STEP_RUN, -(i + 1) * STEP_RISE, TRIM)
    # Tunnel walls, the gap above the entry room's ceiling, and a railing round the opening.
    b.box(K.STONE, x0 - 60, STAIR_TOP_Y, z0, x0, STAIR_BOTTOM_Y, 0, GUILD_STONE)
    b.box(K.STONE, x1, STAIR_TOP_Y, z0, x1 + 60, STAIR_BOTTOM_Y, 0, GUILD_STONE)
    b.box(K.STONE, x0 - 60, STAIR_BOTTOM_Y, z0 + ROOM_H, x1 + 60, STAIR_BOTTOM_Y + 60, -20, GUILD_STONE)
    for x in (x0 - 10, x1 + 10):
        for y in (STAIR_TOP_Y + 20, (STAIR_TOP_Y + OPENING_Y) / 2, OPENING_Y - 10):
            b.box(K.WOOD, x - 6, y - 6, 0, x + 6, y + 6, 100, WOOD_DARK)
        b.box(K.WOOD, x - 7, STAIR_TOP_Y + 14, 92, x + 7, OPENING_Y, 106, WOOD)
    b.box(K.WOOD, x0 - 10, OPENING_Y - 17, 92, x1 + 10, OPENING_Y - 3, 106, WOOD)


def house(b):
    hx0, hy0, hx1, hy1 = HOUSE
    sx0, sx1 = STAIR_X
    # Plank floor with the stair opening.
    b.box(K.WOOD, hx0, hy0, -20, sx0, hy1, 0, PLANKS, vary=0.05)
    b.box(K.WOOD, sx1, hy0, -20, hx1, hy1, 0, PLANKS, vary=0.05)
    b.box(K.WOOD, sx0, hy0, -20, sx1, STAIR_TOP_Y, 0, PLANKS, vary=0.05)
    b.box(K.WOOD, sx0, OPENING_Y, -20, sx1, hy1, 0, PLANKS, vary=0.05)
    th, low, top = 50.0, 380.0, 700.0
    door = (sx0 + 20, sx1 - 20, 260.0)   # the door opens straight onto the stair head
    # Ground storey: stone, door in the north wall facing the path from the fort's gate.
    b.box(K.STONE, hx0, hy0, 0, door[0], hy0 + th, low, HOUSE_STONE)
    b.box(K.STONE, door[1], hy0, 0, hx1, hy0 + th, low, HOUSE_STONE)
    b.box(K.STONE, door[0], hy0, door[2], door[1], hy0 + th, low, HOUSE_STONE)
    b.box(K.STONE, hx0, hy1 - th, 0, hx1, hy1, low, HOUSE_STONE)
    b.box(K.STONE, hx0, hy0 + th, 0, hx0 + th, hy1 - th, low, HOUSE_STONE)
    b.box(K.STONE, hx1 - th, hy0 + th, 0, hx1, hy1 - th, low, HOUSE_STONE)
    b.box(K.TRIM, door[0] - 25, hy0 - 12, door[2], door[1] + 25, hy0 + th + 12, door[2] + 30, WOOD_DARK)
    # Door leaf open against the inside of the wall.
    b.box(K.WOOD, door[0] - 130, hy0 + th, 0, door[0], hy0 + th + 10, door[2] - 5, WOOD_DARK)
    # Shuttered windows.
    for x in (hx0 + 300, hx1 - 300):
        b.box(K.DECOR, x - 50, hy0 - 4, 150, x + 50, hy0 + 2, 260, SLIT, vary=0)
        b.box(K.WOOD, x - 95, hy0 - 10, 145, x - 52, hy0, 265, WOOD)
        b.box(K.WOOD, x + 52, hy0 - 10, 145, x + 95, hy0, 265, WOOD)
    for y in (hy0 + 500, hy1 - 500):
        for x, s in ((hx0, -1), (hx1, 1)):
            b.box(K.DECOR, x + s * 2 - 3, y - 50, 150, x + s * 2 + 3, y + 50, 260, SLIT, vary=0)
    # Upper storey: half-timbering (plaster panels and dark beams), not enterable.
    b.box(K.WOOD, hx0, hy0, low, hx1, hy1, low + 20, WOOD_DARK)
    b.box(K.PLASTER, hx0 + 10, hy0 + 10, low + 20, hx1 - 10, hy1 - 10, top, PLASTER, vary=0.03)
    for x in range(int(hx0), int(hx1) + 1, 200):
        for y in (hy0 + 4, hy1 - 4):
            b.box(K.DECOR, x - 10, y - 10, low + 20, x + 10, y + 10, top, WOOD_DARK, vary=0)
    for y in range(int(hy0), int(hy1) + 1, 200):
        for x in (hx0 + 4, hx1 - 4):
            b.box(K.DECOR, x - 10, y - 10, low + 20, x + 10, y + 10, top, WOOD_DARK, vary=0)
    for z in (low + 160, top - 10):
        b.box(K.DECOR, hx0 - 2, hy0 - 2, z - 10, hx1 + 2, hy0 + 8, z + 10, WOOD_DARK, vary=0)
        b.box(K.DECOR, hx0 - 2, hy1 - 8, z - 10, hx1 + 2, hy1 + 2, z + 10, WOOD_DARK, vary=0)
    # Gable roof, ridge along Y; stepped gable ends.
    over = 60.0
    half = (hx1 - hx0) / 2 + over
    rise = 360.0
    angle = math.degrees(math.atan2(rise, half))
    slope = math.hypot(half, rise)
    mid_x = (hx0 + hx1) / 2
    for s in (-1, 1):
        b.piece(K.ROOF, (mid_x + s * half / 2, (hy0 + hy1) / 2, top + rise / 2 + 8), (slope + 20, hy1 - hy0 + 2 * over, 16),
                DARK_SHINGLE, pitch=-s * angle)
    for k in range(6):
        w = (hx1 - hx0) * (1 - (k + 0.5) / 6)
        z = top + k * rise / 6
        for y0, y1 in ((hy0, hy0 + 30), (hy1 - 30, hy1)):
            b.box(K.PLASTER, mid_x - w / 2, y0, z, mid_x + w / 2, y1, z + rise / 6, PLASTER, vary=0.03)
    b.box(K.STONE, hx1 - 300, hy1 - 360, top - 100, hx1 - 140, hy1 - 200, top + rise + 200, HOUSE_STONE)
    # Hanging sign by the door: a dark board with a dagger.
    sx, sy = door[1] + 160, hy0 - 70
    b.box(K.WOOD, sx - 6, hy0 - 120, 330, sx + 6, hy0, 342, WOOD_DARK)
    b.box(K.CLOTH, sx - 45, sy - 5, 230, sx + 45, sy + 5, 320, VELVET, vary=0)
    b.box(K.DECOR, sx - 4, sy - 8, 245, sx + 4, sy + 8, 305, METAL, vary=0)
    b.box(K.DECOR, sx - 18, sy - 8, 285, sx + 18, sy + 8, 292, METAL, vary=0)
    sconce(b, door[0] - 60, hy0 - 12, 280)
    # Inside: a lantern, crates and a table where the guild's front business happens.
    b.light((hx0 + hx1) / 2 + 300, (hy0 + hy1) / 2 + 300, 330, 2200.0, 1000.0)
    b.box(K.WOOD, hx0 + 80, hy1 - 260, 0, hx0 + 180, hy1 - 160, 100, WOOD)
    b.box(K.WOOD, hx0 + 80, hy1 - 150, 0, hx0 + 180, hy1 - 60, 90, WOOD)
    b.box(K.WOOD, hx0 + 95, hy1 - 240, 100, hx0 + 175, hy1 - 170, 170, WOOD)
    tx, ty = hx1 - 450, hy1 - 450
    b.box(K.WOOD, tx - 90, ty - 50, 72, tx + 90, ty + 50, 82, WOOD)
    for dx in (-80, 80):
        for dy in (-40, 40):
            b.box(K.WOOD, tx + dx - 5, ty + dy - 5, 0, tx + dx + 5, ty + dy + 5, 72, WOOD_DARK)
    b.box(K.WOOD, tx - 20, ty - 20, 82, tx + 20, ty + 20, 92, METAL)
    b.box(K.WOOD, hx0 + 10, hy0 + th, low - 20, hx1 - 10, hy1 - th, low, WOOD_DARK)


def old_spot(b):
    """Forest floor and trees where the guild was on the map (it is not reachable, but visible)."""
    (x0, y0), (x1, y1) = GUILD_RECT
    b.box(K.FOREST, x0 * T, y0 * T, -20, (x1 + 1) * T, (y1 + 1) * T, 0, FOREST_FLOOR, vary=0)
    trees = 0
    for x in range(x0, x1 + 1):
        for y in range(y0, y1 + 1):
            h = ((x * 73856093) ^ (y * 19349663)) & 0xFFFFFFFF
            if h % 9:
                continue
            cx, cy = (x + 0.5) * T + (h >> 8) % 80 - 40, (y + 0.5) * T + (h >> 16) % 80 - 40
            trunk = 280 + (h >> 4) % 160
            crown = 260 + (h >> 12) % 120
            b.piece(K.TRUNK, (cx, cy, trunk / 2), (40, 40, trunk), BARK)
            b.piece(K.LEAVES, (cx, cy, trunk + crown * 0.3), (crown, crown, crown * 0.9), LEAVES, vary=0.12)
            b.piece(K.LEAVES, (cx + crown * 0.2, cy - crown * 0.15, trunk + crown * 0.7), (crown * 0.7, crown * 0.7, crown * 0.65), LEAVES, vary=0.12)
            trees += 1
    return trees


def underground_exposure(actors, section):
    """A post-process volume over the guild that fixes the exposure, so the torch-lit rooms stay dim
    instead of auto exposure brightening them to daylight."""
    label = LABEL + "_Exposure"
    for a in actors.get_all_level_actors():
        if a.get_actor_label() == label:
            actors.destroy_actor(a)
    xs = [world_tile(p)[0] for p in section]
    ys = [world_tile(p)[1] for p in section]
    x0, x1 = (min(xs) - 1) * T, (max(xs) + 2) * T
    y0, y1 = STAIR_TOP_Y + 200, (max(ys) + 2) * T
    z0, z1 = -GUILD_DEPTH - 100, -80.0
    vol = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2))
    vol.set_actor_label(label)
    vol.set_folder_path("Astonia/Buildings")
    vol.set_editor_property("is_spatially_loaded", False)
    # The default brush is a 200 cm cube.
    vol.set_actor_scale3d(unreal.Vector((x1 - x0) / 200.0, (y1 - y0) / 200.0, (z1 - z0) / 200.0))
    vol.set_editor_property("unbound", False)
    vol.set_editor_property("blend_radius", 300.0)
    s = vol.get_editor_property("settings")
    s.set_editor_property("override_auto_exposure_min_brightness", True)
    s.set_editor_property("auto_exposure_min_brightness", -1.0)
    s.set_editor_property("override_auto_exposure_max_brightness", True)
    s.set_editor_property("auto_exposure_max_brightness", -1.0)
    vol.set_editor_property("settings", s)


def main():
    if not os.path.exists(MAP_FILE):
        raise RuntimeError("build_thieves_guild: map not found: %s" % MAP_FILE)
    tiles = read_map(MAP_FILE)
    ensure_materials()
    unreal.EditorLoadingAndSavingUtils.load_map(WORLD)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actor = None
    for a in actors.get_all_level_actors():
        if a.get_actor_label() == LABEL:
            actor = a
    if actor is None:
        actor = actors.spawn_actor_from_class(unreal.AstoniaBuilding, unreal.Vector(0, 0, 0))
        actor.set_actor_label(LABEL)
        actor.set_folder_path("Astonia/Buildings")
    actor.set_editor_property("is_spatially_loaded", False)
    actor.clear_pieces()
    b = Builder(actor)
    floors, walls, counts = guild(b, tiles)
    stair(b)
    house(b)
    trees = old_spot(b)
    actor.finish_pieces()
    underground_exposure(actors, guild_section(tiles)[0])
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    unreal.log("build_thieves_guild: %s built: %d pieces, guild %d floor / %d wall tiles, %s, %d trees on the old spot"
               % (LABEL, actor.get_piece_count(), floors, walls, counts, trees))


main()
