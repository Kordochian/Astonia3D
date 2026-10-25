"""Paints the kit's tileable textures (our own, procedural: no original Astonia art) into SourceArt/Textures:
for each surface T_<Name>.png (albedo, hand-painted look with baked shading), T_<Name>_N.png (normal map
from the painted height) and T_<Name>_ORM.png (R ambient occlusion, G roughness, B height), plus
T_Macro.png (large-scale variation the material uses to hide tiling). Every texture wraps seamlessly
(noise and drawing are done on a wrapped canvas). averages.json holds each albedo's average linear colour.

Runs with any Python 3 that has Pillow:  python Scripts/make_textures.py [size]   (default 2048)
Then Scripts/import_textures.py (in the editor) imports them and builds the kit materials.
"""
import json
import math
import os
import random
import sys
from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageOps

N = int(sys.argv[1]) if len(sys.argv) > 1 else 2048
S = N / 512.0          # the designs below are in 512 px units
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "SourceArt", "Textures")


def px(v):
    return v * S


def count(n):
    return int(n * S * S)


# --- wrapped helpers ---
def wrap_op(img, fn):
    """Applies fn to a 3x3 tiling and crops the middle, so filters wrap round the edges."""
    w, h = img.size
    big = Image.new(img.mode, (w * 3, h * 3))
    for i in range(3):
        for j in range(3):
            big.paste(img, (i * w, j * h))
    return fn(big).crop((w, h, 2 * w, 2 * h))


def blur(img, r):
    return wrap_op(img, lambda b: b.filter(ImageFilter.GaussianBlur(px(r))))


def noise(cells_x, cells_y=None, seed=0):
    """Smooth value noise: random cells_x x cells_y grid, wrapped and upscaled to N x N."""
    cells_y = cells_y or cells_x
    rnd = random.Random(seed)
    small = Image.new("L", (cells_x, cells_y))
    small.putdata([rnd.randrange(256) for _ in range(cells_x * cells_y)])
    pad = 3
    big = Image.new("L", (cells_x + 2 * pad, cells_y + 2 * pad))
    for i in range(-1, 2):
        for j in range(-1, 2):
            big.paste(small, (pad + i * cells_x, pad + j * cells_y))
    fx, fy = N / cells_x, N / cells_y
    big = big.resize((int((cells_x + 2 * pad) * fx), int((cells_y + 2 * pad) * fy)), Image.BICUBIC)
    return big.crop((int(pad * fx), int(pad * fy), int(pad * fx) + N, int(pad * fy) + N))


def fbm(base, octaves, seed, gain=0.5):
    out = None
    weight, total = 1.0, 0.0
    for o in range(octaves):
        n = noise(min(base * 2 ** o, N // 2), seed=seed * 31 + o)
        out = n if out is None else Image.blend(out, n, weight / (total + weight))
        total += weight
        weight *= gain
    return ImageOps.autocontrast(out, cutoff=1)


def colorize(gray, dark, light, mid=None):
    return ImageOps.colorize(gray, dark, light, mid) if mid else ImageOps.colorize(gray, dark, light)


def multiply(img, gray, strength):
    """Darkens img where gray is dark (strength 0..1)."""
    shade = gray.point(lambda v: int(255 - strength * (255 - v)))
    return ImageChops.multiply(img, Image.merge("RGB", (shade, shade, shade)))


def fine(img, seed, strength=0.12):
    """High-frequency grain so the full resolution carries detail, not just a smooth upscale."""
    return multiply(img, fbm(96, 3, seed), strength)


def draw_wrapped(draw_fn):
    """Calls draw_fn(draw, dx, dy) for the 9 offsets so shapes crossing an edge wrap."""
    def run(d):
        for dx in (-N, 0, N):
            for dy in (-N, 0, N):
                draw_fn(d, dx, dy)
    return run


AVERAGES = {}


def save(name, img, height=None, rough=0.85, rough_var=0.12, bump=1.0):
    """Writes albedo, normal and ORM. height: L image (bright = high); defaults to the albedo's luminance."""
    os.makedirs(OUT, exist_ok=True)
    img = img.convert("RGB")
    img.save(os.path.join(OUT, "T_%s.png" % name))
    small = img.resize((64, 64), Image.BOX)
    pix = list(small.getdata())
    AVERAGES[name] = [round(sum((p[i] / 255.0) ** 2.2 for p in pix) / len(pix), 4) for i in range(3)]

    h = ImageOps.autocontrast(height if height is not None else img.convert("L"), cutoff=1)
    h = blur(h, 0.6)
    # Normal from height differences (wrapped), strength scaled with resolution.
    k = 0.5 / max(bump * S, 0.25)
    off = max(1, int(round(S)))
    dx = ImageChops.subtract(ImageChops.offset(h, off, 0), ImageChops.offset(h, -off, 0), scale=k, offset=128)
    dy = ImageChops.subtract(ImageChops.offset(h, 0, off), ImageChops.offset(h, 0, -off), scale=k, offset=128)
    z = Image.new("L", (N, N), 255)
    Image.merge("RGB", (dx, dy, z)).save(os.path.join(OUT, "T_%s_N.png" % name))
    # AO: darker where the surface is lower than its surroundings. Roughness: rougher in the low parts.
    around = blur(h, 6)
    ao = ImageChops.add(ImageChops.subtract(h, around, scale=1, offset=200), Image.new("L", (N, N), 0)).point(lambda v: min(255, v + 30))
    lum = img.convert("L")
    r0 = int(255 * rough)
    roughness = lum.point(lambda v: max(0, min(255, int(r0 + 255 * rough_var * (0.5 - v / 255.0) * 2))))
    Image.merge("RGB", (ao, roughness, h)).save(os.path.join(OUT, "T_%s_ORM.png" % name))
    print("make_textures: T_%s (+_N, _ORM)" % name)


# --- textures (designed in 512 px units, scaled by S) ---
def grass():
    base = colorize(fbm(4, 5, 1), (58, 86, 34), (128, 160, 70), (92, 124, 48))
    rnd = random.Random(2)
    img = base.copy()
    d = ImageDraw.Draw(img)
    blades = [(rnd.uniform(0, N), rnd.uniform(0, N), rnd.uniform(-0.5, 0.5), px(rnd.uniform(6, 16)), rnd.random()) for _ in range(count(9000))]
    w = max(1, int(round(px(1.5))))

    def blade(dr, dx, dy):
        for x, y, a, l, t in blades:
            if not (-l < x + dx < N + l and -l < y + dy < N + l):
                continue
            c = (int(70 + 80 * t), int(105 + 85 * t), int(35 + 40 * t))
            dr.line([(x + dx, y + dy), (x + dx + l * math.sin(a), y + dy - l * math.cos(a))], fill=c, width=w)
    draw_wrapped(blade)(d)
    img = Image.blend(base, blur(img, 0.5), 0.8)
    return fine(img, 40, 0.10)


def dirt():
    img = colorize(fbm(6, 6, 3), (92, 70, 48), (168, 136, 98), (130, 102, 72))
    rnd = random.Random(4)
    d = ImageDraw.Draw(img)
    pebbles = [(rnd.uniform(0, N), rnd.uniform(0, N), px(rnd.uniform(1.5, 6)), rnd.random()) for _ in range(count(900))]

    def peb(dr, dx, dy):
        for x, y, r, t in pebbles:
            if not (-r < x + dx < N + r and -r < y + dy < N + r):
                continue
            c = int(110 + 90 * t)
            dr.ellipse([x + dx - r, y + dy - r * 0.7, x + dx + r, y + dy + r * 0.7], fill=(c, int(c * 0.88), int(c * 0.72)))
    draw_wrapped(peb)(d)
    return fine(multiply(blur(img, 0.6), fbm(16, 4, 5), 0.25), 41)


def stones_layout(rows, seed, min_w=0.6, max_w=1.4):
    """Rows of rectangular stones: list of (x0, y0, x1, y1, tone), in pixels."""
    rnd = random.Random(seed)
    out = []
    h = N / rows
    for r in range(rows):
        x = rnd.uniform(0, h)
        start = x
        while x < start + N - h * 0.3:
            w = h * rnd.uniform(min_w, max_w)
            if x + w > start + N:
                w = start + N - x
            out.append((x, r * h, x + w, (r + 1) * h, rnd.random()))
            x += w
    return out


def masonry(stones, mortar, dark, light, gap, round_r, seed, grain=0.35):
    """Stones on mortar with bevelled edges. Returns (albedo, height)."""
    gap, round_r = px(gap), px(round_r)
    img = Image.new("RGB", (N, N), mortar)
    height = Image.new("L", (N, N), 40)
    d, dh = ImageDraw.Draw(img), ImageDraw.Draw(height)
    rnd = random.Random(seed)
    bulge = [rnd.uniform(150, 230) for _ in stones]

    def draw(dr, dx, dy):
        for (x0, y0, x1, y1, t), hv in zip(stones, bulge):
            if x1 + dx < 0 or x0 + dx > N or y1 + dy < 0 or y0 + dy > N:
                continue
            c = tuple(int(dark[i] + (light[i] - dark[i]) * t) for i in range(3))
            box = [x0 + dx + gap, y0 + dy + gap, x1 + dx - gap, y1 + dy - gap]
            dr.rounded_rectangle(box, radius=round_r, fill=c)
            dh.rounded_rectangle(box, radius=round_r, fill=int(hv))
    draw_wrapped(draw)(d)
    edge = Image.new("L", (N, N), 128)
    de = ImageDraw.Draw(edge)
    lw = max(1, int(round(px(3))))

    def bevel(dr, dx, dy):
        for x0, y0, x1, y1, _ in stones:
            if x1 + dx < 0 or x0 + dx > N or y1 + dy < 0 or y0 + dy > N:
                continue
            dr.line([(x0 + dx + gap, y0 + dy + gap + lw / 2), (x1 + dx - gap, y0 + dy + gap + lw / 2)], fill=200, width=lw)
            dr.line([(x0 + dx + gap, y1 + dy - gap - lw / 2), (x1 + dx - gap, y1 + dy - gap - lw / 2)], fill=50, width=lw)
    draw_wrapped(bevel)(de)
    edge = blur(edge, 1.2)
    img = ImageChops.overlay(img, Image.merge("RGB", (edge, edge, edge)))
    surface = fbm(8, 6, seed)
    img = fine(multiply(img, surface, grain), seed + 100)
    height = ImageChops.multiply(blur(height, 1.5), surface.point(lambda v: 150 + v * 105 // 255))
    return img, height


def cobble():
    rnd = random.Random(6)
    stones = []
    rows = 9
    h = N / rows
    for r in range(rows):
        x = rnd.uniform(0, h)
        start = x
        while x < start + N - h * 0.4:
            w = h * rnd.uniform(0.8, 1.3)
            stones.append((x, r * h + px(rnd.uniform(-2, 2)), x + w, (r + 1) * h + px(rnd.uniform(-2, 2)), rnd.random()))
            x += w
    return masonry(stones, (70, 64, 56), (112, 106, 96), (170, 162, 148), 3, 14, 7, 0.3)


def forest():
    img = colorize(fbm(5, 6, 8), (40, 46, 24), (104, 98, 56), (70, 72, 38))
    rnd = random.Random(9)
    d = ImageDraw.Draw(img)
    leaves = [(rnd.uniform(0, N), rnd.uniform(0, N), rnd.uniform(0, math.pi), px(rnd.uniform(3, 7)), rnd.random()) for _ in range(count(2500))]
    w = max(1, int(round(px(2.5))))

    def leaf(dr, dx, dy):
        for x, y, a, l, t in leaves:
            if not (-l < x + dx < N + l and -l < y + dy < N + l):
                continue
            c = (int(110 + 70 * t), int(70 + 40 * t), int(30 + 15 * t)) if t > 0.4 else (int(60 + 40 * t), int(80 + 40 * t), 30)
            ex, ey = l * math.cos(a), l * math.sin(a)
            dr.line([(x + dx - ex, y + dy - ey), (x + dx + ex, y + dy + ey)], fill=c, width=w)
    draw_wrapped(leaf)(d)
    return fine(multiply(blur(img, 0.4), fbm(12, 4, 10), 0.3), 42)


def planks(boards=6, vertical=False):
    h = N / boards
    rnd = random.Random(11)
    grain = noise(6, 90, seed=12)          # stretched along the board
    grain = ImageOps.autocontrast(Image.blend(Image.blend(grain, noise(12, 180, seed=13), 0.4), noise(24, 360, seed=14), 0.25))
    base = colorize(grain, (88, 56, 30), (156, 108, 62), (122, 80, 44))
    img = base.copy()
    height = Image.new("L", (N, N), 200)
    d, dh = ImageDraw.Draw(img), ImageDraw.Draw(height)
    seam = max(2, int(round(px(3))))
    for b in range(boards):
        tone = rnd.uniform(0.82, 1.12)
        band = img.crop((0, int(b * h), N, int((b + 1) * h))).point(lambda v, t=tone: min(255, int(v * t)))
        img.paste(band, (0, int(b * h)))
        y = int(b * h)
        d.line([(0, y), (N, y)], fill=(42, 26, 14), width=seam)
        dh.line([(0, y), (N, y)], fill=40, width=seam)
        d.line([(0, y + seam), (N, y + seam)], fill=(176, 130, 82), width=max(1, seam // 3))
        for _ in range(2):    # butt joints
            x = rnd.uniform(0, N)
            d.line([(x, y), (x, y + h)], fill=(50, 30, 16), width=max(1, seam * 2 // 3))
            dh.line([(x, y), (x, y + h)], fill=60, width=max(1, seam * 2 // 3))
        for _ in range(3):    # knots
            kx, ky = rnd.uniform(0, N), y + rnd.uniform(h * 0.3, h * 0.7)
            d.ellipse([kx - px(6), ky - px(3), kx + px(6), ky + px(3)], fill=(70, 42, 22))
    height = ImageChops.multiply(height, grain.point(lambda v: 170 + v * 85 // 255))
    img = fine(blur(img, 0.4), 43, 0.08)
    if vertical:
        return img.rotate(90), height.rotate(90)
    return img, height


def tiles():
    stones = []
    n = 4
    rnd = random.Random(14)
    for i in range(n):
        for j in range(n):
            stones.append((i * N / n, j * N / n, (i + 1) * N / n, (j + 1) * N / n, rnd.random()))
    return masonry(stones, (96, 90, 80), (150, 140, 124), (198, 188, 168), 3, 4, 15, 0.22)


def flagstone():
    return masonry(stones_layout(5, 16, min_w=0.8, max_w=1.8), (64, 60, 54), (104, 100, 92), (156, 150, 138), 3, 6, 17, 0.35)


def bricks():
    return masonry(stones_layout(8, 18, min_w=1.2, max_w=2.4), (88, 82, 74), (118, 112, 102), (178, 170, 156), 2, 4, 19, 0.35)


def smooth_stone():
    surface = fbm(5, 7, 20)
    img = colorize(surface, (132, 126, 116), (186, 180, 168))
    return fine(multiply(img, fbm(20, 4, 21), 0.15), 44, 0.08), surface


def shingles(rows=10, dark=(78, 44, 32), light=(150, 86, 58), seed=22, slate=False):
    rnd = random.Random(seed)
    img = Image.new("RGB", (N, N), dark)
    height = Image.new("L", (N, N), 30)
    d, dh = ImageDraw.Draw(img), ImageDraw.Draw(height)
    h = N / rows
    shapes = []
    for r in range(rows):
        x = (r % 2) * h * 0.5
        while x < N + h:
            w = h * (rnd.uniform(0.7, 1.1) if not slate else 0.9)
            shapes.append((x, r * h, x + w, (r + 1) * h + h * 0.35, rnd.random()))
            x += w
    lw = max(1, int(round(px(3))))

    def draw(dr, dx, dy):
        for x0, y0, x1, y1, t in shapes:
            if x1 + dx < 0 or x0 + dx > N or y1 + dy < 0 or y0 + dy > N:
                continue
            c = tuple(int(dark[i] + (light[i] - dark[i]) * (0.35 + 0.65 * t)) for i in range(3))
            box = [x0 + dx + px(1.5), y0 + dy, x1 + dx - px(1.5), y1 + dy]
            dr.rounded_rectangle(box, radius=px(4 if slate else 7), fill=c)
            dr.line([(x0 + dx + px(3), y1 + dy - px(2)), (x1 + dx - px(3), y1 + dy - px(2))], fill=tuple(int(v * 0.55) for v in c), width=lw)
            # Height: each shingle rises towards its lower edge (it lies on the next row up).
            for k in range(4):
                f = k / 4.0
                dh.rounded_rectangle([box[0], y0 + dy + (y1 - y0) * f, box[2], y1 + dy], radius=px(4), fill=int(90 + 120 * f))
    draw_wrapped(draw)(d)
    return fine(multiply(blur(img, 0.5), fbm(10, 4, seed + 1), 0.25), seed + 50), height


def plaster():
    surface = fbm(4, 7, 24)
    img = colorize(surface, (176, 164, 140), (226, 216, 194))
    return fine(multiply(img, fbm(24, 3, 25), 0.12), 45, 0.06), surface


def bark():
    g = ImageOps.autocontrast(Image.blend(Image.blend(noise(40, 4, seed=26), noise(80, 8, seed=27), 0.4), noise(160, 16, seed=46), 0.2))
    img = colorize(g, (48, 34, 22), (124, 96, 66), (84, 62, 42))
    return fine(multiply(img, fbm(10, 4, 28), 0.2), 47), g


def leaves():
    rnd = random.Random(29)
    img = colorize(fbm(3, 4, 30), (34, 62, 22), (70, 110, 40))
    height = Image.new("L", (N, N), 40)
    d, dh = ImageDraw.Draw(img), ImageDraw.Draw(height)
    pts = [(rnd.uniform(0, N), rnd.uniform(0, N), rnd.uniform(0, math.pi), px(rnd.uniform(7, 14)), rnd.random()) for _ in range(count(4200))]

    def leaf(dr, dx, dy):
        for x, y, a, l, t in pts:
            if not (-2 * l < x + dx < N + 2 * l and -2 * l < y + dy < N + 2 * l):
                continue
            c = (int(48 + 70 * t), int(84 + 90 * t), int(28 + 40 * t))
            ex, ey = abs(l * math.cos(a)) + px(3), abs(l * math.sin(a)) + px(3)
            dr.ellipse([x + dx - ex, y + dy - ey, x + dx + ex, y + dy + ey], fill=c)
            dh.ellipse([x + dx - ex, y + dy - ey, x + dx + ex, y + dy + ey], fill=int(80 + 170 * t))
    draw_wrapped(leaf)(d)
    return fine(multiply(blur(img, 0.5), fbm(6, 4, 31), 0.35), 48), height


def rock():
    surface = fbm(4, 7, 32)
    img = colorize(surface, (84, 80, 74), (166, 160, 150), (122, 118, 110))
    cracks = Image.new("L", (N, N), 255)
    rnd = random.Random(33)
    segments = []
    for _ in range(14):
        x, y = rnd.uniform(0, N), rnd.uniform(0, N)
        a = rnd.uniform(0, 2 * math.pi)
        for _ in range(rnd.randrange(8, 20)):
            a += rnd.uniform(-0.6, 0.6)
            nx, ny = x + px(9) * math.cos(a), y + px(9) * math.sin(a)
            segments.append((x, y, nx, ny))
            x, y = nx, ny
    lw = max(1, int(round(px(2))))

    def crack(dr, dx, dy):
        for x0, y0, x1, y1 in segments:
            dr.line([(x0 + dx, y0 + dy), (x1 + dx, y1 + dy)], fill=60, width=lw)
    draw_wrapped(crack)(ImageDraw.Draw(cracks))
    cracks = blur(cracks, 0.8)
    return fine(multiply(img, cracks, 0.5), 49), ImageChops.multiply(surface, cracks)


def macro():
    g = fbm(3, 4, 60)
    os.makedirs(OUT, exist_ok=True)
    g.resize((1024, 1024), Image.BICUBIC).convert("RGB").save(os.path.join(OUT, "T_Macro.png"))
    print("make_textures: T_Macro")


def main():
    save("Grass", grass(), rough=0.92, bump=0.6)
    save("Dirt", dirt(), rough=0.95)
    save("Cobble", *cobble(), rough=0.85)
    save("Forest", forest(), rough=0.95, bump=0.7)
    save("Planks", *planks(), rough=0.75)
    save("Timber", *planks(5, vertical=True), rough=0.78)
    save("Tiles", *tiles(), rough=0.7)
    save("Flagstone", *flagstone(), rough=0.82)
    save("Bricks", *bricks(), rough=0.85)
    save("SmoothStone", *smooth_stone(), rough=0.8, bump=0.5)
    save("Shingles", *shingles(), rough=0.8)
    save("Slate", *shingles(12, (40, 46, 58), (96, 108, 128), 34, slate=True), rough=0.6)
    save("Plaster", *plaster(), rough=0.9, bump=0.4)
    save("Bark", *bark(), rough=0.9)
    save("Leaves", *leaves(), rough=0.7)
    save("Rock", *rock(), rough=0.85)
    macro()
    with open(os.path.join(OUT, "averages.json"), "w") as f:
        json.dump(AVERAGES, f, indent=1, sort_keys=True)


main()
