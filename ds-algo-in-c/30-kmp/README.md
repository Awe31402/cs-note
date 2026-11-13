# 30 · KMP 字串比對

## 一句話
**字串比對 (string matching)**：在長度 n 的文字 T 裡，找出長度 m 的樣式 P 所有出現的位置。
**KMP（Knuth-Morris-Pratt）** 先花 O(m) 分析 P 自己的結構（前綴函數 π），之後掃 T 時**永遠不用往回走**，總共 O(n + m)。

## 暴力法為什麼慢（CLRS 32.1）
每個起點都從頭比：
```
T = a a a a a a a a b      P = a a a b
    a a a ✗                 ← 比了 4 次才失敗
      a a a ✗               ← 又從頭比，前面比過的全部浪費
        ...
最壞 O((n − m + 1) · m)
```

## KMP 的想法
失敗的時候，**已經對上的那段字元我們是知道的**（就是 P 的前綴）。利用它，直接把 P 往右滑到「還有可能」的位置，T 的指標不用退回。

### 前綴函數 π（prefix function）
`π[q]` = P[0..q] 的「最長、而且不是整個字串」的**前綴，同時也是它的後綴**，長度是多少。
```
P = a b a b a c a     （CLRS Figure 32.10）
q   0 1 2 3 4 5 6
π   0 0 1 2 3 0 1

π[4] = 3：P[0..4] = "ababa"，"aba" 既是前綴也是後綴
```

### 用 π 滑動
```
T: ... a b a b a b a c a ...
P:     a b a b a c a
                   ✗        已經對上 "ababa"（q = 5），下一個字元不同
π[4] = 3 → 已對上的 "ababa" 裡，最後 3 個 "aba" 就是 P 開頭的 3 個
P 直接往右滑，讓 q = 3，從 T 的同一個位置繼續比：
T: ... a b a b a b a c a ...
P:         a b a b a c a   ✅
```
T 的指標只會往前，每次失敗 q 至少變小 1，所以總共 O(n)。

### 計算 π 本身
就是「P 跟自己做 KMP」：已經有長度 k 的前綴 = 後綴，下一個字元對得上就 k+1；對不上就退到 `π[k−1]` 再試。O(m)。

## Rabin-Karp（CLRS 32.2）
把長度 m 的字串當成一個 256 進位的數字，取模數 Q 當作雜湊值。
滑動到下一個位置時，**扣掉最左邊的字元、乘上基底、加上新字元**，O(1) 更新：
```
hash("bcd") = (hash("abc") − 'a'·B²) · B + 'd'     (mod Q)
```
雜湊值相同還要逐字確認，因為不同字串可能剛好雜湊值一樣（稱為 spurious hit）。平均 O(n + m)，最壞 O(nm)。
適合**同時找很多個樣式**、或找重複的子字串（搭配二分搜尋）。

## Z 函數
`z[i]` = s 和 s[i..] 的最長共同前綴長度。用 [l, r) 區段借用之前的結果，O(n)。
把 P + "#" + T 算 Z 函數，`z[i] == m` 的位置就是一次匹配。跟 KMP 能解的題目大多相同。

## 複雜度
| 演算法 | 前處理 | 比對 | 備註 |
|---|---|---|---|
| 暴力 | 0 | O((n − m + 1) m) | 隨機文字時其實很快 |
| Rabin-Karp | O(m) | 平均 O(n + m)，最壞 O(nm) | 多樣式、滾動雜湊 |
| 有限狀態機（CLRS 32.3） | O(m · |Σ|) | O(n) | 字母表大時很占空間 |
| **KMP** | **O(m)** | **O(n)** | 最壞也保證線性 |
| Z 函數 | — | O(n + m) | 跟 KMP 等價 |

## 面試陷阱／常考點
1. **π 的定義要講清楚**：「最長的、真的 (proper) 前綴，同時也是後綴」。不能是整個字串本身，否則永遠等於長度。
2. **找到一次之後，`q = π[m−1]` 繼續找**，這樣重疊的出現也找得到（"aa" 在 "aaa" 出現 2 次）。
3. **π 的神奇應用**（都在 LeetCode 題目裡）：
   - 最長的「既是前綴又是後綴」：直接就是 π[n−1]（1392）。
   - 最小週期：n − π[n−1]，n 能被它整除就是重複字串（459）。
   - 最長回文前綴：對 `s + "#" + reverse(s)` 算 π（214）。
4. **Rabin-Karp 的無號數相減**：先加 Q 再減，否則會變成很大的數。
5. **C 的 `strstr`** 就是字串比對，但標準沒有規定演算法，面試時不能拿來交差。
6. **後綴陣列 (suffix array, CLRS 32.5)**：一次處理之後可以回答很多種子字串問題，例如最長重複子字串。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 28 | [Find the Index of the First Occurrence in a String](https://leetcode.com/problems/find-the-index-of-the-first-occurrence-in-a-string/) | Easy | 標準 KMP |
| 459 | [Repeated Substring Pattern](https://leetcode.com/problems/repeated-substring-pattern/) | Easy | 用 π 求最小週期 |
| 1392 | [Longest Happy Prefix](https://leetcode.com/problems/longest-happy-prefix/) | Hard | 就是 π[n−1] |
| 214 | [Shortest Palindrome](https://leetcode.com/problems/shortest-palindrome/) | Hard | s + # + 反轉 |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `kmp.h` / `.c` | 暴力比對、Rabin-Karp（滾動雜湊 + 逐字確認）、KMP 前綴函數、KMP 比對（含重疊）、Z 函數 |
| `test_kmp.c` | CLRS 圖 32.10 的 π、圖 32.1 的例子、5000 組小字母表隨機測試（三種方法結果一致、π 和 Z 都對照定義）、20 萬字元的最壞情況 |

執行：`make test T=30-kmp`

## 出處
- **CLRS** 第 32 章 String Matching · PDF p.1234（32.1 暴力法 p.1237、32.2 Rabin-Karp p.1240、32.3 有限狀態機 p.1247、32.4 KMP p.1257、32.5 Suffix arrays p.1270）
- **Thareja** 4.2 Operations on Strings（找子字串的位置，暴力法）· p.118
