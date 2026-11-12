#!/usr/bin/env python3
"""Check the engine links' unresolved symbols against the alias files and retail.

  aliascheck.py <img> <score-stem> [alias.rsp...] [--out=FILE] [--also=RSP]

<img>: lt | alt | sv | asv (Primal Hunt's / AvP2's lithtech.exe, server.dll).
<score-stem>: out/plan/qs2.py output stem (its .placed and .near).
Alias files default to the image's link/aliases.rsp.

For every symbol the image's link leaves unresolved (<image>-link.txt) it
prints one TSV row:

  sym  alias-target  target-defined  verdict  fix  U-retail  T-retail  cands  refs

  U-retail   the retail addresses the referencing sites reach: every
             function of ours the scorer placed (identical I, small S, or a
             same-size near N whose call opcode agrees) has each relocation
             read back out of the retail bytes at the same offset
  T-retail   the alias target's retail address: its own placement, else
             what our placed functions' references to it read back (data)
  cands      our defined symbols placed at U-retail (the right target if
             the alias is wrong)
  verdict    OK (U-retail == {T-retail}), LOOK (so, but the target's
             placement is a look-alike: its bytes recur in retail and its
             relocations do not confirm it), OK= (the target has no relocation
             and its bytes are retail's at U-retail), WRONG (they differ), CONFLICT
             (U-retail has several addresses), NOEV (no placed site), NOT
             (target unknown in retail), UNDEF (target not defined in the
             image), NOALIAS
  how        the evidence: I S N placed sites, D a table slot, q a near's
             call sequence; j: retail's target is a jump thunk, and the
             candidate is what it jumps to; b: the candidate has no
             relocation and its bytes are retail's there; L: the
             candidate is a look-alike (as LOOK), so not taken
  fix        the candidate to bind to (one, or several our link folded
             into one address), '?' when they are several distinct ones
  refs       referencing functions with their placement kind (I/S/N/-),
             and data references (vtables) as D:<section symbol>
"""
import collections
import os
import re
import struct
import sys

W = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(W, "tools"))
from exedump import Pe  # noqa: E402
from linkresolve import parse_obj  # noqa: E402
from callseq import disasm_calls  # noqa: E402

IMG = {
    "lt": ("src/engine/lithtech", "Release", "lithtech", "lithtech.exe", "out/reference/lithtech.exe"),
    "alt": ("src/engine/lithtech", "AvP2Release", "lithtech", "lithtech.exe", "out/reference/avp2/lithtech.exe"),
    "sv": ("src/engine/server", "Release", "server", "server.dll", "out/reference/server.dll"),
    "asv": ("src/engine/server", "AvP2Release", "server", "server.dll", "out/reference/avp2/server.dll"),
}


def read_aliases(paths):
    al = {}
    for p in paths:
        for line in open(p):
            line = line.strip()
            if not line.upper().startswith("/ALTERNATENAME:"):
                continue
            a, t = line.split(":", 1)[1].split("=", 1)
            al.setdefault(a, t)
    return al


def data_refs(data):
    """-> {undefined-ish external: [section-symbol]} for non-code sections."""
    machine, nsec, _ts, symoff, nsyms, optsz, _ch = struct.unpack_from("<HHIIIHH", data, 0)
    strtab = data[symoff + nsyms * 18:]

    def symname(raw):
        if raw[:4] == b"\0\0\0\0":
            off = struct.unpack_from("<I", raw, 4)[0]
            return strtab[off:strtab.find(b"\0", off)].decode("latin-1")
        return raw.rstrip(b"\0").decode("latin-1")
    ents = [None] * nsyms
    i = 0
    while i < nsyms:
        raw = data[symoff + i * 18:symoff + i * 18 + 18]
        value, secnum, typ, storage, naux = struct.unpack_from("<IhHBB", raw, 8)
        ents[i] = (symname(raw[:8]), value, secnum, storage)
        i += 1 + naux
    out = collections.defaultdict(list)
    for si in range(1, nsec + 1):
        s = 20 + optsz + (si - 1) * 40
        rawsz, rawp, relp, _lp, nrel, _nl = struct.unpack_from("<IIIIHH", data, s + 16)
        flags = struct.unpack_from("<I", data, s + 36)[0]
        if flags & 0x20 or nrel == 0:
            continue
        owners = sorted((e[1], e[0]) for e in ents if e and e[2] == si and e[3] == 2)
        for r in range(nrel):
            va, idx, _t = struct.unpack_from("<IIH", data, relp + r * 10)
            e = ents[idx] if idx < nsyms else None
            if not e or e[3] != 2:
                continue
            own = [(n, va - v) for v, n in owners if v <= va]
            out[e[0]].append(own[-1] if own else ("?", 0))
    return out


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    opts = dict(a.split("=", 1) if "=" in a else (a, True) for a in sys.argv[1:] if a.startswith("--"))
    img, stem = args[0], args[1]
    ldir, odir, base, exe, retail = IMG[img]
    apaths = args[2:] or [os.path.join(W, ldir, "link/aliases.rsp")]
    aliases = read_aliases(apaths)

    unres = collections.defaultdict(set)
    for line in open(os.path.join(W, ldir, odir, base + "-link.txt"), errors="replace"):
        m = re.match(r"(\S+) : error LNK20(?:01|19): unresolved external symbol (.*)$", line.strip())
        if not m:
            continue
        rest = m.group(2)
        mm = re.search(r"\((\S+)\)(?: referenced in function .*)?$", rest)
        sym = mm.group(1) if mm and rest.startswith('"') else rest.split()[0]
        unres[sym].add(m.group(1))

    # --also=<rsp>: the names an alias file already binds count as
    # unresolved too (the link that left them so is the one before it)
    if opts.get("--also"):
        for a in read_aliases([opts["--also"]]):
            unres[a].add("?")
    # and every symbol an earlier run saw unresolved (build/aliases/universe.<img>.txt),
    # kept while an object still references it
    upath = os.path.join(W, "build/aliases", f"universe.{img}.txt")
    if os.path.exists(upath):
        for a in open(upath).read().split():
            unres[a].add("?")

    defined = {}
    inpub = False
    for line in open(os.path.join(W, ldir, odir, base + ".map"), errors="replace"):
        if "Publics by Value" in line:
            inpub = True
            continue
        if line.startswith(" Static symbols"):
            break
        f = line.split()
        if inpub and len(f) >= 4 and re.match(r"[0-9a-f]{4}:", f[0]):
            defined[f[1]] = (f[2], f[-1])

    for u in unres:
        defined.pop(u, None)          # an alias already bound shows in the map

    objs, libs = [], {}
    for line in open(os.path.join(W, ldir, odir, exe + ".rsp")):
        line = line.strip()
        if line.lower().endswith(".obj"):
            objs.append(os.path.join(W, ldir, line.replace("\\", "/")))
        elif line.lower().endswith(".lib"):
            lp = os.path.join(W, ldir, line.replace("\\", "/"))
            if os.path.exists(lp):
                libs[os.path.basename(lp)[:-4].lower()] = lp

    placed = {}                                    # (objbase, sym) -> (addr, kind)
    for line in open(os.path.join(W, stem + ".placed")):
        a, k, s = line.rstrip("\n").split("\t")
        o, _, n = s.partition(":")
        placed[(o, n)] = (int(a, 16), k, 0)
    for line in open(os.path.join(W, stem + ".near")):
        a, ss, osz, s = line.rstrip("\n").split("\t")
        o, _, n = s.partition(":")
        if (o, n) not in placed:
            placed[(o, n)] = (int(a, 16), "N" if ss == osz else "n", int(osz))
    sym_addr = collections.defaultdict(set)
    for (o, n), (a, k, _z) in placed.items():
        if k in "ISN":
            sym_addr[n].add(a)

    pe = Pe(os.path.join(W, retail))
    tva, tvs, trp = pe.secs[".text"]
    text = pe.d[trp:trp + tvs]

    refs = collections.defaultdict(set)            # unresolved sym -> referencing fns
    drefs, nears = [], []
    how = collections.defaultdict(set)             # sym -> evidence kinds
    allf = {}
    undef_all = set()
    for path in objs:
        ob = os.path.basename(path)
        data = open(path, "rb").read()
        funcs, _d, _u = parse_obj(data)
        undef_all |= _u
        for name, (code, _mask, rels) in funcs.items():
            allf[(ob, name)] = (code, rels, _mask)
            pk = placed.get((ob, name))
            k = pk[1] if pk else "-"
            for off, kind, sym in rels:
                if sym in unres:
                    refs[sym].add(f"{ob}:{name}[{k}]")
            if pk and pk[1] == "n" and pk[2]:
                nears.append((pk[0], pk[2], code, rels))
        for sym, owners in data_refs(data).items():
            for own, off in owners:
                drefs.append((sym, own, off))
                if sym in unres:
                    refs[sym].add(f"{ob}:D:{own}")

    def readings(key):
        """(sym, retail value) for each relocation of a placed function."""
        a, k, _z = placed[key]
        code, rels, _m = allf[key]
        out = []
        for off, kind, sym in rels:
            i = a - tva + off
            if i < 2 or i + 4 > len(text):
                continue
            if kind == "rel" and text[i - 1] != code[off - 1]:
                continue
            if k == "N" and (text[i - 1] != code[off - 1] or text[i - 2] != code[off - 2]):
                continue
            v = struct.unpack_from("<i" if kind == "rel" else "<I", text, i)[0]
            add = struct.unpack_from("<i" if kind == "rel" else "<I", code, off)[0]
            out.append((sym, (a + off + 4 + v - add) if kind == "rel" else (v - add) & 0xffffffff))
        return out

    live = [key for key in placed if key in allf and placed[key][1] in "ISN"]
    seen = collections.defaultdict(set)            # sym -> retail addresses
    for key in live:
        for sym, v in readings(key):
            seen[sym].add(v)
    # a placement whose own calls and references mostly disagree with what
    # their targets' placements say is a look-alike, not evidence; nor is a
    # small one (under 32 bytes) that no relocation of its own confirms
    libplaced, libf = set(), {}
    verified = set()                               # every relocation agrees
    discredited = set()                            # not evidence of a call site
    nontarget = set()                              # nor of where a target is
    for key in live:
        agree = disagree = unknown = 0
        for sym, v in readings(key):
            if sym in unres:
                unknown += 1
                continue
            x = sym_addr.get(sym)
            x = next(iter(x)) if x and len(x) == 1 else (
                next(iter(seen[sym])) if len(seen.get(sym, ())) == 1 else None)
            if x is None:
                unknown += 1
                continue
            if x == v:
                agree += 1
            else:
                disagree += 1
        if agree and not disagree and not unknown:
            verified.add(key[1])
        small = len(allf[key][0]) < 32
        if (disagree and (small or disagree > agree)) or (small and not agree and unknown):
            discredited.add(key)
            nontarget.add(key)
        elif small and not agree:
            discredited.add(key)
    # a caller whose bytes the retail image has more than once was placed by
    # the scorer's order heuristics: its call sites are not evidence (a
    # template's instantiations, which differ only in what they call)
    from exematch import find
    ambiguous = set()
    for key in live:
        if key in discredited or not any(sy in unres for _o, _k, sy in allf[key][1]):
            continue
        code, rels, mask = allf[key]
        if len(list(find(text, tva, code, mask))) > 1:
            ambiguous.add(key)
            discredited.add(key)
    sym_addr = collections.defaultdict(set)
    for key in live:
        if key not in nontarget:
            sym_addr[key[1]].add(placed[key][0])
    # nears of another size: the scorer's pairing names their address
    for key, (a, k, _z) in placed.items():
        if k == "n":
            sym_addr[key[1]].add(a)
    # targets in the libraries (lith, StdLith, RezMgr, WON): their bytes,
    # found once in the retail image
    # the libraries (lith, StdLith, RezMgr, WON): every member that defines
    # a symbol of the image or an alias target; their functions placed where
    # their bytes occur once in the retail image. A target only a library
    # defines counts as defined: the alias object references it, which
    # draws its member in
    import tempfile
    from libextract import extract
    from exematch import find
    tmp = tempfile.mkdtemp()
    targets = set(aliases.values())
    multi = []
    for lib in ("lith", "stdlith", "rezmgr", "wonapi"):
        if lib not in libs:
            continue
        d = os.path.join(tmp, lib)
        extract(libs[lib], d)
        for mem in sorted(os.listdir(d)):
            funcs, ldef, _u = parse_obj(open(os.path.join(d, mem), "rb").read())
            if not (ldef & targets or any(x in defined for x in ldef)):
                continue
            for x in ldef:
                if x not in defined and x in targets and x not in unres:
                    defined[x] = ("-", f"{lib}:{mem}")
            for name, (code, mask, _r) in funcs.items():
                if name in sym_addr or name in libf:
                    continue
                libf[name] = (code, mask)
                if len(code) - len(mask) >= 8:
                    hits = list(find(text, tva, code, mask))
                    if len(hits) == 1:
                        sym_addr[name].add(hits[0])
                        libplaced.add(name)
                    elif 1 < len(hits) <= 16:
                        multi.append((name, code, _r, hits))
    # a library function whose bytes occur several times: the one copy its
    # calls agree with (two passes, as the callees are placed in turn)
    for _pass in range(2):
        for name, code, rels, hits in multi:
            if name in sym_addr:
                continue
            good = []
            for h in hits:
                agree = disagree = 0
                for off, kind, sym in rels:
                    if kind != "rel" or sym not in sym_addr or len(sym_addr[sym]) != 1:
                        continue
                    v = struct.unpack_from("<i", text, h - tva + off)[0]
                    add = struct.unpack_from("<i", code, off)[0]
                    if h + off + 4 + v - add == next(iter(sym_addr[sym])):
                        agree += 1
                    else:
                        disagree += 1
                if agree and not disagree:
                    good.append(h)
            if len(good) == 1:
                sym_addr[name].add(good[0])
                libplaced.add(name)
    seen = collections.defaultdict(set)
    for key in live:
        if key in discredited:
            continue
        for sym, v in readings(key):
            seen[sym].add(v)
            how[sym].add(placed[key][1])
    print(f"{img}: {len(discredited)} placements discredited, {len(ambiguous)} of them callers"
          " placed among several copies", file=sys.stderr)

    def taddr(t):
        if t in sym_addr and len(sym_addr[t]) == 1:
            return next(iter(sym_addr[t]))
        if len(seen.get(t, ())) == 1:
            return next(iter(seen[t]))
        return None

    # vtables and other tables: the slot at the same offset of the table's
    # retail copy (the table's address read back from a placed reference)
    for sym, own, off in drefs:
        if sym not in unres:
            continue
        oa = taddr(own)
        fo = pe.va2off(oa + off) if oa else None
        if fo is None:
            continue
        seen[sym].add(struct.unpack_from("<I", pe.d, fo)[0])
        how[sym].add("D")
    # nears of another size (and unplaced callers the scorer paired): the
    # calls in order. Calls whose targets are known on both sides (and the
    # indirect ones) are aligned as a longest common subsequence; between two
    # aligned calls, a run of our calls to unresolved symbols is paired with
    # the retail run of the same length, call for call
    for a, osz, code, rels in nears:
        rc = [t for _o, t in disasm_calls(text[a - tva:a - tva + osz], a)]
        oc = disasm_calls(code, 0)
        relat = {off: sym for off, kind, sym in rels if kind == "rel"}
        ours = [relat.get(o + 1, "?") if t is not None else None for o, t in oc]
        if not any(x in unres for x in ours) or len(ours) * len(rc) > 400000:
            continue
        known = [None if x is None else (taddr(x) if x not in unres and x != "?" else -1)
                 for x in ours]

        def eq(i, j):
            return (known[i] is None and rc[j] is None) or \
                (known[i] is not None and known[i] != -1 and known[i] == rc[j])
        n, m = len(ours), len(rc)
        d = [[0] * (m + 1) for _ in range(n + 1)]
        for i in range(n - 1, -1, -1):
            for j in range(m - 1, -1, -1):
                d[i][j] = d[i + 1][j + 1] + 1 if eq(i, j) else max(d[i + 1][j], d[i][j + 1])
        pairs, i, j = [(-1, -1)], 0, 0
        while i < n and j < m:
            if eq(i, j):
                pairs.append((i, j)); i += 1; j += 1
            elif d[i + 1][j] >= d[i][j + 1]:
                i += 1
            else:
                j += 1
        if len(pairs) < 3:
            continue
        pairs.append((n, m))
        for (i0, j0), (i1, j1) in zip(pairs, pairs[1:]):
            gi, gj = list(range(i0 + 1, i1)), list(range(j0 + 1, j1))
            if not gi or len(gi) != len(gj):
                continue
            for x, y in zip(gi, gj):
                if ours[x] in unres and rc[y] is not None:
                    seen[ours[x]].add(rc[y])
                    how[ours[x]].add("q")

    by_addr = collections.defaultdict(set)
    ikinds = collections.defaultdict(set)
    for key in live:
        if key not in nontarget:
            ikinds[key[1]].add(placed[key][1])
    for n, aa in sym_addr.items():
        if len(aa) == 1 and n in defined:
            by_addr[next(iter(aa))].add(n)
        elif len(aa) > 1 and n in defined and ikinds.get(n) == {"I"}:
            # one COMDAT of ours (a template's member), several identical
            # copies in retail (instantiations ours spells as one): each is it
            for a in aa:
                by_addr[a].add(n)
    for n, aa in seen.items():
        if n in defined and n not in sym_addr and len(aa) == 1:
            by_addr[next(iter(aa))].add(n)

    purecode = {n: code for (o, n), (code, rels, m) in allf.items() if not m and code}
    purecode.update({n: code for n, (code, m) in libf.items() if not m and code})

    def same_bytes(code, a):
        i = a - tva
        return 0 <= i and text[i:i + len(code)] == code and \
            (i + len(code) >= len(text) or text[i + len(code)] in (0x90, 0xCC))

    bykey = collections.defaultdict(list)
    for key in live:
        bykey[key[1]].append(key)
    lookcache = {}

    def lookalike(n):
        if n in lookcache:
            return lookcache[n]
        r = False
        if n not in verified and n not in purecode:
            for key in bykey.get(n, ()):
                code, rels, mask = allf[key]
                if len(list(find(text, tva, code, mask))) > 1:
                    r = True
                    break
        lookcache[n] = r
        return r

    scorer_i = collections.defaultdict(set)        # what the scorer itself placed identical
    for (o, n), (a, k, _z) in placed.items():
        if k == "I":
            scorer_i[a].add(n)
    # candidates with no relocation at all: placed, their bytes are retail's
    pure = {n for (o, n), (code, rels, m) in allf.items() if not m and n in sym_addr}
    pure |= {n for n, (code, m) in libf.items() if not m and n in sym_addr}
    if os.environ.get("K1DBG"):
        x = os.environ["K1DBG"]
        print("DBG", x, x in defined, sym_addr.get(x), [k for k in placed if k[1] == x], [k for k in live if k[1] == x], x in nontarget, file=sys.stderr)
    out = open(opts["--out"], "w") if opts.get("--out") else sys.stdout
    cnt = collections.Counter()
    for u in list(unres):
        if unres[u] == {"?"} and u not in undef_all:
            del unres[u]                  # no object references it any more
    for u in sorted(unres):
        t = aliases.get(u)
        ua = sorted(seen.get(u, ()))
        cands = sorted(by_addr.get(ua[0], ())) if len(ua) == 1 else []
        # a retail jump thunk (a member that tail-calls another, or a
        # table slot to one): the function it jumps to
        thunk = False
        if len(ua) == 1 and not cands and tva <= ua[0] < tva + len(text) - 5 \
                and text[ua[0] - tva] == 0xE9:
            j = ua[0] + 5 + struct.unpack_from("<i", text, ua[0] - tva + 1)[0]
            if by_addr.get(j):
                cands, thunk = sorted(by_addr[j]), True
        tdef = "-"
        ta = None
        if not t:
            v = "NOALIAS"
        else:
            tdef = "Y" if t in defined else "N"
            ta = taddr(t)
            if tdef == "N":
                v = "UNDEF"
            elif len(ua) > 1:
                v = "CONFLICT"
            elif not ua:
                v = "NOEV"
            elif ta is None:
                v = "OK?" if t in cands else ("WRONG" if cands else "NOT")
            else:
                v = "OK" if ta == ua[0] else "WRONG"
        # a target placed by its bytes where the retail image has those bytes
        # several times over (a template's instantiations: one array's
        # constructor looks like any other's) and that its own relocations do
        # not confirm is a look-alike, not the function the sites reach
        if v in ("OK", "OK?") and lookalike(t):
            v = "LOOK"
        # a target with no relocation whose bytes are retail's at the address
        # the sites reach is that function, whatever copy retail kept
        if v in ("WRONG", "NOT") and len(ua) == 1 and t in purecode and \
                same_bytes(purecode[t], ua[0]):
            v = "OK="
        fix = "-"
        if v not in ("OK", "OK?", "OK=") and not cands and len(ua) == 1:
            for c in sorted(purecode):
                if c in defined and c not in unres and same_bytes(purecode[c], ua[0]):
                    cands = [c]
                    how[u].add("b")
                    break
        if v not in ("OK", "OK?", "OK=") and cands:
            if t in cands:
                fix = t
            elif len({defined[c][0] for c in cands}) == 1:
                fix = cands[0]
            elif [c for c in cands if c in pure]:
                fix = [c for c in cands if c in pure][0]
            elif [c for c in cands if c in verified]:
                fix = [c for c in cands if c in verified][0]
            elif len([c for c in cands if c in scorer_i.get(ua[0], ())]) == 1:
                fix = [c for c in cands if c in scorer_i.get(ua[0], ())][0]
            else:
                fix = "?"
        if fix not in ("-", "?") and lookalike(fix):
            fix = "-"
            how[u].add("L")
        if fix == "?":
            b = [c for c in cands if c in purecode and same_bytes(purecode[c], ua[0])]
            if b:
                fix = b[0]
                how[u].add("b")
        if thunk and fix not in ("-", "?"):
            how[u].add("j")
        cnt[v] += 1
        out.write("\t".join([u, t or "-", tdef, v, fix, ",".join(f"{a:#x}" for a in ua),
                             f"{ta:#x}" if ta else "-", " ".join(cands) or "-",
                             "".join(sorted(how.get(u, ()))) or "-",
                             " ".join(sorted(refs.get(u, ()))) or "-"]) + "\n")
    print(img, len(unres), dict(cnt), file=sys.stderr)
    open(upath, "w").write("".join(u + "\n" for u in sorted(unres)))


if __name__ == "__main__":
    main()
