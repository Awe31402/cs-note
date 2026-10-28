"""課本 §13.8.4 的 NAND flash：
(1) 規則：erase 之後全部是 1；program 只能把 1 變 0；要把 0 變回 1 只能整個 block erase。
(2) wear leveling：照課本的做法，P/E 次數比最少的那個多出門檻時，就用一個暫存 block 把兩個 block 的內容對調。
    模擬「最愛的歌」：90% 的寫入落在 10% 的 logical block。"""
import random

class Block:
    def __init__(self, pages=4): self.pages = [None] * pages; self.pe = 0
    def erase(self): self.pages = [None] * len(self.pages); self.pe += 1
    def program(self, i, bits):
        old = self.pages[i] or [1] * len(bits)
        if any(o == 0 and b == 1 for o, b in zip(old, bits)):
            raise ValueError('要把 0 變回 1：必須先 erase 整個 block')
        self.pages[i] = [o & b for o, b in zip(old, bits)]

b = Block()
b.program(0, [0, 1, 1, 0]); print('erase 後寫入 0110：', b.pages[0])
b.program(0, [0, 0, 1, 0]); print('再把一個 1 改成 0（0010）：可以 →', b.pages[0])
try: b.program(0, [1, 0, 0, 1])
except ValueError as e: print('想寫成 1001：', e)
b.erase(); b.program(0, [1, 0, 0, 1]); print(f'erase 之後才能寫 1001：{b.pages[0]}，這個 block 的 P/E = {b.pe}')

def simulate(nblocks=100, writes=300000, threshold=None, seed=1):
    rng = random.Random(seed)
    pe = [0] * nblocks
    l2p = list(range(nblocks))                   # logical block → physical block（最後一個 physical 當暫存）
    hot = nblocks // 10
    swaps = 0
    for _ in range(writes):
        lb = rng.randrange(hot) if rng.random() < 0.9 else rng.randrange(hot, nblocks - 1)
        pb = l2p[lb]; pe[pb] += 1                # 改寫這個 logical block = 它的 physical block 一次 P/E
        if threshold is not None:
            cold_lb = min(range(nblocks - 1), key=lambda x: pe[l2p[x]])
            cb = l2p[cold_lb]
            if pe[pb] - pe[cb] > threshold:      # 熱的和冷的對調：tmp ← cold，erase cold，cold ← hot，erase hot，hot ← tmp
                tmp = nblocks - 1
                pe[tmp] += 1; pe[cb] += 1; pe[pb] += 1
                l2p[lb], l2p[cold_lb] = cb, pb
                swaps += 1
    return max(pe[:-1]), min(pe[:-1]), pe[-1], swaps

for th in (None, 100, 20):
    mx, mn, tmp, sw = simulate(threshold=th)
    name = '沒有 wear leveling' if th is None else f'門檻 {th:3d} 的 wear leveling'
    worst = max(mx, tmp)
    print(f'{name:22s}：資料 block 的 P/E 最大 {mx:6d}、最小 {mn:5d}；暫存 block {tmp:6d}（對調 {sw} 次）'
          f' → 最先壞掉的 block 撐到 10 萬次時，總共寫了 {100000 * 300000 // worst / 1e6:.1f} 百萬次')
