#!/usr/bin/env python3
"""第 7 支影片：9 bit 位址怎麼切成 row／column／bank，以及切法對速度的影響。

影片的切法（高位 -> 低位）： row[3] | col_hi[1] | bank[3] | col_lo[2]
  * 每個位址 = 一個 byte（一個 bank 有 8 個 array，同位置各出 1 bit）
  * burst length = 4（col_lo 的 2 bit 由 DIMM 自己數）
"""

def split_video(a):
    row = (a >> 6) & 7
    col_hi = (a >> 5) & 1
    bank = (a >> 2) & 7
    col_lo = a & 3
    return row, bank, (col_hi << 2) | col_lo


def split_naive(a):
    """對照組：bank 放最高位，row、col 照順序（連續位址都在同一個 bank）。"""
    bank = (a >> 6) & 7
    row = (a >> 3) & 7
    col = a & 7
    return row, bank, col


print("影片的對照表（只列每個 burst 的第一個位址）：")
print(" addr  bits(r|c|bbb|cc)  row bank col")
for a in range(0, 80, 4):
    r, b, c = split_video(a)
    s = f"{a:09b}"
    print(f"{a:>5}  {s[:3]}|{s[3]}|{s[4:7]}|{s[7:]}       {r}    {b}   {c}")

# 每個位址都對到唯一的 (row, bank, col)：切法只是重新排列 bit
assert len({split_video(a) for a in range(512)}) == 512
assert len({split_naive(a) for a in range(512)}) == 512


def simulate(split, n_bursts, t_open=3, t_burst=1, t_recover=3, n_banks=8):
    """影片第 6、7 支的模型：每個 burst 都要「開 row -> 送資料 -> 關 row 回復」。
    data bus 一次只能送一個 burst；同一個 bank 回復完才能再開。"""
    bank_ready = [0] * n_banks
    bus_free = 0
    for i in range(n_bursts):
        _, b, _ = split(i * 4)
        start = bank_ready[b]
        data = max(start + t_open, bus_free)
        bus_free = data + t_burst
        bank_ready[b] = bus_free + t_recover
    return bus_free


N = 128   # 讀 128 個 burst = 512 byte，整個模型讀一遍
for name, f in (("影片的切法（bank 在中間）", split_video), ("bank 在最高位", split_naive)):
    t = simulate(f, N)
    print(f"{name:<16}: {N} 個 burst 花 {t:>3} 個時間單位，bus 忙碌 {N / t:.0%}")

# bank 數不夠多時：輪一圈回來，第一個 bank 還沒回復好
for nb in (1, 2, 4, 8, 16):
    bits = nb.bit_length() - 1
    def split_nb(a, bits=bits):
        return 0, (a >> 2) & ((1 << bits) - 1), 0
    t = simulate(split_nb, N, n_banks=nb)
    print(f"{nb:>2} 個 bank 輪流：bus 忙碌 {N / t:.0%}")
