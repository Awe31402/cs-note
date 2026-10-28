# Basic Computer Architecture 第三十支影片完整筆記：Pipeline 的概念、pipeline register 與三種 hazard（Chapter 9 Pipelining, Part 1/4 → 課本 Ch10）

> **課程**：Basic Computer Architecture — Chapt 9: Pipelining, Part 1/4（播放清單第 30 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（1:08:36，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 **Ch10** §10.1–10.4（p.411–427）
> **章號**：影片的 Chapter 9 是課本 v3.09 的 **Chapter 10**；影片說的「chapter 8」是課本 Ch9。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 單週期處理器的浪費](#1-單週期處理器的浪費)
- [2. Pipeline 的想法](#2-pipeline-的想法)
- [3. 效能：理想上快 5 倍](#3-效能理想上快-5-倍)
- [4. Pipeline register](#4-pipeline-register)
- [5. Instruction packet](#5-instruction-packet)
- [6. 每一級加上 pipeline register](#6-每一級加上-pipeline-register)
- [7. 兩個不經過 pipeline register 的例外](#7-兩個不經過-pipeline-register-的例外)
- [8. Pipeline diagram](#8-pipeline-diagram)
- [9. Data hazard：RAW](#9-data-hazardraw)
- [10. In-order pipeline，WAW 與 WAR](#10-in-order-pipelinewaw-與-war)
- [11. Control hazard](#11-control-hazard)
- [12. Structural hazard](#12-structural-hazard)
- [13. 【實測】](#13-實測)
- [14. 一頁速查表](#14-一頁速查表)
- [15. 與後續講次的連結](#15-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

【口頭】[00:45]「pipelining 是讓處理器變快**最重要**的技術，沒有別的比它更重要。」這章有 125 張投影片，分好幾講。

```
問題：單週期處理器一次只有一個階段在做事，其他 4 個在發呆
解法：pipeline → 5 條指令同時在 IF、OF、EX、MA、RW
做法：階段之間放 4 個 pipeline register；instruction packet 帶著自己的全部狀態往下走
新問題（hazard）：
  data hazard        RAW（我們的 pipeline 會發生）；WAW、WAR（只在 out-of-order 發生）
  control hazard     分支後面那兩條可能是 wrong path（會發生）
  structural hazard  搶同一個資源（我們的 pipeline 不會發生）
```

本講只看 **hardwired** 處理器；microprogram 處理器不管，因為這裡追求的是效能，不是彈性。

---

## 1. 單週期處理器的浪費

[02:05]–[04:11]　課本 §10.1.1（p.411–412）

【口頭】[03:07]「MA 在做事的時候，IF 在幹嘛？什麼都沒做，只是在發呆。」EX 也一樣。五個階段，任何時候**只有一個**在動。

【課本補充】p.412：
- 單週期處理器裡，電訊號要從 fetch unit 一路跑到 writeback unit，每一段都要時間：讀指令記憶體、讀暫存器、ALU、存取記憶體、寫回。全部做完才能開始下一條。
- 如果五個階段花一樣久，任何時刻**大約 80% 的電路在閒置**。

---

## 2. Pipeline 的想法

[04:11]–[08:54]　課本 §10.1.2（p.412–413，Fig 10.1，Definition 65）

【口頭】[04:26] 回到汽車工廠：油漆部門在上漆時，引擎部門閒著嗎？沒有，它在做**另一台車**的引擎。引擎送去車身部門後，引擎部門馬上開始做下一台。很多台車**同時**在做，半成品一站一站往前走。

處理器也一樣：

```
         IF      OF      EX      MA      RW
週期 n   inst5   inst4   inst3   inst2   inst1   ← 最早的指令在最後面
週期 n+1 inst6   inst5   inst4   inst3   inst2   ← inst1 離開，inst6 進來
```

- 每一級各放一條指令 → 5 條同時處理。
- 一條指令：週期 n 在 IF、n+1 在 OF、n+2 在 EX、n+3 在 MA、n+4 在 RW。
- 好處：data path 的每一部分**一直都在忙**。

【口頭】[07:42]「聽起來很簡單，其實**非常複雜**。」

**定義（Definition 65）**：把處理器分成一個接一個的階段，每個階段同時處理一條不同的指令，叫 **pipelining**。隱含的假設：每個階段花的時間一樣。這一串階段叫 **pipeline**，有 pipeline 的處理器叫 **pipelined processor**。

---

## 3. 效能：理想上快 5 倍

[08:54]–[12:57]　課本 §10.1.3（p.413–414，Fig 10.2）

假設五個階段工作量一樣，整條指令要 T 秒：

| | 每隔多久完成一條 | n 秒內完成幾條 |
|---|---|---|
| 沒有 pipeline | T | n／T |
| 有 pipeline（每級 T／5） | T／5 | n／(T／5) = 5n／T |

【口頭】[10:53] 站在處理器出口看的人：沒有 pipeline 時每 T 秒出來一條；有 pipeline 時每 T／5 秒出來一條 → 指令的**產出率**（throughput）是 5 倍。

【課本補充】p.414：
- 照這個邏輯，100 級 pipeline 就快 100 倍，甚至可以切到一級只有一顆電晶體？**不行**。pipeline 的效能有根本的限制，超過某個點再加級數**反而變慢**（細節在課本 §10.9）。
- 「stages are balanced」（每級時間一樣）是設計者盡量追求的目標。

⚠️ 影片和課本算的是「穩定狀態」，沒算一開始把 pipeline 填滿的 4 個週期。實測見 [§13.1](#131-理想的-pipeline5n--n--4)。

---

## 4. Pipeline register

[12:57]–[16:29]　課本 §10.2.1–10.2.2（p.414–416，Fig 10.3，Definition 66）

把 data path 乾淨地切成五段，每段放一條指令。在相鄰的兩級之間放 **pipeline register**（也叫 **pipeline latch**）：

```
IF ─[IF-OF]─ OF ─[OF-EX]─ EX ─[EX-MA]─ MA ─[MA-RW]─ RW
       ↑          ↑           ↑           ↑
       └──────────┴── 同一個時脈 ┴───────────┘
```

- 5 級 → **4 個** pipeline register，用位置命名：IF-OF、OF-EX、EX-MA、MA-RW。
- 【口頭】[13:30] 它們「不是真的 latch，是 flip-flop」，因為受時脈控制。叫 latch 是「有點濫用名稱」；只叫 register 又會和暫存器檔搞混，所以叫 pipeline register。
- 每個**負緣**，每一級的指令同時往下一級移一格；RW 的那條離開 pipeline。

【課本補充】p.415：
- 任何電路原則上都能切成好幾段做 pipeline，但有規則：① 各段最好是**分開的單元**、彼此連線越少越好；② 所有資料**照同樣順序**經過各段，而且**每段工作量差不多**。
- **div 和 mod 是例外**：比 add、mul 慢很多，會拉長 EX、讓 pipeline 不平衡。簡單的 pipeline 處理器常常乾脆不做這兩條，或用特別的邏輯處理；課本 §10.6 的做法是讓 pipeline 停下來等除法做完。

---

## 5. Instruction packet

[16:29]–[19:18]　課本 §10.2.3（p.416）

在 pipeline register 之間移動的，其實是 **instruction packet**（指令封包）：

| 內容 | 為什麼 |
|---|---|
| 指令本身（32 位元） | 後面還要拿欄位，例如 rd |
| 這條指令的 PC | call 要 pc+4、算 branch target |
| 中間結果 | 後面的階段要用 |
| 控制訊號 | 控制後面各級的多工器 |

【口頭】[17:42] 為什麼需要？單週期時，OF 裡的控制單元直接拉虛線到每一級。現在 pipeline 裡有 **5 條指令**，控制單元產生的訊號是給哪一條的？做 5 個控制單元「不合理」。所以指令經過 OF 時，把後面要的控制訊號**收進自己的封包**，一路帶著走。

「每條指令帶著自己全部的狀態，指令之間就不會互相干擾。」有些階段會加資訊、有些會拿掉。

【課本補充】p.416：
- 指令離開 pipeline 時，它的中間狀態也一起消失，很乾淨。
- 封包越往後需要的資訊越少；但為了一致，課本假設**所有 pipeline register 一樣大**，有些欄位用不到，「代價可以忽略」。

---

## 6. 每一級加上 pipeline register

[19:18]–[36:21]　課本 §10.3（p.417–420，Fig 10.4–10.9）

電路**和第 27 支的單週期處理器完全一樣**，只多了綠色方塊（pipeline register）。

| pipeline register | 存什麼 |
|---|---|
| **IF-OF** | pc、instruction |
| **OF-EX** | pc、branchTarget、**B**、**A**、**op2**、instruction、control |
| **EX-MA** | pc、aluResult、op2、instruction、control |
| **MA-RW** | pc、ldResult、aluResult、instruction、control |

**OF**（[20:29]–[29:48]）：
- 控制單元產生全部控制訊號，存進 OF-EX 的 control 欄位。
- isSt、isRet（課本還有 isImmediate）在 OF **馬上就用**。
- 和單週期不同：**isImmediate 多工器移到 OF**，選好的第二運算元存成 **B**；op1 存成 **A**。

**為什麼 op2 和 B 要分開存**（【口頭】[24:15]–[27:46]）：st 是特例。
- `st rd, imm[rs1]`：I 位元是 1（因為有 offset），所以多工器選 immx → **B = offset**。
- 但 st 還需要 rd 的值（要存的資料）。isSt 讓讀埠 2 讀 rd → **op2 = reg[rd]**。
- B 已經被 immx 佔走，只好**另外存 op2**。
- 其他指令沒有這個問題：A 是第一來源，B 是第二來源（暫存器或 immediate）。

**EX**（[29:48]–[32:35]）：ALU 吃 A、B → aluResult；op2 直接傳下去；branch unit 用 isBeq、isBgt、isUBranch 和 flags 算 isBranchTaken；isRet 多工器在 branchTarget 和 **A**（ret 時是 ra 的內容）之間選 branchPC。

**MA**（[32:35]–[33:40]）：aluResult → mar（位址）；op2 → mdr（要存的資料）；isLd、isSt 控制；讀到的值存成 ldResult。

**RW**（[33:40]–[36:04]）：和單週期一樣，pc+4／ldResult／aluResult 三選一；isCall 選 rd 或 ra；isWb 決定要不要寫。RW 後面**沒有** pipeline register（最後一級）。

【口頭】[34:43] 暫存器檔可以放在 RW 或 OF，「OF 比較好」。

---

## 7. 兩個不經過 pipeline register 的例外

[36:21]–[41:40]　課本 §10.3.6（p.420，Fig 10.10）

一般的流程：每個負緣從前一個 pipeline register 拿資料 → 中間一團邏輯處理（可能存取記憶體）→ 結果寫進下一個 pipeline register。

只有**兩個**地方資料直接跨級，不經過 pipeline register：

| | 從 | 到 | 傳什麼 |
|---|---|---|---|
| ① | EX | IF | isBranchTaken、branchPC |
| ② | RW | OF（暫存器檔） | 要寫的暫存器編號、資料、寫入 enable |

【口頭】[38:03] 圖上 RW 到暫存器檔那條線用「Star Trek 的蟲洞」畫法：值從這裡進去、那裡出來，免得畫一條很長的線。

之後都用簡化的圖（Fig 10.10）：IF（fetch unit＋指令記憶體）、OF（控制單元、immediate 與 branch unit、暫存器檔）、EX（branch unit、flags、ALU）、MA（memory unit、資料記憶體）、RW（register write unit），兩個例外用**紅線**畫。

【課本補充】p.420：圖已經大到一整頁了，之後課本只講 pipeline 的邏輯，實際電路留給讀者。rs1/ra、rd/rs2 兩個多工器在簡化圖裡被當成暫存器檔的一部分，不畫出來。

---

## 8. Pipeline diagram

[41:40]–[46:42]　課本 §10.4.1（p.420–423，Fig 10.11，Example 137）

分析 pipeline 正確性的工具：

```
                         週期 1  2  3  4  5  6  7
[1]: add r1, r2, r3   IF     1  2  3
[2]: sub r4, r5, r6   OF        1  2  3
[3]: mul r8, r9, r10  EX           1  2  3
                      MA              1  2  3
                      RW                 1  2  3
```

- **列**是 pipeline 的階段，**行**是時脈週期。
- 每條指令沿著**往右下的對角線**走。
- 規則（課本 p.422–423）：
  1. 5 列、N 行。
  2. 指令 [k] 在第 m 週期進來 → 寫在第 1 列第 m 行。
  3. 下一個週期，指令可以**留在原地**（pipeline stall，之後講）或往下一列。
  4. 只往下走，**不會往回**。
  5. **一格只能放一條指令**（同一級不能同時有兩條）。
  6. 離開 RW 就從圖上拿掉。

---

## 9. Data hazard：RAW

[46:42]–[52:08]　課本 §10.4.2（p.423–424，Fig 10.12，Definition 67）

```
[1]: add r1, r2, r3     ← 寫 r1
[2]: sub r3, r1, r4     ← 讀 r1

         1  2  3  4  5
IF       1  2
OF          1  2   ← [2] 在週期 3 就要讀 r1
EX             1  2
MA                1  2
RW                   1   ← [1] 週期 5 才寫回 r1
```

- [2] 在**週期 3** 需要 r1，[1] 在**週期 5** 才寫進暫存器檔。
- 把兩格連起來，箭頭**往回指向過去**（往左）→ 不可能 → [2] 會讀到錯的值。
- 這叫 **RAW**（read after write）**相依**，也叫 **true dependence**（真相依）：先寫、後讀。

**定義（Definition 67）**：**hazard** = 指令在 pipeline 裡**可能**執行錯誤。**data hazard** = 因為拿不到正確資料而可能執行錯誤。

【口頭】[51:02] [2] 最早要到週期 5 才能進 IF，週期 6 到 OF 才能讀到對的 r1。所以有 **3 個週期**不能放指令進 pipeline，pipeline 又在發呆了，「而我們做 pipeline 就是為了解決發呆」。這種模式「非常常見」，一定要解決。

---

## 10. In-order pipeline，WAW 與 WAR

[52:08]–[58:38]　課本 §10.4.2（p.424–425，Definition 68）

**In-order pipeline**（循序 pipeline，就是我們這種）：**前面的指令永遠在後面指令的前面**，不准超車。從 pipeline diagram 看：任何週期，較早的指令都在較後面的階段。

【口頭】[53:29]–[54:14]：
- 現代的高效能處理器用 **out-of-order pipeline**（亂序），打破這條規則，後面的指令可以先執行。「這會帶來很多複雜度」，超出這門課，在進階課程講。
- In-order pipeline 還是很多人用，現在主要用在**小型、嵌入式**處理器，那裡重點是**省電**，不是高效能。

In-order pipeline 只會有 RAW hazard。另外兩種只在 out-of-order 發生：

| hazard | 例子 | 出錯的情況 |
|---|---|---|
| **WAW**（write after write） | [1] add **r1**, r2, r3　[2] sub **r1**, r4, r3 | [2] 比 [1] 先寫 r1 → r1 最後是錯的 |
| **WAR**（write after read） | [1] add r1, **r2**, r3　[2] add **r2**, r5, r6 | [2] 在 [1] 讀之前就寫了 r2 → [1] 讀到錯的值 |

【口頭】[58:06] RAR（read after read）不是 hazard：兩個都是讀，就算超車也沒關係，「只要沒有寫就沒問題」。

【課本補充】p.425：
- 現代處理器用 **register renaming**（暫存器重新命名，課本 §10.11.4）來避免 WAW、WAR。
- 「hazard 是理論上的風險」，不是真的會出錯，因為處理器會採取足夠的措施。
- 這本書主要處理 RAW。解法要讓 pipeline 知道「有一條寫 r、後面一條讀 r」，並確保消費者（consumer）拿到生產者（producer）的值；硬體和軟體的解法都會講。

---

## 11. Control hazard

[58:38]–[64:54]　課本 §10.4.3（p.425–427，Fig 10.13，Definition 69–70）

```
[1]: beq .foo
[2]: mov r1, 4
[3]: add r2, r4, r3
...
.foo:
[100]: add r4, r1, r2

         1  2  3  4  5
IF       1  2  3
OF          1  2  3
EX             1  2  3   ← 週期 3：beq 在 EX 才知道要不要跳
```

- beq 在 EX（週期 3）才看 flags.E，決定跳不跳。
- 可是週期 2、3 已經把 [2]、[3] 抓進來了。
- 【口頭】[62:10]「大概在週期 3 的中間或最後才知道是 taken branch，那時已經抓了兩條指令。」
- 如果什麼都不做，[2]、[3] 會繼續往下走，把 4 寫進 r1、把 r3+r4 寫進 r2 → **錯了**。
- 所以要有辦法**取消**（kill）它們，不准它們改記憶體或暫存器檔。

**定義**：
- **Wrong path**（Definition 69）：如果分支結果和真實結果相反時才會執行的指令。例如分支 taken 時，緊接在它後面的指令。
- **Control hazard**（Definition 70）：wrong path 上的指令可能被執行、把結果存進記憶體或暫存器檔，造成錯誤。

---

## 12. Structural hazard

[64:54]–[68:29]　課本 §10.4.4（p.427，Definition 71）

兩條指令在同一個週期搶同一個資源。**我們的 pipeline 不會發生**，但其他 pipeline 會，觀念要懂。

例子：假設有一種 add 可以從記憶體讀一個運算元，而且在 OF 讀：

```
[1]: st  r4, 20[r5]
[2]: sub r8, r9, r10
[3]: add r1, r2, 10[r3]     ← 第二個運算元在記憶體

週期 4：[1] 在 MA 要用 memory unit；[3] 在 OF 也要用 memory unit
```

memory unit 一個週期只能服務一個請求 → 衝突 → 其中一條要等（stall）。

【口頭】[66:55] 更一般的說法：一個資源同時能服務 **m** 個請求，卻來了 **n > m** 個 → structural hazard。

【課本補充】p.427 證明 SimpleRisc pipeline 沒有 structural hazard：只有兩個單元被不只一級用到：
- **fetch unit**：IF 用它，EX 的分支也送資料給它，它設計成兩個都能處理。
- **暫存器檔**：OF 讀、RW 寫。它有 **2 個讀埠、1 個寫埠**，同一週期都能處理。

**三種 hazard 總結**（【口頭】[67:42]）：

| 種類 | 在我們的 pipeline |
|---|---|
| Data：RAW | **會**發生 |
| Data：WAR、WAW | 不會（in-order） |
| Control | **會**發生 |
| Structural | 不會 |

下一講：怎麼處理 RAW 和 control hazard。

---

## 13. 【實測】

程式在 `experiments/lecture30/`。`pipeline.py` 照 Fig 10.9 寫了一個五級 pipeline：4 個 pipeline register 存 instruction packet（pc、inst、control、A、B、op2、branchTarget、aluResult、ldResult），控制單元和各級電路直接用第 27 支的 `experiments/lecture27/datapath.py`。**沒有任何 hazard 處理**，看看會怎樣。

暫存器檔在週期結束（負緣）才寫入，所以同一週期 OF 讀到的是舊值（和課本 §10.5.1「中間要隔 3 條」一致）。`experiments/lecture30/pipeline.py:69-72`：

```python
        # 負緣：所有 pipeline register 同時更新、暫存器檔寫入、pc 更新
        if wb: self.reg[wb[0]] = wb[1]
        self.if_of, self.of_ex, self.ex_ma, self.ma_rw = n_of, n_ex, n_ma, n_rw
        self.pc = branchPC if taken else self.pc + 4
```

### 13.1 理想的 pipeline：5n / (n + 4)

`indep.s`（課本 Fig 10.11 的三條獨立指令），r2=10 r3=20 r5=2 r6=3 r9=4 r10=5：

```
$ python3 -I pipeline.py indep.s r2=10 r3=20 r4=1 r5=2 r6=3 r9=4 r10=5
       1  2  3  4  5  6  7
IF     1  2  3
OF        1  2  3
EX           1  2  3
MA              1  2  3
RW                 1  2  3
7 cycles; r1=30 ... r4=-1 ... r8=20
```

和課本的 pipeline diagram 一模一樣；結果和單週期處理器一致。

n 條**互不相依**的 add，pipeline 要 n + 4 個週期（前 4 個週期在填滿 pipeline）。每週期只有單週期的 1/5，所以加速 = 5n／(n + 4)：

| n | pipeline 週期數 | 加速 |
|---|---|---|
| 3 | 7 | 2.14 |
| 10 | 14 | 3.57 |
| 100 | 104 | 4.81 |
| 1000 | 1004 | 4.98 |

指令夠多才接近影片說的 5 倍。

### 13.2 RAW hazard：讀到舊值

`raw.s`（課本 Fig 10.12）：

```
$ python3 -I pipeline.py raw.s r2=10 r3=20 r4=1 ...
[1] add r1, r2, r3
[2] sub r3, r1, r4
6 cycles; r1=30 r2=10 r3=-1 ...          ← pipeline
raw.s: 2 cycles; r1=30 r2=10 r3=29 ...   ← 單週期（正確）
```

r1 應該是 30，所以 r3 = 30 − 1 = **29**。pipeline 裡 [2] 讀到 r1 的舊值 0，算出 **−1**。

中間塞 nop 試試：

| 程式 | 中間 nop 數 | r3 | 對嗎 |
|---|---|---|---|
| raw.s | 0 | −1 | ✗ |
| raw_nop2.s | 2 | −1 | ✗ |
| raw_nop3.s | 3 | 29 | ✓ |

要隔 **3 條**才對，和影片說的「3 個週期不能放指令」一致。

### 13.3 Control hazard：wrong path 被執行了

`ctrl.s`：`cmp r0, r0` 一定相等，所以 beq 一定跳。

```
[1] cmp r0, r0
[2] beq .foo
[3] mov r1, 4          ← wrong path
[4] add r2, r4, r3     ← wrong path
[5] mov r5, 7          ← wrong path
[6] add r6, r9, r10    ← .foo
       1  2  3  4  5  6  7  8  9
IF     1  2  3  4  6
OF        1  2  3  4  6
EX           1  2  3  4  6
MA              1  2  3  4  6
RW                 1  2  3  4  6
9 cycles; r1=4 r2=21 ...      ← pipeline
ctrl.s: 3 cycles; r2=10 ...   ← 單週期（正確）
```

- beq 在週期 4 的 EX 才決定要跳；這時 [3]、[4] 已經在週期 3、4 被抓進來。
- 週期 5 才抓 .foo 的 [6]。[5] 沒被抓到。
- 結果 r1 = 4、r2 = 21：**兩條 wrong path 指令都寫回了**。剛好是分支後面 2 條，和課本說的一樣。

### 13.4 真的 CPU：乘法器本身也是 pipeline

`ilp.c`：1 億條 `imul`，一種每條都依賴上一條（RAW），一種是 4 條互不相干的鏈輪流做。綁在 CPU 2 上跑 3 次，結果都一樣：

```
$ gcc -O2 -o ilp ilp.c && taskset -c 2 ./ilp
dep  : 1.80 TSC ticks per imul
indep: 0.60 TSC ticks per imul
dep / indep = 3.0
```

第 23 支量過本機 1 個週期 ≈ 0.6 TSC ticks，所以：
- 相依：每條 imul **3 個週期**（乘法器的延遲，latency）。
- 不相依：每條 imul **1 個週期**。乘法器裡同時有 3 條在做，就像這講的 pipeline。
- RAW 相依讓 pipeline 的好處完全消失，慢了 3 倍。

⚠️ 本機的 CPU 是 out-of-order 而且一個週期能發好幾條指令，比這講的 5 級 in-order pipeline 複雜很多；這裡只看「乘法器是 pipeline」這一點。

---

## 14. 一頁速查表

| 主題 | 重點 |
|---|---|
| 單週期的問題 | 任何時候只有一級在做事，約 80% 閒置 |
| Pipelining | 5 級各放一條指令，同時處理；週期 n 在 IF … n+4 在 RW |
| 理想加速 | 每 T／5 完成一條 → 5 倍；實際 5n／(n+4) |
| 不能無限加級數 | 有根本限制，超過某點反而變慢（課本 §10.9） |
| 切 pipeline 的規則 | 單元分開、連線少；資料同順序；每級工作量一樣（div/mod 是例外） |
| Pipeline register | 4 個：IF-OF、OF-EX、EX-MA、MA-RW；其實是負緣觸發的 flip-flop |
| Instruction packet | 指令、PC、中間結果、控制訊號；控制訊號在 OF 產生後跟著走 |
| OF-EX | pc、branchTarget、B、A、op2、instruction、control |
| 為什麼另存 op2 | st 的 B 是 offset，要存的值 reg[rd] 只能放 op2 |
| 不經 pipeline register | ① EX → IF（分支）② RW → 暫存器檔（寫回） |
| Pipeline diagram | 列 = 階段，行 = 週期，指令走右下對角線，一格一條 |
| Hazard | 指令**可能**執行錯誤 |
| RAW | 先寫後讀；真相依；我們的 pipeline 要隔 3 條 |
| In-order | 前面的指令永遠在前面；只會有 RAW |
| WAW、WAR | 只在 out-of-order；用 register renaming 解決 |
| Control hazard | 分支在 EX 才決定，後面 2 條可能是 wrong path |
| Structural hazard | 搶同一資源（m 個服務、n > m 個請求）；我們的 pipeline 沒有 |

---

## 15. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| RAW、control hazard 的軟體解法（nop、重排、delay slot） | **第 31 支**：Pipelining Part 2（課本 §10.5） |
| 硬體解法：interlock、stall、bubble | **第 31–32 支**（課本 §10.6） |
| Forwarding | [第 32 支](lecture32_note.md)（課本 §10.7） |
| Interrupt、exception、效能公式、級數的極限 | **第 32–33 支**（課本 §10.8–10.9） |
| 單週期處理器的五級電路 | [第 27 支](lecture27_note.md#13-整個-data-path) |
| nop 為什麼有用 | [第 8 支](lecture08_note.md#11-nop) 埋的伏筆 |
| 乘法器延遲 | [第 23 支](lecture23_note.md) |
| Out-of-order、register renaming | Advanced Computer Architecture 課程 |

---

## 附：本講一句話總結

> **單週期處理器任何時候只有一級在做事，pipelining 學汽車生產線，讓 IF、OF、EX、MA、RW 五級各處理一條不同的指令，理想上每 T／5 就完成一條、快 5 倍（實測 n 條獨立指令要 n + 4 個週期，n = 1000 時快 4.98 倍）；做法是在相鄰兩級之間放 4 個負緣觸發的 pipeline register（IF-OF、OF-EX、EX-MA、MA-RW），讓 instruction packet（指令、PC、中間結果、在 OF 產生的控制訊號）帶著自己全部的狀態往下走，只有分支結果（EX → IF）和寫回（RW → 暫存器檔）兩條線不經過 pipeline register；但 pipeline 帶來 hazard：RAW data hazard（後面的指令在 OF 讀暫存器時，前面的指令還沒在 RW 寫回，實測要隔 3 條才讀得對）、control hazard（分支在 EX 才決定，後面已抓進來的 2 條可能是 wrong path，實測它們真的寫回了），structural hazard（搶同一資源）以及 WAW、WAR 則不會出現在我們的 in-order pipeline。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 9: Pipelining, Part 1/4 錄影逐字稿（YouTube 自動字幕，1:08:36）、Sarangi *Basic Computer Architecture* v3.09 Ch10 §10.1–10.4（p.411–427）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
