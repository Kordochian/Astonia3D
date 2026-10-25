"""Creates the two default MetaHuman characters (smooth body + face), MH_DefaultMale and MH_DefaultFemale,
in /Game/Characters/MetaHumans, and with --finish rigs, textures and assembles them.

- New characters get a body shape and average height. Existing ones are never reshaped, so edits made in
  MetaHuman Creator are kept.
- Skin tone and undergarments (Skin > Underwear) are set in the editor: committing skin settings from a
  commandlet crashes (the skin texture graph needs the editor's GPU).
- --finish uses Epic's MetaHuman cloud service (Auto-Rig "Joints only", Download Texture Sources), then
  assembles (Optimized, Medium) into /Game/MetaHumans/<name>. It needs an Epic sign-in done once in the
  editor (open the character there; it asks).

  UnrealEditor-Cmd.exe AstoniaUE5.uproject -run=pythonscript -script="<project>/Scripts/create_metahumans.py [--finish]"
"""
import sys
import unreal

FOLDER = "/Game/Characters/MetaHumans"
BUILD_FOLDER = "/Game/MetaHumans"

# name: (masculine/feminine: negative = masculine, height cm)
CHARACTERS = {
    "MH_DefaultMale": (-1.2, 178.0),
    "MH_DefaultFemale": (1.2, 166.0),
}

mh = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)
tools = unreal.AssetToolsHelpers.get_asset_tools()
finish = "--finish" in " ".join(sys.argv)


def load_or_create(name):
    """Returns (character, created)."""
    path = "%s/%s" % (FOLDER, name)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path), False
    character = tools.create_asset(asset_name=name, package_path=FOLDER, asset_class=unreal.MetaHumanCharacter,
                                   factory=unreal.new_object(type=unreal.MetaHumanCharacterFactoryNew))
    return character, True


def shape(name, character, gender, height):
    constraints = {str(c.name): c for c in mh.get_body_constraints(character)}
    constraints["Masculine/Feminine"].is_active = True
    constraints["Masculine/Feminine"].target_measurement = gender
    constraints["Height"].is_active = True
    constraints["Height"].target_measurement = height
    mh.set_body_constraints(character, list(constraints.values()))
    mh.commit_body_state(character)
    m = {str(c.name): round(c.target_measurement, 1) for c in mh.get_body_constraints(character)}
    unreal.log("create_metahumans: %s chest %s hip %s waist %s" % (name, m["Chest"], m["Hip"], m["Waist"]))


def complete(name, character):
    rig = unreal.MetaHumanCharacterAutoRiggingRequestParams()
    rig.blocking = True
    rig.report_progress = False
    rig.rig_type = unreal.MetaHumanRigType.JOINTS_ONLY
    mh.request_auto_rigging(character, rig)
    unreal.log("create_metahumans: %s auto-rig done" % name)

    tex = unreal.MetaHumanCharacterTextureRequestParams()
    tex.blocking = True
    tex.report_progress = False
    mh.request_texture_sources(character, tex)
    unreal.log("create_metahumans: %s textures done" % name)

    if not mh.can_build_meta_human(character, True):
        unreal.log_error("create_metahumans: %s cannot be assembled yet (see the error above)" % name)
        return
    params = unreal.MetaHumanCharacterEditorBuildParameters()
    params.pipeline_type = unreal.MetaHumanDefaultPipelineType.OPTIMIZED
    params.pipeline_quality = unreal.MetaHumanQualityLevel.MEDIUM
    params.absolute_build_path = "%s/%s" % (BUILD_FOLDER, name)
    params.common_folder_path = "%s/Common" % BUILD_FOLDER
    params.enable_wardrobe_item_validation = False
    mh.build_meta_human(character=character, params=params)
    unreal.log("create_metahumans: assembled %s into %s/%s" % (name, BUILD_FOLDER, name))


for name, (gender, height) in CHARACTERS.items():
    character, created = load_or_create(name)
    if not mh.try_add_object_to_edit(character):
        raise RuntimeError("%s is open in another editor" % name)
    try:
        if created:
            shape(name, character, gender, height)
        if finish:
            complete(name, character)
    finally:
        mh.remove_object_to_edit(character)
    unreal.EditorAssetLibrary.save_asset("%s/%s" % (FOLDER, name))
    unreal.log("create_metahumans: saved %s" % name)
