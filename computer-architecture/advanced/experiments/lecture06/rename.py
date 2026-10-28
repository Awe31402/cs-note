# rename.py：把架構暫存器改名成實體暫存器，列出改名前後的 RAW／WAR／WAW 相依

def deps(code):
    """code: [(dst, [srcs])]；回傳 (種類, i, j) 的清單，i 在 j 之前"""
    out = []
    for j, (dj, sj) in enumerate(code):
        for i in range(j - 1, -1, -1):
            di, si = code[i]
            if di in sj and not any(code[k][0] == di for k in range(i + 1, j)):
                out.append(("RAW", i + 1, j + 1))
            if dj in si:
                out.append(("WAR", i + 1, j + 1))
            if dj == di and not any(code[k][0] == dj for k in range(i + 1, j)):
                out.append(("WAW", i + 1, j + 1))
    return sorted(set(out), key=lambda d: (d[1], d[2]))

def rename(code, first_free):
    table, nxt, out = {}, first_free, []
    for d, s in code:
        s = [table.get(r, r.replace("r", "p")) for r in s]   # 一開始 rX 在 pX
        table[d] = f"p{nxt}"; nxt += 1                        # 每次寫入，給一個新的實體暫存器
        out.append((table[d], s))
    return out

# 課本 p.55 的例子：add r3,r1,r2 / add r4,r3,2 / sub r3,r1,1
code = [("r3", ["r1", "r2"]), ("r4", ["r3"]), ("r3", ["r1"])]
for name, c in (("改名前", code), ("改名後", rename(code, 3))):
    print(name, ["%s ← %s" % (d, ",".join(s)) for d, s in c])
    print("   相依：", ", ".join(f"{k} {i}→{j}" for k, i, j in deps(c)) or "無")
