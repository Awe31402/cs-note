# microsim28.py：課本 §9.6.3–9.6.7 的微碼（照抄），在 19 個微暫存器、shared bus 的模型上跑一個 SimpleRisc 小程式
from collections import Counter

UCODE = {   # (微指令, 運算元…, args)；label 用 ('label', 名字)
 'pre': [('label', '.begin'), ('mloadIR',), ('mdecode',), ('madd', 'pc', 4, None), ('mswitch',)],
 'add': [('mmov', 'regSrc', 'rs1', 'read'), ('mmov', 'A', 'regVal', None),           # 3-address ALU（p.392）
         ('mbeq', 'I', 1, '.add_imm'),
         ('mmov', 'regSrc', 'rs2', 'read'), ('mmov', 'B', 'regVal', 'add'), ('mb', '.add_rw'),
         ('label', '.add_imm'), ('mmov', 'B', 'immx', 'add'),
         ('label', '.add_rw'), ('mmov', 'regSrc', 'rd', None), ('mmov', 'regData', 'aluResult', 'write'),
         ('mb', '.begin')],
 'mov': [('mbeq', 'I', 1, '.mov_imm'),                                                 # p.393
         ('mmov', 'regSrc', 'rs2', 'read'), ('mmov', 'regData', 'regVal', None), ('mb', '.mov_rw'),
         ('label', '.mov_imm'), ('mmov', 'regData', 'immx', None),
         ('label', '.mov_rw'), ('mmov', 'regSrc', 'rd', 'write'), ('mb', '.begin')],
 'not': [('mbeq', 'I', 1, '.not_imm'),                                                 # p.394
         ('mmov', 'regSrc', 'rs2', 'read'), ('mmov', 'B', 'regVal', 'not'), ('mb', '.not_rw'),
         ('label', '.not_imm'), ('mmov', 'B', 'immx', 'not'),
         ('label', '.not_rw'), ('mmov', 'regData', 'aluResult', None), ('mmov', 'regSrc', 'rd', 'write'),
         ('mb', '.begin')],
 'cmp': [('mmov', 'regSrc', 'rs1', 'read'), ('mmov', 'A', 'regVal', None),             # p.394
         ('mbeq', 'I', 1, '.cmp_imm'),
         ('mmov', 'regSrc', 'rs2', 'read'), ('mmov', 'B', 'regVal', 'cmp'), ('mb', '.begin'),
         ('label', '.cmp_imm'), ('mmov', 'B', 'immx', 'cmp'), ('mb', '.begin')],
 'nop': [('mb', '.begin')],                                                            # p.395
 'ld':  [('mmov', 'regSrc', 'rs1', 'read'), ('mmov', 'A', 'regVal', None),             # p.395
         ('mmov', 'B', 'immx', 'add'), ('mmov', 'mar', 'aluResult', 'load'),
         ('mmov', 'regData', 'ldResult', None), ('mmov', 'regSrc', 'rd', 'write'), ('mb', '.begin')],
 'st':  [('mmov', 'regSrc', 'rs1', 'read'), ('mmov', 'A', 'regVal', None),             # p.396
         ('mmov', 'B', 'immx', 'add'), ('mmov', 'mar', 'aluResult', None),
         ('mmov', 'regSrc', 'rd', 'read'), ('mmov', 'mdr', 'regVal', 'store'), ('mb', '.begin')],
 'b':   [('mmov', 'pc', 'branchTarget', None), ('mb', '.begin')],                       # 第 29 支（p.397）
 'beq': [('mbeq', 'flags.E', 1, '.beq_t'), ('mb', '.begin'),
         ('label', '.beq_t'), ('mmov', 'pc', 'branchTarget', None), ('mb', '.begin')],
}

# 程式：(op, rd, rs1, rs2 或 imm, I)。把 1 + 2 + … + 10 的每一步部分和存進記憶體 200、204…，最後讀回最後一個
PROG = [('mov', 1, 0, 0, 1),      #  0: mov r1, 0        sum
        ('mov', 2, 0, 1, 1),      #  4: mov r2, 1        i
        ('mov', 4, 0, 200, 1),    #  8: mov r4, 200      位址
        ('add', 1, 1, 2, 0),      # 12: add r1, r1, r2   ← 暫存器版
        ('st',  1, 4, 0, 1),      # 16: st r1, 0[r4]
        ('add', 4, 4, 4, 1),      # 20: add r4, r4, 4    ← immediate 版
        ('add', 2, 2, 1, 1),      # 24: add r2, r2, 1
        ('cmp', 0, 2, 11, 1),     # 28: cmp r2, 11
        ('beq', 0, 0, 40, 1),     # 32: beq .done
        ('b',   0, 0, 12, 1),     # 36: b .loop
        ('ld',  5, 4, -4, 1),     # 40: ld r5, -4[r4]
        ('not', 6, 0, 5, 0),      # 44: not r6, r5
        ('nop', 0, 0, 0, 0),      # 48: nop
        ('halt', 0, 0, 0, 0)]

flat, upc_of = [], {}
for name, seq in UCODE.items():
    upc_of[name] = len(flat)
    for u in seq:
        if u[0] == 'label': upc_of[u[1]] = len(flat)
        else: flat.append(u)

R, RF, mem = Counter(), Counter(), {}
def bus_args(a):     # 誰擁有被寫的微暫存器，誰就處理 args（影片 [33:42]）
    if a == 'read': R['regVal'] = RF[R['regSrc']]
    elif a == 'write': RF[R['regSrc']] = R['regData']
    elif a == 'load': R['ldResult'] = mem.get(R['mar'], 0)
    elif a == 'store': mem[R['mar']] = R['mdr']
    elif a == 'add': R['aluResult'] = R['A'] + R['B']
    elif a == 'not': R['aluResult'] = ~R['B']
    elif a == 'cmp': R['flags.E'], R['flags.GT'] = int(R['A'] == R['B']), int(R['A'] > R['B'])

upc, cur, per, dyn, path = 0, None, Counter(), Counter(), Counter()
while True:
    u = flat[upc]; upc += 1; op = u[0]
    if op == 'mloadIR':
        ins = PROG[R['pc'] // 4]
        if ins[0] == 'halt': break
        cur = (ins[0], 'imm' if ins[4] else 'reg') if ins[0] in ('add', 'mov', 'not', 'cmp') else (ins[0], ''); dyn[cur] += 1
    per[cur] += 1
    if op == 'mdecode':
        R['rd'], R['rs1'], R['rs2'], R['I'] = ins[1], ins[2], ins[3], ins[4]
        R['immx'] = ins[3]; R['branchTarget'] = ins[3]
    elif op == 'mswitch': upc = upc_of[ins[0]]
    elif op == 'mmov': R[u[1]] = R[u[2]]; bus_args(u[3])
    elif op == 'madd': R[u[1]] += u[2]
    elif op == 'mb': upc = upc_of[u[1]]
    elif op == 'mbeq' and R[u[1]] == u[2]: upc = upc_of[u[3]]

print("暫存器：", {f"r{i}": RF[i] for i in (1, 2, 4, 5, 6)})
print("記憶體 200–236：", [mem.get(a) for a in range(200, 240, 4)])
print("每條指令幾條微指令（含 4 條 preamble）：")
for k in sorted(dyn): print(f"  {k[0]:4s}{('（' + k[1] + '）') if k[1] else '':6s} {per[k] / dyn[k]:4.1f}   × {dyn[k]} 次")
tot_u, tot_i = sum(per.values()), sum(dyn.values())
print(f"總共 {tot_i} 條 SimpleRisc 指令、{tot_u} 條微指令，平均每條 {tot_u / tot_i:.1f} 條")
