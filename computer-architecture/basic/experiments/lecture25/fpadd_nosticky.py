"""照課本 Figure 8.17 的流程做 float32 加法（只處理 normal 數）：
解開欄位 → 對齊（右移 W，維護 guard／round／sticky）→ 加或減 → 取絕對值、記號 → 正規化 → 捨入 → 再正規化。"""
import sys

EXTRA = 3                                  # W 的最低 3 位：guard、round、sticky

def unpack(x):
    return x >> 31, (x >> 23) & 0xFF, (x & 0x7FFFFF) | 0x800000      # S, E, 24 位元 significand

def shr_sticky(w, k):                      # 右移 k 位，掉出去的位元 OR 進 sticky（最低位）
    if k <= 0: return w
    lost = w & ((1 << k) - 1)
    return w >> k                           # 拿掉 sticky：掉出去的位元直接丟掉

def fadd(a, b, mode):                      # mode: 'rtz' 'rup' 'rdn' 'rne'
    (sa, ea, pa), (sb, eb, pb) = unpack(a), unpack(b)
    if ea < eb or (ea == eb and pa < pb): (sa, ea, pa), (sb, eb, pb) = (sb, eb, pb), (sa, ea, pa)
    E, S = ea, sa                           # 預設：A 的指數和符號
    W = shr_sticky(pb << EXTRA, ea - eb)    # 對齊小數點
    W = (pa << EXTRA) + W if sa == sb else (pa << EXTRA) - W   # 同號相加，異號相減（|A| ≥ |B|，W 不會是負的）
    if W == 0: return 0
    while W >= (1 << (24 + EXTRA)):         # 太大（≥ 2）：右移、E + 1
        W = shr_sticky(W, 1); E += 1
    while W < (1 << (23 + EXTRA)):          # 太小（< 1）：左移、E − 1
        W <<= 1; E -= 1
    P, g, rest = W >> EXTRA, (W >> (EXTRA - 1)) & 1, W & ((1 << (EXTRA - 1)) - 1)
    r, s = g, 1 if rest else 0              # round 位元 = 殘餘的最高位；sticky = 其他位的 OR
    inc = {'rtz': False,
           'rup': S == 0 and (r or s),
           'rdn': S == 1 and (r or s),
           'rne': (r and s) or (r and not s and P & 1)}[mode]   # R > 0.5，或 R = 0.5 且 LSB 是 1
    P += inc
    if P == (1 << 24): P >>= 1; E += 1      # 捨入後再正規化
    return (S << 31) | (E << 23) | (P & 0x7FFFFF)

bad = {m: 0 for m in ('rtz', 'rup', 'rdn', 'rne')}
n = 0
for line in open(sys.argv[1]):
    a, b, *ref = [int(x, 16) for x in line.split()]
    for m, r in zip(('rtz', 'rup', 'rdn', 'rne'), ref):
        bad[m] += fadd(a, b, m) != r
    n += 1
print(f"{n} additions; mismatches vs x86 hardware:", bad)
