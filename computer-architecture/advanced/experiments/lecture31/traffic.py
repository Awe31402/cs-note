# traffic.py：影片 [27:58] 的 synthetic traffic，在 8 × 8 mesh 上用 XY routing
#   每個節點送一個 flow 到它的目的地；算平均 hop 數，和「最忙的一條 link 上有幾個 flow」
#   最忙的 link 決定吞吐量上限：每個節點每 cycle 最多送 1 / (最忙的 link 的 flow 數) 個 flit
import random
from collections import Counter
k = 8; b = 3                                          # 8 = 2^3，每個座標 3 bit
pat = {
    '隨機（uniform random）': None,
    'bit complement': lambda x, y: ((k - 1) ^ x, (k - 1) ^ y),
    'transpose': lambda x, y: (y, x),
    'tornado': lambda x, y: ((x + k // 2 - 1) % k, y),
}
def bitrev(x, y):                                     # s = x 接 y（x 在高位），整串 6 bit 反過來
    s = (x << b) | y; r = int(format(s, f'0{2 * b}b')[::-1], 2)
    return r >> b, r & (k - 1)
def shuffle(x, y):                                    # 整串左旋一位
    s = (x << b) | y; s = ((s << 1) | (s >> (2 * b - 1))) & ((1 << 2 * b) - 1)
    return s >> b, s & (k - 1)
def rotation(x, y):                                   # 整串右旋一位
    s = (x << b) | y; s = ((s >> 1) | ((s & 1) << (2 * b - 1)))
    return s >> b, s & (k - 1)
pat['bit reverse'] = bitrev; pat['shuffle'] = shuffle; pat['bit rotation'] = rotation
def route(s, d):                                      # XY routing 經過的 link
    (x, y), links = s, []
    while x != d[0]: nx = x + (1 if d[0] > x else -1); links.append(((x, y), (nx, y))); x = nx
    while y != d[1]: ny = y + (1 if d[1] > y else -1); links.append(((x, y), (x, ny))); y = ny
    return links
r = random.Random(1)
print(f'{"pattern":24}{"平均 hop":>9}{"最忙 link 的 flow 數（隨機的取 50 次平均）":>18}{"吞吐量上限（flit/節點/cycle）":>24}')
for name, f in pat.items():
    load, hops, trials = Counter(), 0, (50 if f is None else 1)
    for _ in range(trials):
        for x in range(k):
            for y in range(k):
                d = (r.randrange(k), r.randrange(k)) if f is None else f(x, y)
                L = route((x, y), d); hops += len(L)
                for l in L: load[l] += 1
    mx = max(load.values()) / trials
    print(f'{name:24}{hops / (k * k * trials):9.2f}{mx:18.1f}{1 / mx:24.3f}')
