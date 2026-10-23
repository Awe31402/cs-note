# Operating Systems: A Linux Kernel-Oriented Approach 第三講完整筆記：Timer Interrupt 與 x86（Chapter 2, Part II）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 3（Chapter 2, Part II）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（34:58，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§2.1.4 後半（Timer Interrupts、Inter-processor interrupts，p.30–33）與附錄 A.1–A.2（p.461–464）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
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
- [1. Timer interrupt：保證 OS 一定會跑](#1-timer-interrupt保證-os-一定會跑)
  - [1.1 問題：如果都沒有「感興趣的事件」？](#11-問題如果都沒有感興趣的事件)
  - [1.2 解法：主機板上的 timer chip](#12-解法主機板上的-timer-chip)
  - [1.3 Jiffy 與 64 位元計數器](#13-jiffy-與-64-位元計數器)
  - [1.4 【原始碼】`jiffies` 的宣告與遞增](#14-原始碼jiffies-的宣告與遞增)
  - [1.5 OS 和應用程式的破壞性干擾](#15-os-和應用程式的破壞性干擾)
  - [1.6 HZ：編譯時參數](#16-hz編譯時參數)
  - [1.7 【原始碼】HZ 的選項：預設其實是 250](#17-原始碼hz-的選項預設其實是-250)
  - [1.8 【課本補充】Inter-processor interrupt（IPI）](#18-課本補充inter-processor-interruptipi)
- [2. 第一節回顧與 ring 的現況](#2-第一節回顧與-ring-的現況)
- [3. 為什麼 kernel 需要組合語言](#3-為什麼-kernel-需要組合語言)
- [4. x86 的歷史與向下相容](#4-x86-的歷史與向下相容)
- [5. x86 的整數暫存器](#5-x86-的整數暫存器)
  - [5.1 從 a 到 rax：一路「延伸」](#51-從-a-到-rax一路延伸)
  - [5.2 sp、bp、si、di](#52-spbpsidi)
  - [5.3 r8–r15 與 8 位元子暫存器](#53-r8r15-與-8-位元子暫存器)
  - [5.4 【口頭】商業理由也是設計理由](#54-口頭商業理由也是設計理由)
- [6. 特殊暫存器：flags 與 ip](#6-特殊暫存器flags-與-ip)
  - [6.1 演進：flags → eflags → rflags](#61-演進flags--eflags--rflags)
  - [6.2 四個常用旗標](#62-四個常用旗標)
  - [6.3 【Q&A】8、16 位元的機器還在哪裡？](#63-qa816-位元的機器還在哪裡)
- [7. 兩種組合語言格式：Intel vs AT&T](#7-兩種組合語言格式intel-vs-att)
- [8. 浮點暫存器：一個堆疊](#8-浮點暫存器一個堆疊)
- [9. 一頁速查表](#9-一頁速查表)
- [10. 與後續講次的連結](#10-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

接著上一講。這一講兩件事：

| # | 主題 | 對應課本 |
|---|---|---|
| 1 | **Timer interrupt**：沒有任何事件時，OS 怎麼確定自己會被叫醒 | §2.1.4 後半 |
| 2 | **x86 組合語言**入門：暫存器、兩種語法、浮點堆疊 | 附錄 A |

---

## 1. Timer interrupt：保證 OS 一定會跑

### 1.1 問題：如果都沒有「感興趣的事件」？

`[00:32]` 上一講說 OS 只有三種方式會被叫醒：**interrupt、exception、system call**。課本把它們叫做 **events of interest（感興趣的事件）**。

現在想一個情況：

- 程式是**純計算**的，例如找質數 → 不會發 system call。
- 程式寫得很正確 → 沒有 exception；也沒有 page fault 之類的記憶體事件。
- 沒接硬體，或接了但沒產生中斷 → 沒有 interrupt。

`[01:00]` 假設 **8 個核心跑 8 個這種程式**，那麼：

- 沒有任何事件發生 → **OS 永遠不會被叫醒**。
- 程式可以一直跑下去，系統**完全沒有反應**。

`[03:00]` OS 沒機會做 bookkeeping（記帳、維護資料），也沒機會排程別的 process。教授說系統就會像**藍畫面或當機**一樣。

【課本補充】Point 2.1.1：OS 在很多方面就像一般程式，**需要一個核心才能跑**。如果沒有任何核心在跑 OS，OS 就完全沒有作用。「OS 在背景默默執行」是**天真的想法**——它是一個要被特殊機制叫醒的獨立程式。

### 1.2 解法：主機板上的 timer chip

`[01:30]` 系統**實際上不會**卡住，因為有一個**假的中斷來源（dummy source of interrupts）**：

```
   ┌─────── CPU ───────┐
   │  ┌──────┐┌──────┐ │          ┌────────────┐
   │  │ Core ││ Core │ │ ◄─────── │ Timer chip │  定期送 timer interrupt
   │  └──────┘└──────┘ │          └────────────┘
   │  ┌──────┐┌──────┐ │          （在主機板上，不在 CPU 裡）
   │  │ Core ││ Core │ │
   │  └──────┘└──────┘ │
   └───────────────────┘
```

- 一顆**獨立的 timer 晶片**，在**主機板**上，**不是 CPU 的一部分**。
- 它的工作：**每個 jiffy 產生一次中斷**。
- 這保證了：沒有人能**獨佔 CPU**，系統一定有反應。

`[03:33]` 【口頭】這是一個例子：**處理器和 OS 自己都不能自給自足**，需要一顆外部的 timer 晶片。

【課本補充】課本說：所有支援 OS 的平台**都必須有 timer 晶片**，它可以說是支援 OS 的機器裡**最不可或缺的部分**。timer interrupt 會被送到其中一個核心，發生 context switch，OS 開始執行。

### 1.3 Jiffy 與 64 位元計數器

`[02:00]` **Jiffy** = **timer interrupt 的間隔**。

| 時期 | jiffy 長度 | 每秒 timer interrupt 次數 |
|---|---|---|
| 很久以前 | 10 ms | 100 |
| 大約近十年（教授 / 課本） | **1 ms** | **1000** |

`[04:00]` 【口頭】原始碼去 elixir.bootlin.com 看，**最好看 6.2 版**。「雖然現在已經 6.8 了，我們會定期跟著 kernel 版本走，但這些部分不常變。」

`[04:00]` kernel 用一個 `unsigned long` 記錄**到目前為止的 jiffy 數**：

- 每來一次 timer interrupt，jiffy 計數 **+1**。
- 教授說 `unsigned long` 是 **64 位元**，所以**永遠不會溢位**。

【口頭】教授算了一下 `[04:30]`：

- 2³⁰ ≈ 10 億（billion），所以 2⁶⁴ = 2³⁰ × 2³⁰ × 2⁴ ≈ 10 億 × 10 億 × 16。
- 10¹⁸ 叫 **exa**（艾）；10¹⁵ 叫 peta。
- 所以 2⁶⁴ ≈ **16 exa** 個 jiffy。

我補算一下（不是教授講的）：2⁶⁴ ≈ 1.8 × 10¹⁹。以每秒 1000 次算，要 1.8 × 10¹⁶ 秒 ≈ **5.8 億年**才會溢位。

> ⚠️ `[05:02]`–`[05:32]` 逐字稿在這裡跳掉一段（從溢位直接跳到「application OS destructive interference」），中間內容**待補**。

### 1.4 【原始碼】`jiffies` 的宣告與遞增

課本 Listing 2.1 引用的是一行，實際在 v6.2.12 長這樣：

```c
// include/linux/jiffies.h:79-80
extern u64 __cacheline_aligned_in_smp jiffies_64;
extern unsigned long volatile __cacheline_aligned_in_smp __jiffy_arch_data jiffies;
```

**有兩個變數！**

| 變數 | 型別 | 說明 |
|---|---|---|
| `jiffies_64` | `u64` | **永遠是 64 位元**的計數器 |
| `jiffies` | `unsigned long` | **跟著機器字長**：64 位元機器上是 64 位元，**32 位元機器上只有 32 位元** |
| `volatile` | | 告訴編譯器：「這個值隨時會被別人（中斷）改掉，每次都要真的去記憶體讀」 |
| `__cacheline_aligned_in_smp` | | 在多核心系統上，讓它獨佔一條快取線，避免跟別的變數擠在一起互相干擾 |

所以教授說的「`unsigned long` 是 64 位元」只在 **64 位元機器**上成立。在 x86-64 上，這兩個其實是**同一塊記憶體**：

```c
// arch/x86/kernel/vmlinux.lds.S:48
jiffies = jiffies_64;
```

這是 **linker script**（連結器腳本），意思是「`jiffies` 這個名字就指向 `jiffies_64` 的位址」。

**32 位元機器怎麼辦？** 32 位元以 1000 Hz 計算，大約 **49.7 天**就會溢位。kernel 的對策很有趣：

```c
// include/linux/jiffies.h:169-173
/*
 * Have the 32 bit jiffies value wrap 5 minutes after boot
 * so jiffies wrap bugs show up earlier.
 */
#define INITIAL_JIFFIES ((unsigned long)(unsigned int) (-300*HZ))
```

| 部分 | 意思 |
|---|---|
| `-300*HZ` | 負的「300 秒的 jiffy 數」 |
| `(unsigned int)` | 轉成 32 位元無號數 → 變成一個**非常接近溢位**的大數 |
| 註解 | 「讓 32 位元的 jiffies 在**開機 5 分鐘後**就溢位，讓溢位 bug **早點現形**。」 |

也就是說，kernel **故意**不從 0 開始數，而是從「再 5 分鐘就溢位」的地方開始。有 bug 的程式碼開機 5 分鐘就會出事，不用等 49 天。

比較兩個 jiffy 值時，kernel 要求用這個巨集，它能正確處理溢位：

```c
// include/linux/jiffies.h:104-107
#define time_after(a,b)		\
	(typecheck(unsigned long, a) && \
	 typecheck(unsigned long, b) && \
	 ((long)((b) - (a)) < 0))
```

- 不直接寫 `a > b`，而是算 `b - a` 再轉成**有號數**看正負。
- 這樣就算中間溢位繞了一圈，只要兩個時間相差不到一半範圍，結果還是對的。

**誰把 jiffies 加 1？** 每次 timer interrupt 會走到：

```c
// kernel/time/tick-common.c:85-102
static void tick_periodic(int cpu)
{
	if (tick_do_timer_cpu == cpu) {
		raw_spin_lock(&jiffies_lock);
		write_seqcount_begin(&jiffies_seq);

		/* Keep track of the next tick event */
		tick_next_period = ktime_add_ns(tick_next_period, TICK_NSEC);

		do_timer(1);
		write_seqcount_end(&jiffies_seq);
		raw_spin_unlock(&jiffies_lock);
		update_wall_time();
	}

	update_process_times(user_mode(get_irq_regs()));
	profile_tick(CPU_PROFILING);
}
```

```c
// kernel/time/timekeeping.c:2289-2293
void do_timer(unsigned long ticks)
{
	jiffies_64 += ticks;
	calc_global_load();
}
```

| 行 | 意思 |
|---|---|
| 87 `tick_do_timer_cpu == cpu` | **只有一個指定的核心**負責加 jiffies。每個核心都有 timer interrupt，但不能每個都加，不然會加太多次 |
| 88–89 | 上鎖，避免別人同時讀到一半更新的值（鎖是第 5 章） |
| 94 `do_timer(1)` | → `jiffies_64 += 1`，**就是這一行**讓 jiffy 計數 +1 |
| 97 `update_wall_time()` | 更新「牆上時鐘」（真實世界時間） |
| 100 `update_process_times(user_mode(...))` | **每個核心都會做**：記錄目前 process 用了多少 CPU 時間，並檢查要不要換人（排程）。`user_mode()` 就是上一講看過的那個函式，判斷被打斷的是 user 還是 kernel |

第 100 行就是「timer interrupt 讓 OS 有機會排程」的實際入口。

### 1.5 OS 和應用程式的破壞性干擾

`[05:32]` timer interrupt 有代價。教授稱之為 **application–OS destructive interference（應用程式與 OS 的破壞性干擾）**：

- OS 每次進來執行，都會把**應用程式的快取線（cache lines）擠出快取**。
- 也可能把應用程式的 page 擠出主記憶體，但這**很少見**。
- 主要問題是**快取被擠掉**，這確實會發生，是**效能殺手**。

`[06:02]` Linux kernel 做了很多事來減輕：

- **OS 的足跡（footprint）要很小**。
- **interrupt handler 的程式碼要非常短**。
- 經常執行的 OS 部分叫 **top halves**（⚠️ 逐字稿寫成「top hubs」，依上下文判斷是第 4 章會講的 top half），足跡更要小。

【口頭】`[06:31]` 「所以**減少程式碼行數**是我們的首要準則。」

### 1.6 HZ：編譯時參數

`[06:31]` jiffy 長度由一個**編譯時參數**決定，叫 **HZ**。

- `HZ = 1000` → 每秒 1000 個 jiffy → 一個 jiffy = 1 ms。

【課本補充】課本說 jiffy 不能太長，不然系統反應慢；也不能太短，不然太多時間花在處理 timer interrupt 上。

#### 【口頭】作業：找出 GCC 的編譯參數

`[06:31]` 教授出的作業：找出**所有 GCC 編譯時參數**。舉例：

| 參數 | 意思 | 例子 |
|---|---|---|
| `-D` | 在編譯時定義一個巨集，**等於在程式裡寫 `#define`** | `-DHZ=1000` |
| `-I` | 加一個**找 header 檔的目錄** | 預設會找 `/include`、`/usr/include` 等系統目錄 |
| `-L` | 加一個**找函式庫的目錄** | |

`[07:30]` header 的兩種寫法：

```c
#include "myheader.h"   // 雙引號：先找目前目錄（或 -I 指定的目錄）
#include <stdio.h>      // 角括號：找系統目錄
```

`[08:00]` 「最好的方法是 `man gcc`，看所有可以指定的編譯參數。」

### 1.7 【原始碼】HZ 的選項：預設其實是 250

HZ 的選項定義在 Kconfig（kernel 的設定系統）：

```text
# kernel/Kconfig.hz:6-18
choice
	prompt "Timer frequency"
	default HZ_250
	help
	 Allows the configuration of the timer frequency. It is customary
	 to have the timer interrupt run at 1000 Hz but 100 Hz may be more
	 beneficial for servers and NUMA systems that do not need to have
	 a fast response for user interaction and that may experience bus
	 contention and cacheline bounces as a result of timer interrupts.
	 Note that the timer interrupt occurs on each processor in an SMP
	 environment leading to NR_CPUS * HZ number of timer interrupts
	 per second.
```

```text
# kernel/Kconfig.hz:43-56
	config HZ_1000
		bool "1000 HZ"
	help
	 1000 Hz is the preferred choice for desktop systems and other
	 systems requiring fast interactive responses to events.

endchoice

config HZ
	int
	default 100 if HZ_100
	default 250 if HZ_250
	default 300 if HZ_300
	default 1000 if HZ_1000
```

可選的值：

| 選項 | jiffy 長度 | Kconfig 說明 |
|---|---|---|
| `HZ_100` | 10 ms | 伺服器、多處理器、NUMA 系統 |
| **`HZ_250`** | **4 ms** | **預設值**。伺服器效能和互動反應的折衷 |
| `HZ_300` | 3.33 ms | 跟 250 一樣是折衷，而且能整除 PAL 和 NTSC 影片的幀率 |
| `HZ_1000` | 1 ms | 桌機、需要快速互動反應的系統 |

**跟教授 / 課本對照：**

- 教授和課本都說「現在 jiffy 是 1 ms」（HZ = 1000）。
- 但 v6.2.12 的**預設值是 `HZ_250`**（第 8 行）。說明文字也說「**習慣上**是 1000 Hz」，但預設給 250。
- 我查了這台電腦（Ubuntu，kernel 6.5.0）的設定 `/boot/config-6.5.0-21-generic`：**`CONFIG_HZ=250`**。
- 所以「1 ms」是**一種常見設定**，不是固定值。實際要看發行版怎麼編。

說明文字還有一個重點（第 15–17 行）：**每個處理器都有自己的 timer interrupt**，所以每秒總共有 **核心數 × HZ** 次。

> 補充（不是教授講的）：同一份設定檔裡還有 `CONFIG_NO_HZ_IDLE=y`，代表核心**閒置時會停掉週期性 tick**，省電。也就是說，「每個 jiffy 都一定有中斷」在現代 kernel 上也不是絕對的。tick 的細節後面講排程時可能再遇到。

### 1.8 【課本補充】Inter-processor interrupt（IPI）

教授這一講沒講，課本 §2.1.4 最後有（p.32–33）：

- OS 在某個核心上被叫醒後，常常要判斷某個 process 跑太久了、該換掉了。
- 如果要換的是**目前這個核心**上的 process：簡單，載入新 process 的 context 就好。
- 如果要換的是**別的核心**上的 process：**比較麻煩**。要送一個中斷給那個遠端核心，讓它開始跑 OS。這叫 **inter-processor interrupt（IPI，處理器間中斷）**。
- 幾乎所有多核心處理器都有這個功能，透過**中斷控制器（interrupt controller）**送。

> **Point 2.1.2（課本）**：kernel 在「目前」核心上跑 user process 很簡單；要在**遠端**核心上跑，就得用 IPI 讓遠端核心先切到 OS，再由那邊的 OS 換入 user process。

【原始碼】x86 送「請重新排程」IPI 的函式：

```c
// arch/x86/kernel/apic/ipi.c:65-72
void native_smp_send_reschedule(int cpu)
{
	if (unlikely(cpu_is_offline(cpu))) {
		WARN(1, "sched: Unexpected reschedule of offline CPU#%d!\n", cpu);
		return;
	}
	apic->send_IPI(cpu, RESCHEDULE_VECTOR);
}
```

| 行 | 意思 |
|---|---|
| 65 | 參數 `cpu`：要叫醒的**遠端核心**編號 |
| 67–70 | 那個核心已經離線？那就印警告、不送 |
| 71 | 透過 **APIC**（x86 的中斷控制器，第 15 講會講）送一個 IPI，中斷號碼是 `RESCHEDULE_VECTOR` |

```c
// arch/x86/include/asm/irq_vectors.h:70, 107
#define RESCHEDULE_VECTOR		0xfd
#define LOCAL_TIMER_VECTOR		0xec
```

- `RESCHEDULE_VECTOR` = `0xfd`：遠端核心收到這個號碼的中斷，就知道「該重新排程了」。
- 順便看到 `LOCAL_TIMER_VECTOR` = `0xec`：**每個核心自己的 timer interrupt** 用的號碼。

---

## 2. 第一節回顧與 ring 的現況

`[08:00]` 到這裡講完了「核心、暫存器、中斷」。學到的：

- 特權指令與特權暫存器，以及它們的用途
- system call、interrupt、exception
- 一般指令 vs 特權指令
- ring level 與 CPL 位元

`[08:30]` 【口頭】教授補充：**ring 的說法有點過時了**。

- ring 是硬體還不太支援虛擬化時的設計。
- 現在硬體為**虛擬機管理程式（VMM）** 定義了**額外的特權等級**，等級比以前更多。
- ring 1、2 **已經不放虛擬機了**，因為有更好的機制。
- 但 ring 1、2 還是有一些軟體在用，例如需要多一點特權的 **middleware（中介軟體）**。
- 虛擬化一開始是**純軟體**的，現在大家公認它會一直存在，所以**硬體和處理器都支援 OS 虛擬化**了。

> 對照上一講的筆記：上一講教授說「ring 1、2 給虛擬機」，這一講自己更新成「現在不是了」。

---

## 3. 為什麼 kernel 需要組合語言

`[09:30]` 接下來講一點 x86 組合語言（課本附錄 A），然後才進入虛擬記憶體和 I/O。

`[10:01]` kernel 有大約 **5–8%** 是組合語言（教授說「看你怎麼算」，他把包住組語的 C 程式碼也算進去）。

【口頭】教授的比喻：

> 如果把原始硬體看成一個東西，組合語言就是**套在它外面的一件外套（assembly jacket）**。其餘的 kernel 就能**不管底下的硬體**運作。

`[10:33]` 組合語言的好處：

| 好處 | 說明 |
|---|---|
| 低階控制 | 直接控制硬體 |
| 速度、效率 | |
| 程式碼小 | 用 C 寫要寫很多 |
| 精確 | 你**確切知道**給硬體的是什麼。連 C 都沒有這種控制力 |

`[11:00]` 「OS 執行中**敏感的部分**，一定需要組合語言。不要懷疑這一點。」不管 32 位元（x86）還是 64 位元（x86-64），只要寫 OS 的敏感部分，都需要組語。

> 對照上一講：`entry_SYSCALL_64` 就是這種「外套」——system call 一進 kernel，先由組語存好暫存器，才進 C 的 `do_syscall_64()`。

---

## 4. x86 的歷史與向下相容

`[11:32]` x86 是個**非常老**的指令集，名字就說明了它的歷史。

【口頭】教授說的年份：

| 晶片 | 教授說 |
|---|---|
| 8080 | 1980 |
| 8085 | 1985 |
| 8086 | 1986 |

> ⚠️ 年份不一致：課本附錄 A（p.461）說 **16 位元版在 1978 年**出現、**32 位元的 80386 在 1985 年**發布、**x86-64 在 2003 年**由 Intel 和 AMD 推出。8086 就是第一顆 16 位元 x86，所以教授說的「8086 是 1986 年」跟課本的「16 位元 1978 年」對不上。教授說「85 是因為 1985、86 是因為 1986」應是口誤，以課本為準。

演進：**8 位元 → 16 位元 → 32 位元 → 64 位元**。

`[12:02]` 設計 64 位元指令集時，要不要說「30 年前寫的程式我不跑了」？

- 這是**很大的商業風險**，幾乎沒有大廠這樣做。
- 客戶要被支援。就算軟體是新的，裡面可能有很久以前寫的 library，已經測試過、跑了很久，沒人想丟掉。

`[13:03]` 所以**向下相容（backward compatibility）在商業世界是美德**。技術人員可能不喜歡，所以像 **RISC-V** 這種新指令集就沒有這些包袱。

---

## 5. x86 的整數暫存器

### 5.1 從 a 到 rax：一路「延伸」

`[13:30]` 問題：每加一種寬度，就定義一組新暫存器嗎？**沒有**，工程師用了很有趣的解法——**把舊的加寬**。

| 位元數 | 暫存器名稱 | 命名由來 |
|---|---|---|
| 8 | `a`、`b`、`c`、`d` | 就是 A、B、C、D 四個 |
| 16 | `ax`、`bx`、`cx`、`dx` | **x = extended（延伸）**。【口頭】「那時候很多人以為世界到 16 位元就結束了」 |
| 32 | `eax`、`ebx`、`ecx`、`edx` | x 已經用掉了，所以前面加 **e = extended** |
| 64 | `rax`、`rbx`、`rcx`、`rdx` | 前面改成 **r** |

【口頭】`[16:30]` 教授說他不確定 **r** 代表什麼，「如果你能上網查到，那是個有教育意義的練習」。（⚠️ 這裡依教授原意保留為**待查**，不在筆記裡自己補答案。）

`[16:05]` 【口頭】「人們總是用同樣的方式想：今天的技術就是極限。而時間總是證明人們錯了。」

### 5.2 sp、bp、si、di

`[14:04]` 16 位元時加了幾個暫存器：

| 暫存器 | 全名 | 用途 |
|---|---|---|
| `sp` | stack pointer | 指向**堆疊頂端** |
| `bp` | base pointer / frame pointer | 指向**目前函式的活動記錄（activation block）** 的開頭 |
| `si` | source index | 迴圈的**起點** |
| `di` | destination index | 迴圈的**終點** |

`[14:30]` 為什麼要 `si`、`di`？因為 x86 是 CISC，目標是**把整個 for 迴圈塞進一條指令**：告訴指令從哪裡開始、到哪裡結束，它就自動用遞增的索引重複做同樣的計算。

（課本的說法是：「兩個索引暫存器，用來透過**單一指令**執行迴圈中的計算。」）

### 5.3 r8–r15 與 8 位元子暫存器

`[17:01]` 整理（課本 Fig A.2）：

```
 64 位元        32 位元      16 位元     8 位元
 rax ─────────  eax ──────  ax ──────  ah | al
 rbx ─────────  ebx ──────  bx ──────  bh | bl
 rcx ─────────  ecx ──────  cx ──────  ch | cl
 rdx ─────────  edx ──────  dx ──────  dh | dl
 rsp ─────────  esp ──────  sp
 rbp ─────────  ebp ──────  bp
 rsi ─────────  esi ──────  si
 rdi ─────────  edi ──────  di
 r8                          ← x86-64 新增
 ...
 r15
```

- 32 位元時只有 **8 個**暫存器；64 位元變成 **16 個**。
- 前 8 個是**歷史遺留（legacy）**，從舊的一路加寬而來。
- 後 8 個是 x86-64 **新加的**：`r8`–`r15`。
- 小的暫存器是大暫存器的**一部分**：`ax` 是 `eax` 的低 16 位元，`eax` 是 `rax` 的低 32 位元。課本說它們是「**虛擬地**」存在（作為大暫存器的一部分）。

`[18:31]` 為什麼要 16 個？「編譯需要暫存器，不然**記憶體流量太大**，不是好主意。」

`[18:31]` **8 位元還在**——「我們從不忘記歷史」：

- `ax` 拆成兩個 8 位元子暫存器：**`ah`（high，高 8 位元）** 和 **`al`（low，低 8 位元）**。
- `bh`/`bl`、`ch`/`cl`、`dh`/`dl` 同理。
- 「沒在用，但**可以用**。」

### 5.4 【口頭】商業理由也是設計理由

`[19:03]` 教授花了一段時間講這個觀念：

> 系統裡很多設計決定有**技術理由**，也有**商業理由**。工程師常常不重視商業理由。但設計產品時，世界不是只有技術。很多產品（他不點名）就是因為**商業部分做錯**而活不下去——做的人當時覺得自己是對的，但歷史判他們錯了。

> 所以他認為 x86 的做法是**對的**：所有寫過的程式碼都繼續支援，因為「你永遠不知道不支援的話，什麼東西會壞掉」。

`[20:00]` 例子：**銀行**到今天還有很多程式碼可以追溯到 80 年代，用 **COBOL** 寫的。「你們可能沒聽過 COBOL，但很多重要的銀行程式碼就是用它寫的。」還有一個專案叫 **GNU COBOL**（⚠️ 逐字稿寫成「gnu Cobalt」）。

結論：**歷史包袱很重要，所以做設計時要讓它面向未來（future-proof）。**

---

## 6. 特殊暫存器：flags 與 ip

### 6.1 演進：flags → eflags → rflags

`[20:32]` 特殊暫存器也一樣一路加寬（課本 Fig A.3）。它們**看不到**，或頂多**隱含地**看得到。

| 暫存器 | 16 位元 | 32 位元 | 64 位元 | 說明 |
|---|---|---|---|---|
| Flags（ALU 暫存器） | `flags` | `eflags` | `rflags` | 存算術指令的例外、上一次比較的結果 |
| Program counter | `ip` | `eip` | `rip` | x86 把 PC 叫 **instruction pointer** |

【課本補充】課本說：跟較新的 RISC 指令集不同，x86 的 **PC 不能直接存取**，程式設計師看不到。

### 6.2 四個常用旗標

`[21:30]` 跟 ARM 等指令集一樣：

| 旗標 | 名稱 | 什麼時候設 |
|---|---|---|
| **OF** | Overflow | 計算結果**超出數字系統範圍** |
| **CF** | Carry | 有**進位或借位** |
| **ZF** | Zero | 結果是 **0** |
| **SF** | Sign | 結果是**負的** |

`[22:07]` **比較指令其實就是減法**。比較 A 和 B：

```
  A − B = 0   →  ZF = 1   →  A == B
  A − B < 0   →  SF = 1   →  A <  B
  A − B > 0   →  都不設    →  A >  B
```

根據這些旗標，**後面的條件分支**決定要不要跳。

（在 `pt_regs` 裡，這些都存在 `flags` 欄位；上一講看過，`syscall` 指令會把 `rflags` 先放進 `r11`。）

### 6.3 【Q&A】8、16 位元的機器還在哪裡？

`[23:00]`

**學生問**：哪裡還找得到 8 位元和 16 位元的機器？（⚠️ 逐字稿開頭是「well in Intel's Museum」，看起來是有人先半開玩笑回答）

**教授答**：

- 他在幾個**科技博物館**看過。現在沒在用了。
- 但在**非常小的嵌入式應用**還有，嵌入式晶片種類非常多。例如**麥克風裡的晶片**、**助聽器裡的晶片**，可能用 16 位元運算。可以去看 **Texas Instruments** 或 **STMicroelectronics** 的晶片型錄（⚠️ 逐字稿寫成「STD micro Electronics」）。
- 結論：就筆電、桌機、伺服器來說，**8 位元基本上死了**；16 位元還有利基市場，但**可能也快消失**。

為什麼？`[24:00]` 16 位元晶片的暫存器是 16 位元，**能存取的記憶體有天生限制**：

- 位址範圍 0 到 2¹⁶ − 1。
- 2¹⁶ = **64 KB**，「連 1 MB 都不到，等於沒有」。
- 記憶體又小、又快、又便宜，所以 16 位元「我猜快退場了」，除非未來有比智慧手錶還小很多的處理器。

---

## 7. 兩種組合語言格式：Intel vs AT&T

`[25:02]` x86 程式碼有兩種寫法：

| | Intel 格式 | AT&T 格式 |
|---|---|---|
| 誰在用 | 教授的計組課本 | **GCC 預設**、**Linux kernel** |
| 目的地位置 | **最前面** | **最後面** |
| 暫存器寫法 | `eax` | `%eax`（前面加 `%`） |
| 常數寫法 | `4` | `$4`（前面加 `$`） |
| 運算元大小 | 從暫存器推斷 | 指令加字尾：**`l` = 32 位元、`q` = 64 位元** |

**累加器式（accumulator-based）設計** `[25:30]`：兩種格式都是**其中一個來源同時也是目的地**。

範例：`eax = eax + ebx`

```asm
; Intel：第 1 個運算元是來源也是目的地
add  eax, ebx

# AT&T：第 2 個運算元是來源也是目的地
addl %ebx, %eax
```

`[26:00]` 【口頭】「這不會傷效能」——進到處理器內部，會再分配更多暫存器，有另一套流程處理。

**記憶體運算元** `[26:32]`：x86 是 CISC，**記憶體可以直接當運算元**，不用先 load。

範例：把 `eax` 存到「`esp` + 4」的位址

```asm
; Intel
mov  [esp+4], eax

# AT&T
movl %eax, 4(%esp)
```

- `4(%esp)` 的意思：**`esp` 的內容 + 4**，當作記憶體位址。寫法比較像 RISC。
- **一條 x86 指令最多只能有一個記憶體運算元**，但這個記憶體運算元**可以同時是來源和目的地**。

`[29:32]` 「AT&T 格式就是**順序反過來**，加上要寫大小字尾。其他沒什麼大差別，只是要花時間習慣。」

（課本附錄 A.2 的例子，AT&T 格式：）

```asm
movq  $3, %rax        # rax = 3
movq  $4, %rbx        # rbx = 4
addq  %rbx, %rax      # rax = rax + rbx = 7
movq  %rax, 8(%rsp)   # 把 rax 存到記憶體 [rsp + 8]
```

> 對照上一講：`entry_SYSCALL_64` 裡的 `movq %rsp, %rdi`、`pushq %rax` 就是 AT&T 格式，`q` 表示 64 位元。

---

## 8. 浮點暫存器：一個堆疊

`[29:32]` x86 的浮點暫存器**很有意思**（課本 Fig A.4）：

- 有 **8 個**，叫 **`st0` 到 `st7`**。**st = stack（堆疊）**。
- 它們**排成一個堆疊**：`st0` **永遠是堆疊頂端**。

```
  推入一個新值之前：      推入之後：
  st0: A                 st0: 新值
  st1: B                 st1: A
  st2: C                 st2: B
  ...                    ...
  st7: H                 st7: G   （H 被擠出去）
```

`[30:31]` 所以**暫存器其實沒有固定的名字**，有名字的是「**堆疊裡的位置**」：

- 也**可以**直接用名字存取，例如 `st4`。
- 但一 push 或 pop，原本的 `st4` 就變成 `st5`（或 `st3`）了。課本說：一旦 push/pop，值和 `st5` 這個位置的連結就斷了。

`[31:31]` 每個暫存器是 **80 位元**，叫 **extended precision（延伸精度）**，比 double（64 位元）還精確。印成 float 時精度會降下來。

`[32:01]` 很多浮點指令是：**拿 `st0` 和 `st1` 算，結果推回堆疊頂端**。

**為什麼這樣設計？** `[32:30]`

1. **歷史**：以前處理器裡**沒有浮點運算單元**，浮點運算在另一顆**輔助處理器（coprocessor）** 上。在那上面用堆疊管理容易得多。
2. **程式碼大小**：教授說浮點輔助處理器是 **1987 年**、叫 **8087**（⚠️ 逐字稿寫成「FP 87」；課本說是 **80387**）。當時**減少程式碼大小**非常重要，程式碼還要送到另一顆輔助處理器去。用**隱含運算元**（你知道它在哪，但不用寫出來），只要寫「add」，它就自動拿頂端兩個加起來，結果推回堆疊。
3. **理論**：堆疊對應到**下推自動機（push-down automaton）**，有不少理論研究如何把計算編譯成堆疊模型。

`[34:00]` 「最大的好處是**程式碼大幅縮小**——這本來就是選 CISC 的理由。」

`[31:00]` 現在編譯器很聰明，知道怎麼處理。但**其他現代指令集都不這樣設計浮點**，而是跟整數暫存器一樣用名字存取。

【課本補充】課本說：堆疊模型通常是**非常簡單、容易寫程式的機器**採用的——指令只需要算術、load/store、push、pop。因為向下相容是必要條件，這個模型留到現在；有了快速硬體和編譯器技術，它沒有造成太大阻礙。

`[34:00]` 下一講：x86 的其他特性（尤其是**記憶體定址**）、看一個範例程式，然後很快進入**編譯、連結、載入**。

---

## 9. 一頁速查表

### 9.1 Timer interrupt

| 觀念 | 一句話 |
|---|---|
| **問題** | 沒有 interrupt / exception / system call → OS 永遠不會被叫醒 → 系統當掉 |
| **Timer chip** | 主機板上的獨立晶片，定期送中斷，保證 OS 會跑 |
| **Jiffy** | timer interrupt 的間隔 |
| **HZ** | 每秒幾個 jiffy；編譯時參數（Kconfig） |
| **教授 / 課本** | HZ = 1000，jiffy = 1 ms |
| **v6.2.12 預設** | `HZ_250`，jiffy = 4 ms；這台 Ubuntu 也是 250 |
| **jiffies** | `unsigned long`：64 位元機器 64 位元、32 位元機器 32 位元 |
| **jiffies_64** | 永遠 64 位元；x86-64 上兩者是同一個 |
| **INITIAL_JIFFIES** | 故意從「5 分鐘後溢位」開始，讓 bug 早現形 |
| **比較 jiffies** | 用 `time_after(a,b)`，不要直接 `>` |
| **破壞性干擾** | OS 每次進來都擠掉應用程式的快取 → handler 要很短 |
| **IPI** | 要在別的核心上換 process，得先送中斷叫醒那邊的 OS |

### 9.2 x86 暫存器

| 類別 | 內容 |
|---|---|
| 命名演進 | `a` → `ax`（x = extended）→ `eax`（e = extended）→ `rax` |
| 64 位元 | 16 個：`rax rbx rcx rdx rsp rbp rsi rdi r8`–`r15` |
| 8 位元 | `ah`/`al`、`bh`/`bl`… 還能用 |
| `sp` / `bp` | 堆疊頂端 / 目前函式的活動記錄 |
| `si` / `di` | 把整個迴圈塞進一條指令用的起點 / 終點 |
| PC | `ip` → `eip` → `rip`，不能直接存取 |
| Flags | `flags` → `eflags` → `rflags`；OF、CF、ZF、SF |
| 比較 | 就是減法；0 → ZF、負 → SF |
| 浮點 | `st0`–`st7` 排成堆疊，80 位元；為了程式碼小 |

### 9.3 Intel vs AT&T

| | Intel | AT&T（GCC、Linux） |
|---|---|---|
| 目的地 | 最前 | 最後 |
| 暫存器 | `eax` | `%eax` |
| 常數 | `4` | `$4` |
| 記憶體 | `[esp+4]` | `4(%esp)` |
| 大小 | 推斷 | `l`（32）、`q`（64） |
| `eax += ebx` | `add eax, ebx` | `addl %ebx, %eax` |

### 9.4 GCC 參數

| 參數 | 用途 |
|---|---|
| `-D名稱=值` | 等於 `#define` |
| `-I目錄` | 找 header 的目錄 |
| `-L目錄` | 找 library 的目錄 |

### 9.5 本講看過的原始碼位置

| 檔案:行號 | 看什麼 |
|---|---|
| `include/linux/jiffies.h:79-80` | `jiffies_64` 和 `jiffies` 的宣告 |
| `arch/x86/kernel/vmlinux.lds.S:48` | `jiffies = jiffies_64;` |
| `include/linux/jiffies.h:169-173` | `INITIAL_JIFFIES`：5 分鐘後溢位 |
| `include/linux/jiffies.h:104-107` | `time_after()` |
| `kernel/time/tick-common.c:85-102` | `tick_periodic()`：每個 tick 做什麼 |
| `kernel/time/timekeeping.c:2289-2293` | `do_timer()`：`jiffies_64 += ticks` |
| `kernel/Kconfig.hz` | HZ 選項，預設 250 |
| `arch/x86/kernel/apic/ipi.c:65-72` | `native_smp_send_reschedule()`：送 IPI |
| `arch/x86/include/asm/irq_vectors.h:70, 107` | `RESCHEDULE_VECTOR`、`LOCAL_TIMER_VECTOR` |

---

## 10. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **x86 記憶體定址、範例程式** | **第 4 講**：靜態與動態連結（下一講開頭會先講完 x86） |
| **`-I`、`-L`、header、library** | **第 4 講**：編譯、連結、載入；課本附錄 B |
| **`update_process_times()`：timer 驅動排程** | **第 24–26 講**：排程、Linux 排程（CFS） |
| **`jiffies_lock`、`raw_spin_lock`** | **第 20–23 講**：鎖、kernel 裡的並行 |
| **IPI、APIC、`RESCHEDULE_VECTOR`、`LOCAL_TIMER_VECTOR`** | **第 15 講**：x86 中斷架構 |
| **top half、interrupt handler 要短** | **第 16 講**：softirq、threaded IRQ、work queue |
| **context switch 由 timer 觸發** | **第 12–13 講**：context switch |
| **ring 1–2 的現況、硬體虛擬化支援** | 課本第 8 章。對應講次**待補** |
| **即時系統對 timer 精度的要求** | 另一門課 **Real Time Systems**：時鐘驅動排程、時鐘同步 |

---

## 附：本講一句話總結

> **OS 不會自己在背景跑，所以每台機器都要一顆 timer 晶片定期送中斷，每個 jiffy 把 OS 叫醒一次來記帳和排程（v6.2.12 預設每秒 250 次，不是常說的 1000 次）；而 kernel 最底層那件「組合語言外套」用的是 x86——一個為了向下相容，把 `a` 一路加寬成 `rax`、還把浮點暫存器排成堆疊的老指令集。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 3（Chapter 2, Part II）錄影逐字稿（YouTube 自動字幕，34:58）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§2.1.4 與附錄 A，以及 Linux kernel v6.2.12 原始碼整理。*
