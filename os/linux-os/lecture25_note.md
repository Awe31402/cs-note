# Operating Systems: A Linux Kernel-Oriented Approach 第二十五講完整筆記：多處理器排程與 Banker's Algorithm（Chapter 5, Part VIII）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 25（Chapter 5, Part VIII：Multiprocessor scheduling）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（39:53，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§5.4.3–5.4.4（p.236–244）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
> **標記說明**：
> - `【口頭】`：教授在課堂上口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與教授回答
> - `【課本補充】`：課本有、但這堂課沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（`檔名:行號`），用來對照觀念
> - `【實測】`：在本機（Ubuntu、kernel 6.5.0、x86-64、20 個 CPU）實際執行的結果（本講多為 Python 暴力驗證）
> - `⚠️ 逐字稿缺字`：自動字幕在這裡掉字或聽不清，內容依上下文或課本補上；也用來標示課堂或課本說法需要修正之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 這堂課要做什麼](#0-這堂課要做什麼)
- [1. 多核心排程的架構](#1-多核心排程的架構)
  - [1.1 Dispatcher 與每顆 CPU 的 run queue](#11-dispatcher-與每顆-cpu-的-run-queue)
  - [1.2 【原始碼】`struct rq`：每顆 CPU 一個](#12-原始碼struct-rq每顆-cpu-一個)
- [2. 目標：makespan C_max](#2-目標makespan-c_max)
- [3. 可搶占：很簡單](#3-可搶占很簡單)
- [4. 不可搶占：NP-complete](#4-不可搶占np-complete)
  - [4.1 Bin packing](#41-bin-packing)
  - [4.2 歸約（reduction）的邏輯](#42-歸約reduction的邏輯)
  - [4.3 Partition / subset sum](#43-partition--subset-sum)
- [5. List scheduling](#5-list-scheduling)
  - [5.1 規則：不准故意閒著](#51-規則不准故意閒著)
  - [5.2 各種優先權](#52-各種優先權)
  - [5.3 定理：不會差超過 2 − 1/m 倍](#53-定理不會差超過-2--1m-倍)
  - [5.4 LPT：4/3 − 1/(3m)](#54-lpt43--13m)
  - [5.5 【實測】驗證界限](#55-實測驗證界限)
- [6. CPU affinity](#6-cpu-affinity)
- [7. 多份資源：Banker's algorithm](#7-多份資源bankers-algorithm)
  - [7.1 有環不一定 deadlock](#71-有環不一定-deadlock)
  - [7.2 資料結構](#72-資料結構)
  - [7.3 安全狀態的檢查](#73-安全狀態的檢查)
  - [7.4 要求資源](#74-要求資源)
  - [7.5 偵測 deadlock](#75-偵測-deadlock)
  - [7.6 發現 deadlock 之後](#76-發現-deadlock-之後)
  - [7.7 【實測】跑課本的例子](#77-實測跑課本的例子)
- [8. 預告：Linux 的排程](#8-預告linux-的排程)
- [9. 一頁速查表](#9-一頁速查表)
- [10. 與後續講次的連結](#10-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

[00:33] 兩件事：

1. **多核心（多處理器）排程**：目標換成 makespan；不可搶占時是 NP-complete；用 list scheduling 得到不超過 2 倍的近似解。
2. **多資源排程**：每種資源有好幾份時，怎麼避免 deadlock：**Banker's algorithm**。

---

## 1. 多核心排程的架構

### 1.1 Dispatcher 與每顆 CPU 的 run queue

[00:33] 基本想法（課本 Figure 5.28）：

```
          新 job ──▶ ┌──────────────┐
                     │  Dispatcher  │   一條全域的 queue
                     └──────┬───────┘
              ┌─────────────┼─────────────┐
              ▼             ▼             ▼
          [run queue]   [run queue]   [run queue]
            CPU 1         CPU 2         CPU 3
          （各自的排程器，跑上一講的某個演算法）
```

- 所有 job 先進一條 **dispatcher queue**，再分到各 CPU 自己的 **run queue**。
- 【口頭】[00:33] 教授說：run queue 傳統上寫成一個字 "runqueue"，中間不空格。
- [01:01] 每顆 CPU 有自己的排程器，跑上一講的某個演算法。

【課本補充】課本：全域 queue 通常放**新建立**的 task 或**要遷移**的 task。理論上不同 CPU 可以用不同演算法，但實務上很少這樣。

### 1.2 【原始碼】`struct rq`：每顆 CPU 一個

`kernel/sched/sched.h:954-992`（節錄）

```c
struct rq {
	/* runqueue lock: */
	raw_spinlock_t		__lock;

	/*
	 * nr_running and cpu_load should be in the same cacheline because
	 * remote CPUs use both these fields when doing load calculation.
	 */
	unsigned int		nr_running;
	...
	u64			nr_switches;
	...
	struct cfs_rq		cfs;
	struct rt_rq		rt;
	struct dl_rq		dl;
```

`kernel/sched/sched.h:1195-1198`

```c
DECLARE_PER_CPU_SHARED_ALIGNED(struct rq, runqueues);
...
#define cpu_rq(cpu)		(&per_cpu(runqueues, (cpu)))
#define this_rq()		this_cpu_ptr(&runqueues)
```

| 部分 | 說明 |
|---|---|
| `DECLARE_PER_CPU_SHARED_ALIGNED` | **每顆 CPU 一個** `struct rq`（第 22 講的 per-CPU 變數），而且對齊到 cache line，避免兩顆 CPU 的 rq 擠在同一條 line |
| `__lock` | 每個 run queue 一把 spinlock。CPU 大多只鎖自己的 rq，不用搶全域鎖 |
| `nr_running` | 這個 rq 上有幾個可執行的 task。註解說：**別的 CPU 算負載時會讀它**（負載平衡） |
| `cfs`, `rt`, `dl` | 一個 rq 裡面又分三種子 queue，對應上一講的排程類別（fair、rt、deadline）。`rt` 就是上一講那 100 條 queue + bitmap |
| `cpu_rq(cpu)` / `this_rq()` | 拿某顆 / 這顆 CPU 的 rq |

⚠️ Linux 沒有一條明確叫「dispatcher queue」的全域 queue。新 task 在 fork / 被叫醒時，由 `select_task_rq()`（§6）**直接挑一顆 CPU** 放進它的 rq；之後再由定期的**負載平衡**在 CPU 之間搬 task。效果等同課本的 dispatcher，但沒有中央的 queue，避免一把全域鎖成為瓶頸。

---

## 2. 目標：makespan C_max

[01:01] 多核心排程要重新定義目標。看的是 **C_max**。

[01:30] m 顆處理器，橫軸是時間：

```
CPU 1  [  J1  ][ J4 ][   J7   ]
CPU 2  [ J2 ][   J5   ][ J8 ]
CPU 3  [   J3   ][ J6 ]               ← 最後一個做完的時間 = C_max
       0 ───────────────────────────▶ 時間
```

[02:03] 一堆 job（例如一堆 pthread）最後**重要的是最大完成時間 C_max**：到那時其他核心早就做完了，只要找出最慢的那個「落後者」。

[02:03] 這叫 **makespan**。教授說如果把計算畫成圖，它就是**最長的路徑**。

---

## 3. 可搶占：很簡單

[02:30] 多處理器上也一樣：**可搶占的比不可搶占的容易得多**，不可搶占的大多是 NP-complete。

[02:30] `P | pmtn | C_max`（P = 多顆處理器、可搶占、最小化 makespan）有很簡單的演算法：

[03:00] 讓每顆 CPU 做**一樣多**的工作。搶占讓你可以把 job 切成任意薄的片：

```
C_max = (所有 job 長度總和) / m
```

每顆 CPU 分配這麼多，不夠就切開、搬到另一顆。[03:00] 假設遷移延遲、context switch 延遲都是 0，這就是最佳解。

⚠️ 還要加一個條件：一個 job **不能同時在兩顆 CPU 上跑**。如果有某個 job 比平均還長，它自己就要那麼久。正確的最佳值是

```
C_max = max( Σp_i / m ,  max p_i )
```

（McNaughton 的 wrap-around 演算法可以達到這個值。）課本寫成 `C_max = ΣC_i / n`，符號也有誤，應是 `Σp_i / m`。

【實測】p = [7, 5, 4, 3, 2, 2, 1]、m = 3：

```
P|pmtn|Cmax: sum/m = 8.0  max p = 7  -> Cmax = 8.0
```

這組平均 8 比最長的 7 大，所以答案就是 8。

---

## 4. 不可搶占：NP-complete

[03:30] 拿掉搶占：`P || C_max` 是 **NP-complete**。教授：「拿掉搶占，地獄之門就打開了。」

[03:30] 多處理器排程證明 NP-complete 時，最常用的兩個問題：**bin packing** 和 **partition**。

### 4.1 Bin packing

[04:01] **Bin packing（裝箱問題）**：

| 裝箱 | 排程 |
|---|---|
| 有限個箱子 | m 顆核心 |
| 每個箱子容量 λ | makespan 的上限 λ（C_max 門檻） |
| n 個物品，第 i 個大小 s_i | n 個 job，第 i 個長度 p_i |
| 物品不能切開 | **不能搶占**（切開 = 一部分放這顆 CPU、一部分放那顆） |
| 全部裝進去、不超過容量 | 全部排完、不超過 λ |

[05:02] 這是 NP-complete 問題的**決策版本**：「裝得下嗎？」是或不是。

[05:02] 回顧 NP：給你一個解，可以在**多項式時間內驗證**它是不是解；但要**找到**解，不知道多項式時間的演算法。

[05:31] 所以「makespan 為 λ 的多核心排程」和裝箱是**等價的問題**。

【課本補充】課本 Point 5.4.1：裝箱問題的最佳化版本是「用最少的箱子裝完所有物品」。

### 4.2 歸約（reduction）的邏輯

[06:00] 證明 NP-complete 的標準方法：

1. 拿一個**已知** NP-complete 的問題（這裡是 bin packing）。
2. 把它的**每個實例**轉成一個多核心排程問題的實例：箱子容量 = λ，每個物品 = 一個 job，大小 = 執行時間。決策問題是「C_max = λ 排得出來嗎？」
3. [07:01] 如果排程問題有多項式時間演算法 ⟹ bin packing 也有。

[07:31] 用**逆否命題**：A ⟹ B 等於 ¬B ⟹ ¬A。bin packing 沒有已知的多項式時間演算法 ⟹ 排程問題也不會有。

[08:00] 所以：**不可搶占、以 C_max 為目標的多核心排程是 NP-complete**。

【課本補充】課本說得更精確：把 bin packing 歸約到排程問題，表示排程問題**至少跟 bin packing 一樣難**。NP-complete 是 NP 裡**最難**的問題：只要其中一個有多項式時間解，NP 裡所有問題都有。

### 4.3 Partition / subset sum

[08:00] 另一個問題是 **partition**（也可以從 bin packing 歸約過來證明它是 NP-complete）。

[08:33] 例子：S = {2, 4, 5, 7, 9, 12, 13}，找一個子集合，總和 = 24。例如 5 + 7 + 12 = 24。

[09:00] 也可以要求「分成總和相等的兩半」（目標 = 總和 / 2），但這裡講的是更一般的版本，也叫 **subset sum（子集合和）**問題。7 個數很容易；[09:00] 100 萬個數就是 NP-complete 的難題。

[09:32] 跟排程的關係：例如想讓某顆 CPU 的工作總和**剛好**是 302，就要找一組 job 的執行時間加起來是 302，這就是 partition 的一個實例。

[10:00] 總結：在任何**不可搶占**的情況下，要讓一顆 CPU 做**固定量**的工作，計算上就很難，因為你會碰到 bin packing 或 partition。

【實測】列出所有總和 = 24 的子集合：

```
subsets of [2, 4, 5, 7, 9, 12, 13] summing to 24: [(2, 9, 13), (4, 7, 13), (5, 7, 12), (2, 4, 5, 13)]
```

7 個數只有 2⁷ = 128 個子集合，暴力就好；n 個數有 2ⁿ 個。

---

## 5. List scheduling

### 5.1 規則：不准故意閒著

[10:30] 多處理器排程的**啟發式**：**list scheduling**。有很多種，但共同點是：

[11:01]

1. 維護一個**可執行 job 的串列**，依某種優先權排序（哪種都可以）。
2. CPU 空了，就從串列裡找一個 job 來做（最好是優先權最高的）。
3. [11:31] **關鍵限制：不准閒著（no idling）**。有 job 可做，CPU 不能選擇什麼都不做。
4. [12:02] 所有機器都**一樣快**；**不可搶占**（就是上面 NP-complete 的那個版本）。

【課本補充】課本：如果最高優先權的 job 不能執行，CPU 就往下找別的。唯一的條件是串列不空時不能空手而回。

### 5.2 各種優先權

[12:02] 可以設計出很多種 list scheduling：

| 優先權 | 理由 |
|---|---|
| 執行時間由長到短 | 最長的先做（LPT，§5.4） |
| [12:32] **關鍵路徑（critical path）**上的先做 | job 之間有依賴時畫成圖，最長的路徑叫關鍵路徑，它決定了 makespan |
| [12:32] **出度（out-degree）**大的先做 | 做完它，最多後續 job 就能開始（少一個依賴） |

【課本補充】課本：資源足夠時，關鍵路徑通常決定整體 makespan，所以幾乎所有排程問題都很重視它。也可以照到達時間排。

### 5.3 定理：不會差超過 2 − 1/m 倍

[13:01] 這些問題都是 NP-complete，但可以證明 list scheduling **接近最佳**。

[13:30] 設 n 個 job、m 顆 CPU，問題是 `P || C_max`，C* 是最佳 makespan：

> **定理（Graham, 1969；課本 Theorem 5.4.1）**：不管用什麼優先權，只要不故意閒著，
> ```
> C_max / C* ≤ 2 − 1/m
> ```

**教授的證明（直覺版）**：

[15:01] 理想情況：所有 CPU 剛好同時做完，長度 L。這時 C* = L，總工作量 = mL。

[16:01] 最壞情況：大家都做到一樣高，**只有一顆**多出一個 job：

```
CPU 1  [■■■■■■■■■]
CPU 2  [■■■■■■■■■]
 ...
CPU m  [■■■■■■■■■][     最後那個 job     ]
       0 ───── D ─┘└──────── ≤ L ─────────┘
```

- [17:30] 最後那個 job 最長是多少？最多 L（= C*），因為它總要在某顆 CPU 上整個跑完。
- [18:01] 扣掉它，剩下的工作是 mL − L，平均分給 m 顆：D = (mL − L)/m = L − L/m。
- [18:32] C_max = D + L = 2L − L/m = L(2 − 1/m)。
- [19:03] 所以 C_max / C* ≤ 2 − 1/m。

**課本的證明（嚴謹版）**：

設最後一個做完的是 job k，它在時間 t 開始，C_max = t + p_k。

1. 因為**不准閒著**，在 t 之前**所有 CPU 都 100% 在忙**（不然 job k 早就開始了）。
2. t 之前做的工作都來自 k 以外的 job：

```
Σ_{i≠k} p_i ≥ m·t
⟹ t ≤ (1/m)·Σ p_i − p_k/m
⟹ C_max = t + p_k ≤ (1/m)·Σ p_i + p_k·(1 − 1/m)
```

3. 最佳解有兩個下界：**C* ≥ p_k**（job 不能切）和 **C* ≥ (1/m)·Σ p_i**（平均工作量）。代入：

```
C_max ≤ C* + C*·(1 − 1/m)
⟹ C_max / C* ≤ 2 − 1/m
```

[20:02] 結論：最佳解是 NP-complete 才能找到的，但隨便一種 list scheduling 都保證在最佳的 **2 − 1/m 倍**以內，大約 2 倍。這個界限**跟 job 數量、優先權怎麼排都無關**。

【課本補充】課本：Graham 最早的論文掀起了一波研究熱潮，大家開始研究各種限制、優先權、目標函數的組合，到現在已經有非常豐富的結果。

### 5.4 LPT：4/3 − 1/(3m)

[14:01] 如果照**執行時間由長到短**排（最長的先做），叫 **LPT（Longest Processing Time first）**：

```
C_max / C* ≤ 4/3 − 1/(3m)
```

[20:02] 把執行時間考慮進去，可以做得更好，在 4/3 倍以內。證明在課本的參考文獻裡。

直覺：長 job 先放，最後剩下的都是短 job，最後那個「多出來的 job」很短，不平衡就小。

### 5.5 【實測】驗證界限

`sched2.py`：list scheduling = 照串列順序，把下一個 job 給**目前負載最小**的 CPU（就是最早空出來的 CPU）。跟暴力列舉所有分配的最佳解比，每組 7 個 job、長度 1–20，各 300 組：

```
m=2: worst list/OPT=1.261 (bound 1.500)   worst LPT/OPT=1.093 (bound 1.167)
m=3: worst list/OPT=1.440 (bound 1.667)   worst LPT/OPT=1.171 (bound 1.222)
```

隨機的例子都在界限內，而且 LPT 明顯比較接近最佳。

**界限是緊的**：m(m−1) 個長度 1 的 job，最後放一個長度 m 的 job：

```
tight m=2: list=3  OPT=2  ratio=1.500 = 2-1/m=1.500
tight m=3: list=5  OPT=3  ratio=1.667 = 2-1/m=1.667
tight m=4: list=7  OPT=4  ratio=1.750 = 2-1/m=1.750
tight m=5: list=9  OPT=5  ratio=1.800 = 2-1/m=1.800
```

- list scheduling 先把短 job 平均分完（每顆 m − 1），最後那個長 job 只能接在某顆後面：(m − 1) + m = 2m − 1。
- 最佳解：一顆 CPU 只跑長 job（m），短 job 分給其他 m − 1 顆（每顆 m）：C* = m。
- 比值 (2m − 1)/m = 2 − 1/m，**剛好等於**界限。這正是教授畫的「最壞情況」。
- LPT 會先放長 job，就不會掉進這個陷阱。

---

## 6. CPU affinity

[20:32] 排程問題可以很複雜：到達時間不同、搶占、截止時間、優先權、依賴……多核心還有 **CPU affinity（CPU 親和性）**：

[20:32] 指定「第 i 顆核心只跑這些 thread」或「這個 thread 只在第 i 顆核心跑」。

[21:02] Linux 有 set affinity 的函式（`sched_setaffinity`），告訴排程器「這個 thread 偏好這顆核心」。

[21:02] 目標函數回顧：平均完成時間、L_max、C_max（makespan）。多核心排程三個都看過了。

【原始碼】每個 task 帶著自己允許的 CPU 集合：

`include/linux/sched.h:828-831`

```c
	int				nr_cpus_allowed;
	const cpumask_t			*cpus_ptr;
	cpumask_t			*user_cpus_ptr;
	cpumask_t			cpus_mask;
```

挑 CPU 時檢查：

`kernel/sched/core.c:3549-3571`（節錄）

```c
int select_task_rq(struct task_struct *p, int cpu, int wake_flags)
{
	lockdep_assert_held(&p->pi_lock);

	if (p->nr_cpus_allowed > 1 && !is_migration_disabled(p))
		cpu = p->sched_class->select_task_rq(p, cpu, wake_flags);
	else
		cpu = cpumask_any(p->cpus_ptr);
	...
	if (unlikely(!is_cpu_allowed(p, cpu)))
		cpu = select_fallback_rq(task_cpu(p), p);

	return cpu;
}
```

| 部分 | 說明 |
|---|---|
| `nr_cpus_allowed > 1` | 可以去好幾顆 CPU → 交給排程類別自己挑（例如 CFS 會挑比較閒的）。這就是 §1 的 dispatcher 角色 |
| 否則 | 只能去一顆 → 直接用那顆 |
| `is_cpu_allowed` | 挑出來的 CPU 一定要在 `cpus_ptr` 裡，不然改挑一顆合法的 |

`kernel/sched/core.c:2279-2283`

```c
static inline bool is_cpu_allowed(struct task_struct *p, int cpu)
{
	/* When not in the task's cpumask, no point in looking further. */
	if (!cpumask_test_cpu(cpu, p->cpus_ptr))
		return false;
```

【實測】

```
$ nproc
20
$ taskset -p $$
pid 219158's current affinity mask: fffff
$ taskset -c 3 sh -c 'grep Cpus_allowed_list /proc/self/status'
Cpus_allowed_list:	3
```

- 預設 mask 是 `fffff`：20 個位元全是 1，20 顆 CPU 都能跑。
- `taskset -c 3` 用 `sched_setaffinity` 把它限制在 CPU 3。（第 19 講的 store-buffering 測試就是用 `pthread_setaffinity_np` 把兩個 thread 綁在 CPU 0 和 1。）

---

## 7. 多份資源：Banker's algorithm

### 7.1 有環不一定 deadlock

[21:30] 正確性：要保證沒有 deadlock。每種資源**一份**的情況已經處理過了（第 18 講的循環等待、第 22 講的 lockdep）。

[21:30] 但如果每種資源有**好幾份**呢？例如 3 台印表機、4 台掃描器。這難多了。

[21:30] 教授：有多份資源時，**循環等待不再是問題**。（Banker's algorithm 考慮多份資源；傳統的 deadlock 偵測只考慮一份。）

[22:01] 例子（課本 Figure 5.29）：資源 A、B 各 **2 份**。

```
 資源   總數
  A      2
  B      2

  P1：拿著 A×1、B×1
  P2：拿著 B×1，想要 A ──▶ 等 P3（P3 拿著另一個 A）
  P3：拿著 A×1，想要 B ──▶ 等 P2（P2 拿著另一個 B）
```

- [22:31] 四份資源全被拿走了。
- [23:02] P2 等 P3、P3 等 P2：**有環**。
- [23:30] 但**不會 deadlock**：P1 做完會放掉它的 A 和 B，就能分給 P2 和 P3。

[23:30] 教授：這張圖可以有很多種畫法。照現在這樣畫有環；但也可以把 P2 的等待邊畫向 P1（P1 也拿著 A），就沒有環。[24:00] 期待別人「畫成沒有環的樣子」不公平，那本身就不容易。不如設計一個演算法，**不管有沒有環**，直接看能不能分配資源而不 deadlock。

[24:32] 思路是**貪心**：

1. 先滿足**現在就能滿足**的（這裡是 P1）。
2. 它做完會**放掉**資源，可用資源變多。
3. [25:00] 也許就能滿足更多人……一直做下去，直到所有人都被滿足。
4. 做不到，就表示有一組要求不可能同時滿足，狀態不對。

這就是 **Banker's algorithm（銀行家演算法）**。

【課本補充】課本：lockdep 是 Banker's algorithm 的簡化版（每種資源一份）。

### 7.2 資料結構

[25:30] n 個 process，m **種**資源（不是 m 個資源，是 m 種）。課本 Table 5.2：

| 結構 | 大小 | 意思 |
|---|---|---|
| `avlbl[m]` | 向量 | [26:02] 資源 i **現在還有幾份空著** |
| `max[n][m]` | 矩陣 | [26:02] process i **最多**會要求幾份資源 j。**事先宣告** |
| `acq[n][m]` | 矩陣 | [27:00] process i **現在已經拿了**幾份資源 j |
| `need[n][m]` | 矩陣 | [27:00] process i 將來**還可能要**幾份資源 j |

不變式：

```
acq + need = max
```

[27:30] 例子：我最多要 5 份，已經拿了 3 份，所以還需要 2 份。

[28:01] 重點：`avlbl` 是**系統**的（跟 process 無關）；`max`、`acq`、`need` 是**每個 process** 的。系統可能有 20 份某資源，但我的 max 是 5、拿了 2、need 是 3。

### 7.3 安全狀態的檢查

[28:01] **安全狀態（safe state）**：不管誰要什麼，都能分配得出來，不會 deadlock。

課本 Algorithm 1：

```
initialize:
    cur_cnt ← avlbl                 ▷ 目前空著的資源（複製一份）
    ∀i, done[i] ← false             ▷ 還沒有人被滿足

find:
    if ∃i, done[i] == false && need[i] ⪯ cur_cnt then
        go to update
    else
        go to safety check

update:
    ▷ 假設它的要求被滿足、做完了，把它拿著的全部還回來
    cur_cnt ← cur_cnt + acq[i]
    done[i] ← true
    go to find

safety check:
    if ∀i, done[i] == true then return safe
    else return unsafe
```

[29:01]–[31:02] 逐步：

1. `cur_cnt` 是 `avlbl` 的複本：此刻每種資源空著幾份。
2. 每個 process 的 `done` 都是 false。
3. 找一個**還沒做完**、而且**需要的 ⪯ 目前空著的**的 process i。
4. [30:00] 找到了：**假設**滿足它（假想的）。它做完了，就把拿著的 `acq[i]` 全部還回來。[31:02] 標記 done。
5. 回到第 3 步再找。剛剛還回來一些資源，也許原本不行的現在可以了。
6. [31:02] 找不到了：如果**所有人都 done**，就是安全的；否則**不安全**。

[31:30] 教授：這在概念上就是**多資源情況下的 deadlock 偵測**：看是不是能滿足所有人的要求。

`⪯` 的意思：A ⪯ B 表示每一個元素 A[i] ≤ B[i]。

[32:00] **複雜度 O(mn²)**：

- 外層最多做 n 次（每次標記一個 done）。
- 每次要掃過 n 個 process 找一個符合的。
- 每次比較 `need[i] ⪯ cur_cnt` 是長度 m 的向量。

> **課本 Point 5.4.3**：安全狀態的意思是：如果**每個 process 都馬上要求它有權要的最大量**（need），也能找到一個順序全部滿足。這是保守的最壞情況。不安全狀態**不一定**會 deadlock，但要求夠大時就可能。一開始還沒分配任何資源時應該是安全的（合理性檢查）。

### 7.4 要求資源

[32:32] process i 要求一組資源 `req_i`（`req_i[j] = k` 表示要 k 份資源 j）。系統決定給不給，也就是給了會不會變成不安全。

課本 Algorithm 2：

```
check:
    if ¬(req_i ⪯ need[i]) then return false     ▷ 違約：超過事先宣告的量
    else if avlbl ≺ req_i then wait()             ▷ 現在不夠，等

allocate:                                         ▷ 假裝分配
    avlbl  ← avlbl  − req_i
    acq[i] ← acq[i] + req_i
    need[i]← need[i]− req_i
    if state is unsafe then
        disallow the request and undo changes
    else return
```

[33:30] 三個檢查：

1. **`req > need`**：違反**合約**。你說過最多要這麼多，現在超過了，永遠不能給。
2. [34:00] **`req > avlbl`**：資源不夠，等。
3. **否則先假裝分配**：從 `avlbl` 拿出來、加到 `acq`、從 `need` 扣掉（保持 `acq + need = max`）。[34:31] 得到一個新的系統狀態，用 Algorithm 1 檢查**還安不安全**。[35:00] 不安全就**還原**，讓它等。

【課本補充】課本：如果能**事先審核**每一個要求，就永遠不會進入 deadlock 狀態。這是 deadlock **避免**演算法的關鍵。

### 7.5 偵測 deadlock

[35:00] 另一個演算法：**偵測**現在是不是已經 deadlock 了。想法幾乎一樣。

[35:30] 新的矩陣 `reqs[n][m]`：`reqs[i][j]` = process i **現在實際在要求**幾份資源 j。

課本 Algorithm 3：

```
initialize:
    cur_cnt ← avlbl
    for ∀i:
        if acq[i] ≠ 0 then done[i] ← false
        else done[i] ← true          ▷ 什麼都沒拿 → 不可能參與 deadlock

find:
    if ∃i, done[i] == false && reqs[i] ⪯ cur_cnt then go to update
    else go to deadlock check

update:
    cur_cnt ← cur_cnt + acq[i]
    done[i] ← true
    go to find

deadlock check:
    if ∀i, done[i] == true then return No Deadlock
    else return Deadlock
```

[35:30] 教授：什麼都沒拿的 process，`done` 設成 true，「我沒興趣」。（沒有 hold，就沒有 hold-and-wait，不可能是 deadlock 的一員。）拿了東西的才要跑迴圈。

⚠️ 課本文字說「沒拿任何資源的 process，把 `done[i]` 設成 **false**；其他設成 true」，跟它自己的 Algorithm 3 和理由（「它們不可能參與 deadlock」）正好相反。照演算法和教授的說法：**沒拿資源 → true**。

[36:31] 其他一樣：找一個沒做完、**現在的要求** ⪯ 空著的資源的 process，假設滿足它、收回它的資源。最後還有人沒 done，就是 deadlock：有某個 process 的要求**不管怎麼排都滿足不了**。

[37:01] 教授：多資源情況下的 deadlock，就是「大家的要求**聯合起來**無法滿足，不管順序怎麼排」。

> **課本 Point 5.4.4**：Algorithm 3 和 Algorithm 1 幾乎一樣。差別：
> - Algorithm 1（安全檢查）用 **`need`**：假設最壞情況，每個人都要求最大量。
> - Algorithm 3（deadlock 偵測）用 **`reqs`**：實際的、現在的要求。
> - 關係：`reqs ⪯ need`。

### 7.6 發現 deadlock 之後

[37:33] Banker's algorithm 是 **deadlock 避免**演算法：一開始就不讓 deadlock 形成。

[37:33] 每次要求前檢查；發現會往 deadlock 走，可以：

- **延遲**這個要求，
- **取消**這個要求，
- 或**殺掉**一個 process。

[38:02] 重點是**不要進入不安全狀態**。要做避免，就一定要有辦法**拒絕**要求。

[38:02] 另一種做法：**故意**允許進入不安全狀態，然後一直做 deadlock 偵測。

[38:32] 真的偵測到 deadlock，就得殺掉一個 process、釋放它的資源。教授說這是「大鐵鎚」式的做法。Banker's algorithm 的好處是**事先警告**你：這個要求會帶你進入不安全狀態，別去。

【課本補充】課本：要殺的時候，可以挑等很久的、或優先權很低的 process。系統看起來沒有進展時，才跑 deadlock 偵測。

⚠️ Linux kernel **沒有**實作 Banker's algorithm：它要求每個 process **事先宣告** `max`，一般程式做不到。kernel 用的是第 22 講的 lockdep（除錯時抓「可能的」deadlock）和程式設計紀律（固定的拿鎖順序）。

### 7.7 【實測】跑課本的例子

`banker.py` 照 Algorithm 1–3 寫：

**課本 Example 5.4.1**：兩個 process、兩種資源。`avlbl = [1, 1]`，`acq` 全 0，`need = [[1,1],[1,1]]`。

```
P1 asks [1,0]: GRANT order ['P1', 'P2']
P2 asks [0,1]: DENY (unsafe)
P1 asks [0,1]: GRANT
```

- P1 要 [1, 0]：假裝給了之後 `avlbl = [0, 1]`、P1 的 need = [0, 1]。P1 還要的 [0, 1] 剛好空著 → P1 做完還回 [1, 0] → 變成 [1, 1]，P2 的 [1, 1] 也滿足。**安全，給。**
- 接著 P2 要 [0, 1]：給了之後 `avlbl = [0, 0]`，P1 還需要 [0, 1]、P2 還需要 [1, 0]，**誰都滿足不了 → 不安全，拒絕**。跟課本結論一樣。
- 換成 P1 要 [0, 1]：P1 就拿齊了，做完會全部還回來，**安全**。

（課本這個例子在 P1 拿到 [1, 0] 後沒有更新 `need`；照不變式 `acq + need = max`，P1 的 need 應該變成 [0, 1]。上面的程式有更新，結論不變。）

**課本 Figure 5.29**（用 deadlock 偵測，Algorithm 3）：

```
Fig 5.29: ('no deadlock', ['P1', 'P2', 'P3'])
Fig 5.29 without P1 (one copy each): ('DEADLOCK', [])
```

- 有 P1 時：P1 不需要更多資源，先做完、還回 A、B；P2、P3 就都滿足。**有環但沒有 deadlock。**
- 拿掉 P1（等於每種資源只有一份）：P2 拿 B 等 A、P3 拿 A 等 B，誰都動不了 → **deadlock**。一份資源時，環就等於 deadlock。

---

## 8. 預告：Linux 的排程

[39:02] 下一講：**Linux kernel 的排程**。實際用的演算法：**CFS（Completely Fair Scheduler，完全公平排程器）**，再看一點即時排程，然後第 5 章就結束了。

---

## 9. 一頁速查表

### 9.1 多核心排程

| 問題 | 結果 |
|---|---|
| `P \| pmtn \| C_max` | 簡單：C_max = max(Σp_i/m, max p_i) |
| `P ‖ C_max` | NP-complete（bin packing、partition 歸約） |
| list scheduling（不閒著、任何優先權） | C_max/C* ≤ 2 − 1/m（緊的） |
| LPT（長的先） | C_max/C* ≤ 4/3 − 1/(3m) |

證明 2 − 1/m 的兩個下界：C* ≥ max p_i、C* ≥ Σp_i/m。

### 9.2 Banker's algorithm

| 結構 | 意思 |
|---|---|
| `avlbl[m]` | 系統空著的 |
| `max[n][m]` | 事先宣告的最大量 |
| `acq[n][m]` | 已經拿的 |
| `need[n][m]` | 還可能要的；acq + need = max |

| 演算法 | 用什麼比 | 回答 |
|---|---|---|
| 1 安全檢查 | `need`（最壞情況） | safe / unsafe |
| 2 要求資源 | 超過 need → 錯；超過 avlbl → 等；假裝給 → 跑 1 | 給 / 拒絕 |
| 3 deadlock 偵測 | `reqs`（實際要求） | deadlock / 沒有 |

複雜度 O(mn²)。多份資源時有環不一定 deadlock；一份時環 = deadlock。

### 9.3 本講看過的原始碼位置

| 位置 | 內容 |
|---|---|
| `kernel/sched/sched.h:954-992` | `struct rq`：鎖、`nr_running`、cfs/rt/dl 子 queue |
| `kernel/sched/sched.h:1195-1198` | per-CPU `runqueues`、`cpu_rq`、`this_rq` |
| `include/linux/sched.h:828-831` | task 的 `cpus_ptr`、`nr_cpus_allowed` |
| `kernel/sched/core.c:3549-3571` | `select_task_rq`：挑 CPU、檢查 affinity |
| `kernel/sched/core.c:2279-2283` | `is_cpu_allowed` |

---

## 10. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **per-CPU run queue、`cfs_rq`** | **第 26 講**：Linux scheduling（CFS） |
| **負載平衡、`select_task_rq`** | **第 26 講** |
| **`dl_rq`、EDF、即時排程的可排程性** | **第 27 講**：Real-time schedulers |
| **多資源、資源分配** | **第 28 講**：Real-time resource allocation |
| **lockdep 是一份資源的特例** | 回顧第 22 講 |

（講次對應依播放清單標題推估。）

---

## 附：本講一句話總結

> **多核心排程是一個 dispatcher 把 job 分到每顆 CPU 的 run queue（Linux 是 per-CPU 的 `struct rq`，由 `select_task_rq` 依 affinity 挑 CPU），目標換成最後一個 job 做完的時間 makespan C_max。可搶占時很簡單，把工作切平均就好（還要不小於最長的 job）；不可搶占時等價於 bin packing，是 NP-complete。啟發式 list scheduling 只要 CPU 不故意閒著，不管優先權怎麼排，都保證在最佳的 2 − 1/m 倍以內（因為最後那個 job 開始前所有 CPU 都在忙，再用 C* ≥ 最長 job、C* ≥ 平均工作量兩個下界），實測這個界限是緊的；長的先做（LPT）更好，在 4/3 − 1/(3m) 以內。每種資源有多份時，有環不一定 deadlock，要用 Banker's algorithm：假想地反覆滿足「需要的 ≤ 目前空著的」的 process 並收回它的資源，全部滿足得了就安全；每次要求前先假裝分配再檢查安全，不安全就拒絕，用實際要求跑同樣的流程就是 deadlock 偵測。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 25（Chapter 5, Part VIII）錄影逐字稿（YouTube 自動字幕，39:53）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§5.4.3–5.4.4，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
