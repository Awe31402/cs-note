"""課本 Fig 10.9 的五級 pipeline（沒有 interlock、沒有 forwarding）：
每個週期 IF、OF、EX、MA、RW 各處理一條指令，指令封包（instruction packet）在 4 個 pipeline register 之間移動。
用法：python3 -I pipeline.py prog.s [r0=值 ...]   會印出 pipeline diagram 和最後的暫存器"""
import sys, os
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path[:0] = [os.path.join(HERE, '..', 'lecture27'), os.path.join(HERE, '..', 'simplerisc')]
from datapath import control, bits, s32, M32
from srasm import assemble

class Pipeline:
    def __init__(self, src):
        prog = assemble(src)
        self.imem = {pc: w for pc, _, w in prog}; self.text = {pc: l for pc, l, _ in prog}
        self.reg = [0] * 16; self.reg[14] = 0x10000
        self.E = self.GT = 0; self.dmem = {}; self.pc = 0
        self.if_of = self.of_ex = self.ex_ma = self.ma_rw = None     # 4 個 pipeline register
        self.diagram = []                                            # 每個週期：(IF, OF, EX, MA, RW) 的 pc

    def cycle(self):
        p_if, p_of, p_ex, p_ma = self.if_of, self.of_ex, self.ex_ma, self.ma_rw
        self.diagram.append(tuple(p['pc'] if p else None for p in
                                  ({'pc': self.pc} if self.pc in self.imem else None, p_if, p_of, p_ex, p_ma)))
        # RW：結果在週期結束（負緣）才寫進暫存器檔，所以同一週期的 OF 讀到的是舊值
        wb = None
        if p_ma and p_ma['c']['isWb']:
            c = p_ma['c']
            wb = (15 if c['isCall'] else bits(p_ma['inst'], 23, 26),
                  s32(p_ma['pc'] + 4) if c['isCall'] else p_ma['ldResult'] if c['isLd'] else p_ma['aluResult'])
        # MA
        n_rw = None
        if p_ex:
            n_rw = dict(p_ex); c = p_ex['c']; a = p_ex['aluResult']; n_rw['ldResult'] = 0
            if c['isLd']: n_rw['ldResult'] = s32(sum(self.dmem.get((a + i) & M32, 0) << (8 * i) for i in range(4)))
            if c['isSt']:
                for i in range(4): self.dmem[(a + i) & M32] = (p_ex['op2'] >> (8 * i)) & 0xFF
        # EX：branch unit 的結果直接送回 IF（不經過 pipeline register）
        n_ma = None; taken, branchPC = 0, 0
        if p_of:
            c, A, B = p_of['c'], p_of['A'], p_of['B']; r = 0
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
            n_ma = dict(p_of, aluResult=s32(r))
            branchPC = A & M32 if c['isRet'] else p_of['branchTarget']
            taken = (c['isBeq'] & self.E) | (c['isBgt'] & self.GT) | c['isUBranch']
        # OF：解碼、讀暫存器、算 immx 與 branchTarget，控制訊號跟著封包走
        n_ex = None
        if p_if:
            inst, pc = p_if['inst'], p_if['pc']; c = control(bits(inst, 28, 32), bits(inst, 27, 27))
            imm, mod = bits(inst, 1, 16), bits(inst, 17, 18)
            immx = imm << 16 if mod == 2 else imm if mod == 1 else s32(imm << 16) >> 16
            off = bits(inst, 1, 27) << 2
            op1 = self.reg[15 if c['isRet'] else bits(inst, 19, 22)]
            op2 = self.reg[bits(inst, 23, 26) if c['isSt'] else bits(inst, 15, 18)]
            n_ex = dict(pc=pc, inst=inst, c=c, A=op1, B=immx if c['isImmediate'] else op2, op2=op2,
                        branchTarget=(pc + (off - (1 << 29) if off >> 28 else off)) & M32)
        # IF
        n_of = dict(pc=self.pc, inst=self.imem[self.pc]) if self.pc in self.imem else None
        # 負緣：所有 pipeline register 同時更新、暫存器檔寫入、pc 更新
        if wb: self.reg[wb[0]] = wb[1]
        self.if_of, self.of_ex, self.ex_ma, self.ma_rw = n_of, n_ex, n_ma, n_rw
        self.pc = branchPC if taken else self.pc + 4

    def run(self, max_cycles=10**5):
        n = 0
        while (self.pc in self.imem or self.if_of or self.of_ex or self.ex_ma or self.ma_rw) and n < max_cycles:
            self.cycle(); n += 1
        return n

    def show(self):
        order = sorted(self.text); idx = {pc: i + 1 for i, pc in enumerate(order)}
        for pc in order: print(f'[{idx[pc]}] {self.text[pc]}')
        print('     ' + ''.join(f'{k + 1:>3}' for k in range(len(self.diagram))))
        for s, name in enumerate(('IF', 'OF', 'EX', 'MA', 'RW')):
            print(f'{name:4s} ' + ''.join(f'{idx[d[s]] if d[s] is not None else "":>3}' for d in self.diagram))

if __name__ == '__main__':
    p = Pipeline(open(sys.argv[1]).read())
    for kv in sys.argv[2:]:
        k, v = kv.split('='); p.reg[int(k[1:])] = int(v, 0)
    n = p.run(); p.show()
    print(f'{n} cycles;', ' '.join(f'r{i}={p.reg[i]}' for i in range(14) if p.reg[i]), f'| E={p.E} GT={p.GT}')
