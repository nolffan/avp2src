#!/usr/bin/env python3
"""netpos.py <netpos.log>: how smoothly another player moved, as this client saw it.

For the object that moved the most: over the frames while it was running
(a horizontal speed above 50 units/s over the surrounding half second),
how many frames it did not move at all (stalls), and how many frame steps
were more than twice its median step (jumps), per second.
"""
import sys
from collections import defaultdict

rows = defaultdict(list)
for line in open(sys.argv[1]):
    p = line.split()
    if len(p) != 5:
        continue
    rows[p[1]].append((float(p[0]), float(p[2]), float(p[3]), float(p[4])))


def path(v):
    return sum(((b[1] - a[1]) ** 2 + (b[3] - a[3]) ** 2) ** 0.5 for a, b in zip(v, v[1:]))


obj = max(rows, key=lambda k: path(rows[k]))
v = rows[obj]
steps = []
for i in range(1, len(v)):
    d = ((v[i][1] - v[i - 1][1]) ** 2 + (v[i][3] - v[i - 1][3]) ** 2) ** 0.5
    # speed over the surrounding half second
    j0 = i
    while j0 > 0 and v[i][0] - v[j0][0] < 0.25:
        j0 -= 1
    j1 = i
    while j1 < len(v) - 1 and v[j1][0] - v[i][0] < 0.25:
        j1 += 1
    span = v[j1][0] - v[j0][0]
    sp = (((v[j1][1] - v[j0][1]) ** 2 + (v[j1][3] - v[j0][3]) ** 2) ** 0.5 / span) if span > 0 else 0
    if sp > 50:
        steps.append((v[i][0], d, v[i][0] - v[i - 1][0]))
if not steps:
    print("no running frames")
    sys.exit()
secs = sum(s[2] for s in steps)
ds = sorted(s[1] for s in steps if s[1] > 0)
med = ds[len(ds) // 2] if ds else 0
stalls = sum(1 for s in steps if s[1] == 0)
jumps = sum(1 for s in steps if med and s[1] > 2 * med)
print("object %s: %d running frames over %.1f s (%.0f fps), median step %.2f units" % (obj, len(steps), secs, len(steps) / secs, med))
print("stalled frames: %d (%.1f%%)" % (stalls, 100.0 * stalls / len(steps)))
print("jumps (> 2x median): %d, %.2f a second" % (jumps, jumps / secs))
