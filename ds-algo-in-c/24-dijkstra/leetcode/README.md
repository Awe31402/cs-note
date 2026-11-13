# Dijkstra · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

執行：`make test T=24-dijkstra`

---

## 743 · Network Delay Time（Medium）
[題目](https://leetcode.com/problems/network-delay-time/) · [程式](0743-network-delay-time.c)

**題意**：從節點 k 發出訊號，沿著有向、有權重的邊傳送。所有節點都收到要多久？有節點收不到就回傳 −1。

**思路**：從 k 做 Dijkstra，答案是**最短距離中最大的那個**（最晚收到的那個節點）。
n ≤ 100，用 O(V²) 的陣列版最好寫，也不用 heap。

**複雜度**：O(V² + E)。

**陷阱**：
- 節點編號是 1..n，不是 0..n−1。
- 答案是 `max(dist)`，不是 `sum(dist)`：訊號是同時往各個方向傳的。

---

## 1514 · Path with Maximum Probability（Medium）
[題目](https://leetcode.com/problems/path-with-maximum-probability/) · [程式](1514-path-with-maximum-probability.c)

**題意**：無向圖，每條邊有一個成功機率。從 start 到 end，成功機率最大的路徑的機率是多少？

**思路：Dijkstra 反過來用**
| 最短路徑 | 這題 |
|---|---|
| 距離相加 | 機率**相乘** |
| 取最小 | 取**最大** |
| min-heap | **max-heap** |
| 權重 ≥ 0，越走越長 | 機率 ≤ 1，越乘越小 |

「越走只會越差」這個性質一樣成立，所以 Dijkstra 的貪心還是對的。

**複雜度**：O((V + E) log E)。

**陷阱**：
- 走不到時回傳 0。
- 另一個做法是取 −log(p) 變成「相加取最小」的標準最短路徑，但直接用 max-heap 比較簡單。

---

## 1631 · Path With Minimum Effort（Medium）
[題目](https://leetcode.com/problems/path-with-minimum-effort/) · [程式](1631-path-with-minimum-effort.c)

**題意**：網格上每格有高度。從左上走到右下，一條路的「費力程度」是路上相鄰兩格高度差的**最大值**。求最小的費力程度。

**思路**：Dijkstra，但路徑的代價是 `max(目前代價, 這一步的高度差)`，而不是相加。
`max` 一樣只會越走越大，所以第一次拿出終點時，它的代價就是最小的。

**複雜度**：O(mn log(mn))。

**其他做法**：
- **二分搜尋答案**：猜一個值 x，用 BFS 看「只走高度差 ≤ x 的邊」能不能到終點。O(mn log H)。
- **Kruskal 風格**：所有相鄰格子的邊依高度差排序，依序用 Union-Find 合併，起點和終點第一次連通時的邊權重就是答案。

---

## 1976 · Number of Ways to Arrive at Destination（Medium）
[題目](https://leetcode.com/problems/number-of-ways-to-arrive-at-destination/) · [程式](1976-number-of-ways-to-arrive-at-destination.c)

**題意**：無向圖，從 0 到 n−1 的**最短路徑**有幾條？答案 mod 10⁹+7。

**思路**：Dijkstra 的同時，用 `ways[v]` 記錄以最短距離到達 v 的方法數：
- 鬆弛時找到**更短**的路：`dist[v]` 更新，`ways[v] = ways[u]`（之前算的都作廢）。
- 找到**一樣短**的路：`ways[v] += ways[u]`。

為什麼對：u 從 heap 裡被拿出來時，`dist[u]` 和 `ways[u]` 都已經確定，不會再變了。

**複雜度**：O(V²)，n ≤ 200。

**陷阱**：
- **距離一定要用 `long long`**：每條邊最長 10⁹，路徑最多 199 條邊。本機測試特別用了總長 2 × 10¹⁰ 的例子。
- `ways` 要一路取餘數。
- 只有一個節點時（n = 1），答案是 1。
