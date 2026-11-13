# 陣列 · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

執行：`make test T=03-array`

---

## 88 · Merge Sorted Array（Easy）
[題目](https://leetcode.com/problems/merge-sorted-array/) · [程式](0088-merge-sorted-array.c)

**題意**：`nums1` 前 m 個、`nums2` 的 n 個都已排序。把 `nums2` 合併進 `nums1`，`nums1` 後面剛好留了 n 格空位。

**思路**：從**尾端**往前放。每次比較兩邊目前最大的，把大的放到 `nums1[k]`。
```
nums1 = [1 2 3 _ _ _]   nums2 = [2 5 6]
               i     k            j
6 > 3 → 放 6；5 > 3 → 放 5；3 > 2 → 放 3；2 = 2 → 放 nums2 的 2 ...
```
從前面放的話會蓋掉 `nums1` 還沒比較的元素；從後面放，寫入的位置永遠在 i 的後面，所以很安全。

**複雜度**：O(m + n)，額外空間 O(1)。

**陷阱**：
- 迴圈條件是 `j >= 0`（nums2 還沒放完）。nums1 剩下的元素本來就在正確位置，不用搬。
- m = 0 時，i 一開始就是 -1，要先檢查 `i >= 0`。

---

## 26 · Remove Duplicates from Sorted Array（Easy）
[題目](https://leetcode.com/problems/remove-duplicates-from-sorted-array/) · [程式](0026-remove-duplicates-from-sorted-array.c)

**題意**：原地移除已排序陣列中的重複值，回傳剩下幾個。

**思路：快慢指標**
- `fast` 往前掃每一個元素。
- `slow` 是「下一個不重複的值要寫的位置」。
- `nums[fast]` 跟已經保留的最後一個 `nums[slow-1]` 不同，才寫進去。
```
[0 0 1 1 1 2]
 s f           0 == 0 跳過
   s   f       1 != 0 → 寫到 s
     s     f   2 != 1 → 寫到 s
結果 [0 1 2 ...]，回傳 3
```

**複雜度**：O(n)，額外空間 O(1)。

**陷阱**：因為已經排好序，重複的值一定相鄰，只要跟前一個比就好。沒排序的話要用 hash set。

---

## 189 · Rotate Array（Medium）
[題目](https://leetcode.com/problems/rotate-array/) · [程式](0189-rotate-array.c)

**題意**：把陣列往右轉 k 格。

| 做法 | 時間 | 空間 |
|---|---|---|
| 每次轉 1 格，做 k 次 | O(nk) | O(1) |
| 開新陣列：`new[(i+k)%n] = old[i]` | O(n) | O(n) |
| **三次反轉**（本檔） | O(n) | **O(1)** |

**三次反轉**：
```
[1 2 3 4 5 6 7], k = 3
整個反轉:   [7 6 5 4 3 2 1]
前 k 個:    [5 6 7 4 3 2 1]
後 n-k 個:  [5 6 7 1 2 3 4] ✅
```
為什麼可以？往右轉 k 格，就是把「後 k 個」搬到前面。整個反轉會把後段換到前面，但兩段內部的順序也跟著反了，所以再各自反轉一次轉回來。

**陷阱**：**k 可能比 n 大**，一定要先 `k %= n`。

---

## 238 · Product of Array Except Self（Medium）
[題目](https://leetcode.com/problems/product-of-array-except-self/) · [程式](0238-product-of-array-except-self.c)

**題意**：`answer[i]` = 除了 `nums[i]` 以外所有數的乘積。**不能用除法**，要 O(n)。

**思路**：answer[i] = 左邊全部的乘積 × 右邊全部的乘積。
```
nums:       1   2   3   4
左乘積:     1   1   2   6     ← 第一趟，左到右
右乘積:    24  12   4   1     ← 第二趟，右到左
answer:    24  12   8   6
```
第二趟不用另外開陣列：用一個變數 `right` 邊走邊乘，直接乘進 `answer`。

**複雜度**：O(n)，除了回傳的陣列之外只用 O(1) 空間。

**陷阱**：
- 為什麼不能用「全部乘起來再除以 nums[i]」？因為遇到 **0** 就不能除。題目也明確禁止用除法。
- 題目保證乘積放得進 32-bit int。

---

## 48 · Rotate Image（Medium）
[題目](https://leetcode.com/problems/rotate-image/) · [程式](0048-rotate-image.c)

**題意**：n×n 矩陣原地順時針轉 90 度。

**思路**：順時針 90° = **轉置** + **每列左右翻轉**。
```
1 2 3      1 4 7      7 4 1
4 5 6  →   2 5 8  →   8 5 2
7 8 9      3 6 9      9 6 3
   轉置        左右翻轉
```
記法：原本 `(i, j)` 的元素會跑到 `(j, n-1-i)`。轉置把它送到 `(j, i)`，左右翻轉再送到 `(j, n-1-i)`。

**複雜度**：O(n²)，額外空間 O(1)。

**陷阱**：
- 轉置時 j 只能從 `i + 1` 開始（只換上三角）。整個矩陣都換的話，每一對會被換兩次，等於沒換。
- 逆時針 90° = 轉置 + **上下**翻轉。
