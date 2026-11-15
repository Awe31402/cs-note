# DRAM 系列第四支影片筆記：Multiplexer 與 demultiplexer（Part 4: Multiplexers and Demultiplexers）

> **課程**：Dynamic Random Access Memory (DRAM) — Part 4: Multiplexers and Demultiplexers（播放清單第 4 支）
> **講者**：YouTube 頻道 Computer Science Lessons（旁白，沒有露臉）
> **來源**：YouTube 影片（8:41，whisper 轉錄）＋ 投影片截圖
> **標記說明**：
> - `【口頭】`：旁白講了、投影片上沒寫的內容
> - `【補充】`：影片沒講，從本 repo 其他筆記或通用知識補的
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、Python 3.10）實際執行的結果
> - `⚠️`：影片說法跟實際不一致、或逐字稿不清楚的地方
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. MUX 和 DEMUX 是什麼](#1-mux-和-demux-是什麼)
- [2. 並列 ↔ 串列](#2-並列--串列)
- [3. 分時多工（TDM）](#3-分時多工tdm)
- [4. MUX 的電路](#4-mux-的電路)
- [5. Demultiplexer 的電路](#5-demultiplexer-的電路)
- [6. 在 DRAM 裡的用途](#6-在-dram-裡的用途)
- [7. 總結](#7-總結)
- [8. 【實測】](#8-實測)
- [9. 一頁速查表](#9-一頁速查表)
- [10. 與其他講次的連結](#10-與其他講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
MUX（8-to-1）：8 條資料進、1 條出；3 條控制線決定哪一條通過
DEMUX（1-to-8）：1 條資料進、8 條出；3 條控制線決定送到哪一條

一起用：
  並列 → MUX → 一條線 → DEMUX → 並列（兩邊控制線要同步）
  多個發送端共用一條線 → TDM

電路：MUX = 8 個 AND（資料 + 控制）→ 1 個 OR
      DEMUX = 帶 enable 的 decoder，enable 當資料輸入

在 DRAM：讀 = column MUX 從 sense amp 選一個；寫 = column DEMUX 改一個
        memory controller 也用 MUX 把 row、column 位址分兩次送
```

---

## 1. MUX 和 DEMUX 是什麼

| | MUX（multiplexer） | DEMUX（demultiplexer） |
|---|---|---|
| 做什麼 | [00:00] 從幾條資料輸入**選一條**，轉到唯一的輸出 | [00:42] 反過來：一條資料輸入，**送到**幾條輸出之一 |
| 控制 | [00:14] 額外的**控制線**決定哪條輸入通過 | [00:57] 一樣用控制線 |
| 例子 | [00:34] 8 進 1 出 → **8-to-1 MUX** | [00:53] **1-to-8 DEMUX** |

投影片 [00:30]～[00:39] 輸入是 `1 0 1 0 1 0 1 0`：控制線 001 → 第 2 條（0）；011 → 第 4 條（0）；100 → 第 5 條（1）；111 → 第 8 條（0）。

---

## 2. 並列 ↔ 串列

- [01:11] MUX 和 DEMUX 常常一起用，例如 [01:15] 把**並列**傳輸轉成**串列**再轉回來。[01:21] 網路傳輸特別需要。
- [01:27] MUX：同時到的幾條輸入（並列），一個接一個送出去（串列）。
- [01:41] DEMUX：把這串資料重新組回並列。
- [01:47] 兩邊的控制線**一定要同步**。
- [01:55] 收端要一個 **buffer** 把組回來的輸出收齊再送走。

投影片 [01:48]～[01:57]：兩邊的控制線一起數 000 → 111，buffer 一格一格填成 `1 0 1 0 1 0 1 0`。

---

## 3. 分時多工（TDM）

- [02:01] MUX＋DEMUX 也能讓不同的訊號**共用同一個傳輸媒介**。[02:11] 媒介可以是銅線、光纖、甚至無線電。
- [02:17] **Time Division Multiplexing（TDM）**：MUX **輪流**（round-robin）取樣每條資料流，合成一條高速的輸出。
- [02:33] 每輪過所有輸入一次（可能約 1 ms），就產生一個 **frame**。
- [02:44] 收端的 DEMUX 再把它們分開。

| 種類 | 說明 |
|---|---|
| [02:49] **同步 TDM** | 兩端同步；每個發送端分到一樣多的時間，就算它沒東西要送 |
| [02:58] **非同步（統計）TDM** | [03:05] 優先給有東西要送的；[03:15] frame 裡要多帶資訊，說明每個發送端放了多少 |

- [03:24] 其他還有 frequency division、wavelength division、code division multiplexing。
- [03:35] 用在手機網路、[03:43] 電視和 DAB 廣播、數位混音等。

---

## 4. MUX 的電路

- [03:50] 用組合邏輯（一般的邏輯閘）就能做。
- [03:58] 8-to-1 MUX 內部：[04:02] 左邊 8 條資料、右邊 1 條輸出、下面 3 條控制線。

```
控制線 ─▶ 3 個 NOT ─▶ 每個 AND 拿到一組（原值/反相）
D0 ─┐
    AND(D0, ¬S2, ¬S1, ¬S0) ─┐
D1 ─┐                        │
    AND(D1, ¬S2, ¬S1,  S0) ─┤
 ...                          ├─▶ OR ─▶ 輸出
D7 ─┐                        │
    AND(D7,  S2,  S1,  S0) ─┘
```

- [04:10] 控制線 000：只有最上面那個 AND 被選到。[04:22] 它的資料輸入是 1 → AND 輸出 1。
- [04:31] 所有 AND 的輸出接到一個 **OR**。[04:37] 有一個是 1 → OR 輸出 1 → MUX 輸出 1。
- [04:46] 控制線 001：第二個 AND 被選到，資料是 0 → 輸出 0。
- [05:02] 010 → 第三條（1）；[05:09] 011 → 第四條，以此類推。

【補充】跟 [第 3 支](lecture03_note.md#2-用-not-和-and-組出來) 的 decoder 比：每個 AND 多吃一條資料線，最後多一個 OR。也就是「decoder 選一條，再把那條資料放出來」。

---

## 5. Demultiplexer 的電路

- [05:22] DEMUX 更簡單。
- [05:27] 電路**幾乎就是帶 enable 的 decoder**（[第 3 支 §4](lecture03_note.md#4-enable-輸入)），只是 enable 改當**資料輸入**。
- [05:36] 照控制線追下去，就知道 8 條輸出中哪一條被選到。資料是 1，那條就是 1；資料是 0，全部都是 0。

---

## 6. 在 DRAM 裡的用途

- [05:45] MUX／DEMUX 在電腦記憶體裡很重要。[05:52] 用一次讀寫 1 bit 的 DRAM 模型看：

| 誰 | 做什麼 | 時間 |
|---|---|---|
| column **MUX** | 讀：照 column 位址從很多 sense amp 選一個值，送往輸出 buffer | [05:59] |
| column **DEMUX** | 寫：把一個輸入值送去更新某一個 sense amp | [06:12] |
| memory controller 的 MUX | 把 row 位址和 column 位址**分開**送給記憶體模組（memory address multiplexing），[06:34] 控制讀寫的步驟，也省腳 | [06:23] |

投影片 [06:00]～[06:21]：讀 column 011 → 選到 1；寫：把 row 5 最右邊那格從 0 改成 1，sense amp 從 `1 1 0 1 0 0 1 0` 變成 `1 1 0 1 0 0 1 1`。

- [06:46] 【口頭】現代 DRAM 裡的 MUX／DEMUX 是為了面積和速度**高度最佳化**過的。
- [06:58] 用 CMOS 可以做出**一顆同時當 MUX 和 DEMUX** 的元件，共用輸入輸出。

【補充】這種雙向的就是 transmission gate 做的 analog switch：開關本身不分方向，訊號從哪邊進都能過，所以同一組 column 選擇電路讀寫都能用。

---

## 7. 總結

投影片 [07:12]～[08:21] 的 Summary：

1. MUX 從幾條輸入選一條，經由一條共同的輸出送出。
2. DEMUX 把一條輸入送到多條輸出之一。
3. MUX 的輸入、DEMUX 的輸出，由一組**控制線**選。
4. 可以用組合邏輯（AND、OR、NOT）做。[07:52] 但有些是用 CMOS 從頭設計，針對用途高度最佳化。
5. [08:04] 有些 MUX 一次選**多條**輸入、產生多條輸出。
6. 用在資料傳輸、電話、廣播、數位音訊、**電腦記憶體**等。

---

## 8. 【實測】

`experiments/lecture04/mux.py` 照影片的閘級電路寫 MUX（`mux.py:15`）和 DEMUX（`mux.py:20`），再跑一次並列 → 串列 → 並列。結果（`mux.out`）：

```
8-to-1 MUX（256x8 種）與 1-to-8 DEMUX（2x8 種）全部正確： OK
線上依序傳送： 10101010
收端 buffer  ： 10101010 （跟輸入一樣）
收端慢一拍   ： 01010101 （全部錯位）
```

- 窮舉 8 bit 資料的 256 種 × 8 種控制值，MUX 都輸出被選的那條；DEMUX 都只在被選的那條輸出資料。
- 收端控制線故意**慢一拍**（`mux.py:52`）：每個 bit 都被放進隔壁那格，整串錯位。這就是 [01:47]「控制線一定要同步」的意思。

---

## 9. 一頁速查表

| 觀念 | 重點 |
|---|---|
| MUX | 多進一出；控制線選哪一條通過 |
| DEMUX | 一進多出；控制線選送到哪一條 |
| 8-to-1 MUX 電路 | 8 個 4 輸入 AND（資料 + 3 條控制）＋ 1 個 8 輸入 OR |
| DEMUX 電路 | 帶 enable 的 decoder，enable 當資料 |
| 並列 ↔ 串列 | MUX 送、DEMUX 收；控制線要同步；收端要 buffer |
| 同步 TDM | 輪流、每人固定時段；一輪 = 一個 frame |
| 統計 TDM | 優先給有東西的；frame 要帶額外資訊 |
| DRAM 的 column MUX／DEMUX | 讀：從 sense amp 選一個；寫：改一個 sense amp |
| 位址多工 | memory controller 用 MUX 把 row、column 分兩次送 |

---

## 10. 與其他講次的連結

| 本講觀念 | 在哪裡繼續 |
|---|---|
| column MUX／DEMUX 在讀寫週期的哪一步 | [第 2 支 §6](lecture02_note.md#6-讀週期)、[§8](lecture02_note.md#8-寫週期) |
| decoder 與 enable | [第 3 支](lecture03_note.md) |
| 一次選 8 個 bit（8 個 array 同時） | [第 5 支 §4](lecture05_note.md#4-bank一次一個-byte) |

---

## 附：本講一句話總結

> **Multiplexer 用控制線從多條輸入選一條送到唯一的輸出，demultiplexer 反過來把一條輸入送到控制線選的那條輸出；兩者一起用可以把並列資料變串列再變回來，或讓多個發送端用分時多工共用一條線（兩邊控制線必須同步）。8-to-1 MUX 是 8 個「資料 AND 控制」的 AND 接一個 OR，DEMUX 就是把 enable 當資料的 decoder；在 DRAM 裡讀的時候 column MUX 從 sense amplifier 選一個 bit、寫的時候 column DEMUX 改一個 sense amplifier，memory controller 也用 MUX 把 row、column 位址分兩次送。**

---

*筆記依據 YouTube 頻道 Computer Science Lessons「Dynamic Random Access Memory (DRAM). Part 4: Multiplexers and Demultiplexers」（8:41）的 whisper 逐字稿與投影片截圖整理。實測為本機（Intel i7-13650HX、Ubuntu、Python 3.10）結果。*
