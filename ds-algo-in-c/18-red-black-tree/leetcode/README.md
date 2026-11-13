# Red-Black Tree · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

C 沒有內建的有序集合（像 C++ 的 `std::set`），所以每題都內嵌一份**精簡版紅黑樹**，演算法跟 `rbtree.c` 完全一樣，只是變數名稱比較短，而且 key 改成 `long long`。每題只放它用得到的函式，否則 `-Werror` 會報「函式沒用到」的錯誤。

執行：`make test T=18-red-black-tree`

---

## 729 · My Calendar I（Medium）
[題目](https://leetcode.com/problems/my-calendar-i/) · [程式](0729-my-calendar-i.c)

**題意**：一直加入預約 `[start, end)`。如果跟已有的預約重疊就拒絕，否則接受。

**思路**：已接受的預約彼此不重疊，所以用 start 排序後，**只要檢查新區間的兩個鄰居**：
```
已有：[10,20)  [20,30)  [40,50)
新的：[25,35)
  下一個 = 第一個 start ≥ 25 → [40,50)：40 ≥ 35 ✅
  上一個 = 最後一個 start < 25 → [20,30)：30 > 25 ❌ 重疊
```
- 紅黑樹以 start 當 key、end 當 val。
- `t_lower(s)` 找下一個，`t_before(s)` 找上一個，都是 O(log n)。

**複雜度**：每次 O(log n)。用陣列逐一檢查是 O(n)，總共 O(n²)。

**陷阱**：區間是半開的 `[s, e)`，所以 `[10,20)` 和 `[20,30)` **不算**重疊。判斷要用 `next->start < e` 和 `prev->end > s`，都是嚴格的比較。

---

## 1845 · Seat Reservation Manager（Medium）
[題目](https://leetcode.com/problems/seat-reservation-manager/) · [程式](1845-seat-reservation-manager.c)

**題意**：座位 1..n。`reserve()` 回傳目前**最小**的空位並訂下；`unreserve(k)` 退回座位 k。

**思路**：
- `next` = 從來沒被訂過的最小座位號。
- 被退回的座位放進有序集合。
- `reserve`：集合不是空的 → 取最小的並刪掉；空的 → 回傳 `next++`。

為什麼可以這樣：被退回的座位一定是訂過的，所以一定比 `next` 小。集合裡只要有東西，它的最小值就是全部空位裡最小的。

**複雜度**：每次 O(log n)。

**其他做法**：min-heap 更簡單（13 主題），效果一樣。這題用紅黑樹，是為了示範「取最小 + 刪除任意節點」。如果題目還要求「找大於 x 的最小空位」，heap 就做不到了，一定要用有序集合。

---

## 220 · Contains Duplicate III（Hard）
[題目](https://leetcode.com/problems/contains-duplicate-iii/) · [程式](0220-contains-duplicate-iii.c)

**題意**：是否存在 i ≠ j，使得 `|i − j| ≤ indexDiff` 而且 `|nums[i] − nums[j]| ≤ valueDiff`？

**思路：滑動視窗 + 有序集合**
- 集合裡只放「最近 indexDiff 個」數字（index 的條件）。
- 新的 x 進來時，要找集合裡有沒有落在 `[x − valueDiff, x + valueDiff]` 的數：
  - `c = lower_bound(x − valueDiff)`（第一個 ≥ x − valueDiff 的數）
  - 如果 `c ≤ x + valueDiff` → 找到了。
- 然後把 x 放進去；視窗超過 indexDiff 個就把最舊的刪掉。

```
nums = [1 5 9 1 5 9], indexDiff = 2, valueDiff = 3
i=0 集合 {}      放 1 → {1}
i=1 找 [2,8]     沒有 → {1,5}
i=2 找 [6,12]    沒有 → {1,5,9} → 刪 1 → {5,9}
i=3 找 [-2,4]    沒有 → {1,5,9} → 刪 5 → {1,9}
...
```

**複雜度**：O(n log k)，k = indexDiff。暴力是 O(nk)。

**陷阱**：
- **溢位**：`x − valueDiff` 和 `x + valueDiff` 可能超出 int（例如 x = INT_MIN），要用 `long long`。
- **重複值**：如果 x 已經在集合裡，`lower_bound` 一定會找到它（差距 0 ≤ valueDiff），直接回傳 true。所以集合裡不會有重複值，刪除時不會刪錯。
- **另一個 O(n) 解法**：桶子。桶寬設成 valueDiff + 1，同一桶裡的兩個數一定符合條件，只要再檢查相鄰的兩個桶（跟 14 主題 164 題的想法類似）。
