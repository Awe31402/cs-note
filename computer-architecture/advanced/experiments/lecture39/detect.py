# detect.py：影片的兩個 data race 偵測演算法（課本 §9.6.5–9.6.6）跑同一批 trace
#   lockset（Savage et al. 1997，Eraser）：L(v) = L(v) ∩ L(T)；加上 Start/Exclusive/Shared/Modified 狀態機
#   vector clock：課本 Algorithm 1–4（acquire 時 CT、CL 互相 union；release 只清 inLock）
#   trace 的每一步：(thread, 'acq'|'rel'|'rd'|'wr', 鎖或變數)；照 trace 的順序執行（一種 SC 的交錯）
def lockset(trace, basic=False):
    held, L, st, owner, races = {}, {}, {}, {}, set()
    for t, op, a in trace:
        h = held.setdefault(t, set())
        if op == 'acq': h.add(a); continue
        if op == 'rel': h.discard(a); continue
        s = st.get(a, 'Start')
        if basic: s = 'Modified'                         # 基本版：每次存取都算
        elif s == 'Start': s, owner[a] = 'Exclusive', t   # 第一次一定是寫（初始化）
        elif s == 'Exclusive' and t != owner[a]: s = 'Shared' if op == 'rd' else 'Modified'
        elif s == 'Shared' and op == 'wr': s = 'Modified'
        st[a] = s
        if s in ('Shared', 'Modified'):                  # Shared 也縮小 lock set，但不報（影片 [10:44]）
            L[a] = set(h) if a not in L else L[a] & h
            if s == 'Modified' and not L[a]: races.add(a)
    return races
def vclock(trace, n):
    CT = {t: [0] * n for t in range(n)}; inlock = {t: False for t in range(n)}
    CL, Rv, Wv, races = {}, {}, {}, set()
    le = lambda a, b: all(x <= y for x, y in zip(a, b))
    un = lambda a, b: [max(x, y) for x, y in zip(a, b)]
    for t, op, a in trace:
        c = CT[t]
        if op == 'acq':                                  # Algorithm 1
            c[t] += 1; CT[t] = c = un(c, CL.get(a, [0] * n)); CL[a] = list(c); inlock[t] = True
        elif op == 'rel': inlock[t] = False              # Algorithm 2
        else:
            if not inlock[t]: c[t] += 1
            R, Wc = Rv.get(a, [0] * n), Wv.get(a, [0] * n)
            if op == 'rd':                               # Algorithm 3
                if le(Wc, c): Rv[a] = un(R, c)
                else: races.add(a)
            else:                                        # Algorithm 4
                if le(Wc, c) and le(R, c): Rv[a], Wv[a] = un(R, c), un(Wc, c)
                else: races.add(a)
    return races
def truth_hb(trace):
    """課本 Definition 84：兩個衝突的存取（不同 thread、至少一個寫）之間沒有經過 sync 的 happens-before 路徑 → data race
    hb = 同 thread 的程式順序 ∪ 同一把鎖的 rel → 下一個 acq，再取遞移閉包"""
    n = len(trace); hb = [[False] * n for _ in range(n)]
    last_rel = {}
    for j, (t, op, a) in enumerate(trace):
        for i in range(j):
            if trace[i][0] == t: hb[i][j] = True
        if op == 'acq' and a in last_rel: hb[last_rel[a]][j] = True
        if op == 'rel': last_rel[a] = j
    for k in range(n):
        for i in range(n):
            if hb[i][k]:
                for j in range(n):
                    if hb[k][j]: hb[i][j] = True
    races = set()
    for i, (t1, o1, a1) in enumerate(trace):
        for j, (t2, o2, a2) in enumerate(trace):
            if i < j and t1 != t2 and a1 == a2 and o1 in ('rd', 'wr') and o2 in ('rd', 'wr') and 'wr' in (o1, o2) and not hb[i][j]:
                races.add(a1)
    return races
A, B = 'A', 'B'
cases = {   # 名字：(trace, 真的有 data race 的變數)
    '都用同一把鎖保護': ([(0, 'acq', A), (0, 'wr', 'x'), (0, 'rel', A), (1, 'acq', A), (1, 'wr', 'x'), (1, 'rel', A)], set()),
    '不保護的 counter++': ([(0, 'rd', 'c'), (1, 'rd', 'c'), (0, 'wr', 'c'), (1, 'wr', 'c')], {'c'}),
    '用不同的鎖保護': ([(0, 'acq', A), (0, 'wr', 'x'), (0, 'rel', A), (1, 'acq', B), (1, 'wr', 'x'), (1, 'rel', B)], {'x'}),
    '初始化後大家只讀（中間沒有同步）': ([(0, 'wr', 'k'), (1, 'rd', 'k'), (2, 'rd', 'k'), (0, 'rd', 'k')], {'k'}),
    '初始化後大家只讀（建 thread 當同步）': ([(0, 'wr', 'k'), (0, 'acq', 'S'), (0, 'rel', 'S'), (1, 'acq', 'S'), (1, 'rel', 'S'),
                                     (2, 'acq', 'S'), (2, 'rel', 'S'), (1, 'rd', 'k'), (2, 'rd', 'k'), (0, 'rd', 'k')], set()),
    '先不加鎖初始化，之後都加鎖': ([(0, 'wr', 'x'), (0, 'acq', A), (0, 'wr', 'x'), (0, 'rel', A), (1, 'acq', A), (1, 'wr', 'x'), (1, 'rel', A)], set()),
    '用加鎖的旗子交棒，資料本身不加鎖': ([(0, 'wr', 'd'), (0, 'acq', A), (0, 'wr', 'f'), (0, 'rel', A),
                                     (1, 'acq', A), (1, 'rd', 'f'), (1, 'rel', A), (1, 'wr', 'd')], set()),
    '一個加鎖寫、一個不加鎖讀': ([(0, 'wr', 'x'), (0, 'acq', A), (0, 'wr', 'x'), (0, 'rel', A), (1, 'rd', 'x')], {'x'}),
    '先寫的不加鎖，晚很久才有人加鎖讀': ([(0, 'wr', 'x'), (1, 'acq', A), (1, 'rel', A), (1, 'acq', A), (1, 'rd', 'x'), (1, 'rel', A),
                                     (0, 'acq', A), (0, 'wr', 'x'), (0, 'rel', A)], {'x'}),
}
print(f"{'':30}{'真的 race':>10}{'lockset 基本版':>16}{'lockset＋狀態機':>16}{'vector clock':>14}")
fmt = lambda s: ','.join(sorted(s)) or '−'
for nm, (tr, truth) in cases.items():
    n = 1 + max(t for t, _, _ in tr)
    assert truth_hb(tr) == truth, nm                  # 手標的答案要跟定義算出來的一樣
    print(f'{nm:30}{fmt(truth):>10}{fmt(lockset(tr, True)):>16}{fmt(lockset(tr)):>16}{fmt(vclock(tr, n)):>14}')
