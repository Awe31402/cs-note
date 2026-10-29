# Advanced Computer Architecture 第四十一支影片完整筆記：主記憶體（一）：DRAM cell、bit line 架構、sense amplifier 與 channel／DIMM／rank／bank（Chapter 10, Part I）

> **課程**：Advanced Computer Architecture — Chapter 10: Main Memory (DRAM devices, arrays and architectures) -- Part I（播放清單第 41 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 字幕（1:01:50）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch10** §10.1（p.536–551）
> **章號**：影片 Chapter 10 = 課本 Ch10。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 字幕把「DIMM」寫成「DIM」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 這一章要講什麼](#1-這一章要講什麼)
- [2. DRAM cell](#2-dram-cell)
- [3. 讀是破壞性的，而且要 refresh](#3-讀是破壞性的而且要-refresh)
- [4. 兩種電容：trench 與 stacked](#4-兩種電容trench-與-stacked)
- [5. DRAM 陣列](#5-dram-陣列)
- [6. Open bit line 與 folded bit line](#6-open-bit-line-與-folded-bit-line)
- [7. Sense amplifier](#7-sense-amplifier)
- [8. 讀、寫、refresh 的步驟](#8-讀寫refresh-的步驟)
- [9. 系統：channel、DIMM、rank、chip、bank、array](#9-系統channeldimmrankchipbankarray)
- [10. Channel 上的訊號與 bus 的負載](#10-channel-上的訊號與-bus-的負載)
- [11. 【實測】](#11-實測)
- [12. 一頁速查表](#12-一頁速查表)
- [13. 與後續講次的連結](#13-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
DRAM cell = 1 個電容 ＋ 1 個 access transistor（SRAM 是 6 個 transistor）
  讀：precharge 到 Vdd/2，看電壓往哪邊偏；讀是破壞性的 → 讀完要寫回（restore）
  電容會漏電 → 定期 refresh（每個 row 32–64 ms 一次）
電容：trench（往下挖，難做）、stacked（往上疊，常用）
陣列：sense amplifier 在 column mux 之前（每個 column 一個，兼當 buffer）；一次開一整個 row
  open bit line（6F²）vs folded bit line（8F²，比較抗雜訊）
sense amplifier：equalizer（precharge）＋ 4 個 transistor 的交叉回授（放大 ＋ 記住）
階層：channel → DIMM → rank → chip（device）→ bank → array → row → column
  rank：一組同步動作、跟 memory controller 等距的 chip；x4／x16 = 一個 bank 有幾個 array
address／command bus 接到每個 chip（負載重、慢）；data bus 分 lane（負載輕、快）→ DDR
```

---

## 1. 這一章要講什麼

[00:21]「第 10 章：**主記憶體**。」幾乎所有處理器都有的**晶片外**記憶體結構，「比 cache 複雜多了、大很多，可以用很多種技術做」，要講好幾講。

需要的背景：cache、一點數位電子學；[00:59] 基礎計算機結構（老師上一本書，網站 basiccomputerarchitecture.com）；看非揮發記憶體時還要一點**元件物理**。[01:39]「第 7 章大概就是起點。」

[01:39] 順序：

1. DRAM：元件構造 → 協定 → 時序 → **memory controller**。
2. [02:13] 新興的（非揮發）記憶體技術。
3. **Roofline model**：一個跟記憶體有關的效能模型，大部分由晶片外記憶體的效能決定。

---

## 2. DRAM cell

[02:48]「整個 DRAM 系列最有代表性的一張投影片」：

```
        word line
            │
bit line ──┤ access transistor ├── 電容 ── GND
```

- 一個**電容**：電壓接近 Vdd 是 1，否則是 0。「就這樣，沒別的。」
- 一個 **access transistor**，由 **word line** 控制；SRAM 有兩個 access transistor，這裡只有**一個**。
- [03:19] 接到 **bit line**：存 1 就把 bit line 充上去，存 0 就把 bit line 上的電放掉。

[03:52] **寫**很簡單：transistor 就是開關，打開它，把 bit line 設成 0 或 1，電容就被充電或放電。

[04:20] **讀**難很多：打開 transistor 後要等 bit line 被充電或放電。[04:54]「一個很小的電容要去充一條很大的 bit line」，很花時間 → 用 **precharge** 的技巧：先把 bit line 設成 **Vdd/2**，只看電壓往上或往下的小變化；超過雜訊範圍就能判定（[第 23 支 §8](lecture23_note.md#8-為什麼要-precharge)）。

【課本補充】課本 p.536：SRAM cell 要 6 個 transistor（[第 23 支 §3](lecture23_note.md#3-6t-sram-cell)），容量有限；DRAM 為了更高密度用一個電容。SRAM 有兩個 access transistor 是因為 inverter pair 有兩個輸出；電容只有一端，所以一個 transistor、一條 bit line 就夠。

---

## 3. 讀是破壞性的，而且要 refresh

[05:22] 跟 SRAM 不同的兩個麻煩：

| | 說明 |
|---|---|
| **Destructive read**（破壞性讀） | [05:22] 讀的時候電容的電流到 bit line 上，存的 1 會變成 0 → [05:56] **每次讀完都要寫回去**（restore） |
| **Refresh** | [06:27] 電容會**漏電**：存 1 的話電一直漏，最後變 0（存 0 沒這問題）→ [07:00] 要**定期**讀出來再寫回去 |

[07:00] 例：一開始電壓 Vdd，漏到 2Vdd/3 時 refresh，電壓回到 Vdd。refresh 前讀還是對的，「但不會一直對下去」。[07:32]「refresh 會讓協定變複雜。」

【課本補充】課本 p.536 Definition 89（refresh）、p.539 Definition 90（destructive read）。課本 p.537：電容越小密度越高，但漏得越快、要越常 refresh → 要在**密度**和 **refresh 間隔**之間取捨。課本 p.546：每個 cell 要**每 32 到 64 ms** refresh 一次。

---

## 4. 兩種電容：trench 與 stacked

[08:01] DRAM 的關鍵是做出那個電容。電容 C = εA／d：A 要大、d 要小，又不想佔晶片面積。

| 種類 | 構造 | 優缺點 |
|---|---|---|
| **Trench**（溝槽） | [08:41] 往矽**下面**挖一個又窄又深的洞。[09:53] 從 transistor 的摻雜區直接接到電容：一個電極是 polysilicon，中間是 dielectric，另一個電極是埋在 substrate 裡的 **n 型摻雜區**（接地） | [10:50] 頂上的截面很小 → 密度高；但[11:25] 「超深的溝很難做」，要在裡面沉積摻雜物很困難 |
| **Stacked**（堆疊） | [11:25] 往**上面**長的 3D 結構：兩個電極（金屬或 polysilicon）夾著 dielectric | [12:00] 密度稍微低一點，但表面積比看起來大很多，電容還是夠；[12:27] 好做、可靠 → **常用**；trench 只用在特定情況 |

【課本補充】課本 p.537–538：

- trench 的 dielectric 是 Al₂O₃、HfO₂、Ta₂O₅ 這類材料；40 nm 寬的溝，dielectric 厚 15–20 nm（Gutsche et al. 2005）；feature size 40 nm 時溝深 **4–6 µm**：「深度是寬度的 100 倍」。
- trench 埋在矽裡，很適合 **3D 晶片的 embedded DRAM**：上面可以再放 transistor 層或金屬層，不會造成繞線問題。
- trench 最深處的參數很難保證 → 很多商用製程用 stacked，雖然面積比較大。新的 stacked 有好幾片**鰭**（fin），有的做成**圓柱形**。

---

## 5. DRAM 陣列

[13:04]（課本圖 10.4）

```
row decoder ─▶ 一個 word line ─▶ 整個 row 的 cell 接到各自的 bit line
             ▼
       每個 column 一個 sense amplifier（放大 ＋ 暫存）
             ▼
       column mux/demux（column decoder 選一部分）─▶ read buffer ─▶ 送出
```

- [13:04] 一個 column 的 cell 接到**同一條** bit line；row decoder 打開一個 row，整個 row 的值都到 bit line 上。
- [13:33] 讀在 critical path 上，寫通常不在 → 主要看讀。
- [14:09] sense amplifier 兩個用途：① 偵測 precharge 電壓上的極小變化；② **存住這個 bit**（讀是破壞性的，要先存起來再寫回）。
- [14:43] 再用 **column decoder** 選一部分 column，放進 read buffer，讀出去。
- [15:12] **跟 SRAM 最大的不同**：SRAM 的 sense amplifier 在 column mux **之後**，DRAM 的在**之前**。
- 寫比較容易：有很強的 **write driver**，往反方向推 0 或 1。

【課本補充】課本 p.538–541：

- 一個 row 也叫一個 **page**（Important Point 17；跟虛擬記憶體的 page 不同）。
- 為什麼 sense amplifier 放在 mux 前面：它要暫存、還要 restore 整個 row，所以**每個 column 都要一個**。很多 DRAM 存取方式會直接從 sense amplifier 拿資料，不用重新存取 row：sense amplifier 像一個**很快的小 cache**。
- precharge 快，是因為可以用很強的 precharge driver 灌電流；bit line 被 DRAM cell 帶動時就沒有這種好處。

---

## 6. Open bit line 與 folded bit line

[15:40] 一個 DRAM cell 只接一條 bit line；bit line 的 R、C 很大，就算用 precharge 也慢。[16:48]「DRAM 的讀就是慢，這沒什麼辦法」，但可以：① 讓 bit line **短一點**；② 讓它**更抗雜訊**。sense amplifier 是**差動**的：比較兩個輸入的差，差 > ΔV 就是 1、< −ΔV 就是 0。

| | **Open bit line** | **Folded bit line** |
|---|---|---|
| 做法 | [17:23] 把一個 column 分成好幾段；第 i 段和第 i+1 段的 bit line 接到同一個 sense amplifier | [20:40] 兩條 bit line 蛇行穿過一個 column 的 cell，每條只接一半的 cell（接兩個、跳兩個） |
| 每個 cell 的面積 | [19:31] 約 **6F²** | [20:02] 約 **8F²**（「稍微沒那麼有效率」） |
| 優點 | 密度高、bit line 短 | [20:02] 比較簡單、結構規則、好做；兩條 bit line 靠在一起，雜訊相同 → 在 sense amplifier 相減時抵消 |
| sense amplifier | 插在陣列中間很多個 | [21:37] 都在尾端 |

（F 是 feature size：製程能可靠做出的最小尺寸。）

[22:05] 也可以混合：把長的 column 切成小段，每段各有一排 sense amplifier。選哪一種看：好不好製造、存取延遲（尤其是讀）、儲存密度。

⚠️ 影片和課本對這兩種架構的說法有出入：

| | 影片 | 課本 p.541–542 |
|---|---|---|
| Open bit line 的雜訊 | [18:26]–[19:31] 先說兩段 bit line 在同一區域、雜訊差不多會抵消，[20:02] 又說「抗雜訊能力明顯比較差」 | 兩條輸入的 bit line **不在一起**，撿到的雜訊不同 → **比較容易出錯** |
| Folded bit line 的速度 | [21:06]「每條只接一半的 cell，電容大幅降低，RC delay 低很多，比較快」 | 每條 bit line 接的 cell 數約是 row 數的**一半**（bit line 貫穿整個 column），「可能讓電容大幅增加、變慢」 |

課本的比較是：open 的 bit line 被切成很多段（每段短），folded 的 bit line 要跑完整個 column（雖然只接一半）。

【課本補充】課本 p.541：cell 的面積至少 **4F²**（cell 最小 F × F，跟旁邊的 cell 至少隔 F → 2F × 2F）；再加上 bit line、電路，而且電容和 transistor 不能完全垂直疊在一起 → open bit line 約 6F²。課本 p.541 也說明 sense amplifier 為什麼需要兩個輸入：DRAM 只有一條 bit line，另外拉一條固定電壓的參考線太花面積和繞線；反正同一個陣列一次只開一個 row，就拿另一段 bit line 當參考。

---

## 7. Sense amplifier

[23:16] 兩部分（課本圖 10.7–10.9）：

**① Equalizer／precharge 電路**：三個 transistor（T1、T2、T3），gate 都接 **EQ**。

- [23:54] EQ = 1 → 三個都導通 → 參考電壓 Vdd/2 灌到兩條 bit line → 兩條都 precharge 到 Vdd/2；之後關掉，電壓維持住。
- [24:27] 中間那個 transistor 讓兩條 bit line 電壓相等（萬一有一點誤差）。

**② Sense amplifier 本體**：[25:00] 四個 transistor，**交叉接**：一個 NMOS 和一個 PMOS 的 gate 接到第二條 bit line，另一對接到第一條；中點分別接 **SAN**（negative）和 **SAP**（positive）。[25:34]「有回授，回授就代表某種**儲存**。」

[26:08] 分析：V1 = V2 + ε（第一條 bit line 稍微高一點），SAN = 0 V、SAP = Vdd（假設 1 V）：

1. [26:39] 看 T2：一邊 0 V、另一邊 V2，gate 是比較高的 V1 → T2 變得更導通。
2. [27:19] → V2 往下掉 → 掉得越多，T3（PMOS）越導通。
3. [27:52] → V1 被拉向 Vdd → T2 又更導通……
4. [29:29] **互相回授**：V2 降 → V1 升 → V2 再降……很快 V1 = Vdd、V2 = 0。

- [30:01] 電路是對稱的：V2 = V1 + ε 時換 T1、T4 動作，結果反過來。一點點差就被放大成大差。
- [31:18] 要停用：SAN、SAP 都設成 Vdd/2 → 電壓都一樣，回授不會開始。
- [31:50] 因為回授，它**本身是穩定的**：會一直維持這個值 → 暫存的功能。

[32:23]「其實是**三個**用途：precharge、sense、store。」

[32:55] 再往外（課本圖 10.9）：

- **chip select**：0 的時候 sense amplifier 和 column mux 之間斷開；1 才能讀寫。
- [33:22] **讀**：read enable 打開，接到 column mux。
- [33:51] **寫**：write enable 打開，把值寫到一條 bit line、**反相**的值寫到另一條；write driver 同時改掉 sense amplifier 和 cell。

【課本補充】課本 p.543–544：課本的 equalizer 是 EQ 一打開 T1 先讓兩條 bit line 相等，再由 T2、T3 設成 Vdd/2；sense amplifier 的 SAN、SAP 設成 Vdd/2 時四個 transistor 都在 cut-off。

---

## 8. 讀、寫、refresh 的步驟

**讀**：[34:52]

1. precharge bit line；SAN、SAP 設成 Vdd/2（sense amplifier 停用）。
2. [35:25] 用 row decoder 打開 word line。
3. 打開 sense amplifier（SAN = 0、SAP = Vdd）。
4. [35:57] 打開 chip select 和 read enable。
5. 把 column address 送進 column mux，讀出資料。

[35:57] 讀是破壞性的，但值暫存在 sense amplifier 裡；讀完 word line 先**保持打開**，[36:29] sense amplifier 推著 bit line，把舊值**自動寫回** cell → 順便 refresh 了。

**寫**：[36:29] 兩個階段：

1. 跟讀一樣：打開 row、precharge、讓 sense amplifier 拿到值、自動 restore。
2. [37:04] 用很強的 write driver 把新值寫進 sense amplifier，再寫進 cell。

[37:38] 為什麼要先「假裝讀一次」：word line 是一整個 row 共用的，**沒辦法只開一部分**。一開 row，bit line 就開始被充放電，cell 的值就被破壞了 → 一定要先讀、存在 sense amplifier。[38:37] 這一步同時讓 sense amplifier 跟 row **同步**；之後 row 可以保持打開一陣子（不同架構不一樣），[39:08] 沒有電流在流。

[39:39] 只改 column mux 選到的那幾個 column。【口頭】「**每個 column 只存一個 bit**：這一點我自己也搞混過好幾次。」

[40:53] **write recovery time**：從開始寫到寫完、sense amplifier 和 cell 再次同步的時間。

**Refresh**：[40:53] 就是寫的**第一階段**：打開 row、precharge、讓 sense amplifier 拿到值、寫回 cell，再關掉。一個 row 一個 row 做。

| 方式 | 做法 | |
|---|---|---|
| **Burst** | [41:56] 一次把所有 row 全部做完 | 期間整個陣列不能用，很沒效率 |
| **Distributed**（interspersed） | 在一般操作之間擠時間做 | 對一般操作影響不大 |

[42:31] 總結：sense amplifier 只有三種狀態：「1 0」、「0 1」、或 SAN、SAP 都是 Vdd/2 的停用狀態（老師叫它 X）。

【課本補充】課本 p.546：refresh 可以看成一次「假的讀」，不用 column decoder、不用打開 chip select。**沒有有效資料的 row 可以不 refresh**；現代 DRAM 還可以**稍微超過**最大 refresh 間隔，先服務很重要的讀。

（本機量到一個像是 refresh 的週期性延遲，見 [§11.2](#112-每隔約-18-µs-就有一次特別慢)。）

---

## 9. 系統：channel、DIMM、rank、chip、bank、array

[43:04]（課本圖 10.10–10.13）

- CPU 有好幾個 **DRAM channel**：channel 是一組平行的銅線，32 到 128 bit 寬。
- [43:37] channel 上接 **DIMM**（dual inline memory module）：插在主機板上的一片板子，上面有好幾顆 DRAM chip（老師的圖有 8 顆）。
- [44:05] 階層：I-cache、D-cache → L2（或 L3、L4）→ **memory controller** → DIMM。[44:38]「**每個 channel 一個 memory controller**。」[45:04] 以前 memory controller 是主機板上另一顆晶片，「現在為了效率移進 CPU 裡了」。
- [45:36] channel 也叫 **memory bus**。

[46:07] DIMM 裡面：

```
channel → DIMM → rank → chip（device）→ bank → array → row → column
```

| 層級 | 說明 |
|---|---|
| **DIMM** | [46:07] 可以分成好幾個 rank；通常一個 DIMM 就是一個 rank |
| **Rank** | [46:44] 一組 DRAM chip。[47:58] 定義：① 這些 chip **同步**（lockstep）動作；② 它們跟 memory controller **一樣遠**（訊號到每顆 chip 的時間相同）。[48:28] memory controller 把 rank 當成**最小的單位**，對 rank 下命令。[51:08] 理論上一個 rank 可以跨好幾個 DIMM，但不同 DIMM 跟 controller 的距離不同，實際上不會 |
| **Chip**（device） | [47:18] 一個 chip 裡有好幾個 bank |
| **Bank** | [49:33] 一組 DRAM array 的邏輯集合。[50:12] 理論上不同 bank 可以同時做不同的事（協定允許），「實際上很少」。[50:40] 同一個 bank 裡的 array 可以**共用**一些硬體 |
| **Array** | [51:42] 一次存取一個 column → 每個 array 出 **1 bit** |

[52:19] **xN**：一個 bank 有 N 個 array，一次出 N bit。例：**x16** = 16 個 array、一個 bank 一次出 16 bit。[52:53] 2021 年以後有 x64、x128，但只在 **3D** 的設計裡（[第 45 支](lecture45_note.md)）。

[53:53] 一個 rank 裡平行存取 64 個 array，每個出 1 bit → 一次 64 bit。

⚠️ [52:19] 老師說「一個 bank 4 個 array 的話，一次要讀 **64 byte** 就要 16 個 bank 一起動作」：16 × 4 = 64 **bit**，不是 byte。課本 p.548 的例子是「一次要 **64 bit**，就用 16 顆 chip 組成一個 rank，每顆出 4 bit」。

【課本補充】課本 p.547–549：

- DRAM 不適合做晶片上的 cache：① cell 很弱，一個電容要推一條很長的 bit line（SRAM 的 bit line 直接透過 transistor 接到電源或地）；② 讀是破壞性的，讀其實是「讀 ＋ 寫」；③ 一定要花時間 refresh。
- **eDRAM**（embedded DRAM）：DRAM 跟處理器放在同一個 die，或同一個封裝裡的另一個模組 → LLC 和 eDRAM 之間的連線比較短、頻寬比較高。
- 好幾個 memory controller 要分攤實體位址空間，例如用位址的 **MSB**（兩個 controller：MSB = 0 給 0 號，= 1 給 1 號）。
- channel 越來越窄：頻率高時，很難讓很多條銅線上的資料保持同步。
- 一個 DIMM 通常 1 到 4 個 rank；組 rank 的好處是**高頻寬**，又讓每顆 chip 保持小、省電。DDR4 的 bank 再分成 **bank group**（[第 43 支](lecture43_note.md)）。
- 一個 bank 裡的 array 不能分開定址；同一個 chip 裡所有 bank 的 array 數一樣。

---

## 10. Channel 上的訊號與 bus 的負載

[53:53] channel 上送什麼：

| 訊號 | 方向 | 說明 |
|---|---|---|
| **Address** | 單向（controller → DRAM） | 要哪個位址 |
| **Command** | 單向 | 讀、寫、refresh…… |
| **Data** | [54:24] **雙向** | 寫：controller → DRAM；讀：反過來 |
| **Chip select** | 單向 | [54:57]「歷史很久」：以前選一顆 chip，現在用來**選 rank** |

[55:29] 一個 channel 接好幾個 DIMM，怎麼共用：

| 方式 | 做法 |
|---|---|
| **時間上分** | 「我用 20 個 cycle、你用 20 個 cycle」：老師說他比較喜歡這個 |
| **空間上分** | 128 bit 的 channel 分成兩個 64 bit，一個 DIMM 用一半 |

[56:05]（課本圖 10.14）address 和 command 共用一條 **address/command bus**。例：16 個 bank（假設一個 bank 一顆 chip）排成矩陣，**一個 column 是一個 rank**：

- [57:14] 對 rank 下的命令（activate 打開一個 row、close、refresh、拿資料……）和位址（「大家都讀第 13 個 row 的第 17 個 column」）都要**同時**到達所有 chip → address/command bus 是一條**廣播**的 bus，**接到每一顆 chip**。
- [58:07] 只有被 chip select 選到的那個 rank 會處理。
- [58:34] **data bus** 可以分開：例如分成 4 條 lane，每條 16 bit，接到每個 rank 的同一個位置的 bank；它不需要每個訊號同時到達。

[59:07] 結果：

| Bus | 接幾個 | 負載 | 速度 |
|---|---|---|---|
| **Address/command** | 每一顆（例中 16 個） | 連接越多 → 線越長、電容越大（很多 gate 電容並聯）→ RC delay 高 | **慢** |
| **Data**（每條 lane） | 只有一排（例中 4 個） | 輕 | **快** |

[60:08]「如果只記一件事：**address/command bus 負載很重，data bus 沒那麼重，所以快很多**。」[60:38] 這就是 **DDR**（double data rate）的關鍵：data bus 跑的速度是 address/command bus 的**兩倍**，「因為它本來就是比較快的 bus」。

[61:08] 下一講：DDR4 協定的細節。

【課本補充】課本 p.549–551：

- 4 個 rank 的話 chip select 要 2 bit。address 和 command 可以分開，也可以共用一條，因為兩者通常在**不同時間**送。
- 一個 channel 接好幾個 DIMM 的好處：① 把 channel 分給兩個 DIMM（128 bit 分成兩個 64 bit），兩個同步動作 → 頻寬加倍；② 可以接不一樣的 DIMM（例如一個 64 bit、一個 32 bit 介面，給舊系統用），但不能同時用，要交錯存取；③ DIMM 忙著做存取時 channel 是閒的，可以拿去存取別的 DIMM。
- Important Point 18（p.550）：例中 address/command bus 接 16 個 bank、每條 data lane 只接 4 個 → address/command bus 的電容負載是 **4 倍**，「這個關鍵觀察之後設計 DDR 時會用到」。

---

## 11. 【實測】

程式在 `experiments/lecture41/dram.c`（`gcc -O2 dram.c -o dram`）。綁在 cpu2，配 1 GB 的記憶體（要求 huge page），用 TSC 計時（本機量到 TSC = 2.803 GHz）。每次讀之前先 `clflush`，確定一定要去 DRAM。

⚠️ 本機的 DIMM 規格（DDR 世代、幾個 rank、x4 還是 x16）要 root 才能讀（`dmidecode -t memory`），本機沒有權限，**待補**。`free -g` 顯示總共約 38 GB。

### 11.1 Row buffer：同一個 bank 的不同 row 互相干擾

輪流讀兩個位址（一個固定的 base、一個隨機的 q，各 `clflush` 後讀），重複 200 次取 q 的延遲中位數。隨機選 2000 個 q：

| 延遲（ns） | 個數 |
|---|---|
| 64–68 | 6 |
| 68–72 | **1096** |
| 72–76 | **700** |
| 76–80 | 21 |
| 80–84 | 11 |
| 84–88 | 34 |
| 88–92 | 46 |
| 92–96 | 18 |
| 96–112 | 54 |
| 112–128 | 14 |

- 明顯**兩群**：約 90% 的 q 在 **68–76 ns**；約 9% 慢了 15–50 ns（84–125 ns）。
- 照 [§8](#8-讀寫refresh-的步驟) 的道理：一次存取會**打開一整個 row**，值留在 sense amplifier（row buffer）裡。q 跟 base 在**不同 bank**：各開各的 row，互不干擾；q 跟 base 在**同一個 bank 的不同 row**：每次都要把對方的 row 關掉、precharge、再開自己的 → 慢。慢的那一群就是後者。
- 本機的位址怎麼對應到 channel、rank、bank（[第 43 支](lecture43_note.md) 的 address mapping）不知道，所以「9%」對應到幾個 bank，**待補**。

### 11.2 每隔約 1.8 µs 就有一次特別慢

一直 `clflush` ＋ 讀**同一個**位址 400 萬次（共 597 ms，平均每次約 150 ns），記下每一次的時間和延遲：

```
中位數 72.4 ns；p99 227.2 ns；p99.9 289.3 ns
```

比中位數慢 40 ns 以上的有 404825 次（約 10%）。看**相鄰兩次慢的存取之間隔多久**：

| 間隔（ns） | 佔所有間隔 |
|---|---|
| 100–600 | 約 16 % |
| 600–700 | 9.1 % |
| 700–1600 | 約 6 % |
| **1600–1700** | **17.9 %** |
| **1700–1800** | **7.4 %** |
| **1800–1900** | **26.4 %** |
| **1900–2000** | **11.7 %** |
| 2000–2200 | 2.3 % |

- **約 63% 的間隔落在 1.6–2.0 µs**：同一個位址的存取，大約**每 1.8 µs** 就有一次特別慢（量測的解析度只到一次迴圈約 150 ns，所以分散在 1.65、1.8、1.95 µs 幾格）。最前面幾次慢的時間點是 425、2235、4160、6083、7872、9681 ns：間隔都約 1.8–1.9 µs。
- 用整段 597 ms 去找固定週期（Rayleigh 檢定）反而找不到：TSC 和 DRAM 的時鐘不是同一個來源，週期只要差一點點，幾十萬個週期累積下來相位就亂了；看**相鄰間隔**就不受影響。
- 最有可能的解釋是 **refresh**（[§8](#8-讀寫refresh-的步驟)）：refresh 時那個 bank（或整個 rank）不能存取，剛好碰上的讀就要等。課本 p.573 的例子是 64 ms ÷ 8192 個 row = **每 7.8 µs** 送一次 refresh 命令（[第 43 支](lecture43_note.md)）；本機量到的約 1.8 µs 大約是它的 **1/4**。差在哪裡（DDR 的世代、幾個 rank 輪流 refresh、或其他週期性的事件）要先知道本機的 DIMM 規格，**待補**。

---

## 12. 一頁速查表

| 主題 | 重點 |
|---|---|
| DRAM cell | 1 個電容 ＋ 1 個 access transistor；word line、bit line |
| 讀 | precharge 到 Vdd/2，看偏向哪邊；**破壞性**，讀完要寫回 |
| Refresh | 電容漏電；每個 row 32–64 ms 一次；burst 或 distributed |
| 電容 | trench（往下挖 4–6 µm，難做）；stacked（往上疊，常用） |
| 陣列 | sense amplifier 在 column mux **之前**、每個 column 一個；一次開一整個 row（page） |
| Bit line 架構 | open（6F²、分段）；folded（8F²、兩條靠在一起抗雜訊）；影片和課本對雜訊、速度的說法不同 |
| Sense amplifier | equalizer（EQ、Vdd/2）＋ 4 個 transistor 交叉回授（SAN、SAP）；precharge、sense、store |
| 寫 | 先開 row（等於讀一次），再用 write driver 改 sense amplifier 和 cell；write recovery time |
| 階層 | channel → DIMM → rank → chip → bank → array → row → column |
| Rank | 同步動作、跟 controller 等距；controller 對 rank 下命令 |
| xN | 一個 bank 有 N 個 array，一次出 N bit |
| Bus | address/command 單向、廣播到每顆 chip（慢）；data 雙向、分 lane（快）→ DDR；chip select 選 rank |
| 實測 | 同 bank 不同 row 的兩個位址輪流讀慢 15–50 ns（約 9% 的隨機位址）；同一個位址每約 1.8 µs 有一次特別慢（像是 refresh） |

---

## 13. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| SRAM cell、precharge、sense amplifier | [第 23 支](lecture23_note.md) |
| Cache 的 miss 到 memory controller | [第 21 支](lecture21_note.md)、[第 25 支](lecture25_note.md) |
| 非同步、同步、buffered 的 DRAM 協定、DDR 世代 | [第 42 支](lecture42_note.md) |
| DDR4 的時序、refresh 命令、memory controller、address mapping | [第 43 支](lecture43_note.md) |
| Flash、FeRAM | [第 44 支](lecture44_note.md) |
| MRAM、PCM、ReRAM、3D 記憶體、roofline model | [第 45 支](lecture45_note.md) |

---

## 附：本講一句話總結

> **DRAM cell 只有一個電容和一個 access transistor，所以密度遠高於 6 個 transistor 的 SRAM，代價是讀要先 precharge 到 Vdd/2 再用差動的 sense amplifier 看電壓往哪邊偏、讀完電容的電就沒了得寫回去、電容還會漏電所以每 32–64 ms 要 refresh；電容可以往下挖成 trench 或往上疊成 stacked，陣列裡每個 column 都有一個兼當 buffer 的 sense amplifier（放在 column mux 之前），一次開一整個 row，bit line 有 open（6F²）和 folded（8F²）兩種排法；系統上是 channel → DIMM → rank → chip → bank → array，rank 是同步動作、跟 memory controller 等距的一組 chip，address/command bus 要廣播到每一顆 chip 所以慢，data bus 分成 lane 負載輕所以快，這就是 DDR 能讓 data bus 跑兩倍速的原因。本機實測：隨機兩個位址輪流讀，約 9% 的組合慢了 15–50 ns（同一個 bank 的不同 row 互相關掉對方），而一直讀同一個位址時約每 1.8 µs 就有一次特別慢，很像 refresh，只是比課本例子的 7.8 µs 短。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 10: Main Memory (DRAM devices, arrays and architectures) -- Part I 的 YouTube 字幕（1:01:50）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch10 §10.1（p.536–551）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
