# Topological Sort · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

執行：`make test T=21-topological-sort`

---

## 207 · Course Schedule（Medium）
[題目](https://leetcode.com/problems/course-schedule/) · [程式](0207-course-schedule.c)

**題意**：`prerequisites[i] = [a, b]` 代表「修 a 之前要先修 b」。能不能修完全部的課？

**思路**：課是頂點，先修關係是邊 **b → a**。修得完 ⇔ 圖裡**沒有環**。
用 Kahn 演算法：能排出全部 n 門課就沒有環。

```
[[1,0],[0,1]]：0 → 1 且 1 → 0
入度都是 1，一開始沒有入度 0 的課 → 排出 0 門 < 2 → false
```

**複雜度**：O(V + E)。

**陷阱**：
- **邊的方向**：`[a, b]` 是 b → a，不要寫反。
- 課可能完全沒有先修條件，`prerequisitesSize` 可能是 0。
- 自己是自己的先修課 `[0, 0]` 也是環。
- 本檔用 CSR 存圖（先數每個頂點的出度，再用前綴和分段），不用每個頂點各自 realloc。

---

## 210 · Course Schedule II（Medium）
[題目](https://leetcode.com/problems/course-schedule-ii/) · [程式](0210-course-schedule-ii.c)

**題意**：同 207，但要回傳一種修課順序；做不到就回傳空陣列。

**思路**：Kahn 演算法拿出來的順序就是答案。`order` 陣列同時當 queue 用。

**複雜度**：O(V + E)。

**陷阱**：
- 答案不唯一，測試要檢查「每個先修課都排在前面」，不能跟固定答案比。
- 有環時回傳空陣列：把 `*returnSize` 設成 0 就好（陣列本身還是要 malloc，讓呼叫端可以 free）。

---

## 802 · Find Eventual Safe States（Medium）
[題目](https://leetcode.com/problems/find-eventual-safe-states/) · [程式](0802-find-eventual-safe-states.c)

**題意**：從一個節點出發，如果**每一條**路最後都會停在終點（沒有出邊的節點），它就是「安全」的。由小到大回傳所有安全節點。

**思路：三色 DFS**
- 白：還沒檢查。
- 灰：正在目前的路徑上。**走到灰色 → 有環 → 不安全**。
- 黑（SAFE）：已確定安全。
- 一個節點安全 ⇔ 它的**所有**鄰居都安全。

關鍵技巧：發現不安全時，**讓節點維持灰色**就直接回傳。之後別人再走到它，看到灰色就知道不安全，不用重新檢查。

**複雜度**：O(V + E)，每個節點只會被完整處理一次。

**其他做法**：把所有邊反過來，從終點開始做 Kahn（用「出度」代替「入度」），能被排到的就是安全節點。
