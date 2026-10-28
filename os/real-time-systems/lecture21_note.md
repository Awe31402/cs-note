# Real Time Systems 第二十一支影片完整筆記：POSIX 標準（Lecture 22）

> **課程**：Real Time Systems — Lecture 22：POSIX standards（播放清單第 21 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（35:24，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch5 §6（PDF p.124–126）
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
- [1. 複習：Windows 與 Linux 的時間片與優先權](#1-複習windows-與-linux-的時間片與優先權)
- [2. POSIX 的起源](#2-posix-的起源)
- [3. Open system 與 open software](#3-open-system-與-open-software)
- [4. POSIX 規定什麼、不規定什麼](#4-posix-規定什麼不規定什麼)
- [5. Application environment profiles](#5-application-environment-profiles)
- [6. Real-time POSIX 的要求](#6-real-time-posix-的要求)
- [7. 排程政策：SCHED_FIFO、SCHED_RR、SCHED_OTHER](#7-排程政策sched_fifosched_rrsched_other)
- [8. 即時信號](#8-即時信號)
- [9. 執行緒](#9-執行緒)
- [10. SCHED_SPORADIC](#10-sched_sporadic)
- [11. 【原始碼】【實測】POSIX.4 在 Linux 上](#11-原始碼實測posix4-在-linux-上)
- [12. 一頁速查表](#12-一頁速查表)
- [13. 與後續講次的連結](#13-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

**POSIX**（Portable Operating System Interface）：讓程式在不同 OS 上**原始碼層級**可攜。這門課關心的是第四部分 **POSIX.4（real-time extensions，POSIX-RT）**：

- 靜態優先權（至少 32 級）、SCHED_FIFO / SCHED_RR
- 系統呼叫有最壞執行時間
- 計時器（相對 / 絕對、高精度 nanosleep）
- 記憶體鎖定（mlock / mlockall）
- 不會遺失、有優先順序的**即時信號**
- 執行緒、SCHED_SPORADIC

---

## 1. 複習：Windows 與 Linux 的時間片與優先權

[00:00]–[06:31]

**時間片**（比計時器中斷間隔大得多，可設定）：

| | 時間片 |
|---|---|
| Windows | 10–120 ms；背景程序稍短、前景稍長（前景 / 背景約差 1:2 或 1:3） |
| Linux | 10–200 ms，預設 100 ms；依任務性質、使用者參數、用了多少 CPU 而變 |

兩者的 kernel 都在不同程度上**可重入、可搶占**。

**優先權**：

- Windows：0–31，idle 程序多半是 1，大多數程序在動態範圍，16–31 是即時範圍（數字越大越高）。即時類別固定優先權 16–31；動態類別 1–15 依行為變動；動態執行緒**醒來時會被提升**（例如 I/O 完成、變成前景）；一般不會被降低（除非自己 nice）。
- Linux：**140** 個優先權等級。
  - 即時：0–99。
  - 一般：100–139，預設 120；加 nice 調整。負的 nice（提高優先權）要 superuser；正的 nice（對別人好）隨時可以。
  - CPU 密集任務多在某一端，I/O 密集任務和低優先權 kernel 執行緒在另一端。
  - Linux 沒有像 Windows 那樣特別提升互動任務的機制，但在即時方面表達力更強：很多 kernel 執行緒本身就用即時優先權。
- 同一優先權（例如 25）內，可以選 **FIFO** 或 **round robin**；更高優先權的即時程序永遠先跑。

⚠️ 老師說 Linux 即時範圍「數字越大優先權越高，99 最高」，後面又說「100 之後數字越大越低」。兩個說法用的是**不同的尺度**：

- 使用者空間的 `sched_priority`（`chrt`、`sched_setscheduler`）：1–99，**越大越高**。
- kernel 內部的 `p->prio`：0–139，**越小越高**；即時任務的 `prio = 99 − sched_priority`，一般任務 `prio = 120 + nice`（`include/linux/sched/prio.h`，第 9 支影片筆記）。

**排程複雜度**【口頭】：很舊的 Linux 要掃描全部任務找最高優先權，O(n)；2.6 之後有**每 CPU 佇列**，任務依優先權組織（老師稱為 radix tree），到某個優先權等級就是一條 linked list，FIFO 和 RR 都很容易做。

⚠️ v6.2.12 的即時排程類別用的是 **bitmap ＋ 每級一條 list**（`struct rt_prio_array`，第 7 支影片筆記），找最高優先權是 `sched_find_first_bit()`，不是 radix tree。

---

## 2. POSIX 的起源

[06:31]–[10:31]（課本 §6.2，p.125）

- 目標：跨 OS 標準化政策、API、介面；增加附加套件的可用性；開發者不必為不同 OS 寫不同程式；方便系統整合與移植。
- Unix 的分支：AT&T 在 80 年代把 Unix 釋出；很多廠商各自延伸（多為商用）：

| 廠商 | Unix |
|---|---|
| IBM | AIX |
| HP | HP-UX |
| Sun | Solaris（網際網路初期 Sun 提供大量網路硬體，非常有影響力） |
| DEC | Ultrix |
| SCO | SCO Unix |

- 家族樹：VAX/VMS 那一支隨 VAX 機器消失；**BSD**（Berkeley Software Distribution）仍用在很多商用場合；System III、System V；IBM、HP 的版本因為專有而沒那麼普及；真正普及的是 Linux——kernel 獨立開發，但承襲 Unix 哲學、和 Unix 很相容。
- 問題：同一個程式在不同 Unix 上跑不了；服務、語法、語意都不同。
- 第一次標準化：AT&T 的 **System V Interface（SVID）**。但社群分裂，AT&T / System V 和 Berkeley 走不同方向。
- **IEEE** 接手，產生 **IEEE 1003**（老師說 1990 年）。

【課本補充】（p.125）Unix 由 AT&T Bell Labs 在 70 年代初開發；AT&T 是電信公司，覺得 Unix 沒商業價值，就免費把原始碼給很多大學，UC Berkeley 是最早的之一。之後 AT&T 發展 Unix V；Berkeley 拿 DARPA 經費把 TCP/IP 加進 Unix，做出 BSD。SVID 被 BSD 和其他廠商忽略，下一個嘗試是 ANSI/IEEE，產生 POSIX。

（⚠️ 年份：老師說 IEEE 在 1990 年產生第一個標準；課本沒寫年份，待補。）

---

## 3. Open system 與 open software

[10:31]–[14:01]（課本 §6.1，p.124–125）

- **Open source software**（老師引 Wikipedia）：允許使用者研究、修改、改善、再散布。授權不同：有的「拿去隨便用」，有的如 Linux 用的 **GNU GPL**，散布的產品必須附原始碼。
- **Open system ≠ open software**：

| | Open system | Open software |
|---|---|---|
| 範圍 | 更廣：軟體、協定、網路、甚至部分硬體；開放介面與標準 | Open system 的一個分支 |
| 好處 | **廠商中立**：可以自由混搭不同廠商的硬體、軟體、網路方案、第三方方案，因介面標準而保證能一起運作 | 降低開發成本與上市時間、更多附加套件、好寫程式、好整合 |

- 例【口頭】：換別家的鍵盤、滑鼠、裝別的 OS 都能用，因為有標準。70 年代末還不是這樣，Unix 廠商推動 open system，但很多大廠商反對。
- 開放系統與標準**沒有著作權** → 省掉昂貴的智財訴訟；有互通性與可攜性。
- 課本：open system 兩大目標是 **interoperability**（不同廠商的系統能交換資訊）與 **portability**（系統能不修改地搬到別的環境）。

【課本補充】（p.125）開放軟體標準三類：

| 類別 | 可攜層級 | 例子 |
|---|---|---|
| Open source | 原始碼：重新編譯、連結即可 | ANSI、POSIX |
| Open object | 未連結的 object module：重新連結 | — |
| Open binary | 執行檔：完全可攜 | 課本：目前不存在 |

POSIX 的主要目標是**原始碼層級**的應用可攜性。

---

## 4. POSIX 規定什麼、不規定什麼

[13:30]–[16:33]（課本 §6.3，p.125–126）

- 關心原始碼層級的可攜：在 Linux 寫的程式應能在別的 OS 上跑。老師例：vi 編輯器在各種 Unix / Linux 間大致可直接用；Windows 另當別論，但 Windows 也提供很多 POSIX 相容 API，搬移時改動不多。
- 現在被廣泛接受，很多非 Unix 的 OS 也讓自己 POSIX 相容。IEEE 標準，也是 **ISO 9945**。

> **POSIX 只定義 OS 服務的介面與語意，不規定服務怎麼實作。**

課本補充：POSIX 規定系統呼叫、確切參數、語意；不規定 kernel 要單執行緒還是多執行緒、kernel 服務在什麼優先權執行、用什麼語言寫。

**POSIX 的四個部分**：

| 部分 | 內容 | 老師的例子 |
|---|---|---|
| POSIX.1 | 系統介面與系統呼叫參數 | glibc（C / C++，甚至 Java 等也依賴部分 libc） |
| POSIX.2 | Shell 與工具程式 | ls、grep；PowerShell 也有很多 |
| POSIX.3 | 驗證是否符合 POSIX 的測試方法 | 測 POSIX.1、POSIX.2 的相容性 |
| **POSIX.4** | **即時擴充** | 本講重點 |

POSIX.4 讓 RTOS 能**互相相容**：在一個系統上不錯過 deadline 的程式，搬到另一個系統、照規則執行，希望也能正確執行。

---

## 5. Application environment profiles

[16:33]–[20:31]

- 很多小型嵌入式系統限制很多，不會支援整個 POSIX，只支援一部分。
- 即時工作小組定義了四種 **AEP（application environment profile）**：什麼時候、怎麼實作哪個 POSIX 子集。
- 相關文件：**POSIX.4**（即時應用可攜性的介面）；子標準 **POSIX.4a**（執行緒擴充）、**POSIX.13**（各 AEP 要支援哪些服務）。

| AEP | 系統 | 特徵 |
|---|---|---|
| **Minimal system** | 小型嵌入式（例如感測器單元） | 沒有 MMU、沒有磁碟、沒有 I/O 終端；沒有虛擬記憶體和次級儲存 |
| **Real-time controller** | 多了持久儲存 | 有檔案系統和 I/O 終端；仍是**單一程序**（用多個函式模擬多個程序，像我們 EDF / RMS 的簡化模型），但允許**多執行緒** |
| **Dedicated system** | 大型嵌入式 | 很多感測器、記憶體模組、I/O 模組；**沒有檔案** |
| **Multi-purpose system** | 一般桌機或伺服器 | 支援所有 POSIX.4 功能 |

---

## 6. Real-time POSIX 的要求

[20:31]–[24:01]（課本 §6.4，p.126）

| 要求 | 內容 |
|---|---|
| **系統呼叫的效能** | 每個系統呼叫要有**最壞執行時間**的上限 → 才能規劃（老師：即時系統就是規劃） |
| **靜態即時優先權** | 不會變的優先權（RMS 需要）；**至少 32 級** |
| **計時器** | **相對**計時器（從上個事件算起）和**絕對**計時器（實作時鐘）；**高精度** `nanosleep`——讓程序休息很短的時間，不必用浪費電的空迴圈 |
| **記憶體鎖定** | `mlock`（鎖一頁 / 一段）、`mlockall`（鎖整個位址空間）、`munlock` → 存取時間確定，不必到 swap |
| **多執行緒** | 必須支援 |
| **檔案** | 很棘手：檔案在次級儲存，存取時間不確定，所以很多 AEP 沒有檔案系統 |

【課本補充】（p.126）

- 計時器：週期與一次性（watchdog）計時器；支援 POSIX-RT 時系統時鐘叫 **CLOCK_REALTIME**。
- **即時檔案**：要支援即時檔案系統——預先配置儲存空間、檔案區塊連續存放 → 存取延遲可預測。
- 記憶體鎖定的服務：`mlockall()`、`mlock()`、`mlockpage()`；解鎖 `munlockall()`、`munlock()`、`munlockpage()`。
- 執行緒：即時執行緒是可排程的實體，可以有個別的時間限制，屬於同一組時也可以有集體的時間限制。

⚠️ 課本列的 `mlockpage()` / `munlockpage()` 不在 POSIX 標準裡，Linux 也沒有這兩個函式（Linux 有 `mlock`、`mlock2`、`mlockall`、`munlock`、`munlockall`）。

---

## 7. 排程政策：SCHED_FIFO、SCHED_RR、SCHED_OTHER

[24:01]–[29:30]

| 政策 | 說明 |
|---|---|
| **SCHED_FIFO** | 可搶占的優先權排程；同等級先來先服務，**沒有時間片** |
| **SCHED_RR** | SCHED_FIFO ＋ **時間片**：用完換同等級的下一個，大家都輪過一次才再輪到 |
| **SCHED_OTHER** | 其他排程（老師說可以是自訂的 EDF、RMS 等）；在 Linux 是預設的分時排程器 |

- 優先權範圍用 `sched_get_priority_min()` / `sched_get_priority_max()` 查：Linux 的 SCHED_FIFO / RR 是 **1（最低）到 99（最高）**。
- SCHED_RR 的時間片用 `sched_rr_get_interval()` 查。
- SCHED_FIFO 程序一變成可執行，**一定**搶占 SCHED_OTHER 程序；但不能搶占同等級的 SCHED_FIFO 程序。
- SCHED_FIFO 程序會一直跑，直到：
  - 因 I/O 請求而阻塞；
  - 被更高優先權的程序搶占；
  - 自己呼叫 `sched_yield()` → 移到該等級 FIFO 佇列的**尾端**（變成最近到達的）；
  - 做完。
- 老師的例子：P1 先跑、去做 I/O 進 kernel；P2 跑；P1 的 I/O 完成、P1 優先權較高 → P2 被搶占。
- **SCHED_OTHER** 在 Linux 是預設的分時排程器，靜態優先權 **0** → 低於所有 1–99 的即時程序。它負責非即時程序：只要處理器還有空閒（沒有即時任務可跑），就跑一般任務——和 EDF / RMS 下「有空檔才跑非即時任務」的想法一樣。

⚠️ 老師說 SCHED_FIFO 程序「被 I/O 阻塞或被搶占時，會呼叫 `sched_yield`（自己或 kernel 代它呼叫）」[25:33]。`sched_yield()` 是程序**主動**讓出時才呼叫的；被高優先權搶占時並不會 yield——POSIX 規定被搶占的 SCHED_FIFO 執行緒留在其優先權佇列的**最前面**，等高優先權的跑完就繼續跑它。阻塞後再變成可執行時，才放到佇列尾端。

⚠️ 老師說 SCHED_OTHER 可以用來實作 EDF、RMS 等自訂排程。POSIX 對 SCHED_OTHER 的語意是「實作定義」；在 Linux 上它就是 CFS（一般任務），不能用來做 EDF。Linux 的 EDF 是另一個政策 `SCHED_DEADLINE`（第 6 支影片筆記）。

---

## 8. 即時信號

[29:30]–[30:30]

| | 傳統信號（POSIX.1） | **即時信號（POSIX.4）** |
|---|---|---|
| 排隊 | **不排隊**：同一種信號未處理時再來就會遺失 | **一定排隊，永不遺失** |
| 優先順序 | 沒有 | **依優先順序**處理 |
| 資料 | 不同事件可能產生同一號信號，無法區分 | 帶**額外欄位**，可以在 kernel 和即時程序之間、或即時程序之間交換資料 |

老師：傳統信號沒被當一回事（大不了再送一次）；POSIX.4 很認真看待它。（§11 實測。）

---

## 9. 執行緒

[30:30]–[34:02]

為什麼 POSIX.1 的**程序模型**不適合很多即時應用：

- 程序的 context switch 很貴。
- 很多嵌入式應用沒有虛擬記憶體 → 沒辦法有獨立的位址空間（base–limit 之類的簡單方法問題很多）。

| | 單執行緒程序 | 多執行緒程序 |
|---|---|---|
| 共享 | — | 程式碼、全域資料、檔案、網路連線 |
| 各自擁有 | 全部 | **程式計數器、暫存器、堆疊** |

- 執行緒切換輕量得多；**不需要虛擬記憶體**就能做多執行緒。
- 老師引用：Solaris 上建立執行緒比建立程序便宜 **30 倍**，執行緒 context switch 快 **5 倍**。

**執行緒排程的三種範圍**：

| 種類 | 說明 |
|---|---|
| **Global** | 所有執行緒在全系統競爭 CPU 與資源 |
| **Local** | 每個多執行緒程序分到一組 CPU，執行緒只和**同一程序**的執行緒競爭（嵌入式系統通常沒有多個多執行緒程序，用不到） |
| **Mixed** | 有些執行緒是全域競爭，有些是局部競爭 |

（POSIX 的術語是 `PTHREAD_SCOPE_SYSTEM` / `PTHREAD_SCOPE_PROCESS`；Linux 只支援 system scope。）

---

## 10. SCHED_SPORADIC

[34:02]–[35:24]

- 複習：非週期與偶發任務的差別是偶發任務有**最小到達間隔**（第 4 支影片）。
- **POSIX.4b** 定義了 **SCHED_SPORADIC**：用來在想要的優先權等級處理偶發事件，同時確保低優先權任務不受影響——就是第 8 支影片的 **sporadic server**。

（第 8 支影片筆記已查過：Linux v6.2.12 **沒有**實作 SCHED_SPORADIC。）

---

## 11. 【原始碼】【實測】POSIX.4 在 Linux 上

**傳統信號 vs 即時信號**：kernel 決定要不要把信號放進佇列時：

`kernel/signal.c:1074`

```c
static inline bool legacy_queue(struct sigpending *signals, int sig)
{
	return (sig < SIGRTMIN) && sigismember(&signals->signal, sig);
}
```

`kernel/signal.c:1099`（`__send_signal_locked()` 裡）

```c
	result = TRACE_SIGNAL_ALREADY_PENDING;
	if (legacy_queue(pending, sig))
		goto ret;
```

- 傳統信號（< SIGRTMIN）已經在 pending 集合裡 → **直接丟掉**。
- 即時信號（≥ SIGRTMIN）每次都配置一個 `sigqueue` 放進佇列。
- 取信號時 `next_signal()`（`kernel/signal.c:207`）從 bitmap 找**編號最小**的 pending 信號 → 即時信號中編號越小優先權越高。

【實測】先把信號擋住，交錯送出 3 次 SIGUSR1、3 次 SIGRTMIN+5（值 1–3）、3 次 SIGRTMIN（值 1–3），再一起放行：

```c
for (int k = 1; k <= 3; k++) {
  kill(getpid(), SIGUSR1);
  sigqueue(getpid(), SIGRTMIN + 5, (union sigval){ .sival_int = k });
  sigqueue(getpid(), SIGRTMIN, (union sigval){ .sival_int = k });
}
```

```
SCHED_FIFO 1..99  SCHED_RR 1..99  SCHED_OTHER 0..0
sched_rr_get_interval (as SCHED_OTHER) = 0.020000000 s
SIGRTMIN=34 SIGRTMAX=64 (31 real-time signals)
sent 3x SIGUSR1, 3x SIGRTMIN+5 (value 1..3), 3x SIGRTMIN (value 1..3), interleaved
delivered (7): USR1 RT0:1 RT0:2 RT0:3 RT5:1 RT5:2 RT5:3
create+join thread 17.5 us, fork+wait process 107.9 us  (6x)
```

看到什麼：

- 送了 9 個信號只收到 7 個：**3 個 SIGUSR1 只剩 1 個**（不排隊），即時信號 6 個全收到（排隊、不遺失）。
- 即時信號依**編號**排序：SIGRTMIN（RT0）全部先於 SIGRTMIN+5（RT5），即使送出時是交錯的；同一種內部依送出順序（FIFO），而且帶著 `sival_int` 資料。
- SIGUSR1（10 號）在 bitmap 的第一個 word，比所有即時信號都先送達。
- 優先權範圍：SCHED_FIFO / RR 1–99（遠超過 POSIX 要求的 32 級），SCHED_OTHER 只有 0。
- `sched_rr_get_interval()` 對 SCHED_OTHER 程序回傳 20 ms（CFS 給這個任務的時間片估計）；對 SCHED_RR 程序才是 RR 時間片（`/proc/sys/kernel/sched_rr_timeslice_ms` = 100 ms，第 9 支影片筆記）。
- 建立 + join 一個執行緒 17.5 μs，fork + wait 一個程序 108 μs：Linux 上約 **6 倍**（老師引的 Solaris 是 30 倍；Linux 的 fork 用 copy-on-write，本來就比較便宜）。

---

## 12. 一頁速查表

| 主題 | 重點 |
|---|---|
| 時間片 | Windows 10–120 ms（前景較長）；Linux 10–200 ms，預設 100 ms |
| Linux 優先權 | 140 級；即時：使用者 1–99（大 = 高），kernel prio 0–99（小 = 高）；一般 100–139（120 + nice） |
| POSIX 起源 | Unix 分支（AIX、HP-UX、Solaris、Ultrix、SCO、BSD…）→ SVID 失敗 → IEEE 1003 / ISO 9945 |
| 規定範圍 | 只規定介面與語意，不規定實作 |
| 四部分 | .1 系統介面、.2 shell 與工具、.3 測試、**.4 即時擴充** |
| AEP | minimal（無 MMU / 磁碟）、real-time controller（有檔案、單程序多執行緒）、dedicated（大、無檔案）、multi-purpose（全部） |
| POSIX-RT | 系統呼叫最壞時間、≥ 32 個靜態優先權、相對 / 絕對 / 高精度計時器、mlock / mlockall、執行緒、即時檔案 |
| 排程 | SCHED_FIFO（無時間片）、SCHED_RR（有時間片）、SCHED_OTHER（Linux：CFS，靜態優先權 0）；SCHED_SPORADIC（POSIX.4b，Linux 沒有） |
| 即時信號 | 排隊不遺失、依編號優先、帶資料；Linux 的 `legacy_queue()` 丟掉重複的傳統信號 |
| 執行緒 | 共享碼、資料、檔案；各有 PC、暫存器、堆疊；不需虛擬記憶體；global / local / mixed scope |
| 實測 | 3 個 SIGUSR1 只收到 1 個；即時信號 6 個全收、RT0 先於 RT5；執行緒建立比程序快 6 倍 |

---

## 13. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **各 RTOS 的 POSIX 相容性** | **第 22 支**：Open Source and Commercial RTOSs |
| **Linux 的 SCHED_FIFO / RR、PREEMPT_RT** | **第 23 支**：Linux-based RTOSs |
| **VxWorks、QNX、PSOS 的 POSIX 支援** | **第 24 支**：Commercial RTOSs |
| **系統呼叫最壞時間的量測** | **第 26 支**：Benchmarking RTOSs |

---

## 附：本講一句話總結

> **POSIX 是為了解決 Unix 各分支（AIX、HP-UX、Solaris、Ultrix、BSD…）互不相容而由 IEEE 制定的標準（也是 ISO 9945），只規定 OS 服務的介面與語意而不規定實作，目標是原始碼層級的可攜性；它分成系統介面、shell 與工具、相容性測試和即時擴充四部分，其中 POSIX.4（POSIX-RT）要求系統呼叫有最壞執行時間、至少 32 個靜態優先權、相對與絕對以及奈秒級的計時器、mlock/mlockall 記憶體鎖定、多執行緒，以及會排隊、不遺失、依編號排優先且能帶資料的即時信號，排程政策有無時間片的 SCHED_FIFO、有時間片的 SCHED_RR、預設的 SCHED_OTHER 和 POSIX.4b 的 SCHED_SPORADIC；為了讓小型嵌入式系統只實作需要的子集，又定義了 minimal、real-time controller、dedicated、multi-purpose 四種 application environment profile。Linux 提供 1–99 的 SCHED_FIFO/RR（kernel 內部 prio 數字越小越高），沒有 SCHED_SPORADIC；實測交錯送出的 3 個 SIGUSR1 只收到 1 個，而即時信號 6 個全部送達、SIGRTMIN 一律先於 SIGRTMIN+5，對應 kernel 裡 legacy_queue() 丟棄重複傳統信號的那一行；執行緒建立則比程序快約 6 倍。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 22 錄影逐字稿（YouTube 自動字幕，35:24）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch5 §6 整理。原始碼為 Linux v6.2.12；實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
