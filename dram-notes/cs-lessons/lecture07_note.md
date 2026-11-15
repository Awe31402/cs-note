# DRAM 系列第七支影片筆記：Memory address mapping（Part 7: Memory Address Mapping）

> **課程**：Dynamic Random Access Memory (DRAM) — Part 7: DRAM Memory Address Mapping（播放清單第 7 支）
> **講者**：YouTube 頻道 Computer Science Lessons（旁白，沒有露臉）
> **來源**：YouTube 影片（5:51，whisper 轉錄）＋ 投影片截圖
> **標記說明**：
> - `【口頭】`：旁白講了、投影片上沒寫的內容
> - `【補充】`：影片沒講，從本 repo 其他筆記或 DRAM 通用知識補的
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、Python 3.10）實際執行的結果
> - `⚠️`：影片說法跟實際 DRAM 不一致、或逐字稿不清楚的地方
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 複習：bank interleaving 的時間軸](#1-複習bank-interleaving-的時間軸)
- [2. 從數數開始](#2-從數數開始)
- [3. 模型：一個 bank](#3-模型一個-bank)
- [4. 9 bit 位址怎麼切](#4-9-bit-位址怎麼切)
- [5. 一步一步跑](#5-一步一步跑)
- [6. 推廣](#6-推廣)
- [7. 【實測】](#7-實測)
- [8. 一頁速查表](#8-一頁速查表)
- [9. 與其他講次的連結](#9-與其他講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
9 bit 位址（高位 → 低位）：

  ┌──────┬────────┬──────┬────────┐
  │ row  │ col 高 │ bank │ col 低 │
  │ 3bit │  1bit  │ 3bit │  2bit  │
  └──────┴────────┴──────┴────────┘
     │       │        │       └─ burst 裡的 4 個 column（DIMM 自己數）
     │       │        └───────── 每 4 個位址換一個 bank → interleaving
     │       └────────────────── 8 個 bank 輪完一圈，換到 column 4–7
     └────────────────────────── 再輪完一圈，才換下一個 row

連續的位址：bank0 col0–3 → bank1 col0–3 → … → bank7 → bank0 col4–7 → … → row 1
```

---

## 1. 複習：bank interleaving 的時間軸

- [00:00] 上一支：bank interleaving 在 DIMM 的每顆晶片裡發生，降低延遲，[00:10] 再配合 prefetching 大大提高速度。
- [00:17] 投影片 [00:24]～[00:48] 的時間軸：

```
Bank 0: [Initiate read][Data][  Bank 0 unavailable  ]
Bank 1:        [Initiate read][Data][  Bank 1 unavailable  ]
Bank 2:               [Initiate read][Data][ ... ]
  ...
Bank 7:                                   [Initiate read][Data][ ... ]
Bank 0:                                          [Initiate read][Data] ...
──────────────────────────────────────────────────────────────▶ Time
```

- [00:24] 讀一個 bank 要先等一段時間才有資料。[00:31] burst 結束後 bank 要**回復**才能再送。
- [00:39] 但可以接著在下一個 bank 開始 burst，不用等 → 經過一開始的延遲後，資料**連續**流出。
- [00:46] 最後一個 bank 送完時，第一個 bank 已經準備好了。
- [00:52] 這支：**怎麼用位址 bit 對應到 row、column、bank 的方式**來控制 interleaving。

---

## 2. 從數數開始

- [00:58] 先數數：[01:07] 投影片 [01:06]～[01:27] 是一串二進位數，就是 0、1、2……
- [01:12] 旁邊標上十進位。[01:14] 這些就是要送進模型的位址。
- [01:21] 用 **9 bit** 可以數到 511，[01:27] 但這裡數到 71 就夠說明了。

---

## 3. 模型：一個 bank

- [01:30] 投影片上是 8 顆晶片其中一顆的 8 個 bank 之一，[01:35] 第一個，叫 **bank 0**。
- [01:42] 這個 bank 的每個 array 有 8 row（0–7）、8 column（0–7）。
- 一個 bank 有 8 個 array（[第 5 支 §4](lecture05_note.md#4-bank一次一個-byte)），同一個位址每個 array 出 1 bit → **每個位址 = 1 byte**。

---

## 4. 9 bit 位址怎麼切

[01:50] 要讓 **4 byte 的 burst** 輪流分到 8 個 bank，位址 bit 這樣分配（投影片 [02:09]～[02:12] 用顏色標）：

| 位置 | bit 數 | 用途 | 時間 |
|---|---|---|---|
| 最高 3 bit | 3 | **row** 位址（橘色） | [01:57] |
| 下一個 bit | 1 | **column 位址的第一部分**（高位，綠色） | [02:06] |
| 再下 3 bit | 3 | **bank**（藍色） | [02:09] |
| 最低 2 bit | 2 | **column 位址的其餘部分**（低位，綠色） | [02:14] |

- [02:21] 把 column 位址拆成兩段「看起來很怪」，[02:27] 但這個小技巧就是 bank interleaving 的核心。

---

## 5. 一步一步跑

| 時間 | 位址（十進位） | row | col | bank | 說明 |
|---|---|---|---|---|---|
| [02:35] | 0 | 0 | 0 | 0 | burst 的起點 |
| [02:43] | 0–3 | 0 | 0–3 | 0 | column 0–3 連續讀出。[02:51] 8 個 array 同一個位置 → 4 byte 的 burst |
| [03:25] | 4 | 0 | 0 | **1** | [03:31] row、column 跟剛才一樣，**bank 變了** → bank 1 送 4 byte |
| [03:39] | 8 | 0 | 0 | 2 | bank 2 |
| [03:49]～[04:02] | 12、16、… | 0 | 0 | 3、4、… 7 | bank 3、bank 4…… |
| [04:10] | 32 | 0 | **4** | 0 | 回到 bank 0：[04:16] column 位址的**最高位**變成 1 → column 100 = 4，從 row 的另一個位置開始 |
| [04:34] | 36、40、… | 0 | 4 | 1、2、… | 接著還是一個 bank 接一個 bank |
| [04:42] | 64 | **1** | 0 | 0 | 又輪回 bank 0：[05:03] row 的最低位從 0 變 1、column 最高位回到 0 → [05:10] row 1、column 0 |

- [02:57] DIMM 設定成**自動做 burst**，不用每個 column 都給位址。[03:04] 但每個 column 還是有自己的位址。
- [03:11] burst 裡只有**最低 2 bit** 在變：[03:17] column 位址從 0 加到 3，配合 burst length。
- [03:25] 下一個送給 DIMM 的位址比上一個**大 4**。

【口頭】[04:10]「這個安排真正漂亮的地方」：連續位址先**橫著**輪過 8 個 bank，再往 row 裡面走，最後才換 row。同一個 row 被讀完之前不會換 row，而相鄰的 burst 永遠在不同 bank。

---

## 6. 推廣

- [05:17] 這只是模型，但同樣原理用在大很多的長方形 array 上。
- [05:25] DIMM 的 bank 可多可少，burst 可長可短。
- [05:33] 只要**每顆晶片的 bank 數和 burst length 都是 2 的次方**，[05:39] 控制資料怎麼在 bank 之間交錯，[05:46] 就只是**決定位址 bit 怎麼分配**的問題。

【補充】這個切法是 memory controller 決定的（CPU 送的是實體位址，controller 自己切成 channel／rank／bank／row／column），影片沒有說是誰負責。真的 controller 還要決定 channel、rank、bank group 放哪幾個 bit，Intel 等硬體還會把較高的 bit XOR 進 bank／channel 位址，避免「兩串資料剛好差 2 的次方」時永遠撞同一個 bank。見 [Advanced 第 43 支 §13](../../../cs-note/computer-architecture/advanced/lecture43_note.md#13-address-mapping)。

⚠️ 影片的模型假設**每個 burst 之後 bank 都要關 row、回復**（close page）。如果 controller 讓 row 開著（open page），連續位址都在同一個 row 時一直是 row hit，不用換 bank 也很快；這時候 interleaving 的好處主要在「好幾串不同的存取同時進來」。Advanced 第 43 支的模擬比較了 open／close page 和不同切法（[§16.1](../../../cs-note/computer-architecture/advanced/lecture43_note.md#161-模擬-address-mapping-與-openclose-page)）。

---

## 7. 【實測】

`experiments/lecture07/mapping.py` 照影片的切法寫 `split_video()`（`mapping.py:9`），再寫一個 bank 放最高位的對照組 `split_naive()`（`mapping.py:17`）。

### 7.1 重現投影片的對照表

`mapping.out`（每個 burst 只列第一個位址）：

```
 addr  bits(r|c|bbb|cc)  row bank col
    0  000|0|000|00       0    0   0
    4  000|0|001|00       0    1   0
    8  000|0|010|00       0    2   0
  ...
   28  000|0|111|00       0    7   0
   32  000|1|000|00       0    0   4
  ...
   60  000|1|111|00       0    7   4
   64  001|0|000|00       1    0   0
```

- 跟投影片 [03:21]～[05:18] 一樣：每 4 個位址換 bank；位址 32 回到 bank 0 的 column 4；位址 64 才換到 row 1。
- 512 個位址對到 512 個不同的 (row, bank, column)：切法只是把 bit **重新排列**，不會撞（`mapping.py:34`）。

### 7.2 切法對速度的影響

`simulate()`（`mapping.py:37`）照第 6、7 支的模型：每個 burst「開 row 3、送資料 1、回復 3」個時間單位，data bus 一次一個 burst，同一個 bank 回復完才能再開。連續讀 128 個 burst（512 byte，整個模型讀一遍）：

| 切法 | 花的時間 | bus 忙碌 |
|---|---|---|
| 影片的切法（bank 在中間） | 131 | **98%** |
| bank 放最高位 | 851 | **15%** |

- bank 放最高位時，前 64 個位址全在 bank 0，每個 burst 都要等 bank 0 回復（7 個單位才送 1 個）。
- 影片的切法讓相鄰的 burst 落在不同 bank，回復時間被其他 bank 的傳輸蓋掉。
- 輪流的 bank 數對 bus 忙碌度的影響見 [第 6 支 §9.2](lecture06_note.md#92-interleaving-能讓-bus-多忙)：8 個 bank 才夠蓋掉 7 單位的週期。

---

## 8. 一頁速查表

| 觀念 | 重點 |
|---|---|
| 目的 | 讓連續的 burst 落在不同 bank → bank interleaving |
| 模型 | 8 bank，每個 bank 8 個 8×8 array；每個位址 1 byte；burst 4 byte |
| 切法 | row[3] \| col 高[1] \| bank[3] \| col 低[2] |
| col 低 2 bit | burst 裡 DIMM 自己數的 4 個 column |
| bank 3 bit | 每 4 個位址換一個 bank |
| col 高 1 bit | 8 個 bank 輪完，換到 column 4–7 |
| row 3 bit | 每 64 個位址換一個 row |
| 條件 | bank 數、burst length 是 2 的次方 → 只是分配 bit 的問題 |
| 誰決定 | memory controller（影片沒講） |
| ⚠️ | 影片假設每個 burst 後都關 row；open page 時同 row 連續讀本來就快 |

---

## 9. 與其他講次的連結

| 本講觀念 | 在哪裡繼續 |
|---|---|
| bank、burst、interleaving 的由來 | [第 6 支](lecture06_note.md) |
| row／column 位址分兩次送 | [第 2 支](lecture02_note.md#4-位址多工省腳) |
| 真的 controller 的 address mapping、XOR hash | [Advanced 第 43 支 §13](../../../cs-note/computer-architecture/advanced/lecture43_note.md#13-address-mapping) |
| open／close page 與不同切法的模擬 | [Advanced 第 43 支 §16.1](../../../cs-note/computer-architecture/advanced/lecture43_note.md#161-模擬-address-mapping-與-openclose-page) |

---

## 附：本講一句話總結

> **bank interleaving 要靠 memory address mapping 實現：影片的 9 bit 位址從高到低切成 row 3 bit、column 高 1 bit、bank 3 bit、column 低 2 bit，最低 2 bit 是 burst 裡 DIMM 自己數的 4 個 column，所以每 4 個位址就換一個 bank，8 個 bank 輪完一圈才換到 column 4–7，再輪一圈才換 row；只要 bank 數和 burst length 都是 2 的次方，控制交錯就只是分配位址 bit 的問題。模擬中這種切法讓 bus 98% 忙碌，把 bank 放最高位只剩 15%；但影片假設每個 burst 後都關 row，真的 controller 還要考慮 open page、channel、rank，以及 XOR hash。**

---

*筆記依據 YouTube 頻道 Computer Science Lessons「Dynamic Random Access Memory (DRAM). Part 7: Memory Address Mapping」（5:51）的 whisper 逐字稿與投影片截圖整理。實測為本機（Intel i7-13650HX、Ubuntu、Python 3.10）結果。*
