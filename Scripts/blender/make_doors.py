"""Builds the door kit in Blender and exports SourceArt/Meshes/SM_Kit_<Piece>.fbx: four styles, each a door
leaf and a frame that fits one 2 m map tile between two wall pieces.

- Rickety: grey weathered planks with gaps, uneven ends, crooked battens; rough-hewn posts and lintel.
- Plank: tight boards on a Z brace, iron strap hinges and a ring pull; squared timber frame.
- Fancy: arched, panelled door with brass studs, handle and knocker; stone jambs and an arch with a keystone.
- Metal: riveted iron plates with cross bands and a heavy bolt; stone jambs, lintel and an iron band frame.

Frames: tile centre on the ground, 0.6 m deep like the walls, opening 1.5 m wide along X (you walk
through along Y), jambs up to
the frame top (FRAME_TOP below; the builder fills the wall above it). Leaves: pivot at the hinge
(x = 0, bottom), closed leaf spans +X to 1.5 m, thickness along Y centred on 0. Slots by name: Wood, Metal,
Brass, Trim (smooth stone). Metres.

Headless:  blender.exe -b --factory-startup --python Scripts/blender/make_doors.py
"""
import math
import os
import random

import bmesh
import bpy
from mathutils import Matrix, Vector

OUT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "SourceArt", "Meshes"))
SLOTS = ["Wood", "Metal", "Brass", "Trim"]
W = 1.5            # opening width
FRAME_TOP = {"Rickety": 2.65, "Plank": 2.85, "Fancy": 3.45, "Metal": 3.05}   # keep in step with build_cameron_3d


def add(bm, verts_faces, mat, smooth=False):
    for f in verts_faces:
        if f.is_valid:
            f.material_index = SLOTS.index(mat)
            f.smooth = smooth


def box(bm, lo, hi, mat, rot=None, pivot=None, bevel=0.0):
    geom = bmesh.ops.create_cube(bm, size=1.0)
    verts = geom["verts"]
    bmesh.ops.scale(bm, vec=Vector(hi) - Vector(lo), verts=verts)
    bmesh.ops.translate(bm, vec=(Vector(hi) + Vector(lo)) / 2, verts=verts)
    faces = list({f for v in verts for f in v.link_faces})
    if bevel > 0:
        edges = list({e for f in faces for e in f.edges})
        res = bmesh.ops.bevel(bm, geom=edges + verts, offset=bevel, segments=1, affect="EDGES")
        faces = list(set(faces) | set(res["faces"]) | {f for v in res.get("verts", []) for f in v.link_faces})
        verts = list({v for f in faces if f.is_valid for v in f.verts})
    if rot is not None:
        c = Vector(pivot) if pivot is not None else (Vector(hi) + Vector(lo)) / 2
        bmesh.ops.rotate(bm, cent=c, matrix=rot, verts=verts)
    add(bm, faces, mat)
    return verts


def cylinder(bm, center, radius, depth, axis, mat, seg=10):
    geom = bmesh.ops.create_cone(bm, cap_ends=True, segments=seg, radius1=radius, radius2=radius, depth=depth)
    verts = geom["verts"]
    if axis == "y":
        bmesh.ops.rotate(bm, cent=Vector(), matrix=Matrix.Rotation(math.radians(90), 3, "X"), verts=verts)
    elif axis == "x":
        bmesh.ops.rotate(bm, cent=Vector(), matrix=Matrix.Rotation(math.radians(90), 3, "Y"), verts=verts)
    bmesh.ops.translate(bm, vec=Vector(center), verts=verts)
    add(bm, list({f for v in verts for f in v.link_faces}), mat, smooth=True)


def sphere(bm, center, radius, mat):
    geom = bmesh.ops.create_uvsphere(bm, u_segments=8, v_segments=5, radius=radius)
    bmesh.ops.translate(bm, vec=Vector(center), verts=geom["verts"])
    add(bm, list({f for v in geom["verts"] for f in v.link_faces}), mat, smooth=True)


def slab(bm, outline, y0, y1, mat):
    """Extrudes a polygon outline [(x, z)] between y0 and y1."""
    front = [bm.verts.new((x, y0, z)) for x, z in outline]
    back = [bm.verts.new((x, y1, z)) for x, z in outline]
    faces = [bm.faces.new(front), bm.faces.new(list(reversed(back)))]
    n = len(outline)
    for i in range(n):
        faces.append(bm.faces.new((front[i], back[i], back[(i + 1) % n], front[(i + 1) % n])))
    add(bm, faces, mat)


def arch_outline(x0, x1, z_spring, segments=12, z0=0.0):
    r = (x1 - x0) / 2
    cx = (x0 + x1) / 2
    pts = [(x0, z0), (x1, z0)]
    for i in range(segments + 1):
        a = math.pi * i / segments
        pts.append((cx + r * math.cos(a), z_spring + r * math.sin(a)))
    return pts


# --- leaves (hinge at x = 0) ---
def door_rickety():
    rnd = random.Random(3)
    bm = bmesh.new()
    H = FRAME_TOP["Rickety"] - 0.2
    n = 5
    pw = W / n
    for i in range(n):
        x0 = i * pw + 0.012
        x1 = (i + 1) * pw - rnd.uniform(0.02, 0.05)
        top = H - rnd.uniform(0.0, 0.14)
        bottom = rnd.uniform(0.02, 0.09)
        tilt = Matrix.Rotation(math.radians(rnd.uniform(-1.2, 1.2)), 3, "Y")
        box(bm, (x0, -0.04, bottom), (x1, 0.04, top), "Wood", rot=tilt)
    for z, a in ((0.45, 2.5), (H - 0.5, -2.0)):
        box(bm, (0.05, 0.04, z - 0.08), (W - 0.04, 0.09, z + 0.08), "Wood", rot=Matrix.Rotation(math.radians(a), 3, "Y"))
    brace = Matrix.Rotation(math.atan2(H - 1.1, W - 0.2), 3, "Y")
    length = math.hypot(W - 0.2, H - 1.1)
    box(bm, (W / 2 - length / 2, 0.045, 0.5 * H - 0.07), (W / 2 + length / 2, 0.085, 0.5 * H + 0.07), "Wood", rot=brace)
    for z in (0.45, H - 0.5):
        for x in (0.15, W - 0.2):
            cylinder(bm, (x, 0.1, z), 0.012, 0.02, "y", "Metal", seg=6)   # nail heads
    export("SM_Kit_DoorRickety", bm)


def door_plank():
    bm = bmesh.new()
    H = FRAME_TOP["Plank"] - 0.2
    n = 7
    pw = W / n
    for i in range(n):
        box(bm, (i * pw + 0.004, -0.05, 0.02), ((i + 1) * pw - 0.004, 0.05, H), "Wood", bevel=0.006)
    for z in (0.4, H / 2, H - 0.4):
        box(bm, (0.06, 0.05, z - 0.09), (W - 0.06, 0.1, z + 0.09), "Wood", bevel=0.008)
    for (za, zb) in ((0.4, H / 2), (H / 2, H - 0.4)):
        ang = math.atan2(zb - za, W - 0.3)
        length = math.hypot(W - 0.3, zb - za)
        box(bm, (W / 2 - length / 2, 0.055, (za + zb) / 2 - 0.07), (W / 2 + length / 2, 0.095, (za + zb) / 2 + 0.07), "Wood",
            rot=Matrix.Rotation(-ang, 3, "Y"))
    for z in (0.55, H - 0.55):   # strap hinges on the front
        box(bm, (0.0, -0.065, z - 0.04), (W * 0.65, -0.05, z + 0.04), "Metal")
        for k in range(5):
            cylinder(bm, (0.1 + k * 0.18, -0.07, z), 0.014, 0.012, "y", "Metal", seg=6)
        cylinder(bm, (0.0, 0.0, z), 0.03, 0.18, "z", "Metal")
    cylinder(bm, (W - 0.22, -0.07, 1.05), 0.05, 0.02, "y", "Metal")   # ring pull and plate
    ring = bmesh.ops.create_cone(bm, cap_ends=False, segments=12, radius1=0.07, radius2=0.07, depth=0.015)
    bmesh.ops.rotate(bm, cent=Vector(), matrix=Matrix.Rotation(math.radians(90), 3, "X"), verts=ring["verts"])
    bmesh.ops.translate(bm, vec=Vector((W - 0.22, -0.085, 0.98)), verts=ring["verts"])
    add(bm, list({f for v in ring["verts"] for f in v.link_faces}), "Metal")
    export("SM_Kit_DoorPlank", bm)


def door_fancy():
    bm = bmesh.new()
    spring = FRAME_TOP["Fancy"] - 0.2 - W / 2
    slab(bm, arch_outline(0.0, W, spring), -0.06, 0.06, "Wood")
    # Raised panels (two tall, two short) and an arched top panel on both faces.
    for s in (-1, 1):
        y0, y1 = (s * 0.06, s * 0.085) if s > 0 else (s * 0.085, s * 0.06)
        for (x0, x1) in ((0.14, W / 2 - 0.06), (W / 2 + 0.06, W - 0.14)):
            box(bm, (x0, y0, 0.25), (x1, y1, 0.95), "Wood", bevel=0.012)
            box(bm, (x0, y0, 1.1), (x1, y1, spring - 0.05), "Wood", bevel=0.012)
        top = arch_outline(0.2, W - 0.2, spring + 0.02, z0=spring + 0.02)
        slab(bm, top, y0, y1, "Wood")
        for x in [0.08 + k * (W - 0.16) / 6 for k in range(7)]:
            for z in (0.12, 1.02, spring - 0.0):
                sphere(bm, (x, s * 0.09, z), 0.016, "Brass")
    cylinder(bm, (W - 0.2, -0.1, 1.05), 0.03, 0.06, "y", "Brass")          # handle
    box(bm, (W - 0.24, -0.075, 0.95), (W - 0.16, -0.065, 1.2), "Brass")    # escutcheon
    knocker = bmesh.ops.create_cone(bm, cap_ends=False, segments=14, radius1=0.09, radius2=0.09, depth=0.02)
    bmesh.ops.rotate(bm, cent=Vector(), matrix=Matrix.Rotation(math.radians(90), 3, "X"), verts=knocker["verts"])
    bmesh.ops.translate(bm, vec=Vector((W / 2, -0.1, 1.55)), verts=knocker["verts"])
    add(bm, list({f for v in knocker["verts"] for f in v.link_faces}), "Brass")
    for z in (0.5, spring - 0.3):
        cylinder(bm, (0.0, 0.0, z), 0.028, 0.2, "z", "Brass")
    export("SM_Kit_DoorFancy", bm)


def door_metal():
    bm = bmesh.new()
    H = FRAME_TOP["Metal"] - 0.2
    box(bm, (0.0, -0.05, 0.02), (W, 0.05, H), "Metal", bevel=0.01)
    for s in (-1, 1):
        y0, y1 = (s * 0.05, s * 0.07) if s > 0 else (s * 0.07, s * 0.05)
        for z in (0.25, H / 2, H - 0.25):
            box(bm, (0.02, y0, z - 0.07), (W - 0.02, y1, z + 0.07), "Metal")
            for k in range(8):
                sphere(bm, (0.08 + k * (W - 0.16) / 7, s * 0.075, z), 0.018, "Metal")
        for x in (0.06, W - 0.06):
            box(bm, (x - 0.06, y0, 0.05), (x + 0.06, y1, H - 0.05), "Metal")
        # Diagonal cross bands.
        for a in (1, -1):
            ang = math.atan2(H - 0.6, W - 0.2) * a
            length = math.hypot(W - 0.2, H - 0.6)
            box(bm, (W / 2 - length / 2, y0, H / 2 - 0.05), (W / 2 + length / 2, y1, H / 2 + 0.05), "Metal",
                rot=Matrix.Rotation(ang, 3, "Y"))
    box(bm, (W - 0.45, -0.11, 1.05), (W - 0.05, -0.07, 1.13), "Metal")     # bolt
    cylinder(bm, (W - 0.4, -0.13, 1.09), 0.03, 0.05, "y", "Metal")
    for z in (0.4, H - 0.4):
        cylinder(bm, (0.0, 0.0, z), 0.04, 0.24, "z", "Metal")
    export("SM_Kit_DoorMetal", bm)


# --- frames (tile centre, opening along X) ---
def jamb_boxes(bm, top, mat, depth=0.3, bevel=0.0):
    for x0, x1 in ((-1.0, -W / 2), (W / 2, 1.0)):
        box(bm, (x0, -depth, 0.0), (x1, depth, top), mat, bevel=bevel)


def frame_rickety():
    rnd = random.Random(5)
    bm = bmesh.new()
    top = FRAME_TOP["Rickety"]
    for x in (-W / 2 - 0.1, W / 2 + 0.1):
        tilt = Matrix.Rotation(math.radians(rnd.uniform(-2, 2)), 3, "Y")
        box(bm, (x - 0.12, -0.3, 0.0), (x + 0.12, 0.3, top - 0.1), "Wood", rot=tilt)
    for x0, x1 in ((-1.0, -W / 2 - 0.22), (W / 2 + 0.22, 1.0)):      # filler to the wall pieces
        box(bm, (x0, -0.25, 0.0), (x1, 0.25, top), "Wood")
    box(bm, (-1.05, -0.32, top - 0.22), (1.05, 0.32, top), "Wood", rot=Matrix.Rotation(math.radians(1.5), 3, "Y"))
    export("SM_Kit_FrameRickety", bm)


def frame_plank():
    bm = bmesh.new()
    top = FRAME_TOP["Plank"]
    jamb_boxes(bm, top, "Wood", bevel=0.02)
    box(bm, (-1.0, -0.32, top - 0.3), (1.0, 0.32, top), "Wood", bevel=0.02)
    for s in (-1, 1):   # stop moulding round the opening
        y0, y1 = sorted((s * 0.3, s * 0.34))
        box(bm, (-W / 2 - 0.1, y0, 0.0), (-W / 2, y1, top - 0.3), "Wood")
        box(bm, (W / 2, y0, 0.0), (W / 2 + 0.1, y1, top - 0.3), "Wood")
        box(bm, (-W / 2 - 0.1, y0, top - 0.4), (W / 2 + 0.1, y1, top - 0.3), "Wood")
    export("SM_Kit_FramePlank", bm)


def frame_fancy():
    bm = bmesh.new()
    top = FRAME_TOP["Fancy"]
    spring = top - 0.2 - W / 2
    # Stone surround: jambs to the springing line, then a solid block with the arch cut out (voussoirs).
    jamb_boxes(bm, spring, "Trim", bevel=0.015)
    seg = 9
    r_in, r_out = W / 2, W / 2 + 0.25
    for i in range(seg):
        a0, a1 = math.pi * i / seg, math.pi * (i + 1) / seg
        pts = [(r_in * math.cos(a0), spring + r_in * math.sin(a0)), (r_out * math.cos(a0), spring + r_out * math.sin(a0)),
               (r_out * math.cos(a1), spring + r_out * math.sin(a1)), (r_in * math.cos(a1), spring + r_in * math.sin(a1))]
        slab(bm, list(reversed(pts)), -0.32, 0.32, "Trim")
    # Spandrels: fill between the arch and the frame top / tile edges.
    # Spandrels: thin vertical strips from the arch's outer edge up to the frame top.
    strips = 16
    for i in range(strips):
        x0, x1 = -1.0 + 2.0 * i / strips, -1.0 + 2.0 * (i + 1) / strips
        xm = max(abs(x0), abs(x1)) if x0 * x1 > 0 else 0.0
        z0 = spring + math.sqrt(max(r_out * r_out - xm * xm, 0.0)) - 0.03 if xm < r_out else spring
        if z0 < top:
            box(bm, (x0, -0.28, z0), (x1, 0.28, top), "Trim")
    box(bm, (-0.12, -0.36, top - 0.42), (0.12, 0.36, top), "Trim", bevel=0.02)    # keystone
    for x in (-W / 2 - 0.25, W / 2):   # plinths and imposts
        box(bm, (x, -0.34, 0.0), (x + 0.25, 0.34, 0.3), "Trim", bevel=0.02)
        box(bm, (x - 0.03, -0.34, spring - 0.12), (x + 0.28, 0.34, spring), "Trim", bevel=0.02)
    export("SM_Kit_FrameFancy", bm)


def frame_metal():
    bm = bmesh.new()
    top = FRAME_TOP["Metal"]
    jamb_boxes(bm, top, "Trim", bevel=0.02)
    box(bm, (-1.02, -0.33, top - 0.4), (1.02, 0.33, top), "Trim", bevel=0.03)      # lintel stone
    for s in (-1, 1):
        y0, y1 = sorted((s * 0.3, s * 0.35))
        box(bm, (-W / 2 - 0.08, y0, 0.0), (-W / 2, y1, top - 0.4), "Metal")
        box(bm, (W / 2, y0, 0.0), (W / 2 + 0.08, y1, top - 0.4), "Metal")
        box(bm, (-W / 2 - 0.08, y0, top - 0.48), (W / 2 + 0.08, y1, top - 0.4), "Metal")
        for z in (0.4, 1.2, 2.0):
            for x in (-W / 2 - 0.04, W / 2 + 0.04):
                sphere(bm, (x, s * 0.36, z), 0.022, "Metal")
    export("SM_Kit_FrameMetal", bm)


def export(name, bm):
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.0005)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    used = sorted({p.material_index for p in mesh.polygons})
    remap = {old: new for new, old in enumerate(used)}
    for p in mesh.polygons:
        p.material_index = remap[p.material_index]
    for i in used:
        slot = SLOTS[i]
        mesh.materials.append(bpy.data.materials.get(slot) or bpy.data.materials.new(slot))
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    for o in bpy.context.scene.objects:
        o.select_set(o == obj)
    bpy.context.view_layer.objects.active = obj
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, name + ".fbx")
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                             axis_forward="-Y", axis_up="Z", mesh_smooth_type="FACE", add_leaf_bones=False, bake_anim=False)
    print("make_doors: %s (%d tris, slots %s)" % (name, sum(len(p.vertices) - 2 for p in mesh.polygons), [m.name for m in mesh.materials]))
    bpy.data.objects.remove(obj)


def main():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o)
    door_rickety()
    door_plank()
    door_fancy()
    door_metal()
    frame_rickety()
    frame_plank()
    frame_fancy()
    frame_metal()


main()
