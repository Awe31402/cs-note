# 34 · NP 觀念

> 這個主題依照計劃**只寫筆記、沒有主程式**。LeetCode 題目還是附上 C 解法，用來示範「遇到 NP-hard 問題、但 n 很小時，實際上怎麼解」。

## 一句話
有些問題，**檢查一個答案對不對很快**，但**找出答案**好像只能暴力試。**P vs NP** 問的就是：「能很快檢查的問題，是不是也都能很快解？」這是電腦科學最有名的未解問題。面試要會的是：**認出 NP-hard 的問題，然後知道該怎麼退而求其次**。

## 名詞（CLRS 34.1–34.3）
「很快」= **多項式時間 (polynomial time)**，O(nᵏ)。
| 名詞 | 意思 | 例子 |
|---|---|---|
| **P** | 能在多項式時間**解出來**的判定問題 | 最短路徑、排序、二分匹配、最大流 |
| **NP** | 給一個答案（證書 certificate），能在多項式時間**驗證**它對不對 | 「這組數字加起來是不是剛好 k？」一加就知道 |
| **NP-hard** | 至少跟 NP 裡**最難**的一樣難（所有 NP 問題都能歸約到它） | TSP 的最佳化版本 |
| **NP-complete (NPC)** | 既在 NP 裡，又是 NP-hard | SAT、3-SAT、CLIQUE、VERTEX-COVER、HAM-CYCLE、TSP（判定版）、SUBSET-SUM |

```
        NP-hard
     ┌───────────┐
┌────┼─────┐     │
│ NP │ NPC │     │     P ⊆ NP 一定成立
│ ┌─┐│     │     │     P = NP？沒人知道（大部分人相信 ≠）
│ │P││     │     │     任何一個 NPC 問題有多項式解 → P = NP
│ └─┘│     │     │
└────┼─────┘     │
     └───────────┘
```
注意 NP 是 **Nondeterministic Polynomial**（非確定性多項式），**不是** "Not Polynomial"。

## 歸約 (reduction, CLRS 34.3)
**A ≤ₚ B**：能在多項式時間把 A 的任何輸入轉成 B 的輸入，而且答案一樣。意思是「**B 至少跟 A 一樣難**」：會解 B 就會解 A。

**證明一個新問題 X 是 NPC 的標準步驟**：
1. 證明 X ∈ NP（給答案能很快驗證）。
2. 找一個已知的 NPC 問題 Y，證明 **Y ≤ₚ X**（方向不要搞反！是把已知難題轉成新問題）。

CLRS 34.5 的歸約鏈（Figure 34.13）：
```
CIRCUIT-SAT → SAT → 3-CNF-SAT ─┬→ CLIQUE → VERTEX-COVER → HAM-CYCLE → TSP
                                └→ SUBSET-SUM
```

## 常見的 NPC 問題：面試時要認得出來
| 問題 | 描述 | 看起來很像、但是 P 的問題 |
|---|---|---|
| **TSP**（旅行推銷員） | 走過所有城市再回來的最短路線 | 最小生成樹（第 23 主題） |
| **HAM-CYCLE**（漢彌爾頓環） | 經過**每個頂點**剛好一次的環 | **尤拉環**：經過每條**邊**一次，O(E) |
| **最長簡單路徑** | 不重複頂點的最長路徑 | 最短路徑（第 24 主題）；DAG 上的最長路徑 |
| **CLIQUE**（最大團） | 最大的「兩兩相連」頂點集合 | — |
| **VERTEX-COVER**（最小點覆蓋） | 最少幾個點能碰到每一條邊 | **二分圖**上是 P（König 定理，第 33 主題） |
| **最大獨立集** | 最多幾個點彼此不相鄰 | **二分圖**上是 P（LeetCode 1349） |
| **SUBSET-SUM / PARTITION** | 能不能選出加總剛好 k 的子集合 | 數字不大時有**偽多項式** DP：O(n · k) |
| **0/1 背包** | 第 28 主題 | 分數背包是 P（第 29 主題） |
| **圖著色**（k ≥ 3） | 用 k 種顏色塗，相鄰不同色 | **2 色**就是判斷二分圖，O(V + E) |
| **SAT / 3-SAT** | 布林式能不能被滿足 | **2-SAT** 是 P（用強連通元件） |

**偽多項式 (pseudo-polynomial)**：SUBSET-SUM 的 DP 是 O(n · k)，看起來是多項式，但 k 是**數值**、要用 log k 個位元表示，所以對輸入長度來說是指數的。

## 遇到 NP-hard 怎麼辦
| 做法 | 什麼時候用 | 例子 |
|---|---|---|
| **位元 DP (bitmask DP)** | n ≤ 20 左右 | Held-Karp TSP O(2ⁿ n²)、LeetCode 698、847 |
| **回溯 + 剪枝** | n 小、剪枝有效 | N 皇后、LeetCode 473 |
| **偽多項式 DP** | 數值不大 | SUBSET-SUM、0/1 背包 |
| **近似演算法**（CLRS 35） | 要有品質保證的「夠好」答案 | 點覆蓋 2 倍近似、TSP（滿足三角不等式）2 倍近似 |
| **特殊結構** | 圖是二分圖、樹、DAG | 樹上的最大獨立集用樹 DP，O(n) |
| **啟發式 (heuristic)** | 實務上的大型問題 | 貪心、區域搜尋、模擬退火、SAT solver |

### 近似演算法的例子（CLRS 35.1）
**點覆蓋 2 倍近似**：隨便挑一條還沒被覆蓋的邊 (u, v)，**兩端都選**，刪掉所有碰到 u 或 v 的邊，重複。
為什麼最多是最佳解的 2 倍：挑出的邊彼此不共用端點，最佳解對每一條都至少要選一個端點。

## 面試陷阱／常考點
1. **看到限制 n ≤ 15～20**：幾乎就是在暗示指數解（bitmask DP 或回溯）。
2. **「看起來很像」的問題難度可能天差地遠**：尤拉環 vs 漢彌爾頓環、最短路徑 vs 最長路徑、2-SAT vs 3-SAT、2 色 vs 3 色。
3. **歸約的方向**：證明 X 很難，是把「已知的難題」轉成 X，不是反過來。
4. **NP 不是 "Not Polynomial"**。
5. **不要說「NP 問題就是沒辦法解」**：它們都解得出來，只是（目前已知的方法）要指數時間。
6. **近似比 (approximation ratio)**：演算法的解最多是最佳解的幾倍。

## 練習題（LeetCode）
這三題都是 NP-hard 問題的小規模版本，示範三種常見的指數解法：
| # | 題目 | 難度 | 解法 |
|---|---|---|---|
| 698 | [Partition to K Equal Sum Subsets](https://leetcode.com/problems/partition-to-k-equal-sum-subsets/) | Medium | 位元 DP |
| 473 | [Matchsticks to Square](https://leetcode.com/problems/matchsticks-to-square/) | Medium | 回溯 + 剪枝 |
| 847 | [Shortest Path Visiting All Nodes](https://leetcode.com/problems/shortest-path-visiting-all-nodes/) | Hard | (點, 集合) 狀態的 BFS |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
這個主題沒有主程式（依照計劃書）。`leetcode/` 裡的 3 個檔案都有本機測試，會對照暴力法（k 組分法枚舉、4ⁿ 枚舉、Held-Karp DP）。

執行：`make test T=34-np`

## 出處
- **CLRS** 第 34 章 NP-Completeness · PDF p.1343（34.1 多項式時間 p.1350、34.2 多項式時間驗證 p.1360、34.3 NP 完備與歸約 p.1366、34.4 NP 完備證明 p.1381、34.5 NP 完備問題 p.1391）
- **CLRS** 第 35 章 Approximation Algorithms · PDF p.1423（35.1 點覆蓋 p.1426、35.2 TSP p.1430）
- **Thareja**：沒有
