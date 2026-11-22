#!/usr/bin/env python3
"""A game directory's MANIFEST.txt rows of the retail data, without the
retail games (NODISCS=1: build/all.sh, AVP2_NODISCS=1).

  RETAIL_LAYOUT=<layout> build/nodiscs.py resources  <release> <gamedir>
  RETAIL_LAYOUT=<layout> build/nodiscs.py thirdparty <release> <gamedir>
  RETAIL_LAYOUT=<layout> build/nodiscs.py loose      <release> <gamedir> <file>...

A build with the retail games copies their data into the game directory
(ext/tools/retailextract.py resources and thirdparty, and build/all.sh's
loose files) and records each file in MANIFEST.txt. A build without them
copies nothing, but writes the same rows, from build/retail-files.txt:
build/mkinstall.py makes the public archive's install scripts from them,
so the archive is the same in both builds. <layout> is build/retail.py
(Primal Hunt) or build/retail_avp2.py (AvP2), as for retailextract.py.
"""
import os
import sys
from pathlib import Path

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(REPO, "ext", "tools"))
sys.path.insert(0, os.path.join(REPO, "build"))
import retailextract as rx  # noqa: E402  (the layout, RETAIL_LAYOUT; Manifest)
import discs as disclist  # noqa: E402


def main():
    a = sys.argv[1:]
    if len(a) < 3 or a[0] not in ("resources", "thirdparty", "loose"):
        sys.exit(__doc__)
    cmd, release, gamedir = a[0], a[1], Path(a[2])
    files = [f.path for f in disclist.List().files if f.release == release]
    if not files:
        sys.exit(f"nodiscs.py: no {release} in build/retail-files.txt")

    def find(name):
        for p in files:
            if p.lower() == name.lower():
                return p
        sys.exit(f"nodiscs.py: no {release}:{name} in build/retail-files.txt")

    gamedir.mkdir(parents=True, exist_ok=True)
    man = rx.Manifest(gamedir)
    if cmd == "resources":
        for p in files:
            if p.split("/", 1)[0] in rx.DATA_DIRS and "/" in p:
                man.add(p, f"data {release}:{p}")
                # (the directory itself, as a build with the discs has it)
                (gamedir / p).parent.mkdir(parents=True, exist_ok=True)
        for name, copy in rx.RETAIL_COPIES:
            man.add(copy, f"data {release}:{find(name)}")
        for name in rx.REZ:
            p = find(name)
            man.add(p, f"data {release}:{p} (game binaries inside zeroed)")
    elif cmd == "thirdparty":
        for p in files:
            if "/" not in p and rx.is_third_party(p):
                man.add(p, f"THIRD-PARTY {release}:{p}")
    else:
        for name in a[3:]:
            man.add(name, f"data {release}:{find(name)}")
    man.save()


if __name__ == "__main__":
    main()
