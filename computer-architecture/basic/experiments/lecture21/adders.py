"""三種加法器的閘級模型：每個訊號記錄 (值, 到達時間)，每個 AND/OR/XOR 閘延遲 1。
量最長路徑（critical path）的閘延遲，看 ripple O(n)、carry select O(√n)、carry lookahead O(log n)。"""
import math, random

def AND(a, b): return (a[0] & b[0], max(a[1], b[1]) + 1)
def OR(a, b):  return (a[0] | b[0], max(a[1], b[1]) + 1)
def XOR(a, b): return (a[0] ^ b[0], max(a[1], b[1]) + 1)
def MUX(s, x0, x1):                       # s ? x1 : x0，2 層閘
    v = x1[0] if s[0] else x0[0]
    return (v, max(s[1], x0[1], x1[1]) + 2)

def full_adder(a, b, c):
    p = XOR(a, b)
    return XOR(p, c), OR(AND(a, b), AND(p, c))      # sum, carry out

def ripple(A, B, cin):
    s, c = [], cin
    for a, b in zip(A, B):
        x, c = full_adder(a, b, c); s.append(x)
    return s, c

def carry_select(A, B, cin, k):
    s, c = [], cin
    for i in range(0, len(A), k):
        s0, c0 = ripple(A[i:i+k], B[i:i+k], (0, 0))   # 假設 carry in = 0
        s1, c1 = ripple(A[i:i+k], B[i:i+k], (1, 0))   # 假設 carry in = 1（同時算）
        s += [MUX(c, x0, x1) for x0, x1 in zip(s0, s1)]
        c = MUX(c, c0, c1)                             # 選出真正的 carry，傳給下一塊
    return s, c

def lookahead(A, B, cin):
    """parallel-prefix（Kogge-Stone）的 carry lookahead：log n 層的 (G,P) 合併"""
    g = [AND(a, b) for a, b in zip(A, B)]
    p = [XOR(a, b) for a, b in zip(A, B)]
    G, P = [OR(g[0], AND(p[0], cin))] + g[1:], p[:]   # 把 cin 併進第 0 位
    d = 1
    while d < len(A):                                  # (G,P)_hi ∘ (G,P)_lo
        G, P = ([G[i] if i < d else OR(G[i], AND(P[i], G[i - d])) for i in range(len(A))],
                [P[i] if i < d else AND(P[i], P[i - d]) for i in range(len(A))])
        d *= 2
    carries = [cin] + G[:-1]                            # 第 i 位的 carry in
    return [XOR(p[i], carries[i]) for i in range(len(A))], G[-1]

def bits(x, n): return [((x >> i) & 1, 0) for i in range(n)]
def val(s, c): return sum(b[0] << i for i, b in enumerate(s)) | (c[0] << len(s))
def delay(s, c): return max([b[1] for b in s] + [c[1]])

random.seed(1)
print(f"{'n':>4} {'ripple':>7} {'select(k=√n)':>13} {'lookahead':>10}   correct?")
for n in (4, 8, 16, 32, 64, 128):
    ok, worst = True, [0, 0, 0]
    k = max(1, round(math.sqrt(n)))
    tests = [(2**n - 1, 1)] + [(random.getrandbits(n), random.getrandbits(n)) for _ in range(200)]
    for a, b in tests:
        A, B = bits(a, n), bits(b, n)
        for j, (s, c) in enumerate((ripple(A, B, (0, 0)), carry_select(A, B, (0, 0), k), lookahead(A, B, (0, 0)))):
            ok &= val(s, c) == a + b
            worst[j] = max(worst[j], delay(s, c))
    print(f"{n:>4} {worst[0]:>7} {worst[1]:>13} {worst[2]:>10}   {ok}")
