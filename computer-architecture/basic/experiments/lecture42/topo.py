"""課本 Table 12.7：用 BFS 算 diameter、用暴力法算 bisection bandwidth（N = 16，把節點平分成兩半的所有方法裡，要剪斷的最少連線數）。
只算沒有 switch 的拓樸（chain、ring、mesh、torus、hypercube）；butterfly 只算 diameter。"""
from itertools import combinations
from collections import deque
from math import isqrt, log2

def chain(n): return [(i, i + 1) for i in range(n - 1)]
def ring(n): return chain(n) + [(n - 1, 0)]
def mesh(n, wrap=False):
    k = isqrt(n); e = []
    for r in range(k):
        for c in range(k):
            v = r * k + c
            if c + 1 < k: e.append((v, v + 1))
            elif wrap: e.append((v, r * k))
            if r + 1 < k: e.append((v, v + k))
            elif wrap: e.append((v, c))
    return e
def hypercube(n): return [(v, v ^ (1 << b)) for v in range(n) for b in range(int(log2(n))) if v < v ^ (1 << b)]

def diameter(n, edges, nodes=None):
    adj = {}
    for a, b in edges: adj.setdefault(a, []).append(b); adj.setdefault(b, []).append(a)
    nodes = nodes if nodes is not None else range(n); best = 0
    for s in nodes:
        d = {s: 0}; q = deque([s])
        while q:
            u = q.popleft()
            for w in adj[u]:
                if w not in d: d[w] = d[u] + 1; q.append(w)
        best = max(best, max(d[t] for t in nodes))
    return best

def bisection(n, edges):
    best = len(edges)
    for half in combinations(range(1, n), n // 2 - 1):     # 固定節點 0 在第一半，避免重複
        s = set(half) | {0}
        best = min(best, sum((a in s) != (b in s) for a, b in edges))
    return best

def butterfly(n):
    """n 個節點、log n 欄、每欄 n/2 個 switch；節點 i 接到第 0 欄的 switch i//2，也接到最後一欄（課本說它其實是圓柱）"""
    L = int(log2(n)); h = n // 2; sw = lambda c, r: n + c * h + r; e = []
    for i in range(n): e += [(i, sw(0, i // 2)), (i, sw(L - 1, i // 2))]
    for c in range(L - 1):
        bit = 1 << (L - 2 - c)
        for r in range(h): e += [(sw(c, r), sw(c + 1, r)), (sw(c, r), sw(c + 1, r ^ bit))]
    return e

N = 16; k = isqrt(N)
print(f'N = {N}            diameter（實測／課本）   bisection（實測／課本）   links')
for name, e, dia, bis in [('chain', chain(N), N - 1, 1), ('ring', ring(N), N // 2, 2),
                          ('mesh', mesh(N), 2 * k - 2, k), ('torus', mesh(N, True), k, 2 * k),
                          ('hypercube', hypercube(N), int(log2(N)), N // 2)]:
    print(f'{name:10s}  {diameter(N, e):3d} / {dia:<3d}                {bisection(N, e):3d} / {bis:<3d}               {len(e)}')
for n in (8, 16, 64):
    print(f'butterfly N={n:2d}: diameter（節點到節點） {diameter(n, butterfly(n), range(n))}，課本 log N + 1 = {int(log2(n)) + 1}；'
          f'switch {n * int(log2(n)) // 2}，links {len(butterfly(n))}')
