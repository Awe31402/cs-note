import sys; sys.path.insert(0, '.')
import adders
from adders import bits, carry_select, delay
n = 64
for k in (1, 2, 4, 6, 8, 11, 16, 32, 64):
    A, B = bits(2**n - 1, n), bits(1, n)
    s, c = carry_select(A, B, (0, 0), k)
    print(f"n=64 k={k:2}: delay {delay(s, c)}")
