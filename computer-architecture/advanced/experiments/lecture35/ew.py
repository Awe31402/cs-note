# ew.py：execution witness、access graph、causal graph（影片 [07:08]、[39:16]、[55:06]；課本 §9.3.6–9.3.9）
#   節點：每個讀、寫、fence；另外每個位址有一個「初值寫入」節點 init
#   邊：po（同 thread，依 memory model 留下 global 的那幾種）、rf（寫 → 讀到它的讀）、
#       ws（同位址的寫的順序：每一種排法都試）、fr（讀 → 比它讀到的那個寫更晚的寫）
#   ghb = gpo ∪ grf ∪ ws ∪ fr 沒有環 → 這個 execution 在該 memory model 下可行（feasible）
from itertools import permutations, product
from functools import lru_cache
W = lambda a, v: ('W', a, v); R = lambda a, v: ('R', a, v); F = ('F', None, None)
# 每個 model：哪幾種 po 是 global、rfe／rfi 是不是 global
MODELS = {
    'SC':   dict(po={'RR', 'RW', 'WR', 'WW'}, rfe=True,  rfi=True),
    'TSO':  dict(po={'RR', 'RW', 'WW'},       rfe=True,  rfi=False),   # x86：store→load 可以重排、有 store buffer forwarding
    'PSO':  dict(po={'RR', 'RW'},             rfe=True,  rfi=False),   # 再放掉 store→store
    'WEAK': dict(po=set(),                    rfe=True,  rfi=False),   # 只有 fence 的 po 是 global；write 還是 atomic
    'WEAK-NA': dict(po=set(),                 rfe=False, rfi=False),   # 再加上 write 不 atomic（IBM、ARM 那一類）
}
def nodes_of(th):
    return [(t, i) for t, ops in enumerate(th) for i in range(len(ops))]
def find_cycle(nodes, edges):
    adj = {n: [] for n in nodes}
    for a, b, k in edges: adj[a].append((b, k))
    color, stack = {n: 0 for n in nodes}, []
    def dfs(u):
        color[u] = 1; stack.append(u)
        for v, k in adj[u]:
            if color[v] == 1: return stack[stack.index(v):] + [v]
            if color[v] == 0:
                c = dfs(v)
                if c: return c
        color[u] = 2; stack.pop(); return None
    for n in nodes:
        if color[n] == 0:
            c = dfs(n)
            if c: return c
    return None
def base_edges(th, ws_order):
    """rf、ws、fr 邊（不分 model）；ws_order[a] = 位址 a 的寫的順序（init 在最前）"""
    E = []
    wr = {}                                              # (位址, 值) → 寫的節點
    for t, ops in enumerate(th):
        for i, (op, a, v) in enumerate(ops):
            if op == 'W': wr[(a, v)] = (t, i)
    for a, seq in ws_order.items():
        for x, y in zip(seq, seq[1:]): E.append((x, y, 'ws'))
    for t, ops in enumerate(th):
        for i, (op, a, v) in enumerate(ops):
            if op != 'R': continue
            src = wr.get((a, v), ('init', a))
            kind = 'rfi' if src[0] == t else 'rfe'
            E.append((src, (t, i), kind))
            seq = ws_order[a]
            for later in seq[seq.index(src) + 1:]: E.append(((t, i), later, 'fr'))
    return E
def po_edges(th, keep):
    """keep：哪幾種 po 是 global；碰到 fence 的 po 永遠是 global"""
    E = []
    for t, ops in enumerate(th):
        for i in range(len(ops)):
            for j in range(i + 1, len(ops)):
                a, b = ops[i][0], ops[j][0]
                if a == 'F' or b == 'F' or (a + b) in keep: E.append(((t, i), (t, j), 'po'))
    return E
def fence_rf(th, E):
    """write 不 atomic 時 rfe 不是 global，但第 34 支 [64:41] 的 fence 保證：fence 前的寫都傳到所有 core 了，
    fence 後的寫才開始 → 有人讀到 fence 後的寫，那個 fence 一定已經完成 → 加 fence → 讀 的 global 邊"""
    out = []
    for a, b, k in E:
        if k != 'rfe' or a[0] == 'init': continue
        t, i = a
        out += [((t, j), b, 'fence-rf') for j in range(i) if th[t][j][0] == 'F']
    return out
def ws_choices(th):
    addrs = sorted({a for ops in th for op, a, _ in ops if a})
    per = {}
    for a in addrs:
        ws = [(t, i) for t, ops in enumerate(th) for i, (op, b, _) in enumerate(ops) if op == 'W' and b == a]
        per[a] = [[('init', a)] + list(p) for p in permutations(ws)]
    for combo in product(*per.values()):
        yield dict(zip(addrs, combo))
def witness(th, model):
    """傳回 (可行嗎, 一個環)：試每一種 ws 順序，找得到一個沒有環的 ghb 就可行"""
    m = MODELS[model]
    nodes = nodes_of(th) + [('init', a) for a in {a for ops in th for _, a, _ in ops if a}]
    cyc = None
    for ws in ws_choices(th):
        E = [e for e in base_edges(th, ws) if e[2] in ('ws', 'fr') or
             (e[2] == 'rfe' and m['rfe']) or (e[2] == 'rfi' and m['rfi'])]
        E += po_edges(th, m['po'])
        if not m['rfe']: E += fence_rf(th, base_edges(th, ws))
        c = find_cycle(nodes, E)
        if c is None: return True, None
        cyc = cyc or c
    return False, cyc
def plsc(th):
    """access graph：每個位址各一張，邊 = up（同 thread、同位址）∪ rf ∪ ws ∪ fr"""
    for a in sorted({a for ops in th for _, a, _ in ops if a}):
        sub = [[o if o[1] == a else None for o in ops] for ops in th]   # 留著位置，方便印出節點
        sub = [[o for o in ops if o] for ops in sub]
        nodes = nodes_of(sub) + [('init', a)]
        up = [((t, i), (t, j), 'up') for t, ops in enumerate(sub) for i in range(len(ops)) for j in range(i + 1, len(ops))]
        if all(find_cycle(nodes, base_edges(sub, ws) + up) for ws in ws_choices(sub)):
            return False, a
    return True, None
def causal(th, model, dep, with_fr=False):
    """causal graph：rf ∪ gpo ∪ dep（課本式 9.9）；dep = [((t,i),(t,j)), ...]（讀 → 用到它的值的指令）
    with_fr=True：再加 fr 邊（課本圖 9.32 畫的環用到了 fr）"""
    m = MODELS[model]
    nodes = nodes_of(th) + [('init', a) for a in {a for ops in th for _, a, _ in ops if a}]
    ws0 = next(ws_choices(th))
    E = [e for e in base_edges(th, ws0) if e[2] in ('rfe', 'rfi') or (with_fr and e[2] == 'fr')] + po_edges(th, m['po'])
    E += [(a, b, 'dep') for a, b in dep]
    return find_cycle(nodes, E)
def is_sc_bruteforce(th):
    """跟第 34 支 check.py 一樣的方法：直接找合法的交錯（fence 不影響 SC）"""
    th = [tuple(o for o in ops if o[0] != 'F') for ops in th]
    @lru_cache(None)
    def go(pos, mem):
        if all(p == len(t) for p, t in zip(pos, th)): return True
        m = dict(mem)
        for i, t in enumerate(th):
            if pos[i] == len(t): continue
            op, a, v = t[pos[i]]
            if op == 'R' and m.get(a, 0) != v: continue
            m2 = dict(m)
            if op == 'W': m2[a] = v
            np = list(pos); np[i] += 1
            if go(tuple(np), tuple(sorted(m2.items()))): return True
        return False
    return go(tuple(0 for _ in th), ())
