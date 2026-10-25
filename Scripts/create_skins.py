"""Creates or refreshes /Game/Characters/Skins/DT_Skins: looks for the UE5 mannequin (Manny / Quinn).
Each row sets the mannequin material's paint colour, emblem colour, metal and shine (FAstoniaSkinRow).
Rows can also be edited in the editor afterwards; re-running this script resets them to the values below.

  UnrealEditor-Cmd.exe AstoniaUE5.uproject -run=pythonscript -script="<project>/Scripts/create_skins.py"
"""
import json
import unreal

TABLE = "/Game/Characters/Skins/DT_Skins"

# name: (body rgb, trim rgb, metallic, roughness)
SKINS = {
    "Adventurer":     ((0.30, 0.20, 0.11), (0.85, 0.65, 0.25), 0.0, 0.75),  # player: brown leather
    "KnightPlate":    ((0.62, 0.63, 0.66), (0.15, 0.25, 0.70), 1.0, 0.30),  # Yoakin
    "MageRobe":       ((0.92, 0.92, 0.88), (0.80, 0.65, 0.20), 0.0, 0.85),  # Gwendylon: white robes
    "NobleDress":     ((0.12, 0.25, 0.70), (0.95, 0.95, 0.95), 0.0, 0.50),  # Lydia: blue, like her eyes
    "Drunkard":       ((0.40, 0.08, 0.10), (0.30, 0.25, 0.20), 0.0, 0.90),  # James
    "Guard":          ((0.35, 0.36, 0.40), (0.70, 0.10, 0.10), 0.85, 0.40),
    "Peasant":        ((0.45, 0.38, 0.25), (0.35, 0.30, 0.20), 0.0, 0.95),
    "TownsfolkBlue":  ((0.20, 0.30, 0.50), (0.80, 0.80, 0.75), 0.0, 0.85),
    "TownsfolkRed":   ((0.55, 0.18, 0.12), (0.85, 0.75, 0.40), 0.0, 0.85),
    "TownsfolkGreen": ((0.22, 0.38, 0.22), (0.75, 0.65, 0.35), 0.0, 0.85),
    "Merchant":       ((0.22, 0.45, 0.18), (0.95, 0.80, 0.25), 0.2, 0.55),
    "ThiefLeather":   ((0.035, 0.035, 0.045), (0.45, 0.05, 0.05), 0.0, 0.60),
}


def color(c):
    return {"R": c[0], "G": c[1], "B": c[2], "A": 1.0}


rows = [{"Name": name, "Body": color(b), "Trim": color(t), "Metallic": m, "Roughness": r}
        for name, (b, t, m, r) in SKINS.items()]

if unreal.EditorAssetLibrary.does_asset_exist(TABLE):
    table = unreal.load_asset(TABLE)
else:
    path, name = TABLE.rsplit("/", 1)
    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", unreal.AstoniaSkinRow.static_struct())
    table = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, path, unreal.DataTable, factory)

if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows)):
    raise RuntimeError("create_skins: could not fill %s" % TABLE)
unreal.EditorAssetLibrary.save_asset(TABLE)
unreal.log("create_skins: %d skins in %s" % (len(rows), TABLE))
