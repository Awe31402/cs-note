#!/usr/bin/env python3
"""Lecture 11: Lee (BFS wave), Hadlock (detour number, 0-1 BFS) and Soukup (line + bubble)
on the same grid; also Akers' 1,1,2,2 labelling and multi-terminal Lee."""
from collections import deque

GRID = [
    "..............",
    "...######.....",
    "........#.....",
    "..##....#..T..",
    "..##....#.....",
    "........####..",
    "....S.........",
    "..............",
]
H, W = len(GRID), len(GRID[0])
find = lambda ch: next((r, c) for r in range(H) for c in range(W) if GRID[r][c] == ch)
S, T = find('S'), find('T')
free = lambda r, c: 0 <= r < H and 0 <= c < W and GRID[r][c] != '#'
NB = ((1, 0), (-1, 0), (0, 1), (0, -1))


def lee(src, dst_set):
    dist = {src: 0}
    q = deque([src])
    while q:
        u = q.popleft()
        if u in dst_set:
            break
        for dr, dc in NB:
            v = (u[0] + dr, u[1] + dc)
            if free(*v) and v not in dist:
                dist[v] = dist[u] + 1
                q.append(v)
    return dist, u


def retrace(dist, t):
    path = [t]
    while dist[path[-1]] > 0:
        r, c = path[-1]
        path.append(next((r + dr, c + dc) for dr, dc in NB if dist.get((r + dr, c + dc)) == dist[path[-1]] - 1))
    return path[::-1]


def hadlock(src, dst):
    md = lambda p: abs(p[0] - dst[0]) + abs(p[1] - dst[1])
    det = {src: 0}
    dq = deque([src])
    expanded = 0
    while dq:
        u = dq.popleft()
        expanded += 1
        if u == dst:
            return det[u], expanded
        for dr, dc in NB:
            v = (u[0] + dr, u[1] + dc)
            if not free(*v):
                continue
            nd = det[u] + (1 if md(v) > md(u) else 0)      # moving away from T costs a detour
            if nd < det.get(v, 1 << 30):
                det[v] = nd
                (dq.append if nd > det[u] else dq.appendleft)(v)
    return None, expanded


def soukup(src, dst):
    """Line search toward T; when blocked, Lee-type bubble until a cell closer to T is found."""
    md = lambda p: abs(p[0] - dst[0]) + abs(p[1] - dst[1])
    cur, length, visited = src, 0, {src}
    while cur != dst:
        moved = False
        for dr, dc in NB:                      # greedy straight step that reduces distance
            v = (cur[0] + dr, cur[1] + dc)
            if free(*v) and v not in visited and md(v) < md(cur):
                cur, length, moved = v, length + 1, True
                visited.add(v)
                break
        if moved:
            continue
        dist = {cur: 0}                        # bubble around the obstacle
        q = deque([cur])
        while q:
            u = q.popleft()
            if md(u) < md(cur) and u not in visited:
                break
            for dr, dc in NB:
                v = (u[0] + dr, u[1] + dc)
                if free(*v) and v not in dist:
                    dist[v] = dist[u] + 1
                    q.append(v)
        else:
            return None
        length += dist[u]
        visited |= set(dist)
        cur = u
    return length


def akers(d):
    return (1, 1, 2, 2)[d % 4]


def show(path):
    g = [list(row) for row in GRID]
    for r, c in path[1:-1]:
        g[r][c] = '*'
    return '\n'.join('   ' + ''.join(row) for row in g)


if __name__ == '__main__':
    md = abs(S[0] - T[0]) + abs(S[1] - T[1])
    dist, _ = lee(S, {T})
    path = retrace(dist, T)
    print(f'S={S} T={T} Manhattan distance MD={md}')
    print(f'Lee: shortest length {dist[T]}, cells labelled {len(dist)} of {sum(free(r, c) for r in range(H) for c in range(W))} free')
    print(show(path))
    d, exp = hadlock(S, T)
    print(f'Hadlock: detour d(P)={d}  ->  l(P) = MD + 2d = {md + 2 * d}; cells expanded {exp}')
    print(f'Soukup: path length {soukup(S, T)} (not guaranteed shortest)')
    labels = [akers(dist[p]) for p in path]
    print('Akers 1,1,2,2 labels along the Lee path:', ''.join(map(str, labels)))
    pins = [S, T, (0, 0), (7, 13)]
    tree = {pins[0]}
    total = 0
    rest = set(pins[1:])
    while rest:
        hit = None
        q, seen = deque(tree), {p: 0 for p in tree}
        while q:
            u = q.popleft()
            if u in rest:
                hit = u
                break
            for dr, dc in NB:
                v = (u[0] + dr, u[1] + dc)
                if free(*v) and v not in seen:
                    seen[v] = seen[u] + 1
                    q.append(v)
        p = [hit]
        while seen[p[-1]] > 0:
            r, c = p[-1]
            p.append(next((r + dr, c + dc) for dr, dc in NB if seen.get((r + dr, c + dc)) == seen[p[-1]] - 1))
        total += seen[hit]
        tree |= set(p)
        rest.discard(hit)
    print(f'multi-terminal Lee (4 pins, propagate from ALL tree cells): tree wirelength {total}')
