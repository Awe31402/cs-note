# Operating Systems: A Linux Kernel-Oriented Approach 第八講完整筆記：Kernel Stack 與優先權（Chapter 3, Part II）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 8（Chapter 3, Part II）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（43:12，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§3.1.5–3.1.8（p.72–79）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
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
- [1. Kernel thread 與 kernel stack](#1-kernel-thread-與-kernel-stack)
  - [1.1 誰在 kernel 裡替使用者 process 做事？](#11-誰在-kernel-裡替使用者-process-做事)
  - [1.2 共用位址空間，但 stack 是私有的](#12-共用位址空間但-stack-是私有的)
  - [1.3 【原始碼】x86-64 的 kernel stack 其實是 16 KB](#13-原始碼x86-64-的-kernel-stack-其實是-16-kb)
- [2. 中斷用的 stack](#2-中斷用的-stack)
  - [2.1 為什麼不用 kernel stack](#21-為什麼不用-kernel-stack)
  - [2.2 每顆 CPU 七個中斷 stack](#22-每顆-cpu-七個中斷-stack)
  - [2.3 【原始碼】「七」從哪裡來：TSS 的 IST](#23-原始碼七從哪裡來tss-的-ist)
- [3. 快速找到 `current`：舊方法](#3-快速找到-current舊方法)
  - [3.1 為什麼要這麼在乎一兩條指令](#31-為什麼要這麼在乎一兩條指令)
  - [3.2 `thread_info` 放在 stack 底部](#32-thread_info-放在-stack-底部)
  - [3.3 技巧一：把低 13 位元清成 0](#33-技巧一把低-13-位元清成-0)
  - [3.4 技巧二：第一個欄位就是 `task_struct` 指標](#34-技巧二第一個欄位就是-task_struct-指標)
  - [3.5 【原始碼】舊方法還活在其他架構上](#35-原始碼舊方法還活在其他架構上)
- [4. 快速找到 `current`：新方法](#4-快速找到-current新方法)
  - [4.1 `gs` 一條指令](#41-gs-一條指令)
  - [4.2 為什麼不用其他方法](#42-為什麼不用其他方法)
  - [4.3 【Q&A】segment 暫存器為什麼叫 segment 暫存器？](#43-qasegment-暫存器為什麼叫-segment-暫存器)
  - [4.4 False sharing：per-CPU 區域要對齊快取線](#44-false-sharingper-cpu-區域要對齊快取線)
- [5. Process 優先權](#5-process-優先權)
  - [5.1 0–139：即時與一般](#51-0139即時與一般)
  - [5.2 兩段方向相反](#52-兩段方向相反)
  - [5.3 `nice`：只能對別人好](#53-nice只能對別人好)
  - [5.4 kernel 把方向統一](#54-kernel-把方向統一)
  - [5.5 【原始碼】`prio.h` 與 `__normal_prio()`](#55-原始碼prioh-與-__normal_prio)
  - [5.6 【實測】看自己的優先權](#56-實測看自己的優先權)
- [6. `sched_info`：給排程器的統計](#6-sched_info給排程器的統計)
- [7. 一頁速查表](#7-一頁速查表)
- [8. 與後續講次的連結](#8-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

`[00:33]` 兩個主題：

| # | 主題 | 對應課本 |
|---|---|---|
| 1 | **Kernel stack**：context switch 後資訊放哪？誰在 kernel 裡替使用者 process 做事？怎麼最快找到目前的 `task_struct`？ | §3.1.5 |
| 2 | **優先權**：0–139 的數字、`nice`、kernel 怎麼統一方向 | §3.1.6–3.1.8 |

---

## 1. Kernel thread 與 kernel stack

### 1.1 誰在 kernel 裡替使用者 process 做事？

`[00:33]` 問題：process 進到 kernel mode 後，**誰替它做事**？是不是有另一個 kernel thread，當它在 kernel 裡的**化身（avatar）**？

`[01:01]` 教授說這是個複雜機制，之後會反覆看到。目前只要知道：

- 使用者 thread → 轉換到 kernel mode。
- **其實不是另一個 thread**：是**同一個使用者 thread** 轉換進 kernel mode、再轉回來。
- 負責「把 context 存到記憶體、替使用者 thread 處理 system call」的那個 kernel thread，有**專屬的 stack**——因為 stack 一定需要，用來放區域變數。

`[03:01]` 為什麼不另開一個 thread？

> 「一開始就產生一個新的 kernel thread 來做使用者 thread 的事、平常讓它睡著——**都是壞主意**。比較好的想法是讓**同一個 thread** 做，但它要換成 kernel 化身，而且一定要用**不同的 stack**，因為 kernel mode 裡發生的事不應該讓 user mode 看到。」

`[28:01]` 教授後面再補充：

> 「目前先假設使用者 thread 和它的 kernel 夥伴是**分開的**。之後的章節會告訴你它們其實是**同一個**——只是進 kernel 時我變成另一個人，換了一套衣服。就像**化身博士（Jekyll and Hyde）**：在 user mode 我這樣做事，在 kernel mode 我那樣做事。」

【課本補充】課本的說法（p.72）：每個使用者空間的 thread 都有一個**對應的 kernel thread**，使用者 thread 在跑時它通常**休眠**。使用者 thread 發 system call 時，OS 不產生新 thread，而是**執行那個對應的 kernel thread**，用**它的 kernel stack**。「kernel stack 就成了所有 thread 相關狀態的天然的家。」

### 1.2 共用位址空間，但 stack 是私有的

`[02:00]` 所有 kernel thread **共用同一個虛擬位址空間**（所以也共用同一張 page table）。那 stack 還算私有嗎？

- stack **一定要私有**，不能讓多個 thread 改同一個 stack。
- 所以對 kernel stack 加了**很多限制**，保證每個 thread 有自己的 stack、別人不能寫。

`[03:30]` 有人會擔心：兩個 kernel thread 的 stack 互相覆寫？——經典的**重疊問題**。

`[04:01]` 不會發生，因為：

- kernel stack 大小**限制在 8 KB**（教授和課本都這樣說，但 x86-64 實際是 16 KB，見 §1.3）。
- 每個 kernel thread 的 8 KB 在記憶體裡排好，**永遠不會重疊**。

```
  kernel 的虛擬位址空間（所有 kernel thread 共用）
  ┌────────────┬────────────┬────────────┬────────────┐
  │ thread A 的 │ thread B 的 │ thread C 的 │    ...     │
  │ stack 8 KB │ stack 8 KB │ stack 8 KB │            │
  └────────────┴────────────┴────────────┴────────────┘
       互不重疊 → 實際上每個都是私有的
```

`[04:01]` 只要 thread 還活著——**包括 zombie 狀態**——它的 kernel stack 都有用的資料。

【課本補充】課本說（p.73）：跟使用者 stack 不同，kernel 不希望 kernel stack **長得很大**，那會造成很多記憶體管理問題。所以**所有版本**的 Linux kernel 都對 kernel stack 大小設了**硬上限**。

### 1.3 【原始碼】x86-64 的 kernel stack 其實是 16 KB

教授和課本都說「8 KB（兩個 4 KB page）」，看 v6.2.12 的 x86-64：

```c
// arch/x86/include/asm/page_64_types.h:9-22
#ifdef CONFIG_KASAN
#define KASAN_STACK_ORDER 1
#else
#define KASAN_STACK_ORDER 0
#endif

#define THREAD_SIZE_ORDER	(2 + KASAN_STACK_ORDER)
#define THREAD_SIZE  (PAGE_SIZE << THREAD_SIZE_ORDER)

#define EXCEPTION_STACK_ORDER (1 + KASAN_STACK_ORDER)
#define EXCEPTION_STKSZ (PAGE_SIZE << EXCEPTION_STACK_ORDER)

#define IRQ_STACK_ORDER (2 + KASAN_STACK_ORDER)
#define IRQ_STACK_SIZE (PAGE_SIZE << IRQ_STACK_ORDER)
```

| 常數 | 計算（沒開 KASAN） | 大小 | 用途 |
|---|---|---|---|
| **`THREAD_SIZE`** | 4 KB << 2 | **16 KB** | 每個 thread 的 **kernel stack** |
| `EXCEPTION_STKSZ` | 4 KB << 1 | 8 KB | 特殊 exception 用的 stack（§2.3） |
| `IRQ_STACK_SIZE` | 4 KB << 2 | 16 KB | 每顆 CPU 處理一般硬體中斷的 stack |

- `ORDER` 是「2 的幾次方個 page」：order 2 = 4 個 page = 16 KB。
- **KASAN**（一種記憶體錯誤偵測工具）開啟時，每個都再**加倍**，因為它會讓 stack 用量變大。

**跟教授 / 課本對照**：x86-64 的 kernel stack 在 v6.2.12 是 **16 KB**，不是 8 KB。8 KB 是 **32 位元 x86** 和**舊版 kernel**（x86-64 在 3.15 版左右從 8 KB 加大到 16 KB）的數字。但**觀念完全一樣**：大小固定、有硬上限、對齊到自己的大小。下面 §3 的「清掉低位元」技巧，在 16 KB 時就是清掉低 **14** 位元。

---

## 2. 中斷用的 stack

### 2.1 為什麼不用 kernel stack

`[04:31]` 進 kernel mode 除了 system call，還有**中斷**：一般程式在跑，硬體中斷來了。

`[05:00]` 跟 system call 一樣，可以讓同一個使用者 thread 轉進 kernel mode 來處理（理論上也可以另開一個高優先權的中斷 thread）。但**不想用 kernel stack**，三個理由：

| # | 理由 |
|---|---|
| 1 | 中斷狀態是**高度敏感、安全**的——處理的**不是這個 process 的資料** |
| 2 | 有時候 **8 KB 不夠** |
| 3 | 中斷處理到一半，**又來一個中斷**：要存目前的 context、開始第二個 handler，又需要另一個 stack |

所以中斷 handler 一般**不用 kernel stack**，而是用**另外的中斷 stack**。

### 2.2 每顆 CPU 七個中斷 stack

`[06:31]` 巢狀中斷：

```
  一般程式在跑
       │ 硬體中斷 1
       ▼
  handler 1 在跑 ─── 更高優先權中斷 2 ───► handler 2 在跑 ─── NMI ───► handler 3 在跑
  （暫停）                                  （暫停）
```

- **NMI（non-maskable interrupt，不可遮罩中斷）**：**不能被忽略**的中斷。
- 每個 handler 都需要自己的 stack。

`[07:01]` Linux 的做法：

- **每顆 CPU 維護七個專用的中斷 stack**。
- handler 要跑時，挑一個**空的** stack 來用。
- 跑完 → stack 裡的暫存資料清掉，給下一個用。
- 跑到一半又來中斷 → **handler 對 handler 的 context switch**，新 handler 再挑一個空的 stack。

`[07:31]` 所以支援**七層中斷巢狀**：

> 「實務上**夠用了**。不需要更高層的巢狀，也不預期會同時來那麼多中斷——**除非有人想駭進系統**。」

【課本補充】課本說：這概念上跟使用者 task 的 context switch 類似，只是被暫停和恢復的是 **interrupt handler**，而且全部發生在 kernel 裡。

### 2.3 【原始碼】「七」從哪裡來：TSS 的 IST

「七」不是 Linux 隨便選的，是 **x86 硬體**的限制：

```c
// arch/x86/include/asm/page_64_types.h:24-31
/*
 * The index for the tss.ist[] array. The hardware limit is 7 entries.
 */
#define	IST_INDEX_DF		0
#define	IST_INDEX_NMI		1
#define	IST_INDEX_DB		2
#define	IST_INDEX_MCE		3
#define	IST_INDEX_VC		4
```

註解：「`tss.ist[]` 陣列的索引。**硬體上限是 7 格**。」

**TSS**（Task State Segment）是 x86 的一個硬體結構，每顆 CPU 一個：

```c
// arch/x86/include/asm/processor.h:270-289
struct x86_hw_tss {
	u32			reserved1;
	u64			sp0;
	u64			sp1;

	/*
	 * Since Linux does not use ring 2, the 'sp2' slot is unused by
	 * hardware.  entry_SYSCALL_64 uses it as scratch space to stash
	 * the user RSP value.
	 */
	u64			sp2;

	u64			reserved2;
	u64			ist[7];
	u32			reserved3;
	u32			reserved4;
	u16			reserved5;
	u16			io_bitmap_base;

} __attribute__((packed));
```

| 欄位 | 意思 |
|---|---|
| `sp0` | 從 ring 3 進 ring 0 時，硬體**自動切換**到這個 stack——就是 **kernel stack** 的頂端 |
| `sp1`、`sp2` | 進 ring 1、ring 2 用的 stack |
| **`ist[7]`** | **Interrupt Stack Table**：**7 個** stack 指標。中斷描述表裡每個中斷可以指定「用 IST 第幾號」，硬體就自動切到那個 stack |
| `io_bitmap_base` | 第 6 講的 I/O port 權限表 |

兩個驚喜：

1. **`ist[7]`**：就是教授說的「**七**個中斷 stack」——這是**硬體**給的 7 格。
2. **第 276 行的註解**：「**因為 Linux 不使用 ring 2**，`sp2` 這格硬體不會用，`entry_SYSCALL_64` 拿它當暫存空間存使用者的 RSP。」——印證第 2 講筆記的觀察「Linux 只用 ring 0 和 ring 3」。第 2 講 `entry_SYSCALL_64` 那行 `movq %rsp, PER_CPU_VAR(cpu_tss_rw + TSS_sp2)` 就是把使用者 stack pointer **暫存在這格**。

v6.2.12 實際怎麼用這 7 格？

```c
// arch/x86/include/asm/cpu_entry_area.h:48-56
enum exception_stack_ordering {
	ESTACK_DF,
	ESTACK_NMI,
	ESTACK_DB,
	ESTACK_MCE,
	ESTACK_VC,
	ESTACK_VC2,
	N_EXCEPTION_STACKS
};
```

| IST | 給誰 |
|---|---|
| DF | **Double fault**：處理 exception 時又出 exception（例如 stack 爆了） |
| NMI | **不可遮罩中斷** |
| DB | **除錯** exception |
| MCE | **Machine check**：硬體錯誤 |
| VC、VC2 | AMD SEV 虛擬化相關 |

**跟教授說的對照**：

- 教授說「每顆 CPU 7 個中斷 stack，handler 挑空的用，支援 7 層巢狀」。
- 原始碼：7 是**硬體 IST 的格數**；Linux 實際**為特定的嚴重 exception 各綁一個專用 stack**（DF、NMI、DB、MCE……），不是「挑空的用」。
- **一般的硬體中斷**用的是每顆 CPU 一個的 **IRQ stack**（`IRQ_STACK_SIZE` = 16 KB，§1.3）。
- 為什麼 NMI、double fault 要專用 stack？因為它們可能在**任何時候**發生——包括 kernel stack 壞掉或用完的時候。如果還用原本的 stack，handler 自己也會跟著出錯。

---

## 3. 快速找到 `current`：舊方法

### 3.1 為什麼要這麼在乎一兩條指令

`[09:01]` 有了 kernel stack，現在想**非常快地**找到**目前 process 的 `task_struct` 指標**。

> 「同一個操作會被呼叫幾百次、幾千次，**多一條指令都太多**。」

`[09:30]` 教授先講**舊 kernel** 的做法（現在不用了），看看 kernel 設計中效率扮演什麼角色：

> 【口頭】「很多時候只省了**一兩條指令**，但如果一個操作被非常頻繁地呼叫，省 20% 可能換來整個程式 2–3% 的加速——這**很大**。一般來說 OS 不負責大量加速，但**能做到的地方就要做**。」

### 3.2 `thread_info` 放在 stack 底部

`[10:02]`（課本 Fig 3.3）一個往下長的 stack，8 KB（兩個 page）：

```
  高位址 ┌──────────────────────┐ ← stack 從這裡開始
        │                      │
        │       Stack ↓         │
        │                      │
        │   （最多長到這裡）       │
        ├──────────────────────┤
        │     thread_info       │ ← 第一個欄位：指向 task_struct 的指標
  低位址 └──────────────────────┘ ← 對齊 8 KB 邊界（n × 8 KB）
                                    current 從這裡找起
```

- **`thread_info` 放在這塊區域的最底部**（最低位址）。
- 所以 stack **最多只能長到 `thread_info` 上面**，不能越過，不然會蓋掉 `thread_info`。
- **這塊區域的底部對齊 8 KB 邊界**：位址是 n × 8 KB。

### 3.3 技巧一：把低 13 位元清成 0

`[11:02]` 目標：給 stack 裡的**任何位址**（目前的 stack pointer，或 stack 上某個變數的位址），**很快**跳到底部的 8 KB 邊界。

【口頭】教授的十進位例子 `[11:30]`：

> 「給一個十進位數字，找**小於它的最大的 100 的倍數**。很簡單：17567 → 把最後兩位換成 00 → **17500**。」

二進位也一樣 `[12:01]`：

- 找小於某數的最大的 **4 的倍數**：4 = `100₂`，把**最後兩位**清成 0。
- **8 KB** = 8192 = **2¹³**：把**最後 13 位**清成 0。

做法：跟一個「前面全 1、後面 13 個 0」的數字做 **AND**：

```
  任意 stack 位址:   0x...a5f38  = ... 1010 0101 1111 0011 1000
  遮罩 ~(8192-1):    0x...fe000  = ... 1111 1110 0000 0000 0000
  AND 結果:          0x...a4000  = ... 1010 0100 0000 0000 0000   ← stack 底部
                                              └── 低 13 位全變 0
```

`[13:30]` 「x86 **一條指令**就做得到。」這是設計的**第一個巧妙之處**。

【課本補充】課本的 Example 3.1.1：

```c
int find_multiple(int x) {
	return (x & ~7);      /* 小於等於 x 的最大的 8 的倍數 */
}
```

`~7` = `...11111000`，把低 3 位清掉（8 = 2³）。8 KB 就是 `x & ~8191`。

### 3.4 技巧二：第一個欄位就是 `task_struct` 指標

`[14:02]` **第二個巧妙之處**：`thread_info` 是**低階**資料結構（知道硬體）。定義它時，讓**第一個欄位**就是 **`struct task_struct *`**。

- 第一個欄位就放在**最底部那個位址**。
- 所以底部位址的那 8 bytes，就是**指向 `task_struct` 的指標**。

`[15:01]` 整個方法要**兩條指令**：

```asm
  and  $~8191, %rsp_copy     # 指令 1：清掉低 13 位 → 得到 thread_info 的位址
  mov  (%rsp_copy), %rax     # 指令 2：讀第一個欄位 → 得到 task_struct 指標
```

（示意，實際暫存器由編譯器決定。）

`[15:32]` 兩個技巧：

1. **數學**技巧：一個運算直接跳到起點。
2. **資料佈局**技巧：把欄位排好，讓 `task_struct` 指標是第一個欄位。

`[16:01]` 為什麼不直接把整個 `task_struct` 放這裡？因為 `task_struct` **很大**，會吃掉 8 KB 的一大塊，所以只放**指標**。

`[16:01]` 現在 `thread_info` 原本的用途沒了，所以它**變小了很多**（第 7 講看過，只剩 4 個欄位）。

`[16:32]` 【口頭】「kernel 裡有很多這種例子：**結構在記憶體裡怎麼排列非常重要**。任何低階程式設計師都要注意這些細節。」

【課本補充】課本補充（p.74）：

- 即使在現在的 kernel，這塊記憶體**還是對齊到 8 KB 邊界**。
- 舊 `thread_info` 裡有一個叫 **`task`** 的變數指向 `task_struct`。`current` 巨集包辦了「取得 `thread_info`、再取得 `task_struct`」的邏輯。
- **Point 3.1.2**：存一組位元，最省空間的通常是無號整數的變形。存 12 個位元用 `u16`，存 40 個位元用 `u64`。

### 3.5 【原始碼】舊方法還活在其他架構上

課本說「沒有 segmentation 的架構**還在用**這個較慢的轉址方法」。v6.2.12 裡確實有：

```c
// arch/csky/include/asm/thread_info.h:51（節錄自 current_thread_info()）
	return (struct thread_info *)(sp & ~(THREAD_SIZE - 1));
```

```c
// arch/nios2/include/asm/thread_info.h:37
	struct task_struct	*task;		/* main task structure */
```

| 架構 | 做法 |
|---|---|
| **C-SKY、Nios II、ARC、SuperH** | `sp & ~(THREAD_SIZE - 1)`——**就是技巧一**：stack pointer 清掉低位元 |
| **Nios II** 等 | `thread_info` 裡有 `struct task_struct *task`——**就是技巧二** |
| x86 | 不用了，改用 `gs`（§4） |

`~(THREAD_SIZE - 1)`：`THREAD_SIZE` 是 2 的次方，減 1 就是「低位全 1」，再 `~` 就是「低位全 0、高位全 1」的遮罩——跟課本的 `~7` 同一招。

---

## 4. 快速找到 `current`：新方法

### 4.1 `gs` 一條指令

`[17:00]` 現在的 kernel 用 **`current` 巨集**：

- 大家都用它，**不在乎**它怎麼實作——像前置處理器的函式。
- 宣告一個 **per-CPU 變數** `current_task`，指向目前的 `task_struct`。
- `get_current()` 是一個 **inline 函式**。

`[17:31]` 【口頭】**inline 函式**是什麼？

> 「效果上就是巨集：**沒有函式呼叫和返回**，整個函式在呼叫的地方**展開、複製貼上**。函式呼叫的開銷是什麼？存暫存器、還原暫存器、call、return、建 stack frame——這些都沒有。某種意義上，這是程式設計師建立巨集的方式，但**保留呼叫函式的優雅**。」

`[18:31]` 流程：

```
 current
   └─► get_current()           （inline 函式）
         └─► this_cpu_read_stable(current_task)
               └─► 編譯後：一條 mov %gs:偏移, %rax
```

- 看起來是一串函式和巨集，但**編譯器實際產生的程式碼只有一條指令**：從 `gs` 指向的 per-CPU 區域讀出 `task_struct` 指標。
- `[19:30]` `gs` 指向一個區域，從區域開頭算起，`task_struct` 指標在哪個偏移是**已知的**。所以**一次 load** 就拿到。
- **一條指令取代兩條**，所以舊方法被取代了。

`[20:00]` 【口頭】「讀起來有點複雜：`current` 巨集 → `get_current` inline 函式 → 又是一個巨集呼叫……在 C 裡呼叫一個東西，你**不知道**它是函式還是巨集，呼叫的樣子都一樣。但最後就是從某個記憶體位址載入一個值。」

（原始碼第 6 講 §4.5 看過了：`arch/x86/include/asm/current.h:39-44`。v6.2.12 的變數叫 `pcpu_hot.current_task`，課本 Listing 3.2 寫的是較舊版本的 `current_task`。）

### 4.2 為什麼不用其他方法

`[21:01]` 教授比較了其他選項：

| 選項 | 為什麼不用 |
|---|---|
| **專用一個一般暫存器**存 `current` | 一般暫存器**數量有限**，會少一個。而且暫存器分配是**編譯器的專屬權利**，很難強迫它「永遠把這個放在暫存器」。C 的 `register` 關鍵字只是**建議** |
| **全域變數** | 要**每顆 CPU 一份**，所以要用陣列（10 顆 CPU 就 10 格）。寫高階程式時聽起來合理，但**翻成指令數一數**：找陣列基底、算索引、再 load……**多出很多工作** |
| **存在 stack 底部** | 就是舊方法，主要要 **2 條**指令；新方法 **1 條** |
| **MSR**（model-specific register，CPU 裡的特權暫存器） | 技術上不壞，但 32 顆 CPU 就要 32 個這種暫存器，**沒那麼多**給這個用途。而且特權暫存器**不是通用標準**，同一家族的處理器不一定都有，**不可攜** |
| ✅ **`gs` 指向的 per-CPU 區域** | 「合理的解法」 |

`[23:34]` 整個遊戲：

- **`gs` 存一個指向 per-CPU 區域的指標**。
- segment 暫存器是 **CPU 的屬性**：**每顆 CPU 的 `gs` 都不一樣**，所以自然指向各自的區域。
- 區域裡不只放 `task_struct`，還有一堆**只跟這顆 CPU 有關**的變數。

> 【口頭】「這像是我**私人的、超快的儲存區**。我們好好利用了 segment 暫存器——x86-64 只用得上 `fs` 和 `gs`，其他都沒用，我們用得很好。」

`[24:02]` Linux 有一個巨集 **`DEFINE_PER_CPU`**，讓你建立 per-CPU 區域，並把位址存在 segment 暫存器裡。

【課本補充】課本補充（p.76–77）：

- 這是 x86 segmentation **少數真正非常有用**的例子。用分段定址**不需要額外指令**，segment 資訊可以直接嵌在記憶體位址裡，「這部分是**免費**的，kernel 設計者把它用到極致」。
- 存在**固定記憶體位址**的問題：大幅限制使用虛擬位址空間的彈性，可能造成跨架構的可攜性問題。
- 「要注意 kernel 開發者在這方面的**勤奮**，體會他們多努力讓**每一個**機制盡可能有效率——**能省記憶體存取就省**。」

### 4.3 【Q&A】segment 暫存器為什麼叫 segment 暫存器？

`[24:32]`

**學生問**：segment 暫存器為什麼叫 segment 暫存器？

**教授答**：來自 x86 的**分段觀點**（第 6 講）：

- 我們把虛擬位址**再虛擬化一次**，有 data segment、code segment 等。
- CPU 產生 **logical address** → 加上 segment 暫存器的內容 → 得到 **linear address**。
- linear address 就是我們一直以來說的**虛擬位址**，它再去做虛擬→實體的轉換。
- 很多時候 CPU 能**根據存取類型自動判斷**用哪個 segment；也可以在記憶體運算元的格式裡**明確指定**，例如 `%fs:4(%esp)` 表示 logical address 是 `esp + 4`、segment 是 `fs`。

### 4.4 False sharing：per-CPU 區域要對齊快取線

`[25:31]` 寫**高效能**程式碼時要注意（「5%、10%、20%、甚至 25% 都很重要」）：

> **一條從 64 bytes 倍數開始的 64-byte 快取線，永遠不要跨處理器共用。**

```
   一條快取線（64 bytes）
  ┌────────────────────────────────┐
  │  核心 0 在用這半  │  核心 1 在用這半  │
  └────────────────────────────────┘
          ↑ 位址不重疊，但在同一條快取線上
          → 快取線在兩個核心之間「彈來彈去」
```

`[26:33]` 為什麼？

- **64 bytes（或 128）是記憶體系統的原子單位**。
- 我要寫**一個 byte**，就需要**整條**快取線。
- 核心 0 寫左半、核心 1 寫右半——位址根本不重疊，但每次寫都要把整條快取線搶過來。
- 快取線會**一直彈來彈去（bounce）**。

`[27:00]` 這叫 **false sharing（偽共享）**：明明沒有共享資料，卻因為在同一條快取線上而互相干擾。

**解法**：建立這些 per-CPU 區域時要**很有紀律**，**對齊到 64 或 128 bytes 邊界**。

`[27:31]` 小結：

> 「寫正經的程式碼時，**小小的互動很重要**，會大幅降低效能。把指標放在 stack 尾端是好主意，因為很快就能到；但**更好的主意**是直接用 segment 暫存器。」

`[29:01]` `task_struct` 是那**一個主資料結構**，大家都指向它，所以要能快速存取。

（第 3 講看過 `jiffies` 宣告裡的 `__cacheline_aligned_in_smp`，就是這個目的：讓它獨佔一條快取線。）

---

## 5. Process 優先權

`[29:32]` 現在有很多 thread：使用者 thread、kernel thread、處理中斷的**超高優先權** thread（用 CPU 專屬的中斷 stack）。假設有 100 個 thread，**怎麼排程**？需要理解**優先權**。

### 5.1 0–139：即時與一般

`[30:01]` Linux（以及所有類 Unix OS）的優先權範圍是 **0 到 139**，但有點複雜、有點詭異。（課本 Table 3.2）

| 類型 | 範圍 |
|---|---|
| **即時（real-time）** | **0–99** |
| **一般使用者（user）** | **100–139** |

`[30:01]` **即時 process**：跟**截止時間（deadline）**綁在一起，計算一定要在某個時間點前做完。

【口頭】例子：

> 「**影片處理**：如果是 60 Hz 的幀率，每幀有 16.66 ms。錯過一幀，你會看到一點點抖動，大多數情況看不出來。但**硬即時系統（hard real-time system）** 是火箭或飛彈上的那種。」（⚠️ 逐字稿把 16.66 ms 寫成「16.66%」）

### 5.2 兩段方向相反

`[31:01]`（課本 Fig 3.4）**不是單調遞增**，有點奇怪：

```
  實際優先權
     ▲
  最高│              ●  ← 99（整個系統最高）
     │            ╱ │
     │          ╱   │●  ← 100（一般 process 裡最高）
     │        ╱     │  ╲
     │      ╱       │    ╲
     │    ╱         │      ╲
  最低│  ●           │        ● ← 139（整個系統最低）
     └──┼───────────┼────────┼──► 優先權數字
        0          99/100   139
          即時：越大越高    一般：越大越低
```

| 範圍 | 規則 | 最高 | 最低 |
|---|---|---|---|
| 即時 0–99 | **數字越大，優先權越高** | 99 | 0 |
| 一般 100–139 | **數字越大，優先權越低** | 100 | 139 |

- **100 的實際優先權比 0 還低**：任何一般 process 都比任何即時 process 低。
- `[32:02]` **整個系統優先權最高的是 99**。

`[32:02]` 【口頭】警告：

> 「強烈建議**不要跑優先權超過 60 或 70** 的東西，整個系統會變得**不穩定然後當掉**。所以大家被勸告不要亂改即時優先權，**除非你想讓系統當機**。連 kernel 自己的很多工作都跑在優先權 **50**。但 Linux 要能跑在各種系統上，包括飛彈、火箭、無人機，它們可能需要這種超高優先權的工作——但那時你得**非常小心地設計系統**，不然只有這個在跑，其他工作都不會進行。」

【課本補充】課本補充（p.77）：

- 為什麼即時要 100 個等級？因為即時 task 也有**不同的重要程度**：有些是「軟」需求，偶爾延遲沒關係；有些**完全不能延遲**。
- 參與重要記帳活動、或跟敏感硬體互動的 kernel thread，優先權通常在 **40 到 60**。
- 不要有很多優先權超過 60 的即時 task：CPU 時間被它們**完全壟斷**，其他 task（包括很多 OS task）拿不到時間，很多重要的 kernel 活動被延遲。

### 5.3 `nice`：只能對別人好

`[34:02]` 一般程式（打開命令列、開 vi 或 emacs）跑在優先權 **120**。

問題：要不要給**一般使用者**提高**實際優先權**的權力？

> 「聽起來不合理，不然**大家都會這樣做**。所以給的是**反向**的權力。」

`[34:30]` 你可以**降低**自己的優先權：「如果我在做的事不重要，就當個好公民，降低優先權，讓我朋友的作業拿到更多優先權。」

這個指令就叫——**`nice`**：

```bash
nice -n 10 某指令        # 用優先權 130（120 + 10）跑這個指令
```

`[35:00]` 「130 的優先權其實是**低**的。我在**對人好（being nice）**。」

`[35:30]` **superuser（超級使用者）**：

- 【口頭】「superuser **不是 kernel**，只是系統管理員，比其他使用者權力大，但**還是使用者**。可以改檔案系統、裝更新、建使用者……」
- superuser 可以給**負的** nice 值：例如把 120 變成 105。
- **只有 superuser** 有這個權力。
- 但**不能**用這個機制把一般工作變成即時工作。

| 誰 | nice 值範圍 | 效果 |
|---|---|---|
| 一般使用者 | **1 到 19**（只能正的） | 只能**降低**優先權 |
| superuser（root） | **−20 到 19** | 可以升高或降低，但**最多到 100**，不會變成即時 |

> 【口頭】「一般我只能**往一個方向**對人好，除非我是 superuser，那就兩個方向都行。kernel 開發者還**蠻懂人性**的。」

### 5.4 kernel 把方向統一

`[36:30]` kernel 程式碼（課本 Listing 3.3）：

```c
else if (rt_policy(policy))
	prio = MAX_RT_PRIO - 1 - rt_prio;
else
	prio = NICE_TO_PRIO(nice);
```

| 情況 | 公式 | 例子 |
|---|---|---|
| **即時** | `prio = 100 − 1 − rt_prio` = `99 − rt_prio` | rt_prio 99 → **prio 0**；rt_prio 0 → **prio 99** |
| **一般** | `prio = 120 + nice` | nice 0 → **120**；nice 10 → **130**；nice −20 → **100** |

`[37:01]` 結果：**不管即時還是一般，`prio` 都是越低越好**。

```
  實際優先權
     ▲
  最高│●
     │  ╲
     │    ╲
     │      ╲              ← 統一後：單調遞減
     │        ╲
     │          ╲
  最低│            ●
     └──┼─────────┼──► kernel 內部的 prio
        0        139
```

`[38:00]` 「這段 kernel 程式碼**修正了這個小小的歷史產物**：`prio` 越小，實際優先權越高；`nice` 的邏輯也很漂亮地摺進來了。」

`[38:30]` 排程器是**差別待遇**的：

> 【口頭】「實際優先權越高（`prio` 越低）→ 給你越多 CPU 時間、優先存取資源。你就像**商務艙**乘客：頭等艙、商務艙、豪華經濟艙、經濟艙，不同等級不同服務。`prio` 0 是**頭等艙**；139 是**被擠下飛機的經濟艙乘客**。」

`[39:00]` 有了單調性，kernel 內部的邏輯就很簡單：**`prio` 越低，實際優先權越高。**

【課本補充】課本強調（p.78）：數字有兩個概念——**指定的數字**（0–139）和**怎麼解讀**。應用層對即時和一般有歷史原因的不同解讀，但 **kernel 程式碼裡要消除歧義、確保單調**——不能一下越小越好、一下越大越好。

### 5.5 【原始碼】`prio.h` 與 `__normal_prio()`

常數定義：

```c
// include/linux/sched/prio.h:5-27
#define MAX_NICE	19
#define MIN_NICE	-20
#define NICE_WIDTH	(MAX_NICE - MIN_NICE + 1)

/*
 * Priority of a process goes from 0..MAX_PRIO-1, valid RT
 * priority is 0..MAX_RT_PRIO-1, and SCHED_NORMAL/SCHED_BATCH
 * tasks are in the range MAX_RT_PRIO..MAX_PRIO-1. Priority
 * values are inverted: lower p->prio value means higher priority.
 */

#define MAX_RT_PRIO		100

#define MAX_PRIO		(MAX_RT_PRIO + NICE_WIDTH)
#define DEFAULT_PRIO		(MAX_RT_PRIO + NICE_WIDTH / 2)

/*
 * Convert user-nice values [ -20 ... 0 ... 19 ]
 * to static priority [ MAX_RT_PRIO..MAX_PRIO-1 ],
 * and back.
 */
#define NICE_TO_PRIO(nice)	((nice) + DEFAULT_PRIO)
#define PRIO_TO_NICE(prio)	((prio) - DEFAULT_PRIO)
```

把巨集算出來：

| 常數 | 計算 | 值 | 意思 |
|---|---|---|---|
| `MAX_NICE` | — | **19** | nice 最大 |
| `MIN_NICE` | — | **−20** | nice 最小 |
| `NICE_WIDTH` | 19 − (−20) + 1 | **40** | nice 共 40 個值 |
| `MAX_RT_PRIO` | — | **100** | 即時用 0–99 |
| `MAX_PRIO` | 100 + 40 | **140** | 總共 0–139 |
| `DEFAULT_PRIO` | 100 + 40/2 | **120** | 一般 process 的預設——教授說的「120」 |
| `NICE_TO_PRIO(nice)` | nice + 120 | | 課本說的「把 nice 加到 120」 |

第 10–13 行的註解直接寫明：「**Priority values are inverted: lower p->prio value means higher priority.**」——kernel 自己承認「值是反過來的：`prio` 越低，優先權越高」。

課本 Listing 3.3 的完整版本：

```c
// kernel/sched/core.c:2100-2112
static inline int __normal_prio(int policy, int rt_prio, int nice)
{
	int prio;

	if (dl_policy(policy))
		prio = MAX_DL_PRIO - 1;
	else if (rt_policy(policy))
		prio = MAX_RT_PRIO - 1 - rt_prio;
	else
		prio = NICE_TO_PRIO(nice);

	return prio;
}
```

| 行 | 意思 |
|---|---|
| 2104–2105 | **課本沒列的第一個分支**：`dl_policy`——**deadline 排程**（`SCHED_DEADLINE`）。`MAX_DL_PRIO` 是 0，所以 `prio = −1`：**比所有即時 task 還高** |
| 2106–2107 | 即時：`99 − rt_prio` |
| 2108–2109 | 一般：`120 + nice` |

所以完整的 kernel 內部優先權是：

```
  -1        0 ──────── 99    100 ──────── 139
 deadline   即時（越小越高）    一般（越小越高）
```

（deadline 排程是第 27 講「即時排程器」的內容，課本 §5.4.7。）

`task_struct` 裡存優先權的欄位：

```c
// include/linux/sched.h:783-786
	int				prio;
	int				static_prio;
	int				normal_prio;
	unsigned int			rt_priority;
```

| 欄位 | 意思 |
|---|---|
| `static_prio` | 由 nice 決定的**靜態**優先權（120 + nice） |
| `normal_prio` | `__normal_prio()` 算出來的「正常」優先權 |
| **`prio`** | 排程器**實際使用**的優先權。平常等於 `normal_prio`，但可能**暫時被提高**（例如**優先權繼承**，第 28 講） |
| `rt_priority` | 即時優先權（0–99，使用者設定的那個，越大越高） |

### 5.6 【實測】看自己的優先權

```bash
$ awk '{print "priority="$18, "nice="$19}' /proc/self/stat
priority=20 nice=0

$ nice -n 10 awk '{print "priority="$18, "nice="$19}' /proc/self/stat
priority=30 nice=10

$ nice -n -5 true
nice: cannot set niceness: Permission denied
```

- `/proc/<pid>/stat` 第 18 欄顯示的是 **`prio − 100`**：預設 120 → 顯示 **20**；nice 10 → 130 → 顯示 **30**。
- 一般使用者給**負的** nice → **Permission denied**——跟教授說的一樣，只有 superuser 可以。

**系統裡的即時 kernel thread**（`ps -eo pid,rtprio,cls,comm`，篩選 `CLS = FF`，也就是 `SCHED_FIFO` 即時排程）：

```text
    PID RTPRIO CLS COMMAND
     94     99  FF migration/19
     99     50  FF idle_inject/1
```

- **`migration/N`**：rt_priority **99**——整個系統最高。它負責把 task 在 CPU 之間搬移，必須能搶占任何人。
- **`idle_inject/N`**：rt_priority **50**——就是教授說的「kernel 自己的很多工作跑在 50」。

---

## 6. `sched_info`：給排程器的統計

`[39:31]` 還有一些**後設資訊（metadata）**，對統計和「要不要排這個 task」有用。（課本 Listing 3.4）

```c
// include/linux/sched.h:372-390
struct sched_info {
#ifdef CONFIG_SCHED_INFO
	/* Cumulative counters: */

	/* # of times we have run on this CPU: */
	unsigned long			pcount;

	/* Time spent waiting on a runqueue: */
	unsigned long long		run_delay;

	/* Timestamps: */

	/* When did we last run on a CPU? */
	unsigned long long		last_arrival;

	/* When were we last queued to run? */
	unsigned long long		last_queued;

#endif /* CONFIG_SCHED_INFO */
```

| 欄位 | 意思 | 教授說為什麼有用 |
|---|---|---|
| **`pcount`** | 在這顆 CPU 上跑過**幾次** | 在某顆 CPU 上跑得多 → 有選擇的話讓它**再跑同一顆**，因為**快取熱、TLB 熱**（熱 = 有它的資料） |
| **`run_delay`** | 在 run queue 裡**等了多久** | 等很久的應該**有一點優先**——公平性 |
| **`last_arrival`** | **上次**在 CPU 上跑是什麼時候 | 你在這顆 CPU 跑過，但**很久以前** → 資料很可能**已經不在**快取裡了 |
| **`last_queued`** | **上次**被放進 run queue 是什麼時候 | 你可能在等東西，等了很久才被放進 run queue |

`[41:00]` 「這些都是有趣的統計，告訴你一個程式的**過去歷史**。」

`[41:31]` 現在在第 3 章：**收集所有資訊**，讓第 5 章的排程器拿來用。「排程器想用什麼都可以，拿來做適當的決定。」所有這些都放在 `task_struct` 裡——「所以我說 `task_struct` 是巨大的結構，所以要存一個**指標**到它。」

【課本補充】CPU 裡的時間單位通常是**毫秒**或 **jiffy**（第 3 講）。

`[42:01]` 下一講：繞道去看**記憶體**相關的部分（`mm_struct`），之後再回來看 process 相關的部分（PID 等）。「之前是**單一** process 的事；之後會講**一群** process。」

---

## 7. 一頁速查表

### 7.1 Kernel stack

| 項目 | 教授 / 課本 | v6.2.12 x86-64 |
|---|---|---|
| 每個 thread 的 kernel stack | 8 KB（2 個 page） | **16 KB**（`THREAD_SIZE`，4 個 page） |
| 對齊 | 對齊到自己的大小 | 同 |
| 為什麼私有 | 所有 kernel thread 共用位址空間，但各 stack 排好不重疊 | 同 |
| 使用者 thread 和 kernel thread | 先當作分開；其實是同一個換衣服（化身博士） | — |

### 7.2 中斷 stack

| 項目 | 教授 / 課本 | v6.2.12 x86-64 |
|---|---|---|
| 為什麼另外用 | 敏感、可能不夠大、巢狀中斷 | — |
| 數量 | 每顆 CPU **7 個**，支援 7 層巢狀 | **TSS `ist[7]`：硬體限制 7 格**；綁給 DF、NMI、DB、MCE、VC |
| 一般中斷 | — | 每顆 CPU 一個 IRQ stack（16 KB） |
| NMI | 不可遮罩中斷 | 有專用 IST stack |

### 7.3 找 `current`

| | 舊方法 | 新方法（x86） |
|---|---|---|
| 位置 | `thread_info` 在 kernel stack 底部 | `gs` 指向的 per-CPU 區域 |
| 步驟 | ① stack 位址 AND `~(8KB−1)`；② 讀第一個欄位 | 一條 `mov %gs:偏移` |
| 指令數 | 2 | **1** |
| 還有誰在用 | C-SKY、Nios II、ARC、SuperH 等 | — |

### 7.4 其他選項為什麼不用

| 選項 | 問題 |
|---|---|
| 一般暫存器 | 數量少；編譯器管 |
| 全域陣列 | 指令多 |
| MSR | 不夠多、不可攜 |

### 7.5 優先權

| 項目 | 內容 |
|---|---|
| 範圍 | 0–139（加 deadline 的 −1） |
| 即時 | 0–99；使用者看：越大越高；99 最高 |
| 一般 | 100–139；越大越低；預設 120 |
| 建議 | 即時別超過 60–70；kernel 很多工作在 50 |
| `nice` | 一般使用者 +1 ~ +19（只能降）；root −20 ~ +19；不能變成即時 |
| kernel 統一 | 即時：`prio = 99 − rt_prio`；一般：`prio = 120 + nice`；**`prio` 越小越高** |
| `task_struct` 欄位 | `prio`、`static_prio`、`normal_prio`、`rt_priority` |
| `/proc/pid/stat` 第 18 欄 | `prio − 100` |

### 7.6 `sched_info`

| 欄位 | 意思 |
|---|---|
| `pcount` | 在這顆 CPU 跑過幾次 |
| `run_delay` | 在 run queue 等多久 |
| `last_arrival` | 上次跑的時間 |
| `last_queued` | 上次排進佇列的時間 |

### 7.7 本講看過的原始碼位置

| 檔案:行號 | 看什麼 |
|---|---|
| `arch/x86/include/asm/page_64_types.h:9-22` | `THREAD_SIZE` 16 KB、exception / IRQ stack 大小 |
| `arch/x86/include/asm/page_64_types.h:24-31` | IST 索引；「硬體上限 7 格」 |
| `arch/x86/include/asm/processor.h:270-289` | `struct x86_hw_tss`：`sp0`、`ist[7]`、「Linux 不用 ring 2」 |
| `arch/x86/include/asm/cpu_entry_area.h:48-56` | 專用 exception stack 的種類 |
| `arch/csky/include/asm/thread_info.h:51` | 舊方法：`sp & ~(THREAD_SIZE - 1)` |
| `arch/nios2/include/asm/thread_info.h:37` | 舊方法：`thread_info->task` |
| `arch/x86/include/asm/current.h:39-44` | 新方法：`gs` 讀 `current` |
| `include/linux/sched/prio.h:5-27` | `MAX_RT_PRIO`、`DEFAULT_PRIO`、`NICE_TO_PRIO` |
| `kernel/sched/core.c:2100-2112` | `__normal_prio()` |
| `include/linux/sched.h:783-786` | `prio`、`static_prio`、`normal_prio`、`rt_priority` |
| `include/linux/sched.h:372-390` | `struct sched_info` |

---

## 8. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **記憶體相關的 `task_struct` 欄位（`mm`）** | **第 9 講**：`mm_struct` 與 maple tree（下一講） |
| **一群 process、PID** | **第 10 講**：PID 與 namespace |
| **kernel stack、`sp0`、context 存在哪** | **第 12–13 講**：context switch 細節 |
| **中斷 stack、IST、NMI、巢狀中斷** | **第 15 講**：x86 中斷架構；**第 16 講**：exception |
| **使用者 thread 和 kernel thread 其實是同一個** | **第 22 講**：kernel 裡的並行 |
| **False sharing、快取線對齊** | **第 19 講**：memory model；**第 20–21 講**：鎖與並行佇列 |
| **`prio`、`nice`、`sched_info`、快取熱的 CPU** | **第 24–26 講**：排程、多處理器排程、Linux 排程（CFS 用 nice 算權重） |
| **即時優先權、deadline 排程（prio −1）** | **第 27 講**：即時排程器 |
| **`prio` 可能被暫時提高** | **第 28 講**：即時資源分配（優先權繼承） |
| **即時排程理論** | 另一門課 **Real Time Systems**：RMA、EDF、PIP、PCP |

---

## 附：本講一句話總結

> **使用者 thread 進 kernel 時換上「kernel 化身」，用一塊固定大小、互不重疊的 kernel stack（課本說 8 KB，x86-64 實際 16 KB），中斷則另用 CPU 專屬的 stack（x86 硬體 IST 剛好 7 格）；為了用最少指令找到目前的 `task_struct`，舊 kernel 把 `thread_info` 放在 stack 底部、清掉低位元再讀一次（2 條指令），新 kernel 改用 `gs` 一條指令搞定。優先權方面，使用者看到的即時（越大越高）與一般（越大越低）方向相反，kernel 用 `prio = 99 − rt_prio` 和 `prio = 120 + nice` 統一成「越小越高」，而一般使用者只能用 `nice` 對別人好。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 8（Chapter 3, Part II）錄影逐字稿（YouTube 自動字幕，43:12）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§3.1.5–3.1.8，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0）結果。*
