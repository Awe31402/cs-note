# Real Time Systems 第五講完整筆記：Clock-driven 排程

> **課程**：Real Time Systems — Lecture 5：Clock-driven Scheduling（播放清單第 5 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（1:02:44，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch2 §3.2–5（PDF p.37–46）
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
- [1. 複習：控制迴圈與 cyclic executive](#1-複習控制迴圈與-cyclic-executive)
- [2. Clock-driven 排程的基本想法](#2-clock-driven-排程的基本想法)
- [3. Table-driven 排程](#3-table-driven-排程)
  - [3.1 排程表與 major cycle](#31-排程表與-major-cycle)
  - [3.2 定理：major cycle = LCM（就算有相位）](#32-定理major-cycle--lcm就算有相位)
  - [3.3 Table-driven 排程器的虛擬碼](#33-table-driven-排程器的虛擬碼)
  - [3.4 缺點：計時器設太多次](#34-缺點計時器設太多次)
- [4. Cyclic 排程](#4-cyclic-排程)
  - [4.1 Major cycle 與 minor cycle（frame）](#41-major-cycle-與-minor-cycleframe)
  - [4.2 Job slice](#42-job-slice)
  - [4.3 例子：四個任務、frame 4 ms](#43-例子四個任務frame-4-ms)
  - [4.4 Cyclic executive 的虛擬碼與 ABCD 例子](#44-cyclic-executive-的虛擬碼與-abcd-例子)
  - [4.5 【原始碼】Linux 的週期 tick](#45-原始碼linux-的週期-tick)
  - [4.6 【實測】用一個週期 timerfd 跑 cyclic executive](#46-實測用一個週期-timerfd-跑-cyclic-executive)
- [5. 選 frame 大小的三個限制](#5-選-frame-大小的三個限制)
  - [5.1 限制 1：F ≥ max eᵢ](#51-限制-1f--max-eᵢ)
  - [5.2 限制 2：F 整除 M](#52-限制-2f-整除-m)
  - [5.3 限制 3：到達和 deadline 之間至少一整個 frame](#53-限制-3到達和-deadline-之間至少一整個-frame)
  - [5.4 定理：最小間隔 = gcd(F, pᵢ)](#54-定理最小間隔--gcdf-pᵢ)
  - [5.5 合起來](#55-合起來)
  - [5.6 【課本補充】例題與【實測】frame 大小搜尋](#56-課本補充例題與實測frame-大小搜尋)
- [6. 非週期與偶發任務：slack stealing](#6-非週期與偶發任務slack-stealing)
- [7. Frame overrun 與排程變更](#7-frame-overrun-與排程變更)
- [8. Cyclic 排程的優缺點、generalized scheduler](#8-cyclic-排程的優缺點generalized-scheduler)
- [9. 可搶占 vs 不可搶占](#9-可搶占-vs-不可搶占)
- [10. 【課本補充】其他分類、round-robin、foreground-background](#10-課本補充其他分類round-robinforeground-background)
- [11. 一頁速查表](#11-一頁速查表)
- [12. 與後續講次的連結](#12-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

Clock-driven 排程：**排程點只在時鐘中斷**。排程事先算好、存成表，執行時照表跑。

1. **Table-driven**：表裡寫「哪個任務、從什麼時候開始」。表長 = 所有週期的 LCM。
2. **Cyclic**：把 major cycle 切成固定大小的 **frame**，只在 frame 邊界做決定。重點是**怎麼選 frame 大小**（三個限制）。
3. 剩下的空檔（slack）拿來跑非週期、偶發任務。

---

## 1. 複習：控制迴圈與 cyclic executive

[00:16]–[03:30]

- 基本模型：輸入調理 → 輸入介面 → 即時電腦 →（人機介面）。
- 單一感測器 / 致動器迴圈的虛擬碼：每 T ms 設一次計時器中斷 → ADC → 算控制輸出 → DAC。
- 更複雜的系統：任務有不同週期 p₁、p₂、p₃；階層式控制（上層如航管取樣慢，下層取樣快）。
- 最簡單的 **cyclic executive**：等輸入 → 處理 → 輸出 → 等下一個週期。
  - Event-driven：任務實例來了才醒。
  - Cycle-driven：設計好週期，計時器讓我在實例到達時剛好醒來，立刻處理。

---

## 2. Clock-driven 排程的基本想法

[04:00]–[07:00]

- 只在**時鐘中斷**（例如時鐘的上升緣）做排程決定。
- 那個時刻，看已經到達的任務，決定跑哪個。
- 「誰、何時、跑多久」存在**表**裡 → 所以叫 table-driven。
- 也叫 **offline / static 排程器**：排程事先存好，執行時幾乎不用算。嵌入式應用大量使用。

⚠️ 老師說 **round-robin** 是 clock-driven 排程「最常見的例子」[05:30]：每次時鐘中斷照同樣順序 T1、T2、T3、T4 跑。課本 p.36、p.45 把 time-sliced round-robin 歸為 **hybrid**（時鐘中斷 ＋ 事件都會觸發排程）。

---

## 3. Table-driven 排程

### 3.1 排程表與 major cycle

[06:32]–[08:01]

n 個週期任務 T₁…Tₙ，週期 p₁…pₙ。把排程存成表，重複執行。表要存多長？

> **Major cycle（hyperperiod）M = LCM(p₁, …, pₙ)**

【課本補充】（p.38）

- 排程表通常是程式設計師在**設計時**自己決定、寫進去的，不是排程器自動算的。
- 例：e₁=5, p₁=20；e₂=20, p₂=100；e₃=30, p₃=250 ms → 每 LCM = 500 ms 重複一次。
- 定義：**major cycle 是時間軸上一段區間，每個 major cycle 裡各任務以完全相同的方式出現**。

### 3.2 定理：major cycle = LCM（就算有相位）

[08:33]–[11:32]（課本 Theorem 2.1，p.38–39）

設 major cycle 長 M。任務 Tᵢ 相位 φ，在一個 major cycle 內出現 k 次（老師的版本）：

```
|<--------------- M --------------->|<--------------- M ---------------->|
| φ |Ti(1)  Ti(2) ...   Ti(k)   | x | φ |Ti(k+1) ...
```

- 下一個 major cycle 也要在 φ 之後出現第一個實例，排程才能沿用。
- 從 Tᵢ(1) 到下一個 cycle 的第一個實例，間隔一定是整數個週期。
- 推得 M 是 pᵢ 的整數倍：pᵢ | M。
- 每個任務都成立 → M 是所有 pᵢ 的公倍數 → 最小的就是 **LCM(p₁, …, pₙ)**。
- 相位不影響結論。

課本的寫法：M = (k−1)·pᵢ + φ + x；要讓下個 cycle 一樣，φ + x 必須等於 pᵢ，所以 M = k·pᵢ。

### 3.3 Table-driven 排程器的虛擬碼

[12:31]–[15:30]

```
// 排程表有 10 個 entry
entry = -1
timer_handler():
    entry = (entry + 1) % table_size          // 模加：繞回開頭
    current = sched_table[entry].task
    next_time = sched_table[entry].time + get_time()
    set_timer(next_time)                      // 每個 entry 都要設一次計時器
    execute(current)
    // 如果 current 提早做完、還有空檔 → 跑非週期任務
```

排程表例子【口頭】：

| 任務 | 時間 |
|---|---|
| T1 | 0–100 |
| T2（第 1 個實例） | 101–150 |
| T3 | 151–225 |
| T2（第 2 個實例） | … |
| T1（第 2 個實例） | … |
| （重複） | |

優點：很省空間、執行時負擔小，適合低成本應用。

缺點：**不彈性**。中途冒出來的非週期、偶發任務沒辦法容納。

### 3.4 缺點：計時器設太多次

[15:30]–[16:30]

- 任務很多、每個實例很短時，**每個任務都要設一次計時器**，處理計時器中斷的開銷就很可觀。
- Cyclic 排程器就是為了改善這點：工業界和大量嵌入式系統都用它。

【課本補充】（p.45 §4.4）

- Cyclic 只在初始化時設**一次**週期計時器；table-driven 每個任務開始都要設一次。即時任務通常只跑幾 ms，所以每幾 ms 就要設一次計時器。
- 但如果計時器開銷可以忽略，table-driven 反而**比較 proficient**：cyclic 的 frame 至少要和最長任務一樣大，短任務會浪費處理器時間。

---

## 4. Cyclic 排程

### 4.1 Major cycle 與 minor cycle（frame）

[16:30]–[22:00]

- 排程一樣事先算好，存一個 major cycle，重複執行。
- 不同：major cycle 切成幾個 **minor cycle**，每個叫一個 **frame**。
- **排程點只在 frame 開頭**。
- Frame 內每個 job 確切的開始、結束時間不知道，但知道**最大**執行時間，這就夠了。
- Frame 邊界由**週期性計時器**的中斷標記。
- 兩層階層：minor cycle、major cycle。

```
|<------------ major cycle ------------>|<------------ major cycle ------------>|
| f1  | f2  | f3  | f4  |               | f4n | f4n+1 | ...
  ↑ 每個 frame 開頭：tick → 跑排程器（很短）→ 照表跑這個 frame 的任務
```

- Major cycle 至少是 LCM(p₁…pₙ)，就算有任意相位也一樣。
- 如果 frame 大小 F 不整除 major cycle，hyperperiod 可能要比 LCM 更長——不希望這樣，所以不考慮。
- Major cycle 通常包含**整數個** frame。

### 4.2 Job slice

[17:30]–[19:02]

- 若某組 job 排不出來，可以把一個 job（例如 T₁ₖ）切成幾個 **job slice**（工作切片）。
- 先跑 slice 1 → 中間插其他更重要、deadline 更近的任務 → 再跑 slice 2。
- **前提：job 要能切**。在自然的斷點切，否則別的 job 可能拿到算一半的結果，產生不一致 [35:01]。

### 4.3 例子：四個任務、frame 4 ms

[22:00]–[24:31]

Major cycle 20 ms、frame 4 ms，5 個 frame。

| 任務 | 週期 | 執行時間 |
|---|---|---|
| T1 | 4 ms（在 0、4、8、12、16 到達） | 1 ms |
| T2 | 5 ms | — |
| T3 | 20 ms | — |
| T4 | 20 ms | — |

- T2 在 t = 5 到達。這時第 2 個 frame（4–8）的排程**已經決定了**，所以 T2 不能在 frame 2 跑，被推到 frame 3（8–12）。
- 到 frame 3 開頭，排程器看到 T2 的實例，立刻拿來跑，甚至先於 T1。
- T3、T4 週期 20，可以放在任何 frame。
- 重點：**在 frame 開頭還沒到達的任務，要等到下一個 frame**。

### 4.4 Cyclic executive 的虛擬碼與 ABCD 例子

[25:02]–[30:02]

Major cycle 10 ms、minor cycle 1 ms 的虛擬碼：

```
frame = -1
tick_handler():                     // 每個 frame 開頭被呼叫
    if (上一個 frame 的工作還在跑)
        處理 task overrun             // 錯誤情況
    frame = (frame + 1) % num_frames
    schedule = read_schedule(frame) // 讀出這個 frame 的排程，之後不能改
    execute_schedule(schedule)      // 依序執行這個 frame 的每個 job
    // 可延伸：若還有 slack，跑非週期 / 偶發任務
```

Frame 內**不搶占**：在 frame 邊界決定好，就照跑。新的決定要等下一個 frame 邊界。

**ABCD 例子**【口頭】（e, p, d）：

| 任務 | e | p | d |
|---|---|---|---|
| A | 1 | 10 | 10 |
| B | 3 | 10 | 10 |
| C | 2 | 20 | 20 |
| D | 8 | 20 | 20 |

- 老師：「8 好像太多了」，把 D 切成 D1 = 2、D2 = 6 兩個 slice。
- Major cycle = LCM(10, 10, 20, 20) = 20 ms，frame = 10 ms。

| Frame | 排程 | 用掉 |
|---|---|---|
| 0（0–10） | A, B, C, D1 | 1+3+2+2 = 8 |
| 1（10–20） | A, B, D2 | 1+3+6 = 10 |

- A、B 週期 10，每個 frame 都要跑。
- 實作：設好「目前時間 ＋ 10」的時鐘中斷；frame 號碼模 2，在 0 和 1 之間交替。

補充（不是課堂說法，是用 §5.6 的程式算的）：不切的話 D 的 8 ≤ 10，frame 大小限制其實滿足；問題是 A+B+C+D = 14 > 10，**塞不進一個 frame**。切開後才能分到兩個 frame。

### 4.5 【原始碼】Linux 的週期 tick

Cyclic executive 的「每個 frame 開頭的 tick」，Linux kernel 自己也有一個：週期性時鐘中斷。

`kernel/time/tick-common.c:85`

```c
static void tick_periodic(int cpu)
{
	if (tick_do_timer_cpu == cpu) {
		raw_spin_lock(&jiffies_lock);
		write_seqcount_begin(&jiffies_seq);

		/* Keep track of the next tick event */
		tick_next_period = ktime_add_ns(tick_next_period, TICK_NSEC);

		do_timer(1);
		write_seqcount_end(&jiffies_seq);
		raw_spin_unlock(&jiffies_lock);
		update_wall_time();
	}

	update_process_times(user_mode(get_irq_regs()));
	profile_tick(CPU_PROFILING);
}
```

- 下一個 tick = 上一個 + `TICK_NSEC`（固定間隔），和 frame 邊界一樣是等距的。
- `update_process_times()` 會呼叫排程器的 tick 處理（`scheduler_tick()`），檢查要不要換任務——這就是「在時鐘中斷時做排程決定」。
- 但一般 Linux 是 **event-driven 為主**：任務醒來、阻塞、結束時都會重新排程，tick 只是其中一個排程點。用課本的分類，比較接近 hybrid。

【實測】本機設定：`CONFIG_HZ=250`（tick 間隔 4 ms）、`CONFIG_NO_HZ_IDLE=y`（CPU 閒置時停掉 tick，省電）。

### 4.6 【實測】用一個週期 timerfd 跑 cyclic executive

把 ABCD 例子寫成 C 程式：只設**一次**週期 10 ms 的 `timerfd`，每次 `read()` 回來就是一個 frame 邊界；job 用忙迴圈模擬執行時間；跑 300 個 frame。

```c
static void (*sched[2][5])(void) = { { A, B, C, D1, 0 }, { A, B, D2, 0 } };
...
  timerfd_settime(tfd, 0, &it, NULL);                 /* the ONLY timer setup */
  for (int k = 0; k < frames; k++) {
    uint64_t n; read(tfd, &n, sizeof n);              /* tick: frame boundary */
    if (n > 1) overruns++;                            /* missed a boundary = frame overrun */
    ...
    for (int j = 0; sched[k % 2][j]; j++) sched[k % 2][j]();
    double slack = (t0 + (k + 1) * 10.0) - now_ms();  /* time left in frame: run aperiodics here */
```

結果：

```
frames=300  tick lateness avg=0.127 ms worst=0.192 ms  min slack=-0.191 ms  overruns=0
```

`strace -c`：

```
calls  syscall
  301  read
    1  timerfd_settime
```

看到什麼：

- 300 個 frame 只設了 **1 次**計時器。Table-driven 每個 job 都要設一次：這裡平均每 frame 3.5 個 job，約需 1050 次。
- Tick 平均晚 0.13 ms、最壞 0.19 ms（一般 Linux）。
- Frame 1 排滿 10 ms（slack = 0），加上 tick 晚到，最小 slack 變成 **−0.19 ms**：job 稍微跑過了 frame 邊界。`read()` 回傳的到期次數仍是 1，所以沒被算成漏掉整個 frame，但這已經是輕微的 overrun。**設計時不要把 frame 排到 100% 滿**。

---

## 5. 選 frame 大小的三個限制

[30:02]–[49:31]（課本 p.40–42）

### 5.1 限制 1：F ≥ max eᵢ

- 每個 job（或 job slice）要能在**一個 frame 內**跑完，不要溢出。
- 課本稱為「最少 context switch」：沒跑完就得暫停、下個 frame 再接著跑，要多一次 context switch。
- 這是 F 的**下限**。

### 5.2 限制 2：F 整除 M

- M mod F = 0（課本：⌊M/F⌋ = M/F）。
- 這樣 major cycle 有整數個 frame，排程表最小，跨 major cycle 能無縫重複。
- 課本：cyclic 排程器常用在儲存空間很小的嵌入式產品，表小對商業成功很重要。

### 5.3 限制 3：到達和 deadline 之間至少一整個 frame

- 任務只能在 frame 開頭被拿來排。
- 最壞情況：任務在 frame **剛開始一點點**（ε）之後到達 → 這個 frame 用不上，要等下一個 frame。
- 如果 deadline 落在下一個 frame 裡，而任務可能要跑滿一整個 frame → 會錯過。
- 所以**到達和 deadline 之間至少要有一個完整的 frame**。

```
kF   t(到達)        (k+1)F              (k+2)F
|-Δt-|---------------|===== 完整 frame =====|
                                            ↑ deadline 要在這之後
```

課本：任務在 frame 開始 Δt 之後到達，能趕上的條件是 **2F − Δt ≤ dᵢ**。Δt 越小越糟。

### 5.4 定理：最小間隔 = gcd(F, pᵢ)

[39:00]–[45:01]（課本 Theorem 2.2，p.42）

> 任務 Tᵢ（相位 0）所有實例中，到達時間和其 frame 開頭的**最小**間隔 min(Δt) = **gcd(F, pᵢ)**。

證明（反證法）：

- 令 g = gcd(F, pᵢ)。假設某實例 Tᵢ(n) 落在第 m 個 frame，且 0 < m·F − n·pᵢ < g。
- 同除以 g：0 < m·(F/g) − n·(pᵢ/g) < 1。
- F/g、pᵢ/g 都是整數（g 是公因數），所以左邊是兩個整數的差。
- 兩整數的差不可能嚴格介於 0 和 1 之間 → 矛盾。

【口頭】老師把 I₁ = I₂（差為 0，到達剛好落在 frame 邊界）當成另一種情況討論 [42:01]–[44:30]：差為 0 時任務在邊界到達，沒有浪費。

⚠️ 逐字稿 [41:20] 一段公式沒有被字幕記錄（老師在板書上寫），以課本的推導為準。

### 5.5 合起來

把最壞的 Δt = gcd(F, pᵢ) 代入：

> **2F − gcd(F, pᵢ) ≤ dᵢ，對每個任務都要成立**（課本式 2.5）

即 F ≤ (gcd(F, pᵢ) + dᵢ) / 2，這是 F 的**上限**。

| 限制 | 式子 | 作用 |
|---|---|---|
| 1 | F ≥ max eᵢ | 下限 |
| 2 | M mod F = 0 | 只能取 M 的因數 |
| 3 | 2F − gcd(F, pᵢ) ≤ dᵢ（每個 i） | 上限 |

- 這不是一個連續區間：F 只能取 M 的因數，選擇不多 [49:01]。
- 找到合適的 F **不代表一定排得出來**：可能 frame 數量不夠分給所有實例。
- 沒有任何 F 合適時，常見原因是某個任務執行時間太大，把下限撐高了 → 把它切成 job slice（要注意一致性）。
- 【課本補充】多個 F 都合適時，選**最小的**：frame 越多，越容易排得出來。

⚠️ 課本 p.42 寫「frame size must be smaller than **max**(gcd(F,pᵢ)+dᵢ)/2」。限制 3 要對**每個**任務成立，應該是取 **min**。

### 5.6 【課本補充】例題與【實測】frame 大小搜尋

寫一個 Python 程式，對 M 的每個因數檢查三個限制：

```python
for F in range(1, M + 1):
    if M % F: continue                                   # (2) F divides M
    c1 = F >= emax                                       # (1) F >= max e
    bad = [i + 1 for i, (e, p, d) in enumerate(tasks) if 2 * F - gcd(F, p) > d]  # (3)
```

結果：

| 任務集合 (e, p)，d = p | M | 可行的 F |
|---|---|---|
| 課本 Ex 2.1：(1,4)(1,5)(1,20)(2,20) | 20 | **2** |
| 課本 Ex 2.2：(1,4)(2,5)(5,20) | 20 | **無** |
| Ex 2.2，T3 切成 1+2+2 | 20 | **2** |
| 課堂 ABCD：(1,10)(3,10)(2,20)(8,20) | 20 | **10** |
| ABCD，D 切成 2+6 | 20 | **10** |

細節（Ex 2.1）：

```
  F=  1: F<max e
  F=  2: OK
  F=  4: 2F-gcd>d for T[2]       ← 2·4 − gcd(4,5) = 7 > 5
  F=  5: 2F-gcd>d for T[1]       ← 2·5 − gcd(5,4) = 9 > 4
  F= 10: 2F-gcd>d for T[1, 2]
  F= 20: 2F-gcd>d for T[1, 2]
```

⚠️ 課本的兩處問題：

- Ex 2.1 的 e 最大是 2，但課本寫「F ≥ 1.5」。結果不受影響（F 只能取 M 的因數，1 本來就排除了）。
- Ex 2.2 切開 T3 後，課本說「F = 2 和 F = 4 都可行」。但 T2 的 p = 5，F = 4 時 2·4 − gcd(4,5) = 7 > 5，**不滿足限制 3**——和課本自己 Ex 2.1 對 F = 4 的判斷一致。程式算出只有 F = 2。

---

## 6. 非週期與偶發任務：slack stealing

[50:32]–[55:31]

- 非週期任務通常在處理器**有空**時跑。
- 週期任務提早完成沒有好處（它很可預測），但非週期任務在乎**回應時間**。所以有空就讓它們先跑。
- **Slack stealing**：原本是為優先權式系統提出的想法。在不違反週期任務 deadline 的前提下，**盡量把非週期任務往前排**，縮短它們的回應時間。
- 前景 / 背景：週期任務是前景（我們在乎、已承諾的）；非週期任務是背景，在閒置區段跑。
- 非週期任務必須**可搶占**：它是「順便」跑的，週期任務要用時就得讓出。

**數學**：

- xₖ = 第 k 個 frame 裡 job slice 用掉的時間。
- 第 k 個 frame 的 slack = **F − xₖ**，在 frame 開頭就知道。
- 用掉 y 單位給偶發 / 非週期任務後，剩下 F − xₖ − y。
- 實作：每個 job slice 跑完後，若還有 slack，就檢查並跑非週期任務。

【口頭】老師用圖比較：沒有 slack stealing 時，非週期 job 只能塞在週期任務**之後**的閒置區段；有 slack stealing 時，把它們**挪到前面**先跑（不違反 deadline），必要時還把非週期 job 切成兩段。排程仍然可行，非週期任務的回應時間變短。

【課本補充】（p.44）

- 偶發任務有硬 deadline，到達時要做**接受測試（acceptance test）**：看 slack 夠不夠讓它在 deadline 前完成。不夠就拒絕，執行對應的恢復程序。
- 非週期任務沒有嚴格 deadline，不做接受測試，盡力而為。
- 有效率的做法：把每個 frame 的 slack 存成表，接受測試時查表。

---

## 7. Frame overrun 與排程變更

[55:31]–[57:32]

**Overrun**：job 的執行時間超過預估的最大值。原因：執行時間預估錯、輸入資料不尋常、有錯誤或故障。

處理方式：

1. **中止**這個 job，回報它提前終止：「沒時間做完，算了。」
2. **搶占**它（如果可以），剩下的部分**降級成非週期任務**，之後再排，盡量縮短回應時間——剛好接上 slack stealing。

目的：不讓延遲**連鎖**影響其他 job，害它們也錯過 deadline。

**排程變更**：job 新增、移除、執行時間調整。可以：

- 立刻凍結排程表、更新；或
- 等目前的 frame 結束、或等目前的 major cycle 結束，找適當時機再換。

---

## 8. Cyclic 排程的優缺點、generalized scheduler

[57:32]–[60:00]

| 優點 | 缺點 |
|---|---|
| 簡單、有效率 | 不彈性：難修改、難維護；需求一變就要凍結、更新排程表 |
| 比 table-driven 有更多決策點 | 任務多時很難找到 frame 大小 → 次佳排程、很多 frame 浪費 CPU、可排程性差 |
| 可延伸：處理先後關係、減少 context switch 造成的 jitter | Overrun 可能讓系統失效（雖然有上面的處理方法） |
| | 偶發、非週期任務不好處理（不是很難，但麻煩） |

**Generalized scheduler（一般化排程器）**：同時處理週期、非週期、偶發任務，用 frame 的 slack 跑後兩者。

課本 p.45 的虛擬碼：

```
cyclic-scheduler() {
current-task T = Schedule-Table[k];
k = k + 1;
k = k mod N;                 // N is the total number of tasks
                             //in the schedule table
dispatch-current-Task(T);
schedule-sporadic-tasks();   //Current task T completed early,
                             //sporadic tasks can be taken up.
schedule-aperiodic-tasks();  //At the end of the frame, the running
                             //task is preempted, if not complete.
idle();                      // No task to run, idle.
}
```

順序：先跑週期任務 → 有 slack 先給**偶發**、再給**非週期** → 都沒有就 idle，直到下一個 frame 或 hyperperiod 邊界。

---

## 9. 可搶占 vs 不可搶占

[60:00]–[62:30]

| | 可搶占（preemptive） | 不可搶占（non-preemptive） |
|---|---|---|
| 本質 | 事件驅動：有事發生就中斷正在跑的任務 | 只在任務**完成**時做排程決定 |
| 優點 | 更彈性、更能適應；有時回應時間好；有很好的最佳演算法（下一講） | 簡單、計算資源少 |
| 缺點 | 計算開銷大（動態切 job 比靜態切貴） | 不彈性；可能飢餓（starvation），高優先權任務錯過 deadline |
| 適合 | 任務切換時間很小（接近 0） | 執行時間和任務切換時間差不多、切換成本高 → 算一個靜態排程就好 |

- 本講的 frame timer 其實也被當成事件，用來打斷超時的任務、把剩下的部分變成非週期任務。
- 但本講的 job slice 是**靜態**切的；動態切會增加排程開銷。

下一講：經典的 **EDF（earliest deadline first）**。

---

## 10. 【課本補充】其他分類、round-robin、foreground-background

（p.37、p.45–47，課堂沒講或只帶過）

**依接受測試分**（p.37）：

- **Planning-based**：任務到達先檢查能不能趕上 deadline、會不會害別人錯過；不行就拒絕。
- **Best effort**：不測試，全部接受，盡力趕 deadline，不保證。

**依平台分**：單處理器、多處理器（有共享記憶體）、分散式（沒有共享記憶體、沒有全域即時狀態、靠訊息傳遞）。第 4 章再談。

**Time-sliced round-robin**（p.45–46，hybrid）：

- 就緒任務放在環狀佇列，每個跑一個時間片（time slice），沒跑完就放回佇列。
- 比 table-driven 或 cyclic **不 proficient**：所有任務一視同仁，不看優先權、關鍵性、deadline 遠近。
- 可以給高優先權任務更大的時間片，但仍然不適合即時：那只是讓高優先權任務**盡快**完成，而好的即時排程器要讓**盡量多**任務趕上 deadline。

**Event-driven 排程**（p.46）：

- Cyclic 很有效率，但任務一多就很難找 frame 大小，而且幾乎每個 frame 都浪費時間。
- Event-driven 能更好地處理非週期、偶發任務，但比較複雜、比較耗資源 → 中大型應用用 event-driven；小型嵌入式應用用 cyclic。
- 最簡單的 event-driven：**foreground-background**。週期即時任務是前景；偶發、非週期、非即時任務是背景，優先權最低。背景任務 T_B 的完成時間：

  **ct_B = e_B / (1 − Σ eᵢ/pᵢ)**（式 2.7）

  意思：前景任務吃掉 Σ eᵢ/pᵢ 的 CPU，背景只剩 1 − Σ eᵢ/pᵢ 可用。

---

## 11. 一頁速查表

### 11.1 Clock-driven 兩兄弟

| | Table-driven | Cyclic |
|---|---|---|
| 排程點 | 每個任務開始時（每次都設計時器） | 每個 frame 開頭（週期計時器只設一次） |
| 表的長度 | M = LCM(pᵢ) | M = LCM(pᵢ)，切成 M/F 個 frame |
| 開銷 | 計時器開銷大 | 小 |
| Proficiency | 計時器開銷可忽略時較高 | frame ≥ 最長任務，短任務浪費時間 |

### 11.2 定理

- Major cycle = LCM(p₁…pₙ)，有相位也一樣。
- 任務到達與 frame 開頭的最小間隔 = gcd(F, pᵢ)。

### 11.3 Frame 大小

1. F ≥ max eᵢ
2. M mod F = 0
3. 2F − gcd(F, pᵢ) ≤ dᵢ（每個 i）

多個可行時選最小的；沒有可行的就切 job（要能在自然斷點切）。可行的 F ≠ 一定排得出來。

### 11.4 Slack

第 k 個 frame 的 slack = F − xₖ。先給偶發（要接受測試），再給非週期（盡力），可用 slack stealing 往前挪。

### 11.5 Overrun

中止並回報；或搶占、剩下的變非週期任務。

### 11.6 Linux

`tick_periodic()` 每 `TICK_NSEC` 一次；本機 HZ=250（4 ms），閒置時停 tick。Linux 排程以事件驅動為主，tick 只是排程點之一。

---

## 12. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **Event-driven、可搶占、最佳演算法** | **第 6 支**：EDF |
| **Foreground-background、優先權式排程** | **第 7 支**：RMA |
| **非週期 / 偶發任務、接受測試** | **第 8 支**：RMA 的延伸 |
| **Jitter、context switch** | **第 9 支**：Jitter and Limited Priority Levels |
| **先後關係、資源** | **第 10–13 支**：Priority Inversion、PIP / HLP / PCP |
| **多處理器、分散式排程** | **第 14–15 支** |
| **計時器、週期 tick** | **第 18 支**：Hardware, Timers, and OSes |

---

## 附：本講一句話總結

> **Clock-driven 排程只在時鐘中斷做決定，排程事先算好存成表（所以叫 offline / static），簡單、開銷小，嵌入式大量使用，但難以容納非週期與偶發任務；table-driven 版本存一個 major cycle = LCM(p₁…pₙ)（就算任務有相位也一樣）的「任務—開始時間」表，缺點是每個任務都要設一次計時器；cyclic 版本把 major cycle 切成固定大小的 frame、只在 frame 開頭排程、週期計時器只設一次，frame 大小 F 要同時滿足 F ≥ max eᵢ、F 整除 M、以及 2F − gcd(F, pᵢ) ≤ dᵢ（因為任務到達與 frame 開頭的最小間隔恰為 gcd(F, pᵢ)），找不到就把大任務切成 job slice；每個 frame 剩下的 slack F − xₖ 先給偶發、再給非週期任務，可用 slack stealing 往前挪以縮短回應時間，overrun 時可中止或把剩下部分降級為非週期任務。實測用一個週期 timerfd 跑 300 個 frame 只需設定 1 次計時器，但排滿 100% 的 frame 會因 tick 延遲 0.19 ms 而輕微超時；課本 Ex 2.2 切分後「F = 4 也可行」的說法則被程式驗證為錯誤。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 5 錄影逐字稿（YouTube 自動字幕，1:02:44）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch2 §3.2–6 整理。原始碼為 Linux v6.2.12；實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
