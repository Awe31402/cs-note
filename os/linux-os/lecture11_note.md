# Operating Systems: A Linux Kernel-Oriented Approach 第十一講完整筆記：Process 的建立（Chapter 3, Part V）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 11（Chapter 3, Part V）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（36:54，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§3.2（p.90–98）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
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
- [1. 開機後的三個特殊 process](#1-開機後的三個特殊-process)
  - [1.1 PID 0：idle](#11-pid-0idle)
  - [1.2 PID 1：init，從 kernel 走到使用者空間](#12-pid-1init從-kernel-走到使用者空間)
  - [1.3 PID 2：kthreadd](#13-pid-2kthreadd)
  - [1.4 【原始碼】`rest_init()`：生出 PID 1 和 PID 2](#14-原始碼rest_init生出-pid-1-和-pid-2)
- [2. Linux 怪在哪裡：複製一份再說](#2-linux-怪在哪裡複製一份再說)
- [3. System call 時不另開 thread](#3-system-call-時不另開-thread)
- [4. `fork`](#4-fork)
  - [4.1 範例程式](#41-範例程式)
  - [4.2 一次呼叫、兩次返回](#42-一次呼叫兩次返回)
  - [4.3 `fork` 複製了什麼](#43-fork-複製了什麼)
  - [4.4 【原始碼】`fork` system call 與「子行程拿到 0」](#44-原始碼fork-system-call-與子行程拿到-0)
- [5. Copy-on-write（COW）](#5-copy-on-writecow)
  - [5.1 先只複製 page table](#51-先只複製-page-table)
  - [5.2 一寫就複製](#52-一寫就複製)
  - [5.3 怎麼知道有人寫？故意製造 fault](#53-怎麼知道有人寫故意製造-fault)
  - [5.4 本來就唯讀的 page 怎麼辦：P2 位元](#54-本來就唯讀的-page-怎麼辦p2-位元)
  - [5.5 【原始碼】fork 時把 page 設成唯讀](#55-原始碼fork-時把-page-設成唯讀)
- [6. `exec`：換掉自己的映像](#6-exec換掉自己的映像)
  - [6.1 範例程式](#61-範例程式)
  - [6.2 `exec` 做了什麼](#62-exec-做了什麼)
- [7. `clone` 與 `copy_process()`](#7-clone-與-copy_process)
  - [7.1 `fork` 只是 `clone` 的一種](#71-fork-只是-clone-的一種)
  - [7.2 `copy_process()` 的步驟](#72-copy_process-的步驟)
  - [7.3 Thread 就是共享比較多的 clone](#73-thread-就是共享比較多的-clone)
  - [7.4 【原始碼】`copy_process()`](#74-原始碼copy_process)
  - [7.5 【原始碼】`copy_mm()`：共享還是複製](#75-原始碼copy_mm共享還是複製)
  - [7.6 【原始碼】clone 旗標](#76-原始碼clone-旗標)
- [8. 三種 thread：user、I/O、kernel](#8-三種-threaduseriokernel)
- [9. 【實測】fork、COW、exec、PID 1 與 2](#9-實測forkcowexecpid-1-與-2)
- [10. 一頁速查表](#10-一頁速查表)
- [11. 與後續講次的連結](#11-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

`[00:33]` process 的**建立與銷毀**。

> 「Linux 建立 process 的方式跟其他 OS **很不一樣**，但大致遵循所有**類 Unix** 系統的機制。一開始可能看起來**不傳統、很怪**，但慢慢體會它的好處，就會開始覺得有道理。」

| # | 主題 | 對應課本 |
|---|---|---|
| 1 | idle（PID 0）、init（PID 1）、kthreadd（PID 2） | §3.2 開頭 |
| 2 | **`fork`** 與 **copy-on-write** | §3.2.1 |
| 3 | **`exec`** | §3.2.2 |
| 4 | `clone`、`copy_process()` | §3.2.1 Details |
| 5 | user / I/O / kernel thread | §3.2.3 |

---

## 1. 開機後的三個特殊 process

### 1.1 PID 0：idle

`[01:30]` kernel 啟動、開機完成之後，**基本上什麼都沒在做**。

- **PID 0** 叫 **idle process（閒置 process）**。
- 用途是**記帳（accounting）**：「如果你希望所有 process 加起來佔 100% 的時間，那假設 50% 的時間什麼都沒做，就把那 50% 算到 idle process 頭上。」

【課本補充】課本補充（p.90）：kernel 一開始只啟動**一個** process 來開機。開機完成後，**這個開機 process 就變成 idle process**（也叫 **swapper**），PID 0。它產生 init。

### 1.2 PID 1：init，從 kernel 走到使用者空間

`[02:01]` 接著跑 **init process**，它幫 kernel 做一些開機後的工作。

`[02:33]` init 很特別：

```
  ─────────────── kernel 空間 ───────────────
        init 在這裡誕生（kernel process）
              │
              │ 遷移
              ▼
  ─────────────── 使用者空間 ───────────────
        init 在這裡度過餘生，成為所有使用者 process 的「母親」
```

`[04:31]` 「這是**少見**的情況：某個東西從 kernel 開始、走到使用者模式。一般我們看到的是**反過來**（使用者 process 發 system call 進 kernel）。」

- `[04:00]` **`start_kernel()`**：開機時呼叫的核心函式（第 1 講看過，`init/main.c`），它（間接）建立 init。
- 之後**每個使用者 process**都是 **fork init**，或 fork init 的子孫產生的 → 一棵以 init 為根的 process 樹。

【課本補充】課本補充（p.91）：

- `start_kernel` 初始化 kernel 和所有連接的裝置。「把它想成 **kernel 的 `main` 函式**。」
- init 怎麼從 kernel 轉到使用者模式？靠 **`execve`**（§6）。
- init、所有必要的使用者層 process 和 kthreadd 都建好後，**開機過程結束**，就可以跟系統互動、啟動應用程式。

### 1.3 PID 2：kthreadd

`[35:02]` 跟使用者空間的 init 對應，kernel 空間有 **`kthreadd`**：

| | 使用者空間 | kernel 空間 |
|---|---|---|
| 母親 process | **init** | **kthreadd** |
| PID | **1** | **2** |
| 產生什麼 | 所有使用者 process | 所有 **kernel thread** |
| 建立函式 | `fork` / `clone` | `kthread_create` |

【課本補充】idle process 產生另一個 process 叫 **kthreadd**（PID 2），它是所有 kernel thread 的根。**kernel thread 專門替 kernel 做事，永遠不會轉到使用者模式。**

所以完整的家譜是：

```
              idle（PID 0，swapper）
               ╱                 ╲
      init（PID 1）            kthreadd（PID 2）
       ╱    │    ╲              ╱    │    ╲
   使用者 process 樹         kernel thread 樹
```

### 1.4 【原始碼】`rest_init()`：生出 PID 1 和 PID 2

第 1 講看過 `start_kernel()` 的開頭。它最後會呼叫 `rest_init()`：

```c
// init/main.c:686-732（節錄）
noinline void __ref rest_init(void)
{
	struct task_struct *tsk;
	int pid;

	rcu_scheduler_starting();
	/*
	 * We need to spawn init first so that it obtains pid 1, however
	 * the init task will end up wanting to create kthreads, which, if
	 * we schedule it before we create kthreadd, will OOPS.
	 */
	pid = user_mode_thread(kernel_init, NULL, CLONE_FS);
	...
	numa_default_policy();
	pid = kernel_thread(kthreadd, NULL, CLONE_FS | CLONE_FILES);
	...
	complete(&kthreadd_done);

	/*
	 * The boot idle thread must execute schedule()
	 * at least once to get things moving:
	 */
	schedule_preempt_disabled();
	/* Call into cpu_idle with preempt disabled */
	cpu_startup_entry(CPUHP_ONLINE);
}
```

| 行 | 意思 | 對照 |
|---|---|---|
| 692–696 註解 | 「要**先**產生 init，讓它拿到 **PID 1**。但 init 之後會想建立 kthread，如果在 kthreadd 建好之前就排程它，會 **OOPS**（當掉）。」 | 順序很重要：先 init 拿 PID 1，再 kthreadd 拿 PID 2 |
| 697 | **`user_mode_thread(kernel_init, ...)`** → **PID 1** | 名字就說了：「會變成**使用者模式**的 thread」 |
| 710 | **`kernel_thread(kthreadd, ...)`** → **PID 2** | 純 kernel thread |
| 724 | `complete(&kthreadd_done)` | 通知 init：kthreadd 好了，你可以繼續 |
| 726–732 | 「開機的 idle thread 必須至少執行一次 `schedule()`」→ 進入 `cpu_startup_entry` | **執行 `rest_init` 的這個開機 thread 自己，就變成 idle（PID 0）**：它讓出 CPU，之後只在沒事做時跑——正是課本說的「開機 process 變成 idle process」 |

init 怎麼走到使用者空間？`kernel_init` 最後會嘗試執行使用者層的 init 程式：

```c
// init/main.c:1578-1582
	if (!try_to_run_init_process("/sbin/init") ||
	    !try_to_run_init_process("/etc/init") ||
	    !try_to_run_init_process("/bin/init") ||
	    !try_to_run_init_process("/bin/sh"))
		return 0;
```

- 依序試 `/sbin/init`、`/etc/init`、`/bin/init`，都沒有就跑 `/bin/sh`。
- `try_to_run_init_process` 底下就是 **`execve`**——把 kernel thread 的映像**換成**使用者程式，從此它就是使用者 process。
- 在這台 Ubuntu 上，`/sbin/init` 是 **systemd**（§9 實測）。

---

## 2. Linux 怪在哪裡：複製一份再說

`[02:33]` Linux 建 process 怪在：

> 「你拿一個 process 和它**整個記憶體映像**，**複製一份**，建立它的**分身（clone）**。」

`[03:01]` 

- **fork** 是 cloning 的**一種**：參數寫死、設好了。
- **cloning** 是**通用**機制。
- 建新 process = 建一個現有 process 的**完全複製品**，之後兩者可以**分道揚鑣**。

`[05:01]` 你可能會說：那所有 process 都做同樣的事？**不會**：

```
  shell（父）
     │ fork
     ▼
  shell 的複製品（子）── exec ──► notepad
     「我不想當 shell 的複製品，我想當 notepad」
```

`[05:31]` 複製之後，可以**丟掉映像、載入新的映像**：「我不做我父親在做的事，我要做**完全不同**的事。」例如父 process 是 shell，子 process 一開始也是 shell，但它呼叫 **`exec` 家族**的 system call 把自己的映像換成 notepad。

`[06:30]` 「這就是 Linux 啟動新 process 的方式：**先成為父 process 的複製品**，再用 `exec` 把映像換成你想執行的東西。」

【課本補充】課本說（p.63）：Linux 跟其他類 Unix 系統一樣，**先完全複製父 process 的記憶體映像，需要的話再替換**。**Windows 等其他 OS 不是這樣做的。** 有歷史原因，而且程式設計師隨時間學會利用它來設計有效率的系統。「一旦正確理解，這個模型**簡單又直覺**。」

---

## 3. System call 時不另開 thread

`[07:00]` process 建好、發了一個 system call，會怎樣？（第 8 講預告過）

- **不**建一個獨立的 kernel thread——那開銷很大。
- **同一個 process** 轉換到 **kernel mode**：模式從 user 改成 kernel。
- 在 kernel mode 可以存取 kernel 的資料結構（不是使用者的）。
- 但「**process 是同一個**，只是模式變了」。
- 要恢復時，模式從 kernel 改回 user。

`[08:30]` 「這是**非常有效率**的機制，不浪費時間、不浪費工作。」

【課本補充】課本 §3.2.3 補充：為了確保 kernel mode 讀寫的資料**回到 user mode 後存取不到**，採取兩步：

1. kernel mode 時分配一個**獨立的 kernel stack**，放在 kernel 的虛擬位址空間裡。kernel 化身可以存取使用者空間的資料，也可以存取 kernel 空間的資料。
2. kernel 的虛擬位址**分開**：例如 32 位元系統，下 3 GB 給使用者、上 1 GB 給 kernel，所有 kernel 資料結構（包括 kernel stack）都在上面 1 GB。**使用者 thread 存取不到 kernel 虛擬記憶體**——TLB 會馬上擋下來，因為每個 TLB entry 都記錄了這個資訊（第 5 講的 `USER` 位元）。

---

## 4. `fork`

### 4.1 範例程式

`[08:30]`（課本 Listing 3.9）

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
	int pid = fork();

	if (pid == 0) {
		printf("I am the child\n");
	} else {
		printf("I am the parent: child = %d\n", pid);
	}
}
```

### 4.2 一次呼叫、兩次返回

`[09:00]` 原本的 process 決定建一個子 process，呼叫 `fork`，建立一個**複製品**：

```
              原本的 process
                    │
                 fork()
              ╱          ╲
    父（繼續）              子（新建的複製品）
  fork() 回傳 子的 PID      fork() 回傳 0
```

`[10:03]` **fork 的魔法**：

> 「`fork` 之前只有**一個** process，`fork` 之後有**兩個**。兩個都在執行**完全一樣的程式碼**——位址空間已經分開了，但它們在**同一個點**，要執行同一段程式碼，**只有一個關鍵差別**。」

`[10:32]` 「**這一點要用最大的注意力理解**」：

> **`fork` 回傳給父和子的值不一樣。**

| | `fork()` 回傳 | 意思 |
|---|---|---|
| **父 process** | **子 process 的 PID** | 「我是父親，我的子是這個」 |
| **子 process** | **0** | 「我是子」 |

`[10:32]` 【口頭】「`fork` 之前沒有子 process，`fork` 之後才有。之後子 process **以為自己從一開始就在跑**——雖然顯然不是。它以為自己**從來沒被 fork 過**。」

`[11:31]` 然後檢查 PID：是 0 就印「I am the child」，不是 0 就印「I am the parent」。

`[12:00]` 「這是 OS（類 Linux 的 OS）的**基本機制**：看 `fork` 的回傳值，決定做什麼——是父就做這個，是子就做那個。」

【課本補充】課本把這個「怪」講得很清楚（p.93）：

- 「整件事最怪的地方：**子 process 看起來會從 `fork` 返回，雖然它根本沒呼叫 `fork`**——呼叫 `fork` 時它根本不存在。」
- 子 process 在 kernel 深處的 cloning 邏輯裡被建立：完整的 `task_struct` 和所有附屬資料結構，記憶體空間**完全複製**，**包括暫存器狀態和返回位址**。
- 所有位址都是**虛擬**的，所以複製不影響正確性：子 process 需要的位址都在**它自己的位址空間**裡。
- 就像父 process 最後會從 `fork` 返回，子 process 也會：它從暫存器或 stack 拿到返回位址（看架構），那個返回位址是虛擬的、在它自己的空間裡，於是把回傳值放進它私有的 `pid` 變數，開始執行第 8 行。
- **Point 3.2.1**：子 process 拿到 0，父 process 拿到子 process 的 PID。「這就是這個機制的**精妙之處**。」

### 4.3 `fork` 複製了什麼

`[12:31]`

| 複製的東西 | 說明 |
|---|---|
| **所有記憶體區域** | 父 process 的整個記憶體空間 |
| **process 狀態** | 例如 `TASK_RUNNING`。「當然不能 fork 一個 blocked 的 process——你的 process 一定是在跑的」 |
| **PC** | **兩個都設成同一個值**：都指向 `fork` **之後**那條指令 |

`[13:30]` 唯一的差別：**回傳值不同**。「一個拿 0，一個拿 PID，這就是區分的方法。」

【課本補充】課本強調「複製」的意思（p.92）：

> 假設 `fork` 之前定義 `x = 7`。`fork` 之後兩個 process 讀 `x` 都是 7。但**這兩個 `x` 是不同的變數**：父 process 把 `x` 改成 19，子 process 讀到的**還是 7**，因為它有自己的私有副本。子 process 拿到的是 `x` 的**值的副本**，不是父 process `x` 的**參考**。名字一樣，但變數本身不同。

（§9 實測正是這個例子。）

### 4.4 【原始碼】`fork` system call 與「子行程拿到 0」

`fork` system call 本身非常短：

```c
// kernel/fork.c:2764-2776
SYSCALL_DEFINE0(fork)
{
#ifdef CONFIG_MMU
	struct kernel_clone_args args = {
		.exit_signal = SIGCHLD,
	};

	return kernel_clone(&args);
#else
	/* can not support in nommu mode */
	return -EINVAL;
#endif
}
```

| 部分 | 意思 |
|---|---|
| `SYSCALL_DEFINE0(fork)` | 定義一個**沒有參數**（0）的 system call `fork`。這個巨集會展開成第 2 講 system call table 裡的 `__x64_sys_fork` |
| `.exit_signal = SIGCHLD` | 子 process 死掉時，送 **`SIGCHLD`** 給父 process——第 7 講的 zombie 流程 |
| **其他欄位都沒設（= 0）** | **沒有任何 `CLONE_*` 旗標** → 什麼都不共享、全部複製 |
| `kernel_clone(&args)` | 所有建 process 的路徑最後都走這裡 |
| `#else` | 沒有 MMU（記憶體管理單元）的系統**不支援 fork**——因為 fork 靠的就是虛擬記憶體（§5 的 COW）。沒有 MMU 要用 `vfork` |

**子 process 為什麼拿到 0？** `kernel_clone` 回傳的是新的 PID——這是**父 process** 從 system call 回來時 `rax` 裡的值。子 process 呢？它從來沒真的呼叫過 `fork`，它的「返回」是 kernel **假造**的：

```c
// arch/x86/kernel/process.c:192-196
	frame->bx = 0;
	*childregs = *current_pt_regs();
	childregs->ax = 0;
	if (sp)
		childregs->sp = sp;
```

| 行 | 意思 |
|---|---|
| 193 | **`*childregs = *current_pt_regs();`**：把父 process 進 kernel 時存的 **`pt_regs`**（第 2 講的 context）**整個複製**給子 process——所以子 process 的 PC、stack pointer、所有暫存器都跟父 process 一樣，「從同一點繼續」 |
| **194** | **`childregs->ax = 0;`**：**只改一個暫存器**：把子 process 的 `rax` 設成 **0**。第 2 講說 system call 的回傳值放在 `rax` |
| 195–196 | 如果有指定新的 stack（建 thread 時），換掉 `sp` |

所以子 process 第一次被排程執行時，就像「從 system call 返回」一樣，從 `pt_regs` 恢復暫存器，`rax` 是 0——`fork()` 在子 process 裡「回傳 0」就是這樣來的。教授說的「**唯一的差別是回傳值**」，在原始碼裡就是**這一行**。

---

## 5. Copy-on-write（COW）

### 5.1 先只複製 page table

`[13:30]` 有人會說：「複製位址空間**很慢**，位址空間可能有幾 MB 甚至幾 GB，全部複製要很久。」

`[14:01]` 所以**一開始什麼都不複製**，只**複製 page table**（課本 Fig 3.9a）：

```
  父的 page table              子的 page table
  ┌────────┐                  ┌────────┐
  │ VA → ──┼───────┐  ┌───────┼── ← VA │
  └────────┘       ▼  ▼       └────────┘
                 ┌──────┐
                 │ frame│   ← 兩個虛擬位址指向同一個實體 frame
                 └──────┘
```

- 父和子的虛擬位址**指向同一個實體 frame**。
- 這樣位址空間就「有效地」被複製了。
- 但**一個寫，另一個就看得到**——這不是我們要的。所以這只是**部分**解法。

### 5.2 一寫就複製

`[14:30]` 最重要的機制：**copy-on-write（寫入時複製）**，簡稱 **COW**。

> 【口頭】「大家都說 copy on write，但寫成 **C-O-W**（牛）。」

`[15:30]`（課本 Fig 3.9b）

```
  父寫入那個 page
        │
        ▼
  ┌──────┐            ┌──────┐
  │ frame│ ◄── 父      │ frame│ ◄── 子
  │（新副本）│            │（原本的）│
  └──────┘            └──────┘
     映射改了            映射不變
```

- **讀**沒關係。
- **寫**的時候：複製一份 frame。父寫 → 寫到父的那份；子寫 → 寫到子的那份。虛擬位址的映射跟著改。

`[16:32]` 「預設情況下 page 在父子之間**共享**，**一寫入才複製**——因為父和子的位址空間**本來就應該分開**。」

「這是個**快速**的機制：預設情況下，只要**複製 page table** 就能讓子 process 跑起來。這是整個 fork 機制裡**產生效率**的地方。」

【課本補充】課本補充（p.95）：為什麼 COW 的開銷很少被看到？

1. 父和子之後可能根本**不寫**記憶體的大部分，例如**程式碼和 data section**——COW 永遠不會被觸發。
2. 子 process 可能馬上**用另一個執行檔覆蓋自己的映像**（`exec`）——整個 memory map 都被抹掉，根本不用 COW。
3. **懶惰地**（有需要才）建副本，把開銷**分散**到很長一段時間，大部分應用程式很容易吸收。

「所以 fork 機制**經得起時間考驗**。」

### 5.3 怎麼知道有人寫？故意製造 fault

`[17:02]` 問題：**怎麼知道有人寫入了？**

`[15:04]` 做法：

1. fork 之後，把父和子**所有 page** 的 TLB entry 和 page table entry 都設成**唯讀**。
2. 任何一方寫入共享的 page → 產生 **fault**：「你沒有寫入權限。」
3. fault → **interrupt handler** 執行。
4. handler 建一份 frame 的副本，改映射，然後讓寫入繼續。

`[19:31]` 「你在做什麼？你在**故意製造假的 fault 來追蹤寫入**。」

（這就是第 5 講課本說的 **soft page fault**：page 在記憶體裡，但沒權限。）

### 5.4 本來就唯讀的 page 怎麼辦：P2 位元

`[17:30]` 一個**正確性**問題：

- 有些 page **本來就是唯讀**的，例如**程式碼 page**。
- 有人寫它 → 那是**安全違規**，應該報錯，**不應該**建副本。
- 但 COW 也把 page 設成唯讀——**怎麼區分**？

`[18:01]` 再加一個位元，教授和課本叫它 **P2**：

| P1（READONLY） | P2 | 意思 | 寫入時 |
|---|---|---|---|
| 1 | 0 | **本來就唯讀**（程式碼） | **安全違規** |
| 0 | 1 | **本來可寫，只是因為 fork 被設成唯讀** | **COW**：建副本，然後把 P2 設回 0 |

`[18:33]` COW 完成後把 P2 設成 0：「這個 page 不會再有 fault 了，因為已經建好副本，父和子可以在各自的私有副本上寫。」

`[19:31]` 「重點：寫入沒有寫權限的 page 時要建副本，但有個邊角案例——page **從第一天就是唯讀**的，寫它是安全錯誤。為了區分兩種情況，需要一個額外的位元，說『它被設成唯讀**只是因為 fork**』。」

【課本補充】課本的說法（p.96）：fork 時把父和子所有 page 的 **P2 設成 1**（在 page table 裡）。寫入一個 **READONLY = 0（正常情況可寫）、P2 = 1** 的 page 時，就知道是要觸發 COW。建好副本後，把父和子對應 page 的 P2 都設成 0。

### 5.5 【原始碼】fork 時把 page 設成唯讀

fork 複製 page table 時，對每個 page 做這件事：

```c
// mm/memory.c:922-929
	/*
	 * If it's a COW mapping, write protect it both
	 * in the parent and the child
	 */
	if (is_cow_mapping(vm_flags) && pte_write(pte)) {
		ptep_set_wrprotect(src_mm, addr, src_pte);
		pte = pte_wrprotect(pte);
	}
```

| 行 | 意思 |
|---|---|
| 922–925 註解 | 「如果是 COW 映射，**在父和子裡都**把它設成**寫入保護**」——跟教授說的完全一樣 |
| `is_cow_mapping(vm_flags)` | 這個區域是不是 COW 區域？（見下） |
| `pte_write(pte)` | 這個 page **目前可寫**嗎？本來就唯讀的不用管 |
| `ptep_set_wrprotect(src_mm, ...)` | 把**父 process**（src）的 PTE 改成唯讀——清掉第 5 講的 **`RW` 位元** |
| `pte = pte_wrprotect(pte)` | 要複製給**子 process** 的 PTE 也是唯讀 |

**P2 位元在哪裡？** Linux 實際上**沒有在 PTE 裡放一個 P2 位元**，而是看這個 page 所屬的 **VMA**（第 9 講）：

```c
// include/linux/mm.h:1367-1370
static inline bool is_cow_mapping(vm_flags_t flags)
{
	return (flags & (VM_SHARED | VM_MAYWRITE)) == VM_MAYWRITE;
}
```

| 旗標 | 意思 |
|---|---|
| `VM_MAYWRITE` | 這個區域**原則上可以寫** |
| `VM_SHARED` | 這個區域是**共享**的（例如共享記憶體），寫入**應該**讓別人看到，不能 COW |
| `== VM_MAYWRITE` | 「**可以寫、但不是共享的**」→ 是 COW 區域 |

所以 page fault 時 kernel 的判斷是：

| PTE 的 `RW` | VMA 的 `VM_WRITE` | 情況 |
|---|---|---|
| 0（唯讀） | **0** | **本來就唯讀**（例如程式碼）→ 安全違規，送 `SIGSEGV` |
| 0（唯讀） | **1** | **本來可寫，是 COW** → 複製 page（`mm/memory.c` 的 `do_wp_page()` → `wp_page_copy()`），再設成可寫 |

課本的 **「P2 位元」** 在 Linux 裡的實際角色，是由 **VMA 的 `vm_flags`** 擔任的——每個區域記一次，不用每個 PTE 都多一個位元。這又是第 9 講說的「**把 page 分組成區域、每個區域套一組政策**」的好處。

---

## 6. `exec`：換掉自己的映像

### 6.1 範例程式

`[20:00]` fork 之後，子 process 決定**不當**父 process 的複製品，要執行**別的程式**。例如父 process 是 shell，子 process 不當 shell，而是變成 vi。（課本 Listing 3.10）

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PWDPATH "/usr/bin/pwd"

int main(void) {
	char *argv[2] = {"pwd", NULL};
	int pid = fork();

	if (pid == 0) {
		execv(PWDPATH, argv);
	} else {
		printf("I am the parent: child = %d\n", pid);
	}
}
```

`[20:33]`

- `exec` 家族有**很多個**呼叫，參數數量各不同。
- 子 process 執行 **`pwd`** 指令（print working directory，印出目前目錄）。
- `execv` 收：**執行檔的路徑**，和一個**參數陣列**（第一個是指令名稱、後面是命令列參數、**最後一定是 NULL**）。
- 子 process 變成 `pwd`；父 process 印「I am the parent」。

【課本補充】`exec` 家族的 library call 有很多，**都包裝同一個 `exec` system call**。

### 6.2 `exec` 做了什麼

`[22:01]`

| 步驟 | 內容 |
|---|---|
| 1 | **清掉記憶體狀態**（大部分） |
| 2 | 從**新的執行檔**複製 **text、data、BSS** section（跟啟動新 process 的方式一樣） |
| 3 | **stack 和 heap 重設**：heap 大小變 0、stack 大小變 0 |
| 4 | BSS（沒初值的全域變數）要**準備空間** |
| 5 | 建好新 process 的 **memory map**——「你是子 process、從 fork 生出來的，但你決定不執行父親的映像，要執行新映像，那就請 **loader** 從執行檔載入」（第 4 講） |
| 6 | **其他資源保留**：開啟的檔案、網路連線等，**`exec` 之後還在**，除非你放棄它們，或 `exec` 的參數說「把我的資源都交出去」 |
| 7 | **PC 設到 text section 的開頭**，開始執行新映像 |

`[24:31]` 「從 shell 生出來的，但決定執行 **emacs**，那 emacs 的整個程式碼和資料就被載入，然後從 emacs 的**第一條指令**開始執行。」

【課本補充】課本說：在命令列下指令時，**shell 先 fork 出一個子 process**，子 process 再用指令裡的執行檔替換自己的 memory map。「這就像重新開始一次執行。`exec` 返回後，從新 text section 的開頭執行——我們從一個 fork 出來的 process 開始的這件事，被**方便地遺忘了**。這就是 **Linux 的方式**。」

> 補充（不是教授講的）：`exec` 成功的話**不會返回**——呼叫 `exec` 的程式碼已經被抹掉了。所以 `execv` 下一行只有在**失敗**時才會執行（例如檔案不存在）。§9 實測的程式裡就放了一行「這行不會執行」來示範。kernel 裡的實作在 `fs/exec.c`（`do_execveat_common()` → `begin_new_exec()`）。

---

## 7. `clone` 與 `copy_process()`

### 7.1 `fork` 只是 `clone` 的一種

`[25:01]` **總的** system call 是 **`clone`**——`fork` 是它的子集。

- 用 `clone` 也可以建 **thread**：告訴 `clone`「我的 code section 要共享、data 要共享、BSS 要共享、heap 要共享」，不共享的是 **stack** 和 **thread local storage**。
- `clone` 可以收**很多參數**。

`[25:31]` 不管帶什麼參數，最後都會呼叫 **`copy_process()`** 函式：

> 「它的工作是拿父 process，最後**為子 process 建一個 `task_struct`**。不管怎麼走到這裡，都會複製 process、建 `task_struct`。」

【課本補充】課本補充（p.96）：

- 支援的 system call 清單在 `include/linux/syscalls.h`。
- **`vfork`**：為「子 process **馬上**呼叫 `exec`」的情況最佳化——那就不用完整初始化子 process、也不用複製父 process 的 page table。
- 在多執行緒 process 裡 fork，**只有呼叫的那個 thread 被 fork**。
- 這些函式最後都呼叫 `kernel/fork.c` 的 `copy_process`。

### 7.2 `copy_process()` 的步驟

`[26:00]`、`[28:32]` 主要目標：**拿父 process 的 `task_struct`，複製一份給子 process**。（之後子 process 可能清掉它、換成別的映像，那是另一回事。）

教授的摘要：

> 「kernel 裡**不用 `new` 或 `malloc`**，那是很複雜的使用者空間操作，kernel 負擔不起這種記憶體**不紀律**。所以有獨立的 kernel 配置器（之後講）。」

`[29:01]` 步驟（課本列成三大類）：

**① 複製 `task_struct`** `[26:30]`

| # | 步驟 | 教授的說明 |
|---|---|---|
| a | 建新 task 和它的 `task_struct` | 跟**物件配置器**要一個新 `task_struct`（記憶體那章講） |
| b | 設好 **kernel stack** | 跟**8 KB stack 配置器**要一塊，指標存進 `task_struct` |
| c | 複製**架構狀態** | 浮點暫存器的狀態等等。課本：把所有暫存器（一般、特權、flags）推進 kernel stack |
| d | 其他記帳資訊 | |
| e | **執行時間設成 0** | 【口頭】「它是**全新**的。就像買新車，**里程表要從 0 開始**。」 |
| f | **分配給一顆 CPU** | 初始化完就能在那顆 CPU 上跑 |
| g | **配一個 PID** | 第 10 講的 `alloc_pid`：從子 namespace 走到根，每層拿一個號碼 |

**② 複製資源** `[30:00]`

| # | 步驟 | 教授的說明 |
|---|---|---|
| a | **開啟檔案的連線** | 「什麼是開啟檔案的連線？檔案是硬碟上一串連續 bytes；**連線**只是一個資料結構，表示『這個 process 在存取這個檔案』，存一些狀態和記帳資訊。」課本：之後父和子可以存取**同一個**開啟的檔案（除非父 process 獨佔鎖住） |
| b | **目前檔案系統的參考** | |
| c | **signal handler** | |
| d | **整個 `mm_struct`**（memory map、page table） | |
| e | **namespace 成員資格**、I/O 權限 | 預設子 process 跟父 process **權限一樣** |

「預設全部複製，除非新 task 說『我不需要』。」`[31:02]`「`task_struct` 附帶的那一大堆東西，全部**整套（lock, stock and barrel）**從父複製給子。」

**③ 建立外部關係** `[31:02]`

| # | 步驟 |
|---|---|
| a | 把子 process 加進父 process 的**子 process 串列** |
| b | 子 process 的**父指標**設成父 process；把子 process 加進所有兄弟的 **sibling 串列**，也把兄弟加進它的串列 |
| c | 把 **thread group、process group、session** 資訊加進子 process 的 `struct pid` |

### 7.3 Thread 就是共享比較多的 clone

`[32:00]` 建**多執行緒** process，也是**同一個** `clone` system call，只是明確指定 memory map 的哪些區域要 clone、哪些不要：

```
       Thread A                     Thread B（clone 出來的）
  ┌──────────────┐             ┌──────────────┐
  │ Stack        │             │ Stack（新的） │  ← 分開
  ├──────────────┤             ├──────────────┤
  │ Heap         │ ═══════════ │ Heap         │
  │ BSS          │ ═══ 共享 ═══ │ BSS          │
  │ Data         │ ═══════════ │ Data         │
  │ Text         │ ═══════════ │ Text         │
  └──────────────┘             └──────────────┘
```

> 「thread 和 process **是一樣的**，只是 `clone` 的參數不同。thread 的參數是：『請建一個**獨立的 stack** 和獨立的 **TLS**，其他的你儘管複製——**這裡的複製其實是共享**。』」

`[33:31]` 建 thread 時要設好 **thread group ID**：加入現有的 thread group 就要加進去；加入 process group、session 也一樣。

### 7.4 【原始碼】`copy_process()`

`copy_process` 從 `kernel/fork.c:2013` 開始，**將近 600 行**。主要步驟（行號）：

| 行 | 呼叫 | 對照課本的步驟 |
|---|---|---|
| 2100 | **`dup_task_struct(current, node)`** | ① a、b：**複製 `task_struct`、配 kernel stack** |
| 2129 | `copy_creds()` | 複製身分（使用者 ID、權限） |
| 2239 | `sched_fork()` | ① e、f：排程器初始化（執行時間歸零、分配 CPU） |
| 2257 | **`copy_files()`** | ② a：開啟的檔案 |
| 2260 | **`copy_fs()`** | ② b：檔案系統 |
| 2263 | **`copy_sighand()`** | ② c：signal handler |
| 2266 | `copy_signal()` | signal 狀態 |
| 2269 | **`copy_mm()`** | ② d：**記憶體**（§7.5） |
| 2272 | **`copy_namespaces()`** | ② e：namespace |
| 2275 | `copy_io()` | I/O context |
| 2278 | **`copy_thread()`** | ① c：**架構狀態**（§4.4 的 `childregs->ax = 0` 就在這裡） |
| 2285 | **`alloc_pid()`** | ① g：**配 PID**（第 10 講） |

```c
// kernel/fork.c:2257-2290（節錄）
	retval = copy_files(clone_flags, p);
	if (retval)
		goto bad_fork_cleanup_semundo;
	retval = copy_fs(clone_flags, p);
	if (retval)
		goto bad_fork_cleanup_files;
	retval = copy_sighand(clone_flags, p);
	if (retval)
		goto bad_fork_cleanup_fs;
	retval = copy_signal(clone_flags, p);
	if (retval)
		goto bad_fork_cleanup_sighand;
	retval = copy_mm(clone_flags, p);
	if (retval)
		goto bad_fork_cleanup_signal;
	...
	retval = copy_thread(p, args);
	if (retval)
		goto bad_fork_cleanup_io;
	...
	if (pid != &init_struct_pid) {
		pid = alloc_pid(p->nsproxy->pid_ns_for_children, args->set_tid,
				args->set_tid_size);
```

**錯誤處理的模式**：每一步失敗就 `goto` 到對應的清理標籤——注意標籤的順序是**反過來**的：`copy_fs` 失敗 → 清 `files`；`copy_sighand` 失敗 → 清 `fs`……這樣只會清理**已經做完**的步驟。這是 kernel 裡很常見的 C 寫法（C 沒有例外處理，用 `goto` 做集中的清理）。

每個 `copy_*` 都收 **`clone_flags`**——由旗標決定這個資源要**複製**還是**共享**（§7.5）。

③ 建立外部關係：

```c
// kernel/fork.c:2472-2489（節錄）
			list_add_tail(&p->sibling, &p->real_parent->children);
			list_add_tail_rcu(&p->tasks, &init_task.tasks);
			attach_pid(p, PIDTYPE_TGID);
			attach_pid(p, PIDTYPE_PGID);
			attach_pid(p, PIDTYPE_SID);
			__this_cpu_inc(process_counts);
		} else {
			current->signal->nr_threads++;
			...
			list_add_tail_rcu(&p->thread_group,
					  &p->group_leader->thread_group);
			...
		}
		attach_pid(p, PIDTYPE_PID);
		nr_threads++;
```

| 行 | 對照 |
|---|---|
| 2472 | **`list_add_tail(&p->sibling, &p->real_parent->children)`**：③ a——把子 process 的 `sibling` 節點掛到**父 process 的 `children` 串列**尾端（第 7 講的欄位） |
| 2473 | 加進**全系統所有 task** 的串列（從 `init_task`，也就是 PID 0 開始） |
| 2474–2476 | ③ c——**`attach_pid`** 把它加進 TGID、PGID、SID 的串列（第 10 講 `struct pid` 的 `tasks[]`） |
| 2478–2487 | `else`：**建的是 thread**（`CLONE_THREAD`）——不加進父 process 的 children，而是加進 **thread group** 的串列，thread 數量 +1 |
| 2489 | 不管哪種，都 `attach_pid(PIDTYPE_PID)` |

### 7.5 【原始碼】`copy_mm()`：共享還是複製

教授說「thread 和 process 一樣，只是 `clone` 參數不同」。看記憶體這一步：

```c
// kernel/fork.c:1592-1606
	oldmm = current->mm;
	if (!oldmm)
		return 0;

	if (clone_flags & CLONE_VM) {
		mmget(oldmm);
		mm = oldmm;
	} else {
		mm = dup_mm(tsk, current->mm);
		if (!mm)
			return -ENOMEM;
	}

	tsk->mm = mm;
	tsk->active_mm = mm;
```

| 行 | 意思 |
|---|---|
| 1592–1594 | 父 process 沒有 `mm`？那它是 **kernel thread**（kernel thread 沒有使用者記憶體空間）→ 不用複製 |
| **1596–1598** | **有 `CLONE_VM`** → **共享**：`mm = oldmm`，子 process 直接用**父 process 的同一個 `mm_struct`**，只把引用計數 +1（`mmget`）。這就是 **thread** |
| **1599–1600** | **沒有 `CLONE_VM`** → **複製**：`dup_mm` 建一個新的 `mm_struct`，複製 VMA 和 page table（§5.5 的 COW 寫入保護就發生在這裡面）。這就是 **fork** |
| 1605–1606 | 設好子 process 的 `mm` 和 `active_mm`（第 9 講說的兩個指標） |

**整個「thread vs process」的差別，在記憶體這一步就是一個 `if`。** 其他 `copy_files`（看 `CLONE_FILES`）、`copy_fs`（看 `CLONE_FS`）、`copy_sighand`（看 `CLONE_SIGHAND`）都是同樣的模式。這正是課本 Point 3.1.1 說的「**thread 和獨立 process 是一個光譜的兩端**」——每個旗標決定一種資源要不要共享。

### 7.6 【原始碼】clone 旗標

```c
// include/uapi/linux/sched.h:11-19（節錄）
#define CLONE_VM	0x00000100	/* set if VM shared between processes */
#define CLONE_FS	0x00000200	/* set if fs info shared between processes */
#define CLONE_FILES	0x00000400	/* set if open files shared between processes */
#define CLONE_SIGHAND	0x00000800	/* set if signal handlers and blocked signals shared */
...
#define CLONE_VFORK	0x00004000	/* set if the parent wants the child to wake it up on mm_release */
...
#define CLONE_THREAD	0x00010000	/* Same thread group? */
```

又是**位元旗標**（第 7 講的模式），每個旗標一個位元，可以 OR 起來組合：

| 旗標 | 設了就共享…… |
|---|---|
| `CLONE_VM` | **記憶體**（整個 `mm_struct`） |
| `CLONE_FS` | 檔案系統資訊（根目錄、目前目錄） |
| `CLONE_FILES` | 開啟的檔案 |
| `CLONE_SIGHAND` | signal handler |
| `CLONE_THREAD` | 加入**同一個 thread group** |
| `CLONE_VFORK` | 父 process 等子 process `exec` 或結束才繼續 |

各種 system call 帶什麼旗標：

| 呼叫 | 旗標 | 結果 |
|---|---|---|
| **`fork`** | （無） | 全部**複製** |
| **`vfork`** | `CLONE_VFORK \| CLONE_VM` | **共享記憶體**、父 process 等待——所以不用複製 page table（課本說「為馬上 exec 最佳化」）。見 `kernel/fork.c:2780-2788` |
| **建 thread**（`pthread_create`） | `CLONE_VM \| CLONE_FS \| CLONE_FILES \| CLONE_SIGHAND \| CLONE_THREAD \| ...` | **幾乎全部共享** |
| init（§1.4） | `CLONE_FS` | 共享檔案系統資訊 |
| kthreadd（§1.4） | `CLONE_FS \| CLONE_FILES` | |

---

## 8. 三種 thread：user、I/O、kernel

`[33:31]` Linux 區分三種 thread：

| 種類 | 說明 |
|---|---|
| **User thread** | 主要在**使用者空間**跑，偶爾為了 system call 進 kernel 空間，再回來 |
| **I/O thread** | 專門存取 **I/O 裝置**。通常**優先權很低**（有時也可能比較高） |
| **Kernel thread** | **只住在 kernel 裡**。由 kernel 產生，**只做 kernel 的工作** |

`[34:31]` 共同點：

- 跟使用者空間的 `clone` 類似，kernel 有 **`kernel_clone`** 建 kernel thread。
- user thread 和 kernel thread 用**同一個 `task_struct`** 資料結構。
- `task_struct` 的 **`flags`**（一個位元向量）標記「這是 user thread 還是 kernel thread」——「**只差一個位元**」。

`[35:30]` 為什麼叫 kernel **thread** 而不是 kernel **process**？

> 「每個 thread 都是一個 process。但 kernel 裡的 process 都叫 **kernel thread**，因為**所有 kernel process 共享虛擬位址空間**——只有**一個** kernel 虛擬位址空間，不同的 kernel process 沒有各自的 kernel 虛擬位址空間。因為這種程度的共享，所以叫 thread。」

`[36:02]` kernel thread 做什麼？「**所有事**：週期性的記帳、timer、中斷處理、I/O 裝置介面……kernel 核心工作的一大部分都是 kernel thread 做的。」

【課本補充】課本 §3.2.3 補充：

- `task_struct.flags` 的 **`PF_KTHREAD`** 位元設了，就是 kernel thread（`include/linux/sched.h:1734`：`#define PF_KTHREAD 0x00200000 /* I am a kernel thread */`）。
- 「user thread」還有**第二種意思**：**純使用者層的 thread（pure-user threads）**——完全在使用者空間建立、管理、銷毀，**kernel 不知道它們的存在**。一個 process P 自己切分虛擬位址空間、給每個 user thread 一個 stack、建一個共用的 heap。但 kernel 只看到**一個** process P：P 被暫停，所有 user thread 都被暫停。不如 kernel 認得的原生 thread 有彈性，但 user threading library 已經很成熟，可以用 signal handler、kernel timer 和組合語言模擬很多 kernel 的工作（例如 timer interrupt 和收集 context）。

`[36:02]` 下一講：**context switch**。

---

## 9. 【實測】fork、COW、exec、PID 1 與 2

**① fork 回傳值與 copy-on-write**（課本 p.92 的 `x = 7` 例子，我寫的程式，已在本機執行）

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int x = 7;

int main(void)
{
	int pid = fork();

	if (pid == 0) {
		sleep(1);                      /* 等父行程先改 x */
		printf("child : pid=%d, fork() returned %d, x=%d, &x=%p\n",
		       getpid(), pid, x, (void *)&x);
	} else {
		x = 19;                        /* 父行程寫 x → 觸發 copy-on-write */
		printf("parent: pid=%d, fork() returned %d, x=%d, &x=%p\n",
		       getpid(), pid, x, (void *)&x);
		wait(NULL);
	}
	return 0;
}
```

輸出：

```text
child : pid=211087, fork() returned 0, x=7, &x=0x55c3e282f010
parent: pid=211086, fork() returned 211087, x=19, &x=0x55c3e282f010
```

| 觀察 | 印證 |
|---|---|
| 子 process 的 `fork()` 回傳 **0**；父 process 回傳 **211087**（= 子 process 的 PID） | §4.2：一次呼叫、兩次返回、回傳值不同（§4.4 的 `childregs->ax = 0`） |
| 父 process 把 `x` 改成 **19**，子 process 讀到的還是 **7** | 課本：「子 process 有自己的私有副本」 |
| **`&x` 一模一樣**（`0x55c3e282f010`），值卻不一樣 | **虛擬位址相同、實體 frame 不同**——父 process 寫入時觸發 **COW**，kernel 給父 process 一份新 frame、改了它的 page table，虛擬位址不變（§5） |

**② exec**（課本 Listing 3.10，加了 `wait` 和一行永遠不會執行的 `printf`）

```text
/tmp/claude-1000/.../scratchpad/proc
I am the parent: child = 211096
```

- 第一行是子 process 變成 **`pwd`** 之後印出的目前目錄。
- 「this line never runs if execv succeeds」**沒有**出現——`execv` 成功就不會返回。
- 父 process 那行在後面，是因為它的 `printf` 輸出在 buffer 裡、程式結束時才送出（stdout 接到管線時是整塊緩衝）。

**③ PID 1 和 PID 2**

```text
$ ps -o pid,ppid,cmd -p 1,2
    PID    PPID CMD
      1       0 /sbin/init splash
      2       0 [kthreadd]

$ ps -eo pid,ppid,comm | awk '$2==2' | head -4
      3       2 rcu_gp
      4       2 rcu_par_gp
      5       2 slub_flushwq
      6       2 netns
```

| 觀察 | 印證 |
|---|---|
| PID **1** 是 `/sbin/init`，PID **2** 是 `kthreadd` | §1.4 `rest_init()`：先 init、再 kthreadd |
| 兩個的父 process（PPID）都是 **0** | 都是 **idle（PID 0）** 生的 |
| `/sbin/init`（在 Ubuntu 上是 systemd 的連結）是使用者 process | §1.4：`kernel_init` 執行 `/sbin/init`，從 kernel 走到使用者空間 |
| `[kthreadd]` 名字有**方括號** | `ps` 用方括號表示 **kernel thread**（沒有使用者空間的命令列） |
| PID 3、4、5、6……的父 process 都是 **2** | 所有 kernel thread 都是 kthreadd 的子孫 |

---

## 10. 一頁速查表

### 10.1 三個特殊 process

| PID | 名稱 | 說明 |
|---|---|---|
| 0 | idle（swapper） | 開機 process 變成的；沒事做時跑；記帳用 |
| 1 | init | kernel 裡誕生，`exec` `/sbin/init` 走到使用者空間；所有使用者 process 的根 |
| 2 | kthreadd | 所有 kernel thread 的根 |

### 10.2 fork

| 項目 | 內容 |
|---|---|
| 做什麼 | 複製呼叫者，建一個子 process |
| 回傳值 | 父：子的 PID；子：**0** |
| 複製什麼 | 記憶體、狀態、PC（兩個都指向 fork 之後） |
| 原始碼 | `SYSCALL_DEFINE0(fork)` → `kernel_clone`，沒有任何 `CLONE_*` 旗標 |
| 子拿到 0 | `copy_thread` 裡 `childregs->ax = 0` |

### 10.3 Copy-on-write

| 步驟 | 內容 |
|---|---|
| 1 | fork 時只複製 page table，父子共享 frame |
| 2 | 父子的 page 都設成唯讀 |
| 3 | 任一方寫入 → fault（故意製造的） |
| 4 | handler 複製 frame、改映射、設回可寫 |
| P2 位元 | 區分「本來就唯讀」vs「因為 fork 才唯讀」；Linux 用 **VMA 的 `vm_flags`**（`is_cow_mapping`）判斷 |
| 為什麼開銷小 | 很多 page 不會被寫；子 process 常馬上 `exec`；懶惰複製分散開銷 |

### 10.4 exec

| 項目 | 內容 |
|---|---|
| 做什麼 | 把自己的映像換成另一個執行檔 |
| 清掉 | 記憶體映像；stack、heap 重設 |
| 載入 | 新執行檔的 text、data、BSS |
| 保留 | 開啟的檔案、網路連線（預設） |
| 開始 | 新 text section 的第一條指令 |
| 返回 | 成功就**不返回** |
| shell 怎麼跑指令 | fork → 子 process exec |

### 10.5 clone 與 copy_process

| 項目 | 內容 |
|---|---|
| clone | 通用版；fork、vfork、建 thread 都是它 |
| 決定共享什麼 | `CLONE_VM`、`CLONE_FS`、`CLONE_FILES`、`CLONE_SIGHAND`、`CLONE_THREAD`…… |
| thread | 共享幾乎所有東西，只有 stack 和 TLS 分開 |
| vfork | `CLONE_VFORK \| CLONE_VM`：不複製記憶體，為馬上 exec 最佳化 |
| copy_process 三步 | ① 複製 `task_struct`、kernel stack、架構狀態、時間歸零、配 CPU、配 PID ② 複製檔案、fs、signal、`mm`、namespace ③ 掛進父的 children、sibling、TGID/PGID/SID |
| `copy_mm` | `CLONE_VM` → 共享同一個 `mm`；否則 `dup_mm` 複製 |

### 10.6 Thread 種類

| 種類 | 說明 |
|---|---|
| User thread | 平常在使用者空間，system call 時進 kernel（同一個 thread 換模式） |
| I/O thread | 做 I/O，通常低優先權 |
| Kernel thread | 只在 kernel；`PF_KTHREAD`；沒有 `mm`；`ps` 顯示方括號 |
| Pure-user thread | kernel 不知道；library 自己管 |

### 10.7 本講看過的原始碼位置

| 檔案:行號 | 看什麼 |
|---|---|
| `init/main.c:686-732` | `rest_init()`：PID 1、PID 2、自己變 idle |
| `init/main.c:1578-1582` | init 執行 `/sbin/init` |
| `kernel/fork.c:2764-2776` | `fork` system call |
| `kernel/fork.c:2780-2788` | `vfork` system call |
| `arch/x86/kernel/process.c:192-196` | `childregs->ax = 0` |
| `mm/memory.c:922-929` | fork 時把 COW page 設成唯讀 |
| `include/linux/mm.h:1367-1370` | `is_cow_mapping()` |
| `kernel/fork.c:2013` 起 | `copy_process()` |
| `kernel/fork.c:2257-2290` | `copy_files/fs/sighand/mm/thread`、`alloc_pid` |
| `kernel/fork.c:2472-2489` | 掛進父的 children、thread group、`attach_pid` |
| `kernel/fork.c:1592-1606` | `copy_mm()`：`CLONE_VM` 共享 vs `dup_mm` |
| `include/uapi/linux/sched.h:11-19` | `CLONE_*` 旗標 |
| `include/linux/sched.h:1734` | `PF_KTHREAD` |

---

## 11. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **新 process 第一次被排程、從 `pt_regs` 恢復** | **第 12–13 講**：context switch（下一講） |
| **`mm` 與 `active_mm`、kernel thread 沒有 `mm`** | **第 12–13 講**：context switch 時借用 `active_mm` |
| **system call 時同一個 thread 換模式** | **第 14 講**：library call 與 system call |
| **COW 的 fault 處理** | **第 16 講**：exception（page fault） |
| **`SIGCHLD`、signal handler 被複製** | **第 17 講**：signal |
| **`clone` 建 thread、`pthread_create`** | **第 18 講**：多執行緒 |
| **kernel thread 做中斷處理、work queue** | **第 16 講**：softirq、threaded IRQ、work queue |
| **`sched_fork`：新 task 的排程初始化** | **第 24–26 講**：排程 |
| **反向映射（COW 後一個 frame 對應多個 page）** | **第 31 講**：reverse mapping |
| **`task_struct`、kernel stack 的配置器** | **第 33 講**：kernel 記憶體配置（slab） |

---

## 附：本講一句話總結

> **Linux 建立 process 的方式很怪：開機 thread 變成 idle（PID 0），生出 init（PID 1，靠 `exec` 從 kernel 走到使用者空間）和 kthreadd（PID 2）；之後每個新 process 都是先 `fork` 出一個完全一樣的分身——靠 copy-on-write 只複製 page table、有人寫入才真的複製 frame，子 process 靠 `childregs->ax = 0` 拿到回傳值 0——再用 `exec` 換成自己要跑的程式。`fork`、`vfork`、建 thread 其實都是同一個 `clone`，差別只在 `CLONE_VM` 等旗標決定哪些資源共享、哪些複製。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 11（Chapter 3, Part V）錄影逐字稿（YouTube 自動字幕，36:54）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§3.2，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0）結果。*
