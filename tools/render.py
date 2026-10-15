"""Rough renderer for GD object strings produced by the generator.

Draws solids, spikes and small spikes using the initial colour-trigger values,
respecting z order, opacity and additive blending. It is an approximation of
what the editor shows (no textures), meant for checking composition.

usage: python render.py level.txt out.png [scale]
"""
import math
import sys

import numpy as np
from PIL import Image, ImageDraw

TRIGGERS = {899, 901, 1006, 1007, 1268, 1346}
U = 30.0


def parse(path):
    objs = []
    for chunk in open(path).read().strip().split(";"):
        if not chunk:
            continue
        p = chunk.split(",")
        objs.append({int(p[i]): p[i + 1] for i in range(0, len(p) - 1, 2)})
    return objs


def main():
    src, out = sys.argv[1], sys.argv[2]
    scale = float(sys.argv[3]) if len(sys.argv) > 3 else 0.7
    solid = 211
    objs = parse(src)

    colors = {}
    for o in objs:
        if int(o[1]) == 899:
            ch = int(o[23])
            if ch not in colors:
                colors[ch] = ((int(o[7]), int(o[8]), int(o[9])), float(o.get(35, 1)), o.get(17) == "1")

    vis = [o for o in objs if int(o[1]) not in TRIGGERS]
    xs = [float(o[2]) for o in vis]
    ys = [float(o[3]) for o in vis]
    x0, x1 = min(xs) - 30, max(xs) + 30
    y0, y1 = -60.0, min(max(ys) + 30, 26 * U)
    W, H = int((x1 - x0) * scale), int((y1 - y0) * scale)

    bg = colors.get(1000, ((40, 40, 60), 1, False))[0]
    img = np.zeros((H, W, 3), dtype=np.float32)
    img[:] = np.array(bg, dtype=np.float32) / 255.0

    def to_px(x, y):
        return ((x - x0) * scale, (y1 - y) * scale)

    def shape(o):
        oid = int(o[1])
        x, y = float(o[2]), float(o[3])
        rot = math.radians(float(o.get(6, 0)))
        sx, sy = float(o.get(128, o.get(32, 1))), float(o.get(129, o.get(32, 1)))
        if oid == solid:
            pts = [(-15, -15), (15, -15), (15, 15), (-15, 15)]
        elif oid == 8:
            pts = [(-14, -15), (14, -15), (0, 14)]
        elif oid == 39:
            pts = [(-14, -15), (14, -15), (0, -1)]
        else:
            return None
        res = []
        for px, py in pts:
            px, py = px * sx, py * sy
            # GD rotation is clockwise
            rx = px * math.cos(rot) + py * math.sin(rot)
            ry = -px * math.sin(rot) + py * math.cos(rot)
            res.append(to_px(x + rx, y + ry))
        return res

    vis.sort(key=lambda o: int(o.get(25, 0)))
    i = 0
    while i < len(vis):
        ch = int(vis[i].get(21, 0))
        z = int(vis[i].get(25, 0))
        j = i
        while j < len(vis) and int(vis[j].get(21, 0)) == ch and int(vis[j].get(25, 0)) == z:
            j += 1
        col, op, blend = colors.get(ch, ((255, 255, 255), 1.0, False))
        c = np.array(col, dtype=np.float32) / 255.0
        if blend:
            acc = np.zeros((H, W), dtype=np.float32)
            for o in vis[i:j]:
                pts = shape(o)
                if not pts:
                    continue
                m = Image.new("L", (W, H), 0)
                ImageDraw.Draw(m).polygon(pts, fill=255)
                acc += np.asarray(m, dtype=np.float32) / 255.0
            img += acc[..., None] * op * c
            np.clip(img, 0, 1, out=img)
        else:
            m = Image.new("L", (W, H), 0)
            dr = ImageDraw.Draw(m)
            for o in vis[i:j]:
                pts = shape(o)
                if pts:
                    dr.polygon(pts, fill=255)
            a = np.asarray(m, dtype=np.float32)[..., None] / 255.0 * op
            img = img * (1 - a) + c * a
        i = j

    # ground
    gcol = np.array(colors.get(1001, ((20, 20, 30), 1, False))[0], dtype=np.float32) / 255.0
    gy = int((y1 - 0) * scale)
    img[gy:, :] = gcol
    lcol = np.array(colors.get(1002, ((255, 255, 255), 1, False))[0], dtype=np.float32) / 255.0
    img[max(0, gy - 1):gy + 1, :] = lcol
    Image.fromarray((img * 255).astype(np.uint8)).save(out)
    print(out, W, H)


if __name__ == "__main__":
    main()
