#!/usr/bin/env python3
"""Lecture 4: re-implement the tram demo shown in the video (walk s->s+1 cost 1, tram s->3s cost 3,
N = 500) with backtracking, memoised DP and uniform cost search, and time them."""
import heapq
import sys
import time

sys.setrecursionlimit(100000)   # the video shows the program crashing without this line


class Transportation:
    def __init__(self, n, factor=3, tram_cost=3):
        self.n, self.factor, self.tram_cost = n, factor, tram_cost

    def start(self):
        return 1

    def is_end(self, s):
        return s == self.n

    def succ_and_cost(self, s):
        out = []
        if s + 1 <= self.n:
            out.append(('walk', s + 1, 1))
        if s * self.factor <= self.n:
            out.append(('tram', s * self.factor, self.tram_cost))
        return out


def backtracking(p):
    best = {'cost': float('inf'), 'hist': None}

    def rec(s, hist, cost):
        if p.is_end(s):
            if cost < best['cost']:
                best['cost'], best['hist'] = cost, hist
            return
        for a, t, c in p.succ_and_cost(s):
            rec(t, hist + [(a, t, c)], cost + c)

    rec(p.start(), [], 0)
    return best['cost'], best['hist']


def dynamic_programming(p):
    cache = {}

    def future(s):
        if p.is_end(s):
            return 0
        if s not in cache:
            cache[s] = min(c + future(t) for _, t, c in p.succ_and_cost(s))
        return cache[s]

    return future(p.start())


def uniform_cost_search(p):
    pq, done = [(0, p.start())], set()
    while pq:
        cost, s = heapq.heappop(pq)
        if p.is_end(s):
            return cost
        if s in done:
            continue
        done.add(s)
        for _, t, c in p.succ_and_cost(s):
            if t not in done:
                heapq.heappush(pq, (cost + c, t))


def timed(f, *a):
    t0 = time.perf_counter()
    r = f(*a)
    return r, time.perf_counter() - t0


if __name__ == '__main__':
    p = Transportation(500)
    print('succ(3) =', p.succ_and_cost(3), ' succ(6) =', p.succ_and_cost(6))
    (cost, hist), t = timed(backtracking, p)
    print(f'backtracking   N=500: totalCost {cost}  ({t:.3f} s)')
    for h in hist:
        print('   ', h)
    r, t = timed(dynamic_programming, p)
    print(f'dynamicProg    N=500: totalCost {r}  ({t * 1e3:.3f} ms)')
    r, t = timed(uniform_cost_search, p)
    print(f'uniformCost    N=500: totalCost {r}  ({t * 1e3:.3f} ms)')
    print('backtracking runtime grows exponentially with N:')
    for n in (100, 200, 300, 400, 500, 700):
        (_, _), t = timed(backtracking, Transportation(n))
        print(f'   N={n:4d}: {t:.3f} s')
