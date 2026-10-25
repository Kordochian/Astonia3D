"""Shared pieces for scripts that build AstoniaBuilding structures (build_fortress.py,
build_thieves_guild.py): the original map reader, kit materials, the piece builder and colours.
Import from a script run by the pythonscript commandlet (the Scripts folder is put on sys.path).
"""
import math
import unreal

T = 200.0          # tile edge, cm
MAT_DIR = "/Game/Astonia/Materials"

K = unreal.AstoniaKit

# Linear colours.
STONE = (0.50, 0.48, 0.44)
STONE_DARK = (0.33, 0.32, 0.30)
TRIM = (0.62, 0.60, 0.55)
SLATE = (0.10, 0.16, 0.30)
WOOD = (0.30, 0.19, 0.10)
WOOD_DARK = (0.18, 0.11, 0.06)
SHINGLE = (0.34, 0.15, 0.09)
BANNER = (0.06, 0.13, 0.42)
GOLD = (0.75, 0.52, 0.12)
METAL = (0.12, 0.12, 0.13)
SLIT = (0.04, 0.04, 0.05)
PAVING = (0.47, 0.45, 0.41)
THRESHOLD = (0.40, 0.38, 0.35)
EARTH = (0.30, 0.24, 0.16)
PATH = (0.40, 0.33, 0.23)
GRASS = (0.19, 0.27, 0.12)
LEAVES = (0.13, 0.29, 0.08)
BARK = (0.22, 0.15, 0.09)
FLAME = (1.0, 0.55, 0.15)

# --- Original map ---
def read_map(path):
    tiles = {}
    cur = None
    with open(path, encoding="latin-1") as f:
        for line in f:
            line = line.strip()
            if line.startswith("field="):
                x, y = (int(v) for v in line.split("=", 1)[1].strip('"').split(","))
                cur = tiles.setdefault((x, y), {"g": 0, "f": 0, "fl": set(), "it": "", "ch": ""})
            elif cur is not None and "=" in line:
                k, v = line.split("=", 1)
                if k == "gsprite":
                    cur["g"] = int(v) & 0xFFFF
                elif k == "fsprite":
                    cur["f"] = int(v) & 0xFFFF
                elif k == "flag":
                    cur["fl"].add(v)
                elif k == "it":
                    cur["it"] = v
                elif k == "ch":
                    cur["ch"] = v
    return tiles


def in_rects(x, y, rects):
    return any(lo[0] <= x <= hi[0] and lo[1] <= y <= hi[1] for lo, hi in rects)


# --- Materials ---
def ensure_materials():
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mel = unreal.MaterialEditingLibrary
    base_path = MAT_DIR + "/M_Kit"
    if not unreal.EditorAssetLibrary.does_asset_exist(base_path):
        mat = tools.create_asset("M_Kit", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
        mat.set_editor_property("used_with_instanced_static_meshes", True)

        def node(cls, x, y, **props):
            e = mel.create_material_expression(mat, cls, x, y)
            for k, v in props.items():
                e.set_editor_property(k, v)
            return e

        def link(a, b, inp, out=""):
            mel.connect_material_expressions(a, out, b, inp)

        def op(cls, a, x, y, const_b):
            e = node(cls, x, y, const_b=const_b)
            link(a, e, "A")
            return e

        def unary(cls, a, x, y):
            e = node(cls, x, y)
            link(a, e, "")
            return e

        def band(v, x, y, width):
            # 0 on a thin band at the bottom of each [0,1) cell (mortar), 1 elsewhere.
            return unary(unreal.MaterialExpressionSaturate,
                         op(unreal.MaterialExpressionMultiply,
                            op(unreal.MaterialExpressionSubtract, unary(unreal.MaterialExpressionFrac, v, x, y), x + 150, y, width),
                            x + 300, y, 30.0), x + 450, y)

        color = node(unreal.MaterialExpressionPerInstanceCustomData3Vector, -1800, -300, data_index=0)
        wp = node(unreal.MaterialExpressionWorldPosition, -2600, 200)
        mx = node(unreal.MaterialExpressionComponentMask, -2400, 0, r=True, g=False, b=False, a=False)
        my = node(unreal.MaterialExpressionComponentMask, -2400, 200, r=False, g=True, b=False, a=False)
        mz = node(unreal.MaterialExpressionComponentMask, -2400, 400, r=False, g=False, b=True, a=False)
        for m in (mx, my, mz):
            link(wp, m, "")

        # Walls: 45 cm courses, 90 cm blocks staggered by half a block per course.
        cz = op(unreal.MaterialExpressionDivide, mz, -2200, 400, 45.0)
        courses = band(cz, -2000, 400, 0.07)
        xy = node(unreal.MaterialExpressionAdd, -2200, 100)
        link(mx, xy, "A")
        link(my, xy, "B")
        u = node(unreal.MaterialExpressionAdd, -2000, 150)
        link(op(unreal.MaterialExpressionDivide, xy, -2100, 100, 90.0), u, "A")
        link(op(unreal.MaterialExpressionMultiply, unary(unreal.MaterialExpressionFloor, cz, -2100, 300), -2050, 300, 0.5), u, "B")
        joints = band(u, -1850, 150, 0.05)
        wall = node(unreal.MaterialExpressionMultiply, -1300, 300)
        link(courses, wall, "A")
        link(joints, wall, "B")

        # Tops: 100 cm flagstones, rows staggered.
        cx = op(unreal.MaterialExpressionDivide, mx, -2200, 700, 100.0)
        cy = node(unreal.MaterialExpressionAdd, -2000, 800)
        link(op(unreal.MaterialExpressionDivide, my, -2200, 850, 100.0), cy, "A")
        link(op(unreal.MaterialExpressionMultiply, unary(unreal.MaterialExpressionFloor, cx, -2100, 950), -2050, 950, 0.5), cy, "B")
        flags = node(unreal.MaterialExpressionMultiply, -1300, 700)
        link(band(cx, -1850, 650, 0.04), flags, "A")
        link(band(cy, -1850, 800, 0.04), flags, "B")

        normal = node(unreal.MaterialExpressionVertexNormalWS, -1900, 1100)
        nz = node(unreal.MaterialExpressionComponentMask, -1750, 1100, r=False, g=False, b=True, a=False)
        link(normal, nz, "")
        top = unary(unreal.MaterialExpressionSaturate,
                    op(unreal.MaterialExpressionMultiply,
                       op(unreal.MaterialExpressionSubtract, unary(unreal.MaterialExpressionAbs, nz, -1600, 1100), -1450, 1100, 0.7),
                       -1300, 1100, 10.0), -1150, 1100)
        pattern = node(unreal.MaterialExpressionLinearInterpolate, -1000, 500)
        link(wall, pattern, "A")
        link(flags, pattern, "B")
        link(top, pattern, "Alpha")
        mortar = node(unreal.MaterialExpressionLinearInterpolate, -850, 500, const_a=0.55, const_b=1.0)
        link(pattern, mortar, "Alpha")
        pattern_param = node(unreal.MaterialExpressionScalarParameter, -850, 700, parameter_name="Pattern", default_value=1.0)
        shade = node(unreal.MaterialExpressionLinearInterpolate, -700, 500, const_a=1.0)
        link(mortar, shade, "B")
        link(pattern_param, shade, "Alpha")

        noise = node(unreal.MaterialExpressionNoise, -1000, -100, scale=0.012, output_min=0.72, output_max=1.12, levels=4)
        link(wp, noise, "Position")
        mottle_param = node(unreal.MaterialExpressionScalarParameter, -850, 0, parameter_name="Mottle", default_value=1.0)
        mottle = node(unreal.MaterialExpressionLinearInterpolate, -700, -100, const_a=1.0)
        link(noise, mottle, "B")
        link(mottle_param, mottle, "Alpha")

        c1 = node(unreal.MaterialExpressionMultiply, -500, -200)
        link(color, c1, "A")
        link(shade, c1, "B")
        c2 = node(unreal.MaterialExpressionMultiply, -350, -200)
        link(c1, c2, "A")
        link(mottle, c2, "B")
        mel.connect_material_property(c2, "", unreal.MaterialProperty.MP_BASE_COLOR)
        rough = node(unreal.MaterialExpressionConstant, -350, 100, r=0.88)
        mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
        mel.recompile_material(mat)
        unreal.EditorAssetLibrary.save_asset(base_path)
        unreal.log("build_fortress: created %s" % base_path)

    base = unreal.load_asset(base_path)
    for name, pattern, mottle in (("MI_KitStone", 1.0, 1.0), ("MI_KitPlain", 0.0, 0.6)):
        path = MAT_DIR + "/" + name
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            continue
        mi = tools.create_asset(name, MAT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mi.set_editor_property("parent", base)
        mel.set_material_instance_scalar_parameter_value(mi, "Pattern", pattern)
        mel.set_material_instance_scalar_parameter_value(mi, "Mottle", mottle)
        unreal.EditorAssetLibrary.save_asset(path)
        unreal.log("build_fortress: created %s" % path)

    glow_path = MAT_DIR + "/M_KitGlow"
    if not unreal.EditorAssetLibrary.does_asset_exist(glow_path):
        mat = tools.create_asset("M_KitGlow", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
        mat.set_editor_property("used_with_instanced_static_meshes", True)
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        color = mel.create_material_expression(mat, unreal.MaterialExpressionPerInstanceCustomData3Vector, -500, 0)
        color.set_editor_property("data_index", 0)
        boost = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -250, 0)
        boost.set_editor_property("const_b", 25.0)
        mel.connect_material_expressions(color, "", boost, "A")
        mel.connect_material_property(boost, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        mel.recompile_material(mat)
        unreal.EditorAssetLibrary.save_asset(glow_path)
        unreal.log("build_fortress: created %s" % glow_path)


# --- Textured layers ---
# AstoniaBuilding layer name -> (texture made by make_textures.py, world size of one repeat in cm).
# import_textures.py builds /Game/Astonia/Materials/Kit/MI_Kit_<layer> from this.
TEXTURED = {
    "Stone": ("Bricks", 360.0),
    "StoneRound": ("Bricks", 360.0),
    "Trim": ("SmoothStone", 300.0),
    "Wood": ("Planks", 200.0),
    "Roof": ("Shingles", 260.0),
    "RoofCone": ("Slate", 220.0),
    "Trunk": ("Bark", 160.0),
    "Leaves": ("Leaves", 220.0),
    "Ground": ("Dirt", 400.0),
    "Rock": ("Rock", 300.0),
    "Grass": ("Grass", 400.0),
    "Dirt": ("Dirt", 400.0),
    "Cobble": ("Cobble", 260.0),
    "Forest": ("Forest", 400.0),
    "Planks": ("Planks", 200.0),
    "Tiles": ("Tiles", 200.0),
    "Flagstone": ("Flagstone", 260.0),
    "Plaster": ("Plaster", 300.0),
    "Timber": ("Timber", 200.0),
}


def _kit_name(kit):
    # unreal enum values print as e.g. "AstoniaKit.STONE_ROUND"; the layer is "StoneRound".
    return "".join(p.capitalize() for p in str(kit).split(".")[-1].split("_"))


def _texture_averages():
    import json
    import os
    path = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "SourceArt", "Textures", "averages.json")
    try:
        with open(path) as f:
            return json.load(f)
    except OSError:
        return {}


_AVERAGES = _texture_averages()
_TINT_CACHE = {}


# Model layers (Blender kit) whose main material is another layer's: tinted like that layer.
MODEL_MATERIAL = {
    "StoneWall": "Stone", "StoneWindow": "Stone", "StonePost": "Stone",
    "PlasterWall": "Plaster", "PlasterWindow": "Plaster", "PlasterPost": "Plaster",
    "TimberWall": "Timber", "TimberWindow": "Timber", "TimberPost": "Timber",
    "RoofPanel": "Roof", "RoofRidge": "Roof", "Gable": "Plaster",
}


def kit_tint(kit):
    """Per-channel factor that turns a wanted colour into the tint for a textured layer, so the piece
    shows that colour on average (the material draws texture x tint x 2). None for plain layers."""
    if kit not in _TINT_CACHE:
        name = _kit_name(kit)
        name = MODEL_MATERIAL.get(name, name)
        avg = _AVERAGES.get(TEXTURED[name][0]) if name in TEXTURED else None
        _TINT_CACHE[kit] = [0.5 / max(a, 0.02) for a in avg] if avg else None
    return _TINT_CACHE[kit]


# --- Piece helpers ---
class Builder:
    def __init__(self, actor):
        self.actor = actor
        self.count = 0

    def piece(self, kit, center, size, color, yaw=0.0, pitch=0.0, roll=0.0, vary=0.05):
        # Small brightness changes per piece so repeated blocks don't look pasted.
        h = (int(center[0] * 7 + center[1] * 13 + center[2] * 17) * 2654435761) & 0xFFFFFFFF
        f = 1.0 + vary * (((h >> 8) % 1000) / 500.0 - 1.0)
        tint = kit_tint(kit)
        c = [color[i] * f * (tint[i] if tint else 1.0) for i in range(3)]
        self.actor.add_piece(kit, unreal.Vector(*center), unreal.Vector(*size),
                             unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw),
                             unreal.LinearColor(c[0], c[1], c[2], 1.0))
        self.count += 1

    def box(self, kit, x0, y0, z0, x1, y1, z1, color, **kw):
        self.piece(kit, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (abs(x1 - x0), abs(y1 - y0), abs(z1 - z0)), color, **kw)

    def light(self, x, y, z, intensity=2500.0, radius=1400.0):
        self.actor.add_light(unreal.Vector(x, y, z), intensity, radius, unreal.LinearColor(1.0, 0.68, 0.38, 1.0))


def kit_tree(b, x, y, h, kind="tree", collide=True):
    """One Blender tree model (SM_Kit_Tree*/Bush, Scripts/blender/make_trees.py) at x, y with a random
    turn and size from the hash h; kind: "tree", "forest" (big) or "bush". The trunk gets a blocker."""
    if kind == "forest":
        kit = K.TREE_FOREST
    elif kind == "bush":
        kit = K.BUSH
    else:
        kit = K.TREE_ROUND if h % 3 else K.TREE_TALL
    s = 0.8 + (h >> 5) % 45 / 100.0
    jx, jy = (h >> 3) % 70 - 35, (h >> 9) % 70 - 35
    b.piece(kit, (x + jx, y + jy, 0), (100 * s, 100 * s, 100 * s * (0.92 + (h >> 13) % 16 / 100.0)), (0.5, 0.5, 0.5), yaw=h % 360, vary=0.12)
    if collide and kind != "bush":
        b.box(K.BLOCKER, x + jx - 25 * s, y + jy - 25 * s, 0, x + jx + 25 * s, y + jy + 25 * s, 300, (0.5, 0.5, 0.5), vary=0)


def sconce(b, x, y, z):
    b.box(K.DECOR, x - 8, y - 8, z - 40, x + 8, y + 8, z, METAL, vary=0)
    b.piece(K.GLOW, (x, y, z + 18), (26, 26, 36), FLAME, vary=0)
    b.light(x, y, z + 40, 1800.0, 1100.0)


