#!/usr/bin/env python3
"""第 3 支影片：用 NOT／AND 閘組 3-to-8 decoder，再用兩顆組成 4-to-16，窮舉驗證。"""

NOT = lambda a: 1 - a
AND = lambda *xs: int(all(xs))


def dec3to8(a2, a1, a0, en=1):
    """影片 [01:11]：3 個 NOT + 8 個 AND（有 enable 時每個 AND 4 個輸入）。"""
    n2, n1, n0 = NOT(a2), NOT(a1), NOT(a0)
    lines = [(n2, n1, n0), (n2, n1, a0), (n2, a1, n0), (n2, a1, a0),
             (a2, n1, n0), (a2, n1, a0), (a2, a1, n0), (a2, a1, a0)]
    return [AND(en, *l) for l in lines]


def dec4to16(a3, a2, a1, a0):
    """影片 [03:28]：兩顆 3-to-8，enable 用 a3 和 NOT(a3)，所以只有一顆會動。"""
    return dec3to8(a2, a1, a0, en=NOT(a3)) + dec3to8(a2, a1, a0, en=a3)


ok = True
print("addr  word line（3-to-8）")
for v in range(8):
    out = dec3to8((v >> 2) & 1, (v >> 1) & 1, v & 1)
    print(f"{v:03b}   {''.join(map(str, out))}")
    ok &= out == [int(i == v) for i in range(8)]

for v in range(8):  # enable = 0 時全部是 0
    ok &= dec3to8((v >> 2) & 1, (v >> 1) & 1, v & 1, en=0) == [0] * 8

for v in range(16):
    out = dec4to16(*[(v >> s) & 1 for s in (3, 2, 1, 0)])
    ok &= out == [int(i == v) for i in range(16)]

print("3-to-8、enable、4-to-16 全部 one-hot 正確：", "OK" if ok else "FAIL")

# 閘數：n-to-2^n 用 n 個 NOT、2^n 個 n 輸入 AND
for n in (3, 4, 10, 14):
    print(f"{n:>2}-to-{2**n:<6} decoder：{n} 個 NOT、{2**n} 個 {n} 輸入 AND")
