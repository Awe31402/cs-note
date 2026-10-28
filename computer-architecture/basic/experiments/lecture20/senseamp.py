# bit line 的 RC 模型：從 0.5 V 開始，一條往 1 V、一條往 0 V 走
# V(t) = 0.5 ± 0.5 (1 - e^{-t/RC})，兩條線的差 Δ(t) = 1 - e^{-t/RC}
import math
for target in (0.9, 0.99):                      # 等到差距 0.9 V / 0.99 V（接近滿擺幅）
    print(f"wait until Δ = {target:.2f} V : t = {-math.log(1 - target):.3f} RC")
for th in (0.1, 0.05):                          # sense amplifier 的門檻
    print(f"sense amp threshold {th:.2f} V : t = {-math.log(1 - th):.3f} RC")
print(f"speed-up (0.99 V vs 0.1 V) = {math.log(100) / -math.log(0.9):.0f}x")
