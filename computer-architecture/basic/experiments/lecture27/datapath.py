"""課本 Fig 9.15 的單週期 SimpleRisc 處理器：IF → OF → EX → MA → RW，控制訊號用 Table 9.5 的布林式。
用法：python3 -I datapath.py prog.s [r0=值 ...] [trace]   （機器碼由 ../simplerisc/srasm.py 產生）"""
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'simplerisc'))
from srasm import assemble

M32 = 0xFFFFFFFF
def s32(x): x &= M32; return x - (1 << 32) if x & 0x80000000 else x
def bits(w, lo, hi): return (w >> (lo - 1)) & ((1 << (hi - lo + 1)) - 1)   # 課本的位元編號從 1 開始

def control(opcode, I):
    """Table 9.5：op1 是 LSB、op5 是 MSB；~x 寫成 (1-x)"""
    o5, o4, o3, o2, o1 = [(opcode >> k) & 1 for k in (4, 3, 2, 1, 0)]
    n = lambda x: 1 - x
    c = dict(
        isSt=n(o5)&o4&o3&o2&o1, isLd=n(o5)&o4&o3&o2&n(o1),
        isBeq=o5&n(o4)&n(o3)&n(o2)&n(o1), isBgt=o5&n(o4)&n(o3)&n(o2)&o1,
        isRet=o5&n(o4)&o3&n(o2)&n(o1), isImmediate=I,
        isWb=n(o5 | (n(o5)&o3&o1&(o4|n(o2)))) | (o5&n(o4)&n(o3)&o2&o1),
        isUBranch=o5&n(o4)&((n(o3)&o2) | (o3&n(o2)&n(o1))),
        isCall=o5&n(o4)&n(o3)&o2&o1,
        isAdd=(n(o5)&n(o4)&n(o3)&n(o2)&n(o1)) | (n(o5)&o4&o3&o2))
    for name, code in [('isSub', 1), ('isCmp', 5), ('isMul', 2), ('isDiv', 3), ('isMod', 4), ('isLsl', 10),
                       ('isLsr', 11), ('isAsr', 12), ('isOr', 7), ('isAnd', 6), ('isNot', 8), ('isMov', 9)]:
        c[name] = int(opcode == code)
    return c

class Processor:
    def __init__(self, src):
        self.imem = {pc: w for pc, _, w in assemble(src)}
        self.reg = [0] * 16; self.reg[14] = 0x10000
        self.E = self.GT = 0; self.dmem = {}; self.pc = 0; self.trace = False

    def cycle(self):
        # IF：讀指令
        inst = self.imem[self.pc]
        # OF：解碼、算 immx 與 branchTarget、讀暫存器
        c = control(bits(inst, 28, 32), bits(inst, 27, 27))
        imm, mod = bits(inst, 1, 16), bits(inst, 17, 18)
        immx = imm << 16 if mod == 2 else imm if mod == 1 else s32(imm << 16) >> 16
        off = bits(inst, 1, 27) << 2                                 # 29 位元，再做 sign extension
        branchTarget = (self.pc + (off - (1 << 29) if off >> 28 else off)) & M32
        op1 = self.reg[15 if c['isRet'] else bits(inst, 19, 22)]
        op2 = self.reg[bits(inst, 23, 26) if c['isSt'] else bits(inst, 15, 18)]
        # EX：branch unit ＋ ALU
        branchPC = op1 & M32 if c['isRet'] else branchTarget
        isBranchTaken = (c['isBeq'] & self.E) | (c['isBgt'] & self.GT) | c['isUBranch']
        A, B = op1, (immx if c['isImmediate'] else op2)
        aluResult = 0
        if c['isAdd']: aluResult = A + B
        elif c['isSub']: aluResult = A - B
        elif c['isCmp']: self.E, self.GT = int(A == B), int(A > B)
        elif c['isMul']: aluResult = A * B
        elif c['isDiv']: aluResult = int(A / B)
        elif c['isMod']: aluResult = A - int(A / B) * B
        elif c['isLsl']: aluResult = A << (B & 31)
        elif c['isLsr']: aluResult = (A & M32) >> (B & 31)
        elif c['isAsr']: aluResult = A >> (B & 31)
        elif c['isOr']: aluResult = A | B
        elif c['isAnd']: aluResult = A & B
        elif c['isNot']: aluResult = ~B
        elif c['isMov']: aluResult = B
        aluResult = s32(aluResult)
        # MA：mar = aluResult，mdr = op2
        ldResult = 0
        if c['isLd']: ldResult = s32(sum(self.dmem.get((aluResult + i) & M32, 0) << (8 * i) for i in range(4)))
        if c['isSt']:
            for i in range(4): self.dmem[(aluResult + i) & M32] = (op2 >> (8 * i)) & 0xFF
        # RW：三選一（isLd, isCall），寫入位址二選一（isCall）
        result = s32(self.pc + 4) if c['isCall'] else ldResult if c['isLd'] else aluResult
        if c['isWb']: self.reg[15 if c['isCall'] else bits(inst, 23, 26)] = result
        if self.trace:
            on = ' '.join(k for k, v in c.items() if v)
            print(f'pc={self.pc:3d} inst={inst:08X} op1={op1} op2={op2} immx={immx} aluResult={aluResult} '
                  f'isBranchTaken={isBranchTaken} branchPC={branchPC} | {on}')
        # 下一個 pc
        self.pc = branchPC if isBranchTaken else self.pc + 4

    def run(self, max_cycles=10**6):
        n = 0
        while self.pc in self.imem and n < max_cycles: self.cycle(); n += 1
        return n

if __name__ == '__main__':
    p = Processor(open(sys.argv[1]).read())
    for kv in sys.argv[2:]:
        if kv == 'trace': p.trace = True; continue
        k, v = kv.split('='); p.reg[int(k[1:])] = int(v, 0)
    n = p.run()
    print(f'{os.path.basename(sys.argv[1])}: {n} cycles;', ' '.join(f'r{i}={p.reg[i]}' for i in range(14) if p.reg[i]),
          f'| E={p.E} GT={p.GT} | sp=0x{p.reg[14]:X}')
