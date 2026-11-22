#!/usr/bin/env python3
"""The retail art our binaries are compiled with, in assets/art/.

  build/art.py write <retaildir>    write assets/art/ again from the retail games
  build/art.py check <retaildir>    check assets/art/ against the retail games

Two of our programs have resources whose art is not in Monolith's source
release: the launchers (src/launcher/launcher.rc: AVP2.exe's and
PrimalHunt.exe's icons, cursor, bitmaps, sounds and animation) and the
dedicated server of Primal Hunt (src/AVP2/AVP2Serv/ServerApp.rc with
PRIMAL_HUNT: AVP2XServ.exe's splash bitmap). That art is the retail
games'. It is committed, unchanged, in assets/art/ (assets/README.md):

  assets/art/launcher/avp2/   AVP2.exe's art (avp2-eu-1.0.9.6)
  assets/art/launcher/ph/     PrimalHunt.exe's art (avp2-ph-eu)
  assets/art/avp2xserv/       AVP2XServ.exe's splash bitmap (avp2-ph-eu)

build/build.mk copies these files into the directories the resource
scripts include them from. So every build has them, with or without the
retail games, and every binary is the same.

ext/tools/rsrc2rc.py reads the art out of the retail binaries in
<retaildir> (out/retail/: build/retail.sh). `write` (make art) writes it
into assets/art/. `check` (make art-check; every build with the retail
games runs it) writes it into a temporary directory and compares: exit
status 1 if a file differs, is missing or is extra.
"""
import filecmp
import os
import shutil
import subprocess
import sys
import tempfile

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ART = os.path.join(REPO, "assets", "art")
RSRC2RC = os.path.join(REPO, "ext", "tools", "rsrc2rc.py")

# <assets/art/ directory>: (<retail image>, <rsrc2rc --name>, {<file rsrc2rc
# writes>: <file in assets/art/>} or None for every file but the .rc)
SETS = {
    "launcher/avp2": ("avp2-eu-1.0.9.6/AVP2.exe", "launcher", None),
    "launcher/ph": ("avp2-ph-eu/PrimalHunt.exe", "launcher", None),
    "avp2xserv": ("avp2-ph-eu/AVP2XServ.exe", "retail", {"retail_142.bmp": "splash.bmp"}),
}


def extract(retaildir, dest):
    """Every set's files into dest/<set>/."""
    for d, (image, name, keep) in SETS.items():
        work = tempfile.mkdtemp(prefix="art-")
        try:
            subprocess.run([sys.executable, RSRC2RC, os.path.join(retaildir, image),
                            os.path.join(work, "retail.rc"), "--res-dir=.", f"--name={name}"],
                           check=True, stdout=subprocess.DEVNULL)
            out = os.path.join(dest, d)
            os.makedirs(out, exist_ok=True)
            files = sorted(f for f in os.listdir(work) if not f.endswith(".rc"))
            pairs = [(f, f) for f in files] if keep is None else sorted(keep.items())
            for src, to in pairs:
                shutil.copyfile(os.path.join(work, src), os.path.join(out, to))
        finally:
            shutil.rmtree(work)


def listing(root):
    out = set()
    for d, _dirs, files in os.walk(root):
        for f in files:
            out.add(os.path.relpath(os.path.join(d, f), root))
    return out


def main():
    if len(sys.argv) != 3 or sys.argv[1] not in ("write", "check"):
        sys.exit(__doc__)
    mode, retaildir = sys.argv[1], sys.argv[2]
    if mode == "write":
        for d in SETS:
            shutil.rmtree(os.path.join(ART, d), ignore_errors=True)
        extract(retaildir, ART)
        print(f"   wrote assets/art/ ({len(listing(ART))} files) from {retaildir}")
        return 0
    tmp = tempfile.mkdtemp(prefix="art-check-")
    try:
        extract(retaildir, tmp)
        want, have = listing(tmp), listing(ART) if os.path.isdir(ART) else set()
        have = {f for f in have if f.split(os.sep, 1)[0] in {d.split("/")[0] for d in SETS}}
        bad = [f"missing  assets/art/{f}" for f in sorted(want - have)]
        bad += [f"extra    assets/art/{f}" for f in sorted(have - want)]
        bad += [f"differs  assets/art/{f}" for f in sorted(want & have)
                if not filecmp.cmp(os.path.join(tmp, f), os.path.join(ART, f), shallow=False)]
    finally:
        shutil.rmtree(tmp)
    for line in bad:
        print(f"   {line}")
    if bad:
        print("error: assets/art/ is not the retail art: run make art", file=sys.stderr)
        return 1
    print(f"   assets/art/ is the retail art ({len(want)} files)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
