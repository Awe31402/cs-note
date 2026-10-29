# ist.py：用真的指令 trace 測課本 §3.2 的 IST（instruction status table）
#   用法：python3 ist.py work lackey.log
#   每條指令（照執行順序）：先用 IST 猜它是什麼類型，再用真的類型（objdump 看得出來）更新 IST
import re, subprocess, sys
from collections import Counter

def types(binary):
    t = {}
    for line in subprocess.run(['objdump', '-d', '--no-show-raw-insn', binary], capture_output=True, text=True).stdout.splitlines():
        m = re.match(r'\s+([0-9a-f]+):\s+(?:(?:bnd|notrack|rep|repz)\s+)*([a-z]+)', line)
        if not m: continue
        op = m.group(2)
        t[int(m.group(1), 16)] = ('ret' if op.startswith('ret') else 'call' if op.startswith('call') else
                                  'jmp' if op == 'jmp' else 'cond' if op.startswith('j') else 'none')
    return t

T = types(sys.argv[1])
pcs = [int(l[3:].split(',')[0], 16) for l in open(sys.argv[2]) if l.startswith('I ')]
kinds = [T.get(p, 'none') for p in pcs]
mix = Counter(kinds)
print(f"{len(pcs)} 條動態指令、{len(set(pcs))} 個不同的 PC；類型：" +
      "、".join(f"{k} {v / len(pcs):.1%}" for k, v in mix.most_common()))

def ist(index, tagbits, n=10):
    """index：從哪個 bit 開始取 n 個 bit 當 index；tagbits：存幾個 bit 的 tag（None = 完整 tag）"""
    table, ok, okb, nb = {}, 0, 0, 0
    mask = (1 << n) - 1
    for pc, k in zip(pcs, kinds):
        i = (pc >> index) & mask
        rest = pc >> (index + n) if index == 0 else ((pc >> (index + n)) << index) | (pc & ((1 << index) - 1))
        tag = rest if tagbits is None else rest & ((1 << tagbits) - 1)
        e = table.get(i)
        guess = e[1] if e and (tagbits == 0 or e[0] == tag) else 'none'   # 沒有這個 entry：猜「不是 branch」
        ok += guess == k
        if k != 'none': nb += 1; okb += guess == k
        table[i] = (tag, k)
    return ok / len(pcs), okb / nb

print(f"{'IST 設計（1024 entry）':34s}{'全部指令猜對':>12s}{'branch 猜對':>12s}")
for name, idx, tb in [("LSB 10 bit，沒有 tag", 0, 0), ("LSB 10 bit，4 bit tag", 0, 4),
                      ("LSB 10 bit，8 bit tag", 0, 8), ("LSB 10 bit，完整 tag", 0, None),
                      ("中間 bit 6–15，沒有 tag", 6, 0), ("高位 bit 14–23，沒有 tag", 14, 0),
                      ("高位 bit 14–23，完整 tag", 14, None)]:
    a, b = ist(idx, tb)
    print(f"{name:34s}{a:12.2%}{b:12.2%}")
print(f"{'一律猜「不是 branch」':34s}{mix['none'] / len(pcs):12.2%}{0:12.2%}")
