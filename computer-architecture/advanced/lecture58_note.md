# Advanced Computer Architecture 第五十八支影片完整筆記：機器學習硬體架構（四）：推論最佳化、位元串列乘法、記憶體系統與記憶體內類比運算（Chapter 14, Part IV — 全課程完結篇）

> **課程**：Advanced Computer Architecture — Chapter 14: Architectures for Machine Learning (Part IV) -- Optimizations and Memory Systems（播放清單第 58 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（38:54）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch14** §14.4–14.6（p.730–738）
> **章號**：影片 Chapter 14 = 課本 Ch14（亦為全書與整個播放清單最後一講）。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、NVIDIA GeForce RTX 4060 Laptop GPU、`nvcc` 12.4 + cuBLAS INT8 Tensor Cores、Ubuntu、kernel 6.5.0）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕把「ReLU layer / ReLU function / ReLU layers」寫成「real layer／value function／radio layers」、「ifmap / ofmap(s)」寫成「if M map／of map／OS map(s)」、「coarse idea」寫成「course idea」、「the systolic array / systolic array / systolic arrays」寫成「this historic array／systolically／systolic Aries」、「doubly block circulant matrix」寫成「W block Circle and Matrix」、「1D convolution」寫成「only convolution」、「sparsity」寫成「parsit／sparse it」、「a miss」寫成「a mess」、「prefetch」寫成「prefet」、「inferencing」（`[27:48]`–`[27:59]`）寫成「interference」、「ReRAMs」寫成「relamps／re-rams」、「pooling layer」寫成「pulling array」、「prune it」寫成「Pro net」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 為什麼要對推論硬體動刀？百億級參數、無效運算（Ineffectual Operations）與定點數](#1-為什麼要對推論硬體動刀百億級參數無效運算ineffectual-operations與定點數)
- [2. 縮短運算時間的最佳化：共同子表達式消除（CSE）、跳零／提早終止 ReLU 與二元神經網路（BCNN）](#2-縮短運算時間的最佳化共同子表達式消除cse跳零提早終止-relu-與二元神經網路bcnn)
- [3. 縮短記憶體存取與可變精度：Base-Delta 壓縮、位元串列乘法（1D 位元卷積）與零區塊跳過](#3-縮短記憶體存取與可變精度base-delta-壓縮位元串列乘法1d-位元卷積與零區塊跳過)
- [4. 加速器記憶體系統組織：32-Bank Global Buffer、Push 式預取（Listing 14.2）與 3D 堆疊記憶體](#4-加速器記憶體系統組織32-bank-global-bufferpush-式預取listing-142與-3d-堆疊記憶體)
- [5. 記憶體內運算（PIM）與神經形態類比交叉陣列：電荷共享（Charge Sharing）vs 電流加總（Current Summing / ReRAM）](#5-記憶體內運算pim與神經形態類比交叉陣列電荷共享charge-sharingvs-電流加總current-summing--reram)
- [6. 全章與全書總結（§14.6 Summary）](#6-全章與全書總結146-summary)
- [7. 【實測】跳零＋MSB 提早 ReLU、1D 位元卷積、RTX 4060 INT8 Tensor Core 406× 加速與 ReRAM 雙行交叉陣列模擬](#7-實測跳零msb-提早-relu1d-位元卷積rtx-4060-int8-tensor-core-406-加速與-reram-雙行交叉陣列模擬)
- [8. 一頁速查表](#8-一頁速查表)
- [9. 與全書前後講次的總連結（全課程完結篇）](#9-與全書前後講次的總連結全課程完結篇)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
前三講造出了高吞吐量的 2D 空間資料流與脈動陣列；本講解決最後兩大實戰瓶頸：無效算力浪費與記憶體頻寬牆（§14.4–14.6）
  │
  ├── 一、減少算術運算時間（§14.4.1 Compute-Time Optimizations）
  │   ├── 共同子表達式消除（CSE）：重複權重先加輸入再乘 w(x1+x2)+w'(x3+x4)+w''(x5+x6+x7) → 7 次乘法降為 3 次
  │   ├── 跳零與提早 ReLU 終止：1-bit 稀疏遮罩跳過 ×0；後接 ReLU 時先乘 MSB 估計正負號，若必為負直接輸出 0
  │   ├── 【口頭】二元卷積網路（BCNN）：{0,1} 輸入與權重 → 乘法退化為 bitwise AND、加法退化為 OR
  │   └── 架構相容性：剛性純脈動陣列難以跳零；Row Stationary（Eyeriss）與雙塊循環稀疏矩陣代數天生契合
  │
  ├── 二、減少記憶體頻寬與支援任意精度（§14.4.2 Memory-Access & Precision Optimizations）
  │   ├── Base-Delta 壓縮（Wang et al., 2016）：存均值 base + 窄位寬差值 Δ；【口頭】分配律 (base+Δ)·w 免解壓直接算！
  │   ├── 動態零位元圖索引（Dynamic Zero Bitmap）：在記憶體建立非零索引，為 0 的元素連抓進快取都省掉
  │   └── 位元串列乘法器（Bit-Serial Multiplier）：
  │       ├── 核心洞察：直式二進位乘法的每一直欄加總 ＝ 兩個位元向量的 **1D 卷積**（外加進位傳遞 Carry Ripple）！
  │       └── 同一套 1D 脈動硬體即可通吃任意位寬（O(N) 延遲換極小面積功耗），切成 m-bit 區塊還能直接跳過全 0 區塊
  │
  ├── 三、數位記憶體階層設計（§14.5.1 DRAM + Multi-Banked SRAM）
  │   ├── 16×16 OS 陣列每拍需 16 路輸入 + 16 路權重 → 必須配 **32-Bank Global Buffer** 平行供料
  │   ├── 拋棄傳統快取未命中才拉資料的 Pull 模式 → 改用 DNN 編譯器 + Dispatcher FSM 的 **Push 式預取**（Listing 14.2）
  │   └── 突破晶片外 DRAM 頻寬牆：eDRAM、HBM / HMC 3D 堆疊記憶體（每個 Vault 配專屬 PE）
  │
  └── 四、終極突破：記憶體內類比運算（§14.5.2 Processing-in-Memory / Neuromorphic Crossbars，圖 14.16）
      ├── Wordline 電壓編碼輸入 x_i、Cell 內部物理量編碼權重 w_i，單條 Bitline 一次完成內積 ∑ x_i w_i → ADC 轉數位
      ├── 電荷共享（Charge Sharing，圖 14.16a）：Q_i = V_i C_i，開關閉合後 V_bitline = (∑ Q_i) / C_lumped（抗雜訊較佳）
      └── 電流加總（Current Summing，圖 14.16b）：歐姆定律 I_i = V_i·(1/R_i) + 克希荷夫電流定律加總；用 **ReRAM** 可動態改寫權重，負權重拆成正負兩行相減
```

---

## 1. 為什麼要對推論硬體動刀？百億級參數、無效運算（Ineffectual Operations）與定點數

`[00:12]` 進入第 14 章第 4 支影片（也是整門課最後一支影片）。Sarangi 教授一開場就點出規模壓力：

- `【口頭】` **百億參數時代的推論延遲與頻寬壓力** `[00:22]`：小型 CNN 已有數百萬個權重，而大型模型規模更是驚人——老師口頭舉例「像 ChatGPT / GPT-4 據稱有 1000 億到 1750 億個權重」（⚠️ 註：`175 billion` 精確對應 OpenAI 公開的 GPT-3 參數規模；GPT-4 官方未公開確切參數數，業界推估為破兆參數的 MoE 架構，老師此處口頭混稱為 `ChatGPT-4` 以強調百億級以上規模）。訓練可以在大型叢集跑上數週，但**推論（Inferencing）**必須在終端裝置或線上毫秒內完成，因此必須同時砍掉**無效算術**與**記憶體頻寬／容量** `[00:38]`–`[01:50]`。
- **第一刀：消滅無效運算（Ineffectual Operations）** `[00:53]`（§14.4 導言，p.730）：
  1. **乘以零**：任何輸入或權重為 $0$ 的 MAC（$\times 0$）結果必為 $0$，根本不必動用乘法器 `[01:05]`。
  2. **ReLU 前的負輸出**：卷積層後面通常緊接 $\text{ReLU}(y) = \max(0, y)$；只要能預判某個輸出像素（或整塊區域）算出來必為負數，就可以直接填 $0$ 跳過，完全不影響最終結果 `[01:14]`。
- `【口頭】` **第二刀：告別浮點數，改用定點數（Fixed-Point）與動態量化（Quantization）** `[01:52]`–`[03:58]`：
  - 神經網路推論極少使用雙精度（FP64）甚至標準單精度浮點數（FP32）；即便使用浮點，也會壓縮指數與尾數位數（如 BF16 / FP16 / FP8）`[01:59]`。
  - 硬體最偏愛**定點數（Fixed-Point）**（老師口頭舉例如十進位概念的 `44.20` `[02:25]`）：底層直接存成窄整數、用廉價整數 ALU 運算，本質上就等同於把連續區間離散化的**量化（Quantization）** `[02:45]`。
  - 更進一步，連同一張 `ifmap`／`ofmap` 內部對誤差敏感度不同的區域，或網路前後不同層，都可以動態調整位元寬度（混合精度） `[02:51]`–`[03:05]`。

---

## 2. 縮短運算時間的最佳化：共同子表達式消除（CSE）、跳零／提早終止 ReLU 與二元神經網路（BCNN）

### 2.1 共同子表達式消除（Common Subexpression Elimination, CSE）

`[04:01]`（§14.4.1，p.730）當濾波器權重經過量化後，同一個濾波器內很容易出現重複的權重數值 $w, w', w''$。假設某段卷積展開式為 `[04:14]`：

$$w x_1 + w x_2 + w' x_3 + w' x_4 + w'' x_5 + w'' x_6 + w'' x_7$$

- **天真算術** `[04:34]`：需要 **7 次乘法**與 **6 次加法**。
- **提出公因數後** `[04:44]`：改寫為

$$w(x_1 + x_2) + w'(x_3 + x_4) + w''(x_5 + x_6 + x_7)$$

  加法次數維持 **6 次不變**，但昂貴又耗電的乘法直接從 **7 次降為 3 次**（省下 $57\%$ 乘法） `[05:02]`！
- 這項轉換可由 DNN 編譯器靜態排程，也可在具備可重組能力（FPGA 或彈性 ASIC）的硬體內動態偵測執行；`【課本補充】`（p.730）若多個不同濾波器剛好在同一相對位置共享重複權重模式，連先算好的 $(x_1 + x_2)$ 輸入部分和都能跨濾波器重複使用。

### 2.2 稀疏遮罩跳零與 MSB 提早終止 ReLU

`[05:28]`（§14.4.1，p.730–731）為什麼 CNN 裡到處都是 $0$？因為前一層的 **ReLU 會把所有小於 $0$ 的輸出全部截斷成 $0$**（常見 `ifmap` 稀疏度高達 $40\%\sim 70\%$），再加上剪枝（Pruning）後的濾波器也有大量零權重 `[06:24]`–`[07:17]`。硬體可採取三層最佳化：

1. **1-bit 非零遮罩（Bit-Vector / Bit-Matrix）** `[05:45]`：為濾波器與 `ifmap` 各維護一個位元遮罩（`0` 代表零、`1` 代表非零）；只有當對應位置的兩個遮罩位元皆為 `1`（bitwise `AND` 為 `1`）時才派發乘法，否則直接跳過 `[06:12]`。
2. **中途負值終止與 MSB 粗略符號預測（Early ReLU Termination）** `[07:20]`–`[08:40]`：若本層輸出緊接 ReLU，當累加到一半發現部分和已經深陷負值、後續不可能翻正時，可立刻中止內積並輸出 $0$ `[07:38]`。更聰明的做法是**不要一次做完整寬度乘法，先只拿輸入與權重的最高有效位元（MSBs）相乘累加** `[07:55]`：只花極小位元成本就能算出粗略估值（Coarse Estimate），一旦判定趨勢為負，整顆輸出像素的剩餘低位元 MAC 全部免算，直接輸出 $0$！
3. `【課本補充】` **跳零帶來的硬體同步難題：執行時間非確定性（Nondeterminism）**（p.730–731）：一旦 PE 依資料內容跳過不同數量的零，各 PE 完成一個 Tile 的時間就不再一致；由於全體區塊必須在同步點等待**最慢的那顆 PE**，若不搭配**動態工作分配（Dynamic Work Apportioning）**平衡負載，省下的算力就會變成空等氣泡。

### 2.3 【口頭】二元卷積神經網路（Binary CNN, BCNN）與硬體架構相容性

- `【口頭】` **把量化推向極致的 BCNN** `[08:42]`–`[10:28]`：如果連 8-bit / 4-bit 定點數都嫌寬，直接把所有 `ifmap` 像素與濾波器權重都壓成 **1-bit 二元值 $\{0, 1\}$**，會發生什麼事？
  - 原本昂貴的乘法器 $x_i \cdot w_i$ 瞬間退化成一顆 **bitwise `AND` 閘** `[09:35]`、`[10:12]`！
  - 累加運算在布林邏輯近似（如影像邊緣偵測 Edge Detection 等形態學特徵）下甚至可退化成 **`OR` 閘** `[09:37]`、`[10:21]`（註：若採另一種主流二元神經網路 BNN 編碼 $\{+1, -1\}$，則乘法退化為 `XNOR` 閘、累加退化為計算 `1` 個數的 `popcount` 計數器）。無論哪一種，儲存空間與運算功耗都能壓低一個數量級以上。
- **哪種硬體架構最適合吃稀疏性？** `[10:29]`–`[12:34]`：
  - **純脈動陣列（Systolic Array）最難配合** `[10:34]`：如[第 57 支](lecture57_note.md)所見，脈動陣列靠嚴格鎖步（Lockstep）節拍讓資料對撞；定點量化放進脈動陣列完全沒問題 `[10:46]`，但若要動態跳過不規則的零，鎖步節拍就會大亂。
  - **兩條友善路線**：一是採用 MIT Eyeriss 的 **Row Stationary（RS）** 架構 `[11:09]`，以列為單位做稀疏壓縮編碼與彈性路由；二是回到[第 57 支](lecture57_note.md) §5.2 的**雙塊循環矩陣（Doubly Block Circulant Matrix）**視角 `[11:51]`，一旦把 CNN 轉成標準線性代數，就能直接套用發展數十年的**稀疏矩陣乘法（SpGEMM / SpMV）**軟硬體演算法！

---

## 3. 縮短記憶體存取與可變精度：Base-Delta 壓縮、位元串列乘法（1D 位元卷積）與零區塊跳過

### 3.1 Base-Delta 壓縮與免解壓分配律技巧

`[12:35]`（§14.4.2，p.731）自然影像（例如大片藍天像素）與同一區塊內的濾波器權重往往高度聚集在平均值附近。**Base-Delta 壓縮**（Wang et al., 2016）只存一個區塊基準值（`base`），其餘元素只存相對於 `base` 的窄位寬差值（`delta`）：

- **影片口頭數值範例** `[12:58]`：四個數值 `[100, 101, 103, 104]`，取均值 `base = 102`，四個差值就變成極小的 `[-2, -1, +1, +2]`（原本每個數要 8 bits 共 32 bits，現在存一個 8-bit `102` 加四個 3-bit 有號差值只要 $8 + 4 \times 3 = 20\text{ bits}$，省下 $37.5\%$）。
- `【課本補充】` **課本數值範例**（p.731）：四個 4-bit 數值 `[10, 11, 12, 13]`（原需 $4 \times 4 = 16\text{ bits}$），取 4-bit `base = 12`，差值 `[-2, -1, 0, +1]` 剛好落入 2-bit 二補數範圍 $[-2, +1]$，總位元數降為 $4 + 4 \times 2 = 12\text{ bits}$（省下 $25\%$）。但課本也提醒：若區塊內數值離散度太大，差值位寬無法縮小，就沒有壓縮效益。
- `【口頭】` **免解壓縮直接算內積（乘法分配律）** `[13:39]`–`[14:18]`：把壓縮資料讀進 PE 後，難道還要花額外加法器與功耗先還原出 `base + Δ_i` 才能乘權重嗎？不必！利用乘法分配律：

$$\sum_{i} (\text{base} + \Delta_i)\, w_i = \text{base} \cdot \left(\sum_{i} w_i\right) + \sum_{i} \Delta_i w_i$$

  權重總和 $\sum w_i$ 可以預先算好（或固定權重共用一次 `base` 乘法），迴圈內只需要用**窄位寬乘法器**計算極小的 $\Delta_i \times w_i$！再搭配**動態零位元圖索引（Dynamic Zero Bitmap）** `[14:22]`（p.731）把零元素擋在記憶體之外連抓都不抓，就能同時砍掉記憶體容量、匯流排頻寬與乘法位寬。

### 3.2 動態精度的硬體困境與「二進位乘法 ＝ 1D 位元卷積」

`[14:45]`（§14.4.2，p.731–732）前面說過網路的前段層通常需要較高精度、後段層只需低精度 `[15:12]`，甚至不同濾波器需要不同位寬。但**傳統 ASIC 與 FPGA DSP 裡面的乘法器都是固定位寬（如固定 16-bit 或 8-bit）**（p.731）：把 4-bit 資料餵進 16-bit 平行乘法器，多出來的 12 bits 電路照樣佔面積、也無法自動變成四倍吞吐量。

怎麼做出**硬體不用重畫、卻能天然適應任意位元寬度 $N$** 的乘法器？Sarangi 教授給出本講最漂亮的一個數學洞察——**位元串列乘法器（Bit-Serial Multiplier）就是一維卷積** `[15:46]`–`[20:27]`！

觀察國小直式二進位乘法（設 4-bit 被乘數 $A = a_4 a_3 a_2 a_1$、乘數 $B = b_4 b_3 b_2 b_1$，依老師投影片由右至左編號 $1 \dots 4$ `[16:32]`）：

```text
                a4     a3     a2     a1
          ×     b4     b3     b2     b1
        ---------------------------------
              a4·b1  a3·b1  a2·b1  a1·b1
       a4·b2  a3·b2  a2·b2  a1·b2
a4·b3  a3·b3  a2·b3  a1·b3
... (下略 b4 列)
```

看中間下標和等於 $5$ 的那一直欄 `[16:46]`–`[17:42]`：由上往下加總的四個部分積正好是

$$c_5 = a_4 b_1 + a_3 b_2 + a_2 b_3 + a_1 b_4 \quad (4+1 = 3+2 = 2+3 = 1+4 = 5)$$

再看左邊下標和等於 $6$ 的那一直欄 `[17:46]`–`[18:17]`，則是 $b_2 a_4 + b_3 a_3 + b_4 a_2$！換成標準 0-based 位元索引 $A = \sum_{i=0}^{N-1} a_i 2^i$、$B = \sum_{j=0}^{N-1} b_j 2^j$：

$$A \times B = \sum_{k=0}^{2N-2} \underbrace{\left(\sum_{i+j=k} a_i b_j\right)}_{\text{位元向量的 1D 卷積 } (a * b)[k]} 2^k$$

- **結論** `[18:45]`–`[20:27]`：直式乘法每一直欄的部分積加總，完完全全就是把兩個數字看成位元向量後的**一維卷積（1D Convolution）**！唯一跟標準 1D 卷積不同之處，只在於算完各欄卷積和之後，最後要讓大於 $1$ 的數值向左做一次**進位傳遞（Carry Ripple）** `[19:29]`–`[19:51]`。
- **硬體威力與取捨**（p.732）：既然乘法就是位元向量的 1D 卷積，我們直接拿[第 57 支](lecture57_note.md)設計好的 **1D 脈動卷積陣列**就能逐位元串流算出**任意精度 $N$** 的乘積！
  - `【課本補充】` **延遲 vs 面積／功耗權衡**（p.732）：傳統平行樹狀乘法器延遲為 $O(\log N)$，位元串列乘法器逐位元推進需要 **$O(N)$ 時間**；但它換來了極小的矽面積、極低功耗，以及跨層任意切換位寬的完全彈性。
- **$m$-bit 分塊跳零（Chunk / Zero Prefix-Suffix Skipping）** `[21:19]`–`[23:04]`（p.732）：為了把位元級稀疏性也榨乾，可把乘數 $B$ 切成多個 $m$-bit 小區塊（課本 p.732 亦補充可偵測每個區塊頭尾的全零 Prefix 與 Suffix 長度）：若某個 $m$-bit 區塊全為 `0`，整塊 1D 位元卷積直接跳過不跑；只對非零區塊跑 1D 位元脈動卷積，最後再依位元位移左移（Left-shift）並經加法樹合併部分積！

---

## 4. 加速器記憶體系統組織：32-Bank Global Buffer、Push 式預取（Listing 14.2）與 3D 堆疊記憶體

`[23:05]`（§14.5.1，p.732–733）算術單元再快，餵不飽也是枉然。在 CNN 加速器中，單筆存取延遲（Latency）不是重點，**持續吞吐頻寬（Sustained Throughput / Bandwidth）**才是生死線：

- **為什麼 $16 \times 16$ 陣列需要 32-Bank Global Buffer？** `[23:44]`（p.732）：回顧[第 57 支](lecture57_note.md)圖 14.9 的 $16 \times 16$ Output Stationary（OS）PE 陣列——每一拍需要從左邊界同時餵入 **16 條輸入串流**，並從上邊界同時餵入 **16 條濾波器權重串流**，合計每週期 **32 筆平行讀取**！單埠 SRAM 絕對辦不到，因此晶片上的 Global Buffer（地位類似 CPU 的 L2 Cache `[24:01]`）必須切分為 **32 個獨立 SRAM Banks**（16 個 Banks 放輸入、16 個 Banks 放權重），每拍 32 個 Banks 同步吐資料。
- **Pull 模式 vs Push 模式預取（Listing 14.2）** `[24:09]`–`[27:38]`（p.732–733）：
  - **傳統 CPU 的 Pull-based（需求驅動）**：等快取未命中（Cache Miss）才去下一層拉資料，會讓整個鎖步 PE 陣列停擺，在 CNN 中完全不切實際 `[24:15]`。
  - **DNN 專屬的 Push-based（推播預取）**：CNN 的七層迴圈存取軌跡幾乎完全確定（唯一的輕微非確定性只有稀疏性 `[25:21]`）。因此由 **DNN 編譯器**在程式碼恰當的迴圈層級插入預取指令，交給加速器控制核心（`【口頭】` 老師比喻為加速器大腦的一顆小型 OOO 處理器／Dispatcher，課本 p.732 則指出可用專用有限狀態機 **FSM** 實作 `[24:41]`）在資料即將被用到的前一刻精準推入 Global Buffer / Local Buffer，用完即丟、絕不提早污染快取 `[26:58]`！

課本 Listing 14.2（p.733）展示了對應 OS 映射 $n \triangleright h'\parallel w'\parallel \triangleright k\parallel c \triangleright r \triangleright s \triangleright h'' \triangleright w''$ 的編譯器預取註記位置：

```c
/* 課本 Listing 14.2 (p.733): OS 架構 2D 卷積與記憶體預取指示點 */
for (n = 0; n < N; n++) {                     /* input images */
    // [預取至 DRAM] 載入第 n 張影像 I[n]
    parallel for (h1 = 0; h1 < H; h1 += Th) { /* rows: ifmap / ofmap */
    parallel for (w1 = 0; w1 < W; w1 += Tw) { /* cols: ifmap / ofmap */
        // [駐留於各 PE Local Buffer] 為每個 k 載入輸出切塊 O[n][k][h1][w1]
        parallel for (k = 0; k < K; k++) {    /* outputs */
            for (c = 0; c < C; c++) {         /* input channels */
                // [Push 預取至 Global Buffer] 載入 F[k][c] 與 I[n][c] 準備串流對撞
                for (r = 0; r < R; r++)
                    for (s = 0; s < S; s++)
                        for (h2 = h1; h2 < h1 + Th; h2++)
                            for (w2 = w1; w2 < w1 + Tw; w2++)
                                O[n][k][h2][w2] += I[n][c][h2 + r][w2 + s] * F[k][c][r][s];
}}}}}
```

- `【課本補充】` **3D 堆疊高頻寬記憶體（HBM / HMC）對接**（p.733）：傳統晶片外 DDR DRAM 頻寬往往跟不上數百顆 PE；現代 AI 加速器普遍透過矽中介層（Interposer）整合 **eDRAM、HBM 或 Hybrid Memory Cube（HMC）**（呼應第 10 章 §10.5.6，見[第 45 支](lecture45_note.md)），並在每個 3D 記憶體垂直通道（**Vault**）底下直接配置對應的 PE 就近供料！

---

## 5. 記憶體內運算（PIM）與神經形態類比交叉陣列：電荷共享（Charge Sharing）vs 電流加總（Current Summing / ReRAM）

`[27:40]`（§14.5.2，圖 14.16，p.733–735）就算用上 32-Bank SRAM 與 HBM，資料還是得不斷搬進 PE 再寫回。針對終端裝置上的**推論（Inferencing）**，能不能乾脆不搬資料，讓記憶體陣列自己利用基礎電路物理定律在原地把內積 $\sum_{i=1}^n x_i w_i$ 算完？這就是**記憶體內運算（Processing-in-Memory, PIM）**與**神經形態電路（Neuromorphic Circuits）** `[28:03]`–`[29:16]`：

把記憶體陣列的每一列字元線（Wordline）透過 DAC 施加代表輸入像素 $x_i$ 的類比電壓 $V_i$，把濾波器權重 $w_i$ 直接編碼成記憶胞內部的電容或電導，整條位元線（Bitline）瞬間就能平行完成一整欄的乘加運算，最後再用一顆 **ADC（類比數位轉換器）** 量出數位結果：

```
        圖 14.16(a) 電荷共享 (Charge Sharing)        圖 14.16(b) 電流加總 (Current Summing)
        Wordline x_i ──► [ 類比乘法 / 電容 C_i ]       Wordline V_i (=x_i) ──► [ 可變電阻 R_i (G_i=1/R_i=w_i) ]
                              │ (開關閉合共享電荷)                                     │ (I_i = V_i · G_i 匯入位元線)
                              ▼                                                        ▼
                     Bitline: V_bl = (∑ Q_i)/C_lumped                         Bitline: I_total = ∑ V_i G_i
                              │                                                        │ (流經感測電阻轉電壓)
                              ▼                                                        ▼
                           [ ADC ]                                                  [ ADC ]
```

### 5.1 電荷共享（Charge Sharing，圖 14.16a）

- **物理乘法與加法** `[29:18]`–`[33:38]`（p.734）：利用電容公式 $Q_i = V_i C_i$ 實現乘法——可以固定電容 $C$、用輕量類比電壓乘法器產生代表 $x_i w_i$ 的電壓，或者把輸入編碼為電壓 $V_i$、把固定權重編碼為電容值 $C_i$ `[29:55]`。接著把同一條 Bitline 上所有記憶胞的開關同時閉合，各電容儲存的電荷 $Q_i$ 會在並聯的位元線上重新分配：

$$\sum_{i=1}^{n} Q_i = V_{\text{bitline}} \cdot C_{\text{lumped}} \implies V_{\text{bitline}} = \frac{\sum_{i=1}^{n} Q_i}{C_{\text{lumped}}}$$

  其中總等效電容 $C_{\text{lumped}}$ 是事前已知的電路常數（p.734），因此 ADC 量到的 $V_{\text{bitline}}$ 直接正比於內積 $\sum x_i w_i$！
- `【課本補充】` **抗雜訊優勢與權重固化限制**（p.734）：課本明確指出，在類比製程變異與熱雜訊下，**電荷共享架構對雜訊的容忍度普遍優於電流加總架構**；缺點則是電容值不像電阻那麼容易在執行期動態改寫 `[33:26]`。

### 5.2 電流加總（Current Summing，圖 14.16b）與 ReRAM 交叉陣列

- **歐姆定律乘法 ＋ 克希荷夫電流定律加總** `[33:42]`–`[35:15]`（p.734–735）：把輸入編碼為字元線電壓 $V_i$，把權重編碼為記憶胞的**電導（Conductance）$G_i = 1 / R_i$**。根據歐姆定律，每個記憶胞流出的電流就是乘積 $I_i = V_i \cdot (1/R_i) = V_i G_i$；所有記憶胞匯入同一條維持虛地電位（Ground Potential `[34:39]`）的 Bitline，總電流即為內積 $I_{\text{total}} = \sum_i V_i G_i$，流過感測電阻轉成電壓後即可由 ADC 讀出！
  - ⚠️ **口誤勘誤（兩處）**：
    1. `[34:02]` 老師先正確說了「$1/R$ or the conductance」，下一句口誤說成「$1/R$ which is the resistance」——$R$ 才是電阻（Resistance），$G = 1/R$ 是**電導（Conductance）**。
    2. `[34:21]` 老師口頭說「in series the current will get summed」——電學上串聯（in series）各元件電流相等不會相加；圖 14.16(b) 各列記憶胞是**並聯匯入同一條接地 Bitline 節點**，依**克希荷夫電流定律（KCL）**才會在位元線上把各支路電流 $I_i$ 相加！
- **三種硬體實作元件** `[35:16]`–`[36:21]`（p.734–735）：
  1. **標準 CMOS 電晶體操作於線性區（Linear Mode）** `[35:18]`：讓汲極電流 $I_D$ 近似線性正比於 $V_{DS}$ 或閘極字元線電壓（若為二元網路甚至用一顆 `AND` 閘即可，p.734）。
  2. **非揮發性可變電阻記憶體 ReRAM（Resistive RAM）** `[35:38]`：呼應第 10 章 §10.5.5（[第 45 支](lecture45_note.md)），ReRAM 記憶胞的電阻是歷史施加電壓的函數，可在執行期透過寫入脈衝動態調變電導狀態——完美解決類比交叉陣列「如何動態更換神經網路權重」的痛點！
- `【課本補充】` **物理電導不能為負，怎麼算負權重？雙行相減法！**（p.735）：真實世界的電容 $C$ 與電導 $G = 1/R$ 恆為非負數，無法直接表示負權重。課本 p.735 給出標準工程解法：把每一行濾波器拆成**正權重行 $G^+$（負權重填 0）**與**負權重絕對值行 $G^-$（正權重填 0）**兩組內積，最後在輸出端將兩者相減：

$$y = \sum_{i} x_i w_i^+ - \sum_{i} x_i |w_i^-|$$

---

## 6. 全章與全書總結（§14.6 Summary）

`[36:23]`–`[38:40]` Sarangi 教授笑稱來到了「永遠最令人開心的一頁投影片——第 14 章最後一頁，也是第一版課本全書的最後一頁」 `[36:23]`（對應課本 §14.6 Summary 第 1–16 點與 Further Reading，p.735–738）：

1. **為什麼全章聚焦卷積層？** `[36:42]`：雖然單看參數數量，全連接層（FC）最龐大，但一個網路只有最後 1～2 層 FC，Pooling 與 ReLU 又極快；真正佔據絕大多數算術吞吐時間的是**卷積層（CONV）**（見[第 55 支](lecture55_note.md)實測驗證：`CONV` 佔 $97.3\%$ FLOPs 與 $84\%$ GPU 時間）。
2. **從七層迴圈記號到硬體落地** `[37:07]`：用 $\parallel$（空間平行）與 $\triangleright$（時間序列）統一描述 Tiling 與四大駐留架構（IS、WS、OS、RS，見[第 56 支](lecture56_note.md)），再映射到雙塊循環矩陣（GEMM）或半脈動／純脈動陣列；半脈動陣列的廣播與加法樹瓶頸由 **Retiming Lemma** 轉成純脈動陣列，而 $k$ 倍降速氣泡則靠同時交錯多組卷積補回 $100\%$ 吞吐率（見[第 57 支](lecture57_note.md)）。
3. **模型壓縮與記憶體內運算收尾** `[38:13]`：透過剪枝（Pruning）、定點量化、Base-Delta 壓縮、位元串列 1D 卷積與 ReRAM 電荷共享／電流加總神經形態電路，把算力與能效推向矽晶片的物理極限。`【課本補充】` 延伸閱讀（p.737–738）推薦了經典加速器論文 **NeuFlow**、**DianNao**、**Cnvlutin**（跳零）、**Eyeriss**（RS + 壓縮）、**Google TPU**（脈動陣列）與脈動模擬器 **Scale-Sim**。

---

## 7. 【實測】跳零＋MSB 提早 ReLU、1D 位元卷積、RTX 4060 INT8 Tensor Core 406× 加速與 ReRAM 雙行交叉陣列模擬

為了逐一驗證本講四大核心機制，我們撰寫了 [experiments/lecture58/quant_sparse_and_pim.cu](experiments/lecture58/quant_sparse_and_pim.cu)，在 **NVIDIA GeForce RTX 4060 Laptop GPU**（Ada Lovelace `sm_89`）上完成四項實驗：

1. **位元串列乘法器（1D 位元卷積）與 GPU `__dp4a` 四路 INT8 內積核心**：

```cuda
// experiments/lecture58/quant_sparse_and_pim.cu:99-122
static uint32_t bit_conv_mul16(uint16_t a, uint16_t b, int *chunks_run) {
    /* Verify Sarangi [16:01]-[20:27]: binary product = 1D convolution of bit vectors + carry ripple */
    int conv[31] = {0};
    for (int i = 0; i < 16; i++) {
        int ai = (a >> i) & 1;
        for (int j = 0; j < 16; j++) {
            int bj = (b >> j) & 1;
            conv[i + j] += ai * bj; /* 1D convolution at lag k = i + j */
        }
    }
    uint32_t res = 0, carry = 0;
    for (int k = 0; k < 32; k++) {
        uint32_t s = (k < 31 ? (uint32_t)conv[k] : 0) + carry;
        res |= (s & 1U) << k;
        carry = s >> 1;
    }
    /* Count non-zero 4-bit chunks in multiplier b ([22:03] chunk skipping) */
    int nz_chunks = 0;
    for (int c = 0; c < 4; c++) {
        if (((b >> (c * 4)) & 0xF) != 0) nz_chunks++;
    }
    *chunks_run += nz_chunks;
    return res;
}
```

2. **ReRAM 雙行（$G^+ / G^-$）電流加總交叉陣列與電導變異／ADC 量化模擬**（課本 §14.5.2 p.734–735）：

```cuda
// experiments/lecture58/quant_sparse_and_pim.cu:202-221
        for (int j = 0; j < COLS; j++) {
            double i_pos = 0.0, i_neg = 0.0;
            for (int i = 0; i < ROWS; i++) {
                double w = W[i][j];
                /* Textbook p.735: split signed weight into G+ column and G- column! */
                double g_plus  = G_OFF + (w > 0 ?  w : 0.0) * (G_ON - G_OFF);
                double g_minus = G_OFF + (w < 0 ? -w : 0.0) * (G_ON - G_OFF);
                g_plus  *= (1.0 + sigma * rand_gauss());
                g_minus *= (1.0 + sigma * rand_gauss());
                /* Ohm's Law I = V * G + Kirchhoff's Current Law column sum */
                i_pos += x[i] * g_plus;
                i_neg += x[i] * g_minus;
            }
            /* Quantize column currents with B-bit ADC */
            double i_max = ROWS * G_ON * 0.45;
            int levels = (1 << adc_bits) - 1;
            double q_pos = round(fmin(i_pos / i_max, 1.0) * levels) / levels * i_max;
            double q_neg = round(fmin(i_neg / i_max, 1.0) * levels) / levels * i_max;
            double y_pim = (q_pos - q_neg) / (G_ON - G_OFF);
```

### 編譯與實測輸出

```bash
nvcc -O3 -arch=sm_89 experiments/lecture58/quant_sparse_and_pim.cu -lcublas -o /tmp/quant_sparse_and_pim && /tmp/quant_sparse_and_pim
```

```text
=== 1. Ineffectual Ops (Zero Bitmap + Early ReLU) & Base-Delta Compression ===
  Dense MACs = 1048576 | Bitmap Zero-Skip MACs = 140873 (86.6% saved!)
  + MSB Early-ReLU Termination effective MACs = 60252 (94.3% saved, exact ReLU match = 99.85%)
  Base-Delta [100,101,103,104] (base=102, 3-bit deltas): 32b -> 20b (37.5% saved), raw_dot=417 == base*sum(w)+delta*w=417

=== 2. Bit-Serial Multiplier = 1D Bit-Convolution + 4-bit Zero-Chunk Skip ===
  10,000 16-bit products via 1D Bit-Convolution: mismatches = 0 | 4-bit chunks executed = 23.5% (76.5% skipped!)

=== 3. GPU Precision Reduction (FP32 vs INT8 __dp4a vs INT8 Tensor Cores) on NVIDIA GeForce RTX 4060 Laptop GPU ===
  Weight Footprint (2048x2048): FP32 = 16.00 MiB -> INT8 = 4.00 MiB (4.0x memory savings!)
  Naive SIMT   FP32           : 81.541 ms ( 210.69 GFLOP/s,  1.00x)
  Naive SIMT   INT8 (__dp4a)  : 20.418 ms ( 841.42 GIOP/s,   3.99x)
  cuBLAS Tiled FP32           :  1.966 ms (8736.87 GFLOP/s, 41.47x)
  cuBLAS Tiled INT8 TensorCore:  0.201 ms (85671.80 GIOP/s,  406.62x; 9.81x over cuBLAS FP32!)

=== 4. ReRAM Dual-Column (G+/G-) Current-Summing PIM Crossbar (§14.5.2, Fig. 14.16b) ===
  Config (sigma_G= 0.0%, 8-bit ADC): Cosine Similarity = 0.99983
  Config (sigma_G= 5.0%, 8-bit ADC): Cosine Similarity = 0.99895
  Config (sigma_G= 5.0%, 6-bit ADC): Cosine Similarity = 0.99554
```

### 硬體與架構深度解析

1. **雙向稀疏遮罩 ＋ MSB 提早 ReLU 直接砍掉 `94.3%` 算術量**：在典型的 ReLU 後 `55%` 輸入零稀疏度與剪枝後 `70%` 權重零稀疏度下，`__builtin_popcountll(mask_x & mask_w)` 只對兩者同時非零的位置做乘法，先把 `1,048,576` 次 MAC 砍到 `140,873` 次（省下 **`86.6%`**）；接著對高 4 位元 MSB 做粗略正負號估計，將必為負值的 ReLU 輸出提早終止，等效 MAC 進一步降到 `60,252` 次（總共省下 **`94.3%`** 算力，且與精確 ReLU 結果的一致率高達 **`99.85%`**！）。同時驗證了 `[13:49]` 的 Base-Delta 分配律恆等式 `102 × 4 + (-2×3 - 1×(-5) + 1×2 + 2×4) = 417` 分毫不差。
2. **10,000 筆隨機 16-bit 乘法完全由「1D 位元卷積 ＋ 進位傳遞」零誤差還原**：驗證了 `[16:01]`–`[20:27]` 的核心定理 `mismatches = 0`；且當活化值切成 4-bit 區塊時，高達 **`76.5%`** 的全零位元區塊可直接跳過。
3. **從 FP32 定點量化到 INT8：記憶體減為 `1/4`，吞吐量狂飆 `406.62×`**：
   - 單看 SIMT 核心（不開 Tensor Core），把 $2048 \times 2048$ 權重從 `16.00 MiB`（FP32）量化為 `4.00 MiB`（INT8），並用硬體四路位元組內積指令 `__dp4a` 取代逐元素 FP32 FMA，頻寬需求與指令數同步除以 $4$，實測從 `81.541 ms` 降到 `20.418 ms`（精準 **`3.99×`** 加速！）。
   - 當結合[第 56 支](lecture56_note.md)的 Tiling 與[第 57 支](lecture57_note.md)的脈動張量陣列（`CUBLAS_COMPUTE_32I` INT8 Tensor Cores）時，執行時間更從 `81.541 ms` 驟降至 **`0.201 ms`**，衝出 **`85.67 TOPS`（`85,671.80 GIOP/s`）**——比 cuBLAS FP32 快 **`9.81×`**，比天真 FP32 快上驚人的 **`406.62×`**！這正是課本 p.731 所強調「必須在硬體內建專屬窄位寬運算單元（`__dp4a` / INT8 Tensor Core），量化才能真正轉化為倍數級加速」的鐵證。
4. **ReRAM 雙行（$G^+ / G^-$）類比內積的高容錯性**：即使在每個 ReRAM 記憶胞都疊加上 `5.0%` 的高斯電導製程變異（$\sigma_G = 5\%$）、並將位元線 ADC 壓低到只有 `6-bit`（64 階），$128 \times 64$ 交叉陣列輸出的向量餘弦相似度依然高達 **`0.99554`**，印證了 `[32:39]`「神經網路本身具備近似容錯性，因此極適合交給超低功耗類比 PIM 電路執行推論」。

---

## 8. 一頁速查表

| 最佳化／架構技術 | 核心作法與物理／數學原理 | 省下的資源 | 硬體代價與限制 |
|---|---|---|---|
| **CSE（共同子表達式消除）** | $w x_1 + w x_2 \to w(x_1 + x_2)$，先加輸入再乘重複權重 | 範例 7 次乘法 $\to$ 3 次乘法 | 需編譯器排程或可重組硬體路由 |
| **稀疏遮罩跳零 ＋ MSB 提早 ReLU** | 1-bit 遮罩 `AND` 才乘；先乘 MSB 估負號直接由 ReLU 歸零 | 實測省下 `86.6%`～`94.3%` MAC | 各 PE 執行時間非確定性（需動態負載平衡）；剛性純脈動陣列難支援 |
| **BCNN（二元卷積網路）** | 輸入與權重壓成 $\{0,1\}$，乘法變 `AND`、加法近似變 `OR` | 32× 記憶體縮減、超高速邏輯閘 | 精度損失較大，適合邊緣偵測等任務 |
| **Base-Delta 壓縮** | 存區塊均值 `base` ＋ 窄位寬 `Δ_i`；可用 $(\text{base}+\Delta_i)w_i$ 免解壓 | 25%～37.5%+ 頻寬與容量 | 區塊數值若太分散則 `Δ` 位寬省不了 |
| **位元串列乘法器（Bit-Serial）** | 二進位乘法每欄加總 ＝ **位元向量的 1D 卷積** ＋ 進位傳遞；可跳過全 0 的 $m$-bit 區塊 | 極小面積功耗、任意精度 $N$ 通吃 | 單筆延遲由平行 $O(\log N)$ 退為 $O(N)$ |
| **32-Bank GB ＋ Push 預取** | $16\times 16$ OS 陣列配 16 輸入 + 16 權重 Banks；編譯器 + FSM 主動推資料（Listing 14.2） | 消除 Cache Miss 停頓、餵飽 32 路串流 | 需智慧型 DNN 編譯器與 HBM/HMC 高頻寬支撐 |
| **PIM 電荷共享（圖 14.16a）** | $Q_i = V_i C_i$，開關閉合後 $V_{\text{bl}} = (\sum Q_i)/C_{\text{lumped}} \to$ ADC | 免資料搬移、抗雜訊優於電流加總 | 電容權重不易在執行期動態改寫 |
| **PIM 電流加總 / ReRAM（圖 14.16b）** | 歐姆定律 $I_i = V_i G_i$ ＋ KCL 位元線電流加總 $\to$ ADC | 免資料搬移、**ReRAM 可動態改寫權重** | 受製程變異與 ADC 功耗限制；負權重需拆 $G^+, G^-$ 兩行相減 |

---

## 9. 與全書前後講次的總連結（全課程完結篇）

- **前承第 14 章前三講**：[第 55 支](lecture55_note.md)拆解了 CNN 四大層與 ReLU 非線性本質、[第 56 支](lecture56_note.md)建立了七層迴圈 $\parallel/\triangleright$ 記號與 Tiling／四大駐留策略、[第 57 支](lecture57_note.md)落地為 2D PE 空間資料流、Retiming Lemma 純脈動陣列與雙塊循環矩陣（GEMM）；本講則補齊了稀疏壓縮、可變精度位元串列算術與記憶體子系統的最後一塊拼圖。
- **回扣全書核心章節（Ch1–Ch13）**：
  - **指令管線與編譯器排程（Ch2–Ch5，[第 4 支](lecture04_note.md)–[第 17 支](lecture17_note.md)）**：本講 §4 的 Push 式預取與加速器 Dispatcher 直接借鏡了亂序處理器與靜態編譯器軟體管線化（Software Pipelining）的精神。
  - **GPU 與 SIMT／Tensor Cores（Ch6，[第 18 支](lecture18_note.md)–[第 20 支](lecture20_note.md)）**：從傳統 SIMT 核心到本講實測飆出 `85.67 TOPS` 的 INT8 Tensor Cores，展現了通用平行架構與專用脈動矩陣的合流。
  - **多體快取、NoC 與主記憶體／NVM（Ch7–Ch10，[第 21 支](lecture21_note.md)–[第 45 支](lecture45_note.md)）**：32-Bank Global Buffer 對應 Ch7 多體 SRAM（[第 23 支](lecture23_note.md)–[第 25 支](lecture25_note.md)），2D PE 脈動互連對應 Ch8 晶片內網路（[第 27 支](lecture27_note.md)），而 HBM/HMC 3D 堆疊與 ReRAM 神經形態交叉陣列更是直接把 Ch10（[第 44 支](lecture44_note.md)、[第 45 支](lecture45_note.md)）的非揮發性記憶體元件變成運算引擎本身！
  - **功耗、可靠度與安全（Ch11–Ch13，[第 46 支](lecture46_note.md)–[第 54 支](lecture54_note.md)）**：後摩爾定律時代在功耗牆（Ch11）、製程變異容錯（Ch12）與硬體安全邊界（Ch13）的夾擊下，專用領域架構（Domain-Specific Architectures）與記憶體內運算正是《*Next-Gen Computer Architecture: Till the End of Silicon*》全書指向的終極答案。

---

## 附：本講一句話總結

**現代 AI 推論加速器透過「跳過零與必負 ReLU 的無效運算、Base-Delta 與定點量化壓縮、把二進位乘法化為 1D 位元卷積的位元串列脈動電路」，再搭配「32-Bank Push 式預取快取、3D HBM 以及利用電容電荷共享與 ReRAM 歐姆電流加總的記憶體內類比交叉陣列」，同時擊穿算術功耗牆與記憶體頻寬牆。**

---

*筆記整理自 Smruti R. Sarangi 教授 Advanced Computer Architecture 第 58 講影片與 Next-Gen Computer Architecture v3.2 第 14 章 §14.4–14.6（全書完結篇），搭配本機稀疏跳零、1D 位元卷積、RTX 4060 INT8 Tensor Cores 與 ReRAM 雙行交叉陣列實測驗證。*
