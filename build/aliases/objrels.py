# objrels.py <obj> <symbol>: the relocations of one function of an object
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "../../tools"))
from linkresolve import parse_obj
f, _d, _u = parse_obj(open(sys.argv[1], "rb").read())
for n, (code, mask, rels) in f.items():
    if n == sys.argv[2]:
        print(len(code), code.hex())
        for r in rels:
            print(*r)
