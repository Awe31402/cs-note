"""課本 Algorithm 1（iterative）與 Booth multiplier 的位元級模型。
U 有 n+1 位元、V 有 n 位元；UV 合起來做算術右移。回傳 (乘積, 加減法次數, 每步的記錄)。"""
import random

def sx(v, bits): v &= (1 << bits) - 1; return v - (1 << bits) if v >> (bits - 1) else v

def run(M, N, n, booth):
    U, V, ops, prev, trace = 0, M & ((1 << n) - 1), 0, 0, []
    for i in range(n):
        cur = V & 1
        if booth:
            if (cur, prev) == (1, 0): U -= N; ops += 1        # 一串 1 開始：減
            elif (cur, prev) == (0, 1): U += N; ops += 1      # 一串 1 結束：加
            prev = cur
        elif cur:
            if i < n - 1: U += N                              # 一般的位元：加
            else: U -= N                                       # 最後一位（MSB = 1）：減
            ops += 1
        U = sx(U, n + 1)
        before = f"{U & ((1 << (n + 1)) - 1):0{n+1}b} {V:0{n}b}"
        uv = (U << n) | V                                      # 算術右移 UV
        uv = sx(uv, 2 * n + 1) >> 1
        U, V = sx(uv >> n, n + 1), uv & ((1 << n) - 1)
        trace.append(f"  before shift: {before}   after shift: {U & ((1 << (n + 1)) - 1):0{n+1}b} {V:0{n}b}")
    return sx((U << n) | V, 2 * n), ops, trace

for name, M, N in (("Example 124: 2 x 3 (N=2, M=3)", 3, 2), ("Example 125: 3 x -2 (N=3, M=-2)", -2, 3)):
    p, ops, tr = run(M, N, 4, False)
    print(f"{name} iterative -> {p}, {ops} add/sub"); print("\n".join(tr))
    p, ops, tr = run(M, N, 4, True)
    print(f"{name} Booth     -> {p}, {ops} add/sub"); print("\n".join(tr))

random.seed(0)
bad, it_ops, bo_ops = 0, 0, 0
for _ in range(20000):
    M, N = random.randint(-2**31, 2**31 - 1), random.randint(-2**31, 2**31 - 1)
    for booth in (False, True):
        p, ops, _ = run(M, N, 32, booth)
        bad += p != M * N
        if booth: bo_ops += ops
        else: it_ops += ops
for M, N in ((-2**31, -2**31), (-2**31, 2**31 - 1), (-1, -1)):
    for booth in (False, True): bad += run(M, N, 32, booth)[0] != M * N
print(f"\n32-bit random: wrong = {bad}; average add/sub per multiply: iterative {it_ops/20000:.1f}, Booth {bo_ops/20000:.1f}")
for M in (0x0000FFFF, 0x7FFFFFF0, 0x55555555, -1):
    print(f"multiplier 0x{M & 0xFFFFFFFF:08X}: iterative {run(M, 7, 32, False)[1]:2} ops, Booth {run(M, 7, 32, True)[1]:2} ops")
