#!/usr/bin/env python3
"""Inspect cpufreq/HWP + cpuidle (clock vs power gating latency/residency),
and test 2-core job placement (adjacent vs far-apart P-cores) on i7-13650HX."""
import glob, os, subprocess, time

def read_str(path):
    try:
        return open(path).read().strip()
    except OSError:
        return "N/A"

def show_cpufreq_and_cpuidle():
    base = "/sys/devices/system/cpu/cpu0/cpufreq"
    print("=== 1) cpufreq / Hardware DVFS (cpu0) ===")
    print(f"driver={read_str(base+'/scaling_driver')} | governor={read_str(base+'/scaling_governor')} "
          f"| EPP={read_str(base+'/energy_performance_preference')} "
          f"| range={int(read_str(base+'/scaling_min_freq'))//1000}-{int(read_str(base+'/scaling_max_freq'))//1000} MHz")

    print("\n=== 2) cpuidle States: Clock Gating vs Power Gating (cpu0) ===")
    print(f"{'State':<10} | {'Exit Latency (us)':>17} | {'Min Residency (us)':>18} | {'Usage Count':>12}")
    print("-" * 66)
    for st in sorted(glob.glob("/sys/devices/system/cpu/cpu0/cpuidle/state*")):
        name = read_str(st + "/name")
        lat = read_str(st + "/latency")
        res = read_str(st + "/residency")
        usage = read_str(st + "/usage")
        print(f"{name:<10} | {lat:>17} | {res:>18} | {usage:>12}")

def find_coretemp():
    for hw in glob.glob("/sys/class/hwmon/hwmon*"):
        if read_str(hw + "/name") == "coretemp":
            return {read_str(lbl): lbl[:-6] + "_input"
                    for lbl in glob.glob(hw + "/temp*_label")}
    return {}

def run_placement(label, cpus, sensors):
    time.sleep(2.5)  # cool down between runs
    pcores = ["Core 0", "Core 4", "Core 8", "Core 12", "Core 16", "Core 20"]
    burn_code = "import numpy as np; a=np.ones(8192)\nwhile True: a = a*1.0000001 + 1e-7"
    procs = [subprocess.Popen(["taskset", "-c", str(c), "python3", "-c", burn_code]) for c in cpus]
    peak_per_core = {k: 0 for k in pcores}
    freqs = []
    try:
        for _ in range(3):
            time.sleep(1.1)
            for k in pcores:
                t = int(read_str(sensors[k])) // 1000
                if t > peak_per_core[k]:
                    peak_per_core[k] = t
            f_mhz = [int(read_str(f"/sys/devices/system/cpu/cpu{c}/cpufreq/scaling_cur_freq")) // 1000 for c in cpus]
            freqs.append(sum(f_mhz) / len(f_mhz))
    finally:
        for p in procs:
            p.terminate()
        for p in procs:
            p.wait()
    temps_str = " ".join(f"{k.split()[1]}:{peak_per_core[k]}C" for k in pcores)
    print(f"{label:<26} | peak={max(peak_per_core.values()):2d}C | avg_f={sum(freqs)/len(freqs):.0f} MHz | [{temps_str}]")

if __name__ == "__main__":
    show_cpufreq_and_cpuidle()
    sensors = find_coretemp()
    if sensors:
        print("\n=== 3) Job Placement on P-cores (2 hot threads, 3.3s each) ===")
        run_placement("Far apart (cpu0 + cpu10)", [0, 10], sensors)
        run_placement("Adjacent  (cpu0 + cpu2)",  [0, 2],  sensors)
