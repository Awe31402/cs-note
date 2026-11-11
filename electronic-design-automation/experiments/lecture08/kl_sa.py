#!/usr/bin/env python3
"""Lecture 8: Kernighan-Lin on the slide's weighted 6-node example, brute-force optimum,
and simulated annealing partitioning (cost = cut + lambda*(|S1|-|S2|)^2)."""
import itertools
import math
import random

V = 'abcdef'
C = [[0, 1, 2, 3, 2, 4],
     [1, 0, 1, 4, 2, 1],
     [2, 1, 0, 3, 2, 1],
     [3, 4, 3, 0, 4, 3],
     [2, 2, 2, 4, 0, 2],
     [4, 1, 1, 3, 2, 0]]
idx = {v: i for i, v in enumerate(V)}


def c(x, y):
    return C[idx[x]][idx[y]]


def cut(A, B):
    return sum(c(a, b) for a in A for b in B)


def kl_pass(A, B, verbose=True):
    A, B = list(A), list(B)
    D = {x: sum(c(x, y) for y in (B if x in A else A)) - sum(c(x, y) for y in (A if x in A else B) if y != x)
         for x in A + B}
    if verbose:
        print('   D =', {k: D[k] for k in V})
    locked, gains, pairs = set(), [], []
    for _ in range(len(A)):
        best = max(((D[a] + D[b] - 2 * c(a, b), a, b) for a in A for b in B
                    if a not in locked and b not in locked))
        g, a, b = best
        gains.append(g)
        pairs.append((a, b))
        locked |= {a, b}
        for x in A:          # D'_x = D_x + 2c_xa - 2c_xb
            if x not in locked:
                D[x] += 2 * c(x, a) - 2 * c(x, b)
        for y in B:          # D'_y = D_y + 2c_yb - 2c_ya
            if y not in locked:
                D[y] += 2 * c(y, b) - 2 * c(y, a)
        if verbose:
            print(f'   swap {a},{b}: g = {g}')
    partial = list(itertools.accumulate(gains))
    k = max(range(len(partial)), key=lambda i: partial[i])
    G = partial[k]
    if verbose:
        print(f'   gains {gains}, partial sums {partial}, best k = {k + 1}, G_k = {G}')
    if G > 0:
        for a, b in pairs[:k + 1]:
            A[A.index(a)], B[B.index(b)] = b, a
    return A, B, G


def kernighan_lin(A, B):
    it = 1
    while True:
        print(f' pass {it}: A={A} B={B} cut={cut(A, B)}')
        A, B, G = kl_pass(A, B)
        if G <= 0:
            return A, B
        it += 1


def brute():
    best = min((cut(S, [v for v in V if v not in S]), S) for S in itertools.combinations(V, 3) if 'a' in S)
    return best


def sa(seed, lam=1.0, T0=10.0, r=0.9):
    rnd = random.Random(seed)
    side = {v: rnd.random() < 0.5 for v in V}

    def cost():
        A = [v for v in V if side[v]]
        B = [v for v in V if not side[v]]
        return cut(A, B) + lam * (len(A) - len(B)) ** 2

    T, cur, frozen = T0, cost(), 0
    while frozen < 3:
        acc = 0
        for _ in range(100 * len(V)):
            v = rnd.choice(V)
            side[v] = not side[v]           # move one cell to the other side
            new = cost()
            d = new - cur
            if d <= 0 or rnd.random() < math.exp(-d / T):
                cur, acc = new, acc + 1
            else:
                side[v] = not side[v]
        frozen = frozen + 1 if acc < 0.02 * 100 * len(V) else 0
        T *= r
    A = sorted(v for v in V if side[v])
    return A, sorted(v for v in V if not side[v]), cur


if __name__ == '__main__':
    print('Kernighan-Lin from A={a,b,c}, B={d,e,f}:')
    A, B = kernighan_lin(list('abc'), list('def'))
    print(f' result A={sorted(A)} B={sorted(B)} cut={cut(A, B)}')
    print('brute-force optimum over all balanced bisections:', brute())
    print('simulated annealing (lambda=1, r=0.9):')
    for s in range(5):
        print('  seed', s, sa(s))
