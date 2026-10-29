# Basic Computer Architecture 第三十七支影片完整筆記：Virtual memory：page、frame、page table、TLB、swap space（Chapter 10 The Memory System, Part 4/4 → 課本 Ch11）

> **課程**：Basic Computer Architecture — Chapt 10: The Memory System, Part 4/4（播放清單第 37 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（1:06:46，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 **Ch11** §11.4（p.539–556）
> **章號**：影片的 Chapter 10 是課本 v3.09 的 **Chapter 11**；影片說的「chapter 11」（平行）「chapter 12」（I/O、硬碟、memory mapping）是課本的 Ch12、Ch13。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 兩個問題：overlap 與 size](#1-兩個問題overlap-與-size)
- [2. Physical memory 與 virtual memory](#2-physical-memory-與-virtual-memory)
- [3. 一個 process 的 memory map](#3-一個-process-的-memory-map)
- [4. 使用者與核心怎麼分 4 GB](#4-使用者與核心怎麼分-4-gb)
- [5. Address translation system](#5-address-translation-system)
- [6. Page 與 frame](#6-page-與-frame)
- [7. 整個 virtual memory 的核心：page 對到 frame](#7-整個-virtual-memory-的核心page-對到-frame)
- [8. Page table 與單層 page table](#8-page-table-與單層-page-table)
- [9. 兩層 page table](#9-兩層-page-table)
- [10. Inverted page table](#10-inverted-page-table)
- [11. TLB](#11-tlb)
- [12. Page fault](#12-page-fault)
- [13. Swap space](#13-swap-space)
- [14. 一次記憶體存取的完整流程](#14-一次記憶體存取的完整流程)
- [15. 進階功能：shared memory 與 protection](#15-進階功能shared-memory-與-protection)
- [16. 【原始碼】Linux 的 page table](#16-原始碼linux-的-page-table)
- [17. 【實測】](#17-實測)
- [18. 一頁速查表](#18-一頁速查表)
- [19. 與後續講次的連結](#19-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

【口頭】[00:00]「virtual memory 是個很簡單的概念，但大部分學生不知怎麼地都會搞錯。」這講的目標是讓大家都有像樣的理解。

```
問題：overlap（程式互相蓋掉）、size（實體記憶體比程式要的小）
解法：每個 process 看到自己的 virtual memory → address translation → physical memory
  切成 4 KB：virtual 的叫 page，physical 的叫 frame；page table 把 page 對到 frame
  page table：單層（2.5 MB／process，太大）→ 兩層（10 + 10 bit，只配用到的）→ inverted（全系統一張）
  TLB：page table 的小 cache，hit rate 約 99%
  page fault：找不到 → 配一個 frame；不夠就把一個 frame 趕到 swap space
進階：shared memory（兩個 page 對同一個 frame）、protection（唯讀 bit）
```

---

## 1. 兩個問題：overlap 與 size

[00:43]–[05:14]　課本 §11.4 開頭、§11.4.2（p.539、p.546）

到目前為止的假設：32 位元機器上，每個程式都以為能用位址 0 到 2³² − 1 的**全部** 4 GB（2³² = 4 × 2³⁰ = 4 GB），想存取哪裡就存取哪裡。

**問題 ①：overlap problem（重疊問題）**
- 【口頭】[02:12] 老師的機器上大概有 100 個程式在跑。CPU 跑 A 一陣子、切到 B、再切到 C：叫 **time-shared system**（分時系統）。
- A 和 B 都以為整個記憶體是自己的。如果 A 寫位址 20、B 也寫位址 20，就會互相蓋掉。
- 但實際上它們**不會**互相弄壞資料。怎麼做到的？

**問題 ②：size problem（大小問題）**
- 實體記憶體可能只有 2 GB，比 4 GB 的位址空間小。程式存取 3 GB 之後的位址，還是要能跑。

【課本補充】p.539：
- 課本寫作時作者的工作站上有 **232** 個程式在跑；Windows 用工作管理員、Linux／Mac 用 `ps -ef` 就能看。
- 以前主記憶體只有幾 MB，使用者卻能跑要幾百 MB 的程式。現在寫一個 C 程式配置比實體記憶體還大的資料結構，「大部分系統也能編譯、執行成功」（【實測】見 [§17.3](#173-demand-paging-與-size-problem)）。

---

## 2. Physical memory 與 virtual memory

[05:14]–[07:50]　課本 Definition 111（p.542）

| | 定義 |
|---|---|
| **Physical memory**（實體記憶體） | **真正有**的記憶體：主記憶體裡由 DRAM 晶片組成的那些位置。inclusive cache 時，大小就是主記憶體的大小，看你買了多少硬體 |
| **Virtual memory**（虛擬記憶體） | 程式**以為**它有的記憶體：連續、沒有限制。位址 n bit → 大小 2ⁿ |

- 32 位元：2³² bytes = 4 GB。
- 64 位元：2⁶⁴ bytes ≈ 16 × 10¹⁸ bytes，「非常巨大」。

【口頭】[07:07]「一個是理想的夢幻世界（虛擬），一個是真實世界（有真正的儲存裝置）。」

【課本補充】p.542：
- Definition 111：virtual memory 是一個假想的記憶體系統，process 以為它擁有整個記憶體空間，不受其他 process 干擾；大小等於系統可定址的全部記憶體。所有位置的集合叫 **virtual address space**。
- 為什麼不讓程式設計師或編譯器自己避開別人的位址？編譯時不可能知道別的 process 會用哪些位址；而且同一個程式連跑兩份也會撞在一起。所以每個程式都要看到一個**虛擬**的記憶體。

【課本補充】p.540–541 Definition 109–110：**process** 是程式的一次執行實例；程式是檔案系統裡的一串 bytes（binary、executable）。**作業系統**管理機器、周邊裝置和所有 process；核心叫 **kernel**。kernel 裡的 **loader** 把程式內容搬進記憶體、設定好環境，跳到進入點；**scheduler** 排程各個 process（包括 kernel 自己）。每個 process 有一個 **pid**。

---

## 3. 一個 process 的 memory map

[07:50]–[11:54]　課本 Fig 11.17、Definition 112（p.543–545）

Linux（32 位元）上一個 process 的虛擬記憶體長這樣（由下往上）：

```
0xC0000000 ┌──────────────────────┐
           │ stack ↓              │  往低位址長
           │ memory mapping 段     │
           │ heap ↑               │  malloc、new
           │ bss                  │  沒初始化的 static／global，填 0
           │ data                 │  有初始值的 static／global，例如 int x = 10;
0x08048000 │ text                 │  程式的指令
           │ header               │  程式種類、編碼等 meta 資訊
         0 └──────────────────────┘
```

- 【口頭】[08:06] 這是 **ELF** 格式；所有程式都從虛擬位址 0 開始。
- **memory mapping segment**：【口頭】「相當複雜，先不講」。一句話：可以把一段記憶體區域和一個檔案（例如硬碟上的電影檔）的 bytes 一對一對應起來。之後在影片的 chapter 12（課本 Ch13）再談。
- 重點（【口頭】[11:40]）：**每個 process 的 memory map 都差不多**，例如 stack 的位址在各個 process 裡都一樣。那它們怎麼不會互相蓋掉？→ 這就是要解決的問題。

【課本補充】p.543–545：
- Definition 112：**memory map** = 作業系統在記憶體裡擺放程式碼和資料的方式。
- 有些作業系統還有一個放**唯讀資料**（常數）的區域。
- **bss 一定要填 0**，是為了**安全**：如果程式 A 在 bss 留下密碼或信用卡號，程式 B 不先寫就讀，會讀到 A 的資料。
- heap 往上長；heap 裡配置、釋放後會留下**洞**，越來越多叫 **fragmentation**（碎片化），需要好的記憶體管理器。
- memory mapping 段也放**動態連結函式庫**（DLL）。
- 4 GB 裡程式通常只用幾百 MB，heap 和 stack 之間有一大片**空的**區域。

---

## 4. 使用者與核心怎麼分 4 GB

[11:54]–[12:58]　課本 Fig 11.19（p.545）

| 作業系統 | 使用者程式 | 作業系統（kernel） |
|---|---|---|
| Linux（32 位元） | 低 **3 GB** | 高 **1 GB** |
| Windows（32 位元） | 2 GB | 2 GB（50／50） |

【口頭】[12:26]「這是我們第一次聽到作業系統這個詞」：作業系統是一個特別的程式，工作是管理其他程式。

【課本補充】p.545：為什麼要分？作業系統很常執行（處理裝置請求、管理 process），而**換**一個虛擬記憶體視角有點貴。把 kernel 放在每個 process 的高位址，從使用者程式切進 kernel 時就不用換視角。程式只要假設自己的空間少了一點（Linux 是 3 GB），通常 2–3 GB 也遠比程式需要的多。

---

## 5. Address translation system

[12:58]–[17:03]　課本 Fig 11.20、Definition 113（p.546–547）

```
程式 A ── virtual address ──→ [A 的 translation system] ──┐
                                                         ├─→ physical address ──→ 記憶體系統（含 L1）
程式 B ── virtual address ──→ [B 的 translation system] ──┘
```

- 程式（和編譯器）用的位址都是 **virtual address**（虛擬位址）。
- 送進記憶體系統的是 **physical address**（實體位址）。
- 中間是 **address translation system**，也叫 **MMU**（memory management unit，記憶體管理單元）或 virtual-to-physical address translator。
- **每個程式有自己的** translation system；**physical address 對所有程式都一樣**，因為它對應真實的位置。
- 處理器**裡面**都是虛擬位址；出了處理器到記憶體系統（**包括 L1 cache**）就是實體位址。

【課本補充】p.546：像俄羅斯代表團去杜拜需要俄文翻阿拉伯文的翻譯，兩邊都講自己的語言就好。
- 程式這邊的要求：範圍內的位址都能存取；記憶體看起來連續；除非自己要，不受其他程式干擾。
- 記憶體系統這邊的要求：不同程式用不重疊的位址；不能給一個程式一大塊連續的記憶體（會因碎片化浪費）；實體記憶體不夠時要有額外的儲存空間。

---

## 6. Page 與 frame

[17:03]–[21:01]　課本 Definition 114（p.547–548）

和 cache block 一樣，把空間切成塊：

| | 切哪個空間 | 大小 |
|---|---|---|
| **page**（頁） | 虛擬位址空間 | 4 KB |
| **frame**（頁框） | 實體位址空間 | 4 KB（**一定和 page 一樣大**） |

translation system 的工作就是**把 page 對到 frame**。

- 4 KB = 2² × 2¹⁰ = **2¹² bytes** → 位址最低 **12 bit** 是 page 裡的 offset。
- 32 位元位址：丟掉最低 12 bit，剩下 **20 bit** 是 page number，要對到一個 frame number。
- 【口頭】[17:46] 虛擬和實體位址空間可以不一樣大，例如現代處理器的虛擬位址 **48 bit**、實體位址 **36 bit**。（本機是 48／39，見 [§17.1](#171-本機的位址寬度與-page-大小)。）

**為什麼是 4 KB**（【口頭】[19:58]）：
- 用塊來對應，**不用每個 byte 都翻譯**，省力。
- page 太大（例如 100 MB）：大部分用不到，又不能切小 → 浪費記憶體。
- page 太小（例如 8 bytes）：對應的工作量太大。
- 「4 KB 是實驗上找到的好選擇。」

---

## 7. 整個 virtual memory 的核心：page 對到 frame

[21:01]–[28:40]　課本 §11.4.3（p.547–548）

【口頭】[26:34]「這張圖非常非常重要……懂了這張圖就懂了整個 virtual memory；沒懂這張圖，等於什麼都沒懂。」

```
process A 的虛擬記憶體（看起來連續）        實體記憶體（frame 散落各處）
┌──────┐                                  ┌──────┐
│page 0│ ─────────────────────────────┐   │      │
│page 1│ ──────────┐                  └─→ │frame 7│
│page 2│ ───┐      └────────────────────→ │frame 2│
└──────┘    └────────────────────────────→│frame 9│
process B                                  │      │
┌──────┐                                  │      │
│page 0│ ────────────────────────────────→│frame 4│
└──────┘                                  └──────┘
```

- 每個 process 看到的是一整片**連續**的虛擬記憶體，中間的**對應層**它看不到，「這就是秘方」。
- 每個用到的 page 被對到某個 frame，frame **可以在實體記憶體的任何地方**，不用連續。
- 【口頭】[25:01] 程式從一個 page 的最後一個 byte 走到下一個 page 的第一個 byte，程式以為只往前走了 1 byte；實際上從一個 frame 跳到了另一個完全不相干的 frame。
- **一個 frame 只對到一個 page**（除非是故意的，見 §15）→ 兩個 process 不會共用實體記憶體 → **overlap problem 解決**。
- 因為空間局部性，程式用的資料會集中在少數幾個 page。

**size problem 也解決了**（【口頭】[27:53]）：虛擬 4 GB、實體只有 1 GB 也沒關係，反正只是把**用到的** page 對到 frame。只有當 A 和 B 合起來的 working set 超過 1 GB 時實體記憶體才會滿，「但這很少見」（真的滿了就用 §13 的 swap space）。

---

## 8. Page table 與單層 page table

[28:40]–[31:54]　課本 Fig 11.21–11.22（p.548–549）

```
虛擬位址：| page number (20) | offset (12) |
                 ↓ page table
實體位址：| frame number (20) | offset (12) |   ← offset 原封不動
```

做這個翻譯的表叫 **page table**（頁表），每個 process 一張。

**單層 page table**（one-dimensional）：
- 用 20 bit page number 當 index → 2²⁰ ≈ **100 萬**個 entry。
- 每個 entry 存一個 20 bit frame number。
- 就像 direct mapped cache：讀一個 entry 就拿到答案。

**問題：太大**

```
一個 entry = 20 bit = 2.5 bytes
2²⁰ 個 entry × 2.5 bytes = 2.5 MB（每個 process）
200 個 process × 2.5 MB = 500 MB   ← 「不能接受」
```

【課本補充】p.548–549：主記憶體 2 GB 的話，光存 page table 就用掉 **25%**；有些系統甚至根本沒有 500 MB 可用。

**觀察**（【口頭】[31:23]）：虛擬位址空間**大部分是空的**。4 GB 裡程式可能只用 10 MB。

---

## 9. 兩層 page table

[31:54]–[39:57]　課本 Fig 11.23（p.550）

把 20 bit page number 拆成兩個 **10 bit**：

```
| 10 bit | 10 bit | offset 12 bit |
    ↓
primary page table（2¹⁰ = 1024 個 entry）
    ↓ entry 指向一張 secondary page table
secondary page table（1024 個 entry，每個存 20 bit frame number）
    ↓
frame number
```

**為什麼有效**（【口頭】[34:13]–[36:37]）：
- 從 0 數到 1000：000、001……201、202……**最高位很少變，最低位一直變**。
- 記憶體位址也一樣：存取陣列、讀指令時，page number 的低 10 bit 變很多，高 10 bit 很穩定。
- 所以 primary page table 的 1024 個 entry **大部分是空的**，只有少數幾個指向 secondary page table。
- secondary page table **需要時才配**（on demand）。primary 裡只有 3 個有效 entry → 只配 3 張 secondary。

**算一下**：

| | 影片 [37:08] | 課本 p.550 |
|---|---|---|
| 一張 secondary | 1024 × 2.5 B = **2.5 KB**，對應 4 MB 的虛擬記憶體 | 同左 |
| primary | 「最多也差不多這麼大」 | 1024 × 10 bit = **1.25 KB** |
| 例子 | 程式用 100 MB → 25 張 → 2.5 × 25 = **62.5 KB**（＋一點 primary） | 程式用 10 MB → 3 張（text／data／heap 2 張、stack 1 張）→ 1.25 + 3 × 2.5 = **8.75 KB** |
| 和單層比 | 62.5 KB vs 2.5 MB | 8.75 KB（最多 16.25 KB）vs 2.5 MB |

都小了一個數量級以上。「這就是為什麼幾乎所有商用系統都用兩層 page table。」（實際的 Linux 見 [§16](#16-原始碼linux-的-page-table)。）

---

## 10. Inverted page table

[39:57]–[50:56]　課本 Fig 11.24（p.551–552）

**動機**（【口頭】[40:13]）：一張 page table 50 KB、100 個 process → 5 MB。有些記憶體很少、process 卻很多的系統放不下；而且 context switch（換 process）時 page table 也要換。

**想法**：全系統**只有一張**表，**每個 frame 一個 entry**，存「這個 frame 屬於哪個 process 的哪個 page」：

```
inverted page table[frame number] = (page number, process id)
大小：2²⁰ 個 frame × 2.5 B = 2.5 MB（全系統，不管有幾個 process）
```

process 夠多時（例如 100 個 × 50 KB = 5 MB）就比較省空間。

**怎麼查**（要從 page number 找 frame，但表是用 frame 排的）：

```
(page number 20 bit, pid 10 bit) = 30 bit
   ↓ hashing engine（把 30 bit 打亂，輸出 12 bit，分佈均勻）
hash table（2¹² 個 entry）
   ↓ entry 指向 inverted page table 的某一格
比對那一格存的 (page, pid)：
   相同 → 讀出 frame number
   不同 → 往後找 K 格（例如 K = 3）；都不同 → 這個 page 沒有對應
```

- 1000 個 process 需要 **10 bit** 的 pid。
- **hashing**（雜湊）：輸入很多種、輸出比較少種；每個輸入只對到一個輸出，但很多輸入可能對到同一個輸出，所以一定要比對。好的雜湊是 **uniform hashing**（每個輸出機率一樣）。

**為什麼中間要多一張 hash table**（【口頭】[49:03]，「聰明的讀者會問」）：如果直接用雜湊結果去查 inverted page table，有些想保留給作業系統或特殊用途、**不想分配**的 frame，一定會被某些輸入雜湊到。多一層間接，只要 hash table 裡沒有 entry 指向那些 frame 就好。

【口頭】inverted page table「不太流行，但動機很強」，「某些 **IBM** 機器」有用。

【課本補充】p.551–552：
- 課本舉的例子是 **Intel Itanium** 和 **PowerPC 603**（PowerPC 是 IBM 參與設計的，和影片的「IBM 機器」一致）。
- 往後找 K 格叫 **linear probing**（線性探測）。K 格內都找不到，就要像 cache 一樣趕出一個 entry 來建立對應，被趕出的 entry 放在主記憶體的專用區域。
- 「維護和更新 hash table 的成本超過全系統一張表的好處」，所以商用系統**一般不用**。

---

## 11. TLB

[50:56]–[55:06]　課本 p.552

問題：page table 本身也存在**記憶體**裡（不可能為它另外做硬體）。每個 load 都要先去讀 page table（叫 **walking the page table**，走訪頁表）→ 記憶體存取次數**加倍**，很慢。

解法：**TLB**（translation lookaside buffer，轉譯後備緩衝區）：存最近用過的對應的**小 cache**。

```
page number ──→ TLB ──→ frame number
```

| | 影片 | 課本 |
|---|---|---|
| 結構 | set associative 或 fully associative | 小的 fully associative cache |
| entry 數 | 通常 64，也可以 128 | 32–64 |
| 替換 | LRU | — |
| hit rate | 可以超過 97、98、99% | ≈ 99% |

**為什麼 hit rate 這麼高**：時間局部性（一直用同一批 page）＋空間局部性（page 有 4 KB 那麼大，很可能一直待在同一個 page）。TLB miss 時才去 page table 拿對應、填進 TLB。

「TLB 可以想成 **page table 的 cache**。」

【課本補充】p.552：
- 位址在 EX 級算出來就送進 TLB；TLB 很快（不到一個週期），hit 時到 MA 級就已經有實體位址了。
- TLB miss 時 pipeline 要 stall，查 page table 要**幾十個週期**。
- **64 個 entry × 4 KB = 256 KB** 的虛擬位址空間，大部分程式在一小段時間內的 working set 都在這範圍內。（【實測】見 [§17.6](#176-看見-tlb64-個-entry)。）

---

## 12. Page fault

[55:06]–[56:07]　課本 §11.4.5（p.553–555，Definition 115）

**Page fault**（缺頁）：要的 page **不在主記憶體**裡。例如第一次存取一個 page，page table 裡還沒有它的對應。

處理：
1. 在主記憶體配一個**空的 frame**，建立對應。
2. 更新**空 frame 清單**（把用掉的那個拿掉）。MMU 要維護這份清單；MMU 可以用硬體或軟體做。

【課本補充】p.553–554：
- process 開始時，kernel 在主記憶體配一張 primary page table、**清空 TLB**，放進 text、data 的對應。
- TLB miss 時由 **MMU** 走 page table。primary page table 的起始位址**不能是虛擬位址**（不然 page table 也要 page table），所以用**實體位址**，放在專用暫存器（**x86 的 CR3**）或實體記憶體的固定位置。
- **所有 page table 都必須在主記憶體裡**，不能在 swap space。
- 空 frame 清單可以用 **bit vector**（每個 frame 一個 bit，0 空、1 已用）。
- 從 swap space 抓一整個 page 要**幾百萬個週期**；page fault rate 對**page replacement policy**（選哪個 frame 趕出去，例如 FIFO、LRU）非常敏感。細節看作業系統教科書。

---

## 13. Swap space

[56:07]–[60:32]　課本 §11.4.4（p.552–553）

【口頭】[56:07] 問題：主記憶體 500 MB，能不能跑一個要 1 GB 記憶體的程式？答案是**可以**；不允許的話會太限制程式，也要花很多錢買記憶體。

做法：
- page table（和 TLB）的 entry 除了 frame number，再多一個 **bit**：這個 frame 在**主記憶體**還是在**別的地方**。
- 別的地方：**硬碟**。【口頭】主記憶體 8–16 GB，硬碟通常 500 GB 或 1 TB；硬碟用**磁性**儲存，不是半導體，所以慢很多很多。硬碟在影片 chapter 12（課本 Ch13）細講。
- 硬碟上劃一塊專用區域叫 **swap space**，放主記憶體放不下的 frame。

```
處理器 → L1 → L2 → 主記憶體 → 硬碟（swap space）
```

這時**實體記憶體 = 主記憶體 ＋ swap space**。主記憶體快得多，但 swap space 多給了一點額外空間。

⚠️ bit 的意思兩邊相反：影片 [60:14] 說 **0 = 在主記憶體、1 = 在 swap space**；課本 p.553 說 **1 = 在主記憶體**、否則在 swap space。只是慣例不同，概念一樣。

【課本補充】p.552–553：
- 主記憶體存取約 100–300 ns，硬碟是**毫秒**級；儲存裝置的位址和記憶體位址無關，它不屬於記憶體系統；甚至可以在網路另一頭。
- swap space 可以是主記憶體的**延伸**（2 GB + 3 GB = 5 GB 實體記憶體），也可以是 **inclusive** 的（實體記憶體 3 GB，主記憶體當作 swap space 的 cache）。
- 也可以有好幾個 swap space，用好幾個 bit 表示在哪一個。

---

## 14. 一次記憶體存取的完整流程

[60:32]–[62:37]　課本 Fig 11.25（p.555）

```
記憶體存取（load 或 store）
  TLB hit？ ── 是 ──→ 把對應給處理器 → 組出實體位址 → 送去記憶體系統
     否
  page table hit？ ── 是 ──→ 填進 TLB → 把對應給處理器
     否（沒有對應，要建立）
  有空的 frame？ ── 否 ──→ ① 把一個 frame 趕到 swap space ② 更新它的 page table entry
     是
  從 swap space 讀進新的 frame（如果有內容），或建立一個空的 frame
  → 建立／更新 page table 的對應 → 填進 TLB → 把對應給處理器
```

---

## 15. 進階功能：shared memory 與 protection

[62:37]–[66:33]　課本 §11.4.6（p.555–556）

**Shared memory**（共享記憶體，**故意的**）：
- 【口頭】[63:07] producer-consumer 例子：P1 接收網路來的請求，交給 P2；P2 查資料庫，把資料給 P1；P1 再送回客戶端。P1、P2 要共用資料。
- 做法：把**同一個 frame** 對到兩個 page，一個在 P1 的位址空間、一個在 P2 的。P2 寫它的 page，等於寫那個 frame；P1 讀它的 page 就讀得到。
- 「用 virtual memory 做 shared memory 非常非常常見。」

**Protection**（保護）：
- 不想讓 **text 區**（程式的指令）被改。【口頭】有人改了指令，例如讓它亂寄 email 給網路上的人，「就像植入病毒」。
- 做法：page table／TLB entry 加一個 **read-only bit**。設成 1 → 沒有 process 能寫這塊，**連這個 process 自己**都不行；只在第一次初始化時寫入。

【課本補充】p.555–556：
- shared memory 要先告訴 kernel，由 kernel 建立對應；它是 process 之間溝通**最快**的方法之一。
- 病毒常用特殊的錯誤輸入讓程式的變數被蓋掉、變成指向 text 區的指標，進而改指令。把 text 的 page 標成唯讀就防住了。
- **Segmentation**（分段，x86 用）：在虛擬記憶體上再加一層，text、data、stack 各有一個 **segment register**，虛擬位址 = segment register ＋ offset，MA 級加好之後再交給 MMU。（[第 11 支](lecture11_note.md) x86 講過 segment register。）

【口頭】[66:17]「記憶體系統這章結束了」，下一章是影片的 chapter 11：平行程式與多處理器系統。

---

## 16. 【原始碼】Linux 的 page table

**32 位元 x86（沒開 PAE）就是課本的兩層 page table**，`linux-src/arch/x86/include/asm/pgtable-2level_types.h:26-38`：

```c
 * traditional i386 two-level paging structure:
 */

#define PGDIR_SHIFT	22
#define PTRS_PER_PGD	1024


/*
 * the i386 is two-level, so we don't really have any
 * PMD directory physically.
 */

#define PTRS_PER_PTE	1024
```

- `PGDIR_SHIFT 22`：位址的第 22 bit 以上是 primary（Linux 叫 **PGD**，page global directory）的 index → 32 − 22 = **10 bit**，1024 個 entry。
- `PTRS_PER_PTE 1024`：secondary（Linux 叫 **PTE** table）也是 **10 bit**，1024 個 entry。
- 加上 `PAGE_SHIFT 12`（`linux-src/arch/x86/include/asm/page_types.h:10`）的 offset：**10 + 10 + 12**，和課本 Fig 11.23 一模一樣。

**64 位元 x86 用四層**（`linux-src/arch/x86/include/asm/pgtable_64_types.h:75-97`）：

| 層 | Linux 名稱 | shift | 每層 entry 數 | index bit |
|---|---|---|---|---|
| 1 | PGD | 39 | 512 | 47–39（9 bit） |
| 2 | PUD | 30 | 512 | 38–30（9 bit） |
| 3 | PMD | 21 | 512 | 29–21（9 bit） |
| 4 | PTE | 12（PAGE_SHIFT） | 512 | 20–12（9 bit） |

9 × 4 + 12 = **48 bit** 虛擬位址，剛好是本機 CPU 支援的虛擬位址寬度。每個 entry 8 bytes，512 × 8 = 4 KB，**每張表剛好一個 page**。

page table entry 裡的旗標（`linux-src/arch/x86/include/asm/pgtable_types.h:10-29` 節錄）：

```c
#define _PAGE_BIT_PRESENT	0	/* is present */
#define _PAGE_BIT_RW		1	/* writeable */
#define _PAGE_BIT_USER		2	/* userspace addressable */
#define _PAGE_BIT_ACCESSED	5	/* was accessed (raised by CPU) */
#define _PAGE_BIT_DIRTY		6	/* was written to (raised by CPU) */
#define _PAGE_BIT_NX		63	/* No execute: only valid after cpuid check */
```

- **PRESENT**：在不在主記憶體（就是 §13 那個 bit）。
- **RW**：可不可以寫（§15 的 read-only bit，0 就是唯讀）。
- **USER**：使用者程式能不能存取（kernel 那 1 GB 的 page 這個 bit 是 0）。
- **ACCESSED**、**DIRTY**：CPU 自動設；給 page replacement policy 用，就像 cache 的 LRU 資訊和 modified bit。
- **NX**：不可執行。

---

## 17. 【實測】

程式在 `experiments/lecture37/`。

### 17.1 本機的位址寬度與 page 大小

```
$ grep -m1 "address sizes" /proc/cpuinfo
address sizes	: 39 bits physical, 48 bits virtual
$ getconf PAGESIZE
4096
```

- 虛擬位址 **48 bit**、實體位址 **39 bit**，正是影片說的「虛擬和實體位址可以不一樣寬」（影片舉 48／36）。
- page 是 **4 KB**。

### 17.2 Memory map、同一個虛擬位址不同的資料

`vm.c` 的第 1、2 部分（64 位元、有 ASLR）：

```
[1] memory map
    text  main     = 0x55d622737320
    data  &inited  = 0x55d62273a010
    bss   &uninit  = 0x55d62273a040
    heap  malloc16 = 0x55d6242372a0
    mmap  malloc64M= 0x7f7d163ff010
    stack &local   = 0x7fff239ab50c
    55d622737000-55d622738000 r-xp ... vm          ← text：可讀、可執行、不可寫
    55d62273a000-55d62273b000 rw-p ... vm          ← data＋bss：可讀寫
    55d624237000-55d624258000 rw-p ... [heap]
    7f7d1a428000-7f7d1a5bd000 r-xp ... libc.so.6   ← 動態連結函式庫（memory mapping 段）
    7fff2398b000-7fff239ad000 rw-p ... [stack]
[2] fork：同一個虛擬位址 0x55d62273a010
    child : inited = 222
    parent: inited = 10
```

- 順序和課本 Fig 11.17 一樣：text < data < bss < heap ……（libc、大塊 malloc 在 memory mapping 段）…… < stack。
- 大塊的 malloc（64 MB）不在 heap，而是在 memory mapping 段（glibc 對大配置改用 mmap）。
- **fork** 後 parent 和 child 的 `inited` 在**同一個虛擬位址**，child 改成 222，parent 還是 10：同一個 page 在兩個 process 裡對到**不同的 frame**。overlap problem 就是這樣解決的。

用 `gcc -m32 -no-pie` 編成 32 位元再看：

```
main=0x8049196 data=0x804c024 heap=0x9c4e1a0 stack=0xffbe2658
```

text 在 **0x0804xxxx**，就是課本圖上的 0x08048000 附近。⚠️ 但 stack 在 0xff……，不是課本的 0xC0000000：這是在 **64 位元 kernel** 上跑的 32 位元程式，kernel 不在它的 4 GB 裡，所以使用者可以用幾乎整個 4 GB，不是「3 GB／1 GB」。

### 17.3 Demand paging 與 size problem

`vm.c` 第 3 部分：

```
[3] mmap 1 GiB：RSS 1440 KB → 1600 KB，minor faults +3，第 0 頁 present=0
    每頁寫 1 byte 後：RSS 1050240 KB，minor faults +262147（1 GiB / 4 KiB = 262144 頁），第 0 頁 present=1
    256 GiB 的虛擬空間：頭、中間、尾各寫 1 byte → 1 2 3，RSS 只有 1704 KB
```

- **RSS**（resident set size）：這個 process 真正占用的實體記憶體。
- 要了 1 GiB 虛擬記憶體，實體記憶體幾乎沒增加；`/proc/self/pagemap` 顯示第 0 頁 **present = 0**（還沒有 frame）。
- 每頁寫一次之後，RSS 變成約 1 GB，**minor page fault 多了 262,147 次 ≈ 每個 page 一次**：每個 page 第一次被碰時發生 page fault，kernel 才配一個 frame（§12）。這叫 **demand paging**（需要時才配）。
- 本機實體記憶體 38 GiB，卻能要到 **256 GiB** 的虛擬空間，頭、中、尾都能讀寫；RSS 只有 1.7 MB：只有碰過的 3 個 page 有 frame。這就是 size problem 的解法。

### 17.4 Protection：text 是唯讀的

```
[4] 寫進 text 區（main 的第一個 byte）→ SIGSEGV：text 的 page 是唯讀的
```

對應 maps 裡 text 那行的權限 `r-xp`（沒有 w），也就是 page table entry 的 RW bit = 0。

### 17.5 Shared memory

```
[5] MAP_SHARED：child 寫了 777，parent 讀到 777（兩個 page 對到同一個 frame）
```

和 §17.2 的 fork 對照：一般的變數 child 改了 parent 看不到；用 `MAP_SHARED` 要的記憶體，兩個 process 的 page 對到**同一個 frame**，看得到彼此寫的值。

### 17.6 看見 TLB：64 個 entry

`tlb.c`：每個 page 只碰 **1 條 cache line**，用 pointer chasing 隨機繞 n 個 page。line 在 page 裡的位置錯開（第 i 頁用第 i % 64 條），所以資料分散在不同的 cache set；n = 512 時資料也只有 32 KB，放得進 48 KB 的 L1。因此**變慢只會是因為 TLB**。用 `MADV_NOHUGEPAGE` 確定是 4 KB page：

```
$ gcc -O2 -o tlb tlb.c && taskset -c 2 ./tlb
    16 pages (     64 KB of address space,   1 KB of data):  1.22 ns per load
    32 pages (    128 KB of address space,   2 KB of data):  1.08 ns per load
    64 pages (    256 KB of address space,   4 KB of data):  1.07 ns per load
   128 pages (    512 KB of address space,   8 KB of data):  2.56 ns per load
   256 pages (   1024 KB of address space,  16 KB of data):  2.56 ns per load
   512 pages (   2048 KB of address space,  32 KB of data):  2.57 ns per load
  1024 pages (   4096 KB of address space,  64 KB of data):  4.69 ns per load
  2048 pages (   8192 KB of address space, 128 KB of data):  5.94 ns per load
  4096 pages (  16384 KB of address space, 256 KB of data):  8.03 ns per load
 32768 pages ( 131072 KB of address space, 2048 KB of data): 20.76 ns per load
```

- **64 個 page 以內**：1.07 ns，L1 hit、TLB hit。
- **128 個 page**：跳到 2.56 ns，可是資料只有 8 KB，一定在 L1 裡。多出來的約 1.5 ns 就是 **TLB miss**。
- 所以本機最快的那層 data TLB 大約是 **64 個 entry**，涵蓋 64 × 4 KB = **256 KB**，和課本說的「64 entry TLB 涵蓋 256 KB」一樣。
- 128–512 個 page 都是 2.56 ns：miss 之後很快就從第二層 TLB 拿到了（不用真的走 page table）。⚠️ 這是我的推測；1024 頁以上資料也超過 L1，cache miss 和 page table walk 混在一起，這張表分不開，「待補」。

---

## 18. 一頁速查表

| 主題 | 重點 |
|---|---|
| Overlap problem | 很多 process 都以為整個記憶體是自己的，會互相蓋掉 |
| Size problem | 實體記憶體比程式要的小 |
| Physical memory | 真正有的（主記憶體） |
| Virtual memory | 程式以為有的；2ⁿ bytes，連續 |
| Memory map（Linux 32 位元） | header、text（0x08048000）、data、bss、heap↑、mmap 段、stack↓（0xC0000000） |
| 使用者／核心 | Linux 3／1 GB；Windows 2／2 GB |
| Translation | 虛擬位址 → MMU → 實體位址；出了處理器（含 L1）就是實體位址 |
| Page／frame | 4 KB；虛擬的叫 page、實體的叫 frame；offset 12 bit |
| 核心觀念 | 每個用到的 page 對到某個 frame；frame 散落各處；一個 frame 只給一個 page |
| 單層 page table | 2²⁰ entry × 2.5 B = 2.5 MB／process，太大 |
| 兩層 page table | 10 + 10 + 12；secondary 需要時才配；幾 KB 到幾十 KB |
| Inverted page table | 每個 frame 一個 entry 存 (page, pid)；hash table + linear probing；少用 |
| TLB | page table 的 cache；32–128 entry；hit rate ≈ 99%；64 entry 涵蓋 256 KB |
| Page fault | page 不在主記憶體；配 frame、建立對應；從 swap 讀要幾百萬週期 |
| Swap space | 硬碟上的區域；PTE 多一個 bit 說在主記憶體還是 swap |
| 存取流程 | TLB → page table → 找空 frame（不夠就趕到 swap）→ 建立對應 → 填 TLB |
| Shared memory | 兩個 page 對同一個 frame |
| Protection | read-only bit，text 不能改 |
| Linux | 32 位元：PGD 10 + PTE 10 + 12；64 位元：4 層 × 9 bit + 12 = 48 bit |

---

## 19. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| 多處理器、平行程式（shared memory 的硬體版） | [第 38 支](lecture38_note.md)：Multiprocessor Systems Part 1（課本 Ch12） |
| 多個 cache 裡同一份資料的一致性 | [第 39](lecture39_note.md)–[40 支](lecture40_note.md)：Coherence and Consistency |
| 硬碟、memory-mapped file、I/O | [第 43](lecture43_note.md)–[46 支](lecture46_note.md)：I/O System（課本 Ch13） |
| cache 的結構、LRU、modified bit（對照 TLB、PTE 的 accessed／dirty） | [第 35 支](lecture35_note.md) |
| AMAT（TLB miss 也算進去） | [第 36 支](lecture36_note.md) |
| x86 的 segment register | [第 11 支](lecture11_note.md) |
| process、kernel、page fault 的作業系統細節 | 作業系統課程 |

---

## 附：本講一句話總結

> **virtual memory 同時解決兩個問題：很多 process 都以為整個記憶體是自己的（overlap problem），以及實體記憶體比程式要的小（size problem）；做法是讓每個 process 看到一片連續的虛擬記憶體（Linux 32 位元的 memory map 是 header、text、data、bss、heap、mmap 段、stack，使用者 3 GB、kernel 1 GB），把它切成 4 KB 的 page、把實體記憶體切成同樣大的 frame，由每個 process 自己的 page table 把用到的 page 對到散落各處的 frame（一個 frame 只給一個 page）；單層 page table 每個 process 要 2.5 MB，兩層（10 + 10 + 12 bit，secondary 需要時才配）只要幾 KB 到幾十 KB，inverted page table 全系統一張但查起來要 hash，很少用；TLB 是 page table 的小 cache（約 64 entry，hit rate ≈ 99%），找不到對應就是 page fault，要配一個空 frame，不夠就把某個 frame 趕到硬碟上的 swap space；同一個機制還能做 shared memory（兩個 page 對同一個 frame）和 protection（text 唯讀）。實測本機 48 bit 虛擬、39 bit 實體位址，fork 後同一個虛擬位址在兩個 process 裡有不同的值，1 GiB 的 mmap 每頁第一次寫時各 page fault 一次，38 GiB 的機器能要到 256 GiB 的虛擬空間，寫 text 會 SIGSEGV，MAP_SHARED 能共享，而每個 page 只碰一條 line 時第 65 個 page 起延遲從 1.07 跳到 2.56 ns，看得到 64 entry 的 TLB；Linux 32 位元的 PGD／PTE 正是 10／10／12，64 位元是四層各 9 bit。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 10: The Memory System, Part 4/4 錄影逐字稿（YouTube 自動字幕，1:06:46）、Sarangi *Basic Computer Architecture* v3.09 Ch11 §11.4（p.539–556）、Linux v6.2.12 `arch/x86/include/asm/pgtable-2level_types.h`、`pgtable_64_types.h`、`pgtable_types.h`、`page_types.h` 整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4）結果。*
