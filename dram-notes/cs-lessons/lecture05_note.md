# DRAM 系列第五支影片筆記：DIMM 的組織（Part 5: DIMM Organisation）

> **課程**：Dynamic Random Access Memory (DRAM) — Part 5: DIMM Organisation（播放清單第 5 支）
> **講者**：YouTube 頻道 Computer Science Lessons（旁白，沒有露臉）
> **來源**：YouTube 影片（7:38，whisper 轉錄）＋ 投影片截圖
> **標記說明**：
> - `【口頭】`：旁白講了、投影片上沒寫的內容
> - `【補充】`：影片沒講，從本 repo 其他筆記或 DRAM 通用知識補的
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、Python 3.10）實際執行的結果
> - `⚠️`：影片說法跟實際 DRAM 不一致、或逐字稿不清楚的地方
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 複習](#1-複習)
- [2. 更大的陣列：長方形而不是正方形](#2-更大的陣列長方形而不是正方形)
- [3. 一個 array 一次還是只有 1 bit](#3-一個-array-一次還是只有-1-bit)
- [4. Bank：一次一個 byte](#4-bank一次一個-byte)
- [5. Chip、DIMM、rank](#5-chipdimmrank)
- [6. Channel：single、dual、quad](#6-channelsingledualquad)
- [7. 【實測】](#7-實測)
- [8. 一頁速查表](#8-一頁速查表)
- [9. 與其他講次的連結](#9-與其他講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
array    ：16384 row × 1024 column，一次出 1 bit
  ×8
bank     ：8 個 array 吃同一個 row／column 位址，各接 data bus 的一條線 → 一次 8 bit
  ×8
chip     ：8 個 bank（也有 4、16 個），要 3 bit bank 位址 + bank decoder
  ×8
rank     ：8 顆 chip 同時動，經同一個 channel 存取（DIMM 上的一面）
  ×1–4
DIMM     ：插在主機板的 RAM slot
  ×1–4
channel  ：memory controller 的一條通道；single／dual／quad channel
```

---

## 1. 複習

- [00:00] 前幾支：cell 排成二維陣列；[00:10] 讀的時候 row 位址經 decoder、column 位址經 multiplexer 選出一個 cell；[00:17] 寫的時候 row 一樣 decode，column 改用 demultiplexer。
- [00:24] 這支：把模型**放大**，[00:30] 並介紹 **DIMM**（dual inline memory module）的整體組織。

---

## 2. 更大的陣列：長方形而不是正方形

- [00:37] 首先，陣列可以做得大很多。[00:45] 製程進步，一小片矽上能放幾百萬個元件。
- [00:50] 為了更好利用空間，[00:57] 陣列是**長方形**的，不是正方形。理由：

| | 每多一個要多什麼周邊電路 | 時間 |
|---|---|---|
| 每多一個 **column** | 一個 sense amplifier（帶 latch）、MUX／DEMUX 的一部分、precharge 控制 | [01:05] |
| 每多一個 **row** | decoder 裡的一小部分 | [01:22] |

- [01:29] 想把空間盡量留給 cell，就要減少周邊電路 → [01:36] **row 比 column 多很多**。
- [01:43] 典型的 array：**16384 row × 1024 column**，[01:55] 約 1670 萬個 cell，每個 1 bit。
- [02:04] row 位址 **14 bit**（2¹⁴ = 16384），column 位址 **10 bit**（2¹⁰ = 1024）。

---

## 3. 一個 array 一次還是只有 1 bit

- [02:14] 就算是幾百萬個 cell 的完整 array，[02:23] 一次還是只能讀寫**一個 bit**。
- [02:31] 回到縮小的模型看怎麼解決。

---

## 4. Bank：一次一個 byte

[02:36] 要一次讀寫一個 byte，「辦法很漂亮」：

- [02:42] 用**多個 array，8 個**。每個 array 有自己的周邊：[02:50] 自己的 decoder、sense amp、MUX／DEMUX。
- [02:58] 但每個 array 的 decoder 都餵**同一個 row 位址** → [03:06] row 位址一 strobe，每個 array 都選到**同一條** word line。
- [03:14] 每個 array 的 MUX／DEMUX 也餵**同一個 column 位址** → [03:22] 每個 array 都選到同一個 column。
- [03:27] 也就是說：一個位址送進這組 array，每個 array 選出**一個** cell，**位置都一樣**。
- [03:36] 再接一條 **data bus**：每個 array 接 **8 條資料線中不同的一條**。[03:42] 所以同一個位置可以讀寫 8 個不同的 bit。
- [03:52] 結果：一次讀寫**一整個 byte**。
- [03:59] 這樣一組 array 叫做 **bank**。

```
         同一個 row 位址、同一個 column 位址
          │        │        │             │
       array 0  array 1  array 2  ...  array 7
          │        │        │             │
         D0       D1       D2            D7     ← 8 條資料線
```

投影片 [03:45]：8 個 array 同一個位置的 cell（紅藍相間）同時被選到。

---

## 5. Chip、DIMM、rank

- [04:05] **一顆晶片裡放 8 個 bank**。[04:09] 典型是 4、8 或 16 個。
- [04:16] memory controller 要能指定 bank：8 個 bank → 每個位址裡 **3 bit 是 bank 位址**。[04:28] 用 **bank decoder** 選出一個 bank 去讀寫。
- [04:33] **8 顆晶片**裝在一片電路板上。
- [04:41] 這片板子叫一條 RAM（stick of RAM），也叫記憶體模組，[04:48] 正式名稱 **DIMM**（Dual Inline Memory Module）。
- [04:54] DIMM 插在主機板的 RAM slot，[05:00] slot 經 **memory channel** 接到 memory controller。
- [05:00] 一組**能經同一個 channel 同時存取**的晶片叫 **rank**。

| 名稱 | 意思 | 時間 |
|---|---|---|
| single rank DIMM | 只有一個 rank（常見） | [05:07] |
| dual rank DIMM | 再一組 8 顆，常在板子背面 | [05:22] |
| quad rank DIMM | 四個 rank | [05:14] |

【補充】「8 顆晶片、每顆接 8 條資料線」就是 **x8 晶片**：8 × 8 = 64 bit，剛好是一個 channel 的寬度（[第 6 支 §2](lecture06_note.md#2-memory-channel-的三組線)）。Bank 裡的 8 個 array 就是 x8 晶片那 8 條資料線的來源。用 x4 晶片的 DIMM 一個 rank 要 16 顆。

---

## 6. Channel：single、dual、quad

- [05:34] 一般 PC 主機板有 **4 個 RAM slot**，最多裝 4 條 DIMM。
- [05:45] 想知道有幾個 slot、用了幾個：[05:51] 打開機殼看（「小心，你得知道自己在做什麼」），或 [05:57] 看 Windows **工作管理員**。投影片 [06:03] 的工作管理員：16 GB、Speed 2133 MHz、Slots used 2 of 4（⚠️ 畫面很小，依稀可辨）。

| 模式 | 影片的說法 | 時間 |
|---|---|---|
| quad channel | 4 條 DIMM 各有自己的 channel，可以同時存取，理論上快 4 倍 | [05:57] |
| dual channel | 大部分現代 PC 處理器「只能同時用兩條 DIMM」，快 2 倍 | [06:34] |
| single channel | 所有 slot 共用一個 channel | [06:41] |

- [06:13] 【口頭】對某些應用（例如遊戲）差很多。
- [06:18] 能不能用多個 channel 看 CPU：[06:26] Intel Core i9、AMD Ryzen Threadripper 這類高階處理器支援 quad channel。
- [06:54] single channel 的電腦每個 slot 都插一樣有好處，但速度不會像多 channel 那樣增加。
- [07:00] 用 **CPU-Z** 可以看 channel 模式。投影片 [07:15] 的 CPU-Z：DDR4、16 GB、Channel # Dual、DRAM Frequency 1064.1 MHz、CL 15、tRCD 15、tRP 15、tRAS 36、tRFC 278、CR 2T。

⚠️ 「每條 DIMM 自己一個 channel」不精確。channel 是 **memory controller** 那邊的，一個 channel 可以接 1～2 條 DIMM（[第 6 支 §3](lecture06_note.md#3-channel-模式與一個-channel-能接幾條) 旁白自己也說了）。所以 dual channel 的電腦插 4 條 DIMM 時，是**兩個 channel 各接兩條**，4 條都會用到，不是「只能用兩條」。快 2 倍是因為有 2 個 channel 能同時傳，跟插幾條無關。

【補充】CPU-Z 的 DRAM Frequency 1064 MHz 是 clock；DDR 每個 clock 傳兩次，所以是 2128 MT/s，工作管理員寫成「2133 MHz」其實是 MT/s（[第 6 支 §4](lecture06_note.md#4-ddr一個-clock-傳兩次)）。

- [07:08] 下一支：影響 DRAM 效能的因素，設計者在容量、速度、能耗、成本之間的取捨，還有 DDR 的世代。

---

## 7. 【實測】

### 7.1 容量算術

`experiments/lecture06/burst.py`（第 6 支會用到的同一個程式）用影片的數字一層層乘上去（`burst.out`）：

| 層級 | 怎麼算 | 結果 |
|---|---|---|
| array | 16384 × 1024 | 16 Mib |
| bank | × 8 array | 128 Mib |
| chip | × 8 bank | **1 Gib** |
| rank（8 顆） | × 8 chip | **1 GiB** |

- 用位址算一次：row 14 + column 10 + bank 3 = **27 bit**，每個位址拿 8 B（8 顆 × 8 bit），2²⁷ × 8 B = 1 GiB，一樣。
- 影片模型的晶片是 1 Gib。投影片上 CPU-Z 的 16 GB 要 16 個這種 rank：【補充】實際的 DDR4 晶片是 4、8、16 Gib，靠的是更多 row（row 位址比 14 bit 多）和更多 bank（x8 的 DDR4 有 16 個 bank，分成 4 個 bank group）。

### 7.2 本機

本機（i7-13650HX）記憶體類型、DIMM 數、rank 數要 `dmidecode`（要 root），**待補**。本機 64 B 的 line 要搬 64 B、雙 channel 的實際頻寬等，已在 [Advanced 第 42 支 §11](../../../cs-note/computer-architecture/advanced/lecture42_note.md#11-實測) 量過。

【補充】對照：Genio 720 EVK 板子上是 LPDDR5 8 GiB，**2 channel × 2 rank**（[`../dram-brief.md`](../dram-brief.md)）。手機／嵌入式用的 LPDDR 是直接焊在板上的晶片，沒有 DIMM，但 channel、rank、bank 的觀念一樣。

---

## 8. 一頁速查表

| 觀念 | 重點 |
|---|---|
| 長方形 array | column 的周邊電路多、row 的少 → row 多；典型 16384 × 1024 |
| 位址 | 14 bit row + 10 bit column |
| Bank | 8 個 array 吃同一個位址，各接 1 條資料線 → 一次 8 bit |
| Chip | 4／8／16 個 bank；8 個要 3 bit bank 位址 + bank decoder |
| Rank | 一組經同一個 channel、同時動的晶片（8 顆 x8 = 64 bit） |
| DIMM | 一片插在 slot 的板子；single／dual／quad rank |
| Channel | memory controller 的通道；dual channel 理論 2 倍頻寬 |
| ⚠️ | channel 數跟 DIMM 數不同；一個 channel 可接 1～2 條 |
| 看設定 | Windows：工作管理員、CPU-Z；Linux：`dmidecode -t memory`（root） |

---

## 9. 與其他講次的連結

| 本講觀念 | 在哪裡繼續 |
|---|---|
| channel 的線、DDR、一個 channel 接幾條 | [第 6 支](lecture06_note.md) |
| bank 之間怎麼輪流 | [第 6 支 §7](lecture06_note.md#7-bank-interleaving)、[第 7 支](lecture07_note.md) |
| channel／DIMM／rank／chip／bank／array 階層 | [Advanced 第 41 支 §9](../../../cs-note/computer-architecture/advanced/lecture41_note.md#9-系統channeldimmrankchipbankarray) |
| 本機量到的 row conflict | [Advanced 第 41 支 §11.1](../../../cs-note/computer-architecture/advanced/lecture41_note.md#111-row-buffer同一個-bank-的不同-row-互相干擾) |

---

## 附：本講一句話總結

> **一個 DRAM array 做成 row 比 column 多的長方形（典型 16384 × 1024，因為每個 column 要 sense amp 和 MUX，每個 row 只要一小塊 decoder），但它一次只出 1 bit；把 8 個 array 接同一個 row／column 位址、各接一條資料線就成了一次出 8 bit 的 bank，一顆晶片放 4～16 個 bank（用 3 bit bank 位址選），8 顆晶片同時動組成 64 bit 寬的 rank，裝在 DIMM 上經 memory channel 接到 CPU 裡的 memory controller。多個 channel 能同時傳，所以 dual channel 理論上頻寬兩倍；但 channel 是 controller 那邊的，一個 channel 可以接兩條 DIMM，影片說「dual channel 只能用兩條」不對。**

---

*筆記依據 YouTube 頻道 Computer Science Lessons「Dynamic Random Access Memory (DRAM). Part 5: DIMM Organisation」（7:38）的 whisper 逐字稿與投影片截圖整理。實測為本機（Intel i7-13650HX、Ubuntu、Python 3.10）結果。*
