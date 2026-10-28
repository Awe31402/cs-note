# Real Time Systems 第十五支影片完整筆記：多處理器與分散式系統的排程（Lecture 16）

> **課程**：Real Time Systems — Lecture 16：Scheduling in Multiprocessor Systems（播放清單第 15 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（43:32，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch4 §1–3（PDF p.98–101）
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
- [1. 多處理器排程可以多沒效率](#1-多處理器排程可以多沒效率)
- [2. 實用的啟發式](#2-實用的啟發式)
- [3. Runtime anomaly（排程異常）](#3-runtime-anomaly排程異常)
- [4. Predictability 與 critical instant](#4-predictability-與-critical-instant)
- [5. 靜態任務分配](#5-靜態任務分配)
  - [5.1 Utilization balancing](#51-utilization-balancing)
  - [5.2 Next fit（給 RMA）](#52-next-fit給-rma)
  - [5.3 Bin packing（給 EDF）](#53-bin-packing給-edf)
- [6. 動態任務分配](#6-動態任務分配)
  - [6.1 Focused addressing and bidding](#61-focused-addressing-and-bidding)
  - [6.2 Buddy algorithm](#62-buddy-algorithm)
  - [6.3 【原始碼】Linux RT 排程的 push：overloaded mask 與找最低優先權的 CPU](#63-原始碼linux-rt-排程的-pushoverloaded-mask-與找最低優先權的-cpu)
- [7. 容錯排程：ghost copy](#7-容錯排程ghost-copy)
- [8. 【實測】](#8-實測)
- [9. 一頁速查表](#9-一頁速查表)
- [10. 與後續講次的連結](#10-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

1. 多處理器上，靜態優先權排程在約 **50%** 使用率就可能失敗。
2. 沒有漂亮的界限 → 用**啟發式**。
3. 多處理器特有的怪現象：**runtime anomaly**（給更多資源反而錯過 deadline）、critical instant 不再是「全部同時釋出」。
4. 任務分配演算法：靜態（utilization balancing、next fit、bin packing）與動態（focused addressing and bidding、buddy）。
5. 容錯：ghost copy。

---

## 1. 多處理器排程可以多沒效率

[00:00]–[07:30]

設定：m 顆處理器，總容量 m。

- m+1 個任務，每個執行時間 1+ε，週期 = deadline = 2。
- 總使用率 = (m+1)(1+ε)/2 → ε→0 時 ≈ (m+1)/2，m 大時 ≈ **m/2**，也就是約 **50%**。
- 比單處理器 RMS 最差的 69% 還低，卻**排不出來**：
  - 不遷移的話，必有兩個任務擠在同一顆處理器：2 + 2ε > 2 → 錯過。
  - 老師：就算切開、移來移去也不行——前提是**靜態優先權**（某個任務永遠比另一個優先）。
- 所以靜態優先權多處理器排程器，可保證的使用率**不超過理論最大值的一半**。

**換成動態優先權 / 更聰明地移動任務就排得出來**【口頭】：三個任務 e = 1.1、p = d = 2，兩顆處理器 P1、P2。

| 時段 | P1 | P2 |
|---|---|---|
| 0–0.5 | T1 | T2 |
| 0.5–1.0 | T1 | T3 |
| 1.0–1.5 | T2 | T3 |
| 1.5–2.0 | T1、T2、T3 各 0.1 | — |

到 t = 1.5 每個任務都做了 1，剩下 0.5 裡各給 0.1 → 三個都在 2 前做完。這裡**沒有**哪個任務一直比另一個優先。

⚠️ 老師說三個任務的情況「靜態優先權下排不出來」。更精確地說：global 固定優先權下，最高的兩個先跑完 1.1，第三個 1.1 才開始，2.2 > 2 → 錯過；以 job 為單位的 global EDF 在這個例子也一樣失敗（三個 deadline 相同）。成功需要像上表這樣在 job 執行中途改變誰先跑（Pfair 的精神，上一支影片）。

---

## 2. 實用的啟發式

[07:30]–[09:30]

多處理器沒有像 EDF、RMS 那樣漂亮的界限，所以重點是好的啟發式：

1. **週期接近調和的任務放同一顆處理器**：調和週期下 RMS（Lehoczky）的界限是 1；接近調和就接近 1。
2. **依使用率分大小兩群**：以 **1/3** 為門檻（實驗決定）。小任務一起排、大任務一起排；或組成「一大一小」配對放同一顆 CPU。

還要決定：partitioned 還是 global；系統模型（分散式、多核、多處理器、同質或異質）；任務參數（週期 / 非週期、依賴、資源共享）。

---

## 3. Runtime anomaly（排程異常）

[09:30]–[16:02]

- 類比 OS 課的 **Belady's anomaly**（頁框越多、page fault 反而可能越多）。
- 一組可排程的任務，若**增加處理器、減少執行時間、放寬先後限制**，最壞回應時間在某些情況下反而**增加**，甚至錯過 deadline。這叫 **runtime anomaly**。
- 發生在**不可搶占**、有資源限制的多處理器排程。
- 定理：任何 **work-conserving**（貪婪：有任務就不閒著，不會為了等高優先權任務而故意閒置）的做法都可能發生 runtime anomaly。
  - 為什麼會想故意閒置：系統不可搶占，若預期等一下有高優先權任務來，先空著可能比較好。（RMS、EDF 是可搶占的，這裡不是。）

⚠️ 用語【口頭】[10:03]–[11:00]：老師說「worst case execution time」時其實指**最壞回應時間**（到達到完成），並說社群習慣這樣叫。本筆記一律寫「回應時間」，WCET 仍指任務本身的執行時間 e。

**例子**：兩顆處理器、不可搶占；T3 和 T4 有資源衝突（不能同時跑）。

| 任務 | 到達 | 執行時間 | deadline |
|---|---|---|---|
| T1 | 0 | 20 | 22 |
| T2 | 0 | 12 | 25 |
| T3 | 10 | 10 | 26 |
| T4 | 8 | 8 | 30 |

（依老師後面的講解整理；⚠️ 逐字稿唸出的是「t3 10 8 26、t4 8 10 30」，欄位順序沒說清楚。照講解，T3 在 10 到達、跑 10；T4 在 8 到達、跑 8。）

- **(a) T1 = 20**：T1 佔 P1 到 20，T2 佔 P2 到 12。12 時 T3、T4 都到了，先跑 T3（12–22），T4 不能和 T3 同時跑，22 才開始（22–30）。**全部趕上**。
- **(b) T1 縮短成 10**：10 時 P1 空出來，只有 T4 已在等 → 跑 T4（10–18）。12 時 P2 空出來，但 T3 和 T4 衝突，不能跑。18 才跑 T3（18–28）→ **錯過 26**。

T1 變快一倍，系統卻變差——runtime anomaly。（§8 實測重現。）

---

## 4. Predictability 與 critical instant

[16:02]–[20:30]

- **Predictable（可預測的）排程演算法**：對系統「好一點」（縮短執行時間、增加資源）不會讓總回應時間增加，也就是沒有異常。
- **Ha 與 Liu（1994）**：所有**可搶占**的固定優先權演算法都是可預測的。上面的異常來自**不可搶占**。

**Critical instant effect**：

- 單處理器 RMS：所有任務**同時**釋出是最壞情況（第 7 支影片）。
- **Global 固定優先權多處理器**：最壞回應時間**不一定**發生在全部同時釋出。
- 老師的例子：1、2、3、4 同時釋出時在某時刻全部完成；若 1、2 先釋出、3 和 4 晚一點分別到達，work-conserving 排程會讓 1、2 先佔住處理器，3 在某顆空出來時立刻跑、不等 4，4 要等 2 做完 → 總完成時間比同時釋出**更長**。
- 結論：多處理器上**知道所有任務越多，排得越好**；單處理器上沒有這種現象。

單處理器複雜度複習：RMS 優先權靜態，找任務是線性；EDF 要 priority queue，O(log n)。

---

## 5. 靜態任務分配

[20:30]–[32:01]

多處理器 / 分散式要把任務分給核心——bin packing，NP-hard。本講的演算法：

| 靜態 | 動態 |
|---|---|
| Utilization balancing、Next fit、Bin packing | Focused addressing and bidding、Buddy |

隱含假設：處理器同質；週期任務、無依賴；無資源限制；可搶占。

### 5.1 Utilization balancing

[22:30]–[24:32]

- 任務依使用率**遞增**排成佇列。
- 一個一個取出，分給**目前使用率最低**的處理器。
- 目標：讓各處理器使用率差不多（以使用率當重量分配）。但不會完全相等，會有一些不平衡。

【課本補充】（p.98）

- 目標函數：最小化 Σᵢ |ū − uᵢ|（ū = 平均使用率，uᵢ = 處理器 i 的使用率）。
- 適合處理器數**固定**、各處理器用 **EDF** 的情況。

### 5.2 Next fit（給 RMA）

[24:32]–[28:30]

- 想法：使用率**相近**的任務放同一顆處理器（各處理器跑 RMA）。
- 分 m 類：任務屬於第 j 類若且唯若

  **2^{1/(j+1)} − 1 < eᵢ/pᵢ ≤ 2^{1/j} − 1**（課本式 4.1）

- 像 Liu-Layland bound 的形式，但是啟發式、來自實驗。
- 分成 4 類時的格線（課本 p.99）：

| 類別 | 使用率範圍 | 寬度 |
|---|---|---|
| 1 | (0.41, 1] | 0.59 |
| 2 | (0.26, 0.41] | |
| 3 | (0.19, 0.26] | 0.07 |
| 4 | (0, 0.19] | |

- 高使用率的類別很寬（它們多半會獨佔一顆 CPU，不需要細分）；低使用率的類別很細（可以塞進零碎空間）。和第 9 支影片「有限優先權等級」的幾何 / 對數分法很像。
- 每類分配一顆或多顆專用處理器；不需要事先知道處理器數（課本）。
- 模擬：最多需要最佳處理器數的 **2.34 倍**。老師：類別太粗，所以結果不太好。

【課本補充】Ex 4.1（p.99）10 個任務的分類表（§8 實測檢查）。

### 5.3 Bin packing（給 EDF）

[28:30]–[32:01]

- 各處理器跑 EDF → 每顆的使用率 ≤ 1 就排得出來 → 純 bin packing：最少要幾顆處理器？下界 ⌈Σuᵢ⌉（課本）。

| 演算法 | 做法 | 需要的處理器 / 最佳 |
|---|---|---|
| First fit random | 隨機取任務，放進任一顆放得下的（使用率不超過 1） | 約 **1.7** 倍 |
| First fit decreasing | 依使用率**遞減**排序；一個一個放進**第一顆**放得下的處理器 | 約 **1.22** 倍 |

- 老師的描述 [30:30]–[31:31]：FFD 時，處理器依目前**使用率遞減**的順序考慮——先試最滿的，放不下才往下，最後才開新的空處理器（盡量把已有的填滿）。
- 老師總結：2.34、1.7 不太能接受，1.22 還可以。

⚠️ 老師在 [32:01] 把 first fit random 說成「1.77」，前面 [29:31] 說 1.7；課本是 1.7。

⚠️ 課本 p.100 的 FFD 描述只說「放進第一個放得下的 bin」，沒有說要依使用率排序處理器；「先試最滿的處理器」比較接近 **best fit** 的精神。這是老師的口頭描述，和課本略有不同。

（上一支影片的實測：30 個隨機任務，FFD 平均只比下界多 3.2%，first fit 多 9.7%。）

---

## 6. 動態任務分配

[32:01]–[41:00]

- 任務**非同步**地在不同節點出現，事先不知道哪些任務會來 → 出現時才分配。
- 執行期開銷高：每個節點的分配器要追蹤其他節點的負載，才能遷移任務。

### 6.1 Focused addressing and bidding

[32:31]–[37:00]

每顆處理器維護兩張表：

| 表 | 內容 |
|---|---|
| **Status table** | 自己承諾要跑的任務：執行時間、週期 / deadline |
| **System load table** | 其他所有處理器最近的負載（是自己的看法，**可能過時**） |

流程：

1. 時間軸切成固定長度的**視窗（window）**。
2. 每個視窗結束時，每顆處理器**廣播**下一個視窗中自己**空閒**的比例（例如已承諾 70%，就說 30% 空著）。收到的處理器更新 system load table。
3. 新任務到達某節點：
   - 能在本地處理 → 更新 status table（理想上要廣播，但訊息太多，所以資訊會不一致）。
   - 不能 → 查 system load table，找負載輕的處理器（**focused processors**），送出 **RFB（request for bids）**。
4. 對方可能在這期間已經變忙 → 回絕 → 要改問名單上的下一個。

【課本補充】（p.100）處理器只在確定「收集 bid、再把任務送到 focused processor 所需的時間」之後任務仍來得及完成，才送出 RFB。選擇可以根據距離、確切負載等。

- 缺點：**通訊開銷高**（定期廣播）。
- **視窗大小**是關鍵：視窗大 → 通訊少，但資訊更過時（課本：可能沒有 focused processor 投標）；視窗小 → 資訊新，但通訊開銷無法接受。

### 6.2 Buddy algorithm

[37:00]–[41:00]

目的：降低 focused addressing 的通訊開銷。

- 處理器只有兩種狀態：**underloaded**（uᵢ < Th）、**overloaded**（uᵢ ≥ Th）。
- **不定期廣播**：只在狀態**改變**時才送訊息。
- 也**不廣播給所有人**：只告訴自己的 **buddy set**（附近的處理器；多跳網路中通常是直接相鄰的鄰居）。Buddy set 不能太大也不能太小。
- 本地任務**允入測試失敗**（加進來會讓任務排不出）時，在 buddy set 裡找一個 underloaded 的，請它接手。

兩個政策：

| 政策 | 決定什麼 |
|---|---|
| **Information policy** | 多常、怎麼通知 buddy set；buddy set 多大（例如積極：一變就通知） |
| **Transfer policy** | 根據負載資訊選哪個 buddy 當接收者；對方答應或拒絕後轉移任務 |

### 6.3 【原始碼】Linux RT 排程的 push：overloaded mask 與找最低優先權的 CPU

Linux 的即時排程類別在多核上做的事和 buddy algorithm 有幾分像：CPU 上有不只一個可跑的 RT 任務時，標記自己「overloaded」，並把多出來的任務**推**給優先權最低（最閒）的 CPU。

`kernel/sched/rt.c:327`

```c
static inline int rt_overloaded(struct rq *rq)
{
	return atomic_read(&rq->rd->rto_count);
}

static inline void rt_set_overload(struct rq *rq)
{
	if (!rq->online)
		return;

	cpumask_set_cpu(rq->cpu, rq->rd->rto_mask);
```

`kernel/sched/rt.c:1880`

```c
static int find_lowest_rq(struct task_struct *task)
{
	struct sched_domain *sd;
	struct cpumask *lowest_mask = this_cpu_cpumask_var_ptr(local_cpu_mask);
	int this_cpu = smp_processor_id();
	int cpu      = task_cpu(task);
	int ret;

	/* Make sure the mask is initialized first */
	if (unlikely(!lowest_mask))
		return -1;

	if (task->nr_cpus_allowed == 1)
		return -1; /* No other targets possible */
```

對照：

| 本講 | Linux |
|---|---|
| overloaded / underloaded 兩種狀態 | `rto_mask`：哪些 CPU 是 RT overloaded（只在狀態改變時設定或清除） |
| 只通知 buddy set | 只在同一個 **root domain**（`rq->rd`）裡共享，不是全系統 |
| 找 underloaded 的接收者 | `find_lowest_rq()`：用 `cpupri` 找正在跑最低優先權的 CPU |
| 任務綁死在某節點、不能遷移 | `nr_cpus_allowed == 1` → 直接回 -1，沒有可推的目標 |

差別：Linux 是共享記憶體多核，狀態直接寫在共用資料結構，不必像分散式系統那樣傳訊息、也沒有過時的 system load table。

---

## 7. 容錯排程：ghost copy

[41:00]–[43:00]

- 某顆處理器可能故障 → 在不同 CPU 上排**多份**任務。這會傷即時保證，但換來容錯。
- **Ghost copy（次要 / 備份副本）**：主任務的冗餘副本。
- 副本可以**縮減**：故障時優雅降級。例：在影像中找特徵，備份只看部分影像或較低解析度 → 跑得快、負擔小，仍有一定容錯。
- 主任務成功 → 取消 ghost。
- 不同任務的 ghost 可以排在同一個時段（圖中 P 是主任務、B 是備份）。執行時當然不能同時跑兩個，要依應用的優先順序選一個。

課本 p.101：這種做法只需要很少的冗餘硬體；ghost 可以重疊（overload）在同一個時段，主任務成功後就釋放備份。

老師：單處理器與多處理器排程到此結束；之後講實際的 RTOS。

---

## 8. 【實測】

**Runtime anomaly**：兩顆 CPU、不可搶占、T3/T4 互斥，空出來的 CPU 選 deadline 最早的就緒任務（同一時刻先處理完成、再處理到達）。

```python
ready = sorted([x for x in waiting if (tasks[x][0] < t or tasks[x][0] == 0) and
                not any((x, y) in conflict for y in cpu if y)],
               key=lambda x: (tasks[x][2], x))
```

```
T1 exec = 20:
   T1: CPU0 start  0 finish 20 deadline 22
   T2: CPU1 start  0 finish 12 deadline 25
   T3: CPU1 start 12 finish 22 deadline 26
   T4: CPU0 start 22 finish 30 deadline 30
T1 exec = 10:
   T1: CPU0 start  0 finish 10 deadline 22
   T2: CPU1 start  0 finish 12 deadline 25
   T3: CPU0 start 18 finish 28 deadline 26  MISS
   T4: CPU0 start 10 finish 18 deadline 30
```

和老師的圖完全一樣：T1 縮短 10 個單位，T3 反而錯過 deadline。

（附帶一提：若改成「依到達順序」挑任務，T1 = 20 的情況就會在 12 時先挑 T4，T3 直接錯過——非搶占多處理器排程對挑選規則和事件順序非常敏感。）

**m+1 個任務、e = 1+ε、p = 2**（ε = 0.1）：

```
   m=2: U=1.65 (82% of capacity)  last task finishes 2.2 > 2 -> miss
   m=4: U=2.75 (69% of capacity)  last task finishes 2.2 > 2 -> miss
   m=8: U=4.95 (62% of capacity)  last task finishes 2.2 > 2 -> miss
lecture's dynamic schedule (3 tasks e=1.1 p=2, 2 CPUs) work by t=2: {'T1': 1.1, 'T2': 1.1, 'T3': 1.1}
```

m 越大，失敗時的使用率比例越接近 50%（m → ∞ 時 (m+1)(1.1)/2/m → 55%，ε → 0 時 → 50%）。老師的動態排程確實讓三個任務都做完 1.1。

**Next fit 分類（課本 Ex 4.1）**：

```
   grid: [1.0, 0.414, 0.26, 0.189]
   T1  u=0.500 class=1  book=1
   T2  u=0.333 class=2  book=2
   ...
   T9  u=0.129 class=4  book=4
   T10 u=0.170 class=4  book=3  <-- differs
```

⚠️ 課本 Table 4.2 把 T10（u = 17/100 = 0.17）放在第 3 類，但第 3 類是 (0.19, 0.26]；0.17 ≤ 0.189 應屬第 4 類。其他九個都一致。

---

## 9. 一頁速查表

| 主題 | 重點 |
|---|---|
| 靜態優先權的極限 | m+1 個 (1+ε, 2) 任務，使用率約 50% 就失敗 |
| 啟發式 | 接近調和的放一起；以 1/3 分大小、或大小配對 |
| Runtime anomaly | 不可搶占 ＋ 資源衝突：縮短執行時間 / 加資源反而錯過；work-conserving 都可能 |
| Predictable | 可搶占固定優先權演算法都可預測（Ha & Liu 1994） |
| Critical instant | 多處理器 global 固定優先權下，最壞不一定是同時釋出 |
| Utilization balancing | 依使用率遞增取，給最閒的處理器；適合 EDF、處理器數固定 |
| Next fit（RMA） | 類別 2^{1/(j+1)}−1 < u ≤ 2^{1/j}−1；最多 2.34 倍 |
| Bin packing（EDF） | First fit random 1.7 倍；FFD 1.22 倍 |
| Focused addressing | 兩張表、視窗結束時廣播空閒比例、RFB 給輕負載處理器；開銷高、視窗大小是取捨 |
| Buddy | 兩種狀態、狀態改變才通知、只通知 buddy set（鄰居）；information / transfer policy |
| Ghost copy | 冗餘（可縮減）的備份；主任務成功就取消 |
| Linux | RT push：`rto_mask`（overloaded CPU）＋ `find_lowest_rq()` |

---

## 10. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **分散式系統的時鐘（課本 Ch4 §4）** | **第 16–17 支**：Clock Synchronization |
| **實際的 RTOS** | **第 17–26 支**：RTOS 基礎、計時器、kernel 架構、POSIX、各種 RTOS |
| **節點之間的通訊** | **第 27–30 支**：Real Time Communication |

---

## 附：本講一句話總結

> **多處理器上靜態優先權排程可以非常沒效率：m+1 個執行時間 1+ε、週期 2 的任務只用了約一半容量就排不出來，但允許在 job 中途改變先後（例如三個 1.1/2 的任務切成 0.5、0.1 的片段）就能排；既然沒有漂亮的界限，就用啟發式（接近調和的任務放一起、以 1/3 使用率分大小）。多處理器還有單處理器沒有的怪現象：不可搶占又有資源衝突時，縮短某任務的執行時間反而讓別的任務錯過 deadline（runtime anomaly，實測重現 T1 從 20 縮到 10 時 T3 錯過），而可搶占的固定優先權演算法才是可預測的；global 固定優先權的最壞情況也不一定是全部同時釋出。任務分配方面，靜態有 utilization balancing（給最閒的處理器）、給 RMA 用的 next fit（依使用率分類，最多 2.34 倍處理器）、給 EDF 用的 bin packing（first fit random 1.7 倍、first fit decreasing 1.22 倍）；動態有定期廣播空閒比例、向輕負載處理器發 RFB 的 focused addressing and bidding（開銷高、視窗大小是取捨），以及只在 overloaded / underloaded 狀態改變時通知鄰近 buddy set 的 buddy algorithm；容錯則用可縮減的 ghost copy。Linux RT 排程的 rto_mask 加 find_lowest_rq 就是共享記憶體版的「標記過載、推給最閒的 CPU」；課本 Ex 4.1 則把 u = 0.17 的任務誤分到第 3 類。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 16 錄影逐字稿（YouTube 自動字幕，43:32）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch4 §1–3 整理。原始碼為 Linux v6.2.12；實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
