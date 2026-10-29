# elmore.py：Elmore delay model（影片 [53:41]–[72:20]）
#  1. 一條導線切成 n 段 RC：Elmore 的時間常數 τ = Σ R_ij C_j = R C n(n+1)/2 → 跟長度的平方成正比
#  2. 用數值方法真的解 RC 梯子的電路（1 → 0 的 step），量末端掉到 1/e、到 50% 的時間，跟 Elmore 的 τ 比
#  3. 影片 [55:52] 的 RC tree：用「共用路徑上的電阻和」算 τ
import math

def elmore_ladder(n, R=1.0, C=1.0):
    return sum(R * j * C for j in range(1, n + 1))          # 第 j 個電容的共用路徑有 j 個電阻

def simulate_ladder(n, R=1.0, C=1.0, dt=None):
    """節點 1..n，節點 0 是電源（t=0 時從 1 V 變 0 V）。回傳末端到 1/e 和 0.5 的時間"""
    dt = dt or 0.002 * R * C
    v = [1.0] * (n + 1); v[0] = 0.0
    t, t_e, t_half = 0.0, None, None
    while t_e is None or t_half is None:
        dv = [0.0] * (n + 1)
        for k in range(1, n + 1):
            i_in = (v[k - 1] - v[k]) / R                    # 從左邊流進來
            i_out = (v[k] - v[k + 1]) / R if k < n else 0.0 # 往右邊流出去
            dv[k] = (i_in - i_out) / C * dt
        for k in range(1, n + 1): v[k] += dv[k]
        t += dt
        if t_half is None and v[n] <= 0.5: t_half = t
        if t_e is None and v[n] <= math.exp(-1): t_e = t
    return t_e, t_half

print('1、2. n 段的 RC 導線（R、C 都是 1，時間單位 RC）')
print('   n  Elmore τ   τ/n²   真的到 1/e   真的到 50%   50%/τ')
for n in (1, 2, 4, 8, 16, 32):
    tau = elmore_ladder(n); te, th = simulate_ladder(n)
    print(f'  {n:2d}  {tau:7.1f}   {tau / n ** 2:.3f}    {te:8.2f}    {th:8.2f}    {th / tau:.3f}')

# 3. RC tree：0 →R1→ 1；1 →R2→ 2 →R3→ 3；1 →R4→ 4 →R5→ 5（每個節點一個電容）
R = {1: 1.0, 2: 2.0, 3: 1.0, 4: 3.0, 5: 1.0}               # R[k]：通往節點 k 的那個電阻
parent = {1: 0, 2: 1, 3: 2, 4: 1, 5: 4}
C = {1: 1.0, 2: 1.0, 3: 2.0, 4: 1.0, 5: 1.0}
def path(k):
    p = []
    while k: p.append(k); k = parent[k]
    return set(p)
def tau(i):
    return sum(sum(R[k] for k in path(i) & path(j)) * C[j] for j in C)   # Σ_j R_ij C_j
print('3. RC tree（R1=1、R2=2、R3=1、R4=3、R5=1；C3=2，其他 C=1）')
for i in (3, 5):
    print(f'   節點 {i} 的 Elmore τ = {tau(i):.1f}；例：R_{i}4 = {sum(R[k] for k in path(i) & path(4)):.0f}（共用路徑上的電阻和）')
