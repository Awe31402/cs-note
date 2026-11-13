# 14 · Counting / Radix / Bucket Sort 線性時間排序

## 一句話
這三種排序**不靠兩兩比較大小**，而是利用「值本身」（當 index 用、拆成一位一位、分到桶子裡），所以能突破比較排序 Ω(n log n) 的下界，做到線性時間。代價是對資料有限制。

## 為什麼比較排序最快只有 O(n log n)（CLRS 8.1）
比較排序可以畫成一棵**決策樹 (decision tree)**：每個節點是一次比較，每片葉子是一種排列結果。
- n 個元素有 n! 種排列，所以至少要 n! 片葉子。
- 高度 h 的二元樹最多 2ʰ 片葉子 → 2ʰ ≥ n! → h ≥ log(n!) = Ω(n log n)。
- 樹高就是最壞情況的比較次數。

## 圖示

### Counting sort（計數排序）
```
in = [2 5 3 0 2 3 0 3]      值的範圍 0..5

1. 計數     cnt = [2 0 2 3 0 1]        (0 有 2 個、2 有 2 個、3 有 3 個…)
2. 前綴和   cnt = [2 2 4 7 7 8]        cnt[v] = 「≤ v」的個數
3. 從後往前放：
   in[7]=3 → cnt[3]=7 → 先減成 6 → out[6] = 3
   in[6]=0 → cnt[0]=2 → 減成 1   → out[1] = 0
   ...
out = [0 0 2 2 3 3 3 5]
```
**從後往前放**，值相同的元素後面的會放在後面，所以是**穩定**的。

### Radix sort（基數排序，LSD 從最低位開始）
```
329  457  657  839  436  720  355
依個位 → 720 355 436 457 657 329 839
依十位 → 720 329 436 839 355 457 657
依百位 → 329 355 436 457 657 720 839 ✅
```
每一趟都要用**穩定**的排序（通常是計數排序），前面幾趟排好的順序才不會被打亂。
本專案一次處理 8 個位元（基數 256），32 位元的 int 只要 4 趟。**負數**的處理方式是把最高位元（正負號）反轉，讓無號數的大小順序和原本有號數的順序一致。

### Bucket sort（桶排序）
```
值在 [0, 1)，n = 10 個桶
0.78 → 桶 7    0.17 → 桶 1    0.39 → 桶 3    0.26 → 桶 2 ...
每桶各自插入排序，再依序串起來
```
資料均勻分布時，平均每桶 O(1) 個，所以平均 O(n)。

## 複雜度
| 排序 | 時間 | 空間 | 穩定？ | 限制 |
|---|---|---|---|---|
| Counting | O(n + k) | O(n + k) | ✅ | 整數、值域 k 不能太大 |
| Radix（d 位、基數 b） | O(d · (n + b)) | O(n + b) | ✅ | 可以拆成「位數」的 key |
| Bucket | 平均 O(n)，最壞 O(n²) | O(n) | 看桶內用的排序 | 均勻分布 |

## 面試陷阱／常考點
1. **Counting sort 的 k 太大就不划算**：值域 0..10⁹ 就要開 10⁹ 格的陣列。
2. **Counting sort 要從後往前放才穩定**，Radix sort 的正確性就靠這個。
3. **Radix sort 的每一趟一定要穩定**，否則前面的趟數就白做了。
4. **不需要真的排序也能用計數的想法**：「比我小的有幾個」（1365）、H-Index（274）、Top K Frequent 的 bucket 解法（347，13 主題）。
5. **鴿籠原理 + 桶子**可以做到 O(n) 找最大間距（164），不用真的排序。
6. **Bucket sort 最壞 O(n²)**：全部資料都掉進同一個桶時。
7. **這些排序不是比較排序**，所以不受 Ω(n log n) 限制。面試官問「能不能比 n log n 更快」時，要想到這一類。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 1365 | [How Many Numbers Are Smaller Than the Current Number](https://leetcode.com/problems/how-many-numbers-are-smaller-than-the-current-number/) | Easy | 計數 + 前綴和 |
| 1051 | [Height Checker](https://leetcode.com/problems/height-checker/) | Easy | 計數排序 |
| 274 | [H-Index](https://leetcode.com/problems/h-index/) | Medium | 截斷在 n 的計數 |
| 164 | [Maximum Gap](https://leetcode.com/problems/maximum-gap/) | Medium | 桶子 + 鴿籠原理 |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `linear_sort.h` / `.c` | 計數排序（整數、紀錄）、LSD 基數排序（基數 256、支援負數）、桶排序（[0,1) 的 double） |
| `test_linear_sort.c` | 2000 組隨機資料對照 `qsort`、`INT_MIN`／`INT_MAX`、計數排序的穩定性、桶排序 |

執行：`make test T=14-linear-sort`

## 出處
- **CLRS** 第 8 章 Sorting in Linear Time · PDF p.284（8.1 比較排序的下界 p.284、8.2 Counting sort p.288、8.3 Radix sort p.292、8.4 Bucket sort p.297）
- **Thareja** 14.12 Radix Sort · p.450；附錄 F Address Calculation Sort · p.520
