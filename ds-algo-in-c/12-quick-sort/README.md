# 12 · Quick Sort 快速排序

## 一句話
**Quicksort（快速排序）** 挑一個 **pivot（基準值）**，把比它小的放左邊、比它大的放右邊（這一步叫 **partition 分割**），pivot 就到了最終位置；再對左右兩邊各自遞迴。

## 圖示
```
[3 7 8 5 2 1 9 5 4]   pivot = 4（最後一個）

partition 之後：
[3 2 1] 4 [7 9 5 8 5]
  ≤ 4   ↑    > 4
      已經在最終位置
然後對 [3 2 1] 和 [7 9 5 8 5] 各自遞迴
```

### Lomuto 分割（CLRS 用的版本）
```
pivot = a[hi]，i 是「≤ pivot 區」的最後一格

 lo                          hi
 [ ≤ pivot | > pivot | 還沒看 | p ]
         i         j
a[j] ≤ pivot → i++，交換 a[i] 和 a[j]
全部看完 → 把 pivot 換到 i+1
```

### Hoare 分割（原版）
兩個指標從兩端往中間走：左邊找「≥ pivot」的，右邊找「≤ pivot」的，找到就交換。交換次數大約只有 Lomuto 的 1/3，但回傳的位置**不一定是 pivot 的位置**，比較不直覺（CLRS 問題 7-1）。

### 三路切分（荷蘭國旗 Dutch National Flag）
```
[ < pivot | == pivot | 還沒看 | > pivot ]
         lt         i        gt
```
等於 pivot 的元素一次全部歸位，不用再遞迴。**大量重複值**時，兩路切分會退化成 O(n²)，三路切分反而會變得很快。

### 為什麼最壞是 O(n²)
每次 pivot 都選到最小或最大值，就會切成「0 個 + n-1 個」：
T(n) = T(n-1) + Θ(n) = **Θ(n²)**。
**已經排好序**的陣列 + 固定選最後一個當 pivot，剛好就是這種情況。

解法：**隨機選 pivot**（CLRS 7.3）。這樣期望時間是 O(n log n)，沒有任何特定輸入能讓它一直慢。

### Quickselect（快速選擇，CLRS 9.2）
只想找第 k 小的值，partition 完只要往**答案那一邊**繼續找：
T(n) = T(n/2) + Θ(n) = **Θ(n)**（平均）。

## 複雜度
| | 最好 | 平均 | 最壞 | 額外空間 | 穩定？ |
|---|---|---|---|---|---|
| Quicksort（固定 pivot） | O(n log n) | O(n log n) | O(n²)，已排序時 | O(log n)～O(n) | ❌ |
| Quicksort（隨機 pivot） | O(n log n) | O(n log n) 期望 | O(n²)，機率極低 | O(log n) | ❌ |
| 三路切分 | **O(n)**，全部相同時 | O(n log n) | O(n²) | O(log n) | ❌ |
| Quickselect | O(n) | **O(n)** | O(n²) | O(1) | |

## 面試陷阱／常考點
1. **固定 pivot 遇到已排序的輸入會變成 O(n²)**，要用隨機 pivot 或「三數取中」(median-of-three)。
2. **大量重複值要用三路切分**，否則全部相同的陣列也是 O(n²)（LeetCode 215、912 都有這種測資）。
3. **堆疊深度**：先遞迴**比較小**的那一半，大的那一半用迴圈處理（尾遞迴消除），最壞的堆疊深度就從 O(n) 變成 O(log n)。本專案三個版本都這樣做。
4. **Quicksort 不穩定。**
5. **為什麼實務上 quicksort 比 merge sort 和 heapsort 快？** 原地排序、cache 友善、內層迴圈很精簡。C 的 `qsort`、C++ 的 `std::sort`（introsort）都是以它為基礎。
6. **Lomuto 和 Hoare 不要混著寫**：Hoare 回傳的 j 不是 pivot 的位置，遞迴範圍是 `[lo, j]` 和 `[j+1, hi]`，不是 `[lo, j-1]`。
7. **Quickselect 是面試常考題**：「找第 k 大」、「找中位數」、「前 k 個最近的點」。
8. **Introsort**：遞迴太深就改用 heapsort，保證最壞 O(n log n)；很小的子陣列改用插入排序。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 905 | [Sort Array By Parity](https://leetcode.com/problems/sort-array-by-parity/) | Easy | 兩路 partition |
| 75 | [Sort Colors](https://leetcode.com/problems/sort-colors/) | Medium | 三路切分（荷蘭國旗） |
| 215 | [Kth Largest Element in an Array](https://leetcode.com/problems/kth-largest-element-in-an-array/) | Medium | Quickselect |
| 973 | [K Closest Points to Origin](https://leetcode.com/problems/k-closest-points-to-origin/) | Medium | Quickselect 前 k 個 |

215 在 13 主題寫過 heap 版，這裡是 quickselect 版。

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `quick_sort.h` / `.c` | Lomuto 與 Hoare 分割、固定 pivot 版、隨機 pivot 版、三路切分版、quickselect |
| `test_quick_sort.c` | 分割後的性質檢查、3000 組隨機資料（含只有兩種值）對照 `qsort`、20 萬個已排序／全部相同的元素、quickselect 對照排序 |

執行：`make test T=12-quick-sort`

## 出處
- **CLRS** 第 7 章 Quicksort · PDF p.255（7.1 描述與 PARTITION p.255、7.2 效能與最壞情況 p.261、7.3 隨機化 p.267、7.4 分析 p.268、問題 7-1 Hoare partition p.276）
- **CLRS** 9.2 Selection in expected linear time（RANDOMIZED-SELECT）· PDF p.315
- **Thareja** 14.11 Quick Sort · p.446
