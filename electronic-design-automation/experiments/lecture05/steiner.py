#!/usr/bin/env python3
"""Lecture 5: rectilinear MST vs exact minimum rectilinear Steiner tree (MRST).
MRST(P) = MST(P ∪ S) and (Hanan) S can be taken from the Hanan grid, so for small nets we
enumerate up to n-2 Hanan points and keep the best.  Also checks the 'MST <= 1.5 MRST' bound."""
import itertools
import random


def mst_len(pts):
    """Prim, O(n^2), Manhattan distance."""
    pts = list(pts)
    if len(pts) < 2:
        return 0
    in_tree = {0}
    d = [abs(p[0] - pts[0][0]) + abs(p[1] - pts[0][1]) for p in pts]
    total = 0
    while len(in_tree) < len(pts):
        j = min((k for k in range(len(pts)) if k not in in_tree), key=lambda k: d[k])
        total += d[j]
        in_tree.add(j)
        for k in range(len(pts)):
            if k not in in_tree:
                d[k] = min(d[k], abs(pts[k][0] - pts[j][0]) + abs(pts[k][1] - pts[j][1]))
    return total


def mrst(pins):
    xs = sorted({p[0] for p in pins})
    ys = sorted({p[1] for p in pins})
    hanan = [(x, y) for x in xs for y in ys if (x, y) not in pins]
    best, best_s = mst_len(pins), ()
    for k in range(1, len(pins) - 1):            # an MRST needs at most n-2 Steiner points
        for s in itertools.combinations(hanan, k):
            L = mst_len(list(pins) + list(s))
            if L < best:
                best, best_s = L, s
    return best, best_s


if __name__ == '__main__':
    # 5 pins read approximately off the slide figure (a) (8x8 grid)
    pins = [(4, 7), (6, 5), (2, 3), (8, 3), (5, 1)]
    m = mst_len(pins)
    r, s = mrst(pins)
    print(f'slide-like 5-pin net: rectilinear MST = {m}, MRST = {r}, Steiner points {s}, ratio {m / r:.3f}')
    # cross-shaped worst case for the ratio: 4 pins around a centre
    cross = [(0, 1), (1, 0), (2, 1), (1, 2)]
    m, (r, s) = mst_len(cross), mrst(cross)
    print(f'cross 4-pin net:       rectilinear MST = {m}, MRST = {r}, Steiner points {s}, ratio {m / r:.3f}')
    random.seed(1)
    worst = 0
    for trial in range(300):
        n = random.randint(3, 6)
        p = list({(random.randint(0, 20), random.randint(0, 20)) for _ in range(n)})
        if len(p) < 3:
            continue
        m, (r, _) = mst_len(p), mrst(p)
        worst = max(worst, m / r)
    print(f'300 random nets (3-6 pins): worst MST/MRST ratio = {worst:.3f}  (bound 1.5)')
