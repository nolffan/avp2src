# whorefs.py <objdir> <symbol-substring>: functions and data (by section owner) whose relocations name it
import sys, os, glob
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "../../ext/tools"))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from linkresolve import parse_obj
from aliascheck import data_refs
for p in sorted(glob.glob(os.path.join(sys.argv[1], "*.obj"))):
    d = open(p, "rb").read()
    try:
        f, _d, _u = parse_obj(d)
    except Exception:
        continue
    for n, (c, m, rels) in f.items():
        for off, k, s in rels:
            if sys.argv[2] in s:
                print(os.path.basename(p), n, s)
    for s, owners in data_refs(d).items():
        if sys.argv[2] in s:
            for o, off in owners:
                print(os.path.basename(p), "D", o, hex(off), s)
