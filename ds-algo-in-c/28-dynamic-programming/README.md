# 28 · Dynamic Programming 動態規劃

## 一句話
**動態規劃 (DP)** 跟分治法一樣，把問題拆成子問題；不同的是**子問題會重複出現**，所以每個子問題只算一次，把答案存在表格裡，之後直接查表。

## 什麼時候能用 DP（CLRS 14.3）
1. **最佳子結構 (optimal substructure)**：最佳解裡面包含子問題的最佳解。
2. **子問題重疊 (overlapping subproblems)**：同一個子問題會被問很多次。沒有重疊的話用分治就好。

## 寫 DP 的四個步驟
1. **定義狀態**：`dp[i]`（或 `dp[i][j]`）代表什麼？**這一步最重要。**
2. **轉移方程式**：`dp[i]` 怎麼從比較小的狀態算出來？通常是想「**最後一步**做了什麼」。
3. **初始值 (base case)** 和**計算順序**：算 `dp[i]` 的時候，它需要的狀態都要已經算好了。
4. **答案在哪一格**；需要的話，再往回走還原出解本身。

## 兩種寫法
| | 由上而下 (top-down) + 備忘錄 (memoization) | 由下而上 (bottom-up) 填表 |
|---|---|---|
| 寫法 | 遞迴，算過的存起來 | 迴圈，依序填表 |
| 優點 | 直接照遞迴式寫，只算需要的子問題 | 沒有遞迴成本，容易壓縮空間 |
| 缺點 | 遞迴太深會爆堆疊 | 要想清楚填表的順序 |

本專案的 `rod_cut_memo` 和 `rod_cut` 就是同一題的兩種寫法。

## CLRS 第 14 章的例子

### 切鋼條 (rod cutting, 14.1)
```
price: 長度 1  2  3  4  5  6  7  8  9  10
       價錢 1  5  8  9 10 17 17 20 24  30
r[j] = max over i (price[i] + r[j − i])     ← 第一刀切下長度 i，剩下的用最佳切法
r[4] = 10（切成 2 + 2）   r[7] = 18（1 + 6 或 2 + 2 + 3）   r[10] = 30（不切）
```

### 矩陣鏈乘 (matrix-chain multiplication, 14.2)
A₁A₂…Aₙ 怎麼加括號，讓純量乘法次數最少？（矩陣乘法有結合律，但不同順序的成本差很多）
```
m[i][j] = min over i ≤ k < j ( m[i][k] + m[k+1][j] + p[i−1]·p[k]·p[j] )
依「鏈的長度」由短到長填表
CLRS 的例子 30×35, 35×15, 15×5, 5×10, 10×20, 20×25 → 最少 15125 次：((A1(A2A3))((A4A5)A6))
```

### 最長共同子序列 (LCS, 14.4)
```
c[i][j] = x 的前 i 個 和 y 的前 j 個 的 LCS 長度
  x[i] == y[j] → c[i−1][j−1] + 1
  否則        → max(c[i−1][j], c[i][j−1])

      ""  B  D  C  A  B  A
  ""   0  0  0  0  0  0  0
  A    0  0  0  0  1  1  1
  B    0  1  1  1  1  2  2
  C    0  1  1  2  2  2  2
  B    0  1  1  2  2  3  3
  D    0  1  2  2  2  3  3
  A    0  1  2  2  3  3  4
  B    0  1  2  2  3  4  4    → LCS 長度 4（例如 BCBA）
```

## 常見的 DP 類型
| 類型 | 狀態 | 例子 |
|---|---|---|
| 線性 | `dp[i]` 前 i 個的答案 | 爬樓梯 (70)、打家劫舍 (198)、LIS (300) |
| 背包 | `dp[容量]` | 0/1 背包、零錢兌換 (322) |
| 兩個序列 | `dp[i][j]` 兩個前綴 | LCS (1143)、編輯距離 (72) |
| 區間 | `dp[i][j]` 區間 [i, j] | 矩陣鏈乘、最佳 BST (CLRS 14.5)、戳氣球 (312) |
| 樹上 | 每個節點回傳 (選它, 不選它) | 337 House Robber III |

## 複雜度
| 函式 | 時間 | 空間 |
|---|---|---|
| `rod_cut` / `rod_cut_memo` | O(n²) | O(n) |
| `matrix_chain` | O(n³) | O(n²) |
| `lcs` | O(nm) | O(nm)（要還原字串） |
| `edit_distance` | O(nm) | O(m)，只留一列 |
| `knapsack01` | O(n · cap) | O(cap) |
| `lis_length` | O(n log n) | O(n) |

## 面試陷阱／常考點
1. **先把狀態定義講清楚**，再寫轉移方程式。面試官最在意的就是這一步。
2. **空間壓縮**：如果 `dp[i]` 只需要 `dp[i−1]`，就只留一列（或兩個變數）。
3. **0/1 背包的容量要由大到小掃**：由小到大的話，同一樣東西會被用好幾次（那是「完全背包」）。
4. **貪心不一定對**：零錢兌換用面額 [1, 3, 4] 湊 6，貪心拿 4+1+1 是 3 枚，DP 答案是 3+3 只要 2 枚。
5. **LIS 的 O(n log n) 解法**：`tails` 陣列本身不是一個 LIS，只有它的**長度**是對的。
6. **初始值很容易錯**：「湊出 0 元要 0 枚」、「空字串的 LCS 是 0」、「不可能的狀態」要設成 ∞ 或 −1。
7. **遞迴 + memo 在 n 很大時會爆堆疊**，這時改成迴圈填表。
8. **DP vs 分治 vs 貪心**：子問題不重疊 → 分治；子問題重疊 → DP；每一步的局部最佳選擇就能組成全域最佳解 → 貪心（第 29 主題）。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 70 | [Climbing Stairs](https://leetcode.com/problems/climbing-stairs/) | Easy | 最基本的一維 DP |
| 198 | [House Robber](https://leetcode.com/problems/house-robber/) | Medium | 選或不選 |
| 322 | [Coin Change](https://leetcode.com/problems/coin-change/) | Medium | 完全背包、貪心的反例 |
| 1143 | [Longest Common Subsequence](https://leetcode.com/problems/longest-common-subsequence/) | Medium | 兩個序列的 DP |
| 300 | [Longest Increasing Subsequence](https://leetcode.com/problems/longest-increasing-subsequence/) | Medium | O(n²) → O(n log n) |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `dp.h` / `.c` | 切鋼條（由下而上 + 還原切法、由上而下 + 備忘錄）、矩陣鏈乘（含括號字串）、LCS（含還原）、編輯距離（一列空間）、0/1 背包、LIS O(n log n) |
| `test_dp.c` | CLRS 的切鋼條價目表 r1..r10、矩陣鏈乘 15125 與括號、LCS 例子；隨機測試對照暴力法：切鋼條枚舉、LCS／編輯距離遞迴、背包枚舉 2ⁿ、LIS 的 O(n²) 版 |

執行：`make test T=28-dynamic-programming`

## 出處
- **CLRS** 第 14 章 Dynamic Programming · PDF p.481（14.1 Rod cutting p.482、14.2 Matrix-chain multiplication p.495、14.3 Elements of DP p.506、14.4 LCS p.521、14.5 Optimal BST p.529）
- **Thareja**：沒有專門章節（附錄 C 的回溯法 p.514 是相關的遞迴技巧）
