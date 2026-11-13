# Binary Search · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

執行：`make test T=09-binary-search`

---

## 704 · Binary Search（Easy）
[題目](https://leetcode.com/problems/binary-search/) · [程式](0704-binary-search.c)

**題意**：在排好序、值都不同的陣列裡找 target，回傳 index，找不到回傳 -1。

**思路**：標準模板，左閉右開 `[lo, hi)`。
- `nums[mid] < target` → `lo = mid + 1`
- `nums[mid] > target` → `hi = mid`

**複雜度**：O(log n)。

**陷阱**：`mid = lo + (hi - lo) / 2`，不要寫 `(lo + hi) / 2`。

---

## 35 · Search Insert Position（Easy）
[題目](https://leetcode.com/problems/search-insert-position/) · [程式](0035-search-insert-position.c)

**題意**：找 target 的位置；不存在的話，回傳它應該插入的位置。

**思路**：這就是 **lower_bound**：第一個 `>= target` 的位置。
- 存在 → 那個位置就是 target。
- 不存在 → 那個位置就是插入點。

**複雜度**：O(log n)。

**陷阱**：target 比全部都大時要回傳 n，所以 `hi` 一定要從 n 開始，不能從 n - 1 開始。

---

## 34 · Find First and Last Position of Element in Sorted Array（Medium）
[題目](https://leetcode.com/problems/find-first-and-last-position-of-element-in-sorted-array/) · [程式](0034-find-first-and-last-position-of-element-in-sorted-array.c)

**題意**：有重複值的排序陣列，找 target 第一次和最後一次出現的位置。

**思路：兩次 lower_bound**
```
[5 7 7 8 8 10], target = 8
lower_bound(8) = 3   ← 第一個 >= 8
lower_bound(9) = 5   ← 第一個 >= 9，減 1 就是最後一個 8
答案 [3, 4]
```

**複雜度**：O(log n)。找到一個再往兩邊線性擴展，最壞是 O(n)（全部都是 target 時）。

**陷阱**：
- 先確認 `first < n && nums[first] == target`，不然 target 不存在。
- `target + 1` 在 target = `INT_MAX` 時會溢位。本檔讓 lower_bound 接收 `long long`。另一個做法是改寫成 upper_bound。

---

## 33 · Search in Rotated Sorted Array（Medium）
[題目](https://leetcode.com/problems/search-in-rotated-sorted-array/) · [程式](0033-search-in-rotated-sorted-array.c)

**題意**：排好序的陣列在某處被「旋轉」過（例如 `[4 5 6 7 0 1 2]`），要在 O(log n) 內找到 target。

**思路**：從 mid 切開，**至少有一半是正常排序的**。
```
[4 5 6 7 0 1 2]，mid = 7
左半 [4 5 6 7] 有序（nums[lo] <= nums[mid]）
  target 在 [4, 7) 內 → 往左找
  否則 → 往右找
```
1. 先用 `nums[lo] <= nums[mid]` 判斷左半是不是有序的。
2. 在**有序的那半**判斷 target 在不在範圍內（有序才能用大小判斷）。
3. 在範圍內就往那半找，不在就往另一半找。

**複雜度**：O(log n)。

**陷阱**：
- 要寫 `nums[lo] <= nums[mid]`，不能用 `<`。當 lo == mid（只剩一兩個元素）時，左半只有一個元素，也算有序。
- 這題用**閉區間**比較直覺，因為要讀 `nums[hi]`。模板換了，其他地方也要一起換。
- 延伸題 81：有重複值時，`nums[lo] == nums[mid]` 就判斷不出哪半有序，只能 `lo++`，最壞變成 O(n)。

---

## 875 · Koko Eating Bananas（Medium）
[題目](https://leetcode.com/problems/koko-eating-bananas/) · [程式](0875-koko-eating-bananas.c)

**題意**：有幾堆香蕉，每小時選一堆吃 k 根（那堆不夠 k 根就吃完，這小時也結束）。要在 h 小時內吃完，最小的 k 是多少？

**思路：在答案上二分**
- 吃完一堆要 `ceil(p / k)` 小時。總時數對 k 是**遞減**的。
- 所以「h 小時內吃得完」對 k 來說是 F F F T T T，要找第一個 T。
- 範圍：k 最小 1，最大 `max(piles)`（再快也沒用，一小時最多吃一堆）。

```
piles = [3 6 7 11], h = 8
k=6: 1+1+2+2 = 6 ≤ 8 ✅ → 往左找
k=3: 1+2+3+4 = 10 > 8 ❌ → 往右找
k=4: 1+2+2+3 = 8 ≤ 8 ✅ → 答案 4
```

**複雜度**：O(n log M)，M = max(piles)。

**陷阱**：
- `ceil(p / k)` 用整數寫成 `(p + k - 1) / k`。p 可能到 10⁹，`p + k - 1` 會超過 int，要用 `long long`。
- **總時數可能超過 int**（很多堆、k 很小時），也要用 `long long` 加總。
- 從 1 開始一個一個試，是 O(n · M)，會超時。
