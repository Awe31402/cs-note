# Operating Systems: A Linux Kernel-Oriented Approach 第三十講完整筆記：Kernel 的記憶體架構（Chapter 6, Part II）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 30（Chapter 6, Part II：Memory architecture of the kernel）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（57:58，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§6.2（p.297–314）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
> **標記說明**：
> - `【口頭】`：教授在課堂上口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與教授回答
> - `【課本補充】`：課本有、但這堂課沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（`檔名:行號`），用來對照觀念
> - `【實測】`：在本機（Ubuntu、kernel 6.5.0、x86-64、Intel i7-13650HX）實際執行的結果
> - `⚠️ 逐字稿缺字`：自動字幕在這裡掉字或聽不清，內容依上下文或課本補上；也用來標示課堂或課本說法需要修正之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 這堂課要做什麼](#0-這堂課要做什麼)
- [1. 虛擬記憶體地圖](#1-虛擬記憶體地圖)
  - [1.1 57 位元：128 PB，一人一半](#11-57-位元128-pb一人一半)
  - [1.2 Direct-mapped 區域](#12-direct-mapped-區域)
  - [1.3 其他區域](#13-其他區域)
  - [1.4 【原始碼】官方文件的地圖](#14-原始碼官方文件的地圖)
- [2. 頁表：五層](#2-頁表五層)
  - [2.1 結構](#21-結構)
  - [2.2 【原始碼】`follow_pte`：走頁表](#22-原始碼follow_pte走頁表)
  - [2.3 【原始碼】PMD 這一層怎麼走](#23-原始碼pmd-這一層怎麼走)
  - [2.4 PTE 的內容](#24-pte-的內容)
- [3. Page 與 folio](#3-page-與-folio)
  - [3.1 `struct page`](#31-struct-page)
  - [3.2 Folio](#32-folio)
  - [3.3 Huge page](#33-huge-page)
  - [3.4 【實測】Huge page 的效果](#34-實測huge-page-的效果)
- [4. PFN 與 `struct page` 的轉換](#4-pfn-與-struct-page-的轉換)
  - [4.1 Section 與偏移指標（biased pointer）](#41-section-與偏移指標biased-pointer)
  - [4.2 ⚠️【原始碼】x86-64 實際用的是 vmemmap](#42-️原始碼x86-64-實際用的是-vmemmap)
- [5. 管理 TLB](#5-管理-tlb)
  - [5.1 換 process 就要 flush](#51-換-process-就要-flush)
  - [5.2 ASID 與 PCID](#52-asid-與-pcid)
  - [5.3 ⚠️【原始碼】數字搞反了：Intel 有 4096 個、Linux 只用 6 個](#53-️原始碼數字搞反了intel-有-4096-個linux-只用-6-個)
  - [5.4 Lazy TLB 模式](#54-lazy-tlb-模式)
- [6. 分割實體記憶體](#6-分割實體記憶體)
  - [6.1 NUMA](#61-numa)
  - [6.2 Zone](#62-zone)
  - [6.3 【原始碼】`enum zone_type`](#63-原始碼enum-zone_type)
  - [6.4 `struct zone` 與 `pglist_data`](#64-struct-zone-與-pglist_data)
  - [6.5 【實測】本機的 zone](#65-實測本機的-zone)
- [7. 一頁速查表](#7-一頁速查表)
- [8. 與後續講次的連結](#8-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

[00:32] 看 Linux 的**虛擬與實體位址空間**：

1. 虛擬記憶體地圖（kernel 和使用者各占哪裡）。
2. 五層頁表、怎麼走。
3. `struct page`、folio、huge page。
4. TLB：ASID / PCID、lazy TLB。
5. 實體記憶體：NUMA、zone、section。

---

## 1. 虛擬記憶體地圖

### 1.1 57 位元：128 PB，一人一半

[00:32] Linux 支援兩種虛擬位址長度：**48 位元**和 **57 位元**。教授用 57 位元，因為比較大、比較一般。

[01:03] 2⁵⁷ = 2⁷ × 2⁵⁰ = **128 PB**（2³⁰ = G、2⁴⁰ = T、2⁵⁰ = P）。

[01:30]

- **64 PB 給使用者空間**。
- **64 PB 給 kernel**。一半一半。

[01:30]–[02:01] kernel 這一半再切成好幾個區域，區域之間留**空洞**：

- 讓區域有成長的空間。
- 除錯用：有 bug 時程式會闖進這些空洞，**空洞裡什麼都不該有**，很容易發現。

【課本補充】使用者和 kernel 的虛擬位址**不重疊**，嚴格分開。使用者空間再分成 text、stack、heap 等（第 4 講）。

### 1.2 Direct-mapped 區域

[02:01] 最值得注意的是 **direct-mapped（直接對應）區域**。

[02:01]–[02:32] 先想最簡單的情況：如果**虛擬位址 = 實體位址**，那就等於**直接存取記憶體**。處理器不知道你要不要轉換，一律會轉換；但如果故意把對應設成一樣，就能直接存取。

[03:00] 但這塊區域**不是從 0 開始**的，所以關係是：

```
實體位址 = 虛擬位址 − 一個固定的偏移量
```

[03:32] 還是很容易互相換算。kernel 用它來**隨意存取任何實體位址**，這本來就是 kernel 的工作。

[03:32]–[04:31] 用途：

- 頁表等結構放在保留的實體記憶體裡，用固定偏移直接存取。
- 不想要虛擬記憶體帶來的**缺頁風險**、想把 page **釘在**記憶體裡的東西：系統表、頁表……

【課本補充】課本：kernel 有時一定要用真實的實體位址，特別是跟 I/O 裝置、DMA 控制器、開機子系統打交道時。頁表不該放在會缺頁的虛擬記憶體裡。也很適合做跟 I/O 共享的大緩衝區、固定大小結構的快取，總之需要實體連續、不准缺頁的地方。

### 1.3 其他區域

[04:31]–[05:00]

| 區域 | 大小（57 位元） | 用途 |
|---|---|---|
| Direct-mapped | 32 PB | 對應所有實體記憶體 |
| 記憶體對映 I/O | 12.5 PB | [04:31] 裝置的位址空間可能很大 |
| Per-CPU 區域 | 0.5 TB | [04:31] 很大。之前只放了一些小欄位（`current`、preempt count），其實可以放很多資料 |
| Kernel 程式碼 | 512 MB | [05:00] 核心本身其實**很小**，半個 GB |
| 模組 | 1520 MB | [05:00] 大部分程式碼（驅動程式等）在**模組**裡，需要時才載入、接到 kernel 上 |

[05:31] 教授說：開機的慣例是 kernel 程式碼從**實體位址 0** 開始（或某個固定偏移）。

【課本補充】模組用來實作驅動程式、檔案系統、加密協定，讓核心保持小而乾淨。以前模組幾乎可以隨意存取 kernel 資料結構，現在越來越受限；模組簽章確保只載入可信的模組。1520 MB 是 v6.2 的代表數字，可設定。

### 1.4 【原始碼】官方文件的地圖

`Documentation/x86/x86_64/mm.rst:91-126`（57 位元，節錄）

```
   0000000000000000 |    0       | 00ffffffffffffff |   64 PB | user-space virtual memory, different per mm
   ...
   ff11000000000000 |  -59.75 PB | ff90ffffffffffff |   32 PB | direct mapping of all physical memory (page_offset_base)
   ...
   ffa0000000000000 |  -24    PB | ffd1ffffffffffff | 12.5 PB | vmalloc/ioremap space (vmalloc_base)
   ...
   ffd4000000000000 |  -11    PB | ffd5ffffffffffff |  0.5 PB | virtual memory map (vmemmap_base)
   ...
   fffffe0000000000 |   -2    TB | fffffe7fffffffff |  0.5 TB | cpu_entry_area mapping
   ...
   ffffffff80000000 |   -2    GB | ffffffff9fffffff |  512 MB | kernel text mapping, mapped to physical address 0
   ffffffffa0000000 |-1536    MB | fffffffffeffffff | 1520 MB | module mapping space
```

48 位元時（第 32-67 行）：使用者 128 TB、direct mapping 64 TB、vmalloc 32 TB、vmemmap 1 TB，最後三塊一樣。

⚠️ 跟課堂 / 課本的差別：

| 課堂 / 課本 | 官方文件 |
|---|---|
| 12.5 PB = 記憶體對映 I/O | **vmalloc / ioremap** 空間。`ioremap`（把裝置記憶體對應進來）只是其中一部分；`vmalloc`（虛擬連續、實體不連續的 kernel 配置，第 33 講）也在這裡 |
| 0.5 TB = per-CPU 區域 | **cpu_entry_area**：每顆 CPU 的進入點相關資料（entry stack、TSS、GDT 等）。一般的 per-CPU 變數（第 7–8 講的 `current`、`pcpu_hot`）放在 kernel 的資料區，透過 `%gs` 存取 |
| 沒提 | **vmemmap**（0.5 PB）：所有 `struct page` 的陣列（§4.2） |

第 124 行「kernel text mapping, mapped to physical address **0**」：kernel 文字段的虛擬區域從實體位址 0 開始對應。但 kernel 實際被載入到 `CONFIG_PHYSICAL_START`（本機 `0x1000000` = 16 MB），而且開了 KASLR 還會隨機放。另外 `CONFIG_RANDOMIZE_MEMORY=y` 時，direct mapping、vmalloc、vmemmap 的起點也會隨機化（文件第 148-149 行）。

---

## 2. 頁表：五層

### 2.1 結構

[06:01] 第 9 講看過：`task_struct` 裡的 `mm_struct` 有一個指向頁表的指標。頁表在 Linux 裡叫 **page directory**。這個指標指向**最上層**頁表的基底位址，**CR3** 暫存器也指向它。

[06:30] Linux 有**五層**分頁。【口頭】教授說：第 2 章看過兩層，第 5 章（教授口誤，這是第 6 章）看五層，「我們成熟了，二變成五」。

[07:01] page 還是 4 KB（12 位元）。剩下 45 位元（45 + 12 = 57）切成五組，每組 9 位元：

```
 56      48 47      39 38      30 29      21 20      12 11         0
┌──────────┬──────────┬──────────┬──────────┬──────────┬────────────┐
│   PGD    │   P4D    │   PUD    │   PMD    │   PTE    │  page 內偏移 │
│  9 bits  │  9 bits  │  9 bits  │  9 bits  │  9 bits  │  12 bits   │
└──────────┴──────────┴──────────┴──────────┴──────────┴────────────┘
   ↑ CR3 指向 PGD
```

| 層 | 全名 |
|---|---|
| PGD | Page Global Directory |
| P4D | Fourth level directory |
| PUD | Page Upper Directory |
| PMD | Page Middle Directory |
| PTE | Page Table Entry（最下層，就是 PTE 的陣列） |

（上圖用 kernel 慣用的 0 起算位元編號。課本 Figure 6.10 用 1 起算：PGD 57-49、P4D 48-40……PTE 21-13、偏移 12-1，是同一件事。）

9 位元 → 每張表 **512 個項目**，每個 8 bytes，一張表剛好 **4 KB**，一個 page。

【課本補充】課本：

- 用五層是因為有 57 位元，目的是盡量減少頁表的記憶體用量、利用虛擬位址空間的**稀疏性**（沒用到的部分不用建下層的表）。
- CR3 是每個 process 一個。換 process 要改 CR3，也要 flush TLB，很貴。
- user ↔ kernel 切換時 CR3 不用變（第 8 講）：所有 kernel thread 共用位址空間。

【實測】本機：

```
address sizes	: 39 bits physical, 48 bits virtual
```

CPU 沒有 `la57` 旗標，所以雖然 kernel 編了 `CONFIG_X86_5LEVEL=y`，實際跑的是 **48 位元、四層**頁表（P4D 這一層在執行期被「摺疊」掉）。實體位址 39 位元 = 最多 512 GB。

### 2.2 【原始碼】`follow_pte`：走頁表

[08:02] 走頁表在 Linux 記憶體系統裡非常重要。

`mm/memory.c:5350-5386`

```c
int follow_pte(struct mm_struct *mm, unsigned long address,
	       pte_t **ptepp, spinlock_t **ptlp)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *ptep;

	pgd = pgd_offset(mm, address);
	if (pgd_none(*pgd) || unlikely(pgd_bad(*pgd)))
		goto out;

	p4d = p4d_offset(pgd, address);
	if (p4d_none(*p4d) || unlikely(p4d_bad(*p4d)))
		goto out;

	pud = pud_offset(p4d, address);
	if (pud_none(*pud) || unlikely(pud_bad(*pud)))
		goto out;

	pmd = pmd_offset(pud, address);
	VM_BUG_ON(pmd_trans_huge(*pmd));

	if (pmd_none(*pmd) || unlikely(pmd_bad(*pmd)))
		goto out;

	ptep = pte_offset_map_lock(mm, pmd, address, ptlp);
	if (!pte_present(*ptep))
		goto unlock;
	*ptepp = ptep;
	return 0;
unlock:
	pte_unmap_unlock(ptep, *ptlp);
out:
	return -EINVAL;
}
```

| 行 | 說明 |
|---|---|
| 5359 | [08:32] 從 `mm_struct` 拿 PGD，用位址的最高 9 位元找到 PGD 項目 |
| 5363, 5367, 5371 | [09:01] 一層一層往下：P4D → PUD → PMD |
| 每層的 `_none` / `_bad` | 這層沒有對應（或壞了）就直接失敗（課本 Listing 6.3 假設項目都存在，把這些省略了） |
| 5372 | PMD 不能是 huge page（2 MB 的 huge page 在 PMD 就結束了，沒有下一層） |
| 5377 | [09:30] 拿到 PTE 時**順便把它鎖起來**（一把 spinlock）並回傳 |
| 5378 | page 不在記憶體裡也算失敗 |
| 5380-5381 | 成功：透過指標回傳 PTE，回傳值 0 |

[09:30]–[10:01] 為什麼回傳時順便鎖？PTE 是**共享**的結構，存取時本來就要拿鎖。與其先拿到 PTE 再鎖，不如一次給你一個**已經鎖好**的 PTE，用完再解鎖。教授說這是一個捷徑。

[10:30] 為什麼參數是 `pte_t **`（雙重指標）？第 18 講講過：回傳值要放**狀態**（0 = 成功），所以要回傳的 PTE 指標，只能讓呼叫者給一個指標，函式把結果填進去。

### 2.3 【原始碼】PMD 這一層怎麼走

[11:01] 看其中一層就懂了：

`include/linux/pgtable.h:69-72, 109-112`

```c
static inline unsigned long pmd_index(unsigned long address)
{
	return (address >> PMD_SHIFT) & (PTRS_PER_PMD - 1);
}
...
static inline pmd_t *pmd_offset(pud_t *pud, unsigned long address)
{
	return pud_pgtable(*pud) + pmd_index(address);
}
```

`arch/x86/include/asm/pgtable_64_types.h:91-92`

```c
#define PMD_SHIFT	21
#define PTRS_PER_PMD	512
```

[11:31]–[14:01]

- 輸入：一個 PUD 項目、一個位址。
- `pud_pgtable(*pud)`：這個 PUD 項目指向的 **PMD 表的基底位址**。
- `pmd_index(address)`：位址右移 21 位，再 AND 511（9 個 1）→ 取出 PMD 那 9 位元，就是**表內的索引**。
- 兩者相加。[12:31] 指標運算會自動乘上 `sizeof(pmd_t)` = 8（第 22 講講過：`int *` 加 5 是加 20 bytes）。

PMD 表的基底位址怎麼來：

`arch/x86/include/asm/pgtable.h:855-858`

```c
static inline pmd_t *pud_pgtable(pud_t pud)
{
	return (pmd_t *)__va(pud_val(pud) & pud_pfn_mask(pud));
}
```

`arch/x86/include/asm/page.h:59`

```c
#define __va(x)			((void *)((unsigned long)(x)+PAGE_OFFSET))
```

[14:31]–[16:32]

- `pud_val(pud)`：PUD 項目的值，裡面存著 PMD 表的**實體位址**。
- `& pud_pfn_mask(pud)`：把**低 12 位元清掉**。位址都對齊到 page 邊界，低 12 位元本來就是 0；Linux 很省，就拿這些位元**存別的資訊**（權限、存在位元等），用之前要遮掉。（也遮掉第 52 位以上，Linux 假設實體位址最多 2⁵² bytes。）
- `__va(...)`：實體位址 + `PAGE_OFFSET` = **direct-mapped 區域**裡的虛擬位址。kernel 才能用這個位址存取 PMD 表。反方向是 `__pa`。

⚠️ 課本說 `PAGE_OFFSET` 是「頁表項目存放的記憶體區域的偏移」。它其實是**整個 direct-mapped 區域的起點**（`page_offset_base`，§1.4），所有實體記憶體都在這裡有對應，不只是頁表。頁表剛好也透過它存取。

### 2.4 PTE 的內容

[07:30] PTE 除了轉換（frame 號碼），還有：在不在記憶體、在 swap 的哪裡，以及**權限**。

[17:01] 結構：frame 號碼最多 **52 位元**（52 + 12 = 64），再加保護位元。

課本 Table 6.3（`include/uapi/asm-generic/mman-common.h`）的權限：

| 旗標 | 意思 |
|---|---|
| `PROT_READ` | 可讀 |
| `PROT_WRITE` | 可寫 |
| `PROT_EXEC` | 可執行 |
| `PROT_SEM` | 可以做 atomic 操作 |
| `PROT_NONE` | 不能存取。[07:30] 教授說這就是用來故意觸發軟缺頁、追蹤存取的那個 |

【課本補充】權限也存在 TLB 裡，硬體不用查頁表就能檢查。程式碼 page 通常不可寫，防止病毒改程式、劫持控制流程；資料 page 不可執行。

（`PROT_*` 是 `mmap()` 的使用者介面；硬體 PTE 實際的位元是第 29 講看過的 `_PAGE_PRESENT`、`_PAGE_RW`、`_PAGE_USER`、`_PAGE_ACCESSED`、`_PAGE_DIRTY`、`_PAGE_NX` 等。）

---

## 3. Page 與 folio

### 3.1 `struct page`

[17:32] 第 29 講區分了 frame 和 physical page。現在再加一個名詞：**folio**。

[19:01] **每個 physical page 都有一個 `struct page`**（`include/linux/mm_types.h`），存它的資訊：

- [19:32] 它屬於哪個位址空間：一般記憶體 page、I/O page，還是 DMA 會寫的 page。
- 它的連結：例如讀檔案時，一堆 page 對應到檔案的位址；它是不是某個 page pool 的一部分。
- [19:32] **參照計數**：kernel 裡有多少東西指著它。[20:00] 變 0 才能回收。

[20:00] 它裡面有一個很大的 **union**：page 的種類很多（一般、I/O、檔案、DMA……），一次只用一種。

【課本補充】`struct page` 開頭是一組**旗標**：鎖住了嗎、修改過嗎、正在寫回嗎、active 嗎、被參照過嗎、保留做特殊用途嗎。union 大小依設定在 20 到 40 bytes。page 回收前參照計數一定要是 0。

（補充：`struct page` 大約 64 bytes，每 4 KB 的實體記憶體就要一個，約占記憶體的 1.6%。本機 40 GB 記憶體就有約 640 MB 拿來放 `struct page`。這也是要 folio 的原因之一。）

### 3.2 Folio

[17:32] 現代程式可能有**數十億**個 physical page。每個都要轉換、PTE、TLB 項目……太多了。

[18:02] 把 **2ᵏ 個**在**虛擬和實體位址空間都連續**的 page 組成一組，叫 **folio**：

- 整組只維護**一個**項目。
- [18:02] 如果硬體支援，也只用一個 TLB 項目。

[20:33] folio 通常指向**第一個 page**，再記錄有幾個 page。

[20:33]–[21:31] 好處：

- 處理大塊資料（檔案、記憶體對映檔案）時很有用。
- **預取（prefetch）**：硬體預取器看存取序列猜你接下來要什麼（教授說像在做機器學習）。folio 是連續的，很容易猜到「我在這裡，接下來會存取這一整塊」，就能提前搬到上層記憶體、加進頁表和 TLB。

[21:31] 現在 kernel 越來越用 **folio** 而不是 page 來思考，folio 正在**取代** page 成為基本單位。LRU 統計、修改狀態都在 folio 層級維護。原因是**可擴展性**。

[22:01] 【口頭】教授的觀察：「人生總是繞回原點。」一開始用 base–limit（虛擬和實體都連續）；因為洞（外部碎片）太多，改用分頁（虛擬 page 可以對應到任何地方）；現在 page 太多管不動，又回頭追求虛擬和實體的連續性。

[23:00]–[24:00] folio 的性質：

- 它**假裝**自己是一個 page：記憶體管理子系統的基本單位現在是 folio。
- folio 裡的 page **共享**權限。
- **copy-on-write** 也以 folio 為單位：fork 後寫一個 byte，整個 folio 複製一份。
- 遷移、swap、複製都以 folio 為單位。

【課本補充】folio 是 **v5.18** 才加進 kernel 的。最早的 folio 只是一組連續的**虛擬** page（以第一個 page 識別），現在也追求**實體**連續，對 I/O、DMA、減少轉換成本都有好處。I/O 和 DMA 裝置不做位址轉換、直接存取實體記憶體，給它們一大塊連續的實體記憶體，傳輸很方便。

`include/linux/mm_types.h:328-348`（開頭，節錄）

```c
struct folio {
	/* private: don't document the anon union */
	union {
		struct {
	/* public: */
			unsigned long flags;
			union {
				struct list_head lru;
				...
			};
			struct address_space *mapping;
			pgoff_t index;
			void *private;
			atomic_t _mapcount;
```

`struct folio` 的開頭**跟 `struct page` 的版面一模一樣**（flags、lru、mapping、index……），因為一個 folio 就是用它的第一個 `struct page` 來表示的，`page` 和 `folio` 指標可以互相轉。`lru` 就是 LRU 串列的節點（第 29 講的 active/inactive、第 32 講的 MGLRU）。

⚠️ folio 嚴格的保證是**實體連續**、大小是 2 的次方。它被對應到使用者空間時，虛擬位址也通常連續，但 kernel 允許只對應其中一部分（例如 `mremap` 之後）。

### 3.3 Huge page

[24:00] Linux 要相容很多硬體。有些硬體支援 **huge page**。huge page 是**硬體**概念，folio 是**軟體**概念，有硬體支援時兩者可以結合。

[24:32]–[26:31] 動機：

- 現代 process 記憶體很大 → 要很多 TLB 項目和 PTE → TLB 很快滿了、miss 率高、轉換成本高。
- 用比 4 KB 大的 page，例如 2 MB：整個 page 一次轉換，一個項目就夠。TLB miss 機會小很多。
- 代價：2 MB 的 page 可能沒有全部用到 → **內部碎片**。所以 huge page 不是到處都適合，但對很大的軟體確實有用。

[26:31]–[28:03] huge page 該多大？看頁表結構：

```
           ... │   PMD    │   PTE    │  偏移 12 bits │
                           └──────── 21 bits ───────┘   → 在 PMD 結束：2^21 = 2 MB huge page
           ... │   PUD    │   PMD + PTE + 偏移 = 30 bits │ → 在 PUD 結束：2^30 = 1 GB huge page
```

- 在 PMD 切：低 21 位元都是 page 內偏移 → **2 MB**。PMD 項目直接指向 huge page，不需要最下層的表。
- 在 PUD 切：低 30 位元 → **1 GB**。一個 PUD 項目對應一個 1 GB huge page。

[28:03] 所以現代 Intel 處理器有 **2 MB** 和 **1 GB** 的 huge page，這些數字跟頁表結構自然對齊。資料庫這種大軟體會很受益。

[28:32] folio 和 huge page 的關係：folio 也是 2ᵏ 個 page，只是在軟體層。一個 huge page 可以是四個 folio，也可以就是一個 folio，看 OS 設定的最大 folio 大小。

【課本補充】1 GB huge page 可以放 2¹⁸ 個 4 KB page，只用一個對應，TLB 和頁表的項目大量減少。

【實測】本機 CPU 旗標有 `pse`（2 MB page）和 `pdpe1gb`（1 GB page）。

### 3.4 【實測】Huge page 的效果

`thp.c`：`mmap` 1 GB 匿名記憶體，用 `madvise` 要求（或拒絕）**透明 huge page（THP）**，然後隨機讀 5000 萬次（大量 TLB miss）：

```
$ cat /sys/kernel/mm/transparent_hugepage/enabled
always [madvise] never

MADV_NOHUGEPAGE: 50M random reads in 0.426 s
   Rss:             1046528 kB
   AnonHugePages:         0 kB

MADV_HUGEPAGE  : 50M random reads in 0.371 s
   Rss:             1046528 kB
   AnonHugePages:   1046528 kB
```

- 本機 THP 設定是 `madvise`：只有程式用 `MADV_HUGEPAGE` 要求的區域才用 huge page。
- 要求之後，`/proc/<pid>/smaps` 顯示整整 1 GB 都是 `AnonHugePages`（2 MB 的 huge page），否則是 0。
- 同樣的隨機讀，用 huge page 快了約 **13%**：1 GB 用 4 KB page 要 26 萬個 PTE，TLB 根本裝不下；用 2 MB page 只要 512 個項目。
- 差距沒有更大，是因為這顆 CPU 的 TLB 有硬體頁表走訪器，而且頁表本身多半還在 cache 裡。

---

## 4. PFN 與 `struct page` 的轉換

### 4.1 Section 與偏移指標（biased pointer）

[29:30] 看一些轉換的程式碼，教授說這裡有一些很有意思的工程技巧。

課本 Listing 6.5（`include/asm-generic/memory_model.h`）：

```c
#define pte_pfn(x)	phys_to_pfn(x.pte)
#define phys_to_pfn(p)	((p) >> PAGE_SHIFT)

#define __pfn_to_page(pfn)				\
({	unsigned long __pfn = (pfn);			\
	struct mem_section *__sec = __pfn_to_section(__pfn);	\
	__section_mem_map_addr(__sec) + __pfn;		\
})
```

[29:30]–[30:02]

- PTE → PFN（page frame number）：PFN 就存在 PTE 裡。
- 實體位址 → PFN：右移 12 位（吃掉 page 內偏移）。

[30:02] 有趣的是 **PFN → `struct page`**：

1. [30:31]–[31:30] 所有 PFN 被分成好幾個 **section**（實體記憶體可能有不同區段、對應到不同的記憶體裝置）。從 PFN 可以算出它在哪個 section，拿到 section 的指標。
2. 拿 section 的「基底位址」，加上 PFN，就是 `struct page` 的位址。

[33:00]–[35:30] 問題：如果 section A 只包含 PFN 60–80，而 section 的陣列從 A 開始，`A + 80` 就超出範圍了。每個 section 都配 N 個項目（N = 全部 frame 數）又太浪費。

[35:00]–[36:01] Linux 的**算術技巧**：回傳的「基底位址」其實是 **A − 60**：

```
回傳：base_sec = A − 60        （偏移指標，biased pointer）
PFN 60 → base_sec + 60 = A      ← 陣列第一格
PFN 80 → base_sec + 80 = A + 20 ← 陣列最後一格
```

只配 20 格，PFN 直接加上去就對了。這個「基底位址」不是真的指標，是**偏移過的指標**。

> **課本 Point 6.2.1**：`__section_mem_map_addr` 回傳的不是 section 真正的基底位址 A，而是 A − 起始 PFN。例如起始 PFN 是 32、要找的 PFN 是 36，實際上是加 4。這是 Linux 的 **sparse memory** 對應機制。

【原始碼】註解直接寫了：

`include/asm-generic/memory_model.h:28-31`

```c
#elif defined(CONFIG_SPARSEMEM)
/*
 * Note: section's mem_map is encoded to reflect its start_pfn.
 * section[i].section_mem_map == mem_map's address - start_pfn;
 */
```

### 4.2 ⚠️【原始碼】x86-64 實際用的是 vmemmap

同一個檔案有**三種**記憶體模型：

`include/asm-generic/memory_model.h:18-26`

```c
#define __pfn_to_page(pfn)	(mem_map + ((pfn) - ARCH_PFN_OFFSET))
...
#elif defined(CONFIG_SPARSEMEM_VMEMMAP)

/* memmap is virtually contiguous.  */
#define __pfn_to_page(pfn)	(vmemmap + (pfn))
#define __page_to_pfn(page)	(unsigned long)((page) - vmemmap)
```

| 模型 | `pfn_to_page` | 用在 |
|---|---|---|
| FLATMEM | `mem_map + (pfn − offset)` | 小系統，一個大陣列 |
| SPARSEMEM | 查 section + 偏移指標（課本講的） | 實體記憶體有很多洞的系統 |
| **SPARSEMEM_VMEMMAP** | **`vmemmap + pfn`** | **x86-64 預設** |

本機：`CONFIG_SPARSEMEM=y`、`CONFIG_SPARSEMEM_VMEMMAP=y`。

vmemmap 的做法：在 kernel 虛擬位址空間保留一大塊（§1.4 的 **0.5 PB `vmemmap`** 區域），把**所有** `struct page` 排成一個**虛擬上連續**的陣列，PFN 就是索引。實體記憶體有洞的地方，對應的 `struct page` 那段虛擬位址**根本不對應實體 page**，不浪費記憶體。這等於把 section 的工作交給**頁表**來做，轉換只要一次加法，比查 section 快。section 仍然存在（記憶體熱插拔、建立 vmemmap 時用），只是 `pfn_to_page` 不用查它了。

---

## 5. 管理 TLB

### 5.1 換 process 就要 flush

[36:31]–[37:00] TLB 是硬體結構，存最近用過的對應。現代 TLB：

- **i-TLB**（指令）、**d-TLB**（資料）、**L2 TLB**（上層放不下的往下流）。
- 命中率 **99% 以上**；miss 很貴，要走頁表（硬體或軟體）。
- [37:00] x86 上通常是 4 到 16 路集合關聯的 cache，兩層。
- 每個項目存實體 frame 號碼和**保護位元**。

[37:31] 切換時：

- user process → kernel、kernel thread → kernel thread：**不用**換頁表、不用 flush TLB。
- 換 **user process**：要重新載入 **CR3**（指向不同的頁表），也要 **flush TLB**。很貴：舊 process 回來時，它的項目全都不見了。[38:02] 但為了安全和正確，必須 flush。

【課本補充】課本：

- Intel 可以把某些 TLB 項目標成 **global（G）**，不會被 flush。kernel 位址空間的項目就標成 global，因為所有 process 都一樣。
- `invlpg` 指令可以**選擇性**地讓某個項目失效，不用整個 flush。
- 把虛擬位址空間分成 user 和 kernel 兩半，最大的好處就是進出 kernel 時不用 flush TLB。

### 5.2 ASID 與 PCID

[38:30]–[40:02] 想避免 flush：在每個 TLB 項目加上 **process ID**。問題是 process ID 是**軟體**概念，要做出一個對應的**硬體**概念（就像 huge page 對應 folio）。

- process 結束時，只**選擇性清除**它的項目。
- 代價：查 TLB 時要同時比對**虛擬位址**和**這個 ID**，硬體成本高一點。
- 這個 ID 叫 **ASID（Address Space Identifier）**。

```
TLB 項目：[ ASID │ 虛擬 page 號碼 ] → 實體 frame 號碼 + 權限
```

[40:32] Linux 在軟體層支援 ASID；Intel 在硬體層支援 **PCID（Processor Context ID）**。PCID 存在 CR3 裡（目前在跑哪個 PCID）。

[42:02] 兩個 process 可能有相同的虛擬位址，但因為項目標了 PCID（process ID 的代理），轉換仍然正確。

[42:30] 兩種失效指令：

- 全部失效，不管屬於哪個 process。
- 只讓**某個 process** 的項目失效，例如那個 process 被殺掉了。（Intel 的 `INVPCID` 指令。）

【課本補充】課本說這是在**打破**一個長久的抽象：process ID 本來是純軟體的概念，現在讓硬體看到。但不這樣做，每換一次 user process，TLB miss 率就會很高，要找中間點。

### 5.3 ⚠️【原始碼】數字搞反了：Intel 有 4096 個、Linux 只用 6 個

[40:32]–[41:30] 教授說「Intel 每顆核心支援 **6 個** PCID……奇怪的是 Linux 支援 4096 個 ASID，Intel 只支援 6 個」。**搞反了**。課本是對的：

- **Intel 硬體**：CR3 的低 12 位元放 PCID，**4096 個**（PCID 0 有特殊用途）。
- **Linux**：每顆 CPU 只用 **6 個**。

`arch/x86/mm/tlb.c:87-88`

```c
/* There are 12 bits of space for ASIDS in CR3 */
#define CR3_HW_ASID_BITS		12
```

`arch/x86/include/asm/tlbflush.h:57-61`

```c
/*
 * 6 because 6 should be plenty and struct tlb_state will fit in two cache
 * lines.
 */
#define TLB_NR_DYN_ASIDS	6
```

為什麼只用 6 個？說明在：

`arch/x86/mm/tlb.c:60-84`

```c
 * The x86 feature is called PCID (Process Context IDentifier). It is similar
 * to what is traditionally called ASID on the RISC processors.
 *
 * We don't use the traditional ASID implementation, where each process/mm gets
 * its own ASID and flush/restart when we run out of ASID space.
 *
 * Instead we have a small per-cpu array of ASIDs and cache the last few mm's
 * that came by on this CPU, allowing cheaper switch_mm between processes on
 * this CPU.
 *
 * We end up with different spaces for different things. To avoid confusion we
 * use different names for each of them:
 *
 * ASID  - [0, TLB_NR_DYN_ASIDS-1]
 *         the canonical identifier for an mm
 *
 * kPCID - [1, TLB_NR_DYN_ASIDS]
 *         the value we write into the PCID part of CR3; corresponds to the
 *         ASID+1, because PCID 0 is special.
 *
 * uPCID - [2048 + 1, 2048 + TLB_NR_DYN_ASIDS]
 *         for KPTI each mm has two address spaces and thus needs two
 *         PCID values, but we can still do with a single ASID denomination
 *         for each mm. Corresponds to kPCID + 2048.
```

- Linux **不**給每個 process 一個全域的 ASID。
- 而是每顆 CPU 有一個**很小的陣列**（6 格），快取「最近在這顆 CPU 上跑過的幾個 mm」。切回其中一個時，它的 TLB 項目還在，不用 flush。
- 「6 個應該很夠了，而且 `struct tlb_state` 剛好能塞進兩條 cache line」（`tlbflush.h:58-59`）。
- **KPTI**（Meltdown 修補，本機 `CONFIG_PAGE_TABLE_ISOLATION=y`）讓每個 process 有兩套頁表（user 用、kernel 用），所以每個 mm 用兩個 PCID：kPCID 和 uPCID = kPCID + 2048。

課本說「為了跨架構的最大公約數才只用 6 個」，原始碼的理由則是：per-CPU 的小快取就夠用，而且結構小、放得進 cache。

【實測】本機 CPU（i7-13650HX，Raptor Lake）的旗標有 `invpcid` 但**沒有 `pcid`**。可能是 kernel 針對 Alder Lake / Raptor Lake 混合架構的 INVLPG 勘誤，主動停用了 PCID（6.4 版之後加入的修補），細節待補。

### 5.4 Lazy TLB 模式

[43:01]–[44:00] **TLB 之間的一致性**：多執行緒程式的 thread 跑在不同 CPU 上，共享位址空間。某顆 CPU 上一個虛擬 → 實體對應被取消了，**其他核心**也要從它們的 TLB 拿掉這個對應。

[44:00]–[45:02] 但其他 CPU 可能正在跑 kernel thread 或別的 process。只因為某顆 CPU 取消了一個對應，所有 CPU 都要停下手上的事去處理嗎？**不需要那麼急**，可以讓更緊急的事先做完。

[45:02] 這就是 **lazy TLB 模式**：不馬上失效，等適當的時機再做。

[45:30] 情況分析：

- 另一顆核心正在跑**同一個 process 的另一個 thread** → 要送 **IPI**，馬上失效。
- 正在跑**別的 process** 或 **kernel** → 可以等。

[46:00]–[47:31] kernel 拿到控制權時，三種情況：

| 情況 | 處理 |
|---|---|
| kernel 想存取那個（已失效的）**使用者** page | [46:00] kernel 不能隨便存取使用者 page，只能透過 **`copy_from_user` / `copy_to_user`**。在這兩個函式裡檢查有沒有被延後的失效工作，有就先做 |
| kernel 切到**另一個** user process | [47:01] 原本 process 的 page 本來就全部被 flush 了，那個失效的也一起走 |
| kernel 切**回同一個** user process | [47:01] 返回使用者空間前（第 8 講那五個步驟之一）檢查有沒有待辦的工作，有就在那時候失效 |

[47:31] 把維持 TLB 一致性的工作**延後到比較方便的時機**，不用為此打斷重要的工作。

> **課本 Point 6.2.2**：kernel thread 沒有自己的頁表。一份共用的 kernel 頁表接在每個使用者頁表上：使用者和 kernel 的虛擬位址只差最高位元，所以在五層頁表的最上層放一個指向 kernel 頁表的指標。

⚠️ Linux 實際的 lazy TLB 跟教授說的「在 `copy_from_user` 裡檢查」不太一樣（`arch/x86/mm/tlb.c`）：

- CPU 從 user process 切到 **kernel thread** 時，kernel thread 沒有自己的 `mm`，就**繼續沿用**上一個 process 的頁表（第 13 講的 `active_mm`、`prev_mm`），不換 CR3。這時這顆 CPU 被標成 lazy：

`arch/x86/mm/tlb.c:668`（`enter_lazy_tlb` 裡）

```c
	this_cpu_write(cpu_tlbstate_shared.is_lazy, true);
```

- 別的 CPU 要求 flush 時，**跳過** lazy 的 CPU，不送 IPI：

`arch/x86/mm/tlb.c:881-884`

```c
static bool tlb_is_not_lazy(int cpu, void *data)
{
	return !per_cpu(cpu_tlbstate_shared.is_lazy, cpu);
}
```

- lazy 的 CPU 之後要切回某個 user process（`switch_mm_irqs_off`）時，檢查它的 TLB 是不是過時了（比對每個 mm 的 `tlb_gen` 世代號碼），過時就在那時候 flush。
- kernel thread 本來就不會碰使用者 page（`copy_from_user` 只在 process context 用，那時用的是該 process 自己的 mm，不是 lazy 狀態）。

想法一樣（延後、找方便的時機），機制是「沿用舊頁表 + 世代號碼 + 跳過 IPI」。

---

## 6. 分割實體記憶體

### 6.1 NUMA

[48:30]–[49:31] 記憶體和 CPU 很多時，CPU 會分成**叢集**，記憶體也是：有些記憶體離這群 CPU 近，有些遠。這是 **NUMA（Non-Uniform Memory Access）** 機器。

[49:31] OS 要知道「哪個叢集常存取哪些 page」，把那些 page 放在離它近的記憶體。所以從一個 NUMA **node**（節點）的角度，要把實體記憶體分成好幾個 **zone**，不同 zone 的 page 要不同對待。

【課本補充】課本 Figure 6.11：幾個晶片透過共享互連連在一起；同一個叢集叫一個 **node**，存取本地記憶體比遠端快很多。要盡量讓一個 node 用到的資料和程式碼留在本地記憶體。

### 6.2 Zone

[50:02] 有些 zone 專門給 DMA、有些給 I/O、有些是 NVM、快閃記憶體、USB 上的記憶體，當然要不同對待。

[50:31]–[52:03] zone 是一塊有共同特性的實體記憶體。zone 再分成 **section**（一組連續的 page），用偏移指標做對應（§4.1）。教授說這是 kernel 的**標準模式：階層式設計**，就像 namespace（第 10 講）、IRQ domain（第 15 講）都是一棵樹。

[52:32] 分組的好處：可以對一整組 page 套用相同的政策；可以一起回收、一起從一個實體裝置搬到另一個。

[52:32]–[54:30] zone 的種類：

| Zone | 用途 |
|---|---|
| **ZONE_DMA** | [52:32] 給 DMA |
| **ZONE_NORMAL** | 一般的 page |
| **ZONE_HIGHMEM** | [52:32] 嵌入式裝置：**實體位址空間比虛擬位址空間大**。例如 32 位元系統虛擬最多 4 GB，但記憶體更多，超出的 page 暫時對應不進來，放這裡。[53:31] 要對應它，得先拿掉別的對應 |
| **ZONE_MOVABLE** | [53:31] 假設在可插拔的裝置上，page 一生中會一直搬動或變得無法存取 |
| **ZONE_DEVICE** | [54:00] 現在用在**非揮發性記憶體（NVM）**：介於記憶體和儲存裝置之間。當儲存用就擴大硬碟容量，當記憶體用就擴大實體記憶體 |

【課本補充】課本的解釋：

- ZONE_MOVABLE：要建立大塊連續的實體記憶體（給 folio、huge page），page 必須能**自由搬動**、不能被釘住。只要有一個 process（通常是 kernel）釘住一個 page，就破壞了連續性。所以把可搬動的 page 集中到一個不准釘住的 zone。
- ZONE_DEVICE：包括 NVM、顯示卡上的記憶體、Intel Optane 等。裝置可能隨時被拔掉，所以 DRAM 裡不該留它們的副本（不准 page cache），由裝置自己管理。
- Point 6.2.3：硬碟延遲是毫秒、NVM 是微秒或幾百奈秒、DRAM 不到 100 奈秒。NVM 斷電後內容還在，也能當儲存裝置。

### 6.3 【原始碼】`enum zone_type`

`include/linux/mmzone.h:610-660`（節錄）

```c
	 * ZONE_DMA and ZONE_DMA32 are used when there are peripherals not able
	 * to DMA to all of the addressable memory (ZONE_NORMAL).
	 ...
#ifdef CONFIG_ZONE_DMA
	ZONE_DMA,
#endif
#ifdef CONFIG_ZONE_DMA32
	ZONE_DMA32,
#endif
	/*
	 * Normal addressable memory is in ZONE_NORMAL. DMA operations can be
	 * performed on pages in ZONE_NORMAL if the DMA devices support
	 ...
	ZONE_NORMAL,
#ifdef CONFIG_HIGHMEM
	ZONE_HIGHMEM,
#endif
	/*
	 * ZONE_MOVABLE is similar to ZONE_NORMAL, except that it contains
	 ...
	ZONE_MOVABLE,
#ifdef CONFIG_ZONE_DEVICE
	ZONE_DEVICE,
#endif
	__MAX_NR_ZONES
```

⚠️ 跟課本的差別：

- 課本漏了 **ZONE_DMA32**。
- 課本說 ZONE_DMA 是「保留給 DMA 控制器的區域……可以確保這裡的資料**不被 cache**」。原始碼註解說的是另一回事：**有些周邊裝置沒辦法對所有可定址的記憶體做 DMA**（它們的位址線比較少）。
  - x86 上 **ZONE_DMA** = 實體記憶體最低的 **16 MB**（給古老的 ISA 裝置，只有 24 位元位址）。
  - **ZONE_DMA32** = 最低的 **4 GB**（給只能用 32 位元位址的裝置）。
  - 能定址全部記憶體的裝置，直接在 ZONE_NORMAL 做 DMA 就好。
  - 這跟 cache 無關；DMA 的 cache 一致性是另外處理的（x86 硬體本身就保持 DMA 與 cache 一致）。
- ZONE_HIGHMEM 只在 32 位元系統有，64 位元 kernel 沒有（本機設定裡沒有 `CONFIG_HIGHMEM`）。

### 6.4 `struct zone` 與 `pglist_data`

[54:30] `struct zone`（課本 Listing 6.7，`include/linux/mmzone.h`）：

```c
struct zone {
    int node;                       /* 屬於哪個 NUMA node */
    struct pglist_data *zone_pgdat; /* 那個 node 的詳細資料 */
    unsigned long zone_start_pfn;   /* 起始 PFN */
    atomic_long_t managed_pages;    /* kernel 實際管理的 page 數 */
    unsigned long spanned_pages;    /* 範圍：end − start + 1 */
    unsigned long present_pages;    /* 實際存在的 page 數 */
    const char *name;
    struct free_area free_area[MAX_ORDER];   /* 空閒區塊串列（buddy allocator） */
};
```

| 欄位 | 說明 |
|---|---|
| `node` | [54:30] 每個 zone 屬於一個 NUMA node |
| `zone_start_pfn` | [55:00] 起始 PFN |
| `spanned_pages` | [55:00] zone 從頭到尾的**範圍**。結束 PFN = start + spanned − 1 |
| `present_pages` | 實際**有**幾個 page。zone 裡的 page 不一定連續（所以才要 section），連續時 = spanned，否則 < spanned |
| `managed_pages` | 【課本】kernel 真正在管理的 page。有些 page 屬於這個 zone，但 kernel 沒在管它們（例如開機時保留的） |
| `free_area[]` | [55:32] 空閒區域的串列，像一串「洞」，第 33 講的 **buddy allocator** 會大量使用。教授說這有點像 base–limit 的洞 |

[56:01] NUMA node 的資料是 `struct pglist_data`（課本 Listing 6.8）：

```c
typedef struct pglist_data {
    int node_id;
    struct zone node_zones[MAX_NR_ZONES];          /* 這個 node 的 zone（每種最多一個） */
    struct zonelist node_zonelists[MAX_ZONELISTS]; /* 所有 node 的 zone，第一個是本地的 */
    int nr_zones;
    unsigned long node_present_pages;
    unsigned long node_spanned_pages;
    struct task_struct *kswapd;                    /* 回收 page 的背景 thread */
    struct lruvec __lruvec;                        /* LRU 資訊 */
} pg_data_t;
```

[56:31]–[57:01] 兩個欄位下一講會大量用到：

- **`kswapd`**：一個一直在跑的 **daemon（背景程式）**，找出 zone 裡不常用的 page，**主動**推到儲存裝置，空出實體記憶體。【課本】在 NUMA 機器上也依存取模式在 node 之間搬 page，平衡記憶體用量。
- **`lruvec`**：存放置換用的 **LRU 資訊**。

### 6.5 【實測】本機的 zone

```
$ grep -E '^Node|present|spanned|managed' /proc/zoneinfo
Node 0, zone      DMA
        spanned  4095
        present  3998
        managed  3840
Node 0, zone    DMA32
        spanned  1044480
        present  376086
        managed  359624
Node 0, zone   Normal
        spanned  10024960
        present  10024960
        managed  9822130
Node 0, zone  Movable
        spanned  0
        present  0
        managed  0
Node 0, zone   Device
        spanned  2031616
        present  0
        managed  0
```

| Zone | spanned | present | 說明 |
|---|---|---|---|
| DMA | 4095 | 3998 | 約 16 MB（4095 × 4 KB）。有一點洞 |
| DMA32 | 1044480 | **376086** | 範圍是 16 MB–4 GB，但只有約 36% **實際存在**：中間有一大段被 PCI 裝置的 MMIO 等占走了。正是 spanned ≠ present 的例子 |
| Normal | 10024960 | 10024960 | 4 GB 以上的約 38 GB，連續 |
| Movable | 0 | 0 | 沒有設定（要開機參數 `movablecore=` 才有） |
| Device | 2031616 | 0 | 保留了範圍，但沒有實際的裝置記憶體 |

- 只有 `Node 0`：這台筆電只有一個 NUMA node（`CONFIG_NUMA=y` 但硬體不是 NUMA）。
- managed 比 present 少一點：開機時保留給 kernel 本身、`struct page` 陣列等的 page，不歸 buddy allocator 管。

`/proc/buddyinfo` 是每個 zone 的 `free_area[]`（第 33 講詳細講）：

```
Node 0, zone      DMA      0      0      0      0      0      0      0      0      1      1      2
Node 0, zone    DMA32      6      6      6      4      4      4      4      3      4      4    346
Node 0, zone   Normal   1457   3146  10955  10563   9218   1055    237    144    127    118   4867
```

第 k 欄 = 有幾塊 2ᵏ 個連續 page 的空閒區塊（k = 0 到 10，最大 4 MB）。

---

## 7. 一頁速查表

### 7.1 虛擬記憶體地圖（57 位元）

| 區域 | 大小 |
|---|---|
| 使用者 | 64 PB |
| direct mapping（`PAGE_OFFSET`） | 32 PB |
| vmalloc / ioremap | 12.5 PB |
| vmemmap（`struct page` 陣列） | 0.5 PB |
| cpu_entry_area | 0.5 TB |
| kernel text | 512 MB |
| 模組 | 1520 MB |

`__va(x) = x + PAGE_OFFSET`；`__pa` 反過來。

### 7.2 頁表

| 層 | shift | 每層 9 位元 → 512 項 |
|---|---|---|
| PGD / P4D / PUD / PMD / PTE | 48 / 39 / 30 / 21 / 12 | 每張表剛好 4 KB |

huge page：在 PMD 結束 = 2 MB；在 PUD 結束 = 1 GB。`follow_pte` 一層一層走，回傳鎖好的 PTE。

### 7.3 Page / folio

| | 意思 |
|---|---|
| `struct page` | 每個 4 KB physical page 一個，約 64 bytes：旗標、union、參照計數 |
| folio | 2ᵏ 個實體連續的 page，以第一個 `struct page` 表示；LRU、COW、權限都以它為單位（v5.18+） |
| huge page | 硬體概念：2 MB / 1 GB，減少 TLB 項目 |
| `pfn_to_page` | SPARSEMEM：section + 偏移指標；x86-64 用 VMEMMAP：`vmemmap + pfn` |

### 7.4 TLB

| 機制 | 說明 |
|---|---|
| global 位元 | kernel 的項目不 flush |
| PCID | 硬體 12 位元（4096 個）；Linux 每顆 CPU 只用 6 個，快取最近的 mm |
| lazy TLB | kernel thread 沿用舊頁表；flush 時跳過 lazy 的 CPU；切回 user 時再補 |

### 7.5 Zone

DMA（< 16 MB）、DMA32（< 4 GB）、NORMAL、HIGHMEM（只有 32 位元）、MOVABLE、DEVICE。spanned = 範圍，present = 實際存在，managed = buddy 管的。

### 7.6 本講看過的原始碼位置

| 位置 | 內容 |
|---|---|
| `Documentation/x86/x86_64/mm.rst:32-126` | 48 / 57 位元的虛擬記憶體地圖 |
| `mm/memory.c:5350-5386` | `follow_pte` |
| `include/linux/pgtable.h:69-72, 109-112` | `pmd_index`、`pmd_offset` |
| `arch/x86/include/asm/pgtable.h:855-858` | `pud_pgtable` |
| `arch/x86/include/asm/page.h:59` | `__va` |
| `arch/x86/include/asm/pgtable_64_types.h:62-92` | 各層 shift |
| `include/linux/mm_types.h:328-348` | `struct folio` |
| `include/asm-generic/memory_model.h:18-43` | FLATMEM / VMEMMAP / SPARSEMEM 的 `pfn_to_page` |
| `arch/x86/mm/tlb.c:60-88` | PCID 與 ASID 的說明 |
| `arch/x86/include/asm/tlbflush.h:57-61` | `TLB_NR_DYN_ASIDS` = 6 |
| `arch/x86/mm/tlb.c:668, 881-884` | lazy TLB |
| `include/linux/mmzone.h:610-660` | `enum zone_type` |

---

## 8. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **`struct page` 的 `mapping`、`_mapcount`：一個 page 被誰對應** | **第 31 講**：Reverse mapping |
| **`lruvec`、`kswapd`、folio 的 LRU** | **第 32 講**：MGLRU |
| **`free_area[]`、`/proc/buddyinfo`、vmalloc 區域** | **第 33 講**：Kernel memory allocation |
| **`copy_from_user`、返回使用者空間前的檢查** | 回顧第 8 講 |
| **`active_mm`、`prev_mm`** | 回顧第 13 講 |

（講次對應依播放清單標題推估。）

---

## 附：本講一句話總結

> **57 位元的 128 PB 虛擬位址空間一半給使用者、一半給 kernel，kernel 這半最重要的是 32 PB 的 direct-mapped 區域（虛擬位址 = 實體位址 + `PAGE_OFFSET`，`__va` / `__pa` 一次加減），另外有 vmalloc/ioremap、vmemmap（所有 `struct page` 的陣列）、512 MB 的 kernel 程式碼和 1520 MB 的模組。頁表有五層，每層 9 位元、512 項、剛好一個 page，`follow_pte` 一層層用「表基底 + 索引」往下走，最後回傳鎖好的 PTE；在 PMD 或 PUD 提早結束就是 2 MB / 1 GB 的 huge page（實測 1 GB 隨機讀快了 13%）。每個 physical page 有一個 `struct page`，而 folio 把 2ᵏ 個實體連續的 page 當成一個單位，正逐漸取代 page。換 process 要換 CR3、flush TLB，所以有 PCID：硬體其實有 4096 個（教授說反了），Linux 每顆 CPU 只用 6 個來快取最近的位址空間；跨核心的 TLB 失效用 lazy 模式延後到方便時才做。實體記憶體依 NUMA node 分成 DMA（16 MB 以下）、DMA32（4 GB 以下）、NORMAL、MOVABLE、DEVICE 等 zone（本機 DMA32 範圍 4 GB 但只有 36% 真的存在），每個 node 的 `pglist_data` 帶著 `kswapd` 和 `lruvec`，下一講就會用到。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 30（Chapter 6, Part II）錄影逐字稿（YouTube 自動字幕，57:58）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§6.2，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0、x86-64、i7-13650HX）結果。*
