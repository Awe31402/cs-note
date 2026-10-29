# topo.py：把影片 [53:33] 那張表的 diameter 和 bisection bandwidth 真的算出來（n = 16 個節點）
#   diameter：每一對節點用 BFS 找最短路徑，取最大的
#   bisection bandwidth：試遍所有「分成兩半、各 8 個」的切法（C(16,8)/2 = 6435 種），取要剪最少條 link 的
#   另外：buffered wire 的最佳段數；folded torus 的線長
from itertools import combinations
from collections import deque
import math

def bfs(adj, s):
    d = {s: 0}; q = deque([s])
    while q:
        u = q.popleft()
        for v in adj[u]:
            if v not in d: d[v] = d[u] + 1; q.append(v)
    return d
def diameter(adj): return max(max(bfs(adj, s).values()) for s in adj)
def bisection(adj):
    nodes = sorted(adj); n = len(nodes); best = None
    for half in combinations(nodes[1:], n // 2 - 1):       # 節點 0 固定在一邊，避免重複
        A = set((nodes[0],) + half)
        cut = sum(1 for u in A for v in adj[u] if v not in A)
        best = cut if best is None else min(best, cut)
    return best
def build(edges, n):
    adj = {i: set() for i in range(n)}
    for a, b in edges: adj[a].add(b); adj[b].add(a)
    return adj

n, k = 16, 4
topos = {
    'chain': [(i, i + 1) for i in range(n - 1)],
    'ring': [(i, (i + 1) % n) for i in range(n)],
    'mesh 4×4': [(r * k + c, r * k + c + 1) for r in range(k) for c in range(k - 1)] + [(r * k + c, (r + 1) * k + c) for r in range(k - 1) for c in range(k)],
    'torus 4×4': [(r * k + c, r * k + (c + 1) % k) for r in range(k) for c in range(k)] + [(r * k + c, ((r + 1) % k) * k + c) for r in range(k) for c in range(k)],
    'hypercube H4': [(i, i ^ (1 << b)) for i in range(n) for b in range(4) if i < i ^ (1 << b)],
}
print(f'{"topology":14}{"link 數":>8}{"diameter":>10}{"bisection":>11}   公式代入 n=16、每邊 √n=4：(link, diameter, bisection)')
formula = {'chain': (n - 1, n - 1, 1), 'ring': (n, n // 2, 2), 'mesh 4×4': (2 * k * (k - 1), 2 * (k - 1), k),
           'torus 4×4': (2 * n, k, 2 * k), 'hypercube H4': (n * 4 // 2, 4, n // 2)}
for name, e in topos.items():
    adj = build(e, n); L = sum(len(v) for v in adj.values()) // 2
    print(f'{name:14}{L:>8}{diameter(adj):>10}{bisection(adj):>11}   {formula[name]}')

# buffered wire：總延遲 = (s−1)·d ＋ s·κ(L/s)² = (s−1)d ＋ κL²/s（影片 [08:21]）
kappa, d = 1.0, 4.0
print('buffered wire（κ = 1、每個 buffer d = 4）：')
for L in (10, 20, 40, 80):
    best = min(range(1, 200), key=lambda s: (s - 1) * d + kappa * L * L / s)
    sopt = math.sqrt(kappa / d) * L
    print(f'   L = {L:3d}：不加 buffer 延遲 {kappa * L * L:7.0f}；最佳 s = {best}（公式 √(κ/d)·L = {sopt:.1f}），'
          f'延遲 {(best - 1) * d + kappa * L * L / best:6.1f}（公式 2√(κd)·L − d = {2 * math.sqrt(kappa * d) * L - d:.1f}）')

# folded torus：一列 8 個節點。一般 torus：0-1-…-7 再一條 7→0 的長線；folded：把環 0,1,…,7 排成 0 7 1 6 2 5 3 4
ring = list(range(8))
plain_pos = {v: v for v in ring}
folded_order = [0, 7, 1, 6, 2, 5, 3, 4]
fold_pos = {v: i for i, v in enumerate(folded_order)}
for name, pos in (('一般 torus', plain_pos), ('folded torus', fold_pos)):
    lens = [abs(pos[ring[i]] - pos[ring[(i + 1) % 8]]) for i in range(8)]
    print(f'{name}：環上 8 條 link 的實體長度 {lens}，最長 {max(lens)}')
