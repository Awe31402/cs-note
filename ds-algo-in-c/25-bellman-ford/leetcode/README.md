# Bellman-Ford · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

執行：`make test T=25-bellman-ford`

---

## 743 · Network Delay Time（Medium，Bellman-Ford 版）
[題目](https://leetcode.com/problems/network-delay-time/) · [程式](0743-network-delay-time.c)

**題意**：同 24 主題：從 k 發出訊號，全部節點都收到要多久？

**思路**：Bellman-Ford 最大的好處是**好寫**：不用 heap，也不用建鄰接串列，直接掃題目給的邊清單就好。
- 所有邊鬆弛 n − 1 輪。
- 某一輪都沒有更新就提早結束。

**複雜度**：O(V · E) = 100 × 6000 = 6 × 10⁵，跟 Dijkstra 一樣會過。

**什麼時候選 Bellman-Ford 而不是 Dijkstra**：有負權重、要偵測負環、限制邊數（787），或是圖很小、想少寫一點程式的時候。

---

## 787 · Cheapest Flights Within K Stops（Medium）
[題目](https://leetcode.com/problems/cheapest-flights-within-k-stops/) · [程式](0787-cheapest-flights-within-k-stops.c)

**題意**：從 src 飛到 dst，**最多轉機 k 次**，最便宜多少錢？

**思路**：轉機 k 次 = 最多搭 k + 1 段航班 = 最多用 k + 1 條邊。
Bellman-Ford 的第 i 輪結束後，dist 就是「最多用 i 條邊」的最短距離，所以**只做 k + 1 輪**就好。

**關鍵：每一輪都要從「上一輪」的結果延伸**
```c
memcpy(prev, dist, ...);                // 先複製上一輪的結果
if (prev[u] + w < dist[v]) dist[v] = prev[u] + w;   // 用 prev，不是 dist
```
如果直接用 `dist[u]`，同一輪裡剛更新的 u 馬上又被拿來延伸，等於一輪走了好幾條邊。
```
邊 0→1 (1)、1→2 (1)、0→2 (5)，k = 0（不能轉機）
錯誤寫法：第 1 輪先更新 dist[1] = 1，接著馬上用它算出 dist[2] = 2 ❌（轉了一次機）
正確寫法：第 1 輪只能從 prev（只有 dist[0] = 0）延伸 → dist[2] = 5 ✅
```

**複雜度**：O(k · E)。

**為什麼一般的 Dijkstra 不行**：Dijkstra 只記「最短距離」，但一條比較貴、轉機比較少的路，可能才是符合 k 限制的答案，會被 Dijkstra 丟掉。要改成狀態 (城市, 已轉機次數) 的 Dijkstra 才行。
