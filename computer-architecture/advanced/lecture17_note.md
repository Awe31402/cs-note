# Advanced Computer Architecture 第十七支影片完整筆記：VLIW、EPIC 與 Intel Itanium（Chapter 5, Part IV）

> **課程**：Advanced Computer Architecture — Chapter 5: Alternative Approaches to Issue and Commit (Part IV) -- VLIW and EPIC Processors（播放清單第 17 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕下載不到）（59:26）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch5** §5.5–5.7（p.194–209）
> **章號**：影片 Chapter 5 = 課本 Ch5。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 把「Itanium」有時聽成「Titanium」、「PAp」聽成「BAP」、「BTB」聽成「BTV」、「callee」聽成「poly」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 能不能把 rename 和排程交給 compiler](#1-能不能把-rename-和排程交給-compiler)
- [2. VLIW](#2-vliw)
- [3. EPIC](#3-epic)
- [4. Itanium 的 fetch](#4-itanium-的-fetch)
- [5. 四種 branch predictor](#5-四種-branch-predictor)
- [6. Instruction dispersal：stop bit 與資源標記](#6-instruction-dispersalstop-bit-與資源標記)
- [7. Register remapping：static 與 stacked](#7-register-remappingstatic-與-stacked)
- [8. Register window 與傳參數](#8-register-window-與傳參數)
- [9. 回傳值與 tail recursion](#9-回傳值與-tail-recursion)
- [10. Register stack engine 與 rotating register](#10-register-stack-engine-與-rotating-register)
- [11. Scoreboard](#11-scoreboard)
- [12. Predicated execution](#12-predicated-execution)
- [13. 把 load 往前提：ALAT](#13-把-load-往前提alat)
- [14. Chapter 5 總結與 Itanium 的命運](#14-chapter-5-總結與-itanium-的命運)
- [15. 【實測】](#15-實測)
- [16. 一頁速查表](#16-一頁速查表)
- [17. 與後續講次的連結](#17-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
問題：rename 和排程能不能交給 compiler？→ 省硬體、省電、省複雜度
VLIW（80 年代中）：compiler 把指令包成 bundle，一起執行；正確性是 compiler 的事 → 只用在 DSP
EPIC（VLIW 第二代）：硬體保證正確 → Intel Itanium（Intel ＋ HP，伺服器）
Itanium：
  fetch：32 byte、兩個 bundle、decoupling buffer；四種 branch predictor
  dispersal：9 個 issue port；stop bit；bundle 的資源標記
  remapping：32 static ＋ 96 stacked；register window；RSE；rotating register
  執行：scoreboard（沒有 window/wakeup/select）；predication；ALAT（把 load 往前提）
```

---

## 1. 能不能把 rename 和排程交給 compiler

[00:00] 一個「挑釁」的問題：硬體做的兩件重要的事，**renaming** 和**排程**，能不能外包給 compiler？

[01:11] 好處：

- **硬體變少** → 耗電少、複雜度低。
- 現代軟體又快又聰明，可以在程式跑之前**離線**做很多分析，產生很有效率的程式碼，還能告訴硬體怎麼 rename、怎麼排程。

[01:56] 出發點是 multi-issue in-order pipeline（Ch2）：compiler 把好幾條指令包成一個 **bundle**，bundle 裡**沒有相依**，整個 bundle 一次排程。

---

## 2. VLIW

[02:54] **VLIW**（very long instruction word），80 年代中期就有。概念上是 **microprogramming** 的後繼者。

[03:30] 把連續的幾條指令包成一個長的 word：例如 4 條 4-byte 的指令 → 16-byte 的 word。整個 word 一起排程、一起執行，假設 compiler 已經排好、沒有問題。

[04:14] 但一定會有問題，兩種麻煩的樣式：

| 樣式 | 問題 |
|---|---|
| **if 敘述** | 控制流程在編譯時不知道 |
| **記憶體指令** | 位址執行時才算出來，可能有複雜的相依 |

[05:10] VLIW 的哲學：**正確性是 compiler 的工作**，處理器不負責。

[05:41] if 的處理（像 GPU 的 predicated execution）：bundle 裡有一個 taken 的分支，bundle 剩下的指令就**標記成無效**（像 poison bit），照樣流過 pipeline，但**不改架構狀態**。

[06:41] 記憶體：同一個 bundle 裡好幾個記憶體指令，除了兩個 load（read-read 沒問題），都可能有 hazard，要在軟體或硬體裡避免。

[08:06] VLIW 不保證正確 → 用途有限。今天主要用在 **DSP**（數位訊號處理器）和多媒體處理器，從來沒有通用化。

【課本補充】課本 p.195–196：80 年代中期有兩家這方面的新創 **Cydrome** 和 **Multiflow**。當時軟體已經很成熟（C、Pascal 都有了），硬體卻很慢：Intel 386 是 1 µm 製程、最多大約 30 MHz。VLIW 的 compiler 很保守，會塞很多 nop。DSP 的控制流和資料流都很好預測，所以適合 VLIW，而且省電是重要目標。

---

## 3. EPIC

[08:55] 想把 VLIW 帶進主流：2000–2010 年 Intel 和 HP 的 **Itanium**，叫 **EPIC**（explicitly parallel instruction computing），VLIW 的第二代。

[09:15] 差別：EPIC **不管程式設計師或 compiler 做了什麼，硬體都保證正確**。硬體負擔變大，但接受度和可攜性好：更多程式能跑，compiler 支援也可以馬虎一點。

[09:40] 背景（90 年代末、2000 年代初）：compiler 技術已經很成熟，硬體也很成熟。想法是：硬體資源不要花在 OOO 上，拿來做 EPIC 的伺服器。目標是**伺服器市場**，不是特定應用。

[10:43] 拿掉的：**排程器**：instruction window、wakeup、select、broadcast。

[11:00] 留著的：branch predictor、decode、執行單元、進階的 load/store 處理（位址執行時才知道）。

[11:23] 最好的參考資料：**Sharangpani and Arora**（2000）的論文。

| | VLIW | EPIC |
|---|---|---|
| 年代 | 80 年代中 | 90 年代末、2000 年代 |
| 誰保證正確 | compiler | **硬體** |
| 用途 | DSP、多媒體 | 伺服器（Itanium） |

---

## 4. Itanium 的 fetch

[11:41]

- 從 instruction cache **一次讀 32 byte** = 6 條指令 = **兩個 bundle**，每個 bundle **3 條**。
- [12:21] fetch 很快，後面不一定跟得上 → 中間放一個 queue，叫 **decoupling buffer**，可以放 **8 個 bundle**。進入高 IPC 的階段時，保證有足夠的指令。

【課本補充】課本 p.197：i-cache 是 16 KB、4-way；分支猜錯的代價是 **9 個 cycle**。fetch 的最後一級也處理軟體發的 prefetch 指令。

---

## 5. 四種 branch predictor

[13:16] 就算沒有 broadcast、wakeup、排程，branch predictor「沒辦法許願讓它消失」。Itanium 有四種：

| 種類 | 做什麼 |
|---|---|
| **Compiler 指定** | 4 個 **TAR**（target address register）：compiler 寫進「分支的 PC ＋ 目標」。PC 一對到 → 猜 taken、直接跳到目標，不用 BTB。**第一優先** |
| **傳統的** | 一個很大的 **PAp** predictor（[第 8 支](lecture08_note.md)）：兩層都用 PC 的 bit |
| **Multi-way** | 一個 bundle 裡可能有好幾個分支 → 好幾個可能的目標；猜**第一個 taken** 的是哪個 |
| **Loop exit** | compiler 知道迴圈要跑幾圈 → 一個 **loop count 暫存器**，每次到迴圈分支就 −1，到 0 就猜「離開」 |

[14:00] TAR 適合的情況：例如例外處理的程式碼，大部分時候都會跳出去，compiler 分析一下就可以直接告訴硬體。

[15:00] 為什麼要這麼多？Itanium 是很高 IPC 的處理器，**不想讓 fetch 變成瓶頸**。

[15:42] Multi-way：compiler 通常讓 bundle 的**最後一條**是分支，這樣不管 taken 不 taken，bundle 本身都對。但分支太多時，一個 bundle 可能有兩個分支：目標可能是「都不 taken」、「第一個 taken」、「第二個 taken」。

[18:27] Loop exit：一般的 predictor 在迴圈最後一圈幾乎一定猜錯（[第 8 支](lecture08_note.md)）；這裡 count 到 0 就知道要離開，**猜對**。進入、離開迴圈時那種「差一」的錯也沒了。

【課本補充】課本 p.199：loop exit predictor 對圈數很多的迴圈幫助不大，但對**小迴圈**很有用。

---

## 6. Instruction dispersal：stop bit 與資源標記

[19:33] **Dispersal**：把指令送到不同的功能單元，大約是 dispatch ＋ issue 的組合，EPIC 的文獻叫 dispersal。

[19:55] **9 個 issue port**：

| 種類 | 數量 |
|---|---|
| 記憶體 | 2 |
| 整數 | 2 |
| 浮點 | 2 |
| **分支** | **3** |

[20:20] 分支用了不成比例的資源：設計者非常重視 fetch 瓶頸。

[20:37] Bundle 裡還是可能有 data hazard（程式就是這樣，沒辦法）：

- 傳統的退路：**nop**。
- Itanium 的 pipeline 有大量的 **forwarding**。
- 但像 load-use 這種不能 forward 的 → **stop bit**。

[21:00] **Stop bit**：每條指令一個 bit。

```
stop bit：0 0 0 1 | 0 0 0 0 1 | 1 | …
          └─ 這幾條互相獨立，可以隨便排、平行跑 ─┘
```

兩個 1 之間的指令都**互相獨立**，硬體可以任意排程；碰到 stop 就要等（因為相依而停），讓執行變成循序。compiler 負責插 stop bit，硬體負責解讀。

[22:26] 每個 bundle 還標記了**資源需求**：硬體一看就知道要哪些功能單元，避免 **structural hazard**。

【課本補充】課本 p.200：

- 一個 bundle 裡可以同時有 compare 和依賴它的條件分支，Itanium 內部會把比較結果 forward 給分支。
- 兩個 stop bit 之間的指令叫一個 **instruction group**。
- 每個 bundle 有一個 4-bit 的 **template** 欄位，標示指令類型：M（記憶體）、I（整數）、F（浮點）、B（分支）。一次 disperse 兩個 bundle（6 條），看 8 個 bit 就知道要哪些 port。

---

## 7. Register remapping：static 與 stacked

[23:31] 跟 rename 概念類似，但差很多。

[23:44] 很多暫存器（像 GPU，但不是真的「無限」）：**128 個**，分成兩部分：

| | 數量 | 誰看得到 |
|---|---|---|
| **Static** | 32 | 所有函數 |
| **Stacked** | 96 | 只有一個函數（存那個函數的狀態） |

[24:20] 每個函數在 stacked 區拿到**不同的一組**虛擬暫存器 → 就算好幾個函數同時「活著」，也不用把暫存器 **spill** 到記憶體。這叫 **register window**（每個函數一組私人的暫存器），SPARC 也這麼叫。

【課本補充】課本 p.200：為什麼還要 remapping？要去掉 false dependence。如果讓 compiler 直接指定實體暫存器，可攜性差，而且執行時有很多不確定（分支猜錯、cache miss），compiler 不可能 100% 預測。所以軟體用**虛擬暫存器**，硬體再對應到實體暫存器。課本也說明了 caller-saved 和 callee-saved：兩種都要讀寫記憶體，很貴。

---

## 8. Register window 與傳參數

[25:31] 例子：`foo` 呼叫 `bar`。每個函數的暫存器分成三組：

| 組 | 用途 |
|---|---|
| **in** | 傳進來的參數 |
| **local** | 內部的變數和計算 |
| **out** | 要傳給它呼叫的函數（callee）的參數 |

```
foo：  [ in | local | out ]
                       ↕ 同一組實體暫存器
bar：               [ in | local | out ]
```

[26:42] 像 virtual memory 共用一個 page：**foo 的 out 和 bar 的 in 對到同一組實體暫存器**。foo 寫進 out，bar 自動在 in 裡看到參數，**不用複製**。

[27:30] 故意讓兩個函數的虛擬暫存器重疊來傳參數。有一個硬體和軟體的「管理者」負責建立這些對應。

[28:01] **Register stack frame**：

- in 和 local 在函數呼叫前後**保留**。
- out 用來傳參數給 callee，callee 用完就不管了。
- **`alloc`** 指令幫一個函數建立 register stack frame。

---

## 9. 回傳值與 tail recursion

[28:56] 用遞迴的 binary search 說明：

```c
int bsearch(int arr[], int left, int right, int val) {
    if (left > right) return -1;
    int mid = (left + right) / 2;
    if (val == arr[mid]) return mid;
    if (val < arr[mid]) return bsearch(arr, left, mid - 1, val);
    return bsearch(arr, mid + 1, right, val);
}
```

[29:45] 往下呼叫時做了很多計算；但**往回傳**時，每一層只是把下一層的回傳值**原封不動**傳上去。

[31:25] 遞迴在 `return -1` 或 `return mid` 結束。最好能從那裡**直接跳回** `main`，跳過中間所有層。

[31:50] 所以回傳值放在 **static 暫存器**（前 32 個），不放在 register stack frame 裡：任何一層都能放，然後直接跳到 main 的回傳位址。

[32:59] 這個樣式叫 **tail recursion**；直接跳回去叫 **tail recursion elimination**。看起來偏向 tail recursion，但程式裡有很多類似的樣式，分析後發現回傳值放在 stack 外面是安全的，也不用在暫存器之間複製或從記憶體恢復。

（本機的 gcc 怎麼處理這個 binary search，見 [§15.2](#152-gcc-的-tail-recursion-elimination)。）

---

## 10. Register stack engine 與 rotating register

[33:45] **暫存器不夠怎麼辦**？stacked 區只有 96 個，所有活著的函數都在裡面。

Itanium 有 **register stack engine（RSE）**：自動把暫存器 spill 到記憶體、再恢復。compiler 和使用者都不用管。

[34:38] **Software pipelining**（[第 16 支 §11](lecture16_note.md#11-software-pipelining用不同暫存器)）：同一個變數在不同的圈要用不同的暫存器。Itanium 自動處理：**rotating register set**。

[35:35] 依**迴圈的圈數**分配暫存器：compiler 會標記迴圈（也可以靠往回跳的分支找到），所以圈數一定知道，用圈數選 rotating register set 裡對應的暫存器。

[36:24] 之前 compiler 要多用暫存器、產生額外的程式碼，現在硬體都做了 → 寫 software pipelining 的迴圈容易很多。

---

## 11. Scoreboard

[36:52] 沒有 instruction window、沒有 wakeup／select／broadcast，改用 **scoreboard**。

[37:22] Scoreboard 在 **70 年代末**很流行，之後就沒人用了，但它很快、很省電。比一般的 OOO 簡單很多：

- 一個 queue，指令照順序進來。
- 每格存 **5 個欄位**：rs1、rs2、rd、要用的功能單元、**finished**（0 或 1）。
- 讓一條指令等到**安全**了才執行。

[38:40] 條件：對每一個**更早**的 entry E，下面的式子都要是**假**（I 是現在這條）：

| Hazard | 要是假的式子 |
|---|---|
| **WAW** | E.finished = 0 ∧ E.rd = I.rd |
| **WAR** | E.finished = 0 ∧ (E.rs1 = I.rd ∨ E.rs2 = I.rd) |
| **RAW** | E.finished = 0 ∧ (E.rd = I.rs1 ∨ E.rd = I.rs2) |
| **Structural** | E.finished = 0 ∧ E.fu = I.fu |

[40:04] 做一個電路平行檢查這些 boolean 式子。有一個平行的 scoreboard 單元一直在檢查，一安全就執行。

---

## 12. Predicated execution

[41:24] 例子：

```c
if (rand() % 2 == 0) x = y;
else                 x = z;
```

[41:50] 分支猜錯的機率接近 **50%**（亂數）。每次猜錯都 flush，代價太大（整個 ROB、整個狀態都清掉）。

[42:47] 做法：if 只用來**記下比較的結果**（放在 flags／predicate 暫存器），兩邊的指令**都處理**，只是每條帶著「條件」：

```
moveq x, y     // 上一個比較是相等才搬
movne x, z     // 不相等才搬
```

[44:11] 好處：**沒有分支，就沒有猜錯、不用 flush**。

[44:44] 沒有 predication 的版本：算 `r0 % 2`、比較、條件分支、無條件分支……多了指令和分支，條件分支一猜錯就要 flush。

[46:10] 有 predication 的版本（「不只 Itanium，ARM 現在也這樣做」）：

```
mod r4, r0, 2
cmp r4, 0          → 設 predicate 暫存器 pe（偶數）、po（奇數），只有一個是 1
(po) mov r1, r3    // x = z
(pe) mov r1, r2    // x = y
```

[48:03] predicate 是 1 → 指令執行；是 0 → 照樣流過 pipeline，但**不改架構狀態**。

[48:44] 巢狀的 if：一條指令可以帶好幾個 predicate 暫存器，**全部**都是 1（AND）才執行。

（本機用 x86 的 `cmov` 試了，見 [§15.1](#151-predication-對分支)。）

---

## 13. 把 load 往前提：ALAT

[49:33] 整條 pipeline 的總覽：i-cache 和 fetch、decode、decoupling buffer、四種 branch predictor、dispersal 和 remapping、branch 和 predicate 單元、128 個整數暫存器（32 ＋ 96）、浮點暫存器也類似、SIMD 單元、整數和浮點 ALU、L1 cache、**ALAT**、scoreboard、predicated execution、例外處理（不講）、L2、L3 和 bus controller。

[51:00] 剩下的：load/store。做法：把 load **往前提（boost）**，連帶把一些用它的值的指令也往前提。

- 理由：load 幾乎都在**關鍵路徑**上。早點拿到值，相依的指令早點開始 → ILP 更高。

[51:59] 問題：提到前面之後，中間可能有寫**同一個位址**的 store，結果就錯了。

[52:33] **ALAT**（advanced load address table）：

1. 被提前的 load 放進 ALAT。
2. 之後每個 store 都查 ALAT，位址對到就把那格標成 **dirty**。
3. 在 load **原本的位置**放一條 **load check**（`ld.c`）指令：查 ALAT，看那個 load 還有效嗎。
4. 有效 → 提前成功。無效（中間有同位址的 store）→ 在原本的位置**重做** load 和它的 forward slice。

[53:59] 概念上很像 [第 14 支](lecture14_note.md) 的延遲推測、LSQ 的相依推測，只是那裡有專用的硬體（LSQ），這裡是 compiler 往前提，用 ALAT 檢查。

【課本補充】課本 §5.6.6（p.207）有更多 Itanium 支援 aggressive speculation 的細節。

---

## 14. Chapter 5 總結與 Itanium 的命運

[54:46] Chapter 5 的重點：

| 主題 | 內容 |
|---|---|
| 四種 aggressive speculation | load 位址、LSQ 的相依、load 延遲、值的預測（[第 14 支](lecture14_note.md)） |
| 三種 replay | non-selective（「拿 AK-47 打蚊子」）、delayed selective、token based（[第 15 支](lecture15_note.md)） |
| 降低複雜度 | ARF 設計：用 ROB 存暫存值（[第 16 支](lecture16_note.md)） |
| Compiler | CSE、constant folding、strength reduction、software pipelining（[第 16 支](lecture16_note.md)） |
| EPIC | Itanium：register window、tail recursion elimination、rotating register、load boosting… |

[57:32] Itanium 曾經風光、存活了一段時間，但漸漸失寵。【口頭】老師說「我只能猜，因為我沒有那些資訊」：

- 到 2005 年左右，**硬體技術已經很成熟**，不太需要進階的 compiler 支援。用 GCC 這種開源 compiler 就能有很好的效能、每瓦效能、每塊錢效能。
- 產業不覺得換到一個全新的平台有價值。**產業有很大的慣性**，很多好的、革命性的想法卡在這個慣性裡（雖然 Itanium 也努力相容既有的 ISA）。
- 「這絕對不是這類處理器的終點」，要留意之後的新想法。

[59:03] 處理器設計的討論**全部結束**，下一章是 GPU。

---

## 15. 【實測】

程式在 `experiments/lecture17/`。本機是 x86（OOO），沒有 Itanium，只能測跟本講有關、x86 也有的部分。

### 15.1 Predication 對分支

`pred.c` 把 [§12](#12-predicated-execution) 的例子寫成兩種：

- **branch**：條件分支（`test` ＋ `jnz` ＋ `jmp`）。
- **cmov**：x86 的 `cmov`（條件搬移）：先 `mov` y，條件成立再 `cmovnz` 換成 z，**沒有分支**。這就是 predicated execution。

`experiments/lecture17/pred.c:15-16`、`:25`：

```c
		asm volatile("test $1, %1\n\tjnz 1f\n\tmov %2, %0\n\tjmp 2f\n1:\tmov %3, %0\n2:"
			     : "=&r"(x) : "r"((long)r[i]), "r"(y), "r"(z));
		asm volatile("mov %2, %0\n\ttest $1, %1\n\tcmovnz %3, %0" : "=&r"(x) : "r"((long)r[i]), "r"(y), "r"(z) : "cc");
```

條件來自一個陣列：**random**（0、1 亂數，像 `rand() % 2`）或 **fixed**（全部是 0）。每種跑 1000 萬次，跑兩輪：

| | P-core branch | P-core cmov | E-core branch | E-core cmov |
|---|---|---|---|---|
| random | **3.37** ns/次 | 0.27 | **4.21–4.23** | 0.65–0.66 |
| fixed | 0.26 | 0.27–0.29 | 0.85–0.86 | 0.65–0.66 |

- **亂數**：分支版慢了 **12 倍**（P-core）。每次多 3.1 ns ≈ 14.6 cycle，一半的時候猜錯 → 每次猜錯約 **29 cycle**，跟 [第 8 支](lecture08_note.md) 量到的約 25 cycle 差不多。這就是 [41:50]「猜錯 50%、每次都 flush」的代價。
- **cmov 不管輸入是什麼都一樣快**：沒有分支，就沒有猜錯。
- **輸入很好猜時**（fixed），P-core 上分支版跟 cmov 一樣快，甚至快一點：predication 要兩邊都做，分支猜對時反而不用。E-core 上 cmov 還是比較快。
- 所以 predication 適合**猜不準**的分支；好猜的分支，branch predictor 就夠了。

### 15.2 gcc 的 tail recursion elimination

`bsearch.c` 是 [§9](#9-回傳值與-tail-recursion) 的遞迴 binary search。看 gcc 產生的組合語言裡有幾個 `call`：

```
-O0：call 指令 2 個，ret 1 個
-O1：call 指令 2 個，ret 2 個
-O2：call 指令 0 個，ret 1 個
```

- `-O2` 把兩個遞迴呼叫都變成迴圈裡的跳躍（`jle .L3`、`jg .L9`、`jge .L3`），**完全沒有 `call`**：這就是 tail recursion elimination。
- 一般的處理器沒有 Itanium 的 static 暫存器那種硬體支援，但 compiler 在軟體裡就把「回傳值一路原封不動傳上去」這件事整個消掉了：根本沒有要回傳的那幾層。

---

## 16. 一頁速查表

| 主題 | 重點 |
|---|---|
| 想法 | 把 rename、排程交給 compiler → 少硬體、少功耗 |
| VLIW | 80 年代中；長 word 的 bundle；正確性是 compiler 的事；只用在 DSP、多媒體 |
| EPIC | VLIW 第二代；**硬體保證正確**；Itanium（Intel ＋ HP，伺服器） |
| 拿掉／留下 | 拿掉 window、wakeup、select、broadcast；留下 branch predictor、decode、執行單元、load/store 處理 |
| Fetch | 32 byte = 2 bundle × 3 條；decoupling buffer 8 個 bundle |
| Branch predictor | TAR（compiler 給目標，第一優先）、PAp、multi-way、loop exit |
| Dispersal | 9 個 port（2 記憶體、2 整數、2 浮點、3 分支）；stop bit；bundle 的資源標記（template） |
| 暫存器 | 128 = 32 static ＋ 96 stacked；register window（in/local/out，out 對到 callee 的 in）；alloc |
| 回傳值 | 放 static 暫存器 → tail recursion elimination |
| RSE | 自動 spill/restore stacked 暫存器 |
| Rotating register | 依圈數換暫存器 → software pipelining 不用額外程式碼 |
| Scoreboard | 70 年代末；5 欄位；WAW/WAR/RAW/structural 的 boolean 檢查 |
| Predication | 比較結果放 predicate 暫存器；兩邊都做，只有 predicate = 1 的改狀態；沒有猜錯 |
| ALAT | load 往前提 → 放進 ALAT；store 查 ALAT；原位置的 ld.c 檢查，壞了就重做 |
| 命運 | 2005 年左右硬體夠成熟、產業有慣性 → 失寵（老師的猜測） |
| 實測 | 亂數條件：branch 3.37 ns、cmov 0.27 ns；好猜時一樣快；gcc -O2 把遞迴 bsearch 變迴圈 |

---

## 17. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Multi-issue in-order pipeline | [第 4 支](lecture04_note.md)、[第 5 支](lecture05_note.md)（Ch2） |
| PAp、loop 最後一圈猜錯 | [第 8 支](lecture08_note.md) |
| Software pipelining | [第 16 支](lecture16_note.md) |
| Load 推測、LSQ | [第 12 支](lecture12_note.md)、[第 14 支](lecture14_note.md) |
| GPU 的 predicated execution、很多暫存器 | [第 18 支](lecture18_note.md)–[第 20 支](lecture20_note.md)（Ch6） |

---

## 附：本講一句話總結

> **能不能把 rename 和排程交給 compiler？VLIW 讓 compiler 把互不相依的指令包成一個長 bundle 一起執行，正確性也全歸 compiler，所以只用在 DSP；第二代的 EPIC（Intel 和 HP 的 Itanium）改由硬體保證正確，拿掉 instruction window 和 wakeup/select/broadcast，改用 scoreboard 檢查 WAW、WAR、RAW 和 structural hazard，並加上一大堆 compiler 合作的機制：一次抓兩個 3 條指令的 bundle、四種 branch predictor（compiler 寫的 TAR、PAp、multi-way、loop exit）、9 個 issue port 和 stop bit、32 個 static 加 96 個 stacked 暫存器做 register window（out 對到 callee 的 in）和放回傳值（tail recursion elimination）、自動 spill 的 RSE、給 software pipelining 用的 rotating register、讓猜不準的 if 不用分支的 predication、以及用 ALAT 檢查往前提的 load；最後因為硬體夠成熟、產業有慣性而失寵。本機實測：條件是亂數時，用分支每次 3.37 ns、用 x86 的條件搬移 cmov 只要 0.27 ns，條件好猜時兩者一樣快；gcc -O2 也會把遞迴的 binary search 整個變成迴圈。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 5: Alternative Approaches to Issue and Commit (Part IV) -- VLIW and EPIC Processors 的 whisper 轉錄稿（本機 `large-v3-turbo`，59:26）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch5 §5.5–5.7（p.194–209）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4）結果。*
