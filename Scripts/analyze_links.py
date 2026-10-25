"""Offline analysis of one area: which walkable sections the map has and how teleports (driver 10)
join them. Used to plan the vertical layout (basements, caverns) that replaces the 2D teleport holes.

  python Scripts/analyze_links.py [zone] [map] > report.txt     (defaults: 1 cameron.map)
"""
import os
import re
import sys
from collections import Counter, deque

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "../../../source_codes/astonia_community_server35-main/zones"))
ZONE = sys.argv[1] if len(sys.argv) > 1 else "1"
MAP = sys.argv[2] if len(sys.argv) > 2 else "cameron.map"
SIZE = 256
TOKEN = re.compile(r'\s*(?:(#)[^\n]*|(;)|([A-Za-z0-9_\-]+)\s*(?:(:)|=\s*(?:"([^"]*)"|([A-Za-z0-9_\-]*))))')


def tokens(text):
    pos = 0
    while True:
        m = TOKEN.match(text, pos)
        if not m or m.end() == pos:
            return
        pos = m.end()
        if m.group(1):
            continue
        if m.group(2):
            yield ";", None
        elif m.group(4):
            yield m.group(3), ":"
        else:
            yield m.group(3), m.group(5) if m.group(5) is not None else m.group(6)


def load_templates(ext):
    out = {}
    for folder in ["generic", ZONE]:
        for f in sorted(os.listdir(os.path.join(ROOT, folder))):
            if not f.endswith(ext):
                continue
            cur = None
            for k, v in tokens(open(os.path.join(ROOT, folder, f), encoding="latin-1").read()):
                if v == ":":
                    cur = {"_name": k, "flag": []}
                    out[k.lower()] = cur
                elif k == ";":
                    cur = None
                elif cur is not None:
                    if k.lower() == "flag":
                        cur["flag"].append(v.upper())
                    else:
                        cur.setdefault(k.lower(), v)
    return out


items = load_templates(".itm")
chars = load_templates(".chr")
tiles = {}
x = y = 0
for k, v in tokens(open(os.path.join(ROOT, ZONE, MAP), encoding="latin-1").read()):
    k = k.lower()
    if k == "field":
        x, y = (int(p) for p in v.split(","))
        tiles[(x, y)] = {"g": 0, "f": 0, "flags": set(), "it": None, "ch": None}
    elif k in ("gsprite", "fsprite"):
        tiles[(x, y)][k[0]] = int(v)
    elif k == "flag":
        tiles[(x, y)]["flags"].add(v.upper())
    elif k == "it":
        tiles[(x, y)]["it"] = v
    elif k == "ch":
        tiles[(x, y)]["ch"] = v


def walkable(p):
    t = tiles.get(p)
    if not t or not t["g"] or "MF_MOVEBLOCK" in t["flags"]:
        return False
    it = items.get((t["it"] or "").lower())
    if it and "IF_MOVEBLOCK" in it["flag"] and "IF_DOOR" not in it["flag"]:
        return False
    return True


comp = {}
sections = []
for p in sorted(tiles, key=lambda q: (q[1], q[0])):
    if p in comp or not walkable(p):
        continue
    sid = len(sections)
    q = deque([p])
    comp[p] = sid
    cells = []
    while q:
        c = q.popleft()
        cells.append(c)
        for d in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            n = (c[0] + d[0], c[1] + d[1])
            if n not in comp and walkable(n):
                comp[n] = sid
                q.append(n)
    sections.append(cells)


def decode_teleport(arg):
    b = bytes.fromhex(arg[:12].ljust(12, "0"))
    return b[0] | b[1] << 8, b[2] | b[3] << 8, b[4] | b[5] << 8


def section_of(p):
    if p in comp:
        return comp[p]
    for r in range(1, 3):
        for dx in range(-r, r + 1):
            for dy in range(-r, r + 1):
                if (p[0] + dx, p[1] + dy) in comp:
                    return comp[(p[0] + dx, p[1] + dy)]
    return None


links = []
for p, t in tiles.items():
    it = items.get((t["it"] or "").lower())
    if not it or it.get("driver") != "10" or "arg" not in it:
        continue
    tx, ty, area = decode_teleport(it["arg"])
    links.append((p, section_of(p), (tx, ty), area, section_of((tx, ty)) if area in (0, int(ZONE)) else None,
                  it["_name"], it.get("name", ""), it.get("description", "")))

print("Zone %s %s: %d walkable sections, %d teleport items\n" % (ZONE, MAP, len(sections), len(links)))
print("== Sections with 20+ tiles (id: tiles, bbox, indoors%%, NPCs)")
big = [i for i, s in enumerate(sections) if len(s) >= 20]
for i in sorted(big, key=lambda i: -len(sections[i])):
    s = sections[i]
    xs = [c[0] for c in s]
    ys = [c[1] for c in s]
    indoor = sum("MF_INDOORS" in tiles[c]["flags"] for c in s) * 100 // len(s)
    npcs = Counter(chars.get((tiles[c]["ch"] or "").lower(), {}).get("name", tiles[c]["ch"]) for c in s if tiles[c]["ch"])
    print("S%-3d %6d tiles  x %3d-%3d  y %3d-%3d  indoors %3d%%  %s" % (i, len(s), min(xs), max(xs), min(ys), max(ys), indoor,
          ", ".join("%s x%d" % kv for kv in npcs.most_common(6))))

print("\n== Teleports (from tile [section] -> to tile [section] area, template: name / description)")
for p, s1, dst, area, s2, tpl, name, desc in sorted(links, key=lambda l: (l[1] if l[1] is not None else -1, l[0])):
    print("%3d,%3d [S%s] -> %3d,%3d [%s] area %d  %s: %s / %s" % (p[0], p[1], s1, dst[0], dst[1],
          ("S%s" % s2) if s2 is not None else "other area", area, tpl, name, desc[:60]))
