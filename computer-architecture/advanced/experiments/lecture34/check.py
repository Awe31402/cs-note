# check.py：判斷一個 parallel execution 是不是 SC、是不是 PLSC（影片 [01:12]、[19:08]）
#   SC：找得到一個「照每個 thread 的程式順序」交錯、而且 legal（每個讀都拿到最近一次寫的值）的排法
#   PLSC：每一個位址單獨拿出來（只看對那個位址的存取），都是 SC
#   execution 的寫法：每個 thread 一串 ('W', 位址, 值) 或 ('R', 位址, 讀到的值)；初值都是 0
from functools import lru_cache
def is_sc(threads):
    threads = [tuple(t) for t in threads]
    @lru_cache(None)
    def go(pos, mem):
        if all(p == len(t) for p, t in zip(pos, threads)): return True
        m = dict(mem)
        for i, t in enumerate(threads):
            if pos[i] == len(t): continue
            op, a, v = t[pos[i]]
            if op == 'R' and m.get(a, 0) != v: continue      # 這個讀現在做的話拿不到它讀到的值
            m2 = dict(m)
            if op == 'W': m2[a] = v
            np = list(pos); np[i] += 1
            if go(tuple(np), tuple(sorted(m2.items()))): return True
        return False
    return go(tuple(0 for _ in threads), ())
def is_plsc(threads):
    addrs = {a for t in threads for _, a, _ in t}
    return all(is_sc([[o for o in t if o[1] == a] for t in threads]) for a in addrs)
W = lambda a, v: ('W', a, v); R = lambda a, v: ('R', a, v)
cases = {
    '[15:22] 單一變數，T2、T3 看到一樣的順序': [[R('x', 0), W('x', 1), W('x', 2)], [R('x', 1), R('x', 2)], [R('x', 1), R('x', 2)]],
    '[16:03] 單一變數，T3 看到相反的順序': [[W('x', 1), W('x', 2)], [R('x', 1), R('x', 2)], [R('x', 2), R('x', 1)]],
    '[23:42] SB：兩個 load 都讀到 0': [[W('x', 1), R('y', 0)], [W('y', 1), R('x', 0)]],
    '[32:25] 三個 thread（write 不 atomic）': [[W('x', 1)], [R('x', 1), W('y', 1)], [R('y', 1), R('x', 0)]],
    'MP：t1 = 1、t2 = 0': [[W('x', 1), W('y', 1)], [R('y', 1), R('x', 0)]],
}
for name, ex in cases.items():
    print(f'{name}：SC？{is_sc(ex)}　PLSC？{is_plsc(ex)}')
