# 11 · Merge Sort 合併排序

## 一句話
**Merge sort（合併排序）** 是**分治法 (divide and conquer)** 的經典例子：把陣列切成兩半、各自排好，再把兩個排好的半邊**合併**起來。

## 圖示
```
             [38 27 43 3 9 82 10]
           /                      \
     [38 27 43 3]              [9 82 10]          ← 切 (divide)
      /        \               /      \
  [38 27]    [43 3]         [9 82]    [10]
   /  \       /  \           /  \
 [38] [27]  [43] [3]       [9] [82]              ← 一個元素：已排好
   \  /       \  /           \  /
  [27 38]    [3 43]         [9 82]    [10]       ← 合併 (combine)
      \        /               \      /
     [3 27 38 43]              [9 10 82]
           \                      /
             [3 9 10 27 38 43 82]
```

### 合併 (merge)：兩個排好的序列，每次拿比較小的頭
```
左 [3 27 38 43]   右 [9 10 82]
    ↑                 ↑        3 < 9  → 拿 3
       ↑              ↑        27 > 9 → 拿 9
       ↑                 ↑     27 > 10 → 拿 10
...
```

### 遞迴式
T(n) = 2T(n/2) + Θ(n)：主定理情況 2，所以 **T(n) = Θ(n log n)**。
遞迴樹有 log n 層，每層合併的總工作量都是 n。
```
層 0:        n            → n
層 1:     n/2  n/2        → n
層 2:  n/4 n/4 n/4 n/4    → n
...                          共 log n 層 → n log n
```

### 由下而上 (bottom-up)
不用遞迴：先把相鄰的每 1 個合併成 2 個一組，再 2 個合併成 4 個，4 → 8 ……

### 用合併來數逆序對
合併時，右半的 `a[j]` 比左半的 `a[i]` 小，就代表 `a[j]` 比左半剩下的 `mid - i` 個全部都小。這些都是逆序對，所以一次可以加 `mid - i` 個。

## 複雜度
| | 時間 | 額外空間 | 穩定？ |
|---|---|---|---|
| 最好／平均／最壞 | **Θ(n log n)** | O(n)，陣列版 | ✅ |
| 串列版（LeetCode 148） | Θ(n log n) | O(log n) 遞迴堆疊 | ✅ |
| `count_inversions` | Θ(n log n) | O(n) | |

## 面試陷阱／常考點
1. **最壞也是 O(n log n)**，quicksort 最壞是 O(n²)。需要保證效能、或需要**穩定**排序時，選 merge sort。
2. **陣列版需要 O(n) 的額外空間**，這是它的主要缺點。串列版不需要，因為合併只要改指標。所以**串列排序首選 merge sort**。
3. **合併時用 `<=`**，左邊先放，才是穩定的。
4. **暫存陣列只配置一次**，不要在每次遞迴裡 malloc，否則會慢很多。
5. **優化**：如果 `a[mid-1] <= a[mid]`，兩半已經接得起來，可以跳過合併。這樣已排序的輸入只要 O(n)。
6. **合併的應用**：數逆序對、LeetCode 315（右邊比自己小的個數）、493（Reverse Pairs）、外部排序（資料太大、放不進記憶體時，Thareja 14.17）。
7. **切串列時的快慢指標**：fast 從 `head->next` 出發，只有兩個節點時才切得開。從 `head` 出發會無限遞迴。
8. 逆序對最多 n(n-1)/2 個，n = 10⁵ 時大約 5 × 10⁹，超過 int，要用 `long long`。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 977 | [Squares of a Sorted Array](https://leetcode.com/problems/squares-of-a-sorted-array/) | Easy | 雙指標合併 |
| 912 | [Sort an Array](https://leetcode.com/problems/sort-an-array/) | Medium | 手寫 merge sort |
| 148 | [Sort List](https://leetcode.com/problems/sort-list/) | Medium | 串列 merge sort |
| 315 | [Count of Smaller Numbers After Self](https://leetcode.com/problems/count-of-smaller-numbers-after-self/) | Hard | 合併時計數 |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `merge_sort.h` / `.c` | 由上而下（含已排序優化）、由下而上、用合併數逆序對 |
| `test_merge_sort.c` | 2000 組隨機資料（含大量重複值）對照 `qsort` 與暴力逆序對；20 萬個反序元素（逆序對超過 int） |

執行：`make test T=11-merge-sort`

## 出處
- **CLRS** 2.3.1 The divide-and-conquer method（MERGE、MERGE-SORT）· PDF p.64；2.3.2 分析分治演算法 · PDF p.70；問題 2-4 Inversions（逆序對）· PDF p.81
- **Thareja** 14.10 Merge Sort · p.443；14.17 External Sorting · p.460
