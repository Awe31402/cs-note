# nuca.py：S-NUCA 和 D-NUCA（影片 [03:16]、[07:38]），用 work.c 的 valgrind trace 的 load/store
#   用法：python3 nuca.py work.lackey
#   LLC：4 × 4 = 16 個 bank；core 在左上角 (0,0) 那個 bank 旁邊；到一個 bank 要走「曼哈頓距離 + 1」個 hop
#   前面有一個 32 KB 的 L1（8-way、LRU），只有 L1 miss 才到 LLC
#   每個 bank 很小（32 個 set × 4-way），讓 D-NUCA 的搬移有意義
import sys
from collections import OrderedDict, Counter
N = 4; SETS, W = 32, 4
def dist(b): return b[0] + b[1] + 1
blocks = []
for line in open(sys.argv[1]):
    if line[:2] in (' L', ' S', ' M'): blocks.append(int(line[3:line.index(',')], 16) >> 6)
l1 = [OrderedDict() for _ in range(64)]; llc_stream = []
for blk in blocks:                                    # L1：64 set × 8 way
    st = l1[blk % 64]
    if blk in st: st.move_to_end(blk); continue
    llc_stream.append(blk); st[blk] = 1
    if len(st) > 8: st.popitem(last=False)
print(f'load/store {len(blocks)} 次，L1 miss（送到 LLC）{len(llc_stream)} 次')

def bank_of(blk, low):                                # bank id 取 block address 的低 4 bit，或比較高的 4 bit
    v = blk & 15 if low else (blk >> 9) & 15
    return (v >> 2, v & 3)
for low in (True, False):
    cnt = Counter(bank_of(b, low) for b in llc_stream)
    print(f'S-NUCA，bank id 用{"低" if low else "高（第 9–12）"}位：16 個 bank 的存取數 最多 {max(cnt.values())}、最少 {min(cnt.get((i, j), 0) for i in range(4) for j in range(4))}；'
          f'平均 {sum(dist(bank_of(b, low)) * 1 for b in llc_stream) / len(llc_stream):.2f} hop')

def snuca_sim(low):                                    # 同樣大小的 S-NUCA：每個 bank 一樣 32 set × 4-way、LRU
    banks = {(x, y): [OrderedDict() for _ in range(SETS)] for x in range(N) for y in range(N)}
    hit = hhops = 0
    for blk in llc_stream:
        bk = bank_of(blk, low); st = banks[bk][(blk >> 4) % SETS]
        if blk in st: hit += 1; hhops += dist(bk); st.move_to_end(blk); continue
        st[blk] = 1
        if len(st) > W: st.popitem(last=False)
    return hit, hhops
def dnuca2(near=False):                                          # broadcast 搜尋；回傳 hit 數、hit 的 hop 總和、hit 在 home bank 的次數
    banks = {(x, y): [OrderedDict() for _ in range(SETS)] for x in range(N) for y in range(N)}
    hit = hhops = home = 0
    for blk in llc_stream:
        x = blk & 3; s_ = (blk >> 2) % SETS
        found = next((y for y in range(N) if blk in banks[(x, y)][s_]), None)
        if found is None: dest = (x, 0 if near else N - 1)   # miss：放最近（home）或最遠
        else:
            hit += 1; hhops += dist((x, found)); home += found == 0
            del banks[(x, found)][s_][blk]; dest = (x, max(0, found - 1))
        st = banks[dest][s_]; st[blk] = 1
        if len(st) > W:
            old, _ = st.popitem(last=False)
            if dest[1] + 1 < N:
                st2 = banks[(x, dest[1] + 1)][s_]; st2[old] = 1
                if len(st2) > W: st2.popitem(last=False)
    return hit, hhops, home
n = len(llc_stream)
h, hh = snuca_sim(True)
print(f'S-NUCA（bank id 用低位）：LLC hit {100 * h / n:.1f}%；hit 平均 {hh / h:.2f} hop')
for near in (False, True):
    h, hh, home = dnuca2(near)
    print(f'D-NUCA（broadcast 搜尋、hit 就往 home 升一格；miss 時放{"最近的 home bank" if near else "最遠的 bank"}）：LLC hit {100 * h / n:.1f}%；'
          f'hit 平均 {hh / h:.2f} hop；在 home bank hit 的比例 {100 * home / h:.0f}%')
