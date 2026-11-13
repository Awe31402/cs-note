# 27 · Divide & Conquer 分治法

## 一句話
**分治法 (divide and conquer)** 三個步驟：
1. **切 (divide)**：把問題切成幾個一樣形式、但比較小的子問題。
2. **治 (conquer)**：遞迴解子問題；夠小了就直接解（base case）。
3. **合 (combine)**：把子問題的答案合併成原問題的答案。

Merge sort（第 11 主題）、Quick sort（第 12 主題）、Binary search（第 9 主題）都是分治法。這個主題補上 CLRS 第 4 章的其他經典例子。

## 分析：寫出遞迴式，用主定理
T(n) = a · T(n/b) + f(n)（a 個子問題、每個大小 n/b、切和合花 f(n)）。主定理見第 02 主題。
| 演算法 | 遞迴式 | 結果 |
|---|---|---|
| Binary search | T(n/2) + O(1) | O(log n) |
| Merge sort | 2T(n/2) + O(n) | O(n log n) |
| 最大子陣列（分治） | 2T(n/2) + O(n) | O(n log n) |
| 矩陣乘法（遞迴） | 8T(n/2) + O(n²) | O(n³)，**沒有比較快** |
| **Strassen** | **7**T(n/2) + O(n²) | **O(n^lg 7) ≈ O(n^2.81)** |

## 矩陣乘法：從 8 次到 7 次（CLRS 4.1、4.2）
把 n×n 矩陣切成四塊 n/2 × n/2：
```
[ C11 C12 ]   [ A11 A12 ] [ B11 B12 ]
[ C21 C22 ] = [ A21 A22 ] [ B21 B22 ]

C11 = A11·B11 + A12·B21      ← 每一塊需要 2 次子矩陣乘法
C12 = A11·B12 + A12·B22
C21 = A21·B11 + A22·B21
C22 = A21·B12 + A22·B22      → 一共 8 次 → T(n) = 8T(n/2) + Θ(n²) = Θ(n³)
```
**Strassen 的技巧**：先算 10 個加減法的組合 S1..S10，再用它們做 **7 次**乘法 P1..P7，最後只用加減法組回 C。
```
P1 = A11·(B12 − B22)          C11 = P5 + P4 − P2 + P6
P2 = (A11 + A12)·B22          C12 = P1 + P2
P3 = (A21 + A22)·B11          C21 = P3 + P4
P4 = A22·(B21 − B11)          C22 = P5 + P1 − P3 − P7
P5 = (A11 + A22)·(B11 + B22)
P6 = (A12 − A22)·(B21 + B22)
P7 = (A11 − A21)·(B11 + B12)
```
少一次乘法看起來不多，但**每一層都少一次**，遞迴下去指數就從 3 降到 log₂7 ≈ 2.81。
實務上，矩陣小的時候 Strassen 反而比較慢（常數大、要配置暫存），所以本專案在 n ≤ 32 時改用三層迴圈。

## 最大子陣列（CLRS 第 3 版 4.1）
```
[ 左半 | 右半 ]
答案只有三種可能：
  1. 完全在左半 → 遞迴
  2. 完全在右半 → 遞迴
  3. 跨過中點   → 從中點往左的最大延伸 + 從中點往右的最大延伸，O(n) 算得出來
```
CLRS 第 4 版已經刪掉這一節（見 01 主題的說明），但它仍然是「合併步驟不簡單」的經典例子。實務上用 Kadane 的 O(n) 就好。

## 複雜度
| 函式 | 時間 | 額外空間 |
|---|---|---|
| `matmul_naive` | Θ(n³) | O(1) |
| `matmul_recursive` | Θ(n³) | O(log n) 遞迴 |
| `strassen` | Θ(n^2.81) | O(n²) 暫存 |
| `max_subarray_dc` | Θ(n log n) | O(log n) |
| `majority_dc` | Θ(n log n) | O(log n) |

## 面試陷阱／常考點
1. **分治不一定比較快**：遞迴版矩陣乘法還是 Θ(n³)。要看子問題的數量 a 和合併的成本 f(n)。
2. **合併步驟是關鍵**：最大子陣列的「跨中點」、merge sort 的合併、LeetCode 315 的計數，都是在合併時做事。
3. **子問題重疊的話，分治會重複計算**，這時要改用動態規劃（第 28 主題）。例如 Fibonacci、LeetCode 241（可以加 memo）。
4. **Strassen 的數值穩定性比較差**（浮點數誤差），而且常數大，所以 BLAS 等數值函式庫很少用它。
5. **Boyer-Moore 投票法**（LeetCode 169）比分治版好：O(n)、O(1) 空間。
6. **多數元素的分治正確性**：整段的多數元素，一定是左半或右半的多數元素。
7. 分治的其他經典例子：最近點對 (closest pair, O(n log n))、Karatsuba 大數乘法、快速傅立葉轉換 (FFT)。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 169 | [Majority Element](https://leetcode.com/problems/majority-element/) | Easy | 分治 vs 投票法 |
| 53 | [Maximum Subarray](https://leetcode.com/problems/maximum-subarray/) | Medium | 分治：跨中點的情況 |
| 241 | [Different Ways to Add Parentheses](https://leetcode.com/problems/different-ways-to-add-parentheses/) | Medium | 以「最後一個運算子」切開 |

53 在 01 主題寫過 Kadane 的 O(n) 版，這裡是分治版。

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `dnc.h` / `.c` | 三層迴圈矩陣乘法、遞迴矩陣乘法（子矩陣視窗，8 次遞迴）、Strassen（7 次遞迴，n ≤ 32 改用三層迴圈）、最大子陣列分治版、多數元素分治版 |
| `test_dnc.c` | n = 1..128 的隨機矩陣三種乘法結果一致、CLRS 第 3 版的股價例子（答案 43）與 2000 組隨機資料對照暴力法、多數元素 |

執行：`make test T=27-divide-and-conquer`

## 出處
- **CLRS** 第 4 章 Divide-and-Conquer · PDF p.119（4.1 Multiplying square matrices p.125、4.2 Strassen's algorithm p.131、4.3–4.5 解遞迴式）
- **CLRS** 2.3.1 分治法的三個步驟 · PDF p.64
- **Thareja** 2.6 Different Approaches to Designing an Algorithm（由上而下、由下而上）· p.51
