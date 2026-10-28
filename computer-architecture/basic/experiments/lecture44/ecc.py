"""課本 §13.4.2：parity、(12,8) SEC（Table 13.2 的位置）、SECDED（多一個 P5）、4-bit CRC（Example 156）。
全部用窮舉驗證：單一錯誤有沒有被更正、雙重錯誤有沒有被偵測、code word 之間的最小 Hamming distance。"""
from itertools import combinations

DATA_POS = [3, 5, 6, 7, 9, 10, 11, 12]          # D1..D8 放在第 3、5、6、7、9、10、11、12 位；P1..P4 在 1、2、4、8

def sec_encode(d):                               # d：8 個 bit 的 list（D1..D8）
    m = [0] * 13                                 # m[1..12]，m[0] 不用
    for bit, pos in zip(d, DATA_POS): m[pos] = bit
    for k in range(4):                           # Pk = 位置的二進位第 k 位是 1 的那些 data bit 的 XOR
        m[1 << k] = 0
        for pos in DATA_POS:
            if pos >> k & 1: m[1 << k] ^= m[pos]
    return m[1:]

def syndrome(c):                                 # c：12 個 bit（位置 1..12）
    s = 0
    for k in range(4):
        p = 0
        for pos in range(1, 13):
            if pos >> k & 1: p ^= c[pos - 1]
        s |= p << k
    return s                                     # = 錯誤的位置（0 表示沒錯）

def secded_encode(d): c = sec_encode(d); return c + [sum(c) % 2]   # P5：全部 12 個 bit 的 parity，放第 13 位

def secded_decode(c):
    s = syndrome(c[:12]); p5ok = sum(c) % 2 == 0
    if s == 0 and p5ok: return 'ok'
    if not p5ok: return 'single'                 # 總 parity 錯 → 奇數個錯 → 當成一個錯，可以更正
    return 'double'                              # syndrome ≠ 0 但總 parity 對 → 兩個錯，只能偵測

words = [[(x >> i) & 1 for i in range(8)] for x in range(256)]
codes = {'parity (9,8)': [w + [sum(w) % 2] for w in words],
         'SEC (12,8)': [sec_encode(w) for w in words],
         'SECDED (13,8)': [secded_encode(w) for w in words]}
for name, cw in codes.items():
    dmin = min(sum(a != b for a, b in zip(x, y)) for x, y in combinations(cw, 2))
    print(f'{name:14s}：code word 之間最小的 Hamming distance = {dmin}')

fixed = sum(1 for w in words for e in range(12)
            if (lambda c: (c.__setitem__(e, c[e] ^ 1), syndrome(c) == e + 1)[1])(sec_encode(w)))
print(f'SEC：256 個資料 × 12 個位置的單一錯誤，syndrome 指出正確位置的有 {fixed} / {256 * 12}')
single = double = 0
for w in words:
    c = secded_encode(w)
    for e in range(13):
        x = c[:]; x[e] ^= 1; single += secded_decode(x) == 'single'
    for e1, e2 in combinations(range(13), 2):
        x = c[:]; x[e1] ^= 1; x[e2] ^= 1; double += secded_decode(x) == 'double'
print(f'SECDED：單一錯誤判成 single 的 {single} / {256 * 13}；雙重錯誤判成 double 的 {double} / {256 * 78}')

def crc(msg, div):                               # modulo-2 除法，課本 Example 156 的步驟
    n = len(div) - 1; m = msg + [0] * n
    for i in range(len(msg)):
        if m[i]:
            for j, b in enumerate(div): m[i + j] ^= b
    return m[-n:]
msg, div = [1, 0, 1, 1, 0, 0, 1, 1], [1, 1, 0, 0, 1]
code = crc(msg, div)
print(f'CRC：訊息 10110011、除數 11001 → CRC = {"".join(map(str, code))}（課本 Example 156：0100）')
ok = {L: 0 for L in range(1, 9)}; tot = dict(ok)
for x in range(256):
    w = [(x >> i) & 1 for i in range(8)]; sent = w + crc(w, div)
    for L in range(1, 9):                        # 長度 L 的 burst：第一個和最後一個 bit 一定錯，中間任意
        for start in range(12 - L + 1):
            for mid in range(1 << max(L - 2, 0)):
                e = [0] * 12; e[start] = 1; e[start + L - 1] = 1
                for k in range(L - 2): e[start + 1 + k] = mid >> k & 1
                r = [a ^ b for a, b in zip(sent, e)]
                tot[L] += 1; ok[L] += crc(r[:8], div) != r[8:]
print('CRC 偵測 burst error：', '  '.join(f'長度{L}: {ok[L] / tot[L]:.0%}' for L in range(1, 9)))
