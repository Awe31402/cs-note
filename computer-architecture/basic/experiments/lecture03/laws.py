# laws.py：把第 3 支講到的布林代數規則，用真值表全部驗一遍
from itertools import product
NOT = lambda a: 1 - a
laws = {
    "identity      A+0=A, A·1=A":          lambda a, b, c: (a | 0) == a and (a & 1) == a,
    "annulment     A+1=1, A·0=0":          lambda a, b, c: (a | 1) == 1 and (a & 0) == 0,
    "idempotence   A+A=A, A·A=A":          lambda a, b, c: (a | a) == a and (a & a) == a,
    "complement    A+A'=1, A·A'=0":        lambda a, b, c: (a | NOT(a)) == 1 and (a & NOT(a)) == 0,
    "double neg.   A''=A":                 lambda a, b, c: NOT(NOT(a)) == a,
    "commutative":                         lambda a, b, c: (a | b) == (b | a) and (a & b) == (b & a),
    "associative":                         lambda a, b, c: (a | (b | c)) == ((a | b) | c) and (a & (b & c)) == ((a & b) & c),
    "distributive  A·(B+C)=A·B+A·C":       lambda a, b, c: (a & (b | c)) == ((a & b) | (a & c)),
    "distributive  A+B·C=(A+B)·(A+C)":     lambda a, b, c: (a | (b & c)) == ((a | b) & (a | c)),
    "De Morgan     (A+B)'=A'·B'":          lambda a, b, c: NOT(a | b) == (NOT(a) & NOT(b)),
    "De Morgan     (A·B)'=A'+B'":          lambda a, b, c: NOT(a & b) == (NOT(a) | NOT(b)),
    "consensus     XY+X'Z+YZ=XY+X'Z":      lambda x, y, z: ((x & y) | (NOT(x) & z) | (y & z)) == ((x & y) | (NOT(x) & z)),
    "（錯的）      XY+XZ+YZ=XY+XZ":         lambda x, y, z: ((x & y) | (x & z) | (y & z)) == ((x & y) | (x & z)),
}
for name, f in laws.items():
    bad = [r for r in product((0, 1), repeat=3) if not f(*r)]
    print(f"{name:36s} {'成立' if not bad else '不成立，反例 ' + str(bad[0])}")

# 兩個輸入的布林函數有幾個？每個真值表有 4 列，每列 0 或 1
tables = set(tuple(f) for f in product((0, 1), repeat=4))
print("兩個輸入的真值表共", len(tables), "種")

# 只用 NAND 做出 NOT、AND、OR、XOR
nand = lambda a, b: NOT(a & b)
n_not = lambda a: nand(a, a)
n_and = lambda a, b: n_not(nand(a, b))
n_or = lambda a, b: nand(n_not(a), n_not(b))
n_xor = lambda a, b: nand(nand(a, nand(a, b)), nand(b, nand(a, b)))
ok = all(n_and(a, b) == (a & b) and n_or(a, b) == (a | b) and n_xor(a, b) == (a ^ b) and n_not(a) == NOT(a)
         for a, b in product((0, 1), repeat=2))
print("只用 NAND 做出 NOT／AND／OR／XOR：", "全對" if ok else "錯")
