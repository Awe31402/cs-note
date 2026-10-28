# Advanced Computer Architecture 第六支影片完整筆記：相依的種類、register renaming 與 precise exception（Chapter 2, Part 3/3）

> **課程**：Advanced Computer Architecture — Chapter 2: Out-of-order Pipelines: Part III/III (Hazards and Exceptions)（播放清單第 6 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（52:01，自動字幕）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch2** §2.3.2–2.4（p.48–59）
> **章號**：影片 Chapter 2 = 課本 Ch2。
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
- [1. Program order：static 與 dynamic 指令](#1-program-orderstatic-與-dynamic-指令)
- [2. RAW：真相依](#2-raw真相依)
- [3. WAW：output dependence](#3-wawoutput-dependence)
- [4. WAR：anti dependence](#4-waranti-dependence)
- [5. Control dependence](#5-control-dependence)
- [6. Register renaming](#6-register-renaming)
- [7. 目前的 OOO pipeline](#7-目前的-ooo-pipeline)
- [8. 從外面看：in-order 還是 out-of-order？](#8-從外面看in-order-還是-out-of-order)
- [9. Precise exception](#9-precise-exception)
- [10. 作業系統為什麼需要它](#10-作業系統為什麼需要它)
- [11. 第 2 章的五個重點](#11-第-2-章的五個重點)
- [12. 【原始碼】Linux 怎麼處理除以 0](#12-原始碼linux-怎麼處理除以-0)
- [13. 【實測】](#13-實測)
- [14. 一頁速查表](#14-一頁速查表)
- [15. 與後續講次的連結](#15-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

第 2 章的最後一講。兩個問題：

1. **相依**：OOO 要遵守哪些順序？哪些順序其實可以拿掉？→ RAW 是真的；WAW、WAR 是假的，用 **renaming** 拿掉。
2. **正確性**：亂序執行時發生 exception 或 interrupt 怎麼辦？→ **precise exception**：裡面亂序、外面看起來照順序。

| 相依 | 別名 | 真的？ | 怎麼處理 |
|---|---|---|---|
| Program order | — | — | 只有 fetch、decode、commit 照它 |
| **RAW** | true／flow dependence | ✅ 真 | 一定要遵守 |
| **WAW** | output dependence | ❌ 假 | renaming |
| **WAR** | anti dependence | ❌ 假 | renaming |
| Control | — | ✅ | branch prediction ＋ 猜錯時丟掉 |

---

## 1. Program order：static 與 dynamic 指令

[00:38] **Program order**：指令在原始程式裡出現的順序。

[00:51] 要分清楚兩種「指令」：

| | 意思 | 例子 |
|---|---|---|
| **Static instruction** | 程式碼裡寫的那一條 | 一個 for 迴圈有 5 條組合語言 |
| **Dynamic instruction** | 實際執行時跑到的每一次 | 迴圈跑 100 次 → 500 條 |

[01:16] 【口頭】硬體怎麼認出迴圈？**往回跳的 jump（offset 是負的）**，大多數時候就是迴圈。

[01:47] 之後講的「順序」都是指 **dynamic** 的順序。

---

## 2. RAW：真相依

[02:06] **RAW（read after write）**：第一條寫、第二條讀。in-order pipeline 裡讓我們頭痛的就是它。

```
add r1, r2, r3     ← producer（寫 r1）
add r4, r1, r5     ← consumer（讀 r1）
```

[02:50] Consumer 一定要在 producer **之後**做，in-order 和 OOO 都一樣，沒得逃。所以叫 **true dependence（真相依）**。

【課本補充】課本 Definition 10（p.49）叫它 **data dependence order**，而且有遞移性：A → B、B → C 就有 A → C。可以畫成 **DAG**（directed acyclic graph，有方向、沒有環的圖，像全是單行道的城市；Definition 11，p.49–50）。DAG 上兩條指令之間**沒有路徑**，就可以同時做。

---

## 3. WAW：output dependence

[03:30] 兩條指令寫**同一個**暫存器：

```
mov r1, 3
mov r1, 5
```

[04:03] 不能對調：先做第二條再做第一條，r1 最後會是 3，但應該是 5。這叫 **WAW（write after write）**，又叫 **output dependence**。

[04:53] 但它是**假相依**（false dependence）：

[05:07] 會有它，只是因為**暫存器數量有限**。如果有無限多個暫存器，第二條就寫 r20，根本不會有 WAW。

---

## 4. WAR：anti dependence

[06:08] 第一條**讀** r2、第二條**寫** r2：

```
add r1, r2, r3     ← 讀 r2
add r2, r5, r6     ← 寫 r2
```

[06:42] 也不能對調：第一條可能讀到第二條寫的新值，錯了。這叫 **WAR（write after read）**，又叫 **anti dependence**。

[06:56] 同樣是**暫存器有限**造成的假相依。

[07:15] 如果每個暫存器都「**寫一次、讀很多次**」（write once, read many），每次寫都是一個新暫存器，WAW 和 WAR 都不會出現。要無限多個暫存器，做不到，但可以想辦法逼近。

【課本補充】課本 Definition 13（p.54）的箭頭記法：RAW 寫成 A → B，WAR 寫成 A →ᵃ B，WAW 寫成 A →ᵒ B（中間沒有其他人寫同一個暫存器）。

---

## 5. Control dependence

[08:34] 跳到一個 label，執行後面的指令。OOO 裡如果 branch **猜錯**，就會從錯的路徑（wrong path）抓指令，污染 instruction window，最後都要丟掉。

[09:32] 所以 OOO 需要非常準的 branch predictor；**control hazard 在 OOO 比在 in-order 重要得多**。

[09:48] 兩種處理器遵守什麼：

| | 遵守 |
|---|---|
| In-order | 所有 **program order**（所以資料和控制相依都自動遵守） |
| OOO | 只遵守**資料相依**和**控制相依**；而且控制相依只是「看起來」有遵守 |

[10:41] 「看起來」的意思：遇到 if，先猜方向，抓後面的指令；如果它們跟 if 在資料上不相干，**可以比 if 本身先做**。之後再做 branch，猜對就什麼都不用做。不管 in-order 還是 OOO，最後都要走在**對的路徑**上。

【課本補充】課本 p.52–53 算了 branch 要多準：假設每 5 條指令有 1 個 branch，每個 branch 猜錯的機率是 p，那 n 條指令裡至少猜錯一次的機率是

```
Pn = 1 − (1 − p)^(n/5)        （Equation 2.3）
```

n = 100 時，課本說 p = 1% 有 **17%**、p = 0.5% 大約 **7%** 機率至少錯一次，所以每個 branch 要 **99% 以上**準。⚠️ 照公式算，p = 1% 是 1 − 0.99²⁰ = **18.2%**、p = 0.5% 是 **9.5%**；課本 Figure 2.22 的曲線（看過 PDF 原圖）也比較接近公式。內文數字略低，結論不變。

---

## 6. Register renaming

### 6.1 Avatar

[11:34] WAW 和 WAR 能拿掉嗎？兩個例子都可以畫在同一條時間線上：

```
寫 r1 ─ 讀 ─ 讀 ─ 讀 │ 寫 r1 ─ 讀 ─ 讀 │ 寫 r1 ─ …
└──── 第一個 avatar ──┘└─ 第二個 avatar ┘
```

[12:51] 每個 **avatar**（化身）從一次「寫」開始，後面跟著零到多次「讀」。

[14:04] 把第一個 avatar 裡的 r1 **全部**換成 r100（寫的那次和後面所有讀的），程式結果完全一樣，程式設計師看不出差別。只要換得一致就好。

[14:39] 這叫 **renaming（改名）**：**每個 avatar 給一個不同的暫存器**。

### 6.2 架構暫存器與實體暫存器

[15:32]

| 名稱 | 是什麼 | 誰看得到 |
|---|---|---|
| **Architectural register（架構暫存器）** | ISA 裡的 r1、r2…，compiler 用的 | 程式設計師、compiler |
| **Physical register（實體暫存器）** | 處理器內部的 p1、p2…，數量比較多 | 只有硬體 |

[16:42] 硬體裡有一個 **renaming engine**，把架構暫存器換成實體暫存器。這件事在內部發生，程式設計師和 compiler 都不知道。怎麼做，第 4 章講。

### 6.3 影片的例子

[16:22] 先假設實體暫存器無限多。這個例子的命名規則：rX 的第 k 個 avatar 叫 pXk；還沒被寫過的 rX 一開始在 pX。

⚠️ 逐字稿聽不清楚每條指令的運算元，這裡只列暫存器的對應：

| # | 寫 | 讀 | 改名後 |
|---|---|---|---|
| 1 | r1 | — | 寫 p11 |
| 2 | r1 | r2、r3（在 p2、p3） | 寫 **p12** |
| 3 | r4 | r1（最新的 avatar） | 讀 p12，寫 p41 |
| 4 | r2 | —（mov 5） | 寫 p21 |
| 5 | r6 | r2、r8 | 讀 p21、p8，寫 p61 |
| 6 | r1 | —（寫 8） | 寫 **p13**（r1 的第三個 avatar） |
| 7 | r9 | r1、r2 | 讀 p13、p21，寫 p91 |

[20:21] 檢查三種相依：

- **RAW**：還在。讀的永遠是最新的 avatar，例如第 3 條讀 p12。
- **WAW**：沒了。實體暫存器無限多，永遠不會寫同一個兩次。
- **WAR**：沒了。第 3 條讀 p12、第 6 條寫 p13，是不同的暫存器，對調也沒關係。

[22:46] 所以 WAW 和 WAR 是 **false dependence**：可以自動拿掉。只有 RAW 是 true dependence。

[23:57] Renaming 把「有 RAW、WAR、WAW 的架構暫存器程式」變成「**只有 RAW** 的實體暫存器程式」。相依越少 → **ILP**（instruction level parallelism，指令層級的平行度）越高 → 每個 cycle 能找到越多可以一起發的指令。

【課本補充】課本 p.55 的小例子（已用 [§13.1](#131-課本的-renaming-例子) 重跑）：

```
架構暫存器              實體暫存器
add r3, r1, r2    →    add p3, p1, p2
add r4, r3, 2     →    add p4, p3, 2
sub r3, r1, 1     →    sub p5, p1, 1
```

改名前：1 → 2（RAW）、1 →ᵒ 3（WAW）、2 →ᵃ 3（WAR）；改名後只剩 1 → 2。

【課本補充】課本 p.55：renaming 也可以讓 compiler 做，但不好。compiler 要知道處理器有幾個實體暫存器，而這個數字每一代處理器都可能變；我們希望 ISA 固定，內部隨便改。所以幾乎所有處理器都在**硬體**裡做 renaming。

---

## 7. 目前的 OOO pipeline

[25:54] 到目前為止的「原型 OOO 處理器」：

```
I-cache → [branch predictor] → Fetch → Decode → Rename → 指令池（instruction window）
        → 每 cycle 挑一組 ready 的指令 → 多個執行單元 → Write back 到 register file
```

- Branch predictor：讓 instruction window 很快填滿。
- Rename：拿掉 WAW、WAR。
- 指令池：每個 cycle 找出運算元都好了（ready）、彼此不相干的指令，好幾條一起送去執行。

⚠️ [26:41] 投影片把「讀暫存器」畫在 rename 後面，老師說那是小錯，讀暫存器其實在更後面，之後再講。

【課本補充】課本 p.52：instruction window 在現代處理器大約 **64–256** 個 entry。太小找不到 ILP，太大會變慢、很耗電（它是 SRAM 做的）。

---

## 8. 從外面看：in-order 還是 out-of-order？

[27:56] 把處理器當成**黑盒子**：指令進去，處理器更新**架構暫存器**和**記憶體**。外面看不到實體暫存器。

[28:48] 對外面的人來說，處理器是 in-order 還是 OOO **應該完全沒差**。

[29:15] 那發生 exception 或 interrupt 時怎麼辦？例子：

```
...
div r7, r5, r6     ← 除以 0！
sub r4, r4, 1
```

[29:56] 除以 0 可以讓程式當掉；但成熟的軟體有 **exception handler**。【口頭】例如 Excel 除以 0 不會當掉，它會跳訊息說「除以 0 不合法」，然後繼續。C、Java 這類語言都能寫自己的 exception handler。

[31:10] 問題：handler 在跑的時候，`sub` 應該做完了嗎？

- **OOO**：有可能 `sub` 先做完、寫了 r4，然後 `div` 才出錯跳進 handler。
- **In-order**（[31:52]）：絕對不會。照順序做，div 一出錯就停，`sub` 根本沒做；跑完 handler 回來，才做 `sub`。外面看到的永遠是程式順序。

[32:54] OOO 直接跳 handler 會很難懂：外面可能看到 `sub` 做完了，甚至前面的指令還沒做完、後面的反而做完了。這樣就**沒辦法在某一點停下來，再從那一點繼續**。

[33:41] 作業系統也需要這件事：**timer chip**（處理器外面的晶片）每隔一段時間送一個 interrupt 說「時間到了」，OS 就換另一個程式來跑。所以要能在任何一點**停下來（stop）**，之後**從同一點繼續（resume）**。

---

## 9. Precise exception

[35:18] 我們要的順序：

| 外面應該看到 | 外面不該看到 |
|---|---|
| 前面的指令 → div（除以 0）→ **exception handler** → sub | 前面的指令 → div → **sub** → exception handler |

[36:53] 原則：**任何外面的人看處理器，都分不出它是 in-order 還是 OOO**。裡面想怎麼亂都行，外面必須看起來像 in-order。

[38:35] 「外面的人」可以是程式設計師、compiler、作業系統，甚至是**接在處理器腳位上的示波器**。

[39:31] 怎麼同時做到？第 4 章。這一章只是**設定目標**。

[40:04] 正式定義。把 dynamic 指令編號 1、2、3 … n，假設處理器在**剛寫完第 k 條的結果**時開始跑 exception／interrupt handler。對外面的人來說，必須看起來：

1. 第 1 到 k 條**全部做完**，結果都寫進記憶體或 register file。
2. 第 k 之後的**一條都還沒開始**。

[41:23] 這樣就可以停：存下架構暫存器，處理 exception，再從 k + 1 繼續。這叫 **precise exception**。

[41:56] 現在幾乎所有 OOO 處理器都支援 precise exception。

[43:00] 這個能力不只為了直覺和正確性：

- 處理 exception 需要它。
- **Branch 猜錯的復原**也需要它，因為猜錯也算一種 exception。
- 作業系統需要它。

【課本補充】課本 p.56 的定義：令 I 是最新（照程式順序）把結果寫回記憶體或 register file 的指令。要正確：(1) I 之前的指令都要在 handler 開始前完成；(2) I 之後的指令都不能完成。課本把 exception 和 interrupt 一視同仁：**interrupt** 是外部事件（例如 I/O 裝置的訊息），**exception** 是內部事件（例如除以 0）。

【課本補充】課本 p.56 的例子（[§13.3](#133-precise-exception除以-0-之後-sub-做了嗎) 在本機跑過）：

```
1 mov r4, 1
2 div r7, r5, r6
3 sub r4, r4, 1
```

在第 2 條後面停下來處理 interrupt，回來時應該 r4 = 1、第 3 條還沒做。如果不 precise、第 3 條已經做了，回來後第 3 條會**做兩次**。

【課本補充】課本 p.57：in-order pipeline 很簡單，讓 MA、RW stage 的指令做完，其他的作廢（nullify）就好。OOO 的解法是：結果可以亂序算出來，但**照程式順序**寫進 register file 或記憶體，這叫 **commit**，要一個專門的 commit 引擎（第 4 章）。

---

## 10. 作業系統為什麼需要它

[43:46] 【口頭】老師打開 Windows 的工作管理員：很多程式同時在跑，包括 PowerPoint。

[44:24] 在單一 core 上：

1. 程式 1 跑一陣子。
2. **Timer chip**（不是 OS 的一部分，是獨立的硬體）送 interrupt，老師說通常**每 1 ms** 一次。
3. 處理器存下程式的 **context**：**program counter ＋ 架構暫存器**（實體暫存器軟體看不到，不用存）。
4. 開始跑程式 2。
5. 又一個 1 ms 後，OS 也許讓程式 1 從**剛剛那一點**繼續。

[45:46] 程式 1 完全不知道自己被停過，「像魔術一樣」。做得到是因為**有 precise exception**。

[46:21] 一個 **time slice** = 1 ms。每秒切換幾百次，所以我們感覺不到。這叫 **multitasking**。

⚠️ 本機 kernel 的 timer interrupt 是 **250 Hz**（`/boot/config-6.5.0-21-generic` 的 `CONFIG_HZ=250`），也就是每 **4 ms** 一次，不是影片說的 1 ms。OS 課對 timer interrupt 的說明見 [Linux OS 第 3 講 §1](../../os/linux-os/lecture03_note.md#1-timer-interrupt保證-os-一定會跑)；context 要存什麼見 [Linux OS 第 2 講 §7](../../os/linux-os/lecture02_note.md#7-context要存哪些東西)。

[48:25] 口號：**out-of-order inside, in-order outside**。

| | 給我們什麼 |
|---|---|
| 裡面 out-of-order | 效能 |
| 外面 in-order | 簡單、直覺、正確（能隨時停下來、再繼續） |

Precise exception 就是「外面 in-order」的自然結果。

---

## 11. 第 2 章的五個重點

[49:20]

1. **In-order pipeline** 因為 branch 和 data hazard，IPC 有限；做成複雜的 multi-issue in-order 也沒用，相依和 interlock 太複雜。要改做 **OOO**。
2. OOO 需要**大的 instruction window**，才能找到夠多不相干的指令。
3. 要填滿大 window，需要**非常準的 branch predictor**：老師說至少 **97–99%**，越高越好。⚠️ 課本 p.53 說要 **99% 以上**。
4. 用 **renaming** 拿掉 WAW、WAR（假相依），每個 avatar 一個實體暫存器，拿到更多 ILP。
5. 為了正確，要有 **precise exception**，才能在任何一點停下程式、之後從同一點繼續。

下一章：fetch 和 branch prediction。

【課本補充】課本 Summary 1（p.57–59）還提到：multi-issue in-order 有 **convoy effect**（一條慢指令拖住後面全部）；一次 fetch、執行很多條叫 **superscalar**；instruction window 裡放 50–100 條改名後的指令。⚠️ 這和同一章 p.52 說的 64–256 不一樣。

---

## 12. 【原始碼】Linux 怎麼處理除以 0

x86 的除以 0 是 trap 0（`#DE`）。`arch/x86/include/asm/trapnr.h:7`：

```c
#define X86_TRAP_DE		 0	/* Divide-by-zero */
```

處理器做到 precise 的那一點後跳進 kernel，kernel 把它轉成給程式的 `SIGFPE`。`arch/x86/kernel/traps.c:205-209`：

```c
DEFINE_IDTENTRY(exc_divide_error)
{
	do_error_trap(regs, 0, "divide error", X86_TRAP_DE, SIGFPE,
		      FPE_INTDIV, error_get_trap_addr(regs));
}
```

- `regs`（`struct pt_regs`）就是影片說的 **context**：出錯那一刻的架構暫存器和 RIP。
- 因為是 precise exception，`regs->ip` 正好指著出錯的那條 `div`，後面的指令都還沒「發生」。
- 錯誤位址由 `error_get_trap_addr(regs)` 給（`traps.c:200-203`，經過 uprobe 修正後就是出錯指令的位址）。

---

## 13. 【實測】

程式在 `experiments/lecture06/`。

### 13.1 課本的 renaming 例子

`rename.py`：照「每次寫入給一個新的實體暫存器、讀的時候用最新對應」的規則改名，並列出改名前後的相依：

```
改名前 ['r3 ← r1,r2', 'r4 ← r3', 'r3 ← r1']
   相依： RAW 1→2, WAW 1→3, WAR 2→3
改名後 ['p3 ← p1,p2', 'p4 ← p3', 'p5 ← p1']
   相依： RAW 1→2
```

跟課本 p.55 一樣：改名後只剩 RAW。

### 13.2 本機有沒有 renaming

如果硬體真的會改名，「一直重用同一個暫存器」應該**不會**比「用不同暫存器」慢。`rename.c`：1 億條 `imul`，三種寫法：

| 寫法 | 相依 |
|---|---|
| chain | `imul %rax,%rax` 一直接下去：RAW |
| reuse | `mov $3,%rax; imul %rax,%rax; mov $5,%rax; imul %rax,%rax; …`：每一對都重用 `%rax`，彼此之間只有 WAW、WAR |
| fresh | 一樣的事，但 4 對分別用 4 個不同暫存器：沒有相依 |

綁在 P-core（cpu2），跑 3 次：

```
chain : 0.641 ns/imul
reuse : 0.214 ns/imul
fresh : 0.214 ns/imul
```

E-core（cpu14）：chain 1.393、reuse 0.209、fresh 0.209 ns/imul。

- **reuse = fresh**：重用 `%rax` 一點都不慢。WAW、WAR 被硬體的 renaming 拿掉了。
- **chain 慢 3 倍**：RAW 拿不掉。P-core 大約 4.7 GHz，0.641 ns ≈ 3 個 cycle，就是一條 `imul` 的延遲；reuse 的 0.214 ns ≈ 1 個 cycle。
- 如果沒有 renaming，reuse 應該跟 chain 一樣慢（每一對都要等前一對寫完 `%rax`）。

### 13.3 Precise exception：除以 0 之後 sub 做了嗎

`precise.c` 把課本 p.56 的例子寫成 x86（課本的 r4 → `%r12`），而且在 `div` 後面再加 100 條跟它無關的 `add`。OOO 處理器很可能早就先算好它們了，問題是外面看不看得到。`experiments/lecture06/precise.c:25-32`：

```c
	asm volatile("mov $1, %%r12\n\t"		/* 1: mov r4, 1 */
		     "xor %%r13, %%r13\n\t"
		     "mov $100, %%rax\n\txor %%edx, %%edx\n\txor %%ecx, %%ecx\n\t"
		     ".globl div_insn\n"
		     "div_insn: div %%rcx\n\t"		/* 2: div（除以 0） */
		     "sub $1, %%r12\n\t"		/* 3: sub r4, r4, 1 */
		     ".rept 100\n\tadd $1, %%r13\n\t.endr"	/* 跟 div 無關的 100 條 add */
		     : "=r"(r12), "=r"(r13) :: "rax", "rcx", "rdx");
```

`SIGFPE` handler 讀出出錯那一刻的 context（`uc_mcontext.gregs`），然後把除數 `%rcx` 改成 1，讓程式回去**重做** `div`（stop → 處理 → resume）。P-core 和 E-core 結果一樣：

```
SIGFPE：RIP 正好指著 div 指令，r12 = 1，r13 = 0
回到程式之後：r12 = 0（sub 只做了一次），r13 = 100，handler 進了 1 次
```

- 出錯那一刻：RIP 指著 `div`，`r12 = 1`（`sub` 沒做）、`r13 = 0`（100 條 `add` 一條都沒做）。完全符合 precise 的定義：前面的都做完、後面的都沒開始。
- 回來之後：`sub` 只做了一次（r12 = 0），100 條 `add` 都做了。這就是課本說的「不 precise 的話第 3 條會做兩次」沒有發生。
- 這段程式在 OOO 處理器上跑，但從軟體（外面）完全看不出來。

⚠️ 第一次寫這個測試時，C 變數沒有綁到 `%r12`、`%r13`，印出來是錯的暫存器（r13 = 8）。用 `register long r12 asm("r12")` 綁定後才對。（另外，handler 裡用 `printf` 嚴格說不是 async-signal-safe，這裡只是示範。）

---

## 14. 一頁速查表

| 主題 | 重點 |
|---|---|
| Static／dynamic | 程式碼裡的一條／實際跑到的每一次；5 條 × 100 圈 = 500 條 dynamic |
| RAW | 寫了才讀；true dependence；一定要遵守 |
| WAW | output dependence；暫存器有限造成；假的 |
| WAR | anti dependence；暫存器有限造成；假的 |
| Control | 猜錯會污染 window；OOO 比 in-order 更需要準的 predictor |
| In-order vs OOO | in-order 遵守全部 program order；OOO 只遵守資料和控制相依 |
| Avatar | 一次寫 ＋ 之後的讀 |
| Renaming | 每個 avatar 一個實體暫存器 → 只剩 RAW → ILP 變多；硬體做，軟體看不到 |
| 架構／實體暫存器 | ISA 看到的／處理器內部的，比較多 |
| 原型 OOO | fetch → decode → rename → instruction window → 挑 ready 的 → 執行 → write back |
| 黑盒子 | 外面只看得到架構暫存器和記憶體 |
| Precise exception | 第 1..k 條全做完、k 之後一條都沒開始；所有 OOO 處理器都支援 |
| 用途 | exception handler、branch 猜錯復原、OS 切換程式 |
| Context | PC ＋ 架構暫存器 |
| Timer interrupt | 影片 1 ms；⚠️ 本機 kernel 250 Hz = 4 ms |
| 口號 | out-of-order inside, in-order outside |
| 實測 | 重用 `%rax` 跟用 4 個暫存器一樣快（renaming）、RAW chain 慢 3 倍；除以 0 時 `sub` 和後面 100 條 `add` 都沒做 |

---

## 15. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| In-order pipeline 的 RAW、interlock、forwarding | [Basic 第 30 支](../basic/lecture30_note.md)、[Basic 第 32 支](../basic/lecture32_note.md) |
| Interrupt、exception handler（基礎） | [Basic 第 44 支 §12](../basic/lecture44_note.md#12-interrupt) |
| 為什麼要 OOO、Figure 1.3 | [第 3 支](lecture03_note.md) |
| Fetch、branch prediction | [第 7 支](lecture07_note.md)、[第 8 支](lecture08_note.md)（Ch3） |
| Renaming 的硬體 | [第 10 支](lecture10_note.md)（Ch4） |
| Commit、怎麼做到 precise exception | [第 13 支](lecture13_note.md)（Ch4） |
| Timer interrupt、context | [Linux OS 第 3 講](../../os/linux-os/lecture03_note.md)、[Linux OS 第 2 講 §7](../../os/linux-os/lecture02_note.md#7-context要存哪些東西) |

---

## 附：本講一句話總結

> **OOO 處理器要遵守的只有資料相依和控制相依：RAW 是真的（consumer 一定在 producer 之後），WAW（output）和 WAR（anti）只是因為暫存器有限才出現的假相依，用 register renaming 給每個 avatar（一次寫加後面的讀）一個新的實體暫存器就能拿掉，程式只剩 RAW、ILP 變多；控制相依則靠非常準的 branch predictor（97–99% 以上）讓 instruction window 填滿正確路徑的指令；另外，為了讓 exception handler、branch 猜錯的復原和作業系統的 multitasking 都能「在任何一點停下、再從同一點繼續」，處理器必須提供 precise exception：外面看起來第 1..k 條全做完、k 之後一條都沒開始，也就是 out-of-order inside, in-order outside。本機實測：一直重用 `%rax` 跟用 4 個暫存器一樣快（硬體 renaming 有效），而除以 0 那一刻，後面的 `sub` 和 100 條 `add` 一條都看不到。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 2: Out-of-order Pipelines: Part III/III (Hazards and Exceptions) 錄影逐字稿（YouTube 自動字幕，52:01）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch2 §2.3.2–2.4（p.48–59）、Linux v6.2.12 `arch/x86/kernel/traps.c`、`arch/x86/include/asm/trapnr.h` 整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
