#!/usr/bin/env python3
"""Switch-level check of the CMOS compound-gate recipe (lecture 2, slides 91-92).

A network is a series/parallel tree of transistors.
  n-network: built from F' (AND -> series, OR -> parallel), nMOS conducts when gate = 1
  p-network: the dual (series <-> parallel), pMOS conducts when gate = 0
For every input, exactly one network must conduct (no float, no VDD-GND short).
"""
from itertools import product

# expression tree: ('var', name) | ('and', a, b, ...) | ('or', a, b, ...)
def V(n): return ('var', n)
def AND(*a): return ('and',) + a
def OR(*a): return ('or',) + a


def n_network(expr):
    """Pull-down network for output F where expr = F' (no inversions inside)."""
    if expr[0] == 'var':
        return ('nmos', expr[1])
    kind = 'series' if expr[0] == 'and' else 'parallel'
    return (kind,) + tuple(n_network(e) for e in expr[1:])


def dual(net):
    """p-network = dual of n-network: series <-> parallel, nMOS -> pMOS."""
    if net[0] == 'nmos':
        return ('pmos', net[1])
    kind = 'parallel' if net[0] == 'series' else 'series'
    return (kind,) + tuple(dual(n) for n in net[1:])


def conducts(net, inp):
    t = net[0]
    if t == 'nmos':
        return inp[net[1]] == 1
    if t == 'pmos':
        return inp[net[1]] == 0
    sub = [conducts(n, inp) for n in net[1:]]
    return all(sub) if t == 'series' else any(sub)


def count(net):
    return 1 if net[0] in ('nmos', 'pmos') else sum(count(n) for n in net[1:])


def evaluate(expr, inp):
    if expr[0] == 'var':
        return inp[expr[1]]
    sub = [evaluate(e, inp) for e in expr[1:]]
    return int(all(sub)) if expr[0] == 'and' else int(any(sub))


def check(name, fbar):
    names = sorted(variables(fbar))
    pdn = n_network(fbar)
    pun = dual(pdn)
    print(f'== {name}: F = ({fbar_str(fbar)})\'')
    print(f'   n-network: {pdn}')
    print(f'   p-network: {pun}')
    print(f'   transistors: {count(pdn)} nMOS + {count(pun)} pMOS = {count(pdn) + count(pun)}')
    ok = True
    for bits in product((0, 1), repeat=len(names)):
        inp = dict(zip(names, bits))
        up, down = conducts(pun, inp), conducts(pdn, inp)
        out = 1 if up and not down else 0 if down and not up else None
        want = 1 - evaluate(fbar, inp)
        if out != want or up == down:
            ok = False
            print(f'   FAIL {inp}: pull-up={up} pull-down={down}')
    print(f'   all {2 ** len(names)} inputs: exactly one network conducts and output = F -> {ok}')


def variables(e):
    return {e[1]} if e[0] == 'var' else set().union(*(variables(x) for x in e[1:]))


def fbar_str(e, top=True):
    if e[0] == 'var':
        return e[1]
    op = '·' if e[0] == 'and' else ' + '
    s = op.join(fbar_str(x, False) for x in e[1:])
    return s if e[0] == 'and' or top else '(' + s + ')'


if __name__ == '__main__':
    check('INV ', V('A'))
    check('NAND2', AND(V('A'), V('B')))
    check('NOR2 ', OR(V('A'), V('B')))
    check('AOI22 (slide 91 example)', OR(AND(V('A'), V('B')), AND(V('C'), V('D'))))
