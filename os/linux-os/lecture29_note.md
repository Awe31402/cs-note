# Operating Systems: A Linux Kernel-Oriented Approach 第二十九講完整筆記：傳統的頁面置換演算法（Chapter 6, Part I）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 29（Chapter 6, Part I：Traditional Page Replacement Algorithms）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（42:34，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）第 6 章開頭與 §6.1（p.281–297）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
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

- [0. 第 6 章的地圖](#0-第-6-章的地圖)
- [1. Base–limit：沒有虛擬記憶體的年代](#1-baselimit沒有虛擬記憶體的年代)
  - [1.1 做法](#11-做法)
  - [1.2 外部碎片與四種挑洞的方法](#12-外部碎片與四種挑洞的方法)
- [2. 分頁：frame 與 physical page](#2-分頁frame-與-physical-page)
- [3. Stack distance](#3-stack-distance)
- [4. 最佳置換演算法（OPT）](#4-最佳置換演算法opt)
- [5. Stack 性質](#5-stack-性質)
- [6. LRU 與它的近似](#6-lru-與它的近似)
  - [6.1 純 LRU 做不到](#61-純-lru-做不到)
  - [6.2 用存取位元近似](#62-用存取位元近似)
  - [6.3 ⚠️【原始碼】x86 的存取位元是硬體設的](#63-️原始碼x86-的存取位元是硬體設的)
  - [6.4 WS-Clock](#64-ws-clock)
  - [6.5 WS-Clock 第二次機會](#65-ws-clock-第二次機會)
  - [6.6 【原始碼】Linux 傳統的 active / inactive 串列](#66-原始碼linux-傳統的-active--inactive-串列)
- [7. FIFO 與 Belady 異常](#7-fifo-與-belady-異常)
  - [7.1 例子：4 個 frame 10 次缺頁、3 個 frame 9 次](#71-例子4-個-frame-10-次缺頁3-個-frame-9-次)
  - [7.2 為什麼會這樣](#72-為什麼會這樣)
- [8. 【實測】四種演算法、五種記憶體大小](#8-實測四種演算法五種記憶體大小)
- [9. Working set 與 thrashing](#9-working-set-與-thrashing)
- [10. 【課本補充＋實測】共享記憶體 IPC](#10-課本補充實測共享記憶體-ipc)
- [11. 一頁速查表](#11-一頁速查表)
- [12. 與後續講次的連結](#12-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 第 6 章的地圖

[00:31] 第 6 章講**記憶體系統**：

| 部分 | 內容 | 講次 |
|---|---|---|
| 傳統啟發式 | base–limit、頁面置換演算法、working set | 第 29 講（本講） |
| 虛擬記憶體 | kernel 的記憶體地圖、頁表、page 與 folio、zone | 第 30 講 |
| 頁面管理 | reverse mapping、MGLRU、thrashing | 第 31–32 講 |
| kernel 的記憶體配置 | buddy、SLAB、SLUB | 第 33 講 |

【課本補充】課本開頭點出幾個難題：一個 physical page 被很多 process 共享（fork、copy-on-write），要追蹤所有共享者；kernel 的記憶體不能像使用者空間那樣隨意配置，要更確定性地管理，反而比較像一個聰明版的 base–limit 方案。

---

## 1. Base–limit：沒有虛擬記憶體的年代

### 1.1 做法

[01:02] 還沒有虛擬記憶體時，只能直接用實體位址，加一點變化：

```
實體記憶體
┌────────┐
│ 已配置  │
├────────┤ ← base
│ process │   CPU 發出位址 V → 實際位址 = base + V
│ 的區域   │   要 < limit，否則越界
├────────┤ ← base + limit
│   洞    │
├────────┤
│ 已配置  │
└────────┘
```

[01:30] 每個 process 事先估計要多少記憶體，配給它一塊**連續**的實體區域：

- **base 暫存器**：區域的開頭。
- [02:00] 實際位址 = base + V。可用範圍從 base 到 base + limit。
- 某種意義上位址還是「虛擬」的，只是轉換很簡單：加上 base。

[02:00] 超過 limit 就可能闖進別人的區域，所以不准超過。

### 1.2 外部碎片與四種挑洞的方法

[02:32] 區域之間沒用到的空間叫**洞（hole）**。

[02:32]–[03:00] 比較兩種碎片：

> **課本 Definition 6.1.1**：
> - **內部碎片（internal fragmentation）**：分頁系統裡，一個 page 內沒用到的空間。每頁最多浪費 4 KB（第 4 講看過）。
> - **外部碎片（external fragmentation）**：base–limit 系統裡，區域之間留下的洞。

[03:00] 教授：外部碎片**糟糕得多**。之後如果來一個需要記憶體不多的 process，也許能塞進洞裡；否則這個洞就永遠浪費了。

[03:31] 除非做**壓縮（compaction）**：把所有區域往一起搬，合成一個大洞。但這非常貴，不想做。

[03:31]–[04:30] 新的要求需要 R 這麼多空間，好幾個洞都夠大，挑哪一個？

| 方法 | 挑哪個洞 |
|---|---|
| **Best fit** | 剛好比 R 大的**最小**洞 |
| **Worst fit** | **最大**的洞 |
| **Next fit** | 從上次配置的地方往後找，第一個夠大的（到底就繞回開頭） |
| **First fit** | 從頭開始找，第一個夠大的 |

[04:30] 也可以用複雜的資料結構加速。但這種方法沒有分頁那麼有彈性，所以一般不用。

[04:30] 不過教授說：**要管理實體記憶體，就是這樣做**，所以 kernel 仍然用它的一個（複雜得多的）版本。（第 33 講的 buddy allocator。）

【課本補充】課本：這些啟發式對不同的工作負載表現差很多；在一些簡單的請求大小分佈下可以證明是最佳的。最大的問題是現在的程式很難事先知道要多少記憶體（取決於輸入），所以 base–limit 只剩很小的嵌入式裝置在用，「已經進了虛擬記憶體的博物館」。

---

## 2. 分頁：frame 與 physical page

[05:00] 另一個想法是**分頁**（第 4–5 講）。執行期主要是**硬體**的事：TLB 提供對應；[05:32] TLB miss 時查頁表。軟體太慢，不能放在執行期。

[05:32]–[06:31] 教授區分兩個名詞：

| 名詞 | 意思 |
|---|---|
| **Frame** | 主記憶體裡一塊 4 KB 的**空間**。記憶體 4 MB 就有 1000 個 frame |
| **Physical page** | process 位址空間裡一塊 4 KB 的**內容**，存放在某個 frame 裡 |

[06:31] 比喻：frame 是容器，physical page 是裝在裡面的東西。例如「記憶體有 100 個 frame」、「從磁碟讀一個 physical page 存到記憶體」。大部分時候兩個詞混用。

[07:01] 記憶體滿了，要**踢掉**某個 physical page。踢誰？這就是**頁面置換（page replacement）**問題。

【課本補充】課本：追蹤空的 frame 很簡單，一個位元向量（每個 frame 一個位元），再用增廣樹在 O(log N) 時間找到第一個空 frame。真正重要的是踢誰：缺頁（page fault）要讀硬碟或快閃記憶體，要幾百萬個週期，是現代系統最大的效能殺手之一。

---

## 3. Stack distance

[07:01] 先建立一個理論工具。**維護一個 stack，裝所有被存取過的 page**：

- 存取一個**新的** page：放到 stack 頂端。
- [07:30] 存取一個**以前存取過的** page：找到它在 stack 裡的位置，把它拿出來放到頂端。

[07:30] 它原本離頂端的距離，叫 **stack distance（堆疊距離）**。

```
存取 C 之前：       存取 C 之後：
  頂 ─ A              頂 ─ C   ← 移到頂端
       B                   A
       C   ← distance = 2   B
       D                   D
```

[08:00] 頂端是最近存取的，下一個是第二近的，以此類推。新的 page 不記錄距離。

> **課本 Definition 6.1.2**：存取一個 page 時，記錄它在 stack 裡離頂端多遠，再把它移到頂端。第一次存取的 page 直接放頂端，不記錄。stack distance 分佈是評估**時間局部性**的標準工具，從 cache 到分頁系統都適用。

[08:30] 可以畫 stack distance 的**機率密度**：

- 距離小：存取一個 page 後很快又存取它。
- 距離大：很久以後才再存取。

[08:30]–[10:01] 典型的形狀像**對數常態分佈（log-normal）**（課本 Figure 6.4）：

```
機率
  │      ▄
  │     ███
  │    █████▄
  │   ▄███████▄▄▄
  │  ▁██████████████▄▄▄▄▄▄▂▂▂▂▂▁▁▁▁▁   ← 重尾
  └──────────────────────────────────▶ stack distance
   0 1 2
```

| 區段 | 現象 | 原因 |
|---|---|---|
| 非常小（0、1、2） | **很少** | [09:01] 程式同時存取好幾個資料流：好幾個陣列、程式碼 page……連續存取同一個 page 不常見 |
| 中低 | [09:30] **非常多**（高峰） | 時間局部性強 |
| 很大 | [09:30] **不罕見**（重尾） | 【課本】程式會隨機存取、會換階段、會存取很多不常用的資料 |

[10:01] **時間局部性**：存取了某個東西，很可能很快又存取它。

[10:31] stack distance 可以幫助設計：TLB 要多大、頁表的延遲要多低。分佈集中在小值 → 大 TLB 就夠了；尾巴很重 → TLB miss 多、缺頁也多，頁表和**置換演算法**就變得很重要。

【課本補充】研究者通常用對數常態分佈建模，因為它有重尾，數學上也好分析。

---

## 4. 最佳置換演算法（OPT）

[11:02] 有一個理論上**證明最小化缺頁率**的最佳演算法。

[11:02] 設定：一串存取序列（page 3、4、1、7……）。每次存取在主記憶體裡找；找到很好；找不到要踢掉一個騰出空間，這就是**缺頁**。目標是缺頁最少。

[11:32]–[12:30] 做法（**假想的**）：

1. 假設知道每個 page **下一次什麼時候會被用**（例如 page 4 在 17 次存取後、page 5 在 10 次後）。
2. 把記憶體裡的 page 照下次使用時間排序。
3. 踢掉**最晚**才會被用到的那個。

[13:00] 可以證明這是最佳的。

【課本補充】證明方法跟排程的最佳性證明一樣：反證法加交換論證。像排程時假設知道 job 長度一樣，這裡也假設知道未來，不實際。

---

## 5. Stack 性質

[13:00] 這些叫**以 stack 為基礎的演算法（stack-based algorithm）**。先講它們的關鍵性質。

⚠️ [13:31]–[14:43] 逐字稿這段公式掉字，依課本補上：

> **課本 Point 6.1.1（Stack 性質）**：S_n = 有 n 個 frame 的記憶體裡的 page 集合。對所有 n ≥ 1，
> ```
> S_n ⊆ S_{n+1}
> ```

[14:43]–[16:02] 意思：同一串存取序列、同一個置換演算法，跑在小記憶體和大一格的記憶體上。**任何時刻**，小記憶體有的，大記憶體都有。

- 大記憶體多一個格子，所以不會相等（剛開始填的時候例外）。

[16:30] 用類似歸納法的論證：一開始兩個都空，性質成立。假設到某一點都成立，什麼時候會被破壞？

[17:01]–[19:00] 要有某個 page X 在 S_n 裡、**不在** S_{n+1} 裡。這表示：

- 大記憶體把 X 踢掉了，小記憶體卻留著 X、踢掉了別的（Y）。
- 但到上一步為止性質成立，所以 Y 也在大記憶體裡。
- 同一個演算法，在小記憶體裡判斷「X 比 Y 好」，在大記憶體裡判斷「Y 比 X 好」。比較結果**不一致**，性質就被破壞了。

【課本補充】課本用**頁面成本函數 F**（值越大越該被踢）說得更精確：M_n 裡 F(r) < F(q)，M_{n+1} 裡 F(q) < F(r)。這表示 F **跟記憶體大小有關**，不是全域的。

[19:32]–[20:00] OPT 的 F = 下一次存取的時間，這是「絕對的事實」，跟記憶體大小無關，兩邊不會比出不同結果。所以 OPT 滿足 stack 性質。

[20:00]–[21:02] 違反 stack 性質會發生很怪的事：**記憶體變小，缺頁反而變少**。滿足 stack 性質時，記憶體變大，缺頁率**只會不變或下降**，不會變差。

---

## 6. LRU 與它的近似

### 6.1 純 LRU 做不到

[21:02] **LRU（Least Recently Used）**：踢掉**最久沒被用**的。stack distance 用的就是這個。

[21:34] 成本函數 ∝ 1 / 最後存取時間：越早存取過，越該被踢。跟記憶體大小無關，所以 LRU 也滿足 stack 性質。

[22:01] 問題：

- OPT 不實際：不知道未來。
- LRU 很難做：要追蹤**每一次**存取，每個 TLB 項目都要記最後存取時間。

【課本補充】課本：不能在主記憶體或 TLB 加欄位存時間戳（成本高，要改硬體）；不能每次存取都計算存時間戳；找最小時間戳要維護 stack（每次存取都要更新）或 priority queue（O(log N)）。存在頁表項目裡也不行：不是每次存取都會查頁表（TLB 命中時不查）。

[22:01] 用 FIFO（照進記憶體的順序）？也不好，它不滿足 stack 性質（§7）。

### 6.2 用存取位元近似

[22:33] **實用的 LRU**：利用 page 的**保護位元**。

- **故意**把 page 標成不可存取（就算它其實可以）。
- [23:01] 存取時就會觸發**缺頁**。跟第 12 講的 copy-on-write 同一個技巧。
- 這是**軟缺頁（soft page fault）**：page 其實在記憶體裡，只是為了追蹤存取才觸發。

[23:32]–[24:00]

1. 定期掃過所有 page，把存取位元設成 0。
2. page 一被存取 → 軟缺頁 → 存取位元 0 → 1。
3. 要踢 page 時，挑一個**存取位元還是 0** 的：表示從上次清除後就沒被存取過，不是最近用的。

[24:00] 教授：這是 LRU **很粗略**的近似，但在作業系統裡這是能做到最好的，再多就會增加太多額外負擔。

[24:00]–[24:30] 也可以在 0 → 1 時記下時間戳，但這只增加複雜度，好處不多。

【課本補充】課本：軟缺頁還是要進例外處理，但不用讀硬碟（幾百萬個週期），只要做一點簿記、改權限，快得多。課本也說「用時間戳當近似」在實務上準確度有疑問，**沒有**被真實系統採用，實際用的是 WS-Clock 這一類。

### 6.3 ⚠️【原始碼】x86 的存取位元是硬體設的

教授和課本說「存取位元靠故意的軟缺頁來設」。在 **x86 上不是這樣**：CPU 存取 page 時，**硬體自動**在頁表項目設 Accessed 位元，寫入時設 Dirty 位元，**不會**觸發缺頁。

`arch/x86/include/asm/pgtable_types.h:10-16`

```c
#define _PAGE_BIT_PRESENT	0	/* is present */
#define _PAGE_BIT_RW		1	/* writeable */
#define _PAGE_BIT_USER		2	/* userspace addressable */
...
#define _PAGE_BIT_ACCESSED	5	/* was accessed (raised by CPU) */
#define _PAGE_BIT_DIRTY		6	/* was written to (raised by CPU) */
```

註解直接寫了 **"raised by CPU"**。

kernel 要「清除並讀出」存取位元時：

`arch/x86/mm/pgtable.c:545-555`

```c
int ptep_test_and_clear_young(struct vm_area_struct *vma,
			      unsigned long addr, pte_t *ptep)
{
	int ret = 0;

	if (pte_young(*ptep))
		ret = test_and_clear_bit(_PAGE_BIT_ACCESSED,
					 (unsigned long *) &ptep->pte);

	return ret;
}
```

- `pte_young`：Accessed 位元是 1 嗎？（Linux 把「最近被存取過」叫 **young**。）
- 是的話，用 atomic 的 `test_and_clear_bit` 清成 0，回傳「剛才是 1」。
- 這就是 §6.4 時鐘指針做的事：看到 1 就清成 0、繼續往前。

那教授講的「故意缺頁」是誰在用？kernel 的通用標頭說明了：

`include/linux/pgtable.h:281-291`

```c
#ifndef arch_has_hw_pte_young
/*
 * Return whether the accessed bit is supported on the local CPU.
 *
 * This stub assumes accessing through an old PTE triggers a page fault.
 * Architectures that automatically set the access bit should overwrite it.
 */
static inline bool arch_has_hw_pte_young(void)
{
	return false;
}
#endif
```

- 預設（沒有硬體支援的架構）：「假設透過一個 old PTE 存取會**觸發缺頁**」。這就是教授說的軟缺頁做法。
- 「自動設存取位元的架構應該覆寫它」：

`arch/x86/include/asm/pgtable.h:1452-1456`

```c
#define arch_has_hw_pte_young arch_has_hw_pte_young
static inline bool arch_has_hw_pte_young(void)
{
	return true;
}
```

x86 回傳 true。ARM64 看 CPU 有沒有硬體 AF（`cpu_has_hw_af`）。所以：**觀念對，x86 的實作是硬體幫你做**，省掉了軟缺頁的成本。

（也因為 TLB 裡快取了 PTE，清掉 Accessed 位元後，嚴格來說要 flush TLB，硬體才會再設一次；Linux 為了效能常常不 flush，接受一點不準確。）

### 6.4 WS-Clock

[25:02] 純 LRU 需要很多硬體支援。改用：

[25:30] 把記憶體裡所有 physical page 排成一個**環狀串列**，有一根像時鐘的指針：

```
           ┌─────┐
      ┌───▶│ A=1 │────┐
      │    └─────┘    ▼
   ┌─────┐         ┌─────┐
   │ A=0 │         │ A=1 │◀── 指針
   └─────┘         └─────┘
      ▲    ┌─────┐    │
      └────│ A=0 │◀───┘
           └─────┘
```

[26:03]–[27:31] 要找一個 page 來踢：

1. 指針指的 page 存取位元是 **1**：最近被存取過。**設成 0**（下次存取會再設回 1），指針往前。
2. 再看下一個，是 1 就一樣設成 0、往前。
3. 遇到存取位元是 **0** 的：選它踢掉。這個 physical page 被送到 swap space，frame 空出來，放新的 physical page。
4. 下次從**停下來的地方**繼續找，到底就繞回開頭。

[28:00] 教授：這是 LRU 的實用近似，因為時間戳難以維護和儲存。

【課本補充】課本：這個演算法**不需要定期掃過所有 page** 把存取位元清成 0（那很花時間），只有在需要踢 page 時才走指針。WS 代表 working set（§9）。

⚠️ 正式文獻裡，上面這個叫 **Clock（時鐘）演算法**。**WSClock**（Carr & Hennessy, 1981）還會看每個 page 的「最後使用時間」，超過 working set 時間窗 τ 的才踢。課本和教授把 Clock 叫做 WS-Clock。

### 6.5 WS-Clock 第二次機會

[28:00] 聰明一點的版本：區分**讀**和**寫**。用兩個位元：**存取位元 A** 和**修改位元 M**。

課本 Table 6.1：

| ⟨A, M⟩ | 新狀態 | 動作 |
|---|---|---|
| ⟨0, 0⟩ | ⟨0, 0⟩ | [29:00] 沒存取、沒修改：**直接踢掉** |
| ⟨0, 1⟩ | ⟨0, 0⟩ | [29:31] 最近沒存取，但修改過：**不踢**，排程把內容寫回 swap space，往前走。下一圈回來，如果期間沒被存取，就能**無痛**踢掉（swap 裡已經有副本，像 write-back cache） |
| ⟨1, 0⟩ | ⟨0, 0⟩ | [30:02] 存取過、沒修改：跟上一個演算法一樣，給第二次機會，往前走 |
| ⟨1, 1⟩ | ⟨1, 0⟩ | [30:32] 存取過又修改過：很重要、常被寫的 page。**保留存取位元**，只排程寫回，往前走 |

[31:30] 這類演算法用粗略的方式，**偏好留下**常用的 page、踢掉不常用的。

【課本補充】課本：這假設 swap space 是**包含式**的（記憶體裡每個 page 在 swap 裡都有副本，可能是舊的），swap 像下一層 cache。沒修改的 page 可以直接丟，修改過的要先寫回，所以沒修改的優先被踢。

### 6.6 【原始碼】Linux 傳統的 active / inactive 串列

Linux（MGLRU 之前，第 32 講）用的也是「第二次機會」的想法，只是用**兩個串列**：

`mm/swap.c:478-488`

```c
/*
 * Mark a page as having seen activity.
 *
 * inactive,unreferenced	->	inactive,referenced
 * inactive,referenced		->	active,unreferenced
 * active,unreferenced		->	active,referenced
 *
 * When a newly allocated page is not yet visible, so safe for non-atomic ops,
 * __SetPageReferenced(page) may be substituted for mark_page_accessed(page).
 */
void folio_mark_accessed(struct folio *folio)
```

- 每個 page 在 **active** 或 **inactive** 串列上，另外有一個 **referenced** 旗標。
- 被存取一次：inactive 裡的 page 先標成 referenced；**再被存取一次**才升級到 active。
- 要回收記憶體時，從 inactive 串列尾端找；active 太多時，把 active 的尾端降到 inactive。
- 這像兩層的 Clock：一個 page 要被存取**兩次**才算「常用」，只被掃過一次的大檔案不會把真正常用的 page 擠掉。

---

## 7. FIFO 與 Belady 異常

[31:30] **FIFO**：照 page **進入記憶體**的順序，先進來的先踢。

【課本補充】課本：FIFO 很受歡迎，因為很好做：不用追蹤使用、不用存取位元、沒有軟缺頁，只要一個 queue。

[32:00] FIFO **不滿足** stack 性質：S_n 不一定是 S_{n+1} 的子集。

[32:00]–[33:32] 原因：成本函數 = 1 / **這個 page 最後一次進入這個記憶體的時間**。不同大小的記憶體，page 進出的時間點不同，這不是全域的量。（OPT 的「下次存取時間」則不會有分歧。）

### 7.1 例子：4 個 frame 10 次缺頁、3 個 frame 9 次

[33:32] 存取序列：

```
1 2 3 4 1 2 5 1 2 3 4 5
```

**4 個 frame**（課本 Figure 6.6，最上面是 queue 的尾巴＝最新的）：

```
存取    1  2  3  4  1  2  5  1  2  3  4  5
        ✗  ✗  ✗  ✗  ✓  ✓  ✗  ✗  ✗  ✗  ✗  ✗    → 10 次缺頁
queue   1  2  3  4  4  4  5  1  2  3  4  5
           1  2  3  3  3  4  5  1  2  3  4
              1  2  2  2  3  4  5  1  2  3
                 1  1  1  2  3  4  5  1  2
```

[34:00] 存取 1、2 時已經在記憶體裡，FIFO 不調整順序（跟 LRU 不同）。[34:32] 存取 5：踢掉 queue 頭的 1。[34:32] 接著存取 1，又不在了，再進來。教授說這種「剛踢掉又被存取」的情況是缺頁的主因。

**3 個 frame**（課本 Figure 6.7）：

```
存取    1  2  3  4  1  2  5  1  2  3  4  5
        ✗  ✗  ✗  ✗  ✗  ✗  ✗  ✓  ✓  ✗  ✗  ✓    → 9 次缺頁
queue   1  2  3  4  1  2  5  5  5  3  4  4
           1  2  3  4  1  2  2  2  5  3  3
              1  2  3  4  1  1  1  2  5  5
```

[35:00] **記憶體變小，缺頁反而變少**（9 < 10）。這叫 **Belady 異常（Belady's anomaly）**。

### 7.2 為什麼會這樣

[35:30]–[37:01] 找出 stack 性質在哪裡被破壞：

| 存取後 | 3 個 frame | 4 個 frame | 子集？ |
|---|---|---|---|
| 第 5 個（1） | {1, 3, 4} | {1, 2, 3, 4} | ✓ |
| 第 6 個（2） | {2, 1, 4} | {1, 2, 3, 4} | ✓ |
| 第 7 個（5） | {5, 2, 1} | {5, 2, 3, 4} | **✗**：1 不在大記憶體裡 |

[37:01]–[38:02] 存取 5 時：

- **4 個 frame** 踢掉了 **1**，留著 4。
- **3 個 frame** 踢掉了 **4**，留著 1。
- 同一個演算法，對 1 和 4 的偏好在兩個記憶體裡**相反**。

[38:32] 原因：成本函數不是全域的，是**局部**的：1 和 4 的優先權取決於它們「最後一次進入**這個**記憶體」的時間，兩個記憶體不一樣。

【課本補充】課本強調這不只是小差異：Fornai 和 Iványi 證明了一定存在某些序列，能讓這個差距**任意大**（沒有上界）。所以 FIFO 這種非 stack 演算法在最壞情況下表現可以非常差。

[39:01] 結論：

> **課本 Point 6.1.2**：純 stack 演算法沒有 Belady 異常，但通常不實際（LRU、OPT）。WS-Clock 和 WS-Clock 第二次機會是 LRU 的近似。FIFO 比較容易做，但有 Belady 異常，最壞情況可以任意差。

[39:01] 教授：**不要用 FIFO**；時鐘類的演算法好得多。

---

## 8. 【實測】四種演算法、五種記憶體大小

`repl.py` 用同一串序列跑 FIFO、Clock（§6.4，教授的 WS-Clock）、LRU、OPT，記憶體 1 到 5 個 frame：

```
sequence [1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5]
  frames=1: FIFO=12  CLOCK=12  LRU=12  OPT=12
  frames=2: FIFO=12  CLOCK=12  LRU=12  OPT= 9
  frames=3: FIFO= 9  CLOCK= 9  LRU=10  OPT= 7
  frames=4: FIFO=10  CLOCK=10  LRU= 8  OPT= 6
  frames=5: FIFO= 5  CLOCK= 5  LRU= 5  OPT= 5
```

- **FIFO**：3 個 frame 9 次、4 個 frame 10 次，重現課本的 Belady 異常。
- **LRU、OPT**：記憶體越大缺頁越少（12 → 12 → 10 → 8 → 5；12 → 9 → 7 → 6 → 5），符合 stack 性質。
- OPT 在每種大小都是最少的。
- ⚠️ **Clock 也出現了 Belady 異常**（9 → 10）！Clock 本質上是「FIFO 加第二次機會」，它的成本函數（指針位置 + 參考位元）也跟記憶體大小有關，所以**不是** stack 演算法。教授說「時鐘類的演算法好得多」，在一般工作負載上是對的（它近似 LRU），但嚴格來說它**不保證**沒有 Belady 異常。

**Stack distance 的一個小例子**：模擬 64×64 double 矩陣相乘（C = A × B，一列 512 bytes，一頁 8 列），以 page 為單位算 stack distance：

```
matmul N=64: 528360 reuses, 24 first touches, pages used=24
  stack distance  1:  92.3%
  stack distance  2:   0.8%
  stack distance  9:   7.0%
  stack distance 10:   0.0%
```

- **完全沒有距離 0**：最內層迴圈交替存取 A 和 B，同一個 page 不會連續被存取兩次。正是課本說「同時存取好幾個資料流，所以距離很小的情況少見」。
- 大部分是 1（A 和 B 輪流）；距離 9 是 B 的 8 個 page 輪流被走過。
- 這個程式太規律，不會有對數常態的重尾；真實程式混合很多資料結構和階段才會有。

---

## 9. Working set 與 thrashing

[39:01] WS 的意思是 **working set（工作集）**：

[39:30] 一個程式在一段時間內**經常存取**的 page 集合。「一段時間」很模糊，沒辦法說到底多長。

[40:00] 但大部分程式有這個行為（課本 Figure 6.8）：

```
缺頁率
  │█
  │█▇
  │ ▇▆▅▄
  │      ▃
  │       ▂            ← 急降
  │        ▁▁▁▁▁▁▁▁▁▁▁
  └───────┬────────────▶ 記憶體裡的 page 數
       working set
```

- 記憶體裡的 page 數增加，缺頁率一開始降得很慢，到某一點**突然大降**。
- 沒有證明，是**經驗觀察**：程式往往存取同一塊資料、同一段程式碼，然後換到別的區域。
- [40:00] 急降的那一點就是程式的 **working set 大小**：要這麼多 page 才能順利運作。

【課本補充】課本：曲線形狀因程式而異（有的陡、有的緩），但幾乎所有真實程式都看得到這個效應，所以用這種方式定義 working set 相當合理。

[40:31] 相關的概念：**thrashing（顛簸）**（第 32 講會花更多時間）：

[40:31]–[41:32]

1. 開機時系統很慢，因為在載入 working set。
2. 你沒耐心，又開了另一個程式 → 要載入更多 page。
3. 原本一個 working set 都塞不下，現在兩個更塞不下 → 兩個程式都很慢。
4. 又開第三個……整個系統都很慢。
5. 同時，**排程器看到 CPU 負載很低**（大家都在等從磁碟讀 page），就開更多程式。
6. 惡性循環，系統崩潰。

[41:32] 教授的建議：程式沒反應時，**不要亂點、不要再開別的程式，等就對了**。

[41:32] 原因：第 26 講的負載公式（PELT）會算出負載很低，排程器就在 CPU 上放更多 task，只會讓問題更糟。

---

## 10. 【課本補充＋實測】共享記憶體 IPC

本講沒講，課本 §6.1.4 有。兩個 process 共享一塊記憶體：

```c
key_t key = ftok("shmfile", 59);                    /* 產生 key */
int shmid = shmget(key, SHM_SIZE, 0666|IPC_CREAT);   /* 建立 / 取得共享記憶體區段 */
char *str = (char *) shmat(shmid, NULL, 0);          /* 附加到自己的位址空間 */
strcpy(str, "Message from the writer.");             /* 直接寫 */
shmdt(str);                                          /* 分離 */
shmctl(shmid, IPC_RMID, NULL);                       /* 刪除區段 */
```

- `shmat` 在頁表裡建立對應：`str` 的虛擬位址指向共享的 physical page。
- 之後讀寫 `str` 就直接存取共享的 page，另一個 process **馬上**看得到（不用系統呼叫）。
- `shmdt` 做相反的事：拿掉對應。

【實測】`shm.c`：父 process 寫入，子 process 再 `shmat` 一次來讀：

```
7f68a95bb000-7f68a95bc000 rw-s 00000000 00:01 10                         /SYSV3b084013 (deleted)
writer: shmid=10 attached at 0x7f68a95bb000
reader: attached at 0x7f68a9581000, read: Message from the writer.
```

- `/proc/<pid>/maps` 裡多了一塊 4 KB 的區域，權限 `rw-s`：**`s` = shared**（一般的私有對應是 `p`）。名稱 `/SYSV3b084013` 就是 key（`0x3b084013`）。
- 子 process 附加在**不同的虛擬位址**（`0x7f68a9581000`），卻讀到父 process 寫的字串：兩個虛擬位址對應到**同一個 frame**。這就是第 31 講 reverse mapping 要處理的「一個 physical page 有很多對應」。

---

## 11. 一頁速查表

### 11.1 Base–limit

實際位址 = base + V（< limit）。外部碎片（洞）；挑洞：best / worst / next / first fit。

### 11.2 置換演算法

| 演算法 | 踢誰 | 成本函數 | Stack 性質 | 實用？ |
|---|---|---|---|---|
| OPT | 最晚才會用到的 | 下次存取時間 | ✓ | ✗（要知道未來） |
| LRU | 最久沒用的 | 1 / 最後存取時間 | ✓ | ✗（要時間戳） |
| Clock（WS-Clock） | 指針遇到的第一個 A = 0 | 指針 + 參考位元 | ✗（實測也有 Belady） | ✓ |
| 第二次機會（A, M） | ⟨0,0⟩；⟨0,1⟩ 先寫回 | | ✗ | ✓ |
| FIFO | 最早進來的 | 1 / 進入時間 | ✗（Belady 異常，可任意差） | ✓ 但不該用 |

Stack 性質 S_n ⊆ S_{n+1} ⟺ 記憶體變大，缺頁不會變多。

### 11.3 存取位元

| | 教授 / 課本 | x86 實際 |
|---|---|---|
| 怎麼設 | 故意設成不可存取 → 軟缺頁 → 設 1 | CPU 硬體自動設 Accessed（bit 5）、Dirty（bit 6） |
| 清除 | 定期 / 指針經過 | `ptep_test_and_clear_young` |

### 11.4 Working set、thrashing

Working set = 缺頁率急降的點。Thrashing：記憶體不夠 → 一直缺頁 → CPU 閒 → 排程器開更多程式 → 更糟。

### 11.5 本講看過的原始碼位置

| 位置 | 內容 |
|---|---|
| `arch/x86/include/asm/pgtable_types.h:10-16` | PTE 位元：Accessed、Dirty 由 CPU 設 |
| `arch/x86/mm/pgtable.c:545-555` | `ptep_test_and_clear_young` |
| `include/linux/pgtable.h:281-291` | 預設：沒有硬體存取位元時靠缺頁 |
| `arch/x86/include/asm/pgtable.h:1452-1456` | x86：有硬體存取位元 |
| `mm/swap.c:478-488` | active / inactive + referenced 的升級規則 |

---

## 12. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **frame / physical page、page 與 folio、zone、頁表** | **第 30 講**：Memory architecture of the kernel |
| **一個 page 被很多 process 對應（共享記憶體、fork、COW）** | **第 31 講**：Reverse mapping |
| **LRU 近似、Accessed 位元、thrashing** | **第 32 講**：MGLRU |
| **base–limit 式的連續配置** | **第 33 講**：buddy、slab、slub |
| **軟缺頁、COW** | 回顧第 12 講 |

（講次對應依播放清單標題推估。）

---

## 附：本講一句話總結

> **沒有虛擬記憶體時用 base–limit：每個 process 一塊連續的實體區域，位址加上 base，區域之間的洞造成外部碎片，挑洞用 best / worst / next / first fit，kernel 管實體記憶體仍用它的進化版。有了分頁，關鍵問題變成記憶體滿了要踢哪個 page：用 stack distance 看時間局部性（典型是極小值少、中低值多、尾巴很重的對數常態形狀）；OPT（踢最晚才用的）最佳但要知道未來，LRU 要時間戳也不實際，兩者都滿足 stack 性質 S_n ⊆ S_{n+1}，記憶體變大缺頁只會變少。實務上用存取位元近似：教授說靠故意的軟缺頁來設，但 x86 的 Accessed / Dirty 位元其實由 CPU 硬體自動設定，kernel 只要 `test_and_clear`；時鐘指針遇到 1 就清成 0、遇到 0 就踢，第二次機會版再考慮修改位元先寫回。FIFO 的成本函數跟記憶體大小有關，會出現 Belady 異常（實測 4 個 frame 10 次缺頁、3 個只有 9 次），而且連 Clock 也會。程式的 working set 是缺頁率急降的點，記憶體不夠時一直缺頁、CPU 看起來很閒、排程器又開更多程式，就是 thrashing。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 29（Chapter 6, Part I）錄影逐字稿（YouTube 自動字幕，42:34）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）第 6 章開頭與 §6.1，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
