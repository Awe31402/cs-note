"""SimpleRisc 直譯器（課本 Ch3 的 21 條指令，含 u/h modifier）。
用法：python3 simplerisc.py prog.s [r0=值 ...]"""
import re, sys

M32 = 0xFFFFFFFF
def s32(x): x &= M32; return x - (1 << 32) if x & 0x80000000 else x

class CPU:
    def __init__(self, src):
        self.r = [0] * 16; self.r[14] = 0x10000          # sp 從高位址往下長
        self.E = self.GT = 0; self.mem = {}; self.prog = []; self.labels = {}
        for line in src.splitlines():
            line = re.sub(r'(@|//).*|/\*.*?\*/', '', line).strip()
            while (m := re.match(r'([.\w]+):\s*(.*)', line)):
                self.labels[m.group(1)] = len(self.prog); line = m.group(2)
            if line: self.prog.append(line)

    def reg(self, t): return {'sp': 14, 'ra': 15}.get(t) if t in ('sp', 'ra') else int(t[1:])
    def imm(self, t, mod):
        v = int(t, 0) & 0xFFFF
        if mod == 'u': return v
        if mod == 'h': return v << 16
        return v - 0x10000 if v & 0x8000 else v          # 預設：sign extension
    def val(self, t, mod=''):
        return self.r[self.reg(t)] if re.fullmatch(r'r\d+|sp|ra', t) else self.imm(t, mod)
    def addr(self, t):
        off, base = re.fullmatch(r'(-?\w*)\[(\w+)\]', t).groups()
        return (int(off or '0', 0) + self.r[self.reg(base)]) & M32
    def load(self, a): return s32(sum(self.mem.get(a + i, 0) << (8 * i) for i in range(4)))
    def store(self, a, v):
        for i in range(4): self.mem[a + i] = (v >> (8 * i)) & 0xFF   # little endian

    def run(self, max_steps=10**6):
        pc = steps = 0; self.min_sp = self.r[14]
        while 0 <= pc < len(self.prog) and steps < max_steps:
            op, *args = re.split(r'[\s,]+', self.prog[pc]); steps += 1; npc = pc + 1
            mod = ''
            if op not in ('mov', 'not', 'mod', 'sub', 'cmp', 'b') and op[-1] in 'uh' and op[:-1] in (
                    'mov', 'add', 'sub', 'mul', 'div', 'mod', 'and', 'or', 'not', 'cmp'):
                op, mod = op[:-1], op[-1]
            elif op in ('movu', 'movh'): op, mod = 'mov', op[-1]
            a = args
            if op == 'mov': self.r[self.reg(a[0])] = s32(self.val(a[1], mod))
            elif op == 'not': self.r[self.reg(a[0])] = s32(~self.val(a[1], mod))
            elif op == 'cmp':
                x, y = self.val(a[0]), s32(self.val(a[1], mod)); self.E, self.GT = int(x == y), int(x > y)
            elif op in ('add', 'sub', 'mul', 'div', 'mod', 'and', 'or', 'lsl', 'lsr', 'asr'):
                x, y = self.val(a[1]), s32(self.val(a[2], mod))
                f = {'add': lambda: x + y, 'sub': lambda: x - y, 'mul': lambda: x * y,
                     'div': lambda: int(x / y), 'mod': lambda: x - int(x / y) * y,
                     'and': lambda: x & y, 'or': lambda: x | y, 'lsl': lambda: x << (y & 31),
                     'lsr': lambda: (x & M32) >> (y & 31), 'asr': lambda: x >> (y & 31)}[op]
                self.r[self.reg(a[0])] = s32(f())
            elif op == 'ld': self.r[self.reg(a[0])] = self.load(self.addr(a[1]))
            elif op == 'st': self.store(self.addr(a[1]), self.r[self.reg(a[0])])
            elif op == 'b': npc = self.labels[a[0]]
            elif op == 'beq': npc = self.labels[a[0]] if self.E else npc
            elif op == 'bgt': npc = self.labels[a[0]] if self.GT else npc
            elif op == 'call': self.r[15] = npc; npc = self.labels[a[0]]
            elif op == 'ret': npc = self.r[15]
            elif op == 'nop': pass
            else: raise SystemExit(f'unknown instruction: {self.prog[pc]}')
            pc = npc; self.min_sp = min(self.min_sp, self.r[14])
        return steps

if __name__ == '__main__':
    cpu = CPU(open(sys.argv[1]).read())
    for kv in sys.argv[2:]:
        k, v = kv.split('='); cpu.r[cpu.reg(k)] = int(v, 0)
    n = cpu.run()
    print(f'{sys.argv[1]}: {n} steps;', ' '.join(f'r{i}={cpu.r[i]}' for i in range(14) if cpu.r[i]),
          f'| E={cpu.E} GT={cpu.GT} | sp=0x{cpu.r[14]:X} min_sp=0x{cpu.min_sp:X}')
