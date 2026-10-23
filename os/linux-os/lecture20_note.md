# Operating Systems: A Linux Kernel-Oriented Approach 第二十講完整筆記：進度保證、Semaphore、Reader-Writer Lock 與 Wait-Free Queue（Chapter 5, Part III）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 20（Chapter 5, Part III：RW Locks and queues）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（42:32，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§5.1.7–5.1.11、§5.2–5.2.2（p.186–200）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
> **標記說明**：
> - `【口頭】`：教授在課堂上口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與教授回答
> - `【課本補充】`：課本有、但這堂課沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（`檔名:行號`），用來對照觀念
> - `【實測】`：在本機（Ubuntu、kernel 6.5.0、x86-64、20 個 CPU）實際編譯執行的結果
> - `⚠️ 逐字稿缺字`：自動字幕在這裡掉字或聽不清，內容依上下文或課本補上
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 這堂課要做什麼](#0-這堂課要做什麼)
- [1. 進度保證（liveness）](#1-進度保證liveness)
  - [1.1 為什麼不用 lock](#11-為什麼不用-lock)
  - [1.2 三種保證](#12-三種保證)
  - [1.3 實務上選哪個](#13-實務上選哪個)
- [2. Semaphore](#2-semaphore)
  - [2.1 `sem_wait` 與 `sem_post`](#21-sem_wait-與-sem_post)
  - [2.2 【原始碼】kernel 的 `down()` 與 `up()`](#22-原始碼kernel-的-down-與-up)
- [3. 【課本補充】Condition variable](#3-課本補充condition-variable)
- [4. Reader-writer lock](#4-reader-writer-lock)
  - [4.1 規則：多個讀者，或一個寫者](#41-規則多個讀者或一個寫者)
  - [4.2 用兩把鎖實作](#42-用兩把鎖實作)
  - [4.3 公平性問題](#43-公平性問題)
  - [4.4 【實測】寫者會餓死嗎](#44-實測寫者會餓死嗎)
- [5. 【課本補充】Barrier 與 phaser](#5-課本補充barrier-與-phaser)
- [6. Queue：kernel 最常用的資料結構](#6-queuekernel-最常用的資料結構)
  - [6.1 各種 queue](#61-各種-queue)
  - [6.2 五個例子的路線](#62-五個例子的路線)
- [7. Wait-free 的單生產者單消費者 queue](#7-wait-free-的單生產者單消費者-queue)
  - [7.1 C 的 atomics](#71-c-的-atomics)
  - [7.2 主程式：一個 enqueue thread、一個 dequeue thread](#72-主程式一個-enqueue-thread一個-dequeue-thread)
  - [7.3 環狀緩衝區](#73-環狀緩衝區)
  - [7.4 `enq` 與 `deq`](#74-enq-與-deq)
  - [7.5 為什麼是 wait-free](#75-為什麼是-wait-free)
  - [7.6 為什麼要空一格](#76-為什麼要空一格)
  - [7.7 【實測】跑 1000 萬筆](#77-實測跑-1000-萬筆)
  - [7.8 【原始碼】kernel 的 `kfifo`](#78-原始碼kernel-的-kfifo)
- [8. 多個生產者／消費者：要用 lock](#8-多個生產者消費者要用-lock)
- [9. 一頁速查表](#9-一頁速查表)
- [10. 與後續講次的連結](#10-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

1. 補完上一講最後預告的 **liveness**：三種進度保證。
2. 兩種比 lock 進階的同步機制：**semaphore**、**reader-writer lock**。
3. 用這些東西做 **queue**。先看最簡單的：**wait-free 的單生產者單消費者 queue**。

---

## 1. 進度保證（liveness）

### 1.1 為什麼不用 lock

[00:33] 進度保證就是 **liveness 保證**：保證一個操作**會做完**。

[00:33] 用 lock 的問題：持有 lock 的 thread 可能被換出去。它沒機會跑，就沒機會放 lock，[01:00] 其他等這把 lock 的 thread 就**永遠等下去**。

所以有時會考慮 **lock-free 演算法**：不用 lock，只靠 atomic 操作（atomic fetch-and-increment、atomic compare-and-swap 等）。

[01:32] 有一個定理：**任何並行資料結構，都可以只用這些指令、不用 lock 來實作。** 不過效率可能很差，定理不保證效率。

⚠️ 教授說這個定理叫 "universal consensus theorem"。課本 §5.1.7 引用的是 Herlihy & Shavit 書裡的 **universal construction**（通用建構）。

[02:01] 【口頭】很多現代 C++ 和 Java 的資料結構都是用這些快速 primitive 寫的，很多根本不用 lock。kernel 裡也有很大一部分資料結構本質上是 lock-free 的。上一講最後的定理在這裡一樣成立。

### 1.2 三種保證

[02:30] 設系統裡有 n 個 thread。「步」指的是那個 thread 自己的**內部步驟（internal step）**。

| 保證 | 意思 | 例子 |
|---|---|---|
| **Obstruction-free** | 其他 n−1 個 thread 在任意時間點**停下來**，剩下那一個保證在有限步內做完 | [03:32] CAS 迴圈。用 lock **不行**：停下來的 thread 可能正拿著 lock |
| **Lock-free** | 任何時刻，**至少有一個** thread 會達成目標。系統整體在前進，但不一定是你 | [03:32] CAS 迴圈也是 lock-free。可能 starvation，不會 deadlock |
| **Wait-free** | **所有** thread 都在有限（通常說 bounded）步內做完。沒有 starvation | [04:31] 直接呼叫 fetch-and-increment：硬體保證有限步內完成 |

[05:02] 一句話總結 CAS 迴圈：「一直跑 compare-and-set，等著把 0 變成 1」→ **lock-free，但不是 wait-free**，因為可能 starvation。

[05:02] 階層：

```
wait-free ⊂ lock-free ⊂ obstruction-free
```

- wait-free 比 lock-free 強，所以 wait-free 程式一定是 lock-free。
- lock-free 比 obstruction-free 強，所以 lock-free 程式一定是 obstruction-free。

（課本的證明與文氏圖見第 19 講筆記 §10。）

### 1.3 實務上選哪個

[05:31] 重點整理：

- **不能有 thread 拿著 lock**。拿著 lock 去睡，唯一醒著的 thread 就動不了。
- **Lock-free 實務上很快**，所以大多數時候用它。理論上有 starvation 的問題，但負載低時不一定會出現。
- [06:01] **Wait-free 保證準時完成**，但一般比 lock-free 慢。要保證就得付代價：所有操作都**一致地變慢**。

所以 queue、stack 這類資料結構通常用 lock-free。

---

## 2. Semaphore

### 2.1 `sem_wait` 與 `sem_post`

[06:30] 接下來看 kernel 程式碼層級常見的同步機制。lock 已經講過，現在講 **semaphore（號誌）**和 **reader-writer lock**。

[06:30] 教授說：semaphore 像是「**lock++**」。

- lock 只有兩種狀態：0 或 1。大家搶著把 0 變 1，成功的就擁有它。
- [07:00] semaphore 比較「重」：內建一個 **count（計數）**和一個**等待中 process 的 queue**。

[07:00] semaphore 一般在 **kernel** 層實作，可以想成 kernel 的資料結構，使用者空間再包一層函式庫。不過這不是硬性規定。

**`sem_wait`**（課本 Listing 5.7），整段 atomic 執行：

```c
/* execute atomically */
if (count == 0)
    insert_into_wait_queue(current_task);   /* 0 → 去排隊等 */
else
    count--;                                 /* 非 0 → 減 1，不用等 */
```

[07:32] 注意意思跟 lock 有點不一樣：**count 是 0 才要等**，不是 0 就減 1 直接過。

**`sem_post`**（課本 Listing 5.8），也是整段 atomic：

```c
/* execute atomically */
if ((count == 0) && process_waiting())
    wake_from_wait_queue();                  /* 有人在等 → 直接叫醒一個 */
else
    count++;
```

[08:30] 為什麼有人在等時**不動 count**？教授解釋這是一個**最佳化**：

- 如果先把 count 設成 1，再叫醒等待者，等待者醒來又會 `count--` 變回 0。
- 何必加一次又減一次？直接從等待 queue 叫醒它，就當它拿到了。

[09:00] 重點：

| | lock | semaphore |
|---|---|---|
| 狀態 | 0 / 1 | count = 0, 1, 2, … |
| 「要等」的條件 | 已經是 1 | count 是 0 |
| 關係 | — | **binary semaphore（count 最多 1）= lock** |
| 額外能力 | — | 可以做 **bounded queue**（下一講） |

【課本補充】課本：count = 5，就可以有 5 個 process 同時拿到這個 semaphore。`sem_wait` 也叫 **wait**，`sem_post` 也叫 **signal**。

### 2.2 【原始碼】kernel 的 `down()` 與 `up()`

kernel 的 semaphore 跟課本的偽碼幾乎一模一樣。結構：

`include/linux/semaphore.h:15-19`

```c
struct semaphore {
	raw_spinlock_t		lock;
	unsigned int		count;
	struct list_head	wait_list;
};
```

| 欄位 | 說明 |
|---|---|
| `lock` | 一把 spinlock，用來讓下面的 if/else **整段 atomic**（就是偽碼裡的 "execute atomically"） |
| `count` | 計數 |
| `wait_list` | 等待中 task 的 queue（第 14 講的 `list_head`） |

`sem_wait` 在 kernel 叫 `down()`：

`kernel/locking/semaphore.c:54-65`

```c
void __sched down(struct semaphore *sem)
{
	unsigned long flags;

	might_sleep();
	raw_spin_lock_irqsave(&sem->lock, flags);
	if (likely(sem->count > 0))
		sem->count--;
	else
		__down(sem);
	raw_spin_unlock_irqrestore(&sem->lock, flags);
}
```

| 行 | 說明 |
|---|---|
| 58 | `might_sleep()`：提醒（除錯用）這個函式可能會睡，不能在中斷裡呼叫 |
| 59, 64 | 拿 spinlock、關中斷：讓中間這段 atomic |
| 60-61 | count > 0 → 減 1，直接走 |
| 62-63 | count = 0 → `__down`：去排隊睡覺 |

`sem_post` 在 kernel 叫 `up()`：

`kernel/locking/semaphore.c:183-193`

```c
void __sched up(struct semaphore *sem)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&sem->lock, flags);
	if (likely(list_empty(&sem->wait_list)))
		sem->count++;
	else
		__up(sem);
	raw_spin_unlock_irqrestore(&sem->lock, flags);
}
```

| 行 | 說明 |
|---|---|
| 188-189 | 沒人在等 → `count++` |
| 190-191 | 有人在等 → `__up`：直接叫醒一個，**count 不動**。這就是教授講的最佳化 |

（kernel 只檢查「有沒有人在等」，沒有另外檢查 count == 0。有人在等時 count 一定是 0。）

叫醒的細節：

`kernel/locking/semaphore.c:272-279`

```c
static noinline void __sched __up(struct semaphore *sem)
{
	struct semaphore_waiter *waiter = list_first_entry(&sem->wait_list,
						struct semaphore_waiter, list);
	list_del(&waiter->list);
	waiter->up = true;
	wake_up_process(waiter->task);
}
```

- 拿等待 queue 的**第一個**（先來先服務）。
- `waiter->up = true`：告訴它「你拿到了」。
- `wake_up_process`：把它叫醒。

等待的那一方（`___down_common`，`kernel/locking/semaphore.c:209-229`）：

```c
	list_add_tail(&waiter.list, &sem->wait_list);
	waiter.task = current;
	waiter.up = false;

	for (;;) {
		if (signal_pending_state(state, current))
			goto interrupted;
		if (unlikely(timeout <= 0))
			goto timed_out;
		__set_current_state(state);
		raw_spin_unlock_irq(&sem->lock);
		timeout = schedule_timeout(timeout);
		raw_spin_lock_irq(&sem->lock);
		if (waiter.up)
			return 0;
	}
```

- 把自己加到 queue **尾端**。
- 設成睡眠狀態，**先放掉 spinlock 再睡**（拿著 spinlock 睡會害別人一直空轉）。
- 醒來看 `waiter.up`：是 true 就表示 `__up` 已經把 semaphore 交給我了，直接返回。**不再 `count--`**，跟 `__up` 不動 count 互相配合。
- 也可能被 signal 叫醒（第 17 講），就走 `interrupted`。

`up()` 的註解（第 180-181 行）還提到一個跟 mutex 的差別：**`up()` 可以由從沒呼叫過 `down()` 的 task 來呼叫。** semaphore 沒有「擁有者」的概念。kernel 的 semaphore 和 mutex 在第 22 講會再談。

---

## 3. 【課本補充】Condition variable

本講沒講，課本 §5.1.9 有。

semaphore 需要 OS 支援（要有等待 queue、要能叫醒）。pthreads 在使用者空間提供另一種方案：**condition variable（條件變數）**。

```c
pthread_mutex_t mlock;
pthread_cond_t cond;
pthread_cond_init(&cond, NULL);

/* 等待條件（類似 sem_wait） */
pthread_mutex_lock(&mlock);
pthread_cond_wait(&cond, &mlock);
pthread_mutex_unlock(&mlock);

/* 發信號（類似 sem_post） */
pthread_mutex_lock(&mlock);
pthread_cond_signal(&cond);
pthread_mutex_unlock(&mlock);
```

（課本 Listing 5.9 的 `pthread_cond_init(&count, NULL)` 應是 `&cond` 的筆誤。）

- 等待前要先拿 `mlock`：跟條件有關的狀態要在 critical section 裡改。
- `pthread_cond_wait` 吃兩個參數：條件變數和它配的那把鎖。
- `pthread_cond_signal` 叫醒一個等待者；`pthread_cond_broadcast` 叫醒全部。

> **課本 Point 5.1.5**：condition variable **不是** semaphore。
> - semaphore 有**記憶**：存著 count，就算沒人在等也可以先加上去。
> - condition variable **沒有記憶**：signal 時只叫醒「那一刻」正在等的 thread。如果那時沒人在等，這個 signal 就**丟掉了**。這叫 **lost wakeup problem（遺失喚醒問題）**。

---

## 4. Reader-writer lock

### 4.1 規則：多個讀者，或一個寫者

[09:34] **Reader-writer lock（讀寫鎖）**：

- 允許**一個寫者**，**或**允許**多個讀者**。
- 讀者和寫者**不能同時**存在。

[09:34] 兩種模式：

| 模式 | 誰在裡面 |
|---|---|
| 寫模式 | 只有一個寫者，沒有讀者 |
| 讀模式 | 只有讀者，可以很多個 |

[10:00] 難的是在兩種模式之間**公平地切換**。

[10:30] 讀者很多時，讓它們一起讀能撐起很高的吞吐量。但這能**無限期延後寫者**嗎？寫者寫的時候別人都不能動，這沒錯，但不代表可以一直不讓它寫。公平性要想清楚。

[11:03] 例子：queue 的 **peek**（只看最前面那個）只要讀鎖；**enqueue / dequeue** 會改東西，要寫鎖。

[11:31] 條件：

- 拿**讀鎖**：不能有寫者，其他讀者沒關係。
- 拿**寫鎖**：誰都不能在（沒有讀者，也沒有寫者）。

### 4.2 用兩把鎖實作

[11:31] 內部用兩把鎖：

| 鎖 | 用途 |
|---|---|
| `__rwlock` | 讀者寫者共用。拿到它 = 沒有別的寫者（或讀者群）在裡面 |
| `__rdlock` | 只給讀者用，保護 `readers` 這個計數 |

課本 Listing 5.10（`LOCK`/`UNLOCK` 是包 mutex 的巨集）：

```c
void get_write_lock() {
    LOCK(__rwlock);
}
void release_write_lock() {
    UNLOCK(__rwlock);
}

void get_read_lock() {
    LOCK(__rdlock);
    if (readers == 0) LOCK(__rwlock);
    readers++;
    UNLOCK(__rdlock);
}
void release_read_lock() {
    LOCK(__rdlock);
    readers--;
    if (readers == 0)
        UNLOCK(__rwlock);
    UNLOCK(__rdlock);
}
```

[12:03] **寫者**很簡單：拿 `__rwlock` 就是獨占；放掉就讓大家進來。

[13:00] **拿讀鎖**：

1. 先拿 `__rdlock`。這不會跟寫者衝突，因為還沒做任何永久的改變。
2. [13:30] 看 `readers`：
   - **不是 0**：已經有讀者在裡面，所以一定沒有寫者。直接進。
   - **是 0**：可能沒人，也可能**有寫者**在裡面。去拿 `__rwlock`。[14:00] 有寫者就會在這裡等到它離開。
3. `readers++`：登記自己是讀者。
4. 放掉 `__rdlock`，但**繼續拿著 `__rwlock`**。

[14:00] 重點：**第一個讀者**拿著 `__rwlock` 不放，之後就沒有寫者進得來。

[14:32] **放讀鎖**：

1. 拿 `__rdlock`（因為要改 `readers`）。
2. `readers--`。
3. [15:01] 如果變成 0：最後一個讀者走了，放掉 `__rwlock`，寫者可以進來了。否則繼續拿著。
4. 放掉 `__rdlock`。

[15:31] 【口頭】有趣的是：拿 `__rwlock` 的是**第一個**讀者，放掉它的是**最後一個**讀者，兩者可能是**不同的 thread**。這完全可能。

（補充：POSIX 規定 `pthread_mutex_unlock` 要由持有者呼叫，用預設型別的 mutex 讓別的 thread 去 unlock 是未定義行為。課本這段是概念示範；實務上 `__rwlock` 應該用 semaphore 這種沒有擁有者概念的東西，見 §2.2。）

### 4.3 公平性問題

[15:31] 這個設計有公平性問題：讀者可以**一直進來**，沒有東西擋它們，所以 `readers` 可能永遠不會降到 0，`__rwlock` 永遠不會放，**寫者會一直餓下去（starve）**。

[16:01] 這個設計**對讀者公平，對寫者不公平**。

【課本補充】課本說：就算底下的兩把 mutex 本身是公平的，整體還是不能保證寫者公平，這個設計需要改進。解法可參考 Herlihy & Shavit 的書。

### 4.4 【實測】寫者會餓死嗎

照 Listing 5.10 寫 `rw.c`。R 個讀者 thread 不停地「拿讀鎖 → 讀 1 ms → 放讀鎖」。0.1 秒後，一個寫者用 `pthread_mutex_timedlock(&__rwlock, 3 秒)` 試著拿寫鎖：

```
1 readers: writer got the lock after 0.921 s (reads done meanwhile: 868)
4 readers: writer STILL WAITING after 3.000 s (reads done meanwhile: 10873)
16 readers: writer STILL WAITING after 3.000 s (reads done meanwhile: 46089)
```

- **1 個讀者**：讀者放掉讀鎖後、再拿之前，`readers` 會短暫變 0，寫者偶爾搶得到。即使如此也等了快 1 秒。
- **4 個、16 個讀者**：讀者輪流重疊，`readers` 幾乎從不歸零。寫者等了 3 秒還沒拿到，期間讀者讀了 1 萬到 4.6 萬次。

和教授說的一樣：對讀者公平，寫者會餓死。

---

## 5. 【課本補充】Barrier 與 phaser

本講只在最後預告，課本 §5.1.11 有。

**Barrier（屏障）**：一個集合點。所有 thread 都到了，才一起放行。

例子：平行加總一個大陣列。

1. 切成 n 塊，第 i 塊給第 i 個 thread 加（map 階段）。
2. 每個 thread 把部分和交給一個 root thread。
3. root thread 要**等所有 thread 都做完**，才能把部分和加起來（reduce 階段）。

這個「等大家都到」就是 barrier。線性代數之類的科學計算很常用。超級電腦甚至有專門做 barrier 的硬體。

注意：這個 barrier 跟第 18–19 講的 **memory barrier（fence）**完全不同，只是名字一樣（課本說「很不幸」）。

**Phaser**：更有彈性的 barrier。定義兩個點：Point 1 在前、Point 2 在後。

> 規則：所有 thread 都到了 Point 1，才能有 thread 越過 Point 2。

Java 等語言有提供。做 pipeline 式計算時很好用：前一階段要全部做完，才能分派某些新工作。

---

## 6. Queue：kernel 最常用的資料結構

### 6.1 各種 queue

[16:01] 接下來用這些 primitive 做更高階的結構：**queue**。

[16:30] 教授說 queue 是 kernel 裡最常用的資料結構之一（說到一半改口：「就算不是最常用的」）。通常是**生產者–消費者 queue**：

- 生產者放東西進去，做 process 間通訊。
- 消費者把東西拿出來。

[16:30]–[18:01] 很多種類：

| 分類 | 種類 |
|---|---|
| 大小 | **Bounded**（有上限，例如 10 個，滿了生產者就不能再放）／ unbounded |
| 生產者、消費者數量 | 單生產者多消費者、多生產者單消費者、**多生產者多消費者**（一般情況）、單生產者單消費者 |
| 等不等 | **Synchronous**：queue 空了消費者就等；bounded 且滿了生產者就等 ／ **Asynchronous**：空了消費者直接回「空的」；滿了生產者直接回「放不進去」，誰都不等 |

[18:01] queue 是平行（並行）資料結構，一定需要同步。也需要某種**進度保證**：就算用 lock，也要假設 lock 能在有限時間內放掉。obstruction-free 或 lock-free 都好。

[18:31] 實務上用 **bounded queue**，因為實務上什麼都不該是無限的。kernel 裡很多子系統都用，**主要是 device driver**：從裝置讀到東西，丟進 queue。一個 queue 同時被很多人存取，至少生產者和消費者同時在用，所以必須是並行的。

【課本補充】課本 §5.2：子系統之間傳訊息通常用 queue，不用直接函式呼叫或寫陣列。queue 有 FIFO 性質（也隱含一種優先順序），而且天生是**非同步**的：生產者放了就走，消費者晚點再拿。

### 6.2 五個例子的路線

[19:01] 教授會看五個 queue 的例子：

| # | 例子 | 本講？ |
|---|---|---|
| 1 | 單生產者單消費者：**wait-free queue** | ✓ |
| 2 | 簡單的 busy-waiting lock（mutex） | 本講只開頭 |
| 3 | semaphore | 下一講 |
| 4 | 改良版 semaphore（不 busy wait） | 下一講 |
| 5 | reader-writer lock | 下一講 |

[19:30] 單生產者單消費者，做 wait-free queue **很容易**。多於一個生產者或消費者，就難多了。

[20:01] 【口頭】教授的建議：看投影片時，先懂理論，再看程式碼，然後**一定要跑**。「不跑程式，你不會真的感覺到它怎麼運作。能跑的程式是最有價值的。」

---

## 7. Wait-free 的單生產者單消費者 queue

### 7.1 C 的 atomics

[20:30] `stdatomic.h` 提供 atomic 整數、字元等。最重要的是 **`atomic_load`** 和 **`atomic_store`**：它們是同步指令，提供上一講講的 happens-before 保證。

[21:01] 其他操作：

| 函式 | 做什麼 |
|---|---|
| `atomic_fetch_add` | 讀值、加上某數、存回，一次做完。例如 `x = x + 3` 整個瞬間完成 |
| `atomic_flag_test_and_set` | 是 0 就設成 1 |
| `atomic_compare_exchange_strong` | CAS（第 18 講） |

[21:30] 教授說：這些 atomic 操作都是**循序一致（sequentially consistent）**的。怎麼做到的現在還沒辦法完全理解，但事實就是如此。

（補充：C11 的這些函式不指定 memory order 時，預設就是 `memory_order_seq_cst`。課本說「依架構和參數，它們帶有不同的記憶體順序保證（內嵌不同的 fence）」。）

### 7.2 主程式：一個 enqueue thread、一個 dequeue thread

課本 Listing 5.11 的 `main`：

```c
int main() {
    int errcode, i = 0; int *ptr;
    void *(*fptr)(void *);                 /* 函式指標 */
    for (i = 0; i < 2; i++) {
        ptr = (int *) malloc(sizeof(int));
        *ptr = i;
        fptr = (i % 2) ? &enqfunc : &deqfunc;
        errcode = pthread_create(&(tid[i]), NULL, fptr, ptr);
        if (errcode)
            printf("Error in creating pthreads\n");
    }
    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
}
```

[22:30] 迴圈跑兩次：`i = 0` → `deqfunc`；`i = 1` → `enqfunc`。也就是**一個生產者、一個消費者**，ID 分別是 1 和 0。

[23:32] 兩個 thread 函式：

```c
void *enqfunc(void *arg) {
    int i, val;
    int thread = *((int *) arg);
    srand(thread);                 /* 用 thread ID 當亂數種子 */
    for (i = 0; i < NUM; i++) {
        val = rand() % 10;         /* 0~9 的亂數 */
        enq(val);
        nap();                     /* 睡一段隨機的時間 */
    }
}

void *deqfunc(void *arg) {
    ...                            /* 一樣，但呼叫 deq() */
}
```

- [24:01] 用 thread ID 當亂數種子，兩個 thread 的亂數就不同。`NUM` 現在是 25，改成 1,000、10,000 都行。
- [24:30] 生產者：產生一個 0–9 的值，enqueue，睡一下，再 enqueue……

[24:30] `nap()`：

```c
void nap() {
    struct timespec rem;
    int ms = rand() % 100;                             /* 0~99 ms */
    struct timespec req = {0, ms * 1000 * 1000};       /* 換成奈秒 */
    nanosleep(&req, &rem);
}
```

[26:00] 為什麼種子要不同？這是**偽亂數**產生器，種子一樣，產生的數字就一樣，兩個 thread 會睡一樣久。

### 7.3 環狀緩衝區

[26:30] queue 有 **head**（頭）和 **tail**（尾）兩個指標，都宣告成 `atomic_int`，因為要做成**環狀 queue**。

[27:30]–[29:00] 用一般陣列做環狀 queue：

```
            0
       N-1     1
     N-2         2        下一格：INC(e) = (e + 1) % N
       ...     3          N-1 的下一格是 0
          ...
```

```c
#define BUFSIZE 10
#define INC(x) ((x + 1) % BUFSIZE)
```

[29:33]–[31:32] 想像排隊的人：

- **Dequeue**：從 **head** 拿走，然後 `head = INC(head)`。
- **Enqueue**：放到 **tail** 那邊，然後 `tail = INC(tail)`。

（教授在 [31:01] 先說反了，然後自己更正。）

[31:32] 它可以一直轉，跨過 0 也沒關係，只要裡面的東西比格子少就好。

[32:03] 好處：多個 enqueue 只會搶 tail，多個 dequeue 只會搶 head。這是做 bounded queue 的好方法。

### 7.4 `enq` 與 `deq`

```c
atomic_int queue[BUFSIZE];
atomic_int head = 0, tail = 0;

int enq(int val) {
    int cur_head = atomic_load(&head);
    int cur_tail = atomic_load(&tail);
    int new_tail = INC(cur_tail);
    /* check if the queue is full */
    if (new_tail == cur_head)
        return -1;
    /* There are no other enqueuers */
    atomic_store(&queue[cur_tail], val);
    atomic_store(&tail, new_tail);
    return 0; /* success */
}

int deq() {
    int cur_head = atomic_load(&head);
    int cur_tail = atomic_load(&tail);
    int new_head = INC(cur_head);
    /* check if the queue is empty */
    if (cur_tail == cur_head)
        return -1;
    /* There are no other dequeuers */
    int val = atomic_load(&queue[cur_head]);
    atomic_store(&head, new_head);
}
```

（課本這段 `deq` 少了最後的 `return val;`。）

[32:33] **滿的條件**：`INC(tail) == head`，回傳 −1。

[33:30] 教授這裡**修正 tail 的定義**：tail 不是指向最後一個元素，而是指向**最後一個元素的下一格（第一個空格）**。所以：

- [34:01] **enqueue**：把值寫到 `queue[cur_tail]`，再把 tail 設成 `new_tail`。
- [35:00] **dequeue**：反過來。先檢查**空的條件** `cur_tail == cur_head`（初始時兩者相等），是空的就回傳 −1。否則讀出 `queue[cur_head]`，再把 head 設成 `new_head`。

[33:00] 為什麼可以這麼簡單？因為**只有一個 enqueue thread、一個 dequeue thread**，沒有競爭。有多個 dequeue，這段程式就不能用了。

【課本補充】順序很重要（課本 §5.2.1 最後）：

- `enq`：**先寫入元素，再移動 tail**。
- `deq`：**先讀出元素，再移動 head**。

讀 queue 或寫一個空格，別的 thread 看不到（它們只看 head 和 tail 之間的東西），算是「**區域動作**」。改 head / tail 才是「**全域動作**」，別人才看得到結果。因為兩道指令之間的延遲可以無限長，所以**一律先做區域的、最後才做全域的**。全域那一步通常就是 **linearization point**（第 19 講）。

### 7.5 為什麼是 wait-free

[36:00] 這是一個能平行跑的、bounded、單生產者單消費者 queue，而且是 **wait-free**：

- [36:31] **沒有 lock** → 一定是 lock-free。
- **沒有迴圈** → 不可能一直繞，不會 starvation。生產者沒有任何東西會擋住它。

所以在「只有一個生產者、一個消費者」的前提下，這是 wait-free queue。

[37:00] 【口頭】多生產者多消費者也有優雅的 wait-free 解法，但程式複雜得多，本課不講。

【課本補充】wait-free 的嚴格證明見 Herlihy & Shavit；linearizability 的證明超出課本範圍。

### 7.6 為什麼要空一格

[37:32] 修正後的定義：

- **head**：指向第一個有效元素（如果有的話）。
- **tail**：**不**指向有效元素，指向最後一個有效元素的**下一格**。

[37:32] 一直 dequeue，head 一直往前，最後 `head == tail`：**空**。

[38:01]–[39:01] 一直 enqueue，tail 一直往前。如果把最後一格也填了，`tail` 就會追上 `head`，變成 `head == tail`，跟「空」分不出來。所以 tail 走到 head 的前一格就**停**：`INC(tail) == head` 解讀為**滿**。

[39:01] 結論：如果 N = 1000，queue **最多只能放 999 個**。故意空一格，才能區分空和滿。

> **課本 Point 5.2.1**：一律從 head 拿、往 tail 放，然後把指標加 1（mod BUFSIZE）。`tail == head` → 空；`INC(tail) == head` → 滿。至少有一格永遠空著。

### 7.7 【實測】跑 1000 萬筆

照課本寫 `spsc.c`，拿掉 `nap()`，讓生產者依序放 0, 1, 2, …，消費者檢查拿到的順序。滿了或空了就重試：

```
dequeued 10000000 items, out-of-order: 0
max entries ever in queue: 9 (BUFSIZE=10)
retries: full=136569416 empty=131795
```

- 1000 萬筆，**順序錯誤 0 筆**：FIFO 正確，沒有任何 lock。
- `BUFSIZE = 10`，queue 裡**最多只有過 9 個**：空一格，跟 §7.6 說的一樣。
- 生產者比消費者快很多，所以大部分時間 queue 是滿的（「滿了重試」1.3 億次）。

注意 `enq` / `deq` 本身是 wait-free 的（回傳 −1 也是在有限步內返回）。這裡的「重試迴圈」是測試程式自己加的。

### 7.8 【原始碼】kernel 的 `kfifo`

kernel 真的有一個「單生產者單消費者不用鎖」的 queue：**`kfifo`**。

`include/linux/kfifo.h:30-36`

```c
 * Note about locking: There is no locking required until only one reader
 * and one writer is using the fifo and no kfifo_reset() will be called.
 * kfifo_reset_out() can be safely used, until it will be only called
 * in the reader thread.
 * For multiple writer and one reader there is only a need to lock the writer.
 * And vice versa for only one writer and multiple reader there is only a need
 * to lock the reader.
```

- 「只有一個讀者、一個寫者時，**不需要鎖**」：就是本講的 wait-free SPSC queue。
- 多個寫者、一個讀者：只要把**寫者那邊**鎖起來。反之亦然。這跟教授說的「enqueue 只搶 tail、dequeue 只搶 head」是同一個道理。

結構：

`include/linux/kfifo.h:44-50`

```c
struct __kfifo {
	unsigned int	in;
	unsigned int	out;
	unsigned int	mask;
	unsigned int	esize;
	void		*data;
};
```

| 欄位 | 對應課本 |
|---|---|
| `in` | tail（寫到哪裡） |
| `out` | head（讀到哪裡） |
| `mask` | 大小 − 1，用來代替 `% BUFSIZE` |
| `esize` | 每個元素幾 bytes |
| `data` | 緩衝區 |

放進去：

`lib/kfifo.c:89-125`（節錄）

```c
static void kfifo_copy_in(struct __kfifo *fifo, const void *src,
		unsigned int len, unsigned int off)
{
	...
	off &= fifo->mask;
	...
	memcpy(fifo->data + off, src, l);
	memcpy(fifo->data, src + l, len - l);
	/*
	 * make sure that the data in the fifo is up to date before
	 * incrementing the fifo->in index counter
	 */
	smp_wmb();
}

unsigned int __kfifo_in(struct __kfifo *fifo,
		const void *buf, unsigned int len)
{
	unsigned int l;

	l = kfifo_unused(fifo);
	if (len > l)
		len = l;

	kfifo_copy_in(fifo, buf, len, fifo->in);
	fifo->in += len;
	return len;
}
```

| 部分 | 說明 |
|---|---|
| `off &= fifo->mask` | 用位元 AND 取代 `%`，所以大小要是 2 的次方 |
| 兩個 `memcpy` | 資料可能跨過陣列尾端，分兩段複製（環狀） |
| `smp_wmb()` | **先把資料寫好，再加 `in`**。就是課本說的「先區域、後全域」。這裡用 write barrier 保證順序（第 19 講） |
| `fifo->in += len` | 全域動作：讀者現在看得到新資料了 |

拿出來（`lib/kfifo.c:128-172`）是對稱的：先 `memcpy` 出資料，`smp_wmb()`，再 `fifo->out += len`。

剩多少空間：

`lib/kfifo.c:19-22`

```c
static inline unsigned int kfifo_unused(struct __kfifo *fifo)
{
	return (fifo->mask + 1) - (fifo->in - fifo->out);
}
```

**和課本的差別**：kfifo **不用空一格**。

- `in`、`out` 是一直往上加的計數器，**不取模**。真正存取陣列時才 `& mask`。
- 裡面有幾個 = `in - out`。空 = `in == out`；滿 = `in - out == 大小`。
- 因為差值不會搞混，所以每一格都能用。
- `unsigned int` 溢位後繞回 0 也沒關係，只要大小是 2 的次方，`in - out` 一樣是對的。

所以分配時一定會把大小進位到 2 的次方：

`lib/kfifo.c:27-31`

```c
	/*
	 * round up to the next power of 2, since our 'let the indices
	 * wrap' technique works only in this case.
	 */
	size = roundup_pow_of_two(size);
```

兩種做法都解決「空和滿分不出來」的問題：課本**犧牲一格**；kfifo **用不取模的計數器**，代價是大小必須是 2 的次方。

---

## 8. 多個生產者／消費者：要用 lock

[39:32] wait-free queue 靠的是「只有一個生產者、一個消費者」這個限制。多個 enqueue / dequeue 時，同樣的技巧就不行了，要用 lock。

[40:00] 最簡單：enqueue 和 dequeue 都拿一把 pthread mutex。

【課本補充】課本 Listing 5.12（下一講會細講）：

```c
#define LOCK(x) (pthread_mutex_lock(&x))
#define UNLOCK(x) (pthread_mutex_unlock(&x))
pthread_mutex_t qlock;

int enq(int val) {
    int status;
    do {
        LOCK(qlock);
        if (INC(tail) == head) status = -1;   /* full */
        else {
            queue[tail] = val;
            tail = INC(tail);
            status = 0;
        }
        UNLOCK(qlock);
    } while (status == -1);                   /* 滿了就一直重試 */
    return status;
}
```

[40:00] Linux 用的是比較聰明的鎖：**futex**（教授：「可以想成 future mutex」）。

⚠️ futex 的正式名稱是 **fast userspace mutex**（快速使用者空間互斥鎖）。

[40:30] futex 的做法：

1. 先 **busy wait 一下**。
2. 但 busy wait 只是一直檢查變數，耗能量、耗時間，lock 沒放之前什麼都沒做成。
3. 所以等一陣子拿不到，就**告訴 OS**：「我拿不到 lock，請把我換掉，讓別的 process 跑。」

[41:04] 這叫 **yielding（讓出）**：告訴 OS「我沒辦法前進，讓別人來吧」。

（第 18 講 §6.4 的 strace 已經看到：mutex 搶不到時，glibc 呼叫 `futex(…, FUTEX_WAIT_PRIVATE, …)` 讓 thread 睡著，另一邊放鎖時 `FUTEX_WAKE_PRIVATE` 叫醒它。）

[41:04] 下一講從這裡繼續：用 lock、semaphore、reader-writer lock 做 queue，以及 condition variable、barrier、phaser。

[41:31] 之後進入 **kernel 層的同步**，再進入**排程**問題。教授說：先懂基本同步，才懂 kernel 同步。

---

## 9. 一頁速查表

### 9.1 進度保證

| | 保證 | 會 starve？ | 例子 |
|---|---|---|---|
| Obstruction-free | 別人都停，我有限步內做完 | 會 | CAS 迴圈 |
| Lock-free | 隨時至少一個人做完 | 會 | CAS 迴圈 |
| Wait-free | 每個人有限步內做完 | 不會 | fetch-and-add、SPSC queue |

wait-free ⊂ lock-free ⊂ obstruction-free。實務上多用 lock-free。

### 9.2 Semaphore

| 操作 | 別名 | 邏輯（整段 atomic） |
|---|---|---|
| `sem_wait` | wait、kernel `down()` | count == 0 → 排隊；否則 count-- |
| `sem_post` | signal、kernel `up()` | 有人在等 → 直接叫醒一個（count 不動）；否則 count++ |

binary semaphore = lock。semaphore 有記憶（count），condition variable 沒有（會 lost wakeup）。

### 9.3 Reader-writer lock

| | 拿 | 放 |
|---|---|---|
| 寫者 | `LOCK(__rwlock)` | `UNLOCK(__rwlock)` |
| 讀者 | 拿 `__rdlock`；`readers == 0` 就拿 `__rwlock`；`readers++` | `readers--`；變 0 就放 `__rwlock` |

第一個讀者拿 `__rwlock`、最後一個讀者放。讀者可以一直進來 → 寫者會餓死（實測 4 個讀者時，寫者等 3 秒拿不到）。

### 9.4 環狀 bounded queue

| | 條件 |
|---|---|
| 下一格 | `INC(x) = (x + 1) % BUFSIZE` |
| Enqueue | 寫 `queue[tail]`，再 `tail = INC(tail)` |
| Dequeue | 讀 `queue[head]`，再 `head = INC(head)` |
| 空 | `tail == head` |
| 滿 | `INC(tail) == head`（空一格，最多 BUFSIZE − 1 個） |
| 順序 | 先區域（資料）、後全域（指標） |

kfifo：`in`/`out` 不取模、大小是 2 的次方，`in - out` 就是個數，不用空格。

### 9.5 本講看過的原始碼位置

| 位置 | 內容 |
|---|---|
| `include/linux/semaphore.h:15-19` | `struct semaphore`：lock、count、wait_list |
| `kernel/locking/semaphore.c:54-65` | `down()` |
| `kernel/locking/semaphore.c:183-193` | `up()`：有人等就直接叫醒，count 不動 |
| `kernel/locking/semaphore.c:209-229` | `___down_common`：排隊、睡、醒來看 `waiter.up` |
| `kernel/locking/semaphore.c:272-279` | `__up`：叫醒第一個等待者 |
| `include/linux/kfifo.h:30-36` | 「一個讀者一個寫者不用鎖」 |
| `include/linux/kfifo.h:44-50` | `struct __kfifo` |
| `lib/kfifo.c:19-22` | `kfifo_unused` |
| `lib/kfifo.c:27-31` | 大小進位到 2 的次方 |
| `lib/kfifo.c:89-125` | `kfifo_copy_in` + `smp_wmb()` + `in += len` |

---

## 10. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **用 mutex、semaphore、reader-writer lock 做 queue；condition variable** | **第 21 講**：Concurrent queues |
| **kernel semaphore、mutex、spinlock、futex** | **第 22 講**：Concurrency in the kernel |
| **reader-writer 的另一種解法：讀者完全不用鎖** | **第 23 講**：RCU |
| **等待 queue、`wake_up_process`、`schedule_timeout`** | **第 24–26 講**：排程 |
| **Linux message queue**（課本 §5.2.6） | **第 21 講**（依課本順序推估） |

（講次對應依播放清單標題與課本章節推估。）

---

## 附：本講一句話總結

> **進度保證分三級：obstruction-free（別人都停我就做得完）⊃ lock-free（總有人做完，CAS 迴圈）⊃ wait-free（人人有限步內做完，fetch-and-add），實務多用 lock-free。Semaphore 是帶計數和等待 queue 的「lock++」：`wait` 遇 0 就排隊、否則減 1；`post` 有人在等就直接叫醒（不動 count），kernel 的 `down()`/`up()` 一模一樣。Reader-writer lock 用兩把鎖讓多個讀者或一個寫者進入，但讀者能一直進來，寫者會餓死（實測 4 個讀者時寫者 3 秒拿不到鎖）。Queue 是 kernel 最常用的結構；單生產者單消費者的環狀 bounded queue 不用鎖也沒有迴圈，是 wait-free 的，關鍵是先寫資料再移指標、並空一格區分空與滿（實測 1000 萬筆順序全對、最多只放 9 格）；kernel 的 `kfifo` 就是這種 queue，改用不取模的計數器加 2 的次方大小，省下那一格。多生產者多消費者就得用 lock，Linux 的 futex 先空轉一下、拿不到就讓 OS 把自己換掉。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 20（Chapter 5, Part III）錄影逐字稿（YouTube 自動字幕，42:32）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§5.1.7–5.1.11、§5.2–5.2.2，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
