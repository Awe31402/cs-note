# swp.py：單發射、照順序（in-order）的 pipeline，load 要 L 個 cycle、其他指令 1 cycle。
# 一條指令要等它的來源暫存器都好了才能發出（RAW stall）；分支假設都猜對、沒有代價。
# 比影片 [51:39] 的 a[i] = b[i] 迴圈的四種寫法：每個元素平均要幾個 cycle
def prog(kind):
    # 每條：(目的, [來源], 種類)；種類 'ld' 的延遲是 L
    L = lambda b, ad, dst: [(ad, ['r1', b], 'alu'), (dst, [ad], 'ld')]
    S = lambda o, ad, val: [(ad, ['r0', o], 'alu'), (None, [ad, val], 'st')]
    I = lambda it, o, k: [(it, [it], 'alu'), (o, [it], 'alu')]
    loop = [(None, ['r2'], 'alu'), (None, [], 'br'), (None, [], 'br')]           # cmp、beq、b
    if kind == '原本 LSI':
        return 1, loop[:2] + L('r10', 'r3', 'r5') + S('r10', 'r4', 'r5') + I('r2', 'r10', 1) + loop[2:]
    if kind == '只展開 3 次':
        body = []
        for _ in range(3): body += L('r10', 'r3', 'r5') + S('r10', 'r4', 'r5') + I('r2', 'r10', 1)
        return 3, loop[:2] + body + loop[2:]
    if kind == '影片的 software pipelining':   # I0 S1 L2 I1 S2 L3 I2 S3 L4，三組 (iterator, offset)
        body = (I('r6', 'r10', 3) + S('r11', 'r4', 'r5') + L('r12', 'r3', 'r5') +
                I('r7', 'r11', 3) + S('r12', 'r4', 'r5') + L('r10', 'r3', 'r5') +
                I('r8', 'r12', 3) + S('r10', 'r4', 'r5') + L('r11', 'r3', 'r5'))
        return 3, [(None, ['r6'], 'alu'), (None, [], 'br')] + body + loop[2:]
    if kind == '不展開、用 t 傳值':            # i++；a[i] = t；t = b[i+1]
        return 1, ([(None, ['r2'], 'alu'), (None, [], 'br'), ('r2', ['r2'], 'alu'), ('r10', ['r2'], 'alu')] +
                   S('r10', 'r4', 't') + [('r9', ['r10'], 'alu')] + L('r9', 'r3', 't') + loop[2:])

def cycles(kind, lat, reps=300):
    per, body = prog(kind)
    ready, t = {}, 0
    for _ in range(reps):
        for dst, src, k in body:
            t = max([t + 1] + [ready.get(s, 0) for s in src])
            if dst: ready[dst] = t + (lat if k == 'ld' else 1)
    return t / (reps * per)

kinds = ['原本 LSI', '只展開 3 次', '影片的 software pipelining', '不展開、用 t 傳值']
print('load 延遲 L：' + ''.join(f'{l:>7}' for l in (1, 2, 3, 4, 6)))
for k in kinds:
    print(f'{k:16}' + ''.join(f'{cycles(k, l):7.2f}' for l in (1, 2, 3, 4, 6)) + '  cycle/元素')
