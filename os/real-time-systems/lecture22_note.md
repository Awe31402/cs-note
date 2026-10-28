# Real Time Systems 第二十二支影片完整筆記：開源與商用 RTOS（Lecture 23）

> **課程**：Real Time Systems — Lecture 23：Open Source and Commercial RTOSs（播放清單第 22 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（38:50，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch5 §4（PDF p.117–121）
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
- [1. 為什麼 RTOS 重要](#1-為什麼-rtos-重要)
- [2. 讓 Unix 變即時的五種做法](#2-讓-unix-變即時的五種做法)
  - [2.1 延伸 Unix kernel](#21-延伸-unix-kernel)
  - [2.2 Host-target](#22-host-target)
  - [2.3 Preemption point](#23-preemption-point)
  - [2.4 Fully preemptible kernel 與 spin lock](#24-fully-preemptible-kernel-與-spin-lock)
  - [2.5 【課本補充】Self-host 與即時優先權](#25-課本補充self-host-與即時優先權)
- [3. 【原始碼】Linux 的 preemption point：cond_resched](#3-原始碼linux-的-preemption-pointcond_resched)
- [4. 為什麼大家都拿 Linux 來改](#4-為什麼大家都拿-linux-來改)
- [5. Linux 的兩個元兇](#5-linux-的兩個元兇)
  - [5.1 中斷處理](#51-中斷處理)
  - [5.2 鎖](#52-鎖)
- [6. 讓 Linux 即時的兩條路](#6-讓-linux-即時的兩條路)
  - [6.1 改 kernel 本身](#61-改-kernel-本身)
  - [6.2 Interrupt abstraction：在 kernel 下面加一層](#62-interrupt-abstraction在-kernel-下面加一層)
  - [6.3 Kernel latency 的組成](#63-kernel-latency-的組成)
- [7. 【實測】一般 Linux 的喚醒延遲](#7-實測一般-linux-的喚醒延遲)
- [8. 2021–2022 年常見的 RTOS](#8-20212022-年常見的-rtos)
- [9. 結語](#9-結語)
- [10. 一頁速查表](#10-一頁速查表)
- [11. 與後續講次的連結](#11-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

老師說這講看的是 90 年代末、2000 年代初的經典設計，但原則至今不變。

1. 怎麼把 Unix 類 OS 變即時：延伸 kernel、host-target、preemption point、fully preemptible kernel（＋課本的 self-host）。
2. Linux 的兩個元兇：**中斷**和**鎖**。
3. 兩條路：改 kernel（PREEMPT_RT 一類）或在 kernel 下面加一層（RTLinux、RTAI、Xenomai）。
4. 現在常見的 RTOS 清單。

---

## 1. 為什麼 RTOS 重要

[01:00]–[02:02]

- 汽車、自駕、各種 IoT 都是 RTOS 的大用戶。老師提到汽車用的 OSEK/VDX（逐字稿為「vdx」）、Symbian、Android、iOS。
- 手機 OS 雖是通用的，但電話、影片等有即時需求，至少是軟即時。
- 世界各地（日本、歐洲、美國）有不同的標準。

---

## 2. 讓 Unix 變即時的五種做法

### 2.1 延伸 Unix kernel

[02:02]–[03:30]（課本 §4.1，p.117）

- 加即時任務排程器（新的可排程任務類別，用 EDF 或 RMS）、更細的計時器、即時檔案系統與儲存。
- 但底層 OS 不是即時的，這些附加只能稍微緩解。兩大問題依舊：
  1. kernel 的大部分**不可搶占**；
  2. 中斷處理、儲存與 I/O **不是即時就緒**的。

課本：表面延伸而不處理根本缺點（不可搶占 kernel、動態優先權），遠遠達不到硬即時應用的要求。

### 2.2 Host-target

[04:00]–[10:31]（課本 §4.2，p.117–118）

```
 Host（Unix / Windows / Linux）                         Target board
 ┌──────────────────────────────┐   TCP/IP / 序列埠 /   ┌──────────────────┐
 │ 編輯器、cross-compiler、       │ ─────USB──────────▶  │ ROM / flash 上的   │
 │ cross-debugger、函式庫、虛擬 CPU │                      │ 小型 RTOS ＋ 應用   │
 └──────────────────────────────┘                      └──────────────────┘
```

- 在 Unix host 上開發、除錯、測試（必要時用虛擬 CPU，因為 target 的 CPU 多半不同），再下載到專用的（多半不是 Unix 的）target OS。
- 為什麼：target 太小太弱，撐不起完整開發環境（編譯、除錯、函式庫、虛擬記憶體）。target OS 小而快，適合嵌入式。
- Target 的**任務搶占時間**預期約 **10 μs**；傳統 Unix 上可達 10–100 ms（老師）。
- **Cross-compiler**：在 host 編譯，產生 target 的程式碼。**Cross-debugger**：在 host 上模擬 target 並除錯。**Target debugger**：target 上只留一點點除錯能力；現在常接 USB 除錯器把資訊送回 host。
- 測完後經網路或 USB 傳到 target；早期是把程式**燒進 ROM**，直接換 ROM。
- 例：Wind River 的系統、embedded Linux、uClinux 等；老師以 **pSOS** 為例（host 上有 cross-compiler、函式庫、編輯器，XRAY+ 是 source-level cross-debugger）。
- 歷史【口頭】：Wind River 在即時系統發展中很有代表性；pSOS 的開發已停止，但很多即時專案受它啟發。另有一個至今（2022）仍在的專案支援 pSOS+ 的 API，讓既有的工業即時應用遷移到 Linux 環境——一個在 Linux 上提供即時執行的層。（專案名稱逐字稿聽不清，待補。）

⚠️ 老師說「Wind River 被 VxWorks 併購」[09:31]，隨即改口說是「和 VxWorks 有某種程度的整合」。VxWorks 是 Wind River 的產品，不是一家公司（課本 p.127 也說 pSOS 由 Wind River 提供）。

【課本補充】（p.118）商用 host-target RTOS：pSOS、VxWorks、VRTX。target OS 因為小、功能有限、設計精簡，任務搶占時間只有幾微秒，傳統 Unix 是幾百毫秒。

### 2.3 Preemption point

[10:31]–[11:32]（課本 §4.3，p.118–119）

- 在系統呼叫裡插入固定的 **preemption point**：在這些點上 kernel 資料結構保證一致，可以安全搶占。
- 不是讓 kernel 每一點都可搶占，而是只在這些點可以。
- 到了這些點，OS 看看有沒有別的事要跑（系統服務、子系統、中斷）或排別的即時任務。
- 例：HP-UX、Linux 等（課本：HP-UX、Windows CE）。

課本：最壞搶占延遲 = 兩個相鄰 preemption point 之間最長的時間；比沒有 preemption point 的傳統 OS 好好幾倍，適合很多硬即時應用，但仍不到幾微秒以下的要求。好處是 kernel 程式碼只需小改。

### 2.4 Fully preemptible kernel 與 spin lock

[11:32]–[14:00]（課本 §4.4，p.119–120）

- 整個 kernel 像使用者程式一樣**任何時候都能被搶占**。前提：kernel 資料結構要一直保持一致 → 用鎖。

| 鎖 | 行為 | 適合 |
|---|---|---|
| **Kernel-level lock** | 拿不到就**阻塞**（會睡、有 context switch） | critical section 長 |
| **Spin lock** | 拿不到就**一直迴圈檢查**（busy wait） | critical section 很短；但浪費時間、資源、電 |

- 拿著 spin lock 被搶占 → 大問題：所有在等的人都要等很久。
- Spin lock 的原理：某記憶體位址設成 1 表示被拿走；等的人反覆檢查直到它變 0。

【課本補充】（p.119–120）

- 為什麼要鎖：兩個程序都要把共享變數 i 加一，若都先讀到 2，各加一再寫回，結果是 3 而不是 4。
- Spin lock 的例子：某任務只需要資源很短（例如 1 ms 的運算）。若用 kernel-level lock：拿不到 → 阻塞、context switch（快取、頁也可能被換掉）→ 1 ms 後鎖釋放 → 又一次 context switch → 回應時間 3 ms；若 busy wait 只要 2 ms。
- 多處理器上 spin lock 通常利用快取一致性協定，每顆處理器在自己快取的副本上迴圈。
- 單處理器上，資源只需很短時間時，**關中斷**就能互斥：x86 的 `cli` / `sti`。所以單處理器上 spin lock 會編譯成 `cli`、`sti`。

### 2.5 【課本補充】Self-host 與即時優先權

（課本 §4.4，p.119–121；host-target 以外的另一種做法，課堂沒講）

- **Self-host**：在**同一個**系統上開發和執行即時應用。部署時把執行期間不需要的 OS 模組拿掉，再把應用和精簡過的 OS 燒進 target 的 ROM 或 flash。
- 大多數 self-host OS 是 **microkernel** 架構：只有中斷處理、程序管理等核心功能在 kernel；記憶體管理、檔案管理、裝置管理等是 user mode 的附加模組 → 容易裁剪；元件在受保護的獨立位址空間，很少因此當機。
- 解決 Unix 的兩大問題：
  - 不可搶占 kernel → 在 kernel 適當位置用 kernel-level lock 和 spin lock（上一節）。
  - 動態優先權 → 三種優先權：

| 類別 | 等級（課本 Fig.7） | 說明 |
|---|---|---|
| Real-time | 0–127 | 靜態，不重算；硬即時任務用這些，優先於動態等級 |
| Dynamic | 128–254 | 定期重算，讓 I/O bound 往上、CPU bound 往下 |
| Idle（non-migrating） | 255 | 最低；沒事時的 idle 任務，靜態 |

---

## 3. 【原始碼】Linux 的 preemption point：cond_resched

Linux 一般桌機 kernel（本機的 `PREEMPT_VOLUNTARY`）用的正是 **preemption point**：

`kernel/Kconfig.preempt:32`

```
config PREEMPT_VOLUNTARY
	bool "Voluntary Kernel Preemption (Desktop)"
	depends on !ARCH_NO_PREEMPT
	select PREEMPT_VOLUNTARY_BUILD if !PREEMPT_DYNAMIC
	help
	  This option reduces the latency of the kernel by adding more
	  "explicit preemption points" to the kernel code. These new
	  preemption points have been selected to reduce the maximum
	  latency of rescheduling, providing faster application reactions,
	  at the cost of slightly lower throughput.

	  This allows reaction to interactive events by allowing a
	  low priority process to voluntarily preempt itself even if it
	  is in kernel mode executing a system call. This allows
	  applications to run more 'smoothly' even when the system is
	  under load.
```

這些「explicit preemption point」就是 kernel 程式碼裡的 `cond_resched()` 呼叫。例如釋放大塊記憶體時，每處理一批 page table entry 就檢查一次：

`mm/memory.c:1479`（`zap_pte_range()` 的結尾）

```c
	if (addr != end) {
		cond_resched();
		goto again;
	}
```

- 這裡剛處理完一批、鎖已經放掉（資料結構一致）→ 課本說的「kernel 資料結構一致的點」。
- 有更高優先權的任務在等，就在這裡切換；沒有就繼續。
- 【實測】`grep -rw "cond_resched()"` 在 v6.2.12 的 `.c` 檔裡出現 **1145** 次（其中 fs 285、mm 181、kernel 73）——就是 1145 個左右手工放的 preemption point。

對照課本的三種模型：

| 課本 | Linux 選項 |
|---|---|
| 不可搶占 kernel | `PREEMPT_NONE`（伺服器：「傳統模型，重吞吐量」） |
| Preemption point | `PREEMPT_VOLUNTARY`（本機預設） |
| Fully preemptible（用鎖保護） | `PREEMPT`（spin lock 以外都可搶占）；`PREEMPT_RT`（連 spin lock 都換成可搶占的 rt_mutex，第 2 支影片筆記） |

本機是 `PREEMPT_DYNAMIC`（`uname -v`），可以在開機參數 `preempt=none/voluntary/full` 切換。

---

## 4. 為什麼大家都拿 Linux 來改

[14:00]–[15:00]

很多即時方案都以 Linux 為目標（在 Linux 上加一層，或加一個 module），因為：

- 原始碼免費取得；穩健可靠。
- 模組化、可設定：可隨需載入的 kernel module、編譯選單和執行期的大量選項。
- 可擴展：從伺服器到小裝置都能跑。
- 網路與網際網路支援很好；不需要執行期授權費；熟悉 Linux kernel 的開發者很多。

2004 年的一項調查【口頭】：問開發者和使用者什麼阻礙 Linux 用在即時應用上，除了軟體與環境支援，主要是 **kernel 的即時限制**——kernel 仍是通用的，不是即時就緒的。

---

## 5. Linux 的兩個元兇

### 5.1 中斷處理

[15:00]–[18:01]

- Linux 的 IRQ 子系統延遲**沒有上限**：任務搶占延遲隨硬體中斷負載增加，因為不知道中斷要處理多久。
- 中斷處理常式**不可搶占**；中斷**沒有優先權系統**——有些中斷其實幾小時、幾天後處理都沒關係，但沒辦法表達。
- 結果：IRQ 子系統總是把中斷當高優先權，**無條件搶占任務**，對即時任務很不利。
- Top half / bottom half：bottom half 在 Linux 叫 **softirq**（現代 Linux 還有 workqueue、tasklet 等）。bottom half 由主要中斷（計時器、磁碟、網路）啟動，排進佇列稍後執行，但**沒有即時保證**：仍以 kernel 優先權執行，會佔用即時任務的資源、延遲它們。（第 20 支影片筆記的 `/proc/softirqs` 實測。）

### 5.2 鎖

[18:01]–[21:04]

- Spin lock 本質上不可搶占：拿著 spin lock 的程序被搶占 → 等這把鎖的程序全都動不了。
- 老師：「Linux 在這方面沒有 priority inheritance」→ 嚴重問題。kernel 裡 critical section 很多，拿著鎖時 kernel 被搶占，會讓很多任務錯過 deadline。
- 所以老師說：中斷的 top half **不允許有 critical section**，否則可能永遠等下去。
- 要做大量 kernel 測試，找出延遲最大的程式路徑，改寫 critical section、盡量少用鎖——非常費工。

**Linux kernel 鎖的發展**【口頭】：

```
完全不可搶占 → critical section 之外可搶占（critical section 仍是 spin lock）
→ 排程器改進 → 即時變體：kernel critical section 也可搶占、中斷有優先權、priority inheritance、高解析度計時器
```

⚠️ 老師說「Linux 有一個叫 **Zephyr** 的、相當即時的版本」[20:30]。Zephyr 是 Linux Foundation 主持的**另一個獨立 RTOS 專案**（給微控制器用），不是 Linux 的版本；老師在 [34:34] 列清單時也說它是「Linux Foundation 的產品之一」。

⚠️ 老師說 top half 不存取 critical section [19:00]、[26:00]。在 v6.2.12 裡，中斷處理常式**會**拿 spin lock（驅動常見 `spin_lock()` 配合行程上下文的 `spin_lock_irqsave()`，讓同一把鎖在中斷和行程間互斥，且持鎖期間關本地中斷，避免死結）。「top half 盡量短、少做事」是對的，但「完全沒有 critical section」並不成立。

⚠️ 「Linux 沒有 priority inheritance」：一般 kernel 的 spin lock 確實沒有（持鎖時本來就禁止搶占）；但 Linux 有 PI 的 rt_mutex（第 11 支影片），PREEMPT_RT 把 spin lock 改成以 rt_mutex 實作，就有了 PI。

其他阻礙【口頭】：Linux 不像 Windows 那麼普及 → 標準、驅動、認證測試的問題。老師說這是 10–20 年前比較嚴重的問題，現在好很多。

---

## 6. 讓 Linux 即時的兩條路

[21:31]–[34:01]

元兇是**中斷**和**搶占**，兩者都要處理。

### 6.1 改 kernel 本身

[22:00]–[30:30]

- 重構 kernel，讓每個呼叫（中斷、系統呼叫）都有**有上限的延遲**；主線 kernel 的任何做法也都必須保持可搶占、延遲有上限。
- 因為改主線太複雜，出現了分支：**Linux RT（PREEMPT_RT patch）**、**MontaVista** 等。
- 主要策略：**縮短不可搶占程式碼的長度**。說來容易做來難：要彈性地搶占，程式碼變複雜；但仔細設計、做足研究，可以讓 OS 的不可搶占區段極小。

**Spin lock 與中斷**（老師的整理）：

- 鎖的目的：防止程序上下文之間（其他程序或中斷）的並行存取。critical section 小時 spin lock 很好；但持鎖者被搶占，大家都卡住。
- 中斷處理常式也要存取 critical section 時會造成延遲 → 老師說 Linux 設計時決定 top half 不存取 critical section、並**關中斷**讓 top half 原子地執行。
- Bottom half 由 top half 排程、稍後執行，做大部分處理。**即時關鍵的中斷不應該用 bottom half**：可能被排到很後面、受其他中斷影響；最好足跡最小、盡量全在 top half 做完。能做到多少要看應用。
- 目前中斷處理常式由 CPU 的中斷機制**直接**執行（查中斷表、呼叫），不在 Linux 排程器控制下 → 中斷處理常式實際上是最高優先權，能強迫任何高優先權即時任務 context switch。真正的 RTOS 不允許這樣，要讓中斷處理受軟體（或可程式化硬體）控制。

（Linux 的 threaded IRQ 正是把中斷處理放到排程器控制下：第 17、20 支影片筆記。）

### 6.2 Interrupt abstraction：在 kernel 下面加一層

[22:30]–[34:01]

```
   Linux kernel（當成低優先權的一個任務，不修改）│ 即時任務
   ─────────────────────────────────────────────────────────
   薄薄的即時層（hypervisor / 即時 kernel）：攔截所有中斷、強制優先順序、即時排程
   ─────────────────────────────────────────────────────────
   Hardware
```

- 在 kernel 和硬體之間加一層**虛擬化層**：處理所有即時需求；就算不改 kernel，外界看起來也是即時的。
- 這層接收所有硬體中斷、執行處理常式、必要時把中斷轉交 kernel、需要時才跑 kernel。kernel 變成它排程的**一個低優先權應用**（以前 kernel 是最高優先權）。
- 老師：hypervisor 和 virtual machine manager 指同一件事。
- 代表：**RTLinux**（曾經很流行，現已停止）、**RTAI**、**Xenomai**（仍以某種形式存在）。
- 兩種形式：kernel 下面的虛擬機器；或在 kernel 裡載入 **loadable module**（即時任務與 API），重新對應中斷表、強制中斷優先順序、決定每個中斷怎麼處理。

**限制**：

- 即時應用越來越複雜，需要**虛擬記憶體**（但虛擬記憶體又帶來效能問題）。
- 有效的資源管理至少要在 Linux 上實作類似 **PCP** 的協定。
- 沒有虛擬記憶體時的**程序隔離**是問題，要另有類似虛擬記憶體或 hypervisor 的機制。

### 6.3 Kernel latency 的組成

[23:32]–[25:00]

```
 中斷發生
 │←─ interrupt latency ─→│←─ handler duration ─→│←─ scheduler latency ─→│←─ scheduler duration ─→│ 任務開始跑
 │←──────────────────────────── kernel latency ───────────────────────────────────────────→│
```

- 中斷延遲 → 處理常式執行時間 → 喚醒的任務變成可執行、叫起排程器的延遲 → 排程器執行時間。
- 要縮短處理中斷和叫起排程器的時間；越短，程式反應越快。

---

## 7. 【實測】一般 Linux 的喚醒延遲

仿 `cyclictest`：每 1 ms 用絕對時間睡一次，量「預定醒來時間」到「實際醒來」的差（一般 SCHED_OTHER 任務、綁在 CPU 3，各跑 3000 次）：

```c
next.tv_nsec += 1000000; ...
clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, 0);
clock_gettime(CLOCK_MONOTONIC, &now);
long l = (now.tv_sec - next.tv_sec) * 1000000000L + now.tv_nsec - next.tv_nsec;
```

```
idle                         avg  167.9 us  worst    250.8 us  >100us: 2991/3000
3 CPU hogs on same CPU       avg  152.4 us  worst  23338.6 us  >100us: 25/3000
find /usr (syscall-heavy)    avg   56.6 us  worst     93.6 us  >100us: 0/3000
```

輔助資訊：`/proc/self/timerslack_ns` = 50000；CPU 3 的 idle 狀態 POLL / C1 / C2 / C3，退出延遲 0 / 1 / 127 / 1048 μs。

看到什麼：

- **閒置時反而最慢**（平均 168 μs）：一般任務有 50 μs 的 **timer slack**（kernel 會把計時器延後最多 50 μs 以合併喚醒、省電），加上 CPU 進入深度 idle 狀態（C3 退出要約 1 ms）。即時系統常要關掉深度 C-state、用 SCHED_FIFO（RT 任務 timer slack 為 0）。
- **同一顆 CPU 上有 3 個 CPU 密集任務**：平均還好，但**最壞 23 ms**——CFS 讓大家公平分 CPU，一般任務醒來可能要等別人用完時間片。這就是「沒有即時優先權」的代價。
- **系統呼叫密集（find）**：CPU 一直醒著，所以平均只有 57 μs、最壞 94 μs；本機 `PREEMPT_VOLUNTARY` 的 preemption point 讓長系統呼叫不至於把延遲拉長。
- 要得到有上限的延遲：SCHED_FIFO ＋ mlock ＋ 關深度 C-state ＋（必要時）PREEMPT_RT——正是本講與前幾講的各項要求。

---

## 8. 2021–2022 年常見的 RTOS

[34:01]–[36:00]

老師說清單取自 LynxOS 的網站：

| RTOS | 老師的說明 |
|---|---|
| **Zephyr** | Linux Foundation 的專案 |
| **VxWorks** | 非常流行，原由 Wind River 設計 |
| **µC/OS（Micrium）** | 微控制器 OS |
| **ThreadX（Express Logic）** | 逐字稿聽成「microsoft express logic」（ThreadX 後來被 Microsoft 收購），待補 |
| **pSOS** | 前面講過 |
| **QNX Neutrino** | BlackBerry 手機曾用，現在沒那麼多 |
| **Nucleus**（Mentor Graphics）、**MQX** | — |
| **LynxOS** | 很流行 |
| **Keil RTX** | 越來越流行，因為跑在 **ARM** 上（ARM 是小型嵌入式 IP 的市場龍頭） |
| **INTEGRITY** | — |
| **FreeRTOS** | 老師的課程作業用它；Amazon 維護，**MIT 授權**，很多附加支援與函式庫 |
| **embOS** | 很流行 |

老師鼓勵觀眾下載這些 OS 來跑跑看。

---

## 9. 結語

[36:00]–[38:30]

1. **kernel 的可搶占性很重要**：盡量減少 kernel 處於不可搶占模式的時間。
2. 和**鎖**結合時更糟：拿著鎖被搶占，對自己和等待者都很慘，spin lock 大多有這問題 → 需要 **priority inheritance**。
3. **中斷管理**：top half 傳統上關中斷 → 可搶占性問題；bottom half 被排到很後面 → 中斷回應時間變長，一跑又可能佔住 CPU 很久。
4. 要一起看這些問題：RTOS 讓 kernel 程式碼高度可搶占、不太依賴鎖（例如 **lock-free 演算法**），並加上管理中斷的硬體支援（中斷優先權等）。

---

## 10. 一頁速查表

| 做法 | 重點 |
|---|---|
| 延伸 Unix | 加排程器、計時器、檔案系統；根本問題沒解決 |
| Host-target | host 開發（cross-compiler / debugger），target 跑小 RTOS（pSOS、VxWorks、VRTX）；搶占 ~10 μs |
| Preemption point | 只在資料結構一致的點可搶占；最壞延遲 = 最長點間距；kernel 小改（HP-UX、Windows CE）；Linux：`cond_resched()`、PREEMPT_VOLUNTARY |
| Fully preemptible | 用 kernel-level lock / spin lock 保護；持 spin lock 被搶占是大問題；單處理器 spin lock = cli/sti |
| Self-host（課本） | 同機開發與執行；microkernel；即時 0–127、動態 128–254、idle 255 |
| Linux 元兇 | 中斷（延遲無上限、不可搶占、無優先權、softirq 無保證）；鎖（spin lock 不可搶占、無 PI） |
| 兩條路 | 改 kernel（PREEMPT_RT、MontaVista）；kernel 下加一層（RTLinux、RTAI、Xenomai：kernel 變低優先權任務） |
| Kernel latency | 中斷延遲 ＋ 處理常式 ＋ 排程器延遲 ＋ 排程器執行 |
| 實測 | 1145 個 cond_resched；喚醒延遲閒置 168 μs（timer slack ＋ C-state）、有 CPU hog 時最壞 23 ms |

---

## 11. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **PREEMPT_RT、RTLinux、RTAI、Xenomai 的細節** | **第 23 支**：Linux-based RTOSs |
| **pSOS、VxWorks、QNX、LynxOS 等商用 RTOS** | **第 24 支**：Commercial RTOSs |
| **Windows CE（preemption point）** | **第 25 支**：Windows CE |
| **延遲的量測方法** | **第 26 支**：Benchmarking RTOSs |

---

## 附：本講一句話總結

> **要讓 Unix 類 OS 變即時，光加即時排程器和細計時器不夠，因為 kernel 不可搶占、中斷與 I/O 不是即時就緒的；做法有 host-target（在 Unix host 上用 cross-compiler 與 cross-debugger 開發，再下載到 pSOS、VxWorks 這種搶占時間約 10 μs 的小 target）、preemption point（只在 kernel 資料結構一致的點允許搶占，最壞延遲是最長的點間距——Linux 桌機 kernel 的 PREEMPT_VOLUNTARY 正是 1145 個 cond_resched()）、以及用 kernel-level lock 與 spin lock 保護的 fully preemptible kernel，課本另有在同一台機器開發、以 microkernel 加上 0–127 靜態即時優先權的 self-host 做法。大家都拿 Linux 來改，因為它開源、模組化、可擴展、開發者多，但它的兩個元兇是延遲無上限、不可搶占、沒有優先權的中斷（含沒有保證的 softirq）和拿著時不可搶占、又沒有 priority inheritance 的 spin lock；解法一是改 kernel 讓不可搶占區段極小（PREEMPT_RT、MontaVista），解法二是在 kernel 下加一層薄薄的即時層攔截所有中斷、把 Linux 當成低優先權任務跑（RTLinux、RTAI、Xenomai）。實測一般 Linux 的 1 ms 週期任務閒置時因 timer slack 與深度 C-state 平均晚 168 μs，同 CPU 有 CPU 密集任務時最壞晚 23 ms，說明為什麼需要 SCHED_FIFO 和這些 kernel 改造。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 23 錄影逐字稿（YouTube 自動字幕，38:50）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch5 §4 整理。原始碼為 Linux v6.2.12；實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
