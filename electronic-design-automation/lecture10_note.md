# 設計自動化（一）第十支影片完整筆記：Placement——wirelength 估計、min-cut、force-directed、TimberWolf、GA 與 GORDIAN（ch6 placement）

> **課程**：Electronic Design Automation (1)（清大 CSR 5302）— 第10講｜ch6.placement（播放清單第 10 支）
> **講者**：李炯霆（JT Li），清大半導體學院，2026 春，NTHU OCW 錄影
> **來源**：YouTube 錄影（2:59:22，無字幕，whisper 轉錄）＋ 投影片截圖（Unit 5C: Placement，部分引用 IIT Roorkee GIAN 課程、Sung Kyu Lim《Practical Problems in VLSI Physical Design》、Chris Chu 寫的課本 Ch11）＋ 課本 Wang, Chang, Cheng（eds.）*Electronic Design Automation* Ch11 Placement（p.635–680）
> **標記說明**：`【口頭】` 老師口頭補充；`【Q&A】` 學生提問（本講沒有）；`【課本補充】` 課本有、影片沒細講；`【實測】` 本機（Intel i7-13650HX、Ubuntu、Python 3.10、numpy）實際執行的結果；`⚠️` 辨識不清或不一致；`[h:mm:ss]` 影片時間點。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Placement 的定義與目標](#1-placement-的定義與目標)
- [2. 線長估計](#2-線長估計)
- [3. Placement 演算法分類、cluster growth](#3-placement-演算法分類cluster-growth)
- [4. Min-cut placement 與 terminal propagation](#4-min-cut-placement-與-terminal-propagation)
- [5. Force-directed placement](#5-force-directed-placement)
- [6. TimberWolf（simulated annealing）](#6-timberwolfsimulated-annealing)
- [7. Genetic algorithm](#7-genetic-algorithm)
- [8. Analytical placement：quadratic programming](#8-analytical-placementquadratic-programming)
- [9. GORDIAN](#9-gordian)
- [10. GORDIAN 的收尾：partitioning、repartitioning、整體流程](#10-gordian-的收尾partitioningrepartitioning整體流程)
- [11. Detailed placement、多高度 cell、cell edge spacing](#11-detailed-placement多高度-cellcell-edge-spacing)
- [12. 其他 placer、mixed-size、macro placement 指引](#12-其他-placermixed-sizemacro-placement-指引)
- [13. ML 與 placement](#13-ml-與-placement)
- [14. 【實測】](#14-實測)
- [15. 一頁速查表](#15-一頁速查表)
- [16. 與後續講次的連結](#16-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
placement：把固定大小的 cell 擺到晶片上，最佳化線長、routability、timing
線長估計：HPWL（最常用）、clique（2/n 加權）、chain、star、RMST、RSMT、RSA、STST
演算法：constructive（cluster growth、min-cut）、iterative（force-directed）、
        nondeterministic（SA = TimberWolf、GA）、analytical（GORDIAN、RePlAce）
min-cut + terminal propagation（Dunlop-Kernighan）→ 建 row → legalize → pairwise swap
quadratic placement：Qx + d_x = 0；GORDIAN 用 center-of-gravity 限制把 cell 散開
```

---

## 1. Placement 的定義與目標

[1:27]–[15:18]　投影片 Unit 5C 首頁、Placement、Objectives and Constraints、Building Blocks、Standard Cells、More Styles、IIT Roorkee Introduction；課本 §11.1–11.2

- 內容：placement metrics；constructive placement（cluster growth、min cut）；iterative placement（force-directed、simulated annealing、genetic algorithm）。閱讀：Chapter 7.1–7.4、5.8（Gerez）。
- **Placement**：自動為預先設計好的 cell 在晶片上指定正確位置，使某個 cost function 最佳。輸入：一組**固定大小**的 cell／module 和 netlist。考量：routability／congestion、channel density、線長、cut size、performance、熱、I/O pad。
- **目標**：總面積、總線長、穿過切線的線段數。**限制**：要 routable（cell 不重疊、密度不 overflow）；timing（某些線要短於給定長度）。
  - 例：同 8 個 cell，線長 10 vs 12；密度 2（2 條 track）vs 線長較短但要 3 條 track——線長短不一定好 route。
- 不同 design style 有不同 placement 問題：building block（任意形狀、mixed-size macro）、standard cell（排成 row，power、clock 水平穿過 cell；以前要加 feedthrough cell）、gate array。
- Row 的排法：back-to-back、有 routing channel、channel-less。
- 流程：**global placement → detailed placement（legalize cell 位置）**。
- 目標項目：總線長、cut net 數、congestion、signal delay。

---

## 2. 線長估計

[15:18]–[27:51]　投影片 Optimization Objectives – Total Wirelength、Estimation of Wirelength、An Example (For Complete Graph)

同一條 net 的各種估法（IIT Roorkee 例子）：

| 模型 | 長度 |
|---|---|
| **HPWL**（half-perimeter） | 9 |
| Complete graph（clique），$(2/p)\sum d_M(e)$ | 14.5 |
| Monotone chain | 12 |
| Star | 15 |
| RMST | 11 |
| RSMT | 10 |
| RSA（rectilinear Steiner arborescence） | 10 |
| STST（single-trunk Steiner tree，custom routing 常用 trunk） | 10 |

- **首選 HPWL**：$L_{HPWL} = w + h$；比 RSMT 快一個數量級；2、3 pin 的 net 和 RSMT 一樣；真實電路誤差約 8%（Chu, ICCAD 04）。
- 其他：squared Euclidean $\frac12\sum_i\sum_j\gamma_{ij}[(x_i-x_j)^2+(y_i-y_j)^2]$；Steiner tree（貴）；MST（Steiner 的好近似）；complete graph：邊數 $n(n-1)/2 = \frac n2 \times (n-1)$，所以線長 ≈ $\frac2n\sum dist(i,j)$。
  - 例：3 pin net 實際 2 + 3 = 5；clique（歐氏）$(2+3+\sqrt{13})\times\frac23 \approx 5.7$。
  - 4 pin net：semi-perimeter 11、clique × 2/n = 17.5、Steiner 12、spanning 13。
- 製造方式的影響：full custom 可以自由調 aspect ratio 和 channel 高度；semi-custom（gate array、standard cell）尺寸和 channel capacity 固定。
- 理想上 placement 和 routing 應該一起做（P&R），但太複雜；實務上先 placement，用 metric 估線長。

---

## 3. Placement 演算法分類、cluster growth

[27:51]–[37:51]　投影片 Placement Algorithms、Bottom-Up Placement: Clustering、Cluster Growth、Example

- **Placement 是 NP-complete。**
- constructive（位置一旦決定就不改）：cluster growth、min cut；iterative：force-directed；nondeterministic：SA、GA；analytical：Gordian、Gordian-L、RePlAce。常見組合：constructive 得初始解 → iterative 改善 → SA。
- **Clustering（bottom-up）**：從一個 cell 開始，找和它共用 net 的 cell。投影片：K-L 是 top-down，可以結合兩者——先 bottom-up 確保連得緊的 cell 在同一個 partition。
- **Cluster growth（greedy）**：SELECT 和已擺 cell 連接最強的未擺 cell；PLACE 到讓部分 placement cost 最小的 slot。
- 例（排成一列，暫時忽略 primary IO）：連接數 $c_a=3, c_b=1, c_c=1, c_d=1, c_e=4, c_f=3, c_g=3$ → e 最多，放中間 slot 4；a、b、g 和 e 相連，$c_{ae}=2$、$c_{be}=c_{eg}=1$ → a 放 e 旁邊。**tie-breaking 很重要**；之後用**交換**改善。結果之一：density 4、線長 16、最長路徑 6；改善後 density 2、線長 8、最長路徑 2。

---

## 4. Min-cut placement 與 terminal propagation

[37:51]–[1:18:00]　投影片 Top-down Placement: Min Cut、Min-Cut Placement（Breuer）、Algorithm、Quadrature Example、Iterative Improvement、Terminal Propagation（Dunlop & Kernighan）、Sung Kyu Lim 的例子、Creating Rows、Pairwise Swapping、Experimental Results、Remarks；課本 §11.3

- **Top-down min cut**：從整個電路遞迴二分（例如 K-L），得到 min-cut placement。
- Breuer, "A class of min-cut placement algorithms," DAC-77：**quadrature**（中間密度高的電路）、**bisection**（standard cell）、**slice/bisection**（周邊連接多的 cell）。

```
Algorithm: Min_Cut_Placement(N, n, C)   /* N: layout surface; n: #cells; n0: #cells in a slot; C: connectivity */
1 begin
2 if (n ≤ n0) then PlaceCells(N, n, C)
3 else
4   (N1, N2) ← CutSurface(N);
5   (n1, C1), (n2, C2) ← Partition(n, C);
6   Call Min_Cut_Placement(N1, n1, C1);
7   Call Min_Cut_Placement(N2, n2, C2);
8 end
```

- Quadrature 例子：K-L + quadrature，cut cost C1 = 4、C2L = C2R = 2，四組 {2,4,5,7}、{8,12,13,14}、{1,3,6,9}、{10,11,15,16}。
- **Terminal propagation**（Dunlop & Kernighan, IEEE TCAD 1985）：原本的 min-cut 沒考慮**進入一個區域的 terminal 位置**。
  - net s 有 cell 已經在 L1，就把 L1 那些 pin 的中心投影到邊界上成為 dummy cell p，p 會把相連的 cell 拉到 R1（之後 p 一直留在 R1）。
  - p 落在中間 1/3 的範圍內時**不要用它 bias**；net 的 cell 分布在很多組時也不用。
  - **partitioning 要 breadth-first，不能 depth-first**（不然還沒決定的區域無法提供 terminal 資訊）。
  - 投影片：產生 dummy pin 的函式要考慮多種連接型態和 1/3 陰影區；cell 若連到四處很多子區域，就不必產生 bias pin。
- **Sung Kyu Lim 的例子**（16 個 cell a–p、13 條 net，k-clique 加權：3-pin 邊 0.5、2-pin 邊 1，放到 4×4 格）：
  - Quadrature：6 刀，HPBB 線長 **27**（投影片：quadrature partitioning 的程式不好寫）。
  - Recursive bisection + terminal propagation（middle third window）：15 刀，線長 **23**。各刀說明 terminal 怎麼被傳播、把哪些 node 拉到哪邊（例如 cut 3：k、o 連到 n、j，p1 在 window 外，把 k/o 拉到上面；g 連到 j、f、b，p2 把 g 拉到下面）。
- **建 row**：terminal propagation 讓總面積約少 30%；選 α、β（α + β = 1）平衡每列長度；**partition 後要 legalize**。例：電路切成 32 組，每組放一列或拆成兩列，四列 standard cell 設計。
- 放進 row 之後再做 **pairwise swapping**（例：交換 D、A，從 3 條 track 降到 2 條）。
- **Dunlop-Kernighan 結果**：CMOS 晶片 453 net、412 cell。手工：track density 147、feedthrough 184。自動無 TP：313／591（iterative interchange 後 235）；**有 TP：186／182**（interchange 後 152）。在 AT&T Bell Labs 量產使用，**解在最好手工 layout 的 10% 以內**。
- Remarks：也實作了 F-M partitioning——快很多但解不如 K-L；SA partitioning 慢很多，限時的話品質和 F-M 差不多，好實作；想找好方法把某些 cell 固定在某些位置；**terminal propagation 是 CPU 時間的瓶頸**。

---

## 5. Force-directed placement

[1:19:42]–[1:28:18]　投影片 Placement by the Force-Directed Method、Zero-Force Target Location、Force-Directed Placement、Algorithm

- Hanan & Kurtzberg（1972）、Quinn & Breuer（IEEE TCAS 1979）。把 placement 化成解聯立線性方程找**平衡**位置；類比 Hooke's law $F = kd$。
- Cell i 用權重 $w_{ij}$ 的線連到距離 $d_{ij}$ 的 cell j，總力 $F_i = \sum_j w_{ij}d_{ij}$。令 x、y 分量為 0：
$$\hat x_i = \frac{\sum_j w_{ij}x_j}{\sum_j w_{ij}}, \quad \hat y_i = \frac{\sum_j w_{ij}y_j}{\sum_j w_{ij}}$$
- 例（inverter：Vdd 權重 8、in 3、out 10、Gnd 3，都是固定物件）：$\hat x_i = \frac{8\cdot0+10\cdot2+3\cdot0+3\cdot2}{8+10+3+3} = 1.083$、$\hat y_i = 1.50$。
- 可以是 constructive 或 iterative：選「最有利」的 cell p（最大 F、critical cell）放到它的 zero-force 位置；若被 q 佔了：**ripple move**（p 放進去，再算 q 的新位置…）、**chain move**（p 放進去，q 移到相鄰位置…）、或把 p 放到 q 附近的空位。
- 演算法依 connectivity 排序 cell，依序當 seed；target 位置 VACANT（放下鎖住）、SAME AS PRESENT、LOCKED（移到最近空位，AbortCount + 1，超過上限就全部 unlock）、OCCUPIED AND NOT LOCKED（ripple 下去）。投影片問：**一開始很多 cell 還是浮動的怎麼辦？歡迎建議。**

---

## 6. TimberWolf（simulated annealing）

[1:28:18]–[1:32:48]　投影片 Placement by Simulated Annealing、Solution Space、Neighborhood Structure、Cost Function、Annealing Schedule

- Sechen & Sangiovanni-Vincentelli, "The TimberWolf placement and routing package," IEEE JSSC 1985；TimberWolf 3.2（DAC-86）。
- **Stage 1**：module 可在 row 之間和 row 內移動；**允許重疊**；溫度低於某值進 stage 2。**Stage 2**：移除重疊；只在同一 row 內交換相鄰 module。
- Move：M1 把 module 移到新位置；M2 交換兩個 module；M3 改變方向（沿 Y 翻）。先在 M1/M2 選，Prob(M1) = 0.8、Prob(M2) = 0.2；M1 被拒絕時，以 0.1 機率對同一 module 試 M3。
- **Range limiter**：一開始窗口 $(W_T, H_T)$ 包住整顆晶片；溫度降低時窗口縮小，寬高 ∝ log(T)；窗口小到不能跨 row 交換時進 stage 2。
- **Cost** $C = C_1 + C_2 + C_3$：
  - $C_1 = \sum_{nets}(\alpha_i w_i + \beta_i h_i)$（α = β = 1 就是 HPWL；critical net 加大權重；垂直線便宜就用 β < α）
  - $C_2 = \gamma\sum_{i\ne j}O_{ij}^2$（x 方向重疊量）
  - $C_3 = \delta\sum_{rows}|L_r - D_r|$（列長偏離期望列長）——⚠️ 投影片把這項誤寫成「C2 =」。
- Schedule：$T_k = r_kT_{k-1}$，$r_k$ 從 0.8 升到 0.94 再降回 0.8；每個溫度 nP 次嘗試；T < 0.1 停止。

---

## 7. Genetic algorithm

[1:32:48]–[1:34:54]　投影片 Placement by the Genetic Algorithm、Two More Crossover Operations

- Cohoon & Paris, "Genetic placement," ICCAD-86。模仿生物演化：一群隨機 configuration（population），每個個體是一串 symbol（chromosome = 一個解、symbol = gene）；每一代用 **fitness** 評估；挑兩個比較好的 parent 產生 offspring；運算子 **crossover、mutation、inversion**。
- 例：string [aghcbidef] 對應 3×3 格子位置，fitness = $1/\sum w_{ij}d_{ij}$ = 1/85。
- 另外兩種 crossover：cut-and-paste + chain move（從 parent 1 複製一個 cell 和四個鄰居到 parent 2，原位置的 cell 往外推）；cut-and-paste + swapping（複製 k×k 方塊，k 取平均 3、變異數 1 的常態分佈，交換不在兩個方塊裡的 cell）。

---

## 8. Analytical placement：quadratic programming

[1:34:54]–[1:49:24]　投影片 Place all the cells within this bounding box、Analytical Approaches（Chu 寫的課本 §11.5）、Quadratic Programming、Fig. 11.11、Linear vs Quadratic Objective；課本 §11.5（p.653–）

- 原始問題：最小化 $\sum_e c_e \times HPWL_e$，限制所有 cell 不重疊、在 (W, H) 範圍內——**非常難處理**；而且線性線長 $|x_i-x_j| + |y_i-y_j|$ **不可微分**。
- 改用平方歐氏距離：
$$\tilde L = \frac12\sum_{i<j}c_{ij}\left((x_i-x_j)^2+(y_i-y_j)^2\right) = \frac12x^TQx + d_x^Tx + \frac12y^TQy + d_y^Ty + \text{const}$$
  投影片註：鄭仲寬（CK Cheng）、蔡仁松（RenSong Tsay）教授也做過類似的事。
- 課本 Fig. 11.11：movable 1、2、3，fixed 4、5、6：
$$Q = \begin{pmatrix}c_{12}+c_{13}+c_{14}&0&0\\0&c_{21}+c_{24}+c_{25}&0\\0&0&c_{31}+c_{36}\end{pmatrix} - \begin{pmatrix}0&c_{12}&c_{13}\\c_{21}&0&c_{23}\\c_{31}&c_{32}&0\end{pmatrix},\ d_x = \begin{pmatrix}-c_{14}x_4\\-c_{24}x_4-c_{25}x_5\\-c_{36}x_6\end{pmatrix}$$
  投影片：歡迎再加一個 movable node，自己推矩陣比較看看（老師在投影片上手寫展開）。
- $\tilde L$ 是凸且可微 → 最小線長解滿足 **$Qx + d_x = 0$、$Qy + d_y = 0$**（式 11.1）。**忽略不重疊限制時，quadratic placement 就是解線性方程組**；若所有 movable module 都直接或間接連到固定 module，**Q 正定、可逆**。
- **Linear vs quadratic**：A（固定）—B（可動）—C（固定），A、B 之間兩條線 α、β，B、C 之間一條線 γ，總長 l：
  - quadratic：$\varphi_q = 2(l-l_\gamma)^2 + l_\gamma^2$，$\varphi_q' = -4(l-l_\gamma)+2l_\gamma = 0$ → $l_\gamma = \frac23 l$
  - linear：$\varphi_l = l_\alpha + l_\beta + l_\gamma$ → 最佳 $l_\gamma = l$（B 貼著 A）
  - **quadratic 讓很長的 net 變短、讓短 net 稍微變長。**
- 參考：Quadratic Placement Revisited（DAC 1997）、GitHub KangliC/Gordian_Placement、LAPACK、BLAS。

---

## 9. GORDIAN

[1:49:24]–[2:34:06]　投影片 Analytical Placement（Lim）、QP、Problem Definition、Cost Function、Constraints、Problem Formulation、Fig. 11.17–11.18、Adaptec1 Stats、Experimental Results、GORDIAN Placement 1–20/21

- GORDIAN（Kleinhans, Sigl, Johannes, Antreich, IEEE TCAD 1991）；GORDIAN-L（Sigl, Doll, Johannes, DAC91）。**Global optimization**：解一連串 QP；**partitioning**：強制不重疊。
- QP：minimize $\frac12x^TQx + c^Tx$ subject to $Ax \preceq b$。
- 模型：module u 中心 $(x_u, y_u)$，pin 偏移 $(a_{vu}, b_{vu})$，net 中心節點 v；net v 的平方線長 $L_v = \sum_{u\in M_v}[(x_{uv}-x_v)^2+(y_{uv}-y_v)^2]$；minimize $\phi = \frac12\sum_v L_vw_v = X^TCX + d^TX$。
- **Center-of-gravity 限制**：第 l 層把晶片分成 q（≤ 2^l）個區域，區域 p 的中心 $(u_p, v_p)$：$\sum_{m\in M_p}F_mx_m = u_p\sum_{m\in M_p}F_m$（$F_m$ 是面積）→ $A^lX = u^l$，$a_{pm} = F_m/\sum F_m$。矩陣列數 = partition 數、行數 = cell 數。**LQP**：$\min X^TCX + d^TX$ s.t. $A^lX = u^l$。
- Fig. 11.17：固定 I/O 在邊界、只最小化平方線長、忽略不重疊時，所有 cell 擠在中間；Fig. 11.18：加 center-of-mass 限制把 module 散開。
- Adaptec1（約 21 萬 cell、6000 µm × 6000 µm、120 × 120 個 bin、平均每 bin 14.6 gate）：HPBB 隨迭代 34,069,060 → 46,352,680 → 80,783,336 → 98,111,904（散開的代價是線長增加）。
- 實驗（routing 後面積 mm²，GORDIAN／min-cut／annealing）：例如 scb8 51.7／89.2／59.5、scb9 54.0／98.6／80.0；**CPU 時間比 1 : 3 : 300**。

**Sung Kyu Lim 的 GORDIAN 一步步例子**（10 個 movable gate a–j、4 個輸入 w1–w4、4 個輸出 z1–z4，k-clique 每條邊權重 2/k，面積權重相同、area balance 0.5）：
- IO 位置：w1–w3 在左邊 y = 1、2、3，w4 在上 (1, 4)，z1、z2 在下 (2, 0)、(3, 0)，z3、z4 在右 (4, 1)、(4, 2)。投影片：某種意義上是 dataflow 的概念。
- 建 **adjacency matrix**（10×10，movable 之間）、**pin connection matrix**（10×8，movable 對 IO）、**degree matrix**（兩者每列和，例如 a 是 14/6 + 11/6 = 25/6）、**Laplacian = degree − adjacency** 就是 C。
- **Fixed pin vector**：$d_{x,i} = -\sum_j p_{ij}x(p_j)$，例 $d_{x,1} = -(\frac23\cdot0+\frac23\cdot0+\dots+\frac12\cdot2) = -1$；$d_x^T = (-1, 0, -\frac23, -\frac23, -1, -1, 0, -3, -4, -4)$，$d_y^T = (-2, -\frac{13}6, -\frac{25}6, -\frac{25}6, -\frac43, 0, 0, 0, -1, -2)$。
- **Level 0**（無限制，用 MOSEK 解）：$x^T = (0.95, 0.92, 1.21, 1.32, 1.32, 1.61, 1.98, 2.13, 2.59, 2.51)$，$y^T = (1.27, 1.83, 2.48, 2.61, 1.16, 1.45, 1.84, 0.92, 1.41, 2.03)$——實際尺寸的 cell 會重疊。
- **Level 1**：依 x 排序 {b, a, c, e, d, f, g, h, j, i}，α = 0.5 切成 {b, a, c, e, d}、{f, g, h, j, i}，中心 (1, 2)、(3, 2)；$A^{(1)}$ 每列是五個 1/5。解 LQP：$x^T = (0.70, 0.71, 1.17, 1.21, 1.22, 2.17, 3.10, 2.84, 3.56, 3.33)$（投影片：可以看到 e、f 分得更開）。驗證左半重心 = (1.00, 2.00)。
- **Level 2**：再加兩刀得 p1 = {c, d}、p2 = {a, b, e}、p3 = {g, j}、p4 = {f, h, i}；中心 y 是 3.2、1.2（晶片高 4，5 個 cell 依 2:3 切，4 × 3/5 = 2.4 為切線，4 × 3/5 / 2 = 1.2）；解 LQP 得 Level 2 placement（clique 接線圖）。

---

## 10. GORDIAN 的收尾：partitioning、repartitioning、整體流程

[2:34:06]–[2:40:57]　投影片 Summary（GORDIAN 21/21）、Partitioning、Repartitioning、i = 0/29/58/87、Experimental Results、Overview of Gordian Package、Summary of Gordian、Gordian 連結

- **Center-of-gravity 限制**：幫助把 cell 平均散開，同時盯著線長；（在真實尺寸下）消除 cell 之間的重疊。
- **Partitioning**：需要遞迴切割來**解決 global placement 的 module 重疊**；每切一次，global placement 就多兩個 center-of-gravity 限制再解一次。投影片註：**另一種做法是用 density**（例如 RePlAce）。
  - 把 $M_p$ 切成 $(M_{p'}, M_{p''})$：依座標排序，$x_{u'} \le x_{u''}$；面積比 $\alpha = \sum_{u'\in M_{p'}}F_u / \sum_{u\in M_p}F_u \approx 0.5$；cut value $C_p(\alpha) = \sum_{v\in N_C}w_v$——在 α 0.5 附近找 cut 最小的切點（圖：$C_p(\alpha)$ 對 α 的曲線）。
- **Repartitioning**：每切一刀後交換 module 改善 cut size（用 global placement 的位置做 terminal propagation）；並「undo」上一層犯的錯：
```
Procedure repartition(l)
    if overlap exists
        for each r ∈ R(l−1)
            merge-regions(r, r', r'');
            partition(r, r', r'');
        setup-constraints(l);
        global-optimize(l);
    endif
```
- 圖（請參考 gordian.pdf）：i = 0 時 cell 擠在中間，i = 29、58 逐漸散開，i = 87 均勻鋪滿——對應前面 Adaptec1 的 HPBB 越來越大。
- **整體流程**：
```
Procedure Gordian
    l := 1;
    global-optimize(l);
    while (there exists |M_l| > k)       // 還有區域的 cell 太多，要再切
        for each r ∈ R(l)
            partition(r, r', r'');
        l++;
        setup-constraints(l);
        global-optimize(l);
        repartition(l);
    final-placement(l);
endprocedure
```
- **總結圖**：global optimization（最小化線長）⇄ partitioning（module 集合與 placement 區域的切割）互相傳 module 座標和位置限制；區域只剩 ≤ k 個 module 時進 final placement（依 style 的限制）。**Complexity：空間 O(m)、時間 O(m^1.5 log²m)**；final placement 支援 standard cell、macro-cell、sea-of-gates。
- 投影片：**請把 Gordian bring up 起來，並說明它的主要模組**（Prog-2；連結：Quadratic Placement Revisited、GORDIAN 論文、GitHub KangliC/Gordian_Placement 的 project report、ZhenshenQiu/Gordian-VLSI-Placement）。

---

## 11. Detailed placement、多高度 cell、cell edge spacing

[2:40:57]–[2:48:06]　投影片 Detailed Placement、Nowadays Cell Heights Vary、FinFet Coloring Rule Impacting Cell Placement、NanoFlex；課本 §11.6–11.7

- **Detailed placement**：global placement 之後，要解決重疊、得到合法（legal）位置。投影片註：**現在聽到 1.5 倍、2.5 倍的 cell 高度**。
- **ECO placement**：有 gate 改動時，怎麼得到**位移最小**的 placement？
- **Cell 高度不一**（Mixed-Cell-Height Standard Cell Placement, Hung, Chou 等）：(a) 多列高的 standard cell；(b) 多列高 cell 要對齊 power rail（VDD／VSS 交錯，偶數列高的 cell 只能放在特定的列）。
- **FinFET coloring rule 影響 cell placement**：LEF 可以定義 **standard cell edge spacing rule**，foundry 可以規定兩個 cell 邊之間的最小間距（例：ICCAD 2017 contest Problem C）：
```
PROPERTYDEFINITIONS
    MACRO LEF58_EDGETYPE STRING ;
    LIBRARY LEF58_CELLEDGESPACINGTABLE STRING
    "CELLEDGESPACINGTABLE
        EDGETYPE 1 2 0.400
        EDGETYPE 1 1 0.400
        EDGETYPE 2 2 0.000
    ;" ;
END PROPERTYDEFINITIONS
```
  - 每個 cell 的左右邊各定義一個 edge type，例如 MACRO ms00f80：`EDGETYPE LEFT 2 ; EDGETYPE RIGHT 2`。若 A_RIGHT 和 B_LEFT、B_RIGHT 和 C_LEFT 的組合有非零間距，**legalization 時就要遵守，避免 DRC violation**（Tung-Chieh Chen 的說明）。
- **TSMC NanoFlex**（IEDM 2024）：短 cell library 省面積和功耗；**短 cell + 高 cell 混用**在同面積下速度快 15%；選擇性使用高 cell 達到頻率目標——也就是 placer 必須處理混合高度的 cell。

---

## 12. 其他 placer、mixed-size、macro placement 指引

[2:48:06]–[2:54:42]　投影片 Other Good Works、Global Placement in FastPlace、Mixed-Size Placement、Guidelines and recommendations for macro placement

- **FastPlace 2.0／3.0**（Chris Chu，ASP-DAC 2006、2007）；**RePlAce**（Cheng, Kahng, Kang, Wang：Advancing Solution Quality and Routability Validation in Global Placement；GitHub The-OpenROAD-Project/RePlAce——投影片：我們來試著 bring up 這個程式）。
- **FastPlace 的 global placement**：重複 ① 解 convex QP ② 用 iterative heuristic 降線長 ③ 把 cell 散開，直到 cell 平均分布 ④。特色：**hybrid net model**（加速解 QP）、**cell shifting**（好算、收斂快）、**iterative local refinement**（以 linear objective 降線長）。
- **Mixed-size placement**：
  - Jason Cong 等：global placement（每個 cell 大小固定或視為點，用 analytical 方法）→ 移除重疊 → greedy 合法化 standard cell → sliding-window 的 cell 交換降線長。
  - Adya 等：先用任意 black-box standard-cell placer 得初始解 → 用 fixed-outline floorplanner 移除重疊 → 再專注 standard cell（或用 force-directed 得初始解）。
  - Chen 等：multi-packing tree macro placer——**用 dataflow 資訊**加上 design exploration 的自動 floorplanning。
  - Chang／聯發科等：用 **reinforcement learning** 做 chip placement（arXiv 2204.06407）。
  - 投影片：**（paper reading）可以選用 ML 做 floorplanning／placement 的論文研讀報告。**
- **Macro placement 的指引**（LinkedIn 文章）：不要把 macro 擺在 random logic 中間或讓線擠成「Jam」；macro 推到邊角、pin 朝向 random logic；看 floorplanner 顯示的 rat net 檢查連接；和 I/O 有連接的 macro 靠近 I/O；macro 之間留**更寬的 routing channel**；不要把 random logic 切成好幾塊讓線斜穿過 macro。

---

## 13. ML 與 placement

[2:54:42]–[2:59:18]　投影片 Analogy To Chess & Go For Placement、Google's paper、Key Takeaways、Google AI blog、參考連結、Cadence Genus、Modeling Into ML／NN、DAWN webinar、Analog Placement、Course Ordering

- **和棋類比**：狀態數 chess 10¹²³、Go 約 10³⁶⁰、placement > 10^90,000、真實晶片「> 10^???,???，不可能」；而且 placement 沒有輸贏，只有好壞。
- **Google 的論文**：Mirhoseini 等, "A graph placement methodology for fast chip design," *Nature*, 2021。
  - deep RL 在 chip floorplanning 上**超越或追平人類專家**；6 小時內產生 placement（人類要幾週到幾個月）；用在 Google 最新的 TPU。
  - 投影片：**目前只擺 macro，不擺 standard cell。**
  - 做法（Google AI blog）：每次訓練迭代，**RL agent 一次擺一個 macro**，之後 standard cell cluster 用 **force-directed** 方法擺（把電路看成彈簧系統最小化線長）；reward 是近似線長（HPWL）和近似 congestion 的加權平均。例如 pre-trained policy 會把 macro 擺在晶片邊緣，中間留出凸的空間給 standard cell。
- 參考：Google 的 Chip Designing AI 影片、macro placement guidelines、用 ML 預測 congestion／timing 的 macro placement 論文、ML for EDA survey（arXiv 2102.03357）、IEEE CEDA DAWN webinar；Cadence Genus 合成工具內建 physical 資訊（可以看線長、floorplan blockage、估計的 routing）。
- **把 placement 建模成 neural network**：David Pan 教授的 **DREAMPlace**——**NN 訓練和 placement 的類比**：NN 是 $\min_w \sum f(\phi(x_i; w), y_i) + \lambda R(w)$，placement 是 $\min_w \sum WL(e_i; w) + \lambda D(w)$（w 是 cell 座標、D 是密度懲罰）；都用 forward 算 objective、backward 算 gradient，所以能用 deep learning 框架和 GPU 加速。
- **Analog placement**：和 digital 不同，下學期（DA2）再講；UT Austin 2019 年發表 MAGICAL。
- 課程順序：placement 之後，basic logic synthesis／technology mapping 先講（因為有 project），下一個是 **routing**。

---

## 14. 【實測】

程式：`experiments/lecture10/quadratic.py`，輸出：`experiments/lecture10/output.txt`。

```
zero-force x = 1.083  y = 1.5
Q =
 [[ 3. -1. -1.]
 [-1.  3.  0.]
 [-1.  0.  2.]]
 d_x = [ 0.  0. -4.]  d_y = [ 0. -4. -4.]
quadratic optimum  x = [0.9231 0.3077 2.4615]  y = [1.5385 1.8462 2.7692]  Q positive definite: True
zero-force iteration x = [0.9231 0.3077 2.4615]  y = [1.5385 1.8462 2.7692]
quadratic: best B at 1.000 -> l_gamma = 2.000 (2l/3 = 2.000)
linear   : best B at 0.000 -> l_gamma = 3.000 (= l)
```

- inverter 的 zero-force 位置 (1.083, 1.50) 和投影片一致。
- 照課本 Fig. 11.11 的接法（movable 1、2、3，固定 4、5、6，權重都設 1）建 Q 和 $d_x$、$d_y$，Q 的對角線是各點連線權重和、非對角是負的連線權重，和投影片的公式一致；Q 正定，直接解 $Qx = -d_x$。
- **把每個 cell 反覆移到自己的 zero-force 位置，收斂到和 quadratic 最佳解完全一樣的座標**——因為 zero-force 條件 $\sum_j w_{ij}(x_j - x_i) = 0$ 就是 $\partial\tilde L/\partial x_i = 0$。force-directed placement 其實就是在解同一個 quadratic 問題（Gauss-Seidel 迭代）。
- Linear vs quadratic：A 在 0、C 在 3，quadratic 讓 B 停在 1（$l_\gamma = 2 = \frac23 l$），linear 讓 B 貼到 A（$l_\gamma = l$），和投影片推導一致。

---

## 15. 一頁速查表

| 主題 | 重點 |
|---|---|
| 目標 | 線長、routability（密度）、timing |
| HPWL | w + h；最常用；2/3 pin = RSMT；誤差約 8% |
| Clique | 邊權 2/n |
| Cluster growth | 連接最強的先放；tie-breaking；之後交換 |
| Min-cut | 遞迴 K-L；quadrature／bisection／slice |
| Terminal propagation | 外部 pin 投影成 dummy；中間 1/3 不 bias；要 breadth-first；面積約少 30% |
| Force-directed | x̂ = Σw x / Σw；ripple、chain move |
| TimberWolf | 兩階段；M1/M2/M3；range limiter ∝ log T；C = 線長 + 重疊 + 列長 |
| GA | population、fitness、crossover／mutation／inversion |
| Quadratic | Qx + d_x = 0；Q 正定；長線變短 |
| GORDIAN | 解 QP + center-of-gravity 限制 A^l X = u^l，逐層切開、repartition；O(m^1.5 log²m)；比 annealing 快約 300 倍 |
| Detailed placement | legalize；多高度 cell 要對齊 power rail；cell edge spacing rule；ECO 最小位移 |
| 其他 placer | FastPlace、RePlAce（density）、DREAMPlace（NN 框架 + GPU）；Google RL 擺 macro |

---

## 16. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| Gordian／RePlAce bring-up | Prog-2（[第 1 支](lecture01_note.md) §13） |
| congestion、placement blockage | [第 12 支](lecture12_note.md) §13 |
| routing | [第 11](lecture11_note.md)–[12 支](lecture12_note.md) |
| min-cut、terminal propagation、K-L | [第 8 支](lecture08_note.md) |
| SA、B\*-tree（macro） | [第 8 支](lecture08_note.md)、[第 9 支](lecture09_note.md) |
| HPWL、Steiner tree | [第 5 支](lecture05_note.md) |

---

## 附：本講一句話總結

> **Placement 把固定大小的 cell 擺上晶片，以 HPWL（w + h，2、3 pin 時等於 RSMT）估線長，同時顧 routability 和 timing；方法有 cluster growth、遞迴 K-L 的 min-cut（加上 terminal propagation 後在 AT&T 做到手工 layout 的 10% 內）、用 x̂ = Σwx/Σw 找零力位置的 force-directed、兩階段加 range limiter 的 TimberWolf 退火、genetic algorithm，以及 analytical 的 quadratic placement：平方線長讓問題變成解 Qx + d_x = 0，但 cell 會擠在中間，所以 GORDIAN 逐層加上 center-of-gravity 限制把 cell 散開，速度約是 annealing 的 300 倍（本機實測：反覆把 cell 移到零力位置會收斂到同一個 quadratic 最佳解）；之後 detailed placement 要處理多高度 cell 與 cell edge spacing，而 FastPlace、RePlAce、DREAMPlace 和 Google 的 RL macro placement 是後續的發展。**

---

*筆記依據李炯霆「Electronic Design Automation (1)」第10講｜ch6.placement 錄影（NTHU OCW，2:59:22；whisper large-v3-turbo 轉錄）、投影片截圖（Unit 5C）、Wang, Chang, Cheng（eds.）*Electronic Design Automation* Ch11 整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
