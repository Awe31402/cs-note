#!/usr/bin/env python3
"""Lecture 9 checks.
(1) Polish expressions: balloting property, normalization, the M3 legality rule 2*N_{i+1} < i.
(2) Shape functions: vertical/horizontal composition of hard blocks with rotation
    (slide 'Floorplan Sizing - Example': A = 3x5, B = 2x4).
(3) Wong-Liu style simulated annealing on a small random instance (area only).
(4) B*-tree packing with a contour."""
import math
import random


# ---------------- (1) Polish expressions ----------------
def is_polish(E, n):
    ops = 0
    seen = []
    for k, e in enumerate(E, 1):
        if e in 'HV':
            ops += 1
        else:
            seen.append(e)
        if (k - ops) <= ops:          # balloting: #operands > #operators for every prefix
            return False
    return sorted(seen) == [str(i) for i in range(1, n + 1)] and len(E) == 2 * n - 1


def is_normalized(E):
    return all(not (a in 'HV' and a == b) for a, b in zip(E, E[1:]))


def m3_legal(E, i):
    """swap operand E[i-1] with operator E[i] (1-based i as on the slide): legal iff 2*N_{i+1} < i
    where N_k = #operators in e1..ek, and the result stays normalized."""
    N = sum(1 for e in E[:i + 1] if e in 'HV')
    F = E[:i - 1] + [E[i], E[i - 1]] + E[i + 1:]
    return 2 * N < i and is_normalized(F), F


# ---------------- (2) shape functions ----------------
def pareto(shapes):
    shapes = sorted(set(shapes))
    out = []
    for w, h in shapes:
        if not out or h < out[-1][1]:
            out.append((w, h))
    return out


def block(w, h):
    return pareto([(w, h), (h, w)])


def combine(op, L, R):
    """V: side by side (w add, h max); H: stacked (w max, h add)."""
    if op == 'V':
        return pareto([(a + c, max(b, d)) for a, b in L for c, d in R])
    return pareto([(max(a, c), b + d) for a, b in L for c, d in R])


def evaluate(E, dims):
    st = []
    for e in E:
        if e in 'HV':
            R = st.pop()
            L = st.pop()
            st.append(combine(e, L, R))
        else:
            st.append(block(*dims[e]))
    return st[0]


# ---------------- (3) simulated annealing (Wong-Liu moves) ----------------
def neighbours(E, rnd):
    E = list(E)
    while True:
        m = rnd.choice((1, 2, 3))
        if m == 1:                                  # swap two adjacent operands
            pos = [i for i, e in enumerate(E) if e not in 'HV']
            k = rnd.randrange(len(pos) - 1)
            a, b = pos[k], pos[k + 1]
            E[a], E[b] = E[b], E[a]
            return E
        if m == 2:                                  # complement a chain
            starts = [i for i, e in enumerate(E) if e in 'HV' and (i == 0 or E[i - 1] not in 'HV')]
            i = rnd.choice(starts)
            while i < len(E) and E[i] in 'HV':
                E[i] = 'H' if E[i] == 'V' else 'V'
                i += 1
            return E
        cand = [i for i in range(1, len(E)) if (E[i - 1] in 'HV') != (E[i] in 'HV')]
        i = rnd.choice(cand)
        F = E[:i - 1] + [E[i], E[i - 1]] + E[i + 1:]
        if is_polish(F, (len(E) + 1) // 2) and is_normalized(F):
            return F


def anneal(dims, seed=0, r=0.85, k=8):
    rnd = random.Random(seed)
    n = len(dims)
    E = ['1', '2', 'V'] + sum([[str(i), 'V'] for i in range(3, n + 1)], [])   # 12V3V...nV
    cost = lambda X: min(w * h for w, h in evaluate(X, dims))
    cur = best = cost(E)
    bestE, T = E, 50.0
    while T > 0.05:
        acc = 0
        for _ in range(k * n):
            F = neighbours(E, rnd)
            d = cost(F) - cur
            if d <= 0 or rnd.random() < math.exp(-d / T):
                E, cur, acc = F, cur + d, acc + 1
                if cur < best:
                    best, bestE = cur, E
        if acc < 0.05 * k * n:
            break
        T *= r
    return best, ''.join(bestE)


# ---------------- (4) B*-tree packing ----------------
def bstar_pack(tree, dims, root):
    """tree[n] = (left, right). left child: right-adjacent (x = x_parent + w_parent);
    right child: above, same x.  y from a contour (list of (x0, x1, top))."""
    pos, contour = {}, []

    def y_at(x0, x1):
        return max([t for a, b, t in contour if a < x1 and b > x0] + [0])

    def place(n, x):
        w, h = dims[n]
        y = y_at(x, x + w)
        pos[n] = (x, y)
        contour.append((x, x + w, y + h))
        L, R = tree.get(n, (None, None))
        if L:
            place(L, x + w)
        if R:
            place(R, x)

    place(root, 0)
    W = max(pos[n][0] + dims[n][0] for n in pos)
    H = max(pos[n][1] + dims[n][1] for n in pos)
    return pos, W, H


if __name__ == '__main__':
    for s in ('16H35V2HV74HV', '16H2V75VH34HV', '12H34H5HV', '123H4VV', '123HV4V', '12HH3V'):
        E = list(s)
        n = sum(1 for e in E if e not in 'HV')
        print(f'{s:15s} polish={is_polish(E, n)!s:5s} normalized={is_normalized(E)}')
    print('slide perturbation sequence (M1, M2, M3):')
    for s in ('12V4H3V', '12V3H4V', '12H3H4V', '12H34HV'):
        print(f'  {s}: polish={is_polish(list(s), 4)} normalized={is_normalized(list(s))}')
    E = list('12H3H4V')
    for i in range(1, len(E)):
        if E[i - 1] not in 'HV' and E[i] in 'HV':
            ok, F = m3_legal(E, i)
            print(f'  M3 operand/operator swap on 12H3H4V at i={i}: {"".join(F)}  2N_(i+1)<i and normalized -> {ok}')
    F = E[:4] + [E[5], E[4]] + E[6:]
    print(f'  M3 operator/operand swap H<->4 on 12H3H4V: {"".join(F)}  polish={is_polish(F, 4)} normalized={is_normalized(F)}')

    A, B = block(3, 5), block(2, 4)
    print('shape A:', A, ' shape B:', B)
    print('vertical stack (H) corner points:', combine('H', A, B))
    print('side by side   (V) corner points:', combine('V', A, B))

    rnd = random.Random(7)
    dims = {str(i): (rnd.randint(1, 6), rnd.randint(1, 6)) for i in range(1, 9)}
    total = sum(w * h for w, h in dims.values())
    best, E = anneal(dims)
    print(f'8 random hard blocks, total block area {total}: SA best bounding area {best} '
          f'(dead space {100 * (best - total) / best:.1f}%)  E = {E}')

    dims2 = {'b0': (4, 3), 'b1': (2, 2), 'b2': (2, 3), 'b3': (1, 2), 'b4': (3, 1)}
    tree = {'b0': ('b3', 'b1'), 'b1': (None, 'b2'), 'b3': ('b4', None)}
    pos, W, H = bstar_pack(tree, dims2, 'b0')
    print('B*-tree packing:', pos, f' bounding box {W} x {H}')
