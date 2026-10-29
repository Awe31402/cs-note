# Advanced Computer Architecture 第二十四支影片完整筆記：CACTI、多 port 與 bank、array 的階層與 Elmore delay model（Chapter 7, Part IV）

> **課程**：Advanced Computer Architecture — Chapter 7: Caches (Part IV) -- Cacti, Cache Design, Elmore Delay（播放清單第 24 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕下載不到）（1:15:38）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch7** §7.3.2–7.3.3（p.289–302）
> **章號**：影片 Chapter 7 = 課本 Ch7。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 把「Elmore」寫成「ELMO」、「Nspd」寫成「n spd／nsbd」、「Bamboo Paper」（老師的手寫軟體）寫成「bamboo paper software」，筆記直接改正。

⚠️ 本機沒有跑真的 CACTI：從 GitHub 下載 HP Labs 的 CACTI 原始碼後，「編譯、執行外部程式碼」這一步被這個環境的安全規則擋下來了。所以這一講影片的 CACTI 數字（[16:24]、[50:14]）只能照抄，**沒有在本機重現**，待補。實測改做 Elmore delay model 的驗證，加上前幾講已經量過的本機 cache 延遲。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 4 KB 2-way 還是 8 KB direct mapped](#1-4-kb-2-way-還是-8-kb-direct-mapped)
- [2. 多 port 的 SRAM cell](#2-多-port-的-sram-cell)
- [3. 多 port 的代價：面積是 port 數的平方](#3-多-port-的代價面積是-port-數的平方)
- [4. CACTI 的輸入與輸出](#4-cacti-的輸入與輸出)
- [5. Technology node 與 scaling rule](#5-technology-node-與-scaling-rule)
- [6. 形狀要接近正方形](#6-形狀要接近正方形)
- [7. Nspd：aspect ratio](#7-nspdaspect-ratio)
- [8. Ndwl、Ndbl：切成 subarray](#8-ndwlndbl切成-subarray)
- [9. Bank：讓存取可以平行](#9-bank讓存取可以平行)
- [10. Array → bank → subbank → mat → subarray](#10-array--bank--subbank--mat--subarray)
- [11. 用 AI 找最佳設計](#11-用-ai-找最佳設計)
- [12. CACTI 的結果：大小變大，延遲幾乎不變](#12-cacti-的結果大小變大延遲幾乎不變)
- [13. RC 模型](#13-rc-模型)
- [14. RC tree 的分析](#14-rc-tree-的分析)
- [15. Elmore 的近似](#15-elmore-的近似)
- [16. 導線的延遲跟長度的平方成正比](#16-導線的延遲跟長度的平方成正比)
- [17. 【實測】](#17-實測)
- [18. 一頁速查表](#18-一頁速查表)
- [19. 與後續講次的連結](#19-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
架構師的難題：4 KB 2-way 好，還是 8 KB direct mapped 好？→ 要知道存取時間、功耗 → 再做架構模擬
同時存取：多 port 的 SRAM cell（8T）→ 面積 ∝ port 數²，不划算 → 改用 bank
CACTI（HP Labs）：輸入 C、B、A（＋ bank 數、technology node）→ 輸出存取時間、面積、功耗，並找出最佳的組織方式
  Nspd（aspect ratio）、Ndwl、Ndbl（切成 subarray）
  Array → bank（平行）→ subbank → mat（一個 block 分給幾個 mat）→ subarray（共用 decoder）
  最佳化：CACTI 用窮舉，老師建議用 AI（hill climbing 之類）
每個 subarray 的時間：Elmore delay model
  導線 → RC tree；τ = Σ R_ij C_j（R_ij = 共用路徑上的電阻和）
  結論：導線的延遲 ∝ 長度²
```

---

## 1. 4 KB 2-way 還是 8 KB direct mapped

[00:00] 架構師常遇到的兩難：**4 KB 2-way** 的 cache 好，還是 **8 KB 1-way**（direct mapped）好？

- 沒有明確的答案：可能一個存取要 2 cycle、另一個 1 cycle（associativity 低比較快），也可能相反；miss rate 也不同。
- [01:21] 要先知道每個設計的**存取時間**，再對大量的工作負載做**模擬**，才能判斷。

[01:52] 需要知道：

| 問題 | |
|---|---|
| 讀、寫要多久 | |
| 平均功耗、每次存取多少能量 | |
| 允許幾個**同時**的存取 | 一個 cycle 一個還是兩個？ |

[03:08] 三個基本步驟：

1. 找出存取時間（在「每個 cycle 允許幾個存取」的前提下）。
2. 換算成 clock cycle。
3. 對各個設計做詳細的架構模擬，選比較快的。

---

## 2. 多 port 的 SRAM cell

[04:36] 多個同時的存取：一個簡單的想法，「在電的意義上可行，在計算機結構的意義上是壞主意」：改 SRAM cell。

[05:38] 6T SRAM cell 再加：

- 第二條 **word line**（WL1、WL2）。
- 兩顆**額外的 access transistor**。
- 第二對 **bit line**。

→ **8 顆電晶體**。每個 cell 都這樣做；陣列有**兩個 decoder**，分別驅動 WL1 和 WL2。

[06:21] 結果：可以**同時**存取兩個不同的列：一列用 WL1 ＋ 第一對 bit line，另一列用 WL2 ＋ 第二對 bit line。這叫兩個 **port**，每個 port 可以讀或寫 → 兩個 **read/write port**。

[07:55] 也可以設計成一個 read/write port ＋ 一個**只讀** port（不用 write driver），或只寫 port，減少硬體。

---

## 3. 多 port 的代價：面積是 port 數的平方

[08:36]

| | 6T | 8T（2 port） |
|---|---|---|
| 每個 cycle | 1 次存取 | 2 次 |
| 面積 | 1 | **約 4 倍** |

[08:36] 為什麼 4 倍：word line 數變兩倍（y 方向）、bit line 數也變兩倍（x 方向），而兩條 bit line 之間要保持最小距離（**pitch**）→ 兩個方向都變兩倍。

[09:35] 一般化：**k 個 port → 面積 k² 倍**。

| 代價 | 好處 |
|---|---|
| 電晶體的面積；更重要的是**導線的面積**（後者主導） | 允許**平行**存取 |
| 比較慢、比較耗電（電路變大） | 例：1/3 的指令是記憶體指令、每 cycle issue 6 條 → 預期 2 條記憶體指令 → IPC 提高 |

[11:25] 【口頭】「這就是計算機架構師的工作：不同的人從不同的方向拉你。」有人要大 cache、多 port；有人說面積不夠，要放別的東西。

[12:15] 多 port 的設計沒有流行起來，主要就是因為 **k 倍的 port → k² 倍的面積**。

【課本補充】課本 p.292 Figure 7.29：2-port 的 SRAM cell，每一對 bit line 要有自己的 column mux、sense amplifier、write driver；陣列面積跟 port 數的**平方**成正比。

---

## 4. CACTI 的輸入與輸出

[03:30] **CACTI**：HP Labs 做的工具，很流行。以前有網頁介面（「至少兩年前還有」），現在可以免費下載、修改。

[12:15] 輸入（「**ABC**」，但給的順序是 C、B、A）：

| 參數 | 意思 |
|---|---|
| **C** | cache 大小（byte） |
| **B** | block 大小（byte） |
| **A** | associativity |
| bank 數 | bank 可以想成一個子 cache（[§9](#9-bank讓存取可以平行)） |
| technology node | 例：32 nm |
| （隱藏的）輸出寬度 | 一次讀出幾 byte（4 或 8） |
| （隱藏的）位址寬度 | 32 或 64 bit |

[13:53] 輸出：**存取時間、面積、功耗**（新版也給 **leakage power**，Ch11）。

- CACTI 同時是一個**估計器**，也是一個**最佳化器**：照你要的目標（最小面積、最小時間、最小功耗或組合），選出最佳的實體設計，告訴你它的規格。

[13:53] 範例（老師投影片上的 CACTI 輸出）：

```
輸入：16 KB、line 64 B、4-way、1 bank、32 nm、1 個 read/write port
輸出：存取時間 0.61 ns、面積 0.326 mm²（還有能量、功率的數字）
```

（本機沒有跑 CACTI，見開頭的 ⚠️。）

---

## 5. Technology node 與 scaling rule

[14:41] **Technology node**（feature size）：在矽上能可靠製造的**最小結構**的大小。傳統上是 **DRAM half pitch**：DRAM 裡平行導線之間的距離（pitch）的一半（Ch10）。

- 以前每 1 年、現在每 2 年，每個方向縮小 √2 → 電晶體面積減半。
- 老師錄影時是 **7 nm**。

[16:24] CACTI 大多只做到 32 nm。要 7 nm 的數字，就用 **scaling rule**：經驗推導出的一組係數，把 32 nm 的數字換算過去。IEEE 有很多篇不同的 scaling rule 論文，有些被廣泛接受。

---

## 6. 形狀要接近正方形

[17:58] 不最佳化會得到很糟的設計：

| 形狀 | 問題 |
|---|---|
| 很高很窄（列很多、行很少） | 很難擺在晶片上（要把別的元件推開）；**row decoder** 很大、很慢 |
| 很寬很矮（行很多） | 很難擺；**column mux／demux** 很慢 |

[18:45] 快的設計通常**接近正方形**（不完全是，看最佳化）。

【課本補充】課本 p.290：接近正方形的形狀，從在晶片上擺放元件的角度看也幾乎永遠是最好的。

---

## 7. Nspd：aspect ratio

[19:34] 從輸入算出：

```
block 數 = C / B
set 數 = C / (B × A)
```

[20:29] 第一個要決定的：**aspect ratio**（寬 ÷ 高，像買電視時說的）。

[21:10] 參數 **Nspd**：資料陣列每一**列**放幾個 **set**：

```
每列的行數（bit）= Nspd × A × B × 8
列數 = set 數 / Nspd = C / (B × A × Nspd)
兩者相乘 = 8C（cache 的總 bit 數）✓
```

[23:08] 早期的 CACTI，Nspd 必須是整數；現在可以是分數：一個 set 的一部分在這一列、另一部分在下一列。

【課本補充】課本 p.290 Example 5 也算了 subarray 的行列數。

---

## 8. Ndwl、Ndbl：切成 subarray

[23:55] 就算形狀對了，陣列很大還是很慢：

- bit line、word line 很長 → 訊號傳得慢。
- 列多 → decoder 大；行多 → column mux／demux 大。

[24:49] 切成很多 **subarray**：

| 參數 | 意思 |
|---|---|
| **Ndwl** | 把 **word line** 切成幾段（垂直的切線） |
| **Ndbl** | 把 **bit line** 切成幾段（水平的切線） |
| subarray 數 | Ndwl × Ndbl（例：4 × 4 = 16） |

（d = data array；tag array 有對應的 Ntwl、Ntbl 等。）

[25:49] 兩層階層的簡單版本：一個 block 整個放在一個 subarray 裡。存取時：

```
時間 = 找出是哪個 subarray、把位址送過去（decoder ＋ demux：decoding and routing）
     ＋ 那個 subarray 的存取時間（比整個陣列小，所以快）
```

[27:25] 取捨：不能切 100、1000 個 subarray：

1. decoding 和 routing 的負擔會太大。
2. 一個 block（或一次要讀的 4、8 byte）會被分散在好幾個 subarray，要讀好幾個。
3. 每個 subarray 也有自己的邏輯和額外的導線。

[29:27] 所以最佳化器在兩層的版本要找 **3 個參數**：**Nspd、Ndwl、Ndbl**。

[31:01] 早期 CACTI 每個 subarray 有自己的 decoder、sense amplifier，像一個獨立的迷你 cache；現在不是這樣了，用多層的階層。

---

## 9. Bank：讓存取可以平行

[31:52] 大陣列先切成好幾個 **bank**：

- Bank 是一個**獨立的陣列**，有自己的位址線、資料線和電路。
- 比較小 → 比較快。但重點是：**不同的 bank 可以平行存取**。

[32:28] 從 block address 取最低的 2 個 bit 當 **bank id** → 4 個 bank。只要兩個位址不落在同一個 bank（**bank conflict**），就能同時做 4 個存取。

[33:08] 例：沒有 bank conflict 的機率 3/4，平行存取就很容易。代價：更多面積、routing 和 decoding 的負擔。

[33:42] bank 數**純粹**看要支援多少個平行存取決定：要 8 個 bank，就從 block address 取 3 個 bit。

[34:19] 每個 bank 再由 CACTI 設計：切成 subarray，算 Nspd、Ndwl、Ndbl。所有 bank 一樣大的話（通常如此），最佳化一個就等於全部。

【課本補充】課本 p.291：用 bank 代替多 port；bank conflict 時要把存取**排成先後**。

---

## 10. Array → bank → subbank → mat → subarray

[35:42] 一個 bank 切成很多很小的 subarray 之後，怎麼組織？只做「bank → subarray」兩層不好：routing 負擔大，而且現代處理器有複雜的存取樣式。

[36:30] 一個重要的樣式：**SIMD**（SSE、AVX 指令集，Intel、AMD、ARM 都有）：

- 一次讀 128、256 或 **512 bit**（= 64 byte，一整個 block）。
- [37:44] 例：128 bit 裡**打包** 4 個 int，放進 128-bit 暫存器，跟另一個 128-bit 暫存器做**打包加法**，一條指令加 4 對數字（單一指令多資料）。AI、矩陣乘法很需要。
- 所以要能一次從記憶體讀出一大塊。

[39:23] 如果切成很小的 subarray，64 byte 會分散在很多 subarray，不好管理；從耗電的角度，也不該有很大的 routing 和 decoding 邏輯。→ **切成更多層**：

| 層 | 為什麼切 | 說明 |
|---|---|---|
| **Array** | | |
| **Bank** | **平行** | 不同 bank 可以同時存取 |
| **Subbank** | **效率** | 從位址馬上知道去哪一個；同一個 bank 裡的 subbank **不能**平行存取；一個 subbank 存一整個 block |
| **Mat** | **block 內的平行**、更快 | 例：64 byte 的讀取，兩個 mat 各給 32 byte 再合併；mat 小所以快 |
| **Subarray** | **共用資源** | 同一個 mat 裡的 subarray 同時被啟動，各給一部分（例：每個 8 byte）；可以**共用 row decoder** 等電路，省面積。mat 之間通常不共用 |

[42:12] 【口頭】Elmore delay model 會告訴你：word line、bit line 這種導線的延遲跟**長度的平方**成正比（[§16](#16-導線的延遲跟長度的平方成正比)），所以不想要長的 word line 和 bit line，寧可用小陣列各給一部分再合併。

[46:37] 更多的最佳化參數（CACTI 6 寫死了，但不一定要寫死）：bank 數（要架構模擬）、每個 bank 幾個 subbank、每個 subbank 幾個 mat。每個 mat 的 subarray 數則已知（Ndwl × Ndbl）。

【課本補充】課本 p.292：這是 **CACTI 5.0** 提出的階層；課本說每個 mat 切成 **4 個 subarray**。課本還講了 **H-tree**：位址要送到所有的 mat 和 subarray，大 cache 的導線延遲很長，用 H 形的樹狀導線讓到每個 subarray 的距離相等（影片沒講）。

---

## 11. 用 AI 找最佳設計

[03:56] 本質上是一個**最佳化問題**。

[34:19] CACTI 用**窮舉**（brute force）：試所有可能的值。「這不是好主意，很慢。」

[34:57] 老師建議：用 **AI 的啟發式方法**（例如 [74:28] 說的 **hill climbing**），很快找出讓目標函數（最小時間、最小面積、最小功耗或組合）最好的參數組合。

[48:11] 「傳統上沒人用 AI 做這種事，但在架構設計裡越來越常用。」【口頭】「你從沒想過可以用 AI 設計 cache 吧？原來可以。」

[48:11] 小結：

1. 先決定 bank 數（平行度）。
2. 每個 bank：算 aspect ratio（Nspd）、Ndwl、Ndbl。
3. 每個 bank 切成 subbank、mat、subarray。
4. 然後算每個 subarray 的時間和功耗（功耗比較簡單；時間難得多 → Elmore delay model）。

---

## 12. CACTI 的結果：大小變大，延遲幾乎不變

[50:14] 老師的 CACTI 實驗：2-way、1 bank、64 B line、32 nm，cache 大小從幾 KB 到 128 KB：

| | 小 cache | 128 KB |
|---|---|---|
| 存取時間 | 0.4243 ns | 0.55 ns |
| 面積 | | **超線性**成長 |
| 功率（每個 cycle 都存取） | 0.15 W | 0.25 W |

[50:42] 大小變大 64 倍，存取時間只多一點：因為最後存取的是一個小 subarray，而 subarray 很快。多出來的主要是額外的導線、decoding、找對的 subarray 和合併資料。

[51:30] 面積**不是線性**成長：subarray 越多，導線、decoder、routing 邏輯也越多。

（本機的 cache 延遲隨大小的變化見 [§17.3](#173-本機-cache-的延遲隨大小的變化)；但那是整個 cache 階層，不是 CACTI 的單一陣列。）

---

## 13. RC 模型

[52:51] 要算一個 subarray 的時間，把電路換成簡單的模型：**導線 → RC 模型**，只有電阻和電容：

- 電阻：一小段導線的電阻。
- 電容：對地的電容。導線是金屬導體，晶片裡到處都是其他導線，只要兩塊「平行板」之間有電位差，就是一個電容，模型成一個接地的電容。
- 一段接一段：R、C、R、C……

[54:24] 電晶體、sense amplifier、bit line、word line、SRAM cell 都可以做成 RC 模型。

兩個假設：

1. 只考慮 **RC tree**：沒有迴圈，也沒有「兩條路徑交叉匯合」。
2. 輸入只考慮 **step**：電壓瞬間從 0 變 1 或從 1 變 0，不考慮慢慢變。

---

## 14. RC tree 的分析

[55:52] 一個 RC tree，端點 0–5（0 接電壓源）。只考慮 **1 → 0** 的轉變（0 → 1 留給觀眾當練習）：電壓源突然從 1 V 掉到 0 V，各端點的電壓隨時間怎麼變？

[56:39] 把每個電容換成**電流源**：電容 C_j 提供的電流是 −C_j dV_j/dt（Q = CV，I = dQ/dt；電壓在下降，所以是負號）。

[58:24] 要算的是 **V_ij**：端點 i 的電壓中，由端點 j 的電流源造成的部分。系統是線性的 → **疊加**：V_i = Σ_j V_ij。

[59:24] 一次只看一個電流源，其他的換成**斷路**。例：V_34（端點 4 的電流源對端點 3 的影響）：

- 電流從端點 4 經過 R1 流到地；其他分支沒有電流。
- 端點 3 的電壓 = 電流 × R1（R1 是唯一在這條路徑上、通往地的電阻）。

[61:54] 一般化：

```
R_ij = 從 0 到 i 的路徑、和從 0 到 j 的路徑，兩者「共用」部分的電阻和
V_ij = −R_ij C_j dV_jj/dt
V_i  = −Σ_j R_ij C_j dV_jj/dt
```

[63:39] 例：端點 2 的電流源對端點 3 的影響：共用路徑有 R1、R2 → R_23 = R1 + R2。

（本機用一個 RC tree 實際算了，見 [§17.2](#172-rc-tree-的-elmore-時間常數)。）

---

## 15. Elmore 的近似

[65:24] 這個公式很難算。【口頭】Elmore delay model「很老，我記得是 40 年代末或 50 年代初提出的」。

[66:00] Elmore 的假設（**single pole approximation**）：

```
dV_jj/dt ≈ dV_i/dt
（只看一個電流源時，所有端點電壓變化的速率都一樣）
```

代進去：

```
V_i = −(Σ_j R_ij C_j) dV_i/dt = −τ_i dV_i/dt
τ_i = Σ_j R_ij C_j     ← 整個系統的時間常數
解：V_i(t) = V_0 e^(−t/τ_i)
```

[67:49] t = 0 時 V_i = V_0，t → ∞ 時 V_i = 0。

[68:49] 【口頭】「Elmore 跟 cache 沒有關係，他那時候 cache 還沒被發明。他只是給了一個算 RC tree 時間的一般近似。」知道時間之後，功耗也能接著算。

【課本補充】課本 p.293–299：Elmore 的論文是 **1948** 年；課本主要介紹 **Horowitz 1983** 年關於 MOS 電路延遲的論文（以 Elmore 為基礎）；**CACTI 1.0**（Wilton and Jouppi 1993）把 cache 的每個元件都換成 RC 模型。課本的近似是 dV_jj/dt = α dV_i/dt，並證明 **α = 1** 時誤差最小。

---

## 16. 導線的延遲跟長度的平方成正比

[69:34] 一條長導線（bit line、word line）切成 n 段，每段 R、C：

```
第 j 個電容的共用路徑有 j 個電阻 → R_ij = jR
τ = Σ_{j=1..n} jR × C = RC × n(n+1)/2 = O(n²)
```

[71:24] **最重要的結果：導線的延遲跟長度的平方成正比。** 長度加倍 → 延遲 4 倍。「這對我們是壞消息：電路裡絕對不要有長的導線。」

[72:20] 加上 decoder、電晶體、mux／demux、sense amplifier 的簡單模型，就能把整個 SRAM bank 看成一串簡單的元件，很快算出時間；知道每個轉變的時間，功耗也能算。

[73:27] CACTI 每次切陣列（array → bank → subbank → mat → subarray，可以有 10 層），每一層都要算這些參數；對每一種設定用 Elmore delay model 算時間，要最快的就選最快的。

[74:28] 下一講：進階的 cache 設計（6–7 個最常見的進階最佳化）。

（本機用數值解驗證了 n² 的關係和 Elmore 的準確度，見 [§17.1](#171-導線的-elmore-delay)。）

---

## 17. 【實測】

程式在 `experiments/lecture24/`。

### 17.1 導線的 Elmore delay

`elmore.py`：一條導線切成 n 段 RC（R = C = 1，時間單位是 RC）。一邊用 Elmore 的公式算 τ，一邊用**數值方法真的解**這個 RC 梯子的電路（每個節點的電流守恆，很小的時間步長往前推），量末端的電壓掉到 1/e（約 37%）和 50% 要多久：

```
   n  Elmore τ   τ/n²   真的到 1/e   真的到 50%   50%/τ
   1      1.0   1.000        1.00        0.69    0.694
   2      3.0   0.750        3.03        2.23    0.742
   4     10.0   0.625       10.08        7.54    0.754
   8     36.0   0.562       36.25       27.23    0.756
  16    136.0   0.531      136.89      102.98    0.757
  32    528.0   0.516      531.38      399.92    0.757
```

- **τ/n² 趨近 0.5**：就是 n(n+1)/2 ÷ n²。導線長度（段數）加倍，延遲變**約 4 倍**（例：16 段 136、32 段 528）。這就是 [71:24] 的結論。
- Elmore 的 τ 跟**真正**掉到 1/e 的時間幾乎一樣（差不到 1%）：single pole 近似雖然是近似，對導線非常準。
- 真正掉到 50% 的時間約 **0.757 τ**（n 很大時）；一段的時候是 0.693 τ（= ln 2，就是單一 RC 電路的答案）。所以拿 Elmore 的 τ 估「多久之後過一半」，乘一個常數就好。

### 17.2 RC tree 的 Elmore 時間常數

同一個程式算 [§14](#14-rc-tree-的分析) 那種 RC tree（0 →R1→ 1；1 →R2→ 2 →R3→ 3；1 →R4→ 4 →R5→ 5；R1 = 1、R2 = 2、R3 = 1、R4 = 3、R5 = 1，C3 = 2，其他電容 = 1）：

```
   節點 3 的 Elmore τ = 14.0；例：R_34 = 1（共用路徑上的電阻和）
   節點 5 的 Elmore τ = 13.0；例：R_54 = 4（共用路徑上的電阻和）
```

手算節點 3：τ₃ = R_31 C1 + R_32 C2 + R_33 C3 + R_34 C4 + R_35 C5 = 1·1 + 3·1 + 4·2 + 1·1 + 1·1 = **14**，跟程式一樣。節點 3 和節點 4 的路徑只在 R1 這一段共用，所以 R_34 = R1 = 1；節點 5 和 4 共用 R1、R4，所以 R_54 = 4。

### 17.3 本機 cache 的延遲隨大小的變化

真的 CACTI 沒辦法在本機跑（開頭的 ⚠️）。手上有的是前幾講量到的整個 cache 階層的延遲（[第 21 支 §14.1](lecture21_note.md#141-p-core-和-e-core-的各層延遲)），可以看一個粗略的趨勢：

| | 大小 | 延遲（P-core） |
|---|---|---|
| L1 data | 48 KB | 約 5 cycle（1.07 ns） |
| L2 | 1.25 MB | 約 15 cycle（3.2 ns） |
| L3 | 24 MB | 數十 cycle（受 TLB 影響，見第 22 支） |

- L1 到 L2：容量大了 **26 倍**，延遲只多了 **3 倍**。這跟 [§12](#12-cacti-的結果大小變大延遲幾乎不變) 的「大小變大很多，延遲只多一點」方向一致：大的 cache 也是切成很多小 subarray。
- 但這不是同一種比較：L1、L2 是不同的設計（associativity、port、bank 都不同），中間還有 L1 miss 的判斷。也從這裡可以看到：**P-core 的 L1 每個 cycle 能做約 3 次 load**（[第 11 支 §16.3](lecture11_note.md#163-真的-cpu-有幾個功能單元)），所以它一定有 [§2](#2-多-port-的-sram-cell) 的多 port 或 [§9](#9-bank讓存取可以平行) 的 bank；是哪一種、幾個 bank，Intel 的公開資料這裡沒有，**待補**。

---

## 18. 一頁速查表

| 主題 | 重點 |
|---|---|
| 兩難 | 4 KB 2-way vs 8 KB 1-way：先算存取時間 → 換成 cycle → 架構模擬 |
| 多 port | 8T cell：兩條 word line、兩對 bit line、兩個 decoder；也可以只讀／只寫 port |
| 多 port 的代價 | 面積 ∝ port 數²（導線主導）；慢、耗電 → 不流行 |
| CACTI | HP Labs；輸入 C、B、A、bank、tech node；輸出時間、面積、功耗（leakage）；也找最佳設計 |
| 範例 | 16 KB、64 B、4-way、32 nm → 0.61 ns、0.326 mm² |
| Tech node | 最小可製造的結構（DRAM half pitch）；每代每邊 ÷ √2；scaling rule 換算到 7 nm |
| 形狀 | 接近正方形最快：太高 → decoder 慢；太寬 → column mux 慢 |
| Nspd | 每列幾個 set；行 = 8·A·B·Nspd，列 = C/(A·B·Nspd) |
| Ndwl、Ndbl | 切 word line／bit line → Ndwl × Ndbl 個 subarray；decoding、routing 有負擔 |
| Bank | 獨立陣列；平行存取；block address 的低位 bit 選 bank；bank conflict |
| 階層 | array → bank（平行）→ subbank（效率）→ mat（block 內平行）→ subarray（共用 decoder） |
| 最佳化 | CACTI 窮舉；老師建議 AI（hill climbing） |
| 結果 | 128 KB 時延遲只從 0.42 到 0.55 ns；面積超線性 |
| RC tree | 導線 → R、C；step 輸入；電容 → 電流源；疊加 |
| Elmore | R_ij = 共用路徑的電阻和；τ_i = Σ R_ij C_j；V_i = V₀ e^(−t/τ) |
| 導線 | τ = RC n(n+1)/2 → 延遲 ∝ 長度² |
| 實測 | τ/n² → 0.5；Elmore τ 跟真正到 1/e 的時間差不到 1%；真的 CACTI 沒能在本機跑（待補） |

---

## 19. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| SRAM、CAM 陣列、decoder、sense amplifier | [第 23 支](lecture23_note.md) |
| Cache 的組織、本機的 cache 延遲 | [第 21 支](lecture21_note.md) |
| GPU 的多 bank register file、bank conflict | [第 20 支](lecture20_note.md) |
| 每個 cycle 幾個 load（多 port／bank 的證據） | [第 11 支](lecture11_note.md) |
| Pipelined cache、non-blocking cache 等進階設計 | [第 25 支](lecture25_note.md) |
| DRAM 的 half pitch、bank | [第 41 支](lecture41_note.md)–[第 43 支](lecture43_note.md)（Ch10） |
| Leakage power | [第 46 支](lecture46_note.md)（Ch11） |
| 晶片上的網路、長導線 | [第 27 支](lecture27_note.md)（Ch8） |

---

## 附：本講一句話總結

> **要比較 4 KB 2-way 和 8 KB direct mapped 這種設計，得先知道每種 cache 的存取時間和功耗再做架構模擬；同時存取可以靠多 port 的 SRAM cell（多一條 word line、一對 bit line），但 k 個 port 面積就變 k² 倍，所以改用可以平行存取的 bank；HP Labs 的 CACTI 給定 C、B、A、bank 數和製程，用 Nspd 決定陣列的形狀（越接近正方形越快）、用 Ndwl、Ndbl 把陣列切成 subarray，並組成 array → bank → subbank → mat → subarray 的階層（bank 給平行度、mat 給 block 內的平行、同一個 mat 的 subarray 共用 decoder），用窮舉（老師建議改用 AI 之類的搜尋）找出最快或最省的設計，結果 cache 變大 64 倍延遲只多一點、面積卻超線性成長；每個 subarray 的時間則用 Elmore delay model：把導線和電晶體換成 RC tree，時間常數 τ = Σ R_ij C_j（R_ij 是共用路徑上的電阻和），由此得到導線延遲跟長度平方成正比，所以絕對不要長導線。本機實測：數值解 n 段 RC 導線，τ/n² 趨近 0.5，Elmore 的 τ 和真正掉到 1/e 的時間差不到 1%；真的 CACTI 因為編譯外部程式碼被擋而沒能在本機重現（待補）。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 7: Caches (Part IV) -- Cacti, Cache Design, Elmore Delay 的 whisper 轉錄稿（本機 `large-v3-turbo`，1:15:38）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch7 §7.3.2–7.3.3（p.289–302）整理。實測為本機（Intel i7-13650HX、Python 3.10）結果。*
