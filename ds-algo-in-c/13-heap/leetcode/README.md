# Heap · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

執行：`make test T=13-heap`

---

## 703 · Kth Largest Element in a Stream（Easy）
[題目](https://leetcode.com/problems/kth-largest-element-in-a-stream/) · [程式](0703-kth-largest-element-in-a-stream.c)

**題意**：數字一個一個進來，每次都要回答「目前第 k 大的是多少」。

**思路**：只留**最大的 k 個**，放在一個 **min-heap** 裡。
- 堆頂是這 k 個裡最小的，也就是第 k 大。
- 新數字比堆頂大 → 取代堆頂，再往下沉。
- 新數字比堆頂小 → 它連前 k 名都排不上，直接丟掉。

```
k = 3，目前 heap = {4, 5, 8}，堆頂 4
add 10 → 10 > 4，取代 → {5, 8, 10}，堆頂 5 ✅
add 2  → 2 < 5，丟掉 → 堆頂還是 5
```

**複雜度**：每次 add 是 O(log k)，空間 O(k)。

**陷阱**：
- 直覺會想用 max-heap，但**第 k 大要用 min-heap**。
- 一開始 `nums` 可能少於 k 個，所以還沒滿 k 個時要直接放進去。

---

## 215 · Kth Largest Element in an Array（Medium）
[題目](https://leetcode.com/problems/kth-largest-element-in-an-array/) · [程式](0215-kth-largest-element-in-an-array.c)

**題意**：找陣列中第 k 大的數（重複的也算，例如 `[3,3]` 的第 2 大是 3）。

**思路**：跟 703 一樣，只是資料一次全部給你。
1. 前 k 個用 `build` 建成 min-heap，這步是 O(k)。
2. 剩下的逐一比較堆頂，比堆頂大就取代。

**複雜度**：時間 O(n log k)，空間 O(k)。

**其他解法**：
| 方法 | 時間 | 備註 |
|---|---|---|
| 排序後取 `[n-k]` | O(n log n) | 最簡單，面試可先講這個 |
| 大小 k 的 min-heap | O(n log k) | 本檔解法，k 小時很快 |
| Quickselect | 平均 O(n)，最壞 O(n²) | CLRS 9.2。面試官常追問。第 12 主題（Quick Sort）會實作 |

**陷阱**：這題要的是「第 k 大」，不是「第 k 個不同的值」，重複的值也要算。

---

## 347 · Top K Frequent Elements（Medium）
[題目](https://leetcode.com/problems/top-k-frequent-elements/) · [程式](0347-top-k-frequent-elements.c)

**題意**：回傳出現次數最多的 k 個數，順序不限。

**思路**：兩步。
1. **數次數**：題目保證 `-10^4 ≤ nums[i] ≤ 10^4`，所以開一個大小 20001 的陣列 `cnt[v + 10000]`，不用 hash table。
2. **挑前 k 名**：大小 k 的 min-heap。heap 裡放「值」，但比大小時看的是 `cnt[值]`。

**複雜度**：時間 O(n + R log k)，R 是值的範圍（20001）。空間 O(R)。

**其他解法**：**Bucket sort**，時間 O(n)。
開 `bucket[次數]`，把每個值丟進它次數對應的桶子，再從次數最高的桶子往回拿，拿滿 k 個為止。面試官問「能不能比 O(n log k) 更好」時就講這個。

**陷阱**：
- 值可能是負數，當陣列 index 前要先加 offset。
- 如果值的範圍很大，例如 ±10^9，就一定要用 hash table（第 08 主題）。

---

## 23 · Merge k Sorted Lists（Hard）
[題目](https://leetcode.com/problems/merge-k-sorted-lists/) · [程式](0023-merge-k-sorted-lists.c)

**題意**：k 條已經排好序的 linked list，要合併成一條。

**思路**：每條串列的最小值都在它的頭。把 k 個頭放進 **min-heap**：
1. 取出堆頂（全部裡面最小的），接到答案後面。
2. 如果它還有 `next`，就把 `next` 放回 heap。程式裡是直接取代堆頂再往下沉，比 pop 再 push 少一次。
3. heap 空了就結束。

```
heap: [1(A) 1(B) 2(C)]  → 取 1(A)，放 4(A)
heap: [1(B) 2(C) 4(A)]  → 取 1(B)，放 3(B)
...
```

**複雜度**：N 是所有節點總數。時間 O(N log k)，空間 O(k)。

**其他解法**：兩兩合併（divide & conquer）也是 O(N log k)，而且不需要 heap。

**陷阱**：
- `lists` 裡可能有 `NULL`，也就是空串列，**不能放進 heap**，否則讀 `->val` 會當掉。
- `listsSize` 可能是 0。
- 用 `dummy` 假頭節點，就不用特別處理「答案的第一個節點」。
- 逐條依序合併是 O(kN)，太慢。

---

## 295 · Find Median from Data Stream（Hard）
[題目](https://leetcode.com/problems/find-median-from-data-stream/) · [程式](0295-find-median-from-data-stream.c)

**題意**：數字一個一個進來，隨時要能回答中位數。

**思路**：把資料切成兩半，用**兩個 heap**。
```
     low（max-heap）       high（min-heap）
   較小的一半，堆頂最大    較大的一半，堆頂最小
   [1, 2, 3]  → 堆頂 3  |  堆頂 4 ←  [4, 5]
                      中位數在這條線上
```
- 規則 1：low 的所有值 ≤ high 的所有值。
- 規則 2：`low.size == high.size` 或多 1 個。

**addNum 的三步**：
1. 先放進 low。
2. 把 low 的最大值移到 high。這樣保證規則 1。
3. 如果 high 比 low 多，就把 high 的最小值移回 low。這樣保證規則 2。

**findMedian**：
- 總數是奇數 → low 的堆頂。
- 總數是偶數 → 兩個堆頂的平均。

**複雜度**：addNum O(log n)，findMedian O(1)。

**陷阱**：
- 算平均時要先轉成 `double`，否則兩個 int 相加可能溢位，整數除法也會把 .5 吃掉。
- C 沒有內建 heap，所以這題要自己寫 max 和 min 兩種。本檔用 `is_max` 旗標共用同一份程式。
- 另一個常見技巧是存 `-x`，讓 min-heap 當 max-heap 用。但如果 `x == INT_MIN`，`-x` 會溢位，要小心。
