# microsim.py：課本 §9.6–9.8 的 microprogrammed SimpleRisc，只做一小部分指令
# 用法：python3 microsim.py [stack]   （stack：call／ret 改用課本 Example 132、133 的 stack 版）
import sys
from collections import Counter

ALU = {'add': lambda a, b: a + b, 'sub': lambda a, b: a - b}

# 微碼：(微指令, 運算元..., args)。b／beq／call／ret 照課本 p.397–398；ALU、mov、cmp 是我照同樣風格寫的
UCODE = {
 'pre':   [('label', '.begin'), ('mloadIR',), ('mdecode',), ('madd', 'pc', 4, None), ('mswitch',)],
 'b':     [('mmov', 'pc', 'branchTarget', None), ('mb', '.begin')],
 'beq':   [('mbeq', 'flags.E', 1, '.beq_t'), ('mb', '.begin'),
           ('label', '.beq_t'), ('mmov', 'pc', 'branchTarget', None), ('mb', '.begin')],
 'call':  [('mmov', 'regData', 'pc', None), ('mmovi', 'regSrc', 15, 'write'),
           ('mmov', 'pc', 'branchTarget', None), ('mb', '.begin')],
 'ret':   [('mmovi', 'regSrc', 15, 'read'), ('mmov', 'pc', 'regVal', None), ('mb', '.begin')],
 'callS': [('mmovi', 'regSrc', 14, 'read'), ('madd', 'regVal', -4, None), ('mmov', 'mar', 'regVal', None),
           ('mmov', 'regData', 'regVal', 'write'), ('mmov', 'mdr', 'pc', 'store'),
           ('mmov', 'pc', 'branchTarget', None), ('mb', '.begin')],
 'retS':  [('mmovi', 'regSrc', 14, 'read'), ('mmov', 'mar', 'regVal', 'load'), ('mmov', 'pc', 'ldResult', None),
           ('madd', 'regVal', 4, None), ('mmov', 'regData', 'regVal', 'write'), ('mb', '.begin')],
 'movi':  [('mmov', 'regData', 'immx', None), ('mmov', 'regSrc', 'rd', 'write'), ('mb', '.begin')],
 'addi':  [('mmov', 'regSrc', 'rs1', 'read'), ('mmov', 'A', 'regVal', None), ('mmov', 'B', 'immx', 'add'),
           ('mmov', 'regData', 'aluResult', None), ('mmov', 'regSrc', 'rd', 'write'), ('mb', '.begin')],
 'cmpi':  [('mmov', 'regSrc', 'rs1', 'read'), ('mmov', 'A', 'regVal', None), ('mmov', 'B', 'immx', 'cmp'),
           ('mb', '.begin')],
}

# 一個小程式：r1 從 0 數到 3，每次用 call 呼叫 .inc 加 1（PC 以 byte 計，每條 4 byte）
PROG = [('movi', 1, 0, 0), ('movi', 14, 0, 1000), ('cmpi', 0, 1, 3), ('beq', 0, 0, 24),
        ('call', 0, 0, 28), ('b', 0, 0, 8), ('halt', 0, 0, 0), ('addi', 1, 1, 1), ('ret', 0, 0, 0)]

def run(stack):
    sw = {'call': 'callS', 'ret': 'retS'} if stack else {}
    mem, upc_of, flat = {}, {}, []
    for name, seq in UCODE.items():            # 把所有微碼排進 microprogram memory
        upc_of[name] = len(flat)
        for u in seq:
            if u[0] == 'label': upc_of[u[1]] = len(flat)
            else: flat.append(u)
    R = Counter(); RF = Counter(); upc = 0; ucount = Counter(); dyn = Counter(); cur = None
    def args(a):
        if a == 'read': R['regVal'] = RF[R['regSrc']]
        elif a == 'write': RF[R['regSrc']] = R['regData']
        elif a == 'load': R['ldResult'] = mem.get(R['mar'], 0)
        elif a == 'store': mem[R['mar']] = R['mdr']
        elif a == 'cmp': R['flags.E'] = int(R['A'] == R['B'])
        elif a in ALU: R['aluResult'] = ALU[a](R['A'], R['B'])
    while True:
        u = flat[upc]; upc += 1; op = u[0]
        if op == 'mloadIR':
            ins = PROG[R['pc'] // 4]
            if ins[0] == 'halt': return RF, mem, ucount, dyn
            cur = ins[0]; dyn[cur] += 1
        ucount[cur] += 1
        if op == 'mloadIR': pass
        elif op == 'mdecode':
            R['rd'], R['rs1'], R['immx'] = ins[1], ins[2], ins[3]; R['branchTarget'] = ins[3]
        elif op == 'mswitch': upc = upc_of[sw.get(cur, cur)]
        elif op == 'mmov': R[u[1]] = R[u[2]]; args(u[3])
        elif op == 'mmovi': R[u[1]] = u[2]; args(u[3])
        elif op == 'madd': R[u[1]] += u[2]; args(u[3])
        elif op == 'mb': upc = upc_of[u[1]]
        elif op == 'mbeq':
            if R[u[1]] == u[2]: upc = upc_of[u[3]]

for stack in (False, True):
    RF, mem, uc, dyn = run(stack)
    print(f"{'stack 版 call/ret' if stack else '課本 call/ret（存在 r15）'}：r1 = {RF[1]}，r15 = {RF[15]}，"
          f"sp(r14) = {RF[14]}，記憶體 {dict(mem)}")
    print("   每條指令幾條微指令（含 4 條 preamble）：" +
          "，".join(f"{k} {uc[k] / dyn[k]:.1f}" for k in sorted(dyn)) +
          f"；總共 {sum(dyn.values())} 條指令、{sum(uc.values())} 條微指令")
