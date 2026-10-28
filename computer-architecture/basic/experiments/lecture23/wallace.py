"""用 lecture21 的閘級模型做三種（無號）乘法器，量最長路徑的閘延遲：
 iterative（n 次 CLA 串接）、O(log² n) 的加法器樹、Wallace tree（CSA 樹 ＋ 一個 CLA）。"""
import math, random, sys
sys.argv = ['x']                               # 不要執行 adders.py 的測試
exec(open('adders.py').read().split('random.seed(1)')[0])
Z = (0, 0)

def partial_sums(A, B, w):                     # 第 i 個部分和 = (A AND b_i) << i，全部同時產生（1 層 AND）
    return [[Z] * i + [AND(a, B[i]) for a in A] + [Z] * (w - len(A) - i) for i in range(len(B))]

def add(x, y):                                 # w 位元的 carry lookahead 加法（丟掉最後的 carry）
    s, _ = lookahead(x, y, Z); return s

def iterative(P):
    acc = P[0]
    for p in P[1:]: acc = add(acc, p)          # 一個接一個加：n 次
    return acc

def tree(P):
    while len(P) > 1:                          # 兩兩相加，log n 層
        P = [add(P[i], P[i + 1]) for i in range(0, len(P) - 1, 2)] + ([P[-1]] if len(P) % 2 else [])
    return P[0]

def csa(a, b, c):                              # carry save adder：每一位一個 full adder，O(1)
    fa = [full_adder(x, y, z) for x, y, z in zip(a, b, c)]
    return [Z] + [f[1] for f in fa[:-1]], [f[0] for f in fa]   # D（carry 左移一位）、E（sum）

def wallace(P):
    levels = 0
    while len(P) > 2:
        nxt = []
        for i in range(0, len(P) - 2, 3): nxt += csa(P[i], P[i + 1], P[i + 2])
        nxt += P[len(P) - len(P) % 3:] if len(P) % 3 else []
        P, levels = nxt, levels + 1
    return add(P[0], P[1]), levels

def num(s): return sum(b[0] << i for i, b in enumerate(s))
random.seed(2)
print(f"{'n':>3} {'iterative':>10} {'log² tree':>10} {'Wallace':>8} {'CSA levels':>11}  correct")
for n in (4, 8, 16, 32):
    w, ok, worst = 2 * n, True, [0, 0, 0]
    for a, b in [(2**n - 1, 2**n - 1)] + [(random.getrandbits(n), random.getrandbits(n)) for _ in range(30)]:
        A, B = bits(a, n), bits(b, n)
        for j, f in enumerate((iterative, tree, lambda P: wallace(P)[0])):
            s = f(partial_sums(A, B, w))
            ok &= num(s) == a * b
            worst[j] = max(worst[j], max(x[1] for x in s))
    lv = wallace(partial_sums(bits(1, n), bits(1, n), w))[1]
    print(f"{n:>3} {worst[0]:>10} {worst[1]:>10} {worst[2]:>8} {lv:>11}  {ok}")
