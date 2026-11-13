# 01 · 複雜度 Big-O / Ω / Θ

## 一句話
**複雜度 (complexity)** 用來描述「輸入變大時，執行時間或記憶體**長得多快**」。我們只看成長的趨勢，常數和低次項都丟掉。

## 三個符號
| 符號 | 意思 | 白話 | 類比 |
|---|---|---|---|
| **O(g)** | 上界 (upper bound) | 最多這麼慢 | f ≤ g |
| **Ω(g)** | 下界 (lower bound) | 至少這麼慢 | f ≥ g |
| **Θ(g)** | 緊界 (tight bound) | 剛好這個等級 | f = g |

正式定義（CLRS 3.2）：
- f(n) = O(g(n))：存在 c > 0、n₀，使所有 n ≥ n₀ 都有 0 ≤ f(n) ≤ c·g(n)
- f(n) = Ω(g(n))：存在 c > 0、n₀，使所有 n ≥ n₀ 都有 0 ≤ c·g(n) ≤ f(n)
- f(n) = Θ(g(n))：同時是 O(g) 和 Ω(g)

另外還有兩個「嚴格」版本：**o**（小 o，f < g）和 **ω**（小 omega，f > g）。面試很少考。

## 圖示：常見等級，由快到慢
```
n = 1,000 時大約要跑幾次

O(1)        │ 1
O(log n)    │ 10
O(√n)       │ 32
O(n)        │ 1,000
O(n log n)  │ 10,000
O(n²)       │ 1,000,000
O(2ⁿ)       │ 10³⁰⁰        ← 宇宙毀滅都跑不完
```

面試用的經驗法則（C 語言每秒大約跑 10⁸ 次簡單運算）：

| n 的上限 | 可接受的複雜度 |
|---|---|
| ≤ 10 | O(n!) |
| ≤ 20 | O(2ⁿ) |
| ≤ 500 | O(n³) |
| ≤ 5,000 | O(n²) |
| ≤ 10⁶ | O(n log n) |
| ≤ 10⁸ | O(n) |
| 更大 | O(log n) 或 O(1) |

## 看程式碼算複雜度
| 迴圈型態 | 次數 | 複雜度 |
|---|---|---|
| `for (i=0; i<n; i++)` | n | O(n) |
| `for (i=1; i<=n; i*=2)` | ⌊log₂n⌋+1 | O(log n) |
| `for (i=1; i*i<=n; i++)` | ⌊√n⌋ | O(√n) |
| 兩層都跑 n 次 | n² | O(n²) |
| `for i<n` 裡面 `for j<=i` | n(n+1)/2 | O(n²)，常數除以 2 還是 n² |
| 外層 n 次，內層乘 2 | n·log n | O(n log n) |

規則：**循序執行取最大的，巢狀迴圈用乘的**。`complexity.c` 會實際數這幾種迴圈跑了幾次，測試用公式對照。

## 最好、最壞、平均情況
跟 O/Ω/Θ 是**兩件不同的事**：
- **情況 (case)**：在說「哪一種輸入」。
- **符號 (O/Ω/Θ)**：在說「這個函數的上界或下界」。

例子：insertion sort（CLRS 2.2、3.1）
- 最好情況（已經排好）：Θ(n)
- 最壞情況（反向排序）：Θ(n²)
- 所以「insertion sort 是 O(n²)」✅ 對；「insertion sort 是 Θ(n²)」❌ 不精確，因為最好情況只要 Θ(n)。

## 攤銷分析 (Amortized Analysis)
有些操作偶爾很貴，但**平均分攤**下來很便宜。

動態陣列 push：陣列滿了就開一個 **2 倍大**的新陣列，把舊元素搬過去。
```
容量:  1 → 2 → 4 → 8 → 16 ...
搬移:    1 + 2 + 4 + 8 + ...  < 2n
```
push n 次，總搬移不到 2n 次，所以**每次 push 平均 O(1)**。
如果每次只加大 1 格，總搬移會是 n(n-1)/2，每次平均 O(n)。`dynarray_copies_*` 會實測這兩種做法。

## 複雜度表
這個主題沒有資料結構，改列本專案 `complexity.c` 裡的函式：

| 函式 | 迴圈次數 | 複雜度 |
|---|---|---|
| `count_constant` | 100 | O(1) |
| `count_log` | ⌊log₂n⌋+1 | O(log n) |
| `count_sqrt` | ⌊√n⌋ | O(√n) |
| `count_linear` | n | O(n) |
| `count_n_log_n` | n(⌊log₂n⌋+1) | O(n log n) |
| `count_triangular` | n(n+1)/2 | O(n²) |
| `count_quadratic` | n² | O(n²) |
| `dynarray_copies_double` | < 2n | 每次 push 攤銷 O(1) |
| `dynarray_copies_plus1` | n(n-1)/2 | 每次 push 攤銷 O(n) |

## 面試陷阱／常考點
1. **O 不等於「最壞情況」。** O 是上界，最壞情況是一種輸入。兩者可以任意搭配，例如「最好情況是 O(n)」也是合法的說法。
   Thareja 2.9 把這兩個概念寫在一起（「Worst case O describes a lower bound…」），容易誤會。以 CLRS 3.1 的說法為準。
2. **log 的底數不重要。** log₂n 和 log₁₀n 只差常數倍，都寫 O(log n)。但 2ⁿ 和 3ⁿ 是**不同**等級。
3. **O(n + m) 不能簡化成 O(n)。** 兩個輸入大小互相獨立時，兩個都要寫（例如圖的 V 和 E）。
4. **空間複雜度要算遞迴的堆疊 (stack)。** 遞迴深度 n，就是 O(n) 的額外空間。
5. **雜湊表 (hash table) 的 O(1) 是平均情況**，最壞是 O(n)。
6. **n² 和 n log n 在 n = 10⁶ 時**差了約 5 萬倍，所以面試官說「n 到 10⁵」時，O(n²) 就會超時。
7. **攤銷 O(1) ≠ 每次都是 O(1)。** 動態陣列某一次 push 可能是 O(n)，只是平均起來是 O(1)。

## 練習題（LeetCode）
這幾題的重點是**從暴力解優化到更好的複雜度**。

| # | 題目 | 難度 | 暴力 → 優化 |
|---|---|---|---|
| 1480 | [Running Sum of 1d Array](https://leetcode.com/problems/running-sum-of-1d-array/) | Easy | O(n²) → O(n) |
| 1 | [Two Sum](https://leetcode.com/problems/two-sum/) | Easy | O(n²) → O(n log n) → O(n) |
| 53 | [Maximum Subarray](https://leetcode.com/problems/maximum-subarray/) | Medium | O(n³) → O(n²) → O(n) |
| 204 | [Count Primes](https://leetcode.com/problems/count-primes/) | Medium | O(n√n) → O(n log log n) |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `complexity.h` / `.c` | 各種迴圈的計數函式、動態陣列攤銷模擬 |
| `test_complexity.c` | 用公式對照迴圈次數、n 翻倍時的成長比例、攤銷上界 |

執行：`make test T=01-complexity`

## 出處
- **CLRS** 2.2 Analyzing algorithms · PDF p.53
- **CLRS** 第 3 章 Characterizing Running Times · PDF p.85（3.1 p.86、3.2 p.91、3.3 p.103）
- **CLRS** 16.4 Dynamic tables · PDF p.608（攤銷分析；第 16 章不在本專案範圍，這裡只引用這一節）
- **Thareja** 2.8 Time and Space Complexity ～ 2.12 Other Useful Notations · p.54–63
