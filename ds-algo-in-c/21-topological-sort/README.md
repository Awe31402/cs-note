# 21 · Topological Sort 拓撲排序

## 一句話
**拓撲排序 (topological sort)** 把**有向無環圖 (DAG, Directed Acyclic Graph)** 的頂點排成一列，讓每條邊 u → v 的 u 都排在 v 前面。就像排「修課順序」或「編譯順序」：先做完前置條件，才能做後面的事。

**有環就排不出來**：A 要先於 B、B 又要先於 A，不可能。

## 圖示
```
CLRS Figure 20.7：穿衣服
內褲 → 褲子 → 皮帶 → 外套          襪子 → 鞋子
  ↘      ↘                         手錶（沒有依賴）
    鞋子   鞋子
襯衫 → 皮帶；襯衫 → 領帶 → 外套

一種合法順序：襪子 內褲 褲子 鞋子 手錶 襯衫 皮帶 領帶 外套
（答案通常不唯一）
```

## 兩種做法

### 1. Kahn 演算法（BFS 風格）
```
1. 算每個頂點的入度 (in-degree)，也就是「還有幾個前置條件沒完成」
2. 入度 0 的全部放進 queue
3. 拿出 u，放到答案後面；u 的每個鄰居 v 入度 −1，變成 0 就放進 queue
4. 排出來的數量 < n → 有環（環上的頂點入度永遠不會變 0）
```

### 2. DFS 版（CLRS 20.4）
```
對每個頂點做 DFS，頂點「完成」(finish) 時，把它放到答案的**最前面**
→ 完成時間越晚，排越前面
```
為什麼對：如果有邊 u → v，那 v 一定比 u 先完成（v 是 u 的子孫，或 v 早就走完了）。
**三色標記**：白 = 沒走過、灰 = 在目前的遞迴路徑上、黑 = 完成。**走到灰色的頂點 = 後向邊 = 有環**。

| | Kahn | DFS |
|---|---|---|
| 判斷有環 | 排不滿 n 個 | 遇到灰色頂點 |
| 找出環本身 | 不方便 | 沿 parent 往回走就是環（`find_cycle`） |
| 字典順序最小的答案 | 把 queue 換成 min-heap | 不方便 |
| 分層（「最少要幾學期」） | 很自然：一層一層 BFS | 不方便 |

## 複雜度
| 操作 | 時間 | 空間 |
|---|---|---|
| Kahn | O(V + E) | O(V) |
| DFS 版 | O(V + E) | O(V)，遞迴深度 |
| 找環 | O(V + E) | O(V) |

## 面試陷阱／常考點
1. **邊的方向**：LeetCode 207 的 `[a, b]` 是「修 a 之前要先修 b」，所以邊是 **b → a**。方向搞反的話，排出來的順序剛好反過來。
2. **無向圖沒有拓撲排序**。無向圖判斷有沒有環，用 DFS 時忽略「走回父節點」那條邊，或用 Union-Find（第 22 主題）。
3. **DFS 判斷有環要三色，不能只用兩色（走過／沒走過）**：走到「黑色」（已完成）的頂點是正常的，例如兩條路匯合到同一點；只有走到「灰色」才是環。
4. **答案不唯一**，所以測試時要檢查「每條邊都符合順序」，而不是跟某個固定答案比。
5. **應用**：修課順序、編譯依賴（make）、套件安裝順序、試算表儲存格計算、DAG 上的最短路徑（CLRS 22.2）。
6. **找「最終安全節點」**（LeetCode 802）：三色 DFS 的變化，也可以把邊反過來再做 Kahn。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 207 | [Course Schedule](https://leetcode.com/problems/course-schedule/) | Medium | 判斷有沒有環 |
| 210 | [Course Schedule II](https://leetcode.com/problems/course-schedule-ii/) | Medium | 輸出拓撲順序 |
| 802 | [Find Eventual Safe States](https://leetcode.com/problems/find-eventual-safe-states/) | Medium | 三色 DFS |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `topo.h` / `.c` | CSR 格式的有向圖、Kahn 演算法、DFS 版拓撲排序（三色）、找出一個環 |
| `test_topo.c` | CLRS 圖 20.7 穿衣服的例子、1000 張隨機圖（一半保證是 DAG）：兩種排序都要符合每條邊、有沒有環要跟遞移閉包的暴力法一致、找到的環每一步都是真的邊 |

執行：`make test T=21-topological-sort`

## 出處
- **CLRS** 20.4 Topological sort · PDF p.746
- **Thareja** 13.7 Topological Sorting · p.400
