# divide.py：課本 Algorithm 3（restoring）和 Algorithm 4（non-restoring），用 U（n+1 位元）、V（n 位元）、D 三個暫存器
import random

def bits(x, w): return format(x & ((1 << w) - 1), f'0{w}b')

def restoring(N, D, n, trace=False):
    U, V, ops = 0, N, 0
    if trace: print(f"  開始           U={bits(U, n + 1)} V={bits(V, n)}")
    for i in range(1, n + 1):
        UV = ((U << n) | V) << 1                         # UV 一起左移一位
        U, V = (UV >> n) & ((1 << (n + 1)) - 1), UV & ((1 << n) - 1)
        if trace: print(f"  {i} 左移後      U={bits(U, n + 1)} V={bits(V >> 1, n - 1)}X")
        U -= D; ops += 1                                 # 試著減
        if U >= 0: q = 1
        else: U += D; ops += 1; q = 0                    # 減成負的：加回去（restore）
        V |= q
        if trace: print(f"    iteration 後 U={bits(U, n + 1)} V={bits(V, n)}")
    return V, U, ops

def nonrestoring(N, D, n, trace=False):
    U, V, ops = 0, N, 0
    if trace: print(f"  開始           U={bits(U, n + 1)} V={bits(V, n)}")
    for i in range(1, n + 1):
        UV = ((U << n) | (V & ((1 << n) - 1))) << 1
        U = UV >> n                                      # 保留 U 的正負號（Python 整數）
        V = UV & ((1 << n) - 1)
        if trace: print(f"  {i} 左移後      U={bits(U, n + 1)} V={bits(V >> 1, n - 1)}X")
        U = U - D if U >= 0 else U + D; ops += 1         # 正的就減、負的就加，不 restore
        V |= 1 if U >= 0 else 0
        if trace: print(f"    iteration 後 U={bits(U, n + 1)} V={bits(V, n)}")
    if U < 0: U += D; ops += 1                           # 最後如果還是負的，加一次 D
    if trace: print(f"  最後修正       U={bits(U, n + 1)}")
    return V, U, ops

print("restoring：7 / 3（4 位元）"); q, r, _ = restoring(7, 3, 4, True); print(f"  商 {q}，餘數 {r}\n")
print("non-restoring：7 / 3（4 位元）"); q, r, _ = nonrestoring(7, 3, 4, True); print(f"  商 {q}，餘數 {r}\n")
print("13 / 3（影片的手算例子）：", restoring(13, 3, 4)[:2])

random.seed(1); bad = 0; tot = {'r': 0, 'n': 0}
for _ in range(100000):
    N, D = random.getrandbits(32), random.getrandbits(random.randint(1, 32)) or 1
    q1, r1, o1 = restoring(N, D, 32); q2, r2, o2 = nonrestoring(N, D, 32)
    bad += (q1, r1) != (N // D, N % D) or (q2, r2) != (N // D, N % D)
    tot['r'] += o1; tot['n'] += o2
print(f"10 萬組隨機的 32 位元除法：錯 {bad} 組；平均每次加減法：restoring {tot['r'] / 1e5:.1f}、non-restoring {tot['n'] / 1e5:.1f}")
