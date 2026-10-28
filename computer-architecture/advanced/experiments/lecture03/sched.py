# sched.py：課本 Figure 1.3 的程式，用 2-issue in-order 和 2-issue OOO 排一次（每條指令 1 個 cycle）
code = [("add r1, r2, r3", "r1", ["r2", "r3"]),
        ("mul r4, r1, r1", "r4", ["r1"]),
        ("add r5, r4, r1", "r5", ["r4", "r1"]),
        ("mov r8, 5",      "r8", []),
        ("add r9, r8, r8", "r9", ["r8"]),
        ("sub r10, r9, r8", "r10", ["r9", "r8"])]
W = 2

def ready(i, done):
    """第 i 條的來源暫存器，是否都已經由更早、已完成的指令寫好"""
    return all(not (code[j][1] in code[i][2]) or j in done for j in range(i))

def run(ooo):
    done, cycles, left = set(), [], list(range(len(code)))
    while left:
        pick = []
        for i in left:
            if len(pick) == W:
                break
            if ready(i, done):
                pick.append(i)
            elif not ooo:
                break           # in-order：前面的卡住，後面的也不能先走
        done |= set(pick)
        left = [i for i in left if i not in pick]
        cycles.append(pick)
    return cycles

for name, ooo in (("2-issue in-order", False), ("2-issue OOO", True)):
    c = run(ooo)
    print(f"{name}：{len(c)} cycles")
    for n, pick in enumerate(c, 1):
        print(f"  cycle {n}: " + " | ".join(code[i][0] for i in pick))
