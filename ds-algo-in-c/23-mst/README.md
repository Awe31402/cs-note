# 23 · MST 最小生成樹（Kruskal / Prim）

## 一句話
**最小生成樹 (Minimum Spanning Tree, MST)**：在一張連通、有權重的**無向**圖裡，挑出剛好 n − 1 條邊，把所有頂點連起來，而且**總權重最小**。例如：用最少的電纜把所有城市接上網路。

## 關鍵性質：切割性質 (cut property, CLRS 21.1)
把頂點隨便切成兩群，**跨過這一刀的邊裡，最輕的那條一定可以放進某棵 MST**（這種邊叫「安全邊 light edge」）。
Kruskal 和 Prim 都是貪心法 (greedy)，每一步都選一條安全邊。

## Kruskal：由小到大挑邊
```
1. 所有邊依權重排序
2. 由小到大看每條邊：兩端「還沒連通」就選它（用 Union-Find 判斷），否則跳過（選了會形成環）
3. 選滿 n − 1 條就結束
```
```
CLRS Figure 21.1（總權重 37）：
權重 1 (h,g) ✅  2 (i,c) ✅  2 (g,f) ✅  4 (a,b) ✅  4 (c,f) ✅
     6 (i,g) ❌ 已連通  7 (c,d) ✅  7 (h,i) ❌  8 (a,h) ✅  8 (b,c) ❌
     9 (d,e) ✅ → 選滿 8 條
```

## Prim：從一個點長出一棵樹
```
1. 從任一頂點開始，它自己就是一棵樹
2. 每次選「一端在樹裡、另一端在樹外」的邊中，最輕的那條，把樹外那個頂點加進來
3. 所有頂點都加進來就結束
```
兩種寫法：
- **heap 版**：候選邊放在 min-heap 裡，O(E log E)。適合稀疏圖。
- **陣列版**：`key[v]` = v 連到樹的最輕邊，每輪線性找最小的 key，O(V²)。適合**稠密圖**，例如完全圖（LeetCode 1584）。

## 複雜度
| 演算法 | 時間 | 適合 |
|---|---|---|
| Kruskal | O(E log E)，主要是排序 | 稀疏圖、邊已經排好序、要逐步加邊的題目 |
| Prim（二元 heap） | O(E log V) | 稀疏圖 |
| Prim（陣列） | **O(V²)** | 稠密圖（E ≈ V²） |
| Prim（Fibonacci heap） | O(E + V log V) | 理論上最好，實務很少用 |

## 面試陷阱／常考點
1. **MST 只適用於無向圖**。有向圖的版本叫「最小樹形圖」(arborescence)，要用完全不同的演算法。
2. **MST 不一定唯一**：權重有重複時可能有好幾棵，但**總權重一定唯一**。權重全部不同時，MST 才唯一。
3. **MST ≠ 最短路徑樹**：MST 讓「總權重」最小，不保證兩點之間的路徑最短。這兩個常被搞混。
4. **Kruskal = 排序 + Union-Find**。面試最常寫的就是這個版本。
5. **完全圖（每兩點都有邊）用陣列版 Prim**，不用把 V² 條邊都存下來再排序（1584）。
6. **不連通的圖**沒有生成樹：選不滿 n − 1 條邊。本專案回傳 −1。
7. **「只用權重 < limit 的邊，兩點連不連通」**：邊和查詢都排序後，一邊加邊一邊回答，這是 Kruskal 的延伸（1697）。
8. **關鍵邊 / 偽關鍵邊**：拿掉一條邊、或強迫先選一條邊，再重算 MST 比較（1489）。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 1584 | [Min Cost to Connect All Points](https://leetcode.com/problems/min-cost-to-connect-all-points/) | Medium | 完全圖用 O(V²) Prim |
| 1697 | [Checking Existence of Edge Length Limited Paths](https://leetcode.com/problems/checking-existence-of-edge-length-limited-paths/) | Hard | 離線 + Kruskal 的想法 |
| 1489 | [Find Critical and Pseudo-Critical Edges in Minimum Spanning Tree](https://leetcode.com/problems/find-critical-and-pseudo-critical-edges-in-minimum-spanning-tree/) | Hard | 拿掉／強迫選一條邊 |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `mst.h` / `.c` | Kruskal（排序 + Union-Find）、Prim heap 版（lazy，同一頂點可能重複進 heap）、Prim 陣列版（O(V²)） |
| `test_mst.c` | CLRS 圖 21.1（總權重 37）、2000 張隨機小圖：三種演算法都要等於「暴力枚舉所有 n−1 條邊的組合」，並檢查選出的邊真的是一棵生成樹；不連通的圖 |

執行：`make test T=23-mst`

## 出處
- **CLRS** 第 21 章 Minimum Spanning Trees · PDF p.762（21.1 切割性質與安全邊、21.2 Kruskal 與 Prim p.769）
- **Thareja** 13.8.1 Minimum Spanning Trees p.405、13.8.2 Prim's Algorithm p.407、13.8.3 Kruskal's Algorithm p.409
