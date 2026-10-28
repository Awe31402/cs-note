"""用 Fraction 精確計算，看 Goldschmidt 和 Newton-Raphson 求 1/b（1 ≤ b < 2）收斂得多快。"""
from fractions import Fraction as F
import math
bs = [F(1) + F(i, 1000) for i in range(1, 1000)]        # b = 1.001 … 1.999

print("Newton-Raphson: x0 = 1/2, x_{k+1} = 2x_k − b·x_k²；|E| = |b·x − 1| 的最大值")
xs = {b: F(1, 2) for b in bs}
for k in range(6):
    err = max(abs(b * xs[b] - 1) for b in bs)
    print(f"  iteration {k}: max |E| = {float(err):.3e}  (≈ 2^{math.log2(err):.1f})")
    xs = {b: 2 * x - b * x * x for b, x in xs.items()}

print("Goldschmidt: 1/b = 1/2 × 1/(1−y)，y = (1 − x)/2 = (2 − b)/2；只乘到 (1+y^(2^k))")
for terms in range(1, 6):
    worst = 0
    for b in bs:
        y = (2 - b) / 2
        prod, p = F(1), y
        for _ in range(terms): prod *= 1 + p; p *= p
        worst = max(worst, abs(prod / 2 - 1 / b) * b)       # 相對誤差
    print(f"  {terms} terms (up to 1+y^{2**(terms-1)}): max relative error {float(worst):.3e} (≈ 2^{math.log2(worst):.1f})")
print("float32 的 mantissa 精度：2^-23 =", f"{2**-23:.3e}")
