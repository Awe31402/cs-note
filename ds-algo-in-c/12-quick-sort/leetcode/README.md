# Quick Sort · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

執行：`make test T=12-quick-sort`

---

## 905 · Sort Array By Parity（Easy）
[題目](https://leetcode.com/problems/sort-array-by-parity/) · [程式](0905-sort-array-by-parity.c)

**題意**：偶數放前面、奇數放後面，順序不限。

**思路**：這就是 quicksort 的 **partition**，只是把「≤ pivot」換成「是偶數」。
- 左指標往右找奇數，右指標往左找偶數。
- 兩個都找到了就交換。

**複雜度**：O(n)，額外空間 O(1)。

**陷阱**：這個做法不穩定。如果題目要求保持原本的相對順序，要改用 283 Move Zeroes 那種寫法。

---

## 75 · Sort Colors（Medium）
[題目](https://leetcode.com/problems/sort-colors/) · [程式](0075-sort-colors.c)

**題意**：陣列只有 0、1、2，要原地排好，最好一趟完成。

**思路：三路切分（荷蘭國旗問題）**，以 1 當 pivot：
```
[ 0 0 | 1 1 | ? ? ? | 2 2 ]
       lo    i     hi
a[i] = 0 → 跟 a[lo] 交換，lo++、i++
a[i] = 2 → 跟 a[hi] 交換，hi--（i 不動！）
a[i] = 1 → i++
```

**複雜度**：一趟 O(n)，額外空間 O(1)。計數排序要兩趟（先數再寫）。

**陷阱**：
- **跟 hi 交換後，i 不能前進**：換過來的元素還沒檢查過，可能是 0。
- 跟 lo 交換後可以前進：換過來的一定是 1（`[lo, i)` 都是 1），或 lo == i 時就是自己。
- 迴圈條件是 `i <= hi`，不是 `i < n`。

---

## 215 · Kth Largest Element in an Array（Medium，Quickselect 版）
[題目](https://leetcode.com/problems/kth-largest-element-in-an-array/) · [程式](0215-kth-largest-element-in-an-array.c)

**題意**：找第 k 大的元素。

**思路：Quickselect**
- 第 k 大 = 由小到大排序後 index 為 `n - k` 的元素。
- partition 之後，pivot 的最終位置是 p：
  - `target == p` → 找到了。
  - `target < p` → 只要往左邊找。
  - `target > p` → 只要往右邊找。

| 做法 | 時間 | 空間 |
|---|---|---|
| 排序 | O(n log n) | — |
| 大小 k 的 min-heap（13 主題） | O(n log k) | O(k) |
| **Quickselect**（本檔） | 平均 **O(n)**，最壞 O(n²) | O(1) |
| Median of medians（CLRS 9.3） | 最壞 O(n) | 常數很大，實務不用 |

**陷阱**：
- **一定要隨機選 pivot**，LeetCode 有專門卡固定 pivot 的測資。
- **一定要用三路切分**：LeetCode 有「大量相同值」的測資，兩路切分會退化成 O(n²) 而超時。這是這題最常見的超時原因。

---

## 973 · K Closest Points to Origin（Medium）
[題目](https://leetcode.com/problems/k-closest-points-to-origin/) · [程式](0973-k-closest-points-to-origin.c)

**題意**：回傳離原點最近的 k 個點，順序不限。

**思路**：用「距離平方」當比較值做 quickselect，讓 index `k - 1` 就定位。這時前 k 格剛好就是最近的 k 個（partition 保證左邊都 ≤ 它）。

**複雜度**：平均 O(n)。heap 版是 O(n log k)，排序版是 O(n log n)。

**陷阱**：
- **不要開根號**：比大小用平方就夠了，還能避開浮點數誤差。
- 只交換 `int*` 指標就好，不用複製座標。
- C 的回傳格式：`int**` 加上 `returnColumnSizes`（每列長度都是 2），兩者都要 malloc。
