# Advanced Computer Architecture 第四十二支影片完整筆記：主記憶體（二）：非同步、同步、DDR 的協定，prefetch length 與 buffered DIMM（Chapter 10, Part II）

> **課程**：Advanced Computer Architecture — Chapter 10: Main Memory (asynchronous, synchronous and buffered communication) -- Part II（播放清單第 42 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕下載不到）（56:15）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch10** §10.2（p.551–562）
> **章號**：影片 Chapter 10 = 課本 Ch10。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 把「CAS」寫成「gas」、「AMB」寫成「amv／amd」、「poor bits」其實是「four bits」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 非同步傳輸與 strobe](#1-非同步傳輸與-strobe)
- [2. 非同步 DRAM：RAS 與 CAS](#2-非同步-dramras-與-cas)
- [3. Fast page mode 與 EDO](#3-fast-page-mode-與-edo)
- [4. Burst EDO：prefetch length](#4-burst-edoprefetch-length)
- [5. 同步 DRAM](#5-同步-dram)
- [6. DDR](#6-ddr)
- [7. DDR 的世代](#7-ddr-的世代)
- [8. 為什麼要 prefetch：內部時鐘比 bus 慢](#8-為什麼要-prefetch內部時鐘比-bus-慢)
- [9. Fully buffered DIMM](#9-fully-buffered-dimm)
- [10. Registered DIMM](#10-registered-dimm)
- [11. 【實測】](#11-實測)
- [12. 一頁速查表](#12-一頁速查表)
- [13. 與後續講次的連結](#13-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
非同步：沒有共用時鐘 → 用 strobe 告訴對方何時鎖存（避開 setup/hold 視窗，免得 metastability）
  RAS／CAS（active low）→ FPM／EDO（row 不關，讀同一 row 的好幾個 column）→ BEDO（一個 column 位址，內部產生後面的：prefetch length）
同步（SDRAM）：共用時鐘；ACT、NOP、CAS 是命令
DDR：data bus 負載輕、快 → 時鐘兩個邊緣都送資料；DQS 跟資料一起送（source synchronous）
  DDR → DDR2 → DDR3 → DDR4：bus 100 MHz → 2133 MHz；prefetch 2 → 8；電壓 2.6 → 1.2 V；DIMM 1 → 64 GB
  內部 200 MHz、bus 800 MHz → 每個內部 cycle 要出 2 × 800/200 = 8 bit → 64 bit × 8 = 最少 64 B 一次
bus 越快能接的 DIMM 越少 → FB-DIMM（AMB、串列 lane；沒流行）→ RDIMM（位址／命令加 register；常用）
```

---

## 1. 非同步傳輸與 strobe

[00:00]「DRAM 的第二講。」如果你覺得 cache 很複雜，「看看 DRAM：整整一章都還只是皮毛。」

**問題**：memory controller 和 DRAM 怎麼溝通？

[00:00] 最直覺的：**非同步**：有資料或位址要送就放上 bus，對方自己拿。[01:23] 問題：雙方**沒有共用時鐘**。

- 接收端最後要把每個 bit 鎖進一個 D flip-flop。flip-flop 在時鐘邊緣前 **t_setup**、之後 **t_hold** 這段時間裡，輸入必須**穩定**。
- [02:18] 接收端有自己的時鐘，訊號不能在這段時間裡變化，不然會發生 **metastability**：輸出會震盪，不知道鎖到的是 0 還是 1。

[03:13] 解法：資料旁邊**一起送時間資訊**：**strobe**。資料叫 **DQ**，strobe 叫 **DQS**。接收端看 strobe 就知道什麼時候鎖存。上升邊緣或下降邊緣觸發都可以，雙方約定好就行。

[04:09] 兩種：**active high**（變 1 的時候鎖存）、**active low**（變 0 的時候，符號上面加一槓）。

[04:09] 幾乎所有記憶體 bus 都是 **half duplex**：資料一次只往一個方向流。

【課本補充】課本 p.552：DRAM 是**被動**的記憶體，裡面沒有計算單元：排程、避免衝突、公平性**全部**交給 memory controller；controller 要知道每種 DRAM 的時序和支援的命令。課本也說 strobe 不一定只靠**轉換**傳資訊，有時也用**電位**表示某個單元有沒有被啟用。

---

## 2. 非同步 DRAM：RAS 與 CAS

[05:10]（課本圖 10.16）非同步 DRAM 有好幾個 strobe：

| 訊號 | 說明 |
|---|---|
| **RAS**（row address strobe，active low） | 從 1 變 0 時，DRAM 鎖存 address bus 上的 **row** 位址 |
| **CAS**（column address strobe，active low） | 從 1 變 0 時，鎖存 **column** 位址 |
| address | 位址線 |
| DQ | 資料線 |

- [06:07] 位址要在 strobe 轉換**之前**就穩定在 bus 上：64 條平行的線，要讓 64 個 bit 同時穩定，轉換的那一刻一起鎖進 flip-flop。
- [07:05] row 讀好之後送 column → 資料送回來 → RAS、CAS 回到 1，memory controller 讀資料。
- [07:57]「這是純非同步的，沒有共用時鐘」：完全靠 strobe 的 active low／high 協調。

【課本補充】課本 p.553：送 column 位址時可以一起送一個 bit 表示讀還是寫。讀的時候，DRAM 把資料放上 DQ 之後把 CAS 設回 1，告訴 controller 可以讀了。有些協定寫完會回一個 acknowledgement。

---

## 3. Fast page mode 與 EDO

[07:57] 很多時間 bus 是閒的，可以改進。

[08:49] 存取有**空間局部性**。DRAM 的一個 row 也叫 **page**（跟虛擬記憶體的 page 不同），每個 array 大約 **2 Kbit**：所有 array 加起來很大。

- [08:49] 打開一個 row 之後**先不要關**（「關」= 把 bit line 重新 precharge），值留在 sense amp 裡。
- 猜下一個存取還在同一個 row 的別的地方 → 直接送新的 column 位址就好。
- [09:43] 同一組 row、column 位址送到 64 個 array → 一次出 64 bit。

[10:39]（課本圖 10.18）**EDO**（Extended Data Out）：

- RAS 送 row，CAS 送 column；多一個 **data strobe**（active low）告訴 memory controller 什麼時候讀資料。
- [11:38] 送完一個 column、資料還在傳的時候，就送下一個 column。
- [14:40]「只要再加一個 strobe」；還是**非同步**的。

【課本補充】課本 p.553–555 分得更細：

| 模式 | 做法 |
|---|---|
| **FPM**（Fast Page Mode） | 整個 row 放在 sense amp 裡，之後只送 column 位址；用 CAS 當時間基準。例：每個 row 1024 個 column、每個 column 16 bit（一個 rank 16 個 array） |
| **EDO** | 送一個 column、內部先把資料暫存起來，資料傳的同時送下一個 column → column 之間的延遲變短；需要另一個 strobe **DQS** |

---

## 4. Burst EDO：prefetch length

[15:39]（課本圖 10.19）**BEDO**（Burst EDO）：閒置時間少很多，而且**不需要 data strobe**。

- [15:39] 通常一次要讀很多資料：每次 64 bit，但一個 cache block 是 64 byte = 8 次。下一個、再下一個 column 早就知道了。
- [16:44] 送 column n，DRAM 送回 n **和** n+1 的資料 → **prefetch length = 2**：只送一個位址，後面的位址在**內部**產生。column 和資料都用 CAS 當 strobe。
- [17:44]「burst」就是這個意思：內部算出後面的位址，一次出好幾個 column。prefetch length 可以加到 8，更有效率。

[17:44] 但非同步傳輸要複雜的 strobe，**對雜訊很敏感**，兩端的電路又複雜又笨重。

【課本補充】課本 p.555：**prefetch length**（prefetch width）：每個 DRAM array 一次讀幾個 bit。例如一次讀 8 bit，prefetch length 就是 8；在有時鐘的 bus 上要 8 個 bus cycle 送完（每個 cycle 1 bit）。

---

## 5. 同步 DRAM

[18:48] 結論：用**同步**（有時鐘）的方式比較好。**SDRAM**：

- 好設計、好驗證，不用複雜的 strobe 邏輯。
- 可以在**協定**層級做更多事。
- [18:48] 可以有複雜的階層架構：原則上可以分別控制不同的 bank，因為大家跟同一個時鐘同步。

【課本補充】課本 p.555–556：

- 非同步時 RAS、CAS 直接控制 bank，DRAM 裡沒辦法加可程式的邏輯；同步時只要送**命令**，DRAM 怎麼實作都可以。
- SDRAM 可以設定：切換模式、動態改 prefetch length（1、2、4、8）。
- 可以對不同 bank 送不同命令，例如讀一個 bank 的同時 precharge 另一個（pipeline）。
- memory controller 和 DRAM 的時鐘可能有**相位差** → 大部分 SDRAM 裡有 **DLL**（delay locked loop）把相位差壓到最小。
- 圖 10.20：CLK、Command、Address、DQ 四條 bus；命令和位址在時鐘的**上升邊緣**鎖存。ACT（打開 row）→ 一個 cycle → CAS ＋ column → 再一個 cycle 開始收資料；prefetch length 是 4。

---

## 6. DDR

[19:41] 回顧 [第 41 支 §10](lecture41_note.md#10-channel-上的訊號與-bus-的負載)：address/command bus 接到**所有** array，電容負載、RC 很大；data bus 只接一排 → 快。

[20:29] 所以 data bus 跑 address/command bus 的**兩倍速** → **DDR**（double data rate）。之後有 DDR1、2、3、4，「現在 DDR5 也來了」。

[21:35]（課本圖 10.21）時鐘的**上升和下降邊緣都送資料**：

```
Command:  ACT   NOP   CAS         CAS
Address:  ROW         COL         COL
DQ:                   d d d d     d d d d      （每個 cycle 兩個）
```

- ACT（activate：打開 row、讓 sense amp 同步）→ NOP（等 row 打開）→ CAS（名字沿用，「完全是歷史遺留」）＋ column。
- [22:29] prefetch length = 4。
- [23:27] **DQS** 有三種狀態：預設是 X（三態）、0 → 1 → 0 → 1 → 0、回到 X：每個轉換都代表有資料。
- [25:21] 命令：ACT、NOP（一個閒置的 cycle）、CAS（activate column）、COL（column 位址）。

[24:25] 兩段資料中間空了一個 cycle：

⚠️ 影片和課本對這個空檔的解釋不同：

| | 解釋 |
|---|---|
| 影片 [24:25]–[26:22] | DRAM 裡是類比電路，一個操作（讀 4 bit）之後要一段「休耕期」讓電壓穩定、時鐘重新同步，新的 column 才生效 |
| 課本 p.557 | 假設第二段資料是**另一個 rank** 送的（那個 rank 的 row 之前就打開了）；**換 rank** 要一個 cycle，因為新的 rank 要重新跟資料 bus 同步時鐘 |

[26:22] 一次通常傳 **64 byte = 512 bit**：每輪 64 bit 的話要 8 輪。最好一次傳輸只用**一個 rank**。[27:21]【口頭】也可以有 9 個 bank，其中一個備用，「現代的協定允許」。

[27:21] **Source synchronous**：資料和 strobe（當作時鐘）一起送，接收端從 strobe 還原時鐘，再還原資料。

| | strobe 的轉換在哪裡 | 為什麼 |
|---|---|---|
| **讀**（DRAM → controller） | 跟資料的轉換**對齊** | DRAM 在時鐘邊界送資料比較容易；controller 有能力從 strobe 還原時鐘 |
| **寫**（controller → DRAM） | 在**每個 bit 的中間** | [28:14] DRAM 沒那麼聰明、沒那麼多處理能力：收到 strobe 時資料已經穩定，直接拿 |

[28:14] **QDR**：

⚠️ [29:14] 老師在前一張投影片說 QDR 是「時鐘頻率的四倍」，這張投影片**自己更正**：QDR 是把 half duplex 的 bus 改成 **full duplex**（兩個方向同時傳）。課本 p.557：QDR 有**兩條**資料 bus，一條讀、一條寫，讀寫交錯時頻寬比較高。DDR 只有一條 half duplex 的 bus，所以全是讀或全是寫的時候效率最高。

【課本補充】課本 p.557：寫的時候 strobe 轉換放在 bit 中間，會讓 controller 那一端的電路**更複雜**：可以接受，「我們希望 DRAM 簡單，把複雜度搬到負擔得起的 controller」。DQS 的電壓由送資料的那一方（controller 或 DRAM）設定。

---

## 7. DDR 的世代

[29:14]（課本表 10.1）

| 技術 | bus 時鐘 | prefetch length | 傳輸率（MT/s） | 電壓 | 最大 DIMM |
|---|---|---|---|---|---|
| DDR | 100–200 MHz | 2 | 200–400 | 2.5–2.6 V | 1 GB |
| DDR2 | 200–400 MHz | 4 | 400–800 | 1.8 V | 4 GB |
| DDR3 | 400–1066 MHz | 8 | 800–2133 | 1.35／1.5 V | 16 GB |
| DDR4 | 800–2133 MHz | **8** | 1600–4266 | 1.2 V | 64 GB |

- [30:13] prefetch length **到 8 就飽和了**。
- **傳輸率 = 2 × bus 時鐘**（DDR 的 D）：單位是 MT/s（million transfers per second）。
- [31:14] 為什麼越來越快、越來越大：transistor 的 feature size 越來越小 → 時鐘越快、電壓越低、能存越多。
- [31:14] 這些是 **bus** 的時鐘：CPU（memory controller）和 DRAM 之間那條 bus 的速度。
- [38:39] 命名：**DDR3-1600** = 第三代 DDR、每秒 1600 百萬次傳輸（bus 800 MHz × 2）。

【課本補充】課本 p.557–558：DDR 之後**沒有根本性的改變**，只是製程和訊號技術變好 → 合稱 **DDRX**。「到 2020 年 3 月，DDR4 是最新的，DDR5 還在標準化。」電壓降低可以讓傳訊息更快，但對雜訊和 crosstalk 更敏感。

---

## 8. 為什麼要 prefetch：內部時鐘比 bus 慢

[32:24] DRAM **內部**的速度不用那麼快。例：bus 800 MHz、內部 400 MHz → 內部每個 cycle 要產生 **2 bit**，外面看起來才是每個 800 MHz 的 cycle 送 1 bit。

[33:17] DDR 的算術（課本 p.558 的例子）：bus 800 MHz、內部 200 MHz：

```
每個內部 cycle 要產生的 bit 數 = 2（DDR，每個 bus cycle 送 2 次）× 800 / 200 = 8 = prefetch length
```

[37:47] 一個內部 cycle 裡有 4 個 bus cycle、8 個邊緣：第 1 個 bit 在第一個邊緣，……第 8 個 bit 在第八個邊緣。

[34:25] 處理器到記憶體的 bus 寬 **64 bit**、prefetch length 8 → **最小的傳輸量 = 64 × 8 bit = 64 byte**。[38:39]「送一個 column 位址就產生 8 個；64 條平行的線 → 一個命令最少 64 byte。」

[38:39]【口頭】「最後這兩張投影片有點讓人混亂，多想幾次就會清楚；我試著用不同方式解釋了，也可以看書。」

【課本補充】課本 p.558：

- 為什麼 bus 頻率永遠比 DRAM 內部快：bus 這些年變快了，DRAM 元件沒有跟著快 → 只能靠**內部更寬**來補。DDR4 的內部時鐘是 200–533 MHz（每級加 33⅓ MHz），bus 是 800–2133 MHz。
- 名詞：所有 DDR 的 channel 寬 **64 bit**（加 ECC 是 **72 bit**）；每半個 cycle 送的 64 bit 叫一個 **memory word**，每半個 cycle 叫一個 **beat**；一串連續的 bit 叫 **burst**：DDR4 最小 burst length 是 8。
- 壞處：DDR3、DDR4 沒辦法傳小於 64 byte 的資料 → DDR3 加了 **burst chop** 模式：DRAM 可以忽略 8 個 beat 的後半，最小 burst 變 4 個 beat（但那些 beat 也沒送有用的資料）。

（本機實測「只讀 8 byte，DRAM 也搬整個 64 byte」，見 [§11.1](#111-只讀-8-b記憶體還是搬-64-b)。）

---

## 9. Fully buffered DIMM

[38:39] DDR 的問題：**不能擴充**。

- [39:43] bus 頻率快到頭了；bus 越快，越不能在上面放更多元件：會增加電容負載 → 一個 channel 能接的 DIMM、每個 DIMM 的元件數都越來越少。
- DDR 是被廣泛接受的標準，不能改。→ 嘗試**重新設計**記憶體，但 memory controller 看起來還是在講 DDR，只是後端換一種做法 → **FB-DIMM**（fully buffered DIMM）。

[40:36] 核心：在 memory controller 和 DRAM 中間加一個 **AMB**（advanced memory buffer）：每個 DIMM 一個，像是那個 rank 的代理介面（假設一個 DIMM 一個 rank）。

[41:37] 例：技術上一條 memory bus 只撐得住一個 DIMM，但你想接三個。

```
memory controller ═southbound═▶ AMB(DIMM1) ═▶ AMB(DIMM2) ═▶ AMB(DIMM3)
                  ◀═northbound═            ◀═            ◀═
```

- **southbound lane**：往記憶體的方向；**northbound lane**：往 memory controller 的方向。
- [42:32] 每條 lane 是**一個 bit 的高速串列 bus**：跟別的 lane 不用嚴格同步。
- 可靠：一條 lane 壞了可以**優雅地降級**，其他 lane 接手，或有備用的 lane。
- [43:29] AMB 是**有電源的主動元件**：只跟**自己 DIMM 裡**的元件溝通；給別的 DIMM 的就轉給下一個 AMB → DIMM 裡的 bus 負載也小了（DDR 的 bus 要接到每一個元件）。

[45:20] AMB 收到 southbound 的封包：

| 目的地 | 做法 |
|---|---|
| **自己的 DIMM** | 好幾條 lane 不同步 → 等全部到齊，**重組封包**，送給 DIMM 裡的元件 |
| **別的 DIMM** | [47:24] 不用重組，直接**轉送**（往南或往北） |

[47:24] 優點：

| 優點 | 說明 |
|---|---|
| 可擴充 | 一個 bus 可以接很多 DIMM |
| 高速 | lane 的 RC delay 小很多：沒有一條長長的銅線蛇行經過每個元件 |
| 少接腳 | [48:26] 一個 channel 就能接很多 DIMM，不用每顆晶片三個 channel → 晶片的接腳變少 |
| 可靠 | 幾條 lane 壞了沒關係 |

[48:26] 高速傳輸：southbound **10** 條 lane、northbound **14** 條；每條 lane 每個 DRAM cycle 送 **12 bit**（位址、命令、狀態、錯誤更正……）→ southbound 每個 DRAM cycle 120 bit、northbound 168 bit。

[50:23] **Resample、resync、reliability**：

| | 做法 | 取捨 |
|---|---|---|
| **Resample** | 直接讓訊號穿過 AMB，保留各 lane 之間的 clock skew | 快；但 skew 會累積，最後由**最慢**的訊號決定整組何時到（有時延遲會互相抵消） |
| **Resync** | [51:24] 每個 AMB 都重新同步、重組封包 | 每個 AMB 都增加延遲；也不讓延遲自然抵消 |
| **Bit lane steering** | 某條 lane 壞了，流量改走別的 lane | 可靠性 |

[52:25] 實際上要看一排有幾個 AMB，用兩者的混合。

[44:24] 為什麼沒流行：記憶體業界**不太接受**根本架構的改變；FB-DIMM 雖然盡量跟 DDR 相容，「實際上變成一個很不一樣的協定」。「很好的主意，但沒有起飛。」[44:24] 伺服器需要很大的記憶體容量，理論上 FB-DIMM 可以一直接下去。

【課本補充】課本 p.558–562：

- 例：DDR2（bus 400 MHz）一個 channel 可以接 **8** 個元件，800 MHz 的 DDR3 只能接 **2** 個。
- DRAM 業競爭激烈，廠商不願意在元件上加電路；所有 controller 和 RAM 都要符合 DDR 標準 → 創新的空間很小。
- FB-DIMM 是 2000–2005 年提出的。AMB 要當 router、序列化／反序列化、暫存資料、跟 DRAM 跑 DDR 協定、監控傳輸的可靠度 → 太貴。「任何革命性的技術都還是市場經濟的奴隸。」
- northbound 比較多 lane：給讀用，讀常在 critical path 上。錯誤偵測用 **CRC**，特別適合偵測一連串的錯誤。
- 頻寬比較：DDRX 每個 DRAM cycle 可以傳 144 bit（72 bit 寬的 bus，雙邊緣）；FB-DIMM 南向一個寫（64 bit 資料 ＋ 8 bit ECC = 72），北向同時 144 bit 的讀 → 共 **216 bit = DDRX 的 1.5 倍**。
- 錯誤太多時先 **channel reset**（兩端丟掉狀態重新同步），不行再用 **BIST**（built-in self test）找出壞掉的 lane，再 bit lane steering。

---

## 10. Registered DIMM

[52:25]「現在商業上買得到的是 **RDIMM**（registered DIMM）。」

- [53:22] DIMM 上加一個 **register**（一組 flip-flop）：替 **address 和 command bus** 做緩衝 → 不再是一條銅線蛇行經過每個元件，而是有緩衝的線。
- **完全支援一般的 DDRx 協定**，不用改。
- 緩衝多一點延遲，「大致上可以忍受」；但 RC delay 被壓住了 → 一個一般的 channel 可以接更多元件，像 FB-DIMM 一樣。
- [54:19] 沒有 FB-DIMM 的 resample、resync、reliability 那些好處；只替 address/command bus 加緩衝，因為 **data bus 本來就快**。
- [55:14] 多出來的延遲**可以 pipeline**，所以不太傷。RDIMM「稍微貴一點」，但大家用的是它。

[55:14] 下一講：DRAM 時序，特別是 **DDR4** 協定。

【課本補充】課本 p.562：**LRDIMM**（load-reduced DIMM）連 **data bus** 也加緩衝。RDIMM 的讀寫多延遲一個 cycle，但 bus 可以跑得更快、接更多 DIMM，「在伺服器市場非常流行」。缺點：主機板要特別設計，通常**不能**把一般 DIMM 和 RDIMM 混在一起用。

---

## 11. 【實測】

程式在 `experiments/lecture42/bw.c`（`gcc -O2 -fopenmp bw.c -o bw`；thread 綁在 cpu0、2、4、6、8、10 六個 P-core）。2 GB 的陣列（遠大於 24 MB 的 L3，要求 huge page），每個設定跑 3 次取最快的；整個程式跑兩次。

### 11.1 只讀 8 B，記憶體還是搬 64 B

每隔 stride 個 byte 讀一個 8 B 的 long（6 個 thread），兩次：

| stride | 讀幾個 long | 時間（ms） | 有用的 GB/s | 搬的 line 換算成 GB/s（每個 line 算 64 B） |
|---|---|---|---|---|
| 8 B | 2.7 億 | 41.7／42.1 | 51.5／51.0 | 51.5／51.0 |
| 16 B | 1.3 億 | 41.9／42.4 | 25.6／25.3 | 51.2／50.6 |
| 32 B | 6700 萬 | 42.2／43.4 | 12.7／12.4 | 50.9／49.5 |
| **64 B** | 3400 萬 | **42.4／45.2** | **6.3／5.9** | 50.6／47.6 |
| 128 B | 1700 萬 | 30.5／30.6 | 4.4／4.4 | 35.2／35.0 |
| 256 B | 840 萬 | 17.0／17.4 | 3.9／3.9 | 31.5／30.8 |
| 4096 B | 52 萬 | 1.2／1.2 | 3.6／3.6 | 28.7／28.7 |

- **stride 8 到 64 B 花的時間幾乎一樣**（約 42 ms）：只讀每個 64 B 裡的 8 B，跟 64 B 全讀，DRAM 搬的量一樣 → 有用的頻寬掉到 1/8。這就是 [§8](#8-為什麼要-prefetch內部時鐘比-bus-慢) 的「一個命令最少 64 B（64 bit × burst 8）」：DRAM 沒辦法只給你 8 B。
- 64 B 也是本機 cache line 的大小：CPU 本來就以 line 為單位要資料；兩者剛好一樣大不是巧合（課本 p.558：一次傳一個 64 B 的 block）。
- stride 128 B 時**沒有**變成一半的時間（30.5 而不是約 21 ms）：讀一個 line 時，硬體可能也順便拿了旁邊的那個 line（next line prefetching，[第 26 支 §9](lecture26_note.md#9-next-line-prefetching)）。stride 越大，每個 line 的成本越高（4096 B 時只剩 28.7 GB/s 的 line 速率）：每個 line 都在不同的 DRAM row，不能享受 [§3](#3-fast-page-mode-與-edo) 的「row 不關，連續讀」，也沒有 prefetch 的幫忙。

### 11.2 本機的記憶體頻寬

把 2 GB 全部讀一遍（兩次）：

| thread 數 | GB/s |
|---|---|
| 1 | 19.0／19.1 |
| 2 | 34.3／33.7 |
| 3 | 42.5／41.3 |
| 4 | 46.3／46.4 |
| 5 | 48.2／49.8 |
| 6 | **49.0／50.6** |

- 1 個 thread 只有 19 GB/s：單一 core 能同時等的 miss 有限（[第 25 支 §13.1](lecture25_note.md#131-同時能等幾個-miss)），被**延遲**限制，不是被 DRAM 頻寬限制。
- 4 個 thread 以上就飽和在 **約 50 GB/s**。
- 照課本表 10.1：DDR4 最快 4266 MT/s，一個 64 bit 的 channel 最多 4266 × 8 B ≈ **34.1 GB/s**。本機量到 50 GB/s，比一個 DDR4 channel 的理論上限還高 → 本機至少有**兩個 channel**，或者是比 DDR4 更新的記憶體。到底是哪一種、理論頻寬多少，要讀 DIMM 規格（要 root），**待補**。

（[第 41 支 §11](lecture41_note.md#11-實測) 量的是單一存取的延遲；這裡量的是頻寬，並對照 DDR 的傳輸率。）

---

## 12. 一頁速查表

| 主題 | 重點 |
|---|---|
| 非同步 | 沒共用時鐘；strobe（DQS）告訴對方何時鎖存；避開 setup/hold → 否則 metastability |
| RAS／CAS | active low；先送 row 再送 column；位址要比 strobe 早穩定 |
| FPM／EDO | row 不關，連續送 column；EDO 多一個 data strobe |
| BEDO | 送一個 column，內部產生後面的（prefetch length） |
| SDRAM | 共用時鐘；ACT、NOP、CAS 是命令；可程式、可 pipeline 不同 bank；DLL |
| DDR | data bus 快 → 雙邊緣送資料；source synchronous；讀時 strobe 對齊、寫時在 bit 中間 |
| QDR | 讀、寫兩條 bus（full duplex） |
| 世代 | DDR → DDR4：bus 100–200 → 800–2133 MHz；prefetch 2 → 8；2.6 → 1.2 V；1 → 64 GB |
| 算術 | prefetch = 2 × bus 頻率／內部頻率；64 bit × 8 = 最少 64 B；DDR3-1600 = 1600 MT/s |
| Burst | beat、memory word（64 bit，ECC 72 bit）；DDR4 burst 8；DDR3 有 burst chop |
| FB-DIMM | AMB 串成鏈；串列 lane（南 10、北 14，每 lane 每 DRAM cycle 12 bit）；resample／resync／lane steering；沒流行 |
| RDIMM | 位址／命令加 register；完全相容 DDR；伺服器常用；LRDIMM 連 data 也緩衝 |
| 實測 | stride 8–64 B 時間一樣（DRAM 一次就是 64 B）；頻寬飽和在約 50 GB/s，超過一個 DDR4 channel 的上限 |

---

## 13. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Rank、bank、bus 負載 | [第 41 支](lecture41_note.md) |
| DDR4 的狀態圖、時序參數、refresh 命令、memory controller | [第 43 支](lecture43_note.md) |
| Cache line、next line prefetching | [第 21 支](lecture21_note.md)、[第 26 支](lecture26_note.md) |
| 同時能等幾個 miss（MSHR） | [第 25 支](lecture25_note.md) |
| 3D 記憶體、roofline model | [第 45 支](lecture45_note.md) |

---

## 附：本講一句話總結

> **memory controller 和 DRAM 一開始用非同步的方式溝通，沒有共用時鐘只好靠 RAS、CAS 這些 active low 的 strobe 告訴對方何時鎖存，再演進成 row 不關、連續送 column 的 FPM／EDO，以及送一個 column 位址就在內部產生後面幾個的 BEDO（prefetch length）；之後改成共用時鐘的 SDRAM，ACT、CAS 變成命令，再利用 data bus 負載輕、跑得快的特點做出時鐘兩個邊緣都送資料的 DDR，從 DDR 到 DDR4 bus 從 100 MHz 加到 2133 MHz、prefetch length 從 2 加到 8，因為 DRAM 內部的時鐘遠慢於 bus，只能每個內部 cycle 出更多 bit，於是 64 bit 的 channel 一次最少就是 64 byte；bus 越快能接的 DIMM 越少，FB-DIMM 用 AMB 和串列 lane 解決卻因為改太多沒流行，替位址和命令加 register 的 RDIMM 才成了主流。本機實測：每 64 byte 只讀 8 byte 跟全部讀花的時間一樣，DRAM 確實一次就搬 64 byte；6 個 core 一起讀時頻寬飽和在約 50 GB/s，比課本表上單一 DDR4 channel 的上限 34 GB/s 還高。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 10: Main Memory (asynchronous, synchronous and buffered communication) -- Part II 的 whisper 轉錄稿（本機 `large-v3-turbo`，56:15）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch10 §10.2（p.551–562）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4）結果。*
