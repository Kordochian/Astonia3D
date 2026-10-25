"""Creates the default human body look for the UE5 mannequin (Manny / Quinn):

- /Game/Characters/Skins/M_HumanSkin: skin-tone material with undergarments (briefs, and a chest band
  on female bodies) painted from the mesh's rest-pose position, so they follow the body when it moves.
  Parameters: SkinTone, UnderwearColor, Roughness, BriefsMin/Max (cm from the feet), ChestMin/Max,
  HalfWidth (how far from the body's centre line the garments reach), Female (0/1).
- /Game/Characters/Skins/DT_Skins: body rows (FAstoniaSkinRow): skin tone, undergarment colour, eye and
  hair colour (eyes and hair are separate pieces of the appearance system; the colours wait for them).

Clothes, armour, hair, jewellery and weapons are separate models layered on top (AstoniaAppearance.h).
Re-running resets the rows to the values below and rebuilds the material.

  UnrealEditor-Cmd.exe AstoniaUE5.uproject -run=pythonscript -script="<project>/Scripts/create_skins.py"
"""
import json
import unreal

TABLE = "/Game/Characters/Skins/DT_Skins"
MATERIAL = "/Game/Characters/Skins/M_HumanSkin"

# name: (skin tone, undergarment colour, eye colour, hair colour)
SKINS = {
    "Fair":   ((0.80, 0.60, 0.50), (0.85, 0.83, 0.78), (0.25, 0.45, 0.75), (0.75, 0.60, 0.35)),
    "Light":  ((0.72, 0.50, 0.38), (0.85, 0.83, 0.78), (0.35, 0.25, 0.15), (0.30, 0.18, 0.10)),
    "Olive":  ((0.58, 0.40, 0.27), (0.80, 0.78, 0.72), (0.30, 0.35, 0.20), (0.12, 0.08, 0.05)),
    "Tan":    ((0.50, 0.32, 0.20), (0.75, 0.72, 0.65), (0.30, 0.20, 0.10), (0.10, 0.07, 0.05)),
    "Brown":  ((0.33, 0.20, 0.12), (0.85, 0.83, 0.78), (0.20, 0.12, 0.06), (0.05, 0.04, 0.03)),
    "Dark":   ((0.18, 0.11, 0.07), (0.85, 0.83, 0.78), (0.15, 0.09, 0.05), (0.03, 0.03, 0.03)),
}

CODE = """
float Garment = 0;
if (P.z > BriefsMin && P.z < BriefsMax && abs(P.x) < HalfWidth) Garment = 1;
if (Female > 0.5 && P.z > ChestMin && P.z < ChestMax && abs(P.x) < HalfWidth * 0.9) Garment = 1;
return lerp(SkinTone, UnderwearColor, Garment);
"""

SCALARS = {"BriefsMin": 83.0, "BriefsMax": 102.0, "ChestMin": 120.0, "ChestMax": 134.0,
           "HalfWidth": 21.0, "Female": 0.0, "Roughness": 0.55}


def build_material():
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mel = unreal.MaterialEditingLibrary
    if unreal.EditorAssetLibrary.does_asset_exist(MATERIAL):
        mat = unreal.load_asset(MATERIAL)
        mel.delete_all_material_expressions(mat)
    else:
        path, name = MATERIAL.rsplit("/", 1)
        mat = tools.create_asset(name, path, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("used_with_skeletal_mesh", True)

    custom = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -300, 0)
    custom.set_editor_property("code", CODE)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    custom.set_editor_property("description", "Undergarments")
    names = ["P", "SkinTone", "UnderwearColor", "BriefsMin", "BriefsMax", "ChestMin", "ChestMax", "HalfWidth", "Female"]
    inputs = []
    for n in names:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        inputs.append(ci)
    custom.set_editor_property("inputs", inputs)

    # The rest-pose position only exists in the vertex shader; hand it to the pixel shader.
    pos = mel.create_material_expression(mat, unreal.MaterialExpressionPreSkinnedPosition, -1000, -200)
    interp = mel.create_material_expression(mat, unreal.MaterialExpressionVertexInterpolator, -700, -200)
    mel.connect_material_expressions(pos, "", interp, "")
    mel.connect_material_expressions(interp, "", custom, "P")
    y = -100
    for n, default in (("SkinTone", (0.72, 0.50, 0.38)), ("UnderwearColor", (0.85, 0.83, 0.78))):
        v = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -700, y)
        v.set_editor_property("parameter_name", n)
        v.set_editor_property("default_value", unreal.LinearColor(default[0], default[1], default[2], 1.0))
        mel.connect_material_expressions(v, "", custom, n)
        y += 150
    scalars = {}
    for n, default in SCALARS.items():
        s = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -700, y)
        s.set_editor_property("parameter_name", n)
        s.set_editor_property("default_value", default)
        scalars[n] = s
        if n in names:
            mel.connect_material_expressions(s, "", custom, n)
        y += 100
    mel.connect_material_property(custom, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(scalars["Roughness"], "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(MATERIAL)
    unreal.log("create_skins: built %s" % MATERIAL)


def color(c):
    return {"R": c[0], "G": c[1], "B": c[2], "A": 1.0}


def build_table():
    rows = [{"Name": name, "SkinTone": color(s), "UnderwearColor": color(u), "EyeColor": color(e), "HairColor": color(h)}
            for name, (s, u, e, h) in SKINS.items()]
    if unreal.EditorAssetLibrary.does_asset_exist(TABLE):
        table = unreal.load_asset(TABLE)
        if table.get_editor_property("row_struct") != unreal.AstoniaSkinRow.static_struct():
            unreal.EditorAssetLibrary.delete_asset(TABLE)
            table = None
    else:
        table = None
    if table is None:
        path, name = TABLE.rsplit("/", 1)
        factory = unreal.DataTableFactory()
        factory.set_editor_property("struct", unreal.AstoniaSkinRow.static_struct())
        table = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, path, unreal.DataTable, factory)
    if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows)):
        raise RuntimeError("create_skins: could not fill %s" % TABLE)
    unreal.EditorAssetLibrary.save_asset(TABLE)
    unreal.log("create_skins: %d skins in %s" % (len(rows), TABLE))


PIECES = "/Game/Characters/Appearance/DT_Pieces"


def ensure_piece_table():
    """Empty table of wearable pieces (FAstoniaAppearancePieceRow); fill it as models arrive."""
    if unreal.EditorAssetLibrary.does_asset_exist(PIECES):
        return
    path, name = PIECES.rsplit("/", 1)
    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", unreal.AstoniaAppearancePieceRow.static_struct())
    unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, path, unreal.DataTable, factory)
    unreal.EditorAssetLibrary.save_asset(PIECES)
    unreal.log("create_skins: created empty %s" % PIECES)


build_material()
build_table()
ensure_piece_table()
