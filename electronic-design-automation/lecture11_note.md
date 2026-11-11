# 設計自動化（一）第十一支影片完整筆記：Routing（一）——routing 基礎、DRC 規則、maze router 與 line search（ch7 routing, part 1）

> **課程**：Electronic Design Automation (1)（清大 CSR 5302）— Lecture 11-1 | Ch7. Routing（播放清單第 11 支）
> **講者**：李炯霆（JT Li），清大半導體學院，2026 春，NTHU OCW 錄影
> **來源**：YouTube 錄影（1:17:03，無字幕，whisper 轉錄）＋ 投影片截圖（Unit 5D: Maze (Area) and Global Routing，頁尾 Chang, Huang, Li, Lin, Liu）＋ 課本 Wang, Chang, Cheng（eds.）*Electronic Design Automation* Ch12 §12.1–12.3 General-purpose routing（p.688–697）
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、投影片和課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿辨識不清、或影片／投影片／課本說法不一致之處
> - `[h:mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Maze routing 的直覺](#1-maze-routing-的直覺)
- [2. Global routing 與 detailed routing](#2-global-routing-與-detailed-routing)
- [3. Routing constraints](#3-routing-constraints)
- [4. 岔題：DRC 規則（EOL、parallel-run spacing）](#4-岔題drc-規則eolparallel-run-spacing)
- [5. Routing 的分類](#5-routing-的分類)
- [6. Lee 演算法](#6-lee-演算法)
- [7. 減少 memory 與執行時間](#7-減少-memory-與執行時間)
- [8. Multi-terminal net 與 weighted grid](#8-multi-terminal-net-與-weighted-grid)
- [9. Hadlock 演算法](#9-hadlock-演算法)
- [10. Soukup 演算法](#10-soukup-演算法)
- [11. Line search：Mikami-Tabuchi 與 Hightower](#11-line-searchmikami-tabuchi-與-hightower)
- [12. 演算法比較與 multi-layer routing](#12-演算法比較與-multi-layer-routing)
- [13. Net ordering](#13-net-ordering)
- [14. 各種 design style 的 global routing](#14-各種-design-style-的-global-routing)
- [15. 【實測】](#15-實測)
- [16. 一頁速查表](#16-一頁速查表)
- [17. 與後續講次的連結](#17-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
placement → global routing（每條 net 分到哪些 routing region）→ detailed routing（真正的線、不能有 DRC）
主流：global route → detailed route（track assignment + rip-up-and-reroute）
DRC：min spacing、end-of-line（EOL）spacing、parallel-run spacing → router 要自己檢查且和 signoff 一致
maze（area）routing：
   Lee（BFS 波前；保證最短；O(MN) 時間與空間）
   └ Akers 1,1,2,2 標記省 memory；起點選擇、雙向、framing 省時間
   └ multi-terminal：從整棵樹擴散；weighted grid
   Hadlock（detour number，最短、搜尋少）、Soukup（line + bubble，快但不一定最短）
line search（O(L)）：Mikami-Tabuchi（每點都是 escape point）、Hightower（每段一個 escape point）
net ordering 影響結果；mutually intervening 要 rip-up-and-reroute
```

---

## 1. Maze routing 的直覺

[0:00]–[10:12]　投影片 Unit 5D 首頁（Filling – wave expansion、Retrace）；課本 §12.3.1

- 內容：routing 基礎、**maze（area）routing**、line searching／probing；global routing；detailed routing——**主流做法是 track assignment + rip-up-and-reroute**。閱讀：Chapters 9.1、9.2、9.5（Gerez）。
- 【口頭】
  - line search／probing 在 **custom design** 的 routing 常用；digital（standard cell）則是 global route 接 detailed route，detailed route 做 track assignment，再看 DRC violation、用 rip-up-and-reroute 解掉。
  - 圖：灰色是障礙物，S 是 source、T 是 target。最暴力的方法就是 maze route：從 source 往上下左右擴散，cost 都是 1，標成 1；波前再往外走標 2、3…（有些看起來是往反方向走）；直到碰到 T，再從 T 沿著數字遞減**retrace** 回 S。
  - retrace 時可以有好幾條路（例如一條 L 形、一條轉很多彎）；轉彎太多不好，在 multi-layer 時每個轉彎可能代表要打一個 via，cost 高，所以通常挑**轉彎比較少**的。
  - 平面上的 maze route 花的時間和 memory 很可觀；往反方向擴散有點浪費，但怕的是障礙物必須從外面繞進來。這個演算法大概在 1960 年代就被提出。
- 投影片問：**traceback 有多個解，選哪個？轉彎少的？多層呢？這個演算法有什麼問題？**

---

## 2. Global routing 與 detailed routing

[10:12]–[17:00]　投影片 Routing、Routing & Layout、Slicing vs Non sliceable；課本 §12.1–12.2

```
placement
   ↓
global routing   • 為每條 net 產生「loose」route
                 • 分配每條 net 經過哪些 routing region，不決定導線的實際 layout
   ↓
detailed routing • 在分到的 routing region 裡找出每條 net 實際的幾何 layout
   ↓
compaction
```
- 投影片的問題：怎麼 model routing region？channel？non-slicing？**能不能從 macro 上面繞過去？** layer assignment 呢？**怎麼產生沒有 DRC error 的導線？**
- 【口頭】這種 building block 的 layout style，通常把 routing 拆成 global routing 和 detailed routing 兩個階段；global route 的虛線就是大致的走法。
- **Routing 與 layout**（同[第 9 支](lecture09_note.md) §4）：
  - 晶片大小可變：哪個 channel 可以拉寬？incremental routing？過去照 slicing 結構，channel 可以調。
  - **晶片大小固定——現在大多數**：每條 net 都先接再 rip-up-and-reroute 修 DRC；或盡量接、有 violation 就留著不接。
  - slicing 的 channel 順序 4, 3, 2, 1；non-slicing 沒辦法這樣排。

---

## 3. Routing constraints

[17:00]–[18:36]　投影片 Routing Constraints；課本 §12.2

**100% routing completion、clean DRC ＋（面積最小？）**，在一組限制下：
- **Placement constraint**：通常是固定的 placement。
- routing layer 的數目；routing blockage。
- **Geometrical constraint**：必須滿足 design rule（圖：線寬 W、間距 S）。
- **Timing constraint**（performance-driven routing）：必須滿足 delay 限制；clock tree routing 要特別考慮。
  - crosstalk？process variation？
- 投影片註：**contact 會影響 pitch**（contact 比線寬大，兩條相鄰的線如果都有 contact，間距要更大）。

---

## 4. 岔題：DRC 規則（EOL、parallel-run spacing）

[18:36]–[28:45]　投影片 Digress To Talk About DRC、Min Spacing and End-Of-Line Spacing Violation Examples（Mentor Graphics）、Fig. 2、Rule Examples (2) – End of line rule

- **對 router 開發是很重要的題目。**
- Mentor Graphics 的例子：congested 區域裡 routing object 之間的 **minimum spacing** 和 **EOL spacing** violation；很多出現在被指定 **NDR（non-default rule）** 的 pin 附近。
  - 【口頭】有一條線比較寬，間距的要求就不一樣。問題是寫 router 時要自己寫 design rule check，而且這些 rule 要和 **signoff 的 DRC rule 一致**。
- **EOL（end-of-line）spacing**（Fig. 2a）：一個金屬的端點，如果寬度小於 eolWidth 就是 EOL；EOL 往前超過 eolSpace、往兩側 eolWithin 距離內，不能有其他金屬（violation region）。
  - 【口頭】線的短邊結束的地方就是 end of line，它會往前看、再往旁邊看。
- **Parallel-run spacing**（Fig. 2b）：兩個金屬有 parallelRunLength（彼此投影重疊的長度）時，有 spacing 要求，值取決於兩個長方形的寬度。
  - 【口頭】這個 rule 要看每家 foundry 給的，據老師所知又變成一張**表格**。
- **更複雜的 EOL rule**：對 object 1、2 套 EOL spacing——因為 object 3 從 edge 1 頂端算起和 parallel length 重疊，所以 1、2 之間要 EOL spacing；object 3 必須留在 parallel halo 外面（halo、parallel length、spacing、length of end、parallel halo）。
  - 投影片：**我們要怎麼檢查它？**
  - 投影片：**從 DRM（design rule manual）的規則，可以猜出哪家 foundry 在某個 node 比較好。**
  - 【口頭】rule 越複雜，表示 foundry 為了保護製造良率越小心；據老師所知，同一個 node，rule 越多的 foundry 在製程上越小心。

---

## 5. Routing 的分類

[28:45]–[29:15]　投影片 Classification of Routing

```
routers ─┬─ global ─────┬─ graph search
         │              ├─ Steiner tree
         │              ├─ hierarchical
         │              └─ iterative
         ├─ detailed ───┬─ general purpose ─┬─ maze
         │              │                   └─ line search
         │              └─ restricted ──────┬─ channel ─┬─ left-edge
         │                                  │           ├─ greedy
         │                                  │           └─ hierarchical
         │                                  ├─ switchbox
         │                                  ├─ over-the-cell
         │                                  └─ river
         └─ specialized ┬─ clock-tree
                        └─ power & ground
```
- 投影片註：track assignment、rip-up-reroute（現在 detailed routing 的主流）。

---

## 6. Lee 演算法

[29:15]–[31:51]　投影片 Maze Router: Lee Algorithm、Lee Algorithm；課本 §12.3.1（p.692–694）

- Lee, "An algorithm for path connection and its application," *IRE Trans. Electronic Computer*, EC-10, 1961。
- 主要討論**單層** routing。
- **優點**：
  - **如果兩個 terminal 之間存在連線，保證找得到**。
  - **保證是最短路徑**。
- **缺點**：
  - dense layout 需要很大的 memory。
  - 慢。
- 應用：path finding、global routing、detailed routing。
- 要處理多層 routing 需要加強。
- 做法：用「**wave propagation**」從 S 找到 T（filling），再 retrace。
- **M × N 格子的時間和空間 complexity：O(MN)（huge!）**；考慮多層更貴。

---

## 7. 減少 memory 與執行時間

[31:51]–[40:00]　投影片 Reducing Memory Requirement、Reducing Running Time

**Akers 的觀察（1967）**：
- 標記 k 的格子，相鄰的格子不是 k − 1 就是 k + 1。
- 所以只要一種標記法，讓每個標記的「前一個」和「後一個」不同，retrace 時就分得出往哪走。
- 方法 1：編碼序列 1, 2, 3, 1, 2, 3, …；狀態有 1, 2, 3, empty, blocked → 要 **3 bits**。
- 方法 2：編碼序列 **1, 1, 2, 2, 1, 1, 2, 2, …**；狀態 1, 2, empty, blocked → **只要 2 bits**。
- 投影片：**取代原本 32／64 bits 的整數標記。**

**減少執行時間**：
- **起點選擇**：選離格子中心**最遠**的點當起點（波前碰到邊界就不會再擴大，擴散面積比較小）。
- **Double fan-out**：從 source 和 target **兩邊同時**擴散（兩個小圓的面積比一個大圓小）。
- **Framing**：只在比 source、target 的 bounding box 大 10–20% 的長方形裡搜尋；失敗的話放大長方形重來。

---

## 8. Multi-terminal net 與 weighted grid

[40:00]–[44:51]　投影片 Connecting Multi-Terminal Nets、Routing on a Weighted Grid、A Routing Example on a Weighted Grid

**連接 multi-terminal net**：
1. 從 source s 擴散到最近的 target。
2. 把路徑上**所有**格子標成 s。
3. 從**所有** s 格子同時擴散到其他格子。
4. 繼續直到所有 pin 都接到。
5. 用 heuristic 再降低 tree 的 cost。
- 投影片問：**什麼時候「美化」routing pattern？**（圖：A–E 五個 pin，先接 A–B，再從整條線擴散接 C…；最後的 tree 有一段可以改得更好看。）

**Weighted grid**：
- 動機：找**更理想**的路徑（不只最短，例如別貼著障礙物走、留空間給別人）。
- weight(grid cell) = 未被擋住的格子邊數 − 1（靠近障礙物的格子權重比較小——⚠️ 投影片圖上靠障礙物的格子是 1、開闊處是 3，照這個定義是越開闊權重越高；所以「更理想」的路徑是指 cost 和最小，會選經過擁擠處的格子？這和直覺相反，投影片沒有多解釋）。
- 例：初始化 cell 權重 → wave propagation（累加權重）→ 第一個波碰到 target → 找其他路徑 → 找到 min-cost path。

---

## 9. Hadlock 演算法

[44:51]–[49:45]　投影片 Hadlock's Algorithm

- Hadlock, "A shortest path algorithm for grid graphs," *Networks*, 1977。
- 用 **detour number** 取代 Lee 的波前標記：
  - **detour number d(P)**：路徑 P 上「**往遠離** target 的方向」走的格子數。
  - MD(S, T)：S、T 的 Manhattan distance。
  - 路徑長度 $l(P) = MD(S, T) + 2d(P)$（每往外走一步，之後就要多走一步回來）。
  - **MD(S, T) 固定 → 最小化 d(P) 就是找最短路徑。**
  - 標記 i 的格子，相鄰未被擋的格子如果是**遠離** T 的方向就標 i + 1，否則標 i。
- 時間空間還是 O(MN)，但**大幅減少搜尋的格子數**。
- **找得到最短路徑。**
- 例：S 往 T 方向的格子都標 0、往外（away）標 1、2…；投影片註：「有點像 line search」（同一個 detour 值的格子會沿直線一路往 T 延伸）。
- 【口頭】有障礙物擋住會停下來，要從 2 那邊走，所以從綠色的線再 branch 出去，可能有好幾條，要看演算法怎麼寫。

---

## 10. Soukup 演算法

[49:45]–[53:30]　投影片 Soukup's Algorithm

- Soukup, "Fast maze router," DAC-78。
- **結合 breadth-first 和 depth-first search**：
  - Depth-first（**line**）search 先朝 target T 直走，直到碰到障礙物或 T。
  - 碰到障礙物時，用 breadth-first（Lee 式）search 在障礙物周圍「**bubble**」。
- 時間空間 O(MN)，但**比 Lee 快 10–50 倍**。
- **找得到一條路徑，但不一定是最短的！**
- 圖：如果 blockage 更大，bubble 要繞更大一圈。

---

## 11. Line search：Mikami-Tabuchi 與 Hightower

[53:30]–[1:00:21]　投影片 Mikami-Tabuchi's Algorithm、Hightower's Algorithm、Features of Line-Search Algorithms

**Mikami-Tabuchi**：Mikami & Tabuchi, "A computer program for optimal routing of printed circuit connectors," IFIP, H47, 1968。
- **每個格點都是 escape point。**
- 從 source 和 target 各畫水平、垂直的 trial line（level 0）；線上每個點（base point）再畫垂直方向的 trial line（level 1）…；直到 source 和 target 的線**相交**（point of intersection）。

**Hightower**：Hightower, "A solution to line-routing problem on the continuous plane," DAC-69。
- **每條線段只有一個 escape point。**
- 如果線平行於被擋住的格子，escape point 放在剛好超過那段障礙物端點的地方。
- 投影片註：**routing 順序會影響結果——第一條 net 的結果會變成第二條 net 的新障礙物。**
- 【口頭】Hightower 不是每個點都取 escape point，所以線段數比較少。

**Line search 的特色**：
- 線由「escape point」產生：每個點都是 escape point（Mikami-Tabuchi）或每條線段一個（Hightower）。
- source 產生線集合 A、target 產生線集合 B，A 的某條線和 B 的某條線相交，路徑就找到了。
- 時間空間 **O(L)**，L 是產生的線段數。
- 也叫 **line probing**；後來發展成 **area probing**——**在 custom routing 用得很多**。
- 【課本補充】課本 §12.3.2 還介紹 **A\*-search**：在 Lee 的 BFS 加上到 target 的估計距離（例如 Manhattan distance）當 heuristic，優先展開「已走 + 估計剩下」最小的格子，仍保證最短但搜尋格子少很多（p.694–697）。

---

## 12. 演算法比較與 multi-layer routing

[1:00:21]–[1:05:36]　投影片 Comparison of Algorithms、Multi-layer Routing

| | Lee | Soukup | Hadlock | Mikami | Hightower |
|---|---|---|---|---|---|
| 類型 | maze | maze | maze | line search | line search |
| Time | O(MN) | O(MN) | O(MN) | O(L) | O(L) |
| Space | O(MN) | O(MN) | O(MN) | O(L) | O(L) |
| 有路徑時一定找得到？ | yes | yes | yes | yes | **no** |
| 路徑最短？ | yes | **no** | yes | **no** | **no** |
| 在 grid 還是 line 上 | grid | grid | grid | line | line |

- **Soukup、Mikami、Hightower 都用了某種 line-search 操作 → 不能保證最短路徑。**

**Multi-layer routing**：
- **3D grid**：波前也往上下層擴散。
- **兩個平面陣列**：
  - 忽略層間 via 的權重；
  - pin 從兩層都能接。
  - 圖：第 1 層、第 2 層各自的標記，合成一條 path（layer-1 實線、layer-2 虛線、via 方塊）。

---

## 13. Net ordering

[1:05:36]–[1:10:36]　投影片 Net Ordering、Net Ordering (cont'd)

- **net 的順序大大影響 routing 結果。**
- 例：應該先 route net b 再 route net a。為什麼？——先 route a 會把 b 擋住；先 route b（短的），a 可以繞過去。
- 排序方法：
  - 依 bounding box 裡的 **pin 數遞增**排序（**pin 多的 net 要等**）。
  - 依**長度遞增**（或遞減??）排序（**短的先**）。
  - 依 **timing criticality** 排序。
  - 例：routing 順序 a(0) → b(1) → d(2) → c(6)（括號是 bounding box 裡的 pin 數）。
- **互相阻擋（mutually intervening）的情況**：a 擋住 b、b 也擋住 a，不管先 route 哪條都失敗，但存在一個 feasible routing——要靠 **rip-up-and-reroute**。

---

## 14. 各種 design style 的 global routing

[1:10:36]–[1:17:00]　投影片 Routing（重複）、Global Routing in different Design Styles、Global Routing in Standard Cell；課本 §12.4

```
global routing
 ├─ full custom   ：flexible channels，最一般的問題   ┐ variable size
 ├─ standard cell ：flexible channels，fixed feedthroughs ┘ → 現在是 fixed size
 ├─ gate array    ：fixed channels                     ┐ fixed size
 ├─ FPGA          ：fixed routing tracks，switchbox constraints ┘
 └─ die-to-die wiring（packaging）
```

**Standard cell 的 global routing**：
- 目標：最小化**總 channel 高度**；**feedthrough** 的分配要在 placement 做還是 global routing 做？
- 高效能設計：最小化**最大線長**、最小化**最大路徑長度**。
- 圖：cell row 之間的 channel、穿過 cell row 的 feedthrough、一條 failed net；綠色格線是「**GRcell 的一種可能切法**」。
- 【口頭】把晶片切成一格一格的 **G-cell（GRcell）**，model 每個 G-cell 的邊界在第一層可以有幾條線通過、pin 在哪裡、水平和垂直方向各有幾個 track；如果不能從 cell 上方 route over，就只能靠 via 的數量。
- 【課本補充】課本把 global routing 建成 **routing graph**：每個節點是一個 tile（G-cell），每條邊是相鄰 tile 的邊界，邊的 **capacity** 是穿過那個邊界的 track 數；可以用 0-1 ILP 最小化最大 congestion（式 12.1），或先解 LP relaxation 再 rounding（p.697–700）。

---

## 15. 【實測】

程式：`experiments/lecture11/maze.py`，輸出：`experiments/lecture11/output.txt`。在一個 8 × 14 的格子（# 是障礙物）上比較 Lee、Hadlock、Soukup。

```
S=(6, 4) T=(3, 11) Manhattan distance MD=10
Lee: shortest length 12, cells labelled 80 of 95 free
   ..............
   ...######.....
   ........#.....
   ..##....#..T..
   ..##....#..**.
   ........####*.
   ....S********.
   ..............
Hadlock: detour d(P)=1  ->  l(P) = MD + 2d = 12; cells expanded 34
Soukup: path length 18 (not guaranteed shortest)
Akers 1,1,2,2 labels along the Lee path: 1122112211221
multi-terminal Lee (4 pins, propagate from ALL tree cells): tree wirelength 24
```

- **Lee** 找到最短 12，但為了找到 T 標記了 95 個空格裡的 80 個——就是投影片說的 memory 和時間問題。
- **Hadlock** 也找到 12：MD = 10，路徑要往右繞過障礙物多走 1 格「遠離」的路（d = 1），12 = 10 + 2×1；只展開 34 個格子，不到 Lee 的一半。
  - 實作上 detour number 只會加 0 或 1，所以用 0-1 BFS（deque：加 0 放前面、加 1 放後面）：

```python
            nd = det[u] + (1 if md(v) > md(u) else 0)      # moving away from T costs a detour
```
（`experiments/lecture11/maze.py:60`）

- **Soukup** 先往 T 直走、撞牆才 bubble，得到 18——比最短多 6，印證「快但不保證最短」。
- **Akers**：沿著 Lee 的路徑，標記變成 1,1,2,2,1,1,2,2…，只要 2 bits；retrace 時從 T 看前一格的標記就知道方向。
- **Multi-terminal**：4 個 pin 照投影片的步驟（每次從整棵樹的所有格子擴散到最近的未接 pin），總線長 24。

---

## 16. 一頁速查表

| 主題 | 重點 |
|---|---|
| 流程 | placement → global routing（region 分配）→ detailed routing（實際幾何）|
| 主流 detailed routing | track assignment + rip-up-and-reroute |
| Constraints | 100% 完成、clean DRC、layer 數、blockage、design rule、timing、crosstalk |
| DRC | min spacing、EOL spacing（eolWidth、eolSpace、eolWithin）、parallel-run spacing（看兩邊寬度，查表） |
| Lee | BFS 波前 + retrace；保證找到、保證最短；O(MN) |
| Akers | 1,1,2,2 標記 → 2 bits |
| 加速 | 起點選離中心遠的、雙向擴散、framing（bounding box 加 10–20%） |
| Multi-terminal | 從整棵樹擴散到最近的 pin |
| Weighted grid | 每格有權重，找 min-cost path |
| Hadlock | detour number；l(P) = MD + 2d(P)；最短、搜尋少 |
| Soukup | line 朝 T 走、撞牆 bubble；快 10–50 倍、不保證最短 |
| Mikami-Tabuchi | 每個點都是 escape point；O(L)；找得到、不保證最短 |
| Hightower | 每段一個 escape point；O(L)；可能找不到 |
| Multi-layer | 3D grid 或兩個平面陣列 |
| Net ordering | pin 少的先、短的先、timing critical 先；互相阻擋要 rip-up-and-reroute |
| Global routing | G-cell、邊 capacity；standard cell：channel 高度、feedthrough |

---

## 17. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| global routing（G-cell、capacity、congestion）、Steiner tree | 第 12 支：routing part 2；Prog-3 的 2D global router |
| channel routing（left-edge、greedy） | 第 14 支：ch8 channel routing |
| clock tree、power & ground routing | 第 14–15 支 |
| pin assignment | 第 13 支 |
| Steiner tree | [第 5 支](lecture05_note.md) |
| BFS、priority queue、shortest path | [第 3 支](lecture03_note.md)、[第 4 支](lecture04_note.md) |
| design rule、DRC | [第 2 支](lecture02_note.md) §31、[第 7 支](lecture07_note.md) §1 |

---

## 附：本講一句話總結

> **Routing 分成 global routing（決定每條 net 經過哪些 routing region）和 detailed routing（在 region 裡畫出真正的線），現在 detailed routing 的主流是 track assignment 加 rip-up-and-reroute，而且 router 必須自己檢查和 signoff 一致的 DRC（min spacing、end-of-line spacing、要查表的 parallel-run spacing——規則越多代表 foundry 越小心）；最基本的 maze router 是 Lee 演算法，用 BFS 波前從 S 擴散到 T 再 retrace，保證找得到也保證最短，但時間空間都是 O(MN)，所以有 Akers 的 1,1,2,2 標記（2 bits）、起點選擇、雙向擴散、framing 等改良，Hadlock 用 detour number（l = MD + 2d）同樣保證最短但搜尋少很多，Soukup 用直線加 bubble 快 10–50 倍但不保證最短（本機實測同一張圖 Lee/Hadlock 都是 12、Soukup 是 18，Hadlock 只展開 Lee 不到一半的格子），line search 的 Mikami-Tabuchi 和 Hightower 只用 O(L) 但不保證最短；multi-terminal net 從整棵樹擴散，net 的順序會影響結果，互相阻擋時要 rip-up-and-reroute。**

---

*筆記依據李炯霆「Electronic Design Automation (1)」Lecture 11-1 | Ch7. Routing 錄影（NTHU OCW，1:17:03；影片無字幕，以 whisper large-v3-turbo 轉錄）、投影片截圖（Unit 5D）、Wang, Chang, Cheng（eds.）*Electronic Design Automation: Synthesis, Verification, and Test* Ch12 §12.1–12.4 整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
