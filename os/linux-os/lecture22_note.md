# Operating Systems: A Linux Kernel-Oriented Approach 第二十二講完整筆記：Kernel 裡的並行（Chapter 5, Part V）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 22（Chapter 5, Part V：Concurrency in the kernel）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（57:37，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§5.3–5.3.4（p.205–215）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
> **標記說明**：
> - `【口頭】`：教授在課堂上口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與教授回答
> - `【課本補充】`：課本有、但這堂課沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（`檔名:行號`），用來對照觀念
> - `【實測】`：在本機（Ubuntu、kernel 6.5.0、x86-64、20 個 CPU）實際編譯執行的結果
> - `⚠️ 逐字稿缺字`：自動字幕在這裡掉字或聽不清，內容依上下文或課本補上；也用來標示課堂說法與原始碼不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 這堂課要做什麼](#0-這堂課要做什麼)
- [1. 為什麼 kernel 的同步特別難](#1-為什麼-kernel-的同步特別難)
- [2. Spinlock：鎖住整顆 CPU](#2-spinlock鎖住整顆-cpu)
  - [2.1 拿著 spinlock 不准被搶占](#21-拿著-spinlock-不准被搶占)
  - [2.2 Preemption count](#22-preemption-count)
  - [2.3 【原始碼】`preempt_disable` / `preempt_enable`](#23-原始碼preempt_disable--preempt_enable)
  - [2.4 【原始碼】x86 的 per-CPU 計數與「反向」的 resched 位元](#24-原始碼x86-的-per-cpu-計數與反向的-resched-位元)
- [3. Spinlock 的程式碼](#3-spinlock-的程式碼)
  - [3.1 包裝：`raw_spinlock_t`](#31-包裝raw_spinlock_t)
  - [3.2 Ticket lock：銀行的號碼牌](#32-ticket-lock銀行的號碼牌)
  - [3.3 忙等的巨集：`smp_cond_load_relaxed`](#33-忙等的巨集smp_cond_load_relaxed)
  - [3.4 Unlock 與大小端](#34-unlock-與大小端)
  - [3.5 Acquire 與 release：半個 fence](#35-acquire-與-release半個-fence)
  - [3.6 Trylock：快速路徑](#36-trylock快速路徑)
  - [3.7 ⚠️ x86 其實不用 ticket lock](#37-️-x86-其實不用-ticket-lock)
  - [3.8 【實測】ticket lock 真的公平嗎](#38-實測ticket-lock-真的公平嗎)
- [4. Spinlock 與中斷](#4-spinlock-與中斷)
  - [4.1 `_irq` 與 `_irqsave` 版本](#41-_irq-與-_irqsave-版本)
  - [4.2 【原始碼】`spin_lock` 實際做了什麼](#42-原始碼spin_lock-實際做了什麼)
- [5. Kernel mutex](#5-kernel-mutex)
  - [5.1 跟 spinlock 的差別](#51-跟-spinlock-的差別)
  - [5.2 【原始碼】`struct mutex` 與 owner 欄位的小技巧](#52-原始碼struct-mutex-與-owner-欄位的小技巧)
  - [5.3 快速路徑、慢速路徑](#53-快速路徑慢速路徑)
  - [5.4 【課本補充＋原始碼】競爭條件：為什麼慢速路徑要再試一次](#54-課本補充原始碼競爭條件為什麼慢速路徑要再試一次)
- [6. 其他鎖：MCS / qspinlock、osq、qrwlock](#6-其他鎖mcs--qspinlockosqqrwlock)
- [7. Kernel semaphore](#7-kernel-semaphore)
- [8. Lockdep](#8-lockdep)
  - [8.1 像 `assert` 一樣的除錯工具](#81-像-assert-一樣的除錯工具)
  - [8.2 Lock depth](#82-lock-depth)
  - [8.3 四種狀態](#83-四種狀態)
  - [8.4 Lock inversion 與 context inconsistency](#84-lock-inversion-與-context-inconsistency)
  - [8.5 【原始碼】lockdep 官方文件的定義](#85-原始碼lockdep-官方文件的定義)
  - [8.6 用 hash 快取鎖鏈](#86-用-hash-快取鎖鏈)
- [9. 預告：RCU](#9-預告rcu)
- [10. 一頁速查表](#10-一頁速查表)
- [11. 與後續講次的連結](#11-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

[00:33] 講 Linux kernel 用的各種同步機制。

| 機制 | 誰持有 | 可以睡嗎 | 可以在中斷裡用嗎 |
|---|---|---|---|
| **spinlock** | CPU | 不行 | 可以 |
| **mutex** | task | 可以 | 不行 |
| **semaphore** | 沒有擁有者 | 可以 | 不行（`down` 不行） |
| **lockdep** | 除錯工具，檢查會不會 deadlock | — | — |

---

## 1. 為什麼 kernel 的同步特別難

[00:33] 整個 kernel 都是平行的。kernel 裡的 thread 同時在跑，會存取很多資料結構，這些都是**並行資料結構**。

[01:01] 兩個選擇：

1. 傳統的 **lock**。
2. **lock-free**：用 atomic fetch-and-increment、CAS 之類的指令。

[01:32] lock-free 難很多。單一生產者單一消費者就夠難了，再多就需要相當的專業。kernel 開發者做得到，但還是很複雜。

[01:32] 而且很多東西都能打斷 kernel thread。例如中斷來了：

```
kernel thread 跑到一半
   → 中斷 → interrupt handler
   → 排程器 → 可能去跑別的
   → ……很久以後才回來
```

[02:30] 所以任何演算法都要假設**兩道指令之間的延遲可以非常長**。

【課本補充】課本 §5.3：

- 因為 kernel thread 共享位址空間，所以稱它們為 thread。
- Linux 把多核心處理器叫 **SMP（symmetric multiprocessor，對稱多處理器）**：各計算單元對記憶體和 I/O 的存取大致相同。但也可能是 **NUMA（non-uniform memory access）**機器，不同核心存取不同記憶體模組的延遲不同。
- lock-free 的唯一缺點是可能 starvation，但實務上很少見，必要時可以提高快餓死的 thread 的優先權。只是很多資料結構很難寫成 lock-free，所以 kernel 大部分還是用 spinlock。

---

## 2. Spinlock：鎖住整顆 CPU

### 2.1 拿著 spinlock 不准被搶占

[02:30] kernel 裡最基本的鎖叫 **spinlock**，跟之前看過的鎖類似：

- 一直繞圈檢查 lock 變數，直到它空了。
- [03:00] 空了，所有想要的 thread 用 atomic 指令搶著把它從「空」設成「忙」。成功的進 critical section。
- 做完 unlock。

[03:00] 但 kernel 的 spinlock **多了一些限制**。教授說：spinlock 可以想成**鎖住 CPU**，不只是鎖住資源。

[03:31] 拿到 spinlock 後：

- thread **不能睡**。
- **不准搶占（preemption）**：不能換別的 thread 上來跑。
- [05:30] **不准遷移（migration）**：不能搬到別顆 CPU。

[04:01] 所以 spinlock **可以在中斷 context 裡用**，因為那裡本來就不准阻塞。

[04:01] 教授說 spinlock 是「很高優先權的鎖」：拿到它，這顆 CPU 上不准跑別的 thread；做完工作就放掉。不准再去呼叫排程器跑別的東西。

[04:01] 【口頭】這些很多是靠**程式設計師的紀律**，沒有絕對強制的方法。

[04:30] **為什麼不准搶占？** 會 deadlock：

```
Thread A 拿到 spinlock
   → 被搶占，換 Thread B 上來
   → Thread B 也要這把 spinlock → 一直空轉
   → Thread A 沒機會跑，就沒機會放 lock
   → 卡死
```

[05:02] 所以關掉搶占，讓拿著鎖的 thread 一定能把 critical section 做完、把鎖放掉。

【課本補充】課本補充：

- 新上來的 thread 可能優先權更高。它空轉等鎖，但拿鎖的 thread 優先權較低，永遠輪不到它跑。
- 拿鎖的 thread 不能阻塞、不能睡，所以效能上它**等同 lock-free 演算法**（假設 critical section 有限時間內做完，不做 I/O 之類可能無限久的事）。

> **課本 Point 5.3.1**：spinlock 實際上是**鎖住 CPU**，因為在它被放掉之前，這顆 CPU 上不准跑別的 thread。

⚠️ 課本說「這表示 critical section 裡要**關中斷**」。實際上一般的 `spin_lock()` **只關搶占、不關中斷**；要關中斷得用 `spin_lock_irq()` / `spin_lock_irqsave()`（見 §4）。

### 2.2 Preemption count

[06:02] 問題：spinlock 怎麼開關搶占？要告訴排程器「不要搶占我」。這必須**非常快**，因為 spinlock 很常用。

[06:02] kernel 用一個 **per-CPU 變數**（每顆 CPU 一份），就像之前的 `current` 指標，放在 **GS segment** 能存取的地方：

| 值 | 意思 |
|---|---|
| 0 | 允許搶占 |
| 非 0 | 不允許搶占 |

[07:02] 它像一個計數，不是開關：

- 關搶占 = **加 1**。
- 開搶占 = **減 1**。
- [07:30] 關了 10 次，就要開 10 次才會回到 0。

[09:02] 為什麼要計數？因為可以**巢狀**拿好幾把 spinlock：

```
spin_lock(A)    count: 0 → 1
spin_lock(B)    count: 1 → 2
spin_lock(C)    count: 2 → 3
spin_unlock(C)  count: 3 → 2
spin_unlock(B)  count: 2 → 1
spin_unlock(A)  count: 1 → 0   ← 這時才真的允許搶占
```

【課本補充】課本說這很像 semaphore：關了 n 次，要開 n 次。（Trivia 5.3.1）

### 2.3 【原始碼】`preempt_disable` / `preempt_enable`

`include/linux/preempt.h:201-205`

```c
#define preempt_disable() \
do { \
	preempt_count_inc(); \
	barrier(); \
} while (0)
```

`include/linux/preempt.h:218-223`（`CONFIG_PREEMPTION` 打開時）

```c
#define preempt_enable() \
do { \
	barrier(); \
	if (unlikely(preempt_count_dec_and_test())) \
		__preempt_schedule(); \
} while (0)
```

| 部分 | 說明 |
|---|---|
| `preempt_count_inc()` | [07:02] per-CPU 計數加 1 |
| `barrier()` | 教授說「跟 memory barrier 一樣，像 fence」。見下面 ⚠️ |
| `do { … } while (0)` | [07:30] kernel 寫巨集的慣用法：讓它像**一個區塊**，迴圈只跑一次 |
| `barrier()`（enable 開頭） | [08:00] 讓 critical section 裡的事不會被重排到這之後 |
| `preempt_count_dec_and_test()` | [08:30] 減 1，**看是不是變 0** |
| `unlikely(...)` | [09:31] 給分支預測器的提示：大部分時候減完還不是 0（還有別的鎖） |
| `__preempt_schedule()` | [09:31] 變 0 了 → 允許搶占了 → [10:01] 叫排程器，看有沒有別的 task 想在這顆 CPU 上跑 |

⚠️ 這裡的 `barrier()` 是 **compiler barrier**（`asm volatile("" ::: "memory")`），只阻止**編譯器**重排，**不產生任何 CPU 指令**。它不是第 19 講的硬體 fence。為什麼夠用？因為 preempt count 是**這顆 CPU 自己**的變數，只有自己會讀，不需要讓別的 CPU 看到順序。真正保護共享資料的硬體順序，由 spinlock 本身的 atomic 指令和 acquire/release 負責（§3.5）。

`preempt.h:239-243` 是沒開 `CONFIG_PREEMPTION` 的版本：只減 1，**不**呼叫排程器。本機 kernel 是 `CONFIG_PREEMPT_VOLUNTARY=y`、`CONFIG_PREEMPT_DYNAMIC=y`（開機時可選搶占模式）。

### 2.4 【原始碼】x86 的 per-CPU 計數與「反向」的 resched 位元

x86 把這個計數放在 per-CPU 的 `pcpu_hot` 結構裡：

`arch/x86/include/asm/preempt.h:78-97`

```c
static __always_inline void __preempt_count_add(int val)
{
	raw_cpu_add_4(pcpu_hot.preempt_count, val);
}

static __always_inline void __preempt_count_sub(int val)
{
	raw_cpu_add_4(pcpu_hot.preempt_count, -val);
}

/*
 * Because we keep PREEMPT_NEED_RESCHED set when we do _not_ need to reschedule
 * a decrement which hits zero means we have no preempt_count and should
 * reschedule.
 */
static __always_inline bool __preempt_count_dec_and_test(void)
{
	return GEN_UNARY_RMWcc("decl", pcpu_hot.preempt_count, e,
			       __percpu_arg([var]));
}
```

- `raw_cpu_add_4` 會編譯成一道 `add %gs:...` 指令。教授說的「GS segment 裡的 per-CPU 變數」就是這個。
- `dec_and_test` 是一道 `decl %gs:...`，再看 zero flag。

有個巧妙的設計：

`arch/x86/include/asm/preempt.h:12-28`

```c
/* We use the MSB mostly because its available */
#define PREEMPT_NEED_RESCHED	0x80000000

/*
 * We use the PREEMPT_NEED_RESCHED bit as an inverted NEED_RESCHED such
 * that a decrement hitting 0 means we can and should reschedule.
 */
#define PREEMPT_ENABLED	(0 + PREEMPT_NEED_RESCHED)

/*
 * We mask the PREEMPT_NEED_RESCHED bit so as not to confuse all current users
 * that think a non-zero value indicates we cannot preempt.
 */
static __always_inline int preempt_count(void)
{
	return raw_cpu_read_4(pcpu_hot.preempt_count) & ~PREEMPT_NEED_RESCHED;
}
```

- 最高位元是「**不需要**重新排程」（反向的旗標）：平常是 1。
- 有人需要搶占（例如更高優先權的 task 醒了）時，把它清成 0。
- 所以一道 `decl` 減到**整個 32 位元剛好是 0**，代表「計數歸零」**而且**「需要重新排程」兩個條件同時成立。一道指令同時檢查兩件事。
- 讀計數時把這個位元遮掉，其他程式碼看到的還是「0 = 可搶占」。

【課本補充】preempt count 不只記 spinlock，還分好幾段（`include/linux/preempt.h:26-29`）：

```
 *         PREEMPT_MASK:	0x000000ff
 *         SOFTIRQ_MASK:	0x0000ff00
 *         HARDIRQ_MASK:	0x000f0000
 *             NMI_MASK:	0x00f00000
```

低 8 位元是搶占關閉次數；再上去是 softirq、hardirq、NMI 的巢狀層數。所以 kernel 只看這一個數字，就知道自己是不是在中斷 context 裡（第 16 講的 `in_interrupt()` 就是看這些位元）。

---

## 3. Spinlock 的程式碼

### 3.1 包裝：`raw_spinlock_t`

[10:32] spinlock 在 `include/linux/spinlock.h`，依架構有不同版本。

[10:32] 教授說：在 x86 上用的是 **raw spinlock**，它包住一個**架構相關的 spinlock**。[11:02] 因為要用到架構定義的 atomic 指令，不同架構的 atomic 指令不同。

`include/linux/spinlock_types_raw.h:14-23`

```c
typedef struct raw_spinlock {
	arch_spinlock_t raw_lock;
#ifdef CONFIG_DEBUG_SPINLOCK
	unsigned int magic, owner_cpu;
	void *owner;
#endif
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	struct lockdep_map dep_map;
#endif
} raw_spinlock_t;
```

| 欄位 | 說明 |
|---|---|
| `raw_lock` | 真正的鎖，**架構相關** |
| `magic`, `owner_cpu`, `owner` | 除錯用：誰拿著、在哪顆 CPU |
| `dep_map` | **lock dependency map**，給 lockdep 用（§8） |

[11:02]–[12:30] 為什麼有 `dep_map`？deadlock 有兩種：

1. 循環等待（A 等 B、B 等 A）。
2. [11:32] 拿著鎖的 process **沒機會跑**。排程器跑別的，它不放鎖，別人一直空轉。[12:00] CPU 和 OS 以為它們在做有用的事，其實只是一直戳那個鎖變數。

所以要一個 map 記錄誰拿了什麼、誰在等什麼。

[13:30] 教授說這是一個好的**軟體設計模式**：通用的高層鎖，包住**架構相關**的低層鎖，再加上**只在除錯時才有**的結構。

### 3.2 Ticket lock：銀行的號碼牌

[14:00] spinlock 的核心是一個經典的 **ticket lock**（號碼牌鎖）。

[14:30] 教授的比喻：去銀行。

1. 先到號碼機按一下，拿到一張號碼牌，例如 **65**。
2. 螢幕上現在叫到 **55**。
3. 你去坐著等，直到螢幕顯示 65。
4. [15:00] 65 = 65，去櫃台辦事。

kernel 裡：號碼牌 = **ticket**；螢幕上的號碼 = **next**（教授叫它 to-be-served）。

[15:31] 兩個都是 **16 位元**，塞進**一個 32 位元**的無號整數：

```
 31            16 15             0
┌────────────────┬────────────────┐
│     ticket     │      next      │
│  （下一張號碼牌） │ （現在叫到幾號） │
└────────────────┴────────────────┘
```

[16:00] 所以 x86 上的 `spinlock_t` 可以就是一個 32 位元整數。（但見 §3.7。）

`include/asm-generic/spinlock.h:33-51`

```c
static __always_inline void arch_spin_lock(arch_spinlock_t *lock)
{
	u32 val = atomic_fetch_add(1<<16, lock);
	u16 ticket = val >> 16;

	if (ticket == (u16)val)
		return;

	/*
	 * atomic_cond_read_acquire() is RCpc, but rather than defining a
	 * custom cond_read_rcsc() here we just emit a full fence.  We only
	 * need the prior reads before subsequent writes ordering from
	 * smb_mb(), but as atomic_cond_read_acquire() just emits reads and we
	 * have no outstanding writes due to the atomic_fetch_add() the extra
	 * orderings are free.
	 */
	atomic_cond_read_acquire(lock, ticket == (u16)VAL);
	smp_mb();
}
```

| 行 | 說明 |
|---|---|
| 35 | [16:00] **atomic fetch-and-add**：拿到 lock 原本的值（存進 `val`），同時把 lock 加上 `1 << 16`。[16:32] `1 << 16` 剛好是 ticket 欄位的最低位元，所以等於 **ticket 加 1**。[17:01] 下一個人就拿到下一號 |
| 36 | [17:31] `val >> 16`：取出**我的號碼牌** |
| 38 | [18:00] `(u16)val`：轉型成 16 位元就是**低 16 位元** = next（現在叫到幾號） |
| 38-39 | [18:32] 我的號碼 = 現在叫到的號碼 → 直接拿到，返回 |
| 49 | [18:32] 否則就**忙等**：等到 lock 裡的 next 變成我的號碼 |
| 50 | [19:32] 拿到之後放一道 **memory barrier**（`smp_mb`）：拿鎖之後一定要 fence |

[19:00] 第 49 行很怪：傳給巨集的是一個**條件**，而且裡面用到一個沒定義過的變數 `VAL`。[19:32] 因為巨集是給**前置處理器**處理的，展開後才變成程式碼，`VAL` 是巨集裡面定義的。

第 41-48 行的註解解釋為什麼要多一道 `smp_mb()`：`atomic_cond_read_acquire` 只是 acquire（RCpc），作者懶得另寫一個更強的版本，乾脆補一道 full fence；反正這裡前面沒有未完成的寫入，多出來的排序「免費」。

【課本補充】課本：ticket lock **保證公平**，不會 starvation（先拿號碼的先服務）。

### 3.3 忙等的巨集：`smp_cond_load_relaxed`

[20:01] `atomic_cond_read_acquire(ptr, cond)`：在 `ptr` 上一直忙等，直到 `cond` 成立。它最後展開成：

`include/asm-generic/barrier.h:248-258`

```c
#define smp_cond_load_relaxed(ptr, cond_expr) ({		\
	typeof(ptr) __PTR = (ptr);				\
	__unqual_scalar_typeof(*ptr) VAL;			\
	for (;;) {						\
		VAL = READ_ONCE(*__PTR);			\
		if (cond_expr)					\
			break;					\
		cpu_relax();					\
	}							\
	(typeof(*ptr))VAL;					\
})
```

| 行 | 說明 |
|---|---|
| 249 | [20:30] 把指標轉成正確的型別，存成 `__PTR` |
| 250 | [21:01] 定義 `VAL`：指標指向的那個型別的值。如果 `ptr` 是 `int *`，`VAL` 就是 `int` |
| 251 | [21:30] `for (;;)`：用 `for` 寫的**無窮迴圈**（像 `while (1)`）。這就是「spin」的部分 |
| 252 | [22:02] `READ_ONCE`：老實地讀一次 lock 的值，放進 `VAL` |
| 253-254 | [22:02] 把 `VAL` 代進條件 `ticket == (u16)VAL`：號碼對了就跳出 |
| 255 | 沒對到就 `cpu_relax()`，再試 |
| 257 | [23:01] 巨集「回傳」值的標準寫法：`({ ... })` 最後一個運算式就是結果 |

⚠️ [22:31] 教授說 `cpu_relax()` 是「睡一段**隨機**的時間，減少對同一個鎖變數的爭搶」。實際上在 x86 上它是：

`arch/x86/include/asm/vdso/processor.h:11-19`

```c
static __always_inline void rep_nop(void)
{
	asm volatile("rep; nop" ::: "memory");
}

static __always_inline void cpu_relax(void)
{
	rep_nop();
}
```

`rep; nop` 就是 **`PAUSE` 指令**：告訴 CPU「我在空轉」，讓它稍微暫停幾十個 cycle、省電，並讓同一顆實體核心上的另一個 hyper-thread 多跑一點。**不是睡覺，也不是隨機時間。** 而且 ticket lock 本來就公平，不需要隨機退讓。

`smp_cond_load_acquire`（`barrier.h:270-275`）就是在上面的 relaxed 版本後面加一個 acquire 屏障。

### 3.4 Unlock 與大小端

`include/asm-generic/spinlock.h:63-69`

```c
static __always_inline void arch_spin_unlock(arch_spinlock_t *lock)
{
	u16 *ptr = (u16 *)lock + IS_ENABLED(CONFIG_CPU_BIG_ENDIAN);
	u32 val = atomic_read(lock);

	smp_store_release(ptr, (u16)val + 1);
}
```

[24:00] 教授建議**倒著讀**：

| 行 | 說明 |
|---|---|
| 68 | [24:00] `smp_store_release`：把值寫進指標位置，**內建 release fence**。[24:32] 寫的值是 `(u16)val + 1`：**next 加 1**。就像銀行服務完一位，螢幕上的號碼加 1 |
| 66 | 讀出目前整個 lock 的值 |
| 65 | [25:00] `ptr` 要指向 **next 那一半**。這裡有**大小端**問題 |

[25:31] 大小端（第 2 講）：

- **Little endian**：最低位的 byte 放在最低的位址。next 是低 16 位元，就在 lock 的開頭，`ptr = lock + 0`。
- **Big endian**：反過來，低 16 位元在高位址，`ptr = lock + 1`。

[26:01] `IS_ENABLED(CONFIG_CPU_BIG_ENDIAN)` 在 big endian 是 1，否則是 0。

[26:01] 指標運算：`p + 1` 不是加 1，是加 **`sizeof(*p)`**。`int *p` 加 1 是加 4；`double *` 加 4 是加 32。這裡是 `u16 *`，加 1 就是加 **2 bytes**。

[26:31] Linux 要能跑在各種機器上，所以要做這個修正。

### 3.5 Acquire 與 release：半個 fence

[27:00] 鎖的程式裡出現了 **acquire** 和 **release** 兩個字。它們是「**半個 fence**」：

| | 規則 | 用在 |
|---|---|---|
| **Acquire** | [27:00] 不管我**之前**發生什麼；但 critical section 和之後的一切，都要在 acquire 完成**之後**才完成。全世界要先看到「鎖被拿了」，才看到 critical section 的修改 | 拿鎖 |
| **Release** | [27:32] 不管我**之後**發生什麼；但我之前的一切都要先完成，我才能完成 | 放鎖：critical section 做的事要先讓大家看到，再讓大家看到鎖被放了 |

[28:01] 之前講的定理（第 19 講：沒有 data race ⟹ SC），把 full fence 換成**拿鎖用 acquire、放鎖用 release**，一樣成立。

[28:31] half fence 效能比較好，也比較容易實作。

（第 19 講筆記 §7.4 看過：`queued_spin_unlock` 也是 `smp_store_release`。）

### 3.6 Trylock：快速路徑

[28:31] **trylock** 像「快速通關」：走進銀行發現沒人排隊，就直接去櫃台。

`include/asm-generic/spinlock.h:53-61`

```c
static __always_inline bool arch_spin_trylock(arch_spinlock_t *lock)
{
	u32 old = atomic_read(lock);

	if ((old >> 16) != (old & 0xffff))
		return false;

	return atomic_try_cmpxchg(lock, &old, old + (1<<16)); /* SC, for RCsc */
}
```

| 行 | 說明 |
|---|---|
| 55 | [29:01] atomic 地讀出整個 lock |
| 57 | [29:01] `old >> 16` = ticket；`old & 0xffff`（16 個 1）= next |
| 57-58 | [29:30] 不相等 → 有人在排隊，**輪不到你**，回傳 false，放棄 |
| 60 | 相等 → 用 **CAS**：lock 還是 `old` 嗎？是的話換成 `old + (1 << 16)`（拿一張號碼牌）。[30:02] 一次完成。成功回 true、失敗回 false |

[30:32] 回 false 就要退回**慢速路徑**（一般的 `arch_spin_lock`）。

[30:32] Linux 的通用做法：**先試快速路徑，不行再走慢速路徑。**

【課本補充】課本：快速路徑處理「幾乎沒人搶」的情況，慢速路徑處理中到高度競爭。這是一個通用的模式，很多並行物件都能用。§4.2 的 `LOCK_CONTENDED` 就是這個模式。

### 3.7 ⚠️ x86 其實不用 ticket lock

教授和課本都說 x86 用這個 ticket lock。但 v6.2.12 的 x86 **不是**：

`arch/x86/Kconfig:119-120`

```
	select ARCH_USE_QUEUED_RWLOCKS
	select ARCH_USE_QUEUED_SPINLOCKS
```

`arch/x86/include/asm/spinlock.h:27`

```c
#include <asm/qspinlock.h>
```

本機 kernel 設定也是 `CONFIG_QUEUED_SPINLOCKS=y`。

- x86 用的是 **qspinlock**（queued spinlock，以 MCS lock 為基礎，§6）。從 Linux 4.2 起就是這樣。
- `include/asm-generic/spinlock.h` 這個「通用 ticket lock」是給**沒有自己實作**的架構用的。v6.2.12 裡在 `Kbuild` 用到它的有 **RISC-V、m68k、OpenRISC、Nios II**。
- 檔案開頭的註解（第 4-8 行）也說了：這是「'Generic' ticket-lock implementation」，前提是 `atomic_fetch_add()` 在競爭下有良好的前進保證；做不到的架構應該用 test-and-set lock。

ticket lock 的觀念完全正確，也很適合教學；只是 x86 實際跑的是 qspinlock。qspinlock 沒人搶時也是一個 32 位元整數加一次 CAS，跟 ticket lock 一樣便宜；有人搶時才排進 MCS 佇列。

### 3.8 【實測】ticket lock 真的公平嗎

把 `asm-generic/spinlock.h` 的 ticket lock 照抄到使用者空間（C11 atomics），跟一個 test-and-set（TAS）lock 比。8 個 thread 綁在 8 顆 CPU 上，1 秒內盡量反覆拿鎖、加計數、放鎖（`ticket.c`）：

```
TAS    total=9486874 shared=9486874  per-thread min=229037 max=2829059  max/min=12.35
  t0:2829059  t1:2639658  t2:229490  t3:229037  t4:865903  t5:934827  t6:882744  t7:876156
ticket total=8450882 shared=8450882  per-thread min=1056148 max=1056603  max/min=1.00
  t0:1056521  t1:1056452  t2:1056303  t3:1056330  t4:1056148  t5:1056167  t6:1056358  t7:1056603
```

- 兩種鎖都**正確**：`total == shared`，沒有少加。
- **TAS 很不公平**：最多的 thread 拿了 283 萬次，最少的只有 23 萬次，差 **12 倍**。剛放鎖的 thread（cache 裡還有那條 line）最容易再搶到。
- **Ticket lock 幾乎完全平均**：每個 thread 都是 105.6 萬次左右，差距不到 0.05%。先到先服務。
- 代價：ticket lock 總數少了約 11%。嚴格輪流，每次都要把 cache line 交給下一個人。

---

## 4. Spinlock 與中斷

### 4.1 `_irq` 與 `_irqsave` 版本

[31:00] CPU 在 spinlock 上忙等時，中斷還是可以處理，沒理由延遲中斷。

但有個問題：

```
kernel thread 拿著 spinlock L
   → 中斷來了，跑 interrupt handler
   → handler 也要 L → 一直空轉
   → handler 優先權比較高，kernel thread 永遠回不來放 L
   → deadlock
```

[31:32] 所以要用比較安全的 **IRQ 版本**：

- 拿鎖時**關中斷**，放鎖時**開中斷**。
- 用 `raw_spin_lock_irq` / `raw_spin_unlock_irq` 取代 `raw_spin_lock` / `raw_spin_unlock`。

（教授 [31:32] 先說反了「critical section 裡開中斷、放鎖時關」，馬上更正。）

[32:02] 【口頭】教授的比喻：「kernel 裡每一處，開了一扇門，就開了兩扇窗；但你從窗戶掉下去，腿會斷。」

[32:32] 還有一個問題：`raw_spin_unlock_irq` 會把中斷**全部打開**。但如果有些中斷**本來就是關的**呢？不應該把它們也打開。

所以有 **`irqsave` / `irqrestore`** 版本：

- 拿鎖時，**先存下**目前的中斷狀態，再關中斷。
- 放鎖時，**恢復**成原本存下的狀態，不是一律打開。

[33:00] 大家都應該用 `irqsave` / `irqrestore`。它們延遲比較高，但比較安全。確定不會有問題（例如根本不牽涉中斷）時，才用不安全的版本。

### 4.2 【原始碼】`spin_lock` 實際做了什麼

`include/linux/spinlock_api_smp.h:130-135`

```c
static inline void __raw_spin_lock(raw_spinlock_t *lock)
{
	preempt_disable();
	spin_acquire(&lock->dep_map, 0, 0, _RET_IP_);
	LOCK_CONTENDED(lock, do_raw_spin_trylock, do_raw_spin_lock);
}
```

| 行 | 說明 |
|---|---|
| 132 | **先關搶占**（§2.3）。注意：只關搶占，**不關中斷** |
| 133 | 告訴 lockdep「我要拿這把鎖」（沒開 lockdep 時是空的） |
| 134 | 先 trylock（快速路徑），失敗才 lock（慢速路徑）。見下面 |

`include/linux/lockdep.h:454-461`（開了 lock 統計時）

```c
#define LOCK_CONTENDED(_lock, try, lock)			\
do {								\
	if (!try(_lock)) {					\
		lock_contended(&(_lock)->dep_map, _RET_IP_);	\
		lock(_lock);					\
	}							\
	lock_acquired(&(_lock)->dep_map, _RET_IP_);			\
} while (0)
```

這就是教授說的「先快速路徑、再慢速路徑」，順便記錄「這把鎖被搶過」。沒開統計時（第 480-481 行）直接就是 `lock(_lock)`。

`irqsave` 版本：

`include/linux/spinlock_api_smp.h:104-113`

```c
static inline unsigned long __raw_spin_lock_irqsave(raw_spinlock_t *lock)
{
	unsigned long flags;

	local_irq_save(flags);
	preempt_disable();
	spin_acquire(&lock->dep_map, 0, 0, _RET_IP_);
	LOCK_CONTENDED(lock, do_raw_spin_trylock, do_raw_spin_lock);
	return flags;
}
```

- `local_irq_save(flags)`：把目前的中斷旗標（x86 的 `EFLAGS.IF`）**存進 `flags`**，再關中斷（`cli`）。
- 然後跟一般版本一樣。

放鎖：

`include/linux/spinlock_api_smp.h:146-153`

```c
static inline void __raw_spin_unlock_irqrestore(raw_spinlock_t *lock,
					    unsigned long flags)
{
	spin_release(&lock->dep_map, _RET_IP_);
	do_raw_spin_unlock(lock);
	local_irq_restore(flags);
	preempt_enable();
}
```

- 先放鎖。
- `local_irq_restore(flags)`：**恢復**成原本的中斷狀態。原本就關著的，就繼續關著。
- 最後才開搶占（可能在這裡被排程）。

順序是對稱的：拿鎖「關中斷 → 關搶占 → 拿鎖」，放鎖「放鎖 → 恢復中斷 → 開搶占」。

（第 20 講的 kernel semaphore `down()`/`up()` 用的就是 `raw_spin_lock_irqsave`。）

---

## 5. Kernel mutex

### 5.1 跟 spinlock 的差別

[33:31] **kernel mutex** 很像 pthread 的 mutex。

| | spinlock | kernel mutex |
|---|---|---|
| 持有者 | [34:01] **CPU** | **task** |
| 可以睡嗎 | 不行 | **可以** |
| 搶占 | 關掉 | 不關 |
| 額外限制 | 很多（不能睡、不能遷移……） | 沒有那些 |
| 能在中斷 context 用嗎 | 能 | **不能** |

[34:01] 教授說 spinlock 是「加了料的 lock」（會開關搶占），mutex 則是建立在 spinlock 提供的基礎上。

### 5.2 【原始碼】`struct mutex` 與 owner 欄位的小技巧

`include/linux/mutex.h:63-76`

```c
struct mutex {
	atomic_long_t		owner;
	raw_spinlock_t		wait_lock;
#ifdef CONFIG_MUTEX_SPIN_ON_OWNER
	struct optimistic_spin_queue osq; /* Spinner MCS lock */
#endif
	struct list_head	wait_list;
#ifdef CONFIG_DEBUG_MUTEXES
	void			*magic;
#endif
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	struct lockdep_map	dep_map;
#endif
};
```

| 欄位 | 說明 |
|---|---|
| `owner` | [34:01] 擁有者。概念上是 `task_struct *` |
| `wait_lock` | [34:31] 一把 spinlock，保護下面的等待串列 |
| `osq` | 樂觀空轉用的 MCS 鎖（§6） |
| `wait_list` | [34:31] 想拿但沒拿到的 task 串列 |
| `dep_map` | [35:02] lockdep 用 |

[34:31] 注意 `wait_lock` 用的是 `raw_spinlock_t`（通用包裝），不是 `arch_spinlock_t`，因為這部分程式要通用。

【課本補充】為什麼 `owner` 是 `atomic_long_t` 而不是 `task_struct *`？這是一個小技巧：

`kernel/locking/mutex.c:59-72`

```c
/*
 * @owner: contains: 'struct task_struct *' to the current lock owner,
 * NULL means not owned. Since task_struct pointers are aligned at
 * at least L1_CACHE_BYTES, we have low bits to store extra state.
 *
 * Bit0 indicates a non-empty waiter list; unlock must issue a wakeup.
 * Bit1 indicates unlock needs to hand the lock to the top-waiter
 * Bit2 indicates handoff has been done and we're waiting for pickup.
 */
#define MUTEX_FLAG_WAITERS	0x01
#define MUTEX_FLAG_HANDOFF	0x02
#define MUTEX_FLAG_PICKUP	0x04

#define MUTEX_FLAGS		0x07
```

- `task_struct` 至少對齊到 cache line（64 bytes），所以指標的**最低幾個位元一定是 0**。
- 拿這 3 個位元來存狀態：有沒有人在等、要不要直接交給排第一的人……
- [37:31] 教授：把多個欄位**塞進同一個 32/64 位元字組**，就能用**一次 atomic 操作**同時改它們。（ticket lock 塞 ticket 和 next 也是同一招。）

### 5.3 快速路徑、慢速路徑

`kernel/locking/mutex.c:281-287`

```c
void __sched mutex_lock(struct mutex *lock)
{
	might_sleep();

	if (!__mutex_trylock_fast(lock))
		__mutex_lock_slowpath(lock);
}
```

| 行 | 說明 |
|---|---|
| 283 | [35:30] `might_sleep()`：如果在 **atomic（中斷）context** 呼叫，就印出整個**呼叫堆疊**，看看是怎麼走到這裡的。mutex 可能會睡，中斷裡不准睡 |
| 285 | [35:30] 先試快速路徑 |
| 286 | 不行就走慢速路徑 |

快速路徑：

`kernel/locking/mutex.c:166-175`

```c
static __always_inline bool __mutex_trylock_fast(struct mutex *lock)
{
	unsigned long curr = (unsigned long)current;
	unsigned long zero = 0UL;

	if (atomic_long_try_cmpxchg_acquire(&lock->owner, &zero, curr))
		return true;

	return false;
}
```

[37:01] **一次 CAS**：`owner` 是 0（沒人拿、旗標也都是 0）嗎？是的話換成 `current`（我自己）。[38:00] 成功就拿到了。注意是 `_acquire` 版本（§3.5）。

放鎖的快速路徑也是一次 CAS，用 `_release`：

`kernel/locking/mutex.c:177-182`

```c
static __always_inline bool __mutex_unlock_fast(struct mutex *lock)
{
	unsigned long curr = (unsigned long)current;

	return atomic_long_try_cmpxchg_release(&lock->owner, &curr, 0UL);
}
```

`owner` 剛好等於我（沒有任何旗標，也就是沒人在等）→ 換成 0，結束。有人在等時 `MUTEX_FLAG_WAITERS` 被設了，CAS 會失敗，改走慢速路徑去叫醒人。

[36:02]–[38:32] **慢速路徑**：

1. 拿 `wait_lock`（spinlock）。
2. 把 `current` 加到等待串列。
3. 把狀態設成 **`TASK_UNINTERRUPTIBLE`**（不可中斷的睡眠）。
4. 呼叫排程器，去跑別的。

[36:31] 為什麼用 uninterruptible？它不處理 signal。反正這是 kernel thread，本來就不會收 signal。[36:31] 這對排程器也好：它知道這個 task 唯一會發生的好事就是**被叫醒**。

⚠️ 實際上 `mutex_lock()` 傳的是 `TASK_UNINTERRUPTIBLE`；另外有 `mutex_lock_interruptible()`（可被 signal 打斷）和 `mutex_lock_killable()`。而且使用者 process 進了系統呼叫，在 kernel 裡也會用 mutex，那時它就是可能收到 signal 的。

[39:03] 放鎖時：擁有者檢查等待串列，有人就叫醒一個。[39:30] 把它移出串列、讓它成為新擁有者、叫醒它。

[39:03] 教授的總結：「進來時試著**硬闖**，闖不進去就排隊。擁有者離開時，看看有沒有人在排，叫醒一個。」

【課本補充】mutex 的「睡」不是馬上睡：只是把 task 狀態設成睡眠狀態，task 其實還在跑；接著呼叫排程器，排程器不會選睡眠狀態的 task，等它被換出去，才真的開始睡。

【原始碼】v6.2.12 在快速路徑和慢速路徑之間其實還有一段 **optimistic spinning（樂觀空轉）**（`CONFIG_MUTEX_SPIN_ON_OWNER=y`，本機也開了）：如果擁有者**正在別顆 CPU 上跑**，它應該很快就會放鎖，那就先空轉一下，不急著睡。這時多個空轉者用 `osq`（一個 MCS 鎖）排隊，避免大家一起戳 `owner`。擁有者被換下 CPU，才去睡。

### 5.4 【課本補充＋原始碼】競爭條件：為什麼慢速路徑要再試一次

本講沒講，課本有。如果不小心，會有這個競爭：

1. T_w 試快速路徑失敗（T_l 拿著鎖）。
2. T_w 準備走慢速路徑、把自己加進等待串列。
3. T_w **被延遲**了。
4. T_l 放鎖，檢查等待串列，**是空的**，走了。
5. T_w 醒來，把自己加進串列。
6. **沒人會來叫醒它**，永遠卡在串列裡。

這就是第 21 講的 **lost wakeup**。

解法：慢速路徑**拿著 `wait_lock` 再試一次**。v6.2.12 的程式：

`kernel/locking/mutex.c:617-626`

```c
	raw_spin_lock(&lock->wait_lock);
	/*
	 * After waiting to acquire the wait_lock, try again.
	 */
	if (__mutex_trylock(lock)) {
		if (ww_ctx)
			__ww_mutex_check_waiters(lock, ww_ctx);

		goto skip_wait;
	}
```

`kernel/locking/mutex.c:635-648`（節錄）

```c
	if (!use_ww_ctx) {
		/* add waiting tasks to the end of the waitqueue (FIFO): */
		__mutex_add_waiter(lock, &waiter, &lock->wait_list);
	} else {
		...
	}

	set_current_state(state);
```

`kernel/locking/mutex.c:650-679`（節錄）

```c
	for (;;) {
		bool first;

		/*
		 * Once we hold wait_lock, we're serialized against
		 * mutex_unlock() handing the lock off to us, do a trylock
		 * before testing the error conditions to make sure we pick up
		 * the handoff.
		 */
		if (__mutex_trylock(lock))
			goto acquired;

		/*
		 * Check for signals and kill conditions while holding
		 * wait_lock. This ensures the lock cancellation is ordered
		 * against mutex_unlock() and wake-ups do not go missing.
		 */
		if (signal_pending_state(state, current)) {
			ret = -EINTR;
			goto err;
		}
		...
		raw_spin_unlock(&lock->wait_lock);
		schedule_preempt_disabled();
```

步驟對照課本：

| 課本步驟 | 原始碼 |
|---|---|
| 1. 拿 `wait_lock` | 617 |
| 2. 再試一次快速路徑，成功就放 `wait_lock` 返回 | 621-626 |
| 3. 否則把自己加到 `wait_list`（FIFO，加到尾端） | 637 |
| 4. 設成睡眠狀態 | 648 |
| 5. 放 `wait_lock`，去睡 | 678-679 |

為什麼這樣就安全？放鎖的人**也要拿 `wait_lock`** 才能查等待串列。T_w 拿著 `wait_lock` 時，要嘛 trylock 成功，要嘛已經把自己放進串列才放 `wait_lock`。所以放鎖的人一拿到 `wait_lock`，一定看得到 T_w。如果它在 T_w 睡著前就叫醒它，因為 T_w 已經 `set_current_state` 了，`schedule()` 會直接返回（第 21 講 `wait_event` 的「先掛 queue、再檢查、再睡」同一個道理）。

---

## 6. 其他鎖：MCS / qspinlock、osq、qrwlock

[39:30] kernel 裡的鎖種類一直在增加。

**MCS lock / queued spinlock（qspinlock）**：

- [39:30] 很有效率。建一個節點串列（跟 mutex 類似），現在的 process 加到**串列尾端**。
- [40:01] 用大量 **lock-free** 程式管理這個串列，串列本身**沒有** spinlock 保護。很 scalable。
- [40:01] 最大的好處：**對 cache 很友善**，[40:31] 消除 **cache line bouncing**（同一條 cache line 在核心之間彈來彈去）。
- [40:31] 怎麼做到？每個等待者都在**自己的節點**的變數上空轉（本地副本）。輪到它時，前一個人改它節點裡的變數，那條 cache line 才傳過來。[41:00] 不會有 false sharing（偽共享），大部分動作都在本地副本上。

【課本補充】課本 Figure 5.17：

```
   node ──▶ node ──▶ node ──▶ node
                               ▲
                              tail
```

- 拿鎖分兩步：讓目前的 tail 節點指向新節點、再把 tail 指向新節點。這兩步要看起來 atomic。
- 每個節點裡有一個 lock 變數，等待者只在自己的節點上空轉。
- 放鎖時，把**下一個節點**的 lock 變數設成「空」，下一個人就拿到了。

kernel 的節點：

`kernel/locking/mcs_spinlock.h:18-22`

```c
struct mcs_spinlock {
	struct mcs_spinlock *next;
	int locked; /* 1 if lock acquired */
	int count;  /* nesting count, see qspinlock.c */
};
```

qspinlock 檔案開頭的說明：

`kernel/locking/qspinlock.c:32-48`

```c
/*
 * The basic principle of a queue-based spinlock can best be understood
 * by studying a classic queue-based spinlock implementation called the
 * MCS lock. A copy of the original MCS lock paper ("Algorithms for Scalable
 * Synchronization on Shared-Memory Multiprocessors by Mellor-Crummey and
 * Scott") is available at
 *
 * https://bugzilla.kernel.org/show_bug.cgi?id=206115
 *
 * This queued spinlock implementation is based on the MCS lock, however to
 * make it fit the 4 bytes we assume spinlock_t to be, and preserve its
 * existing API, we must modify it somehow.
 *
 * In particular; where the traditional MCS lock consists of a tail pointer
 * (8 bytes) and needs the next pointer (another 8 bytes) of its own node to
 * unlock the next pending (next->locked), we compress both these: {tail,
 * next->locked} into a single u32 value.
 */
```

- 傳統 MCS lock 要一個 tail 指標（8 bytes）加節點的 next 指標（又 8 bytes）。
- 但 kernel 假設 `spinlock_t` 只有 **4 bytes**（很多結構裡都嵌了 spinlock，不能變大）。
- 所以 qspinlock 把 {tail, next->locked} **壓進一個 u32**。tail 不存指標，存「哪顆 CPU 的第幾個節點」的編號（每顆 CPU 預先配好幾個節點）。
- 又是「把多個欄位塞進一個字組」這招。

[41:00] 其他：

| 鎖 | 說明 |
|---|---|
| **osq lock** | MCS 的變體。mutex 的樂觀空轉用它（§5.3） |
| **qrwlock** | kernel 自己的 reader-writer lock（課本：偏好讀者） |

[41:00] 全部都在 **`kernel/locking/`** 目錄。v6.2.12 裡有：

```
lockdep.c  mcs_spinlock.h  mutex.c  osq_lock.c  percpu-rwsem.c  qrwlock.c
qspinlock.c  rtmutex.c  rwsem.c  semaphore.c  spinlock.c  ww_mutex.h ...
```

---

## 7. Kernel semaphore

[41:31] kernel 的 semaphore 跟使用者空間的**差不多**：

- [42:00] 一個 **count**（核心概念）、一個**等待串列**、一把 **spinlock**（保護 count，共享變數一定要有鎖，否則有 data race）。
- **`down`** = wait；**`up`** = post。

[42:33] `down`：count 已經是 0 → 把目前的 task 設成睡眠狀態（通常是 **uninterruptible**，kernel 裡不想處理 signal，那是使用者空間的事）。[43:01] count 不會變成負的，所以只能等。

[43:32] `up`：如果有等待中的 thread，把它的狀態設成 **running**，再呼叫排程器。

[43:32] 通則：kernel 裡要叫醒一個 thread，就是**改它的狀態**。改成 running 只代表它**可以跑了**，但還沒在跑，甚至還沒被考慮。[44:00] 要等排程器的 `schedule()` 被呼叫。

【課本補充】課本：在 kernel 裡直接改 task 狀態很容易；使用者空間做不到，什麼都要系統呼叫。所以 kernel 的這些機制快很多。

（kernel semaphore 的原始碼，包括 `down()`、`up()` 和「有人等就直接交棒、count 不動」的最佳化，在第 20 講筆記 §2.2。）

---

## 8. Lockdep

### 8.1 像 `assert` 一樣的除錯工具

[12:00] **lockdep**（lock dependency，鎖依賴）記錄誰拿了什麼、誰在等什麼。

- [12:00] 能**抓到** deadlock。
- 也能做 **deadlock avoidance**：拿鎖**之前**，系統告訴你這樣會不會 deadlock。會的話你可以退讓。
- [12:30] 這是**除錯時才開的功能**，正式版不開。

【實測】本機（Ubuntu 的發行版 kernel）：`# CONFIG_PROVE_LOCKING is not set`。沒開。

[12:30]–[13:30] 教授說它像 **`assert`**（`assert.h`）：

- 寫 C/C++ 時應該到處放 `assert`：「這裡應該成立這個條件」。
- 這樣有 bug 時，**bug 的來源和被發現的地方很近**；不然可能很久以後才 segmentation fault，很難追。

[44:00] lockdep 是一種**避免 deadlock** 的機制：拿鎖前檢查「拿了會不會造成 deadlock」，不一定是馬上，可能是**將來**。[44:30] 要保守，避免可能導致 deadlock 的情況。除錯模式下印出「有危險的事發生了」，程式設計師就能改 kernel。

[45:00] 已知一種 deadlock：**循環等待**。但這**不是唯一**的一種。還有一種來自 **context inconsistency（context 不一致）**。[45:30] 教授說：「名字聽起來很高級，其實不是。」

[45:30] 每次拿鎖都會呼叫 lockdep 的 `lock_acquire`，檢查拿這把鎖會不會（或可能會）出問題。

### 8.2 Lock depth

[46:00] 先做幾個簡單檢查。第一個是 **lock depth**：目前這個 task 同時拿著幾把鎖，要有上限。

[46:00] 教授的理由：假設資源只夠拿 10 把，要拿第 11 把時可能記憶體不夠之類的，process 卡住；卡住就放不掉手上的鎖。[47:00] 所以限制一個 task 同時能拿的鎖數。

`include/linux/sched.h:1164`

```c
# define MAX_LOCK_DEPTH			48UL
```

`kernel/locking/lockdep.c:5066-5074`

```c
	if (unlikely(curr->lockdep_depth >= MAX_LOCK_DEPTH)) {
		debug_locks_off();
		print_lockdep_off("BUG: MAX_LOCK_DEPTH too low!");
		printk(KERN_DEBUG "depth: %i  max: %lu!\n",
		       curr->lockdep_depth, MAX_LOCK_DEPTH);

		lockdep_print_held_locks(current);
		debug_show_all_locks();
		dump_stack();
```

- 上限是 **48** 把。
- 超過時印「`BUG: MAX_LOCK_DEPTH too low!`」，**關掉 lockdep**，印出手上所有的鎖和堆疊。
- 從訊息看，主要理由是 lockdep 自己要為每個 task 記錄「手上拿著的鎖」（`task_struct` 裡一個固定 48 格的陣列），格子用完就沒辦法繼續追蹤。課本也說限制是為了讓 kernel 和驗證系統的複雜度有上限。

[47:00] 第二個檢查：**驗證整條拿鎖的鏈**（一連串的拿、放、拿、放），看起來安不安全。[47:33] spinlock、mutex、reader-writer lock 都要驗證。

### 8.3 四種狀態

[47:33]–[49:31] 每一次**拿鎖**可以標成四種狀態：

| 狀態 | 教授的定義（拿鎖那一刻的環境） |
|---|---|
| **softirq-safe** | 拿鎖時 softirq 是**關**的 |
| **softirq-unsafe** | 拿鎖時 softirq 是**開**的 |
| **hardirq-safe** | 拿鎖時中斷是**關**的 |
| **hardirq-unsafe** | 拿鎖時中斷是**開**的 |

[48:31] 教授強調：不是**鎖**是 softirq-safe，是**這一次拿鎖**是 softirq-safe。只是記錄「拿鎖時的環境是什麼」。

[50:01] 如果是 softirq-unsafe（softirq 開著），那 hardirq 一定也開著（hardirq 優先權更高），所以**也是 hardirq-unsafe**。反過來不成立。

⚠️ lockdep 官方文件的定義跟這個不太一樣，見 §8.5。

### 8.4 Lock inversion 與 context inconsistency

[50:01] 先找簡單的 deadlock：不能有 A 等 B、B 等 A。

[50:01] kernel 強制的規則**很保守**：遵守它們可能讓你寫程式比較不自由，但保證不會 deadlock。

**規則 1：不能有 lock inversion（鎖反轉）**

[50:30] 鏈中如果看到「先 A 後 B」，又看到「先 B 後 A」，就很危險。代表系統裡有 process 不遵守 A→B 的順序。

[51:00] 現在可能還沒 deadlock，但程式這樣寫，**將來**可能會。【口頭】[51:00] 教授說：「你在電腦前除錯，是為了真實世界。在實驗室裡安全一點比較好。」

**規則 2：不能有 context inconsistency**

[51:32] 同一條拿鎖的路徑上，不能**同時**有 hardirq-unsafe 和 hardirq-safe（或 softirq-unsafe 和 softirq-safe）。

[52:00] 教授的解釋：

```
kernel thread（process context）在 softirq 開著時拿了鎖 A   ← softirq-unsafe
   → softirq 來了（優先權比較高）
   → softirq handler 也要拿 A → 一直空轉
   → softirq 優先權永遠比較高，kernel thread 永遠沒機會跑、放不掉 A
   → deadlock
```

[53:00] 這裡**沒有循環等待**，只是因為拿鎖的 thread 永遠沒機會跑，所以永遠不會放鎖。

[53:00] 這叫 context inconsistency：一把**應該在 softirq-safe 狀態下用**的鎖，被拿到 **softirq-unsafe 狀態下用**了。

[53:32] lockdep 的態度：

- 一把鎖**一直**在 safe 狀態下用 → 沒問題，至少不會被另一個 softirq 擠掉。
- 一把鎖**一直**在 unsafe 狀態下用 → 表示你的程式應該有別的邏輯避免這種問題。
- **safe 和 unsafe 混著用** → 代表你**沒想清楚**。[54:00] 所以要標出來。

[54:00] 經驗是：大多數跟 context 有關的 deadlock 都來自這個原因：大家沒有給自己「什麼時候用什麼鎖」的紀律。程式語言沒有那麼嚴格，不能強制所有行為。

【口頭】[54:30] 教授：「寫出無 bug 程式最好的方法，就是建立方法去除錯、或強制程式的某些部分**在構造上就是對的**。」

### 8.5 【原始碼】lockdep 官方文件的定義

lockdep 追蹤的狀態只有兩種 context：

`kernel/locking/lockdep_states.h:7-8`

```c
LOCKDEP_STATE(HARDIRQ)
LOCKDEP_STATE(SOFTIRQ)
```

官方文件對 safe / unsafe 的定義：

`Documentation/locking/lockdep-design.rst:117-132`

```
A lock is irq-safe means it was ever used in an irq context, while a lock
is irq-unsafe means it was ever acquired with irq enabled.

A softirq-unsafe lock-class is automatically hardirq-unsafe as well. The
following states must be exclusive: only one of them is allowed to be set
for any lock-class based on its usage::

 <hardirq-safe> or <hardirq-unsafe>
 <softirq-safe> or <softirq-unsafe>

This is because if a lock can be used in irq context (irq-safe) then it
cannot be ever acquired with irq enabled (irq-unsafe). Otherwise, a
deadlock may happen. For example, in the scenario that after this lock
was acquired but before released, if the context is interrupted this
lock will be attempted to acquire twice, which creates a deadlock,
referred to as lock recursion deadlock.
```

⚠️ 跟課堂說法的差別：

| | 教授 / 課本 | lockdep 官方文件 |
|---|---|---|
| 標記的對象 | **每一次拿鎖** | **鎖的類別（lock class）**，累積它所有的使用紀錄 |
| irq-**safe** | 拿鎖時中斷是**關**的 | 這把鎖**曾經在中斷 context 裡**被用過 |
| irq-**unsafe** | 拿鎖時中斷是**開**的 | 這把鎖**曾經在中斷開著時**被拿過 |
| 規則 | 同一條路徑不能 safe 和 unsafe 混用 | 同一個 lock class 不能**同時**是 safe 和 unsafe |

用官方定義重講 §8.4 的例子：鎖 A 在 softirq handler 裡被用過（→ softirq-safe），又在 process context 裡、softirq 開著時被拿過（→ softirq-unsafe）。兩個狀態同時成立 → lockdep 報「`WARNING: inconsistent lock state`」（`lockdep.c:3939`）。正確寫法是 process context 裡拿 A 時用 `spin_lock_bh()`（先關 softirq）。教授講的情境和結論是對的，只是「safe」這個字的意思不一樣。

官方文件還有兩條多鎖規則：

`Documentation/locking/lockdep-design.rst:140-166`（節錄）

```
The same lock-class must not be acquired twice, because this could lead
to lock recursion deadlocks.

Furthermore, two locks can not be taken in inverse order::

 <L1> -> <L2>
 <L2> -> <L1>

because this could lead to a deadlock - referred to as lock inversion
deadlock - ...

Furthermore, the following usage based lock dependencies are not allowed
between any two lock-classes::

   <hardirq-safe>   ->  <hardirq-unsafe>
   <softirq-safe>   ->  <softirq-unsafe>
```

| 規則 | 對應 | 違反時的訊息 |
|---|---|---|
| 同一個 lock class 不能拿兩次 | lock recursion | — |
| L1→L2 和 L2→L1 不能同時出現 | §8.4 規則 1 lock inversion | `WARNING: possible circular locking dependency detected`（`lockdep.c:1942`） |
| 拿著 irq-safe 鎖時，不能去拿 irq-unsafe 鎖 | §8.4 規則 2 的多鎖版本 | `WARNING: possible irq lock inversion dependency detected`（`lockdep.c:4001`） |

第三條的理由（文件第 162-166 行）：hardirq-safe 鎖可能被中斷 context 拿；如果它依賴一把 hardirq-unsafe 的鎖，中斷可能打斷正拿著那把 unsafe 鎖的人，形成反轉 deadlock。

### 8.6 用 hash 快取鎖鏈

[54:30] 一般的循環 deadlock：A→B→C→…，建一張圖，找圖上的環。

[55:01] 用 BFS 或 DFS 找環要 **O(V + E)**。E 大約是 V 的平方，所以差不多是**平方時間**，太慢。每次拿鎖都做，受不了。

[55:01] 聰明的想法：用**模式（pattern）**。[55:32] 觀察發現 kernel 程式裡，拿鎖的鏈常常是**重複出現的固定模式**。

[55:32] 做法：

1. 把整條拿鎖的鏈算一個 **hash**（教授說「像 blockchain 一樣」）。
2. hash 表裡存這條鏈的狀態：以前檢查過是**安全的嗎**？
3. [56:00] 遇到**新的**鏈，才做環偵測、context inconsistency 檢查等，結果寫進 hash 表。
4. 之後再遇到同一條鏈，**查表就好**，不用再找環。

[56:30] 這依賴「kernel 裡這種模式通常存在」的事實。

【課本補充】課本 Figure 5.18：key 是鏈的 hash，value 是一個布林值：有環是 1，沒有是 0。

```
 鏈 hash  │ 狀態
──────────┼─────
 0x3f2a…  │  0      ← 不用再找環
 0x91c0…  │  0
 ...      │
```

原始碼：

`kernel/locking/lockdep.c:3720-3732`

```c
static inline struct lock_chain *lookup_chain_cache(u64 chain_key)
{
	struct hlist_head *hash_head = chainhashentry(chain_key);
	struct lock_chain *chain;

	hlist_for_each_entry_rcu(chain, hash_head, entry) {
		if (READ_ONCE(chain->chain_key) == chain_key) {
			debug_atomic_inc(chain_lookup_hits);
			return chain;
		}
	}
	return NULL;
}
```

`kernel/locking/lockdep.c:3785-3800`

```c
static int validate_chain(struct task_struct *curr,
			  struct held_lock *hlock,
			  int chain_head, u64 chain_key)
{
	/*
	 * Trylock needs to maintain the stack of held locks, but it
	 * does not add new dependencies, because trylock can be done
	 * in any order.
	 *
	 * We look up the chain_key and do the O(N^2) check and update of
	 * the dependencies only if this is a new dependency chain.
	 * (If lookup_chain_cache_add() return with 1 it acquires
	 * graph_lock for us)
	 */
	if (!hlock->trylock && hlock->check &&
	    lookup_chain_cache_add(curr, hlock, chain_key)) {
```

| 部分 | 說明 |
|---|---|
| `chain_key` | 64 位元的 hash：把目前手上拿著的鎖（依序）hash 起來 |
| `lookup_chain_cache` | 在 hash 表裡找這個 key。找到就是「看過了」 |
| 註解 "do the O(N^2) check ... only if this is a new dependency chain" | 跟教授說的一模一樣：**只有新的鏈**才做平方時間的檢查 |
| `!hlock->trylock` | trylock 不會等，不會造成 deadlock，所以不加依賴邊 |
| `lookup_chain_cache_add` 回傳 1 | 是新鏈，已經加進快取，接著做完整檢查（`check_deadlock` 等） |

和課本的小差別：快取裡**只放驗證通過的鏈**，不是存一個 0/1 狀態。驗證失敗時 lockdep 會印警告並把自己關掉（`debug_locks_off()`），不會繼續用。

---

## 9. 預告：RCU

[56:30] kernel 的同步到這裡結束。

[57:01] 下一講：**RCU（read-copy-update）**。教授說這「可以說是 Linux 世界在同步方面**最重要的輸出**」，一種全新的、非常有效率的同步模式。

（第 19 講的 litmus test 註解也說：store buffering 除了 full barrier，「Locking 和 RCU 也可以」。）

---

## 10. 一頁速查表

### 10.1 三種鎖

| | spinlock | mutex | semaphore |
|---|---|---|---|
| 持有者 | CPU | task | 無 |
| 等待方式 | 空轉（`PAUSE`） | 先樂觀空轉，再睡 | 睡 |
| 搶占 | 關 | 不關 | 不關 |
| 中斷 context | 可以 | 不行（`might_sleep`） | 不行 |
| 快速路徑 | trylock：一次 CAS | `owner` 0 → `current`：一次 CAS | count > 0 就減 |

### 10.2 Preemption count

- per-CPU（x86：`pcpu_hot.preempt_count`，用 `%gs` 存取）。
- `preempt_disable` = 加 1 + compiler barrier；`preempt_enable` = compiler barrier + 減 1，變 0 就呼叫排程器。
- 巢狀：拿 n 把鎖，要放 n 次。
- x86：最高位元是反向的 need_resched，一道 `decl` 同時檢查「計數為 0」和「需要排程」。

### 10.3 Ticket lock（asm-generic）

```
32 位元：[ ticket (高 16) | next (低 16) ]
lock：   val = fetch_add(lock, 1<<16); 我的號碼 = val>>16; 等到 (u16)lock == 我的號碼; smp_mb
unlock： next 那一半 += 1（smp_store_release；big endian 要 +1 個 u16）
trylock：ticket == next 才用 CAS 拿號碼
```

- 公平（實測 8 thread：ticket 每人差 < 0.05%，TAS 差 12 倍）。
- x86 實際用 **qspinlock**（MCS 為基礎，壓進 4 bytes）。ticket lock 給 RISC-V 等架構用。

### 10.4 中斷版本

| API | 關搶占 | 關中斷 | 恢復原狀態 |
|---|---|---|---|
| `spin_lock` | ✓ | ✗ | — |
| `spin_lock_irq` | ✓ | ✓ | ✗（unlock 一律開） |
| `spin_lock_irqsave` | ✓ | ✓ | ✓ |
| `spin_lock_bh` | ✓ | 只關 softirq | — |

### 10.5 Lockdep

| 檢查 | 內容 |
|---|---|
| Lock depth | 最多 48 把（`MAX_LOCK_DEPTH`） |
| Lock inversion | A→B 和 B→A 不能同時出現 |
| Context inconsistency | 同一把鎖不能既 irq-safe 又 irq-unsafe |
| irq-safe → irq-unsafe | 拿著 safe 鎖不能去拿 unsafe 鎖 |
| 加速 | 鎖鏈 hash 快取，只有新鏈才做 O(N²) 檢查 |

### 10.6 本講看過的原始碼位置

| 位置 | 內容 |
|---|---|
| `include/linux/preempt.h:201-205, 218-223` | `preempt_disable` / `preempt_enable` |
| `include/linux/preempt.h:26-29` | preempt count 的位元分段 |
| `arch/x86/include/asm/preempt.h:12-28, 78-97` | per-CPU 計數、反向 need_resched |
| `include/linux/spinlock_types_raw.h:14-23` | `raw_spinlock_t` |
| `include/asm-generic/spinlock.h:33-69` | ticket lock：lock、trylock、unlock |
| `include/asm-generic/barrier.h:248-258` | `smp_cond_load_relaxed` |
| `arch/x86/include/asm/vdso/processor.h:11-19` | `cpu_relax` = `rep; nop`（PAUSE） |
| `arch/x86/Kconfig:119-120`、`arch/x86/include/asm/spinlock.h:27` | x86 用 qspinlock |
| `include/linux/spinlock_api_smp.h:104-153` | `spin_lock`、`irqsave`、`irqrestore` |
| `include/linux/lockdep.h:454-461` | `LOCK_CONTENDED`：快速路徑 → 慢速路徑 |
| `include/linux/mutex.h:63-76` | `struct mutex` |
| `kernel/locking/mutex.c:59-72` | owner 低位元當旗標 |
| `kernel/locking/mutex.c:166-182, 281-287` | mutex 快速路徑 |
| `kernel/locking/mutex.c:617-679` | mutex 慢速路徑：拿著 `wait_lock` 再試、排隊、睡 |
| `kernel/locking/mcs_spinlock.h:18-22` | MCS 節點 |
| `kernel/locking/qspinlock.c:32-48` | qspinlock 把 MCS 壓進 u32 |
| `include/linux/sched.h:1164`、`kernel/locking/lockdep.c:5066-5074` | `MAX_LOCK_DEPTH` = 48 |
| `kernel/locking/lockdep_states.h:7-8` | HARDIRQ、SOFTIRQ |
| `Documentation/locking/lockdep-design.rst:117-166` | safe/unsafe 定義與規則 |
| `kernel/locking/lockdep.c:3720-3800` | 鎖鏈 hash 快取、`validate_chain` |

---

## 11. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **RCU** | **第 23 講**：RCU |
| **`preempt_enable` 呼叫 `__preempt_schedule`、task 狀態改成 running 不等於在跑** | **第 24–26 講**：排程 |
| **per-CPU run queue 的鎖、多核心** | **第 25 講**：Multiprocessor scheduling |
| **rt_mutex（`PREEMPT_RT` 下 mutex 改用它）** | **第 28 講**：Real-time resource allocation（priority inheritance） |
| **softirq、hardirq context** | 回顧第 16 講 |

（講次對應依播放清單標題推估。）

---

## 附：本講一句話總結

> **Kernel 的 spinlock 是「鎖住 CPU」：拿著它不准睡、不准被搶占（per-CPU 的 preemption count 加 1，放完所有鎖減回 0 才呼叫排程器；x86 還用反向的 need_resched 位元讓一道 `decl` 同時檢查兩件事），否則拿鎖者被換走、別人空轉就 deadlock。課堂講的 spinlock 是銀行號碼牌式的 ticket lock：一個 32 位元裡塞 ticket 和 next，`fetch_add(1<<16)` 拿號碼、等 next 叫到自己、放鎖時用 release 把 next 加 1，實測完全公平（TAS 鎖則差 12 倍）；但 x86 實際用的是 MCS 式的 qspinlock，ticket lock 留給 RISC-V 等架構。會被中斷 handler 用到的鎖要用 `irqsave`/`irqrestore`。Mutex 由 task 持有、可以睡：快速路徑一次 CAS 把 `owner` 從 0 換成 `current`（低位元還塞了旗標），失敗就拿著 `wait_lock` 再試一次、排隊、睡，避免 lost wakeup。Semaphore 是 count + 等待串列 + spinlock。Lockdep 是除錯用的 deadlock 偵測器：限制同時最多 48 把鎖、抓 A→B/B→A 的反轉、抓同一把鎖既在中斷裡用又在中斷開著時拿的 context 不一致，並用鎖鏈 hash 快取，只有新鏈才做 O(N²) 的檢查。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 22（Chapter 5, Part V）錄影逐字稿（YouTube 自動字幕，57:37）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§5.3–5.3.4，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
