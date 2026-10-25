"""Creates /Game/Maps/L_World: an empty World Partition open world (no template terrain) with sky,
sun, sky light, fog, clouds and a PlayerStart. The terrain is built by us zone by zone
(Scripts/build_zone.py). Re-running leaves an existing map alone.

Headless:
  UnrealEditor-Cmd.exe AstoniaUE5.uproject -run=pythonscript -script="<path>/Scripts/create_world.py"
"""
import unreal

MAP_PATH = "/Game/Maps/L_World"

if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
    unreal.log("create_world: %s already exists, nothing to do" % MAP_PATH)
else:
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.new_level(MAP_PATH, True):
        raise RuntimeError("create_world: could not create %s" % MAP_PATH)

    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    def spawn(cls, label, loc=(0.0, 0.0, 0.0), rot=(0.0, 0.0, 0.0)):
        a = actors.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(*rot))
        a.set_actor_label(label)
        a.set_folder_path("Environment")
        a.set_editor_property("is_spatially_loaded", False)
        return a

    # Rotator(roll, pitch, yaw): late-morning sun from the south-east.
    sun = spawn(unreal.DirectionalLight, "Sun", rot=(0.0, -40.0, 135.0))
    sun.light_component.set_editor_property("atmosphere_sun_light", True)
    sun.light_component.set_editor_property("intensity", 8.0)
    spawn(unreal.SkyAtmosphere, "SkyAtmosphere")
    sky = spawn(unreal.SkyLight, "SkyLight")
    sky.light_component.set_editor_property("real_time_capture", True)
    spawn(unreal.ExponentialHeightFog, "HeightFog")
    spawn(unreal.VolumetricCloud, "Clouds")
    spawn(unreal.PlayerStart, "PlayerStart", loc=(0.0, 0.0, 300.0))

    levels.save_current_level()
    unreal.log("create_world: created %s" % MAP_PATH)
