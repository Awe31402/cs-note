"""SimpleRisc assembler：把組合語言編成 32 位元機器碼（課本 §3.3.14）。
用法：python3 srasm.py prog.s"""
import re, sys

OP = dict(add=0, sub=1, mul=2, div=3, mod=4, cmp=5, AND=6, OR=7, NOT=8, mov=9, lsl=10, lsr=11,
          asr=12, nop=13, ld=14, st=15, beq=16, bgt=17, b=18, call=19, ret=20)
OP.update({'and': 6, 'or': 7, 'not': 8})
MODS = {'': 0, 'u': 1, 'h': 2}

def reg(t): return {'sp': 14, 'ra': 15}.get(t, None) if t in ('sp', 'ra') else int(t[1:])
def isreg(t): return re.fullmatch(r'r\d+|sp|ra', t) is not None

def encode(line, pc, labels):
    op, *a = re.split(r'[\s,]+', line.strip()); mod = ''
    if op not in OP and op[-1] in 'uh' and op[:-1] in OP: op, mod = op[:-1], op[-1]
    code = OP[op] << 27
    if op in ('nop', 'ret'): return code
    if op in ('b', 'beq', 'bgt', 'call'):                       # branch format：word offset
        return code | (((labels[a[0]] - pc) // 4) & 0x7FFFFFF)
    def imm(t):                                                 # I = 1，18 位元 imm = 2 bit modifier + 16 bit
        return (1 << 26) | (MODS[mod] << 16) | (int(t, 0) & 0xFFFF)
    if op in ('ld', 'st'):
        off, base = re.fullmatch(r'(-?\w*)\[(\w+)\]', a[1]).groups()
        return code | (1 << 26) | (reg(a[0]) << 22) | (reg(base) << 18) | (int(off or '0', 0) & 0xFFFF)
    if op == 'cmp':  rd, rs1, src = 0, reg(a[0]), a[1]
    elif op in ('mov', 'not'): rd, rs1, src = reg(a[0]), 0, a[1]
    else: rd, rs1, src = reg(a[0]), reg(a[1]), a[2]
    code |= (rd << 22) | (rs1 << 18)
    return code | (reg(src) << 14 if isreg(src) else imm(src))

def assemble(src):
    lines, labels = [], {}
    for l in src.splitlines():
        l = re.sub(r'(@|//).*|/\*.*?\*/', '', l).strip()
        while (m := re.match(r'([.\w]+):\s*(.*)', l)): labels[m.group(1)] = 4 * len(lines); l = m.group(2)
        if l: lines.append(l)
    return [(4 * i, l, encode(l, 4 * i, labels)) for i, l in enumerate(lines)]

if __name__ == '__main__':
    for pc, l, w in assemble(open(sys.argv[1]).read()):
        b = f'{w:032b}'
        print(f'{pc:4d}: {w:08X}  {b[:5]} {b[5]} {b[6:10]} {b[10:14]} {b[14:]}   {l}')
