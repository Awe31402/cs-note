#!/usr/bin/env python3
"""Lecture 10 checks.
(1) zero-force location of the slide's inverter example.
(2) quadratic placement Qx + d = 0 on a Fig.11.11-like circuit (movable 1,2,3; fixed 4,5,6),
    and the fact that repeatedly moving each cell to its zero-force location converges to the same
    answer (force-directed iteration = Gauss-Seidel on the quadratic objective).
(3) linear vs quadratic objective: A(fixed)=B(movable)-C(fixed), two wires A-B, one wire B-C."""
import numpy as np

# (1) inverter: Vdd w=8 at (0,2), in w=3 at (0,0), out w=10 at (2,?) , Gnd w=3 at (2,0)
pins = [(8, 0, 2), (3, 0, 0), (10, 2, 2), (3, 2, 0)]
W = sum(w for w, _, _ in pins)
print('zero-force x =', round(sum(w * x for w, x, _ in pins) / W, 3),
      ' y =', round(sum(w * y for w, _, y in pins) / W, 3))

# (2) Fig 11.11 topology with unit weights, fixed pins 4=(0,0), 5=(0,4), 6=(4,4)
c = {(1, 2): 1, (1, 3): 1, (1, 4): 1, (2, 4): 1, (2, 5): 1, (3, 6): 1}
fixed = {4: (0, 0), 5: (0, 4), 6: (4, 4)}
mov = [1, 2, 3]
Q = np.zeros((3, 3))
dx, dy = np.zeros(3), np.zeros(3)
for (i, j), w in c.items():
    for a, b in ((i, j), (j, i)):
        if a in mov:
            Q[mov.index(a), mov.index(a)] += w
            if b in mov:
                Q[mov.index(a), mov.index(b)] -= w
            else:
                dx[mov.index(a)] -= w * fixed[b][0]
                dy[mov.index(a)] -= w * fixed[b][1]
x = np.linalg.solve(Q, -dx)
y = np.linalg.solve(Q, -dy)
print('Q =\n', Q, '\n d_x =', dx, ' d_y =', dy)
print('quadratic optimum  x =', x.round(4), ' y =', y.round(4),
      ' Q positive definite:', bool(np.all(np.linalg.eigvalsh(Q) > 0)))
pos = {**fixed, 1: (2, 2), 2: (2, 2), 3: (2, 2)}
for it in range(60):                                   # zero-force iteration
    for m in mov:
        nb = [(w, j if i == m else i) for (i, j), w in c.items() if m in (i, j)]
        s = sum(w for w, _ in nb)
        pos[m] = (sum(w * pos[k][0] for w, k in nb) / s, sum(w * pos[k][1] for w, k in nb) / s)
print('zero-force iteration x =', np.round([pos[m][0] for m in mov], 4),
      ' y =', np.round([pos[m][1] for m in mov], 4))

# (3) linear vs quadratic, A at 0, C at l = 3
l = 3.0
grid = np.linspace(0, l, 3001)
quad = 2 * grid ** 2 + (l - grid) ** 2              # B at position b: two wires of length b, one of l-b
lin = 2 * grid + (l - grid)
print(f'quadratic: best B at {grid[quad.argmin()]:.3f} -> l_gamma = {l - grid[quad.argmin()]:.3f} (2l/3 = {2 * l / 3:.3f})')
print(f'linear   : best B at {grid[lin.argmin()]:.3f} -> l_gamma = {l - grid[lin.argmin()]:.3f} (= l)')
