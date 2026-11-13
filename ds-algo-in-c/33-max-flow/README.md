# 33 · Max Flow 最大流（Edmonds-Karp）

## 一句話
**最大流 (maximum flow)**：水管網路裡每條管子有**容量 (capacity)**，從**源點 s** 灌水、從**匯點 t** 流出，最多能流多少？
**Ford-Fulkerson 方法**：只要還找得到一條能再多送一點的路（**增廣路徑 augmenting path**），就沿著它送。**Edmonds-Karp** 就是「每次用 BFS 找邊數最少的增廣路徑」，保證 O(V E²)。

## 流的規則（CLRS 24.1）
1. **容量限制**：每條邊的流量 ≤ 容量。
2. **流量守恆**：除了 s 和 t，每個頂點「流進 = 流出」。

## 剩餘網路 (residual network) 與反向邊
這是最大流最關鍵的概念。
```
邊 u → v 容量 10，目前流 7：
  剩餘網路裡：u → v 還能送 3
              v → u 能「退回」7   ← 反向邊
```
**為什麼要反向邊**：貪心地先選了一條路，可能擋住了更好的安排。反向邊讓演算法可以「後悔」，把之前送的流量改道。
```
s → a → t、s → b → t、a → b（容量都是 1）
先走 s→a→b→t，a→t 和 s→b 就沒用到，只送了 1
有反向邊 b→a：再找到 s→b→a→t（走 b→a 等於取消 a→b 的流量）→ 總共 2 ✅
```

## Edmonds-Karp
```
重複：
  1. 在剩餘網路用 BFS 找 s → t 的路（只走剩餘容量 > 0 的邊）
  2. 找不到 → 結束，目前的流就是最大流
  3. 瓶頸 = 路上最小的剩餘容量
  4. 路上每條邊：正向 −= 瓶頸、反向 += 瓶頸
```
用 BFS（邊數最少的路）保證最多增廣 O(VE) 次；如果用 DFS 隨便找，容量很大時可能要很多次。

```
CLRS Figure 24.6：
  s→v1 16   s→v2 13
  v2→v1 4
  v1→v3 12  v3→v2 9   v2→v4 14
  v4→v3 7   v3→t 20   v4→t 4
最大流 = 23
```

## 最大流最小割定理 (max-flow min-cut theorem, CLRS 24.2)
**割 (cut)**：把頂點分成兩群 S（含 s）和 T（含 t），割的容量 = 從 S 到 T 的邊的容量總和。
**最大流 = 最小割的容量。**
做完最大流之後，**在剩餘網路中從 s 走得到的頂點**就是最小割的 S 那一側。
本專案的測試：暴力枚舉所有的割，最小的那個一定等於 Edmonds-Karp 算出來的最大流。

## 二分圖最大匹配 (CLRS 24.3、25.1)
左邊一群、右邊一群，每條邊連一個左、一個右。最多能配成幾對（每個點最多配一次）？
**轉成最大流**：s → 每個左邊點（容量 1）、左 → 右（容量 1）、每個右邊點 → t（容量 1）。最大流 = 最大匹配。
**直接做增廣（Kuhn 演算法）**：
```
左邊 u 想配 v：
  v 還沒人要 → 配給 u
  v 已經配給 u' → 叫 u' 去找別人；u' 找得到，v 就讓給 u
```
O(VE)，程式很短。更快的是 Hopcroft-Karp，O(E √V)。

### König 定理（二分圖限定）
**最大匹配 = 最小點覆蓋**；**最大獨立集 = 點數 − 最大匹配**。
一般圖的最大獨立集是 NP-hard（第 34 主題），但二分圖可以用匹配在多項式時間解（LeetCode 1349）。

### 匈牙利演算法 (Hungarian algorithm, CLRS 25.3)
**指派問題 (assignment problem)**：n 個人、n 件工作，每個配對有一個分數，找總分最大（或成本最小）的一對一配對。
用「頂點位能 (potential)」維護一個可行的對偶解，只沿著「緊的邊」(reduced cost = 0) 找增廣路徑。O(n³)（LeetCode 1947）。

## 複雜度
| 演算法 | 時間 |
|---|---|
| Ford-Fulkerson（容量是整數） | O(E · f*)，f* = 最大流的值 |
| **Edmonds-Karp** | **O(V E²)** |
| Dinic | O(V² E)，實務上很快 |
| 二分匹配（Kuhn） | O(V E) |
| Hopcroft-Karp | O(E √V) |
| 匈牙利演算法 | O(n³) |

## 面試陷阱／常考點
1. **一定要有反向邊**，否則貪心可能卡在不是最大的解。
2. **最大流 = 最小割**，常用來證明或轉換問題（「最少要切斷幾條路」）。
3. **很多問題可以轉成最大流**：二分匹配、邊互斥的路徑數、專案選擇 (project selection)、圖片分割。
4. **二分圖判斷**（LeetCode 785）：BFS 兩色著色，⇔ 沒有奇數環。
5. **一般圖的最大獨立集是 NP-hard**，但二分圖上可以用 König 定理轉成匹配。
6. **指派問題**不要用暴力 n!，要用匈牙利演算法或最小成本流。
7. 面試很少要你從頭寫 Edmonds-Karp，但**把問題轉成流或匹配**的能力很重要。

## 練習題（LeetCode）
LeetCode 很少直接考最大流。以下是**二分圖與匹配**的題目：
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 785 | [Is Graph Bipartite?](https://leetcode.com/problems/is-graph-bipartite/) | Medium | 二分圖判斷 |
| 1947 | [Maximum Compatibility Score Sum](https://leetcode.com/problems/maximum-compatibility-score-sum/) | Medium | 指派問題：匈牙利演算法 |
| 1349 | [Maximum Students Taking Exam](https://leetcode.com/problems/maximum-students-taking-exam/) | Hard | 二分圖最大獨立集 = 點數 − 最大匹配 |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `maxflow.h` / `.c` | Edmonds-Karp（鄰接矩陣、剩餘網路、每條邊的流量）、最小割的 s 側、二分圖最大匹配（Kuhn 增廣） |
| `test_maxflow.c` | CLRS 圖 24.6（最大流 23 = 最小割 23）、1000 張隨機網路：流量守恆與容量限制、暴力枚舉所有割的最小值 = 最大流；1000 張隨機二分圖用 Hall 定理暴力驗證匹配數 |

執行：`make test T=33-max-flow`

## 出處
- **CLRS** 第 24 章 Maximum Flow · PDF p.869（24.1 Flow networks p.870、24.2 Ford-Fulkerson 與最大流最小割 p.876、Edmonds-Karp p.892、24.3 Maximum bipartite matching p.897）
- **CLRS** 第 25 章 Matchings in Bipartite Graphs · PDF p.911（25.1 p.912、25.3 Hungarian algorithm p.936）
- **Thareja**：沒有
