# Floyd-Warshall · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

執行：`make test T=26-floyd-warshall`

---

## 1334 · Find the City With the Smallest Number of Neighbors at a Threshold Distance（Medium）
[題目](https://leetcode.com/problems/find-the-city-with-the-smallest-number-of-neighbors-at-a-threshold-distance/) · [程式](1334-find-the-city-with-the-smallest-number-of-neighbors-at-a-threshold-distance.c)

**題意**：無向加權圖。對每個城市，數距離 ≤ threshold 的其他城市有幾個。回傳數量最少的城市；一樣少就回傳**編號最大**的。

**思路**：要**所有點對**的距離，n ≤ 100 → Floyd-Warshall，O(n³) = 10⁶。

**複雜度**：O(n³)。也可以從每個城市各做一次 Dijkstra，但程式長很多。

**陷阱**：
- **一樣少取編號大的**：由小到大掃，比較時用 `<=`，後面的就會覆蓋前面的。
- **INF 要選相加不會溢位的值**：`1 << 29`。用 `INT_MAX` 的話，`INF + INF` 會溢位變成負數。
- 無向圖，兩個方向都要設。

---

## 1462 · Course Schedule IV（Medium）
[題目](https://leetcode.com/problems/course-schedule-iv/) · [程式](1462-course-schedule-iv.c)

**題意**：`[a, b]` 代表 a 是 b 的直接先修課。先修關係有遞移性（a → b → c，所以 a 是 c 的先修課）。每個查詢問「u 是不是 v 的先修課」。

**思路**：**遞移閉包**：`r[i][j]` = i 是不是 j 的先修課（直接或間接）。
```
for k: for i: for j:
    r[i][j] |= r[i][k] && r[k][j]
```
算完之後，每個查詢 O(1)。

**複雜度**：O(n³ + Q)，n ≤ 100。

**其他做法**：拓撲排序，同時把每門課的所有先修課集合往後傳（用 bitset），O(n · E)。

**陷阱**：跟 207、210 相反，這題的 `[a, b]` 是「**a 是 b 的先修課**」，邊是 a → b。每一題都要仔細看方向。

---

## 399 · Evaluate Division（Medium）
[題目](https://leetcode.com/problems/evaluate-division/) · [程式](0399-evaluate-division.c)

**題意**：給一些等式，例如 `a / b = 2.0`、`b / c = 3.0`，回答 `a / c`、`b / a` 之類的查詢。算不出來就回傳 −1.0。

**思路：把除法變成圖**
- 每個變數是一個頂點。
- `a / b = k` → 邊 a → b 權重 k，邊 b → a 權重 1/k。
- `a / c = (a / b) × (b / c)`：沿著路徑把權重**相乘**。

Floyd-Warshall，把「相加取最小」改成「**相乘，有路就填**」：
```
r[i][j] 還是 0（不知道），但 r[i][k]、r[k][j] 都知道 → r[i][j] = r[i][k] × r[k][j]
```
題目保證等式之間不會矛盾，所以不管走哪條路，算出來的值都一樣。

**複雜度**：變數最多 40 個，O(40³)。

**陷阱**：
- **沒出現過的變數**：連 `x / x` 都要回傳 −1.0，不是 1.0。
- 出現過的變數，`a / a = 1.0`。
- 變數是字串，要先轉成編號。變數很少，線性搜尋就夠了。
- 其他做法：每個查詢做一次 BFS/DFS，或用**加權並查集**（邊上存「到根的比值」）。
