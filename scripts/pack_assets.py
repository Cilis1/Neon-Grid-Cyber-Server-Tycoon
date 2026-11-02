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
        for base, _dirs, files in os.walk(staging):
            for fn in files:
                full = os.path.join(base, fn)
                rel = os.path.relpath(full, staging)
                z.write(full, rel)
                count += 1
    print(f"packed {count} files -> {out} ({os.path.getsize(out)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
