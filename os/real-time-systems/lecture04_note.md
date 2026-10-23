# Real Time Systems 第四講完整筆記：排程的基礎

> **課程**：Real Time Systems — Lecture 4：Basics of Scheduling（播放清單第 4 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：錄影逐字稿（14:46；YouTube 只有印地語自動字幕，改用 whisper medium 轉錄英文音訊）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch2 §1–3.2（PDF p.31–36）
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
- [1. 任務、工作、釋出](#1-任務工作釋出)
- [2. 時間名詞](#2-時間名詞)
  - [2.1 絕對 deadline vs 相對 deadline](#21-絕對-deadline-vs-相對-deadline)
  - [2.2 回應時間](#22-回應時間)
  - [2.3 週期、偶發、非週期與相位](#23-週期偶發非週期與相位)
  - [2.4 火箭軌道修正的例子](#24-火箭軌道修正的例子)
  - [2.5 【原始碼】SCHED_DEADLINE 的 (runtime, deadline, period)](#25-原始碼sched_deadline-的-runtime-deadline-period)
- [3. 排程名詞](#3-排程名詞)
  - [3.1 Valid schedule 與 feasible schedule](#31-valid-schedule-與-feasible-schedule)
  - [3.2 【實測】檢查一個排程是否 valid / feasible](#32-實測檢查一個排程是否-valid--feasible)
  - [3.3 Proficient 與 optimal scheduler](#33-proficient-與-optimal-scheduler)
  - [3.4 排程點：clock-driven vs event-driven](#34-排程點clock-drivenvs-event-driven)
- [4. 【課本補充】其他名詞](#4-課本補充其他名詞)
- [5. 一頁速查表](#5-一頁速查表)
- [6. 與後續講次的連結](#6-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

這一講很短，只做一件事：**定義排程要用的名詞**。

- 任務（task）、工作（job）、釋出（release）
- 相對 / 絕對 deadline、回應時間、相位
- valid / feasible 排程、proficient / optimal 排程器
- clock-driven vs event-driven

---

## 1. 任務、工作、釋出

[00:00]–[04:35]

- 即時任務因事件（內部或外部的 stimulus）而產生。例：溫度感測器讀到過高 → 產生一個任務。
- 任務產生時，說它**被釋出（released）**或**到達（arrived）**。
- **即時任務排程**：決定一群任務**以什麼順序**執行。
  - 這是 RTOS 確保趕上 deadline 的**主要方法**。
  - 所以排程器是 RTOS 最重要的元件。

三個名詞：

| 名詞 | 意思 |
|---|---|
| **Job（工作）** | 一個工作單位：一次計算、一次讀檔、一次傳訊息 |
| **Task（任務）** | 一串相似的 job。週期任務每一期就是一個 job |
| **Task instance（任務實例）** | 就是 job，兩個詞混用 |

- 任務一般會重複很多次，每次由一個事件觸發（週期、偶發或非週期事件）。
- 任務 T 第 i 次出現，記作 **Tᵢ** 或 **T(i)** 。課本記作 **Tᵢ(j)**：任務 Tᵢ 的第 j 個實例。
- 老師一開始提到還有 **job slice**（工作切片），這一講先不講。
- 釋出之後，RTOS 不一定**立刻**執行它，會等一段時間才開始。

---

## 2. 時間名詞

### 2.1 絕對 deadline vs 相對 deadline

[04:35]–[05:01]

| | 從哪裡算 |
|---|---|
| **絕對 deadline（absolute）** | 從 t = 0 算 |
| **相對 deadline（relative）** | 從任務**到達**的時間算 |

- 老師：絕對 deadline **幾乎不用**，只有少數怪例子。
- 一般說的 **d** 指的是**相對 deadline**。

【課本補充】（p.32 Fig.1）任務 Tᵢ 第一個實例在 φ 到達、相對 deadline 是 d，絕對 deadline 就是 **φ + d**。

```
0          φ          φ+d        φ+p
|----------|=Ti(1)=   ↑          |=Ti(2)=   ...
           ↑到達       deadline
|<-- 絕對 deadline = φ+d -->|
           |<- 相對 d ->|
```

### 2.2 回應時間

[05:01]–[06:01]

- **回應時間（response time）**：從 job **到達**到 job **完成**（產生結果）的時間。
- Soft 即時任務：要**盡量縮短**回應時間（課本：平均回應時間是衡量 soft 排程器的重要指標）。
- Hard 即時任務：只要在 deadline 前完成就好，提早沒有好處。老師：「那我就很開心了，畫個大笑臉。」

### 2.3 週期、偶發、非週期與相位

[06:01]–[07:36]

| 種類 | 說明 | 課本記法（p.33–34） |
|---|---|---|
| **週期（periodic）** | 每 p 時間重複一次。絕大多數即時任務是這種 | Tᵢ = (φᵢ, pᵢ, eᵢ, dᵢ) |
| **偶發（sporadic）** | 隨機出現，但兩個實例之間有**最小間隔** | Tᵢ = (eᵢ, gᵢ, dᵢ)，gᵢ = 最小間隔 |
| **非週期（aperiodic）** | 隨機出現，同一任務兩個實例甚至可以**同時**到達 | — |

- **相位（phase, φ）**：從 t = 0 到第一個實例到達的時間。
- 週期任務第 k 個實例在 φ + (k−1)·p 到達。

【課本補充】記法細節（p.33）：

- e = **最壞情況執行時間**（worst case execution time, WCET）。
- 若 d = p，可省略 d：Tᵢ = (2000 ms, 50 ms, 8 ms)。
- 若 φ = 0，也可省略：Tᵢ = (20 ms, 100 ms) 表示 φ=0、e=20 ms、p=d=100 ms。⚠️ 注意這時第一個數字是 **e**，第二個是 **p**，和四元組的順序不同，容易看錯。

【課本補充】三種任務的特性（p.34–35）：

- 週期任務的到達通常由**時鐘中斷**標記，所以也叫 clock-driven task。也可以動態產生：例如雷達偵測到飛機後開始的週期追蹤，飛出範圍就結束。
- 偶發任務常是**高關鍵性**的：機器人突然遇到障礙、工廠的火警。也有中等關鍵性的：I/O 或 DMA 中斷。
- 非週期任務一般是 **soft** 即時：可能一下子擠在一起來，很難全部趕上；deadline 通常以平均值或統計方式表示。例：分散式系統的 logging、鍵盤、滑鼠、所有使用者互動指令。

### 2.4 火箭軌道修正的例子

[07:02]–[08:00]

火箭發射 **2000 ms** 後，開始執行軌道修正任務，之後每 **50 ms** 一次；每次需要 **8 ms** 處理時間，相對 deadline **50 ms**。

| 參數 | 值 |
|---|---|
| φ（相位） | 2000 ms |
| p（週期） | 50 ms |
| e（執行時間） | 8 ms |
| d（相對 deadline） | 50 ms |

課本記成 (2000 ms, 50 ms, 8 ms, 50 ms)。

老師的說法：50 ms 裡，任意挑 8 ms 把它做完就行。

### 2.5 【原始碼】SCHED_DEADLINE 的 (runtime, deadline, period)

Linux 的 `SCHED_DEADLINE` 排程類別，直接用本講的任務模型描述一個任務。使用者用 `sched_setattr()` 傳入：

`include/uapi/linux/sched/types.h:102`

```c
struct sched_attr {
	__u32 size;

	__u32 sched_policy;
	__u64 sched_flags;

	/* SCHED_NORMAL, SCHED_BATCH */
	__s32 sched_nice;

	/* SCHED_FIFO, SCHED_RR */
	__u32 sched_priority;

	/* SCHED_DEADLINE */
	__u64 sched_runtime;
	__u64 sched_deadline;
	__u64 sched_period;
```

kernel 內部存成：

`include/linux/sched.h:601`

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

	/*
	 * Actual scheduling parameters. Initialized with the values above,
	 * they are continuously updated during task execution. Note that
	 * the remaining runtime could be < 0 in case we are in overrun.
	 */
	s64				runtime;	/* Remaining runtime for this instance	*/
	u64				deadline;	/* Absolute deadline for this instance	*/
```

新的實例開始時，絕對 deadline = 現在時間 + 相對 deadline：

`kernel/sched/deadline.c:772`

```c
static inline void replenish_dl_new_period(struct sched_dl_entity *dl_se,
					    struct rq *rq)
{
	/* for non-boosted task, pi_of(dl_se) == dl_se */
	dl_se->deadline = rq_clock(rq) + pi_of(dl_se)->dl_deadline;
	dl_se->runtime = pi_of(dl_se)->dl_runtime;
}
```

對照：

| 本講 | Linux |
|---|---|
| e（每個實例的執行時間） | `dl_runtime`（每個實例**最多**能跑多久） |
| d（相對 deadline） | `dl_deadline` |
| p（週期） | `dl_period`（註解寫的是「兩個實例的間隔」，所以也能描述偶發任務的最小間隔 g） |
| 絕對 deadline = 到達時間 + d | `deadline = rq_clock(rq) + dl_deadline` |
| 使用率 e/p（§4） | `dl_bw = dl_runtime / dl_period` |
| task instance / job | 註解裡的 "instance" |

注意：老師說絕對 deadline「幾乎不用」，是指**描述任務**時不用。排程器**執行時**反而需要每個 job 的絕對 deadline 才能比較誰先到期（EDF，第 6 支）。

---

## 3. 排程名詞

### 3.1 Valid schedule 與 feasible schedule

[08:00]–[10:06]

**Valid schedule（有效排程）**——全是常識：

1. 一個處理器同一時間**最多跑一個**任務。
2. 任務**到達之前**不能被排上去。
3. 任務之間的**先後關係（precedence）**和**資源限制**都要滿足：資源沒空，任務就不能跑。

**Feasible schedule（可行排程）** = valid ＋ **所有任務都趕上 deadline**。

> feasible = valid + 符合所有 deadline

### 3.2 【實測】檢查一個排程是否 valid / feasible

寫一個 Python 小程式：給三個週期任務，手動排幾種排程，檢查 valid 與 feasible。

任務（φ, p, e, d）：T1 = (0, 4, 1, 4)、T2 = (0, 6, 2, 6)、T3 = (1, 12, 3, 8)。看 [0, 12)。使用率 U = 1/4 + 2/6 + 3/12 = 0.833。

```python
def check(sched):
    """sched: list of (start, end, job) on one CPU"""
    ...
    for (a0, a1, x), (b0, b1, y) in zip(s, s[1:]):
        if b0 < a1: print(f"  invalid: {x} and {y} overlap"); ok_valid = False
    for a, b, x in s:
        r, e, D = J[x]
        if a < r: print(f"  invalid: {x} starts at {a} before release {r}"); ok_valid = False
```

結果（節錄）：

```
schedule A:
  T2(1)  release= 0 finish= 3 response= 3 abs_deadline= 6 OK
  T3(1)  release= 1 finish= 6 response= 5 abs_deadline= 9 OK
  => valid / feasible
schedule B (T3 first):
  T2(1)  release= 0 finish= 6 response= 6 abs_deadline= 6 OK
  T3(1)  release= 1 finish= 4 response= 3 abs_deadline= 9 OK
  => valid / feasible
schedule C (T1(2) before release):
  invalid: T1(2) starts at 1 before release 4
  => NOT valid / NOT feasible
schedule D (T1(2) before T2(1)):
  T2(1)  release= 0 finish= 7 response= 7 abs_deadline= 6 MISS
  => valid / NOT feasible
```

看到什麼：

- A 和 B 都可行，但回應時間不同：B 讓 T2(1) 剛好在 deadline 6 完成。對 hard 任務來說，兩個一樣好（提早沒有獎勵）；對 soft 任務，A 的平均回應時間比較短。
- C 在 T1(2) 到達前就排它 → 違反常識規則 2 → 不是 valid。
- D 每條規則都守，但 T2(1) 晚 1 個單位 → valid 但**不** feasible。

### 3.3 Proficient 與 optimal scheduler

[10:06]–[12:02]

- **S1 至少和 S2 一樣 proficient（能幹）**：S2 能排出可行排程的任務集合，S1 也都能。
  - 若 S1 還能排一些 S2 排不出的 → S1 **比較** proficient。
- 兩個方向都成立 → **equally proficient（一樣能幹）**：沒有任何任務集合是一個排得出、另一個排不出。
- **Optimal scheduler（最佳排程器）**：任何排程器能可行排程的任務集合，它都能。世界上沒有比它更能幹的。
  - 課本 p.35：若最佳排程器排不出某個任務集合，那就沒有任何排程器排得出。
- 目標：設計最佳排程器。

### 3.4 排程點：clock-driven vs event-driven

[12:02]–[14:46]

**排程點（scheduling point）**：時間軸上，排程器決定「下一個跑誰」的時刻。

| | 排程點由誰決定 | 優缺點（老師） |
|---|---|---|
| **Clock-driven** | 週期性計時器 | 定期更新排程，但可能做白工 |
| **Event-driven** | 事件：任務到達、任務完成 | 比較有效率：只在需要時才算 |

- 即時排程是所有即時系統的核心。單處理器、多處理器、分散式系統都有大量研究。
- 單處理器的研究很早，**1970、80 年代**就開始了。（課本 p.35：重要結果大多在 1970 年代初期就做出來了。）

[13:37]–[14:46] 總結：

- Clock-driven：基本上是一個**無窮迴圈**，時間驅動、循環式（cyclic）。
- Event-driven：不是這樣。⚠️ 逐字稿此處為 "there are no tasks that is pulled"，語意不清。
- 其他屬性：
  - **多速率（multi-rate）**：任務週期不同，可以快任務一個排程器、慢任務一個，但要處理資源競爭。
  - 可以考慮**優先權**。
  - 可以考慮**裝置中斷**：中斷來時做一次排程。

【課本補充】分類（p.36）：

| 類別 | 排程點 | 例子 |
|---|---|---|
| Clock-driven | 時鐘中斷 | table-driven、cyclic |
| Event-driven | 事件 | 簡單優先權、RMA、EDF |
| Hybrid | 兩者都用 | round-robin |

Clock-driven 簡單又有效率，所以嵌入式應用很常用。

---

## 4. 【課本補充】其他名詞

（p.32–36，課堂沒講）

| 名詞 | 意思 |
|---|---|
| **Task precedence（先後關係）** | Tᵢ 要在 Tⱼ 開始前完成；那麼每個 Tᵢ(k) 都先於 Tⱼ(k)。這是**偏序（partial order）** |
| **Data sharing（資料共享）** | 任務之間傳結果。不一定有先後：兩個任務可以同時跑，中途傳資料。課本用**虛線箭頭**表示，和先後關係（實線）分開 |
| **Preemptive scheduler（可搶占排程器）** | 高優先權任務到達時，暫停正在跑的低優先權任務。不會有「高優先權在等、低優先權在跑」的情況 |
| **Utilization（使用率）** | uᵢ = eᵢ / pᵢ；總和 U = Σ eᵢ/pᵢ。單處理器上 U > 1 不可能排得出來。好的排程演算法要能排 U 接近 1 的任務集合 |
| **Jitter（抖動）** | 週期任務偏離嚴格週期的程度。到達抖動（時鐘不準、網路壅塞）；完成抖動（排程演算法造成） |

先後關係例（p.33 Fig.2）：T1 → T2、T4 → T3（實線，先後）；T3 ⇢ T2（虛線，資料共享）。T1 和 T3、T4 之間沒有關係。

---

## 5. 一頁速查表

### 5.1 名詞

| 名詞 | 一句話 |
|---|---|
| Job / task instance | 任務的一次執行 |
| Task | 一串 job |
| Release / arrival | job 產生的時刻 |
| 相對 deadline d | 從到達算起的期限（一般都用這個） |
| 絕對 deadline | 從 0 算起 = 到達時間 + d |
| 回應時間 | 到達 → 完成 |
| 相位 φ | 0 → 第一個實例到達 |
| WCET e | 最壞情況執行時間 |

### 5.2 任務記法

- 週期：(φ, p, e, d)。火箭例：(2000, 50, 8, 50) ms。
- 偶發：(e, g, d)，g = 最小間隔。
- 非週期：最小間隔 0，通常是 soft。

### 5.3 排程

- Valid：一次一個、不早於到達、守先後與資源。
- Feasible：valid ＋ 全部趕上 deadline。
- S1 ≥ S2（proficient）：S2 排得出的，S1 都排得出。
- Optimal：沒有比它更能幹的。
- Clock-driven（計時器決定排程點）vs event-driven（到達、完成決定排程點）。

### 5.4 Linux

`SCHED_DEADLINE`：`sched_runtime` = e、`sched_deadline` = d、`sched_period` = p；kernel 每個新實例算 `deadline = now + dl_deadline`（絕對 deadline）。

---

## 6. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **Clock-driven 排程（table-driven、cyclic）** | **第 5 支**：Clock-driven Scheduling |
| **Event-driven、絕對 deadline、optimal** | **第 6 支**：EDF（單處理器上的最佳排程器之一） |
| **優先權、使用率 U** | **第 7–8 支**：RMA 與其延伸 |
| **Jitter** | **第 9 支**：Jitter and Limited Priority Levels |
| **資源限制（valid 的第 3 條）** | **第 10–13 支**：Priority Inversion、PIP / HLP / PCP |
| **多處理器排程** | **第 15 支**：Scheduling in Multiprocessor Systems |

---

## 附：本講一句話總結

> **即時排程就是決定任務的執行順序，是 RTOS 趕上 deadline 的主要手段；一個 task 是一串 job（task instance），每個 job 在 release 時到達，週期任務可記成 (φ, p, e, d)（例：火箭軌道修正 (2000, 50, 8, 50) ms），偶發任務記成 (e, g, d)，非週期任務最小間隔為 0；deadline 一般指從到達算起的相對 deadline，回應時間是到達到完成，hard 任務只要不超過 deadline、soft 任務才要越短越好。排程要先 valid（一次一個、不早於到達、守先後與資源），再加上全部趕上 deadline 才是 feasible；排程器之間以「能可行排程的任務集合」比 proficient，沒有更能幹的就是 optimal；排程點可以由週期計時器決定（clock-driven）或由到達、完成等事件決定（event-driven）。Linux 的 SCHED_DEADLINE 正是用 (runtime, deadline, period) 描述任務，並在每個新實例算出 now + d 作為絕對 deadline。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 4 錄影（14:46，whisper medium 轉錄）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch2 §1–3.2 整理。原始碼為 Linux v6.2.12；實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
