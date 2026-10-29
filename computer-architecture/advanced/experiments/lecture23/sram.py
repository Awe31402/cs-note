# sram.py：第 23 支的三個小模擬
#  1. 10 電晶體的 CAM cell：照影片 [34:41] 的接法（兩條串聯的 NMOS 從 match line 到地），把電晶體當開關，
#     看哪些 (Q, A) 會讓 match line 被拉到地
#  2. 一個 8 列的 CAM 陣列：所有 match line 先 precharge 成 1，比完 → OR 得到 hit、priority encoder 得到 index
#  3. Bit line 的 RC 充放電：沒有 precharge（等 bit line 從 0 V 升上去）vs precharge 到 0.5 V、差 100 mV 就判斷
import math
from itertools import product

def conducts(series, sig):               # 一串 NMOS：全部的 gate 都是 1 才導通
    return all(sig[g] for g in series)

print('1. CAM cell（match line 先 precharge 成 1）')
print('   Q  A | Q·A̅ 那串  Q̅·A 那串 | match line')
for Q, A in product((0, 1), repeat=2):
    sig = {'Q': Q, 'Qb': 1 - Q, 'A': A, 'Ab': 1 - A}
    p1, p2 = conducts(['Q', 'Ab'], sig), conducts(['Qb', 'A'], sig)   # T1-T2 一串、T3-T4 一串
    ml = 0 if (p1 or p2) else 1
    print(f'   {Q}  {A} |    {"通" if p1 else "斷"}        {"通" if p2 else "斷"}     |   {ml}{"（Q ≠ A，被拉到地）" if ml == 0 else "（Q = A，保持 1）"}')

print('2. 8 列 × 4 bit 的 CAM 陣列，找 key = 1011')
rows = ['0110', '1011', '1110', '0001', '1011', '0000', '1010', '1111']
key = '1011'
ml = []
for r in rows:                           # 每一列：每個 cell 不相等就把 match line 拉到地
    m = 1
    for q, a in zip(r, key):
        if q != a: m = 0
    ml.append(m)
print('   match lines =', ''.join(map(str, ml)), '→ OR =', int(any(ml)),
      '；priority encoder（選最小的 index）→', ml.index(1) if any(ml) else None, '（第 4 列也符合，重複時要 priority encoder）')

print('3. Bit line：cell 很弱，bit line 的電容很大 → 用 RC 模型，時間以 RC 為單位')
# 沒有 precharge：bit line 從 0 V 往 1 V 充（V = 1 − e^{−t/RC}），等它到某個電壓才能判斷
for v in (0.5, 0.9, 0.95):
    print(f'   沒有 precharge，等 bit line 到 {v:.2f} V：{-math.log(1 - v):.3f} RC')
# precharge 到 0.5 V：BL 往 1 V、BL̅ 往 0 V，各自 0.5(1 − e^{−t/RC})，差 = 1 − e^{−t/RC}
for d in (0.05, 0.1, 0.2):
    print(f'   precharge 到 0.5 V，等兩條差 {d * 1000:.0f} mV：{-math.log(1 - d):.3f} RC')
t_pre = -math.log(1 - 0.1)
print(f'   快幾倍（以差 100 mV 為準）：對 0.5 V 是 {-math.log(0.5) / t_pre:.1f} 倍，對 0.9 V 是 {-math.log(0.1) / t_pre:.1f} 倍')
