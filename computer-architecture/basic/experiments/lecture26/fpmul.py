"""照課本 Figure 8.18 做 float32 乘法（只處理 normal 數）：S = S_A xor S_B；E = E_A + E_B − bias；
W = P_A × P_B（48 位元，小數點後 46 位）；正規化；捨入（round／sticky）；再正規化。"""
import sys
def unpack(x): return x >> 31, (x >> 23) & 0xFF, (x & 0x7FFFFF) | 0x800000

def fmul(a, b, mode):
    (sa, ea, pa), (sb, eb, pb) = unpack(a), unpack(b)
    S, E, W = sa ^ sb, ea + eb - 127, pa * pb          # W 有 47 或 48 位元
    if W >> 47: E += 1; keep = 24                      # significand ≥ 2：等於右移一位
    else: keep = 23
    P, R = W >> keep, W & ((1 << keep) - 1)            # 保留 24 位元，其餘是殘餘
    r, s = R >> (keep - 1), int(R & ((1 << (keep - 1)) - 1) != 0)
    inc = {'rtz': False, 'rup': S == 0 and (r or s), 'rdn': S == 1 and (r or s),
           'rne': (r and s) or (r and not s and P & 1)}[mode]
    P += inc
    if P == 1 << 24: P >>= 1; E += 1
    return (S << 31) | (E << 23) | (P & 0x7FFFFF)

bad, n = {m: 0 for m in ('rtz', 'rup', 'rdn', 'rne')}, 0
for line in open(sys.argv[1]):
    a, b, *ref = [int(x, 16) for x in line.split()]
    for m, r in zip(bad, ref): bad[m] += fmul(a, b, m) != r
    n += 1
print(f"{n} multiplications; mismatches vs x86 hardware:", bad)
