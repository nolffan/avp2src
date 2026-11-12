#!/usr/bin/env python3
"""Draft an engine image's full-length vtables (tools/mkvtabobj.py specs).

Why: much of the engine is reconstructed through stand-in ("view") classes
that declare only the virtual slots their own code touches. The table such
a class installs is one or two slots long where the shipped one has dozens,
and the game DLLs call through the shipped layout: AvP2's cshell called
ILTClient slot 53 through CLTClientQ404's two-slot table and ran
CObjectBank0c::Term, the next table in .rdata, on the client interface.

How: tools/vtabcheck.py --all maps each of our `??_7` tables to the shipped
table it stands for (through the placed functions' relocations); the
shipped table's slots are build/reference.sh's <stem>-vtables.csv
(tools/vtabaudit.py, every run of code pointers in .rdata). A table goes in
the spec when our compiled one is SHORTER than the shipped one (a stand-in;
a full-length compiled table is the source's own declaration and stays,
unless it has a `_purecall` where the shipped slot is concrete).
Each shipped slot becomes the symbol of ours the scoreboard places at the
slot's target (tools/progress.py: identical functions first, then the
small ones placed by shape, then the near pairings); a compiled slot is
kept where it is placed there or nothing is; `_purecall` stays
`__purecall`. A slot with nothing of ours placed at its target is written
`MISSING 0x<shipped>`, which mkvtabobj refuses: each is a function to
write (or to bind with evidence), so this tool exits 1 when there is one.

Usage (from the repository root, in the build container or wherever
tools/progress.py runs):
  build/vtables/mkvtables.py <target label> <vtabcheck --all output> <out spec>
      [--drop=SYM,...]   tables never to replace
      [--purecall=0x..]  the shipped _purecall (default: the commonest slot target,
                         when it is the static CRT's)
      [--map=FILE]       more `<sym> 0x<shipped table>` mappings, by hand
build/vtables/gen4.sh runs it for the four engine images.
"""
import bisect
import collections
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(REPO, "tools"))
import progress  # noqa: E402
import tgt as tgt_mod  # noqa: E402
import vtabcheck  # noqa: E402


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    opts = dict((a.split("=", 1) + [""])[:2] for a in sys.argv[1:] if a.startswith("--"))
    label, vtout, outp = args
    drop = set(filter(None, opts.get("--drop", "").split(",")))
    os.chdir(REPO)
    tgt = tgt_mod.find(label)
    ship_path = tgt["shipped"]
    stem = os.path.basename(tgt_mod.ourmap(tgt) or "")
    ours_img = tgt_mod.require_built(tgt)
    csv = tgt.get("vtables_csv")
    if not csv:
        base = os.path.splitext(os.path.basename(ship_path))[0]
        pre = "AVP2" if label.startswith("AvP2 ") else ""
        csv = os.path.join("out", "reference", pre + base + "-vtables.csv")
    pc = int(opts["--purecall"], 16) if opts.get("--purecall") else \
        (tgt.get("purecall") or vtabcheck.find_purecall(os.path.join(REPO, ship_path)))

    # the shipped tables, slot by slot
    rows = collections.defaultdict(dict)
    for line in open(csv):
        p = line.strip().split(",")
        if p[0] == "table":
            continue
        rows[int(p[0], 16)][int(p[2])] = int(p[3], 16)
    if not pc:
        # the static CRT's _purecall (`push 19h; call _amsg_exit`): the
        # commonest slot target of all, every abstract table's filler
        ship = vtabcheck.Img(os.path.join(REPO, ship_path))
        common = collections.Counter(t for r in rows.values() for t in r.values()).most_common(1)
        if common and ship.dword(common[0][0]) & 0xFFFFFF == 0xE8196A:
            pc = common[0][0]

    # our tables -> the shipped table each stands for (vtabcheck's mapping)
    maps = []
    for line in open(vtout):
        m = re.match(r"(\?\?_7\S+)\s+ours 0x([0-9a-f]+) \((\d+) slots\)\s+shipped 0x([0-9a-f]+)", line)
        if m:
            maps.append((m.group(1), int(m.group(4), 16)))
        m = re.match(r"(\?\?_7\S+)\s+ours 0x[0-9a-f]+ agrees slot for slot with shipped 0x([0-9a-f]+)", line)
        if m:
            maps.append((m.group(1), int(m.group(2), 16)))

    # ... and the ones it cannot map (no placed function installs them),
    # mapped by hand: build/vtables/map-<dir>-<game>.txt, `<sym> 0x<shipped>`
    if opts.get("--map") and os.path.exists(opts["--map"]):
        for line in open(opts["--map"]):
            w = line.split("#", 1)[0].split()
            if len(w) == 2:
                maps = [m for m in maps if m[0] != w[0]] + [(w[0], int(w[1], 16))]

    # what our compiled objects declare
    compiled = {}
    for o in progress.image_objs(tgt, ours_img):
        # (not our own tables' object, tools/mkvtabobj.py's, once linked)
        if o.lower().endswith(".obj") and os.path.basename(o).lower() != "vtables.obj":
            for k, v in vtabcheck.compiled_vtables(open(os.path.join(REPO, o), "rb").read()).items():
                if len(v) > len(compiled.get(k, [])):
                    compiled[k] = v

    # shipped address -> our symbols placed there, best evidence first
    progress.score_target(tgt)
    ship2sym = collections.defaultdict(list)
    for rank, src in enumerate((progress.PLACED_SYMS, progress.SMALL_SYMS)):
        for (lb, a), n in sorted(src.items()):
            if lb == label:
                ship2sym[a].append((rank, n.split(":", 1)[1]))
    for (lb, a, _sz, n, _our) in progress.NEAR_PAIRS:
        if lb == label:
            ship2sym[a].append((2, n.split(":", 1)[1]))
    # the symbols another object can name: the map's publics (its "Static
    # symbols" are file-local, and so are the compiler's _$E initialisers)
    defined = set()
    for line in open(os.path.join(REPO, tgt_mod.ourmap(tgt)), errors="replace"):
        if line.strip() == "Static symbols":
            break
        m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+[0-9a-f]{8}\s", line)
        if m and not m.group(1).startswith("_$E"):
            defined.add(m.group(1))

    def rank_of(st, n):
        return min([r for r, x in ship2sym.get(st, ()) if x == n or x == "??_E" + n[4:]] or [0])

    s2a, a2s = vtabcheck.load_map(os.path.join(REPO, tgt_mod.ourmap(tgt)))

    def placed_at(st):
        c = sorted(ship2sym.get(st, ()))
        got = [n for _r, n in c if n in defined and not n.startswith("??_E")] + \
              ["??_G" + n[4:] for _r, n in c if n.startswith("??_E") and "??_G" + n[4:] in defined]
        if not got:
            # only a file-local function is placed there: a public one our
            # /OPT:ICF folded onto the same body is the same code (the
            # shipped `return 1` ILTMessage slot is a static's twin)
            for _r, n in c:
                got += sorted(x for x in a2s.get(s2a.get(n), ()) if x in defined)
        if not got:
            got = same_return(st)
        return got

    # Last: a slot whose shipped body is only `return <constant>` (no stack
    # arguments, a bare `ret`) is any public function of ours with that body
    # -- ILTMessage's 37th slot, one past the SDK header's 36, is a
    # `return 1` the shipped links folded onto library code.
    ship_img = vtabcheck.Img(os.path.join(REPO, ship_path))
    our_img = vtabcheck.Img(os.path.join(REPO, ours_img))

    def const_ret(img, va):
        if va is None or img.dword(va) is None or img.dword(va + 4) is None:
            return None
        b = b"".join(img.dword(va + i).to_bytes(4, "little") for i in (0, 4))
        for pat, n in ((b"\xb8", 5), (b"\x6a", 2)):
            if b.startswith(pat):
                v = int.from_bytes(b[1:n], "little", signed=(pat == b"\x6a"))
                if b[n:n + 1] == b"\xc3":
                    return v
                if pat == b"\x6a" and b[n:n + 2] == b"\x58\xc3":
                    return v
        for pre, v in ((b"\x33\xc0\xc3", 0), (b"\x33\xc0\x40\xc3", 1)):
            if b.startswith(pre):
                return v
        return None

    def same_return(st):
        v = const_ret(ship_img, st)
        if v is None:
            return []
        return sorted(n for a, ns in a2s.items() for n in ns
                      if n in defined and not n.startswith("??") and const_ret(our_img, a) == v)[:1]

    # Same-shape twins. Two bodies that differ only in the data they reach
    # (CLTClient's GetGlobalLightDir and GetGlobalLightColor: three loads
    # from g_RenderStruct+0x138, or +0x144) are placed either way round, so
    # a slot taken from the placement alone can run the other one. Our
    # data symbols' shipped addresses come from every placed function (the
    # commonest of the shipped dwords where ours reach the symbol); a
    # candidate whose data operands read back elsewhere in the shipped
    # body is swapped for the twin of ours whose operands do.
    starts = sorted(set(s2a.values()))

    def body(a):
        i = bisect.bisect_right(starts, a)
        return a, (starts[i] if i < len(starts) else a + 0x40)

    def isdata(img, v):
        return v is not None and any(lo <= v < lo + vs for name, lo, vs, rs, rp in img.secs
                                     if name in (".data", ".rdata", ".bss"))
    dsyms = sorted((a, n) for n, a in s2a.items() if isdata(our_img, a))
    dstarts = [a for a, _ in dsyms]

    def dsym(v):
        i = bisect.bisect_right(dstarts, v) - 1
        return dsyms[i] if i >= 0 else (None, None)
    votes = collections.defaultdict(collections.Counter)
    for (lb, sa), n in progress.PLACED_SYMS.items():
        if lb != label or n.split(":", 1)[1] not in s2a:
            continue
        a, e = body(s2a[n.split(":", 1)[1]])
        for q in range(a, e - 3):
            v = our_img.dword(q)
            if isdata(our_img, v):
                sv = ship_img.dword(sa + q - a)
                if isdata(ship_img, sv):
                    da, dn = dsym(v)
                    if dn:
                        votes[dn][sv - (v - da)] += 1
    dbase = {dn: c.most_common(1)[0][0] for dn, c in votes.items()}

    def refs(a):
        a, e = body(a)
        return [(q - a, v) for q in range(a, e - 3) for v in [our_img.dword(q)] if isdata(our_img, v)]

    sym2ship = collections.defaultdict(set)
    for st_, names in ship2sym.items():
        for r_, n_ in names:
            if r_ == 0:   # (identical placements only: a shape is no evidence of a call)
                sym2ship[n_].add(st_)

    def calls(a):
        """(offset, our target) of each rel32 call in our body at a"""
        a, e = body(a)
        out_ = []
        for q in range(a, e - 4):
            if (our_img.dword(q) or 0) & 0xFF == 0xE8:
                rel = our_img.dword(q + 1)
                if rel is not None:
                    out_.append((q - a, (q + 5 + rel - (1 << 32 if rel & 0x80000000 else 0)) & 0xFFFFFFFF))
        return out_

    def fits(n, st):
        """None: nothing to tell by; else whether every data operand of ours
        with a known shipped base, and every call of ours to a function the
        scoreboard places, reads back the same in the shipped body (a
        deleting destructor's twins differ only in the destructor called)"""
        told = None
        for off, v in refs(s2a[n]):
            da, dn = dsym(v)
            if dn in dbase:
                told = True
                if ship_img.dword(st + off) != dbase[dn] + (v - da):
                    return False
        for off, t in calls(s2a[n]):
            if (ship_img.dword(st + off) or 0) & 0xFF != 0xE8:
                continue
            # (the shipped engine has two copies of the allocator front end
            # where our link has one: tools/vtabcheck.py's TWINS)
            if set(a2s.get(t, ())) & vtabcheck.TWINS:
                continue
            want = set()
            for x in a2s.get(t, ()):
                want |= sym2ship.get(x, set())
            rel = ship_img.dword(st + off + 1)
            got = (st + off + 5 + rel - (1 << 32 if rel & 0x80000000 else 0)) & 0xFFFFFFFF
            if want:
                told = True
                if got not in want:
                    return False
            else:
                # ours is placed nowhere, but what the shipped body calls is
                # placed: as another function of ours, it is not this call
                there = {s2a.get(n_) for r_, n_ in ship2sym.get(got, ()) if r_ == 0} - {None}
                if there:
                    told = True
                    if t not in there:
                        return False
        return told

    def masked(a):
        a, e = body(a)
        b = bytearray(e - a)
        for i in range(e - a):
            b[i] = (our_img.dword(a + i) or 0) & 0xFF
        for off, _v in refs(a):
            b[off:off + 4] = b"\0\0\0\0"
        for off, _t in calls(a):
            b[off + 1:off + 5] = b"\0\0\0\0"
        return bytes(b)

    def twin_fix(n, st):
        if n not in s2a or fits(n, st) is not False:
            return n, None
        mine = masked(s2a[n])
        for a, ns in a2s.items():
            if a == s2a[n] or not any(x in defined for x in ns):
                continue
            b = body(a)
            if b[1] - b[0] != len(mine) or masked(a) != mine:
                continue
            # (a ??_E is a weak external of the object that compiled it:
            # name its ??_G)
            x = sorted((x.startswith("??_E") and "??_G" + x[4:] in defined, x)
                       for x in ns if x in defined)[0][1]
            if x.startswith("??_E") and "??_G" + x[4:] in defined:
                x = "??_G" + x[4:]
            if fits(x, st):
                return x, "%s is placed here, but its data operands read back elsewhere; this twin's read back right" % n
        return n, "its data operands do not read back right here, and no twin's do"

    out = ["# %s: full-length tables for the stand-in classes (build/vtables/mkvtables.py;" % label,
           "# tools/mkvtabobj.py makes them an object linked ahead of ours). One slot",
           "# per line: the symbol of ours the scoreboard places at the shipped slot's target.",
           ""]
    missing = 0
    for sym, sva in sorted(maps):
        if sym in drop or sva not in rows:
            continue
        sl = [rows[sva][i] for i in sorted(rows[sva])]
        csl = compiled.get(sym, [])
        # (a table none of our objects compiles is a library's, whole) A
        # full-length compiled table goes in only for its holes: a compiled
        # `_purecall` where the shipped slot is concrete is a virtual the
        # class never overrode (CLTServer's ILTCSBase file slots: object.lto
        # calls GetFileInfo, slot 44, while loading), which can only crash.
        holes = [k for k, t in enumerate(sl) if k < len(csl) and csl[k] == "__purecall"
                 and not (pc and t == pc)]
        if not csl or (len(csl) >= len(sl) and not holes):
            continue
        out.append("vtable %s 0x%08x  # compiled with %d slots%s" % (
            sym, sva, len(csl), "; %d _purecall where the shipped slot is concrete" % len(holes) if holes else ""))
        for k, st in enumerate(sl):
            c = csl[k] if k < len(csl) else None
            if c and c.startswith("??_E"):
                c = "??_G" + c[4:]
            cand = placed_at(st)
            if pc and st == pc:
                out.append("    __purecall")
            elif c and c != "__purecall" and c in defined and (
                    c in cand or not cand or fits(c, st) or
                    # a real declaration (three slots or more) is the
                    # source's own, slot for slot: only its tail and its
                    # _purecall holes are taken from the shipped table (the
                    # mapping itself can be wrong: CObjWorldModel's and
                    # LAllocSimpleBlock's tables are both mapped to AvP2's
                    # 0x004c7e2c)
                    len(csl) >= 3 or
                    # a deleting destructor is the class's own: another
                    # class's same-shape one placed there is kept only
                    # when its calls read back right
                    (c.startswith("??_G") and cand[0].startswith("??_G") and
                     fits(cand[0], st) is not True and fits(c, st) is not False)):
                out.append("    %s%s" % (c, "" if c in cand else "  # nothing placed at 0x%08x: compiled slot kept" % st if not cand else
                                         "  # the compiled slot, kept over %s placed there" % cand[0]))
            elif cand:
                fixed, why = twin_fix(cand[0], st)
                if fixed != cand[0]:
                    out.append("    %s  # %s" % (fixed, why))
                    continue
                note = ["", "placed by shape", "near pairing only"][rank_of(st, cand[0])] if \
                    any(x in (cand[0], "??_E" + cand[0][4:]) for _r, x in ship2sym.get(st, ())) else "the same code as what is placed there"
                if len(cand) > 1:
                    note = (note + "; " if note else "") + "also: " + " ".join(cand[1:3])
                out.append("    %s%s" % (cand[0], "  # " + note if note else ""))
            elif c == "__purecall":
                # (a hole nothing of ours can fill yet stays as compiled)
                out.append("    __purecall  # the shipped slot is 0x%08x, which nothing of ours is placed at" % st)
            else:
                out.append("    MISSING 0x%08x" % st)
                missing += 1
    open(outp, "w").write("\n".join(out) + "\n")
    print("%s: %d tables, %d MISSING slots -> %s" % (label, sum(1 for l in out if l.startswith("vtable")), missing, outp))
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
