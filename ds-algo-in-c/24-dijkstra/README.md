# 24 · Dijkstra 最短路徑

## 一句話
**Dijkstra 演算法**找「單一起點到所有頂點」的最短路徑，但**所有邊的權重都必須 ≥ 0**。它像 BFS，只是把 queue 換成 **priority queue**：每次處理「目前距離最小、還沒確定」的頂點。

## 鬆弛 (relaxation)：所有最短路徑演算法的核心（CLRS 22 章開頭）
```
如果 dist[u] + w(u, v) < dist[v]：
    dist[v] = dist[u] + w(u, v)     ← 找到一條經過 u、更短的路
    parent[v] = u
```

## 演算法
```
dist[s] = 0，其他都是 ∞
把 (0, s) 放進 min-heap
重複：
    拿出距離最小的 u
    → 這時 dist[u] 就「確定」了，之後不會再變
    對 u 的每個鄰居 v 做鬆弛
```

### 為什麼拿出來就確定了？（貪心的正確性）
u 是目前所有未確定頂點中 dist 最小的。任何其他路徑想到 u，都必須經過另一個未確定的頂點 x，而 dist[x] ≥ dist[u]。**邊的權重都 ≥ 0**，所以從 x 再走到 u 只會更長，不會更短。

### 為什麼負權重不行？
```
s ──1──► a
│        ▲
4       -5
▼        │
b ───────┘
Dijkstra 會先確定 a（距離 1），但 s→b→a = 4 + (−5) = −1 更短
```
有負權重就要用 Bellman-Ford（第 25 主題）。

## 圖示
```
CLRS Figure 22.6 的有向圖：
  s→t 10   s→y 5
  t→x 1    t→y 2
  y→t 3    y→x 9    y→z 2
  x→z 4
  z→s 7    z→x 6

處理順序（每次拿距離最小的）：
  拿 s(0)  → t=10, y=5
  拿 y(5)  → t=8,  x=14, z=7
  拿 z(7)  → x=13
  拿 t(8)  → x=9
  拿 x(9)
最短距離：s=0  y=5  z=7  t=8  x=9
s 到 x 的最短路徑：s → y → t → x（5 + 3 + 1 = 9）
```

### Lazy 刪除
二元 heap 不支援 decrease-key（除非另外記每個頂點在 heap 的位置）。簡單的做法是：**直接 push 新的 (距離, 頂點)**，舊的留在 heap 裡。拿出來時如果 `d > dist[u]`，就是過期的，跳過。heap 最多會有 E 個元素。

## 複雜度
| 實作 | 時間 | 適合 |
|---|---|---|
| 陣列（線性找最小） | **O(V²)** | 稠密圖、V 很小（LeetCode 743 的 n ≤ 100） |
| 二元 heap（lazy） | O((V + E) log E) | 稀疏圖，最常用 |
| Fibonacci heap | O(E + V log V) | 理論上最好 |

## 面試陷阱／常考點
1. **不能有負權重。** 題目一有負權重就要換演算法。
2. **拿出來時要檢查是不是過期的**（`d > dist[u]` 就跳過），否則會重複處理，最壞變成指數時間。
3. **距離要用 `long long`**：權重 10⁹ × 路徑長度很容易超過 int（LeetCode 1976）。
4. **只要一個終點時，可以提早結束**：第一次從 heap 拿出終點時，它的距離就確定了。
5. **Dijkstra 的變形**：只要路徑代價「只會越走越差」（單調），貪心就成立：
   - 機率相乘取最大（1514）
   - 路徑上的最大高度差取最小（1631）
   - 同時數最短路徑有幾條（1976）
6. **權重只有 0 和 1**：用 **0-1 BFS**（deque，權重 0 放前面、1 放後面），O(V + E)。
7. **所有權重都一樣**：直接用 BFS 就好（第 20 主題）。
8. **無向圖**：每條邊加兩個方向。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 743 | [Network Delay Time](https://leetcode.com/problems/network-delay-time/) | Medium | 標準 Dijkstra |
| 1514 | [Path with Maximum Probability](https://leetcode.com/problems/path-with-maximum-probability/) | Medium | 相乘取最大，max-heap |
| 1631 | [Path With Minimum Effort](https://leetcode.com/problems/path-with-minimum-effort/) | Medium | 代價取 max 而不是相加 |
| 1976 | [Number of Ways to Arrive at Destination](https://leetcode.com/problems/number-of-ways-to-arrive-at-destination/) | Medium | 數最短路徑有幾條 |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `dijkstra.h` / `.c` | CSR 加權有向圖、二元 heap 版（lazy 刪除）、陣列版 O(V²)、路徑還原 |
| `test_dijkstra.c` | CLRS 圖 22.6 的距離與路徑、1000 張隨機圖（含 0 權重、重邊、自環）：兩種版本都對照 Bellman-Ford，並檢查還原出的路徑權重加總剛好等於距離 |

執行：`make test T=24-dijkstra`

## 出處
- **CLRS** 第 22 章 Single-Source Shortest Paths · PDF p.785（開頭：鬆弛與最短路徑的性質）；22.3 Dijkstra's algorithm · PDF p.804
- **Thareja** 13.8.4 Dijkstra's Algorithm · p.413
