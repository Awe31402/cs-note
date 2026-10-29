# cmos.py：把電晶體當開關，算 CMOS 閘的輸出
#   NMOS：gate = 1 時導通；PMOS：gate = 0 時導通（第 18 支 [17:28]、[21:39]）
#   輸出接到 VCC → 1；接到 GND → 0；兩個都接到 → 短路（X）；都沒接到 → 浮接（Z）
from itertools import product

def solve(trans, inputs):
    """trans: [(種類, gate 的輸入名, 節點1, 節點2)]，回傳 out 的值"""
    on = [(a, b) for kind, g, a, b in trans if (inputs[g] == 1) == (kind == 'n')]
    def reach(start):
        seen, todo = {start}, [start]
        while todo:
            x = todo.pop()
            for a, b in on:
                for p, q in ((a, b), (b, a)):
                    if p == x and q not in seen: seen.add(q); todo.append(q)
        return seen
    r = reach('out')
    return {(True, True): 'X', (True, False): 1, (False, True): 0, (False, False): 'Z'}[('vcc' in r, 'gnd' in r)]

gates = {
    # 反相器：PMOS 在上、NMOS 在下（[22:47]）
    'NOT':  (['a'], [('p', 'a', 'vcc', 'out'), ('n', 'a', 'out', 'gnd')]),
    # NOR：NMOS 並聯、PMOS 串聯（[26:07]）
    'NOR':  (['a', 'b'], [('n', 'a', 'out', 'gnd'), ('n', 'b', 'out', 'gnd'),
                          ('p', 'a', 'vcc', 'm'), ('p', 'b', 'm', 'out')]),
    # NAND：NMOS 串聯、PMOS 並聯（[30:36]）
    'NAND': (['a', 'b'], [('n', 'a', 'out', 'm'), ('n', 'b', 'm', 'gnd'),
                          ('p', 'a', 'vcc', 'out'), ('p', 'b', 'vcc', 'out')]),
    # 故意接錯：NMOS 和 PMOS 都並聯
    '錯的（都並聯）': (['a', 'b'], [('n', 'a', 'out', 'gnd'), ('n', 'b', 'out', 'gnd'),
                                ('p', 'a', 'vcc', 'out'), ('p', 'b', 'vcc', 'out')]),
    # 故意接錯：NMOS 和 PMOS 都串聯
    '錯的（都串聯）': (['a', 'b'], [('n', 'a', 'out', 'm'), ('n', 'b', 'm', 'gnd'),
                                ('p', 'a', 'vcc', 'k'), ('p', 'b', 'k', 'out')]),
}
for name, (ins, t) in gates.items():
    rows = [''.join(map(str, v)) + '→' + str(solve(t, dict(zip(ins, v)))) for v in product((0, 1), repeat=len(ins))]
    print(f"{name:14s}（{len(t)} 個電晶體）: {'  '.join(rows)}")
