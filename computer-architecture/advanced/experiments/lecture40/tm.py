# tm.py：影片的兩個 STM（課本 §9.7.3 圖 9.61–9.67）用 Python 模擬，交錯由排程器決定
#   Bartok：eager version management（undo log）＋ lazy conflict detection；寫時上鎖、commit 時檢查 read set 的 version
#   TL2：lazy version management（redo log）＋ 全域時鐘；讀時檢查 timestamp、commit 時才上鎖
#   transaction 寫成 generator：每 yield 一次 = 做完一個讀或寫，換排程器決定下一步給誰
import random
class Abort(Exception): pass
class Var:
    def __init__(s, v): s.value, s.version, s.lock = v, 0, None
class Bartok:
    def __init__(s, mem): s.mem = mem
    def begin(s, tid): return dict(tid=tid, rs={}, ws=set(), undo={})
    def read(s, tx, k):
        v = s.mem[k]; tx['rs'].setdefault(k, v.version); return v.value     # 記下 version、加進 read set、讀
    def write(s, tx, k, val):
        v = s.mem[k]
        if v.lock not in (None, tx['tid']): raise Abort                      # 別人鎖著 → abort
        v.lock = tx['tid']
        if k not in tx['undo']: tx['undo'][k] = v.value                      # 第一次寫：舊值進 undo log
        tx['ws'].add(k); v.value = val                                       # 直接寫進記憶體（別人看得到！）
    def commit(s, tx):
        for k, ver in tx['rs'].items():
            v = s.mem[k]
            if v.version != ver or v.lock not in (None, tx['tid']): raise Abort
        for k in tx['ws']: s.mem[k].version += 1; s.mem[k].lock = None
    def abort(s, tx):
        for k, old in tx['undo'].items(): s.mem[k].value = old               # 用 undo log 修回去
        for k in tx['ws']: s.mem[k].lock = None
class BartokPlus(Bartok):
    """多一條規則：abort 時也把寫過的變數的 version 加一（讓讀過髒值的人 commit 時驗證失敗）"""
    def abort(s, tx):
        for k, old in tx['undo'].items(): s.mem[k].value = old
        for k in tx['ws']: s.mem[k].version += 1; s.mem[k].lock = None
class TL2:
    def __init__(s, mem): s.mem, s.clock = mem, 0
    def begin(s, tid): return dict(tid=tid, rv=s.clock, rs=set(), redo={})
    def read(s, tx, k):
        if k in tx['redo']: return tx['redo'][k]
        v = s.mem[k]; v1 = v.version; val = v.value; v2 = v.version        # timestamp 讀兩次（atomic snapshot）
        if v1 != v2 or v1 > tx['rv'] or v.lock is not None: raise Abort      # 開始之後被別人改過 → 馬上 abort
        tx['rs'].add(k); return val
    def write(s, tx, k, val): tx['redo'][k] = val                            # 只寫 redo log
    def commit(s, tx):
        got = []
        for k in tx['redo']:
            if s.mem[k].lock is not None:
                for g in got: s.mem[g].lock = None
                raise Abort
            s.mem[k].lock = tx['tid']; got.append(k)
        s.clock += 1; wv = s.clock
        for k in tx['rs']:
            if s.mem[k].version > tx['rv'] or s.mem[k].lock not in (None, tx['tid']):
                for g in got: s.mem[g].lock = None
                raise Abort
        for k, val in tx['redo'].items(): s.mem[k].value = val               # 先寫回 redo log
        for k in tx['redo']: s.mem[k].version = wv; s.mem[k].lock = None      # 再更新 timestamp、放鎖
    def abort(s, tx):
        for k in tx['redo']:
            if s.mem[k].lock == tx['tid']: s.mem[k].lock = None
def run(stm_cls, mem0, bodies, schedule, max_steps=10000):
    """bodies：每個 thread 一個 generator function(stm, tx)；schedule：thread 編號的序列（用完就隨機）
    回傳 (每個 thread 的結果, 最後的記憶體, abort 次數)"""
    mem = {k: Var(v) for k, v in mem0.items()}; stm = stm_cls(mem)
    n = len(bodies); live, txs, res, aborts = {}, {}, {}, [0] * n
    def start(t): txs[t] = stm.begin(t); live[t] = bodies[t](stm, txs[t])
    for t in range(n): start(t)
    sched, steps = list(schedule), 0
    while live and steps < max_steps:
        steps += 1
        t = sched.pop(0) if sched else random.choice(list(live))
        if t not in live: continue
        try:
            next(live[t])
        except StopIteration as e:
            try: stm.commit(txs[t]); res[t] = e.value; del live[t]
            except Abort: stm.abort(txs[t]); aborts[t] += 1; start(t)
        except Abort:
            stm.abort(txs[t]); aborts[t] += 1; start(t)
    return res, {k: v.value for k, v in mem.items()}, aborts, steps
# ---- 1. 影片 [34:07]、課本圖 9.60：opacity ----
def reader(stm, tx):
    t1 = stm.read(tx, 'x'); yield
    t2 = stm.read(tx, 'y'); yield
    if t1 != t2: return ('會卡在 while (t1 != t2)', t1, t2)
    return ('t1 == t2', t1, t2)
def writer(stm, tx):
    stm.write(tx, 'x', 5); yield
    stm.write(tx, 'y', 5); yield
    return 'commit'
print('== opacity（影片 [35:43]：T1 讀 x，T2 整個做完並 commit，T1 再讀 y）')
for cls in (Bartok, TL2):
    seen = []
    def reader1(stm, tx):
        t1 = stm.read(tx, 'x'); seen.append(['x', t1]); yield
        t2 = stm.read(tx, 'y'); seen[-1] += ['y', t2]; yield
        return (t1, t2)
    r, m, a, _ = run(cls, {'x': 0, 'y': 0}, [reader1, writer], [0, 1, 1, 1, 0, 0, 0, 0])
    print(f'{cls.__name__:7}：T1 第一次執行看到 {seen[0]}{"  ← 不一致：會卡在 while" if len(seen[0]) == 4 and seen[0][1] != seen[0][3] else "（讀 y 時就 abort 了）" if len(seen[0]) == 2 else ""}；abort {a[0]} 次；最後 commit 的結果 {r.get(0)}')
print('== opacity（課本 p.518：T2 寫了 x = 5、還沒寫 y，T1 讀 x 和 y）')
for cls in (Bartok, TL2):
    seen = []
    def reader2(stm, tx):
        t1 = stm.read(tx, 'x'); yield
        t2 = stm.read(tx, 'y'); seen.append((t1, t2)); yield
        return (t1, t2)
    r, m, a, _ = run(cls, {'x': 0, 'y': 0}, [reader2, writer], [1, 0, 0, 1, 1, 1, 0, 0, 0])
    print(f'{cls.__name__:7}：T1 第一次執行讀到 (t1, t2) = {seen[0]}{"  ← 不一致：會卡在 while" if seen[0][0] != seen[0][1] else ""}；最後 commit 的結果 {r.get(0)}')
# ---- 2. 轉帳：隨機交錯 2000 次，總額要守恆、每個 transaction 恰好生效一次 ----
def transfer(a, b, amt):
    def body(stm, tx):
        va = stm.read(tx, a); yield
        vb = stm.read(tx, b); yield
        stm.write(tx, a, va - amt); yield
        stm.write(tx, b, vb + amt); yield
        return 'ok'
    return body
def audit(stm, tx):
    s = 0
    for k in 'ABCD': s += stm.read(tx, k); yield
    return s
print('== 轉帳：4 個帳戶各 100，三筆轉帳 ＋ 一個查總額的 transaction，隨機交錯 2000 次')
for cls in (Bartok, BartokPlus, TL2):
    random.seed(1); bad_total = bad_audit = 0; ab = 0; stuck = 0
    for _ in range(2000):
        r, m, a, steps = run(cls, dict(A=100, B=100, C=100, D=100),
                             [transfer('A', 'B', 10), transfer('B', 'C', 20), transfer('C', 'A', 5), audit], [])
        if len(r) < 4: stuck += 1; continue
        exp = dict(A=95, B=90, C=115, D=100)
        bad_total += m != exp; bad_audit += r[3] != 400; ab += sum(a)
    print(f'{cls.__name__:7}：最後餘額錯 {bad_total} 次；查到的總額不是 400 有 {bad_audit} 次；平均每次 abort {ab / 2000:.2f} 次；跑不完 {stuck} 次')
# ---- 3. 最小的反例：課本字面上的 Bartok 讓 T1 commit 了一個被 rollback 掉的值 ----
print('== 最小反例：T2 第一次寫 A = 999（之後重試寫 2）→ T1 讀 A、把讀到的值寫進 B → T2 撞到 T3 的鎖')
tries = [0]
def t2body(stm, tx):
    tries[0] += 1
    stm.write(tx, 'A', 999 if tries[0] == 1 else 2); yield     # 只有第一次（會 abort 的那次）寫 999
    stm.write(tx, 'L', 1); yield          # L 被 T3 鎖著 → T2 abort
    return 'T2 commit'
def t1body(stm, tx):
    v = stm.read(tx, 'A'); yield
    stm.write(tx, 'B', v); yield
    return v
def t3body(stm, tx):
    stm.write(tx, 'L', 7); yield
    yield
    return 'T3 commit'
for cls in (Bartok, BartokPlus, TL2):
    tries[0] = 0
    r, m, a, _ = run(cls, dict(A=1, B=0, L=0), [t1body, t2body, t3body], [2, 1, 0, 1, 0, 0, 2, 2, 1, 1, 1])
    print(f'{cls.__name__:10}：T2 abort {a[1]} 次；A = {m["A"]}、B = {m["B"]}；B 讀到的值有沒有被 commit 過：{"有" if m["B"] in (1, m["A"]) else "沒有！"}')
