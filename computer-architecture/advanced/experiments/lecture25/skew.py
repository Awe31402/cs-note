# skew.py：影片 [14:08] 的 skewed associative cache（cuckoo）和 [27:30] 的 way prediction
#   用法：python3 skew.py work.lackey（valgrind lackey 的 trace，見 ../lecture14/predict.py）
#   cache：32 KB、64 B line、4-way → 128 個 set
#   set-assoc：一般的 4-way，LRU
#   skewed   ：分成 4 個 sub-cache（各 128 格 direct mapped），第 k 個用不同的 hash f_k；
#              4 個位置都滿了就踢掉一個，被踢的去它自己的其他位置（cuckoo search，最多 4 輪）
import sys, random
S, W, LINE = 128, 4, 64
def mix(x, k):                                 # 4 個不同的 hash（乘上不同的奇數再取高位）
    m = (0x9E3779B97F4A7C15, 0xC2B2AE3D27D4EB4F, 0x165667B19E3779F9, 0xD6E8FEB86659FD93)[k]
    return ((x * m) & (2 ** 64 - 1)) >> (64 - 7)  # 7 bit → 0..127

class SetAssoc:
    def __init__(s): s.sets = [[] for _ in range(S)]; s.miss = s.n = 0; s.wp_ok = 0; s.mru = [None] * S
    def access(s, blk):
        s.n += 1; st = s.sets[blk % S]
        if blk in st:
            s.wp_ok += (blk == s.mru[blk % S])        # way prediction：猜「這個 set 上次用到的那個 way（那個 block）」
            s.mru[blk % S] = blk
            st.remove(blk); st.append(blk); return  # LRU：最近用的放後面
        s.miss += 1
        if len(st) == W: st.pop(0)
        st.append(blk); s.mru[blk % S] = blk

class Skewed:
    def __init__(s): s.sub = [[None] * S for _ in range(W)]; s.miss = s.n = 0; s.rng = random.Random(1)
    def access(s, blk):
        s.n += 1
        if any(s.sub[k][mix(blk, k)] == blk for k in range(W)): return
        s.miss += 1
        cur = blk
        for _ in range(4):                     # cuckoo search
            for k in range(W):
                if s.sub[k][mix(cur, k)] is None: s.sub[k][mix(cur, k)] = cur; return
            k = s.rng.randrange(W)
            cur, s.sub[k][mix(cur, k)] = s.sub[k][mix(cur, k)], cur
        # 4 輪都沒找到空位：cur 被趕到下一層

def run(blocks, name):
    a, b = SetAssoc(), Skewed()
    for x in blocks: a.access(x); b.access(x)
    hits = a.n - a.miss
    print(f'{name}：存取 {a.n} 次；set-assoc miss {100 * a.miss / a.n:.2f}%，skewed miss {100 * b.miss / b.n:.2f}%；'
          f'way prediction（猜上次用的 way）在 hit 裡猜對 {100 * a.wp_ok / max(1, hits):.1f}%')

# 1. 故意的 conflict：5 個 block 對到同一個 set（相差 128 條 line），一直循環
run([i * S for i in range(5)] * 2000, '5 個 block 擠同一個 set')
# 2. 真的程式
blocks = []
for line in open(sys.argv[1]):
    if line[:2] in (' L', ' S', ' M'):
        blocks.append(int(line[3:line.index(',')], 16) // LINE)
run(blocks, 'work.c 的 trace')
