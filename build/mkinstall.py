#!/usr/bin/env python3
"""The install scripts of a game directory's public archive, for both games.

  build/mkinstall.py <gamedir> <outdir>

A game directory holds two retail games, each in a directory of its own
(build/all.sh): AVP2/, Aliens versus Predator 2 (Europe) with the English
1.0.9.6 patch, and PrimalHunt/, Primal Hunt (Europe). Their data -- the
.rez archives (with the retail game DLLs inside them zeroed), the movies,
the loose files the games read, the shipped settings, and Bink, Smacker
and Miles in the legacy builds -- came from the retail games (each
MANIFEST.txt's `data` and `THIRD-PARTY` rows). The public archive
(build/package.sh) leaves all of that out. Three scripts put it back from
the player's own disc images -- the two discs of AvP2 and the disc of
Primal Hunt; AvP2's 1.0.9.6 patch is a file on the Primal Hunt disc -- as
.iso images or as raw .bin/.cue images (or a .zip of one), which the
scripts convert to the .iso first:

  linux-install.sh       bash, coreutils and python3 (its zlib inflates)
  windows-install.bat    cmd running the PowerShell it carries (5.1, as every
                         Windows 10 and 11 has; .NET's DeflateStream inflates)
  mac-install.command    the Perl every Mac has, with its own modules (its
                         Compress::Raw::Zlib inflates), for CrossOver or
                         another Wine

None of them reads ISO 9660 or an InstallShield header. An image is
recognised by its size and SHA-256 (those of the .iso: a .bin is converted,
then checked), and then every byte the games need is known. This script
writes it as a list (ext/tools/mkinstall.py's docstring has its format):
the discs (D, R), the patch's payload (M), the sets of discs (S), each
retail file the games need and the ways to read it (V, W), and each file
of the game directory made from one (F, then C and Z: its bytes, and zeros
over the retail game DLLs inside an archive; E).

This script needs neither the retail games nor a disc image. It reads two
lists: build/retail-files.txt (where each retail file is on which disc,
its size and SHA-256) and build/retail-ranges.txt (the zeroed DLLs in each
archive, and the archive's SHA-256 after that). A game directory built
with the retail games is checked against them: each file the scripts write
must be in it as they write it. One built without them (NODISCS=1: it has
the file NOT-PLAYABLE.txt) has no game data to check, and gives the same
scripts.

A set of discs that does not have every file the games need (the USA
release's discs: AvP2's launcher, readme and EULA differ) is left out of
the list, and said so; the first set must be complete.

The list goes at the end of each script, after a marker line (in the Perl
one, after its __END__); the scripts' code is
build/game-scripts/linux-install.sh, windows-install.ps1 (the .bat's
header is build/retail.py's INSTALL_BAT_HEADER) and mac-install.command.

Writes <outdir>/linux-install.sh, <outdir>/windows-install.bat,
<outdir>/mac-install.command, <outdir>/disc-files.txt, the game directory's
paths the scripts write (the archive leaves them out), and
<outdir>/disc-sums.txt, the SHA-256 of each of those but the `once` files,
autoexec.cfg and Profiles/Default.prf, which the games rewrite
(build/package.sh puts them in the archive's SHA256SUMS).
"""
import hashlib
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(REPO, "build"))
import discs as disclist  # noqa: E402  (build/discs.py)

SCRIPTS = os.path.join(REPO, "build", "game-scripts")
SECTOR = 2048
RAW_SECTOR = 2352
CHUNK = 1 << 20
NODISCS_MARK = "NOT-PLAYABLE.txt"   # build/all.sh, AVP2_NODISCS=1
# the files the games rewrite: their shipped copies (<name>.retail) and the
# live file each starts, written only if there is none
SETTINGS = {"autoexec.cfg.retail": "autoexec.cfg",
            "Default.prf.retail": "Profiles/Default.prf"}


def layout():
    ns = {}
    path = os.path.join(REPO, "build", "retail.py")
    with open(path) as f:
        exec(compile(f.read(), path, "exec"), ns)
    return ns


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for b in iter(lambda: f.read(CHUNK), b""):
            h.update(b)
    return h.hexdigest()


def retail_ops(key, f, ranges):
    """(op lines, size, SHA-256) of the game directory's copy of a retail
    file: the whole file, or an archive with its game DLLs as zeros."""
    if key not in ranges.zsum:
        return [f"C {f.size}"], f.size, f.sha
    size, sha = ranges.zsum[key]
    if size != f.size:
        sys.exit(f"mkinstall: {key}: build/retail-ranges.txt and retail-files.txt "
                 "do not agree on its size")
    ops, pos = [], 0
    for off, n, _entry in sorted(ranges.zero.get(key, [])):
        if off > pos:
            ops.append(f"C {off - pos}")
        ops.append(f"Z {n}")
        pos = off + n
    if f.size > pos:
        ops.append(f"C {f.size - pos}")
    return ops, size, sha


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    ns = layout()
    games, tag, bat_header = ns["INSTALL_GAMES"], ns["INSTALL_TAG"], ns["INSTALL_BAT_HEADER"]
    marker = f"__{tag}_INSTALL_DATA__"
    game, outdir = sys.argv[1].rstrip("/"), sys.argv[2]
    nodiscs = os.path.isfile(os.path.join(game, NODISCS_MARK))
    lst = disclist.List()
    ranges = disclist.Ranges()
    retail = {(f.release, f.path.lower()): f for f in lst.files}
    set_names = [name for name, _ids in lst.sets]

    sources, vlines, records, sums, written = {}, [], [], [], []
    lacking = {}                          # set -> the files it lacks
    for g in games:
        rel_dir, release = g["dir"], g["release"]
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

        # each retail file once, with its ways to read it
        for rp in sorted(set(wanted.values()), key=str.lower):
            f = retail.get((release, rp.lower()))
            if f is None:
                sys.exit(f"mkinstall: {release}:{rp} is not in build/retail-files.txt")
            have = set()
            n = len(sources) + 1
            sources[(release, rp)] = n
            vlines.append(f"V {n} {f.size} {f.sha} {rp}")
            for sets, kind, rr in f.ways:
                have.update(sets)
                vlines.append(f"W {kind} " + " ".join(f"{i}:{o}:{k}" for i, o, k in rr))
            for s in set_names:
                if s not in have:
                    lacking.setdefault(s, []).append(f"{rel_dir}/{rp}")

        for rel in sorted(wanted, key=str.lower):
            rp = wanted[rel]
            f = retail[(release, rp.lower())]
            ops, size, sha = retail_ops(f"{release}/{f.path}", f, ranges)
            path = os.path.join(game, rel)
            if not nodiscs and (not os.path.isfile(path) or os.path.getsize(path) != size
                                or sha256(path) != sha):
                sys.exit(f"mkinstall: {rel} in {game} is not what the scripts would write "
                         "(build/retail-files.txt, build/retail-ranges.txt)")
            src = sources[(release, rp)]
            records.append(f"F {rel} {size} {sha} - {src}")
            sums.append(f"{sha}  {rel}")
            records.extend(ops)
            records.append("E")
            written.append(rel)
            base = rel[len(rel_dir) + 1:]
            if base in SETTINGS:
                live = f"{rel_dir}/{SETTINGS[base]}"
                records.append(f"F {live} {size} {sha} once {src}")
                records.extend(ops)
                records.append("E")
                written.append(live)

    # Only the sets that have every file; the first must.
    kept = []
    for name, ids in lst.sets:
        if name in lacking:
            if not kept:
                sys.exit(f"mkinstall: the {name} discs lack {', '.join(lacking[name])}")
            print(f"   install scripts: no {name} set (not on its discs: {', '.join(lacking[name])})")
            continue
        kept.append((name, ids))
    used = {i for _name, ids in kept for i in ids}

    data = [f"{tag}INSTALL 2"]
    for i, d in lst.discs.items():
        if i in used:
            data.append(f"D {i} {d.size} {d.sha} {d.name}")
            # its raw .bin: one 2352-byte sector for each 2048 bytes of the .iso
            data.append(f"R {i} {d.size // SECTOR * RAW_SECTOR}")
    for p in lst.payloads:
        if p.disc in used:
            data.append(f"M {p.id} {p.disc} {p.off} {p.size} {p.sha}")
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
          f"discs ({', '.join(n for n, _ in kept)})")


if __name__ == "__main__":
    main()
