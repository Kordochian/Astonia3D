"""Imports SourceArt/Textures (made by Scripts/make_textures.py: T_<Name>, T_<Name>_N, T_<Name>_ORM, T_Macro)
into /Game/Astonia/Textures and builds the kit materials in /Game/Astonia/Materials/Kit:

M_KitPBR, world-aligned (no UVs needed, pieces of any size and rotation tile the same):
- static switch "Triplanar": walls, roofs, rocks and trees blend three projections; floors use the top
  projection only (3 texture samples instead of 9), which keeps the ground cheap;
- albedo x per-instance colour x 2, normal map (added to the vertex normal in world space), ORM
  (ambient occlusion, roughness), and T_Macro at two large scales to hide tiling;
- one MI_Kit_<Layer> per AstoniaBuilding layer (TEXTURED in astonia_kit.py), picked up by name.

  UnrealEditor-Cmd.exe AstoniaUE5.uproject -run=pythonscript -script="<project>/Scripts/import_textures.py"
Re-running re-imports the textures and updates the instances.
"""
import os
import sys
import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE = os.path.join(PROJECT, "SourceArt", "Textures")
TEX_DIR = "/Game/Astonia/Textures"
KIT_DIR = "/Game/Astonia/Materials/Kit"
MASTER = KIT_DIR + "/M_KitPBR"

sys.path.insert(0, os.path.join(PROJECT, "Scripts"))
from astonia_kit import TEXTURED as LAYERS  # noqa: E402  (layer -> texture, repeat size)

# Layers that only ever show their top (floor slabs): one projection is enough.
FLOORS = {"Ground", "Grass", "Dirt", "Cobble", "Forest", "Planks", "Tiles", "Flagstone"}
# Normal map strength where the painted height is noisy (soft ground reads better flatter).
NORMAL_STRENGTH = {"Ground": 0.45, "Dirt": 0.45, "Grass": 0.55, "Forest": 0.55, "Leaves": 0.8, "Plaster": 0.6}

tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary


def import_textures():
    tasks = []
    for name in sorted(os.listdir(SOURCE)):
        if not name.lower().endswith(".png"):
            continue
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", os.path.join(SOURCE, name))
        t.set_editor_property("destination_path", TEX_DIR)
        t.set_editor_property("automated", True)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", False)
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    for t in tasks:
        name = os.path.splitext(os.path.basename(t.get_editor_property("filename")))[0]
        path = "%s/%s" % (TEX_DIR, name)
        tex = unreal.load_asset(path)
        if name.endswith("_N"):
            tex.set_editor_property("srgb", False)
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD_NORMAL_MAP)
        elif name.endswith("_ORM") or name == "T_Macro":
            tex.set_editor_property("srgb", False)
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
            tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD_SPECULAR)
        else:
            tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
        eal.save_asset(path)
    unreal.log("import_textures: imported %d textures" % len(tasks))


def build_master():
    if eal.does_asset_exist(MASTER):
        eal.delete_asset(MASTER)
    mat = tools.create_asset("M_KitPBR", KIT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    mat.set_editor_property("used_with_nanite", True)   # Blender models are Nanite meshes
    mat.set_editor_property("tangent_space_normal", False)
    tex = lambda n: unreal.load_asset("%s/%s" % (TEX_DIR, n))
    shared = unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS

    def node(cls, x, y, **props):
        e = mel.create_material_expression(mat, cls, x, y)
        for k, v in props.items():
            e.set_editor_property(k, v)
        return e

    def link(a, b, inp, out=""):
        mel.connect_material_expressions(a, out, b, inp)

    def mask(src, x, y, r=False, g=False, b=False):
        m = node(unreal.MaterialExpressionComponentMask, x, y, r=r, g=g, b=b, a=False)
        link(src, m, "")
        return m

    def binop(cls, a, b, x, y):
        e = node(cls, x, y)
        link(a, e, "A")
        link(b, e, "B")
        return e

    def const(v, x, y):
        return node(unreal.MaterialExpressionConstant, x, y, r=v)

    def sample(param, default, uv, x, y, kind=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR):
        s = node(unreal.MaterialExpressionTextureSampleParameter2D, x, y, parameter_name=param, texture=default, sampler_type=kind, sampler_source=shared)
        link(uv, s, "UVs")
        return s

    def vec3(a, b, c, x, y):
        return binop(unreal.MaterialExpressionAppendVector, binop(unreal.MaterialExpressionAppendVector, a, b, x, y), c, x + 120, y)

    def switch(name, on, off, x, y):
        s = node(unreal.MaterialExpressionStaticSwitchParameter, x, y, parameter_name=name, default_value=True)
        link(on, s, "True")
        link(off, s, "False")
        return s

    wp = node(unreal.MaterialExpressionWorldPosition, -3200, 0)
    size = node(unreal.MaterialExpressionScalarParameter, -3200, 300, parameter_name="Size", default_value=400.0)
    wx, wy, wz = mask(wp, -3000, -100, r=True), mask(wp, -3000, 0, g=True), mask(wp, -3000, 100, b=True)
    nz = node(unreal.MaterialExpressionMultiply, -2850, 150, const_b=-1.0)
    link(wz, nz, "A")
    uvs = []
    for i, (a, b) in enumerate(((wy, nz), (wx, nz), (wx, wy))):   # faces along X, along Y, tops
        uv = binop(unreal.MaterialExpressionAppendVector, a, b, -2700, -150 + i * 150)
        uvs.append(binop(unreal.MaterialExpressionDivide, uv, size, -2550, -150 + i * 150))

    alb = [sample("Tex", tex("T_Grass"), uvs[i], -2300, -900 + i * 220) for i in range(3)]
    orm = [sample("ORM", tex("T_Grass_ORM"), uvs[i], -2300, -200 + i * 220, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS) for i in range(3)]
    nrm = [sample("Nrm", tex("T_Grass_N"), uvs[i], -2300, 500 + i * 220, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL) for i in range(3)]

    # Triplanar weights from the vertex normal (sharpened).
    vn = node(unreal.MaterialExpressionVertexNormalWS, -2300, 1300)
    p = binop(unreal.MaterialExpressionMultiply, vn, vn, -2150, 1300)
    p = binop(unreal.MaterialExpressionMultiply, p, p, -2000, 1300)
    p = binop(unreal.MaterialExpressionMultiply, p, p, -1850, 1300)
    w = [mask(p, -1700, 1200 + i * 100, r=(i == 0), g=(i == 1), b=(i == 2)) for i in range(3)]
    total = binop(unreal.MaterialExpressionAdd, binop(unreal.MaterialExpressionAdd, w[0], w[1], -1550, 1250), w[2], -1400, 1300)

    def blend3(samples, x, y):
        acc = None
        for i in range(3):
            term = binop(unreal.MaterialExpressionMultiply, samples[i], w[i], x, y + i * 120)
            acc = term if acc is None else binop(unreal.MaterialExpressionAdd, acc, term, x + 150, y + i * 120)
        return binop(unreal.MaterialExpressionDivide, acc, total, x + 300, y)

    albedo = switch("Triplanar", blend3(alb, -1900, -900), alb[2], -1450, -800)
    masks = switch("Triplanar", blend3(orm, -1900, -200), orm[2], -1450, -100)

    # Detail normals into world space per projection, then added to the vertex normal.
    zero = const(0.0, -2050, 1100)
    tn = [(mask(nrm[i], -2050, 500 + i * 220, r=True), mask(nrm[i], -2050, 560 + i * 220, g=True)) for i in range(3)]
    neg_ty = [node(unreal.MaterialExpressionMultiply, -1950, 520 + i * 220, const_b=-1.0) for i in range(3)]
    for i in range(3):
        link(tn[i][1], neg_ty[i], "A")
    d_x = vec3(zero, tn[0][0], neg_ty[0], -1850, 500)
    d_y = vec3(tn[1][0], zero, neg_ty[1], -1850, 720)
    d_z = vec3(tn[2][0], tn[2][1], zero, -1850, 940)
    detail = switch("Triplanar", blend3([d_x, d_y, d_z], -1600, 500), d_z, -1150, 600)
    strength = node(unreal.MaterialExpressionScalarParameter, -1150, 800, parameter_name="NormalStrength", default_value=1.0)
    scaled = binop(unreal.MaterialExpressionMultiply, detail, strength, -1000, 650)
    normal = node(unreal.MaterialExpressionNormalize, -700, 650)
    link(binop(unreal.MaterialExpressionAdd, vn, scaled, -850, 650), normal, "")
    mel.connect_material_property(normal, "", unreal.MaterialProperty.MP_NORMAL)

    # Large-scale variation: two scales of T_Macro over world XY.
    macro_vals = []
    for i, scale in enumerate((5200.0, 1300.0)):
        uv = node(unreal.MaterialExpressionDivide, -1800, 1500 + i * 150, const_b=scale)
        link(uvs[2], uv, "A")  # top uv is already divided by Size; re-divide to get metres-scale noise
        mul = node(unreal.MaterialExpressionMultiply, -1650, 1500 + i * 150)
        link(uv, mul, "A")
        link(size, mul, "B")
        m = sample("Macro", tex("T_Macro"), mul, -1500, 1500 + i * 220, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        macro_vals.append(mask(m, -1300, 1500 + i * 220, r=True))
    mavg = node(unreal.MaterialExpressionMultiply, -1100, 1550, const_b=0.5)
    link(binop(unreal.MaterialExpressionAdd, macro_vals[0], macro_vals[1], -1200, 1550), mavg, "A")
    mstr = node(unreal.MaterialExpressionScalarParameter, -1100, 1700, parameter_name="MacroStrength", default_value=0.22)
    lo = binop(unreal.MaterialExpressionSubtract, const(1.0, -950, 1650), mstr, -900, 1650)
    hi = binop(unreal.MaterialExpressionAdd, const(1.0, -950, 1750), mstr, -900, 1750)
    mfac = node(unreal.MaterialExpressionLinearInterpolate, -750, 1650)
    link(lo, mfac, "A")
    link(hi, mfac, "B")
    link(mavg, mfac, "Alpha")

    ao = mask(masks, -1150, -100, r=True)
    rough = mask(masks, -1150, 0, g=True)
    cavity = node(unreal.MaterialExpressionLinearInterpolate, -950, -150, const_a=1.0, const_alpha=0.6)
    link(ao, cavity, "B")
    tint = node(unreal.MaterialExpressionPerInstanceCustomData3Vector, -1150, -500, data_index=0)
    c = binop(unreal.MaterialExpressionMultiply, albedo, tint, -1000, -700)
    c = binop(unreal.MaterialExpressionMultiply, c, mfac, -850, -700)
    c = binop(unreal.MaterialExpressionMultiply, c, cavity, -700, -700)
    base = node(unreal.MaterialExpressionMultiply, -550, -700, const_b=2.0)
    link(c, base, "A")
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(ao, "", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    mel.recompile_material(mat)
    eal.save_asset(MASTER)
    unreal.log("import_textures: created %s" % MASTER)
    return mat


def build_instances(master):
    for layer, (name, size) in LAYERS.items():
        path = "%s/MI_Kit_%s" % (KIT_DIR, layer)
        if eal.does_asset_exist(path):
            mi = unreal.load_asset(path)
        else:
            mi = tools.create_asset("MI_Kit_%s" % layer, KIT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mi.set_editor_property("parent", master)
        for param, suffix in (("Tex", ""), ("Nrm", "_N"), ("ORM", "_ORM")):
            mel.set_material_instance_texture_parameter_value(mi, param, unreal.load_asset("%s/T_%s%s" % (TEX_DIR, name, suffix)))
        mel.set_material_instance_scalar_parameter_value(mi, "Size", size)
        mel.set_material_instance_scalar_parameter_value(mi, "NormalStrength", NORMAL_STRENGTH.get(layer, 1.0))
        mel.set_material_instance_static_switch_parameter_value(mi, "Triplanar", layer not in FLOORS)
        mel.update_material_instance(mi)
        eal.save_asset(path)
    old = KIT_DIR + "/M_KitTex"
    if eal.does_asset_exist(old):
        eal.delete_asset(old)
    unreal.log("import_textures: %d kit material instances" % len(LAYERS))


import_textures()
build_instances(build_master())
