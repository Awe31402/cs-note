# Basic Computer Architecture 第三十三支影片完整筆記：Interrupt、exception 與 pipeline 裡的 precise exception（Chapter 9, Part 4/4 → 課本 Ch10）

> **課程**：Basic Computer Architecture — Chapt 9: Pipelining, Part 4/4（播放清單第 33 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕一直下載不到）（55:58）＋ 課本 *Basic Computer Architecture* v3.09 **Ch10** §10.8（p.455–463）
> **章號**：影片的 Chapter 9 是課本 v3.09 的 **Chapter 10**。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Interrupt：按一個鍵發生了什麼](#1-interrupt按一個鍵發生了什麼)
- [2. Exception](#2-exception)
- [3. Precise exception：非正式的定義](#3-precise-exception非正式的定義)
- [4. 正式的定義](#4-正式的定義)
- [5. Pipeline 裡為什麼難](#5-pipeline-裡為什麼難)
- [6. 標記一條指令](#6-標記一條指令)
- [7. 讓標記的指令走到最後](#7-讓標記的指令走到最後)
- [8. 程式狀態（context）](#8-程式狀態context)
- [9. PC：NPC 欄位與 oldPC](#9-pcnpc-欄位與-oldpc)
- [10. 暫存器：oldSP 與 spill](#10-暫存器oldsp-與-spill)
- [11. Flags：flags 欄位與 oldFlags](#11-flagsflags-欄位與-oldflags)
- [12. 特權暫存器、movz、retz 與 CPL](#12-特權暫存器movzretz-與-cpl)
- [13. Interrupt handler 的組合語言](#13-interrupt-handler-的組合語言)
- [14. 加上 exception unit 的 pipeline](#14-加上-exception-unit-的-pipeline)
- [15. 【原始碼】x86 Linux 存的 context](#15-原始碼x86-linux-存的-context)
- [16. 【實測】](#16-實測)
- [17. 一頁速查表](#17-一頁速查表)
- [18. 與後續講次的連結](#18-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

Pipelining 這章的最後一講。Interrupt 和 exception 對現代 pipeline 處理器很重要，尤其是跟 **I/O 裝置**（鍵盤、滑鼠）互動時。

```
interrupt（外部，例如按鍵）／exception（程式自己出錯，例如除以 0）
  → 停下程式 → 跑 handler → 回到同一點繼續（precise exception）
  → pipeline 裡同時有 5 條指令：標記 MA 的那條（或出錯的那條），後面的變 bubble
  → 存 context：PC（NPC → oldPC）、暫存器（oldSP ＋ spill）、flags（flags 欄位 → oldFlags）
  → 特權暫存器、movz、retz、CPL
```

---

## 1. Interrupt：按一個鍵發生了什麼

[00:53] 按一個鍵：

1. 鍵盤記錄按了哪個鍵，換成 **ASCII**（7 位元）或 **Unicode**（可能好幾個 byte，看語言）。
2. 把鍵的編碼送給處理器。處理器收到的是一個 **interrupt**。
3. [01:47] Interrupt 是一個隨時可能出現的訊息，本質上是一個 1 bit 的訊號：「有資料來了」。【口頭】像**門鈴**：鍵盤按鈴告訴處理器有訊息在等它。
4. [02:13] 處理器如果正在跑程式，就**暫停**它，跳到作業系統裡一個專門的小程式：**interrupt handler**。
5. [03:03] Handler 處理輸入：例如按了 O，就在螢幕上畫出 O（自己畫或叫別的程式畫）。【口頭】老師的平板也一樣：「筆往右移了 1 公分」，handler 就把螢幕上的點往右移、畫一條線。
6. [04:03] 處理完，回去繼續執行原本的程式。

```
程式 ──▶ interrupt ──▶ handler ──▶ 程式 ──▶ interrupt ──▶ handler ──▶ …
```

[02:37] **作業系統**：一組管理一般程式和裝置（滑鼠、鍵盤）的特殊程式。

【課本補充】課本 Definition 77（p.456）：interrupt 是 I/O 裝置送給處理器的訊號，用來讓處理器注意新的輸入或裝置狀態的改變。

---

## 2. Exception

[04:30] **Exception**：程式做了不正常的事。例如：

- 存取**不合法的位址**（不在可以用的位址空間裡）。
- [04:55] **整數除以 0**：結果是無限大，但整數沒辦法表示無限大。
- [05:27] 執行**不合法的指令**（不存在的 opcode）。

[05:55] 處理方式跟 interrupt 一樣，只是叫 **exception handler**。例如存取不合法的位址，handler 可能直接結束程式、告訴使用者。

[06:19] Handler 結束後，回去跑原來的程式，或跑**另一個**程式。

【課本補充】課本 Definition 78（p.457）。課本 p.456：記憶體系統偵測到不合法的位址時，其實也是送一個 interrupt 給處理器；Java 裡存取 −1 這種陣列索引也會產生 exception。課本裡 interrupt handler 和 exception handler 兩個詞混用。

---

## 3. Precise exception：非正式的定義

[06:46] 這一講最重要的性質：**precise exception**。

非正式定義：**回到原本程式時，必須回到剛好離開的那一點**。

- [07:09] 不能跳過任何一條指令，也不能有任何一條執行兩次。
- [07:34] Handler 的執行**完全不能干擾**原本的程式：程式的結果應該跟有沒有 interrupt 無關（除非那個 interrupt 跟程式有關，例如除以 0 把程式結束掉）。

[09:20] 【口頭】例子：下西洋棋下到一半，按了幾個按鈕、開了別的程式，棋局的走法不會因此改變。原因就是有 precise exception。

[09:42] Precise exception 和 precise interrupt 是同一件事，兩個詞混用。

---

## 4. 正式的定義

[10:08] 程式的動態指令是 I₁ … Iₙ。

**「完成（complete）」**的意思（[11:02]）：一條指令**更新了記憶體、寫了暫存器，或到達 MA stage**，看哪個先發生。

- cmp：更新了 flags。
- b、beq、bgt、ret：把新的 PC 寫進 PC 暫存器。
- [11:53] 總之就是對**永久狀態**做了改變，它的工作就算做完了。

[12:16] 設 **I_k** 是 handler 的第一條指令完成之前，程式**最後一條**完成的指令。所有在那之前完成的程式指令構成集合 **C**。

正式條件（[14:35]，課本 Definition 79，p.457）：

1. 只要 I_j ∈ C，就有 j ≤ k：**I_k 之前的都做完了**，I_k 之後的都沒完成。
2. [15:15] I_k 之後的任何指令（I_k′，k′ > k），都不能在 handler **全部做完之前**完成。
3. [16:08] Handler 結束後，可以無縫地從 **I_k₊₁**（或必要時**重新執行 I_k**）開始。

[16:42] 單週期處理器很容易：interrupt 來了，做完目前這條（就是 I_k），前面的都已經做完了，跳去 handler，再回來。

【課本補充】課本 p.458：重新執行 I_k 的例子是 **page fault**：第一次讀寫某個 4 KB 的記憶體區塊時，作業系統要從硬碟把它讀進記憶體，然後**重新執行**那條出錯的指令，第二次就成功了（[第 37 支](lecture37_note.md)）。

---

## 5. Pipeline 裡為什麼難

[12:36] 5 級 pipeline 同時有 5 條指令。interrupt 來時，如果**全部殺掉**、之後從 IF 那條重新開始：

- RW stage 的那條可能是 store，它在 MA 就已經寫了記憶體、**已經完成了**。
- 重新開始會把它**再做一次**，錯了。

[17:04] 有些指令已經改了永久狀態，要讓它們做完；有些（例如 IF、OF 裡的）還沒改任何狀態，可以丟掉。

[18:59] 例子：一個 branch 已經把 PC 從 10 更新成 20（offset 10），如果又被執行一次（offset 12），PC 會變成 32：**offset 被加了兩次**。所以除非我們刻意要，否則同一條指令不能執行兩次。

---

## 6. 標記一條指令

[19:18] 做法：**標記（mark）**一條指令，它就是 I_k。

| 事件 | 標記哪一條 |
|---|---|
| **Interrupt** | 那個 cycle 裡在 **MA stage** 的指令（所有永久的改變都在 MA 或之前發生，連 branch 也是在 MA 更新 PC） |
| **Exception** | **出錯的那條**，在哪個 stage 出錯就標記哪裡：IF 抓不合法的位址就標 IF、EX 除以 0 就標 EX |

【課本補充】課本 p.458 解釋為什麼不能標 RW：MA 裡的可能是 store，這個 cycle 就會完成，違反條件 2。其他四個 stage 都可以選，課本選 MA。

---

## 7. 讓標記的指令走到最後

[21:40] 標記之後，pipeline 裡的指令分成兩群：

| | 要怎樣 |
|---|---|
| 比標記的指令**早**進 pipeline（在它右邊、比較接近 RW） | **全部要完成** |
| 比標記的指令**晚**進 pipeline（在它左邊） | **一條都不能完成** |

⚠️ [21:40] 老師說「標記指令**之後**的全部完成、**之前**的都不完成」，跟上表的方向相反：他說的「之後」是 pipeline 圖上的位置（越後面的 stage），不是程式順序。照課本和他接下來的說明，是**程式順序較早**的完成、**較晚**的不完成。

[22:35] 步驟：

1. 等標記的指令走到 pipeline 的**最後**。
2. 標記指令後面（程式順序較晚）的全部**變成 bubble**，不能做任何永久的改變。
3. [23:33] 標記的指令走到最後時，**exception unit**（新加的特別單元）把 handler 的 PC 載入。

---

## 8. 程式狀態（context）

[23:33] 程式的狀態：**PC、16 個 SimpleRisc 暫存器、flags 暫存器、記憶體**。

[23:56] 假設程式和 handler 的記憶體**不重疊**（除非刻意），所以記憶體不用特別存。

[27:07] 為什麼記憶體沒問題：標記之前的 store 都做完了；之後的指令都變 bubble，不會寫記憶體；handler 也不碰程式的記憶體。

[28:23] 要處理的只有三樣：**PC、暫存器、flags**，要恢復成標記的那條指令「看到或更新後」的樣子。

【課本補充】課本 Definition 80、81（p.459）：**程式狀態** = register file、PC、flags、記憶體；**context** = PC、flags 和所有暫存器的值。課本也提到 exception 的情況（例如 page fault），handler 會刻意存取程式的記憶體，把它需要的資料放進去。

---

## 9. PC：NPC 欄位與 oldPC

[28:51] 在 **instruction packet**（跟著指令在 pipeline 裡走的那包 bit，[第 30 支](lecture30_note.md)）裡加一個 **NPC**（next PC）欄位：

| 指令 | NPC |
|---|---|
| taken 的 branch | branch target |
| 其他 | PC + 4 |

- [29:14] EX stage 算 branch target 時，順便把 NPC 填好，它就跟著指令往下走。
- [29:32] 標記的指令走到最後時，exception unit 依 interrupt／exception 的種類，把 **PC 或 NPC** 存進一個新的 **oldPC** 暫存器：
  - 回來時從下一條開始 → 存 NPC。
  - 要重新執行標記的那條（例如 page fault）→ 存 PC。
- [30:09] oldPC 是**特權暫存器**：一般程式看不到，handler 才能存取。

---

## 10. 暫存器：oldSP 與 spill

[32:12] 函式呼叫時，暫存器是存在 stack 上（**spilling**，[第 8 支](lecture08_note.md)）。Handler 也要這樣做，但：

- [32:24] Handler 要用**自己的 stack**。
- [32:39] 可是要換到 handler 的 stack 就得改 stack pointer，程式原本的 sp 就沒了。

[33:20] 解法：再加一個特權暫存器 **oldSP**。標記的指令走到最後時：

1. 把目前的 sp（SimpleRisc 的 r14）存進 oldSP。
2. 載入 handler 的 stack pointer。
3. 把所有暫存器 spill 到那裡，連同 oldPC、oldSP。

---

## 11. Flags：flags 欄位與 oldFlags

[34:14] Flags 比較麻煩。

**天真的方法**：標記之後不准任何指令更新 flags。問題：interrupt 或 exception 通常在 cycle 中間或後段才偵測到，那時某條指令可能已經改了 flags 暫存器（至少改了 master-slave flip-flop 的 master，[第 19 支](lecture19_note.md)），很難知道什麼時候改、什麼時候要還原。

[34:59] **比較好的方法**：在 instruction packet 裡加一個 **flags 欄位**。

- [35:54] 每條指令經過 EX stage 時，把**它看到的** flags（cmp 的話就是**它更新後的** flags）放進自己的 flags 欄位。【口頭】「像行李箱一樣帶著走。」
- [36:22] 標記的指令走到最後時，exception unit 把它的 flags 欄位存進 **oldFlags** 暫存器。
- 真正的 flags 暫存器愛怎麼改都沒關係。

[37:12] 新加的三個特權暫存器：

| 暫存器 | 存什麼 |
|---|---|
| **oldPC** | 要回去的 PC（標記指令的 PC 或 NPC） |
| **oldSP** | 程式的 stack pointer |
| **oldFlags** | 標記指令看到或更新的 flags（實際只有 flags.E、flags.GT 兩個 bit，為了簡單當成 32 位元） |

---

## 12. 特權暫存器、movz、retz 與 CPL

[38:33] 把 flags 暫存器也露給 ISA（還原時要寫它），加上三個 old 暫存器，共四個新暫存器。

[39:52] 處理器設計者通常有一組一般暫存器、一組給 handler 這類特殊程式用的特殊暫存器，編碼方式也不同，所以 decode unit 也要改。

[40:16] 兩條新的 **Z 系列特權指令**：

| 指令 | 做什麼 |
|---|---|
| **movz** | 在一般暫存器和特殊暫存器之間搬資料 |
| **retz** | oldPC → PC：從 handler 回到程式 |

[40:48] 怎麼確保一般程式不能用？**CPL**（current privilege level）bit：

| | CPL |
|---|---|
| 使用者程式 | **1** |
| Interrupt handler、kernel | **0** |

- [41:18] 使用者程式看不到特殊暫存器，連 flags 暫存器都只能「隱含地」用、不能明確讀寫。
- Interrupt／exception 來時，CPL 自動從 1 變 0；`retz` 回去時從 0 變 1。
- [41:58] **Kernel**：作業系統的程式，管理其他程式的特殊程式。
- [42:28] movz 和 retz 的 opcode 只有 CPL = 0 時能用。OF 和 RW stage 要擴充，必要時用這些特殊暫存器。

[42:28] 特權模式下看到的暫存器編號不同（課本 Table 10.2，p.461）：

| 編號 | 暫存器 |
|---|---|
| 0000 | r0 |
| 0001 | oldPC |
| 0010 | oldSP |
| 0011 | flags |
| 0100 | oldFlags |
| 1110 | sp（和 14 一樣，維持原本的編號） |

【課本補充】課本 p.462：特權指令很強大，讓一般程式用可能破壞系統狀態、帶來病毒；執行特權指令時處理器會檢查 CPL，不允許就產生 exception，OS 通常會把那個程式結束掉，「因為它可能是病毒」（Definition 83）。課本 Definition 84 把這種「不同指令看到不同的暫存器」叫 **register window**，最早由 Sun 的 UltraSPARC 處理器使用（它是給不同函式用不同的暫存器組，避免 spill）。

（本機 x86 上的 CPL 和特權指令實測見 [§16.1](#161-cpl-與特權指令)。）

---

## 13. Interrupt handler 的組合語言

[43:39] 目標：把 PC、暫存器、flags 全部存到記憶體（handler 的 stack），之後再讀回來。課本 Figure 10.32、10.33（p.463），handler 的 stack 從 **0xFFFFFFFC** 開始。

**存 context**（[44:31]）：

```
movz oldSP, sp                 # 程式的 sp 先存起來（也可以由硬體自動做）
mov  sp, 0xFFFFFFFC            # 換成 handler 的 stack
st   r0, -4[sp]                # 存 r0 … r13、r15
st   r1, -8[sp]
...                            # （不存 r14：它就是 sp）
movz r0, oldSP                 # oldSP → r0 → 記憶體
st   r0, -64[sp]
movz r0, oldFlags              # oldFlags
st   r0, -68[sp]
movz r0, oldPC                 # oldPC
st   r0, -72[sp]
sub  sp, sp, 72                # stack 長了 72 byte
```

- [45:55] **不存 r14**，因為 r14 就是 sp。
- 72 byte = 15 個暫存器（r0–r13、r15）× 4 ＋ oldSP ＋ oldFlags ＋ oldPC。
- [47:22] 為了簡單，除了 movz、retz 沒有加其他指令，store 就用一般的 `st`。

[47:53] 存完了，就可以跑 handler 的本體。

**還原 context**（[48:24]）：反過來做：

1. sp 加回 72。
2. 讀回 oldPC（經 r0，用 movz 寫回 oldPC）。
3. 讀回 oldFlags，用 movz 寫進 **flags** 暫存器（所以 flags 要露給 ISA）。
4. 讀回 r0 … r15（**跳過 sp**）。
5. [50:04] 最後才還原 sp（從 −64 的位置）。
6. **`retz`**：oldPC → PC，CPL 變回 1，回到使用者程式。

⚠️ [48:24]、[49:07] 老師兩次說「投影片這裡有錯」：還原的順序和寄存器名稱（先 oldPC、再 flags），照他口頭更正的寫。

[51:24] 程式完全不知道中間有 interrupt。

[52:38] 為什麼要特殊暫存器？要把一般暫存器從 A 搬到 B，需要寫組合語言，組合語言本身又要用暫存器，就會蓋掉一些一般暫存器。所以要額外的暫存器，避免一般暫存器被蓋掉。

---

## 14. 加上 exception unit 的 pipeline

[53:08] 最後的 pipeline（課本 Figure 10.34，p.463）：

- 新的**標記電路**：可以標記任何 stage 的指令。
- PC／NPC、instruction packet 的 flags 欄位送到 **exception unit**，它還管 **CPL** 暫存器。
- Exception unit 控制 handler 的執行：標記的指令走到最後時，更新 CPL、依種類通知 fetch unit 開始抓 handler。
- [54:00] Register file 擴充成 **register unit**：多了 oldFlags、oldPC、oldSP、flags 四個暫存器（這個 flags 和 ALU 旁邊的 flags 概念上是同一個）。

[54:43] 整個 pipeline 處理器現在有：stall 和 bubble 邏輯、data hazard 和 control hazard 處理、forwarding unit、exception unit。「不然要怎麼跟 I/O 裝置互動？怎麼處理出錯的程式？」

[55:16] Pipelining 這章結束，下一部分是**記憶體系統**。

---

## 15. 【原始碼】x86 Linux 存的 context

x86 的硬體在 interrupt／exception 時，自己就做了課本「exception unit」的事：把 **RIP（oldPC）、CS（含 CPL）、RFLAGS（oldFlags）、RSP（oldSP）、SS** 推到 kernel 的 stack 上；Linux 再把一般暫存器推上去，湊成一個 `struct pt_regs`。回去時用 **`iretq`**（就是課本的 retz）。`arch/x86/include/asm/ptrace.h:80-92`：

```c
/*
 * On syscall entry, this is syscall#. On CPU exception, this is error code.
 * On hw interrupt, it's IRQ number:
 */
	unsigned long orig_ax;
/* Return frame for iretq */
	unsigned long ip;
	unsigned long cs;
	unsigned long flags;
	unsigned long sp;
	unsigned long ss;
/* top of stack page */
};
```

| 課本 | x86 Linux |
|---|---|
| oldPC | `ip` |
| oldFlags | `flags` |
| oldSP | `sp` |
| CPL | `cs` 的最低 2 bit |
| retz | `iretq` |
| spill r0 … r15 | 同一個結構前面的 r15 … di（`ptrace.h:59` 起） |

第 6 支（Advanced）的實測裡，SIGFPE handler 讀到的 `uc_mcontext.gregs` 就是從這裡來的。

---

## 16. 【實測】

程式在 `experiments/lecture33/`。

### 16.1 CPL 與特權指令

`cpl.c`：

```
cs = 0x33 → CPL = 3（cs 的最低 2 bit）
rflags = 0x246 → IOPL = 0、IF（允許 interrupt）= 1
cli（關掉 interrupt）：被擋下來，收到 SIGSEGV（#GP）
hlt（停住 CPU）：被擋下來，收到 SIGSEGV（#GP）
```

- 使用者程式的 **CPL = 3**。⚠️ 課本說使用者是 1、kernel 是 0；x86 有 0–3 四個等級（ring），Linux 的使用者程式用 **3**，kernel 用 0。概念一樣，數字不同。
- **IF = 1**：使用者程式跑的時候 interrupt 是開著的。
- 在使用者模式執行 `cli`（關 interrupt）和 `hlt`（停住 CPU）這兩條特權指令，CPU 拒絕執行、產生 #GP，kernel 送 `SIGSEGV`。就是課本 p.462 說的「不允許就產生 exception」。如果使用者程式能 `cli`，它就能讓鍵盤、timer 的 interrupt 全部進不來，整台電腦被它霸佔。

### 16.2 真的 interrupt 有多少

`irqrate.py` 讀 `/proc/interrupts` 兩次（隔 2 秒），同時讓 cpu2 一直忙、cpu4 閒著：

```
名稱           全部 CPU 每秒     cpu2（忙）     cpu4（閒）  說明
143                  2           0           0  … xhci_hcd（USB）
186                 10           0           0  … iwlwifi（Wi-Fi）
203                  5           0           0  … i915（顯示）
LOC               1528         252         183  Local timer interrupts
CAL                 95           0           0  Function call interrupts
TRM               1810          90          90  Thermal event interrupts
```

- **Local timer**：忙著的 cpu2 每秒 **252** 次，正好是 kernel 的 `CONFIG_HZ=250`。這就是 Advanced 第 6 支說的「timer chip 每隔一段時間打斷程式，OS 換別的程式」。閒著的 cpu4 只有 183 次：這個 kernel 開了 `CONFIG_NO_HZ_IDLE`，閒置時可以少收 timer interrupt 省電。
- USB、Wi-Fi、顯示卡都會送 interrupt，每秒幾次到十幾次。
- `TRM`（溫度事件）每個 core 每秒 90 次：這台筆電一直在 100°C 附近（[第 47 支 §17.1](lecture47_note.md#171-power-wall本機一秒就到-100c)）。
- 鍵盤是 IRQ 1（`i8042`），開機到現在累計 10411 次（按下和放開各算一次）。

---

## 17. 一頁速查表

| 主題 | 重點 |
|---|---|
| Interrupt | I/O 裝置送來的訊號，像門鈴；停程式 → handler → 回來 |
| Exception | 程式自己出錯：不合法位址、除以 0、不合法 opcode |
| Precise（非正式） | 回到剛好離開的那一點；不跳過、不重複 |
| 完成 | 寫了記憶體、寫了暫存器，或到達 MA |
| Precise（正式） | I_k 以前全完成；I_k 之後在 handler 結束前都不完成；從 I_k₊₁（或 I_k）繼續 |
| Pipeline 的難處 | RW 裡的 store 已經完成，不能全部殺掉重來 |
| 標記 | interrupt → MA 的那條；exception → 出錯的那條 |
| 之後 | 等標記的走到最後；後面的變 bubble；exception unit 載入 handler |
| Context | PC、暫存器、flags（記憶體不重疊，不用存） |
| PC | packet 加 NPC 欄位 → oldPC（存 PC 或 NPC） |
| 暫存器 | oldSP；換 handler 的 stack；spill |
| Flags | packet 加 flags 欄位 → oldFlags |
| 特權 | movz、retz；CPL：使用者 1、kernel 0 |
| Handler | 存 15 個暫存器 ＋ oldSP ＋ oldFlags ＋ oldPC = 72 byte；不存 sp |
| x86 | 硬體推 ip、cs、flags、sp、ss；iretq；使用者 CPL = 3 |
| 實測 | cs = 0x33（CPL 3）；cli、hlt 被擋（#GP）；忙的 core 每秒 252 次 timer interrupt（HZ = 250） |

---

## 18. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Instruction packet、bubble、interlock | [第 30 支](lecture30_note.md)、[第 31 支](lecture31_note.md) |
| Forwarding、完整的 pipeline | [第 32 支](lecture32_note.md) |
| Spill、stack、函式呼叫 | [第 8 支](lecture08_note.md) |
| Page fault（要重新執行 I_k） | [第 37 支](lecture37_note.md) |
| I/O 裝置怎麼送 interrupt、polling vs interrupt | [第 44 支 §12](lecture44_note.md#12-interrupt) |
| OOO 處理器的 precise exception、除以 0 實測 | [Advanced 第 6 支](../advanced/lecture06_note.md) |
| Linux 的 timer interrupt、context | [Linux OS 第 3 講](../../os/linux-os/lecture03_note.md)、[Linux OS 第 2 講 §7](../../os/linux-os/lecture02_note.md#7-context要存哪些東西) |

---

## 附：本講一句話總結

> **Interrupt 是 I/O 裝置按的門鈴、exception 是程式自己出的錯（不合法位址、除以 0、不合法 opcode），兩者都要停下程式、跑 handler、再回到剛好離開的那一點，也就是 precise exception：I_k 以前的全部完成、之後的在 handler 結束前一條都不完成，回來從 I_k₊₁（page fault 時重做 I_k）繼續；在 5 級 pipeline 裡做法是標記一條指令（interrupt 標 MA 的、exception 標出錯的）、讓它走到最後、把後面的都變成 bubble，再由 exception unit 存好 context：instruction packet 多帶 NPC 和 flags 兩個欄位，分別存進 oldPC、oldFlags，sp 存進 oldSP 後換到 handler 自己的 stack 把暫存器 spill 下去（共 72 byte），這些特權暫存器只能在 CPL = 0 時用 movz 存取、用 retz 回到 CPL = 1 的使用者程式。本機實測：x86 Linux 的使用者程式 CPL 是 3，執行 cli、hlt 會被 #GP 擋下；忙碌的 core 每秒收到 252 次 timer interrupt（CONFIG_HZ = 250），而 x86 硬體推到 stack 上的 ip、flags、sp 正好就是課本的 oldPC、oldFlags、oldSP。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 9: Pipelining, Part 4/4 的 whisper 轉錄稿（本機 `large-v3-turbo`，55:58）、Sarangi *Basic Computer Architecture* v3.09 Ch10 §10.8（p.455–463）、Linux v6.2.12 `arch/x86/include/asm/ptrace.h` 整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
