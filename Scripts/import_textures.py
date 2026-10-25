"""Imports SourceArt/Textures/T_*.png (made by Scripts/make_textures.py) into /Game/Astonia/Textures and
builds the textured kit materials: M_KitTex (world-aligned triplanar projection, so pieces of any size
and rotation tile without UVs; albedo x piece colour x 2) and one MI_Kit_<Layer> per AstoniaBuilding
layer in /Game/Astonia/Materials/Kit, which the building picks up by name.

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
MASTER = KIT_DIR + "/M_KitTex"

sys.path.insert(0, os.path.join(PROJECT, "Scripts"))
from astonia_kit import TEXTURED as LAYERS  # noqa: E402  (layer -> texture, repeat size)

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
        t.set_editor_property("save", True)
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    unreal.log("import_textures: imported %d textures" % len(tasks))


def build_master():
    if eal.does_asset_exist(MASTER):
        return unreal.load_asset(MASTER)
    mat = tools.create_asset("M_KitTex", KIT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    default_tex = unreal.load_asset(TEX_DIR + "/T_Grass")

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

    wp = node(unreal.MaterialExpressionWorldPosition, -2000, 0)
    size = node(unreal.MaterialExpressionScalarParameter, -2000, 300, parameter_name="Size", default_value=400.0)
    wx, wy, wz = mask(wp, -1800, -100, r=True), mask(wp, -1800, 0, g=True), mask(wp, -1800, 100, b=True)
    nz = node(unreal.MaterialExpressionMultiply, -1650, 150, const_b=-1.0)
    link(wz, nz, "A")
    uv_top = binop(unreal.MaterialExpressionAppendVector, wx, wy, -1500, -150)
    uv_x = binop(unreal.MaterialExpressionAppendVector, wy, nz, -1500, 0)    # faces looking along X
    uv_y = binop(unreal.MaterialExpressionAppendVector, wx, nz, -1500, 150)  # faces looking along Y
    samples = []
    for i, uv in enumerate((uv_x, uv_y, uv_top)):
        scaled = binop(unreal.MaterialExpressionDivide, uv, size, -1300, -150 + i * 150)
        s = node(unreal.MaterialExpressionTextureSampleParameter2D, -1100, -300 + i * 250, parameter_name="Tex", texture=default_tex)
        link(scaled, s, "UVs")
        samples.append(s)

    n = node(unreal.MaterialExpressionVertexNormalWS, -1300, 600)
    p = binop(unreal.MaterialExpressionMultiply, n, n, -1150, 600)
    p = binop(unreal.MaterialExpressionMultiply, p, p, -1000, 600)
    weights = [mask(p, -850, 500 + i * 100, r=(i == 0), g=(i == 1), b=(i == 2)) for i in range(3)]
    total = binop(unreal.MaterialExpressionAdd, binop(unreal.MaterialExpressionAdd, weights[0], weights[1], -700, 550), weights[2], -550, 600)
    acc = None
    for i in range(3):
        term = binop(unreal.MaterialExpressionMultiply, samples[i], weights[i], -800, -250 + i * 250)
        acc = term if acc is None else binop(unreal.MaterialExpressionAdd, acc, term, -650, -150 + i * 150)
    col = binop(unreal.MaterialExpressionDivide, acc, total, -450, 0)
    tint = node(unreal.MaterialExpressionPerInstanceCustomData3Vector, -450, 200, data_index=0)
    tinted = binop(unreal.MaterialExpressionMultiply, col, tint, -300, 0)
    base = node(unreal.MaterialExpressionMultiply, -150, 0, const_b=2.0)
    link(tinted, base, "A")
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = node(unreal.MaterialExpressionConstant, -150, 200, r=0.9)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    eal.save_asset(MASTER)
    unreal.log("import_textures: created %s" % MASTER)
    return mat


def build_instances(master):
    for layer, (tex, size) in LAYERS.items():
        path = "%s/MI_Kit_%s" % (KIT_DIR, layer)
        if eal.does_asset_exist(path):
            mi = unreal.load_asset(path)
        else:
            mi = tools.create_asset("MI_Kit_%s" % layer, KIT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mi.set_editor_property("parent", master)
        mel.set_material_instance_texture_parameter_value(mi, "Tex", unreal.load_asset("%s/T_%s" % (TEX_DIR, tex)))
        mel.set_material_instance_scalar_parameter_value(mi, "Size", size)
        eal.save_asset(path)
    unreal.log("import_textures: %d kit material instances" % len(LAYERS))


import_textures()
build_instances(build_master())
