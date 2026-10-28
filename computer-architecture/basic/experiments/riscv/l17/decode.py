# 把每條指令拆成欄位（R/I/S/B/U/J），並從 B、J 的打散位元還原 offset
import struct, sys
code = open(sys.argv[1], 'rb').read()
names = sys.argv[2].split(';')
def b(w, hi, lo): return (w >> lo) & ((1 << (hi - lo + 1)) - 1)
def sx(v, n): return v - (1 << n) if v >> (n - 1) & 1 else v
for i, name in enumerate(names):
    w = struct.unpack('<I', code[4 * i:4 * i + 4])[0]
    op = b(w, 6, 0); s = f'{w:08x} {name:28} '
    if op in (0x33, 0x53):
        s += f'R: funct7={b(w,31,25):07b} rs2={b(w,24,20):2} rs1={b(w,19,15):2} f3={b(w,14,12):03b} rd={b(w,11,7):2} op={op:07b}'
    elif op == 0x43:
        s += f'R4: rs3={b(w,31,27):2} fmt={b(w,26,25):02b} rs2={b(w,24,20):2} rs1={b(w,19,15):2} rm={b(w,14,12):03b} rd={b(w,11,7):2} op={op:07b}'
    elif op in (0x13, 0x03, 0x67, 0x07):
        s += f'I: imm={sx(b(w,31,20),12):5} rs1={b(w,19,15):2} f3={b(w,14,12):03b} rd={b(w,11,7):2} op={op:07b}'
    elif op == 0x23:
        imm = sx(b(w,31,25) << 5 | b(w,11,7), 12)
        s += f'S: imm[11:5]={b(w,31,25):07b} rs2={b(w,24,20):2} rs1={b(w,19,15):2} f3={b(w,14,12):03b} imm[4:0]={b(w,11,7):05b} op={op:07b} → imm={imm}'
    elif op == 0x63:
        imm = sx(b(w,31,31) << 12 | b(w,7,7) << 11 | b(w,30,25) << 5 | b(w,11,8) << 1, 13)
        s += f'B: [12]={b(w,31,31)} [10:5]={b(w,30,25):06b} rs2={b(w,24,20):2} rs1={b(w,19,15):2} f3={b(w,14,12):03b} [4:1]={b(w,11,8):04b} [11]={b(w,7,7)} → offset={imm}'
    elif op == 0x37:
        s += f'U: imm={b(w,31,12)} rd={b(w,11,7):2} op={op:07b}'
    elif op == 0x6F:
        imm = sx(b(w,31,31) << 20 | b(w,19,12) << 12 | b(w,20,20) << 11 | b(w,30,21) << 1, 21)
        s += f'J: [20]={b(w,31,31)} [10:1]={b(w,30,21):010b} [11]={b(w,20,20)} [19:12]={b(w,19,12):08b} rd={b(w,11,7):2} → offset={imm}'
    print(s)
