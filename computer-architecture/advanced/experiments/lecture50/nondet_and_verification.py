#!/usr/bin/env python3
"""Lecture 50 verification:
1) Real run-to-run timing & memory-map nondeterminism on i7-13650HX + ts+theta_2 bus determinism
2) BFS Model Checker finding shortest counterexample on a Pentium-4-style 3-condition snoop/bus deadlock,
   and verifying programmable signal-tap workaround (Sarangi et al., Table 12.1)
3) DfD circular trace buffer capacity gain via spatial + temporal summarization
"""
from collections import deque
import subprocess, sys, time
import numpy as np

def measure_real_nondeterminism(runs=12):
    # Spawn fresh deterministic Python processes pinned to cpu0 to observe memory-map + timing variation
    code = (
        "import ctypes, time\n"
        "x = ctypes.c_int(42)\n"
        "t0 = time.perf_counter_ns()\n"
        "acc = 0\n"
        "for i in range(50000): acc = (acc * 131 + i) & 0xFFFFFFFF\n"
        "dt = time.perf_counter_ns() - t0\n"
        "print(acc, hex(ctypes.addressof(x)), dt)\n"
    )
    outputs, addrs, times_us = set(), set(), []
    for _ in range(runs):
        out = subprocess.check_output(["taskset", "-c", "0", sys.executable, "-c", code], text=True).strip()
        acc_s, addr_s, dt_s = out.split()
        outputs.add(int(acc_s))
        addrs.add(addr_s)
        times_us.append(int(dt_s) / 1e3)
    return outputs, len(addrs), min(times_us), max(times_us), np.std(times_us)

def sim_deterministic_bus(theta1=3, theta2=5, n_msgs=200):
    # Two senders send messages every 6 cycles; receiver logs arrival order + cycle into internal state
    def run_once(seed, buffer_to_worst_case):
        rng = np.random.default_rng(seed)
        deliveries = []
        for m in range(n_msgs):
            ts = m * 6
            j_a = int(rng.integers(theta1, theta2 + 1))
            j_b = int(rng.integers(theta1, theta2 + 1))
            ta = (ts + theta2) if buffer_to_worst_case else (ts + j_a)
            tb = (ts + theta2) if buffer_to_worst_case else (ts + j_b)
            deliveries.append((ta, 0, m))
            deliveries.append((tb, 1, m))
        deliveries.sort()  # receiver processes in timestamp, then deterministic port ID order
        lat = np.mean([d[0] - d[2] * 6 for d in deliveries])
        return tuple(deliveries), lat

    s1_raw, lat_raw = run_once(1, False)
    s2_raw, _       = run_once(2, False)
    s1_det, lat_det = run_once(1, True)
    s2_det, _       = run_once(2, True)
    return (s1_raw == s2_raw), lat_raw, (s1_det == s2_det), lat_det

def model_check_erratum(mode="unpatched"):
    # State = (line_state in {'I','S','M'}, snoop_active in {0,1}, bus_reinit in {0,1})
    start = ("I", 0, 0)
    def next_states(st):
        ls, snp, reinit = st
        # Fault 2 (Table 12.1): M-hit & snoop_active & bus_reinit simultaneously -> DEADLOCK (no outgoing edges!)
        if ls == "M" and snp == 1 and reinit == 1:
            return []
        nxt = []
        if not (mode == "full_fix" and reinit == 1):  # [28:08] freeze accesses while bus re-initializes
            for new_ls in ("I", "S", "M"):
                nxt.append((f"cpu_to_{new_ls}", (new_ls, snp, reinit)))
            nxt.append(("snoop_toggle", (ls, 1 - snp, reinit)))
        if mode == "unpatched" or (mode == "naive_fix" and not (ls == "M" and snp == 1)) or (mode == "full_fix" and snp == 0):
            nxt.append(("bus_reinit_req", (ls, snp, 1)))
        nxt.append(("bus_reinit_done", (ls, snp, 0)))
        return nxt

    q = deque([(start, [])])
    visited = {start}
    while q:
        u, path = q.popleft()
        edges = next_states(u)
        if len(edges) == 0:  # found deadlock counterexample!
            return visited, path + [("DEADLOCK_AT", u)]
        for act, v in edges:
            if v not in visited:
                visited.add(v)
                q.append((v, path + [(act, v)]))
    return visited, None

def sim_dfd_trace_summarization(cycles=4096, cores=4, sigs=8, buf_bytes=512):
    rng = np.random.default_rng(99)
    # 85% of epochs end cleanly in sleep/verified state (flushed by temporal summarization);
    # within active epochs, signals change sparsely (compressed spatially via bitmask delta)
    raw_cycles_fit = (buf_bytes * 8) // (cores * sigs)
    kept_bits = 0
    captured_cycles = 0
    for c in range(cycles):
        in_verified_sleep_epoch = (c % 64) < 54  # 84.4% benign verified epochs
        if in_verified_sleep_epoch:
            captured_cycles += 1
            continue
        # Active bug-hunting window: spatial column compression (1 bit header if all-zero column across 4 cores)
        mat = (rng.random((cores, sigs)) < 0.08).astype(int)
        col_bits = sum(1 if mat[:, j].sum() == 0 else (1 + cores) for j in range(sigs))
        if kept_bits + col_bits > buf_bytes * 8:
            break
        kept_bits += col_bits
        captured_cycles += 1
    return raw_cycles_fit, captured_cycles

if __name__ == "__main__":
    outs, n_addrs, t_min, t_max, t_std = measure_real_nondeterminism()
    same_raw, lat_raw, same_det, lat_det = sim_deterministic_bus()
    print("=== 1) Inherent Nondeterminism vs Deterministic Bus Buffering ===")
    print(f"Real CPU (12 runs): distinct outputs={len(outs)} | distinct stack addrs={n_addrs}/12 "
          f"| time={t_min:.1f}..{t_max:.1f} us (std={t_std:.1f} us)")
    print(f"Raw bus (jitter 3..5 cyc) : identical internal trace={same_raw!s:<5} | avg latency={lat_raw:.2f} cyc")
    print(f"Buffered to ts+theta2 (5) : identical internal trace={same_det!s:<5} | avg latency={lat_det:.2f} cyc")

    print("\n=== 2) BFS Model Checking on Fault-2-style Snoop/Bus-Reinit FSM ===")
    for mode in ("unpatched", "naive_fix", "full_fix"):
        vis, cex = model_check_erratum(mode)
        print(f"  {mode:<10}: explored {len(vis):2d} states -> Counterexample: {cex}")

    raw_fit, sum_fit = sim_dfd_trace_summarization()
    print(f"\n=== 3) DfD 512-Byte Trace Buffer Effective Lookback Window ===")
    print(f"Uncompressed raw signals: {raw_fit} cycles | Spatial + Temporal summarization: {sum_fit} cycles ({sum_fit/raw_fit:.1f}x window)")
