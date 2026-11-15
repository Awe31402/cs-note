#!/usr/bin/env python3
"""第 2 支影片：8x8、一次 1 bit 的 DRAM 模型，照影片的步驟跑讀和寫。

重點：
  * 位址 6 bit，分兩次送（先 row 3 bit 配 RAS#，再 column 3 bit 配 CAS#）
  * 打開 word line 會把整個 row 讀進 sense amplifier，cell 被「破壞」
  * 最後由 sense amplifier 把 row 寫回去（refresh），寫入時先改 sense amplifier
"""

# 影片 [01:06] 畫面上的 8x8 內容（紅=1、藍=0），row 0 在最上面
INIT = [
    "10000000",
    "00111110",
    "01111101",
    "00000100",
    "00000000",
    "11010011",
    "00101000",
    "00000001",
]


class Dram8x8:
    def __init__(self, rows):
        self.cell = [[int(c) for c in r] for r in rows]
        self.sense = [None] * 8          # sense amplifier 的 latch
        self.open_row = None
        self.log = []

    def ras(self, row):                  # RAS# 拉低：row 位址被 latch、decode
        self.log.append(f"RAS# low, row={row:03b}: precharge -> float -> word line {row} 開")
        self.open_row = row
        self.sense = self.cell[row][:]   # 整個 row 進 sense amplifier
        self.cell[row] = ["?"] * 8       # 破壞性讀：cell 的電被分掉了

    def cas_read(self, col):             # CAS# 拉低：column mux 選一個
        bit = self.sense[col]
        self.log.append(f"CAS# low, col={col:03b}: mux 選第 {col} 個 sense amp -> data out = {bit}")
        return bit

    def cas_write(self, col, bit):       # WE# 低 + CAS# 低：demux 改一個 sense amp
        self.log.append(f"WE# low, CAS# low, col={col:03b}: demux 把 {bit} 寫進第 {col} 個 sense amp")
        self.sense[col] = bit

    def close(self):                     # 寫回整個 row，關 word line，再 precharge
        self.cell[self.open_row] = self.sense[:]
        self.log.append(f"sense amp 寫回 row {self.open_row}（refresh），word line 關，precharge")
        self.open_row = None

    def dump(self):
        return "\n".join("".join(str(b) for b in r) for r in self.cell)


def addr_split(addr6):
    return addr6 >> 3, addr6 & 7         # 高 3 bit = row，低 3 bit = column


d = Dram8x8(INIT)

# 讀：影片 [05:21] row = 101，[06:24] column = 011
row, col = 0b101, 0b011
bit = None
d.ras(row)
print("打開 row 後的 cell（? = 電被分掉）：")
print(d.dump(), "\n")
bit = d.cas_read(col)
d.close()
print("\n".join(d.log))
print(f"\n讀到 row {row} col {col} = {bit}")
assert d.dump() == "\n".join(INIT), "讀完應該跟原來一模一樣"
print("讀完整個陣列跟原來一樣：OK\n")

# 寫：影片 [08:57] row = 100，[09:27] column = 110，把 0 改成 1
d.log.clear()
row, col = 0b100, 0b110
print(f"寫之前 row {row} = {''.join(map(str, d.cell[row]))}")
d.ras(row)
d.cas_write(col, 1)
d.close()
print("\n".join(d.log))
print(f"寫之後 row {row} = {''.join(map(str, d.cell[row]))}")

# 位址多工省了多少腳
for bits in (6, 24, 32):
    print(f"{bits:>2} bit 位址：不多工要 {bits} 支腳，多工只要 {(bits + 1) // 2} 支")
