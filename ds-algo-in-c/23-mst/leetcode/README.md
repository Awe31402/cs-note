# MST · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

執行：`make test T=23-mst`

---

## 1584 · Min Cost to Connect All Points（Medium）
[題目](https://leetcode.com/problems/min-cost-to-connect-all-points/) · [程式](1584-min-cost-to-connect-all-points.c)

**題意**：平面上 n 個點，連接兩點的代價是曼哈頓距離 `|x1−x2| + |y1−y2|`。把所有點連起來的最小總代價？

**思路**：任兩點都能連，所以是**完全圖**，E ≈ n²/2。用**陣列版 Prim**：
- `key[v]` = v 到目前這棵樹的最短距離。
- 每輪線性找出 key 最小、還沒加入的點，加進樹裡，再用它更新其他點的 key。
- 距離直接用座標算，**不用把 n² 條邊存起來**。

**複雜度**：O(n²)，n ≤ 1000，大約 10⁶ 次運算。

**為什麼不用 Kruskal**：要先產生並排序約 50 萬條邊，O(n² log n)，也比較吃記憶體。heap 版 Prim 在完全圖上也是 O(n² log n)。

**陷阱**：這是稠密圖的經典例子。面試時說明「為什麼選 O(V²) 的 Prim」會加分。

---

## 1697 · Checking Existence of Edge Length Limited Paths（Hard）
[題目](https://leetcode.com/problems/checking-existence-of-edge-length-limited-paths/) · [程式](1697-checking-existence-of-edge-length-limited-paths.c)

**題意**：每個查詢 `(p, q, limit)`：有沒有一條從 p 到 q 的路，路上**每條邊**的長度都嚴格小於 limit？

**思路：離線處理 (offline processing)**
- 一次看完所有查詢，照自己方便的順序回答，最後再放回原本的位置。
- 邊依長度排序、查詢依 limit 排序。
- 由 limit 小到大處理查詢：先把所有長度 < limit 的邊加進 Union-Find，再問 p、q 在不在同一組。
- limit 越來越大，能用的邊只會越來越多，**不會刪邊**，剛好適合 Union-Find。

**複雜度**：O(E log E + Q log Q)。每個查詢各跑一次 BFS 是 O(Q · (V + E))，會超時。

**陷阱**：
- 條件是**嚴格小於** limit。
- 查詢排序後要記得原本的 index，答案才能放回正確的位置。本檔把查詢複製成 `{p, q, limit, 原index}`。
- 有重邊（同兩點之間好幾條邊）也沒關係，Union-Find 會自動處理。

---

## 1489 · Find Critical and Pseudo-Critical Edges in Minimum Spanning Tree（Hard）
[題目](https://leetcode.com/problems/find-critical-and-pseudo-critical-edges-in-minimum-spanning-tree/) · [程式](1489-find-critical-and-pseudo-critical-edges-in-minimum-spanning-tree.c)

**題意**：
- **關鍵邊**：每一棵 MST 都一定包含它（拿掉它，MST 會變重或根本連不起來）。
- **偽關鍵邊**：不是關鍵邊，但**至少有一棵** MST 包含它。

**思路**：先算出 MST 的總權重 `best`。然後對每條邊 i：
1. **拿掉 i** 再跑 Kruskal：結果比 best 大，或連不起來 → 關鍵邊。
2. 否則，**強迫先選 i**，再跑 Kruskal：結果還是等於 best → 偽關鍵邊。

```
四條權重都是 1 的邊圍成一個正方形：
拿掉任一條 → 剩下三條還是 MST，總權重不變 → 不是關鍵
強迫選任一條 → 總權重還是 3 → 都是偽關鍵
```

**複雜度**：O(E² · α(V))，E ≤ 200，很快。

**陷阱**：
- 排序的是 **index**，不要動到原本的 edges，因為答案要回報原本的邊編號。
- 先判斷關鍵，**是關鍵就不要再算偽關鍵**，題目要求兩者互斥。
- C 的 qsort 比較函式拿不到額外參數，所以用一個全域指標 `g_w` 讓它看到權重。
