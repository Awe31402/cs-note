#!/usr/bin/env python3
"""Lecture 12: a toy 2D global router on a G-cell grid.
Each boundary (edge between adjacent G-cells) has a capacity.  Two-pin nets are routed with A*
(cost = 1 + congestion penalty + history); then rip-up-and-reroute (negotiated congestion, like
NTHU-Route's history cost) until no boundary overflows.  Also: iterated 1-Steiner on 4 points."""
import heapq
import itertools
import random

W, H, CAP = 8, 8, 2


def edges_of(path):
    return [tuple(sorted((a, b))) for a, b in zip(path, path[1:])]


def astar(s, t, usage, hist, pres):
    def cost(e):
        u = usage.get(e, 0)
        over = max(0, u + 1 - CAP)
        return (1 + hist.get(e, 0)) * (1 + pres * over)

    h = lambda p: abs(p[0] - t[0]) + abs(p[1] - t[1])          # Manhattan heuristic
    pq, g, prev = [(h(s), 0, s)], {s: 0}, {}
    while pq:
        f, d, u = heapq.heappop(pq)
        if u == t:
            break
        if d > g[u]:
            continue
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            v = (u[0] + dx, u[1] + dy)
            if not (0 <= v[0] < W and 0 <= v[1] < H):
                continue
            nd = d + cost(tuple(sorted((u, v))))
            if nd < g.get(v, 1e18):
                g[v], prev[v] = nd, u
                heapq.heappush(pq, (nd + h(v), nd, v))
    path = [t]
    while path[-1] != s:
        path.append(prev[path[-1]])
    return path[::-1]


def overflow(usage):
    return sum(max(0, u - CAP) for u in usage.values())


def route(nets, iters=30):
    usage, hist, paths = {}, {}, {}
    for i, (s, t) in enumerate(nets):                  # initial: shortest paths, ignore congestion
        p = astar(s, t, {}, {}, 0)
        paths[i] = p
        for e in edges_of(p):
            usage[e] = usage.get(e, 0) + 1
    log = [(0, overflow(usage), sum(len(p) - 1 for p in paths.values()))]
    for it in range(1, iters + 1):
        if overflow(usage) == 0:
            break
        for e, u in usage.items():                      # history: remember congested boundaries
            if u > CAP:
                hist[e] = hist.get(e, 0) + 1
        bad = [i for i, p in paths.items() if any(usage[e] > CAP for e in edges_of(p))]
        for i in bad:                                   # rip up and reroute
            for e in edges_of(paths[i]):
                usage[e] -= 1
            p = astar(*nets[i], usage, hist, pres=it)
            paths[i] = p
            for e in edges_of(p):
                usage[e] = usage.get(e, 0) + 1
        log.append((it, overflow(usage), sum(len(p) - 1 for p in paths.values())))
    return log, usage


def mst(pts):
    pts = list(pts)
    seen, total = {0}, 0
    d = [abs(p[0] - pts[0][0]) + abs(p[1] - pts[0][1]) for p in pts]
    while len(seen) < len(pts):
        j = min((k for k in range(len(pts)) if k not in seen), key=lambda k: d[k])
        total += d[j]
        seen.add(j)
        for k in range(len(pts)):
            d[k] = min(d[k], abs(pts[k][0] - pts[j][0]) + abs(pts[k][1] - pts[j][1]))
    return total


def iterated_1_steiner(P):
    S = []
    while True:
        cur = mst(P + S)
        hanan = {(x, y) for x, _ in P + S for _, y in P + S} - set(P + S)
        gain, x = max(((cur - mst(P + S + [h]), h) for h in hanan), default=(0, None))
        if gain <= 0:
            return cur, S
        S.append(x)


if __name__ == '__main__':
    rnd = random.Random(3)
    # a hot spot: many nets crossing the middle of the chip
    nets = []
    for _ in range(14):
        s = (rnd.randint(0, 2), rnd.randint(2, 5))
        t = (rnd.randint(5, 7), rnd.randint(2, 5))
        nets.append((s, t))
    lower = sum(abs(s[0] - t[0]) + abs(s[1] - t[1]) for s, t in nets)
    log, usage = route(nets)
    print(f'{W}x{H} G-cells, boundary capacity {CAP}, {len(nets)} two-pin nets, '
          f'sum of Manhattan lengths {lower}')
    for it, ov, wl in log:
        print(f'  iteration {it:2d}: total overflow {ov:3d}, wirelength {wl}')
    P = [(0, 2), (2, 0), (4, 2), (2, 4)]
    cost, S = iterated_1_steiner(P)
    print(f'iterated 1-Steiner on {P}: MST {mst(P)} -> {cost}, Steiner points {S}')
