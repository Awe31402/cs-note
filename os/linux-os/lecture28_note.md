# Operating Systems: A Linux Kernel-Oriented Approach 第二十八講完整筆記：即時系統的資源分配（Chapter 5, Part XI）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 28（Chapter 5, Part XI：Real-time Resource Allocation）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（53:16，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§5.5.5–5.6（p.265–275）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
> **標記說明**：
> - `【口頭】`：教授在課堂上口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與教授回答
> - `【課本補充】`：課本有、但這堂課沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（`檔名:行號`），用來對照觀念
> - `【實測】`：在本機（Ubuntu、kernel 6.5.0、x86-64）實際執行的結果
> - `⚠️ 逐字稿缺字`：自動字幕在這裡掉字或聽不清，內容依上下文或課本補上；也用來標示口誤或需要修正之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 這堂課要做什麼](#0-這堂課要做什麼)
- [1. 優先權反轉](#1-優先權反轉)
  - [1.1 有界的反轉](#11-有界的反轉)
  - [1.2 無界的反轉](#12-無界的反轉)
- [2. PIP：優先權繼承協定](#2-pip優先權繼承協定)
  - [2.1 做法](#21-做法)
  - [2.2 【課本補充】PIP 的問題](#22-課本補充pip-的問題)
  - [2.3 【原始碼】Linux 的 rt_mutex 就是 PIP](#23-原始碼linux-的-rt_mutex-就是-pip)
- [3. 三種阻塞與它們的關係](#3-三種阻塞與它們的關係)
- [4. HLP：最高鎖定者協定](#4-hlp最高鎖定者協定)
  - [4.1 資源的天花板](#41-資源的天花板)
  - [4.2 做法](#42-做法)
  - [4.3 定理：HLP 沒有資源阻塞](#43-定理hlp-沒有資源阻塞)
  - [4.4 代價：繼承阻塞](#44-代價繼承阻塞)
- [5. PCP：優先權天花板協定](#5-pcp優先權天花板協定)
  - [5.1 系統天花板 CSC](#51-系統天花板-csc)
  - [5.2 兩個條款](#52-兩個條款)
  - [5.3 引理：之後的 task 不會覬覦之前被拿走的資源](#53-引理之後的-task-不會覬覦之前被拿走的資源)
  - [5.4 定理：PCP 沒有二次阻塞](#54-定理pcp-沒有二次阻塞)
- [6. 【實測】模擬三種協定](#6-實測模擬三種協定)
- [7. 【實測】pthread 的 PRIO_INHERIT 與 PRIO_PROTECT](#7-實測pthread-的-prio_inherit-與-prio_protect)
- [8. 第 5 章總結](#8-第-5-章總結)
- [9. 一頁速查表](#9-一頁速查表)
- [10. 與後續講次的連結](#10-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

[00:33] 上一講談可排程性，但假設 task 之間**不搶資源**。這一講看 task 要**拿鎖、拿資源**時會發生什麼事。

三個協定：

| 協定 | 一句話 |
|---|---|
| **PIP** | 拿著資源的低優先權 task，暫時**繼承**等它的高優先權 task 的優先權 |
| **HLP** | 一拿到資源，就把優先權拉到「可能用這個資源的最高優先權 + 1」 |
| **PCP** | 系統天花板：就算資源空著，優先權不夠高也**不給** |

課本 Point 5.5.2 的共同假設：**單處理器**；每個 task 的原始優先權都不一樣（同分用 task 編號打破）；數字越大優先權越高。

---

## 1. 優先權反轉

### 1.1 有界的反轉

[00:33] 低優先權 task T_low（優先權 5）先拿到資源 R。

[01:00] 高優先權 task T_high（優先權 20）來了，搶占 T_low，一直跑到要拿 R 的地方，**卡住**，因為 R 在 T_low 手上。

[01:32] T_high 優先權雖高也沒辦法，只能讓 T_low 跑。這看起來不公平，但鎖不能被搶走，只能等。

【課本補充】這叫 **bounded priority inversion（有界的優先權反轉）**：等待時間可預測，就是 T_low 用完 R 的時間。

> **課本 Definition 5.5.2**：**優先權反轉**：低優先權 task 拿著高優先權 task 想要的資源，因而擋住高優先權 task。

### 1.2 無界的反轉

[01:32] 更糟的情況：T_low 被一個**中優先權** task T_medium（優先權 10）搶占了。

```
優先權 20  T_high  ██░░░░░░░░░░░░░░░░░░░░██   ← 卡在 R 上
優先權 12  T_med′            ██████
優先權 10  T_med     ██████          ██████  ...
優先權  5  T_low  ██                        ██  ← 拿著 R，一直被搶占
                  ───────────────────────────────▶ 時間
```

[02:01]–[02:31]

- T_high 在等 T_low：這部分還可以接受。
- 但 T_medium 搶占了 T_low，在跑。它又可能被優先權 12 的 T_medium′ 搶占……
- T_high 實際上被一大堆**根本沒拿資源**、優先權介於 5 和 20 之間的 task 擋住了。

[02:31] 這叫 **unbounded priority inversion（無界的優先權反轉）**：不知道會持續多久，T_medium′ 做完，可能又來另一個中優先權 task，[03:01] 沒有上限。

（補充：這正是 1997 年火星探路者號（Mars Pathfinder）一再重開機的原因，後來遠端打開 VxWorks mutex 的優先權繼承選項才修好。）

---

## 2. PIP：優先權繼承協定

### 2.1 做法

[03:01] 解法：**暫時提高 T_low 的優先權**。

[03:31]–[05:01]

```
T_low 的優先權：  5 ────┐            ┌──── 5
                       │     20     │
                       └────────────┘
                     T_high 開始等    T_low 放掉 R
```

- T_high 開始等 T_low 的那一刻，T_low 的優先權從 5 **變成 20**（繼承 T_high 的）。
- T_low 用優先權 20 跑，中優先權的 task 搶不了它。
- T_low 放掉 R，T_high 以優先權 20 開始跑；T_low 回到**拿資源之前**的優先權 5。

[05:30] 好處：那些中優先權 task（例如 12）原本會擋住 T_low，進而擋住 T_high。T_low 暫時繼承 T_high 的優先權之後，就不會了。

[06:03] **PIP 阻止了無界的優先權反轉。**

【課本補充】課本的細節：

- 拿資源的 T_hld 優先權 p_hld、要求的 T_req 優先權 p_req。p_hld < p_req 才提高；p_hld > p_req 就不用動。
- 放掉資源時恢復成**拿這個資源之前**的優先權（它本身可能已經是因為別的資源繼承來的）。
- 一個 task 拿著資源期間可以**多次**繼承：例如 T_low（5）拿著 R，T_med（10）來要 → T_low 變 10；T_high（15）也來要 → 變 15；放掉後回到 5。
- 簡單、有效，所以**很多版本的 Linux 都支援 PIP**。

### 2.2 【課本補充】PIP 的問題

[06:03] 教授說還有很多事會發生：

**Deadlock**（課本例子）：

1. T_low 拿了 R1，被 T_high 搶占。
2. T_high 拿了 R2，接著要 R1 → 卡住，T_low 繼承它的優先權，繼續跑。
3. T_low 要 R2 → 在 T_high 手上。
4. 互相等，**deadlock**。

**二次阻塞**：R1 在 T1 手上、R2 在 T2 手上。T3 依序要 R1、R2，兩次都會被擋。

> **課本 Point 5.5.3**：PIP 的主要問題是 deadlock、chain blocking、second blocking。它只解決了無界的優先權反轉。

課本也定義兩種阻塞：

| 阻塞 | 意思 |
|---|---|
| **資源阻塞（resource blocking）** | 想拿的資源被別人拿著；或因為優先權不夠高，要求被拒絕（PCP） |
| **繼承阻塞（inheritance blocking）** | 自己沒要資源，但別的 task 優先權被提高了，自己跑不了 |

### 2.3 【原始碼】Linux 的 rt_mutex 就是 PIP

Linux 的 PIP 實作叫 **rt_mutex**：

`kernel/locking/rtmutex.c:1-17`（節錄）

```c
/*
 * RT-Mutexes: simple blocking mutual exclusion locks with PI support
 *
 * started by Ingo Molnar and Thomas Gleixner.
 ...
 *  See Documentation/locking/rt-mutex-design.rst for details.
 */
```

「with PI support」：PI = priority inheritance。

每個 task 記錄誰在等它：

`include/linux/sched.h:1128-1140`

```c
	/* Protection of the PI data structures: */
	raw_spinlock_t			pi_lock;

	struct wake_q_node		wake_q;

#ifdef CONFIG_RT_MUTEXES
	/* PI waiters blocked on a rt_mutex held by this task: */
	struct rb_root_cached		pi_waiters;
	/* Updated under owner's pi_lock and rq lock */
	struct task_struct		*pi_top_task;
	/* Deadlock detection and priority inheritance handling: */
	struct rt_mutex_waiter		*pi_blocked_on;
#endif
```

| 欄位 | 說明 |
|---|---|
| `pi_waiters` | 在等**我手上的** rt_mutex 的 task，依優先權排成紅黑樹 |
| `pi_top_task` | 其中優先權最高的那個：我要繼承它的優先權 |
| `pi_blocked_on` | **我自己**在等哪一個 rt_mutex（用來沿著鏈往下走） |

調整優先權：

`kernel/locking/rtmutex.c:475-485`

```c
static __always_inline void rt_mutex_adjust_prio(struct task_struct *p)
{
	struct task_struct *pi_task = NULL;

	lockdep_assert_held(&p->pi_lock);

	if (task_has_pi_waiters(p))
		pi_task = task_top_pi_waiter(p)->task;

	rt_mutex_setprio(p, pi_task);
}
```

- 有人在等我：拿等待者裡**優先權最高**的，`rt_mutex_setprio` 把我的優先權提到它那麼高（繼承）。
- 沒人在等了：`pi_task = NULL`，恢復原本的優先權。

**鏈式繼承與 deadlock 偵測**：

`kernel/locking/rtmutex.c:550-569`（節錄）

```c
/*
 * Adjust the priority chain. Also used for deadlock detection.
 * Decreases task's usage by one - may thus free the task.
 *
 * @task:	the task owning the mutex (owner) for which a chain walk is
 *		probably needed
 ...
 * Returns 0 or -EDEADLK.
```

- T1 等 T2、T2 又等 T3（chain blocking）：提高 T2 之後，要**沿著鏈**（`pi_blocked_on`）把 T3 也提高。
- 走鏈的時候如果繞回自己，就是 deadlock，回傳 **`-EDEADLK`**。
- 這正好印證課本：PIP 本身**不能防止** deadlock 和 chain blocking，Linux 只能在發生時偵測出來。鏈的長度上限是 `max_lock_depth`（`rtmutex.c:639`）。

rt_mutex 用在：使用者空間的 PI futex（§7）、`PREEMPT_RT` kernel（把一般的 mutex 和 spinlock 換成 rt_mutex）、RCU 的優先權提升（第 23 講）。

---

## 3. 三種阻塞與它們的關係

[06:03]–[07:30] 除了無界反轉，還有：

| 現象 | 意思 |
|---|---|
| **Chain blocking（鏈式阻塞）** | [06:03] T1 等 T2 手上的資源，T2 等 T3 手上的……能鏈式阻塞，就可能 deadlock |
| **Second blocking（二次阻塞）** | [06:32] task 已經拿了一個資源 R，之後（還拿著 R）要拿**另一個**資源時被擋住。像巢狀的 acquire |

【口頭】[06:32] 教授說「二次阻塞」是**他們書裡引入的名詞**，不是標準術語，但對證明很有用。

> **課本 Definition 5.5.3**：低優先權 task 不斷被搶占，害高優先權 task 被擋無限久 → 無界的優先權反轉。拿了第一個資源後，等待後續的資源 → 二次阻塞。T1 被 T2 擋、T2 被 T3 擋 → 鏈式阻塞。

[08:01]–[10:31] 三者的關係：

```
沒有二次阻塞  ⟹  沒有鏈式阻塞  ⟹  沒有 deadlock
```

**沒有二次阻塞 ⟹ 沒有鏈式阻塞**（課本 Lemma 6）。看逆否命題「鏈式阻塞 ⟹ 二次阻塞」：

```
T1 ──等──▶ T2 ──等──▶ T3
```

[08:34] 看中間的 T2：它拿了一個 T1 想要的資源，之後又在等 T3 手上的資源。[09:00] 這就是 T2 的**二次阻塞**。

**沒有鏈式阻塞 ⟹ 沒有 deadlock**（課本 Lemma 3）。逆否命題「deadlock ⟹ 鏈式阻塞」：

[09:30] deadlock 需要循環等待 T1 → T2 → … → T1（T3 可以就是 T1），這本身就是鏈式阻塞的樣子。

[10:00] 重點：只要能**防止二次阻塞**，也就是「一個 task 拿到第一個資源之後，就有絕對的權利拿到它要的任何其他資源，不會被擋」，就沒有鏈式阻塞，也就沒有 deadlock。[10:31] 這對我們非常重要。

---

## 4. HLP：最高鎖定者協定

### 4.1 資源的天花板

[11:01] 一個比較聰明的協定：**HLP（Highest Locker Protocol）**。

[11:01] 要一個學生常常覺得難接受的假設：對資源 R，**事先知道**所有**可能**要它的 task（現在或將來，不一定真的會要）。怎麼知道的不重要。

[11:33] 設這個集合是 S；S 以外的 task 永遠不會要 R。

[12:03] **資源的天花板（ceiling）**：

```
ceil(R) = max_{T ∈ S} prio_o(T)
```

S 裡所有 task **原始**優先權（不是繼承來的）的最大值。[13:01] 教授：天花板是假想的，「可能要 R 的 task 裡，最大的原始優先權」。

### 4.2 做法

[13:01] **一拿到資源 R，就把優先權提到 ceil(R) + 1**（如果這比目前的優先權高；否則不動）。

[13:31]–[14:00] 例子：可能要 R 的有 T1（5）、T2（10）、T3（20），ceil(R) = 20。T1 拿到 R 後，優先權變成 **21**。

[15:00] 如果還有一個優先權 100 的 T4 也可能要 R，T1 拿到 R 後就變 **101**，幾乎能擋住所有人（不是阻塞它們，是搶占它們、不讓它們跑）。

【課本補充】課本稱之為「**打了類固醇的優先權繼承**」。跟 PIP 一樣不會有無界的優先權反轉：任何提高優先權的協定都能避免，因為拿資源的 task 優先權至少跟要資源的一樣高，中優先權的 task 搶不了它。

符號（[15:30]）：

| 符號 | 意思 |
|---|---|
| pri_c(T) | T **目前**的優先權（可能被繼承或 HLP 提高過） |
| pri_o(T) | T 的**原始**優先權 |

⚠️ [14:33] 教授先說「可以證明 ceil(R) + 1 一定大於目前的優先權」，[24:00] 自己更正：**不對**。例如某資源只有優先權 5 的 task 會用，ceil + 1 = 6；但這個 task 之前可能已經拿了一個天花板是 100 的資源，目前優先權是 101。所以要取 max。

### 4.3 定理：HLP 沒有資源阻塞

[16:31] 要證明 HLP **不會有資源阻塞**：一個 task 要拿資源時被擋住，這種事不會發生。

[17:00] 思路：task 要先在跑，才可能在拿資源時被擋。如果它**根本沒機會跑**，就不可能被擋。

> **課本 Theorem 5.5.1**：HLP 沒有資源阻塞。

**證明**（反證法）：

[17:30] T2 在 t₀ 拿了 R。T1 在 t_block 要拿 R 時被擋。設這是系統裡**第一次**資源阻塞，也就是 t_block 之前沒有任何 task 被資源阻塞過。

[18:01] 問：t₀ 時 T1 在做什麼？兩種情況。

**情況 1：T1 還沒開始**

[18:30]

```
pri_o(T1) ≤ ceil(R) < pri_c(T2)
```

- 第一個 ≤：T1 會要 R，依天花板的定義。
- 第二個 <：T2 拿了 R，優先權至少是 ceil(R) + 1。

[19:01]–[20:00] T1 開始時用的是原始優先權，比 T2 低。這時跑的要嘛是 T2，要嘛是比 T2 更高優先權的 task；它們在 t_block 之前都不會被阻塞（只會被更高的搶占）。所以 T1 **根本沒機會開始**，更不可能跑到拿 R 的地方被擋。矛盾。

**情況 2：T1 早就開始了**

[20:00]–[22:31]

- t₀ 時跑的是 T2，所以 pri_c(T2) > pri_c(T1)。
- T2 在 t_block 之前不會被阻塞，只可能被更高優先權的 T_H 搶占。
- T1 要能再跑，唯一的方法是**透過繼承**提高優先權：T_H 被 T1 手上的某個資源擋住，把優先權傳給 T1。
- 但 t_block 之前**沒有任何 task 被資源阻塞**，T_H 不可能被擋、也不可能傳優先權。
- 所以 T1 在 t_block 之前沒機會跑。矛盾。

[23:02] 結論：

```
HLP 沒有資源阻塞 ⟹ 沒有二次阻塞 ⟹ 沒有鏈式阻塞 ⟹ 沒有 deadlock
```

【課本補充】課本 Lemma 1 另一個證法：deadlock 需要 hold-and-wait；沒有資源阻塞，hold-and-wait 永遠不會成立。

### 4.4 代價：繼承阻塞

[15:30] [23:31] 但 HLP 引入了一個新問題：**繼承阻塞**。T1 拿了資源就變成「超人」，把其他人都擋住了，很不公平。

【課本補充】課本的例子：T 的優先權 5，資源天花板 25。T 拿到資源後優先權變 26。25 只是一個假想的最大值，可能很少真的發生；但優先權 6 到 25 的 task 全都被搶占了。可悲的是，**可能根本沒有別的 task 要這個資源**，還是擋住了一堆人。

> **課本 Point 5.5.4**：HLP 的主要問題是繼承阻塞。它沒有資源阻塞、鏈式阻塞、二次阻塞、deadlock、無界優先權反轉。

---

## 5. PCP：優先權天花板協定

### 5.1 系統天花板 CSC

[24:31] **PCP（Priority Ceiling Protocol）**要解決 HLP 的問題，特別是繼承阻塞。

[25:02] 關鍵：**有時資源明明空著，也不給**。這給了一點空間，讓我們不必大幅調整優先權，改用一個新的量：**系統天花板**。

[25:31] PCP 在單處理器和多處理器上都能跑，分析只看單處理器。

[26:01]–[27:01] **目前系統天花板 CSC（Current System Ceiling）**：

```
CSC(t) = max { ceil(R) : R 在時間 t 被某個 task 拿著 }
```

天花板的定義不變。資源被放掉時，CSC 重新計算。

### 5.2 兩個條款

[27:01] **資源授予條款（resource grant clause）**：task T 要拿資源 R，以下**任一**成立才給：

1. [28:00] T 的**原始**優先權 **大於** CSC。
2. [27:30] T 手上拿著**設定目前 CSC 的那個資源**（叫 **critical resource，關鍵資源**）。

[28:31]–[30:02] 教授的白話：

- task 剛開始、還沒拿任何資源時，要靠**條款 1**：優先權要夠高（大於 CSC）。例：CSC = 20、T 的優先權 25 → 可以拿 R，CSC 變成 R 的天花板，R 變成關鍵資源。
- 拿到第一個資源後，靠**條款 2** 就能繼續拿任何資源，不需要再申請許可。
- 目標就是**防止二次阻塞**：第一次拿資源可能被擋（資源被別人拿著，或優先權不大於 CSC），但**第二次不會**。

[30:30] **繼承條款（inheritance clause）**：跟 PIP 一樣。一個 task 被另一個 task 資源阻塞，拿著資源的 task 繼承被擋的 task 的優先權（如果被擋的比較高）。[31:02] 資源放掉後，優先權恢復、CSC 也更新。

【課本補充】課本 Lemma 4：原始優先權大於 CSC 的 task 一拿到資源，就會設定 CSC，那個資源就成為關鍵資源。之後不管舊的關鍵資源繼續關鍵、或新拿的資源變成關鍵，T 都一直持有關鍵資源。

### 5.3 引理：之後的 task 不會覬覦之前被拿走的資源

> **課本 Lemma 5**：task T 在時間 t 因為原始優先權大於 CSC 而拿到資源 R。設 S = 在 t 之前已經拿了資源的 task 集合。如果保證 T 不會被阻塞，那麼**之後的任何 task 都不會對 S 裡的 task 拿著的資源有興趣**。

[34:04] 教授：時間 t 像一個**分隔點**，之後跑的 task 都不會想要 t 之前被拿走的東西。

**證明**：

[34:32] T 拿到 R 後繼續跑，依假設不會被阻塞。可能被更高優先權的 T_H 搶占。

[35:01] T_H 一定是**新來的**：t 時 T_H 不在跑，否則 T 根本不會跑；也沒有事件提高它的優先權。

[35:31] T_H 會不會想要 S 裡的某個資源 R_x？

```
pri_c(T) > CSC(t⁻) ≥ ceil(R_x) ≥ pri_o(T_H)
```

- 第一個 >：T 是靠條款 1 拿到 R 的。
- 第二個 ≥：CSC 是所有已被拿走的資源的天花板最大值，R_x 在其中。
- 第三個 ≥：如果 T_H 要 R_x，依天花板的定義。

[37:02] 所以 T 的優先權**大於** T_H 的，T_H 根本搶占不了 T、跑不起來，也就不可能「之後想要 R_x」。

[38:01]–[39:32] 教授承認還有一些邊角情況（例如 T_H 拿了一個資源後被二次阻塞、T 又放掉了某個資源……），他刻意沒考慮，因為下面定理的情境會把它們排除；完整起見要加上「假設沒有二次阻塞」。

[40:00] 總結：之後的 task 不會對 t 之前被拿走的資源有興趣，因為 CSC 的定義：如果它有興趣，CSC(t⁻) 就會 ≥ 它的優先權，那 T 就不會拿到資源。

【課本補充】課本：如果 T_H 又被更高優先權的 task 搶占，同樣的論證也成立。

### 5.4 定理：PCP 沒有二次阻塞

> **課本 Theorem 5.5.2**：一個 task 拿到一個資源後，拿其他資源時不會被擋。PCP 沒有二次阻塞。

**證明**：

[40:32] 設系統跑了一陣子都沒有二次阻塞，t_block 時發生**第一次**：T1 在 t₁ 拿了第一個資源 R1，之後要拿 R2 時被擋，因為 R2 在 T2 手上。

[41:00] 看 T2 什麼時候拿到 R2。

**情況 1：T2 在 t₁ 之前拿到 R2**

[41:31]–[43:33]

```
pri_o(T1) ≤ ceil(R2) ≤ CSC(t₁)
```

- t₁ 之前 T1 還沒拿資源，目前優先權 = 原始優先權。
- T1 會要 R2 → 第一個 ≤（天花板的定義）。
- t₁ 時 R2 已被 T2 拿著 → CSC(t₁) ≥ ceil(R2)。

[42:03] t₁ 時 T1 還沒有關鍵資源，只能靠**條款 1** 拿 R1，需要 pri_o(T1) **>** CSC(t₁)。但上面說 ≤。[42:33] 所以 T1 在 t₁ 不可能拿到 R1。矛盾。

**情況 2：T2 在 t₁ 之後拿到 R2**

[44:00]–[46:01]

```
pri_o(T2) > CSC(t₁) ≥ ceil(R1) ≥ pri_o(T1)
```

- T1 在 t₁ 拿了 R1 並設定了 CSC，T2 沒有關鍵資源，只能靠條款 1 → 第一個 >。
- R1 被拿著 → CSC(t₁) ≥ ceil(R1)。
- T1 要 R1 → ceil(R1) ≥ pri_o(T1)。

[46:01] 時間軸：

```
t₁：T1 拿 R1   →   T2 拿 R2   →   t_block：T1 要 R2，失敗（第一次二次阻塞）
```

[46:31] T2 拿到 R2 後一直在跑，t_block 之前它不會被阻塞（那是第一次二次阻塞）。

[47:03] T1 要能跑，唯一辦法是**透過繼承**：某個超高優先權的 T_H 搶占 T2，然後被 T1 手上的資源（例如 R1）擋住，把優先權傳給 T1。

[47:31]–[50:02] 用 Lemma 5：T2 在 t₂ 靠條款 1 拿到 R2；S = t₂ 之前拿了資源的 task，T1 ∈ S、R1 是之前被拿走的資源之一。Lemma 5 說之後的 task（包括 T_H）**不會想要 R1**。所以 T_H 不會被 R1 擋住、不會把優先權傳給 T1。

[50:30] T1 在 t_block 之前沒機會跑，不可能去拿 R2 被擋。矛盾。

[50:30] 教授的高層總結：T1 拿了 R1，T2 拿了 R2。T2 拿到 R2 後，T1 要等 T2 放掉 R2、差不多做完才能跑；T2 不會被阻塞，T1 唯一的機會是優先權繼承，而 Lemma 5 把這條路堵死了。

[52:00] 結論：

```
PCP 沒有二次阻塞 ⟹ 沒有鏈式阻塞 ⟹ 沒有 deadlock
```

【課本補充】PCP 的優先權繼承跟 PIP 一樣，**不會**把優先權提到很高（不像 HLP）。所以繼承阻塞受到相當的控制。

> **課本 Point 5.5.5**：PCP 沒有 deadlock、二次阻塞、鏈式阻塞、無界優先權反轉。繼承阻塞的問題也大幅減輕。

【課本補充】課本總結：即使資源空著，task 也可能拿不到，這是一種「**利他**」的選擇，但對整個系統有好結果。

---

## 6. 【實測】模擬三種協定

寫一個單處理器、單位時間的模擬器（`prot.py`）。每個 task 有到達時間、原始優先權，和一串動作：`L` 拿鎖、`C` 算 n 個單位、`U` 放鎖。每個時刻跑**有效優先權**最高、且沒被擋住的 task。

**情境 1：無界的優先權反轉**

L（優先權 1）在 t = 0 拿 R、要算 3 單位；H（3）在 t = 1 到，算 1 單位後要 R；M（2）在 t = 2 到，純計算 6 單位、不碰 R。

```
Scenario 1: L(prio1) holds R; H(prio3) needs R; M(prio2) is CPU-bound (6 units)
  none timeline LHMMMMMMLLHL   H finishes at 11
  PIP  timeline LHLLHMMMMMML   H finishes at 5
  HLP  timeline LLLHHMMMMMML   H finishes at 5
  PCP  timeline LHLLHMMMMMML   H finishes at 5
```

（時間軸每個字母是那一個時間單位在跑的 task。）

- **沒有協定**：t = 2 H 被 R 擋住，M 到了，優先權比 L 高，連跑 6 單位。H 等到 t = 11 才做完。這就是無界反轉：M 跑多久，H 就多等多久。
- **PIP**：H 被擋的那一刻 L 繼承優先權 3，M 搶不了，L 把 R 用完，H 在 t = 5 做完。
- **HLP**：L 一拿到 R 就變成 ceil(R) + 1 = 4，H 到了也搶不了它（`LLL`），H 一跑就拿得到 R，**完全沒被資源阻塞**，一樣 t = 5 做完。
- **PCP**：這個情境下跟 PIP 一樣。

**情境 2：PIP 會 deadlock**

L 先拿 A、算 2、再拿 B；H 在 t = 1 到，先拿 B、算 1、再拿 A（順序相反）。

```
Scenario 2: L locks A then B; H (arrives t=1) locks B then A
  PIP  timeline LHLX           finish {}   <- DEADLOCK
  HLP  timeline LLLHH          finish {'L': 3, 'H': 5}
  PCP  timeline LLLHH          finish {'L': 3, 'H': 5}
```

- **PIP**：H 拿了 B、要 A 被擋；L 繼承優先權後要 B，被擋。誰都跑不了（`X`）。跟課本 §5.5.5 的例子一樣。
- **HLP**：L 拿 A 後優先權變 ceil(A) + 1 = 3，H 到了搶不了，L 順利拿到 B、做完，H 再跑。
- **PCP**：L 拿 A，CSC = ceil(A) = 2。H 在 t = 1 要 B，**B 明明空著**，但 H 的原始優先權 2 不大於 CSC 2 → **不給**。L 繼承 H 的優先權，靠條款 2（持有關鍵資源 A）拿到 B、做完。正是「空著也不給」換來的無 deadlock。

**情境 3：HLP 的繼承阻塞**

L（1）拿 R、算 4；M（5）在 t = 1 到、純計算 3、**完全不碰 R**；H（9）在 t = 2 到，算 1 後要 R。ceil(R) = 9。

```
Scenario 3 (inheritance blocking): ceil(R)=9; M(prio5) never touches R
  PIP  timeline LMHLLLHMM      M finishes at 9, H at 7
  HLP  timeline LLLLHHMMM      M finishes at 9, H at 6
  PCP  timeline LMHLLLHMM      M finishes at 9, H at 7
```

- **HLP**：L 一拿到 R 就變優先權 10。M 在 t = 1 到了，跟 R 毫無關係，卻要等到 t = 6 才第一次跑（`LLLL` 之後 H 還先跑）。這就是**繼承阻塞**。
- **PIP / PCP**：M 一到（t = 1）就能跑；L 只有在 H 真的被擋住後（t = 3）才被提高。
- 代價：HLP 下 H 完全不用等資源（t = 6 做完），PIP / PCP 下 H 要等 L 用完 R（t = 7）。各有取捨。

---

## 7. 【實測】pthread 的 PRIO_INHERIT 與 PRIO_PROTECT

POSIX 讓使用者空間的 mutex 選協定。`pi.c`：主 thread 拿著 mutex，另一個 thread 來搶：

```c
pthread_mutexattr_setprotocol(&at, PTHREAD_PRIO_INHERIT);   /* 或 PTHREAD_PRIO_PROTECT */
```

**`PTHREAD_PRIO_INHERIT`**（= PIP）：

```
[pid 219967] futex(0x56020c72f040, FUTEX_LOCK_PI_PRIVATE, NULL <unfinished ...>
[pid 219966] futex(0x56020c72f040, FUTEX_UNLOCK_PI_PRIVATE) = 0
...
[pid 219967] futex(0x56020c72f040, FUTEX_UNLOCK_PI_PRIVATE) = 0
PRIO_INHERIT done
```

- 拿不到時呼叫 **`FUTEX_LOCK_PI`**，不是一般的 `FUTEX_WAIT`。kernel 會為這個使用者空間的鎖建一個 **rt_mutex**，讓持有者繼承等待者的優先權（§2.3）。
- 放鎖用 `FUTEX_UNLOCK_PI`，kernel 把鎖直接交給優先權最高的等待者。

**`PTHREAD_PRIO_PROTECT`**（天花板協定）：

```
sched_getparam(219972, [0])             = 0
sched_getscheduler(219972)              = 0 (SCHED_OTHER)
sched_setscheduler(219972, SCHED_OTHER, [1]) = -1 EINVAL (Invalid argument)
...
[pid 219973] sched_setscheduler(219973, SCHED_OTHER, [0]) = 0
...
sched_setscheduler(219972, SCHED_OTHER, [0]) = 0
PRIO_PROTECT done
```

- 完全沒有 futex 的 PI 操作。glibc 在**拿鎖時**直接呼叫 `sched_setscheduler`，把自己的優先權提高到 mutex 的天花板（`prioceiling`）；放鎖時恢復。
- 這裡失敗了（`EINVAL`）是因為測試程式用的是一般的 `SCHED_OTHER` task，沒有即時優先權可調（本機 `ulimit -r` 是 0，沒有權限用 `SCHED_FIFO`）。但呼叫的**模式**看得很清楚：**一拿到鎖就把優先權拉到天花板**。
- 這正是 **HLP** 的做法（POSIX 叫 immediate priority ceiling；跟 HLP 差別只在拉到 ceil 而不是 ceil + 1）。不是課本那種帶系統天花板的 PCP。

所以：PIP 在 kernel（rt_mutex + PI futex）實作；HLP 式的天花板在 glibc 用 `sched_setscheduler` 實作；課本完整的 PCP（系統天花板）Linux 沒有提供。

---

## 8. 第 5 章總結

[52:00]–[53:01] 教授回顧第 5 章走過的路：

| 主題 | 講次 |
|---|---|
| 同步：data race、lock、deadlock、pthreads、atomics | 第 18 講 |
| 記憶體模型、linearizability、SC、fence | 第 19 講 |
| 進度保證、semaphore、reader-writer lock、wait-free queue | 第 20 講 |
| 並行 queue、condition variable、barrier | 第 21 講 |
| kernel 的 spinlock、mutex、lockdep | 第 22 講 |
| RCU | 第 23 講 |
| 排程：KSW、SJF、EDF、SRTF、FIFO、round robin | 第 24 講 |
| 多處理器排程、Banker's algorithm | 第 25 講 |
| Linux 的排程類別與 CFS | 第 26 講 |
| 即時排程：EDF、RMS、Lehoczky、DMS | 第 27 講 |
| 即時資源分配：PIP、HLP、PCP | 第 28 講 |

[53:01] 下一章：**記憶體系統**。

【課本補充】課本 §5.6.2 延伸閱讀：Herlihy & Shavit《The Art of Multiprocessor Programming》（lock-free、wait-free）；Ben-Ari 的並行與分散式程式設計；Karger、Stein、Wein 的排程綜述；即時系統：Mall、Krishna & Shin、Jane Liu；Davis & Burns 的硬即時排程綜述（2011）；Linux 排程器的評估（Lozi et al., 2016）等。

---

## 9. 一頁速查表

### 9.1 問題

| 現象 | 意思 |
|---|---|
| 有界反轉 | 高優先權等低優先權用完資源 |
| 無界反轉 | 低優先權被中優先權一直搶占 → 高優先權等無限久 |
| 二次阻塞 | 拿了一個資源後，拿下一個時被擋 |
| 鏈式阻塞 | T1 → T2 → T3 |
| 繼承阻塞 | 沒要資源，卻被提高了優先權的 task 擋住 |

沒有二次阻塞 ⟹ 沒有鏈式阻塞 ⟹ 沒有 deadlock。

### 9.2 三個協定

| | 規則 | 無界反轉 | deadlock | 鏈式 / 二次阻塞 | 繼承阻塞 |
|---|---|---|---|---|---|
| PIP | 被擋時持有者繼承等待者的優先權 | ✓ 解決 | ✗ 會 | ✗ 會 | 小 |
| HLP | 拿到 R 就變 max(目前, ceil(R)+1) | ✓ | ✓ 沒有 | ✓ 沒有（連資源阻塞都沒有） | **大** |
| PCP | 原始優先權 > CSC 或持有關鍵資源才給；加上 PIP 式繼承 | ✓ | ✓ 沒有 | ✓ 沒有 | 受控 |

ceil(R) = 可能要 R 的 task 的最大原始優先權；CSC = 目前被拿著的資源的天花板最大值。

### 9.3 Linux

| 機制 | 對應 |
|---|---|
| rt_mutex（`pi_waiters`、`pi_top_task`、`pi_blocked_on`） | PIP；走鏈繼承、偵測 `-EDEADLK` |
| `PTHREAD_PRIO_INHERIT` → `FUTEX_LOCK_PI` | PIP |
| `PTHREAD_PRIO_PROTECT` → glibc `sched_setscheduler` | 立即天花板（≈ HLP） |

### 9.4 本講看過的原始碼位置

| 位置 | 內容 |
|---|---|
| `kernel/locking/rtmutex.c:1-17` | rt_mutex：with PI support |
| `include/linux/sched.h:1128-1140` | task 的 PI 欄位 |
| `kernel/locking/rtmutex.c:475-485` | `rt_mutex_adjust_prio`：繼承最高等待者的優先權 |
| `kernel/locking/rtmutex.c:550-569` | 調整優先權鏈、deadlock 偵測 |

---

## 10. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **記憶體系統** | **第 29–33 講**：第 6 章 |
| **PIP、HLP、PCP 的完整理論與更多協定** | 另一門課 Real Time Systems（Rajib Mall 課本，資源共享一章） |
| **`PREEMPT_RT` 把鎖換成 rt_mutex** | 待補（本課程未專講） |

（講次對應依播放清單標題推估。）

---

## 附：本講一句話總結

> **低優先權 task 拿著高優先權 task 要的資源，就是優先權反轉；如果低優先權 task 又一直被中優先權 task 搶占，反轉就沒有上限（實測沒有協定時 H 要等到 t = 11，有協定 t = 5 就做完）。PIP 讓持有資源的 task 暫時繼承等待者的優先權，解決了無界反轉，但仍會 deadlock 和鏈式阻塞（實測反向拿兩把鎖就卡死；Linux 的 rt_mutex 和 PI futex 就是 PIP，只能沿鏈偵測出 `-EDEADLK`）。HLP 一拿到資源就把優先權拉到天花板 + 1，證明出完全沒有資源阻塞，因而沒有二次阻塞、鏈式阻塞和 deadlock，但會讓不相干的中優先權 task 被繼承阻塞（glibc 的 PRIO_PROTECT 就是這種立即天花板）。PCP 定義系統天花板 CSC：原始優先權大於 CSC、或手上有設定 CSC 的資源，才給資源，就算資源空著也不給；搭配 PIP 式繼承，用「之後的 task 不會覬覦之前被拿走的資源」這個引理證明沒有二次阻塞，於是也沒有鏈式阻塞和 deadlock，繼承阻塞又比 HLP 輕。第 5 章（同步與排程）到此結束，下一章是記憶體系統。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 28（Chapter 5, Part XI）錄影逐字稿（YouTube 自動字幕，53:16）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§5.5.5–5.6，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
