# Real Time Systems 第二十九支影片完整筆記：流量特性化與整形（Lecture 30, Part I）

> **課程**：Real Time Systems — Lecture 30, Part I：Traffic Characterization and Shaping（播放清單第 29 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（37:12，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch6 §8–12（PDF p.156–163）
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
- [1. 網際網路上的即時通訊：合約](#1-網際網路上的即時通訊合約)
- [2. 流量特性化模型](#2-流量特性化模型)
- [3. QoS 框架的四塊](#3-qos-框架的四塊)
- [4. QoS routing](#4-qos-routing)
  - [4.1 三種限制](#41-三種限制)
  - [4.2 NP-complete 與實務](#42-np-complete-與實務)
- [5. Multicast routing](#5-multicast-routing)
- [6. RSVP](#6-rsvp)
- [7. Rate control：shaping vs policing](#7-rate-controlshaping-vs-policing)
- [8. Token bucket with leaky rate control](#8-token-bucket-with-leaky-rate-control)
- [9. 【原始碼】Linux 的 TBF（token bucket filter）](#9-原始碼linux-的-tbftoken-bucket-filter)
- [10. 【實測】token bucket 當 shaper 和 policer](#10-實測token-bucket-當-shaper-和-policer)
- [11. 一頁速查表](#11-一頁速查表)
- [12. 與後續講次的連結](#12-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

從 LAN 走到**封包交換網路**（大到網際網路）：

1. 傳送者和 ISP 之間要有**合約**：傳送者說明流量特性，ISP 保證 QoS。
2. 怎麼描述流量：(Xmin, Smax)、(r, T)、(Xmin, Xavg, Smax, I)、**(σ, ρ)**、多重速率界限。
3. QoS 框架四塊：routing、admission control、rate control（service discipline）、QoS renegotiation。
4. QoS routing、multicast、**RSVP**（資源預約）。
5. Rate control：**shaping vs policing**、**token bucket**。

（Part II——service discipline——在下一支影片。）

---

## 1. 網際網路上的即時通訊：合約

[00:00]–[02:30]（課本 §8，p.156–157）

- 要給即時流量（語音、影片、視訊會議，甚至你正在看的這支影片）**優先待遇**。
- 節點和 **ISP** 之間要有**合約**：
  - ISP 提供 QoS 保證；
  - 傳送者保證傳送速率不超過某些界限；
  - 傳送者違約時，ISP 要有辦法處理這種不守規矩的節點。
- 主要手段：**traffic shaping** 和 **policing**（相對於上一講 LAN 的 global priority 和 bounded access）；交換器採用不同的排程政策，確保流量之間的公平；控制所有連線的輸入速率。
- 網際網路的 QoS 架構：**IntServ**、**DiffServ** 等。
- 建立連線時：傳送者說明**流量特性**，ISP 同意能支援，才建立連線、開始傳。

【課本補充】（p.156–157 Fig.15）

```
Host（sender）─ Edge router（ingress）─ core routers ─ Edge router（egress）─ Host（receiver）
```

- Ingress router：接在傳送端，是傳送者資料的第一個介面；實體鏈路可以是雙絞線、光纖、無線。資料經過 ISP 的 **core router**（ISP 端密度很高，大量資料匯集），再從 egress router 到接收者。
- 建立連線時傳送者提供兩個參數：**traffic specification** 和 **service specification**；ISP 據此檢查可能路徑上的資源，決定接受或拒絕，或找一條（unicast / multicast）有足夠資源的路徑。
- 保證只在**客戶流量不超出規格**時有效；超出時網路可以丟封包或重新整形。
- 要**保護**即時連線免受不守規矩（或惡意）的即時與非即時來源影響 → 對每條連線做 shaping / policing，或在節點用自動做到這點的排程演算法（Fair Queuing、Weighted Fair Queuing 等，下一講）。

---

## 2. 流量特性化模型

[02:30]–[08:01]（課本 §8.2，p.157–158）

為什麼要特性化：ISP 要保證就得**預留**網路資源；描述準確才能少預留、又不會預留太多閒置。

| 模型 | 定義 | 適合 |
|---|---|---|
| **(Xmin, Smax)** | 兩個封包的到達間隔**至少** Xmin；最大封包不超過 Smax；峰值速率 Smax/Xmin | CBR：界限很緊 |
| **(r, T)** | 時間切成長度 T 的 frame；任何長度 T 的區間產生不超過 r·T 位元 | CBR；也是限制頻寬 |
| **(Xmin, Xavg, Smax, I)** | 滿足 (Xmin, Smax)，而且任何長度 I 的區間內平均到達間隔 ≥ Xavg | VBR：同時限制**峰值** Smax/Xmin 和**平均** Smax/Xavg |
| **(σ, ρ)** | 任何長度 t 的區間產生的位元數 ≤ **σ + ρ·t** | **爆發式**來源 |
| **多重速率界限** {(r₁,T₁), (r₂,T₂), …}，T₁ < T₂ < … | 長度 I 的區間（Tᵢ₋₁ < I < Tᵢ）位元數 ≤ rᵢ·Tᵢ | 更準確地描述爆發式流量 |

**(σ, ρ) 的直覺**（老師：像 y = mx + c）：

```
bits
  │            ╱  斜率 ρ（長期平均速率）
  │          ╱
  │        ╱
σ ┤──────╱       ← 截距 σ：爆發量，和時間無關
  │
  └──────────────── t
```

t 很小時仍允許 σ 的量 → 可以描述很爆發的來源。

**多重速率**：平均的區間越長，允許的速率越低、越接近長期平均（老師：大數法則）。

【課本補充】（p.157–158）

- (Xmin, Smax) 對爆發式流量會造成非常保守的資源預留、使用率很低；(r, T) 也一樣。
- (σ, ρ)：σ 是最大爆發大小，ρ 是長期平均速率（= 長時間內產生的封包數 ÷ 長時間）。

⚠️ 課本 p.157 寫 (Xmin, Smax)：「兩個封包的到達間隔**永遠小於** Xmin」——應是**大於**（Xmin 是最小間隔）；老師說的「更大」正確。

⚠️ 老師說平均速率是「X **max** S max over X average」[05:32]，應是 **Smax / Xavg**（口誤）。

---

## 3. QoS 框架的四塊

[08:01]–[10:30]

```
 Ingress ────────── Core ────────── Egress
   │                 │
 Rate control     Routing、Admission control、Service discipline、QoS renegotiation
```

| 元件 | 工作 |
|---|---|
| **Routing** | 找一條路徑（下一節） |
| **Admission control** | 測試需要的資源是否夠：夠就建立連線，不夠就拒絕 |
| **Rate control** | 確保來源不違反它宣告的流量規格（(r, T)、(σ, ρ) 的參數） |
| **Service discipline** | 把資源分給連線，確保建立通道時做的預約真的被維持（下一講） |
| **QoS renegotiation** | 使用者想改 QoS，或網路出現新的擁塞時，重新協商 |

這些都需要上一節的流量模型。

---

## 4. QoS routing

[10:30]–[17:01]（課本 §9，p.158–159）

- 建立連線時找一條從傳送者到接收者的路徑；多個接收者 → unicast 變 **multicast**。
- 傳統路由（如**最短路徑**）用別的指標最佳化，選出的路徑不一定能滿足 QoS。
- **QoS routing / constraint-based routing**：
  - 目標：選出能滿足 QoS 的路徑；提高網路使用率。
  - 要看拓樸、各路由器的資源（佇列、緩衝）、鏈路、可用頻寬、演算法複雜度、限制條件。
  - 變數很多：跳數（hop count）、延遲、頻寬、jitter……

課本 p.159：QoS routing 可能找到一條**比較長但負載輕**的路徑，而不是很擠的最短路徑。

### 4.1 三種限制

令 d(i, j) 是鏈路 (i, j) 的某個限制量，路徑 P = (i, j, k, …, l, m)：

| 類型 | 路徑的值 | 例子 |
|---|---|---|
| **Additive（可加）** | d(P) = d(i,j) + d(j,k) + … + d(l,m) | 延遲、jitter、成本、跳數 |
| **Multiplicative（可乘）** | d(P) = d(i,j) × d(j,k) × … | **可靠度**（獨立事件機率相乘）；取 log 就變可加 |
| **Concave（凹）** | d(P) = min{d(i,j), d(j,k), …} | **頻寬**：鏈子只和最弱的一環一樣強 |

### 4.2 NP-complete 與實務

- 找滿足**兩個以上**可加和 / 或可乘限制的路徑是 **NP-complete**（老師：可以從 Hamiltonian cycle 等問題歸約）；找不到精確解，但有好的近似解。
- 這個證明假設：(1) 各限制**彼此獨立**；(2) 每條鏈路的延遲和 jitter 事先已知。circuit-switched 網路大致成立；**packet-switched** 網路中頻寬、延遲、jitter 並不獨立（相關）→ 不能照傳統意義相加，但仍可以加起來得到近似解。
- 可以用 **Bellman-Ford**、**Dijkstra** 以跳數、延遲、jitter 算路徑。
- **頻寬**是基本限制；**跳數**決定路由複雜度與資源使用；延遲和 jitter 被看作頻寬和跳數的衍生結果，不是主要因素。

課本：Wang 與 Crowcroft 證明了這個 NP-completeness；因為 packet-switched 網路中參數不獨立，Bellman-Ford 和延伸的 Dijkstra 演算法可以在多項式時間算出以跳數、延遲、jitter 為限制的路徑。

---

## 5. Multicast routing

[17:01]–[19:00]（課本 §10，p.159–160）

- 多媒體應用（老師的例子：Teams 上課，30 個學生都是接收者）。點對點連線不實際、浪費資源 → 用 **multicast tree**。
- 可能有多個來源（不同時說話：老師講完換學生報告）和多個目的地（**sink**）；所有接收者同時都在聽。
- Multicast 路由要找一棵以來源為根、涵蓋所有接收者的樹，而且每條從傳送者到接收者的路徑都要滿足 QoS。

兩種建樹方式：

| 方式 | 說明 |
|---|---|
| **Source-based** | 從每個來源建樹 |
| **Core-based** | 先選一個核心；之後的接收者接到最近的節點 |

【課本補充】（p.160–161）

- **DVMRP**：source-based；reverse path forwarding、pruning、grafting；交換 distance vector（目的地與跳數）。
- **MOSPF**：OSPF 的 multicast 延伸；每個路由器知道所有路由器上的群組成員 → 建最短路徑 multicast tree；大網路不易擴展。
- **CBT（core-based tree）**：接收者向核心送訂閱請求，中途路由器記住介面；來源送資料時，第一個在樹上的路由器複製到其他樹上的介面。
- **PIM**：DM（dense mode，假設成員密集，用 reverse path flooding）、SM（sparse mode，用 rendezvous point，路由器送明確的 join 訊息）。
- **QoS-based**：Kompella 等人的分散式啟發式演算法建受限的 Steiner tree（三階段訊息傳遞），通訊開銷很高、每個節點要維護全域狀態；Hou 等人的 QoS 延伸 CBT（在 join 時檢查 QoS）。

---

## 6. RSVP

[19:00]–[28:00]（課本 §11，p.162–163）

**Resource Reservation Protocol**：

- 1993 年設計（課本：Xerox PARC），能擴展到 multicast。
- **不建樹**：假設樹已經建好，**給定一棵樹，它預約資源**，並做**允入控制**決定（有沒有資源）。
- 為每個目的地維護一棵 **sink tree**：以**目的地**為根、連到所有來源的 spanning tree（相對於 source tree），從目的地往來源方向送控制訊息。
- 每個路由器維護兩種資訊：

| 狀態 | 內容 |
|---|---|
| **Path state** | 每個來源用的路徑；也用來維護目的地的 sink tree |
| **Reservation state** | 為各目的地預留的資源。老師：兩棵 sink tree 交會的路由器，把有限的緩衝空間分給兩條流量 |

- **Receiver-oriented**：每個路由器從 multicast tree 的每條**下游**鏈路收到預約訊息，只往**上游**送一個。

**兩種訊息**（都是控制訊息，不帶資料）：

| 訊息 | 誰送、往哪 | 作用 |
|---|---|---|
| **Path message** | **來源**送，路由器沿樹**往下**轉送 | 建立路徑；來源在開始傳之前先送第一個，之後（接收者集合變化等）**定期**送；帶流量規格（flow spec）；路由器收到就更新 path state |
| **Reservation request（Resv）** | **目的地**沿自己的 sink tree 送 | 路由器檢查下游鏈路能否提供所需預約（**緩衝空間和頻寬**）；不行就**拒絕**（回錯誤訊息給接收者） |

- 目的地收到某個它想接收的來源的 path message 時，才送第一個 Resv；之後**定期**送 Resv 維持預約，否則資源會被收回——這叫 **soft state**。

**開銷分析**：重傳和定期訊息都是開銷，但控制訊息很短，開銷比資料訊息低得多；可以合併不同主機的控制訊息、把控制訊息搭在資料訊息上（piggyback）、只在資訊改變時才送。

【課本補充】（p.163）

- RSVP 不定義允入測試，只假設路由器會做、RSVP 能和它互動。
- 定期訊息維持預約是 RSVP 和大多數其他預約協定的不同之處；soft state 適合動態的應用。
- **Filter**：一串來源名稱，只有這些來源的訊息流能用為該目的地預約的資源；沒有 filter 時，multicast 群組中所有來源都能用。

---

## 7. Rate control：shaping vs policing

[28:00]–[31:30]（課本 §12，p.163）

- 只靠允入控制和 service discipline 不夠：傳送者可能**意外或惡意**超過約定的速率。
- 傳送者和目的地約好流量；目的地已經沿 sink tree 為傳送者預留資源；傳送者一多送，整個協議就破了 → 要在**傳送端**做 rate control。

課本 Table 6.1：

| | **Shaping（整形）** | **Policing（監管）** |
|---|---|---|
| 目的 | 超過約定速率的封包**緩衝、排隊** | 超過約定速率的封包**丟掉**，不緩衝 |
| 處理爆發 | 用 leaky bucket **延遲**流量，達到平滑效果 | **傳遞**爆發，不做平滑 |
| 優點 | 避免因丟包而重傳 | 避免排隊延遲；網路裡不會造成擁塞和長佇列 |
| 缺點 | 排隊造成延遲；長佇列、可能擁塞 | 降低受影響流量的吞吐量（重傳） |

老師：policing 比較「反射式（knee-jerk）」。用哪個要看應用、網路類型和 QoS 需求。

---

## 8. Token bucket with leaky rate control

[31:30]–[36:30]（課本 §12.1，p.163）

```
        r tokens/s
            ↓
        ┌───────┐  最多 b 個
        │ ○○○○  │
        └───┬───┘
packets → [queue] → 有 token？拿一個、送出 → outgoing link
```

- 桶子最多裝 **b** 個 token；以 **r** 個 / 秒產生新 token；桶子未滿就加進去，滿了就丟掉新 token。
- 等待的封包**至少有一個 token** 才能送；送出後拿掉一個 token。
- token 不夠：
  - **shaper**：封包等到 token 夠；
  - **policer**：封包丟掉（或標記降級）；佇列滿了也只能丟。
- 可以有爆發：桶裡有 b 個 token 時可以一次用完；之後只能以 r 的速率送。
- 變形：封包要等到桶裡有一定數量的 token 才送——更好的 shaper。

> **任何長度 t 的區間進入網路的封包數 ≤ r·t + b**

- 最大爆發 = **b**；長期平均 = (r·t + b)/t → **r**（t → ∞）。
- 和 **(σ, ρ)** 模型一一對應：b ↔ σ、r ↔ ρ。

老師的總結：封包交換網路的四個關鍵元件——傳送者、接收者、途中的交換器（有固定延遲的部分和內部封包緩衝）、可以建立路徑並指定 service discipline（保留多少頻寬、路由器內的緩衝）；rate control 用 r·t + b 的模型（b 處理爆發、r 是固定速率）。這提供了某種 **bounded access**；在網際網路這麼大的分散式系統上還沒談 global priority。

課本：token 產生速率 r 限制了封包進入網路的長期平均速率；這個技術也可以同時監管峰值速率和長期平均速率。

---

## 9. 【原始碼】Linux 的 TBF（token bucket filter）

Linux 的 `tbf` qdisc 就是本講的 token bucket：

`net/sched/sch_tbf.c:31`

```
	Description.
	------------

	A data flow obeys TBF with rate R and depth B, if for any
	time interval t_i...t_f the number of transmitted bits
	does not exceed B + R*(t_f-t_i).

	Packetized version of this definition:
	The sequence of packets of sizes s_i served at moments t_i
	obeys TBF, if for any i<=k:

	s_i+....+s_k <= B + R*(t_k - t_i)
```

`net/sched/sch_tbf.c:281`（`tbf_dequeue()`）

```c
		now = ktime_get_ns();
		toks = min_t(s64, now - q->t_c, q->buffer);

		if (tbf_peak_present(q)) {
			ptoks = toks + q->ptokens;
			if (ptoks > q->mtu)
				ptoks = q->mtu;
			ptoks -= (s64) psched_l2t_ns(&q->peak, len);
		}
		toks += q->tokens;
		if (toks > q->buffer)
			toks = q->buffer;
		toks -= (s64) psched_l2t_ns(&q->rate, len);

		if ((toks|ptoks) >= 0) {
			skb = qdisc_dequeue_peeked(q->qdisc);
```

對照：

| 本講 | Linux TBF |
|---|---|
| 位元數 ≤ b + r·t（σ + ρ·t） | 註解原文：「不超過 B + R*(t_f − t_i)」 |
| 每秒產生 r 個 token、桶深 b | token 以**時間**（奈秒）計：`toks = now − t_c` 是累積的 token，上限 `q->buffer`（= B/R） |
| 有足夠 token 才送 | `toks -= psched_l2t_ns(&q->rate, len)`（送這個封包要花的時間）；`>= 0` 才 dequeue |
| 同時監管峰值與平均 | 第二個桶 `ptoks`：峰值速率 P、深度 = MTU（註解：「QoS requires two TBF」） |
| Shaper | token 不夠時不送、啟動 watchdog 計時器到時再試（註解 NOTES）；佇列滿才丟 |

Policer 版本是 `act_police`（`tc filter ... action police`），超過就丟或重新標記。（設定 tc 要 root，這裡只看原始碼。）

---

## 10. 【實測】token bucket 當 shaper 和 policer

Python 模擬：爆發式來源（每 4 秒中有 1 秒以約 40 pkt/s 送、3 秒安靜 → 平均約 10 pkt/s），token bucket r = 10 pkt/s、b = 20：

```python
tokens = min(b, tokens + r * dt)
...
if mode == "police":
    if tokens >= 1: tokens -= 1; sent.append(t); delays.append(0)
    else: dropped += 1
...
while mode == "shape" and q and tokens >= 1:
    tokens -= 1; a = q.pop(0); sent.append(t); delays.append(t - a)
```

```
source: 195 packets in 20 s (avg 9.8 pkt/s), bursts of ~40 pkt/s
shape : sent 195, dropped   0, max delay  1.52 s
        window 0.5 s: max  24 pkts  <=  b + r*w = 25
        window 2.0 s: max  39 pkts  <=  b + r*w = 40
        window 5.0 s: max  69 pkts  <=  b + r*w = 70
police: sent 145, dropped  50, max delay  0.00 s
        window 0.5 s: max  24 pkts  <=  b + r*w = 25
        window 2.0 s: max  29 pkts  <=  b + r*w = 40
        window 5.0 s: max  58 pkts  <=  b + r*w = 70
input itself, window 0.5 s: 26  vs b + r*w = 25.0
```

看到什麼：

- **輸入本身違約**：某 0.5 s 內有 26 個封包 > b + r·w = 25。
- **Shaping**：一個都不丟，但最多延遲 **1.52 s**（排隊）；輸出在每個視窗都 ≤ b + r·w。
- **Policing**：零延遲，但丟了 **50 個**（26%）；輸出也守住界限，而且長視窗的量比 shaping 少（丟掉的就沒了，要靠上層重傳）。
- 和課本 Table 6.1 一致：shaping 換來延遲、policing 換來吞吐量損失；兩者的輸出都滿足 (σ, ρ) = (b, r) 的界限。

---

## 11. 一頁速查表

| 主題 | 重點 |
|---|---|
| 合約 | 傳送者給 traffic spec ＋ service spec；ISP 允入並保證；違約就 shaping / policing |
| 模型 | (Xmin, Smax)：峰值 Smax/Xmin，CBR；(r, T)；(Xmin, Xavg, Smax, I)：峰值與平均，VBR；**(σ, ρ)：≤ σ + ρt，爆發**；多重速率 |
| 框架 | routing、admission control、rate control、service discipline、renegotiation |
| QoS routing | additive（延遲、jitter、跳數）、multiplicative（可靠度，取 log）、concave（頻寬 = min）；兩個以上可加 / 可乘 → NP-complete；實務以頻寬與跳數為主 |
| Multicast | source-based vs core-based；DVMRP、MOSPF、CBT、PIM（課本） |
| RSVP | 1993；不建樹、只預約；sink tree；path（來源往下）＋ Resv（目的地往上）；receiver-oriented；soft state 定期更新 |
| Shaping vs policing | 緩衝延遲、平滑 vs 丟棄、傳遞爆發 |
| Token bucket | r tokens/s、深度 b；≤ r·t + b；最大爆發 b、長期 r；= (σ, ρ) |
| Linux | `sch_tbf.c`：B + R·t；token 以奈秒計；雙桶（rate ＋ peak） |
| 實測 | 輸入 26 > 25；shaping 0 丟、延遲 1.52 s；policing 丟 50/195、零延遲 |

---

## 12. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **Service discipline：交換器怎麼排程封包（WFQ、EDF 類等）** | **第 30 支**：Service Disciplines |
| **即時資料** | **第 31 支**：Real Time Data Stores |

---

## 附：本講一句話總結

> **要在網際網路這種封包交換網路上做即時通訊，傳送者與 ISP 之間要有合約：傳送者在建立連線時用流量模型描述自己——(Xmin, Smax) 與 (r, T) 適合 CBR、(Xmin, Xavg, Smax, I) 同時限制峰值 Smax/Xmin 與平均 Smax/Xavg 適合 VBR、(σ, ρ) 以「任何長度 t 的區間不超過 σ + ρt」描述爆發式來源——ISP 據此做路由、允入控制、service discipline 與必要時的 QoS 重新協商。QoS routing 要處理可加（延遲、jitter、跳數）、可乘（可靠度，取 log 變可加）與凹型（頻寬取最小）限制，兩個以上可加或可乘限制同時存在時是 NP-complete，實務以頻寬與跳數為主；multicast 用 source-based 或 core-based 樹；RSVP 不建樹、只在給定的樹上預約資源：來源送 path 訊息往下建立路徑狀態，目的地沿自己的 sink tree 送 Resv 預約緩衝與頻寬，並定期更新成 soft state。為防止傳送者違約，傳送端要做 rate control：shaping 把超量封包緩衝延遲、policing 直接丟掉；最常用的 token bucket 以 r 個/秒產生 token、深度 b，保證任何長度 t 的區間不超過 rt + b，正好就是 (σ, ρ) 模型，也就是 Linux tbf qdisc 原始碼註解裡的 B + R·t。實測一個違約的爆發式來源經 token bucket 後，shaping 不丟包但最多延遲 1.52 秒，policing 零延遲但丟了四分之一的封包。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 30 Part I 錄影逐字稿（YouTube 自動字幕，37:12）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch6 §8–12 整理。原始碼為 Linux v6.2.12；實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
