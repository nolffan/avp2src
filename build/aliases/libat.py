# libat.py <lib> <retail image> <hexaddr> [name-substring]: library functions whose masked bytes match at the address
import sys, os, tempfile
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "ext", "tools"))
from libextract import extract
from linkresolve import parse_obj
from exedump import Pe
lib, img, a = sys.argv[1], sys.argv[2], int(sys.argv[3], 16)
sub = sys.argv[4] if len(sys.argv) > 4 else ""
pe = Pe(img); tva, tvs, trp = pe.secs[".text"]; text = pe.d[trp:trp + tvs]
d = tempfile.mkdtemp(); extract(lib, d)
for m in sorted(os.listdir(d)):
    for n, (code, mask, rels) in parse_obj(open(os.path.join(d, m), "rb").read())[0].items():
        if sub not in n: continue
        i = a - tva
        ok = all(text[i + k] == code[k] for k in range(len(code)) if k not in mask)
        if ok: print("MATCH", m, n, len(code))
