# Insertion Sort · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

執行：`make test T=10-insertion-sort`

---

## 283 · Move Zeroes（Easy）
[題目](https://leetcode.com/problems/move-zeroes/) · [程式](0283-move-zeroes.c)

**題意**：把所有 0 移到最後面，非 0 元素的**相對順序不變**，而且要原地做。

**思路**：`w` 是下一個非 0 元素要寫的位置。
1. 掃過一遍，把非 0 的依序寫到 `nums[w++]`。
2. 剩下的格子全部補 0。

**複雜度**：O(n)，額外空間 O(1)。

**陷阱**：題目要求保持順序，也就是**穩定**。用「兩頭交換」的做法比較快，但會打亂順序。

---

## 976 · Largest Perimeter Triangle（Easy）
[題目](https://leetcode.com/problems/largest-perimeter-triangle/) · [程式](0976-largest-perimeter-triangle.c)

**題意**：選三個邊長組成面積 > 0 的三角形，回傳最大周長。組不出來就回傳 0。

**思路**：排序以後，三邊 `a ≤ b ≤ c` 能組成三角形的條件只剩一個：`a + b > c`。
- 從最大的 c 開始往下試。
- c 固定時，最好的 a、b 就是 c 左邊緊鄰的兩個（最大的兩個）。
- 如果它們都不夠大，其他組合更不可能，直接換下一個 c。

**複雜度**：O(n log n)，主要花在排序。

**陷阱**：
- 周長最大的三角形不一定用到最大的那根邊。例如 `[1, 2, 1, 10]` 裡的 10 就用不到。
- `qsort` 的比較函式不要寫 `a - b`。

---

## 147 · Insertion Sort List（Medium）
[題目](https://leetcode.com/problems/insertion-sort-list/) · [程式](0147-insertion-sort-list.c)

**題意**：用插入排序排一條單向串列。

**思路**：跟陣列版一樣，只是「插入」改成接指標。
- 另外建一條已排序的串列，開頭放 `dummy`。
- 從原串列逐一拿下節點，從已排序串列裡找第一個比它大的節點，插在它前面。

串列不能往回走，所以是從前面往後找插入點（陣列版是從後面往前找）。

**優化**：記住上次插入的位置 `last`。新節點 `>= last` 時就從 `last` 往後找，不用每次都從頭開始。這樣已排序的輸入會變成 O(n)。

**複雜度**：最壞 O(n²)，額外空間 O(1)。

**陷阱**：
- 要先存 `head = head->next`，再去改 `node->next`，不然會斷掉。
- 用 `<=` 往後找，值相等的節點會排在後面，這樣才是穩定的。
- 如果題目要求 O(n log n)，就用 merge sort（LeetCode 148，第 11 主題）。
