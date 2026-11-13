# 26 · Floyd-Warshall 所有點對最短路徑

## 一句話
**Floyd-Warshall** 一次算出**每一對頂點之間**的最短距離。核心只有三層迴圈：「從 i 到 j，經過 k 會不會比較近？」。O(V³)，可以有負權重，也能偵測負環。

## 核心想法：動態規劃 (DP)
定義 d⁽ᵏ⁾[i][j] = 從 i 到 j、**中間只允許經過頂點 0..k** 的最短距離。
```
d⁽ᵏ⁾[i][j] = min( d⁽ᵏ⁻¹⁾[i][j],                  ← 不經過 k
                  d⁽ᵏ⁻¹⁾[i][k] + d⁽ᵏ⁻¹⁾[k][j] )  ← 經過 k
```
全部 k 都允許之後，就是真正的最短距離。實作時可以原地更新同一個矩陣（CLRS 習題 23.2-4）。

```c
for (k = 0; k < n; k++)          // k 一定要放在最外層！
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            d[i][j] = min(d[i][j], d[i][k] + d[k][j]);
```

## 圖示
```
CLRS Figure 23.4（頂點 1..5）：
  1→2 3    1→3 8    1→5 −4
  2→4 1    2→5 7
  3→2 4
  4→1 2    4→3 −5
  5→4 6

最後的距離矩陣：
      1   2   3   4   5
  1 [ 0   1  −3   2  −4 ]
  2 [ 3   0  −4   1  −1 ]
  3 [ 7   4   0   5   3 ]
  4 [ 2  −1  −5   0  −2 ]
  5 [ 8   5   1   6   0 ]
1 到 2 的最短路徑：1 → 5 → 4 → 3 → 2（−4 + 6 − 5 + 4 = 1）
```

### 還原路徑
另外維護 `next[i][j]` = 從 i 往 j 走，第一步要去哪。經過 k 比較短時，`next[i][j] = next[i][k]`。

### 負環
做完之後，如果某個 `d[i][i] < 0`（從自己出發繞一圈回來比 0 還少），就有負環。

### Warshall 遞移閉包 (transitive closure)
只問「走不走得到」，不問距離：把 `min` 和 `+` 換成 `OR` 和 `AND`。
```
r[i][j] = r[i][j] OR (r[i][k] AND r[k][j])
```
Thareja 13.8.5 的 Warshall 演算法就是這個；13.8.6 的 Modified Warshall 就是 Floyd-Warshall。

## 複雜度
| 方法 | 時間 | 適合 |
|---|---|---|
| **Floyd-Warshall** | **O(V³)** | V ≤ 400 左右、稠密圖、程式要短 |
| V 次 Dijkstra | O(V · E log V) | 稀疏圖、沒有負權重 |
| V 次 Bellman-Ford | O(V² E) | 很少用 |
| Johnson（CLRS 23.3） | O(V · E log V) | 稀疏圖、有負權重：先用 Bellman-Ford 重新加權成非負，再做 V 次 Dijkstra |

空間 O(V²)。

## 面試陷阱／常考點
1. **k 一定要在最外層。** 放到裡面的話，算 `d[i][j]` 時，`d[i][k]` 和 `d[k][j]` 可能還沒更新好，答案會錯。這是最常見的 bug。
2. **∞ + ∞ 會溢位**：用 `INF = 1 << 29` 這種「相加也不會溢位」的值，或先檢查是不是 ∞ 再加。
3. **初始化**：`d[i][i] = 0`；有重邊時取最小的；沒有邊放 ∞。
4. **無向圖**：`d[a][b]` 和 `d[b][a]` 都要設。
5. **「+、min」可以換成別的運算**：
   - 走不走得到：OR、AND → 遞移閉包（LeetCode 1462）
   - 比值：相乘 → Evaluate Division（399）
   - 路徑上的最大邊最小化（minimax）：`min(d[i][j], max(d[i][k], d[k][j]))`
6. **什麼時候選 Floyd**：題目要「很多對點的距離」而且 V 很小（≤ 100～400），程式只有 4 行，最不容易寫錯。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 1334 | [Find the City With the Smallest Number of Neighbors at a Threshold Distance](https://leetcode.com/problems/find-the-city-with-the-smallest-number-of-neighbors-at-a-threshold-distance/) | Medium | 標準 Floyd |
| 1462 | [Course Schedule IV](https://leetcode.com/problems/course-schedule-iv/) | Medium | 遞移閉包 |
| 399 | [Evaluate Division](https://leetcode.com/problems/evaluate-division/) | Medium | 權重相乘的 Floyd |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `floyd.h` / `.c` | Floyd-Warshall（含 `next` 路徑還原）、負環判斷、路徑還原、Warshall 遞移閉包 |
| `test_floyd.c` | CLRS 圖 23.4 的完整距離矩陣與路徑、1000 張隨機圖（用「位能」產生負權重但保證沒有負環）對照 V 次 Bellman-Ford、每條路徑的權重加總、遞移閉包、負環偵測 |

執行：`make test T=26-floyd-warshall`

## 出處
- **CLRS** 第 23 章 All-Pairs Shortest Paths · PDF p.838；23.2 The Floyd-Warshall algorithm（含遞移閉包）· PDF p.849；23.3 Johnson's algorithm
- **Thareja** 13.3.2 Transitive Closure of a Directed Graph p.386、13.8.5 Warshall's Algorithm p.414、13.8.6 Modified Warshall's Algorithm p.417
