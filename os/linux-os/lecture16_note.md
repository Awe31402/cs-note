# Operating Systems: A Linux Kernel-Oriented Approach 第十六講完整筆記：Exception、softirq、threaded IRQ、work queue（Chapter 4, Part III）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 16（Chapter 4, Part III）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（57:17，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§4.2.7、§4.3（p.138–145）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
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
- [1. Exception](#1-exception)
  - [1.1 x86 的 exception 清單](#11-x86-的-exception-清單)
  - [1.2 【原始碼】`trapnr.h`：完整清單](#12-原始碼trapnrh完整清單)
  - [1.3 【課本補充】breakpoint 怎麼運作](#13-課本補充breakpoint-怎麼運作)
  - [1.4 IDT 入口：宣告與定義](#14-idt-入口宣告與定義)
  - [1.5 `do_error_trap` 的參數](#15-do_error_trap-的參數)
  - [1.6 CR2：page fault 的位址](#16-cr2page-fault-的位址)
  - [1.7 【實測】除以零變成 `SIGFPE`](#17-實測除以零變成-sigfpe)
- [2. Exception handler 可以做的五件事](#2-exception-handler-可以做的五件事)
  - [2.1 送 signal 給 process](#21-送-signal-給-process)
  - [2.2 寫 kernel log](#22-寫-kernel-log)
  - [2.3 Kernel panic](#23-kernel-panic)
  - [2.4 動態二進位轉譯：用軟體補上缺的指令](#24-動態二進位轉譯用軟體補上缺的指令)
  - [2.5 `notify_die`：觀察者模式](#25-notify_die觀察者模式)
  - [2.6 【原始碼】`NOTIFY_*` 的位元設計](#26-原始碼notify_-的位元設計)
- [3. 為什麼需要 bottom half](#3-為什麼需要-bottom-half)
  - [3.1 Top half 不能做的事](#31-top-half-不能做的事)
  - [3.2 為什麼 top half 不能拿鎖](#32-為什麼-top-half-不能拿鎖)
- [4. 四種執行 context](#4-四種執行-context)
- [5. 三種 bottom half 機制](#5-三種-bottom-half-機制)
- [6. Softirq](#6-softirq)
  - [6.1 兩種啟動方式](#61-兩種啟動方式)
  - [6.2 不能無限連鎖：交給 ksoftirqd](#62-不能無限連鎖交給-ksoftirqd)
  - [6.3 ksoftirqd 跑在 process context](#63-ksoftirqd-跑在-process-context)
  - [6.4 怎麼 raise：一個 per-CPU 的位元](#64-怎麼-raise一個-per-cpu-的位元)
  - [6.5 【原始碼】softirq 的種類與 `__do_softirq`](#65-原始碼softirq-的種類與-__do_softirq)
- [7. Threaded IRQ](#7-threaded-irq)
  - [7.1 【原始碼】優先權 50 從哪來](#71-原始碼優先權-50-從哪來)
- [8. Work queue](#8-work-queue)
  - [8.1 `work_struct`：函式指標 + 參數](#81-work_struct函式指標--參數)
  - [8.2 Worker pool](#82-worker-pool)
  - [8.3 Pool workqueue：wrapper 管 active 和 inactive](#83-pool-workqueuewrapper-管-active-和-inactive)
  - [8.4 系統內建的 work queue](#84-系統內建的-work-queue)
  - [8.5 【原始碼】`work_struct`、`worker_pool`、`pool_workqueue`](#85-原始碼work_structworker_poolpool_workqueue)
- [9. 【實測】softirq、ksoftirqd、irq thread、kworker](#9-實測softirqksoftirqdirq-threadkworker)
- [10. 一頁速查表](#10-一頁速查表)
- [11. 與後續講次的連結](#11-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

`[00:34]` 第 4 章第 3 講：

| # | 主題 | 對應課本 |
|---|---|---|
| 1 | **Exception** | §4.2.7 |
| 2 | 處理中斷、exception 和重要 kernel 工作的機制——「幾乎都是**另外的 thread**」 | §4.3 |

第二部分就是上一講結尾說的 **bottom half**：

```
             top half（interrupt context，超高優先權、很短）
                 │
                 │ 把剩下的工作延後
                 ▼
   ┌─────────────┬──────────────────┬────────────────┐
   │  softirq    │   threaded IRQ    │  work queue     │
   │（最高、最受限）│（rtprio 50 的 thread）│（一般 kernel thread，最通用）│
   └─────────────┴──────────────────┴────────────────┘
```

---

## 1. Exception

### 1.1 x86 的 exception 清單

`[00:34]` Intel 處理器通常支援 **24 種** exception。

> 【口頭】「這是現在這版 kernel、我的處理器的情況。我的處理器已經**有點舊**了，四年前買的。但我相信到 2025 年架構大致還是這樣。」

`[01:30]` exception 也叫 **trap（陷阱）**。常見的（課本 Table 4.6）：

| 編號 | 名稱 | 說明 |
|---|---|---|
| **0** | `X86_TRAP_DE` | **除以零**：算術 exception。整數除以零結果是無限大，不允許 |
| 1 | `X86_TRAP_DB` | **除錯**：設了除錯器感興趣的事件，就在那裡停。「之後會看到除錯 exception 有很多種」 |
| **2** | `X86_TRAP_NMI` | **不可遮罩中斷**：不能被忽略 |
| **3** | `X86_TRAP_BP` | **中斷點（breakpoint）**：程式碼裡叫處理器停的地方。PC 一執行到就停、產生 exception |
| 4 | `X86_TRAP_OF` | **溢位**：不該發生的錯誤狀況，所以**明確**被抓 |
| 5 | `X86_TRAP_BR` | 超出範圍（bound range exceeded） |
| 6 | `X86_TRAP_UD` | **無效的 opcode**（指令） |
| 7 | `X86_TRAP_NM` | 裝置不可用 |
| **8** | `X86_TRAP_DF` | **Double fault** |

`[02:30]` **Double fault** 很有趣：

> 「意思是：出了一個 fault（exception），exception handler 開始跑，**然後 handler 又出了 fault**——『fault 中的 fault』。這是**真正**的例外狀況，很多時候代表 **kernel 本身有 bug**。所以最好**關機**。因為有 bug 就可能變成安全漏洞，攻擊者可能鑽進來——所以關機重開好多了。」

（第 8 講：double fault 有自己專用的 IST stack——因為它常發生在 kernel stack 壞掉時。）

### 1.2 【原始碼】`trapnr.h`：完整清單

```c
// arch/x86/include/asm/trapnr.h:7-30
#define X86_TRAP_DE		 0	/* Divide-by-zero */
#define X86_TRAP_DB		 1	/* Debug */
#define X86_TRAP_NMI		 2	/* Non-maskable Interrupt */
#define X86_TRAP_BP		 3	/* Breakpoint */
#define X86_TRAP_OF		 4	/* Overflow */
#define X86_TRAP_BR		 5	/* Bound Range Exceeded */
#define X86_TRAP_UD		 6	/* Invalid Opcode */
#define X86_TRAP_NM		 7	/* Device Not Available */
#define X86_TRAP_DF		 8	/* Double Fault */
#define X86_TRAP_OLD_MF		 9	/* Coprocessor Segment Overrun */
#define X86_TRAP_TS		10	/* Invalid TSS */
#define X86_TRAP_NP		11	/* Segment Not Present */
#define X86_TRAP_SS		12	/* Stack Segment Fault */
#define X86_TRAP_GP		13	/* General Protection Fault */
#define X86_TRAP_PF		14	/* Page Fault */
#define X86_TRAP_SPURIOUS	15	/* Spurious Interrupt */
#define X86_TRAP_MF		16	/* x87 Floating-Point Exception */
#define X86_TRAP_AC		17	/* Alignment Check */
#define X86_TRAP_MC		18	/* Machine Check */
#define X86_TRAP_XF		19	/* SIMD Floating-Point Exception */
#define X86_TRAP_VE		20	/* Virtualization Exception */
#define X86_TRAP_CP		21	/* Control Protection Exception */
#define X86_TRAP_VC		29	/* VMM Communication Exception */
#define X86_TRAP_IRET		32	/* IRET Exception */
```

課本只列到 8，後面幾個重要的：

| 編號 | 名稱 | 跟前幾講的關係 |
|---|---|---|
| 10 | Invalid **TSS** | 第 8、13 講的 TSS 設錯了 |
| 13 | **General Protection Fault**（GPF） | 違反保護規則的「大雜燴」：例如使用者模式執行特權指令（第 2 講）、segment 違規（第 6 講） |
| **14** | **Page Fault** | 第 5 講的 page fault、第 11 講 COW 的「故意製造的 fault」 |
| 16、19 | x87 / SIMD 浮點 exception | 第 3 講的浮點堆疊 |
| 17 | Alignment Check | 存取沒對齊的位址 |
| **18** | **Machine Check**（MCE） | **硬體錯誤**——§2.5 的例子；第 8 講有專用 IST stack |

第 15 講說「向量 0–31 是寫死的 exception」——這張表就是那 32 格的內容。

### 1.3 【課本補充】breakpoint 怎麼運作

課本對 **breakpoint** 補充了很多（p.139）——「這基本上是**使用者加上去**的 exception」：

1. 用 `gdb` 這種除錯器除錯時，想讓程式**停在某一行**。「很遺憾這**需要硬體支援**，純軟體的做法會讓程式**慢很多**。」
2. 編譯時加 **`-g`**：在執行檔裡加入**除錯資訊**——把每一行程式碼對應到 **PC 值**、每個變數對應到**記憶體位址**。這些資訊通常用 **DWARF** 格式存。除錯器把它們讀進內部的 hash table。
3. 設中斷點時，除錯器找到那一行對應的 PC，**告訴硬體**到那裡要停。
4. x86 有專門的**除錯暫存器**（**DR0…DR3** 等，第 2 講的「除錯暫存器」），事先存好這個資訊。
5. 處理器執行到那裡 → 產生 **breakpoint exception** → OS 接住、通知除錯器。
6. 被除錯的程式**暫停**著，除錯器可以看它的暫存器和記憶體。因為編譯時記住了變數和暫存器 / 記憶體位址的對應，就能查到所有區域和全域變數的值。

（OS 怎麼「通知除錯器」？第 10 講的 **ptrace**：被追蹤的 process 停下來、送 `SIGTRAP` 給追蹤者。）

### 1.4 IDT 入口：宣告與定義

`[03:31]` IDT 的**前 24 格是 exception**。

`[04:00]` 兩步：

**① 宣告（declare）**——只是**保留空間**，不填內容：

```c
// arch/x86/include/asm/idtentry.h:548
DECLARE_IDTENTRY(X86_TRAP_DE,		exc_divide_error);
```

「用一個巨集說：這個號碼（0）跟這個 exception 的程式碼關聯起來。」

**② 定義（define）**——`[04:30]` 用另一個巨集**填入**：這個 exception 號碼對應一個函式，執行處理它的程式碼：

```c
// arch/x86/kernel/traps.c:205-209
DEFINE_IDTENTRY(exc_divide_error)
{
	do_error_trap(regs, 0, "divide error", X86_TRAP_DE, SIGFPE,
		      FPE_INTDIV, error_get_trap_addr(regs));
}
```

`[04:30]` 幾乎所有 exception 事件最後都會走到 **`do_error_trap`**——當然是在存好 context 之後。

`[05:01]` 

> 「在入口先存 context，然後跳到 IDT 入口。x86 這裡，特別是中斷和 exception，**軟體填、也由軟體走**。硬體**不會自動呼叫函式**，因為有很多很多子情況，函式也要存 context 再往下走，所以讓**軟體做全部**比較聰明。」

（第 15 講也說：硬體查 IDT 跳到入口後，後面的 `irq_desc`、`irqaction` 都是軟體在走。）

【課本補充】課本說：建立 IDT 入口是**兩階段**的——**宣告**和**定義**。「這是 C 程式裡**標準的設計模式**。」（第 4 講：宣告 vs 定義。）

### 1.5 `do_error_trap` 的參數

`[05:30]` 參數很有趣：

| 參數 | 值 | 意思 |
|---|---|---|
| `regs` | | **所有 CPU 暫存器**的內容（上一講看過，`pt_regs`） |
| 錯誤碼 | `0` | 這裡沒用到（課本） |
| 名字 | `"divide error"` | exception 的文字名稱 |
| 編號 | `X86_TRAP_DE` | trap 的號碼（0） |
| **signal** | **`SIGFPE`** | 要**送給**發生除法錯誤的 process 的 **signal** |
| 子代碼 | **`FPE_INTDIV`** | `[06:31]`「一種子訊息代碼，說是因為**整數除法**」 |
| 位址 | `error_get_trap_addr(regs)` | `[07:04]` **出錯指令的位址**：使用者空間程式的 PC |

`[06:00]` signal 遵守 **POSIX** 標準：

> 「POSIX 很久以前訂的，確保**基本的 Unix 語意**——至少**介面**部分——被大量 OS 遵守。它們內部可以不同，但至少遵守大致的 Unix 介面。Linux **大部分**符合 POSIX，**不是 100%**。但特別是 signal 名稱等等，跟傳統 Unix 有某種程度的相容。」

【課本補充】`SIGFPE` 是 POSIX 對一般**浮點 exception** 的錯誤碼（名字叫 FPE = Floating-Point Exception，但整數除以零也用它）。除法 exception 在除數是 0 **或溢位**時都可能發生。

### 1.6 CR2：page fault 的位址

`[07:04]` 出錯指令的位址也有價值：想知道是**哪條指令**出錯。

`[07:31]` **page fault** 時，還要知道**出錯的資料位址**——拿到那個位址，看它的資料在哪，做適當的記憶體管理。

| 要知道的 | 從哪裡拿 |
|---|---|
| **出錯的指令**位址 | `error_get_trap_addr`（從 `regs` 的 PC） |
| **出錯的資料**位址（page fault） | **CR2 暫存器** |

【原始碼】

```c
// arch/x86/mm/fault.c:1531-1533
DEFINE_IDTENTRY_RAW_ERRORCODE(exc_page_fault)
{
	unsigned long address = read_cr2();
```

page fault handler 的**第一行**就是讀 **CR2**——「硬體自動存在 CR2 裡」（課本：CR2 是一個 MSR）。拿到這個位址，接下來就是第 9 講的 `find_vma()`：這個位址屬於哪個 VMA？

`[08:00]` 這個例子的**預設動作**：送 `SIGFPE` 給 process。如果 process **沒處理**這個 signal，就要做別的預設動作（§2.1）。

### 1.7 【實測】除以零變成 `SIGFPE`

我寫的程式（已在本機執行），註冊一個 `SIGFPE` 的 handler，然後故意除以零：

```c
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

static void handler(int sig, siginfo_t *info, void *ctx)
{
	/* 只能用 async-signal-safe 的函式；這裡為了示範用 printf */
	printf("got signal %d (SIGFPE=%d), si_code=%d (FPE_INTDIV=%d), fault addr=%p\n",
	       sig, SIGFPE, info->si_code, FPE_INTDIV, info->si_addr);
	exit(1);
}

int main(int argc, char **argv)
{
	struct sigaction sa = { .sa_sigaction = handler, .sa_flags = SA_SIGINFO };
	sigaction(SIGFPE, &sa, NULL);

	volatile int zero = argc - 1;     /* 執行時才知道是 0，編譯器擋不掉 */
	printf("about to divide by zero...\n");
	fflush(stdout);
	int x = 10 / zero;                /* x86：div 指令觸發 exception 0（#DE） */
	printf("x = %d\n", x);
	return 0;
}
```

輸出：

```text
about to divide by zero...
got signal 8 (SIGFPE=8), si_code=1 (FPE_INTDIV=1), fault addr=0x561f62ed82eb
```

| 觀察 | 對照 `do_error_trap` 的參數 |
|---|---|
| 收到 signal **8** = **`SIGFPE`** | 第 5 個參數 `SIGFPE` |
| `si_code` = **1** = **`FPE_INTDIV`** | 第 6 個參數 `FPE_INTDIV`：「因為整數除法」 |
| `fault addr` = `0x561f...82eb` | 第 7 個參數 `error_get_trap_addr(regs)`：出錯的 `div` 指令的位址 |
| 「`x = ...`」**沒印** | handler 裡 `exit(1)` 了 |

整條路：`div` 指令 → CPU exception 0 → IDT 第 0 格 → `exc_divide_error` → `do_error_trap` → kernel 送 `SIGFPE` → **我的 handler**（第 17 講的 signal 遞送）。

**沒有 handler 會怎樣？**

```bash
$ ./dz2        # 同樣除以零，但沒註冊 handler
Floating point exception (core dumped)
$ echo $?
136
```

- `SIGFPE` 的**預設動作**是**終止 process 並產生 core dump**。
- exit code **136** = 128 + **8**（shell 的慣例：被 signal N 殺掉就回 128 + N）。
- 這就是教授說的「如果 process 沒處理，就用預設機制——很多情況是**終止 process**」。

---

## 2. Exception handler 可以做的五件事

`[08:00]`（課本 Fig 4.7）

```
                              ┌─► ① 送 signal 給 process
                              ├─► ② 用 printk 寫 kernel log
  DEFINE_IDTENTRY_* ──────────┼─► ③ double fault 等 → kernel panic，停機
                              ├─► ④ 數學錯誤 / 不支援的指令 → 用軟體模擬或忽略
                              └─► ⑤ 用 notify_die 機制
```

### 2.1 送 signal 給 process

`[08:30]` 已經看過一個例子：送 signal 給 process。

- 這**不是**很嚴重的 exception。
- process 可以照自己覺得合適的方式處理這個 signal。
- `[09:00]` process **沒處理**的話，就用**預設機制**——很多情況是**終止 process**。

【課本補充】課本說這是「**最無害**的選項」：只送 signal、kernel 不做其他動作。**除錯**是顯著的例子：處理器偵測到除錯事件（例如 breakpoint）產生 exception，kernel 被通知，送 signal 給除錯的 process。**Watchpoint**（存取某個位址時暫停）也類似。

### 2.2 寫 kernel log

`[09:00]` 另一組不太嚴重的 exception：用 **`printk`** 寫 log。

- `printk` = **print kernel**。「一般的 print 函式在 kernel 裡**不能用**（第 4 講：kernel 不連結標準 C library），但 `printk` 可以寫到 kernel 的 log。」
- **不是互斥的**：可以跟其他選項一起用。
- 「跟 `irqaction` 類似，同一個 exception 可以呼叫**多個** exception handler，做不同的事，其中一個可能就是寫 kernel log。」

【課本補充】這些 log 可以用 **`dmesg`** 指令看，或通常在 **`/var/log/messages`**。很多時候了解 exception 的原因非常重要，特別是在**除錯 kernel 程式碼**時。

### 2.3 Kernel panic

`[09:33]` **double fault**（exception handler 裡又出 exception）：

- 系統本身可以**停下來**。
- kernel 進入 **panic 模式**，整個系統停止或卡住。

【課本補充】課本說：這永遠**不應該發生**，除非 kernel 程式碼有嚴重的 bug。建議的處理方式是**停機、重啟 kernel**，這叫 **kernel panic**（`kernel/panic.c`）。

### 2.4 動態二進位轉譯：用軟體補上缺的指令

`[10:00]` 某種**數學錯誤**：

> 【口頭】「例如某個處理器沒有**開根號**指令，但其他指令都有。程式是為**第 10 版**的處理器編譯的，你卻在**第 9 版**上跑——像是為 i9 編譯、在 i7 上跑。可能有一個指令目前的處理器不支援。但**終止整個程式**好像**太嚴苛**了。」

`[10:31]` 正確做法：**處理這個 exception**：

```
  程式：  sqrt %eax → %ebx     （目前的 CPU 沒有這個指令）
            │
            ▼ 無效 opcode exception（#UD）
  kernel 的 handler：
     1. 發現「這是開根號指令，只是不支援」
     2. 用加減乘除跑小學學的開根號演算法，算出 √eax
     3. 把結果放進 ebx
     4. 從下一條指令繼續執行
            │
            ▼
  程式根本不知道：它的 ebx 裡就是正確的平方根
```

`[11:30]` 「使用者 process **不知道**這件事：值被算出來了，但**不是**硬體指令算的，因為程式在**過時的硬體**上跑。聰明的**軟體在補償**——用一個**軟體補丁（patch）**修補開根號指令。」

（第 2 講也舉過同一個開根號的例子：「不是所有 exception 都會終止程式」。）

【課本補充】課本把這叫 **動態二進位轉譯（dynamic binary translation）**（p.141），用的例子是 **cosine** 指令：

- 處理器第 10 版有 cosine 指令，第 9 版沒有。
- 讓原本的程式碼**不改動地**跑。CPU 遇到不認識的指令 → 產生 exception（非法指令）。
- kernel 的 handler 分析出「這是 cosine 指令，不支援」，用其他現有的指令做數值計算（解 **Maclaurin 級數**），把結果填進目的暫存器。
- 程式從**下一條**指令重新開始。「從正確性的角度沒問題。」
- 當然有**效能代價**，比硬體指令慢很多。「但程式碼現在**完全可以跨機器移植**了。沒有這個機制，整個程式就得被終止。小小的效能代價，是很小的代價。」

### 2.5 `notify_die`：觀察者模式

`[12:01]` 最後一個：**`notify_die` 機制**。

- 跟中斷處理**非常類似**：多個觀察者（或 exception handler）**註冊**自己，然後**一個接一個**被呼叫。
- `[12:32]` 每個事件（例外事件）都有一組對它感興趣的函式：handler 1、handler 2……

`[13:02]` 用 `notify_die` 處理 exception 時，它走一個 **linked list**（所有這些 handler 的串列），依序呼叫。

> 「例如 **machine check exception**（硬體錯誤）：其中一個 handler 想**修正處理器的狀態**，另一個只想用 `printk` 或其他方式**記錄**這個 exception。不同的 handler 做不同的事——跟中斷處理類似，但**更精密一點**。」

`[13:31]` 中斷有 3 種回傳值（`IRQ_NONE`、`IRQ_HANDLED`、`IRQ_WAKE_THREAD`，第 15 講），這裡有 **4 種**（課本 Table 4.7）：

| 回傳值 | 意思 | 繼續走串列？ |
|---|---|---|
| **`NOTIFY_DONE`** | 「我**不在乎**這個事件」 | ✅ 繼續 |
| **`NOTIFY_OK`** | 事件**成功處理**了 | ✅ **繼續**——「這**不是**在阻止你。它說：我處理好了，但你**繼續**沿著串列往下，呼叫其他 handler，讓它們**也**處理」 |
| **`NOTIFY_STOP`** | 處理好了，**沒有別人**該處理 | ❌ **停** |
| **`NOTIFY_BAD`** | **出事了**：handler 本身、或傳給 handler 的環境、或 handler 的程式碼出錯 | ❌ 停——「比較嚴重的情況，不該再呼叫其他函式，要處理這個狀況」 |

`[15:01]` 「這是一個**相當有彈性**的機制。`notify_die` 是 kernel 裡的**通用機制**：從『不在乎』，到『OK，繼續掃』，到『別掃了』，到『別掃了、出事了』。」

```
  exception ──► handler 1 ──► handler 2 ──► handler 3 ──► ...
                  DONE          OK           STOP
                  （繼續）       （繼續）       （停！）
```

【課本補充】課本說這實作了軟體工程經典的**觀察者模式（observer pattern）**。每個註冊興趣的叫 **observer** 或 **listener**。

課本特別點出跟 `irqaction` 的**差別**（p.142）：

- **IRQ**：一個中斷只想**處理一次**（找到發中斷的裝置）。
- **Exception**：**多個** handler 都可以被呼叫，做**不同種類**的處理。「exception handler **不享有**類似的**獨佔感**。」
- 所以 `NOTIFY_OK` 是「我處理了，**但不是獨佔的**」，後面的 handler 還要被呼叫。exception 處理在回 `NOTIFY_STOP` 或 `NOTIFY_BAD` 時才結束。

### 2.6 【原始碼】`NOTIFY_*` 的位元設計

```c
// include/linux/notifier.h:183-191（節錄）
#define NOTIFY_DONE		0x0000		/* Don't care */
#define NOTIFY_OK		0x0001		/* Suits me */
#define NOTIFY_STOP_MASK	0x8000		/* Don't call further */
#define NOTIFY_BAD		(NOTIFY_STOP_MASK|0x0002)
...
#define NOTIFY_STOP		(NOTIFY_OK|NOTIFY_STOP_MASK)
```

| 常數 | 值 | 拆開來看 |
|---|---|---|
| `NOTIFY_DONE` | `0x0000` | 註解：「**不在乎**」 |
| `NOTIFY_OK` | `0x0001` | 註解：「**合我意**（Suits me）」 |
| **`NOTIFY_STOP_MASK`** | **`0x8000`** | 註解：「**別再往下呼叫**」——最高位元 |
| `NOTIFY_BAD` | `0x8002` | `STOP_MASK` \| `0x0002`（錯誤） |
| `NOTIFY_STOP` | `0x8001` | **`NOTIFY_OK` \| `STOP_MASK`**——「OK，而且別往下」 |

**又是位元旗標**（第 7 講的模式）：「要不要停」是**一個獨立的位元**（`0x8000`）。走串列的程式只要檢查 `ret & NOTIFY_STOP_MASK` 就知道要不要停，不用一個一個比較四種值。`NOTIFY_STOP` 就是「`OK` 加上停止位元」，表格上的四種情況從位元結構一眼就看得出來。

---

## 3. 為什麼需要 bottom half

### 3.1 Top half 不能做的事

`[16:01]` 接下來：**三種不同的延後工作機制**（kernel 工作和中斷處理）。

- interrupt handler 有**很多限制**。
- 不想讓它跑很久：它是**非常高優先權**的 thread，自己還會**關中斷**一段時間。
- 所以處理中斷時**只做一點點**——那是 **top half**——**剩下的延後**，那是 **bottom half**。

`[17:00]` top half 不能做的事（以優先權很高為前提）：

| 限制 | 為什麼 |
|---|---|
| **不能存取使用者空間記憶體** | 「不想讓它跟**任何 process** 關聯。interrupt handler 不該像在做某個使用者 process 的 kernel 工作，它就**只處理中斷**」 |
| **不能做阻塞呼叫**（= **不能拿鎖**） | §3.2 |
| **不應該產生中斷** | `[19:00]` 「你是 interrupt handler，你的工作不是產生**更多**中斷。不然會變成**拖很長**的中斷處理序列：一個中斷、它的 handler 跑、決定做某件事，可能寫硬體、或出 exception，又產生另一個中斷、又一個……**不是好行為**」 |
| | `[20:01]` **例外**：**IPI**。IPI 是軟體 thread 產生的中斷，技術上 top half handler 可以產生，雖然不是理想行為 |
| **程式碼要小、執行時間要短** | |
| **不能被任何 kernel thread 搶占** | `[20:30]` 優先權最高，要**跑到完成** |
| **能 mask 的中斷都要關掉** | 少數高優先權的中斷還開著 |

`[20:30]` 被另一個中斷打斷沒關係：

> 「這就是為什麼有**不同的中斷 stack**（第 8 講）。中斷 1 的 handler 在跑，中斷 2 的 handler 也可以跑，沒問題。做完它的 top half，中斷 1 的 top half 恢復、跑到完成。但要注意：**只有 top half handler 在跑**，沒有 kernel thread 在跑——因為所有 top half handler 的優先權都**比系統裡其他所有 thread 都高**，除了別的 top half。」

### 3.2 為什麼 top half 不能拿鎖

`[18:00]` 教授仔細解釋為什麼「不能阻塞 = 不能拿鎖」：

```
  低優先權的 thread A：拿到鎖，正在臨界區（critical section）裡
       │
       │  中斷來了！
       ▼
  高優先權的 top half：想拿同一把鎖
       │  拿不到 → 一直重試（spin）
       │  它優先權最高 → 壟斷 CPU
       │
       ▼
  thread A 永遠沒機會跑 → 永遠不會放鎖 → top half 永遠拿不到
                                            ═══ 系統卡死 ═══
```

> 「它優先權很高，會一直試著拿鎖——進入臨界區。但已經拿到鎖的那個 thread **優先權低得多**，**沒機會跑**。為什麼沒機會？因為 interrupt handler 在跑。它在跑，**卻沒做任何工作**：它一直試著拿鎖，一直失敗，在這過程中**壟斷 CPU**、不讓任何東西跑——最重要的是，**不讓現在拿著鎖的那個 thread 跑**。那個 thread 不跑，就**不能解鎖**——任何拿著鎖的 thread 都要跑完臨界區才能解鎖，也就是說它**需要被給機會**。這正是為什麼不准 top half handler 做阻塞呼叫。」

（第 12 講也講過；第 28 講的**優先權反轉**、另一門課 Real Time Systems 的 PIP / PCP 都是這個問題的延伸。）

---

## 4. 四種執行 context

`[21:31]` 「**context** 是一個隱含的概念，不是明確的，但夠清楚。它基本上像一種**執行模式**，模式是好幾件事的組合。」

（課本 Definition 4.3.1）

| Context | 誰在用 | 特性 |
|---|---|---|
| **User context** | `[22:03]` 一般使用者 process | ring 3；有使用者 process 的優先權 |
| **Process context** | `[22:03]` **一般的 kernel thread** | 有優先權（即時或一般）。可以是**暫時升級**的使用者 process，也可以是**純 kernel thread**。**排程器管**它們。**最低優先權**的 kernel task |
| **Interrupt context** | `[22:30]` **top half handler** | **不准搶占**：跑到完成；能關的中斷都關掉（大部分中斷是關的，不是全部）。限制很多 |
| **Softirq context** | `[23:02]` **softirq** | interrupt context 的延伸：**同樣的限制**，其他 kernel thread 或使用者 process 都不能把它換出去。**唯一的差別：中斷是開的**——連很低優先權的中斷都能打斷 softirq thread、跑完，然後 softirq thread 繼續。但總之它會**在任何一般 kernel thread 或使用者 process 開始之前**跑完 |

`[24:31]` 「interrupt 和 softirq context **在排程器的管轄範圍外**——除非是**即時 kernel**，那是例外，之後再講。」

```
  優先權  高 ┌─────────────────────────────────┐
            │ Interrupt context（top half）   │  中斷關、不可搶占
            ├─────────────────────────────────┤
            │ Softirq context                 │  中斷開，但其他 softirq / thread 不能搶
            ├─────────────────────────────────┤  ← 以上：排程器管不到
            │ Process context（kernel thread） │  排程器管，可以被搶占
            ├─────────────────────────────────┤
         低 │ User context                    │  ring 3
            └─────────────────────────────────┘
```

`[24:02]` 「這四種 context 要記住，之後的討論會**一再出現**。」

【課本補充】課本補充（p.143）：softirq handler 的**實際優先權比較低**：中斷是開的，所以低優先權的 top half interrupt handler 可以搶占它。但 softirq handler **不能被其他 kernel thread 搶占，包括其他在 softirq context 裡跑的 thread**。

---

## 5. 三種 bottom half 機制

`[25:00]` bottom half 的角色很清楚：做**不想在 interrupt context 裡跑**的額外工作。bottom half 本身可以跑在 **softirq context** 或 **process context**。

`[25:32]` Linux 以前有很多種機制，現在都被取代了，剩**三種**：

| 機制 | 跑在哪 | 說明 |
|---|---|---|
| **Softirq** | 如名字所說，**softirq context**——「但有細節，**不總是**這樣，有時候會跑在 **process context**」（§6.3） | 專門的、種類固定 |
| **Threaded IRQ** | **永遠在 process context** | 「目標很清楚：盡量縮短 top half 的時間，因為 interrupt context 是**壟斷**的 context。為了找低優先權的機制，做了 softirq 和 threaded IRQ。但 threaded IRQ 的優先權**還是相當高**，因為至少在精神上它們還是要當 bottom half」 |
| **Work queue** | process context，**一般優先權** | `[26:31]` 「有一個**通用**的方法排程工作：不管 top half、bottom half，就是把一份工作排成延後的函式呼叫。所有 kernel 子系統都能用，包括 device driver 這種**第三方**程式碼」 |

`[26:31]` 所有這些都有**同一組基本參數**：

> **一個函式指標，加上參數。**

「每次建一份要延後執行的工作，需要的就是**指向函式的指標**和**要給那個函式的參數**。我們叫它 **work item** 或 **work parcel**（工作包），意思一樣。」

【課本補充】課本說：softirq 和 threaded IRQ 是**專門**的 bottom half 機制，用來存和執行延後的工作項目，「**不是**用來做其他一般工作的（這是它們的精神）」。work queue 是**通用**的，可以把任何函式當延後的函式呼叫執行，以**一般 kernel thread** 的身分在 kernel 空間跑。

---

## 6. Softirq

### 6.1 兩種啟動方式

`[27:32]` 傳統的 top half interrupt handler 也叫 **hard IRQ**，相對於 bottom half 的 **soft irq**。

> 【口頭】「書裡也提到：**softirq** 永遠用**全小寫**，但 IRQ 單獨用時**全大寫**。這像是標準術語。有時候術語邏輯上不太說得通，但我們沿用慣例。」

（課本 Fig 4.8）

```
   ① 中斷路徑                                 ② 系統管理路徑
   ┌──────────┐                            ┌──────────────────┐
   │ Hard IRQ │                            │ kernel thread     │
   │（top half）│                            │（系統管理工作）     │
   └────┬─────┘                            └────────┬─────────┘
        │ raise softirq request                     │ raise softirq request
        ▼                                           ▼
   local_bh_enable()                        由 kernel daemon ksoftirqd 處理
        │                                           │
        └──────────────► do_softirq ◄───────────────┘
```

**① 中斷路徑** `[28:30]`：

1. top half（hard IRQ）先跑。
2. 需要延後一些工作 → **raise 一個 softirq request**。
3. 呼叫 **`local_bh_enable`**（**bh = bottom half**）→ 開始執行這些 softirq request。

**② 系統管理路徑** `[30:00]`：

- softirq 有**通用**的性質，**其他 kernel 子系統**也能 raise——其他 kernel thread，甚至 device driver（「雖然**不建議**」）。
- 這些 request 存在一個「**資料庫**」裡（所有 pending 的 request）。

### 6.2 不能無限連鎖：交給 ksoftirqd

`[28:30]` 問題：

```
  softirq request 1 執行 → raise request 2 → 馬上執行 → raise request 3 → ...
```

- softirq 可以 raise **另一個** softirq，那個馬上執行，再 raise 一個……
- 這些全都在 **softirq context** 裡執行——「某種意義上**不公平**」。
- `[29:31]` 所以**次數有上限**。超過之後，softirq 就被**延後（deferred）**：不現在執行，晚點再執行。

`[30:32]` 誰來執行被延後的？**`ksoftirqd`**——一個 kernel **daemon**：

> 【口頭】「daemon 就是**定期**跑的 kernel thread。大家說它『在背景跑』——那正式說法沒太大意義。它的意思是：**偶爾**跑一下，基本上就是設一個 timer，偶爾跑。**k** 代表 kernel，**d** 代表 daemon。」

`[31:01]` ksoftirqd 定期掃描那個資料結構，看到有 pending 的 softirq 就執行。

### 6.3 ksoftirqd 跑在 process context

`[31:30]` 【口頭】「請**讀者在這裡稍微更專心一點**。」

| 由誰執行 | 跑在哪個 context |
|---|---|
| top half **之後**馬上執行 | **softirq context** |
| 由 **ksoftirqd** 執行 | 「**很遺憾地**」跑在 **process context**——因為 **ksoftirqd 是一般的 kernel process** |

`[32:00]` ksoftirqd 在**它自己的 context**（process context）裡執行這些 softirq request：

- 一般的**排程器**可以排程它們，也可以**搶占**它們。
- 「它們**享受不到** softirq context 的**商務艙待遇**。」
- 中斷可以來；限制還是一樣；但**最大的差別是可以被搶占**：其他 kernel thread 可以搶占它們。
- `[33:01]` 「在 softirq context 裡某種意義上也可以被搶占，但**只有 top half** 能搶占它，其他 softirq handler 不行。這裡，**排程器覺得合適的任何 kernel thread** 都能搶占——也就是說它跑在**較低優先權**，而且是 process context。」

`[33:32]` 教授從軟體工程角度的評論：

> 「如果 bottom half（這裡是 softirq）**很小**，就在 hard IRQ 之後馬上在 softirq context 跑。但如果**太多**了，就不能在 softirq context 跑，要延後、用 daemon 機制跑。kernel 子系統產生的 softirq 也用 daemon 跑——那時就享受不到 softirq context 的好處，主要因為**這個 context 很貴**。這是你給 top half interrupt handler 的**特權**：核心工作做完後可以多做一點點。但你**不想讓它變成一個一直被使用的通用機制**，所以用 ksoftirqd 就在 process context 跑。」

`[35:01]` 不管怎麼執行（softirq context 或 process context），最後都呼叫 **`do_softirq`**。

`[39:31]` 被延後時，**`TIF_NEED_RESCHED`** 會被設起來，告訴排程器有工作在等。「kernel 裡還有很多類似的情況：工作一被延後，就設 `TIF_NEED_RESCHED`。」

【課本補充】課本補充（p.144）：「softirq context 裡的 softirq task 跑到完成，**中途不能被搶占**。但它在執行時可以 raise 其他 softirq request，**那些 request 在逾時後不一定要執行**，可以延後到之後。」ksoftirqd 也會執行那些「top half 結束後因為**逾時**沒處理完」的 softirq 工作項目。

### 6.4 怎麼 raise：一個 per-CPU 的位元

`[35:31]` raise softirq **不難**：

- 在 **per-CPU 區域**有一個**記憶體 word**。
- 有一組**定義好的 softirq 種類**——**編譯時**就決定了，**不能新增種類**。
- `[36:00]` 種類例如：`HI`（高優先權）、`TIMER`、網路傳送、網路接收、block I/O、排程器、高解析度 timer……
- **每個種類對應 word 裡的一個位元**：這個位元設了 → 這種 softirq 有 request 在等。

| 動作 | 做什麼 |
|---|---|
| **raise softirq** | `[36:32]` 把那種 softirq 對應的位元**設成 1**，把工作放到某處。「就這樣」 |
| **do_softirq** | `[37:01]` 看到位元是 1 → 拿出工作項目、執行 → 做完把 1 改回 0 |

`[37:32]` 結果很清楚：**每種 softirq 最多只能有一個 pending 的工作項目**：

> 「這就是為什麼它**不是通用機制**。以 timer softirq 來說，任何時刻、某顆 CPU 上**只能有一個** timer 工作項目在等，不能多個。……支援的 softirq 種類也有限，所以這個方法的**彈性有疑問**。」

【課本補充】課本補充（p.145）：「與其用一個軟體佇列，有更快的方法記錄這個資訊：在 per-CPU 區域存一個記憶體 word。」softirq 機制「**從來就不是**要當通用機制的，一直以來就是為了**少數定義好的**中斷和 kernel task 卸載延後工作」，也**不是給 device driver 用**的（雖然理論上可以），driver 比較適合用 work queue。處理時，依**已知的優先權順序**呼叫每個設成 1 的位元對應的 handler。

### 6.5 【原始碼】softirq 的種類與 `__do_softirq`

**種類**：

```c
// include/linux/interrupt.h:550-562
	HI_SOFTIRQ=0,
	TIMER_SOFTIRQ,
	NET_TX_SOFTIRQ,
	NET_RX_SOFTIRQ,
	BLOCK_SOFTIRQ,
	IRQ_POLL_SOFTIRQ,
	TASKLET_SOFTIRQ,
	SCHED_SOFTIRQ,
	HRTIMER_SOFTIRQ,
	RCU_SOFTIRQ,    /* Preferable RCU should always be the last softirq */

	NR_SOFTIRQS
};
```

| 位元 | 種類 | 用途 |
|---|---|---|
| 0 | `HI` | 高優先權 |
| 1 | **`TIMER`** | timer 相關 |
| 2 | `NET_TX` | 網路**傳送** |
| 3 | **`NET_RX`** | 網路**接收**——網路卡中斷的 bottom half 主要在這裡 |
| 4 | `BLOCK` | block 裝置（硬碟）的操作 |
| 5 | `IRQ_POLL` | |
| 6 | **`TASKLET`** | 舊的 **tasklet** 機制其實是建在這個 softirq 上的（§7） |
| 7 | **`SCHED`** | 排程器（多核心負載平衡） |
| 8 | `HRTIMER` | 高解析度 timer |
| 9 | **`RCU`** | RCU（第 23 講）。註解：「RCU **最好**永遠是**最後一個** softirq」 |
| — | `NR_SOFTIRQS` = **10** | **只有 10 種**——「彈性有疑問」 |

**raise**：

```c
// kernel/softirq.c:709-714
void __raise_softirq_irqoff(unsigned int nr)
{
	lockdep_assert_irqs_disabled();
	trace_softirq_raise(nr);
	or_softirq_pending(1UL << nr);
}
```

**`or_softirq_pending(1UL << nr)`**：把 per-CPU word 的第 `nr` 個位元 **OR 成 1**——教授說的「把那種 softirq 的位元設成 1，就這樣」，真的就這一行。

**執行**：

```c
// kernel/softirq.c:492-493
#define MAX_SOFTIRQ_TIME  msecs_to_jiffies(2)
#define MAX_SOFTIRQ_RESTART 10
```

```c
// kernel/softirq.c:528-602（節錄）
asmlinkage __visible void __softirq_entry __do_softirq(void)
{
	unsigned long end = jiffies + MAX_SOFTIRQ_TIME;
	...
	int max_restart = MAX_SOFTIRQ_RESTART;
	...
	pending = local_softirq_pending();
	...
restart:
	/* Reset the pending bitmask before enabling irqs */
	set_softirq_pending(0);

	local_irq_enable();

	h = softirq_vec;

	while ((softirq_bit = ffs(pending))) {
		...
		h += softirq_bit - 1;
		...
		h->action(h);
		...
		h++;
		pending >>= softirq_bit;
	}
	...
	local_irq_disable();

	pending = local_softirq_pending();
	if (pending) {
		if (time_before(jiffies, end) && !need_resched() &&
		    --max_restart)
			goto restart;

		wakeup_softirqd();
	}
	...
}
```

| 行 | 意思 | 對照教授說的 |
|---|---|---|
| 492 | **`MAX_SOFTIRQ_TIME`** = **2 毫秒** | 「次數有上限」——其實是**時間**上限 |
| 493 | **`MAX_SOFTIRQ_RESTART`** = **10** | 和**次數**上限 |
| 545 | `pending = local_softirq_pending();`：讀 per-CPU 的那個 word | |
| 553 | **先把 pending 清成 0**，註解：「在打開中斷**之前**重設」 | |
| **555** | **`local_irq_enable();`** | **softirq context：中斷是開的**（§4） |
| **559** | **`while ((softirq_bit = ffs(pending)))`**：用 **`ffs`**（find first set）找第一個是 1 的位元 | 第 10 講的「找第一個 1」——同一招 |
| **571** | **`h->action(h);`**：呼叫那種 softirq 的 handler——**又是函式指標** | 「依已知的優先權順序」：位元 0（`HI`）先、位元 9（`RCU`）最後 |
| 580 | 右移，繼續找下一個 1 | |
| 589 | 處理完，**再讀一次** pending：處理過程中可能**又被 raise** 了新的 | 「softirq 可以 raise 另一個 softirq」 |
| **591–593** | 還有新的、**而且**還沒超過 2 毫秒、**而且**沒有人需要排程、**而且**還沒重來 10 次 → **`goto restart`**，繼續在 softirq context 處理 | 「小的話就馬上在 softirq context 跑」 |
| **595** | 否則 → **`wakeup_softirqd()`**：**交給 ksoftirqd** | 「太多了就延後，用 daemon 跑」 |

```c
// kernel/softirq.c:74-81
static void wakeup_softirqd(void)
{
	/* Interrupts are disabled: no need to stop preemption */
	struct task_struct *tsk = __this_cpu_read(ksoftirqd);

	if (tsk)
		wake_up_process(tsk);
}
```

`__this_cpu_read(ksoftirqd)`：**每顆 CPU 有自己的 ksoftirqd**（`ksoftirqd/0`、`ksoftirqd/1`……，§9 實測）。

---

## 7. Threaded IRQ

`[41:30]` softirq 還是**相當受限**：不是設計來跑很久的、**不能拿鎖**。需要一個**限制較少**的選項：**threaded IRQ**——**獨立的 kernel thread** 執行工作包。

`[41:30]` 以前有一個機制叫 **tasklet**：

> 「它跟 threaded IRQ **不一樣**。很多 device driver **還在用** tasklet，但 threaded IRQ 在使用上**大致取代了** tasklet。」

`[42:00]` threaded IRQ：

- 執行延後的函式，但在 **process context**。
- 是**專門的 kernel thread**，跑在**即時優先權 50**——「**相當高**，不是小的優先權，**絕對**比所有使用者 process 都高」。

`[42:00]` 動機：再看一次 `struct irqaction`（第 15 講）：

```c
struct irqaction {
	...
	struct task_struct  *thread;      /* 執行 thread_fn 的 kernel thread */
	irq_handler_t       thread_fn;    /* 要在那個 thread 裡跑的函式 */
	...
};
```

`[42:31]` 「用一個獨立的 kernel thread 跑這個 `thread_fn`。如果這個 handler 想在獨立的 thread 上跑，它會回 **`IRQ_WAKE_THREAD`**（第 15 講），新 thread 就可以開始執行這個函式。」

`[43:01]` 「某種意義上，**threaded IRQ 就是這個機制**：你指定一個 thread 和一個函式。」

【課本補充】課本補充（p.146）：

- 跑 threaded IRQ 的 kernel thread 優先權還是相當高，但**不像**跑 top half 或 softirq 的 thread 那麼高。
- 在 process context 裡，thread 像一般 process：**可以睡覺、等鎖、存取使用者空間記憶體**。
- `thread` 欄位是 NULL 的話，這個 IRQ 就不是 threaded IRQ。

### 7.1 【原始碼】優先權 50 從哪來

IRQ thread 一啟動就：

```c
// kernel/irq/manage.c:1294-1296（irq_thread() 裡）
	irq_thread_set_ready(desc, action);

	sched_set_fifo(current);
```

```c
// kernel/sched/core.c:7831-7836
void sched_set_fifo(struct task_struct *p)
{
	struct sched_param sp = { .sched_priority = MAX_RT_PRIO / 2 };
	WARN_ON_ONCE(sched_setscheduler_nocheck(p, SCHED_FIFO, &sp) != 0);
}
EXPORT_SYMBOL_GPL(sched_set_fifo);
```

| 部分 | 意思 |
|---|---|
| **`MAX_RT_PRIO / 2`** | 第 8 講：`MAX_RT_PRIO` = 100 → **100 / 2 = 50**。**教授說的「優先權 50」就是這樣算出來的** |
| **`SCHED_FIFO`** | **即時**排程策略：先進先出，沒有時間片，跑到自己讓出或被更高優先權搶占 |
| `sched_setscheduler_nocheck` | 設定排程策略和優先權（不檢查權限，因為是 kernel 自己設的） |

第 8 講教授說「kernel 自己很多工作都跑在優先權 50」、「強烈建議不要超過 60–70」——現在知道其中一個來源了：**所有 threaded IRQ**（§9 實測：`irq/...` 那些 thread 都是 `FF 50`）。

---

## 8. Work queue

`[43:30]` 比前兩個**更通用**的東西：

- 給 **kernel 子系統**和 **device driver** 都用。
- 不要 interrupt context，也不要 threaded IRQ 那種**非常高**的優先權——「50 已經非常高了，更高會讓系統不穩，因為真正的 kernel 工作做不了」。
- 用**低優先權**的 kernel thread，以**通用、有彈性**的方式做事 → **work queue**。

`[44:00]` 跟 softirq 比：

| | Softirq | Work queue |
|---|---|---|
| 每種最多幾個 pending | **1 個**（一個位元） | **沒有限制** |
| 代價 | — | 要付的代價：**process context**、**一般優先權** |

### 8.1 `work_struct`：函式指標 + 參數

`[44:00]` work queue 的核心是 **`work_struct`**：「**完全一樣**：一個函式指標和參數。**永遠都是這樣**。」

`[47:31]` 三個欄位：

| 欄位 | 型別 | 意思 |
|---|---|---|
| **`func`** | 函式指標 | 要執行的函式 |
| **`data`** | `atomic_long_t` | **參數**——`[48:02]` 「`long` 假設是 64 位元。所有參數塞得進 64 位元就好；不然它可以是一個**指標**，指向存了很多資料的結構」 |
| **`entry`** | `list_head` | 讓 `work_struct` 串成 linked list（第 14 講的 kernel linked list） |

### 8.2 Worker pool

`[44:30]` work queue 是**相當複雜**的結構，不是一個結構而是**好幾個**。（課本 Fig 4.9）

```
                         Work queue（workqueue_struct）
                    ┌──────────┬──────────┬──────────┐
                    │ Wrapper  │ Wrapper  │ Wrapper  │   ← pool_workqueue（每 CPU 一個）
                    └────┬─────┴────┬─────┴────┬─────┘
                         ▼          ▼          ▼
                    ┌──────────────────────────────────┐
                    │ Worker pool                       │
                    │  ┌──────────────┐  ┌───────────┐  │
                    │  │ work 串列     │  │ thread 們  │  │
                    │  │ ws→ws→ws→ws  │  │ （idle /   │  │
                    │  └──────────────┘  │  busy）    │  │
                    │                    └───────────┘  │
                    └──────────────────────────────────┘
                     Inactive items（溢位區）
```

`[45:00]` **Worker pool**：

- 一組 **thread**，工作是處理 `work_struct`。
- 一個 `work_struct` 的 **linked list**——每個是一份工作。
- `[46:01]` 每個 work item 是一個 kernel task，被**指派給一個 worker thread**，那個 thread 執行它、做完。

`[49:01]` 細節：

- worker pool 通常**跟一顆 CPU 綁在一起**，每顆 CPU 有一個 worker pool，裡面有一組 worker thread，負責執行**那顆 CPU** 上的 work item。
- 有一個 **idle thread 的串列**。
- `[49:30]` 要給工作時：從 idle 串列**拿一個 thread**，從 work 串列**拿一個工作包**，配對。之後看排程器方便，它被排程、工作做完。
- `[50:01]` **為什麼一顆 CPU 要多個 thread？** 「thread 可能會為了 I/O 之類的被**阻塞**——這裡 thread 可以做很多事，**沒有限制**。被阻塞時，別的 thread 可以接手。」
- 每個 thread 一次一個工作包，做完回 idle 串列，再拿下一個。
- `[50:31]` **thread 和 `work_struct` 的配對**存在一張 **hash table**：給一個 `work_struct` 指標，能找到**目前在處理它的 thread**。

`[51:00]` 「根本上兩種結構：一組 `work_struct`（延後的工作），一組分配給 CPU 的 worker thread——需要時直接拿一個 idle thread 指派工作，**不用重新建 thread**。」

【課本補充】課本說：「不用每次配置或完成工作項目就建立和銷毀 thread——**維護一個 thread pool**。」（第 10 講的 **pool** 概念：預先配置好的物件。）

### 8.3 Pool workqueue：wrapper 管 active 和 inactive

`[51:31]` **`pool_workqueue`** 是 worker pool 的 **wrapper（包裝）**：

> 【口頭】「需要 wrapper，這是**標準的軟體工程技巧**：一個物件之上有另一個物件當包裝，它的工作是**控制**底下物件的行為，並提供**對外的介面**。這某種意義上是一種軟體工程**模式**。」

`[52:00]` 為什麼？

- worker pool 裡 active 的 work item 數量**超過某個門檻**（叫 **`max_active`**），linked list 會變得**很長**。
- 「**長的 linked list 在 kernel 裡很難搞**，因為 kernel 記憶體非常有紀律。」
- 所以超過的部分放到另一個區域：**inactive works**——一個**大得多**的區域，**不是優先**區。
- 讓 active 的 `work_struct` 串列**維持在上限內**。

`[53:00]` 「wrapper 的工作主要是**規範** worker pool 的行為：建**兩個**串列，一個給 active 的 work item，一個當**溢位區**。」

`[53:00]` 最上層是 **`workqueue_struct`**：一個由 `pool_workqueue` 組成的 linked list（`pwqs`）。

`[54:00]` 例如 16 顆 CPU，就有 **16 個** wrapper（`pool_workqueue`），每 CPU 一個。

【課本補充】課本補充（p.148）：一般操作時只處理**比較小**的資料結構（active 串列）。「worker pool 和它的 wrapper 可以想成**一個整體**。」大系統有很多 CPU，每顆（或每組）CPU 一個包好的 worker pool，「**優雅地**分割工作，也做一些負載平衡」。work queue 通常跟**一類**工作綁在一起（高優先權、批次工作、bottom half……），「不是嚴格規定，但從軟體工程看是合理的決定」。

### 8.4 系統內建的 work queue

`[53:30]` 基本函式：

| 函式 | 做什麼 |
|---|---|
| `create_workqueue(name)` | 建一個 work queue |
| `queue_work(wq, work)` | 把一個 `work_struct` 排進去 |
| `flush_scheduled_work()` | 把工作**全部做完**來清理 |

`[54:30]` 有些 work queue **永遠存在**（開機時初始化、一直都在），因為一直有工作要做：

| Work queue | 用途 |
|---|---|
| **`system_wq`** | 一般的全系統工作 |
| **`system_highpri_wq`** | 高優先權 |
| **`system_long_wq`** | 要做**很久**的工作 |
| **`system_unbound_wq`** | **不綁**特定 CPU，可以指派給任何 CPU |
| **`system_freezable_wq`** | 隨時可以**凍結**（暫停、之後恢復）的工作 |
| **`system_power_efficient_wq`** | **省電**的工作——`[55:00]`「在**行動裝置**上非常重要：想盡量省電的工作放在獨立的 power efficient work queue。處理它時，函式可以把處理器設成**低功耗模式**」 |

`[55:30]` 另外**每顆 CPU 有兩個** work queue：一個給**一般優先權**、一個給**高優先權**的工作。

`[56:00]` 教授的總結：

> 「work queue 的大想法其實很**簡單**，因為我們一再看到同樣的模式：一組 thread、一組 `work_struct`（串成 linked list 的工作包）。要做事就把一個 `work_struct` 指派給一個 thread——指派的是**函式指標加參數**。整個故事的**核心層**就是**函式指標**，我們把它存起來，就能建出這些**美妙、漂亮、複雜**的結構——它們不過是包裝函式指標這個想法的漂亮資料結構。」

### 8.5 【原始碼】`work_struct`、`worker_pool`、`pool_workqueue`

```c
// include/linux/workqueue.h:97-104
struct work_struct {
	atomic_long_t data;
	struct list_head entry;
	work_func_t func;
#ifdef CONFIG_LOCKDEP
	struct lockdep_map lockdep_map;
#endif
};
```

**三個欄位，跟教授說的一模一樣**：`data`（參數，`atomic_long_t`）、`entry`（`list_head`）、`func`（函式指標）。

```c
// kernel/workqueue.c（struct worker_pool 節錄，行號如註）
	int			cpu;		/* I: the associated cpu */          // 150
	...
	struct list_head	worklist;	/* L: list of pending works */      // 165
	...
	int			nr_workers;	/* L: total number of workers */    // 167
	int			nr_idle;	/* L: currently idle workers */      // 168
	...
	struct list_head	idle_list;	/* L: list of idle workers */       // 170
	...
	/* a workers is either on busy_hash or idle_list, or the manager */ // 174
	DECLARE_HASHTABLE(busy_hash, BUSY_WORKER_HASH_ORDER);               // 175
	...
	struct list_head	workers;	/* A: attached workers */           // 179
```

| 欄位 | 對照教授說的 |
|---|---|
| **`cpu`** | 「worker pool 通常跟**一顆 CPU** 綁在一起」 |
| **`worklist`** | 「`work_struct` 的 linked list」——註解：「**pending 的工作**串列」 |
| `nr_workers`、`nr_idle` | 總共幾個 worker、幾個閒著 |
| **`idle_list`** | 「**idle thread 的串列**」 |
| **`busy_hash`** | 「thread 和 `work_struct` 的配對存在一張 **hash table**」——第 174 行註解：「一個 worker **不是**在 `busy_hash` 就是在 `idle_list`」 |
| `workers` | 所有掛在這個 pool 的 worker |

註解開頭的 **`I:`**、**`L:`**、**`A:`** 是 kernel 的慣例：說明這個欄位**被什麼保護**——`I` = 初始化後不變、`L` = 要拿 pool 的鎖、`A` = 要拿另一把鎖。第 5 章講鎖時會更懂這個。

```c
// kernel/workqueue.c（struct pool_workqueue 節錄）
	struct worker_pool	*pool;		/* I: the associated pool */        // 202
	...
	int			nr_active;	/* L: nr of active works */          // 226
	int			max_active;	/* L: max active works */            // 227
	struct list_head	inactive_works;	/* L: inactive works */             // 228
```

| 欄位 | 對照 |
|---|---|
| **`pool`** | 包裝的那個 worker pool |
| `nr_active` | 目前 active 的工作數 |
| **`max_active`** | 「**門檻 `max_active`**」 |
| **`inactive_works`** | 「超過的放到 **inactive works** 溢位區」 |

系統內建的 work queue：

```c
// include/linux/workqueue.h:380-386
extern struct workqueue_struct *system_wq;
extern struct workqueue_struct *system_highpri_wq;
extern struct workqueue_struct *system_long_wq;
extern struct workqueue_struct *system_unbound_wq;
extern struct workqueue_struct *system_freezable_wq;
extern struct workqueue_struct *system_power_efficient_wq;
extern struct workqueue_struct *system_freezable_power_efficient_wq;
```

教授列的那幾個都在，還多一個「可凍結 + 省電」的組合。

---

## 9. 【實測】softirq、ksoftirqd、irq thread、kworker

**① `/proc/softirqs`**：每種 softirq 在每顆 CPU 上執行了幾次

```text
             CPU0       CPU1       CPU2  ...
HI:                 0          0          0 ...
TIMER:         117238      49035     110611 ...
NET_TX:             1          0          0 ...
NET_RX:          8910      14140     184255 ...
BLOCK:            247        196        719 ...
IRQ_POLL:           0          0          0 ...
TASKLET:           98         19        518 ...
SCHED:         544472     203680     448950 ...
HRTIMER:            0       2637          1 ...
RCU:           805324     584045     779860 ...
```

- **10 行、順序**跟 §6.5 的 `enum` **完全一樣**：HI、TIMER、NET_TX、NET_RX、BLOCK、IRQ_POLL、TASKLET、SCHED、HRTIMER、RCU。
- 最多的是 **`RCU`**、**`SCHED`**、**`TIMER`**——排程器的負載平衡和 RCU 每秒都在跑。
- **`NET_RX`** 在 CPU2 特別多（18 萬）：網路卡的中斷大概被路由到 CPU2（第 15 講的靜態分配），它的 bottom half 就在同一顆 CPU 上跑。

**② 三種 bottom half 的 thread**

```text
    PID CLS RTPRIO COMMAND
     15  TS      - ksoftirqd/0
    101  TS      - ksoftirqd/1
     23  TS      - ksoftirqd/2
    167  FF     50 irq/122-aerdrv
    168  FF     50 irq/123-aerdrv
    334  FF     50 irq/145-VEN_04F3:00
      8  TS      - kworker/0:0H-events_highpri
    226  TS      - kworker/0:1H-kblockd
```

| Thread | CLS | RTPRIO | 印證 |
|---|---|---|---|
| **`ksoftirqd/N`** | **`TS`**（一般排程，不是即時） | — | §6.3：**ksoftirqd 是一般的 kernel process，跑在 process context**，「享受不到商務艙待遇」。**每顆 CPU 一個**（§6.5 的 `__this_cpu_read(ksoftirqd)`） |
| **`irq/N-名字`** | **`FF`**（`SCHED_FIFO`，即時） | **50** | §7.1：**threaded IRQ**，`sched_set_fifo` → `MAX_RT_PRIO / 2` = **50**。名字裡的數字是 IRQ 號碼（對照第 15 講的 `/proc/interrupts`：122、123 是 `aerdrv`） |
| **`kworker/CPU:ID`** | `TS` | — | §8：**work queue 的 worker thread**。`0:0H` = CPU 0、`H` = **高優先權**（`events_highpri`）；`kblockd` = block 裝置的 work queue |

三種 bottom half 機制的優先權差別，在 `ps` 裡一目了然：**threaded IRQ（FIFO 50）> ksoftirqd / kworker（一般）**。（softirq 在 softirq context 裡跑時根本**不是一個 thread**，所以 `ps` 看不到——它借用被打斷的那個 thread。）

---

## 10. 一頁速查表

### 10.1 Exception

| 編號 | 名稱 | 重點 |
|---|---|---|
| 0 | DE 除以零 | → `SIGFPE` + `FPE_INTDIV` |
| 1 | DB 除錯 | DR0–DR3 |
| 2 | NMI | 不可遮罩 |
| 3 | BP 中斷點 | gdb；`-g`、DWARF |
| 6 | UD 無效指令 | 動態二進位轉譯 |
| 8 | DF double fault | fault 中的 fault → kernel panic |
| 13 | GP 一般保護錯誤 | |
| 14 | PF page fault | 資料位址在 **CR2** |
| 18 | MC machine check | 硬體錯誤；`notify_die` 例子 |

| 項目 | 內容 |
|---|---|
| IDT 入口 | `DECLARE_IDTENTRY`（宣告）+ `DEFINE_IDTENTRY`（定義） |
| 大多數走到 | `do_error_trap(regs, 錯誤碼, 名字, 編號, signal, 子代碼, 位址)` |
| 出錯指令位址 | `error_get_trap_addr(regs)` |
| 出錯資料位址 | `read_cr2()` |

### 10.2 Exception handler 的五個選項

| # | 選項 | 例子 |
|---|---|---|
| 1 | 送 signal | 除錯、除以零；沒處理就預設終止 |
| 2 | 寫 log | `printk` → `dmesg` |
| 3 | kernel panic | double fault |
| 4 | 動態二進位轉譯 | 軟體算開根號 / cosine |
| 5 | `notify_die` | 觀察者模式；machine check |

### 10.3 `notify_die` 回傳值

| 值 | 位元 | 繼續？ |
|---|---|---|
| `NOTIFY_DONE` | `0x0000` | ✅ |
| `NOTIFY_OK` | `0x0001` | ✅ |
| `NOTIFY_STOP` | `0x8001` | ❌ |
| `NOTIFY_BAD` | `0x8002` | ❌ |
| `NOTIFY_STOP_MASK` | `0x8000` | 停止位元 |

### 10.4 執行 context

| Context | 誰 | 中斷 | 可被搶占 | 排程器管？ |
|---|---|---|---|---|
| Interrupt | top half | 關 | 只被更高的中斷 | ❌ |
| Softirq | softirq | **開** | 只被 top half | ❌ |
| Process | kernel thread | 開 | ✅ | ✅ |
| User | 使用者 process | 開 | ✅ | ✅ |

### 10.5 三種 bottom half

| | Softirq | Threaded IRQ | Work queue |
|---|---|---|---|
| Context | softirq（top half 後）或 process（ksoftirqd） | process | process |
| 優先權 | 最高 / 一般（ksoftirqd） | **即時 FIFO 50** | 一般（也有高優先權版） |
| 能拿鎖、睡覺 | ❌ | ✅ | ✅ |
| 種類 | 固定 10 種，每種最多 1 個 pending | 每個 irqaction 一個 thread | 不限 |
| 誰用 | 特定子系統（timer、網路、block、排程、RCU） | driver（取代 tasklet） | 任何人，含 driver |
| 結構 | per-CPU 位元 word | `irqaction->thread`、`thread_fn` | `work_struct` → worker pool ← pool_workqueue ← workqueue_struct |
| `ps` | `ksoftirqd/N`（TS） | `irq/N-名字`（FF 50） | `kworker/C:ID`（TS） |

### 10.6 Softirq 細節

| 項目 | 內容 |
|---|---|
| raise | `or_softirq_pending(1UL << nr)`：設一個位元 |
| 執行 | `__do_softirq`：`ffs` 找位元、呼叫 `h->action`；中斷是開的 |
| 上限 | 2 毫秒、重來 10 次，超過 → `wakeup_softirqd()` |
| ksoftirqd | 每 CPU 一個，process context，可被搶占 |

### 10.7 Work queue 細節

| 結構 | 重點 |
|---|---|
| `work_struct` | `func`（函式指標）、`data`（參數或指標）、`entry`（list_head） |
| `worker_pool` | `cpu`、`worklist`、`idle_list`、`busy_hash`（work → thread） |
| `pool_workqueue` | wrapper：`pool`、`max_active`、`inactive_works`（溢位區） |
| 系統內建 | `system_wq`、`system_highpri_wq`、`system_long_wq`、`system_unbound_wq`、`system_freezable_wq`、`system_power_efficient_wq` |

### 10.8 本講看過的原始碼位置

| 檔案:行號 | 看什麼 |
|---|---|
| `arch/x86/include/asm/trapnr.h:7-30` | exception 編號 |
| `arch/x86/include/asm/idtentry.h:548` | `DECLARE_IDTENTRY(X86_TRAP_DE, ...)` |
| `arch/x86/kernel/traps.c:205-209` | `exc_divide_error` → `do_error_trap` |
| `arch/x86/mm/fault.c:1531-1533` | page fault 讀 `cr2` |
| `include/linux/notifier.h:183-191` | `NOTIFY_*` |
| `include/linux/interrupt.h:550-562` | softirq 種類 |
| `kernel/softirq.c:709-714` | `__raise_softirq_irqoff` |
| `kernel/softirq.c:492-493, 528-602` | `__do_softirq`、上限 |
| `kernel/softirq.c:74-81` | `wakeup_softirqd` |
| `kernel/irq/manage.c:1296` | IRQ thread 呼叫 `sched_set_fifo` |
| `kernel/sched/core.c:7831-7836` | `sched_set_fifo`：`MAX_RT_PRIO / 2` |
| `include/linux/workqueue.h:97-104` | `struct work_struct` |
| `kernel/workqueue.c:150-228` | `worker_pool`、`pool_workqueue` 欄位 |
| `include/linux/workqueue.h:380-386` | 系統 work queue |

---

## 11. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **exception 送 signal（`SIGFPE`）、signal handler** | **第 17 講**：signal（下一講） |
| **breakpoint、ptrace、`SIGTRAP`** | 第 10 講的 ptrace；**第 17 講** |
| **top half 不能拿鎖、死結** | **第 20–22 講**：鎖、kernel 裡的並行（spinlock、`local_bh_disable`） |
| **`RCU_SOFTIRQ`、`__rcu`** | **第 23 講**：RCU |
| **`SCHED_SOFTIRQ`、`SCHED_FIFO`、即時優先權 50** | **第 24–27 講**：排程、即時排程器 |
| **page fault handler（`read_cr2` 之後）** | **第 29–32 講**：記憶體管理 |
| **`kblockd`、`BLOCK_SOFTIRQ`、`NET_RX_SOFTIRQ`** | 課本第 7 章（I/O 系統）。對應講次**待補** |
| **中斷延遲、bottom half 優先權對即時性的影響** | 另一門課 **Real Time Systems**：RTOS、Linux-based RTOS（PREEMPT_RT 把 softirq 也 thread 化） |

---

## 附：本講一句話總結

> **Exception 跟中斷共用 IDT（x86 前 32 格，例如 0 除以零、8 double fault、14 page fault——資料位址在 CR2），handler 可以送 signal（除以零→`SIGFPE`+`FPE_INTDIV`，實測如此）、寫 log、panic、用軟體模擬缺的指令，或用 `notify_die` 讓一串觀察者依序處理。因為 top half 跑在不能阻塞、不能拿鎖的 interrupt context，剩下的工作交給三種 bottom half：softirq（固定 10 種、每種一個 per-CPU 位元，在 softirq context 跑超過 2 毫秒或 10 輪就交給一般優先權的 `ksoftirqd`）、threaded IRQ（`irqaction` 指定的 thread，`SCHED_FIFO` 優先權 `MAX_RT_PRIO/2` = 50），和最通用的 work queue（`work_struct` = 函式指標 + 參數，由每顆 CPU 的 worker pool 裡的 `kworker` 執行）。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 16（Chapter 4, Part III）錄影逐字稿（YouTube 自動字幕，57:17）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§4.2.7、§4.3，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0）結果。*
