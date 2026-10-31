# Advanced Computer Architecture 第五十六支影片完整筆記：機器學習硬體架構（二）：七重迴圈數學表示法、Tiling、Halo Pixels 與四種 Stationarity 映射（Chapter 14, Part II）

> **課程**：Advanced Computer Architecture — Chapter 14: Architectures for Machine Learning (Part II) -- Mathematical representations（播放清單第 56 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（1:06:22）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch14** §14.2.1–14.2.2（p.712–717）
> **章號**：影片 Chapter 14 = 課本 Ch14。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、`gcc` 11.4 AVX2/FMA）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕把「ifmaps / C ifmaps / K ofmaps / ofmap(s)」寫成「if Maps／CIF maps／kof Maps／wave map(s)／voice map」、「$H \times W$（H cross W）」聽成「h plus W」、「tiling / tile size」寫成「styling／trial size」、「zero pad」寫成「zero path」、「PEs / a PE」寫成「peas／a p」、「out-of-order processor」寫成「out of order process」、「halo pixels」寫成「hello pixels」、「ifmap row / filter row」寫成「if map brew／filter rule」、「1D convolution」寫成「one day convolution／only convolution」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 從單一滑窗逐步推導完整多影像 2D 卷積公式（式 14.7）](#1-從單一滑窗逐步推導完整多影像-2d-卷積公式式-147)
- [2. 七重巢狀迴圈（Listing 14.1）的四大性質與 CNN 參考硬體架構](#2-七重巢狀迴圈listing-141的四大性質與-cnn-參考硬體架構)
- [3. 迴圈切塊（Loop Tiling：7 變 9 層）與邊界零填充的軟硬體修補](#3-迴圈切塊loop-tiling7-變-9-層與邊界零填充的軟硬體修補)
- [4. 巢狀迴圈形式化映射代數：`▷`、`∥`、執行摺疊（Folding）與陰影快取框](#4-巢狀迴圈形式化映射代數執行摺疊folding與陰影快取框)
- [5. 四大資料駐留策略（Stationarity）的迴圈映射與 Halo Pixels 代價](#5-四大資料駐留策略stationarity的迴圈映射與-halo-pixels-代價)
- [6. 【實測】單核 AVX2 七重迴圈重排與 OS-Tiling 加速（23.7×）及 Global Buffer 流量精算](#6-實測單核-avx2-七重迴圈重排與-os-tiling-加速237及-global-buffer-流量精算)
- [7. 一頁速查表](#7-一頁速查表)
- [8. 與後續講次的連結](#8-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
既然 CONV2D 占掉 CNN 推論 85–90% 時間，它的運算空間該如何形式化描述並映射到平行硬體？（§14.2.1–14.2.2）
  │
  ├── 第一步：把多通道多批次卷積寫成經典 7 層巢狀迴圈（Listing 14.1：n, k, c, h, w, r, s）
  │   ├── 關鍵洞察：每項部分積（Partial Sum）彼此獨立 → 7! = 5040 種迴圈順序在數學上完全等價！
  │   ├── 效能命脈：迴圈順序決定記憶體存取軌跡與 Cache/NoC 區域性；切塊（Tiling）則將 7 層拆成 9 層以塞入小快取
  │   └── 參考硬體（圖 14.5）：2D PE 陣列（每顆含 MAC + Local Buffer≈L1）＋ NoC ＋ Global Buffer(≈L2) ＋ Dispatcher 主控核心
  │
  ├── 第二步：發明緊湊的「迴圈映射表示法（Formal Loop Mapping Notation）」取代冗長程式碼
  │   ├── `a ▷ b`：時間循序巢狀（a 的每次迭代內跑完所有 b）
  │   ├── `a∥`：空間平行展開；對比扁平平行 `[h∥w∥]` vs 階層分組平行 `h∥ ▷ w∥`
  │   ├── Execution Folding（執行摺疊）：迭代數 > PE 數時分批輪流算（課本 Example 15 揭示摺疊順序會影響影像重載次數！）
  │   └── 陰影框 `[ ... ]_LB`：提示硬體將對應張量區塊駐留於 PE 的 Local Buffer（時間重用 Temporal Reuse）
  │
  └── 第三步：探索四大駐留架構（Stationarity Design Space；對比無快取 NLR）
      ├── Weight Stationary (WS)：駐留 R×S 濾波器；若用 `k▷c▷n` 雖極大化權重重用，卻害跨影像 psum 擠爆緩衝區！改 `n▷k∥c∥...` 才平衡
      ├── Input Stationary (IS)：駐留 C 張輸入切塊，但滑窗右／下緣需額外納入 **Halo Pixels**，切塊膨脹為 (Th+R-1)×(Tw+S-1)
      ├── Output Stationary (OS)：駐留 K 張輸出切塊（僅將 IS 的 k 與 c 對調），部分和原地累加免寫回 Global Buffer
      └── Row Stationary (RS)：同時駐留 1 列 ifmap 與 1 列濾波器，將 2D 卷積拆解成極快的 **1D 卷積（w ▷ s）**！
```

---

## 1. 從單一滑窗逐步推導完整多影像 2D 卷積公式（式 14.7）

[00:26]（課本 §14.1.3 回顧至 §14.2.1 式 14.7，p.710–712）
老師在開場強調：後面整整兩講的所有硬體架構都建立在卷積索引方程式上，因此前 16 分鐘帶著觀眾由簡入繁、四步推導出完整式子（並請大家務必背熟七個大小寫符號 `[14:00]`）：

1. **單張輸入、單張輸出** `[02:20]`：在輸入 $I$ 的座標 $(h, w)$ 放上 $R \times S$ 濾波器視窗，逐元素相乘相加（⚠️ 影片 `[06:09]` 老師自己抓出投影片上標示視窗與濾波器座標的箭頭指反了，口頭現場更正）：
   $$O[h][w] = \sum_{r=0}^{R-1} \sum_{s=0}^{S-1} I[h + r][w + s] \times F[r][s]$$
2. **加入 $C$ 個輸入通道（ifmaps）** `[07:01]`：每個輸入通道 $c \in [0, C-1]$ 配各自的專屬濾波器 $F[c]$，分別卷積後**跨通道全部加總**成單張輸出：
   $$O[h][w] = \sum_{c=0}^{C-1} \sum_{r=0}^{R-1} \sum_{s=0}^{S-1} I[c][h + r][w + s] \times F[c][r][s]$$
3. **加入 $K$ 張輸出特徵圖（ofmaps）** `[09:29]`：每對 `(輸入通道 c, 輸出特徵圖 k)` 都有獨立濾波器（共 $C \times K$ 個濾波器），濾波器升為四維 $F[k][c][r][s]$。
4. **加入批次 $N$ 張原始影像（Batch Size $= N$）** `[11:26]`（課本式 14.7，p.712）：
   $$O[n][k][h][w] = \sum_{c=0}^{C-1} \sum_{r=0}^{R-1} \sum_{s=0}^{S-1} I[n][c][h + r][w + s] \times F[k][c][r][s]$$
   - **關鍵觀察** `[13:48]`：影像編號 $n$ 只出現在 $O$ 跟 $I$，**絕對不會出現在濾波器 $F$ 上**——代表同一組訓練好的濾波器權重可跨批次內所有 $N$ 張影像共用！
   - `【口頭】` **跨步存取（Strided Access）** `[14:49]`：課本與本講為簡化推導，假設 $I$ 與 $O$ 皆為 $H \times W$；實務上若只在偶數座標 $(h, w)$ 計算卷積（stride $= 2$），輸出 ofmap 長寬就會等比例縮小。

| 符號（總數／迭代變數） | 物理意義（課本表 14.1） |
|---|---|
| $N$ ／ $n$ | 單批次輸入影像數（Batch size） |
| $C$ ／ $c$ | 該層輸入特徵圖（Input Channels / ifmaps）數量 |
| $K$ ／ $k$ | 該層輸出特徵圖（Output Channels / ofmaps）數量（對應 $K$ 個卷積單元；濾波器總數為 $C \times K$） |
| $H \times W$ ／ $h, w$ | 每張 ifmap／ofmap 的高度（列數）與寬度（行數） |
| $R \times S$ ／ $r, s$ | 每個 2D 濾波器（Filter / Kernel）的高度與寬度 |

---

## 2. 七重巢狀迴圈（Listing 14.1）的四大性質與 CNN 參考硬體架構

[16:01]（課本 §14.2.1 Listing 14.1 與圖 14.5，p.712–713）

把式 14.7 直接寫成 C 程式碼，就得到貫穿全章的**經典七重巢狀迴圈（Listing 14.1）**：

```c
for (n = 0; n < N; n++)         /* 1. input images */
  for (k = 0; k < K; k++)       /* 2. output feature maps (ofmaps) */
    for (c = 0; c < C; c++)     /* 3. input channels (ifmaps) */
      for (h = 0; h < H; h++)   /* 4. rows of ifmap / ofmap */
        for (w = 0; w < W; w++) /* 5. cols of ifmap / ofmap */
          for (r = 0; r < R; r++)   /* 6. rows of filter */
            for (s = 0; s < S; s++) /* 7. cols of filter */
              O[n][k][h][w] += I[n][c][h + r][w + s] * F[k][c][r][s];
```

### 2.1 七重迴圈的四大結構特性（課本 p.712）

1. **迴圈順序完全自由可換（Loop Reordering Invariance）** `[19:05]`：七個迴圈變數彼此毫無相依性！你可以把這 7 層迴圈任意排列組合（共 $7! = 5040$ 種排法），算出的數學結果完全一樣；但**記憶體區域性（Cache / Buffer Locality）與 NoC 流量會天差地遠** `[24:37]`！
2. **純粹的乘積累加（MAC）與部分和（Partial Sum）**：每算 1 個輸出像素需要 $C \times R \times S$ 次乘法與同數量加法，即 **$C \times R \times S$ 次 MAC 運算**（$= 2CRS$ FLOPs）。每一次單項乘積 $I \times F$ 稱為一個 **Partial Sum（部分和）**，累加完所有 $(c, r, s)$ 才成為最終輸出像素（平行累加時僅需注意原子性或歸約順序 `[24:25]`）。
3. **七維獨立空間天然適合切塊（Loop Tiling）** `[24:56]`：可把任意維度切成小區塊塞進快取，讀進來一次榨乾所有重用再換下一塊。
4. **極簡處理單元（PE）平行化**：因為迴圈內只做 `a += b * c`（MAC）與固定步進索引，根本不需要分支預測或亂序執行引擎——最適合鋪設成百上千顆極簡 **PE（Processing Element）** 平行狂算！

### 2.2 CNN 參考硬體架構（課本圖 14.5）

[21:36]（課本 p.713）在進入映射公式前，我們先在腦中建立目標硬體模板：

```
  ┌────────────┐      ┌───────────────┐      ┌───────────────────────────────────┐
  │ Dispatcher │◄────►│ Global Buffer │◄────►│ 2D Matrix of PEs (connected by NoC)│
  │ (主控大腦 / │      │  (GB ≈ L2 快取)│      │  ┌──────────┐   ┌──────────┐      │
  │  OOO CPU)  │      └───────▲───────┘      │  │ PE: MAC  ├───┤ PE: MAC  ├───   │
  └────────────┘              │              │  │ + LB(≈L1)│   │ + LB(≈L1)│      │
                              ▼              │  └────┬─────┘   └────┬─────┘      │
                       ┌─────────────┐       │       │   水平/垂直   │            │
                       │ Main Memory │       │  (註：圖14.5交點無黑點代表互不導通) │
                       └─────────────┘       └───────────────────────────────────┘
```

- **PE + Local Buffer（LB）**：每顆 PE 只是一顆強化版 MAC ALU 配上一小塊 **Local Buffer（等效於私有 L1 快取）** `[22:22]`。
- **NoC + Global Buffer（GB）**：PE 陣列透過水平／垂直晶片內網路連接到大容量的 **Global Buffer（等效於共享 L2 快取）**，再往外接主記憶體（DRAM）。
- **Dispatcher（調度主控器）** `[22:42]`：負責切分工作給 PE、對記憶體發送預取（Prefetch）指令、控制歸約（Reduction）與寫回；`【口頭】` 老師指出這顆主控「大腦」通常就是一顆傳統的 **Out-of-Order 通用處理器**。

---

## 3. 迴圈切塊（Loop Tiling：7 變 9 層）與邊界零填充的軟硬體修補

[24:56]（課本 §14.2.1 p.714；⚠️ 注意影片參考章節口誤）
- ⚠️ 影片 `[25:06]` 與 `[25:45]` 老師兩度口頭說「我們在本書第 5 章詳細教過 Loop Tiling」，實際上在課本 v3.2 中 Loop Tiling 位於 **Chapter 7**（§7.4.5，課本 p.714 正確標註 `see Section 7.4.5`，對應 [第 25 支](lecture25_note.md)）。
- **把 7 層迴圈切成 9 層迴圈** `[25:53]`：
  - 若整張 $H \times W$ 特徵圖太大塞不進 PE 的 Local Buffer，我們將 $h, w$ 各拆成「外層跨步跳躍迴圈」與「內層塊內逐格迴圈」：
    - 外層 `h'`（影片程式碼寫 `h1`）每次遞增切塊高度 `Th`；外層 `w'`（`w1`）每次遞增 `Tw`。
    - 內層 `h''`（`h2`）在 `[h', h' + Th - 1]` 內逐一遞增 `1`；內層 `w''`（`w2`）在 `[w', w' + Tw - 1]` 內逐一遞增 `1`。
  - ⚠️ **影片投影片與課本 p.714 的內層順序差異**：影片 `[26:39]` 與 `[37:45]` 投影片將塊內迴圈放在濾波器迴圈**裡面**（`... ▷ h' ▷ w' ▷ r ▷ s ▷ h'' ▷ w''`），課本 p.714 則放在濾波器迴圈**外面**（`... ▷ h' ▷ w' ▷ h'' ▷ w'' ▷ r ▷ s`）。由 §2.1 迴圈交換律可知兩者結果完全相同，且都能把單一 $T_h \times T_w$ 切塊鎖在快取裡算完再換塊！
- **修補右緣與下緣越界存取（Zero Padding）** `[28:45]`：
  - 在 Listing 14.1 中，當 `h` 靠近底邊（如 $h = H-1$）且 $r > 0$ 時，`I[n][c][h+r][w+s]` 會讀到超出 $H \times W$ 邊界的無效位址（右邊界 `w+s >= W` 同理）。
  - **兩種極簡修法** `[29:54]`：(1) **硬體攔截**：告知記憶體／緩衝區控制器，凡座標越界讀取一律忽略並直接回傳 `0`；(2) **軟體顯式補零（Zero Padding）**：直接配置一張擴充為 **$(H + R - 1) \times (W + S - 1)$** 的矩陣，在右側補 $S-1$ 行零、下側補 $R-1$ 列零。

---

## 4. 巢狀迴圈形式化映射代數：`▷`、`∥`、執行摺疊（Folding）與陰影快取框

[31:03]（課本 §14.2.1 後半，p.713–715）
每次討論最佳化都要貼九層 `for` 迴圈太笨重了，課本定義了一套優雅的**單行映射代數（Mapping Representation）**：

1. **時間循序算子 `▷`（Sequential / Temporal Nesting）** `[31:37]`：
   - Listing 14.1 直接寫成：`n ▷ k ▷ c ▷ h ▷ w ▷ r ▷ s`。右邊的迴圈嚴格巢狀包在左邊迴圈的每一次迭代之內。
2. **空間平行算子 `∥`（Parallel Execution）與扁平 vs 階層平行** `[33:19]`：
   - `n∥` 代表若硬體夠多，所有 $N$ 張影像可分散到不同 PE 同時平行處理。
   - **單層扁平平行 `[h∥w∥]`** `[33:52]`：將所有 $H \times W$ 個像素座標攤平，以任意順序派發給空閒 PE。
   - **階層式分組平行 `h∥ ▷ w∥`** `[34:33]`：先按列 `h` 把 PE 陣列分成多個 PE 群組（各群組平行跑不同列）；在每一個群組內部，再把該列的不同行 `w` 分給群組內的個別 PE 平行跑！這賦予硬體規整的二維空間結構。
3. **執行摺疊（Execution Folding）** `[37:04]`（課本 p.714 與 **Example 15**）：
   - 當迴圈迭代數大於實體 PE 數（例如有 1024 次迭代但只有 256 顆 PE），硬體會以 256 次為一波分四波依序執行，稱為 **Folded Execution**。
   - `【課本補充】` **課本 Example 15 的精闢陷阱（p.714）**：比較 `S1 = [h∥ ▷ n ▷ w∥]` 與 `S2 = [h∥ ▷ w∥ ▷ n]`（課本印為 `[h∥ ▷ w∥n▷]`）：
     - 若 PE 多到**不需摺疊**，$S_1$ 與 $S_2$ 載入每張影像的次數完全相等。
     - 但若群組內 PE 數少於寬度 $W$（**`w∥` 發生摺疊**）：$S_1$ 外層先固定一張影像 `n`，把該影像所有摺疊波次的 `w` 全算完才換下一張圖，**每張圖只載入 1 次**；反觀 $S_2$ 先固定第一波 `w`，在內層把所有 $N$ 張圖輪流掃過一遍，接著第二波 `w` 又把所有 $N$ 張圖**重新從記憶體載入一遍**，頻寬瞬間爆炸！
4. **陰影快取框（Shaded Box，筆記記為 $\boxed{\dots}$）與粗粒度屏障** `[38:44]`（課本 p.715）：
   - 把迴圈區段框進陰影盒（如 $n \triangleright k \triangleright c\parallel h \triangleright w \triangleright \boxed{r \triangleright s}$），代表給予硬體的快取提示：將對應資料（此處為整個 $R \times S$ 濾波器）駐留在 PE 的 **Local Buffer（LB）** 中榨取時間重用；且平行執行緒只需在外層（如每算完一個 `k`）設一道粗粒度 **Barrier** 同步即可。

---

## 5. 四大資料駐留策略（Stationarity）的迴圈映射與 Halo Pixels 代價

[40:40]（課本 §14.2.2，p.715–717）
在卷積核心運算 `O += I * F` 中共有三類張量：**權重（Weights `F`）**、**輸入像素（Inputs `I`）**、**輸出與部分和（Outputs / Partial Sums `O`）**。選擇把哪一類釘死在 PE 的 Local Buffer 裡反覆重用，就稱為 **Stationarity**（`【課本補充】` p.715：若完全不在 PE 快取任何資料，則稱為 **NLR, No Local Reuse** 架構）：

### 5.1 權重駐留架構（Weight Stationary, WS）與「過猶不及」教訓

[40:59]（課本式 14.8–14.9，p.716）
- **極端最大化權重重用的天真寫法（課本式 14.8）** `[42:07]`：
  $$k \triangleright c \triangleright n \triangleright h'\parallel w'\parallel h'' \triangleright w'' \triangleright \boxed{r \triangleright s}$$
  - 因為濾波器只取決於 `(k, c)`，把 `k ▷ c` 拉到最外層可以讓每個 $R \times S$ 濾波器**全場只載入 1 次**，一口氣刷過全部 $N$ 張影像後才丟掉！
  - **為什麼這反而很糟？** `[43:21]`（課本 p.716）：(1) 每換一組 `(k, c)` 就要把全部 $N$ 張影像重讀一遍；(2) 更慘的是，單一 `(k, c)` 算出的只是 **Partial Sum**——你必須同時在記憶體裡暫存**全部 $N$ 張影像未完成的龐大 ofmap 部分和**，每換一個 `c` 就把全部部分和讀出來加完再寫回去，輸出時間區域性爛到極點！
- **實務平衡版 WS 映射（課本式 14.9）** `[44:20]`：
  $$n \triangleright k\parallel c\parallel h \triangleright w \triangleright \boxed{r \triangleright s}$$
  - **黃金準則：把 `n` 放回最外層（一次專心算完一張圖，別把不同影像混在一起 `[44:50]`）**！由不同 PE 平行認領不同的 `(k, c)` 濾波器並駐留於 LB，一口氣算完當前影像的所有空間位置。
  - **架構師銘言（課本 p.716）**：*死命壓榨單一資料塊（權重）的區域性，往往會摧毀另一塊資料（部分和／輸入）的區域性並撐爆暫存空間；系統設計永遠追求整體平衡！*

### 5.2 輸入駐留架構（Input Stationary, IS）與 Halo Pixels

[45:28]（課本式 14.10，p.717）
- **映射公式（課本式 14.10）**：
  $$n \triangleright h'\parallel w'\parallel k \triangleright r \triangleright s \triangleright \boxed{c \triangleright h'' \triangleright w''}$$
- **運作方式** `[46:47]`：將影像空間切成小塊分給各 PE；每顆 PE 在自己的 LB 裡**駐留該座標切塊的全部 $C$ 個輸入通道資料**（$\boxed{c \triangleright h'' \triangleright w''}$），接著由外部不斷串流轟炸（bomb with `[52:31]`）各個輸出通道 `k` 的濾波器權重進來相乘累加，直接在塊內湊齊所有通道算完最終輸出切塊！
- **Halo Pixels（光暈邊界像素）的面積膨脹代價** `[52:55]`（課本 p.717）：
  - 當 PE 負責計算 $T_h \times T_w$ 輸出切塊的最右行或最下列像素時，$R \times S$ 濾波器會往右凸出 $S - 1$ 格、往下凸出 $R - 1$ 格！
  - 若不想在計算中途透過 NoC 跟隔壁 PE 伸手要像素（跨 PE 通訊複雜且慢 `[53:59]`），每顆 PE 就必須在 LB 中**刻意多存一圈重疊邊界**，使每個輸入切塊尺寸膨脹為：
    $$(T_h + R - 1) \times (T_w + S - 1)$$
  - 這圈為了避免跨塊通訊而在右緣與下緣多存的重疊像素，就稱為 **Halo Pixels** `[54:19]`！

### 5.3 輸出駐留架構（Output Stationary, OS）

[55:19]（課本式 14.11，p.717）
- **映射公式（課本式 14.11）**：
  $$n \triangleright h'\parallel w'\parallel c \triangleright r \triangleright s \triangleright \boxed{k \triangleright h'' \triangleright w''}$$
- 對照式 14.10（IS）與式 14.11（OS）`[57:27]`：**僅僅把 `k` 與 `c` 兩個字對調！**
- 每顆 PE 在 LB 裡駐留某個切塊座標對應的**全部 $K$ 張輸出特徵圖切塊（部分和累加器）**，從外部串流餵入含 Halo 的輸入切塊與濾波器權重。最大優勢：**所有 Partial Sums 自始至終都在 PE 內部原地累加，完全不必讀寫回 Global Buffer**，等全部 `c, r, s` 流完後直接吐出最終答案！

### 5.4 列駐留架構（Row Stationary, RS：MIT Eyeriss 核心思想）

[58:14]（課本式 14.12，p.717）
- **映射公式（課本式 14.12）**：
  $$n \triangleright k \triangleright c \triangleright h'\parallel r'\parallel w \triangleright s \triangleright \boxed{h'' \triangleright r''}$$
- **神來一筆的混合駐留與 1D 拆解** `[59:11]`：
  - 不單獨駐留權重或輸入，而是按「列（Row）」切分，在每顆 PE 的 LB 裡**同時快取 1 列（或數列）輸入特徵圖 `h''` 與 1 列（或數列）濾波器權重 `r''`**！（`[61:48]` 老師特別解釋：陰影框 $\boxed{h'' \triangleright r''}$ 裡面的算子不重要，重點是指向這兩個迭代變數對應的「ifmap 列」與「filter 列」同時駐留在快取中。）
  - 一旦單列輸入與單列濾波器都鎖在 LB 裡，剩下的 `w ▷ s` 就是在做**一維卷積（1D Convolution）** `[63:33]`！硬體可以打造極度高速精簡的 1D 卷積引擎，把龐大的 2D 卷積拆解成大量超高速 1D 卷積，最後再跨 PE 累加部分和（下一講 Eyeriss 將展示完整的對角線脈動連接）。

---

## 6. 【實測】單核 AVX2 七重迴圈重排與 OS-Tiling 加速（23.7×）及 Global Buffer 流量精算

我們撰寫 [`experiments/lecture56/loop_order_and_tiling.c`](experiments/lecture56/loop_order_and_tiling.c)，在本機 Intel Core i7-13650HX 單顆 P-core（24 MiB L3 cache、16 個 256-bit AVX2 YMM 暫存器）上對 $N=16, C=32, K=64, 112 \times 112, 3 \times 3$ 卷積層（共 `3.699G` MACs = `7.399 GFLOPs`，輸出張量 `O` 占 `49.00 MiB` 遠大於 24 MiB L3 快取）進行實測與流量精算：
1. **實測四種數學完全等價的迴圈映射效能**：(a) 課本 Listing 14.1 原始順序 `n▷k▷c▷h▷w▷r▷s`；(b) 式 14.8 把 `n` 塞進 `k▷c` 內層導致 49 MiB 部分和反覆溢出的 `k▷c▷n▷r▷s▷h▷w`；(c) 把 `n` 拉回最外層並將 `w` 放最內層向量化的 `n▷k▷c▷r▷s▷h▷w`；(d) 課本式 14.11 **Output-Stationary Tiling**，將 $T_k=4 \times T_h=2 \times T_w=16$（剛好塞滿 x86-64 全部 **16 個 YMM 暫存器 = 512 B 極速 LB**）駐留在暫存器檔內原地累加部分和！
2. **定量計算 Halo Pixels 膨脹率與 NLR / WS(式 14.8) / IS / OS 的 Global Buffer 總存取字組數**。

式 14.11 的暫存器駐留 Output-Stationary 核心實作節錄如下（`experiments/lecture56/loop_order_and_tiling.c:131-158`）：

```c
                    for (int c = 0; c < C; c++) {
                        const float *in = I + (size_t)(n * C + c) * HP * WP;
                        for (int r = 0; r < R; r++) {
                            for (int s = 0; s < S; s++) {
                                __m256 i00 = _mm256_loadu_ps(in + (hp + 0 + r) * WP + wp + s);
                                __m256 i01 = _mm256_loadu_ps(in + (hp + 0 + r) * WP + wp + s + 8);
                                __m256 i10 = _mm256_loadu_ps(in + (hp + 1 + r) * WP + wp + s);
                                __m256 i11 = _mm256_loadu_ps(in + (hp + 1 + r) * WP + wp + s + 8);
                                for (int kk = 0; kk < 4; kk++) {
                                    __m256 vw = _mm256_set1_ps(
                                        F[((kp + kk) * C + c) * R * S + r * S + s]);
                                    acc[kk][0][0] = _mm256_fmadd_ps(i00, vw, acc[kk][0][0]);
                                    acc[kk][0][1] = _mm256_fmadd_ps(i01, vw, acc[kk][0][1]);
                                    acc[kk][1][0] = _mm256_fmadd_ps(i10, vw, acc[kk][1][0]);
                                    acc[kk][1][1] = _mm256_fmadd_ps(i11, vw, acc[kk][1][1]);
                                }
                            }
                        }
                    }
```

單核執行 `taskset -c 0 /tmp/loop_order_and_tiling` 的實測輸出：

```text
=== 1. CPU 7-Loop Reordering & OS-Tiling Benchmark (N=16,C=32,K=64,112x112,3x3) ===
  Total MACs = 3.699 G  (7.399 GFLOPs) | Working Set: I=25.38 MiB, O=49.00 MiB, F=0.07 MiB
  Mapping                                      | Time (ms) | GFLOP/s | Speedup | MaxDiff
  ---------------------------------------------+-----------+---------+---------+---------
  (a) Listing 14.1 n->k->c->h->w->r->s         |   2769.20 |    2.67 |  1.00x  | 0.00e+00
  (b) Eq.14.8 outer k->c->n->r->s->h->w (spill)|    336.63 |   21.98 |  8.23x  | 7.15e-06
  (c) Image-first   n->k->c->r->s->h->w        |    300.50 |   24.62 |  9.22x  | 7.15e-06
  (d) Eq.14.11 OS-Tiled (Tk=4,Th=2,Tw=16 in RF)|    116.71 |   63.39 | 23.73x  | 7.15e-06

=== 2. Halo Pixel Expansion & Analytical Global-Buffer (GB) Accesses (N=16) ===
  Tile  4x4  (3x3 filter) -> Input tile w/ Halo =  6x6  (125.0% extra pixels)
  Tile  8x8  (3x3 filter) -> Input tile w/ Halo = 10x10 ( 56.2% extra pixels)
  Tile 16x16 (3x3 filter) -> Input tile w/ Halo = 18x18 ( 26.6% extra pixels)

  Stationarity Model (Th=Tw=7, 256 tiles)| LB Size/PE | Total GB Words (M) | vs NLR
  ---------------------------------------+------------+--------------------+--------
  No Local Reuse (NLR)                   |      0 B   |        14797.50 M  |  1.00x
  WS Eq.14.8 (k->c->n, R*S in LB)        |     36 B   |         1501.58 M  |  9.85x
  IS Eq.14.10 (C*9*9 halo tile in LB)    |  10368 B   |           98.96 M  | 149.53x
  OS Eq.14.11 (K*7*7 ofmap tile in LB)   |  12544 B   |           98.96 M  | 149.53x
```

**實測觀察與硬體印證**：
1. **一字不改演算法，光靠迴圈重排與切塊就快了 `23.73×`（且數值誤差僅單精度浮點結合律 `7.15e-6`）**：
   - Listing 14.1 把長度僅 3 的濾波器迴圈 `r ▷ s` 放最內層，破壞連續空間向量化，單核僅 `2.67 GFLOP/s`（耗時 `2.77 秒`）；把 `w` 調到最內層立刻讓硬體能以連續 stride-1 發射 AVX2 FMA，飆上 `24.62 GFLOP/s`（快 `9.22×`）。
   - 對照 `(b)` 與 `(c)`：兩者內層指令完全相同，但 `(b)` 照課本式 14.8 把 `k ▷ c` 放最外、`n` 塞中間，害 `49 MiB` 的全批次輸出部分和與 `25 MiB` 輸入在每次換 `(k, c)` 時衝破 24 MiB L3 cache；照式 14.9 原則把 `n` 移回最外層（一次處理一張圖，單張圖工作集僅 `4.65 MiB` 穩居 L3 內），零成本省下 `36 ms`（快 `12%`）！
   - 最後 `(d)` 實作式 14.11 **Output-Stationary Tiling**，把 `4 × 2 × 16 = 128` 個輸出部分和鎖在 16 個 YMM 暫存器內直到 `C × R × S = 288` 次累加全部結束才寫回記憶體一次，直接衝上 **`63.39 GFLOP/s`（快 `23.73×`）**！
2. **Halo Pixels 的切塊大小權衡**：當 $3 \times 3$ 濾波器搭配極小的 $4 \times 4$ 切塊時，含 Halo 的輸入塊變成 $6 \times 6 = 36$ 像素，**多出高達 `125.0%` 的重疊邊界像素**；放大到 $16 \times 16$ 切塊時 Halo 額外負擔就降到 `26.6%`，但需要更大的 PE Local Buffer。
3. **定量打臉式 14.8 的偏科設計**：在 Global Buffer 存取統計中，式 14.8 雖然讓濾波器全場只讀 `1.8 萬` 個字組，卻因為每次 MAC 都要讀寫未完成的部分和，總 GB 流量仍高達 **`15.02 億` 字組**；反觀平衡的 IS（式 14.10）與 OS（式 14.11）只需在 PE 內放 `10–12 KiB` 的 Local Buffer，就能把總 GB 流量壓到 **`0.99 億` 字組（比 NLR 減少近 `150×`，比式 14.8 少 `15.2×`）**！

---

## 7. 一頁速查表

| 符號／架構概念 | 形式化定義或映射範例 | 關鍵效能與硬體意義 |
|---|---|---|
| **七重迴圈（Listing 14.1）** | `n ▷ k ▷ c ▷ h ▷ w ▷ r ▷ s` | 每輸出像素做 $C \cdot R \cdot S$ 次 MAC；任意重排皆正確，但決定快取命中率 |
| **`▷` vs `∥` 算子** | `[h∥w∥]`（扁平） vs `h∥ ▷ w∥`（階層分組） | `▷` 表時間巢狀順序；`∥` 表空間跨 PE 平行 |
| **Execution Folding** | 迭代數 $>$ PE 數時分波次循序執行 | 摺疊時迴圈順序極關鍵（如課本 Ex.15 `[h∥▷w∥▷n]` 會害影像重複載入） |
| **Loop Tiling（切塊）** | `h, w` 拆為外層步進 `h', w'`（`+Th, +Tw`）與塊內 `h'', w''` | 將 7 層迴圈擴為 9 層，確保工作集塞進 PE Local Buffer |
| **Halo Pixels** | 輸入切塊擴張為 $(T_h + R - 1) \times (T_w + S - 1)$ | 預存右／下重疊邊界像素，免除運算中途昂貴的跨 PE NoC 通訊 |
| **Weight Stationary (WS)** | 平衡版：$n \triangleright k\parallel c\parallel h \triangleright w \triangleright \boxed{r \triangleright s}$（式 14.9） | LB 快取 $R \times S$ 濾波器；切忌用 `k▷c▷n`（式 14.8）以免跨圖部分和爆快取 |
| **Input Stationary (IS)** | $n \triangleright h'\parallel w'\parallel k \triangleright r \triangleright s \triangleright \boxed{c \triangleright h'' \triangleright w''}$（式 14.10） | LB 快取全部 $C$ 通道的含 Halo 輸入切塊，串流轟炸濾波器權重 |
| **Output Stationary (OS)** | $n \triangleright h'\parallel w'\parallel c \triangleright r \triangleright s \triangleright \boxed{k \triangleright h'' \triangleright w''}$（式 14.11） | LB 快取全部 $K$ 張輸出切塊，部分和原地累加零寫回（實測單核快 `23.7×`） |
| **Row Stationary (RS)** | $n \triangleright k \triangleright c \triangleright h'\parallel r'\parallel w \triangleright s \triangleright \boxed{h'' \triangleright r''}$（式 14.12） | LB 同時快取 ifmap 列與濾波器列，將 2D 卷積化為超高速 **1D 卷積（`w▷s`）** |

---

## 8. 與後續講次的連結

- **往回對照 [第 25 支](lecture25_note.md)（Ch7 §7.4.5 快取軟體最佳化 Loop Tiling）**：第 7 章在矩陣乘法首次登場的 Loop Tiling，在本講正式推廣到七維卷積空間，並衍生出邊界重疊的 **Halo Pixels** 問題。
- **往下接 [第 57 支](lecture57_note.md)（Part III：2D 硬體資料流、脈動陣列 Systolic Arrays、Retiming Lemma 與矩陣乘法轉換）**：本講只在「單一 PE 內部快取（Temporal Reuse 時間重用）」的軟體概念層次分析 WS / IS / OS / RS；真實硬體還能透過鄰居 PE 之間逐拍接力傳遞（**Spatial Reuse 空間重用 / Systolic Transfer**）砍掉廣播線 RC 延遲與 Global Buffer 頻寬——下一講將完整推導五種 2D 硬體陣列、用 **Retiming Lemma（Bellman-Ford 最短路徑）** 自動把半脈動陣列轉為純脈動陣列，並將 2D 卷積化約成矩陣乘法餵進 GPU Tensor Cores！

---

## 附：本講一句話總結

> 二維卷積的七重獨立迴圈雖允許五千多種數學等價的排列組合，但唯有透過 `▷ / ∥` 映射代數與切塊（Tiling）精準權衡權重、含 Halo 輸入與部分和三者的快取駐留（WS / IS / OS / RS），避免「單顧權重卻擠爆部分和」的陷阱，才能在不改動一個演算法乘號的前提下榨出數十倍的硬體吞吐量。

---

*筆記整理自 Smruti R. Sarangi 教授 Advanced Computer Architecture 第 56 講影片與 Next-Gen Computer Architecture v3.2 第 14 章 §14.2.1–14.2.2，搭配本機 i7-13650HX AVX2 實測驗證。*
