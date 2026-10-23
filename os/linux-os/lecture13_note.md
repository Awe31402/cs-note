# Operating Systems: A Linux Kernel-Oriented Approach 第十三講完整筆記：Context Switch 細節（Chapter 3, Part VII）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 13（Chapter 3, Part VII）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（44:16，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§3.3.3–3.4（p.104–110）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
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
- [1. 存 context：system call 進來時](#1-存-contextsystem-call-進來時)
  - [1.1 入口：`entry_SYSCALL_64`](#11-入口entry_syscall_64)
  - [1.2 三個特等公民：PC、flags、stack pointer](#12-三個特等公民pcflagsstack-pointer)
  - [1.3 步驟](#13-步驟)
  - [1.4 為什麼 stack pointer 要先放 TSS](#14-為什麼-stack-pointer-要先放-tss)
  - [1.5 存的時候要關中斷](#15-存的時候要關中斷)
- [2. 還原 context：`sysret` 與 `iret`](#2-還原-contextsysret-與-iret)
  - [2.1 反過來做](#21-反過來做)
  - [2.2 還原時不用關中斷](#22-還原時不用關中斷)
  - [2.3 `sysret` vs `iret`](#23-sysret-vs-iret)
  - [2.4 【原始碼】`sysretq` 的回程](#24-原始碼sysretq-的回程)
- [3. 額外的 context：`thread_struct`](#3-額外的-contextthread_struct)
- [4. 進了 kernel 之後：kernel 是機會主義者](#4-進了-kernel-之後kernel-是機會主義者)
  - [4.1 `exit_to_user_mode_loop`](#41-exit_to_user_mode_loop)
  - [4.2 【原始碼】回使用者空間前檢查 `TIF_NEED_RESCHED`](#42-原始碼回使用者空間前檢查-tif_need_resched)
- [5. `context_switch()` 的五個步驟](#5-context_switch-的五個步驟)
  - [5.1 跨架構的「超集合」](#51-跨架構的超集合)
  - [5.2 切換記憶體結構：`mm` 與 `active_mm`](#52-切換記憶體結構mm-與-active_mm)
  - [5.3 切換暫存器和 stack：`switch_to`](#53-切換暫存器和-stackswitch_to)
  - [5.4 【原始碼】`__switch_to_asm`：真正換 stack 的兩行](#54-原始碼__switch_to_asm真正換-stack-的兩行)
  - [5.5 【原始碼】`__switch_to`：其他的 context](#55-原始碼__switch_to其他的-context)
  - [5.6 【原始碼】新 process 第一次跑：`ret_from_fork`](#56-原始碼新-process-第一次跑ret_from_fork)
  - [5.7 收尾：`finish_task_switch` 與 kmap](#57-收尾finish_task_switch-與-kmap)
- [6. C 的可見性：底線和 `static`](#6-c-的可見性底線和-static)
- [7. 兩個小知識](#7-兩個小知識)
  - [7.1 `likely` / `unlikely`](#71-likely--unlikely)
  - [7.2 `__latent_entropy`](#72-__latent_entropy)
- [8. 第 3 章總結](#8-第-3-章總結)
- [9. 一頁速查表](#9-一頁速查表)
- [10. 與後續講次的連結](#10-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

`[00:33]` 上一講看了**大方向**（什麼時候要換 page table），這一講看**細節**。

| # | 主題 | 對應課本 |
|---|---|---|
| 1 | system call 進來時**怎麼存** context | §3.3.3 |
| 2 | 回去時**怎麼還原**：`sysret`、`iret` | §3.3.3 |
| 3 | 額外的 context：`thread_struct` | §3.3.3 |
| 4 | 進了 kernel 之後：排程、`context_switch()` 的五個步驟 | §3.3.4 |
| 5 | 雜談：C 的可見性、`likely`、`__latent_entropy` | §3.3.4 Trivia |
| 6 | 第 3 章總結 | §3.4 |

第 2 講已經看過 `entry_SYSCALL_64` 和 `pt_regs` 的程式碼。這一講從**流程和理由**的角度再看一次，並往下看到 `switch_to`。

---

## 1. 存 context：system call 進來時

### 1.1 入口：`entry_SYSCALL_64`

`[00:33]` 去看 **`arch/x86/entry/entry_64.S`** 這個組合語言檔：system call 和中斷都是用**組合語言**寫的。

- 第一件事當然是**存下剛被打斷的程式的狀態**。
- system call 和中斷是**不同的入口**，但邏輯大致類似。這裡**主要看 system call**，中斷的差異會指出來。
- `[01:01]` **`entry_SYSCALL_64`** 是 system call 的**唯一入口**。它**註冊在硬體的一張表裡**：執行 `syscall` 這個會打斷的指令時，硬體知道要呼叫哪個函式。

`[01:30]` 你會在 kernel 裡看到 **`SYM_CODE_START`** 這個指令（directive）：表示這個函式是**用組合語言寫的**。

> 【口頭】「這裡**非常小心地**存整個狀態。這是一套**精心編排**的動作，**絕對不能出錯**。有精確的順序，**任何兩步都不能對調**，不然整個順序就錯了，context 就存不對。」

【課本補充】為什麼一定要用組合語言？課本說（p.105）：因為要存取**個別暫存器**，特別是很多**特權暫存器**——x86-64 叫做 **MSR（model-specific register，處理器特定暫存器）**。

### 1.2 三個特等公民：PC、flags、stack pointer

`[15:00]` 存和還原時，最關鍵的是三個——教授叫它們**「特等公民（primary / special citizens）」**：

| 特等公民 | x86-64 暫存器 | 為什麼特別 |
|---|---|---|
| **指令指標（PC）** | `rip` | 一執行任何別的程式碼，PC 就變了 |
| **Flags** | `rflags` | 每條指令都可能改它 |
| **Stack pointer** | `rsp` | 要換成 kernel stack，舊的值會被蓋掉 |
| （code segment） | `cs` | 中斷時 code segment 可能會變，邏輯上當作 PC 的一部分 |

課本說這些是**短暫的（ephemeral）**：每條指令都在變。

### 1.3 步驟

`[02:00]`（課本 §3.3.3 步驟 1–6）

| 步 | 誰做 | 做什麼 |
|---|---|---|
| **1** | **硬體** | **system call**：把 PC（`rip`）存進 **`rcx`**，把 flags（`rflags`）存進 **`r11`** |
| **2** | **硬體** | **中斷**：不能用 `rcx`、`r11`（見下），改把 `rip`、`cs`、`rflags` **推進 stack** |
| **3** | 軟體 | **`swapgs`**：把 `gs` 的內容跟一個 **MSR** 交換——把使用者的 `gs` 存起來，換成 kernel 的（第 6 講） |
| **4** | 軟體 | 把 **stack pointer（`rsp`）** 存進 **TSS**（Task State Segment） |
| **5** | 軟體 | 把 stack 換成 **kernel stack** |
| **6** | 軟體 | 把剩下的狀態推進 kernel stack：data segment、**從 TSS 拿回的 `rsp`**、`r11`（flags）、code segment、`rcx`（PC）、其他一般暫存器 |

**為什麼步驟 1 一定是硬體做的？** `[02:30]`

> 「這**不能用程式碼**做。用程式碼做，就要改 PC 去跑那段負責存 PC 的程式碼——那要存的是**誰的** PC？所以每次 system call，**一定要硬體做**。」

**為什麼 system call 可以用 `rcx`、`r11`，中斷不行？** `[02:30]`

| | System call | 中斷 |
|---|---|---|
| 性質 | **計畫好的活動（planned）** | **沒計畫的活動（unplanned）** |
| `rcx`、`r11` | 假設**沒有有用的資料**——**library 要先清空**它們（第 2 講：`syscall` 會蓋掉這兩個） | 中斷**隨時**會來，不可能事先空出來 |
| 硬體把 PC、flags 放哪 | **暫存器**（`rcx`、`r11`） | **stack**（或 MSR、專用記憶體區） |

> 【口頭】「保持暫存器空著是**計畫好的事**，但中斷是**沒計畫的事**。」

`[17:31]` 「system call 是計畫好的，所以可以**更有效率**一點：用暫存器。中斷沒計畫，不能把暫存器空著，所以 `rip` 和 `rflags` 會被推到 stack 上。」

（第 2 講看過的 `entry_SYSCALL_64` 對照：`pushq %r11 /* pt_regs->flags */`、`pushq %rcx /* pt_regs->ip */`——就是把硬體放在 `r11`、`rcx` 的 flags 和 PC 推進 kernel stack，也就是**步驟 6**。）

### 1.4 為什麼 stack pointer 要先放 TSS

`[04:30]` stack pointer 是**最麻煩**的：

> 「要載入 kernel stack 之前，得先把**現在的** stack pointer 存在某個地方。你看其他東西都被推到 stack 上——但 stack pointer 本身要存哪？不存起來，之後怎麼拿回那些東西？」

`[05:00]` 存在一個**專用的硬體 segment**：**TSS（Task State Segment）**——Intel 架構提供的。

`[07:33]` 完整的舞步：

```
  ① 把使用者的 rsp 暫存到 TSS          （不能存 kernel stack：要先改 rsp 才能用它，舊值就沒了）
  ② rsp ← kernel stack 的頂端           （覆蓋 stack pointer）
  ③ 把 TSS 裡的使用者 rsp 搬到 kernel stack
  ④ 其他一般暫存器全部推進 kernel stack
```

【課本補充】課本說（p.106）：幾乎所有 x86 / x86-64 處理器每顆 CPU 都有一個 **TSS**。它很小，但用來存 context switch 的重要資訊。**以前用來存 task 的整個 context**；現在只存硬體 context 的一部分。x86-64 上 `rsp` 存在這裡。「**很遺憾沒有別的選擇**」：不能用 kernel stack（要先更新 stack pointer，舊值就丟了），也不能用一般暫存器。

（第 2 講 `entry_SYSCALL_64` 的 `movq %rsp, PER_CPU_VAR(cpu_tss_rw + TSS_sp2)`；第 8 講看過 TSS 的結構，`sp2` 那格「因為 Linux 不用 ring 2」被拿來當暫存空間。）

### 1.5 存的時候要關中斷

`[08:00]` 整個存的過程**非常敏感**：

> 「要**關掉中斷**。不然中斷一來、interrupt handler 一跑，**整個狀態就亂了**。所以在存狀態的這段關鍵區間關中斷，狀態存好就**馬上**打開。」

【課本補充】課本補充（p.105）：如果允許被打斷，狀態會**只存了一部分**，其餘的丟掉。所以最簡單的做法是一開始關中斷、context 完全存好再打開。「當然這**會稍微延遲**中斷處理，但我們可以確定 context 存對了。」

（第 2 講 `entry_SYSCALL_64` 裡的註解：`/* IRQs are off. */`。）

---

## 2. 還原 context：`sysret` 與 `iret`

### 2.1 反過來做

`[08:32]` 還原時：**完全反過來**的順序。

- **最後一步永遠是載入指令指標**：回到被打斷的那個 process 的那個點（第 2 講：PC 或 next PC，看情況）。
- 要從 kernel mode 回 user mode，還要**切換模式**。
- **`sysret`** 和 **`iret`** 指令就做這件事——還原的最後階段。

`[09:30]` `sysret` 是 `syscall` 的反向：

- `syscall` 是現代發 system call 的機制，從 system call 返回就是 `sysret`。
- `rcx` 存著要回去的指令指標 → 搬回 `rip`。
- `r11` 存著 flags → 搬回 `rflags`。

### 2.2 還原時不用關中斷

`[10:01]` 還原時中斷進來，會有很多**競爭情況（race condition）**嗎？

> 「還原狀態**沒有存狀態那麼關鍵**。存的時候狀態要**完好無缺**；但還原時，狀態**反正在資料結構裡**，不用那麼擔心。」

`[10:31]` 例子：中斷在「設好使用者 stack」和「執行 `sysret`」**之間**來了——**沒問題**：

- interrupt handler 本來就會**存和還原**狀態。
- 這也說明為什麼 interrupt handler 要用**獨立的 stack**：還原時正在存取 kernel stack，不想讓 handler 也用同一個 stack。
- 任何 handler 返回時都會**還原它開始時的狀態**。
- 「就算你是 kernel、正在還原某個使用者 process 的狀態，然後跳去某個 interrupt handler，回來後還是會繼續、最後回到使用者 process。」

`[11:31]` 另一個角度：

> 「你在 kernel mode 關了中斷，然後回到 user mode——使用者 process **沒辦法打開中斷**。你可以說讓硬體在進 user mode 時自動打開（例如讓 `sysret` 指令順便做），但如我們討論的，**技術上不需要**。而且你不會想**長時間關中斷**，那會讓 kernel **反應變差**。」

【課本補充】課本說：整個還原序列**不能關中斷**——「速度會慢到無法接受」。中斷在還原 `rsp` 和 `sysret` 之間來了，可以用中斷 stack table（IST，第 8 講）裡的專用 stack 執行 handler，**沒有正確性問題**。

### 2.3 `sysret` vs `iret`

`[12:30]`

| | `sysret` | `iret` |
|---|---|---|
| 從哪裡返回 | **system call** | **中斷**（interrupt return） |
| PC、flags 從哪裡來 | **暫存器**：`rcx` → `rip`、`r11` → `rflags` | **stack**：從 stack 上取回 `rip`、`cs`、`rflags` |
| 為什麼 | system call 是計畫好的 | 中斷沒計畫，進來時硬體把它們推在 stack 上 |
| 切換模式 | kernel → user | kernel → user |

`[13:00]` 「設定指令指標，**等於跳到使用者程式的某個點**。好像你回到使用者程式、跳到裡面的某一點。」

`[16:32]` 還原時，`rsp`、`rflags`、`rip` 也是特等公民：

1. 用完 kernel stack 後，還原 **`rsp`**：`rsp` 變成指向**使用者 stack** 的指標。
2. 「但注意：`rsp` 一指向使用者 stack，**kernel stack 就不能再用了**。」——沒關係，那時已經全部還原好了。
3. 最後還原 `rip` 和 `rflags`：
   - system call：它們在兩個暫存器裡，`sysret` 自動取用。
   - 中斷：它們已經在使用者 stack 頂端（⚠️ 教授說「推在使用者 stack 上」；嚴格說 x86-64 的 `iret` 是從**目前的 stack 頂端**取 `rip`、`cs`、`rflags`、`rsp`、`ss` 五個值），`iret` 把它們取出來、跳過去。

【課本補充】課本強調：**`rip` 在最後才設**——「設定指令指標就等於從中斷返回」。這個指令指標指向使用者 process 虛擬位址空間裡的位置，「所以等於跳到使用者 process 的返回位址」。

### 2.4 【原始碼】`sysretq` 的回程

`entry_SYSCALL_64` 呼叫完 `do_syscall_64`（第 2 講）之後的回程：

```asm
// arch/x86/entry/entry_64.S:132-136（節錄）
	movq	RCX(%rsp), %rcx
	...
	cmpq	%rcx, %r11	/* SYSRET requires RCX == RIP */
	jne	swapgs_restore_regs_and_return_to_usermode
```

```asm
// arch/x86/entry/entry_64.S:198-226（節錄）
syscall_return_via_sysret:
	IBRS_EXIT
	POP_REGS pop_rdi=0

	/*
	 * Now all regs are restored except RSP and RDI.
	 * Save old stack pointer and switch to trampoline stack.
	 */
	movq	%rsp, %rdi
	movq	PER_CPU_VAR(cpu_tss_rw + TSS_sp0), %rsp
	...
	SWITCH_TO_USER_CR3_STACK scratch_reg=%rdi

	popq	%rdi
	popq	%rsp
SYM_INNER_LABEL(entry_SYSRETQ_unsafe_stack, SYM_L_GLOBAL)
	ANNOTATE_NOENDBR
	swapgs
	sysretq
```

| 行 | 意思 | 對照 |
|---|---|---|
| 132–136 | 從 `pt_regs` 取回 PC 放進 `rcx`；**檢查 `rcx == rip`**（註解：「SYSRET 要求 RCX == RIP」）。如果使用者的 context 被改過（例如被 ptrace 改了 PC），不符合 `sysret` 的條件 → **改走 `iret` 的路**（`swapgs_restore_regs_and_return_to_usermode`） | `sysret` 比較快但有限制；`iret` 是萬用的備案 |
| 200 | **`POP_REGS`**：把 kernel stack 上的一般暫存器**全部彈回來** | 步驟 6 的**反向** |
| 203–204 註解 | 「現在除了 **RSP 和 RDI** 之外，所有暫存器都還原了」 | stack pointer 永遠是最後處理的 |
| 207 | 換到一個「跳板 stack（trampoline stack）」 | 為了 KPTI（第 12 講補充）：等一下要換 CR3，kernel stack 在使用者 page table 裡看不到 |
| 219 | **`SWITCH_TO_USER_CR3_STACK`** | 換回使用者用的 page table（KPTI） |
| 222 | **`popq %rsp`**：**`rsp` 變成使用者 stack** | 從這一刻起 kernel stack 就不能用了 |
| 225 | **`swapgs`** | 把 `gs` 換回使用者的（步驟 3 的反向） |
| **226** | **`sysretq`** | `rcx` → `rip`、`r11` → `rflags`、切回 ring 3——**回到使用者程式** |

標籤名字 `entry_SYSRETQ_unsafe_stack` 也在說同一件事：從這裡開始 stack 是「不安全」的（已經是使用者的 stack）。

---

## 3. 額外的 context：`thread_struct`

`[13:31]` 還有一些**額外的 context** 存在 **`thread_struct`** 裡：

- **thread local storage 區域**的清單。
- 某些 kernel 實作裡有 stack pointer。
- segment 暫存器的值、I/O 狀態等。

`[14:03]` 「這個**依架構而定**，不是通用的。」但不管是什麼，切回使用者程式前全部都要還原。

【課本補充】為什麼需要 `thread_struct`？課本說（p.107）：kernel stack 大小有限，**放不下太多資訊**，所以定義一個專門的 `thread_struct` 存所有額外和雜項資訊（`arch/x86/include/asm/processor.h`）：TLS 區域的清單（每個的起點和大小）、stack pointer（選用）、segment 暫存器（`ds`、`es`、`fs`、`gs`）、I/O 權限、浮點單元的狀態。

```c
// arch/x86/include/asm/processor.h:414-432（節錄）
struct thread_struct {
	/* Cached TLS descriptors: */
	struct desc_struct	tls_array[GDT_ENTRY_TLS_ENTRIES];
	...
	unsigned long		sp;
	...
	unsigned short		es;
	unsigned short		ds;
	unsigned short		fsindex;
	unsigned short		gsindex;
	...
	unsigned long		fsbase;
	unsigned long		gsbase;
```

| 欄位 | 對照課本 |
|---|---|
| `tls_array[]` | **TLS 區域的清單**——每個是一個 GDT 描述子（第 6 講的 `desc_struct`） |
| **`sp`** | **stack pointer**——這個很重要：§5.4 換 stack 時，舊 task 的 kernel stack pointer 就存在這裡 |
| `es`、`ds`、`fsindex`、`gsindex` | **segment 暫存器** |
| `fsbase`、`gsbase` | `fs`、`gs` 的 **base**（第 6 講：64 位元用 MSR / `wrfsbase` 設定的那個值） |

`thread_struct` 嵌在 `task_struct` 裡（`task_struct` 的最後一個欄位 `thread`），每個 task 一份。

---

## 4. 進了 kernel 之後：kernel 是機會主義者

`[14:31]` context 存好之後，進到 kernel 會怎樣？

`[18:31]` 已經討論過：從 user 到 kernel **不用另開 kernel thread**，同一個 thread 在 kernel mode 繼續，處理中斷也是同一個 thread。

`[19:01]` 【口頭】但是：

> 「你發了 system call 或中斷，進到 kernel——**kernel 是個機會主義的客人（opportunistic customer）**。它沒理由要**回報你**。事實是你已經進來了，kernel 現在會看看**它能做的最好的事**是什麼。它有很多選擇：
> - 做一些它自己**擱著的工作**；
> - 開始一個**不同的 kernel thread**；
> - 開始一個**不同的使用者 process**；
> - **恢復同一個**使用者 process；
> - 開始**同一個 thread group 的另一個 thread**。
>
> 它想做什麼都可以，因為它是 kernel。你進了 kernel，就**任由它擺布**了。」

### 4.1 `exit_to_user_mode_loop`

`[20:01]` kernel 先看看有沒有**優先權更高**的額外工作：

1. 呼叫 **`exit_to_user_mode_loop`**。
2. 如果除了恢復使用者 process 之外**還有別的工作**，就呼叫 **`schedule`**——排程器在這裡跑，它的工作是找出**下一個要執行的 task**。
3. 找到另一個 task → 呼叫 **`context_switch`**。可能是恢復同一個 process，也可能是新的。

`[20:32]` 「如上一講看到的，新的有**代價**：不同的使用者 process（不在同一個 thread group），要清 TLB、載入新的 page table——不理想。但 kernel 可能會說：『某個 process **一直在跑**，需要**公平**——系統裡公平也很重要，不能一直跑同一個 process。』」

`[21:01]` **run queue**：一個資料結構。

> 【口頭】「Linux 原始碼和我們的課本都寫成 **runqueue**，run 和 queue 中間**沒有空格**。我在投影片上加了空格，因為你們第一次看到，要知道它是什麼。之後的投影片就不加空格了。」

（第 7 講：run queue 存所有 `TASK_RUNNING` 的 task。）

### 4.2 【原始碼】回使用者空間前檢查 `TIF_NEED_RESCHED`

```c
// kernel/entry/common.c:147-187（節錄）
static unsigned long exit_to_user_mode_loop(struct pt_regs *regs,
					    unsigned long ti_work)
{
	/*
	 * Before returning to user space ensure that all pending work
	 * items have been completed.
	 */
	while (ti_work & EXIT_TO_USER_MODE_WORK) {

		local_irq_enable_exit_to_user(ti_work);

		if (ti_work & _TIF_NEED_RESCHED)
			schedule();

		if (ti_work & _TIF_UPROBE)
			uprobe_notify_resume(regs);

		if (ti_work & _TIF_PATCH_PENDING)
			klp_update_patch_state(current);

		if (ti_work & (_TIF_SIGPENDING | _TIF_NOTIFY_SIGNAL))
			arch_do_signal_or_restart(regs);

		if (ti_work & _TIF_NOTIFY_RESUME)
			resume_user_mode_work(regs);
		...
		/*
		 * Disable interrupts and reevaluate the work flags as they
		 * might have changed while interrupts and preemption was
		 * enabled above.
		 */
		local_irq_disable_exit_to_user();
		...
		ti_work = read_thread_flags();
	}
```

| 行 | 意思 | 對照 |
|---|---|---|
| 150–153 註解 | 「**回使用者空間之前**，確保所有擱著的工作都做完」 | 教授說的「kernel 是機會主義者」 |
| 154 | `while (ti_work & EXIT_TO_USER_MODE_WORK)` | `ti_work` 就是第 7 講 **`thread_info->flags`** 那個位元向量。只要還有任何一個「工作」位元是 1 就繼續 |
| **158–159** | **`if (ti_work & _TIF_NEED_RESCHED) schedule();`** | 第 7 講的 **`TIF_NEED_RESCHED`**（第 3 位元）：timer interrupt 時排程器覺得該換人，就設這個位元（第 3 講 `update_process_times`）。**在這裡才真的去排程** |
| **167–168** | **`_TIF_SIGPENDING`** → 處理 **signal** | 第 7 講的 `TIF_SIGPENDING`（第 2 位元）：有 signal 要送給這個 process——**回使用者空間前**送（第 17 講） |
| 181–186 | 關中斷、**重新讀一次旗標**，再判斷要不要繼續迴圈 | 上面打開中斷做事時，可能又有新工作進來了 |

這段程式碼把前幾講串起來了：**timer interrupt（第 3 講）設 `TIF_NEED_RESCHED`（第 7 講）→ 回使用者空間前在這裡檢查 → `schedule()` → `context_switch()`（第 12 講）**。

---

## 5. `context_switch()` 的五個步驟

`[21:30]` `context_switch(runqueue, prev, next)`：

| 參數 | 意思 |
|---|---|
| runqueue | 所有準備好的 process |
| **prev** | **原本在跑**的 task |
| **next** | 排程器**選來要跑**的 task |

（課本 §3.3.4）

| # | 步驟 | 做什麼 |
|---|---|---|
| 1 | **`prepare_task_switch`** | 準備：如果 prev 和 next 不一樣，告訴 prev「你不在跑了」（狀態還可以是 `TASK_RUNNING`，只是不在 CPU 上） |
| 2 | **`arch_start_context_switch`** | 初始化架構狀態（如果需要）。x86 目前只做基本檢查 |
| 3 | **切換記憶體結構** | `mm`、`active_mm`、page table、TLB（§5.2） |
| 4 | **`switch_to`** | 切換**暫存器狀態和 stack**（§5.3） |
| 5 | **`finish_task_switch`** | 收尾（§5.7） |

（第 12 講 §4.4 已經看過 `context_switch()` 的原始碼，第 5250、5257、5266–5292、5299、5302 行就是這五步。）

### 5.1 跨架構的「超集合」

`[22:31]` 為什麼有一個 x86 不太用的 `arch_start_context_switch`？

> 【口頭】「從 kernel 的角度，**同一組函式要能在每一種架構上運作**。今天 Linux 什麼都能跑。事實上就算我在做**新硬體**，客戶唯一會問的就是：『**它能開 Linux 嗎？**』那是我被評價的**唯一**標準。你問任何做新 RISC-V 硬體的人，你還沒開口，他或她就會告訴你：『**我們能開 Linux**。』那就像一張證書、權威的印章，代表這顆處理器完成了。」

所以 kernel 會比較**寬鬆**地定義很多函式，形成一個**超集合（superset）**，但不是每個架構都會用到每個函式。

### 5.2 切換記憶體結構：`mm` 與 `active_mm`

`[24:30]` 「從我們的角度，**最重要的**是切換記憶體結構。」

每個 `task_struct`——不管是使用者 process、kernel process 還是 kernel thread——都有一個 **`mm`** 欄位（`struct mm_struct *`）。

> 【口頭】`[25:35]`「`mm_struct` 永遠是**指標**。C 裡結構裡面不會直接放另一個結構，一定是放**指標**——這是 C 的常態。kernel 程式碼要**減少重複、最大化效率**，所以永遠用指標。」

第 9 講說「`mm` 和 `active_mm` 之後再講」——「**那個之後就是這裡**，第 88 張投影片。」

| task 類型 | `mm` | `active_mm` |
|---|---|---|
| **使用者 process** | 自己的 `mm_struct` | **跟 `mm` 一樣** |
| **純 kernel thread**（不是暫時升級的使用者 process） | **NULL** | **最後一個執行的使用者 process 的 `mm`** |

`[26:31]` 為什麼純 kernel thread 的 `mm` 是 NULL？

> 「所有 kernel thread **共享虛擬位址空間**，不需要獨立的 `mm_struct`。`mm_struct` 的主要工作是什麼？維護 page table 指標、列出所有虛擬記憶體區域……但那對**整個系統的 kernel 空間都一樣**，所以不用每個 thread 一個。」

`[27:33]` 為什麼還要 `active_mm`？

> 「假設你跑了某個使用者 process，然後中斷、exception 或 system call 進了 kernel。之後**可能輪流跑了好幾個** kernel thread。但你總是想要一個指向**最後一個執行的使用者 process** 的 `active_mm` 的指標——因為也許你**還在處理它的 system call**，也許你需要它位址空間裡的某個東西。」

（教授先說「在目前這顆 CPU 上」，又改口說「在目前這條執行路線上：一個 kernel thread 跑完、跑排程、跑別的、再跑別的……」。）

【課本補充】課本說（p.108）：有兩種 kernel thread——一種是 system call 或中斷後**暫時轉成** kernel thread 的使用者 thread，另一種是跟使用者 thread **無關**的純 kernel thread。保持 `active_mm` 是因為：就算純 kernel thread 在跑，也要能參考上一個在這顆 CPU 上跑的使用者 process。需要存取那個使用者 process 的記憶體時，可以做到——**它的映射還活在 TLB 裡**。「這是提升效能的措施。」

**四種情況**（教授逐行解釋第 12 講看過的那段程式碼，課本 Listing 3.11）：

`[28:31]` **切到 kernel thread**（`next->mm` 是 NULL）：

- `next->active_mm = prev->active_mm`：把「**最後一個使用者 process**」這個資訊**傳下去**。「這是一個要一直保持的**不變量（invariant）**。」
- 從哪裡來的？
  - **從使用者 process 來**（`prev->mm` 不是 NULL）→ **引用計數 +1**（`mmgrab`）：「使用者 process 本來就關心它的 `mm`，現在 kernel thread **也關心**了。好處是之後如果決定殺掉那個使用者 process，你知道還有幾個 process 持有指向它的指標，計數不是 0 就不能釋放。」
  - **從純 kernel thread 來** → 把 prev 的 `active_mm` 設成 **NULL**：「它工作做完了，沒必要再持有任何狀態，**清理**一下。」

`[31:00]` **切回使用者空間**（「簡單多了」）：

- **恢復同一個 process** → 什麼都不用做。
- **不同的使用者 process**（不在同一個 thread group）→ 載入新 page table、清 TLB。
- 如果 prev 是純 kernel thread → 把它的 `active_mm` 設成 NULL。

【課本補充】課本補充：比較 `prev->active_mm` 和 `next->mm`。**一樣**的話，代表最後在這顆 CPU 上跑的使用者 process 又要跑了——中間可能跑過很多 kernel thread，但最後**同一個使用者 process 回來了**。它的 page table 和 TLB 狀態都保留著，**不用清 TLB**，效能大幅提升。

### 5.3 切換暫存器和 stack：`switch_to`

`[36:31]` **`switch_to`** 做的事：

- 取出 **`thread_struct`**，裡面有額外的 context（TLS、segment 暫存器……），把它們切換。
- 切換 **`current` 指標**（指向目前 task）。
- 切換其他：**浮點狀態、ALU 狀態、一般暫存器、MSR**……
- 「依照程式邏輯、依照我們看過的順序：存 context 時一個順序，還原時**完全相反**。」

【課本補充】課本說的順序（p.109）：

1. 先取出 `thread_struct` 裡的資訊還原——「對執行**不是很關鍵**，可以先還原」。
2. 再還原 thread local state 和 segment 暫存器（code segment 以外）。
3. 最後還原 **`current`** 指標、幾個暫存器和 **stack pointer**。

### 5.4 【原始碼】`__switch_to_asm`：真正換 stack 的兩行

`switch_to` 巨集在 x86-64 上會呼叫這段組合語言：

```asm
// arch/x86/entry/entry_64.S:232-276
/*
 * %rdi: prev task
 * %rsi: next task
 */
.pushsection .text, "ax"
SYM_FUNC_START(__switch_to_asm)
	/*
	 * Save callee-saved registers
	 * This must match the order in inactive_task_frame
	 */
	pushq	%rbp
	pushq	%rbx
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15

	/* switch stack */
	movq	%rsp, TASK_threadsp(%rdi)
	movq	TASK_threadsp(%rsi), %rsp
	...
	/* restore callee-saved registers */
	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%rbx
	popq	%rbp

	jmp	__switch_to
SYM_FUNC_END(__switch_to_asm)
```

| 行 | 意思 |
|---|---|
| 233–234 | `rdi` = **prev**、`rsi` = **next**（第 2 講：C 函式的第 1、2 個參數） |
| 242–247 | 把 **callee-saved** 暫存器（`rbp`、`rbx`、`r12`–`r15`）推進 **prev 的 kernel stack** |
| **250** | **`movq %rsp, TASK_threadsp(%rdi)`**：把目前的 `rsp` 存進 **`prev->thread.sp`**（§3 的 `thread_struct.sp`） |
| **251** | **`movq TASK_threadsp(%rsi), %rsp`**：把 **`next->thread.sp`** 載入 `rsp`——**現在用的是 next 的 kernel stack 了** |
| 268–273 | 從 **next 的 kernel stack** 彈出 callee-saved 暫存器——那是 next **上次被換出去時**推進去的 |
| 275 | 跳到 C 函式 `__switch_to`（§5.5） |

**整個 context switch 的魔法，就在第 250–251 行這兩條 `movq`**：

```
  prev 的 kernel stack              next 的 kernel stack
  ┌──────────────────┐            ┌──────────────────┐
  │ ...              │            │ ...              │
  │ prev 的 r15…rbp   │ ← rsp 原本   │ next 的 r15…rbp   │ ← next->thread.sp
  └──────────────────┘   在這      └──────────────────┘   （上次存的）
         │                                 ▲
         │ 250: 存進 prev->thread.sp         │ 251: rsp 改指向這裡
         ▼                                 │
```

為什麼只存 6 個 callee-saved 暫存器？

- `__switch_to_asm` 是**從 C 函式（`context_switch`）呼叫**的。
- 依照 C 的呼叫慣例，`rax`、`rcx`、`rdx` 等 **caller-saved** 暫存器，呼叫者本來就假設會被蓋掉，不用存。
- **使用者程式**的所有暫存器呢？早在**進 kernel 時**（§1.3 步驟 6）就存在 kernel stack 底部的 **`pt_regs`** 裡了。這裡存的只是「**kernel 執行到一半**」的狀態。

所以這個 task 下次被換回來時：`rsp` 指回它的 kernel stack → 彈出 callee-saved → 從 `__switch_to` 返回 → 從 `context_switch` 返回 → 從 `schedule` 返回 → 回到 `exit_to_user_mode_loop`（§4.2）→ 最後 `sysret` / `iret`（§2）回使用者空間。**它根本不知道自己被換出去過。**

### 5.5 【原始碼】`__switch_to`：其他的 context

```c
// arch/x86/kernel/process_64.c:557-628（節錄）
__visible __notrace_funcgraph struct task_struct *
__switch_to(struct task_struct *prev_p, struct task_struct *next_p)
{
	struct thread_struct *prev = &prev_p->thread;
	struct thread_struct *next = &next_p->thread;
	...
	if (!test_thread_flag(TIF_NEED_FPU_LOAD))
		switch_fpu_prepare(prev_fpu, cpu);

	/* We must save %fs and %gs before load_TLS() because
	 * %fs and %gs may be cleared by load_TLS().
	 ...
	 */
	save_fsgs(prev_p);

	/*
	 * Load TLS before restoring any segments so that segment loads
	 * reference the correct GDT entries.
	 */
	load_TLS(next, cpu);
	...
	 * Note that we don't need to do anything for CS and SS, as
	 * those are saved and restored as part of pt_regs.
	 */
	savesegment(es, prev->es);
	if (unlikely(next->es | prev->es))
		loadsegment(es, next->es);

	savesegment(ds, prev->ds);
	if (unlikely(next->ds | prev->ds))
		loadsegment(ds, next->ds);

	x86_fsgsbase_load(prev, next);
	...
	/*
	 * Switch the PDA and FPU contexts.
	 */
	raw_cpu_write(pcpu_hot.current_task, next_p);
	raw_cpu_write(pcpu_hot.top_of_stack, task_top_of_stack(next_p));

	switch_fpu_finish();

	/* Reload sp0. */
	update_task_stack(next_p);
```

| 行 | 做什麼 | 對照課本的順序 |
|---|---|---|
| 560–561 | 取出 prev 和 next 的 **`thread_struct`** | ① `thread_struct` |
| 568–569 | 準備切換**浮點**狀態（FPU） | |
| 571–576 | **先存** prev 的 `fs`、`gs`——註解：「必須在 `load_TLS()` **之前**，因為 `load_TLS()` 可能會清掉它們」 | **順序很重要**的例子 |
| 578–582 | 載入 next 的 **TLS** 描述子到 GDT——註解：「在還原任何 segment **之前**載入，segment 才會參考到正確的 GDT entry」 | ② TLS |
| 602–603 註解 | 「**CS 和 SS 不用處理**，它們是 `pt_regs` 的一部分」 | §1.3：進 kernel 時已經存了 |
| 605–611 | 存 prev 的 `es`、`ds`，載入 next 的。注意 **`unlikely`**：兩個都是 0 就跳過（x86-64 上通常都是 0，第 6 講：扁平模型） | ② segment 暫存器 |
| 613 | 載入 next 的 `fs`、`gs` **base** | ② |
| **620** | **`raw_cpu_write(pcpu_hot.current_task, next_p)`**：**切換 `current`**——第 6、8 講看過的那個用 `gs` 讀的 per-CPU 變數 | ③ **current 指標** |
| 621 | 更新 per-CPU 的「目前 kernel stack 頂端」 | ③ |
| **626** | **`update_task_stack(next_p)`**：註解「**Reload sp0**」——把 next 的 kernel stack 頂端寫進 **TSS 的 `sp0`** | 下次 next 從使用者空間進 kernel 時，硬體 / `entry_SYSCALL_64` 就知道要用**哪一塊** kernel stack（第 8 講 TSS 的 `sp0`） |

從第 571–582 行的註解可以看到教授說的「**任何兩步都不能對調**」不是誇張——kernel 開發者特地寫註解解釋**為什麼這一步要在那一步之前**。

### 5.6 【原始碼】新 process 第一次跑：`ret_from_fork`

一個問題：剛 `fork` 出來的子 process（第 11 講），**從來沒有被換出去過**，它的 kernel stack 上沒有 `__switch_to_asm` 存的東西，要怎麼「被換回來」？

答案：`copy_thread()` 時**偽造**一個看起來像是剛被換出去的 stack，讓它「返回」到一個特別的地方：

```asm
// arch/x86/entry/entry_64.S:279-300（節錄）
/*
 * A newly forked process directly context switches into this address.
 *
 * rax: prev task we switched from
 * rbx: kernel thread func (NULL for user thread)
 * r12: kernel thread arg
 */
.pushsection .text, "ax"
	__FUNC_ALIGN
SYM_CODE_START_NOALIGN(ret_from_fork)
	...
	movq	%rax, %rdi
	call	schedule_tail			/* rdi: 'prev' task parameter */

	testq	%rbx, %rbx			/* from kernel_thread? */
	jnz	1f				/* kernel threads are uncommon */
```

| 部分 | 意思 |
|---|---|
| 280 註解 | 「**新 fork 出來的 process 直接 context switch 到這個位址**」 |
| `schedule_tail` | 做 `finish_task_switch` 的收尾工作（§5.7）——一般 task 是從 `context_switch` 返回時做，新 task 沒有那段返回路徑，所以在這裡補做 |
| `testq %rbx, %rbx` | `rbx` 是 kernel thread 要執行的函式：**不是 NULL → 是 kernel thread**，跳去執行那個函式；**是 NULL → 是使用者 process**，繼續往下走到回使用者空間的路徑 |
| 註解 `kernel threads are uncommon` | 一個小小的**分支預測**提示：kernel thread 比較少見 |

使用者 process 繼續往下，就會走到 `sysret` / `iret` 的回程，從 `pt_regs` 還原暫存器——那個 `pt_regs` 就是第 11 講 `copy_thread()` 從父 process 複製、並把 `ax` 改成 0 的那份。**子 process「從 `fork` 返回、拿到 0」就是這樣接上的。**

### 5.7 收尾：`finish_task_switch` 與 kmap

`[37:01]` **`finish_task_switch`**：

- 把 prev 的狀態設成「**不在跑**」，next 設成「**在跑**」。
- 更新兩個 task 的**時間資訊**（排程器會用）。

`[37:30]` 有時候：

> 「kernel 的**實體**大小比分配給它的虛擬記憶體空間**還大**。64 位元系統不會發生，但較小的系統會——32 位元、16 位元。32 位元系統上 kernel 只有 **1 GB**，但可能實際上有更多記憶體。」

這時就用 **kmap**：

- 在虛擬位址空間裡留一塊，暫時把一些 page **映射**進來（有點像 kernel 層級的 swap）。
- 用完就 **kunmap**，**解除映射**。
- 「離開前做 kunmap，基本上就是解除映射、然後走人。」

【課本補充】課本說（p.109）：這些 page 通常在這個函式（`finish_task_switch`）裡、回到使用者 process 之前被解除映射。

`[38:31]` 最後，**硬體層級的還原**，然後出去。「**終於可以開始新 task 了！**」（課本：「設好其他 flags、暫存器、code segment 暫存器，**最後是指令指標**。」）

---

## 6. C 的可見性：底線和 `static`

`[32:01]` 教授看到函式名字前面的 `__`（例如 `__switch_to`），岔題講了 **C 的可見性**：

`[32:32]` C 的變數和函式名字**可以有底線**，甚至可以**用底線開頭**。有底線**不會**給它任何特殊地位。但有一個**慣例**：

`[33:02]` 大型程式有很多變數、函式：

> 「**全域變數一般是壞主意**。除非某個東西**真的是全域的**——例如 OS 的名字，大家都該看到——不然不要用全域變數。真正全域的東西很少；如果是唯讀的，就用 `#define`。」

（教授先說是巨集，又更正：`#define` 嚴格說是**前置處理器符號**。）

`[33:32]` **函式**也是全域的：全世界都看得到——「這**也不是好主意**，大部分函式不需要那種可見性。」

`[34:02]` 如果是**物件導向**語言，有修飾字：

| 修飾字 | 看得到的範圍 |
|---|---|
| `private` | 只有**自己的 class** 裡的變數和函式 |
| `protected` | 自己的 class 和**衍生 class** |
| `public` | **全世界** |

`[34:31]` 而且**變數本身一般不該是 public**：

> 「每個變數都要用 **getter 和 setter** 方法保護。變數叫 `x`，getter 就叫 `get_x`、setter 叫 `set_x`，`x` 本身是 private 或 protected。因為你**不想讓變數被直接存取**——很難追蹤誰在存取。函式可以是 **inline** 的，編譯器會最佳化掉。但至少你知道誰在存取它。之後如果你想**改變數的單位**或做任何事，都很容易。所以在大型多檔專案裡，**用函式包住每個變數**、**限制存取、不要讓東西全域**，是正確的做法。」

`[35:31]` 但 C **沒有這些**。C 有的是：

> **把函式宣告成 `static`，它的可見性就只限於定義它的那個 C 檔**，其他 C 檔看不到。

`[36:00]` **Linux kernel 的慣例**：

> 「如果函式用**底線開頭**，你會看到它**幾乎一定**被定義成 `static`。底線函式是**這個 C 檔內部用的**，外面看不到。當然加底線**不保證**這件事，但底線開頭的名字表示『**這是內部的**』，而且也會寫 `static` 確保外面看不到——**連結器**不會理它。」

`[36:30]` 「**可見性是軟體設計很重要的一面**。」

> 補充（不是教授講的）：這個慣例是「大致如此」。例如本講看到的 **`__switch_to`**（§5.5）就**不是** `static`——它前面是 `__visible`，因為它要被**另一個檔案**（`entry_64.S` 的 `jmp __switch_to`）呼叫。而 **`context_switch`**（第 12 講）沒有底線，卻是 **`static __always_inline`**。底線在 kernel 裡更常見的意思是「**較低階、少了一些檢查或鎖的版本**，呼叫者要自己負責」（例如有 `foo()` 也有 `__foo()`）。

---

## 7. 兩個小知識

`[39:02]` 【口頭】「看到這個**漢堡圖示**，就是一份很好的『**思考的食物（food for thought）**』。」

### 7.1 `likely` / `unlikely`

`[39:02]` kernel 程式碼裡常看到：

```c
if (likely(某個條件)) { ... }
if (unlikely(某個條件)) { ... }
```

- 這是給**硬體分支預測器（branch predictor）**的**提示**：「這個分支**很可能**會走」或「**不太可能**走」。
- 「程式設計師透過**編譯器層級的提示**，提高分支預測器的效率。」
- 現在寫成巨集，會被翻成提供編譯器提示的東西。
- `[40:02]` 「分支預測器可能沒那麼聰明，但你身為程式設計師**有這個智慧**，何不告訴它？這是 kernel 程式碼**刻意提升效率**的一種方式。」

【原始碼】

```c
// include/linux/compiler.h:77-78
# define likely(x)	__builtin_expect(!!(x), 1)
# define unlikely(x)	__builtin_expect(!!(x), 0)
```

| 部分 | 意思 |
|---|---|
| `__builtin_expect(值, 預期)` | **GCC 內建函式**：告訴編譯器「這個值**預期**是多少」 |
| `!!(x)` | 兩次 NOT：把任何非 0 值變成 **1**，0 還是 **0**——讓它可以跟 1 / 0 比較 |
| 效果 | 編譯器會把**比較可能走的那條路**排在**緊接在後面**（不用跳），不太可能的那條放到遠處 |

> 補充（不是教授講的）：現代 x86 CPU 其實**沒有**讓程式碼直接告訴分支預測器的指令（老 Pentium 4 有過，後來被忽略了）。`likely` / `unlikely` 實際的效果是**編譯器重新排列程式碼**：常走的路徑排成「直直往下」，讓指令快取和預測器都比較順。所以說它是「**給編譯器**的提示」比較精確。本講就看到很多例子：`__switch_to` 的 `if (unlikely(next->es | prev->es))`、第 2 講 `do_syscall_x64` 的 `if (likely(unr < NR_syscalls))`。

### 7.2 `__latent_entropy`

`[40:02]` 也常看到 `__latent_entropy`，例如第 11 講的：

```c
static __latent_entropy struct task_struct *copy_process(...)
```

`[40:32]` 想法：

> 「kernel 在跑的時候發生很多事：硬體中斷進來……是一個**相當複雜、混亂、不確定**的環境。你配置了很多指標，但下次跑 kernel 時，那些指標的值會**不一樣**——指令順序和很多事會變，記憶體配置器配出來的位址就不同。**沒有兩次執行是一樣的。**
>
> 把這些資料結構指標的值**組合起來**，就產生一個**亂數來源**。它可能不是很強的亂數，但相當強，很多情況下**足夠用在密碼學**上——可以當亂數來源產生**密碼學金鑰**。」

`[41:02]` 「產生密碼學金鑰需要**真亂數產生器**，但除非用**量子現象**之類，很難做出真亂數產生器。這個實務上**相當接近**真亂數——沒有理論保證，當然可能被破解——但很多情況下，光是從 kernel 抽出這些資訊，就能做出一個相當好的亂數來源。」

【原始碼】

```c
// include/linux/compiler-gcc.h:45
#define __latent_entropy __attribute__((latent_entropy))
```

```c
// include/linux/compiler_types.h:277
# define __latent_entropy
```

- 有 GCC 外掛時，`__latent_entropy` 變成 `__attribute__((latent_entropy))`：一個 **GCC 外掛**（latent_entropy plugin）會在這個函式裡**插入額外的程式碼**，把執行過程中的一些值混進 kernel 的亂數池。
- 沒有那個外掛時（第二個定義），它就是**空的**，什麼都不做。

> 補充（不是教授講的）：課本說「用 `task_struct*` 指標的值當亂數來源」。精確地說，這個外掛是在編譯時，在被標記的函式裡加入一些「依執行路徑和中間值變化」的運算，結果混進一個全域的熵變數。概念跟教授說的一樣：**利用執行時的不確定性**收集熵。

---

## 8. 第 3 章總結

`[42:00]` 教授回到第 3 章的路線圖：

| 主題 | 內容 |
|---|---|
| **process 的概念** | |
| **`task_struct`** | |
| **process 的建立與銷毀** | fork、exec |
| **process 切換** | TLB 清除、page table；**存什麼、怎麼存**——最重要的是**指令指標、flags、stack pointer**（中斷可能換 code segment 時加上 code segment 暫存器）；system call 是計畫好的、中斷是沒計畫的，存法和還原法不同 |

`[43:31]` 下一章：**溝通機制**——「我們已經建好 process，但它們還沒在溝通。」四種機制：

- 進 kernel 的方法：**system call、exception、interrupt**
- kernel 傳訊息給使用者 process 的方法：**signal**

【課本補充】課本的第 3 章摘要（Summary 3.4.1，重點整理）：

| # | 摘要 |
|---|---|
| 1 | process 是程式的執行實例 |
| 2 | `struct task_struct` 存所有 process 相關資訊 |
| 3 | 指向目前 `task_struct` 的指標由 `current` 巨集提供，存在 `gs` 指向的 per-CPU 區域 |
| 4 | task 狀態：`TASK_RUNNING`、`TASK_ZOMBIE`、`TASK_STOPPED`、`TASK_INTERRUPTIBLE`、`TASK_UNINTERRUPTIBLE`（後兩個是阻塞狀態） |
| 5 | task 結束 → `exit` → zombie → 送 `SIGCHLD` 給父 → 父 `wait` 收屍 → 子的狀態完全清除 |
| 6 | 每個 task 有一個 kernel stack，限制 8 KB（x86-64 v6.2.12 實際 16 KB，第 8 講） |
| 7 | 140 個優先權：100 個即時（0 到 99 遞增）、40 個使用者（100–139）；預設 120，可用 `nice`（−20 到 19）改 |
| 8 | `mm_struct` 存 VMA 和 page table；VMA 用 maple tree（B+ 樹）存 |
| 9 | 每個 process 有 PID，在它的 namespace 裡唯一；namespace 可以暫停、checkpoint、搬到新機器恢復 |
| 10 | namespace 有階層；process 在所有祖先 namespace 都看得到，各有不同號碼，但都指向同一個 `struct pid` |
| 11 | process 組成樹：使用者 process 的根是 init（PID 1），kernel thread 的根是 kthreadd（PID 2） |
| 12 | 用 fork 建新 process：完整複製父的記憶體和執行狀態 |
| 13 | copy-on-write：寫入共享 page 時給寫入者一份新副本，要更新寫入者的 page table 和 TLB |
| 14 | 子 process 可以用 `exec` 換成另一個執行檔，PC 設到 text section 第一條指令 |
| 15 | 硬體 context：所有暫存器（一般、特權）、next PC、ALU flags——要存起來、之後還原 |
| 16 | 所有 kernel thread 共享 kernel 虛擬位址空間 → 切到 kernel mode 或 kernel thread 之間切換**不用清 TLB、不用換 page table**；只有恢復**不同的**使用者 process 才要 |
| 17 | thread group 裡共享整個虛擬位址空間，不同 thread 的 stack 放在不同位置、不重疊；TLS 也一樣 |
| 18 | 存 context 是**非常精細**的步驟：next PC、flags、segment 暫存器、stack pointer、MSR、一般暫存器各有專門的做法；**還原完全照相反的順序** |

---

## 9. 一頁速查表

### 9.1 存 context（system call 進來）

| 步 | 誰 | 做什麼 |
|---|---|---|
| 1 | 硬體 | `rip` → `rcx`、`rflags` → `r11`（中斷則推到 stack） |
| 2 | 軟體 | `swapgs`：`gs` 換成 kernel 的 |
| 3 | 軟體 | 使用者 `rsp` → **TSS** |
| 4 | 軟體 | `rsp` ← kernel stack |
| 5 | 軟體 | 推進 kernel stack：`ss`、`rsp`（從 TSS）、`r11`、`cs`、`rcx`、其他暫存器 → **`pt_regs`** |
| 全程 | | **關中斷** |

### 9.2 還原 context

| 項目 | 內容 |
|---|---|
| 順序 | 完全相反；**`rip` 最後** |
| 中斷 | **不用關** |
| system call | `POP_REGS` → `rsp` ← 使用者 stack → `swapgs` → **`sysretq`**（`rcx` → `rip`、`r11` → `rflags`） |
| 中斷 | **`iret`**：從 stack 取 `rip`、`cs`、`rflags`（、`rsp`、`ss`） |
| `sysret` 不行時 | 改走 `iret`（例如 `rcx != rip`） |

### 9.3 System call vs 中斷

| | System call | 中斷 |
|---|---|---|
| 性質 | 計畫好的 | 沒計畫的 |
| PC、flags 放哪 | `rcx`、`r11` | stack |
| 返回指令 | `sysret` | `iret` |

### 9.4 `context_switch(rq, prev, next)` 五步

| # | 函式 | 做什麼 |
|---|---|---|
| 1 | `prepare_task_switch` | prev 標成不在跑 |
| 2 | `arch_start_context_switch` | 架構初始化（x86 幾乎沒做） |
| 3 | `mm` / `active_mm` / `switch_mm_irqs_off` | 記憶體結構 |
| 4 | `switch_to` → `__switch_to_asm` → `__switch_to` | 暫存器和 stack |
| 5 | `finish_task_switch` | 狀態、時間、kunmap |

### 9.5 `mm` vs `active_mm`

| task | `mm` | `active_mm` |
|---|---|---|
| 使用者 process | 自己的 | = `mm` |
| 純 kernel thread | NULL | 最後一個使用者 process 的（借用，引用計數 +1） |

### 9.6 `switch_to` 的重點

| 位置 | 做什麼 |
|---|---|
| `__switch_to_asm` 第 250–251 行 | `prev->thread.sp ← rsp`；`rsp ← next->thread.sp`——**換 kernel stack** |
| `__switch_to` | 存 / 載 `fs`、`gs`（順序有講究）、TLS、`es`、`ds`、FPU |
| `__switch_to` 第 620 行 | `current ← next` |
| `__switch_to` 第 626 行 | TSS `sp0 ← next` 的 kernel stack 頂端 |
| 新 process | 「返回」到 `ret_from_fork` |

### 9.7 雜項

| 項目 | 內容 |
|---|---|
| `thread_struct` | kernel stack 放不下的額外 context：TLS、`sp`、segment、`fsbase`/`gsbase`、FPU |
| `exit_to_user_mode_loop` | 回使用者前：`TIF_NEED_RESCHED` → `schedule()`；`TIF_SIGPENDING` → 送 signal |
| kernel 是機會主義者 | 進了 kernel，接下來跑誰由它決定 |
| 底線 + `static` | kernel 慣例：底線開頭 = 內部用，通常 `static`（不絕對） |
| `likely` / `unlikely` | `__builtin_expect`：讓編譯器把常走的路排在前面 |
| `__latent_entropy` | GCC 外掛：從執行過程收集熵給亂數池 |

### 9.8 本講看過的原始碼位置

| 檔案:行號 | 看什麼 |
|---|---|
| `arch/x86/entry/entry_64.S:132-136` | `sysret` 要求 `rcx == rip`，否則走 `iret` |
| `arch/x86/entry/entry_64.S:198-226` | `syscall_return_via_sysret`：`POP_REGS`、`popq %rsp`、`swapgs`、`sysretq` |
| `arch/x86/include/asm/processor.h:414-432` | `struct thread_struct` |
| `kernel/entry/common.c:147-187` | `exit_to_user_mode_loop()` |
| `arch/x86/entry/entry_64.S:232-276` | `__switch_to_asm`：換 kernel stack |
| `arch/x86/kernel/process_64.c:557-628` | `__switch_to()` |
| `arch/x86/entry/entry_64.S:279-300` | `ret_from_fork` |
| `include/linux/compiler.h:77-78` | `likely` / `unlikely` |
| `include/linux/compiler-gcc.h:45` | `__latent_entropy` |

---

## 10. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **進 kernel 的四種機制：system call、exception、interrupt、signal** | **第 14–17 講**：第 4 章 |
| **library 先清空 `rcx`、`r11`、system call 的完整生命週期** | **第 14 講**：library call 與 system call（課本 §4.1） |
| **中斷時硬體把 `rip`、`cs`、`rflags` 推到 stack、`iret`、IST** | **第 15 講**：x86 中斷架構（課本 §4.2） |
| **exception、double fault、NMI** | **第 16 講**：exception、softirq（課本 §4.2.7、4.3） |
| **回使用者空間前處理 `TIF_SIGPENDING`** | **第 17 講**：signal（課本 §4.4） |
| **`schedule()`、run queue、公平性** | **第 24–26 講**：排程（課本 §5.4） |
| **關中斷保護關鍵區段** | **第 22 講**：kernel 裡的並行（課本 §5.3） |
| **kmap、32 位元 kernel 只有 1 GB** | **第 30 講**：kernel 的記憶體架構（課本 §6.2.5） |

---

## 附：本講一句話總結

> **system call 進 kernel 時，硬體先把 PC 和 flags 放進 `rcx`、`r11`（中斷沒計畫，只能推到 stack），軟體再 `swapgs`、把使用者 `rsp` 暫存到 TSS、換成 kernel stack、把其餘暫存器推成 `pt_regs`——全程關中斷；回去時照相反順序、不用關中斷，最後用 `sysret`（或 `iret`）設回 PC。中間 kernel 是機會主義者：回使用者空間前若 `TIF_NEED_RESCHED` 是 1 就 `schedule()` → `context_switch()`，先處理 `mm`/`active_mm`（kernel thread 借用前一個的），再由 `__switch_to_asm` 用兩條 `movq` 把 `rsp` 從 prev 的 kernel stack 換到 next 的——整個 context switch 的魔法就在那兩行。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 13（Chapter 3, Part VII）錄影逐字稿（YouTube 自動字幕，44:16）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§3.3.3–3.4，以及 Linux kernel v6.2.12 原始碼整理。*
