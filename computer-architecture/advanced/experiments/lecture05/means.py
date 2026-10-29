# means.py：為什麼比較一組程式要用「比值的幾何平均」
from math import prod
# 三個程式在機器 A、B、C 上的執行時間（秒）；數字是假設的
T = {'A': [1, 1000, 10], 'B': [2, 500, 10], 'C': [1, 1500, 4]}
gm = lambda xs: prod(xs) ** (1 / len(xs))
print("算術平均時間：", {m: sum(t) / 3 for m, t in T.items()})
for ref in 'ABC':
    r = {m: gm([T[ref][i] / T[m][i] for i in range(3)]) for m in T}
    print(f"以 {ref} 為基準，比值的幾何平均（越大越快）：", {m: round(v, 3) for m, v in r.items()})

# Pn = 1 − (1 − p)^(n/5)：window 裡 n 條指令，至少猜錯一個 branch 的機率
Pn = lambda p, n: 1 - (1 - p) ** (n / 5)
for target in (0.10, 0.20):
    p = 1 - (1 - target) ** (5 / 100)
    print(f"n=100，要 Pn ≤ {target:.0%}：每個 branch 猜錯率 p ≤ {p:.3%}（準確率 ≥ {1 - p:.2%}）")
print(f"n=100，p=2.5%：Pn = {Pn(0.025, 100):.1%}")
for n in (100, 150, 500, 1000):
    print(f"p=0.5%，n={n:4d}：Pn = {Pn(0.005, n):.1%}")
