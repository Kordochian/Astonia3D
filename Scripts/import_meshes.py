"""Imports the Blender kit models (SourceArt/Meshes/SM_Kit_*.fbx, made by Scripts/blender/*.py) into
/Game/Astonia/Meshes as Nanite static meshes without collision (the builders add Blocker pieces where
something must block). Material slots keep their Blender names (Trunk, Leaves, ...); AstoniaBuilding
gives each slot MI_Kit_<SlotName>, so no materials are imported.

  UnrealEditor-Cmd.exe AstoniaUE5.uproject -run=pythonscript -script="<project>/Scripts/import_meshes.py"
"""
import glob
import os
import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE = os.path.join(PROJECT, "SourceArt", "Meshes")
DEST = "/Game/Astonia/Meshes"
eal = unreal.EditorAssetLibrary

tasks = []
for path in sorted(glob.glob(os.path.join(SOURCE, "SM_Kit_*.fbx"))):
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", path)
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", False)
    opts = unreal.FbxImportUI()
    opts.set_editor_property("import_mesh", True)
    opts.set_editor_property("import_materials", False)
    opts.set_editor_property("import_textures", False)
    opts.set_editor_property("import_as_skeletal", False)
    smd = opts.get_editor_property("static_mesh_import_data")
    smd.set_editor_property("combine_meshes", True)
    smd.set_editor_property("auto_generate_collision", False)
    smd.set_editor_property("generate_lightmap_u_vs", False)
    t.set_editor_property("options", opts)
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

for t in tasks:
    name = os.path.splitext(os.path.basename(t.get_editor_property("filename")))[0]
    path = "%s/%s" % (DEST, name)
    mesh = unreal.load_asset(path)
    if mesh is None:
        unreal.log_error("import_meshes: %s did not import" % name)
        continue
    nanite = mesh.get_editor_property("nanite_settings")
    nanite.set_editor_property("enabled", True)
    mesh.set_editor_property("nanite_settings", nanite)
    body = mesh.get_editor_property("body_setup")
    if body:
        body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_DEFAULT)
    eal.save_asset(path)
    b = mesh.get_bounding_box()
    unreal.log("import_meshes: %s, %d materials, bounds %s .. %s" % (name, len(mesh.get_editor_property("static_materials")), b.min, b.max))

# Drop any material assets an importer made anyway (slots are filled by name at build time).
for asset in eal.list_assets(DEST, recursive=False):
    obj = unreal.load_asset(asset)
    if isinstance(obj, unreal.MaterialInterface):
        eal.delete_asset(asset)
