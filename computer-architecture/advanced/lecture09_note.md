# Advanced Computer Architecture 第九支影片完整筆記：Decode 時的最佳化：CISC → micro-op、predecode、stack pointer、指令壓縮（Chapter 3, Part 3/3）

> **課程**：Advanced Computer Architecture — Chapter 3: The fetch and decode stages: Part III/III (Decode time optimizations)（播放清單第 9 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（45:04，自動字幕）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch3** §3.5–3.6（p.83–91）
> **章號**：影片 Chapter 3 = 課本 Ch3。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、gcc 11.4、valgrind 3.18、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Decode 做的五件事](#1-decode-做的五件事)
- [2. CISC 的麻煩：長度不固定](#2-cisc-的麻煩長度不固定)
- [3. CISC 外、RISC 內：micro-op](#3-cisc-外risc-內micro-op)
- [4. Predecode：在 I-cache 裡標出邊界](#4-predecode在-i-cache-裡標出邊界)
- [5. 在 decode 裡處理 stack pointer](#5-在-decode-裡處理-stack-pointer)
- [6. 角落情況：sp 被載入新的值](#6-角落情況sp-被載入新的值)
- [7. 這樣做的好處](#7-這樣做的好處)
- [8. 指令壓縮：reduced-width ISA](#8-指令壓縮reduced-width-isa)
- [9. 指令壓縮：code word 與 dictionary](#9-指令壓縮code-word-與-dictionary)
- [10. 【實測】](#10-實測)
- [11. 一頁速查表](#11-一頁速查表)
- [12. 與後續講次的連結](#12-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

第 3 章的最後一講：decode stage，以及三種 decode 時的最佳化（大多是給 CISC 用的）：

| 最佳化 | 解決什麼 |
|---|---|
| CISC → **micro-op**（微碼表處理特別複雜的指令） | OOO pipeline 需要一致、規則的指令 |
| **Predecode** | 找出長度不固定的指令從哪裡開始、到哪裡結束 |
| 在 decode 裡處理 **stack pointer** | 很多指令只是在 sp 上加減常數 |
| **指令壓縮** | I-cache 放更多指令 |

---

## 1. Decode 做的五件事

[00:35]

1. **展開 immediate**：RISC 指令（例如 32 位元）放不下 32 位元的常數，只能放 16–18 bit（SimpleRisc：16 bit ＋ 2 bit modifier）。decode 時 sign extend 成 data path 的寬度（32 或 64 位元）。
2. **取出暫存器編號**：來源、目的。
3. **算 branch target**：大多是 **PC-relative**：target = PC ＋ 指令裡的 offset。
4. **處理特殊指令**：例如 `ret` 有一個**隱含**的來源 ra，這時明確寫進 instruction packet。
5. **加控制 bit**：之後的 stage 要用哪個功能單元、是不是記憶體指令……

---

## 2. CISC 的麻煩：長度不固定

[04:30] 【口頭】投影片上畫了一隻蠍子：CISC 最大的問題就在 decode stage。

[05:00] Fetch 讀進一串 byte，decode 要把它拆成一條一條指令。

| | RISC | CISC（x86） |
|---|---|---|
| 長度 | 固定 4 或 8 byte | **1 到 15 byte** |
| 對齊 | 4 byte 的指令從 4 的倍數開始 | 沒有 |
| 找邊界 | 一眼就知道 | 要從頭讀、解碼到結尾，才知道下一條從哪開始 |
| decode | 讀 16 byte，4 條**平行**解碼 | **循序**的 |

[07:24] 循序 decode 不能接受：一次就沒辦法處理多條指令。

[07:57] 另一個問題：有的 CISC 指令幾乎什麼都不做，有的做很多事。OOO pipeline **依賴指令的規則和一致**，很難處理這種複雜的指令。

【課本補充】課本 p.84：到 2020 年，x86 指令長度可以從 1 到 15 byte。（本機量到的長度分布見 [§10.1](#101-x86-指令有多長)。）

---

## 3. CISC 外、RISC 內：micro-op

[08:12] Intel、AMD 這些 CISC 處理器：fetch 從 I-cache 讀的是 CISC 指令，但 decode unit 在內部把它們**轉成 RISC 指令**，叫 **micro-op**（µop，或 microcode）。程式設計師看不到，但一定會發生，這樣才能從 OOO pipeline 拿到最大效能。

[08:48] 「**外面是 CISC、裡面是 RISC**。」

[09:07] 很複雜的例子：x86 的 **`rep movsd`**（Basic [第 13 支](../basic/lecture13_note.md)）：一條指令裡包了一整個迴圈，用隱含的 esi、edi、ecx 把 n 個元素從一處複製到另一處。轉成 RISC 時會變成**非常多個** micro-op。簡單的 add、sub（暫存器運算元）只變成 **1 個** micro-op。

[10:58] 所以 decode unit 分兩條路：

| CISC 指令 | 怎麼轉 |
|---|---|
| 比較簡單的（add、sub、mul、div…） | **一般的 decoder** 產生 micro-op |
| 非常複雜的（rep movsd 這類） | 查一張很大的 **microcode cache／microcode table**，直接讀出所有 micro-op，送下 pipeline |

（Basic [第 29 支 §13.2](../basic/lecture29_note.md#132-本機也有微碼)：本機的 Intel 處理器的微碼版本是 0x133。）

---

## 4. Predecode：在 I-cache 裡標出邊界

[13:09] CISC → RISC 解決了一個問題，但「在一串 byte 裡找出指令邊界」還沒解決。老師引用 **Narayan 和 Tran 的專利（1999）**，現代處理器都做類似的事，「否則沒有別的辦法」。

[14:13] 記憶體階層：pipeline 旁邊有分開的 **I-cache**（只讀）和 **D-cache**（讀寫），下面是同時放指令和資料的 **L2**，再下面還可以有更多層。

[15:12] 關鍵：指令從下層記憶體**搬進 I-cache 的時候**，經過一個小的 **predecoder**，替**每個 byte** 加幾個 bit 的註記。

[16:20] 那個專利：每個 8 bit 的 byte 多加 **5 bit**（「不小的負擔，但為了效能必要」）：

| bit | 意思 |
|---|---|
| **start bit** | 這個 byte 是一條指令的**開頭** |
| **end bit** | 這個 byte 是一條指令的**結尾** |
| functional bit | 依實作而定；例如 0 表示用 decoder、1 表示用 microcode cache |
| two-ROP、three-ROP | 這條指令會變成幾個 micro-op |

- [17:52] 後面三個 bit 的用法依實作而不同，但**前兩個**一定能找出邊界。
- [18:08] 只有 start bit 理論上就夠了，但加上 end bit 處理比較快。「在硬體裡，每省一個 cycle 都很重要。」
- [19:40] predecoder 比一般的 decoder **小很多**：只是掃過 byte、找出邊界。

---

## 5. 在 decode 裡處理 stack pointer

[19:40] 一段典型的函式程式碼：

```
sub sp, sp, 8          # 在 activation block 開兩格
st  r1, 4[sp]          # spill r1、r2
st  r2, 0[sp]
...                    # 做很多事、呼叫別的函式
ld  r1, 4[sp]          # 還原
ld  r2, 0[sp]
add sp, sp, 8          # 刪掉 stack frame
```

[20:42] 兩種很常見的模式：

1. 讀 sp、**加減一個常數**、寫回 sp（`sub sp, sp, 8`、`add sp, sp, 8`）。
2. load、store 讀 sp、加一個小常數當位址（`4[sp]`）。

[21:39] 想法：反正 fetch 和 decode 是**照程式順序**做的，那就在 decode unit 裡放一份 sp 的**小副本**，叫 **CSP**（current stack pointer）：

- 看到 `sub sp, sp, 8`：直接 CSP − 8。
- 看到 `add sp, sp, 8`：直接 CSP + 8。
- 看到 `ld r1, 4[sp]`：直接算出位址 CSP + 4，放進 instruction packet。

[22:24] 只有一個暫存器、不是整個 register file，所以存取很快。

[22:42] 硬體：decode stage 加一個 CSP 暫存器和一小塊「stack pointer 計算邏輯」，原本的 decoder 和 microcode cache 不變。很快地偵測指令有沒有用到 sp，有的話就同時送到這塊邏輯：

- 只**讀** sp（例如記憶體位址）：算出結果放進 instruction packet。
- **改** sp：把結果寫回 CSP。

【課本補充】課本 p.86–87：位址計算的那個 stage 可能在 **20–30 個 cycle** 之後，所以在 decode 就算好位址的好處非常大。

---

## 6. 角落情況：sp 被載入新的值

[25:41] 不符合「sp ± 常數」格式的指令，例如：

```
ld sp, 12[r1]          # sp 從記憶體載入一個新的值
```

[26:07] 做法：

1. 把 **CSP 設成 null**（表示不知道 sp 現在是多少）。
2. 這條指令當成一般指令，sp 當成一般暫存器，送進 OOO pipeline。
3. 在 load 回來之前，後面還是有 `add sp, sp, 8` 這種指令：**累積它們的差 Δ**。
4. [28:13] load 回來（值叫 addr）時：**CSP ← addr ＋ Δ**。

[28:27] 例：load 之後有兩條 `add sp, sp, 8`，Δ = 16；load 回來後 CSP = addr + 16。

[28:58] 【口頭】聰明的 compiler 不會產生這種程式碼，因為它知道處理器要多做事。這說明了**處理器和 compiler 之間很密切的關係**：compiler 要知道哪些指令處理器做得快、哪些會造成效能問題。

---

## 7. 這樣做的好處

[30:20]

- `sub sp, sp, 8` 在 decode 就做完了，**可以直接丟掉**，不佔 pipeline 後面的資源。
- 不用 pipeline 裡「重量級」的加法器，decode 裡一個快速加法就好。
- [31:57] Load／store 的位址在 decode 就算好：後面不用讀 sp、不用做加法；滿足一些條件（第 4 章）的話，load 可以**更早**送到記憶體系統。

（本機實測：x86 的 push／pop 真的有類似的硬體，見 [§10.2](#102-decode-有沒有處理-stack-pointer)。）

---

## 8. 指令壓縮：reduced-width ISA

[33:32] I-cache 很小：通常 **32–64 KB**，而效能對它的大小**非常敏感**，尤其是指令很多的雲端工作負載。所以想在 I-cache 裡塞越多指令越好。

[34:21] 方法一：**較窄的指令集**。例如 ARM 的 **Thumb**（「不是 arm，是 thumb，小很多」）：同樣大小的 I-cache 能多放 **30–40%** 的指令。怎麼縮：

| 方法 | 例子 |
|---|---|
| 只支援最常用的 opcode | add、sub、load、store、call、return |
| 不編碼複雜的 flag 和選項 | |
| 看到比較少的暫存器 | ARM 16 個 → Thumb **8 個**：每個暫存器編號從 4 bit 變 3 bit |
| 縮小 immediate 欄位 | 不需要放很大的常數 |
| 三個運算元改兩個 | `add r1, r2`（r1 ← r1 + r2），叫 **accumulator** 式，r1 同時是來源和目的 |

【課本補充】課本 p.88–89：Thumb 是 16 位元、32 位元 ARM ISA 的子集，程式通常**小 35%**；進 pipeline 後解壓成一般的 ARM 指令，所以處理器內部不用改。

---

## 9. 指令壓縮：code word 與 dictionary

[38:05] 方法二：程式裡常有**一樣的指令序列**出現在很多地方，例如進函式時 spill 暫存器、某些常見的運算。

[38:39] **compiler**（compile 時，靜態）或 **profiler**（用代表性的輸入跑一次，記下所有執行到的指令，動態）找出這些常出現的序列，換成一個很短的 **code word**。

[41:09] 處理器裡有一個 **dictionary**（像 microcode cache）：從 I-cache 讀到 code word，就查 dictionary，拿出那串指令，送給 fetch stage 的其他部分。⚠️ [41:24] 老師說投影片上「送到 decode stage」寫錯了，應該是 fetch stage（因為 fetch 還要做別的事）。

[42:10] 兩個好處：

1. 程式的靜態大小和動態大小都變小，I-cache 的指令密度變高。
2. dictionary 裡可以存**已經 decode 好的指令**（micro-op），直接跳過 decoder 到下一個 stage（renaming），省時間也省電。

[42:45] Intel 延伸了這個想法（不是壓縮）：**trace cache**，存一長串 decode 好的指令，跳過 fetch 和 decode。第 7 章再講（[第 26 支](lecture26_note.md)）。「這只是電影的預告。」

[44:23] 下一章：issue、execute、commit stage。

---

## 10. 【實測】

程式在 `experiments/lecture09/`。

### 10.1 x86 指令有多長

`lengths.py` 用第 7 支那個程式（`qsort` ＋ `snprintf`，靜態連結）：動態的部分用 valgrind lackey trace 裡每條指令的長度；靜態的部分用 `objdump -d --insn-width=15` 數整個執行檔：

```
動態（執行到的每一條）：10068446 條，平均 3.49 byte，最長 12 byte
   1B:5.2%  2B:25.5%  3B:21.1%  4B:29.4%  5B:8.0%  6B:7.2%  7B:1.7%  8B:1.2%  9B:0.6%
靜態（整個執行檔）：145988 條，平均 4.23 byte，最長 13 byte
   1B:3.7%  2B:14.1%  3B:20.7%  4B:21.9%  5B:17.2%  6B:10.0%  7B:7.8%  8B:2.1%  9B:1.0%  10B:0.7%  11B:0.7%
```

- 從 1 byte 到 12、13 byte 都有，沒有哪一種長度超過 30%。這就是 [§2](#2-cisc-的麻煩長度不固定) 的問題：decoder 讀到一串 byte，不解碼就不知道下一條從哪開始。
- 執行到的指令平均 **3.49 byte**，比 RISC 的 4 byte 還短：常用的指令（例如 `push`、`ret`、兩個暫存器的 `add`）都很短。x86 的變長編碼本身就有一點「壓縮」的效果，跟 [§8](#8-指令壓縮reduced-width-isa) 的 Thumb 是同一個想法。
- 同一個 `work.c` 只編譯成 object file 比較：x86-64 的 `.text` 是 430 byte、AArch64（固定 4 byte）是 421 byte，差不多。
- ⚠️ 用 objdump 預設的輸出時，最長只看到 7 byte：objdump 把超過 7 byte 的指令拆成兩行印。要加 `--insn-width=15` 才數得對。

### 10.2 decode 有沒有處理 stack pointer

`stackeng.c`：同樣是 4 條互相依賴的加減（做完回到原值），比較改 **rsp** 和改一般暫存器 **rbx**：

| 一串相依的 | P-core（cpu2） | E-core（cpu14） |
|---|---|---|
| `sub/add $8, %rsp` | 0.054 ns | 0.279 ns |
| `sub/add %reg, %rbx`（加暫存器） | 0.215 ns | 0.279 ns |
| `sub/add $8, %rbx`（加常數） | 0.057 ns | 0.279 ns |
| `push/pop`（隱含 rsp ± 8，加存取記憶體） | 0.069 ns | **0.139 ns** |
| `sub`、`mov rsp→rcx`、`mov rcx→rsp`、`add` | 0.055 ns | 0.139 ns |

- **E-core**：一般的 ALU 相依鏈（不管 rsp 還是 rbx）都是 0.279 ns，大約 **1 個 cycle**。但 **push／pop 只要 0.139 ns，半個 cycle**：push、pop 每一條都會改 rsp，如果真的照順序用 ALU 算，至少要 1 個 cycle。所以 E-core 在 decode 附近就處理掉了 push／pop 對 rsp 的 ±8，就是 [§5](#5-在-decode-裡處理-stack-pointer) 的 CSP 想法。但明確寫的 `sub $8, %rsp` 在 E-core 上**沒有**特別快。
- **P-core**：任何暫存器加減**常數**都是約 0.26 個 cycle（rsp、rbx 都一樣，第 4 支 [§12](lecture04_note.md#12-實測) 也看過），所以這個測試看不出 P-core 有沒有特別處理 rsp；push／pop 也很快（0.069 ns）。
- 「rsp 被搬到 rcx 再搬回來」這種 [§6](#6-角落情況sp-被載入新的值) 的角落情況，在這個簡單測試裡看不出額外的代價。處理器實際怎麼同步，**待補**。

---

## 11. 一頁速查表

| 主題 | 重點 |
|---|---|
| Decode 五件事 | 展開 immediate、取暫存器編號、算 PC-relative target、處理隱含運算元（ret 的 ra）、加控制 bit |
| CISC 的問題 | 長度 1–15 byte、沒對齊 → 找邊界是循序的；指令複雜度差很多 |
| Micro-op | CISC 外、RISC 內；簡單指令 1 個 µop；複雜的（rep movsd）查 microcode cache |
| Predecode | 搬進 I-cache 時每 byte 加 5 bit：start、end、functional、two-ROP、three-ROP（Narayan & Tran 1999） |
| CSP | decode 裡的 sp 副本；sp ± 常數直接算、load／store 位址直接算 |
| 角落情況 | sp 被載入：CSP = null，累積 Δ，回來後 CSP = addr + Δ |
| 好處 | sp 的加減直接丟掉；位址早算好，load 可以早送 |
| Compiler | 知道處理器哪些指令快，避免角落情況 |
| Reduced-width ISA | Thumb：少 opcode、8 個暫存器、小 immediate、兩運算元；小 30–40%（課本 35%） |
| Code word | 常見序列換成短 code word，dictionary 查回來；可以存 decode 好的 µop |
| Trace cache | Intel 存一長串 decode 好的指令（第 7 章） |
| 實測 | x86 執行到的指令平均 3.49 byte、1–12 byte 都有；E-core 的 push／pop 半個 cycle（比 ALU 快）→ 有 stack engine |

---

## 12. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Fetch、IST、BTB、branch prediction | [第 7 支](lecture07_note.md)、[第 8 支](lecture08_note.md) |
| x86 的編碼（prefix、ModR/M、SIB） | [Basic 第 14 支](../basic/lecture14_note.md) |
| rep movs、microcode、Linux 的 microcode 更新 | [Basic 第 13 支](../basic/lecture13_note.md)、[Basic 第 29 支](../basic/lecture29_note.md) |
| Renaming（decode 的下一個 stage） | [第 10 支](lecture10_note.md)（Ch4） |
| Load 早點送到記憶體的條件（load-store queue） | [第 12 支](lecture12_note.md)（Ch4） |
| Trace cache、instruction prefetch | [第 26 支](lecture26_note.md)（Ch7） |
| Spill、activation block | [Basic 第 8 支](../basic/lecture08_note.md) |

---

## 附：本講一句話總結

> **Decode 要展開 immediate、取出暫存器編號、算 PC-relative 的 branch target、補上隱含的運算元並加上控制 bit；對 x86 這種 CISC 最麻煩的是指令長度 1 到 15 byte 不固定，找邊界變成循序的工作，而且指令複雜度差很多，所以處理器「外面 CISC、裡面 RISC」，把指令轉成 micro-op（太複雜的查 microcode cache），並在指令搬進 I-cache 時 predecode，替每個 byte 加 start、end 等 5 個 bit 標出邊界；另外在 decode 裡放一份 stack pointer（CSP），sp 加減常數和以 sp 為基底的位址都當場算好（sp 被載入新值時先設成 null、累積差值 Δ），以及用 Thumb 這種較窄的指令集或 code word ＋ dictionary 壓縮指令，讓 I-cache 放更多指令。本機實測：真的 x86 程式執行到的指令從 1 到 12 byte 都有、平均 3.49 byte；E-core 上 push／pop 只要半個 cycle，比一般的 ALU 相依鏈還快，表示它在前端就處理了 stack pointer。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 3: The fetch and decode stages: Part III/III (Decode time optimizations) 錄影逐字稿（YouTube 自動字幕，45:04）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch3 §3.5–3.6（p.83–91）整理。實測為本機（Intel i7-13650HX、Ubuntu、gcc 11.4、valgrind 3.18、Python 3.10）結果。*
