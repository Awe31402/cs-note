# Real Time Systems 第八支影片完整筆記：RMA 的延伸（Lectures 8 & 9）

> **課程**：Real Time Systems — Lectures 8 and 9：Extensions of RMA Scheduling（播放清單第 8 支；一支影片含兩講，Lecture 9 從 [16:32] 開始）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（49:47，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch2 §9.2–10.2（PDF p.58–62）
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
- **Lecture 8**
- [1. DMA：deadline 小於週期時](#1-dmadeadline-小於週期時)
- [2. Context switch 的開銷](#2-context-switch-的開銷)
- [3. 【課本補充】Self suspension](#3-課本補充self-suspension)
- [4. 週期很長的關鍵任務：period transformation](#4-週期很長的關鍵任務period-transformation)
- [5. 任務之間的先後關係](#5-任務之間的先後關係)
  - [5.1 Table-driven：倒著排、盡量晚，再往前推](#51-table-driven倒著排盡量晚再往前推)
  - [5.2 Event-driven：前置任務完成才啟用](#52-event-driven前置任務完成才啟用)
  - [5.3 改釋出時間與 deadline 的「改裝」法](#53-改釋出時間與-deadline-的改裝法)
- **Lecture 9**
- [6. 非週期與偶發任務的問題](#6-非週期與偶發任務的問題)
- [7. Aperiodic server](#7-aperiodic-server)
  - [7.1 背景排程](#71-背景排程)
  - [7.2 Polling server](#72-polling-server)
  - [7.3 Deferrable server](#73-deferrable-server)
  - [7.4 Priority exchange](#74-priority-exchange)
  - [7.5 Sporadic server](#75-sporadic-server)
  - [7.6 動態：slack stealing](#76-動態slack-stealing)
  - [7.7 【實測】四種 server 的比較](#77-實測四種-server-的比較)
  - [7.8 【原始碼】Linux：沒有 SCHED_SPORADIC，但有 RT throttling 和 CBS](#78-原始碼linux沒有-sched_sporadic但有-rt-throttling-和-cbs)
- [8. 單處理器排程總結](#8-單處理器排程總結)
- [9. 一頁速查表](#9-一頁速查表)
- [10. 與後續講次的連結](#10-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

把 RMA 從理想模型推到實務：

- **Lecture 8**：d < p（DMA）、context switch 開銷、長週期的關鍵任務（period transformation）、任務的先後關係。
- **Lecture 9**：非週期與偶發任務 → **aperiodic server**：polling、deferrable、priority exchange、sporadic（靜態）以及 slack stealing（動態）。

---

## 1. DMA：deadline 小於週期時

[00:00]–[01:31]

- dᵢ < pᵢ 時，RMA **不再是最佳**的。
- **DMA（Deadline Monotonic Algorithm）**：也是靜態優先權，但依**相對 deadline** 給優先權：deadline 越短，優先權越高。
- DMA 在這個情況下是**最佳**的靜態優先權演算法。
- d = p 時，RMA 和 DMA 產生一樣的排程。
- d 任意時：RMA 失敗的，DMA 可能成功；DMA 失敗的，RMA 一定失敗。
- 老師不講 DMA 細節，只講動機（課程網站有連結）。

【課本補充】（p.58–59 Ex 2.12）T1 = (e=10, p=50, d=35)、T2 = (15, 100, 20)、T3 = (20, 200, 200)。

| | 優先權順序 | 檢查 | 結果 |
|---|---|---|---|
| RMA | T1 > T2 > T3（看週期） | T2：10 + 15 = 25 > 20 ✗ | 排不出 |
| DMA | T2 > T1 > T3（看 deadline） | T2：15 ≤ 20 ✓；T1：15 + 10 = 25 ≤ 35 ✓；T3：20 + 10 + 15 = 45 ≤ 200 ✓ | 排得出 |

（課本 T3 寫成 70 + 30 + 20 < 200，用的是 d = 200 內各任務出現次數的上界；結論相同。）

⚠️ 課本檢查 DMA 的 T1 時寫「(15 + 20) msec ≤ 20 msec」，數字和不等式都不對；照式 2.13 應是 e₁ + ⌈d₁/p₂⌉·e₂ = 10 + 15 = 25 ≤ 35（上表）。

---

## 2. Context switch 的開銷

[01:31]–[04:30]（課本 §9.3，p.59）

- 之前都忽略 context switch。實際上任務切換時，OS 要花一小段時間。
- 在 RMA 下，**每個任務最多造成兩次** context switch：
  1. 它開始跑時，搶占了正在跑的低優先權任務。
  2. 它結束時，切回被搶占的任務（或別的任務）。
- 如果它開始時沒有搶占別人（例如處理器閒著），可能只有一次。但算最壞情況要算兩次。
- 設 context switch 時間是常數 c，則每個任務的執行時間等於變成 **eᵢ + 2c**。之後照常用 Lehoczky 測試。

老師的例子：c = 1 ms，三個任務的執行時間 10 → 12、25 → 27、50 → 52，代入 Lehoczky 仍然滿足 → 可排。（週期與 deadline 的數值在投影片上，逐字稿沒有唸出，待補。）

【課本補充】Ex 13（p.59）：T1 = (20, 100)、T2 = (30, 150)、T3 = (90, 200)，c ≤ 1 ms。

- 加上 2c：U = 22/100 + 32/150 + 92/200 = **0.893** > 0.78 → Liu-Layland 不通過。
- Lehoczky：T1 = 22 < 100；T2 = 22·2 + 32 = 76 < 150；T3 = 22·2 + 32·2 + 92 = 200 ≤ 200 → 可排。

⚠️ 課本 T3 寫成「22·2 + 32·2 + **90** < 200」，忘了 T3 自己也要加 2c（應為 92，總和剛好 200）。用 92 時是 200 ≤ 200，剛好壓線，仍然滿足 ≤。

---

## 3. 【課本補充】Self suspension

（課本 §9.4–9.5，p.60–61；課堂沒講）

- 任務做 I/O 或等待某事件時會**自我暫停（self suspension）**：OS 把它從就緒佇列移到阻塞佇列，換下一個任務。
- 所以 event-driven 排程的排程點要再加一種：**任務完成、任務到達、自我暫停**。
- 設 Tᵢ 最壞自我暫停時間 bᵢ。Tᵢ 因自己和所有高優先權任務的暫停而多出的延遲：

  **btᵢ = bᵢ + Σₖ<ᵢ min(eₖ, bₖ)**（式 2.15）

  高優先權任務 Tₖ 暫停回來後，可能剛好在原本 Tᵢ 要跑的時候跑，最多多拖 eₖ；但若 eₖ > bₖ，最多只多拖 bₖ。
- Lehoczky 改成：**eᵢ + btᵢ + Σₖ<ᵢ ⌈pᵢ/pₖ⌉·eₖ ≤ pᵢ**（式 2.16）。
- 不可搶占的系統更糟：暫停時可能被不可搶占的低優先權任務卡住。
- 加上 context switch：每次自我暫停最多再多兩次切換，所以一次暫停時 eᵢ 換成 **eᵢ + 4c**。

---

## 4. 週期很長的關鍵任務：period transformation

[04:30]–[08:31]（課本 §10.1，p.61–62）

**問題**：

- RMA 依週期給優先權，**不看關鍵性**。
- 一個很關鍵、但週期很長的任務，優先權會很低、回應時間很長。
- 一個不重要但高優先權的任務暫時過載，可能就害它錯過 deadline。
- 直接把它的優先權調高，RMA 的可排程性結果就不成立了。

**Period transformation**（Sha 與 Rajkumar 提出）：

- 把關鍵任務 Tᵢ **在概念上**切成 k 個子任務，每個子任務：

  **Tᵢⱼ = ⟨eᵢ/k, pᵢ/k, dᵢ/k⟩**

- 不實際改程式：第 1 個 pᵢ/k 區間跑前 1/k，第 2 個區間跑下一個 1/k……
- 週期變短 → 在 RMA 下優先權**變高**。

老師的例子：T2 是關鍵任務，e = 8 ms、deadline = 30 ms；T1 的 deadline = 20 ms。即使暫時過載也不希望 T2 錯過。把 T2 切成兩個子任務，各 4 ms，週期與 deadline 都是 15 ms → T2 的優先權高過 T1。

【口頭】

- 有時任務的前半段在回應時間上比較關鍵，類似中斷處理的 top half / bottom half。
- 不要有「一邊倒」的任務組合（有的很小、有的很大）：大的永遠在小的之後才跑，而且常被打斷。

【課本補充】（p.62）切開後 RMA 分析不再精確：子任務的使用率被算成 k·(eᵢ/k)/(pᵢ/k) = k·eᵢ/pᵢ，實際只有 eᵢ/pᵢ。所以分析說排不出時，可能其實排得出。

---

## 5. 任務之間的先後關係

[08:01]–[16:32]

到目前為止都假設任務**彼此獨立**。實際上常有依賴。

### 5.1 Table-driven：倒著排、盡量晚，再往前推

[08:31]–[13:02]

演算法：

1. 把任務按 **deadline 遞增**排好：T1、T2、…。
2. **從最右邊**（deadline 最晚的）開始掃描。
3. 找一個**還沒排、而且所有後繼任務都已經排好**的任務，把它排得**越晚越好**（as late as possible）。
4. 重複直到全部排完 → 得到一個可行排程：開頭有很多空檔，越往後越擠。
5. 最後把所有任務**往前推**（越早越好），但不能早於到達時間、不能改變順序 → 縮短回應時間。

- 為什麼倒著排：前置任務一定要在後繼任務之前，從尾巴開始排就自然滿足。
- 先「盡量晚」是把 slack 留在前面；再整體往前推，讓回應時間變短。

老師的例子：五個任務，按 deadline 排序為 T1、T3、T2、T5、T4（T4 deadline 最晚）。倒著排：T4 → T5 → T2 → T3 → T1，各自盡量晚、不違反先後關係；然後全部往前推。老師說這裡**不宣稱最佳**。（任務的數值與依賴圖在投影片上，逐字稿沒有唸出，待補。）

### 5.2 Event-driven：前置任務完成才啟用

[13:02]–[14:32]

- 不管 RMA 還是 EDF，原則一樣：**前置任務還沒完成，就不要啟用（enable）這個任務**。
- 前置任務完成時，檢查等待啟用的佇列，再依靜態或動態規則排程。
- 不需要事先知道所有任務（table-driven 需要）。

### 5.3 改釋出時間與 deadline 的「改裝」法

[14:32]–[16:32]

- 修改每個任務的**到達時間**和 **deadline**，讓任務不可能在它的前置任務之前開始。
  - 例：T1 要先於 T2 → 把 T2 的到達時間設在 T1 之後。
  - 也不能搶占自己的前置任務、或在它之前執行。
- 改完之後，傳統的 EDF / RMA 幾乎不用改就能用，它們甚至不知道任務之間有關係。
- 老師：這像是一種**改裝的 hack**，做得好很多情況可行，但實務上很難做。

---

## 6. 非週期與偶發任務的問題

[16:32]–[19:30]（Lecture 9 開始；課本 §10.2，p.62）

- 選項：給非週期 / 偶發任務**很高**或**很低**的優先權，看系統而定。
- 給**高**優先權：一陣偶發任務湧入就能讓系統過載。雖然 RMA 處理過載比較好，還是會有很多任務錯過 deadline；而且違反 RMA 的基本前提。
- 給**低**優先權：但有些偶發任務很關鍵（緊急事件）。
- 實際上偶發任務多半兩極：**高優先權的緊急事件**，或**不重要的背景工作**，很少在中間。
- 背景工作在過載時可以延後（本來就能忍受長回應時間）。
- 高優先權的：**把它們轉成週期任務** → 建一個 **aperiodic server**（非週期伺服器），它本身是週期任務，週期夠短以配得上高優先權，每次醒來處理期間到達的非週期任務。

---

## 7. Aperiodic server

[19:30]–[47:30]

**Aperiodic server** = 一個高優先權的週期任務 (Cₛ, Pₛ)，專門用來服務非週期任務。

- 兩層排程：第一層是週期任務（含 server）用 RMA；server 執行時，第二層用自己的規則（FIFO 或 EDF）挑非週期任務。
- Server 的週期和執行時間依非週期任務的特性決定。系統裡可以有一個或多個 server。
- Cₛ 叫 **budget（預算）**：server 每個週期最多能花多少時間服務非週期任務。

| 類別 | 種類 |
|---|---|
| 靜態 server | polling、deferrable、priority exchange、sporadic |
| 動態 server | slack stealer |

### 7.1 背景排程

[19:30]–[22:00]

- 最基本的：非週期任務只在**沒有週期任務可跑**時才跑。
- 週期任務在高優先權佇列（RMS）；非週期任務在低優先權佇列，有自己的排程器（FIFO、EDF……）。
- 非週期任務塞在週期排程的**空洞（hole）**裡跑。

### 7.2 Polling server

[23:01]–[30:31]

規則：

- Server 週期性被叫醒（依 RMS 的優先權）。
- 被叫醒時**沒有**非週期任務 → **自我暫停**，直到下一個週期。這段期間就算有任務到達，也「算了」，要等下一個週期。
- 有非週期任務 → 最多執行 Cₛ。
- 最簡單、好實作。

老師的例子：T1 = (e=1, p=4)、T2 = (2, 6)，server Tₛ = (2, 5)。

- t = 0 沒有非週期任務 → server 暫停到 t = 5。
- 中途到達的非週期任務只能等到 t = 5、t = 10、t = 15……才被服務。
- A2 只需要 1 單位，server 還剩 1；但之後來的任務仍要等下一個週期——「不太公平，但最簡單的演算法就是這樣」。

**可排程性分析**：把 server 當成一個週期任務 (Cₛ, Pₛ)，套 Liu-Layland：

> **Σ (Cᵢ/Pᵢ) + Cₛ/Pₛ ≤ (n+1)(2^{1/(n+1)} − 1)**

（充分條件。⚠️ 投影片公式逐字稿沒唸出，這裡依「server 視為多一個週期任務」的說法寫出。）

**非週期任務的保證**：非週期任務 a 在 rₐ 到達，需要 Cₐ，deadline Dₐ。

- 最壞情況：剛好在 server 週期開始**之後**到達 → 等到下一個週期才被服務。
- 若 Cₐ ≤ Cₛ，下一個週期內做完。最晚要到下一個週期結束，總共約 2Pₛ。
- 充分條件：**2Pₛ ≤ Dₐ**。
- Cₐ 任意時，回應時間上界：**Pₛ + ⌈Cₐ/Cₛ⌉·Pₛ**（先等一個週期，再需要 ⌈Cₐ/Cₛ⌉ 個週期）。

缺點：完成時間對很多應用來說**太長**。

### 7.3 Deferrable server

[30:31]–[37:01]

兩個關鍵字：**defer（延後）**、**維持優先權**。

- 和 polling 一樣週期性以高優先權執行，budget 在**每個週期開始時補滿**為 Cₛ。
- 不同：沒有非週期任務時**不丟掉** budget，而是**留著**（延後使用）。週期中途有非週期任務到達，**立刻**以 server 的優先權服務（只要還有 budget）。
- 目的：更好的非週期回應時間。

老師的例子：T1 = (1, 4)、T2 = (2, 6)、server (2, 5)。

- 一個需要 2 單位的非週期任務在週期中途到達 → **不用等到 t = 5**，處理器空著就立刻跑。
- 下一個週期又來一個 → 不用等到 t = 10，T1 跑完就跑（T1 週期 4 < server 週期 5，優先權比 server 高）。
- Budget 用完就只能等下一次補充。

【課本補充】（p.62）用「票（ticket）」來描述：deferrable server 的票**定期補充**，和實際用量無關。所以可能有一陣子把一堆任務送進排程器、沒任務時又累積票；這偏離了 RMA 嚴格的週期執行模型，導致保守的設計和較低的處理器使用率。（§7.7 實測會看到這個問題。）

### 7.4 Priority exchange

[34:31]–[42:31]

- Server 執行時若**沒有**非週期任務：高優先權的 server 把它的**優先權**和一個低優先權**週期任務**交換。
- 交換只持續 **Cₛ′**（server 剩下的 budget）。例：budget 每週期 5，已用 2，剩 3 → 低優先權週期任務以 server 的高優先權跑 3 單位。
- Server 的 budget 保留，但優先權降低；下一個週期開始時 budget 補滿、優先權恢復。
- 之後若有非週期任務，server 以**降低後的優先權**最多執行 Cₛ − e。

老師的例子（圖中顏色 = 優先權）：沒有非週期任務 → 低優先權週期任務 T_L 以 server 的優先權執行 Cₛ − e 單位，之後回到自己的（黃色、較低）優先權；之後來的非週期任務以黃色的低優先權執行最多 Cₛ − e。

評價：

- 對**週期**任務比較好 → 可排程性界限比較好、很多情況效能也比較好。
- 對**非週期**任務比 deferrable 差 → 回應時間比較差。
- 「你的份做完了、還有 budget，就把優先權和 budget 給我們」——一種折衷。
- 優先權動態改變，精確的可排程性分析比較複雜。

### 7.5 Sporadic server

[42:01]–[47:00]

- 改善非週期回應時間，而且**不降低週期任務的使用率界限**。
- 已被納入 **POSIX 標準**。
- 關鍵是不同的**補充（replenishment）規則**：budget 不在固定邊界補充，而是在**開始執行非週期任務之後滿一個週期**才補回用掉的量。

```
Deferrable：|---Ps---|---Ps---|        每個邊界補滿
              ↑用掉       ↑補
Sporadic：  ──↑開始用─────────↑補（開始用的時間 + Ps）
```

老師的例子：server (2, 8) 優先權最高；T1 = (3, 10)、T2 = (4, 15)。

- 一開始沒有非週期任務 → 像 deferrable 一樣保留 budget。
- 非週期任務在 t = 2 到達、要 2 單位 → 立刻跑，budget 用完。
- t = 7 又來一個，但 server 沒 budget。
- 補充時間 = 開始用的時間 2 + 週期 8 = **10** → t = 10 才補回，A2 在那時執行。

評價（老師）：

| Server | 對非週期任務 |
|---|---|
| Polling | 很差 |
| Deferrable | 一直「我要服務你」 |
| Priority exchange | 介於中間，偏差 |
| Sporadic | 介於 deferrable 和固定補充之間的折衷：不降 server 的優先權，但也不在下一個邊界立刻補充 |

【課本補充】（p.62）Sporadic server 的補充時間取決於**確切的用票時間**：一用票就設計時器，到期時補回用掉的票。這保證**兩次之間有最小間隔**，所以可以把它當成週期任務來分析、給適當的優先權。可排程的使用率比 deferrable 高、比較好分析，但實作比較複雜。

### 7.6 動態：slack stealing

[47:00]–[48:00]

- 不建立一個獨立的週期任務；建立一個**被動任務**，從週期任務**偷**處理時間。
- 週期任務保持原樣；這個小任務很聰明，看得到執行狀況，能在不讓任何 deadline 錯過的前提下偷時間給非週期 job。
- 動態 → 開銷比較大。
- 對週期任務是**硬保證**，對非週期任務是**軟保證**。

### 7.7 【實測】四種 server 的比較

寫一個 1 單位步長的 RM 模擬器，實作背景、polling、deferrable、sporadic 四種規則。核心差別：

```python
if kind in ("poll", "defer") and t % Ps == 0: budget = Cs              # 固定邊界補滿
if kind == "poll" and t % Ps == 0 and not queue: budget = 0           # polling：沒任務就丟掉
if kind == "spor":                                                    # sporadic：開始用之後 Ps 才補回用量
    budget += sum(x for rt, x in repl if rt == t)
...
if kind == "spor" and busy_start is not None and (...):
    repl.append((busy_start + Ps, used))
```

**實驗 1**：老師的設定 T1 = (1, 4)、T2 = (2, 6)、server (2, 5)；非週期任務（到達, 需求）= (2,2)、(7,1)、(12,2)、(21,1)。時間軸：數字 = 週期任務，`s` = server，`a` = 背景跑的非週期任務，`.` = 閒置。

```
    122a1a221a..122a1a221a..122.1.221...122.
  background aperiodic response times [4, 3, 6, 1]  avg=3.50  periodic misses=[]
    122.1ss212s.122s1s221...1s221.221...122.
  polling    aperiodic response times [5, 4, 6, 5]  avg=5.00  periodic misses=[]
    12ss122s12..1ss212221s..122.1.221...122.
  deferrable aperiodic response times [2, 1, 3, 1]  avg=1.75  periodic misses=[]
    12ss122s12..1ss212221s..122.1.221...122.
  sporadic   aperiodic response times [2, 1, 3, 1]  avg=1.75  periodic misses=[]
```

- Polling 最慢（平均 5.0）：t = 2 到達的任務要等到 t = 5。這裡甚至比背景排程（3.5）慢，因為系統負載輕、空洞很多。
- Deferrable 和 sporadic 一樣快（1.75）：到達就立刻服務。這組負載下兩者的補充時間點剛好不造成差異。

**實驗 2**：deferrable 的「連打兩次」問題。用程式搜尋到一組：週期任務 T1 = (1, 4)、T2 = (2, 7)，server (2, 6)；把 server 換成普通週期任務 (2, 6) 時整組 RM 可排。非週期任務 (2,2)、(14,1)、(15,3)。

```
    0123456789012345678901234567
    122.1.ss122.1.221.ss122.1ss.
  poll   resp=[6, 5, 12] misses=[]
    12ss12.212..1.ss12ss122.1...
  defer  resp=[2, 1, 5] misses=[('T2', 21)]
    12ss12.212..1.ss122.1ss212..
  spor   resp=[2, 1, 8] misses=[]
```

- Deferrable：t = 14、15 用掉第 2 個週期的 budget，t = 18 一補滿又**立刻**用掉 18、19 → 6 個單位內 server 跑了 4 單位，比週期任務 (2, 6) 可能的最壞情況還多。T2 在 t = 14 釋出的 job（deadline 21）被擠掉 → **錯過**。
- Sporadic：t = 14 開始用，要到 14 + 6 = **20** 才補回 → server 的行為不會比週期任務 (2, 6) 更糟 → T2 沒錯過。代價是非週期任務回應時間 5 → 8。
- Polling：週期任務也安全，但非週期回應時間最差。
- 這就是為什麼 sporadic server「不降低週期任務的使用率界限」、可以當成週期任務來分析，而 deferrable 不行（課本說的「偏離 RMA 嚴格週期模型」）。

### 7.8 【原始碼】Linux：沒有 SCHED_SPORADIC，但有 RT throttling 和 CBS

POSIX 定義了 `SCHED_SPORADIC`，但 Linux v6.2.12 **沒有實作**：

`include/uapi/linux/sched.h:114`

```c
#define SCHED_NORMAL		0
#define SCHED_FIFO		1
#define SCHED_RR		2
#define SCHED_BATCH		3
/* SCHED_ISO: reserved but not implemented yet */
#define SCHED_IDLE		5
#define SCHED_DEADLINE		6
```

（`grep -rn SCHED_SPORADIC include/ kernel/` 沒有結果。）

Linux 有兩個類似「budget ＋ 補充」的機制：

**1. RT throttling**：整個即時排程類別（SCHED_FIFO / RR）每個週期最多能跑多久。

`kernel/sched/rt.c:998`

```c
static int sched_rt_runtime_exceeded(struct rt_rq *rt_rq)
{
	u64 runtime = sched_rt_runtime(rt_rq);

	if (rt_rq->rt_throttled)
		return rt_rq_throttled(rt_rq);

	if (runtime >= sched_rt_period(rt_rq))
		return 0;

	balance_runtime(rt_rq);
	runtime = sched_rt_runtime(rt_rq);
	if (runtime == RUNTIME_INF)
		return 0;

	if (rt_rq->rt_time > runtime) {
		struct rt_bandwidth *rt_b = sched_rt_bandwidth(rt_rq);

		/*
		 * Don't actually throttle groups that have no runtime assigned
		 * but accrue some time due to boosting.
		 */
		if (likely(rt_b->rt_runtime)) {
			rt_rq->rt_throttled = 1;
			printk_deferred_once("sched: RT throttling activated\n");
```

【實測】本機：

```
/proc/sys/kernel/sched_rt_period_us  = 1000000
/proc/sys/kernel/sched_rt_runtime_us = 950000
```

意思：每 1 秒，即時任務最多跑 0.95 秒，剩下 5% 留給一般任務。用完就節流（`rt_throttled = 1`），由週期計時器 `do_sched_rt_period_timer()`（`kernel/sched/rt.c:901`）在每個週期邊界補回——**固定邊界補充**，和 deferrable server 的 budget 規則同一類。只是它是用來「保護一般任務不被即時任務餓死」，方向和 aperiodic server 相反。

**2. SCHED_DEADLINE 的 CBS**（上一講）：每個任務有 (runtime, period) 的 budget，用完就節流到下一期——是動態優先權（EDF）世界裡的 server，可以拿來包住非週期工作。

---

## 8. 單處理器排程總結

[47:30]–[49:47]

- Clock-driven：table-driven、cyclic。
- Event-driven：**EDF**（動態優先權）和 **RMA**（靜態優先權）基本上涵蓋了整個領域。
- 延伸與特例：DMA、context switch、period transformation、先後關係，以及給 RMA 用的各種 aperiodic server（靜態或動態）。
- 下一個主題：任務之間有**資源依賴**（兩個任務要拿同一個資源，不能同時跑）→ 會產生死結（deadlock）等獨特問題。

---

## 9. 一頁速查表

### 9.1 Lecture 8

| 主題 | 重點 |
|---|---|
| DMA | d < p 時依 deadline 給靜態優先權；最佳；RMA 失敗它可能成功 |
| Context switch | 每任務最多 2 次 → eᵢ + 2c，再跑 Lehoczky |
| Self suspension（課本） | btᵢ = bᵢ + Σ min(eₖ, bₖ)；一次暫停 → eᵢ + 4c |
| Period transformation | 關鍵長週期任務切成 k 份 ⟨e/k, p/k, d/k⟩ 提高優先權 |
| 先後關係（table） | deadline 排序 → 從尾巴開始盡量晚 → 整體往前推 |
| 先後關係（event） | 前置任務完成才啟用；或改到達時間 / deadline |

### 9.2 Lecture 9：aperiodic server

| Server | Budget 規則 | 非週期回應 | 對週期任務 |
|---|---|---|---|
| 背景 | 只在空洞跑 | 看負載 | 無影響 |
| Polling | 週期開始若沒任務就丟掉 | 最差（最壞 ≈ Pₛ + ⌈Cₐ/Cₛ⌉Pₛ） | 等同週期任務 |
| Deferrable | 每週期補滿、保留到用完 | 好 | 可能「連打兩次」，界限變差 |
| Priority exchange | 沒任務時把優先權借給低優先權週期任務 | 偏差 | 好 |
| Sporadic（POSIX） | 開始用後 Pₛ 才補回用量 | 好 | 等同週期任務 |
| Slack stealing（動態） | 偷週期任務的 slack | 好 | 硬保證、開銷大 |

### 9.3 Linux

沒有 SCHED_SPORADIC。RT throttling：每 1 s 最多 0.95 s（固定邊界補充）。SCHED_DEADLINE：CBS budget。

---

## 10. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **優先權等級有限、jitter（課本 §10.3–10.4）** | **第 9 支**：Jitter and Limited Priority Levels |
| **資源依賴、死結** | **第 10–13 支**：Priority Inversion、PIP / HLP / PCP |
| **多處理器 / 分散式的先後關係與任務分配** | **第 14–15 支** |
| **POSIX 的 sporadic server** | **第 21 支**：POSIX standards |
| **商用 RTOS 的排程支援** | **第 22–25 支** |

---

## 附：本講一句話總結

> **把 RMA 用到實務要補很多洞：d < p 時改用依 deadline 給靜態優先權的 DMA（RMA 失敗它可能成功，反之不然）；context switch 讓每個任務最多多兩次切換，把 eᵢ 換成 eᵢ + 2c 再跑 Lehoczky（課本另有 self suspension 的 bᵢ 修正與 eᵢ + 4c）；週期長但很關鍵的任務可以用 period transformation 在概念上切成 k 份 ⟨e/k, p/k, d/k⟩ 來提高優先權；有先後關係時可以倒著從 deadline 最晚的任務開始盡量晚排、再整體往前推，或前置完成才啟用、或改到達時間與 deadline。非週期與偶發任務則交給 aperiodic server：polling 沒任務就丟掉 budget、回應最慢；deferrable 保留 budget 隨到隨服務、回應快但可能在週期邊界連打兩次而害週期任務錯過（實測重現 T2 錯過）；priority exchange 把優先權借給低優先權週期任務、偏向週期任務；sporadic server 在開始使用後滿一個週期才補回用量，回應快又能當成週期任務分析，所以進了 POSIX——不過 Linux 沒有實作 SCHED_SPORADIC，只有固定邊界補充的 RT throttling（本機每 1 s 最多 0.95 s）和 SCHED_DEADLINE 的 CBS。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lectures 8 & 9 錄影逐字稿（YouTube 自動字幕，49:47）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch2 §9.2–10.2 整理。原始碼為 Linux v6.2.12；實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
