# Real Time Systems 第十八支影片完整筆記：硬體、計時器與作業系統（Lecture 19）

> **課程**：Real Time Systems — Lecture 19：Hardware, Timers, and OSes（播放清單第 18 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（35:40，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch5 §1（PDF p.108–111）
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
- [1. Flash memory：RTOS 放在哪裡](#1-flash-memoryrtos-放在哪裡)
- [2. HAL 與 board support package](#2-hal-與-board-support-package)
- [3. OS 的光譜](#3-os-的光譜)
- [4. 時鐘中斷：jitter 的大來源](#4-時鐘中斷jitter-的大來源)
- [5. 時鐘解析度、軟體時鐘、硬體 timestamp counter](#5-時鐘解析度軟體時鐘硬體-timestamp-counter)
- [6. 【原始碼】【實測】Linux 的「粗」時鐘與「細」時鐘](#6-原始碼實測linux-的粗時鐘與細時鐘)
- [7. 計時器：週期與一次性、watchdog](#7-計時器週期與一次性watchdog)
- [8. Unix 為什麼不適合即時](#8-unix-為什麼不適合即時)
- [9. Linux](#9-linux)
- [10. Monolithic vs microkernel](#10-monolithic-vs-microkernel)
- [11. 一頁速查表](#11-一頁速查表)
- [12. 與後續講次的連結](#12-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

1. 硬體：RTOS kernel 放在 **flash**，不是硬碟；flash 的讀寫特性。
2. **HAL / BSP**：把硬體抽象掉，讓 RTOS 可移植。
3. **時鐘與計時器**：時鐘中斷做什麼、解析度的取捨、軟體時鐘 vs 硬體 timestamp counter、週期 / 一次性計時器、watchdog。
4. **Unix、Linux** 的結構與即時特性；monolithic vs microkernel。

---

## 1. Flash memory：RTOS 放在哪裡

[00:00]–[09:01]

- 很多嵌入式裝置的 RTOS kernel **不放在硬碟**：硬碟不提供好的即時保證（上一講）→ 放在 **flash memory**。
- Flash 讀取不慢，但不想太常存取，所以常搭配一些 **DRAM**，否則記憶體存取時間會太高。

**Flash 的特性**：

| 特性 | 內容 |
|---|---|
| 持久性 | 斷電資料還在（DRAM 斷電就沒了）。以前叫 **EEPROM**（老師：可程式化就不是唯讀了，但歷史名稱就這樣） |
| 讀 | 以 **page** 為單位（可達 1 KB），不能讀 4、8 bytes 這種小單位；速度是奈秒等級，和 DRAM 差不多 |
| 寫 | 以 page 為單位，但**寫過不能重寫** |
| 改寫 | 要把整個 **block** 抹除。做法：把半寫的 block 搬到新的空 block，修改的 page 寫到新位置，更新最上層的**目錄**（邏輯位址 → 新位置），再抹除舊 block |
| 壽命 | 有最大的**寫入 / 抹除次數**，之後會磨損 |

→ **寫入很慢**。所以 flash 前面要放 DRAM，讓寫入有一定的確定性；對即時系統來說這才是重點。

⚠️ 老師說抹除是「把整個 block 填成 0」[02:32]。NAND / NOR flash 抹除後的狀態通常定義為全 **1**，寫入（program）才把位元變成 0。（講義沒有這段，無法對照。）

**Floating gate transistor**：

- 有兩個閘極：**control gate** 和 **floating gate**；floating gate 存電荷。
- 在 control gate 加高電壓（**5–15 V**），把電子吸進 floating gate 並困住 → cell 被「程式化」；加負電壓把電子推回通道。
- 老師稱這為 **avalanche injection**：高能電子穿過絕緣層進入 floating gate。
- Cell 有沒有存電荷，會改變它的電流–電壓特性 → 可以讀出。
- 排成陣列時，control gate 由 **word line** 控制。
- **Multi-level flash**：存不同數量的電子，一個 cell 存 2 bits。

---

## 2. HAL 與 board support package

[09:01]–[12:04]

```
   Applications
   ─────────────
   RTOS kernel（如 Wind River 的 kernel）
   ─────────────
   HAL / BSP（board support package）
   ─────────────
   Hardware（硬碟、flash……）
```

- **HAL（hardware abstraction layer）** 把硬體抽象給 kernel。
- **BSP**：OS 和硬體之間的邏輯層。
  - 通常放在 **firmware / ROM**。
  - 包含裝置驅動程式、**boot loader**（載入 OS）。
  - 包含 **built-in self-test**：偵測、回報硬體錯誤（第 2 支影片的 BIST）。
  - 也可以把時間、jitter 的限制嵌在這層，讓上層軟體知道最壞情況的數字。
- 硬體不同，但只要介面標準，RTOS kernel 就容易和它溝通 → **軟硬體解耦**，延長軟體壽命、提高可移植性。VxWorks、QNX 等主流 RTOS 都這樣寫。

---

## 3. OS 的光譜

[12:04]–[13:01]

```
硬即時 ←────────────────────────────────────→ 通用
QNX、VxWorks、Lynx、RT Linux、Windows CE      Linux、Windows、Unix（頂多軟即時）
```

---

## 4. 時鐘中斷：jitter 的大來源

[13:01]–[16:00]（課本 §1.1，p.109–110）

> 很大一部分 jitter 來自**時鐘中斷和它的處理**。

時鐘中斷時 kernel 做的事：

| 工作 | 內容 |
|---|---|
| 處理計時器事件 | 檢查**計時器佇列**（依到期時間排序），到期的就執行（或把對應的 handler 放進 ready queue）——像鬧鐘 |
| 更新執行預算 | 例如距上次中斷 5 ms，正在跑的任務各加 5 ms。若任務沒做完但時間片用完（同優先權用 round robin，例如 100 ms）→ 搶占，換同等級的下一個 |
| 更新 ready list | 等待佇列裡的任務若條件已滿足 → 放進 ready queue；若比目前的任務優先權高 → 搶占 |
| 更新目前時間 | 軟體時鐘 |
| 順便做 kernel 自己的事 | kernel 會趁機做一些一直想做卻沒機會做的工作 → 不確定性 |

課本 Fig.1 的計時器佇列：

```
... │ t4=15 │ t3=3 │ t2=3 │ t1=1 │  ← 依到期時間排序
                      ↓      ↓
                  Handler2 Handler1
```

（老師順帶複習：4 顆處理器、6 個同為最高優先權的任務時，用 100 ms 時間片輪流。）

---

## 5. 時鐘解析度、軟體時鐘、硬體 timestamp counter

[16:00]–[19:01]（課本 §1、§1.2，p.108–110）

- **時鐘解析度（clock resolution）**：時鐘提供的時間粒度（課本註腳：兩個 clock tick 之間的時間）。可以是奈秒、數十微秒、毫秒。
- 硬體時鐘之外，kernel 還維護一個**軟體時鐘**（system clock）：每次硬體時鐘中斷（例如每 1 ms 或 10 ms，Linux 叫 **jiffy**）更新一次。
- 任務讀時間不必讀硬體暫存器，讀一個記憶體位址就好；POSIX 函式 `clock_gettime()`（Linux 支援，部分 Windows 版本也支援）。

**取捨**：

- 解析度越細 → 硬體時鐘中斷越頻繁 → 處理中斷的處理器時間越多。
- `clock_gettime()` 本身的回應時間不確定。老師：軟體時鐘是毫秒粒度時，幾百奈秒的變動無所謂。

【課本補充】（p.109–110）

- 硬體時鐘的解析度已經比奈秒還細（處理器 > 3 GHz），但 RTOS 提供給程式的解析度常只有**數毫秒或更差**。
- 原因一：中斷處理開銷隨解析度變細而增加。
- 原因二：`clock_gettime()` 系統呼叫有 **jitter**（中斷優先權比系統呼叫高、很多 OS 處理系統呼叫時關中斷），商用 OS 中可達**數毫秒**。比這個誤差更細的解析度沒有意義。

**硬體 timestamp counter**：

- 很多處理器提供硬體計數器，**映射到即時任務的位址空間**：不用系統呼叫、不經過 OS，直接讀一個記憶體位址就得到時間。
- 例（課本）：Pentium 的 **time stamp counter**，開機時從 0 開始，每個硬體時鐘週期加一。
- 缺點：**可移植性**差。換硬體（或軟硬體組合）可能沒有這個計數器、位址和解析度也不同。

---

## 6. 【原始碼】【實測】Linux 的「粗」時鐘與「細」時鐘

Linux 同時提供兩種，正好對應本講的「軟體時鐘」和「硬體計數器」：

- `CLOCK_*_COARSE`：就是 jiffy 粒度的**軟體時鐘**。解析度直接回傳一個 tick 的長度：

  `kernel/time/posix-timers.c:234`

  ```c
  static int posix_get_coarse_res(const clockid_t which_clock, struct timespec64 *tp)
  {
  	*tp = ktime_to_timespec64(KTIME_LOW_RES);
  	return 0;
  }
  ```

  `include/vdso/ktime.h:13`

  ```c
  #define LOW_RES_NSEC		TICK_NSEC
  #define KTIME_LOW_RES		(LOW_RES_NSEC)
  ```

- `CLOCK_MONOTONIC` / `CLOCK_REALTIME`：讀硬體計數器（x86 上是 TSC）再換算，奈秒解析度。
- 兩者都透過 **vDSO**：kernel 把時間資料和讀取程式碼映射到每個程序的位址空間——就是課本說的「把硬體時鐘映射到應用程式的位址空間」，但由 OS 包好，解決了可移植性問題。

【實測】

```
CLOCK_REALTIME_COARSE    getres=  4000000 ns  smallest observed step=  3999703 ns  read cost=  3.7 ns
CLOCK_MONOTONIC_COARSE   getres=  4000000 ns  smallest observed step=  3999702 ns  read cost=  3.7 ns
CLOCK_MONOTONIC          getres=        1 ns  smallest observed step=       14 ns  read cost= 10.8 ns
rdtsc (user-mode TSC read) cost = 15.0 cycles
```

`strace -c ./clocks`：整個程式呼叫了幾百萬次 `clock_gettime`，但 strace 統計裡**沒有** `clock_gettime` 系統呼叫（總共只有 78 個系統呼叫，都是載入程式用的）。

看到什麼：

- COARSE 時鐘一次跳 **4 ms**（本機 `CONFIG_HZ=250`，第 5 支影片筆記），就是 jiffy。讀它最便宜（3.7 ns：只讀記憶體）。
- 細時鐘實際觀察到 **14 ns** 的步進（大約就是讀一次的成本），讀一次 10.8 ns（要讀 TSC、做乘法換算）。
- 直接用 `rdtsc` 指令讀 TSC 只要 15 個 cycle。
- 全部都**不用系統呼叫**（vDSO）。課本寫的「`clock_gettime()` 系統呼叫有數毫秒 jitter」在現代 Linux 上已經不是這樣；剩下的 jitter 主要來自中斷、搶占（第 9、17 支影片）。

---

## 7. 計時器：週期與一次性、watchdog

[19:01]–[22:30]（課本 §1.3，p.110–111）

| 種類 | 用途 | 實作 |
|---|---|---|
| **週期計時器（periodic）** | 定期取樣、定期做事；把週期任務送進系統（RMS 的任務） | 設定後週期性到期；到期時呼叫 handler，再把計時器資料結構**放回**計時器佇列 |
| **一次性計時器（aperiodic / one-shot）** | 只到期一次；非週期任務、監看任務的 deadline | — |

- 週期計時器也能當鬧鐘：到期時檢查任務做完沒，沒做完 → 錯過 deadline（d = p 的情況）。
- 課本例：週期 100 ms 的計時器，handler 每 100 ms 讀一次溫度感測器。

**Watchdog timer**（一次性計時器的典型用途）：

```c
f(){
    wd_start(t1, exception-handler);   ← start：t1 後到期
    ...
    wd_tickle();                       ← end：做完了，重設 watchdog
}
```

- 任務開始時啟動 watchdog；在 deadline 前做完 → `wd_tickle()` 重設，不會響。
- 沒在 t₁ 內做完 → watchdog 到期 → 系統知道錯過 deadline → 執行例外處理。

（Linux 對應：一次性 / 週期計時器可以用 `timer_create()`、`timerfd`（第 3、5 支影片實測）；kernel 內部是 hrtimer。）

---

## 8. Unix 為什麼不適合即時

[22:30]–[26:30]

- Unix：60 年代末、70 年代初；Bell Labs 和 Berkeley 都有參與。GUI 很晚才有（老師說 1985 年 X Window System 才來）。
- 原本的 Unix 著重**重負載、高吞吐量**，是批次處理系統：公平、進展、資源共享——不是為即時系統設計的 → 用來做即時系統是「真正的不搭」。

**結構**：硬體 → 程序控制（記憶體管理、排程、IPC）＋ 檔案系統、裝置驅動 → 系統呼叫介面 → 使用者程式。

**資料結構**：記憶體表（page table）、I/O 表、檔案表、程序表；每個程序有 process image（指向使用者資料、程式、堆疊）。每個使用者模式執行緒有一個在 kernel 模式的「雙胞胎」：平常睡著，系統呼叫時醒來替它做事。

**系統呼叫**：程序控制、檔案系統、其他特殊用途（OS 課的內容，老師刻意不深入）。

**優先權**：

- 系統程序最高：磁碟 I/O、swapper、檔案程序……然後是較低階的 kernel 程序。
- 使用者執行緒有一組優先權，由**使用者**和**超級使用者（root）**設定；root 可以在使用者優先權範圍內調高。

（課本 Ch5 §3 對 Unix 不適合即時的原因有更詳細的分析，下一支影片筆記一起整理。）

---

## 9. Linux

[26:30]–[33:00]

**歷史**：

- 90 年代 PC 出現，但 Unix 不是 PC 的 OS → 需要一個類 Unix 的 PC OS。
- **1991** 年 Linus Torvalds 參考 Tanenbaum 寫的玩具 OS **Minix**，做得更像 Unix、能在 PC 上跑。
- 90 年代中網際網路已經出現 → 程式碼能分享，大量開發者和公司加入，變成群眾運動。
- 1994 年加入 TCP/IP、SCSI 磁碟控制器的檔案系統支援 → 90 年代中期已是能用的系統。

⚠️ 老師說「Linux 一直都有 GUI」[23:00]。Linux kernel 本身沒有 GUI；圖形介面是另外的 X Window 實作（後來的 Wayland）跑在使用者空間。待補：講義沒有這段。

**Monolithic kernel**：

- 所有 kernel 子系統是**一整塊程式碼**，彼此之間沒有保護、沒有邊界。
- 兩種模組化：
  1. **編譯時**：大量設定選項（自己編過 kernel 就知道）。
  2. **執行時**：動態載入 **kernel module**（像 DLL / 共享函式庫，載入 kernel 的執行空間），例如裝置驅動、額外的檔案系統。→ 可以有最小化、能適應機器的 kernel，讓 monolithic kernel 不至於太大。
- 系統呼叫進 kernel → kernel 自己處理，或交給實作它的 module（必要時載入）→ 回傳結果。

**Real-time Linux**【口頭】：從一般 Linux 分支出來；kernel 要特別編譯，用一個 `rt load` 之類的指令啟動 kernel 裡的即時功能（部分以 module 形式存在）；啟動後即時功能接手，一般 Linux kernel 的功能用得很少。（第 23 支影片會詳談。）

**排程**：

- Round robin ＋ 可搶占：每個任務一個時間片（例如 100 ms），用完換同優先權的下一個。
- 老師：優先權會**動態調整**，用計算出的 CPU 使用因子加上任務的基本優先權。
- 即時任務基本優先權 **0–99**，使用者程序 **100–139**。

⚠️ 「依 CPU 使用因子動態調整優先權」是較舊 Linux 排程器（2.4 / O(1)）的做法。v6.2.12 的一般任務用 **CFS**：不調優先權數值，而是依 nice 權重累積 vruntime、挑最小的（OS 課 `../linux-os/` 第 26 講）。0–99 / 100–139 的範圍則和 `include/linux/sched/prio.h` 一致（`MAX_RT_PRIO = 100`、`MAX_PRIO = 140`，第 9 支影片筆記）。

**進入 kernel 的方式**：

| 種類 | 來源 |
|---|---|
| 同步 | 程序要 kernel 替它做事：**系統呼叫**（送一個「假中斷」，載入系統呼叫處理程式）；或程序的異常動作，例如**硬體例外** |
| 非同步 | **中斷**：I/O 裝置、或以 I/O 裝置形式出現的即時時鐘；獨立於程序發生 |

---

## 10. Monolithic vs microkernel

[33:00]–[35:40]

```
Monolithic（Linux）                      Microkernel
┌──────────────────────────┐            ┌─────────────────────────────────────────┐
│ Applications（user mode）│            │ Apps │ Device mgr │ Memory mgr │ Net mgr │ ← 全在 user mode
├──────────────────────────┤            ├─────────────────────────────────────────┤
│ 記憶體管理 程序管理 檔案系統│            │          小小的 kernel                   │
│ 驅動（裝置、網路、圖形、終端）│            ├─────────────────────────────────────────┤
│ Hardware interface layer │            │               Hardware                  │
├──────────────────────────┤            └─────────────────────────────────────────┘
│ Hardware                 │
└──────────────────────────┘
```

- Monolithic：kernel 很大、自己的工作很多 → 即時保證變得比較難。
- Microkernel：kernel 盡量小，原本在 kernel 模式的很多東西改在**使用者模式**跑。
  - 好處：原本的 kernel 程序和使用者程序優先權差不多 → 更容易給即時應用額外的優先。
  - 挑戰：安全性等。
- Windows 不是純 microkernel，但比 Linux 更接近。
- 下兩支影片詳談。

---

## 11. 一頁速查表

| 主題 | 重點 |
|---|---|
| Flash | 持久；page 讀寫、寫過不能重寫、要抹整個 block；寫慢、有壽命；前面放 DRAM；floating gate 存電荷 |
| HAL / BSP | 抽象硬體；在 firmware；驅動、boot loader、BIST；可移植 |
| 時鐘中斷 | 處理計時器佇列、更新預算（時間片）、更新 ready list、更新軟體時鐘；kernel 順便做事 → jitter |
| 解析度取捨 | 越細中斷越多；課本：RTOS 常只有數毫秒 |
| 硬體計數器 | 映射到位址空間、免系統呼叫；可移植性差（Pentium TSC） |
| Linux 時鐘 | COARSE = jiffy（本機 4 ms，讀 3.7 ns）；MONOTONIC 讀 TSC（14 ns 步進，讀 10.8 ns）；都走 vDSO，無系統呼叫 |
| 計時器 | 週期（取樣、送入週期任務）／一次性（deadline 監看）；watchdog：wd_start / wd_tickle |
| Unix | 為吞吐量設計；系統程序優先權最高 |
| Linux | 1991、Minix；monolithic ＋ module；RT 0–99、一般 100–139（v6.2.12 一般任務用 CFS） |
| Microkernel | 服務移到 user mode；kernel 小，較易給即時優先 |

---

## 12. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **Monolithic vs microkernel** | **第 19 支**：Monolithic and Micro-kernels |
| **Windows 的 DPC、優先權** | **第 20 支**：Windows NT as an RTOS |
| **POSIX 時鐘與計時器** | **第 21 支**：POSIX standards |
| **QNX、VxWorks、Lynx、Windows CE、RT Linux** | **第 22–25 支** |
| **時鐘中斷延遲、計時器精度的量測** | **第 26 支**：Benchmarking RTOSs |

---

## 附：本講一句話總結

> **嵌入式 RTOS 的 kernel 放在 flash 而不是硬碟——flash 斷電不丟資料、讀取很快，但以 page 寫入、寫過不能重寫、要整塊抹除而且有壽命，所以寫入很慢、前面要放 DRAM；HAL 與放在 firmware 的 board support package（驅動、boot loader、自我測試）把硬體抽象掉，讓 VxWorks、QNX 這類 RTOS 可以移植。很大一部分 jitter 來自時鐘中斷：kernel 要處理計時器佇列、扣時間片、更新 ready list 和軟體時鐘，還會順便做自己的事；時鐘解析度越細中斷開銷越大，所以 RTOS 常只提供毫秒級的軟體時鐘，要更細就直接讀映射到位址空間的硬體 timestamp counter，代價是可移植性。計時器分週期（取樣、送入週期任務）和一次性（例如 watchdog：開始時 wd_start、做完 wd_tickle，沒做完就觸發例外處理）。Unix 為吞吐量而生、不適合即時；Linux 從 1991 年的 Minix 啟發而來，是可載入 module 的 monolithic kernel，即時優先權 0–99、一般 100–139；microkernel 則把服務搬到使用者模式，更容易給即時任務優先。實測 Linux 的 COARSE 時鐘正是 4 ms 一跳的 jiffy，細時鐘讀 TSC 達到 14 ns 步進，而且兩者都經由 vDSO、完全不用系統呼叫。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 19 錄影逐字稿（YouTube 自動字幕，35:40）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch5 §1 整理。原始碼為 Linux v6.2.12；實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
