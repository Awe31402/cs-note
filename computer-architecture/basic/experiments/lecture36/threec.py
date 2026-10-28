"""課本 §11.3.2：把 miss 分成三個 C，並試 victim cache、square root rule、associativity rule。
compulsory = 第一次碰到這個 block；capacity = 同大小 fully associative LRU 還是 miss 的（扣掉 compulsory）；
conflict = 真正的 cache 比 fully associative 多出來的 miss。全部用 LRU、64-byte block。
用法：python3 -I threec.py trace"""
import sys
from collections import OrderedDict

def blocks(path, limit=2 * 10**6):
    with open(path) as f:
        for n, line in enumerate(f):
            if n == limit: break
            k, rest = line.split(); b = int(rest.split(',')[0], 16) >> 6
            yield b
            if k == 'M': yield b                       # modify = 讀＋寫，同一個 block 兩次

def misses(trace, size, ways, victim=0):
    """回傳 miss 次數。ways=0 表示 fully associative。victim>0 時加一個 victim entries 的 fully associative victim cache"""
    nblk = size // 64; ways = ways or nblk; nsets = nblk // ways
    sets = [OrderedDict() for _ in range(nsets)]; vc = OrderedDict(); m = 0
    for b in trace:
        s = sets[b % nsets]
        if b in s: s.move_to_end(b); continue          # hit：移到 MRU
        if victim and b in vc: del vc[b]               # victim cache hit：搬回主 cache（不算 miss）
        else: m += 1
        if len(s) == ways:
            old, _ = s.popitem(last=False)             # 主 cache 趕出 LRU
            if victim:
                vc[old] = 1
                if len(vc) > victim: vc.popitem(last=False)
        s[b] = 1
    return m

tr = list(blocks(sys.argv[1])); n = len(tr)
comp = len(set(tr))
print(f'{sys.argv[1].rsplit("/", 1)[-1]}: {n} 次存取，{comp} 個不同的 block')
print(' 8 KB cache       | miss rate | compulsory | capacity | conflict')
for ways, name in [(1, 'direct mapped'), (2, '2-way'), (4, '4-way'), (0, 'fully assoc.')]:
    tot = misses(tr, 8192, ways); fa = misses(tr, 8192, 0)
    print(f' {name:15s} | {tot / n:8.2%} | {comp / n:9.2%} | {(fa - comp) / n:7.2%} | {(tot - fa) / n:7.2%}')
dm = misses(tr, 8192, 1)
for v in (4, 8, 16):
    print(f' direct mapped + {v:2d}-entry victim cache: miss rate {misses(tr, 8192, 1, v) / n:.2%}（沒有 victim：{dm / n:.2%}）')
print(' square root rule（4-way）：')
prev = None
for kb in (1, 2, 4, 8, 16, 32, 64, 128, 256):
    r = misses(tr, kb * 1024, 4) / n
    print(f'   {kb:3d} KB: {r:7.3%}' + (f'   ×{r / prev:.2f}（√ 規則預測 ×0.71）' if prev else ''))
    prev = r
for a, b in [((16, 4), (32, 2)), ((64, 4), (128, 2)), ((8, 8), (16, 4))]:
    ra, rb = misses(tr, a[0] * 1024, a[1]) / n, misses(tr, b[0] * 1024, b[1]) / n
    print(f' associativity rule：{a[0]} KB {a[1]}-way {ra:.3%} vs {b[0]} KB {b[1]}-way {rb:.3%}')
