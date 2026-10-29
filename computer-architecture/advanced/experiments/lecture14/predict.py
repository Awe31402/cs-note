# predict.py：用真的程式的記憶體 trace 試第 14 支的三種預測器
#   用法：python3 predict.py work.lackey
#   trace 是 valgrind --tool=lackey --trace-mem=yes ./work 的輸出（work.c 在 ../lecture07/）
#   1. last address：猜「跟上次一樣的位址」
#   2. stride：猜「上次位址 + stride」。兩種更新規則：
#        課本 p.155：每次都 A=A'、S=S'，stride 一不對 P 就變 0
#        影片 [15:06]：P 換成 2-bit 飽和計數器，偶爾一次不對不改 stride
#   3. collision history table（CHT）：猜這個 load 會不會跟「最近 32 個 store」撞位址（2-bit 計數器）
#   表都用完整的 PC 當 key（不考慮 aliasing）
import sys
from collections import deque
last, st_b, st_v, cht = {}, {}, {}, {}
n = c_last = 0
b_pred = b_ok = v_pred = v_ok = 0
coll = c_pred_ok = c_miss = 0
stores = deque(maxlen=32); sset = {}
pc = None
def load(pc, a):
    global n, c_last, b_pred, b_ok, v_pred, v_ok, coll, c_pred_ok, c_miss
    n += 1
    c_last += last.get(pc) == a; last[pc] = a
    A, S, P = st_b.get(pc, (None, 0, 0))                 # 課本的規則
    if P: b_pred += 1; b_ok += A + S == a
    if A is not None: S2 = a - A; st_b[pc] = (a, S2, int(S2 == S))
    else: st_b[pc] = (a, 0, 0)
    A, S, C = st_v.get(pc, (None, 0, 0))                 # 影片的規則（C：0–3）
    if C >= 2: v_pred += 1; v_ok += A + S == a
    if A is None: st_v[pc] = (a, 0, 0)
    elif a - A == S: st_v[pc] = (a, S, min(C + 1, 3))
    else: st_v[pc] = (a, S if C >= 2 else a - A, max(C - 1, 0))
    hit = sset.get(a, 0) > 0                             # 真的撞了嗎
    k = cht.get(pc, 0); guess = k >= 2
    coll += hit; c_pred_ok += guess == hit; c_miss += hit and not guess
    cht[pc] = min(k + 1, 3) if hit else max(k - 1, 0)
def store(a):
    if len(stores) == stores.maxlen:
        old = stores[0]; sset[old] -= 1
    stores.append(a); sset[a] = sset.get(a, 0) + 1
for line in open(sys.argv[1]):
    if line.startswith('I'): pc = int(line[3:line.index(',')], 16)
    elif line.startswith(' L'): load(pc, int(line[3:line.index(',')], 16))
    elif line.startswith(' S'): store(int(line[3:line.index(',')], 16))
    elif line.startswith(' M'): a = int(line[3:line.index(',')], 16); load(pc, a); store(a)
pct = lambda a, b: f'{100 * a / b:5.1f}%'
print(f'load 共 {n} 個')
print(f'1. last address      ：猜對 {pct(c_last, n)}（每個 load 都猜）')
print(f'2. stride（課本規則）：有猜的 {pct(b_pred, n)}，猜的裡面對 {pct(b_ok, b_pred)}，等於全部 load 的 {pct(b_ok, n)}')
print(f'   stride（影片規則）：有猜的 {pct(v_pred, n)}，猜的裡面對 {pct(v_ok, v_pred)}，等於全部 load 的 {pct(v_ok, n)}')
print(f'3. 真的跟最近 32 個 store 撞到的 load：{pct(coll, n)}')
print(f'   CHT 猜對（撞／不撞）：{pct(c_pred_ok, n)}；猜「不撞」結果撞了（要 replay）：{pct(c_miss, n)}')
