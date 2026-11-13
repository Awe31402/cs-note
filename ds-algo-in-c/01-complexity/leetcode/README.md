# 複雜度 · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。每個測試都會拿**暴力解**來對照答案。

執行：`make test T=01-complexity`

---

## 1480 · Running Sum of 1d Array（Easy）
[題目](https://leetcode.com/problems/running-sum-of-1d-array/) · [程式](1480-running-sum-of-1d-array.c)

**題意**：`out[i] = nums[0] + nums[1] + ... + nums[i]`。

**暴力**：每個 i 都從 0 加到 i，總共 1+2+...+n 次，**O(n²)**。

**優化**：`out[i] = out[i-1] + nums[i]`。前面的和已經算過，直接拿來用，**O(n)**。

這就是**前綴和 (prefix sum)**。之後很多題都會用到：有了前綴和，任意區間 `[l, r]` 的和就是 `out[r] - out[l-1]`，只要 O(1)。

**陷阱**：題目的值範圍很小，int 不會溢位。如果是一般情況，前綴和要用 `long long`。

---

## 1 · Two Sum（Easy）
[題目](https://leetcode.com/problems/two-sum/) · [程式](0001-two-sum.c)

**題意**：找兩個不同位置的數，相加等於 target，回傳它們的 index。

| 做法 | 時間 | 空間 |
|---|---|---|
| 兩層迴圈試所有組合 | O(n²) | O(1) |
| **排序 + 雙指標**（本檔） | O(n log n) | O(n) |
| Hash table：看過的值存起來，查 `target - x` 在不在 | O(n) | O(n) |

**排序 + 雙指標**：
```
排序後: [2, 7, 11, 15]  target = 9
         L          R    2+15=17 > 9 → R 往左
         L      R        2+11=13 > 9 → R 往左
         L  R            2+7=9 ✅
```
- 和太小 → L 往右（換大一點的）。
- 和太大 → R 往左（換小一點的）。

**陷阱**：
- 要回傳**原本的 index**，所以排序前要把 `(值, index)` 綁在一起。
- 兩個 int 相加可能溢位，要先轉成 `long`。
- Hash 版的 O(n) 是面試官最想聽到的，第 08 主題 Hash Table 會實作。

---

## 53 · Maximum Subarray（Medium）
[題目](https://leetcode.com/problems/maximum-subarray/) · [程式](0053-maximum-subarray.c)

**題意**：找和最大的**連續**子陣列，回傳它的和。

**從 O(n³) 一路優化到 O(n)**：
1. **O(n³)**：列舉所有 (i, j)，每組再用迴圈加總。
2. **O(n²)**：固定起點 i，往右邊走邊加，不用每次重算。測試裡的 `brute` 就是這個版本。
3. **O(n) Kadane**：
   - `cur` = 「**以 i 結尾**」的最大和。
   - 如果前面累積的 `cur` 是負的，它只會拖累你，就丟掉，從 `nums[i]` 重新開始。
   ```
   nums: -2   1  -3   4  -1   2   1  -5   4
   cur : -2   1  -2   4   3   5   6   1   5
   best: -2   1   1   4   4   5   6   6   6  ← 答案 6
   ```

**其他解法**：Divide & Conquer，O(n log n)。這是 CLRS **第 3 版** 4.1 的經典例子，但第 4 版已經刪掉了（見第 4 版前言，PDF p.18）。第 27 主題會提到這個解法。

**陷阱**：**全部是負數**時，答案是最大的那個負數，**不是 0**。所以 `best` 要從 `nums[0]` 開始，不能從 0 開始。

---

## 204 · Count Primes（Medium）
[題目](https://leetcode.com/problems/count-primes/) · [程式](0204-count-primes.c)

**題意**：算**小於** n 的質數有幾個。n 最大 5×10⁶。

**暴力**：每個數 k 都試除到 √k，總共 **O(n√n)**。n = 5×10⁶ 時大約 10¹⁰ 次，會超時。

**篩法 (Sieve of Eratosthenes)**：
```
2 是質數 → 劃掉 4, 6, 8, 10, ...
3 是質數 → 劃掉 9, 12, 15, ...   ← 從 3² 開始，6 已經被 2 劃過了
4 已劃掉 → 跳過
5 是質數 → 劃掉 25, 30, ...
```
每個質數 p 大約劃掉 n/p 個數，全部加起來是 n(1/2 + 1/3 + 1/5 + ...) = **O(n log log n)**，幾乎是線性。

**陷阱**：
- 是「**小於** n」，所以 n = 2 的答案是 0。
- `p * p` 可能超出 int，本檔用 `long`。
- 標記陣列用 `char` 就好，比 `int` 省 4 倍記憶體。
