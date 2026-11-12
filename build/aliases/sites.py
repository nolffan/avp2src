# sites.py <img> <score-stem> <symbol>: each placed site that references the symbol, and the retail target there
import sys, os, struct
W = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
sys.path.insert(0, W + "/tools"); sys.path.insert(0, W + "/build/aliases")
from exedump import Pe
from linkresolve import parse_obj
from aliascheck import IMG
img, stem, want = sys.argv[1:4]
ldir, odir, base, exe, retail = IMG[img]
placed = {}
for l in open(f"{W}/{stem}.placed"):
    a, k, s = l.rstrip("\n").split("\t"); o, _, n = s.partition(":")
    placed[(o, n)] = (int(a, 16), k)
pe = Pe(f"{W}/{retail}"); tva, tvs, trp = pe.secs[".text"]; text = pe.d[trp:trp+tvs]
for l in open(f"{W}/{ldir}/{odir}/{exe}.rsp"):
    l = l.strip()
    if not l.lower().endswith(".obj"): continue
    p = f"{W}/{ldir}/" + l.replace("\\", "/"); ob = os.path.basename(p)
    for n, (code, m, rels) in parse_obj(open(p, "rb").read())[0].items():
        for off, kind, sym in rels:
            if sym != want: continue
            if (ob, n) not in placed:
                print("unplaced", ob, n, hex(off)); continue
            A, k = placed[(ob, n)]
            v = struct.unpack_from("<i", text, A - tva + off)[0]
            add = struct.unpack_from("<i", code, off)[0]
            r = A + off + 4 + v - add if kind == "rel" else v - add
            print(f"{r & 0xffffffff:#x}", k, f"{A + off - 1:#x}", ob, n)
