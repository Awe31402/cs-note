#!/usr/bin/env python3
"""第 6 支影片：DIMM 的寬度、burst、cache line 之間的算術。"""
import subprocess

BUS_BITS = 64            # 一個 channel 的資料線（ECC 是 72）
CHIPS = 8                # 一個 rank 8 顆 x8 晶片
print(f"每顆晶片的資料線 = {BUS_BITS} / {CHIPS} = {BUS_BITS // CHIPS}（x8 晶片）")

# 影片 [02:47]：「有效頻率」2132 MHz，DDR 一個 clock 送兩次
mts = 2132e6
print(f"\n2132 MT/s：clock = {mts / 2 / 1e6:.0f} MHz，每拍 {1e9 / mts:.3f} ns，"
      f"一個 channel 峰值 {mts * BUS_BITS / 8 / 1e9:.2f} GB/s")
for ch in (1, 2, 4):
    print(f"  {ch} channel：{ch * mts * BUS_BITS / 8 / 1e9:6.2f} GB/s")

# burst：一次 burst 送幾 byte，夠不夠一個 64 B cache line
LINE = 64
print()
for gen, bl, width in (("DDR2", 4, 64), ("DDR3", 8, 64), ("DDR4", 8, 64), ("DDR5", 16, 32)):
    got = bl * width // 8
    print(f"{gen}: BL{bl:<2} x {width} bit sub-channel = {got:>3} B -> "
          f"{'剛好一個' if got == LINE else ('半個' if got * 2 == LINE else '?')} 64 B cache line")

# 本機的 cache line 大小
line = subprocess.run(["getconf", "LEVEL1_DCACHE_LINESIZE"], capture_output=True, text=True).stdout.strip()
print(f"\n本機 getconf LEVEL1_DCACHE_LINESIZE = {line}")

# 容量：影片的 array 16384 x 1024，一個 bank 8 個 array，一顆晶片 8 個 bank，一個 rank 8 顆
arr = 16384 * 1024
bank = arr * 8
chip = bank * 8
rank = chip * 8
print(f"\narray {arr / 2**20:.0f} Mib，bank {bank / 2**20:.0f} Mib，chip {chip / 2**30:.0f} Gib，"
      f"一個 rank {rank / 8 / 2**30:.0f} GiB")
print(f"位址 bit：row 14 + col 10 + bank 3 = {14 + 10 + 3}，乘上每次 8 B（64 bit）-> "
      f"{2**27 * 8 / 2**30:.0f} GiB，跟上面一樣")
