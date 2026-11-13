# 02 · 遞迴 Recursion

## 一句話
**遞迴 (recursion)** 就是函式呼叫自己，去解一個**更小的同類問題**，直到問題小到可以直接回答。

每個遞迴都要有兩部分：
- **Base case（終止條件）**：夠小了，直接回答。
- **Recursive case（遞迴步驟）**：切小 → 呼叫自己 → 把結果組合起來。

寫遞迴的心法是**相信它 (leap of faith)**：假設 `f(n-1)` 已經會正確回答，你只要想「怎麼用它算出 `f(n)`」。

## 圖示

### 呼叫堆疊 (call stack)
每次呼叫都會在系統堆疊上疊一層，存參數、區域變數和返回位址（Thareja 7.7.4）：
```
factorial(4)                          堆疊（往下長）
= 4 * factorial(3)                    ┌──────────────┐
      = 3 * factorial(2)              │ factorial(4) │ 等著乘 4
            = 2 * factorial(1)        │ factorial(3) │ 等著乘 3
                  = 1  ← base case    │ factorial(2) │ 等著乘 2
            = 2                       │ factorial(1) │ → 回傳 1
      = 6                             └──────────────┘
= 24                                  深度 n → 空間 O(n)
```

### 遞迴樹：Fibonacci 為什麼慢
```
                fib(5)
             /          \
         fib(4)          fib(3)       ← fib(3) 算了 2 次
        /     \          /    \
    fib(3)  fib(2)   fib(2)  fib(1)   ← fib(2) 算了 3 次
    /   \
 fib(2) fib(1)  ...
```
同樣的子問題一直重算，呼叫次數 = 2·fib(n+1) − 1，大約是 1.618ⁿ。
**記起來 (memoization)** 就只要 O(n)。這就是動態規劃（第 28 主題）的起點。

### 河內塔 (Tower of Hanoi)
```
把 n 個從 A 搬到 C（B 暫放）：
  1. 上面 n-1 個：A → B（C 暫放）   ← 遞迴
  2. 最大的那個：A → C              ← 一步
  3. 那 n-1 個：B → C（A 暫放）     ← 遞迴
步數 T(n) = 2T(n-1) + 1 = 2ⁿ - 1
```

## 遞迴的分類（Thareja 7.7.4）
| 分類 | 說明 | 例子 |
|---|---|---|
| 直接 / 間接 (direct / indirect) | 自己呼叫自己 / A 呼叫 B，B 再呼叫 A | `factorial` / `is_even` ↔ `is_odd` |
| 尾遞迴 / 非尾遞迴 (tail / non-tail) | 遞迴回來後**沒有**要做的事 / 還有事要做 | `factorial_tail` / `factorial` |
| 線性 / 樹狀 (linear / tree) | 每層呼叫 1 次 / 呼叫多次 | `factorial` / `fib_naive`、`hanoi` |

**尾遞迴**可以被編譯器改寫成迴圈，堆疊只要 O(1)。但 **C 標準不保證這件事**：gcc 開 `-O2` 通常會做，`-O0` 不會。

## 分析遞迴：寫出遞迴式
遞迴的時間可以寫成**遞迴式 (recurrence)**，再解出來（CLRS 4.3–4.5）。

### 主定理 (Master Theorem, CLRS Theorem 4.1)
適用 **T(n) = a·T(n/b) + f(n)**，其中 a > 0、b > 1。
先算「分水嶺函數」**n^(log_b a)**，再拿 f(n) 跟它比：

| 情況 | 條件 | 答案 | 直覺 |
|---|---|---|---|
| 1 | f(n) 比 n^(log_b a) 小一個多項式因子 | Θ(n^(log_b a)) | 葉子最多，葉子決定 |
| 2 | f(n) = Θ(n^(log_b a) · logᵏn)，k ≥ 0 | Θ(n^(log_b a) · logᵏ⁺¹n) | 每層差不多，乘上層數 |
| 3 | f(n) 比 n^(log_b a) 大一個多項式因子，且符合 regularity 條件 | Θ(f(n)) | 根最貴，根決定 |

### 常見遞迴式，直接背
| 遞迴式 | 答案 | 例子 |
|---|---|---|
| T(n) = T(n/2) + O(1) | O(log n) | Binary search、`power_fast` |
| T(n) = T(n/2) + O(n) | O(n) | Quickselect 平均 |
| T(n) = 2T(n/2) + O(1) | O(n) | 二元樹走訪 |
| T(n) = 2T(n/2) + O(n) | O(n log n) | Merge sort |
| T(n) = T(n-1) + O(1) | O(n) | `factorial`、`power_naive` |
| T(n) = T(n-1) + O(n) | O(n²) | Selection sort 的遞迴版 |
| T(n) = 2T(n-1) + O(1) | O(2ⁿ) | 河內塔 |
| T(n) = T(n-1) + T(n-2) + O(1) | O(φⁿ) ≈ O(1.618ⁿ) | `fib_naive` |

後四個是 T(n-1) 的形式，**不能用主定理**，要畫遞迴樹或直接展開。

## 複雜度表
| 函式 | 時間 | 額外空間（堆疊） |
|---|---|---|
| `factorial` | O(n) | O(n) |
| `factorial_tail` | O(n) | O(n)；開最佳化後可能變 O(1) |
| `gcd` | O(log min(a,b)) | O(log min(a,b)) |
| `power_naive` | O(n) | O(n) |
| `power_fast` | O(log n) | O(log n) |
| `fib_naive` | O(φⁿ) | O(n) |
| `fib_memo` | O(n) | O(n) |
| `fib_iter` | O(n) | O(1) |
| `hanoi` | O(2ⁿ) | O(n) |

## 面試陷阱／常考點
1. **忘了 base case，或 base case 永遠到不了** → 無限遞迴 → stack overflow。例如 `factorial(-1)` 在只判斷 `n == 1` 的版本裡永遠停不下來。本專案寫成 `n <= 1`。
2. **遞迴太深會爆堆疊。** Linux 預設堆疊 8 MB，遞迴深度大約到 10⁵～10⁶ 層就會當掉。深度可能到 n = 10⁶ 時，就要改成迴圈。
3. **重複子問題** → 用 memo 記起來，或改成由下往上的迴圈 (bottom-up)。
4. **`power_fast` 要把 `half` 存起來。** 如果寫成 `power(x, n/2) * power(x, n/2)`，遞迴式變成 T(n) = 2T(n/2) + 1 = O(n)，快速冪就白做了。
5. **整數溢位：** `20!` 和 `fib(92)` 是 `long long` 的極限。UBSan 在這個專案抓到過 `fib_iter` 多算一項造成的溢位。
6. **遞迴改迴圈**：任何遞迴都能用自己維護的 stack 改寫成迴圈（第 06 主題）。面試有時會要求「不要用遞迴」。
7. **空間複雜度要算堆疊深度。** 很多人說「遞迴版的空間是 O(1)」，這是錯的。
8. **gcd 的 a < b 不用特別處理。** `gcd(8, 12)` 的第一步是 `gcd(12, 8 % 12)` = `gcd(12, 8)`，兩個數自動交換了。Thareja 說要先交換，其實不需要。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 509 | [Fibonacci Number](https://leetcode.com/problems/fibonacci-number/) | Easy | 重複子問題、memo |
| 206 | [Reverse Linked List](https://leetcode.com/problems/reverse-linked-list/) | Easy | 相信遞迴、回來再接 |
| 21 | [Merge Two Sorted Lists](https://leetcode.com/problems/merge-two-sorted-lists/) | Easy | 選一個當頭，其餘交給遞迴 |
| 50 | [Pow(x, n)](https://leetcode.com/problems/powx-n/) | Medium | 快速冪、INT_MIN |
| 22 | [Generate Parentheses](https://leetcode.com/problems/generate-parentheses/) | Medium | 回溯 (backtracking) |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `recursion.h` / `.c` | 階乘（一般、尾遞迴）、gcd、次方（O(n)、O(log n)）、Fibonacci 三種寫法、河內塔 |
| `test_recursion.c` | 各函式對照答案、`fib_naive` 呼叫次數公式、河內塔在三根柱子上實際模擬每一步 |

執行：`make test T=02-recursion`

## 出處
- **CLRS** 2.3 Designing algorithms（分治法與遞迴、2.3.2 分析遞迴演算法）· PDF p.64、p.70
- **CLRS** 4.3 Substitution method · PDF p.138
- **CLRS** 4.4 Recursion-tree method · PDF p.144
- **CLRS** 4.5 Master method（Theorem 4.1）· PDF p.153–156
- **Thareja** 7.7.4 Recursion（階乘、GCD、次方、Fibonacci、遞迴分類、河內塔、優缺點）· p.243–251
