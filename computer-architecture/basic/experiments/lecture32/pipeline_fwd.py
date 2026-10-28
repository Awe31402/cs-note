"""五級 pipeline 加上 interlock（data-lock、branch-lock）與 forwarding（課本 Fig 10.30–10.31）。
mode=fwd：4 條 forwarding path（RW→OF、RW→EX、MA→EX、RW→MA），只有 load-use 要停 1 個週期
mode=lock：沒有 forwarding，只要和 EX/MA/RW 裡的指令有 RAW 就停（課本 §10.6）
用法：python3 -I pipeline_fwd.py prog.s [fwd|lock] [r0=值 ...]"""
import sys, os
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path[:0] = [os.path.join(HERE, '..', 'lecture27'), os.path.join(HERE, '..', 'simplerisc')]
from datapath import control, bits, s32, M32
from srasm import assemble

NAMES = 'add sub mul div mod cmp and or not mov lsl lsr asr nop ld st beq bgt b call ret'.split()
def op(p): return NAMES[bits(p['inst'], 28, 32)]

def conflict1(a, b):
    """Algorithm 6：[B] → [A] 在第一個運算元（rs1／ra）上有沒有 RAW"""
    if a is None or b is None: return False
    if op(a) in ('nop', 'b', 'beq', 'bgt', 'call', 'not', 'mov'): return False
    if op(b) in ('nop', 'cmp', 'st', 'b', 'beq', 'bgt', 'ret'): return False
    src1 = 15 if op(a) == 'ret' else bits(a['inst'], 19, 22)
    dest = 15 if op(b) == 'call' else bits(b['inst'], 23, 26)
    return src1 == dest

def conflict2(a, b):
    """Algorithm 7：第二個運算元（rs2；st 是 rd）"""
    if a is None or b is None: return False
    if op(a) in ('nop', 'b', 'beq', 'bgt', 'call'): return False
    if op(b) in ('nop', 'cmp', 'st', 'b', 'beq', 'bgt', 'ret'): return False
    if op(a) != 'st' and bits(a['inst'], 27, 27) == 1: return False
    src2 = bits(a['inst'], 23, 26) if op(a) == 'st' else bits(a['inst'], 15, 18)
    dest = 15 if op(b) == 'call' else bits(b['inst'], 23, 26)
    return src2 == dest

def wbval(p):
    c = p['c']; return s32(p['pc'] + 4) if c['isCall'] else p['ldResult'] if c['isLd'] else p['aluResult']

class Pipeline:
    def __init__(self, src, mode='fwd'):
        prog = assemble(src); self.mode = mode
        self.imem = {pc: w for pc, _, w in prog}
        self.reg = [0] * 16; self.reg[14] = 0x10000
        self.E = self.GT = 0; self.dmem = {}; self.pc = 0
        self.if_of = self.of_ex = self.ex_ma = self.ma_rw = None
        self.stalls = self.flushed = self.retired = 0
        self.used = {k: 0 for k in ('RW->OF', 'RW->EX', 'MA->EX', 'RW->MA')}

    def cycle(self):
        p_of, p_ex, p_ma, p_rw = self.if_of, self.of_ex, self.ex_ma, self.ma_rw
        # RW
        wb = None
        if p_rw:
            self.retired += op(p_rw) != 'nop'
            if p_rw['c']['isWb']: wb = (15 if p_rw['c']['isCall'] else bits(p_rw['inst'], 23, 26), wbval(p_rw))
        fw = self.mode == 'fwd'
        # MA：M6 選 st 要存的值
        n_rw = None
        if p_ma:
            n_rw = dict(p_ma, ldResult=0); c = p_ma['c']; a = p_ma['aluResult']; v = p_ma['op2']
            if fw and c['isSt'] and conflict2(p_ma, p_rw): v = wbval(p_rw); self.used['RW->MA'] += 1
            if c['isLd']: n_rw['ldResult'] = s32(sum(self.dmem.get((a + i) & M32, 0) << (8 * i) for i in range(4)))
            if c['isSt']:
                for i in range(4): self.dmem[(a + i) & M32] = (v >> (8 * i)) & 0xFF
        # EX：M3、M4（MA 比 RW 優先，因為 MA 裡的指令比較新）、M5
        n_ma = None; taken = branchPC = 0
        if p_ex:
            c, A, B, o2 = p_ex['c'], p_ex['A'], p_ex['B'], p_ex['op2']
            if fw:
                if conflict1(p_ex, p_ma): A = p_ma['aluResult']; self.used['MA->EX'] += 1
                elif conflict1(p_ex, p_rw): A = wbval(p_rw); self.used['RW->EX'] += 1
                if not c['isSt']:
                    if conflict2(p_ex, p_ma): B = p_ma['aluResult']; self.used['MA->EX'] += 1
                    elif conflict2(p_ex, p_rw): B = wbval(p_rw); self.used['RW->EX'] += 1
                elif conflict2(p_ex, p_rw): o2 = wbval(p_rw); self.used['RW->EX'] += 1
            r = 0
            if c['isAdd']: r = A + B
            elif c['isSub']: r = A - B
            elif c['isCmp']: self.E, self.GT = int(A == B), int(A > B)
            elif c['isMul']: r = A * B
            elif c['isDiv']: r = int(A / B)
            elif c['isMod']: r = A - int(A / B) * B
            elif c['isLsl']: r = A << (B & 31)
            elif c['isLsr']: r = (A & M32) >> (B & 31)
            elif c['isAsr']: r = A >> (B & 31)
            elif c['isOr']: r = A | B
            elif c['isAnd']: r = A & B
            elif c['isNot']: r = ~B
            elif c['isMov']: r = B
            n_ma = dict(p_ex, aluResult=s32(r), op2=o2)
            branchPC = A & M32 if c['isRet'] else p_ex['branchTarget']
            taken = (c['isBeq'] & self.E) | (c['isBgt'] & self.GT) | c['isUBranch']
        # OF：M1、M2（只有 RW→OF）
        n_ex = None
        if p_of:
            inst, pc = p_of['inst'], p_of['pc']; c = control(bits(inst, 28, 32), bits(inst, 27, 27))
            imm, mod = bits(inst, 1, 16), bits(inst, 17, 18)
            immx = imm << 16 if mod == 2 else imm if mod == 1 else s32(imm << 16) >> 16
            off = bits(inst, 1, 27) << 2
            op1 = self.reg[15 if c['isRet'] else bits(inst, 19, 22)]
            op2 = self.reg[bits(inst, 23, 26) if c['isSt'] else bits(inst, 15, 18)]
            pk = dict(pc=pc, inst=inst)
            if fw and conflict1(pk, p_rw): op1 = wbval(p_rw); self.used['RW->OF'] += 1
            if fw and conflict2(pk, p_rw): op2 = wbval(p_rw); self.used['RW->OF'] += 1
            n_ex = dict(pk, c=c, A=op1, B=immx if c['isImmediate'] else op2, op2=op2,
                        branchTarget=(pc + (off - (1 << 29) if off >> 28 else off)) & M32)
        # data-lock：要不要讓 OF 裡的指令停一個週期
        if fw:   # 只剩 load-use；ld → st 的存值可以 RW→MA，不用停
            stall = p_ex is not None and p_ex['c']['isLd'] and n_ex is not None and (
                conflict1(n_ex, p_ex) or (conflict2(n_ex, p_ex) and not n_ex['c']['isSt']))
        else:    # 沒有 forwarding：和 EX、MA、RW 任何一條有 RAW 就停
            stall = n_ex is not None and any(conflict1(n_ex, q) or conflict2(n_ex, q) for q in (p_ex, p_ma, p_rw))
        n_of = dict(pc=self.pc, inst=self.imem[self.pc]) if self.pc in self.imem else None
        if wb: self.reg[wb[0]] = wb[1]
        if taken:                      # branch-lock：IF、OF 裡的兩條變成 bubble
            self.flushed += (n_of is not None) + (n_ex is not None)
            self.if_of, self.of_ex, self.pc = None, None, branchPC
        elif stall:                    # IF、OF 不動，往 EX 送一個 bubble
            self.stalls += 1; self.of_ex = None
        else:
            self.if_of, self.of_ex, self.pc = n_of, n_ex, self.pc + 4
        self.ex_ma, self.ma_rw = n_ma, n_rw

    def run(self, max_cycles=10**6):
        n = 0
        while (self.pc in self.imem or self.if_of or self.of_ex or self.ex_ma or self.ma_rw) and n < max_cycles:
            self.cycle(); n += 1
        return n

if __name__ == '__main__':
    args = sys.argv[2:]; mode = 'fwd'
    if args and args[0] in ('fwd', 'lock'): mode = args.pop(0)
    p = Pipeline(open(sys.argv[1]).read(), mode)
    for kv in args:
        k, v = kv.split('='); p.reg[int(k[1:])] = int(v, 0)
    n = p.run()
    print(f'{os.path.basename(sys.argv[1])} [{mode}]: {n} cycles, {p.retired} insts, CPI={n / p.retired:.3f}, '
          f'stalls={p.stalls}, flushed={p.flushed};', ' '.join(f'r{i}={p.reg[i]}' for i in range(14) if p.reg[i]),
          f'| E={p.E} GT={p.GT} | sp=0x{p.reg[14]:X}')
    if mode == 'fwd': print('   forwarding used:', p.used)
