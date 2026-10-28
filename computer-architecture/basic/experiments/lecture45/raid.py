"""課本 §13.8.2 的 RAID：RAID 4／5 的 parity（XOR）、壞一顆時重建、小寫入只要改 2 個 block；
RAID 6 的第二個 parity 用 Linux lib/raid6 的 GF(2^8) 算法（多項式 0x11d、產生元 2），壞兩顆也能重建。"""
import os, random
from functools import reduce

def xor(*blocks): return bytes(reduce(lambda a, b: a ^ b, col) for col in zip(*blocks))

def gfmul(a, b):                                 # 和 linux-src/lib/raid6/mktables.c 的 gfmul 一樣
    v = 0
    while b:
        if b & 1: v ^= a
        a = ((a << 1) ^ (0x1d if a & 0x80 else 0)) & 0xff
        b >>= 1
    return v
def gfpow(a, n): r = 1; [r := gfmul(r, a) for _ in range(n)]; return r
def gfinv(a): return next(x for x in range(1, 256) if gfmul(a, x) == 1)
def mulblk(c, blk): return bytes(gfmul(c, x) for x in blk)

def pq(data):                                    # P = D0 ⊕ D1 ⊕ …；Q = Σ 2^i · Di（GF(2^8)）
    P = xor(*data)
    Q = xor(*[mulblk(gfpow(2, i), d) for i, d in enumerate(data)])
    return P, Q

random.seed(1)
B = 512
data = [os.urandom(B) for _ in range(4)]         # 4 顆資料碟的同一列 block：B1 B2 B3 B4
P = xor(*data)
print('RAID 4／5：P = B1 ⊕ B2 ⊕ B3 ⊕ B4')
for lost in range(4):
    rebuilt = xor(P, *[d for i, d in enumerate(data) if i != lost])
    print(f'  第 {lost + 1} 顆壞了 → 用其他 3 顆和 P 重建：{"一樣" if rebuilt == data[lost] else "不一樣"}')
new = os.urandom(B)                              # 寫 B1：P' = P ⊕ B1 ⊕ B1'（不用讀 B2–B4）
P_small = xor(P, data[0], new)
P_full = xor(new, *data[1:])
print(f'  小寫入：P ⊕ B1 ⊕ B1\' {"==" if P_small == P_full else "!="} 重算 B1\' ⊕ B2 ⊕ B3 ⊕ B4（讀 2 個 block、寫 2 個 block）')

print('RAID 5 的 parity 位置（5 顆碟，課本 Fig 13.36 的輪流方式）：')
for row in range(5):
    pdisk = (4 + row) % 5                        # 第一列在第 5 顆，之後依序第 1、2、3、4 顆
    print('  ' + '  '.join('P' if d == pdisk else 'B' for d in range(5)))

D = [os.urandom(B) for _ in range(4)]
P6, Q6 = pq(D)
ok = 0
for a in range(4):
    for b in range(a + 1, 4):                    # 兩顆資料碟同時壞
        Px = xor(P6, *[D[i] for i in range(4) if i not in (a, b)])          # = Da ⊕ Db
        Qx = xor(Q6, *[mulblk(gfpow(2, i), D[i]) for i in range(4) if i not in (a, b)])  # = g^a Da ⊕ g^b Db
        ga, gb = gfpow(2, a), gfpow(2, b)
        Db = mulblk(gfinv(ga ^ gb), xor(mulblk(ga, Px), Qx))                # 解兩個方程式
        Da = xor(Px, Db)
        ok += Da == D[a] and Db == D[b]
print(f'RAID 6：4 顆資料碟任選 2 顆同時壞（{4 * 3 // 2} 種），用 P 和 Q 重建成功 {ok} 種')

def parallel(disks, layout, reqs=4, trials=20000):
    """隨機讀 reqs 個不同的 block，平均可以同時有幾顆碟在工作（同一顆碟一次只能讀一個）"""
    tot = 0
    for _ in range(trials):
        blocks = random.sample(range(10000), reqs)
        tot += len({layout(b, disks) for b in blocks})
    return tot / trials
raid0 = lambda b, n: b % n                       # 依序輪流放在每顆碟
raid4 = lambda b, n: b % (n - 1)                 # 最後一顆專門放 parity，讀的時候用不到
raid5 = lambda b, n: (b % (n - 1) + (1 if b % (n - 1) >= (n - 1 + b // (n - 1)) % n else 0)) % n
print('隨機讀 4 個 block 時，平均同時有幾顆碟在讀（5 顆碟）：'
      f'RAID 0 {parallel(5, raid0):.2f}、RAID 4 {parallel(5, raid4):.2f}、RAID 5 {parallel(5, raid5):.2f}')
