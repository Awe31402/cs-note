#!/usr/bin/env python3
"""Lecture 6 checks.
(1) A tiny ROBDD (unique table + ITE) to count nodes for the slide examples under different orders.
(2) Quine-McCluskey prime implicants + Petrick's method on the slide examples.
(3) Two-level example F = x1'x2'x3' + x1'x2'x3 + x1x2'x3' + x1x2'x3 + x1x2x3'  ==  x2' + x1x3'."""
import itertools


# ---------------- (1) ROBDD ----------------
class BDD:
    def __init__(self, order):
        self.order = list(order)
        self.level = {v: i for i, v in enumerate(order)}
        self.unique = {}                 # (var, lo, hi) -> node id
        self.nodes = {0: None, 1: None}  # terminals
        self.memo = {}

    def mk(self, v, lo, hi):
        if lo == hi:                     # rule: eliminate redundant test
            return lo
        key = (v, lo, hi)
        if key not in self.unique:       # rule: merge isomorphic nodes
            n = len(self.nodes)
            self.nodes[n] = key
            self.unique[key] = n
        return self.unique[key]

    def var(self, v):
        return self.mk(v, 0, 1)

    def top(self, *fs):
        lv = [self.level[self.nodes[f][0]] for f in fs if f > 1]
        return self.order[min(lv)]

    def cof(self, f, v, b):
        if f <= 1 or self.nodes[f][0] != v:
            return f
        return self.nodes[f][2] if b else self.nodes[f][1]

    def ite(self, i, t, e):
        if i == 1:
            return t
        if i == 0:
            return e
        if t == e:
            return t
        if t == 1 and e == 0:
            return i
        key = (i, t, e)
        if key in self.memo:
            return self.memo[key]
        v = self.top(i, t, e)
        hi = self.ite(self.cof(i, v, 1), self.cof(t, v, 1), self.cof(e, v, 1))
        lo = self.ite(self.cof(i, v, 0), self.cof(t, v, 0), self.cof(e, v, 0))
        r = self.mk(v, lo, hi)
        self.memo[key] = r
        return r

    def AND(self, a, b): return self.ite(a, b, 0)
    def OR(self, a, b): return self.ite(a, 1, b)
    def NOT(self, a): return self.ite(a, 0, 1)
    def XOR(self, a, b): return self.ite(a, self.NOT(b), b)

    def size(self, f):
        seen, st = set(), [f]
        while st:
            n = st.pop()
            if n > 1 and n not in seen:
                seen.add(n)
                st += [self.nodes[n][1], self.nodes[n][2]]
        return len(seen)


def build(order, expr):
    b = BDD(order)
    x = {v: b.var(v) for v in order}
    return b, b.size(expr(b, x))


def f_abc(b, x):    # f = abc + b'd + c'd
    return b.OR(b.OR(b.AND(b.AND(x['a'], x['b']), x['c']), b.AND(b.NOT(x['b']), x['d'])),
                b.AND(b.NOT(x['c']), x['d']))


def f_pairs(b, x):  # f = ab + cd + ef
    return b.OR(b.OR(b.AND(x['a'], x['b']), b.AND(x['c'], x['d'])), b.AND(x['e'], x['f']))


def parity(n):
    def f(b, x):
        r = x['x1']
        for i in range(2, n + 1):
            r = b.XOR(r, x[f'x{i}'])
        return r
    return f


# ---------------- (2) Quine-McCluskey + Petrick ----------------
def qm_primes(minterms, nvars):
    terms = {tuple((m >> (nvars - 1 - i)) & 1 for i in range(nvars)) for m in minterms}
    primes = set()
    while terms:
        nxt, used = set(), set()
        for a, b in itertools.combinations(terms, 2):
            diff = [i for i in range(nvars) if a[i] != b[i]]
            if len(diff) == 1:
                c = list(a)
                c[diff[0]] = '-'
                nxt.add(tuple(c))
                used |= {a, b}
        primes |= terms - used
        terms = nxt
    return primes


def covers(p, m, nvars):
    bits = [(m >> (nvars - 1 - i)) & 1 for i in range(nvars)]
    return all(c == '-' or c == b for c, b in zip(p, bits))


def name(p, letters='ABCD'):
    s = ''.join(l + ("'" if c == 0 else '') for l, c in zip(letters, p) if c != '-')
    return s or '1'


def petrick(primes, minterms, nvars, letters='ABCD'):
    """Return all minimum-cardinality covers (exact, via product-of-sums expansion)."""
    primes = sorted(primes, key=lambda p: name(p, letters))
    sums = [frozenset(i for i, p in enumerate(primes) if covers(p, m, nvars)) for m in minterms]
    prods = {frozenset()}
    for s in sums:
        prods = {q | {i} for q in prods for i in s}
        prods = {q for q in prods if not any(o < q for o in prods)}   # absorption
    k = min(map(len, prods))
    return [sorted(name(primes[i], letters) for i in q) for q in prods if len(q) == k]


if __name__ == '__main__':
    print('ROBDD sizes (non-terminal nodes):')
    for order in ('bcda', 'abcd', 'adbc', 'bcad'):
        print(f'  f = abc + b\'d + c\'d   order {"<".join(order)}: {build(order, f_abc)[1]}')
    for order in ('abcdef', 'acebdf'):
        print(f'  f = ab + cd + ef      order {"<".join(order)}: {build(order, f_pairs)[1]}')
    for n in (4, 8, 16):
        print(f'  odd parity of {n} vars: {build([f"x{i}" for i in range(1, n + 1)], parity(n))[1]}  (2n-1 = {2 * n - 1})')

    ex1 = [0, 2, 5, 6, 7, 8, 10, 12, 13, 14, 15]
    p1 = qm_primes(ex1, 4)
    print('QM example 1  Σm(0,2,5,6,7,8,10,12,13,14,15) primes:', sorted(name(p) for p in p1))
    print('   minimum covers:', petrick(p1, ex1, 4))
    ex2 = [0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13]
    p2 = qm_primes(ex2, 4)
    print('QM example 2  Σm(0,2,3,4,5,6,7,8,9,10,11,12,13) primes:', sorted(name(p) for p in p2))
    print('   minimum covers:', petrick(p2, ex2, 4))
    ex3_on, ex3_dc = [2, 3, 7, 9, 11, 13], [1, 10, 15]
    p3 = qm_primes(ex3_on + ex3_dc, 4)
    print('QM example 3  Σm(2,3,7,9,11,13)+d(1,10,15) primes:', sorted(name(p) for p in p3))
    print('   minimum covers:', petrick(p3, ex3_on, 4))

    on = [0b000, 0b001, 0b100, 0b101, 0b110]          # x1'x2'x3', x1'x2'x3, x1x2'x3', x1x2'x3, x1x2x3'
    pr = qm_primes(on, 3)
    print("two-level example primes:", sorted(name(p, ['x1', 'x2', 'x3']) for p in pr),
          ' cover:', petrick(pr, on, 3, ['x1', 'x2', 'x3']))
