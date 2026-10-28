"""從 valgrind lackey 的記憶體 trace 算出課本 §11.1.4–11.1.5 的兩個分佈：
stack distance（時間局部性）與 address distance（空間局部性）。
做法照課本：前 100 萬次存取暖身（只維護 stack，不計數），接下來 100 萬次才計數；
stack 深度上限 L = 1000；address distance 用最近 K = 10 次存取。
用法：valgrind --tool=lackey --trace-mem=yes 程式 2>&1 | grep -E '^ [LSM] ' > x.trace
      python3 -I locality.py x.trace [line]   （加 line：stack distance 改用 64-byte 區塊編號）"""
import sys
from collections import Counter

L, K, WARM, COUNT = 1000, 10, 10**6, 10**6
addrs = []
with open(sys.argv[1]) as f:
    for line in f:
        a = int(line.split()[1].split(',')[0], 16)
        addrs.append(a)
        if len(addrs) == WARM + COUNT: break

LINE = len(sys.argv) > 2 and sys.argv[2] == 'line'
stack, sd, ad, window = [], Counter(), Counter(), []
for i, a in enumerate(addrs):
    key = a >> 6 if LINE else a
    try:
        d = stack.index(key); del stack[d]          # 找到：距離 = 離頂端幾格，移到頂端
    except ValueError:
        d = L                                     # 沒找到（或已被擠出去）：當成 L
        if len(stack) == L: stack.pop()
    stack.insert(0, key)
    if window:
        near = min(window, key=lambda w: abs(a - w)); dist = a - near
    window.append(a); window[:] = window[-K:]
    if i >= WARM:
        sd[d] += 1
        if len(window) > 1: ad[dist] += 1

n = sum(sd.values())
def frac(c, test): return sum(v for k, v in c.items() if test(k)) / n
print(f'{sys.argv[1].rsplit("/", 1)[-1]}{"（以 64-byte 區塊計）" if LINE else ""}: 計數 {n} 次存取')
print(f'  stack distance 0–9     : {frac(sd, lambda d: d < 10):6.1%}   （課本 perlbench：約 27%）')
print(f'  stack distance < 100   : {frac(sd, lambda d: d < 100):6.1%}   （課本：超過三分之二）')
print(f'  stack distance = L（沒找到）: {frac(sd, lambda d: d == L):6.1%}')
print(f'  address distance ±5    : {frac(ad, lambda d: abs(d) <= 5):6.1%}   （課本：超過四分之一）')
print(f'  address distance ±25   : {frac(ad, lambda d: abs(d) <= 25):6.1%}   （課本：超過三分之二）')
print(f'  address distance 0     : {frac(ad, lambda d: d == 0):6.1%}')
print('  stack distance 分佈（每 10 一格）：', ' '.join(f'{frac(sd, lambda d, b=b: b <= d < b + 10):.2f}' for b in range(0, 100, 10)))
print('  最常見的 address distance：', ', '.join(f'{k:+d}:{v / n:.1%}' for k, v in ad.most_common(6)))
