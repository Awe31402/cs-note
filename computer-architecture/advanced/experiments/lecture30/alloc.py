# alloc.py：第 30 支的 switch 成本、arbiter、allocator
import random
from itertools import permutations

# 1. switch 的經驗法則：M × N 的 switch，面積 ≈ M·N、延遲 ≈ M + N（影片 [03:52]）
def sw(m, n): return m * n, m + n
print('1. 10 × 5 的 switch 三種做法（面積, 最長延遲）')
a, l = sw(10, 5); print(f'   一個 10 × 5：{a}, {l}')
a1, l1 = sw(2, 1); a2, l2 = sw(5, 5); print(f'   5 個 2 × 1 ＋ 一個 5 × 5：{5 * a1 + a2}, {l1 + l2}')
a1, l1 = sw(5, 2); a2, l2 = sw(4, 5); print(f'   2 個 5 × 2 ＋ 一個 4 × 5：{2 * a1 + a2}, {l1 + l2}')

# 2. arbiter
def round_robin(n):
    p = 0
    def arb(req):
        nonlocal p
        for k in range(n):
            i = (p + k) % n
            if req[i]: p = (i + 1) % n; return i
        return None
    return arb
def matrix_arb(n):
    W = [[1 if j > i else 0 for j in range(n)] for i in range(n)]   # W[i][j] = 1：i 比 j 優先
    def arb(req):
        for i in range(n):
            if req[i] and all(W[i][j] for j in range(n) if j != i and req[j]):
                for j in range(n):
                    if j != i: W[i][j], W[j][i] = 0, 1          # 自己的列清 0、行設 1：變最低優先
                return i
        return None
    return arb
def fixed_priority(n):
    return lambda req: next((i for i in range(n) if req[i]), None)

print('2. arbiter：4 個 agent 搶 1 個資源，每個 cycle 各有 60% 機率想要，跑 10 萬個 cycle')
for name, mk in (('固定優先（0 最大）', fixed_priority), ('round robin', round_robin), ('matrix', matrix_arb)):
    r = random.Random(1); arb = mk(4); wins = [0] * 4; wait = [0] * 4; worst = 0
    for _ in range(100000):
        req = [w > 0 or r.random() < 0.6 for w in wait]   # 沒拿到的會一直要
        g = arb(req)
        for i in range(4):
            if req[i] and i != g: wait[i] += 1; worst = max(worst, wait[i])
            elif i == g: wins[i] += 1; wait[i] = 0
    print(f'   {name:12}：每個 agent 拿到 {wins}，最久等了 {worst} 個 cycle')

# 3. allocator：N 個 request（agent）× N 個 resource
def maximum(R):
    n = len(R); best = 0
    for perm in permutations(range(n)):
        best = max(best, sum(1 for i in range(n) if R[i][perm[i]]))
    return best
def separable(R, arbs_req, arbs_res):
    """input-first（影片 [31:33]）：每個 request 先選一個想要的 resource，每個 resource 再從選它的裡面選一個"""
    n = len(R)
    pick = [arbs_req[i](R[i]) for i in range(n)]          # 第一層：每個 request 只送出一個請求
    pairs = []
    for j in range(n):
        g = arbs_res[j]([pick[i] == j for i in range(n)])
        if g is not None: pairs.append((g, j))
    return pairs
def wavefront(R):
    n = len(R); row = [True] * n; col = [True] * n; pairs = []
    for d in range(n):                                    # 第 d 個對角線（wrap around）
        for i in range(n):
            j = (i + d) % n
            if R[i][j] and row[i] and col[j]:             # 同時拿到 row token 和 column token
                row[i] = col[j] = False; pairs.append((i, j))
    return pairs
def maximal(R, pairs):                                    # 還能不能再多配一對？
    ri = {i for i, _ in pairs}; rj = {j for _, j in pairs}
    return not any(R[i][j] and i not in ri and j not in rj for i in range(len(R)) for j in range(len(R)))
print('3. allocator：4 × 4，每一對有 50% 機率「有興趣」，隨機 2 萬組')
r = random.Random(2); n = 4
tot = {'maximum': 0, 'separable': 0, 'wavefront': 0}; short = {'separable': 0, 'wavefront': 0}; notmax = {'separable': 0, 'wavefront': 0}
arbs_res = [round_robin(n) for _ in range(n)]; arbs_req = [round_robin(n) for _ in range(n)]
for _ in range(20000):
    R = [[r.random() < 0.5 for _ in range(n)] for _ in range(n)]
    m = maximum(R); s_ = separable(R, arbs_req, arbs_res); w = wavefront(R)
    tot['maximum'] += m; tot['separable'] += len(s_); tot['wavefront'] += len(w)
    for k, p in (('separable', s_), ('wavefront', w)):
        short[k] += len(p) < m; notmax[k] += not maximal(R, p)
print(f'   平均配對數：最大可能（maximum）{tot["maximum"] / 20000:.2f}、separable {tot["separable"] / 20000:.2f}、wavefront {tot["wavefront"] / 20000:.2f}')
print(f'   比 maximum 少的比例：separable {100 * short["separable"] / 20000:.1f}%、wavefront {100 * short["wavefront"] / 20000:.1f}%')
print(f'   連 maximal 都不是（明明還能多配一對）的比例：separable {100 * notmax["separable"] / 20000:.1f}%、wavefront {100 * notmax["wavefront"] / 20000:.1f}%')
# 影片 [32:38] 的例子：request 0 想要 resource 0、1；request 1 只想要 resource 0（第一層都先挑編號小的 resource）
R = [[True, True, False], [True, False, False], [False, False, False]]
lowest = lambda req: next((j for j in range(len(req)) if req[j]), None)
sep = separable(R, [lowest] * 3, [lambda req: 1 if req[1] else lowest(req)] + [lowest] * 2)
print(f'   影片的例子：maximum {maximum(R)}；separable 配了 {sep}（maximal？{maximal(R, sep)}）；wavefront 配了 {wavefront(R)}（maximal？{maximal(R, wavefront(R))}）')

# 4. router pipeline：每一 hop 的 head flit 要幾個 cycle
print('4. 10 個 hop，head flit 的延遲（沒有排隊）')
for name, c in (('5 級（BW RC VA SA ST）', 5), ('look-ahead routing（RC 移出）', 4), ('speculative VC allocation', 3), ('bypassing（佇列是空的）', 2)):
    print(f'   {name:28}：每 hop {c} cycle → {10 * c} cycle')
