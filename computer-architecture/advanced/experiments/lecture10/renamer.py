# renamer.py：課本 §4.1 的 renaming 硬體：RAT（rename table）＋ free list（circular queue）＋ dependence check logic
from collections import deque

class Renamer:
    def __init__(self, n_arch=16, n_phys=128):
        self.rat = list(range(n_arch))                      # 一開始 ri → pi
        self.free = deque(range(n_arch, n_phys))            # 其餘 112 個是空的
    def rename_bundle(self, bundle, dcl=True):
        """bundle：同一個 cycle 一起改名的指令 [(dst, [srcs])]；dcl=False 表示沒有 dependence check logic"""
        dsts = [self.free.popleft() if d is not None else None for d, _ in bundle]   # 平行從 free list 拿
        out = []
        for k, (d, srcs) in enumerate(bundle):
            ps = []
            for s in srcs:
                p = self.rat[s]                              # 讀 RAT（這個 cycle 開始時的值）
                if dcl:                                      # 多工器：同一組裡前面最近寫 s 的那條優先
                    for j in range(k - 1, -1, -1):
                        if bundle[j][0] == s: p = dsts[j]; break
                ps.append(p)
            out.append((dsts[k], ps))
        for (d, _), p in zip(bundle, dsts):                  # 這個 cycle 結束才寫回 RAT
            if d is not None: self.rat[d] = p
        return out

def show(code, out):
    for (d, s), (pd, ps) in zip(code, out):
        print(f"   r{d} ← " + ", ".join(f"r{x}" for x in s) + f"   →   p{pd} ← " + ", ".join(f"p{x}" for x in ps))

# 影片 [41:36] 的三條指令：add r1, r2, r3 / add r4, r1, 1 / sub r3, r4, r6（第 2 條讀 r1、第 3 條讀 r4，都在同一組裡寫的）
bundle = [(1, [2, 3]), (4, [1]), (3, [4, 6])]
for dcl in (False, True):
    print("有 dependence check logic：" if dcl else "沒有 dependence check logic（只讀 RAT）：")
    show(bundle, Renamer().rename_bundle(bundle, dcl))

# 影片 [39:22] 的兩條：add r1, r2, r3 → mov r3, r1；一個 cycle 一條，RAT 一開始 r2→p29、r3→p34，free list 頭是 32、28
r = Renamer(); r.rat[2], r.rat[3] = 29, 34; r.free = deque([32, 28] + list(range(40, 128)))
print("一個 cycle 一條：")
show([(1, [2, 3])], r.rename_bundle([(1, [2, 3])])); show([(3, [1])], r.rename_bundle([(3, [1])]))
