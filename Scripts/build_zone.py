"""Builds an original area into the open world map as an AstoniaZoneActor (placeholder geometry),
with its teleport-hole areas moved underground and joined by lifts (AstoniaLift) and secret doors
(AstoniaSecretDoor).

Headless (defaults build Cameron, area 1, at the world origin):
  UnrealEditor-Cmd.exe AstoniaUE5.uproject -run=pythonscript -script="<project>/Scripts/build_zone.py"
Other areas: set the environment variables ASTONIA_ZONE (folder, e.g. 3), ASTONIA_MAP (file name),
ASTONIA_LABEL and ASTONIA_ORIGIN ("x,y" in metres) before running, and add a LAYOUTS entry.
Re-running rebuilds in place. Scripts/analyze_links.py lists an area's sections and teleports.
"""
import os
import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
ZONES = os.path.normpath(os.path.join(PROJECT, "../../source_codes/astonia_community_server35-main/zones"))

ZONE = os.environ.get("ASTONIA_ZONE", "1")
MAP_NAME = os.environ.get("ASTONIA_MAP", "cameron.map")
LABEL = os.environ.get("ASTONIA_LABEL", "Zone_01_Cameron")
ORIGIN = [float(v) * 100.0 for v in os.environ.get("ASTONIA_ORIGIN", "0,0").split(",")]
SET_PLAYER_START = ZONE == "1"

WORLD = "/Game/Maps/L_World"
MATERIAL = "/Game/Astonia/Materials/M_Placeholder"
MATERIAL_SOLID = "/Game/Astonia/Materials/M_PlaceholderSolid"

# Per area: underground placements (label, entry tile = original teleport target, column tile = the
# original hole, parent placement index or -1 for the surface, depth in cm, secret door) and taller
# surface walls (min tile, max tile, wall height cm). Levels that overlap on the map are kept at
# least 600 cm apart; shafts that pass another level get a casing.
LAYOUTS = {
    "1": {
        "placements": [
            ("Thieves' Guild", (160, 212), (108, 92), -1, 600, False),
            ("Robber cave", (31, 237), (244, 78), -1, 600, False),
            ("Mage's cellar tunnels", (184, 226), (138, 136), -1, 600, False),
            ("Guard quarters", (36, 221), (112, 107), -1, 1200, False),
            ("Crypt cavern", (101, 211), (195, 231), 2, 1200, True),   # behind a secret door in the tunnels
            ("Skeleton cave", (38, 232), (85, 83), -1, 1800, False),
        ],
        # The castle courtyard where new characters start (server: tile 126,179) becomes the fortress.
        "heights": [
            ((110, 165), (146, 191), 800),
            ((110, 165), (113, 168), 1200),
            ((143, 165), (146, 168), 1200),
            ((110, 188), (113, 191), 1200),
            ((143, 188), (146, 191), 1200),
            ((120, 165), (121, 171), 1000),   # gate towers either side of the north entrance
            ((125, 165), (126, 171), 1000),
        ],
        # Named characters' looks (DT_Skins rows, Scripts/create_skins.py); others pick from a pool.
        "skins": {
            "knight_castle_mage": "MageRobe",      # Gwendylon
            "knight_castle_fighter": "KnightPlate", # Yoakin
            "knight_castle_lydia": "NobleDress",
            "knight_castle_james": "Drunkard",
            "knight_castle_nook": "Guard",
        },
    },
}


def ensure_materials():
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mel = unreal.MaterialEditingLibrary
    for asset, instanced in ((MATERIAL, True), (MATERIAL_SOLID, False)):
        if unreal.EditorAssetLibrary.does_asset_exist(asset):
            continue
        path, name = asset.rsplit("/", 1)
        mat = tools.create_asset(name, path, unreal.Material, unreal.MaterialFactoryNew())
        if instanced:
            # Instanced meshes: colour per instance (PerInstanceCustomData 0-2).
            color = mel.create_material_expression(mat, unreal.MaterialExpressionPerInstanceCustomData3Vector, -400, 0)
            color.set_editor_property("data_index", 0)
            mat.set_editor_property("used_with_instanced_static_meshes", True)
        else:
            # Single meshes (secret doors, lifts): colour from custom primitive data 0-2.
            color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, 0)
            color.set_editor_property("parameter_name", "Color")
            color.set_editor_property("default_value", unreal.LinearColor(0.45, 0.43, 0.40, 1.0))
            color.set_editor_property("use_custom_primitive_data", True)
            color.set_editor_property("primitive_data_index", 0)
        mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
        rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 200)
        rough.set_editor_property("r", 0.85)
        mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
        mel.recompile_material(mat)
        unreal.EditorAssetLibrary.save_asset(asset)
        unreal.log("build_zone: created %s" % asset)


def point(p):
    return unreal.IntPoint(p[0], p[1])


ensure_materials()
unreal.EditorLoadingAndSavingUtils.load_map(WORLD)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

zone = None
for a in actors.get_all_level_actors():
    if a.get_actor_label() == LABEL:
        zone = a
        break

if zone is None:
    zone = actors.spawn_actor_from_class(unreal.AstoniaZoneActor, unreal.Vector(ORIGIN[0], ORIGIN[1], 0.0))
    zone.set_actor_label(LABEL)
    zone.set_folder_path("Astonia/Zones")

layout = LAYOUTS.get(ZONE, {"placements": [], "heights": []})
placements = []
for label, entry, column, parent, depth, secret in layout["placements"]:
    p = unreal.AstoniaSectionPlacement()
    p.set_editor_property("label", label)
    p.set_editor_property("entry_tile", point(entry))
    p.set_editor_property("column_tile", point(column))
    p.set_editor_property("parent", parent)
    p.set_editor_property("depth", float(depth))
    p.set_editor_property("secret_door", secret)
    placements.append(p)
heights = []
for lo, hi, h in layout["heights"]:
    r = unreal.AstoniaHeightRegion()
    r.set_editor_property("min", point(lo))
    r.set_editor_property("max", point(hi))
    r.set_editor_property("wall_height", float(h))
    heights.append(r)

zone.set_editor_property("is_spatially_loaded", False)
zone.set_editor_property("map_file", os.path.join(ZONES, ZONE, MAP_NAME).replace("\\", "/"))
zone.set_editor_property("zone", ZONE)
zone.set_editor_property("placements", placements)
zone.set_editor_property("height_regions", heights)
zone.set_editor_property("character_skins", {unreal.Name(k): unreal.Name(v) for k, v in layout.get("skins", {}).items()})
zone.rebuild_from_map()

# Lifts and secret doors are separate actors (they move); rebuild them from the zone's shafts.
folder = "Astonia/Zones/%s_Shafts" % LABEL
for a in actors.get_all_level_actors():
    if str(a.get_folder_path()) == folder:
        actors.destroy_actor(a)

solid = unreal.load_asset(MATERIAL_SOLID)
zone_transform = zone.get_actor_transform()
for shaft in zone.get_editor_property("shafts"):
    top = unreal.MathLibrary.transform_location(zone_transform, shaft.top)
    lift = actors.spawn_actor_from_class(unreal.AstoniaLift, top)
    lift.set_actor_label("Lift_%s" % shaft.label.replace(" ", "_").replace("'", ""))
    lift.set_folder_path(folder)
    lift.set_editor_property("is_spatially_loaded", False)
    lift.set_editor_property("bottom_offset", -shaft.drop)
    lift.set_editor_property("destination", shaft.label)
    platform = lift.get_component_by_class(unreal.StaticMeshComponent)
    platform.set_material(0, solid)
    for i, v in enumerate((0.55, 0.45, 0.25)):
        platform.set_custom_primitive_data_float(i, v)
    if shaft.secret_door:
        door = actors.spawn_actor_from_class(unreal.AstoniaSecretDoor, top + unreal.Vector(0.0, 0.0, 150.0))
        door.set_actor_label("SecretDoor_%s" % shaft.label.replace(" ", "_").replace("'", ""))
        door.set_folder_path(folder)
        door.set_editor_property("is_spatially_loaded", False)
        door.set_editor_property("wall_color", shaft.wall_color)
    unreal.log("build_zone: shaft %s at %s, drop %.0f cm%s" % (shaft.label, top, shaft.drop, ", secret door" if shaft.secret_door else ""))

if SET_PLAYER_START:
    start = zone.get_editor_property("start_tile")
    loc = zone.tile_to_world(start.x, start.y) + unreal.Vector(0.0, 0.0, 120.0)
    for a in actors.get_all_level_actors():
        if isinstance(a, unreal.PlayerStart):
            a.modify()
            a.set_actor_location(loc, False, False)
            unreal.log("build_zone: PlayerStart moved to tile %d,%d (%s)" % (start.x, start.y, loc))

unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log("build_zone: saved %s" % LABEL)
