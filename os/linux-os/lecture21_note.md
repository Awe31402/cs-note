# Operating Systems: A Linux Kernel-Oriented Approach 第二十一講完整筆記：並行 Queue、Condition Variable、Barrier（Chapter 5, Part IV）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 21（Chapter 5, Part IV：Concurrent queues）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（35:48，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§5.1.9、§5.1.11、§5.2.2–5.2.6（p.190–204）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
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
- [1. 共同的架子](#1-共同的架子)
- [2. 例 2：用 mutex 的 queue](#2-例-2用-mutex-的-queue)
  - [2.1 `volatile`](#21-volatile)
  - [2.2 程式碼](#22-程式碼)
  - [2.3 為什麼 lock 要放在迴圈裡面](#23-為什麼-lock-要放在迴圈裡面)
  - [2.4 【原始碼】kernel 對 `volatile` 的看法](#24-原始碼kernel-對-volatile-的看法)
- [3. 例 3：binary semaphore 當 lock](#3-例-3binary-semaphore-當-lock)
- [4. 例 4：三個 semaphore，不 busy wait](#4-例-4三個-semaphore不-busy-wait)
  - [4.1 程式碼](#41-程式碼)
  - [4.2 empty 與 full：把格子搬來搬去](#42-empty-與-full把格子搬來搬去)
  - [4.3 【實測】CPU 用量差 25 倍](#43-實測cpu-用量差-25-倍)
- [5. 例 5：加上 `peek`，用 reader-writer lock](#5-例-5加上-peek用-reader-writer-lock)
- [6. Condition variable](#6-condition-variable)
  - [6.1 用法](#61-用法)
  - [6.2 【課本補充＋實測】lost wakeup](#62-課本補充實測lost-wakeup)
  - [6.3 【原始碼】kernel 的 `wait_event` 與 `completion`](#63-原始碼kernel-的-wait_event-與-completion)
- [7. Barrier 與 phaser](#7-barrier-與-phaser)
- [8. 【課本補充】Linux message queue](#8-課本補充linux-message-queue)
  - [8.1 用法與【實測】](#81-用法與實測)
  - [8.2 【原始碼】`msg_queue` 與 `find_msg`](#82-原始碼msg_queue-與-find_msg)
- [9. 預告：排程與 NP-completeness](#9-預告排程與-np-completeness)
- [10. 一頁速查表](#10-一頁速查表)
- [11. 與後續講次的連結](#11-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

[00:00] 從一個用 mutex 的普通 queue 開始，**一步一步讓它更聰明**。接著講 condition variable、barrier、phaser。

上一講列了五個 queue 例子，例 1（wait-free SPSC queue）已經做完。本講做例 2–5：

| 例 | 同步方式 | 會 busy wait？ | 能 peek 並行？ |
|---|---|---|---|
| 2 | pthread mutex + 重試迴圈 | 會 | — |
| 3 | binary semaphore 當 lock + 重試迴圈 | 會 | — |
| 4 | 三個 semaphore（qlock、empty、full） | **不會** | — |
| 5 | 例 4 + reader-writer lock | 不會 | **會** |

---

## 1. 共同的架子

[00:00] 基本結構不變：**環狀 queue**，有 head、tail，enqueue / dequeue 時以取模的方式把 tail / head 加 1。

[00:30] 改變的是 `main`：

- **拿掉「單一生產者、單一消費者」的限制**。誰都可以 enqueue、誰都可以 dequeue。
- [01:00] 開一堆 thread：**偶數號是 dequeue，奇數號是 enqueue**。10 個 thread → 5 個生產者、5 個消費者。

[01:32] 而且這次是 **synchronous queue**（上一講的 wait-free queue 是 asynchronous）：

- [02:00] 要 dequeue 但 queue 是空的 → **等**，直到有東西進來。
- 要 enqueue 但 queue 是滿的 → **等**，直到有東西被拿走。

[02:00] thread 函式跟上一講一樣：重複呼叫 `enq` / `deq`，中間 `nap()`。

---

## 2. 例 2：用 mutex 的 queue

### 2.1 `volatile`

[04:30] 新東西：queue、head、tail 都宣告成 **`volatile int`**。

教授的說明：

- `volatile` 是告訴編譯器**不要把它放在暫存器**，要放在記憶體。
- [05:00] 這樣別的 thread 一改，**cache coherence 協定**（快取一致性協定，讓各核心的快取保持一致）就會自動把新值帶過來。
- 如果放在暫存器，值不會（至少不會及時）更新，也不會遵守記憶體模型，不會是 SC。

【課本補充】課本 §5.2 也這麼說：共享變數（head、tail、陣列）通常宣告成 `volatile`，存在快取而不是暫存器，靠 cache coherence 讓別的核心很快看到。

⚠️ 這個說法要小心：`volatile` **只**阻止編譯器把值留在暫存器、省略讀寫，**不會**產生 fence，也**不會**讓存取變成 atomic 或 SC。第 19 講的 store-buffering 實測裡 `x`、`y` 都是 `volatile`，照樣出現 `0, 0`。這個例子之所以正確，是因為所有存取都在 **mutex 保護的 critical section** 裡（第 19 講的定理），不是因為 `volatile`。kernel 的看法見 §2.4。

### 2.2 程式碼

[02:30] 宣告一把 global 的 pthread mutex，在 `main` 裡初始化：

```c
pthread_mutex_t qlock;
pthread_mutex_init(&qlock, NULL);   /* 第二個參數：屬性，這裡沒有 */
```

[03:01] 兩個巨集：

```c
#define LOCK(x)   (pthread_mutex_lock(&x))
#define UNLOCK(x) (pthread_mutex_unlock(&x))
```

[03:32] 教授強調這是**巨集**不是函式：你寫 `LOCK(qlock)`，展開後就是 `pthread_mutex_lock(&qlock)`。從 `pthread_mutex_lock` 返回，就表示拿到 lock 了。

課本 Listing 5.12：

```c
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
    } while (status == -1);
    return status;
}

int deq() {
    int val, status;
    do {
        LOCK(qlock);
        if (tail == head) status = -1;        /* empty */
        else {
            val = queue[head];
            head = INC(head);
            status = 0;
        }
        UNLOCK(qlock);
    } while (status == -1);
    return val;
}
```

[04:03] `enq`：

1. 一個 `do … while` 迴圈，`status == -1`（queue 滿了，沒放成功）就重來。
2. [06:02] 先拿 lock。
3. 檢查滿不滿：滿了 `status = -1`。
4. 沒滿：在 tail 放值、tail 加 1（取模），`status = 0`（成功）。
5. 放掉 lock。

[07:01] `deq` 一樣：拿 lock → 檢查空不空 → 不空就從 head 拿值、head 加 1 → 放 lock → 沒成功就重來。

### 2.3 為什麼 lock 要放在迴圈裡面

[08:01] 重點：**每一次嘗試都要拿 lock、做一次、放 lock**，不成功再重來。

[08:30] 如果把 `LOCK` 放到迴圈**外面**、`UNLOCK` 也放到外面，會怎樣？

- 例如 dequeue 時 queue 是空的。它拿著 lock 一直繞圈等東西進來。
- 但 enqueue 的 thread 拿不到 lock，永遠放不進東西。
- 教授說：「不只是不公平，是會 **deadlock**。」

所以要讓出 lock，讓別人有機會。

[09:01] 這是最簡單的平行 queue 之一。**不是 wait-free**（跟上一講的例 1 不同），但它是**通用的**：任意數量的生產者和消費者都可以。

### 2.4 【原始碼】kernel 對 `volatile` 的看法

kernel 文件有一篇專門講這件事，標題就叫「為什麼不該用 volatile」：

`Documentation/process/volatile-considered-harmful.rst:7-26`

```
C programmers have often taken volatile to mean that the variable could be
changed outside of the current thread of execution; as a result, they are
sometimes tempted to use it in kernel code when shared data structures are
being used.  In other words, they have been known to treat volatile types
as a sort of easy atomic variable, which they are not.  The use of volatile in
kernel code is almost never correct; this document describes why.

The key point to understand with regard to volatile is that its purpose is
to suppress optimization, which is almost never what one really wants to
do.  In the kernel, one must protect shared data structures against
unwanted concurrent access, which is very much a different task.  The
process of protecting against unwanted concurrency will also avoid almost
all optimization-related problems in a more efficient way.

Like volatile, the kernel primitives which make concurrent access to data
safe (spinlocks, mutexes, memory barriers, etc.) are designed to prevent
unwanted optimization.  If they are being used properly, there will be no
need to use volatile as well.  If volatile is still necessary, there is
almost certainly a bug in the code somewhere.  In properly-written kernel
code, volatile can only serve to slow things down.
```

重點翻譯：

- 很多人把 `volatile` 當成「簡單的 atomic 變數」，**它不是**。
- `volatile` 的用途是**壓制最佳化**，而真正需要的是**防止不想要的並行存取**，這是兩回事。
- spinlock、mutex、memory barrier 本身就會阻止不想要的最佳化。用對了，就不需要 `volatile`。「如果還需要 volatile，程式裡幾乎一定有 bug。」

同一篇文件第 35-50 行舉例：spinlock 本身就是 memory barrier，編譯器在它前後會「忘掉」對共享資料的假設。拿著鎖時資料不會被別人改，**拿著鎖時它就不是 volatile 的**；硬加 `volatile` 只會讓 critical section 裡的程式不能最佳化，變慢。

第 52 行：`volatile` 原本是給**記憶體對映 I/O 暫存器**用的。

所以教授的 queue 正確，是因為有 mutex；`volatile` 在這裡是多餘的。（kernel 自己需要「老實讀一次、寫一次」時用 `READ_ONCE` / `WRITE_ONCE`，第 19 講的 litmus test 裡看過。）

---

## 3. 例 3：binary semaphore 當 lock

[09:31] 把 semaphore 當成 lock 用（**binary semaphore**）。

[09:31] 複習：

- `wait`：count 減 1；已經是 0 就等，等別人 post。
- [10:02] `post`：count 加 1；如果是從 0 變 1，看看有沒有人在等，叫醒一個。

[10:30] `qlock` 改成 semaphore，在 `main` 初始化：

```c
sem_t qlock;
sem_init(&qlock, 0, 1);   /* 第 2 個參數 0：只在同一個 thread group 的 thread 之間共享
                             （1 = 跨 process 共享）；第 3 個參數：初始值 1 */
...
sem_destroy(&qlock);
```

[11:01] 用法保證 count 永遠在 0 和 1 之間：**1 = 空閒，0 = 被占用**。（跟 lock 的 0/1 意思剛好相反。）

[11:31] 程式碼**完全不用改**，只要改兩行巨集：

```c
#define LOCK(x)   (sem_wait(&x))
#define UNLOCK(x) (sem_post(&x))
```

[12:02] 教授把兩個檔案並排逐行比，`enq`、`deq` **一模一樣**。

[13:02] 教授說這是「寫程式的魔法」：**高層的改變不需要動到低層**。只改 `LOCK`/`UNLOCK` 的定義就好。

[13:32] 為什麼 `sem_wait` = lock：count 是 1 表示沒人拿，把 1 變 0 就拿到了。之後來的人看到 0 就等，直到有人 post。[14:00] 每個 `sem_wait` 都配一個 `sem_post`，最後沒人搶時 count 回到 1。

【課本補充】有人會說「連續 post 好幾次，count 不就大於 1 了？」對，但假設程式設計師有紀律，每個 `LOCK` 都配一個 `UNLOCK`，就不會發生。

[13:02] 不過這樣**還沒用到 semaphore 真正的能力**，而且還是 busy wait（滿了 / 空了就一直重試）。

---

## 4. 例 4：三個 semaphore，不 busy wait

### 4.1 程式碼

[14:31] 這次真的用 semaphore 的能力。空/滿的檢查可以很自然地交給 semaphore 做。

[15:01] 巨集改叫 `WAIT` / `POST`，因為不再只是 binary semaphore：

```c
#define WAIT(x) (sem_wait(&x))
#define POST(x) (sem_post(&x))
sem_t qlock, empty, full;

int enq(int val) {
    WAIT(empty);              /* 要有空格 */
    WAIT(qlock);
    queue[tail] = val;
    tail = INC(tail);
    POST(qlock);
    POST(full);               /* 多了一個有東西的格子 */
    return 0; /* success */
}

int deq() {
    WAIT(full);               /* 要有東西 */
    WAIT(qlock);
    int val = queue[head];
    head = INC(head);
    POST(qlock);
    POST(empty);              /* 多了一個空格 */
    return val;
}

int main() {
    sem_init(&qlock, 0, 1);
    sem_init(&empty, 0, BUFSIZE);
    sem_init(&full, 0, 0);
    ...
}
```

[15:31] 三個 semaphore：

| semaphore | 種類 | 初始值 | 意思 |
|---|---|---|---|
| `qlock` | binary | 1 | 當 lock，保護 head、tail、陣列 |
| `empty` | 一般 | `BUFSIZE` | **空格數**。一開始整個 queue 都是空的 |
| `full` | 一般 | 0 | **有東西的格子數**。一開始沒有 |

[16:01] 穩定時：**empty + full = 格子總數**。

### 4.2 empty 與 full：把格子搬來搬去

[16:30] 想法：把「**更新 queue**」和「**檢查空/滿**」分開。

[17:00] **`enq`**：

- 中間四行（`WAIT(qlock)` … `POST(qlock)`）：更新 queue，上一講看過。
- [18:01] 前面 `WAIT(empty)`：檢查有沒有空格。假設 `BUFSIZE = 10`，每 enqueue 一次就 10 → 9 → 8 → …；變成 0 就**等**，直到不是 0。
- [18:30] 後面 `POST(full)`：放進一個東西，就多了一個「有東西的格子」。

> `WAIT(empty)` 配 `POST(full)`：從 empty 拿走一格，搬到 full。

[19:02] **`deq`** 反過來：

- [19:32] `WAIT(full)`：至少要有一個東西。是 0 就等，否則減 1。
- `POST(empty)`：拿走一個，就多了一個空格。

> `WAIT(full)` 配 `POST(empty)`：從 full 拿走一格，搬回 empty。方向相反。

[20:01] 好處：

1. **自然**：用 semaphore 就能在 enqueue 和 dequeue 之間互相通知，空/滿判斷變得很簡單。
2. **critical section 變小**：以前空/滿的檢查在 lock 裡面，現在移到外面了。
3. [20:31] **更 scalable、更自然**。這才是 semaphore 的強項。

【課本補充】課本補充的細節：

- `WAIT(empty)` 成功，表示**至少有一個空格**，可以想成那一格**隱含地被保留**給這個 thread 了。所以進了 critical section **不用再檢查滿不滿**。
- `deq` 同理：`WAIT(full)` 成功，那個東西就會一直在，直到被拿走。
- `qlock` 還是需要，讓程式沒有 data race。
- **沒有 busy wait**：count 不是 0 就馬上拿到，是 0 就被換出去、放進 kernel 的等待 queue，不會霸占 CPU。

### 4.3 【實測】CPU 用量差 25 倍

把例 2、3、4 寫進同一個檔案（`q.c`，用 `-DMODE=1/2/3` 切換）。5 個生產者、5 個消費者，`BUFSIZE = 10`，每個生產者放 200 個、每放一個睡 1 ms（生產者慢，消費者常常遇到空 queue）：

```
MODE 1: in=99500 out=99500  CPU user 0.26s sys 0.74s
  wall 0.22s
MODE 2: in=99500 out=99500  CPU user 0.26s sys 0.78s
  wall 0.22s
MODE 3: in=99500 out=99500  CPU user 0.00s sys 0.04s
  wall 0.22s
```

| | 例 2 mutex | 例 3 binary sem | 例 4 三個 sem |
|---|---|---|---|
| 放進／拿出的總和 | 99500 / 99500 | 99500 / 99500 | 99500 / 99500 |
| 實際時間 | 0.22 s | 0.22 s | 0.22 s |
| **CPU 時間** | **1.00 s** | **1.04 s** | **0.04 s** |

- 三個版本都**正確**（放進去的總和 = 拿出來的總和）。
- 都花一樣久，因為瓶頸是生產者的 `nap()`。
- 但例 2、3 的 5 個消費者在 queue 空的時候**一直繞圈重試**，0.22 秒內燒掉超過 1 秒的 CPU（多個核心一起燒）。例 4 的消費者直接睡著，CPU 時間只有 0.04 秒，差 **25 倍**。
- 例 2、3 的 `sys` 時間很高：mutex 和 semaphore 在搶不到時會進 kernel（futex），一直重試就一直進出 kernel。

---

## 5. 例 5：加上 `peek`，用 reader-writer lock

[20:31] 再加一個 **`peek`** 函式：看 head 的值，**不拿走**。

[21:02] enqueue、dequeue 會改 queue 的狀態 → **寫**。`peek` 只是看 → **讀**。所以用 reader-writer lock：多個讀者可以一起 peek，或一個寫者做 enqueue / dequeue。

[21:31] 四個 semaphore（課本 Listing 5.15）：

```c
sem_t rwlock, read_lock, full, empty;
```

[22:00] reader-writer lock 需要兩把鎖（`rwlock` 和 `read_lock`），做成兩個 binary semaphore；`empty`、`full` 照舊。`main` 這次開三種 thread：enqueue、dequeue、peek。

```c
int peek() {
    /* This is a read function */
    get_read_lock();
    int val = (head == tail) ? -1 : queue[head];
    release_read_lock();
    return val;
}

int enq(int val) {
    WAIT(empty);
    /* Get the write lock and perform the enqueue */
    get_write_lock();
    queue[tail] = val;
    tail = INC(tail);
    release_write_lock();
    POST(full);
    return 0; /* success */
}

int deq() {
    int val;
    WAIT(full);
    /* Get the write lock and perform the dequeue */
    get_write_lock();
    val = queue[head];
    head = INC(head);
    release_write_lock();
    POST(empty);
    return val;
}
```

（課本程式碼寫作 `peak`，應是 `peek`。）

[22:31] **`peek`**：

- 拿讀鎖。[23:01] 怎麼實作的「不關我的事」。
- [24:03] 空的就回 −1，否則回 head 的值。`peek` 是 **wait-free** 的：空了就直接回，不等。
- 拿著讀鎖，就不會有寫者來搗亂。放掉讀鎖。

【口頭】[23:01] 教授說這是寫程式的重要原則：**從使用者的角度寫**。「我會用這些函式，我給它們這些，期待拿回那些，它們怎麼做到的我不在乎。」這樣把大問題拆成一堆小問題。[24:31] 他叫這種風格「宣告式」：說我需要什麼，怎麼實作之後再說，或交給別人。

[24:31] **`enq` / `deq`**：`WAIT(empty)` / `POST(full)`（或反過來）照舊。唯一的改變是把 `qlock` 換成 `get_write_lock()` / `release_write_lock()`。[25:30] 「我只要拿到寫的許可。」

[26:00] reader-writer lock 本身的實作就是第 20 講的 Listing 5.10，只是 `LOCK`/`UNLOCK` 換成 semaphore 的 wait/post。

[27:02] 教授再提一次：讀者在時，寫者會被擋住。最後一個讀者走了才放 `rwlock`，否則會「永遠」擋住寫者，那不公平。

[28:00] 【口頭】程式碼可以從課程網站下載來跑。

【課本補充】

- 課本說 pthreads 在某些平台有提供 `pthread_rwlock_t`，但這裡用自己寫的。
- `enq` / `deq` 換成寫鎖其實沒差（沒有其他並行的讀者或寫者），好處在 `peek` 可以多個一起跑。
- 用 semaphore 當 `rwlock` 還有一個好處：第 20 講提到「拿 `rwlock` 的是第一個讀者、放掉的是最後一個讀者」。semaphore 沒有擁有者的概念，誰都能 post，所以這樣做是合法的。

---

## 6. Condition variable

### 6.1 用法

[28:00] semaphore 可以看成一種通用的 OS 機制。**condition variable（條件變數）**也是通用的。

[28:30] condition variable 一定**配一把 lock**。它提供一個通用的方法，讓一個 thread **送信號叫醒**另一個 thread。

[29:01] 跟 semaphore 比：semaphore 的介面很明確（wait / post）。condition variable 更通用：「我就是要等，等別人通知我。」

```c
pthread_mutex_t mlock;
pthread_cond_t cond;

/* 等待 */
pthread_mutex_lock(&mlock);
pthread_cond_wait(&cond, &mlock);
pthread_mutex_unlock(&mlock);

/* 叫醒 */
pthread_mutex_lock(&mlock);
pthread_cond_signal(&cond);
pthread_mutex_unlock(&mlock);
```

[29:32] **等待**：

1. 先拿 mutex（主要是保護 condition variable 內部的狀態）。
2. `pthread_cond_wait(&cond, &mlock)`：在這個條件變數上等，用這把 mutex 保護它的內部資料。
3. [29:32] 一開始等，**函式庫就把 `mlock` 放掉**。這樣別人才能拿到 mutex 來通知你。

[30:01] **叫醒**：另一個 thread 也先拿 mutex，再 `pthread_cond_signal`。

- signal 會叫醒**任意一個**正在等這個條件變數的 thread。
- [30:30] 被叫醒的 thread 會**重新拿回 lock**，做點事，最後放掉。

[30:30] `pthread_cond_broadcast` 會叫醒**所有**等待者。

### 6.2 【課本補充＋實測】lost wakeup

課本 Point 5.1.5（第 20 講筆記 §3 也有）：condition variable **沒有記憶**。signal 只叫醒「那一刻」正在等的 thread；那時沒人等，signal 就**丟了**。semaphore 則記在 count 裡。

【實測】`lost.c`：先 signal / post，再 wait（限時 1 秒）：

```
cond var : signal then wait -> timed out (wakeup lost)
semaphore: post then wait   -> got it immediately (count remembered)
```

所以實務上 `pthread_cond_wait` 一定要搭配一個**條件**（例如 `while (queue_empty) pthread_cond_wait(...)`），條件本身存在共享變數裡，由 mutex 保護。條件已經成立就根本不用等，這樣就不怕 signal 先到。

### 6.3 【原始碼】kernel 的 `wait_event` 與 `completion`

kernel 裡沒有叫 "condition variable" 的東西，對應的是**等待 queue**加上一個條件：

`include/linux/wait.h:340-346`

```c
#define wait_event(wq_head, condition)						\
do {										\
	might_sleep();								\
	if (condition)								\
		break;								\
	__wait_event(wq_head, condition);					\
} while (0)
```

`include/linux/wait.h:300-322`（`__wait_event` 最後展開成這個）

```c
#define ___wait_event(wq_head, condition, state, exclusive, ret, cmd)		\
({										\
	__label__ __out;							\
	struct wait_queue_entry __wq_entry;					\
	long __ret = ret;	/* explicit shadow */				\
										\
	init_wait_entry(&__wq_entry, exclusive ? WQ_FLAG_EXCLUSIVE : 0);	\
	for (;;) {								\
		long __int = prepare_to_wait_event(&wq_head, &__wq_entry, state);\
										\
		if (condition)							\
			break;							\
										\
		if (___wait_is_interruptible(state) && __int) {			\
			__ret = __int;						\
			goto __out;						\
		}								\
										\
		cmd;								\
	}									\
	finish_wait(&wq_head, &__wq_entry);					\
__out:	__ret;									\
})
```

| 行 | 說明 |
|---|---|
| 343-344 | 條件已經成立 → 根本不睡。這就是避開 lost wakeup 的方法 |
| 308 | `prepare_to_wait_event`：**先**把自己掛上等待 queue、設成睡眠狀態 |
| 310-311 | **再**檢查一次條件。成立就跳出 |
| 313-316 | 可被 signal 打斷的等待：收到 signal 就離開 |
| 318 | `cmd` 通常是 `schedule()`：真的去睡 |
| 307 | 醒來後回到迴圈開頭，**再檢查一次條件**（可能被叫醒了但條件又不成立） |

關鍵順序：**先掛上 queue，再檢查條件，最後才睡**。如果別人在「檢查條件」和「睡覺」之間 `wake_up`，因為我已經在 queue 上了，就會被叫醒，不會漏掉。這和 pthread 要求「拿著 mutex 呼叫 `pthread_cond_wait`，由函式庫放 mutex」是同一個目的。

如果真的需要「有記憶」的版本，kernel 有 **`completion`**：

`include/linux/completion.h:26-29`

```c
struct completion {
	unsigned int done;
	struct swait_queue_head wait;
};
```

`kernel/sched/completion.c:28-38`

```c
void complete(struct completion *x)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&x->wait.lock, flags);

	if (x->done != UINT_MAX)
		x->done++;
	swake_up_locked(&x->wait);
	raw_spin_unlock_irqrestore(&x->wait.lock, flags);
}
```

`kernel/sched/completion.c:70-95`（`wait_for_completion` 的核心，節錄）

```c
	if (!x->done) {
		...
		do {
			...
			__prepare_to_swait(&x->wait, &wait);
			__set_current_state(state);
			raw_spin_unlock_irq(&x->wait.lock);
			timeout = action(timeout);
			raw_spin_lock_irq(&x->wait.lock);
		} while (!x->done && timeout);
		...
	}
	if (x->done != UINT_MAX)
		x->done--;
```

- `done` 就是一個計數，`complete()` 加 1。
- `wait_for_completion` 看到 `done > 0` 就**不用睡**，直接減 1 走人。
- 所以「先 complete、後 wait」不會漏掉：跟 semaphore 一樣**有記憶**。
- 結構和第 20 講的 kernel semaphore 幾乎一樣：一個計數 + 一個等待 queue + 一把 spinlock。

| | 有記憶？ | 先通知後等待 |
|---|---|---|
| pthread condition variable | 沒有 | 漏掉（要自己配條件變數） |
| kernel `wait_event` | 條件本身就是記憶 | 條件已成立就不睡 |
| semaphore、kernel `completion` | 有（count / done） | 直接拿到 |

---

## 7. Barrier 與 phaser

[31:01] **Barrier** 在高效能運算裡用得很多。

例子：加總 100 萬個數，有 10 個 thread。

```
 陣列：[ 塊0 | 塊1 | 塊2 | ... | 塊9 ]
         │     │     │           │
        s0    s1    s2   ...    s9      ← 每個 thread 算自己那塊的部分和
         └─────┴─────┴─── ... ───┘
                   │
              ═════╪═════  barrier：大家都到了才放行
                   │
            s0 + s1 + ... + s9           ← 某個 thread 負責加總
```

[31:32] 要等**所有** thread 都算完，才能把部分和收起來加。這像 map-reduce。

[32:00] 做法：定一個共同的點叫 **barrier**，讓所有 thread 都到。先到的就停在那裡等。**最快的 thread 要等最慢的 thread。** 全部到齊後，一次全部放行。

[32:32] 之後可以派一個 thread 去收部分和、加起來、印結果，其他 thread 去做別的事。

[33:01] **Phaser**：更進階的 barrier。

> **所有 thread 都到了 Point 1，才能有 thread 越過 Point 2。**

[33:31] barrier 是 Point 1 = Point 2 的特例。phaser 的 Point 2 比較後面：

- 到了 Point 1 之後，可以**稍微往前走一點**，但不能走太遠。
- 走太遠的話，thread 之間的落差會越來越大。
- 所以到 Point 2 就停，等大家都到 Point 1。

[34:03] 用剛才的例子：

- **Point 1**：所有部分和都算好了（大家都用完舊陣列、拿到結果了）。
- **Point 2**：要把這個陣列**換成另一個陣列**的地方。
- [34:30] 在大家都用完舊陣列之前，不能換。所以等大家都到 Point 1，某個 thread 才能越過 Point 2 去換陣列。

【課本補充】課本補充：

- 這個 barrier 跟 memory barrier（fence）**不是**同一個東西，只是名字一樣。
- 幾乎所有平行程式語言都支援 barrier。超級電腦有專門的 barrier 硬體。
- phaser 比較少見，Java 等語言有。做 pipeline 式的計算時很有用。

---

## 8. 【課本補充】Linux message queue

本講沒講，課本 §5.2.6 有。

### 8.1 用法與【實測】

要用 kernel 的 IPC（行程間通訊）機制，先產生一個唯一的 key，再用它找到（或建立）一個 message queue：

```c
#define MAX_TEXT 512
struct msg_buffer {
    long msg_type;                 /* 訊息類型 */
    char msg_text[MAX_TEXT];       /* 內容 */
};

key_t key = ftok("msgfile", 65);                  /* 產生 key */
int msgid = msgget(key, 0666 | IPC_CREAT);        /* 建立 / 取得 queue */

/* 送 */
msgsnd(msgid, &message, sizeof(message.msg_text), 0);

/* 收：只收 type 1 */
msgrcv(msgid, &message, sizeof(message.msg_text), 1, 0);

/* 刪掉 queue */
msgctl(msgid, IPC_RMID, NULL);
```

- kernel 內部維護一個 `msg_buffer` 的 queue。
- `msgrcv` 會拿**符合類型的最早那一則**訊息，填進你給的 buffer（使用者空間，這裡是 stack 上的變數）。

【實測】`mq.c`：父 process 送兩則（type 1、再 type 2），子 process **先收 type 2**、再收 type 1：

```
child  received type 2: second message (type 2)
child  received type 1: Hello from sender (type 1)
parent sent two messages
```

`strace -f` 看系統呼叫：

```
msgget(0x41084013, IPC_CREAT|0666)      = 1
[pid 214362] msgsnd(1, {mtype=1, mtext="Hello from sender (type 1)..."}, 512, 0) = 0
[pid 214362] msgsnd(1, {mtype=2, mtext="second message (type 2)..."}, 512, 0 <unfinished ...>
[pid 214363] msgrcv(1,  <unfinished ...>
...
[pid 214363] <... msgrcv resumed>{mtype=2, ...}, 512, 2, 0) = 512
[pid 214363] msgrcv(1, {mtype=1, ...}, 512, 1, 0) = 512
msgctl(1, IPC_RMID, NULL)               = 0
```

- 每個操作都是一個系統呼叫，queue 在 kernel 裡。
- type 1 先進 queue，但子 process 指定要 type 2，所以拿到的是第二則。**不是單純的 FIFO，是「同類型裡的 FIFO」。**

### 8.2 【原始碼】`msg_queue` 與 `find_msg`

`ipc/msg.c:49-63`

```c
struct msg_queue {
	struct kern_ipc_perm q_perm;
	time64_t q_stime;		/* last msgsnd time */
	time64_t q_rtime;		/* last msgrcv time */
	time64_t q_ctime;		/* last change time */
	unsigned long q_cbytes;		/* current number of bytes on queue */
	unsigned long q_qnum;		/* number of messages in queue */
	unsigned long q_qbytes;		/* max number of bytes on queue */
	struct pid *q_lspid;		/* pid of last msgsnd */
	struct pid *q_lrpid;		/* last receive pid */

	struct list_head q_messages;
	struct list_head q_receivers;
	struct list_head q_senders;
} __randomize_layout;
```

| 欄位 | 說明 |
|---|---|
| `q_perm` | 權限（`0666` 存在這裡）和 key |
| `q_qbytes` | 最多幾 bytes：這是一個 **bounded queue** |
| `q_messages` | 訊息串列（又是 `list_head`） |
| `q_receivers` | 在等訊息的接收者：queue **空**時睡在這裡（對應 `full` semaphore 的等待者） |
| `q_senders` | 在等空間的傳送者：queue **滿**時睡在這裡（對應 `empty` semaphore 的等待者） |

所以 kernel 的 message queue 就是本講例 4 的 **synchronous bounded queue**，只是兩個 semaphore 換成兩個等待串列。

每則訊息：

`include/linux/msg.h:9-16`

```c
struct msg_msg {
	struct list_head m_list;
	long m_type;
	size_t m_ts;		/* message text size */
	struct msg_msgseg *next;
	void *security;
	/* the actual message follows immediately */
};
```

`m_type` 就是 `msg_type`。收訊息時照類型找：

`ipc/msg.c:1074-1096`（節錄）

```c
static struct msg_msg *find_msg(struct msg_queue *msq, long *msgtyp, int mode)
{
	struct msg_msg *msg, *found = NULL;
	long count = 0;

	list_for_each_entry(msg, &msq->q_messages, m_list) {
		if (testmsg(msg, *msgtyp, mode) &&
		    !security_msg_queue_msgrcv(&msq->q_perm, msg, current,
					       *msgtyp, mode)) {
			...
			} else
				return msg;
			count++;
		}
	}

	return found ?: ERR_PTR(-EAGAIN);
}
```

- 從頭（最早的）走過整個串列，回傳**第一則類型符合的**。這就是實測裡「先拿到 type 2」的原因。
- 都沒有就回 `-EAGAIN`，`do_msgrcv` 就讓接收者睡到 `q_receivers` 上。

---

## 9. 預告：排程與 NP-completeness

[34:30] 下一講開始看 **kernel 裡面**發生什麼事，以及**排程**相關的問題。

[35:01] 教授的建議：要理解排程，特別是**多核心排程**，一定要對 **NP-completeness（NP 完全）**有扎實的觀念。這是整個排程單元的先修。

- 本講的同步不需要 NP-completeness。
- [35:30] **即時排程**很好玩，只需要簡單的算術。
- 但多核心排程那部分需要 NP-completeness 的知識，請先準備好。

---

## 10. 一頁速查表

### 10.1 五個 queue

| 例 | 生產者／消費者 | 同步 | busy wait | 特色 |
|---|---|---|---|---|
| 1 | 1 / 1 | atomics，無鎖 | — | wait-free（第 20 講） |
| 2 | 多 / 多 | mutex + 重試 | 會 | 最簡單的通用 queue |
| 3 | 多 / 多 | binary semaphore + 重試 | 會 | 只改兩行巨集 |
| 4 | 多 / 多 | qlock + empty + full | **不會** | critical section 變小；CPU 少 25 倍 |
| 5 | 多 / 多 | 例 4 + RW lock | 不會 | 多個 `peek` 可並行 |

### 10.2 例 4 的配對

| 函式 | 開頭 | 結尾 |
|---|---|---|
| `enq` | `WAIT(empty)` | `POST(full)` |
| `deq` | `WAIT(full)` | `POST(empty)` |

初始：`qlock = 1`、`empty = BUFSIZE`、`full = 0`。

### 10.3 同步原語比較

| | 有記憶 | 用途 |
|---|---|---|
| mutex | — | 互斥 |
| semaphore | 有（count） | 互斥、計數、bounded queue |
| condition variable | 沒有（會 lost wakeup） | 通用的等待／通知，一定配一把 mutex |
| barrier | — | 大家到齊再一起走 |
| phaser | — | 大家都到 Point 1，才能越過 Point 2 |

### 10.4 `volatile`

只防止編譯器把值留在暫存器。**不是** atomic、**不是** fence。正確性靠 lock。kernel：「volatile considered harmful」。

### 10.5 本講看過的原始碼位置

| 位置 | 內容 |
|---|---|
| `Documentation/process/volatile-considered-harmful.rst:7-26` | 為什麼不該用 `volatile` |
| `include/linux/wait.h:340-346` | `wait_event`：條件成立就不睡 |
| `include/linux/wait.h:300-322` | `___wait_event`：先掛 queue、再查條件、再睡 |
| `include/linux/completion.h:26-29` | `struct completion`：`done` 計數 + 等待 queue |
| `kernel/sched/completion.c:28-38` | `complete()`：`done++` 並叫醒 |
| `kernel/sched/completion.c:70-95` | `do_wait_for_common`：`done` > 0 就不睡 |
| `ipc/msg.c:49-63` | `struct msg_queue`：訊息串列 + 接收者／傳送者等待串列 |
| `include/linux/msg.h:9-16` | `struct msg_msg` |
| `ipc/msg.c:1074-1096` | `find_msg`：找第一則類型符合的訊息 |

---

## 11. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **kernel 的 spinlock、mutex、semaphore、lockdep** | **第 22 講**：Concurrency in the kernel |
| **讀者完全不用鎖的另一種 reader-writer 方案** | **第 23 講**：RCU |
| **等待 queue、`schedule()`、被叫醒** | **第 24–26 講**：排程 |
| **NP-completeness** | **第 24–25 講**：Scheduling、Multiprocessor scheduling |
| **即時排程只要簡單算術** | **第 27 講**：Real-time schedulers |

（講次對應依播放清單標題推估。）

---

## 附：本講一句話總結

> **把 queue 一步步做聰明：例 2 用 mutex 包住每一次嘗試（lock 一定要在重試迴圈裡面，不然會 deadlock）；例 3 只把 `LOCK`/`UNLOCK` 巨集換成 `sem_wait`/`sem_post`，其他一行都不用改；例 4 用三個 semaphore（qlock、empty = BUFSIZE、full = 0），`enq` 從 empty 搬一格到 full、`deq` 搬回來，空/滿檢查移出 critical section 也不再 busy wait（實測結果一樣、時間一樣，CPU 卻少了 25 倍）；例 5 再加讀寫鎖讓多個 `peek` 並行。Condition variable 配一把 mutex 做通用的等待／通知，但沒有記憶，先 signal 後 wait 會漏掉（實測：cond var 逾時、semaphore 直接拿到），kernel 的 `wait_event` 用「先掛 queue、再查條件、再睡」避開，`completion` 則像 semaphore 一樣有計數。Barrier 讓大家到齊再走，phaser 允許先走一段但不能越過 Point 2。教授的 queue 用 `volatile`，但正確性其實來自鎖，kernel 文件直說 volatile 有害。Linux 的 System V message queue 就是 kernel 裡的同步 bounded queue，可依類型收訊息。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 21（Chapter 5, Part IV）錄影逐字稿（YouTube 自動字幕，35:48）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§5.1.9、§5.1.11、§5.2.2–5.2.6，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
