# bpsim.py：第 8 支的 branch predictor，用影片／課本的例子跑跑看，印出猜錯率
import random

class Bimodal1:                     # 每個 entry 1 bit：上次的結果
    def __init__(s, n=10): s.t = {}; s.m = (1 << n) - 1
    def predict(s, pc): return s.t.get(pc & s.m, 0)
    def update(s, pc, out): s.t[pc & s.m] = out

def sat(c, out): return min(c + 1, 3) if out else max(c - 1, 0)

class Sat2:                         # 每個 entry 一個 2-bit saturating counter，從 01 開始
    def __init__(s, n=10): s.t = {}; s.m = (1 << n) - 1
    def predict(s, pc): return s.t.get(pc & s.m, 1) >> 1
    def update(s, pc, out): k = pc & s.m; s.t[k] = sat(s.t.get(k, 1), out)

class TwoLevel:
    """first：'G' 一個 GHR，或 'P' 用 PC 的中間幾位選 GHR
       second：'g' PHT 只用 history 當 index，'p' 再接上 PC 的 n 位，'x' 跟 PC XOR（gshare）"""
    def __init__(s, first, second, k=2, n=10, shift=4, nsel=3):
        s.f, s.sec, s.k, s.n, s.sh, s.nsel = first, second, k, n, shift, nsel
        s.ghr, s.pht = {}, {}
    def _g(s, pc): return 0 if s.f == 'G' else (pc >> s.sh) & ((1 << s.nsel) - 1)
    def _idx(s, pc):
        h = s.ghr.get(s._g(pc), 0); p = pc & ((1 << s.n) - 1)
        return {'g': h, 'p': (p << s.k) | h, 'x': p ^ h}[s.sec]
    def predict(s, pc): return s.pht.get(s._idx(pc), 1) >> 1
    def update(s, pc, out):
        i = s._idx(pc); s.pht[i] = sat(s.pht.get(i, 1), out)
        g = s._g(pc); s.ghr[g] = ((s.ghr.get(g, 0) << 1) | out) & ((1 << s.k) - 1)

class Tournament:                   # selector：00、01 用 p1，10、11 用 p2
    def __init__(s, p1, p2, n=10): s.p1, s.p2, s.sel, s.m = p1, p2, {}, (1 << n) - 1
    def predict(s, pc):
        return (s.p2 if s.sel.get(pc & s.m, 1) >> 1 else s.p1).predict(pc)
    def update(s, pc, out):
        a, b = s.p1.predict(pc), s.p2.predict(pc); k = pc & s.m
        if a != b: s.sel[k] = sat(s.sel.get(k, 1), b == out)
        s.p1.update(pc, out); s.p2.update(pc, out)

def run(pred, trace, watch=None):
    miss = cnt = 0
    for pc, out in trace:
        if watch is None or pc == watch:
            cnt += 1; miss += pred.predict(pc) != out
        pred.update(pc, out)
    return miss / cnt

def loop5(calls=1000, inner_if=False, noise=False):
    """Figure 3.8／影片的 foo()：for (i=0;i<5;i++)，beq .exit 在 PC 0x100；
       inner_if：加上 if (i == 4)，bne .cont 在 0x108（不成立時 taken）；
       noise：每次呼叫前，別的地方（PC 0x5f0、0x6f0）有兩個亂數 branch"""
    t = []
    for _ in range(calls):
        if noise:
            t += [(0x5f0, random.randint(0, 1)), (0x6f0, random.randint(0, 1))]
        for i in range(6):
            t.append((0x100, int(i == 5)))          # beq .exit：第 6 次 taken
            if i < 5 and inner_if:
                t.append((0x108, int(i != 4)))      # bne .cont：i==4 時 not taken
    return t

random.seed(1)
P = lambda: {"1-bit bimodal": Bimodal1(), "2-bit saturating": Sat2(),
             "GAg": TwoLevel('G', 'g'), "GAp": TwoLevel('G', 'p'), "PAg": TwoLevel('P', 'g'),
             "PAp": TwoLevel('P', 'p'), "gshare": TwoLevel('G', 'x'),
             "tournament(2-bit, PAp)": Tournament(Sat2(), TwoLevel('P', 'p'))}
cases = [("Figure 3.8：for 迴圈", loop5(), 0x100),
         ("加上 if (i == 4)", loop5(inner_if=True), 0x100),
         ("加上 if，呼叫前有 2 個亂數 branch", loop5(inner_if=True, noise=True), 0x100)]
alt = []
for i in range(6000):               # 交錯：PC 0x200 T,N,T,N…，中間夾一個別處的亂數 branch
    alt += [(0x200, i & 1), (0x5f0, random.randint(0, 1))]
cases.append(("T,N 交錯，中間夾別處的亂數 branch", alt, 0x200))
print(f"{'':26s}" + "".join(f"{c[0][:14]:>16s}" for c in cases))
for name in P():
    rates = [run(P()[name], tr, w) for _, tr, w in cases]
    print(f"{name:26s}" + "".join(f"{r:16.3f}" for r in rates))
