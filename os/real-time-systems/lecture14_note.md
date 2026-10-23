# Real Time Systems 第十四支影片完整筆記：即時分散式系統與多處理器排程入門（Lecture 15）

> **課程**：Real Time Systems — Lecture 15：Real Time Distributed Systems（播放清單第 14 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（41:07，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch4 開頭（PDF p.97）
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
- [1. 背景：共享記憶體 vs 訊息傳遞](#1-背景共享記憶體-vs-訊息傳遞)
- [2. 為什麼要即時分散式系統、難在哪](#2-為什麼要即時分散式系統難在哪)
- [3. 系統模型](#3-系統模型)
- [4. 多處理器排程為什麼難](#4-多處理器排程為什麼難)
- [5. Partitioned vs global](#5-partitioned-vs-global)
  - [5.1 Partitioned：bin packing](#51-partitionedbin-packing)
  - [5.2 Global：一條共用佇列](#52-global一條共用佇列)
  - [5.3 兩者比較](#53-兩者比較)
- [6. Job 層級排程會失敗 → Pfair](#6-job-層級排程會失敗--pfair)
- [7. Dhall's effect](#7-dhalls-effect)
- [8. 【實測】多處理器模擬](#8-實測多處理器模擬)
- [9. 【原始碼】Linux 文件裡的 Dhall's effect](#9-原始碼linux-文件裡的-dhalls-effect)
- [10. 一頁速查表](#10-一頁速查表)
- [11. 與後續講次的連結](#11-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

從單處理器走到多處理器 / 分散式：

1. 第一部分（很快帶過）：共享記憶體 vs 訊息傳遞、為什麼要分散式即時系統、系統模型。
2. 第二部分：多處理器排程 = **任務分配** ＋ **每顆處理器上的排程**。兩種做法：**partitioned**（靜態分配）和 **global**（一條共用佇列）。
3. 單處理器的最佳結果（EDF、RMS）在多處理器上**都失效**：Pfair、Dhall's effect。

---

## 1. 背景：共享記憶體 vs 訊息傳遞

[00:00]–[03:31]

| | 共享記憶體多處理器 | 分散式記憶體系統 |
|---|---|---|
| 記憶體 | 處理器共享位址空間（傳統上叫 UMA） | 不共享 |
| 溝通 | 讀寫共享記憶體 | 節點之間**傳訊息**（LAN 或 WAN 都可） |

- 也有「共享但不均勻（NUMA）」的模型；細節屬於計算機結構課。
- ⚠️ 老師說 UMA「cache 是共享的、看起來像一個 cache」[01:00]。UMA 的定義是**各處理器存取記憶體的時間相同**；cache 共不共享是另一回事。

**訊息傳遞**：兩個基本操作。

- `send`：指定 IP 位址和 port。
- `receive`：從任何人、或從特定的程序 / port / 遠端電腦接收，放進本地緩衝區。

| 優點 | 缺點 |
|---|---|
| 溝通與同步比較簡單；同步是隱含在送 / 收之中 | 比較難寫：演算法要改成送收的語意 |
| 溝通明確 → 程式好懂、好維護 → 開發成本低 | 處理訊息有時間開銷；每次傳訊息都要呼叫 OS，開銷大 |
| | 小規模系統上，共享記憶體效能比較高 |

---

## 2. 為什麼要即時分散式系統、難在哪

[03:31]–[05:31]

- 很多應用本身就大又分散，而且有即時需求：航管、軍事系統、核電系統……
- 單一電腦失效，整座核電廠就失效——不能接受。要**沒有單點故障**，就要有冗餘和容錯 → 自然變成分散式，而且必須是即時的。

困難：

- 資源管理難：沒有全域的工作量、甚至設定的知識。
- **時鐘不同步**，沒有單一時間基準。
- 通訊問題：封包亂序、遺失。網路協定能處理，但增加不確定性和延遲。
- 節點、連線故障很難找出來。
- 這些都增加 jitter 和不確定性，讓「每件事都要趕 deadline」更難。

【課本補充】（p.97）多處理器是**緊耦合**（有共享實體記憶體，程序間通訊便宜、可忽略），分散式是**鬆耦合**（通訊時間和任務執行時間相當）。所以多處理器可以用集中式的分派器，分散式不行（維護集中狀態的通訊開銷太大）。

---

## 3. 系統模型

[05:31]–[08:00]

- 任務在每個節點**各自獨立**地到達。
- 每個節點有一個**資源管理器**：管理本地工作量（何時執行、怎麼排）。
- 本地節點可能過載：
  - 需要特殊資源的任務只能在本地跑。
  - 其他任務可以**遷移**到別的節點。
- 節點要合作，讓越少任務錯過 deadline 越好。

工作量假設：

- 週期與非週期任務混合。
- 有先後限制、資源限制（前面講過）。
- **新的**：有**容錯需求**（冗餘任務、額外檢查）。

網路假設（之後的課才詳細討論）：網路在我們控制下，頻寬和緩衝都夠，訊息傳輸時間**固定或有上限**。

---

## 4. 多處理器排程為什麼難

[08:00]–[10:33]

老師引用 **1969** 年的一句話：

> "The simple fact that a task can use only one processor even when several processors are free at the same time adds a surprising amount of difficulty to the scheduling of multiple processors."

- 單處理器上最佳的 RMS、DMS、EDF，在多處理器上**保證全部失效**，很多問題變成 **NP-complete**。
- 過去 20 年很多研究，大多是 NP-hard 問題：最佳結果很少，多是實務上好用的啟發式方法，任務模型也被簡化。
- 有一些**充分**的可排程性測試，但沒有**必要**的。
- RTOS 也開始支援；隨著嵌入式、IoT，這領域仍在發展。

多處理器排程拆成兩個問題：

1. **任務分配（task assignment）**：任務沒有偏好要在哪顆跑，怎麼分給處理器？
2. **單處理器排程**：分好之後，每顆處理器怎麼排自己的任務？

【課本補充】（p.97）分配好之後，第二個問題就退化成單處理器排程。課本也強調：單處理器上獨立任務的最佳排程是多項式時間，多處理器 / 分散式卻是 NP-hard。

---

## 5. Partitioned vs global

[10:33]–[27:32]

- **Local scheduler**：每個節點有資源管理器兼排程器，必要時把負載轉給別的節點。
- **Global scheduler**：一個排程器負責全部節點的負載平衡，有轉移政策。

佇列組織：

```
Global（集中式）                  Partitioned（分散式）
   ┌────────┐                     ┌──┐ ┌──┐ ┌──┐
   │ 共用佇列 │──▶ CPU1             │Q1│ │Q2│ │Q3│  ← 任務事先分到各自的佇列
   └────────┘──▶ CPU2             └┬─┘ └┬─┘ └┬─┘
             ──▶ CPU3              CPU1 CPU2 CPU3（可能閒著）
```

- 共用佇列是**並行**佇列：多個生產者放、多個消費者（核心）拿 → 是**爭用點**。

### 5.1 Partitioned：bin packing

[13:01]–[20:31]

- 每個任務**永遠**在同一顆處理器上跑：靜態對應。
- 不允許任務遷移。
- 缺點：不夠彈性。可能 CPU3 完全閒著。負載一變（例如來了一個週期很短的新任務）就要**重新分割**，又會造成遷移。
- **最佳分割 = bin packing 問題**，NP-complete（NP-hard；很多變形是 NP-complete，給一個解很快能驗證）。

Bin packing 例子【口頭】：箱子容量 10 kg，球重 3、5、7 → 3 和 7 裝一箱，5 要另一箱。問：最少要幾個箱子？

- 一般情況很難，但有很好的啟發式：**first fit**（照順序，放進第一個放得下的箱子）；排序或不排序、從最重或最輕開始等變形，各有不同的保證。
- 分好之後，每顆處理器跑單處理器演算法：動態優先權用 **EDF**（有充要條件），靜態優先權用 **RMS**。

術語：靜態分配 = partitioned；處理器在任務出現時才分配 = global（動態）。

- 靜態簡單、可預測，所以很多實際系統用它。
- 動態複雜、開銷高，但如果能管好複雜度，結果可能更好——和 RMS vs EDF 的爭論類似。

### 5.2 Global：一條共用佇列

[17:32]–[24:30]

- 一個排程演算法排所有處理器上的所有任務；單一佇列；任務**可以遷移**。
- 適合處理器**對稱**的情況（任務在哪顆都能跑）；若某資源只有某顆處理器有，就複雜了。
- 要回答：可排程性分析、把任務分到哪、每顆上怎麼跑。

運作【口頭】：

1. 依政策排序的全域佇列；前 m 個任務派給 m 顆 CPU。
2. 某 CPU 做完 → 從佇列拉下一個（work queue；老師說也叫 work stealing queue，雖然這裡沒有在「偷」）。
3. **可搶占**時：高優先權任務到達，搶占**正在執行的任務中優先權最低的**那個。
4. 排程器可以改變主意：例如原本把 T4 放到 CPU3 搶 T3，後來改成讓 T3 繼續、把 T4 遷移到 CPU1 搶 T1；被搶的 T1 先「冷凍」，之後再恢復。

> Global 排程保證：**任何時刻，正在執行的是優先權最高的 m 個 job**。

### 5.3 兩者比較

[24:30]–[27:32]、[39:30]–[40:31]

| | Global | Partitioned |
|---|---|---|
| 負載平衡 | 好；容易處理動態負載；必要時可關掉某顆 CPU | 差 |
| 平均回應時間 | 較低（排隊理論、實驗都證實） | — |
| 過載管理 | 較有效率 | 超時只影響同一顆 CPU 上的任務（故障被侷限） |
| Context switch | 老師：「如果這是優先目標就能減少」，看演算法 | — |
| 遷移 | 有：被搶占的執行緒多半不會回到同一顆 CPU，cache、TLB 白暖了 | **沒有**遷移開銷 |
| 可排程性測試 | 很少 | 分好後直接用單處理器的結果 |
| 弱點 | 共用佇列是單點故障、爭用點，要互斥，可能成瓶頸 | 分配 = bin packing，NP-hard |

---

## 6. Job 層級排程會失敗 → Pfair

[27:32]–[32:31]

- 老師的例子：兩顆處理器上排三個任務，deadline 相同。T1、T2 的 job 各佔一顆，T3 的 job 只能等到其中一顆空出來 → 錯過 deadline。即使早點到也排不進去。（投影片數值逐字稿沒唸，§8 用經典的 (2,3)×3 重現。）

**Pfair 排程**：

- 處理器時間以**時間量（quantum）**為單位分配，quantum 是不可分割的基本單位。
- 把每個 job 切成**子任務**，每個子任務長度 = 一個 quantum；連續的子任務可以接著跑。
- **每個子任務有自己的 deadline**（偽 deadline），而不是整個任務一個 deadline。
- 例子：處理器 1 先跑 T1 一段（q 的倍數），處理器 2 跑 T2；藍色 job 做完時把剩下的部分**遷移**過去——沒有錯過 deadline。

> Pfair（加上這些隱含的偽 deadline）是**最佳的**，只要**總使用率 ≤ m**（m = CPU 數）。

- 老師：像是 EDF 的分散式版本，把所有處理器當成一個整體；總使用率可以 > 1，只要 ≤ m。前提是搶占和同步的開銷已處理好。
- 這只是特殊情況；一般任務模型下，傳統排程器都不是最佳的。

⚠️ 老師說 Pfair「可以叫 priority fair」[28:01]。這個名字通常解作 **proportionate fairness**（比例公平：每個任務在任何時刻得到的處理器時間要接近 wᵢ·t）。待補：課本沒有 Pfair 的內容可對照。

---

## 7. Dhall's effect

[32:31]–[39:30]

設定：m 顆處理器、m+1 個任務，d = p。

- τ₁…τₘ：執行時間 1，週期 T − 1。
- τₘ₊₁：執行時間 T，週期 T。

T 很大時，總使用率 = m/(T−1) + 1 → **趨近 1**（從上方），遠小於 m。

**RMS 失敗**：τ₁…τₘ 週期短 → 優先權高 → t = 0 時佔滿 m 顆 CPU → τₘ₊₁ 在 t = 1 才開始、要跑 T → 在 T + 1 完成，**錯過** deadline T。

**EDF 也失敗**：t = 0 時 τ₁…τₘ 的 deadline（T − 1）比 τₘ₊₁ 的（T）早 → 一樣先跑它們 → τₘ₊₁ 錯過。

**其實排得出來**：讓 τₘ₊₁ 一個人佔一顆 CPU 從 0 跑到 T；小任務都擠在其他 CPU 上（老師的版本是把小任務往旁邊挪，在空出來的空檔裡跑）。

這叫 **Dhall's effect**：使用率很低卻排不出來，顯示這些演算法在多處理器上多沒效率。

**蘋果與袋子**【口頭】：10 個袋子、每袋可裝 10 kg；10 顆 1 kg 蘋果、1 顆 9.1 kg 蘋果。照 RMS 的做法，先把 1 kg 蘋果每袋放一顆 → 9.1 kg 那顆哪袋都放不下。正確做法：1 kg 的全放一袋，9.1 kg 單獨一袋——兩袋就夠。

Partitioned 排程（≈ bin packing）也是 NP-hard，所以連這麼簡單的例子都會出問題。

---

## 8. 【實測】多處理器模擬

寫一個 1 單位步長的 global EDF / global RM 模擬器（可搶占、可遷移），每步讓最高的 m 個 job 跑：

```python
key = (lambda j: (j[0], j[1])) if policy == "EDF" else (lambda j: (tasks[j[1]][1], j[1]))
run = sorted(jobs, key=key)[:m]
```

**Dhall's effect**（T = 10）：

```
  m=2 T=10 U=1.222: global EDF misses=2 first=[(0, 10)]  global RM misses=8  partitioned (big alone | smalls together, U=0.22) -> OK
     global EDF, CPU0: 211111111111  CPU1: 3........23.
  m=3 T=10 U=1.333: global EDF misses=1 first=[(0, 10)]  global RM misses=8  partitioned (big alone | smalls together, U=0.33) -> OK
  m=4 T=10 U=1.444: global EDF misses=1 first=[(0, 10)]  global RM misses=8  partitioned (big alone | smalls together, U=0.44) -> OK
```

- 任務 1（大任務）在 t = 0 被小任務 2、3 擠掉，t = 1 才開始，錯過 deadline 10。
- 4 顆 CPU、總使用率只有 1.44，global EDF 和 global RM 都錯過；partitioned（大任務獨佔一顆）沒問題。

**三個 (2,3) 任務在兩顆 CPU**（U = 2 = m）：

```
  global EDF  misses: [(2, 3)]  CPU0: 113312  CPU1: 22.123
  Pfair EPDF  misses: []  CPU0: 132132  CPU1: 213213
  subtask windows of a (2,3) task: [(0, 2), (1, 3), (3, 5), (4, 6)]
```

- Global EDF（以 job 為單位）：任務 3 錯過第一個 deadline——老師說的「job 層級排程會失敗」。
- Pfair（以 quantum 為單位，子任務 k 的視窗是 [⌊(k−1)/w⌋, ⌈k/w⌉)，每步跑偽 deadline 最早的子任務，即 EPDF）：三個任務輪流、遷移，**沒有錯過**。（EPDF 在 m ≤ 2 時是最佳的；一般 m 需要更完整的 Pfair 演算法，例如 PD²。）

**Partitioned 的 bin packing**：30 個隨機任務（u ∈ [0.05, 0.7]），每顆 CPU 跑 EDF（使用率 ≤ 1 就放得下），看用了幾顆處理器 ÷ 下界 ⌈Σu⌉：

```
  first fit (random order): avg 1.097 max 1.308
  first fit decreasing    : avg 1.032 max 1.167
```

先排序（大的先放）明顯比較好——bin packing 雖然 NP-hard，啟發式已經很接近下界。

---

## 9. 【原始碼】Linux 文件裡的 Dhall's effect

Linux 的 `SCHED_DEADLINE` 預設是 **global EDF**。kernel 文件直接用 Dhall's effect 說明它不是最佳的：

`Documentation/scheduler/sched-deadline.rst:421`

```
 Consider a set {Task_1,...Task_{M+1}} of M+1 tasks on a system with M
 CPUs, with the first task Task_1=(P,P,P) having period, relative deadline
 and WCET equal to P. The remaining M tasks Task_i=(e,P-1,P-1) have an
 arbitrarily small worst case execution time (indicated as "e" here) and a
 period smaller than the one of the first task. Hence, if all the tasks
 activate at the same time t, global EDF schedules these M tasks first
 (because their absolute deadlines are equal to t + P - 1, hence they are
 smaller than the absolute deadline of Task_1, which is t + P). As a
 result, Task_1 can be scheduled only at time t + e, and will finish at
 time t + e + P, after its absolute deadline. The total utilization of the
 task set is U = M · e / (P - 1) + P / P = M · e / (P - 1) + 1, and for small
 values of e this can become very close to 1. This is known as "Dhall's
 effect"[7].
```

`Documentation/scheduler/sched-deadline.rst:440`

```
 have D_i = P_i, a sufficient schedulability condition can be expressed in
 a simple way:

	sum(WCET_i / P_i) <= M - (M - 1) · U_max

 where U_max = max{WCET_i / P_i}[10]. Notice that for U_max = 1,
 M - (M - 1) · U_max becomes M - M + 1 = 1 and this schedulability condition
 just confirms the Dhall's effect.
```

對照：

| 本講 | Linux |
|---|---|
| 老師的 τₘ₊₁ = (T, T)、τᵢ = (1, T−1) | 文件的 Task_1 = (P, P, P)、Task_i = (e, P−1, P−1)，同一個構造 |
| 只有充分測試、沒有必要測試 | 文件給的 Σuᵢ ≤ M − (M−1)·U_max 是充分條件；U_max = 1 時退化成 1 |
| Global EDF 不是最佳 | "global EDF is not an optimal scheduling algorithm"（文件 453–454 行） |
| Partitioned | 文件 642 行：用 **cpuset** 機制實作 partitioned 排程 |

一般任務的 partitioned 做法就是 `sched_setaffinity()` / `taskset` 把執行緒釘在某顆 CPU；不釘的話，CFS 和 RT 排程類別會做負載平衡、遷移（接近 global）。OS 課（`../linux-os/` 第 25 講）有 affinity 的實測。

---

## 10. 一頁速查表

| 主題 | 重點 |
|---|---|
| 共享記憶體 vs 訊息傳遞 | 緊耦合 / 鬆耦合；send、receive；訊息傳遞好懂但開銷大 |
| 分散式難處 | 沒有全域知識、時鐘不同步、封包亂序 / 遺失、故障難定位 |
| 模型 | 任務在各節點獨立到達、本地資源管理器、可遷移；先後、資源、容錯需求；網路傳輸時間有上限 |
| 兩個子問題 | 任務分配 ＋ 單處理器排程 |
| Partitioned | 靜態、不遷移、超時被侷限；分配 = bin packing（NP-hard，FFD 很好用） |
| Global | 一條佇列、可遷移、跑最高的 m 個 job；平衡好、回應快；佇列爭用、遷移傷 cache |
| Pfair | quantum 化、子任務有偽 deadline；總使用率 ≤ m 時最佳 |
| Dhall's effect | m+1 任務，U ≈ 1 也讓 global EDF / RMS 失敗；partitioned 卻排得出 |
| Linux | SCHED_DEADLINE = global EDF（文件明列 Dhall's effect）；cpuset / affinity = partitioned |

---

## 11. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **任務分配演算法（utilization balancing、next fit、bin packing for EDF）、動態分配（focused addressing、bidding、buddy）、容錯排程** | **第 15 支**：Scheduling in Multiprocessor Systems |
| **時鐘不同步** | **第 16–17 支**：Clock Synchronization |
| **網路傳輸時間有上限的假設** | **第 27–30 支**：Real Time Communication |

---

## 附：本講一句話總結

> **即時分散式系統（航管、核電）為了沒有單點故障而分散，但沒有全域知識、時鐘不同步、網路亂序遺失，讓趕 deadline 更難；模型是任務在各節點獨立到達、由本地資源管理器排程、必要時遷移，並假設網路傳輸時間有上限。多處理器排程拆成任務分配與單處理器排程兩個問題，單處理器上最佳的 EDF、RMS 在這裡全部失效、多數問題變成 NP-hard：partitioned 做法把任務靜態分給處理器（就是 bin packing，first fit decreasing 實測只比下界多約 3%），沒有遷移、故障被侷限，但負載平衡差；global 做法用一條共用佇列讓最高的 m 個 job 執行，平衡好、回應快，但佇列是爭用點、遷移會傷 cache。以 job 為單位的 global 排程可能失敗（實測三個 (2,3) 任務在兩顆 CPU 上 global EDF 錯過、Pfair 則全部趕上），Pfair 把工作切成 quantum 大小、各有偽 deadline 的子任務，在總使用率 ≤ m 時最佳；Dhall's effect 則顯示使用率只略大於 1 的任務集合就能讓 global EDF 和 RMS 錯過 deadline（實測 4 顆 CPU、U = 1.44 仍失敗），而 Linux 的 SCHED_DEADLINE 文件正是用同一個例子說明 global EDF 不是最佳的。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 15 錄影逐字稿（YouTube 自動字幕，41:07）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch4 開頭整理。原始碼為 Linux v6.2.12；實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
