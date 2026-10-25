"""Binds every MetaHuman groom (hair, eyebrows, beards, moustaches from MetaHuman Creator Core Data) to the
default MetaHuman faces, so the character creator can swap styles at runtime and they follow the face.
Bindings go to /Game/MetaHumans/Bindings/<character>/<style>. Existing bindings are kept (--rebuild redoes them).

  UnrealEditor-Cmd.exe AstoniaUE5.uproject -run=pythonscript -script="<project>/Scripts/create_groom_bindings.py [--rebuild]"
"""
import os
import sys
import unreal

GROOMS = "/MetaHumanCharacter/Optional/Grooms/GroomAssets"
KINDS = ["Hair", "Eyebrows", "Beards", "Mustaches"]
# Each groom is authored on one of several reference heads; Core Data ships a binding per groom
# (Grooms/Bindings/<kind>/<style>_Binding) whose source mesh names that head. Like MetaHuman's own
# assembly, our bindings transfer the groom from that head onto each character's face.
BINDINGS = "/MetaHumanCharacter/Optional/Grooms/Bindings"
FALLBACK_HEAD = "/MetaHumanCharacter/Optional/Grooms/GroomMesh/MH_Groom_Head.MH_Groom_Head"
FACES = {
    "MH_DefaultMale": "/Game/MetaHumans/MH_DefaultMale/MH_DefaultMale/Face/SKM_MH_DefaultMale_FaceMesh",
    "MH_DefaultFemale": "/Game/MetaHumans/MH_DefaultFemale/MH_DefaultFemale/Face/SKM_MH_DefaultFemale_FaceMesh",
}
CONTENT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir())
rebuild = "--rebuild" in " ".join(sys.argv)
only = next((a.split("=", 1)[1] for a in " ".join(sys.argv).split() if a.startswith("--only=")), None)

# The Core Data folder is not in the asset registry of a commandlet, so list the style folders on disk.
GROOMS_DIR = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.engine_dir()),
                          "Plugins/MetaHuman/MetaHumanCharacter/Content/Optional/Grooms/GroomAssets")
fallback = unreal.load_asset(FALLBACK_HEAD)


def authoring_head(kind, style):
    shipped = unreal.load_asset("%s/%s/%s_Binding" % (BINDINGS, kind, style))
    head = shipped.get_editor_property("source_skeletal_mesh") if shipped else None
    return head or fallback


made = skipped = failed = 0
for character, face_path in FACES.items():
    if only and character != only:
        continue
    face = unreal.load_asset(face_path)
    for kind in KINDS:
        for style in sorted(os.listdir(os.path.join(GROOMS_DIR, kind))):
            target = "/Game/MetaHumans/Bindings/%s/%s" % (character, style)
            if unreal.EditorAssetLibrary.does_asset_exist(target):
                if not rebuild:
                    skipped += 1
                    continue
                # Creating over an existing binding makes "<name>1" instead, so remove the file first.
                path = os.path.join(CONTENT_DIR, target[len("/Game/"):] + ".uasset")
                if os.path.exists(path):
                    os.remove(path)
            groom = unreal.load_asset("%s/%s/%s/%s" % (GROOMS, kind, style, style))
            if not groom:
                failed += 1
                continue
            binding = unreal.GroomLibrary.create_new_groom_binding_asset_with_path(target, groom, face, 100, authoring_head(kind, style), 0)
            if binding:
                unreal.EditorAssetLibrary.save_asset(target)
                made += 1
            else:
                failed += 1
                unreal.log_warning("create_groom_bindings: could not bind %s to %s" % (style, character))
    unreal.log("create_groom_bindings: %s done" % character)
unreal.log("create_groom_bindings: %d made, %d kept, %d failed" % (made, skipped, failed))
