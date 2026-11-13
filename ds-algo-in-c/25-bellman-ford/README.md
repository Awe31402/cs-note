# 25 · Bellman-Ford 最短路徑（可以有負權重）

## 一句話
**Bellman-Ford** 也是找「單一起點到所有頂點」的最短路徑，但**允許負權重**，而且能**偵測負環**。做法很暴力：**把每一條邊都鬆弛 V − 1 輪**。比 Dijkstra 慢，但比較通用。

## 為什麼 V − 1 輪就夠
最短路徑不會重複經過同一個頂點（否則就有環，拿掉環只會更短或一樣），所以最多 V − 1 條邊。
- 第 1 輪結束：「最多用 1 條邊」的最短路徑都正確了。
- 第 2 輪結束：「最多用 2 條邊」的都正確了。
- ……
- 第 V − 1 輪結束：全部正確。

### 負環 (negative cycle)
一個總權重 < 0 的環：繞一圈就更短，可以一直繞下去，最短路徑就**沒有定義**（−∞）。
**偵測方法**：做完 V − 1 輪之後，再掃一次所有的邊。**如果還能鬆弛，就一定有負環。**

## 圖示
```
CLRS Figure 22.4（s t x y z）：
  s→t 6    s→y 7
  t→x 5    t→y 8    t→z −4
  x→t −2
  y→x −3   y→z 9
  z→s 2    z→x 7
最短距離：s=0  t=2  x=4  y=7  z=−2
（s → y → x → t → z：7 − 3 − 2 − 4 = −2）
```

### 找出負環本身
從任一頂點開始都讓 dist = 0（等於加一個虛擬起點，連到每個頂點、權重 0），做 V 輪。第 V 輪還被更新的頂點 v，**可能只是被負環影響、不一定在環上**，所以先沿 `parent` 往回走 V 步，一定會走進環裡，再繞一圈就是整個環。

### DAG 上的最短路徑（CLRS 22.2）
如果圖是**有向無環圖 (DAG)**，依照**拓撲順序**對每個頂點的出邊鬆弛**一次**就好：輪到 u 的時候，所有能走到 u 的頂點都已經處理完了。O(V + E)，一樣可以有負權重。

## 複雜度
| 演算法 | 時間 | 負權重 | 負環 |
|---|---|---|---|
| BFS | O(V + E) | 權重都一樣才能用 | — |
| Dijkstra | O((V + E) log V) | ❌ | ❌ |
| **Bellman-Ford** | **O(VE)** | ✅ | ✅ 可偵測 |
| DAG 最短路徑 | O(V + E) | ✅ | 不會有環 |
| Floyd-Warshall（第 26 主題） | O(V³)，所有點對 | ✅ | ✅ 可偵測 |

## 面試陷阱／常考點
1. **∞ + 負數 不能當成更短**：`dist[u]` 還是 ∞ 時要跳過，不然 ∞ − 5 會被當成一個有限的距離。
2. **提早結束**：某一輪完全沒有更新，之後也不會更新了，可以直接停。實務上通常很快就停了。
3. **「最多 k 條邊」的最短路徑**（LeetCode 787）：只做 k 輪，而且**每一輪都要從上一輪的結果延伸**（先複製一份 dist），否則同一輪裡會連續走好幾條邊。
4. **偵測到的負環要「從起點走得到」才會影響答案**：走不到的負環，對從 s 出發的最短路徑沒有影響。
5. **SPFA (Shortest Path Faster Algorithm)**：用 queue 只鬆弛「上一輪有變動的頂點」的出邊。平均很快，但最壞還是 O(VE)。
6. **差分約束 (difference constraints)**：一組 `x_j − x_i ≤ c` 的不等式，可以轉成圖，用 Bellman-Ford 求解（CLRS 22.4）。
7. **應用**：距離向量路由協定 (RIP)、匯率套利偵測（取 −log 之後，有負環 = 有套利機會）。

## 練習題（LeetCode）
LeetCode 上真正需要 Bellman-Ford 的題目很少（大部分最短路徑題都沒有負權重，用 Dijkstra 就好），所以這裡只放 2 題：
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 743 | [Network Delay Time](https://leetcode.com/problems/network-delay-time/) | Medium | Bellman-Ford 版本，對照 24 主題的 Dijkstra |
| 787 | [Cheapest Flights Within K Stops](https://leetcode.com/problems/cheapest-flights-within-k-stops/) | Medium | 限制邊數：只做 k+1 輪 |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `bellman_ford.h` / `.c` | Bellman-Ford（提早結束 + 負環偵測）、找出一個負環、DAG 最短路徑（拓撲排序 + 一輪鬆弛） |
| `test_bellman_ford.c` | CLRS 圖 22.4、圖 22.5（DAG）、3000 張隨機圖（含負權重與負環）：距離對照 Floyd-Warshall、「s 走得到負環」的判斷、找到的環每步都是真的邊而且總和 < 0 |

執行：`make test T=25-bellman-ford`

## 出處
- **CLRS** 22.1 The Bellman-Ford algorithm · PDF p.794；22.2 Single-source shortest paths in DAGs · PDF p.800；22.4 Difference constraints · PDF p.812
- **Thareja**：沒有專門章節（13.8 只講 Dijkstra 和 Warshall）
