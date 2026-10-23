# Real Time Systems 第十一支影片完整筆記：PIP 與 HLP（Lecture 12）

> **課程**：Real Time Systems — Lecture 12：PIP and HLP Protocols（播放清單第 11 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（53:53，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch3 §3–4（PDF p.77–82）
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
- [1. PIP：Priority Inheritance Protocol](#1-pippriority-inheritance-protocol)
  - [1.1 想法](#11-想法)
  - [1.2 規則](#12-規則)
  - [1.3 Kernel 裡的 lock / unlock 虛擬碼](#13-kernel-裡的-lock--unlock-虛擬碼)
  - [1.4 Inheritance blocking：公平的取捨](#14-inheritance-blocking公平的取捨)
- [2. PIP 的兩個缺點](#2-pip-的兩個缺點)
  - [2.1 死結](#21-死結)
  - [2.2 Chain blocking](#22-chain-blocking)
- [3. PIP 的阻擋時間](#3-pip-的阻擋時間)
- [4. HLP：Highest Locker Protocol](#4-hlphighest-locker-protocol)
  - [4.1 Ceiling priority](#41-ceiling-priority)
  - [4.2 規則：一拿到就升到 ceiling](#42-規則一拿到就升到-ceiling)
  - [4.3 定理：拿到第一個資源後就不會再被擋](#43-定理拿到第一個資源後就不會再被擋)
  - [4.4 缺點：inheritance-related inversion](#44-缺點inheritance-related-inversion)
  - [4.5 HLP 的阻擋時間](#45-hlp-的阻擋時間)
  - [4.6 誰支援 HLP](#46-誰支援-hlp)
- [5. 【實測】PIP / HLP 模擬器](#5-實測pip--hlp-模擬器)
- [6. 【原始碼】Linux 的 PIP：rt_mutex](#6-原始碼linux-的-piprt_mutex)
- [7. 【實測】Linux PI mutex 遇到 PIP 死結](#7-實測linux-pi-mutex-遇到-pip-死結)
- [8. 一頁速查表](#8-一頁速查表)
- [9. 與後續講次的連結](#9-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

解決 priority inversion 的兩個協定：

| 協定 | 一句話 | 解決 | 新問題 |
|---|---|---|---|
| **PIP** | 有人等我的資源時，我暫時繼承等待者中最高的優先權 | unbounded inversion | 死結、chain blocking |
| **HLP** | 一拿到資源，就升到這個資源的 ceiling（所有可能用它的任務中最高的優先權） | unbounded inversion、死結、chain blocking | inheritance-related inversion（中間任務被不必要地擋住） |

下一講的 **PCP** 要同時解決全部。

---

## 1. PIP：Priority Inheritance Protocol

### 1.1 想法

[00:30]–[02:30]

- 持有資源的低優先權任務 T_L **不能被搶走資源**，所以應該讓它**盡快做完**、釋放資源。
- 怎麼讓它盡快：**提高它的優先權**，讓其他（中間優先權的）任務不能從 CPU 搶占它。
  - T_L 需要的不只是資源，還有 CPU。被搶走 CPU，它就沒辦法用完資源。
- 提高多少？提高到**它擋住的那個任務**的優先權。T_L 優先權 x、被擋的任務 y > x → 把 T_L 升到 y，讓它拿到 y 會拿到的 CPU。

（老師開頭說「回去看前幾講講過的 priority inheritance」——指上一講 Pathfinder 的部分。）

### 1.2 規則

[02:30]–[05:31]、[13:01]–[14:00]

- 只有資源**被佔用**時才會啟動。
- 要鎖資源的請求放在**FIFO 佇列**。
- T_H 因為 T_L 持有資源而阻塞 → **繼承**條款啟動：T_L 的優先權升到**佇列中最高**的優先權。
- 只升到等待者中的最高，不會更高 → 不會對其他**不等這個資源**的更高優先權任務佔便宜。
- T_L 釋放資源 → **回到原本的優先權**。
- T_L 同時持有多個資源（例如還有 R2）→ 看 R2 的佇列裡最高的優先權，升到那個值。

```
priority
   │      ┌──────────┐   ← 繼承到等待者的優先權
   │      │          │
   └──────┘          └──── ← 釋放後回到原本
          ↑有人等      ↑釋放資源
```

課本 p.78 的虛擬碼：

```
if the required resource is free then
      grant it.
if the required resource is being held by a higher priority task then
      wait for the resource
if the required resource is held by a lower priority task then
      {
            wait for the resource
            the low priority task holding the
            resource acquires the highest priority of
            tasks waiting for the resource.
      }
```

老師的圖【口頭】[16:30]：Tᵢ 持有資源、優先權 5；Tⱼ 優先權 10（數字越大越高）。Tⱼ 要資源 → Tᵢ 繼承 10 → Tⱼ 等 Tᵢ 做完 → Tⱼ 拿到資源開始跑。這仍是一次 priority inversion，但砍掉了 unbounded 的問題。

【課本補充】（p.77–78）PIP 由 **Sha 與 Rajkumar** 提出。RTOS 設計者不難支援它，大部分商用 RTOS 都支援。

### 1.3 Kernel 裡的 lock / unlock 虛擬碼

[05:31]–[13:01]

**傳統 lock**：

```
disable interrupts                 // 不能被打斷，否則資料結構會壞
if (counter > 0)                   // 資源空著（counter 允許多份副本，通常只在 0、1 之間）
    counter--; holder = current; 把資源加到 current 的資源清單
else
    save context; current.state = BLOCKED
    insert current into lock's queue
    schedule(); load context of next task
enable interrupts
```

**PIP lock**：資源忙時多一步——

```
    ...
    insert current into lock's queue (sorted by priority, max first)
    holder.priority = max priority in the queue      ← 關鍵的一步：繼承
    schedule(); load context
```

**傳統 unlock**：

```
disable interrupts
if (queue empty) counter++
else
    next_to_run = highest-priority task in queue
    next_to_run.state = READY; 放進 ready queue
    save context; schedule(); load context
enable interrupts
```

**PIP unlock**：多一步——

```
    current.priority = 拿這個資源之前的優先權     ← 還原
    next_to_run = queue 的第一個（最高優先權）
    next_to_run 成為 holder；把資源加入它的清單；放進 ready queue
```

### 1.4 Inheritance blocking：公平的取捨

[14:00]–[16:30]

- PIP 確實防止了 unbounded priority inversion：持有者的優先權被拉高，會盡快做完。
- 但**鐘擺可能擺太過頭**：中間優先權任務本來可以跑，現在被一個「暫時被拉高」的低優先權任務擋住 → **inheritance blocking（繼承阻擋）**。
- 沒辦法對每個人都公平：

```
對高優先權任務公平  ←────────────→  對中間優先權任務公平
（inheritance blocking 多）          （unbounded inversion）
```

- 可以把 unbounded inversion 降到 0，但 inheritance blocking 就會很多，執行緒會受苦 → 需要**平衡**。

---

## 2. PIP 的兩個缺點

[18:01]–[25:00]

> **PIP 容易發生 chain blocking，而且完全不防止死結。**（課本 p.79）

### 2.1 死結

[18:32]–[20:03]

T1 優先權高於 T2。課本 p.79 的動作序列：

```
T1: Lock CR1, Lock CR2, Unlock CR2, Unlock CR1
T2: Lock CR2, Lock CR1, Unlock CR1, Unlock CR2
```

1. T2 先跑（T1 還沒到），鎖 CR2。
2. T1 到達，搶占 T2，鎖 CR1。
3. T1 要鎖 CR2 → T2 持有 → T1 阻塞，T2 繼承 T1 的優先權。
4. T2 繼續跑，要鎖 CR1 → T1 持有 → T2 也阻塞。
5. **循環等待 → 死結**。PIP 對此什麼都沒做。

### 2.2 Chain blocking

[20:03]–[25:00]

> 一個任務**每次**要一個資源都遇到一次 priority inversion → **chain blocking（連鎖阻擋）**。要 n 個資源，最多被擋 n 次。

老師的例子：T1 最高優先權，依序要 R2、R3、R4；T2、T3、T4（低優先權）分別先拿了 R2、R3、R4。

```
T1: local → 要 R2（T2 持有，T2 繼承、做完、釋放）→ 拿到 R2
          → 要 R3（T3 持有……）→ 拿到 R3
          → 要 R4（T4 持有……）→ 拿到 R4
```

- 資源不能搶占，T1 稍微等一下還說得過去；但要 10 個資源就被擋 10 次，「這不公平」，可能錯過 deadline。

---

## 3. PIP 的阻擋時間

[25:00]–[31:00]

前提（老師一再強調）：**不考慮優先權比 Tₐ 高的任務**——被它們延遲是理所當然，「沒有理由抱怨」。

**定理 1（單一資源）**：Tₐ 可能被 k 個低優先權任務 T₁…Tₖ 擋住（因為共享一個資源）。最長阻擋時間：

> **B ≤ max_{i ∈ 1..k} eᵢ**（eᵢ = 低優先權任務 i 使用該資源的時間）

- 最壞情況：Tₐ 在最不巧的時刻到達，要等那個低優先權任務用完。
- 沒有 unbounded inversion：低優先權任務會一直跑（除非被比 Tₐ 高的任務搶占），做完後 Tₐ 是最高的，馬上拿到資源。
- 關鍵：**同一個資源，Tₐ 不會被擋兩次**——低優先權任務一放手，Tₐ 就是最高的，沒有別人能插進來。

**定理 2（k 個資源）**：

> **B ≤ Σ_{j=1}^{k} max eⱼ**（對每個資源 j，取「優先權低於 Tₐ、會用資源 j 的任務」中最長的使用時間，再加總）

- max：每個資源只被擋一次，取最壞的那個。
- Σ：k 個資源各擋一次 → 這就是 chain blocking。

老師接著放了四題練習（「需要像追鹿的獅子那樣的動力」），只停留幾秒讓學生截圖，沒有講解。題目在投影片上，逐字稿沒有內容，這裡不附。

---

## 4. HLP：Highest Locker Protocol

### 4.1 Ceiling priority

[32:12]–[33:30]

- 假設：設計系統時，**每個資源會被哪些任務請求、它們的優先權**都已知（靜態優先權演算法正好適合：優先權綁在任務上、不會變）。
- **Ceiling priority（天花板優先權）**：資源 S 的 ceiling = 所有**可能**請求 S 的任務中**最高**的優先權。
- Ceiling 是**每個資源各自的**。那些任務不一定每次執行都會用到它，只要**有可能**用到就算。

【課本補充】（p.80）ceiling 的算法依同等級排程規則與優先權號碼方向而不同：

| | 數字越大越高（如 Windows） | 數字越大越低（如 Unix） |
|---|---|---|
| 同等級 FCFS | Ceil(Rᵢ) = max{pri(Tⱼ) : Tⱼ 需要 Rᵢ}（式 3.1） | min{…}（式 3.2） |
| 同等級時間片 RR | max{…} **+ 1**（式 3.3） | min{…} − 1 ⚠️ |

老師 [35:00] 也說：用 round robin 這類沒有自然順序的公平排程時，要再加 1（數字越大越高時）；否則減 1。

⚠️ 課本式 3.4（數字越大越低、RR）寫成 min{…} **+ 1**。照道理「數字越大越低」時要再高一級應是 **−1**（老師也這樣說）。課本的 +1 應是筆誤。

課本例子（FCFS、數字越大越高）：CR1 被 T1(10)、T5(5)、T7(2) 共用 → Ceil(CR1) = 10；CR2 被 T5、T7 共用 → Ceil(CR2) = 5。同時持有兩者 → 取較大的 10。

### 4.2 規則：一拿到就升到 ceiling

[33:30]–[37:02]

> **任務一拿到資源，優先權就升到該資源的 ceiling。** 釋放後還原。

- 只有一行，但很強大。
- 持有多個資源 → 升到這些資源 ceiling 中最高的（課本）。

老師的例子（**數字越小優先權越高**）：三個任務優先權 2、5、8，最高是 2。

- 號碼方向因 OS 而異【口頭】：老師說 Windows 是一種、Unix/Linux 是另一種，協定本身不受影響，只要講清楚就好。

【課本補充】（p.81）HLP **不需要等待佇列**：PIP 要為每個資源維護等待佇列；HLP 一拿到資源就以 ceiling 執行，其他可能要這個資源的任務根本沒機會跑去請求它。

### 4.3 定理：拿到第一個資源後就不會再被擋

[42:02]–[49:30]（課本 Theorem 1，p.80）

> **HLP 下，任務一旦拿到它需要的某個資源，就不會再被擋。**

證明（老師的版本）：Tᵢ 要 R1、R2、R3……，剛拿到 R1，優先權升到 Ceil(R1) ≥ pri(Tᵢ)。

- **情況 1**：R2、R3 都空著 → 沒問題，Tᵢ 繼續跑、拿資源、做完。
- 之後有比 Tᵢ（升高後）更高優先權的任務來拿走 R2 → Tᵢ 被擋，但對方優先權本來就比較高，「沒有理由抱怨」。
- **情況 2**（重點）：假設在 Tᵢ 拿 R1 **之前**，R2 已經被 Tⱼ 拿走。
  - Tⱼ 持有 R2 → Tⱼ 的優先權 = Ceil(R2)。
  - Tᵢ 也會用 R2 → Ceil(R2) ≥ pri(Tᵢ)。
  - 所以 Tⱼ 的優先權 ≥ Tᵢ → 應該是 Tⱼ 在跑，不是 Tᵢ。
  - 但 Tᵢ 正在跑（剛拿到 R1）→ **矛盾** → R2 不可能被別人持有。
- 對 R3…Rₖ 同理 → Tᵢ 拿到 R1 時，其他它需要的資源**全都空著**。

**推論**：

1. **Corollary 1**：HLP 下，任務拿到一個資源之前，它可能需要的**所有**資源都必須是空的。
2. **Corollary 2**：HLP 下**不會發生 chain blocking**。
3. **不會死結**：拿到資源後，高優先權任務永遠不會等低優先權任務；而死結的循環裡，優先權一路往上，最後一定要有「高等低」才能閉合 → 不可能。
4. 自然也沒有 unbounded priority inversion。

> 「unbounded priority inversion、死結、chain blocking，所有的敵人都被 HLP 消滅了。」

【課本補充】（p.80）

- 「只被擋一次」叫 **single blocking**。但拿到資源後，仍可能被**不共享資源**的更高優先權任務從 CPU 搶占——single blocking 指的只是**資源共享**造成的阻擋。
- 死結和 chain blocking 的結論（HLP 和 PCP 都是）有一個前提：任務**釋放資源後不再取得新資源**。也就是先取得所有需要的資源、再釋放，分成兩個階段。

⚠️ 課本證明裡說「Ceil(CR2) 比所有使用它的任務的最高優先權**多 1**」。在它自己定義的 FCFS 情況（式 3.1），ceiling 等於最高優先權，沒有 +1；+1 是 RR 的情況。證明只需要 Ceil(CR2) ≥ pri(T1)，結論不受影響。

### 4.4 缺點：inheritance-related inversion

[38:01]–[41:40]、[49:30]–[50:30]

- 唯一真正的問題（所以實務上**不太用**）：**inheritance-related inversion（繼承相關的反轉）**。
- PIP 只升到「現在**正在等**的任務」中最高的。
- HLP 升到「**可能**用這個資源的任務」中最高的——就算那個任務這次根本不會來。
  - 例：某個優先權 1000 的任務「可能」會用到這個資源（看執行時的值）→ 持有者一拿到資源就被拉到 1000 → 霸佔 CPU。
  - 跟這個資源**毫無關係**的中間任務全被擋住。
- 太保守 → 大量 inheritance blocking；持有者的優先權常常「高得不合理」。

老師的例子（數字越小越高）：T1 優先權介於 T2、T3 之間，拿到資源 → 不管誰在等，立刻升到 ceiling **2**。之後 T2 要資源只能等（FCFS）；優先權 3、4 的任務（基本優先權比 T1 高）也被擋——它們根本不要這個資源。

【課本補充】（p.81）T1–T5 優先權 1–5（數字越大越高）。T1、T2、T3 用 CR1 → Ceil(CR1) = 3；T1、T4、T5 用 CR2 → Ceil(CR2) = 5。T1 拿到 CR2 → 優先權變 5 → T2、T3 就算就緒也不能跑：它們遭受 inheritance-related inversion。課本：HLP 在實際應用中**很少用**，因為這個問題常嚴重到讓協定無法使用；但它是理解 PCP 的基礎。

### 4.5 HLP 的阻擋時間

[50:30]–[52:32]

記號：CS_{k,S} = 任務 k 使用資源 S 的 critical section 時間；USES(S) = 可能使用 S 的任務集合。任務 i 的最長阻擋時間：

> **Bᵢ = max { CS_{k,S} : i, k ∈ USES(S)，pri(k) < pri(i) }**

- 任務第一次要資源時，可能剛好有低優先權任務拿著某個資源 → 被擋一次。取所有這種情況中最長的 critical section。
- 拿到第一個資源後不會再被擋 → **只有一個 max，沒有 Σ**。
- 對比 PIP 的 Σ max：那個 Σ 正好代表 chain blocking。

⚠️ 投影片公式逐字稿沒唸清楚，這裡依老師的口頭描述（「一個 max、沒有加總」）寫出。嚴格來說 HLP 的阻擋來源還包括「低優先權任務以 ceiling ≥ pri(i) 執行」造成的 inheritance blocking，即使 i 不用那個資源；老師的式子只列了 i、k 都使用 S 的情況。

### 4.6 誰支援 HLP

[52:32]–[53:53]

- 老師：POSIX thread API 支援；舊的 Ada 支援；Real-Time Specification for Java 稱它為 **priority ceiling emulation**（MonitorControl 的子類別）。
- 老師說「**Linux 不支援**」。

⚠️ 這要分兩層看：

- **Kernel 層**：v6.2.12 確實沒有 ceiling 協定。futex 只有 PI 版本（`FUTEX_LOCK_PI`、`FUTEX_UNLOCK_PI`，`include/uapi/linux/futex.h:17`），`kernel/` 和 `include/` 裡找不到 priority ceiling 的實作。
- **使用者空間**：glibc 有實作 POSIX 的 `PTHREAD_PRIO_PROTECT`（就是 HLP）：拿鎖時由 glibc 自己把執行緒優先權設成 ceiling。OS 課（`../linux-os/` 第 28 講）用 strace 看到「一拿到鎖就把優先權拉到天花板」的系統呼叫模式。
- 所以「Linux 不支援」比較準確的說法是：kernel 沒有原生支援，glibc 在使用者空間模擬。

⚠️ 逐字稿 [53:02] 是「that is why hlp is **really used** in real applications」。照上下文（「中間優先權任務可能開始錯過 deadline，所以下一講看 PCP」）和課本「HLP is rarely used」，應是「**not** really used」，字幕漏字。

---

## 5. 【實測】PIP / HLP 模擬器

寫一個單 CPU 的 Python 模擬器：任務有釋出時間、基本優先權（數字越大越高）和腳本（`("c", n)` 計算 n 單位、`("L", R)` 鎖、`("U", R)` 解鎖）。協定只差在「有效優先權」怎麼算：

```python
def eff(n):                                   # effective priority
    p = tasks[n][1]
    if proto == "hlp" and st[n]["held"]: p = max([p] + [ceil[r] for r in st[n]["held"]])
    if proto in ("pip", "pcp"):
        for m in tasks:                       # inherit from tasks blocked on something I hold
            if st[m]["blocked_on"] and owner.get(st[m]["blocked_on"]) == n: p = max(p, eff(m))
    return p
```

圖例：`=` 執行、`#` 持有資源時執行、`b` 阻塞、`.` 就緒但沒跑、`X` 死結。

**1. Unbounded inversion**：L(1) 持有 R 4 單位；H(9) 在 t=1 到達、要 R；M1(5)、M2(4)、M3(3) 不要 R。

```
--- unbounded inversion [none] ok
  H   p=9   =bbbbbbbbbbbbbbb##          finish=19 resp=18
  M1  p=5    ====                       finish=6 resp=4
  M2  p=4     ...====                   finish=10 resp=7
  M3  p=3      ......====               finish=14 resp=10
  L   p=1  #.............###            finish=17 resp=17
--- unbounded inversion [pip] ok
  H   p=9   =bbb##                      finish=7 resp=6
  ...
  L   p=1  #.###                        finish=5 resp=5
--- unbounded inversion [hlp] ok
  H   p=9   ...=##                      finish=7 resp=6
  ...
  L   p=1  ####                         finish=4 resp=4
```

- 沒有協定：H 被三個中間任務一路擠，回應時間 18。
- PIP：L 繼承 9，中間任務插不進來，H 回應時間 6。
- HLP：L 一拿到 R 就是 9，H 到達時甚至還沒開始跑（`...`），同樣 6。

**2. 死結**（課本 p.79 的序列）：

```
--- deadlock [pip] DEADLOCK at t=5
  T1  p=9    =#bX
  T2  p=1  ##..#X
--- deadlock [hlp] ok
  T1  p=9    ..=##          finish=7 resp=5
  T2  p=1  ####             finish=4 resp=4
```

- PIP：t=5 時兩個都阻塞 → 死結。
- HLP：T2 一拿 CR2 就升到 9（CR2 的 ceiling），T1 到達後搶不到 CPU，T2 一口氣做完兩個資源 → 沒有死結。

**3. Chain blocking**：T1(9) 依序要 R2、R3、R4；T2、T3、T4 各先拿了一個。

```
--- chain blocking [pip] ok
  T1  p=9     =bb#bb#bb#        finish=13 resp=10
--- chain blocking [hlp] ok
  T1  p=9     =###              finish=7 resp=4
```

- PIP：T1 被擋**三次**（`bb` × 3）。
- HLP：T4 一拿 R4 就升到 9、跑完，T2、T3 根本沒機會先拿資源；T1 到達時資源全空 → **一次都沒擋**（Corollary 1）。

**4. Inheritance-related inversion**：T1(2) 持有 CR；CR 的 ceiling 是 9，因為 T9「可能」用它（T9 到 t=20 才來）；T5、T6 不用 CR。

```
--- inheritance blocking [pip] ok
  T6  p=6   ==                      finish=3 resp=2
  T5  p=5   ..===                   finish=6 resp=5
  T1  p=2  #.....####               finish=10 resp=10
--- inheritance blocking [hlp] ok
  T6  p=6   ....==                  finish=7 resp=6
  T5  p=5   ......===               finish=10 resp=9
  T1  p=2  #####                    finish=5 resp=5
```

- PIP：沒人在等 CR，T1 不升級，T5、T6 照常搶占它。
- HLP：T1 立刻升到 9，T5、T6 被擋（回應時間 2 → 6、5 → 9），而它們根本不用 CR——這就是 HLP 的缺點。

---

## 6. 【原始碼】Linux 的 PIP：rt_mutex

Linux 的 PI 實作在 rt_mutex。釋放或等待者改變時，持有者的優先權重新計算：

`kernel/locking/rtmutex.c:475`

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

對照：

| 本講 PIP | Linux |
|---|---|
| 升到等待佇列中最高的優先權 | `task_top_pi_waiter(p)`：所有在等 p 持有的鎖的任務中，優先權最高的 |
| 持有多個資源 → 看所有資源的佇列 | `pi_waiters` 是**每個任務**的樹，收集它持有的**所有**鎖的最高等待者 |
| 釋放後回到原本優先權 | 沒有 PI waiter 時 `pi_task = NULL` → `rt_mutex_setprio(p, NULL)` 回到 `normal_prio` |

PIP 不防死結，但 Linux 的 rt_mutex 會在**走訪鎖鏈**（為了把優先權一路傳下去）時順便**偵測**死結：

`kernel/locking/rtmutex.c:781`

```c
	/*
	 * [6] check_exit_conditions_2() protected by task->pi_lock and
	 * lock->wait_lock.
	 *
	 * Deadlock detection. If the lock is the same as the original
	 * lock which caused us to walk the lock chain or if the
	 * current lock is owned by the task which initiated the chain
	 * walk, we detected a deadlock.
	 */
	if (lock == orig_lock || rt_mutex_owner(lock) == top_task) {
		ret = -EDEADLK;
```

- 傳遞優先權本來就要沿著「等誰 → 誰又在等誰」走，走回自己就是循環 → 死結。
- 這是**偵測**，不是**預防**：死結已經形成了，只是告訴呼叫者。

---

## 7. 【實測】Linux PI mutex 遇到 PIP 死結

用 `PTHREAD_PRIO_INHERIT` 的 mutex 照課本序列寫：T2 鎖 CR2 → 睡 → 鎖 CR1；T1 晚 50 ms 鎖 CR1 → 睡 → 鎖 CR2。

```c
static void *t2(void *a) {
  pthread_mutex_lock(&cr2); usleep(100000);
  int r = pthread_mutex_lock(&cr1);
  ...
static void *t1(void *a) {
  usleep(50000); pthread_mutex_lock(&cr1); usleep(100000);
  int r = pthread_mutex_lock(&cr2);
```

**一般（NORMAL）PI mutex**：

```
after 1 s: NORMAL PI mutex
```

兩個 `printf` 都沒印 → 兩個執行緒永遠卡住。strace：

```
futex(0x55775b49c040, FUTEX_LOCK_PI_PRIVATE, NULL) = -1 EDEADLK (Resource deadlock avoided)
```

- **kernel 偵測到了**（就是上面的 `-EDEADLK`），但 glibc 對 NORMAL mutex 的行為是照 POSIX「死結就卡住」，不回報給程式。

**ERRORCHECK PI mutex**：

```
pidead: pthread_mutex_lock.c:433: __pthread_mutex_lock_full: Assertion `e != EDEADLK || (kind != PTHREAD_MUTEX_ERRORCHECK_NP && kind != PTHREAD_MUTEX_RECURSIVE_NP)' failed.
```

- kernel 一樣回 `EDEADLK`，但本機 glibc 2.35 遇到這個情況直接 assertion 失敗、程式 abort（glibc 預期 ERRORCHECK 的 EDEADLK 只來自「自己重複鎖自己」，沒預期到兩個執行緒的循環）。

結論：**PIP 不防止死結**，在真實系統上也一樣；kernel 能偵測，但應用程式拿不到乾淨的錯誤回報。要預防，就要像 HLP / PCP 那樣在**授予**資源時就做檢查，或固定上鎖順序。

---

## 8. 一頁速查表

### 8.1 PIP

- 規則：資源被佔用時，持有者升到等待者中最高的優先權；釋放後還原。
- 解決：unbounded inversion。
- 問題：死結（不預防）、chain blocking（要 n 個資源最多擋 n 次）、也有 inheritance blocking。
- 阻擋時間：單一資源 max eᵢ；k 個資源 Σⱼ max eⱼ。

### 8.2 HLP

- Ceil(R) = 可能用 R 的任務中最高優先權（FCFS；RR 要再高一級）。
- 規則：一拿到 R 就升到 Ceil(R)；持有多個取最高。
- 定理：拿到第一個資源後不再被擋（拿的時候其他需要的資源一定都空著）。
- 解決：unbounded inversion、死結、chain blocking。
- 問題：inheritance-related inversion（升到「可能」的最高，太保守）→ 實務很少用。
- 阻擋時間：單一個 max，沒有 Σ。

### 8.3 Linux

- PIP：rt_mutex（`rt_mutex_adjust_prio` → 最高 PI waiter），走鎖鏈時偵測死結（`-EDEADLK`）。
- HLP：kernel 沒有；glibc 用 `PTHREAD_PRIO_PROTECT` 在使用者空間模擬。
- 實測：PI mutex 照課本序列真的會死結；kernel 回 EDEADLK，但 NORMAL mutex 卡住、ERRORCHECK 在 glibc 2.35 assertion abort。

---

## 9. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **PIP 與 HLP 的缺點 → 同時解決** | **第 12 支**：The PCP Protocol |
| **阻擋時間代入可排程性分析** | **第 13 支**：Analysis of the PCP Protocol |
| **POSIX PRIO_INHERIT / PRIO_PROTECT** | **第 21 支**：POSIX standards |
| **商用 RTOS（VxWorks 等）對 PIP 的支援** | **第 22 支**、**第 24 支** |
| **Linux rt_mutex、PREEMPT_RT** | **第 23 支**：Linux-based RTOSs |

---

## 附：本講一句話總結

> **PIP 讓持有資源的低優先權任務在有人等待時暫時繼承等待者中最高的優先權、釋放後還原，因此中間任務插不進來，解決了 unbounded priority inversion，代價是中間任務的 inheritance blocking，而且它完全不防死結（T1 鎖 CR1→CR2、T2 鎖 CR2→CR1 就會卡死）也會 chain blocking（要 n 個資源最多被擋 n 次，阻擋時間是各資源最長 critical section 的總和）。HLP 事先為每個資源算出 ceiling（所有可能用它的任務中最高的優先權），任務一拿到資源就升到 ceiling；可以證明任務拿到第一個資源時其他需要的資源一定都空著，所以只會被擋一次、沒有 chain blocking、也不會死結，但「可能」用到就算的 ceiling 太保守，讓無關的中間任務被擋住（inheritance-related inversion），所以實務很少用。實測模擬器重現了這四種情況；Linux 的 rt_mutex 實作 PIP、會在走鎖鏈時偵測死結並回 EDEADLK，但用 PI mutex 跑課本的死結序列，程式仍然卡住或 abort；HLP 在 Linux 只有 glibc 的 PTHREAD_PRIO_PROTECT 在使用者空間模擬。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 12 錄影逐字稿（YouTube 自動字幕，53:53）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch3 §3–4 整理。原始碼為 Linux v6.2.12；實測為本機（Ubuntu、kernel 6.5.0、x86-64、glibc 2.35）結果。*
