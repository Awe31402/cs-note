# Basic Computer Architecture 第二十八支影片完整筆記：Microprogrammed 處理器：微暫存器、shared bus、8 條微指令與 ALU／ld／st 的微碼（Chapter 8, Part 2/3 → 課本 Ch9）

> **課程**：Basic Computer Architecture — Chapt 8: Processor Design, Part 2/3（播放清單第 28 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕一直下載不到）（1:15:53）＋ 課本 *Basic Computer Architecture* v3.09 **Ch9** §9.4–9.6.7（p.379–396）
> **章號**：影片的 Chapter 8 是課本 v3.09 的 **Chapter 9**。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有；microcode 更新見 [第 29 支 §12](lecture29_note.md#12-原始碼linux-的-microcode-更新)）
> - `【實測】`：在本機（Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 在開頭 [00:00] 產生了一句無關的字（「Satsang with Mooji」），而且 **[72:24] 之後（store 指令的後半）幾乎全是重複的單字**，那一段照課本 §9.6.7 補上。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Hardwired vs microprogrammed](#1-hardwired-vs-microprogrammed)
- [2. Firmware](#2-firmware)
- [3. 基本想法：一條指令 = 一段微程式](#3-基本想法一條指令--一段微程式)
- [4. Fetch 與 decode：IR 和 decode 暫存器](#4-fetch-與-decodeir-和-decode-暫存器)
- [5. Shared bus](#5-shared-bus)
- [6. Register file：regSrc、regData、regVal](#6-register-fileregsrcregdataregval)
- [7. ALU：A、B、aluResult、flags](#7-aluabaluresultflags)
- [8. 記憶體單元：mar、mdr、ldResult](#8-記憶體單元marmdrldresult)
- [9. 整個 microprogrammed data path](#9-整個-microprogrammed-data-path)
- [10. 19 個微暫存器](#10-19-個微暫存器)
- [11. 8 條微指令](#11-8-條微指令)
- [12. Preamble](#12-preamble)
- [13. 3-address 的 ALU 指令](#13-3-address-的-alu-指令)
- [14. 一邊寫入、一邊使用：省一個 cycle](#14-一邊寫入一邊使用省一個-cycle)
- [15. mov 與 not](#15-mov-與-not)
- [16. cmp 與 nop](#16-cmp-與-nop)
- [17. ld 與 st](#17-ld-與-st)
- [18. 【實測】](#18-實測)
- [19. 一頁速查表](#19-一頁速查表)
- [20. 與後續講次的連結](#20-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

上一講（[第 27 支](lecture27_note.md)）是 **hardwired** 處理器：控制單元直接產生所有控制訊號。這一講是另一種：**microprogrammed（微程式）處理器**。下一講（[第 29 支](lecture29_note.md)）講完 branch、call、ret 的微碼，以及 vertical／horizontal 編碼。

```
把處理器內部的狀態元件（微暫存器）掛在一條 shared bus 上
  → 每條 SimpleRisc 指令 = 一段微程式（microprogram）
  → 8 條微指令：mloadIR、mdecode、mswitch、mmov、mmovi、madd、mbeq、mb
  → 每段微程式 = 共同的 preamble ＋ 自己的部分
```

---

## 1. Hardwired vs microprogrammed

[00:00] Hardwired 處理器的「大腦」是控制單元，它產生控制資料流動的所有訊號，不同指令產生不同訊號。這種處理器很常見，是**標準做法**。

[01:04] 但另一種處理器**彈性大得多**。【口頭】現代的處理器（例如 Intel 系列）其實兩種都有：大部分是 hardwired，一些**非常複雜的指令**用 microprogrammed 的部分實作。

[01:32] 微程式的想法：把處理器的**內部元件**（原本藏起來的）**露給軟體**。這樣就可以「寫」一條指令：

- 改 `add` 的行為，例如變成加三個數。
- 加入很複雜的新指令，例如「(r1 + r2) × 37」。

[03:02] 好處：

1. 實作複雜的指令。
2. **修正**現有指令實作的 bug。
3. 依應用的需要**改變**指令的行為。

[03:32] 代價：**速度**。Hardwired 快得多，microprogrammed 比較慢、比較沒效率。

【課本補充】課本 §9.4（p.379–380）Definition 60 和四個理由：

| 理由 | 例子 |
|---|---|
| 修 bug | Intel Pentium 的除法 bug，Intel 不得不把賣出去的 Pentium **全部回收**；能改除法的實作就不用回收 |
| 複雜指令 | Pentium 4、Core i3、i7 用存在記憶體裡的微指令實作一些複雜指令，例如字串操作、重複的計算 |
| SoC | 手機晶片上有處理器、影像、相機、音效、網路……跟周邊溝通的指令用微碼，依產品客製化 |
| 自我檢測 | built-in-self-test（BIST）的診斷程式用微碼寫，可以調整要檢查多少元件 |

課本 p.379 也說：hardwired 的代價是**彈性**。要加新指令就要加多工器和控制訊號，而且處理器賣出去之後就不能再加。

---

## 2. Firmware

[03:32] 微程式跟一般寫程式不同：是在寫**每一條指令**的程式碼，所以叫 micro-programming。處理器裡所有的狀態元件都露給系統軟體。

[04:06] 這叫 **firmware（韌體）**：

```
一般軟體
─────────  ← firmware：一層很薄的軟體
硬體
```

- [04:42] Firmware 還是軟體，但存在硬體裡**特定的位置**，用來非常細緻地控制硬體，比一般軟體看得到更多硬體細節。
- [05:10] 很多東西都有 firmware：硬碟、網路卡、路由器……

---

## 3. 基本想法：一條指令 = 一段微程式

[05:34] 把一條 SimpleRisc 指令（add、sub、mul…）拆成好幾條**微指令（microinstruction）**，大約一個 cycle 一條。

- 一條 `add` 換成一段**微程式（microprogram）**，從頭跑到尾，這條指令就做完了。
- [06:30] 需要一個 **micro PC（µpc）**和一個**微指令記憶體**，就像一般的 PC 和指令記憶體。

---

## 4. Fetch 與 decode：IR 和 decode 暫存器

[07:04] **Fetch unit**：還是用 PC 去讀指令記憶體，把 SimpleRisc 指令的內容放進 **IR**（instruction register）微暫存器。

[08:01] **Decode unit**：從 IR 取出**所有**欄位。有些欄位對某些指令沒意義，裡面是垃圾，但照樣全部取出來：

| 微暫存器 | 內容 |
|---|---|
| **I** | immediate bit |
| **rd** | 目的暫存器的編號 |
| **rs1**、**rs2** | 第一、第二個來源暫存器的編號 |
| **immx** | 32 位元的 immediate：從 16 位元 ＋ 2 位元 modifier 展開（[第 8 支](lecture08_note.md)） |
| **branchTarget** | 指令裡的 offset ＋ 目前的 PC |

[09:33] 這時還不知道指令是什麼類型，所以**全部先算好**：不是 branch 的指令，branchTarget 就是垃圾。為了效能，事先算好比較好。

---

## 5. Shared bus

[09:58] 所有微暫存器都接在一條 **shared bus** 上：一組銅線。

- [10:21] 預設每個暫存器和 bus 之間的連線是**斷開的**，不然大家同時把資料倒上去就亂了。
- [10:42] 要讀某個暫存器（例如 immx），送一個命令，immx 到 bus 的連線就接通，bus 上出現 immx 的值，別人就能讀；讀完連線再斷開。

【課本補充】課本 Definition 62（p.381）：bus 是連接所有功能單元的一組共同的線，**任何時間只能有一個寫者、可以有多個讀者**。Hardwired 的多工器連線是固定的，例如不可能把記憶體單元的輸出送到 execute 單元的輸入；bus 讓**任何單元都能送資料給任何單元**。

---

## 6. Register file：regSrc、regData、regVal

[11:11] Register file 本身跟上一講一樣，外面加三個微暫存器（老師說：之後新加的結構都會叫「微暫存器」）：

| 微暫存器 | 用途 |
|---|---|
| **regSrc** | 要存取的暫存器編號（4 位元，因為有 16 個暫存器） |
| **regData** | 要**寫入**的值（從 bus 來） |
| **regVal** | **讀出**的值（之後可以放到 bus 上給別人） |
| args | 從 bus 來的命令：`<read>` 或 `<write>` |

- 寫：值從 bus 進 regData，再寫進 register file。
- 讀：regData 不管；args 是 read，編號在 regSrc，讀出來放在 regVal。

⚠️ [13:52] 老師更正：投影片上說 register file 在 EX stage，**錯了**，它還是 **OF**（operand fetch）stage 的一部分。

---

## 7. ALU：A、B、aluResult、flags

[14:13] EX stage 的主角是 ALU。投影片用顏色區分：粉紅色是上一講 hardwired 處理器原有的，淺棕色是新加的。

| 微暫存器 | 用途 |
|---|---|
| **A** | 第一個運算元（32 位元） |
| **B** | 第二個運算元（32 位元） |
| args | 要做哪種運算：add、sub、mul…（⚠️ 投影片上這個欄位的說明寫錯了，老師口頭更正為「ALU 運算的種類」） |
| **aluResult** | ALU 的結果；一直保存到被覆蓋，需要時放到 bus 上 |
| **flags.E**、**flags.GT** | 比較的結果：相等、大於；**只有 cmp 會改** |

[16:03] 不是所有單元一直都在工作，有方法只啟動需要的那些。

---

## 8. 記憶體單元：mar、mdr、ldResult

[17:52] 同樣的想法，資料記憶體外面加：

| 微暫存器 | 用途 |
|---|---|
| **mar** | memory address register：位址 |
| **mdr** | memory data register：要 **store** 的資料 |
| **ldResult** | **load** 出來的值（像 aluResult） |
| args | `<load>` 或 `<store>` |

- Store：位址進 mar、資料進 mdr，送 store 命令。
- Load：mdr 不用；位址進 mar，結果放在 ldResult。

---

## 9. 整個 microprogrammed data path

[20:32] 把前面的小圖合成一張大圖：

```
PC ──→ 指令記憶體 ──→ IR ──→ decode ──→ I、rd、rs1、rs2、immx、branchTarget
                              │
                              └─ opcode ──→ µcontrol unit（決定跑哪一段微程式）

═══════════════════════ shared bus ═══════════════════════
  │            │                │                │
 PC   regSrc／regData／regVal   A／B／aluResult／flags   mar／mdr／ldResult
        register file              ALU                 資料記憶體
```

- [21:01] PC 也要接到 shared bus，可以讀也可以寫（這在前面的投影片沒提）。
- [21:25] opcode 送到 **µcontrol unit**：每條指令有自己的微程式，由它決定跑哪一段。
- [22:25] 【口頭】好處：處理器賣出去、裝進系統以後，還能在 firmware 層改 `add` 的運作方式。「在很多情況下這很酷。」
- [22:48] 原有的主要單元（register file、ALU、資料記憶體）跟 shared bus 之間都夾一層微暫存器：**緩衝輸入、保存輸出**，直到下次存取才覆蓋。控制靠 bus 上的 args。

---

## 10. 19 個微暫存器

[23:52] 課本 Table 9.7（p.387）：

| # | 名稱 | 位元 | # | 名稱 | 位元 |
|---|---|---|---|---|---|
| 1 | pc | 32 | 11 | regVal | 32 |
| 2 | ir | 32 | 12 | A | 32 |
| 3 | I | 1 | 13 | B | 32 |
| 4 | rd | 4 | 14 | flags.E | 1 |
| 5 | rs1 | 4 | 15 | flags.GT | 1 |
| 6 | rs2 | 4 | 16 | aluResult | 32 |
| 7 | immx | 32 | 17 | mar | 32 |
| 8 | branchTarget | 32 | 18 | mdr | 32 |
| 9 | regSrc | 4 | 19 | ldResult | 32 |
| 10 | regData | 32 | | | |

【口頭】[26:15] 老師說之後「微暫存器」和「暫存器」會混用，看上下文。

---

## 11. 8 條微指令

[25:30] 慣例：所有微指令都用 **m** 開頭（micro），跟一般的程式指令分開。只介紹**需要的**那幾條（課本 Table 9.8，p.390）：

| 微指令 | 意思 |
|---|---|
| `mloadIR` | 把 pc 指的指令讀進 ir |
| `mdecode` | 等 1 個 cycle，讓所有 decode 暫存器填好 |
| `mswitch` | 跳到這條 SimpleRisc 指令的微程式的第一條 |
| `mmov r1, r2 [, <args>]` | r1 ← r2；有 args 就把 args 送上 bus |
| `mmovi r1, imm [, <args>]` | r1 ← imm；有 args 就送上 bus |
| `madd r1, imm [, <args>]` | r1 ← r1 + imm（像 x86，目的和來源共用）；有 args 就送上 bus |
| `mbeq r1, imm, label` | r1 = imm 就把 µpc 設成 label 的位址 |
| `mb label` | 無條件把 µpc 設成 label |

- [27:43] `mswitch`：decode 後知道是 add，就跳到 add 那段微程式（老師說「載入那組微指令」）。
- [33:42] **args 誰處理？** args 送上 bus，理論上大家都看得到；但由**擁有那個目的微暫存器的單元**去處理。例如寫到 aluResult 相關的就是 ALU 處理（老師的例子），寫到 regSrc 就是 register file 處理。
- ⚠️ [34:43] 老師說投影片上 `mbeq` 的說明寫「address」，應該寫「label」。

【課本補充】課本 p.389：`madd` 的 immediate 限制在 12 位元（−2048 到 +2047）。`mb`、`mbeq` 用 **PC-direct**（直接寫位址）而不是 PC-relative，因為微指令總數很少，而且這樣不用多一個加法器。

---

## 12. Preamble

[35:12] 每段微程式開頭都有一樣的幾條，叫 **preamble**（課本 Definition 64，p.390）：

```
.begin:
mloadIR          # 讀目前 PC 的指令
mdecode          # 解碼
madd pc, 4       # pc ← pc + 4，下次讀到下一條 SimpleRisc 指令
mswitch          # 跳到這條指令自己的微程式
```

[36:44] `mswitch` 做兩件事：(1) 依 SimpleRisc 指令找出要跑哪段微程式；(2) 把 µpc 移到那段的第一條。例如 add 有一段、sub 有另一段。

[37:20] 所有指令都先跑這 4 條，之後才不一樣；做完都跳回 `.begin`。

---

## 13. 3-address 的 ALU 指令

[38:00] 例如 `add r1, r2, r3`（r1 ← r2 + r3）。步驟：讀來源 → 做 ALU 運算 → 寫回。課本 p.392：

```
mmov regSrc, rs1, <read>     # 第一個運算元的編號進 regSrc，同時叫 register file 讀
mmov A, regVal               # 讀出來的值進 ALU 的 A
mbeq I, 1, .imm              # 第二個運算元是 immediate 嗎？
mmov regSrc, rs2, <read>     # 不是：讀 rs2
mmov B, regVal, <aluop>      # 值進 B，同時叫 ALU 做運算
mb .rw
.imm:
mmov B, immx, <aluop>        # 是：immx 進 B，同時叫 ALU 做運算
.rw:
mmov regSrc, rd              # 目的暫存器的編號（沒有 args：不存取 register file）
mmov regData, aluResult, <write>   # 結果進 regData，同時叫 register file 寫
mb .begin
```

[39:36] 第一行同時做兩件事：

1. rs1 的內容（例如是 r2 的編號）放上 bus，寫進 **regSrc**。
2. 同時 bus 上有 `<read>` 命令，register file 看到是給自己的，就啟動，**拿 bus 上的編號**去讀，結果在這個 cycle 結束時放進 **regVal**。

[43:05] 下一個 cycle 把 regVal 搬到 A。這次沒有 args，所以沒有單元要做事。

[44:19] immediate 的分支：immx 寫進 B，**同時**送 ALU 運算（add、sub、mul…）。A 已經填好了，ALU 一看到命令就開始算。

[47:41] 寫回：第一個 cycle 把 rd 放進 regSrc（不送命令，只填好編號）；第二個 cycle 把 aluResult 放進 regData，**同時**送 `<write>`。

[50:16] 【口頭】「大部分只是把東西搬來搬去。」

---

## 14. 一邊寫入、一邊使用：省一個 cycle

[50:16] 重要的地方是像 `mmov regSrc, rs2, <read>` 這種指令：**同一個 cycle** 裡，一邊把 rs2 寫進 regSrc，一邊用它去讀 register file。

- [51:28] 要做到這樣，需要一個很小的 **forwarding 電路**：regSrc 正在被寫的值，直接繞過去給 register file 用。老師說這是很簡單的細節，略過。
- [51:43] 這樣省了一個 cycle，「有趣、整齊、優雅、有效率的最佳化」。
- [52:01] ALU 也一樣：`mmov B, regVal, <aluop>` 同時寫 B 和啟動 ALU。

【課本補充】課本 p.392：這一點一開始會讓人困惑，讀者要多讀幾次這個例子。

---

## 15. mov 與 not

### 15.1 mov

[52:24] 比 3-address 簡單（課本 p.393）：

```
mbeq I, 1, .imm
mmov regSrc, rs2, <read>     # 來源是暫存器：讀出來
mmov regData, regVal         # 放進 regData
mb .rw
.imm:
mmov regData, immx           # 來源是 immediate：直接放進 regData
.rw:
mmov regSrc, rd, <write>     # 目的編號進 regSrc，同時寫（regData 已經準備好了）
mb .begin
```

[54:42] ⚠️ 老師講到 `.rw` 時先說「regSrc 已經有對的編號了」，馬上改口「還沒，還沒」：要先把 rd 放進 regSrc。

[55:23] 因為 regData 事先就準備好了，寫 regSrc 的同時就能送 `<write>`。

### 15.2 not

[56:31] 2-address 格式（課本 p.394）。⚠️ 投影片標題印錯，老師說應該是 **NOT**：

```
mbeq I, 1, .imm
mmov regSrc, rs2, <read>
mmov B, regVal, <not>        # 單運算元的 ALU 運算：寫 B 的同時就啟動
mb .rw
.imm:
mmov B, immx, <not>
.rw:
mmov regData, aluResult
mmov regSrc, rd, <write>
mb .begin
```

[57:23] 為什麼放 **B**？第 3 章（[第 7 支](lecture07_note.md)）的 not 一直用第二個運算元當來源，這裡照舊。

---

## 16. cmp 與 nop

### 16.1 cmp

[61:20] cmp 沒有目的暫存器、有兩個來源：第一個一定是 rs1，第二個是 rs2 或 immediate（課本 p.394）：

```
mmov regSrc, rs1, <read>
mmov A, regVal
mbeq I, 1, .imm
mmov regSrc, rs2, <read>
mmov B, regVal, <cmp>
mb .begin
.imm:
mmov B, immx, <cmp>
mb .begin
```

[64:32] cmp 的 aluResult **什麼都沒有**；結果在 flags.E、flags.GT，之後的 branch 再讀。cmp 不寫任何暫存器，「沒有暫存器的副作用」，只改 flags。

### 16.2 nop

[65:17] 「不能再簡單了，你們會很高興看到這段程式碼」：

```
mb .begin
```

什麼都不做，直接跳回 preamble 讀下一條指令。

---

## 17. ld 與 st

### 17.1 ld

[65:45] `ld rd, imm[rs1]`：rs1 是 base，immediate 是 offset。⚠️ 老師說投影片上有一個「ID」應該是「LD」，他記下頁碼要改。課本 p.395：

```
mmov regSrc, rs1, <read>     # 讀 base 暫存器
mmov A, regVal
mmov B, immx, <add>          # offset 進 B，同時叫 ALU 加：算出位址
mmov mar, aluResult, <load>  # 位址進 mar，同時叫記憶體 load
mmov regData, ldResult       # 讀到的值
mmov regSrc, rd, <write>     # 寫進 rd
mb .begin
```

[68:13] ALU 算出的位址經 shared bus 進 **mar**，同時送 `<load>`；mdr 這時用不到。一個 cycle 後結果在 **ldResult**。

### 17.2 st

[71:38] 「對稱性說，做了 load 就要做 store。」store 是特例：**沒有目的暫存器、有兩個來源**：一個是位址的 base、一個是要存的資料（放在 rd 欄位裡）。課本 p.396：

```
mmov regSrc, rs1, <read>
mmov A, regVal
mmov B, immx, <add>          # 算位址
mmov mar, aluResult          # 位址先放進 mar（還不送命令）
mmov regSrc, rd, <read>      # 讀要存的資料（編號在 rd）
mmov mdr, regVal, <store>    # 資料進 mdr，同時叫記憶體 store
mb .begin
```

⚠️ [72:24] 之後 whisper 的轉錄壞掉（一直重複「to」），這一段的內容照課本補上；影片最後 [75:38] 說的「完成後跳回 preamble」和課本一致。

---

## 18. 【實測】

`experiments/lecture28/microsim28.py`：把課本 §9.6.3–9.6.7 的微碼**一字不改**放進 19 個微暫存器、shared bus（args 由目的暫存器的擁有者處理）的模型，再加上第 29 支的 `b`、`beq`。跑一個 SimpleRisc 小程式：把 1 + 2 + … + 10 的每一步部分和存進記憶體 200、204…，最後用 `ld` 讀回最後一個、再 `not` 一次。

```
暫存器： {'r1': 55, 'r2': 11, 'r4': 240, 'r5': 55, 'r6': -56}
記憶體 200–236： [1, 3, 6, 10, 15, 21, 28, 36, 45, 55]
每條指令幾條微指令（含 4 條 preamble）：
  add （imm）  11.0   × 20 次
  add （reg）  13.0   × 10 次
  b           6.0   × 9 次
  beq         6.1   × 10 次
  cmp （imm）   9.0   × 10 次
  ld         11.0   × 1 次
  mov （imm）   8.0   × 3 次
  nop         5.0   × 1 次
  not （reg）  11.0   × 1 次
  st         11.0   × 10 次
總共 75 條 SimpleRisc 指令、716 條微指令，平均每條 9.5 條
```

- 結果全對：sum = 55、每一步的部分和都存進記憶體、`ld` 讀回 55、`not 55` = −56。課本的微碼可以直接組成一個會動的處理器。
- 平均每條 SimpleRisc 指令要 **9.5 條微指令**，也就是大約 9.5 個 cycle（一個 cycle 一條）。hardwired 處理器一條指令一個（長的）cycle。這就是 [§1](#1-hardwired-vs-microprogrammed) 說的代價。
- ⚠️ **3-address 指令幾條微指令？** 課本 p.393 說「這段程式有 10 條微指令，加上 preamble 的 4 條，一條指令要執行 14 條微指令」。10 是把**兩個分支**都算進去的程式碼行數；實際執行時只走其中一邊：暫存器版走 9 條（+ 4 = **13**）、immediate 版走 7 條（+ 4 = **11**），實測正是如此。
- 最便宜的是 `nop`（5 條：preamble ＋ `mb .begin`），最貴的是暫存器版的 `add`（13 條）。

---

## 19. 一頁速查表

| 主題 | 重點 |
|---|---|
| Hardwired | 快、標準；加新指令要加多工器和控制訊號 |
| Microprogrammed | 一條指令 = 一段微程式；能加指令、修 bug、改行為；慢 |
| 例子 | Pentium 除法 bug 回收；Intel 複雜指令用微碼；SoC；BIST |
| Firmware | 硬體和軟體之間的薄層；存在硬體的特定位置 |
| Decode 暫存器 | I、rd、rs1、rs2、immx、branchTarget（先全部算好） |
| Shared bus | 一組銅線；一個寫者、多個讀者；預設斷開 |
| Register file | regSrc（編號）、regData（要寫的）、regVal（讀出的）；在 OF stage |
| ALU | A、B、aluResult、flags.E、flags.GT |
| 記憶體 | mar、mdr、ldResult |
| 微暫存器 | 19 個 |
| 8 條微指令 | mloadIR、mdecode、mswitch、mmov、mmovi、madd、mbeq、mb |
| args | 送上 bus，由目的暫存器的擁有者處理 |
| Preamble | mloadIR、mdecode、madd pc, 4、mswitch |
| 3-address ALU | 讀 rs1 → A；I? immx : 讀 rs2 → B ＋ aluop；rd → regSrc；aluResult → regData ＋ write |
| 省一個 cycle | 寫 regSrc 的同時讀 register file（要小小的 forwarding） |
| mov／not | 不用 rs1；not 用 B |
| cmp | 只改 flags |
| nop | `mb .begin` |
| ld | 算位址 → mar ＋ load → ldResult → regData → rd ＋ write |
| st | 算位址 → mar；讀 rd → mdr ＋ store |
| 實測 | 課本微碼照抄就能跑；平均 9.5 條微指令／指令；3-address 實際 11 或 13 條（課本說 14） |

---

## 20. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Hardwired 處理器、data path、控制單元 | [第 27 支](lecture27_note.md) |
| SimpleRisc 的指令格式、immediate 和 modifier | [第 7 支](lecture07_note.md)、[第 8 支](lecture08_note.md) |
| Branch、call、ret 的微碼；bus 的真正結構；vertical／horizontal | [第 29 支](lecture29_note.md) |
| 本機 Intel CPU 的微碼版本、Linux 的微碼更新 | [第 29 支 §12](lecture29_note.md#12-原始碼linux-的-microcode-更新) |
| x86 的 rep movs 這類複雜指令 | [第 13 支](lecture13_note.md) |
| 更快的做法：pipeline | [第 30 支](lecture30_note.md) |

---

## 附：本講一句話總結

> **Microprogrammed 處理器把每條 SimpleRisc 指令拆成一段由微指令組成的微程式（存在 firmware 裡，所以可以加指令、修 bug、改行為，代價是慢），硬體上把 PC、IR、decode 暫存器（I、rd、rs1、rs2、immx、branchTarget）以及包住 register file 的 regSrc／regData／regVal、包住 ALU 的 A／B／aluResult／flags、包住記憶體的 mar／mdr／ldResult 共 19 個微暫存器都掛在一條一次只能一個人寫的 shared bus 上，用 mloadIR、mdecode、mswitch、mmov、mmovi、madd、mbeq、mb 八條微指令操作它們：每段微程式先跑 preamble（讀指令、解碼、pc + 4、跳到自己的微程式），再把值搬來搬去並在同一條 mmov 裡用 args 啟動 register file、ALU 或記憶體（例如寫 regSrc 的同時就讀 register file，省一個 cycle），3-address ALU、mov、not、cmp、nop、ld、st 都這樣寫成 1 到 10 條。本機照抄課本的微碼組成模擬器跑通一個存部分和的程式，平均每條指令要 9.5 條微指令，而 3-address 指令實際只會走 11 或 13 條（課本把兩個分支都算進去，說是 14）。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 8: Processor Design, Part 2/3 的 whisper 轉錄稿（本機 `large-v3-turbo`，1:15:53）、Sarangi *Basic Computer Architecture* v3.09 Ch9 §9.4–9.6.7（p.379–396）整理。實測為本機（Python 3.10）結果。*
