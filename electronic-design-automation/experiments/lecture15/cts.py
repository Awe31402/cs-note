#!/usr/bin/env python3
"""Lecture 15 checks.
(1) Why add buffers on a long interconnect (Fig. 13.44): Elmore delay of an unbuffered wire grows as
    L^2; splitting it into k segments with buffers (R_b, C_b, T_b) makes it ~linear.
(2) How many FF clock pins can one clock buffer drive under a max-transition limit
    (transition ~ 2.2 * R_drive * C_load), given FF input cap and wire RC per pin.
(3) Skew of an unbuffered star vs an H-tree on 16 sinks (pure wire Elmore delay)."""
import itertools

r, c = 0.075, 0.118          # ohm/um, fF/um (0.18um numbers from lecture 14); delays in fs
Rb, Cb, Tb = 200.0, 4.0, 15000.0   # buffer: output R (ohm), input C (fF), intrinsic delay (fs) -- assumed


def wire(L, Cload):
    return r * L * (c * L / 2 + Cload)


def buffered(L, k, Cload):
    seg = L / k
    d = 0.0
    for i in range(k):
        nxt = Cb if i < k - 1 else Cload
        d += Tb + Rb * (c * seg + nxt) + wire(seg, nxt)
    return d


print('(1) wire length (um): unbuffered delay vs best buffered delay [fs]')
for L in (1000, 2000, 4000, 8000, 16000):
    best = min((buffered(L, k, 4.0), k) for k in range(1, 40))
    print(f'    L={L:6d}: unbuffered {Rb * (c * L + 4) + wire(L, 4.0):11.0f}   buffered {best[0]:9.0f} (k={best[1]})')

# (2) max fanout under transition limit
Cff, wire_per_pin = 1.5, 20.0      # FF clock-pin cap (fF), estimated wire length per pin (um)
max_tran = 100000.0                 # 100 ps in fs
for name, Rd in (('CKBUFX4', 400.0), ('CKBUFX8', 200.0), ('CKBUFX16', 100.0)):
    n = 0
    while 2.2 * Rd * (n + 1) * (Cff + c * wire_per_pin) <= max_tran:
        n += 1
    print(f'(2) {name} (R={Rd:.0f} ohm): can drive {n} FF clock pins within 100 ps transition')

# (3) 16 sinks on a 4x4 grid (pitch 1000um), source at chip centre
sinks = [(x * 1000 + 500, y * 1000 + 500) for x, y in itertools.product(range(4), range(4))]
src = (2000, 2000)
star = [r * (abs(x - src[0]) + abs(y - src[1])) * (c * (abs(x - src[0]) + abs(y - src[1])) / 2 + Cff)
        for x, y in sinks]
print(f'(3) star (each sink wired straight from source): skew = {max(star) - min(star):.0f} fs')
print('    H-tree: every sink is at the same wire distance and sees an identical subtree -> skew = 0 by symmetry')
