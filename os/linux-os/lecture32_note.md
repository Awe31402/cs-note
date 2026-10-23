# Operating Systems: A Linux Kernel-Oriented Approach 第三十二講完整筆記：MGLRU 與 Thrashing（Chapter 6, Part IV）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 32（Chapter 6, Part IV：MGLRU Algorithm）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（1:00:06，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§6.3.2–6.3.3（p.324–340）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
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
- [1. 世代的想法](#1-世代的想法)
- [2. 資料結構](#2-資料結構)
  - [2.1 `lruvec`](#21-lruvec)
  - [2.2 `lru_gen_struct`：三維的串列陣列](#22-lru_gen_struct三維的串列陣列)
  - [2.3 `lru_gen_mm_state`：頁表走訪的狀態](#23-lru_gen_mm_state頁表走訪的狀態)
- [3. 追蹤存取：Accessed 位元](#3-追蹤存取accessed-位元)
- [4. 縮小記憶體用量：回收與驅逐](#4-縮小記憶體用量回收與驅逐)
- [5. 老化（aging）](#5-老化aging)
  - [5.1 什麼時候該老化](#51-什麼時候該老化)
  - [5.2 【原始碼】`should_run_aging`](#52-原始碼should_run_aging)
  - [5.3 老化的過程：走頁表 + Bloom filter](#53-老化的過程走頁表--bloom-filter)
  - [5.4 【原始碼】kernel 的 Bloom filter](#54-原始碼kernel-的-bloom-filter)
- [6. 驅逐（eviction）](#6-驅逐eviction)
  - [6.1 先選：踢匿名 page 還是檔案 page](#61-先選踢匿名-page-還是檔案-page)
  - [6.2 【原始碼】`isolate_folios` 的第一步](#62-原始碼isolate_folios-的第一步)
  - [6.3 控制理論：refault 率除以 gain](#63-控制理論refault-率除以-gain)
  - [6.4 【原始碼】`positive_ctrl_err` 與 `get_type_to_scan`](#64-原始碼positive_ctrl_err-與-get_type_to_scan)
  - [6.5 Tier：世代裡再分層](#65-tier世代裡再分層)
  - [6.6 掃描 folio、真正驅逐](#66-掃描-folio真正驅逐)
  - [6.7 Look around：順便看看鄰居](#67-look-around順便看看鄰居)
- [7. Thrashing](#7-thrashing)
  - [7.1 min_ttl：至少留 N 毫秒](#71-min_ttl至少留-n-毫秒)
  - [7.2 【原始碼】留不住就叫 OOM killer](#72-原始碼留不住就叫-oom-killer)
  - [7.3 可調的參數](#73-可調的參數)
- [8. 【實測】本機的 MGLRU](#8-實測本機的-mglru)
- [9. 一頁速查表](#9-一頁速查表)
- [10. 與後續講次的連結](#10-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

[00:31] **MGLRU**（Multi-Generational LRU，多世代 LRU）：Linux 現在用的頁面置換演算法（v6.1 加入）。

[01:02] 任何頁面管理演算法都有兩個部分：

| 部分 | 意思 |
|---|---|
| **被動（reactive）** | 要放新 page 但沒空間了，**當下**踢一個到 swap |
| **主動（proactive）** | **定期**掃描記憶體裡的 page，把不常用的推出去 |

[01:30] MGLRU 兩者都有，**主要靠主動**。

【課本補充】課本：MGLRU 是第 29 講 **WS-Clock 第二次機會**演算法的精緻版。特色：依最後存取的新舊把 page 分成**世代**；被存取的 page 能很快升到最新世代；在背景回收最舊世代的 page；很聰明地老化；適合大工作負載，和 folio 整合得很好。

---

## 1. 世代的想法

[01:30] 把一群 page 分到一個**世代（generation）**。（之後還有**子世代**，叫 tier。）

[02:01]

- **世代號碼越大，越新**（最近被存取過）。最大的世代 = 最近存取的。
- 最小的序號（最早的世代）= 最久沒被存取的。

[02:31] 維護一個範圍：

```
 min_seq                                          max_seq
   │◀──────────── 存活的世代 ─────────────────────▶│
   最舊（踢它們）                                    最新
```

[02:31]–[03:01] 關鍵：序號**只增不減**。

- 某個 page 在世代 G，什麼都不做。
- `max_seq` 一直增加 → G 和 `max_seq` 的距離越來越大 → 這個 page **自動變老**。
- 最後 `min_seq` 也增加到 G：它成了最舊的世代，成為驅逐對象。

教授：「就算我什麼都不做、待在原地，我也會一直變老。」

【課本補充】課本說這是一個「很妙的技巧：不用動任何 page 的資料就讓所有 page 變老」。重要的只是 page 的序號和 `max_seq` 的**相對**距離：不去減少前者，改成增加後者。

[03:32] 好的置換演算法要：

- 利用時間和空間局部性。
- 主動做決定，需要時就有空間。
- 啟發式要**簡單**（容易解釋、理解、驗證），但整體設定可以複雜。
- 教授再次強調：**維護對的資料和資料結構**最重要。

---

## 2. 資料結構

### 2.1 `lruvec`

[04:02] **`lruvec`**：存 LRU 的所有資訊。教授說「每個 zone 一個」，第 30 講的 `pglist_data` 有一個 `__lruvec`。

[04:30] 整個 lruvec 記兩個計數：匿名 page 和檔案 page 的 **refault（重新缺頁）**次數。

> **Refault**：一個 page **被踢出去之後，很快又被存取**。這顯然是**錯誤**：以為它不會再被用，結果馬上又用了。

[05:02] 很多統計都是匿名和檔案**分開**記的。

`include/linux/mmzone.h:508-530`

```c
struct lruvec {
	struct list_head		lists[NR_LRU_LISTS];
	/* per lruvec lru_lock for memcg */
	spinlock_t			lru_lock;
	/*
	 * These track the cost of reclaiming one LRU - file or anon -
	 * over the other. As the observed cost of reclaiming one LRU
	 * increases, the reclaim scan balance tips toward the other.
	 */
	unsigned long			anon_cost;
	unsigned long			file_cost;
	/* Non-resident age, driven by LRU movement */
	atomic_long_t			nonresident_age;
	/* Refaults at the time of last reclaim cycle */
	unsigned long			refaults[ANON_AND_FILE];
	/* Various lruvec state flags (enum lruvec_flags) */
	unsigned long			flags;
#ifdef CONFIG_LRU_GEN
	/* evictable pages divided into generations */
	struct lru_gen_struct		lrugen;
	/* to concurrently iterate lru_gen_mm_list */
	struct lru_gen_mm_state		mm_state;
#endif
```

| 欄位 | 說明 |
|---|---|
| `lists[NR_LRU_LISTS]` | **傳統的** active / inactive 串列（第 29 講 §6.6）。沒開 MGLRU 時用 |
| `lru_lock` | 保護這些串列的 spinlock |
| `refaults[ANON_AND_FILE]` | 匿名、檔案的 refault 數 |
| `lrugen` | [05:02] MGLRU 的世代資訊 |
| `mm_state` | [05:31] **主動走訪頁表**的狀態 |

⚠️ lruvec 實際上是**每個 (memory cgroup, NUMA node)** 一個，不是每個 zone 一個：`pglist_data` 裡的 `__lruvec` 是沒有 cgroup 時用的；有 memcg 時每個 cgroup 在每個 node 都有自己的 lruvec。zone 則是 `lrugen->lists` 的其中一個維度（§2.2）。

### 2.2 `lru_gen_struct`：三維的串列陣列

`include/linux/mmzone.h:407-429`

```c
struct lru_gen_struct {
	/* the aging increments the youngest generation number */
	unsigned long max_seq;
	/* the eviction increments the oldest generation numbers */
	unsigned long min_seq[ANON_AND_FILE];
	/* the birth time of each generation in jiffies */
	unsigned long timestamps[MAX_NR_GENS];
	/* the multi-gen LRU lists, lazily sorted on eviction */
	struct list_head lists[MAX_NR_GENS][ANON_AND_FILE][MAX_NR_ZONES];
	/* the multi-gen LRU sizes, eventually consistent */
	long nr_pages[MAX_NR_GENS][ANON_AND_FILE][MAX_NR_ZONES];
	/* the exponential moving average of refaulted */
	unsigned long avg_refaulted[ANON_AND_FILE][MAX_NR_TIERS];
	/* the exponential moving average of evicted+protected */
	unsigned long avg_total[ANON_AND_FILE][MAX_NR_TIERS];
	/* the first tier doesn't need protection, hence the minus one */
	unsigned long protected[NR_HIST_GENS][ANON_AND_FILE][MAX_NR_TIERS - 1];
	/* can be modified without holding the LRU lock */
	atomic_long_t evicted[NR_HIST_GENS][ANON_AND_FILE][MAX_NR_TIERS];
	atomic_long_t refaulted[NR_HIST_GENS][ANON_AND_FILE][MAX_NR_TIERS];
	/* whether the multi-gen LRU is enabled */
	bool enabled;
};
```

| 欄位 | 說明 |
|---|---|
| `max_seq` | [05:31] 最大序號（最新的世代）。註解：**老化**會增加它 |
| `min_seq[2]` | [06:00] 最小序號，**匿名和檔案分開**。註解：**驅逐**會增加它 |
| `timestamps[]` | [06:00] 每個世代**建立的時間**。[06:31] 不想太躁進：剛進來的不要馬上踢、世代號碼也不要增加太快。教授說這是演算法的**煞車** |
| `lists[gen][type][zone]` | [06:31] page（folio）的串列。[07:00] **每個世代 × 每種類型（匿名 / 檔案）× 每個 zone** 一條。[07:32] 一個三維陣列，每格是一條串列。要踢「檔案、世代 5」的 page，就走那一條 |
| `nr_pages[...]` | 每條串列有幾個 page（§5.2 用來算比例） |
| `avg_refaulted`, `avg_total`, `evicted`, `refaulted`, `protected` | 每個 tier 的 refault 和驅逐統計（§6.3、§6.5） |

`include/linux/mmzone.h:332-338`

```c
 * MAX_NR_GENS is set to 4 so that the multi-gen LRU can support twice the
 * number of categories of the active/inactive LRU when keeping track of
 * accesses through page tables. This requires order_base_2(MAX_NR_GENS+1) bits
 * in folio->flags.
 */
#define MIN_NR_GENS		2U
#define MAX_NR_GENS		4U
```

- 最多 **4 個**世代：「是傳統 active/inactive（2 類）的兩倍」。
- 最少 **2 個**。
- folio 的世代號碼存在 `folio->flags` 的幾個位元裡（4 + 1 = 5 種值，要 3 位元），不另外加欄位。

### 2.3 `lru_gen_mm_state`：頁表走訪的狀態

[08:00] 要主動走過一個 zone、找能清掉的東西、送到 swap。教授稱之為 **zone shrinking（縮小 zone）**。

`include/linux/mmzone.h:444-459`

```c
struct lru_gen_mm_state {
	/* set to max_seq after each iteration */
	unsigned long seq;
	/* where the current iteration continues (inclusive) */
	struct list_head *head;
	/* where the last iteration ended (exclusive) */
	struct list_head *tail;
	/* to wait for the last page table walker to finish */
	struct wait_queue_head wait;
	/* Bloom filters flip after each iteration */
	unsigned long *filters[NR_BLOOM_FILTERS];
	/* the mm stats for debugging */
	unsigned long stats[NR_HIST_GENS][NR_MM_STATS];
	/* the number of concurrent page table walkers */
	int nr_walkers;
};
```

| 欄位 | 說明 |
|---|---|
| `seq` | [08:00] 目前在走的序號 |
| `head`, `tail` | [08:31] 有一條所有 `mm_struct` 的串列（第 9 講：存 maple tree、頁表的那個結構）。`tail` = 已經處理完的；`head` = 下一個要處理的 |
| `filters[2]` | [09:01] 一組 **Bloom filter**（§5.4）。教授：「不知道要做什麼用，但我們知道 Bloom filter 是什麼。」 |
| `nr_walkers` | [09:31] 同時有幾個走訪者（可以有好幾個 thread 一起走） |

---

## 3. 追蹤存取：Accessed 位元

[09:31] 第 29 講的 WS-Clock 第一件事就是**追蹤存取**。kernel 的做法類似，只是改叫 **accessed / not accessed**（不是 used / unused）。

課本 Listing 6.15（`arch/x86/include/asm/pgtable.h`）：

```c
pte_t pte_mkold(pte_t pte)
{
    return pte_clear_flags(pte, _PAGE_ACCESSED);
}
```

[10:30] `pte_mkold`：把 PTE 的 Accessed 旗標**清掉**。有 accessed 的 page 是「new」（Linux 叫 **young**），清掉後變成「old」。反方向是 `pte_mkyoung`。

[11:00]–[11:30] 走訪者走過頁表時，看到 young 的就把它變成 old。

[11:30]–[13:00] 之後再被存取，怎麼知道？依硬體而定：

| 方式 | 誰 |
|---|---|
| 故意觸發軟缺頁，在處理時 old → young，記錄下來 | 第 29 講的 WS-Clock 做法。[13:00] 教授說 **RISC-V** 這類架構需要 |
| **硬體直接在 PTE 設 Accessed 位元**，不中斷 | [12:30] **x86** |

（第 29 講 §6.3 看過 kernel 的 `arch_has_hw_pte_young()`：x86 回傳 true。）

[13:00]–[14:00] 原理：第一次掃描看到 accessed → 改成 not accessed。第二次掃描如果**還是** not accessed → 很久沒被存取了 → 驅逐對象。MGLRU 更細：有世代，沒被存取的 page **世代不升**，最後世代低到被整批踢掉。

> **課本 Point 6.3.3**：page 最近被存取過、Accessed 位元是 1，叫 **young**；否則叫 **old**。

【課本補充】課本說「到 2024 年，有些 x86 處理器能自動設這個旗標」。實際上 x86 從 386 起 PTE 就有硬體設定的 Accessed 位元了。

---

## 4. 縮小記憶體用量：回收與驅逐

[14:33] **kswapd**：一個定期跑的 kernel daemon，工作是**老化** page 並**回收**它們。

- 老化 = 增加世代號碼，讓 page 和 `max_seq` 的距離變大，[15:00] 最後變成最舊、被踢出去。
- 也可以**按需**：要配置 page 但空間不夠，就縮小（把 page 丟到 swap）。

[15:00]–[16:01] **回收（reclamation）**和**驅逐（eviction）**不一樣：

| | 意思 | 例子 |
|---|---|---|
| 驅逐 | 從記憶體寫到 swap | 一般的匿名 page |
| 回收 | 把專門用途的 page 還給**一般用途** | [15:33] 檔案緩衝區：為某些檔案保留了 100 個 page，縮成 80 個，20 個還到共用池，之後再慢慢老化、被踢 |

[16:01] 最後真的要踢時，呼叫 **`evict_folios`**。

【課本補充】課本：

- kernel 隨時知道每個 zone 有多少 page 在記憶體裡（**記憶體壓力**）。
- **被動**的方式：`kswapd` 定期跑；**主動**的方式：其他 kernel 子系統覺得 page 太多時直接呼叫 `evict_folios`。（課本的「被動 / 主動」跟教授開頭說的 reactive / proactive 意思剛好反過來，注意分辨。）
- 回收不一定要驅逐：kernel 自己的 page 緩衝區可以直接放掉；檔案對應的 page 可以縮小對應的範圍。

---

## 5. 老化（aging）

### 5.1 什麼時候該老化

[16:30] 老化 = 增加 `max_seq`；把最小世代清空後再增加 `min_seq`。**就像一個滑動視窗**往前移。

[17:01]–[18:00] 世代數 = `max_seq − min_seq + 1`。

- 太少（< `MIN_NR_GENS`）→ 增加 `max_seq`。
- 【課本】也要有上限 `MAX_NR_GENS`：太多世代，資訊太細，用處不大。

[18:31]–[21:30] 理想是維持 **`MIN_NR_GENS + 1`** 個世代。所以每個世代理想上有 **total / (MIN_NR_GENS + 1)** 個 page（λ）。設兩個門檻：

| 條件 | 意思 | 動作 |
|---|---|---|
| 最新世代的 page 數 > total / MIN_NR_GENS | [20:01] 「年輕」的太多（門檻比 λ 稍大） | 老化 |
| 最舊世代的 page 數 < total / (MIN_NR_GENS + 2) | [20:31] 「超老」的太少（門檻比 λ 稍小）：其他 page 擠在太少的世代裡 → 要攤開 | 老化 |

[21:30] 教授：什麼時候老化？**超年輕的太多**，或**超老的太少**（表示其他 page 擠在太少的世代裡）。

### 5.2 【原始碼】`should_run_aging`

`mm/vmscan.c:4468-4523`（節錄）

```c
static bool should_run_aging(struct lruvec *lruvec, unsigned long max_seq, unsigned long *min_seq,
			     struct scan_control *sc, bool can_swap, unsigned long *nr_to_scan)
{
	int gen, type, zone;
	unsigned long old = 0;
	unsigned long young = 0;
	unsigned long total = 0;
	...
	for (type = !can_swap; type < ANON_AND_FILE; type++) {
		unsigned long seq;

		for (seq = min_seq[type]; seq <= max_seq; seq++) {
			unsigned long size = 0;

			gen = lru_gen_from_seq(seq);

			for (zone = 0; zone < MAX_NR_ZONES; zone++)
				size += max(READ_ONCE(lrugen->nr_pages[gen][type][zone]), 0L);

			total += size;
			if (seq == max_seq)
				young += size;
			else if (seq + MIN_NR_GENS == max_seq)
				old += size;
		}
	}
	...
	/*
	 * The aging tries to be lazy to reduce the overhead, while the eviction
	 * stalls when the number of generations reaches MIN_NR_GENS. Hence, the
	 * ideal number of generations is MIN_NR_GENS+1.
	 */
	if (min_seq[!can_swap] + MIN_NR_GENS > max_seq)
		return true;
	if (min_seq[!can_swap] + MIN_NR_GENS < max_seq)
		return false;

	/*
	 * It's also ideal to spread pages out evenly, i.e., 1/(MIN_NR_GENS+1)
	 * of the total number of pages for each generation. A reasonable range
	 * for this average portion is [1/MIN_NR_GENS, 1/(MIN_NR_GENS+2)]. The
	 * aging cares about the upper bound of hot pages, while the eviction
	 * cares about the lower bound of cold pages.
	 */
	if (young * MIN_NR_GENS > total)
		return true;
	if (old * (MIN_NR_GENS + 2) < total)
		return true;

	return false;
}
```

| 部分 | 說明 |
|---|---|
| 三層迴圈 | 走過每種類型、每個存活的世代、每個 zone，加總 `nr_pages`：`total`、最新世代的 `young`、最舊世代的 `old` |
| `!can_swap` | 不能 swap（沒有 swap 空間）時，只看檔案 page（`type` 從 1 開始） |
| 第一個 if | 世代數 < MIN_NR_GENS + 1 → **一定要**老化 |
| 第二個 if | 世代數 > MIN_NR_GENS + 1 → **不用** |
| 註解 | 「老化盡量懶惰以降低成本，而驅逐在世代數降到 MIN_NR_GENS 時會卡住，所以理想世代數是 MIN_NR_GENS+1」 |
| `young * MIN_NR_GENS > total` | 年輕的比 1/MIN_NR_GENS 多 → 老化 |
| `old * (MIN_NR_GENS + 2) < total` | 老的比 1/(MIN_NR_GENS+2) 少 → 老化 |

註解的最後一句解釋了兩個門檻為什麼不對稱：「**老化在意熱 page 的上限，驅逐在意冷 page 的下限**。」

【課本補充】課本 Trivia 6.3.1 的解釋：老化會增加 `max_seq`（便宜），之後還要掃描、把用過的 page 標成沒用過、增加驅逐，不想太常做。所以年輕 page 稍微超過理想值沒關係（乘 MIN_NR_GENS）；老 page 可以少到 total / (MIN_NR_GENS+2)，因為老化會讓這些老 page 被驅逐，也想減少不必要的驅逐。（課本注意：程式裡的變數叫 `young`、`old`，課本改叫 `max_gen_pages`、`min_gen_pages`，避免跟 page 的 young/old 搞混。）

### 5.3 老化的過程：走頁表 + Bloom filter

[22:01] 增加序號之後，要**走過所有 `mm_struct` 的頁表**。

[22:33]–[23:00] 走的時候：

- 只處理 **young** 的 PTE：把它變成 old（清 Accessed 位元），記錄下來。
- 把它的 page **升到最新世代**（`max_seq`），因為它剛被用過。

[23:00]–[24:01] 加速：**跳過大部分是 old 的 PMD**（倒數第二層的表，管 512 個 PTE）。怎麼知道？用 **Bloom filter**：

- PMD 下的 page 大多是 young → 把這個 PMD 的位址**加進** Bloom filter。
- 之後走到一個 PMD，問 Bloom filter：
  - 回答「**不在**」→ Bloom filter **沒有假陰性** → **一定**大部分是 old → 整個 PMD 跳過。
  - 回答「在」→ 可能真的是 young，也可能是假陽性 → 要掃它底下的 PTE。

[24:01]–[25:00] 大局：

1. 先決定要不要老化（移動滑動視窗）。
2. 老化後走頁表，看 accessed / not accessed。大片 not accessed 的區域不用管，它們本來就在最低世代，會被踢掉。
3. accessed 的：改成 not accessed，把 page / folio 升到最新世代。

[25:00] 教授：這跟 WS 系列演算法很像：掃一次、再掃一次。只是不用一個位元，而是用一組世代；每次老化，滑動視窗往前移。待著不動的 page，視窗底部最後會碰到它，它就成了最舊的，該被踢了。

【課本補充】課本：如果 PMD 的頁表**根本沒被存取過**，可以整個標成沒存取過。Bloom filter 裡的 PMD 被判定 young 但其實是假陽性時，就要掃過它的 PTE，才能準確判斷，這是額外的工作。

### 5.4 【原始碼】kernel 的 Bloom filter

`mm/vmscan.c:3347-3369`（節錄）

```c
/*
 * Bloom filters with m=1<<15, k=2 and the false positive rates of ~1/5 when
 * n=10,000 and ~1/2 when n=20,000, where, conventionally, m is the number of
 * bits in a bitmap, k is the number of hash functions and n is the number of
 * inserted items.
 *
 * Page table walkers use one of the two filters to reduce their search space.
 * To get rid of non-leaf entries that no longer have enough leaf entries, the
 * aging uses the double-buffering technique to flip to the other filter each
 * time it produces a new generation. For non-leaf entries that have enough
 * leaf entries, the aging carries them over to the next generation in
 * walk_pmd_range(); the eviction also report them when walking the rmap
 * in lru_gen_look_around().
 ...
 */
#define BLOOM_FILTER_SHIFT	15
```

- **m = 2¹⁵ = 32768 位元**、**k = 2**（第 31 講教授舉例用 4 個雜湊函數，kernel 只用 2 個）。
- 註解自己寫了假陽性率：n = 10,000 時約 1/5，n = 20,000 時約 1/2。
- **兩個** filter 輪流用（雙緩衝）：每產生一個新世代就換到另一個，舊的清空。這就是「定期重設」：不再夠 young 的 PMD 自然消失；還夠 young 的，走訪時會被帶到新的 filter 裡。

`mm/vmscan.c:3376-3432`（節錄）

```c
static void get_item_key(void *item, int *key)
{
	u32 hash = hash_ptr(item, BLOOM_FILTER_SHIFT * 2);

	BUILD_BUG_ON(BLOOM_FILTER_SHIFT * 2 > BITS_PER_TYPE(u32));

	key[0] = hash & (BIT(BLOOM_FILTER_SHIFT) - 1);
	key[1] = hash >> BLOOM_FILTER_SHIFT;
}
...
static void update_bloom_filter(struct lruvec *lruvec, unsigned long seq, void *item)
{
	int key[2];
	...
	get_item_key(item, key);

	if (!test_bit(key[0], filter))
		set_bit(key[0], filter);
	if (!test_bit(key[1], filter))
		set_bit(key[1], filter);
}

static bool test_bloom_filter(struct lruvec *lruvec, unsigned long seq, void *item)
{
	int key[2];
	...
	if (!filter)
		return true;

	get_item_key(item, key);

	return test_bit(key[0], filter) && test_bit(key[1], filter);
}
```

| 部分 | 說明 |
|---|---|
| `get_item_key` | 不是兩個雜湊函數，而是**一個** 30 位元的雜湊，切成兩個 15 位元的 key |
| `update_bloom_filter` | 設兩個位元。先 `test_bit` 再 `set_bit`：已經是 1 就不寫，避免無謂地弄髒 cache line（第 18 講的 TTAS 同一個想法） |
| `test_bloom_filter` | 兩個位元都是 1 才回答「在」 |
| `if (!filter) return true` | filter 配置失敗時一律回答「在」：寧可多掃，也不能錯過（保持「沒有假陰性」） |

`item` 就是 PMD 項目的位址。

【實測】用同樣的參數（m = 2¹⁵、k = 2、一個 30 位元雜湊切兩半）模擬（`bloom2.py`）：

```
m=2^15, k=2, n= 2000: false positive   1.3%  (theory   1.3%)
m=2^15, k=2, n=10000: false positive  20.7%  (theory  20.9%)
m=2^15, k=2, n=20000: false positive  50.0%  (theory  49.7%)
```

跟 kernel 註解說的「1/5」「1/2」一致。

---

## 6. 驅逐（eviction）

[25:30] 驅逐演算法本身很簡單：最小序號的串列如果空了，就把 `min_seq` 加一；不空就考慮踢它們。

[26:01] 但比較複雜的是要先做兩個選擇：踢**哪一種**（匿名 / 檔案）、**哪些** page（tier）。

### 6.1 先選：踢匿名 page 還是檔案 page

[29:30] 第一個要做的選擇：**踢檔案 page，還是匿名 page？**

【課本補充】課本整理了兩種論點：

| 偏好踢 | 理由 |
|---|---|
| **檔案 page** | 它在儲存裝置上本來就有一份（檔案本身），踢掉不用寫 swap、不用管 swap 空間 |
| **匿名 page** | 檔案 page 常是**程式碼**（程式和共享函式庫的執行檔）或重要資料；匿名 page 有時根本不含有效資料（例如 stack 已經縮回去的部分） |

兩種論點在不同情況下都對，所以 Linux 兩個都提供，用 **swappiness** 調整。

### 6.2 【原始碼】`isolate_folios` 的第一步

`mm/vmscan.c:4983-4997`

```c
	/*
	 * Try to make the obvious choice first. When anon and file are both
	 * available from the same generation, interpret swappiness 1 as file
	 * first and 200 as anon first.
	 */
	if (!swappiness)
		type = LRU_GEN_FILE;
	else if (min_seq[LRU_GEN_ANON] < min_seq[LRU_GEN_FILE])
		type = LRU_GEN_ANON;
	else if (swappiness == 1)
		type = LRU_GEN_FILE;
	else if (swappiness == 200)
		type = LRU_GEN_ANON;
	else
		type = get_type_to_scan(lruvec, swappiness, &tier);
```

[35:30]–[36:32]

| 條件 | 選 | 理由 |
|---|---|---|
| swappiness = 0 | 檔案 | 特殊值：完全不踢匿名 page |
| 匿名的最小序號**更小** | 匿名 | 匿名 page 更老 |
| swappiness = 1 | 檔案 | 一個極端 |
| swappiness = 200 | 匿名 | 另一個極端 |
| 其他 | 「比賽」：`get_type_to_scan` | §6.3 |

注意第二行：檔案 page 比較老時**不會**直接選檔案，而是交給比賽決定。課本說「稍微偏袒檔案 page」。

⚠️ 課本 Listing 6.17 多了一行「`!(sc->gfp_mask & __GFP_IO)` → 選檔案」。v6.2.12 的 `isolate_folios` **沒有**這一行。

### 6.3 控制理論：refault 率除以 gain

[30:02]–[34:30] 比賽怎麼比？先比兩種類型的 **tier 0**（最乾淨、沒被額外存取過的 page）：

```
                         refault 次數
正規化 refault 率 = ─────────────────
                      這一類被驅逐的總數
```

[31:00] 例：100 個 page、20 次 refault → 20 / 100。

[31:00]–[32:01] **gain（增益）**：你**想踢**這類 page 的程度。

| 類型 | gain |
|---|---|
| 匿名 | **swappiness** |
| 檔案 | **200 − swappiness** |

（swappiness 預設 60：匿名 60、檔案 140，偏向踢檔案。）

[32:01]–[34:00] 比較：

```
正規化 refault 率(匿名)            正規化 refault 率(檔案)
──────────────────────     vs     ──────────────────────
     gain(匿名)                         gain(檔案)
```

- 理想上兩邊**相等**：想踢得越多，refault 就該越多，比例是常數。
- 如果某一邊**比較低**，表示這類 page 的 refault 跟「想踢的程度」不成比例 → **多踢它一點**，讓它的 refault 率上升，兩邊趨向相等。

[34:00] 借用了**控制理論**的術語：

| 名詞 | 對應 |
|---|---|
| set point（**SP**，設定點） | 匿名 page 的統計 |
| process variable（**PV**，程序變數） | 檔案 page 的統計 |

控制的目標是讓 PV 接近 SP。

[34:00]–[34:30] 另一個規則：檔案 page 的 refault **太少**（< 64），就選檔案。

【課本補充】課本：

- 不要看 refault 的**絕對數字**：refault 多可能只是因為驅逐也多，要除以驅逐總數。
- 「refault 率 / gain」= **每單位努力**帶來的平均 refault，兩類應該一樣，系統才不會反應過度或不足。（課本式 6.3）
- 實際上 kernel 在分子分母加了常數（課本式 6.4）：

```
   pv.refaulted                (sp.refaulted + α)
──────────────────────  ≤  ──────────────────────────
 pv.total × pv.gain         (sp.total + β) × sp.gain
```

α = 1、β = 64，是實驗得出的，很難用邏輯解釋。成立就選 PV（檔案），否則選 SP（匿名）。

- 檔案 refault < 64 就選檔案的理由：多半表示程式不太存取檔案、或檔案很小；而每個程式都有大量匿名記憶體（stack、heap），就算小程式，初始化的程式碼也不少。所以檔案 refault 超過門檻才考慮踢匿名。

### 6.4 【原始碼】`positive_ctrl_err` 與 `get_type_to_scan`

`mm/vmscan.c:3613-3617`

```c
struct ctrl_pos {
	unsigned long refaulted;
	unsigned long total;
	int gain;
};
```

`mm/vmscan.c:3672-3681`

```c
static bool positive_ctrl_err(struct ctrl_pos *sp, struct ctrl_pos *pv)
{
	/*
	 * Return true if the PV has a limited number of refaults or a lower
	 * refaulted/total than the SP.
	 */
	return pv->refaulted < MIN_LRU_BATCH ||
	       pv->refaulted * (sp->total + MIN_LRU_BATCH) * sp->gain <=
	       (sp->refaulted + 1) * pv->total * pv->gain;
}
```

`include/linux/mmzone.h:384`

```c
#define MIN_LRU_BATCH		BITS_PER_LONG
```

- `MIN_LRU_BATCH` = `BITS_PER_LONG` = **64**（64 位元系統）。這就是 β = 64，也是「refault < 64」的那個 64。
- `+ 1` 就是 α = 1。
- 用**交叉相乘**代替除法：kernel 裡避免除法（第 26 講也是）。

`mm/vmscan.c:4946-4972`

```c
static int get_type_to_scan(struct lruvec *lruvec, int swappiness, int *tier_idx)
{
	int type, tier;
	struct ctrl_pos sp, pv;
	int gain[ANON_AND_FILE] = { swappiness, 200 - swappiness };

	/*
	 * Compare the first tier of anon with that of file to determine which
	 * type to scan. Also need to compare other tiers of the selected type
	 * with the first tier of the other type to determine the last tier (of
	 * the selected type) to evict.
	 */
	read_ctrl_pos(lruvec, LRU_GEN_ANON, 0, gain[LRU_GEN_ANON], &sp);
	read_ctrl_pos(lruvec, LRU_GEN_FILE, 0, gain[LRU_GEN_FILE], &pv);
	type = positive_ctrl_err(&sp, &pv);

	read_ctrl_pos(lruvec, !type, 0, gain[!type], &sp);
	for (tier = 1; tier < MAX_NR_TIERS; tier++) {
		read_ctrl_pos(lruvec, type, tier, gain[type], &pv);
		if (!positive_ctrl_err(&sp, &pv))
			break;
	}

	*tier_idx = tier - 1;

	return type;
}
```

| 行 | 說明 |
|---|---|
| 4950 | gain：匿名 = swappiness、檔案 = 200 − swappiness |
| 4958-4960 | SP = 匿名的 tier 0、PV = 檔案的 tier 0。`positive_ctrl_err` 回 true（1）= `LRU_GEN_FILE`，false（0）= `LRU_GEN_ANON` |
| 4962 | 換成「**另一類**的 tier 0」當 SP |
| 4963-4967 | 被選中那一類的 tier 1、2、3 依序跟它比，第一個「不該踢」的 tier 就停下（§6.5） |
| 4969 | 記下最後一個該踢的 tier |

【實測】用 `positive_ctrl_err` 的公式算幾個例子（swappiness = 60，兩類各驅逐過 10000 個）：

```
swappiness=60: anon refaulted 100/10000, file 50/10000 -> evict FILE
swappiness=60: anon refaulted 100/10000, file 400/10000 -> evict ANON
swappiness=60: anon refaulted 10/10000, file 2000/10000 -> evict ANON
```

- 檔案只 refault 50 次（< 64）→ 直接選檔案。
- 檔案 refault 400：400 × 10064 × 60 ≈ 2.4 億 > 101 × 10000 × 140 ≈ 1.4 億 → 檔案的「refault 率 / gain」比較高，已經踢太多檔案了 → 改踢匿名。

### 6.5 Tier：世代裡再分層

[26:01]–[28:00] 每個世代再分成 **子世代**，叫 **tier（層）**：

- page 升到目前世代後，記錄它被存取的次數 N（用我們的特殊機制：每次看到 not accessed → accessed 就加一，不是真正的存取次數）。
- **tier = ⌈log₂(N + 1)⌉**。

| N | tier |
|---|---|
| 0 | log₂1 = 0 |
| 1 | log₂2 = 1 |
| 2 | ⌈log₂3⌉ = 2 |
| 3 | log₂4 = 2 |
| 6 | ⌈log₂7⌉ = 3 |

[28:00] 用位元運算很容易算。[28:32] **tier 0** = 從沒被存取過的：進來後一直乾淨，第一個該踢。

[37:00]–[38:30] 選好要踢的類型 T 之後：

```
          tier 0   tier 1   tier 2   tier 3
類型 T      >        >        >        <      ← 依序跟另一類的 tier 0 比
另一類   tier 0
                                       ↑
                     從這裡開始：存取多、refault 率也高 → 給第二次機會
```

（課本 Figure 6.24）

- tier 0（已經知道 T 比較該踢）→ 踢。
- tier 1 還是比另一類的 tier 0 低 → 也踢。tier 2 → 也踢。
- tier 3 開始**比較高** → 這些 page 存取比較多、refault 率也比較高（一直被誤踢）。
- [38:01] **給它們第二次機會**：把 tier 3（以上）的 page **升到最新世代**。之後如果又掉到最低世代，再用同樣的測試。

[40:31] 教授：存取多 = 這個 page 熱門；refault 高 = 一直犯錯，明明不該踢卻踢了。兩者都考慮，把這些 tier 的 page 升到最高序號，很合理。

【課本補充】課本：

- tier 越高，存取次數**指數級**越多，所以只要找到第一個「不該踢」的 tier，後面的都直接保留，不用一一計算。
- 升到新世代後**清掉參照計數**：「它已經因為參照次數多而受惠過一次了，不要再受惠第二次，重新開始。」

⚠️ 原始碼對 tier 的定義更精確：

`include/linux/mmzone.h:340-353`

```c
/*
 * Each generation is divided into multiple tiers. A page accessed N times
 * through file descriptors is in tier order_base_2(N). A page in the first tier
 * (N=0,1) is marked by PG_referenced unless it was faulted in through page
 * tables or read ahead. A page in any other tier (N>1) is marked by
 * PG_referenced and PG_workingset. This implies a minimum of two tiers is
 * supported without using additional bits in folio->flags.
 *
 * In contrast to moving across generations which requires the LRU lock, moving
 * across tiers only involves atomic operations on folio->flags and therefore
 * has a negligible cost in the buffered access path. In the eviction path,
 * comparisons of refaulted/(evicted+protected) from the first tier and the
 * rest infer whether pages accessed multiple times through file descriptors
 * are statistically hot and thus worth protecting.
```

- tier 計的是**透過檔案描述子**（`read()`、`write()`）的存取次數，不是頁表掃描看到的 Accessed 位元。
- 透過**頁表**被存取的 page（mmap 的、匿名的），走訪時看到 young 就**直接升到最新世代**（§5.3），不經過 tier。
- 原因：檔案用 `read()` 存取時不經過頁表，沒有 Accessed 位元可看，只能在 `read()` 的路徑上計數。換 tier 只要對 `folio->flags` 做 atomic 操作，幾乎沒成本；換世代則要拿 LRU 鎖。
- `MAX_NR_TIERS = 4`。

### 6.6 掃描 folio、真正驅逐

[41:00]–[43:30] 選好類型和 tier 之後，掃描最小序號、該類型的 folio。掃描時順便：

1. 該踢的：**標記**要驅逐。
2. 看到 accessed 的：改成 not accessed、記錄存取、升世代。
3. 更新 Bloom filter，讓之後的走訪更快。

[42:00]–[43:00] 教授：**做立即的工作，同時做一點延後的簿記工作**。既然都在掃頁表了，就順便多做一點：檢查存取位元、更新 Bloom filter。

[43:30]–[45:00] 標記之後、真正驅逐之前，再檢查：

| 情況 | 處理 |
|---|---|
| folio 被**釘住**（pinned） | 不能踢，像一個覆寫訊號 |
| 正在**寫回** | 讓它寫完，不要再疊一個驅逐 |
| 掃描之後**最近被存取了** | 踢它沒道理 |
| 可能有**競爭條件**（別的 thread 在搶它） | 先冷靜，等並行的事件結束 |

[45:00] 結果：可以驅逐的 folio 數量和串列。

[45:30]–[50:31] 驅逐步驟：

1. 判斷是**驅逐**還是**回收**。
2. 有並行事件就先暫停，等它們結束。
3. [46:31] **look around**：看看這個 folio 在頁表裡的**鄰居**（§6.7）。
4. 大的 folio 需要的話可以切成小的。
5. [48:31] **寫回**：釋放緩衝區（例如檔案的 page cache）、**flush TLB**、修改頁表（標成在 swap 裡）、叫 I/O 子系統寫出去。

> **課本 Point 6.3.5**：把額外的簿記工作**搭便車**在一般操作上（例如驅逐 folio 時），是現代作業系統常見的模式。與其一次處理頁表這種大結構，不如讓每個操作多負擔一點點簿記。驅逐 folio 大多不在關鍵路徑上，負擔得起。

### 6.7 Look around：順便看看鄰居

[50:01]–[53:30] **`lru_gen_look_around`**：驅逐一個 page 時，看它附近：

1. 找到它的 PMD 項目。一個 PMD 管 **512** 個 PTE。
2. 取其中一個 **64 個 PTE** 的視窗（look-around 大小）。
3. 一個一個看：
   - old → 不用管。
   - **young** → 找到它的 folio，把 folio 裡所有 page 標成 old（清 Accessed 位元）；如果它的世代不是 `max_seq`，記下來。
4. 這 64 個裡 young 的超過門檻 → 這是一個 **young 的 PMD** → 加進 Bloom filter。
5. 把剛才記下來的 young folio 升到 **`max_seq`**。

[52:32] 為什麼這樣做？既然有 page 被踢，看看它附近發生什麼事。程式有**空間局部性**。

【原始碼】註解：

`mm/vmscan.c:4619-4626`

```c
/*
 * This function exploits spatial locality when shrink_folio_list() walks the
 * rmap. It scans the adjacent PTEs of a young PTE and promotes hot pages. If
 * the scan was done cacheline efficiently, it adds the PMD entry pointing to
 * the PTE table to the Bloom filter. This forms a feedback loop between the
 * eviction and the aging.
 */
void lru_gen_look_around(struct page_vma_mapped_walk *pvmw)
```

- 「當 `shrink_folio_list()` **走 rmap** 時利用空間局部性」：驅逐時要用第 31 講的 reverse mapping 找到每個 PTE（並檢查它有沒有被存取，`folio_referenced`），既然都找到 PTE 了，就順便看看旁邊的。
- 「這在**驅逐和老化之間形成回饋迴路**」：驅逐時發現的 young PMD 放進 Bloom filter，老化走頁表時就知道哪些 PMD 值得看。
- 「cache line 有效率地掃描」：64 個 PTE × 8 bytes = 512 bytes = 8 條 cache line。

[54:32]–[55:00] `kswapd`、寫入命令、page 配置模組都可能觸發老化和驅逐，它們會呼叫 `walk_mm`（走記憶體地圖）：更新世代、把 accessed 改成 not accessed、**跳過 old 的項目**（只處理 young 的）。Bloom filter 加速走訪；加入容易、刪除很難，所以要定期重設。

---

## 7. Thrashing

[55:32]–[56:33] 回顧第 29 講的 **thrashing**：開機時開 Edge 很慢，沒耐心又開 Chrome、Firefox……working set 塞不進記憶體 → 一直在抓 page → 抓的時候又踢掉別的程式的 working set → CPU 負載看起來很低 → 排程器開更多程式 → 系統崩潰。

【課本補充】課本的描述：

- 第一次從儲存裝置載入 page 本來就慢（比記憶體慢好幾個數量級）。這時再開別的程式，儲存裝置的頻寬被瓜分，兩個都變慢。
- 大程式的 working set 接近記憶體大小時，會互相踢對方的 page。context switch 回來，又缺頁、又抓、又踢別人……
- MGLRU 本身已經有一點保護：剛進來的 page 是最新世代，要一段時間才會變老、可被踢。但程式很多時，§5.2 的條件會很快觸發老化（最新世代的 page 太多），thrashing 時這些機制可能行為失常。

> **課本 Definition 6.3.1**：**Thrashing**：同時跑太多程式，working set 都塞不進記憶體，彼此不斷踢對方 working set 的 page，大部分時間都在等。排程器以為 CPU 負載低，又排更多 process，惡性循環可能讓整個系統當掉。

### 7.1 min_ttl：至少留 N 毫秒

[57:00] 還記得 `lru_gen_struct` 有 `timestamps`（世代的建立時間）。想法很簡單：

> **一個 page 進記憶體不到 N 毫秒，就不准踢。**

[57:30] N = 1000（1 秒）是個好值：程式啟動後，它的 page 至少留 1 秒，不會一直被沖掉。這大幅限制 thrashing。

[57:30]–[58:02] 就算這樣還是留不住 working set、refault 率還是很高，就判定 thrashing 已經發生：負載太重，working set 塞不下，大家互踢、沒做有用的事。這時 **OOM killer**（Out-Of-Memory killer）出動，**終止**幾個程式。

【課本補充】課本：這是一種**准入控制**（admission control），限制 process 的數量。配合讓 working set 的 page 在記憶體裡待久一點，就能有效防止 thrashing。

### 7.2 【原始碼】留不住就叫 OOM killer

`mm/vmscan.c:4561-4562`

```c
/* to protect the working set of the last N jiffies */
static unsigned long lru_gen_min_ttl __read_mostly;
```

`mm/vmscan.c:4543-4553`（`age_lruvec` 裡）

```c
	if (min_ttl) {
		int gen = lru_gen_from_seq(min_seq[LRU_GEN_FILE]);
		unsigned long birth = READ_ONCE(lruvec->lrugen.timestamps[gen]);

		if (time_is_after_jiffies(birth + min_ttl))
			return false;
		...
	}
```

- 看**最舊世代**的建立時間 `birth`。
- 現在還沒超過 `birth + min_ttl` → 最舊的世代都還太年輕 → 這個 lruvec **不能**老化、不能踢，回傳 false。

`mm/vmscan.c:4599-4616`（`lru_gen_age_node` 的結尾）

```c
	/* check the order to exclude compaction-induced reclaim */
	if (success || !min_ttl || sc->order)
		return;

	/*
	 * The main goal is to OOM kill if every generation from all memcgs is
	 * younger than min_ttl. However, another possibility is all memcgs are
	 * either below min or empty.
	 */
	if (mutex_trylock(&oom_lock)) {
		struct oom_control oc = {
			.gfp_mask = sc->gfp_mask,
		};

		out_of_memory(&oc);

		mutex_unlock(&oom_lock);
	}
```

- `kswapd` 對每個 memcg 都試了老化，**沒有一個成功**（所有世代都比 `min_ttl` 年輕），而且有設 `min_ttl` → 呼叫 **`out_of_memory`**：OOM killer。
- 註解：「主要目的是：如果所有 memcg 的每個世代都比 `min_ttl` 年輕，就 OOM kill。」

⚠️ 課本說「大部分實作 N = 1000」。v6.2.12 的 **預設是 0（關閉）**：

`Documentation/admin-guide/mm/multigen_lru.rst:71-81`

```
Users can write ``N`` to ``min_ttl_ms`` to prevent the working set of
``N`` milliseconds from getting evicted. The OOM killer is triggered
if this working set cannot be kept in memory. In other words, this
option works as an adjustable pressure relief valve, and when open, it
terminates applications that are hopefully not being used.

Based on the average human detectable lag (~100ms), ``N=1000`` usually
eliminates intolerable janks due to thrashing. Larger values like
``N=3000`` make janks less noticeable at the risk of premature OOM
kills.

The default value ``0`` means disabled.
```

- 寫到 `/sys/kernel/mm/lru_gen/min_ttl_ms` 才會開。
- 文件說它像一個「可調的洩壓閥」：打開時，終止那些「希望沒在用的」程式。
- **N = 1000 是建議值**：人能察覺的延遲約 100 ms，1000 通常能消除 thrashing 造成的卡頓；3000 更不明顯，但可能過早 OOM kill。

### 7.3 可調的參數

[58:31]–[59:30] MGLRU 的行為可以設定：

| 參數 | 控制什麼 |
|---|---|
| 最少 / 最多世代數 | 積極程度：資訊多細、給幾次第二次機會。世代多，被存取的 page 升到很高的序號後要很久才掉到底，在記憶體待得久 |
| **swappiness** | 檔案和匿名的相對優先 |
| `min_ttl`（N = 1000） | thrashing 保護 |

[59:30] 教授：Linux 工程師的工作就是**了解工作負載**，據此設好這些參數：不要 thrashing、不要變慢、東西還能動。

---

## 8. 【實測】本機的 MGLRU

```
$ cat /sys/kernel/mm/lru_gen/enabled
0x0007
$ cat /sys/kernel/mm/lru_gen/min_ttl_ms
0
$ cat /proc/sys/vm/swappiness
60
$ grep CONFIG_LRU_GEN /boot/config-$(uname -r)
CONFIG_LRU_GEN=y
CONFIG_LRU_GEN_ENABLED=y
# CONFIG_LRU_GEN_STATS is not set
```

- **`enabled = 0x0007`**：MGLRU 開著，三個元件全開（`multigen_lru.rst:36-50`）：

| 位元 | 意思 |
|---|---|
| `0x0001` | 主開關 |
| `0x0002` | MMU 會設 Accessed 位元時（例如 x86），**大批**清除葉節點 PTE 的 Accessed 位元 |
| `0x0004` | 連**非葉節點**（PMD）的 Accessed 位元也清（x86 的 PMD 項目也有硬體 Accessed 位元，只在 Intel 和 AMD 驗證過） |

- **`min_ttl_ms = 0`**：thrashing 保護**沒開**（預設）。
- **swappiness = 60**：匿名 gain 60、檔案 gain 140，偏向踢檔案。

```
$ grep -E 'workingset_refault|pgscan_kswapd|pgsteal' /proc/vmstat
workingset_refault_anon 0
workingset_refault_file 0
pgsteal_kswapd 0
pgsteal_direct 0
pgscan_kswapd 0
...
$ free -g
               total        used        free      shared  buff/cache   available
Mem:              38           3          21           0          13          34
```

- 開機以來 refault 次數、kswapd 回收次數**全是 0**：這台機器有 38 GB 記憶體，只用了 3 GB，從來沒有記憶體壓力，MGLRU 根本沒機會驅逐東西。
- 13 GB 是 page cache（檔案 page），需要時 MGLRU 會優先回收它們（swappiness 60 偏向檔案）。

---

## 9. 一頁速查表

### 9.1 世代

| 名詞 | 意思 |
|---|---|
| `max_seq` | 最新世代；**老化**時 +1 → 所有 page 自動變老 |
| `min_seq[匿名/檔案]` | 最舊世代；**驅逐**清空後 +1 |
| 世代數 | 2 ≤ max_seq − min_seq + 1 ≤ 4，理想是 3 |
| `lists[gen][type][zone]` | 三維的 folio 串列 |
| `timestamps[gen]` | 世代建立時間（煞車、min_ttl） |

### 9.2 老化

| 條件 | 動作 |
|---|---|
| 世代數 < 3 | 老化 |
| 世代數 > 3 | 不老化 |
| 年輕 × 2 > total，或老 × 4 < total | 老化 |

走頁表：young → old，folio 升到 `max_seq`；Bloom filter（m = 2¹⁵、k = 2、雙緩衝）記住 young 的 PMD，不在 filter 裡的 PMD 一定是 old，跳過。

### 9.3 驅逐

| 步驟 | 內容 |
|---|---|
| 選類型 | swappiness 0 → 檔案；匿名更老 → 匿名；1 → 檔案；200 → 匿名；否則比 tier 0 的「refault 率 / gain」 |
| gain | 匿名 = swappiness，檔案 = 200 − swappiness |
| 公式 | 選檔案 ⟺ 檔案 refault < 64，或 pv.ref × (sp.total + 64) × sp.gain ≤ (sp.ref + 1) × pv.total × pv.gain |
| 選 tier | 被選類型的 tier 1..3 依序跟另一類的 tier 0 比；第一個不該踢的 tier 起，升到最新世代 |
| tier | ⌈log₂(N+1)⌉，N = 透過檔案描述子的存取次數；最多 4 層 |
| 檢查 | 釘住、寫回中、剛被存取、競爭 → 跳過 |
| look around | 驅逐時看同一個 PMD 裡 64 個 PTE：young 的升世代、young 的 PMD 進 Bloom filter |
| 寫回 | 釋放緩衝、flush TLB、改頁表、I/O |

### 9.4 Thrashing

`min_ttl_ms`（預設 0，建議 1000）：最舊世代比它年輕就不准驅逐；所有 memcg 都這樣就叫 OOM killer。

### 9.5 本講看過的原始碼位置

| 位置 | 內容 |
|---|---|
| `include/linux/mmzone.h:332-360` | `MIN_NR_GENS`、`MAX_NR_GENS`、tier 的定義、`MAX_NR_TIERS` |
| `include/linux/mmzone.h:407-429` | `struct lru_gen_struct` |
| `include/linux/mmzone.h:444-459` | `struct lru_gen_mm_state` |
| `include/linux/mmzone.h:508-530` | `struct lruvec` |
| `include/linux/mmzone.h:384` | `MIN_LRU_BATCH` = 64 |
| `mm/vmscan.c:3347-3432` | Bloom filter |
| `mm/vmscan.c:3613-3617, 3672-3681` | `ctrl_pos`、`positive_ctrl_err` |
| `mm/vmscan.c:4468-4523` | `should_run_aging` |
| `mm/vmscan.c:4543-4553, 4561-4616` | `min_ttl`、OOM kill |
| `mm/vmscan.c:4619-4626` | `lru_gen_look_around` |
| `mm/vmscan.c:4946-4972` | `get_type_to_scan` |
| `mm/vmscan.c:4983-4997` | `isolate_folios` 選類型 |
| `Documentation/admin-guide/mm/multigen_lru.rst:36-81` | `enabled` 位元、`min_ttl_ms` |

---

## 10. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **回收的 page 回到 buddy allocator** | **第 33 講**：Kernel memory allocation |
| **page cache、檔案對應的 page、`read()` 的路徑** | 課本第 7 章（本播放清單未涵蓋，待補） |
| **reverse mapping（`folio_referenced`、`try_to_unmap`）** | 回顧第 31 講 |
| **Accessed 位元、WS-Clock、working set** | 回顧第 29 講 |
| **memory cgroup** | 待補（本課程未專講） |

（講次對應依播放清單標題推估。）

---

## 附：本講一句話總結

> **MGLRU 把 page 分成最多 4 個世代：老化時只把 `max_seq` 加一，所有沒被存取的 page 就自動變老，最舊世代清空後 `min_seq` 才加一，像一個往前滑的視窗；理想上維持 3 個世代，最新世代的 page 太多或最舊世代太少就老化。老化時走頁表，把 young 的 PTE 清成 old 並把 folio 升到最新世代，用 m = 2¹⁵、k = 2 的雙緩衝 Bloom filter 記住哪些 PMD 大多是 young，不在 filter 裡的 PMD 一定是 old、整個跳過（實測假陽性率在 1 萬項時約 1/5，跟 kernel 註解一致）。驅逐時先決定踢匿名還是檔案：swappiness 決定 gain（匿名 = swappiness、檔案 = 200 − swappiness），比較 tier 0 的「refault 率 / gain」，誰比較低就多踢誰，檔案 refault 不到 64 次則直接踢檔案；被選中的那一類再逐 tier 比較，存取多又常被誤踢的 tier 升到最新世代給第二次機會；真正驅逐前跳過釘住、寫回中、剛被存取的 folio，並順便看同一個 PMD 裡 64 個鄰居，形成驅逐與老化的回饋迴路。防 thrashing 用 `min_ttl_ms`（預設關閉、建議 1000）：最舊世代太年輕就不准驅逐，所有 cgroup 都這樣就叫 OOM killer。本機 MGLRU 開著（0x0007），但 38 GB 記憶體從沒遇到壓力，refault 數一直是 0。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 32（Chapter 6, Part IV）錄影逐字稿（YouTube 自動字幕，1:00:06）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§6.3.2–6.3.3，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
