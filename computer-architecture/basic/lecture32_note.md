# Basic Computer Architecture 第三十二支影片完整筆記：Forwarding 的實作、forwarding 條件與效能公式（Chapter 9 Pipelining, Part 3/4 → 課本 Ch10）

> **課程**：Basic Computer Architecture — Chapt 9: Pipelining, Part 3/4（播放清單第 32 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（1:27:44，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 **Ch10** §10.7.4–10.7.5、§10.9（p.445–455、p.463–477）
> **章號**：影片的 Chapter 9 是課本 v3.09 的 **Chapter 10**；影片說的「chapter 8」是課本 Ch9。
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
- [1. 四條 forwarding path（複習）](#1-四條-forwarding-path複習)
- [2. OF 加上 forwarding：M1、M2](#2-of-加上-forwardingm1m2)
- [3. EX 加上 forwarding：M3、M4、M5](#3-ex-加上-forwardingm3m4m5)
- [4. MA 與 RW 加上 forwarding：M6](#4-ma-與-rw-加上-forwardingm6)
- [5. Forwarding 條件：誰的值比較新](#5-forwarding-條件誰的值比較新)
- [6. 演算法：第一個與第二個運算元的衝突](#6-演算法第一個與第二個運算元的衝突)
- [7. Interlock 和 forwarding 一起用](#7-interlock-和-forwarding-一起用)
- [8. call 指令的特例](#8-call-指令的特例)
- [9. 完整的 pipeline](#9-完整的-pipeline)
- [10. 什麼是效能](#10-什麼是效能)
- [11. 效能公式](#11-效能公式)
- [12. 指令數：編譯器的事](#12-指令數編譯器的事)
- [13. 理想 pipeline 的 CPI 與頻率](#13-理想-pipeline-的-cpi-與頻率)
- [14. 理想 pipeline 的最佳級數](#14-理想-pipeline-的最佳級數)
- [15. 真實 pipeline：CPI = CPI_ideal + stall rate × stall penalty](#15-真實-pipelinecpi--cpi_ideal--stall-rate--stall-penalty)
- [16. 最佳級數 k_opt = √(tmax / lrc)](#16-最佳級數-k_opt--tmax--lrc)
- [17. 例題：比較兩個程式](#17-例題比較兩個程式)
- [18. 技術、架構、編譯器](#18-技術架構編譯器)
- [19. Pipeline 的好處到底在哪裡](#19-pipeline-的好處到底在哪裡)
- [20. 【實測】](#20-實測)
- [21. 一頁速查表](#21-一頁速查表)
- [22. 與後續講次的連結](#22-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

這講接著上一講（[第 31 支](lecture31_note.md)）講到一半的 forwarding。

```
Forwarding 的實作：
  每個功能單元的輸入前面加一個多工器 → 6 個 forwarding 多工器 M1–M6
  OF：M1、M2（RW→OF）  EX：M3、M4（MA→EX、RW→EX），M5（RW→EX，store 的值）
  MA：M6（RW→MA，store 的值）  RW：只送出、不接收
  forwarding unit 看 4 個 pipeline register，產生多工器的控制訊號
  還是要 interlock：load-use 停 1 個週期；branch-lock 不變
效能：
  P ∝ IPC × f / #insts
  理想 CPI = (n+k−1)/n；f = 1/(tmax/k + l)
  真實 CPI = CPI_ideal + stall rate × stall penalty
  k_opt ≈ √(tmax / (l·r·c))
  pipeline 的好處只來自「頻率」
```

---

## 1. 四條 forwarding path（複習）

[00:00]–[02:11]　課本 §10.7.4（p.445）

【口頭】每一級的功能單元都可以選：用前一級傳來的**預設輸入**，或用**被 forward 過來的值**。要選就用多工器；多工器由專門的 **forwarding unit** 控制（就像第 27 支的控制單元控制 data path 裡的多工器）。

| 路徑 | 距離（級數） |
|---|---|
| RW → OF | 3 |
| RW → EX | 2 |
| MA → EX | 1 |
| RW → MA | 1 |

⚠️ [01:32] 老師一度說到「MA → OF」，馬上改口「不需要，我們沒有這條」。

所以：OF 多 1 個輸入來源、EX 多 2 個、MA 多 1 個，一共 4 條路徑。IF 不送也不收 forward 的值，不用改。

---

## 2. OF 加上 forwarding：M1、M2

[02:11]–[05:46]　課本 Fig 10.25（p.446）

原本 OF 有一個多工器（課本叫 M′）在「暫存器」和「immediate」之間選第二運算元。現在多加兩個：

| 多工器 | 選擇 | 結果存到 |
|---|---|---|
| **M1** | op1（rs1 的內容）或 RW forward 來的值 | A |
| **M2** | op2（rs2 的內容）或 RW forward 來的值 | 再進 M′ 選 B；也存成 op2（st 要用） |

【課本補充】p.446：not 不需要第一個暫存器運算元，但照樣讀，因為沒有時間先決定「讀還是不讀」。

---

## 3. EX 加上 forwarding：M3、M4、M5

[05:46]–[12:08]　課本 Fig 10.26（p.447）

EX 收到 A、B、op2。

| 多工器 | 輸入 | 給誰 |
|---|---|---|
| **M3** | A、MA 的值、RW 的值（3 選 1） | ALU；**也給 branch unit**（ret 的 ra 也可能要 forward） |
| **M4** | B、MA 的值、RW 的值（3 選 1） | ALU |
| **M5** | op2、RW 的值（2 選 1） | 傳到 EX-MA（store 要存的值） |

**為什麼 M5 不收 MA 的值？**（【口頭】[08:10]，「一個刁鑽的問題」）
- op2 是 store 要存的值，**MA 才用到**，EX 不用。
- 如果值在 MA，下一個週期它就到 RW，而 store 剛好到 MA → 用 **RW → MA** 就好。
- 原則：**forward 得越晚越好**（as late as possible），還可以少一條路徑。
- EX、MA、RW 裡的指令不會被 stall，所以下一個週期一定會照預期往前走。

【口頭】[09:42]「在現有 pipeline 加 forwarding 其實**超級簡單**」：在每個功能單元的輸入前面加一個多工器，一個輸入是前一級的預設值，其他輸入是 forwarding path；幾條路徑就多幾個輸入。

---

## 4. MA 與 RW 加上 forwarding：M6

[12:08]–[19:23]　課本 Fig 10.27–10.29（p.447–448）

**MA**：
- aluResult 是位址，EX 已經算好了，不受 forwarding 影響，直接給 memory unit。
- aluResult 也是 **MA → EX** 這條路徑的**起點**（綠線）：它是 ALU 上一個週期剛算出來的值。
- **M6**：在 op2（store 的值）和 RW forward 來的值之間選。

**RW**：最後一級，不用任何 forward 的值；但它要寫回暫存器檔的值，**同時送去 OF、EX、MA**（紅線）。【口頭】[15:33]「RW 是最大的 forwarder」，送給三級。

MA 既是 forwarding 的**消費者**（收 RW 的值）也是**生產者**（送給 EX）。

總共加了 **6 個多工器**（OF 2 個、EX 3 個、MA 1 個），加上一些跨級的線。

【課本補充】p.448：圖上不畫 forwarding unit 的控制線，「實際電路已經相當複雜了」。

---

## 5. Forwarding 條件：誰的值比較新

[19:55]–[21:52]　課本 §10.7.5（p.450–452）

**衝突**（conflict）= RAW 相依。要分別檢查：
- 第一個運算元：rs1（ret 是 ra）
- 第二個運算元：rs2（st 是 rd）

**同一個運算元和好幾條指令都衝突**時，要從**最新的**（離自己最近的前一條）那條 forward。

【課本補充】p.452 的例子：

```
[1]: add r1, r2, r3
[2]: sub r1, r4, r5
[3]: mul r8, r9, r1     ← [3] 在 EX 時，[2] 在 MA、[1] 在 RW
```

兩條都寫 r1，但 [2] 會蓋掉 [1] 的值，所以要拿 **MA** 的。MA 的優先權要比 RW 高。（【實測】把優先權弄反，答案就錯了，見 [§20.2](#202-特殊情況)。）

⚠️ [21:03] 老師先說「選最早的」，馬上改口「這句話不對，要從**最新的**指令 forward」。他的例子是 OF 裡的指令和 EX、MA、RW 三條都衝突，「要從 EX forward」。但我們沒有 EX → OF 這條路徑；實際上是等這條指令到 EX 時，原本在 EX 的那條已經到 MA，再走 MA → EX。兩種說法的重點一樣：**離自己最近的生產者贏**。

【課本補充】p.450–451 其他重點：
- forwarding unit 從**4 個 pipeline register** 拿 OF、EX、MA、RW 四條指令的內容。
- 四條路徑的距離 3、2、1、1 → 第 i 條指令可以從 i−1、i−2、i−3 拿值。
- 距離 1 的路徑有兩條，兩條都需要：MA → EX 給連續的 ALU 指令；RW → MA 給 `ld r1, ...` 後面緊接 `st r1, ...`（ld 在 MA 才有值，st 在 MA 才要用）。
- `add r1 ...` 後面緊接 `st r1 ...` 兩條路都可以，選 RW → MA（越晚越好）。
- mov 在 EX 其實沒有產生新值，理論上可以用 RW → MA；為了簡單，當成一般 ALU 指令。

---

## 6. 演算法：第一個與第二個運算元的衝突

[21:52]–[28:47]　課本 Algorithm 6、7（p.452–454）

輸入：指令 [A]（消費者）和 [B]（生產者，在 [A] 前面），問 [B] → [A] 有沒有衝突。【口頭】這要做成硬體，這裡只是虛擬碼。

**Algorithm 6：第一個運算元（rs1／ra）**

| 步驟 | 內容 |
|---|---|
| 1 | [A] ∈ {nop, b, beq, bgt, call, not, mov} → 不讀 rs1 → false |
| 2 | [B] ∈ {nop, cmp, st, b, beq, bgt, ret} → 不寫暫存器 → false |
| 3 | src1 = [A].rs1；[A] 是 ret → src1 = ra |
| 4 | dest = [B].rd；[B] 是 call → dest = ra |
| 5 | src1 == dest → true，否則 false |

**Algorithm 7：第二個運算元（rs2／rd）**

| 步驟 | 內容 |
|---|---|
| 1 | [A] ∈ {nop, b, beq, bgt, call} → false |
| 2 | [B] ∈ {nop, cmp, st, b, beq, bgt, ret} → false |
| 3 | [A] 不是 st 且 I = 1 → 第二運算元是 immediate → false |
| 4 | src2 = [A].rs2；[A] 是 st → src2 = [A].rd |
| 5 | dest = [B].rd；[B] 是 call → dest = ra |
| 6 | src2 == dest → true，否則 false |

【口頭】[26:46]–[27:15] 為什麼要特別檢查「不是 st」？st 有兩個暫存器來源、沒有目的暫存器、**又**有 immediate，是我們當初做的特例。「一有特例，pipeline 就需要很多邏輯處理它，所以特例能免則免」，但 st 當時沒得選。

【課本補充】p.452：虛擬碼為了簡短，沒寫「如果其中一條是 bubble 就不 forward」的部分。

---

## 7. Interlock 和 forwarding 一起用

[28:47]–[30:09]　課本 §10.7.4 結尾（p.450）

有了 forwarding 還是需要 interlock：

- **Data-lock**：只剩 **load-use hazard**。EX 裡是 ld、OF 裡的指令要用它載入的值（而且在 EX 就要用）→ **停 1 個週期**。可以用同一套衝突偵測電路。
- **Branch-lock**：和之前一樣（影片說用「option number three」，細節在 [第 31 支 §11](lecture31_note.md#11-branch-lockpredict-not-taken)）。分支 taken → 把 IF、OF 裡的兩條變成 **bubble**。

【課本補充】p.450：
- 唯一**不用停**的 load 相依：ld 寫的暫存器剛好是後面 st **要存的值** → 用 RW → MA forward。
- **Interlock 永遠比 forwarding 優先**。
- Data-lock unit 只需要看 IF-OF 和 OF-EX 兩個 pipeline register（[33:58] 影片也這樣說）。

---

## 8. call 指令的特例

[30:09]–[32:42]　課本 p.455

【口頭】[30:25] 投影片上畫了一個漢堡：food for thought。

call 在 **RW** 才把 ra = PC + 4 寫進去。那緊接著用 ra 的指令（例如 `call .foo` 跳去的第一條就是 `ret`，或 `add r1, ra, 4`）會不會讀錯？

**不會**。證明大綱：
1. call 是 taken branch，它在 EX 時，IF、OF 裡的兩條會被變成 bubble。
2. 所以 call 到 RW 時，下一條有效的指令**最近也只到 OF**。
3. 它可以走 **RW → OF** 拿到 ra。【口頭】PC + 4 是很簡單、很快的加法，來得及。

---

## 9. 完整的 pipeline

[32:42]–[36:04]　課本 Fig 10.30–10.31（p.449–450）

- 所有 pipeline register 把指令資訊送給 **forwarding unit**，它算出 6 個多工器的控制訊號。
- **Data-lock unit**、**branch-lock unit**、stall 和 bubble 電路和之前一樣。

【口頭】[34:28] 這就是一個**五級 in-order pipeline**：有 pipeline、interlock、forwarding，「做出高效能 in-order 處理器需要的幾乎全部東西」。

【口頭】[34:57]–[35:46] 課本網站提供**沒有 pipeline** 的 Logisim 和 VHDL 基礎處理器。老師的學生作業是自己加上 pipeline、interlock、forwarding，「一兩個月做得完，而且很好玩」。學 pipeline 最有效的方法就是**自己做一個**。

---

## 10. 什麼是效能

[36:04]–[38:47]　課本 §10.9.1（p.463）

「A 處理器比 B 快」這句話**幾乎沒有意義**。要問：**對某個程式**（或某一組程式）的效能是多少？因為 A 可能跑某些程式快、跑另一些慢。

**效能 ∝ 1 ／ 執行時間**：給處理器一個程式，同時按下碼錶，程式結束時停錶。

【口頭】[38:00] 你常聽到時脈頻率、核心數……「效能不是那些」，效能就是跑完一個程式所需時間的倒數。

【課本補充】p.463：買筆電或手機時看到的頻率、RAM、硬碟大小，「都不直接代表處理器的效能」。

---

## 11. 效能公式

[38:47]–[42:51]　課本 Eq 10.2–10.3（p.466–467，Definition 85–86）

```
時間 τ = #seconds
       = (#seconds / #cycles) × (#cycles / #insts) × #insts
       =        1/f           ×        CPI         × #insts

效能 P ∝ IPC × f / #insts      （IPC = 1/CPI）
```

- **CPI**：cycles per instruction，每條指令平均幾個週期。
- **IPC**：instructions per cycle，每個週期平均完成幾條。
- **#insts** 是 **dynamic instruction**（動態指令）的數量，不是執行檔裡的指令數。

| | 定義 |
|---|---|
| **Static instruction**（靜態指令） | 執行檔裡的一條指令 |
| **Dynamic instruction**（動態指令） | 靜態指令的一次執行實例，指令進 pipeline 時由處理器產生 |

例子：for 迴圈裡有 10 條指令、跑 1000 次 → 處理器看到 **10,000** 條動態指令。

【口頭】[41:47]「這個公式很簡單、很重要，而且大部分學生都搞錯，所以更重要。」效能由**三個量**決定，不是一個。單位不重要（任意單位），用法一致就好。

---

## 12. 指令數：編譯器的事

[42:51]–[48:14]　課本 §10.9.2（p.467–468）

動態指令數取決於程式碼，和 pipeline 無關：C → 組合語言 → 機器碼（一對一）。聰明的編譯器可以減少它：
- **Dead code removal**：刪掉對最後輸出沒影響的程式碼。
- **避免重複計算**。
- **Function inlining**（函式內聯）：把很小的函式的程式碼直接貼進呼叫它的地方。

【口頭】[46:37] 例子：`y = square(x)` 改成 `y = x * x`，省下 call、ret、以及 spill／restore 暫存器的指令。程式設計師常寫很小的函式，編譯器預設會自動 inline。

---

## 13. 理想 pipeline 的 CPI 與頻率

[48:14]–[55:09]　課本 Eq 10.4–10.6（p.468）

**CPI**：
- 單週期處理器：每條 1 個週期，CPI = 1。
- 理想 k 級 pipeline（沒有 hazard）、n 條指令：第 1 條在第 k 週期離開，之後每週期離開 1 條 → 共 **n + k − 1** 個週期。

```
CPI = (n + k − 1) / n        → n → ∞ 時趨近 1
```

**頻率**：
- **tmax**：單週期處理器裡最慢的指令要多久（叫 **algorithmic work**，演算法上的工作量）。單週期處理器：f_max = 1／tmax。
- 切成 k 級、每級平衡 → 每級 tmax／k，再加上 pipeline register 的延遲 **l**（latch delay）。

```
1/f = t_stage = tmax/k + l
```

【口頭】[54:16]「切成 k 份不一定切得剛好」，而且訊號還要穿過 pipeline latch，所以要加 l。

---

## 14. 理想 pipeline 的最佳級數

[55:09]–[60:17]　課本 Eq 10.7–10.8（p.469）

假設 #insts 是常數（和 pipeline 無關），P = f／CPI：

```
P = n / ((n−1)·tmax/k + (tmax + l·n − l) + l·k)
∂/∂k = 0  →  k_opt = √((n − 1)·tmax / l)
```

意義：
- n 越大，可以用越多級（一開始填滿 pipeline 的 k 個週期被攤平了）。n → ∞ 時 k_opt → ∞，「不實際」，因為我們假設了完全理想的 pipeline。
- k_opt ∝ √tmax：工作越多，越需要深的 pipeline 來分攤。
- k_opt ∝ 1／√l：latch 越慢，浪費在進出 latch 的時間越多，級數要越少。

---

## 15. 真實 pipeline：CPI = CPI_ideal + stall rate × stall penalty

[60:17]–[65:15]　課本 §10.9.3（p.470–471，Eq 10.9）

```
CPI = CPI_ideal + stall rate × stall penalty
```

- **CPI_ideal**：沒有任何 stall 時的 CPI（我們是 1）。站在 pipeline 出口的人看到每週期出來 1 條。
- **stall rate**（停頓率）：每條指令平均遇到幾次 stall。
- **stall penalty**（停頓代價）：每次停幾個週期。
- 這其實是加權平均。【口頭】「這也是很重要的公式」。

建模時的兩個假設（【口頭】[62:07]：其他教科書也這樣做；無法數學證明，但和實驗、常識大致相符）：
1. **stall rate = r**，只跟程式和它的相依關係有關，和 pipeline 無關。
2. **stall penalty = c·k**，和級數成正比。【口頭】20 級 pipeline 遇到 load-use 不會只停 1 週期；把每級再切 4 份，大概要停 3、4 個週期。

```
CPI = (n + k − 1)/n + r·c·k
```

【課本補充】p.470 Example 140：10% 指令有 load-use（停 1）、20% 是 taken branch（停 2）→ CPI = 1 + 0.1×1 + 0.2×2 = **1.5**。

---

## 16. 最佳級數 k_opt = √(tmax / lrc)

[65:15]–[70:39]　課本 Eq 10.10–10.12（p.471–472）

```
P = (1/(tmax/k + l)) / ((n+k−1)/n + r·c·k)
∂/∂k = 0, n → ∞  →  k_opt = √(tmax / (l·r·c))
```

**意義**（影片和課本的 Implication 1–3）：

| 情況 | 結論 | 原因 |
|---|---|---|
| 相依很多（r 大） | **少**一點級數 | 極端例子：每條都和前一條有 load-use，深 pipeline 根本沒好處，bubble 太多 |
| 有 forwarding（c 小） | **多**一點級數 | interlock 要停 3 週期；forwarding 大部分不停，load-use 只停 1 |
| tmax／l | 隨製程**幾乎不變** | 電晶體變小，邏輯和 latch 都變快，比例差不多 |

【口頭】[69:52] 所以「過去 5 到 10 年 pipeline 級數大致沒變」。

【課本補充】p.472 最佳級數下的效能（Eq 10.12）：

```
P_ideal = 1 / (√(r·c·tmax) + √l)²
```

Implication 4：l、r、c、tmax 任何一個變大，效能都下降；相依很少的工作負載（r、c 小），效能主要由 latch delay 決定。

【課本補充】p.473 Example 142：tmax／l = 20、r = 0.2、c = 0.6 → k = √(20／0.12) = 12.9 ≈ **13**。

---

## 17. 例題：比較兩個程式

[72:30]–[77:35]　課本 Example 143（p.473）

| | P1 | P2 |
|---|---|---|
| loads 比例 | 0.4 | 0.3 |
| branches 比例 | 0.2 | 0.1 |
| taken branch 比例（在 branch 裡） | 0.5 | 0.4 |
| 頻率 | 1 | 1.5 |

CPI_ideal = 1；一半的 load 有 load-use hazard（停 1）；taken branch 停 2。

```
CPI = 1 + 0.5 × ratio(loads) × 1 + ratio(branches) × ratio(taken) × 2
CPI_P1 = 1 + 0.2 + 0.2  = 1.4    → P = 1/1.4   = 0.71 a.u.
CPI_P2 = 1 + 0.15 + 0.08 = 1.23  → P = 1.5/1.23 = 1.22 a.u.
```

P2 比較快。**a.u.** = arbitrary units（任意單位）。

【口頭】[75:14] 最重要的是**怎麼算 CPI**：stall 有好幾種原因（load、branch），它們是互斥事件，可以加起來。load 的 stall rate = 0.4 × 0.5 = 0.2，penalty 1；branch 的 stall rate = 0.2 × 0.5，penalty 2。

---

## 18. 技術、架構、編譯器

[77:35]–[80:51]　課本 §10.9.5（p.474–477，Fig 10.35）

P = f × IPC ／ #insts 的三個量由誰決定：

| 量 | 決定它的 |
|---|---|
| f = 1／(tmax/k + l) | tmax、l：**技術**（電晶體＋電路）；k：**架構** |
| IPC | **編譯器**（重排指令避免 load-use）、**架構**（interlock、forwarding……） |
| #insts | **編譯器** |

【口頭】[80:05] 這講看到最有效的提高 IPC 的方法是 **forwarding**：減少 stall → CPI 下降 → IPC 上升。

【課本補充】p.475：
- 「architecture」這裡泛指處理器的組織和設計；文獻裡常同時指 ISA 和設計。
- Example 144：`add r1,r2,r3; ld r4,10[r5]; sub r1,r4,r2` 有 load-use；把 ld 移到 add 前面就沒了。
- 製程**只影響頻率**，不影響 IPC 或指令數。

---

## 19. Pipeline 的好處到底在哪裡

[80:51]–[87:29]　課本 Important Point 16（p.476）、p.477

【口頭】[80:51]「這是**幾乎所有學生都會漏掉**的最重要的一點」：pipeline 的好處從哪來？

| 因素 | 有 pipeline 後 |
|---|---|
| #insts | 不變（是程式的性質） |
| IPC | 單週期 = 1；理想 pipeline = 1；真實 pipeline 有 stall → **< 1**，反而變差 |
| **f** | 每級只要 tmax／k + l → **可以調高** |

所以**唯一**的好處是**頻率**。【口頭】[84:08]「仔細聽：pipeline 的好處是讓我們能提高頻率。」級數越多頻率越高，但 IPC 會掉；到某個最佳點效能最高，之後下降。很多論文畫效能 vs. 級數的圖都看到這個形狀。

【口頭】[85:44]「如果做了 pipeline **卻不提高頻率，效能什麼都得不到**。」

頻率的限制：
- 電晶體撐不住。
- 功耗太大。

⚠️ 功耗和頻率的關係，兩邊說法不同：
- 影片 [86:25]：功耗 ∝ **V² × f**（V 是供電電壓）。
- 課本 p.477：經驗法則，功耗大約和頻率的**三次方**成正比，頻率加 10% 功耗加超過 30%。
- 兩者不衝突：提高頻率通常也要提高電壓（V 大約跟著 f 變），V²f 就變成約 f³。課本 §10.10 有更詳細的功耗討論。

【口頭】[87:12]「至少過去 10 到 15 年，pipeline 級數大約維持在 **10 到 15** 級，頻率大約維持在 **2 到 3.5 GHz**。」課本 p.477：超過 20 級的 pipeline 因為複雜度「非常難做」；這個功耗問題叫 **power wall**（功耗牆），設計者越來越避免很深、很高頻的 pipeline。

⚠️ 本機 i7-13650HX 的 `lscpu` 顯示 **CPU max MHz: 4900**，比影片說的 3.5 GHz 高（影片 2015 年錄製；現代 CPU 只在少數核心、短時間用 turbo 衝到這麼高）。

下一講（[第 33 支](lecture33_note.md)）：interrupt 和 exception。

---

## 20. 【實測】

程式在 `experiments/lecture32/`。`pipeline_fwd.py` 在第 30 支的 pipeline 上加了：
- **branch-lock**：EX 裡是 taken branch → IF、OF 變 bubble。
- **data-lock**：`fwd` 模式只擋 load-use（停 1）；`lock` 模式（沒有 forwarding）只要和 EX、MA、RW 裡任一條有 RAW 就停。
- **forwarding**：四條路徑、六個多工器，衝突判斷就是 Algorithm 6、7（`experiments/lecture32/pipeline_fwd.py:14-21`）：

```python
def conflict1(a, b):
    """Algorithm 6：[B] → [A] 在第一個運算元（rs1／ra）上有沒有 RAW"""
    if a is None or b is None: return False
    if op(a) in ('nop', 'b', 'beq', 'bgt', 'call', 'not', 'mov'): return False
    if op(b) in ('nop', 'cmp', 'st', 'b', 'beq', 'bgt', 'ret'): return False
    src1 = 15 if op(a) == 'ret' else bits(a['inst'], 19, 22)
    dest = 15 if op(b) == 'call' else bits(b['inst'], 23, 26)
    return src1 == dest
```

EX 的 M3（`experiments/lecture32/pipeline_fwd.py:67-68`）先看 MA、再看 RW，就是「MA 優先」：

```python
                if conflict1(p_ex, p_ma): A = p_ma['aluResult']; self.used['MA->EX'] += 1
                elif conflict1(p_ex, p_rw): A = wbval(p_rw); self.used['RW->EX'] += 1
```

### 20.1 結果全部正確，forwarding 少停很多

13 個程式（第 7、8、30 支的）在 `fwd` 和 `lock` 兩種模式下跑，暫存器和 flags 都和第 27 支的單週期處理器**完全一樣**。週期數：

| 程式 | 指令數 | lock 週期（stall） | fwd 週期（stall） | flushed |
|---|---|---|---|---|
| l07/ex28.s | 5 | 15（6） | 9（0） | 0 |
| l07/fact.s（r0=10） | 38 | 88（30） | 58（0） | 0 |
| l07/lcm.s（12, 18） | 18 | 46（18） | 28（0） | 4 |
| l07/shift.s | 8 | 24（12） | 12（0） | 0 |
| l08/rfact.s | 125 | 236（45） | 191（0） | 61 |
| l30/raw.s | 2 | 9（3） | 6（0） | 0 |

- 有 forwarding 時，這些程式**一次都沒停**（它們剛好沒有 load-use）。
- 沒有 forwarding 時，RAW 每次要停到 3 個週期。
- flushed 是被 branch-lock 變成 bubble 的指令數，兩種模式一樣。rfact.s 有很多 call、ret、b，flush 了 61 條。

### 20.2 特殊情況

| 程式 | 測什麼 | 結果 |
|---|---|---|
| `loaduse.s`：`ld r1,[r2]` → `add r3,r1,r1` | load-use | 停 **1** 次，之後走 RW → EX，r3 = 14 ✓ |
| `ldst.s`：`ld r1,[r2]` → `st r1,4[r2]` | ld → st 的存值 | **0** 次停頓，用 RW → MA ✓ |
| `addst.s`：`add r1,...` → `st r1,20[r4]` | 兩條路都可以時 | 選 RW → MA（越晚越好）✓ |
| `prio.s`：課本 p.452 的例子 | MA 比 RW 優先 | r8 = 4 × (10 − 3) = **28** ✓，用 MA → EX |
| `callret.s`：`call .f` → `.f: ret` | call 的特例 | 3 條被 flush，ret 走 **RW → OF** 拿到 ra ✓ |

把 M4 的 MA → EX 拿掉（讓 RW 先），`prio.s` 的 r8 變成 4 × 3 = **12**：拿到 [1] 的舊 r1（= 2 + 1），錯了。所以優先權真的重要。

### 20.3 最佳級數

`perf.py` 用課本 Eq 10.10 直接掃 k = 1…200，和公式比：

```
Example 142                  r=0.2 c=0.6: 數值最佳 k= 13，公式 k=  12.9，P(最佳整數 k)=0.1539，公式 P=0.1539
c 減半（forwarding 讓停頓變便宜）      r=0.2 c=0.3: 數值最佳 k= 18，公式 k=  18.3，P(最佳整數 k)=0.2277，公式 P=0.2277
r 加倍（相依很多的程式）                r=0.4 c=0.6: 數值最佳 k=  9，公式 k=   9.1，P(最佳整數 k)=0.0982，公式 P=0.0982
P(k) 曲線（r=0.2, c=0.6）： k=1:0.043 k=2:0.073 k=5:0.125 k=8:0.146 k=13:0.154 k=20:0.147 k=30:0.130 k=50:0.102
P1: CPI = 1.40，performance = f/CPI = 0.71 a.u.
P2: CPI = 1.23，performance = f/CPI = 1.22 a.u.
```

- 數值最佳的 k 和公式一致（13、18、9）。
- c 減半 → 最佳級數變多；r 加倍 → 變少。和 Implication 1、2 一致。
- P(k) 先升後降，13 級最高；就是影片說的「效能 vs. 級數有一個最高點」。
- Example 143 的答案（1.4、1.23、0.71、1.22）和課本一致。
- r = 0（理想 pipeline、n = 10⁶）時，掃到 200 級都還在變好，公式給 k = 4472，就是「n → ∞ 時 k → ∞」。

---

## 21. 一頁速查表

| 主題 | 重點 |
|---|---|
| 4 條 forwarding path | RW→OF（3）、RW→EX（2）、MA→EX（1）、RW→MA（1） |
| 做法 | 每個功能單元輸入前加多工器：預設輸入＋各 forwarding 輸入 |
| 6 個多工器 | OF：M1（A）、M2（op2）；EX：M3（A）、M4（B）、M5（op2）；MA：M6（store 值） |
| M5 為什麼只收 RW | store 值 MA 才用，晚一級再 RW→MA 就好（越晚越好） |
| M3 也給 branch unit | ret 的 ra 可能要 forward |
| 多個衝突 | 從最新的生產者拿（MA 優先於 RW） |
| Algorithm 6 | 第一運算元 rs1（ret：ra）vs 目的 rd（call：ra） |
| Algorithm 7 | 第二運算元 rs2（st：rd），I=1 且非 st → 不用 |
| 還需要的 interlock | load-use 停 1；branch-lock 不變；interlock 優先 |
| 不用停的 ld | ld 後接 st 存同一暫存器 → RW→MA |
| call 特例 | 後兩條被 flush，用 ra 的指令最近只在 OF → RW→OF |
| 效能 | ∝ 1／執行時間，對某個程式而言 |
| 效能公式 | P ∝ IPC × f ／ #insts（動態指令數） |
| 理想 CPI | (n + k − 1)／n |
| 頻率 | 1／f = tmax／k + l |
| 理想 k_opt | √((n−1)·tmax／l) → ∞ |
| 真實 CPI | CPI_ideal + stall rate × stall penalty = (n+k−1)/n + rck |
| 真實 k_opt | √(tmax／(l·r·c)) |
| 最佳效能 | 1／(√(rc·tmax) + √l)² |
| 誰決定什麼 | f：技術＋架構；IPC：編譯器＋架構；#insts：編譯器 |
| Pipeline 的好處 | **只有頻率**；IPC 反而 ≤ 1 |
| 限制 | 功耗（影片：V²f；課本：約 f³）、複雜度（> 20 級很難） |

---

## 22. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| interlock、stall、bubble、branch-lock 的三種做法 | [第 31 支](lecture31_note.md)：Pipelining Part 2（課本 §10.6–10.7.3） |
| Interrupt、exception、precise exception | [第 33 支](lecture33_note.md)：Pipelining Part 4（課本 §10.8） |
| 功耗與溫度、power wall | 課本 §10.10；[第 38 支](lecture38_note.md)：多處理器概論 |
| 記憶體延遲讓 stall 更嚴重（cache miss） | [第 34](lecture34_note.md)–[37 支](lecture37_note.md)：The Memory System |
| Pipeline、hazard 的基本概念 | [第 30 支](lecture30_note.md) |
| 單週期處理器（IPC = 1） | [第 27 支](lecture27_note.md) |
| Out-of-order、register renaming、超純量 | Advanced Computer Architecture 課程 |

---

## 附：本講一句話總結

> **在五級 pipeline 加 forwarding 很簡單：每個功能單元的輸入前面加一個多工器，共 6 個（OF 的 M1、M2 收 RW；EX 的 M3、M4 收 MA 和 RW，M5 只收 RW；MA 的 M6 收 RW），因為 store 的值到 MA 才用、照「越晚越好」走 RW→MA 就能省一條路徑；forwarding unit 用 Algorithm 6、7 判斷第一（rs1／ra）、第二（rs2／st 的 rd）運算元和前面指令的目的暫存器（rd／call 的 ra）有沒有 RAW，有好幾條衝突時拿最新的（MA 優先於 RW）；interlock 仍然要，但只剩 load-use 停 1 週期和 taken branch 清掉 2 條，call 後面用 ra 的指令也能靠 RW→OF 拿到正確值。效能是對某個程式而言的 1／執行時間，P ∝ IPC × f ／ #insts；理想 pipeline CPI = (n+k−1)/n、1/f = tmax/k + l，真實 CPI = CPI_ideal + stall rate × stall penalty，假設 penalty ∝ k 可得 k_opt = √(tmax／(l·r·c))：相依多就用淺 pipeline，有 forwarding 就能做深；而 pipeline 的好處只來自提高頻率（IPC 反而 ≤ 1），頻率又受功耗限制，所以級數這些年維持在 10–15 級。實測加了 forwarding 和 interlock 的 pipeline 模型在 13 個程式上結果和單週期處理器完全一致，週期數比只有 interlock 時少很多；把 MA 優先權拿掉就會算錯；數值掃描的最佳級數（13、18、9）和公式一致。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 9: Pipelining, Part 3/4 錄影逐字稿（YouTube 自動字幕，1:27:44）、Sarangi *Basic Computer Architecture* v3.09 Ch10 §10.7.4–10.7.5、§10.9（p.445–455、p.463–477）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
