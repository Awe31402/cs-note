# Segment Tree · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

這三題剛好是「該用什麼資料結構」的三個層次：**前綴和 → Fenwick → 線段樹**。

執行：`make test T=32-segment-tree`

---

## 303 · Range Sum Query - Immutable（Easy）
[題目](https://leetcode.com/problems/range-sum-query-immutable/) · [程式](0303-range-sum-query-immutable.c)

**題意**：陣列**不會改變**，一直問 `sumRange(l, r)`。

**思路**：前綴和。`pre[i]` = 前 i 個的和，`sumRange(l, r) = pre[r+1] − pre[l]`。

**複雜度**：建表 O(n)，每次查詢 O(1)。

**重點**：陣列不會變的時候，**不需要**線段樹。面試時先想到最簡單的方法。

---

## 307 · Range Sum Query - Mutable（Medium）
[題目](https://leetcode.com/problems/range-sum-query-mutable/) · [程式](0307-range-sum-query-mutable.c)

**題意**：同 303，但會 `update(i, val)` 修改某一格的值。

**思路**：修改會讓前綴和要 O(n) 重算。只有**單點修改 + 區間和**，用 **Fenwick Tree** 最剛好：
- `update(i, val)`：Fenwick 只支援「加值」，所以加上 `val − 舊值`。要另外記住每一格目前的值。
- `sumRange(l, r) = prefix(r) − prefix(l − 1)`。

**複雜度**：修改和查詢都是 O(log n)。

**用線段樹也可以**，但程式長很多。面試時如果題目只有單點修改，寫 Fenwick 比較不容易出錯。

---

## 699 · Falling Squares（Hard）
[題目](https://leetcode.com/problems/falling-squares/) · [程式](0699-falling-squares.c)

**題意**：正方形一個一個掉下來，第 i 個的左邊界是 `left`、邊長是 `side`。它會落在自己範圍內**目前最高**的地方，疊上去。每掉一個，回傳目前的最高高度。
```
[1,2]：落在地上，蓋住 x = 1..2，高度 2              → 2
[2,3]：範圍 2..4 目前最高是 2，疊上去頂部 = 5      → 5
[6,1]：範圍 6..6 目前是 0，高度 1                   → 5
```

**思路**：線段樹，每個節點存「這一段的最大高度」，支援兩個操作：
- `query(l, r)`：區間最大值 → 新方塊的底部高度。
- `assign(l, r, v)`：區間設成 v（懶標記存「整段要設成多少」）→ 新方塊的頂部高度。

**座標離散化**：座標到 10⁸，開不了那麼大的陣列；但最多只有 1000 個方塊，用到的座標最多 2000 個。把用到的座標排序、去重，換成 0..m−1 的編號，線段樹只要開 4m 格。

**複雜度**：O(n log n)。

**陷阱**：
- 方塊蓋住的是 `[left, left + side − 1]`（閉區間）。用 `left + side` 當右端點的話，兩個剛好相鄰的方塊會被誤判成疊在一起（本機測試的第 2 個例子）。
- 設值型的懶標記：往下傳的時候是「**覆蓋**」小孩的值，不是「加上去」。
