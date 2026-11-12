#!/usr/bin/env python3
"""The install scripts of a game directory's public archive, for both games.

  build/mkinstall.py <gamedir> <outdir> [isodir]

ext/tools/mkinstall.py's scripts and list format (its docstring documents both),
for a game directory that holds two retail games, each in a directory of its
own (build/all.sh): AVP2/, Aliens versus Predator 2 (Europe) with the English
1.0.9.6 patch, and PrimalHunt/, Primal Hunt (Europe). build/retail.py's
INSTALL_* names say which: INSTALL_GAMES (each game's directory, the ext/rez/
release its data came from, the disc images of each region it installs
from, and its update), INSTALL_DISCS, INSTALL_SETS, INSTALL_TAG,
INSTALL_BAT_HEADER, and INSTALL_UPDATE_ON: the update -- AvP2's 1.0.9.6
patch -- is a file on Primal Hunt's disc, so its payload (the M record) is
read from a range of that disc's image, and the scripts need no other file.
Each game directory's MANIFEST.txt (<gamedir>/<dir>/MANIFEST.txt) says what
in it came from the retail game; the list's paths are the game directory's
(AVP2/ALIEN.REZ, PrimalHunt/AVP2X.REZ, ...).

A set whose images do not hold every retail file (another region's release
that differs) is left out of the list, and said so; the first set must be
complete. Exits 3 if an image is missing, as ext/tools/mkinstall.py does.
"""
import hashlib
import os
import re
import sys
import tarfile

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(REPO, "ext", "tools"))
import mkinstall as M  # noqa: E402  (ext/tools/mkinstall.py)

REZ = M.REZ
SCRIPTS = os.path.join(REPO, "build", "game-scripts")
SETTINGS = {"autoexec.cfg.retail": "autoexec.cfg",
            "Default.prf.retail": "Profiles/Default.prf"}


def layout():
    ns = {}
    path = os.path.join(REPO, "build", "retail.py")
    with open(path) as f:
        exec(compile(f.read(), path, "exec"), ns)
    return ns


class Unavailable(Exception):
    pass


def in_cabs(cs, path, size, md5, what):
    for _name, flags, exp, fmd5, ranges in cs.files:
        if exp == size and fmd5 == md5:
            if flags & M.FILE_OBFUSCATED:
                sys.exit(f"mkinstall: {what}: obfuscated in its cabinet")
            return ("is" if flags & M.FILE_COMPRESSED else "copy"), ranges
    raise Unavailable(what)


def main():
    if len(sys.argv) not in (3, 4):
        sys.exit(__doc__)
    ns = layout()
    games, discnames, sets = ns["INSTALL_GAMES"], ns["INSTALL_DISCS"], ns["INSTALL_SETS"]
    update_on, tag, bat_header = ns["INSTALL_UPDATE_ON"], ns["INSTALL_TAG"], ns["INSTALL_BAT_HEADER"]
    marker = f"__{tag}_INSTALL_DATA__"
    game, outdir = sys.argv[1].rstrip("/"), sys.argv[2]
    isodir = sys.argv[3] if len(sys.argv) == 4 else os.path.join(REZ, "isos")

    paths = {i: os.path.join(isodir, n + ".iso") for i, n in discnames.items()}
    missing = [p for p in paths.values() if not os.path.isfile(p)]
    retail = {g["release"]: os.path.join(REZ, "games", g["release"]) for g in games}
    nogame = [r for r, d in retail.items() if not os.path.isfile(os.path.join(d, "MANIFEST.txt"))]
    if missing or nogame:
        print("mkinstall: no disc images (%s) or no ext/rez/games/%s/ -- "
              "ext/rez/fetch-isos.sh and ext/rez/install-games.sh"
              % (", ".join(missing) or "-", ", ".join(nogame) or "-"), file=sys.stderr)
        sys.exit(3)
    discs = {i: M.Disc(i, discnames[i], p) for i, p in paths.items()}

    # The update, from ext/rez/'s copy: its payload unpacked, its cabinets; and
    # where each disc that carries it has it (checked to be the same file).
    upd = pay = ucabs = None
    mlines = []
    for g in games:
        if not g.get("update"):
            continue
        if upd:
            sys.exit("mkinstall: one update only")
        archive = os.path.join(REZ, "artifacts", g["update"] + ".tar.xz")
        if not os.path.isfile(archive):
            import subprocess
            subprocess.run([os.path.join(REZ, "assemble-artifacts.sh"), os.path.basename(g["update"])],
                           check=True, stdout=subprocess.DEVNULL)
        with tarfile.open(archive, "r:xz") as tar:
            members = [m for m in tar.getmembers() if m.isfile()]
            exe = tar.extractfile(members[0]).read()
        upd = M.Image("upd", os.path.basename(g["update"]), data=exe)
        cab_at = M.find_cabinet(exe)
        payload, pfiles = M.unpack_folder(exe, cab_at)
        pay = M.Image("pay", "the update's cabinet", data=payload)
        hdrs = [k for k in pfiles if k.endswith("data1.hdr")]
        if len(hdrs) != 1:
            sys.exit(f"mkinstall: {upd.name}: {len(hdrs)} data1.hdr in its cabinet")
        prefix = hdrs[0][:-len("data1.hdr")]
        hdr_off, hdr_size = pfiles[hdrs[0]]
        vols, n = {}, 1
        while prefix + f"data{n}.cab" in pfiles:
            vols[n] = (pay,) + pfiles[prefix + f"data{n}.cab"]
            n += 1
        ucabs = M.Cabinets(pay.read(hdr_off, hdr_size), vols)
        for i, name in update_on.items():
            off, size = discs[i].files[name.lower()]
            if size != upd.size or M.digest([(discs[i], off, size)]) != upd.sha:
                sys.exit(f"mkinstall: {discs[i].name}: {name} is not {g['update']}")
            mlines.append(f"M {pay.id} {i} {off + cab_at} {pay.size} {pay.sha}")

    # Each game's retail files, the ways to read them, for each region.
    sources, vlines, records, sums, written = {}, [], [], [], []
    literal = 0
    lacking = {}                          # region -> the files it lacks
    for g in games:
        rel_dir, release, root = g["dir"], g["release"], retail[g["release"]]
        cabs = {}
        for region, ids in g["regions"].items():
            def find(name, ids=ids):
                for i in ids:
                    if name in discs[i].files:
                        return (discs[i],) + discs[i].files[name]
                raise KeyError(name)
            img, off, size = find("data1.hdr")
            vols, n = {}, 1
            while True:
                try:
                    vols[n] = find(f"data{n}.cab")
                except KeyError:
                    break
                n += 1
            cabs[region] = M.Cabinets(img.read(off, size), vols)

        origin = {}
        for line in open(os.path.join(root, "MANIFEST.txt"), errors="replace"):
            if line.startswith("#") or "\t" not in line:
                continue
            rp, frm = line.rstrip("\n").split("\t", 1)
            origin[rp] = frm

        wanted = {}                       # game path -> retail path
        for line in open(os.path.join(game, rel_dir, "MANIFEST.txt")):
            line = line.rstrip("\n")
            if line.startswith("#") or "\t" not in line:
                continue
            frm, rel = line.split("\t", 1)
            m = re.match(r"(data|THIRD-PARTY) ([^:]+):(.+?)( \(.*\))?$", frm)
            if not m:
                continue
            if m.group(2) != release:
                sys.exit(f"mkinstall: {rel_dir}/{rel} came from {m.group(2)}, not {release}")
            wanted[f"{rel_dir}/{rel}"] = m.group(3)
        for rel in list(wanted) + list(wanted.values()):
            if re.search(r"[\s\"'`$\\%!]", rel):
                sys.exit(f"mkinstall: {rel}: a name the scripts do not quote")

        for rp in sorted(set(wanted.values()), key=str.lower):
            path = os.path.join(root, rp)
            md5, size = hashlib.md5(), os.path.getsize(path)
            with open(path, "rb") as f:
                for b in iter(lambda: f.read(M.CHUNK), b""):
                    md5.update(b)
            md5, sha = md5.digest(), M.sha256(path)
            frm = origin.get(rp, "")
            ways = []
            if g.get("update") and frm.startswith(g["update"] + ":"):
                try:
                    ways.append(in_cabs(ucabs, path, size, md5, rp))
                except Unavailable:
                    sys.exit(f"mkinstall: {rp}: not in the update's cabinets")
            else:
                for region, ids in g["regions"].items():
                    try:
                        if ".iso:" in frm:        # stored as it is on the first disc
                            name = frm.split(".iso:", 1)[1].lower()
                            disc = discs[ids[0]]
                            if name not in disc.files:
                                raise Unavailable(rp)
                            off, n = disc.files[name]
                            way = ("copy", [(disc, off, n)])
                            if n != size or M.digest(way[1]) != sha:
                                raise Unavailable(rp)
                            ways.append(way)
                        else:
                            ways.append(in_cabs(cabs[region], path, size, md5, rp))
                    except Unavailable:
                        lacking.setdefault(region, []).append(f"{rel_dir}/{rp}")
            lines = []
            for kind, ranges in ways:
                M.check_way(kind, ranges, size, sha, rp)
                line = M.way_line(kind, ranges)
                if line not in lines:
                    lines.append(line)
            n = len(sources) + 1
            sources[(release, rp)] = n
            vlines.append(f"V {n} {size} {sha} {rp}")
            vlines += lines

        for rel in sorted(wanted, key=str.lower):
            rp = wanted[rel]
            path = os.path.join(game, rel)
            ops = M.ops_for(os.path.join(root, rp), path)
            if ops is None:
                sys.exit(f"mkinstall: {rel}: not made from {release}:{rp}")
            literal += ops.literal
            size, sha = os.path.getsize(path), M.sha256(path)
            src = sources[(release, rp)]
            records.append(f"F {rel} {size} {sha} - {src}")
            sums.append(f"{sha}  {rel}")
            records.extend(ops.lines())
            records.append("E")
            written.append(rel)
            base = rel[len(rel_dir) + 1:]
            if base in SETTINGS:
                live = f"{rel_dir}/{SETTINGS[base]}"
                records.append(f"F {live} {size} {sha} once {src}")
                records.extend(ops.lines())
                records.append("E")
                written.append(live)
    if literal > M.LITERAL_MAX:
        sys.exit(f"mkinstall: {literal} bytes would ship as literal data")

    # Only the sets whose regions have every file; the first must.
    kept = []
    for name, ids in sets:
        if name in lacking:
            if not kept:
                sys.exit(f"mkinstall: the {name} images lack {', '.join(lacking[name])}")
            print(f"   install scripts: no {name} set (not on its discs: {', '.join(lacking[name])})")
            continue
        kept.append((name, ids))
    used = {i for _name, ids in kept for i in ids}

    data = [f"{tag}INSTALL 2"]
    for i, d in discs.items():
        if i in used:
            data.append(f"D {i} {d.size} {d.sha} {d.name}")
            data.append(f"R {i} {d.size // M.SECTOR * M.RAW_SECTOR}")
    data += [m for m in mlines if m.split(" ")[2] in used]
    for name, ids in kept:
        data.append(f"S {name} {' '.join(ids)}")
    data += vlines + records

    os.makedirs(outdir, exist_ok=True)
    body = open(os.path.join(SCRIPTS, "linux-install.sh")).read()
    with open(os.path.join(outdir, "linux-install.sh"), "w", newline="\n") as f:
        f.write(body + marker + "\n" + "\n".join(data) + "\n")
    os.chmod(os.path.join(outdir, "linux-install.sh"), 0o755)

    perl = open(os.path.join(SCRIPTS, "mac-install.command")).read()
    if not perl.endswith("\n__END__\n"):
        sys.exit("mkinstall: mac-install.command must end with __END__ (the list follows it)")
    with open(os.path.join(outdir, "mac-install.command"), "w", newline="\n") as f:
        f.write(perl + marker + "\n" + "\n".join(data) + "\n")
    os.chmod(os.path.join(outdir, "mac-install.command"), 0o755)

    ps = open(os.path.join(SCRIPTS, "windows-install.ps1")).read()
    psm = f"#__{tag}_PS__"
    if not ps.isascii() or psm in ps or marker in ps:
        sys.exit("mkinstall: windows-install.ps1 must be ASCII, without the markers")
    bat = bat_header.splitlines() + [psm] + ps.splitlines() + [psm, marker] + data
    with open(os.path.join(outdir, "windows-install.bat"), "w", newline="\r\n") as f:
        f.write("\n".join(bat) + "\n")

    with open(os.path.join(outdir, "disc-files.txt"), "w") as f:
        for rel in sorted(written, key=str.lower):
            f.write(rel + "\n")
    with open(os.path.join(outdir, "disc-sums.txt"), "w", newline="\n") as f:
        f.write("".join(line + "\n" for line in sums))
    print(f"   install scripts: {len(written)} files from {len(sources)} retail files on the "
          f"discs ({', '.join(n for n, _ in kept)}), {literal} bytes of ours")


if __name__ == "__main__":
    main()
