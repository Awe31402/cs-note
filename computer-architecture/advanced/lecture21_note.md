# Advanced Computer Architecture 第二十一支影片完整筆記：Cache 的基本設計：locality、direct mapped、fully／set associative、寫入與替換（Chapter 7, Part I）

> **課程**：Advanced Computer Architecture — Chapter 7: Caches (Part I) -- Cache design（播放清單第 21 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（1:01:25）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch7** §7.1.1–7.1.5（p.253–268）
> **章號**：影片 Chapter 7 = 課本 Ch7。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有；[15:47] 是老師說「學生常答錯的題目」）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

這一講大部分是大學部的複習。Basic 課程已經講過、也在本機量過一大部分：[Basic 第 34 支](../basic/lecture34_note.md)（locality、cache 階層、block）、[Basic 第 35 支](../basic/lecture35_note.md)（三種對應方式、寫入、替換，含 cache 模擬器）。這份筆記照影片的順序整理，實測只做 Basic 沒做過的部分。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 這一章要什麼背景、講什麼](#1-這一章要什麼背景講什麼)
- [2. 筆電、書桌、書架](#2-筆電書桌書架)
- [3. Temporal 和 spatial locality](#3-temporal-和-spatial-locality)
- [4. 記憶體階層](#4-記憶體階層)
- [5. Block](#5-block)
- [6. 只寫一個 byte，也要整個 block](#6-只寫一個-byte也要整個-block)
- [7. Direct-mapped cache](#7-direct-mapped-cache)
- [8. CAM 與 fully associative cache](#8-cam-與-fully-associative-cache)
- [9. Set associative cache](#9-set-associative-cache)
- [10. 三種 tag 的長度](#10-三種-tag-的長度)
- [11. Hit 和 miss 時做什麼](#11-hit-和-miss-時做什麼)
- [12. Write-through 與 write-back](#12-write-through-與-write-back)
- [13. 替換策略](#13-替換策略)
- [14. 【實測】](#14-實測)
- [15. 一頁速查表](#15-一頁速查表)
- [16. 與後續講次的連結](#16-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
人的習慣：常用的放身邊（筆電）、次常用的放書桌、很少用的放書架
→ 程式也是：temporal locality（同樣的位址一再用）、spatial locality（附近的位址一起用）
→ 階層：I-cache／D-cache（2–4 cycle）→ L2（10–50）→ L3 →（L4）→ 主記憶體（200–400）；inclusive
以 block（32–128 B）為單位；寫一個 byte 也要先把整個 block 拿進來
放在哪：direct mapped（index ＋ tag）→ fully associative（CAM）→ set associative（折衷）
寫：write-through vs write-back（modified bit）
換：random、FIFO、LRU（不實際）→ pseudo-LRU（飽和計數器 ＋ 定期清零）
```

---

## 1. 這一章要什麼背景、講什麼

[00:15] Cache 和記憶體系統「是現代電腦系統最重要的部分之一」，這一章分好幾講。

背景：

- Cache 由 **SRAM 和 DRAM 陣列**做成 → 要一點 RC 電路（時間常數）、CMOS 電路、電流源和電壓源、記憶元件、latch 和數位邏輯。
- 之後要**猜**未來會用到哪些位址、提早拿來（**prefetch**）→ 要懂 [第 8 支](lecture08_note.md) 的 branch prediction。

[01:48] 這一章的內容：

1. 記憶體系統概論（大學部的複習再多一點）。
2. 怎麼模擬、設計 cache：例如 **CACTI** 工具（[第 24 支](lecture24_note.md)）。
3. 進階的 cache 設計（[第 25 支](lecture25_note.md)）。
4. Intel 的 **trace cache**（[第 26 支](lecture26_note.md)）。
5. **Prefetching**：概念上像 branch prediction，猜之後會存取的位址（[第 26 支](lecture26_note.md)）。

---

## 2. 筆電、書桌、書架

[02:23] 一張圖：一個人用筆電，桌上有幾本書，背後有一整個書架。

| 位置 | 放什麼 | 大小 |
|---|---|---|
| 筆電 | 最常用的資料 | 最小 |
| 書桌 | 比較少用的書 | 比較大 |
| 書架 | 很少用、但還是需要的書 | 最大 |

[03:28] 兩個觀察：

1. **常用的放身邊。**
2. 桌上放**主題相近**的書：例如在找度假地點，就也會看一點極限運動、料理的書。

---

## 3. Temporal 和 spatial locality

[04:33]

| | 人的生活 | 電腦 |
|---|---|---|
| **Temporal locality** | 一小段時間裡一直做同一件事（一直讀同一本書） | 一小段時間裡**同一組位址**被一再存取 |
| **Spatial locality** | 一小段時間裡做相似的事（讀主題相近的書） | 一小段時間裡存取**附近的位址**（proximate addresses） |

[06:38] 為什麼？

- 程式大部分時間在**迴圈**裡 → 同樣的指令一再執行 → temporal locality。
- 程式常常依序掃過大陣列 → 第 1、2、3… 個元素 → spatial locality。程式本身也像一個陣列：沒有 taken branch 時就一條接一條。

【課本補充】課本 p.255 §7.1.1。Basic 課程在本機用 valgrind 量過真實程式的 stack distance 和 address distance：[Basic 第 34 支 §14.2](../basic/lecture34_note.md#142-真實程式的-stack-distance-與-address-distance)。

---

## 4. 記憶體階層

[07:36] 「筆電、書桌、書架」原則套到記憶體系統：

| 層 | 像什麼 | 延遲 |
|---|---|---|
| **I-cache、D-cache** | 筆電／書桌 | **2–4 cycle** |
| **L2** | | 10–50 cycle |
| **L3**（伺服器常有）、L4（很少見） | | 更大、更慢 |
| **主記憶體**（晶片外、主機板上） | 書架 | 300–400（又說 200–400）cycle |

- [09:04] 處理器是「讀寫資料的人」。I-cache 供應指令，D-cache 跟處理器交換資料（可讀可寫）。
- [09:38] 它們很快，**因為**很小：只存位址空間的一個子集合，最常用的那些。

[10:50] 階層是 **inclusive**（包含的）：I-cache 的內容是 L2 的子集合，L2 是主記憶體的子集合。也可以做 exclusive 的設計，但很少見，這一章都假設 inclusive。

[11:24] 程式用的所有位址叫 **physical address space**（之後再談這個抽象）；暫時假設主記憶體有所有位址的資料。

【課本補充】課本 p.258：L1 通常 **1–3 cycle**，L2 10–50 cycle，主記憶體 200–400 cycle；有 L3 的話更大（2–16 MB）、更慢（**20–80 cycle**）。

⚠️ L1 延遲：影片說 2–4 cycle，課本說 1–3 cycle。本機 P-core 量到約 **5 cycle**、E-core 約 3 cycle（[§14.1](#141-p-core-和-e-core-的各層延遲)）。

---

## 5. Block

[11:59] 怎麼利用 spatial locality？不以 byte 或 memory word（4 byte）為單位，而是以 **block**（32–128 byte）為單位，也叫 **cache line**。

- Block 是**不可分割的單位**：一次拿整個 block，從不拿半個、傳半個。
- 讀寫 block 裡一個 byte，因為 spatial locality，很可能也會用到 block 裡其他的 byte。

[13:10] **Cache** = 一組 block ＋ 每個 block 的 **metadata**（例如它的位址）。處理器存取 cache，很可能找到那個 block，再讀寫裡面的 byte。

[13:41] **不為每個 byte 存任何狀態**，只為整個 block 存一些高階資訊。

[20:26] 用詞：**block** 指 64 byte 的**內容**；**cache line** 指 cache 裡放一個 block 的**位置**。兩者常混用。

【課本補充】Basic 課程在本機用 stride 實驗「看見」了 64-byte 的 block：[Basic 第 34 支 §14.3](../basic/lecture34_note.md#143-看見-64-byte-的-cache-block)。

---

## 6. 只寫一個 byte，也要整個 block

[14:14] 【口頭】「這是一題陷阱題，很多學生答錯。」

情境：64-byte 的 block 不在 cache 裡，只想寫前 4 個 byte。反正要被蓋掉，可不可以直接開一個空位，只寫那 4 個 byte？

[14:43] **不行**：剩下的 60 個 byte 呢？

- 不想要「一部分寫了、其他部分不知道是什麼」的**半成品 block**。
- 要記得「只有第 8–11 個 byte 是新的，其他是垃圾」，就要計算和儲存空間，而且破壞了「block 是一個不可分割的單位」這個簡單的抽象。

[15:15] 所以：**就算只寫 1 個 byte，block 不在 cache 裡，就先從下一層把整個 64 byte 拿進來，再寫。**

[16:55] 讀很簡單：整個 block 都在，要第 10 個 byte 就讀它。

[16:55] 基本的流程：程式開始時所有 block 都在主記憶體；需要某個 block 時，從主記憶體拿上來，L2 留一份、I-cache 或 D-cache 留一份。處理器只直接讀寫 I-cache 和 D-cache。因為 inclusive，不會有「D-cache 有、L2 沒有」的情況。

（本機量了這個規則的代價，見 [§14.2](#142-只寫也要先讀整個-block)。）

---

## 7. Direct-mapped cache

[18:27] 跑一個例子：

```
32-bit 位址、block 64 B、cache 64 KB
→ 64 KB / 64 B = 1024 條 cache line = 2¹⁰
```

[19:46] 位址切成：

```
| tag（16 bit） | index（10 bit） | byte offset（6 bit） |
                └──── block address（26 bit）────┘
```

- **Byte offset**：64 = 2⁶ → 6 bit，指出 block 裡哪一個 byte。
- **Index**：10 bit，指出 1024 條裡的哪一條。這是 **direct-mapped**：每個 block 只能放在一個固定的位置。

[22:01] 跟 branch predictor 用 PC 的 bit 查飽和計數器很像（[第 8 支](lecture08_note.md)），也有 **destructive aliasing**。但有一個重要的不同：

| | aliasing 的後果 |
|---|---|
| Branch predictor | 只是猜錯、效能變差，**不影響正確性** |
| Cache | 不同位址對到同一格，**會讀到錯的資料** |

[23:00] 解法跟以前一樣：存 **tag**（剩下的 16 bit）。只有 tag 對了才存取。

[24:12] 兩個陣列，一對一對應：

| 陣列 | 存什麼 |
|---|---|
| **Tag array** | tag（＋ 其他 metadata） |
| **Data array** | block |

[24:40] 讀的流程：

```
index → 讀 tag array 那一格 → 跟位址的 tag 比 → 相等：hit；不等：miss
hit → data array 同一格 → 讀出要的 4 byte（寫的話寫那 4 或 8 byte）
```

[26:13] 為了效能，**tag array 和 data array 平行讀**：miss 就把讀出來的資料丟掉，hit 就省了時間。「為了效能，我們常常多做一些可能被丟掉的工作。」

[27:17] 寫不在關鍵路徑上，**讀**才是，所以重點放在讀。

[27:46] 問題：會有**熱點**。很多位址對到同一條 cache line，互相把對方趕出去，miss 很多。就算 cache 還有空間，有些 entry 的競爭還是很激烈。

---

## 8. CAM 與 fully associative cache

[28:52] **CAM cell**（content addressable memory，[第 13 支 §11](lecture13_note.md#11-方法四cam-做的-rat只存-valid-bit) 的 CAM-based RAT 看過）：

- 每個 CAM cell 存 1 個 bit，跟一個輸入 bit 比較。
- 一列裡**所有** cell 都相等 → 那一列的 **match line** = 1。
- 輸入一個內容，輸出所有 match line，只有一條是 1：告訴我們**哪一列**有一樣的內容。

[31:20] 用來做 **fully associative cache**：block 可以放在**任何一條** cache line，destructive interference 的機率最小。

[32:33] 運作：

```
tag 部分 → 送到 tag array（CAM）跟所有存著的 tag 比
→ 1024 條 match line → OR 起來：1 = hit，0 = miss
→ encoder：找出是哪一條 match（例：4-bit encoder，第 2 條是 1 → 輸出 10）
→ 用那個 index 讀 data array
```

[34:30] 這裡**不能**平行讀 tag 和 data：那要把 data array 每一格都讀出來，不實際。

[35:00] 缺點：**慢、耗電**，一般不會用。但有幾個**小**結構（16 或 32 格）還是偏好用 fully associative；64 KB 這麼大的「絕對不行」。

---

## 9. Set associative cache

[35:32] **折衷**：1024 條分成 **256 個 set**，每個 set 4 條（index 連續）。

- block 先對到一個 **set**（不是一條 line）。
- 在 set 裡面，像 fully associative 一樣，可以放任何一條：**有限的搜尋**。
- 不用耗電的 CAM，用一般的 SRAM 和比較器。

[38:02] 位址：

```
| tag（18 bit） | index（8 bit） | offset（6 bit） |
```

256 = 2⁸ → 8 bit；32 − 8 − 6 = **18 bit** 的 tag。

[40:51] 每個 set 有 4 條叫 **4-way**；K 條叫 K-way。

[41:25] 怎麼查：

```
8-bit index → 接上 00、01、10、11 → 4 條 line 的位址
讀這 4 個 tag，各自跟位址的 tag 比 → 4 輸入 OR：hit／miss
hit → encoder 告訴我們是第幾條（0–3）→ 選 multiplexer：4 個位址選一個 → 讀 data array
```

[42:27] 【口頭】「看不懂就去複習：encoder 怎麼運作（任何數位邏輯的書）、怎麼從 set index 產生 4 個位址（boolean algebra）、encoder 和 multiplexer 怎麼接。對大部分電機電子的學生不難。」

[44:27] 一樣可以**平行**讀 4 個 tag 和 4 個 data：省時間，但多耗電（做了白工）。對加速讀很有用。

【課本補充】Basic 課程在本機用 conflict 測出 L1 的 associativity，也用模擬器比了 1、2、4、8、128-way 的 miss rate：[Basic 第 35 支 §17.1–17.2](../basic/lecture35_note.md#171-量出本機-l1-的-associativity)。本機的 associativity 見 [§14.1](#141-p-core-和-e-core-的各層延遲)。

---

## 10. 三種 tag 的長度

[39:12] 回頭回答 fully associative 的 tag 有多長：沒有 index（0 bit），整個 block address 都是 tag。

| 設計 | index | tag |
|---|---|---|
| Direct mapped（1024 條） | 10 bit | **16 bit** |
| 4-way（256 個 set） | 8 bit | **18 bit** |
| Fully associative | 0 bit | **26 bit** |

[40:17] Fully associative 可以想成「只有一個 set、包含所有 line」的 set associative cache。

---

## 11. Hit 和 miss 時做什麼

[45:31] 讀：

| 情況 | 做什麼 |
|---|---|
| **Hit** | lookup（tag）和讀 data **重疊** → 選出命中的那個 block |
| **Miss** | lookup → 送請求到下一層（下一層一樣的流程，hit 就給，miss 再往下）→ 拿到 block → **insert** |

[46:35] Insert：看 set 裡有沒有空的 line；有就放進去。沒有就要**趕走（evict）**一個：

1. 找一個**替換的候選**（不要換掉常用的）。
2. **Evict**：如果它被**改過**，寫回下一層；沒改過就不用（下一層有一樣的副本）。
3. 放進新的 block。

[49:28] Tag array 每一格除了 tag 還有兩個 bit：

| Bit | 意思 |
|---|---|
| **valid** | 這一條有沒有有效的資料（空的還是滿的） |
| **modified** | block 進來之後，有沒有任何 byte 被寫過 |

---

## 12. Write-through 與 write-back

[49:57]

| | **Write-through** | **Write-back** |
|---|---|---|
| 寫的時候 | **每次**寫都傳到下一層 | 只寫這一層，**modified bit 設 1** |
| Evict 的時候 | 直接丟：上下兩份永遠一樣 | modified 的才寫回下一層 |
| 代價 | 訊息多、耗電 | 每個 block 多 1 bit |
| | | 「效能比較好」 |

[52:15] Write-back cache 的寫：

- Hit：不能像讀那樣 4 個 way 都寫，要先知道哪一個 way hit，再寫那一個。
- Miss：就算只寫 4 byte，也要拿整個 64 byte（[§6](#6-只寫一個-byte也要整個-block)）→ 放在暫存的 buffer 寫 → 整個 block insert（沒空間就替換、evict、insert）。

[53:58] Write-through cache：

- Hit：每一次寫都傳到下一層，保證上下兩份一樣。
- Miss：去 L2，在 L2 寫、把整個 block 拿上來寫好，再 insert；要替換時可以**直接丟**（上下一樣）。

【課本補充】Basic 課程的模擬器比過：同樣的 trace，write-through 寫到下一層的次數是 write-back 的約 94 倍（[Basic 第 35 支 §17.2](../basic/lecture35_note.md#172-cache-模擬器)）。

---

## 13. 替換策略

[55:35] K-way 的 set 裡選誰趕走：

| 策略 | 做法 | 評價 |
|---|---|---|
| **Random** | 隨便選 | 實際上不太差，但不是最好 |
| **FIFO** | 每條存一個時間戳（放進來的時間），趕最舊的 | 多存東西；最舊的可能剛好是最常用的 → 實際上不太用 |
| **LRU** | 趕「最近最少用」的 | 效果最好，很多理論情境可以證明它最佳；但每次存取都要更新時間戳，**在硬體 cache 裡不實際** |
| **Pseudo-LRU** | 近似 LRU（下面） | 實際用的 |

[57:49] 【口頭】「LRU 在軟體裡很容易做，但在硬體、而且是 cache 層級，不實際。FIFO 至少只在放進來時建一次時間戳。」

[58:17] **Pseudo-LRU**：

1. 每個 tag 配一個 **K-bit 飽和計數器**（2、3、4 bit 都可以）。
2. 每次存取 → 計數器 +1：越常、越近用到的值越大。
3. 問題：最後全部都會飽和。所以**定期**把整個 cache 的計數器減小。
4. [59:24] 減 1 是很難的操作；可以把 3-bit 計數器存成 3 個 bit 陣列，**定期把其中一整個陣列清成 0**（課本的 **flash clear**），比一個一個減快很多。
5. 選 set 裡計數器**最小**的趕走：因為 temporal locality，很可能以後也不會用到它。

【課本補充】課本 p.267 說明了 pseudo-LRU；它只是近似，不一定總是選到真正最近最少用的那個。Basic 課程比過四種策略（LRU 最好、pseudo-LRU 幾乎一樣），也試了「比 way 多一條的循環」讓 LRU 和 FIFO 100% miss：[Basic 第 35 支 §17.2–17.3](../basic/lecture35_note.md#172-cache-模擬器)。

---

## 14. 【實測】

程式在 `experiments/lecture21/`。

### 14.1 P-core 和 E-core 的各層延遲

先看本機的 cache 長什麼樣子（`/sys/devices/system/cpu/cpuN/cache/`）：

| | P-core（cpu2） | E-core（cpu14） |
|---|---|---|
| L1 data | 48 KB、**12-way**（cpu2–3 共用＝同一個核心的兩個 hyperthread） | 32 KB、**8-way**（只有 cpu14） |
| L1 instruction | 32 KB、8-way | 64 KB、8-way |
| L2 | 1.25 MB、10-way（cpu2–3） | **2 MB、16-way，cpu12–15 四個 E-core 共用** |
| L3 | 24 MB、12-way，**全部 20 個 CPU 共用** | 同一個 L3 |
| Line | 64 B | 64 B |

`lat.c`：每個元素佔一整條 64-byte 的 line，用**隨機順序**串成一個環（prefetcher 猜不到），一直 `p = *p`。每一步都要等上一步，所以每步的時間就是一次 load 的延遲：

| 資料大小 | P-core ns | ≈ cycle（4.7 GHz） | E-core ns | ≈ cycle（3.6 GHz） |
|---|---|---|---|---|
| 16 KB | 1.11 | 5 | 0.84 | **3** |
| 32 KB | 1.07 | 5 | 0.84 | 3 |
| 48 KB | 1.07 | **5** | 5.57 | 20 |
| 64 KB | 3.19 | 15 | 5.57 | 20 |
| 256 KB | 3.21 | **15** | 6.22 | 22 |
| 1 MB | 5.01 | 24 | 7.71 | 28 |
| 2 MB | 11.77 | 55 | 14.17 | 51 |
| 4 MB | 13.86 | 65 | 20.83 | 75 |
| 16 MB | 25.28 | 119 | 37.27 | 134 |
| 32 MB | 67.57 | 318 | 101.07 | 364 |
| 128 MB | 92.08 | **433** | 108.34 | 390 |

- 每一層的大小都看得出來：P-core 到 **48 KB** 為止都是 L1（5 cycle），E-core 到 **32 KB**（3 cycle）；E-core 48 KB 就跳到 L2。
- 主記憶體約 **90–110 ns**，以 cycle 算 **390–430 cycle**，在影片 [10:13] 的「300–400 cycle」範圍的上緣。
- ⚠️ L1：P-core 5 cycle 比影片（2–4）和課本（1–3）都慢；E-core 3 cycle 在範圍內。P-core 的 L1 比較大（48 KB、12-way），越大越慢（[第 24 支](lecture24_note.md) 的 CACTI 會講）。
- L2：P-core 約 15 cycle、E-core 約 20 cycle，都在 10–50 cycle 的範圍。
- 1 MB 以上的時間也包含 **TLB miss**（隨機跳過太多 4 KB 的 page，[第 22 支](lecture22_note.md) 講）。所以 1 MB（P-core 的 L2 裡）已經比 256 KB 慢，L3 的數字也偏大，不完全是 cache 本身的延遲。
- Basic 課程在 P-core 上用另一個程式量過一樣的層次（[Basic 第 34 支 §14.1](../basic/lecture34_note.md#141-本機的-cache-階層)），數字一致。

### 14.2 只寫也要先讀整個 block

`rfo.c`：對一個 **1 GiB** 的陣列（遠大於 24 MB 的 L3）**只寫不讀**，比兩種寫法：

- **normal**：一般的 store。照 [§6](#6-只寫一個-byte也要整個-block)，寫一個不在 cache 裡的 block，要先把整個 block 從記憶體讀進來（read-for-ownership）。
- **nt**：non-temporal store（`_mm_stream_si64`，x86 的 `movnti`）：告訴 CPU「這塊我只寫、不會再讀」，繞過 cache，**不讀舊的 block**。

`experiments/lecture21/rfo.c:18-21`：

```c
		for (long i = 0; i < n; i++) a[i] = i + r;
		double t1 = now();
		for (long i = 0; i < n; i++) _mm_stream_si64((long long *)&a[i], i + r);
		_mm_sfence();
```

| | P-core 第一次 | P-core 第二次 | E-core |
|---|---|---|---|
| normal store | 16.37 GB/s | 16.36 GB/s | 8.32 GB/s |
| nt store | **35.70** GB/s | **35.46** GB/s | **17.39** GB/s |
| 只讀（對照） | 25.96 GB/s | 27.33 GB/s | 19.28 GB/s |

- 一般的 store **只有 nt store 的一半**。寫 1 GiB，一般的 store 實際上要「從記憶體讀 1 GiB ＋ 寫回 1 GiB」，記憶體的流量是兩倍。這就是 [14:43] 的「只寫 4 byte，也要先拿整個 64 byte」：整條 line 都要被蓋掉，CPU 還是先把它讀進來。
- nt store 不讀，所以快一倍，甚至比只讀還快。
- 代價：nt store 繞過 cache，寫完的資料**不在 cache 裡**。之後馬上要讀的話反而慢。所以只用在「一大塊只寫、近期不會再讀」的情況（例如 memset、複製大量資料）。

---

## 15. 一頁速查表

| 主題 | 重點 |
|---|---|
| 比喻 | 筆電（最常用、最小）、書桌、書架（最少用、最大） |
| Locality | temporal：同樣的位址一再用（迴圈）；spatial：附近的位址一起用（陣列、循序的指令） |
| 階層 | I/D-cache 2–4 cycle（課本 1–3）→ L2 10–50 → L3（20–80）→ 記憶體 200–400；inclusive |
| Block | 32–128 B；不可分割；不存 per-byte 狀態；寫 1 byte 也要先拿整個 block |
| Direct mapped | 64 KB／64 B：tag 16、index 10、offset 6；tag array ＋ data array；平行讀；熱點 |
| Fully associative | CAM：match line → OR → encoder；tag 26 bit；慢、耗電；只用在 16–32 格的小結構 |
| Set associative | 4-way、256 set：tag 18、index 8；讀 4 個 tag 比較 → encoder ＋ mux |
| Miss | 往下一層要 → insert：有空位就放；沒有就選候選 → evict（modified 才寫回）→ insert |
| Metadata | valid bit、modified bit |
| Write-through | 每次寫都傳下去；evict 直接丟 |
| Write-back | 只設 modified；evict 時才寫回；比較省 |
| 替換 | random、FIFO（時間戳）、LRU（不實際）、pseudo-LRU（飽和計數器 ＋ 定期 flash clear，趕最小的） |
| 實測 | P-core L1 48 KB 5 cycle、E-core L1 32 KB 3 cycle；記憶體約 400 cycle；一般 store 只有 nt store 一半快 |

---

## 16. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| 大學部的 cache（含本機實測） | [Basic 第 34 支](../basic/lecture34_note.md)、[Basic 第 35 支](../basic/lecture35_note.md)、[Basic 第 36 支](../basic/lecture36_note.md) |
| 飽和計數器、aliasing、tag | [第 8 支](lecture08_note.md) |
| CAM | [第 13 支](lecture13_note.md) |
| Virtual memory、TLB、physical address | [第 22 支](lecture22_note.md) |
| SRAM、CAM 陣列的電路 | [第 23 支](lecture23_note.md) |
| CACTI、Elmore delay | [第 24 支](lecture24_note.md) |
| 進階的 cache 最佳化、MSHR | [第 25 支](lecture25_note.md) |
| Trace cache、prefetching | [第 26 支](lecture26_note.md) |
| 多核心的 cache、inclusive、write-through 的好處 | [第 33 支](lecture33_note.md)–[第 36 支](lecture36_note.md)（Ch9） |
| DRAM | [第 41 支](lecture41_note.md)（Ch10） |

---

## 附：本講一句話總結

> **人會把常用的東西放在身邊、主題相近的一起放，程式也一樣有 temporal 和 spatial locality，所以記憶體做成階層：很小很快的 I-cache／D-cache（2–4 cycle）、L2（10–50）、L3，再到主記憶體（200–400），而且是 inclusive；資料以 32–128 byte 的 block 為不可分割的單位，就算只寫一個 byte 也要先把整個 block 拿進來；block 放在哪有三種做法：direct mapped 用 index 直接找、存 tag 避免 aliasing（64 KB／64 B 時 tag 16 bit），fully associative 用 CAM 讓 block 放任何地方（tag 26 bit，但慢又耗電，只用在很小的結構），set associative 是折衷（4-way 時 tag 18 bit）；為了速度可以平行讀 tag 和 data；miss 時往下一層拿、沒空位就選一個趕走（modified 的才寫回），寫入分 write-through（每次都寫下去）和 write-back（modified bit），替換用不實際的 LRU 的近似：飽和計數器加定期清零的 pseudo-LRU。本機實測：P-core 的 L1 是 48 KB、5 cycle，E-core 是 32 KB、3 cycle，主記憶體約 400 cycle；而一般的 store 寫一大塊記憶體只有繞過 cache 的 non-temporal store 一半快，因為它真的先把每個 block 讀了進來。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 7: Caches (Part I) -- Cache design 的 YouTube 自動字幕（1:01:25）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch7 §7.1.1–7.1.5（p.253–268）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4）結果。*
