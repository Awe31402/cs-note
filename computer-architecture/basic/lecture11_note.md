# Basic Computer Architecture 第十一支影片完整筆記：x86 的機器模型（Chapter 5, Part 1/4）

> **課程**：Basic Computer Architecture — Chapt 5: x86 assembly, Part 1/4（播放清單第 11 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（1:13:21，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 Ch5 §5.1–5.2（p.175–185）
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（`檔名:行號`）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、NASM）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. x86 是一個家族](#1-x86-是一個家族)
- [2. x86 的主要特色](#2-x86-的主要特色)
- [3. 為什麼要向下相容](#3-為什麼要向下相容)
- [4. 整數暫存器：ax、eax、rax 是同一個暫存器](#4-整數暫存器axeaxrax-是同一個暫存器)
- [5. 8 位元暫存器：al、ah](#5-8-位元暫存器alah)
- [6. flags 與 ip](#6-flags-與-ip)
- [7. 浮點暫存器：FP stack](#7-浮點暫存器fp-stack)
- [8. 記憶體：segmented memory model](#8-記憶體segmented-memory-model)
- [9. 現代的 segmentation：GDT 與 segment descriptor cache](#9-現代的-segmentationgdt-與-segment-descriptor-cache)
- [10. 定址模式](#10-定址模式)
- [11. x86 組合語言的格式（NASM、Intel 語法）](#11-x86-組合語言的格式nasmintel-語法)
- [12. 運算元的規則](#12-運算元的規則)
- [13. 【原始碼】Linux 的 GDT](#13-原始碼linux-的-gdt)
- [14. 【實測】](#14-實測)
- [15. 一頁速查表](#15-一頁速查表)
- [16. 與後續講次的連結](#16-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

Chapter 5 的大綱（[07:11]）：

1. **機器模型** ← 本講
2. 簡單的整數和分支指令 ← 第 12 支
3. 進階記憶體指令 ← 第 13 支
4. 浮點指令 ← 第 13–14 支
5. 編碼 ← 第 14 支

這一講只講 x86「看到的機器長什麼樣」：

```
暫存器：rax ⊃ eax ⊃ ax ⊃ ah/al（同一個暫存器，不同的看法）＋ r8–r15
       rflags、rip
浮點：st0–st7，80 位元，排成一個 stack
記憶體：segmented（cs、ds、ss、es、fs、gs）→ GDT → segment descriptor cache
定址：[base + index × scale + displacement]，每一項都可有可無
```

- 【口頭】**x86 是進階的 ISA**。沒有組合語言基礎的話會很難懂，「一邊走路一邊嚼口香糖」不可能：不能同時學組合語言的基本觀念和一種特定的組合語言。建議先讀 Chapter 3；**Chapter 4（ARM）可以不讀**。

---

## 1. x86 是一個家族

[00:45]–[04:29]　課本 §5.1.1（p.175–176）

| 世代 | 處理器 | 位元 | 年代 |
|---|---|---|---|
| 曾祖父 | 8080 | 8 | 1970 年代中 |
| 祖父 | **8086** | 16 | 1978 |
| 父母 | 80386、80486、Pentium 1–4 | 32 | 1990 年代～2000 年代初 |
| 現在 | Intel Core i3／i5／i7；AMD Opteron、Bulldozer | 64 | |

- 名字的由來：這些處理器的名字**都以 86 結尾**（8086、80386、80486……），所以這個 ISA 叫 **x86**。
- 【口頭】8086 在當時很革命性：16 位元很少見；它開啟了微處理器的時代，大型伺服器也漸漸改用小型微處理器組成。
- **32 位元／64 位元**的意思：暫存器的大小，也是處理器能發出的記憶體位址的大小。
- 【口頭】i5 的組合語言是早期處理器組合語言的**超集合**（superset）。
- 【課本補充】8086 之後是 1982 年的 80186（嵌入式）和 80286（桌機），1985 年 80386、1989 年 80486。IBM PC 大多用 Intel 處理器。AMD 在 1990 年代中推出 K5、K6、K7（32 位元 x86），**2003 年推出 x86-64**。VIA、Transmeta 等公司也做 x86 處理器。各家都加了自己的擴充：Intel 的 MMX、SSE1～4，AMD 的 3D Now!。
- 【課本補充】課本選 **32 位元 x86** 來講，因為它在使用範圍的「中間」：16 位元很少用，64 位元主要在工作站和伺服器。

---

## 2. x86 的主要特色

[04:29]–[05:51]　課本 §5.1.2（p.176）

1. **CISC**：指令長度不固定，運算元長度也不固定。
2. 至少 **300 條**純量指令。影片：「加上所有擴充，截至 2016 年大約 1000 條」；課本：「截至 2012 年超過 900 條」。
3. **幾乎所有指令都可以有記憶體運算元**，而且大部分允許來源和目的地都是記憶體。（⚠️ 但不能**兩個都是**，見 §12。）
4. 大部分是 **2-address format**：`add eax, ebx` → eax = eax + ebx。
5. 很多複雜的記憶體定址模式。
6. **沒有 return address 暫存器**：呼叫和回傳用 **stack** 存 return address（上一講的 SimpleRisc、ARM 用暫存器）。
7. 和 ARM、SimpleRisc 一樣有 **flags** 暫存器。
8. **Segmented memory**：指令和資料在不同的區域。

- 【課本補充】課本的看法：x86 雖然是 CISC，「實際上是相當簡單、容易理解、很優雅的指令集」。而且 Pentium 4 以後的 Intel／AMD 處理器**在內部把 x86 指令翻成 RISC 指令**，整顆處理器本質上是 RISC，「兩全其美」。

---

## 3. 為什麼要向下相容

[07:25]–[09:43]

- 現代 Intel 處理器和最老的 **8086 是 ISA 相容的**：8086 的程式在現代處理器上還能跑。
- 原因是**市場需求**：
  - 一家公司有很多為 32 位元處理器寫的程式。為了效能買了新的 64 位元處理器。
  - 叫它丟掉所有程式重寫或重新編譯是很糟的主意：原本的開發者可能不在了、原始碼可能找不到了、工作量很大、還有穩定性和可靠性的問題。
  - 客戶要的是：新處理器可以加功能，但**不能讓原本能跑的東西壞掉**。
- 所以 Intel 和 AMD 都確保：最新的處理器能跑 30 年前寫的 binary。

---

## 4. 整數暫存器：ax、eax、rax 是同一個暫存器

[09:43]–[23:26]　課本 §5.2.1、Figure 5.1（p.177–178）

**問題**：16 位元、32 位元、64 位元的程式都要能跑，要準備三組暫存器嗎？

- **不行**。暫存器所在的區域非常擁擠（【口頭】「像處理器的市中心」：暫存器、加法器、乘法器等等擠在一起），不能隨便加暫存器。
- **解法**：把 16 位元的暫存器**加寬**成 32 位元，再加寬成 64 位元。16 位元的程式只用低 16 位。

**16 位元的 8 個暫存器**（1978 年的 8086）：

| 暫存器 | 用途 |
|---|---|
| ax、bx、cx、dx | 一般用途（名字來自 8 位元處理器的 a、b、c、d） |
| sp | **stack pointer** |
| bp | **base pointer**（frame pointer）：進入函式時存 sp 的初值 |
| si | source（來源） |
| di | destination（目的地） |

- si、di 也能拿來做一般運算，但**有些指令預設 si 是來源、di 是目的地**（之後講）。
- 【口頭】x86 很保守，只有 8 個暫存器；ARM 和 SimpleRisc 有 16 個。

**同一個暫存器的不同看法**（【口頭】「這張圖非常重要，之後會一直回來看」）：

```
 63                    31            15     7      0
┌──────────────────────┬─────────────┬──────┬──────┐
│                  rax（64 位元）                    │
│                      │    eax（32 位元）           │
│                      │             │  ax（16 位元）│
│                      │             │  ah  │  al  │
└──────────────────────┴─────────────┴──────┴──────┘
```

| 64 位元 | 32 位元 | 16 位元 | 8 位元 |
|---|---|---|---|
| rax | eax | ax | ah、al |
| rbx | ebx | bx | bh、bl |
| rcx | ecx | cx | ch、cl |
| rdx | edx | dx | dh、dl |
| rsp | esp | sp | |
| rbp | ebp | bp | |
| rsi | esi | si | |
| rdi | edi | di | |
| r8–r15 | （沒有子欄位，影片的說法） | | |

- **e** = extended（32 位元）；**r**（64 位元）。
- 64 位元處理器實體上只有這些 64 位元暫存器。讀 eax 時，處理器讀 rax、**丟掉高 32 位**；讀 si 時，讀 rsi、只看低 16 位。
- 寫 si 時，寫進 rsi 的**低 16 位**。
- 32 位元處理器沒有 rax：寫 rax 會出錯；寫 ax 沒問題。
- 【口頭】「儲存空間是同一個，只是我們的**看法**不同。」
- 好處：不浪費空間，程式碼相容。【口頭】「讓客戶開心；設計者沒那麼開心，因為生活變得稍微困難，但其實沒那麼難。」
- **x86-64 多了 r8～r15**。【口頭】一般規則：今天寫的程式要能在**未來**的處理器上跑；不需要能在**過去**的處理器上跑。所以 64 位元的 binary 不能在 32 位元處理器上跑，這沒關係。
  - ⚠️ 影片說到最後口誤成「R9 到 R15」，馬上更正是 **R8 到 R15**。

⚠️ 兩個影片沒講的細節（§14.1 實測）：

- **寫 32 位元暫存器會把高 32 位清成 0**。寫 al、ah、ax 只改那幾個位元，但 `mov eax, …` 會讓 rax 的高 32 位**變成 0**。這是 x86-64 的規則（為了避免 CPU 內部的相依性問題），和「寫進低 32 位」的直覺不一樣。
- **r8～r15 其實有子欄位**：r8d（32 位元）、r8w（16 位元）、r8b（8 位元）。課本說「它們的子欄位不能直接存取」，不精確；r8～r15 是**沒有對應的 16 位元、32 位元時代的舊名字**。

【課本補充】8080 的暫存器、bp、si／di 的由來：

- 課本說 8080 有 7 個 8 位元暫存器「a、b、c、d、e、f、g」。⚠️ 8080 實際的暫存器是 **A、B、C、D、E、H、L**（沒有 f、g）。
- 8086 的設計者留下 a、b、c、d，加上 x（extended）變成 ax、bx、cx、dx；加上 sp、ip；再加了 bp、si、di。
- **bp**：在函式開始時存 sp 的值；函式結束時 compiler 把 sp 設回 bp，就銷毀了 callee 的 stack frame。
- **si、di**：被 **rep** 指令用到；一條 rep 指令就等於一個簡單的 for 迴圈。課本把 si 叫 starting index，di 叫 destination index（Intel 的正式名稱是 source index、destination index）。

---

## 5. 8 位元暫存器：al、ah

[23:26]–[25:21]

- ax、bx、cx、dx 又各分成兩個 8 位元：
  - **低**8 位：al、bl、cl、dl（L = lower）
  - **高**8 位：ah、bh、ch、dh（H = higher）
- 原本是為了和 8 位元的 8080 相容，但這點現在不重要。**真正有用的是可以用 byte 為單位存取資料**。

---

## 6. flags 與 ip

[25:21]–[30:17]　課本 Table 5.1（p.178）

這兩個暫存器**由系統管理**，程式設計師可以讀，但不能直接改。

**flags**：flags（16）、eflags（32）、rflags（64），和 ax／eax／rax 一樣的命名規則。和 SimpleRisc 的 flags、ARM 的 CPSR 一樣：存最近一次比較（或其他會設定 flags 的指令）的結果，給之後的分支指令用。

| 欄位 | 名稱 | 什麼時候設定 |
|---|---|---|
| **OF** | Overflow flag | overflow |
| **CF** | Carry flag | 加法有進位或減法有借位 |
| **ZF** | Zero flag | 結果是 0，或比較的結果相等 |
| **SF** | Sign flag | 結果的符號位 |

- 還有其他位元，見 Intel 手冊。
- ⚠️ 「flags 不能被程式修改」不完全對：`pushf`／`popf` 可以把 flags 存到 stack 再讀回來（某些位元需要權限才能改）；`stc`／`clc` 可以直接設定／清除 CF。

**ip**（instruction pointer）= PC：ip（16）、eip（32）、**rip**（64）。

- 程式不能直接設定 ip，也不能直接讀；但有一些指令可以間接讀到它。
- 【課本補充】和 ARM 不同，x86 的 PC 程式設計師**看不到**。
- 64 位元模式可以用 **rip-relative** 定址（§14.3 實測）：`[rip + offset]`，這就是第 6 支講的 PC-relative。

---

## 7. 浮點暫存器：FP stack

[30:17]–[37:49]　課本 §5.2.2、Figure 5.2（p.179）

**歷史**：

- 80386（1980 年代中）**沒有**浮點運算，要另外接一顆 **80387** 共處理器（coprocessor）。
- 80486 才把浮點單元放進處理器。
- 當時電晶體少，架構反而比較複雜；為了相容，x86 一直沒辦法改掉浮點暫存器的設計。
- 【口頭】影片解釋 ARM 那章為什麼沒講浮點：ARM 比較晚才加浮點支援，不適合放進基礎課程；x86 的浮點支援很早就有了。

**兩種看法**：

1. 8 個暫存器 **st0～st7**，可以個別存取。
2. 它們排成一個 **stack**（**FP stack**，浮點暫存器堆疊）。

- 每個暫存器是 **80 位元**，比 double（64 位元）精度更高。
  - 【課本補充】80 位元格式：**64 位元 mantissa** ＋ 15 位元 exponent（＋ 1 位元符號）。
- **st0 是 stack 的頂端，st7 是底部**。
- **暫存器名稱代表的是「在 stack 上的位置」**，不是固定的儲存位置。push 或 pop 之後，每個 stN 的值都會變。

例：stack 本來是 3、4.5、6（3 在頂端）。push 2 之後：

```
push 前：        push 2 之後：
st0 = 3          st0 = 2
st1 = 4.5        st1 = 3
st2 = 6          st2 = 4.5
                 st3 = 6
```

- 【口頭】「這個觀念很重要，學生通常搞不懂，所以我盡量用很多種方式解釋。」把它想成一個**有編號的 stack**，可以用「離頂端第幾個」存取元素。
- 為什麼這樣設計？歷史原因。【口頭】Chapter 1 講過 **stack 機器**：二、三十年前，為 stack 機器寫程式比較容易。為了相容，就一直保留下來，不然為 486 寫的程式就不能在 i7 上跑，x86 廠商會失去生意。
- 【課本補充】
  - 還有一個 **tag 暫存器**：每個 stack 位置 2 位元，00 = 有效資料、01 = 0、11 = 空的、10 保留。
  - push 時，每個暫存器的內容往下移一格。st7 已經有資料時再 push 就會 **stack overflow**，要避免。
  - 大部分浮點指令作用在 stack 頂端：**pop 來源運算元，push 結果**。
- §14.4 實測：C 的 `long double` 就是這個 80 位元格式，gcc 用 `fld`、`fmul`、`faddp` 這些 x87 指令操作它。

---

## 8. 記憶體：segmented memory model

[37:49]–[45:46]　課本 §5.2.3、Definition 40（p.179–181）

- ARM、SimpleRisc：記憶體是一個從 0 開始的大 byte 陣列 → **linear memory model**。
- x86：還是 Von Neumann（指令和資料在同一個記憶體），但用 **segmented memory model**：每個位址都是**從某個 segment 開頭算起的 offset**。

```
記憶體
┌─────────────┐
│             │
├─────────────┤ ← cs（code segment 的開頭）
│ 程式碼       │   指令位址 4 → cs + 4
├─────────────┤ ← ds
│ 資料（全域變數）│
├─────────────┤ ← ss
│ stack       │   （activation block：參數、return address、區域變數）
└─────────────┘
```

**6 個 segment 暫存器**（16 位元）：

| 暫存器 | segment |
|---|---|
| **cs** | code（指令） |
| **ds** | data（全域變數、常數） |
| **ss** | stack（區域變數、參數、return address） |
| **es、fs、gs** | 額外的，需要時可以定義 |

- 【口頭】ss 的 stack 是**程式語言的 stack**（第 8 支講的 activation block），和 §7 硬體的 FP stack 不一樣。
- 【口頭】可以把 data 想成全域變數，stack 想成區域變數。

**為什麼要 segmentation**（[43:25]）：

1. **安全**：電腦病毒通常想**改程式的程式碼**，讓它去做病毒要做的事。segmented 模型下，store 預設用 data segment，指令在 code segment，store 很難去改到程式碼。
   - 【口頭】「我說 x86 是 Von Neumann 不太精確：**實體上**是同一個記憶體，但**邏輯上**比較像 Harvard 架構，把程式碼和資料分開。」
2. **歷史**：早期記憶體很小的處理器，可以用這個機制在硬碟和記憶體之間搬資料，跑比較大的程式。現在不重要了。

- 影片說這些在 Chapter 10 細講，⚠️ 課本 v3.09 是 **§11.4.6**（記憶體系統那章）。
- 【課本補充】課本說的歷史原因不太一樣：早期程式的不同部分**實體上**存在不同地方（程式碼在打孔卡、資料在 DRAM），所以要分割位址空間。
- 【課本補充】**8086 的做法**：segment 暫存器存 segment 起始位址的**高 16 位**，低位假設是 0。8086 的位址是 **20 位元**：把 segment 暫存器**左移 4 位**，再加上指令給的位址，得到最後的位址（linear address）。小程式的 stack segment 和 data segment 常常是同一個。

---

## 9. 現代的 segmentation：GDT 與 segment descriptor cache

[45:46]–[54:50]　課本 Definition 41（p.181–182）

**問題**：64 位元系統的位址是 64 位元，16 位元的 segment 暫存器放不下起始位址。為了相容，又不能把 segment 暫存器加寬。

**解法**：segment 暫存器裡存的不再是位址，而是**一張大表的索引**（segment id）。

```
cs ──▶ ┌──────────────────────────┐
       │ Segment descriptor table │
       │  …                       │
       │  第 k 筆：segment 起始位址 │ ──▶ 加上 offset ──▶ 記憶體
       │  …                       │
       └──────────────────────────┘
```

- 兩種表：
  - **LDT**（local descriptor table）：每個 **process** 一張。現在通常不用，所以不討論。
  - **GDT**（global descriptor table）：**整個系統一張**，所有程式共用。
- **Process**（行程）：一個程式**正在執行的實例**。同一個程式跑兩份就是兩個 process。
- **OS**（作業系統）：一個特殊的程式，負責管理其他所有程式。segment 暫存器由 OS 設定。影片說 Chapter 10、12 會再講，⚠️ 課本 v3.09 是 Ch11、Ch13。
- **GDT 的大小**：不需要 2¹⁶ 筆，用不到那麼多 segment。只有 **8192 筆** = 1024 × 8 = **2¹³**，所以用 segment 暫存器裡的 **13 個位元**當索引。
  - 【課本補充】是 16 位元裡的第 4～16 位（從 1 算）；第一筆保留，所以最多 8191 筆。每一筆存 segment 的**起始位址、大小、存取需要的權限**。
- 不同程式的 segment 起始位址不同，因為它們共用一個大記憶體。

**每次存取記憶體要做什麼**：

1. 依定址模式算出位址（offset）
2. 讀相關的 segment 暫存器
3. 用它查 GDT，得到 segment 的起始位址
4. 起始位址 ＋ offset = 最後的位址，送到記憶體系統

**問題**：每次存取記憶體都要查 GDT，太慢了。

**解法**：每顆處理器有一個小而快的硬體結構，叫 **segment descriptor cache（SDC）**，存 GDT 裡和**目前程式**有關的幾筆。

- 我們通常只關心 3 個 segment：code、data、stack。而一顆處理器同一時間只跑一個程式。
- 先查 SDC（**快的路**）；找不到或資料過時了，才去查 GDT（**慢的路**），然後填進 SDC。
- 這樣幾乎可以**隱藏**多一層查表的代價。影片說 Chapter 9 講怎麼做到，⚠️ 課本 v3.09 是 Ch10（pipeline）。

⚠️ 影片和課本沒講的現況（§13、§14.2）：**64 位元模式下，cs、ds、es、ss 的 segment 起始位址一律當成 0**，所以 x86-64 實際上是**平坦的**（flat）記憶體模型，安全性改靠**分頁**（paging）的權限位元。真正還在用的是 **fs 和 gs**：Linux 使用者程式用 **fs** 存取 thread-local storage，kernel 用 **gs** 存取 per-CPU 資料。影片說 es、fs、gs「沒有用到」在 64 位元系統上不太對。

---

## 10. 定址模式

[54:50]–[63:11]　課本 §5.2.4、Equation 5.1、Table 5.2（p.182–184）

【口頭】x86 是 CISC，定址模式也比較複雜。「但沒那麼複雜，它有很多優雅之處。」

**一般形式**：

```
                 ┌ cs: ┐   ┌ eax ┐   ┌ eax ┐   ┌ 1 ┐
address  =       │ ds: │ + │ ebx │ + │ ebx │ × │ 2 │ + [displacement]
（可省略）        │ ss: │   │ ecx │   │ ecx │   │ 4 │    （offset，常數）
                 │ es: │   │ edx │   │ edx │   └ 8 ┘
                 │ fs: │   │ esp │   │ ebp │   scale
                 └ gs: ┘   │ ebp │   │ esi │
                 segment   │ esi │   │ edi │
                           └ edi ┘   └─────┘
                            base     index（不能是 esp）
```

- **每一項都可有可無**。
- **segment**：通常不寫，硬體用預設的（資料用 ds、stack 用 ss、程式碼用 cs）。
- **base**：8 個一般暫存器之一（64 位元模式還有更多）。
- **index**：8 個之中**除了 esp** 以外的任何一個，可以**乘上 scale：1、2、4、8**。
- **displacement**：常數 offset，32 位元模式最多 32 位元。
  - Definition 42：x86 裡記憶體運算元的固定 offset 叫做 **displacement**。

**例子**：

| 記憶體運算元 | 位址 | 定址模式 |
|---|---|---|
| `[eax]` | eax | register-indirect |
| `[eax + ecx*2]` | eax + 2 × ecx | base-scaled-index |
| `[eax + ecx*2 - 32]` | eax + 2 × ecx − 32 | base-scaled-index-offset |
| `[edx - 12]` | edx − 12 | base-offset |
| `[edx*2]` | edx × 2 | scaled-index |
| `[0xFFE13342]` | 0xFFE13342 | memory-direct |

- 【課本補充】**memory-direct** 通常只在 OS 的程式碼裡用。一般程式設計師要**避免**：我們通常不知道確切的位址（例如 stack 的起始位址是執行時才決定、每次都不同），也不可攜。

**32 位元和 64 位元的轉換**（[57:24]）：

- 本講用 32 位元的 x86 講。會寫 32 位元的，就會寫 64 位元的：把暫存器名字的 **e 換成 r** 就好（eax → rax），還有少數指令要改。16 位元則是把 e 拿掉（eax → ax）。

§14.3 實測：用 NASM 組譯這張表的每一列，看到每條指令長度從 2 到 10 byte 不等；`[eax + esp*2]` 會被拒絕。

---

## 11. x86 組合語言的格式（NASM、Intel 語法）

[63:11]–[69:55]　課本 §5.2.5（p.184–185）

- 【口頭】「讀書、看影片都很好，但除非學生**真的坐下來寫**組合語言，否則學不會。」
- 本書用 **NASM** assembler（網站 www.nasm.us，Linux 和 Mac 上都能用）。其他 x86 assembler：TASM、MASM，任何一個都可以。
  - 【課本補充】課本的程式碼都用 NASM 測試過，但盡量避免 NASM 專屬的功能；只用到「註解以 `;` 開頭」這一點。

```
<label>: <組合語言指令>  ; <註解>
```

- **註解用分號 `;`**，不是 ARM 的 `@`。
- 【課本補充】課本的 label 以 `.` 開頭，但其實也可以用一般字母開頭。

**兩種格式**：

| 格式 | 目的地在哪 | 本書 |
|---|---|---|
| **AT&T** | **最後** | 不用 |
| **Intel** | **最前** | **用這個** |

- 【口頭】一定要確認用的是 Intel 格式。（第 6 支實測過，gcc 預設是 AT&T，要加 `-masm=intel`。）

**指令格式**：大部分 x86 指令是 1-address 或 2-address。

```
instruction operand1, operand2      ; operand1 = operand1 op operand2
```

- **operand1 同時是來源和目的地**：運算的結果存回 operand1，原本的值被蓋掉。
- 【課本補充】1-address 指令（例如 `not eax`）：唯一的運算元同時是來源和目的地。0-address 指令（例如 nop）沒有運算元。

**三個基本觀念**（【口頭】「我會一直重複，讀者才不會忘記」）：

1. **暫存器**：ax、eax、rax 是**同一個硬體位置**，用不同的部分。
2. **Segmented memory**：segment 暫存器 → GDT → segment 起始位址；用 segment descriptor cache 加速。
3. **FP stack**：浮點值存在 8 格的 stack；stN 代表 stack 上第 N 個位置。

---

## 12. 運算元的規則

[69:55]–[73:16]　課本 p.185

- 兩個運算元都可以是**暫存器**。
- **最多一個是立即值**（兩個都是常數沒有意義；目的地當然不能是立即值）。
- **最多一個是記憶體位置**：不能兩個運算元都是記憶體。
  - ⚠️ 投影片這裡印錯了，老師口頭更正：「這裡應該是 M（memory）。」
- 記憶體運算元用**方括號**包起來。
- **立即值的大小 = 記憶體位址的大小**：32 位元機器的立即值最多 32 位元。
  - 【課本補充】課本：「x86 最棒的是立即值沒有大小限制，可以和暫存器一樣大。」
  - ⚠️ 在 **64 位元**模式這不成立：大部分指令的立即值**最多 32 位元**（再 sign-extend 成 64 位元）；只有 `mov r64, imm64` 能放 64 位元立即值。§14.3 實測：`add rax, 0x123456789A` 被 NASM 截成 `0x3456789A`，還印出警告。
- 本講用 32 位元的 x86；16 位元版本用 ax、bx…，64 位元版本用 rax、rbx… 和 r8～r15。有時指令的後綴也要改，遇到時再說。

---

## 13. 【原始碼】Linux 的 GDT

64 位元 Linux 的 GDT 長什麼樣？`arch/x86/include/asm/segment.h:166-201`（節錄）：

```c
#define GDT_ENTRY_KERNEL32_CS		1
#define GDT_ENTRY_KERNEL_CS		2
#define GDT_ENTRY_KERNEL_DS		3
...
#define GDT_ENTRY_DEFAULT_USER32_CS	4
#define GDT_ENTRY_DEFAULT_USER_DS	5
#define GDT_ENTRY_DEFAULT_USER_CS	6

/* Needs two entries */
#define GDT_ENTRY_TSS			8
/* Needs two entries */
#define GDT_ENTRY_LDT			10

#define GDT_ENTRY_TLS_MIN		12
#define GDT_ENTRY_TLS_MAX		14
...
#define GDT_ENTRIES			16
```

和 selector 的值，`arch/x86/include/asm/segment.h:210-214`：

```c
#define __KERNEL_CS			(GDT_ENTRY_KERNEL_CS*8)
...
#define __USER_DS			(GDT_ENTRY_DEFAULT_USER_DS*8 + 3)
#define __USER_CS			(GDT_ENTRY_DEFAULT_USER_CS*8 + 3)
```

- 64 位元 Linux 的 GDT **只有 16 筆**，遠少於 8192 筆的上限：現代系統幾乎不靠 segment 分割記憶體。
- **selector = 索引 × 8 ＋ 權限**：低 3 位是 TI（用 GDT 還是 LDT）和 **RPL**（2 位元，請求的權限等級）。所以索引在高 13 位，正是 §9 說的「用 13 個位元查 GDT」。
- 使用者的 cs = 6 × 8 + 3 = **0x33**，ss/ds = 5 × 8 + 3 = **0x2b**。最後的 3 是 **ring 3**（使用者模式）；kernel 的 cs 是 2 × 8 + 0（ring 0）。ring level 見 [`os/linux-os` 第 2 支](../../os/linux-os/lecture02_note.md)，segmentation 見 [`os/linux-os` 第 6 支](../../os/linux-os/lecture06_note.md)。

---

## 14. 【實測】

本機：Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、NASM。程式在 `experiments/lecture11/`。

### 14.1 rax、eax、ax、ah、al 是同一個暫存器

先把 rax 設成 `0x1122334455667788`，再寫不同的子暫存器（`regs.c`，inline assembly）：

```
$ ./regs
after mov al, 0xAB   : rax = 0x11223344556677ab
after mov ah, 0xCD   : rax = 0x112233445566cd88
after mov ax, 0xBEEF : rax = 0x112233445566beef
after mov eax, ...   : rax = 0x00000000deadbeef
```

- 寫 **al**：只改最低 8 位（88 → ab）。
- 寫 **ah**：只改第 8–15 位（77 → cd）。
- 寫 **ax**：只改最低 16 位。
- 寫 **eax**：高 32 位**被清成 0**！這是 x86-64 的規則（寫 32 位元暫存器會 zero-extend 到 64 位元）。寫 8／16 位元時則保留其他位元。

### 14.2 現代 Linux 上的 segment 暫存器

```
$ ./seg
cs = 0x0033  (GDT index  6, TI 0, RPL 3)
ds = 0x0000  (GDT index  0, TI 0, RPL 0)
ss = 0x002b  (GDT index  5, TI 0, RPL 3)
es = 0x0000  (GDT index  0, TI 0, RPL 0)
fs = 0x0000  (GDT index  0, TI 0, RPL 0)
gs = 0x0000  (GDT index  0, TI 0, RPL 0)
fs base = 0x7f7e0f6c0740, &tls_var = 0x7f7e0f6c073c
```

- **cs = 0x33**：GDT 第 6 筆、RPL 3，就是 §13 的 `__USER_CS`。**ss = 0x2b**：第 5 筆，就是 `__USER_DS`。
- **ds、es 是 0**：64 位元模式根本不用它們的起始位址（一律當成 0），連設定都不用。
- **fs 的 selector 也是 0**，但 fs 的**起始位址（base）**被 OS 另外設成 `0x7f7e0f6c0740`（用 `arch_prctl` 讀出來）。thread-local 變數 `tls_var` 的位址就在它旁邊：glibc 用 `fs:` 前綴存取每個 thread 自己的資料。這是 segmentation 在今天**還在用的地方**。

### 14.3 定址模式與立即值（NASM）

```
$ nasm -f elf64 -l modes.lst modes.asm
 bits 32
 00000000 8B00          mov eax, [eax]
 00000002 8B0448        mov eax, [eax + ecx*2]
 00000005 8B4448E0      mov eax, [eax + ecx*2 - 32]
 00000009 8B42F4        mov eax, [edx - 12]
 0000000C 8B0412        mov eax, [edx*2]
 0000000F A14233E1FF    mov eax, [0xFFE13342]
 00000014 648B03        mov eax, [fs:ebx]
 00000017 B878563412    mov eax, 0x12345678
 0000001C 0578563412    add eax, 0x12345678
 bits 64
 00000021 48B89A785634120000 00   mov rax, 0x123456789A
 0000002B 8B050A000000  mov eax, [rel $ + 0x10]
```

- 課本 Table 5.2 的每一種定址模式都組譯得出來。
- **指令長度從 2 byte 到 10 byte**：CISC 的「指令長度不固定」。
- `[edx*2]` 被 NASM 編成 `8B0412`，其實是 `[edx + edx]`（base-index），比真的 scaled-index（需要 4 byte 的 displacement）短。
- `[fs:ebx]` 前面多了一個 byte `64`：這是 **fs segment 前綴**。
- `0x12345678` 這個 32 位元立即值在指令裡是 `78 56 34 12`：**little endian**。
- `[rel $ + 0x10]`：64 位元的 **rip-relative** 定址。

**不合法的寫法**：

```
$ nasm bad1.asm      ; mov eax, [eax + esp*2]
bad1.asm:2: error: invalid effective address          ← esp 不能當 index
$ nasm bad3.asm      ; mov [eax], [ebx]
bad3.asm:2: error: invalid combination of opcode and operands   ← 兩個記憶體運算元
$ nasm bad2.asm      ; add rax, 0x123456789A
bad2.asm:2: warning: signed dword immediate exceeds bounds
$ objdump -d -M intel bad2.o
   0:	48 05 9a 78 56 34    	add    rax,0x3456789a      ← 被截成 32 位元
```

### 14.4 x87 的 80 位元浮點數

```
$ ./x87
sizeof(long double) = 16 bytes, mantissa = 64 bits
double mantissa = 53 bits
1/3 long double = 0.3333333333333333333423684
1/3 double      = 0.3333333333333333148296163
```

```
$ gcc -O2 -S -masm=intel x87f.c        （long double f(a, b) { return a*b + a; }）
f:
	fld	TBYTE PTR 8[rsp]      ← push a 到 FP stack（TBYTE = 10 byte = 80 位元）
	fld	TBYTE PTR 24[rsp]     ← push b；現在 st0 = b、st1 = a
	fmul	st, st(1)             ← st0 = b × a
	faddp	st(1), st             ← st1 = a + st0，然後 pop
	ret                           ← 結果留在 st0
```

- `long double` 的 mantissa 是 **64 位元**（double 是 53 位元，含隱含的 1），所以 1/3 多了大約 3 位十進位的精度。
- 它占 16 byte，但真正用到的是 **10 byte（80 位元）**，其餘是對齊用的填充。
- gcc 產生的程式碼完全是 §7 講的 **stack 操作**：`fld` push、`faddp` 加完 pop；回傳值放在 st0。

---

## 15. 一頁速查表

| 主題 | 重點 |
|---|---|
| x86 | 一個家族：8080（8）→ 8086（16，1978）→ 386/486/Pentium（32）→ x86-64（64，AMD 2003） |
| 特色 | CISC；300+ 指令；記憶體運算元；2-address；複雜定址；return address 在 stack；flags；segmented |
| 相容 | 新處理器要能跑舊 binary（市場需求） |
| 暫存器 | rax ⊃ eax ⊃ ax ⊃ ah/al；8 個 + r8–r15；寫 32 位元會清掉高 32 位 |
| 特殊用途 | sp：stack pointer；bp：base／frame pointer；si／di：來源／目的地 |
| flags | OF、CF、ZF、SF；flags／eflags／rflags |
| PC | ip／eip／rip；程式看不到；64 位元有 rip-relative |
| FP | st0–st7，80 位元（64 位元 mantissa）；排成 stack，st0 是頂端；名稱 = 位置 |
| 記憶體 | segmented：cs、ds、ss、es、fs、gs；位址 = segment 起點 ＋ offset |
| 為什麼 | 安全（store 改不到程式碼）；歷史 |
| 8086 | segment × 16 ＋ offset = 20 位元位址 |
| 現代 | segment 暫存器存 GDT 索引（13 位元，8192 筆）；SDC 加速；x86-64 的 cs/ds/ss/es base = 0，fs/gs 還在用 |
| 定址 | [seg: base + index × {1,2,4,8} + disp]；每項可省略；index ≠ esp |
| 語法 | NASM、Intel 格式（目的地在前）；註解 `;` |
| 運算元 | 最多一個立即值、最多一個記憶體運算元；op1 同時是來源和目的地 |
| 立即值 | 32 位元模式最多 32 位元；64 位元模式大多也只有 32 位元（mov r64 例外） |

---

## 16. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| mov、算術、分支、call／ret、stack 的指令 | **第 12 支**：x86 整數與分支指令 |
| si／di 與 rep、進階記憶體指令 | **第 13 支**：x86 進階記憶體指令 |
| FP stack 的指令（fld、fst、fadd…） | **第 13–14 支**：x86 浮點指令 |
| 長度不固定的編碼、prefix、ModR/M、SIB | **第 14 支**：x86 編碼 |
| Segmentation 與分頁、安全 | **第 34–37 支**：課本 Ch11 記憶體系統（§11.4.6） |
| x86 在內部翻成 RISC 式的微指令 | Advanced 課程 |
| 真實的 Intel、AMD 處理器 | 課本 Appendix A |
| Ring level、segmentation、GDT 在 OS 的用法 | [`os/linux-os` 第 2 支](../../os/linux-os/lecture02_note.md)、[第 6 支](../../os/linux-os/lecture06_note.md) |

---

## 附：本講一句話總結

> **x86 是一個家族（8 位元 8080、16 位元 8086、32 位元 386／486／Pentium、64 位元 x86-64），因為市場要求新處理器能跑幾十年前的 binary，所以它不增加暫存器而是把暫存器加寬：ax、eax、rax 是同一個暫存器的不同部分，ax 又分成 ah 和 al，64 位元多了 r8～r15（實測寫 al／ax 只改那幾位，寫 eax 卻會清掉 rax 的高 32 位）；sp 是 stack pointer、bp 是 base pointer、si／di 是預設的來源／目的地，flags（OF、CF、ZF、SF）和 ip 由系統管理。浮點暫存器是 8 個 80 位元的 st0～st7，排成一個 stack，名稱代表在 stack 上的位置（實測 long double 就是 x87 的 80 位元，gcc 用 fld／faddp 操作）。記憶體是 segmented：cs、ds、ss、es、fs、gs 指出各 segment 的起點，位址是 segment 起點加 offset，邏輯上把程式碼和資料分開以防病毒；現代的 segment 暫存器存的是 13 位元的 GDT 索引（8192 筆），用 segment descriptor cache 加速（實測 Linux 的使用者 cs = 0x33 就是 GDT 第 6 筆、ring 3，64 位元模式下 ds／es 根本是 0，只剩 fs 用來存 thread-local storage）。定址模式是 [segment: base + index × 1/2/4/8 + displacement]，每一項都可以省略，index 不能是 esp。本書用 NASM 和目的地在前的 Intel 語法，一條指令最多一個立即值、最多一個記憶體運算元；課本說「立即值沒有大小限制」在 64 位元模式不成立（實測 add rax 的立即值被截成 32 位元）。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 5: x86 assembly, Part 1/4 錄影逐字稿（YouTube 自動字幕，1:13:21）、Sarangi *Basic Computer Architecture* v3.09 Ch5 §5.1–5.2（p.175–185）整理。原始碼為 Linux v6.2.12；實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、NASM）結果。*
