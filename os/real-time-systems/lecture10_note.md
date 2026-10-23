# Real Time Systems 第十支影片完整筆記：Priority Inversion（Lecture 11）

> **課程**：Real Time Systems — Lecture 11：Priority Inversion（播放清單第 10 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（25:11，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch3 §1–2（PDF p.74–77）
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
- [1. CPU 以外的資源](#1-cpu-以外的資源)
  - [1.1 Serially reusable vs non-preemptable](#11-serially-reusable-vs-non-preemptable)
  - [1.2 Critical section](#12-critical-section)
- [2. Priority inversion](#2-priority-inversion)
  - [2.1 Simple（bounded）priority inversion](#21-simplebounded-priority-inversion)
  - [2.2 Unbounded priority inversion](#22-unbounded-priority-inversion)
  - [2.3 用 semaphore 的時間軸例子](#23-用-semaphore-的時間軸例子)
- [3. Mars Pathfinder](#3-mars-pathfinder)
- [4. 怎麼看待兩種 inversion](#4-怎麼看待兩種-inversion)
- [5. 【原始碼】Linux 文件裡的 unbounded priority inversion](#5-原始碼linux-文件裡的-unbounded-priority-inversion)
- [6. 【實測】在一般 Linux 上重現 priority inversion](#6-實測在一般-linux-上重現-priority-inversion)
- [7. 一頁速查表](#7-一頁速查表)
- [8. 與後續講次的連結](#8-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

到目前為止，任務只共享一種資源：CPU，而且可以隨時搶占。這一講加入**其他共享資源**（檔案、資料結構、裝置）：

1. 這些資源**不能搶占** → critical section。
2. 用傳統 OS 的 lock / semaphore 會造成 **priority inversion**（優先權反轉），甚至 **unbounded**（無上限）。
3. 真實案例：**Mars Pathfinder**。
4. 怎麼解決 → 下幾講（PIP、HLP、PCP）。

---

## 1. CPU 以外的資源

### 1.1 Serially reusable vs non-preemptable

[00:00]–[02:30]

- 之前只考慮**一顆 CPU**。CPU 是 **serially reusable（可依序重複使用）**的資源：一個任務跑完換下一個。
- Σ uᵢ ≤ 1 這個條件就建立在兩個前提上：
  1. 同一時間只有一個任務在 CPU 上跑。
  2. 任務**隨時**可以被高優先權任務搶占，**不影響正確性**。
- 實際上任務還會共享：檔案、共享資料結構、其他裝置。這些常是 **non-preemptable（不可搶占）**的：一旦拿到就要用到做完。
- 例：**印表機**。不能印到半頁就搶走：要嘛整頁不印，要嘛整頁印完。很多裝置沒辦法「搶占後再恢復」。

【課本補充】（p.75）

| | Serially reusable（例：CPU） | Non-preemptable（例：檔案、裝置、某些資料結構） |
|---|---|---|
| 獨占使用 | 是 | 是 |
| 使用中可以被搶走、之後再還 | **可以**，不影響結果 | **不行**，否則資源不一致、系統可能失效 |

所以 EDF、RMA 這些「分配 CPU」的演算法，**不能**直接拿來分配不可搶占的資源。

### 1.2 Critical section

[02:30]–[04:00]

- **Critical section（臨界區段，CS）**：存取共享、不可搶占資源的那段程式碼。
- 進去 → 整段做完 → 出來。原則上不會做到一半離開、之後再回來（偶爾有例外）。
- 傳統 OS 的做法：**lock** 或 **semaphore**（semaphore 是 lock 的一般化）。拿到才能進 critical section，結束時釋放。
- 但在即時系統裡這樣做效果不好，會造成 **priority inversion** 和 **unbounded priority inversion**：低優先權任務在跑，高優先權任務卻在等一個遲遲不釋放的資源。

【課本補充】（p.74）現在常把不可搶占資源本身也鬆散地叫做 critical section；但原本 OS 文獻的意思是「以獨占方式使用某個不可搶占資源的**那段程式碼**」。

---

## 2. Priority inversion

### 2.1 Simple（bounded）priority inversion

[04:31]–[07:00]

- 低優先權任務 T_L 在 critical section 裡。
- 高優先權任務 T_H 到達，也要這個資源 → 只能**等** T_L 做完 critical section。
- 等待佇列裡是高優先權任務，等的卻是低優先權任務——這就是 **priority inversion**。
- RMS / EDF 原本不會這樣：高優先權任務一來就能搶占低優先權任務。是 critical section 造成了問題。

⚠️ 老師 [04:31] 說「任務在 critical section 裡**不能被搶占**」。更精確地說：持有資源的任務**不能被搶走資源**，但仍然**可以被搶走 CPU**——下面的 unbounded priority inversion 正是因為 T_L 拿著資源時被中間優先權任務搶走 CPU。老師在 [07:31] 也這樣區分（「從 CPU 搶占，不是從資源」）。

【課本補充】（p.75）simple priority inversion 在多個任務共享不可搶占資源時**常常無法避免**。好在單一 simple inversion 不難處理：它持續的時間**上限**就是低優先權任務獨占該資源的最長時間。只要所有任務都把 critical section 寫得很短，就能靠小心寫程式容忍它。

### 2.2 Unbounded priority inversion

[07:00]–[10:31]

情境：

```
        T_H  ──等──▶ [R] ◀──持有── T_L
                                  ↑ 從 CPU 被搶占
        T_I1, T_I2, T_I3, ...  （中間優先權，不需要 R）
```

1. T_L 拿著資源 R。
2. T_H 到達，要 R → 阻塞，等 T_L。
3. 中間優先權任務 T_I（不需要 R）到達 → 優先權比 T_L 高 → **從 CPU 搶占 T_L**（不是從資源）。
4. T_I 可以有**幾百個**，一個接一個跑。它們是週期任務，跑完還會再來。
5. T_L 拿不到 CPU → 沒辦法釋放 R → T_H 一直等。

老師的例子：最高優先權 T1，最低 T6 拿著資源。T1 來了要資源 → 等。T2–T5（優先權介於中間）陸續到達，全部先跑（投影片藍色部分）。最後 T6 才拿到 CPU、釋放資源，T1 才能跑。

- 高優先權任務經歷的 inversion 次數可能**非常多**，足以讓它錯過 deadline。
- 它唯一的「錯」，就是有個低優先權任務剛好拿著它要的資源。最壞情況是**無限期**等待，不管 deadline 多寬都會錯過。
- 老師的說法 [24:00]：中間的任務在「開派對（enjoy the party）」。

【課本補充】（p.76–77 Fig.2）T1…T5：T5 在 t₀ 鎖住 CR；t₁ 時 T1 到達、搶占 T5；t₂ 時 T1 要 CR → 阻塞，T5 繼續；t₃ 時 T4（不需要 CR）搶占 T5；之後 T3 搶占 T4……T1 可能無限期等下去。

### 2.3 用 semaphore 的時間軸例子

[11:31]–[16:02]

τ₁ 和 τ₃ 共享資料，用 semaphore x 保護。每個任務的程式：

```
local work
P(x)            // wait：等 semaphore 空出來
access shared resource
V(x)            // signal：釋放
local work
```

**只有 τ₁、τ₃（bounded）**：

```
τ1:            ┌local┐ 阻塞 ...........┌CS┐┌local┐
τ3: ┌local┐┌CS─┘     └──CS(續)──┘V(x)  └──────────┴┌local┐
         ↑P(x)     ↑τ1 搶占 τ3        ↑τ1 拿到 x
```

1. τ₃ 做 local work，拿到 x，進 CS。
2. τ₁（高優先權）搶占 τ₃，做 local work，然後要 x → **阻塞**。
3. τ₃ 繼續 CS，做完釋放 x。
4. τ₁ 立刻拿到 x，做 CS、local work，完成。
5. τ₃ 做剩下的 local work。

**加上 τ₂（unbounded）**：步驟 2 之後，中間優先權的 τ₂ 到達，搶占 τ₃，做完它所有的工作；之後 τ₃ 才繼續 CS、釋放 x；τ₁ 才拿到 x。τ₁ 被拖延的時間多了整個 τ₂。如果中間任務不只一個，拖延就更長，τ₁ 很可能錯過 deadline。

---

## 3. Mars Pathfinder

[11:01]、[16:02]–[20:31]

- **1997 年 7 月 4 日**登陸火星。用氣囊彈跳著陸，放出一台探測車，傳回大量資料，包括很多經典的火星全景照片。
- 之後開始出現**整個系統重置（total system reset）**，每次重置都遺失資料。
- 報紙報導成「軟體小故障」、「電腦想同時做太多事」。

技術細節：

| 項目 | 內容 |
|---|---|
| 即時 kernel | **VxWorks**（Wind River Systems） |
| 排程 | **RMA** |
| 共享資源 | **資訊匯流排（information bus）**：一塊共享記憶體，用來在太空船各元件（處理、記憶、儲存、感測）之間傳資訊 |
| 除錯工具 | VxWorks 的 **trace mode**：記錄 context switch、同步物件的使用、中斷等事件 |

除錯過程：

1. JPL（Jet Propulsion Laboratory）工程師在實驗室用**一模一樣的複製品**跑了好幾個小時，重現了重置發生的確切條件。
2. 焦點：VxWorks 的 **mutex** 物件。它有一個布林參數，決定**要不要做 priority inheritance**。這個參數**是關的**。
3. Priority inheritance（上一講提過）：低優先權執行緒擋住高優先權執行緒時，暫時把低優先權執行緒的優先權**拉高**，讓中間的執行緒不能把它換下來。
4. 推論：重置是因為任務錯過 deadline。打開 priority inheritance 就能避免。
5. 上傳一段**很短的 C 程式**，修改這些初始化參數（全域變數），在探測車上執行 → 大致解決問題。

老師：這說明在這種敏感的系統裡，priority inversion 和 unbounded priority inversion 是多嚴重的問題。

⚠️ 老師說程式是在「rover（探測車）」上執行 [20:01]。待補：課本這一版講義沒有 Pathfinder 的內容，無法對照細節。

---

## 4. 怎麼看待兩種 inversion

[20:31]–[24:31]

**Simple priority inversion**：

- 最長持續時間的上限（沒有中間任務時）= 低優先權任務**獨占**資源的最長時間。
- 最壞情況：低優先權任務剛拿到資源，高優先權任務就到達、立刻要資源 → 要等整段。
- 只要這段時間不太長，**可以容忍**：
  - 工程上**限制 critical section 的執行時間**，縮短各任務獨占資源的時間。
  - 小心設計資源、小心寫程式。
  - deadline 和安全餘裕可以把它算進去。「不是什麼大事。」

**Unbounded priority inversion**：

- 完全是另一個等級的問題。
- 中間優先權任務的數量沒有上限，而且是週期任務（跑完還會再來）→ 延遲可能很大 → 高優先權任務錯過 deadline。

老師留下的問題：simple 和 unbounded priority inversion 要怎麼處理？有哪些協定？→ 下幾講。

【課本補充】（p.77）最簡單的避免方法：**不讓中間優先權任務從 CPU 搶占持有資源的低優先權任務** → 把低優先權任務的優先權拉到和等待中的高優先權任務一樣 → 這就是 **Priority Inheritance Protocol（PIP）**，是更複雜協定（HLP、PCP）的基礎。

---

## 5. 【原始碼】Linux 文件裡的 unbounded priority inversion

Linux kernel 的 rt-mutex 設計文件，開頭就用和本講完全一樣的例子：

`Documentation/locking/rt-mutex-design.rst:21`

```
Unbounded Priority Inversion
----------------------------

Priority inversion is when a lower priority process executes while a higher
priority process wants to run.  This happens for several reasons, and
most of the time it can't be helped.  Anytime a high priority process wants
to use a resource that a lower priority process has (a mutex for example),
the high priority process must wait until the lower priority process is done
with the resource.  This is a priority inversion.  What we want to prevent
is something called unbounded priority inversion.  That is when the high
priority process is prevented from running by a lower priority process for
an undetermined amount of time.

The classic example of unbounded priority inversion is where you have three
processes, let's call them processes A, B, and C, where A is the highest
priority process, C is the lowest, and B is in between. A tries to grab a lock
that C owns and must wait and lets C run to release the lock. But in the
meantime, B executes, and since B is of a higher priority than C, it preempts C,
but by doing so, it is in fact preempting A which is a higher priority process.
Now there's no way of knowing how long A will be sleeping waiting for C
to release the lock, because for all we know, B is a CPU hog and will
never give C a chance to release the lock.  This is called unbounded priority
inversion.
```

對照：

| 本講 | Linux 文件 |
|---|---|
| simple inversion「常常無法避免」 | "most of the time it can't be helped" |
| 要防止的是 unbounded | "What we want to prevent is something called unbounded priority inversion" |
| T_H、T_I、T_L | A、B、C |
| 解法 | 文件下一節就是 Priority Inheritance；實作在 `kernel/locking/rtmutex.c`（"RT-Mutexes: simple blocking mutual exclusion locks with PI support"） |

使用者空間的 `pthread_mutexattr_setprotocol(PTHREAD_PRIO_INHERIT)` 會走 PI futex，最後用到 kernel 的 rt_mutex。這相當於 Pathfinder 上那個「要不要做 priority inheritance」的布林參數。

---

## 6. 【實測】在一般 Linux 上重現 priority inversion

本機一般使用者不能設 `SCHED_FIFO`（`ulimit -r` = 0）。改用 CFS 的 **nice 值**當「優先權」，把所有執行緒綁在**同一顆 CPU**：

- **L**：nice 19（低），先拿 mutex，在 critical section 裡做 20 ms 的 CPU 工作。
- **H**：nice 0（高），5 ms 後要同一個 mutex。
- **K 個中間任務**：nice 0，純 CPU 迴圈，**不碰** mutex。

```c
static void *low(void *a) {
  nice_me(19); pthread_mutex_lock(&m);
  double c0 = cpu_ms(); while (cpu_ms() - c0 < 20) ;          /* 20 ms of CPU inside the critical section */
  pthread_mutex_unlock(&m); return 0; }
static void *hog(void *a) { nice_me(0); while (!stop) ; return 0; }
...
  double t0 = now(); pthread_mutex_lock(&m); double w = now() - t0;
```

結果：

```
K=0 intermediates  plain mutex   H waited     15.0 ms
K=1 intermediates  plain mutex   H waited    763.0 ms
K=2 intermediates  plain mutex   H waited   1927.0 ms
K=4 intermediates  plain mutex   H waited   3742.3 ms
K=1 intermediates  PRIO_INHERIT  H waited    981.3 ms
K=4 intermediates  PRIO_INHERIT  H waited   3619.0 ms
```

看到什麼：

- **K = 0**：H 只等 L 做完剩下的 critical section（20 − 5 = 15 ms）→ **simple / bounded** inversion，上限就是 critical section 長度。
- **K ≥ 1**：中間任務不需要 mutex，卻讓 H 的等待暴增到 0.76 s、1.9 s、3.7 s，**隨 K 線性成長**——**unbounded** priority inversion。原因：L 的 nice 19 權重只有 15，nice 0 是 1024；L 拿著鎖卻幾乎分不到 CPU。
- 和即時排程不同的是：CFS 是公平排程，L 還是會分到一點點 CPU，所以等待是「很長但有限」；在 `SCHED_FIFO` 下，只要中間任務一直有工作，L 就**完全**拿不到 CPU，H 真的會無限期等。
- **PRIO_INHERIT 沒有幫助**（0.98 s、3.6 s，和 plain 差不多）。原因在原始碼：PI 只改 `p->prio`，但 CFS 的權重是從 `p->static_prio` 算的——

  `kernel/sched/core.c:1260`

  ```c
  static void set_load_weight(struct task_struct *p, bool update_load)
  {
  	int prio = p->static_prio - MAX_RT_PRIO;
  	struct load_weight *load = &p->se.load;
  ```

  而 `rt_mutex_setprio()`（`kernel/sched/core.c:6980`）只在繼承來的優先權是 RT 或 DL 時才真的換排程類別；兩邊都是 CFS 任務時，L 的權重不變。所以 **priority inheritance 只對即時（SCHED_FIFO / RR / DEADLINE）任務有意義**。OS 課（`../linux-os/` 第 28 講）有 PRIO_INHERIT / PRIO_PROTECT 的 strace 觀察和 rt_mutex 原始碼。

---

## 7. 一頁速查表

| 名詞 | 意思 |
|---|---|
| Serially reusable 資源 | 獨占、但可隨時搶占再還（CPU） |
| Non-preemptable 資源 | 獨占、用到一半不能搶走（印表機、檔案、資料結構） |
| Critical section | 存取不可搶占資源的程式碼，用 lock / semaphore 保護 |
| Simple priority inversion | T_H 等 T_L 釋放資源；上限 = T_L 的 CS 長度；可靠短 CS 容忍 |
| Unbounded priority inversion | T_H 等 T_L，T_L 又被不需要資源的中間任務從 **CPU** 搶占；延遲無上限 |
| Mars Pathfinder（1997） | VxWorks ＋ RMA；資訊匯流排的 mutex 沒開 priority inheritance → 系統重置；上傳 C 程式改參數修好 |
| 解法方向 | 讓持有資源的低優先權任務暫時繼承高優先權 → PIP（下一講） |

實測（CFS、nice、單 CPU）：0 個中間任務等 15 ms；4 個等 3.7 s。PRIO_INHERIT 對 CFS 任務無效（權重看 `static_prio`）。

---

## 8. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **Priority inheritance、HLP** | **第 11 支**：PIP and HLP Protocols |
| **PIP 的死結、連鎖阻塞問題** | **第 11–12 支** |
| **Priority ceiling** | **第 12–13 支**：PCP 與其分析 |
| **VxWorks、商用 RTOS 對 PIP 的支援** | **第 22 支**、**第 24 支** |
| **POSIX 的 PRIO_INHERIT / PRIO_PROTECT** | **第 21 支**：POSIX standards |
| **Linux rt_mutex、PREEMPT_RT** | **第 23 支**：Linux-based RTOSs |

---

## 附：本講一句話總結

> **CPU 是可以隨時搶占再還的 serially reusable 資源，但檔案、資料結構、印表機這類資源用到一半不能被搶走，必須放在用 lock 或 semaphore 保護的 critical section 裡；傳統做法在即時系統會造成 priority inversion——高優先權任務 T_H 等著低優先權任務 T_L 釋放資源。若只有這兩個任務，等待上限就是 T_L 的 critical section 長度，靠縮短 critical section 就能容忍；但若有不需要該資源的中間優先權任務從 CPU 把 T_L 搶走，T_H 就會無上限地等下去（unbounded priority inversion）。1997 年 Mars Pathfinder 在 VxWorks ＋ RMA 下反覆重置，就是因為資訊匯流排的 mutex 沒開 priority inheritance，JPL 在地面複製品上重現後上傳一小段 C 程式打開它才解決。實測在一般 Linux 上用 nice 也能重現：沒有中間任務時 H 只等 15 ms，有 4 個中間任務時等了 3.7 s；而 PRIO_INHERIT 對 CFS 任務無效，因為 CFS 權重來自 static_prio，priority inheritance 只對即時排程類別有意義。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 11 錄影逐字稿（YouTube 自動字幕，25:11）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch3 §1–2 整理。原始碼為 Linux v6.2.12；實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
