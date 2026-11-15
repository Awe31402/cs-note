#!/usr/bin/env python3
"""第 4 支影片：用 AND／OR／NOT 組 8-to-1 MUX 和 1-to-8 DEMUX，窮舉驗證，再跑一次並列->串列->並列。"""

NOT = lambda a: 1 - a
AND = lambda *xs: int(all(xs))
OR = lambda *xs: int(any(xs))


def select_terms(s2, s1, s0):
    n2, n1, n0 = NOT(s2), NOT(s1), NOT(s0)
    return [(n2, n1, n0), (n2, n1, s0), (n2, s1, n0), (n2, s1, s0),
            (s2, n1, n0), (s2, n1, s0), (s2, s1, n0), (s2, s1, s0)]


def mux8(data, s2, s1, s0):
    """影片 [03:58]：8 個 4 輸入 AND（資料 + 3 條控制線）接一個 8 輸入 OR。"""
    return OR(*[AND(d, *t) for d, t in zip(data, select_terms(s2, s1, s0))])


def demux8(d, s2, s1, s0):
    """影片 [05:27]：就是帶 enable 的 decoder，只是 enable 改當資料輸入。"""
    return [AND(d, *t) for t in select_terms(s2, s1, s0)]


ok = True
for word in range(256):
    data = [(word >> (7 - i)) & 1 for i in range(8)]
    for s in range(8):
        sel = ((s >> 2) & 1, (s >> 1) & 1, s & 1)
        ok &= mux8(data, *sel) == data[s]
for d in (0, 1):
    for s in range(8):
        out = demux8(d, (s >> 2) & 1, (s >> 1) & 1, s & 1)
        ok &= out == [d if i == s else 0 for i in range(8)]
print("8-to-1 MUX（256x8 種）與 1-to-8 DEMUX（2x8 種）全部正確：", "OK" if ok else "FAIL")

# 影片 [01:12]：並列 -> 串列 -> 並列，兩邊的控制線同步數 000..111
data = [1, 0, 1, 0, 1, 0, 1, 0]          # 影片畫面上的 8 個輸入
buf = [None] * 8
wire = []
for s in range(8):
    sel = ((s >> 2) & 1, (s >> 1) & 1, s & 1)
    bit = mux8(data, *sel)               # 這一拍線上只有一個 bit
    wire.append(bit)
    out = demux8(bit, *sel)
    buf[s] = out[s]                      # 收端 buffer 存起來
print("線上依序傳送：", "".join(map(str, wire)))
print("收端 buffer  ：", "".join(map(str, buf)), "（跟輸入一樣）" if buf == data else "（錯）")

# 如果控制線沒同步（收端慢一拍），會怎樣
buf2 = [None] * 8
for s in range(8):
    tx = ((s >> 2) & 1, (s >> 1) & 1, s & 1)
    r = (s - 1) % 8
    rx = ((r >> 2) & 1, (r >> 1) & 1, r & 1)
    buf2[r] = demux8(mux8(data, *tx), *rx)[r]
print("收端慢一拍   ：", "".join(map(str, buf2)), "（全部錯位）")
