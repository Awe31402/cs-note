# Basic Computer Architecture 第三十一支影片完整筆記：軟體解法（nop、重排、delayed branch）、interlock 與 forwarding 的觀念（Chapter 9, Part 2/4 → 課本 Ch10）

> **課程**：Basic Computer Architecture — Chapt 9: Pipelining, Part 2/4（播放清單第 31 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（1:14:39，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 **Ch10** §10.5–10.7.3（p.428–445）
> **章號**：影片的 Chapter 9 是課本 v3.09 的 **Chapter 10**。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 這支的自動字幕很亂：stage 名稱常被聽成「a stage」「i and o stages」，有一段（[30:45]）整段重複亂碼。本筆記的 stage 名稱一律對照課本：IF、OF、EX、MA、RW。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 軟體解法一：插 nop](#1-軟體解法一插-nop)
- [2. 軟體解法二：compiler 重排程式碼](#2-軟體解法二compiler-重排程式碼)
- [3. Control hazard 的軟體解法：delay slot 與 delayed branch](#3-control-hazard-的軟體解法delay-slot-與-delayed-branch)
- [4. 為什麼還要硬體解法](#4-為什麼還要硬體解法)
- [5. Interlock：data-lock 與 branch-lock](#5-interlockdata-lock-與-branch-lock)
- [6. 軟體 vs 硬體](#6-軟體-vs-硬體)
- [7. Bubble：pipeline 裡的 nop](#7-bubblepipeline-裡的-nop)
- [8. Control hazard 的 bubble](#8-control-hazard-的-bubble)
- [9. 偵測衝突的演算法（Algorithm 5）](#9-偵測衝突的演算法algorithm-5)
- [10. 怎麼停住 pipeline：data-lock unit](#10-怎麼停住-pipelinedata-lock-unit)
- [11. Branch-lock：predict not taken](#11-branch-lockpredict-not-taken)
- [12. Forwarding 的想法](#12-forwarding-的想法)
- [13. 四條 forwarding path](#13-四條-forwarding-path)
- [14. Load-use hazard：forwarding 也救不了](#14-load-use-hazardforwarding-也救不了)
- [15. 【實測】](#15-實測)
- [16. 一頁速查表](#16-一頁速查表)
- [17. 與後續講次的連結](#17-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

上一講（第 30 支）找到了三種 hazard。這一講講解法：

| 問題 | 軟體解法 | 硬體解法 |
|---|---|---|
| RAW（data hazard） | 插 nop、compiler 重排 | **data-lock** interlock（停住、插 bubble）→ 更好的：**forwarding** |
| Control hazard | **delayed branch**（delay slot） | **branch-lock**（taken 才把兩條變 bubble） |

最後：forwarding 的觀念和四條 forwarding path，以及唯一救不了的 **load-use hazard**。forwarding 怎麼實作，下一講（第 32 支）講。

---

## 1. 軟體解法一：插 nop

[00:52] 例子：

```
add r1, r2, r3     ← 寫 r1
sub r3, r1, r4     ← 讀 r1：RAW
```

[01:14] 第一條要到 RW 才寫進 register file，第二條在 OF 就要讀。上一講算過：中間要隔 **3 條指令**，資料才來得及。

[01:34] 怎麼隔 3 條？用第一章就介紹過、「什麼都不做」的 **nop**：

```
add r1, r2, r3
nop
nop
nop
sub r3, r1, r4
```

[01:50] 當 `sub` 在 OF 時，前面三個 nop 分別在 EX、MA、RW……`add` 已經做完了，`sub` 從 register file 讀到對的 r1。

[02:58] 但這是**爛方法**：白白浪費 3 個 cycle。

---

## 2. 軟體解法二：compiler 重排程式碼

[03:13] 程式長一點的話，可以**重排**，讓限制一樣滿足，但 nop 越少越好。

課本 Example 138（p.429），影片 [03:32] 同一個例子：

```
原本                     重排後
add r1, r2, r3           add r1, r2, r3
add r4, r1, 3   ← RAW    add r8, r5, r6
add r8, r5, r6           add r10, r11, r12
add r9, r8, r5  ← RAW    nop
add r10, r11, r12        add r4, r1, 3
add r13, r10, 2 ← RAW    add r9, r8, r5
                         add r13, r10, 2
```

- [05:13] 把不相干的 `add r8` 和 `add r10` 往上搬，夾在 r1 的 producer 和 consumer 中間。
- [05:51] 還差一條，所以補 **1 個 nop**。
- [06:10] 之後 r8、r10 的 producer 和 consumer 之間也剛好都隔 3 條。
- 從 3 × 3 = 9 個 nop 變成 1 個。

[04:17] 誰來重排？可以是程式設計師，也可以是 compiler。[04:56] 老師說：正確性的責任**不該**放在程式設計師身上，這種轉換應該自動做，交給 **compiler**。

[07:05] 文獻裡的演算法都在做同一件事：重排程式碼、讓要插的 nop 最少。（本機實測見 [§15.1](#151-example-138插-nop-和重排)。）

---

## 3. Control hazard 的軟體解法：delay slot 與 delayed branch

[07:40] 最笨的方法：每個 branch 後面插 **2 個 nop**。因為 branch 到 EX 才知道結果，那時 IF、OF 已經抓了後面兩條，可能是錯的。

[08:13] 更好的方法：規定 **branch 後面那兩條一定會執行**。這兩個位置叫 **delay slot**。

[08:27] 然後從 branch **前面**找兩條跟 branch 無關的指令，搬到 delay slot：

- 它們本來就要在 branch 之前執行，現在只是晚一點做，結果一樣。
- 不用作廢任何指令，也不用插 nop，**一點時間都不浪費**。

[09:22] 這種 branch 叫 **delayed branch**。課本 Example 139（p.430–431），影片 [09:42]：

```
原本                     放進 delay slot
add r1, r2, r3           b .foo
add r4, r5, r6           add r1, r2, r3   ← delay slot 1
b .foo                   add r4, r5, r6   ← delay slot 2
add r8, r9, r10          add r8, r9, r10
```

[10:12] 處理器把 branch 和後面兩條都當成普通指令執行；branch 方向在 EX 決定時，兩條 delay slot 的指令已經在 pipeline 裡，讓它們繼續就好。[13:41] 之後再從正確的地方抓：taken 就從 branch target，not taken 就從 branch 後面第 3 條。

【課本補充】課本 p.430：搬進 delay slot 的指令不能違反資料相依、不能造成新的 RAW hazard，而且**不能搬 `cmp`**（branch 要用它的結果）。找不到就補 nop；只找到一條就補一個 nop。這種做法**完全不用改硬體**，第 30 支的 pipeline 本來就支援。

【課本補充】課本 Definition 72（p.430）：branch 抓進來到結果決定之間，處理器會抓 n 條指令，就說有 **n 個 delay slot**。

（本機實測見 [§15.2](#152-example-139delayed-branch)。）

---

## 4. 為什麼還要硬體解法

[14:17] 軟體解法很好，但有問題：

- [14:44] 每一種 pipeline 都要重新編譯。為一顆處理器排好的程式碼，在另一顆上可能就**不能跑**（stage 數、delay slot 數不一樣）。
- [15:11] 不能完全信任 compiler 會把 nop 插對。
- [15:31] 軟體解法把 compiler 和硬體綁死，還要把硬體的細節公開，硬體廠商不見得願意。

【課本補充】課本 p.431：程式設計師也可能手寫組合語言，沒排好就出錯；而且組合語言的目的之一就是同一個 ISA 的程式能在不同機器上跑，軟體解法破壞了這一點。硬體解法的目標：不管程式怎麼寫，結果永遠和單週期處理器一樣。

---

## 5. Interlock：data-lock 與 branch-lock

[15:48] 硬體的機制叫 **interlock**，有兩種條件：

| 條件 | 意思 | 代價 |
|---|---|---|
| **Data-lock** | consumer 指令拿到正確的值之前，**不能離開 OF** | 停住 IF 和 OF 兩個 stage |
| **Branch-lock** | **絕不執行** wrong path 上的指令 | branch 在 EX 才決定，IF、OF 裡那兩條可能要作廢 |

[17:01] 兩個條件合起來叫 **interlocks**。

【課本補充】課本 Definition 73（p.432）：讓某個 stage 暫時不接受新指令，直到某個條件不成立，叫 **pipeline stall** 或 **pipeline interlock**，目的是保證正確性。

---

## 6. 軟體 vs 硬體

[17:01] 課本 Table 10.1（p.433）：

| | 軟體 | 硬體（interlock） |
|---|---|---|
| 可攜性 | 只能在某一顆處理器上跑 | ✅ 任何程式在任何 pipeline 上都能跑 |
| Branch | ✅ 用 delay slot 可以零損失 | 我們的設計要停 2 個 cycle |
| RAW hazard | ✅ 可以靠排程全部消掉 | 要停 pipeline |
| 效能 | 看程式的性質 | 基本版預期比軟體慢 |

[18:06] 硬體的基本版比軟體慢，但之後會改進它，讓它比軟體還快（就是 forwarding）。

【課本補充】課本 p.433：也可以兩個一起用：compiler 盡量把 producer 和 consumer 拉開、利用 delayed branch，但**不保證**正確；正確性交給 interlock。這樣 stall 變少，兩邊的好處都拿到。

---

## 7. Bubble：pipeline 裡的 nop

[18:24] 回到：

```
[1]: add r1, r2, r3
[2]: sub r4, r1, r2
```

[19:05] 指令 2 要在 OF 停 3 個 cycle，等指令 1 寫好 r1。這段時間要一直送 nop 給 EX。

課本 Figure 10.14（p.434）：

```
            1   2   3   4   5   6   7   8   9
IF          1   2
OF              1   2   2   2   2
EX                  1   ○   ○   ○   2
MA                      1   ○   ○   ○   2
RW                          1   ○   ○   ○   2
                              （○ = bubble）
```

- [20:20] 第 3 個 cycle，指令 2 進 OF 時發現：r1 從 register file 拿不到（指令 1 還沒做完）。
- [20:39] 接下來指令 2 待在 OF，**每個 cycle 送一個 bubble 給 EX**。
- [21:04] **Bubble** 就是 nop，像水裡的泡泡，沿著 pipeline 走、什麼都不做，最後出去。
- [21:38] 第 6 個 cycle，指令 1 把 r1 寫進 register file，指令 2 終於能讀到。
- [22:20] 指令 2 在第 9 個 cycle 做完。

[23:48] Bubble 讓 pipeline 的**前半段停住**：IF、OF 不能有新動作，直到 OF 裡的指令讀到對的值。

【課本補充】課本 p.434：為什麼要等到第 6 個 cycle（3 個 bubble，不是 2 個）？因為**同一個 cycle 不能讀和寫同一個暫存器**，要讓寫（比較早的指令）先，讀要多等一個 cycle。

[24:30] 怎麼插 bubble：做一個新的 instruction packet。【課本補充】課本 p.434 說有兩種：

1. packet 裡加一個 **bubble bit**，是 1 就當 bubble。控制訊號還是開著，但硬體要保證 bubble 改不了暫存器和記憶體。
2. 把 opcode 改成 nop、控制訊號全部設 0。比較侵入，但完全不做多餘的事。

【課本補充】課本 Definition 74（p.435）：**pipeline bubble** 是 interlock 硬體動態插進 pipeline register 的 nop，像普通指令一樣往下走。課本 p.435 也提到：`div`、`mod` 這種慢指令在 EX 要好幾個 cycle（每個叫一個 **T state**），也要把 IF、OF 停 n − 1 個 cycle。

---

## 8. Control hazard 的 bubble

[25:32] 沒有 delayed branch 時，branch 到 EX 才知道方向，IF、OF 已經抓了後面兩條。如果 branch **taken**，那兩條在 wrong path 上，不能執行。

[26:17] 做法：把它們**變成 bubble**（bubble bit 設 1，或改成 nop），讓它們在 pipeline 裡被作廢。它們還在 IF、OF，還沒寫 register file 和記憶體，所以可以安全作廢。

[27:28] 如果 branch **not taken**，那兩條本來就在對的路徑上，繼續做就好。

課本 Figure 10.15（p.436）：taken 的 `beq` 在第 3 個 cycle 決定，第 4 個 cycle 把指令 2、3 變成 bubble，同時從 `.foo` 抓指令 4。

---

## 9. 偵測衝突的演算法（Algorithm 5）

[28:27] Data-lock：指令到 OF 時，檢查它跟 **EX、MA、RW** 裡的指令有沒有 RAW 衝突；有就停，沒有就繼續。

[28:45] 「衝突」= RAW 相依。設 [A] 是 OF 裡的指令（後面那條），[B] 是後面某個 stage 裡的指令（前面那條）。課本 Algorithm 5（p.437），影片 [29:27]：

```
1. A 是 nop、b、beq、bgt、call → 不讀任何暫存器 → 沒衝突
2. B 是 nop、cmp、st、b、beq、bgt、ret → 不寫任何暫存器 → 沒衝突
3. src1 ← A.rs1；src2 ← A.rs2
     A 是 st  → src2 ← A.rd（store 要存的值放在 rd）
     A 是 ret → src1 ← ra
4. dest ← B.rd
     B 是 call → dest ← ra
5. hasSrc1 ← true；A 是 not、mov → false（它們的來源在 rs2）
6. hasSrc2 ← true；A 不是 st 且 A.I = 1（第二個運算元是常數）→ false
7. (hasSrc1 且 src1 = dest) 或 (hasSrc2 且 src2 = dest) → 有衝突
```

- [33:18] `not` 和 `mov` 只有一個來源，指令格式把它放在 rs2，所以 rs1 不算。為什麼這樣設計，要回去看第 3 章的格式。
- [37:00] 算完後有三個 4-bit 欄位（src1、src2、dest）和兩個 boolean（hasSrc1、hasSrc2），比較就很簡單了。
- [38:18] 這個演算法要做成**硬體電路**。要 3 個偵測器：OF↔EX、OF↔MA、OF↔RW。
- [38:33] 【口頭】讓它變複雜的，是一堆**特例**：store 把來源放在 rd、`ret` 讀 ra、`call` 寫 ra、immediate 格式。「這就是為什麼簡單的指令集很重要。」格式再多一點，邏輯會複雜很多，還好只有兩張投影片。

⚠️ 影片說「store 的第二個來源在 rd」（[31:57]）、「ret 的來源是 ra」，跟課本一致；這一段字幕 [30:45] 整段重複亂碼，以課本為準。

---

## 10. 怎麼停住 pipeline：data-lock unit

[39:44] 偵測到衝突後：

1. **關掉 PC 和 IF-OF pipeline register 的寫入**：它們不接受新資料，保持原來的值 → IF、OF 停住。
2. **在 OF-EX register 寫入一個 bubble**（nop 的 instruction packet）。下一個 cycle 它往 EX-MA 走，跟普通指令一樣。

[41:50] Data-lock unit 讀所有 pipeline register 的內容（裡面就是各 stage 的指令），偵測到衝突就送 **stall** 訊號給 fetch unit 和 IF-OF register。

[42:37] 【口頭】Stall 訊號最簡單的做法：一個 **AND gate**，一邊接 clock、一邊接 NOT stall。stall = 0 時輸出就是 clock；stall = 1 時輸出是 0，暫存器收不到 clock，就不會更新。

[43:51] 整個 pipeline 幾乎不變，只是多一個 data-lock unit：從所有 stage 拿輸入、停住前兩個 stage、在一個 stage 插 bubble（課本 Figure 10.16，p.437）。

---

## 11. Branch-lock：predict not taken

[44:10] 三個選項：

| 選項 | 做法 | 評價 |
|---|---|---|
| 1 | Delay slot（軟體） | 不用 interlock |
| 2 | branch 到 EX 時，**不管結果**都把 IF、OF 那兩條變成 bubble | 浪費 |
| 3 | **只有 taken** 才把那兩條變 bubble；not taken 就繼續 | ✅ 比較好 |

[46:57] 選項 3 等於「**predict not taken**」：先假設 branch 不跳，抓 p + 4、p + 8；猜錯（taken）才作廢。

[47:43] Branch-lock unit 只需要 branch unit 給的 1 bit（taken 或不 taken）。taken 時送 bubble 訊號給 IF-OF 和 OF-EX register。

[49:09] Bubble 訊號可能來自 data-lock unit，也可能來自 branch-lock unit，所以中間有一個 OR gate。

【課本補充】課本 p.439：如果 branch 有 30% 是 not taken，選項 3 那 30% 的時間有做事，選項 2 永遠浪費 2 個 cycle。課本只列兩個硬體選項（就是影片的 2、3）。Important Point 14：branch 的 PC 是 p，接下來兩個 cycle 抓 p + 4、p + 8；taken 才取消。

---

## 12. Forwarding 的想法

[50:13] Interlock 的核心就是「bubble 和 stall」。但不一定要停：在 stage 之間**直接傳資料**，可以省掉很多 bubble。

[51:12] 同一個例子：

```
[1]: add r1, r2, r3
[2]: sub r4, r1, r2
```

[51:48] Interlock 要插 3 個 bubble，那一大塊都是浪費。

[52:27] 關鍵觀察：

- 指令 2 **真的需要** r1 的時候，是第 4 個 cycle（在 EX，要拿去算）。OF 只是讀，不用它。
- 指令 1 **算出** r1 的時候，是第 3 個 cycle 結束（EX 做完），不是第 5 個 cycle 寫回的時候。
- 第 4 個 cycle：r1 已經在指令 1 的 packet 裡（它在 MA），指令 2 在 EX 正需要。

[55:18] 到目前為止，指令之間傳資料只有一條路：**register file**（一條寫、一條讀）。但這裡值已經在另一個 stage 裡了，**拉一條線**把 MA 接到 EX 就好。

[56:17] 這叫 **forwarding**。不需要 bubble、不需要隔 3 條、不需要插 nop。

【課本補充】課本 Definition 75（p.440–441）：forwarding 是在不同 stage 的指令之間，用 stage 之間的直接連線傳運算元的值，不經過 register file。判斷能不能 forward：在 pipeline diagram 上畫箭頭，**箭頭不能往回走（不能回到過去）**。

---

## 13. 四條 forwarding path

[57:38] 規則（課本 §10.7.2，p.441）：

1. 從**後面的 stage** 往**前面的 stage** forward（較早的指令給較晚的指令）。
2. **盡量晚** forward：越接近真正使用的地方越好。某個 stage 不用這個值，就等到後面要用的 stage 再拿。

這兩條規則不影響正確性，只是去掉多餘的路徑。

[58:28] 推論：

- **IF 不在任何 forwarding path 上**：IF 還沒 decode，根本不知道要什麼運算元。
- **OF 通常不用**：OF 沒有功能單元，不立刻用值；讓它走到 EX 再拿就好（MA → OF、EX → OF 不需要，用 RW → EX、MA → EX 代替）。
- **唯一例外 RW → OF**：RW 之後就沒有 stage 了，producer 下一個 cycle 就離開 pipeline，沒辦法「晚一點再 forward」。

[63:33] 所以只有**四條**：

| Path | 例子 | 說明 |
|---|---|---|
| **RW → MA** | `ld r1, 4[r2]` → `st r1, 10[r3]` | [64:21] load 在 MA 結束時拿到 r1，store 在 MA 要存它 → load、store 可以**背靠背**執行 |
| **RW → EX** | `ld r1` → （無關的 `st r8`）→ `add r2, r1, r4` | [65:53] 中間夾一條無關的 |
| **MA → EX** | `add r1` → `sub r4, r1, r2` | [67:35] 就是 §12 的例子，ALU 指令背靠背 |
| **RW → OF** | `ld r1` → 兩條無關 → `sub r7, r1, r2` | [68:27] 這時候讀 register file 拿不到（RW 正在寫），只能 forward |

⚠️ RW → EX 的例子，影片 [66:30] 把第三條說成「store」，課本 Figure 10.21（p.442）是 `add r2, r1, r4`。

[70:34] 【口頭】RW → OF「某種意義上違反了規則」（應該盡量晚 forward），但沒得選。

【課本補充】課本 Important Point 15（p.443–444）：RW → OF 很微妙：RW 在寫 register file，OF 在讀同一個暫存器，**同一個 cycle 對同一個 SRAM cell 又讀又寫可能出錯**。標準做法：讓 RW 的寫入照常進行，**取消** OF 的讀取，OF 改從 forwarding path 拿值。

---

## 14. Load-use hazard：forwarding 也救不了

[70:59] Forwarding 消掉了很多 hazard，但不是全部。剩下一個特例：**load-use hazard**。

[71:36] load 的下一條就用 load 的結果：

```
[1]: ld  r1, 10[r2]
[2]: sub r4, r1, r2
```

[72:41] load 到 **MA 結束**才拿到值，最早第 5 個 cycle 才能 forward；`sub` 第 4 個 cycle 在 EX 就要用。箭頭要**往回走**，不可能。

[73:23] 唯一的解法：插**一個 bubble**，然後用 forwarding path。

[73:58] 所以每個功能單元的輸入都要能在「預設輸入」和「forward 來的輸入」之間選，pipeline 會多一些複雜度，下一講講。

【課本補充】課本 Definition 76 與 p.445：

- 如果 producer 是 **ALU 指令**，一定能 forward 給任何 consumer。會寫暫存器的只有 ALU 指令和 load（store 不寫暫存器），所以 **load-use 是唯一要停的情況**。
- 解法：硬體插一個 bubble，或 compiler 重排、插 nop。
- **load 後面接 store、store 存的就是 load 的值**，不用停：store 在 MA 才要那個值，用 RW → MA 就好。

---

## 15. 【實測】

程式在 `experiments/lecture31/`。用兩個之前寫好的模擬器：

- `experiments/lecture30/pipeline.py`：課本 Figure 10.9 的五級 pipeline，**沒有 interlock、沒有 forwarding**（第 30 支）。
- `experiments/lecture32/pipeline_fwd.py` 的 `lock` 模式：有 data-lock 和 branch-lock、沒有 forwarding（第 32 支寫的）。

### 15.1 Example 138：插 nop 和重排

初值 r2=1、r3=2、r5=3、r6=4、r11=5、r12=6，正確答案是 r4 = 6、r9 = 10、r13 = 13。

```
$ python3 -I ../lecture30/pipeline.py ex138.s r2=1 r3=2 r5=3 r6=4 r11=5 r12=6
10 cycles; ... r4=3 ... r9=3 ... r13=2            ← 三個都錯（讀到舊值 0）
$ python3 -I ../lecture30/pipeline.py ex138_sched.s ...
11 cycles; ... r4=6 ... r9=10 ... r13=13          ← 課本重排 ＋ 1 個 nop：全對
$ python3 -I ../lecture32/pipeline_fwd.py ex138.s lock ...
ex138.s [lock]: 19 cycles, 6 insts, CPI=3.167, stalls=9 ... r4=6 r9=10 r13=13
$ python3 -I ../lecture32/pipeline_fwd.py ex138_sched.s lock ...
ex138_sched.s [lock]: 11 cycles, 6 insts, CPI=1.833, stalls=0 ...
```

| 做法 | cycle 數 | 結果 |
|---|---|---|
| 原本的程式、沒有硬體保護 | 10 | ❌ 錯 |
| 課本重排 ＋ 1 個 nop（軟體） | 11 | ✅ |
| 原本的程式 ＋ interlock（硬體） | 19（停了 9 次） | ✅ |
| 重排後的程式 ＋ interlock（兩個一起） | 11（不用停） | ✅ |

- 硬體 interlock 不用改程式就對，但慢很多：3 組 RAW × 3 個 bubble = 9 次 stall。正好是表 10.1「基本版比軟體慢」。
- 先讓 compiler 重排、再交給 interlock 保底，就是課本 p.433 說的「兩邊的好處」：一樣是 11 cycle，而且就算 compiler 排錯也不會出錯。

### 15.2 Example 139：delayed branch

`ex139.s` 是原本的程式，`ex139_delay.s` 是課本放進 delay slot 的版本；`.foo` 後面接一條 `add r11, r9, r10`。正確答案：`add r8` 要被**跳過**（r8 = 0），r11 = 15。

```
$ python3 -I ../lecture30/pipeline.py ex139.s r2=1 r3=2 r5=3 r6=4 r9=7 r10=8
10 cycles; r1=3 ... r4=7 ... r8=15 ... r11=15     ← r8 被改了：wrong path 的 add r8 執行了
$ python3 -I ../lecture30/pipeline.py ex139_delay.s ...
8 cycles; r1=3 ... r4=7 ... r11=15                ← 對：r8 沒動，而且只要 8 cycle
$ python3 -I ../lecture32/pipeline_fwd.py ex139.s lock ...
ex139.s [lock]: 10 cycles, 4 insts, ... flushed=2 ... r11=15   ← 對：作廢了 2 條
```

- 這個沒有 interlock 的 pipeline，branch 後面兩條**一定會執行**，所以它天生就有 2 個 delay slot。程式沒照 delay slot 寫，就會錯（r8 = 15）。
- 照課本把兩條搬進 delay slot：對，而且 8 cycle。
- Branch-lock（predict not taken）：b 是 taken，作廢 2 條（`flushed=2`），對，但要 10 cycle，多 2 個 cycle，正好是表 10.1 說的「要停 2 個 cycle」。

⚠️ 本機沒有 MIPS 這類有 delay slot 的 cross compiler（只有 `aarch64-linux-gnu-gcc`，ARM 沒有 delay slot），所以沒辦法看真實 compiler 怎麼填 delay slot，**待補**。

---

## 16. 一頁速查表

| 主題 | 重點 |
|---|---|
| RAW 要隔幾條 | producer 和 consumer 之間至少 3 條 |
| 插 nop | 對，但浪費 cycle |
| 重排 | compiler 把無關的指令塞進去，nop 越少越好（Example 138：9 → 1） |
| Delay slot | branch 後面 2 條一定執行；從前面搬無關的指令進去（不能搬 cmp） |
| Delayed branch | n 個 delay slot = branch 抓進來到決定之間抓了 n 條 |
| 軟體的問題 | 不可攜、綁 compiler、要公開硬體細節 |
| Data-lock | 拿到對的值之前不能離開 OF；停 IF、OF |
| Branch-lock | 絕不執行 wrong path 的指令 |
| Bubble | interlock 動態插的 nop；bubble bit 或改成 nop |
| 3 個 bubble | 同一 cycle 不能讀寫同一個暫存器，讀要等一個 cycle |
| Algorithm 5 | 誰不讀／誰不寫；st 的 src2 = rd；ret 讀 ra；call 寫 ra；not／mov 沒 src1；I = 1 沒 src2 |
| Stall | 關掉 PC、IF-OF 的寫入（clock AND NOT stall）；OF-EX 寫 bubble |
| Branch-lock 做法 | predict not taken：taken 才把 IF-OF、OF-EX 變 bubble |
| Forwarding | stage 之間直接接線；箭頭不能往回走 |
| 規則 | 後 → 前；盡量晚 |
| 四條 path | RW→MA、RW→EX、MA→EX、RW→OF |
| RW→OF | RW 之後沒 stage 了；取消 OF 的讀、改用 forward |
| Load-use | load 下一條就用 → 停 1 個 cycle；load 接 store 不用停 |
| 實測 | Example 138：沒保護錯、重排 11 cycle、interlock 19 cycle；Example 139：沒照 delay slot 寫會執行 wrong path，delay slot 版 8 cycle、branch-lock 10 cycle |

---

## 17. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Pipeline、instruction packet、三種 hazard | [第 30 支](lecture30_note.md) |
| nop 指令 | [第 8 支](lecture08_note.md) |
| 指令格式（為什麼 not／mov 的來源在 rs2、st 的值在 rd） | [第 8 支](lecture08_note.md) |
| Forwarding 的實作、M1–M6、forwarding 條件 | [第 32 支 §1](lecture32_note.md#1-四條-forwarding-path複習) 起 |
| Interlock ＋ forwarding、load-use 停 1 個 cycle | [第 32 支 §7](lecture32_note.md#7-interlock-和-forwarding-一起用) |
| 真正的 branch prediction | [Advanced 第 8 支](../advanced/lecture08_note.md) |

---

## 附：本講一句話總結

> **RAW hazard 和 control hazard 可以靠軟體解：compiler 讓每個 producer 和 consumer 之間至少隔 3 條指令，先重排、不夠再補 nop（Example 138 從 9 個 nop 變 1 個），branch 則用 delayed branch，把 branch 前面兩條無關的指令搬進後面的兩個 delay slot；但軟體解法不可攜、綁死 compiler、要公開硬體細節，所以硬體用 interlock 保證正確：data-lock 用 Algorithm 5 檢查 OF 裡的指令和 EX、MA、RW 有沒有 RAW 衝突，有就關掉 PC 和 IF-OF 的寫入、往 OF-EX 塞 bubble，branch-lock 用 predict not taken，branch taken 才把 IF、OF 那兩條變成 bubble；interlock 的基本版比軟體慢，更好的做法是 forwarding：值已經在後面的 stage 裡，就直接接線傳過去，依「後到前、盡量晚」只需要 RW→MA、RW→EX、MA→EX、RW→OF 四條路徑，唯一救不了的是 load 後面立刻用它的 load-use hazard，要停 1 個 cycle。本機模擬：Example 138 沒保護會算錯，重排後 11 cycle、只靠 interlock 要 19 cycle；Example 139 沒照 delay slot 寫會執行到 wrong path，放進 delay slot 只要 8 cycle，branch-lock 要 10 cycle。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 9: Pipelining, Part 2/4 錄影逐字稿（YouTube 自動字幕，1:14:39）、Sarangi *Basic Computer Architecture* v3.09 Ch10 §10.5–10.7.3（p.428–445）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
