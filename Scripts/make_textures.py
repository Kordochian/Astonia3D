"""Paints the kit's tileable textures (our own, procedural: no original Astonia art) into
SourceArt/Textures/T_<Name>.png. Hand-painted look in the spirit of WoW: colour and baked shading in the
albedo, no normal maps. Every texture wraps seamlessly (noise and drawing are done on a wrapped canvas).

Runs with any Python 3 that has Pillow:  python Scripts/make_textures.py
Then Scripts/import_textures.py (in the editor) imports them and builds the kit materials.
"""
import json
import math
import os
import random
from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageOps

N = 512
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "SourceArt", "Textures")


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
    return wrap_op(img, lambda b: b.filter(ImageFilter.GaussianBlur(r)))


def noise(cells_x, cells_y=None, seed=0):
    """Smooth value noise: random cells_x x cells_y grid, wrapped and upscaled to N x N."""
    cells_y = cells_y or cells_x
    rnd = random.Random(seed)
    small = Image.new("L", (cells_x, cells_y))
    small.putdata([rnd.randrange(256) for _ in range(cells_x * cells_y)])
    big = Image.new("L", (cells_x * 3, cells_y * 3))
    for i in range(3):
        for j in range(3):
            big.paste(small, (i * cells_x, j * cells_y))
    big = big.resize((N * 3, N * 3), Image.BICUBIC)
    return big.crop((N, N, 2 * N, 2 * N))


def fbm(base, octaves, seed, gain=0.5):
    out = None
    weight, total = 1.0, 0.0
    for o in range(octaves):
        n = noise(base * 2 ** o, seed=seed * 31 + o)
        if out is None:
            out = n
        else:
            out = Image.blend(out, n, weight / (total + weight))
        total += weight
        weight *= gain
    return ImageOps.autocontrast(out, cutoff=1)


def colorize(gray, dark, light, mid=None):
    return ImageOps.colorize(gray, dark, light, mid) if mid else ImageOps.colorize(gray, dark, light)


def multiply(img, gray, strength):
    """Darkens img where gray is dark (strength 0..1)."""
    shade = gray.point(lambda v: int(255 - strength * (255 - v)))
    return ImageChops.multiply(img, Image.merge("RGB", (shade, shade, shade)))


def draw_wrapped(draw_fn):
    """Calls draw_fn(draw, dx, dy) for the 9 offsets so shapes crossing an edge wrap."""
    def run(d):
        for dx in (-N, 0, N):
            for dy in (-N, 0, N):
                draw_fn(d, dx, dy)
    return run


AVERAGES = {}


def save(img, name):
    os.makedirs(OUT, exist_ok=True)
    img = img.convert("RGB")
    img.save(os.path.join(OUT, "T_%s.png" % name))
    # Average linear colour, so builders can tint a texture to hit a wanted colour on average.
    small = img.resize((64, 64), Image.BOX)
    px = list(small.getdata())
    AVERAGES[name] = [round(sum((p[i] / 255.0) ** 2.2 for p in px) / len(px), 4) for i in range(3)]
    print("make_textures: T_%s.png" % name)


# --- textures ---
def grass():
    base = colorize(fbm(4, 4, 1), (58, 86, 34), (128, 160, 70), (92, 124, 48))
    rnd = random.Random(2)
    img = base.copy()
    d = ImageDraw.Draw(img)
    blades = [(rnd.randrange(N), rnd.randrange(N), rnd.uniform(-0.5, 0.5), rnd.randrange(6, 16), rnd.random()) for _ in range(9000)]

    def blade(dr, dx, dy):
        for x, y, a, l, t in blades:
            c = (int(70 + 80 * t), int(105 + 85 * t), int(35 + 40 * t))
            dr.line([(x + dx, y + dy), (x + dx + l * math.sin(a), y + dy - l * math.cos(a))], fill=c, width=2)
    draw_wrapped(blade)(d)
    return Image.blend(base, blur(img, 0.6), 0.75)


def dirt():
    img = colorize(fbm(6, 5, 3), (92, 70, 48), (168, 136, 98), (130, 102, 72))
    rnd = random.Random(4)
    d = ImageDraw.Draw(img)
    pebbles = [(rnd.randrange(N), rnd.randrange(N), rnd.randrange(2, 6), rnd.random()) for _ in range(900)]

    def peb(dr, dx, dy):
        for x, y, r, t in pebbles:
            c = int(110 + 90 * t)
            dr.ellipse([x + dx - r, y + dy - r * 0.7, x + dx + r, y + dy + r * 0.7], fill=(c, int(c * 0.88), int(c * 0.72)))
    draw_wrapped(peb)(d)
    return multiply(blur(img, 0.7), fbm(16, 3, 5), 0.25)


def stones_layout(rows, seed, jitter=0.25, min_w=0.6, max_w=1.4):
    """Rows of rectangular stones: list of (x0, y0, x1, y1, tone)."""
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
    img = Image.new("RGB", (N, N), mortar)
    d = ImageDraw.Draw(img)

    def draw(dr, dx, dy):
        for x0, y0, x1, y1, t in stones:
            c = tuple(int(dark[i] + (light[i] - dark[i]) * t) for i in range(3))
            dr.rounded_rectangle([x0 + dx + gap, y0 + dy + gap, x1 + dx - gap, y1 + dy - gap], radius=round_r, fill=c)
    draw_wrapped(draw)(d)
    # Baked shading: light from the top-left edge of each stone, dark bottom-right.
    edge = Image.new("L", (N, N), 128)
    de = ImageDraw.Draw(edge)

    def bevel(dr, dx, dy):
        for x0, y0, x1, y1, _ in stones:
            dr.line([(x0 + dx + gap, y0 + dy + gap + 1), (x1 + dx - gap, y0 + dy + gap + 1)], fill=200, width=3)
            dr.line([(x0 + dx + gap, y1 + dy - gap - 1), (x1 + dx - gap, y1 + dy - gap - 1)], fill=50, width=3)
    draw_wrapped(bevel)(de)
    edge = blur(edge, 1.2)
    img = ImageChops.overlay(img, Image.merge("RGB", (edge, edge, edge)))
    return multiply(img, fbm(8, 4, seed), grain)


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
            stones.append((x, r * h + rnd.uniform(-2, 2), x + w, (r + 1) * h + rnd.uniform(-2, 2), rnd.random()))
            x += w
    return masonry(stones, (70, 64, 56), (112, 106, 96), (170, 162, 148), 3, 14, 7, 0.3)


def forest():
    img = colorize(fbm(5, 5, 8), (40, 46, 24), (104, 98, 56), (70, 72, 38))
    rnd = random.Random(9)
    d = ImageDraw.Draw(img)
    leaves = [(rnd.randrange(N), rnd.randrange(N), rnd.uniform(0, math.pi), rnd.randrange(3, 7), rnd.random()) for _ in range(2500)]

    def leaf(dr, dx, dy):
        for x, y, a, l, t in leaves:
            c = (int(110 + 70 * t), int(70 + 40 * t), int(30 + 15 * t)) if t > 0.4 else (int(60 + 40 * t), int(80 + 40 * t), 30)
            ex, ey = l * math.cos(a), l * math.sin(a)
            dr.line([(x + dx - ex, y + dy - ey), (x + dx + ex, y + dy + ey)], fill=c, width=3)
    draw_wrapped(leaf)(d)
    return multiply(blur(img, 0.5), fbm(12, 3, 10), 0.3)


def planks(boards=6, vertical=False):
    h = N / boards
    rnd = random.Random(11)
    grain = noise(6, 90, seed=12)          # stretched along the board
    grain = ImageOps.autocontrast(Image.blend(grain, noise(12, 180, seed=13), 0.4))
    base = colorize(grain, (88, 56, 30), (156, 108, 62), (122, 80, 44))
    img = base.copy()
    d = ImageDraw.Draw(img)
    for b in range(boards):
        tone = rnd.uniform(0.82, 1.12)
        band = img.crop((0, int(b * h), N, int((b + 1) * h))).point(lambda v, t=tone: min(255, int(v * t)))
        img.paste(band, (0, int(b * h)))
        y = int(b * h)
        d.line([(0, y), (N, y)], fill=(42, 26, 14), width=3)
        d.line([(0, y + 3), (N, y + 3)], fill=(176, 130, 82), width=1)
        for _ in range(2):    # butt joints
            x = rnd.randrange(N)
            d.line([(x, y), (x, y + h)], fill=(50, 30, 16), width=2)
        for _ in range(3):    # knots
            kx, ky = rnd.randrange(N), y + rnd.uniform(h * 0.3, h * 0.7)
            d.ellipse([kx - 6, ky - 3, kx + 6, ky + 3], fill=(70, 42, 22))
    img = blur(img, 0.5)
    return img.rotate(90) if vertical else img


def tiles():
    stones = []
    n = 4
    rnd = random.Random(14)
    for i in range(n):
        for j in range(n):
            stones.append((i * N / n, j * N / n, (i + 1) * N / n, (j + 1) * N / n, rnd.random()))
    return masonry(stones, (96, 90, 80), (150, 140, 124), (198, 188, 168), 3, 4, 15, 0.22)


def flagstone():
    stones = stones_layout(5, 16, min_w=0.8, max_w=1.8)
    return masonry(stones, (64, 60, 54), (104, 100, 92), (156, 150, 138), 3, 6, 17, 0.35)


def bricks():
    stones = stones_layout(8, 18, min_w=1.2, max_w=2.4)
    return masonry(stones, (88, 82, 74), (118, 112, 102), (178, 170, 156), 2, 4, 19, 0.35)


def smooth_stone():
    img = colorize(fbm(5, 5, 20), (132, 126, 116), (186, 180, 168))
    return multiply(img, fbm(20, 3, 21), 0.15)


def shingles(rows=10, dark=(78, 44, 32), light=(150, 86, 58), seed=22, slate=False):
    rnd = random.Random(seed)
    img = Image.new("RGB", (N, N), dark)
    d = ImageDraw.Draw(img)
    h = N / rows
    shapes = []
    for r in range(rows):
        x = (r % 2) * h * 0.5
        while x < N + h:
            w = h * (rnd.uniform(0.7, 1.1) if not slate else 0.9)
            shapes.append((x, r * h, x + w, (r + 1) * h + h * 0.35, rnd.random()))
            x += w

    def draw(dr, dx, dy):
        for x0, y0, x1, y1, t in shapes:
            c = tuple(int(dark[i] + (light[i] - dark[i]) * (0.35 + 0.65 * t)) for i in range(3))
            dr.rounded_rectangle([x0 + dx + 1.5, y0 + dy, x1 + dx - 1.5, y1 + dy], radius=4 if slate else 7, fill=c)
            dr.line([(x0 + dx + 3, y1 + dy - 2), (x1 + dx - 3, y1 + dy - 2)], fill=tuple(int(v * 0.55) for v in c), width=3)
    draw_wrapped(draw)(d)
    return multiply(blur(img, 0.6), fbm(10, 3, seed + 1), 0.25)


def plaster():
    img = colorize(fbm(4, 5, 24), (176, 164, 140), (226, 216, 194))
    return multiply(img, fbm(24, 2, 25), 0.12)


def bark():
    g = ImageOps.autocontrast(Image.blend(noise(40, 4, seed=26), noise(80, 8, seed=27), 0.4))
    img = colorize(g, (48, 34, 22), (124, 96, 66), (84, 62, 42))
    return multiply(img, fbm(10, 3, 28), 0.2)


def leaves():
    rnd = random.Random(29)
    img = colorize(fbm(3, 3, 30), (34, 62, 22), (70, 110, 40))
    d = ImageDraw.Draw(img)
    pts = [(rnd.randrange(N), rnd.randrange(N), rnd.uniform(0, math.pi), rnd.randrange(7, 14), rnd.random()) for _ in range(4200)]

    def leaf(dr, dx, dy):
        for x, y, a, l, t in pts:
            c = (int(48 + 70 * t), int(84 + 90 * t), int(28 + 40 * t))
            ex, ey = l * math.cos(a), l * math.sin(a)
            dr.ellipse([x + dx - abs(ex) - 3, y + dy - abs(ey) - 3, x + dx + abs(ex) + 3, y + dy + abs(ey) + 3], fill=c)
    draw_wrapped(leaf)(d)
    return multiply(blur(img, 0.6), fbm(6, 3, 31), 0.35)


def rock():
    img = colorize(fbm(4, 6, 32), (84, 80, 74), (166, 160, 150), (122, 118, 110))
    cracks = Image.new("L", (N, N), 255)
    rnd = random.Random(33)
    segments = []
    for _ in range(14):
        x, y = rnd.randrange(N), rnd.randrange(N)
        a = rnd.uniform(0, 2 * math.pi)
        for _ in range(rnd.randrange(8, 20)):
            a += rnd.uniform(-0.6, 0.6)
            nx, ny = x + 9 * math.cos(a), y + 9 * math.sin(a)
            segments.append((x, y, nx, ny))
            x, y = nx, ny

    def crack(dr, dx, dy):
        for x0, y0, x1, y1 in segments:
            dr.line([(x0 + dx, y0 + dy), (x1 + dx, y1 + dy)], fill=60, width=2)
    draw_wrapped(crack)(ImageDraw.Draw(cracks))
    return multiply(img, blur(cracks, 0.8), 0.5)


def main():
    save(grass(), "Grass")
    save(dirt(), "Dirt")
    save(cobble(), "Cobble")
    save(forest(), "Forest")
    save(planks(), "Planks")
    save(planks(5, vertical=True), "Timber")
    save(tiles(), "Tiles")
    save(flagstone(), "Flagstone")
    save(bricks(), "Bricks")
    save(smooth_stone(), "SmoothStone")
    save(shingles(), "Shingles")
    save(shingles(12, (40, 46, 58), (96, 108, 128), 34, slate=True), "Slate")
    save(plaster(), "Plaster")
    save(bark(), "Bark")
    save(leaves(), "Leaves")
    save(rock(), "Rock")
    with open(os.path.join(OUT, "averages.json"), "w") as f:
        json.dump(AVERAGES, f, indent=1, sort_keys=True)


main()
