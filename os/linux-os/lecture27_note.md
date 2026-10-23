# Operating Systems: A Linux Kernel-Oriented Approach 第二十七講完整筆記：即時排程（Chapter 5, Part X）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 27（Chapter 5, Part X：Real-time Schedulers）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（48:35，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§5.4.7、§5.5–5.5.4（p.255–265）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
> **標記說明**：
> - `【口頭】`：教授在課堂上口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與教授回答
> - `【課本補充】`：課本有、但這堂課沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（`檔名:行號`），用來對照觀念
> - `【實測】`：在本機（Ubuntu、kernel 6.5.0、x86-64）實際執行的結果（本講多為 Python 模擬）
> - `⚠️ 逐字稿缺字`：自動字幕在這裡掉字或聽不清，內容依上下文或課本補上；也用來標示口誤或需要修正之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 這堂課要做什麼](#0-這堂課要做什麼)
- [1. Linux 原生的即時支援](#1-linux-原生的即時支援)
  - [1.1 Deadline 類別：EDF + 紅黑樹](#11-deadline-類別edf--紅黑樹)
  - [1.2 【原始碼】`sched_dl_entity` 與挑最早截止的](#12-原始碼sched_dl_entity-與挑最早截止的)
  - [1.3 【原始碼】Linux 也做 U ≤ 1 的檢查](#13-原始碼linux-也做-u--1-的檢查)
  - [1.4 RT 類別：100 條 queue](#14-rt-類別100-條-queue)
  - [1.5 為什麼一般 Linux 不能做保證](#15-為什麼一般-linux-不能做保證)
- [2. 即時系統的三種類型](#2-即時系統的三種類型)
- [3. 可排程性（schedulability）](#3-可排程性schedulability)
- [4. EDF 與週期性 task](#4-edf-與週期性-task)
- [5. RMS：靜態優先權](#5-rms靜態優先權)
  - [5.1 Rate monotonic](#51-rate-monotonic)
  - [5.2 Liu–Layland 界限](#52-liulayland-界限)
- [6. Lehoczky 測試](#6-lehoczky-測試)
- [7. DMS：截止時間小於週期](#7-dms截止時間小於週期)
- [8. 【實測】模擬四組 task](#8-實測模擬四組-task)
- [9. 預告：資源分配協定](#9-預告資源分配協定)
- [10. 一頁速查表](#10-一頁速查表)
- [11. 與後續講次的連結](#11-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

[00:35] 兩部分：

1. 一般（vanilla）Linux kernel 的即時排程：deadline 類別、RT 類別。
2. 專門的**即時作業系統**用的演算法：EDF、RMS、DMS，以及它們的**保證**。

[01:00] 教授：在即時系統裡，**保證**才是最重要的。

（另一門課 Real Time Systems 用的課本，就是本講引用的 Rajib Mall。）

---

## 1. Linux 原生的即時支援

### 1.1 Deadline 類別：EDF + 紅黑樹

[01:00] **deadline（dl）排程類別**：每個 task 有截止時間，優先權很高，**只在 stop 之下**。

[01:30] 就像 CFS 有 `pick_next_task_fair`，deadline 類別也有自己的挑選函式，用的是 **EDF**（第 24 講）：

- 維護一棵**紅黑樹**，以**截止時間**為鍵。
- [02:32] 樹會在對數時間內告訴你哪個截止時間最早。

[01:30]–[02:32] 名詞提醒：

- 即時系統文獻裡，**task** 是會產生 job 的東西：週期性 task 每個週期產生一個 **job**。
- Linux 裡的 task 是一個 `task_struct`，也就是一個 thread。
- 所以在 Linux 的 deadline 排程器裡，**一個 task 就是一個 job**，意思要看上下文。

[03:00] 紅黑樹節點跟 `list_head` 一樣用 `rb_node` 嵌在結構裡，用巨集拿回外層的 `sched_dl_entity`，再找到 `task_struct`（第 26 講 §4.1）。

### 1.2 【原始碼】`sched_dl_entity` 與挑最早截止的

`include/linux/sched.h:601-613`

```c
struct sched_dl_entity {
	struct rb_node			rb_node;

	/*
	 * Original scheduling parameters. Copied here from sched_attr
	 * during sched_setattr(), they will remain the same until
	 * the next sched_setattr().
	 */
	u64				dl_runtime;	/* Maximum runtime for each instance	*/
	u64				dl_deadline;	/* Relative deadline of each instance	*/
	u64				dl_period;	/* Separation of two instances (period) */
	u64				dl_bw;		/* dl_runtime / dl_period		*/
	u64				dl_density;	/* dl_runtime / dl_deadline		*/
```

這幾個欄位**完全對應**本講的符號：

| 欄位 | 本講符號 | 意思 |
|---|---|---|
| `dl_runtime` | d_i（執行時間） | 每個 instance（job）最多跑多久 |
| `dl_deadline` | D_i（相對截止時間） | job 釋出後多久要做完 |
| `dl_period` | P_i（週期） | 兩個 job 之間隔多久 |
| `dl_bw` | **d_i / P_i** | 這個 task 的**使用率**（§4） |
| `dl_density` | d_i / D_i | D < P 時用（§7 的 DMS 也看這個比例） |

（「instance」就是即時文獻裡的 job。註解直接說了。）

樹的排序：

`kernel/sched/deadline.c:1533-1536`

```c
static inline bool __dl_less(struct rb_node *a, const struct rb_node *b)
{
	return dl_time_before(__node_2_dle(a)->deadline, __node_2_dle(b)->deadline);
}
```

以**絕對截止時間** `deadline` 比大小（`dl_time_before` 處理計時器繞回）。

挑下一個：

`kernel/sched/deadline.c:2005-2013`

```c
static struct sched_dl_entity *pick_next_dl_entity(struct dl_rq *dl_rq)
{
	struct rb_node *left = rb_first_cached(&dl_rq->root);

	if (!left)
		return NULL;

	return __node_2_dle(left);
}
```

最左節點 = 截止時間最早 = **EDF**。跟 CFS 的 `__pick_first_entity` 一模一樣，只是鍵從 vruntime 換成截止時間。最左節點有快取，所以是 O(1)；插入刪除是 O(log n)。

### 1.3 【原始碼】Linux 也做 U ≤ 1 的檢查

§4 會講：EDF 在 **U = Σ d_i/P_i ≤ 1** 時一定排得出來。Linux 設定 `SCHED_DEADLINE` 時，就是做這個檢查（**admission control，准入控制**）：

`kernel/sched/deadline.c:224-229`

```c
static inline bool
__dl_overflow(struct dl_bw *dl_b, unsigned long cap, u64 old_bw, u64 new_bw)
{
	return dl_b->bw != -1 &&
	       cap_scale(dl_b->bw, cap) < dl_b->total_bw - old_bw + new_bw;
}
```

`kernel/sched/deadline.c:2848-2853`（`sched_dl_overflow`，定義在第 2822 行）

```c
	if (dl_policy(policy) && !task_has_dl_policy(p) &&
	    !__dl_overflow(dl_b, cap, 0, new_bw)) {
		if (hrtimer_active(&p->dl.inactive_timer))
			__dl_sub(dl_b, p->dl.dl_bw, cpus);
		__dl_add(dl_b, new_bw, cpus);
		err = 0;
```

| 部分 | 說明 |
|---|---|
| `total_bw` | 目前所有 deadline task 的 `dl_bw`（使用率）總和 |
| `new_bw` | 新 task 的 d/P |
| `dl_b->bw` 乘上 `cap` | 上限 = 每顆 CPU 允許的比例 × CPU 容量 |
| 加進去會超過 → overflow | `sched_setattr()` 失敗，回傳 `EBUSY`，這個 task **不准**變成 deadline task |

允許的比例從哪來？

`kernel/sched/deadline.c:505`

```c
		dl_b->bw = to_ratio(global_rt_period(), global_rt_runtime());
```

【實測】

```
$ cat /proc/sys/kernel/sched_rt_runtime_us /proc/sys/kernel/sched_rt_period_us
950000
1000000
```

950000 / 1000000 = **95%**。所以 Linux 的檢查是 **Σ d_i/P_i ≤ 0.95 × CPU 數**，留 5% 給一般 task，避免它們餓死。這就是本講 EDF 定理的實務版本。

⚠️ 多核心上，全域 EDF 在 U ≤ m 時**不保證**能排出來（理論上只保證「延遲有上界」），所以 Linux 的這個檢查在多核心上保證的是 soft real-time。

（補充：deadline 類別實際上是 **EDF + CBS（Constant Bandwidth Server）**：每個 task 每週期最多只能用 `dl_runtime`，用完就被暫停到下個週期（throttling），一個失控的 task 不會拖垮別人的截止時間。）

### 1.4 RT 類別：100 條 queue

[03:31]–[07:31] deadline 類別之後是 **RT 類別**（第 24 講筆記 §10.4–10.5 有原始碼）：

- 即時優先權 0–99，每個優先權一條 queue。
- [04:01] 從最高優先權往下看，queue 不空就先服務它。[04:32] 只有它們**全部阻塞**了，才看低優先權的。
- [05:00] 一個位元向量，每個優先權一個位元：1 = queue 不空。[05:30] 找第一個 1。
- 同一優先權內兩種做法：
  - [05:30] **FIFO**：照第一次進 run queue 的時間排。
  - [07:01] **round robin**：每個跑一個時間片，跑完移到 queue 尾。[07:31] 教授說也可以不移動、只移一個指標（像 WSClock 演算法），但新 task 進來時實際移動比較好處理。

⚠️ [04:01] 教授說「從最高的 99 往下看到 0」。使用者看到的即時優先權（`chrt`）99 最高；kernel 內部反過來，0 最高，所以 bitmap 是「找**第一個** 1」。

[04:32] 公平性完全沒有：優先權 25 的跑完（或都阻塞了），才輪到 24。[06:30] 教授：**優先權被看得太認真**，連低即時優先權或一般 context 的 kernel 工作都可能拿不到 CPU，系統會不穩定。[05:00] 所以這種太死板的做法在即時系統裡**通常不受歡迎**，但一般 Linux 就是這樣。

### 1.5 為什麼一般 Linux 不能做保證

[08:02] 重點：上面兩種（deadline 和 RT）**都不做保證**。

[08:30] 因為它們跑在**通用**的環境裡：沒辦法限制會有哪些 job。

[08:30] 如果是一個**受限的**即時系統：task 是已知的、執行時間已知，知道的資訊多很多，就能做保證。

【課本補充】課本列出一般 Linux 不適合飛彈、火箭、國防設備、醫療器材的原因：

1. kernel 很大一部分**不可搶占**。
2. 高優先權 task 可能被低優先權 task **擋很久**（優先權反轉，下一講）。
3. 排程演算法**不保證**遵守截止時間。

所以需要從頭設計的即時 kernel。（補充：Linux 有 `PREEMPT_RT` 修補，讓 kernel 幾乎全部可搶占、spinlock 改成可睡的 rt_mutex，大幅縮短延遲，但仍不等於硬即時的形式保證。）

---

## 2. 即時系統的三種類型

[09:00] 每個 task 都有截止時間，但依「錯過了會怎樣」分三種：

```
效用
 1 ┼────────┐                  1 ┼────────┐                 1 ┼────────┐
   │        │╲                   │        │                   │        │
   │        │  ╲                 │        │                   │        │
 0 ┼────────┼────────▶         0 ┼────────┼──────▶          0 ┼────────┼──────▶
           截止   完成時間             截止                    │      截止
                                                             ▼ 系統失敗
   Soft                           Firm                         Hard
```

| 類型 | 錯過截止時間 | 例子 |
|---|---|---|
| **Soft（軟）** | [09:31] 效用**降低但不是 0** | [09:31] 影音：60 Hz 影片每格 16.66 ms。[10:36] 錯過了會有點抖動、畫面瑕疵；錯過多少、多少格錯過，決定品質降多少 |
| **Firm（硬性但非致命）** | [11:00] 效用**變 0**，但**不是系統失敗** | [11:00] ATM：插卡、輸入密碼，錢沒及時出來，交易取消。沒有效用，但不是災難 |
| **Hard（硬）** | [11:31] 效用 0，而且**整個系統失敗** | [11:31] 飛彈因為某個 task 沒準時完成而錯過目標 |

[12:01] 本講主要看第三種：火箭、航電的**硬即時系統**。

【課本補充】課本：

- Soft：一般 Linux 支援，用 RT 或 deadline 排程器，**高機率**能守住截止時間，偶爾會錯過。
- Firm 的另一個例子：機票訂位要協調信用卡公司、銀行、航空公司、訂票網站、客戶端 app，延遲太久就訂不成。
- Hard 的其他例子：心律調節器、飛機防撞系統、核電廠控制系統。
- Firm 和 Hard 都要**重新設計作業系統和硬體**，還要有理論框架證明 task 可排程。

---

## 3. 可排程性（schedulability）

[12:01] 硬即時系統的設定：

- **task 是已知的**。
- task 通常是**週期性**的（最漂亮的結果都是針對週期性 task）。
- 執行時間已知、截止時間已知。
- [12:31] 截止時間是**每個 job** 的：job 在 t = 0 釋出，要在什麼時候前做完。

[12:31] 這個領域的核心問題：

> 一組週期性 task，每個 job 的執行時間和截止時間都已知，**這組 task 可排程嗎**？

[13:02] **可排程（schedulable）**：沒有任何 job 錯過截止時間。

[13:30] 也有**最佳性**的概念：演算法 A 是最佳的，意思是「只要存在一個可行的排程（沒有 job 錯過截止），A 就找得到」。

[13:30] 教授：這是一個**非常理論**的領域。

【課本補充】課本：通常考慮**截止時間 = 週期**的週期性 task。例如週期 3 秒：每 3 秒產生一個 job，要在下一個 job 產生前做完。

---

## 4. EDF 與週期性 task

[13:30]–[14:32] 回顧：EDF 是單處理器演算法，在「可搶占、每個 job 有截止時間、到達（**釋出，release**）時間不同」時，最小化 L_max（第 24 講 §5）。

[14:32] 週期性系統可以當成非週期系統：L_max ≤ 0 就表示沒有 job 錯過截止、系統可排程。[15:00] 而且因為最佳性，有可行排程 EDF 就找得到。

[15:31] 但週期性系統有一個**更漂亮、更實用**的結果，所以大家談週期性系統時通常不用 L_max 的結果。

[15:31]–[16:31] 設定：

- 可搶占，**計算排程和搶占的成本都是 0**。
- task i 的執行時間 d_i、週期 P_i：t = 0 釋出的 job 要跑 d_i 單位，下一個 job 在 P_i 釋出。
- [16:31] 假設**截止時間 = 週期**：下一個 job 出來時，目前的 job 一定已經做完。

[17:02] **使用率（utilization）**：

```
U = Σ_{i=1}^{n} d_i / P_i
```

d_i / P_i 是 task i 在一個週期裡要忙的比例；加起來就是整個系統的使用率。[17:31] 夠長的一段時間裡，平均有多少比例在忙。

[17:31] **U > 1**：要做的工作比時間多，常識就知道**不可能**排得出來。

[18:02]–[19:00] 定理：

> **U ≤ 1 ⟺ 可排程，而且 EDF 一定排得出來。**（可搶占、搶占成本 0、截止時間 = 週期）

> **課本 Point 5.5.1**：如果總使用率 Σ d_i/P_i ≤ 1，一定能用 EDF 找到可行排程。

[19:30] 教授的評語：對週期性系統，只要用這個公式算使用率。大於 1，一定有 job 錯過截止；小於等於 1，**閉上眼睛用 EDF 排**，保證找到可行排程。

【課本補充】課本說這可以說是單處理器即時系統**最重要**的結果之一，用得很多、很好實作：用一個 priority queue 照截止時間排就好（Linux 用紅黑樹，§1.2）。證明用跟 SJF 一樣的交換論證。

**EDF 的缺點：動態優先權**

[20:03]–[21:02]

- 一個 job 在跑，另一個截止時間更近的 job 來了，就要**馬上搶占**。
- 同一組 task 在不同情況下，task 1 和 task 2 誰優先可能**相反**：看當下誰的截止時間比較近。
- 優先權不是綁在 task 上，而是綁在**當下的截止時間**上，要一直追蹤，簿記很多。

---

## 5. RMS：靜態優先權

### 5.1 Rate monotonic

[21:02] 另一種系統：**靜態優先權**。優先權在**一開始**就決定好，排程時不看截止時間（但截止時間還是不能錯過）。[21:31] 這樣好管理多了。

[21:31] 一樣假設截止時間 = 週期。優先權：

```
priority_i ∝ 1 / P_i      （週期越短，優先權越高）
```

[22:32] 這叫 **RMS（Rate Monotonic Scheduling，速率單調排程）**：

- task 的**速率（rate）** = 1 / 週期。
- 優先權跟速率成正比，所以是「速率單調」。
- [23:01] 優先權是靜態的，但**還是可搶占**的，只是優先權不會動態改變。

⚠️ [23:01] 教授一度說「優先權跟週期成正比」，是口誤，應是**反比**（跟速率成正比），他前後都是這個意思。

【課本補充】課本：在 RMS 裡，job 的優先權 = task 的優先權。EDF 不是這樣：同一個 task 產生的不同 job 可以有不同優先權，一個 job 的優先權甚至會隨時間改變。硬即時系統通常很可預測，task 都事先知道，最大執行時間和週期都很清楚，很適合 RMS。

### 5.2 Liu–Layland 界限

[22:32] 經典的 **Liu & Layland**（1973）結果：

> n 個 task，如果
> ```
> U ≤ n · (2^{1/n} − 1)
> ```
> 就**一定**能用 RMS 排程。

⚠️ [23:32] 逐字稿是「if not only if」，應是 **if（充分條件）**，不是 iff。教授接著也說「這是充分條件」。

[24:02]

| n | 界限 |
|---|---|
| 1 | 1 |
| 2 | **0.828**（教授說 83%） |
| 3 | 0.780 |
| ∞ | **ln 2 ≈ 0.693**（69%） |

[24:30] 意義：使用率低於 69%，**不管幾個 task**，一定可排程。低使用率表示時間軸上有很多空檔。

[25:01] 但這個界限**很保守**：它是充分條件、不是必要條件，**不是緊的**。很多時候可以做得更好。

【實測】

```
Liu-Layland bound: n=1:1.000, n=2:0.828, n=3:0.780, n=5:0.743, n=10:0.718, n=100:0.696  ln2=0.693
```

---

## 6. Lehoczky 測試

[25:31] 更多結果（見參考文獻）：

> 從可排程性的角度，**最壞情況**是所有 task **同相（in phase）**：都在 t = 0 一起開始。（證明見參考文獻。）

[26:02] 所以用 **Lehoczky 測試**：假設最壞情況，只要證明**每個 task 都趕上它的第一個截止時間**，就完成了。

[26:02] 為什麼只看第一個？最壞情況下第一個 job 都趕上了；之後 task 就不再同相（除非週期互為倍數），排程更寬鬆。

[26:30] 即使 U 超過 Liu–Layland 界限，只要通過這個測試，就是可排程的。

[27:01] 怎麼驗證每個 task 都趕上第一個截止時間？

1. [27:30] 照 RMS 優先權排序：task 1 週期最小，……，task n 週期最大。對 task i 來說，前面 i − 1 個都比它優先。
2. [28:01] 給一段時間 t，task j 在 t 裡面有幾個週期（完整或部分）？ **⌈t / P_j⌉**。
   - [28:33] 例：t = 10、P_j = 3 → ⌈10/3⌉ = 4：3 個完整的加 1 個部分的。
3. [29:00] 最壞情況：這 4 個週期裡，task j 都**完整**跑完。單核心上它總共要 d_j × ⌈t/P_j⌉。
4. [29:31] 前 i 個 task 在 t 裡的總負載：

```
W_i(t) = Σ_{j=1}^{i} d_j · ⌈t / P_j⌉
```

5. [30:02] 平均負載：**Q_i(t) = W_i(t) / t**。
6. [30:33] 在 0 < t ≤ P_i 之間，找讓 Q_i(t) **最小**的 t：

```
Q_i = min_{0 < t ≤ P_i} Q_i(t)
```

7. [31:01] **task i 可排程 ⟺ Q_i ≤ 1**。
8. [34:31] 整個系統：**Q = max_i Q_i ≤ 1 ⟺ 所有 task 都可排程**。（全部的 Q_i ≤ 1 等於最大的 ≤ 1。）

（課本式 5.8。）

[31:32]–[34:01] 教授的直覺說明：

- **⇐**：假設在 t* 時 Q_i(t*) ≤ 1，也就是 W_i(t*) ≤ t*。W_i 是前 i 個 task 的**最大可能**負載（連部分週期都當成完整跑完）。它 ≤ t*，表示它們全部在 t* 前都能做完，特別是 task i 的第一個 job 在 t* 做完了，所以 task i 可排程。
- **⇒**：假設 task i 可排程，就看它第一個 job 做完的那一刻 t*：那時 task i 做完了，比它優先的 task 也沒有工作要做了，這個條件就成立。
- 精確的數學在參考文獻 3。

[35:31] 教授：Lehoczky 測試能證明一些**違反 Liu–Layland 界限**的 task 組合是可排程的，因為 Liu–Layland 太保守。它剩下的保守只來自「部分週期也當成完整跑完」這個假設。

[42:31] 教授後來補充：Lehoczky 測試只說「存在一個 t 讓 Q_i 最小」，沒給演算法找它。簡單的情況可以手算。（補充：只需要檢查 **「排程點」**：t = k·P_j（j ≤ i，k·P_j ≤ P_i）。W_i(t) 是階梯函數，只在這些點跳，所以 Q_i(t) 的最小值一定出現在這些點上。下面的實測就是這樣算的。）

---

## 7. DMS：截止時間小於週期

[36:01] 之前 EDF 和 RMS 都假設截止時間 = 週期。**DMS（Deadline Monotonic Scheduling，截止時間單調排程）**允許**截止時間 < 週期**（D_i < P_i；等於的情況就是 RMS）。

[36:31] job 在週期開始時產生，但要在**週期結束前的某個截止時間**做完。

[37:00] 優先權改成：

```
priority_i ∝ 1 / D_i      （截止時間越短，優先權越高）
```

截止時間 = 週期時，就退化成 RMS。DMS 是 RMS 的推廣。

[37:30] DMS 沒有像 RMS 那樣簡單的使用率公式，但可以用 Lehoczky 測試的概念。

[37:30]–[40:02] 對 task i：

- 它的使用率貢獻是 d_i / D_i（執行時間 / 截止時間）。
- [38:31] **干擾（interference）I_i**：比它優先的前 i − 1 個 task，在它的截止時間之前搶走的時間。
- [39:30] 常識條件：**I_i + d_i ≤ D_i**。截止時間前，task i 要能跑完，前面 i − 1 個要做的也都做完。

[40:02] 干擾的算法（課本式 5.9），跟 Lehoczky 一樣：

```
I_i(t) = Σ_{j=1}^{i−1} ⌈t / P_j⌉ · d_j
```

[41:00] 教授指出：這就是 Lehoczky 測試裡的 W_{i−1}(t)，前 i − 1 個 task 的 CPU 負載。Lehoczky 的公式其實就是從干擾來的。

[42:31] DMS 給了一個**演算法**，把「容易出事的時間點」算出來（課本 Algorithm 4）：

```
for 每個 task τ_i（照 D 由小到大排好）:
    t ← Σ_{j=1}^{i} d_j               ▷ 最早可能做完的時間
    cont ← true
    while cont:
        if t > D_i: return false      ▷ 已經超過截止時間，沒救了
        if I_i(t) + d_i ≤ t:
            cont ← false              ▷ task i 在 t 之前做得完
        else:
            t ← I_i(t) + d_i          ▷ 換一個更晚的估計再試
```

[43:02]–[47:00] 逐步：

1. [43:31] 起點 t = 前 i 個 task 的執行時間總和。這是最緊的點：task i 要做完，前面的都要先做完，而大家同相開始，這是 task i **最早**可能做完的時間。
2. [44:01] t 超過截止時間 → 第一個截止時間就錯過了，不可排程。
3. 干擾 + 自己的執行時間 ≤ t → 在截止前排得完，task i 可排程。
4. [44:30] 否則還不用放棄，還沒到截止時間。[45:02] 下一個估計：**t_新 = I_i(t_舊) + d_i**。（[46:00] 教授提醒這是左值右值的問題：右邊用的是舊的 t。）意思是：到 t_舊 為止的干擾全部算進去，task i 再接在後面跑。
5. [46:30] 每次 t 都變大，最後不是通過，就是超過截止時間。
6. 每個 task 都做一次。任何一個不可排程就回傳 false；全部都可以才是 true。

[47:00] 教授：重點是找到**感興趣的時間點**。Lehoczky 測試也一樣，不是每個點都要看。DMS 把它們列舉得更好。前者比較理論，DMS 是演算法；兩者的核心概念都是**干擾**。

（補充：這個迭代就是即時系統文獻裡的 **response time analysis**，t 收斂到的值就是 task i 的最壞回應時間。）

---

## 8. 【實測】模擬四組 task

用 Python 做單位時間、可搶占的模擬，跑滿一個超週期（所有週期的最小公倍數），同時算 Lehoczky 的 Q_i（`rt.py`）。task 寫成 (d, P, D)：

```
A (U<=LL)            U=0.708 LL=0.780  EDF:OK               RMS:OK               Lehoczky Q_i=[0.25, 0.667, 0.833] -> sched
B (LL<U<=1)          U=0.971 LL=0.828  EDF:OK               RMS:MISS T2 at t=7   Lehoczky Q_i=[0.4, 1.143] -> not
C (LL<U, harmonic)   U=1.000 LL=0.828  EDF:OK               RMS:OK               Lehoczky Q_i=[0.5, 1.0] -> sched
D (U>1)              U=1.100 LL=0.828  EDF:MISS T1 at t=20  RMS:MISS T2 at t=6   Lehoczky Q_i=[0.6, 1.2] -> not
```

| 組 | task (d, P) | U | 結果 | 說明 |
|---|---|---|---|---|
| A | (1,4) (2,6) (1,8) | 0.708 | 都 OK | U 低於 Liu–Layland 界限 → RMS 保證可排程 |
| B | (2,5) (4,7) | 0.971 | **EDF OK、RMS 錯過** | U ≤ 1 → EDF 一定行。超過界限 0.828，RMS 不保證；Lehoczky Q₂ = 1.143 > 1，模擬也在 t = 7 錯過 |
| C | (2,4) (4,8) | **1.000** | 都 OK | 超過界限，但週期互為倍數（harmonic），RMS 也能到 100%。Lehoczky Q₂ = 1.0 ≤ 1 → 可排程。這就是「界限保守、Lehoczky 比較準」的例子 |
| D | (3,5) (3,6) | 1.100 | 都錯過 | U > 1，任何演算法都不行 |

- B 組：t = 0 時 T1（週期 5，優先）先跑 0–2，T2 跑 2–5；t = 5 T1 又來了，搶占 T2 跑 5–7；T2 只跑了 3 個單位，在截止時間 7 錯過。EDF 在 t = 5 時比較截止時間：T2 是 7、T1 是 10，讓 T2 先跑完，就沒事。這正是動態優先權的好處。
- Lehoczky 的結果和模擬**每一組都一致**（它是充要條件）。

**DMS**：(d, P, D) = (1, 10, 2)、(2, 5, 5)、(1, 20, 20)，截止時間 < 週期：

```
DMS set [(1, 10, 2), (2, 5, 5), (1, 20, 20)] U=0.550
  RMS-order sim: MISS T1 at t=2   DMS sim: OK   DMS test per task (by D): [True, True, True]
```

- 使用率才 0.55，但用 **RMS**（看週期）時，T2（週期 5）比 T1（週期 10）優先，T2 先跑 0–2，T1 截止時間 2 就錯過了。
- 用 **DMS**（看截止時間）時，T1（截止 2）最優先，全部 OK。
- DMS 演算法對三個 task 都回傳可排程，跟模擬一致。

---

## 9. 預告：資源分配協定

[47:31] 排程演算法講完了。接下來看**資源分配的協定**：

- **PIP**（Priority Inheritance Protocol，優先權繼承協定）
- **HLP**（Highest Locker Protocol）
- **PCP**（Priority Ceiling Protocol，優先權天花板協定）

[48:00] 留到下一講。

---

## 10. 一頁速查表

### 10.1 Linux 即時類別

| 類別 | 演算法 | 資料結構 | 保證 |
|---|---|---|---|
| deadline | EDF（+ CBS） | 以絕對截止時間為鍵的紅黑樹 | 准入控制：Σ runtime/period ≤ 95% × CPU 數 |
| rt | 固定優先權；同級 FIFO / RR | 100 條 queue + bitmap | 無 |

### 10.2 即時系統類型

Soft（效用遞減）、Firm（效用 0、不失敗）、Hard（系統失敗）。

### 10.3 可排程性測試

| 演算法 | 優先權 | 條件 | 性質 |
|---|---|---|---|
| EDF | 動態：截止時間最早 | U ≤ 1 | 充要；最佳 |
| RMS | 靜態：週期越短越高 | U ≤ n(2^{1/n} − 1)（→ ln 2 ≈ 0.693） | 只是充分 |
| RMS + Lehoczky | 同上 | max_i min_{t ≤ P_i} W_i(t)/t ≤ 1 | 充要（同相是最壞情況） |
| DMS | 靜態：截止時間越短越高 | 迭代 t ← I_i(t) + d_i，看是否 ≤ D_i | 適用 D < P |

U = Σ d_i/P_i；W_i(t) = Σ_{j≤i} d_j⌈t/P_j⌉；I_i(t) = W_{i−1}(t)。

### 10.4 本講看過的原始碼位置

| 位置 | 內容 |
|---|---|
| `include/linux/sched.h:601-613` | `sched_dl_entity`：runtime、deadline、period、bw |
| `kernel/sched/deadline.c:1533-1536` | 紅黑樹依絕對截止時間排序 |
| `kernel/sched/deadline.c:2005-2013` | `pick_next_dl_entity`：最左 = EDF |
| `kernel/sched/deadline.c:224-229` | `__dl_overflow`：使用率總和是否超過上限 |
| `kernel/sched/deadline.c:2848-2853` | `sched_dl_overflow`：准入控制 |
| `kernel/sched/deadline.c:505` | 上限 = rt_runtime / rt_period（95%） |

---

## 11. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **優先權反轉、PIP、HLP、PCP** | **第 28 講**：Real-time resource allocation |
| **`PREEMPT_RT`、rt_mutex** | **第 28 講**（PIP 的 Linux 實作） |
| **EDF、RMS、DMS 的完整證明、多處理器即時排程** | 另一門課 Real Time Systems（Rajib Mall 課本） |
| **第 5 章總結** | **第 28 講**（課本 §5.6） |

（講次對應依播放清單標題推估。）

---

## 附：本講一句話總結

> **一般 Linux 的 deadline 類別就是 EDF：`sched_dl_entity` 存著 runtime（d）、deadline（D）、period（P），用以絕對截止時間為鍵的紅黑樹 O(1) 挑最左邊的，設定時還做 Σd/P ≤ 95% 的准入檢查；RT 類別則是 100 條優先權 queue，太死板、不做保證。專門的即時系統依錯過截止的後果分 soft、firm、hard，核心問題是「一組週期性 task 可不可排程」：EDF（動態優先權）在 U = Σd_i/P_i ≤ 1 時一定排得出來；RMS（靜態，週期越短越優先）只保證 U ≤ n(2^{1/n} − 1)（趨近 69%），太保守，改用 Lehoczky 測試假設所有 task 同相、檢查每個 task 的第一個截止時間（Q_i = min W_i(t)/t ≤ 1）就是充要條件；截止時間小於週期時用 DMS（截止時間越短越優先），以干擾 I_i(t) 迭代 t ← I_i(t) + d_i 直到通過或超過截止。實測 U = 0.971 的組合 EDF 排得出來、RMS 錯過，週期互為倍數的組合 RMS 可以到 100%，而截止時間較短的 task 用 RMS 會錯過、用 DMS 就沒事。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 27（Chapter 5, Part X）錄影逐字稿（YouTube 自動字幕，48:35）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§5.4.7、§5.5–5.5.4，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
