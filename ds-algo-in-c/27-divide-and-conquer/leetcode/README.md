# Divide & Conquer · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

執行：`make test T=27-divide-and-conquer`

---

## 169 · Majority Element（Easy）
[題目](https://leetcode.com/problems/majority-element/) · [程式](0169-majority-element.c)

**題意**：找出現超過 n/2 次的元素（保證存在）。

**思路（分治，本檔）**：
- 左半的多數元素 l、右半的多數元素 r。
- **整段的多數元素一定是 l 或 r**：假設 x 在兩半都沒有過半，那它在整段的出現次數 ≤ n/4 + n/4 = n/2，不可能過半。
- l == r 就是答案；不同的話，各數一次，誰多就是誰。

**複雜度**：T(n) = 2T(n/2) + O(n) = O(n log n)。

**面試最佳解：Boyer-Moore 投票法**，O(n) 時間、O(1) 空間：
```c
int cand = 0, cnt = 0;
for (i...) {
    if (cnt == 0) cand = nums[i];
    cnt += nums[i] == cand ? 1 : -1;   // 不同的兩個互相抵銷
}
return cand;
```
多數元素超過一半，就算每一個都被別人抵銷一次，最後還是會剩下它。

**其他做法**：排序後取中間那個 O(n log n)；hash 計數 O(n) 時間、O(n) 空間。

---

## 53 · Maximum Subarray（Medium，分治版）
[題目](https://leetcode.com/problems/maximum-subarray/) · [程式](0053-maximum-subarray.c)

**題意**：同 01 主題：和最大的連續子陣列。這題的 Follow-up 就是要求分治版。

**思路**：切在中點，答案只有三種：
1. 完全在左半 → 遞迴。
2. 完全在右半 → 遞迴。
3. **跨過中點** → 必須包含 `a[mid]` 和 `a[mid+1]`。從 mid 往左一路加，記最大值；從 mid+1 往右一路加，記最大值；兩個相加。

**複雜度**：O(n log n)。Kadane 是 O(n)。

**陷阱**：跨中點的左右延伸，一定要**至少包含** `a[mid]` 和 `a[mid+1]`，所以初始值是 `a[mid]`、`a[mid+1]`，不是 0。從 0 開始的話，全部是負數時會算錯。

---

## 241 · Different Ways to Add Parentheses（Medium）
[題目](https://leetcode.com/problems/different-ways-to-add-parentheses/) · [程式](0241-different-ways-to-add-parentheses.c)

**題意**：算式像 `"2*3-4*5"`，用所有不同的方式加括號，回傳所有可能的計算結果。

**思路**：每一種加括號的方式，都有一個「**最後才算的運算子**」，它把算式切成左右兩段。
```
"2*3-4*5"
以 * 切：2  |  3-4*5  → 左 {2}，右 {-17, -5}  → {-34, -10}
以 - 切：2*3 | 4*5    → 左 {6}，右 {20}      → {-14}
以 * 切：2*3-4 | 5    → 左 {2, -2}，右 {5}   → {10, -10}
全部：{-34, -10, -14, 10, -10}
```
沒有運算子的一段就是一個數字，這是 base case。

**複雜度**：結果的個數是 Catalan 數（約 4ⁿ / n^1.5），本身就是指數級的。

**優化**：同一段子字串會被重複計算，可以用 memo 記住 `[lo, hi)` 的結果，這就變成動態規劃了（第 28 主題）。

**陷阱**：
- 數字可能有兩位數，例如 `"42"`，要把連續的數字字元組起來。
- 結果可以重複（`-10` 出現兩次），不要去重。
