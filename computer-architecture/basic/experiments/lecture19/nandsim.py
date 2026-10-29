# nandsim.py：只用 NAND 閘的電路模擬器。每個閘延遲 1 個時間單位：t+1 的輸出 = NAND(t 時的輸入)
import itertools

class Circuit:
    def __init__(self): self.g, self.v = {}, {}
    def nand(self, out, *ins): self.g[out] = ins; self.v.setdefault(out, 1)
    def step(self, inputs):
        self.v.update(inputs)
        nv = {o: 1 - int(all(self.v.get(i, 0) for i in ins)) for o, ins in self.g.items()}
        self.v.update(nv)
    def run(self, inputs, n):
        for _ in range(n): self.step(inputs)
        return self

def sr_latch(c, sb, rb, q='Q', qb='Qb', p=''):
    """影片的 SR latch：兩個交叉耦合的 NAND，輸入是 S̄、R̄（[01:23]、[27:51]）"""
    c.nand(p + q, sb, p + qb); c.nand(p + qb, rb, p + q)

# ---- 1. SR latch：set、reset、maintain，以及 S = R = 1 ----
c = Circuit(); sr_latch(c, 'Sb', 'Rb')
def show(S, R, n=6):
    c.run({'Sb': 1 - S, 'Rb': 1 - R}, n); return f"Q={c.v['Q']} Q̄={c.v['Qb']}"
print("SR latch：")
print("  S=1 R=0（set）  →", show(1, 0))
print("  S=0 R=0（保持） →", show(0, 0))
print("  S=0 R=1（reset）→", show(0, 1))
print("  S=0 R=0（保持） →", show(0, 0))
print("  S=1 R=1         →", show(1, 1), "← Q 和 Q̄ 都是 1，不是互補")
for path, name in (([(1, 0), (0, 0)], "11 → 10 → 00"), ([(0, 1), (0, 0)], "11 → 01 → 00")):
    show(1, 1); r = [show(s, r_, 4) for s, r_ in path]
    print(f"  {name}（不同時放開）→", r[-1])
show(1, 1); hist = []
for t in range(8):
    c.step({'Sb': 1, 'Rb': 1}); hist.append(f"{c.v['Q']}{c.v['Qb']}")
print("  11 → 00（同時放開），每一步的 QQ̄：", ' '.join(hist), "← 一直振盪，停不下來")

# ---- 2. Master-slave D flip-flop：只在 clock 的負緣更新 ----
def d_latch(c, d, clk, p):
    """D flip-flop（課本 Figure 7.21）：i1 = NAND(D, clk)，i2 = NAND(i1, clk)，接 SR latch"""
    c.nand(p + 'i1', d, clk); c.nand(p + 'i2', p + 'i1', clk); sr_latch(c, p + 'i1', p + 'i2', p=p)
c = Circuit(); c.nand('clkb', 'clk', 'clk')       # 反相器 = NAND(clk, clk)
d_latch(c, 'D', 'clk', 'm'); d_latch(c, 'mQ', 'clkb', 's')
clk_wave = [1, 1, 0, 0] * 4                      # 每個 clock 狀態維持 6 個時間單位
d_wave = {0: 1, 2: 0, 5: 1, 9: 0, 13: 1}          # D 在第幾個半週期改變
print("\nMaster-slave D flip-flop（每格 = 半個 clock 週期，記錄該半週期結束時的值）：")
row = {'clk': '', 'D': '', '主 (master)': '', 'Q (slave)': ''}
d = 0
for k, ck in enumerate(clk_wave):
    d = d_wave.get(k, d)
    c.run({'clk': ck, 'D': d}, 6)
    row['clk'] += f"{ck:>3}"; row['D'] += f"{d:>3}"; row['主 (master)'] += f"{c.v['mQ']:>3}"; row['Q (slave)'] += f"{c.v['sQ']:>3}"
for k, v in row.items(): print(f"  {k:12s}{v}")

# ---- 3. JK flip-flop：J = K = 1 時 toggle → 除頻、計數 ----
def jk(c, j, k, clk, clkb, p):
    c.nand(p + 'i1', j, clk, p + 'Qb'); c.nand(p + 'i2', k, clk, p + 'Q')       # 三輸入 NAND（[39:38]）
    sr_latch(c, p + 'i1', p + 'i2', 'T', 'Tb', p)                              # master
    c.nand(p + 'j1', p + 'T', clkb); c.nand(p + 'j2', p + 'Tb', clkb)           # slave：clocked SR
    sr_latch(c, p + 'j1', p + 'j2', 'Q', 'Qb', p)
def counter(W):
    """三級漣波計數器；clock 每半週期 W 個時間單位，回傳每個負緣之後讀到的值"""
    c = Circuit(); c.nand('clkb', 'clk', 'clk')
    jk(c, 'one', 'one', 'clk', 'clkb', 'a')
    for p, prev in (('b', 'a'), ('c', 'b')):          # 漣波計數器：下一級的 clock 接上一級的 Q
        c.nand(p + 'ck', prev + 'Q', prev + 'Q'); c.nand(p + 'ck2', p + 'ck', p + 'ck')   # 緩衝：ck2 = Q
        c.nand(p + 'ckb', p + 'ck2', p + 'ck2')
        jk(c, 'one', 'one', p + 'ck2', p + 'ckb', p)
    # 起始狀態一定要前後一致（clk = 0、每一級 Q = 0）。如果全部從 1 開始，第二級的 clock 和反相 clock
    # 會同時是 1，master 和 slave 一起打開，掉進跟 SR latch 一樣的 11 ↔ 00 振盪，永遠出不來
    for p in 'abc':
        c.v.update({p + 'Q': 0, p + 'Qb': 1, p + 'T': 0, p + 'Tb': 1, p + 'i1': 1, p + 'i2': 1, p + 'j1': 1, p + 'j2': 0})
    c.v.update({'clk': 0, 'clkb': 1, 'bck': 1, 'bck2': 0, 'bckb': 1, 'cck': 1, 'cck2': 0, 'cckb': 1})
    counts = []
    for n in range(9):
        c.run({'clk': 1, 'one': 1}, W); c.run({'clk': 0, 'one': 1}, W)
        counts.append(f"{c.v['cQ']}{c.v['bQ']}{c.v['aQ']}")
    return counts

print("\nJK flip-flop，J = K = 1，三級串成漣波計數器（每個負緣之後讀一次 cQ bQ aQ）：")
for W in (12, 30):
    counts = counter(W)
    print(f"  半週期 {W:2d} 個時間單位：第 1 級 Q =", ' '.join(x[2] for x in counts),
          "｜計數 =", ' '.join(str(int(x, 2)) for x in counts))
