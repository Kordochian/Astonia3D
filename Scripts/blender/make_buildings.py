"""Builds the modular building kit in Blender and exports SourceArt/Meshes/SM_Kit_<Piece>.fbx:

- WallStraight: one map tile of wall (2 m along X, 4 m high, centred on the tile line): body 0.5 m
  thick, plinth and cornice 0.6 m, so straight runs join seamlessly. Scaled to half length it is also
  the arm that joins a post to its neighbours.
- WallWindow: the same with a shuttered window recessed into both faces.
- WallPost: corners, ends and junctions: chamfered block with plinth and cap all round.
- RoofPanel: 2 x 2 m of overlapping shingle rows (X along the ridge, +Y down the slope).
- RoofRidge: 2 m of rounded ridge tiles.
- Gable: a unit triangle (width 1 along X, height 1, 0.25 m thick) with trim boards, scaled per roof.
Material slots by name: "Wall" (the layer's wall material: stone, plaster or timber), "Trim", "Wood",
"Roof", "Plaster". World-aligned materials in Unreal, so no UVs. Pivot: tile centre on the ground
(roof pieces: their ridge-side top edge / base centre). Metres.

Headless:  blender.exe -b --factory-startup --python Scripts/blender/make_buildings.py
"""
import math
import os

import bmesh
import bpy
from mathutils import Matrix, Vector

OUT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "SourceArt", "Meshes"))
SLOTS = ["Wall", "Trim", "Wood", "Roof", "Plaster"]


def box(bm, lo, hi, mat, bevel=0.0):
    """Axis-aligned box from lo to hi (Vectors), optional bevel on all edges."""
    geom = bmesh.ops.create_cube(bm, size=1.0)
    verts = geom["verts"]
    size = hi - lo
    center = (hi + lo) / 2
    bmesh.ops.scale(bm, vec=size, verts=verts)
    bmesh.ops.translate(bm, vec=center, verts=verts)
    faces = list({f for v in verts for f in v.link_faces})
    if bevel > 0:
        edges = list({e for f in faces for e in f.edges})
        res = bmesh.ops.bevel(bm, geom=edges + verts, offset=bevel, segments=2, affect="EDGES", profile=0.5)
        faces = list(set(faces) | set(res["faces"]))
    for f in faces:
        if f.is_valid:
            f.material_index = SLOTS.index(mat)
    return faces


def prism(bm, pts, z0, z1, mat):
    """Vertical prism over a polygon (list of (x, y)) from z0 to z1."""
    bottom = [bm.verts.new((x, y, z0)) for x, y in pts]
    top = [bm.verts.new((x, y, z1)) for x, y in pts]
    faces = [bm.faces.new(list(reversed(bottom))), bm.faces.new(top)]
    n = len(pts)
    for i in range(n):
        faces.append(bm.faces.new((bottom[i], bottom[(i + 1) % n], top[(i + 1) % n], top[i])))
    for f in faces:
        f.material_index = SLOTS.index(mat)


# Wall thickness (half widths, metres): body 0.5 m, plinth and cornice 0.6 m; posts 0.72 m.
BODY, LIP = 0.25, 0.3


def wall_profile(bm, x0, x1, H=4.0):
    """Plinth, body and cornice of a straight wall from x0 to x1 (along X), centred on the tile line."""
    box(bm, Vector((x0, -LIP, 0.0)), Vector((x1, LIP, 0.45)), "Trim", bevel=0.02)
    box(bm, Vector((x0, -LIP + 0.03, 0.45)), Vector((x1, LIP - 0.03, 0.55)), "Trim")
    box(bm, Vector((x0, -BODY, 0.55)), Vector((x1, BODY, H - 0.35)), "Wall")
    box(bm, Vector((x0, -LIP + 0.04, H - 0.35)), Vector((x1, LIP - 0.04, H - 0.25)), "Trim")
    box(bm, Vector((x0, -LIP, H - 0.25)), Vector((x1, LIP, H)), "Trim", bevel=0.02)


def wall_straight():
    bm = bmesh.new()
    wall_profile(bm, -1.0, 1.0)
    export("SM_Kit_WallStraight", bm)


def wall_window():
    bm = bmesh.new()
    H = 4.0
    w, z0, z1, depth = 0.45, 1.25, 2.6, 0.1
    box(bm, Vector((-1.0, -LIP, 0.0)), Vector((1.0, LIP, 0.45)), "Trim", bevel=0.02)
    box(bm, Vector((-1.0, -LIP + 0.03, 0.45)), Vector((1.0, LIP - 0.03, 0.55)), "Trim")
    # Body around the opening: below, above, left, right; the core stays solid behind the recess.
    box(bm, Vector((-1.0, -BODY, 0.55)), Vector((1.0, BODY, z0)), "Wall")
    box(bm, Vector((-1.0, -BODY, z1)), Vector((1.0, BODY, H - 0.35)), "Wall")
    box(bm, Vector((-1.0, -BODY, z0)), Vector((-w, BODY, z1)), "Wall")
    box(bm, Vector((w, -BODY, z0)), Vector((1.0, BODY, z1)), "Wall")
    box(bm, Vector((-w, -BODY + depth, z0)), Vector((w, BODY - depth, z1)), "Wood")   # closed shutters
    for s in (-1, 1):
        y_out = s * BODY
        y_in = s * (BODY - depth)
        lo_y, hi_y = min(y_out, y_in), max(y_out, y_in)
        box(bm, Vector((-w - 0.08, lo_y - (0.06 if s < 0 else 0), z0 - 0.12)), Vector((w + 0.08, hi_y + (0.06 if s > 0 else 0), z0)), "Trim", bevel=0.02)   # sill
        box(bm, Vector((-w - 0.06, lo_y, z1)), Vector((w + 0.06, hi_y, z1 + 0.14)), "Trim")   # lintel
        yy0, yy1 = sorted((y_in, y_in + s * 0.04))
        box(bm, Vector((-0.03, yy0, z0)), Vector((0.03, yy1, z1)), "Wood")   # mullion
        box(bm, Vector((-w, yy0, (z0 + z1) / 2 - 0.03)), Vector((w, yy1, (z0 + z1) / 2 + 0.03)), "Wood")
    box(bm, Vector((-1.0, -LIP + 0.04, H - 0.35)), Vector((1.0, LIP - 0.04, H - 0.25)), "Trim")
    box(bm, Vector((-1.0, -LIP, H - 0.25)), Vector((1.0, LIP, H)), "Trim", bevel=0.02)
    export("SM_Kit_WallWindow", bm)


def wall_post():
    """Corner / end / junction pillar; the builder adds half-wall arms towards the connected sides."""
    bm = bmesh.new()
    H = 4.0

    def octagon(r, c):
        return [(r, -r + c), (r, r - c), (r - c, r), (-r + c, r), (-r, r - c), (-r, -r + c), (-r + c, -r), (r - c, -r)]
    prism(bm, octagon(0.4, 0.06), 0.0, 0.45, "Trim")
    prism(bm, octagon(0.38, 0.06), 0.45, 0.55, "Trim")
    prism(bm, octagon(0.34, 0.07), 0.55, H - 0.35, "Wall")
    prism(bm, octagon(0.38, 0.06), H - 0.35, H - 0.25, "Trim")
    prism(bm, octagon(0.42, 0.06), H - 0.25, H, "Trim")
    export("SM_Kit_WallPost", bm)


def roof_panel():
    """2 m along X (ridge), 2 m down the slope (+Y); top surface at z 0..-0.1, rows overlap downhill."""
    bm = bmesh.new()
    rows = 6
    step = 2.0 / rows
    box(bm, Vector((-1.0, 0.0, -0.16)), Vector((1.0, 2.0, -0.08)), "Wood")   # sheathing
    for r in range(rows):
        y0 = r * step
        # Each row: shingles 0.33 m wide with tiny gaps, thicker at the lower edge (tilted strip).
        n = 6
        for i in range(n):
            x0 = -1.0 + i * (2.0 / n) + (0.0 if r % 2 == 0 else 0.5 * 2.0 / n) - (2.0 / n if r % 2 and i == n - 1 else 0)
            x1 = x0 + 2.0 / n - 0.015
            if r % 2:
                x0 = max(x0, -1.0)
                x1 = min(x1, 1.0)
            top0, top1 = -0.02, -0.08 + 0.0
            v = [bm.verts.new(p) for p in (
                (x0, y0, -0.08), (x1, y0, -0.08), (x1, y0 + step * 1.15, -0.08), (x0, y0 + step * 1.15, -0.08),
                (x0, y0, top1 + 0.03), (x1, y0, top1 + 0.03), (x1, y0 + step * 1.15, top0), (x0, y0 + step * 1.15, top0))]
            faces = [bm.faces.new((v[3], v[2], v[1], v[0])), bm.faces.new((v[4], v[5], v[6], v[7])),
                     bm.faces.new((v[0], v[1], v[5], v[4])), bm.faces.new((v[1], v[2], v[6], v[5])),
                     bm.faces.new((v[2], v[3], v[7], v[6])), bm.faces.new((v[3], v[0], v[4], v[7]))]
            for f in faces:
                f.material_index = SLOTS.index("Roof")
        if r % 2 == 1:
            pass
    # Close the half shingles at the row ends of odd rows.
    export("SM_Kit_RoofPanel", bm)


def roof_ridge():
    bm = bmesh.new()
    seg = 10
    for k in range(4):
        x0 = -1.0 + k * 0.5
        x1 = x0 + 0.52
        ring0, ring1 = [], []
        for i in range(seg + 1):
            a = math.pi * i / seg
            y, z = 0.2 * math.cos(a), 0.12 * math.sin(a) + 0.02
            ring0.append(bm.verts.new((x0, y, z)))
            ring1.append(bm.verts.new((x1, y * 1.08, z * 1.08)))
        for i in range(seg):
            f = bm.faces.new((ring0[i], ring0[i + 1], ring1[i + 1], ring1[i]))
            f.material_index = SLOTS.index("Roof")
            f.smooth = True
        for ring in (ring0, ring1):
            cap = bm.faces.new(ring if ring is ring1 else list(reversed(ring)))
            cap.material_index = SLOTS.index("Roof")
    export("SM_Kit_RoofRidge", bm)


def gable():
    bm = bmesh.new()
    t = 0.125
    pts = [(-0.5, 0.0), (0.5, 0.0), (0.0, 1.0)]
    front = [bm.verts.new((x, -t, z)) for x, z in pts]
    back = [bm.verts.new((x, t, z)) for x, z in pts]
    faces = [bm.faces.new(front), bm.faces.new(list(reversed(back)))]
    for i in range(3):
        faces.append(bm.faces.new((front[i], back[i], back[(i + 1) % 3], front[(i + 1) % 3])))
    for f in faces:
        f.material_index = SLOTS.index("Plaster")
    export("SM_Kit_Gable", bm)


def export(name, bm):
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    for slot in SLOTS:
        mesh.materials.append(bpy.data.materials.get(slot) or bpy.data.materials.new(slot))
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    # Drop slots no face uses, so Unreal gets only real slots.
    used = {p.material_index for p in mesh.polygons}
    for i in reversed(range(len(mesh.materials))):
        if i not in used:
            bpy.context.view_layer.objects.active = obj
            for o in bpy.context.scene.objects:
                o.select_set(o == obj)
            obj.active_material_index = i
            bpy.ops.object.material_slot_remove()
    for o in bpy.context.scene.objects:
        o.select_set(o == obj)
    bpy.context.view_layer.objects.active = obj
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, name + ".fbx")
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                             axis_forward="-Y", axis_up="Z", mesh_smooth_type="FACE", add_leaf_bones=False, bake_anim=False)
    print("make_buildings: %s (%d tris, slots %s)" % (path, sum(len(p.vertices) - 2 for p in mesh.polygons), [m.name for m in mesh.materials]))
    bpy.data.objects.remove(obj)


def main():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o)
    wall_straight()
    wall_window()
    wall_post()
    roof_panel()
    roof_ridge()
    gable()


main()
