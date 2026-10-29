# sc.py：把兩個 thread 的指令所有的交錯（interleaving）都列出來，看「照某種順序一條一條執行」能得到哪些結果
#   這就是「能不能把 parallel execution 等價成一個 legal sequential execution」：能列出來的結果就是直覺上合理的
from itertools import combinations
def outcomes(T1, T2):
    res = set()
    n, m = len(T1), len(T2)
    for pos in combinations(range(n + m), n):           # T1 的指令放在哪些位置
        seq, a, b = [], 0, 0
        for k in range(n + m):
            if k in pos: seq.append(T1[a]); a += 1
            else: seq.append(T2[b]); b += 1
        mem, reg = {'x': 0, 'y': 0}, {}
        for op, v, r in seq:                            # ('w', 變數, 值) 或 ('r', 變數, 暫存器)
            if op == 'w': mem[v] = r
            else: reg[r] = mem[v]
        res.add((reg['t1'], reg['t2']))
    return sorted(res)
MP = ([('w', 'x', 1), ('w', 'y', 1)], [('r', 'y', 't1'), ('r', 'x', 't2')])
SB = ([('w', 'x', 1), ('r', 'y', 't1')], [('w', 'y', 1), ('r', 'x', 't2')])
for name, (a, b) in (('MP', MP), ('SB', SB)):
    o = outcomes(a, b)
    allv = [(i, j) for i in (0, 1) for j in (0, 1)]
    print(f'{name}：一條一條交錯能得到的 (t1,t2)：{o}；得不到的：{[v for v in allv if v not in o]}')
