#!/usr/bin/env python3
"""Lecture 3 checks: TSP exhaustive vs branch-and-bound, transportation DP/UCS, FFD bin packing,
HC -> TSP reduction.  Data are the examples on the slides (Gerez Fig. 5.3, CS221, slide 77)."""
import heapq
import itertools

# ---- 1. TSP on Gerez Fig. 5.3 (A..F, missing edges = not allowed) ----
E = {('A', 'B'): 9, ('A', 'E'): 3, ('A', 'F'): 5, ('B', 'F'): 4, ('B', 'C'): 5,
     ('C', 'F'): 8, ('E', 'F'): 5, ('D', 'F'): 7, ('D', 'E'): 1, ('C', 'D'): 2}
W = {}
for (u, v), w in E.items():
    W[u, v] = W[v, u] = w
CITIES = 'ABCDEF'
ADJ = {c: sorted(v for (u, v) in W if u == c) for c in CITIES}  # alphabetical, like the figure


def tsp(prune):
    """Backtracking from A; prune=True kills a partial path whose cost >= best so far."""
    best = [float('inf'), None]
    stats = {'nodes': 0, 'tours': []}

    def go(path, cost):
        stats['nodes'] += 1
        if prune and cost >= best[0]:
            return
        if len(path) == len(CITIES):
            if ('A', path[-1]) in W:
                total = cost + W[path[-1], 'A']
                stats['tours'].append((''.join(path) + 'A', total))
                if total < best[0]:
                    best[:] = [total, ''.join(path) + 'A']
            return
        for nxt in ADJ[path[-1]]:
            if nxt not in path:
                go(path + [nxt], cost + W[path[-1], nxt])

    go(['A'], 0)
    return best, stats


# ---- 2. Transportation (walk s->s+1 = 1 min, tram s->2s = 2 min) ----
def transport_dp(n):
    """futureCost(s) with memo (CS221 slide 70)."""
    cache = {}

    def future(s):
        if s == n:
            return 0
        if s in cache:
            return cache[s]
        moves = [(1, s + 1)] + ([(2, 2 * s)] if 2 * s <= n else [])
        cache[s] = min(c + future(t) for c, t in moves)
        return cache[s]

    return future(1)


def transport_ucs(n):
    """Uniform cost search (Dijkstra), returns (cost, path)."""
    frontier = [(0, 1, (1,))]
    explored = set()
    while frontier:
        p, s, path = heapq.heappop(frontier)
        if s == n:
            return p, path
        if s in explored:
            continue
        explored.add(s)
        for c, t in [(1, s + 1), (2, 2 * s)]:
            if t <= n and t not in explored:
                heapq.heappush(frontier, (p + c, t, path + (t,)))


# ---- 3. First-Fit Decreasing ----
def ffd(sizes, cap):
    bins = []
    for s in sorted(sizes, reverse=True):
        for b in bins:
            if sum(b) + s <= cap:
                b.append(s)
                break
        else:
            bins.append([s])
    return bins


def bin_opt(sizes, cap):
    """Exact minimum #bins by brute force over assignments (tiny instance only)."""
    n = len(sizes)
    for k in range(1, n + 1):
        for assign in itertools.product(range(k), repeat=n):
            loads = [0] * k
            for s, b in zip(sizes, assign):
                loads[b] += s
            if max(loads) <= cap:
                return k


# ---- 4. HC -> TSP reduction on the slide-37 graph ----
def hc_to_tsp(n, edges, big=2):
    d = {(u, v): (1 if (u, v) in edges or (v, u) in edges else big)
         for u in range(1, n + 1) for v in range(1, n + 1) if u != v}
    best = min((sum(d[t[i], t[(i + 1) % n]] for i in range(n)), t)
               for t in ((1,) + p for p in itertools.permutations(range(2, n + 1))))
    return best


if __name__ == '__main__':
    for prune in (False, True):
        (cost, tour), st = tsp(prune)
        name = 'branch-and-bound' if prune else 'exhaustive     '
        print(f'TSP {name}: best {cost} via {tour}; search nodes {st["nodes"]}; '
              f'complete tours {len(st["tours"])} costs {sorted(c for _, c in st["tours"])}')
    for n in (10, 20, 100, 400):
        c, path = transport_ucs(n)
        try:
            dp = transport_dp(n)
        except RecursionError:
            dp = 'RecursionError (default limit 1000)'
        print(f'transport n={n}: DP {dp}  UCS {c}  path {path}')
    sizes, cap = (1, 4, 2, 1, 2, 3, 5), 6
    b = ffd(sizes, cap)
    print(f'FFD {sizes} cap {cap}: {len(b)} bins {b}; exact optimum {bin_opt(sizes, cap)}; '
          f'lower bound ceil(sum/cap) = {-(-sum(sizes) // cap)}')
    edges = {(1, 5), (5, 2), (2, 3), (3, 4), (4, 1), (1, 3)}  # HC <1,5,2,3,4,1> plus chord 1-3
    print('HC->TSP (slide graph):', hc_to_tsp(5, edges), '  bound B = n = 5')
    print('HC->TSP (5-cycle with one edge removed):', hc_to_tsp(5, {(1, 5), (5, 2), (2, 3), (3, 4)}))
