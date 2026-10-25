"""Builds the kit's tree and bush models in Blender and exports them to SourceArt/Meshes/SM_Kit_<Name>.fbx.

Stylised (WoW-like) trees: a tapering, slightly bent trunk with branches, and a crown of lumpy leaf
clusters. Two material slots, "Trunk" and "Leaves"; in Unreal they get MI_Kit_Trunk and MI_Kit_Leaves
(world-aligned textures, so the models need no UVs). Pivot at the foot of the trunk, metres in Blender.

Headless:  blender.exe -b --factory-startup --python Scripts/blender/make_trees.py
Then Scripts/import_meshes.py (in the editor) imports them as Nanite meshes.
"""
import math
import os
import random

import bmesh
import bpy
from mathutils import Matrix, Vector, noise

OUT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "SourceArt", "Meshes"))


def tube(bm, points, radii, seg, mat):
    """A tube through points (list of Vector) with a radius per point; faces get material index mat."""
    rings = []
    for i, p in enumerate(points):
        d = (points[min(i + 1, len(points) - 1)] - points[max(i - 1, 0)]).normalized()
        side = d.orthogonal().normalized()
        other = d.cross(side)
        ring = []
        for k in range(seg):
            a = 2 * math.pi * k / seg
            wobble = 1.0 + 0.12 * noise.noise(p * 3.0 + Vector((k, 0, 0)))
            ring.append(bm.verts.new(p + (side * math.cos(a) + other * math.sin(a)) * radii[i] * wobble))
        rings.append(ring)
    for r0, r1 in zip(rings, rings[1:]):
        for k in range(seg):
            f = bm.faces.new((r0[k], r0[(k + 1) % seg], r1[(k + 1) % seg], r1[k]))
            f.material_index = mat
            f.smooth = True
    return rings


def lumpy_ball(bm, center, radius, squash, seed, mat, subdiv=4):
    """An icosphere pushed in and out by fractal noise: one leaf cluster."""
    tmp = bmesh.new()
    bmesh.ops.create_icosphere(tmp, subdivisions=subdiv, radius=1.0)
    off = Vector((seed * 7.1, seed * 3.3, seed * 1.7))
    for v in tmp.verts:
        n = v.co.normalized()
        lump = noise.fractal(n * 1.3 + off, 0.55, 2.0, 3)
        fine = noise.noise(n * 7.0 + off)
        v.co = n * (1.0 + 0.2 * lump + 0.03 * fine)
        v.co.x *= radius
        v.co.y *= radius
        v.co.z *= radius * squash
        v.co += center
    mesh = bpy.data.meshes.new("tmp")
    tmp.to_mesh(mesh)
    tmp.free()
    start = len(bm.faces)
    bm.from_mesh(mesh)
    bpy.data.meshes.remove(mesh)
    bm.faces.ensure_lookup_table()
    for f in bm.faces[start:]:
        f.material_index = mat
        f.smooth = True


def bent_line(start, direction, length, steps, bend, rnd):
    pts = [start.copy()]
    d = direction.normalized()
    for i in range(steps):
        d = (d + Vector((rnd.uniform(-bend, bend), rnd.uniform(-bend, bend), rnd.uniform(-bend * 0.3, bend * 0.5)))).normalized()
        pts.append(pts[-1] + d * (length / steps))
    return pts


def tree(name, seed, height, trunk_r, crown_r, clusters, squash=0.85, branches=4, spread=0.9):
    rnd = random.Random(seed)
    bm = bmesh.new()
    trunk = bent_line(Vector((0, 0, -0.2)), Vector((0, 0, 1)), height + 0.2, 7, 0.06, rnd)
    radii = [trunk_r * (1.25 if i == 0 else 1.0 - 0.55 * i / (len(trunk) - 1)) for i in range(len(trunk))]
    tube(bm, trunk, radii, 10, 0)
    top = trunk[-1]
    ends = []
    for b in range(branches):
        a = 2 * math.pi * (b + rnd.uniform(-0.2, 0.2)) / branches
        start_i = rnd.randrange(len(trunk) // 2, len(trunk) - 1)
        start = trunk[start_i]
        d = Vector((math.cos(a), math.sin(a), rnd.uniform(0.6, 1.1)))
        line = bent_line(start, d, crown_r * rnd.uniform(0.6, 0.95), 4, 0.15, rnd)
        r = radii[start_i] * 0.55
        tube(bm, line, [r * (1 - 0.7 * i / (len(line) - 1)) for i in range(len(line))], 7, 0)
        ends.append(line[-1])
    # Leaf clusters: one big crown over the top, smaller clumps round its shell (a cloud-like outline)
    # and one on each branch end.
    center = top + Vector((0, 0, crown_r * 0.35))
    lumpy_ball(bm, center, crown_r * 0.92, squash, seed, 1, subdiv=5)
    for i in range(clusters):
        a = 2 * math.pi * (i + rnd.uniform(-0.25, 0.25)) / clusters
        up = rnd.uniform(-0.25, 0.75)
        d = Vector((math.cos(a), math.sin(a), up)).normalized()
        c = center + Vector((d.x * crown_r * spread, d.y * crown_r * spread, d.z * crown_r * squash * 0.8))
        lumpy_ball(bm, c, crown_r * rnd.uniform(0.42, 0.6), squash, seed * 13 + i, 1, subdiv=4)
    for i, e in enumerate(ends):
        lumpy_ball(bm, e + Vector((0, 0, crown_r * 0.15)), crown_r * rnd.uniform(0.35, 0.5), squash, seed * 29 + i, 1, subdiv=4)
    export(name, bm)


def bush(name, seed, radius):
    rnd = random.Random(seed)
    bm = bmesh.new()
    lumpy_ball(bm, Vector((0, 0, radius * 0.45)), radius, 0.7, seed, 1)
    for i in range(4):
        a = 2 * math.pi * i / 4 + rnd.uniform(-0.4, 0.4)
        c = Vector((math.cos(a) * radius * 0.6, math.sin(a) * radius * 0.6, radius * 0.35))
        lumpy_ball(bm, c, radius * rnd.uniform(0.5, 0.7), 0.75, seed * 7 + i, 1, subdiv=3)
    # A short stem so the bush has the trunk slot too.
    tube(bm, [Vector((0, 0, -0.1)), Vector((0, 0, radius * 0.5))], [radius * 0.12, radius * 0.08], 6, 0)
    export(name, bm)


def export(name, bm):
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    for slot in ("Trunk", "Leaves"):
        mat = bpy.data.materials.get(slot) or bpy.data.materials.new(slot)
        mesh.materials.append(mat)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    for o in bpy.context.scene.objects:
        o.select_set(o == obj)
    bpy.context.view_layer.objects.active = obj
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, name + ".fbx")
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                             axis_forward="-Y", axis_up="Z", mesh_smooth_type="FACE", add_leaf_bones=False, bake_anim=False,
                             use_mesh_modifiers=True)
    print("make_trees: %s (%d tris)" % (path, sum(len(p.vertices) - 2 for p in mesh.polygons)))
    bpy.data.objects.remove(obj)


def main():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o)
    tree("SM_Kit_TreeRound", 11, height=2.8, trunk_r=0.22, crown_r=1.6, clusters=8)
    tree("SM_Kit_TreeTall", 23, height=4.2, trunk_r=0.24, crown_r=1.4, clusters=9, squash=1.3, branches=5, spread=0.7)
    tree("SM_Kit_TreeForest", 37, height=4.6, trunk_r=0.32, crown_r=2.1, clusters=10, squash=0.9, branches=5)
    bush("SM_Kit_Bush", 41, radius=0.75)


main()
