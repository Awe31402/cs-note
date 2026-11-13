# NP · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

這三題背後的一般問題都是 **NP-hard**，但題目給的 n 很小（≤ 16），所以可以用指數時間的方法解。重點是**認出來**，然後選對方法。

執行：`make test T=34-np`

---

## 698 · Partition to K Equal Sum Subsets（Medium）
[題目](https://leetcode.com/problems/partition-to-k-equal-sum-subsets/) · [程式](0698-partition-to-k-equal-sum-subsets.c)

**題意**：能不能把陣列分成 k 組，每組總和一樣？n ≤ 16。

**為什麼是 NP-hard**：k = 2 就是 PARTITION 問題（SUBSET-SUM 的特例），它是 NP-complete。

**思路：位元 DP**
- 每組的目標 `target = 總和 / k`。
- 想像把數字**依序填進一組一組**：裝滿 target 就開下一組。
- `dp[mask]` = 用掉 mask 這些數字之後，「目前這一組」已經裝了多少（−1 = 做不到）。
- 轉移：加一個還沒用過的數字，只要不超過這組剩下的空間。
- 答案：`dp[全部] == 0`（每一組都剛好裝滿）。

**複雜度**：O(2ⁿ · n)，n = 16 大約 10⁶。

**關鍵觀察**：只要知道「用了哪些數字」，目前這一組裝了多少就確定了（= 總和 mod target），所以狀態只需要 mask。

---

## 473 · Matchsticks to Square（Medium）
[題目](https://leetcode.com/problems/matchsticks-to-square/) · [程式](0473-matchsticks-to-square.c)

**題意**：火柴全部用上、不能折，能不能圍成正方形？也就是 698 的 k = 4。n ≤ 15。

**思路：回溯 + 剪枝**（示範另一種指數解法）
每根火柴依序試著放到 4 條邊之一，放不下就回頭。三個剪枝：
1. **總和不是 4 的倍數**、或**有火柴比邊長還長** → 直接 false。
2. **由長到短放**：長的火柴能放的位置少，錯誤會更早被發現。
3. **對稱剪枝**：如果兩條邊目前一樣長，把火柴放在哪一條都一樣，只試第一條。

**複雜度**：最壞 O(4ⁿ)，但剪枝之後實際上非常快。

**回溯 vs 位元 DP**：位元 DP 的時間很穩定，但要 O(2ⁿ) 的記憶體；回溯幾乎不用記憶體，好的剪枝讓它通常更快，但最壞情況很難估計。

---

## 847 · Shortest Path Visiting All Nodes（Hard）
[題目](https://leetcode.com/problems/shortest-path-visiting-all-nodes/) · [程式](0847-shortest-path-visiting-all-nodes.c)

**題意**：無向連通圖，每條邊長度 1。可以從任何點出發、可以重複走點和邊。拜訪過所有點的最短路徑長度？n ≤ 12。

**為什麼是 NP-hard**：這跟漢彌爾頓路徑、旅行推銷員 (TSP) 是同一類問題。

**思路：擴充狀態的 BFS**
- 光記「現在在哪個點」不夠，還要記「**已經拜訪過哪些點**」。
- 狀態 = (目前的點, 拜訪過的集合 mask)，共 n · 2ⁿ 個。
- 每條邊長度都是 1 → 在狀態圖上做 BFS，第一次走到 mask = 全部 的狀態就是答案。
- 所有點都可以當起點 → **多源 BFS**：一開始把 n 個 (v, {v}) 都放進 queue。

**複雜度**：O(2ⁿ · n²)。

**對照（本機測試用）**：Held-Karp 演算法。先用 Floyd 算出所有點對的距離，再做 TSP 路徑版的位元 DP：`best[mask][v]` = 拜訪過 mask、最後停在 v 的最短長度。兩種方法的答案要一樣。
