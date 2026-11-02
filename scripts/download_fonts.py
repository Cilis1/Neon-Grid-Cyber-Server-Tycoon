#!/usr/bin/env python3
"""Downloads the open-source fonts used by Neon Grid into assets/fonts/.

Fonts (both SIL Open Font License 1.1):
  - Orbitron Bold   (headings)   - github.com/theleagueof/orbitron
  - VT323 Regular   (mono/logs)  - github.com/phoikoi/VT323

Fetched as codeload tarballs (works through GitHub-only proxies; raw
redirects are not needed). If a download fails the game silently falls
back to the ImGui default font, so this script is best-effort
(CI marks it continue-on-error).
"""
import io
import os
import sys
import tarfile
import urllib.request

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "assets", "fonts")
LICENSES = os.path.join(OUT, "licenses")

# (tarball url, member path inside tarball, output file name, license member, license name)
FONTS = [
    (
        "https://codeload.github.com/theleagueof/orbitron/tar.gz/refs/heads/master",
        "orbitron-master/Orbitron Bold.ttf",
        "Orbitron-Bold.ttf",
        "orbitron-master/Open Font License.markdown",
        "OFL-1.1-orbitron.txt",
    ),
    (
        "https://codeload.github.com/phoikoi/VT323/tar.gz/refs/heads/master",
        "VT323-master/fonts/ttf/VT323-Regular.ttf",
        "VT323-Regular.ttf",
        "VT323-master/OFL.txt",
        "OFL-1.1-vt323.txt",
    ),
]


def fetch(url, timeout=60):
    req = urllib.request.Request(url, headers={"User-Agent": "neon-grid-font-downloader/1.0"})
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return r.read()


def extract_member(tgz_bytes, member):
    with tarfile.open(fileobj=io.BytesIO(tgz_bytes), mode="r:gz") as tf:
        f = tf.extractfile(member)
        if f is None:
            raise KeyError(member)
        return f.read()


def main():
    os.makedirs(OUT, exist_ok=True)
    os.makedirs(LICENSES, exist_ok=True)
    failures = 0
    for url, member, name, lic_member, lic_name in FONTS:
        dest = os.path.join(OUT, name)
        try:
            if not os.path.exists(dest) or os.path.getsize(dest) == 0:
                print("downloading", name)
                data = fetch(url)
                with open(dest, "wb") as f:
                    f.write(extract_member(data, member))
            print("ok", name, os.path.getsize(dest), "bytes")
            lic_dest = os.path.join(LICENSES, lic_name)
            if not os.path.exists(lic_dest) or os.path.getsize(lic_dest) == 0:
                with open(lic_dest, "wb") as f:
                    f.write(extract_member(data, lic_member))
        except Exception as e:  # best effort
            failures += 1
            print("FAILED", name, "->", e, file=sys.stderr)
    if failures:
        print(f"{failures} font download(s) failed; game will use fallback fonts", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
