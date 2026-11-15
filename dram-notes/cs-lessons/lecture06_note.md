# DRAM 系列第六支影片筆記：Burst mode 與 bank interleaving（Part 6: Burst Mode and Bank Interleaving）

> **課程**：Dynamic Random Access Memory (DRAM) — Part 6: Burst Mode and Bank Interleaving（播放清單第 6 支）
> **講者**：YouTube 頻道 Computer Science Lessons（旁白，沒有露臉）
> **來源**：YouTube 影片（12:10，whisper 轉錄）＋ 投影片截圖
> **標記說明**：
> - `【口頭】`：旁白講了、投影片上沒寫的內容
> - `【補充】`：影片沒講，從本 repo 其他筆記或 DRAM 通用知識補的
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、Python 3.10）實際執行的結果
> - `⚠️`：影片說法跟實際 DRAM 不一致、或逐字稿不清楚的地方
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 複習：DIMM 的階層](#1-複習dimm-的階層)
- [2. Memory channel 的三組線](#2-memory-channel-的三組線)
- [3. Channel 模式與一個 channel 能接幾條](#3-channel-模式與一個-channel-能接幾條)
- [4. DDR：一個 clock 傳兩次](#4-ddr一個-clock-傳兩次)
- [5. 每顆晶片接 8 條線：晶片數的取捨](#5-每顆晶片接-8-條線晶片數的取捨)
- [6. Burst：row 開著就連續讀](#6-burstrow-開著就連續讀)
- [7. Bank interleaving](#7-bank-interleaving)
- [8. Bank 數的取捨](#8-bank-數的取捨)
- [9. 【實測】](#9-實測)
- [10. 一頁速查表](#10-一頁速查表)
- [11. 與其他講次的連結](#11-與其他講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
問題：bus 每 0.47 ns 就能送一個 64 bit word，但讀一個 cell 要好幾個 clock

解法 1：burst
  開一次 row（慢）→ row 開著，連續送好幾個 column（快）
  一個 burst = 8 × 64 bit = 64 B = 一個 cache line

解法 2：bank interleaving
  一個 bank 送完 burst 要關 row、precharge（可能 18 個 clock）
  → 換下一個 bank 接著送，前一個趁機回復
  → bus 一直滿

取捨：bank 多 = 平行度高、快；但周邊電路多 → 容量小
      bank 少 = 容量大；但 array 大、bit line 長 → 慢、耗電
```

---

## 1. 複習：DIMM 的階層

- [00:00] [00:12] 一般主機板 4 個 DIMM slot。[00:18] 一般 DIMM 一面有一個 rank、8 顆晶片；[00:24] 有的有第二個 rank，常在背面。
- [00:31] 晶片裡有多個 bank，bank 裡有多個 array，[00:38] array 是 row × column。
- [00:42] 這支：DIMM 的階層怎麼運作，[00:48] 以及設計者在**容量、速度、能耗、成本**之間的取捨。

---

## 2. Memory channel 的三組線

- [00:59] DIMM 插在主機板 slot，[01:04] 經 **memory channel** 接到 memory controller。[01:13] 現代 PC 的 memory controller 跟 CPU 在**同一顆晶片**裡。
- [01:20] channel 包含三組線，[01:27] 畫在主機板上：

| 線 | 寬度 | 說明 | 時間 |
|---|---|---|---|
| address bus | **17 條** | row、column 位址分開送，所以位址可以比 17 bit 長 | [01:29] |
| command／control bus | **6 條** | | [01:42] |
| data bus | **64 條** | 有 ECC（錯誤更正）時是 **72 條** | [01:42]、[01:51] |

- [01:58] 早期 PC 的 data bus 只有 32 條，[02:03] 那時的模組只有 32 bit 的連接，[02:08] 叫 **SIMM**（single inline memory module）。[02:14] 那時資料 32 bit 一次搬，[02:19] 32 bit = 4 byte 叫一個 word。
- [02:24] 現代 PC 的 data bus 64 bit，[02:29] 模組的資料連接是 SIMM 的兩倍，[02:35] 所以叫 **DIMM**。[02:42] 現代 PC 的 word 是 64 bit。

⚠️ 【補充】「dual」不是指「64 是 32 的兩倍」。SIMM 板子兩面的金手指是**同一個訊號**（兩面接通），DIMM 兩面的金手指是**不同訊號**，所以叫 dual inline。剛好 DIMM 出現的時候 bus 也變成 64 bit，兩件事同時發生。

---

## 3. Channel 模式與一個 channel 能接幾條

| 模式 | 說明 | 時間 |
|---|---|---|
| single channel | 例：兩條 DIMM 共用同一個 channel | [03:09] |
| dual channel | 兩條 DIMM 各有自己的 channel，memory controller 可以**同時**存取，理論上 2 倍快 | [03:19] |
| quad channel | 四條各有自己的 channel；高階電競 PC 才有 | [03:36] |

- [03:47] 能不能多 channel，看主機板設計和 CPU。[03:56] 大部分現代 PC 是 dual channel。
- [04:00] **一個 channel 能接幾條 DIMM 有電氣上的限制**：[04:07] 訊號分給多條 DIMM 會變弱，[04:10] 通常一個 channel **最多兩條**。
- [04:14] 一條 DIMM 有兩個 rank 時，[04:18] 兩個 rank 一定共用同一個 channel（同一個 slot）。
- [04:23] dual／quad rank 的 DIMM 容量可能比較大，[04:27] 但比較耗電，速度幾乎沒有多。

這一節旁白自己說了「一個 channel 可以接兩條」，跟[第 5 支 §6](lecture05_note.md#6-channelsingledualquad) 的 ⚠️ 對得上：dual channel 插 4 條 = 2 個 channel × 2 條。

---

## 4. DDR：一個 clock 傳兩次

- [02:47] channel 的「有效頻率」一般是 **2132 MHz**。[02:54] 也就是每秒送 21.32 億個 64 bit word，[03:03] 一個 word 不到半奈秒。
- [04:33] 但存取一個 cell 相對很慢，[04:38] 讀寫一個 bit 要好幾個 clock。
- [04:43] data bus 每個 clock 送**兩個** 64 bit word：[04:51] clock 的**上升緣**一次、**下降緣**一次（投影片 [04:48]）。[04:54] 這就是 **DDR**（double data rate）名字的由來。
- [04:59] 所以模組的設計目標：**讓 data bus 盡量一直有資料在跑**。

【補充】「2132 MHz」嚴格說是 2132 **MT/s**（每秒傳輸次數），clock 本身是 1066 MHz。第 5 支投影片上 CPU-Z 的 DRAM Frequency 1064 MHz 就是這個 clock。

---

## 5. 每顆晶片接 8 條線：晶片數的取捨

- [05:08] 一般 DIMM 每顆晶片接 data bus 的 **8 條線**。[05:14] 所以一個 64 bit word，8 顆晶片各讀寫 8 bit，同時進行。
- [05:22] 8 顆是典型，有的多有的少。

| | 好處 | 壞處 | 時間 |
|---|---|---|---|
| 晶片**多** | 每顆容量固定時，總容量大 | 每次讀寫所有晶片一起動 → **耗電** | [05:27] |
| 晶片**少** | 省電 | 每顆要負責更多條資料線 → 每顆腳多，[05:49] **腳很貴** | [05:41] |

- [05:55] 每顆晶片裡一般 **8 個 bank**，⚠️ 旁白說「每個 bank 可以存 8 bit」，應該是指**每次存取出 8 bit**（8 個 array 各 1 bit，[第 5 支 §4](lecture05_note.md#4-bank一次一個-byte)）。
- [06:02] 各晶片**相同位置**的 bank 共用位址線和控制訊號，[06:09] 所以 8 顆晶片同時寫 8 bit 時，寫進的是同一個位置的 bank。
- [06:16] 但一次讀寫 64 bit **並不會比讀寫 1 bit 快**。[06:25] 要變快，得看一個 bank 裡面。

---

## 6. Burst：row 開著就連續讀

- [06:30] 模型：一個 bank 有 8 個 array，每個 8 × 8。[06:44] 真的 array 是 1024 column × 16384 row。
- [06:53] 讀資料：先 strobe row 位址（開始讀週期），[06:58] 再 strobe column 位址。[07:00] 兩者在每個 array 選到同一個位置的 cell。
- [07:11] row 被打開、latch 進 sense amp。[07:16] **這是讀週期比較慢的部分**。[07:19] 這時候 row 是 **open** 的。
- [07:22] 每個 array 出一個 bit，一起載進 output buffer。[07:32] 8 顆晶片裡同位置的 bank 同時做一樣的事，[07:39] 合起來送一個 64 bit word。
- [07:44] 早期 DRAM 讀完就關掉 row，需要的話再開別的或重開這個。
- [07:55] 但開一整個 row（可能 1024 甚至 2048 個 column 寬）只讀一個 bit，太浪費。
- [08:07] 所以 **row 還開著時，再讀下一個、再下一個**（投影片 [07:27]～[08:15]：同一個 row 的 column 一格一格往右，output buffer 一列一列填滿）。
- [08:16] 只給**一個 row 位址、一個 column 位址**，就能很快讀出一串資料：**burst**。
- [08:23] row 開著時讀幾個 column，叫 **burst length**。

| 世代 | 影片說的 burst length | 實際（【補充】） |
|---|---|---|
| DDR2 | 4 | 4（或 8） |
| DDR3 | 8 | 8 |
| DDR4 | **16** ⚠️ | **8**（另有 burst chop 4） |
| DDR5 | （沒講） | 16，但每條 DIMM 拆成兩個 32 bit sub-channel |

- [08:45] 別忘了這只是一個 bank。[08:49] 真的 DDR3 模組 8 顆晶片裡各一個 bank 一起動，連續送出 **8 個 64 bit word**。[09:01] 這個技術叫 **prefetching**。

【補充】DDR 的 prefetch 更精確的意思是：晶片內部一次從 array 抓 n 倍的資料（DDR3 是 8n），再用快 n 倍的速度從腳送出去；內部 array 的速度沒有變快，是靠一次抓多一點來配合外面的 bus。見 [Advanced 第 42 支 §8](../../../cs-note/computer-architecture/advanced/lecture42_note.md#8-為什麼要-prefetch內部時鐘比-bus-慢)。

---

## 7. Bank interleaving

- [09:05] 還是有問題：[09:07] 一個 bank **一次只能開一個 row**。[09:11] burst 一結束，要先關掉 row 才能開別的：[09:16] 寫回剛讀的 row、再 precharge bit line。[09:22] 可能要 **18 個 clock**。
- [09:25] CPU 和 DRAM 之間的資料以 **cache line** 為單位，[09:33] 一般 **64 B**，[09:37] = 8 個 64 bit word（投影片 [09:30]「Cache Line 64 Bytes」）。
- [09:40] 一個 burst 可能剛好以 bus 的全速填滿一個 cache line。[09:46] 但如果要**連續讀好幾個** cache line 呢？
- [09:51] 為了提高吞吐量，晶片裡每個 bank **各自獨立運作**。[10:01] 不同 bank 可以在讀寫週期的不同階段。
- [10:06] **Bank interleaving**：[10:14] 一個 bank 送完 burst，下一個 bank 立刻接著送，再下一個……
- [10:24] 一個 bank 在送資料時，其他 bank 在準備。[10:29] 等最後一個 bank 送完，第一個早就回復好了。
- [10:37] 所以 bank interleaving 讓 data bus 能跑很快，最重要的是在長時間讀寫時**保持滿載**。

【補充】18 個 clock 的量級可以用第 5 支投影片的 CPU-Z 數字對：tRP（precharge）15 clock，在 1064 MHz 約 14 ns。本機量到「同一個 bank 換 row」比一般慢 15–50 ns，見 [Advanced 第 41 支 §11.1](../../../cs-note/computer-architecture/advanced/lecture41_note.md#111-row-buffer同一個-bank-的不同-row-互相干擾)。

---

## 8. Bank 數的取捨

- [10:50] 這顆有 8 個 bank，有的多有的少。[10:56] bank 數是 DRAM 設計的重要決定。

| | 好處 | 壞處 | 時間 |
|---|---|---|---|
| bank **多** | 平行度高、延遲低，[11:03] 比較能讓 data bus 一直忙 → **快** | 同面積下每個 bank 小，[11:24] array 小而多，每個 array 都有自己的 decoder、sense amp、MUX → 周邊電路多 → [11:46] **容量小** | [11:03]～[11:46] |
| bank **少** | bank 大、array 大 → [11:56] **容量可能比較大** | [11:59] array 大 = **bit line 長** → 慢、耗電 | [11:50]～[11:59] |

投影片 [12:03] 的四句：More banks = more parallelism／More banks = lower capacity／Fewer banks = higher capacity／Fewer banks = slower speed。

---

## 9. 【實測】

### 9.1 Burst、channel、cache line 的算術

`experiments/lecture06/burst.py`（`burst.out`）：

```
每顆晶片的資料線 = 64 / 8 = 8（x8 晶片）
2132 MT/s：clock = 1066 MHz，每拍 0.469 ns，一個 channel 峰值 17.06 GB/s
  2 channel：  34.11 GB/s
  4 channel：  68.22 GB/s
DDR3: BL8  x 64 bit sub-channel =  64 B -> 剛好一個 64 B cache line
DDR4: BL8  x 64 bit sub-channel =  64 B -> 剛好一個 64 B cache line
DDR5: BL16 x 32 bit sub-channel =  64 B -> 剛好一個 64 B cache line
本機 getconf LEVEL1_DCACHE_LINESIZE = 64
```

- 每拍 0.469 ns：就是 [03:03]「一個 word 不到半奈秒」。
- 不管哪一代，**一個 burst 都設計成剛好一個 64 B cache line**。DDR5 把 burst 拉長到 16，代價是把 64 bit 拆成兩個 32 bit sub-channel，結果還是 64 B。影片說 DDR4 是 BL16 ⚠️，如果真的是 64 bit × 16 = 128 B，就是兩個 cache line，不合理。
- 本機 cache line 也是 64 B。

### 9.2 Interleaving 能讓 bus 多忙

用[第 7 支](lecture07_note.md#7-實測)的模擬：每個 burst「開 row 3、送資料 1、回復 3」個時間單位，bus 一次一個 burst：

| 輪流的 bank 數 | bus 忙碌 |
|---|---|
| 1 | 14% |
| 2 | 29% |
| 4 | 57% |
| **8** | **98%** |
| 16 | 98% |

- 一個 bank 要 1 + 3 + 3 = 7 單位才能再送下一個 burst，bus 只用到 1/7。bank 數到 7 以上就能填滿：這就是 [10:29]「最後一個 bank 送完，第一個已經回復好了」。

### 9.3 本機已量過的

- 只讀 8 B，DRAM 還是搬 64 B：[Advanced 第 42 支 §11.1](../../../cs-note/computer-architecture/advanced/lecture42_note.md#111-只讀-8-b記憶體還是搬-64-b)。
- 本機 1～6 thread 的頻寬（約 19～50 GB/s）：[Advanced 第 42 支 §11.2](../../../cs-note/computer-architecture/advanced/lecture42_note.md#112-本機的記憶體頻寬)。

---

## 10. 一頁速查表

| 觀念 | 重點 |
|---|---|
| Channel 的線 | 位址 17、命令／控制 6、資料 64（ECC 72） |
| SIMM → DIMM | 32 bit → 64 bit；⚠️「dual」其實是兩面金手指訊號不同 |
| 一個 channel | 最多約 2 條 DIMM；dual rank 的兩個 rank 共用 channel |
| DDR | 上升緣、下降緣各傳一次；「2132 MHz」= 2132 MT/s，clock 1066 MHz |
| x8 晶片 | 8 顆 × 8 條 = 64 bit；晶片多耗電、晶片少腳多（貴） |
| Burst | row 開一次，連續讀 burst length 個 column |
| Burst length | DDR2 4、DDR3 8、DDR4 8（⚠️ 影片說 16）、DDR5 16 |
| 一個 burst | 8 × 64 bit = 64 B = 一個 cache line |
| 關 row | 寫回 + precharge，可能 18 個 clock |
| Bank interleaving | 一個 bank 回復時下一個送；bus 保持滿載 |
| Bank 多 vs 少 | 多：快、容量小；少：容量大、bit line 長 → 慢、耗電 |

---

## 11. 與其他講次的連結

| 本講觀念 | 在哪裡繼續 |
|---|---|
| 位址 bit 怎麼安排才會輪流到不同 bank | [第 7 支](lecture07_note.md) |
| 非同步 → 同步 → DDR、prefetch length | [Advanced 第 42 支](../../../cs-note/computer-architecture/advanced/lecture42_note.md) |
| ACT／PRE／READ 的時序、tRRD、tFAW | [Advanced 第 43 支](../../../cs-note/computer-architecture/advanced/lecture43_note.md) |
| 第 2 支的「一次讀 1 bit」 | [第 2 支](lecture02_note.md) |

---

## 附：本講一句話總結

> **memory channel 有 17 條位址線、6 條命令線、64 條資料線（ECC 72），DDR 在 clock 的上升緣和下降緣各送一次，所以 2132 MT/s 的 bus 每 0.47 ns 就能送一個 64 bit word；一般 DIMM 用 8 顆 x8 晶片湊滿 64 bit，但讀一個 cell 要好幾個 clock，所以靠兩招讓 bus 一直有資料：一是 burst，row 開一次後連續送 burst length 個 column（一個 burst 剛好是 64 B 的 cache line），二是 bank interleaving，一個 bank 關 row、precharge 的時候換下一個 bank 送。bank 多比較快但周邊電路多、容量小；bank 少容量大但 bit line 長、慢又耗電。影片說 DDR4 的 burst length 是 16 不對，DDR4 是 8、DDR5 才是 16。**

---

*筆記依據 YouTube 頻道 Computer Science Lessons「Dynamic Random Access Memory (DRAM). Part 6: Burst Mode and Bank Interleaving」（12:10）的 whisper 逐字稿與投影片截圖整理。實測為本機（Intel i7-13650HX、Ubuntu、Python 3.10）結果。*
