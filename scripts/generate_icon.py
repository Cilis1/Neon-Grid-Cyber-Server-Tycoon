#!/usr/bin/env python3
"""Deterministic cyberpunk icon generator for Neon Grid: Cyber Server Tycoon.

Draws a 256x256 icon: dark navy background, neon cyan/magenta glowing circuit
traces forming a stylized server rack "N", plus a scanline effect.
Outputs assets/icon/icon.png and assets/icon/icon.ico (sizes 16..256).

Requires Pillow (pip install pillow). Fully deterministic (no RNG).
"""
import os
import struct

try:
    from PIL import Image, ImageDraw, ImageFilter
except ImportError:
    raise SystemExit("Pillow is required: pip install pillow")

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_DIR = os.path.join(ROOT, "assets", "icon")
SIZE = 256

# palette
BG_TOP = (8, 12, 22)
BG_BOTTOM = (14, 8, 26)
CYAN = (0, 240, 255)
MAGENTA = (255, 0, 160)
GREEN = (57, 255, 20)
DARK = (10, 14, 20)


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def draw_icon() -> Image.Image:
    img = Image.new("RGB", (SIZE, SIZE), BG_TOP)
    d = ImageDraw.Draw(img)
    # vertical gradient background
    for y in range(SIZE):
        d.line([(0, y), (SIZE, y)], fill=lerp(BG_TOP, BG_BOTTOM, y / SIZE))

    # faint grid
    grid = Image.new("RGB", (SIZE, SIZE), (0, 0, 0))
    gd = ImageDraw.Draw(grid)
    step = 16
    for x in range(0, SIZE, step):
        gd.line([(x, 0), (x, SIZE)], fill=(0, 60, 70))
    for y in range(0, SIZE, step):
        gd.line([(0, y), (SIZE, y)], fill=(0, 60, 70))
    img = Image.blend(img, Image.new("RGB", (SIZE, SIZE), (0, 0, 0)), 0.0)
    img.paste(grid, (0, 0))

    # glow layer for circuit traces
    glow = Image.new("RGB", (SIZE, SIZE), (0, 0, 0))
    gd = ImageDraw.Draw(glow)

    def trace(pts, color, width=4):
        gd.line(pts, fill=color, width=width, joint="curve")
        for p in pts:
            if isinstance(p, tuple):
                r = width
                gd.ellipse([p[0] - r, p[1] - r, p[0] + r, p[1] + r], fill=color)

    # stylized "N" made of circuit traces (server rack silhouette inside)
    m = 48  # margin
    top, bottom = m, SIZE - m
    left, right = m, SIZE - m
    # left vertical trace
    trace([(left, bottom), (left, top)], CYAN, 6)
    # diagonal trace (the N slash) in magenta
    trace([(left, top), (right, bottom)], MAGENTA, 6)
    # right vertical trace
    trace([(right, bottom), (right, top)], CYAN, 6)
    # circuit branches off the N
    trace([(left, 96), (88, 96), (88, 64)], CYAN, 3)
    trace([(left, 160), (96, 160), (96, 192)], CYAN, 3)
    trace([(right, 96), (168, 96), (168, 128)], MAGENTA, 3)
    trace([(right, 160), (160, 160), (160, 192)], MAGENTA, 3)
    # server rack slots (horizontal bars) across the middle
    for i, y in enumerate(range(104, 160, 14)):
        col = CYAN if i % 2 == 0 else MAGENTA
        trace([(96, y), (160, y)], col, 3)
        gd.ellipse([166, y - 3, 172, y + 3], fill=GREEN)  # status LED

    # blur the glow layer and add it on top
    glow_blur = glow.filter(ImageFilter.GaussianBlur(6))
    img = Image.blend(img, Image.new("RGB", (SIZE, SIZE), (0, 0, 0)), 0.0)
    from PIL import ImageChops
    img = ImageChops.add(img, glow_blur)          # soft glow
    img = ImageChops.add(img, glow.filter(ImageFilter.GaussianBlur(1)))  # crisp core

    d = ImageDraw.Draw(img)
    # scanlines
    for y in range(0, SIZE, 3):
        d.line([(0, y), (SIZE, y)], fill=(0, 0, 0, 40))
    # border
    d.rectangle([2, 2, SIZE - 3, SIZE - 3], outline=CYAN, width=3)
    d.rectangle([6, 6, SIZE - 7, SIZE - 7], outline=(40, 0, 60), width=1)
    return img


def save_ico(png: Image.Image, path: str):
    sizes = [16, 32, 48, 64, 128, 256]
    frames = [png.resize((s, s), Image.LANCZOS) for s in sizes]
    # BMP-in-ICO format
    images = []
    for s, fr in zip(sizes, frames):
        w, h = s, s
        # BGRA pixel data, bottom-up
        px = fr.convert("RGBA").tobytes()
        bgra = bytearray()
        rows = px
        # PIL gives top-down RGBA; ICO wants bottom-up BGRA
        stride = w * 4
        data = bytearray()
        for y in range(h - 1, -1, -1):
            row = rows[y * stride:(y + 1) * stride]
            for x in range(w):
                r, g, b, a = row[x * 4], row[x * 4 + 1], row[x * 4 + 2], row[x * 4 + 3]
                data += bytes((b, g, r, a))
        # AND mask (1bpp), padded to 32 bits per row
        mask_stride = ((w + 31) // 32) * 4
        mask = bytearray(mask_stride * h)  # all zero = fully opaque
        header = struct.pack("<IIIHHIIIIII", 40, w, h * 2, 1, 32, 0, len(data) + len(mask), 0, 0, 0, 0)
        images.append(header + bytes(data) + bytes(mask))
    icondir = struct.pack("<HHH", 0, 1, len(sizes))
    entries = b""
    offset = 6 + 16 * len(sizes)
    for s, blob in zip(sizes, images):
        entries += struct.pack("<BBBBHHII", s if s < 256 else 0, s if s < 256 else 0, 0, 0, 1, 32, len(blob), offset)
        offset += len(blob)
    with open(path, "wb") as f:
        f.write(icondir + entries + b"".join(images))


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    img = draw_icon()
    png_path = os.path.join(OUT_DIR, "icon.png")
    ico_path = os.path.join(OUT_DIR, "icon.ico")
    img.save(png_path)
    save_ico(img, ico_path)
    print("wrote", png_path)
    print("wrote", ico_path)


if __name__ == "__main__":
    main()
