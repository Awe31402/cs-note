# Merge Sort · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

執行：`make test T=11-merge-sort`

---

## 977 · Squares of a Sorted Array（Easy）
[題目](https://leetcode.com/problems/squares-of-a-sorted-array/) · [程式](0977-squares-of-a-sorted-array.c)

**題意**：排好序的陣列（可能有負數），回傳每個數平方後排好序的結果。

**思路**：平方後最大的值一定在**兩端**（最負的或最正的）。
- 左右各一個指標，比較平方，大的放到答案的**尾端**，然後往中間走。
- 這就是 merge：把「負數部分反過來」和「正數部分」這兩個排好的序列合併起來。

```
[-4 -1 0 3 10]
 l          r    100 > 16 → ans[4] = 100
 l        r      16 > 9   → ans[3] = 16
    l     r      9 > 1    → ans[2] = 9 ...
```

**複雜度**：O(n)。平方完再排序是 O(n log n)。

**陷阱**：答案要從**後面**往前填，因為每次找到的是剩下裡面最大的。

---

## 912 · Sort an Array（Medium）
[題目](https://leetcode.com/problems/sort-an-array/) · [程式](0912-sort-an-array.c)

**題意**：不用內建函式，把陣列排好，要 O(n log n)。

**思路**：標準 merge sort，暫存陣列只配置一次。

**為什麼選 merge sort 而不是 quicksort**：這題的測資裡有「已排序」和「全部相同」的陣列。固定選第一個當 pivot 的 quicksort 會退化成 O(n²) 而超時。Quicksort 要用隨機 pivot 加三路切分才安全（第 12 主題）。

**複雜度**：O(n log n)，額外空間 O(n)。

**陷阱**：遞迴裡每次都 malloc 暫存陣列，會慢很多，也容易忘記 free。

---

## 148 · Sort List（Medium）
[題目](https://leetcode.com/problems/sort-list/) · [程式](0148-sort-list.c)

**題意**：排序一條串列，要 O(n log n)。

**思路**：串列上的 merge sort。
1. **切**：快慢指標找中間，把 `slow->next` 設成 NULL，切成兩條。
2. **遞迴**：兩條各自排好。
3. **合併**：就是 LeetCode 21（合併兩條已排序串列）。

**為什麼串列適合 merge sort**：
- 合併只要改指標，**不需要額外的陣列**。
- 不需要隨機存取。Quicksort 和 heapsort 都很依賴 index，放在串列上很難寫。

**複雜度**：O(n log n) 時間，O(log n) 遞迴堆疊。題目的進階要求 O(1) 空間，要用由下而上的版本：每次合併長度 1、2、4…… 的片段。

**陷阱**：**fast 要從 `head->next` 出發。** 只有兩個節點時，如果 fast 從 head 出發，slow 會停在第二個，切出來還是「兩個 + 零個」，就會無限遞迴。

---

## 315 · Count of Smaller Numbers After Self（Hard）
[題目](https://leetcode.com/problems/count-of-smaller-numbers-after-self/) · [程式](0315-count-of-smaller-numbers-after-self.c)

**題意**：對每個 `nums[i]`，算它**右邊**有幾個比它小的數。

**思路：merge sort 的時候順便數**
- 合併時，左半的元素 x 要放下去之前，右半已經放下去 `j - mid` 個元素。
- 那些元素都比 x 小（因為先被放下去），而且原本都在 x 的**右邊**（因為來自右半）。
- 所以 `count[x] += j - mid`。

**關鍵技巧：排序的是 index，不是值。** 值排序之後就不知道原本是哪一個了；排 index 才知道要把計數加到誰身上。

```
nums = [5 2 6 1]
合併 [5] [2]：放 2（右），再放 5 → count[5] += 1
合併 [6] [1]：放 1（右），再放 6 → count[6] += 1
合併 [2 5] [1 6]：放 1（右）；放 2 → count[2] += 1；放 5 → count[5] += 1；放 6
答案 [2 1 1 0]
```

**複雜度**：O(n log n)。暴力解 O(n²) 會超時（n = 10⁵）。

**陷阱**：
- 比較要用**嚴格小於**：值相等時先放左邊的，這樣相等的數不會被算成「比較小」。
- 左半剩下的元素（右半已經放完了）也要加上 `j - mid`。
- 其他解法：Binary Indexed Tree（Fenwick Tree）或 Segment Tree（第 32 主題），都是 O(n log n)。
