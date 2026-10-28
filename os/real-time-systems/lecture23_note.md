# Real Time Systems 第二十三支影片完整筆記：以 Linux 為基礎的 RTOS（Lecture 24）

> **課程**：Real Time Systems — Lecture 24：Linux-based RTOSs（播放清單第 23 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（37:06，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch5 §7.6（PDF p.129–130）
> **標記說明**：
> - `【口頭】`：老師在課堂上口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這堂課沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（`檔名:行號`），用來對照觀念
> - `【實測】`：在本機（Ubuntu、kernel 6.5.0、x86-64）實際執行的結果
> - `⚠️`：逐字稿缺字、或課堂／課本／原始碼說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 做法一：MontaVista（改 kernel）](#1-做法一montavista改-kernel)
  - [1.1 Kernel patch 做了什麼](#11-kernel-patch-做了什麼)
  - [1.2 為什麼需要：多媒體與多處理器](#12-為什麼需要多媒體與多處理器)
  - [1.3 【原始碼】PREEMPT_RT：spin lock 變成 rt_mutex](#13-原始碼preempt_rtspin-lock-變成-rt_mutex)
- [2. 做法二：RTLinux、RTAI、Xenomai（在 kernel 下加一層）](#2-做法二rtlinuxrtaixenomaiin-kernel-下加一層)
  - [2.1 三個專案的歷史](#21-三個專案的歷史)
  - [2.2 共同架構](#22-共同架構)
  - [2.3 RTAI 的細節](#23-rtai-的細節)
  - [2.4 虛擬化中斷：假裝關中斷](#24-虛擬化中斷假裝關中斷)
  - [2.5 即時任務跑在 kernel space](#25-即時任務跑在-kernel-space)
  - [2.6 排程器與計時器](#26-排程器與計時器)
  - [2.7 即時通訊](#27-即時通訊)
  - [2.8 優缺點](#28-優缺點)
- [3. 【課本補充】RT Linux 是 dual kernel](#3-課本補充rt-linux-是-dual-kernel)
- [4. Linux vs RTLinux vs RTAI](#4-linux-vs-rtlinux-vs-rtai)
- [5. 【實測】函式呼叫 vs 系統呼叫](#5-實測函式呼叫-vs-系統呼叫)
- [6. 一頁速查表](#6-一頁速查表)
- [7. 與後續講次的連結](#7-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

上一講的兩條路，這講各看實例：

| 做法 | 代表 | 怎麼做 |
|---|---|---|
| 一：改 kernel | **MontaVista**（Hard Hat Linux）、PREEMPT_RT | 把 kernel 的鎖換成有 priority inheritance 的 mutex、高解析度計時器、縮小不可搶占區域 |
| 二：在 kernel 下加一層 | **RTLinux、RTAI、Xenomai** | 即時 kernel 攔截所有中斷，Linux 當最低優先權的任務；即時任務是 kernel module |

老師：這些做法有點舊，但 RTAI 和 Xenomai 到 2022 年仍以不同形式存在。

---

## 1. 做法一：MontaVista（改 kernel）

### 1.1 Kernel patch 做了什麼

[01:03]–[04:30]

背景【口頭】：VRTX 是很經典的即時系統（後來被 EDA 公司 Mentor Graphics 收購），也影響了 VxWorks 和 Wind River——Wind River 長期是 RTOS 的市場龍頭。

- **MontaVista Linux** 以前叫 **Hard Hat Linux**（相對於當時最流行、但即時性不足的 Red Hat）。後來成為完整的 RTOS，支援硬即時任務，回應時間至少和當時（約 2000–2010）其他 RTOS 一樣快。
- 它加的 **kernel patch**（對基準 kernel 的修改）：
  1. 一般的 mutex → **有 priority inheritance 的 kernel mutex**。
  2. 某些地方取代**不可搶占的 kernel 鎖**：早期 Linux（和一些舊程式碼）有「**一把大鎖**」（big kernel lock），對即時系統很糟 → 改成 PI mutex。
  3. **Spin lock** 也換成 PI mutex（上一講：spin lock 不適合即時，因為沒有 priority inheritance）。
  4. 加入**高解析度計時器**。
- Linux kernel 後來也採納了可搶占 kernel；即時任務的回應時間從約 **1 ms** 一路降到約 **100 μs**。
- 老師的圖：早期 kernel 大部分不可搶占；後來不可搶占的（紅色）區域越來越小。到現在（kernel 5.x）只有非常小的部分不可搶占，連一般 Linux kernel 都比以前即時得多。

### 1.2 為什麼需要：多媒體與多處理器

[05:00]–[06:00]

- 主要需求是**音訊和影片**：一邊編譯一邊聽歌，音訊也要即時，否則會聽到 jitter。之後是影片、多媒體、遊戲。
- **SMP（對稱多處理器）Linux** 出現：程式碼本來就得處理並行（用 spin lock），多顆 CPU 帶來更多 context switch 和搶占。一開始即時性能反而下降，之後逐步加入功能，多核 / 多處理器的即時性能才改善。

### 1.3 【原始碼】PREEMPT_RT：spin lock 變成 rt_mutex

MontaVista 的想法（把 spin lock 換成有 PI 的 mutex）後來成為 PREEMPT_RT 的核心，v6.2.12 主線已經有這套程式碼（只是一般 kernel 不啟用，第 2 支影片筆記）：

`include/linux/spinlock_rt.h:19`

```c
#define spin_lock_init(slock)					\
do {								\
	static struct lock_class_key __key;			\
								\
	rt_mutex_base_init(&(slock)->lock);			\
	__rt_spin_lock_init(slock, #slock, &__key, false);	\
} while (0)
```

`include/linux/spinlock_rt.h:43`

```c
static __always_inline void spin_lock(spinlock_t *lock)
{
	rt_spin_lock(lock);
}
```

- 開了 `CONFIG_PREEMPT_RT` 時，`spinlock_t` 底層是一個 **rt_mutex**：拿不到就**睡**（不再空轉），持有者可以被搶占，而且有 **priority inheritance**（第 11 支影片筆記的 `rt_mutex_adjust_prio`）。
- 真的不能睡的少數地方（排程器、中斷入口、硬體中斷處理）改用 `raw_spinlock_t`，保留原本關搶占的行為——這就是 Kconfig 說明裡「除了非常低階、關鍵的程式路徑以外，kernel 全部可搶占」的意思。
- 對照 MontaVista：PI mutex 取代 spin lock 和大鎖、高解析度計時器（hrtimer，第 3 支影片筆記）、縮小不可搶占區域——全都在主線 kernel 裡看得到。

---

## 2. 做法二：RTLinux、RTAI、Xenomai（在 kernel 下加一層）

### 2.1 三個專案的歷史

[06:00]–[10:01]

| 專案 | 歷史（老師） |
|---|---|
| **RTLinux** | 第一個 Linux 即時擴充。作者很早就申請了涵蓋即時功能的**軟體專利**，嚇跑了很多開發者和使用者（被開源與 GNU 授權吸引來的人）→ 生態系長不起來（老師說這是流傳的說法）。2007 年 2 月賣給 Wind River，成為 Wind River Linux 的即時核心；商業支援 2011 年結束，現在已不存在，但深刻影響了即時系統的設計思路 |
| **RTAI** | 約 1999 年同時開始；**社群專案**，使用者多、貢獻者多（喜歡開源授權、不滿 RTLinux 的人）；當時針對 x86（那時 ARM 還不普及） |
| **Xenomai** | RTAI 的競爭者。老師：延遲較好，但一般認為程式碼較難維護、較難移植。符合 POSIX-RT 的層，有原生 API，也有 **skin**（模仿其他傳統 RTOS 的 API，例如 pSOS、VxWorks）。當作 Linux 的 **co-kernel**：一個大到幾乎取代原 kernel 的模組。程式碼**永遠不會併入主線 kernel** |

⚠️ 老師說「PREEMPT_RT 編譯旗標可以在 kernel 3.0 分支啟用 [Xenomai 的] 大部分」[09:31]。PREEMPT_RT 是做法一（改 kernel）的 patch，和 Xenomai 是不同的東西；Xenomai 3 可以選擇跑在 dual kernel（Cobalt）或單純跑在 PREEMPT_RT kernel 上（Mercury）。逐字稿這段語意不清，待補。

共同點：**kernel 本身不修改**（或只改很少），即時部分是獨立的：要嘛是 kernel 下面的一層，要嘛是載入 kernel、接管大部分功能的大模組。建置方式：下載原始碼、編譯。

### 2.2 共同架構

[10:32]–[14:00]

```
一般 Linux：                              做法二：
 User processes                           User processes（Linux 上）     即時任務
 System libraries                         Linux kernel（最低優先權的「即時任務」）│
 Linux kernel（驅動程式）                   ───────────────────────────────┤ 即時排程器
 Hardware（中斷 → kernel）                  RT Linux plugin（攔截所有中斷、I/O）
                                          Hardware
```

- 即時 kernel 攔截**所有**中斷：
  - 要讓即時任務跑的中斷 → 即時 kernel 處理；Linux 正在跑就**搶占** Linux。
  - 給 Linux 的中斷 → 轉交 Linux kernel。
- 可以把它看成 **virtual machine manager / hypervisor**：硬體在中心，即時 kernel 包一層，Linux 再包一層（課本 Fig.12 的同心圓）。
- 即時任務由即時排程器排程，很多權限很高，可以**直接存取硬體**。
- Linux 和其上的應用是**背景任務**（最低優先權）；中斷、I/O 指令、訊息都可以轉交給它，就像 hypervisor 對 guest 做的那樣——老師稱之為「有特定應用領域（即時）的早期 hypervisor」。

要的功能：即時排程演算法、所有操作的延遲可預測、細粒度計時器、可搶占的 kernel。

- **RTLinux 的 executive 很小**：本身其實**不可搶占**，但常式很短很快，不會造成大延遲。
- Linux kernel 照原樣跑，有各種沒保證的延遲——但它現在只是一個應用，無所謂。
- **沒有虛擬記憶體**支援，事情簡單很多。

### 2.3 RTAI 的細節

[15:00]–[19:00]

- 小即時 kernel 和一般 Linux kernel **共存**；支援 **EDF、RMS**（老師：這兩類各自最佳，非支援不可），也支援 FIFO。
- 即時 kernel 和 Linux kernel **解耦**，可以各自最佳化；即時 kernel 小而簡單：只負責跑即時任務和 Linux kernel。
- 即時任務和一般 Linux 程序透過**共享記憶體**溝通。
- **對 Linux kernel 的改動很少**：主要是加一層 **HAL（hardware abstraction layer）**。
  - RTLinux 比較「侵入」：改了一些 kernel 原始檔 → 每次 kernel 更新都可能壞掉，追蹤更新、修 bug 都難——這也是 RTLinux 沒撐下來、RTAI 存活的原因之一。
  - 當年的 HAL 是一個結構：指向中斷向量和中斷啟用 / 停用函式的指標；即時 kernel 把中斷處理和模擬程式碼集中在這裡，決定中斷送往即時任務還是 Linux。改動大多是自訂的中斷處理常式。
- RTAI 的 HAL 有五個模組（老師列出）：基本排程、多處理器排程、基本框架、**rtai_shm**（即時任務與一般任務的共享記憶體）、**FIFO**（交換資訊）。

### 2.4 虛擬化中斷：假裝關中斷

[19:00]–[21:01]

```
Linux 要關中斷 ──trap──▶ RTAI：記下「Linux 認為中斷已關」，回報成功，但硬體中斷其實沒關

硬體中斷來了 ──▶ RTAI 判斷目的地
   ├─ 即時任務：處理、必要時排程
   └─ Linux：看 Linux 的（虛擬）中斷狀態
         ├─ 關：放進佇列，之後再送
         └─ 開：直接呼叫 Linux 的中斷處理常式（Linux 分不出是真硬體還是 RTAI 呼叫的）
   （所有情況都記錄中斷狀態）
```

- 讓 Linux 沒有執行關中斷指令的權限 → 一執行就 trap 到即時 kernel → 即時 kernel 設好某個暫存器值，讓 Linux 以為成功了。
- 老師：這是虛擬機器課會教的技巧——一個專為即時打造的早期虛擬機器。

### 2.5 即時任務跑在 kernel space

[21:01]–[25:02]

- RTAI 的即時程序主要是 **kernel module**（kernel 擴充模組）：以特權模式執行，能存取 kernel 符號表，進入 kernel 的位址空間——和 kernel 沒有差別。
- 好處：
  - 不會被換出（沒有 swap 的問題）。
  - 完整的監督者權限，直接存取硬體與 I/O。
  - RTOS 和應用共享單一位址空間 → **系統呼叫變成 kernel 裡的普通函式呼叫**，不用存暫存器、存狀態、進 kernel、記帳。
- 壞處：抽象被打破、沒有保護；模組有 bug 就能讓整個 kernel 當掉。
- 老師：大部分即時系統其實是偽裝的嵌入式系統，所以這「複雜但沒那麼糟」。

### 2.6 排程器與計時器

[22:01]–[29:00]

**排程器**：

- 一般 kernel 用 spin lock 和中斷遮罩，在即時情境中不再那麼重要。
- 以**優先權**為基礎，支援固定與動態優先權；context switch 很快（老師：**15 μs**），這是最重要的工程目標。
- 像 Linux 的 SCHED_OTHER 一樣可以加更多排程選項；使用者可以把自己的排程器寫成 loadable kernel module（改良 EDF / RMS 或自創）。
- Linux 是**最低優先權**的即時任務，內部再有自己的排程。

**計時器**：

- 計時誤差會讓即時任務偏離排程 → 釋出與完成的 jitter。
- 一般桌機 Linux 的硬體中斷每 **1–10 ms** 一次（**jiffy**）；中斷處理常式可能只記下時間前進了一個 jiffy 就返回。Linux 的時間片一般 10–200 ms，一個程序會連跑好幾個 jiffy。
- **RTLinux 是最早提供微秒級精度的 OS 之一**，之後其他系統甚至到奈秒。
- 當年用的是**可程式化間隔計時器**晶片（逐字稿為「a354」，應是 Intel **8254** PIT）；老師說一般 Linux 計時器以前是 10 ms，現在是 1 ms（資料有點舊；本機 `CONFIG_HZ=250`，即 4 ms，第 5 支影片筆記）。
- RTAI 讓計時器受即時控制、有自己的中斷處理常式；因為 RTOS、中斷處理和即時程序在同一個位址空間，讀時間只是一個函式呼叫。

兩種計時器：

| | 週期計時器 | 一次性計時器 |
|---|---|---|
| 開銷 | **小**：設定一次，每 T 自動產生中斷，不必每次重設 | 每次呼叫排程器都要重新設定 → 有開銷 |
| 用途 | 週期任務 | 量時間間隔、確保程序在鬧鐘前做完、非週期與偶發工作 |

（第 5 支影片的實測：週期 timerfd 300 個 frame 只設定 1 次。）

### 2.7 即時通訊

[29:00]–[31:31]

- 即時程序在 kernel mode → 可以直接用 kernel 的資料結構：**FIFO、message queue、RPC、共享記憶體**（FIFO 建在共享記憶體上）。
- 在一般 Linux 上這些要系統呼叫，很貴；這裡不用。
- **效能之王是共享記憶體**：不用任何函式呼叫，只要有共享頁、在上面做佇列、一直讀寫。但多執行緒時容易有同步 bug，需要有經驗的程式設計師。適合**無結構**的原始位元組。
- **結構化**資料：kernel mode 的 message queue 和 RPC 也不錯。
- Linux kernel 本身優先權很低、隨時會被即時任務搶占 → 用標準 Linux 原語溝通不是好主意 → RTAI 的 IPC 用簡單的 **FIFO 緩衝**（RT FIFO）。

### 2.8 優缺點

[31:31]–[34:30]

| 優點 | 缺點 |
|---|---|
| 原始碼公開 | 除錯困難：即時應用也跑在 kernel mode |
| 大部分可以微調；某個部分或驅動有問題可以**動態卸載**模組 → 某種程度上穩健 | 系統呼叫有限（OS 必須小，不能很有表達力） |
| | kernel mode 的即時任務**不能充分利用 Linux 的服務**（檔案系統、網路）：它們和 Linux 並行，不是在 Linux 上面 |
| | 執行環境**脆弱**：安全、保護、隔離都沒了，bug 的影響很難圍堵（但對即時嵌入式系統要看情況） |
| | 只能用 POSIX API 的一小部分；把既有 Linux 程式搬過來可能很難 |

RTAI 系統示意【口頭】：硬體（顯示器、磁碟、網路）上面，kernel 裡有 RTAI、即時程序、Linux kernel（最低優先權的即時任務）；user space 有 X Window、一般使用者程序；即時程序和使用者程序用 **RT FIFO** 溝通。

---

## 3. 【課本補充】RT Linux 是 dual kernel

（課本 §7.6，p.129–130）

- 課本把 RT Linux 歸為 **self-host** OS（上一支影片筆記）。
- RT Linux 層在硬體和 Linux 之間；對標準 Linux kernel 來說，RT Linux 層**看起來就是硬體**。
- RT Linux kernel 攔截所有硬體中斷；和即時無關的中斷先**擱著**，等 RT Linux kernel 閒下來、標準 Linux 在跑時，再以**軟體中斷**的形式交給 Linux。
- 即時應用寫成 **loadable kernel module**，在 kernel space 執行。
- 實際上有兩個獨立的 kernel → 叫 **dual kernel approach**。

和 microkernel 做法相比的缺點：

| 缺點 | 說明 |
|---|---|
| **重複寫程式** | 即時 kernel 的任務不能有意義地使用 Linux 服務（檔案系統、網路）——一呼叫 Linux 服務，就受制於讓 Linux 程序無法確定性執行的同樣搶占問題。所以即時 kernel 要另寫驅動和系統服務，即使 Linux 已有 |
| **脆弱的執行環境** | 即時任務沒有 MMU 保護，在 kernel space 不受保護；一個壞指標就能讓 kernel 致命錯誤——很多嵌入式應用是安全關鍵的 |
| **可攜性差** | 即時任務不是 Linux 程序，只用一小部分 POSIX API；而且不同 dual kernel 實作的 API 不同，A 廠商的程式不一定能在 B 廠商的上面跑 |
| **難寫** | 只支援有限的 POSIX API，開發更費工 |

（和老師講的 RTAI 缺點幾乎一一對應。）

---

## 4. Linux vs RTLinux vs RTAI

[34:30]–[36:30]（老師的比較表，依口頭整理）

| 項目 | Linux | RTLinux | RTAI |
|---|---|---|---|
| 平台 | 很多 | 少 | 少 |
| 多處理器 | 支援 | 支援 | 支援 |
| 即時排程 | SCHED_FIFO、SCHED_RR | EDF、RMS（最有表達力） | 曾經只支援 RMS |
| 執行單元 | 程序、執行緒全套 | 只有執行緒 | 輕量程序（就是執行緒） |
| 優先權等級 | — | 很多 | 很多 |
| 記憶體保護、動態記憶體、IPC | 完整 | 不是重點（有一點） | 不是重點（有一點） |
| 同步 | — | 有 PI 的 semaphore、mutex；FIFO | 同左 ＋ **共享記憶體** |
| Priority inversion 控制 | — | 有 | 有 |
| QoS | 有一些，但保證不明確 | — | — |
| 其他（網路檔案系統等） | 有 | 沒有 | 沒有 |

（投影片表格的完整內容逐字稿沒有逐格唸出，部分格子待補。）

---

## 5. 【實測】函式呼叫 vs 系統呼叫

RTAI「即時任務在 kernel space，系統呼叫變成函式呼叫」的好處有多大？在使用者空間量一個不內聯的函式呼叫和一個最簡單的系統呼叫：

```c
__attribute__((noinline)) long service(long x) { __asm__ volatile("" ::: "memory"); return x + 1; }
...
for (int i = 0; i < N; i++) acc = service(acc);
...
for (int i = 0; i < N; i++) acc += syscall(SYS_getppid);
```

```
plain function call 0.42 ns, system call (getppid) 60.3 ns -> 145x
```

- 一個系統呼叫（就算什麼都不做）比函式呼叫貴約 **145 倍**：要切換特權模式、存還原暫存器、kernel 入口的安全檢查（例如 Spectre / Meltdown 的緩解措施）等。
- 這正是 RTAI 把即時任務放進 kernel 的動機；代價是沒有保護（§2.8、§3）。
- 對照第 19 支影片：系統呼叫 64 ns、經 IPC 找使用者模式 server 2251 ns。三個數字放一起：函式呼叫 0.4 ns ≪ 系統呼叫 60 ns ≪ IPC 2 μs——RTAI、monolithic、microkernel 三種取捨。

---

## 6. 一頁速查表

| 主題 | 重點 |
|---|---|
| MontaVista（Hard Hat Linux） | kernel patch：PI mutex 取代一般 mutex、大鎖、spin lock；高解析度計時器；回應時間 1 ms → 100 μs |
| 主線 Linux | 不可搶占區域越來越小；PREEMPT_RT 下 `spin_lock()` → `rt_spin_lock()`（rt_mutex、PI），只有 raw spinlock 不可搶占 |
| 動機 | 音訊、影片、遊戲；SMP |
| RTLinux | 第一個；專利嚇跑社群；2007 賣給 Wind River，2011 停止 |
| RTAI | 1999 社群專案；改動少（HAL）；五個模組（排程、MP、框架、shm、FIFO） |
| Xenomai | co-kernel；POSIX-RT ＋ skin；不併入主線 |
| 架構 | 即時層攔截所有中斷；Linux 是最低優先權任務；虛擬化「關中斷」 |
| 即時任務在 kernel | 系統呼叫 = 函式呼叫（實測 0.4 vs 60 ns）；不會被換出；但無保護、難除錯、不能用 Linux 服務 |
| 計時器 | RTLinux 最早到微秒級；週期計時器開銷小、一次性每次重設 |
| 通訊 | 共享記憶體最快（無結構）、message queue / RPC（結構化）；RTAI 用 RT FIFO |
| 課本 | dual kernel 的缺點：重複寫程式、脆弱、不可攜、難寫 |

---

## 7. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **VRTX、VxWorks、pSOS、QNX、µC/OS-II、Lynx** | **第 24 支**：Commercial RTOSs |
| **Windows CE（priority inheritance 處理 DPC）** | **第 25 支**：Windows CE |
| **context switch、中斷延遲等指標的量測** | **第 26 支**：Benchmarking RTOSs |

---

## 附：本講一句話總結

> **以 Linux 為基礎的 RTOS 有兩種做法：MontaVista（原名 Hard Hat Linux）走「改 kernel」路線，把一般 mutex、big kernel lock 和 spin lock 都換成有 priority inheritance 的 mutex、加上高解析度計時器，讓回應時間從 1 ms 降到約 100 μs，這套想法後來變成主線 Linux 的 PREEMPT_RT（v6.2.12 中開啟後 spin_lock() 就是以 rt_mutex 實作的 rt_spin_lock()）；RTLinux、RTAI、Xenomai 則走「在 kernel 下加一層」路線，即時 kernel 攔截所有中斷、把 Linux 當成最低優先權的任務，Linux 想關中斷時只是被記錄並回報成功，給 Linux 的中斷在它「開中斷」時才送達。這類系統把即時任務寫成 kernel module，系統呼叫變成函式呼叫（實測 0.4 ns vs 60 ns）、不會被換出、可直接存取硬體，計時器可達微秒級，用共享記憶體與 RT FIFO 溝通，支援 EDF、RMS；代價是沒有記憶體保護、難除錯、不能使用 Linux 的檔案系統與網路服務、只能用一小部分 POSIX API，也就是課本所說 dual kernel 的重複開發、脆弱與不可攜。RTLinux 因專利問題沒形成社群，2007 年賣給 Wind River、2011 年停止，而改動較少的 RTAI 與 Xenomai 存活至今。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 24 錄影逐字稿（YouTube 自動字幕，37:06）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch5 §7.6 整理。原始碼為 Linux v6.2.12；實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
