# ARM 的 12 位元立即值：8 位元 payload 右旋 2×rot。列出所有能表示的 32 位元常數
def ror(x, n): return ((x >> n) | (x << (32 - n))) & 0xFFFFFFFF
vals = {ror(p, 2 * r) for r in range(16) for p in range(256)}
print("distinct encodable constants:", len(vals))
for v in (42, 0x2a00, 0xfe000000, 0x101, 0x3fc, 0xff00, 4095, 0x12345678):
    enc = [(r, p) for r in range(16) for p in range(256) if ror(p, 2 * r) == v]
    print(f"0x{v:08x}: " + (f"rot={enc[0][0]} payload=0x{enc[0][1]:02x} -> imm12=0x{enc[0][0]<<8|enc[0][1]:03x}" if enc else "不能表示"))
