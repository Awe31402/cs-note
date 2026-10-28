"""課本 §12.4.6 的兩個 snoopy 協定，數 bus 上的訊息：
write-update：每次寫都把資料廣播給所有 cache（狀態只有 I／S／M，寫入時大家一起更新）
write-invalidate（MSI）：要寫先讓別人變 I，之後在 M 狀態寫都不用送訊息
每個 cache 只看一個 block（課本的 FSM 也是對單一 block）。
用法：python3 -I msi.py"""
import random

def run(trace, ncache, proto):
    st = ['I'] * ncache; val = [None] * ncache; mem = 0; msgs = 0; last = 0; seen = []  # last：最後一次寫的值
    for p, op, v in trace:                     # (哪個處理器, 'R' 或 'W', 寫的值)
        if op == 'R':
            if st[p] == 'I':                   # read miss：廣播，有人有就給，沒有就從下層拿
                msgs += 1
                owner = next((q for q in range(ncache) if st[q] == 'M'), None)
                if owner is not None:
                    mem = val[owner]; st[owner] = 'S'      # M → S，順便寫回下層
                src = next((q for q in range(ncache) if st[q] != 'I'), None)
                val[p] = val[src] if src is not None else mem; st[p] = 'S'
            seen.append((p, val[p], last))
        else:
            last = v
            if proto == 'update':
                msgs += 1                      # 每次寫都廣播（miss 時順便拿 block）
                for q in range(ncache):
                    if st[q] != 'I': val[q] = v
                val[p] = v; st[p] = 'M'
                for q in range(ncache):
                    if q != p and st[q] == 'M': st[q] = 'S'
            else:
                if st[p] != 'M':               # S 或 I：送 write hit / write miss，別人全部變 I
                    msgs += 1
                    for q in range(ncache):
                        if q != p:
                            if st[q] == 'M': mem = val[q]
                            st[q] = 'I'
                    st[p] = 'M'
                val[p] = v                     # 已經在 M：直接寫，不送訊息
    return msgs, seen

random.seed(1)
patterns = {
    '一個處理器一直寫（1000 次）': [(0, 'W', i) for i in range(1000)],
    '一個寫、一個讀，輪流（各 500 次）': [x for i in range(500) for x in ((0, 'W', i), (1, 'R', 0))],
    '4 個處理器隨機讀寫（各 1000 次）': [(random.randrange(4), random.choice('RRW'), i) for i in range(4000)],
}
for name, tr in patterns.items():
    mu, su = run(tr, 4, 'update'); mi, si = run(tr, 4, 'invalidate')
    print(f'{name:30s} write-update {mu:5d} 則訊息   write-invalidate {mi:5d} 則')

# 一次只做一個操作，所以每次讀都應該讀到「最後一次寫」的值（就像只有一個大 cache）
tr = patterns['4 個處理器隨機讀寫（各 1000 次）']
for proto in ('update', 'invalidate'):
    _, seen = run(tr, 4, proto)
    ok = all(v == want or (v in (None, 0) and want == 0) for p, v, want in seen)
    print(f'{proto:10s}：{len(seen)} 次讀，每次都讀到最後一次寫的值 → {ok}')
