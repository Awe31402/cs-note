# 設計自動化（一）第八支影片完整筆記：Circuit partitioning——LEF/DEF、Kernighan-Lin、hypergraph 與 simulated annealing（ch4 partitioning）

> **課程**：Electronic Design Automation (1)（清大 CSR 5302）— 第8講｜ch4.partitioning（播放清單第 8 支）
> **講者**：李炯霆（JT Li），清大半導體學院，2026 春，NTHU OCW 錄影
> **來源**：YouTube 錄影（1:09:46，無字幕，whisper 轉錄）＋ 投影片截圖（Unit 5A: Circuit Partitioning，頁尾 Chang, Huang, Li, Lin, Liu）＋ 課本 Wang, Chang, Cheng（eds.）*Electronic Design Automation* Ch11 §11.3 Global placement: partitioning-based approach（p.641–649）
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、投影片和課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿辨識不清、或影片／投影片／課本說法不一致之處
> - `[h:mm:ss]`：影片時間點

⚠️ 課本沒有獨立的 partitioning 章；partitioning 放在 Ch11 placement 裡當作 global placement 的一種方法（§11.3），而且講的是 **Fiduccia-Mattheyses（FM）** 演算法，不是影片的 Kernighan-Lin。投影片寫「Readings: Chapter 7.5 and 5.6」，指參考書 Gerez 的章節。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Compaction 回顧](#1-compaction-回顧)
- [2. 設計常用名詞：cell、instance、pin、net、netlist](#2-設計常用名詞cellinstancepinnetnetlist)
- [3. LEF／DEF](#3-lefdef)
- [4. Partitioning 的目的](#4-partitioning-的目的)
- [5. 問題定義](#5-問題定義)
- [6. Kernighan-Lin 演算法](#6-kernighan-lin-演算法)
- [7. D 值與 gain 的公式](#7-d-值與-gain-的公式)
- [8. 加權例子：一步步算](#8-加權例子一步步算)
- [9. 演算法與時間複雜度](#9-演算法與時間複雜度)
- [10. K-L 的擴充與 hypergraph](#10-k-l-的擴充與-hypergraph)
- [11. Simulated annealing](#11-simulated-annealing)
- [12. 用 simulated annealing 做 partition](#12-用-simulated-annealing-做-partition)
- [13. Clustering、其他限制、總結](#13-clustering其他限制總結)
- [14. 套用到 layout：固定的 I/O pad、第二刀](#14-套用到-layout固定的-io-pad第二刀)
- [15. 【實測】](#15-實測)
- [16. 一頁速查表](#16-一頁速查表)
- [17. 與後續講次的連結](#17-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
名詞：cell、instance、pin、net、netlist（hypergraph）；檔案格式 LEF（library）、DEF（design）
partitioning：把電路切成幾塊，塊內連接多、塊間連接（cut）少；2-way balanced 已是 NP-complete
  ├─ Kernighan-Lin：兩兩交換 + lock + 取最大部分和；O(r n³)
  ├─ hypergraph：multi-pin net 用 net cut，或 clique 邊權 2/n
  ├─ simulated annealing：允許 uphill move 跳出 local optimum；cost = cut + λ·不平衡²
  └─ clustering：先把連得很緊的 cell 合成 super node 縮小問題
應用到 layout：遞迴切 → 底層 box 裡做 detailed placement；固定 I/O pad 會影響切法
```

---

## 1. Compaction 回顧

[0:00]–[3:48]　投影片 Recap Of Compaction

- 從 design rule 建 constraint graph：forward edge、backward edge、cycle。
- 找 longest path → 用 breadth-first search。
- Liao & Wong：forward 求 longest path、backward edge 更新 cost…
- Bellman-Ford：用兩個集合。
- 應用：building block layout…；interactive compaction 拿掉空白。
- Topological order：對 **project planning** 有用。
  - 【口頭】某個 task 做完才能做下一個，有這些 dependency，要排成一個 linear order。YouTube 上有用 DFS 的好解釋。
- 這門課講很多演算法。

---

## 2. 設計常用名詞：cell、instance、pin、net、netlist

[3:48]–[8:27]　投影片 Unit 5A: Circuit Partitioning、Common Terminologies Used In Designs；課本 §11.3.1.1（p.642）

- 內容：Kernighan-Lin partitioning 演算法、simulated annealing 為基礎的 partitioning。
- 圖：同一個電路，切法不同 cut size 不同（cut size = 4 vs 2），右邊是 graph representation（節點 1–8）。

| 名詞 | 意思 |
|---|---|
| **Cell** | 有基本功能的單元，例如 NAND、NOR、INV |
| **Instance** | cell 的實例化（instantiation） |
| **Pin** | cell 上的連接點，例如 2 輸入 NAND 有 2 個 input pin、1 個 output pin（通常略過 power、ground pin，但要考慮 multiple VDD） |
| **Terminal／port** | instance 上的 pin（pin、terminal、port 常常混用） |
| **Net** | 連到多個 terminal 的一條連線 |
| **Netlist** | 一組 net——就是一個 **hypergraph** |

- 例：4 個 input、2 個 output、4 個 instance（inv、nand、nor、D flip-flop）、8 條 net。
- 【課本補充】課本把電路建成 hypergraph：module 是頂點（面積是頂點大小）、net 是 hyperedge（net 的重要性是權重）。例：頂點 {A, B, C, D, E}、hyperedge {A,B}、{A,C}、{A,D}、{B,D}、{D,E}、{B,C,E}（p.642）。

---

## 3. LEF／DEF

[8:27]–[15:57]　投影片 Popular LEF/DEF For Library Cells & Layout、Popular Layout Interface Files、DEF、Multi-mask Patterns（LEF/DEF Reference 5.7，2009 年 11 月）

**LEF（Library Exchange Format）**——描述製程和 library cell：

```
# DEMO4 CHIP - 1280
NAMESCASESENSITIVE ON
# PLACEMENT SITE SECTION
SITE CORE1 SIZE 67.2 BY 6 ;     # GCD of all Y sizes of Macros
SITE IOX SIZE 37.8 BY 444 ;     # 151.2 / 4 = 37.8 , 4 sites per pad
SITE IOY SIZE 436.8 BY 30 ;     # 150 / 5 = 30 , 5 sites per pad
SITE SQUAREBLOCK SIZE 268.8 BY 252 ;
...
LAYER M1 TYPE ROUTING ; DIRECTION VERTICAL ; PITCH 5.6 ; WIDTH 2.6 ; SPACING 1.5 ; END M1
LAYER CUT12 TYPE CUT ; END CUT12
LAYER M2 TYPE ROUTING ; DIRECTION HORIZONTAL ; PITCH 6.0 ; WIDTH 3.2 ; SPACING 1.6 ; END M2
LAYER CUT23 TYPE CUT ; END CUT23
LAYER M3 TYPE ROUTING ; DIRECTION VERTICAL ; PITCH 5.6 ; WIDTH 3.6 ; SPACING 1.6 ; END M3

MACRO IN1X class pad ;
    FOREIGN IN1X ; SIZE 151.2 BY 444 ;
    SYMMETRY X ; SITE IOX ;
PIN Z DIRECTION OUTPUT ;
    PORT LAYER M1 ; PATH 61.6 444 72.8 444 ; END
END Z
PIN PO DIRECTION OUTPUT ; ...
PIN A DIRECTION INPUT ; ...
```

- 【口頭】
  - LEF 描述這個製程有哪些 layer：Metal 1 給 routing、方向**垂直**、pitch、width、spacing；CUT12 是從 metal 1 到 metal 2 的 via 層；Metal 2 變**水平**、pitch 6.0 和 M1 不同。
  - `MACRO` 就是 cell 的定義：IN1X 是 class pad，SITE 放在 IOX；它有 output pin Z，連接點叫 **port**，在 M1 層，形狀是一個 path——connection point 是一組 polygon，可以接到 pin 上。
  - 看起來關鍵字都是大寫；細節要參考 LEF/DEF 的 definition。

**DEF（Design Exchange Format）**——描述設計本身：

```
DESIGN DEMO4CHIP ;
    TECHNOLOGY DEMO4CHIP ;
    ARRAY DEMO4 ;
    UNITS DISTANCE MICRONS 100 ;
COMPONENTS 243 ;
- CORNER1 CORNER ; - CORNER2 CORNER ;
- C01 IN1X ; - C02 IN1Y ; - C04 IN1X ; - C05 IN1X ;
- C13 BIDIR1Y ; - C14 INV ; - C15 BUF ; - C16 BUF ;
...
NETS 222 ;
- VDD ( Z216 B ) ( Z22 Z ) ( Z21 Z ) ( Z16 Z ) ( Z15 Z ) ...
- Z38QN1 ( Z38A01 QN ) ( Z207 B ) ;
- COZ101 ( Z38A01 Q ) ( Z207 A ) ;
...
```

- net 還可以有更多資料，例如 wire、via（描述繞線）。
- 【口頭】LEF 是 library，DEF 是 design：用了哪些 instance、彼此怎麼連；和外界的介面也用 DEF。

**Multi-mask patterns**（LEF/DEF 5.7 Example 4-11）：

```
+ ROUTED M1 ( 10 0 ) MASK 3 ( 10 20 ) VIA1_1
  NEW M2 ( 10 10 ) ( 20 10 ) MASK 1 ( 20 20 ) MASK 031 VIA1_2 ;
```
- M1 線（10 0）到（10 20）屬於 mask 3。
- VIA1_1 前面沒有 MASK，所以 metal 和 cut 都沒有 mask（uncolored）。
- 第一段 NEW M2（10 10）到（20 10）沒有 mask；第二段 M2（20 10）到（20 20）在 mask 1。
- VIA1_2 是 MASK 031：top mask 0（M2 沒有 mask）、bottom mask 1（M1 用 mask 1）、cut mask 3（最下面最左邊的 cut 是 mask 3，其他 cut 的 mask 由 via-master 的 mask「平移」得到：1→3、2→1、3→2，Figure 4-4）。
- 這是 multiple patterning（一層 metal 拆成好幾道 mask 曝光）在檔案格式上的表示。

---

## 4. Partitioning 的目的

[15:57]–[21:36]　投影片 Partitioning、Circuit Partitioning、What Can Partitioning Achieve?、Then Cell Rows Are Considered

**系統設計的 partitioning**：
- 把複雜系統分解成較小的子系統。
- 每個子系統可以獨立設計，加快設計流程。
- **分解要讓子系統之間的連接最少。**
- 階層式分解，直到每個子系統大小可管理。
- 圖：system design → Module 1、Module 2、…、Module n，加上 Interface。

**Circuit partitioning**：
- **目標**：把電路切成幾部分，使每部分的大小在規定範圍內，而且**部分之間的連接數最少**。某些應用還有更多限制。
- Cutset？cut size？一部分的大小？
- 【口頭】實務上大部分都是一次切兩塊（bisection），二、四…這樣繼續分下去。cut size 就是兩組之間的連接數，希望越小越好。

**Partitioning 能做什麼？**
- 遞迴切成一格一格的 box，**在最底層的 box 裡做 detailed placement**。
- **接著考慮 cell row**：從 partitioning 的結果，把 instance map 到 row 上、不重疊（detailed placement）。

---

## 5. 問題定義

[21:36]–[24:03]　投影片 Problem Definition: Partitioning；課本 §11.3.1.1（p.642–643）

- **k-way partitioning**：給 graph G(V, E)，每個頂點 v 有大小 s(v)、每條邊 e 有權重 w(e)，把 V 分成 k 個互斥子集 V₁, V₂, …, V_k，在某些限制下最佳化目標函數。
- **Bounded size constraint**：第 i 個子集的大小受限 $\sum_{v\in V_i} s(v) \le B_i$。——partition 平衡嗎？
- **兩個子集之間的 min-cut cost**：最小化
$$\sum_{\forall e=(u,v)\wedge p(u)\ne p(v)} w(e)$$
  p(u) 是節點 u 所在的 partition 編號。
- **2-way balanced partitioning 是 NP-complete**，就算是頂點大小相同、邊權都是 1 的最簡單形式也是。
- 【課本補充】課本用 hypergraph 形式：cut cost 是跨兩邊的 hyperedge 權重總和；大小限制用比例參數 r 和容忍度 ε：$r - \varepsilon \le \sum_{u\in V_1}s(u)/\sum_{u\in V}s(u) \le r + \varepsilon$；hypergraph bipartitioning 是 NP-complete（Garey 1979）。

---

## 6. Kernighan-Lin 演算法

[24:03]–[31:12]　投影片 Kernighan-Lin Algorithm、K-L Algorithm: A Simple Example

- Kernighan and Lin, "An efficient heuristic procedure for partitioning graphs," *The Bell System Technical Journal*, vol. 49, no. 2, Feb. 1970。
- **iterative、2-way、balanced** 的 partitioning（bisection）heuristic。
- 只要 cut size 還在下降就繼續：
  - 給初始兩個子集，算初始 cut 數。
  - 找讓 cut size **下降最多或上升最少**的頂點對，交換。
  - 這兩個頂點被 **lock**（之後不再參與交換）。
  - 持續到**所有頂點都被 lock**。
  - 找**部分和最大**的那一段交換來實際執行。
  - unlock 所有頂點。

**簡單例子**（邊權都是 1）：初始 {a, b, c, d}、{e, f, g, h}，cut cost 5。

| Step # | Vertex pair | Cost reduction | Cut cost |
|---|---|---|---|
| 0 | – | 0 | 5 |
| 1 | {d, g} | 3 | 2 |
| 2 | {c, f} | 1 | 1 |
| 3 | {b, h} | −2 | 3 |
| 4 | {a, e} | −2 | 5 |

- 最好的是做前 2 步（部分和 3 + 1 = 4，cut 從 5 降到 1）。
- 投影片問：cost reduction 怎麼算？哪一對要交換？→ 考慮 internal 和 external connection 的變化。
- **一直做到所有頂點都 lock**，是為了避免 ping-pong（同一對換過來又換回去）。

---

## 7. D 值與 gain 的公式

[31:12]–[34:21]　投影片 Properties

兩個集合 A、B，|A| = n = |B|，A ∩ B = ∅。$c_{xy}$ 是 x、y 之間的邊權。

- **External cost**：$E_a = \sum_{v\in B} c_{av}$（a 連到對面的總權重）
- **Internal cost**：$I_a = \sum_{v\in A} c_{av}$（a 連到自己這邊的總權重）
- **D 值**：$D_a = E_a - I_a$（把 a 移過去的 cost reduction）
- **交換 a、b 的 gain**：
$$g_{ab} = D_a + D_b - 2c_{ab}$$
  （a、b 之間的邊原本在 cut 上，交換後還在 cut 上，但 $D_a$、$D_b$ 各把它算成會被消掉，所以要扣兩次。）
- a ∈ A、b ∈ B 交換後，新的 D 值：
$$D'_x = D_x + 2c_{xa} - 2c_{xb},\ \forall x \in A - \{a\}$$
$$D'_y = D_y + 2c_{yb} - 2c_{ya},\ \forall y \in B - \{b\}$$
- 圖解（x 在 A）：x–a 的邊交換前是 internal（−c_xa），交換後變 external（+c_xa），變化 +2c_xa；x–b 的邊交換前是 external（+c_xb），交換後變 internal（−c_xb），變化 −2c_xb。

---

## 8. 加權例子：一步步算

[34:21]–[42:42]　投影片 K-L Algorithm: A Weighted Example、Computing the g Value、Updating the D Value、Determining Swapping Pairs、Next Iteration

6 個頂點的完全圖，權重矩陣：

| | a | b | c | d | e | f |
|---|---|---|---|---|---|---|
| a | 0 | 1 | 2 | 3 | 2 | 4 |
| b | 1 | 0 | 1 | 4 | 2 | 1 |
| c | 2 | 1 | 0 | 3 | 2 | 1 |
| d | 3 | 4 | 3 | 0 | 4 | 3 |
| e | 2 | 2 | 2 | 4 | 0 | 2 |
| f | 4 | 1 | 1 | 3 | 2 | 0 |

A = {a, b, c}、B = {d, e, f}。初始 cut cost = (3 + 2 + 4) + (4 + 2 + 1) + (3 + 2 + 1) = **22**（分別來自 a、b、c）。

**Iteration 1**：

| | I | E | D = E − I |
|---|---|---|---|
| a | 1 + 2 = 3 | 3 + 2 + 4 = 9 | 6 |
| b | 1 + 1 = 2 | 4 + 2 + 1 = 7 | 5 |
| c | 2 + 1 = 3 | 3 + 2 + 1 = 6 | 3 |
| d | 4 + 3 = 7 | 3 + 4 + 3 = 10 | 3 |
| e | 4 + 2 = 6 | 2 + 2 + 2 = 6 | 0 |
| f | 3 + 2 = 5 | 4 + 1 + 1 = 6 | 1 |

$g_{xy} = D_x + D_y - 2c_{xy}$（a 和對面每個節點都算一次，以此類推）：

| 對 | g |
|---|---|
| ad | 6 + 3 − 2×3 = 3 |
| ae | 6 + 0 − 2×2 = 2 |
| af | 6 + 1 − 2×4 = −1 |
| bd | 5 + 3 − 2×4 = 0 |
| be | 5 + 0 − 2×2 = 1 |
| **bf** | 5 + 1 − 2×1 = **4（最大）** |
| cd | 3 + 3 − 2×3 = 0 |
| ce | 3 + 0 − 2×2 = −1 |
| cf | 3 + 1 − 2×1 = 2 |

→ 交換 b 和 f（$\hat g_1 = 4$），鎖住 b、f。

**更新 D**（交換 p = b、q = f）：
- $D'_a = D_a + 2c_{ab} - 2c_{af} = 6 + 2 − 8 = 0$
- $D'_c = D_c + 2c_{cb} - 2c_{cf} = 3 + 2 − 2 = 3$
- $D'_d = D_d + 2c_{df} - 2c_{db} = 3 + 6 − 8 = 1$
- $D'_e = D_e + 2c_{ef} - 2c_{eb} = 0 + 4 − 4 = 0$

$g_{ad} = 0 + 1 − 6 = −5$、$g_{ae} = 0 + 0 − 4 = −4$、$g_{cd} = 3 + 1 − 6 = −2$、**$g_{ce} = 3 + 0 − 4 = −1$（最大）** → 交換 c 和 e（$\hat g_2 = -1$）。

**再更新**：$D''_a = D'_a + 2c_{ac} - 2c_{ae} = 0 + 4 − 4 = 0$；$D''_d = D'_d + 2c_{de} - 2c_{dc} = 1 + 8 − 6 = 3$；$g_{ad} = 0 + 3 − 6 = −3$（$\hat g_3 = -3$）。
- 這一步其實是多餘的（$\sum_{i=1}^n \hat g_i = 0$：全部換完等於 A、B 對調，cut 不變）。

**總結**：$\hat g_1 = 4$、$\hat g_2 = -1$、$\hat g_3 = -3$。最大部分和 $\max\sum_{i=1}^k \hat g_i = 4$（k = 1）→ **只交換第 1 對 b 和 f**。

**Iteration 2**（從前一次的結果開始）：A = {a, c, f}、B = {b, d, e}，初始 cut cost = (1 + 3 + 2) + (1 + 3 + 2) + (1 + 3 + 2) = **18**（= 22 − 4）。
- 重複 Iteration 1 的做法：$\hat g_1 = g_{ce} = -1$、$\hat g_2 = g_{ab} = -3$、$\hat g_3 = g_{fd} = 4$。
- 最大部分和 = 0（k = 3）→ **停止**。最後 cut = 18。
- 【實測】暴力窮舉所有平衡切法，18 確實是最佳，見 [§15](#15-實測)。

---

## 9. 演算法與時間複雜度

[42:42]–[47:48]　投影片 Kernighan-Lin Algorithm、Time Complexity

```
Algorithm: Kernighan-Lin(G)
Input: G = (V, E), |V| = 2n.
Output: Balanced bi-partition A and B with "small" cut cost.
1  begin
2  Bipartition G into A and B such that |V_A| = |V_B|, V_A ∩ V_B = ∅, and V_A ∪ V_B = V.
3  repeat
4      Compute D_v, ∀v ∈ V.
5      for i = 1 to n do
6          Find a pair of unlocked vertices v_ai ∈ V_A and v_bi ∈ V_B whose exchange
           makes the largest decrease or smallest increase in cut cost;
7          Mark v_ai and v_bi as locked, store the gain ĝ_i, and compute the new D_v,
           for all unlocked v ∈ V;
8      Find k, such that G_k = Σ_{i=1}^{k} ĝ_i is maximized;
9      if G_k > 0 then
10         Move v_a1, ..., v_ak from V_A to V_B and v_b1, ..., v_bk from V_B to V_A;
11     Unlock v, ∀v ∈ V.
12 until G_k ≤ 0;
13 end
```

**時間複雜度**：
- Line 4：初始算 D：O(n²)
- Line 5：for 迴圈：O(n)
- 迴圈本體：O(n²)（Lines 6–7：第 i 步要試 (n − i + 1)² 對）
- **Lines 4–11：repeat 每一輪 O(n³)**
- 假設 repeat 跑 r 輪 → 總共 **O(r n³)**。
- 投影片問：這是 polynomial-time 演算法嗎？（r 沒有已知的好上界——K-L 是 heuristic，不保證找到最佳解。）
- 【課本補充】課本講的 **Fiduccia-Mattheyses（FM）** 是 K-L 的改良：一次只**移動一個**頂點（不必成對交換，可以處理不平衡大小），直接處理 hypergraph，用 bucket 資料結構讓一輪做到線性時間；同樣是 lock + 取最好的前 k 步，能跳出 local minimum（p.643–645）。【口頭】老師在總結時提到「後來也有不同的 variation，像 Fiduccia 的 min-cut」。

---

## 10. K-L 的擴充與 hypergraph

[47:48]–[52:15]　投影片 Extensions of K-L Algorithm、Coping With Hypergraph、Net-Cut Model

**K-L 的擴充**：
- **子集大小不同**（n₁ < n₂）：
  1. 分成 |A| = n₁、|B| = n₂。
  2. 在 A 加 n₂ − n₁ 個 dummy 頂點，和原圖沒有連接。
  3. 套 K-L。
  4. 移除 dummy。
- **「頂點」大小不同**：
  1. 假設最小的頂點大小是 1（或取最大公因數）。
  2. 大小 s 的頂點換成 s 個頂點，彼此用**無限大權重**的邊完全連接（保證不會被切開）。
  3. 套 K-L。
- **k-way partition**：
  1. 先分成 k 個大小相同的集合。
  2. 每對子集套 K-L。
  3. complexity？可以用遞迴 bi-partition 降低。
- **怎麼處理 hypergraph？** 要直接處理 multi-terminal net。

**Hypergraph**：H = (N, L)，N 是頂點集合，L 是 hyperedge 集合，每個 hyperedge 對應頂點的一個子集 $N_i$，$|N_i| \ge 2$（multi-pin net）。
- Schweikert and Kernighan, "A proper model for the partitioning of electrical circuits," 9th Design Automation Workshop, 1972。
- multi-terminal net 用 **net cut** 當 cut cost 比較準（一條 net 只要被切到就算 1）。
- 例：net 1 = {A, B, C, D}、net 2 = {A, E}、net 3 = {B, E}、net 4 = {C, F}、net 5 = {D, F}：{A, B, E}、{C, D, F} 是好的 partition，用 net cut 算 cost = 1；但如果把 net 1 展開成 4 個點的完全圖、每條邊權重都是 1，切開它就算 4——**不該所有邊都給一樣的權重**。

**Net-cut model**：
- n(i) = net i 上的 cell 數。連接 cell x、y 的邊權 $w_{xy} = \frac{2}{n(i)}$。
  - 【口頭】4 個 pin 的 net 變成完全圖，邊權 2/4 = 1/2，切開它的 cost 是 4 條邊 × 1/2 = 2。
- 對 K-L 的簡單修改：$g_{xy} = D_x + D_y - \text{Correction}(x, y)$。
- 投影片註：**有些訊號 net（例如 reset）接到很多 pin，通常不必納入**。

---

## 11. Simulated annealing

[52:15]–[1:00:57]　投影片 Simulated Annealing、Convergence of SA、Simulated Annealing Basics、Generic SA Algorithm、Basic Ingredients

- Kirkpatrick, Gelatt, and Vecchi, "Optimization by simulated annealing," *Science*, May 1983。
- Greene and Supowit, "Simulated annealing without rejected moves," ICCD-84。
- 圖：cost 對 state 的曲線有很多 **local optima** 和一個 **global optimum**——**怎麼跳出 local optima？**

**收斂過程**（投影片引用「Solving NP Complete Timetable Problem Using Monte Carlo and Simulated Annealing Algorithms」）：一開始溫度高（init_temp）時幾乎無條件接受；中間有 hill climbing——移動以機率 $e^{-\Delta C/temp}$ 被接受；最後溫度低（final_temp）時收斂。
- 【口頭】溫度很高時，壞的 move 也很容易被接受。

**基本概念**：
- 「up-hill」move 有非零的機率被接受。
- 機率取決於：(1) up-hill 的幅度；(2) 總搜尋時間。

$$\text{Prob}(S \to S') = \begin{cases} 1 & \Delta C \le 0 \quad\text{（down-hill）} \\ e^{-\Delta C/T} & \Delta C > 0 \quad\text{（up-hill）} \end{cases}$$

- $\Delta C = cost(S') - cost(S)$。
- T：控制參數（溫度）。T 高時幾乎所有壞 move 都被接受；T 低時幾乎不接受。
- **Annealing schedule**：$T = T_0, T_1, T_2, \dots$，$T_i = r^i T_0$，r < 1。
- 投影片：「**Printing not clear. Do you see anything missing?**」——原投影片的指數少印了**負號**，應該是 $e^{-\Delta C/T}$（ΔC > 0 時機率才小於 1）。

**Generic SA 演算法**：
```
1  begin
2  Get an initial solution S;
3  Get an initial temperature T > 0;
4  while not yet "frozen" do
5      for 1 ≤ i ≤ P do
6          Pick a random neighbor S' of S;
7          Δ ← cost(S') − cost(S);
           /* downhill move */
8          if Δ ≤ 0 then S ← S'
           /* uphill move */
9          if Δ > 0 then S ← S' with probability e^{−Δ/T};   // 用亂數產生
10     T ← rT;   /* reduce temperature */
11 return S
12 end
```

**類比**：

| 物理系統 | 最佳化問題 |
|---|---|
| state | configuration |
| energy | cost function |
| ground state | optimal solution |
| quenching（急冷） | iterative improvement |
| careful annealing（慢慢退火） | simulated annealing |

**SA 的基本元素**：solution space、neighborhood structure、cost function、annealing schedule。

---

## 12. 用 simulated annealing 做 partition

[1:00:57]–[1:04:03]　投影片 Partition by Simulated Annealing

- **Solution space**：所有 partition 的集合（包括不平衡的，例如 {a,b,c}/{d,e,f}、{a,b}/{c,d,e,f}、{a,b,c,d}/{e,f}）。
- **Neighborhood structure**：**隨機把一個 cell 移到另一邊**。
- **Cost function**：$f = C + \lambda B$
  - C：前面的 partition cost（cut）
  - B：平衡程度，$B = (|S_1| - |S_2|)^2$
  - λ：常數
- **Annealing schedule**：
  - $T_n = r^n T_0$，r = 0.9。
  - 每個溫度做到：平均每個 cell 有 10 次被接受的 move，或嘗試次數 ≥ 100 × cell 總數。
  - 連續 3 個溫度的接受率都很低，就算「frozen」。
- 【口頭】有興趣的同學可以看投影片附的連結。

---

## 13. Clustering、其他限制、總結

[1:04:03]–[1:07:42]　投影片 Clustering、Any Other Constraints You Can Think Of?、Summary So Far

- **Clustering**：把連接很緊（相關）的元件合成一個 **super node**，**縮小問題規模**。
  - 2003 年教育部 IC/CAD contest Problem 7：TOP 下面有 A1、B1、A2、B2…，把 (A1, B1) 合成 C1、(A2, B2) 合成 C2…，TOP 下只剩 C1–C4。
  - 【口頭】K-L 是 n³，元件很多時很慢；A1、B1 連得很緊，希望它們在一起，乾脆當一個 super node，元件數少了速度就快。
- **還想得到什麼限制？** 如果有些 cell **固定在某些位置**呢？
  - 【口頭】實務上常有 cell 固定在某處不能移動，或某些元件必須在一起不能分開。
- **總結**：bi-partitioning；Kernighan-Lin；multi-pin net（hypergraph）；simulated annealing；先 clustering 縮小問題。
  - 【口頭】從 balanced 講到 unbalanced；最重要的是 Kernighan-Lin，後來也有 Fiduccia 的 min-cut 等 variation。

---

## 14. 套用到 layout：固定的 I/O pad、第二刀

[1:07:42]–[1:09:42]　投影片 How To Apply Partitioning To Layout? (1)(2)

- **所有 I/O pad 都可以動** vs **所有 I/O pad 固定**：**固定的節點一定會影響 partitioning。怎麼影響？**
  - 【口頭】接到固定 I/O pad 的元件，希望放在靠近那個 pad 的 partition 裡。
- **第二刀怎麼切？** 兩種做法：
  - 一刀從最左邊切到最右邊，貫穿整個 chip；
  - 只對每個子群各自切，切到中間就停，兩邊的位置可以不同（大小也可以不太平均）。
  - 【口頭】這和 partitioning 演算法有關。下次會講 min-cut 在 placement 裡實際怎麼用。
- 【課本補充】課本 §11.3.2 把這叫 **placement by partitioning**，並介紹 **terminal propagation**：切子區域時，把外面（已經決定在某一側）的連線當成固定在邊界上的假 terminal，這樣子區域的 partition 會考慮到外部的拉力——正是老師問的「固定節點怎麼影響 partitioning」（p.646–648）。

---

## 15. 【實測】

程式：`experiments/lecture08/kl_sa.py`，輸出：`experiments/lecture08/output.txt`。

```python
        for x in A:          # D'_x = D_x + 2c_xa - 2c_xb
            if x not in locked:
                D[x] += 2 * c(x, a) - 2 * c(x, b)
        for y in B:          # D'_y = D_y + 2c_yb - 2c_ya
            if y not in locked:
                D[y] += 2 * c(y, b) - 2 * c(y, a)
```
（`experiments/lecture08/kl_sa.py:40-45`）

```
Kernighan-Lin from A={a,b,c}, B={d,e,f}:
 pass 1: A=['a', 'b', 'c'] B=['d', 'e', 'f'] cut=22
   D = {'a': 6, 'b': 5, 'c': 3, 'd': 3, 'e': 0, 'f': 1}
   swap b,f: g = 4
   swap c,e: g = -1
   swap a,d: g = -3
   gains [4, -1, -3], partial sums [4, 3, 0], best k = 1, G_k = 4
 pass 2: A=['a', 'f', 'c'] B=['d', 'e', 'b'] cut=18
   D = {'a': 0, 'b': -3, 'c': 3, 'd': 1, 'e': 0, 'f': 1}
   swap c,e: g = -1
   swap a,d: g = -3
   swap f,b: g = 4
   gains [-1, -3, 4], partial sums [-1, -4, 0], best k = 3, G_k = 0
 result A=['a', 'c', 'f'] B=['b', 'd', 'e'] cut=18
brute-force optimum over all balanced bisections: (18, ('a', 'c', 'f'))
simulated annealing (lambda=1, r=0.9):
  seed 0 (['b', 'd', 'e'], ['a', 'c', 'f'], 18.0)
  ...（5 個 seed 都是 18）
```

- Pass 1 的 D 值、三次 gain（4、−1、−3）、最大部分和 k = 1，和投影片完全一致。
- Pass 2 的 gain 序列 −1、−3、4 也和投影片一致；投影片寫第 2、3 對是 (a, b)、(f, d)，程式挑到 (a, d)、(f, b)——這是 gain 相同時的 tie-breaking 不同（兩種都是 −3、4），結果一樣停止。
- 暴力窮舉 C(6,3)/2 = 10 種平衡切法，最小 cut 就是 18（{a, c, f} / {b, d, e}）——這個例子 K-L 找到全域最佳。
- SA（cost = cut + 1·(|S₁| − |S₂|)²、r = 0.9、每個溫度 100 × 6 次嘗試、連續 3 個溫度接受率 < 2% 就停）5 個 seed 都收斂到同一個最佳解。

---

## 16. 一頁速查表

| 主題 | 重點 |
|---|---|
| 名詞 | cell → instance；pin／terminal／port；net；netlist = hypergraph |
| LEF | 製程 layer（方向、pitch、width、spacing）、site、MACRO（cell、pin、port 形狀） |
| DEF | DESIGN、COMPONENTS、NETS（可含 wire、via、mask） |
| Partitioning 目標 | 各部分大小在範圍內、cut（部分間連接）最少 |
| Min-cut | Σ w(e)，e 兩端在不同 partition |
| 難度 | 2-way balanced 就是 NP-complete |
| K-L | 成對交換、lock、取最大部分和 G_k > 0 才執行；直到 G_k ≤ 0 |
| D 值 | D_a = E_a − I_a |
| Gain | g_ab = D_a + D_b − 2c_ab |
| 更新 | D'_x = D_x + 2c_xa − 2c_xb（x ∈ A） |
| 複雜度 | 每輪 O(n³)，r 輪 O(r n³) |
| 擴充 | dummy 頂點（大小不等）、拆成 s 個無限權重相連的頂點、k-way |
| Hypergraph | net cut；clique 邊權 2/n；高 fanout 的 net（reset）可略 |
| FM | 一次移一個頂點、bucket、每輪線性（課本） |
| SA | 接受機率 e^{−ΔC/T}；T_i = r^i T_0；跳出 local optimum |
| SA partition | 隨機移一個 cell；f = C + λ(\|S₁\| − \|S₂\|)²；r = 0.9 |
| Clustering | 把緊密相連的 cell 合成 super node |
| Layout | 固定 I/O pad 影響切法（terminal propagation）；第二刀貫穿或各自切 |

---

## 17. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| simulated annealing | 第 9 支：floorplanning（SA + slicing tree／B\*-tree） |
| min-cut placement、terminal propagation | 第 10 支：placement（partitioning-based global placement） |
| LEF／DEF | 第 10–12 支：placement、routing 的輸入輸出格式；Prog-2、Prog-3 |
| Hw-3：partitioning | [第 1 支](lecture01_note.md) §13 |
| clustering | 第 10 支：multilevel placement |

---

## 附：本講一句話總結

> **Netlist 是一個 hypergraph（cell 的 instance 用 net 連起來），實務上用 LEF 描述製程與 cell、DEF 描述設計；partitioning 要把電路切成大小受限、彼此連接（cut）最少的幾塊，就算是 2-way balanced 也已經 NP-complete：Kernighan-Lin 用 D = E − I 和 gain g_ab = D_a + D_b − 2c_ab 每次交換最好的一對、鎖住，全部換完後只執行部分和最大的前 k 對，重複到 G_k ≤ 0，每輪 O(n³)（本機實測投影片的 6 點例子 22 → 18，正是暴力窮舉的最佳解）；multi-pin net 要用 net cut 或 clique 邊權 2/n；simulated annealing 用 e^{−ΔC/T} 的機率接受變壞的 move 來跳出 local optimum，做 partition 時隨機把一個 cell 移到另一邊、cost = cut + λ(|S₁| − |S₂|)²；元件太多時先 clustering 成 super node；套用到 layout 時，固定的 I/O pad 會把相連的 cell 拉到靠近它的那一邊，遞迴切到底層的 box 裡再做 detailed placement。**

---

*筆記依據李炯霆「Electronic Design Automation (1)」第8講｜ch4.partitioning 錄影（NTHU OCW，1:09:46；影片無字幕，以 whisper large-v3-turbo 轉錄）、投影片截圖（Unit 5A）、Wang, Chang, Cheng（eds.）*Electronic Design Automation: Synthesis, Verification, and Test* Ch11 §11.3 整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
