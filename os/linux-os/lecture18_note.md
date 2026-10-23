# Operating Systems: A Linux Kernel-Oriented Approach 第十八講完整筆記：多執行緒與同步基礎（Chapter 5, Part I）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 18（Chapter 5, Part I：Multithreading）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（1:08:58，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）第 5 章開頭與 §5.1.1–5.1.5（p.161–180）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
> **標記說明**：
> - `【口頭】`：教授在課堂上口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與教授回答
> - `【課本補充】`：課本有、但這堂課沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（`檔名:行號`），用來對照觀念
> - `【實測】`：在本機（Ubuntu、kernel 6.5.0、20 個 CPU）實際編譯執行的結果
> - `⚠️ 逐字稿缺字`：自動字幕在這裡掉字或聽不清，內容依上下文或課本補上
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 第 5 章的地圖](#0-第-5-章的地圖)
- [1. Data race：`count++` 為什麼會錯](#1-data-racecount-為什麼會錯)
  - [1.1 一行 C = 三道指令](#11-一行-c--三道指令)
  - [1.2 壞的交錯](#12-壞的交錯)
  - [1.3 Critical section 與 lock](#13-critical-section-與-lock)
  - [1.4 【實測】4 個 thread 各加 100 萬次](#14-實測4-個-thread-各加-100-萬次)
- [2. 最簡單的 lock：TTAS](#2-最簡單的-lockttas)
  - [2.1 Test，再 test-and-set](#21-test再-test-and-set)
  - [2.2 Atomic 是什麼意思](#22-atomic-是什麼意思)
  - [2.3 為什麼 TTAS 不快](#23-為什麼-ttas-不快)
  - [2.4 Unlock 與 fence](#24-unlock-與-fence)
  - [2.5 【原始碼】x86 的 `lock` 前綴與 memory barrier](#25-原始碼x86-的-lock-前綴與-memory-barrier)
- [3. Data race 的理論](#3-data-race-的理論)
  - [3.1 Happens-before](#31-happens-before)
  - [3.2 正式定義](#32-正式定義)
  - [3.3 Properly labeled（PL）程式](#33-properly-labeledpl程式)
- [4. Deadlock](#4-deadlock)
  - [4.1 四個必要條件](#41-四個必要條件)
  - [4.2 怎麼對付每個條件](#42-怎麼對付每個條件)
  - [4.3 哲學家吃飯問題](#43-哲學家吃飯問題)
  - [4.4 【實測】對稱 vs 打破對稱](#44-實測對稱-vs-打破對稱)
  - [4.5 【課本補充】預防、避免、復原](#45-課本補充預防避免復原)
- [5. Starvation 與 livelock](#5-starvation-與-livelock)
- [6. Pthreads](#6-pthreads)
  - [6.1 `pthread_create`](#61-pthread_create)
  - [6.2 回傳值：為什麼是 `void **`](#62-回傳值為什麼是-void-)
  - [6.3 【實測】跑課本 Listing 5.2](#63-實測跑課本-listing-52)
  - [6.4 【實測＋原始碼】pthread 底下是 `clone3` 和 futex](#64-實測原始碼pthread-底下是-clone3-和-futex)
- [7. 用三種方法加 `count`](#7-用三種方法加-count)
  - [7.1 Mutex](#71-mutex)
  - [7.2 Atomic fetch-and-add](#72-atomic-fetch-and-add)
  - [7.3 Compare-and-swap（CAS）](#73-compare-and-swapcas)
  - [7.4 【原始碼】kernel 的 `atomic_inc`、`xadd`、`cmpxchg`](#74-原始碼kernel-的-atomic_incxaddcmpxchg)
  - [7.5 【實測】編譯器產生的指令](#75-實測編譯器產生的指令)
  - [7.6 Lock-free 與 wait-free](#76-lock-free-與-wait-free)
- [8. 一頁速查表](#8-一頁速查表)
- [9. 與後續講次的連結](#9-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 第 5 章的地圖

[00:33] 第 5 章講**同步（synchronization）與排程（scheduling）**。教授說這是管理計算資源最重要的一章。

[01:00] 一樣分四部分：

| 部分 | 內容 |
|---|---|
| 同步：使用者空間 | data race、lock、pthreads、semaphore、condition variable、queue、reader-writer lock |
| 同步：kernel 空間 | spinlock、kernel mutex、lockdep、RCU |
| 一般排程 | 排程分類、Banker's algorithm、CFS、RT／Deadline 排程 |
| 即時系統排程 | EDF、RMS/DMS、priority inheritance 等協定 |

教授特別說：使用者空間的同步和 kernel 空間的同步**不一樣**。

【口頭】[00:33] 教授開玩笑說他會用印度英語的念法 "shed-ule" 而不是 "sked-ule"。

【口頭】[01:32] 「即時（real-time）」的意思：例如飛彈、太空相關的系統，有**絕對、硬性的截止時間**，所以要另一套演算法。

【口頭】[01:32] 名詞：Linux 把多核心 CPU 裡的每個核心都叫做一顆「CPU」。本講也這樣叫。

---

## 1. Data race：`count++` 為什麼會錯

### 1.1 一行 C = 三道指令

[01:32] 兩個 thread 跑同一個程式，共享一個 global 變數 `count`，都做 `count++`。

[02:02] `count++` 翻成組合語言是三道指令（課本 Listing 5.1）：

```c
t1 = count      // 從記憶體讀到暫存器
t1 = t1 + 1     // 暫存器加 1
count = t1      // 寫回記憶體
```

### 1.2 壞的交錯

[02:32] 兩個假設（課本 §5.1.1）：

1. 每道指令本身是 atomic（看起來在一瞬間做完）。
2. **兩道指令之間的延遲可以任意長**。例如中間剛好發生 context switch，很久之後才回來。所以不能對時間做任何假設。

[03:31] 所以可能出現這種交錯：

```
count = 0
Thread 1          Thread 2
t1 = count
                  t2 = count     ← 兩個都讀到 0
t1 = t1 + 1
                  t2 = t2 + 1    ← 兩個都變 1
count = t1
                  count = t2     ← 最後 count = 1，應該是 2
```

[04:31] 問題在於兩段三指令的序列**重疊**了。如果它們一前一後、不重疊，就沒事。

### 1.3 Critical section 與 lock

[04:31] 這三道指令叫 **critical section（CS，臨界區段）**。規則：同一時間只能有一個 thread 在裡面。

[05:00] 做法：

1. 先**取得 lock**。lock 是「執行 critical section 的許可證」，同一時間只發一張。
2. 執行 critical section。
3. **釋放 lock（unlock）**，別人才能拿。

[06:00] 結果：就 critical section 來說，平行的 thread 變成**一個接一個**跑。離開 critical section 後又可以平行。這樣保證正確。

> **課本 Definition 5.1.1**：多個 thread 同時存取同一個共享變數、且至少一個是寫入，就可能出現不直覺的結果，這叫 data race（非正式定義）。這種程式碼要包在 lock–unlock 之間，叫 critical section。

### 1.4 【實測】4 個 thread 各加 100 萬次

把本講的四種做法放進同一個程式（`race.c`，`gcc -O0 -pthread`）。每個 thread 在迴圈裡同時用四種方法各加一次：

```c
plain++;                                              /* 沒保護：data race */
pthread_mutex_lock(&m); locked++; pthread_mutex_unlock(&m);
atomic_fetch_add(&fa, 1);                             /* atomic */
long old = atomic_load(&cas), new;
do { new = old + 1; }                                 /* CAS 迴圈 */
while (!atomic_compare_exchange_strong(&cas, &old, new) && (atomic_fetch_add(&casfail, 1), 1));
```

結果：

```
expected 4000000
plain    3935548
mutex    4000000
fetchadd 4000000
cas      4000000 (failed CAS: 224271)
```

- 沒保護的 `plain` 少了約 6 萬次。再跑一次是 `3930297`，每次都不一樣。
- 其他三種都剛好 400 萬。
- CAS 失敗了 22 萬多次，失敗就重試（見 §7.3）。

用 `objdump` 看 `plain++`，真的是三道指令：

```
1214:	mov    0x2e25(%rip),%rax        # 4040 <plain>
121b:	add    $0x1,%rax
121f:	mov    %rax,0x2e1a(%rip)        # 4040 <plain>
```

---

## 2. 最簡單的 lock：TTAS

### 2.1 Test，再 test-and-set

[06:31] lock 就是**一個記憶體位址**（100 個 lock 就有 100 個位址）。慣例：

- 存 0 → lock 是空的。
- 存 1 → lock 被占用。

上鎖 = 想辦法把 0 變成 1。成功就拿到 lock，直到持有者把 1 改回 0，別人都拿不到。

[07:01] 缺點：如果持有 lock 的 thread 被 context switch 換掉，其他人全都只能乾等。

【口頭】[07:35] 怎麼有效率地實作 lock，本身就是一個研究領域，有很多論文和書。

[07:35]–[10:32] 流程（課本 Figure 5.6）：

```
           ┌──────────────────────────────┐
           ▼                              │
   Test：位址裡是 0 嗎？ ── 不是 ─────────┘   （一直讀，只是讀）
           │ 是
           ▼
   Test-and-set：atomically 把 0 設成 1
           │
     成功 ─┼─ 失敗（別人搶先了）→ 回到 Test
           ▼
   進入 critical section
```

- **Test**：一直讀，看是不是 0。不是 0 就不要白費力氣去寫。
- **Test-and-set**：看到 0 才試著「檢查是 0、馬上設成 1」，一次做完。成功回傳 1，失敗回傳 0。

所以叫 **test-and-test-and-set（TTAS）lock**。

[09:30] 【口頭】這是一種「信任制」：thread 可以說謊，但程式是你寫的，沒理由說謊。

[10:00] Test-and-set 這一步**需要硬體支援**，一般的讀和寫做不到。

[33:00] 這種一直繞圈檢查的等待叫 **busy waiting**，這種 lock 叫 **spin lock**。會耗 CPU 時間和能量，但沒做有用的事。

### 2.2 Atomic 是什麼意思

[08:31] **Atomic（不可分割）**：沒有人看得到中間狀態。要嘛還沒開始，要嘛已經做完。它不一定真的瞬間完成，但**看起來**像瞬間完成。

【口頭】[60:30] 教授說他們有一整門課在講「看起來（appears）」這個字到底是什麼意思，42 堂還不夠，要加課。

【課本補充】這類硬體指令多半屬於 **read-modify-write（RMW）**：讀一個位址、可能檢查條件、再修改它，整個看起來一次完成。

### 2.3 為什麼 TTAS 不快

[11:00] 所有 thread 都在繞圈問「是 0 了嗎？」。一看到 0，大家**同時**衝去寫。

[11:32] 在多核心 CPU 上，**寫入很貴**：cache line（快取裡的一小塊資料）要在 CPU 之間彈來彈去。所以 TTAS 效率不好。Linux 用的是它的改良版，更有效率的 lock 之後再講。

### 2.4 Unlock 與 fence

[12:02] Unlock 很簡單：把 lock 位址設回 0。其他在等的 thread 看到 0，再去搶。

[12:32] 但其實還要加一個 **memory barrier（記憶體屏障）**。教授說這留到下一講講記憶體模型時再說。

【課本補充】§5.1.2「The Fence Instruction」：

- CPU 和編譯器為了效能會**重排指令**。
- 所以 critical section 裡的寫入，可能在 unlock 之後才被別的 CPU 看到；或 critical section 裡的指令，可能在別人看到「lock 已被拿」之前就生效。
- 解法：在 lock 和 unlock 兩端都放 **fence（又叫 memory barrier）**。fence 保證它之前的寫入，在 fence 完成時所有 thread 都看得到。
- **Point 5.1.1**：fence 很貴，要盡量少用，但為了正確性不能省。所以多數 atomic 指令本身就兼當 fence，或由函式庫在 lock/unlock 裡補上。

### 2.5 【原始碼】x86 的 `lock` 前綴與 memory barrier

x86 上，「atomic」靠的是在指令前面加 `lock` 前綴。kernel 把它包成 `LOCK_PREFIX`：

`arch/x86/include/asm/alternative.h:37-50`

```c
#ifdef CONFIG_SMP
#define LOCK_PREFIX_HERE \
		".pushsection .smp_locks,\"a\"\n"	\
		".balign 4\n"				\
		".long 671f - .\n" /* offset */		\
		".popsection\n"				\
		"671:"

#define LOCK_PREFIX LOCK_PREFIX_HERE "\n\tlock; "

#else /* ! CONFIG_SMP */
#define LOCK_PREFIX_HERE ""
#define LOCK_PREFIX ""
#endif
```

| 行 | 說明 |
|---|---|
| 37 | `CONFIG_SMP`：kernel 支援多 CPU 時才需要 |
| 38-43 | 把這個 `lock` 前綴的位置記到 `.smp_locks` 段，留個紀錄 |
| 45 | 真正的效果：在指令前加 `lock;` |
| 47-49 | 單 CPU 的 kernel：前綴是空的。只有一顆 CPU，沒人搶，不用鎖匯流排 |

`.smp_locks` 的用途：如果只有一顆 CPU 在線上，kernel 可以在開機時把這些 `lock` 前綴「修補」掉，省下成本。（這是 kernel 的 alternatives 機制，細節本講沒講。）

Memory barrier 的寫法也很有趣：

`arch/x86/include/asm/barrier.h:57`

```c
#define __smp_mb()	asm volatile("lock; addl $0,-4(%%" _ASM_SP ")" ::: "memory", "cc")
```

| 部分 | 說明 |
|---|---|
| `addl $0, -4(%rsp)` | 在 stack 下方的位址「加 0」，等於什麼都沒改 |
| `lock;` | 但加了 `lock` 前綴，x86 的 locked 指令本身就是一道完整的 fence |
| `"memory"` | 告訴編譯器：不要把記憶體存取搬過這一行 |

這正好對應課本 Point 5.1.1：**x86 的 atomic 指令本身就兼當 fence**。kernel 甚至直接拿一道「什麼都不做的 atomic 指令」來當 barrier（比 `mfence` 便宜）。

---

## 3. Data race 的理論

### 3.1 Happens-before

[13:30] `count++` 到底錯在哪？理論上的說法是有 **data race**：一份資料，多個 thread 搶著更新。

[13:30] 如果大家只是讀，就不會有問題。**至少要有一個寫入**，叫做**衝突存取（conflicting access）**。

[14:00] 正式說法要用 **happens-before（先發生）關係**。看兩個 thread 用 lock 保護 `count++`（課本 Figure 5.5）：

```
Thread 1                     Thread 2
寫 count
   │ hb
unlock ───────── hb ──────▶ lock
                               │ hb
                             讀 count
```

- 寫 `count` → unlock：寫入必須先發生，unlock 之後全世界都要看得到。
- unlock → lock：要先 unlock，別人才能 lock。
- lock → 讀 `count`：要先拿到 lock 才能讀。

[15:31] happens-before 有**遞移性**：A 先於 B、B 先於 C，則 A 先於 C。所以從 Thread 1 的寫入到 Thread 2 的讀取，有一條全是 happens-before 的路徑，它們之間有 happens-before 關係。

【課本補充】happens-before 是針對**某一次執行**說的。不同的執行可能有不同的關係。如果程式寫成「不管怎麼執行都有這個關係」，才能說「a 永遠先於 b」。

### 3.2 正式定義

[16:00] 兩個存取構成 **data race**，條件是：

1. 在**不同的 thread**（同一個 thread 內不算）。
2. 存取**同一個位址**。
3. **衝突**：至少一個是寫入。
4. **並行（concurrent）**：兩者之間**沒有** happens-before 路徑。

[17:01] 重點：並行的定義**不看實際時間**。

【口頭】[17:30] 在分散式／並行系統的理論裡，通常不用絕對時間，只看相對的、因果的順序。例如「我做了某件事，你拿到我的回傳值，那我一定比你先」。就算兩件事實際上隔了 10 年，只要沒有 happens-before 路徑，它們還是「並行」。（教授說之後會再細談例外。）

【課本補充】**Point 5.1.2**：有人會說「搶 lock 本身就是 data race 啊」。對，但那用的是 atomic 指令、在嚴格控制下發生，而且這段程式經過大量驗證、可證明正確。一般變數的 data race 就不是這樣。

### 3.3 Properly labeled（PL）程式

[19:02] 要消除 data race，就要寫 **properly labeled（PL）程式**。

[19:31] 為什麼重要：程式今天跑在 Intel，明天跑在 Mac（ARM），以後跑在 RISC-V。不能 Intel 上沒有 data race、RISC-V 上卻有。你不會為每種硬體重寫程式。

[20:30] PL 程式的定義：**所有共享變數**都只在 critical section 裡存取。也就是：兩個 thread 存取同一個共享變數、其中一個是寫入時，一定要有 lock 保護。私有變數不用管。

[21:01] 注意：不是 PL 的程式**不一定**會出錯。執行時可能剛好一前一後，結果仍是 2。data race 的意思是「**有可能**出錯」。

[21:32] 如果程式是 PL，**有證明**保證不會有 data race；沒有 data race，又能證明更多好性質。

【課本補充】課本 Theorem 5.1.6（下一講）說：沒有 data race 的平行執行，可以用等價的循序執行來推理。

[22:00] 例子（課本 Figure 5.7）：

```
Thread 1               Thread 2
Lock(X)                Lock(Y)
存取 A, B, C           存取 D, E, C
```

[22:30] 【Q&A】教授問這個程式為什麼有 data race，課堂上有學生答對：

- 位址 **C** 兩邊都會碰。
- 但 Thread 1 用 lock X，Thread 2 用 lock Y，**不是同一把鎖**。
- 所以兩個 thread 可以同時在各自的 critical section 裡存取 C。
- 規則：保護同一個變數的鎖，兩邊要有**共同的**那一把（可以是一組鎖，但要有交集）。

[23:00] 教授修正自己的說法：這裡 data race「一定存在」（因為沒有 happens-before），但不是每個 data race 都會造成錯誤。只有看到明顯不直覺的結果時才算錯。目標是從源頭消除它們。

---

## 4. Deadlock

### 4.1 四個必要條件

[24:00] lock 解決了一些問題，但也帶來新問題。

[24:30] 經典情況（課本 Figure 5.8）：

```
Thread 1          Thread 2
Lock X            Lock Y
Lock Y  (等)      Lock X  (等)
```

Thread 1 拿著 X 等 Y，Thread 2 拿著 Y 等 X。互相等，誰都動不了。這叫 **deadlock（死結）**。

[25:00] 出現 deadlock 後，除了終止或其他強制手段，幾乎沒辦法。

[25:00]–[26:30] Deadlock 的四個條件：

| 條件 | 意思 |
|---|---|
| **Hold and wait**（持有並等待） | 拿著一把鎖，又去等另一把 |
| **No preemption**（不可搶奪） | 鎖不會被強制拿走 |
| **Mutual exclusion**（互斥） | 一把鎖同時只能一個 thread 持有。這是鎖的定義 |
| **Circular wait**（循環等待） | X 等 Y、Y 等 Z、Z 等 X，形成一個環 |

[26:30] 邏輯要搞清楚：

- 四個條件**都成立**，才**可能**發生 deadlock。
- 但四個都成立，**不保證**一定發生。
- 只要少一個，就**一定不會**發生。

【口頭】[27:00] 教授的比喻：所有烏鴉都是鳥，但不是所有鳥都是烏鴉。

### 4.2 怎麼對付每個條件

[27:31] 任何系統都要針對這四個條件做點什麼：

| 條件 | 對策 |
|---|---|
| Hold and wait | 要嘛**一次拿全部**，要嘛一個都不拿。不要拿一部分再等剩下的 |
| No preemption | 強制把資源拿走，或等太久就殺掉某個 thread。「為了系統大局，犧牲一個」 |
| Mutual exclusion | 這是鎖的本質，幾乎不可能違反 |
| Circular wait | **在設計時就讓環不可能出現**。這是之後 deadlock 迴避演算法最常用的方法 |

[29:01] 循環等待通常來自系統**太對稱**。打破對稱，循環等待通常就不會發生。

### 4.3 哲學家吃飯問題

[29:31] **Dining philosophers problem**：哲學家圍著圓桌坐，每兩人之間放一支叉子（或筷子）。要吃飯得拿**左右兩支**。

[30:01] Deadlock 很容易發生：每個人都先拿右邊（或左邊）那支，結果每個人手上一支，都在等另一支。

教授提醒：如果規定「拿不到就放下」，大家可能同時放下；再規定「都拿左邊」，大家又同時拿左邊。這裡不能依賴全域時間，是以事件和依賴關係來想。

[30:30] 四個條件逐一看：

- Hold and wait：可以「兩支一起拿或都不拿」。
- No preemption：強迫某人放下叉子。
- Mutual exclusion：叉子不能兩人同時用，違反它沒意義。
- Circular wait：**設計協定讓環不可能出現**。

【課本補充】課本說「兩支一起拿」其實很難做到，因為拿兩支叉子本身不是 atomic，還是得一支一支拿，所以最簡單的是破除循環等待。

[31:01] 解法：**打破對稱**。前 n−1 位哲學家先拿右邊，第 n 位先拿左邊。（課本是反過來寫：大家先拿左邊，第 n 位先拿右邊。概念一樣。）

[31:31] **反證法**（依課本 §5.1.4）：

1. 假設形成了循環等待。那麼 1 到 n−1 號都拿著左叉、在等右叉。
2. 看 n−1 號和 n 號之間：n−1 號在等它的右叉，代表 n 號已經拿走了。那是 n 號的**左叉**。
3. 但 n 號是先拿右叉、再拿左叉的。他已經拿到左叉，表示**兩支都有了**，正在吃飯，沒有在等。
4. 矛盾。所以不會 deadlock。

### 4.4 【實測】對稱 vs 打破對稱

5 位哲學家、5 把 mutex，每人吃 20 萬次（`dine.c`）。關鍵只有一行：

```c
int first = left, second = right;
if (asym && i == P - 1) { first = right; second = left; }  /* 打破對稱 */
pthread_mutex_lock(&fork_[first]);
pthread_mutex_lock(&fork_[second]);
meals[i]++;
```

各跑 3 次，用 `timeout 5` 限時：

```
symmetric exit=124      ← 124 = 5 秒內沒跑完，被 timeout 殺掉（卡死了）
symmetric exit=124
symmetric exit=124
200000 200000 200000 200000 200000
asymmetric exit=0
200000 200000 200000 200000 200000
asymmetric exit=0
200000 200000 200000 200000 200000
asymmetric exit=0
```

全部對稱：3 次都卡死。只改第 5 位的拿取順序：3 次都在一瞬間跑完。

### 4.5 【課本補充】預防、避免、復原

課本把處理 deadlock 分三種，本講沒細講：

| 方法 | 比喻 | 做法 |
|---|---|---|
| **Prevention（預防）** | 打疫苗 | 讓四個條件之一不可能成立。例：**two-phase locking**，事先知道要哪些鎖，第一階段**照位址由小到大**全部拿，第二階段全部放。可證明不會有環 |
| **Avoidance（避免）** | 吃藥 | 拿鎖前先檢查會不會 deadlock，會就不拿，丟例外給呼叫者處理 |
| **Recovery（復原）** | — | 樂觀地跑。另一個 thread 當 deadlock 偵測器，系統長時間沒動靜時找環，找到就殺 process 或強制收回鎖 |

Two-phase locking 的證明（課本）：環上每條邊「Pu 等 Pv 手上的鎖 A」，因為每個 process 只會等比它手上**所有鎖位址都大**的鎖，沿著環走位址會一直變大。繞回起點時得到 A′ > A 又 A > A′，矛盾。

Linux kernel 的 lockdep 就是在檢查「鎖的順序」，之後會講。

---

## 5. Starvation 與 livelock

[33:31] **Starvation（飢餓）**：有了 deadlock 迴避演算法，系統整體有在動，但**某位哲學家一直拿不到兩支叉子**，無限期等下去。也就是不公平。

[34:00] 兩者的關係：

```
沒有 starvation  ⟹  沒有 deadlock
沒有 deadlock    ⟹/  沒有 starvation
```

[34:30] 為什麼：沒有 starvation 表示每個人都在有限時間內拿到資源；deadlock 是無限期拿不到，所以不可能有 deadlock。反過來，沒有 deadlock 不代表公平。（又是烏鴉和鳥。）

[35:01] **Livelock（活結）**：thread 一直在執行指令，但沒做成任何有用的事。

[35:30] 教授的例子：deadlock 的情況下，如果把「執行指令」當成一步，thread 其實一直在檢查 lock 變數，一直在動。這樣看就是 livelock：「一直在做事，卻什麼都沒做成」。教授自己也說這例子有點硬湊，之後會有更多例子。

【課本補充】課本的 livelock 例子：

- 早期乙太網路：兩台機器同時送就撞，撞了各自隨機退讓再送，可能一直撞。狀態一直在變，卻一直送不出去。
- 窄走廊：兩人面對面，同時往右讓、又同時往左讓，一直卡住。

> **課本 Point 5.1.3**：沒有 starvation，就沒有 deadlock 也沒有 livelock。反過來不成立。

---

## 6. Pthreads

### 6.1 `pthread_create`

[36:32] **pthreads（POSIX threads）**是 Linux 上最常見的 thread 函式庫，kernel 有原生支援：glibc 會用 kernel 專門為 thread 準備的系統呼叫（見 §6.4）。只要 `#include <pthread.h>`。

【課本補充】Trivia 5.1.2：編譯要加 `-lpthread`（現在的 gcc 用 `-pthread` 也可以）。

[37:31]–[40:30] 建立 thread：

```c
pthread_t tid[2];                       /* global 陣列，放兩個 thread 的 ID */
...
for (i = 0; i < 2; i++) {
    ptr = (int *) malloc(sizeof(int));
    *ptr = i;                           /* 參數：迴圈編號 0 或 1 */
    errcode = pthread_create(&(tid[i]), NULL, &foo, ptr);
    if (errcode)
        printf("Error in creating pthreads\n");
}
```

`pthread_create` 的四個參數：

| 參數 | 意思 |
|---|---|
| `&tid[i]` | 指向 `pthread_t`，函式會把建好的 thread 填進去 |
| `NULL` | thread 屬性。NULL = 預設 |
| `&foo` | 函式指標：thread 要跑的函式 |
| `ptr` | 傳給函式的唯一參數 |

[38:31] thread 要跑的函式有固定的樣子：

```c
void *foo(void *arg);
```

- 參數是 `void *`：可以指向任何東西。要傳很多參數，就放進一個 struct，傳它的指標。
- 回傳也是 `void *`：要回傳很多東西，也放進 struct 回傳指標。
- 教授說：「這已經是最有彈性的寫法了。」

[40:01] 回傳的 error code 應該是 0，不是 0 就代表建立失敗。

[41:30] 主 thread 用 **`pthread_join`** 等子 thread 結束，類似 process 的 `wait`。

[42:00] 教授提到：`foo` 本身是普通函式，`main` 也可以直接呼叫它。在 thread 裡跑時，用的是跟 signal handler 一樣的技巧：**改它的返回位址**，讓它返回到 pthread 函式庫裡的一段特別程式碼，由那段程式把 thread 收掉。

### 6.2 回傳值：為什麼是 `void **`

[44:02] 教授強調：**絕對不要回傳指向區域變數的指標**。區域變數在 stack 上，函式一返回，stack 就收掉了，指標指向垃圾。要回傳指標，就用 `malloc` 在 heap 上配置。

```c
void *foo(void *arg) {
    int *ptr = (int *) arg;
    printf("Thread %d\n", *ptr);
    int *retval = (int *) malloc(sizeof(int));   /* heap，不是 stack */
    *retval = (*ptr) * 2;                          /* 回傳 2 × thread 編號 */
    return retval;
}
```

【口頭】[44:32] 唯一勉強可以的例外：把區域變數的指標傳給你**呼叫的**函式用（它在你返回前就結束了）。這正是 `main` 把 `&result` 傳給 `pthread_join` 的情況。

[45:32] 主 thread 收回傳值：

```c
int *result;
pthread_join(tid[0], (void **) &result);
printf("For thread 0, %d was returned\n", *result);
```

[48:31]–[53:01] 教授畫圖說明（位址是他舉例的數字）：

```
foo 裡：retval = 100           記憶體位址 100：存著 4
        foo 回傳的是「100」，不是「4」

main 裡：result 這個變數本身放在位址 20（在 stack 上）
        &result = 20

pthread_join(tid, &result)
  → 傳進去的是 20（result 在哪裡），不是 result 的內容
  → pthread_join 手上有 foo 回傳的 100
  → 它把 100 寫進位址 20
  → 現在 result = 100

printf(*result)
  → 去位址 100 拿 → 4
```

[55:30]–[58:31] 為什麼 API 要這樣設計？教授把它當成 API 設計題：

- **選項 1**：`pthread_join` 直接回傳 `void *` 結果。不好：違反慣例。所有系統呼叫和函式庫呼叫的回傳值都是 **error code**（0 = 成功，非 0 = 有問題）。
- 回傳值已經拿來放 error code，沒辦法同時回傳兩樣東西。
- **唯一選擇**：呼叫者給一個指標，函式把結果填進去。結果的型別是 `void *`，所以參數要是「指向 `void *` 的指標」，也就是 `void **`。

[58:31] 教授說：系統程式裡到處都是這種「指標、指向指標的指標」的寫法。Python、JavaScript 底下最後也是用這些。

### 6.3 【實測】跑課本 Listing 5.2

照課本寫 `join.c`，多印出 `result` 的位址和內容：

```
Thread 1
Thread 0
For thread 0, 0 was returned (result @ 0x7ffe02e73c28 -> 0x7f5b5c001ba0)
For thread 1, 2 was returned (result @ 0x7ffe02e73c28 -> 0x7f5b5c001b80)
```

- 「Thread 1」比「Thread 0」先印：誰先跑由 scheduler 決定。
- `result` 本身的位址兩次都一樣（`0x7ffe...`，在 stack 上），就是教授圖裡的「20」。
- `result` 的內容兩次不同（`0x7f5b...`，heap 上 `malloc` 出來的），就是圖裡的「100」。
- 回傳值 0 和 2，正好是 2 × thread 編號。

### 6.4 【實測＋原始碼】pthread 底下是 `clone3` 和 futex

教授說 pthreads 有 kernel 的原生支援。用 `strace -f` 看 `join.c`：

```
clone3({flags=CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_THREAD|CLONE_SYSVSEM|
        CLONE_SETTLS|CLONE_PARENT_SETTID|CLONE_CHILD_CLEARTID,
        child_tid=0x7f66035ff910, ..., stack_size=0x7fff00, ...}) = 213453
...
[pid 213452] futex(0x7f66035ff910, FUTEX_WAIT_BITSET|FUTEX_CLOCK_REALTIME, 213453, NULL, ...
...
[pid 213453] exit(0)                    = ?
[pid 213452] <... futex resumed>)       = 0
```

- `pthread_create` = `clone3`，flags 就是第 12 講講過的「共享一切」：`CLONE_VM`（同一個位址空間）、`CLONE_SIGHAND`（共享 signal handler）、`CLONE_THREAD`（同一個 thread group）等。
- 每個 thread 有自己約 8 MB 的 stack（`stack_size=0x7fff00`）。
- `pthread_join` = 主 thread（213452）在位址 `child_tid` 上做 **futex wait**，等那裡的值從 213453（子 thread 的 TID）變掉。
- 子 thread `exit` 後，主 thread 的 futex 馬上返回。

是誰把它叫醒的？是 **kernel**。`CLONE_CHILD_CLEARTID` 讓 kernel 記住 `child_tid` 這個位址：

`kernel/fork.c:2119`（`copy_process` 裡）

```c
	p->clear_child_tid = (clone_flags & CLONE_CHILD_CLEARTID) ? args->child_tid : NULL;
```

thread 結束、釋放 `mm` 時：

`kernel/fork.c:1493-1504`（`mm_release`，定義在 1481 行）

```c
	if (tsk->clear_child_tid) {
		if (atomic_read(&mm->mm_users) > 1) {
			/*
			 * We don't check the error code - if userspace has
			 * not set up a proper pointer then tough luck.
			 */
			put_user(0, tsk->clear_child_tid);
			do_futex(tsk->clear_child_tid, FUTEX_WAKE,
					1, NULL, NULL, 0, 0);
		}
		tsk->clear_child_tid = NULL;
	}
```

| 行 | 說明 |
|---|---|
| 1493 | 這個 thread 建立時有 `CLONE_CHILD_CLEARTID` |
| 1494 | 還有別人在用這個位址空間（也就是還有其他 thread）才需要通知 |
| 1499 | 把使用者空間的 `child_tid` 寫成 0（原本是 TID） |
| 1500-1501 | 對這個位址做 `FUTEX_WAKE`，叫醒 1 個等待者，也就是卡在 `pthread_join` 的主 thread |
| 1503 | 清掉，避免重複通知 |

所以 `pthread_join` 不是在 busy wait，而是睡在 futex 上，由 kernel 在 thread 結束時叫醒。futex 是 pthread mutex 的基礎，之後講到 mutex 會再看。

---

## 7. 用三種方法加 `count`

### 7.1 Mutex

[59:02] lock 在 pthreads 裡叫 **mutex**（mutual exclusion，互斥）。

```c
int count = 0;
pthread_mutex_t cntlock;                 /* lock 變數 */

void *func(void *arg) {
    pthread_mutex_lock(&cntlock);        /* lock */
    count++;
    pthread_mutex_unlock(&cntlock);      /* unlock */
}

int main() {
    retval = pthread_mutex_init(&cntlock, NULL);   /* 先初始化 */
    ...
}
```

- [59:31] `pthread_mutex_init`：傳 lock 的指標去初始化，第二個參數是屬性（NULL = 預設）。
- `pthread_mutex_lock` 返回時，就表示已經拿到 lock。
- [60:02] 每把 lock 都要先初始化，設好初始值，之後就可以重複 lock/unlock 幾百次。

⚠️ 教授在 [59:31] 說 mutex lock 會「busy loop 等」。實際上 glibc 的 mutex 拿不到時會用 futex 讓 thread 睡著（§6.4 的 strace 就看得到 `FUTEX_WAIT_PRIVATE` / `FUTEX_WAKE_PRIVATE`），不是一直空轉。細節待後面 mutex 那講。

### 7.2 Atomic fetch-and-add

[60:30] 也可以**完全不用 lock**，改用硬體的 atomic 更新：

```c
#include <stdatomic.h>
atomic_int count = 0;

void *fetch_and_increment(void *arg) {
    atomic_fetch_add(&count, 1);
}
```

- [61:02] `count` 宣告成 atomic 整數。
- [61:32] `atomic_fetch_add` 包了一道機器指令：**讀出目前的值、加 1、寫回**，三步一次做完。Intel 有這種指令。
- [62:01] 沒有 lock、沒有等待，叫 **non-blocking（非阻塞）**演算法，也叫 **lock-free** 程式。

【課本補充】atomics 現在的形式是 C++11 定下的。x86 只要在 `add` 前面加 `lock` 前綴就變成 atomic add，保證在有限時間內完成。

[62:30] 但教授說 `count = count + 1` 這種操作是「**弱**」的：拿來做計數器可以，但沒辦法用它實作例如三個 thread 同時用的 stack。「其實是不可能，而且有證明」，但不展開。

### 7.3 Compare-and-swap（CAS）

[63:31] 所以所有硬體都得支援一個「**強**」的操作：**compare-and-exchange**，又叫 **compare-and-swap（CAS）**。

C 的寫法是 `atomic_compare_exchange_strong`（也有 `weak` 版，教授說不展開）。邏輯（課本 Listing 5.5）：

```c
bool CAS(int *valptr, int *oldptr, int new) {
    if (*valptr == *oldptr) {   /* 值還是我讀到的那個 */
        *valptr = new;          /* 換成新值 */
        return true;
    } else {                    /* 中間被別人改過了 */
        *oldptr = *valptr;      /* 把最新的值告訴我 */
        return false;
    }
}
```

整個函式是 atomic 的（x86 用 `cmpxchg` 指令）。

用 CAS 加 `count`（課本 Listing 5.6）：

```c
atomic_int count = 0;
#define CAS atomic_compare_exchange_strong

void *fetch_and_increment(void *arg) {
    int oldval, newval;
    do {
        oldval = atomic_load(&count);
        newval = oldval + 1;
        printf("old = %d, new = %d\n", oldval, newval);
    } while (!CAS(&count, &oldval, newval));
}
```

[64:31]–[66:30] 流程：

1. 用 `atomic_load` 讀目前的值到 `oldval`（為什麼要用 `atomic_load`，下一講談非阻塞演算法時說）。
2. 算出 `newval = oldval + 1`。
3. CAS：`count` 還等於 `oldval` 嗎？
   - 是 → 換成 `newval`，成功離開。
   - 否 → 中間有人改過（指令之間的延遲可以任意長），回傳 false，再試一次。

⚠️ [65:00] 教授在投影片上發現一處打錯（「this should be count」），CAS 的第一個參數應該是 `&count`。

[65:31] 為什麼 `&count`、`&oldval` 傳位址，`newval` 傳值？教授說之後再談。（從 Listing 5.5 可以看出：`count` 要被改，`oldval` 失敗時要被填入最新值，所以都要傳指標；`newval` 只讀不改，傳值就好。）

[66:30] 教授的主張：CAS 是**強**操作，任何硬體都**必須**實作它，沒有別的路。有了 CAS，**任何**並行演算法都能做，不管多複雜：queue、stack、linked list 都可以。

### 7.4 【原始碼】kernel 的 `atomic_inc`、`xadd`、`cmpxchg`

kernel 也有自己的 atomic 型別 `atomic_t`。在 x86 上就是加了 `LOCK_PREFIX` 的指令：

`arch/x86/include/asm/atomic.h:93-97`

```c
static __always_inline void arch_atomic_inc(atomic_t *v)
{
	asm volatile(LOCK_PREFIX "incl %0"
		     : "+m" (v->counter) :: "memory");
}
```

| 部分 | 說明 |
|---|---|
| `LOCK_PREFIX "incl %0"` | `lock incl`：atomic 地把記憶體裡的值加 1 |
| `"+m" (v->counter)` | 運算元是記憶體裡的 `counter`，可讀可寫 |
| `"memory"` | 告訴編譯器記憶體被改了，不要亂搬存取 |

Fetch-and-add 和 CAS：

`arch/x86/include/asm/atomic.h:182-204`

```c
static __always_inline int arch_atomic_fetch_add(int i, atomic_t *v)
{
	return xadd(&v->counter, i);
}
...
static __always_inline int arch_atomic_cmpxchg(atomic_t *v, int old, int new)
{
	return arch_cmpxchg(&v->counter, old, new);
}
...
static __always_inline bool arch_atomic_try_cmpxchg(atomic_t *v, int *old, int new)
{
	return arch_try_cmpxchg(&v->counter, old, new);
}
```

- `fetch_add` → `xadd`（exchange and add）：加上去，並回傳**加之前**的值。
- `cmpxchg` → 回傳舊值，呼叫者自己比對。
- `try_cmpxchg` → 回傳 `bool`，失敗時把最新值寫回 `*old`。**這和 C 的 `atomic_compare_exchange_strong`、課本 Listing 5.5 的介面一模一樣。**

`xadd` 的定義：

`arch/x86/include/asm/cmpxchg.h:227-234`

```c
/*
 * xadd() adds "inc" to "*ptr" and atomically returns the previous
 * value of "*ptr".
 *
 * xadd() is locked when multiple CPUs are online
 */
#define __xadd(ptr, inc, lock)	__xchg_op((ptr), (inc), xadd, lock)
#define xadd(ptr, inc)		__xadd((ptr), (inc), LOCK_PREFIX)
```

`cmpxchg` 的核心（32 位元的情況）：

`arch/x86/include/asm/cmpxchg.h:109-117`

```c
	case __X86_CASE_L:						\
	{								\
		volatile u32 *__ptr = (volatile u32 *)(ptr);		\
		asm volatile(lock "cmpxchgl %2,%1"			\
			     : "=a" (__ret), "+m" (*__ptr)		\
			     : "r" (__new), "0" (__old)			\
			     : "memory");				\
		break;							\
	}								\
```

| 部分 | 說明 |
|---|---|
| `"0" (__old)` | 舊值放進 `eax`（和輸出 0 號同一個暫存器） |
| `cmpxchgl %2,%1` | 比較 `eax` 和記憶體 `*__ptr`：相等就把 `__new` 寫進記憶體；不等就把記憶體的值載入 `eax` |
| `"=a" (__ret)` | 指令結束後 `eax` 的值就是回傳值：記憶體原本的值 |
| `lock` | 加了 `lock` 前綴才是 atomic |

所以呼叫者只要看「回傳值 == 我給的舊值？」就知道成功沒。這就是課本 CAS 偽碼的硬體版。

### 7.5 【實測】編譯器產生的指令

`objdump -d race | grep lock` 看 §1.4 的程式：

```
1256:	f0 48 83 05 f1 2d 00 	lock addq $0x1,0x2df1(%rip)        # 4050 <fa>
12ae:	f0 48 0f b1 0e       	lock cmpxchg %rcx,(%rsi)
```

- `atomic_fetch_add(&fa, 1)` → `lock addq`。注意不是 `xadd`：因為程式沒用到回傳值，編譯器就用比較簡單的 `lock add`。（課本也說「在 `add` 前加 `lock`」。）
- CAS → `lock cmpxchg`。
- `f0` 這個位元組就是 `lock` 前綴。
- mutex 那行沒有 `lock` 指令：它是呼叫 `pthread_mutex_lock@plt`，`lock` 指令在 glibc 裡面。

### 7.6 Lock-free 與 wait-free

[67:31] CAS 版本可能失敗很多次（§1.4 實測失敗了 22 萬次）。失敗是因為**別人成功了**，所以系統整體一直在前進。

但單一 thread **可能一直輸**，也就是會 **starvation**。課本也說：理論上一個 thread 可能永遠拿到 false。

[67:31] fetch-and-add 那種做法，硬體保證在有限時間內完成，所以不會 starvation。

[68:01] 定義：

| 名詞 | 意思 | 例子 |
|---|---|---|
| **Lock-free** | 不用 lock；系統整體一定有人在前進 | CAS 迴圈 |
| **Wait-free** | 不用 lock，**而且**每個 thread 都在有限步內完成（沒有 starvation） | 硬體 fetch-and-add |

```
wait-free  ⟹  lock-free
lock-free  ⟹/ wait-free
```

[68:01] 下一講：非阻塞演算法和並行的整體理論。教授說這是在為之後的**並行資料結構**打底，最後會用在排程 task 和排程資源上。

---

## 8. 一頁速查表

### 8.1 Data race

| 名詞 | 意思 |
|---|---|
| Critical section | 存取共享變數的那段程式，同時只能一個 thread 在裡面 |
| Lock | 進 critical section 的許可證，就是一個位址：0 = 空、1 = 占用 |
| Data race | 不同 thread、同一位址、至少一個寫、沒有 happens-before |
| Happens-before | 因果順序，有遞移性。不看絕對時間 |
| Properly labeled | 所有共享變數都在**同一把（組）鎖**保護下存取 → 沒有 data race |

### 8.2 Lock 實作

| 項目 | 重點 |
|---|---|
| TTAS | 一直讀看是不是 0 → 是 0 才用 atomic test-and-set 去搶 |
| Spin lock | busy waiting，耗 CPU 但沒做事 |
| 為什麼慢 | 大家同時寫，cache line 在 CPU 間彈來彈去 |
| Fence | lock、unlock 兩端都要；x86 的 locked 指令本身就是 fence |

### 8.3 Deadlock

| 條件 | 對策 |
|---|---|
| Hold and wait | 一次全拿或全不拿 |
| No preemption | 強制收回、殺掉 |
| Mutual exclusion | 不能動 |
| Circular wait | 打破對稱、照位址順序拿鎖 |

- 四個都要成立才可能 deadlock；成立了也不一定發生。
- 沒有 starvation ⟹ 沒有 deadlock、沒有 livelock。反之不成立。

### 8.4 Pthreads

| API | 用途 |
|---|---|
| `pthread_create(&tid, attr, func, arg)` | 建 thread；`func` 是 `void *(*)(void *)` |
| `pthread_join(tid, (void **)&result)` | 等 thread 結束、拿回傳值；回傳值本身是 error code |
| `pthread_mutex_init / lock / unlock` | mutex |
| `atomic_fetch_add` | atomic 加法（x86：`lock add` / `lock xadd`） |
| `atomic_compare_exchange_strong` | CAS（x86：`lock cmpxchg`） |

### 8.5 Non-blocking

| 名詞 | 保證 |
|---|---|
| Lock-free | 整體有人前進，個別可能 starve |
| Wait-free | 每個 thread 都在有限步內完成 |

### 8.6 本講看過的原始碼位置

| 位置 | 內容 |
|---|---|
| `arch/x86/include/asm/alternative.h:37-50` | `LOCK_PREFIX` |
| `arch/x86/include/asm/barrier.h:57` | `__smp_mb()`：用 `lock; addl $0` 當 fence |
| `arch/x86/include/asm/atomic.h:93-97` | `arch_atomic_inc`：`lock incl` |
| `arch/x86/include/asm/atomic.h:182-204` | `fetch_add`、`cmpxchg`、`try_cmpxchg` |
| `arch/x86/include/asm/cmpxchg.h:109-117` | `lock cmpxchgl` |
| `arch/x86/include/asm/cmpxchg.h:227-234` | `xadd` |
| `kernel/fork.c:2119` | `clear_child_tid` 在 `copy_process` 設定 |
| `kernel/fork.c:1493-1504` | `mm_release`：thread 結束時 futex wake，`pthread_join` 靠它 |

---

## 9. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **atomic、happens-before、fence、非阻塞演算法的理論** | **第 19 講**：Memory models |
| **semaphore、condition variable、reader-writer lock、queue** | **第 20 講**：RW locks and queues |
| **CAS 可以做任何並行資料結構** | **第 21 講**：Concurrent queues |
| **spin lock、mutex（底下的 futex）、deadlock 與鎖的順序（lockdep）** | **第 22 講**：Concurrency in the kernel |
| **lock-free 讀取** | **第 23 講**：RCU |
| **deadlock 避免（Banker's algorithm）、starvation** | **第 24 講**：Scheduling |
| **多 CPU 上的同步成本** | **第 25 講**：Multiprocessor scheduling |
| **公平性** | **第 26 講**：Linux scheduling（CFS） |
| **priority inversion（低優先權拿著高優先權要的鎖）** | **第 28 講**：Real-time resource allocation；另一門課 Real Time Systems 也會講 |

（講次對應依播放清單標題與課本章節推估。）

---

## 附：本講一句話總結

> **`count++` 是三道指令，兩個 thread 交錯執行就會少加（實測 400 萬次少了 6 萬），這叫 data race：不同 thread、同一位址、至少一個寫、彼此之間沒有 happens-before。解法是把它包進 critical section，用 lock 保護；最簡單的 lock 是 TTAS spin lock，靠硬體 atomic 指令（x86 的 `lock` 前綴，它同時也是 fence）。只要所有共享變數都由同一把鎖保護（properly labeled），就沒有 data race。但 lock 會帶來 deadlock，要四個條件同時成立，最實用的對策是打破對稱來消除循環等待（實測哲學家問題：對稱 3 次全卡死，改一個人的順序就全跑完）。Pthreads 用 `void *` 讓 thread 能跑任何函式，`pthread_join` 用 `void **` 拿回傳值；底下是 `clone3` 加 futex，由 kernel 在 thread 結束時叫醒 join 的人。不用 lock 也能加：fetch-and-add 是 wait-free，CAS 迴圈是 lock-free（可能 starve），但 CAS 是能實作任何並行資料結構的「強」操作。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 18（Chapter 5, Part I）錄影逐字稿（YouTube 自動字幕，1:08:58）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）第 5 章開頭與 §5.1.1–5.1.5，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0、20 CPU）結果。*
