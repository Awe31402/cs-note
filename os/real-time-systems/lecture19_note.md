# Real Time Systems 第十九支影片完整筆記：Monolithic 與 Microkernel（Lecture 20）

> **課程**：Real Time Systems — Lecture 20：Monolithic and Micro-kernels（播放清單第 19 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（17:38，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch5 §3（PDF p.113–117）
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
- [1. Monolithic kernel](#1-monolithic-kernel)
- [2. Microkernel](#2-microkernel)
- [3. 代價：context switch 與 IPC](#3-代價context-switch-與-ipc)
- [4. 對即時系統的意義：deferred processing](#4-對即時系統的意義deferred-processing)
- [5. 【實測】系統呼叫 vs 向使用者模式 server 要服務](#5-實測系統呼叫-vs-向使用者模式-server-要服務)
- [6. 【課本補充】傳統 Unix 當 RTOS 的問題](#6-課本補充傳統-unix-當-rtos-的問題)
- [7. 一頁速查表](#7-一頁速查表)
- [8. 與後續講次的連結](#8-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

（老師開頭說要談 Unix 與 Windows 當 RTOS，實際內容是 monolithic vs microkernel；Windows 在下一支影片。）

| | Monolithic | Microkernel |
|---|---|---|
| 結構 | 所有 kernel 功能一整塊、同一個位址空間 | kernel 極小，其他服務在使用者模式 |
| 優點 | 效能好、好懂、好除錯 | 小、模組化、可延伸、元件隔離 |
| 缺點 | 元件不隔離、大、難客製 | context switch 多、IPC 開銷大 |

---

## 1. Monolithic kernel

[00:00]–[06:02]

```
   Application programs
   ─────────────────────────── System services interface（系統呼叫）
   File system
   程序管理 │ 記憶體管理 │ 裝置管理 │ 儲存管理 │ I/O 管理      ← 全在 kernel mode
   ───────────────────────────
   Hardware
```

- Linux、Unix 屬於這一類：從 HAL 到核心模組全是**同一個程式**，不能拆開。
- Linux 的 **loadable kernel module（LKM）** 可以在執行時擴充或修改功能，但那不是 Linux 的核心本質。

| 優點 | 缺點 |
|---|---|
| 單一程式、單一位址空間；方法成熟、好懂 | kernel 元件之間**沒有保護、沒有隔離** |
| 效能好 | 不容易延伸、修改、客製；很難支援非常多樣的應用 |
| 好寫、好除錯、好驗證（行為種類有限） | kernel 很大（所有功能編譯、連結在一起）；共享位址空間 → 一部分的 bug、病毒或失誤可能破壞另一部分，難以隔離，可能不穩定 |
| | 商用 Unix 的 kernel 原始碼不公開：要新功能只能請原作者重新編譯（Linux 開源，沒有這個問題） |
| | 綁定特定硬體與特定應用需求 |

- 老師的觀點：要做跑在很多硬體上的**通用** OS，monolithic 不是好主意；但嵌入式即時系統的硬體固定，monolithic 可以接受。
- 老師的例子【口頭】：Apple 自己做硬體，OS 不必支援各種硬體，可以是 monolithic；Windows 要跑在各種機器上，需要執行期的彈性。

⚠️ 老師以 Apple 的 OS 為 monolithic 的例子。Apple 的 XNU kernel 一般被歸類為**混合式（hybrid）**：Mach microkernel 加上 BSD 的部分，跑在同一個 kernel 位址空間。講義沒有這段，待補。

---

## 2. Microkernel

[06:02]–[12:31]

- **極簡**：能放使用者空間的都放使用者空間。
- kernel 本身模組化、很小：從 **10 KB** 到幾百 KB。容易移植、維護、延伸。
- 該放什麼進 kernel 沒有共識，但關鍵的通常是：
  - **中斷處理**
  - 基本的**記憶體管理**：支援不同位址空間、記憶體保護
  - 基本的**程序 / 執行緒**實作
  - 基本的 **IPC**（行程間通訊）
- 真正的目標：**小而有彈性**。但要做小並不簡單——這從根本上改變 OS 功能的建構與交付方式。**模組化才是重點，小只是副作用**；不能只因為小就叫 microkernel，還要有彈性。

```
 Client app │ 程序管理/排程器 │ 檔案系統 │ 驅動程式 │ 計時器 │ 管線…   ← user mode
 ───────────────────────────────────────────────────────────────
              Microkernel（排程、記憶體管理、IPC 的「雛形」）           ← kernel mode
 ───────────────────────────────────────────────────────────────
                               Hardware
```

- 「kernel 的 kernel」：只有它在保護模式。
- Client–server 式 OS：kernel 仍要實作排程、記憶體管理、IPC，但進階的版本可以在使用者空間。
  - 例：kernel 只提供基本的 FIFO 或 round robin 排程，但允許使用者寫專門的排程器並連結進來；kernel 需要排程時呼叫那個使用者模式程序。
  - 設定計時器、鬧鐘、其他 IPC 機制（pipe、message queue）都可以放使用者空間。

**優點**：

| 優點 | 說明 |
|---|---|
| 小 | 適合嵌入式應用 |
| 模組化、可延伸、可客製 | 比較是筆電桌機需要的；但嵌入式若有很多特殊感測器或介面也有用 |
| 對即時有利 | kernel 礙事時，即時需求高的使用者程序可以取代原本的 kernel 程序先跑 |
| 開發分散 | 元件間介面明確；同一服務可以有多種實作（老師：這點有爭議） |
| 穩定 | 隔離、可逐元件測試 → bug 較少；元件失敗可以**重新啟動**（老師：也有爭議——編譯時不知道元件之間怎麼互動，宣稱整體穩定不太實際） |

---

## 3. 代價：context switch 與 IPC

[12:31]–[17:02]

```
Monolithic：                       Microkernel：
User ──syscall──▶ Kernel           User ──syscall──▶ Kernel ──▶ User-mode server
User ◀─────────── Kernel                                          （算結果）
                                   User ◀─────────── Kernel ◀──
```

- Monolithic：系統呼叫進 kernel，存 context，處理完回到同一個（或別的）使用者程序。
- Microkernel：系統呼叫進 kernel，kernel 再呼叫跑在使用者空間的程式，算完送回 kernel，kernel 再回給使用者 → **處理一個系統呼叫的時間變長**。
- 缺點：
  - 每個 kernel 呼叫做的事變少 → 可能需要更多系統呼叫；就算介面不變，context switch 也變多。
  - 每次 context switch 都要存、還原狀態，可能要清 TLB。
  - 老師：一個系統呼叫需要「兩次函式呼叫和兩次模式切換」。
- 「彈性不是單行道，天下沒有白吃的午餐。」

⚠️ 「兩次模式切換」：在上圖的路徑中，使用者 → kernel → server → kernel → 使用者，共有**四次**使用者 / kernel 模式切換（monolithic 只有兩次）。老師的數字可能是指投影片上的另一種算法，逐字稿沒有說明。

**IPC 主導效能**：

- Microkernel 高度依賴 IPC，OS 效能和 IPC 效能強相關。
- microkernel 本身大約和一個**即時執行器（real-time executive）**一樣輕；但看「完成一件工作的整個流程」，可能要在 user / kernel 之間來回好幾趟，再加上使用者層之間的訊息 → 開銷大增。

---

## 4. 對即時系統的意義：deferred processing

[15:00]–[15:30]

- 即時應用要的是**快速周轉、快速處理中斷**：中斷來時只做一點點，其餘丟到使用者空間、之後再排程。
- 這叫 **deferred processing**：top half 在 kernel 裡快速處理；bottom half 做大部分的工作，若跑在使用者空間、優先權較低，即時程序就能先跑。（上一支影片的 DPC、Linux 的 threaded IRQ。）

---

## 5. 【實測】系統呼叫 vs 向使用者模式 server 要服務

模擬兩種架構取得一個服務的成本（client 和 server 綁在同一顆 CPU）：

- Monolithic：服務在 kernel 裡 → 一個系統呼叫（`getppid`）。
- Microkernel 式：服務在另一個使用者程序 → client 用 pipe 送請求、server 讀到後回覆。

```c
for (int i = 0; i < N; i++) syscall(SYS_getppid);                  /* service done inside the kernel */
...
if (fork() == 0) { char c; while (read(req[0], &c, 1) == 1) { c++; write(rsp[1], &c, 1); } _exit(0); }  /* user-mode "server" */
for (int i = 0; i < N; i++) { write(req[1], &c, 1); read(rsp[0], &c, 1); }   /* client -> kernel -> server -> kernel -> client */
```

結果：

```
in-kernel syscall (getppid):         63.8 ns
request to user-mode server (IPC):  2251.0 ns  = 35x
```

- 透過使用者模式 server 要一次服務，比一個系統呼叫貴 **35 倍**：多了兩次 context switch（client ↔ server）、四次系統呼叫、pipe 的緩衝處理。
- 這只是示意：Linux 的 pipe 不是為這種用途最佳化的。真正的 microkernel（例如 QNX）會把 IPC 做成專門、精簡的路徑，差距小得多——但「每次服務多一來一回」的本質代價還是在，這正是老師說的「IPC 主導效能」。

---

## 6. 【課本補充】傳統 Unix 當 RTOS 的問題

（課本 Ch5 §3，p.113–117；上一支影片提到 Unix 不適合即時，這裡整理課本的分析）

> 即時程式設計師用 Unix 時最麻煩的兩個問題：**不可搶占的 kernel**、**動態改變的任務優先權**。

**1. 不可搶占的 kernel**（p.113–114）：

- 系統呼叫：檢查參數 → 執行 **trap（軟體中斷）** → 處理器從 user mode 切到 kernel mode → 執行 kernel 常式。
- Unix 中，**kernel mode 的程序不能被其他程序搶占**（user mode 可以）。所以低優先權程序在做系統呼叫時，高優先權程序也得等它做完 → priority inversion。
- 最長的系統呼叫可能要**幾百毫秒**；最壞搶占時間幾百毫秒，會讓 deadline 只有幾毫秒的任務錯過。
- 為什麼這樣設計：kernel 常式一開始就**關掉所有中斷**，做完才打開。這保護了 kernel 資料結構的完整性，省掉上鎖、解鎖的開銷，**平均**搶占時間較低。最壞情況回應時間可達一秒，當時被認為可以接受——Unix 設計者沒料到會被拿去做即時應用。

**2. 動態優先權**（p.115–116）：

- Unix 用**多層回饋佇列**的 round robin：每個排程點從最高優先權的佇列開始找；每個任務跑一個時間片（Unix 通常是 **1 秒**，可在 system generation 時設定）；沒跑完就被搶占，**重新計算優先權**後放回某個佇列。
- 優先權計算（第 j 個時間片結束時）：

  pri(Tᵢ, j) = Base(Tᵢ) + CPU(Tᵢ, j) + nice(Tᵢ)（式 5.1）

  CPU(Tᵢ, j) = U(Tᵢ, j−1)/2 + CPU(Tᵢ, j−1)/2 = U(Tᵢ, j−1)/2 + U(Tᵢ, j−2)/4 + …（式 5.2、5.3）

  （Unix 中數值越大優先權越低。）最近一個時間片的 CPU 使用率權重最大，越早的越小。
- 用滿時間片 → CPU 值高 → 優先權降低；一開始就因 I/O 阻塞 → 優先權升高。
- Base 把任務分成固定的**優先權帶**：swapper、block I/O、檔案操作、character I/O 與裝置控制、使用者程序（由高到低）；U 和 nice 被刻意限制得很小，任務不會跨帶。
- 動機：I/O 是回應時間的瓶頸，讓 I/O 頻道保持忙碌 → I/O 密集的任務往高優先權移、CPU 密集的往低優先權移；互動使用者得到好的回應時間。這做法後來被幾乎所有通用 OS（包括 Windows）採用。
- 對吞吐量和軟即時很好，但對硬即時不行：優先權被 OS 移動，就不能用 RMA、EDF。

（這也就是上一支影片老師說的「依 CPU 使用因子動態調整優先權」；第 9 支影片的 Linux `SCHED_FIFO` 則是 OS 不會改的靜態優先權。）

**3. 其他缺點**（p.117）：

| 缺點 | 說明 |
|---|---|
| 裝置驅動支援不足 | 原始 Unix System V 的驅動在 kernel mode，加新裝置要把驅動連結進 kernel，要做一次 system generation |
| 沒有即時檔案服務 | 檔案區塊用到才配置 → 寫到一半可能空間不足；寫入慢；同檔案的區塊不連續 → 讀取時間不可預測。即時檔案系統會把檔案連續存放、預先配置空間 |
| 計時器支援不足 | 提供給應用的時鐘解析度是 **10 ms**，對很多硬即時應用太粗 |

---

## 7. 一頁速查表

| 主題 | 重點 |
|---|---|
| Monolithic | 一個程式一個位址空間；快、好懂好除錯；元件不隔離、大、難客製；適合硬體固定的嵌入式 |
| LKM | Linux 執行期擴充，但核心仍是 monolithic |
| Microkernel | kernel 只留中斷、基本記憶體管理、程序、IPC（10 KB–數百 KB）；其他在 user mode；模組化是重點 |
| 好處 | 小、可延伸、隔離可重啟；即時程序可優先於原 kernel 服務 |
| 代價 | 系統呼叫要經過 user-mode server：更多 context switch、IPC 主導效能 |
| 實測 | 系統呼叫 64 ns；經 pipe 向 user-mode server 要服務 2251 ns（35 倍） |
| 課本：Unix 問題 | 不可搶占 kernel（關中斷，最壞數百 ms）；動態優先權（pri = Base + CPU + nice）；驅動、檔案、計時器（10 ms）不足 |

---

## 8. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **Windows：介於 monolithic 和 microkernel 之間；動態優先權** | **第 20 支**：Windows NT as an RTOS |
| **POSIX 即時擴充：解決 Unix 的缺點** | **第 21 支**：POSIX standards |
| **Unix-based RTOS：host-target、preemption point 等（課本 Ch5 §4）** | **第 22 支**：Open Source and Commercial RTOSs |
| **QNX（microkernel）、VxWorks** | **第 22 支**、**第 24 支** |
| **Linux 怎麼變得可搶占** | **第 23 支**：Linux-based RTOSs |

---

## 附：本講一句話總結

> **Monolithic kernel（Unix、Linux）把行程、記憶體、裝置、儲存、I/O 管理和檔案系統全放在同一個 kernel 位址空間，效能好、好懂好除錯，但元件之間沒有隔離、kernel 大又難客製，適合硬體固定的嵌入式系統；microkernel 只在 kernel 留下中斷處理、基本記憶體管理、程序與 IPC（可小到 10 KB），排程器、檔案系統、驅動、計時器等都搬到使用者模式，換來模組化、可延伸、元件可隔離重啟，也讓即時程序有機會優先於原本的 kernel 服務，代價是每個服務都要經過 kernel 轉給使用者模式 server 再轉回來，context switch 變多、效能被 IPC 主導——實測經 pipe 向使用者模式 server 要一次服務比一個系統呼叫貴 35 倍。中斷處理則用 top half 快速處理、bottom half 延後在較低優先權執行。課本補充傳統 Unix 當 RTOS 的兩大問題：kernel 不可搶占（關中斷執行系統呼叫，最壞數百毫秒到一秒）以及依 CPU 使用歷史動態重算優先權（pri = Base + CPU + nice），另外還有驅動、即時檔案服務和 10 ms 計時器解析度的不足。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 20 錄影逐字稿（YouTube 自動字幕，17:38）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch5 §3 整理。實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
