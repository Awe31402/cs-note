# 10 · Insertion Sort 插入排序（和其他 O(n²) 排序）

## 一句話
**Insertion sort（插入排序）** 就像整理手上的撲克牌：左邊是已經排好的牌，每次拿右邊的下一張，往左找到它的位置插進去。

## 圖示
```
[5 2 4 6 1 3]      左邊 | 右邊
 5 | 2 4 6 1 3     拿 2：2 < 5，5 右移 → 2 5
 2 5 | 4 6 1 3     拿 4：4 < 5，5 右移；4 > 2 停 → 2 4 5
 2 4 5 | 6 1 3     拿 6：6 > 5，不動
 2 4 5 6 | 1 3     拿 1：全部右移 → 1 2 4 5 6
 1 2 4 5 6 | 3     拿 3 → 1 2 3 4 5 6 ✅
```
（這是 CLRS Figure 2.2 的例子）

### 迴圈不變式 (loop invariant, CLRS 2.1)
每一輪開始時，`a[0..i-1]` 是原本前 i 個元素排好序的樣子。證明正確性要檢查三件事：
- **初始**：i = 1 時，只有一個元素，本來就排好了。
- **維持**：把 a[i] 插進正確位置後，`a[0..i]` 也排好了。
- **終止**：i = n 時，整個陣列都排好了。

### 其他 O(n²) 排序（Thareja 14.7、14.9、14.14）
| 排序 | 做法 | 特色 |
|---|---|---|
| **Bubble sort（泡沫）** | 相鄰的兩個比較，大的往右換；每一趟把最大的「浮」到最右邊 | 加上「一趟都沒交換就停止」，已排序時是 O(n) |
| **Selection sort（選擇）** | 每次從右邊選最小的，跟目前位置交換 | 交換次數最少（≤ n-1 次），但**不穩定** |
| **Shell sort（希爾）** | 先做間隔很大的插入排序，再逐步縮小間隔到 1 | 大約 O(n^1.3)，比 O(n²) 快很多 |

## 複雜度
| 排序 | 最好 | 平均 | 最壞 | 額外空間 | 穩定？ |
|---|---|---|---|---|---|
| Insertion | **O(n)** | O(n²) | O(n²) | O(1) | ✅ |
| Binary insertion | O(n log n) 次比較 | O(n²) | O(n²) | O(1) | ✅ |
| Bubble（提早停止） | O(n) | O(n²) | O(n²) | O(1) | ✅ |
| Selection | O(n²) | O(n²) | O(n²) | O(1) | ❌ |
| Shell（Knuth 間隔） | O(n log n) | ≈ O(n^1.3) | O(n^1.5) | O(1) | ❌ |

**穩定 (stable)**：值相同的元素，排序後的相對順序不變。依多個欄位排序時很重要，例如「先依姓名排，再依部門排」。

## 面試陷阱／常考點
1. **插入排序的搬移次數 = 逆序對 (inversion) 的個數。** 所以「幾乎排好」的資料（逆序對很少）用插入排序接近 O(n)。本專案的測試有驗證這件事。
2. **小陣列時插入排序最快。** 常數很小，而且 cache 友善。實務上 quicksort、merge sort 在子陣列小於 10～20 個時，會改用插入排序（例如 C++ 的 introsort）。
3. **用二分搜尋找插入點，不會讓插入排序變成 O(n log n)**：比較次數少了，但搬移還是 O(n²)。這是 CLRS 習題 2.3-7 的考點。
4. **Selection sort 不穩定**：`[2a, 2b, 1]` → 1 和 2a 交換 → `[1, 2b, 2a]`，兩個 2 的順序反了。
5. **穩定性取決於比較是 `>` 還是 `>=`。** 插入排序寫成 `a[j] >= key` 就會變成不穩定。
6. **Bubble sort 面試幾乎只會被拿來比較**，實務上沒人用。
7. **C 的 `qsort` 比較函式不要寫 `return a - b;`**，可能溢位，例如 `INT_MIN - 1`。要寫 `(a > b) - (a < b)`。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 283 | [Move Zeroes](https://leetcode.com/problems/move-zeroes/) | Easy | 穩定的原地搬移 |
| 976 | [Largest Perimeter Triangle](https://leetcode.com/problems/largest-perimeter-triangle/) | Easy | 排序後貪心 |
| 147 | [Insertion Sort List](https://leetcode.com/problems/insertion-sort-list/) | Medium | 串列上的插入排序 |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `simple_sort.h` / `.c` | 插入排序（回傳搬移次數）、二分插入排序、泡沫排序（提早停止）、選擇排序、希爾排序 |
| `test_simple_sort.c` | 五種排序 × 2000 組隨機資料（含大量重複值）對照 `qsort`；搬移次數 = 逆序對數；最好與最壞情況 |

執行：`make test T=10-insertion-sort`

## 出處
- **CLRS** 2.1 Insertion sort（迴圈不變式）· PDF p.44；2.2 分析插入排序的最好／最壞情況 · PDF p.53；習題 2.3-7（二分插入）· PDF p.78
- **Thareja** 14.6 排序簡介 p.433、14.7 Bubble Sort p.434、14.8 Insertion Sort p.438、14.9 Selection Sort p.440、14.14 Shell Sort p.456、14.16 排序比較 p.460
