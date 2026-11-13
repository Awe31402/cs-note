# 29 · Greedy 貪心法

## 一句話
**貪心法 (greedy)** 每一步都做「**現在看起來最好**」的選擇，而且**選了就不回頭**。對的時候又快又簡單；但很多問題用貪心會出錯，所以**一定要能說明為什麼貪心是對的**。

## 什麼時候貪心是對的（CLRS 15.2）
1. **貪心選擇性質 (greedy-choice property)**：存在一個最佳解，它包含我們貪心挑的第一步。
2. **最佳子結構 (optimal substructure)**：做完貪心選擇後，剩下的子問題的最佳解，加上這一步，就是整體最佳解。

證明的常用招數是**交換論證 (exchange argument)**：假設有一個最佳解沒選貪心的那一步，把它的某個選擇換成貪心的選擇，結果不會變差，所以「包含貪心選擇的最佳解」一定存在。

### 貪心 vs DP
| | 貪心 | 動態規劃 |
|---|---|---|
| 每一步 | 只看目前，選了不回頭 | 考慮所有子問題的答案 |
| 速度 | 通常 O(n log n)，排序為主 | 通常 O(n²) 以上 |
| 正確性 | 要證明 | 只要轉移方程式對就對 |
| 例子 | 分數背包 ✅ | 0/1 背包 ✅（貪心 ❌） |

## CLRS 第 15 章的例子

### 活動選擇 (activity selection, 15.1)
很多活動要用同一間教室，每個活動有開始和結束時間。最多能排幾個？
**貪心：永遠選「最早結束」的活動**，因為它留給後面的時間最多。
```
依結束時間排序後：
a1 [1,4)  a2 [3,5)  a3 [0,6)  a4 [5,7)  a5 [3,9)  ...  a11 [12,16)
選 a1 → 下一個開始 ≥ 4 的最早結束是 a4 → a8 [8,11) → a11
答案 {a1, a4, a8, a11}
```
錯誤的貪心：選「最早開始」的、選「最短」的，都有反例。

### Huffman 編碼 (15.3)
字元出現頻率不同，常見的字元用短的編碼、少見的用長的，讓總位元數最少。編碼要是**前綴碼 (prefix code)**：沒有任何編碼是另一個的開頭，解碼才不會有歧義。
**貪心：反覆把「頻率最小的兩個」合併成一個新節點。**
```
a:45 b:13 c:12 d:16 e:9 f:5
合併 f(5)+e(9)=14 → c(12)+b(13)=25 → 14+d(16)=30 → 25+30=55 → a(45)+55=100
                 100
              0/     \1
           a:45      55
                  0/    \1
                  25     30
                 / \    /  \
               c:12 b:13 14  d:16
                        / \
                      f:5 e:9
a=0  c=100  b=101  f=1100  e=1101  d=111   總共 224（千位元），固定長度要 300
```

### 分數背包 vs 0/1 背包
- **分數背包**（東西可以切開）：照「單位價值 v/w」由高到低拿，拿不下就切一部分。貪心是對的。
- **0/1 背包**（不能切）：貪心是錯的，要用 DP（第 28 主題）。
```
容量 50：(w=10, v=60) (w=20, v=100) (w=30, v=120)
分數：60 + 100 + 120 × 20/30 = 240
0/1：照單位價值貪心拿 60 + 100 = 160；最佳是 100 + 120 = 220
```

### 離線快取 (offline caching, 15.4)
快取只放得下 k 個東西。事先知道所有的請求順序時，快取滿了要踢掉誰？
**貪心（Belady）：踢掉「下次被用到的時間最遠」的那個**（或再也不會用到的）。這是最佳的，也是 LRU 等實際策略的比較基準。

## 複雜度
| 函式 | 時間 |
|---|---|
| `activity_select`（已排序） | O(n) |
| `huffman` | O(n log n) |
| `fractional_knapsack` | O(n log n) |
| `offline_cache_misses` | O(m · k · m)（本專案寫的是直接的版本） |

## 面試陷阱／常考點
1. **區間問題先想「依結束時間排序」**：活動選擇、刪最少區間（435）、最少箭數（452）都是同一招。
2. **一定要能舉反例或說明為什麼對**：面試官常問「為什麼貪心可以？」
3. **0/1 背包、零錢兌換（一般面額）不能用貪心**。
4. **排序的比較函式不要用減法**：座標範圍是整個 int 時，`a - b` 會溢位（452）。
5. **qsort 收到 NULL 是未定義行為**：陣列可能是空的時候要先判斷（UBSan 在 455 抓到這個）。
6. **「從前面走不下去，就換起點」**：加油站（134）的關鍵論證。
7. **Jump Game** 只要維護「目前最遠能到哪裡」，不需要 DP。
8. **Dijkstra、Prim、Kruskal 都是貪心法**（第 23、24 主題）。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 455 | [Assign Cookies](https://leetcode.com/problems/assign-cookies/) | Easy | 兩邊排序，小配小 |
| 55 | [Jump Game](https://leetcode.com/problems/jump-game/) | Medium | 維護最遠可達 |
| 435 | [Non-overlapping Intervals](https://leetcode.com/problems/non-overlapping-intervals/) | Medium | 就是活動選擇 |
| 452 | [Minimum Number of Arrows to Burst Balloons](https://leetcode.com/problems/minimum-number-of-arrows-to-burst-balloons/) | Medium | 依右端點排序 |
| 134 | [Gas Station](https://leetcode.com/problems/gas-station/) | Medium | 換起點的論證 |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `greedy.h` / `.c` | 活動選擇、Huffman 編碼（min-heap 合併 + DFS 產生編碼）、分數背包（交叉相乘比較，不用浮點數排序）、離線快取（furthest-in-future） |
| `test_greedy.c` | CLRS 活動選擇例子與 500 組枚舉對照、CLRS Huffman（224）與另一個獨立 O(n²) 實作對照、前綴碼與 Kraft 等式檢查、CLRS 背包例子、離線快取對照「走遍所有快取狀態」的 DP |

執行：`make test T=29-greedy`

## 出處
- **CLRS** 第 15 章 Greedy Algorithms · PDF p.552（15.1 活動選擇 p.553、15.2 貪心的要素與分數／0-1 背包 p.563、15.3 Huffman codes p.570、15.4 Offline caching p.581）
- **Thareja** 9.5 Huffman's Tree · p.290
