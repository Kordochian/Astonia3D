"""Renders the exported kit meshes (SourceArt/Meshes/*.fbx) side by side to SourceArt/Meshes/preview.png
with the Workbench engine, to check shapes before importing into Unreal.

Headless:  blender.exe -b --factory-startup --python Scripts/blender/preview.py [-- name_filter]
"""
import glob
import math
import os
import sys

import bpy
from mathutils import Vector

MESHES = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "SourceArt", "Meshes"))
flt = sys.argv[sys.argv.index("--") + 1] if "--" in sys.argv else ""

for o in list(bpy.data.objects):
    bpy.data.objects.remove(o)
x = 0.0
objs = []
for path in sorted(glob.glob(os.path.join(MESHES, "*.fbx"))):
    if flt not in os.path.basename(path):
        continue
    bpy.ops.import_scene.fbx(filepath=path)
    for o in bpy.context.selected_objects:
        w = max(o.dimensions.x, o.dimensions.y)
        o.location.x = x + w / 2
        x += w + 1.0
        objs.append(o)
cols = {"Trunk": (0.30, 0.20, 0.12, 1), "Leaves": (0.20, 0.42, 0.14, 1), "Wall": (0.55, 0.53, 0.5, 1), "Trim": (0.75, 0.72, 0.66, 1),
        "Wood": (0.35, 0.22, 0.12, 1), "Roof": (0.55, 0.22, 0.14, 1), "Plaster": (0.85, 0.8, 0.7, 1),
        "Metal": (0.18, 0.18, 0.2, 1), "Brass": (0.8, 0.6, 0.2, 1)}
for m in bpy.data.materials:
    base = m.name.split(".")[0]
    m.diffuse_color = cols.get(base, (0.6, 0.58, 0.55, 1))
scene = bpy.context.scene
scene.render.engine = "BLENDER_WORKBENCH"
scene.display.shading.color_type = "MATERIAL"
scene.display.shading.light = "STUDIO"
scene.display.shading.show_cavity = True
scene.render.resolution_x, scene.render.resolution_y = 1600, 700
height = max((o.dimensions.z for o in objs), default=5)
cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
scene.collection.objects.link(cam)
cam.location = Vector((x / 2, -max(x, height * 2.2) * 0.95, height * 0.6))
cam.rotation_euler = (math.radians(86), 0, 0)
cam.data.lens = 35
scene.camera = cam
scene.render.filepath = os.path.join(MESHES, "preview.png")
bpy.ops.render.render(write_still=True)
print("preview: %s" % scene.render.filepath)
