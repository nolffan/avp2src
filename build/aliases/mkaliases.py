#!/usr/bin/env python3
"""Write one image's alias file from aliascheck.py's verdicts.

  mkaliases.py <chk.tsv> <image label> <out.rsp> [--unproven]

Sections: retail-proven bindings (nolfsrc's, confirmed), bindings retargeted
to what retail calls, and (with --unproven) nolfsrc's bindings no placed
retail site can confirm or refute. Every other row is left out (dropped)
and listed on stderr.
"""
import sys

import os
args = [a for a in sys.argv[1:] if not a.startswith("--")]
unproven = "--unproven" in sys.argv
byhand = [a.split("=", 1)[1] for a in sys.argv[1:] if a.startswith("--byhand=")]
byhand = open(byhand[0]).read() if byhand and os.path.exists(byhand[0]) else ""
handset = {l.split(":", 1)[1].split("=", 1)[0] for l in byhand.splitlines()
           if l.startswith("/ALTERNATENAME:")}
# --skip=FILE: unproven bindings left out (a symbol per line, the reason
# after a tab), where taking one costs the scoreboard a placement
skip = {}
for a in sys.argv[1:]:
    if a.startswith("--skip=") and os.path.exists(a[7:]):
        for l in open(a[7:]):
            if l.strip() and not l.startswith("#"):
                k, _, why = l.rstrip("\n").partition("\t")
                skip[k] = why
chk, label, out = args
proven, fixed, weak, dropped, new = [], [], [], [], []
for line in open(chk):
    u, t, tdef, v, fix, ua, ta, cands, how, refs = line.rstrip("\n").split("\t")
    if u in handset:
        continue
    if v == "NOALIAS":
        if fix not in ("-", "?"):
            new.append((u, fix, ua, how))
        continue
    if v in ("OK", "OK?", "OK="):
        proven.append((u, t, ua, how))
    elif fix not in ("-", "?"):
        fixed.append((u, fix, ua, how, t))
    elif (v in ("NOEV", "NOT", "LOOK") or (v == "WRONG" and "L" in how)) and tdef == "Y":
        weak.append((u, t, v))
    else:
        dropped.append((u, t, v, ua, ta))

with open(out, "w") as f:
    f.write(f"""# {label}: link aliases, /ALTERNATENAME:<unresolved>=<defined>, made into
# weak externals by tools/mkaliasobj.py (VC6 LINK has no /ALTERNATENAME).
# Taken from nolfsrc's link/aliases.rsp (NOLF's) and checked against this
# image's retail bytes by reading each referencing site's call or reference
# out of the retail image where the scoreboard places the function
# (build/aliases/aliascheck.py). The comment above each: the retail address the
# sites reach, and how it was read (I identical, S small, N near of the
# same size, q a near's calls in order, D a table slot; j: retail's target
# is a jump thunk, bound to what it jumps to; b: the target has no
# relocation and its bytes are retail's there).
#
# Proven: the target is placed at the address the sites reach, its
# placement not a look-alike (bytes that recur in the retail image, with
# relocations unconfirmed), or it has no relocation and its bytes are
# retail's at that address (retail kept several copies of such bodies).
""")
    for u, t, ua, how in proven:
        f.write(f"# {ua} {how}\n/ALTERNATENAME:{u}={t}\n")
    f.write("""#
# Retargeted: nolfsrc's target is not what retail reaches here; the target
# is the function or datum of ours placed at that address (where retail
# folded identical bodies into one, any of the names placed there).
""")
    for u, t, ua, how, old in fixed:
        f.write(f"# {ua} {how}; nolfsrc: {old}\n/ALTERNATENAME:{u}={t}\n")
    if byhand:
        f.write("""#
# By hand: read off the retail code where the check above could not decide.
""" + byhand)
    f.write("""#
# New: no nolfsrc binding; the target is placed where the sites reach.
""")
    for u, t, ua, how in new:
        f.write(f"# {ua} {how}\n/ALTERNATENAME:{u}={t}\n")
    if unproven:
        f.write("""#
# Unproven: nolfsrc's binding (an address match in NOLF's image); no site
# the scoreboard places reaches the symbol here, or the target has no
# placement of its own, or only a look-alike one (its bytes recur in the
# retail image, its relocations unconfirmed), or what retail reaches is
# only such a look-alike, so retail neither confirms nor refutes it.
""")
        for u, t, v in weak:
            if u not in skip:
                f.write(f"/ALTERNATENAME:{u}={t}\n")
        if skip:
            f.write("#\n# Unproven, left out: taking them costs the scoreboard a placement\n")
            for u, why in sorted(skip.items()):
                f.write(f"#   {u}  {why}\n")
print(f"{label}: proven {len(proven)}, retargeted {len(fixed)}, new {len(new)}, unproven {len(weak)}"
      f"{'' if unproven else ' (left out)'}, dropped {len(dropped)}", file=sys.stderr)
for d in dropped:
    print("  DROP", *d, sep="\t", file=sys.stderr)
