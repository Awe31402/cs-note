# Real Time Systems 第二十支影片完整筆記：Windows NT 當 RTOS（Lecture 21）

> **課程**：Real Time Systems — Lecture 21：Windows NT as an RTOS（播放清單第 20 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（44:12，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch5 §3、§5（PDF p.113–117、121–124）
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
- [1. 複習：Unix 的兩大問題](#1-複習unix-的兩大問題)
  - [1.1 不可搶占（non-reentrant）kernel](#11-不可搶占non-reentrantkernel)
  - [1.2 動態優先權](#12-動態優先權)
  - [1.3 Unix 的其他不足](#13-unix-的其他不足)
- [2. Windows 的演進](#2-windows-的演進)
- [3. Windows NT 的架構](#3-windows-nt-的架構)
  - [3.1 三層：microkernel、executive、subsystems](#31-三層microkernelexecutivesubsystems)
  - [3.2 Kernel mode 的元件](#32-kernel-mode-的元件)
  - [3.3 Environment subsystem（personality）](#33-environment-subsystempersonality)
  - [3.4 可移植性](#34-可移植性)
  - [3.5 Windows 是 microkernel 嗎](#35-windows-是-microkernel-嗎)
- [4. Windows NT 的優先權](#4-windows-nt-的優先權)
- [5. 中斷處理與 DPC：最大的問題](#5-中斷處理與-dpc最大的問題)
- [6. Windows NT 的缺點總結](#6-windows-nt-的缺點總結)
- [7. 【課本補充】Windows NT vs Unix](#7-課本補充windows-nt-vs-unix)
- [8. 【實測】Linux 的 IRQ 執行緒：DPC 問題的另一種解法](#8-實測linux-的-irq-執行緒dpc-問題的另一種解法)
- [9. 一頁速查表](#9-一頁速查表)
- [10. 與後續講次的連結](#10-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

把傳統 OS 拿來當 RTOS 會出什麼問題？

1. 複習 Unix：不可搶占的 kernel、動態優先權。
2. Windows NT：layered microkernel 架構、32 個優先權（有即時類別）。
3. 最大問題：**DPC**（中斷的 bottom half）全部以同一個、比所有使用者執行緒都高的優先權 FIFO 執行 → 即時執行緒會被不重要的 DPC 擋住。
4. 也沒有 priority inheritance / PCP。

---

## 1. 複習：Unix 的兩大問題

### 1.1 不可搶占（non-reentrant）kernel

[00:32]–[06:01]

- 一旦進 kernel 就跑很久，使用者任務完全被擋住，很難保證即時。
- 使用者程式做系統呼叫時，對應的 kernel 執行緒（一對一）替它執行 → 使用者程式本身也會佔用 kernel 時間。
- **不可搶占 kernel = non-reentrant kernel**：進去了就待到做完才出來。
- 做法：在 kernel mode 時**關掉所有中斷**，中途不會有東西插進來，順便保護 kernel 資料結構。
- 這只是**單處理器**的解法：多處理器上，其他 CPU 的 kernel 執行緒仍可能修改 kernel 狀態。

**Reentrant kernel**：

```
process（user mode）→ exception → kernel ─┐
                                          │ interrupt → 處理中 → 更高優先權的 interrupt → 處理完 → 回到前一個
                                          └→ 回到 process（或換別的 process）
```

- 很多 kernel 常式本身可被搶占 → 同時可能有好幾個程序在 kernel mode；kernel 可以暫停目前的 kernel 執行緒、之後再恢復。
- 中斷處理和使用者任務都有 deadline 時需要這種 reentrancy：例如希望某個使用者任務**比中斷處理更優先**——我們的即時系統是靠排程來達成的。
- 不可搶占時：**任務搶占時間 = 在 kernel mode 花的時間 + context switch 時間**，可能很長；而 kernel 常做和目前任務無關的事（垃圾回收、記帳）。

### 1.2 動態優先權

[06:01]–[12:00]

（和上一支影片筆記 §6 的課本內容相同，這裡只記老師的重點。）

- 多層回饋佇列；kernel 優先權在上，其中 disk I/O、buffer、I/O 密集的較高。
- pri(Tᵢ, j) = Base(Tᵢ) + CPU(Tᵢ, j) + nice(Tᵢ)，數字越大優先權越低；加正的 nice = 對別人好。
- CPU(Tᵢ, j) 是過去 CPU 使用率的**幾何加權**：最近一次權重最大，往前每次減半。
- Base 把任務分成優先權帶：swapper、block I/O、檔案操作、character I/O、kernel 程序、使用者。
- I/O 是瓶頸 → 提高 I/O 密集任務的優先權讓 I/O 頻道保持忙碌 → I/O bound 任務往高優先權移。
- 後果：**主要在計算的即時任務**會吃虧，可能錯過 deadline。

⚠️ 時間片：老師說 Unix 預設 **100 ms** [06:31]；課本 p.115 說 Unix 通常用 **1 秒**的時間片（可在 system generation 時設定）。

結論：Unix V 大多不適合當 RTOS；硬要用，只適合軟即時，不適合毫秒、微秒級 deadline 的應用。

### 1.3 Unix 的其他不足

[12:00]–[13:00]

任務搶占時間長（kernel 開銷）、不支援動態優先權計算（指 EDF 這種由程式決定的動態優先權）、資源共享沒處理好（沒有 PCP，高優先權任務可能等持有資源的低優先權任務）、中斷處理不可搶占、計時器不足、沒有即時檔案系統。

---

## 2. Windows 的演進

[13:00]–[14:31]

- Windows 一開始是 **microkernel** 式的設計，但現在介於 monolithic 和 microkernel 之間。
- 從 **DOS** 演進而來：DOS 很簡單、單工、分段式記憶體管理。
- 90 年代末的 **Windows NT** 之後非常穩定，支援 POSIX API、多執行緒。
- 現在是桌機、筆電上事實上的標準 OS（已到 Windows 11）。
- 要支援大量裝置，所以設計哲學是 microkernel；不過現在 OS 比以前重很多。

【課本補充】（p.121–122）DOS 在 80 年代初；80 年代末演進成 Windows 系列（主要差別是圖形前端）。DOS 系 Windows 一路加功能，程式碼結構退化到很難除錯維護 → 程式碼全部重寫成 Windows NT，穩定得多；之後的版本都是 NT 的後代，DOS 系被放棄。系譜：DOS → Windows 3.1 → 95 → 98（終止）；Windows NT → 2000 → XP。

⚠️ 課本說程式碼「在 **1998** 年完全重寫以發展 Windows NT」；老師說 NT 在「90 年代末」。課本 Fig.8 卻把 NT 畫在 Windows 98 之前就存在的另一條線上。年份待補（講義與課堂都沒有可靠的來源可對照）。

⚠️ 老師說 Windows「從 Windows CE 版本開始」[13:00]；照課本的系譜與老師後面的說法，CE 是另一條**嵌入式**產品線（老師稍後提到 CE 用 priority inheritance 處理 DPC），不是 NT 的起點。

---

## 3. Windows NT 的架構

### 3.1 三層：microkernel、executive、subsystems

[14:31]–[19:03]

```
   User programs
   ──────────────────────────────────────────── user mode
   Subsystems（Win32、POSIX…；在 user / kernel 交界）
   NT Executive（較高階的服務，kernel mode，模組獨立、可分頁、可搶占）
   NT microkernel（基本服務）
   HAL
   ──────────────────────────────────────────── hardware
```

| 層 | 內容 |
|---|---|
| **NT microkernel** | 基本服務 |
| **Executive** | 以基本服務建構更高階的服務；跑在 kernel mode；由獨立模組組成，模組**可分頁（pageable）**也**可搶占** |
| **Subsystems** | 特權子系統，是 OS 的一部分；在 user / kernel 的邊界上；32-bit Windows、POSIX 等 |

- 可搶占是好事；可分頁代表不霸佔實體記憶體，容量不夠時可以換出——這會造成即時問題，但因為模組可搶占，情況不算太糟。
- 簡化的 Windows NT 架構圖（老師）：HAL → kernel 與裝置驅動 → executive，加上**視窗與圖形子系統**；user mode 有 subsystem DLL、使用者應用、服務、系統支援程序。
  - Windows 的**圖形放在 kernel**（不在使用者空間）→ 圖形體驗好；老師說這是 Windows 長期圖形比 Linux 好的原因。
  - 很多服務和系統支援程序在 user mode → 即時使用者執行緒可以和它們競爭、甚至有更高優先權。

更細的圖（老師說取自 Silberschatz、Galvin 的 OS 課本）：

| Executive 的元件 | 說明 |
|---|---|
| I/O manager | 含儲存、網路裝置的驅動、檔案系統、檔案系統快取（彼此相關，放一起） |
| Object manager | Windows 裡**一切都是物件** |
| Security、process manager、PnP（plug-and-play）manager、virtual memory manager | — |
| **LPC（local procedure call）** | Windows 提供的低開銷 kernel 程序呼叫機制 |

User mode：POSIX、MS-DOS、OS/2、Win16 子系統；登入程序、安全子系統、認證、SAM（security account manager）資料庫；**system threads**（kernel 本身是多執行緒的）；以及一個很大的 DLL **ntdll**，所有服務與應用透過它和 kernel 介接。

### 3.2 Kernel mode 的元件

[26:02]–[31:00]

- **Executive**：以**稍低**的優先權執行（microkernel 設計的產物）。提供記憶體管理、程序與執行緒管理、安全、I/O、IPC——一般 Linux kernel 的主要功能都在 executive。
- **Kernel**：比 executive 更低階：基本的**排程、中斷處理（所有中斷的 top half）、同步**。提供一組基本常式和物件，讓 executive、kernel 程序、使用者程序、子系統建構完整的 OS。舊版 NT 都在 `ntoskrnl.exe` 裡（版本間會變）。
  - Kernel **永遠在記憶體裡、不會被換出、執行時不被搶占**；也管開機的關鍵部分與斷電後的復原。
- **驅動程式（`.sys`）**：和 executive、kernel 並列，在使用者函式呼叫和硬體呼叫之間轉譯。
- **視窗與圖形驅動 `win32k.sys`**：在 Windows 的地位很核心（不像 Linux）：視窗、UI 控制、繪圖。
- **HAL**：在 kernel 底下，很薄的一層，把 kernel 和 executive 與硬體細節隔開（主要是主機板差異）。Windows kernel 要跑在各種 x86 設備，現在也跑在 ARM 上。
- **背景系統程序**（user mode）：登入程序、session manager……
- **服務程序**：提供服務給其他程序；有些在 user mode、有些在 kernel——例如 task scheduler 是 executive 的一部分；spooler 有些版本在 user mode、有些在 executive。
- **Windows 應用**（user mode）：例如 Microsoft SQL Server、Exchange Server 提供服務給其他應用——一整個互相堆疊的生態系。

### 3.3 Environment subsystem（personality）

[23:00]–[26:02]

- **Personality（個性）**：環境子系統。可以原生使用 Windows，也可以跑為別的環境（Win16、POSIX、MS-DOS）編譯的執行檔。
- 使用者應用**不直接**呼叫 Windows 原生服務，而是呼叫特殊的 **DLL** 做轉譯：例如 MS-DOS 系統呼叫 → Windows 系統呼叫。有開銷，但換來彈性。
- 環境子系統程序管理它們世界裡的客戶程序，並**加上語意**：MS-DOS 沒有像 Windows 那樣的程序模型和安全模型，子系統要把這些紀律加上去。排程器也要把這些考慮進去，確保即時 deadline 不被違反。
- 演進：原本有 Windows、POSIX、OS/2 三個環境子系統；Windows 2000 有 Windows 和 POSIX；XP 停止部分支援，有一個加強版 POSIX 子系統（給 Unix 服務用，一些 POSIX 相容的執行檔可以跑）。

### 3.4 可移植性

[31:00]–[32:31]

- 大部分 Windows 程式碼和驅動用 **C** 寫。
- 硬體相關的程式碼幾乎都限制在 **HAL 和 kernel**（也可能有組合語言）。
- 老師的 Linux 經驗【口頭】：Linux 原始碼有每種處理器的子目錄（`arch/`），放特定的組合語言片段；上層就看不到硬體的怪癖。
- 更上層用 **C++**（例如視窗圖形子系統驅動、儲存 volume manager），應用層現在用 **C# / .NET**——上層重視可移植與可維護，不那麼重視效能和硬體連結。

### 3.5 Windows 是 microkernel 嗎

[33:00]–[35:31]

**不是純的**：

- 很多 OS 元件和驅動在獨立的私有位址空間、很多在 user mode，**但所有 kernel 元件共享位址空間**，很多情況下 OS 和驅動之間沒有保護。
- 但也不像 Linux、Unix 那麼 monolithic。

**像 microkernel 的地方**：

- OS personality（MS-DOS、OS/2——IBM 的舊 OS）以獨立程序在 user space 跑。
- 很多 kernel mode 元件**不直接**存取彼此的資料結構，而是用 **LPC** 傳參數——一種有嚴格訊息規則的訊息式系統。Windows 支援大量硬體，安全與隔離很重要。
- 所以叫 **modified microkernel**。

為什麼不做純 microkernel：效能與軟體工程的理由——位址空間分開，每次要 OS 幫忙都要 context switch 或受保護的訊息傳遞。很多商用 OS（包括 Linux）也從純 monolithic 往外移、加入 microkernel 的概念，但商業界目前沒有純 microkernel。

---

## 4. Windows NT 的優先權

[35:31]–[38:00]（課本 §5.1，p.122–123）

- **32 個優先權等級**，分**即時類別**和**動態類別**，任務**不能**在兩類之間移動。
- 動態類別的優先權會變：例如把視窗**最小化**（不再是前景任務）時，優先權就改變。

課本 Fig.9（數字越大越高）：

| 類別 | 等級 |
|---|---|
| **Real-time** | 16–31：real-time idle 16、real-time normal、real-time critical 31 |
| **Dynamic** | 1–15：dynamic idle 1、dynamic normal、dynamic critical 15 |
| **Idle** | 0 |

老師的例子【口頭】：

| 動態類別的程序 | 例子 |
|---|---|
| Idle | 沒事做 |
| Low foreground | 前景但優先權低，例如桌面上的行事曆 |
| Normal background | 看不到 GUI 的背景程序 |
| Normal foreground | 文字編輯器、瀏覽器 |
| High foreground | 遊戲、影片 |

即時類別：real-time idle（還沒跑即時執行緒，但 CPU 先保留）、real-time normal、real-time critical。實際上是一個**矩陣**：程序類別 × 執行緒相對優先權 → 1–31 的數字。例如即時程序的 idle 是 16，其他即時等級在 25、26、31 附近；high 類別在 11–14；idle 類別是 1。（逐字稿唸出的數字不完整，完整矩陣待補。）

【課本補充】（p.122）

- 每個程序屬於 idle、normal、high、real-time 之一；預設 normal。
- **normal 和 high 是可變型**：用完時間片 → 降低優先權；因 I/O 阻塞沒用完 → 提高。但和基本優先權的差距限制在 **±2**。
- 優先權驅動的可搶占排程；**即時優先權的執行緒比其他所有執行緒（包括 kernel 執行緒）都優先**。螢幕保護程式用 idle 類別。
- 計時器與時鐘解析度對大多數即時應用夠細。

---

## 5. 中斷處理與 DPC：最大的問題

[38:00]–[43:00]（課本 §5.2，p.122–123）

```
高優先權任務 ───┐ 中斷 ┌──────── 高優先權任務繼續 ────┐ 結束      ┌── DPC（bottom half）
               └─ ISR ┘（top half，在 kernel、很高優先權）          └─ 以「正常」優先權稍後執行
```

- 中斷發生 → 存機器狀態 → 執行 ISR。最關鍵的處理（**top half**）在 kernel 以很高優先權做：外部裝置在等，要正確設定它的狀態。
- 大部分工作是 **bottom half**，在 Windows 叫 **DPC（deferred procedure call）**，在 executive 以較低優先權執行。
- 理想情況：高優先權任務做完、讓出 CPU 後，DPC 才排上去 → 中斷處理有即時性。

**問題**：

- DPC 放在 **FIFO 佇列**，以**同一個優先權**處理。
- 但 DPC 的優先權應該和**引發它的程序**（中斷來源或對象）相稱。
- 結果：即使是很不重要的 DPC，也會影響即時執行緒。例：**很低優先權程序的 page fault**，處理優先權卻比即時程序還高。
- DPC 常做網路或磁碟 I/O → 即時任務的回應時間可能**無上限**。

【課本補充】（p.122–123）

- NT 中斷的優先權**永遠**高於使用者執行緒，包括即時類別。
- DPC 不能被另一個 DPC 搶占，但可以被中斷搶占。DPC 依 FIFO、以**低於硬體中斷、高於排程器 / dispatcher** 的優先權執行。
- **使用者執行緒不可能高於 ISR 或 DPC** → 就算是極低優先權任務的 ISR、DPC 也能搶占即時程序；鍵盤、滑鼠的 ISR 和 DPC 也比即時任務高。
- 有網路或磁碟 I/O 時，全系統的 DPC FIFO 佇列可能讓即時執行緒的回應時間無上限。

**Windows CE 的做法**（老師與課本）：對 DPC 用 **priority inheritance**：DPC 仍在 normal 等級，但高優先權程序的 DPC 會被提高到那個程序的優先權 → 避免 priority inversion 和無上限的阻擋。

---

## 6. Windows NT 的缺點總結

[43:00]–[44:01]

1. **中斷處理不可預測**（上一節）。
2. 沒有資源存取控制：沒有 **PCP**，連基本的 **priority inheritance** 都不支援 → 可能 unbounded priority inversion。
3. 分散式應用（多個 Windows 節點透過網路溝通）支援很少。

【課本補充】（p.123）在 user level 補資源共享：

- 最簡單：拿到不可搶占資源時，把優先權提到最高（31），釋放後還原 → 但會造成大量 inheritance-related inversion（第 11 支影片的 HLP 問題）。
- 自己實作 **PCP**：即時任務只能用**偶數**優先權（16、18、…、30）。因為 NT 不支援同等級的 FIFO 排程：若需要某資源的任務中最高優先權是 2n，資源的 ceiling 就設成 2n + 1（第 11 支影片：RR 時 ceiling 要再高一級）。Unix 有 FIFO 選項，所有等級都能用。

---

## 7. 【課本補充】Windows NT vs Unix

（課本 Table 5.1，p.124）

| 即時功能 | Windows NT | Unix V |
|---|---|---|
| DPC | 有 | 沒有 |
| 即時優先權 | 有 | 沒有 |
| 鎖定虛擬記憶體 | 有 | 有 |
| 計時器精度 | 1 ms | 10 ms |
| 非同步 I/O | 有 | 沒有 |

- NT 有很多即時 OS 需要的功能；但 DPC 的執行方式、加上沒有資源共享協定，讓它**不適合安全關鍵的硬即時應用**。
- 小心寫程式的話，NT 可以用在能容忍偶爾錯過 deadline、deadline 是幾百毫秒（而不是幾微秒）的應用；處理器使用率要夠低，priority inversion 要在 user level 處理。
- 課本：Windows 在桌機很普及，但即時應用壓倒性地用 Unix 系，因為成本和即時程式設計師的「hacker 心態」。

---

## 8. 【實測】Linux 的 IRQ 執行緒：DPC 問題的另一種解法

Windows NT 的問題是「bottom half 的優先權固定、而且高於所有使用者執行緒」。Linux 的 threaded IRQ（第 17 支影片筆記）把 bottom half 變成**一般的 SCHED_FIFO 執行緒**，可以和使用者即時執行緒放在同一個優先權尺度上比較。

本機（一般 Ubuntu kernel，非 PREEMPT_RT）：

```
$ ps -eLo cls,rtprio,comm | awk '$1=="FF"' | ...
     20  FF     99 migration
     20  FF     50 idle_inject
      1  FF     50 irq/204-AudioDSP
      1  FF     50 irq/202-nvidia
      1  FF     50 irq/201-iwlwifi:exception
      1  FF     50 irq/200-iwlwifi:queue_
      ...
```

共有 24 個 `irq/` 執行緒，全部是 **SCHED_FIFO 優先權 50**。

- 使用者的即時執行緒設成 `SCHED_FIFO` 51–99，就能**優先於**這些中斷的 bottom half——這正是 NT 做不到的「使用者執行緒高於 DPC」。也可以用 `chrt` 個別調整某個 IRQ 執行緒的優先權（相當於 Windows CE「依程序優先權決定 DPC 優先權」的手動版）。
- 不過一般 kernel 上只有部分驅動用 threaded IRQ；其他 bottom half 走 **softirq**，它的地位就很像 NT 的 DPC：

  ```
  TIMER: 1195890
  NET_RX: 138417
  BLOCK: 46262
  TASKLET: 107155
  SCHED: 4081403
  ```

  （`/proc/softirqs` 各 CPU 加總。）softirq 在中斷返回時執行，優先於所有執行緒；做太多時才交給 `ksoftirqd` 執行緒。網路收包（NET_RX）、磁碟完成（BLOCK）正是課本說的「網路或磁碟 I/O 的 DPC」。PREEMPT_RT 會把中斷強制執行緒化、softirq 也改在執行緒裡跑，才完全解決這個問題（第 23 支影片）。

---

## 9. 一頁速查表

| 主題 | 重點 |
|---|---|
| Unix 問題 | 不可搶占（non-reentrant）kernel：關中斷，單處理器解法；搶占時間 = kernel 時間 + context switch。動態優先權：I/O bound 往上、計算型即時任務吃虧 |
| Reentrant kernel | kernel 常式可被搶占，多個程序可同時在 kernel mode |
| NT 架構 | HAL → kernel（排程、中斷 top half、同步；常駐、不搶占）→ executive（可分頁、可搶占）→ subsystems / personality → user；圖形在 kernel（win32k.sys）；LPC |
| Microkernel？ | modified microkernel：kernel 元件共享位址空間，但 personality 在 user space、元件間用 LPC |
| 優先權 | 32 級：即時 16–31、動態 1–15（normal / high 可 ±2）、idle 0；即時高於 kernel 執行緒 |
| DPC 問題 | 所有 DPC 同一優先權 FIFO，高於所有使用者執行緒 → 低優先權程序的 page fault、網路 I/O 擋住即時執行緒；CE 用 priority inheritance |
| 其他缺點 | 沒有 PIP / PCP；分散式支援少 |
| NT vs Unix V | DPC、即時優先權、非同步 I/O：NT 有；計時器 1 ms vs 10 ms |
| Linux 對照 | 24 個 irq 執行緒是 SCHED_FIFO 50，使用者可設更高；softirq 仍像 DPC |

---

## 10. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **POSIX 子系統、POSIX 即時擴充** | **第 21 支**：POSIX standards |
| **Unix-based RTOS（preemption point、self-host、spin lock）** | **第 22 支**：Open Source and Commercial RTOSs |
| **Linux 的 PREEMPT_RT（強制 threaded IRQ）** | **第 23 支**：Linux-based RTOSs |
| **Windows CE** | **第 25 支**：Windows CE |
| **中斷延遲、DPC 延遲的量測** | **第 26 支**：Benchmarking RTOSs |

---

## 附：本講一句話總結

> **傳統 Unix 不適合硬即時，因為 kernel 不可搶占（關中斷跑完系統呼叫，只在單處理器上有效，搶占時間等於 kernel 時間加 context switch）、而且依 CPU 使用歷史動態調優先權，讓計算型即時任務吃虧；Windows NT 則是 modified microkernel：HAL 上有負責排程、中斷 top half 與同步的常駐 kernel，再上面是可分頁可搶占的 executive、以 LPC 溝通的元件、在使用者空間的 personality 子系統，圖形則放在 kernel 裡。NT 有 32 個優先權（即時 16–31、動態 1–15、idle 0），即時執行緒甚至高於 kernel 執行緒，還有 1 ms 計時器、記憶體鎖定和非同步 I/O；但所有中斷的 bottom half（DPC）都在同一個高於任何使用者執行緒的優先權上 FIFO 執行，低優先權程序的 page fault 或網路、磁碟 I/O 都能讓即時執行緒的回應時間無上限，再加上沒有 priority inheritance 或 PCP，所以只適合 deadline 在幾百毫秒、能容忍偶爾錯過的應用（Windows CE 對 DPC 用 priority inheritance 改善）。Linux 的做法是把 bottom half 變成 SCHED_FIFO 50 的 IRQ 執行緒（本機有 24 個），使用者即時執行緒設得更高就能優先於它們，但一般 kernel 的 softirq 仍和 DPC 一樣優先於所有執行緒。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 21 錄影逐字稿（YouTube 自動字幕，44:12）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch5 §3、§5 整理。實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
