# 09 · Binary Search 二分搜尋

## 一句話
**Binary search（二分搜尋）** 在**已排序**的陣列裡找東西：每次看中間那個，跟目標比大小，就能丟掉一半。n 個元素最多比較約 log₂n 次。

## 圖示
```
找 13，陣列 [1 3 5 8 13 21 34]

lo                mid                 hi(開)
 1   3   5   8   13   21   34
             ↑ 8 < 13 → 丟掉左半（含 mid）
                 lo    mid       hi
                 13   21   34
                       ↑ 21 > 13 → 丟掉右半（含 mid）
                 lo mid hi
                 13
                  ↑ 找到 ✅
```

### 最重要：想清楚區間的定義
本專案一律用**左閉右開 `[lo, hi)`**：
| | 左閉右開 `[lo, hi)` | 閉區間 `[lo, hi]` |
|---|---|---|
| 初始值 | `lo = 0, hi = n` | `lo = 0, hi = n - 1` |
| 迴圈條件 | `while (lo < hi)` | `while (lo <= hi)` |
| 往右 | `lo = mid + 1` | `lo = mid + 1` |
| 往左 | `hi = mid` | `hi = mid - 1` |

混著用就會出現無限迴圈或漏掉元素，這是二分搜尋最常見的 bug。

### lower_bound 模板：「第一個讓條件成立的位置」
很多問題都能寫成「條件對 index 來說是 F F F T T T」，要找第一個 T：
```c
while (lo < hi) {
    int mid = lo + (hi - lo) / 2;
    if (條件(mid)) hi = mid;      // mid 可能就是答案，保留
    else           lo = mid + 1;
}
return lo;
```
- `lower_bound(x)`：條件是 `a[i] >= x`
- `upper_bound(x)`：條件是 `a[i] > x`
- x 出現的次數 = `upper_bound(x) − lower_bound(x)`
- **在答案上二分**：條件換成「答案 = mid 時可行嗎」（`isqrt_bs`、LeetCode 875）

### 其他搜尋法（Thareja 14.4–14.5）
- **內插搜尋 (interpolation search)**：像查字典，找 "W" 開頭的字會直接翻到後面。依照 x 在 `[a[lo], a[hi]]` 中的比例猜位置。
- **跳躍搜尋 (jump search)**：每次跳 √n 格，跳過頭了再回到上一塊線性找。

## 複雜度
| 方法 | 平均 | 最壞 | 需要排序？ |
|---|---|---|---|
| 線性搜尋 | O(n) | O(n) | 不用 |
| **二分搜尋** | O(log n) | **O(log n)** | 要 |
| 內插搜尋 | O(log log n)，值均勻分布時 | O(n) | 要 |
| 跳躍搜尋 | O(√n) | O(√n) | 要 |

二分搜尋的空間：迴圈版 O(1)，遞迴版 O(log n)（堆疊）。

## 面試陷阱／常考點
1. **`mid = (lo + hi) / 2` 會溢位**，要寫 `lo + (hi - lo) / 2`。這是 Java 標準函式庫曾經真的出過的 bug。
2. **區間定義要一致**（見上表），否則會無限迴圈或漏元素。
3. **有重複值時**，`bs_find` 回傳的是「其中一個」，不保證是第一個。要第一個就用 `lower_bound`。
4. **找不到時，`lower_bound` 回傳的就是插入位置**（LeetCode 35）。
5. **在答案上二分**：題目問「最小的 k 使得…可行」，而可行性對 k 是單調的，就能二分。常見題：吃香蕉（875）、運貨（1011）、分割陣列（410）。
6. **旋轉排序陣列**：切一半後，至少一半是有序的（LeetCode 33）。
7. **計算中間值時小心溢位**：例如 `mid * mid`，要用 `long long`。
8. **二分搜尋需要隨機存取**，所以 linked list 不能二分（要 O(n) 才走得到中間）。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 704 | [Binary Search](https://leetcode.com/problems/binary-search/) | Easy | 標準模板 |
| 35 | [Search Insert Position](https://leetcode.com/problems/search-insert-position/) | Easy | lower_bound |
| 34 | [Find First and Last Position of Element in Sorted Array](https://leetcode.com/problems/find-first-and-last-position-of-element-in-sorted-array/) | Medium | lower + upper bound |
| 33 | [Search in Rotated Sorted Array](https://leetcode.com/problems/search-in-rotated-sorted-array/) | Medium | 判斷哪半有序 |
| 875 | [Koko Eating Bananas](https://leetcode.com/problems/koko-eating-bananas/) | Medium | 在答案上二分 |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `binary_search.h` / `.c` | 線性搜尋、二分（迴圈、遞迴）、lower/upper bound、整數平方根（在答案上二分）、內插搜尋、跳躍搜尋 |
| `test_binary_search.c` | 3000 組隨機排序陣列 × 每個目標值，所有方法互相對照；平方根 0..10⁵ 與 `INT_MAX`；內插搜尋的溢位邊界 |

執行：`make test T=09-binary-search`

## 出處
- **CLRS** 習題 2.3-6（寫出二分搜尋並證明最壞 Θ(lg n)）· PDF p.78
- **Thareja** 14.2 Linear Search p.424、14.3 Binary Search p.426、14.4 Interpolation Search p.428、14.5 Jump Search p.430
