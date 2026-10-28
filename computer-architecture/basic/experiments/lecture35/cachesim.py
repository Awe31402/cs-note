"""課本 §11.2 的 cache：lookup → data read/write；miss 時 insert，滿了就 replace + evict。
可調大小、block、associativity（1 = direct mapped，sets = 1 就是 fully associative）、
替換策略（random、fifo、lru、plru = 影片／課本的計數器 pseudo-LRU）、寫入策略（wb、wt）。
用法：python3 -I cachesim.py trace [size=8192] [block=64] [ways=4] [policy=lru] [write=wb]
trace 是 valgrind lackey 的格式（' L addr,size'、' S addr,size'、' M addr,size'）"""
import sys, random

class Cache:
    def __init__(self, size=8192, block=64, ways=4, policy='lru', write='wb', bits=3, decay=1000):
        self.block, self.ways, self.policy, self.write = block, ways, policy, write
        self.sets = size // (block * ways)
        self.off_bits = block.bit_length() - 1
        self.idx_bits = self.sets.bit_length() - 1
        self.tag_bits = 32 - self.idx_bits - self.off_bits          # 課本 Example 148
        # 每個 set 是 ways 個 entry：[valid, modified, tag, counter]
        self.tagarr = [[[0, 0, 0, 0] for _ in range(ways)] for _ in range(self.sets)]
        self.max = (1 << bits) - 1; self.decay = decay; self.t = 0
        self.hits = self.misses = self.lower_writes = self.lower_reads = 0
        self.rng = random.Random(1)

    def split(self, addr):
        blk = addr >> self.off_bits
        return blk & (self.sets - 1), blk >> self.idx_bits           # set index, tag

    def access(self, addr, is_write):
        self.t += 1
        if self.policy == 'plru' and self.t % self.decay == 0:      # 定期把所有計數器減 1
            for s in self.tagarr:
                for e in s: e[3] = max(0, e[3] - 1)
        i, tag = self.split(addr); s = self.tagarr[i]
        for e in s:                                                  # lookup
            if e[0] and e[2] == tag:
                self.hits += 1; self.touch(e, s)
                if is_write:
                    if self.write == 'wb': e[1] = 1                  # write-back：只設 modified bit
                    else: self.lower_writes += 1                     # write-through：每次都寫下層
                return True
        self.misses += 1; self.lower_reads += 1                      # miss：從下層讀整個 block
        if is_write and self.write == 'wt': self.lower_writes += 1
        e = next((e for e in s if not e[0]), None)                   # insert：先找 invalid entry
        if e is None:
            e = self.replace(s)                                      # replace：選犧牲者
            if e[1]: self.lower_writes += 1                          # evict：modified 就寫回
        e[:] = [1, int(is_write and self.write == 'wb'), tag, 0]
        self.insert(e, s)
        return False

    def touch(self, e, s):
        if self.policy == 'lru': e[3] = self.t
        elif self.policy == 'plru': e[3] = min(self.max, e[3] + 1)

    def insert(self, e, s):
        if self.policy == 'fifo':
            for o in s:
                if o is not e: o[3] += 1                             # 其他人的計數器 +1，新的是 0
        elif self.policy == 'lru': e[3] = self.t
        elif self.policy == 'plru': e[3] = self.max                  # 新進來的給最大值

    def replace(self, s):
        if self.policy == 'random': return self.rng.choice(s)
        if self.policy == 'fifo': return max(s, key=lambda e: e[3])  # 最早進來的
        return min(s, key=lambda e: e[3])                            # lru：最久沒用；plru：計數器最小

def trace(path, limit=2 * 10**6):
    with open(path) as f:
        for n, line in enumerate(f):
            if n == limit: break
            k, rest = line.split(); a = int(rest.split(',')[0], 16) & 0xFFFFFFFF
            if k == 'M': yield a, False; yield a, True               # modify = 讀再寫
            else: yield a, k == 'S'

if __name__ == '__main__':
    kw = dict(a.split('=') for a in sys.argv[2:])
    c = Cache(**{k: (v if k in ('policy', 'write') else int(v)) for k, v in kw.items()})
    for a, w in trace(sys.argv[1]): c.access(a, w)
    n = c.hits + c.misses
    print(f'sets={c.sets} tag={c.tag_bits} index={c.idx_bits} offset={c.off_bits} | '
          f'miss rate {c.misses / n:.2%} | lower reads {c.lower_reads} writes {c.lower_writes}')
