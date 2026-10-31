#!/usr/bin/env python3
"""1) Real CPU lateral & transient temperature measurement on Intel i7-13650HX.
2) 2D steady-state FDM (T = A*P, O(N^2)) vs FFT Green's function (T = P*G) + method of images."""
import os
os.environ.setdefault("OPENBLAS_NUM_THREADS", "1")
os.environ.setdefault("OMP_NUM_THREADS", "1")

import glob, subprocess, time
import numpy as np

def find_coretemp():
    for hw in glob.glob("/sys/class/hwmon/hwmon*"):
        if os.path.exists(hw + "/name") and open(hw + "/name").read().strip() == "coretemp":
            return {open(lbl).read().strip(): lbl[:-6] + "_input"
                    for lbl in glob.glob(hw + "/temp*_label")}
    return {}

def demo_cpu_spatial_transient():
    sensors = find_coretemp()
    keys = ["Core 0", "Core 4", "Core 12", "Core 20"]
    if not all(k in sensors for k in keys):
        return
    read_t = lambda: [int(open(sensors[k]).read()) / 1000.0 for k in keys]
    time.sleep(1.1)  # let coretemp's 1-second (HZ) cache expire in idle

    print("1) Real CPU temperatures (1 heavy thread pinned to cpu0 = Core 0 for 3.3s):")
    print(f"{'Time (s)':>8} | {'Phase':<6} | {'Core 0':>7} | {'Core 4':>7} | {'Core 12':>8} | {'Core 20':>8}")
    print("-" * 58)
    t0 = time.perf_counter()
    vals = read_t()
    print(f"{0.00:8.2f} | {'idle':<6} | {vals[0]:6.0f}C | {vals[1]:6.0f}C | {vals[2]:7.0f}C | {vals[3]:7.0f}C")

    # Tight numpy vector FMA loop pinned on cpu0
    cmd = ["taskset", "-c", "0", "python3", "-c",
           "import numpy as np; a=np.ones(4096); \nwhile True: a = a*1.0000001 + 1e-7"]
    proc = subprocess.Popen(cmd)
    try:
        for target in (1.1, 2.2, 3.3):
            time.sleep(max(0.0, target - (time.perf_counter() - t0)))
            vals = read_t()
            print(f"{time.perf_counter()-t0:8.2f} | {'burn':<6} | {vals[0]:6.0f}C | {vals[1]:6.0f}C | {vals[2]:7.0f}C | {vals[3]:7.0f}C")
    finally:
        proc.terminate()
        proc.wait()

    for target in (4.4, 5.5):
        time.sleep(max(0.0, target - (time.perf_counter() - t0)))
        vals = read_t()
        print(f"{time.perf_counter()-t0:8.2f} | {'cool':<6} | {vals[0]:6.0f}C | {vals[1]:6.0f}C | {vals[2]:7.0f}C | {vals[3]:7.0f}C")
    print()

def build_fdm_matrix(m, k_si=130.0, die_xy=0.02, dz=0.5e-3, r_vert_cm2=0.35):
    dx = die_xy / m
    g_lat = k_si * (dx * dz) / dx
    g_vert = ((dx * 100.0) ** 2) / r_vert_cm2
    n = m * m
    G = np.zeros((n, n), dtype=np.float64)
    for r in range(m):
        for c in range(m):
            u = r * m + c
            diag = g_vert
            for dr, dc in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                nr, nc = r + dr, c + dc
                if 0 <= nr < m and 0 <= nc < m:
                    G[u, nr * m + nc] = -g_lat
                    diag += g_lat
            G[u, u] = diag
    return G, g_lat, g_vert

def solve_greens_fft(P_2d, G_fft):
    m = P_2d.shape[0]
    P_mir = np.block([[P_2d, P_2d[:, ::-1]],
                      [P_2d[::-1, :], P_2d[::-1, ::-1]]])
    return np.fft.irfft2(np.fft.rfft2(P_mir) * G_fft, s=(2 * m, 2 * m))[:m, :m]

def demo_models():
    m = 64  # N = 4096 cells
    n = m * m
    G_mat, g_lat, g_vert = build_fdm_matrix(m)
    A = np.linalg.inv(G_mat)

    ky = 2 * np.pi * np.arange(2 * m)[:, None] / (2 * m)
    kx = 2 * np.pi * np.arange(m + 1)[None, :] / (2 * m)
    G_fft = 1.0 / (g_vert + 2 * g_lat * (2 - np.cos(ky) - np.cos(kx)))

    P_far = np.zeros((m, m))
    P_far[12:20, 12:20] = 15.0 / 64.0
    P_far[44:52, 44:52] = 15.0 / 64.0

    P_adj = np.zeros((m, m))
    P_adj[28:36, 24:32] = 15.0 / 64.0
    P_adj[28:36, 32:40] = 15.0 / 64.0

    T_fdm = (A @ P_far.ravel()).reshape(m, m) + 45.0
    T_grn = solve_greens_fft(P_far, G_fft) + 45.0
    T_adj = solve_greens_fft(P_adj, G_fft) + 45.0
    # Rim/corner effect: same single 15W core at center vs adiabatic corner
    P_ctr = np.zeros((m, m)); P_ctr[28:36, 28:36] = 15.0 / 64.0
    P_cor = np.zeros((m, m)); P_cor[0:8, 0:8] = 15.0 / 64.0

    p_flat = P_far.ravel()
    t0 = time.perf_counter()
    for _ in range(100):
        _ = A @ p_flat
    us_fdm = (time.perf_counter() - t0) / 100 * 1e6

    t0 = time.perf_counter()
    for _ in range(100):
        _ = solve_greens_fft(P_far, G_fft)
    us_grn = (time.perf_counter() - t0) / 100 * 1e6

    print(f"2) 2D Chip Model ({m}x{m}, N={n} cells, 1-thread timing):")
    print(f"   Max |T_FDM - T_Green(mirror)| = {np.max(np.abs(T_fdm - T_grn)):.2e} C")
    print(f"   Single 15W source: center = {(solve_greens_fft(P_ctr, G_fft)+45).max():.2f} C vs adiabatic corner = {(solve_greens_fft(P_cor, G_fft)+45).max():.2f} C")
    print(f"   Two 15W sources:   far apart = {T_grn.max():.2f} C vs side-by-side = {T_adj.max():.2f} C")
    print(f"   1-core time per solve: dense FDM A@P = {us_fdm:.1f} us vs FFT Green's P*G = {us_grn:.1f} us ({us_fdm/us_grn:.1f}x faster)")

if __name__ == "__main__":
    demo_cpu_spatial_transient()
    demo_models()
