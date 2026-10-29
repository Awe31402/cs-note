# Advanced Computer Architecture 第十六支影片完整筆記：沒有 PRF 的簡單 OOO、compiler 最佳化與 software pipelining（Chapter 5, Part III）

> **課程**：Advanced Computer Architecture — Chapter 5: Alternative Approaches to Issue and Commit (Part III) -- Compiler Techniques（播放清單第 16 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 自動字幕是亂碼）（1:30:46）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch5** §5.3（p.173–178）、§5.4（p.178–194）
> **章號**：影片 Chapter 5 = 課本 Ch5。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 老師自己在投影片上改了兩個錯：[12:16] 「broadcast 值」那句少了一個逗號；[28:59] strength reduction 的表「slow／fast 寫反了」。筆記照改正後的寫。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. PRF 設計的麻煩](#1-prf-設計的麻煩)
- [2. 用 ARF ＋ 胖 ROB](#2-用-arf--胖-rob)
- [3. RAT、ROB、window 要怎麼改](#3-ratrobwindow-要怎麼改)
- [4. Broadcast 要帶值](#4-broadcast-要帶值)
- [5. PRF 對 ARF](#5-prf-對-arf)
- [6. 為什麼要 compiler 幫忙](#6-為什麼要-compiler-幫忙)
- [7. 五種基本最佳化](#7-五種基本最佳化)
- [8. 迴圈最佳化](#8-迴圈最佳化)
- [9. Loop unrolling](#9-loop-unrolling)
- [10. Software pipelining：想法](#10-software-pipelining想法)
- [11. Software pipelining：用不同暫存器](#11-software-pipelining用不同暫存器)
- [12. Prologue 和 epilogue](#12-prologue-和-epilogue)
- [13. 不展開的 software pipelining](#13-不展開的-software-pipelining)
- [14. 展開 ＋ 多個暫存值](#14-展開--多個暫存值)
- [15. 【實測】](#15-實測)
- [16. 一頁速查表](#16-一頁速查表)
- [17. 與後續講次的連結](#17-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
前半（硬體）：比較簡單的 OOO
  不要 physical register file，用 architectural register file（ARF）存已 commit 的值
  還沒 commit 的值放在 ROB；window 存運算元的值；broadcast tag ＋ 值
  → 回復簡單、沒有 free list；但值存很多份、broadcast 很胖
後半（軟體）：compiler 最佳化
  constant folding、strength reduction、CSE、dead code、silent store
  迴圈：loop invariant、induction variable、loop fusion、loop unrolling
  software pipelining：把一圈拆開、跟別圈交錯，load 和 use 之間塞別的事
下一講：把 compiler 技巧推到極致 → EPIC／Itanium
```

---

## 1. PRF 設計的麻煩

[00:00] 前幾講一直加複雜度：推測、replay、輕量的狀態回復。現在看一個**比較沒效率、但不同**的設計，「做一點小手術，把幾個單元搬一下」。

[00:56] 到目前為止是 **PRF**（physical register file）設計：又快又有效率，但：

- 狀態回復很複雜：要 RRF 或 RRAT 之類的東西（[第 13 支](lecture13_note.md)）。
- 管理實體暫存器很麻煩。

這些缺點都是**複雜度**。

---

## 2. 用 ARF ＋ 胖 ROB

[01:23] 之前架構暫存器只是「概念上」存在。新設計：真的做一個 **architectural register file（ARF）**，像 RRF 一樣。

[02:36] ARF 存**已 commit 的狀態**，所以回復不成問題：回到 ARF 就好。

[02:58] 那還沒 commit 的值（以前放在 PRF）放哪？**放在 ROB**：每格多存值，ROB 變得「更寬、更胖」。

[03:29] 流程：

```
decode → rename（一樣）
  → 讀暫存器（順序跟以前對調：先讀，再 dispatch）
      RAT 說值在 ARF → 讀 ARF；值還在飛 → 讀 ROB
  → dispatch 進 window → wakeup／select／broadcast（一樣）
  → execute → 結果寫進 ROB 那一格
  → commit：值從 ROB 搬到 ARF
```

[06:00] tag 不再是實體暫存器編號，而是 **ROB entry 的編號**。

[06:45] 簡單在哪：PRF、free list 等等跟 PRF 有關的電路**都不要了**。

---

## 3. RAT、ROB、window 要怎麼改

[07:30] **RAT 的每一格**：

| 欄位 | 意思 |
|---|---|
| **ROBRF bit** | 1：值在 ROB；0：值在 ARF |
| **ROB entry 編號** | 值在 ROB 的話，在哪一格 |

⚠️ 影片叫它「ROBRF bit」（1 = 在 ROB），課本 p.174 叫 **inARF**（1 = 在 ARF），意思相反。

[09:00] **ROB 的每一格**（只看相關的）：

| 欄位 | 意思 |
|---|---|
| 目的暫存器的**值** | 算好了就有 |
| **ready bit** | 值已經在 ROB 裡了，還是還在 pipeline 裡算（要靠 wakeup、broadcast、bypass 拿） |

這個 ready bit 就像舊設計 RAT 裡的 available bit。

[10:39] **Instruction window 的每一格**要存**運算元的值**：讀暫存器已經在前面做完了，之後不會再讀 register file。

- [11:15] 每格變胖 → 面積大、延遲大（結構變慢）、耗電。「是全面性的壞主意。雖然比較簡單，但簡單和效率向來不同路。」

【課本補充】課本 p.177：commit 時要把 ROB 的值寫進 ARF，並把 RAT 的 inARF 設 1。但如果 commit 的指令 I 之後又有一條 I′ 也寫同一個暫存器，就**不能**設：要比較 RAT 那格的 ROB 編號是不是 I 的。所以要多一個比較器。

---

## 4. Broadcast 要帶值

[12:16] 除了 tag，還要 **broadcast 值**。理由：

1. 一條指令可能這個 cycle 醒了，**10 個 cycle 後**才被選中。
2. 可能一個運算元醒了、另一個還沒醒。

這兩種情況，值不會一直留在 bypass network 上，所以 window 要**自己先存起來**。

[15:04] 代價：ROB 128 格 → tag 7 bit（跟以前一樣），但現在還要 broadcast 一個**胖的 64-bit 值**，電路變慢。

[15:30] 其他改動：

- 一樣用推測的 wakeup，select 之後馬上 broadcast（tag ＝ ROB 編號，加上值）。
- 指令從 select 直接到執行單元，**中間沒有讀 register file 那一級**。
- 執行完寫進 ROB 那一格。

[16:20] **Commit 很簡單**：把值寫進 ARF。但每次 commit 都寫，**很耗電**（老師在投影片上打了「− −」）。運作方式跟 RRF 一樣。

[16:58] 猜錯分支時：flush ROB，從 ARF 回復，改一下 rename table 就好。

【課本補充】課本 Historical Note 1（p.177）：最早的 OOO 設計之一是 **Robert Tomasulo 1967** 年的。他把小的 instruction window 接在每個功能單元旁邊，叫 **reservation station**，所有 reservation station 接在一條 **common data bus（CDB）** 上 broadcast tag 和值，概念上跟這個 ARF 設計很像。現代設計常常依指令類別分開 window（整數、浮點、SIMD）：小 window 比較快、省電；統一的 window 則比較能平衡各類功能單元的負載。

---

## 5. PRF 對 ARF

[17:44]

| | PRF 設計 | ARF 設計 |
|---|---|---|
| 值存在哪 | **只有一個地方**（PRF） | **好幾個地方**：ARF、ROB、window |
| Window 每格 | 小（不存值） | 胖（存值） |
| Broadcast | 7-bit tag | tag ＋ 64-bit 值 |
| 狀態回復 | 複雜 | 簡單（ARF 就是已 commit 的狀態） |
| Free list、管理實體暫存器 | 要 | 不要 |
| Commit | （RRAT 時）寫 7 bit | 每次寫 64-bit 值進 ARF |

[19:04] 這是**延遲、功耗和複雜度**之間的取捨。要看目標的工作負載、功耗預算、想要多少效能，還有「工程師的能力：他們應付得了比較複雜的設計嗎？」

【課本補充】課本 Table 5.2（p.177）是同一張比較表。

---

## 6. 為什麼要 compiler 幫忙

[20:18] 硬體的部分到這裡**全部講完了**。接下來是軟體技巧：compiler 先最佳化 80%，硬體再做剩下的 20%。

[20:51] 最極致的例子是 **Intel Itanium**：scheduling、dispatch、rename……幾乎全部交給軟體。

[21:36] Itanium 是 Intel 和 HP 合作的高階伺服器處理器（Itanium 1、2，「我不確定有沒有 3」），非常依賴 compiler。後來並不流行。【口頭】「流行有很多因素，看市場。學術討論和市場討論要分開，它們屬於兩門不同的課。」

[22:21] compiler 能做什麼？In-order pipeline 時就看過了（[第 4 支](lecture04_note.md)）：

- 重排指令避免 stall（在 load-use 之間塞別的指令）。
- 用 delayed branch 把分支前的指令搬到 delay slot。
- 硬體沒有 interlock 時保證正確。

還能做更多：

| 目標 | 做法 |
|---|---|
| 減少動態指令數 | 各種最佳化 |
| 增加 ILP | 找出可以一起抓、一起跑的指令 |
| 用快的指令取代慢的 | **strength reduction** |

[23:59] 【口頭】大學部的 compiler 課大多在教 **lexing 和 parsing**（前端）：lexer 把 C 程式切成 token，parser（例如 yacc）做出 parse tree。研究所的課假設這些都很簡單，專注在 **abstract syntax tree（AST）**：像生產線一樣跑很多個 **compiler pass**，一直把它變得更有效率，最後產生程式碼。

[25:08] 最佳化等級：`gcc x.c`（預設）、`-O1`、`-O2`、`-O3`。pass 越多，程式越快，編譯越久。【口頭】「我邀請你試試看：拿一個 C 程式，用不同的最佳化等級編譯、量時間，你會看到它越來越快。」（本機試了，見 [§15.2](#152-最佳化等級越高越快嗎)。）

---

## 7. 五種基本最佳化

### 7.1 Constant folding

[26:25]

```c
a = 4 + 6;      →  a = 10
b = a * 2;      →  b = 20
c = b * b;      →  c = 400
```

[27:36] 省掉兩個乘法、兩個加法，還有把常數放進暫存器的指令（大部分 ISA 不能兩個運算元都是 immediate）。這是大部分 compiler **預設**的 pass。

【口頭】「程式設計師想朝自己腳開槍時，compiler 大多數時候會給他們防彈鞋。」

### 7.2 Strength reduction

[27:58] 乘法、除法很慢（很多 cycle），換成 1 cycle 的指令：

| 慢 | 快 |
|---|---|
| `x * 8` | `x << 3` |
| `x / 4` | `x >> 2` |
| `x * 12` | `(x << 3) + (x << 2)`：兩個 shift ＋ 一個 add |

[30:00] `× 12` 那種：一個運算換成三個，看機器；兩個 shift 可以平行，有三個功能單元的話不錯。這種比較少見，前兩種非常普遍。今天 strength reduction 是大部分 compiler 預設的。

### 7.3 Common subexpression elimination（CSE）

[30:31]

```c
c = (a + b) * 10;          t1 = a + b;
d = (a + b) * (a + b);  →  c = t1 * 10;
                           d = t1 * t1;
```

a + b 只算一次，放在暫存器 t1。

[32:30] 一串算術式子，可以結合 constant folding、strength reduction、CSE，變成更短、更快的指令序列。

### 7.4 Dead code elimination

[33:15] 程式最後重要的只有**輸出**。算出來卻沒有影響 printf 的程式碼叫 **dead code**，可以刪掉。

[34:00] ⚠️ 老師說「至少到今天，這在很多 compiler 都不是預設的最佳化等級，要 -O3 之類的才會找出來」。本機的 gcc 在 **-O1** 就刪了（`-O0` 才會留著，而 `-O0` 正是 gcc 的預設），見 [§15.1](#151-gcc-真的有做嗎)。

### 7.5 Silent store

[34:48] dead code 更進一步的版本，編譯時常常很難認出來：

```c
arr[0] = 3;
…（中間沒有改 arr[0]）
arr[0] = 3;     ← silent store：寫的值本來就在那裡
```

[36:08] 這種 store 可以、也應該刪掉。

---

## 8. 迴圈最佳化

[36:23] 程式大部分時間（有人說 **90%**）在迴圈裡，所以迴圈最佳化特別重要。

### 8.1 Loop invariant

[36:51]

```c
for (i = 0; i < n; i++) {         val = 5;
    val = 5;                  →   for (i = 0; i < n; i++) {
    …                                 …
}                                 }
```

`val = 5` 每一圈都一樣（**loop invariant**），n 是一百萬就做一百萬次，而且每次都是 silent store。移到迴圈前面。

### 8.2 Induction variable

[38:39]

```c
for (i = 0; i < n; i++) {         j = 0;
    j = 6 * i;                →   for (i = 0; i < n; i++) {
    … 用 i 和 j …                     … 用 i 和 j …
}                                     j = j + 6;
                                  }
```

j 由迴圈變數 i 推導出來，叫 **induction variable**。乘法換成加法。

### 8.3 Loop fusion

[40:40]

```c
for (i = 0; i < n; i++) a[i] = 0;        for (i = 0; i < n; i++) {
for (i = 0; i < n; i++) b[i] = 0;   →        a[i] = 0; b[i] = 0;
                                          }
```

少掉第二個迴圈的分支、檢查等額外負擔，指令數和分支數都少很多。

[41:30] compiler 會做，但迴圈複雜時可能認不出來，**程式設計師自己要寫好**。

---

## 9. Loop unrolling

[42:38] 「這方面可能最流行的最佳化」。以前 in-order pipeline 時更流行，現在也是；它也是更進階最佳化的基礎；寫 Verilog、VHDL 的迴圈也會被展開。

例子：`for (i = 0; i < 10; i++) sum = sum + i;`，r0 = sum、r1 = i：

```
.loop: cmp r1, 10
       beq .exit
       add r0, r0, r1
       add r1, r1, 1
       b .loop
```

[44:10] 每兩圈合成一圈（**unrolling factor** = 2）：i 每次加 2，圈數從 10 變 5：

```
.loop: cmp r1, 10
       beq .exit
       add r0, r0, r1     // sum += i
       add r1, r1, 1
       add r0, r0, r1     // sum += i+1（也可以用 2i+1 算）
       add r1, r1, 1
       b .loop
```

[47:01] 好處：

| 好處 | 說明 |
|---|---|
| 指令變少 | 迴圈本身的比較、分支變少 |
| **分支變少** | 重點：猜錯變少；存取 branch predictor 也要時間；迴圈裡的分支可能互相 alias、讓猜錯變多 |
| In-order | 跟分支有關的 stall 沒了 |
| OOO | 有一大段**沒有分支**的指令，排程邏輯可以排得很好（展開 5 次的話更大） |

[49:26] 壞處：

| 壞處 | 說明 |
|---|---|
| **程式碼變大** | instruction cache 壓力大 |
| 要更多暫存器 | 才有好效能（software pipelining 一整節都在講這個） |

---

## 10. Software pipelining：想法

[50:34] 在軟體裡做硬體 pipeline 做的事（抽象、概念上）。

[50:58] 例子：兩個 300 元素的陣列，`a[i] = b[i]`。r0 = a 的基底、r1 = b 的基底、r2 = i、r10 = offset：

```
.loop: cmp r2, 300
       beq .exit
       add r3, r1, r10    ┐ L：load b[i] 到 r5
       ld  r5, 0[r3]      ┘
       add r4, r0, r10    ┐ S：把 r5 存到 a[i]
       st  r5, 0[r4]      ┘
       add r2, r2, 1      ┐ I：i++；offset = i × 4（用 lsl 2，不用乘法）
       lsl r10, r2, 2     ┘
       b .loop
```

[54:38] 三個區塊：**L、S、I**，每個兩條指令。In-order pipeline 上，load 和 use 之間隔了一條指令，load 1 cycle 時沒有 load-use stall；load 要 2、3、4 個 cycle 就有問題了。

[56:04] 把每一圈的 L S I 排成斜的，再**一列一列**地執行：

```
     第 0 圈  第 1 圈  第 2 圈  第 3 圈  …
列1  L⁰
列2  S⁰       L¹
列3  I⁰       S¹       L²
列4           I¹       S²       L³
…
穩定狀態：I⁰ S¹ L²，I¹ S² L³，I² S³ L⁴，…（上標 = 第幾圈）
```

[57:25] 執行的區塊**跟原本完全一樣**，只是**順序不同**。

[58:50] 為什麼叫 pipelining：把「一圈」當成一條指令，把每一列當成一個 pipeline stage：一圈依序經過 L、S、I 三個 stage，同一列裡同時有好幾圈的不同部分。

[63:01] 好處：

1. L² 和 S² 之間多了一個 I¹ 區塊 → 可以**吸收多 cycle 的 load**（I 區塊 2 cycle 的話，load 可以多花 2 cycle）。
2. 同一列的 I、S、L 來自**不同圈**，沒有相依的話可以**平行**跑，ILP 更高。

---

## 11. Software pipelining：用不同暫存器

[66:00] 問題：直接照 I S L 的順序跑不對。例如先跑 I⁰ 會改掉 offset，S¹ 和 L² 用到的 offset 就錯了。

[68:21] 解法：**展開 3 次**，三組暫存器輪流用：

| 圈 | iterator | offset |
|---|---|---|
| 0、3、6、… | r6 | r10 |
| 1、4、7、… | r7 | r11 |
| 2、5、8、… | r8 | r12 |

[69:38] 9 個區塊：I⁰ S¹ L²，I¹ S² L³，I² S³ L⁴。每個 I 區塊的 iterator **加 3**（不是加 1），offset = iterator × 4。

[71:16] 檢查：r10 只有 0、3、6…圈用；I⁰ 算好 r10，下一個用 r10 的是 L³，正確。

[75:33] 但還有一個問題：S¹ 和 L² **不能平行**：S¹ 要讀 r5、L² 要寫 r5。平行跑就是 hazard（⚠️ 老師說「write after read」；這是 WAR）。

- [76:47] **OOO pipeline 沒問題**：renaming 會處理。
- **In-order** 有問題：而 software pipelining 主要是 in-order 時代發明的。

[77:58] 假設有 32 個暫存器：每一圈用**不同的一組**暫存器（例如 S¹ 用 r21、L² 用 r22……），三個區塊用的暫存器完全不重疊，就可以平行跑。

[79:32] 下一講的 Itanium 就很依賴這種做法。

[80:30] 重點：software pipelining **需要更多暫存器**（r13–r15 留給 stack pointer 之類的，老師不想動）。Itanium 有很多暫存器，硬體也直接支援「每一圈用不同的一組暫存器」。

---

## 12. Prologue 和 epilogue

[80:55] 斜的圖最前面（只有 L⁰、S⁰ L¹ 的那幾列）叫 **prologue**，最後面叫 **epilogue**。它們沒有被 pipeline。

[81:33] 300 圈的話大部分都在穩定狀態；compiler 在前後自動加一些程式碼，讓整個迴圈正確。

[82:05] 小結：對 compiler 來說，迴圈是「一整本**最佳化的百科全書**」。Software pipelining 能暴露 ILP，在 OOO 以前、或 Itanium 那種特殊 pipeline 上特別重要。有了 renaming 之後它沒那麼耀眼，但對主要是 in-order、又要應付很長的多 cycle 延遲的簡單處理器，仍然很重要。

---

## 13. 不展開的 software pipelining

[83:22] 另一種，不用 renaming、不用展開：

```c
i = -1;
t = b[0];
while (i < 298) {    // i 從 -1 到 297
    i++;             // i：0 到 298
    a[i] = t;        // 用上一圈讀好的 t
    t = b[i + 1];    // 先讀下一圈要的
}
a[299] = t;          // 最後一個
```

[86:15] 上一圈讀的 b[i+1] 就是這一圈的 b[i]，經過 t 傳過來。跨圈的相依叫 **loop-carried dependence**。

[87:33] load 在這裡，store 在下一圈：中間隔了迴圈的檢查和 i++ → 也能吸收多 cycle 的 load。

---

## 14. 展開 ＋ 多個暫存值

[88:00] 例子：`a[i] = b[i] * 5`。展開 3 次，用三個暫存值：

```c
for (i = 0; i < n; i += 3) {
    t1 = b[i];     t2 = b[i+1];   t3 = b[i+2];
    t4 = t1 * 5;   t5 = t2 * 5;   t6 = t3 * 5;
    a[i] = t4;     a[i+1] = t5;   a[i+2] = t6;
}
```

[89:20] 讀 b[i] 到用它之間隔了兩條指令。load 很慢也沒關係：pipeline 在忙著發別的 load。store 不在關鍵路徑上，load 才是；重點是把 load 和 use 的距離拉開。

[89:59] 五節裡做完四節了。最後一節：把所有 compiler 技巧放在一起 → **EPIC** 處理器，例子是 Intel Itanium。

---

## 15. 【實測】

程式在 `experiments/lecture16/`。

### 15.1 gcc 真的有做嗎

`opts.c` 把影片的每個例子寫成一個小函數，用 gcc 11.4 的 `-O0`／`-O1`／`-O2`／`-O3` 編譯成組合語言（`gcc -O1 -S opts.c`）來看：

| 例子 | `-O0`（gcc 預設） | `-O1` 以上 |
|---|---|---|
| Constant folding | `4 + 6` 已經變 `10`；`× 2` 變 `add`；但 `b * b` 還是 `imull` | `movl $400, %eax`：整個算好 |
| Strength reduction `× 8` | `sall $3`（**-O0 就做了**） | `leal 0(,%rdi,8)` |
| Strength reduction `× 12` | `x + x + x` 再 `sall $2`（-O0 就做了） | `leal (%rdi,%rdi,2)` ＋ `sall $2`：3x × 4 |
| CSE `(a+b)` | `addl` 算了 **3 次** | 只算 1 次 |
| Dead code | 留著 | 刪掉（只剩 `movl %edi, %eax`） |
| Silent store | | 只留**一個** `movl $3, (%rdi)`，放在迴圈**後面** |
| Loop invariant `*val = 5` | | 移出迴圈，放在迴圈**後面** |
| Induction variable `6*i + i` | | 迴圈裡只有 `addl $7`，沒有乘法 |
| Loop fusion | | **沒有合併**；`-O2` 起把兩個迴圈各換成一次 `memset` |
| 10 圈的 `sum += i` | | `movl $45, %eax`：整個迴圈算掉了 |

- 大部分影片說的最佳化 `-O1` 就做了，有幾個（strength reduction、簡單的 constant folding）連 `-O0` 都做。
- **Silent store**：gcc 刪掉的是**第一個** `arr[0] = 3`（它會被後面的蓋掉，是 dead store），留下最後那個。結果跟影片「刪第二個」一樣，只是刪的是另一個。
- **Loop invariant**：gcc 不是把 `*val = 5` 移到迴圈**前面**，而是移到**後面**，然後再補寫一次 `a[n-1] = n-1`。原因：`val` 可能剛好指到 `a[]` 裡面（aliasing），移到前面就可能被 `a[i] = i` 蓋掉，結果不一樣。這說明 compiler 做這種搬動時要很小心。
- **Loop fusion**：gcc 沒有合併兩個迴圈，跟影片 [41:30]「複雜時 compiler 可能認不出來，程式設計師要自己來」相符（這裡其實不複雜，gcc 選擇了換成 `memset`）。

### 15.2 最佳化等級越高越快嗎

`levels.c`：對 4096 個 `unsigned` 重複做 `a[i] = a[i] * 12 + (i & 7)`，再全部加總（P-core cpu2，跑兩次）：

| 編譯 | ns/元素 |
|---|---|
| `-O0` | 1.69–1.75 |
| `-O1` | 0.43 |
| `-O2` | 0.38 |
| `-O3` | **0.86** |
| `-O2 -funroll-loops` | **0.33** |

- `-O0` → `-O1` 快了 4 倍；`-O1` → `-O2` 再快一點。跟影片 [25:08] 說的一樣。
- ⚠️ 但 **`-O3` 反而比 `-O2` 慢一倍多**：`-O3` 開了自動向量化（用 SSE 的 xmm 暫存器），在這個例子上反而變慢。原因這裡沒有細查，**待補**。所以「等級越高越快」不是永遠成立，要量。
- 加上 `-funroll-loops`（[§9](#9-loop-unrolling)）最快。看 `unr.c` 的組合語言：`-O2` 的迴圈每個元素有 `addq` ＋ `addq $8` ＋ `cmpq` ＋ `jne`；加了 `-funroll-loops` 之後展開成 8 份，中間不用比較和跳躍，前面再用一串 `cmpq`／`je` 處理不是 8 的倍數的剩餘部分（就是 [§12](#12-prologue-和-epilogue) 說的 prologue）。

### 15.3 Software pipelining 在 in-order pipeline 上

`swp.py`：一個**單發射、照順序**的 pipeline：一條指令要等來源暫存器都好了才能發出；load 要 L 個 cycle，其他 1 cycle；分支都猜對、沒有代價。跑 [§10](#10-software-pipelining想法) 的 `a[i] = b[i]` 迴圈的四種寫法，看每個元素平均要幾個 cycle：

```
load 延遲 L：      1      2      3      4      6
原本 LSI             9.00   9.00  10.00  11.00  13.00  cycle/元素
只展開 3 次            7.00   7.00   8.00   9.00  11.00  cycle/元素
影片的 software pipelining   7.00   7.00   7.00   7.00   8.33  cycle/元素
不展開、用 t 傳值        10.00  10.00  10.00  10.00  10.00  cycle/元素
```

- **原本的寫法**：load 超過 2 cycle，每多 1 cycle 每個元素就多 1 cycle（load 和 store 之間只隔一條 `add`）。
- **只展開**：迴圈的 cmp／beq／b 只做 1/3 次，少了 2 cycle；但 load 一慢還是一樣一直變慢。
- **影片的 software pipelining**（I⁰ S¹ L²…，三組暫存器）：L 從 1 到 4 都是 **7 cycle**，完全沒變慢，因為 load 和用它的 store 之間塞了一個 I 區塊。L = 6 時才開始藏不住。這就是 [§10](#10-software-pipelining想法) 說的「吸收多 cycle 的 load」。
- **不展開、用 t 傳值**（[§13](#13-不展開的-software-pipelining)）：每個元素多一條指令（算 b[i+1] 的位址），所以基本是 10 cycle，但 L 到 6 都**不變**：load 和下一圈的 store 之間隔了很多條。
- 這只是很簡化的模型（單發射、沒有 cache miss），用來看「load 延遲變大時誰會變慢」。

---

## 16. 一頁速查表

| 主題 | 重點 |
|---|---|
| ARF 設計 | 沒有 PRF；ARF 存已 commit 的值；在飛的值放 ROB；先讀暫存器再 dispatch |
| RAT | ROBRF bit（課本 inARF）＋ ROB 編號 |
| Window | 存運算元的值；tag = ROB 編號；broadcast tag ＋ 64-bit 值 |
| 優缺點 | ＋ 回復簡單、沒有 free list；− 值存三份、window 胖、broadcast 胖、commit 耗電 |
| Tomasulo 1967 | reservation station ＋ CDB（課本） |
| Compiler 能做的 | 減少指令、增加 ILP、strength reduction；在 AST 上跑很多 pass；-O1/-O2/-O3 |
| 五種基本 | constant folding、strength reduction（×8 → <<3）、CSE、dead code、silent store |
| 迴圈 | 90% 時間在迴圈；loop invariant、induction variable（×6 → +6）、loop fusion |
| Unrolling | ＋ 少分支、少指令、大段無分支程式；− 程式變大、要更多暫存器 |
| Software pipelining | 一圈拆成 L S I，跨圈交錯（I⁰ S¹ L²）；展開 3 次、每圈一組暫存器；load 和 use 之間塞別的；有 prologue、epilogue |
| 不展開版 | 用 t 傳值（loop-carried dependence） |
| 實測 | gcc -O1 就做了大部分；loop fusion 沒做；-O3 比 -O2 慢（⚠️）；software pipelining 在 load 1–4 cycle 時都 7 cycle/元素 |

---

## 17. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| PRF、RAT、free list | [第 10 支](lecture10_note.md) |
| RRF、回復 | [第 13 支](lecture13_note.md) |
| Broadcast、wakeup、window | [第 11 支](lecture11_note.md) |
| In-order 的 compiler 重排、delayed branch、load-use | [第 4 支](lecture04_note.md) |
| 分支 aliasing、猜錯 | [第 8 支](lecture08_note.md) |
| VLIW、EPIC、Itanium（rotating registers） | [第 17 支](lecture17_note.md) |
| Instruction cache 壓力 | [第 21 支](lecture21_note.md)（Ch7） |

---

## 附：本講一句話總結

> **硬體部分的最後一個設計是不用 physical register file 的簡單 OOO：已 commit 的值放在 architectural register file、還在飛的值放在變胖的 ROB、instruction window 也存運算元的值、broadcast 時連 64-bit 的值一起送，換來簡單的回復和不用 free list，代價是值存三份、結構變胖變慢又耗電；接著換成軟體，compiler 用 constant folding、strength reduction、CSE、dead code 和 silent store 刪除減少指令，用 loop invariant、induction variable、loop fusion、loop unrolling 改善佔了 90% 時間的迴圈，再用 software pipelining 把一圈拆成 L、S、I 區塊跟別圈交錯、每圈用不同的暫存器，讓 load 和用它的指令之間有別的事可做。本機實測：gcc 在 -O1 就做了大部分這些最佳化（但沒有合併迴圈，-O3 在一個例子上反而比 -O2 慢一倍），而在模擬的 in-order pipeline 上，software pipelining 讓 load 從 1 到 4 個 cycle 時每個元素都維持 7 cycle，原本的寫法卻從 9 一路變慢到 11。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 5: Alternative Approaches to Issue and Commit (Part III) -- Compiler Techniques 的 whisper 轉錄稿（本機 `large-v3-turbo`，1:30:46）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch5 §5.3–5.4（p.173–194）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
