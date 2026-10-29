# select.py：（1）樹狀 select unit：一棵左邊優先、一棵右邊優先，會不會選到同一個？
#            （2）broadcast 的時間點：相依的兩條 add 中間隔幾個 cycle？
from itertools import product

def tree_select(req, prefer_left):
    """二元樹的 choice box：兩邊都要時，看 prefer_left 選左或右；回傳被 grant 的 index（或 None）"""
    nodes = [(i if r else None) for i, r in enumerate(req)]
    while len(nodes) > 1:
        nxt = []
        for a, b in zip(nodes[0::2], nodes[1::2]):
            nxt.append(a if b is None else b if a is None else (a if prefer_left else b))
        nodes = nxt
    return nodes[0]

for n in (4, 8, 16):
    bad = sum(1 for req in product((0, 1), repeat=n) if sum(req) >= 2 and tree_select(req, True) == tree_select(req, False))
    one = all(tree_select(req, True) == tree_select(req, False) for req in product((0, 1), repeat=n) if sum(req) == 1)
    print(f"{n:2d} 個 entry：{2 ** n} 種 request 組合，≥ 2 個 request 時兩棵樹選到同一個：{bad} 次；只有 1 個 request 時兩棵都選它：{one}")

# (2) pipeline：rename、dispatch、wakeup+select、register read、execute、write；consumer 最早在 producer broadcast 的那個 cycle wakeup+select
STAGES = ['RN', 'DP', 'WS', 'RR', 'EX', 'WB']
def gap(broadcast_stage, exec_cycles=1):
    p_ws = 2                                           # producer 在 cycle 2 wakeup+select
    p_at = {s: p_ws + i - 2 for i, s in enumerate(STAGES)}
    p_at['EX_end'] = p_at['EX'] + exec_cycles - 1
    b = p_at[broadcast_stage] if broadcast_stage != 'EX_k' else p_at['RR'] + exec_cycles - 1
    c_ws = b                                           # 同一個 cycle 收到 tag、wakeup、select
    c_ex = c_ws + 2                                    # 之後 RR、EX
    return c_ex - p_at['EX'] - exec_cycles             # 0 = 背靠背
for name, st in (("跟 write back 一起 broadcast", 'WB'), ("跟 execute 一起", 'EX'), ("跟 register read 一起（early broadcast）", 'RR')):
    print(f"{name:34s}：producer 做完到 consumer 開始執行，中間空 {gap(st)} 個 cycle")
print(f"{'load（執行 2 cycle），select 後 2 cycle 才 broadcast':34s}：中間空 {gap('EX_k', 2)} 個 cycle")
print(f"{'load 卻跟 add 一樣，select 後 1 cycle 就 broadcast':34s}：中間空 {gap('RR', 2)} 個 cycle（負的 = consumer 太早，拿不到值）")
