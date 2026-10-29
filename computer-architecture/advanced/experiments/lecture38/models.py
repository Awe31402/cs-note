# models.py：用第 35 支的 ew.py，照課本表 9.4 的八個 memory model 判斷 §9.5 的六個例子
#   表 9.4：✓ = 那個順序被放掉（relaxed）；ws、fr、碰到 fence 的 po 永遠 global
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'lecture35'))
from ew import MODELS, witness, W, R
ALL = {'WR', 'WW', 'RR', 'RW'}
TABLE = {   # 放掉的 po、放掉的 rf
    'SC': (set(), {'rfi'}), 'TSO': ({'WR'}, {'rfi'}), 'PC': ({'WR'}, {'rfe', 'rfi'}),
    'PSO': ({'WR', 'WW'}, {'rfi'}), 'Weak': (ALL, {'rfi'}), 'RC': (ALL, {'rfi'}),
    'PowerPC': (ALL, {'rfe', 'rfi'}), 'ARM': (ALL, {'rfe', 'rfi'}),
}
for m, (po, rf) in TABLE.items():
    MODELS[m] = dict(po=ALL - po, rfe='rfe' not in rf, rfi='rfi' not in rf)
cases = {   # 名字：(execution, 課本說要放掉哪一個才可行, 本機 x86 看過沒有)
    '圖 9.52(a) rfi': ([[W('x', 1), R('x', 1), W('y', 1)], [R('y', 1), R('x', 0)]], 'rfi', '沒看過（本講）'),
    '圖 9.52(b) rfe': ([[W('x', 1)], [R('x', 1), W('y', 1)], [R('y', 1), R('x', 0)]], 'rfe', '沒看過（第 34 支 WRC）'),
    '圖 9.53 W→R（SB）': ([[W('x', 1), R('y', 0)], [W('y', 1), R('x', 0)]], 'poWR', '看過（第 33 支）'),
    '圖 9.54 W→W（MP）': ([[W('x', 1), W('y', 1)], [R('y', 1), R('x', 0)]], 'poWW', '沒看過（第 33 支）'),
    '圖 9.55 R→R': ([[R('x', 1), R('y', 0)], [W('y', 1)], [R('y', 1), W('x', 1)]], 'poRR', '沒看過（本講）'),
    '圖 9.56 R→W（LB）': ([[R('x', 1), W('y', 1)], [R('y', 1), W('x', 1)]], 'poRW', '沒看過（本講）'),
}
print(f"{'':20}" + ''.join(f'{m:>8}' for m in TABLE) + '   課本：要放掉   本機 x86')
for nm, (th, need, seen) in cases.items():
    print(f'{nm:20}' + ''.join(f"{'可行' if witness(th, m)[0] else '✗':>7}" for m in TABLE) + f'   {need:10} {seen}')
