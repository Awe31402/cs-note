# Real Time Systems 第六講完整筆記：EDF 排程演算法

> **課程**：Real Time Systems — Lecture 6：EDF Scheduling Algorithm（播放清單第 6 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（44:12，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch2 §6–7（PDF p.46–50）
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
- [1. Event-driven 排程器](#1-event-driven-排程器)
  - [1.1 特性](#11-特性)
  - [1.2 Time-sliced round-robin：hybrid](#12-time-sliced-round-robinhybrid)
  - [1.3 Foreground-background 排程器](#13-foreground-background-排程器)
  - [1.4 【實測】背景任務完成時間](#14-實測背景任務完成時間)
- [2. 靜態 vs 動態優先權](#2-靜態-vs-動態優先權)
- [3. EDF 演算法](#3-edf-演算法)
  - [3.1 規則](#31-規則)
  - [3.2 可排程性條件：Σ eᵢ/pᵢ ≤ 1](#32-可排程性條件σ-eᵢpᵢ--1)
  - [3.3 證明](#33-證明)
  - [3.4 Deadline 小於週期](#34-deadline-小於週期)
  - [3.5 【實測】EDF 模擬器](#35-實測edf-模擬器)
- [4. EDF 是動態優先權嗎？](#4-edf-是動態優先權嗎)
- [5. EDF 的缺點](#5-edf-的缺點)
  - [5.1 暫時過載](#51-暫時過載)
  - [5.2 【實測】過載的骨牌效應](#52-實測過載的骨牌效應)
  - [5.3 資源共享、搶占](#53-資源共享搶占)
- [6. 實作：priority queue 與 O(1) 方法](#6-實作priority-queue-與-o1-方法)
  - [6.1 【原始碼】Linux SCHED_DEADLINE 就是 EDF](#61-原始碼linux-sched_deadline-就是-edf)
- [7. 變形：Minimum Laxity First](#7-變形minimum-laxity-first)
- [8. 一頁速查表](#8-一頁速查表)
- [9. 與後續講次的連結](#9-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

第一個 event-driven 排程演算法：**EDF（Earliest Deadline First，最早 deadline 優先）**。

1. Event-driven 排程器的特性；foreground-background 排程器。
2. EDF：每個排程點跑 deadline 最近的任務。
3. 最重要的結果：d = p 時，**Σ eᵢ/pᵢ ≤ 1 ⇔ EDF 排得出來**。老師花了一半時間證明它。
4. EDF 最佳、簡單，但**很少用**：過載時無法預測誰會錯過、實作開銷、資源共享。

---

## 1. Event-driven 排程器

### 1.1 特性

[00:00]–[05:31]

- 比 clock-driven **複雜**，但**更 proficient**：能排更多任務集合，能輕鬆處理偶發和非週期任務。用在更複雜的應用。
- 排程點：**任務到達**和**任務完成**（也可以加其他事件）。
- 主要是**可搶占**的：更高優先權的任務到達時，正在跑的任務可能被暫停、之後再恢復。需要硬體 ＋ OS 支援這種切換。
- **貪婪（greedy）**：只要有任務就緒，處理器就不會閒著。
- 簡化假設：**單處理器、任務彼此獨立**（不共享資源、沒有先後關係）。
- 兩個基石演算法：**EDF** 和 **RMA**（rate monotonic algorithm，下一講）。

### 1.2 Time-sliced round-robin：hybrid

[02:02]–[04:00]

- 幾乎所有 OS 都用：把每個任務切成時間片，輪流跑。
- 不是純 clock-driven：新任務來了就加入輪流、任務離開就調整。是 **hybrid**，現代 OS 都這樣。
- 傳統排程器的事件：任務完成、任務暫停（I/O、收到 suspend 訊號），加上計時器晶片產生的時鐘中斷。
- **忽略 deadline**，所以比 cyclic 排程器還不 proficient。本課不考慮它。

### 1.3 Foreground-background 排程器

[01:31]–[08:31]

- 最簡單的 event-driven 排程器。
- **前景**：即時任務（週期）。**背景**：偶發、非週期、非即時任務。
- 前景任務一來，就搶占背景任務。前景任務之間有優先權順序，每個排程點選最高的。
- 背景任務**只在沒有前景任務就緒時**才能跑。
- 【口頭】這和 Linux 的做法大致一致。

**背景任務 T_B 的完成時間**：

> **ct_B = e_B / (1 − Σᵢ eᵢ/pᵢ)**

- eᵢ/pᵢ：第 i 個前景任務佔用 CPU 的比例。加總 = 前景佔用的比例。
- 1 − 前景比例 = 背景能用的比例 → 分母是**減速因子（slowdown factor）**。
- 假設：context switch 時間是 0（或已併入執行時間），沒有其他開銷。

老師出了兩題【口頭】：

1. 前景任務 p = 100 ms、e = 50 ms、d = 100 ms，背景任務 1020 ms ⚠️，完成時間？
2. 同上，但每次 context switch 有 1 ms 開銷？

⚠️ 逐字稿是「1020 milliseconds」，課本 Example 2.3 是 **1000 ms**，可能是字幕聽錯。另外課本 Example 2.3 寫「p_f = 50 msec, e_f = 100 msec」，但計算用 1 − 50/100，顯然是 **e = 50、p = 100**（和課堂一致），課本把 p 和 e 寫反了。

【課本補充】（p.47–48）

- Ex 2.3：ct_B = 1000 / (1 − 50/100) = **2000 ms**。
- Ex 2.4：T1 每 20 ms 要 10 ms、T2 每 50 ms 要 20 ms → 前景 90%，背景每 10 ms 只拿到 1 ms → 100 ms 的背景任務要 **1000 ms**。
- Ex 2.5（加 1 ms context switch）：每個前景實例會造成兩次切換（切進來、切回背景），等於前景執行時間 50 → 52 → ct_B = 1000 / (1 − 52/100) ≈ **2083 ms**（課本寫 2083.4）。

### 1.4 【實測】背景任務完成時間

用 0.1 ms 為單位模擬課本 Ex 2.3 / 2.5（前景 p=100、e=50；背景 1000 ms；每次切換花 cs ms）：

```
no overhead : 2000.0 ms   formula 1000/(1-50/100) = 2000.0
1 ms switch : 2092.0 ms   book: 1000/(1-52/100) = 2083.3
```

- 沒有開銷時，和公式完全一樣。
- 有開銷時，模擬是 2092 ms，公式是 2083 ms。原因：公式假設背景「平均」每 100 ms 拿 48 ms（流體近似）。實際上前 20 個週期背景拿到 960 ms，到 t = 2000 時還差 40 ms；但 t = 2000 前景又來了，要先切換 1 ＋ 跑 50 ＋ 切回 1 → 2052 ＋ 40 = **2092**。公式忽略了最後一個不完整週期。

---

## 2. 靜態 vs 動態優先權

[09:01]–[10:01]

| | 靜態優先權（static） | 動態優先權（dynamic） |
|---|---|---|
| 優先權 | 程式設計師指定後，執行期間**不變** | 執行期間依相對急迫程度**改變** |
| 代表 | **RMA**：靜態優先權裡已被證明是最佳的（下一講） | **EDF**：單處理器上最佳 |

---

## 3. EDF 演算法

### 3.1 規則

[10:01]–[12:00]

> 每個排程點，從所有**就緒**任務中，派出 **deadline 最近**的那個。若正在跑的任務 deadline 比較遠，就**搶占**它。

- 假設：單處理器、任務獨立（沒有資源依賴、沒有先後順序）。
- **EDF 是單處理器上的最佳演算法**：若 EDF 排不出一組任務的可行排程，那就沒有任何演算法排得出來。
- 週期和非週期任務都能排。

### 3.2 可排程性條件：Σ eᵢ/pᵢ ≤ 1

[12:00]–[15:31]

先假設 **pᵢ = dᵢ**（EDF 最典型的設定）。

- 每個任務的**密度（density）**或使用率 = eᵢ/pᵢ。
- 總使用率 U = Σ eᵢ/pᵢ。

> **U = Σ eᵢ/pᵢ ≤ 1**（課本式 2.8）

- **必要條件**：單處理器同一時間只跑一個 job，U 不可能超過 1。
- **也是充分條件**：這是 EDF 最有趣的地方。只要 U ≤ 1（且 p = d），EDF 一定排得出可行排程。

例【口頭】：T1 = (e=1, p=3)、T2 = (e=4, p=6)。U = 1/3 + 4/6 = 1 → 排得出來。

【口頭】為什麼 d > p 不太合理：任務一直來，deadline 卻比週期長，任務會**堆積**。所以實際的情況是 d ≤ p。

### 3.3 證明

[16:00]–[35:31]

老師說：這個證明很重要，能讓你看到即時系統的證明怎麼做。最難的部分是**找到 t′**。

**設定**：任務 T₁…Tₙ，執行時間 e₁…eₙ，週期 p₁…pₙ，dᵢ = pᵢ，Σ eᵢ/pᵢ ≤ 1。

**反證**：假設 EDF 下有任務錯過 deadline。令 **t = 第一次**有任務錯過 deadline 的時刻。

**步驟 1：連續執行區段** [18:31]–[22:00]

- 看時間軸上一段連續執行、沒有空檔的區段，開頭是 tₓ。
- 在 tₓ − ε 時，**沒有任何 job 就緒**：否則 EDF（貪婪）一定會跑它，不會故意延遲。
- 所以區段裡執行的 job 都是 tₓ **之後**才到達的。
- 錯過 deadline 的任務 Tᵢ 在 t_y 到達，從 t_y 到 t 處理器一直在忙（否則它就會被執行）。這段連續區段可以往前延伸到某個 tₓ；t = 0 一定是這樣的點，所以 tₓ 一定存在。

**步驟 2：找 t′** [22:41]–[27:01]

- 在 t_y 到 t 之間執行的 job，deadline 一定都 ≤ t：否則 EDF 會先跑 Tᵢ。
- 從 t_y 往回走向 tₓ，找到這性質開始不成立的點，令它為 **t′**。
- 會不會有 job 在 tₓ 和 t′ 之間到達、deadline ≤ t、卻在 t′ 之後才執行？不會：如果它已經到達、deadline ≤ t，EDF 就不會在它之前跑 deadline > t 的任務，那 t′ 就可以往前移到它的到達時間。

**區間 [t′, t] 的性質**（長度 Δ）[27:01]–[28:58]：

1. **緊（tight）**：沒有閒置時間。
2. 這段時間執行的所有 job，都在這段時間**內到達**。
3. 它們的 deadline 都在這段時間**內**。
4. 除了錯過的那個，其他都在這段時間**內完成**。
5. t′ 之前到達的 job 不會在這段時間執行。

一個自給自足的小世界。所以 Δ ≥ dᵢ（對出現在其中的任務）。

**步驟 3：代數** [31:01]–[34:30]

任務 Tᵢ 在長度 Δ 的區間內，最多有 ⌊(Δ − dᵢ)/pᵢ⌋ + 1 個 job 的到達與 deadline 都落在區間裡。總需求：

```
D = Σᵢ ( ⌊(Δ − dᵢ)/pᵢ⌋ + 1 ) · eᵢ
  ≤ Σᵢ ( (Δ − dᵢ)/pᵢ + 1 ) · eᵢ          （拿掉 floor）
  = Σᵢ ( Δ/pᵢ − dᵢ/pᵢ + 1 ) · eᵢ
  = Σᵢ (Δ/pᵢ) · eᵢ                       （dᵢ = pᵢ，後兩項相消）
  = Δ · Σᵢ eᵢ/pᵢ
  ≤ Δ                                    （U ≤ 1）
```

**矛盾**：有 job 在 t 錯過 deadline，代表區間 [t′, t] 的需求**大於** Δ（時間不夠用）。但上面算出 D ≤ Δ。所以假設錯誤，EDF 不會錯過 deadline。

> 結論：pᵢ = dᵢ 時，**Σ eᵢ/pᵢ ≤ 1 是 EDF 可排程的充要條件**。

### 3.4 Deadline 小於週期

[35:31]–[38:32]

dᵢ < pᵢ 時，條件改成用 min(pᵢ, dᵢ)：

> **Σ eᵢ / min(pᵢ, dᵢ) ≤ 1**（課本式 2.9）

這變成**只是充分條件**，不是必要條件。

反例【口頭】：兩個任務，p = 10、e = 1；d₁ = 1、d₂ = 10。

- Σ eᵢ/min(pᵢ, dᵢ) = 1/1 + 1/10 = **1.1 > 1**。
- 但先跑 T1（0–1），再跑 T2（1–2），兩個都趕上 → **排得出來**。

⚠️ 課本 p.48 的文字是：「若 pᵢ > dᵢ，用式 2.9；但若 **pᵢ < dᵢ**，即使不滿足 2.9 也可能排得出來，所以 2.9 在 **pᵢ < dᵢ** 時是保守的、只是充分條件。」課堂反例是 **d < p**，和課本寫的方向相反。照數學看：d < p 時用 min = d 會高估需求，條件變保守，課堂的說法正確；課本的 pᵢ < dᵢ 應是筆誤。

### 3.5 【實測】EDF 模擬器

寫一個以 1 單位為步長的可搶占 EDF 模擬器（相位 0）。核心就一行：

```python
r = min(ready, key=lambda r: (r[0], r[1]))    # earliest absolute deadline
```

結果（兩個任務的列出時間軸，數字 = 任務編號，`.` = 閒置）：

```
    1 2 2 1 2 2
lecture ex (1,3),(4,6)                   sum e/min(p,d) = 1.000  misses = 0 []
book Ex2.6 (10,20),(5,50),(10,35)        sum e/min(p,d) = 0.886  misses = 0 []
    1 1 2 2 2 1 1 2 2 2 1 1 2 2 2 1 1 1 1 2
U=1.1 (2,4),(3,5)                        sum e/min(p,d) = 1.100  misses = 1 [(0, 3, 16)]
    1 2 . . . . . . . .
lecture d<p: (1,10,d=1),(1,10,d=10)      sum e/min(p,d) = 1.100  misses = 0 []
```

- 課堂例子 U = 1：剛好排滿，沒有空檔、沒有錯過。
- 課本 Ex 2.6：U = 0.886（課本寫 0.89），沒有錯過。
- U = 1.1 的 (2,4)、(3,5)：T1 的第 4 個 job（deadline 16）錯過。U > 1 一定排不出來。
- d < p 的反例：Σ = 1.1，但排得出來——證實式 2.9 在 d < p 時不是必要條件。

---

## 4. EDF 是動態優先權嗎？

[15:31]–[16:00]、[38:32]

- 老師：EDF 是動態優先權演算法。deadline 最近的優先權最高，而這會隨時間改變。
- 可以想像每個任務有一個**虛擬優先權**，越接近 deadline 越高，最後被拿去排。

【課本補充】（p.49 §7.1）課本進一步說：嚴格來說 EDF **沒有**「優先權值」這個概念。排程只看 deadline 遠近，不需要算任何優先權值。等越久、越可能被選中，所以才「想像」有一個隨時間上升的虛擬優先權。

---

## 5. EDF 的缺點

[38:32]–[40:31]

> EDF 簡單、最佳，但**很少用**。

### 5.1 暫時過載

- **暫時過載（transient overload）**：短時間內任務太多，或某任務跑得比預期久。
- 為什麼會發生：執行時間很難準確預測（像 cyclic 排程器的 overrun）；某次輸入特殊；deadline 可能 99/100 次成立，但偶爾不成立。
- 後果：搶占很頻繁，執行期間的支援變得沒效率。

【課本補充】（p.50）更嚴重的問題：過載時**無法預測哪個任務會錯過**。一個低重要性的任務超時，可能讓緊接在後的任務錯過，甚至讓**最關鍵的任務**錯過。不管設計多小心，EDF 都不能保證過載時最關鍵的任務不錯過。

### 5.2 【實測】過載的骨牌效應

三個任務 (1,4)、(2,6)、(3,12)，U = 0.833，正常情況 48 個單位內沒有錯過。讓 T3 的第一個 job 多跑幾個單位：

```
overload base U = 0.833 misses: []
T3 job0 runs +4 units -> [(2, 0, 12)]
T3 job0 runs +6 units -> 4 misses: [(2, 0, 12), (0, 3, 16), (1, 2, 18), (2, 1, 24)]
T3 job0 runs +10 units -> 14 misses: [(2, 0, 12), (0, 3, 16), (1, 2, 18), (0, 4, 20), ...]
```

（格式：(任務編號從 0 起, 第幾個 job, 絕對 deadline)）

- 多跑 4 單位：只有 T3 自己錯過。
- 多跑 6 單位：T3 錯過 deadline 12 之後，它的 deadline（12）仍然是**最早的**，所以 EDF 繼續跑它，害 T1、T2 跟著錯過——**骨牌效應**。
- 多跑 10 單位：錯過 14 次，三個任務都受害。
- 這正是課本說的：一個超時的任務，會拖垮誰是不可預測的。（之後的 RMA 用固定優先權，超時的低優先權任務不會影響高優先權任務。）

### 5.3 資源共享、搶占

- 任務之間有**依賴**或**共享資源**時，EDF 很難處理（課本：要付出很高的開銷，下一章再談）。
- EDF 假設可以搶占，但**不是所有資源都能搶占**，這會讓事情很沒效率。
- 可排程性條件要一般化時（d ≠ p），只剩充分條件。

---

## 6. 實作：priority queue 與 O(1) 方法

[40:00]–[42:01]

- 任務要按 deadline **排序**維護。
- 理想資料結構：**priority queue**，例如以 deadline 為 key 的 **min-heap**，複雜度 **O(log n)**。
- 更快的方法：用**絕對 deadline** 做雜湊（或 radix tree）。每個 deadline 等級一條 linked list，例如「deadline 很近」的一條、其他的另一條。目標：從 O(log n) 降到接近 **O(1)**。前提是 deadline 的種類**有限**。

【課本補充】（p.49 §7.2）三種實作：

| 做法 | 插入 | 選下一個 |
|---|---|---|
| 未排序佇列 | O(1) | O(n)（掃整個佇列） |
| Heap（priority queue） | O(log n) | 在 heap 頂端 |
| 每種**相對 deadline** 一個 FIFO 佇列（共 Q 個） | O(1) | 只看 Q 個佇列的頭，O(Q) ≈ O(1) |

- 第三種為什麼可行：同一個相對 deadline 的任務，**先到的絕對 deadline 一定比較早**，所以每個佇列自然按絕對 deadline 排好。
- 但課本 p.50 也說：實務上很難把不同 deadline 的種類限制在合理數量，所以常常還是得用 O(log n) 的 priority queue，對週期短、deadline 嚴的即時任務是不小的開銷。

⚠️ 課本 p.49 說任務被拿去排程時「從 priority queue 移除只要 O(1)」。用 heap 的話，**刪除頂端要 O(log n)**（要重新調整）；只有「看頂端」是 O(1)。

### 6.1 【原始碼】Linux SCHED_DEADLINE 就是 EDF

Linux 的 `SCHED_DEADLINE` 用**紅黑樹**按絕對 deadline 排序（priority queue 的一種，O(log n) 插入）。

比較函式——按絕對 deadline：

`kernel/sched/deadline.c:1533`

```c
static inline bool __dl_less(struct rb_node *a, const struct rb_node *b)
{
	return dl_time_before(__node_2_dle(a)->deadline, __node_2_dle(b)->deadline);
}
```

`include/linux/sched/deadline.h:25`

```c
static inline bool dl_time_before(u64 a, u64 b)
{
	return (s64)(a - b) < 0;
}
```

（用有號減法比較，時間計數器繞回時也不會出錯。）

插入就緒佇列：

`kernel/sched/deadline.c:1614`

```c
static void __enqueue_dl_entity(struct sched_dl_entity *dl_se)
{
	struct dl_rq *dl_rq = dl_rq_of_se(dl_se);

	WARN_ON_ONCE(!RB_EMPTY_NODE(&dl_se->rb_node));

	rb_add_cached(&dl_se->rb_node, &dl_rq->root, __dl_less);

	inc_dl_tasks(dl_se, dl_rq);
}
```

選下一個——樹最左邊 = deadline 最早：

`kernel/sched/deadline.c:2005`

```c
static struct sched_dl_entity *pick_next_dl_entity(struct dl_rq *dl_rq)
{
	struct rb_node *left = rb_first_cached(&dl_rq->root);

	if (!left)
		return NULL;

	return __node_2_dle(left);
}
```

新任務到達時要不要搶占——比 deadline：

`kernel/sched/sched.h:264`

```c
dl_entity_preempt(struct sched_dl_entity *a, struct sched_dl_entity *b)
{
	return dl_entity_is_special(a) ||
	       dl_time_before(a->deadline, b->deadline);
}
```

`kernel/sched/deadline.c:1951`

```c
static void check_preempt_curr_dl(struct rq *rq, struct task_struct *p,
				  int flags)
{
	if (dl_entity_preempt(&p->dl, &rq->curr->dl)) {
		resched_curr(rq);
		return;
	}
```

對照：

| 本講 | Linux |
|---|---|
| 按 deadline 排序的 priority queue | 紅黑樹，key = 絕對 deadline |
| 選 deadline 最近的 | `rb_first_cached()`：快取最左節點，O(1) 取得 |
| 插入 O(log n) | `rb_add_cached()`：O(log n) |
| 到達事件時，deadline 更近就搶占 | `check_preempt_curr_dl()` → `resched_curr()` |
| U ≤ 1 才排得出來 | `sched_setattr()` 時做頻寬檢查 `sched_dl_overflow()`（`kernel/sched/core.c:7672` 呼叫）：總頻寬超過上限就拒絕 |
| 過載骨牌效應 | Linux 用 **CBS**（constant bandwidth server）限制：任務用完 `runtime` 就被節流（throttle）到下一週期，超時的任務不能拖垮別人 |

最後一列是 Linux 對 §5.1 缺點的解法：`deadline.c` 註解（約 817 行起）寫明「純 EDF 不處理任務跑得比宣告久的情況」，所以加了 runtime 用完就停的機制。OS 課（`../linux-os/` 第 27 講）有 SCHED_DEADLINE 允入控制的實測。

---

## 7. 變形：Minimum Laxity First

[42:01]–[43:00]

- **Laxity（餘裕）** = 相對 deadline − 完成任務還需要的時間。就是 slack。
- **MLF（Minimum Laxity First）**：餘裕最小的最可能先失敗，給它最高優先權。
- 任務的執行時間和 deadline 都相同時，EDF = MLF。
- 不同設定下 MLF 的最佳性也有人研究，老師不多講。

課本 p.48：MLF 和 EDF 的主要差別，是 MLF 有考慮任務的**執行時間**。

【口頭】[43:00] 老師連到 OS 課：OS 課裡「最好」的演算法是能預測完成時間、選最早完成的那個（SJF/SRTF 一類）。EDF 精神類似，但不一樣；而且證明 EDF 最佳比較 tricky。

結語：單處理器上、pᵢ = dᵢ 時只要 U ≤ 1，這讓 EDF 在**它那一類**（考慮所有前提假設）是最 proficient 的排程器。

---

## 8. 一頁速查表

### 8.1 Event-driven

排程點 = 到達、完成；可搶占；貪婪（有就緒就不閒置）。Round-robin 是 hybrid、忽略 deadline。

### 8.2 Foreground-background

ct_B = e_B / (1 − Σ eᵢ/pᵢ)。有 context switch 開銷時，每個前景實例的 e 加 2 次切換。這是平均值近似，最後一個週期可能多出一些（實測 2092 vs 2083）。

### 8.3 EDF

| 項目 | 內容 |
|---|---|
| 規則 | 跑 deadline 最近的就緒任務，可搶占 |
| 最佳性 | 單處理器、任務獨立時最佳 |
| d = p | Σ eᵢ/pᵢ ≤ 1 ⇔ 可排程（充要） |
| d < p | Σ eᵢ/min(pᵢ, dᵢ) ≤ 1 ⇒ 可排程（只是充分） |
| 證明關鍵 | 找第一次錯過的 t，往回找緊的區間 [t′, t]；需求 ≤ Δ·U ≤ Δ，矛盾 |
| 缺點 | 過載時骨牌效應、不可預測；資源共享難；priority queue 開銷 |
| 實作 | heap O(log n)；有限 deadline 種類時每種一個 FIFO → O(1) |
| 變形 | MLF：laxity = d − 剩餘執行時間 |

### 8.4 Linux

`SCHED_DEADLINE` = EDF ＋ 紅黑樹（key = 絕對 deadline）＋ 頻寬允入控制 ＋ CBS 節流。

---

## 9. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **靜態優先權、RMA** | **第 7 支**：RMA Scheduling |
| **過載時固定優先權比較可預測** | **第 7–8 支**：RMA 與其延伸 |
| **d ≠ p 的情況** | **第 8 支**：DMA（deadline monotonic）等延伸 |
| **資源共享** | **第 10–13 支**：Priority Inversion、PIP / HLP / PCP |
| **多處理器上的 EDF** | **第 15 支**：Scheduling in Multiprocessor Systems |
| **POSIX / Linux 的即時排程** | **第 21 支**、**第 23 支** |

---

## 附：本講一句話總結

> **Event-driven 排程器在任務到達與完成時做決定、可搶占、不讓處理器閒著，比 clock-driven 更 proficient；最簡單的是 foreground-background，背景任務完成時間為 e_B / (1 − Σ eᵢ/pᵢ)（實測證實無開銷時精確，有切換開銷時只是近似）。EDF 每次跑 deadline 最近的就緒任務，是單處理器、獨立任務下的最佳演算法：當 d = p 時，Σ eᵢ/pᵢ ≤ 1 是充要條件——證明是假設 t 時第一次錯過、往回找出沒有閒置且自給自足的區間 [t′, t]，再算出需求 ≤ Δ·U ≤ Δ 與「錯過 deadline」矛盾；d < p 時 Σ eᵢ/min(pᵢ, dᵢ) ≤ 1 只是充分條件（反例：1/1 + 1/10 = 1.1 仍排得出）。EDF 雖然最佳卻很少用，因為暫時過載時會產生不可預測的骨牌效應（實測一個任務多跑 10 單位就造成 14 次錯過）、資源共享難處理、priority queue 有 O(log n) 開銷；Linux 的 SCHED_DEADLINE 就是用紅黑樹實作的 EDF，並加上頻寬允入控制和 CBS 節流來擋住過載。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 6 錄影逐字稿（YouTube 自動字幕，44:12）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch2 §6–7 整理。原始碼為 Linux v6.2.12；實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
