# Dynamic Programming · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

執行：`make test T=28-dynamic-programming`

---

## 70 · Climbing Stairs（Easy）
[題目](https://leetcode.com/problems/climbing-stairs/) · [程式](0070-climbing-stairs.c)

**題意**：每次爬 1 或 2 階，爬到第 n 階有幾種方法？

**狀態**：`ways[i]` = 爬到第 i 階的方法數。
**轉移**：最後一步是從 i−1 爬 1 階，或從 i−2 爬 2 階 → `ways[i] = ways[i−1] + ways[i−2]`。
**初始**：`ways[0] = ways[1] = 1`。

這就是 Fibonacci（02 主題）。只需要前兩項，所以用兩個變數就好。

**複雜度**：O(n) 時間、O(1) 空間。

---

## 198 · House Robber（Medium）
[題目](https://leetcode.com/problems/house-robber/) · [程式](0198-house-robber.c)

**題意**：一排房子各有一些錢，不能偷相鄰的兩間，最多能偷多少？

**狀態**：走到第 i 間時，兩個數字：
- `take`：第 i 間**有偷**，目前最多多少。
- `skip`：第 i 間**沒偷**，目前最多多少。

**轉移**：
- `take = 上一間的 skip + nums[i]`（偷這間，上一間就不能偷）
- `skip = max(上一間的 skip, 上一間的 take)`（不偷這間，上一間偷不偷都可以）

**複雜度**：O(n) 時間、O(1) 空間。

**陷阱**：答案不一定是「隔一間偷一間」：`[2, 1, 1, 2]` 最好的是偷頭尾，得 4。

---

## 322 · Coin Change（Medium）
[題目](https://leetcode.com/problems/coin-change/) · [程式](0322-coin-change.c)

**題意**：每種面額的硬幣無限多，湊出 amount 最少要幾枚？湊不出來回傳 −1。

**狀態**：`dp[x]` = 湊出 x 元最少要幾枚。
**轉移**：最後一枚是 c → `dp[x] = min over c (dp[x − c] + 1)`。
**初始**：`dp[0] = 0`；其他先設成 ∞（本檔用 `amount + 1`，因為答案不可能超過 amount 枚）。

**複雜度**：O(amount × 面額數)。

**陷阱**：
- **貪心是錯的**：面額 [1, 3, 4] 湊 6，先拿最大的 4 → 4+1+1 = 3 枚；最佳是 3+3 = 2 枚。
- ∞ 不要用 `INT_MAX`，`INT_MAX + 1` 會溢位。
- 這是「完全背包」：每種硬幣可以用很多次，所以外層是金額、內層是硬幣（或容量由小到大掃）。

---

## 1143 · Longest Common Subsequence（Medium）
[題目](https://leetcode.com/problems/longest-common-subsequence/) · [程式](1143-longest-common-subsequence.c)

**題意**：兩個字串的最長共同**子序列**（不用連續，但順序要一樣）有多長？

**狀態**：`c[i][j]` = text1 前 i 個、text2 前 j 個字元的 LCS 長度。
**轉移**：
- 最後一個字元一樣 → 這個字元一定可以用：`c[i−1][j−1] + 1`
- 不一樣 → 至少有一個不在 LCS 裡，丟掉其中一個：`max(c[i−1][j], c[i][j−1])`

**複雜度**：O(nm) 時間。只要長度的話，只留一列，空間 O(m)。要還原出 LCS 字串就需要整張表（見 `dp.c` 的 `lcs`）。

**陷阱**：**子序列 (subsequence)** 不用連續；**子字串 (substring)** 要連續。「最長共同子字串」的轉移是：字元不同時 `dp = 0`，而不是取 max。

---

## 300 · Longest Increasing Subsequence（Medium）
[題目](https://leetcode.com/problems/longest-increasing-subsequence/) · [程式](0300-longest-increasing-subsequence.c)

**題意**：最長**嚴格遞增**子序列的長度。

**O(n²) DP**：`dp[i]` = 以 `nums[i]` 結尾的 LIS 長度 = 1 + max(`dp[j]`)，其中 j < i 且 `nums[j] < nums[i]`。

**O(n log n)（本檔）**：
- `tails[k]` = 所有長度 k+1 的遞增子序列中，**最小的結尾**。結尾越小，後面越容易接東西。
- `tails` 一定是遞增的，所以可以二分搜尋。
- 對每個 x：找 `tails` 裡第一個 ≥ x 的位置，換成 x；如果 x 比全部都大，就接在最後面，長度 +1。
```
[10 9 2 5 3 7 101 18]
10  → [10]
9   → [9]          換掉 10
2   → [2]
5   → [2 5]
3   → [2 3]        換掉 5
7   → [2 3 7]
101 → [2 3 7 101]
18  → [2 3 7 18]   長度 4
```

**陷阱**：
- **`tails` 不是一個真的 LIS**，只有它的長度是對的。
- **嚴格遞增**要找「第一個 ≥ x」（lower_bound）；如果是「非遞減」，就改成找「第一個 > x」（upper_bound）。
