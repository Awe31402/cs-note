#!/usr/bin/env python3
"""第 1 支影片：bit line 上的電壓到底變多少？（charge sharing）

cell 電容 Cs 和 bit line 電容 Cb 接在一起後，電荷守恆：
    Cs*Vcell + Cb*Vpre = (Cs+Cb)*Vfinal
=>  dV = Vfinal - Vpre = (Vcell - Vpre) * Cs / (Cs + Cb)
"""

VDD = 3.0          # 影片用的供電電壓（示意值）
VPRE = VDD / 2     # bit line precharge 到一半
CS = 30e-15        # 影片：cell 電容約 30 fF

print(f"Vdd = {VDD} V, Vpre = {VPRE} V, Cs = {CS*1e15:.0f} fF")
print(f"{'Cb/Cs':>6} {'Cb (fF)':>8} {'dV 存 1 (mV)':>14} {'dV 存 0 (mV)':>14}")
for ratio in (1, 2, 5, 10, 20, 50, 100):
    cb = CS * ratio
    dv1 = (VDD - VPRE) * CS / (CS + cb)
    dv0 = (0.0 - VPRE) * CS / (CS + cb)
    print(f"{ratio:>6} {cb*1e15:>8.0f} {dv1*1e3:>+14.1f} {dv0*1e3:>+14.1f}")

# 反過來問：要小到「零點幾 mV」，Cb 得多大？
target = 0.5e-3
cb_needed = CS * ((VDD - VPRE) / target - 1)
print(f"\n要讓 dV 只有 {target*1e3} mV，Cb 要約 {cb_needed*1e12:.0f} pF"
      f"（= Cs 的 {cb_needed/CS:.0f} 倍）")

# 影片：每個 cell 約 64 ms refresh 一次。DDR 標準把它分成 8192 次 REF 命令。
T_REFW = 64e-3
N_REF = 8192
print(f"\nrefresh：{T_REFW*1e3:.0f} ms / {N_REF} 次 = 每 {T_REFW/N_REF*1e6:.2f} us 一次 REF")
