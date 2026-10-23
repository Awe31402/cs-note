# Operating Systems: A Linux Kernel-Oriented Approach 第三十三講完整筆記：Kernel 的記憶體配置（Chapter 6, Part V）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 33（Chapter 6, Part V：Kernel Memory Allocation）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（50:42，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§6.4（p.340–348）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
> **標記說明**：
> - `【口頭】`：教授在課堂上口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與教授回答
> - `【課本補充】`：課本有、但這堂課沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（`檔名:行號`），用來對照觀念
> - `【實測】`：在本機（Ubuntu、kernel 6.5.0、x86-64）實際執行的結果
> - `⚠️ 逐字稿缺字`：自動字幕在這裡掉字或聽不清，內容依上下文或課本補上；也用來標示需要修正之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 這堂課要做什麼](#0-這堂課要做什麼)
- [1. 為什麼 kernel 的配置不一樣](#1-為什麼-kernel-的配置不一樣)
- [2. Buddy allocator](#2-buddy-allocator)
  - [2.1 想法：在連續區域上疊一棵二元樹](#21-想法在連續區域上疊一棵二元樹)
  - [2.2 例子：配置 20 KB、64 KB，再釋放](#22-例子配置-20-kb64-kb再釋放)
  - [2.3 Order 與 `free_area`](#23-order-與-free_area)
  - [2.4 【原始碼】`free_area`、`MAX_ORDER`、migrate type](#24-原始碼free_areamax_ordermigrate-type)
  - [2.5 不用存父子指標：位元運算](#25-不用存父子指標位元運算)
  - [2.6 【原始碼】`__find_buddy_pfn`](#26-原始碼__find_buddy_pfn)
  - [2.7 【原始碼】配置：`__rmqueue_smallest` 與 `expand`](#27-原始碼配置__rmqueue_smallest-與-expand)
  - [2.8 【原始碼】釋放：`__free_one_page`](#28-原始碼釋放__free_one_page)
  - [2.9 【實測】模擬 buddy allocator](#29-實測模擬-buddy-allocator)
- [3. Slab allocator](#3-slab-allocator)
  - [3.1 物件池](#31-物件池)
  - [3.2 結構](#32-結構)
  - [3.3 per-CPU 的 array cache](#33-per-cpu-的-array-cache)
- [4. SLUB allocator](#4-slub-allocator)
  - [4.1 兩個簡化](#41-兩個簡化)
  - [4.2 【原始碼】`struct slab` 與 `kmem_cache_cpu`](#42-原始碼struct-slab-與-kmem_cache_cpu)
  - [4.3 【原始碼】freelist 存在空物件裡面](#43-原始碼freelist-存在空物件裡面)
  - [4.4 【原始碼】從位址找到 slab](#44-原始碼從位址找到-slab)
  - [4.5 ⚠️ 現況：哪個 allocator 是預設](#45-️-現況哪個-allocator-是預設)
  - [4.6 【實測】本機的 slab cache](#46-實測本機的-slab-cache)
- [5. 第 6 章結束](#5-第-6-章結束)
- [6. 一頁速查表](#6-一頁速查表)
- [7. 與後續講次的連結](#7-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

[00:33] kernel 的記憶體配置跟一般的虛擬記憶體配置不同，在某種意義上**更有挑戰性**，挑戰的種類也不一樣。

| allocator | 管什麼 | 單位 |
|---|---|---|
| **Buddy allocator** | 每個 zone 的實體記憶體（預設的配置器） | 2ᵏ 個連續 page |
| **Slab / SLUB** | 同一種 kernel 物件的物件池（`kmem_cache`） | 一個物件 |

[01:01]–[02:31] 第 30 講的 `struct zone` 裡有一個 `free_area[MAX_ORDER]` 陣列。教授說今天唯一要先記住的是：**每個 zone 有一個 buddy allocator 負責配置實體記憶體，它的核心資料結構就是這個陣列。**

---

## 1. 為什麼 kernel 的配置不一樣

[03:00] kernel 的虛擬記憶體當然可以用一般的分頁，但這裡關心的是 **direct-mapped 區域**（第 30 講）：虛擬位址 = 實體位址 + 固定偏移，所以**虛擬上連續就是實體上連續**。

[03:30] 很多資料結構（例如頁表）放在實體記憶體裡，要直接管理記憶體。**分頁的好處在這裡用不上**：分頁能透明地處理很多事，這裡則是直接管理。

[04:00] 上一次直接管理記憶體是第 29 講的 **base–limit**，效果不好：連續區域和洞交錯，洞很難管。

[04:32] 需要一個更好的配置器：沒有 base–limit 那麼有彈性，但**不浪費那麼多記憶體**，而且能把「填洞」的問題控制住。

【課本補充】課本：

- 使用者空間的問題幾乎都被虛擬記憶體和分頁解決了。kernel 的 direct-mapped 區域沒有分頁幫忙，好像回到了「管理一大塊連續記憶體」的老日子，又會看到外部碎片。
- 但另一方面，就算在分頁系統裡，也常常需要一大塊連續的實體位址（預取、I/O、DMA、folio、huge page）。
- 兩種情況是同一個問題：管理一大塊連續記憶體，又要避開洞和失控的外部碎片。只要記憶體存取有**多一點規律**，就能用比 base–limit 聰明的方法。

---

## 2. Buddy allocator

### 2.1 想法：在連續區域上疊一棵二元樹

[04:32]–[05:00] 有一塊 128 KB 的記憶體，在上面**疊一棵樹**。

[05:00] 假設最小單位是 1 KB，總共 128 個單位。

規則：

- 要配置 R，找出**大於等於 R 的最小 2 的次方**。
- 把大塊**一分為二**，一直分，直到剛好是那個大小。
- 被分開的兩半互為 **buddy（夥伴）**。

> 只要 R > 區塊大小 / 2，就停止分割，把這塊給它；否則再分。

### 2.2 例子：配置 20 KB、64 KB，再釋放

（課本 Figure 6.25、6.26）

[05:00]–[06:33] **配置 20 KB**：大於等於 20 的最小 2 的次方是 **32**。

```
                    128 KB
              ┌───────┴───────┐
            64 KB            64 KB（空）
          ┌───┴───┐
       32 KB     32 KB（空）
    （配給 20 KB）
```

- 128 分成兩個 64，左邊的 64 再分成兩個 32。
- 20 > 32 / 2 = 16，所以不再分，左邊的 32 KB 給它。
- [06:01] 如果要的是 16 KB，就會再把 32 分成兩個 16。

[06:33] **浪費 12 KB**。

[07:31] **配置 64 KB**：64 剛好是 2 的次方，右邊正好有一個空的 64 KB，直接給它。

[08:00]–[09:00] **釋放 20 KB**：

- 左邊的兩個 32 KB 現在**都空了**。
- 規則：**一個節點的兩個子節點不能同時都是空的**，遇到就把它們刪掉，讓父節點變回葉節點。
- 兩個 32 合併回 64。等於把相鄰的空塊（洞）**合併**起來。

[09:00]–[10:00] 那 12 KB 的浪費算內部還是外部碎片？教授說兩種說法都對：

- 它在配出去的區塊**裡面**被浪費：整個 32 KB 都給了 process，是 process 自己只用 20 KB，所以可以算**內部碎片**。
- 也可以說是外部的。教授：「不要糾結名詞。」（教授在 [06:33] 先說是外部碎片，後來才補充這段。）

[10:00] 關鍵：buddy allocator 能**透明地**處理小區塊和大區塊的請求。要小塊就一直分。

> **課本 Point 6.4.1**：buddy 系統在一個線性的 page 陣列上疊一棵（可能不平衡的）**二元樹**。每個節點是一組連續的 page（數量是 2 的次方），平分給左右子節點。配置只在**葉節點**上，大小是 2 的次方。同一個節點的兩個子節點不會同時是空的，否則刪掉它們、讓父節點成為葉節點。配置在一個葉節點上的記憶體一定**超過該節點容量的 50%**（否則會再分）。特例：只要 10 bytes 也得給一整個 page。

### 2.3 Order 與 `free_area`

[10:31]–[11:02] **order（階）**：

- 最小的單位（1 KB、在 kernel 裡是 **1 個 page**）是 **order 0**（2⁰）。
- 128 KB = 2⁷，是 **order 7**。64 KB 是 order 6。
- 同一層水平的節點都是同一個 order。

[11:31] **每個 order 一個 `struct free_area`**，裡面基本上是一條串列：這個 order 所有**目前空著**的節點。

[12:00] 但其實不只一條：**每種 migrate type 一條**。page 有不可移動、可移動、可回收等類型，每種類型像是有自己的 buddy allocator，全部融合在一個結構裡。

[13:03] 單位改成 page：order 3 的一個節點 = **8 個連續的 page**，串列裡存的是那 8 個 page 的**第一個** `struct page`，它再指向下一個空的 8-page 區塊……

[13:31]–[15:30] 例：order 3 那一層有好幾個 8-page 的節點：

- 完全空的：放進 order 3 的串列（存它的起始位址）。
- 部分被配出去的：**不**放進來，因為它沒有連續 8 個空 page。

[16:02] 重點：order 3 串列裡的任何一項，都代表**整整 8 個 page 是空的**。

課本 Figure 6.27：

```
 zone ── free_area[0] ──┬── 類型 0 的串列 ──▶ 空區塊 ──▶ 空區塊 ...
         free_area[1]   ├── 類型 1 的串列
         ...            └── 類型 2 的串列
         free_area[10]
```

### 2.4 【原始碼】`free_area`、`MAX_ORDER`、migrate type

`include/linux/mmzone.h:105-108`

```c
struct free_area {
	struct list_head	free_list[MIGRATE_TYPES];
	unsigned long		nr_free;
};
```

- `free_list[MIGRATE_TYPES]`：每種 migrate type 一條串列（第 14 講的 `list_head`）。
- `nr_free`：這個 order 有幾個空區塊。

`include/linux/mmzone.h:28-32`

```c
#define MAX_ORDER 11
...
#define MAX_ORDER_NR_PAGES (1 << (MAX_ORDER - 1))
```

- order 從 0 到 **10**（`MAX_ORDER − 1`）。最大的區塊是 2¹⁰ = 1024 個 page = **4 MB**。
- 第 30 講 `/proc/buddyinfo` 的 11 欄就是 order 0 到 10。

`include/linux/mmzone.h:42-62`（節錄）

```c
enum migratetype {
	MIGRATE_UNMOVABLE,
	MIGRATE_MOVABLE,
	MIGRATE_RECLAIMABLE,
	MIGRATE_PCPTYPES,	/* the number of types on the pcp lists */
	MIGRATE_HIGHATOMIC = MIGRATE_PCPTYPES,
#ifdef CONFIG_CMA
	...
	MIGRATE_CMA,
#endif
#ifdef CONFIG_MEMORY_ISOLATION
	MIGRATE_ISOLATE,	/* can't allocate from here */
#endif
```

| 類型 | 意思 |
|---|---|
| UNMOVABLE | 配出去就不能搬（例如 kernel 的資料結構，direct map 的位址被到處引用） |
| MOVABLE | 可以搬（使用者的 page：改頁表就好） |
| RECLAIMABLE | 可以回收（例如檔案的 page cache） |
| HIGHATOMIC | 保留給高優先、不能等的配置 |
| CMA | 連續記憶體配置器（給需要大塊連續記憶體的裝置） |
| ISOLATE | 隔離中，不能從這裡配 |

【課本補充】分開管理的理由：把不能動的 page 集中，可移動的 page 才能被搬到一起，湊出大塊連續的空間（第 30 講 ZONE_MOVABLE 也是同一個想法）。

### 2.5 不用存父子指標：位元運算

[16:30] 這棵樹**沒有**父子指標。因為是連續區域，父節點、子節點、兄弟節點都可以用**指標運算**算出來。

[17:02]–[22:02] 以 page 為單位、從 0 開始：

```
order 2:  [ 0 1 2 3 ]                [ 4 5 6 7 ]
order 1:  [ 0 1 ]  [ 2 3 ]           [ 4 5 ]  [ 6 7 ]
order 0:  [0] [1]  [2] [3]           [4] [5]  [6] [7]
```

| order | 左子節點的起點 | 右子節點的起點 |
|---|---|---|
| 0 | 0, 2, 4, 6…（偶數） | 1, 3, 5, 7…（奇數） |
| 1 | 0, 4, 8, 12… | 2, 6, 10, 14… |

[20:32] 一般化：order k 的區塊起點一定是 **n × 2ᵏ**。n 是偶數 → 左子節點；n 是奇數 → 右子節點。

[21:30] 父節點的起點 = **左子節點**的起點。

[22:31]–[28:01] 教授的捷徑：兩個 buddy 的起點 A、A′（order k），不知道誰左誰右：

- B = min(A, A′) = 左、C = max(A, A′) = 右，C = B + 2ᵏ。
- [25:14] 兩者的二進位**只差一個位元**：第 k 位（從 0 起算）。低 k 位都是 0（因為對齊 2ᵏ）。
- [26:00] 例：order 1 的 4 和 6：4 = `100`、6 = `110`，只差第 1 位。6 = 4 + 2¹。
- [27:01] 父節點 = B = **A & A′**（AND 把不同的那一位變成 0）。
- [28:01] 右子節點 = 父節點 + 2ᵏ（一次左移、一次加法）。

課本的寫法：

```
buddy：  A = B ⊕ 2^φ          （⊕ = XOR，翻轉第 φ 位）
父節點： min(A, B) = A & B
```

### 2.6 【原始碼】`__find_buddy_pfn`

kernel 的註解跟教授講的完全一樣：

`mm/internal.h:292-313`

```c
/*
 * Locate the struct page for both the matching buddy in our
 * pair (buddy1) and the combined O(n+1) page they form (page).
 *
 * 1) Any buddy B1 will have an order O twin B2 which satisfies
 * the following equation:
 *     B2 = B1 ^ (1 << O)
 * For example, if the starting buddy (buddy2) is #8 its order
 * 1 buddy is #10:
 *     B2 = 8 ^ (1 << 1) = 8 ^ 2 = 10
 *
 * 2) Any buddy B will have an order O+1 parent P which
 * satisfies the following equation:
 *     P = B & ~(1 << O)
 *
 * Assumption: *_mem_map is contiguous at least up to MAX_ORDER
 */
static inline unsigned long
__find_buddy_pfn(unsigned long page_pfn, unsigned int order)
{
	return page_pfn ^ (1 << order);
}
```

- **buddy = PFN XOR (1 << order)**。註解的例子：PFN 8、order 1 → 8 ^ 2 = 10。
- **父節點 = B & ~(1 << O)**：把第 O 位清掉。跟「A & A′」是同一件事（兩個 buddy 只差第 O 位）。
- 假設 `mem_map` 至少在 `MAX_ORDER` 範圍內連續：`struct page` 陣列連續，才能用 `page + (buddy_pfn − pfn)` 找到 buddy 的 `struct page`。

`mm/internal.h:329-340`（節錄）

```c
static inline struct page *find_buddy_page_pfn(struct page *page,
			unsigned long pfn, unsigned int order, unsigned long *buddy_pfn)
{
	unsigned long __buddy_pfn = __find_buddy_pfn(pfn, order);
	struct page *buddy;

	buddy = page + (__buddy_pfn - pfn);
	if (buddy_pfn)
		*buddy_pfn = __buddy_pfn;

	if (page_is_buddy(page, buddy, order))
		return buddy;
```

- `page + (__buddy_pfn − pfn)`：指標運算，自動乘上 `sizeof(struct page)`（教授一再強調的那件事）。
- `page_is_buddy`：buddy **必須是空的、而且是同一個 order**，才能合併；否則回傳 NULL。

### 2.7 【原始碼】配置：`__rmqueue_smallest` 與 `expand`

[28:30]–[31:02] 配置時，從要求的 order 開始**往上**找：

`mm/page_alloc.c:2554-2577`

```c
struct page *__rmqueue_smallest(struct zone *zone, unsigned int order,
						int migratetype)
{
	unsigned int current_order;
	struct free_area *area;
	struct page *page;

	/* Find a page of the appropriate size in the preferred list */
	for (current_order = order; current_order < MAX_ORDER; ++current_order) {
		area = &(zone->free_area[current_order]);
		page = get_page_from_free_area(area, migratetype);
		if (!page)
			continue;
		del_page_from_free_list(page, zone, current_order);
		expand(zone, page, order, current_order, migratetype);
		set_pcppage_migratetype(page, migratetype);
		...
		return page;
	}

	return NULL;
}
```

| 行 | 說明 |
|---|---|
| 2562 | [29:00] 從要求的 order 往上，直到 `MAX_ORDER` |
| 2563-2566 | [29:30] 拿這個 order、這種 migrate type 的空區塊；沒有就往上一層 |
| 2567 | [30:31] 找到了，從空串列拿掉 |
| 2568 | **`expand`**：區塊比需要的大，就切開 |
| 2573 | 回傳 |

[30:01] 例：要 20 個 page → 至少 order 5（32 個）。從 order 5 開始找，沒有就找 order 6……

[31:02] 教授：「把高 order 的區塊切開」這個邏輯，看不到，藏在另一個函式裡。就是 `expand`：

`mm/page_alloc.c:2340-2362`（節錄）

```c
static inline void expand(struct zone *zone, struct page *page,
	int low, int high, int migratetype)
{
	unsigned long size = 1 << high;

	while (high > low) {
		high--;
		size >>= 1;
		...
		add_to_free_list(&page[size], zone, high, migratetype);
		set_buddy_order(&page[size], high);
	}
}
```

- 從找到的 order（`high`）一層一層切到要的 order（`low`）。
- 每切一次，**右半邊**（`page[size]`）放進下一層的空串列，**左半邊**繼續切。
- 最後留下最左邊那塊給呼叫者。
- [30:31] 例：找到一個 64、需要 32 → 切成兩個 32，右邊的 32 放回 order 5 的串列，左邊的 32 給它。

### 2.8 【原始碼】釋放：`__free_one_page`

[31:32]–[36:02] 釋放時從自己的 order 往上，能合併就一直合併：

`mm/page_alloc.c:1113-1166`（節錄）

```c
	while (order < MAX_ORDER - 1) {
		...
		buddy = find_buddy_page_pfn(page, pfn, order, &buddy_pfn);
		if (!buddy)
			goto done_merging;
		...
		/*
		 * Our buddy is free or it is CONFIG_DEBUG_PAGEALLOC guard page,
		 * merge with it and move up one order.
		 */
		if (page_is_guard(buddy))
			clear_page_guard(zone, buddy, order, migratetype);
		else
			del_page_from_free_list(buddy, zone, order);
		combined_pfn = buddy_pfn & pfn;
		page = page + (combined_pfn - pfn);
		pfn = combined_pfn;
		order++;
	}

done_merging:
	set_buddy_order(page, order);
	...
	if (to_tail)
		add_to_free_list_tail(page, zone, order, migratetype);
	else
		add_to_free_list(page, zone, order, migratetype);
```

| 行 | 說明 |
|---|---|
| 1120 | [32:02] 找 buddy：PFN XOR (1 << order)（§2.6） |
| 1121-1122 | [35:32] buddy **不是空的** → 不能合併，跳到結尾 |
| 1124-1137（省略） | order 夠大時，不同 migrate type 的區塊不准合併 |
| 1146 | [33:01] buddy 是空的 → 把它從空串列**拿掉**（要合併了） |
| 1147 | [33:30] **父節點的 PFN = buddy_pfn & pfn**（§2.5） |
| 1148 | [34:02] `page` 移到父節點的第一個 page：自己是左子節點 → `combined_pfn − pfn = 0`，不動；自己是右子節點 → `= −2^order`，往回退。指標運算自動乘上 `sizeof(struct page)` |
| 1149-1150 | 換成父節點，order + 1，再試著跟**父節點的 buddy** 合併 |
| 1153-1154 | [36:02] 合併停了：記下這塊的 order |
| 1163-1166 | 放進那個 order 的空串列（`buddy_merge_likely`：如果它很可能很快又跟 buddy 合併，就放到串列**尾端**，盡量別先被配出去） |

【課本補充】課本：設 A < B 是 buddy，order φ，則 B = A + 2^φ，合併後起點是 A、大小 2^{φ+1}。`combined_pfn − pfn` = min(A, B) − A，只會是 0 或 −2^φ。課本請讀者自己手算一個小例子（§2.9 就是）。

### 2.9 【實測】模擬 buddy allocator

`buddy.py`：128 個 page（order 7），照 kernel 的規則（`expand` 留左半、XOR 找 buddy、AND 找父節點）寫一個模擬。每行印出各 order 的空串列（起始 PFN）：

```
start                        {7: [0]}
alloc 20 pages -> (0, 5) (wastes 12)
after alloc 20               {5: [32], 6: [64]}
alloc 64 pages -> (64, 6)
after alloc 64               {5: [32]}
alloc 3 pages  -> (32, 2)
after alloc 3                {2: [36], 3: [40], 4: [48]}
free the 20-page block       {2: [36], 3: [40], 4: [48], 5: [0]}
free the 3-page block        {6: [0]}
free the 64-page block       {7: [0]}
buddy of pfn 8 at order 1 = 10 ; parent of 8 and 10 = 8
```

| 步驟 | 發生什麼 |
|---|---|
| 配 20 個 page | 要 order 5（32 個）。從 order 7 切：128 → 64 + **64**（右半放 order 6）→ 32 + **32**（右半放 order 5）。給 PFN 0 開始的 32 個，**浪費 12 個** |
| 配 64 個 | order 6 剛好有一塊（PFN 64），直接給 |
| 配 3 個 | 要 order 2（4 個）。只剩 order 5 的 PFN 32：切成 16+16、8+8、4+4，右半依序放進 order 4（48）、3（40）、2（36），給 PFN 32 |
| 釋放 20 個那塊 | PFN 0、order 5，buddy = 0 ^ 32 = 32，**不是空的**（被 3-page 那塊用著）→ 不能合併 |
| 釋放 3 個那塊 | PFN 32、order 2：buddy 36 空 → 合併成 32（o3）；buddy 40 空 → 32（o4）；buddy 48 空 → 32（o5）；buddy 0 空 → **0（o6）**。**一路合併了 4 次** |
| 釋放 64 個那塊 | PFN 64、order 6，buddy = 0 空 → 合併回整塊 128 |

最後一行對照 kernel 註解：PFN 8、order 1 的 buddy 是 10；8 和 10 的父節點是 8 & 10 = 8。

（實際 kernel 裡，配置是以 2 的次方個 page 為單位的：`alloc_pages(order)` 要 20 個 page 就得要 order 5、拿到 32 個。如果只想要剛好 20 個，有 `alloc_pages_exact()`，它會把多出來的 page 立刻還回去。）

---

## 3. Slab allocator

### 3.1 物件池

[36:32] buddy allocator 很通用，能配置任何大小。但常常需要更專門的：**同一種型別的物件**，要**非常快**地拿到。

[37:01] 這就是**物件快取**或**物件池**，第 7 講看過的 **`kmem_cache`**。用 **slab allocator**，名詞有 slab、cache、chunk。

[37:01]–[37:30] 歷史：

- 舊的 **Solaris**（很老的一種 Unix）allocator 用 first fit，有點像 base–limit。已經過時。
- **slab** 很受歡迎，但正快速被 **SLUB** 取代：可擴展性好、效能好、也更簡單。

【課本補充】課本：slab allocator 常跟 buddy allocator 一起用：buddy 是**高層**的配置器，給 slab 一塊連續區域，slab 再自己管理，在裡面建物件池和相關的資料結構。

### 3.2 結構

課本 Figure 6.28：

```
 slab（一塊連續記憶體，例如 4 或 8 個 page）
 ┌───────┬───────┬───────┬───────┬───────┐
 │ 物件  │  空   │ 物件  │  空   │  空   │   s_mem 指向第一個物件
 └───────┴───────┴───────┴───────┴───────┘
   freelist：空物件的索引     slab_cache ──▶ kmem_cache（這種物件的池）
                                           ├── array_cache（每顆 CPU 一個）：最近釋放的物件
                                           ├── ctor：物件的初始化函式
                                           └── node[NUMA 節點] ──▶ kmem_cache_node
                                                                    ├── full 串列
                                                                    ├── partial 串列
                                                                    └── free 串列
```

[37:30]–[38:30] **slab** = 一塊**連續的實體記憶體**（在 direct-mapped 區域，所以虛擬上也連續），單位通常是 page。

[38:30] 例：一個物件 300 bytes → slab 裡切成一格一格 300 bytes。每格要嘛是**已配置的物件**，要嘛是**空的**。

[39:01] 用一個位元向量（或空物件索引的串列，概念一樣）記錄：1 = 已配置、0 = 空的。

[39:32] slab 指向它所屬的物件池 **`kmem_cache`**。教授說：前面很多地方都看過 `kmem_cache` 這個名字，只要有自訂的物件，就放進這種池。

[40:00] `kmem_cache` 管理**一種**物件的儲存：從 buddy allocator 拿一大塊，再自己組織。

[41:30]–[42:00] 三條 slab 串列（每個 NUMA node 各一組）：

| 串列 | 意思 |
|---|---|
| **full** | 所有物件都被配出去了，沒有空位 |
| **partial** | 有一些空位 |
| **free** | 一個物件都沒配出去 |

【課本補充】課本：

- 配置：先找一個 partial slab，在裡面找一個空物件，用使用者提供的初始化函式（建構子）初始化，就能用了。沒有 partial slab，就拿一個 free slab，配一個物件，它就變成 partial。
- 歸還：反過來。從物件的位址做一點指標運算，找到最近的 slab 邊界，就知道它屬於哪個 slab。full 的變 partial；partial 裡最後一個物件被歸還，就變 free。
- 批評：串列太多，slab 常常要從一條串列搬到另一條。

### 3.3 per-CPU 的 array cache

[40:00]–[41:00] `kmem_cache` 為**每顆 CPU** 保留一個小陣列 **array cache**：空物件的陣列。

- 有請求就從這裡拿一個給 CPU。
- [40:30] 物件被釋放時，**不**呼叫 `kfree`（kernel 版的 `free`），而是還給池；有空位就放進這個「最近釋放的物件」陣列。
- [41:00] 好處：下一個請求拿到的是剛釋放的物件，它還在 cache 裡（**cache 是熱的**）。
- 重複使用前要**重新初始化**：有一個函式指標（建構子）負責。

[42:33]–[44:01] 兩種策略：

1. 物件一進 array cache，就在 slab 裡**標成已使用**。
2. 物件真的被用時，才從 array cache 拿出、到 slab 裡標記。

slab 是**跨 CPU 共享**的，array cache 是每顆 CPU 私有的。教授（和 Linux）偏好第 1 種：物件一進入某顆 CPU 的 array cache，slab 就標成使用中，**其他 CPU 不會來搶**，之後這顆 CPU 想什麼時候用就什麼時候用。

【課本補充】課本：array cache 裡的物件被配置或釋放時，它所屬 slab 的狀態**也要**改，這是額外的負擔，SLUB 沒有這個問題。

[44:32]–[45:01] 總結：

- 從 buddy allocator 拿一大塊，切成固定大小的 slab，每個 slab 有每個物件的空間，記錄哪些已配置、哪些空著。
- 有 full、partial、free 三條串列。
- 常用的物件就用 `kmem_cache`：`kmalloc` / `kfree` 成本高，從池裡拿、還給池就好。教授說現代 kernel 裡 `kmem_cache` 越用越多。

---

## 4. SLUB allocator

### 4.1 兩個簡化

[45:31] **SLUB** 用的是同樣的 slab（空物件串列、使用中的物件數、指向 slab cache 的指標），差別在：

**簡化一：每顆 CPU 給一整個 slab，不是一個物件陣列**

[45:31]–[46:31]

- slab allocator 的 array cache 讓物件「離開」它的 slab，造成**多重擁有權**，管理變複雜。
- SLUB 改成：**每顆 CPU 分到一個 slab**。這個 slab 有自己的空物件串列，可以很快找到空物件，再用建構子初始化。
- 教授：**一致性帶來效能**。

**簡化二：沒有 full 和 free 的 slab 串列**

[46:31]–[49:30] 只留 **partial** 串列。

| 情況 | 怎麼辦 |
|---|---|
| **full 的 slab** | [47:01] 直接忘掉它。之後它的某個物件被釋放，還給池時，池發現這個 slab 不在 partial 串列裡；既然有物件被還回來，它之前一定是 full 的 → 把它加回 partial 串列。[47:30] 也可以看 slab 的空串列、或看物件的位址判斷 |
| **free（空）的 slab** | [48:02] 不用記。需要空間時（例如這顆 CPU 的 slab 滿了，[48:32] 要換一個新的），[49:03] 直接去跟 **buddy allocator** 要一塊，比維護一長串空 slab 有效率。slab 變空了，就還給 buddy allocator |

[49:30] 結論：SLUB 沒有 full 和空 slab 的串列（full 的可以推斷出來、空的直接跟 buddy 要），per-CPU 存一整個 slab 而不是物件陣列。所以 SLUB 正快速取代 slab。

課本 Figure 6.29：

```
 kmem_cache
 ├── cpu_slab ──▶ kmem_cache_cpu（每顆 CPU）
 │                 ├── freelist：空物件的指標
 │                 └── slab：目前在用的 slab
 ├── object_size、ctor
 └── node[NUMA] ──▶ kmem_cache_node ──▶ 只有 partial 串列
     1. 空的 slab 還給記憶體系統
     2. 忘掉 full 的 slab
```

### 4.2 【原始碼】`struct slab` 與 `kmem_cache_cpu`

`mm/slab.h:9-52`（節錄）

```c
struct slab {
	unsigned long __page_flags;

#if defined(CONFIG_SLAB)

	struct kmem_cache *slab_cache;
	union {
		struct {
			struct list_head slab_list;
			void *freelist;	/* array of free object indexes */
			void *s_mem;	/* first object */
		};
		struct rcu_head rcu_head;
	};
	unsigned int active;

#elif defined(CONFIG_SLUB)

	struct kmem_cache *slab_cache;
	union {
		struct {
			union {
				struct list_head slab_list;
#ifdef CONFIG_SLUB_CPU_PARTIAL
				struct {
					struct slab *next;
					int slabs;	/* Nr of slabs left */
				};
#endif
			};
			/* Double-word boundary */
			void *freelist;		/* first free object */
			union {
				unsigned long counters;
				struct {
					unsigned inuse:16;
					unsigned objects:15;
					unsigned frozen:1;
				};
			};
		};
		struct rcu_head rcu_head;
	};
	unsigned int __unused;
```

| | SLAB 版 | SLUB 版 |
|---|---|---|
| `slab_cache` | 所屬的 `kmem_cache` | 同 |
| `freelist` | **空物件索引的陣列** | **第一個空物件**（的指標，§4.3） |
| `s_mem` | 第一個物件 | 沒有 |
| `active` / `inuse` | 使用中的物件數 | `inuse`（16 位元）、總數 `objects`（15 位元） |
| `frozen` | — | 1 位元：這個 slab 被某顆 CPU 拿走了（`kmem_cache_cpu->slab`） |

- 開頭是 `__page_flags`：**`struct slab` 其實是疊在 `struct page`（folio）上的**，不是另外配置的。slab 就是一種 folio，用同一塊 metadata 換個型別來看（第 30 講）。
- `freelist` 和 `counters` 放在一起、「雙字邊界」（Double-word boundary）：SLUB 可以用一道 **double-width CAS**（`cmpxchg16b`）同時改兩者，不用拿鎖（第 18 講的 CAS）。
- `CONFIG_SLUB_CPU_PARTIAL`：SLUB 其實**每顆 CPU 也有一條小的 partial 串列**（`next`、`slabs`），不只一個 slab。

`include/linux/slub_def.h:49-60`

```c
struct kmem_cache_cpu {
	void **freelist;	/* Pointer to next available object */
	unsigned long tid;	/* Globally unique transaction id */
	struct slab *slab;	/* The slab from which we are allocating */
#ifdef CONFIG_SLUB_CPU_PARTIAL
	struct slab *partial;	/* Partially allocated frozen slabs */
#endif
	local_lock_t lock;	/* Protects the fields above */
	...
};
```

| 欄位 | 說明 |
|---|---|
| `freelist` | 這顆 CPU 下一個可用的物件 |
| `tid` | 交易 ID：快速路徑用 `cmpxchg` 同時比對 `freelist` 和 `tid`，確保中間沒被搶占、沒換 CPU |
| `slab` | 這顆 CPU 正在配置的 slab（教授說的「每顆 CPU 一個 slab」） |
| `partial` | 這顆 CPU 自己的 partial slab 串列 |

### 4.3 【原始碼】freelist 存在空物件裡面

SLUB 最精巧的地方：**空物件串列不另外存，就存在空物件自己身上**。

`mm/slub.c:392-396`

```c
static inline void *get_freepointer(struct kmem_cache *s, void *object)
{
	object = kasan_reset_tag(object);
	return freelist_dereference(s, object + s->offset);
}
```

`mm/slub.c:430-440`

```c
static inline void set_freepointer(struct kmem_cache *s, void *object, void *fp)
{
	unsigned long freeptr_addr = (unsigned long)object + s->offset;

#ifdef CONFIG_SLAB_FREELIST_HARDENED
	BUG_ON(object == fp); /* naive detection of double free or corruption */
#endif

	freeptr_addr = (unsigned long)kasan_reset_tag((void *)freeptr_addr);
	*(void **)freeptr_addr = freelist_ptr(s, fp, freeptr_addr);
}
```

```
 slab:  [ 物件 0 ][ 空 → 物件 3 ][ 物件 2 ][ 空 → 物件 4 ][ 空 → NULL ]
                        ↑
 kmem_cache_cpu->freelist
```

- 空物件反正沒在用，就在它裡面偏移 `s->offset` 的地方，存「**下一個空物件**的位址」。
- 配置 = 拿 `freelist` 指的物件，`freelist` 改成它存的「下一個」。
- 釋放 = 在物件裡寫入目前的 `freelist`，`freelist` 改成這個物件。
- 就是一個**用空物件串起來的 stack**，O(1)，完全不用額外空間。
- `CONFIG_SLAB_FREELIST_HARDENED`：存的指標會先跟一個隨機值 XOR（`freelist_ptr`），讓攻擊者很難偽造；`object == fp` 是在偵測**重複釋放**。

### 4.4 【原始碼】從位址找到 slab

[47:30] 教授和課本說：從物件位址可以找到它的 slab，因為「slab 放在專用的記憶體區域」，算「最近的 slab 邊界」就好。實際做法：

`mm/slab.h:183-191`

```c
static inline struct slab *virt_to_slab(const void *addr)
{
	struct folio *folio = virt_to_folio(addr);

	if (!folio_test_slab(folio))
		return NULL;

	return folio_slab(folio);
}
```

- 位址 → 它所在的 page / folio（direct-mapped 區域：`__pa` 減掉偏移 → PFN → `vmemmap + pfn`，第 30 講）。
- 看 folio 有沒有 **slab 旗標**。
- 有的話，`struct slab` 就疊在這個 folio 的 metadata 上，直接轉型。
- 不需要專用的記憶體區域：slab 就是從 buddy 拿來的普通 page，靠 `struct page` 上的旗標認出來。`kfree(ptr)` 不用告訴它大小，也是靠這招找到所屬的 `kmem_cache`。

⚠️ 「空的 slab 直接還給 buddy」也稍微簡化了：

`mm/slub.c:2544-2547`（`deactivate_slab` 裡）

```c
	if (!new.inuse && n->nr_partial >= s->min_partial) {
		mode = M_FREE;
	} else if (new.freelist) {
		mode = M_PARTIAL;
```

空了（`inuse == 0`），而且這個 node 的 partial slab **已經夠多**（≥ `min_partial`）才還給 buddy；不夠多就留在 partial 串列，避免剛還回去又馬上要配。

### 4.5 ⚠️ 現況：哪個 allocator 是預設

課本說「到 v6.2，slab allocator 最受歡迎，已經讓更早的 slob 過時」。實際情況：

- **SLUB** 從 Linux 2.6.23（2007）起就是**預設**。
- **SLOB**（給很小的嵌入式系統）在 v6.2 被標成棄用，**v6.4 移除**。
- **SLAB** 在 v6.5 被標成棄用，**v6.8 移除**。現在只剩 SLUB。

教授說的「SLUB 正快速取代 slab」是對的方向。本機：

```
CONFIG_SLUB=y
CONFIG_SLUB_CPU_PARTIAL=y
```

### 4.6 【實測】本機的 slab cache

```
$ head -3 /proc/slabinfo
head: cannot open '/proc/slabinfo' for reading: Permission denied
$ ls /sys/kernel/slab | wc -l
374
$ ls -l /sys/kernel/slab | grep -c '^l'
163
$ ls -l /sys/kernel/slab | grep '^l' | head -4
lrwxrwxrwx 1 root root 0 Oct 23 04:15 Acpi-Namespace -> :0000048
lrwxrwxrwx 1 root root 0 Oct 23 04:15 Acpi-Operand -> :0000072
lrwxrwxrwx 1 root root 0 Oct 23 04:15 Acpi-Parse -> :0000080
lrwxrwxrwx 1 root root 0 Oct 23 04:15 Acpi-ParseExt -> :0000104
```

- `/proc/slabinfo` 和各 cache 的詳細屬性（`object_size`、`objs_per_slab`、`order`）都要 root 才能讀，這裡讀不到，待補。
- 有 374 個項目，其中 **163 個是符號連結**：例如 `Acpi-Namespace -> :0000048`。SLUB 會把**物件大小相同、屬性相容**的 cache **合併**（slab merging）成一個，名稱 `:0000048` 表示 48 bytes 的共用 cache，原本的名字只是指向它的別名。這也是 SLUB「比較簡單、比較省」的一個表現：少很多個獨立的 slab。

---

## 5. 第 6 章結束

[49:30]–[50:02] 參考文獻：Belady 原始的論文，以及「FIFO 異常可以無上限」的那篇（第 29 講）。

[50:02] 第 6 章到此結束。第 7 章是 **I/O 裝置、儲存裝置，以及 kernel 的 I/O 與儲存子系統**。（本課程播放清單到這一講為止。）

第 6 章的路線：

| 講次 | 主題 |
|---|---|
| 第 29 講 | 傳統啟發式：base–limit、OPT、LRU、Clock、FIFO / Belady、working set |
| 第 30 講 | kernel 的記憶體架構：地圖、五層頁表、page / folio、TLB、zone |
| 第 31 講 | reverse mapping：anon_vma、AVC、index |
| 第 32 講 | MGLRU、thrashing |
| 第 33 講 | buddy、slab、SLUB |

---

## 6. 一頁速查表

### 6.1 Buddy allocator

| 項目 | 內容 |
|---|---|
| 結構 | `zone->free_area[MAX_ORDER]`，每個 order 一組串列，每種 migrate type 一條 |
| order | 0..10；order k = 2ᵏ 個連續 page；最大 4 MB |
| buddy | `pfn ^ (1 << order)` |
| 父節點 | `pfn & buddy_pfn`（= `pfn & ~(1 << order)`） |
| 配置 | 從要的 order 往上找；`expand` 切開，右半放回空串列，留左半 |
| 釋放 | buddy 空就合併、order + 1，一直到 buddy 不空 |
| 浪費 | 配置量 > 區塊的 50% |

### 6.2 Slab vs SLUB

| | SLAB | SLUB |
|---|---|---|
| per-CPU | 最近釋放的物件陣列（array cache） | 一整個 slab（+ 小的 partial 串列） |
| slab 串列 | full、partial、free | 只有 partial |
| full 的 slab | 記在串列 | 忘掉；有物件還回來時再加回 partial |
| 空的 slab | 記在串列 | 夠多就還給 buddy |
| freelist | 空物件索引的陣列 | 存在空物件裡面的指標鏈 |
| 現況 | v6.8 移除 | 唯一的 allocator |

### 6.3 本講看過的原始碼位置

| 位置 | 內容 |
|---|---|
| `include/linux/mmzone.h:105-108` | `struct free_area` |
| `include/linux/mmzone.h:28-32` | `MAX_ORDER` = 11 |
| `include/linux/mmzone.h:42-62` | `enum migratetype` |
| `mm/internal.h:292-313` | `__find_buddy_pfn` 與 buddy / 父節點公式 |
| `mm/internal.h:329-340` | `find_buddy_page_pfn` |
| `mm/page_alloc.c:2554-2577` | `__rmqueue_smallest` |
| `mm/page_alloc.c:2340-2362` | `expand` |
| `mm/page_alloc.c:1092-1166` | `__free_one_page` |
| `mm/slab.h:9-52` | `struct slab`（SLAB / SLUB 兩版） |
| `include/linux/slub_def.h:49-60` | `struct kmem_cache_cpu` |
| `mm/slub.c:392-396, 430-440` | `get_freepointer`、`set_freepointer` |
| `mm/slab.h:183-191` | `virt_to_slab` |
| `mm/slub.c:2544-2547` | 空 slab 何時還給 buddy |

---

## 7. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **I/O、儲存、page cache** | 課本第 7 章（本播放清單未涵蓋，待補） |
| **另一門課的即時系統記憶體配置** | Real Time Systems（若有相關講次，見該課筆記） |
| **`kmem_cache`、物件池** | 回顧第 7 講 |
| **CAS、per-CPU、指標運算** | 回顧第 18、22、30 講 |

（本講是課程 A 播放清單的最後一講。）

---

## 附：本講一句話總結

> **kernel 在 direct-mapped 區域裡直接管理實體記憶體，沒有分頁幫忙，所以用 buddy allocator：每個 zone 有 `free_area[0..10]`，每個 order 一條（依 migrate type 再分）空區塊串列，order k 是 2ᵏ 個連續 page；配置時從需要的 order 往上找，`expand` 把大塊一路對半切、右半放回串列，所以配到的量一定超過區塊的一半（20 個 page 會拿到 32 個）；釋放時用 `pfn ^ (1 << order)` 找 buddy、用 `pfn & buddy_pfn` 找父節點，buddy 也空就一路合併上去，整棵樹完全不用存父子指標（實測釋放一個 4-page 區塊連續合併了 4 次回到 64 頁）。同一種物件的大量配置則交給 slab 類的物件池（`kmem_cache`）：舊的 SLAB 有 full / partial / free 三條串列加上每顆 CPU 的最近釋放物件陣列，現在唯一的 SLUB 則讓每顆 CPU 直接拿一個 slab、只記 partial slab、full 的忘掉、空的還給 buddy，空物件串列就存在空物件裡面，`struct slab` 疊在 folio 上、從位址就能找回所屬的 slab，還會把同樣大小的 cache 合併（本機 374 個裡有 163 個是別名）。第 6 章與課程 A 到此結束。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 33（Chapter 6, Part V）錄影逐字稿（YouTube 自動字幕，50:42）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§6.4，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
