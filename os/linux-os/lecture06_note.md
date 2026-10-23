# Operating Systems: A Linux Kernel-Oriented Approach 第六講完整筆記：分段記憶體與 I/O（Chapter 2, Part V）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 6（Chapter 2, Part V）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（1:03:41，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§2.2.4、§2.3、§2.4（p.51–60）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
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
- [1. 虛擬記憶體的效率回顧](#1-虛擬記憶體的效率回顧)
  - [1.1 多層 page table 為什麼省空間](#11-多層-page-table-為什麼省空間)
  - [1.2 TLB 與 page fault：兩條規則](#12-tlb-與-page-fault兩條規則)
  - [1.3 踢掉一個 frame 很重](#13-踢掉一個-frame-很重)
- [2. 分段記憶體（Segmentation）](#2-分段記憶體segmentation)
  - [2.1 三段式：logical → linear → physical](#21-三段式logical--linear--physical)
  - [2.2 六個 segment 暫存器](#22-六個-segment-暫存器)
  - [2.3 明確指定 segment](#23-明確指定-segment)
- [3. 為什麼還要分段？](#3-為什麼還要分段)
  - [3.1 安全：隨機化 stack](#31-安全隨機化-stack)
  - [3.2 安全：資料存取改不到程式碼](#32-安全資料存取改不到程式碼)
  - [3.3 「資料錢包」：fs 與 gs](#33-資料錢包fs-與-gs)
- [4. Segment descriptor、GDT、SDC](#4-segment-descriptorgdtsdc)
  - [4.1 Segment descriptor 裡有什麼](#41-segment-descriptor-裡有什麼)
  - [4.2 LDT、GDT 與 SDC](#42-ldtgdt-與-sdc)
  - [4.3 現況：只剩 fs 和 gs](#43-現況只剩-fs-和-gs)
  - [4.4 【原始碼】GDT：base 全是 0](#44-原始碼gdtbase-全是-0)
  - [4.5 【原始碼】gs：per-CPU 資料與 `current`](#45-原始碼gsper-cpu-資料與-current)
  - [4.6 【原始碼】fs：thread local storage](#46-原始碼fsthread-local-storage)
- [5. I/O 系統：主機板](#5-io-系統主機板)
- [6. Port-mapped I/O（PMIO）](#6-port-mapped-iopmio)
  - [6.1 I/O 位址空間與 I/O port](#61-io-位址空間與-io-port)
  - [6.2 `in` 與 `out`](#62-in-與-out)
  - [6.3 開機前就能用](#63-開機前就能用)
  - [6.4 【原始碼】`inb` / `outb` 與標準 port](#64-原始碼inb--outb-與標準-port)
- [7. Memory-mapped I/O（MMIO）](#7-memory-mapped-iommio)
  - [7.1 把虛擬記憶體的一塊對應到裝置](#71-把虛擬記憶體的一塊對應到裝置)
  - [7.2 兩個好處](#72-兩個好處)
  - [7.3 【原始碼】`readl` / `writel`](#73-原始碼readl--writel)
- [8. DMA](#8-dma)
- [9. 第 2 章總結](#9-第-2-章總結)
- [10. 一頁速查表](#10-一頁速查表)
- [11. 與後續講次的連結](#11-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

`[00:32]` 第 2 章的最後一講。三件事：

| # | 主題 | 對應課本 |
|---|---|---|
| 1 | 虛擬記憶體的**效率**回顧 | §2.2.3 |
| 2 | x86 的**分段記憶體**（segmentation） | §2.2.4 |
| 3 | 三種 **I/O 機制**：PMIO、MMIO、DMA | §2.3 |

（⚠️ 教授開頭說「第二章的第五講」，但播放清單上是第 6 部影片；本章第 1 講是第 2 部影片，所以算起來是本章第 5 講，兩者一致。）

---

## 1. 虛擬記憶體的效率回顧

### 1.1 多層 page table 為什麼省空間

`[01:04]` 再複習一次：memory map **很稀疏**——只有底部（text、data、bss、heap）和頂部（stack）有東西，中間距離非常大。

`[02:00]` 所以看位址的**最高 9 位元**，只會看到很少的組合，level 1 page table 裡**真的有用的格子很少**。

【口頭】十進位的例子 `[03:00]`：

> 有 1000 個記憶體位置（000–999），只有最下面 99 個（000–098）和最上面 99 個（901–999）有用到。看**百位數**：不是 0 就是 9。所以 level 1 表只有 **2 格**有東西，其他都是空的。空的格子**不指向** level 2 表。

`[04:00]` level 2 也一樣很稀疏。越往下（從百位走到個位），**組合越多**，level 3、level 4 的表才多起來。

> 「我們只用**需要的** page table，**捕捉到稀疏性**，所以不會花太多空間存 page table。」

### 1.2 TLB 與 page fault：兩條規則

`[05:30]` 走 page table 很慢。就算 x86 有**硬體 page walker**，還是要**幾百個 cycle**，不可能每次轉換都這樣。所以用 **TLB** 快取映射。

`[06:30]` TLB miss（很少發生）→ 去 page table → 看 page 在主記憶體還是 swap space → 在 swap space 就是 **page fault**，要把 frame 搬進主記憶體，可能要先把另一個 frame 踢到 swap space。

`[08:01]` 教授再強調**兩條規則**：

| # | 規則 |
|---|---|
| 1 | **映射一定從 TLB 來**。不會直接從 page table 拿——page table → TLB → CPU 從 TLB 拿 |
| 2 | **資料一定從快取（或經過快取從主記憶體）來**。不會直接從 swap space 拿——frame 要先實際搬進主記憶體 |

### 1.3 踢掉一個 frame 很重

`[07:02]` 把 frame 從主記憶體踢出去**沒有聽起來那麼簡單**：

1. **清快取**：快取和 write buffer 裡不能有這個 frame 的資料。
2. 找出這個 frame 被**哪些虛擬 page** 對應到——可能跨好幾個 process。
3. 清理它們的 page table：標記「這個 frame 現在在 swap space」。
4. 從**所有可能有這筆映射的 TLB** 移除。

> 「把 frame 從主記憶體踢出去，是個**重量級**的過程。」

搬進來時也要更新 page table 和 TLB。

`[09:01]` 第 6 章會看到相關的資料結構，**反向映射（reverse mapping）特別難**。為什麼會有「一個 frame 對應多個 page」？因為**共享記憶體通道**：同一個實體 frame 對應到兩個 process 的虛擬 page。

> 【口頭】「好處是建立一個快速的共享記憶體通道，傳資料**不用 OS 介入**。這最重要——一旦 OS 介入，就是重量級的 system call 和 context switch，根本上就很慢。」

---

## 2. 分段記憶體（Segmentation）

### 2.1 三段式：logical → linear → physical

`[10:02]` 【口頭】「我們已經講過虛擬化了，現在**再虛擬化一次**——雙層。」這叫 **segmentation（分段）**，是 **x86 獨有**的。ARM 和 RISC-V **沒有**分段，只有一個扁平的虛擬空間對應到實體空間。

`[10:32]`（課本 Fig 2.14）

```
  CPU 產生
 ┌──────────────┐   + segment 的 base   ┌──────────────┐    TLB / page table   ┌──────────────┐
 │ Logical      │ ───────────────────► │ Linear       │ ───────────────────► │ Physical     │
 │ address      │                      │ address      │                      │ address      │
 │（邏輯位址）    │                      │（= 虛擬位址）  │                      │（實體位址）    │
 └──────────────┘                      └──────────────┘                      └──────────────┘
```

| 位址 | 誰產生 | 說明 |
|---|---|---|
| **Logical address（邏輯位址）** | CPU | — |
| **Linear address（線性位址）** | logical + **對應 segment 暫存器裡的 base** | Intel 的叫法；**就是我們說的虛擬位址** |
| **Physical address（實體位址）** | linear 經過 TLB 轉換 | — |

`[12:04]` 「**對應的** segment 暫存器」：CPU 會**根據存取類型**選 segment 暫存器。例如抓指令，自動選 **code segment 暫存器**，加上它的 base。

`[12:32]` memory map 在 linear 位址空間裡。所以用今天的話說，segment 就像是**memory map 裡的偏移量**。

### 2.2 六個 segment 暫存器

`[13:03]`（課本 Fig 2.13）x86 有**六個** segment 暫存器，歷史上是 **16 位元**，到現在還是：

| 暫存器 | 名稱 | 什麼時候用 |
|---|---|---|
| **`cs`** | Code segment | 抓指令（用 PC 時） |
| **`ss`** | Stack segment | stack 存取 |
| **`ds`** | Data segment | 資料存取 |
| **`es`**、**`fs`**、**`gs`** | Extra segments | 程式（或 OS）**自己決定**用途 |

`[13:30]` 早期 segment 暫存器用得很多（**現在沒那麼多了**）：

- 用 PC 加偏移 → 硬體知道要用 `cs`。
- stack page 的 TLB entry 有額外位元標記「這是 stack page」→ 用 `ss`。
- 資料 page → 用 `ds`。

【課本補充】`cs` 這類 segment 暫存器**只有特權軟體能改**；有些則可以由使用者 process 改。它們都是 **process context 的一部分**，換 process 時要一起換。

### 2.3 明確指定 segment

`[14:03]` AT&T 格式的記憶體運算元：

```asm
movl  %ebx, 4(%eax)        # logical address = eax + 4，segment 由硬體自動決定
movl  %ebx, %fs:4(%eax)    # 明確指定用 fs 的 base
```

- 一般情況下，segment 是**根據存取類型隱含決定**的，硬體夠聰明。
- 但可以**明確指定**：`%fs:` 前綴，覆蓋硬體的預設行為，要硬體用 `fs` 裡的 base。

`[15:30]` 為什麼到今天還是 16 位元？兩個原因：

1. **歷史包袱**。
2. 它們其實**不直接存 base**，而是指向一大堆資訊，叫 **segment descriptor**。base 只是其中一欄。（見 §4）

---

## 3. 為什麼還要分段？

`[18:00]` 已經用虛擬記憶體解決了相容和重疊問題，為什麼還要加一層？

`[18:31]` 有些**歷史原因**，但那些現在已經不是限制了。看**到 2025 年還重要**的理由。

【課本補充】歷史原因（p.51）：以前沒有虛擬記憶體，程式碼和資料存在**不同的裝置**上，實體位址空間分給各裝置，用 base–limit 系統，**segment 暫存器就是 base 暫存器**。base–limit 淘汰後，segment 暫存器失去原本用途，但有了新用途。

### 3.1 安全：隨機化 stack

`[19:01]` 安全現在是重大議題，以前不是。

- stack 上任何位址 = **`ss` 的內容 + 偏移**。
- segment 暫存器的內容是 **process context 的一部分**：換出時寫到記憶體某處，換入時讀回來。
- OS 可以在 process **第一次建立**時，把 `ss` 設成一個**隨機值**——攻擊者**猜不到**。

`[20:32]` 為什麼有用？

> 很多攻擊，包括有名的 **buffer overflow（緩衝區溢位）攻擊**，攻擊者需要**確切知道 stack 上某個變數的位址**。每次執行前把 `ss` 隨機化，就讓攻擊者很難做到。這是一個成功的對策。

【課本補充】課本把這叫 **stack obfuscation（stack 混淆）**。這類攻擊想改 stack 上存的**返回位址**，所以要知道返回位址存在哪。程式正確性**不受影響**，因為編譯時就假設所有 stack 位址都是「stack pointer + 偏移」。

### 3.2 安全：資料存取改不到程式碼

`[21:01]` memory map 裡，程式碼在底部，資料在上面。

- 把 `cs` 的 base 設在底部（程式碼），`ds` / `ss` 的 base 設在上面。
- 如果處理器**保證只產生正的 logical address**，那用 `ds` 算出來的位址**一定在程式碼上面**。
- 所以**資料存取不可能改到程式碼**。

`[22:03]` 這對安全是一大福音：駭進一個程式就是**執行時改它的程式碼**——病毒和惡意程式就是這樣做的。

`[22:30]` 早期還有：根據 linear 位址，把存取**送到不同裝置**——一個小記憶體放程式碼、一個大記憶體放資料，是**不同的硬體**。這個動機現在大多消失了。

`[23:00]` 第三個理由其實同源：**store 不能改 text section**。segment descriptor 裡有 **limit**（見 §4），可以規定：用 data segment 時，**不准**存取 code segment 那一段。程式碼只能在程式剛設定時被改，或由**已知安全**、PC 有登記的特定指令修改。

### 3.3 「資料錢包」：fs 與 gs

`[24:00]` 一個**很有趣**的用途，Linux kernel 大量使用 `fs` 和 `gs`。

`[24:32]` 想法：一個 process 想要一個**小小的「資料錢包」（data wallet）**——記憶體裡一小塊，存**非常重要**的東西。例如在 kernel 的記憶體空間裡，存「**上一個在跑的使用者 process**」的資訊。

> 【口頭】「data wallet 不是標準術語，是我們自己取的。」

`[25:31]` 這塊記憶體**存取非常頻繁**，每次都要**越快越好**。「能一條指令完成，就不想花五條。就是這麼關鍵。」

**問題** `[26:01]`：

- 位址是 **64 位元**的大數字。
- 要把這麼大的**立即值**（常數）載入暫存器，因為指令裡的立即值大小固定，要**好幾條指令**（分別載入低位、中間、高位），大概 3、4 條。
- 然後才能用那個暫存器當 base 去存取。太慢了。

**解法** `[27:00]`：

- 程式一開始就把這塊區域的**起始位址存進 `gs`**。
- 之後所有存取都**相對於 `gs` 的 base**。
- 只要**一個記憶體運算元、一條指令**就搞定。

> 【口頭】「本質上，我把 segment 暫存器當成一個**立即值的快取**。」

`[29:01]` 小結：

| 用途 | 現況 |
|---|---|
| 程式碼和資料放不同記憶體 | 已經消失 |
| **安全**（隨機化、隔離程式碼） | 還在 |
| **kernel 的快速資料錢包**（存在 segment 暫存器裡，整個 process 生命週期都在） | **還在**，追求存取速度 |

【課本補充】課本的說法（p.52）：kernel thread 常常需要快速存取一些資訊，例如**上一個在跑的使用者 process 的 id** 和它的部分 context。

---

## 4. Segment descriptor、GDT、SDC

### 4.1 Segment descriptor 裡有什麼

`[29:31]` 很早期 segment 暫存器只存 base，現在存的是**一個 segment descriptor 表的偏移（索引）**。descriptor 本身放在表裡。

`[30:01]` segment descriptor 的欄位：

| 欄位 | 意思 |
|---|---|
| **Base address** | 要加到 logical address 上的基底 |
| **Limit** | 這個 segment 最大能長多大 |
| **Type** | 資料、程式碼、stack 等 |
| **Execute** | 這個 segment 裡有沒有可執行的程式碼 |
| **Privilege level** | 使用者還是 kernel |

這些資訊幫我們**規範存取**、維持**權限和特權**的概念——基本上都是安全用途。

### 4.2 LDT、GDT 與 SDC

`[31:01]` x86 以前有兩種 descriptor 表：

| 表 | 說明 | 現況 |
|---|---|---|
| **LDT**（Local Descriptor Table） | 每個 process 一張 | **不用了**：process 可能太多 |
| **GDT**（Global Descriptor Table） | 硬體結構，**每個核心一張** | process 的 segment 暫存器指向 GDT 的某一格 |

`[31:32]` 又遇到跟 page table 一樣的問題：每次都去 GDT 撈 base **很慢**（就算是硬體做）。所以用**跟 TLB 一樣的招**：

```
 segment 暫存器（例如 cs 存 37）
        │
        ▼
     SDC（Segment Descriptor Cache）── 命中（幾乎都命中）──► 檢查權限 ──► base 加上 logical address
        │ 沒中（很少）
        ▼
       GDT ──► 讀出 descriptor，放進 SDC
```

`[32:00]` **SDC 命中率非常高**，因為 segment descriptor **幾乎不變**：只有換 process 時才變。一個 process 的生命週期中，例如 code segment 的 descriptor 是固定的。比 TLB 映射的變化**少得多**。

`[33:00]` 命中後，用 descriptor 裡的欄位檢查存取是否合法、有沒有違反保護或權限。檢查通過，base 就送去加到 logical address 上。

`[34:00]` 「這個機制看起來很重，但其實**不會拖慢程式**，非常快。」

【課本補充】課本補充（p.53）：

- 可以把 LDT 想成 L1（每個 process）、GDT 想成 L2。轉到 **32 位元** x86 時 LDT 就不用了。
- GDT 最多 **8191 格**（13 位元定址，保留一格）。
- GDT 也沒中的話，產生中斷，由 OS 填入正確的值，跟 TLB 一樣。

### 4.3 現況：只剩 fs 和 gs

`[34:32]` 現在 x86 為了向下相容保留了 SDC、GDT 這些硬體，但：

- **`cs`、`ds`、`ss`、`es` 這四個不用了**——**假設它們的 base 都是 0**。
- **只有 `fs` 和 `gs` 還在用**，而且是**自訂用途**（例如資料錢包）。

`[35:32]` GDT 也**不受歡迎了**，因為它的主要客戶（segment 暫存器）base 都假設是 0。`fs`、`gs` 還需要 base，但改用 **MSR（model-specific register，處理器特定暫存器）**：核心裡的特殊特權暫存器，提供**暫存器等級的快速存取**，比 SDC + GDT 快很多，功能上像一個 segment descriptor cache。

`[36:00]` 兩個暫存器的分工：

| 暫存器 | 誰用 | 用途 |
|---|---|---|
| **`gs`** | **kernel** | 存重要的 **per-CPU 資料**（每顆 CPU 自己的資料），在 kernel 的虛擬位址空間裡。「我們會看到 `gs` 被大量使用」 |
| **`fs`** | **GCC 編譯器**（使用者程式） | **thread local data（執行緒區域資料）**：不跟其他 thread 共享的資料 |

`[37:01]` 例子：

```asm
movl  $32, %fs:(%eax)    # 把 32 存到「fs 的 base + eax」
```

`fs` 是 segment 暫存器，`eax` 是區段開頭算起的偏移，displacement 是 0。

`[38:02]` 總結：segment 有多方面用途——**安全**、**分開程式碼和資料**（也是安全）、**指定資料錢包的位址**。

### 4.4 【原始碼】GDT：base 全是 0

教授說「`cs`、`ds`、`ss`、`es` 假設 base 為 0」。看 kernel 怎麼設 GDT：

```c
// arch/x86/kernel/cpu/common.c:205-220
DEFINE_PER_CPU_PAGE_ALIGNED(struct gdt_page, gdt_page) = { .gdt = {
#ifdef CONFIG_X86_64
	/*
	 * We need valid kernel segments for data and code in long mode too
	 * IRET will check the segment types  kkeil 2000/10/28
	 * Also sysret mandates a special GDT layout
	 *
	 * TLS descriptors are currently at a different place compared to i386.
	 * Hopefully nobody expects them at a fixed place (Wine?)
	 */
	[GDT_ENTRY_KERNEL32_CS]		= GDT_ENTRY_INIT(0xc09b, 0, 0xfffff),
	[GDT_ENTRY_KERNEL_CS]		= GDT_ENTRY_INIT(0xa09b, 0, 0xfffff),
	[GDT_ENTRY_KERNEL_DS]		= GDT_ENTRY_INIT(0xc093, 0, 0xfffff),
	[GDT_ENTRY_DEFAULT_USER32_CS]	= GDT_ENTRY_INIT(0xc0fb, 0, 0xfffff),
	[GDT_ENTRY_DEFAULT_USER_DS]	= GDT_ENTRY_INIT(0xc0f3, 0, 0xfffff),
	[GDT_ENTRY_DEFAULT_USER_CS]	= GDT_ENTRY_INIT(0xa0fb, 0, 0xfffff),
```

`GDT_ENTRY_INIT` 的三個參數是 **(flags, base, limit)**：

```c
// arch/x86/include/asm/desc_defs.h:23-35（節錄）
#define GDT_ENTRY_INIT(flags, base, limit)			\
	{							\
		.limit0		= (u16) (limit),		\
		.limit1		= ((limit) >> 16) & 0x0F,	\
		.base0		= (u16) (base),			\
		.base1		= ((base) >> 16) & 0xFF,	\
		.base2		= ((base) >> 24) & 0xFF,	\
		.type		= (flags & 0x0f),		\
		.s		= (flags >> 4) & 0x01,		\
		.dpl		= (flags >> 5) & 0x03,		\
		.p		= (flags >> 7) & 0x01,		\
		.avl		= (flags >> 12) & 0x01,		\
		.l		= (flags >> 13) & 0x01,		\
```

逐格看：

| GDT 格 | base | limit | 意思 |
|---|---|---|---|
| `KERNEL_CS` | **0** | `0xfffff`（最大） | kernel 的 code segment：從 0 開始、涵蓋全部 |
| `KERNEL_DS` | **0** | 最大 | kernel 的 data segment |
| `DEFAULT_USER_CS` | **0** | 最大 | 使用者的 code segment |
| `DEFAULT_USER_DS` | **0** | 最大 | 使用者的 data segment（也給 `ss` 用） |
| `*32_CS` | **0** | 最大 | 給 32 位元相容模式用 |

**所有 base 都是 0、limit 都是最大**——這叫**扁平模型（flat model）**：logical address = linear address，分段等於**沒作用**。這就是教授說的「假設 base 是 0」。

descriptor 裡的欄位對照教授列的：

| 欄位 | 從 flags 取 | 對應教授說的 |
|---|---|---|
| `base0/1/2` | 第 2 個參數，拆成 16 + 8 + 8 位元 | **Base address** |
| `limit0/1` | 第 3 個參數，拆成 16 + 4 位元 | **Limit** |
| `type` | flags 低 4 位元 | **Type**（程式碼 / 資料、可讀 / 可寫 / 可執行） |
| `dpl` | flags 位元 5–6 | **Privilege level**（Descriptor Privilege Level：0 = kernel、3 = user） |
| `p` | flags 位元 7 | Present |
| `l` | flags 位元 13 | **64 位元**程式碼段 |

用 flags 驗算：kernel CS 是 `0xa09b`，使用者 CS 是 `0xa0fb`，差在 `0x9b` vs `0xfb`。取位元 5–6：`0x9b = 1001 1011` → DPL = **0**；`0xfb = 1111 1011` → DPL = **3**。正好是 **ring 0** 和 **ring 3**（第 2 講的 `USER_RPL = 3`）。

為什麼 base 拆成三段（`base0`、`base1`、`base2`）？看結構定義：

```c
// arch/x86/include/asm/desc_defs.h:16-21
struct desc_struct {
	u16	limit0;
	u16	base0;
	u16	base1: 8, type: 4, s: 1, dpl: 2, p: 1;
	u16	limit1: 4, avl: 1, l: 1, d: 1, g: 1, base2: 8;
} __attribute__((packed));
```

因為**歷史包袱**：最早 descriptor 只有 16 位元 base，後來加寬時只能塞進剩下的空位，所以 base 被拆得七零八落。總共只有 **8 bytes**，base 最多 32 位元——**放不下 64 位元的 base**。這也是為什麼 64 位元模式下 `fs`、`gs` 的 base 要改用 MSR 存。

### 4.5 【原始碼】gs：per-CPU 資料與 `current`

kernel 用哪個 segment 暫存器存 per-CPU 資料？

```c
// arch/x86/include/asm/percpu.h:5-16
#ifdef CONFIG_X86_64
#define __percpu_seg		gs
#else
#define __percpu_seg		fs
#endif

#ifdef __ASSEMBLY__

#ifdef CONFIG_SMP
#define PER_CPU_VAR(var)	%__percpu_seg:var
#else /* ! SMP */
#define PER_CPU_VAR(var)	var
```

| 行 | 意思 |
|---|---|
| 5–6 | **64 位元**：per-CPU 用 **`gs`**（跟教授說的一樣） |
| 7–8 | 32 位元：用 `fs`（32 位元時使用者的 TLS 用 `gs`，剛好反過來） |
| 14 | `PER_CPU_VAR(var)` 在組合語言裡展開成 **`%gs:var`**——就是 §2.3 的「明確指定 segment」寫法 |
| 16 | 單核心（非 SMP）不需要分 per-CPU，直接用 `var` |

**第 2 講看過的 `entry_SYSCALL_64`** 裡就有：

```asm
movq	%rsp, PER_CPU_VAR(cpu_tss_rw + TSS_sp2)
movq	PER_CPU_VAR(pcpu_hot + X86_top_of_stack), %rsp
```

展開就是 `movq %rsp, %gs:(cpu_tss_rw + TSS_sp2)`——用 `gs` 一條指令就存取到「**這顆 CPU 自己的**」TSS 和 kernel stack 頂端。而那段開頭的 **`swapgs`** 指令，就是把使用者的 `gs` 換成 kernel 的 `gs`，讓這個「資料錢包」能用。

kernel 裡最常用的 per-CPU 資料，就是「**現在這顆 CPU 上在跑哪個 process**」：

```c
// arch/x86/include/asm/current.h:39-44
static __always_inline struct task_struct *get_current(void)
{
	return this_cpu_read_stable(pcpu_hot.current_task);
}

#define current get_current()
```

- `current` 是 kernel 裡**到處都在用**的巨集：指向目前 process 的 `task_struct`（第 3 章主角）。
- 實作就是 `this_cpu_read`：用 **`gs` 一條指令**讀出 per-CPU 變數 `pcpu_hot.current_task`。
- 這正是教授說的「資料錢包」：**頻繁存取、要一條指令完成**。也對應課本說的「存上一個在跑的 process 的資訊」。

### 4.6 【原始碼】fs：thread local storage

使用者程式設定 `fs` base 的 system call 是 `arch_prctl(ARCH_SET_FS, 位址)`：

```c
// arch/x86/kernel/process_64.c:779-794（節錄）
	case ARCH_SET_FS: {
		/*
		 * Not strictly needed for %fs, but do it for symmetry
		 * with %gs
		 */
		if (unlikely(arg2 >= TASK_SIZE_MAX))
			return -EPERM;

		preempt_disable();
		/*
		 * Set the selector to 0 for the same reason
		 * as %gs above.
		 */
		if (task == current) {
			loadseg(FS, 0);
			x86_fsbase_write_cpu(arg2);
```

| 行 | 意思 |
|---|---|
| 784–785 | 要設的 base（`arg2`）不能超過使用者空間上限 `TASK_SIZE_MAX`（第 5 講的 47 位元）——不能讓使用者把 `fs` 指到 kernel 空間 |
| 793 | `loadseg(FS, 0)`：把 `fs` 暫存器本身（selector，指向 GDT 的索引）設成 **0**——不用 GDT 了 |
| 794 | `x86_fsbase_write_cpu(arg2)`：**直接寫 base**。底層是 MSR 或 `wrfsbase` 指令 |

```c
// arch/x86/include/asm/fsgsbase.h:42-45
static __always_inline void wrfsbase(unsigned long fsbase)
{
	asm volatile("wrfsbase %0" :: "r" (fsbase) : "memory");
}
```

`wrfsbase` 是較新 x86 處理器的指令，直接把 64 位元值寫進 `fs` 的 base——印證教授說的「`fs`、`gs` 改用 MSR / 特殊暫存器，不走 GDT」。

glibc 在每個 thread 啟動時就用這個 system call，把 `fs` 指到那個 thread 自己的 TLS 區域。C 語言的 `__thread int x;`（或 C11 的 `_Thread_local`）編譯後就會變成 `%fs:偏移` 的存取。

---

## 5. I/O 系統：主機板

`[38:30]` 虛擬記憶體（含分段）講完了。接著看處理器的 I/O 機制，三種。

`[39:02]` **主機板（motherboard）**：一塊大的印刷電路板，上面有 CPU 和晶片。

- 主 CPU 很耗電、很燙，需要**散熱片（heat sink）**，表面積很大才能散熱，還有風扇。
- 有插記憶體模組的插槽。
- 很多晶片協助主機板跟 I/O 裝置溝通，這些晶片合稱**晶片組（chipset）**。

`[39:31]` Intel 主機板的經典示意圖（課本 Fig 2.15）。教授說**有點過時**，部分已經淘汰，但現在的設計還是很大程度沿用它的基本架構。

```
                ┌───────────────┐
                │      CPU      │  （多核心、多層快取都在裡面）
                └───────┬───────┘
                        │
   ┌─────┐       ┌──────┴──────┐
   │ GPU ├───────┤ Northbridge │──── 記憶體控制器 ──── DRAM
   └─────┘       └──────┬──────┘
                        │
   ┌─────────┐   ┌──────┴──────┐
   │PCI 插槽  ├───┤ Southbridge │──── 鍵盤、滑鼠、USB port、I/O 晶片……
   └─────────┘   └─────────────┘
```

| 元件 | 角色 |
|---|---|
| **Northbridge（北橋）** | 以前在 CPU 外面，**現在大部分移進 CPU 封裝裡**，變成一個概念上的元件。一部分是**記憶體控制器**（接 DRAM），一部分接 **GPU**。連的都是**超高頻寬**的裝置 |
| GPU 連線 | 以前用 **AGP** 匯流排，現在很多 GPU 接一般的 **PCI Express**（因為 PCIe 很快）——「所以我說這張圖有點過時」 |
| **Southbridge（南橋）** | ① 跟北橋來回搬資料；② 接晶片組裡較小的晶片（鍵盤、滑鼠、USB port、I/O 晶片）；③ 接業界標準匯流排，例如 **PCI（現在是 PCI Express）**，可以接 GPU、網路卡等週邊。頻寬比北橋低 |

南橋和北橋**一對一**連接，北橋再連到 CPU。

【課本補充】課本補充（p.54–55）：

- 處理器晶片有**幾百到上千根針腳**，大多是**供電和接地**，因為現代處理器電流很大、每根針腳能送的電流有限。只剩幾百根用來跟外部溝通。
- 記憶體控制器在晶片上，知道接了幾顆記憶體晶片、怎麼跟它們溝通——**OS 完全不知道**這層發生什麼。
- 晶片組裡可以有一棵**晶片樹**，訊息一層一層路由到 I/O 裝置。例如南橋把訊息送給 PCIe 晶片，再送到裝置。

---

## 6. Port-mapped I/O（PMIO）

### 6.1 I/O 位址空間與 I/O port

`[42:30]` 系統開機時，每個 I/O 裝置會被分配一組 **I/O 位址**。

- Intel 機器的 **I/O 位址空間**是 **64 KB** = 2¹⁶ 個位置，需要 **16 位元**位址。
- 每一個 byte 叫一個 **I/O port**。

`[43:00]` 注意：這個 **I/O port** 跟筆電側邊的 **HDMI port、USB port** 這種硬體接口**完全不同**，只是字一樣，要看上下文判斷。

- I/O port 是一個**儲存位置的抽象**。
- 開機時把一組 I/O port 分給每個裝置。例如告訴網路卡：「64 KB 裡，4 KB 到 8 KB 是你的。」
- OS 開機後可以查詢這個資訊，知道鍵盤分到哪些 I/O 位址、這些位址有什麼**語意**。

`[44:30]` 例子：鍵盤按 Ctrl+Alt+Del 是三個鍵——語意是已知的。鍵盤把東西寫進 I/O port，CPU 讀；CPU 寫東西進去，裝置讀。一般鍵盤、滑鼠不會從 CPU 讀東西，但網路卡可以**雙向**傳。

### 6.2 `in` 與 `out`

`[45:01]` 整體抽象：

```
       CPU                    I/O port（概念上的位址）              I/O 裝置
  ┌──────────┐      out 19      ┌─────────────────┐              ┌────────┐
  │          │ ───────────────► │ 17 │ 18 │ 19 │ 20│ ◄──────────► │ 裝置    │
  │          │ ◄─────────────── │                 │              │        │
  └──────────┘      in 19       └─────────────────┘              └────────┘
                     晶片組負責把讀寫轉給正確的裝置
```

- 例如裝置分到 I/O port **17 到 20**。CPU 寫 port 19，資料就**直接到這個裝置**；裝置已經把 19 對應到某個內部功能，知道要做什麼。
- 裝置要回傳：寫某個 status 到某個 I/O port，CPU 讀的時候就拿到了。

`[46:30]` 存取大小：

- 一次 **1 byte**（單一 I/O port）。
- 也可以**合併相鄰的** port：**2 bytes**（2 個 port）或 **4 bytes**（4 個連續 port）。

`[47:01]` 主機板硬體負責：CPU 一發出 **`out` 指令**（加上 port 號碼和資料），就直接送到裝置。如果裝置暴露了內部記憶體，就對應到 I/O port，寫到該寫的地方。

`[48:01]` I/O port 是**概念上的東西**：裝置分到 4 個 port，**不代表**主機板上有一個 4 格的暫存器。某個 port 可能對應裝置內部或主機板上裝置控制器裡的某個儲存位置，也可能是**寫死的值**——「收到這個 port 號碼，就回傳某個值」。

`[49:00]` CPU 讀的時候只說「我要讀，port 號碼是 X」，主機板把請求送到正確的裝置，裝置的控制器知道要回什麼。

| 指令 | 方向 |
|---|---|
| **`in`** | 從 port **讀** |
| **`out`** | **寫**到 port |

這是**很慢、低頻寬**的通道，不可擴展。

### 6.3 開機前就能用

`[49:32]` 但這個機制**非常基本**，存在很久了，而且**在 OS 開機前**就能運作：

> 【口頭】「OS 開機前，**BIOS** 會印很多東西，還有一個小畫面讓你選從哪個裝置開機——隨身碟、硬碟、網路。這是怎麼辦到的？就是 BIOS 程式還在跟裝置溝通，這時候 port 就很好用。」

OS 接手開機後，**低頻寬溝通**還是靠這些 port。晶片組負責把 I/O 位址空間分給各裝置；CPU 上的程式只要發 `in` 和 `out` 兩種指令，「神奇地」就能讀寫裝置。

`[51:02]` 這叫 **port-mapped I/O（PMIO）**：雖然有缺點，但存在很久、**非常穩固**，只是頻寬很低。

【課本補充】課本補充（p.55–56）：

- 開機後 OS 首先要做的事之一，就是知道接了哪些裝置、對應到哪些 I/O port，並把這資訊交給 **device driver**。
- 使用者程式請 OS 做 I/O → OS 交給 driver → driver 用 `in` / `out` 的變體跟裝置溝通 → 完成後把資料和狀態交回程式。
- 用了**五十年**。傳 1–4 bytes 就要一條指令，適合**控制訊息**，不適合網路卡這種高頻寬裝置的**資料訊息**。
- 現在 I/O port 主要用在 **BIOS**、**簡單裝置（如 LED）**、**嵌入式系統**。

### 6.4 【原始碼】`inb` / `outb` 與標準 port

kernel 用巨集一次生出 `in`/`out` 的 1、2、4 bytes 版本：

```c
// arch/x86/include/asm/shared/io.h:7-25
#define BUILDIO(bwl, bw, type)						\
static inline void __out##bwl(type value, u16 port)			\
{									\
	asm volatile("out" #bwl " %" #bw "0, %w1"			\
		     : : "a"(value), "Nd"(port));			\
}									\
									\
static inline type __in##bwl(u16 port)					\
{									\
	type value;							\
	asm volatile("in" #bwl " %w1, %" #bw "0"			\
		     : "=a"(value) : "Nd"(port));			\
	return value;							\
}

BUILDIO(b, b, u8)
BUILDIO(w, w, u16)
BUILDIO(l,  , u32)
#undef BUILDIO
```

| 部分 | 意思 |
|---|---|
| `u16 port` | port 號碼是 **16 位元**——對應 64 KB 的 I/O 位址空間 |
| `"out" #bwl` | 產生 `outb`、`outw`、`outl` 指令（`#` 把參數變成字串） |
| `"a"(value)` | 資料一定要放在 **`al` / `ax` / `eax`**（x86 規定 `in`/`out` 只能用 a 暫存器） |
| `"Nd"(port)` | port 放在 **`dx`**，或是 0–255 的常數 |
| `BUILDIO(b, b, u8)` | 生出 `__outb` / `__inb`：**1 byte** |
| `BUILDIO(w, w, u16)` | **2 bytes**（合併 2 個 port） |
| `BUILDIO(l, , u32)` | **4 bytes**（合併 4 個 port） |

正好是教授說的「1、2、4 bytes」。又是第 4 講說的「巨集讓 C 看起來不像 C」。

**哪些 port 分給誰？** PC 傳統上有一組**固定**的 port：

```c
// arch/x86/kernel/setup.c:630-651（節錄）
static struct resource standard_io_resources[] = {
	{ .name = "dma1", .start = 0x00, .end = 0x1f,
		.flags = IORESOURCE_BUSY | IORESOURCE_IO },
	{ .name = "pic1", .start = 0x20, .end = 0x21,
		.flags = IORESOURCE_BUSY | IORESOURCE_IO },
	{ .name = "timer0", .start = 0x40, .end = 0x43,
		.flags = IORESOURCE_BUSY | IORESOURCE_IO },
	...
	{ .name = "keyboard", .start = 0x60, .end = 0x60,
		.flags = IORESOURCE_BUSY | IORESOURCE_IO },
	{ .name = "keyboard", .start = 0x64, .end = 0x64,
		.flags = IORESOURCE_BUSY | IORESOURCE_IO },
	...
```

| 名稱 | Port 範圍 | 是什麼 |
|---|---|---|
| `dma1` | `0x00`–`0x1f` | 傳統 DMA 控制器（§8） |
| `pic1` | `0x20`–`0x21` | 傳統中斷控制器 |
| `timer0` | `0x40`–`0x43` | **傳統 timer 晶片**——第 3 講的 timer chip |
| `keyboard` | `0x60`、`0x64` | **鍵盤**——教授舉的例子 |
| `pic2`、`dma2`、`fpu` | `0xa0`… | 第二個中斷控制器、DMA、浮點 |

在 Linux 上可以 `cat /proc/ioports` 看。我在本機用**一般使用者**跑，名稱都有（`dma1`、`pic1`、`timer0`、`keyboard`……），但**位址全部顯示成 `0000-0000`**——kernel 對非 root 使用者隱藏真實位址（安全考量）。要用 `sudo` 才看得到。

---

## 7. Memory-mapped I/O（MMIO）

### 7.1 把虛擬記憶體的一塊對應到裝置

`[51:02]` 更快的方法：**memory-mapped I/O**。（課本 Fig 2.16）

```
  Process 的虛擬位址空間                 I/O 裝置
  ┌─────────────────┐
  │                 │
  ├─────────────────┤    映射到 I/O 位址    ┌──────────────┐
  │ 這一塊           │ ──────────────────► │ 裝置的 port / │
  ├─────────────────┤                     │ 內部記憶體     │
  │                 │                     └──────────────┘
  └─────────────────┘
```

- 把虛擬位址空間的**一塊區域**，對應到裝置的 port 或**內部記憶體**。
- 用**一般的 load 和 store** 指令：store 到這裡 → 直接送到裝置；load 這裡 → 從裝置讀。
- 晶片組和主機板負責把**記憶體存取轉成裝置存取**。

`[52:32]` **設定很慢**：牽涉很多角色（process、晶片組的晶片、裝置本身）。但**設好之後**，讀裝置就是一條 load，寫裝置就是一條 store——「寫我自己的虛擬位址空間，讀寫就神奇地送到裝置。」

`[53:00]` 怎麼做到自動路由？**TLB entry**：

- 平常 TLB entry 存實體位址。
- 映射到裝置時，存的是 **I/O 位址**，並記錄「這筆映射的另一端**不在主記憶體**，在某個 I/O 裝置、裝置內的某個位址」。
- CPU 和主機板就知道怎麼把請求送到裝置、再帶回來。

### 7.2 兩個好處

`[54:01]`

| 好處 | 說明 |
|---|---|
| **① 優雅** | process 只要做 load 和 store，其他什麼都不用。寫高階程式碼很容易抽象化：**記憶體讀寫和 I/O 讀寫在概念上一樣** |
| **② 一條指令大量傳輸** | x86 有 **`rep movs`** 和 **`rep stos`** 這類指令（s = string） |

`[55:02]` `rep` 前綴的指令：

- 給兩個記憶體位址，把**一大串 bytes**（string）從來源搬到目的地。
- 用 **`esi` / `edi`**（64 位元是 **`rsi` / `rdi`**）當來源索引和目的索引——第 3 講說的「`si`、`di` 是為了把整個迴圈塞進一條指令」，就是這個。
- **一條指令裡內建一個 for 迴圈**，整個傳輸一條指令完成。

`[56:00]` 「省下的是**指令數量**。」相比 PMIO 每 1–4 bytes 一條指令，這裡一條複雜的 CISC 指令就能在記憶體和 I/O 裝置之間搬一整串資料。

`[57:00]` 所以 MMIO 用得**非常大量**：既優雅，又能用 x86 的 CISC 指令最佳化傳輸。

【課本補充】課本補充（p.57）：

- I/O 裝置和晶片組也演進了：除了傳統的 port 介面，也加了**小記憶體**，傳輸中的資料可以暫存在那裡。
- 到 **2025 年** MMIO 非常流行，手冊裡常簡稱 **MMIO**。
- **Zero-copy（零複製）**：從裝置讀進來的資料，driver 只要**改 page 映射**，把那些 page 直接對應給使用者程式就好，不用先讀進 OS 專用的 page、再複製一次到使用者 page。

### 7.3 【原始碼】`readl` / `writel`

kernel 的 driver 存取 MMIO 不直接寫指標，而是用這組函式：

```c
// arch/x86/include/asm/io.h:49-69（節錄）
#define build_mmio_read(name, size, type, reg, barrier) \
static inline type name(const volatile void __iomem *addr) \
{ type ret; asm volatile("mov" size " %1,%0":reg (ret) \
:"m" (*(volatile type __force *)addr) barrier); return ret; }

#define build_mmio_write(name, size, type, reg, barrier) \
static inline void name(type val, volatile void __iomem *addr) \
{ asm volatile("mov" size " %0,%1": :reg (val), \
"m" (*(volatile type __force *)addr) barrier); }

build_mmio_read(readb, "b", unsigned char, "=q", :"memory")
build_mmio_read(readw, "w", unsigned short, "=r", :"memory")
build_mmio_read(readl, "l", unsigned int, "=r", :"memory")
...
build_mmio_write(writeb, "b", unsigned char, "q", :"memory")
build_mmio_write(writew, "w", unsigned short, "r", :"memory")
build_mmio_write(writel, "l", unsigned int, "r", :"memory")
```

跟 PMIO 的 `inb`/`outb` 比一比：

| | PMIO | MMIO |
|---|---|---|
| 函式 | `inb` / `outb` | `readb` / `writeb`、`readl` / `writel` |
| 底層指令 | **`in` / `out`**（特殊 I/O 指令） | **`mov`**（就是一般的 load / store） |
| 位址型別 | `u16 port`（16 位元 port 號碼） | `void __iomem *addr`（一個**指標**，虛擬位址） |

所以 MMIO 真的就是「**一般的 `mov` 指令**」，跟教授說的一樣。那為什麼不直接 `*addr = val`？

| 部分 | 原因 |
|---|---|
| `volatile` | 告訴編譯器：**每次都要真的去讀寫**，不准最佳化掉、不准合併。對一般記憶體，寫兩次同一個值可以省掉一次；對裝置，寫兩次可能代表兩個命令 |
| `"memory"`（barrier） | 告訴編譯器不要把前後的記憶體存取**重新排序**越過這條指令 |
| `__iomem` | 一個標記，讓檢查工具（sparse）抓出「把 I/O 位址當一般指標用」的錯誤 |

driver 要先用 **`ioremap()`** 把裝置的實體位址映射到 kernel 的虛擬位址空間，拿到 `__iomem` 指標——這就是教授說的「**設定很慢**」那一步。之後就只是 `readl` / `writel`。

---

## 8. DMA

`[57:30]` PMIO 和 MMIO 都是**同步**的：要**等 I/O 做完**，做完之前什麼都不能做。

- 讀的話還說得過去。
- **寫**的話就沒什麼道理。
- 資料量大的時候，更希望有一個**晶片外的單元**來做。

`[57:30]` **DMA（Direct Memory Access，直接記憶體存取）**：（課本 Fig 2.17）

```
         1. 傳輸細節（裝置 ↔ 記憶體）
  CPU ─────────────────────────────────►  DMA 控制器
   ▲                                          │
   │     2. 中斷 CPU（傳完了）                  │ 自己搬資料
   └──────────────────────────────────────────┘
                                         主記憶體 ↔ I/O 裝置
```

1. **DMA 晶片**（也叫 **DMA 控制器**）是主機板上的外部晶片。
2. CPU 把傳輸工作**外包**給它：「主記憶體裡這些實體 frame 的資料，要傳到這個 I/O 裝置」，或反過來。
3. `[58:30]` DMA 一般是**主記憶體 ↔ I/O**，所以**快取要先失效（invalidate）**。
4. `[59:00]` 給 DMA 控制器什麼？起始指標，或**一串 frame 清單**加上裝置內的目標位置。
   - 可以給虛擬區域再轉換，但「**讓 DMA 控制器碰 TLB 不是好主意**」，所以給它實體 frame 清單。
5. DMA 控制器**自己完成**整個傳輸。
6. 傳完 → **發中斷**給 CPU → 也就是告訴 OS「傳完了」。
7. `[60:02]` OS 再通知發起的應用程式（可能是 kernel process 或使用者 process）：讀的話可以開始處理資料，寫的話可以當作寫完了。

`[60:30]` 好處：**非同步（asynchronous）**——CPU 不用等，可以去做別的事。

| 機制 | 同步 / 非同步 | 適合 |
|---|---|---|
| PMIO、MMIO | **同步**：CPU 要等 | 小筆資料 |
| DMA | **非同步**：CPU 可以去做別的 | **大量資料**，而且希望讀寫裝置時能**平行**做別的事 |

【課本補充】課本補充（p.58）：

- 使用者程式**不能直接**發 DMA 請求，要發 **system call**，讓 OS 知道它想存取 I/O 裝置。介面可以很簡單，因為只有 **OS 的 device driver** 會跟 DMA 晶片溝通。
- 收到 DMA 的中斷，OS 知道怎麼通知 driver。很多時候也要讓使用者程式知道——例如印表機印完一頁，圖示從「列印中」變成「列印完成」，可以用 **signal**。

> 補充（不是教授講的）：v6.2.12 裡 driver 用的 DMA API 在 `include/linux/dma-mapping.h`，例如 `dma_map_single()`（第 411 行）——把一塊 kernel 記憶體準備好給裝置做 DMA，回傳一個裝置看得懂的 `dma_addr_t` 位址。這對應教授說的「給 DMA 控制器實體 frame，而不是讓它碰 TLB」。細節第 7 章（I/O 系統）。

---

## 9. 第 2 章總結

`[61:31]` 教授的整章回顧：

| 主題 | 內容 |
|---|---|
| **核心、暫存器、中斷** | 特權暫存器、ring——一般程式設計師看不到、需要 OS 等級權限的部分 |
| **x86 組合語言** | AT&T 格式，夠讀懂課本其他部分 |
| **虛擬記憶體** | TLB、page table、swap space、**分段**；多個 process 的記憶體空間怎麼分開 |
| **三種 I/O** | **PMIO**：古老、傳統、慢，但幾乎都支援、幾乎不用設定。**MMIO**：虛擬位址空間的一塊對應到裝置位址空間。**DMA**：純外包 |

`[63:02]` 下一章：**process**。「process 是 kernel 的心臟。」會講 process 的表示、建立、銷毀、context switch。

【課本補充】課本的第 2 章摘要（Summary 2.4.1，節錄）：

1. 每個核心有一般暫存器和特權暫存器。後者給 OS 或 VMM 用，可以在很低的層級控制硬體。
2. 現代處理器定義四個特權 ring：**OS kernel 在 ring 0、應用程式在 ring 3**，middleware 等用中間的 ring。

---

## 10. 一頁速查表

### 10.1 虛擬記憶體兩條規則

| # | 規則 |
|---|---|
| 1 | 映射**一定**從 TLB 來 |
| 2 | 資料**一定**從快取 / 主記憶體來，不會直接從 swap space |

### 10.2 分段

| 項目 | 內容 |
|---|---|
| 誰有 | **只有 x86**；ARM、RISC-V 沒有 |
| 流程 | logical →（+ segment base）→ linear（= 虛擬）→（TLB）→ physical |
| 六個暫存器 | `cs`（程式碼）、`ss`（stack）、`ds`（資料）、`es`/`fs`/`gs`（自訂）；16 位元 |
| 明確指定 | `%fs:4(%eax)` |
| Descriptor 欄位 | base、limit、type、execute、privilege level |
| 查找 | segment 暫存器 → SDC（快取）→ GDT（每核心一張，最多 8191 格）；LDT 已淘汰 |
| 現在的用途 | 安全（stack 隨機化、資料改不到程式碼）、**資料錢包** |
| x86-64 現況 | `cs`/`ds`/`ss`/`es` base = 0（扁平模型）；只用 `fs`、`gs`，base 存在 MSR |
| `gs` | **kernel** 的 per-CPU 資料（例如 `current`） |
| `fs` | **使用者**的 thread local storage（GCC） |

### 10.3 三種 I/O

| | PMIO | MMIO | DMA |
|---|---|---|---|
| 怎麼做 | `in` / `out` 指令 | 一般 load / store（`mov`） | 外包給 DMA 晶片 |
| 位址 | 64 KB I/O 位址空間，16 位元 port | 虛擬位址空間的一塊 | 實體 frame 清單 |
| 每次 | 1、2、4 bytes | 可以一大串（`rep movs`） | 一大塊 |
| 同步？ | 同步 | 同步 | **非同步**，完成後發中斷 |
| 設定成本 | 幾乎沒有 | 高（`ioremap`） | 中 |
| 速度 | 慢 | 快 | 最快、CPU 不用等 |
| 用在 | BIOS、LED、嵌入式、控制訊息 | 大部分現代裝置 | 大量資料 |
| kernel 函式 | `inb` / `outb` | `readl` / `writel` | `dma_map_single` |

### 10.4 本講看過的原始碼位置

| 檔案:行號 | 看什麼 |
|---|---|
| `arch/x86/kernel/cpu/common.c:205-220` | GDT 內容：所有 base = 0 |
| `arch/x86/include/asm/desc_defs.h:16-35` | segment descriptor 結構、`GDT_ENTRY_INIT` |
| `arch/x86/include/asm/percpu.h:5-16` | 64 位元 per-CPU 用 `gs`、`PER_CPU_VAR` |
| `arch/x86/include/asm/current.h:39-44` | `current`：用 `gs` 讀目前 process |
| `arch/x86/kernel/process_64.c:779-794` | `ARCH_SET_FS`：設定 `fs` base |
| `arch/x86/include/asm/fsgsbase.h:42-45` | `wrfsbase` 指令 |
| `arch/x86/include/asm/shared/io.h:7-25` | `inb`/`outb` 等：PMIO |
| `arch/x86/kernel/setup.c:630-651` | 傳統 PC 的固定 I/O port |
| `arch/x86/include/asm/io.h:49-69` | `readl`/`writel` 等：MMIO |
| `include/linux/dma-mapping.h:411` | `dma_map_single` |

---

## 11. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **Process、`current`、`task_struct`** | **第 7–11 講**：process 狀態、kernel stack、`mm_struct`、PID、process 建立（第 3 章） |
| **`gs` 存上一個 process 的資訊、segment 暫存器是 context** | **第 12–13 講**：context switch |
| **`swapgs`、per-CPU 資料** | **第 14–15 講**：system call、中斷進入路徑 |
| **DMA 完成發中斷** | **第 15–16 講**：中斷架構、softirq |
| **I/O 完成用 signal 通知程式** | **第 17 講**：signal |
| **`fs` 與 thread local storage** | **第 18 講**：多執行緒 |
| **共享記憶體通道、反向映射** | **第 31 講**：reverse mapping |
| **PMIO、MMIO、DMA、device driver、block device** | 課本第 7 章（I/O 系統）。對應講次**待補** |

---

## 附：本講一句話總結

> **x86 在虛擬記憶體之上還有一層「分段」：logical 位址加上 segment base 才是 linear（虛擬）位址；如今 `cs`/`ds`/`ss` 的 base 都是 0，只剩 `fs`（使用者的 thread local）和 `gs`（kernel 的 per-CPU「資料錢包」，例如 `current`）還在用。I/O 則有三種方式：古老但開機前就能用的 PMIO（`in`/`out`）、把裝置映射進位址空間用一般 `mov` 存取的 MMIO、以及把整個傳輸外包、做完才發中斷的 DMA。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 6（Chapter 2, Part V）錄影逐字稿（YouTube 自動字幕，1:03:41）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§2.2.4、§2.3–2.4，以及 Linux kernel v6.2.12 原始碼整理。*
