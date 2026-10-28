# moore.py：用影片和課本給的數字，算「多久翻倍一次」
from math import log2

def per_doubling(years, ratio):
    return years / log2(ratio)

# 電晶體數量：影片「2,000 → 20 億」；課本 p.568 說 1971 年約 2000–3000 個，影片的圖畫到 2018
print(f"電晶體 2000→2e9（1971→2018）：每 {per_doubling(2018 - 1971, 2e9 / 2000):.2f} 年翻倍")
# feature size 每一代縮 √2 → 一代 = 面積減半 = 電晶體翻倍
for a, b, ya, yb, src in [(1000, 14, 1980, 2016, "影片"), (14, 7, 2016, 2020, "影片"), (130, 22, 2001, 2011, "課本 Table 12.1")]:
    gens = 2 * log2(a / b)
    print(f"feature size {a}→{b} nm（{ya}→{yb}，{src}）：{gens:.1f} 代，每代 {(yb - ya) / gens:.2f} 年")
# 5 nm 有幾個原子：影片用 1 Å = 0.1 nm
print(f"5 nm / 0.1 nm = {5 / 0.1:.0f} 個原子寬")
# power ∝ f³（課本 p.477）
for x in (1.1, 2, 4.9 / 2.9):
    print(f"頻率 ×{x:.2f} → 功率 ×{x ** 3:.2f}")
