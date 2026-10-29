# Advanced Computer Architecture 第二十二支影片完整筆記：AMAT、三個 C、簡單的 cache 最佳化與 virtual memory（Chapter 7, Part II）

> **課程**：Advanced Computer Architecture — Chapter 7: Caches (Part II) - Optimizations and Virtual Memory（播放清單第 22 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕下載不到）（50:58）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch7** §7.1.6–7.2（p.268–277）
> **章號**：影片 Chapter 7 = 課本 Ch7。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 的結尾 [50:17] 之後是重複的「Thank you」和亂碼，影片在那裡結束。[40:44] 老師先說「CPI 會變很低」，馬上改口「我說錯了，是 CPI 變高、IPC 變低」，筆記照改正後的寫。

這一講的前半（AMAT、三個 C、victim cache、write buffer、critical word first）在 [Basic 第 36 支](../basic/lecture36_note.md) 講過、也實測過；virtual memory 在 Basic 課程的對應章節也有。這份筆記照影片整理，實測做 Basic 沒做過的部分。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. AMAT](#1-amat)
- [2. AMAT 和 CPI](#2-amat-和-cpi)
- [3. 三個 C](#3-三個-c)
- [4. 簡單的最佳化](#4-簡單的最佳化)
- [5. Victim cache](#5-victim-cache)
- [6. Write buffer](#6-write-buffer)
- [7. Critical word first 與 early restart](#7-critical-word-first-與-early-restart)
- [8. Process 和它的 memory map](#8-process-和它的-memory-map)
- [9. 兩個問題：overlap 和 size](#9-兩個問題overlap-和-size)
- [10. Virtual memory：page 和 frame](#10-virtual-memorypage-和-frame)
- [11. 解決 overlap，也能故意共用](#11-解決-overlap也能故意共用)
- [12. 解決 size：swap space](#12-解決-sizeswap-space)
- [13. Page table 太慢 → TLB](#13-page-table-太慢--tlb)
- [14. TLB miss：hardware 或 software page walk](#14-tlb-misshardware-或-software-page-walk)
- [15. 一次記憶體存取的完整流程](#15-一次記憶體存取的完整流程)
- [16. 【原始碼】新的 frame 為什麼是 0](#16-原始碼新的-frame-為什麼是-0)
- [17. 【實測】](#17-實測)
- [18. 一頁速查表](#18-一頁速查表)
- [19. 與後續講次的連結](#19-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
AMAT = L1 hit time ＋ L1 miss rate × L1 miss penalty（一層一層展開）
CPI = CPI_base ＋ f_mem × (AMAT − L1 hit time)
miss 的三個 C：compulsory、capacity、conflict → 對應的最佳化（block 大小、cache 大小、associativity、prefetch、victim cache）
write buffer、critical word first ＋ early restart
virtual memory：
  每個 process 以為自己擁有整個 memory map → overlap 問題、size 問題
  virtual address → 翻譯 → physical address；page（4 KB）對到 frame
  frame 在主記憶體或 swap space；page table 存對應，TLB 是 page table 的 cache
  TLB miss → hardware（x86：CR3）或 software page walk；新的 frame 要清成 0
```

---

## 1. AMAT

[00:00] **AMAT**（average memory access time，平均記憶體存取時間）：

```
AMAT = L1 hit time ＋ L1 miss rate × L1 miss penalty
L1 miss penalty ≈ L2 access time = L2 hit time ＋ L2 miss rate × L2 miss penalty
L2 miss penalty ≈ L3 hit time ＋ L3 miss rate × L3 miss penalty
L3 miss penalty = 主記憶體的存取時間（假設主記憶體一定 hit）
```

- 不管 hit 或 miss，**都要先 lookup** 才知道，所以 hit time 一定要付。
- 這就是期望值的公式。
- L1 miss penalty 大約等於 L2 的存取時間（忽略 L1 到 L2 的傳輸延遲）。

---

## 2. AMAT 和 CPI

[01:24]

```
CPI = CPI_base ＋ f_mem × (AMAT − L1 hit time)
```

- **CPI_base**：理想記憶體（每次都 L1 hit）時的 CPI。
- **f_mem**：記憶體指令（在關鍵路徑上的）的比例。

[02:53] 檢查：L1 miss rate = 0 → AMAT = L1 hit time → CPI = CPI_base。

[03:48] 這個公式主要適用於 **in-order** pipeline。OOO pipeline 裡不同記憶體指令的 miss 時間有很多**重疊**，這也是 OOO 的好處之一。

【課本補充】課本 p.268 §7.1.6 是同樣的推導。

---

## 3. 三個 C

[03:48] 做好 cache 的策略：降低 **hit time**、降低 **miss rate**、降低 **miss penalty**，或它們的組合。

[04:58] miss 的三種（都是 C 開頭）：

| 種類 | 原因 | 例子 |
|---|---|---|
| **Compulsory（cold）miss** | 第一次讀到這個 block，本來就不在 | 「它一定會發生」 |
| **Capacity miss** | cache 太小 | 常用 100 KB，cache 只有 64 KB → 多出來的 36 KB 會 miss |
| **Conflict miss** | associativity 有限、destructive interference | 4-way 的 cache，5 個 block 對到同一個 set，一直互相趕走 |

【課本補充】Basic 課程在本機用模擬器把真實程式的 miss 分成三個 C：[Basic 第 36 支 §16.2](../basic/lecture36_note.md#162-把-miss-分成三個-c)。

---

## 4. 簡單的最佳化

[06:05]

| 目標 | 做法 | 缺點 |
|---|---|---|
| 降低 hit time | 小而簡單的 cache | miss rate 變高 |
| 降低 compulsory miss | **block 變大**（64 → 128 B）：spatial locality | cache 能放的 block 變少，蓋到的位址空間變少 |
| 降低 capacity miss | cache 變大 | 變慢 |
| | **Prefetching**：預測之後會存取的位址，提早拿來 | |
| 降低 conflict miss | associativity 變高 | 延遲、耗電 |
| | prefetching 也有幫助 | |
| | **Victim cache** | |

[08:05] Prefetching 可以：

- **軟體**：插入專門的 prefetch 指令。
- **硬體**：一個 **prefetcher** 單元，像 branch predictor 一樣預測之後要存取的位址（[第 26 支](lecture26_note.md)）。

【課本補充】Basic 課程試過 software prefetch 在本機有沒有用：[Basic 第 36 支 §16.1](../basic/lecture36_note.md#161-software-prefetch-真的有用嗎)。

---

## 5. Victim cache

[09:34] 【口頭】「在計算機結構裡，我們永遠需要一個**程式層級的洞見**。」

洞見：只有**少數幾個 set** 競爭很激烈。那些 set 裡有幾個「不幸的」block 一直被趕出去。

[10:08] 做法：與其把它們放回 L2，不如放在一個**小的 fully associative cache**（8 或 16 格）：**victim cache**。「fully associative 少見的用途之一。」

[11:08] L1 miss 時，**同時**查 L1 和它的 victim cache；兩個都沒有才去 L2。

[11:30] 怎麼管理 victim cache、哪些 line 要進去，有很多演算法，這裡不討論。

【課本補充】課本 p.270：victim cache 放在 L1 和 L2 之間，8–64 格。Basic 課程模擬過 victim cache：[Basic 第 36 支 §16.3](../basic/lecture36_note.md#163-victim-cache)。

---

## 6. Write buffer

[11:52] 老師說投影片上沒寫、但值得一提：**write buffer**。

情境：L1 miss，接著來了一大堆寫入。

[12:20] 如果是寫同一個 block 的不同位置（例如 64 byte 的 line 寫前 4 byte、中間 4 byte、後面 4 byte），何必分開送、分開存取 cache？

- 用一個小結構記下「哪些 bit 被寫了」。因為 spatial locality，這些寫入會在一小段時間內一起來。
- 把這些寫入**合併**，最後跟 line 的內容合成一次寫入。

[13:49] 好處：減少 cache 請求 → **省電**，也減少到 cache 的流量。Write buffer 也可以做成一個小的 fully associative cache。

【課本補充】課本 p.270：write buffer 放在兩層之間，對整個 block 只送一個 write-miss 訊息、把寫入合併。讀的時候，如果資料在 write buffer 裡就從那裡拿；不在的可以繞過 write buffer 直接往下送。這讓處理器可以繼續處理其他記憶體請求。

---

## 7. Critical word first 與 early restart

[14:39] 降低 miss penalty 的另一招。處理器通常只要 64 byte block 裡的 **4 byte**。

[15:07] 例：L2 → L1 的資料要 10 個 cycle 才開始來，每個 cycle 傳 4 byte → 傳完要 16 個 cycle（每 cycle 8 或 16 byte 的話也要 4–8 個 cycle）。

[15:40] 不要從 byte 0 依序傳到 byte 63：**先傳處理器要的那 4 byte**，其他 60 byte 之後再傳。

[16:17] 那 4 byte 一到，處理器就**開始做事**，不用等整個 block：**early restart**。整個優化叫 **critical word first ＋ early restart**。

[16:52] Cache 的「骨架」到這裡講完了；還沒從電的角度看（[第 23 支](lecture23_note.md)、[第 24 支](lecture24_note.md)）。

---

## 8. Process 和它的 memory map

[17:35] 記憶體系統的另一個重要觀念，老師打了「兩個加號」：**virtual memory**。「其實很簡單，但 99% 的人好像都沒搞懂。」

[17:51] **Process**：一個執行中的程式。它假設自己**獨佔**一大塊連續的記憶體空間（就算不是全部，也是很大一部分），裡面任何位置都可以隨意讀寫，不用經過允許。

[18:35] 32-bit Linux 的 memory map（`0x00000000`–`0xFFFFFFFF`）：

```
0xFFFFFFFF ┌────────────────┐
           │ OS kernel       │ 上面 1 GB
0xC0000000 ├────────────────┤
           │ stack ↓         │ 從 process 最高的位址往下長（可以長好幾 GB；圖不照比例）
           │      …          │
           │ heap ↑          │ malloc／new 配的（跨函數呼叫還活著）
           │ bss             │ 沒有初值的全域、static 變數
           │ data            │ 有初值的全域、static 變數
           │ text            │ 程式的指令
           │ header          │ 前幾百 byte：magic number、各區段的位置
0x00000000 └────────────────┘
```

- [19:55] 下面 **3 GB** 全部給 process，它是「無可爭議的擁有者」。
- [20:27] 所有函數共用同一個 stack 區，很深的遞迴也有很多空間。

（本機 64-bit 的 memory map 見 [§17.1](#171-一個-process-的-memory-map)。）

---

## 9. 兩個問題：overlap 和 size

[23:35] Windows 按 Ctrl-Alt-Delete 開工作管理員，或 Linux 跑 `ps -ef`，會看到幾百個 process。它們輪流執行：一個跑一陣子 → timer interrupt → 另一個跑……

| 問題 | 說明 |
|---|---|
| **Overlap／overwrite** | 所有 process 看到**一樣的** memory map（stack 都從同一個地方開始）→ 一個 process 可能（故意或不小心）寫到另一個 process 的資料 |
| **Size** | memory map 最多 3 GB，用了 2 GB，但實際只有 512 MB 的 RAM → 跑不起來？ |

[26:28] 兩個都要解決，才能讓一個處理器變成一個能**同時跑很多 process** 的電腦系統。

---

## 10. Virtual memory：page 和 frame

[27:00] 解法：**virtual memory**。

```
處理器、compiler、程式 → virtual address（32 bit；process 以為自己擁有 0–3 GB）
        ↓ 翻譯（memory management unit，MMU）
     physical address
```

[28:00] 翻譯保證兩件事：

1. 除非刻意，**兩個 process 不會存取同一個 physical address** → 沒有 overlap。
2. 不受主記憶體大小限制。

[31:08] 做法：

| | 切成 | 大小 |
|---|---|---|
| Virtual memory（程式用的位址） | **page** | 4 KB |
| Physical memory | **frame** | 4 KB |

只要把 page 對到 frame。

---

## 11. 解決 overlap，也能故意共用

[32:38] Process 1 和 process 2 各有 3 個連續的 page：

- 它們可以對到主記憶體裡**任何** 3 個 frame，**不用連續**。在 virtual 裡連續，在 physical 裡不一定。
- 重點：**同一個 frame 不會對到兩個 process 的 page**。

[34:22] 所以就算一個 process 有惡意、想讀另一個 process 的密碼或信用卡號，virtual memory 也會擋下來：程式只能存取自己 memory map 裡的 virtual address，最後對到哪個 physical frame，由處理器（和 OS）決定。**virtual memory 也保護了系統的安全**。

[35:34] 有時**想要** overlap：**inter-process communication**（IPC，「另一個意思的 IPC」）。Linux 的 shared memory（shmem）呼叫：把兩個 process 的 virtual page 對到**同一個** frame，一邊寫另一邊就看得到。這是雙方同意的，不是安全漏洞；但一般會避免 overlap。

（本機試了 fork 之後同一個位址的值、和 MAP_SHARED 的共用，見 [§17.2](#172-同一個虛擬位址不同的值或故意共用)。）

---

## 12. 解決 size：swap space

[36:59] 在硬碟（或 flash 之類的穩定儲存）上切一塊 **swap space**，讓「主記憶體 ＋ swap space」≥ 所有 process 需要的記憶體。

- swap space 慢很多，要做一些最佳化。
- 對應表會指出 page 對到主記憶體的 frame，還是 swap space 的 frame。

[37:58] 要用 swap space 裡的 frame：

1. 先把它從 swap space 搬到主記憶體，放在某個位置，更新對應。
2. 主記憶體沒有空的 frame？就像 set 裡沒有空的 way：選一個 frame 搬到 swap space，騰出空間（**page replacement**）。

[38:40] Page replacement 由 **OS** 做：這也是 OS 當「記憶體管理者」的角色。frame 在主記憶體時不需要 OS 出手，管理 swap space 才需要。

---

## 13. Page table 太慢 → TLB

[39:58] 對應存在 **page table**：virtual page → physical frame。

[40:20] 但 page table 是很複雜的資料結構，查一次要好幾個 cycle。估算：

```
in-order pipeline、沒有相依：理想 CPI = 1
1/3 的指令是記憶體指令（老師用 0.3）、全部 L1 hit、翻譯要 3 cycle
CPI = 1 ＋ 0.3 × 3 = 1.9 → IPC ≈ 1/1.9 ≈ 一半
```

「這是 IPC 的巨大損失，不能接受。」

[42:00] 用 cache 一樣的邏輯：做一個**對應的 cache**：**TLB**（translation lookaside buffer）。

- 因為 temporal locality，TLB 非常有效：命中率約 **99.5%**。
- 通常 **32–64 格**，fully associative 或 4–8-way。
- 很多現代設計有**兩層** TLB（快的 L1 TLB ＋ 慢一點的 L2 TLB）。
- 有時分開成 **ITLB**（指令）和 **DTLB**（資料）。
- 存取很快：一個 cycle 的一部分，最多一個 cycle。

【課本補充】課本 p.276：page table 單層的話要 2²⁰ 格（每格 20 bit 的 frame 編號）；大多數做成 **2 層**：前 10 bit 查第一層，每格指到一個第二層的表，再用接下來的 10 bit 查。很多第一層的格子用不到，就不用建對應的第二層 → 省很多空間。64-bit 位址空間用 3 或 4 層。代價是查一次要好幾次記憶體存取（每次主記憶體 200–400 cycle）。課本說 TLB 通常 **32–128** 格、不到一個 cycle。

（本機量了 TLB 的大小和 miss 的代價，見 [§17.4](#174-tlb-的大小與-huge-page)。）

---

## 14. TLB miss：hardware 或 software page walk

[43:32] TLB 沒有對應 → 查 page table。就算 page table 有，也要先放進 TLB 處理器才能用。

| 方式 | 誰做 | 說明 |
|---|---|---|
| **Hardware page walk** | 硬體（大多數 Intel 處理器） | 一個 machine-specific register **CR3** 存 page table 的基底位址；硬體裡（firmware）的小程式讀 CR3，走過 page table，找到對應，更新 TLB。有效率 |
| **Software page walk** | OS | TLB miss 時發 interrupt，OS（特權操作）找出對應、放進 TLB |

[45:00] 不管誰做，對應最後都來自 page table；沒有的話要建一個，再放進 TLB。

⚠️ 老師說 CR3 是「machine-specific register，叫 MSR」。x86 的 CR3 是一個 **control register**（CR 就是 control register 的縮寫）；MSR 在 x86 是另一類暫存器（用 `rdmsr`／`wrmsr` 存取）。Linux 原始碼裡兩者也分開：CR3 用 `arch/x86/include/asm/special_insns.h:52` 的 `native_write_cr3()`（`mov %0,%%cr3`），MSR 用 `arch/x86/include/asm/msr.h:245` 的 `rdmsr()` 巨集。

---

## 15. 一次記憶體存取的完整流程

[46:00] 「這張圖非常、非常、非常重要」：

```
記憶體存取
 → TLB hit？ 是 → 把對應給處理器
 → 否：walk page table（軟體或硬體）
     → frame 在主記憶體？ 是 → 放進 TLB → 給處理器
     → 否（或要配新的記憶體）：
         有空的 frame 嗎？ 沒有 → page replacement（FIFO、LRU…）：
             把一個 frame 趕到 swap space，更新它的 page table 和 TLB
         從 swap space 讀進新 frame 的內容，或建一個新的空 frame（清成 0）
         → 建 page table 的對應 → 放進 TLB → 給處理器
```

[47:38] 不同 OS 的政策不同：有的不准存取還沒對應的 frame；有的第一次存取時才配。

[48:54] **為什麼新的 frame 要清成 0？** 它之前可能裝著別的 process 寫的資料，例如密碼或信用卡號，當然要刪掉。

[48:54] Page table 的設計（通常兩層或多層，不能每個 virtual page 都一格）在老師的前一本書和 OS 的書裡。Page table 是 OS 當記憶體管理者的關鍵機制。

---

## 16. 【原始碼】新的 frame 為什麼是 0

Linux v6.2.12 處理「第一次碰一個匿名（anonymous，例如 malloc、stack）page」的 page fault 是 `do_anonymous_page()`。`mm/memory.c:4066-4068`、`:4087-4091`：

```c
	/* Use the zero-page for reads */
	if (!(vmf->flags & FAULT_FLAG_WRITE) &&
			!mm_forbids_zeropage(vma->vm_mm)) {
		…
	}

	/* Allocate our own private page. */
	if (unlikely(anon_vma_prepare(vma)))
		goto oom;
	page = alloc_zeroed_user_highpage_movable(vma, vmf->address);
```

- **寫**：配一個新的 frame，而且是 **zeroed**（`alloc_zeroed_user_highpage_movable`）：就是 [48:54] 說的「清成 0，免得看到別人的密碼」。
- **讀**：連 frame 都不配，直接對到一個全部是 0 的共用 **zero page**。等真的要寫時，再發一次 page fault，才配自己的 frame。

本機實測看得到這兩種行為，見 [§17.3](#173-第一次碰一個-pagepage-fault-與清成-0)。

---

## 17. 【實測】

程式在 `experiments/lecture22/`。本機是 64-bit Linux，page 大小 4096 byte（`getconf PAGESIZE`）。

### 17.1 一個 process 的 memory map

`vm.c` 印出各種東西的位址：

```
1. text（main）    0x5603d20551e0
   data（g_init）  0x5603d2058010
   bss（g_uninit） 0x5603d2058024
   heap（malloc）  0x5603d24102a0
   stack（local）  0x7ffd9351832c
```

- 順序跟 [§8](#8-process-和它的-memory-map) 的圖一樣：text < data < bss < heap ……（很遠）…… stack。
- 但 64-bit 的位址大很多：stack 在 `0x7ffd…`（約 128 TB 的地方），不是 3 GB。
- 每次執行位址都不一樣（上面兩次跑的 `0x5587…`、`0x5603…`）：Linux 的 **ASLR**（位址隨機化），讓攻擊者猜不到東西在哪。這跟 [34:22] 的安全性是同一類考量，影片沒提。

### 17.2 同一個虛擬位址，不同的值，或故意共用

`vm.c` 接著 `fork()` 出一個 child，child 把 `g_init` 改成 7：

```
2. child ：&g_init = 0x5603d2058010，g_init = 7
2. parent：&g_init = 0x5603d2058010，g_init = 42
3. MAP_SHARED：child 寫了 99，parent 讀到 99
```

- **同一個虛擬位址**，兩個 process 看到**不同的值**：它們的 page 對到不同的 frame。這就是 [§11](#11-解決-overlap也能故意共用) 的「沒有 overlap」。
- 用 `mmap(MAP_SHARED)` 開的一塊，child 寫 99，parent 也讀到 99：故意讓兩個 process 對到**同一個** frame，就是 [35:34] 的 shared memory IPC。

### 17.3 第一次碰一個 page：page fault 與清成 0

`vm.c` 最後 `mmap` 256 MiB（65536 個 4 KB page），每個 page 碰一個 byte，用 `getrusage` 數 page fault（跑兩次）：

```
4. 第一次讀 65536 個 page：65537 次 page fault，448 ns/page（讀到的全是 0）；第二次：0 次 fault，3.0 ns/page
   讀過之後再寫：又 65536 次 page fault（這時才真的配 frame）
4. 第一次寫 65536 個 page：65536 次 page fault，870 ns/page；第二次：0 次 fault，13.5 ns/page
```

（第二次跑：讀 404 ns/page、寫 887 ns/page，其他一樣。）

- **第一次碰每個 page 都是一次 page fault**：OS 這時才建對應（[§15](#15-一次記憶體存取的完整流程) 的「第一次存取時才配」那種政策）。
- **第一次寫**：每個 page 約 **870 ns**，包括配一個 frame 並把 4 KB **清成 0**。之後再寫只要 13.5 ns。
- **第一次讀**：只要約 450 ns，讀到的全是 0；第二次讀只要 **3 ns**，比第二次寫快很多。原因就是 [§16](#16-原始碼新的-frame-為什麼是-0) 的原始碼：讀的時候 65536 個 page 全部對到**同一個 zero page**，所以第二次讀全部打在 cache 裡的同一個 frame 上。
- 讀過之後再寫，**又**發生 65536 次 page fault：這時才真的各配一個自己的 frame。

### 17.4 TLB 的大小與 huge page

`tlb.c`：在 N 個 page 裡，每個 page 用**一條** cache line，隨機順序 pointer chase。資料（cache line 的數量）一樣，一次用一般的 **4 KB page**，一次用 **2 MB 的 huge page**（`madvise(MADV_HUGEPAGE)`，本機 THP 是 `madvise` 模式）。cache 的負擔一樣，差別只在 TLB：每一步碰一個不同的 4 KB page，但一個 2 MB 的 huge page 能蓋住 512 個這種 page。

| page 數 | 資料 | P-core 4 KB | P-core 2 MB | E-core 4 KB | E-core 2 MB |
|---|---|---|---|---|---|
| 32 | 2 KB | 1.07 ns | 1.07 | 0.84 | 0.84 |
| 64 | 4 KB | 1.07 | 1.07 | **3.34** | 0.84 |
| 128 | 8 KB | **2.57** | 1.07 | 3.34 | 0.84 |
| 512 | 32 KB | 2.57 | 1.07 | 3.34 | 0.84 |
| 1024 | 64 KB | 4.70 | 3.21 | 8.08 | 5.57 |
| 4096 | 256 KB | 8.06 | 3.21 | 14.81 | 5.57 |
| 16384 | 1 MB | 11.91 | 3.26 | 16.73 | 5.58 |
| 32768 | 2 MB | 20.60 | 14.44 | 26.03 | 7.86 |

（2 MB 那欄的 1.07 → 3.21 → 14.44 就是資料超過 L1、L2 的跳躍，跟 [第 21 支 §14.1](lecture21_note.md#141-p-core-和-e-core-的各層延遲) 一樣，這是 cache 的效果。）

- **P-core**：64 個 page 以內都是 1.07 ns；128 個 page 就變 2.57 ns，資料明明還在 L1。多出來的約 1.5 ns（約 7 cycle）是 **L1 DTLB miss、去 L2 TLB 查**。所以 P-core 的 L1 DTLB 大約 **64 格**，跟影片 [42:00] 的「32–64 格」一致。
- **E-core**：32 個 page 以內 0.84 ns，64 個就跳到 3.34 ns → E-core 的 L1 DTLB **比 64 格小**（介於 32 和 64 之間）。
- page 更多時，4 KB 的時間一直變長（P-core 1 MB 時 11.9 ns，huge page 只要 3.3 ns）：L2 TLB 也裝不下，要做 **page walk**（[§14](#14-tlb-misshardware-或-software-page-walk)）。
- 換成 **huge page** 後，同樣的資料快了 **2.4–3.7 倍**（P-core 256 KB–1 MB、E-core 256 KB–2 MB）：一格 TLB 蓋住 2 MB，TLB 幾乎不會 miss。
- 精確的 TLB 格數 Intel 的公開資料這裡沒有，**待補**；這裡只能從跳躍的位置估。
- ⚠️ 做這個實驗時踩到一個坑：一開始每個 page 用的 line 位置是 `(k × 64) % 4096`，huge page 版本反而**比較慢**（13 ns 以上）。因為 huge page 的實體位址低 21 bit 跟虛擬位址一樣，這種排法讓 1024 條 line 只落在 L2 的約 64 個 set 裡（conflict miss，[§3](#3-三個-c)）；4 KB page 的實體 frame 是亂的，反而分散了。改成 `((k ^ (k >> 5)) & 63) × 64` 讓 set 分散開，才量到真正的 TLB 效果。

---

## 18. 一頁速查表

| 主題 | 重點 |
|---|---|
| AMAT | L1 hit ＋ L1 miss rate × L1 miss penalty，一層層展開；主記憶體一定 hit |
| CPI | CPI_base ＋ f_mem × (AMAT − L1 hit)；主要是 in-order |
| 三個 C | compulsory（第一次）、capacity（太小）、conflict（associativity 不夠） |
| 最佳化 | 小 cache（hit time）、大 block（compulsory）、大 cache／prefetch（capacity）、高 associativity／prefetch／victim cache（conflict） |
| Victim cache | 8–16 格 fully associative；裝常被趕出去的 block；跟 L1 一起查 |
| Write buffer | 合併同一個 block 的寫入；省電、少流量 |
| Critical word first | 先傳要的那 4 byte；early restart |
| Memory map | text、data、bss、heap ↑、stack ↓；32-bit Linux 上面 1 GB 給 kernel |
| 兩個問題 | overlap（互寫）、size（RAM 不夠） |
| Virtual memory | virtual → MMU → physical；4 KB page ↔ 4 KB frame；不同 process 不共用 frame（除非 shmem） |
| Swap space | 主記憶體 ＋ swap ≥ 需求；page replacement 由 OS 做 |
| Page table | virtual page → frame；2 層或多層 |
| TLB | page table 的 cache；32–64（課本 32–128）格；99.5% 命中；ITLB／DTLB、兩層 |
| Page walk | 硬體（x86：CR3）或軟體（OS） |
| 新 frame | 清成 0（別人的密碼）；Linux 讀時對到 zero page |
| 實測 | fork 後同位址不同值；第一次寫 870 ns/page（清 0）；P-core L1 DTLB 約 64 格；huge page 快 2.4–3.7 倍 |

---

## 19. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Cache 的基本設計、本機的 cache | [第 21 支](lecture21_note.md) |
| 大學部的 AMAT、三個 C、victim cache（含本機實測） | [Basic 第 36 支](../basic/lecture36_note.md) |
| Page fault 當作例外、precise exception | [第 13 支](lecture13_note.md) |
| SRAM、CAM 的電路 | [第 23 支](lecture23_note.md) |
| CACTI | [第 24 支](lecture24_note.md) |
| VIPT cache（同時用 virtual 和 physical address）、MSHR、non-blocking cache | [第 25 支](lecture25_note.md) |
| Prefetcher | [第 26 支](lecture26_note.md) |
| Swap 所在的 flash、SSD | [第 44 支](lecture44_note.md)（Ch10） |
| 安全：side channel、ORAM | [第 53 支](lecture53_note.md)、[第 54 支](lecture54_note.md)（Ch13） |

---

## 附：本講一句話總結

> **AMAT 等於 L1 hit time 加上 L1 miss rate 乘 miss penalty（一層一層展開到主記憶體），CPI 則多出 f_mem ×（AMAT − L1 hit time）；miss 分成 compulsory、capacity、conflict 三個 C，對應用大 block、大 cache 或 prefetch、高 associativity 或 victim cache 處理，再加上合併寫入的 write buffer 和先傳需要那個 word 的 critical word first；virtual memory 讓每個 process 以為自己擁有整個 memory map（text、data、bss、heap、stack），由 MMU 把 4 KB 的 virtual page 翻譯到 4 KB 的 physical frame，解決不同 process 互寫（overlap）和 RAM 不夠（size，用 swap space 補）兩個問題，對應存在多層的 page table，用命中率 99.5% 的 TLB 快取，TLB miss 時由硬體（x86 的 CR3）或 OS 走 page table，新的 frame 一定先清成 0。本機實測：fork 之後同一個虛擬位址兩邊看到不同的值，第一次寫一個 page 要約 870 ns（Linux 配一個清成 0 的 frame，只讀的話則先對到共用的 zero page），P-core 的 L1 DTLB 大約 64 格，換成 2 MB 的 huge page 後同樣的存取快了 2.4–3.7 倍。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 7: Caches (Part II) - Optimizations and Virtual Memory 的 whisper 轉錄稿（本機 `large-v3-turbo`，50:58）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch7 §7.1.6–7.2（p.268–277）、Linux v6.2.12 `mm/memory.c` 整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4）結果。*
