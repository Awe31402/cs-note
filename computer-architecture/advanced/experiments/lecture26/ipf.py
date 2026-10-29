# ipf.py：instruction prefetching（影片 [24:00]–[31:25]）：next line 對上 Markov
#   用法：python3 ipf.py work.lackey（valgrind lackey trace，只看 I 開頭的指令位址）
#   I-cache：8 KB、64 B line、4-way、LRU（故意做小，才有夠多的 miss）
#   prefetcher 只看「miss 的序列」（[27:09]）；prefetch 進來的 line 直接放進 I-cache
import sys
from collections import defaultdict, Counter
S, W = 32, 4
def run(lines, mode, k=1):
    sets = [[] for _ in range(S)]; pf = set()          # pf：被 prefetch 進來、還沒被用過的 line
    miss = issued = useful = 0; last = None; markov = defaultdict(Counter)
    def insert(l):
        st = sets[l % S]
        if l in st: return False
        if len(st) == W: pf.discard(st.pop(0))
        st.append(l); return True
    for l in lines:
        st = sets[l % S]
        if l in st:
            st.remove(l); st.append(l)
            if l in pf: useful += 1; pf.discard(l)
            continue
        miss += 1; insert(l)
        targets = []
        if mode == 'next': targets = [l + i for i in range(1, k + 1)]
        if mode == 'markov':
            if last is not None: markov[last][l] += 1             # 記下「miss X 之後，下一個 miss 是 Y」
            if markov[l]: targets = [y for y, _ in markov[l].most_common(k)]
            last = l
        for t in targets:
            if insert(t): issued += 1; pf.add(t)
    return miss, issued, useful

lines, prev = [], None
for line in open(sys.argv[1]):
    if line.startswith('I'):
        l = int(line[3:line.index(',')], 16) // 64
        if l != prev: lines.append(l); prev = l               # 同一條 line 連續的指令只算一次
base, _, _ = run(lines, 'none')
print(f'I-cache 存取（line 層級）{len(lines)} 次；沒有 prefetch：miss {base} 次（{100 * base / len(lines):.2f}%）')
for mode, k in (('next', 1), ('next', 2), ('next', 4), ('markov', 1), ('markov', 2)):
    m, i, u = run(lines, mode, k)
    print(f'  {"next line" if mode == "next" else "Markov"}（一次抓 {k} 條）：miss {m} 次（少了 {100 * (base - m) / base:4.1f}%）；'
          f'發了 {i} 個 prefetch，用到 {u} 個（{100 * u / max(1, i):.0f}%）')
