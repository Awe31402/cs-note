# turns.py：第 29 支的 deadlock 分析，把 channel dependence graph（CDG）真的建出來找 cycle
#   mesh K × K；channel = 一條有方向的 link（例如 (1,1)→(2,1) 是往 E 的 channel）
#   CDG 的邊：channel c1 走到某個 router，接著可以走 c2（直走一定可以；轉彎要看 routing 允不允許；不能迴轉）
#   CDG 沒有 cycle → 不會 deadlock
from itertools import product
K = 4
D = {'E': (1, 0), 'W': (-1, 0), 'N': (0, 1), 'S': (0, -1)}
OPP = {'E': 'W', 'W': 'E', 'N': 'S', 'S': 'N'}
CW = [('N', 'E'), ('E', 'S'), ('S', 'W'), ('W', 'N')]      # 順時針的 4 個轉彎（影片 [38:16]）
CCW = [('N', 'W'), ('W', 'S'), ('S', 'E'), ('E', 'N')]     # 逆時針的 4 個轉彎
ALL_TURNS = set(CW + CCW)

def channels():
    for x, y in product(range(K), repeat=2):
        for d, (dx, dy) in D.items():
            if 0 <= x + dx < K and 0 <= y + dy < K: yield (x, y, d)
def has_cycle(allowed):
    ch = list(channels())
    nxt = {c: [] for c in ch}
    for (x, y, d) in ch:
        nx, ny = x + D[d][0], y + D[d][1]
        for d2 in D:
            if d2 == OPP[d]: continue
            if d2 != d and (d, d2) not in allowed: continue
            c2 = (nx, ny, d2)
            if c2 in nxt: nxt[(x, y, d)].append(c2)
    color = {c: 0 for c in ch}                              # DFS 找 cycle
    def dfs(u):
        color[u] = 1
        for v in nxt[u]:
            if color[v] == 1 or (color[v] == 0 and dfs(v)): return True
        color[u] = 2; return False
    return any(color[c] == 0 and dfs(c) for c in ch)

print(f'mesh {K} × {K}：')
print('   所有轉彎都允許：CDG 有 cycle？', has_cycle(ALL_TURNS))
xy = {t for t in ALL_TURNS if t[0] in 'EW'}               # XY：先走 x（E/W）再走 y，只允許 x → y 的轉彎
print('   XY routing（只允許 E/W → N/S，共 4 個轉彎）：CDG 有 cycle？', has_cycle(xy))
names = {('S', 'W'): 'west first 的一半', ('N', 'W'): 'west first 的一半'}
ok, bad = [], []
for a, b in product(CW, CCW):                             # 順、逆時針各拿掉一個 → 剩 6 個轉彎
    allowed = ALL_TURNS - {a, b}
    (bad if has_cycle(allowed) else ok).append((a, b))
print(f'   順、逆時針各拿掉一個轉彎（4 × 4 = 16 種）：沒有 cycle 的 {len(ok)} 種，還有 cycle 的 {len(bad)} 種')
fmt = lambda t: f'{t[0]}→{t[1]}'
for a, b in bad: print(f'      還有 cycle：拿掉 {fmt(a)} 和 {fmt(b)}')
for name, a, b in (('west first', ('S', 'W'), ('N', 'W')), ('north last', ('N', 'E'), ('N', 'W')), ('negative first', ('N', 'W'), ('E', 'S'))):
    print(f'   {name}（拿掉 {fmt(a)}、{fmt(b)}）：CDG 有 cycle？', has_cycle(ALL_TURNS - {a, b}))

# ring + dateline（影片 [44:04]）：n 個節點的單向 ring，每條 link 2 個 VC
def ring_cycle(n, dateline):
    nodes = [(i, v) for i in range(n) for v in (0, 1)]        # (link i：i → i+1, VC v)
    nxt = {c: [] for c in nodes}
    for i, v in nodes:
        j = (i + 1) % n
        if not dateline: nxt[(i, v)] += [(j, 0), (j, 1)]       # 沒有規則：可以換到任何一個 VC
        else:
            if v == 1:                                         # 過了 dateline 就只用 VC 1；
                if j != 0: nxt[(i, 1)].append((j, 1))          # 一個 packet 最多繞不到一圈，不會再跨一次 dateline
            elif j == 0: nxt[(i, 0)].append((j, 1))            # 跨過 dateline（link n−1 → link 0）就換到 VC 1
            else: nxt[(i, 0)].append((j, 0))
    color = {c: 0 for c in nodes}
    def dfs(u):
        color[u] = 1
        for w in nxt[u]:
            if color[w] == 1 or (color[w] == 0 and dfs(w)): return True
        color[u] = 2; return False
    return any(color[c] == 0 and dfs(c) for c in nodes)
print('ring 8 個節點、每條 link 2 個 VC：')
print('   VC 隨便用：CDG 有 cycle？', ring_cycle(8, False))
print('   dateline（從 VC 0 開始，跨過 dateline 就換到 VC 1、不再回去）：CDG 有 cycle？', ring_cycle(8, True))
