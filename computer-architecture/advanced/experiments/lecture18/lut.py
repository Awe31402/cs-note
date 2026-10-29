# lut.py：影片 [13:37] 的 FPGA lookup table：兩個 4-bit 數相加 = 一張 2^8 格、每格 5 bit 的表
#   再把 16-bit 加法拆成 4 個「4-bit 加法 LUT」串起來（像 FPGA 把大運算拆成很多小 LUT）
lut = [((i >> 4) + (i & 15)) for i in range(256)]           # 位址 = a<<4 | b，內容 = a+b（最多 5 bit）
lut_c = [((i >> 5 & 15) + (i >> 1 & 15) + (i & 1)) for i in range(512)]  # 帶進位的版本：位址 = a<<5 | b<<1 | cin
print('4-bit 加法 LUT：', len(lut), '格，每格', max(lut).bit_length(), 'bit；全部對嗎：',
      all(lut[a << 4 | b] == a + b for a in range(16) for b in range(16)))
def add16(x, y):
    c, s = 0, 0
    for k in range(4):                                       # 4 個 LUT 串起來
        v = lut_c[(x >> 4 * k & 15) << 5 | (y >> 4 * k & 15) << 1 | c]
        s |= (v & 15) << 4 * k; c = v >> 4
    return s | c << 16
import random
r = random.Random(1)
print('16-bit 加法用 4 個帶進位的 LUT（每個 512 格）：隨機 10 萬組全部對嗎：',
      all(add16(x, y) == x + y for x, y in ((r.randrange(65536), r.randrange(65536)) for _ in range(100000))))
print('如果 16-bit 加法直接做成一張表：', 2 ** 32, '格 × 17 bit ≈', round(2 ** 32 * 17 / 8 / 2 ** 30, 1), 'GiB')
