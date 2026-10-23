# Real Time Systems 第十二支影片完整筆記：PCP（Lecture 13）

> **課程**：Real Time Systems — Lecture 13：The PCP Protocol（播放清單第 12 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（35:51，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch3 §5–7（PDF p.82–85、88）
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
- [1. PCP 的兩個新東西](#1-pcp-的兩個新東西)
  - [1.1 Current System Ceiling（CSC）](#11-current-system-ceilingcsc)
  - [1.2 不是貪婪的](#12-不是貪婪的)
- [2. 規則](#2-規則)
  - [2.1 Resource grant clause](#21-resource-grant-clause)
  - [2.2 Inheritance clause](#22-inheritance-clause)
  - [2.3 Resource release rule](#23-resource-release-rule)
  - [2.4 和 HLP 的關鍵差別](#24-和-hlp-的關鍵差別)
- [3. 例子](#3-例子)
- [4. PCP 下的三種 priority inversion](#4-pcp-下的三種-priority-inversion)
- [5. 定理：PCP 下任務只被擋一次](#5-定理pcp-下任務只被擋一次)
- [6. 為什麼沒有死結、unbounded inversion、chain blocking](#6-為什麼沒有死結unbounded-inversionchain-blocking)
- [7. 【實測】PIP / HLP / PCP 同場比較](#7-實測pip--hlp--pcp-同場比較)
- [8. 【原始碼】Linux 有沒有 PCP](#8-原始碼linux-有沒有-pcp)
- [9. 一頁速查表](#9-一頁速查表)
- [10. 與後續講次的連結](#10-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

PCP（Priority Ceiling Protocol）= 取 PIP 和 HLP 的優點：

| | PIP | HLP | **PCP** |
|---|---|---|---|
| Unbounded inversion | ✅ 解決 | ✅ | ✅ |
| 死結 | ❌ | ✅ | ✅ |
| Chain blocking | ❌ | ✅ | ✅ |
| Inheritance-related inversion | 只對真的在等的 | ❌ 很嚴重（升到 ceiling） | 只對真的在等的（和 PIP 一樣） |
| 新代價 | — | — | **avoidance-related inversion** |

做法：像 HLP 一樣每個資源有 ceiling；但**不**一拿到就升優先權，而是維護一個**系統 ceiling（CSC）**，用它來決定**要不要給**資源。

---

## 1. PCP 的兩個新東西

### 1.1 Current System Ceiling（CSC）

[01:32]–[04:30]

- 和 HLP 一樣：每個資源的 **ceiling** = 所有**可能**請求它的任務中最高的優先權（事先準確知道；假設 FIFO）。
- 新的：**CSC（current system ceiling，目前系統天花板）**

  > **CSC = max{ Ceil(CRᵢ) : CRᵢ 目前正在被使用 }**

- 資源（硬體）由軟體 semaphore 保護，「資源」和「semaphore」在這裡指同一件事。
- Ceiling 是理論上的最大值（靜態）；CSC 只看**目前在用**的資源（**動態**）。
- 系統啟動時沒有資源在用 → **CSC = 0**（比系統中最低的優先權還低）。之後隨資源取得與釋放上升、下降；沒有資源在用時又回到 0。

### 1.2 不是貪婪的

[03:01]–[04:02]

- PIP 和 HLP 都是**貪婪的**：資源空著就立刻給。
- PCP **不是**：就算資源空著，也可能**故意延後**、不給。
- 這種刻意的延後讓我們能證明很多好性質，實際上也沒那麼糟。

課本 p.82：PIP 和 PCP 的根本差別——PIP 是貪婪的，PCP 不是。

---

## 2. 規則

[04:30]–[16:30]

兩條規則：

```
Resource request rule
 ├─ Resource grant clause   ← 任務「請求」資源時套用
 └─ Inheritance clause      ← 任務「被迫等待」時套用（持有者會怎樣）
Resource release rule        ← 任務釋放資源時套用
```

### 2.1 Resource grant clause

[06:01]–[12:31]

任務 Tᵢ 請求資源 CRⱼ：

1. 若 Tᵢ **持有的某個資源的 ceiling 等於 CSC**（也就是 Tᵢ 自己設定了目前的 CSC）→ **給**。
2. 否則，**只有 pri(Tᵢ) > CSC 才給**。

拿到之後：若 CSC < Ceil(CRⱼ)，把 CSC 設成 Ceil(CRⱼ)。

老師的具體說法【口頭】[07:00]–[09:31]：

- CSC 原本是 9，某任務鎖了一個資源讓 CSC 變 10。
- 這個「設定了 CSC = 10」的任務可以繼續鎖資源。
- 其他沒有設定 CSC 的任務，除非優先權 **> 10**，否則都**不能**鎖任何資源。
- 所以設定了 CSC 的任務「幾乎拿到一張空白支票，想鎖多少資源都可以」；別人要鎖資源，優先權得比我高很多，那它本來就是很重要的任務，我們沒什麼好抱怨。

⚠️ 逐字稿 [08:00] 說「greater than the current system ceiling which is **15**」，但前文例子的 CSC 是 10。應是口誤或字幕錯誤，以 10 為準。

**為什麼拿到後 Ceil(CRⱼ) ≥ CSC**（老師的推導 [10:32]–[12:00]）：

- 情況 1：pri(Tᵢ) = CSC（Tᵢ 自己設的）。Tᵢ 會用 CRⱼ，所以 Ceil(CRⱼ) ≥ pri(Tᵢ) = CSC。
- 情況 2：pri(Tᵢ) > CSC。同理 Ceil(CRⱼ) ≥ pri(Tᵢ) > CSC。
- 兩種都是 Ceil(CRⱼ) ≥ CSC → 把 CSC 設成 Ceil(CRⱼ)。

（嚴格說情況 1 應是「Tᵢ 持有的資源 ceiling = CSC」，不是 pri(Tᵢ) = CSC；但 Ceil(CRⱼ) ≥ pri(Tᵢ) 不一定 ≥ CSC。課本的寫法是「若 CSC < Ceil(CRⱼ) 才更新」，比較精確。）

⚠️ 是「**大於**」CSC，不是「大於等於」——老師在 [24:02] 特別強調。

### 2.2 Inheritance clause

[12:31]–[13:30]

- 任務因為不滿足 grant clause 而被擋 → 它**阻塞**。
- **持有資源的任務**（若優先權比被擋的任務低）**繼承**被擋任務的優先權。
- 這和 **PIP** 一樣（繼承等待者的優先權），**不是** HLP 那種升到 ceiling。

【課本補充】課本 p.83 的條款就是：「任務因不滿足 grant clause 而無法鎖資源時，它阻塞，持有資源的任務若優先權較低，就繼承被擋任務的優先權。」

（被擋的資源若是**空的**——下面的 avoidance 情況——「持有資源的任務」指的是持有那個讓 CSC 這麼高的資源的任務。§7 的模擬器就是這樣實作。）

### 2.3 Resource release rule

[14:01]–[16:30]

Tᵢ 釋放 CRⱼ：

1. **CSC**：若 Ceil(CRⱼ) = CSC → 重新計算 CSC = 所有**仍在使用**的資源 ceiling 的最大值（ceiling 是靜態屬性，直接取 max）。否則 CSC 不變。
2. **Tᵢ 的優先權**：
   - 不再持有任何資源 → 回到原本的優先權。
   - 還持有 R1…Rₖ → 取「在等 R1…Rₖ 的所有任務」中最高的優先權（若比 Tᵢ 自己高就繼承，否則用自己的）。

老師：就是做好「記帳」——移除 Tᵢ 持有 CRⱼ 的紀錄，必要時更新 CSC 和 Tᵢ 的優先權。

### 2.4 和 HLP 的關鍵差別

[20:34]–[21:01]

> **PCP 下，任務拿到資源時，只有 CSC 改變；任務的優先權不變。**
> 任務的優先權只在 **inheritance clause** 啟動（有高優先權任務在等它）時才改變。

課本 p.83：PCP 和 HLP 很像，差別只在 PCP 拿到資源時不會立刻升到 ceiling；而且任務 Tᵢ 也可能因為「別的任務持有某個 ceiling ≥ pri(Tᵢ) 的資源」而被擋在 critical section 外。這避免了 HLP 不必要的 inheritance blocking；而拿 CSC 和請求者的優先權比較，避免了死結。

---

## 3. 例子

[16:30]–[20:34]

**老師的例子**（數字越大優先權越高）：資源 R 被優先權 10、2、5 的任務使用 → **Ceil(R) = 10**。另有 T4（優先權 8）只用 CR1（CR1 ≠ R）。

1. T2（優先權 2）拿到 R → **CSC = 10**。T2 的優先權**仍是 2**（HLP 會升到 10）。
2. T3（優先權 5）開始跑 → **可以**跑，因為 T2 沒被升級。這是和 HLP 的關鍵不同：沒有 inheritance-related inversion。
3. T3 要 R → R 不能搶占 → T3 等 T2；CSC 仍是 10；T2 **繼承** T3 的優先權：2 → 5。
4. T2 釋放 R → 優先權回到 2；R 給 T3，T3 優先權 5。
5. T4（優先權 8）要 CR1（空著）→ 8 **不 > 10** → **不給**。T4 在 CPU 上想跑多久都行，但拿不到資源。這是一種**新的** inversion（下一節的 avoidance-related inversion）。

**課本 Example 3.1**（p.83–84）：T1–T4 優先權 10、12、15、20；CR1 給 T1、T2、T3 用 → Ceil = 15；CR2 給 T1、T4 用 → Ceil = 20。T1 拿到 CR1 → CSC = 15。

- 情況 1：T4（20）搶占 T1，要 CR2 → 20 > 15 → **給**，CSC = 20。
- 情況 2：T3（15）搶占 T1，要 CR1 → 15 **不 >** 15 → **不給**；T3 阻塞，T1 繼承 15。

---

## 4. PCP 下的三種 priority inversion

[23:01]–[26:26]（課本 §6，p.84–85）

| 種類 | 發生情況 | 圖 |
|---|---|---|
| **Direct（直接）** | 高優先權 T_H 等低優先權 T_L 釋放它需要的資源 | T_H → [CR] ← T_L |
| **Inheritance-related（繼承相關）** | T_L 持有資源、T_H 在等 → T_L 繼承 T_H 的優先權 → 中間的 T_I（不需要資源）沒辦法跑 | T_I 被擋 |
| **Avoidance-related（避免相關）** | T_L 持有資源使 CSC 很高；T_H 優先權高於正在跑的任務，但**不高於 CSC**；它要的資源**空著**，仍然被拒 | 課本 Fig.8：Ceil(R1) = 10、CSC = 10、pri(T_H) = 8 |

- Avoidance-related 是 **PCP 新引入**的，來自 resource grant clause。
- 別名：**priority ceiling-related inversion**、**deadlock avoidance inversion**。
- 為什麼叫「避免」：這是**避免死結的代價**。

【課本補充】（p.85）

- 為什麼必須擋：Fig.8 中 T_L、T_H 都需要 R1 和 R2。若讓 T_H 拿 R2，之後 T_H 要 R1、T_L 要 R2 → 死結。
- 反過來說：若 pri(T_H) > CSC，代表 T_H 根本不會用任何正在被用的資源，給它是安全的。
- 但 PCP 也會在**根本不可能死結**時拒絕：T1–T4 優先權 2、4、5、10；T1、T4 共用 CR1（ceil 10）、T2、T3 共用 CR2（ceil 5）。T1 拿 CR1 → CSC = 10 → T2 要 CR2 被拒——但 T2 拿 CR2 不可能造成死結。這是 PCP 保守的地方。

---

## 5. 定理：PCP 下任務只被擋一次

[26:26]–[30:32]

> **Theorem**：PCP 下任務是 **single blocking** 的。
> **Corollary**：PCP 下任務最多經歷**一次** priority inversion，發生在**取得第一個資源**時。

證明（老師的版本）：

- 任務什麼時候能拿到資源？(a) pri > CSC，或 (b) 它自己設定了 CSC。
- 若是 (b)：往回追——它是怎麼設定 CSC 的？是拿某個資源時設的。再往前追……一直追到它**還沒有任何資源**的那一刻。沒有資源就不可能設定 CSC，所以那一刻一定是 (a)：**拿第一個資源時，pri > CSC**（CSC 也可能是 0）。
- 拿到第一個資源之後，它就是設定 CSC 的那一個 → 依 grant clause 可以繼續拿資源。
- 唯一會擋它的情況：**第一次**要資源時，某個低優先權任務正拿著某個資源（使 CSC ≥ 它的優先權）→ 等一次。
- 之後只有比它優先權**更高**的任務 Tⱼ 會打斷它——那不算 priority inversion。Tⱼ 做完後控制權回到它，它仍被視為設定 CSC 的那一個。

課本 p.85 的總阻擋時間（三種只會遇到其中一種、而且只一次）：

> **btᵢ = maxⱼ { b_idj, b_iij, b_iaj }**（式 3.5，j 是比 i 低優先權的任務）

（數值例子在下一支影片的筆記。）

---

## 6. 為什麼沒有死結、unbounded inversion、chain blocking

[31:02]–[35:30]

**死結**：

- 經典情況：T1 鎖 R1 再鎖 R2；T2 鎖 R2 再鎖 R1。
- 兩個任務都會用 R1 和 R2 → Ceil(R1)、Ceil(R2) ≥ pri(T1)，也 ≥ pri(T2)。
- 只要其中一個（例如 T1）拿到 R1，CSC ≥ Ceil(R1) ≥ pri(T2) → pri(T2) **不 >** CSC → T2 **拿不到任何資源**（包括 R2）。
- 所以 T1 拿到 R1 後，R2 不可能落到 T2 手上 → 不會形成循環 → **沒有死結**。T2 先拿 R2 時同理。
- 老師的直覺：兩個**衝突的任務**（需要相同的一組資源）之間，grant clause 不允許一個「插隊」到另一個中間。

**Unbounded priority inversion**：

- 高優先權任務在等時，把優先權給（繼承給）持有資源的低優先權任務 → 中間任務無法搶占它 → 不會 unbounded。

**Chain blocking**：

- 任務只被擋一次（§5）→ 依定義不會 chain blocking。

---

## 7. 【實測】PIP / HLP / PCP 同場比較

沿用上一支影片的模擬器（`proto.py`），加上 PCP。PCP 只改兩處：授予條件和「誰擋住我」：

```python
def csc(excl):                                # PCP system ceiling from resources held by OTHER tasks
    return max([ceil[r] for r, o in owner.items() if o != excl] or [-1])
def can_lock(n, r):
    if r in owner: return False
    if proto == "pcp": return tasks[n][1] > csc(n)
    return True
def pcp_blocker(n, r):                        # who blocks n under PCP (holder of r, or holder of highest-ceiling resource)
    if r in owner: return r
    held = [(ceil[x], x) for x, o in owner.items() if o != n]
    return max(held)[1]
```

（`csc(excl)` 排除自己持有的資源，等同於「自己設定了 CSC 就可以拿」。）

圖例：`=` 執行、`#` 持有資源時執行、`b` 阻塞、`.` 就緒但沒跑。

**老師的例子**：R 被 T1(10)、T2(2)、T3(5) 用（Ceil = 10）；T4(8) 只用 CR1。T2 在 t=0 拿 R；T3 在 t=1 到達、t=2 要 R；T4 在 t=3 到達、要 CR1。

```
--- lecture example (avoidance inversion) [pip] ok
  T4  p=8     ##              finish=5 resp=2
  T3  p=5   =bbbbb##          finish=9 resp=8
  T2  p=2  #.#..##            finish=7 resp=7
--- lecture example (avoidance inversion) [hlp] ok
  T4  p=8     .##             finish=6 resp=3
  T3  p=5   .....=##          finish=9 resp=8
  T2  p=2  ####               finish=4 resp=4
--- lecture example (avoidance inversion) [pcp] ok
  T4  p=8     bb##            finish=7 resp=4
  T3  p=5   =bbb..##          finish=9 resp=8
  T2  p=2  #.###              finish=5 resp=5
```

- **PCP**：t=1 T3 照常跑（`=`），T2 沒被升級——和 HLP 不同（HLP 下 T3 一開始就是 `.`）。t=2 T3 要 R → 阻塞，T2 繼承 5。t=3 T4 要的 CR1 **空著**，但 8 不 > CSC 10 → **avoidance inversion**（`bb`），T2 再繼承 8 加速做完。
- **PIP**：T4 立刻拿到 CR1（`##`），因為 PIP 不看 CSC——在這個例子裡比較快，但 PIP 不防死結。

**PCP 跑上一支影片的三個情境**：

```
--- deadlock [pcp] ok
  T1  p=9    =bb##          finish=7 resp=5
  T2  p=1  ##.##            finish=5 resp=5
--- chain blocking [pcp] ok
  T1  p=9     =###              finish=7 resp=4
  T2  p=3    b....###           finish=10 resp=8
  T3  p=2   bb.......###        finish=13 resp=12
  T4  p=1  ###                  finish=3 resp=3
--- inheritance blocking (HLP's problem) [pcp] ok
  T6  p=6   ==                      finish=3 resp=2
  T5  p=5   ..===                   finish=6 resp=5
  T1  p=2  #.....####               finish=10 resp=10
```

| 情境 | PIP | HLP | PCP |
|---|---|---|---|
| 死結 | **死結** | 沒有 | 沒有（T1 要 CR1 時 9 不 > CSC 9 → 被擋，T2 繼承後做完兩個資源） |
| Chain blocking（T1 回應時間） | 10（被擋 3 次） | 4 | 4（被擋 0 次：T2、T3 一開始就因 CSC 被擋，拿不到資源） |
| 中間任務 T5、T6 回應時間 | 5、2 | **9、6** | 5、2（和 PIP 一樣，沒有 HLP 的問題） |

PCP 在三個情境都拿到最好的那一欄。

---

## 8. 【原始碼】Linux 有沒有 PCP

- Kernel：v6.2.12 只有 PI（`FUTEX_LOCK_PI` / `FUTEX_UNLOCK_PI`、rt_mutex），沒有 ceiling，也沒有 CSC 這種全系統的變數。
- POSIX 的 `PTHREAD_PRIO_PROTECT`（glibc 在使用者空間實作）是「一拿到鎖就升到 ceiling」——**這其實是 HLP**，不是本講的 PCP。

本講的原始 PCP 需要一個**全系統**的 CSC，每次鎖資源都要和它比較；這在多處理器、大量鎖的通用 OS 上代價太高。所以實務上（POSIX、Java 的 priority ceiling emulation）常用的是較簡單的 HLP。

（上一講老師說「Linux 不支援 HLP」的 ⚠️ 討論也適用這裡。）

---

## 9. 一頁速查表

### 9.1 定義

- Ceil(CR) = 可能用 CR 的任務中最高優先權（靜態）。
- CSC = 目前在用的資源中最高的 ceiling（動態；無資源時 0）。

### 9.2 規則

| 規則 | 內容 |
|---|---|
| Grant | 自己設定了 CSC → 給；否則 pri(Tᵢ) **>** CSC 才給。給了之後 CSC = max(CSC, Ceil(CRⱼ)) |
| Inheritance | 被擋 → 持有者繼承被擋者的優先權（像 PIP） |
| Release | Ceil(CRⱼ) = CSC 時重算 CSC；優先權回到原本，或剩下資源的等待者中最高的 |

拿資源時**只改 CSC，不改優先權**。

### 9.3 三種 inversion

Direct（等資源）、Inheritance-related（中間任務被繼承者擋）、**Avoidance-related**（資源空著但 pri ≤ CSC，避免死結的代價）。

### 9.4 性質

Single blocking（只在拿第一個資源時被擋一次）→ 沒有 chain blocking；沒有死結；沒有 unbounded inversion；inheritance-related inversion 只和 PIP 一樣多。阻擋時間 btᵢ = max(direct, inheritance, avoidance)。

### 9.5 Linux

只有 PI。POSIX `PRIO_PROTECT` = HLP（glibc 使用者空間）。

---

## 10. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **三種 inversion 的時間表、代入 Lehoczky** | **第 13 支**：Analysis of the PCP Protocol |
| **多處理器上的資源共享** | **第 15 支**：Scheduling in Multiprocessor Systems |
| **POSIX 的 PRIO_PROTECT** | **第 21 支**：POSIX standards |
| **RTOS 對 ceiling 協定的支援** | **第 22–25 支** |

---

## 附：本講一句話總結

> **PCP 結合 PIP 和 HLP：每個資源像 HLP 一樣有 ceiling，但多了動態的 current system ceiling（目前在用資源中最高的 ceiling，起始為 0）；任務要鎖資源時，除非它自己設定了 CSC，否則優先權必須嚴格大於 CSC 才給——所以 PCP 不是貪婪的，資源空著也可能被拒；被擋時持有者像 PIP 一樣只繼承被擋任務的優先權，拿到資源時只改 CSC、不改優先權，釋放時重算 CSC 並還原優先權。結果任務只會在拿第一個資源時被擋一次（single blocking），因此沒有 chain blocking；衝突的任務之間有一個拿到資源後 CSC 就擋住另一個，所以沒有死結；繼承機制擋住中間任務，所以沒有 unbounded inversion；而 inheritance-related inversion 只和 PIP 一樣多。代價是新的 avoidance-related inversion（資源空著但 pri ≤ CSC，是避免死結的代價，有時甚至在不可能死結時也擋）。實測模擬器顯示 PCP 在死結、chain blocking、中間任務被擋三個情境都拿到最好的結果；Linux kernel 只有 PI，POSIX 的 PRIO_PROTECT 其實是 HLP。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 13 錄影逐字稿（YouTube 自動字幕，35:51）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch3 §5–7 整理。原始碼為 Linux v6.2.12；實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
