# Operating Systems: A Linux Kernel-Oriented Approach 第五講完整筆記：虛擬記憶體（Chapter 2, Part IV）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 5（Chapter 2, Part IV）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（46:56，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§2.2–2.2.3（p.35–50）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
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
- [1. Process 怎麼看記憶體](#1-process-怎麼看記憶體)
- [2. 三個問題](#2-三個問題)
- [3. Process 的記憶體映像（memory map）](#3-process-的記憶體映像memory-map)
  - [3.1 32 位元 Linux](#31-32-位元-linux)
  - [3.2 大家的 map 都一樣 → 更容易重疊](#32-大家的-map-都一樣--更容易重疊)
  - [3.3 【原始碼】3 GB / 1 GB 的分界在哪裡設定](#33-原始碼3-gb--1-gb-的分界在哪裡設定)
- [4. 位址轉換：虛擬位址 → 實體位址](#4-位址轉換虛擬位址--實體位址)
- [5. 第一個嘗試：base–limit](#5-第一個嘗試baselimit)
  - [5.1 做法](#51-做法)
  - [5.2 問題：內部碎片與外部碎片](#52-問題內部碎片與外部碎片)
  - [5.3 用 linked list 延伸？最後還是需要一張表](#53-用-linked-list-延伸最後還是需要一張表)
- [6. Page 與 frame](#6-page-與-frame)
- [7. Page table：多層設計](#7-page-table多層設計)
  - [7.1 為什麼不能用一張大表](#71-為什麼不能用一張大表)
  - [7.2 64 位元其實只用 48 位元](#72-64-位元其實只用-48-位元)
  - [7.3 【實測】用 `/proc/self/maps` 看真的 memory map](#73-實測用-procselfmaps-看真的-memory-map)
  - [7.4 關鍵觀察：高位元變化少](#74-關鍵觀察高位元變化少)
  - [7.5 四層 page table 與 CR3](#75-四層-page-table-與-cr3)
  - [7.6 【Q&A】每個 level 1 的格子都指向 level 2 嗎？](#76-qa每個-level-1-的格子都指向-level-2-嗎)
  - [7.7 【原始碼】x86-64 的四層：PGD、PUD、PMD、PTE](#77-原始碼x86-64-的四層pgdpudpmdpte)
  - [7.8 【原始碼】kernel 自己走一次 page table](#78-原始碼kernel-自己走一次-page-table)
  - [7.9 【原始碼】換 process 時換 CR3](#79-原始碼換-process-時換-cr3)
- [8. TLB：讓 page table 變得實用](#8-tlb讓-page-table-變得實用)
- [9. 解決大小問題：swap space](#9-解決大小問題swap-space)
  - [9.1 想法](#91-想法)
  - [9.2 存取流程](#92-存取流程)
  - [9.3 主記憶體滿了：要踢掉一個 frame](#93-主記憶體滿了要踢掉一個-frame)
  - [9.4 Page fault：hard 與 soft](#94-page-faulthard-與-soft)
- [10. Page table entry 裡的保護位元](#10-page-table-entry-裡的保護位元)
  - [10.1 【原始碼】x86 PTE 的每個位元](#101-原始碼x86-pte-的每個位元)
- [11. 【課本補充】共享記憶體與反向頁表](#11-課本補充共享記憶體與反向頁表)
- [12. 一頁速查表](#12-一頁速查表)
- [13. 與後續講次的連結](#13-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

`[00:32]` 計組課學過記憶體系統和虛擬記憶體，現在**從 OS 的角度**重新看一次。「觀點也很重要。」

主線：

```
 process 以為整個記憶體都是它的
          │
          ▼
 產生三個問題：相容、重疊、大小
          │
          ▼
 base–limit（失敗，但帶出「位址轉換」的想法）
          │
          ▼
 page + frame + 多層 page table（解決相容與重疊）
          │
          ▼
 TLB（讓轉換夠快）
          │
          ▼
 swap space（解決大小）
```

---

## 1. Process 怎麼看記憶體

`[01:01]` process 的看法很簡單（課本 Fig 2.7）：

1. **每個位置都能存取。**
2. **它是系統裡唯一的 process**，沒有別人在跑。

| 位址寬度 | 位址範圍 | 大小 |
|---|---|---|
| 32 位元 | 0 到 2³² − 1 | 4 GB |
| 64 位元 | 0 到 2⁶⁴ − 1 | **16 EB**（exabyte）。「到 2025 年沒有人有這麼多記憶體」 |

程式設計師、編譯器、甚至大部分 CPU pipeline 都做同樣的假設。課本說這個假設**很優雅**，讓所有人的工作都變簡單——**除了設計記憶體系統的工程師**。

【課本補充】Trivia 2.2.1：load/store 指令從**暫存器**讀基底位址，再加一個常數偏移。所以**暫存器的寬度**決定了位址範圍：暫存器 32 位元，位址空間就是 2³² bytes。

---

## 2. 三個問題

`[01:31]` 但實際的記憶體可能只有 4 GB、8 GB、16 GB。

| 問題 | 說明 | 教授的說法 |
|---|---|---|
| **① 相容問題（compatibility）** | process 假設的空間（2³² 或 2⁶⁴）和實際記憶體（例如 1 GB = 2³⁰）**根本不相容**。怎麼把大空間對應到小空間？ | `[02:01]` |
| **② 重疊問題（overlap）** | 兩個 process 的記憶體空間怎麼確保**分開**？不然一個可以讀另一個寫的資料，或兩個互相覆寫——**正確性和安全性**都有問題 | `[02:31]` |
| **③ 大小問題（size）** | 程式要 2 GB，但只有 1 GB。直接給「記憶體不足」錯誤？「聽起來有點不公平。」應該能**暫時擴大**記憶體空間，例如借用硬碟的空間 | `[03:01]` |

`[04:00]` 要用**最有效率的方法**同時解決三個問題。

【課本補充】課本對三個問題補了理由：

- **相容問題**：程式只在開發者的機器上**編譯一次**，然後散布到全世界。一百萬份拷貝跑在記憶體大小差很多的機器上。除了「假設 2ⁿ 個位址都能用」，**沒有別的優雅假設**。
- **重疊問題**：一個 process 存了信用卡號碼，另一個 process 可以直接從記憶體讀出來——巨大的安全風險。
- **大小問題**：如果直接拒絕，代表在大記憶體機器上編譯的程式**不能在小記憶體機器上跑**，失去向下相容，商業風險很大。

---

## 3. Process 的記憶體映像（memory map）

### 3.1 32 位元 Linux

`[04:30]` Linux（其實所有 OS）的每個 process 都有一個 **memory map**：所有能存取的位址、怎麼排列。這是**標準化**的——loader 就照這個結構把 `a.out` 的各 section 載入記憶體。（課本 Fig 2.8）

```
 0xFFFFFFFF ┌──────────────────────────┐
            │      Kernel（1 GB）       │  ← 每個 process 都保留給 kernel
 0xC0000000 ├──────────────────────────┤
            │  Stack  ↓（往下長）        │
            │                          │
            │  Memory-mapped 區域        │  ← DLL（如 libc.so）、映射的檔案
            │  與 library               │
            │         ...              │  ← 非常大的空洞（數 GB）
            │                          │
            │  Heap  ↑（往上長）         │  ← new、malloc 產生的物件
            ├──────────────────────────┤
            │  .bss（沒初值的全域變數）   │
            │  .data（有初值的全域變數）  │
            │  Text（指令）              │
 0x08048000 ├──────────────────────────┤
            │  Metadata                │  ← 檔案、執行檔格式的資訊
 0x00000000 └──────────────────────────┘
```

| 區域 | 內容 | 方向 |
|---|---|---|
| Metadata | 位址 0 附近：檔案細節、執行檔格式 | — |
| Text | 所有指令 | — |
| Data / BSS | 有初值 / 沒初值的**全域變數** | — |
| **Heap** | 執行時用 `new`、`malloc` 產生的物件 | **往上長**（往高位址） |
| 中間區域 | **DLL** 的程式碼「複製貼上」到這裡（例如 `libc.so`）；**記憶體映射的檔案**也在這裡 | — |
| **Stack** | 從最上面開始 | **往下長** |
| **Kernel** | `0xC0000000` 以上 | — |

`[05:30]` `0xC0000000` = **3 GB** 的位置。32 位元總共 4 GB，**最上面 1 GB 保留給 kernel**，所以使用者程式只有 3 GB。

`[06:03]` 【口頭】教授說這是**很棒的設計**：每個使用者 process 的 memory map 裡**都有 kernel 區域**，讓 process 可以很順地跟 kernel 溝通。當然，process 不能存取那塊區域。（好處在記憶體那章再講。）

`[06:30]` 這張圖**不是按比例畫的**：heap 和 stack 之間的距離其實**非常非常大**，有好幾 GB。

`[07:02]` 【口頭】**記憶體映射檔案（memory-mapped file）** 是什麼？讀檔案的內容，放進你的記憶體空間。DLL 其實也是一樣的事：讀 `libc.so` 這種共享物件的內容，把指令複製到你的記憶體空間。

【課本補充】課本補充（p.38）：

- 如果 stack、heap 和其他 section 加起來超過 3 GB，就會用完空間。「這確實會發生，所以世界轉向 64 位元。」
- kernel 那 1 GB 在所有 process 的 memory map 裡都**一樣**（invariant），永遠在最上面 1 GB，一般使用者 process 不能碰。

### 3.2 大家的 map 都一樣 → 更容易重疊

`[08:00]` **每個 process 都有一樣的 map**。好處是標準化，但壞處是：

- 從各 process 自己的角度，**位址很可能一樣**。例如某個 heap 位址，在另一個 process 裡很可能也指向一個 heap 變數。
- 同一個程式開好幾份，位址**一定一樣**。

`[09:02]` 這只是 **process 的觀點**，不是記憶體系統的觀點。但標準化讓**重疊的機會更高**，所以一定要設計一個機制解決三個問題。

【課本補充】課本也說：固定的 memory map 讓**產生程式碼很容易**，執行檔格式也能固定，OS 知道怎麼擺程式碼和資料。但它讓重疊問題**更嚴重**——就算兩個 process 都完全無害，也可能互相破壞。

### 3.3 【原始碼】3 GB / 1 GB 的分界在哪裡設定

32 位元 x86 的這條分界線叫 **`PAGE_OFFSET`**，是 Kconfig 的選項：

```text
# arch/x86/Kconfig:1473-1480
config PAGE_OFFSET
	hex
	default 0xB0000000 if VMSPLIT_3G_OPT
	default 0x80000000 if VMSPLIT_2G
	default 0x78000000 if VMSPLIT_2G_OPT
	default 0x40000000 if VMSPLIT_1G
	default 0xC0000000
	depends on X86_32
```

| 值 | 使用者 / kernel 怎麼分 |
|---|---|
| **`0xC0000000`**（預設） | **3 GB / 1 GB**——就是教授和課本說的 |
| `0x80000000` | 2 GB / 2 GB |
| `0x40000000` | 1 GB / 3 GB |
| `0xB0000000` | 約 2.75 GB / 1.25 GB |

- `depends on X86_32`：只有 **32 位元**才有這個選項。
- 所以「上面 1 GB 給 kernel」是**預設值**，可以調整。

64 位元的分界在哪？見 [§7.2](#72-64-位元其實只用-48-位元)。

---

## 4. 位址轉換：虛擬位址 → 實體位址

`[09:30]` 解決相容和重疊問題的方法：

```
  CPU 產生的位址          轉換（translation）         送到記憶體系統
  ───────────────  ──────────────────────────►  ─────────────────
   虛擬位址（VA）                                    實體位址（PA）
```

- **虛擬位址（virtual address）**：CPU 產生的位址。
- 它**被換成另一個位址**——**實體位址（physical address）**——才送到記憶體系統。
- 我們希望**轉換這個動作本身**就解決相容和重疊問題。

【課本補充】Definition 2.2.4：虛擬位址是程式設計師、process、編譯器、CPU 看到的位址。它不直接送到記憶體系統，要先轉成實體位址。記憶體系統的入口是 **i-cache（指令）和 L1 d-cache（資料）**，它們**只看到實體位址**。**如果每個實體位址只對應到一個虛擬位址，process 之間就永遠不會重疊。**

---

## 5. 第一個嘗試：base–limit

### 5.1 做法

`[10:00]`（課本 Fig 2.9）這個方法簡單，也**確實解決了不少問題**，只是沒有好到能用在現代系統。

```
 實體記憶體：
 ┌────────┬──────────┬──────┬────────────┬──────┬─────────┐
 │  P1    │   hole   │  P2  │    hole    │  P3  │  hole   │
 └────────┴──────────┴──────┴────────────┴──────┴─────────┘
 ↑base1   ↑limit1    ↑base2 ↑limit2       ↑base3 ↑limit3
```

- 每個 process 拿到一段**連續**的記憶體，在 **base 暫存器**和 **limit 暫存器**之間。
- 從 process 的角度，它的位址從 **0** 開始。
- 真正的位址 = **base + 位址**。例如 P1 要存取位址 3，實際位址是 base₁ + 3。

`[12:01]` 結果：

| 問題 | 解決了嗎？ |
|---|---|
| **重疊** | ✅ 同一個記憶體位置**永遠不會**對應到兩個 process。每個 process 有自己的連續區域，是私有的 |
| **相容** | ❌ 沒完全解決。memory map 是 0 到 3 GB，不可能每段都給 3 GB |

【課本補充】超過 limit 會產生 **fault**。base 的值程式設計師和編譯器**不用知道**，只要知道 limit − base（最多能用幾個 byte）。

### 5.2 問題：內部碎片與外部碎片

`[12:30]` 要幫新 process 配記憶體，就要在**空洞（hole）**裡找一個夠大的。問題：

1. `[13:01]` **事先（a priori）** 就要知道 process 會用多少記憶體——**幾乎不可能**，因為取決於執行時的參數。
2. 所以只能用**保守估計**：假設要 6 單位，實際只用 3 單位，**分配區內的浪費**就叫**內部碎片（internal fragmentation）**。「只要是最壞情況的估計，一定有浪費。」
3. `[14:01]` **外部碎片（external fragmentation）**：每次配置都把一個大洞變成「已配置 + 小洞」。久了會有很多小洞。**總空間夠，但不連續**，就浪費了。

| | 內部碎片 | 外部碎片 |
|---|---|---|
| 浪費在哪 | **分配區之內** | **分配區之外**（洞裡） |
| 原因 | 保守估計，給太多 | 洞太小、不連續 |

`[15:30]` 再加上相容問題：完整的 64 位元空間、stack 從非常高的位址開始，要對應到**連續**的實際 bytes，實際上很難。

> 【口頭】「所以這方法是三、四十年前用的。**kernel 的某些部分還在用**，之後會講是哪些。」

【課本補充】課本提到一個補救：定期**壓縮（compact）**記憶體，把資料搬到新位置、更新 base 和 limit，把小洞合成大洞。問題是要大量讀寫，搬的時候 process 大部分時間要停著。

### 5.3 用 linked list 延伸？最後還是需要一張表

`[16:00]` 一個很直覺的補救：配了 6 單位，但需要 10 單位怎麼辦？

- 後面剛好有空間 → 直接再拿 4 單位。
- 沒有 → 用 **linked list** 串到另一塊 4 單位。還要更多就再串一塊。

`[16:32]` 但問題來了：我要去某個位址，**怎麼知道它在哪一塊**？

`[17:00]` **最後一切都歸結到同一件事**：

> 給一個虛擬位址（相對於 process 的位址），要得到一個實體位址（真正的儲存位置）。這就是**轉換系統**：每個 process 有自己的虛擬位址空間，全部轉換到**同一個實體位址空間**——因為實體上只有一個記憶體。

> 【口頭】「虛擬是一個抽象、一個概念、一個想法。要實作這個叫**虛擬記憶體**的抽象，base–limit 不行。但好消息是：**每個失敗的想法裡，都有成功想法的種子。**」

【課本補充】Definition 2.2.5（**虛擬記憶體**）：process 假設它是整個記憶體系統的**獨佔擁有者**，可以隨意存取 0 到 2ⁿ − 1 的任何位址。實作方式是把每個虛擬位址對應到實體位址。映射本身就解決了相容問題；確保一個實體位址不會對應到兩個虛擬位址，就解決了重疊問題。實體位址空間還可以**延伸到硬碟**等儲存裝置，所以可以**大於主記憶體**。

---

## 6. Page 與 frame

`[17:31]` 從 base–limit 學到的：把記憶體**切成單位**。這裡也一樣，切成 **4 KB** 一塊。

| 名稱 | 在哪裡 | 大小 |
|---|---|---|
| **Page（頁）**、virtual page | **虛擬**位址空間 | 4 KB |
| **Frame（頁框）**、physical page | **實體**位址空間 | 4 KB |

`[18:30]` 建一張**映射表（mapping table）**，把一個 page（4 KB）對應到一個 frame（4 KB 的真實記憶體）。（課本 Fig 2.10）

```
 Process 1 虛擬空間        實體記憶體（frames）       Process 2 虛擬空間
 ┌──────┐                 ┌──────┐                 ┌──────┐
 │ page │ ──────────────► │frame │ ◄────────────── │ page │
 ├──────┤        ┌──────► ├──────┤                 ├──────┤
 │ page │ ───────┘        │frame │ ◄─────┐         │ page │
 ├──────┤                 ├──────┤       └──────── ├──────┤
 │ page │ ──────────────► │frame │                 │ page │
 └──────┘                 └──────┘                 └──────┘
```

`[19:00]` **實體位址空間**：所有實體上可定址的位置。例如有 1 GB 主記憶體，實體位址空間就是 1 GB（之後會再擴大）。

`[20:00]` 結果：

| 問題 | 怎麼解決 |
|---|---|
| **重疊** | 只要**沒有兩個 process 的虛擬 page 對應到同一個 frame**，就**自然不會重疊** |
| **相容** | `[20:31]` 整個虛擬空間大部分沒用，只有上面 stack、下面 heap、中間 library 有用。把**有用的部分**切成 4 KB，各自對應到不同 frame 就好 |
| **大小** | ❌ 還沒解決（見 §9） |

`[21:01]` **刻意的重疊**是可以的：例如兩個 process 之間建一個**共享記憶體通道**，一個寫、一個讀。除此之外，我們要的是**沒有意外的重疊**。

【課本補充】為什麼是 4 KB 而不是一個 byte？跟快取用 64 或 128 bytes 的 **cache block** 同樣的道理：byte 層級的映射資訊太多，不可擴展；而且有**時間與空間局部性**，同一塊會一再被用到，也等於**順便預取**。大型伺服器常用 **huge page**，2023 年時可以從 **2 MB 到 1 GB**。不管多大，page 和對應的 frame **一樣大**。

---

## 7. Page table：多層設計

### 7.1 為什麼不能用一張大表

`[22:01]` 映射表（**page table**）的輸入是 64 位元虛擬位址，輸出是 64 位元實體位址。

`[22:33]` 4 KB = **2¹²** bytes，所以：

```
  64 位元虛擬位址
  ┌──────────────────────────────────┬────────────┐
  │        上面 52 位元：page 編號      │ 下 12 位元  │
  │        （要被映射）                 │ page 內偏移 │
  └──────────────────────────────────┴────────────┘
                                       （不用映射，直接照抄）
```

- **下 12 位元**：page 內的 byte 位置，**固定不變**。
- **上 52 位元**：要映射到實體的 52 位元。

`[23:00]` 一張有 **2⁵² 格**的表？**太多了**。要利用某種**規律**：**虛擬位址空間大部分是空的**。

【課本補充】課本算了 32 位元的情況（p.43）：20 位元的 page 編號 → 2²⁰ ≈ 一百萬個 page，每個存 20 位元（2.5 bytes）的 frame 位址 → **每個 process 要 2.5 MB**。100 個 process 就要 **250 MB** 只放 page table！64 位元更不可能。

### 7.2 64 位元其實只用 48 位元

`[23:33]` 就算虛擬位址是 64 位元，**也用不到那麼多**，假設**用 48 位元**就好。（課本 Fig 2.12）

```
 2^48 − 1  ┌──────────────────────┐
           │  Stack ↓             │
           │                      │
           │  Memory-mapped 與     │
           │  library             │
           │        ...           │  ← 巨大的空洞
           │  Heap ↑              │
           │  .bss / .data         │
           │  Text                │
 0x400000  ├──────────────────────┤
           │  Metadata            │
 0         └──────────────────────┘
```

- section 的順序完全一樣，stack 從 **2⁴⁸** 附近開始往下長，heap 往上長，中間是**巨大的空洞**。
- 上面 16 位元假設都是 0。

`[24:01]` 用 **`pmap`** 指令可以看一個 process 的 memory map：給它 process ID，它會告訴你 text、data、bss 各多大等等。

【課本補充】2⁶⁴ bytes 超過十億 GB，2024 年沒有系統有這麼多記憶體，所以大部分系統用 48 位元虛擬位址。**實體位址**倒是可以用完整 64 位元，因為只是每格多存幾個位元，不用加層數。扣掉 12 位元偏移，frame 編號是 **52 位元**。

#### 【原始碼】x86-64 實際用幾位元？

```c
// arch/x86/include/asm/page_types.h:10-11
#define PAGE_SHIFT		12
#define PAGE_SIZE		(_AC(1,UL) << PAGE_SHIFT)
```

- `PAGE_SHIFT 12`：page 內偏移 **12 位元**。
- `PAGE_SIZE` = `1 << 12` = **4096** = 4 KB。

```c
// arch/x86/include/asm/page_64_types.h:54, 57, 60
#define __PHYSICAL_MASK_SHIFT	52
#define __VIRTUAL_MASK_SHIFT	(pgtable_l5_enabled() ? 56 : 47)
#define __VIRTUAL_MASK_SHIFT	47
```

| 常數 | 值 | 意思 |
|---|---|---|
| `__PHYSICAL_MASK_SHIFT` | **52** | 實體位址最多 52 位元 |
| `__VIRTUAL_MASK_SHIFT` | **47**（四層）或 **56**（五層） | 使用者空間的虛擬位址位元數 |

**跟課本對照**：

- 課本說「48 位元虛擬位址、stack 從 2⁴⁸ 附近開始」。
- 原始碼是 **47**。因為 x86-64 的 48 位元空間**對半切**：**下半（0 到 2⁴⁷）給使用者**，**上半給 kernel**。這就是 64 位元版的「3 GB / 1 GB」。
- 所以使用者 stack 的頂端在 **2⁴⁷ ≈ 128 TB**，不是 2⁴⁸。下面的實測可以印證。

### 7.3 【實測】用 `/proc/self/maps` 看真的 memory map

`pmap` 讀的就是 `/proc/<pid>/maps`。我在本機跑 `cat /proc/self/maps`（印出 `cat` 自己的 map），節錄：

```text
560b45cc2000-560b45cc4000 r--p 00000000 103:08 23988745   /usr/bin/cat
560b45cc4000-560b45cc8000 r-xp 00002000 103:08 23988745   /usr/bin/cat
560b45cc8000-560b45cca000 r--p 00006000 103:08 23988745   /usr/bin/cat
560b45ccb000-560b45ccc000 rw-p 00008000 103:08 23988745   /usr/bin/cat
560b46a59000-560b46a7a000 rw-p 00000000 00:00 0          [heap]
7f22e8600000-7f22e8628000 r--p 00000000 103:08 23988377   /usr/lib/x86_64-linux-gnu/libc.so.6
7f22e8628000-7f22e87bd000 r-xp 00028000 103:08 23988377   /usr/lib/x86_64-linux-gnu/libc.so.6
...
7f22e8a2e000-7f22e8a58000 r-xp 00002000 103:08 23988332   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
7fff455b7000-7fff455d9000 rw-p 00000000 00:00 0          [stack]
7fff455e3000-7fff455e5000 r-xp 00000000 00:00 0          [vdso]
ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0  [vsyscall]
```

對照課本的圖：

| 課本 | 實測 | 說明 |
|---|---|---|
| Text 在低位址 | `/usr/bin/cat`，`r-xp` 那段（可讀、可執行） | 但起點是 `0x560b...`，不是 `0x400000`。因為現代發行版預設把程式編成 **PIE**（位置無關執行檔），載入位址會**隨機化**（上一講課本提到 loader 可以隨機化位址） |
| Data / BSS | `cat` 的 `rw-p` 那段（可讀寫） | |
| Heap ↑ | `[heap]` | 緊接在程式後面 |
| 中間的 library | `libc.so.6`、`ld-linux-x86-64.so.2` | **動態連結**的 library 就映射在這裡（上一講） |
| Stack ↓ | `[stack]`，在 `0x7fff...` | `0x7fff455d9000` ≈ 2⁴⁷ = `0x800000000000`，印證 **47 位元** |
| Kernel | `ffffffffff600000` 的 `[vsyscall]` | 上半部（kernel 區）唯一露出來的一小塊，是舊式的快速 system call 機制 |

權限欄（`r-xp` 之類）：**r** 讀、**w** 寫、**x** 執行、**p** 私有。就是 [§10](#10-page-table-entry-裡的保護位元) 的保護位元。

### 7.4 關鍵觀察：高位元變化少

`[24:30]` 要利用的規律：

> 【口頭】寫數字 17、18、19、20……**哪一位變得比較多？個位數**變得多，十位數變得少。

一般來說，**低位元比較「亂」，高位元比較「不亂」**。

`[25:30]` 所以看所有位址的**最高 9 位元**（教授一開始說 8，後來更正為 9），預期只會看到**一兩種**組合。因為 memory map 長這樣：0 附近用一點、中間用一塊、上面用一塊。

【課本補充】課本用十進位說明（p.45）：從 0 數到 999，個位數**每次**都變，十位數每 10 個數變一次，百位數每 100 個數變一次。二進位位址也一樣：**越高位的位元變化越少**。

### 7.5 四層 page table 與 CR3

`[26:03]`（課本 Fig 2.11）

```
 48 位元虛擬位址（上面 16 位元假設為 0）
 ┌──────────┬──────────┬──────────┬──────────┬──────────────┐
 │ 位元40-48 │ 位元31-39 │ 位元22-30 │ 位元13-21 │ 位元1-12      │
 │  (9 位元) │  (9 位元) │  (9 位元) │  (9 位元) │ page 內偏移   │
 └────┬─────┴────┬─────┴────┬─────┴────┬─────┴──────────────┘
      │          │          │          │
 CR3 ─┴► Level 1 ┴► Level 2 ┴► Level 3 ┴► Level 4 ──► page table entry
         (大多空) (大多空)   (多一點)   (大多有)        → 52 位元 frame 位址
```

（位元編號從 **1** 開始數，跟教授、課本一樣。）

**CR3 暫存器** `[26:32]`：

- x86 的一個**特權暫存器**（第 2 講講過的控制暫存器）。
- 存 **page table 的起點**（level 1 表的位址）。
- 換入一個新 process 時，也要**換入它的 page table 位址**——**CR3 是 context 的一部分**。

**一層一層走**：

1. 用**最高 9 位元**當索引，查 **level 1 表**。預期大部分格子是**空的**。
2. 非空的格子指向一個 **level 2 表**。用下一組 9 位元查。也預期大部分是空的。
3. 繼續 **level 3**、**level 4**。越下層越滿。
4. 到 level 4，位元用完了（剩下 12 位元是 page 內偏移，不用來映射），得到一個 **page table entry（PTE）**。
5. PTE 指向 **52 位元的 frame 位址**——如果 frame 在記憶體裡。如果還沒配置、或被搬到別處，PTE 裡也會記錄。

`[29:31]` 總結位元分配：

```
 64 位元 → 實際用 48 位元 = 36 + 12
 36 = 4 × 9  → 四層 page table，每層 9 位元（每張表 2⁹ = 512 格）
 12          → page 內偏移
```

`[30:33]` **為什麼這設計行得通？因為數字的性質**：低位元變化多、高位元變化少。

> 【口頭】「如果反過來，你會有**太多 level 4 page table**，這設計就不緊湊了。它緊湊是因為**大部分格子是空的**，空的格子不指向任何下層表，那些記憶體就省下來了。」

`[31:01]` 也有**五層**的版本（L1 到 L5），道理一樣。

`[31:31]` 「一張有 2³⁶ 格的大表都太多了。而且你**也不需要**大表，因為大部分空間是空的。這種設計能**緊湊地表示**用到的虛擬位址空間，又能很容易地轉換。」

【課本補充】課本補充：要走過 4 層，最壞情況要讀 4 次主記憶體，**超過 1000 個 cycle**。而大約**三分之一**的指令是記憶體存取，每次都這樣走根本不可行——所以需要 TLB（§8）。

### 7.6 【Q&A】每個 level 1 的格子都指向 level 2 嗎？

`[32:06]`

**學生問**：level 1 page table 的每一格都會指向一個 level 2 嗎？

**教授答**：**只有有用到的才會**。如果虛擬位址空間裡有位址的上面 9 位元是那個值，那一格才會填；沒有的話就不填。

> 「這就是省空間的地方。我們**押寶**的就是：level 1 和 level 2 的大部分格子都是空的。有用到才填，空的就空著。」

`[33:00]` 「page table 是很重要的概念，**理解為什麼這樣設計、為什麼不那樣設計**也很重要。」

### 7.7 【原始碼】x86-64 的四層：PGD、PUD、PMD、PTE

Linux 給每一層取了名字：

```c
// arch/x86/include/asm/pgtable_64_types.h:72-97（四層版本）
/*
 * PGDIR_SHIFT determines what a top-level page table entry can map
 */
#define PGDIR_SHIFT		39
#define PTRS_PER_PGD		512
#define MAX_PTRS_PER_P4D	1

#endif /* CONFIG_X86_5LEVEL */

/*
 * 3rd level page
 */
#define PUD_SHIFT	30
#define PTRS_PER_PUD	512

/*
 * PMD_SHIFT determines the size of the area a middle-level
 * page table can map
 */
#define PMD_SHIFT	21
#define PTRS_PER_PMD	512

/*
 * entries per page directory level
 */
#define PTRS_PER_PTE	512
```

對照課本的 L1–L4：

| 課本 | Linux 名稱 | 全名 | `*_SHIFT` | 用哪些位元（從 0 數） | 課本的位元（從 1 數） | 格數 |
|---|---|---|---|---|---|---|
| Level 1 | **PGD** | Page Global Directory | 39 | 39–47 | 40–48 | 512 |
| （五層才有） | P4D | Page 4th-level Directory | — | 四層時只有 1 格（`MAX_PTRS_PER_P4D 1`），等於跳過 | — | 1 |
| Level 2 | **PUD** | Page Upper Directory | 30 | 30–38 | 31–39 | 512 |
| Level 3 | **PMD** | Page Middle Directory | 21 | 21–29 | 22–30 | 512 |
| Level 4 | **PTE** | Page Table Entry | 12（`PAGE_SHIFT`） | 12–20 | 13–21 | 512 |
| 偏移 | — | — | — | 0–11 | 1–12 | — |

- `*_SHIFT`：這一層的索引**從第幾個位元開始**（從 0 數）。例如 `PGDIR_SHIFT 39` → 取位元 39 以上的 9 位元。
- **每層都是 512 格** = 2⁹，跟教授說的「每層 9 位元」一樣。
- 也可以反過來看：一個 PGD 格子管 2³⁹ = **512 GB**，一個 PUD 格子管 2³⁰ = **1 GB**，一個 PMD 格子管 2²¹ = **2 MB**，一個 PTE 管 2¹² = **4 KB**。這就是 huge page 為什麼是 **2 MB** 和 **1 GB**——直接在 PMD 或 PUD 那層停下來。

### 7.8 【原始碼】kernel 自己走一次 page table

page table 走訪平常是硬體做的（見 §8），但 kernel 有時候也要自己走。x86 的一個例子：

```c
// arch/x86/mm/pat/set_memory.c:661-700
pte_t *lookup_address_in_pgd(pgd_t *pgd, unsigned long address,
			     unsigned int *level)
{
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;

	*level = PG_LEVEL_NONE;

	if (pgd_none(*pgd))
		return NULL;

	p4d = p4d_offset(pgd, address);
	if (p4d_none(*p4d))
		return NULL;

	*level = PG_LEVEL_512G;
	if (p4d_large(*p4d) || !p4d_present(*p4d))
		return (pte_t *)p4d;

	pud = pud_offset(p4d, address);
	if (pud_none(*pud))
		return NULL;

	*level = PG_LEVEL_1G;
	if (pud_large(*pud) || !pud_present(*pud))
		return (pte_t *)pud;

	pmd = pmd_offset(pud, address);
	if (pmd_none(*pmd))
		return NULL;

	*level = PG_LEVEL_2M;
	if (pmd_large(*pmd) || !pmd_present(*pmd))
		return (pte_t *)pmd;

	*level = PG_LEVEL_4K;

	return pte_offset_kernel(pmd, address);
}
```

這段完全就是課本 Fig 2.11 的程式版本：

| 行 | 對應的步驟 |
|---|---|
| 661 | 傳入 PGD 裡的某一格（已經用最高 9 位元選好了）和虛擬位址 |
| 670–671 | **`pgd_none`：這格是空的** → 回傳 `NULL`。這就是「大部分格子是空的、空的就不指向下層」 |
| 673 | `p4d_offset(pgd, address)`：用位址裡對應的位元，在下一層表裡找到那一格 |
| 681–683 | 同樣的模式：PUD 層，空的就停 |
| 686–687 | **`pud_large`**：如果這一格直接是一個 **1 GB 的 huge page**，就在這裡停 |
| 689–695 | PMD 層，`pmd_large` → **2 MB huge page** |
| 697–699 | 走到底：用 `pte_offset_kernel` 找到 **PTE**，是一般的 **4 KB page** |

`*level` 回傳最後停在哪一層：`PG_LEVEL_1G`、`PG_LEVEL_2M`、`PG_LEVEL_4K`——就是 [§7.7](#77-原始碼x86-64-的四層pgdpudpmdpte) 算出來的那三種大小。

### 7.9 【原始碼】換 process 時換 CR3

教授說「CR3 是 context 的一部分，換 process 時要換」。實際寫 CR3 的地方：

```c
// arch/x86/include/asm/special_insns.h:52-55
static inline void native_write_cr3(unsigned long val)
{
	asm volatile("mov %0,%%cr3": : "r" (val) : "memory");
}
```

就一條指令：`mov 某暫存器, %cr3`。這是**特權指令**，只有 ring 0 能執行。

誰呼叫它？context switch 換記憶體空間的路徑：

```c
// arch/x86/mm/tlb.c:277-294
static void load_new_mm_cr3(pgd_t *pgdir, u16 new_asid, bool need_flush)
{
	unsigned long new_mm_cr3;

	if (need_flush) {
		invalidate_user_asid(new_asid);
		new_mm_cr3 = build_cr3(pgdir, new_asid);
	} else {
		new_mm_cr3 = build_cr3_noflush(pgdir, new_asid);
	}

	/*
	 * Caution: many callers of this function expect
	 * that load_cr3() is serializing and orders TLB
	 * fills with respect to the mm_cpumask writes.
	 */
	write_cr3(new_mm_cr3);
}
```

| 部分 | 意思 |
|---|---|
| `pgdir` | 新 process 的 **PGD**（level 1 表）的位址 |
| `build_cr3(pgdir, new_asid)` | 把 PGD 位址和一個 **ASID** 組成要寫進 CR3 的值 |
| `need_flush` | 要不要**清掉 TLB** 裡這個 ASID 的舊翻譯 |
| `write_cr3(...)` | 真正寫進 CR3 |

**ASID**（Address Space ID）是什麼？換 process 時，TLB 裡存的是**舊 process** 的翻譯，照理要全部清掉。有了 ASID，TLB 的每一格都標記「這是哪個位址空間的」，就**不用每次都清**。（細節第 6 章 §6.2.4「Managing the TLB」，課本稱這類標記為 PCID。）

---

## 8. TLB：讓 page table 變得實用

`[33:00]` 問題：

- 每次 load / store 都去查 page table：讀 CR3 → level 1 → level 2 → level 3 → level 4……
- 「這就像**每條 load / store 指令都要跑一個小程式**。」
- load / store 大約佔 **30%** 以上的指令。**太沒效率了。**

`[34:00]` 解法：利用**空間與時間局部性**。

```
   Core ──► TLB（64–128 格，現代更多）
             │ 命中（hit）：一個 cycle 以內拿到翻譯
             │
             │ 沒中（miss）
             ▼
          走 page table（慢：幾百到幾千個 cycle）
             │ 兩種走法：
             │  ① 產生 fault，交給 OS 用軟體走
             │  ② 硬體 page walker 直接讀 CR3、自己走（x86）
             ▼
          把翻譯放進 TLB
```

| 項目 | 內容 |
|---|---|
| **TLB** | Translation Lookaside Buffer，**轉譯後備緩衝區**。一個小的**硬體快取** |
| 存什麼 | 最近常用的 **64 個左右**的虛擬→實體映射 |
| 命中 | 一個 cycle 以內（有時多一點）拿到翻譯，繼續往下 |
| 沒中 | **TLB miss**，去走 page table。走 page table 叫 **walking the page table** |
| 命中率 | **99% 以上** |

`[36:01]` **TLB 一定是硬體**，「不可能是軟體結構」，因為需要的速度只有硬體給得起。

【課本補充】課本補充（p.48–49）：

- 4 KB 的 page 能放 **1024 個 int**。有局部性的話，短時間內大部分存取落在**少數幾個 page**。page 層級的局部性比 cache block 層級**更強**，因為 page 比較大。
- 現代 TLB 有**兩層**（L1 TLB、L2 TLB），大約 **1000 格**。每個核心可能有兩個 TLB：一個給指令、一個給資料。
- L1 TLB 很小，**不到一個 cycle** 就能存取。在大型亂序執行 pipeline 裡幾乎感覺不到，所以「位址轉換基本上是**免費**的」。
- Trivia 2.2.2：**page table 是軟體資料結構**，不是用特製硬體實作的。高效能的 Intel 和 AMD 處理器有**硬體 page walker**。不管誰走，**翻譯一定要先放進 TLB 才能用**。

---

## 9. 解決大小問題：swap space

### 9.1 想法

`[36:31]` 程式要用滿 3 GB，但實際記憶體只有 2 GB。叫它「記憶體不足、走開」？「太嚴苛了。」

`[37:00]` 做法：

- 主記憶體 2 GB 照用。
- 從**硬碟**切 1 GB 出來，當成**記憶體空間的延伸**。
- 這塊叫 **swap space（交換空間）**。

`[37:33]` 有了虛擬記憶體機制，讓記憶體空間「流進」儲存裝置**很簡單**：只要**修改 page table entry**，多存一點狀態：

- frame **在不在主記憶體**？
- 在 → 存主記憶體位址。
- 不在 → 它在 swap space，存 swap space 裡的位址。

【口頭】教授的比喻 `[38:00]`：

> 「突然有 20 個客人來你家。10 個住你家，10 個住旅館——旅館就是 swap space。要知道某個客人在家還是旅館，每個客人只要記**一個位元**。你可以列一張客人清單：在我家的話住哪個房間？在旅館的話房號幾號？」

`[39:00]` **swap space 不一定要在你的硬碟**：

- 可以在 USB 隨身碟、SSD、NVMe 硬碟。
- 甚至可以在**網路另一端朋友的機器**上。

> 【口頭】「我在印度，朋友在阿根廷，他硬碟的 1 GB 可以當我機器的 swap space。有什麼問題？我只要在 page table entry 裡加上**朋友機器的 IP 位址**，再加上 frame 在他 swap space 裡的位址。」

### 9.2 存取流程

`[40:01]` 有一個前提：**frame 要能用，一定要在主記憶體裡**。在 swap space 裡就要**先搬進主記憶體**。

```
 存取一個虛擬位址
   │
   ▼
 TLB 命中？ ── 是 ──► 用實體位址去存取
   │ 否
   ▼
 走 page table
   │
   ├─ frame 在主記憶體 ──► 更新 TLB ──► 重新執行
   │
   ├─ frame 在 swap space ──► 搬進主記憶體（主記憶體滿了要先踢一個出去）
   │                            ──► 更新 page table ──► 更新 TLB ──► 重新執行
   │
   └─ 位址非法 ──► exception（可能終止程式）
```

`[40:31]` 「映射一定要先放進 TLB，才能繼續。」

【課本補充】課本說：大多數現代處理器**不能直接用** page table 查到的映射，要先加進 TLB，再**重新發出**那條記憶體指令；第二次就會在 TLB 裡找到。加新翻譯時可能要踢掉 TLB 裡舊的一格，可以用 **LRU**。

### 9.3 主記憶體滿了：要踢掉一個 frame

`[41:31]` 主記憶體可能**滿了**，沒有空的 frame。但 swap space 裡的東西**不能直接用**，所以要**騰出空間**：

1. 從主記憶體選一個 frame，**踢到（evict）** swap space。
2. 改相關的 page table：
   - 如果那個 frame 是**自己 process** 的，改自己的 page table。
   - 如果是**別的 process** 的，比較複雜：那個 process 可能正在用這筆資料，要嘛讓它停一下，要嘛有**同步問題**要處理。
3. 把那筆映射**從 TLB 移除**。
4. 空出來後，把新 frame 搬進來。
5. 在 page table 記下「某虛擬位址的 frame 現在在主記憶體」。
6. 把新映射放進 TLB——「不放進 TLB，CPU 就不能用這個映射。」

`[43:00]` 【口頭】教授要大家**背兩件事**：

> 1. **沒放進 TLB，CPU 就不能用那個映射。** 映射在 swap space 裡沒用，一定要進 TLB。
> 2. **frame 沒進主記憶體，內容就不能用。** 在 swap space 裡可以待著，但要用就要先搬進主記憶體。

（當然有系統會最佳化這些，但這裡先不看那麼複雜的。）

`[43:30]` 選哪個 frame 踢出去？有很多演算法：**FIFO、random、LRU**……之後再看。

### 9.4 Page fault：hard 與 soft

`[44:01]` 在主記憶體找不到 frame、要去 swap space 拿的事件叫 **page fault（分頁錯誤）**。

| 類型 | 情況 | 速度 |
|---|---|---|
| **Hard page fault** | frame 要**實際從 swap space 搬進來** | 慢 |
| **Soft page fault** | frame **在記憶體裡**，但**沒有權限**存取 | — |

（這呼應第 2 講：page fault 是 exception，但**不會終止程式**，處理完要**重新執行同一條指令**。）

【課本補充】Definition 2.2.7：soft page fault 一般會被拒絕（產生 exception）。但也有別的用途：像 **MGLRU** 這種 page 置換演算法會**故意製造** soft page fault 來**追蹤 page 被存取的情況**，之後再改權限位元放行。（MGLRU 是第 32 講。）

---

## 10. Page table entry 裡的保護位元

`[44:30]` 每個 TLB entry 和 PTE 還能存**額外資訊**：

| 資訊 | 用途 |
|---|---|
| **在主記憶體還是 swap space** | 上面講的 |
| **能不能寫** | **程式碼 page 不能寫**，不然惡意程式會改你的程式（像病毒） |
| **使用者 page 還是 kernel page** | 一般 process 不能碰 kernel page |
| **能不能執行** | |
| **有沒有被寫過（dirty）** | 見下 |

`[45:32]` **dirty 位元**的用途：

- page 被**寫過** → 踢出去時，要把修改**寫回** swap space。
- page 已經在 swap space 有一份、主記憶體這份**沒被寫過** → 踢出去時**直接丟掉**就好，不用寫。

`[46:00]` 這些資訊為了**安全和正確性**，放在 PTE 裡，有時也放在 TLB entry 裡。例如別的 process 想寫這個 page，可以直接說「你沒有權限」。

【課本補充】課本補充（p.50）：

- 可以有三個權限位元：**讀、寫、執行**。例如 `101` = 能讀、能執行、不能寫。
- **只能執行（execute-only）** 的 page：有授權需求時，開發者不想讓使用者程式**讀**正在執行的程式碼（容易找到漏洞）。
- 還可以有一個位元表示「**只有 kernel 能存取**」。大部分 kernel page 在使用者空間都不能存取。

`[46:00]` 下一講：效率問題，以及 **segmented memory（分段記憶體）**。

### 10.1 【原始碼】x86 PTE 的每個位元

x86 的 PTE 是 64 位元，每個位元的意義：

```c
// arch/x86/include/asm/pgtable_types.h:10-29
#define _PAGE_BIT_PRESENT	0	/* is present */
#define _PAGE_BIT_RW		1	/* writeable */
#define _PAGE_BIT_USER		2	/* userspace addressable */
#define _PAGE_BIT_PWT		3	/* page write through */
#define _PAGE_BIT_PCD		4	/* page cache disabled */
#define _PAGE_BIT_ACCESSED	5	/* was accessed (raised by CPU) */
#define _PAGE_BIT_DIRTY		6	/* was written to (raised by CPU) */
#define _PAGE_BIT_PSE		7	/* 4 MB (or 2MB) page */
#define _PAGE_BIT_PAT		7	/* on 4KB pages */
#define _PAGE_BIT_GLOBAL	8	/* Global TLB entry PPro+ */
#define _PAGE_BIT_SOFTW1	9	/* available for programmer */
#define _PAGE_BIT_SOFTW2	10	/* " */
#define _PAGE_BIT_SOFTW3	11	/* " */
#define _PAGE_BIT_PAT_LARGE	12	/* On 2MB or 1GB pages */
#define _PAGE_BIT_SOFTW4	58	/* available for programmer */
#define _PAGE_BIT_PKEY_BIT0	59	/* Protection Keys, bit 1/4 */
#define _PAGE_BIT_PKEY_BIT1	60	/* Protection Keys, bit 2/4 */
#define _PAGE_BIT_PKEY_BIT2	61	/* Protection Keys, bit 3/4 */
#define _PAGE_BIT_PKEY_BIT3	62	/* Protection Keys, bit 4/4 */
#define _PAGE_BIT_NX		63	/* No execute: only valid after cpuid check */
```

對照教授列的資訊：

| 教授 / 課本說的 | x86 位元 | 位元號 | 說明 |
|---|---|---|---|
| 在不在主記憶體 | **`PRESENT`** | 0 | 1 = 在記憶體。**0 時，其他位元可以拿來記 swap 位置**（見下） |
| 能不能寫 | **`RW`** | 1 | 1 = 可寫 |
| 使用者 / kernel | **`USER`** | 2 | 1 = 使用者可存取；0 = 只有 kernel |
| 有沒有被存取過 | **`ACCESSED`** | 5 | 註解寫「raised by CPU」：**硬體**存取時自動設成 1。置換演算法用它判斷誰最近有被用 |
| 有沒有被寫過 | **`DIRTY`** | 6 | 也是 **CPU 自動設**。踢出去時看它決定要不要寫回 |
| huge page | **`PSE`** | 7 | 在 PMD / PUD 層設這個位元 → 2 MB / 1 GB page（上面 `pmd_large()` 檢查的就是它） |
| TLB 全域 | `GLOBAL` | 8 | 換 process 時這筆 TLB 翻譯**不用清**。kernel 的映射每個 process 都一樣，就設這個 |
| 能不能執行 | **`NX`** | **63** | **No eXecute**：1 = **不能**執行。注意是**反過來**的：x86 沒有「可執行」位元，只有「不可執行」 |
| 給軟體用 | `SOFTW1`–`SOFTW4` | 9–11、58 | 硬體不管，**OS 自己拿來記東西** |

- 位元 **12–51** 中間沒列出來的，就是 **frame 編號**（40 位元，配合 `PAGE_SHIFT 12` → 52 位元實體位址，跟 [§7.2](#72-64-位元其實只用-48-位元) 的 `__PHYSICAL_MASK_SHIFT 52` 對得上）。
- 這裡沒有「讀」位元：x86 的規則是 **`PRESENT` 就代表能讀**。所以課本說的「讀、寫、執行三個位元」在 x86 上是 **PRESENT、RW、NX**。

緊接著的註解：

```c
// arch/x86/include/asm/pgtable_types.h:37-39
/* If _PAGE_BIT_PRESENT is clear, we use these: */
/* - if the user mapped it with PROT_NONE; pte_present gives true */
#define _PAGE_BIT_PROTNONE	_PAGE_BIT_GLOBAL
```

「**如果 `PRESENT` 是 0，我們把這些位元拿來用**」——`PRESENT = 0` 時硬體完全不看其他位元，所以 kernel 可以**重新利用**這些位元，例如記錄「這個 page 在 swap space 的哪裡」。這就是教授說的「在 PTE 裡存 swap space 的位址」的實際做法。

---

## 11. 【課本補充】共享記憶體與反向頁表

教授這一講只提了一句「刻意的重疊」，課本 p.50–51 多講了兩件事：

### 共享記憶體通道

- 一般來說，兩個 process 的虛擬位址空間要分開（正確性、安全性）。
- 但有時候**刻意**讓兩個 process 的虛擬 page 對應到**同一個 frame**。
- 要**非常受控**：兩個 process 和 OS 都同意。
- 好處：當作**共享記憶體資料通道**，兩個 process 傳資料很快，**不用 OS 介入**。
- Linux 的標準 C 函式庫有 **`shmget`**（建立共享記憶體區段）和 **`shmat`**（掛上去）。

### 反向頁表（Inverted page table）

- page table：虛擬 page → frame。
- **反向頁表**：frame → **所有**對應到它的虛擬 page（跨 process）。這個過程叫 **reverse mapping（反向映射）**。
- 用途：
  - 置換演算法決定踢掉某個 frame → 要更新**所有**指向它的 page table。
  - 改某個 frame 的權限位元。
  - 複製 process（`fork`、`clone`）時。
  - Intel **SGX**、**TDX** 這種安全系統，要確保安全 page 不會被映射到不安全的 process。
- frame 數量比虛擬 page 少很多，而且不分 process，所以做得到。

（反向映射是第 31 講的主題，課本 §6.3.1。）

---

## 12. 一頁速查表

### 12.1 三個問題與解法

| 問題 | 解法 |
|---|---|
| 相容（虛擬空間比實體大） | page → frame 映射，只映射用到的部分 |
| 重疊（process 互相干擾） | 不讓兩個 process 的 page 對應到同一個 frame |
| 大小（記憶體不夠） | swap space：用硬碟延伸實體位址空間 |

### 12.2 名詞

| 名詞 | 意思 |
|---|---|
| 虛擬位址 | CPU / 程式看到的位址 |
| 實體位址 | 送到記憶體系統（快取）的位址 |
| Page / frame | 虛擬 / 實體的 4 KB 單位 |
| Page table | 虛擬 page → frame 的映射表，軟體資料結構 |
| PTE | page table 最下層的一格，存 frame 位址與權限 |
| CR3 | 存 page table 起點的特權暫存器；context 的一部分 |
| TLB | 存最近翻譯的硬體快取；命中率 99%+ |
| Page walk | TLB miss 時走 page table；x86 用硬體走 |
| Swap space | 硬碟上延伸記憶體的區域，可以在別台機器 |
| Hard page fault | frame 在 swap，要搬進來 |
| Soft page fault | frame 在記憶體，但沒權限 |
| 內部碎片 | 分配區之內浪費 |
| 外部碎片 | 分配區之間的洞太小、不連續 |

### 12.3 數字

| 項目 | 值 |
|---|---|
| Page 大小 | 4 KB = 2¹²（`PAGE_SHIFT 12`） |
| Huge page | 2 MB（PMD 層）、1 GB（PUD 層） |
| 32 位元 Linux | 使用者 3 GB / kernel 1 GB（`PAGE_OFFSET 0xC0000000`） |
| 64 位元虛擬位址 | 課本：48 位元；原始碼：使用者 47 位元（下半）+ kernel（上半） |
| 實體位址 | 52 位元 |
| 四層 page table | 4 × 9 + 12 = 48；每層 512 格 |
| 走完四層 | 最壞 1000+ cycle |
| TLB | 64–128 格（現代兩層約 1000 格），命中率 99%+ |

### 12.4 四層名稱

| 課本 | Linux | 位元（從 0 數） | 一格管多大 |
|---|---|---|---|
| L1 | PGD | 39–47 | 512 GB |
| （五層） | P4D | — | — |
| L2 | PUD | 30–38 | 1 GB |
| L3 | PMD | 21–29 | 2 MB |
| L4 | PTE | 12–20 | 4 KB |

### 12.5 x86 PTE 位元

| 位元 | 名稱 | 意思 |
|---|---|---|
| 0 | PRESENT | 在記憶體；0 時其他位元給 OS 用 |
| 1 | RW | 可寫 |
| 2 | USER | 使用者可存取 |
| 5 | ACCESSED | CPU 自動設：被存取過 |
| 6 | DIRTY | CPU 自動設：被寫過 |
| 7 | PSE | huge page |
| 12–51 | — | frame 編號 |
| 63 | NX | 不可執行 |

### 12.6 本講看過的原始碼位置

| 檔案:行號 | 看什麼 |
|---|---|
| `arch/x86/Kconfig:1473-1480` | `PAGE_OFFSET`：32 位元 3G/1G 分界 |
| `arch/x86/include/asm/page_types.h:10-11` | `PAGE_SHIFT`、`PAGE_SIZE` |
| `arch/x86/include/asm/page_64_types.h:54-60` | 實體 52 位元、虛擬 47 / 56 位元 |
| `arch/x86/include/asm/pgtable_64_types.h:72-97` | PGD / PUD / PMD / PTE 的 shift 和格數 |
| `arch/x86/mm/pat/set_memory.c:661-700` | `lookup_address_in_pgd()`：軟體走 page table |
| `arch/x86/include/asm/special_insns.h:52-55` | `native_write_cr3()` |
| `arch/x86/mm/tlb.c:277-294` | `load_new_mm_cr3()`：換 process 時換 CR3 |
| `arch/x86/include/asm/pgtable_types.h:10-39` | PTE 每個位元的定義 |

---

## 13. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **效率問題、segmented memory、`cs`/`ss`** | **第 6 講**：segmentation 與 I/O（下一講） |
| **CR3 是 context 的一部分** | **第 12–13 講**：context switch 細節 |
| **每個 process 的 memory map、`mm_struct`** | **第 9 講**：`mm_struct` 與 maple tree |
| **`fork` 時要複製 / 共享 page** | **第 11 講**：process 建立 |
| **Page fault 是 exception** | **第 16 講**：exception |
| **base–limit「kernel 某些部分還在用」、置換演算法 FIFO / LRU** | **第 29 講**：傳統頁面置換演算法（課本 §6.1） |
| **Huge page、TLB 管理、ASID / PCID** | **第 30 講**：kernel 的記憶體架構（課本 §6.2） |
| **反向頁表、reverse mapping** | **第 31 講**：reverse mapping |
| **Soft page fault 追蹤存取、ACCESSED 位元** | **第 32 講**：MGLRU |
| **記憶體虛擬化（guest 也有 page table）** | 課本第 8 章。對應講次**待補** |

---

## 附：本講一句話總結

> **每個 process 都以為自己獨佔一個標準化的巨大記憶體空間；OS 把它切成 4 KB 的 page，透過 CR3 指向的四層 page table（PGD→PUD→PMD→PTE，利用「高位元變化少」讓大部分格子空著）對應到實體 frame，用 TLB 讓轉換幾乎免費，再用 swap space 把實體空間延伸到硬碟——一次解決相容、重疊、大小三個問題。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 5（Chapter 2, Part IV）錄影逐字稿（YouTube 自動字幕，46:56）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§2.2–2.2.3，以及 Linux kernel v6.2.12 原始碼整理。memory map 為本機（Ubuntu、kernel 6.5.0）實測。*
