# Operating Systems: A Linux Kernel-Oriented Approach 第九講完整筆記：`mm_struct` 與 Maple Tree（Chapter 3, Part III）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 9（Chapter 3, Part III）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（51:36，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§3.1.9–3.1.10（p.79–83）與附錄 C.2–C.4（紅黑樹、B 樹、B+ 樹、maple tree）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
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
- [1. Process 的記憶體「地產」](#1-process-的記憶體地產)
  - [1.1 稀疏的 memory map 與 VM 區域](#11-稀疏的-memory-map-與-vm-區域)
  - [1.2 為什麼要分成區域](#12-為什麼要分成區域)
- [2. `mm_struct` 的欄位](#2-mm_struct-的欄位)
  - [2.1 總覽](#21-總覽)
  - [2.2 一個指標代表整張 page table](#22-一個指標代表整張-page-table)
  - [2.3 Locked page 與 pinned page](#23-locked-page-與-pinned-page)
  - [2.4 CPU bitmap](#24-cpu-bitmap)
  - [2.5 【原始碼】`struct mm_struct`](#25-原始碼struct-mm_struct)
- [3. VM 區域：`vm_area_struct`](#3-vm-區域vm_area_struct)
  - [3.1 每個區域的起點與終點](#31-每個區域的起點與終點)
  - [3.2 Anonymous 與 file-backed](#32-anonymous-與-file-backed)
  - [3.3 【原始碼】`struct vm_area_struct`](#33-原始碼struct-vm_area_struct)
- [4. 從二元搜尋樹到 maple tree](#4-從二元搜尋樹到-maple-tree)
  - [4.1 問題：這個位址屬於哪個區域？](#41-問題這個位址屬於哪個區域)
  - [4.2 楓葉：不是二元的](#42-楓葉不是二元的)
  - [4.3 二元樹的問題：快取](#43-二元樹的問題快取)
  - [4.4 B 樹](#44-b-樹)
  - [4.5 B 樹的例子](#45-b-樹的例子)
  - [4.6 B+ 樹](#46-b-樹)
  - [4.7 插入與刪除：分裂與合併](#47-插入與刪除分裂與合併)
  - [4.8 為什麼 B 樹比較快取友善](#48-為什麼-b-樹比較快取友善)
  - [4.9 Maple tree](#49-maple-tree)
  - [4.10 【原始碼】maple tree 的節點](#410-原始碼maple-tree-的節點)
  - [4.11 【原始碼】`find_vma()`：一行查完](#411-原始碼find_vma一行查完)
- [5. 再看 anonymous 與 file-backed](#5-再看-anonymous-與-file-backed)
- [6. 抽象化：外界只看到一個指標](#6-抽象化外界只看到一個指標)
- [7. 【實測】一個 process 有幾個 VMA](#7-實測一個-process-有幾個-vma)
- [8. 一頁速查表](#8-一頁速查表)
- [9. 與後續講次的連結](#9-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

`[00:33]` `task_struct` 裡**跟記憶體有關**的部分。上一講結尾說這是一段「記憶體繞道」：之前看單一 process 怎麼跑，這一講看它的記憶體，下一講再回來看**一群** process。

| # | 主題 | 對應課本 |
|---|---|---|
| 1 | `mm_struct`：process 的記憶體資訊 | §3.1.9 |
| 2 | `vm_area_struct`：一個連續的虛擬記憶體區域；anonymous vs file-backed | §3.1.10 |
| 3 | **Maple tree**：從二元樹 → B 樹 → B+ 樹 → maple tree | 附錄 C.2–C.4 |

---

## 1. Process 的記憶體「地產」

### 1.1 稀疏的 memory map 與 VM 區域

`[00:33]` 管記憶體是 process 擁有的**地產（real estate）** 的關鍵部分。process 在整個 memory map 上擁有很多記憶體。

`[01:01]` memory map 是**稀疏**的（第 5 講）：

```
 高 ┌────────────────────┐
    │ Stack ↓            │  ← 有東西
    │                    │
    │      （空）         │
    │ library、映射檔案    │  ← 中間有幾塊（動態連結 library、記憶體映射檔案）
    │      （空）         │
    │                    │
    │ Heap ↑             │  ← 有東西
    │ Data / BSS / Text   │  ← 有東西
 低 └────────────────────┘
```

`[01:33]` 這些**虛擬記憶體區域（VM region）**，總要有人追蹤：「我身為一個 process，擁有什麼？」——這就是 **`mm_struct`** 的工作。

### 1.2 為什麼要分成區域

`[02:00]` 把虛擬記憶體切成一個個**局部連續**的區域，好處是：**每個區域可以套用一組標準政策**。

| 可以按區域設定的 | 例子 |
|---|---|
| **權限** | 這區域的所有 page 只能讀、不能寫；或只能讀、不能執行 |
| **安全政策** | |
| **置換政策** | 「這些是很高優先權的 page，**最後**才考慮踢它們」 |

`[03:31]` 怎麼追蹤這些區域，「本身就是好幾張投影片的旅程」，**不簡單**：

`[04:01]` 就算只問一個簡單問題——「**給一個虛擬位址，它屬不屬於某個區域？屬於哪個？**」——在高效能實作中都不容易。答案是一個叫 **maple tree** 的資料結構（§4）。

【課本補充】課本說（p.82）：與其把每個虛擬 page 分開處理，不如把它們**分組成區域**，給每個區域共同的屬性和政策——同樣的記憶體保護、讀寫政策、**處理 page fault 的方法**。memory map 裡除了 heap、stack、text、data、bss，還有記憶體映射檔案、共享 library，以及 process 擁有的資源的 handle 等雜項。

---

## 2. `mm_struct` 的欄位

### 2.1 總覽

`[02:00]`（課本 Fig 3.5）

| 欄位 | 內容 | 教授的說明 |
|---|---|---|
| **`struct maple_tree mm_mt`** | 所有 VM 區域 | 用 maple tree 存，§4 |
| `unsigned long task_size` | VM 空間的大小 | `[05:01]`「瑣碎的欄位」 |
| **`pgd_t *pgd`** | **指向 page table** | `[05:32]`「**非常重要**」，§2.2 |
| `int map_count` | VM 區域的數量 | |
| `total_vm`、`locked_vm`、`pinned_vm` | 總共映射的 page 數、**locked** page 數、**pinned** page 數 | §2.3 |
| `start_code`、`end_code`、`start_data`、`end_data`、`start_stack`…… | 重要區域的**起點和終點** | `[10:01]` |
| `struct task_struct *owner` | **擁有者 process**（指回 `task_struct`） | `[11:00]` |
| `unsigned long cpu_bitmap[]` | 這個 process **跑過哪些 CPU** | §2.4 |

`[10:31]` 「`mm_struct` 的工作就是**存 memory map**，你看到的其他東西都是圍繞著它，再加一個指向 page table 的指標。」

### 2.2 一個指標代表整張 page table

`[05:32]` 另一個非常重要的欄位：**指向 page table 的指標**。

> 【口頭】「整個多層 page table——包括它所有的資料和複雜性——**只用一個指標**表示。設計就是這麼**優雅**：每個 `task_struct` 指向一個 `mm_struct`，`mm_struct` 指向一張 page table。」

```
  task_struct ──mm──► mm_struct ──pgd──► 多層 page table（第 5 講的 PGD→PUD→PMD→PTE）
```

`[06:00]` page table 是巨大的結構，也是**低階結構**（因為要支援硬體走訪），但**一個指標**就指到它。型別叫 **`pgd_t`**（page global directory）——就是第 5 講的第一層 PGD。

【課本補充】課本強調（p.80）：「讀者要注意這裡**很高的抽象層級**。整張 page table 只用一個指標 `pgd_t *pgd` 參考。對 page table 的**所有操作**只需要這一個指標。這是很優雅的設計模式，在 kernel 裡一再重複。」

（第 5 講看過：換 process 時，把這個 `pgd` 寫進 **CR3**。）

### 2.3 Locked page 與 pinned page

`[06:31]` 兩個容易搞混的概念：

**Locked（鎖定）** `[06:31]`：

- 「**請把它留在記憶體裡，不要踢出去。**」
- 很高的優先權：除非**絕對緊急**（很多高優先權 process，或你鎖了太多 page），硬體不會踢它。
- 例子：**page table 本身的 page** 最好鎖住——「因為你沒有 page table 的 page table。」

**Pinned（釘住）** `[07:01]`：

```
  主記憶體
  ┌────────┐
  │ frame A│ ← 虛擬 page X 原本在這
  ├────────┤
  │  ...   │
  ├────────┤
  │ frame B│ ← 被踢出去、之後 page fault 搬回來時，可能放到這裡（換位置了）
  └────────┘
```

- 一個 frame 被踢到 swap space，之後 page fault 搬回來，**很可能放到不同位置**，映射就變了。
- **Pinning** 的意思：「我這個虛擬 page **永遠對應到同一個 frame**，**映射不變**。」

| | Locked | Pinned |
|---|---|---|
| 保證什麼 | **在記憶體裡**（不被踢到 swap） | **映射不變**（實體位置不動） |
| 一句話 | 維持你的**存在** | 維持你的**映射** |

`[08:30]` **pinning 對 page table 也是好主意**：

- 可以直接用**實體位址**處理。
- kernel 記憶體的很大一部分，**虛擬位址和實體位址一樣**（或只差一個固定偏移），「反正都要經過轉換，何不用一個**瑣碎的轉換**」——但只有在 pinned 時才可能，被踢出去再搬回來到別的位置就出問題了。

`[09:01]` 可以**同時** locked 又 pinned：映射維持、也一直在。

> 【口頭】「如果沒有人會踢你，你的映射本來就會一直維持，只有被踢掉時映射才可能改變。所以像 page table 這種超高優先權的 page，**鎖住就夠了**。但如果發生系統緊急狀況、page table 的 page 不得不被換出去，那就算要讀回來，也**必須讀回同一個位址**——所以要 pin。」

【課本補充】課本補充（p.80）：

- **Lock**：除非系統緊急，否則不換到硬碟。這會**消除那些 page 的 page fault**，代價是其他 page 的存取比較吃虧。
- **Pin**：不允許 kernel **在記憶體裡搬動**這些 page，也就是不改變虛擬→實體映射。正常的 page 可能隨時間被搬動。
- page table 的 page 被 pin 住，kernel 就能**直接用實體位址**存取 page table，不用查它在哪。這也**減少 TLB miss**，因為很多映射不會變。
- 參考 [Corbet, 2014]。

### 2.4 CPU bitmap

`[11:00]` 「需要多一點解釋的有趣資料結構」：

```
   CPU 編號：  0  1  2  3  4  5  6  7  8  9  10 ...
   bitmap：   0  0  1  0  0  0  0  0  0  0  1  ...
                    ↑                        ↑
              跑過 CPU 2                 跑過 CPU 10
```

- **每顆 CPU 一個位元**。這個 task 在 CPU 10 跑過，第 10 位元就設成 1。
- 換出去的 process 要換回來時，**選哪顆 CPU**？
  1. 最好是**上次跑的那顆**。
  2. 不行的話，選**最近跑過的**其中一顆——bitmap 存的就是這個資訊。
- 很久以前跑過的話，1 可以變回 0。

`[12:01]` 好處：**快取局部性**——快取是熱的、TLB 可能是熱的。

`[13:02]` 【口頭】有趣的問題：**為什麼 CPU bitmap 放在 `mm_struct` 裡**，而不是別的地方？

> 「因為從**記憶體的角度**來看很重要：如果我想最大化快取命中率，就要知道我最近在哪些 CPU 上跑過。還要有某種**抹掉過去**的演算法——很久以前跑過的沒意義，只要最近的，所以要有某種『近期』的標準。所以 CPU bitmap 放在這裡，讓**所有記憶體相關的資訊都在同一個地方**。」

`[12:33]` 總結 `mm_struct`：「這跟 Linux 無關——如果我要實作**任何** OS，都需要像這樣的東西：追蹤 memory map、所有虛擬記憶體 page，以及一個指向 page table 的指標。」

### 2.5 【原始碼】`struct mm_struct`

開頭和結尾：

```c
// include/linux/mm_types.h:601-617
struct mm_struct {
	struct {
		struct maple_tree mm_mt;
#ifdef CONFIG_MMU
		unsigned long (*get_unmapped_area) (struct file *filp,
				unsigned long addr, unsigned long len,
				unsigned long pgoff, unsigned long flags);
#endif
		unsigned long mmap_base;	/* base of mmap area */
		unsigned long mmap_legacy_base;	/* base of mmap area in bottom-up allocations */
#ifdef CONFIG_HAVE_ARCH_COMPAT_MMAP_BASES
		/* Base addresses for compatible mmap() */
		unsigned long mmap_compat_base;
		unsigned long mmap_compat_legacy_base;
#endif
		unsigned long task_size;	/* size of task vm space */
		pgd_t * pgd;
```

課本 Fig 3.5 的其他欄位（節錄，行號不連續）：

```c
// include/linux/mm_types.h:652, 681-683, 698-699, 728, 806-810
		int map_count;			/* number of VMAs */
		...
		unsigned long total_vm;	   /* Total pages mapped */
		unsigned long locked_vm;   /* Pages that have PG_mlocked set */
		atomic64_t    pinned_vm;   /* Refcount permanently increased */
		...
		unsigned long start_code, end_code, start_data, end_data;
		unsigned long start_brk, brk, start_stack;
		...
		struct task_struct __rcu *owner;
		...
	} __randomize_layout;

	/*
	 * The mm_cpumask needs to be at the end of mm_struct, because it
	 * is dynamically sized based on nr_cpu_ids.
	 */
	unsigned long cpu_bitmap[];
};
```

| 行 | 欄位 | 對照 |
|---|---|---|
| 603 | **`mm_mt`** | **maple tree**：存所有 VMA。**第一個欄位** |
| 605–607 | `get_unmapped_area` | **函式指標**（第 2 講說 kernel 到處用）：要配新區域時，找一塊**沒用到**的空間。不同架構 / 情況可以換不同函式 |
| 609 | `mmap_base` | mmap 區域（library、映射檔案）的起點——memory map 中間那塊 |
| 616 | `task_size` | 使用者空間大小（x86-64 是第 5 講的 47 位元） |
| 617 | **`pgd`** | **整張 page table 只用這一個指標** |
| 652 | `map_count` | VMA 數量 |
| 681 | `total_vm` | 總共映射幾個 page |
| 682 | `locked_vm` | 註解：「設了 `PG_mlocked` 的 page」——使用者用 `mlock()` system call 鎖住 |
| 683 | `pinned_vm` | 註解：「**引用計數被永久增加**」——kernel 的 pin 機制：讓 page 的引用計數一直 > 0，系統就不會搬它。型別是 `atomic64_t`，可以多核心同時安全地加減 |
| 698 | `start_code`…`end_data` | text、data 的起終點 |
| 699 | `start_brk`、**`brk`**、`start_stack` | **heap 的起點和目前頂端**（`brk` 就是 `malloc` 底層用的 `brk()` system call 調整的那個值）、stack 起點 |
| 728 | `owner` | 擁有者 `task_struct` |
| 804 | `__randomize_layout` | 跟 `task_struct` 一樣：**欄位順序隨機化**，防攻擊 |
| 810 | **`cpu_bitmap[]`** | **CPU bitmap**。註解：「必須放在 `mm_struct` **最後**，因為它的大小**依 CPU 數量動態決定**」 |

**為什麼 `cpu_bitmap` 要放最後？** `unsigned long cpu_bitmap[];` 沒有寫大小——這是 C 的 **flexible array member（彈性陣列成員）**。配置 `mm_struct` 時才依實際 CPU 數多配一點空間接在後面。只有**最後一個欄位**能這樣做。這又是「低階資料結構」的例子：知道記憶體怎麼排。

---

## 3. VM 區域：`vm_area_struct`

### 3.1 每個區域的起點與終點

`[13:30]` memory map 裡一個連續的 VM 區域，用 **`struct vm_area_struct`** 表示，簡稱 **VMA**（VM area）。

- 每個 VMA 有 **`vm_start`** 和 **`vm_end`**：一個連續區域的起點和終點。
- 每個 VMA 屬於一個 `mm_struct`，所以有一個**指回去的指標**（`vm_mm`）。
- 【口頭】「這是**循環關係**：`mm_struct` 有很多 VMA，每個 VMA 指回它的父親。這沒什麼高深的。」

```
          mm_struct
         ╱    │    ╲
       VMA   VMA   VMA    （maple tree 管理）
        │     │     │
        └─────┴─────┴──── vm_mm 指回 mm_struct
```

### 3.2 Anonymous 與 file-backed

`[14:32]` 教授說記憶體那章會細講，這裡先講概念——「**重要的概念**」。

`[15:01]` 列出各個區域：

| 區域 | 性質 |
|---|---|
| Stack | ? |
| Heap | ? |
| Data / BSS | ? |
| Text（程式碼） | ? |
| 記憶體映射檔案 / library | ? |

`[15:30]` 它們的**本質非常不同**：

**Stack 和 heap** `[16:32]`：

- 隨 `main` 裡的邏輯**長大縮小**：呼叫更多函式（例如遞迴）stack 就長；配更多記憶體 heap 就長。
- **完全是執行時產生的**。程式結束後，這些區域就**完全清空**。

**Text 和記憶體映射檔案** `[17:01]`：

- 真正的資料在**某個檔案**裡（檔案 = 儲存裝置上一串連續的 bytes）。
- 檔案的一部分被映射到 process 的記憶體空間。例如：執行檔裡有你所有的程式碼，一部分被映射到你的虛擬記憶體；library 檔案裡有某個函式，那個函式的程式碼被映射到你的虛擬記憶體。
- 就算把這區域**清掉**，也能**從檔案再拿回來**。

**Data / BSS** `[15:30]`、`[18:02]`：

- 在 `main` 之前的程式碼裡（編譯器加的）**初始化**：把有初值的變數的立即值載入記憶體，「程式某種程度上**自己載入自己**」。
- 這些區域**不太會長大縮小**。
- 大部分算 anonymous（程式產生的）；但如果是從檔案**整塊複製**過來的，就算 file-backed。「這裡先不表態。」

`[18:02]` 兩種 page：

| 類型 | 是什麼 | 壽命 | 例子 |
|---|---|---|---|
| **Anonymous（匿名）** | **程式自己產生**的 | **短暫（ephemeral）**：程式結束就沒了 | stack、heap、BSS |
| **File-backed（檔案支撐）** | 檔案的一段連續內容反映在虛擬記憶體裡 | 可以從檔案再拿回來 | text、記憶體映射檔案、共享 library |

`[19:00]` 「任何 OS 裡**只有這兩種** page。」

`[19:00]` 管理方式：

- **Anonymous page 的儲存是個挑戰**——「我們會用**很辛苦的方式**體會到這一點」。需要非常精密的資料結構，叫 **`anon_vma_chain`**，「這個複雜的怪物之後再看」（記憶體那章）。
- **File-backed**：VMA 知道是**哪個檔案**在支撐它，所以存一個指向那個檔案的指標 **`struct file *`**。「我不是 anonymous，我是 file-backed，這是存我備份資料的檔案。」

【課本補充】課本補充（p.83）：

- **Anonymous**：不是從檔案鏡像或複製來的，例如 stack 和 heap。在 process 執行期間產生，存**動態配置**的資料。動態存在，跟二進位檔或目的檔的特定 section 沒有關係。
- **File-backed**：通常**寫進那個區域的修改，最後會反映到支撐它的檔案**。
- 追蹤 anonymous 記憶體的資料結構其實是「**一張由 linked list 組成的複雜圖**」，講實體記憶體配置（特別是**反向映射**）時再看。

### 3.3 【原始碼】`struct vm_area_struct`

課本 Listing 3.5 是簡化版，完整版（節錄）：

```c
// include/linux/mm_types.h:535-577（節錄）
struct vm_area_struct {
	/* The first cache line has the info for VMA tree walking. */

	unsigned long vm_start;		/* Our start address within vm_mm. */
	unsigned long vm_end;		/* The first byte after our end address
					   within vm_mm. */

	struct mm_struct *vm_mm;	/* The address space we belong to. */

	/*
	 * Access permissions of this VMA.
	 * See vmf_insert_mixed_prot() for discussion.
	 */
	pgprot_t vm_page_prot;
	unsigned long vm_flags;		/* Flags, see mm.h. */
	...
	/*
	 * A file's MAP_PRIVATE vma can be in both i_mmap tree and anon_vma
	 * list, after a COW of one of the file pages.	A MAP_SHARED vma
	 * can only be in the i_mmap tree.  An anonymous MAP_PRIVATE, stack
	 * or brk vma (with NULL file) can only be in an anon_vma list.
	 */
	struct list_head anon_vma_chain; /* Serialized by mmap_lock &
					  * page_table_lock */
	struct anon_vma *anon_vma;	/* Serialized by page_table_lock */

	/* Function pointers to deal with this struct. */
	const struct vm_operations_struct *vm_ops;

	/* Information about our backing store: */
	unsigned long vm_pgoff;		/* Offset (within vm_file) in PAGE_SIZE
					   units */
	struct file * vm_file;		/* File we map to (can be NULL). */
```

| 行 | 欄位 | 對照教授說的 |
|---|---|---|
| 536 | 註解 | 「**第一條快取線**放走訪 VMA 樹需要的資訊」——又是快取線意識 |
| 538 | `vm_start` | 起點 |
| 539–540 | `vm_end` | 註解：「**結尾之後的第一個 byte**」。也就是區間是 **[vm_start, vm_end)**，左閉右開 |
| 542 | `vm_mm` | **指回 `mm_struct`**（循環關係） |
| 548–549 | `vm_page_prot`、`vm_flags` | 這個區域的**權限和旗標**——就是教授說的「每個區域套用一組標準政策」 |
| 561–565 | 註解 | 「**anonymous 的 MAP_PRIVATE、stack、brk（heap）VMA，`file` 是 NULL**，只能在 anon_vma 串列裡」——stack、heap 就是 anonymous |
| 567 | **`anon_vma_chain`** | anonymous 記憶體那個「複雜的怪物」 |
| 572 | `vm_ops` | **函式指標表**：這個區域發生 page fault 時該怎麼處理（file-backed 和 anonymous 不同） |
| 575 | `vm_pgoff` | 映射的是**檔案的哪一段**（以 page 為單位的偏移） |
| 577 | **`vm_file`** | **支撐這個區域的檔案**。註解：「can be NULL」——**NULL 就是 anonymous** |

所以判斷一個 VMA 是不是 anonymous，就是看 **`vm_file` 是不是 NULL**。第 5 講的 `/proc/self/maps` 裡，有檔名的那幾行（`/usr/bin/cat`、`libc.so.6`）是 file-backed；`[heap]`、`[stack]`、沒名字的那幾行是 anonymous。

---

## 4. 從二元搜尋樹到 maple tree

### 4.1 問題：這個位址屬於哪個區域？

`[21:02]` 要回答：

> **給一個虛擬位址，它屬於哪個 VMA？或者不屬於任何 VMA？**

【課本補充】課本 Question 3.1.1 一模一樣的問題。

### 4.2 楓葉：不是二元的

`[20:01]` 【口頭】

> 「這對北美的人可能是常識，但印度的人除非去過**山區避暑勝地（hill station）**，可能沒看過楓葉。楓葉是**加拿大的國家象徵**，在國旗上。你看楓葉有某種結構：很漂亮的階層結構，而且**不是二元的**——它分成 5 片，每片又 1、2、3、4、5……**大自然啟發科學**。」

`[21:02]` 楓葉啟發我們建一棵 **k 元樹（k-ary tree）**。當然比那複雜一點。

`[21:33]` 「現在先把『樹』這件事放一邊，**一步步往上走**。」

### 4.3 二元樹的問題：快取

`[22:01]` 大學資料結構課學過：

- **二元樹** → **二元搜尋樹（BST）**：如果**平衡**，搜尋是 **O(log n)**。
- 傳統 BST 的平衡變體：**紅黑樹**、**AVL 樹**——演算法複雜，但平衡。高度 O(log n)，搜尋 O(log n)。

`[22:30]` 但**紅黑樹是二元的，其實不是好主意**：

> 【口頭】「整個世界不是只有**計算複雜度**，要看**實際執行時間**——那才是真正重要的。」

`[23:01]` 考慮一條 **64-byte 快取線**——快取和實體記憶體系統裡的**原子儲存單位**（也可能是 128 bytes）。

- 二元樹的節點存什麼？**左子指標和右子指標**（16 bytes）。
- 快取線剩下的空間呢？**可能存了別的節點**，或別的東西。

`[24:02]` 問題：

1. **False sharing**：假設多個 thread 平行存取。只讀的話沒問題，各自有副本。但**偶爾有寫入**時：一個核心存取這部分、另一個核心存取那部分，在同一條快取線上 → **快取線彈來彈去**。
2. **層數太多**：二元樹變大時，**層數很多**，「日子就難過了」。

`[25:01]` 解法：**多元樹（multi-way tree）**，就像楓葉：

> 「一個節點存**盡可能多**的子節點——**能塞進一條快取線**的那麼多。這樣就不可能有**不相干的資料**存在同一條快取線上。」

`[25:31]` 當然多個核心還是可能衝突在同一個節點上，但那時它們**存取的位址相近**——那是真的在搶同一份資料。而二元樹的情況是：**位址根本不相近**卻在衝突。

`[26:00]` 「所以把節點結構**對齊到快取線大小**、有很多子節點，是更好的主意。」有 n 個節點，需要 **log_m(n)** 層；每層內部可以像 BST 一樣搜尋。

`[26:31]` 也可以用 **hash table**，但「hash table 很難產生**排序過的 key 清單**，很難做**範圍查詢（range query）**，因為 hash table 打亂了 key 的順序。有時候我就是需要那個順序。」

【課本補充】課本補充（p.65）：

- 給 key 找 value 的搜尋問題，hash table 常常**不是好主意**，特別是不確定要存多少 key 時。它們的**快取局部性差**，也不容易做範圍查詢。
- 樹很通用：對數時間就能做很多事，支援**並行存取**和高度快取友善的組織，**節點數量**也自然可擴展。
- 所以更常用 **m 元 B 樹**：節點大小等於一個或多個快取區塊，**減少快取區塊的讀取次數**，也方便**節點層級的鎖**。

### 4.4 B 樹

`[26:31]`（課本附錄 C.3）**m 階 B 樹（B-tree of order m）**：

| 規則 | 內容 |
|---|---|
| 1 | 每個節點**最多 m 個子節點** |
| 2 | **根節點**至少要有 **1 個 key** |
| 3 | 每個**內部節點**至少有 **⌈m/2⌉ 個子節點**——「不這樣，怎麼保證深度小？」 |
| 4 | 有 k 個子節點的節點，存 **k − 1 個 key**。「兩個子節點就一個 key 隔開；k 個子節點就 k − 1 個分隔」 |
| 5 | **所有葉子都在同一層**——平衡 |

`[27:00]` 怎麼走：

```
        ┌────────┬────────┐
        │  key₁  │  key₂  │       ← 節點裡的 key 是排序過的
        └───┬────┴───┬────┴───┬───
            │        │        │
         < key₁   key₁–key₂  > key₂
```

- 小於 key₁ → 走左邊。
- 在 key₁ 和 key₂ 之間 → 走中間。
- 大於 key₂ → 走右邊。
- 節點裡的 key 是排序過的，用**二元搜尋**找該走哪個子節點，花 **O(log m)**。

`[28:02]` **時間複雜度**：

```
  層數 log_m(n) × 每層 log(m) = log_m(n) × log(m) = log(n)
```

> 【口頭】「所以**不是特別快**，漸近上還是一樣是 O(log n)。但如我所說：**層數少**，而且**比較快取友善**。」

### 4.5 B 樹的例子

`[30:00]`（課本 Fig C.1，order 3）

```
                      ┌─────┬──────┐
                      │  6  │  12  │
                      └──┬──┴──┬───┴──┬──
              ┌──────────┘     │      └──────────┐
         ┌────┴┬────┐     ┌────┴┬─────┐     ┌────┴┬─────┐
         │  2  │  4 │     │  8  │  10 │     │ 16  │  25 │
         └─┬───┴─┬──┴─┬─  └─┬───┴──┬──┴──┬─ └─┬───┴──┬──┴──┬─
           1     3    5     7      9     11   14     17    26
```

- 根：**6** 和 **12**。小於 6 → 左；6 到 12 → 中；大於 12 → 右。
- 左子樹：**2** 和 **4**。小於 2 是 1；2 到 4 是 3；4 到 6 是 5。
- 中子樹：**8** 和 **10**。小於 8 是 7；8 到 10 是 9；10 到 12 是 11。
- 右子樹：**16** 和 **25**。14 大於 12、小於 16；17 在 16 和 25 之間；26 大於 25。

### 4.6 B+ 樹

`[30:30]` 兩種變體：

| | B 樹 | B+ 樹 |
|---|---|---|
| key 存在哪 | **任何節點**（內部節點也可以） | **只存在葉子** |
| 內部節點的 key | 可能是真的資料，也可能只是分隔 | **只是分隔（separator）**，不關聯 value |

`[30:30]` 「B+ 樹更**有紀律**。」在 B 樹裡（像上面的 6 和 12），有兩種 key：一種**只扮演分隔者**，可能根本不存在資料結構裡、沒有 value；另一種**關聯一個 value**。這讓儲存變複雜，所以 **B+ 樹比較好**。

`[35:30]` B+ 樹的好處：**不用在每個節點存 value 或指向 value 的指標**，所以節點結構**更一致（homogeneous）**。

【課本補充】課本 C.3.3：B+ 樹的所有葉子還**用 linked list 串起來**，所以**範圍查詢**非常有效率，也可以在串列裡循序搜尋、快速找到相鄰的 key。

### 4.7 插入與刪除：分裂與合併

`[36:31]`

**插入**：

1. 沿路往下走。
2. 那裡有空位 → 直接放進去。
3. **父節點滿了** → **分裂（split）** 成兩個父節點。
4. 但多了一個父節點，**祖父節點**就要多一個**分隔 key**（「小於它走這邊、大於它走那邊」）——所以祖父節點要有空位。
5. 祖父節點也滿了 → 分裂祖父節點。
6. 一直往上，直到根。根也滿了 → **分裂根**（樹長高一層）。

**刪除** `[38:01]`：反過來。

- 拿掉 key。
- 如果違反了「每個內部節點至少 ⌈m/2⌉ 個子節點」→ **合併（merge）**。

| 操作 | 違反條件時 |
|---|---|
| 插入 | **分裂** |
| 刪除 | **合併** |

「這些細節在核心資料結構課講得比較好。」

【課本補充】這些操作**都不會改變任何葉子的深度**。最壞情況要分裂根、建新的父節點，所有葉子的深度一樣、只是樹高 +1。刪除最壞情況是樹高 −1。

### 4.8 為什麼 B 樹比較快取友善

`[39:00]` 再從另一個角度看。

**二元樹** `[39:31]`：

```
           ●           ← 快取線 1
         ╱   ╲
        ●     ●        ← 快取線 2、3（可能）
       ╱ ╲   ╱ ╲
      ●   ● ●   ●      ← 快取線 4、5、6、7……
```

- 每一層可能在**不同的快取線**上 → 很多次快取存取。

**B 樹** `[40:00]`：

```
  ┌──────────────────────────────┐
  │ key key key key key key key   │  ← 一條快取線，塞滿完整的 key 資訊
  └──────────────────────────────┘
       讀一條快取線就知道下一步去哪
```

- 每個節點**完全塞滿**一條（或幾條）快取線。
- 讀一條快取線 → 拿到**完整的 key 資訊**，知道要去哪 → 再讀下一條……

`[40:31]` 結果：

| | 二元樹 | B 樹 |
|---|---|---|
| 快取讀取次數 | 多 | **少** |
| 每次讀取的「產值」 | 低：一條快取線只用一點點 | **高**：一條快取線裡全是有用的 key 和指標 |
| 同一條快取線上有誰 | **不知道**：可能是別的核心正在**寫**的東西 → 一直彈來彈去 → 效能被殺掉 | **全是同一個 B 樹節點** |
| False sharing | 有 | **沒有**（別的核心真的在寫同一個節點，那是**真共享**） |
| 鎖 | 難 | 容易做**節點層級的鎖**：在節點裡留一個欄位當鎖，**一起塞在快取線裡**，鎖住整棵子樹 |

`[42:33]` 「好處很多，所以傳統上**關聯式資料庫**都用它。」資料庫用在**硬碟區塊**層級，因為硬碟存取非常非常慢，好處很大；但在快取這裡，好處也很明顯。

【課本補充】課本 C.3.4：

- 64 或 128 byte 的快取區塊是記憶體系統的**原子傳輸單位**。BST 每個節點只存兩個指標，「讀了 64 bytes 卻只用 25%，**沒有意義**」。
- 統計上快取失誤會變少，長時間的記憶體停頓機會低很多。

### 4.9 Maple tree

`[34:02]` **Linux 的 maple tree**：

| 特性 | 內容 |
|---|---|
| 基礎 | **B+ 樹**的變體 |
| **key** | **不是一個值，是一個範圍**：**(start, end)** 兩個數字 |
| 為什麼能這樣 | 範圍**互不重疊** → 可以**完全排序**：比較兩個範圍，只要拿一個的下界跟另一個的上界比 |
| 存什麼 | **VMA 的範圍**——兩個 VMA 永遠不會重疊，所以可以這樣排成 B+ 樹 |
| 分支因子 | **寫死的**：教授說**非葉節點 10、葉節點 16** |
| 節點大小 | **256 bytes = 4 條快取線** |
| 分隔 key | 可以是**單一值**，沒問題 |

`[44:00]` 教授強調：

> 「我說的是 **kernel 用的 maple tree** 的定義，不是一般 maple tree 的定義。」

`[45:30]` 好處：

- 比傳統**紅黑樹快**。
- 每個節點塞進快取區塊 → 有效率、**減少 false sharing**。
- 減少 false sharing → 容易**平行**：多個 thread 可以同時讀、寫、改 maple tree 的不同部分。重疊在同一棵子樹時要上鎖，但鎖很容易做。

**對照課本**：

- 課本 Fig 3.5 和 §3.1.9：maple tree 是 B+ 樹的精密變體，key 是 **2-tuple（區域的起點和終點）**，互不重疊，所以走訪 maple tree 就能找出位址屬於哪個區域，**對數時間**。
- 課本附錄 C.4：「非葉節點最多 **10 個子節點（9 個 key）**，葉節點最多存 **15 個 entry**。」
- ⚠️ 不一致：教授說葉節點 **16**，課本說 **15**。看原始碼（§4.10）：葉節點有 **16 個 slot**、**15 個 pivot**（分隔值）——兩個數字都對，只是一個數 slot、一個數 pivot。
- 課本還說：節點**對齊快取線邊界**，消除 false sharing；可以服務**並行存取**；key 可以是單一值，也可以是範圍（VMA 的情況）。參考 [Rybczynska, 2021, Howlett, 2021]。

### 4.10 【原始碼】maple tree 的節點

```c
// include/linux/maple_tree.h:28-34（64 位元）
#if defined(CONFIG_64BIT) || defined(BUILD_VDSO32_64)
/* 64bit sizes */
#define MAPLE_NODE_SLOTS	31	/* 256 bytes including ->parent */
#define MAPLE_RANGE64_SLOTS	16	/* 256 bytes */
#define MAPLE_ARANGE64_SLOTS	10	/* 240 bytes */
#define MAPLE_ARANGE64_META_MAX	15	/* Out of range for metadata */
#define MAPLE_ALLOC_SLOTS	(MAPLE_NODE_SLOTS - 1)
```

| 常數 | 值 | 註解 | 對照教授說的 |
|---|---|---|---|
| `MAPLE_RANGE64_SLOTS` | **16** | 256 bytes | **葉節點 16**（存範圍） |
| `MAPLE_ARANGE64_SLOTS` | **10** | 240 bytes | **非葉節點 10** |
| 節點大小 | — | **256 bytes** | **4 條快取線**（64 × 4） |

兩種節點結構：

```c
// include/linux/maple_tree.h:106-116
struct maple_range_64 {
	struct maple_pnode *parent;
	unsigned long pivot[MAPLE_RANGE64_SLOTS - 1];
	union {
		void __rcu *slot[MAPLE_RANGE64_SLOTS];
		struct {
			void __rcu *pad[MAPLE_RANGE64_SLOTS - 1];
			struct maple_metadata meta;
		};
	};
};
```

```c
// include/linux/maple_tree.h:118-133
/*
 * At tree creation time, the user can specify that they're willing to trade off
 * storing fewer entries in a tree in return for storing more information in
 * each node.
 *
 * The maple tree supports recording the largest range of NULL entries available
 * in this node, also called gaps.  This optimises the tree for allocating a
 * range.
 */
struct maple_arange_64 {
	struct maple_pnode *parent;
	unsigned long pivot[MAPLE_ARANGE64_SLOTS - 1];
	void __rcu *slot[MAPLE_ARANGE64_SLOTS];
	unsigned long gap[MAPLE_ARANGE64_SLOTS];
	struct maple_metadata meta;
};
```

**算算看大小**（64 位元，每個指標 / `unsigned long` 8 bytes）：

| 結構 | parent | pivot | slot | gap | 合計 |
|---|---|---|---|---|---|
| `maple_range_64` | 1 | **15** | **16** | — | 32 × 8 = **256 bytes** |
| `maple_arange_64` | 1 | **9** | **10** | **10** | 30 × 8 = **240 bytes** |

（`meta` 在 range_64 裡跟最後一個 slot 共用空間，在 arange_64 裡用掉剩下的一點點。）

| 欄位 | 意思 | 對照 B+ 樹 |
|---|---|---|
| `parent` | 指向父節點 | — |
| **`pivot[]`** | **分隔值**。`pivot[i]` 是第 i 個 slot 涵蓋範圍的**上界** | 就是 B 樹的 **key**：k 個子節點、k − 1 個 key。range_64 有 16 slot、**15 pivot**——這就是課本說的「15」 |
| **`slot[]`** | 指向子節點（內部節點），或指向**資料**（葉節點：就是 `vm_area_struct *`） | 子節點指標 / value |
| **`gap[]`** | **只有 arange 有**：每個子樹裡**最大的一段空白**有多大 | B 樹沒有，maple tree 的擴充 |
| `__rcu` | 標記這個指標用 **RCU** 保護，讀的人不用上鎖 | 第 23 講 |

**`gap[]` 是做什麼的？** 註解說「記錄這個節點裡**最大的一段 NULL（空白）範圍**，這讓樹在**配置一段範圍**時最佳化」。

- 想想 `mmap()`：程式說「給我 1 MB 的空間」，kernel 要在 memory map 裡**找一段夠大的空白**。
- 如果每個內部節點都記得「我底下最大的空白有多大」，就可以直接跳過那些空白不夠大的子樹——**O(log n) 找到空位**。
- 這就是 §2.5 `mm_struct` 裡那個 `get_unmapped_area` 函式指標會用到的。
- 所以 VMA 的 maple tree 用 **arange（allocation range）** 型的內部節點：**少存一點 entry（10 而不是 16），換來每個節點多存 gap 資訊**——註解第一段說的「trade off」。

節點型別：

```c
// include/linux/maple_tree.h:147-152
enum maple_type {
	maple_dense,
	maple_leaf_64,
	maple_range_64,
	maple_arange_64,
};
```

### 4.11 【原始碼】`find_vma()`：一行查完

「給一個位址，找它屬於哪個 VMA」，kernel 的函式：

```c
// mm/mmap.c:1830-1837
struct vm_area_struct *find_vma(struct mm_struct *mm, unsigned long addr)
{
	unsigned long index = addr;

	mmap_assert_locked(mm);
	return mt_find(&mm->mm_mt, &index, ULONG_MAX);
}
EXPORT_SYMBOL(find_vma);
```

| 行 | 意思 |
|---|---|
| 1830 | 給 `mm_struct` 和一個虛擬位址 |
| 1834 | 確認呼叫者**已經持有** `mmap_lock`（記憶體映像的讀寫鎖） |
| 1835 | **`mt_find`**：在 `mm->mm_mt`（maple tree）裡，從 `addr` 開始往上找第一個 entry。回傳的就是 `vm_area_struct *` |
| 1837 | `EXPORT_SYMBOL`：第 4 講看過，給 kernel module 用 |

整個 maple tree 的複雜度被藏在 `mt_find` 後面——呼叫者只要**一行**。注意它找的是「**第一個結尾在 addr 之後的 VMA**」：可能包含 addr，也可能在 addr 上面（addr 落在空洞裡）。呼叫者要再檢查 `vma->vm_start <= addr`。

page fault 時，kernel 第一件事就是呼叫類似的函式：「出錯的位址在哪個 VMA？」——找不到 → 非法存取 → **segmentation fault**；找到了 → 用那個 VMA 的 `vm_ops` 處理。這就是第 5 講「位址非法 → exception」的實際實作。

> 歷史補充（不是教授講的）：maple tree 是在 **Linux 6.1** 才取代原本存 VMA 的**紅黑樹 + linked list**。所以課本選 6.2 版，剛好用上新設計；更舊的 kernel 教科書講的都是紅黑樹版本。

---

## 5. 再看 anonymous 與 file-backed

`[46:30]` **File-backed（non-anonymous）**：虛擬記憶體空間的一部分**鏡像一個執行檔**——執行檔裡的那些 bytes 被讀出來、放進 process 的記憶體映像、映射到虛擬記憶體系統。

| 區域 | 說明 |
|---|---|
| **程式碼（code）** | 所有指令都在執行檔裡，複製進來放到 memory map |
| **Data** | **有初值的全域變數**：初始化的常數也在執行檔裡，讀出來複製進位址空間 |
| **共享 library** | 「共享 library 的**大好處**：20 個程式呼叫 `printf`，**不用 20 份** `printf`，**只有一份**，映射到多個 process」（第 4 講） |

`[48:04]` **Anonymous**：程式執行期間**動態產生**的區域：

| 區域 | 說明 |
|---|---|
| **Stack** | |
| **Heap** | |
| **BSS** | **沒有初值**的全域變數 |

`[48:37]` 兩種要**用不同方式管理**：

- File-backed：很多時候**寫入要送回檔案**。
- File-backed 不會像 anonymous 那樣長大縮小。

`[49:01]` `task_struct` 裡其實有**兩個** `mm_struct` 指標：**`mm`** 和 **`active_mm`**。「兩個都需要，但現在不想開這個話題，講 context switch 時再說。」

---

## 6. 抽象化：外界只看到一個指標

`[49:32]` 該體會的是**抽象化（abstraction）**：

> 【口頭】「外界**不在乎** OS 的記憶體系統怎麼實作。他們只在乎：我有一個叫 `mm_struct` 的 struct，據說包含所有資訊，還有一堆函式可以處理它。」

`[50:00]` C 和物件導向的對比：

- 這是 C，所以函式放在**外面**某個檔案裡。
- 如果是 C++ 或 Java，這會是一個漂亮的 **class**，函式在 class 裡面。「這是物件導向的一個好處。」
- 但**概念一樣**：**隱藏（hiding）**。「我不在乎你怎麼實作記憶體系統，隨你怎麼做，給我一組函式就好。我送出和收到的只有一個 `mm_struct` 指標。」

> 「這是電腦科學裡**經典的抽象化**：你**刻意不在乎**內部細節。只要介面不變，內部細節不重要。」

（§4.11 的 `find_vma()` 就是例子：呼叫者完全不用知道 maple tree 的存在。）

`[51:01]` 下一講：**process ID** 和 process 怎麼被管理。「之前講的都是**單一** process、單一 task 的各方面；現在要講**多個** process 怎麼管——需要**另外的資料結構**。」

---

## 7. 【實測】一個 process 有幾個 VMA

```bash
$ /bin/cat /proc/self/maps | wc -l
25
```

一個最簡單的 `cat` process 就有 **25 個 VMA**（`/proc/self/maps` 的每一行就是一個 `vm_area_struct`，數量就是 `mm_struct->map_count`）。第 5 講看過它們的內容：`cat` 本身 5 段、heap、libc 6 段、ld-linux 5 段、stack、vdso……

`/proc/self/status` 裡的統計，對應 `mm_struct` 的欄位：

```text
VmSize:	   10292 kB     ← total_vm（總共映射多少）
VmLck:	       0 kB     ← locked_vm（mlock 鎖住的）
VmPin:	       0 kB     ← pinned_vm（被 pin 住的）
VmData:	     360 kB     ← data + heap 等
VmStk:	     136 kB     ← stack
VmExe:	      16 kB     ← end_code − start_code（程式碼）
VmLib:	    1796 kB     ← 共享 library
```

- **VmLck 和 VmPin 都是 0**：一般程式不會鎖 page。
- **VmLib（1796 kB）遠大於 VmExe（16 kB）**：`cat` 自己的程式碼只有 16 KB，大部分是**共享 library**——而這些 library 的實體記憶體是跟其他 process **共用**的（§5）。
- **VmSize（約 10 MB）** 是**虛擬**大小，不是實際用掉的實體記憶體。

---

## 8. 一頁速查表

### 8.1 `mm_struct`

| 欄位 | 意思 |
|---|---|
| `mm_mt` | **maple tree**：所有 VMA |
| `pgd` | **整張 page table 只用這一個指標**（換 process 時寫進 CR3） |
| `task_size` | 使用者空間大小 |
| `map_count` | VMA 數量 |
| `total_vm` / `locked_vm` / `pinned_vm` | 總 page / 鎖住的 / 釘住的 |
| `start_code`…`start_stack`、`brk` | 各區域的起終點；`brk` 是 heap 頂端 |
| `owner` | 指回 `task_struct` |
| `cpu_bitmap[]` | 跑過哪些 CPU（彈性陣列，放最後） |
| `get_unmapped_area` | 函式指標：找空白區域 |

### 8.2 Locked vs Pinned

| | 保證 | 用途 |
|---|---|---|
| Locked | **在記憶體裡**（不被換出） | 避免 page fault；`mlock()` |
| Pinned | **映射不變**（實體位置不動） | page table、DMA；可直接用實體位址 |

### 8.3 `vm_area_struct`（VMA）

| 欄位 | 意思 |
|---|---|
| `vm_start`、`vm_end` | 區間 [start, end) |
| `vm_mm` | 指回 `mm_struct` |
| `vm_page_prot`、`vm_flags` | 權限、政策 |
| `anon_vma_chain` | anonymous 記憶體的複雜結構 |
| `vm_ops` | 函式指標表（page fault 怎麼處理） |
| `vm_file` | 支撐的檔案；**NULL = anonymous** |

### 8.4 Anonymous vs File-backed

| | Anonymous | File-backed |
|---|---|---|
| 來源 | 程式自己產生 | 檔案的一段 |
| 例子 | stack、heap、BSS | text、data、共享 library、mmap 檔案 |
| 壽命 | 短暫 | 可從檔案拿回 |
| 大小 | 會長大縮小 | 大致固定 |
| `vm_file` | NULL | 指向檔案 |

### 8.5 樹的比較

| | 二元搜尋樹（紅黑樹） | B 樹 | B+ 樹 | Maple tree（kernel） |
|---|---|---|---|---|
| 每節點子數 | 2 | 最多 m | 最多 m | 內部 10、葉 16 |
| key 在哪 | 每個節點 | 任何節點 | **只在葉子** | 只在葉子；key 是**範圍** |
| 複雜度 | O(log n) | O(log n) | O(log n) | O(log n) |
| 層數 | log₂ n | log_m n | log_m n | log_m n |
| 快取 | 差、false sharing | 好 | 好 | 節點 256 B = 4 條快取線 |
| 鎖 | 難 | 節點層級鎖 | 節點層級鎖 | 加上 RCU |
| 額外 | — | — | 葉子串成 linked list | 內部節點記錄 **gap**，加速找空位 |

### 8.6 B 樹規則（m 階）

| # | 規則 |
|---|---|
| 1 | 每節點最多 m 個子節點 |
| 2 | 根至少 1 個 key |
| 3 | 內部節點至少 ⌈m/2⌉ 個子節點 |
| 4 | k 個子節點 → k − 1 個 key |
| 5 | 所有葉子同一層 |
| 插入滿了 | 分裂，可能一路分裂到根 |
| 刪除太少 | 合併 |

### 8.7 本講看過的原始碼位置

| 檔案:行號 | 看什麼 |
|---|---|
| `include/linux/mm_types.h:601-617` | `mm_struct` 開頭：`mm_mt`、`pgd` |
| `include/linux/mm_types.h:652-728` | `map_count`、`total_vm`、`locked_vm`、`pinned_vm`、區域起終點、`owner` |
| `include/linux/mm_types.h:806-810` | `cpu_bitmap[]`（彈性陣列） |
| `include/linux/mm_types.h:535-577` | `vm_area_struct` |
| `include/linux/maple_tree.h:28-34` | 節點大小：16 / 10 slot、256 bytes |
| `include/linux/maple_tree.h:106-133` | `maple_range_64`、`maple_arange_64`（pivot、slot、gap） |
| `include/linux/maple_tree.h:147-152` | 節點型別 |
| `mm/mmap.c:1830-1837` | `find_vma()` |

---

## 9. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **多個 process 怎麼管、PID** | **第 10 講**：PID 與 namespace（下一講；也會用到 radix tree 等資料結構） |
| **`fork` 時複製 `mm_struct`、VMA** | **第 11 講**：process 建立 |
| **`mm` 與 `active_mm` 兩個指標、換 `pgd`** | **第 12–13 講**：context switch |
| **Page fault 時用 `find_vma` 找 VMA** | **第 16 講**：exception |
| **RCU（`__rcu` 標記）** | **第 23 講**：RCU |
| **Anonymous page、`anon_vma_chain`、反向映射** | **第 31 講**：reverse mapping |
| **置換政策、locked page 不被踢** | **第 29 講**：頁面置換；**第 32 講**：MGLRU |
| **Pinned page、kernel 記憶體虛擬 = 實體 + 偏移** | **第 30 講**：kernel 的記憶體架構 |
| **File-backed page、檔案** | 課本第 7 章（檔案系統）。對應講次**待補** |

---

## 附：本講一句話總結

> **每個 process 的記憶體資訊都在 `mm_struct` 裡：一個 `pgd` 指標代表整張 page table，一棵 maple tree 存所有不重疊的虛擬記憶體區域（VMA，分成程式自己產生的 anonymous 和從檔案來的 file-backed）。maple tree 是 key 為 (start, end) 範圍的 B+ 樹，節點 256 bytes 剛好 4 條快取線（內部 10 個子、葉子 16 個 slot）——用多元樹取代紅黑樹，不是為了漸近複雜度，而是為了更少的快取讀取、沒有 false sharing、容易平行。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 9（Chapter 3, Part III）錄影逐字稿（YouTube 自動字幕，51:36）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§3.1.9–3.1.10 與附錄 C，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0）結果。*
