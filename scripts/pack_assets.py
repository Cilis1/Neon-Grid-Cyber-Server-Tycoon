#!/usr/bin/env python3
"""Packs the runtime assets (assets/ + data/ + scripts/lua) into a zip next to
the built game exe, producing the `game.zip` payload uploaded to GitHub releases.

Usage: python scripts/pack_assets.py <staging_dir> <output.zip>
The staging dir should be the CMake install prefix (contains NeonGrid.exe,
SDL DLLs, assets/, data/, scripts/lua/).
"""
import os
import sys
import zipfile

# Top-level staging dirs that are build/dev artifacts, never game runtime files.
SKIP_TOP_DIRS = {"include", "lib", "share", "bin"}


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    staging = os.path.abspath(sys.argv[1])
    out = os.path.abspath(sys.argv[2])
    if not os.path.isdir(staging):
        print("staging dir not found:", staging, file=sys.stderr)
        return 1
    count = 0
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
        for base, dirs, files in os.walk(staging):
            rel_base = os.path.relpath(base, staging)
            top = rel_base.split(os.sep)[0]
            if rel_base != "." and top in SKIP_TOP_DIRS:
                dirs[:] = []  # prune
                continue
            for fn in files:
                full = os.path.join(base, fn)
                rel = os.path.relpath(full, staging)
                z.write(full, rel)
                count += 1
    print(f"packed {count} files -> {out} ({os.path.getsize(out)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
