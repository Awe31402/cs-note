# ARM 的 NZCV：照 ARM 的定義用 32 位元算（減法 a - b = a + ~b + 1）
M = 0xFFFFFFFF
def s32(x): x &= M; return x - (1 << 32) if x >> 31 else x
def add(a, b, cin=0):
    r = (a & M) + (b & M) + cin
    res = r & M
    N, Z, C = res >> 31, int(res == 0), r >> 32
    V = int((s32(a) < 0) == (s32(b) < 0) and (s32(res) < 0) != (s32(a) < 0))
    return res, f"N={N} Z={Z} C={C} V={V}"
def sub(a, b, cin=1): return add(a, ~b & M, cin)

for name, (res, f) in [("cmp 0, 5", sub(0, 5)), ("cmp 5, 0", sub(5, 0)), ("cmp 5, 5", sub(5, 5)),
                       ("-1 + -2 ", add(-1, -2)), ("0x7FFFFFFF + 1", add(0x7FFFFFFF, 1))]:
    print(f"{name:16} -> {s32(res):12d}  {f}")

# 64 位元加法：adds r5, r1, r3 ; adc r6, r2, r4
a, b = 0x00000001_FFFFFFFF, 0x00000002_00000001
r1, r2, r3, r4 = a & M, a >> 32, b & M, b >> 32
r5, f = add(r1, r3); C = int(f.split("C=")[1][0])
r6, _ = add(r2, r4, C)
print(f"64-bit add: 0x{a:016X} + 0x{b:016X} = 0x{r6:08X}{r5:08X}  (python: 0x{a+b:016X})")
# 64 位元減法：subs r5, r1, r3 ; sbc r6, r2, r4（sbc 減掉 NOT(C)）
a, b = 0x00000005_00000000, 0x00000001_00000001
r5, f = sub(a & M, b & M); C = int(f.split("C=")[1][0])
r6, _ = sub(a >> 32, b >> 32, C)
print(f"64-bit sub: 0x{a:016X} - 0x{b:016X} = 0x{r6:08X}{r5:08X}  (python: 0x{a-b:016X})")
print(f"bic 0b1101, 0b0110 = 0b{0b1101 & ~0b0110 & 0xF:04b}")
