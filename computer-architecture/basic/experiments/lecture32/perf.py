"""課本 §10.9 的效能模型：P = f / CPI，f = 1 / (tmax/k + l)，CPI = (n+k-1)/n + r*c*k
用數值掃 k，和公式 k_opt = sqrt(tmax / (l r c))、P = 1 / (sqrt(r c tmax) + sqrt(l))^2 比較"""
from math import sqrt

def P(k, tmax, l, r, c, n=float('inf')):
    f = 1 / (tmax / k + l)
    cpi = (1 if n == float('inf') else (n + k - 1) / n) + r * c * k
    return f / cpi

tmax, l = 20.0, 1.0                                   # Example 142：tmax/l = 20
for r, c, what in [(0.2, 0.6, 'Example 142'), (0.2, 0.3, 'c 減半（forwarding 讓停頓變便宜）'),
                   (0.4, 0.6, 'r 加倍（相依很多的程式）'), (0.0, 0.6, 'r = 0（理想 pipeline）')]:
    ks = range(1, 201)
    best = max(ks, key=lambda k: P(k, tmax, l, r, c, n=10**6 if r == 0 else float('inf')))
    form = sqrt(tmax / (l * r * c)) if r else sqrt((10**6 - 1) * tmax / l)
    line = f'{what:28s} r={r} c={c}: 數值最佳 k={best:3d}，公式 k={form:6.1f}'
    if r: line += f'，P(最佳整數 k)={P(best, tmax, l, r, c):.4f}，公式 P={1 / (sqrt(r * c * tmax) + sqrt(l)) ** 2:.4f}'
    print(line)
print('P(k) 曲線（r=0.2, c=0.6）：', ' '.join(f'k={k}:{P(k, tmax, l, 0.2, 0.6):.3f}' for k in (1, 2, 5, 8, 13, 20, 30, 50)))
# Example 143
for name, ld, br, tk, f in [('P1', 0.4, 0.2, 0.5, 1.0), ('P2', 0.3, 0.1, 0.4, 1.5)]:
    cpi = 1 + 0.5 * ld * 1 + br * tk * 2
    print(f'{name}: CPI = {cpi:.2f}，performance = f/CPI = {f / cpi:.2f} a.u.')
