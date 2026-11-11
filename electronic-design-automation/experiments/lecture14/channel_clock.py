#!/usr/bin/env python3
"""Lecture 14 checks: basic & constrained left-edge on the slide examples, the robust-channel-router
DP for the top row, Elmore delay example, and the zero-skew tapping-point example (page 38)."""


def left_edge(intervals, vcg_parent=None):
    """intervals: {net: (s, e)}; vcg_parent: {net: set of nets that must be above it}."""
    U = dict(intervals)
    track, t = {}, 0
    while U:
        t += 1
        wm = 0
        while True:
            cand = [n for n, (s, e) in U.items() if s > wm and
                    (vcg_parent is None or not (vcg_parent.get(n, set()) & set(U)))]
            if not cand:
                break
            n = min(cand, key=lambda k: U[k][0])          # nearest watermark
            track[n] = t
            wm = U.pop(n)[1]
    return track


def density(intervals, ncol):
    return max(sum(s <= c <= e for s, e in intervals.values()) for c in range(1, ncol + 1))


if __name__ == '__main__':
    I = {1: (1, 3), 2: (2, 6), 3: (4, 8), 4: (5, 10), 5: (7, 11), 6: (9, 12)}
    tr = left_edge(I)
    print('basic left-edge :', {t: [n for n in tr if tr[n] == t] for t in sorted(set(tr.values()))},
          ' channel density', density(I, 12))
    J = {1: (1, 3), 2: (1, 5), 3: (6, 8), 4: (10, 11), 5: (2, 6), 6: (7, 9)}
    parents = {2: {1}, 5: {2}, 3: {5, 6}}               # VCG 1->2->5->3, 6->3
    tr = left_edge(J, parents)
    print('constrained LE  :', {t: [n for n in tr if tr[n] == t] for t in sorted(set(tr.values()))},
          ' channel density', density(J, 11))

    # robust channel router, top row (B=1000, K=5): weights from the slide, nets as intervals
    w = {1: -9, 2: 987, 3: 1004, 4: 993}
    span = {1: (1, 2), 2: (2, 4), 3: (3, 5), 4: (4, 5)}
    total, sel = [0] * 6, [0] * 6
    for c in range(1, 6):
        total[c], sel[c] = total[c - 1], 0
        for n, (a, b) in span.items():
            if b == c and w[n] + total[a - 1] > total[c]:
                total[c], sel[c] = w[n] + total[a - 1], n
    chosen, c = [], 5
    while c > 0:
        if sel[c]:
            chosen.append(sel[c])
            c = span[sel[c]][0] - 1
        else:
            c -= 1
    print('robust router top row: total =', total[1:], ' selected nets', chosen)

    # Elmore example (0.18um): r=0.075 ohm/um, c=0.118 fF/um
    r, cc = 0.075, 0.118
    cAB, cBC, cBD = 100 * cc, 150 * cc, 200 * cc
    dBC = r * 150 * (cBC / 2 + 2)
    dBD = r * 200 * (cBD / 2 + 4)
    dAB = r * 100 * (cAB / 2 + cBC + 2 + cBD + 4)
    print(f'Elmore: dBC={dBC:.0f} fs  dBD={dBD:.0f} fs  dAB={dAB:.0f} fs  critical={dAB + dBD:.0f} fs')

    # zero-skew merging (alpha=0.1 ohm/unit, beta=0.2 F/unit)
    a, b = 0.1, 0.2

    def tap(t1, C1, t2, C2, l):
        return ((t2 - t1) + a * l * (C2 + b * l / 2)) / (a * l * (b * l + C1 + C2))

    xE = tap(0, 16, 0, 10, 20)     # A(8,0) 16F with B(22,6) 10F, L=20
    tE = a * (1 - xE) * 20 * (b * (1 - xE) * 20 / 2 + 10)
    CE = 16 + 10 + b * 20
    xF = tap(0, 2, 0, 1, 10)       # D(5,15) 2F with C(0,10) 1F, L=10 (x measured from D)
    tF = a * xF * 10 * (b * xF * 10 / 2 + 2)
    CF = 2 + 1 + b * 10
    xG = tap(tF, CF, tE, CE, 10)   # F with E, L(F,E)=10
    print(f'E: x={xE:.2f} t_E={tE:.2f} C_E={CE:.0f}   F: x={xF:.2f} t_F={tF:.2f} C_F={CF:.0f}   G: x={xG:.2f}')
    # x outside [0,1] -> snaking: wire from G(=E) to F of length L with tF + aL(bL/2 + CF) = tE
    import math
    L = (-a * CF + math.sqrt((a * CF) ** 2 + 2 * a * b * (tE - tF))) / (a * b)
    print(f'snaking: wire length to F = {L:.2f} (Manhattan distance 10 -> extra {L - 10:.2f}, zigzag {(L - 10) / 2:.2f})')
