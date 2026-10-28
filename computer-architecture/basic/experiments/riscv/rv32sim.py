"""極小的 RV32IM（＋ F 的一部分）模擬器：執行 llvm-mc 組譯出來的機器碼（.text 的原始 byte）。
用法：python3 rv32sim.py prog.bin [a0=值 ...]；遇到 ecall 或 ret 到 0 時停止。"""
import math, struct, sys

ABI = 'zero ra sp gp tp t0 t1 t2 s0 s1 a0 a1 a2 a3 a4 a5 a6 a7 s2 s3 s4 s5 s6 s7 s8 s9 s10 s11 t3 t4 t5 t6'.split()
M = 0xFFFFFFFF
def sx(v, bits): return v - (1 << bits) if v >> (bits - 1) & 1 else v
def s32(v): return sx(v & M, 32)

def f32(v): return struct.unpack('<f', struct.pack('<f', v))[0]   # 捨入成單精度（RNE）
def rnd(v, rm):                                                        # fcvt.w.s 的五種捨入模式
    fl = math.floor(v); frac = v - fl
    if rm == 0: return int(fl + (frac > 0.5 or (frac == 0.5 and fl % 2 == 1)))  # RNE
    if rm == 1: return int(v)                                                  # RTZ
    if rm == 2: return math.floor(v)                                           # RDN
    if rm == 3: return math.ceil(v)                                            # RUP
    if rm == 4: return int(fl + (frac >= 0.5)) if v >= 0 else -int(-v // 1 + ((-v) % 1 >= 0.5))  # RMM

class RV32:
    def __init__(self, code):
        self.x = [0] * 32; self.f = [0.0] * 32; self.frm = 0; self.pc = 0; self.mem = {}
        for i, b in enumerate(code): self.mem[i] = b
        self.x[2] = 0x80000; self.x[1] = 0xFFFFFFF0          # sp；ra 指向「結束」
    def ld(self, a, n, signed):
        v = sum(self.mem.get((a + i) & M, 0) << (8 * i) for i in range(n))
        return sx(v, 8 * n) if signed else v
    def st(self, a, v, n):
        for i in range(n): self.mem[(a + i) & M] = (v >> (8 * i)) & 0xFF
    def step(self):
        w = self.ld(self.pc, 4, False)
        op, rd, f3, rs1, rs2, f7 = w & 0x7F, (w >> 7) & 31, (w >> 12) & 7, (w >> 15) & 31, (w >> 20) & 31, w >> 25
        a, b = self.x[rs1], self.x[rs2]
        immI = sx(w >> 20, 12)
        immS = sx(((w >> 25) << 5) | ((w >> 7) & 31), 12)
        immB = sx(((w >> 31) << 12) | (((w >> 7) & 1) << 11) | (((w >> 25) & 63) << 5) | (((w >> 8) & 15) << 1), 13)
        immU = w & 0xFFFFF000
        immJ = sx(((w >> 31) << 20) | (((w >> 12) & 255) << 12) | (((w >> 20) & 1) << 11) | (((w >> 21) & 1023) << 1), 21)
        npc, r = self.pc + 4, None
        if op == 0x37: r = immU                                  # lui
        elif op == 0x17: r = self.pc + immU                      # auipc
        elif op == 0x6F: r, npc = self.pc + 4, self.pc + immJ    # jal
        elif op == 0x67: r, npc = self.pc + 4, (a + immI) & ~1   # jalr
        elif op == 0x63:                                         # beq bne blt bge bltu bgeu
            sa, sb = s32(a), s32(b)
            t = {0: a == b, 1: a != b, 4: sa < sb, 5: sa >= sb, 6: (a & M) < (b & M), 7: (a & M) >= (b & M)}[f3]
            if t: npc = self.pc + immB
        elif op == 0x03:                                         # lb lh lw lbu lhu
            n, sg = {0: (1, 1), 1: (2, 1), 2: (4, 1), 4: (1, 0), 5: (2, 0)}[f3]
            r = self.ld((a + immI) & M, n, sg)
        elif op == 0x23: self.st((a + immS) & M, b, {0: 1, 1: 2, 2: 4}[f3])
        elif op == 0x13:                                         # addi slti sltiu xori ori andi slli srli srai
            sh = (w >> 20) & 31
            r = {0: a + immI, 2: int(s32(a) < immI), 3: int((a & M) < (immI & M)), 4: a ^ immI, 6: a | immI, 7: a & immI,
                 1: a << sh, 5: (s32(a) >> sh) if f7 & 0x20 else ((a & M) >> sh)}[f3]
        elif op == 0x33 and f7 == 1:                             # M 擴充
            sa, sb = s32(a), s32(b)
            def div(x, y): return -1 if y == 0 else int(x / y)
            def rem(x, y): return x if y == 0 else x - int(x / y) * y
            r = {0: sa * sb, 1: (sa * sb) >> 32, 2: (sa * (b & M)) >> 32, 3: ((a & M) * (b & M)) >> 32,
                 4: div(sa, sb), 5: div(a & M, b & M), 6: rem(sa, sb), 7: rem(a & M, b & M)}[f3]
        elif op == 0x33:
            sh = b & 31
            r = {(0, 0): a + b, (0, 0x20): a - b, (1, 0): a << sh, (2, 0): int(s32(a) < s32(b)), (3, 0): int((a & M) < (b & M)),
                 (4, 0): a ^ b, (5, 0): (a & M) >> sh, (5, 0x20): s32(a) >> sh, (6, 0): a | b, (7, 0): a & b}[(f3, f7)]
        elif op == 0x07:                                         # flw
            self.f[rd] = struct.unpack('<f', struct.pack('<I', self.ld((a + immI) & M, 4, False)))[0]
        elif op == 0x27:                                         # fsw
            self.st((a + immS) & M, struct.unpack('<I', struct.pack('<f', self.f[rs2]))[0], 4)
        elif op in (0x43, 0x47):                                 # fmadd.s / fmsub.s（fused：只捨入一次）
            p = self.f[rs1] * self.f[rs2]; c = self.f[w >> 27]
            self.f[rd] = f32(p + c if op == 0x43 else p - c)
        elif op == 0x53:
            fa, fb, f5 = self.f[rs1], self.f[rs2], w >> 27
            if f5 == 0: self.f[rd] = f32(fa + fb)
            elif f5 == 1: self.f[rd] = f32(fa - fb)
            elif f5 == 2: self.f[rd] = f32(fa * fb)
            elif f5 == 3: self.f[rd] = f32(fa / fb)
            elif f5 == 11: self.f[rd] = f32(math.sqrt(fa))
            elif f5 == 5: self.f[rd] = min(fa, fb) if f3 == 0 else max(fa, fb)
            elif f5 == 20: r = int({2: fa == fb, 1: fa < fb, 0: fa <= fb}[f3])
            elif f5 == 24: r = rnd(fa, self.frm if f3 == 7 else f3) & M       # fcvt.w.s
            elif f5 == 26: self.f[rd] = f32(s32(a))                          # fcvt.s.w
            else: raise SystemExit(f'unsupported FP instruction {w:08x}')
        elif op == 0x73: return False                            # ecall：停
        else: raise SystemExit(f'unsupported instruction {w:08x} at pc={self.pc:#x}')
        if r is not None and rd: self.x[rd] = r & M
        self.pc = npc & M
        return self.pc != 0xFFFFFFF0

if __name__ == '__main__':
    cpu = RV32(open(sys.argv[1], 'rb').read())
    for kv in sys.argv[2:]:
        k, v = kv.split('='); cpu.x[ABI.index(k)] = int(v, 0) & M
    n = 0
    while cpu.step() and n < 10**6: n += 1
    shown = ' '.join(f'{ABI[i]}={s32(cpu.x[i])}' for i in range(5, 32) if cpu.x[i] and ABI[i] not in ('sp',))
    fs = ' '.join(f'f{i}={cpu.f[i]:.6g}' for i in range(32) if cpu.f[i])
    print(f'{sys.argv[1]}: {n + 1} instructions; {shown}' + (f' | {fs}' if fs else ''))
