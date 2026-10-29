# replay.py：第 15 支的四種「猜錯後重做」做法，數一數各要重做幾條、會不會有 orphan
#   模型：一個 load（第 0 條）猜會在 L1 命中，提早 broadcast；後面 K 條指令，每條有 2 個來源，
#        每個來源有一半機會來自前面 8 條裡的某一條，不然就是早就好了的暫存器。
#        每個 cycle 最多選 W 條（最老的先），1 cycle 的 ALU，被選中就 broadcast（背靠背）。
#        cycle N 時發現 load 沒命中（window of vulnerability 結束），要重做。
#   比：flush（全部重來）、non-selective（WV 裡醒過的運算元都殺）、delayed selective（加 poison bit）、token（只動 forward slice）
import random
K, W, N, T = 64, 4, 4, 2000

def window(r):
    src = []
    for i in range(1, K + 1):
        s = []
        for _ in range(2):
            s.append(r.randrange(max(0, i - 8), i) if r.random() < 0.5 else None)
        src.append(s)
    return src                                  # src[i-1] 是第 i 條的兩個來源（None = 早就好了）

def one(r):
    src = window(r)
    ready_at = {0: 1}                           # 第 0 條（load）在 cycle 1 broadcast（猜命中）
    wake = {}                                   # (指令, 第幾個來源) → 那個運算元醒來的 cycle
    issued = {}
    for c in range(1, N + 1):                   # 跑到 cycle N
        cand = []
        for i in range(1, K + 1):
            if i in issued: continue
            ok = True
            for k, p in enumerate(src[i - 1]):
                t = 0 if p is None else ready_at.get(p)
                if t is None or t > c: ok = False
                elif p is not None: wake.setdefault((i, k), t)
            if ok: cand.append(i)
        for i in cand[:W]:
            issued[i] = c; ready_at[i] = c + 1
    # forward slice：直接或間接用到 load 的指令
    fs = set()
    for i in range(1, K + 1):
        if any(p == 0 or p in fs for p in src[i - 1] if p is not None): fs.add(i)
    # non-selective：運算元在 WV（cycle 1..N）裡醒過的指令都被殺
    killed = {i for (i, k), t in wake.items() if 1 <= t <= N}
    orphan_ns = 0
    for i in killed:
        for k, p in enumerate(src[i - 1]):
            if (i, k) in wake and 1 <= wake[(i, k)] <= N and p != 0 and p not in killed:
                orphan_ns += 1; break          # 殺掉的運算元，它的 producer 不會再 broadcast 了
    # delayed selective：已 issue、沒有 poison 的不用重做；還沒 issue 的照樣被殺
    killed_ds = {i for i in killed if i not in issued or i in fs}
    orphan_ds = sum(1 for i in killed_ds if i not in fs and i not in issued)   # 靠 completion bus 救
    return dict(
        flush=len(issued),                      # 已經做了、要丟掉的
        ns=len(killed & set(issued)), ns_sq=len(killed), ns_or=orphan_ns,
        ds=len(killed_ds & set(issued)), ds_or=orphan_ds,
        tok=len(fs & set(issued)), fs=len(fs))

r = random.Random(1)
tot = {}
for _ in range(T):
    for k, v in one(r).items(): tot[k] = tot.get(k, 0) + v
a = {k: v / T for k, v in tot.items()}
print(f'K={K} 條、每 cycle 選 W={W} 條、WV = {N} cycle，平均 {T} 次：')
print(f'  到 cycle {N} 為止已經 issue 的：{a["flush"]:.1f} 條；其中真的在 load 的 forward slice 裡的：{a["tok"]:.1f} 條（整個 forward slice 共 {a["fs"]:.1f} 條）')
print(f'  {"做法":22}{"已做、要重做的":>10}{"orphan":>10}')
print(f'  {"flush（全部重來）":20}{a["flush"]:>12.1f}{"0":>10}（而且 K 條全部要重新 fetch）')
print(f'  {"non-selective":22}{a["ns"]:>12.1f}{a["ns_or"]:>10.1f}（被殺的運算元所屬指令共 {a["ns_sq"]:.1f} 條）')
print(f'  {"delayed selective":22}{a["ds"]:>12.1f}{a["ds_or"]:>10.1f}（orphan 靠 completion bus 救回）')
print(f'  {"token":22}{a["tok"]:>12.1f}{"0":>10}')
