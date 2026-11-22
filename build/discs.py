#!/usr/bin/env python3
"""Install the two retail games from your own disc images, and check them.

  build/discs.py install <retaildir> [image ...]
  build/discs.py verify  <retaildir>
  build/discs.py find    [image ...]
  build/discs.py ranges  write|check <retaildir>

The retail games are Aliens versus Predator 2 (Europe) with its English
1.0.9.6 patch, and its expansion Primal Hunt (Europe), on three discs: the
two discs of AvP2 and the disc of Primal Hunt. The patch is a file on the
Primal Hunt disc (Updates/avp2_en_096.exe). No part of the games is in this
repository. build/retail-files.txt is the list of their files: for each
file, its size, its SHA-256, and where its bytes are on which disc. This
script uses that list and nothing else to install the games, as their own
setups and the patch do. It runs nothing from the discs.

The disc images are the files you give on the command line. If you give no
files, the script looks in $AVP2SRC_ISOS, or else in isos/ at the root of
the repository. Each image can be:

  .iso   a plain image, 2048-byte sectors (your own rip, for example)
  .bin   a raw image, 2352-byte sectors (for example a redump dump), MODE1
         or MODE2
  .cue   a cue sheet: the .bin it names is used
  .zip   a zip that holds one such .bin or .iso (it is read in place;
         nothing is unpacked to the disk)

A disc is recognised by the volume label and the volume size in its file
system. The image does not have to be identical to a known image: every
file that the install writes is checked against the list (its size and its
SHA-256). Thus any correct copy of the discs works. The European discs
come first. The USA discs are a second set, but they do not have all the
files of the European release (AvP2's launcher, readme and EULA are not
the same), so the install needs the European discs.

The list has these lines (the fields are separated by one TAB):

  D <id> <volume label> <volume blocks> <image size> <image SHA-256> <name>
      One disc. The image size, SHA-256 and name are those of the .iso of
      the redump dump; another correct copy can have another size.
  M <id> <disc> <offset> <size> <SHA-256> <path on the disc>
      The patch's payload: the one folder of the Microsoft cabinet at
      <offset> of the disc's image (inside the patch program), unpacked.
      Its size and SHA-256 are those of the unpacked folder.
  S <name> <id> ...
      A set of discs that the games install from.
  F <release> <path> <size> <SHA-256> <origin>
      One installed file of one release (avp2-eu-1.0.9.6, avp2-ph-eu): its
      path in the install, its size and SHA-256, and where it comes from
      (disc: the discs; update: the patch). One or more W lines follow.
  W <sets> copy|is <id>:<offset>:<length> ...
      A way to read the file in the sets named (comma-separated): those
      byte ranges of the disc images (or of the patch's payload), joined,
      are the file (copy), or the InstallShield 6 cabinet data it is stored
      as (is: a run of chunks, each a 16-bit length and a raw deflate
      stream).

`install` writes every file of the list into <retaildir>/<release>/ (the
build uses out/retail/). A file that is already correct stays. Each file
is written to <name>.part, checked, and then renamed. Each release gets a
MANIFEST.txt (<path><TAB><origin>, which ext/tools/retailextract.py
reads). At the end the script writes <retaildir>/.retail-files, a copy of
the list that it installed. `verify` checks every file of <retaildir>
against the list. Exit status 0 if they are all there and correct, 1 if
not. `find` shows which image is which disc. Exit status 3 if a disc image
is missing: the message tells you what to supply.

`ranges` writes or checks build/retail-ranges.txt from the installed
games: the retail game DLLs inside the .rez archives, which the build
writes zeros over, and each archive's SHA-256 after that (see that file).

build/mkinstall.py reads the same two lists: the install scripts of a
public archive read the same byte ranges out of the player's disc images.
"""
import hashlib
import os
import re
import struct
import sys
import zipfile
import zlib

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LIST = os.path.join(REPO, "build", "retail-files.txt")
RANGES = os.path.join(REPO, "build", "retail-ranges.txt")
SECTOR = 2048
RAW_SECTOR = 2352
SYNC = b"\x00" + b"\xff" * 10 + b"\x00"
CHUNK = 1 << 20
EXTS = (".iso", ".bin", ".cue", ".zip")
ENV = "AVP2SRC_ISOS"


class Disc:
    def __init__(self, ident, label, blocks, size, sha, name):
        self.id, self.label, self.blocks = ident, label, int(blocks)
        self.size, self.sha, self.name = int(size), sha, name


class Payload:
    def __init__(self, ident, disc, off, size, sha, dpath):
        self.id, self.disc, self.off = ident, disc, int(off)
        self.size, self.sha, self.dpath = int(size), sha, dpath


class File:
    def __init__(self, release, path, size, sha, origin):
        self.release, self.path, self.size, self.sha = release, path, int(size), sha
        self.origin, self.ways = origin, []       # [([sets], kind, [(id, off, n)])]

    @property
    def rel(self):
        return f"{self.release}/{self.path}"


class List:
    """build/retail-files.txt."""
    def __init__(self, path=LIST):
        self.discs, self.payloads, self.sets, self.files = {}, [], [], []
        for line in open(path, encoding="utf-8"):
            line = line.rstrip("\n")
            if not line or line.startswith("#"):
                continue
            f = line.split("\t")
            if f[0] == "D":
                self.discs[f[1]] = Disc(*f[1:7])
            elif f[0] == "M":
                self.payloads.append(Payload(*f[1:7]))
            elif f[0] == "S":
                self.sets.append((f[1], f[2].split(" ")))
            elif f[0] == "F":
                self.files.append(File(*f[1:6]))
            elif f[0] == "W" and self.files:
                ranges = []
                for r in f[3].split(" "):
                    ident, off, n = r.split(":")
                    ranges.append((ident, int(off), int(n)))
                self.files[-1].ways.append((f[1].split(","), f[2], ranges))
            else:
                sys.exit(f"discs.py: {path}: a line it does not know: {line[:60]}")
        self.releases = sorted({f.release for f in self.files})


# --- disc images ------------------------------------------------------------

class Image:
    """The 2048-byte user data of one disc image, read from any of the
    accepted formats. Reads that go forward are cheap in all of them; a
    .zip goes back to the start of its member for a read that goes back."""

    def __init__(self, path):
        self.path, self.label = path, os.path.basename(path)
        self.zip = self.f = None
        src = path
        if path.lower().endswith(".cue"):
            src = self._cue(path)
        if zipfile.is_zipfile(src):
            self.zip = zipfile.ZipFile(src)
            members = [i for i in self.zip.infolist()
                       if i.filename.lower().endswith((".bin", ".iso"))]
            if len(members) != 1:
                raise ValueError("not one .bin or .iso in the zip")
            self.member = members[0]
            self.f = self.zip.open(self.member)
            self.length = self.member.file_size
        else:
            self.f = open(src, "rb")
            self.length = os.path.getsize(src)
        head = self.f.read(16)
        self.raw = head[:12] == SYNC and self.length % RAW_SECTOR == 0
        # the user data: after the 16-byte sync and header (MODE1), and
        # after the 8-byte subheader too (MODE2 form 1)
        self.skip = 24 if self.raw and head[15] == 2 else 16
        self._rewind()

    @staticmethod
    def _cue(path):
        text = open(path, encoding="latin-1").read()
        m = re.search(r'^\s*FILE\s+"([^"]+)"', text, re.M | re.I)
        if not m or not re.search(r"TRACK\s+01\s+MODE[12]/(2352|2048)", text, re.I):
            raise ValueError("not a cue sheet of one data track")
        return os.path.join(os.path.dirname(path), m.group(1))

    def _rewind(self):
        if self.zip:
            self.f.close()
            self.f = self.zip.open(self.member)
        else:
            self.f.seek(0)
        self.pos = 0

    def _seek(self, raw_off):
        if raw_off < self.pos:
            self._rewind()
        if self.zip:
            while self.pos < raw_off:      # forward: read and drop
                n = len(self.f.read(min(CHUNK, raw_off - self.pos)))
                if not n:
                    raise EOFError
                self.pos += n
        else:
            self.f.seek(raw_off)
            self.pos = raw_off

    def _read(self, n):
        b = self.f.read(n)
        self.pos += len(b)
        return b

    def chunks(self, off, n):
        """The user data bytes off .. off + n, in chunks."""
        if not self.raw:
            self._seek(off)
            while n:
                b = self._read(min(CHUNK, n))
                if not b:
                    raise EOFError
                n -= len(b)
                yield b
            return
        sec, within = divmod(off, SECTOR)
        self._seek(sec * RAW_SECTOR)
        per = CHUNK // SECTOR
        k = self.skip
        while n:
            buf = self._read(RAW_SECTOR * per)
            if not buf:
                raise EOFError
            while len(buf) % RAW_SECTOR:
                more = self._read(RAW_SECTOR - len(buf) % RAW_SECTOR)
                if not more:
                    raise EOFError
                buf += more
            data = b"".join(buf[i + k:i + k + SECTOR] for i in range(0, len(buf), RAW_SECTOR))
            data = data[within:within + n]
            within = 0
            n -= len(data)
            yield data

    def read(self, off, n):
        return b"".join(self.chunks(off, n))

    def volume(self):
        """(volume label, volume size in blocks) of its ISO 9660 file system."""
        pvd = self.read(16 * SECTOR, SECTOR)
        if pvd[1:6] != b"CD001" or pvd[0] != 1:
            return None
        return pvd[40:72].decode("ascii", "replace").rstrip(), struct.unpack_from("<I", pvd, 80)[0]


def candidates(args):
    """The image files to look at: the arguments, or the files in
    $AVP2SRC_ISOS or isos/ (and the directories given)."""
    if args:
        out = []
        for a in args:
            out += candidates_in(a) if os.path.isdir(a) else [a]
        return out
    return candidates_in(os.environ.get(ENV) or os.path.join(REPO, "isos"))


def candidates_in(d):
    if not os.path.isdir(d):
        return []
    names = sorted(n for n in os.listdir(d) if n.lower().endswith(EXTS))
    # .iso first (no conversion), and a .bin before the .cue that names it
    order = {e: i for i, e in enumerate(EXTS)}
    names.sort(key=lambda n: order[os.path.splitext(n)[1].lower()])
    return [os.path.join(d, n) for n in names]


def identify(lst, paths, quiet=False):
    """{disc id: Image} for the images among `paths` that are discs of the
    list: the first image found for each disc."""
    found, seen = {}, set()
    for p in paths:
        if len(found) == len(lst.discs):
            break
        try:
            real = os.path.realpath(Image._cue(p) if p.lower().endswith(".cue") else p)
        except (OSError, ValueError):
            real = p
        if real in seen:
            continue
        seen.add(real)
        try:
            img = Image(p)
            vol = img.volume()
        except (OSError, ValueError, EOFError, zipfile.BadZipFile) as e:
            if not quiet:
                print(f"   {os.path.basename(p)}: not read ({e})")
            continue
        for d in lst.discs.values():
            if d.id not in found and vol == (d.label, d.blocks):
                found[d.id] = img
                if not quiet:
                    print(f"   {os.path.basename(p)}: {d.name}")
                break
        else:
            if not quiet:
                print(f"   {os.path.basename(p)}: not a disc of these releases")
    return found


def choose_set(lst, found):
    """The first set whose discs are all found, or None."""
    for name, ids in lst.sets:
        if all(i in found for i in ids):
            return name, ids
    return None


def missing_message(lst, found):
    name, ids = lst.sets[0]
    lines = ["error: the retail games' disc images are missing. Bring your own discs:"]
    for i in ids:
        if i not in found:
            d = lst.discs[i]
            lines.append(f"  {d.name} (volume label \"{d.label}\")")
    lines += [
        "Put the images of the two discs of Aliens versus Predator 2 (Europe) and of",
        "the disc of Primal Hunt (Europe) in isos/ at the root of the repository (a",
        f"hard link is good: ln <image> isos/), or set {ENV}=<directory>. An image",
        "can be a .iso, a .bin with its .cue, or a .zip that holds one of them.",
        "See README.md, \"Building\".",
    ]
    return "\n".join(lines)


# --- reading a file ---------------------------------------------------------

def unpack_payload(img, p):
    """The patch's payload: the one folder of the Microsoft cabinet at
    p.off of the image, unpacked (MSZIP: deflate blocks, each with the
    previous one's output as its dictionary)."""
    head = img.read(p.off, 36 + 8)
    if head[:4] != b"MSCF":
        raise ValueError(f"no Microsoft cabinet at {p.off} of {img.label}")
    cab_size, = struct.unpack_from("<I", head, 8)
    cab = img.read(p.off, cab_size)
    off, nblocks, kind = struct.unpack_from("<IHH", cab, 36)
    pos, out, window = off, bytearray(), b""
    for _ in range(nblocks):
        _csum, cb, _cu = struct.unpack_from("<IHH", cab, pos)
        block = cab[pos + 8:pos + 8 + cb]
        pos += 8 + cb
        if kind & 15 == 0:
            piece = block
        else:
            z = zlib.decompressobj(-15, zdict=window) if window else zlib.decompressobj(-15)
            piece = z.decompress(block[2:]) + z.flush()
        out += piece
        window = bytes(piece[-32768:])
    if len(out) != p.size or hashlib.sha256(out).hexdigest() != p.sha:
        raise ValueError(f"{p.dpath} in {img.label} did not unpack to the patch's files")
    return bytes(out)


def read_ranges(found, memory, ranges):
    for ident, off, n in ranges:
        if ident in memory:
            yield memory[ident][off:off + n]
        else:
            yield from found[ident].chunks(off, n)


def inflate(gen):
    """InstallShield 6: chunks of a 16-bit length and a raw deflate stream."""
    buf = b""
    while True:
        while len(buf) < 2:
            more = next(gen, None)
            if more is None:
                if buf:
                    raise EOFError
                return
            buf += more
        n, = struct.unpack_from("<H", buf)
        while len(buf) < 2 + n:
            more = next(gen, None)
            if more is None:
                raise EOFError
            buf += more
        yield zlib.decompress(buf[2:2 + n], -15)
        buf = buf[2 + n:]


def file_chunks(found, memory, kind, ranges):
    gen = read_ranges(found, memory, ranges)
    return gen if kind == "copy" else inflate(gen)


# --- install and verify -------------------------------------------------------

def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for b in iter(lambda: f.read(CHUNK), b""):
            h.update(b)
    return h.hexdigest()


def correct(path, f):
    return os.path.isfile(path) and os.path.getsize(path) == f.size and \
        sha256_file(path) == f.sha


def write_manifests(lst, outdir, origins):
    for rel in lst.releases:
        with open(os.path.join(outdir, rel, "MANIFEST.txt"), "w") as out:
            out.write(f"# {rel}: where each file came from (build/discs.py, "
                      "build/retail-files.txt)\n")
            for f in sorted((f for f in lst.files if f.release == rel), key=lambda f: f.path.lower()):
                out.write(f"{f.path}\t{origins.get(f.rel, f.origin)}\n")


def install(outdir, args):
    lst = List()
    found = identify(lst, candidates(args))
    chosen = choose_set(lst, found)
    if not chosen:
        print(missing_message(lst, found), file=sys.stderr)
        return 3
    set_name, ids = chosen
    print(f"   installing from the {set_name} discs")
    os.makedirs(outdir, exist_ok=True)
    stamp = os.path.join(outdir, ".retail-files")
    if os.path.exists(stamp):
        os.unlink(stamp)                   # not whole until the end
    memory = {}
    todo = []
    for f in lst.files:
        way = next((w for w in f.ways if set_name in w[0]), None)
        if way is None:
            print(f"error: {f.rel} is not on the {set_name} discs", file=sys.stderr)
            return 1
        todo.append((f, way))
    written = kept = 0
    origins = {}

    def first(item):
        ident, off, _n = item[1][2][0]
        return (ident in memory or ident not in found, ident, off)
    # forward through each image (a .zip is read once), the patch's files last
    for f, (_sets, kind, ranges) in sorted(todo, key=first):
        dest = os.path.join(outdir, f.release, f.path)
        if f.origin == "update":
            p = next(p for p in lst.payloads if p.disc in ids)
            origins[f.rel] = f"{lst.discs[p.disc].name}:{p.dpath} (the 1.0.9.6 patch)"
        else:
            discs = sorted({i for i, _o, _n in ranges if i in lst.discs})
            origins[f.rel] = " + ".join(lst.discs[i].name for i in discs)
        if correct(dest, f):
            kept += 1
            continue
        for ident, _o, _n in ranges:
            if ident not in found and ident not in memory:
                p = next((p for p in lst.payloads if p.id == ident and p.disc in found), None)
                if p is None:
                    print(f"error: {f.rel}: no disc has {ident}", file=sys.stderr)
                    return 1
                memory[ident] = unpack_payload(found[p.disc], p)
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        part = dest + ".part"
        h = hashlib.sha256()
        try:
            with open(part, "wb") as out:
                for b in file_chunks(found, memory, kind, ranges):
                    h.update(b)
                    out.write(b)
        except (EOFError, zlib.error):
            os.unlink(part)
            print(f"error: {f.rel}: an image ends early, or its data is damaged: "
                  "it is not a whole copy of its disc", file=sys.stderr)
            return 1
        if h.hexdigest() != f.sha or os.path.getsize(part) != f.size:
            os.unlink(part)
            print(f"error: {f.rel} came out wrong (SHA-256 {h.hexdigest()}, the list has "
                  f"{f.sha}): an image is not a correct copy of its disc", file=sys.stderr)
            return 1
        os.replace(part, dest)
        written += 1
    write_manifests(lst, outdir, origins)
    with open(LIST, "rb") as src, open(stamp, "wb") as out:
        out.write(src.read())
    print(f"   {written} files written, {kept} already correct -> {outdir}")
    return 0


def verify(outdir):
    lst = List()
    bad = []
    for f in lst.files:
        p = os.path.join(outdir, f.release, f.path)
        if not os.path.isfile(p):
            bad.append(f"missing  {f.rel}")
        elif os.path.getsize(p) != f.size or sha256_file(p) != f.sha:
            bad.append(f"changed  {f.rel}")
    for line in bad:
        print(f"   {line}")
    if bad:
        return 1
    print(f"   all {len(lst.files)} files match build/retail-files.txt")
    return 0


# --- build/retail-ranges.txt ------------------------------------------------

RANGES_HEAD = """\
## What the build changes in the retail games' files that it copies: the
## retail game DLLs inside the .rez archives, which it writes zeros over
## (ext/tools/retailextract.py resources). This list holds no game data:
## only names, offsets, sizes and SHA-256 values. build/discs.py writes it
## from the installed retail games (`make retail-ranges`), and every build
## with the retail games checks it. A build without them (NODISCS=1) and the
## install scripts of a public archive (build/mkinstall.py) use it in place
## of the retail games.
##
## ZERO <release> <archive> <offset> <size> <entry>
##   A retail game DLL in an archive: the build writes zeros over its data
##   in the game directory's copy. The offset is in the archive.
## ZSUM <release> <archive> <size> <SHA-256>
##   The game directory's copy of that archive, with those zeros.
## The fields are separated by one TAB.
"""


class Ranges:
    def __init__(self, path=RANGES):
        self.zero, self.zsum = {}, {}          # "<release>/<archive>" -> ...
        if not os.path.exists(path):
            return
        for line in open(path, encoding="utf-8"):
            line = line.rstrip("\n")
            if not line or line.startswith("#"):
                continue
            f = line.split("\t")
            key = f"{f[1]}/{f[2]}"
            if f[0] == "ZERO":
                self.zero.setdefault(key, []).append((int(f[3]), int(f[4]), f[5]))
            elif f[0] == "ZSUM":
                self.zsum[key] = (int(f[3]), f[4])
            else:
                sys.exit(f"discs.py: {path}: a line it does not know: {line[:60]}")


def layouts():
    """{release: the layout's REZ names} (build/retail.py, retail_avp2.py)."""
    out = {}
    for name in ("retail.py", "retail_avp2.py"):
        path = os.path.join(REPO, "build", name)
        ns = {}
        exec(compile(open(path).read(), path, "exec"), ns)
        out[name] = ns
    games = out["retail.py"]["INSTALL_GAMES"]
    return {g["release"]: out[os.path.basename(g["layout"])] for g in games}


def ranges_text(retaildir):
    sys.path.insert(0, os.path.join(REPO, "ext", "tools"))
    import retailextract as rx  # noqa: E402  (ext/tools: rez_pe_entries)
    lines = [RANGES_HEAD.rstrip("\n")]
    for rel, ns in sorted(layouts().items()):
        for name in ns["REZ"]:
            path = os.path.join(retaildir, rel, name)
            hits = rx.rez_pe_entries(path)
            if not hits:
                continue
            data = bytearray(open(path, "rb").read())
            for ename, pos, sz in sorted(hits, key=lambda h: h[1]):
                lines.append(f"ZERO\t{rel}\t{name}\t{pos}\t{sz}\t{ename}")
                data[pos:pos + sz] = bytes(sz)
            lines.append(f"ZSUM\t{rel}\t{name}\t{len(data)}\t{hashlib.sha256(data).hexdigest()}")
    return "\n".join(lines) + "\n"


def ranges(mode, retaildir):
    text = ranges_text(retaildir)
    if mode == "write":
        with open(RANGES, "w", encoding="utf-8") as f:
            f.write(text)
        print(f"   wrote {RANGES}")
        return 0
    old = open(RANGES, encoding="utf-8").read() if os.path.exists(RANGES) else ""
    if old != text:
        print("error: build/retail-ranges.txt is not what the retail games give: "
              "run make retail-ranges", file=sys.stderr)
        return 1
    print("   build/retail-ranges.txt matches the retail games")
    return 0


def main():
    a = sys.argv[1:]
    if a[:1] == ["install"] and len(a) >= 2:
        return install(a[1], a[2:])
    if a[:1] == ["verify"] and len(a) == 2:
        return verify(a[1])
    if a[:1] == ["find"]:
        lst = List()
        found = identify(lst, candidates(a[1:]))
        if not choose_set(lst, found):
            print(missing_message(lst, found), file=sys.stderr)
            return 3
        return 0
    if a[:1] == ["ranges"] and len(a) == 3 and a[1] in ("write", "check"):
        return ranges(a[1], a[2])
    sys.exit(__doc__)


if __name__ == "__main__":
    sys.exit(main())
