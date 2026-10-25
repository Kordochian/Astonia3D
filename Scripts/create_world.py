"""Creates /Game/Maps/L_World: a World Partition open world from the engine's OpenWorld template
(landscape, sky, sun, fog) with a PlayerStart. Re-running it leaves an existing map alone.

Headless:
  UnrealEditor-Cmd.exe AstoniaUE5.uproject -run=pythonscript -script="<path>/Scripts/create_world.py"
"""
import unreal

MAP_PATH = "/Game/Maps/L_World"
TEMPLATE = "/Engine/Maps/Templates/OpenWorld"

if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
    unreal.log("create_world: %s already exists, nothing to do" % MAP_PATH)
else:
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.new_level_from_template(MAP_PATH, TEMPLATE):
        raise RuntimeError("create_world: could not create %s from %s" % (MAP_PATH, TEMPLATE))

    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0.0, 0.0, 300.0))
    start.set_actor_label("PlayerStart")

    levels.save_current_level()
    unreal.log("create_world: created %s" % MAP_PATH)
