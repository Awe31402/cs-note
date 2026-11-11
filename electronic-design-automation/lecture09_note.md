# 設計自動化（一）第九支影片完整筆記：Floorplanning——slicing、normalized Polish expression、shape curve 與 B*-tree（ch5 floorplanning）

> **課程**：Electronic Design Automation (1)（清大 CSR 5302）— 第9講｜ch5.floorplanning（播放清單第 9 支）
> **講者**：李炯霆（JT Li），清大半導體學院，2026 春，NTHU OCW 錄影
> **來源**：YouTube 錄影（2:23:03，無字幕，whisper 轉錄）＋ 投影片截圖（Unit 5B: Floorplanning，頁尾 Chang, Huang, Li, Lin, Liu）＋ 課本 Wang, Chang, Cheng（eds.）*Electronic Design Automation* Ch10 Floorplanning（Tung-Chieh Chen、Yao-Wen Chang 著，p.575–625）
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、投影片和課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿辨識不清、或影片／投影片／課本說法不一致之處
> - `[h:mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Floorplanning 是什麼](#1-floorplanning-是什麼)
- [2. Early layout decision 與 floorplanning 問題](#2-early-layout-decision-與-floorplanning-問題)
- [3. Leaf／composite cell、slicing floorplan、slicing tree](#3-leafcomposite-cellslicing-floorplanslicing-tree)
- [4. Slicing 的好處：channel routing 的順序](#4-slicing-的好處channel-routing-的順序)
- [5. Skewed slicing tree 與 Polish expression](#5-skewed-slicing-tree-與-polish-expression)
- [6. Normalized Polish expression](#6-normalized-polish-expression)
- [7. Simulated annealing：三種 move 與 cost](#7-simulated-annealing三種-move-與-cost)
- [8. 面積計算、incremental 更新、annealing schedule](#8-面積計算incremental-更新annealing-schedule)
- [9. Wong-Liu 演算法](#9-wong-liu-演算法)
- [10. Shape curve 與 floorplan sizing](#10-shape-curve-與-floorplan-sizing)
- [11. Non-slicing：wheel、order 5、polar graph](#11-non-slicingwheelorder-5polar-graph)
- [12. B*-tree](#12-b-tree)
- [13. B*-tree 的 packing、perturbation 與各種 module](#13-b-tree-的-packingperturbation-與各種-module)
- [14. 結果、fixed outline、bus](#14-結果fixed-outlinebus)
- [15. 總結](#15-總結)
- [16. 【實測】](#16-實測)
- [17. 一頁速查表](#17-一頁速查表)
- [18. 與後續講次的連結](#18-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
floorplanning：決定 block（hard／soft）的位置和形狀；目標：面積、線長、routability
slicing floorplan ── slicing tree ── Polish expression（post-order，長度 2n−1）
   └ normalized（沒有連續相同的 H/V）↔ skewed slicing tree ↔ 唯一的 floorplan
   └ Wong-Liu SA：M1 交換 operand、M2 反轉 chain、M3 交換 operand/operator；cost = A + λW
   └ shape curve：hard block 是階梯、soft block 是雙曲線；V 合併寬度相加、H 合併高度相加
non-slicing：wheel（order 5）、polar graph（longest path 得面積）
   └ B*-tree：左小孩 = 右邊相鄰、右小孩 = 正上方同 x；O(n) packing；contour 算 y
```

---

## 1. Floorplanning 是什麼

[0:00]–[7:00]　投影片 Unit 5B 首頁、Apple A12、Floorplanning、Very Often；課本 §10.1（p.575–580）

- 內容：floorplan 基礎、**slicing** floorplan 的 normalized Polish expression、**non-slicing** floorplan 的 B\*-tree。閱讀：Chapters 8 and 5.6（Gerez）。
- 圖：PowerPC 604、Pentium 4 的 floorplan；Apple A12（[第 2 支](lecture02_note.md) §1）——EDA plays a critical role!
  - 【口頭】A12 這張圖是老師以前的老闆在 Stanford EE292A 上課秀的；同學將來做 chip design 的 floorplanning 時可以趁機了解一下這些 block 怎麼擺、怎麼連。
- **Floorplanning**：partitioning 之後得到：
  - 面積和形狀都確定的 block（**rigid／hard block**）
  - 面積大概知道、形狀沒限定的 block（**flexible／soft block**）
  - 描述 block 之間連接的 **netlist**
- 目標：找出所有 block 的**位置**；決定 soft block 的形狀和所有 block 的 pin 位置。
- 流程：partitioning → **floorplanning／placement（／pin assignment）** → routing。
- **常常 floorplanning 和 placement 兩個詞混用。** 真要區分的話，floorplanning 比較是處理 **block／macro**（含 soft 的），placement 處理**數百萬個 standard cell**（global placement、detailed placement）。

---

## 2. Early layout decision 與 floorplanning 問題

[7:00]–[12:39]　投影片 Some Early Layout Decision Examples、Early Layout Decision Methodology、Floorplanning Problem、Floorplan Design

- **Floorplanning 跟著 schematic（或 block diagram）走**：圖上 M1、RB、RC、RA、M2、ALU、Cont 的電路圖和對應的 floorplan。投影片：**Samsung DRAM 用這種方法**。
  - 【口頭】老師知道有些 memory design house（例如三星）用 schematic 來 guide floorplan。
- **Early layout decision methodology**：
  - IC 本質上是二維的；在設計早期就考慮這點有助於得到好的設計。
  - **floorplanning 提供早期 feedback**：早點想 layout 可能會建議有價值的架構修改；也幫助估計**導線造成的 delay**。
  - floorplanning 很適合 **top-down** 的設計策略（**step-wise refinement**，軟體設計也這麼做）。
  - 但 floorplanning 假設 layout 有**彈性**：存在能調整形狀和 terminal 位置的 cell。

**Floorplanning 問題**：
- 輸入：一組 block（hard 或 soft）、hard block 的 pin 位置、netlist。
- 目標：**最小化面積**、減少（critical）net 的**線長**、**最大化 routability**（最小化 congestion）、決定 soft block 的形狀等。
- 圖：一個面積最佳的 floorplan（1–7 塞滿）vs 一個不是最佳的（有空白）。投影片問：white space？re-orientation（轉向）？怎麼估線長？congestion 呢？

**Floorplan design**：module 寬 x、高 y；面積 A = xy；**aspect ratio** $r \le y/x \le s$；可以**旋轉**；有 module connectivity（圖：a–f 之間的連線和權重）。

---

## 3. Leaf／composite cell、slicing floorplan、slicing tree

[12:39]–[16:21]　投影片 Floorplanning Concepts、Slicing Floorplan + Slicing Tree

- **Leaf cell（block／module）**：階層最底層的 cell，不包含其他 cell。
- **Composite cell**：由 leaf cell 或 composite cell 組成的 cell；**整個 IC 是最高層的 composite cell**。
- **Slicing floorplan**：composite cell 的子 cell 都是用**水平或垂直的二分切割**得到的。
  - 可以用 **slicing tree** 表示：除了最上層，每個 cell 都有 parent；composite cell 有 children。
  - slicing floorplan 又叫 **floorplan of order 2**。
- 投影片：**H：horizontal cut、V：vertical cut——和課本的定義不同！！**
  - ⚠️ 投影片這裡的 slicing tree 是多叉的（一個 H 下面有好幾個子 cell），課本和後面的 Polish expression 用的是二元樹；另外有些書把「H」定義成上下堆疊的運算、有些定義成水平切線，名稱容易搞混。後面 Polish expression 的定義是：**ijH = i 在 j 下面（水平切線分開）、ijV = i 在 j 左邊（垂直切線分開）**。

---

## 4. Slicing 的好處：channel routing 的順序

[16:21]–[35:24]　投影片 Unit 5E（channel／switchbox routing）、A Good Merit Of Slicing Structure、Any Example Of Non-Slicing Structure、Order of Routing Regions and L-Channels、Why Slicing Is Attractive?

（老師先借 Unit 5E 的一張投影片說明 channel routing 和 switchbox routing。）
- **Channel routing**：pin 固定在上下兩邊，router 決定左右兩邊的出口位置。
- **Switchbox routing**：pin 固定在 ≥ 3 邊。

**Slicing 的一個好處**（floorplan E、D、C、B、A，channel 1–4）：
- 先 route channel 4，A、B 的區域就能 pack；再 route channel 3，C 和 (A+B) 的區域決定；再 channel 2 得到 D、E 的區域；最後 route channel 1。
- **channel routing 的順序是 4, 3, 2, 1**（slicing tree 的 post-order 反過來，由下往上）。每次 route 的 channel 兩側 pin 都已經固定、只有兩端是浮動的，都是單純的 channel routing。

**Non-slicing 的例子**（wheel 形狀）：slicing 的切線沒辦法一刀切到底，會產生 **switchbox routing**（pin 固定在 switchbox 的邊界）→ switchbox routing 很難，可能留下沒接上的 net。

**Routing 區域的順序與 L-channel**：
- (a) 照 1、2、3 的順序 route，沒有衝突。
- (b) 沒有任何順序能避免衝突。
- (c) (b) 的情況可以用 **L-channel** 解決。
- (d) L-channel 可以拆成一個 channel 加一個 switchbox。
- 【口頭】四、五十年前確實有人講 L-shape router，但後來用得不多，因為有些不方便——例如 pin 都固定在某一邊時拉不起來，沒有好的演算法。

**為什麼 slicing 有吸引力？channel routing 比較簡單。**
- **Variable size**（晶片大小可以變）：以前設計者會照 slicing 結構，channel 高度可以調。但不是每顆晶片都能照 slicing 結構；incremental routing 怎麼處理？
- **Fixed size**（晶片大小固定）——**現在大多數是這樣**。兩種接線風格：
  - 每條 net 都先接上，再用 **rip-up-and-reroute** 修 DRC violation；
  - 盡量接，有 violation 的 net 就留著不接（open）。

---

## 5. Skewed slicing tree 與 Polish expression

[35:24]–[50:06]　投影片 Skewed Slicing Tree、Solution Representation、Example 10.5、Redundant Representations；課本 §10.2.1（p.582–）

- **Rectangular dissection**：用有限條水平和垂直線段把一個長方形切成有限個不重疊的長方形。
- **Slicing structure**：可以透過反覆把長方形水平或垂直切開得到的 rectangular dissection。
- **Slicing tree**：二元樹，每個內部節點是一條垂直切線或水平切線，每個葉子是一個基本長方形。
- **Skewed slicing tree**：**沒有任何節點和它的右小孩相同**（連續的垂直切線由右到左做、連續的水平切線由上到下做）。
- 圖：non-slicing floorplan、slicing floorplan（1–7）、一棵 skewed slicing tree、另一棵 non-skewed slicing tree——**一個 slicing floorplan 可以有好幾棵 slicing tree**。

**Polish expression**（投影片：請看課本 Chap 10，Tung-Chieh Chen、YW Chang 著）：
- $E = e_1e_2\dots e_{2n-1}$，$e_i \in \{1, 2, \dots, n, H, V\}$，是長度 **2n − 1** 的 Polish expression iff：
  1. 每個 operand j（1 ≤ j ≤ n）在 E 裡**恰好出現一次**；
  2. **balloting property**：每個前綴 $E_i = e_1\dots e_i$ 裡，**operand 數 > operator 數**。
- 例：`1 6 H 3 5 V 2 H V 7 4 H V`（長度 13）：在第一個 V 的位置 operand = 4、operator = 2；在倒數第二個 H 的位置 operand = 7、operator = 5。投影片：「歡迎畫出它的 floorplan」——[1 上面 6] | [2 上面是 (3 | 5)] | [7 上面 4]。
- **Polish expression ↔ post-order traversal**（slicing tree 的後序走訪）。
- **ijH：長方形 i 在 j 的下面；ijV：長方形 i 在 j 的左邊。**
- 例：$E = 16H2V75VH34HV = 16+2*75*+34+*$（把 H 看成 +、V 看成 \*，就是運算式的後序表示）。floorplan：左邊是 [7 5] 在上、[6 上 1, 2] 在下；右邊是 4 在 3 上面。

**課本 Example 10.5**：二元樹的 post-order 得到 $E = 12H34H5HV$。

| | 1 | 2 | H | 3 | 4 | H | 5 | H | V |
|---|---|---|---|---|---|---|---|---|---|
| operand 數 | 1 | 2 | 2 | 3 | 4 | 4 | 5 | 5 | 5 |
| operator 數 | 0 | 0 | 1 | 1 | 1 | 2 | 2 | 3 | 4 |

- 每一欄 operand 數都大於 operator 數 → 滿足 balloting property；沒有連續相同的 operator → **normalized**。長度 9 = 2×5 − 1。floorplan：左邊 2 在 1 上面（12H）、右邊 5 在 4 在 3 上面（34H5H），兩者並排（V）。

**Redundant representations**：floorplan「1 | (2 下、3 上) | 4」有兩種表示：
- $E = 123H4VV$：**non-skewed**（V 的右小孩又是 V）
- $E = 123HV4V$：**skewed**
- non-skewed 的情況就是 expression 裡出現 `…HH…` 或 `…VV…`（某個 H 節點是另一個 H 節點的右小孩）。
- 投影片問：**怎麼消除這種模稜兩可的表示？**

---

## 6. Normalized Polish expression

[50:06]–[53:45]　投影片 Normalized Polish Expression、How Do We Decide If A Slicing Structure Is Good

- Polish expression $E = e_1e_2\dots e_{2n-1}$ 叫 **normalized** iff E **沒有連續相同類型的 operator**（H 或 V）。
- **給一個 normalized Polish expression，就能建出唯一的 slicing structure。**（所以 normalized ↔ skewed slicing tree ↔ slicing floorplan 一一對應。）
- 例：$E = 16H2V75VH34HV$ 是 normalized。
- **怎麼判斷 slicing structure 好不好？** 除了面積，能不能快速估總線長？congestion 呢？

---

## 7. Simulated annealing：三種 move 與 cost

[53:45]–[1:08:54]　投影片 Neighborhood Structure (For Simulated Annealing)、Effects of Perturbation、Cost Function、流程、Figure 10.11；課本 §10.2.2

**Neighborhood structure**：
- **Chain**：`HVHVH…` 或 `VHVHV…`（連續的 operator）。在 `1 6 H 3 5 V 2 H V 7 4 H V` 裡的 chain：H、V、HV、HV。
- **Adjacent**：1 和 6 是相鄰 operand；2 和 7 是相鄰 operand（中間隔著 operator 也算）；5 和 V 是相鄰的 operand 與 operator。
- **三種 move**：
  - **M1（Operand Swap）**：交換兩個相鄰的 operand。
  - **M2（Chain Invert）**：把某條 chain 反相（V = H、H = V）。
  - **M3（Operator/Operand Swap）**：交換相鄰的一個 operand 和一個 operator。

**Perturbation 的效果**：$12V4H3V \xrightarrow{M1} 12V3H4V \xrightarrow{M2} 12H3H4V \xrightarrow{M3} 12H34HV$（每一步對應的 floorplan 都畫出來）。
- 問題：move 之後 balloting property 還成立嗎？
  - **M1、M2 一定 OK。**
  - **要檢查 M3！** 拒絕「不合法」的 M3。
- **檢查 M3**：假設 M3 交換 operand $e_i$ 和 operator $e_{i+1}$（1 ≤ i ≤ k − 1），則**交換不違反 balloting property iff $2N_{i+1} < i$**，$N_k$ = 前 k 個元素裡的 operator 數。
  - 為什麼：交換後前 i 個元素是 $e_1\dots e_{i-1}e_{i+1}$，裡面 operator 有 $N_{i+1}$ 個、operand 有 $i - N_{i+1}$ 個，balloting 要求 $i - N_{i+1} > N_{i+1}$。

**Cost function**：$\phi = A + \lambda W$
- A：包住所有 block 的最小長方形面積
- W：總線長，$W = \sum_{ij} c_{ij}d_{ij}$；$c_{ij}$：block i、j 之間的連線數；$d_{ij}$：基本長方形 i、j 的**中心到中心距離**
- λ：使用者指定的參數

**流程**：floorplan 的 slicing tree → Polish expression → perturb expression → evaluate cost function。

**課本 Figure 10.11（Example 10.7）**：(a) $12H4H35VH$ →Op1→ (b) $12H3H45VH$ →Op2→ (c) $12H3H45HV$ →Op3→ (d) $12H34H5HV$。
- 投影片：**老師相信當時開發了幾個 debugging 工具：給 slicing structure 產生 expression；給 expression 畫出對應的 slicing structure。**

---

## 8. 面積計算、incremental 更新、annealing schedule

[1:08:54]–[1:22:15]　投影片 shape 小例子、Area Computation for Hard Blocks、Incremental Computation of Cost Function、Annealing Schedule

- 小例子：block 5（寬 1、高 2）和 block 6（寬 2、高 2）：並排是寬 3、高 2；轉一下可以變寬 4、高 2 等不同組合。
- **Hard block 的面積計算（允許旋轉）**：
  - 例：Inst-1（w = 2, h = 3），旋轉後是 (3, 2)。
  - 每個節點存一串可能的 (w, h)；**V 合併**（左右並排）：寬 = v + w、高 = max(u₁, u₂)；**H 合併**（上下堆疊）：寬 = max(v, w)、高 = u₁ + u₂。
  - 由下往上算到 root，root 的清單裡挑面積最小的。例：最後的 floorplan 寬 2 + 1 + 2、高 2 + 3：[2 5 6 / 1 3 4]。
  - 線長 cost：中心到中心的連線長度。
- **Incremental computation**：
  - 每個 move 只讓 Polish expression 有小小的改變。
  - **每個 move 最多只要更新 slicing tree 的兩條路徑。**
  - M1：$12H34V56VHV \to 12H35V46VHV$（4、5 交換，只有它們到 root 的兩條路徑要重算）。
  - M2：$12H34V56VHV \to 12H34V56HVH$；M3：$12H34V56VHV \to 123H4V56VHV$（「歡迎畫出對應的 floorplan」）。
- **Annealing schedule**：
  - 初始解：$12V3V\dots nV$（全部排成一列）。
  - $T_i = r^iT_0$，i = 1, 2, 3, …；**r = 0.85**。
  - 每個溫度試 kn 次 move（k = 5–10）。
  - 終止條件：被接受的 move < 5%、溫度夠低、或時間用完。

---

## 9. Wong-Liu 演算法

[1:22:15]–[1:26:51]　投影片 Algorithm: Wong-Liu Floorplanning (P, ε, r, k)、Slicing Floorplan Design by Simulated Annealing

```
1  begin
2  E ← 12V3V4V...nV;   /* initial solution */
3  Best ← E; T0 ← −Δavg / ln(P); M ← MT ← uphill ← 0; N = kn;
4  repeat
5      MT ← uphill ← reject ← 0;
6      repeat
7          SelectMove(M);
8          Case M of
9          M1: Select two adjacent operands e_i and e_j; NE ← Swap(E, e_i, e_j);
10         M2: Select a nonzero length chain C; NE ← Complement(E, C);
11         M3: done ← FALSE;
12             while not (done) do
13                 Select two adjacent operand e_i and operator e_{i+1};
14                 if (e_{i−1} ≠ e_{i+1}) and (2N_{i+1} < i) then done ← TRUE;
13'                Select two adjacent operator e_i and operand e_{i+1};
14'                if (e_i ≠ e_{i+2}) then done ← TRUE;
15             NE ← Swap(E, e_i, e_{i+1});
16         MT ← MT + 1; Δcost ← cost(NE) − cost(E);
17         if (Δcost ≤ 0) or (Random < e^{−Δcost/T})
18         then
19             if (Δcost > 0) then uphill ← uphill + 1;
20             E ← NE;
21             if cost(E) < cost(best) then best ← E;
22         else reject ← reject + 1;
23     until (uphill > N) or (MT > 2N);
24     T ← rT;   /* reduce temperature */
25 until (reject/MT > 0.95) or (T < ε) or OutOfTime;
26 end
```

- 第 14、14' 行的 $e_{i-1} \ne e_{i+1}$、$e_i \ne e_{i+2}$ 保證交換後還是 **normalized**（不會產生連續相同的 operator）。
- $T_0 = -\Delta_{avg}/\ln(P)$：讓一開始 uphill move 被接受的機率約為 P。
- 相關論文：
  - Wong & Liu, "A new algorithm for floorplan design," DAC-86——slicing floorplan。
  - Wong & Liu, "Floorplan design for rectangular and L-shaped modules," ICCAD'87——也處理 L 形 module。
  - Wong, Leong, Liu, *Simulated Annealing for VLSI Design*, pp. 31–71, Kluwer, 1988。
- SA 的四個元素在這裡分別是什麼？solution space（normalized Polish expression）、neighborhood（M1–M3）、cost（A + λW）、schedule（r = 0.85、kn 次）。

---

## 10. Shape curve 與 floorplan sizing

[1:26:51]–[1:43:27]　投影片 Shape Curve (For Soft Macros)、Floorplan Sizing – Example（IIT Roorkee）、Figure 10.30、Addition for Vertical Abutment、Deriving Shapes of Children、Sizing Algorithm、Feasible Implementations；課本 §10.2.3

**Shape curve（soft macro）**：
- flexible cell 可以有不同的 aspect ratio。
- 寬 x 和高 y 的關係：xy = A，y = A/x——shape function 是**雙曲線**（實際上常說「實現需要面積 A」，即 xy ≥ A）。
- 太細長的 cell 沒意思、也常常做不出來：shape function 是**雙曲線加兩條直線**（aspect ratio $r \le y/x \le s$，即 y = sx 和 y = rx 兩條線之間）。
- leaf cell 由離散的電晶體組成，假設 shape function 是連續的雙曲線不實際。
- 極端情況：cell 是 **rigid** 的，在 floorplanning／placement 時只能旋轉和鏡射。圖：2 × 4 inset cell 的 shape function（階梯形）。

**Floorplan sizing 例子**（IIT Roorkee GIAN 課程，2018）：
- Step 1：建 block 的 shape function：Block A 3 × 5（或轉成 5 × 3）、Block B 2 × 4（或 4 × 2）→ $h_A(w)$、$h_B(w)$ 都是階梯。
- Step 2：算最上層 floorplan（**垂直堆疊**）的 shape function $h_C(w) = h_A(w) + h_B(w)$，corner point 是 **3 × 9、4 × 7、5 × 5**。
- **最小的 top-level floorplan（vertical composition）是 5 × 5**（面積 25）。
- 【實測】驗證，見 [§16](#16-實測)。

**Figure 10.30**：(a) 雙曲線 h = A/w；(b) 加上最小寬／高限制；(c) **piecewise linear**（階梯）。
- 一般來說，**piecewise linear** 函數可以近似任何 shape function；函數改變方向的點叫 **corner（break）point**。

**Vertical abutment 就是 shape function 相加**：$h_3(w) = h_1(w) + h_2(w)$。例：I 5 × 5、II 5 × 7、III 3 × 9、IV 4 × 7。投影片：「We are doing these for cost functions」。
- **推回子 cell 的形狀**：選定 composite cell 的最小形狀，就決定了它子 cell 的形狀（minimal area of parent → consequences for children's shapes）。
- **Floorplan sizing**：有 flexible cell，同一個硬體單元可以有不同形狀；所以可以為每個 leaf cell 挑合適的形狀，讓整個 floorplan 在某種意義下最佳（例如面積最小）。

**Slicing floorplan 的 sizing 演算法**：
1. 所有 leaf cell 的 shape function 用 piecewise linear 表示。
2. 走訪 slicing tree，**由下往上**算所有 composite cell 的 shape function。
3. 挑最上層 cell 想要的形狀；因為 shape function 是 piecewise linear，找最小面積時**只要檢查 break point**。
4. 把選擇的結果**由上往下**傳回 leaf cell。
- **slicing floorplan 的 sizing 是 polynomial time**；**non-slicing floorplan 是 NP-complete**。

**Feasible implementations**（陰影是可行區域）：
- (a) rigid、固定方向：$x_i \ge a, y_i \ge b$
- (b) rigid、可轉向：$x_i \ge a, y_i \ge b$ 或 $x_i \ge b, y_i \ge a$
- (c) flexible、固定方向：$x_i \ge a, y_i \ge b, x_iy_i \ge A$
- (d) flexible、可轉向

**Cost function** 再看一次：φ = A + λW。投影片問：**能不能把 congestion 放進 cost function？怎麼放？**

---

## 11. Non-slicing：wheel、order 5、polar graph

[1:43:27]–[1:56:27]　投影片 Wheel or Spiral Floorplan、Order-5 Floorplan Examples、General Floorplan Representation: Polar Graphs、About Sizing Of Floorplans

- **Wheel（spiral）floorplan**：**不是 slicing**。**wheel 是最小的 non-slicing floorplan**（5 塊）。
- 把 floorplan 限制在 slicing 很合理：讓 floorplanning 演算法簡單很多。
- 把 wheel 和它的鏡像當成 operator，就得到 **order 5** 的階層描述。
- **Order-5 例子**：floorplan A–L，tree 的 root 是一個 wheel，五個小孩是 E、V(B, C)、wheel(H, G, L, V(J, K), F)、H(I, D)、A。wheel 有兩種方向（5-4-1-3-2 和 5-2-1-4-3）。

**Polar graph（一般 floorplan 的表示，non-slicing）**：
- **頂點：channel segment；邊（權重）：cell／block／module（的尺寸）**。
- **vertical polar graph**（頂點 h₁–h₇ 是水平 channel，邊是 block 的高度）和 **horizontal polar graph**（頂點 v₁–v₇ 是垂直 channel，邊是 block 的寬度）。
- **Sizing**：floorplan 的高和寬 = vertical、horizontal polar graph 的 **longest path**。用 §6.4.1 的演算法（[第 7 支](lecture07_note.md) §7 的 DAG longest path）可以在 **O(n)** 時間算出，因為 polar graph 恰好有 n 條邊。

---

## 12. B*-tree

[1:56:27]–[2:02:24]　投影片 B\*-Tree for Compacted Non-Slicing Floorplans、比較表、Table 10.6；課本 §10.2

- Chang et al., "B\*-tree: A new representation for non-slicing floorplans," DAC-2000（B\*-tree 用 **ordered binary tree** 表示 compacted floorplan）。
  1. 把 module 往**左**、往**下**壓緊（O-tree 的想法）。
  2. 建 ordered binary tree（B\*-tree）（更嚴格的規則請看論文）：
     - **左小孩：右邊最低的相鄰 block**（$x_j = x_i + w_i$）
     - **右小孩：正上方第一個、x 座標相同的 block**（$x_j = x_i$）
- compacted non-slicing floorplan 和它導出的 B\*-tree 之間有**一對一的對應**。
- 投影片：**admissible placement**：已經 compacted，任何 block 都不能再往下或往左移。
- 【Q&A】（[1:55:17]，逐字稿只錄到轉述）同學問：如果上面剛好讓 b₁₀ 可以往這邊推呢？老師答：那時就不一定有從 b₉ 指到 b₁₀ 的邊。照這些規則能建出 B\*-tree；有了 compacted floorplan，很快就能用工具產生 B\*-tree，反過來也一樣。

**和其他表示法比較**（投影片引用 CiteSeer 上的論文）：

| 表示法 | 優點 | 缺點 |
|---|---|---|
| Sequence pair／BSG（bounded-sliceline grid） | 能處理 non-slicing；表示很有彈性 | 耗時；solution space 大；編碼成本高；SP 和 placement 互轉較難；SP 不能直接處理 soft module；BSG 有冗餘 |
| O-tree | 能處理 non-slicing；solution space 較小；和 placement 互轉線性時間；編碼 bit 少 | 彈性不如 BSG／SP；tree 結構不規則、較難實作；要編碼 module 序列；插入位置受限，可能偏離最佳 |
| **B\*-tree** | 能處理 non-slicing；二元樹、有效率；**能彈性處理 hard、pre-placed、soft、rectilinear module**；編碼成本小；除了 soft module，tree 轉 placement 線性時間；可以 incremental 算面積；solution space 較小 | 彈性不如 BSG／SP |

**課本 Table 10.6：floorplan 表示法比較**：

| 表示法 | Solution space | Packing time | Flexibility |
|---|---|---|---|
| Normalized Polish expression | O(n!2^{3n}/n^{1.5}) | O(n) | Slicing |
| Corner block list | O(n!2^{3n}) | O(n) | Mosaic |
| Twin binary sequence | O(n!2^{3n}/n^{1.5}) | O(n) | Mosaic |
| O-tree | O(n!2^{2n}/n^{1.5}) | O(n) | Compacted |
| **B\*-tree** | O(n!2^{2n}/n^{1.5}) | O(n) | Compacted |
| Corner sequence | ≤ (n!)² | O(n) | Compacted |
| Sequence pair | (n!)² | O(n²) | General |
| BSG | O(n!C(n², n)) | O(n²) | General |
| Transitive closure graph | (n!)² | O(n²) | General |
| TCG-S | (n!)² | O(n lg n) | General |
| Adjacent constraint graph | O((n!)²) | O(n²) | General |

---

## 13. B*-tree 的 packing、perturbation 與各種 module

[2:02:24]–[2:12:09]　投影片 B\*-tree Packing、To Improve → Perturbations、B\*-Tree Perturbation、Computing y-coordinates、Coping with Pre-placed Modules、Rectilinear Modules、Soft Modules

**Packing**：
- **x 座標由 tree 結構決定**：左小孩 $x_j = x_i + w_i$；右小孩 $x_j = x_i$。例：n₇ 是 n₀ 的左小孩 → $x_7 = x_0 + w_0$；n₁ 是 n₀ 的右小孩 → $x_1 = x_0$。
  - 「n₇ 沒有左小孩」→ 它右邊沒有相鄰的 block。
  - 「不需要 n₃ → n₆（就算 x₃ = x₆）」：n₆ 不必是 n₃ 的右小孩。
- **y 座標呢？** 要看下面已經擺的 block 有多高——用 **contour**。
- **Computing y-coordinates**：新插入 module 的 y 座標，直接算要對 module 數線性的時間；用 **horizontal contour**（記錄目前上緣的輪廓）可以把每次算 y 座標降到**均攤 O(1)**。

**Perturbation**（SA 裡改 B\*-tree）：
- Op1：rotate 一個 module；Op2：flip 一個 module；Op3：把一個 module 移到別的地方；Op4：交換兩個 module。
- 有 API 可以快速更新 B\*-tree。投影片附 ntur.lib 的論文和張耀文教授的 boundary B\*-tree 論文連結。
- Y.-W. Chang 版本：op1 rotate macro；op2 **delete & insert**；op3 swap 2 nodes；op4 **resize soft macro**。例：Op3(n₁, n₇) 交換兩個節點、Op2(n₁₁, n₆) 把 n₁₁ 刪掉插到 n₆ 下面。

**Pre-placed module**（位置固定的 module）：
- 如果有 module 在 $b_i$ 前面或下面，讓 $b_i$ 不能放在固定位置 $(x_i^f, y_i^f)$，就把 $b_i$ 和 $D_i = \{b_j \mid (x_j, y_j) \le (x_i^f, y_i^f)\}$ 裡最接近 $(x_i^f, y_i^f)$ 的 module 交換。
- 可以 incremental 更新面積：例如 b₀、b₇、b₈、b₁₁、b₉、b₁₀、b₁（在 tree 的 DFS 順序中排在 b₂ 前面）交換後位置不變。（圖：b₆ 是 pre-placed module。）

**Rectilinear module**：
- 把 rectilinear module 切成長方形的 sub-module（L 形的四種切法）。
- 保留 sub-module 的 **location constraint**：例如右邊的 sub-module 必須是 B\*-tree 裡的左小孩。
- 必要時把 sub-module 對齊。
- 處理時把一個 module 的所有 sub-module 當成一個整體。

**Soft module**：
- 簡單的做法：改變 soft module 的形狀去對齊相鄰的 module（圖：M10 依上下左右邊界 $T_i$、$B_i$、$L_i$、$R_i$ 調整形狀）；一個一個處理。
- 更多：Step 1：改變新插入的 soft module（例如 b₀）的形狀；Step 2：改變其他 soft module 的形狀（依 contour）。

---

## 14. 結果、fixed outline、bus

[2:12:09]–[2:21:48]　投影片 Perturbations & Solutions、B\*-tree Based Floorplanner、Figure 10.35、How About Fixed-Outline Constraints?、How About Buses?

**結果**：
- Cost = $\alpha\frac{A}{A_{norm}} + (1-\alpha)\frac{W}{W_{norm}}$（$A_{norm}$ = 平均面積）。
- **ami49**：面積 36.74 mm²、dead space 3.53%、CPU 0.25 分鐘（SUN Ultra 60）；最佳值 35.445 mm²。也能處理長方形、L 形、T 形 module。
- **B\*-tree based floorplanner**：ISPD98 ibm01，12,752 個 cell、247 個 macro，$A_{max}/A_{min}$ = 8416（Courtesy of Tung-Chieh Chen）。
  - 【口頭】這是陳東傑先生他們當時的貢獻，最大和最小 block 面積差到這麼多倍。
  - ⚠️ 老師口頭講的中文名字和[第 1 支](lecture01_note.md)致謝的 Tung-Chieh Chen 是同一人，但逐字稿兩次寫法不一致（陳同杰／陳東傑），中文名以英文 Tung-Chieh Chen 為準。
- 課本 Figure 10.35：MCNC ami49 在 fixed-outline 比例 1、2、3、4 下的 floorplan。

**Fixed-outline constraint**：
- 一組 block 總面積 A、最大 dead space 比例 Γ，建一個 aspect ratio 為 R\*（高／寬）的 fixed outline：
$$H^* = \sqrt{(1+\Gamma)AR^*}, \qquad W^* = \sqrt{\frac{(1+\Gamma)A}{R^*}}$$
- floorplan F 的 cost：
$$\Phi(F) = \alpha A + \beta W + (1-\alpha-\beta)(R - R^*)^2$$
- Fig. 3：兩個 feasible floorplan，虛線是 fixed outline（R\* = 0.5）；floorplan 的 aspect ratio R 分別是 (a) 0.5、(b) 1。
- 這對應前面說的「現在大多數晶片尺寸是固定的」。

**Bus**：B\*-tree 加 bus constraint（bus 放在最上面兩層）：
- 例：block b₁、b₂、b₃ 要讓一條水平 bus u 通過，bus 寬 t：$y_{max} - y_{min} \ge t$。
- (b) 插入 **dummy block** 調整位置，bus（H、t、{b₁, b₂, b₃, b₄}）就能滿足。
- Fig. 8：right-skewed subtree 和對應的 feasible 垂直 bus $u = \langle V, t, \{b_0, b_4, b_5\}\rangle$。
- Fig. 10：兩條水平 bus u = {b₀, b₄}、v = {b₂, b₃} 重疊；插入 dummy block D₂ 後得到沒有 bus 重疊的 feasible floorplan。

---

## 15. 總結

[2:21:48]　投影片 Summary So Far

- floorplan 的表示法
- slicing 結構
- 各種 representation
- B\* tree——NTU EDALab 下載頁（eda.ee.ntu.edu.tw）：NTUplace mixed-size placer、Interconnect-driven multilevel floorplanner（IMF）、B\*-tree based placement/floorplanning、TCG、TCG-S、multilevel routing、two-pass bottom-up routing、microfluidic biochip testcases、obstacle-avoiding rectilinear Steiner tree 等。
- 還有其他演算法嗎？

---

## 16. 【實測】

程式：`experiments/lecture09/floorplan.py`，輸出：`experiments/lecture09/output.txt`。

**(1) Polish expression 檢查**：

```
16H35V2HV74HV   polish=True  normalized=True
16H2V75VH34HV   polish=True  normalized=True
12H34H5HV       polish=True  normalized=True
123H4VV         polish=True  normalized=False
123HV4V         polish=True  normalized=True
12HH3V          polish=False normalized=False
slide perturbation sequence (M1, M2, M3):
  12V4H3V: polish=True normalized=True
  12V3H4V: polish=True normalized=True
  12H3H4V: polish=True normalized=True
  12H34HV: polish=True normalized=True
  M3 operand/operator swap on 12H3H4V at i=2: 1H23H4V  2N_(i+1)<i and normalized -> False
  M3 operand/operator swap on 12H3H4V at i=4: 12HH34V  2N_(i+1)<i and normalized -> False
  M3 operand/operator swap on 12H3H4V at i=6: 12H3HV4  2N_(i+1)<i and normalized -> False
  M3 operator/operand swap H<->4 on 12H3H4V: 12H34HV  polish=True normalized=True
```

- 投影片的 expression 都是 Polish；redundant 例子裡 non-skewed 的 123H4VV 不是 normalized、skewed 的 123HV4V 是，和投影片一致。
- 投影片 M1→M2→M3 序列每一步都還是 normalized Polish expression。
- 在 12H3H4V 上做「operand 在前」的 M3，三個位置都被 $2N_{i+1} < i$ 或 normalized 條件擋掉（例如 1H23H4V 的前綴「1 H」違反 balloting）；投影片實際用的是「operator 在前」的那種 M3（H 和 4 交換，對應 Wong-Liu 演算法的 13'、14' 行）。

**(2) Shape function**：

```
shape A: [(3, 5), (5, 3)]  shape B: [(2, 4), (4, 2)]
vertical stack (H) corner points: [(3, 9), (4, 7), (5, 5)]
side by side   (V) corner points: [(5, 5), (7, 4), (9, 3)]
```
- 垂直堆疊的 corner point 3 × 9、4 × 7、5 × 5 和 IIT Roorkee 例子一致；最小面積 25。

**(3) Wong-Liu 風格的 SA**（8 個隨機 hard block，cost 只算面積）：

```
8 random hard blocks, total block area 62: SA best bounding area 64 (dead space 3.1%)  E = 15H82HV6V47V3VH
```
- 從 12V3V…8V（排成一列）開始，用 M1/M2/M3（M3 檢查 balloting 和 normalized），最後 dead space 3.1%。

**(4) B\*-tree packing**：

```
B*-tree packing: {'b0': (0, 0), 'b3': (4, 0), 'b4': (5, 0), 'b1': (0, 3), 'b2': (0, 5)}  bounding box 8 x 8
```
- b₀（4×3）在原點；左小孩 b₃ 放在 b₀ 右邊 x = 4；b₃ 的左小孩 b₄ 在 x = 4 + 1 = 5；右小孩 b₁ 放在 b₀ 正上方 x = 0、y 由 contour 得到 3；b₂ 在 b₁ 上方 y = 5。

---

## 17. 一頁速查表

| 主題 | 重點 |
|---|---|
| Floorplanning | 擺 block（hard／soft）；目標：面積、線長、routability、soft block 形狀 |
| vs placement | floorplan 處理 block/macro；placement 處理大量 standard cell |
| Slicing | 反覆水平／垂直二分；order 2；channel routing 順序容易（post-order） |
| Non-slicing | wheel 是最小的；order 5；會產生 switchbox |
| Polish expression | 長度 2n − 1；每個 operand 一次；balloting：前綴 operand > operator |
| ijH／ijV | i 在 j 下面／i 在 j 左邊 |
| Normalized | 沒有連續相同 operator ↔ skewed slicing tree ↔ 唯一 floorplan |
| SA moves | M1 交換相鄰 operand；M2 反轉 chain；M3 交換相鄰 operand/operator（檢查 2N_{i+1} < i） |
| Cost | φ = A + λW；W = Σ c_ij d_ij（中心距離） |
| Schedule | 初始 12V3V…nV；T_i = 0.85^i T_0；每溫度 kn 次；接受 < 5% 停 |
| Incremental | 每個 move 最多改兩條路徑 |
| Shape curve | soft：雙曲線 + aspect ratio；hard：階梯；V 合併寬相加、H 合併高相加 |
| Sizing | slicing：polynomial（bottom-up 算、只看 break point、top-down 傳回）；non-slicing：NP-complete |
| Polar graph | 頂點 = channel、邊 = block；longest path 得寬高，O(n) |
| B\*-tree | 左小孩 = 右邊相鄰（x + w）、右小孩 = 正上方（同 x）；O(n) packing；contour 算 y（均攤 O(1)） |
| B\*-tree perturbation | rotate、flip／delete-insert、move、swap、resize soft |
| 特殊 module | pre-placed（交換）、rectilinear（拆成子 module + location constraint）、soft（改形狀對齊） |
| Fixed outline | H\* = √((1+Γ)AR\*)、W\* = √((1+Γ)A/R\*)；Φ = αA + βW + (1−α−β)(R−R\*)² |

---

## 18. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| floorplan 之後的 standard cell placement | 第 10 支：placement |
| channel routing、switchbox routing | 第 11–12 支：routing；第 14 支：channel routing |
| pin assignment | 第 13 支：ch5 pin assignment（和 floorplanning 同章） |
| simulated annealing | [第 8 支](lecture08_note.md) §11；第 10 支：TimberWolf placement |
| longest path（polar graph sizing） | [第 7 支](lecture07_note.md) §7 |
| Hw-3：floorplan representation、B\*-tree packing | [第 1 支](lecture01_note.md) §13 |

---

## 附：本講一句話總結

> **Floorplanning 在 partitioning 之後決定 hard／soft block 的位置和形狀，目標是面積小、線長短、好繞線；slicing floorplan 可以用 slicing tree 表示，它的 post-order 就是長度 2n−1、滿足 balloting property 的 Polish expression，而沒有連續相同 operator 的 normalized Polish expression 和 skewed slicing tree、slicing floorplan 一一對應，所以 Wong-Liu 用 M1（交換相鄰 operand）、M2（反轉 chain）、M3（交換 operand 與 operator，要檢查 2N_{i+1} < i 和 normalized）在這個空間做 simulated annealing，cost 是 A + λW，每個 move 只要重算 slicing tree 的兩條路徑；hard block 的 shape function 是階梯、soft block 是雙曲線加 aspect ratio 限制，V 合併寬度相加、H 合併高度相加，slicing floorplan 的 sizing 是 polynomial（本機實測 3×5 和 2×4 堆疊得到 3×9、4×7、5×5 三個 corner point），non-slicing 則是 NP-complete；non-slicing floorplan 最小的是 wheel，可以用 polar graph 的 longest path 算大小，而 B*-tree（左小孩是右邊相鄰的 block、右小孩是正上方同 x 的 block）用 O(n) packing、contour 算 y 座標，能彈性處理 pre-placed、rectilinear、soft module，還能加上 fixed outline 和 bus 的限制。**

---

*筆記依據李炯霆「Electronic Design Automation (1)」第9講｜ch5.floorplanning 錄影（NTHU OCW，2:23:03；影片無字幕，以 whisper large-v3-turbo 轉錄）、投影片截圖（Unit 5B）、Wang, Chang, Cheng（eds.）*Electronic Design Automation: Synthesis, Verification, and Test* Ch10 整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
