#!/usr/bin/env python3
"""Lecture 7: constraint-graph compaction.  Longest path on the DAG (BFS with in-degrees),
Liao-Wong and Bellman-Ford on the graph with backward (max-distance) edges, and positive-cycle
detection.  Graph = slide 'DAG Longest-Path Example' + backward edges of the Liao-Wong example."""
from collections import deque

NEG = float('-inf')
FWD = [(0, 1, 1), (0, 2, 5), (1, 2, 2), (1, 5, 2), (2, 3, 1), (2, 4, 1), (5, 4, 4)]
BWD = [(2, 1, -3), (4, 3, -1), (4, 5, -4)]       # x2-x1<=3, x4-x3<=1, x4-x5<=4
N = 6                                            # v0..v5


def longest_dag(edges, x, n=N, trace=None):
    """Slide 'Longest-Path Algorithm for DAGs' (breadth-first with in-degree counters)."""
    p = [0] * n
    for _, j, _ in edges:
        p[j] += 1
    q = deque([0])
    while q:
        i = q.popleft()
        for a, j, d in edges:
            if a == i:
                x[j] = max(x[j], x[i] + d)
                p[j] -= 1
                if p[j] <= 0:
                    q.append(j)
        if trace is not None:
            trace.append((i, x[1:]))
    return x


def liao_wong(fwd, bwd):
    x = [NEG] * N
    x[0] = 0
    rows, count = [], 0
    while True:
        longest_dag(fwd, x)
        rows.append(('forward', x[1:]))
        flag = False
        for i, j, d in bwd:
            if x[j] < x[i] + d:
                x[j] = x[i] + d
                flag = True
        rows.append(('backward', x[1:]))
        count += 1
        if not flag:
            return rows
        if count > len(bwd):
            rows.append(('positive cycle', None))
            return rows


def bellman_ford(edges):
    x = [NEG] * N
    x[0] = 0
    s1, rows, count = [0], [], 0
    while count <= N and s1:
        s2 = []
        for i in s1:
            for a, j, d in edges:
                if a == i and x[j] < x[i] + d:
                    x[j] = x[i] + d
                    if j not in s2:
                        s2.append(j)
        rows.append((sorted(s1), x[1:]))
        s1, count = s2, count + 1
    if count > N:
        rows.append(('positive cycle', None))
    return rows


def fmt(v):
    return ' '.join(f'{a:>4}' if a != NEG else '-inf' for a in v)


if __name__ == '__main__':
    tr = []
    x = longest_dag(FWD, [0] * N, trace=tr)
    print('DAG longest path (x1..x5), one row per vertex popped from Q:')
    for v, row in tr:
        print(f'  pop v{v}: {fmt(row)}')
    print('\nLiao-Wong (all backward edges each round):')
    for k, row in liao_wong(FWD, BWD):
        print(f'  {k:9s}: {fmt(row) if row else ""}')
    print('\nBellman-Ford longest path, wave fronts S1:')
    for s, row in bellman_ford(FWD + BWD):
        print(f'  S1={s}: {fmt(row) if row else ""}')
    print('\nMake x2-x1 <= 1 (backward v2->v1 weight -1) -> conflicts with x2-x1 >= 2 via v1->v2:')
    bad = [(2, 1, -1), (4, 3, -1), (4, 5, -4)]
    print('  Liao-Wong :', liao_wong(FWD, bad)[-1][0])
    print('  Bellman-Ford:', bellman_ford(FWD + bad)[-1][0])
