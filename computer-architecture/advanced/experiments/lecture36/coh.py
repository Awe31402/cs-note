# coh.py：snoopy bus 上的四種 coherence 協定（影片 [03:38]–[54:37]；課本 §9.4.1–9.4.4 圖 9.37–9.44）
#   update（write-update MSI）、MSI（write-invalidate）、MESI、MOESI（含 St／Se 和 Probe）
#   每一步都檢查：① 每個讀都拿到最新寫入的值（coherence）② invalidate 協定：一個 M／E 就沒有別的副本（single writer）
from collections import Counter
class Bus:
    def __init__(s, proto, n):
        s.p, s.n = proto, n
        s.st = [dict() for _ in range(n)]      # 每個 cache：block → 狀態
        s.val = [dict() for _ in range(n)]     # 每個 cache：block → 值
        s.mem, s.latest, s.c = {}, {}, Counter()
    def others(s, i, b, states):
        return [j for j in range(s.n) if j != i and s.st[j].get(b, 'I') in states]
    def supply(s, cands, b):                   # 從一群候選 cache 裡選一個送資料；兩個以上就要 arbitration
        if len(cands) > 1: s.c['arbitration'] += 1
        return s.val[cands[0]][b]
    def lower(s, b):
        s.c['lower-read'] += 1; return s.mem.get(b, 0)
    def msg(s, m): s.c[m] += 1; s.c['bus msgs'] += 1
    def read(s, i, b):
        p, st = s.p, s.st[i].get(b, 'I')
        if st == 'I':
            s.msg('RdX')
            if p == 'update' or p == 'MSI':
                for j in s.others(i, b, 'M'):
                    if p == 'MSI': s.st[j][b] = 'S'; s.mem[b] = s.val[j][b]; s.c['writeback'] += 1
                c = s.others(i, b, 'SM')
                v = s.supply(c, b) if c else s.lower(b); new = 'S'
            elif p == 'MESI':
                for j in s.others(i, b, 'M'): s.mem[b] = s.val[j][b]; s.c['writeback'] += 1
                for j in s.others(i, b, 'EM'): s.st[j][b] = 'S'
                c = s.others(i, b, 'MES')
                if c: v, new = s.supply(c, b), 'S'
                else: v, new = s.lower(b), 'E'
            else:                                                    # MOESI
                own = s.others(i, b, 'OEM')                          # 最多一個 owner
                if own: v = s.val[own[0]][b]; s.st[own[0]][b] = 'O'; new = 'S'
                else:                                                # St 逾時 → Probe → Se
                    s.msg('Probe'); c = s.others(i, b, 'S')
                    if c: v, new = s.supply(c, b), 'S'
                    else: v, new = s.lower(b), 'E'
            s.st[i][b], s.val[i][b] = new, v
        assert s.val[i][b] == s.latest.get(b, 0), (s.p, 'stale read')
        s.check(b)
    def write(s, i, b, v):
        p, st = s.p, s.st[i].get(b, 'I')
        if p == 'update':
            if st == 'I':
                s.msg('WrX'); c = s.others(i, b, 'SM')
                s.val[i][b] = s.supply(c, b) if c else s.lower(b)
            s.msg('Broadcast')                                       # 每個寫都廣播，有副本的都更新
            for j in s.others(i, b, 'SM'): s.val[j][b] = v; s.c['remote update'] += 1
            s.st[i][b] = 'M'
        else:
            if st in ('S', 'O'):
                s.msg('WrX.u')
                for j in s.others(i, b, 'SO'): s.st[j][b] = 'I'
            elif st == 'I':
                s.msg('WrX')
                dirty = s.others(i, b, 'MO' if p == 'MOESI' else 'M')
                c = dirty or (s.others(i, b, 'E') if p != 'MSI' else []) or (s.others(i, b, 'S') if p != 'MOESI' else [])
                s.val[i][b] = s.supply(c, b) if c else s.lower(b)   # MOESI：S 不回應 WrX，沒有 owner 就讀下層
                for j in s.others(i, b, 'MOES'): s.st[j][b] = 'I'
            s.st[i][b] = 'M'                                         # E → M 不用送任何訊息
        s.val[i][b] = v; s.latest[b] = v
        s.check(b)
    def evict(s, i, b):
        st = s.st[i].get(b, 'I')
        if st in ('M', 'O'): s.mem[b] = s.val[i][b]; s.c['writeback'] += 1
        s.st[i][b] = 'I'
    def check(s, b):
        if s.p == 'update': return
        sts = [s.st[j].get(b, 'I') for j in range(s.n)]
        if any(x in 'ME' for x in sts): assert sum(x != 'I' for x in sts) == 1, (s.p, sts)
        assert sts.count('O') <= 1
def run(proto, trace, n=4):
    bus, v = Bus(proto, n), 0
    for op, i, b in trace:
        if op == 'R': bus.read(i, b)
        elif op == 'W': v += 1; bus.write(i, b, v)
        else: bus.evict(i, b)
    return bus.c
N = 100
patterns = {
    '私有：每個 core 先讀再寫自己的 block': [(o, i, f'p{i}') for _ in range(N) for i in range(4) for o in 'RW'],
    '生產者-消費者：core0 寫，core1–3 讀': [x for _ in range(N) for x in [('W', 0, 'x')] + [('R', j, 'x') for j in (1, 2, 3)]],
    '遷移：輪流「讀再寫」同一個 block': [(o, i, 'x') for _ in range(N) for i in range(4) for o in 'RW'],
    '大家只讀': [('R', i, 'x') for _ in range(N) for i in range(4)],
    'core0 連寫 100 次（別人先讀過一次）': [('R', j, 'x') for j in range(4)] + [('W', 0, 'x')] * N,
    '共用讀＋owner 被踢掉後再有人讀': [('W', 0, 'x'), ('R', 1, 'x'), ('R', 2, 'x'), ('E', 0, 'x'), ('R', 3, 'x')] * 20 + [('E', j, 'x') for j in range(4)],
}
cols = ['bus msgs', 'Broadcast', 'remote update', 'WrX.u', 'Probe', 'arbitration', 'writeback', 'lower-read']
for name, tr in patterns.items():
    print(f'== {name}（{len(tr)} 個操作）')
    print(f"{'':8}" + ''.join(f'{c:>14}' for c in cols))
    for p in ['update', 'MSI', 'MESI', 'MOESI']:
        c = run(p, tr)
        print(f'{p:8}' + ''.join(f'{c[k]:>14}' for k in cols))
