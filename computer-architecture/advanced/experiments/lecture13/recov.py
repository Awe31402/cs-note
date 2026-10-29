# recov.py：第 13 支的 4 種「猜錯分支後回到正確狀態」的方法，跑同一串隨機指令，看結果對不對、各要搬多少 bit
#   RRF ：commit 時把值寫進 retirement register file；回復時把 16 個值搬回去
#   RRAT：commit 時把對應寫進 retirement RAT；回復時把 16 個 7-bit 編號搬回去
#   SRAM：每個分支把整張 RAT 存一份（circular queue）；回復時直接用那份
#   CAM ：RAT 做成 128 格的 CAM（每格：架構暫存器編號＋valid bit），每個分支只存 128 個 valid bit
import random
NA, NP, W = 16, 128, 64                    # 架構暫存器、實體暫存器、值的寬度
TAG = (NP - 1).bit_length()                # 7 bit

def gen(n, seed):                          # 隨機程式：('alu', rd, rs1, rs2, k) 或 ('br', 會不會猜錯)
    r = random.Random(seed); prog = []
    for _ in range(n):
        if r.random() < 0.15: prog.append(('br', r.random() < 0.3))
        else: prog.append(('alu', r.randrange(NA), r.randrange(NA), r.randrange(NA), r.randrange(1000)))
    return prog

def alu(a, b, k): return (a * 3 + b + k) % (1 << 61)

def golden(prog):                          # 照順序執行，只執行正確路徑
    reg = list(range(NA))
    for ins in prog:
        if ins[0] == 'alu': _, d, s1, s2, k = ins; reg[d] = alu(reg[s1], reg[s2], k)
    return reg

def wrong_path(r, n):                      # 猜錯之後抓進來的錯誤指令
    return [('alu', r.randrange(NA), r.randrange(NA), r.randrange(NA), r.randrange(1000)) for _ in range(n)]

def run(prog, scheme, seed, early=False):     # early：rename 時就釋放舊的對應（錯的做法）
    r = random.Random(seed)
    prf = [0] * NP; prf[:NA] = range(NA)
    rat = list(range(NA)); free = list(range(NA, NP))
    rrf = list(range(NA)); rrat = list(range(NA))
    cam_arch = [None] * NP; cam_valid = [0] * NP
    for a in range(NA): cam_arch[a] = a; cam_valid[a] = 1
    moved = per_commit = 0; inv_bad = 0
    def lookup(a):
        if scheme == 'CAM':
            hit = [p for p in range(NP) if cam_arch[p] == a and cam_valid[p]]
            assert len(hit) == 1; return hit[0]
        return rat[a]
    def rename(ins):                       # 回傳 ROB entry
        _, d, s1, s2, k = ins
        p1, p2 = lookup(s1), lookup(s2)
        old = lookup(d); new = free.pop(0)
        if early: free.append(old)
        if scheme == 'CAM': cam_valid[old] = 0; cam_arch[new] = d; cam_valid[new] = 1
        else: rat[d] = new
        prf[new] = alu(prf[p1], prf[p2], k)      # 這裡直接算好（執行順序不影響回復對不對）
        return dict(d=d, new=new, old=old, val=prf[new])
    def commit(e):
        nonlocal per_commit
        if not early: free.append(e['old'])      # 第 13 支 §4：commit 時釋放「舊的」對應
        if scheme == 'RRF': rrf[e['d']] = e['val']; per_commit += W
        if scheme == 'RRAT': rrat[e['d']] = e['new']; per_commit += TAG
    for ins in prog:
        if ins[0] == 'alu': commit(rename(ins)); continue
        if not ins[1]: continue                  # 猜對的分支：什麼都不用做
        # 猜錯的分支：先存 checkpoint，再抓幾條錯誤路徑的指令進 ROB（還沒 commit）
        ck_rat, ck_valid = rat[:], cam_valid[:]
        if scheme == 'SRAM': moved += NA * TAG
        if scheme == 'CAM': moved += NP
        rob = [rename(x) for x in wrong_path(r, r.randrange(1, 40))]
        if scheme == 'CAM': inv_bad += sum(cam_valid) != NA
        # 分支到了 ROB 頭：flush。錯誤路徑的實體暫存器還給 free list
        for e in rob: free.append(e['new'])
        if scheme == 'RRF':
            for a in range(NA): rat[a] = a; prf[a] = rrf[a]
            free[:] = list(range(NA, NP)); moved += NA * W
        elif scheme == 'RRAT': rat[:] = rrat; moved += NA * TAG
        elif scheme == 'SRAM': rat[:] = ck_rat
        else: cam_valid[:] = ck_valid
        if scheme == 'CAM': inv_bad += sum(cam_valid) != NA
    return [prf[lookup(a)] for a in range(NA)], moved, per_commit, inv_bad

prog = gen(20000, 1)
nmis = sum(1 for x in prog if x[0] == 'br' and x[1]); ncom = sum(1 for x in prog if x[0] == 'alu')
want = golden(prog)
print(f'程式：{len(prog)} 條，其中猜錯的分支 {nmis} 個、commit 的 ALU 指令 {ncom} 條')
print(f'{"方法":6}{"結果對嗎":>8}{"回復＋存 checkpoint 共搬幾 bit":>22}{"每個猜錯平均":>12}{"commit 時多寫幾 bit":>18}')
for s in ('RRF', 'RRAT', 'SRAM', 'CAM'):
    got, moved, pc, bad = run(prog, s, 2)
    extra = f'（valid bit 不是剛好 16 個的次數：{bad}）' if s == 'CAM' else ''
    print(f'{s:6}{str(got == want):>8}{moved:>22}{moved / nmis:>14.0f}{pc:>18}{extra}')
print('對照：rename 時就把舊的實體暫存器還給 free list（不等 commit）：')
for s in ('RRAT', 'SRAM'):
    got, *_ = run(prog, s, 2, early=True)
    print(f'{s:6}結果對嗎：{got == want}，16 個架構暫存器裡錯了 {sum(g != w for g, w in zip(got, want))} 個')
