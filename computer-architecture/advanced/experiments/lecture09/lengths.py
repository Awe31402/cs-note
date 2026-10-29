# lengths.py：x86 指令有多長？（動態：valgrind lackey 的 trace；靜態：objdump）
import re, subprocess, sys
from collections import Counter
dyn = Counter(int(l.split(',')[1]) for l in open(sys.argv[1]) if l.startswith('I '))
st = Counter()
for line in subprocess.run(['objdump', '-d', '--insn-width=15', sys.argv[2]], capture_output=True, text=True).stdout.splitlines():
    m = re.match(r'\s+[0-9a-f]+:\t((?:[0-9a-f]{2} )+)\s*\t', line)
    if m: st[len(m.group(1).split())] += 1
def show(name, c):
    n = sum(c.values()); avg = sum(k * v for k, v in c.items()) / n
    print(f"{name}：{n} 條，平均 {avg:.2f} byte，最長 {max(c)} byte")
    print("   " + "  ".join(f"{k}B:{c[k] / n:.1%}" for k in sorted(c) if c[k] / n >= 0.005))
show("動態（執行到的每一條）", dyn)
show("靜態（整個執行檔）", st)
