# Operating Systems: A Linux Kernel-Oriented Approach 第三十一講完整筆記：Reverse Mapping（Chapter 6, Part III）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 31（Chapter 6, Part III：Reverse Mapping）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（47:05，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§6.3–6.3.1（p.314–324）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
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
- [1. 背景：Bloom filter](#1-背景bloom-filter)
  - [1.1 做法](#11-做法)
  - [1.2 【實測】假陽性率隨元素增加](#12-實測假陽性率隨元素增加)
- [2. Reverse mapping 的問題](#2-reverse-mapping-的問題)
  - [2.1 為什麼需要](#21-為什麼需要)
  - [2.2 壞主意：每個 page 一個串列](#22-壞主意每個-page-一個串列)
  - [2.3 拆成兩個問題](#23-拆成兩個問題)
- [3. 問題一：page → VMA](#3-問題一page--vma)
  - [3.1 anon_vma：加一層 stub](#31-anon_vma加一層-stub)
  - [3.2 fork 和 COW 讓關係變成多對多](#32-fork-和-cow-讓關係變成多對多)
  - [3.3 anon_vma_chain：連接節點](#33-anon_vma_chain連接節點)
  - [3.4 【原始碼】三個結構](#34-原始碼三個結構)
  - [3.5 例子：fork、COW、再 fork](#35-例子forkcow再-fork)
  - [3.6 【原始碼】`page->mapping` 的最低位元](#36-原始碼page-mapping-的最低位元)
- [4. 問題二：VMA 裡的哪個虛擬位址](#4-問題二vma-裡的哪個虛擬位址)
  - [4.1 index](#41-index)
  - [4.2 VMA 被搬動、縮小之後](#42-vma-被搬動縮小之後)
  - [4.3 【原始碼】`linear_page_index` 與 `vma_pgoff_address`](#43-原始碼linear_page_index-與-vma_pgoff_address)
- [5. 為什麼用紅黑樹（區間樹）](#5-為什麼用紅黑樹區間樹)
- [6. 【原始碼】整個流程：`rmap_walk_anon`](#6-原始碼整個流程rmap_walk_anon)
- [7. 【課本補充】檔案對應的 page](#7-課本補充檔案對應的-page)
- [8. 【實測】fork 之後的共享與 COW](#8-實測fork-之後的共享與-cow)
- [9. 一頁速查表](#9-一頁速查表)
- [10. 與後續講次的連結](#10-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

[00:31] 頁面管理的兩個主題：

1. **Reverse mapping（反向對應）**：給一個 **physical page**，找出**所有**對應到它的虛擬 page（跨 process）。本講。
2. **MGLRU**：Linux 的頁面置換演算法。下一講。

[01:00] 要懂置換，需要兩個背景：**Bloom filter** 和 **reverse mapping**。

---

## 1. 背景：Bloom filter

### 1.1 做法

[01:00] **Bloom filter（布隆過濾器）**用來判斷**集合成員**：x 是不是集合 S 的元素？

[01:00]–[01:32] 它是**機率性**的資料結構：

| 回答 | 可信嗎 |
|---|---|
| 「是」 | **不一定**：可能其實不在（**假陽性，false positive**） |
| 「不是」 | **一定**不在（**不會有假陰性，false negative**） |

[02:01] 做法：

```
m 個位元的位元向量：  0 0 1 0 0 1 0 0 0 1 0 1 0 0 0 ...

加入 key：用 4 個不同的雜湊函數算出 4 個位置，把這 4 個位元都設成 1（已經是 1 就不動）
查詢 key′：同樣算出 4 個位置，4 個都是 1 → 「可能在」；任一個是 0 → 「一定不在」
```

[02:32]–[03:32] 為什麼會有假陽性？K 的 4 個位置，可能剛好被 K′、K″ 的位置蓋滿了。K 根本沒加過，4 個位元卻都是 1。

為什麼不會有假陰性？K 加過的話，它的 4 個位元一定是 1，不可能出現 0。

[03:32] 加的元素越多，1 越多，假陽性越多。

[03:32]–[04:00] **很難刪除**：清一個位元可能影響別的元素。可以做帶計數器的版本，但很少用得上。所以通常是**定期整個重設**。

[04:00] 它非常快，用得很廣。Linux kernel 也用得不少（下一講的 MGLRU）。

【課本補充】課本：Bloom filter 在附錄 C.6.1。隨著元素增加，假陽性機率上升，到某個程度就要重設（清掉所有項目）。

### 1.2 【實測】假陽性率隨元素增加

`bloom.py`：m = 8192 位元、k = 4 個雜湊函數。加入 n 個元素，再拿 2 萬個**從來沒加過**的元素去查：

```
m=8192 bits, k=4, n= 200: bits set   9.4%  false negatives=0  false positive rate= 0.00%
m=8192 bits, k=4, n= 800: bits set  32.5%  false negatives=0  false positive rate= 1.12%
m=8192 bits, k=4, n=1600: bits set  54.1%  false negatives=0  false positive rate= 8.76%
m=8192 bits, k=4, n=3200: bits set  79.0%  false negatives=0  false positive rate=39.12%
```

- **假陰性永遠是 0**：加過的元素每一個都查得到。
- **假陽性**隨 n 快速上升：1 的比例從 9% 到 79%，假陽性從幾乎 0 到 39%。
- 理論值約 (1 − e^{−kn/m})^k：n = 800 時 ≈ (1 − e^{−0.39})⁴ ≈ 1.1%，跟實測一致。
- 所以要定期重設。

---

## 2. Reverse mapping 的問題

### 2.1 為什麼需要

[04:00] 某個 page frame 要被**踢出**記憶體。從它的 `struct page`，要找出**所有**指向它的 PTE（跨 process），一一改掉，否則那些 process 會存取到錯的資料。

【課本補充】課本的例子：physical page P 同時被 process A 和 B 對應。P 被 swap 出去（或權限改變）時，要先找出 A 和 B，再去改它們的頁表。

課本也提醒：就算沒用共享記憶體，Linux 裡的 page 共享也**很多**，因為 process 是 fork 出來的，一開始所有 page 都以 COW 模式共享；除非之後 exec 載入新的 page，否則會留下不少共享。

### 2.2 壞主意：每個 page 一個串列

[04:32]–[05:31] 在 `struct page` 裡放一個**串列**，記錄所有指向它的 PTE。不好，因為：

- 串列**不知道會多長**：共享很多時就很長。
- [05:31] **重複**：共享是以 folio 為單位的。10 個 process 共享一個 folio，folio 裡**每個** page 都有一個 10 節點的串列，結構完全一樣，卻存了很多份。
- [07:01]–[07:30] fork 時要到每個 page 的串列加一個節點；COW 時又要拿掉。無上限的串列很糟。

[06:00]–[06:30] 目標：

- **省空間**。
- **省時間**：特別是 fork、COW 這種線上操作，要做的工作越少越好。
- 反向查找要**很快**。

【課本補充】課本把它拆成兩個問題：

| 問題 | 意思 |
|---|---|
| **Mapping problem** | 每個 page 要有一份虛擬 page 的串列 |
| **Policy problem** | 要能對一群 page 套用共同的政策（例如權限） |

先解決 policy problem，mapping problem 自然就解決了。

### 2.3 拆成兩個問題

[07:30]–[09:00] 改成：給 `struct page`（或 PFN），先找到一串 **VMA**。

複習 VMA（第 9–10 講）：`vm_area_struct`，一段**連續的虛擬記憶體**，主要有起點和終點。每個 VMA 屬於一個 `mm_struct`，`mm_struct` 屬於一個 `task_struct`。

[09:00] 就像實體記憶體分成 zone 和 section，虛擬記憶體分成一串 VMA。

[09:31]–[10:00] 兩個問題：

| | 問題 | 教授說 |
|---|---|---|
| 問題一 | `struct page` → 有哪些 **VMA** 可能對應到它 | 要 10 張投影片 |
| 問題二 | 在某個 VMA 裡，是**哪個虛擬 page** 對應到它 | 一張投影片 |

[10:33] 用 VMA 組織的好處：可以對整個 VMA 套用**共同政策**。教授說：「為什麼要組織東西？永遠都是為了對一組東西套用共同的政策。」

---

## 3. 問題一：page → VMA

### 3.1 anon_vma：加一層 stub

[11:03] 最簡單的想法：每個 page 指向它的 VMA。不行：

- [11:30] 一個 page 被**多個** VMA（跨 process）共享，只指一個不夠。指一個再串一個串列……也沒解決什麼。
- 如果那個 task 結束了，就要到**每一個** page 去重新指定 VMA，O(N) 的工作。
- 【課本】VMA 常常被**切開、合併**：改權限、搬動區域、對某段套用不同政策（預取、swap 優先權、變成 huge page）。fork 時還會被複製。

[12:00] 解法：讓 page 指向一個**不會變的 stub（替身）**：

```
 struct page ──mapping──┐
 struct page ──mapping──┼──▶ anon_vma（stub）──▶ 一群 VMA
 struct page ──mapping──┘
```

[12:30]–[13:00]

- page 的 `mapping` 欄位**永遠不用改**。要改都在 stub 那裡改。
- 一百萬個 page 指向同一個 VMA，就讓它們都指向一個 **anon_vma（匿名 VMA）**。在 stub 改一次，一百萬個 page 都生效。
- 代價：反向查找時多走幾步，可以接受。

[13:32] 避免了 O(N) 的工作，不用每次都去走訪每個 `struct page`。

【課本補充】課本說這是 kernel 的標準模式：**虛擬化**，在中間加一層資料結構。VMA 的數量遠少於 page，所以只動 anon_vma 和 VMA 之間的關係很快。

### 3.2 fork 和 COW 讓關係變成多對多

[14:00]–[15:01] **fork**：一開始沒有共享時，VMA 和 anon_vma 是一對一。fork 之後：

```
父 VMA ──┐
         ├──▶ anon_vma ◀── 所有 physical page
子 VMA ──┘
```

同一段虛擬位址，父、子各一個 VMA，指向**同一個** anon_vma。還沒有 COW，父子共享全部實體位址空間。

> **2 個 VMA ↔ 1 個 anon_vma**（多對一）

[15:01]–[16:30] **COW**：子 process 寫了一個共享的 page → 複製一份新的。新 page 不能指向原本的 anon_vma（它是子 process 私有的），要指向**子 process 自己的** anon_vma。

```
子 VMA ──▶ 父的 anon_vma  ◀── 還沒被寫過的 page（繼承的）
      └──▶ 子的 anon_vma  ◀── COW 出來的新 page
```

子 process 的同一個 VMA，現在關聯到**兩個** anon_vma。子的 anon_vma 一開始是空的，COW 越多，指向它的 page 越多。

> **1 個 VMA ↔ 2 個 anon_vma**（一對多）

[17:01] 多對一加一對多 = **多對多**。

【課本補充】課本問：子 process 的 VMA 有 1024 個 page，寫了第 500 個，要不要把 VMA 切成 0–499、500、501–1023 三段？**不要**：COW 模式下很多 page 都會被寫，一直切會產生一大堆 VMA。最好的辦法是什麼都不做：VMA 保持原樣，接受它的 page 可能指向不同的 anon_vma。

> **課本 Point 6.3.1**：
> - 每個 process 對一段虛擬位址有自己的 VMA，存在 `mm_struct` 的 maple tree 裡（第 10 講）。
> - 一個 physical page 只指向**一個** anon_vma（stub）。
> - fork 讓多個 VMA（跨 process）指向同一個 anon_vma。
> - COW 讓 page 換 anon_vma，但 VMA 保持不變。
> - VMA 會因使用者的動作被切開或合併。
> - 總結：VMA 和 anon_vma 是**多對多**。

### 3.3 anon_vma_chain：連接節點

[17:31] 遇到多對多，電腦科學的標準做法：在圖上**疊一棵樹**（像 DFS、BFS、最小生成樹那樣）。

[18:00] 在 VMA 和 anon_vma 之間加一個**連接節點**：**anon_vma_chain（AVC）**。

```
VMA ───── AVC ───── anon_vma
     一對一     一對一
```

[18:30]–[19:30]

- 每個 VMA 有一個**私有的** anon_vma，可以直接指向它（`vma->anon_vma`）。
- 但它可能還關聯到別的 anon_vma（祖先的），透過 AVC 連接。
- 為了一致，VMA 和自己私有的 anon_vma 之間**也**用一個 AVC 連。
- 每個 AVC 剛好連**一個** VMA 和**一個** anon_vma。

[20:01]–[22:30] **為什麼叫 chain（鏈）**：一個 VMA 對應的 physical page 可能來自：

- 自己建立的（自己的 anon_vma），
- 從父 process 繼承的（父的 anon_vma），
- 從祖父 process 繼承的：父 process 繼承了祖父的 page、父和自己都沒寫過，自己用的就是祖父的 page……

每個來源都要一個 AVC。**同一個 VMA 的所有 AVC 串成一條串列**，所以叫 chain：

```
VMA ── AVC ── AVC ── AVC        （同一個 VMA 的 AVC 串列：same_vma）
        │      │      │
     自己的   父的    祖父的
    anon_vma anon_vma anon_vma
```

[23:31]–[25:31] **另一個方向**：一個 process 被 fork 很多次，很多子 process 的 VMA 都要連到父 process 的 anon_vma。

- 一個指向父 anon_vma 的 page 要被踢掉，要找到**所有**可能對應它的 VMA（跨 process）。
- anon_vma 有一棵**紅黑樹**，存著所有連到它的 AVC（跨 process）。
- AVC 只是連接器，有了 AVC 就能找到另一端的 VMA。

```
                      anon_vma
                         │ rb_root
                    ┌────┴────┐
                   AVC       AVC          （連到同一個 anon_vma 的 AVC：紅黑樹）
                    │         │
                 父的 VMA   子的 VMA
```

[25:01] 反向查找的流程：

1. page → 它的 anon_vma。
2. anon_vma → 紅黑樹的根。
3. 走過樹裡的每個 AVC（分散在各個 process）。
4. 每個 AVC → 一個 VMA。
5. 告訴每個 VMA：「PFN 為 P 的 page 要被踢掉了，你有任何虛擬位址對應到它的話，請失效。」

[25:31]–[26:00]

| 方向 | 結構 | 範圍 |
|---|---|---|
| 同一個 VMA 的所有 AVC | **串列**（`same_vma`） | 同一個 process |
| 連到同一個 anon_vma 的所有 AVC | **紅黑樹**（`rb_root`） | 跨 process |

> **課本 Point 6.3.2**：一個 task 的虛擬記憶體區域用 VMA 表示。對應到它的 physical page 可能由不同的 process（自己和祖先）配置，所以指向不同的 anon_vma。用 AVC 把它們全部連在一起、關聯到目前的 VMA。

【課本補充】課本說 kernel 開發者有考慮過「每個 anon_vma 一個 VMA 指標陣列、每個 VMA 一個 anon_vma 指標陣列」，但有**擴展性**問題：企業級應用共享很多，串列很長；改 VMA 時要鎖住串列的一部分，並行的修改就受限了。多一層 AVC 可以把 VMA 和 anon_vma 在邏輯上分開，process 改 VMA 時不用管它跟 anon_vma 的連接。

### 3.4 【原始碼】三個結構

`include/linux/rmap.h:82-91`

```c
struct anon_vma_chain {
	struct vm_area_struct *vma;
	struct anon_vma *anon_vma;
	struct list_head same_vma;   /* locked by mmap_lock & page_table_lock */
	struct rb_node rb;			/* locked by anon_vma->rwsem */
	unsigned long rb_subtree_last;
#ifdef CONFIG_DEBUG_VM_RB
	unsigned long cached_vma_start, cached_vma_last;
#endif
};
```

| 欄位 | 說明 |
|---|---|
| `vma`, `anon_vma` | 這個 AVC 連接的兩端 |
| `same_vma` | 同一個 VMA 的 AVC 串列（第 14 講的 `list_head`）。用 `mmap_lock` 保護 |
| `rb` | 在 anon_vma 的紅黑樹裡的節點。用 `anon_vma->rwsem` 保護 |
| `rb_subtree_last` | 這棵子樹裡最大的區間終點：**區間樹**用的（§5） |

兩個方向用**不同的鎖**：改 VMA 的串列和改 anon_vma 的樹可以並行。這就是課本說的擴展性。

`include/linux/rmap.h:31-67`（節錄）

```c
struct anon_vma {
	struct anon_vma *root;		/* Root of this anon_vma tree */
	struct rw_semaphore rwsem;	/* W: modification, R: walking the list */
	...
	atomic_t refcount;
	...
	unsigned long num_children;
	...
	unsigned long num_active_vmas;

	struct anon_vma *parent;	/* Parent of this anon_vma */
	...
	/* Interval tree of private "related" vmas */
	struct rb_root_cached rb_root;
};
```

| 欄位 | 說明 |
|---|---|
| `root`, `parent` | anon_vma 本身也排成一棵樹（父子 process 的關係），指向根和父 |
| `rwsem` | 讀寫 semaphore：修改時寫鎖、走訪時讀鎖（第 20–22 講） |
| `refcount` | 參照計數 |
| `num_children`, `num_active_vmas` | 用來決定 fork 時能不能**重複使用**現有的 anon_vma，不必每次新建（課本說的空間最佳化） |
| `rb_root` | 「相關 VMA 的區間樹」：§3.3 的那棵紅黑樹 |

課本說它「其實不存任何狀態，是一個 dummy 節點」：主要欄位就是指向別的結構的指標和鎖。

VMA 裡的兩個欄位：

`include/linux/mm_types.h:567-575`（節錄）

```c
	struct list_head anon_vma_chain; /* Serialized by mmap_lock &
	...
	struct anon_vma *anon_vma;	/* Serialized by page_table_lock */
	...
	unsigned long vm_pgoff;		/* Offset (within vm_file) in PAGE_SIZE
```

- `anon_vma_chain`：AVC 串列的頭。
- `anon_vma`：自己**私有**的 anon_vma。
- `vm_pgoff`：§4 會用到。

### 3.5 例子：fork、COW、再 fork

[26:00]–[28:30] **第一次 fork**（課本 Figure 6.20）：

```
 父 process                              子 process
  VMA ── AVC₁ ──┐                      ┌── VMA ── AVC₃ ── 子的 anon_vma
                ▼                      │    │
            父的 anon_vma ◀── AVC₂ ────┘    │（同一條 same_vma 串列）
                │ 紅黑樹 = {AVC₁, AVC₂}
                ▼
              page（共享）
```

- [27:00] fork 不是實體複製，複製的是**對應**：父的每個 VMA，子都有一個對應的 VMA。
- [27:30] 子的 VMA 透過 **AVC₂** 連到**父的** anon_vma。父 anon_vma 的紅黑樹裡現在有 AVC₁（父的）和 AVC₂（子的）。
- 這個 page 被踢掉時，透過紅黑樹找到 AVC₁、AVC₂，就找到父和子的 VMA。
- [28:30] 子是新 process，也有自己的私有 anon_vma 和 AVC₃。一開始**沒有** page 指向它。

[29:32]–[30:00] **子 process 寫了一個共享的 page**（COW，課本 Figure 6.22）：新的 page 指向**子的** anon_vma。這個新 page 被踢掉時，走子的 anon_vma 的紅黑樹，只會找到子的 VMA。

[30:00]–[33:32] **子 process 再 fork**（孫 process，課本 Figure 6.23）：

[30:31] 好處：fork 只修改**子** process（新的那一方），父 process 什麼都不用改。

孫 process 的同一個 VMA 有**三個**來源：

| 來源 | 連接 |
|---|---|
| 孫自己建立的 | 孫的私有 anon_vma + AVC |
| 從子繼承的 | AVC → **子的** anon_vma |
| 從父（祖父）繼承的 | AVC → **父的** anon_vma |

- [32:00] 子的 anon_vma 的紅黑樹 = {子的 AVC, 孫的 AVC}：指向它的 page 被子和孫共享，不被父共享。
- [33:01] 父的 anon_vma 的紅黑樹 = {父的 AVC, 子的 AVC, 孫的 AVC}：三個 VMA。

[33:32] 一直 fork 下去會形成相當複雜的結構，但原則就是：一個 VMA 有多少個來源就有多少個 AVC；一個 anon_vma 對應多少個 VMA，紅黑樹就有多少個 AVC。

【課本補充】重複 fork 會產生很多 AVC 和 anon_vma，空間會膨脹。現代 kernel 有時會**重複使用**現有的 anon_vma（加上 AVC），不給子 process 建新的。很複雜，課本不講。（就是上面 `num_children`、`num_active_vmas` 的用途。）

### 3.6 【原始碼】`page->mapping` 的最低位元

page 怎麼指向 anon_vma？用 `struct page` 的 `mapping` 欄位，但有個技巧：

`include/linux/page-flags.h:616-636`（節錄）

```c
/*
 * On an anonymous page mapped into a user virtual memory area,
 * page->mapping points to its anon_vma, not to a struct address_space;
 * with the PAGE_MAPPING_ANON bit set to distinguish it.  See rmap.h.
 ...
 * Please note that, confusingly, "page_mapping" refers to the inode
 * address_space which maps the page from disk; whereas "page_mapped"
 * refers to user virtual address space into which the page is mapped.
 */
#define PAGE_MAPPING_ANON	0x1
#define PAGE_MAPPING_MOVABLE	0x2
#define PAGE_MAPPING_KSM	(PAGE_MAPPING_ANON | PAGE_MAPPING_MOVABLE)
#define PAGE_MAPPING_FLAGS	(PAGE_MAPPING_ANON | PAGE_MAPPING_MOVABLE)
```

- `mapping` 是一個指標。結構都對齊到至少 4 bytes，最低兩個位元一定是 0，就拿來存旗標（第 22 講 mutex 的 owner 也是這招）。
- **匿名 page**：`mapping` = anon_vma 的位址 **| 1**（`PAGE_MAPPING_ANON`）。
- **檔案 page**：`mapping` = 檔案的 `address_space` 位址，最低位元是 0（§7）。
- 一個欄位，看最低位元就知道指向哪種結構。

---

## 4. 問題二：VMA 裡的哪個虛擬位址

### 4.1 index

[34:32] 找到了 VMA，但還要知道 VMA 裡**哪一個虛擬 page** 對應到這個 PFN。

[35:00] 頁表是虛擬 → 實體，**不是**反過來。暴力法：VMA 有 N 個 page，一個個翻譯、比對，O(N)，不實際。要 **Θ(1)**。

[35:30]–[37:01] 想法：

- page **第一次**被配置時，一定是配給某個 process 的某個 VMA 裡的第 i 個虛擬 page。
- 把這個 **i** 存在 `struct page` 的 **`index`** 欄位。
- 之後要反向查找：VMA 的起始位址 + i（乘上 page 大小），就是虛擬位址。常數時間。

課本式 6.1：

```
vaddr = v.start_addr + (i << 12)
```

### 4.2 VMA 被搬動、縮小之後

[37:30] fork 時 VMA 會被複製；也可能跟別的 VMA 合併成更大的；可能被**搬動**（一個 process 裡是 100–200，另一個 process 裡變成 400–500）；也可能被切開。

[38:01]–[41:32] 教授的例子：

- 父的 VMA 從位址 1000 開始，100 個項目，偏移 0–99。
- 複製到子 process，子把它搬到從 2000 開始，而且只留偏移 20–50。
- 所以位址 2000–2030 對應原本偏移 20–50 的那些 page，可能就是同一批 physical page。

[41:00] 公式：

```
虛擬位址 = VMA 的起始位址 + (index − VMA 的起始偏移)
例：2000 + (40 − 20) = 2020
```

[42:00]–[42:31] 只要 `index` 存好，**偏移永遠相對於原本的父 VMA**（假設原本是 0 到 99），不管是原本的 VMA，還是被 fork、縮小、擴大的子 VMA，都能用這個公式算出虛擬位址。

[44:02] 例如往前延伸，起始偏移可能是 −10。

課本式 6.2（VMA 只剩第 50–70 頁）：

```
如果 i 不在 50..70 之間 → 這個 VMA 沒有對應
否則 vaddr = v.start_addr + ((i − 50) << 12)
```

### 4.3 【原始碼】`linear_page_index` 與 `vma_pgoff_address`

kernel 存的不是「相對於父 VMA 開頭的 i」，而是更聰明的東西。

page 第一次被對應時設 `index`：

`mm/rmap.c:1172`

```c
	page->index = linear_page_index(vma, address);
```

`include/linux/pagemap.h:844-853`

```c
static inline pgoff_t linear_page_index(struct vm_area_struct *vma,
					unsigned long address)
{
	pgoff_t pgoff;
	if (unlikely(is_vm_hugetlb_page(vma)))
		return linear_hugepage_index(vma, address);
	pgoff = (address - vma->vm_start) >> PAGE_SHIFT;
	pgoff += vma->vm_pgoff;
	return pgoff;
}
```

```
index = (address − vm_start) / 4096 + vm_pgoff
```

匿名 VMA 建立時，`vm_pgoff` 設成什麼？

`mm/mmap.c:2984`

```c
	vma->vm_pgoff = addr >> PAGE_SHIFT;
```

（`mmap.c:1390` 也一樣：匿名 mmap 時 `pgoff = addr >> PAGE_SHIFT`。）

所以對匿名 page：**index = 它當初的虛擬 page 號碼**（虛擬位址 / 4096）。`vm_pgoff` 就是課本說的「VMA 的起始偏移」，只不過單位是「原本的虛擬 page 號碼」。

反過來算：

`mm/internal.h:551-569`

```c
vma_pgoff_address(pgoff_t pgoff, unsigned long nr_pages,
		  struct vm_area_struct *vma)
{
	unsigned long address;

	if (pgoff >= vma->vm_pgoff) {
		address = vma->vm_start +
			((pgoff - vma->vm_pgoff) << PAGE_SHIFT);
		/* Check for address beyond vma (or wrapped through 0?) */
		if (address < vma->vm_start || address >= vma->vm_end)
			address = -EFAULT;
	} else if (pgoff + nr_pages - 1 >= vma->vm_pgoff) {
		/* Test above avoids possibility of wrap to 0 on 32-bit */
		address = vma->vm_start;
	} else {
		address = -EFAULT;
	}
	return address;
}
```

| 部分 | 說明 |
|---|---|
| `vm_start + ((pgoff − vm_pgoff) << PAGE_SHIFT)` | **就是教授的公式**：起始位址 +（index − 起始偏移）× 4096 |
| 超出 `vm_start`–`vm_end` | 這個 VMA 沒有對應這個 page（課本式 6.2 的「不在 50..70 之間」） |
| `else if` | folio 有好幾個 page，只有一部分落在 VMA 裡 |

`mm/internal.h:577-581`

```c
vma_address(struct page *page, struct vm_area_struct *vma)
{
	VM_BUG_ON_PAGE(PageKsm(page), page);	/* KSM page->index unusable */
	return vma_pgoff_address(page_to_pgoff(page), compound_nr(page), vma);
}
```

VMA 被搬動（`mremap`）時，kernel 會調整新 VMA 的 `vm_pgoff`，讓「index − vm_pgoff」仍然算出正確的相對位置，**page 的 index 不用動**。VMA 被切開時，後半段的 `vm_pgoff` 加上前半段的長度。這正是教授說「偏移相對於原本的 VMA」的實作。

（註解提到 KSM：記憶體去重合併的 page 被很多不相關的位址共享，index 沒有意義，要另外處理。）

---

## 5. 為什麼用紅黑樹（區間樹）

[45:01] 為什麼 anon_vma 用紅黑樹存 AVC，不用陣列？

[45:31]–[46:30]

- 紅黑樹是一個 key–value 結構，value 是 AVC。
- **key 是 index**：從 `struct page` 拿到的那個。
- 每個節點存它涵蓋的**偏移範圍**。例如 index = 12：某個 AVC 涵蓋 10–30 → 交給它；另一個 AVC 涵蓋 80–90 → 不用給它，跳過。
- 只搜尋**範圍包含這個 index** 的 AVC，減少搜尋時間和白做的工。

這種樹叫**區間樹（interval tree）**：`struct anon_vma` 的註解就寫著「Interval tree of private "related" vmas」，AVC 的 `rb_subtree_last` 存的是「這棵子樹裡最大的區間終點」，搜尋時整棵子樹的最大終點都比 index 小，就整棵跳過。

---

## 6. 【原始碼】整個流程：`rmap_walk_anon`

[43:03] 整個反向對應的流程：

1. PFN → `struct page`。
2. `struct page` → anon_vma。
3. 透過紅黑樹找到可能包含這個 page 的所有 VMA。
4. 對每個 VMA，用 `index` 和公式算出虛擬位址。

`mm/rmap.c:2413-2451`

```c
static void rmap_walk_anon(struct folio *folio,
		struct rmap_walk_control *rwc, bool locked)
{
	struct anon_vma *anon_vma;
	pgoff_t pgoff_start, pgoff_end;
	struct anon_vma_chain *avc;

	if (locked) {
		anon_vma = folio_anon_vma(folio);
		/* anon_vma disappear under us? */
		VM_BUG_ON_FOLIO(!anon_vma, folio);
	} else {
		anon_vma = rmap_walk_anon_lock(folio, rwc);
	}
	if (!anon_vma)
		return;

	pgoff_start = folio_pgoff(folio);
	pgoff_end = pgoff_start + folio_nr_pages(folio) - 1;
	anon_vma_interval_tree_foreach(avc, &anon_vma->rb_root,
			pgoff_start, pgoff_end) {
		struct vm_area_struct *vma = avc->vma;
		unsigned long address = vma_address(&folio->page, vma);

		VM_BUG_ON_VMA(address == -EFAULT, vma);
		cond_resched();

		if (rwc->invalid_vma && rwc->invalid_vma(vma, rwc->arg))
			continue;

		if (!rwc->rmap_one(folio, vma, address, rwc->arg))
			break;
		if (rwc->done && rwc->done(folio))
			break;
	}

	if (!locked)
		anon_vma_unlock_read(anon_vma);
}
```

| 行 | 步驟 |
|---|---|
| 2421 / 2425 | **page → anon_vma**（`folio_anon_vma` 就是讀 `mapping` 再去掉最低位元），並拿 `rwsem` 讀鎖 |
| 2430-2431 | 這個 folio 的 index 範圍（folio 可能有好幾個 page） |
| 2432-2433 | **在區間樹裡**只走範圍跟 [pgoff_start, pgoff_end] 重疊的 AVC（§5） |
| 2434 | **AVC → VMA** |
| 2435 | **算虛擬位址**：`vma_address`（§4.3 的公式） |
| 2438 | `cond_resched()`：VMA 可能很多，讓別人有機會跑（第 26 講） |
| 2443 | 對每個 (VMA, 位址) 呼叫 `rmap_one`：**這是一個函式指標**，呼叫者決定要做什麼 |

`rmap_walk_control` 用函式指標讓同一個走訪流程有很多用途：

| 用途 | `rmap_one` 做什麼 |
|---|---|
| 回收 page（`try_to_unmap`） | 清掉那個 PTE、換成 swap 項目 |
| 遷移 page（`try_to_migrate`） | 改成指向新的 frame |
| 看最近有沒有被存取（`folio_referenced`） | 讀並清掉 PTE 的 Accessed 位元（第 29 講的 `ptep_test_and_clear_young`）。這是 LRU / MGLRU 判斷 page 冷熱的方法 |

又是第 26 講「用 C 寫物件導向」的模式。

---

## 7. 【課本補充】檔案對應的 page

本講只講**匿名 page**（stack、heap 這種執行中產生的資料）。課本說**檔案對應的 page**（記憶體對映的檔案，由 page cache 管理）比較簡單：

- 用一個集中的結構，存著所有對應到這個檔案的 VMA（一棵樹）。
- 給一個 physical page，可以對應到一個**固定的檔案偏移**。
- 用這個偏移找出可能包含它的 VMA。

對應到原始碼：檔案 page 的 `mapping` 指向檔案的 `address_space`（最低位元是 0，§3.6），`index` 就是**在檔案裡的 page 偏移**，`address_space` 有一棵 `i_mmap` 區間樹存著所有對應這個檔案的 VMA。`vm_pgoff` 是這個 VMA 從檔案的哪一頁開始對應，所以一樣用 `vm_start + (index − vm_pgoff) × 4096` 算出位址。匿名 page 的設計（index = 原本的虛擬 page 號碼、`vm_pgoff` = 起始虛擬 page 號碼）就是**模仿**檔案 page，讓兩者共用同一條公式。

---

## 8. 【實測】fork 之後的共享與 COW

`cow.c`：父 process 配置並寫滿 64 MB 匿名記憶體，fork，子 process 再寫其中 16 MB。讀 `/proc/<pid>/smaps` 裡這塊區域：

```
parent before fork (pid 220865):
   Rss:            65536 kB
   Shared_Dirty:   0 kB
   Private_Dirty:  65536 kB
   Anonymous:      65536 kB
child right after fork (pid 220868):
   Rss:            65536 kB
   Shared_Dirty:   65536 kB
   Private_Dirty:  0 kB
   Anonymous:      65536 kB
child after writing 16 MB (pid 220868):
   Rss:            65536 kB
   Shared_Dirty:   49152 kB
   Private_Dirty:  16384 kB
   Anonymous:      65536 kB
```

| 時間點 | 共享 | 私有 | 對應到 rmap |
|---|---|---|---|
| fork 前 | 0 | 64 MB | 所有 page 只被父的 VMA 對應，指向父的 anon_vma |
| fork 後 | **64 MB** | 0 | 同一批 page 現在被**兩個** VMA 對應（父的、子的），都透過父的 anon_vma（§3.5 第一次 fork） |
| 子寫了 16 MB | 48 MB | **16 MB** | 那 16 MB 被 COW 成新 page，指向**子的** anon_vma，只有子對應；剩下 48 MB 仍然共享 |

- kernel 怎麼知道一個 page 是「共享」還是「私有」？看它被**幾個 PTE** 對應（`struct page` 的 `_mapcount`）：大於 1 就是共享。
- 子 process 的 Rss 一直是 64 MB，但實體記憶體只多用了 16 MB。

---

## 9. 一頁速查表

### 9.1 Bloom filter

m 位元、k 個雜湊。加入：設 k 個位元；查詢：k 個都是 1 → 可能在。有假陽性、沒有假陰性；不能刪除，定期重設。假陽性 ≈ (1 − e^{−kn/m})^k。

### 9.2 Reverse mapping 的結構

| 結構 | 角色 |
|---|---|
| `page->mapping` | 匿名：anon_vma \| 1；檔案：address_space |
| `page->index` | 匿名：當初的虛擬 page 號碼；檔案：檔案內的 page 偏移 |
| `anon_vma` | stub；`rb_root` = 相關 AVC 的區間樹；`root`、`parent` 組成 anon_vma 樹 |
| `anon_vma_chain` | 連一個 VMA 和一個 anon_vma；`same_vma` 串列 + `rb` 樹節點 |
| `vma->anon_vma` | 私有的 anon_vma |
| `vma->anon_vma_chain` | 這個 VMA 的 AVC 串列 |
| `vma->vm_pgoff` | 起始偏移 |

### 9.3 兩個問題

| 問題 | 解法 | 時間 |
|---|---|---|
| page → 哪些 VMA | page → anon_vma → 區間樹 → AVC → VMA | 只走範圍重疊的節點 |
| VMA 裡哪個位址 | `vm_start + (index − vm_pgoff) << 12` | Θ(1) |

### 9.4 關係

| 事件 | 結果 |
|---|---|
| fork | 2 VMA ↔ 1 anon_vma；子多一個 AVC 連到父的 anon_vma，還有自己的私有 anon_vma |
| COW | 新 page 指向子的 anon_vma；VMA 不切開 → 1 VMA ↔ 2 anon_vma |
| 總結 | 多對多，用 AVC 拆開 |

### 9.5 本講看過的原始碼位置

| 位置 | 內容 |
|---|---|
| `include/linux/rmap.h:31-67` | `struct anon_vma` |
| `include/linux/rmap.h:82-91` | `struct anon_vma_chain` |
| `include/linux/mm_types.h:567-575` | VMA 的 `anon_vma_chain`、`anon_vma`、`vm_pgoff` |
| `include/linux/page-flags.h:616-636` | `PAGE_MAPPING_ANON`：`mapping` 最低位元 |
| `mm/rmap.c:1172` | `page->index = linear_page_index(...)` |
| `include/linux/pagemap.h:844-853` | `linear_page_index` |
| `mm/mmap.c:2984` | 匿名 VMA 的 `vm_pgoff = addr >> PAGE_SHIFT` |
| `mm/internal.h:551-581` | `vma_pgoff_address`、`vma_address` |
| `mm/rmap.c:2413-2451` | `rmap_walk_anon` |

---

## 10. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **Bloom filter** | **第 32 講**：MGLRU 用它記住哪些頁表區域值得掃描 |
| **`rmap_walk` + `folio_referenced` 判斷冷熱、`try_to_unmap` 回收** | **第 32 講**：MGLRU、thrashing |
| **fork、COW** | 回顧第 12 講 |
| **VMA、maple tree** | 回顧第 9–10 講 |

（講次對應依播放清單標題推估。）

---

## 附：本講一句話總結

> **要踢掉一個 physical page，必須找出所有對應到它的 PTE，這就是 reverse mapping。在每個 page 放串列太浪費也太慢，所以拆成兩個問題：第一，page 經由 `mapping` 欄位（最低位元標成匿名）指向一個不會變的 stub `anon_vma`；因為 fork（多個 VMA 共享一個 anon_vma）和 COW（一個 VMA 的 page 分屬自己和祖先的 anon_vma）讓兩者變成多對多，所以中間再加 `anon_vma_chain`：同一個 VMA 的 AVC 串成串列，連到同一個 anon_vma 的 AVC 放在它的區間樹（紅黑樹）裡，只走範圍包含這個 page 的節點。第二，page 的 `index` 存著它當初的虛擬 page 號碼，任何 VMA 都能用 `vm_start + (index − vm_pgoff) × 4096` 在常數時間算出虛擬位址，VMA 被搬動或切開時只要調整 `vm_pgoff`。`rmap_walk_anon` 把這兩步串起來，再用函式指標決定對每個對應做什麼（回收、遷移、讀 Accessed 位元）。實測 fork 後 64 MB 全部共享，子寫了 16 MB 就變成 48 MB 共享、16 MB 私有；Bloom filter 則永遠沒有假陰性，但元素越多假陽性越高（3200 個元素時達 39%），要定期重設。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 31（Chapter 6, Part III）錄影逐字稿（YouTube 自動字幕，47:05）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§6.3–6.3.1，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
