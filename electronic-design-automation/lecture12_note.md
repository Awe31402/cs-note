# 設計自動化（一）第十二支影片完整筆記：Routing（二）——global routing、G-cell、Steiner tree heuristic、congestion 與 pin access（ch7 routing, part 2）

> **課程**：Electronic Design Automation (1)（清大 CSR 5302）— 第11_2講｜ch7.routing（播放清單第 12 支）
> **講者**：李炯霆（JT Li），清大半導體學院，2026 春，NTHU OCW 錄影
> **來源**：YouTube 錄影（1:25:03，無字幕，whisper 轉錄）＋ 投影片截圖（Unit 5D 後半，部分引用 Kahng, Lienig, Markov, Hu《VLSI Physical Design: From Graph Partitioning to Timing Closure》Ch5 Global Routing 的投影片）＋ 課本 Wang, Chang, Cheng（eds.）*Electronic Design Automation* Ch12 §12.4–12.6（p.697–738）
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、投影片和課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿辨識不清、或影片／投影片／課本說法不一致之處
> - `[h:mm:ss]`：影片時間點
>
> ⚠️ 播放清單標題寫「第11_2講」，是第 11 講的下半；本筆記檔名依播放清單順序為第 12 支。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Capacity 與 G-cell 的建模](#1-capacity-與-g-cell-的建模)
- [2. Grid graph 與 global routing 的圖解](#2-grid-graph-與-global-routing-的圖解)
- [3. Global routing 問題的定義](#3-global-routing-問題的定義)
- [4. 高階演算法流程](#4-高階演算法流程)
- [5. Via pillar、NDR 與 congestion](#5-via-pillarndr-與-congestion)
- [6. A* search](#6-a-search)
- [7. Kahng 等人的 Ch5：名詞與 routing 的分類](#7-kahng-等人的-ch5名詞與-routing-的分類)
- [8. Channel、capacity、pitch](#8-channelcapacitypitch)
- [9. Switchbox、T-junction、connectivity graph](#9-switchboxt-junctionconnectivity-graph)
- [10. Global routing flow 與 ICCAD 2024 contest](#10-global-routing-flow-與-iccad-2024-contest)
- [11. Routing tree 與 MRST](#11-routing-tree-與-mrst)
- [12. MRST 的 heuristic：L-shape 與 iterated 1-Steiner](#12-mrst-的-heuristicl-shape-與-iterated-1-steiner)
- [13. Congestion map 與 congestion fix](#13-congestion-map-與-congestion-fix)
- [14. 學術界的 global router](#14-學術界的-global-router)
- [15. Detailed routing：pin access、rip-up-and-reroute](#15-detailed-routingpin-accessrip-up-and-reroute)
- [16. 【實測】](#16-實測)
- [17. 一頁速查表](#17-一頁速查表)
- [18. 與後續講次的連結](#18-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
G-cell（tile）：capacity = 邊界上可用的 track 數（看 pin、blockage、via）
global routing：每條 net 找一棵 Steiner tree，讓每個邊界 demand ≤ capacity，總線長最小
  流程：建 G-cell → 算 capacity → 每條 net 先走最短（Steiner tree、A*）→ 找 overflow
        → rip-up-and-reroute 把 congestion 攤平 → layer assignment
  MRST：MST/MRST ≤ 3/2（Hwang）；L-shape + DP（約少 8%）；iterated 1-Steiner
  工具：congestion map；NTHU-Route 2.0（history cost）、CUGR、OGRE
detailed routing：照 GR 的 guide → track assignment → pin access → router 內建 DRC → rip-up-and-reroute（Dr. CU）
```

---

## 1. Capacity 與 G-cell 的建模

[0:00]–[7:48]　投影片 5.2 Terminology（Capacity）、How To Model In A GCell、Gcells (Tiles) with standard cells、Most Of The SOC Chips

- **Capacity**：可用的 routing track（或 column）數。圖：水平 routing channel，高 h、track pitch $d_{pitch}$。
  - 【口頭】老師在晶片上畫的虛線就是 G-cell 的格線。
- **怎麼建模一個 G-cell**：
  - pin 附近有多少可用的 track？blockage 多大？
  - 水平 track 的 capacity 左右兩邊可以不同（full、partial）。
  - **G-cell 裡的 pin 數會影響結果**。
  - **G-cell 裡的 via 數也會影響結果**。
- **Standard cell 的 G-cell（tile）**：Metal1（standard cell 本身）、Metal2（cell 的 port）、Metal3、Metal4…每層都切成 tile。
- **大部分 SoC 晶片**：由很多 standard cell 和 macro 組成，用前一張投影片那樣的 global routing cell。

---

## 2. Grid graph 與 global routing 的圖解

[7:48]–[11:24]　投影片 5.4 Grid graph model、(My Preference) Pictorial Explanation Of Global Routing

- **Grid graph model**：$g_{grid} = (V, E)$，節點是 routing grid cell（**gcell**），邊是相鄰 grid cell 的連接 $(v_i, v_j)$。圖：5 × 5 的格子 1–25 變成 5 × 5 的 grid graph。
  - 投影片：**G-cell 的建模需要調校（tuning）。**
- **老師偏好的圖解**：三條 net n1、n2、n3 在 G-cell 格子上走：
  - global route 要為所有 net 找出 loose routing，使得：**每條 net 都接上**，而且**沒有任何邊界超過 capacity**。
  - **能不能把邊界的 congestion 攤平（smooth out）？**——很重要。
  - 要考慮 net 的 timing 或 criticality。
  - **GR 用很多 memory（比 DR 多）**。
  - 投影片問：**一條 net 要怎麼接？**

---

## 3. Global routing 問題的定義

[11:24]–[15:45]　投影片 Global-Routing Problem；課本 §12.4（p.697–698）

給 netlist $N = \{N_1, N_2, \dots, N_n\}$、routing graph $G = (V, E)$，**為每條 net $N_i$ 找一棵 Steiner tree $T_i$**，使得 $U(e_j) \le c(e_j),\ \forall e_j \in E$，而且 $\sum_{i=1}^n L(T_i)$ 最小。
- $c(e_j)$：邊 $e_j$ 的 **capacity**
- $x_{ij} = 1$ 若 $e_j$ 在 $T_i$ 裡，否則 0
- $U(e_j) = \sum_{i=1}^n x_{ij}$：穿過 $e_j$ 對應 channel 的線數（**demand**）
- $L(T_i)$：Steiner tree $T_i$ 的總線長
- **高效能設計**：改成最小化最大線長 $\max_{i=1}^n L(T_i)$（或 $T_i$ 中兩點間的最長路徑）。

---

## 4. 高階演算法流程

[15:45]–[17:27]　投影片 An Algorithm At The High Level

1. **決定 global routing 的格子**——可以不均勻；G-cell 大小怎麼定？
2. **算 G-cell 邊界的 track capacity**——要把線的穿越、via、G-cell 中心附近的 pin 都建模進去。
3. **critical net 先 global route**——讓它們線長**比較短**；可以用 maze search、rectilinear Steiner／spanning tree…。
4. **route 其他 signal net**——經過擁擠區域的 cost 比較高。
5. 在 G-cell 邊界**保留一些 slack**。
6. 所有 net 都 route 完之後，找出有 overflow 的邊界。
7. **決定怎麼 rip-up-and-reroute 把 congestion 攤平。**
8. 最好也考慮 **layer assignment**。
9. 有時要用 **hierarchical** 的做法。

---

## 5. Via pillar、NDR 與 congestion

[17:27]–[21:06]　投影片 Via Pillar Insertion、AutoNDR Routing、Global router vs Detailed router、Placement → GR → Congestion Map → DR

- **Via modeling 和 NDR 會影響 global routing**：
  - **Via pillar insertion**：在 timing critical net 的 source 端加**平行的 via**，降低 R。
  - **AutoNDR routing**：把 timing critical net 的金屬加寬、間距加大（**non-default routing，NDR**），降低 RC。
- **Global router 告訴我們只在這裡找 detailed path；detailed router 告訴我們這個區域裡真正的最終路徑。**
- 每個 GRC（global routing cell）邊界：穿過的 net 數／可用 track 數 = 該方向的 **congestion**（例如水平 40/35 就是 overflow）。
  - 投影片：**overflow 太多，DR 就不好做。**
- 流程圖：placement → wire tracks → global routing → **congestion map**（例如 16/16、17/16）→ detailed routing。

---

## 6. A* search

[21:06]–[24:21]　投影片 5.6.4 Finding Shortest Paths with A\* Search；課本 §12.3.2

- **A\* search** 和 Dijkstra 類似，但 cost function 多加了**從目前節點到 target 的估計距離**。
- 只展開最有希望的節點；這種 **best-first** 策略排除了大部分的解空間。
- 例：同一張有障礙物的格子，**Dijkstra 展開 31 個節點、A\* 只展開 6 個**。
  - 【口頭】cost 是「已走的」加上「到 target 的估計」，離開 target 的方向 cost 會增加，所以盡量往 T 走。
- **Bidirectional A\***：從 source 和 target 兩邊同時展開，直到兩個展開區域相交；考慮的節點數可以再減少。

---

## 7. Kahng 等人的 Ch5：名詞與 routing 的分類

[24:21]–[30:03]　投影片 Chapter 5 – Global Routing（taught at UCSD）、目錄、5.3 Optimization Goals、5.1 Introduction

- 老師引用 UCSD 教的 *VLSI Physical Design: From Graph Partitioning to Timing Closure*（Kahng, Lienig, Markov, Hu，2011）Ch5 的投影片。
- 目錄：5.1 Introduction；5.2 Terminology；5.3 Optimization Goals；5.4 Representations of Routing Regions；5.5 Global Routing Flow；5.6 Single-Net Routing（rectilinear routing、connectivity graph、Dijkstra、A\*）；5.7 Full-Netlist Routing（ILP、**rip-up and reroute（RRR）**）；5.8 Modern Global Routing（**pattern routing**、**negotiated-congestion routing**）。
- **Optimization goals**：判斷一個 placement **是否 routable**；為所有 net 在可用的 routing region 裡決定**粗略的 routing**；目標：最小化總線長、減少 critical net 的 delay。
- 流程（5.1）：physical design = partitioning → chip planning → placement → clock tree synthesis → **signal routing** → timing closure。
- **Introduction**：給 placement、netlist、technology 資訊，決定必要的接線（net topology、具體的 routing segment），同時遵守限制（design rule、routing resource capacity），最佳化目標（最小化總線長、最大化 timing slack）。
- **名詞**：
  - **Net**：電位相同的兩個以上 pin 的集合。
  - **Netlist**：所有 net 的集合。
  - **Congestion**：幾條 net 的最短路線經過同一批 track、彼此衝突的地方。
  - **Fixed-die routing**：晶片外框和 routing 資源都固定。
  - **Variable-die routing**：需要時可以加新的 routing track。
- **一般 routing 問題**：netlist $N_1 = \{C_4, D_6, B_3\}$、$N_2 = \{D_4, B_4, C_1, A_4\}$、$N_3 = \{C_2, D_5\}$、$N_4 = \{B_1, A_1, C_3\}$，加上 technology 資訊（design rule），在 A、B、C、D 四個 block 的 pin 之間接線。
- **Routing 的分類**：
  - Multi-stage routing of signal nets：**global routing**（routing region 的粗粒度分配，Ch5）、**detailed routing**（track 的細粒度分配，Ch6）、**timing-driven routing**（net topology 最佳化、把資源分給 critical net，Ch8）
  - **Large single-net routing**：power（VDD）和 ground（GND）routing（Ch3）
  - **Geometric techniques**：non-Manhattan、clock routing（Ch7）
- Global routing（在 channel connectivity graph 上）→ detailed routing（水平段、垂直段、via）。投影片註：**channel connectivity graph——但如果可以從 macro 上面 route 過去呢……**

---

## 8. Channel、capacity、pitch

[30:03]–[35:36]　投影片 5.2 Channel、Capacity、Routing Models

- **Channel**：兩個相對的邊有 pin 的長方形 routing 區域（standard cell layout、two-layer routing）。
- **Capacity**：
  - **單層** routing：capacity = channel 高度 h ÷ pitch $d_{pitch}$。
  - **多層** routing：capacity σ = 各層 capacity 的總和：
$$\sigma(Layers) = \sum_{layer\in Layers}\left\lfloor\frac{h}{d_{pitch}(layer)}\right\rfloor$$
  - 【口頭】其實不是所有 layer 都加，應該只加水平方向 routing 的 layer。有些 global routing 只考慮 **2D**（把所有層壓成一層），比較快——**給大家的 project（2D global router）就是這樣**：雖然有三層，請大家用 2D 的觀念來寫。
- **Routing model（grid-based）**：
  - 在 routing 區域疊一個格子；線沿著格線走。
  - **Pitch**：兩條格線的距離，通常是線寬 + 間距；**如果 contact 比較大，pitch 可能就要更大**。
  - 例：m1 線寬 3λ、via1 4λ：
    - (b) via-to-via pitch 7λ = 2 × (contact 寬的一半) + spacing
    - (c) via-to-line pitch 6.5λ = line-to-line + ½（contact 寬 − 線寬）
    - (d) line-to-line pitch 6λ = 2 × (線寬一半) + spacing

---

## 9. Switchbox、T-junction、connectivity graph

[35:36]–[42:42]　投影片 5.2 Switchbox、T-junction、5.4 Switchbox／Channel connectivity graph、5.6.2

- **Switchbox**（two-layer macro cell layout）：水平和垂直 channel 的交叉處。**水平 channel 要在垂直 channel route 完之後才 route**。
- **T-junction**：一條 channel 接到另一條 channel 側邊的地方。
- **Switchbox connectivity graph**：G = (V, E)，節點是 **switchbox**；兩個 switchbox 在同一條 channel 的對邊時，之間有一條邊（例子 14 個節點）。
- **Channel connectivity graph**：G = (V, E)，節點是 **channel**，邊表示 channel 相鄰（例子 9 個節點）。投影片註：**不允許 over-the-macro routing**。
- 例：N1、N2、N3 在 channel graph 上的 global routing 結果。

---

## 10. Global routing flow 與 ICCAD 2024 contest

[42:42]–[45:27]　投影片 Global Routing Flow、ICCAD 2024 Contest

1. 建 G-cell（疊上格子）。
2. 取得 G-cell 裡的 routing track、blockage、pin → **設定每個 G-cell 邊界的 capacity**。
3. 讓每條 net 走最短路徑（Steiner tree…）。
4. 有些 G-cell 邊界會 **overflow**（demand > capacity）。
5. 在 GR graph 裡 **reroute 解決 overflow**（反覆迭代；有些 slack 可以在 G-cell 邊界調整）。
6. critical net 要線長比較短。
- 投影片問：
  - **如果 global routing 結果沒有任何 overflow，detailed router 一定 route 得完嗎？**
  - **如果有一些 overflow，DR 還能 route 完嗎？**
  - （兩者答案都不一定：GR 的模型很粗，沒考慮 pin access、DRC 細節；反過來，少量 overflow 有時 DR 可以在局部消化。）
- **ICCAD 2024 Contest Problem D：Chip Level Global Router**（聯發科 MediaTek 出題）。
  - 【口頭】有時候也叫 fast version 的 global route。有興趣可以參考，也可能是你們的**碩士論文題目**。

---

## 11. Routing tree 與 MRST

[45:27]–[54:09]　投影片 The Routing-Tree Problem、A Simple Performance Bound、Theoretic Results for the MRST Problem；課本 §12.4.3（p.700–704）

- **Routing-tree problem**：給一條 net 的一組 pin，用一棵「routing tree」把它們接起來（gate array、standard cell、building block 三種情境）。
- **MRST**：給平面上 n 個點，找連接它們、由直線段組成、總長最小的 tree。
- $MRST(P) = MST(P \cup S)$，P 是原本的點、S 是 Steiner point。（同[第 5 支](lecture05_note.md) §1。）

**簡單的 performance bound**：$\frac{Cost(MST(P))}{Cost(MRST(P))} \le 2$。
- 證明：給任意 MRST T（含 Steiner point），在 P 上建一棵 spanning tree T'：
  1. 任選 T 的一個點當 root。
  2. 對 rooted tree T 做 **depth-first traversal**。
  3. 依走訪順序建 T'。
- 每條邊都被走過**兩次** → $Cost(T') \le 2\,Cost(T)$，而 MST 不會比 T' 長。

**理論結果**：
- **Hanan's theorem**：存在一棵 MRST，所有 Steiner point 都選自過 P 的點所畫水平、垂直線的**交點**（Hanan, "On Steiner's problem with rectilinear distance," *SIAM J. Applied Math.*, 1966）。
- **Hwang's theorem**：對任何點集 P，$\frac{Cost(MST(P))}{Cost(MRST(P))} \le \frac32$（Hwang, "On Steiner minimal tree with rectilinear distance," *SIAM J. Applied Math.*, 1976）。
- 目前最好的近似演算法：performance bound **61/48（≈ 1.27）**，Foessmeier et al.。
- 圖：Hanan grid；以及一排「梳子」形的點，MRST 和 MST 的比值趨近 3/2。

---

## 12. MRST 的 heuristic：L-shape 與 iterated 1-Steiner

[54:09]–[1:04:27]　投影片 Coping with the MRST Problem、Iterated 1-Steiner Heuristic for MRST、Figure 9.20、Algorithm Iterated_1-Steiner、j4.pdf、So Far We Covered

**Ho, Vijayan, Wong, "New algorithms for the rectilinear Steiner problem"**：
1. 從 MST 建 MRST。
2. 每條邊是直線或 **L 形**。
3. 用 **dynamic programming** 最大化重疊。
- 結果大約比 Cost(MST) **小 8%**。圖：每條邊有兩種 L 形走法。
- 【口頭】這和[第 5 支](lecture05_note.md) §3 翻 L 形的想法一樣，只是用 DP 系統地找最大重疊。

**Iterated 1-Steiner**（Kahng & Robins, "A new class of Steiner tree heuristics with good performance: the iterated 1-Steiner approach," ICCAD-90）：
- 直覺：**如果只能用一個 Steiner point，它會在哪？**
- **1-Steiner tree** = $P \cup \{x\}$（x 是一個 Steiner point）的 MST。
- Figure 9.20：點集 P (a) 和它的 Hanan point (b)。

```
Algorithm: Iterated_1-Steiner(P)
P: set P of n points.   (Note: result can have more Steiner points)
1 begin
2 S ← ∅;
  /* H(P ∪ S): set of Hanan points */
  /* ΔMST(A, B) = Cost(MST(A)) − Cost(MST(A ∪ B)) */
3 while (Cand ← {x ∈ H(P ∪ S) | ΔMST(P ∪ S, {x}) > 0} ≠ ∅) do
4     Find x ∈ Cand which maximizes ΔMST(P ∪ S, {x});
5     S ← S ∪ {x};
6     Remove degenerate points in S which have degree ≤ 2 in MST(P ∪ S);
7 Output MST(P ∪ S);
8 end
```
- 第 6 行：度數 ≤ 2 的 Steiner point 沒有用（只是在線上轉個彎），拿掉。
- 投影片：歡迎讀課本 Chap-12（張耀文教授寫的）。
- UCSD 論文（j4.pdf）的圖：Fig. 1 同四點的 MST 和 MRST；Fig. 3 iterated 1-Steiner 在四點例子的執行；Fig. 8 一個九點的例子，iterated 1-Steiner 的比值是 **13/11**（最佳 MRST 是 11，heuristic 可能輸出 13）。
  - 【口頭】這個程式雖然是 heuristic，但還不錯。

**目前講過的**：global routing 的 graph 建模；global routing 的 formulation；高階演算法——每條 net 用最短路徑做 global route：用 L-shape 改善來近似 MRST、每次加一個 Steiner point 來近似 MRST、A\* maze search。**怎麼用 rip-up-and-reroute 把 congestion 攤平？**

---

## 13. Congestion map 與 congestion fix

[1:04:27]–[1:09:45]　投影片 Congestion Map – An Important Indicator、Congestion Map、Congestion Fixes、Congestion Profiles

- **Congestion map 是重要指標**（商用工具 IC Compiler II 的截圖）：
  - congestion 的算法可選：**各層 overflow 的總和**，或**總 demand 減總 supply**。
  - 圖上可以選各層（M1 (11)、M2 (13)、M3 (15)…）——投影片：**這表示這個 global router 有做 layer assignment**。
  - 每個 GRC 邊界都標出數值（例如 −15/21、+0/7）。
- **Congestion fix**（vlsi-backend-adventure.com）：
  - 在 channel 和 macro 角落加 **placement blockage**。
  - 檢查 macro placement。
  - 用 **density screen** 降低局部 cell 密度（限制某區域 standard cell 的密度，減少高 pin 密度造成的 congestion）。
  - 重排 scan chain 減少 congestion。
  - 用高 effort 的 **congestion-driven placement**。
  - 反覆迭代直到 congestion 結果夠好。
  - 投影片：**Place ↔ GR**（兩者要來回）。
- **Congestion profile**：fixing 前後的熱圖。
  - 投影片：加 placement blockage；**在這個 bounding box 裡重做 placement，其他 cell 固定；先算 utilization %。GR 能不能 incremental？**
  - 【口頭】實務上就是用這種方法解決；這是 EDA development 的挑戰。

---

## 14. 學術界的 global router

[1:09:45]–[1:13:33]　投影片 Another Good Materials For Global Routing、Pictures From NTHU-Route 2.0、PG shape vs GR shape、GitHub

- 參考資料：Kahng 等人的 chap5.pdf（2011）、UCLA Cong 的 ICCAD routing tutorial。
- **NTHU-Route 2.0: A Fast and Stable Global Router**（ICCAD 2008）：
  - 在 NTHU-Route 上改進：(1) **新的 history based cost function**；(2) 新的 ordering 方法來找 congested region 和做 rip-up-and-reroute；(3) 兩個實作技巧。
  - **先做 2D GR，再做 layer assignment。**
  - 結果：ISPD98 全部解出且品質很好；ISPD07 的 8 個裡 7 個沒有 overflow；一個以前被認為很難的 ISPD07 case 完全消除 overflow；ISPD08 的 16 個裡 12 個沒有 overflow。
  - 【口頭】「fast」而且「stable」——總是找得到好解。history 的意思是：記住某個地方原本很擁擠，rip-up-and-reroute 之後不要再跑回原來的地方。
- **NTHU-Route 2.0 的 Fig. 5**：(a) 前一次迭代的結果（N4 經過比較多 congested 邊）；(b) NTHU-Route 的結果；(c) 用新的 rip-up-and-reroute 順序的結果。另有一篇關於 layer assignment 時 congestion 和 via 最小化的論文。
  - 【口頭】紅色是 congestion（每個 G-cell 邊界只能有一條 net 通過）；從 (a) 改善到 (b)，還有 congestion，再改到 (c)。這就帶到大家的 project：老師定了 routing cell 的 input，請大家把 output 做出來。
- **PG shape vs GR shape**：VDD、VSS 的 power/ground 形狀佔掉 routing 資源。
- **GitHub**：
  - **OGRE**（AUCOHL）：為 ICCAD-Contest 2019 寫的 global router（要 G++、Boost、LEF/DEF parser；投影片：「老師看到用了 Chris Chu 的作品」）。【口頭】ICCAD 2019 也有這個比賽。
  - **CUGR**（cuhk-eda/cu-gr，「worthwhile」）：detailed **routability-driven** global router，品質完全由最後 detailed routing 的結果決定；產生一組連接的長方形來 guide detailed router：
    - 精密的 **probability-based cost** scheme
    - **最佳的 3D pattern routing**，結合 2D pattern routing 和 layer assignment
    - 兩層的 multi-level maze routing
    - **patching** 技巧：加入有用的 route guide 進一步改善 detailed routability
  - 投影片：ieee 9218646（在學校可下載；**可以當 proj-4 的選擇**）。

---

## 15. Detailed routing：pin access、rip-up-and-reroute

[1:13:33]–[1:25:00]　投影片 After Global Routing、Another Critical Problem – Pin Access、Routing Models、Figure 1–2、Detailed Routing、Rip-Up and Re-routing、Latest Attempt In School、Course Ordering；課本 §12.5–12.6

- **Global routing 之後**：照 GR 的 guide 開始做 detailed routing；要談 **pin access**。
  - Global routing → **Track assignment** → **Detailed routing（rip-up-reroute）**。
- **另一個關鍵問題：pin access**：
  - Fig. 1：不同的 routing track 與 placement site **offset**，pin 的 access point 數不同——(a) 好（○），(b) 不好（✗）。圖例：M1 pin、M1 routing track、M2 routing track、cell boundary、access point（×）。
  - 【口頭】track 是從 top level 一條條放下來的；如果事先抓出 access point、了解 track offset、調整得好，就能拿到更多 access point。
- **Pin access for detailed routing**（Figure 1）：(a) **pin access failure**——兩條線擋住藍色斜線的 pin；(b) pin access success（用 via、M2、M3 繞進去）。
  - 投影片：**rip-up-reroute 的一個步驟是檢查某個 pin 的 access point 是不是被擋住了，是的話就移掉附近的 net。**
- **Intra-cell 與 inter-cell pin access**（Figure 2）：(a) M2 track 和 cell layout，**HPC（hit point combination）**；(b) 相鄰 cell 之間可能的 pin access 衝突（violation）。
- **Detailed routing**：
  - 用 GR 的 guide 做 **track assignment**
  - **router 自己的 design rule checker**
  - **area based rip-up and re-route**
- **Rip-up and re-routing**：
  - global 或 detailed router 有 net route 不了時就需要。
  - 方法：手動？自動程序？
  - 兩個步驟：
    1. 找出瓶頸區域，拔掉一些已經 route 好的 net。
    2. route 被擋住的連線，再 reroute 被拔掉的連線。
    3. 但要避免陷入**蹺蹺板（seesaw）**——兩條 net 互相拔來拔去。
  - 重複直到全部 route 完或超過時間限制。
  - **很難，但非常重要而且很強大。**
  - 投影片問：**routing 過程中怎麼偵測 DRC error？**
- **學校最新的 detailed routing 嘗試：Dr. CU**（cuhk-eda/dr-cu，香港中文大學 Evangeline F.Y. Young 教授的團隊）：
  - 和 global routing 不同，detailed routing 要處理很多細節的 design rule，而且在大很多的 routing grid graph 上做；在先進製程是 physical design flow 中**最複雜、最耗時**的階段。
  - 一組兩層的 sparse data structure；最佳的 **correct-by-construction** path search；有效率的 bulk synchronous parallel scheme；…
- **課程順序**：…routing 之後是 **clock/PG routing、Elmore delay、clock tree synthesis**；basic logic synthesis、technology mapping 也標出來了（已經講過）；high level synthesis 灰色（不教）。

---

## 16. 【實測】

程式：`experiments/lecture12/global_route.py`，輸出：`experiments/lecture12/output.txt`。

一個玩具 2D global router：8 × 8 個 G-cell，每個邊界 capacity 2；14 條兩點 net 都從左邊跨到右邊（製造中間的 hot spot）。
- 初始：每條 net 用 A\*（Manhattan 距離當 heuristic）走最短路，不管 congestion。
- 之後每輪：overflow 的邊界加 history cost（記住它擁擠過），把經過 overflow 邊界的 net 拔掉，用「(1 + history) × (1 + present × overflow)」的 cost 重新 A\*——這就是 NTHU-Route 那種 history-based negotiated congestion 的簡化版。

```
8x8 G-cells, boundary capacity 2, 14 two-pin nets, sum of Manhattan lengths 87
  iteration  0: total overflow  35, wirelength 87
  iteration  1: total overflow   6, wirelength 103
  iteration  2: total overflow   0, wirelength 115
iterated 1-Steiner on [(0, 2), (2, 0), (4, 2), (2, 4)]: MST 12 -> 8, Steiner points [(2, 2)]
```

- 只走最短路時總 overflow 35（大家都擠中間）；兩輪 rip-up-and-reroute 後 overflow 歸零，代價是線長從 87 增加到 115（+32%）——**消除 congestion 要拿線長去換**，這正是 global router 要權衡的。
- Iterated 1-Steiner 在十字形四點上第一次就挑到中心 (2, 2)，線長 12 → 8，和 MST/MRST 最差比值 3/2 一致（同[第 5 支](lecture05_note.md) §7）。

---

## 17. 一頁速查表

| 主題 | 重點 |
|---|---|
| G-cell | 把晶片切成 tile；邊界 capacity = 可用 track 數（扣 pin、blockage、via） |
| Capacity | 單層 h/d_pitch；多層 Σ⌊h/d_pitch(layer)⌋ |
| Pitch | line-to-line = 線寬 + spacing；via 大時 pitch 也大 |
| GR 問題 | 每條 net 一棵 Steiner tree；U(e) ≤ c(e)；最小化 ΣL(T_i)（或 max） |
| GR 流程 | 建 G-cell → capacity → 最短路徑 → 找 overflow → RRR → layer assignment |
| Via pillar／NDR | 平行 via 降 R；加寬加間距降 RC；都會吃 routing 資源 |
| A\* | Dijkstra + 到 target 的估計；31 → 6 個節點；bidirectional 更少 |
| Connectivity graph | switchbox graph（節點 = switchbox）、channel graph（節點 = channel） |
| MRST | MST/MRST ≤ 2（DFS 證明）、≤ 3/2（Hwang）；最好近似 61/48；Hanan grid |
| Heuristic | L-shape + DP（少約 8%）；iterated 1-Steiner（每次加最省的 Hanan point，拿掉度數 ≤ 2 的） |
| Congestion map | 每邊界 demand/capacity；fix：placement blockage、density screen、congestion-driven placement |
| NTHU-Route 2.0 | history cost、新的 RRR 順序；先 2D 再 layer assignment |
| CUGR | routability-driven；probability cost、3D pattern routing、multi-level maze、patching |
| DR | GR guide → track assignment → pin access → 內建 DRC → area-based RRR（避免 seesaw） |
| Pin access | track offset 影響 access point 數；HPC；擋到 pin 就拔掉鄰近 net |

---

## 18. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| 2D global router | Prog-3（[第 1 支](lecture01_note.md) §13） |
| pin 位置影響 routing | 第 13 支：pin assignment |
| channel routing（switchbox、T-junction） | 第 14 支：ch8 channel routing |
| power/ground routing、clock routing、Elmore delay | 第 14–15 支 |
| maze router、line search | [第 11 支](lecture11_note.md) |
| Steiner tree、Hanan grid | [第 5 支](lecture05_note.md) |
| congestion-driven placement | 第 10 支：placement |

---

## 附：本講一句話總結

> **Global routing 把晶片切成 G-cell，每個邊界的 capacity 是扣掉 pin、blockage、via 後還能用的 track 數，問題是為每條 net 找一棵 Steiner tree，讓每個邊界的 demand 不超過 capacity、總線長最小：實務流程是 critical net 先走、用 A*（比 Dijkstra 少展開很多節點）或 Steiner tree 走最短路，找出 overflow 後用 rip-up-and-reroute（NTHU-Route 2.0 加上 history cost，不要再跑回原本擁擠的地方）把 congestion 攤平，再做 layer assignment，結果用 congestion map 檢查，congestion 太嚴重要回頭改 placement（placement blockage、density screen）；MRST 是 NP-complete，MST 最多差 3/2（Hwang），實用的 heuristic 有 L-shape 加 DP（少約 8%）和每次加一個最省的 Hanan point 的 iterated 1-Steiner（本機實測十字形四點 12 → 8；玩具 global router 兩輪 RRR 把 overflow 從 35 降到 0，線長付出 32%）；detailed routing 再照 GR 的 guide 做 track assignment，要處理 pin access（track offset 會影響 access point 數）和 router 內建的 DRC，最後靠避免蹺蹺板的 area-based rip-up-and-reroute 收尾。**

---

*筆記依據李炯霆「Electronic Design Automation (1)」第11_2講｜ch7.routing 錄影（NTHU OCW，1:25:03；影片無字幕，以 whisper large-v3-turbo 轉錄）、投影片截圖（Unit 5D；部分引用 Kahng, Lienig, Markov, Hu *VLSI Physical Design* Ch5）、Wang, Chang, Cheng（eds.）*Electronic Design Automation: Synthesis, Verification, and Test* Ch12 §12.3–12.6 整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
