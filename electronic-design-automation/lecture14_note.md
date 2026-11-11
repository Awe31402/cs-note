# 設計自動化（一）第十四支影片完整筆記：Channel routing、clock routing、power/ground routing 與 ECO（ch8 channel, clock, P/G）

> **課程**：Electronic Design Automation (1)（清大 CSR 5302）— 第13講｜ch8.channel_clock_PG（播放清單第 14 支）
> **講者**：李炯霆（JT Li），清大半導體學院，2026 春，NTHU OCW 錄影
> **來源**：YouTube 錄影（2:59:24，無字幕，whisper 轉錄）＋ 投影片截圖（Unit 5E: Channel, Clock, and Power/Ground Routing）＋ 課本 Wang, Chang, Cheng（eds.）*Electronic Design Automation* Ch12 §12.5.1 Channel routing（p.704–）、Ch13 Synthesis of clock and power/ground networks（p.751–）
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、投影片和課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿辨識不清、或影片／投影片／課本說法不一致之處
> - `[h:mm:ss]`：影片時間點
>
> ⚠️ 播放清單標題是「第13講」，在播放清單是第 14 支；檔名依播放清單順序。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Routing 的考量、via pillar、via ladder](#1-routing-的考量via-pillarvia-ladder)
- [2. Routing model 與多層模型](#2-routing-model-與多層模型)
- [3. Channel routing 的名詞與問題](#3-channel-routing-的名詞與問題)
- [4. HCG、VCG 與 left-edge 演算法](#4-hcgvcg-與-left-edge-演算法)
- [5. Dogleg channel router](#5-dogleg-channel-router)
- [6. Robust channel router](#6-robust-channel-router)
- [7. 從 channel routing 到 track assignment](#7-從-channel-routing-到-track-assignment)
- [8. Timing、crosstalk 與 detailed routing](#8-timingcrosstalk-與-detailed-routing)
- [9. Clock routing 問題](#9-clock-routing-問題)
- [10. H-tree、geometric matching、MMM、RGM](#10-h-treegeometric-matchingmmmrgm)
- [11. Elmore delay](#11-elmore-delay)
- [12. Exact zero skew 與 DME](#12-exact-zero-skew-與-dme)
- [13. Power/ground routing](#13-powerground-routing)
- [14. DTCO 與 backside power](#14-dtco-與-backside-power)
- [15. Special routing：bus、spine](#15-special-routingbusspine)
- [16. ECO](#16-eco)
- [17. 【實測】](#17-實測)
- [18. 一頁速查表](#18-一頁速查表)
- [19. 與前後講次的連結](#19-與前後講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
channel routing（舊製程，但演算法值得學）：
   HCG（重疊）、VCG（上下關係）→ left-edge（沒有 vertical constraint 時最佳）
   → constrained left-edge → dogleg router（解 cycle、省 track）→ robust router（weighted interval DP）
   → 現在：trunk／spine routing；GR → track assignment → DR
clock routing：skew 最小、delay 最小
   H-tree、geometric matching、mean & median、RGM → Elmore delay → exact zero skew（tapping point、snaking）、DME
P/G routing：ring、stripe、follow-pin、mesh；via 很關鍵；DTCO；backside power delivery
special：bus、spine（fishbone）；ECO：cell sizing、buffer、metal-only ECO
```

---

## 1. Routing 的考量、via pillar、via ladder

[0:00]–[9:18]　投影片 Unit 5E 首頁、Order of Routing Regions、Routing Considerations、via pillar 圖、Via Pillar／AutoNDR、Wire Engineering and Via Ladders

- 內容：channel routing（舊製程用的，但演算法很值得學）、clock routing、power/ground routing、（bus routing）。閱讀：Chapters 9.3、9.4（Gerez）。
- **Channel routing**：pin 固定在上下兩邊。**Switchbox routing**：pin 固定在相鄰的邊（不是對邊）→ 彈性不大。
- Routing region 的順序與 L-channel（同[第 9 支](lecture09_note.md) §4）。
- **Routing 考量**：
  - terminal 數（兩點 net vs multi-terminal net）
  - net 寬度（power/ground vs signal net）
  - via 限制（stacked vs conventional via；**以前 cut 是正方形，現在可以是長方形，叫 bar via**）
  - 邊界型態（規則 vs 不規則）；layer 數；net 類型（critical vs non-critical）
  - **NDR（non-default routing）規則**：某些 net 有自己的 spacing、width、contact type——依設計而定，通常給 critical net。
- **Via pillar**：平行的金屬線加多個 via 疊成柱子；兩個 via pillar 建在 standard cell 上，再用高層金屬連接（降 R）。
- Via pillar insertion、AutoNDR routing（同[第 12 支](lecture12_note.md) §5）：via 建模和 NDR 會影響 global routing。
- **Wire engineering 與 via ladder**（WikiChip）：用選擇性的 layer 最佳化、buffering、pre-route、via ladder 讓 critical signal 走快的 layer。兩種 via ladder：
  - **High performance**：給推長線的大 buffer。
  - **EM**：給高活動的 gate（例如 clock driver），減輕大 fanout 高活動節點的 electromigration。
  - 投影片問：**via ladder 的 R/C 怎麼抽？是預先 characterize 的嗎？**

---

## 2. Routing model 與多層模型

[9:18]–[18:15]　投影片 Routing Models（grid-based、gridless）、Models for Multi-Layer Routing、Special routing

- **Grid-based**：pitch（同[第 12 支](lecture12_note.md) §8）。
- **Gridless model**：不照格子走的模型；**面積可以更緊湊，但 routing 程式變難**（線寬、線距、via 間距、via 寬都要自己算）。
- **多層模型**：
  - **Unreserved layer model**：任何 net segment 都可以放在任何 layer。
  - **Reserved layer model**：某類 segment 限定在某些 layer。兩層：HV、VH；三層：**HVH、VHV**。
  - **River routing**：單層 routing。
- **Special routing：等電阻（或等長）**——PCB 上的蛇形線（integrasources 的高速 PCB 設計指引）。

---

## 3. Channel routing 的名詞與問題

[18:15]–[21:21]　投影片 Terminology for Channel Routing、Channel Routing Problem；課本 §12.5.1（p.704–）

例：上邊 pin 0 1 4 5 1 6 7 0 4 9 10、下邊 pin 2 3 5 3 5 2 6 8 9 8 7（0 表示沒有 pin）。
- 名詞：terminal、via、**dogleg**、**branch**（垂直段）、**trunk**（水平段）、vertical constraint。
- **Local density** d(i)：穿過第 i 欄的 net 數。例子的 local density：1 3 5 5 4 3 3 3 4 3 2。
- **Channel density**：最大的 local density。**需要的水平 track 數 ≥ channel density。**
- **Channel routing 問題**：
  - 把 net 的水平段分配到 track；
  - 分配垂直段，連接同一 net 在不同 track 的水平段，以及 net 的 terminal 和它的水平段；
  - **不能違反水平和垂直限制**：
    - **水平限制**：兩條 net 的水平範圍重疊（不能放在同一條 track）；
    - **垂直限制**：存在某一欄，上面的 terminal 屬於一條 net、下面的屬於另一條 net（上面那條的 trunk 必須在下面那條之上）；
  - **目標：channel 高度（面積）最小**。

---

## 4. HCG、VCG 與 left-edge 演算法

[21:21]–[39:06]　投影片 HCG、VCG、Basic Left-Edge Algorithm、Example、Constrained Left-Edge

- **HCG（Horizontal Constraint Graph）**：**無向**圖，頂點是 net，net i、j 的範圍重疊就有邊。
- **VCG（Vertical Constraint Graph）**：**有向**圖，邊 i → j 表示 net i 必須在 net j 上面。
- 例：上 1 5 2 0 2 1 1 0 3 4 0、下 3 0 1 2 5 3 4 0 0 2 3，畫出 HCG 和 VCG。

**Basic left-edge**（Hashimoto & Stevens, "Wire routing by optimizing channel assignment within large apertures," DAC-1971）：
- **沒有 vertical constraint**；HV layer model；**不允許 dogleg**。
- 每條 net 當成一個區間；依**左端點**排序；一條一條 route；每條 net 從上到下找第一條放得下的 track。
- **最佳性：沒有 vertical constraint 時，track 數最少**（= channel density）。

```
Algorithm: Basic_Left-Edge(U, track[j])
U: set of unassigned intervals (nets) I1, ..., In;
Ij = [sj, ej]: interval j with left-end sj and right-end ej;
track[j]: track to which net j is assigned.
1  begin
2  U ← {I1, I2, ..., In};
3  t ← 0;
4  while (U ≠ ∅) do
5      t ← t + 1;
6      watermark ← 0;
7      while (there is an Ij ∈ U s.t. sj > watermark) do
8          Pick the interval Ij ∈ U with sj > watermark, nearest watermark;
9          track[j] ← t;
10         watermark ← ej;
11         U ← U − {Ij};
12 end
```

**例子**：I1 = [1, 3]、I2 = [2, 6]、I3 = [4, 8]、I4 = [5, 10]、I5 = [7, 11]、I6 = [9, 12]。
- t = 1：route I1（watermark = 3）、I3（8）、I6（12）
- t = 2：route I2（6）、I5（11）
- t = 3：route I4
- 3 條 track = channel density 3。

**有 vertical constraint 時就不保證最佳**：例 上 0 2 1 0 2、下 1 0 0 3 3，basic left-edge 用 3 條 track，最佳是 2 條。另一個例子：VCG 加上一條紅色的邊就形成 **cycle**——left-edge 無解。

**Constrained left-edge**：同樣的程序，但只挑 **unconstrained** 的區間（在 VCG 裡上面的 net 都已經放好了）。
- 例：I1 = [1, 3]、I2 = [1, 5]、I3 = [6, 8]、I4 = [10, 11]、I5 = [2, 6]、I6 = [7, 9]，VCG：1 → 2 → 5 → 3、6 → 3。
  - Track 1：I1（I3 還不能放）、I6、I4。
  - Track 2：I2；I3 還不能放。
  - Track 3：I5。
  - Track 4：I3。
- 用了 4 條 track（VCG 最長路徑 1→2→5→3 有 4 個點，至少要 4 條），比 channel density 3 多。

---

## 5. Dogleg channel router

[39:06]–[48:39]　投影片 Dogleg Channel Router、Restricted vs Unrestricted Doglegging

- Deutsch, "A dogleg channel router," 13th DAC, 1976。
- left-edge 的缺點 1：**不能處理 constraint cycle** → 用 dogleg 解 cycle（例：上 1 1 2、下 2 0 1，VCG 1 ⇄ 2）。
- 缺點 2：**整條 net 在同一條 track** → 用 dogleg 把一條 net 的不同部分放在不同 track，降低 channel 高度；可能要付出多幾個 via 的代價。
  - 例：上 0 1 2 2 0 3 0 4、下 1 2 0 3 3 4 4 0：不用 dogleg vs 用 dogleg **省 2 條 track，但有 via penalty**。
- 做法：
  - 每條 multi-terminal net 拆成一組兩點 net（subnet，例如 2a、2b、3a、3b）。
  - 兩個參數：**range**（同一 net 連續幾個兩點 subnet 可以放在同一條 track）；**routing sequence**（起始位置和沿 channel 的方向）。
  - 對每個 subnet 用修改過的 left-edge。
- **Unrestricted doglegging**：沒有 pin 的地方也能 dogleg。**Restricted doglegging**：只能在屬於該 net 的 pin 的位置 dogleg。
  - **dogleg channel router 不允許 unrestricted doglegging**：例 上 1 0 2、下 2 0 1——dogleg router 會失敗，但解是存在的（在中間沒有 pin 的欄 dogleg）。

---

## 6. Robust channel router

[48:39]–[1:19:45]　投影片 Robust Channel Router、Interval Graphs、weights、Top-Row／Bottom-Row Net Selection、Maze Routing + Rip-up & Re-route

- Yoeli, "A robust channel router," IEEE TCAD, 1991。
- **在上下兩邊的 track 交替進行**，直到中間；正在做的那一邊叫 **current side**。
- 用 **net weight** 引導 track 的分配：
  - 偏好**對 channel density 有貢獻**的 net；
  - 偏好**在 current side 有 terminal** 的 net；
  - 懲罰在 current side route 會造成 **vertical constraint violation** 的 net。
- 透過 rip-up and re-route 允許 unrestricted dogleg。
- **選 current side 的 net 集合 = interval graph 上的 maximum weighted independent set**。
  - independent set（anti-clique）：一組兩兩不相鄰的頂點。
  - 一般圖是 NP-complete，但 **interval graph 可以用 dynamic programming 有效率地解**。
- **Interval graph**：每個區間一個頂點，重疊的區間之間有邊。**track assignment 等價於找最小的 vertex coloring**（用最少顏色讓相鄰頂點不同色；同色的 net 放在同一條 track）。例：v1–v7 用 3 色。
- **Weight 的計算**：
  1. 區間包含最大密度欄的 net，權重加一個大數 B（鼓勵下一輪降低密度）。
  2. 在 current side 的位置 x 有 terminal 的 net，加上 local density d(x)。
  3. 若把 net 放在 current side 會在第 x 欄造成 vertical constraint violation，減去 K·d(x)，K 在 5 到 10 之間。
- **DP**：net i 的區間 $[x_{i,min}, x_{i,max}]$、權重 $w_i$；從左到右處理，第 c 欄的最佳值 total[c]；**rightmost terminal 在 c 的 net n，若 total[c − 1] < $w_n$ + total[$x_{n,min}$ − 1] 就選它**。

**例子**：上 1 2 3 4 3、下 _ 1 _ 2 4；d(1) = 1、d(2) = 2、d(3) = 2、**d(4) = 3**（net 2、3、4）、d(5) = 2；VCG：V3 → V4 → V2 → V1。B = 1000、K = 5，第一輪做**上面**：
- $w_1 = 0 + 1 + (-5\times2) = -9$（範圍不含最大密度欄；第 1 欄密度 1；放上面會在第 2 欄造成 vertical violation）
- $w_2 = 1000 + 2 + (-5\times3) = 987$（在上面第 2 欄有 terminal）
- $w_3 = 1000 + (2+2) + 0 = 1004$（上面有兩個 terminal）
- $w_4 = 1000 + 3 + (-5\times2) = 993$

| c | total[c] | selected |
|---|---|---|
| 1 | 0 | 0 |
| 2 | max(0, 0 − 9) = 0（net 1 rightmost 在 2） | 0 |
| 3 | 0 | 0 |
| 4 | max(0, $w_2$ + total[1]) = 987 | 2 |
| 5 | max(987, 0 + 1004, 0 + 993) = 1004 | 3 |

- **從右往左往回選，沒有水平衝突**：上面只選 net 3（net 2 和 net 3 重疊）。
- 第二輪做**下面**：$w_1 = 1000 + 2 + 0 = 1002$、$w_2 = 1000 + 2 + (-5\times2) = 992$、$w_4 = 1000 + 1 + (-5\times2) = 991$；total = 0, 1002, 1002, 1002, 1993 → 下面選 net 4 和 net 1。
- 第三輪：把 net 2 放中間會不可行 → 用 **maze routing + rip-up and re-route** 重繞 net 2、4 修好。
- 從 vertical constraint 看，不用 dogleg 要 4 條 track；**加上轉彎可以擠進 3 條**。

---

## 7. 從 channel routing 到 track assignment

[1:19:45]–[1:28:48]　投影片 However, Channel Route Usage Dwindled、GR → TA → DR、Track Assignment、Fig. 6、Another Picture Related To Pin Assignment

- **Channel routing 用得越來越少了**，但 **trunk（spine）routing** 很常用：
  - interval graph 的概念有用；一條 trunk 很難的話可以拆成兩條；常常要考慮多層；用在 memory、custom layout、clock tree routing…（圖：一條 trunk 加上下的 branch，避開 blockage）。
- 現在：**Global routing → Track assignment → Detailed routing（rip-up-reroute）**。
- **Track assignment**：digital SoC routing 在 GR 和 DR 之間做 track assignment。Hai Zhou（Northwestern）ICCAD'02 的 **TrackAssign：GR 和 DR 之間理想的中間步驟**。圖：global cell 和格線、一條 net 在 global cell 裡的標記；**要決定交叉點（cross point）**。Fig. 6：用 lookahead 解 track assignment（區間 a–e 排進 track）。
- 和 pin assignment 有關的圖：assignment 沿著方向進行，從 boundary b − 1 到 boundary b，決定每條 net 在邊界上的交叉點順序，避開 obstacle X1。
  - 【口頭】老師以前的 gate-array routing project 這一步用的是 **linear assignment**；這篇論文用它自己的 heuristic。投影片上的圖有些地方不太清楚（例如 n3b 應該在上面），有興趣可以看論文想想看。
- **track assignment 有 short、DRC error 沒關係，detailed router 用 rip-up-and-reroute（RR）清掉**；是 **area based** 的 RR：一開始區域小，之後逐漸放大（有更多自由度解 DRC error）。**很難，尤其想盡量多修一些 error。**

---

## 8. Timing、crosstalk 與 detailed routing

[1:28:48]–[1:45:33]　投影片 Timing and Crosstalk Driven Area Routing、Fig. 1–4、Coupling Reduction、Aprisa、Detailed Routing（Zroute）、DRC browser、TritonRoute／Dr. CU、Redundant Vias、Layer Assignment、Flip Chip／InFO

- **Timing and crosstalk driven area routing**（Tseng, Scheffer, Sechen）：用在 GR 和 DR 之間；**排 net 順序**、**調整線距**；兩階段：**net order assignment** 和 **space relaxation**；用含 coupling capacitance 的 delay 算每條 critical path；降低 negative slack net 的 delay。MCNC benchmark（線的 aspect ratio 2.0）delay 降 8.4–25%（若完全沒有 interconnect 干擾是 33%）。
  - **Coupling capacitance vs 線距**：最小間距 c 時，AR2.0 是 0.66、AR1.5 0.58、AR1.0 0.49；隔一條 track（3c）時降到 0.44、0.41、0.38。
  - 流程：GR → **timing driven crosspoint assignment**（net ordering：找最 timing critical 的 channel、決定每個邊界的 crosspoint 順序、每個邊界後更新 delay；space relaxation：找最 critical 的 segment、在旁邊加空 track、把鄰近的線推開）→ DR。
- **Coupling reduction**（R. Kastner 碩士論文）：線 i、j 的重疊長度 $l_{ij}$、距離 $d_{ij}$，RC 模型有 $C_{substrate}$、$C_{coupled}$；net A 是敏感線（弱 driver）、net B 是雜訊線（強 driver）——B 轉換時在 A 上造成 glitch 或 delay。coupling-free vs non-coupling-free 的 routing。
- **Aprisa**（Siemens）：Arm 試用時看 Aprisa 怎麼解決：global register placement；用 **GR 和 track assignment 降低 SI**；共同最佳化 buffer／layer／NDR。
- **Detailed routing**：做 design rule checking 和 fixing；rip-up-and-reroute 需要很多專業，要花時間才能做出好的 detailed router。
  - **Synopsys Zroute 的基本流程**（Berkeley EE290C Lecture 26）：已 route 的 power net 和未 route 的 clock tree ＋ technology file → set Zroute options → **route clock nets** → **route signal nets** → postroute optimization → DFM optimization → chip finishing → verify design rules → completed design。
  - DRC error browser 截圖。
- 學校的 detailed router：**TritonRoute**（OpenROAD，Kahng 教授）、**Dr. CU**（同[第 12 支](lecture12_note.md) §15）。
- **Redundant via（double via）**：為良率和可靠度；插入後要重新檢查最大 **via density** rule；**line end extension** 加大延伸距離，緩解 line-end shortening 造成的 cut 對不準。
  - 最新狀況：先進製程的 digital SoC 流程**可能不再用** redundant via insertion，但老師在 routing 步驟還看過；analog layout 流程可能還用（線可以比較寬）。
- **Layer assignment**（Liu, Cai, Zhou, Hong，清華大學（北京），RLC crosstalk minimization）：layer assignment 是處理 crosstalk 的理想步驟——GR 有彈性但不準、DR 準但缺彈性，layer assignment 介於中間兼具兩者。
- 另一種 routing：**flip chip／InFO** 的 global routing 和 track assignment（I/O pin、bump、DT／VD 頂點和邊）。

---

## 9. Clock routing 問題

[1:45:33]–[1:53:39]　投影片 Clock Routing、Setup Time & Hold Time Check、CRP、Clock Tree Routing；課本 §13.1–13.3

- **Setup／hold check**：
$$T_{ck\to q} + T_{prop} + T_{setup} - T_{skew} < T_{period}$$
$$T_{ck\to q} + T_{prop} > T_{hold} + T_{skew}$$
  （[第 6 支](lecture06_note.md) §22 投影片被裁掉的式子，這裡完整出現。）
- 數位系統：**同步**系統（用精確的 clock 達成溝通與 timing）vs **非同步**系統（用 handshake protocol）。
- **Clock skew**：clock 最小和最大到達時間的差。例：到達時間 10、15、20、13、16、9 → skew = 20 − 9 = 11；平衡後全部 20 → skew = 0。
- **CRP（clock routing problem）**：給 routing 平面、一組點 $P = \{p_1, \dots, p_n\}$ 和邊界上的 clock entry point $p_0$，連接每個 $p_i$，使得
  - $\max_{i,j\in P}|t(0,i) - t(0,j)|$（skew）和
  - $\max_{i\in P}t(0,i)$（delay）
  - 都最小。其他：總線長、功耗等。
- 方法：
  - **Pathlength-based**：H-tree（Dhar, Franklin, Wang, ICCD-84；Fisher & Kung, 1982）；geometric matching（Cong, Kahng, Robins, DAC-91）。
  - **RC-delay based**：exact zero skew（Tsay, ICCAD-91）；Lagrangian relaxation（Chen, Chang, Wong, DAC-96）。
- **H-tree、X-tree、mean and median、geometric matching** 都是讓**長度**相等；**zero skew clock routing** 用實際的 interconnect delay 讓 skew 為 0。投影片推薦 vlsibegin 的 CTS 文章（worthwhile）。

---

## 10. H-tree、geometric matching、MMM、RGM

[1:53:39]–[2:01:27]　投影片 H-Tree Based Algorithm、The Geometric Matching Algorithm、H tree / X tree、Method of Mean and Median、RGM

- **H-tree**（Dhar, Franklin, Wang, "Reduction of clock delays in VLSI structure," ICCD-84）：4 個點 p1 (1,1)、p2 (5,1)、p3 (1,5)、p4 (5,5)，entry p0 (3,0)，畫成一個 H；16 個點就遞迴畫 H 的 H。
- **Geometric matching**（Cong, Kahng, Robins, "Matching based models for high-performance clock routing," IEEE TCAD 1993）：
  - n = 2^k 個 clock pin 當成 n 個節點；每個節點自己是一棵 tree、entry point 就是它自己。
  - **n 個點的最小 cost matching 得到 n/2 條線段**。
  - 兩個節點合併後的 entry point 是線段上**兩邊長度相等**的點。
  - 對每層重複。
  - 用 **H-flipping** 再降 skew（也處理邊交叉）。
  - 時間 **O(n² log n)**。
- **X-tree**：斜線版本；圖註：交叉處 crosstalk 和電容的影響不同。
- **Method of mean and median（MMM）：top-down**——找重心、分成兩半；左右各自再找重心、上下分；…最後把各子集連起來、再連各子集的中心。
- **Recursive geometric matching（RGM）：bottom-up**——n 個 sink 做最小 cost geometric matching；找每對的**平衡點（tapping point，讓子樹 zero skew，不一定是中點）**；遞迴。**這個 bottom-up 的方法比 top-down 好。**

---

## 11. Elmore delay

[2:01:51]–[2:15:24]　投影片 Elmore Delay & Skew、Wire Resistance、Wire Models、Elmore Delay: Nonlinear Delay Model、Example、Delay Calculation for a Clock Tree；課本 §13.2

- **導線電阻**：$R = \frac{\rho L}{A} = \frac{\rho}{H}\cdot\frac LW$，$\rho/H$ 叫 **sheet resistance** $R_\square$。

| 材料 | ρ (Ω·m) |
|---|---|
| Silver (Ag) | 1.6 × 10⁻⁸ |
| Copper (Cu) | 1.7 × 10⁻⁸ |
| Gold (Au) | 2.2 × 10⁻⁸ |
| Aluminum (Al) | 2.7 × 10⁻⁸ |
| Tungsten (W) | 5.5 × 10⁻⁸ |

| 材料 | Sheet Res. (Ω/□) |
|---|---|
| n, p well diffusion | 1000–1500 |
| n+, p+ diffusion | 50–150 |
| n+, p+ diffusion with silicide | 3–5 |
| polysilicon | 150–200 |
| polysilicon with silicide | 4–5 |
| aluminum | 0.05–0.1 |

- **Wire model**：分佈式 RC 線的集總近似：**π-model**（最常用，C/2–R–C/2）、T-model（R/2–C–R/2）、L-model。
- **Elmore delay**：deep submicron 時寄生 R、C 主導 delay；**電阻 $r_i$ 要對它下游所有電容充電**；delay ≈ Σ（電阻 × 下游電容）：
$$\delta = \sum_{i=1}^n\left(r_i\sum_{k=i}^n c_k\right) = \sum_{i=1}^n r(n-i+1)c = \frac{n(n+1)}2rc$$
  **delay 隨線長的平方成長。**（板書：r₁(c₁ + … + cₙ) + r₂(c₂ + … + cₙ) + … = rc(n + (n−1) + …)）
- **π-model 例子**（C、D 沒有負載時）：A 到 B：$\delta_{AB} = r_1(c_1/2 + c_2 + c_3)$；B 到 C：$\delta_{BC} = r_2(c_2/2)$；B 到 D：$\delta_{BD} = r_3(c_3/2)$。
- **數值例子**（0.18 µm：$\hat r$ = 0.075 Ω/µm、$\hat c$ = 0.118 fF/µm；$C_C$ = 2 fF、$C_D$ = 4 fF；AB 100 µm、BC 150 µm、BD 200 µm → $c_{AB}$ = 11.8、$c_{BC}$ = 17.7、$c_{BD}$ = 23.6 fF）：
  - $\delta_{BC} = 0.075\times150\,(17.7/2 + 2) = 120$ fs
  - $\delta_{BD} = 0.075\times200\,(23.6/2 + 4) = 240$ fs
  - $\delta_{AB} = 0.075\times100\,(11.8/2 + 17.7 + 2 + 23.6 + 4) = 400$ fs
  - **critical path：$\delta_{AB} + \delta_{BD}$ = 640 fs**
- **Clock tree 的 delay**：RC tree 的點 P，$c_i$ 是 $p_i$ 的電容、$r_i$ 是 $p_i$ 到上一個點的電阻。**子樹電容** $C_i = c_i + \sum_{j\in S_i}C_j$（$S_i$ 是直接下游）；$\delta(i, j)$ 是 $p_i$ 到 $p_j$ 的路徑（不含 $p_i$、含 $p_j$）；**delay $t_{ij} = \sum_{k\in\delta(i,j)}r_kC_k$**。例：$t_{03} = r_0(c_1+c_2+c_3+c_4+c_1^s+c_2^s+c_3^s) + r_2(c_2/2+c_3+c_4+c_2^s+c_3^s) + r_4(c_4/2+c_3^s)$。

---

## 12. Exact zero skew 與 DME

[2:15:24]–[2:35:21]　投影片 Figure 13.23（DME）、Exact Zero Skew Algorithm 1–2、Zero-Skew Computation、Another Solution；課本 §13.3

- **DME（deferred merge embedding）**（課本 Fig. 13.23，Chap-13 p.33）：bottom-up 的 merging 階段——clock source $s_0$、4 個 sink；sink $s_1$、$s_2$ 的 **merging segment** $ms_a$ 上任何一點到 $s_1$、$s_2$ 等距；$ms_b$、$ms_v$ 同理。先記住線段、之後 top-down 再決定實際位置（「deferred」）。
- **Exact zero skew**（Tsay, "Exact zero skew algorithm," ICCAD-91，蔡仁松教授）：**用 delay 決定 tapping point**。要讓 tapping point 到子樹 $T_1$、$T_2$ 葉子的 delay 相等：
$$r_1(c_1/2 + C_1) + t_1 = r_2(c_2/2 + C_2) + t_2$$
  解出
$$x = \frac{(t_2 - t_1) + \alpha l\left(C_2 + \frac{\beta l}2\right)}{\alpha l(\beta l + C_1 + C_2)}$$
  α、β 是單位長度的電阻、電容，l 是連接線長；$r_1 = \alpha xl$、$c_1 = \beta xl$、$r_2 = \alpha(1-x)l$、$c_2 = \beta(1-x)l$。
- **若 x ∉ [0, 1]，需要 snaking（繞線）才能找到 tapping point**（因為 $t_2 - t_1$ 太不平衡）。

**例子**（課本 p.38，α = 0.1 Ω/unit、β = 0.2 F/unit）：sink A (8, 0) $C_A$ = 16F、B (22, 6) $C_B$ = 10F、C (0, 10) $C_C$ = 1F、D (5, 15) $C_D$ = 2F。A、B 合併成 E；C、D 合併成 F；E、F 合併成 G。
- **E**：L(A, B) = (22 − 8) + (6 − 0) = 20；$x = \frac{(0-0) + 0.1\cdot20(10 + 0.2\cdot20/2)}{0.1\cdot20(0.2\cdot20 + 16 + 10)} = \frac{10+2}{4+16+10} = 0.4$；從 A 走長度 8（= 20 × 0.4）→ E = (8 + 2, 0 + 6) = (10, 6)。$t_{EB} = 0.1\cdot12(0.2\cdot12/2 + 10) = 13.44$ ns；$C_E = 16 + 10 + 0.2\cdot20 = 30$F。
- **F**：$x = \frac{2 + 0.2\cdot10/2}{2 + 1 + 2} = 0.6$；長度 10 × 0.6 = 6（從 C 量）→ F = (0 + 5, 10 + 1) = (5, 11)；$t_{FD} = 0.1\cdot4(0.2\cdot4/2 + 2) = 0.96$ ns；$C_F = 2 + 1 + 0.2\cdot10 = 5$F。
- **G**：(F, E) delay 很不平衡（0.96 vs 13.44）；L(F, E) = (10 − 5) + (11 − 6) = 10；tapping point G 只能在 E 上，再讓 E 到 F 的線**繞長**：$t_{EB} = t_{FD} + r(c/2 + C_F)$，$13.44 = 0.96 + 0.1x(0.2x/2 + 5)$ ⟹ **x = 18.28**。投影片註：**這裡的 x 不是 tapping point 公式的 x，只是 G 到 F 的線長**。
- **Snaking**：18.28 − 10 = 8.28 → **4.14 的 zigzag**（往外繞 4.14 再繞回來）。
- **另一個解**（課本 p.42）：snaking 也可以放在別的位置（例如在 (5, 11) 旁邊往右繞 4.14），merging segment $ms_a$ 到 (14.14, 1.86)。
  - 【口頭】老師研究這段時在圖上加了很多說明，圖太擠，所以另外畫了一張比較鬆的圖來解釋；課本說的另一個解，snaking 發生的位置不同。

---

## 13. Power/ground routing

[2:35:21]–[2:44:36]　投影片 Power/Ground Routing、More About P/G Wiring、P/G Meshes、power planning 圖、Pattern-Based Power Network Synthesis、P/G Network Design、set_pg_strategy；課本 §13.4

- P/G 完全用金屬層佈線（寄生小）。兩個步驟：
  1. **建 interconnection topology**：power、ground 兩棵**不交叉**的樹。
  2. **決定線寬**：避免 metal migration、讓電壓（IR）drop 小；耗電多的 module 和高電流密度處要加寬（鋁約 1.5 mA/µm 寬）。（所以面積的 metric？）
- 圖：**grid** vs **interdigitated trees**（VDD 從左、GND 從右，像手指交錯）。
- **現在複雜得多**：standard cell row 有 **P/G follow-pin**、**P/G mesh**、cell 區域外的 **P/G ring**；I/O 區有 outer ring、inner ring。
- **P/G mesh 降低 IR drop**；**via 連接很關鍵**。
- Power planning 圖：power ring、block-level power grid、power stripe、tap array、tap cell／well tie、via array；水平層和垂直層交錯。
- **Pattern-based power network synthesis**（IC Compiler II）：1. 定義 PG region；2. 定義 **pattern**（結構：metal layer、spacing、width…）；3. 定義 **strategy**（P/G topology：把 pattern 套到特定 PG region、voltage area 或特定 power/ground net；不用固定座標，所以 floorplan 改了也能用）；4. compile power network。
- P/G stripe 在 VDD、VSS 間交錯，注意 via array。
- 指令例：
```
icc2_shell> set_pg_strategy ring_strat -core \
    -pattern {{name: ring_pattern} {nets: {VDD VSS}} \
              {offset: {3 3}} {parameters: {M7 10 2 M8 10 2 true}}} \
    -extension {{stop: design_boundary}}
```

---

## 14. DTCO 與 backside power

[2:44:36]–[2:52:54]　投影片 P/G Wiring Affecting Placement, Routability、Logic Density Improvement (I)–(III)（ISPD 2017）、10nm cell power stubs、Backside Power Delivery、Impacts、Examples Of Enhancements For DTCO

- **P/G 線會影響 placement 和 routability。**
- **Logic density improvement (I)：power plan 最佳化**——為了改善 IR，PG via 數隨製程世代增加（縮小 PG pitch 增加 via 數）；但縮小 PG pitch 會吃掉 routing 資源，所以發展出新的 PG 設計方式（2× PG density，讓 cell 擺放更自由）。
- **(II)：power plan 和 cell layout 共同最佳化**——Uniform M1 ✗；Dual M1 ○（PG 設計要對 cell 架構友善，Uniform → Dual）；cell re-design（大 cell 要重新設計以相容 Dual PG 架構）；PG 和 cell 最佳化的 co-design ○。top down 與 bottom up 循環。
- **(III)：推到極限**——**power strap**：PG 下面能放的 cell 少；**power stub**：PG 下面能放更多 cell。**No stagger pin：5 個 access point；stagger pin：6 個 access point**。投影片：**pin access 對 routing 很關鍵，可用 track 越多越好。**
- 10 nm cell 的 Metal1 power stub（multi block level）。
- **Backside power delivery**：電源線從晶圓背面送進來（traditional：電晶體上面是訊號線再上面是電源線；backside：電源線在電晶體下方）。「非常複雜，因此成本比較高」。
- **Backside power 的影響**（imec）：cell library、pin access、P/G wiring、tie-high/low、DRC/LVS/RC、IR-drop、GDS/OASIS 輸出（或 DEF？）…**很多 CAD 程式都要強化**。Dynamic IR drop 圖：conventional front-side PDN 最差、front-side 加 buried power rail 好一點、**back-side PDN 加 buried power rail 和 nTSV 最好**。
  - 【口頭】電源從下面上來，cell library 的設計、pin access 都要重新考慮；所有 EDA 程式都要 enhance。power strap 和 cell pin 的這些設計就是所謂的 **DTCO**。
- **DTCO 的強化例子**：
  - IR recovery：route 之後插入 PG via、PG wire patching；PG grouping（1× → 2× 空 track）；clock net grouping 和 skew 降低 → Cortex-A72 DS 設計同 IR 下**速度 +1%**。
  - **Partial parallel routing**（timing critical net 加平行的低層金屬降 R）、**via pillar insertion**、**layer promotion**（critical net 改走高層金屬降 R）、**AutoNDR routing** → Cortex-A78、A72 速度 +1.5–2%。

---

## 15. Special routing：bus、spine

[2:52:54]–[2:56:00]　投影片 Special Routing、Bus Routing、Spine Routing (Fishbone Routing)、Figure 13.27、Fishbone

- **Bus routing**：EMA-EDA 的 SKILL CAD IC layout automation 截圖、大量平行線轉彎。
- **Spine routing（fishbone routing）**：
  - trunk 要放哪？超過一條 trunk？可以「斷開」嗎？
  - 第二層 branch 放哪？幾個 pin 分成一組？
  - pin 到 trunk；**接到 pin 不容易**。
  - 相關：H-tree、clock routing、non-default routing rule、更多 custom routing 的需求。
- 課本 Figure 13.27：**clock spine 加蛇形線（serpentine）**——兩條 spine，接到 pin 的線故意繞蛇形，讓長度相等。
- **Fishbone**（Fan Mo、Robert K. Brayton，UC Berkeley）：block-level 的 placement 和 routing 方案。**Spine topology**：net 的 output pin 在一條垂直線（trunk）上，所有 input pin 用水平的 branch 接到 spine；給定 pin 位置，net 形狀就完全決定；pin 之間距離是 Manhattan；只要知道 pin 位置就能判斷 routability。用 left-edge 做 row/column routing。
  - 【口頭】Berkeley 有碩士或博士論文做 fishbone routing，後來用在 block level。

---

## 16. ECO

[2:56:00]–[2:59:21]　投影片 Engineering Change Order (ECO)、design iteration 圖、PrimeClosure 圖、Metal Only ECO

- routing 之後抽 RC、做 STA 修 timing error 或改善 timing：
  - cell sizing
  - buffer insertion
  - placement 受影響 → **ECO placement → 位移最小**
  - routing 受影響 → 要 re-route。問題：**這種 ECO 需要叫 global route 嗎？**
  - base layer 和 metal layer 都改了
- 其他方案：**Metal ECO**（Siemens Aprisa blog）：能不能**只改 metal layer、base layer 不動？省時間**；新的 routing 能及時完成嗎？
- **平均每次 design iteration > 1 週**：layout（floorplan、placement、CTS、route/opt、post-route opt）一輪 TAT 2–3 天（> 7 nm）、4–6 天（< 3 nm）；signoff STA；ECO 一輪又 2–3 天／4–6 天。implementation 時 timing met，signoff 時 timing mismatch，ECO recovery 要負擔把差距補回來。
- Synopsys：third-party layout tool + PrimeTime signoff + PrimeECO 約 20 次迭代 → PrimeClosure golden signoff ECO 約 2–3 次 → Fusion Compiler/ICC2 + PrimeClosure 約 1 次。
  - 【口頭】各家都想辦法減少 iteration，EDA 公司都往這邊推。
- **Metal-only ECO**：用 **gate-array cell**——GA filler／decap cell 可以轉換成任何功能 cell（**ECO functional cell**）。圖：ECO 前後加了一個 ECO buffer；ecofiller 變成 ecocell。
  - 【口頭】有點仿照 gate array 的 style：事先多放一些 spare gate（ECO filler），需要時只改 metal 把它接起來。

---

## 17. 【實測】

程式：`experiments/lecture14/channel_clock.py`，輸出：`experiments/lecture14/output.txt`。

```
basic left-edge : {1: [1, 3, 6], 2: [2, 5], 3: [4]}  channel density 3
constrained LE  : {1: [1, 6, 4], 2: [2], 3: [5], 4: [3]}  channel density 3
robust router top row: total = [0, 0, 0, 987, 1004]  selected nets [3]
Elmore: dBC=122 fs  dBD=237 fs  dAB=399 fs  critical=636 fs
E: x=0.40 t_E=13.44 C_E=30   F: x=0.40 t_F=0.96 C_F=5   G: x=1.18
snaking: wire length to F = 18.28 (Manhattan distance 10 -> extra 8.28, zigzag 4.14)
```

- **Basic left-edge**：三條 track {1, 3, 6}、{2, 5}、{4}，和投影片一樣，等於 channel density 3（沒有 vertical constraint 時最佳）。
- **Constrained left-edge**：照 VCG（1→2→5→3、6→3）得到 4 條 track，和投影片一樣，比 channel density 多 1——vertical constraint 讓 left-edge 不再最佳。
- **Robust router 上面那一輪**：total 表 0, 0, 0, 987, 1004，回推只選 net 3，和投影片一致。
- **Elmore**：精確算是 122、237、399、636 fs；投影片的 120、240、400、640 fs 是四捨五入後的值（例如 0.075 × 150 × 10.85 = 122.06）。
- **Zero skew**：E 的 x = 0.4、$t_E$ = 13.44、$C_E$ = 30；F 的 x = 0.4 是從 D 量（= 從 C 量 0.6，和投影片同一點 (5, 11)）、$t_F$ = 0.96、$C_F$ = 5。G 的 tapping x = 1.18 > 1，**落在 [0, 1] 之外，所以要 snaking**；解 $13.44 = 0.96 + 0.1L(0.1L + 5)$ 得 L = 18.28，Manhattan 距離 10，多出 8.28，繞一個 4.14 的 zigzag——和課本例子完全一致。

---

## 18. 一頁速查表

| 主題 | 重點 |
|---|---|
| Routing 考量 | terminal 數、線寬、via（bar via）、layer、critical net、NDR |
| Via pillar／ladder | 平行 via 降 R；high-performance 和 EM 兩種 |
| Gridless | 面積小、程式難 |
| Layer model | unreserved；reserved：HV/VH、HVH/VHV；river（單層） |
| Channel density | max local density；track 數 ≥ density |
| 限制 | 水平：區間重疊（HCG，無向）；垂直：同欄上下不同 net（VCG，有向） |
| Left-edge | 依左端排序、watermark；無 vertical constraint 時最佳 |
| Constrained LE | 只放 VCG 上面都放好的 net；有 cycle 無解 |
| Dogleg router | 拆 subnet、range、routing sequence；解 cycle、省 track；只允許 restricted dogleg |
| Robust router | 上下交替；weighted interval DP（B、d(x)、−K·d(x)）；rip-up-reroute 允許 unrestricted dogleg |
| Interval graph | track assignment = 最小 vertex coloring |
| 現代流程 | GR → track assignment → DR（area-based RR） |
| Crosstalk | coupling cap 隨間距下降；net order + space relaxation；layer assignment |
| Clock skew | max − min arrival；CRP：skew 和 delay 都最小 |
| Clock tree | H-tree、X-tree、MMM（top-down）、geometric matching（O(n² log n)）、RGM（bottom-up，較好） |
| Elmore | Σ r × 下游 C；n(n+1)/2·rc，隨長度平方；π-model |
| Zero skew | r₁(c₁/2 + C₁) + t₁ = r₂(c₂/2 + C₂) + t₂；x ∉ [0,1] → snaking；DME merging segment |
| P/G | 不交叉的樹 + 線寬；ring、stripe、follow-pin、mesh；via 關鍵 |
| DTCO | PG pitch vs routing；power stub、stagger pin；backside power（所有 CAD 都要改） |
| Special | bus、spine／fishbone、serpentine 等長 |
| ECO | cell sizing、buffer；ECO placement 最小位移；metal-only ECO（spare／GA cell） |

---

## 19. 與前後講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| clock tree synthesis（實務） | [第 15 支](lecture15_note.md) |
| global routing、detailed routing、Dr. CU | [第 11](lecture11_note.md)–[12 支](lecture12_note.md) |
| setup／hold、arrival／required time | [第 6 支](lecture06_note.md) §22 |
| floorplan 的 channel routing 順序、L-channel | [第 9 支](lecture09_note.md) §4 |
| H-tree、geometric matching 和 Steiner tree | [第 5 支](lecture05_note.md) |
| compaction 在 channel routing 的應用 | [第 7 支](lecture07_note.md) §10 |

---

## 附：本講一句話總結

> **Channel routing 雖然是舊製程的東西，演算法仍值得學：track 數至少是 channel density，left-edge 依左端點排序、用 watermark 一條條放，沒有 vertical constraint 時最佳，有的話要用 constrained left-edge（照 VCG）、遇到 cycle 就要 dogleg router（拆 subnet）或 robust router（上下交替，用 weighted interval graph 的 DP 選 net，再 rip-up-and-reroute），現代流程則是 GR → track assignment → DR，並用 net 順序、線距、layer assignment 處理 crosstalk；clock routing 要讓 skew 和 delay 都最小，H-tree、geometric matching、MMM、RGM 讓長度相等，Tsay 的 exact zero skew 則用 Elmore delay（Σ 電阻 × 下游電容，隨線長平方成長）解 tapping point，x 落在 [0,1] 外就要 snaking（本機實測課本例子得到同樣的 18.28 和 4.14 zigzag）；P/G routing 從不交叉的樹、ring、stripe 走到 mesh，via 很關鍵，PG 和 cell 的 DTCO、backside power 讓所有 CAD 程式都要強化；最後 routing 完要抽 RC、做 STA、用 cell sizing、buffer 或只改金屬的 metal-only ECO 收斂 timing。**

---

*筆記依據李炯霆「Electronic Design Automation (1)」第13講｜ch8.channel_clock_PG 錄影（NTHU OCW，2:59:24；影片無字幕，以 whisper large-v3-turbo 轉錄）、投影片截圖（Unit 5E）、Wang, Chang, Cheng（eds.）*Electronic Design Automation: Synthesis, Verification, and Test* Ch12 §12.5、Ch13 整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
