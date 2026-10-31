#!/usr/bin/env python3
"""Lecture 49 verification:
1) Unprotected vs Parity vs SECDED (72,64) ECC: No-Error / DUE / SDC breakdown
2) Invariant checker speed & single-bit flip detection on linear solve A*x = b
3) RLC power-grid resonance (50 MHz) & pipeline damping (throttling + dummy ops)
"""
import os
os.environ["OPENBLAS_NUM_THREADS"] = "1"
import time
import numpy as np

def build_secded_72_64():
    # Columns of H (7 rows for Hamming + 1 overall parity row):
    # Pick 64 distinct odd-weight or >=2-weight 7-bit non-zero syndrome columns for data,
    # and 7 unit vectors for Hamming check bits, plus overall parity bit.
    data_cols = [c for c in range(1, 128) if (c & (c - 1)) != 0][:64]
    chk_cols = [1 << i for i in range(7)]
    cols7 = data_cols + chk_cols + [0]  # 72 columns total; col 71 is overall parity
    H = np.zeros((8, 72), dtype=np.uint8)
    for j, c in enumerate(cols7):
        for r in range(7):
            H[r, j] = (c >> r) & 1
        H[7, j] = 1  # overall parity covers all 72 bits
    syn_to_col = {tuple(H[:, j]): j for j in range(72)}
    return H, syn_to_col

def sim_ecc(n_flips, trials=20000, avf=0.4, seed=42):
    rng = np.random.default_rng(seed + n_flips)
    H, syn_to_col = build_secded_72_64()
    stats = {k: {"ok": 0, "DUE": 0, "SDC": 0} for k in ("Unprotected", "Parity", "SECDED(72,64)")}
    for _ in range(trials):
        flips64 = rng.choice(64, size=n_flips, replace=False)
        live_hit = any(pos < int(64 * avf) for pos in flips64)  # first 40% bits architecturally live

        # 1) Unprotected 64-bit register
        stats["Unprotected"]["SDC" if live_hit else "ok"] += 1

        # 2) Single parity bit over 65 bits
        flips65 = rng.choice(65, size=n_flips, replace=False)
        live65 = any(pos < int(64 * avf) for pos in flips65 if pos < 64)
        if n_flips % 2 == 1:
            stats["Parity"]["DUE"] += 1
        else:
            stats["Parity"]["SDC" if live65 else "ok"] += 1

        # 3) SECDED (72,64): error vector e over 72 bits, syndrome s = H @ e mod 2
        e = np.zeros(72, dtype=np.uint8)
        e[rng.choice(72, size=n_flips, replace=False)] = 1
        s = tuple((H @ e) & 1)
        if s == (0,) * 8:
            rem = e
        elif s[7] == 1 and s in syn_to_col:
            rem = e.copy()
            rem[syn_to_col[s]] ^= 1  # 1-bit correction attempt
        else:
            stats["SECDED(72,64)"]["DUE"] += 1
            continue
        live72 = any(rem[pos] == 1 for pos in range(int(64 * avf)))
        stats["SECDED(72,64)"]["SDC" if live72 else "ok"] += 1
    return {k: {m: v * 100.0 / trials for m, v in d.items()} for k, d in stats.items()}

def bench_invariant_checker(n=1024):
    rng = np.random.default_rng(7)
    A = rng.standard_normal((n, n)) + n * np.eye(n)
    b = rng.standard_normal(n)

    t0 = time.perf_counter()
    A_inv = np.linalg.inv(A)
    x = A_inv @ b
    t_comp = (time.perf_counter() - t0) * 1e3

    t1 = time.perf_counter()
    res_ok = np.max(np.abs(A @ x - b))
    t_chk = (time.perf_counter() - t1) * 1e3

    # Inject a single bit-flip in bit 25 (mantissa) of x[100]
    x_corrupt = x.copy()
    bits = x_corrupt.view(np.uint64)
    bits[100] ^= (np.uint64(1) << np.uint64(25))
    res_bad = np.max(np.abs(A @ x_corrupt - b))
    return t_comp, t_chk, res_ok, res_bad

def sim_power_grid_rlc():
    # Package inductance L = 50 pH, on-chip decap C = 200 nF, damping R = 25 mOhm -> f0 ~ 50.3 MHz
    L, C, R = 50e-12, 200e-9, 0.025
    f0 = 1.0 / (2 * np.pi * np.sqrt(L * C))
    results = []
    for label, freq, i_hi, i_lo in [
        ("Off-resonance slow loop (5 MHz)",    5e6,  30.0,  5.0),
        ("At resonance, undamped  (50 MHz)",  50e6,  30.0,  5.0),
        ("At resonance, damped    (50 MHz)",  50e6,  20.0, 15.0),
        ("Off-resonance fast loop (500 MHz)", 500e6, 30.0,  5.0),
    ]:
        # Fundamental Fourier harmonic of square-wave current I(t) between i_lo and i_hi:
        # I_1 = (2 / pi) * (i_hi - i_lo); parallel tank Z(w) = 1 / (1/R + j w C + 1/(j w L))
        w = 2 * np.pi * freq
        Z = 1.0 / ((1.0 / R) + 1j * w * C + 1.0 / (1j * w * L))
        dv_peak = abs(Z) * (2.0 / np.pi) * (i_hi - i_lo)
        results.append((label, abs(Z) * 1e3, dv_peak))
    return f0, results

if __name__ == "__main__":
    print("=== 1) AVF / ECC Outcome Breakdown (AVF = 0.40, 20,000 trials) ===")
    for nf in (1, 2, 3):
        res = sim_ecc(nf)
        print(f"[{nf}-bit flip]")
        for scheme, d in res.items():
            print(f"  {scheme:<14}: No-error={d['ok']:5.1f}% | DUE={d['DUE']:5.1f}% | SDC={d['SDC']:5.1f}%")

    t_comp, t_chk, res_ok, res_bad = bench_invariant_checker(1024)
    print(f"\n=== 2) Compute vs Invariant Check (N=1024 A^-1 b vs ||Ax-b||_inf) ===")
    print(f"Matrix-inv solve: {t_comp:.2f} ms | Invariant check: {t_chk:.2f} ms ({t_comp/t_chk:.1f}x faster)")
    print(f"Residual clean  : {res_ok:.2e}  | Residual after 1 mantissa bit-flip: {res_bad:.2e}")

    f0, rlc = sim_power_grid_rlc()
    print(f"\n=== 3) RLC Power-Grid Resonance (f0 = {f0/1e6:.1f} MHz, Vdd = 1.0 V) ===")
    for label, z_mohm, dv in rlc:
        print(f"  {label:<34} | |Z|={z_mohm:5.2f} mOhm | peak dV = +/-{dv:.3f} V ({dv*100:.1f}% Vdd)")
