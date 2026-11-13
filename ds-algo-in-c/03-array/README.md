# 03 · 陣列 Array

## 一句話
**陣列 (array)** 是一段**連續的記憶體**，裝同一種型別的元素。因為連續，用 index 算位址就能 O(1) 直接拿到任何一格。

## 圖示
```
int a[5];            起始位址 base = 1000，sizeof(int) = 4

index:    0     1     2     3     4
        ┌─────┬─────┬─────┬─────┬─────┐
        │  10 │  20 │  30 │  40 │  50 │
        └─────┴─────┴─────┴─────┴─────┘
位址:   1000  1004  1008  1012  1016

a[i] 的位址 = base + i × sizeof(int)       ← 所以是 O(1)
```

### 插入和刪除要搬家
```
在 index 1 插入 15：從最後一個開始往右搬，再放進去
  [10 20 30 40 __]  →  [10 __ 20 30 40]  →  [10 15 20 30 40]
刪除 index 1：後面的全部往左搬一格
```
要從**後面**往前搬，否則會蓋掉還沒搬的資料。C 的 `memmove` 會自動處理重疊的情況，`memcpy` 不會。

### 二維陣列（row-major）
```
int m[2][3] = {{1,2,3},{4,5,6}};   在記憶體中是一整排：
  1 2 3 4 5 6
m[r][c] 在第 r * cols + c 格
```
C 是 **row-major**（一列一列存），Fortran 是 column-major。一列一列走訪對 cache 比較友善，所以比較快。

### 稀疏矩陣 (Sparse Matrix)
大部分是 0 的矩陣，只存非零元素的 `(row, col, value)`，稱為 triplet 表示法。
```
0 0 3 0
0 0 0 0      →   (0,2,3)  (2,0,7)  (2,3,-1)
7 0 0 -1
```

## 複雜度
| 操作 | 靜態陣列 | 動態陣列 `Vec` |
|---|---|---|
| 用 index 讀寫 | O(1) | O(1) |
| 尾端加入 | — | 攤銷 O(1) |
| 尾端移除 | — | O(1) |
| 中間插入／刪除 | O(n) | O(n) |
| 搜尋（未排序） | O(n) | O(n) |
| 搜尋（已排序） | O(log n)，binary search | O(log n) |
| 合併兩個已排序陣列 | O(n + m) | — |

## 面試陷阱／常考點
1. **陣列當參數傳進函式時會退化成指標 (decay to pointer)。** 在函式裡 `sizeof(a)` 拿到的是指標的大小（8），不是整個陣列的大小。所以一定要另外傳長度。
2. **越界存取不會報錯，是未定義行為。** C 不幫你檢查。本專案用 AddressSanitizer 抓這類錯誤。
3. **原地 (in-place) 操作常用雙指標**：快慢指標（LeetCode 26）、左右夾擠（Two Sum 排序版）、從尾端往前寫（LeetCode 88）。
4. **反轉三次可以旋轉陣列**（LeetCode 189），O(1) 額外空間。
5. **前綴和、前綴積**可以把區間計算從 O(n) 降到 O(1)（LeetCode 238、1480）。
6. **動態陣列的擴容要乘倍數，不能每次加固定格數**，原因見 01 主題的攤銷分析。
7. **`realloc` 可能回傳新的位址**，舊指標要更新；如果回傳 NULL，原本的記憶體還在，要自己處理。
8. **二維陣列的傳參**：`int a[][3]` 必須寫出第二維的大小。LeetCode 用 `int**` 加上每列長度，跟真正的二維陣列在記憶體裡不一樣。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 88 | [Merge Sorted Array](https://leetcode.com/problems/merge-sorted-array/) | Easy | 從尾端往前寫 |
| 26 | [Remove Duplicates from Sorted Array](https://leetcode.com/problems/remove-duplicates-from-sorted-array/) | Easy | 快慢指標 |
| 189 | [Rotate Array](https://leetcode.com/problems/rotate-array/) | Medium | 三次反轉 |
| 238 | [Product of Array Except Self](https://leetcode.com/problems/product-of-array-except-self/) | Medium | 前綴積 + 後綴積 |
| 48 | [Rotate Image](https://leetcode.com/problems/rotate-image/) | Medium | 二維：轉置 + 翻轉 |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `array.h` / `.c` | 動態陣列 `Vec`（push/pop/insert/erase/find）、合併已排序陣列、row-major 索引、轉置、矩陣乘法、稀疏矩陣 triplet 轉換 |
| `test_array.c` | 插入刪除頭中尾、擴容一萬次、合併對照 `qsort`、矩陣運算、稀疏矩陣來回轉換 |

執行：`make test T=03-array`

## 出處
- **CLRS** 10.1 Simple array-based data structures（陣列與矩陣的記憶體配置）· PDF p.344
- **Thareja** 第 3 章 Arrays · p.66–114（3.3.1 位址計算 p.68、3.5 插入刪除合併 p.71–86、3.9–3.12 二維陣列 p.93–106、3.15 稀疏矩陣 p.110）
