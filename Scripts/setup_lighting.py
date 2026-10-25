"""Look of the open world in L_World: an unbound post-process volume (grading, bloom, Lumen quality,
vignette) and tuned sun, sky light and fog. Kept cheap: no volumetric fog, soft but unshadowed sky.

  UnrealEditor-Cmd.exe AstoniaUE5.uproject -run=pythonscript -script="<project>/Scripts/setup_lighting.py"
Re-running updates the same actors.
"""
import unreal

WORLD = "/Game/Maps/L_World"
LABEL = "PPV_World"

unreal.EditorLoadingAndSavingUtils.load_map(WORLD)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = actors.get_all_level_actors()

vol = next((a for a in all_actors if a.get_actor_label() == LABEL), None)
if vol is None:
    vol = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    vol.set_actor_label(LABEL)
    vol.set_folder_path("Environment")
vol.set_editor_property("is_spatially_loaded", False)
vol.set_editor_property("unbound", True)
vol.set_editor_property("priority", -1.0)   # local volumes (the Thieves' Guild exposure) win
s = vol.get_editor_property("settings")


def setp(name, value):
    s.set_editor_property("override_" + name, True)
    s.set_editor_property(name, value)


setp("bloom_intensity", 0.45)
setp("auto_exposure_bias", 0.3)
setp("vignette_intensity", 0.28)
setp("color_saturation", unreal.Vector4(1.0, 1.0, 1.0, 1.0))
setp("color_contrast", unreal.Vector4(1.06, 1.06, 1.06, 1.0))
setp("color_gain_highlights", unreal.Vector4(1.02, 1.0, 0.96, 1.0))
setp("color_gamma_shadows", unreal.Vector4(0.98, 0.99, 1.03, 1.0))
setp("lumen_final_gather_quality", 1.5)
setp("lumen_scene_detail", 1.5)
setp("lumen_reflection_quality", 1.0)
setp("ambient_occlusion_intensity", 0.55)
vol.set_editor_property("settings", s)

for a in all_actors:
    if isinstance(a, unreal.DirectionalLight):
        lc = a.light_component
        lc.set_editor_property("intensity", 9.0)
        lc.set_editor_property("light_source_angle", 1.2)
        lc.set_editor_property("light_color", unreal.Color(r=255, g=244, b=228, a=255))
        a.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=-38.0, yaw=128.0), False)
    elif isinstance(a, unreal.ExponentialHeightFog):
        fc = a.component
        fc.set_editor_property("fog_density", 0.012)
        fc.set_editor_property("fog_height_falloff", 0.25)
        fc.set_editor_property("start_distance", 3000.0)
        fc.set_editor_property("enable_volumetric_fog", False)
    elif isinstance(a, unreal.SkyLight):
        a.light_component.set_editor_property("intensity", 1.1)

unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log("setup_lighting: done")
