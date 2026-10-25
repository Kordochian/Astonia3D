"""Builds an original area into the open world map as an AstoniaZoneActor (placeholder geometry).

Headless (defaults build Cameron, area 1, at the world origin):
  UnrealEditor-Cmd.exe AstoniaUE5.uproject -run=pythonscript -script="<project>/Scripts/build_zone.py"
Other areas: set the environment variables ASTONIA_ZONE (folder, e.g. 3), ASTONIA_MAP (file name),
ASTONIA_LABEL and ASTONIA_ORIGIN ("x,y" in metres) before running. Re-running rebuilds in place.
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
TILE = 200.0
SIZE = 256


def ensure_material():
    if unreal.EditorAssetLibrary.does_asset_exist(MATERIAL):
        return
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    path, name = MATERIAL.rsplit("/", 1)
    mat = tools.create_asset(name, path, unreal.Material, unreal.MaterialFactoryNew())
    mel = unreal.MaterialEditingLibrary
    color = mel.create_material_expression(mat, unreal.MaterialExpressionPerInstanceCustomData3Vector, -400, 0)
    color.set_editor_property("data_index", 0)
    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 200)
    rough.set_editor_property("r", 0.85)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(MATERIAL)
    unreal.log("build_zone: created %s" % MATERIAL)


ensure_material()
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

zone.set_editor_property("is_spatially_loaded", False)
zone.set_editor_property("map_file", os.path.join(ZONES, ZONE, MAP_NAME).replace("\\", "/"))
zone.set_editor_property("zone", ZONE)
zone.rebuild_from_map()

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
