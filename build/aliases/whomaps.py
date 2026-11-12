# whomaps.py <img> <score-stem> <our-hex-addr>: placed functions whose relocation reaches that
# address of OUR image, and what the retail operand at the same place is
import sys, os, re, struct, collections
W = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
sys.path.insert(0, W + "/tools"); sys.path.insert(0, W + "/build/aliases")
from exedump import Pe
from linkresolve import parse_obj
from aliascheck import IMG, read_aliases
img, stem, want = sys.argv[1], sys.argv[2], int(sys.argv[3], 16)
ldir, odir, base, exe, retail = IMG[img]
addr = {}
inpub = False
for l in open(f"{W}/{ldir}/{odir}/{base}.map", errors="replace"):
    if "Publics by Value" in l: inpub = True; continue
    f = l.split()
    if inpub and len(f) >= 4 and re.match(r"[0-9a-f]{4}:", f[0]):
        addr[f[1]] = int(f[2], 16)
for a, t in read_aliases([f"{W}/{ldir}/link/aliases-{'ph' if img in ('lt','sv') else 'avp2'}.rsp"]).items():
    if t in addr: addr[a] = addr[t]
placed = {}
for l in open(f"{W}/{stem}.placed"):
    a, k, s = l.rstrip("\n").split("\t"); o, _, n = s.partition(":")
    if k == "I": placed[(o, n)] = int(a, 16)
pe = Pe(f"{W}/{retail}"); tva, tvs, trp = pe.secs[".text"]; text = pe.d[trp:trp+tvs]
for l in open(f"{W}/{ldir}/{odir}/{exe}.rsp"):
    l = l.strip()
    if not l.lower().endswith(".obj"): continue
    p = f"{W}/{ldir}/" + l.replace("\\", "/"); ob = os.path.basename(p)
    funcs, _d, _u = parse_obj(open(p, "rb").read())
    for n, (code, m, rels) in funcs.items():
        if (ob, n) not in placed: continue
        A = placed[(ob, n)]
        for off, kind, sym in rels:
            if addr.get(sym) != want: continue
            v = struct.unpack_from("<i", text, A - tva + off)[0]
            add = struct.unpack_from("<i", code, off)[0]
            r = A + off + 4 + v - add if kind == "rel" else v - add
            print(f"{r & 0xffffffff:#x}", ob, n, sym)
