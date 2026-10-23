# Operating Systems: A Linux Kernel-Oriented Approach 第十二講完整筆記：Context Switch（Chapter 3, Part VI）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 12（Chapter 3, Part VI）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（37:51，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§3.3.1–3.3.2（p.99–104）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
> **標記說明**：
> - `【口頭】`：教授在課堂上口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與教授回答
> - `【課本補充】`：課本有、但這堂課沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（`檔名:行號`），用來對照觀念
> - `⚠️ 逐字稿缺字`：自動字幕在這裡掉字或聽不清，內容依上下文或課本補上
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 這堂課要做什麼](#0-這堂課要做什麼)
- [1. 硬體 context 與軟體 context](#1-硬體-context-與軟體-context)
  - [1.1 硬體 context：要存要還原](#11-硬體-context要存要還原)
  - [1.2 CR3 與 TLB：能不換就不換](#12-cr3-與-tlb能不換就不換)
  - [1.3 軟體 context：不用動](#13-軟體-context不用動)
- [2. 四種切換](#2-四種切換)
- [3. 兩個關鍵事實](#3-兩個關鍵事實)
  - [3.1 所有 kernel thread 共用虛擬位址空間](#31-所有-kernel-thread-共用虛擬位址空間)
  - [3.2 kernel 那一半的映射在每個 process 都一樣](#32-kernel-那一半的映射在每個-process-都一樣)
  - [3.3 【原始碼】每個新 page table 都抄一份 kernel 的映射](#33-原始碼每個新-page-table-都抄一份-kernel-的映射)
- [4. 什麼時候要換 page table、清 TLB](#4-什麼時候要換-page-table清-tlb)
  - [4.1 兩個選項：TLB 裡加 PID，或清掉](#41-兩個選項tlb-裡加-pid或清掉)
  - [4.2 逐一檢查四種切換](#42-逐一檢查四種切換)
  - [4.3 為什麼 kernel 可以讀使用者的 buffer](#43-為什麼-kernel-可以讀使用者的-buffer)
  - [4.4 【原始碼】`context_switch()`：kernel 自己的四格表](#44-原始碼context_switchkernel-自己的四格表)
  - [4.5 【原始碼】PCID：TLB 裡的「PID」](#45-原始碼pcidtlb-裡的pid)
- [5. 三種 context switch](#5-三種-context-switch)
  - [5.1 Process context switch](#51-process-context-switch)
  - [5.2 Thread context switch](#52-thread-context-switch)
  - [5.3 【原始碼】同一個 process 的 thread 切換：不用清 TLB](#53-原始碼同一個-process-的-thread-切換不用清-tlb)
  - [5.4 Interrupt context switch：top half 與 bottom half](#54-interrupt-context-switchtop-half-與-bottom-half)
- [6. 兩個大想法](#6-兩個大想法)
- [7. 【實測】一秒有多少次 context switch](#7-實測一秒有多少次-context-switch)
- [8. 一頁速查表](#8-一頁速查表)
- [9. 與後續講次的連結](#9-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

`[00:32]` context switch 到底怎麼運作——「比我們想像的**複雜得多**」。

第 7 講教授說 context switch 像**肝臟移植**。這一講先看**大方向**：

| # | 主題 | 對應課本 |
|---|---|---|
| 1 | 硬體 context vs 軟體 context | §3.3.1 |
| 2 | **什麼時候要換 page table、清 TLB**——本講主軸 | §3.3.1 |
| 3 | 三種 context switch：process、thread、interrupt | §3.3.2 |

細節（低階步驟）在下一講。

---

## 1. 硬體 context 與軟體 context

### 1.1 硬體 context：要存要還原

`[00:32]` 每個 process 都有**硬體 context** 和**軟體 context**。

`[01:00]` **硬體 context** 是要**存起來、再還原**的：

| 項目 | 說明 |
|---|---|
| **一般暫存器** | 包括 stack pointer |
| **PC** | x86 叫 **instruction pointer** |
| **segment 暫存器** | x86-64 主要用 `fs`、`gs`；舊版 x86 用得比較多（第 6 講） |
| **特權暫存器** | 使用者程式用得不多，但有一個很有趣：**CR3**，存 page table 的起點（第 5 講） |
| **ALU 和浮點的 flags** | `[02:30]` 存「過去執行的指令的狀態，但對現在和未來有影響」：例如以前做了一次比較、設了 flags，之後的條件分支要靠它決定方向 |

【課本補充】課本強調（p.99）：要**非常精確**地存和還原，「**不容許任何錯誤**」。這樣同一個 process 之後才能重新啟動，**而且它根本不知道自己被換出去過**。x86-64 這種大而複雜的處理器還有很多次要的硬體 context 元件，這裡只列主要的。

### 1.2 CR3 與 TLB：能不換就不換

`[01:30]` CR3 是 context 的一部分，**但**：

> 「我們會盡量確保這個基底位址**不要太常更新**。一改 CR3，page table 就換了，**TLB 也會自動被清掉（flush）**。這些操作**非常非常昂貴**。所以**盡可能少更新 CR3**——這是我們**明確的目標**。」

`[03:01]` page table 和 TLB 會是**這一講的主軸**：

> 「換 page table、換 TLB **強烈不建議**，是壞主意。換到新的 page table，也就是 TLB 換成一組新的內容，會讓系統**非常非常慢**——TLB miss 會**成倍增加**。」

教授在投影片上的 page table / TLB 旁邊打了一個**問號**：「我們這堂課會**反覆回來**看這個問題。」

### 1.3 軟體 context：不用動

`[03:30]` **軟體 context** 好管得多：

- 開啟的檔案、網路連線
- process 跑了多久等所有其他後設資料

`[04:01]` 原因：它們**不是短暫的（ephemeral）**，不會消失。它們是 **`task_struct` 的一部分**，會安全地留在原地，「**什麼都不用做**」。

【課本補充】課本把 **page table** 也列在軟體 context 裡（p.100）：page table、開啟的檔案、網路連線等資源的細節都在 `task_struct` 裡，**context switch 時不用存和還原**，隨時可以從 `task_struct` 拿回來。（硬體 context 裡的是 **CR3**——指向 page table 的那個暫存器。）

---

## 2. 四種切換

`[04:31]` 可能的切換：

| 編號 | 切換 | 什麼時候 |
|---|---|---|
| **1** | **user → kernel** | 使用者 process 進 kernel（system call、中斷） |
| **2** | **kernel → kernel** | kernel 裡從一個 thread 換到另一個 thread |
| **3a** | **kernel → user（同一個）** | 回到**剛剛被打斷的同一個**使用者 process |
| **3b** | **kernel → user（不同的）** | 載入一個**不同的**使用者 process |
| ~~user → user~~ | **不可能** | 一定要**經過 kernel**——已經包含在上面幾種裡了 |

`[05:01]` 「kernel 回到 user 時其實有**兩個子情況**，而且很重要：**same** 和 **diff**。」

`[05:31]` 目標：針對這四種情況**提出最佳化**。

---

## 3. 兩個關鍵事實

`[06:00]` 這些事實以前講過，但「在這次討論裡變成**非常重要的輸入**」。

### 3.1 所有 kernel thread 共用虛擬位址空間

`[06:00]` **事實一**：所有 kernel thread **共用虛擬位址空間**——不管是哪個 kernel thread，同一個 kernel 虛擬位址都對應到**同一個實體位址**。

（第 11 講：「所以叫 kernel **thread**，不叫 kernel process」。）

### 3.2 kernel 那一半的映射在每個 process 都一樣

`[06:30]` **事實二**：任何 thread 或 process 的虛擬位址空間都**分成兩半**：

- **使用者**部分在**下面**。
- **kernel** 部分在**上面**。
- 分法依架構不同，但普遍成立。典型例子：32 位元 x86，4 GB，**下 3 GB 給使用者、上 1 GB 給 kernel**。
- 【口頭】「**Windows** 對使用者比較**不慷慨**：2 GB / 2 GB。」
- 64 位元空間很大，也有類似的分法（第 5 講：47 位元的下半 / 上半）。

`[07:00]` **kernel 那部分的映射，在所有 process 都一樣（constant）**：

```
     Process A              Process B             純 kernel thread
 ┌──────────────┐      ┌──────────────┐      ┌──────────────┐
 │ Kernel space │ ═══  │ Kernel space │ ═══  │ Kernel space │   ← 映射完全相同
 ├──────────────┤      ├──────────────┤      ├──────────────┤
 │              │      │              │      │              │
 │ User space A │      │ User space B │      │   （沒有）     │   ← 映射不同
 │ （stack、     │      │              │      │              │
 │  heap、text…）│      │              │      │              │
 └──────────────┘      └──────────────┘      └──────────────┘
```

（課本 Fig 3.10）

- 使用者 process **不能**存取 kernel 區域的位址：TLB 會發現它們在 kernel 區，**不給有效映射、產生 fault**。「對使用者 process 來說這是**禁區（no-go zone）**。」
- `[07:32]` 例子：我在跑 notepad，context switch 進 kernel……之後換成跑 Internet Explorer。**變的只有使用者那部分**，kernel 部分**一樣**。

`[12:31]` 再看一次：

> 「兩個使用者空間**不一樣**：同一個虛擬位址在這兩個空間裡對應到不同位址。**但是、但是、但是**上面的 kernel 空間**一樣**——一樣的意思是**映射完全相同**：同一個 kernel 虛擬位址對應到完全相同的實體位址。」

`[13:00]` **純 kernel thread**（在 kernel 出生、一輩子都在 kernel）：

- **沒有使用者空間**的部分。
- 但它的 kernel 部分還是從同一個地方開始，**映射一樣**。

【課本補充】Way Point 3.3.1：

- 任何 process 的虛擬位址空間都分成使用者和 kernel 位址。
- 所有 kernel thread 共享虛擬位址空間。
- kernel 部分的虛擬位址在**所有 process 的映射都相同**。kernel 虛擬位址永遠對應到同一個實體位址——**跟使用者空間完全不同**（同一個使用者虛擬位址在不同 process 通常對應到不同實體位址，除非是共享記憶體通道）。
- 因為 kernel 空間的映射相同，context switch 時**常常不用換 page table**。

### 3.3 【原始碼】每個新 page table 都抄一份 kernel 的映射

「kernel 那一半在每個 process 都一樣」是怎麼做到的？建立新 process 的 page table（PGD，第 5 講）時：

```c
// arch/x86/mm/pgtable.c:123-141
static void pgd_ctor(struct mm_struct *mm, pgd_t *pgd)
{
	/* If the pgd points to a shared pagetable level (either the
	   ptes in non-PAE, or shared PMD in PAE), then just copy the
	   references from swapper_pg_dir. */
	if (CONFIG_PGTABLE_LEVELS == 2 ||
	    (CONFIG_PGTABLE_LEVELS == 3 && SHARED_KERNEL_PMD) ||
	    CONFIG_PGTABLE_LEVELS >= 4) {
		clone_pgd_range(pgd + KERNEL_PGD_BOUNDARY,
				swapper_pg_dir + KERNEL_PGD_BOUNDARY,
				KERNEL_PGD_PTRS);
	}
	...
}
```

```c
// arch/x86/include/asm/pgtable.h:979-980
#define KERNEL_PGD_BOUNDARY	pgd_index(PAGE_OFFSET)
#define KERNEL_PGD_PTRS		(PTRS_PER_PGD - KERNEL_PGD_BOUNDARY)
```

| 部分 | 意思 |
|---|---|
| `pgd_ctor` | PGD 的「建構子（constructor）」：每建一個新 process 的 PGD 就呼叫 |
| **`swapper_pg_dir`** | **kernel 自己的 PGD**（swapper 就是 idle，第 11 講） |
| `KERNEL_PGD_BOUNDARY` | `PAGE_OFFSET`（kernel 空間起點，第 5 講）在 PGD 的第幾格。**從這格以上就是 kernel 的** |
| `KERNEL_PGD_PTRS` | 512 − 邊界 = kernel 佔幾格 |
| **`clone_pgd_range(...)`** | 把 kernel 的 PGD **上半部那些格子**複製到新 PGD 的上半部 |

為什麼「複製幾格」就夠？第 5 講說過四層 page table：PGD 每一格指向一整棵下層樹。複製的只是**指標**——新 process 的 PGD 上半部，和 kernel 的 PGD 上半部，**指向同一批下層 page table**。所以：

- 每個 process 的 kernel 映射**真的是同一份**（不是內容一樣的兩份）。
- 只要複製 256 個指標左右，非常便宜。

這就是課本說的：「page table 的『kernel 部分』在 process 之間是**共同的**。」

> 補充（不是教授講的）：為了防範 2018 年的 **Meltdown** 漏洞，現代 Linux 有 **KPTI（kernel page table isolation）**：使用者模式時用另一份**幾乎不含 kernel 映射**的 page table，進 kernel 時才換回完整那份。第 2 講 `entry_SYSCALL_64` 裡的 **`SWITCH_TO_KERNEL_CR3`** 就是這件事。所以在開了 KPTI 的 CPU 上，「user → kernel 完全不碰 CR3」不再嚴格成立——但兩份 page table 的 kernel 部分還是共用的，而且配合 PCID（§4.5）不用清整個 TLB。教授講的是基本設計。

---

## 4. 什麼時候要換 page table、清 TLB

### 4.1 兩個選項：TLB 裡加 PID，或清掉

`[08:30]` 從一個使用者 process 換到另一個（user → user，經過 kernel），兩個選項：

**選項一：每個 TLB entry 加上 PID** `[08:30]`

```
  TLB entry
  ┌──────┬───────────┬───────────┬────────┐
  │ PID  │ 虛擬 page  │ 實體 frame │ 權限    │
  └──────┴───────────┴───────────┴────────┘
     ↑ 這筆屬於哪個 process
```

- 每個 TLB entry 說「這筆屬於這個 process ID」。
- 問題：**PID 是軟體 / OS 的概念，TLB 是硬體的概念**。
- 要放幾個位元？10 位元 → 最多 1024 個 process。「但 PID 可能是 4525 呢？這確實造成一點問題。」
- `[10:01]` 「這**打破了一些抽象**：讓硬體知道軟體層級的概念，這有問題。理想上軟體應該知道硬體，**而不是反過來**。這在某種意義上打破了傳統的分離。」
- **但** Intel 處理器**真的有**這個功能，「而且**非常有效**」。

**選項二：沒有 PID 就清掉 TLB** `[09:30]`

- 換使用者 process 時，**清掉 TLB**（至少清掉所有使用者層的映射），並**換 page table**。
- 「通常清 TLB 和換 page table 是**一起發生**的。」

【課本補充】課本補充（p.100）：換 process 時要清 TLB，因為新 process 有新的 memory map。不能讓它用舊 process 的映射——**不但不對，還是嚴重的安全危害**：新 process 就能存取舊 process 的記憶體空間了。在 TLB entry 加上 PID 的方式，由**硬體強制**每個 process 只能用自己的映射，減少 TLB miss，**整體效能提升**。

### 4.2 逐一檢查四種切換

`[10:31]` 教授的主張（「如果不信，記住它，等下一張投影片」）：

`[23:01]` 「換 page table（改 CR3）和清 TLB 是**一起**的：改 page table，TLB 就自動被清掉。」逐一看：

| 切換 | 要換 page table、清 TLB 嗎？ | 為什麼 |
|---|---|---|
| **1. user → kernel** | ❌ **不用** | kernel thread 用的是上面那塊，**每個使用者 process 的上面那塊都一樣** |
| **2. kernel → kernel** | ❌ **不用** | 上面那塊一樣 |
| **3a. kernel → user（同一個）** | ❌ **不用** | 一樣的映射還在。「進 kernel 時**本來就沒換**，現在用的就是它的 page table」 |
| **3b. kernel → user（不同的）** | ✅ **要** | 使用者部分的映射**不一樣**，要清 TLB、改 CR3 |

`[13:31]` 詳細走一次：

1. **使用者程式在跑，發了一個 system call**。
2. 「沒理由為 system call 開一個新 kernel thread」→ **暫時把使用者 thread 升級成 kernel thread**：設一個位元、換成另一個 stack、把 CPU 設成 kernel mode（**ring 0**）。
3. 升級後的 thread 在 kernel 空間裡**很開心地運作**——**不用改任何映射**，繼續用它的 TLB 映射和 page table 映射。
4. `[14:30]` 「這次不會出錯——為什麼？我身為使用者 thread、在 user mode 存取 kernel 空間，很容易被發現『你的特權等級太低』。但我**暫時被升級**到 kernel mode，就能存取 kernel 空間了，那些存取限制對我不適用。」
5. `[15:01]` 回來時，又開始在使用者空間運作。
6. `[15:31]` 假設這個升級的 kernel thread **被換出去**，換成另一個**純 kernel thread**（沒被升級過、一輩子在 kernel）→ **也不用換 TLB 或 page table**：它也映射完全相同的虛擬位址空間、相同的映射。「純 kernel thread 可以一直工作下去，**完全沒問題**。」
7. `[16:00]` 問題出在：從這裡要轉換到一個**不同的**使用者 thread。如果是**同一個**（剛剛那個重新開始）→ 沒問題，「我一直在用它的 page table」。
8. `[16:32]` 「到目前為止我都沒換 page table：user → kernel、kernel → kernel、same，**這個 page table 一路沿用**。」
9. **只有 diff 的情況**：使用者空間那部分要換 → 清 TLB、載入新 process 的 page table。

`[17:03]` 推論：

> 「這是**唯一**昂貴的情況。所以**排程器應該盡量繼續原本在跑的 process**，盡量恢復它，而不是開始一個全新的 process——因為那有這個開銷。」

`[18:00]` 「這是一個**非常關鍵**的知識。」

【課本補充】Point 3.3.1 和 Point 3.3.2：page table **只在使用者空間的虛擬位址映射改變時**才需要換。kernel thread 之間切換、user mode 到 kernel mode，都不用換。「**只有**在執行一個**新的使用者 task**、使用者空間的映射改變時，才需要。」

### 4.3 為什麼 kernel 可以讀使用者的 buffer

`[20:32]` 「如果你還沒體會到的話」：

- 某個 kernel thread（或升級成 kernel thread 的使用者 process）想**存取使用者空間的資料**——這**很常見**。
- 例子：要送東西到網路，就要把**我的空間裡一個 buffer 的指標**送給 kernel，kernel 要從 process 的使用者空間讀資料。
- `[21:01]` kernel 怎麼做到？**很簡單**：它**正在用被打斷的那個使用者 process 的 page table**。用使用者位址也沒人攔它。
- 「整個陣列可以**整套**複製到 kernel 的 buffer。因為用的是同一張 page table，沒問題；不然就有問題了。」

`[21:31]` 但要用 **kernel 專屬的 stack**：

- 很多資訊（包括 context）要存在某個地方，最好的地方就是 **kernel stack**。
- 它也是 kernel thread 的**工作區**。
- **不能是 user mode 的 stack**：安全理由——「kernel mode 寫的東西，回到 user mode 時**不能讓你看到**。」
- 「處理中斷時也會給一個**獨立的中斷 stack**，但**同一個 thread** 繼續——這是提升效率的關鍵一步。」

### 4.4 【原始碼】`context_switch()`：kernel 自己的四格表

kernel 排程器真正做 context switch 的函式：

```c
// kernel/sched/core.c:5247-5303（節錄）
static __always_inline struct rq *
context_switch(struct rq *rq, struct task_struct *prev,
	       struct task_struct *next, struct rq_flags *rf)
{
	prepare_task_switch(rq, prev, next);
	...
	/*
	 * kernel -> kernel   lazy + transfer active
	 *   user -> kernel   lazy + mmgrab() active
	 *
	 * kernel ->   user   switch + mmdrop() active
	 *   user ->   user   switch
	 */
	if (!next->mm) {                                // to kernel
		enter_lazy_tlb(prev->active_mm, next);

		next->active_mm = prev->active_mm;
		if (prev->mm)                           // from user
			mmgrab(prev->active_mm);
		else
			prev->active_mm = NULL;
	} else {                                        // to user
		membarrier_switch_mm(rq, prev->active_mm, next->mm);
		...
		switch_mm_irqs_off(prev->active_mm, next->mm, next);
		lru_gen_use_mm(next->mm);

		if (!prev->mm) {                        // from kernel
			/* will mmdrop() in finish_task_switch(). */
			rq->prev_mm = prev->active_mm;
			prev->active_mm = NULL;
		}
	}
	...
	/* Here we just switch the register state and the stack. */
	switch_to(prev, next, prev);
	barrier();

	return finish_task_switch(prev);
}
```

**第 5259–5265 行的註解，就是教授的四格表**：

| kernel 註解 | 意思 | 對照教授 |
|---|---|---|
| `kernel -> kernel   lazy + transfer active` | **lazy**（不換 page table），把 `active_mm` **轉交**給下一個 | 情況 2：不用換 |
| `user -> kernel   lazy + mmgrab() active` | **lazy**，**借用**前一個的 `mm`（引用計數 +1） | 情況 1：不用換 |
| `kernel -> user   switch + mmdrop() active` | **switch**（換 page table），放掉借來的 `mm` | 情況 3a / 3b |
| `user -> user   switch` | **switch** | 經過 kernel 的情況 3b |

（這裡的「kernel / user」指的是 `prev` 和 `next` 是 **kernel thread 還是使用者 process**——也就是有沒有 `mm`。）

逐行看：

| 行 | 意思 |
|---|---|
| **5266** | **`if (!next->mm)`**：下一個 task **沒有 `mm`** → 它是 **kernel thread**（第 11 講：kernel thread 沒有使用者記憶體） |
| 5267 | `enter_lazy_tlb`：進入「**懶惰 TLB 模式**」——**不換 CR3、不清 TLB** |
| **5269** | **`next->active_mm = prev->active_mm`**：kernel thread **借用前一個 task 的 `mm`**。這就是第 9、11 講說的「`mm` 和 `active_mm` 兩個指標都需要」的原因——kernel thread 的 `mm` 是 NULL（它沒有自己的使用者空間），但 `active_mm` 指向它**正在借用**的那張 page table（反正 kernel 那一半都一樣，§3.3） |
| 5270–5271 | 前一個是使用者 process → `mmgrab` 引用計數 +1，防止借用期間那個 `mm` 被釋放 |
| **5274** | `else`：下一個是**使用者 process** |
| **5284** | **`switch_mm_irqs_off(prev->active_mm, next->mm, next)`**：**換 page table**（裡面會判斷是不是同一個 `mm`，§5.3） |
| **5299** | **`switch_to(prev, next, prev)`**：註解「這裡**只**切換暫存器狀態和 stack」——**硬體 context** 的切換（下一講細講） |

**記憶體（軟體 context 的一部分）和暫存器（硬體 context）是分開兩步處理的**：先決定要不要換 page table（5266–5292），再換暫存器和 stack（5299）。

### 4.5 【原始碼】PCID：TLB 裡的「PID」

教授說的「選項一：TLB entry 加上 PID，Intel 處理器有」，在 x86 上叫 **PCID**（Process-Context Identifier）。第 5 講看過 `load_new_mm_cr3()` 裡的 ASID，就是它：

```c
// arch/x86/include/asm/tlbflush.h:57-61
/*
 * 6 because 6 should be plenty and struct tlb_state will fit in two cache
 * lines.
 */
#define TLB_NR_DYN_ASIDS	6
```

**對照教授的擔憂**：

| 教授說的 | 實際做法 |
|---|---|
| 「PID 是軟體概念，TLB 是硬體概念，硬體要知道軟體的東西」 | 硬體**不知道** Linux 的 PID。TLB 裡放的是 **PCID**（x86 硬體有 **12 位元**），由 kernel **自己決定**怎麼對應 |
| 「10 位元只能 1024 個 process，PID 4525 怎麼辦？」 | Linux **根本不把 PID 放進去**。每顆 CPU 只用 **6 個**動態 ASID（`TLB_NR_DYN_ASIDS`）：記住這顆 CPU **最近跑過的 6 個 `mm`**，各給一個 ASID。換到最近跑過的 process → 用它原本的 ASID，**不用清 TLB**；換到第 7 個 → 回收最舊的 ASID、只清那一個 |
| 為什麼是 6 | 註解：「6 **應該夠用**，而且 `struct tlb_state` 剛好塞得進**兩條快取線**」——又是快取線意識 |

這解決了教授說的「打破抽象」問題：硬體只提供一個小小的標籤欄位，**怎麼用是軟體的事**。

這台電腦的 CPU 支援 PCID（`/proc/cpuinfo` 的 flags 裡有 `pcid`）。

---

## 5. 三種 context switch

`[18:00]` 有了上面的關鍵知識，看三種 context switch：

| 種類 | 說明 |
|---|---|
| **Process context switch** | process 之間的一般切換（單執行緒、不屬於 thread group） |
| **Thread context switch** | 同一個 thread group 裡的 thread 之間切換 |
| **Interrupt context switch** | 中斷來了 |

### 5.1 Process context switch

`[18:30]` 一般 process 因為 system call 進 kernel：

- **不建**獨立的 kernel thread，**同一個 thread** 繼續。
- system call 時，CPU 模式（ring level）從 user 轉到 kernel，變成 **ring 0**。
- 讓同一個 process 繼續，它變成**升級的 kernel process**，可以存取所有 kernel page。

`[19:30]` 第一件事：**存硬體 context**。

- 硬體 context 裡有些**非常重要（VIP）**的成員，最重要的是 **PC**——「因為執行任何別的東西，PC 都會改變，原本的 PC 就丟了。」
- 存好之後，thread 就能在 kernel mode 繼續執行。

`[20:02]` 這**不是完整的切換**，而是**軟切換（soft switch）**：

- 存 context、改 process 的模式。
- 比完整的 process 切換簡單。
- 虛擬位址空間**一定**一樣（「不是可以一樣，是**會**一樣」），只是**換 stack**：不用 user mode stack，用 **kernel mode stack**。

【課本補充】課本補充（p.102）：

- context switch 可以由三種**感興趣的事件**觸發：中斷、exception、system call。另外還有 **timer 晶片**產生的假中斷（第 3 講），讓 kernel 能週期性執行。
- 處理完事件，kernel 跑**排程器**：決定目前的 task 是不是跑太久了、要不要暫停它、把 CPU 給別的 task。
- 事件來時，**硬體**接手、做**最少量**的 context 儲存，然後依事件性質呼叫適當的 handler（第 2 講：硬體只存一部分，handler 存剩下的）。
- timer interrupt 的話，進 kernel 的理由「不嚴重，只是例行公事」。「把它想成同一個 thread 的**新化身**，從使用者層**升上**了 kernel 層。」
- 這個轉換後的 kernel thread 的工作，就是**繼續存硬體 context**：收集所有暫存器的值、存起來。大多數架構存在 **kernel stack** 上。

### 5.2 Thread context switch

`[25:31]` 「某個東西概念上聽起來很好，但實作時有**獨特的挑戰**。」

- **thread 是 OS 排程的原子單位**，每個 thread 都是一個 task、有自己的 `task_struct`。
- 叫它 thread，是因為它跟 thread group 裡其他 thread **共享虛擬位址空間**。
- 所以同一個 thread group 裡的 thread 切換，**應該**比完整的 process 切換容易——「但**怎麼**容易？需要更多解釋。」

`[26:00]` 從 T1 切到 T2（同一個 thread group）：

1. 存 T1 的 context（暫存器、flags、PC 等）——「顯而易見的動作」。
2. **想要做的**（教授打問號）：只換掉虛擬→實體映射裡**thread 私有**的那部分——**stack** 和 **TLS**。
3. 把 `current` 指標換成 T2。
4. 還原 T2 的 context。

`[27:31]` 「只換掉虛擬位址空間的**一部分**」：

> 【口頭】「像一幅大畫，有一塊你不喜歡，想把那塊**剪掉換一塊新的**——這**很難**、很麻煩，我們不喜歡。」

`[28:30]` **大膽的想法**（「然後我們會發現其實沒那麼大膽」）：

> 「為什麼不用**跟 kernel thread 一樣的解法**：thread group 裡**所有 thread 共享同一個虛擬位址空間**？」

問題：stack 和 TLS 怎麼辦？`[29:30]` 做法——**跟 kernel stack 一樣**（第 8 講）：

```
   一個 thread group 共用的虛擬位址空間
  ┌──────────────────────┐
  │ Stack（user thread 1）│  ← 各自私有，但都在同一個空間裡
  │ Stack（user thread 2）│     間隔夠大，不會互相覆蓋
  │ Stack（user thread 3）│
  │        ...           │
  │ Heap                 │  ← 共享
  │ Data / BSS           │  ← 共享
  │ Text                 │  ← 共享
  └──────────────────────┘
```

- 「這是 user thread 1 的 stack、user thread 2 的 stack、user thread 3 的 stack……」
- stack 是用**相對於 stack pointer 的偏移**來定址的，所以每個 thread **還是**有自己的私有 stack，只是**所有私有 stack 都在同一個虛擬位址空間裡**。
- 啟動 thread 時說：「你是 thread 3，這是你的 stack。」
- 切換時**不用對 stack 做任何事**：T2 的 stack pointer 會回到它之前的位置。
- **TLS 也可以這樣做**。

`[30:32]` 「這個想法的偉大之處：所有 thread 用**完全一樣的虛擬位址空間**，但每個 thread 還是有不同的 stack 和私有區域——**抽象仍然維持**。因為虛擬位址空間一樣，**同一個 thread group 裡切換 thread 時，完全不用更新 page table 或 TLB**——省下很多效能。」

【口頭】教授的比喻 `[31:01]`：

> 「我們兩個都講**英文**，你聽不懂某個字可以查**英文字典**。現在換**另一個老師**來，他也講英文——**不用換字典**，同一本英文字典還能用。我桌上放一本小字典（像 **TLB**），那邊放一本厚的大字典（像 **page table**）。
>
> 問題是如果換來的老師**不是我這組的**——我是講英文的組，新老師講**印地語**。那字典就要換。字典不會自己換，決定我出去、新老師進來的那個人——也就是 **OS**，在這裡是**班導**——要把英文字典拿走、換成印地語字典，小的放我桌上、大的放那邊，印地語課才能開始。」

`[33:00]` 「同一個 thread group 的所有 thread 用同一個虛擬位址空間，所以能用同一張 page table。問題出在換到**不同的 process**：那需要不同的 page table、不同的 TLB entry，所以要清 TLB、換 page table。」

【課本補充】課本補充（p.103–104）：

- **Point 3.3.3**：**task 是 kernel 排程的原子單位**。kernel 雖然支援 thread group，但所有主要的排程決定都在 **task**（單一執行緒）的層級。
- 要確保 stack 之間在虛擬位址空間裡的**間隔夠大**，一個 stack 溢位時才不會蓋掉另一個。
- 在支援 segmentation 的 x86 上更容易：把 **stack segment 暫存器**設成 stack 的起點——它是目前 thread 在 thread group 裡 id 的函數。
- **Point 3.3.4**：Linux 裡同一個 thread group 的 thread **共享完整的虛擬位址空間**，各 thread 的 stack 和 TLS 存在這個共享空間的不同位置。
- 還是要存和還原**暫存器狀態**，並把 `current` 設成新 thread 的 `task_struct`。
- 「很多 kernel 通常會**偏好**同一個 thread group 的另一個 thread，而不是不相干的 thread。」

### 5.3 【原始碼】同一個 process 的 thread 切換：不用清 TLB

§4.4 第 5284 行的 `switch_mm_irqs_off()` 裡面，第一件事就是檢查：

```c
// arch/x86/mm/tlb.c:554-573（節錄）
	if (real_prev == next) {
		VM_WARN_ON(this_cpu_read(cpu_tlbstate.ctxs[prev_asid].ctx_id) !=
			   next->context.ctx_id);
		...
		/*
		 * If the CPU is not in lazy TLB mode, we are just switching
		 * from one thread in a process to another thread in the same
		 * process. No TLB flush required.
		 */
		if (!was_lazy)
			return;
```

| 行 | 意思 |
|---|---|
| **554** | **`if (real_prev == next)`**：前一個和下一個的 **`mm_struct` 是同一個**——也就是**同一個 process 的兩個 thread**（第 11 講：建 thread 時 `CLONE_VM` 讓它們共用同一個 `mm`） |
| 567–571 註解 | 「如果 CPU 不在懶惰 TLB 模式，我們只是**從一個 process 裡的一個 thread 切換到同一個 process 的另一個 thread**。**不需要清 TLB。**」 |
| **572–573** | **直接 `return`**——不碰 CR3、不清 TLB |

這就是教授「英文老師換英文老師，字典不用換」的原始碼版本。整個 thread context switch 的最佳化，在記憶體這部分就是**一個比較加一個 return**。

### 5.4 Interrupt context switch：top half 與 bottom half

`[33:30]` 硬體中斷來了，常常要**很快**處理：

- 這裡也**不開新 thread**：**升級現有的使用者 thread**，讓它變成 kernel thread 去處理中斷。
- 它**不用**自己的 user mode stack，用一個**獨立的中斷 stack**：每顆 CPU 有 **7 個**中斷 stack，挑空的那個用（第 8 講）。

`[34:00]` 中斷處理的程式碼叫 **top half（上半部）**，有**很多限制**。

【口頭】教授的比喻：

> 「假設正在上課，我的系上打電話來有個小緊急狀況。我會跟他們說：『我能做的不多，你們把我從一堂正在上的課叫出來。我可能做 **15 秒或 20 秒**的事，就要回去。**剩下的明天再做**。』那個延後的工作就是 **bottom half（下半部）**。OS 運作方式完全一樣。」

| | Top half | Bottom half |
|---|---|---|
| 是什麼 | 中斷 handler 本身 | 延後做的工作，交給一個**完整的 kernel task** |
| 什麼時候 | **馬上** | **之後** |
| 長度 | **非常短**。理論上可以很長，但慣例上不做很多計算；程式碼或 stack 大小有限 | 做**大部分**的中斷處理 |
| 優先權 | **非常高** | 較低 |
| **能用鎖嗎** | ❌ **不能** | ✅ 能——各種 blocking / non-blocking 同步都能用 |

`[35:02]` 為什麼 top half **不能拿鎖**？

> 「你永遠不知道鎖什麼時候會被釋放。而且中斷 thread 的優先權**非常高**，那個應該要跑來釋放鎖的 thread **可能永遠跑不到**。」

（高優先權的在等低優先權的放鎖，低優先權的又被高優先權的擋住跑不了——這就是**死結**。）

`[36:00]` top half 在 **kernel 虛擬位址空間**運作，**也不用換 TLB 和 page table**，理由一樣：它是 kernel thread，用自己的 stack。

【課本補充】課本補充（p.104）：

- 處理 timer interrupt 通常是例行公事，但其他中斷——特別是**不可遮罩中斷（NMI）**——優先權高得多。
- interrupt handler 很特別，因為它的**程式碼大小**、**需要存取原生硬體**、而且**跟任何使用者 thread 無關**都有限制；也**不准用鎖**。
- top half / bottom half 是**經典**的模式。「當然這個機制現在變得更精密了，但基本想法一樣：**高優先權的工作馬上做，低優先權的工作延後做**。」（第 16 講的 softirq、threaded IRQ、work queue）

---

## 6. 兩個大想法

`[36:31]` 教授的總結：

| # | 大想法 | 效果 |
|---|---|---|
| 1 | **把虛擬位址空間分成 kernel 和 user 兩半**，kernel 那一半在**所有 page table 裡都一樣**（包括 kernel thread 的 page table） | 減少 context switch 的開銷：user → kernel、kernel → kernel、回到同一個 user 都**不用換 page table、不用清 TLB** |
| 2 | **同一個 thread group 的 thread，不替換 stack 和 TLS 的映射**，而是把它們放在**共享虛擬記憶體的不同位置** | thread 之間切換也**不用清 TLB、不用換 page table** |

`[37:00]` 「context switch 的**大方向**講完了：高階步驟，以及**什麼時候**需要清 TLB 和換 page table。下一講看 context switch 的**低階步驟**。」

---

## 7. 【實測】一秒有多少次 context switch

```bash
$ grep ctxt /proc/stat; sleep 1; grep ctxt /proc/stat
ctxt 66094263
ctxt 66095771
```

- `/proc/stat` 的 `ctxt` 是**開機以來**全系統 context switch 的總次數。
- 隔 1 秒差了 **1,508 次**——這台桌機閒置時，每秒大約 1500 次 context switch。
- 所以教授這麼在乎「**能不換 page table 就不換**」：每次都換、都清 TLB 的話，每秒上千次 TLB 冷啟動。

每個 process 自己的統計：

```bash
$ cat /proc/self/status | grep ctxt
voluntary_ctxt_switches:	1
nonvoluntary_ctxt_switches:	0
```

| 欄位 | 意思 |
|---|---|
| `voluntary` | **自願**讓出 CPU 的次數（例如去睡覺等 I/O）——第 7 講的 `TASK_INTERRUPTIBLE` |
| `nonvoluntary` | **被搶占**的次數（例如 timer interrupt 時排程器覺得它跑太久）——第 7 講的「被搶占但狀態還是 `TASK_RUNNING`」 |

（`cat` 很快就跑完，所以只有 1 次自願切換。）

---

## 8. 一頁速查表

### 8.1 Context

| 類別 | 內容 | context switch 時 |
|---|---|---|
| 硬體 context | 一般暫存器、PC、segment 暫存器、CR3 等特權暫存器、ALU / 浮點 flags | **存起來、還原** |
| 軟體 context | 開啟檔案、網路連線、page table、統計資料 | **不用動**（在 `task_struct` 裡） |
| CR3 / TLB | 改 CR3 → 換 page table → 清 TLB | **能不換就不換** |

### 8.2 四種切換

| 切換 | 換 page table / 清 TLB？ |
|---|---|
| 1. user → kernel | ❌ |
| 2. kernel → kernel | ❌ |
| 3a. kernel → user（同一個） | ❌ |
| 3b. kernel → user（不同的） | ✅ **唯一要換的** |
| user → user | 不存在，一定經過 kernel |

### 8.3 為什麼大部分不用換

| 事實 | 原始碼 |
|---|---|
| 所有 kernel thread 共用虛擬位址空間 | — |
| kernel 那一半的映射在每個 process 都**是同一份** | `pgd_ctor`：從 `swapper_pg_dir` 複製上半部 PGD 指標 |
| kernel thread 沒有 `mm`，借用前一個的 `active_mm` | `context_switch`：`next->active_mm = prev->active_mm` |
| 同一個 process 的 thread 共用 `mm` | `switch_mm_irqs_off`：`real_prev == next` 就 return |

### 8.4 TLB 標籤

| 教授 | 實際（x86） |
|---|---|
| TLB entry 加 PID | **PCID**（12 位元） |
| 打破抽象、位元不夠 | Linux 不放 PID，每顆 CPU 只用 **6 個**動態 ASID，記最近跑過的 6 個 `mm` |

### 8.5 三種 context switch

| 種類 | 重點 |
|---|---|
| Process | 同一個 thread 升級成 kernel thread（軟切換）；換 kernel stack；ring 0 |
| Thread | thread group 共享整個位址空間；stack 和 TLS 放在不同位置；不用換 page table |
| Interrupt | 升級目前 thread；用中斷 stack；top half 短、不能拿鎖；bottom half 延後做 |

### 8.6 Top half vs Bottom half

| | Top half | Bottom half |
|---|---|---|
| 時機 | 馬上 | 之後 |
| 長度 | 很短 | 大部分工作 |
| 鎖 | 不行 | 行 |
| 比喻 | 上課被叫出去，只處理 15 秒 | 剩下的明天做 |

### 8.7 本講看過的原始碼位置

| 檔案:行號 | 看什麼 |
|---|---|
| `arch/x86/mm/pgtable.c:123-141` | `pgd_ctor()`：複製 kernel 的 PGD 上半部 |
| `arch/x86/include/asm/pgtable.h:979-980` | `KERNEL_PGD_BOUNDARY`、`KERNEL_PGD_PTRS` |
| `kernel/sched/core.c:5247-5303` | `context_switch()`：四格表註解、`active_mm` 借用、`switch_to` |
| `arch/x86/mm/tlb.c:554-573` | 同一個 `mm` → 不清 TLB |
| `arch/x86/include/asm/tlbflush.h:57-61` | `TLB_NR_DYN_ASIDS` = 6 |

---

## 9. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **`switch_to`：切換暫存器狀態和 stack 的細節** | **第 13 講**：context switch 細節 II（下一講） |
| **system call 時同一個 thread 升級、換 kernel stack** | **第 14 講**：library call 與 system call |
| **中斷 stack、硬體做最少量的 context 儲存** | **第 15 講**：x86 中斷架構 |
| **Top half / bottom half** | **第 16 講**：softirq、threaded IRQ、work queue |
| **Top half 不能拿鎖、死結** | **第 20–22 講**：鎖、kernel 裡的並行 |
| **排程器偏好繼續同一個 process / 同 thread group** | **第 24–26 講**：排程 |
| **ASID / PCID、TLB 管理、TLB shootdown** | **第 30 講**：kernel 的記憶體架構（課本 §6.2.4） |
| **`lru_gen_use_mm`（第 5285 行）** | **第 32 講**：MGLRU |

---

## 附：本講一句話總結

> **Context switch 要存還原的是硬體 context（暫存器、PC、flags、CR3），但最貴的是改 CR3——那會換 page table、清掉 TLB。Linux 靠兩個設計讓它幾乎不用發生：每個 process 的 page table 上半部都指向同一份 kernel 映射（`pgd_ctor` 複製 PGD 指標），所以 user→kernel、kernel→kernel、回到同一個 user 都不用換，只有切到「不同的」使用者 process 才要換（kernel 的 `context_switch()` 註解就是這張四格表，kernel thread 沒有 `mm` 就借用前一個的 `active_mm`）；同一個 thread group 的 thread 共用整個位址空間、把 stack 放在不同位置，所以 `real_prev == next` 時直接 return，連 TLB 都不用清。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 12（Chapter 3, Part VI）錄影逐字稿（YouTube 自動字幕，37:51）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§3.3.1–3.3.2，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0）結果。*
