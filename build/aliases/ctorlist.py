# ctorlist.py <chk.tsv>: chosen ctor/dtor bindings across classes, with non-identical callers
import sys, re
def cls(n):
    m = re.match(r"\?\?[01](.+?)@@Q", n)
    return m.group(1) if m else None
for l in open(sys.argv[1]):
    u, t, tdef, v, fix, ua, ta, cands, how, refs = l.rstrip("\n").split("\t")
    tgt = t if v in ("OK", "OK?") else (fix if fix not in ("-", "?") else (t if v in ("NOEV", "NOT") and tdef == "Y" else None))
    if not tgt or not u.startswith(("??0", "??1")):
        continue
    if cls(u) == cls(tgt):
        continue
    bad = [r for r in refs.split() if not r.endswith(("[I]", "[S]"))]
    if bad:
        print(v, u, tgt, " ".join(bad)[:200], sep="\t")
