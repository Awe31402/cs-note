# comb.py：用 AND／OR／NOT 照影片的電路圖拼出 mux、demux、decoder、encoder，把所有輸入都試一遍
from itertools import product
AND = lambda *x: int(all(x)); OR = lambda *x: int(any(x)); NOT = lambda a: 1 - a

def mux4(x, a, b):        # [40:59]：四種選擇線組合各接一個 AND，再全部 OR
    return OR(AND(NOT(a), NOT(b), x[0]), AND(NOT(a), b, x[1]), AND(a, NOT(b), x[2]), AND(a, b, x[3]))
def demux4(x, a, b):      # [46:28]：同樣四個 AND，但輸入只有一個
    return [AND(NOT(a), NOT(b), x), AND(NOT(a), b, x), AND(a, NOT(b), x), AND(a, b, x)]
def decoder2(a, b):       # [49:49]：demux 拿掉輸入
    return [AND(NOT(a), NOT(b)), AND(NOT(a), b), AND(a, NOT(b)), AND(a, b)]
def encoder8(bits):       # [54:44]：三個 4 輸入的 OR
    b = bits
    return [OR(b[4], b[5], b[6], b[7]), OR(b[2], b[3], b[6], b[7]), OR(b[1], b[3], b[5], b[7])]

ok = all(mux4(x, a, b) == x[2 * a + b] for x in product((0, 1), repeat=4) for a, b in product((0, 1), repeat=2))
print("4 對 1 mux：", "16×4 種輸入全對" if ok else "錯")
ok = all(demux4(x, a, b) == [x if i == 2 * a + b else 0 for i in range(4)] for x in (0, 1) for a, b in product((0, 1), repeat=2))
print("1 對 4 demux：", "全對" if ok else "錯")
print("2 對 4 decoder：", {f"{a}{b}": decoder2(a, b) for a, b in product((0, 1), repeat=2)})
ok = all(encoder8([int(i == k) for i in range(8)]) == [k >> 2 & 1, k >> 1 & 1, k & 1] for k in range(8))
print("8 對 3 encoder（只有一個 1）：", "全對" if ok else "錯")
print("  輸入 b6 = 1 →", encoder8([int(i == 6) for i in range(8)]), "（110 = 6）")
print("  輸入 b3、b5 同時是 1 →", encoder8([int(i in (3, 5)) for i in range(8)]),
      "（111 = 7，兩個都不是：所以要 priority encoder）")
