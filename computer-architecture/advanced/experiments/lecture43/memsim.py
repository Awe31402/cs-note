# memsim.py：照課本表 10.12（DDR-1600，單位 bus cycle）的時序，模擬一個只處理讀的 memory controller
#   比較課本 §10.4.2 的 address mapping（左邊是最高位）× open／close page 在不同存取樣式下的表現
#   簡化：每個 channel 的命令照請求順序發（不重排）；每個命令最早能發的時間 = 所有時序限制的最大值
import random
T = dict(tRCD=10, tRP=10, tRAS=28, CL=10, tCCDS=4, tCCDL=5, tRRDS=4, tRRDL=5, tFAW=20, tRTP=6, BL2=4)
BITS = dict(c=1, k=1, g=2, b=2, r=14, l=7)      # 2 channel、2 rank、4 bank group × 4 bank、每個 row 128 個 64 B block
def decode(blk, scheme):                          # scheme 例如 'r:k:b:g:l:c'，最右邊用最低的 bit
    out = {}
    for f in reversed(scheme.split(':')):
        n = BITS[f]; out[f] = blk & ((1 << n) - 1); blk >>= n
    return out
def run(addrs, scheme, policy):
    ch = {}                                           # 每個 channel 的狀態
    hits = 0; finish = 0
    for blk in addrs:
        a = decode(blk, scheme)
        s = ch.setdefault(a['c'], dict(cmd=0, data=0, last_rank=None, acts=[], last_act={}, last_rd={}, bank={}))
        key = (a['k'], a['g'], a['b'])
        bk = s['bank'].setdefault(key, dict(row=None, act=-99, pre_ok=0, ready=0))
        t = s['cmd']                                  # 命令照順序發：不能早於上一個命令
        if policy == 'open' and bk['row'] == a['r']:
            hits += 1
        else:
            if bk['row'] is not None:                 # open page 的 row miss：要先 PRE
                t = max(t, bk['pre_ok']); t += 1     # PRE 佔一個命令 cycle
                pre = t; t = pre + T['tRP']
            t = max(t, bk['ready'])                   # close page：自動 precharge 完才行
            for (kk, gg), ta in s['last_act'].items():   # tRRD：跟同 rank 上一次 ACT 的距離
                if kk == a['k']: t = max(t, ta + (T['tRRDL'] if gg == a['g'] else T['tRRDS']))
            acts = [x for x in s['acts'] if x[0] == a['k']][-4:]
            if len(acts) == 4: t = max(t, acts[0][1] + T['tFAW'])   # tFAW：同 rank 任 4 次 ACT 的視窗
            act = t; s['acts'].append((a['k'], act)); s['last_act'][(a['k'], a['g'])] = act
            bk['row'], bk['act'] = a['r'], act
            t = act + T['tRCD']
        for (kk, gg), tr in s['last_rd'].items():     # tCCD：跟上一個 READ 的距離
            if kk == a['k']: t = max(t, tr + (T['tCCDL'] if gg == a['g'] else T['tCCDS']))
        data = max(t + T['CL'], s['data'] + (1 if s['last_rank'] not in (None, a['k']) else 0))
        rd = data - T['CL']                           # 資料 bus 被佔用的話 READ 也要晚一點發
        s['last_rd'][(a['k'], a['g'])] = rd; s['cmd'] = rd + 1
        s['data'] = data + T['BL2']; s['last_rank'] = a['k']
        finish = max(finish, s['data'])
        if policy == 'open':
            bk['pre_ok'] = max(bk['act'] + T['tRAS'], rd + T['tRTP'])
        else:                                         # READA：讀完自動 precharge
            pre = max(bk['act'] + T['tRAS'], rd + T['tRTP']); bk['ready'] = pre + T['tRP']; bk['row'] = None
    return finish, hits / len(addrs)
N = 4000; random.seed(1); top = 1 << sum(BITS.values())
pats = {
    '循序（連續的 64 B block）': list(range(N)),
    '隨機': [random.randrange(top) for _ in range(N)],
    '兩條循序的 stream 交錯': [x for i in range(N // 2) for x in (i, (1 << 22) + i)],
    '每次跳 8 KB': [i * 128 for i in range(N)],
}
schemes = ['r:k:b:g:l:c', 'r:k:b:l:g:c', 'r:l:k:b:g:c', 'c:k:g:b:r:l']
print('每個格子：GB/s（bus 800 MHz，每個 block 64 B）／row buffer hit 率')
print(f"{'':28}" + ''.join(f'{s + " " + p:>24}' for s in schemes for p in ('open', 'close')))
for nm, ad in pats.items():
    row = []
    for s in schemes:
        for p in ('open', 'close'):
            cyc, h = run(ad, s, p)
            row.append(f'{N * 64 / (cyc * 1.25):8.2f}／{h * 100:3.0f}%')
    print(f'{nm:28}' + ''.join(f'{x:>24}' for x in row))
print('（兩個 channel 全速的理論上限：2 × 64 B／（4 cycle × 1.25 ns）=', 2 * 64 / (4 * 1.25), 'GB/s）')
