#!/usr/bin/env python3
"""Lecture 13: pin assignment on a soft macro boundary.
(1) assign each pin to the boundary slot that minimises the Manhattan distance to the pin it connects
    to outside (one pin per slot, greedy by distance) and count fly-line crossings before/after.
(2) multiply-instantiated block (MIB): both instances must use the SAME pin slots (in local
    coordinates); compare the shared optimum with each instance's private optimum."""
import itertools

W = H = 4                                   # macro size; slots every 1 unit along the boundary
SLOTS = [(x, 0) for x in range(1, W)] + [(W, y) for y in range(1, H)] + \
        [(x, H) for x in range(1, W)] + [(0, y) for y in range(1, H)]


def dist(a, b):
    return abs(a[0] - b[0]) + abs(a[1] - b[1])


def crosses(p1, q1, p2, q2):
    def ccw(a, b, c):
        return (c[1] - a[1]) * (b[0] - a[0]) - (b[1] - a[1]) * (c[0] - a[0])
    return (ccw(p1, q1, p2) * ccw(p1, q1, q2) < 0) and (ccw(p2, q2, p1) * ccw(p2, q2, q1) < 0)


def n_cross(lines):
    return sum(crosses(*a, *b) for a, b in itertools.combinations(lines, 2))


def assign(origin, targets):
    """exact: try all injective slot assignments (4 pins x 12 slots is small)."""
    best = None
    for combo in itertools.permutations(SLOTS, len(targets)):
        pos = [(origin[0] + s[0], origin[1] + s[1]) for s in combo]
        cost = sum(dist(p, t) for p, t in zip(pos, targets))
        if best is None or cost < best[0]:
            best = (cost, combo, pos)
    return best


if __name__ == '__main__':
    origin = (10, 10)                         # macro lower-left corner
    targets = [(20, 12), (12, 2), (4, 13), (12, 22)]   # pins of neighbouring macros (right, below, left, above)
    naive = [(origin[0] + s[0], origin[1] + s[1]) for s in [(1, H), (2, H), (3, H), (W, 3)]]  # all on top/right
    nl = list(zip(naive, targets))
    print(f'naive slots : wirelength {sum(dist(p, t) for p, t in nl)}, fly-line crossings {n_cross(nl)}')
    cost, combo, pos = assign(origin, targets)
    print(f'optimised   : wirelength {cost}, fly-line crossings {n_cross(list(zip(pos, targets)))}, slots {combo}')

    # MIB: two instances of the same block at different places, each pin i connects to a different target
    inst = [((10, 10), [(20, 12), (12, 2)]), ((30, 10), [(25, 12), (32, 22)])]
    private = sum(assign(o, t)[0] for o, t in inst)
    shared = None
    for combo in itertools.permutations(SLOTS, 2):
        c = sum(dist((o[0] + s[0], o[1] + s[1]), tt) for o, t in inst for s, tt in zip(combo, t))
        if shared is None or c < shared[0]:
            shared = (c, combo)
    print(f'MIB: sum of private optima {private}; best identical pin placement {shared[0]} slots {shared[1]}')
