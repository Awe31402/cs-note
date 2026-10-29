# irqrate.py：讀 /proc/interrupts 兩次（隔 2 秒），看每種 interrupt 每秒來幾次；同時讓 cpu2 忙著、cpu4 閒著
import subprocess, time
def snap():
    rows = {}
    for line in open('/proc/interrupts').read().splitlines()[1:]:
        f = line.split()
        name = f[0].rstrip(':'); nums = []
        for x in f[1:]:
            if x.isdigit(): nums.append(int(x))
            else: break
        rows[name] = (nums, ' '.join(f[1 + len(nums):]))
    return rows
busy = subprocess.Popen(['taskset', '-c', '2', 'python3', '-c', 'while True: pass'])
time.sleep(0.5); a = snap(); time.sleep(2); b = snap(); busy.kill()
print(f"{'名稱':8s}{'全部 CPU 每秒':>14s}{'cpu2（忙）':>12s}{'cpu4（閒）':>12s}  說明")
for k in b:
    d = [y - x for x, y in zip(a[k][0], b[k][0])]
    if sum(d) == 0: continue
    print(f"{k:8s}{sum(d) / 2:14.0f}{d[2] / 2 if len(d) > 2 else 0:12.0f}{d[4] / 2 if len(d) > 4 else 0:12.0f}  {b[k][1][:40]}")
