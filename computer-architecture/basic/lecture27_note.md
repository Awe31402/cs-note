# Basic Computer Architecture 第二十七支影片完整筆記：單週期處理器的五個階段與 hardwired 控制單元（Chapter 8 Processor Design, Part 1/3 → 課本 Ch9）

> **課程**：Basic Computer Architecture — Chapt 8: Processor Design, Part 1/3（播放清單第 27 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（1:27:39，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 **Ch9** §9.1–9.3（p.359–379）
> **章號**：影片的 Chapter 8 是課本 v3.09 的 **Chapter 9**。影片說的「chapter 9 pipelining」「chapter 10 記憶體」分別是課本的 Ch10、Ch11。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 處理器就像汽車生產線：五個階段](#1-處理器就像汽車生產線五個階段)
- [2. Fetch unit（IF）](#2-fetch-unitif)
- [3. isBranchTaken 訊號](#3-isbranchtaken-訊號)
- [4. Data path 與 control path](#4-data-path-與-control-path)
- [5. 複習指令格式：opcode 怎麼編才好解碼](#5-複習指令格式opcode-怎麼編才好解碼)
- [6. Operand fetch：讀暫存器](#6-operand-fetch讀暫存器)
- [7. Operand fetch：immx 與 branchTarget](#7-operand-fetchimmx-與-branchtarget)
- [8. Execute：branch unit](#8-executebranch-unit)
- [9. Execute：ALU](#9-executealu)
- [10. Transmission gate：大的多工器與省電](#10-transmission-gate大的多工器與省電)
- [11. Memory access（MA）](#11-memory-accessma)
- [12. Register writeback（RW）](#12-register-writebackrw)
- [13. 整個 data path](#13-整個-data-path)
- [14. Hardwired 控制單元](#14-hardwired-控制單元)
- [15. 【實測】](#15-實測)
- [16. 一頁速查表](#16-一頁速查表)
- [17. 與後續講次的連結](#17-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

【口頭】[00:00]「終於要進入真正的計算機架構了」：從零開始，替 SimpleRisc 設計一顆**能跑完整程式的處理器**。

```
IF（instruction fetch）  pc → 指令記憶體 → inst；算 pc+4
OF（operand fetch）      解碼；讀 op1、op2；算 immx、branchTarget；送 opcode+I 給控制單元
EX（execute）            branch unit → isBranchTaken、branchPC；ALU → aluResult、flags
MA（memory access）      ld／st：mar = aluResult，mdr = op2
RW（register writeback） aluResult／ldResult／pc+4 三選一 → 寫回 rd 或 ra
控制單元（hardwired）    6 位元輸入（opcode 5 + I 1）→ 22 個控制訊號
```

本章的後面：microprogram 處理器（第 28–29 支）。

---

## 1. 處理器就像汽車生產線：五個階段

[02:32]–[12:47]　課本 §9.1（p.359–361，Fig 9.1–9.2）

目標：用電晶體、邏輯閘和第 18–20 支的數位元件，做出整個 ISA；而且越快越好（performance = 1／執行時間）。

做法：把一條指令的處理切成幾個**階段**，一段一段設計。

【口頭】[03:28] 汽車生產線：一間工廠鑄底盤、一間做引擎、一間做車身、一間做儀表板和內裝，最後組起來、上漆、測試。指令也一樣，一站做一點，再交給下一站。

| 階段 | 做什麼 |
|---|---|
| **IF** | 從指令記憶體讀指令；算下一條指令的位址 |
| **OF** | 解碼（把 32 位元拆成欄位）；讀暫存器；算 branch target；把 18 位元 immediate 展開成 32 位元 |
| **EX** | ALU 做算術與邏輯；branch unit 決定要不要跳；cmp 更新 flags |
| **MA** | 和記憶體系統溝通，做 ld 或 st |
| **RW** | 把結果寫回暫存器檔；call 要把下一條的位址寫進 ra |

【口頭】[09:57] 投影片上 ALU 那行寫錯字，「應該是 mod」。

【課本補充】p.360：
- 機器可以是 **Von Neumann**（指令和資料共用記憶體）或 **Harvard**（指令記憶體分開），fetch 不在乎是哪一種。
- SimpleRisc 格式很規律，解碼很容易；x86 的解碼單元「非常複雜」。
- 解碼通常包含：取出運算元、把 immediate 擴成 32／64 位元、產生處理器自己的訊號（例如「打開記憶體單元」「store 不准寫暫存器」）。

---

## 2. Fetch unit（IF）

[12:47]–[17:11]　課本 §9.2.1（p.361–363，Fig 9.3）

【口頭】[13:00] 怎麼看這種圖：方塊是儲存或運算單元；梯形是**多工器**（multiplexer，從幾個輸入選一個）；**虛線**是控制訊號；斜線上寫 32 表示這組線有 32 條。

```
             isBranchTaken（虛線，來自 EX）
                   │
pc+4 ──────────→ [0]
branchPC ──────→ [1] MUX ──→ pc 暫存器 ──→ 指令記憶體 ──→ inst（32 位元）
                                 │
                                 └──→ (+4) ──→ pc+4
```

- **pc** 是 32 位元暫存器，**負緣觸發**（negative edge triggered：時脈從 1 變 0 時才存新值）。【口頭】[13:17]「我們假設所有電路都是負緣觸發」。
- 指令記憶體通常是 SRAM 陣列（[第 20 支](lecture20_note.md#3-sram-陣列)）；細節到課本 Ch11（影片說「chapter 10」）。這裡先當黑盒子。
- 為什麼 +4：每條指令 4 bytes，記憶體以 byte 定址。
- 整個 fetch unit 只有「一個暫存器、一個加法器、一個多工器」。

【課本補充】Important Point 13（p.361）：大寫 **PC** 是「program counter」這個概念；小寫 **pc** 專指 data path 裡那個暫存器。同樣，**MAR／mar**、**MDR／mdr** 也這樣分。

---

## 3. isBranchTaken 訊號

[17:11]–[18:20]　課本 Table 9.1（p.362）

| 指令 | isBranchTaken |
|---|---|
| 不是分支 | 0 |
| call、ret、b | 1 |
| beq、bgt | 有跳 → 1；沒跳 → 0 |

這個訊號由 EX 階段的 **branch unit** 產生（見[第 8 節](#8-executebranch-unit)），是唯一**不是**控制單元產生的控制訊號。

---

## 4. Data path 與 control path

[18:20]–[22:24]　課本 §9.2.2（p.363–364，Definition 58，Fig 9.4）

| | 內容 |
|---|---|
| **Data path** | 儲存、讀取、處理資料的元件：暫存器檔、記憶體、ALU、pc 暫存器…… |
| **Control path** | 控制單元＋送出去的控制線（虛線），決定指令和資料怎麼在 data path 裡流動 |

【口頭】[20:03] 多工器本身算哪一邊？「看你怎麼解釋」，老師把它放在 data path，因為 data path 是電路「靜態」的部分。

比喻（影片和課本都用）：**data path 是道路網**，指令是車；**control path 是紅綠燈和控制紅綠燈的電腦**。

【口頭】[21:22]–[22:07] 現在講的是 **hardwired** control path：做在矽晶片上，出廠後不能改。Microprogram 處理器會把 control path 的一部分「開放給程式設計師」，可以改。Hardwired 是「到目前為止最常見」的做法。

【課本補充】p.364：聰明城市的紅綠燈由中控中心統一管理，可以繞開塞車和車禍；處理器的控制單元也一樣聰明，進階課程會變得很複雜。

---

## 5. 複習指令格式：opcode 怎麼編才好解碼

[22:24]–[29:39]　課本 Table 9.2–9.3（p.365）；細節見[第 8 支](lecture08_note.md#12-指令編碼opcode)

21 條指令 → 需要 5 位元 opcode。

| 格式 | 欄位（課本位元編號從 1 開始） |
|---|---|
| branch | op (28–32) ・ offset (1–27) |
| register | op (28–32) ・ I (27) = 0 ・ rd (23–26) ・ rs1 (19–22) ・ rs2 (15–18) |
| immediate | op (28–32) ・ I (27) = 1 ・ rd (23–26) ・ rs1 (19–22) ・ imm (1–18) |

- imm 的 18 位元 = 16 位元常數 + 2 位元 modifier（預設／u／h 三種）。
- offset 是 **word offset**（以 4 bytes 為單位）。
- 【口頭】[24:15] nop「什麼都不做」，它的好處要等 pipelining（第 30 支起）才看得懂。
- 【口頭】[24:50] st 的 rd 其實是**來源**，為了不多一種格式才這樣放，「會讓處理器設計複雜一點點」。

**為什麼 opcode 這樣編**（【口頭】[26:45]–[28:19]，重點是**解碼的複雜度**）：

| 群組 | opcode | 怎麼判斷 |
|---|---|---|
| 所有分支（beq bgt b call ret） | 1xxxx | 只看 MSB |
| ld、st | 0111x | 看前 4 位元 = 0111 → 一定要用 MA |

原則：**相似的指令給相似的 opcode**，解碼就快。

---

## 6. Operand fetch：讀暫存器

[29:39]–[38:51]　課本 §9.2.3（p.367–368，Fig 9.6，Definition 59）

**暫存器檔**（register file）：16 個暫存器，**2 個讀埠、1 個寫埠**。
- **埠**（port）：硬體的一個接口。讀埠輸入 A（4 位元的暫存器編號 0–15），輸出 D（32 位元內容）。兩個讀埠可以在同一個週期同時讀兩個暫存器。

```
          isRet                        isSt
ra(15) → [1]                 rd  (23–26) → [1]
rs1    → [0] MUX → 讀埠1 → op1   rs2 (15–18) → [0] MUX → 讀埠2 → op2
(19–22)
```

| 多工器 | 為什麼需要 |
|---|---|
| 讀埠 1，isRet 控制 | ret 要讀 ra（r15）的內容當跳回位址；ret 本來就不用 rs1，所以可以共用 |
| 讀埠 2，isSt 控制 | st 的兩個來源是 **rd**（要存的值）和 rs1（base），沒有 rs2 |

【口頭】[35:11] 例子：`st r1, 20[r2]` → r1（0001）放在 rd 欄位，要讀出來存進記憶體；r2 放在 rs1 欄位，加 20 算位址。

| 指令類型 | op1 | op2 |
|---|---|---|
| 一般 ALU 指令 | rs1 | rs2 |
| ret | ra（返回位址） | （沒用） |
| st | rs1（base） | rd（要存的值） |

【口頭】[36:30] 只有一個來源的指令也照讀兩個，多出來的是「垃圾值」；「為了簡單多做一點事，沒關係」，反正不會弄壞結果。

【課本補充】p.367：有人會說這樣浪費電。但要判斷「這個運算元到底要不要」需要額外電路，會讓 OF 變大、變慢，所以優先選簡單。
【課本補充】p.368：因為格式很一致，取出欄位**不需要任何邏輯閘**，把每個 bit 存進 latch、接線接對就好。isSt 只要幾個閘判斷 opcode == 01111。

---

## 7. Operand fetch：immx 與 branchTarget

[38:51]–[44:08]　課本 §9.2.3（p.366–367，Fig 9.5、Fig 9.7）

```
inst[1:18]  → calculate immediate        → immx（32 位元）
inst[1:27]  → 左移 2 位、sign extend → (+ pc) → branchTarget（32 位元）
```

**branchTarget**：27 位元 word offset → 左移 2 位變 29 位元的 byte offset → sign extend 成 32 位元 → 加上 pc。

**immx**（依 modifier）：

| modifier | 32 位元怎麼來 |
|---|---|
| 預設 | 16 位元 sign extend |
| u | 上面 16 位元補 0 |
| h | 16 位元放在上面兩個 byte，下面補 0 |

**每條指令都算這兩個**：這時還不知道它是不是分支、有沒有 immediate。用不到的那個就是垃圾值。【口頭】[40:33]「為了時間，這點浪費是值得的」。

【課本補充】p.366：每條 SimpleRisc 指令**最多只用其中一個**（branchTarget 或 immx）。ret 的目標不是 branchTarget，而是 ra 的內容，下一階段才挑。

最後，OF 把 **opcode（5 位元）＋ I 位元（1 位元）＝ 6 位元**送給控制單元。【口頭】[44:08] 假設控制單元很快產生 isRet、isSt，同一個階段就能用；其他階段的訊號可以晚一點。

---

## 8. Execute：branch unit

[44:08]–[51:56]　課本 §9.2.4（p.369–370，Fig 9.8）

```
isBeq  ─┐
flags.E ┴ AND ─┐
isBgt   ─┐      ├─ OR ─→ isBranchTaken（送回 IF）
flags.GT┴ AND ─┤
isUBranch ─────┘

branchTarget（來自 OF）→ [0]
op1（ret 的 ra 內容）  → [1] MUX(isRet) → branchPC（送回 IF）
```

- **flags 暫存器**兩個位元：**E**（上次 cmp 相等）、**GT**（上次 cmp 第一個運算元比較大）。
- **isUBranch**（unconditional branch，無條件分支）：b、call、ret。
- ⚠️ [47:55] 老師一度說成「unsigned branch」，馬上改口成 unconditional。

**branchTarget 和 branchPC 不一樣**（【口頭】[49:58]）：
- branchTarget：pc + 指令裡的 offset。beq、bgt、b、call 都是這樣。
- branchPC：**最後真的要跳去的地方**。ret 的目標在暫存器 ra 裡，所以要用 isRet 多工器二選一。

---

## 9. Execute：ALU

[51:56]–[59:02]　課本 §9.2.4（p.370–372，Fig 9.9–9.10，Definition 57）

```
op1 ──────────────→ A
op2  → [0]                ALU（aluSignals）──→ aluResult
immx → [1] MUX(isImmediate) → B
```

isImmediate 就等於指令的 I 位元。

ALU 裡面的模組（每個都有自己的開關訊號）：

| 模組 | 訊號 | 誰用 |
|---|---|---|
| 加法器 | isAdd、isSub、isCmp | add；sub（A + B 的 2 補數）；cmp（相減後設 flags）；**ld、st 算位址** |
| 乘法器 | isMul | mul |
| 除法器 | isDiv、isMod | div 取商、mod 取餘數 |
| 位移單元 | isLsl、isLsr、isAsr | A 移 B 位 |
| 邏輯單元 | isOr、isAnd、isNot | not 只用一個運算元 |
| mov 單元 | isMov | 直接讓 **B** 通過（因為來源可能是暫存器或 immediate，所以接 B） |

- 減法 = 加上 B 的 2 補數 = A + (~B) + 1（[第 4 支](lecture04_note.md)），所以共用加法器。
- cmp 就是 sub，只是**多設 flags**：A = B → E = 1；A > B → GT = 1；預設兩個都是 0。
- 最後一個多工器從 6 個模組的輸出挑一個當 aluResult。

【課本補充】p.372：課本圖上**沒畫**這個輸出多工器，ALU 的詳細電路（含 transmission gate）留作習題。

---

## 10. Transmission gate：大的多工器與省電

[59:02]–[66:42]　課本 Fig 9.11（p.371）

【口頭】[59:18] 「不用多工器的小技巧」：**transmission gate**（傳輸閘）。
- 一個 NMOS 和一個 PMOS 背對背接；NMOS 的閘接 S、PMOS 的閘接 S̄。
- S = 1：兩顆都導通，輸出 = 輸入（開關閉合）。
- S = 0：輸出和輸入斷開（開關打開，floating 浮接）。
- 很多輸入各接一個傳輸閘到同一條輸出線，只要**同時只打開一個**，輸出就等於那個輸入 → 很多輸入的多工器。【口頭】「它也有自己的問題」，細節請看電機的經典教科書。

**省電**（【口頭】[63:02]）：
- 電路只有在 0→1 或 1→0 時才有電流，電流流過電阻就發熱 → **耗電來自切換**（switching）。
- 一條指令只用 ALU 的一個模組。把沒用到的模組（例如乘法器）的輸入用傳輸閘斷開，它的輸入保持舊電壓、不切換 → 不耗電。

所以傳輸閘有兩個用途：① 隔開不用的模組來省電；② 做大的多工器。

【課本補充】p.370：「最簡單的方法是**每個輸入位元**放一個傳輸閘」；S 關掉時輸出「保持先前的值」，模組看不到新輸入，等於被關掉。

---

## 11. Memory access（MA）

[66:42]–[71:00]　課本 §9.2.5（p.372–373，Fig 9.13）

```
aluResult → mar ─┐
op2       → mdr ─┼→ 資料記憶體 ──→ ldResult
isLd、isSt ─────┘
```

| | ld rd, imm[rs1] | st rd, imm[rs1] |
|---|---|---|
| 位址 | aluResult = reg[rs1] + imm → mar | 同左 |
| 資料 | 讀 4 bytes → **ldResult** | op2（= reg[rd]，見第 6 節）→ mdr → 寫進記憶體 |
| 輸出 | ldResult | 沒有 |

isLd 和 isSt 最多一個是 1；兩個都是 0 → 不是記憶體指令，記憶體單元關掉。

【課本補充】p.373：mar、mdr 傳統上叫「暫存器」，但**不是一般的邊緣觸發暫存器**，比較像暫存區（buffer），把位址和資料擋住直到記憶體請求完成。p.375：在這個簡單處理器裡它們**其實不需要**；記憶體要好幾個週期的進階設計、以及 microprogram 處理器才需要。

---

## 12. Register writeback（RW）

[71:00]–[78:12]　課本 §9.2.6（p.374–375，Fig 9.14）

暫存器檔的**寫埠**有三個輸入：

| 輸入 | 內容 | 控制 |
|---|---|---|
| E（enable） | 要不要寫 | **isWb** |
| A（位址，4 位元） | rd（23–26）或 15（ra） | isCall 二選一 |
| D（資料，32 位元） | aluResult／ldResult／pc+4 | isLd、isCall 三選一 |

【口頭】[72:34] 為什麼讀不用 enable、寫要？讀到垃圾最糟就是不用；**寫進垃圾不行**。

寫入資料的多工器：

| isCall | isLd | 選 |
|---|---|---|
| 0 | 0 | aluResult（算術、邏輯、mov、位移） |
| 0 | 1 | ldResult |
| 1 | 0 | pc + 4（call 的返回位址） |

- call：跳去函式，最後 ret 回到 call 的下一條，所以要把 **pc + 4** 寫進 **r15（ra）**。
- 不寫暫存器的指令：st、cmp、nop、分支（call 除外）。

---

## 13. 整個 data path

[78:12]–[81:32]　課本 §9.2.7（p.375–376，Fig 9.15）

把五張圖拼起來就是一顆完整的 SimpleRisc 處理器，「夠強，能跑整個 SimpleRisc ISA」。

【口頭】[78:29] 建議讀者用 **Logisim**（免費、開源、拖拉式，適合第一次做的人）照圖畫出來；懂 Verilog／VHDL 的也可以自己寫。課本網站有 Logisim 和 VHDL 的模型。

【課本補充】p.375：
- 每個週期：從新的 pc 取指令、取運算元、執行、寫回資料記憶體和暫存器檔（**單週期**）。
- 主要的狀態元件：**pc、flags、暫存器檔**（mar、mdr 可選）。
- 圖上的線大多由上往下，只有兩個例外往上：① EX → IF 的分支目標／結果；② RW → 暫存器檔的寫回資料。
- 「處理器的魔法在 data path 和 control path 的互動」：只改控制單元，就能改指令的行為，甚至定義新指令。

---

## 14. Hardwired 控制單元

[81:32]–[87:34]　課本 §9.3（p.375–379，Table 9.4–9.5，Fig 9.16）

**輸入**：opcode（5 位元，inst[28:32]）＋ I（1 位元，inst[27]）＝ 6 位元。
**輸出**：22 個控制訊號（isBranchTaken 不算，它來自 branch unit）。

| 訊號 | 何時為 1 |
|---|---|
| isSt / isLd / isBeq / isBgt / isRet / isCall | 就是那條指令 |
| isImmediate | I 位元 = 1 |
| **isWb** | add sub mul div mod and or not mov ld lsl lsr asr **call** |
| isUBranch | b、call、ret |
| isAdd | add、**ld、st** |
| isSub isCmp isMul isDiv isMod isLsl isLsr isAsr isOr isAnd isNot isMov | 各自那條指令 |

布林式：把 opcode 叫 op₅…op₁（op₁ 是 LSB），例如 isSt = op̄₅·op₄·op₃·op₂·op₁（= 01111）。最長的是 isWb：

```
isWb = ~(op₅ + op̄₅·op₃·op₁·(op₄ + op̄₂)) + op₅·op̄₄·op̄₃·op₂·op₁
         └ 分支 ┘ └── cmp、nop、st ──┘        └─ call ─┘
```

（括號裡的意思是我自己拆的：op̄₅·op₃·op₁ 是 0x1x1 → cmp 00101、or 00111、nop 01101、st 01111；再乘 (op₄ + op̄₂) 把 or 去掉。【實測】確認和 Table 9.4 完全一致。）

【口頭】[86:51] 有了這些布林式，控制單元在 Logisim 或 HDL 裡「很容易做」。

【課本補充】p.378–379：
- isWb 最花時間，但比乘法器或 carry lookahead adder 簡單太多，所以控制單元比執行單元快很多。
- **Hardwired 控制單元快又有效率**，所以大部分商用處理器都用它。
- 缺點：**不夠彈性**。出貨後不能改指令行為、不能加新指令。例：乘法器有設計錯誤時，理論上可以改用加法器和位移單元跑 Booth 演算法（[第 22 支](lecture22_note.md#13-booth-演算法)），但這需要很複雜的控制單元。
- x86 有 `rep`（重複一條指令 N 次）和複雜的字串指令，要支援它們，可以讓控制單元裡面有個**小處理器**來產生控制訊號 → 這就是下一部分的 microprogram。

Way Point 6（p.379）：做好了一顆實作整個 SimpleRisc ISA 的 hardwired 處理器；分成 IF、OF、EX、MA、RW；data path 有狀態元件、算術邏輯單元、多工器；多工器由控制單元的控制訊號控制。

---

## 15. 【實測】

程式在 `experiments/lecture27/`。機器碼由第 8 支的組譯器 `experiments/simplerisc/srasm.py` 產生。

### 15.1 照 Fig 9.15 寫一顆單週期處理器

`datapath.py`：每個週期依序做 IF → OF → EX → MA → RW，只用本講出現的訊號（op1、op2、immx、branchTarget、branchPC、isBranchTaken、aluResult、ldResult、result）。控制單元直接抄 Table 9.5 的布林式（`experiments/lecture27/datapath.py:15-22`）：

```python
    c = dict(
        isSt=n(o5)&o4&o3&o2&o1, isLd=n(o5)&o4&o3&o2&n(o1),
        isBeq=o5&n(o4)&n(o3)&n(o2)&n(o1), isBgt=o5&n(o4)&n(o3)&n(o2)&o1,
        isRet=o5&n(o4)&o3&n(o2)&n(o1), isImmediate=I,
        isWb=n(o5 | (n(o5)&o3&o1&(o4|n(o2)))) | (o5&n(o4)&n(o3)&o2&o1),
        isUBranch=o5&n(o4)&((n(o3)&o2) | (o3&n(o2)&n(o1))),
        isCall=o5&n(o4)&n(o3)&o2&o1,
        isAdd=(n(o5)&n(o4)&n(o3)&n(o2)&n(o1)) | (n(o5)&o4&o3&o2))
```

把第 7、8 支的程式在「這顆處理器」和「第 7 支的直譯器 `simplerisc.py`」上各跑一次：

```
$ python3 -I lecture27/datapath.py simplerisc/l08/rfact.s
rfact.s: 125 cycles; r0=10 r1=3628800 | E=1 GT=0 | sp=0x10000
$ python3 -I simplerisc/simplerisc.py simplerisc/l08/rfact.s
simplerisc/l08/rfact.s: 125 steps; r0=10 r1=3628800 | E=1 GT=0 | sp=0x10000 min_sp=0xFFB8
```

| 程式 | 處理器（週期） | 直譯器（步數） | 暫存器、flags |
|---|---|---|---|
| l07/cmp35、cmp53、cmp55 | 3 | 3 | 一致 |
| l07/ex28 | 5 | 5 | 一致 |
| l07/fact.s（r0=10） | 38 | 38 | r1=3628800，一致 |
| l07/lcm.s（r1=12 r2=18） | 18 | 18 | r0=36，一致 |
| l07/modif.s（u、h modifier） | 7 | 7 | 一致 |
| l07/prime、shift | 6、8 | 6、8 | 一致 |
| l08/foo.s | 7 | 7 | 一致 |
| l08/rfact.s（遞迴、call、ret、ld、st） | 125 | 125 | r1=3628800，一致 |

**全部一致**。單週期處理器每個週期剛好做完一條指令，所以「週期數 = 指令數」。

### 15.2 布林式和文字條件逐條比對

`check_ctrl.py` 把 21 個 opcode × I = 0／1 全部代進 Table 9.5 的布林式，和 Table 9.4 的文字條件比：

```
mismatches: 0
00000 add   isWb isAdd
00101 cmp   isCmp
01101 nop   （全部是 0）
01110 ld    isLd isWb isAdd
01111 st    isSt isAdd
10011 call  isWb isUBranch isCall
10100 ret   isRet isUBranch
...
10101 未定義 （全部是 0）   ← 10101–11111 全部
```

- 兩張表完全一致。
- nop 的 22 個訊號**全是 0**：什麼都不寫、不跳、不碰記憶體。
- 沒用到的 opcode 10101–11111 也全是 0，在這顆處理器上**等於 nop**（課本沒提，這是我的觀察）。

### 15.3 「每條指令都算一堆用不到的東西」

`trace` 模式印出每個週期的中間值（rfact.s 開頭幾條）：

```
pc=  0 inst=90000011 op1=0 op2=0 immx=17 aluResult=0 isBranchTaken=1 branchPC=68 | isUBranch
pc= 68 inst=4C00000A op1=0 op2=0 immx=10 aluResult=10 isBranchTaken=0 branchPC=4026531948 | isImmediate isWb isMov
pc= 72 inst=9FFFFFEF op1=0 op2=0 immx=-17 aluResult=0 isBranchTaken=1 branchPC=4 | isImmediate isWb isUBranch isCall
pc= 24 inst=7C380000 op1=65528 op2=10 immx=0 aluResult=65528 isBranchTaken=0 branchPC=4041211928 | isSt isImmediate isAdd
```

- `mov r0, 10`（pc=68）也算出了 branchPC = 4026531948，完全是垃圾，但 isBranchTaken = 0 所以沒用到。
- `b .main`（pc=0）也算出了 immx = 17，也是垃圾。
- `call`（pc=72）的 **isImmediate = 1**：branch 格式沒有 I 位元，bit 27 其實是 offset 的最高位（offset = −17，是負的）。控制單元照樣把它當 I。沒關係，因為 call 寫回的是 pc+4，不看 ALU。
- `st r0, [sp]`（pc=24）：op2 = 10 = r0 的值（isSt 讓讀埠 2 讀 rd），aluResult = sp = 65528 是位址。

這就是第 6、7 節說的「多做一點、用多工器挑對的」。

---

## 16. 一頁速查表

| 主題 | 重點 |
|---|---|
| 五個階段 | IF → OF → EX → MA → RW |
| IF | pc（負緣觸發）→ 指令記憶體；MUX(isBranchTaken) 選 pc+4 或 branchPC |
| isBranchTaken | (isBeq·E) + (isBgt·GT) + isUBranch；唯一不是控制單元產生的訊號 |
| Data path | 暫存器檔、記憶體、ALU、pc（道路） |
| Control path | 控制單元＋控制線（紅綠燈） |
| Hardwired | 控制做死在矽上，快，最常見；不能改 |
| opcode 設計 | 分支 1xxxx；ld/st 0111x；相似指令給相似 opcode |
| 暫存器檔 | 16 個、2 讀埠、1 寫埠 |
| 讀埠 1 | MUX(isRet)：rs1 或 ra |
| 讀埠 2 | MUX(isSt)：rs2 或 rd |
| immx | 預設 sign extend；u 補 0；h 放上半 |
| branchTarget | pc + signext(offset << 2) |
| branchPC | MUX(isRet)：branchTarget 或 op1 |
| ALU B 輸入 | MUX(isImmediate)：op2 或 immx |
| ALU 模組 | 加法器（add sub cmp ld st）、乘、除（div mod）、位移、邏輯、mov（B 直通） |
| Transmission gate | NMOS＋PMOS；做大 MUX；斷開不用的模組來省電（耗電來自切換） |
| MA | mar = aluResult；mdr = op2；isLd／isSt |
| RW 資料 | 00 aluResult、01 ldResult、10 pc+4（isCall, isLd） |
| RW 位址 | MUX(isCall)：rd 或 15 |
| isWb | 除了 cmp、nop、st、分支以外都寫；call 也寫 |
| 控制單元 | 6 位元進 → 22 個訊號出 |
| 實測 | 單週期模型和直譯器在 11 個程式上完全一致；Table 9.5 = Table 9.4 |

---

## 17. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| hardwired 不夠彈性 → microprogram、microassembly | **第 28–29 支**：Processor Design Part 2–3（課本 §9.4–9.8） |
| mar、mdr、shared bus | **第 28 支**：microprogrammed data path |
| 五個階段同時處理不同指令、nop 的用處 | **第 30–33 支**：Pipelining（課本 Ch10） |
| 指令記憶體、資料記憶體的真面目 | **第 34–37 支**：The Memory System（課本 Ch11） |
| ALU 裡的加法器、乘法器、除法器 | [第 21 支](lecture21_note.md)、[第 22 支](lecture22_note.md)、[第 23 支](lecture23_note.md)、第 24 支 |
| SimpleRisc 指令與編碼 | [第 7 支](lecture07_note.md)、[第 8 支](lecture08_note.md#17-格式總整理) |

---

## 附：本講一句話總結

> **一顆 SimpleRisc 處理器可以切成五個階段：IF 用 pc 讀指令並在 pc+4 和 branchPC 之間二選一；OF 解碼、用 isRet／isSt 多工器決定兩個讀埠讀 rs1/ra 和 rs2/rd、並且不管需不需要都把 immx 和 branchTarget 算好；EX 的 branch unit 用 (isBeq·E)+(isBgt·GT)+isUBranch 決定要不要跳、用 isRet 選出 branchPC，ALU 用 isImmediate 選第二個運算元、由一組 aluSignals 打開加法器、乘法器、除法器、位移、邏輯或 mov 其中一個（transmission gate 可以做大多工器並斷開不用的模組省電）；MA 用 mar＝aluResult、mdr＝op2 做 ld／st；RW 依 isLd、isCall 在 aluResult、ldResult、pc+4 三選一，寫到 rd 或 ra，isWb 決定要不要寫。資料流動的元件叫 data path（道路），產生控制訊號的叫 control path（紅綠燈）；hardwired 控制單元把 6 位元（opcode＋I）用布林式變成 22 個訊號，快、最常見，但出廠後不能改，這就是下一講 microprogram 的動機。實測照課本圖寫的單週期模型和第 7 支的直譯器在 11 個程式上結果、週期數完全一致，Table 9.5 的布林式和 Table 9.4 的文字條件也完全吻合。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 8: Processor Design, Part 1/3 錄影逐字稿（YouTube 自動字幕，1:27:39）、Sarangi *Basic Computer Architecture* v3.09 Ch9 §9.1–9.3（p.359–379）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
