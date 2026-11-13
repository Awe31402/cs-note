# Linked List · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

執行：`make test T=05-linked-list`

---

## 876 · Middle of the Linked List（Easy）
[題目](https://leetcode.com/problems/middle-of-the-linked-list/) · [程式](0876-middle-of-the-linked-list.c)

**題意**：回傳中間節點。偶數個時回傳第二個中間。

**思路：快慢指標**。fast 每次走 2 步，slow 每次走 1 步。fast 走完全程時，slow 剛好走一半。
```
1 → 2 → 3 → 4 → 5
s,f
    s       f
        s           f(最後一個) → 停，slow = 3
```

**複雜度**：O(n)，額外空間 O(1)。暴力做法是先數長度再走一半，要走兩趟。

**陷阱**：迴圈條件寫 `fast && fast->next`，偶數個時會停在**第二個**中間。如果要第一個中間，改成 `fast->next && fast->next->next`。

---

## 141 · Linked List Cycle（Easy）
[題目](https://leetcode.com/problems/linked-list-cycle/) · [程式](0141-linked-list-cycle.c)

**題意**：串列裡有沒有環。

**思路：Floyd 判環（龜兔賽跑）**
- 沒有環：fast 會先走到 NULL。
- 有環：兩個都進到環裡以後，fast 每一步都比 slow 多靠近 1 格，所以一定追得上，不會「跳過」slow。

| 做法 | 時間 | 空間 |
|---|---|---|
| 用 hash set 記錄走過的節點 | O(n) | O(n) |
| **快慢指標**（本檔） | O(n) | **O(1)** |

**陷阱**：
- 比較的是 `slow == fast`（位址），不是 `val`。
- 延伸題 142：找環的**起點**。相遇後把一個指標放回 head，兩個都改成每次走 1 步，再相遇的地方就是起點。

---

## 234 · Palindrome Linked List（Easy）
[題目](https://leetcode.com/problems/palindrome-linked-list/) · [程式](0234-palindrome-linked-list.c)

**題意**：串列的值是不是回文。進階要求 O(1) 空間。

**思路**：
```
1 → 2 → 3 → 2 → 1
1. 快慢指標找中間：slow = 3
2. 從 slow 開始反轉後半段：1 → 2 → 3 ← 2 ← 1（second 指向最後的 1）
3. 從兩頭往中間比：1=1、2=2、3=3 ✅
4. 把後半段反轉回來
```

**複雜度**：O(n)，額外空間 O(1)。簡單做法是複製到陣列再用雙指標，要 O(n) 空間。

**陷阱**：
- 會暫時改到輸入的串列，最後要轉回來。面試時要說明這一點。
- 比較的迴圈以**後半段** `q` 為準。奇數個時中間那個節點會跟自己比，不影響結果。

---

## 19 · Remove Nth Node From End of List（Medium）
[題目](https://leetcode.com/problems/remove-nth-node-from-end-of-list/) · [程式](0019-remove-nth-node-from-end-of-list.c)

**題意**：刪除倒數第 n 個節點，只走一趟。

**思路：前後指標**，兩個指標之間保持 n 格的距離。
```
dummy → 1 → 2 → 3 → 4 → 5,  n = 2
fast 先走 2 步：fast = 2
一起走到 fast 是最後一個：slow = 3, fast = 5
slow->next（4）就是倒數第 2 個 → 刪掉
```
要停在「**前一個**」節點才能刪，所以兩個指標都從 dummy 出發。

**複雜度**：O(n)，一趟。

**陷阱**：
- **要刪的是第一個節點**（n = 長度）時，沒有 dummy 就要特別處理。有了 dummy，slow 會停在 dummy，照樣處理就好。
- LeetCode 上不 free 也會過，但本機有 ASan，會抓到記憶體洩漏。

---

## 2 · Add Two Numbers（Medium）
[題目](https://leetcode.com/problems/add-two-numbers/) · [程式](0002-add-two-numbers.c)

**題意**：兩個非負整數用串列**反向**存（個位數在最前面），回傳它們的和，也用同樣的格式。

**思路**：就是直式加法。反向存剛好讓我們從個位數開始加。
```
  2 → 4 → 3      (342)
+ 5 → 6 → 4      (465)
= 7 → 0 → 8      (807)   4+6=10，寫 0 進 1
```

**複雜度**：O(max(n, m))。

**陷阱**：
- **兩條長度不同**：短的那條走完就當 0。
- **最後還有進位**：迴圈條件要寫 `l1 || l2 || carry`，否則 99 + 1 會少掉最前面的 1。
- **不能轉成整數再相加**：最多 100 位數，`long long` 裝不下。
- 延伸題 445：數字是**正向**存的。要先反轉，或用 stack。
