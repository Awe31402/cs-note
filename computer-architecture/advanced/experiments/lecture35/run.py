# run.py：把影片、課本、常見的 litmus test 丟進 ew.py
from ew import *
def name(th, n):
    if n[0] == 'init': return f'init{n[1]}'
    op, a, v = th[n[0]][n[1]]
    return 'fence' if op == 'F' else f'T{n[0]+1}:{op}{a}{v}'
cases = {   # 名字：(execution, 本機 x86 實測看過沒有)
    '[07:08] x=1;t1=y ‖ y=1;t2=x，t1=0、t2=1': ([[W('x', 1), R('y', 0)], [W('y', 1), R('x', 1)]], None),
    '[18:38] 圖 9.25：x=1 ‖ x=2 ‖ t1=x;fence;t2=x，1 再 2': ([[W('x', 1)], [W('x', 2)], [R('x', 1), F, R('x', 2)]], None),
    '[21:37] 圖 9.26：x=1;x=2 ‖ t1=x，t1=1': ([[W('x', 1), W('x', 2)], [R('x', 1)]], None),
    'SB：x=1;t1=y ‖ y=1;t2=x，0、0': ([[W('x', 1), R('y', 0)], [W('y', 1), R('x', 0)]], '看過（第 33 支）'),
    'SB＋fence': ([[W('x', 1), F, R('y', 0)], [W('y', 1), F, R('x', 0)]], '沒看過（第 33 支，mfence）'),
    'MP：x=1;y=1 ‖ t1=y;t2=x，1、0': ([[W('x', 1), W('y', 1)], [R('y', 1), R('x', 0)]], '沒看過（第 33 支）'),
    'MP＋fence 兩邊': ([[W('x', 1), F, W('y', 1)], [R('y', 1), F, R('x', 0)]], None),
    'LB：t1=x;y=1 ‖ t2=y;x=1，1、1': ([[R('x', 1), W('y', 1)], [R('y', 1), W('x', 1)]], None),
    'WRC（第 34 支 [32:25]）': ([[W('x', 1)], [R('x', 1), W('y', 1)], [R('y', 1), R('x', 0)]], '沒看過（第 34 支）'),
    'WRC＋fence': ([[W('x', 1)], [R('x', 1), F, W('y', 1)], [R('y', 1), F, R('x', 0)]], None),
    'IRIW＋fence': ([[W('x', 1)], [W('y', 1)], [R('x', 1), F, R('y', 0)], [R('y', 1), F, R('x', 0)]], '沒看過（第 34 支，沒 fence）'),
    'MP＋fence（只有 T1 有 fence）': ([[W('x', 1), F, W('y', 1)], [R('y', 1), R('x', 0)]], None),
    'CoRR（同一位址，看到相反的順序）': ([[W('x', 1)], [W('x', 2)], [R('x', 1), R('x', 2)], [R('x', 2), R('x', 1)]], '沒看過（第 34 支）'),
}
print('可行＝找得到沒有環的 execution witness；✗ 後面是一個環')
print(f"{'':44}" + ''.join(f'{m:>9}' for m in MODELS) + '   暴力 SC   PLSC   x86 實測')
for nm, (th, seen) in cases.items():
    row = []
    for m in MODELS:
        ok, _ = witness(th, m); row.append('可行' if ok else '✗')
    sc = '可行' if is_sc_bruteforce(th) else '✗'
    assert (row[0] == '可行') == (sc == '可行')            # 兩種方法對 SC 的判斷要一樣
    p, _ = plsc(th)
    print(f'{nm:44}' + ''.join(f'{r:>8}' for r in row) + f'{sc:>8}   {"是" if p else "否":>4}   {seen or ""}')
print()
for nm in ['MP：x=1;y=1 ‖ t1=y;t2=x，1、0', 'SB：x=1;t1=y ‖ y=1;t2=x，0、0', 'WRC＋fence']:
    th = cases[nm][0]
    for m in ['SC', 'TSO', 'WEAK-NA']:
        ok, c = witness(th, m)
        if not ok: print(f'{nm}，{m} 的環：' + ' → '.join(name(th, n) for n in c))
print()
# [45:01] 影片的 access graph 例子：T1：x=1; t1=x（讀到 2）‖ T2：x=2 ‖ T3：讀到 1 再讀到 0
th = [[W('x', 1), R('x', 2)], [W('x', 2)], [R('x', 1), R('x', 0)]]
print('[45:01] access graph 例子：PLSC？', plsc(th)[0], '；SC？', witness(th, 'SC')[0], '；WEAK-NA？', witness(th, 'WEAK-NA')[0])
th = [[W('x', 1), R('x', 1), W('x', 2), R('x', 2)], [R('x', 2), R('x', 1)]]
print('課本例 11（圖 9.30）：PLSC？', plsc(th)[0])
print()
# 第 35 支 [47:22] 的 thin air read：T1：t1=x; if (t1==1) y=2 ‖ T2：t2=y; fence; x=1，結果 t1=1、t2=2
th = [[R('x', 1), W('y', 2)], [R('y', 2), F, W('x', 1)]]
dep = [((0, 0), (0, 1))]                                  # t1 → if → y=2（控制相依）
print('[47:22] 影片的 thin air：WEAK 的 execution witness 可行？', witness(th, 'WEAK')[0],
      '；causal graph 的環：', ' → '.join(name(th, n) for n in causal(th, 'WEAK', dep)))
print('　　　　　　　　　　　　 TSO 可行？', witness(th, 'TSO')[0], '（R→W 的 po 是 global）')
# 課本圖 9.31：T1：t1=x; if (t1==1) t2=y ‖ T2：y=1; fence; x=1，結果 t1=1、t2=0
th = [[R('x', 1), R('y', 0)], [W('y', 1), F, W('x', 1)]]
dep = [((0, 0), (0, 1))]
c = causal(th, 'WEAK', dep)
print('課本圖 9.31：WEAK 可行？', witness(th, 'WEAK')[0], '；式 9.9（rf ∪ gpo ∪ dep）的環：', ' → '.join(name(th, n) for n in c) if c else '沒有')
c = causal(th, 'WEAK', dep, with_fr=True)
print('　　　　　　 再加上 fr 邊的環：', ' → '.join(name(th, n) for n in c) if c else '沒有')
