# Real Time Systems 第十七支影片完整筆記：分散式時鐘同步與 RTOS 基本要求（Lecture 18）

> **課程**：Real Time Systems — Lecture 18：Distributed Clock Synchronization and RTOS Fundamentals（播放清單第 17 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（37:47，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch4 §6（PDF p.103–105）、Ch5 §2（PDF p.111–113）
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
- **第一部分：分散式（internal）時鐘同步**
- [1. 互相交換、取平均](#1-互相交換取平均)
- [2. 壞時鐘與 Byzantine 時鐘](#2-壞時鐘與-byzantine-時鐘)
- [3. 演算法](#3-演算法)
- [4. 定理：一個 Byzantine 時鐘最多造成 3ε/n 的差距](#4-定理一個-byzantine-時鐘最多造成-3εn-的差距)
- [5. 重新同步的間隔與「少於三分之一」](#5-重新同步的間隔與少於三分之一)
- [6. 【實測】Byzantine 平均同步模擬](#6-實測byzantine-平均同步模擬)
- **第二部分：RTOS 的基本要求**
- [7. 即時優先權等級與排程](#7-即時優先權等級與排程)
- [8. 資源共享協定](#8-資源共享協定)
- [9. 快速搶占與中斷延遲](#9-快速搶占與中斷延遲)
- [10. 【原始碼】Linux 的 top half / bottom half：threaded IRQ](#10-原始碼linux-的-top-half--bottom-halfthreaded-irq)
- [11. 記憶體管理：虛擬記憶體、記憶體保護、memory locking](#11-記憶體管理虛擬記憶體記憶體保護memory-locking)
- [12. 【實測】page fault 的 jitter 與 mlock](#12-實測page-fault-的-jitter-與-mlock)
- [13. 同步與非同步 I/O](#13-同步與非同步-io)
- [14. 一頁速查表](#14-一頁速查表)
- [15. 與後續講次的連結](#15-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

1. **Internal 時鐘同步**：沒有 master，所有時鐘互相交換讀數、取平均。要處理壞掉的時鐘和**會說謊的 Byzantine 時鐘**；只要 Byzantine 時鐘**少於三分之一**就能同步。
2. **RTOS 要什麼**：即時優先權、資源共享協定、微秒級的搶占與中斷延遲、記憶體鎖定、非同步 I/O。

---

## 1. 互相交換、取平均

[00:00]–[01:30]

- 沒有 master 時鐘、沒有大家公認的時間基準。
- 任兩個程序之間有通道；所有時鐘**定期交換讀數**。
- 每個時鐘收到 C2、C3、C4……的讀數，連同自己的，**取平均**當新時間。
- 一樣要**漸進**調整，不能讓時間倒流（上一講）。

【課本補充】（p.103）centralized 同步的主要問題是**單點故障**：master 壞了整個同步機制就壞了。分散式同步沒有這個問題。

---

## 2. 壞時鐘與 Byzantine 時鐘

[01:30]–[04:32]

| 種類 | 行為 | 處理 |
|---|---|---|
| **壞掉的時鐘** | 停了、或漂移遠超過製造商規格；不同型號 / 廠商的精度不同 | 和系統平均差超過門檻 → 判定壞掉、忽略它的值。老師：「完全停掉反而是好消息，至少你知道它壞了。」 |
| **Byzantine 時鐘** | **惡意**：同一時刻給不同時鐘**不同的值**（「兩面人」）。例：程序中了病毒 | 嚴重得多 |

- 例：C1 是 Byzantine，同時告訴 C2「我是 t − e」、告訴 C5「我是 t + e」。
- 名稱來自分散式系統的 **Byzantine generals problem**：即使有節點說謊，只要**超過三分之二**的節點沒有 Byzantine 故障，仍能達成**共識**（大家都同意的值；這裡就是時間）。
- 老師順帶區分：**fault** = 系統裡的缺陷；**failure** = 缺陷表現成錯誤的輸出（第 2 支影片）。

---

## 3. 演算法

[04:32]–[07:01]（課本 p.104）

記號：n 個時鐘，其中 m 個有 Byzantine 故障；目標是彼此差距在 **ε** 以內。

- 收到的時間和自己差超過 ε → 對方大概故障了 → **忽略**。
- 其他（差距 ≤ ε 的「好」值）取平均，設成自己的新時間。
- 太快時不倒退，例如每 3 ms 只加 2 ms。

課本的虛擬碼：

```
Procedure distributed clock synchronization:
good-clocks=n;
for(j=1;j < n;j++){
      if (|(ci - cj)| > ε) good-clocks--; // Bad clock
      else total-time= total-time + cj;
      ci=total-time/good-clocks; // set own time equal to the computed time
}
```

（注意 `ci = total-time/good-clocks` 應該在迴圈**之後**才算；課本把它寫在迴圈裡，照縮排看是筆誤。每個時鐘各自獨立執行同樣的步驟。）

---

## 4. 定理：一個 Byzantine 時鐘最多造成 3ε/n 的差距

[07:01]–[18:31]（課本 Theorem 1，p.104–105）

> n 個時鐘的分散式系統中，**一個** Byzantine 時鐘最多讓任兩個好時鐘的計算結果相差 **3ε/n**（ε = 兩時鐘間允許的最大漂移）。

**三個時鐘的證明**：C1 是 Byzantine，C2、C3 是好的，c₃ = c₂ − ε。C1 對 C2 說「c₂ + ε」，對 C3 說「c₃ − ε」——各自都剛好在 ε 內，不會被拒絕。

```
C2 新值 = (c₂ + (c₂ + ε) + c₃) / 3 = (c₂ + c₂ + ε + c₂ − ε) / 3 = c₂
C3 新值 = (c₃ + (c₃ − ε) + c₂) / 3 = (c₃ + c₃ − ε + c₃ + ε) / 3 = c₃ = c₂ − ε
差距 = ε = 3ε/3
```

```
 C1' ──ε── C2 ──ε── C3 ──ε── C1"      （C1 對兩邊說的值，總共拉開 3ε）
```

**推廣到 n 個時鐘**：其餘 n − 3 個好時鐘的讀數總和 R 對兩邊都一樣。

```
C2 的總和 = 3c₂ + R
C3（或老師版本的 C5）的總和 = 3c₂ + R + 3ε
→ 平均後差 3ε / n
```

老師的第二個版本 [14:04]–[18:31]：C1 對 C2 說 c₂ − ε、對 C5 說 c₂ + 2ε，c₅ = c₂ + ε。C5 收到的 c₂ + 2ε 只比自己大 ε，會接受。再取 R 的最壞情況 (n − 3)(c₅ − ε)（再低 C5 就會拒絕），整理成 n·c₅ − nε + 3ε 的形式，用來推下一節的重新同步間隔。

**m 個 Byzantine 時鐘**：每個貢獻最多 3ε/n → 好時鐘之間最多差 **3εm/n**。

---

## 5. 重新同步的間隔與「少於三分之一」

[18:31]–[24:34]（課本式 4.2、4.3）

- 同步後好時鐘相差最多 3εm/n。下次同步前，兩個時鐘最多再漂 2ρΔT。要讓差距不超過 ε：

  **2ρΔT ≤ ε − 3εm/n = (nε − 3εm)/n** → **ΔT ≤ (nε − 3εm) / (2nρ)**

- 右邊要 > 0 → **n > 3m**：Byzantine 時鐘必須**少於三分之一**。
- n ≥ 3m + 1 時 nε − 3εm ≥ ε，所以取 **ΔT = ε / (2nρ)** 一定安全。

⚠️ 老師 [23:30] 說「重新同步間隔 ΔT 應該 **≥** ε/(2nρ)」。間隔越長漂移越多，條件應是**上限**：課本式 4.3 是 ΔT **≤** ε/(2nρ)。

例（老師與課本 Ex 4.3）：10 個時鐘、最多 3 個 Byzantine、ε = 1 ms、ρ = 5×10⁻⁶ → ΔT = 10⁻³ / (2 × 10 × 5×10⁻⁶) = **10 s**。（n = 10 = 3×3 + 1，剛好是邊界。）

⚠️ 課本內部不一致：p.104 說「少於三分之一（也就是四個裡不超過一個）」，Summary 說「不超過 25%」。三分之一和四分之一不同；照式 4.2 推導，條件是 n > 3m（少於三分之一），「一個 / 四個」只是 n = 4、m = 1 這個特例。

---

## 6. 【實測】Byzantine 平均同步模擬

Python 模擬：好時鐘一開始分散在 ε 內；Byzantine 時鐘對奇數編號的時鐘說「比你快將近 ε」，對偶數編號的說「比你慢將近 ε」（剛好不會被拒絕）。跑**一輪**平均：

```
lecture 3-clock example: C1 byzantine tells C2 'c2+e', tells C3 'c3-e'; c3 = c2 - e
   new C2 = 100.000, new C3 = 99.000, difference = 1.000  (bound 3e/n = 1.000)

n clocks, m byzantine, good clocks start spread within eps; spread of good clocks after ONE round:
   n=10 m= 1 (n>3m: True)  spread before=0.819  after=0.275  bound 3*eps*m/n=0.300
   n=10 m= 3 (n>3m: True)  spread before=0.943  after=0.814  bound 3*eps*m/n=0.900
   n=10 m= 4 (n>3m: False)  spread before=0.467  after=0.986  bound 3*eps*m/n=1.200
   n=31 m=10 (n>3m: True)  spread before=0.958  after=0.911  bound 3*eps*m/n=0.968

resync interval: centralized eps/(2 rho) vs distributed eps/(2 n rho), eps = 1 ms
   n=6: centralized 100 s   distributed 17 s
   n=10: centralized 100 s   distributed 10 s
```

- 三時鐘例子：差距剛好 ε = 3ε/3，和老師的推導一致。
- n > 3m 時，一輪後的分散程度都在 3εm/n 內。
- **n = 10、m = 4**（不滿足 n > 3m）：原本只差 0.467，一輪後反而被拉開到 0.986——Byzantine 時鐘太多，平均反而讓好時鐘**分歧**。
- 分散式的重新同步間隔比 centralized 短 n 倍（10 s vs 100 s）：換來沒有單點故障。

---

## 7. 即時優先權等級與排程

[24:34]–[28:01]（課本 Ch5 §2，p.111）

RTOS 要談：即時優先權與排程、資源共享協定、快速搶占、中斷延遲、記憶體鎖定、行為良好的計時器（下一講）、檔案系統。

- 要支援**靜態**任務優先權（像 RMS），也要能有**動態**優先權（像 EDF，優先權是 deadline 的函數）。
- 程式設計師要能指定很高的**即時優先權**。
- OS 沒提供的話，要讓程式設計師能寫自己的**使用者層即時排程器**。老師：很多現代 OS「退出即時這場遊戲」，例如說「有特殊需求就自己寫排程器」（老師以 Windows 為例）。

【課本補充】（p.111）**靜態優先權等級**（real-time priority levels）：程式設計師指定後，OS **不會自己改**。傳統 OS 會為了提高吞吐量動態調整程式設計師給的優先權——那就不是靜態優先權。

---

## 8. 資源共享協定

[28:01]–[28:31]

- 要盡量減少資源共享時的 priority inversion。
- 老師：**一定要支援 PCP**，因為 PIP 和 HLP 不太實用。

⚠️ 課本 p.112 的說法不同：「每個商用 RTOS **至少**要提供基本的 **PIP**；要支援大中型應用時，最好也支援 PCP。」實務上 PIP 是最普遍的（Linux rt_mutex、VxWorks 都是 PI；第 11–12 支影片）。

---

## 9. 快速搶占與中斷延遲

[28:31]–[32:00]（課本 p.111–112）

**任務搶占時間（task preemption time）**：高優先權任務到達後，要等多久才能執行。

| | 搶占時間 |
|---|---|
| RTOS | 幾微秒 |
| 傳統 OS（最壞情況） | 約**一秒**的數量級 |

- RMS、EDF 都依賴快速搶占。
- 課本：傳統 OS 的長延遲來自**不可搶占的 kernel**；RTOS 必須有可搶占的 kernel。

**中斷延遲（interrupt latency）**：中斷發生到對應的 ISR 開始執行（老師還包括任務重新執行）的時間。

- 很多 kernel 會趁中斷時順便做自己的工作 → 要約束這種行為。上限要是**微秒**等級，不是毫秒。
- 做法：中斷處理分兩半。
  - **Top half**：先看中斷；裝置若有即時需求，立刻滿足。
  - **Bottom half**：大部分的處理放進 **DPC queue（deferred procedure call）**，之後以**另外的（較低的）優先權**執行，不一定是即時優先權。
- **巢狀中斷（nested interrupts）**：處理中斷時又來一個——支援的話處理更快，但要保存 / 還原中斷處理常式的 context，不容易。
- 至少不要讓大型 kernel 常式（中斷處理或其他）**不可搶占**：否則即時任務要等它跑完。要重構 kernel，特別是 ISR，支援不同程度的搶占。

課本 p.112：RTOS 不只執行 kernel 常式時要可搶占，**服務中斷時**也要可搶占——對次微秒時間需求的硬即時應用特別重要。

---

## 10. 【原始碼】Linux 的 top half / bottom half：threaded IRQ

Linux 的 threaded IRQ 就是 top half ＋ bottom half 的一種實作：

`kernel/irq/manage.c:2116`（`request_threaded_irq()` 的說明，節錄）

```c
 *	If you want to set up a threaded irq handler for your device
 *	then you need to supply @handler and @thread_fn. @handler is
 *	still called in hard interrupt context and has to check
 *	whether the interrupt originates from the device. If yes it
 *	needs to disable the interrupt on the device and return
 *	IRQ_WAKE_THREAD which will wake up the handler thread and run
 *	@thread_fn. This split handler design is necessary to support
 *	shared interrupts.
```

對照：

| 本講 | Linux |
|---|---|
| Top half：先看中斷、做最急的事 | `handler`：在 hard interrupt context 執行，確認是不是自己的裝置、關掉裝置中斷 |
| Bottom half 放進 DPC queue | 回傳 `IRQ_WAKE_THREAD` → 喚醒 **IRQ 執行緒**跑 `thread_fn` |
| Bottom half 以自己的優先權執行 | IRQ 執行緒是 `SCHED_FIFO` 的 kernel 執行緒，可以像一般任務一樣被排程、被更高優先權搶占 |

PREEMPT_RT（第 2 支影片筆記）的 "enforcing interrupt threading" 就是強制所有中斷都走這條路，讓中斷處理幾乎全部可被排程器管理。Linux 另外還有 softirq、tasklet、workqueue 等 bottom half 機制。

---

## 11. 記憶體管理：虛擬記憶體、記憶體保護、memory locking

[32:00]–[35:30]（課本 p.112）

**虛擬記憶體**：

- 好處很多，最重要的是**安全**：兩個程序不能覆寫彼此的資料。
- 但大多數嵌入式 RTOS **不支援**：它讓**最壞情況**的記憶體存取時間大增——page fault 時要從硬碟抓頁，對即時程序很糟。最好整個記憶體配置都在掌控中、存取時間完全確定。
- 虛擬記憶體能降低**平均**存取時間（需要的記憶體比實體記憶體多時），但現在更被看作**安全與正確性**的功能。
- 有沒有 RTOS 支援？大型複雜應用一定需要；要同時跑文字編輯器、瀏覽器之類也需要。很小的 IoT 處理器、任務行為良好時，可以不用。

**記憶體保護的取捨**：

| 單一位址空間（無保護） | 有保護 |
|---|---|
| 系統呼叫很輕量 | — |
| 開發與測試成本變高：虛擬記憶體原本提供的正確性保證要自己做 | — |
| 維護成本高：任何新功能都可能弄壞系統 | — |

【課本補充】（p.112）小型嵌入式應用中，每個程序多幾 KB 的記憶體開銷都無法接受；沒有記憶體保護時，系統呼叫和函式呼叫沒有差別，一個亂指的指標就能毀掉 OS、讓系統「凍住」，很難除錯。不支援虛擬記憶體的 RTOS 會配置**實體連續**的記憶體區塊，但可能有碎片問題。

**Memory locking**：

- 不讓某些頁被換到硬碟，避免 page fault 的延遲與 jitter。
- 空間夠就沒問題；空間不夠時還是得換出——即使很關鍵的任務也可能因此遭受很大的記憶體存取延遲。

---

## 12. 【實測】page fault 的 jitter 與 mlock

本機 `ulimit -l` 很大（一般使用者也能 `mlock`）。配置 20,000 頁，量**第一次寫入**每一頁的時間：

```c
char *p = mmap(0, (size_t)N * 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
if (lock && mlock(p, (size_t)N * 4096)) { perror("mlock"); exit(1); }   /* mlock faults all pages in now */
for (int i = 0; i < N; i++) { double t0 = ns(); p[(size_t)i * 4096] = 1; double d = ns() - t0; ... }
```

結果（跑兩次）：

```
plain mmap             avg  1035.0 ns  worst   47057.0 ns per first write
mlock'ed (prefaulted)  avg    14.9 ns  worst     239.0 ns per first write
plain mmap             avg   771.7 ns  worst    7596.0 ns per first write
mlock'ed (prefaulted)  avg    14.6 ns  worst    7240.0 ns per first write
```

- 沒有鎖定：每一頁第一次寫入都觸發 page fault（這裡是 minor fault：分配一頁零頁，不用碰硬碟），平均約 **0.8–1 μs**，最壞到 **47 μs**。
- `mlock` 會在呼叫時就把所有頁**先 fault 進來並鎖住**，之後存取平均只要 **15 ns**，快約 60 倍。
- 第二次 mlock 的最壞值 7.2 μs：不是 page fault，而是那一刻剛好被中斷或排程打斷——提醒我們 jitter 還有其他來源（第 9 支影片）。
- 如果是被換出到硬碟的頁（major fault），代價會是毫秒等級。即時程式通常在初始化時呼叫 `mlockall(MCL_CURRENT | MCL_FUTURE)`。

---

## 13. 同步與非同步 I/O

[35:30]–[37:31]（課本 p.112–113）

| | 同步 I/O | 非同步 I/O（non-blocking） |
|---|---|---|
| 做法 | `read` / `write` 系統呼叫後程序**阻塞**，直到完成 | 發出請求後繼續做事，結果好了 OS 發中斷（通知） |
| 優點 | 中斷開銷大時比較有效率 | 不浪費處理器時間；其他程序在資料傳輸期間可以跑 |
| 適合 | 很短很快的 I/O | 較長的 I/O |

一般而言 RTOS 偏好非同步 I/O，但要看 I/O 操作而定。

另外老師提醒：存取外部硬碟很慢，會帶來大量不確定性；檔案系統、網路裝置的存取也要「行為良好」。

---

## 14. 一頁速查表

### 14.1 分散式時鐘同步

| 項目 | 內容 |
|---|---|
| 做法 | 交換讀數；忽略差距 > ε 的；其餘取平均；太快就慢慢調 |
| Byzantine | 給不同人不同的值；剛好在 ε 內就不會被拒 |
| 定理 | 一個 Byzantine 最多造成 3ε/n；m 個 → 3εm/n |
| 條件 | n > 3m（少於三分之一） |
| 間隔 | ΔT ≤ (nε − 3εm)/(2nρ)；取 ε/(2nρ) 安全（Ex 4.3：10 s） |
| 實測 | n = 10、m = 4 時一輪後好時鐘反而分歧 |

### 14.2 RTOS 要求

| 要求 | 內容 |
|---|---|
| 優先權 | 靜態即時優先權（OS 不改）＋ 可做 EDF；或允許使用者排程器 |
| 資源共享 | 老師：PCP；課本：至少 PIP，最好 PCP |
| 搶占 | 微秒級（傳統 OS 最壞約 1 秒）；可搶占的 kernel |
| 中斷 | 延遲有上限（微秒）；top half ＋ DPC bottom half；巢狀中斷 |
| 記憶體 | 嵌入式 RTOS 常不用虛擬記憶體；memory locking；保護的取捨 |
| I/O | 偏好非同步 I/O |
| Linux | threaded IRQ（`IRQ_WAKE_THREAD`）；`mlock`：首次寫入 1 μs → 15 ns |

---

## 15. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **計時器、時鐘中斷、watchdog（課本 Ch5 §1）** | **第 18 支**：Hardware, Timers, and OSes |
| **可搶占 kernel、kernel 架構** | **第 19 支**：Monolithic and Micro-kernels |
| **Windows 為什麼不適合即時（DPC 等）** | **第 20 支**：Windows NT as an RTOS |
| **POSIX 的即時擴充（mlock、非同步 I/O、優先權）** | **第 21 支**：POSIX standards |
| **Linux 的即時化** | **第 23 支**：Linux-based RTOSs |

---

## 附：本講一句話總結

> **沒有 master 時，所有時鐘定期交換讀數、丟掉和自己差超過 ε 的值、把其餘的取平均（太快就慢慢調），這能擋住停掉或漂太多的壞時鐘，但會說謊、對不同人給不同值的 Byzantine 時鐘可以剛好躲在 ε 內：一個 Byzantine 時鐘最多讓兩個好時鐘差 3ε/n，m 個就是 3εm/n，所以必須 n > 3m（少於三分之一），重新同步間隔 ΔT ≤ (nε − 3εm)/(2nρ)，取 ε/(2nρ) 就安全（10 個時鐘、ε = 1 ms、ρ = 5×10⁻⁶ 時為 10 s）；模擬也顯示 Byzantine 時鐘超過三分之一時平均會讓好時鐘越來越分歧。RTOS 則要提供 OS 不會改動的靜態即時優先權（並能做 EDF 或讓使用者自寫排程器）、資源共享協定（老師說 PCP，課本說至少 PIP）、微秒級的任務搶占與有上限的中斷延遲（可搶占 kernel，中斷分成 top half 與延後執行的 DPC bottom half，Linux 的 threaded IRQ 正是如此）、memory locking（實測 mlock 讓首次存取從約 1 μs 降到 15 ns，最壞 47 μs 的 page fault 消失），以及非同步 I/O；嵌入式 RTOS 常為了確定的存取時間而不用虛擬記憶體，代價是失去記憶體保護。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 18 錄影逐字稿（YouTube 自動字幕，37:47）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch4 §6、Ch5 §2 整理。原始碼為 Linux v6.2.12；實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
