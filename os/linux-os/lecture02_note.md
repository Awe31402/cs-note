# Operating Systems: A Linux Kernel-Oriented Approach 第二講完整筆記：特權暫存器與 Ring Level（Chapter 2, Part I）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 2（Chapter 2, Part I）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（59:19，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）第 2 章開頭與 §2.1.1–2.1.4（p.21–33）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
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
- [1. 多核心處理器與記憶體階層](#1-多核心處理器與記憶體階層)
- [2. 暫存器](#2-暫存器)
  - [2.1 一般暫存器：RISC vs CISC](#21-一般暫存器risc-vs-cisc)
  - [2.2 特權暫存器：為什麼需要「額外的」暫存器](#22-特權暫存器為什麼需要額外的暫存器)
  - [2.3 四種特權暫存器](#23-四種特權暫存器)
- [3. 特權等級：CPL 與 Ring](#3-特權等級cpl-與-ring)
  - [3.1 CPL 位元](#31-cpl-位元)
  - [3.2 Ring 0–3](#32-ring-03)
  - [3.3 【原始碼】kernel 怎麼知道現在是 user 還是 kernel](#33-原始碼kernel-怎麼知道現在是-user-還是-kernel)
- [4. 一般程式執行特權指令會怎樣？](#4-一般程式執行特權指令會怎樣)
- [5. 跟 OS 溝通的四種機制](#5-跟-os-溝通的四種機制)
  - [5.1 Interrupt：舉手發問](#51-interrupt舉手發問)
  - [5.2 Exception：程式做錯事](#52-exception程式做錯事)
  - [5.3 System call：假的中斷](#53-system-call假的中斷)
  - [5.4 【Q&A】interrupt 和 system call 差在哪？](#54-qainterrupt-和-system-call-差在哪)
  - [5.5 【Q&A】OS 做完事怎麼回去？](#55-qaos-做完事怎麼回去)
  - [5.6 Signal：反方向](#56-signal反方向)
  - [5.7 【Q&A】一定是先有硬體中斷才有 system call 嗎？](#57-qa一定是先有硬體中斷才有-system-call-嗎)
  - [5.8 四種機制總整理](#58-四種機制總整理)
- [6. System call 實際怎麼發出](#6-system-call-實際怎麼發出)
  - [6.1 `mov` + `syscall`](#61-mov--syscall)
  - [6.2 【原始碼】system call table](#62-原始碼system-call-table)
  - [6.3 函式指標與分派](#63-函式指標與分派)
  - [6.4 【原始碼】`sys_call_table` 與 `do_syscall_x64()`](#64-原始碼sys_call_table-與-do_syscall_x64)
  - [6.5 參數怎麼傳](#65-參數怎麼傳)
  - [6.6 舊方法：`int 0x80`](#66-舊方法int-0x80)
  - [6.7 放火燒房子：system call 很貴](#67-放火燒房子system-call-很貴)
- [7. Context：要存哪些東西](#7-context要存哪些東西)
  - [7.1 三步驟：存 → 處理 → 還原](#71-三步驟存--處理--還原)
  - [7.2 要存什麼、不用存什麼](#72-要存什麼不用存什麼)
  - [7.3 PC 還是 next PC？](#73-pc-還是-next-pc)
  - [7.4 【原始碼】`struct pt_regs` 與 `entry_SYSCALL_64`](#74-原始碼struct-pt_regs-與-entry_syscall_64)
- [8. Process 是一只行李箱](#8-process-是一只行李箱)
  - [8.1 Process 的生命週期](#81-process-的生命週期)
  - [8.2 可以換核心跑](#82-可以換核心跑)
  - [8.3 【Q&A】換核心不會有問題嗎？](#83-qa換核心不會有問題嗎)
  - [8.4 一個核心一次只跑一個 process](#84-一個核心一次只跑一個-process)
- [9. 一頁速查表](#9-一頁速查表)
- [10. 與後續講次的連結](#10-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

`[00:02]` 第 2 章是「從 OS 角度看計算機結構」。不是一般計組課的全面介紹，**只看會影響 OS 的部分**：特權指令、特權暫存器等。

第 2 章分四塊：

| # | 主題 | 本講有沒有講 |
|---|---|---|
| 1 | 核心、暫存器、中斷 | ✅ 本講 |
| 2 | x86 組合語言 | 散在各處 |
| 3 | 虛擬記憶體（「永遠很難，但對 OS 是核心觀念」） | 第 5 講 |
| 4 | I/O | 第 6 講 |

`[01:31]` 【口頭】現代晶片很複雜，但**給軟體的介面這些年沒什麼變**。複雜度被硬體漂亮地藏起來了，給 OS 的介面大致一樣，只多了一點點。

---

## 1. 多核心處理器與記憶體階層

`[02:02]`（課本 §2.1.1、Fig 2.2）

```
   ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐
   │ Core │ │ Core │ │ Core │ │ Core │   ← 4–64 個核心，每個都是完整 pipeline
   │L1 i/d│ │L1 i/d│ │L1 i/d│ │L1 i/d│
   └──┬───┘ └──┬───┘ └──┬───┘ └──┬───┘
      └────── L2 / L3（/ L4）快取 ──────┘
                     │
               Main memory（DRAM，晶片外）
```

- 4 到 64 核很常見，八核手機很普通。
- 快取有階層：L1、L2、L3，有時還有晶片外的 L4。
- 快取是**包含式**的：L1 有的位址，L2 也有（L1 的資料可能比較新）。主記憶體是整個位址空間的「超集合」。

`[03:00]` **實體位址空間（physical address space）**：所有能用實體位址存取的位置。這是**硬體**看到的觀點，不是程式或編譯器看到的。對硬體來說只有**一個**位址空間。之後講 swap space 時，會再把實體位址空間往外擴。

`[03:31]` 位址就是一個 32 或 64 位元的數字。64 位元機器**不一定用滿 64 位元**，例如只用 48 位元、上面 16 位元全 0。48 位元 = **256 TB**，目前沒有單一節點有這麼多記憶體。

【課本補充】課本強調一個重點（p.22）：**只有主記憶體（DRAM）是軟體看得到的**。L1/L2/L3 快取對 OS 通常是**隱形**的。唯一例外是有些指令集有「清除某層快取」的指令。所以可以假設：OS 能看到的最高層記憶體就是主記憶體。

---

## 2. 暫存器

### 2.1 一般暫存器：RISC vs CISC

`[04:01]` 暫存器 = **有名字的儲存位置**，數量 8 到 32 個：

| 指令集 | 一般暫存器數量 |
|---|---|
| x86（32 位元） | 8 |
| x86-64 | 16 |
| RISC-V | 32 |

`[04:30]` 為什麼 RISC 的暫存器比較多？

| | RISC（如 ARM、RISC-V） | CISC（如 x86） |
|---|---|---|
| 記憶體運算元 | 很少，要先 `load` 進暫存器才能算 | 指令可以直接對記憶體運算 |
| 暫存器需求 | **多** | 少 |

【課本補充】Definition 2.1.1：RISC 的好處是解碼器和編譯器簡單；CISC 的好處是指令編碼比較省位元組。

`[05:01]` 一般暫存器是**所有程式都能用**的，寫一般程式用的就是這些。計組課都教過。這一講要看的是**一般暫存器以外**的東西。

### 2.2 特權暫存器：為什麼需要「額外的」暫存器

`[05:30]` **特權暫存器（privileged registers）**：一般程式看不到，或看得到但**不准用**。只有在特定處理器模式下能用，能用的是：

- **OS**
- **Hypervisor**（虛擬機監控程式，也叫 VMM）：負責監控 guest OS，某種意義上也是一種 OS

`[06:31]` 為什麼需要？教授用計組課寫中斷處理程式時遇過的問題來說明：

> 我有 16 個暫存器，想**拍一張快照**，把 16 個的值都存下來。
> 但要存，就要跑一段程式；那段程式**自己也要用暫存器**，一用就把原本的值蓋掉了。

【口頭】教授的比喻：

> 「就像**站在地板上想拍整片地板**——你的腳一定會踩住一塊磁磚。你得**把自己吊在天花板上**才拍得到。」

所以拍快照的程式需要**額外的暫存器**可用。用這些額外暫存器，才能把所有一般暫存器存起來。**這些額外的暫存器就是特權暫存器**，只有 OS 程式碼能用。

### 2.3 四種特權暫存器

`[08:00]`（課本 §2.1.3）

| 種類 | 用途 | 教授的例子 |
|---|---|---|
| **Flags 暫存器** | 存上一次比較的結果，讓條件分支指令用 | 一般程式只能透過條件分支**間接**用到它。要把它整個存起來，得靠特權機制（中斷處理時需要） |
| **控制暫存器**（control registers） | 開關特定硬體功能 | 網路攝影機旁邊那顆小燈、開關 LED、關風扇、關螢幕，甚至關機 |
| **除錯暫存器**（debug registers） | 設中斷點：程式跑到某條指令就停下來 | — |
| **I/O 暫存器** | 寫進去，就會反映到裝置上 | 寫到接印表機的暫存器，印表機就開始印 |

#### 【Q&A】flags 一般程式不是也能用嗎？

`[10:01]`（⚠️ 逐字稿沒錄到學生的問題，從教授回答推斷）

**教授答**：條件分支指令會**隱含地**用到 flags。但如果你想把**所有 flags 存進一個暫存器**，大部分指令集**沒有一般指令**做得到，只有**特權指令**。程式的狀態平常不完全對你公開，但會**完全對 OS 公開**，讓 OS 能有效地搬移和儲存。

`[11:01]` 使用這些暫存器是 **OS 的專屬權利**。它們都列在處理器手冊的 **Part 2**——硬體廠商一定要發規格給寫 OS 的人，所以一定有 Part 2。

【課本補充】課本補了為什麼除錯暫存器要特權：它們能拿到額外資訊、能從執行中的程式挖出資料，給一般程式用會有嚴重安全問題。I/O 暫存器要特權，除了安全，還因為**不同程式可能想獨佔**某個 I/O 資源，OS 要當仲介。

---

## 3. 特權等級：CPL 與 Ring

`[11:32]` 問題：硬體怎麼知道它現在跑的是 **OS 程式碼**，還是一般程式？

### 3.1 CPL 位元

`[12:00]` 早期處理器有一個 **CPL（Current Privilege Level，目前特權等級）位元**：

| CPL | 模式 | 誰在跑 | 能做什麼 |
|---|---|---|---|
| **0** | 特權模式（privileged） | OS | 可以發特權指令 |
| **1** | 應用程式模式 | 一般程式 | 權力被削減 |

### 3.2 Ring 0–3

`[12:30]` 現在更一般化：**ring（環）**，四個特權等級，不只兩個（課本 Fig 2.3）：

```
        ┌───────────────────────────┐
        │ Ring 3：應用程式            │
        │   ┌───────────────────┐   │
        │   │ Ring 1、2：虛擬機   │   │
        │   │   ┌───────────┐   │   │
        │   │   │  Ring 0   │   │   │
        │   │   │ OS / 最高 │   │   │
        │   │   └───────────┘   │   │
        │   └───────────────────┘   │
        └───────────────────────────┘
```

| Ring | 誰 | 說明 |
|---|---|---|
| **0** | OS，或 type 0 hypervisor | 特權最高。開機時先開起來的就是它。type 0 hypervisor = 一個很薄的 OS，上面跑其他 guest OS（⚠️ 教授說「type zero」，第 8 章再確認名稱） |
| **1、2** | 虛擬機 | 比 ring 0 權限少，比應用程式多 |
| **3** | 應用程式 | 可以跑在一般 OS 上，也可以跑在虛擬環境的 guest OS 上 |

`[13:32]` 有了 ring，硬體就能**自動**知道現在是不是特權模式。

`[14:02]` 每次 **context switch**（從一般程式切到 OS），特權等級就**升高**；反過來就降低。

### 3.3 【原始碼】kernel 怎麼知道現在是 user 還是 kernel

在 x86 上，目前的特權等級藏在 **CS（code segment）暫存器的最低 2 個位元**。kernel 進入時會把 CS 存起來（見 [§7.4](#74-原始碼struct-pt_regs-與-entry_syscall_64)），之後用這個函式判斷「被打斷的是 user 還是 kernel」：

```c
// arch/x86/include/asm/ptrace.h:131-138
static __always_inline int user_mode(struct pt_regs *regs)
{
#ifdef CONFIG_X86_32
	return ((regs->cs & SEGMENT_RPL_MASK) | (regs->flags & X86_VM_MASK)) >= USER_RPL;
#else
	return !!(regs->cs & 3);
#endif
}
```

| 行 | 意思 |
|---|---|
| 131 | 傳入「被打斷時存下來的暫存器」`regs` |
| 133–134 | 32 位元版：取 CS 低 2 位元（`SEGMENT_RPL_MASK` = `0x3`），看是否 ≥ `USER_RPL`（= 3） |
| 136 | 64 位元版：`regs->cs & 3` 取低 2 位元。**0 = ring 0（kernel）**，非 0 = user。`!!` 把結果變成 0 或 1 |

常數定義：

```c
// arch/x86/include/asm/segment.h:33, 48
#define SEGMENT_RPL_MASK	0x3
#define USER_RPL		0x3
```

`0x3` 就是兩個位元能表示的 0–3，剛好對應 4 個 ring。`USER_RPL = 3` 就是 **ring 3**。

> 觀察（不是教授講的）：Linux 實際只用 **ring 0（kernel）** 和 **ring 3（user）**，從這兩個常數就看得出來——程式碼只判斷「是不是 0」。ring 1、2 在 Linux 裡沒有用到。

---

## 4. 一般程式執行特權指令會怎樣？

`[14:31]` 一般指令（load/store、算術、邏輯、分支）大家都能跑，沒問題。

問題是：一般程式**試圖**執行特權指令（例如關機），你沒辦法阻止它「發出」這條指令，但硬體要怎麼**回應**？三種可能：

| 選項 | 行為 | 對虛擬機來說 |
|---|---|---|
| ① 產生 **exception** | 終止程式：「你違反了存取權限」 | ✅ 沒問題 |
| ② **有某種效果**，但不是 exception | 例如回傳錯誤碼，或在某處記一筆「有人想關機」 | ❌ **大問題** |
| ③ **完全沒效果** | 硬體當成 **nop**（什麼都不做），好像這條指令沒發過 | ✅ 沒問題 |

`[16:01]` 選項 ② 的指令叫 **polymorphic instruction（多型指令）**：在特權模式和非特權模式下**效果不同**，而且**不會產生 exception**。

> ⚠️ 不一致：教授說 ① 和 ③ 都 OK、只有 ② 有問題；課本 p.25 說「保持沉默（像 nop）**或**在不同模式下給出不同結果」**兩者**都很麻煩。第 8 章（虛擬化）再確認。

【口頭】為什麼會有這種指令？`[16:31]`

- 找出並處理這種指令，是**寫虛擬機的人面對的最大難題之一**。
- 指令集主要是 **70 年代末、80 年代**設計的。那時沒人想到有一天 **OS 本身會像應用程式一樣被執行**（VMware、VirtualBox、KVM）。
- 教授說他念博士時虛擬機都還不普遍，大約 **2008–2010 年雲端運算興起後**才普及。
- 「他們不能被責怪——我們今天做的事，也沒辦法預見明天會怎樣。」

`[17:31]` 特權指令（手冊 Part 2）可以存取**所有**暫存器，通常不會產生 exception（除非你做了不允許的事）。OS 和類 OS 的軟體（虛擬機管理程式，本質上是很薄的 OS）可以存取特權暫存器、發特權指令，做開關風扇、關螢幕這類事。

---

## 5. 跟 OS 溝通的四種機制

### 5.1 Interrupt：舉手發問

`[18:31]` **Interrupt（中斷）** 是**外部**產生的事件：移動滑鼠、按鍵盤、網路卡收到封包。

【口頭】教授用上課來比喻：

| 上課 | 電腦 |
|---|---|
| 你舉手 | 裝置發出 interrupt |
| 我注意到你舉手 | CPU 確認收到中斷 |
| 我暫停，但**記住講到哪** | **儲存 context** |
| 請你發問、我回答 | **處理（service）中斷** |
| 從同一張投影片、同一個點繼續講 | **還原 context** |

`[19:32]` OS 做的事一模一樣：

1. 存下執行中程式的 context（執行時期狀態）
2. 處理中斷
3. OS 可能做很多別的事，包括跑別的程式
4. 找到適當時機，從**完全相同的位置**恢復原程式——而**原程式不知道**這件事發生過

這種暫停與恢復可能在一個程式的生命中發生**幾百萬次**，**完全不能影響它的正確性**。

### 5.2 Exception：程式做錯事

`[20:01]` **Exception（例外）** 處理流程跟 interrupt 大致一樣，但來源不同：

- interrupt：**硬體裝置**要引起 OS 注意
- exception：**你的程式做錯事**，例如除以 0、存取非法位址（經典的 **segmentation fault**，位址常常是 0）

`[21:00]` OS 常常會終止程式，但**不一定**：

【口頭】例子：你發了 **開根號指令**，但你的處理器沒有這個指令。OS 可以**模擬**它——用加減乘除算出近似平方根（「你們國一、國二學過的那個一次取兩位數的開方法」），放進暫存器再返回。程式沒被終止。

`[22:01]` 之後會看到 **page fault**（分頁錯誤），也是 exception，也不會終止程式。所以**不是所有 exception 都是壞事**。

### 5.3 System call：假的中斷

`[22:01]` 程式要做硬體相關的事（送網路封包、從硬碟讀東西）需要 OS 介入。但：

- **不能用函式呼叫**：程式和 OS **不在同一個位址空間**。
- OS 這時**根本沒在跑**：一個核心一次只能跑一個程式，你在跑，OS 就沒在跑。

`[22:31]` 所以你**產生一個假的 exception（dummy / fake exception）**，引起 OS 注意。接下來一樣：暫停程式 → 存狀態 → 透過**暫存器或記憶體**把資料傳給 OS → OS 處理（例如讀檔、把資料給你）→ 程式從同一個點恢復，在等回傳值的地方拿到回傳值。

### 5.4 【Q&A】interrupt 和 system call 差在哪？

`[24:00]`（⚠️ 學生問題沒錄到，從回答推斷）

**教授答**：

- **Interrupt**：**硬體**產生，用來搶 CPU 的注意。例如你按鍵盤，硬體就是鍵盤，中斷要被處理。
- **System call**：**我（程式）** 需要 OS 幫忙。例如我想讀檔，但我沒有足夠權限；我想透過網路傳訊息給朋友，但我沒權限碰網路卡。

要引起 OS 注意，**唯一的方法就是中斷機制**，所以我產生一個假中斷。「沒有其他方法能讓 OS 注意到任何事。」

### 5.5 【Q&A】OS 做完事怎麼回去？

`[25:01]`

**學生問**：OS 做完工作之後要回去吧？

**教授答**：對。OS 把 system call 的結果放到該放的地方，然後要**退場**，載入原本的 process：

- **還原狀態就等於載入它**：把 program counter 設成那個 process 的下一個 PC。
- 「設定 PC 某個值，就等於**暫停自己、跑另一個 process**。」
- 當然還要切換模式：OS 在特權模式，要回到非特權模式。
- 所以大部分指令集都有 **`iret`（interrupt return）** 或 **`sysret`（system call return）** 指令：**覆寫 PC + 切換模式**一次完成。

### 5.6 Signal：反方向

`[26:31]` Signal 第 4 章才細講，這裡先有概念。

【口頭】例子：你在用小畫家，移動滑鼠畫線。

1. 滑鼠產生 **interrupt** → 送到 OS。
2. OS 知道「滑鼠從 (x, y) 移到 (x′, y′)」。
3. 但**這個資訊在 OS 那裡，不在 process 那裡**。
4. 需要一條**從 OS 回到 process** 的路，告訴它「滑鼠移動了，請做你想做的事」——這就是 **signal**。

Signal 是**非同步**的（asynchronous）：你不是在等它，因為你不知道使用者什麼時候會動滑鼠。

【課本補充】課本對 signal 的定義更精確（p.26）：signal 本質上是一個**回呼函式（callback function）**，程式事先**向 OS 註冊**。事件發生時，OS 在程式的 context 裡呼叫這個函式，這個函式叫 **signal handler**。

### 5.7 【Q&A】一定是先有硬體中斷才有 system call 嗎？

`[28:01]`

**學生問**：可以概括說「硬體中斷一定在 system call 之前」嗎？（⚠️ 逐字稿問題不完整）

**教授答**：system call 本身就會產生中斷。而且**一般程式其實不會直接發 system call**：

```
 使用者程式
    │ 呼叫
    ▼
 Library call（例如 C library 的函式）
    │ 把參數放到 system call 要的位置
    ▼
 System call（= 產生中斷）
    │
    ▼
 OS：先存狀態 → 處理 → 放好回傳值 → 恢復原程式
```

library 就是一堆函式，它**包裝**了 system call。

### 5.8 四種機制總整理

`[29:31]` interrupt、exception、system call 都是**朝向 OS** 的打斷機制；**signal 是唯一離開 OS 的**，從 OS 到 process。

`[30:02]` 教授在黑板上畫的圖：

```
  硬體裝置 ──── interrupt ────►┐
                               │
  程式做錯事 ── exception ────►├──►  OS
  (CPU 產生)                    │
                               │
  程式 ─── system call ───────►┘
  (程式叫 CPU 幫它產生假中斷)

  OS ──────── signal ─────────►  Process
```

| 機制 | 誰發起 | 方向 | 例子 |
|---|---|---|---|
| Interrupt | 硬體裝置 | → OS | 按鍵、滑鼠、網路封包 |
| Exception | 程式出錯，由 CPU 產生 | → OS | 除以 0、segfault、page fault |
| System call | 程式用 `syscall` 指令請 CPU 產生假中斷 | → OS | 讀檔、送封包 |
| Signal | OS | → Process | 通知滑鼠移動、按鍵 |

【口頭】教授說他為了方便一直講「program」，但**技術上應該說 process**。

---

## 6. System call 實際怎麼發出

### 6.1 `mov` + `syscall`

`[32:01]` 【口頭】教授更正投影片：最大數量「現在是 **548**」，要看 `syscall_64.tbl`，舊版投影片的 `unistd` 連結不對。

`[32:30]` 在 x86-64 上發 system call 非常簡單：

```asm
mov  $17, %rax     # 把 system call 號碼（例如 17）放進 rax
syscall            # 發出 system call
```

- `$` 表示**立即值**（常數）。
- 【口頭】這是 **AT&T 語法**：**目的地在最後面**。跟 RISC-V 等「目的地在前面」的語法相反。

`[33:32]` `syscall` 指令做的事：

1. 產生中斷，進入 OS。
2. OS 發現是 system call，**先存狀態**。
3. 從存下來的狀態裡讀 **`rax`**，拿到號碼（17）。
4. 到 **system call table** 查第 17 格。
5. 第 17 格是一個**函式指標**，指向一個函式。
6. 執行那個函式，處理 system call。

### 6.2 【原始碼】system call table

教授說的表在這裡：

```text
# arch/x86/entry/syscalls/syscall_64.tbl:1-14
#
# 64-bit system call numbers and entry vectors
#
# The format is:
# <number> <abi> <name> <entry point>
#
# The __x64_sys_*() stubs are created on-the-fly for sys_*() system calls
#
# The abi is "common", "64" or "x32" for this file.
#
0	common	read			sys_read
1	common	write			sys_write
2	common	open			sys_open
3	common	close			sys_close
```

```text
# arch/x86/entry/syscalls/syscall_64.tbl:28
17	common	pread64			sys_pread64
```

每一行四欄：

| 欄 | 意思 | 例子 |
|---|---|---|
| number | system call 號碼，就是要放進 `rax` 的值 | `1` |
| abi | 給哪種程式用 | `common` |
| name | 名字 | `write` |
| entry point | kernel 裡處理它的函式 | `sys_write` |

所以教授舉的「17 號」就是 **`pread64`**（從檔案的指定位置讀資料）。

**數一數**（我用 `awk` 統計 v6.2.12 這個檔案）：

| abi | 數量 | 意思 |
|---|---:|---|
| `common` | 315 | 64 位元和 x32 都能用 |
| `64` | 47 | 只給 64 位元程式 |
| `x32` | 36 | 給 x32 ABI（64 位元指令、但指標是 32 位元的特殊模式） |
| **合計** | **398** | |

- 64 位元程式能用的：315 + 47 = **362**，跟課本說的「約 362 個」完全一樣。
- 課本說「36 calls are defined for **32-bit** architectures」，對照檔案其實是 **x32** 那 36 個（真正的 32 位元 system call 在另一個檔 `syscall_32.tbl`）。
- 最大號碼是 **547**（`pwritev2` 的 x32 版），所以號碼範圍 0–547 = **548 格**。這就是教授說的「548」和課本說的「最多可以定義 548 個」——是**格子數**，不是實際定義的數量。

### 6.3 函式指標與分派

`[35:00]` **函式指標（function pointer）** 是什麼？

- 指標 = **記憶體位址**（32 位元系統是 32 位元，64 位元系統是 64 位元）。
- 函式指標指向**函式程式碼的第一個位元組**——也就是開始執行函式的那個點。

`[36:01]` 【口頭】**教授出的作業**：用 C 寫一個程式，定義函式指標並使用它。「因為 Linux 裡會大量用到。」

函式指標強大的地方：它**把資料和程式碼連起來**——可以**根據資料決定呼叫哪個函式**。例如資料是 17，就呼叫第 17 號 system call 對應的函式。

下面是作業的示範（我寫的，已在本機用 `gcc -Wall` 編譯執行過），跟 kernel 的做法是同一個概念：

```c
#include <stdio.h>

typedef long (*handler_t)(long a, long b);   /* 函式指標型別 */

long do_add(long a, long b) { return a + b; }
long do_sub(long a, long b) { return a - b; }
long do_mul(long a, long b) { return a * b; }

/* 一張「號碼 → 函式」的表，跟 sys_call_table 同一個概念 */
static const handler_t table[] = { do_add, do_sub, do_mul };

long dispatch(unsigned int nr, long a, long b)
{
	if (nr >= sizeof(table) / sizeof(table[0]))
		return -1;                    /* 號碼超出範圍 */
	return table[nr](a, b);           /* 用資料（nr）決定呼叫哪個函式 */
}

int main(void)
{
	for (unsigned int nr = 0; nr < 4; nr++)
		printf("nr=%u -> %ld\n", nr, dispatch(nr, 7, 3));
	return 0;
}
```

輸出：

```text
nr=0 -> 10
nr=1 -> 4
nr=2 -> 21
nr=3 -> -1
```

`[37:30]` 【口頭】所有 system call 處理函式都有**標準的呼叫格式、同一組參數**，不然會很麻煩。（細節第 4 章講。）

### 6.4 【原始碼】`sys_call_table` 與 `do_syscall_x64()`

kernel 的「函式指標型別」：

```c
// arch/x86/include/asm/syscall.h:19-20
typedef long (*sys_call_ptr_t)(const struct pt_regs *);
extern const sys_call_ptr_t sys_call_table[];
```

| 部分 | 意思 |
|---|---|
| `typedef long (*sys_call_ptr_t)(...)` | 定義一個函式指標型別：參數是 `const struct pt_regs *`，回傳 `long` |
| `const struct pt_regs *` | **所有** system call 都只收這一個參數——指向「存下來的暫存器」。這就是教授說的「標準的一組參數」 |
| `sys_call_table[]` | 一個陣列，每一格是一個函式指標 |

表本身：

```c
// arch/x86/entry/syscall_64.c:10-18
#define __SYSCALL(nr, sym) extern long __x64_##sym(const struct pt_regs *);
#include <asm/syscalls_64.h>
#undef __SYSCALL

#define __SYSCALL(nr, sym) __x64_##sym,

asmlinkage const sys_call_ptr_t sys_call_table[] = {
#include <asm/syscalls_64.h>
};
```

這是個 C 巨集小技巧：

| 行 | 意思 |
|---|---|
| 10–12 | 先把 `__SYSCALL` 定義成「**宣告**函式」，引入 `syscalls_64.h`（這個檔是編譯時從 `syscall_64.tbl` **自動產生**的，每行一個 `__SYSCALL(號碼, 名字)`）。結果：宣告了所有 `__x64_sys_read`、`__x64_sys_write`…… |
| 14 | 再把 `__SYSCALL` 重新定義成「**函式名 + 逗號**」 |
| 16–18 | 同一個檔再引入一次，這次展開成 `{ __x64_sys_read, __x64_sys_write, ... }`，就變成陣列內容了 |

`##` 是**黏貼**：`__x64_##sym` 碰到 `sys_read` 就變成 `__x64_sys_read`。

真正「查表、呼叫」的地方：

```c
// arch/x86/entry/common.c:40-54
static __always_inline bool do_syscall_x64(struct pt_regs *regs, int nr)
{
	/*
	 * Convert negative numbers to very high and thus out of range
	 * numbers for comparisons.
	 */
	unsigned int unr = nr;

	if (likely(unr < NR_syscalls)) {
		unr = array_index_nospec(unr, NR_syscalls);
		regs->ax = sys_call_table[unr](regs);
		return true;
	}
	return false;
}
```

| 行 | 意思 |
|---|---|
| 40 | `nr` 是 system call 號碼（從 `rax` 來的） |
| 46 | 轉成 `unsigned`：負數會變成超大的數，下一行一比就出界。一次比較就同時擋掉「負數」和「太大」 |
| 48 | 號碼在範圍內嗎？`likely` 告訴編譯器「通常是」，讓它最佳化 |
| 49 | `array_index_nospec`：防範 **Spectre** 這類旁通道攻擊（CPU 猜測執行時偷偷越界讀表） |
| **50** | **整個機制的核心**：`sys_call_table[unr]` 拿出函式指標，`(regs)` 呼叫它，回傳值寫回 `regs->ax`——也就是使用者程式恢復後看到的 **`rax`**。跟上面示範程式的 `table[nr](a, b)` 一模一樣 |

呼叫它的是：

```c
// arch/x86/entry/common.c:73-87
__visible noinstr void do_syscall_64(struct pt_regs *regs, int nr)
{
	add_random_kstack_offset();
	nr = syscall_enter_from_user_mode(regs, nr);

	instrumentation_begin();

	if (!do_syscall_x64(regs, nr) && !do_syscall_x32(regs, nr) && nr != -1) {
		/* Invalid system call, but still a system call. */
		regs->ax = __x64_sys_ni_syscall(regs);
	}

	instrumentation_end();
	syscall_exit_to_user_mode(regs);
}
```

| 行 | 意思 |
|---|---|
| 75 | 讓 kernel stack 的位置每次隨機偏移一點，增加攻擊難度 |
| 76 | 進入 kernel 時的共同處理（例如追蹤、稽核） |
| 80 | 先試 64 位元表，不行再試 x32 表 |
| 81–82 | 號碼無效 → 呼叫 `ni_syscall`（ni = **n**ot **i**mplemented），回傳錯誤碼 `-ENOSYS` |
| 86 | 回 user mode 前的共同處理（例如檢查有沒有 signal 要送） |

### 6.5 參數怎麼傳

`[38:00]` 其他暫存器放參數，通常有 **5 到 6 個**暫存器可用。資料更多的話，要傳一個**指標**，指向 process 記憶體裡的資料，kernel 再從那裡讀。

【原始碼】`entry_SYSCALL_64` 開頭的註解把暫存器規則寫得很清楚：

```asm
// arch/x86/entry/entry_64.S:68-78
 * Registers on entry:
 * rax  system call number
 * rcx  return address
 * r11  saved rflags (note: r11 is callee-clobbered register in C ABI)
 * rdi  arg0
 * rsi  arg1
 * rdx  arg2
 * r10  arg3 (needs to be moved to rcx to conform to C ABI)
 * r8   arg4
 * r9   arg5
 * (note: r12-r15, rbp, rbx are callee-preserved in C ABI)
```

| 暫存器 | 用途 |
|---|---|
| `rax` | system call 號碼（回來時放回傳值） |
| `rdi, rsi, rdx, r10, r8, r9` | 第 0–5 個參數，共 **6 個** |
| `rcx` | 被 `syscall` 指令拿去存**返回位址**（所以第 3 個參數改用 `r10`，不用一般函式呼叫的 `rcx`） |
| `r11` | 被 `syscall` 指令拿去存 **rflags** |

示範：不透過 C library，直接用 `syscall` 指令呼叫 `write`（我寫的，已在本機編譯執行，會印出 `hello from syscall`）：

```c
#include <unistd.h>
#include <sys/syscall.h>

int main(void)
{
	const char msg[] = "hello from syscall\n";
	long ret;
	/* write(1, msg, len)：rax=1 (write), rdi=1 (stdout), rsi=msg, rdx=len */
	asm volatile ("syscall"
		      : "=a"(ret)
		      : "a"(SYS_write), "D"(1), "S"(msg), "d"(sizeof(msg) - 1)
		      : "rcx", "r11", "memory");
	return ret < 0;
}
```

- `"a"` = `rax`、`"D"` = `rdi`、`"S"` = `rsi`、`"d"` = `rdx`。
- 最後一行告訴編譯器 `rcx`、`r11` 會被 `syscall` 蓋掉——正好對應上面表格。

【課本補充】課本 p.27 特別提醒：**應用程式最好不要直接發 system call**，因為你不一定清楚它的完整語意，而且 OS 會隨版本改 system call 的格式。應該用 **glibc** 這類 library 包好的函式，library 的介面幾乎不變，可以跨版本、跨 Linux 發行版使用。

### 6.6 舊方法：`int 0x80`

`[38:32]` 舊方法（現在不用了，但還能動）：直接用 **`int` 指令**產生中斷，不假裝是 system call。

```asm
int $0x80      # 產生 128 號中斷
```

- 每個中斷都有唯一號碼，就像每個 system call 有唯一號碼。
- `0x80` = 十進位 **128**。
- 在 `syscall` 指令出現之前，就是這樣做的。

【原始碼】

```c
// arch/x86/include/asm/irq_vectors.h:45
#define IA32_SYSCALL_VECTOR		0x80
```

名字裡的 **IA32** 說明了：`int 0x80` 是 **32 位元 x86** 的 system call 方式，到 v6.2.12 還保留（給 32 位元程式相容用）。

### 6.7 放火燒房子：system call 很貴

`[40:04]` 所有機制處理方式都一樣：**敲 OS 的門**。

- 每個 fault / exception 有自己的 exception 號碼。
- 處理流程相同：**存狀態 → 看號碼和類型 → 查表**。類型很重要：system call、interrupt、exception 是不同類型。
- 每個 system call 有 system call handler，每個 interrupt 有 interrupt handler，每個 exception 有 exception handler。
- **全部都是透過「函式指標表」**：號碼當索引，查出函式指標，呼叫它。所以函式指標很重要。

`[41:31]` 【口頭】

> 「這是一種**非傳統、很奇怪**的引起 OS 注意的方式。沒有直接的方法——不是寫個東西到某處，也不是呼叫函式。它真的是一個**緊急機制**。標準比喻是：想叫消防隊來，就**放火燒你的房子**。」

**這個故事的寓意** `[42:00]`：

- 進 OS 要走**昂貴的中斷處理路徑**，是重量級機制。
- 所以寫任何程式都要**盡量減少進 OS 的次數**。
- 做法：**批次處理（batching）**。C library 也這樣做——不要每次寫 4 bytes 到檔案，而是先寫進一個 **4 KB** 的記憶體緩衝區，滿了再一次寫出去。
- 為什麼 4 KB？因為那是**一個 page 的大小**。
- 這樣存狀態、還原狀態這些「固定成本」就被攤平了。

---

## 7. Context：要存哪些東西

### 7.1 三步驟：存 → 處理 → 還原

`[43:31]` CPU 做的事：

1. **存下**執行中 process 的狀態，這個狀態叫 **context**。
2. 載入 interrupt handler（或 system call handler、exception handler）。三者常常**統稱 interrupt**，因為它們都是打斷。要用哪個 handler，從 context 判斷，透過**查函式指標表**決定。
3. **恢復**。

### 7.2 要存什麼、不用存什麼

`[44:31]`（課本 Fig 2.4）

```
 ┌──────────┬──────────────┬────────┬──────┐
 │ 一般      │ Flags 與      │ Memory │  PC  │
 │ 暫存器    │ 特殊暫存器    │        │      │
 └──────────┴──────────────┴────────┴──────┘
 └──── register state ─────┘
 └────────────── program state ─────────────┘
```

| 項目 | 要存嗎？ | 為什麼 |
|---|---|---|
| **一般暫存器** | ✅ | 暫存器只有一組，**所有 process 共用**，換人就會被蓋掉 |
| **Flags、特殊暫存器** | ✅ | 同上 |
| **記憶體** | ❌ **不用** | **虛擬記憶體機制**已經處理好了，別的 process 也動不了它（第 5、6 講） |
| **PC** | ✅ | 要知道從哪裡恢復。但有點微妙（見下節） |

【課本補充】課本說：暫存器、flags、PC 是**揮發性狀態（volatile state）**，context switch 時會被清掉，所以需要硬體機制把它們讀出來、存到**事先知道的記憶體位置**。硬體可能只存一部分（例如 PC、flags、stack pointer），interrupt handler 要負責把**剩下的**也存起來。

### 7.3 PC 還是 next PC？

`[45:32]` 恢復時要從哪條指令開始？**看中斷類型**：

| 情況 | 存哪個 PC | 為什麼 |
|---|---|---|
| **Page fault** | **出錯那條指令的 PC** | 位址是合法的，只是資料現在不在記憶體裡，要先搬過來。搬好後**要重新執行同一條指令** |
| **硬體 interrupt、system call** | **next PC** | 目前這條指令已經正確執行完了，從下一條繼續，**不能重跑** |

next PC 也有兩種可能 `[46:30]`：

- **PC + 4**（如果每條指令 4 bytes，而且是循序執行）
- **分支目標（branch target）**（如果目前這條是分支指令、要跳去別處）

不管哪種，存下來的就是「**恢復點**」。還原 context 時，處理器跳到恢復點開始執行，這代表 **OS 被暫停了、原 process 在跑了**。

`[49:30]` 原 process **永遠不應該知道**這件事發生過。（它當然可以做很精細的時間測量來察覺，但一般來說應該完全無縫。）

`[50:02]` 任兩個 process，或 process 和 OS，**位址空間永遠不同**，不然會有嚴重安全問題。（「位址空間」這個詞第 6 章再正式定義。）

【口頭】`[48:00]` 教授順便定義了 **kernel**：OS 的**核心部分**，負責管理記憶體、process、排程等所有主要事項，也提供 driver 的介面。但「**driver 不算 kernel 的一部分**，driver 一律分開看。」

### 7.4 【原始碼】`struct pt_regs` 與 `entry_SYSCALL_64`

kernel 把 context 存成一個結構，叫 **`pt_regs`**（pt = ptrace，因為除錯工具 ptrace 也用它）：

```c
// arch/x86/include/asm/ptrace.h:59-92
struct pt_regs {
/*
 * C ABI says these regs are callee-preserved. They aren't saved on kernel entry
 * unless syscall needs a complete, fully filled "struct pt_regs".
 */
	unsigned long r15;
	unsigned long r14;
	unsigned long r13;
	unsigned long r12;
	unsigned long bp;
	unsigned long bx;
/* These regs are callee-clobbered. Always saved on kernel entry. */
	unsigned long r11;
	unsigned long r10;
	unsigned long r9;
	unsigned long r8;
	unsigned long ax;
	unsigned long cx;
	unsigned long dx;
	unsigned long si;
	unsigned long di;
/*
 * On syscall entry, this is syscall#. On CPU exception, this is error code.
 * On hw interrupt, it's IRQ number:
 */
	unsigned long orig_ax;
/* Return frame for iretq */
	unsigned long ip;
	unsigned long cs;
	unsigned long flags;
	unsigned long sp;
	unsigned long ss;
/* top of stack page */
};
```

對照課本 Fig 2.4：

| 欄位 | Fig 2.4 的哪一塊 | 說明 |
|---|---|---|
| `r15`…`di`（15 個） | 一般暫存器 | x86-64 的 16 個一般暫存器（`sp` 另外放） |
| `orig_ax` | — | **三種打斷共用一格**：system call 時是號碼、exception 時是錯誤碼、硬體中斷時是 IRQ 號碼。印證教授說「三者處理流程一樣」 |
| `ip` | **PC** | x86 把 PC 叫 instruction pointer |
| `cs` | 特殊暫存器 | 低 2 位元就是**特權等級**（[§3.3](#33-原始碼kernel-怎麼知道現在是-user-還是-kernel) 的 `user_mode()` 讀的就是它） |
| `flags` | **Flags** | |
| `sp`、`ss` | 特殊暫存器 | stack pointer 與 stack segment |
| （沒有記憶體） | Memory | **不存**，跟教授說的一樣 |

注意第 60–62 行的註解：`r12`–`r15`、`bp`、`bx` 是「callee-preserved」——C 函式自己會保存它們，所以進 kernel 時**不一定**要存。這是實際實作的最佳化。

是誰把暫存器存進 `pt_regs`？x86-64 的 `syscall` 指令一進 kernel，就跳到這段組合語言：

```asm
// arch/x86/entry/entry_64.S:87-120（節錄）
SYM_CODE_START(entry_SYSCALL_64)
	UNWIND_HINT_ENTRY
	ENDBR

	swapgs
	/* tss.sp2 is scratch space. */
	movq	%rsp, PER_CPU_VAR(cpu_tss_rw + TSS_sp2)
	SWITCH_TO_KERNEL_CR3 scratch_reg=%rsp
	movq	PER_CPU_VAR(pcpu_hot + X86_top_of_stack), %rsp
	...
	/* Construct struct pt_regs on stack */
	pushq	$__USER_DS				/* pt_regs->ss */
	pushq	PER_CPU_VAR(cpu_tss_rw + TSS_sp2)	/* pt_regs->sp */
	pushq	%r11					/* pt_regs->flags */
	pushq	$__USER_CS				/* pt_regs->cs */
	pushq	%rcx					/* pt_regs->ip */
	...
	pushq	%rax					/* pt_regs->orig_ax */

	PUSH_AND_CLEAR_REGS rax=$-ENOSYS
	...
	movq	%rsp, %rdi
	/* Sign extend the lower 32bit as syscall numbers are treated as int */
	movslq	%eax, %rsi
	...
	call	do_syscall_64		/* returns with IRQs disabled */
```

| 行 | 在做什麼 |
|---|---|
| `swapgs` | 切換 GS 暫存器，讓 kernel 能找到「這顆 CPU 自己的資料」 |
| `movq %rsp, ...TSS_sp2` | 把**使用者的 stack pointer** 暫存起來 |
| `SWITCH_TO_KERNEL_CR3` | 切換到 kernel 的分頁表（`CR3` 是**控制暫存器**，就是 §2.3 說的那種特權暫存器）。這是防 Meltdown 的措施 |
| `movq ...top_of_stack, %rsp` | 換成 **kernel 自己的 stack** |
| `pushq ...` 五行 | 照 `pt_regs` 的**倒序**一個一個推進 stack：`ss`、`sp`、`flags`（`syscall` 指令先把 rflags 放在 `r11`）、`cs`、`ip`（`syscall` 指令先把返回位址放在 `rcx`） |
| `pushq %rax` | 存 system call 號碼到 `orig_ax` |
| `PUSH_AND_CLEAR_REGS` | 巨集：把剩下的一般暫存器全推進去，並**清成 0**（避免使用者塞的值被 kernel 誤用） |
| `movq %rsp, %rdi` | 現在 stack 頂端就是一個完整的 `pt_regs`，把它的位址當**第 1 個參數** |
| `movslq %eax, %rsi` | system call 號碼當**第 2 個參數** |
| `call do_syscall_64` | 進入 C，就是 [§6.4](#64-原始碼sys_call_table-與-do_syscall_x64) 那個函式 |

這段完全對應教授說的流程：**進 OS → 先存狀態 → 讀 `rax` 的號碼 → 查表 → 呼叫**。

---

## 8. Process 是一只行李箱

### 8.1 Process 的生命週期

`[50:31]`（課本 Fig 2.5）

```
 執行 ──► context switch ──► OS + 其他 process 執行 ──► context switch ──► 執行 ──► ...
```

- 控制權一交給 OS kernel，**不保證**會馬上回到這個 process。可能先跑別的 process。
- 但最後總會回來，繼續執行，然後又被打斷……一直進進出出。
- 在核心上跑叫 **swapped in**，沒在跑叫 **swapped out**。
- process 應該**完全不知道**有這麼多次 context switch。

### 8.2 可以換核心跑

`[51:30]` 【口頭】OS 把 process 看成一只**行李箱（suitcase）**，整個 context 都打包在裡面。

- 它在 CPU A 上跑，被換出去。
- 換回來時，**不一定**要回 CPU A。
- 回 CPU A 比較好，因為**快取是熱的（warm）**，資料就在附近。但**不是必要**。
- 可以在任何核心上恢復，因為：記憶體不用存（虛擬記憶體處理好了），要帶走的只有 **PC + 暫存器狀態**，裝進「context」這只行李箱就好。

`[53:00]` 你跑 Linux 的 **`top`** 指令，會看到 process **一直在核心之間跳來跳去**，完全沒問題。

`[53:30]` 為什麼要換來換去？

- 核心少、process 多，一定要輪流。
- process 可能在**等硬碟 I/O**，在資料回來前沒事做，就去**睡覺**。資料回來時，原本的 CPU 可能正在跑高優先權的 process，那就去別的 CPU。

【口頭】教授說他這裡**故意把 CPU 和 core 混著講**，嚴格來說兩者不一樣。

### 8.3 【Q&A】換核心不會有問題嗎？

`[54:39]`（⚠️ 學生問題沒錄到，從回答推斷是問快取）

**教授答**：快取是記憶體系統的一部分。記憶體我不用管，**虛擬記憶體和快取一致性（cache coherence）**會處理好一切。重要的只有**暫存器狀態**（把 PC 也當成暫存器的話）。

回原核心是為了**效能**，不是為了**正確性**。所以優先排回原核心，因為快取裡還有你的資料。

【口頭】教授的比喻 `[55:31]`：

> 「你常常從德里飛孟買，在孟買有一家車行負責接送。給你選的話，你會選同一家，因為你認識那家公司、它的管理、它的司機。同樣地，你在某顆 CPU 上跑過，它的快取裡還有你的資料，對你來說是熱的，所以你會偏好它。但**不保證**每次都如你所願，因為 process 有優先權。別的 CPU 有空，你就去那裡。」

### 8.4 一個核心一次只跑一個 process

`[56:01]` 每個核心都是**完全獨立的 pipeline**，有自己的 PC，各跑一個 process。八核 CPU 就是同時跑八個不同的程式。核心之間當然有透過記憶體、快取的協調機制，但那只是為了拿到狀態。

`[57:00]` 【口頭】很重要的觀念：

> 「如果這八個程式都不是 OS 程式，那 **OS 就沒在跑**。沒在跑的東西不能做任何事。
> 對課外的人來說可以說『OS』，但在這門課裡：**一個核心上跑一個 process**，是一個單執行緒的 process。八核裡有三核在跑 OS 的 process，完全 OK，但那是**三個不同的 process**，各做各的事。」

`[57:31]` process 有兩種：**使用者 process** 和 **OS process**。在 Windows 按 Ctrl+Alt+Del，或在 Linux 跑 `top`、`ps -ef`，就能看到所有 process，有些是 OS 特權、有些是使用者特權。

**重點**：不管哪種，**每個核心一次只跑一個 process**——可能是使用者 process，也可能是 OS process，但**絕不會同時跑兩個**。

`[58:32]` 下一講（第 2 章第 2 部分）講 process 怎麼被換出、換入。

---

## 9. 一頁速查表

### 9.1 核心觀念

| 觀念 | 一句話 |
|---|---|
| **實體位址空間** | 硬體看到的位址空間，只有一個；48 位元 = 256 TB |
| **特權暫存器** | 只有 OS / hypervisor 能用；四種：flags、控制、除錯、I/O |
| **為什麼需要特權暫存器** | 拍暫存器快照時，自己也要用暫存器——要「吊在天花板上」 |
| **CPL** | 早期：0 = OS、1 = 應用程式 |
| **Ring** | x86 有 4 個：0 = OS、1–2 = 虛擬機、3 = 應用程式（Linux 實際只用 0 和 3） |
| **Polymorphic 指令** | 在不同特權模式下效果不同、又不產生 exception；虛擬化的大麻煩 |
| **Context** | 執行中 process 的狀態：暫存器 + flags + PC（記憶體不用存） |
| **PC vs next PC** | page fault 存 PC（重跑），interrupt / system call 存 next PC |
| **Process = 行李箱** | context 打包好，可以在任何核心恢復；回原核心只是為了快取熱 |
| **一核一 process** | 一般 process 在跑時，OS **沒在跑** |

### 9.2 四種機制

| 機制 | 誰發起 | 方向 |
|---|---|---|
| Interrupt | 硬體 | → OS |
| Exception | 程式出錯（CPU 產生） | → OS |
| System call | 程式（`syscall` 指令產生假中斷） | → OS |
| Signal | OS | → Process |

### 9.3 x86-64 system call

| 項目 | 內容 |
|---|---|
| 發出 | `mov $號碼, %rax` → `syscall` |
| 參數 | `rdi, rsi, rdx, r10, r8, r9`（6 個） |
| 回傳值 | `rax` |
| `syscall` 指令順便用掉 | `rcx`（返回位址）、`r11`（rflags） |
| 返回指令 | `sysret` / `iret` |
| 舊方法 | `int $0x80`（32 位元 x86） |
| 表格數量（v6.2.12） | 548 格；64 位元可用 362 個 |
| 為什麼要 batching | system call 很貴；先寫進 4 KB 緩衝區再一次寫 |

### 9.4 本講看過的原始碼位置

| 檔案:行號 | 看什麼 |
|---|---|
| `arch/x86/include/asm/ptrace.h:59-92` | `struct pt_regs`：存下來的 context |
| `arch/x86/include/asm/ptrace.h:131-138` | `user_mode()`：用 `cs & 3` 判斷特權等級 |
| `arch/x86/include/asm/segment.h:33, 48` | `SEGMENT_RPL_MASK`、`USER_RPL` = 3 |
| `arch/x86/entry/syscalls/syscall_64.tbl` | system call 號碼表 |
| `arch/x86/include/asm/syscall.h:19-20` | `sys_call_ptr_t` 函式指標型別 |
| `arch/x86/entry/syscall_64.c:10-18` | `sys_call_table[]` 怎麼用巨集生出來 |
| `arch/x86/entry/common.c:40-54` | `do_syscall_x64()`：查表呼叫 |
| `arch/x86/entry/common.c:73-87` | `do_syscall_64()` |
| `arch/x86/entry/entry_64.S:68-120` | `entry_SYSCALL_64`：暫存器規則、建 `pt_regs` |
| `arch/x86/include/asm/irq_vectors.h:45` | `IA32_SYSCALL_VECTOR` = `0x80` |

---

## 10. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **一般程式不發 system call，OS 就永遠不會跑？** | **第 3 講**：timer interrupt、jiffies、inter-processor interrupt（課本 §2.1.4 後半） |
| **x86 組合語言、AT&T 語法** | **第 3 講**起常常出現；課本附錄 A |
| **記憶體不用存，因為有虛擬記憶體** | **第 5 講**：虛擬記憶體；**第 6 講**：segmentation（`cs`、`ss` 就是 segment 暫存器） |
| **I/O 暫存器** | **第 6 講**：port-mapped I/O、memory-mapped I/O、DMA |
| **Context、行李箱、換核心** | **第 12–13 講**：context switch 細節 |
| **`syscall` 指令、system call table、library call** | **第 14 講**：library call 與 system call（課本 §4.1） |
| **interrupt / exception 處理、函式指標表** | **第 15–16 講**：x86 中斷架構、exception、softirq |
| **Signal** | **第 17 講**：signal |
| **Polymorphic 指令、ring 1–2、hypervisor** | 課本第 8 章（虛擬化）。對應講次**待補**（播放清單目前到第 33 講） |

---

## 附：本講一句話總結

> **OS 靠特權暫存器、特權指令和 ring 0 才能「吊在天花板上」管理一切；一般程式只能透過 interrupt、exception、system call 這種「放火燒房子」的昂貴方式請 OS 進來——進來第一件事就是把暫存器存成 context（`pt_regs`），用號碼查函式指標表找到 handler，做完再原封不動地把 process 放回去。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 2（Chapter 2, Part I）錄影逐字稿（YouTube 自動字幕，59:19）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）第 2 章 §2.1.1–2.1.4，以及 Linux kernel v6.2.12 原始碼整理。*
