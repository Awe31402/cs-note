# 20 · BFS / DFS 圖的走訪

## 一句話
**圖 (graph)** 是一堆**頂點 (vertex)** 和連接它們的**邊 (edge)**。走訪圖有兩種基本方法：
- **BFS（廣度優先搜尋）**：一圈一圈往外擴散，用 **queue**。**在權重都是 1 的圖上，BFS 找到的就是最短路徑。**
- **DFS（深度優先搜尋）**：一條路走到底，走不下去再回頭，用**遞迴或 stack**。

## 圖的表示法（CLRS 20.1、Thareja 13.5）
```
    0 ── 1
    │  ╱ │
    2    3

鄰接矩陣 (adjacency matrix)       鄰接串列 (adjacency list)
    0 1 2 3                         0: 1 2
0 [ 0 1 1 0 ]                       1: 0 2 3
1 [ 1 0 1 1 ]                       2: 0 1
2 [ 1 1 0 0 ]                       3: 1
3 [ 0 1 0 0 ]
```
| | 鄰接矩陣 | 鄰接串列 |
|---|---|---|
| 空間 | O(V²) | **O(V + E)** |
| 查「u、v 之間有沒有邊」 | **O(1)** | O(deg(u)) |
| 列出 u 的鄰居 | O(V) | **O(deg(u))** |
| 適合 | 稠密圖 (dense) | 稀疏圖 (sparse)，大部分題目都是這種 |

## BFS
```
從 s 出發（CLRS Figure 20.3）：
第 0 層: s
第 1 層: r w           ← s 的鄰居
第 2 層: v t x         ← 第 1 層的鄰居（還沒看過的）
第 3 層: u y
```
- 一個頂點**第一次被發現時**就設好距離，之後不會再變小。
- 記錄 `parent[v]`，就能從終點倒著走回起點，還原最短路徑。

## DFS 與時間戳記
每個頂點記兩個時間：`d[v]` 發現時間（第一次走到）、`f[v]` 完成時間（它的鄰居都處理完了）。
```
       [1          8]      u
          [2    7]         v
            [3 6]          y
             [4 5]         x
 [9      12]               w
    [10 11]                z
```
**括號定理 (parenthesis theorem)**：任兩個頂點的區間 `[d, f]`，不是完全不相交，就是一個完全包在另一個裡面。**不會交錯**。本專案的測試對每一對頂點都檢查了這件事。

### 邊的分類（有向圖）
| 邊 u → v | 條件 | 意義 |
|---|---|---|
| 樹邊 (tree) | v 是白的（還沒發現） | DFS 樹上的邊 |
| **後向邊 (back)** | v 是灰的（還在處理中） | **有環！** |
| 前向邊 (forward) | v 是黑的，d[u] < d[v] | 指向子孫 |
| 交叉邊 (cross) | v 是黑的，d[u] > d[v] | 其他 |

「有向圖有環 ⇔ DFS 會遇到後向邊」，這是第 21 主題拓撲排序的基礎。

### 強連通元件 (SCC, CLRS 20.5)
有向圖裡「互相走得到」的最大頂點集合。Kosaraju 演算法：先 DFS 一次記下完成順序，把所有邊反向，再依完成時間**由大到小**做第二次 DFS，每棵 DFS 樹就是一個 SCC。O(V + E)。

## 複雜度
| 操作 | 時間 | 額外空間 |
|---|---|---|
| BFS | O(V + E) | O(V)，queue |
| DFS | O(V + E) | O(V)，遞迴深度最多 V |
| 連通元件 | O(V + E) | O(V) |
| 網格 (m × n) 上的 BFS/DFS | O(mn) | O(mn) |

## 面試陷阱／常考點
1. **放進 queue 的時候就標記「已拜訪」**，不要等到拿出來才標記，否則同一個頂點會被放進 queue 很多次。
2. **網格題就是圖**：每一格是頂點，上下左右（或 8 個方向）是邊。用 `dr[]`、`dc[]` 陣列列出方向。
3. **遞迴 DFS 在大網格上會爆堆疊**：300 × 300 全部是陸地時，深度可以到 9 萬層。改用 BFS 或自己的 stack（LeetCode 200）。
4. **多源 BFS (multi-source BFS)**：一開始把所有起點一起放進 queue，就能同時擴散（LeetCode 994）。
5. **分層 BFS**：記下每一層的寬度，就能算「第幾步」（994、102）。
6. **BFS 只保證「邊數」最少。** 邊有不同的權重時，要用 Dijkstra（第 24 主題）。
7. **圖不一定連通**：DFS 要從每個還沒走過的頂點都出發一次。
8. **無向圖加邊要加兩次**（u→v 和 v→u）。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 733 | [Flood Fill](https://leetcode.com/problems/flood-fill/) | Easy | 網格 DFS |
| 200 | [Number of Islands](https://leetcode.com/problems/number-of-islands/) | Medium | 數連通元件 |
| 841 | [Keys and Rooms](https://leetcode.com/problems/keys-and-rooms/) | Medium | 有向圖可達性 |
| 994 | [Rotting Oranges](https://leetcode.com/problems/rotting-oranges/) | Medium | 多源 BFS、分層 |
| 1091 | [Shortest Path in Binary Matrix](https://leetcode.com/problems/shortest-path-in-binary-matrix/) | Medium | BFS 最短路徑 |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `graph.h` / `.c` | 鄰接串列、BFS（距離與 parent）、路徑還原、遞迴 DFS（發現與完成時間）、迴圈版 DFS、無向圖連通元件 |
| `test_graph.c` | CLRS 圖 20.3 的 BFS 距離、300 張隨機有向圖：BFS 對照暴力鬆弛、每條還原路徑的長度、DFS 括號定理、連通元件 |

執行：`make test T=20-bfs-dfs`

## 出處
- **CLRS** 第 20 章 Elementary Graph Algorithms · PDF p.718（20.1 表示法、20.2 BFS p.724、20.3 DFS p.736、20.5 強連通元件 p.750）
- **Thareja** 13.1–13.3 名詞與有向圖 p.383–386、13.5 表示法 p.388–393、13.6 走訪 p.393（13.6.1 BFS p.394、13.6.2 DFS p.397）
