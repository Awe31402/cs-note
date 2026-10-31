# Advanced Computer Architecture 第五十七支影片完整筆記：機器學習硬體架構（三）：空間重用硬體資料流、脈動陣列（Systolic Arrays）、Retiming Lemma 與矩陣乘法轉換（Chapter 14, Part III）

> **課程**：Advanced Computer Architecture — Chapter 14: Architectures for Machine Learning (Part III) -- Hardware Architectures（播放清單第 57 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（1:06:44）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch14** §14.2.3–14.3.2（p.718–730）
> **章號**：影片 Chapter 14 = 課本 Ch14。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、NVIDIA GeForce RTX 4060 Laptop GPU、`nvcc` 12.4 + cuBLAS Tensor Cores、Ubuntu、kernel 6.5.0）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕把「naive algorithm」寫成「knife algorithm」、「notation」寫成「motion」、「PEs」寫成「peas／piece」、「clock skew」寫成「clock's Q」、「tree of adders / costly tree of adders」寫成「tree of addits／trio of adders／tree of adults／causal three of addits」、「ifmap(s) / ofmaps」寫成「is map(s)／OS Maps」、「Timeloop」寫成「time Loop」、「No Local Reuse (NLR)」寫成「no local reuser」、「1D / 2D convolution」寫成「1D congregation／2d conversation／one day convolutions」、「halo pixels」寫成「hello pixels」、「RC delay」寫成「rcd」、「semi-systolic array」寫成「CMS is probably carrying」、「Retiming Lemma / Theorem」寫成「retiling Lemma／retiring Lemma／retining theorem」、「dummy Host node」寫成「dummy Force Mode」、「Row Stationary」寫成「roast」、「doubly block circulant matrix」寫成「doubly block Circle and Matrix」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 從時間重用到空間重用：為什麼大型加速器討厭廣播（Broadcast），偏愛脈動傳遞（Systolic Transfer）？](#1-從時間重用到空間重用為什麼大型加速器討厭廣播broadcast偏愛脈動傳遞systolic-transfer)
- [2. PE 間（Inter-PE）五種 2D 硬體資料流：WS、IS、OS、RS（Eyeriss）與 NLR](#2-pe-間inter-pe五種-2d-硬體資料流wsisosrseyeriss與-nlr)
- [3. PE 內（Intra-PE）一維卷積：兩種半脈動陣列（Semi-Systolic Arrays）及其硬體瓶頸](#3-pe-內intra-pe一維卷積兩種半脈動陣列semi-systolic-arrays及其硬體瓶頸)
- [4. Retiming Lemma：用圖論最短路徑自動把半脈動陣列轉成純脈動陣列](#4-retiming-lemma用圖論最短路徑自動把半脈動陣列轉成純脈動陣列)
- [5. 二維卷積硬體化：串接 1D 脈動方塊 vs 雙塊循環矩陣（`im2col` / GEMM）轉換](#5-二維卷積硬體化串接-1d-脈動方塊-vs-雙塊循環矩陣im2col--gemm轉換)
- [6. 【實測】Bellman-Ford 重定時與雙流交錯 100% 脈動模擬，及 RTX 4060 Tensor Core 30× 矩陣加速](#6-實測bellman-ford-重定時與雙流交錯-100-脈動模擬及-rtx-4060-tensor-core-30-矩陣加速)
- [7. 一頁速查表](#7-一頁速查表)
- [8. 與後續講次的連結](#8-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
上一講在軟體概念層建立了時間重用（Stationarity）；這一講真正落地成二維矽晶片電路！（§14.2.3–14.3）
  │
  ├── 第一層：PE 陣列間的空間重用（Inter-PE Spatial Reuse，§14.2.3）
  │   ├── 硬體連線大忌：長導線單週期廣播（Broadcast）有嚴重 Clock Skew 與並聯電容 RC 延遲！
  │   │                 → 改用脈動傳遞（Systolic Transfer）：每拍只經暫存器傳給相鄰右方／下方 PE
  │   └── 五大 2D 陣列架構（圖 14.6–14.11；探索工具：Timeloop）：
  │       ├── WS（列=c、行=k）：水平流同座標 ifmap 切塊，垂直逐拍向下累加部分和（或用破壞規整性的加法樹）
  │       ├── IS（圖 14.8 改良版）：列拆通道 c、行拆空間切塊，水平流權重、垂直脈動歸約部分和
  │       ├── OS（圖 14.9）：行拆輸出 k、列拆空間切塊，水平流 C 張輸入切塊與垂直流權重在 PE 內對撞累加
  │       ├── RS（圖 14.10，MIT Eyeriss）：列駐留濾波器列 r1..r3，輸入列排成對角滑動矩陣 [h1 h2 h3; h2 h3 h4; h3 h4 h5]
  │       └── NLR（圖 14.11）：PE 內零快取換取高密度 PE 數，全靠高頻寬 Global Buffer + 水平脈動加總
  │
  ├── 第二層：單顆 PE 內部的算術電路（Intra-PE Parallelism，§14.3.1）
  │   ├── 半脈動陣列（Semi-systolic，相鄰組合邏輯間無暫存器）：
  │   │   (1) 權重固定＋組合加法樹（圖 14.12 縫紉機模型）→ 濾波器大時加法樹拉長關鍵路徑！
  │   │   (2) 輸出固定＋權重廣播線（圖 14.13）→ 長廣播線巨大 RC 延遲！
  │   └── **Retiming Lemma** 自動轉為純脈動陣列（每對組合邏輯間至少 1 顆暫存器）：
  │       ├── 圖 G - 1 有負環 → 放大為 2G - 1 消除負環，跑 Bellman-Ford 求至 Host 最短路徑 lag(v)
  │       ├── 新暫存器數 w_new = k·w_old + lag(v) - lag(u) >= 1（圖 14.14：輸入與權重反向隔拍對撞）
  │       └── 解救 k=2 帶來的 50% 氣泡閒置：在空拍 `□` 交錯塞入第 2 組獨立卷積 → **利用率重回 100%**！
  │
  └── 第三層：2D 卷積的兩條終極硬體路線（§14.3.2）
      ├── 路線 A：串接多組高速 1D 脈動卷積方塊（Kung & Song 1981，圖 14.15）
      └── 路線 B：展開成**雙塊循環矩陣（Doubly Block Circulant / `im2col`）**，化約為標準矩陣乘法（GEMM）直上 GPU Tensor Cores！
```

---

## 1. 從時間重用到空間重用：為什麼大型加速器討厭廣播（Broadcast），偏愛脈動傳遞（Systolic Transfer）？

[00:12]（課本 §14.2.3 開場，p.718）
- **時間重用（Temporal Reuse）vs 空間重用（Spatial Reuse）** `[00:56]`：
  - 上一講的陰影框只描述了**時間重用**（同一顆 PE 在不同時間反覆讀取自己 Local Buffer 裡的資料）。
  - 真實硬體還必須壓榨**空間重用**：從 Global Buffer（GB）只讀**一次**資料，就讓整排或整群 PE 共享這份資料，避免每顆 PE 各自向 GB 發出重複讀取請求而塞爆頻寬！
- **硬體傳遞資料的兩種機制對決** `[02:46]`：
  1. **單週期廣播（Broadcast / Multicast）** `[02:54]`：拉一條長導線同時接上整排數十甚至數百顆 PE，要求訊號在**同一個時脈週期內**抵達所有 PE。
     - **致命硬體痛點** `[03:20]`：(a) 晶片面積大時各處時脈到達時間有微小偏差（**Clock Skew** `[03:38]`），極難保證時序收斂；(b) 整排 PE 暫存器的輸入電容全部**並聯**在同一條長線上，造成龐大的 **RC 充放電延遲** `[33:36]`，直接拖垮全晶片時脈頻率！
  2. **脈動傳遞（Systolic Transfer）** `[03:59]`：靈感來自心臟搏動——在每個時脈邊緣（clock boundary），每顆 PE **只把資料或部分和透過暫存器傳給緊鄰的右方或下方鄰居**！導線極短、不需嚴苛的全域同步，即使存在輕微 clock skew 也穩如泰山，具備絕佳的可擴展性（Scalability）。

---

## 2. PE 間（Inter-PE）五種 2D 硬體資料流：WS、IS、OS、RS（Eyeriss）與 NLR

[04:46]（課本 §14.2.3 圖 14.6–14.11、式 14.13–14.17，p.718–724；老師於 `[02:13]` 提醒映射符號主要表達計算分配，硬體方向性需搭配架構圖閱讀）

### 2.1 權重駐留硬體（Weight Stationary, WS：課本圖 14.6、式 14.13）

[04:49] 將二維 PE 陣列的**列（Rows）對應輸入通道 $c_1 \dots c_C$**，**行（Columns）對應輸出特徵圖 $k_1 \dots k_K$**，映射微調為階層平行：
$$n \triangleright k\parallel \triangleright c\parallel \triangleright h \triangleright w \triangleright \boxed{r \triangleright s}$$
- **四階段運作（Phase I–IV）** `[09:08]`（課本 p.719）：
  1. **Phase I（預載權重）**：每顆 $\text{PE}(c, k)$ 從 GB 把專屬濾波器 $F[k][c]$ 讀進自己的 Local Buffer 鎖住。
  2. **Phase II（水平餵入同座標輸入）**：沿著每一列 $c$ 水平送入**相同 $(h, w)$ 空間座標**的 ifmap 像素切塊。
  3. **Phase III（局部卷積）**：每顆 PE 算出該 `(c, k)` 配對的部分和（Partial Sum）。
  4. **Phase IV（垂直跨通道歸約）**：同一行 $k$ 的所有 PE 算出的部分和必須全部加總才是最終 ofmap $k$！硬體有兩種加法選擇 `[09:35]`：(a) 在每行底部擺一棵**平行加法樹（Tree of Adders）**——快但破壞二維陣列的規整性；(b) **垂直脈動累加（Systolic Addition）**——最頂端 PE 算完 $x$ 傳給下一列 PE 加上 $y$ 變 $x+y$，再傳給下一列變 $x+y+z$，一路從底部流出完整答案，結構極度規整 `[07:45]`！

### 2.2 輸入駐留硬體（Input Stationary, IS：課本圖 14.7 與改良版圖 14.8、式 14.14）

[10:30]（課本 p.719–721）
- **初版獨立 PE 設計（圖 14.7）** `[10:39]`：每顆 PE 快取某空間區域的**全部 $C$ 張輸入切塊**，外部將濾波器權重廣播給所有 PE，算完後沿垂直線讀出輸出。（`【口頭】` `[13:55]` 與 `[25:36]` 老師特別推薦開源架構探索工具 **Timeloop**，能自動窮舉成千上萬種迴圈切塊與硬體映射組合找出能耗延遲最佳解！）
- **改良版脈動歸約設計（圖 14.8，課本式 14.14）** `[14:17]`：
  $$n \triangleright h'\parallel w'\parallel \triangleright c\parallel k \triangleright r \triangleright s \triangleright \boxed{h'' \triangleright w''}$$
  - 把同一個空間切塊的不同通道拆開：每一**行**負責同一個空間切塊座標 $(h_i, w_i)$，每一**列**負責一個輸入通道 $c_1 \dots c_4$（每顆 PE 僅需快取**單一通道**的輸入切塊！）。
  - 針對固定輸出 $k$，將濾波器權重 $F[k][c_i]$ 沿著各列水平流過，再跟 WS 一樣沿著每一行**由上往下做脈動部分和累加**，徹底免除全域廣播！

### 2.3 輸出駐留硬體（Output Stationary, OS：課本圖 14.9、式 14.16）

[17:54]（課本 p.720–722）
- 課本 p.720 點出：若照軟體版式 14.15 讓單顆 PE 包辦某切塊的全部 $K$ 張輸出，該切塊的輸入就無法跨 PE 空間共享。因此硬體版改將**行對應輸出特徵圖 $k_1 \dots k_4$**、**列對應空間切塊座標 $(h_1, w_1) \dots (h_4, w_4)$**（課本式 14.16）：
  $$n \triangleright h'\parallel w'\parallel \triangleright k\parallel c \triangleright r \triangleright s \triangleright \boxed{h'' \triangleright w''}$$
- 每顆 PE 駐留**單一張 ofmap 的單一切塊**；$C$ 個通道的輸入切塊沿著**列**水平串流，對應的濾波器權重 $F[k][c]$ 沿著**行**垂直串流，兩者在每顆 PE 精準對撞（collide `[19:07]`）並原地累加完所有通道，最後從底部讀出成品。

### 2.4 列駐留硬體（Row Stationary, RS：MIT Eyeriss，課本圖 14.10、式 14.17）

[20:18]（課本 p.722–723，出自 Chen et al., ISCA 2016 *Eyeriss*）
- 以 $3 \times 3$ 濾波器（三列 $r_1, r_2, r_3$）為例，排成 $3 \times 3$ 的 PE 區塊：
  - **濾波器列水平共享**：第 1 列 PE 全部駐留 $r_1$，第 2 列全部駐留 $r_2$，第 3 列全部駐留 $r_3$。
  - **輸入特徵圖列對角滑動配置** `[21:11]`：第一輪在 $3 \times 3$ PE 存入下列對角規律的輸入列（下一輪再換成 $[h_4,h_5,h_6;\ h_5,h_6,h_7;\ h_6,h_7,h_8]$）：
    $$\begin{bmatrix} (h_1, r_1) & (h_2, r_1) & (h_3, r_1) \\ (h_2, r_2) & (h_3, r_2) & (h_4, r_2) \\ (h_3, r_3) & (h_4, r_3) & (h_5, r_3) \end{bmatrix}$$
  - **一維卷積＋垂直歸約**：每顆 PE 內部只算極簡單的 **1D 卷積** $h_i \star r_j$！算完後第 1 行垂直相加 $(h_1 \star r_1) + (h_2 \star r_2) + (h_3 \star r_3)$ 就得到第 1 輸出列，第 2 行相加得到第 2 輸出列！
  - 擴展到多通道與多輸出時，將大陣列切成多個 $(c, k)$ 二維子區塊（課本式 14.17：$n \triangleright k\parallel c\parallel \triangleright h'\parallel r'\parallel w \triangleright s \triangleright \boxed{h'' \triangleright r''}$）。
  - `【課本補充】` **優缺點評析（p.723）**：雖然跨區塊重複存了濾波器與輸入列，但換來三大實戰優勢——(1) PE 硬體簡化為超高速 1D 卷積器；(2) 各行之間無橫向資料交換，不必嚴格鎖步（lockstep）；(3) **計算與載入重疊**：當底列 PE 還在加總部分和時，頂列 PE 已經可以同時預載下一輪的新資料！

### 2.5 無局部重用硬體（No Local Reuse, NLR：課本圖 14.11）

[23:36]（課本 p.723–724）PE 內部完全不放快取（省下面積塞入更多顆 MAC PE），輸入 $I[n][c_i]$ 與權重 $F[k][c_i]$ 全部從高頻寬 Global Buffer 即時餵入各顆 PE，算出的各通道部分和沿水平線由左往右脈動累加吐出結果。

---

## 3. PE 內（Intra-PE）一維卷積：兩種半脈動陣列（Semi-Systolic Arrays）及其硬體瓶頸

[25:50]（課本 §14.3.1 式 14.18、圖 14.12–14.13、Definition 105，p.724–726）
進入單顆 PE 內部，核心任務是計算長度 $n$ 濾波器的一維卷積（為配合電路圖直覺，本節下標改從 $1$ 起算 `[31:00]`）：
$$y_i = \sum_{j=0}^{n-1} w_{j+1} \times x_{i+j}$$

- **半脈動（Semi-systolic）vs 純脈動（Systolic）的嚴格定義（課本 Definition 105，p.725）** `[34:33]`：
  - **純脈動陣列（Systolic Array）**：**任意兩個相鄰的組合邏輯單元（Combinational Elements，如乘法器、加法器）之間，必定隔著至少 1 顆暫存器**！每拍工作量被嚴格限制在單一邏輯級，時脈可以拉到極高。
  - **半脈動陣列（Semi-systolic Array）**：允許相鄰組合邏輯單元之間**直接用導線相連而沒有暫存器**（圖論上邊權重 $= 0$）。雖然容易憑直覺設計，但往往暗藏時脈殺手：
    1. **設計一：權重固定＋組合邏輯加法樹（課本圖 14.12）** `[28:28]`：
       - $w_1 \dots w_4$ 固定存在 4 顆乘法器裡，$x_1, x_2, \dots$ 每拍透過暫存器往右移一格（課本 p.724 比喻像**布料滑過縫紉機針頭**），上方接一棵無暫存器的**組合邏輯加法樹**當場加出 $y_1, y_2, \dots$。
       - **瓶頸** `[31:26]`：若濾波器有 25 個權重，單一週期內要穿透整棵 25 輸入加法樹，嚴重拉長關鍵路徑並造成管線不平衡（除非手動把加法樹逐層切管線暫存器）。
    2. **設計二：輸出固定＋權重全域廣播（課本圖 14.13）** `[32:18]`：
       - 6 顆 MAC 單元各自原地累加 $y_1 \dots y_6$；輸入 $x$ 經暫存器逐拍傳遞，而權重 $w_1, w_2, w_3, w_4$ 在連續週期透過**同一條零暫存器廣播導線**瞬間送到全部 6 顆 MAC！
       - **瓶頸** `[33:22]`：消滅了加法樹，卻換來一條串接所有 MAC 輸入電容的**高 RC 延遲廣播長線**！

---

## 4. Retiming Lemma：用圖論最短路徑自動把半脈動陣列轉成純脈動陣列

[35:30]（課本 §14.3.1 *The Retiming Lemma* 與圖 14.14，p.726–728）
硬體工程師最常採用的標準流程是：先隨手畫出直覺好懂的「半脈動陣列（圖 14.13）」，再套用 **Retiming Lemma（重定時引理，Leiserson & Saxe）** **全自動**把所有零暫存器廣播線消滅，轉成純脈動陣列！

### 4.1 有向圖建模與重定時不變量

1. **建立電路圖 $G = (V, E)$** `[39:12]`：將圖 14.13 的廣播線改畫成逐級穿透組合單元的導線 `[37:20]`；每顆組合邏輯單元是一個節點，並額外加上一顆代表外部輸出的虛擬終點 **Host 節點** `[40:14]`。每條有向邊 $e = (u \to v)$ 的**邊權重 $w(e)$ 等於該連線上串接的暫存器數量**（半脈動圖必存在 $w(e) = 0$ 的邊；純脈動圖則要求所有邊 $w_{\text{new}}(e) \ge 1$）。
2. **節點延遲移動（Retiming / Lag）** `[41:45]`：
   - 對任意節點 $v$ 施加 $\text{lag}(v) = k$，定義為：**在所有進入 $v$ 的邊加上 $k$ 顆暫存器，同時從所有離開 $v$ 的邊拿掉 $k$ 顆暫存器**。對外界任何穿過 $v$ 的路徑而言一加一減剛好抵銷（$+k - k = 0$），外部行為完全不變！
   - 因此任意邊 $e = (u \to v)$ 重定時後的新暫存器數量為 `[43:09]`：
     $$w_{\text{new}}(u \to v) = w_{\text{old}}(u \to v) + \text{lag}(v) - \text{lag}(u)$$
   - **環路權重守恆** `[45:21]`：沿任意有向環路加總時，所有節點的 $\text{lag}$ 正負兩兩相消，環路總暫存器數永遠不變。

### 4.2 `kG - 1` 最短路徑演算法與圖 14.14 推導

我們希望找到一組整數 $\text{lag}(\cdot)$ 使得每一條邊都滿足：
$$w_{\text{old}}(u \to v) + \text{lag}(v) - \text{lag}(u) \ge 1 \iff \text{lag}(u) \le \big(w_{\text{old}}(u \to v) - 1\big) + \text{lag}(v)$$
這條不等式跟圖論最短路徑的**三角不等式**一模一樣 `[48:03]`！自動轉換四步驟如下 `[46:43]`：

1. **測試減一圖 $G - 1$**：將 $G$ 每條邊權重減 $1$。在我們的範例中，往左傳的輸入邊原為 $1$（減一變 $0$），往右穿的權重廣播邊原為 $0$（減一變 $-1$）——兩者形成權重為 $0 + (-1) = -1$ 的**負權重環（Negative-weight cycle）** `[52:20]`，最短路徑不存在！
2. **倍數降速放大為 $kG$（Slowdown by factor $k$）** `[50:46]`：找最小正整數 $k$ 把原圖每條邊的暫存器數乘上 $k$（代表資料改為每 $k$ 拍才推進一次），直到 $kG - 1$ **不再有負權重環**。本例取 **$k = 2$**：在 $2G - 1$ 中，原權重 $1$ 變成 $2(1)-1 = +1$，原權重 $0$ 變成 $2(0)-1 = -1$，小環路總重變成 $+1 + (-1) = 0 \ge 0$，**負環成功消除** `[52:29]`！
3. **跑 Bellman-Ford 演算法求最短路徑** `[47:19]`：在無負環的 $2G - 1$ 上計算各節點到 Host 的最短路徑長度作為 $\text{lag}(v)$，由右至左依序得出 **`lag = -1, -2, -3, -4, -5, -6`** `[53:05]`。
4. **代回公式擺放暫存器** `[53:51]`：
   - 原本為 $0$ 的權重廣播邊：$w_{\text{new}} = 2(0) + (-1) - (-2) = \mathbf{1}$ 顆暫存器（廣播長線徹底消失，變成逐拍移位！）。
   - 反向輸入移位邊：$w_{\text{new}} = 2(1) + (-2) - (-1) = \mathbf{1}$ 顆暫存器（註：影片 `[54:24]` 投影片上另有同節點對向平行回饋邊因 $\text{lag}$ 相同而變為 $2(1)+0 = 2$ 顆暫存器；課本圖 14.14 則化簡為乾淨的雙向各 1 顆暫存器對流結構）。

### 4.3 解救 50% 閒置氣泡：雙卷積交錯執行（Interleaving）

[58:06]（課本圖 14.14，p.727–728）
- 在轉換出的純脈動陣列（圖 14.14）中，輸入與權重從左右兩端**反向對流**，但因為乘了 $k=2$，每兩個數中間都必須夾一個**空拍氣泡 `□`**（如 `w4 □ w3 □ w2 □ w1` 與 `□ x4 □ x5 □ x6`），才能保證正確元素在正確週期對撞！
- **痛點與神救援** `[59:28]`：插空拍雖然換來超高時脈與零廣播線，卻害每顆 MAC 有 **50% 的週期在發呆**（雖可做時脈閘控省電 `[60:25]`，但算力腰斬）！解法極其漂亮 `[60:46]`：**在所有的 `□` 空拍週期裡，同時塞進「第二組完全獨立的卷積」（配另一組輸出累加暫存器）**！兩組卷積一奇一偶完美錯開互不干擾，硬體利用率瞬間從 **50% 拉回滿載 100%**！（`【課本補充】` p.728 另補充若在資源充裕的 FPGA 上不想管脈動對時，也可用複製移位輸入緩衝區 $B_1, B_2, \dots$ 的 **Direct Product** 直接平行硬算。）

---

## 5. 二維卷積硬體化：串接 1D 脈動方塊 vs 雙塊循環矩陣（`im2col` / GEMM）轉換

[62:55]（課本 §14.3.2 圖 14.15 與 *Matrix-Vector Product*，p.728–730）
有了超高速 1D 卷積引擎，最後一步如何完成 2D 卷積？課本給出兩條截然不同的工業界主流路線：

1. **路線一：串接 1D 脈動卷積方塊（Kung & Song, 1981；課本圖 14.15）** `[62:55]`：
   - 直接呼應 Row Stationary：將輸入影像的三列 $\text{row}(x_1), \text{row}(x_2), \text{row}(x_3)$ 餵進第一個 1D 脈動卷積區塊算出三個部分和加總得 $\text{row}(y_1)$；同時將重疊的 $\text{row}(x_2), \text{row}(x_3)$ 連同新讀入的 $\text{row}(x_4)$ 餵進下一個區塊算 $\text{row}(y_2)$（或回授重用同一個區塊）。
2. **路線二：展開成雙塊循環矩陣（Doubly Block Circulant Matrix / `im2col`），直接調用矩陣乘法硬體（GEMM）** `[63:24]`（課本 p.729–730）：
   - 為什麼要大費周章把卷積變成矩陣乘法？因為 **GPU（如 NVIDIA Tensor Cores）與商用 CPU 早就內建了千錘百鍊、極度最佳化的矩陣／張量乘法硬體單元** `[63:44]`（課本 p.729）！
   - 以 $2 \times 2$ 濾波器 $\mathbf{W}$ 卷積 $3 \times 3$ 輸入 $\mathbf{X}$（產生 $2 \times 2$ 輸出 $\mathbf{Y}$）為例，把 $\mathbf{X}$ 拉平成 $9 \times 1$ 行向量，並將權重加零移位排成 $4 \times 9$ 的**雙塊循環矩陣 $\hat{\mathbf{W}}$**（課本 p.729）：
     $$\mathbf{Y}_{4\times 1} = \underbrace{\begin{bmatrix} w_{11} & w_{12} & 0 & w_{21} & w_{22} & 0 & 0 & 0 & 0 \\ 0 & w_{11} & w_{12} & 0 & w_{21} & w_{22} & 0 & 0 & 0 \\ 0 & 0 & 0 & w_{11} & w_{12} & 0 & w_{21} & w_{22} & 0 \\ 0 & 0 & 0 & 0 & w_{11} & w_{12} & 0 & w_{21} & w_{22} \end{bmatrix}}_{\hat{\mathbf{W}}\ (4 \times 9)} \begin{bmatrix} x_{11} \\ x_{12} \\ \vdots \\ x_{33} \end{bmatrix}_{9\times 1}$$
   - ⚠️ **影片投影片 `[64:44]–[65:55]` 與課本 p.729 的對偶寫法差異**：課本 p.729 固定拉平輸入 $\mathbf{X}_{9\times 1}$、將權重展開為 $4 \times 9$ 的 $\hat{\mathbf{W}}$；影片投影片則反過來把輸入展開成左邊的滑窗矩陣 $\hat{\mathbf{X}}$（類似深度學習框架主流的 **`im2col`**），右邊放補零後的 $8 \times 1$ 權重向量相乘（投影片省略了最後一個對齊零元素故寫成 $4 \times 8$ 乘 $8 \times 1$）。若將多個輸出通道與多個滑窗排成稠密矩陣（無補零浪費），就是把 $C \times R \times S$ 滑窗攤平為矩陣列、一次用 **Matrix-Matrix Multiplication（GEMM）** 算完所有通道與像素！代價是展開矩陣會造成記憶體重複儲存膨脹，但換取直接榨乾張量硬體的恐怖算力。

---

## 6. 【實測】Bellman-Ford 重定時與雙流交錯 100% 脈動模擬，及 RTX 4060 Tensor Core 30× 矩陣加速

我們撰寫 [`experiments/lecture57/systolic_and_tensorcore.cu`](experiments/lecture57/systolic_and_tensorcore.cu)，在本機 CPU 與 **NVIDIA GeForce RTX 4060 Laptop GPU** 上實測驗證本講上下半場兩大核心機制：
1. **自動重定時與圖 14.14 純脈動陣列逐拍模擬**：把圖 14.13 的 6-PE 權重廣播半脈動陣列建成有向圖 $G$，直接跑 **Bellman-Ford 演算法**檢測 $G - 1$ 的負環、在 $2G - 1$ 算出與老師 `[53:05]` 投影片完全一致的 `lags = [-1..-6]`，並逐拍模擬圖 14.14 在「單一卷積流（含 `□` 空拍）」與「奇偶交錯雙卷積流」下的真實輸出與 MAC 利用率。
2. **課本 §14.3.2 矩陣轉換 + 硬體 Tensor Cores 實測**：針對 $N=16, C=128, K=128, 56 \times 56, 3 \times 3$ 卷積層（`14.80 GFLOPs`），對比：(a) 直接在一般 FP32 CUDA Cores 跑 2D 卷積；(b) 展開 `im2col` 後用 `nvcuda::wmma` 16×16×16 Tensor Core 硬體指令但**不加 Shared Memory 快取重用（NLR 模式）**；(c) 加上完整 Shared Memory + Register Tiling 的 **cuBLAS Tensor Core GEMM**！

Tensor Core `wmma` 16×16×16 矩陣乘加核心節錄如下（`experiments/lecture57/systolic_and_tensorcore.cu:216-235`）：

```cuda
__global__ void wmma_nlr_gemm(const half *__restrict__ A,
                              const half *__restrict__ B_col_major,
                              float *__restrict__ C_out,
                              int M, int N_cols, int K_red) {
    int warpM = (blockIdx.x * blockDim.x + threadIdx.x) / 32;
    int warpN = blockIdx.y;
    int row = warpM * 16, col = warpN * 16;
    if (row >= M || col >= N_cols) return;

    wmma::fragment<wmma::matrix_a, 16, 16, 16, half, wmma::row_major> a_frag;
    wmma::fragment<wmma::matrix_b, 16, 16, 16, half, wmma::col_major> b_frag;
    wmma::fragment<wmma::accumulator, 16, 16, 16, float> acc_frag;
    wmma::fill_fragment(acc_frag, 0.0f);

    for (int k = 0; k < K_red; k += 16) {
        wmma::load_matrix_sync(a_frag, A + (size_t)row * K_red + k, K_red);
        wmma::load_matrix_sync(b_frag, B_col_major + (size_t)col * K_red + k, K_red);
        wmma::mma_sync(acc_frag, a_frag, b_frag, acc_frag);
    }
    wmma::store_matrix_sync(C_out + (size_t)row * N_cols + col, acc_frag, N_cols, wmma::mem_row_major);
}
```

編譯執行 `nvcc -O3 -arch=sm_89 systolic_and_tensorcore.cu -lcublas` 的實測輸出：

```text
=== 1. Retiming Lemma (Bellman-Ford on G-1 vs 2G-1, Fig. 14.13 -> 14.14) ===
  Check k=1 (G - 1) : NEGATIVE CYCLE DETECTED (0 + (-1) = -1) -> cannot retime at k=1!
  Check k=2 (2G - 1): NO NEGATIVE CYCLES | lags(PE1..PE6) = [-1, -2, -3, -4, -5, -6]
  Retimed 2G edges  : w_bcast (0 -> 1 reg), x_shift (1 -> 1 reg), min_reg=1 (PURE SYSTOLIC!)
  Fig. 14.14 Output : StreamA=EXACT [25,33,41,49,57,65] | StreamB=EXACT [40,28,22,12,15,26]
  Steady-State MAC Utilization (t=6..7): Single-Conv =  50.0% | 2-Way Interleaved = 100.0%

=== 2. §14.3.2 Matrix-Product Lowering + Hardware Tensor Cores on NVIDIA GeForce RTX 4060 Laptop GPU ===
  Memory Footprint : Original ifmap = 12.25 MiB -> Unrolled im2col = 110.25 MiB (9.0x expansion!)
  (a) Direct 2D Conv (FP32 CUDA Cores)         : 17.869 ms (  828.12 GFLOP/s,  1.00x)
  (b) Unbuffered WMMA Tensor Core (NLR GEMM)   : 12.966 ms ( 1141.26 GFLOP/s,  1.38x)
  (c) Tiled Tensor Core GEMM only (cuBLAS)     :  0.593 ms (24959.83 GFLOP/s, 30.14x)
  (d) im2col (3.943 ms) + Tiled TC GEMM Total  :  4.536 ms ( 3262.38 GFLOP/s,  3.94x) | MaxDiff=2.25e-04
```

**實測觀察與硬體印證**：
1. **Retiming Lemma 與雙流交錯零誤差重現**：Bellman-Ford 在 $G-1$ 立刻抓出 `0 + (-1) = -1` 負權重環，放大至 $2G-1$ 後算出的 `lags = [-1, -2, -3, -4, -5, -6]` 與老師 `[53:05]` 板書數字分毫不差，所有廣播邊皆插入 `1` 顆暫存器成為純脈動陣列；在滿載視窗中單一卷積利用率正好掉到 **`50.0%`**，而在奇偶空拍塞入 `StreamB` 後兩組卷積皆算出精確答案，MAC 利用率完美衝回 **`100.0%`**！
2. **沒有資料重用（NLR），再強的張量硬體也會餓死**：對照 `(b)` 與 `(c)` 最為震撼——兩者同樣調用 RTX 4060 的第四代硬體 Tensor Cores，但 `(b)` 採用無局部快取的 NLR 模式直接從 GDDR6 抓 16×16 矩陣塊，算力被記憶體頻寬死死卡在 `1.14 TFLOP/s`（僅比一般 CUDA core 快 `1.38×`）；一旦 `(c)` 補上 Shared Memory 空間重用與暫存器時間重用，Tensor Core 矩陣乘法瞬間飆出 **`24.96 TFLOP/s`（`0.593 ms`，比直接卷積快了整整 `30.14×`！）**。
3. **`im2col` 的 $R \times S = 9\times$ 記憶體膨脹痛點**：展開 $3 \times 3$ 滑窗讓原本 `12.25 MiB` 的輸入特徵圖暴增 **9 倍**成 `110.25 MiB`，光寫出這張展開矩陣就花了 `3.943 ms`（占總時間 `4.536 ms` 的 **87%**！這也是為何現代 cuDNN 進一步發展出在晶片內 Shared Memory 即時做 Implicit GEMM，並引出下一講第 58 講的「演算法與記憶體最佳化」）。

---

## 7. 一頁速查表

| 概念／架構 | 核心機制或公式 | 優劣勢與硬體定位 |
|---|---|---|
| **Broadcast vs Systolic** | 單拍全域長導線 vs 每拍經暫存器傳相鄰 PE | 廣播受制於 Clock Skew 與並聯電容 RC 延遲；脈動傳遞短線高頻、極易擴展 |
| **WS / IS / OS 2D 陣列** | 列／行分別映射 `(c, k)` 或 `(切塊, k/c)` | 結合 PE 內 LB 時間重用 ＋ 沿列／行串流或脈動累加部分和的空間重用 |
| **RS（Row Stationary）** | 駐留濾波器列 $r_j$ 與對角滑動輸入列 $h_i$（MIT Eyeriss） | 將 2D 卷積化為高速 **1D 卷積**＋垂直歸約；免跨行鎖步且可重疊載入與加總 |
| **NLR（No Local Reuse）** | PE 內零快取，全靠 GB 供料＋水平脈動累加 | 單晶片可塞最多 PE，但極度依賴超大頻寬 Global Buffer 與 NoC |
| **Semi-systolic vs Systolic** | 相鄰組合邏輯間無暫存器（$w=0$） vs 必有暫存器（$w \ge 1$） | 半脈動易卡在加法樹或廣播 RC；純脈動保證每拍僅單級邏輯、時脈最高 |
| **Retiming Lemma** | 取最小 $k$ 使 $kG-1$ 無負環，跑 Bellman-Ford 求 $\text{lag}(v)$ | 自動配置 $w_{\text{new}} = k w_{\text{old}} + \text{lag}(v) - \text{lag}(u) \ge 1$；空拍交錯第 2 組卷積可救回 100% 吞吐量 |
| **矩陣展開（`im2col` / $\hat{\mathbf{W}}$）** | 展開為雙塊循環／滑窗矩陣轉成標準 GEMM | 付出 $R\times S$ 倍冗餘儲存代價，換取直接調用超高吞吐量 **GPU Tensor Cores**（實測 GEMM 快 `30×`） |

---

## 8. 與後續講次的連結

- **往回對照 [第 20 支](lecture20_note.md)（GPU Tensor Cores 伏筆）與 [第 56 支](lecture56_note.md)（迴圈映射代數）**：上一講紙上談兵的 `WS / IS / OS / RS` 四條式子，在本講加上空間重用後全數化為具體的 2D PE 陣列；而 §14.3.2 的矩陣轉換則完美解答了為何專攻矩陣乘法的 GPU Tensor Cores 能統治卷積視覺網路。
- **往下接 [第 58 支](lecture58_note.md)（Part IV 全課程最終講：演算法／記憶體壓縮最佳化與 ReRAM 記憶體內計算）**：本講最後指出純脈動陣列雖然規整高速，但結構僵化——如果權重與 ReLU 後的輸入有**超過一半都是 `0`（稀疏性 Sparsity）**，脈動陣列還是會傻傻把 `0` 乘進去白燒電！全系列最後一講將揭曉如何跳過零值、降低位元精度、壓縮頻寬，乃至直接用 **ReRAM 歐姆定律與克希荷夫定律在記憶體陣列內一拍算完類比內積**！

---

## 附：本講一句話總結

> 大型機器學習加速器藉由「脈動傳遞（Systolic Transfer）」取代高 RC 延遲的全域廣播以實現跨 PE 空間重用，對內可透過 Retiming Lemma 最短路徑演算法將半脈動電路自動轉為雙流交錯 100% 滿載的高頻純脈動陣列，對外則能將 2D 卷積展開為標準矩陣乘法直接解放現代 GPU Tensor Cores 數十倍的張量算力。

---

*筆記整理自 Smruti R. Sarangi 教授 Advanced Computer Architecture 第 57 講影片與 Next-Gen Computer Architecture v3.2 第 14 章 §14.2.3–14.3.2，搭配本機 Bellman-Ford 重定時模擬與 RTX 4060 Tensor Cores 實測驗證。*
