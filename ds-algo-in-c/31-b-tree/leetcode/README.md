# B-Tree · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

LeetCode 沒有要求實作 B-Tree 的題目。這裡把 B-Tree 當作**有序集合**來用。兩題都內嵌同一份精簡版 B-Tree：
- t = 4：每個節點最多 7 個 key，key 和 val 直接用陣列存在節點裡。
- 只有插入（CLRS 的一趟式插入 + 主動分裂），這兩題都不需要刪除。

執行：`make test T=31-b-tree`

---

## 729 · My Calendar I（Medium，B-Tree 版）
[題目](https://leetcode.com/problems/my-calendar-i/) · [程式](0729-my-calendar-i.c)

**題意**：同 18 主題：不斷加入預約 `[start, end)`，跟已有的重疊就拒絕。

**思路**：B-Tree 以 start 當 key、end 當 val。新的 `[s, e)` 只要檢查兩個鄰居：
- `bt_lower(s)`：第一個 start ≥ s 的預約，它的 start 必須 ≥ e。
- `bt_before(s)`：最後一個 start < s 的預約，它的 end 必須 ≤ s。

**在 B-Tree 上找「第一個 ≥ k」**：在每個節點找到第一個 ≥ k 的 key，它是候選；更小的候選只可能在它左邊的小孩裡，所以往 `c[i]` 走下去，一路更新候選。「最後一個 < k」是對稱的做法。

**複雜度**：每次 O(t · log_t n)。

**跟紅黑樹版比較**：演算法完全一樣，只是換了底層的有序集合。B-Tree 的節點比較大，程式也比較短（這一題不用刪除）。

---

## 352 · Data Stream as Disjoint Intervals（Hard）
[題目](https://leetcode.com/problems/data-stream-as-disjoint-intervals/) · [程式](0352-data-stream-as-disjoint-intervals.c)

**題意**：數字一個一個加進來（可能重複）。隨時要回傳「目前所有數字合併成的不相交區間」，由小到大。
```
加入 1, 3, 7 → [1,1] [3,3] [7,7]
加入 2      → [1,3] [7,7]
加入 6      → [1,3] [6,7]
```

**思路**：
- `addNum`：已經有了就跳過，否則插入 B-Tree。O(log n)。
- `getIntervals`：**中序走訪**（B-Tree 的中序也是由小到大），邊走邊合併：跟上一段的結尾剛好差 1 就延長，否則開新的一段。O(n)。

**為什麼 get 做 O(n) 可以**：題目說 `addNum` 最多 3 × 10⁴ 次、`getIntervals` 最多 10² 次。

**進階做法**：如果 get 也很多次，就改成直接存「區間」：插入 v 時找 v 左邊和右邊的區間，看能不能合併（需要有序集合的 floor/ceil 和刪除）。這樣 addNum 和維護都是 O(log n)。

**陷阱**：
- 重複加入同一個數字，結果不能變。
- 在 B-Tree 上做中序走訪：`c[0], key[0], c[1], key[1], ..., key[n−1], c[n]`，最後一個小孩別漏掉。
