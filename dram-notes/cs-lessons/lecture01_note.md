# DRAM 系列第一支影片筆記：DRAM cell 與陣列（Part 1: Memory Cell Arrays）

> **課程**：Dynamic Random Access Memory (DRAM) — Part 1: Memory Cell Arrays（播放清單第 1 支）
> **講者**：YouTube 頻道 Computer Science Lessons（旁白，沒有露臉）
> **來源**：YouTube 影片（10:28，whisper 轉錄）＋ 投影片截圖
> **標記說明**：
> - `【口頭】`：旁白講了、投影片上沒寫的內容
> - `【補充】`：影片沒講，從本 repo 其他筆記或 DRAM 通用知識補的
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、Python 3.10）實際執行的結果
> - `⚠️`：影片說法跟實際 DRAM 不一致、或逐字稿不清楚的地方
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 這個系列要講什麼](#1-這個系列要講什麼)
- [2. 電晶體是開關](#2-電晶體是開關)
- [3. 電容與「ground」](#3-電容與ground)
- [4. 一個 cell：寫入 1、讀出來](#4-一個-cell寫入-1讀出來)
- [5. Cell 排成陣列](#5-cell-排成陣列)
- [6. Sense amplifier 與 open bit line](#6-sense-amplifier-與-open-bit-line)
- [7. 讀一整個 row 的步驟](#7-讀一整個-row-的步驟)
- [8. 讀完要寫回](#8-讀完要寫回)
- [9. 漏電與 refresh](#9-漏電與-refresh)
- [10. 總結](#10-總結)
- [11. 【實測】](#11-實測)
- [12. 一頁速查表](#12-一頁速查表)
- [13. 與其他講次的連結](#13-與其他講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
電晶體（開關） + 電容（存電）= 1 個 cell = 1 bit
            │
            ▼
很多 cell 排成陣列：橫的 word line、直的 bit line
            │
            ▼
讀一個 bit = 讀一整個 row
  precharge bit line 到 Vdd/2 → 讓它浮著 → 打開 word line
  → 每條 bit line 變一點點（±ΔV）→ sense amplifier 比較、存起來
            │
            ▼
讀是破壞性的 → sense amplifier 把整個 row 寫回去
電容會漏電   → 每個 cell 約 64 ms 要 refresh 一次 → 所以叫 "Dynamic"
```

---

## 1. 這個系列要講什麼

- [00:00] 整個系列講 DRAM 的基本原理。
- [00:09] 這支：**一個 memory cell** 怎麼存一個 bit（1 或 0），怎麼寫、怎麼讀。
- [00:32] 後面幾支：cell 怎麼組織起來，讓我們一次存取 byte、word，而不是一個 bit。

---

## 2. 電晶體是開關

[00:43] 投影片畫的是電晶體符號，三個接點：**gate、source、drain**。

| 狀態 | 結果 |
|---|---|
| [00:59] 只有 source 有電壓 | 電流**過不去** |
| [01:05] gate 也加電壓 | 電流從 source 流到 drain |
| [01:12] gate 電壓拿掉 | 又不通 |

- [01:15] 所以電晶體就是一個**開關**。
- [01:18] 【口頭】DRAM 晶片裡每個電晶體只有幾奈米寬。

⚠️ 「source 加電壓、電流被擋住」是簡化的說法。實際上 NMOS 的 source／drain 是對稱的，誰是 source 看電流方向；DRAM 讀 0 和讀 1 時電流方向相反（見 [§7](#7-讀一整個-row-的步驟)）。當成「gate 控制的開關」理解就夠了。

---

## 3. 電容與「ground」

- [01:25] 把電晶體的 drain 接到一個**電容**，就是一個 memory cell。
- [01:30] 電容能存電荷。[01:35] 有點像電池，但存的能量少很多，充放電快很多。
- [01:48] 電容單位是法拉（F）。[01:51] **DRAM cell 的電容約 30 fF**（femtofarad，10⁻¹⁵ F）。
- [02:00] 電容另一端接 **ground**。

【口頭】[02:03] ground 先當成 0 V，但其實不一定：

- [02:08] 電壓都是**相對的**，要跟電路裡另一點比。
- [02:17] 「元件上有 3 V」= 一端比另一端高 3 V。[02:24] 是這個**電位差**推動電流、把電荷推上電容。
- [02:32] 所以 ground 是其他電壓量測的**參考點**。這個觀念在 [§7](#7-讀一整個-row-的步驟) 的 precharge 會用到。

---

## 4. 一個 cell：寫入 1、讀出來

這種電晶體叫 **pass transistor**（[03:11]：它「讓電通過」到電容）。

| 動作 | 發生什麼 | 代表 |
|---|---|---|
| [02:48] gate 開、source 有電壓 | 電容充電，[02:54] 只要**幾奈秒** | — |
| [03:04] gate 關 | source 的電被擋住，**電容保持有電** | [03:21] **1** |
| [03:25] gate 開、source **沒**電壓 | 電從電容流出去，直到放光 | [03:43] **0** |

- [03:46] 所以**讀一個 cell 是破壞性的**：要知道電容有沒有電，就得讓它放電。

---

## 5. Cell 排成陣列

- [03:54] DRAM 晶片裡，cell 排成**二維陣列**，用 **word line**（橫）和 **bit line**（直）連起來。
- [04:02] 一個陣列有幾百萬個 cell、幾千條 word line 和 bit line。
- [04:11] 【口頭】用金屬氧化物半導體和矽，經過微影、蝕刻做出來，「但那是另一個故事」。
- [04:23] 投影片上有些電容是紅色（有電 = 1），其他是 0。

```
          bit line  bit line  bit line  bit line
              │         │         │         │
word line ────┼──T──────┼──T──────┼──T──────┼──T────
              │  ╧C     │  ╧C     │  ╧C     │  ╧C
word line ────┼──T──────┼──T──────┼──T──────┼──T────
              │  ╧C     │  ╧C     │  ╧C     │  ╧C
```

每個 cell 的電晶體：gate 接 word line，一端接 bit line，另一端接電容。

---

## 6. Sense amplifier 與 open bit line

- [04:33] 要讀某一個 cell，要靠每條 bit line 尾端的 **sense amplifier**。
- [04:45] 這種接法：**每個 sense amplifier 接兩條 bit line**，[04:51] 理由等一下看。
- [04:54] 放大來看：一排 sense amplifier 把陣列切成上下兩半。[05:02] 這叫 **open bit line** 架構。
- [05:05] 還有別的方式能把一個 sense amplifier 接到兩條 bit line。

【補充】「別的方式」主要是 **folded bit line**：兩條 bit line 在同一側、並排。open 比較省面積，folded 比較抗雜訊。細節見 [Advanced 第 41 支 §6](../../../cs-note/computer-architecture/advanced/lecture41_note.md#6-open-bit-line-與-folded-bit-line)。

---

## 7. 讀一整個 row 的步驟

[05:09] **要知道一個 cell 的值，得讀整個 row。**

| 步驟 | 時間 | 發生什麼 |
|---|---|---|
| ① Precharge | [05:20] | 每條 bit line 充到約**供電電壓的一半**。例：[05:31] 供電 3 V → bit line 1.5 V |
| ② Float | [05:41] | 電壓只加一下就拿掉，bit line 跟電源斷開，像電容一樣暫時保持 1.5 V。[05:52] 叫做讓 bit line **浮著**（float） |
| ③ 打開 word line | [05:56] | word line 拉到高電壓（**asserted**），[06:04] 這個 row 的 pass transistor 全打開 |
| ④ 電荷分享 | [06:07] | cell 有電（1）：電從 cell 流到 bit line，bit line **升高一點**（+δV）<br>[06:24] cell 沒電（0）：電從 bit line 流到 cell，bit line **降低一點**（−δV） |
| ⑤ 偵測 | [07:01] | sense amplifier 拿新的 bit line 電壓跟原來的比 |
| ⑥ 存起來 | [07:23] | sense amplifier 裡有 flip-flop latch，把這個 row 的值暫存起來 |

- [06:51] 變化很小，因為 **bit line 的電容比 cell 大很多**。
- [07:09] 比較的對象：sense amplifier **另一邊**那條 bit line 還是 1.5 V（那邊的 word line 沒開）。[07:16] 所以這種叫 **differential sense amplifier**（差動）。這就是 [§6](#6-sense-amplifier-與-open-bit-line) 說「一定要接兩條」的理由。
- [07:31] latch 住的值之中，有一個可以送到 data bus，下一支影片講。

⚠️ [06:46] 旁白說 δV「大約零點幾 mV（a few tenths of a millivolt）」。照電荷守恆算，bit line 電容要是 cell 的約 3000 倍（約 90 pF）才會這麼小；一般說法是約 100 mV 的量級（見 [§11.1](#111-bit-line-上的電壓到底變多少)）。

⚠️ 3 V 是示意值。現在的 DDR4 是 1.2 V、DDR5 是 1.1 V（【補充】）。

---

## 8. 讀完要寫回

- [07:39] 讀一個 row 是破壞性的：為了知道一個 cell 有沒有電，這個 cell 和同一 row 的其他 cell 都至少放掉了一部分電。
- [07:53] 所以**每次讀之後都要馬上寫回**。[07:59] 這也是 sense amplifier 的工作：
  - [08:02] 依照 latch 住的值，把每條 bit line 推到**滿電壓**（這裡 3 V）或**拉到 ground**。
  - [08:20] 原本的值很快就恢復，然後 word line 關掉。
- [08:29] 【口頭】讀一個 cell 要做這麼多事。[08:35] 就連只改一個 cell，也要先讀整個 row 再寫回。

---

## 9. 漏電與 refresh

[08:42] 有一件事讓上面「整個 row 讀出來再寫回」變得值得：

- [08:49] pass transistor **不是完美的開關**。[08:55] 就算 gate 關著，cell 還是會慢慢漏電到 bit line。
- [09:04] 所以所有 cell 都要**定期 refresh**：[09:09] 讀出來再寫回，就算沒人要這筆資料。
- [09:17] 每個 cell **大約每 64 ms** refresh 一次。

【補充】DDR 標準把 64 ms 分成 8192 次 REF 命令，平均每 7.8 µs 一次（[§11.1](#111-bit-line-上的電壓到底變多少)）。本機量到「每隔約 1.8 µs 有一次特別慢」，可能就是 refresh，見 [Advanced 第 41 支 §11.2](../../../cs-note/computer-architecture/advanced/lecture41_note.md#112-每隔約-18-µs-就有一次特別慢)。

---

## 10. 總結

投影片 [09:30]～[10:00] 的 Summary：

1. DRAM cell 能存**一個 bit**。
2. 一個 cell = **一個電晶體＋一個電容**。
3. **容易縮小**。
4. **便宜**。
5. 很多 cell 接到 word line 和 bit line，排成**長方形陣列**。
6. **破壞性讀取**：讀完一定要寫回。
7. 電荷會漏，所有 cell 都要定期 refresh → 所以叫 **Dynamic** RAM。

[10:08] 下一支：address bus、記憶體位址、怎麼用位址選 row 和 column；也會介紹 memory bank，讓我們存 byte 而不只是 bit。

---

## 11. 【實測】

### 11.1 Bit line 上的電壓到底變多少

`experiments/lecture01/charge_share.py:17`：cell（Cs）和 bit line（Cb）接上後，電荷守恆：

```
Cs·Vcell + Cb·Vpre = (Cs + Cb)·Vfinal
ΔV = (Vcell − Vpre) · Cs / (Cs + Cb)
```

用影片的數字（Vdd = 3 V、Vpre = 1.5 V、Cs = 30 fF），`charge_share.out`：

| Cb／Cs | Cb | 存 1 的 ΔV | 存 0 的 ΔV |
|---|---|---|---|
| 1 | 30 fF | +750 mV | −750 mV |
| 5 | 150 fF | +250 mV | −250 mV |
| **10** | **300 fF** | **+136 mV** | **−136 mV** |
| 20 | 600 fF | +71 mV | −71 mV |
| 100 | 3 pF | +15 mV | −15 mV |

- Cb 越大，ΔV 越小：這就是 [07:01] 旁白說「bit line 電容比 cell 大很多」的後果。
- 要小到 0.5 mV，Cb 要約 **90 pF**（Cs 的 3000 倍）：不合理。所以 [06:46] 的「零點幾 mV」⚠️ 應該是講錯，合理的量級是**幾十到一百多 mV**。
- 正負對稱：存 1 往上、存 0 往下，大小一樣。這是 precharge 到**正中間** Vdd/2 的好處，sense amplifier 只要判斷「比 1.5 V 高還是低」。

### 11.2 Refresh 的節奏

同一個程式（`charge_share.py:28`）：64 ms ÷ 8192 = **每 7.81 µs 一次 REF**。

---

## 12. 一頁速查表

| 觀念 | 重點 |
|---|---|
| Cell | 1 電晶體（pass transistor）＋ 1 電容（約 30 fF）＝ 1 bit |
| 1／0 | 電容有電 = 1，沒電 = 0 |
| 陣列 | word line（橫，接 gate）× bit line（直，接 cell） |
| 讀的單位 | 一次讀**整個 row** |
| Precharge | bit line 充到 Vdd/2，然後讓它浮著 |
| 打開 row | word line asserted → 整個 row 的電晶體打開 |
| 電荷分享 | 1 → bit line +ΔV；0 → −ΔV；ΔV 小因為 Cb ≫ Cs（實際約 100 mV 量級） |
| Differential sense amp | 跟另一邊沒開的 bit line（還是 Vdd/2）比；內含 latch |
| Open bit line | 一排 sense amp 把陣列切兩半，兩邊各一條 bit line |
| 破壞性讀 | 讀完由 sense amp 把 bit line 推到 Vdd 或 0，寫回整個 row |
| Refresh | 漏電 → 每個 cell 約 64 ms 一次；DDR 分成 8192 次 REF，每 7.8 µs 一次 |

---

## 13. 與其他講次的連結

| 本講觀念 | 在哪裡繼續 |
|---|---|
| sense amp latch 住的值怎麼送出去 | [第 2 支](lecture02_note.md)：column 位址、RAS／CAS、讀寫週期 |
| 選哪一條 word line | [第 3 支](lecture03_note.md)：binary decoder |
| 從一排 sense amp 選一個 | [第 4 支](lecture04_note.md)：multiplexer |
| cell 的電晶體層級、SRAM 對照 | [Basic 第 20 支 §7–8](../../../cs-note/computer-architecture/basic/lecture20_note.md#7-dram-單元1-個電晶體--1-個電容) |
| trench／stacked 電容、folded bit line、sense amp 電路 | [Advanced 第 41 支](../../../cs-note/computer-architecture/advanced/lecture41_note.md) |

---

## 附：本講一句話總結

> **一個 DRAM cell 是一個 pass transistor 加一個約 30 fF 的電容，有電是 1、沒電是 0；cell 排成 word line × bit line 的陣列，讀的時候先把 bit line precharge 到 Vdd/2 再讓它浮著，打開一條 word line 後整個 row 的 cell 跟 bit line 分享電荷，bit line 因此高一點或低一點，由跟另一邊 bit line 比較的差動 sense amplifier 偵測並 latch 起來；因為讀是破壞性的，sense amplifier 接著把 bit line 推到 Vdd 或 0 寫回整個 row，而且電容會漏電，所有 cell 每約 64 ms 都要 refresh，這就是「動態」RAM 名字的由來。影片說 ΔV 只有零點幾 mV 是錯的，用影片自己的數字算是約 100 mV 的量級。**

---

*筆記依據 YouTube 頻道 Computer Science Lessons「Dynamic Random Access Memory (DRAM). Part 1: Memory Cell Arrays」（10:28）的 whisper 逐字稿與投影片截圖整理。實測為本機（Intel i7-13650HX、Ubuntu、Python 3.10）結果。*
