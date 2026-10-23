# Operating Systems: A Linux Kernel-Oriented Approach 第二十六講完整筆記：Linux 的排程與 CFS（Chapter 5, Part IX）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 26（Chapter 5, Part IX：Linux scheduling）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（56:13，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§5.4.5–5.4.6（p.245–255）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
> **標記說明**：
> - `【口頭】`：教授在課堂上口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與教授回答
> - `【課本補充】`：課本有、但這堂課沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（`檔名:行號`），用來對照觀念
> - `【實測】`：在本機（Ubuntu、kernel 6.5.0、x86-64、20 個 CPU）實際執行的結果
> - `⚠️ 逐字稿缺字`：自動字幕在這裡掉字或聽不清，內容依上下文或課本補上；也用來標示課堂或課本說法需要修正之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 這堂課要做什麼](#0-這堂課要做什麼)
- [1. `schedule()`：排程的入口](#1-schedule排程的入口)
  - [1.1 【原始碼】`schedule()`](#11-原始碼schedule)
  - [1.2 什麼時候會進來](#12-什麼時候會進來)
- [2. 排程類別](#2-排程類別)
  - [2.1 五個類別，嚴格的優先順序](#21-五個類別嚴格的優先順序)
  - [2.2 【原始碼】`struct sched_class`：用 C 寫物件導向](#22-原始碼struct-sched_class用-c-寫物件導向)
- [3. Run queue](#3-run-queue)
- [4. `task_struct` 裡跟排程有關的欄位](#4-task_struct-裡跟排程有關的欄位)
  - [4.1 `sched_entity`、`sched_class`](#41-sched_entitysched_class)
  - [4.2 Task group / cgroup：兩層的排程](#42-task-group--cgroup兩層的排程)
  - [4.3 ⚠️ `core_node` 與 `core_cookie`](#43-️-core_node-與-core_cookie)
- [5. CFS 的核心：vruntime](#5-cfs-的核心vruntime)
  - [5.1 高優先權的時鐘走得慢](#51-高優先權的時鐘走得慢)
  - [5.2 【原始碼】權重表](#52-原始碼權重表)
  - [5.3 【原始碼】`update_curr` 與 `calc_delta_fair`](#53-原始碼update_curr-與-calc_delta_fair)
  - [5.4 紅黑樹：挑 vruntime 最小的](#54-紅黑樹挑-vruntime-最小的)
- [6. 只挑最小的不夠：排程週期與時間片](#6-只挑最小的不夠排程週期與時間片)
  - [6.1 問題：變成每個 jiffy 都換人](#61-問題變成每個-jiffy-都換人)
  - [6.2 排程週期](#62-排程週期)
  - [6.3 時間片](#63-時間片)
  - [6.4 【原始碼】`__sched_period` 與 `sched_slice`](#64-原始碼__sched_period-與-sched_slice)
  - [6.5 時間片用完才叫排程器](#65-時間片用完才叫排程器)
- [7. CFS = 以 vruntime 為單位的 round robin](#7-cfs--以-vruntime-為單位的-round-robin)
- [8. 防止鑽漏洞：`min_vruntime`](#8-防止鑽漏洞min_vruntime)
  - [8.1 新 task、睡很久的 task](#81-新-task睡很久的-task)
  - [8.2 fork](#82-fork)
  - [8.3 遷移](#83-遷移)
  - [8.4 【原始碼】實際的規則比課本細](#84-原始碼實際的規則比課本細)
- [9. 【實測】nice 0 vs nice 5](#9-實測nice-0-vs-nice-5)
- [10. 計算 CPU 負載：PELT](#10-計算-cpu-負載pelt)
- [11. 一頁速查表](#11-一頁速查表)
- [12. 與後續講次的連結](#12-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

[00:33] 看 Linux kernel 實際用的排程演算法：

1. 入口 `schedule()`。
2. 排程類別（scheduling class）的階層。
3. Run queue。
4. 重點：**CFS（Completely Fair Scheduler，完全公平排程器）**：vruntime、排程週期、時間片、防作弊。
5. 計算 CPU 負載（PELT）。

⚠️ 本機 kernel 6.5 仍然是 CFS。從 **Linux 6.6** 起，CFS 的挑選規則換成 **EEVDF**（Earliest Eligible Virtual Deadline First），但 vruntime、權重、紅黑樹這些概念都還在。本筆記照 v6.2.12。

---

## 1. `schedule()`：排程的入口

### 1.1 【原始碼】`schedule()`

[00:33] 核心排程函式在 `kernel/sched/core.c`：

`kernel/sched/core.c:6681-6693`

```c
asmlinkage __visible void __sched schedule(void)
{
	struct task_struct *tsk = current;

	sched_submit_work(tsk);
	do {
		preempt_disable();
		__schedule(SM_NONE);
		sched_preempt_enable_no_resched();
	} while (need_resched());
	sched_update_worker(tsk);
}
```

| 行 | 說明 |
|---|---|
| 6683 | [00:33] 拿 `current`（第 7–8 講花了很多力氣定義的那個巨集） |
| 6685 | [01:00] `sched_submit_work`：把工作**分出去**。例如目前 task 要睡了，還有沒做完的 I/O 或要交給 worker thread 的工作，現在交出去 |
| 6687 | [02:01] **關搶占**：排程本身是敏感的操作 |
| 6688 | 呼叫內部的 `__schedule()`（兩個底線開頭 = 檔案內部用） |
| 6689 | 開搶占，但**不**在這裡再觸發排程 |
| 6690 | [02:32] 如果又需要重新排程，就再做一次 |
| 6691 | [02:32] `sched_update_worker`：更新 workqueue / worker thread 的狀態（第 16 講） |

【課本補充】課本：kernel 有一組 **kworker** thread 做大部分低優先權的工作。目前 task 要阻塞時，喚醒 worker thread 並分派工作特別重要。

⚠️ [02:01] 教授說「再把**中斷**打開」，應是「把**搶占**打開」。

### 1.2 什麼時候會進來

[03:00]–[04:01]

1. 使用者 process 做完系統呼叫，返回前。
2. [03:31] kernel thread 在 mutex、semaphore 上**阻塞**：被擋住的那一刻就呼叫排程器（第 20–22 講）。
3. 從中斷或系統呼叫返回時，發現 **`TIF_NEED_RESCHED`** 旗標被設了：表示需要叫排程器決定下一個是誰。
4. [04:01] 新 process 進來搶占別人、別的核心遷移過來的 IPI……很多事件都會觸發。

【課本補充】課本強調：排程器**不是每個 timer 中斷都呼叫**，只有目前的 task 用完它的配額才會（§6.5）。

---

## 2. 排程類別

### 2.1 五個類別，嚴格的優先順序

[04:31] 每顆 CPU 一個 run queue（`struct rq`），裡面其實包了好幾個子 run queue。

[05:01] 排程類別，由高到低（上一講筆記 §10.6 看過 linker script 的順序）：

| 類別 | 用途 |
|---|---|
| **stop** | [05:31] 緊急的 kernel 工作，「停下一切，先跑這個」 |
| **dl**（deadline） | [05:31] 有截止時間的即時 task。[06:00] 例如影音處理：60 Hz 影片每格要在 16.66 ms 內處理完；30 Hz 是 33 ms |
| **rt**（real-time） | [06:32] threaded IRQ、關鍵任務的軟體。[07:31] 用 FIFO 或 round robin，很簡單 |
| **fair**（CFS） | [07:31] **所有一般 task**。本講的重點 |
| **idle** | [08:02] 沒事做時跑。讓所有時間加起來是 100%：沒東西跑的那 50% 就記在 idle process 帳上。教授說這是「有點狡猾的記帳」 |

⚠️ [06:32]–[07:03] 教授先說 softirq 是即時類別，馬上更正：softirq 在 atomic（中斷）context 裡跑，在排程類別**之外**，優先權比所有排程類別都高。threaded IRQ（第 16 講）才是 rt 類別。

[14:31]–[15:02] **嚴格的優先順序**：只要上層類別有 task，就不看下層。

- 教授說這可以說是優點，也可以說是缺點：如果某個 kernel 工作剛好在 CFS 類別，它會被排得很晚，系統可能不穩定。
- 【口頭】[14:31] 教授說他聽說很新的 kernel 稍微調整了這點，但沒有百分之百確認。

【課本補充】課本：類別之間**沒有公平性**。DL task 可能霸占 CPU，連 RT task 都跑不了，系統會崩潰。要靠使用者保留足夠的運算頻寬給一般 task 和 RT task。

⚠️ 課本說 stop 類別用在「kernel panic 或新增 CPU」。它實際上用在 `stop_machine()`、把 task 搬到別顆 CPU（migration）、CPU 熱插拔等需要「這顆 CPU 上什麼都別跑」的操作，每顆 CPU 一個 `migration/N` thread。

### 2.2 【原始碼】`struct sched_class`：用 C 寫物件導向

[08:32] 排程類別就是 C++ 類別在 C 裡的對應：一個 struct 加一堆**函式指標**。

`kernel/sched/sched.h:2164-2188`（節錄）

```c
struct sched_class {
	...
	void (*enqueue_task) (struct rq *rq, struct task_struct *p, int flags);
	void (*dequeue_task) (struct rq *rq, struct task_struct *p, int flags);
	void (*yield_task)   (struct rq *rq);
	...
	void (*check_preempt_curr)(struct rq *rq, struct task_struct *p, int flags);

	struct task_struct *(*pick_next_task)(struct rq *rq);
	...
	struct task_struct * (*pick_task)(struct rq *rq);

	void (*migrate_task_rq)(struct task_struct *p, int new_cpu);
	...
```

| 函式指標 | 做什麼 |
|---|---|
| `enqueue_task` / `dequeue_task` | [09:02] 把 task 放進 / 拿出 run queue |
| `pick_task` / `pick_next_task` | [09:32] 挑「最值得」跑的 task |
| `migrate_task_rq` | [09:32] 搬到別的 queue（通常是別顆核心） |
| `update_curr` | [09:32] 更新目前 task 的統計（例如它已經跑多久） |

[10:02] 換掉這些函式指標，行為就變了，就是另一種排程器。

CFS 的實例：

`kernel/sched/fair.c:12447-12458`

```c
DEFINE_SCHED_CLASS(fair) = {

	.enqueue_task		= enqueue_task_fair,
	.dequeue_task		= dequeue_task_fair,
	.yield_task		= yield_task_fair,
	.yield_to_task		= yield_to_task_fair,

	.check_preempt_curr	= check_preempt_wakeup,

	.pick_next_task		= __pick_next_task_fair,
	.put_prev_task		= put_prev_task_fair,
	.set_next_task          = set_next_task_fair,
```

[10:30] 每個排程器在自己的檔案：

| 排程器 | 檔案 |
|---|---|
| stop | `kernel/sched/stop_task.c` |
| deadline | `kernel/sched/deadline.c` |
| real-time | `kernel/sched/rt.c` |
| CFS | `kernel/sched/fair.c` |
| idle | `kernel/sched/idle.c` |

⚠️ 教授和課本都說 CFS 在 `cfs.c`。實際檔名是 **`fair.c`**（沒有 `cfs.c` 這個檔案）。

【課本補充】`pick_task` 是快速路徑；`pick_next_task` 是慢速路徑，會考慮 cgroup（§4.2）。

【口頭】[17:31] 教授說：如果你要加一個新排程器（例如作業），就要定義一個新的排程類別，只有這個辦法。

---

## 3. Run queue

[11:31] run queue 是存放所有待排程 `task_struct` 的資料結構，排程器從裡面挑一個。

上一講筆記 §1.2 看過 `struct rq`（`kernel/sched/sched.h:954`）。重點欄位：

| 欄位 | 說明 |
|---|---|
| `__lock` | [11:31] run queue 會被很多 kernel thread（甚至跨核心）同時存取，要一把鎖。課本稱這種保護整個結構的鎖為 **monitor lock** |
| `cpu` | [12:00] 這個 rq 屬於哪顆 CPU |
| `nr_running` | 可執行的 task 數 |
| `nr_switches` | 換了幾次 task。大致代表這顆核心有多忙 |
| `cfs`, `rt`, `dl` | [12:32] 三個子 run queue |
| `curr`, `idle` | [13:04] 目前的 task、idle task |
| `prev_mm` | [13:04] 第 13 講 context switch 時看過：kernel thread 借用上一個使用者 process 的 `mm_struct` |
| `core_pick` | [13:31] 這次排程選中的 task |

---

## 4. `task_struct` 裡跟排程有關的欄位

### 4.1 `sched_entity`、`sched_class`

[16:01] 從**每個 process** 的角度，排程器要用哪些欄位？課本 Listing 5.37（`include/linux/sched.h`）：

```c
struct sched_entity         se;          /* CFS 的統計 */
struct sched_rt_entity      rt;          /* RT 的 */
struct sched_dl_entity      dl;          /* deadline 的 */
const struct sched_class    *sched_class; /* 這個 task 屬於哪個排程類別 */
struct rb_node              core_node;
unsigned long               core_cookie;
```

[16:01] 最重要的是 `se`（sched entity）：一堆統計，跑了多久、換了幾次、遷移幾次…… [16:32] 排程靠的就是統計：你用了比較多 CPU，統計上會看出來，就該對別人公平一點。

[17:00] `sched_class` 指向這個 task 屬於哪個排程類別，這樣才知道去找哪個排程器。

`struct sched_entity`（課本 Listing 5.38，`include/linux/sched.h`）：

```c
struct sched_entity {
    struct load_weight   load;              /* 權重 */
    struct rb_node       run_node;          /* 紅黑樹節點 */
    u64                  exec_start;        /* 這次從什麼時候開始跑 */
    u64                  sum_exec_runtime;  /* 總共跑了多久 */
    u64                  vruntime;          /* 虛擬執行時間：最重要 */
    u64                  prev_sum_exec_runtime;
    u64                  nr_migrations;
    struct sched_avg     avg;               /* 平均負載（PELT） */
    struct cfs_rq       *cfs_rq;
};
```

[22:02]–[23:32] `run_node` 跟 `list_head` 同一個做法：紅黑樹節點**嵌在**結構裡面，只指向其他紅黑樹節點。要拿回外層的 `sched_entity`，用 `container_of`（第 14 講）。這樣就是一棵**通用的**紅黑樹。

[24:01] 統計很多，但**真正用來決定的只有 vruntime**。其他統計是用來調整 vruntime 的。

### 4.2 Task group / cgroup：兩層的排程

[18:31] Linux 有很多種「一群 task」：session、process group（第 9 講）、多執行緒 process（一群 thread）。每個 thread 在排程上都是獨立的。

[19:01] Linux 還支援 **task group**（也叫 **cgroup**），可以做**階層式**排程：

```
                  頂層紅黑樹
          ┌──────────┼──────────┐
       task A     cgroup X    cgroup Y        ← cgroup 在頂層看起來是「一個節點」
                  ┌──┴──┐     ┌──┼──┐
                 t1    t2    t3 t4 t5         ← cgroup 內部再自己排
```

[19:31] cgroup 裡任何 task 跑，頂層看到的是這個 cgroup 的執行時間在累加。這對**容器（container）**很重要。

【口頭】[20:00] 教授的比喻：大家共用一個銀行帳戶，誰花錢都從這個帳戶扣；而這家銀行在中央銀行也有帳戶。我花錢，我的帳戶減少，我的銀行的帳戶也減少，中央銀行都記下來。

[20:31] 頂層可以說「這個 cgroup 50%、那個 50%」；每個 cgroup 裡再用自己的策略分那 50%。

[21:00] 教授說：Linux 用兩層的階層式排程，但本講只看頂層，讓事情簡單。

【課本補充】課本：cgroup 被當成**一個**有單一 vruntime 的可排程實體，所以 process 不能在 cgroup 裡一直開新 process 來霸占 CPU。cgroup 還可以再有子 cgroup。

### 4.3 ⚠️ `core_node` 與 `core_cookie`

[17:31] 教授說不深入講。課本的說法：

- `core_node` 指向 process 的「**家核心**」，預設在那裡排程，記憶體也配置在附近，對 NUMA 很重要。
- `core_cookie` 標示一組彼此「互相安全」的 task，可以排在同一顆核心上；不相干的 task 排在同一顆核心上，可能透過架構的**側通道**偷秘密。

⚠️ `core_cookie` 的說法對；`core_node` 不對。兩者都屬於 **core scheduling**（`CONFIG_SCHED_CORE`）：在有 SMT（hyper-threading）的 CPU 上，**同一顆實體核心的兩個硬體 thread** 只能同時跑 cookie 相同的 task，防止 L1TF、MDS 這類側通道攻擊。`core_node` 是把 task 依 cookie 掛進每個 rq 的另一棵紅黑樹（`rq->core_tree`）用的節點，**跟 NUMA 的家核心無關**。NUMA 的偏好節點是另外的欄位（`numa_preferred_nid`）。

---

## 5. CFS 的核心：vruntime

### 5.1 高優先權的時鐘走得慢

[25:30]–[26:33] 挑下一個 task 時，先更新目前 task 的統計，然後算 **vruntime（virtual runtime，虛擬執行時間）**：

- 本次執行的長度 = 現在 − 上次 context switch 後開始跑的時間。
- **高優先權的 task**：跑了 10 ms，只算它 3 ms。
- **低優先權的 task**：跑了 10 ms，算它 30 ms。

[26:33] 然後永遠挑 **vruntime 最小**的跑。這樣高優先權的 task 看起來「沒跑多少」，就常被挑到。教授：「這樣**同時尊重了優先權，又公平**。」

公式（課本式 5.4）：

```
              weight(nice = 0)
δvruntime = δ × ────────────────
               weight(nice)
```

[27:00] `weight(nice = 0)` = 1024，也就是預設優先權 120 時的權重。

[27:31] nice 值越高 = 優先權越低（第 7 講），所以權重越小，vruntime 長得越快。

### 5.2 【原始碼】權重表

`kernel/sched/core.c:11338-11347`

```c
const int sched_prio_to_weight[40] = {
 /* -20 */     88761,     71755,     56483,     46273,     36291,
 /* -15 */     29154,     23254,     18705,     14949,     11916,
 /* -10 */      9548,      7620,      6100,      4904,      3906,
 /*  -5 */      3121,      2501,      1991,      1586,      1277,
 /*   0 */      1024,       820,       655,       526,       423,
 /*   5 */       335,       272,       215,       172,       137,
 /*  10 */       110,        87,        70,        56,        45,
 /*  15 */        36,        29,        23,        18,        15,
};
```

[28:02]–[29:30] 教授的例子：

| nice | 權重 | 跑 1 ms，vruntime 加 |
|---|---|---|
| 0 | 1024 | 1 ms |
| +5 | 335 | 1024/335 ≈ **3 ms** |
| −5 | 3121 | 1024/3121 ≈ **0.33 ms** |
| +19 | 15 | 1024/15 ≈ 68 ms |
| −20 | 88761 | 約 1/87 ms |

- [28:31] 每差 1 級，權重大約差 **1.25 倍**（教授說 20–25%），呈指數下降。
- [29:30] 範圍非常大（88761 / 1024 ≈ 87 倍），所以 nice 值要**非常謹慎**地用。
- 為什麼是 1.25 倍？每降一級 nice，CPU 份額大約多 10%（兩個 task 時，權重比 1.25 → 份額約 55% : 45%）。表是用整數，所以有一點誤差。

[30:01] 更新：`curr.vruntime += δvruntime`。

### 5.3 【原始碼】`update_curr` 與 `calc_delta_fair`

`kernel/sched/fair.c:897-924`（節錄）

```c
static void update_curr(struct cfs_rq *cfs_rq)
{
	struct sched_entity *curr = cfs_rq->curr;
	u64 now = rq_clock_task(rq_of(cfs_rq));
	u64 delta_exec;
	...
	delta_exec = now - curr->exec_start;
	if (unlikely((s64)delta_exec <= 0))
		return;

	curr->exec_start = now;
	...
	curr->sum_exec_runtime += delta_exec;
	...
	curr->vruntime += calc_delta_fair(delta_exec, curr);
	update_min_vruntime(cfs_rq);
```

| 行 | 說明 |
|---|---|
| 906 | 本次跑了多久 = 現在 − `exec_start`（奈秒） |
| 910 | 重設起點 |
| 920 | 真實時間累加到 `sum_exec_runtime` |
| 923 | **vruntime 加上換算後的時間** |
| 924 | 順便更新這個 queue 的 `min_vruntime`（§8） |

`kernel/sched/fair.c:709-715`

```c
static inline u64 calc_delta_fair(u64 delta, struct sched_entity *se)
{
	if (unlikely(se->load.weight != NICE_0_LOAD))
		delta = __calc_delta(delta, NICE_0_LOAD, &se->load);

	return delta;
}
```

就是式 5.4：`delta × NICE_0_LOAD / weight`。nice 0 的 task 直接用真實時間（最常見，所以標成 `unlikely` 的是另一邊）。`__calc_delta` 用定點數乘法和位移，避免除法。

### 5.4 紅黑樹：挑 vruntime 最小的

[17:31] [31:03] CFS 的 run queue 是一棵**以 vruntime 為鍵的紅黑樹**。紅黑樹是搜尋樹，**最左邊**的節點就是最小的。

`kernel/sched/fair.c:653-661`

```c
struct sched_entity *__pick_first_entity(struct cfs_rq *cfs_rq)
{
	struct rb_node *left = rb_first_cached(&cfs_rq->tasks_timeline);

	if (!left)
		return NULL;

	return __node_2_se(left);
}
```

- `rb_first_cached`：這棵樹另外**快取**了最左節點，所以拿最小值是 **O(1)**，不用每次往左走到底。插入、刪除是 O(log n)。
- `__node_2_se` 就是 `container_of`，從 `rb_node` 拿回 `sched_entity`。

---

## 6. 只挑最小的不夠：排程週期與時間片

### 6.1 問題：變成每個 jiffy 都換人

[30:01]–[32:30] 教授指出：五個 task 都從 vruntime 0 開始。

- 不管優先權多高，**誰先跑，誰的 vruntime 就比其他人大**（其他人還是 0）。
- 下一個 jiffy（例如 1 ms）就換別人……
- 結果每 1 ms 換一次，等於 **round robin**，優先權根本沒發揮作用，而且換得太頻繁。

### 6.2 排程週期

[33:01] 解法：**不要每個 jiffy 都檢查要不要排程**。

[33:01] 定義一段夠長的時間叫**排程週期（scheduling period）**，Linux 內部叫 `sysctl_sched_latency`：

- [33:31] 在一個週期內，**每個 task 至少跑一次**。
- [34:03] 依**權重比例**切分這段時間：高優先權的拿長一點、低的拿短一點。
- 被選中的 task，如果沒有系統呼叫、沒有中斷，就讓它跑完自己那一段。

[34:31] 三個參數（課本的記號）：

| 參數 | 記號 | 意思 |
|---|---|---|
| `sysctl_sched_latency` | SP | [34:31] 排程週期 |
| `sched_nr_latency` | N | [35:00] 一個週期最多考慮幾個 task。教授說單位其實是「task 數」，不是時間 |
| `sysctl_sched_min_granularity` | G | [35:30] 一個 task 在一個週期裡**至少**跑多久（粒度）。至少做點事，不要只跑半微秒 |

[36:31]–[38:32] 兩種情況：

- **一般情況**（task 數 ≤ N）：週期 = SP。
- **極端情況**（task 數 > N）：系統被 task 淹沒了。週期 = task 數 × G，切成很多小條，**不管優先權**每人一條。教授說這應該不太可能發生。

### 6.3 時間片

[38:32] 每個 task 的**時間片（slice）**（課本式 5.5）：

```
               weight(task_i)
slice_i = SP × ───────────────
               Σ_j weight(task_j)
```

權重高 → 時間片大。

### 6.4 【原始碼】`__sched_period` 與 `sched_slice`

`kernel/sched/fair.c:725-731`

```c
static u64 __sched_period(unsigned long nr_running)
{
	if (unlikely(nr_running > sched_nr_latency))
		return nr_running * sysctl_sched_min_granularity;
	else
		return sysctl_sched_latency;
}
```

跟課本一模一樣。預設值：

`kernel/sched/fair.c:72, 93, 107`

```c
unsigned int sysctl_sched_latency			= 6000000ULL;
...
unsigned int sysctl_sched_min_granularity			= 750000ULL;
...
static unsigned int sched_nr_latency = 8;
```

6 ms、0.75 ms、8 個。但這是「一顆 CPU」的基準值，開機時會依 CPU 數放大：

`kernel/sched/fair.c:253-272`（節錄）

```c
static unsigned int get_update_sysctl_factor(void)
{
	unsigned int cpus = min_t(unsigned int, num_online_cpus(), 8);
	unsigned int factor;

	switch (sysctl_sched_tunable_scaling) {
	...
	case SCHED_TUNABLESCALING_LOG:
	default:
		factor = 1 + ilog2(cpus);
		break;
	}

	return factor;
}
```

本機 20 顆 CPU → `min(20, 8) = 8` → `1 + log₂8 = 4` 倍：**排程週期 24 ms、最小粒度 3 ms**。（CPU 多時，每顆 CPU 換 task 的成本比較不重要，就用長一點的週期。）

`kernel/sched/fair.c:741-780`（節錄）

```c
static u64 sched_slice(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	unsigned int nr_running = cfs_rq->nr_running;
	...
	slice = __sched_period(nr_running + !se->on_rq);

	for_each_sched_entity(se) {
		...
		qcfs_rq = cfs_rq_of(se);
		load = &qcfs_rq->load;
		...
		slice = __calc_delta(slice, se->load.weight, load);
	}

	if (sched_feat(BASE_SLICE)) {
		...
			min_gran = sysctl_sched_min_granularity;

		slice = max_t(u64, slice, min_gran);
	}

	return slice;
}
```

| 部分 | 說明 |
|---|---|
| `__sched_period(...)` | 先算週期 |
| `__calc_delta(slice, se->load.weight, load)` | 乘上「我的權重 / queue 的總權重」：就是式 5.5 |
| `for_each_sched_entity` | 有 cgroup 時，**每一層**都乘一次比例（§4.2 的兩層排程）。沒有 cgroup 時只跑一次 |
| `max_t(..., min_gran)` | 時間片**不小於**最小粒度 |

### 6.5 時間片用完才叫排程器

[39:02]–[41:01]

- task 開始跑後，**時間片用完之前不叫排程器**。
- 中途被中斷（例如阻塞了），下次再跑時跑**剩下的**部分。
- 中間很多 timer 中斷會來，但它們做別的事，**不**叫排程器。
- [40:31] 時間片用完了，才設 **`TIF_NEED_RESCHED`**。[41:01] 排程器跑起來，又挑 vruntime 最小的。

每個 tick 檢查：

`kernel/sched/fair.c:4939-4956`

```c
static void
check_preempt_tick(struct cfs_rq *cfs_rq, struct sched_entity *curr)
{
	unsigned long ideal_runtime, delta_exec;
	struct sched_entity *se;
	s64 delta;

	ideal_runtime = sched_slice(cfs_rq, curr);
	delta_exec = curr->sum_exec_runtime - curr->prev_sum_exec_runtime;
	if (delta_exec > ideal_runtime) {
		resched_curr(rq_of(cfs_rq));
		/*
		 * The current task ran long enough, ensure it doesn't get
		 * re-elected due to buddy favours.
		 */
		clear_buddies(cfs_rq, curr);
		return;
	}
```

| 行 | 說明 |
|---|---|
| 4946 | 我的時間片 |
| 4947 | 這次被選上之後跑了多久（`prev_sum_exec_runtime` 是被選上那一刻的總執行時間） |
| 4948-4949 | 超過了 → `resched_curr`：設 `TIF_NEED_RESCHED`。從中斷返回時就會呼叫 `schedule()`（§1.2） |

【課本補充】課本 Listing 5.41（`hrtick_start_fair`）：開了高解析度 tick 時，直接算「還剩多少」（`slice − ran`），設一個高解析度計時器在**剛好用完**時叫醒；已經用完就馬上 `resched_curr`。

---

## 7. CFS = 以 vruntime 為單位的 round robin

[41:31]–[43:01] 沒有時間片時，大家輪流跑一個 jiffy，就退化成 round robin。

[43:30]–[45:01] 有時間片時，算一下（課本式 5.6）：

```
slice ∝ weight(T)                      （式 5.5）
δvruntime ∝ slice / weight(T)          （式 5.4，δ = slice）
⟹ δvruntime = 常數
```

[45:01] **每個 task 跑完一個時間片，vruntime 都增加一樣多**，跟優先權無關。

[45:30]–[46:30] 所以 CFS 其實**還是 round robin**，只是：

- 輪的單位是 **vruntime**，不是真實時間。
- 教授：就像每個 task 有**不同的時鐘**。低優先權的時鐘走得快，高優先權的走得慢。
- 真實時間上，大家跑的長度不同（依優先權）；vruntime 上，大家跑的一樣多。
- [46:30] 這同時保證了**公平**和**優先權**。

（課本 Figure 5.30：四個同優先權的 task T1–T4，就是一輪一輪的 T1 T2 T3 T4。不同優先權時，把 y 軸換成 vruntime，還是這個樣子。）

[47:01] 但現實沒那麼規矩：有系統呼叫、阻塞，task 不一定跑完時間片。沒跑完的，vruntime 增加得少一點，下個週期就有點優勢。[47:32] 目標是讓所有 task 的 vruntime **一起往前走**。

---

## 8. 防止鑽漏洞：`min_vruntime`

[47:32] 每個 CFS run queue 維護一個 **`min_vruntime`**：大致是這個 queue 裡最小的 vruntime。

### 8.1 新 task、睡很久的 task

[48:00]–[49:01] 問題：新 task 的 vruntime 是 0；睡了很久的 task，vruntime 也很小。永遠挑最小的，就會**一直**挑它，直到它追上其他人。差距可能很大，它會霸占 CPU 很久。

[49:01] 解法：新 task 或恢復的 task，`vruntime += min_vruntime`。

- [49:32] 例如原本是 0，就變成 `min_vruntime`：有一點優勢，但不多。
- [50:01] 所有 task 的 vruntime **永遠單調遞增**，不會變小。

### 8.2 fork

[50:01]–[51:01] 漏洞：時間片用完就 fork 一個子 process 繼續做，子 process 又 fork……每個新 process 都從 `min_vruntime` 開始，有優勢。這是在**玩弄排程器**。

解法：**子 process 繼承父 process 的 vruntime**，fork 就沒有好處。

### 8.3 遷移

[51:01]–[52:01] 從一顆核心搬到另一顆：兩邊的 vruntime 水準可能差很多。

做法：

1. 離開時，`vruntime −= 舊 queue 的 min_vruntime`：只帶走**相對值**。
2. 到達時，`vruntime += 新 queue 的 min_vruntime`。

[52:01] 教授的例子：舊 queue 的最小值 3、新 queue 的最小值 5，task 的 vruntime 是 6。帶走 6 − 3 = 3，到新 queue 變成 5 + 3 = 8。

[52:32] 這樣就沒辦法靠一直遷移來鑽漏洞。

### 8.4 【原始碼】實際的規則比課本細

**fork**：

`kernel/sched/fair.c:11898-11926`（節錄）

```c
static void task_fork_fair(struct task_struct *p)
{
	...
	cfs_rq = task_cfs_rq(current);
	curr = cfs_rq->curr;
	if (curr) {
		update_curr(cfs_rq);
		se->vruntime = curr->vruntime;
	}
	place_entity(cfs_rq, se, 1);

	if (sysctl_sched_child_runs_first && curr && entity_before(curr, se)) {
		...
		swap(curr->vruntime, se->vruntime);
		resched_curr(rq);
	}

	se->vruntime -= cfs_rq->min_vruntime;
	...
}
```

- `se->vruntime = curr->vruntime`：**繼承父 process 的 vruntime**，跟教授說的一樣。
- `place_entity(..., 1)`：再調整（見下）。
- 最後 `−= min_vruntime`：子 process 可能被放到別顆 CPU，所以先存成相對值，真正 enqueue 時再加回目標 queue 的 `min_vruntime`。

**放置新 task / 醒來的 task**：

`kernel/sched/fair.c:4678-4733`（節錄）

```c
static void
place_entity(struct cfs_rq *cfs_rq, struct sched_entity *se, int initial)
{
	u64 vruntime = cfs_rq->min_vruntime;

	/*
	 * The 'current' period is already promised to the current tasks,
	 * however the extra weight of the new task will slow them down a
	 * little, place the new task so that it fits in the slot that
	 * stays open at the end.
	 */
	if (initial && sched_feat(START_DEBIT))
		vruntime += sched_vslice(cfs_rq, se);

	/* sleeps up to a single latency don't count. */
	if (!initial) {
		unsigned long thresh;
		...
			thresh = sysctl_sched_latency;

		/*
		 * Halve their sleep time's effect, to allow
		 * for a gentler effect of sleepers:
		 */
		if (sched_feat(GENTLE_FAIR_SLEEPERS))
			thresh >>= 1;

		vruntime -= thresh;
	}
	...
	if (entity_is_long_sleeper(se))
		se->vruntime = vruntime;
	else
		se->vruntime = max_vruntime(se->vruntime, vruntime);
}
```

| 情況 | 實際規則 |
|---|---|
| **新 task**（`initial = 1`） | 基準 = `min_vruntime` **加上一個時間片**（`START_DEBIT`）：「這個週期已經答應給現有的 task 了，新 task 排到最後面」。最後取 max(繼承來的, 基準) |
| **醒來的 task**（`initial = 0`） | 基準 = `min_vruntime` **減掉半個排程週期**（`GENTLE_FAIR_SLEEPERS`）：睡過的 task 給一點補償，讓互動式程式反應快；但最多補半個週期 |
| `max_vruntime(...)` | 如果它本來的 vruntime 比基準大，就保持原樣，**不會往回拉**。只有睡太久（vruntime 落後太多）的才被拉到基準 |
| 睡超級久（`entity_is_long_sleeper`） | 直接設成基準。註解說：差距大到可能讓 s64 溢位（約 104 天）時，忽略原本的值 |

⚠️ 跟課本 / 教授的「`vruntime += min_vruntime`」的差別：

- 課本的「加上 `min_vruntime`」是**遷移與 fork 時的相對值換算**（§8.3），不是醒來時的規則。
- 對**在同一顆 CPU 上睡著再醒來**的 task，kernel 並沒有先減再加：dequeue 時只有 `!(flags & DEQUEUE_SLEEP)`（不是因為睡覺而離開，例如遷移）才 `−= min_vruntime`：

`kernel/sched/fair.c:4915-4916`

```c
	if (!(flags & DEQUEUE_SLEEP))
		se->vruntime -= cfs_rq->min_vruntime;
```

  enqueue 時對應地加回來（`fair.c:4780-4792` 的 `renorm`）。睡覺醒來的 task 保留絕對的 vruntime，再由 `place_entity` 取 max(原值, `min_vruntime` − 半週期)。
- 課本第 7 點說「遷移**或阻塞**時減掉 `min_vruntime`」，「阻塞」那部分不對。

---

## 9. 【實測】nice 0 vs nice 5

兩個死迴圈 process 綁在同一顆 CPU（CPU 5），一個 nice 0、一個 nice 5，跑 6 秒後讀 `/proc/<pid>/sched`：

```
A(nice0) se.vruntime                                  :          4533.923206
A(nice0) se.sum_exec_runtime                          :          4522.971789
A(nice0) nr_switches                                  :                  236
A(nice0) se.load.weight                               :              1048576
A(nice0) prio                                         :                  120
B(nice5) se.vruntime                                  :          4533.514178
B(nice5) se.sum_exec_runtime                          :          1480.320606
B(nice5) nr_switches                                  :                  226
B(nice5) se.load.weight                               :               343040
B(nice5) prio                                         :                  125
ticks: nice0=451 nice5=148  share nice0=.752  expected=.753
```

| | nice 0 | nice 5 |
|---|---|---|
| 真實執行時間 `sum_exec_runtime` | 4523 ms | 1480 ms |
| **vruntime** | **4533.9 ms** | **4533.5 ms** |
| 權重 `load.weight` | 1048576 = 1024 × 1024 | 343040 = 335 × 1024 |
| CPU 份額 | **75.2%** | 24.8% |

- **真實時間差 3.06 倍，vruntime 幾乎一模一樣**（差 0.4 ms）。這就是 §7：在 vruntime 上是 round robin。
- 份額 75.2%，理論值 1024 / (1024 + 335) = 75.3%。
- `load.weight` 是表上的值左移 10 位（64 位元 kernel 用更高精度，`scale_load`）。
- 各換了約 230 次：6 秒 / 約 230 輪 ≈ 每輪 25 ms，正好接近本機算出的排程週期 **24 ms**（§6.4）。一輪內 nice 0 跑約 18 ms、nice 5 跑約 6 ms。

---

## 10. 計算 CPU 負載：PELT

[52:32]–[53:31] 每顆 CPU 上 task 有的在跑、有的在睡、有的在等 I/O。某顆 CPU 很忙、另一顆很閒，就要把 task 從忙的搬到閒的。要搬，先要**算出每顆核心的負載**。

[54:00] 做法：

- 時間切成 1 ms 的區間 p₀, p₁, p₂……（一個 jiffy）。
- u_i = 區間 p_i 裡「有 task 可執行（在跑）」的時間比例。1 = 一直在跑運算；0.5 = 一半時間在忙，另一半排程器找不到東西跑。

[54:30] 負載平均（課本式 5.7）：

```
loadavg = u₀ + u₁·y + u₂·y² + ...
```

[55:01] y 是一個很接近 1 的數：**y³² = 0.5**，也就是 y = 2^(−1/32) ≈ 0.9786。

- 32 ms 前的活動，權重只剩一半；64 ms 前剩四分之一。
- 這是**指數加權**的平均：最近的比較重要。
- [55:30] 教授：這個定義在二進位下很好算；公式也有經驗上的考量，「實務上很好用」。

【課本補充】這叫 **PELT（Per-Entity Load Tracking）**，在 `kernel/sched/pelt.c`。也可以拿來調整 CPU 的電壓和頻率。

【原始碼】

`kernel/sched/sched-pelt.h:13-14`

```c
#define LOAD_AVG_PERIOD 32
#define LOAD_AVG_MAX 47742
```

`kernel/sched/sched-pelt.h:4-7`（節錄）

```c
static const u32 runnable_avg_yN_inv[] __maybe_unused = {
	0xffffffff, 0xfa83b2da, 0xf5257d14, 0xefe4b99a, 0xeac0c6e6, 0xe5b906e6,
	0xe0ccdeeb, 0xdbfbb796, 0xd744fcc9, 0xd2a81d91, 0xce248c14, 0xc9b9bd85,
	0xc5672a10, 0xc12c4cc9, 0xbd08a39e, 0xb8fbaf46, 0xb504f333, 0xb123f581,
```

- `LOAD_AVG_PERIOD 32`：就是 y³² = ½ 的 32。
- `runnable_avg_yN_inv[n]` = yⁿ × 2³²（定點數）。第 0 項 `0xffffffff` ≈ 1；第 16 項 `0xb504f333` ≈ 0.7071 × 2³² = **√½**，因為 y¹⁶ = (y³²)^½。
- 預先算好表，就不用在 kernel 裡做浮點運算。超過 32 個區間就先每 32 個減半（右移一位），再查表。
- `LOAD_AVG_MAX 47742`：u 一直是 1 時，這個級數的最大值（以 1024 為一個區間的單位）。
- 實際的 PELT 區間是 1024 μs（≈ 1 ms），也是為了用位移代替除法。

---

## 11. 一頁速查表

### 11.1 排程類別

stop > dl（EDF） > rt（FIFO / RR） > **fair（CFS）** > idle。嚴格優先，類別間不公平。

`struct sched_class` = 一組函式指標；CFS 在 `kernel/sched/fair.c`。

### 11.2 CFS 公式

| 量 | 公式 |
|---|---|
| δvruntime | δ × 1024 / weight(nice) |
| 權重 | nice 0 = 1024，每級約 ×/÷ 1.25 |
| 排程週期 | task 數 ≤ 8：`sched_latency`；否則 task 數 × `min_granularity` |
| 預設（×(1+log₂ min(CPU,8))） | 6 ms / 0.75 ms；本機 ×4 → 24 ms / 3 ms |
| 時間片 | 週期 × 我的權重 / 總權重，不小於最小粒度 |
| 結論 | 跑完一個時間片，大家的 δvruntime 一樣 → vruntime 上的 round robin |

### 11.3 挑 task

紅黑樹（以 vruntime 為鍵），最左節點有快取 → O(1) 挑最小；時間片用完才 `resched_curr`。

### 11.4 防作弊

| 情況 | 規則 |
|---|---|
| fork | 繼承父的 vruntime；再放到 `min_vruntime` + 一個時間片之後 |
| 醒來 | max(原值, `min_vruntime` − 半個週期) |
| 遷移 | 減舊 queue 的 `min_vruntime`、加新 queue 的 |
| cgroup | 一個 cgroup 一個 vruntime |

### 11.5 PELT

loadavg = Σ uᵢ yⁱ，y³² = ½；預先算好的定點數表。

### 11.6 本講看過的原始碼位置

| 位置 | 內容 |
|---|---|
| `kernel/sched/core.c:6681-6693` | `schedule()` |
| `kernel/sched/sched.h:2164-2188` | `struct sched_class` |
| `kernel/sched/fair.c:12447-12458` | CFS 的 `sched_class` |
| `kernel/sched/core.c:11338-11347` | `sched_prio_to_weight` |
| `kernel/sched/fair.c:897-924` | `update_curr` |
| `kernel/sched/fair.c:709-715` | `calc_delta_fair` |
| `kernel/sched/fair.c:653-661` | `__pick_first_entity` |
| `kernel/sched/fair.c:72, 93, 107, 253-272` | 週期、粒度、`nr_latency`、CPU 數縮放 |
| `kernel/sched/fair.c:725-731, 741-780` | `__sched_period`、`sched_slice` |
| `kernel/sched/fair.c:4939-4956` | `check_preempt_tick` |
| `kernel/sched/fair.c:11898-11926` | `task_fork_fair` |
| `kernel/sched/fair.c:4678-4733` | `place_entity` |
| `kernel/sched/fair.c:4915-4916` | 只有非睡眠的 dequeue 才換算相對值 |
| `kernel/sched/sched-pelt.h:4-14` | PELT 常數 |

---

## 12. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **dl 類別（EDF）、rt 類別** | **第 27 講**：Real-time schedulers；課本 §5.4.7 |
| **即時 task 的資源與優先權反轉** | **第 28 講**：Real-time resource allocation |
| **`prev_mm`、context switch** | 回顧第 12–13 講 |
| **紅黑樹、`container_of`** | 回顧第 14 講；第 29–33 講記憶體管理也大量使用 |
| **cgroup** | 容器（本課程未專講，待補） |

（講次對應依播放清單標題推估。）

---

## 附：本講一句話總結

> **Linux 的 `schedule()` 先把工作分給 worker、關搶占、呼叫 `__schedule()`，依 stop > deadline > rt > fair > idle 的嚴格順序（每個類別是一個裝滿函式指標的 `struct sched_class`）挑出下一個 task。一般 task 由 CFS（在 `fair.c`，不是 `cfs.c`）負責：每個 task 的 vruntime 依 1024/權重 放大或縮小真實執行時間（nice 每差一級約 1.25 倍），放在以 vruntime 為鍵的紅黑樹裡，永遠挑最左邊的；為了不要每個 jiffy 都換人，把一個排程週期（預設 6 ms × (1+log₂CPU 數)，本機 24 ms）依權重切成時間片，時間片用完才設 `TIF_NEED_RESCHED`，結果每個 task 每輪增加的 vruntime 都一樣，等於以 vruntime 為單位的 round robin（實測 nice 0 和 nice 5 真實時間差 3.06 倍、份額 75.2%，vruntime 卻只差 0.4 ms）。用 `min_vruntime` 防作弊：fork 繼承父的 vruntime 並排到週期末，醒來最多補半個週期，遷移只帶相對值。負載用 PELT 計算：每 1 ms 的忙碌比例做 y³² = ½ 的指數加權平均。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 26（Chapter 5, Part IX）錄影逐字稿（YouTube 自動字幕，56:13）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§5.4.5–5.4.6，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
