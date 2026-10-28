"""把 Table 9.5 的布林式（datapath.control）和 Table 9.4 的文字條件逐一比對，並印出每條指令的控制訊號。"""
import os, sys; sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from datapath import control
NAMES = 'add sub mul div mod cmp and or not mov lsl lsr asr nop ld st beq bgt b call ret'.split()
T94 = dict(isSt={'st'}, isLd={'ld'}, isBeq={'beq'}, isBgt={'bgt'}, isRet={'ret'},
           isWb=set('add sub mul div mod and or not mov ld lsl lsr asr call'.split()),
           isUBranch={'b', 'call', 'ret'}, isCall={'call'}, isAdd={'add', 'ld', 'st'})
bad = 0
for code, name in enumerate(NAMES):
    for I in (0, 1):
        c = control(code, I)
        for sig, members in T94.items():
            if c[sig] != (name in members): bad += 1; print('MISMATCH', name, sig, c[sig])
        if c['isImmediate'] != I: bad += 1
print('mismatches:', bad)
for code, name in enumerate(NAMES):
    on = [k for k, v in control(code, 0).items() if v]
    print(f'{code:05b} {name:5s}', ' '.join(on) or '（全部是 0）')
# 沒用到的 opcode 10101–11111 會打開哪些訊號？
for code in range(21, 32):
    on = [k for k, v in control(code, 0).items() if v]
    print(f'{code:05b} 未定義', ' '.join(on) or '（全部是 0）')
