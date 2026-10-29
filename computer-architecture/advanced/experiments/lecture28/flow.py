# flow.py：第 28 支的 flow control，用一個逐 cycle 的小模擬驗證影片的公式
#  1. 一條 link A → B 的 credit-based flow control：B 的 buffer 有 N 格；B 每個 cycle 往下送 1 個 flit（如果有）
#     一個 flit 傳過去要 tf 個 cycle、credit 傳回來要 tph 個 cycle、每個訊息處理要 tpr 個 cycle
#     credit round trip TD = tph + tf + 2·tpr。影片：N ≥ TD/tf 才不會讓 A 停下來
#  2. on/off flow control：n_off、n_on 兩個門檻，比較訊息數
#  3. 一條 K 個 hop 的路徑上送 L 個 flit：circuit switching、store and forward、virtual cut through 的延遲
import math

def credit_sim(N, tf, tph, tpr, T=20000):
    """回傳 A 平均每 cycle 送出幾個 flit（理想是 1/tf）、B 有沒有 overflow"""
    credits, buf, sent = N, 0, 0
    arrive, cred_back = [], []                 # (到達時間)
    busy_until = 0
    for t in range(T):
        buf += sum(1 for a in arrive if a == t); arrive = [a for a in arrive if a > t]
        credits += sum(1 for c in cred_back if c == t); cred_back = [c for c in cred_back if c > t]
        if buf > N: return None
        if buf > 0:                            # B 往下送一個，空出一格 → 送 credit 回 A
            buf -= 1; cred_back.append(t + tph + tpr)
        if credits > 0 and t >= busy_until:    # A 有 credit 就送（link 要 tf 個 cycle）
            credits -= 1; sent += 1; busy_until = t + tf
            arrive.append(t + tf + tpr)
    return sent / T * tf                       # 1.0 = link 跑滿

print('1. credit-based：tf = 2、tph = 1、tpr = 1 → TD = tph + tf + 2·tpr = 5，TD/tf = 2.5')
for N in (1, 2, 3, 4, 6):
    u = credit_sim(N, 2, 1, 1)
    print(f'   B 有 {N} 格 buffer：link 使用率 {u:.2f}' + ('（overflow！）' if u is None else ''))
print('   tf = 1、tph = 2、tpr = 2 → TD = 7，TD/tf = 7')
for N in (3, 5, 7, 8):
    print(f'   B 有 {N} 格 buffer：link 使用率 {credit_sim(N, 1, 2, 2):.2f}')

def onoff_sim(N, n_off, n_on, tf, tph, tpr, T=20000, drain_every=1):
    """B 剩下的空格 ≤ n_off 送 OFF，≥ n_on 送 ON。回傳（使用率、控制訊息數、有沒有 overflow）"""
    on, buf, sent, msgs, busy_until, state = True, 0, 0, 0, 0, 'on'
    arrive, ctrl = [], []
    for t in range(T):
        buf += sum(1 for a in arrive if a == t); arrive = [a for a in arrive if a > t]
        for (ct, v) in [c for c in ctrl if c[0] == t]: on = v
        ctrl = [c for c in ctrl if c[0] > t]
        if buf > N: return None, msgs, True
        if buf > 0 and t % drain_every == 0: buf -= 1
        free = N - buf
        if state == 'on' and free <= n_off: state = 'off'; msgs += 1; ctrl.append((t + tph + tpr, False))
        elif state == 'off' and free >= n_on: state = 'on'; msgs += 1; ctrl.append((t + tph + tpr, True))
        if on and t >= busy_until:
            sent += 1; busy_until = t + tf; arrive.append(t + tf + tpr)
    return sent / T * tf, msgs, False

print('2. on/off（tf = 1、tph = 1、tpr = 1 → TD = 4；B 每 2 個 cycle 才往下送 1 個，所以一定會塞住）')
for N, n_off, n_on in ((12, 4, 8), (12, 2, 8), (6, 4, 5), (20, 4, 16)):
    u, m, ov = onoff_sim(N, n_off, n_on, 1, 1, 1, drain_every=2)
    print(f'   N = {N:2d}、n_off = {n_off}、n_on = {n_on:2d}：' + ('overflow！' if ov else f'使用率 {u:.2f}（上限 0.50）、控制訊息 {m} 個'))
print('   B 完全不往下送（最壞情況）時，n_off 太小會不會 overflow？（影片：n_off ≥ TD/tf = 4）')
for n_off in (2, 3, 4, 5):
    u, m, ov = onoff_sim(12, n_off, 8, 1, 1, 1, T=200, drain_every=10 ** 9)
    print(f'   n_off = {n_off}：' + ('overflow！（掉 flit 了）' if ov else '沒有 overflow'))
print('   對照：credit-based 每送走一個 flit 就回一個 credit → 2 萬個 cycle 裡約 1 萬個 credit 訊息')

def path_latency(K, L, scheme):
    """K 個 hop、每 hop 1 cycle、每 cycle 1 個 flit；回傳最後一個 flit 到達的 cycle"""
    if scheme == 'circuit':                    # probe 走 K、回覆走 K，然後 pipeline 送 L 個 flit
        return 2 * K + (K + L - 1)
    if scheme == 'saf':                        # 每一 hop 都要收完整個 packet 才往下送
        t = 0
        for _ in range(K): t += L
        return t
    if scheme == 'vct':                        # 一個 flit 一到就往下送：逐 flit 模擬
        arrive = [0] * L                       # 每個 flit 在目前節點準備好的時間
        for _ in range(K):
            new, link_free = [], 0
            for f in range(L):
                start = max(arrive[f], link_free)
                new.append(start + 1); link_free = start + 1
            arrive = new
        return arrive[-1]

print('3. K 個 hop、L 個 flit（B = 1 flit/cycle）')
for K, L in ((3, 4), (5, 8), (8, 16)):
    c, s, v = (path_latency(K, L, x) for x in ('circuit', 'saf', 'vct'))
    print(f'   K = {K}、L = {L:2d}：circuit {c:3d}（3K + L − 1 = {3 * K + L - 1}）、store and forward {s:3d}（K·L = {K * L}）、'
          f'VCT {v:3d}（K + L − 1 = {K + L - 1}）')
