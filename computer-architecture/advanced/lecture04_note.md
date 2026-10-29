# Advanced Computer Architecture 第四支影片完整筆記：In-order pipeline 的快速複習：五級 pipeline、hazard、interlock、forwarding（Chapter 2, Part 1/3）

> **課程**：Advanced Computer Architecture — Chapter 2: Out-of-order Pipelines: Part I/III (Overview)（播放清單第 4 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕一直下載不到）（58:14）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch2** §2.1（p.25–43）
> **章號**：影片 Chapter 2 = 課本 Ch2。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

這一講是 Basic 課程第 27、30–32 支的**濃縮複習**。每一段都只寫重點，細節請看對應的 Basic 筆記。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 先修知識](#1-先修知識)
- [2. 五個階段](#2-五個階段)
- [3. 加上 pipeline register](#3-加上-pipeline-register)
- [4. 三種 hazard](#4-三種-hazard)
- [5. Pipeline diagram 與 RAW hazard](#5-pipeline-diagram-與-raw-hazard)
- [6. Interlock 與 bubble](#6-interlock-與-bubble)
- [7. Forwarding](#7-forwarding)
- [8. 四條 forwarding path](#8-四條-forwarding-path)
- [9. Load-use hazard](#9-load-use-hazard)
- [10. Control hazard](#10-control-hazard)
- [11. In-order pipeline 的限制](#11-in-order-pipeline-的限制)
- [12. 【實測】](#12-實測)
- [13. 一頁速查表](#13-一頁速查表)
- [14. 與後續講次的連結](#14-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

第 2 章分三講：

| 講 | 內容 |
|---|---|
| **本講（第 4 支）** | in-order pipeline 的複習 |
| [第 5 支](lecture05_note.md) | 效能模型 → 為什麼需要 OOO |
| [第 6 支](lecture06_note.md) | 相依的種類、renaming、precise exception |

[03:33] 老師說：幾乎所有筆電、桌機、伺服器，甚至手機的處理器現在都用 **out-of-order pipeline**（簡寫 **OOO**，「triple O」）。

---

## 1. 先修知識

[00:56] 需要三樣：**RISC 組合語言**、**基本處理器設計**、**基本 pipeline 設計**。老師指向他 2015 年 McGraw-Hill 的書（[Basic 課程](../basic/README.md)），以及那本書在 YouTube 上的影片。

[02:40] 「當成**必修**。」不一定要讀他的書，只要懂這三件事就好。

[04:25] 【口頭】這張圖如果看不懂，要回去看前一本書的影片；在那邊「整整一章、好幾個小時」講這張圖，這裡 10 分鐘就帶過。

---

## 2. 五個階段

[04:25] 把一條指令的處理分成 5 個階段（課本 §2.1.1，p.26；Basic [第 27 支](../basic/lecture27_note.md)）：

| 階段 | 做什麼 |
|---|---|
| **IF**（fetch） | 從指令記憶體（實際上是 **I-cache**）讀指令 |
| **OF**（operand fetch／decode） | 控制單元（decoder）產生控制訊號；immediate and branch unit 展開常數、算 branch target；讀 register file 得到 op1、op2；一個多工器選第二個運算元是暫存器還是 immediate |
| **EX**（execute） | ALU 做運算；branch unit 決定 branch 跳不跳；cmp 設 flags |
| **MA**（memory access） | 只有 load、store 會用；位址在 ALU 算好（例如 r2 + 4） |
| **RW**（register write-back） | 把 ALU 或 load 的結果寫回 register file |

- [05:12] **Cache**：一小塊記憶體，存一部分的記憶體位置。I-cache 放指令，**D-cache** 放資料；後面還有更大的 L2、L3（之後講）。
- [06:50] 指令像一個「壓縮過的包裹」（opcode、暫存器、位址都擠在一起），decoder 把它展開。
- [07:33] 一般處理器有 16–32 個暫存器（⚠️ 老師先說 15，馬上改口 16）。
- [09:19] 這些資訊跟著指令一起走，叫 **instruction packet**（課本 p.27）。
- [12:36] 只支援 **base-offset** 定址（SimpleRisc，課本 Appendix A）。
- [14:06] 有兩條**往回**的線：branch unit → fetch unit（branch 的方向）；RW → OF（寫回 register file）。【口頭】老師說投影片上那條線的顏色「介於紅色和粉紅色之間，如果你知道正確的名稱請寫在留言區」。

---

## 3. 加上 pipeline register

[16:26] 沒有 pipeline 時，任何時間只有一個單元在工作，**80% 的晶片是閒的**。

[17:10] Pipeline：指令 1 進 OF 時，抓指令 2；依此類推。某個時候 5 條指令同時在 5 個 stage 裡。

[18:40] 唯一的改變：把 stage 之間的那束線（「interconnection element」）換成 **pipeline register**：

- 負緣觸發的 flip-flop（Basic [第 19 支](../basic/lecture19_note.md)）：負緣時把左邊寫進來的存起來，下個 cycle 給右邊的 stage 用。
- 每個 stage 必須在**一個 clock cycle** 內做完。
- 存的不是一個 bit 或一個 byte，而是**整個 instruction packet**（幾十個 byte）。
- [21:47] n 級 pipeline 需要 **n − 1** 個 pipeline register；5 不是硬性規定，可以再切。

（Basic [第 30 支 §4](../basic/lecture30_note.md#4-pipeline-register)）

---

## 4. 三種 hazard

[22:40] 同時有 5 條指令，可能會「撞在一起」：

| Hazard | 意思 | 本課程 |
|---|---|---|
| **Structural** | 兩條指令同時要用同一個資源，例如資料記憶體一個 cycle 只能存取一次 | 我們的 pipeline **不會**發生（除非 ISA 加了在 EX 就存取記憶體的指令），之後不討論 |
| **Data** | RAW：讀的那條可能讀不到正在被寫的值 | §5–§9 |
| **Control** | branch taken 時，後面抓錯了 | §10 |

[24:46] 老師特別寫下來：**指令從不跳過 stage**。連不存取記憶體的 ALU 指令也要經過 MA。「很多人在這裡犯錯。」

---

## 5. Pipeline diagram 與 RAW hazard

[26:13] **Pipeline diagram**：列是 stage（IF、OF、EX、MA、RW），欄是 cycle。

```
[1]: add r1, r2, r3
[2]: add r4, r1, r3      ← 讀 r1：RAW
```

[27:05] 指令 2 在 **cycle 3** 的 OF 讀 r1，但指令 1 要到 **cycle 5 結束**才把 r1 寫進 register file。指令 2 會讀到舊的、錯的值，程式就錯了。

[28:39] 這是 pipeline 的平行處理才會有的問題。

---

## 6. Interlock 與 bubble

[29:29] 讓指令 1 繼續走；指令 2 在 OF 等。EX 那邊插入 **bubble**：

- **Bubble** = **nop**（no operation），沿著 pipeline 走、什麼都不做。
- cycle 4、5、6 各插一個，共 **3 個 bubble**。
- [30:57] cycle 5 結束時 r1 寫好，cycle 6 指令 2 從 register file 讀到。
- [31:42] In-order：指令不會互相超車。指令 2 從 OF 進 EX 時，指令 3 才從 IF 進 OF。

[31:42] 插 bubble 的邏輯叫 **interlock**（這裡是 **data interlock**）：它看得到 pipeline 裡所有的指令，決定誰前進、誰要停。「把 pipeline 鎖住，直到資料好了再放開」，這就是 interlock 這個名字的由來。

[32:33] 外面的觀察者看到：3 個 cycle 都沒有指令完成，這叫 **stall**：沒有做任何有用工作的 cycle。

[33:17] RAW 很常見：producer 後面**緊接著**就讀要停 3 個 cycle，隔一條停 2 個、隔兩條停 1 個。這嚴重限制了 compiler 怎麼產生程式碼，也讓程式設計師很難寫出有效率的程式。

（Basic [第 31 支 §5–§10](../basic/lecture31_note.md#5-interlockdata-lock-與-branch-lock)）

---

## 7. Forwarding

[34:59] 同一個例子：讓指令 2 在 cycle 3 讀錯的值，沒關係，因為它 cycle 4 進 EX 才真的用。

[35:48] cycle 4 時指令 1 在 MA，**r2 + r3 已經算好、放在它的 instruction packet 裡**，只是還沒寫回 register file。直接把它從 MA **forward** 到 EX。

[36:34] 這樣**完全不用 stall**。結果在內部轉送，不經過 register file；經過 register file 的話要等到 cycle 5 結束。

[38:19] 實作：在 ALU 的兩個輸入前面各加一個**多工器**，另一個輸入是 forward 進來的值。**Forwarding unit** 產生這些多工器的控制訊號，決定用預設的輸入還是 forward 來的。

（Basic [第 31 支 §12](../basic/lecture31_note.md#12-forwarding-的想法)、[第 32 支](../basic/lecture32_note.md)）

---

## 8. 四條 forwarding path

[41:53] 兩條公理：

1. 從**後面**的 stage forward 到**前面**的 stage。
2. **盡量晚** forward：剛好要用時才給，可以減少路徑數。不影響正確性。

[42:37] 我們的 pipeline 只需要**四條**（「前一本書證明過」，課本 p.42）：

| Path | 例子 |
|---|---|
| **RW → MA** | `ld r1` → `st r1`（緊接著） |
| **RW → EX** | `ld r1` → 無關 → 讀 r1 |
| **RW → OF** | `ld r1` → 兩條無關 → 讀 r1 |
| **MA → EX** | `add r1` → `add` 讀 r1（緊接著） |

[43:21] **Program order**：一個單週期、沒有 pipeline 的處理器一條一條執行時看到的動態指令順序。

[46:44] 整個 pipeline 有 **6 個藍色的 forwarding 多工器**。forwarding unit 看得到所有指令，決定誰前進、誰 stall、誰 forward 到哪。

[45:47] 【口頭】pipeline 更長，forwarding path 和多工器就更多、更複雜，所以通常**避免很長的 in-order pipeline**。

[48:20] 「傳統的計算機結構課講 pipeline 大概就停在這裡。」

---

## 9. Load-use hazard

[49:06] 「很有名的例子」：forwarding 救不了。

```
ld  r4, 0[r0]        ← 較早
add r5, r4, 1        ← 下一條就用 r4
```

[50:39] r4 最早在 load 的 **MA 結束**時才有；add 在**同一個 cycle 的開頭**（它的 EX）就要用。時間要倒流，不可能。

[51:18] 唯一的辦法：**停 1 個 cycle**，再用 forwarding。Interlock 和 forwarding **一起**解決。

[49:55] 這對 OOO pipeline 的設計也有重要影響。

（本機實測：真的 load-use 延遲是幾個 cycle，見 [§12](#12-實測)。）

---

## 10. Control hazard

[52:55] 例：`beq`。branch 往前走時，後面的指令照程式順序繼續被抓進來：

- **Not taken**：它們在正確的路徑上，沒問題。
- **Taken**：branch 到 **EX** 才知道結果，這時已經抓了 2 條 **wrong path** 的指令，要把它們變成 bubble（作廢）。

[54:13] 每個 taken branch 就等於 **stall 2 個 cycle**，「真的很糟」。

（Basic [第 31 支 §11](../basic/lecture31_note.md#11-branch-lockpredict-not-taken)）

---

## 11. In-order pipeline 的限制

[55:01]

1. **Load-use hazard** 解決不了。
2. 我們假設 MA 只要 1 個 cycle，**實際上不是**：cache 和記憶體很慢，可能要**幾十個 cycle**。
3. **Head-of-line blocking**：一條指令卡住，後面跟它完全無關的指令也全部停下來，只因為它們「犯了排在它後面的罪」。【口頭】像單線道上的車隊，一台車拋錨，後面全部停住，叫 **convoy effect**（課本 p.58 Summary 也提到）。
4. Branch 處理得不好：taken branch 停 2 個 cycle。
5. [56:37] **IPC 通常小於 1**：對外面的觀察者，每個 cycle 處理不到一條指令（下一講量化）。

[57:20] 所以大型處理器基本上不用 in-order pipeline。OOO 會在很大程度上解決這些問題。

---

## 12. 【實測】

`experiments/lecture04/loaduse.c`：讓每一條指令都依賴上一條的結果（相依鏈），量平均時間。綁在 P-core（cpu2，4.7 GHz）和 E-core（cpu14）：

| 相依鏈 | P-core | P-core 約幾個 cycle | E-core |
|---|---|---|---|
| `add $1, r`（加常數） | 0.066 ns | **0.3** | 0.279 ns |
| `add r1, r`（加暫存器） | 0.214 ns | **1** | 0.279 ns |
| `mov (r), r`（L1 裡的 pointer chase） | 1.072 ns | **5** | 0.836 ns |
| `mov (r), r` ＋ `add r1, r` | 1.281 ns | **6** | 1.393 ns |

- **Forwarding 在真的 CPU 上**：相依的 `add` 一個接一個，每個 **1 個 cycle**，完全不 stall。跟 §7 說的一樣：值不用等寫回 register file，直接轉送給下一條。
- **Load-use**：L1 cache 命中的 load，下一條馬上用，P-core 要 **5 個 cycle**；再接一個 add 就是 **6 個**（5 + 1）。課本的 5 級 pipeline 是 1 個 cycle 的 stall，真的處理器 pipeline 更深、L1 也不是 1 個 cycle，代價更大。這就是 [§11](#11-in-order-pipeline-的限制) 第 2 點。
- ⚠️ P-core 上加**常數**的相依鏈只要 **0.3 個 cycle**，比 1 個 cycle 還快，表示 P-core 在執行前就用某種方式處理了「加一個小常數」，不是照一般的 ALU 加法一個一個做。E-core 上就是正常的 1 個 cycle。P-core 實際怎麼做到的，**待補**。
- E-core：`add` 一個 0.279 ns（約 1 個 cycle），load 0.836 ns。

---

## 13. 一頁速查表

| 主題 | 重點 |
|---|---|
| 五個階段 | IF、OF、EX、MA、RW；I-cache、D-cache |
| Instruction packet | 跟著指令走的所有資訊 |
| Pipeline register | 負緣觸發；n 級要 n − 1 個；每級一個 cycle |
| 三種 hazard | structural（我們的不會）、data、control |
| 不跳 stage | ALU 指令也要經過 MA |
| RAW | 緊接著要 3 個 bubble；隔一條 2 個、隔兩條 1 個 |
| Interlock | 看得到所有指令；鎖住 pipeline 直到資料好 |
| Stall | 沒有指令完成的 cycle |
| Forwarding | MA 的結果直接給 EX；多工器 ＋ forwarding unit |
| 公理 | 後 → 前；盡量晚 |
| 四條 path | RW→MA、RW→EX、RW→OF、MA→EX；6 個多工器 |
| Load-use | 停 1 個 cycle ＋ forwarding |
| Control | taken → 作廢 2 條 → 等於停 2 個 cycle |
| 限制 | load-use、MA 很慢、convoy effect、branch、IPC < 1 |
| 實測 | 相依 add 1 cycle；L1 load-use 5 cycle（P-core） |

---

## 14. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| 單週期處理器的五個階段 | [Basic 第 27 支](../basic/lecture27_note.md) |
| Pipeline register、hazard | [Basic 第 30 支](../basic/lecture30_note.md) |
| Interlock、bubble、forwarding 的想法 | [Basic 第 31 支](../basic/lecture31_note.md) |
| Forwarding 的實作、M1–M6 | [Basic 第 32 支](../basic/lecture32_note.md) |
| 效能模型、為什麼需要 OOO | [第 5 支](lecture05_note.md) |
| OOO 能在 cache miss 後面繼續做事（解 convoy effect） | [第 3 支 §10.2](lecture03_note.md#102-本機的-out-of-order-視窗有多大) |
| Branch prediction | [第 8 支](lecture08_note.md) |

---

## 附：本講一句話總結

> **In-order pipeline 把指令分成 IF、OF、EX、MA、RW 五個階段，中間放存整個 instruction packet 的 pipeline register，讓五條指令同時在不同階段；但它會有 hazard：RAW 的 data hazard 可以用 interlock 插 bubble（stall）解，更好的是用 forwarding 把還沒寫回的結果直接轉送給下一條（照「後到前、盡量晚」只要 RW→MA、RW→EX、RW→OF、MA→EX 四條路徑），唯一救不了的是 load 後面馬上用的 load-use hazard（要停一個 cycle），taken branch 則要作廢兩條 wrong path 的指令；再加上 MA 其實很慢、一條卡住後面全停的 convoy effect，in-order pipeline 的 IPC 小於 1，所以大型處理器改用 out-of-order。本機實測：真的 CPU 上相依的 add 一個 cycle 一條（forwarding），但 L1 命中的 load 後面馬上用就要 5 個 cycle。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 2: Out-of-order Pipelines: Part I/III (Overview) 的 whisper 轉錄稿（本機 `large-v3-turbo`，58:14）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch2 §2.1（p.25–43）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4）結果。*
