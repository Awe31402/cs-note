# Real Time Systems 第三十支影片完整筆記：Service Disciplines 與 QoS 模型（Lecture 30, Part II）

> **課程**：Real Time Systems — Lecture 30, Part II：Service Disciplines（播放清單第 30 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（39:13，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch6 §13–14（PDF p.163–172）
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
- [1. Service discipline 是什麼](#1-service-discipline-是什麼)
- [2. Traffic distortion：流量越走越爆發](#2-traffic-distortion流量越走越爆發)
- [3. Work-conserving vs non-work-conserving](#3-work-conserving-vs-non-work-conserving)
- [4. 具體的 discipline](#4-具體的-discipline)
  - [4.1 多層 FCFS 佇列（靜態優先權）](#41-多層-fcfs-佇列靜態優先權)
  - [4.2 Fluid fair queuing 與 WFQ](#42-fluid-fair-queuing-與-wfq)
  - [4.3 Jitter-EDD](#43-jitter-edd)
  - [4.4 Stop-and-Go 與多層 framing](#44-stop-and-go-與多層-framing)
  - [4.5 RCSP：rate-controlled static priority](#45-rcsprate-controlled-static-priority)
- [5. QoS 模型：IntServ 與 DiffServ](#5-qos-模型intserv-與-diffserv)
  - [5.1 IntServ](#51-intserv)
  - [5.2 DiffServ](#52-diffserv)
- [6. 【實測】五跳網路：FCFS vs Stop-and-Go](#6-實測五跳網路fcfs-vs-stop-and-go)
- [7. 【原始碼】【實測】Linux：DRR 與 DSCP](#7-原始碼實測linuxdrr-與-dscp)
- [8. 一頁速查表](#8-一頁速查表)
- [9. 與後續講次的連結](#9-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

接 Part I 的 QoS 框架（routing、channel establishment、admission control、service discipline、QoS renegotiation），這支講 **service discipline**：

1. 交換器 / 路由器 = 佇列；service discipline = **封包排程器**（像 CPU 排程）。
2. 流量經過一個個交換器會越來越爆發（σ 變大）→ 要重建原本的特性。
3. Work-conserving（貪婪）vs **non-work-conserving**（封包有 eligibility time，早到也要等）。
4. 多層 FCFS、WFQ、Jitter-EDD、Stop-and-Go、**RCSP**。
5. 網際網路的 QoS 模型：**IntServ**（RSVP、每條流的狀態）與 **DiffServ**（標頭裡的 DSCP、複雜度推到網路邊緣）。

---

## 1. Service discipline 是什麼

[00:32]–[03:00]（課本 §13，p.163–164）

- 每個交換器 / 路由器可以看成一個**佇列**：不同來源–目的地流量的封包競爭同一條出口鏈路。
- 要一個**排程器**（像 CPU 排程器）依符合即時要求的政策挑封包送出——這個機制就叫 **service discipline**。
- 希望有 **rate-based** 的 service discipline，防止爆發和網路負載波動。

**問題的來源**：

- 不守規矩的來源：送得比約定的快。
- 不在這套機制裡的其他來源塞封包 → 負載波動、異常擁塞。
- **DoS 攻擊**。
- 來源守規矩，但途中被其他路由器的衝突流量弄亂。

【課本補充】（p.163–164）

- 交換器的緩衝需求取決於最大封包到達率、最大封包大小、封包預期的停留時間；接收端的緩衝需求 = delay jitter × 最大封包到達率 × 封包大小（上一講的 jitter buffer）。
- Service discipline 保證個別連線的**最低服務速率**，不受其他連線流量影響。

---

## 2. Traffic distortion：流量越走越爆發

[03:00]–[06:31]（課本 §13.1，p.164）

- 即使來源守規矩，每個交換器的排隊會讓同一個 session 的封包**擠在一起**（bunching）→ 越走越爆發，不再符合來源的特性。
- 來源滿足 (σ, ρ)，到第 i 個交換器前的最壞特性變成：

  > **(σ + Δσ, ρ)，Δσ = Σ_{j=1}^{i−1} ρ · d_j^max**

  d_j^max = 第 j 個交換器對這條連線的**局部延遲上界**。ρ（長期速率）由傳送者決定、不變；σ（爆發）變大。
- 老師的直覺：交換器把一些封包**延遲**，再一次送出 → 原本平均分布的封包變成一串——延遲時間 × 速率 = 多出來的爆發量。

**控制 distortion**：

- 在網路各處重建流量原本的特性（例如原本瞬時頻寬固定，途中變得忽高忽低，要把它還原）。
- 課本：所有 non-work-conserving discipline 都把 server 分成兩部分：
  1. **Rate controller**：用 **regulator** 控制每條連線的 distortion、分配頻寬。
  2. **Scheduler**：決定不同連線的封包傳送順序、提供每條連線的延遲上界（每個佇列一條流，在它們之間挑）。
- Service discipline 把守規矩的保證型流量和不守規矩的來源、網路負載波動、best-effort 流量**隔離**開來。

---

## 3. Work-conserving vs non-work-conserving

[06:31]–[09:31]（課本 §13.2，p.165–166）

| | Work-conserving（WCD） | Non-work-conserving（NWCD） |
|---|---|---|
| 規則 | 有封包要送，server **絕不閒著**；鏈路忙才排隊 | 每個封包有 **eligibility time**；沒有 eligible 的封包時，鏈路**可以閒著**，即使佇列裡有封包 |
| 精神 | 貪婪 → 吞吐量最大 | 早到的也不送，避免製造爆發 |
| 流量整形 | 沒有 | 有 |
| 緩衝 | 較少 | 較多 |
| 延遲 | 較短 | 較長 |
| Jitter 上界 | **給不出緊的** | **可以給緊的** |
| 例子（課本） | Virtual clock、WFQ、SCFQ、Delay-EDD | Jitter-EDD、Stop-and-Go、HRR、RCSP |

- 老師：在即時意義上，很多情況 non-work-conserving 被發現**更有效**——看似反常，其實正是 traffic shaping 比較好的原因。

⚠️ 老師說「HLP 等協定從不讓 CPU 閒著；而 PCP 即使有高優先權封包也先不送——這是 work conserving」[07:03]。照上下文，他想說 PCP（會故意延後授予資源）是 **non**-work-conserving 的類比；逐字稿的說法正好反了。

【課本補充】（p.165–166）

- 非 work-conserving 以前不受重視：(1) 以前主要的效能指標是**平均**延遲與平均吞吐量，把封包扣著會讓它們變差；(2) 排隊分析只考慮單一 server，沒看到它在網路中的好處。在保證型服務中，更重要的是**端到端延遲上界**、而且要在整個網路中有上界——那兩個理由就不成立了。
- 常被保證的四種流量性質：遺失率上界、最壞頻寬、排隊延遲上界、delay jitter。
- 單一交換器裡分配頻寬與延遲的三種做法：**多層 framing**（Stop-and-Go、HRR）、**sorted priority queue**（Delay-EDD、Jitter-EDD、virtual clock、WFQ、SCFQ；沒有 framing 的缺點，但插入是 O(log N)，對高速網路可能太慢）、**多層 FCFS 佇列**（靜態優先權，RCSP；能提供的延遲上界種類受限於優先權等級數）。

---

## 4. 具體的 discipline

### 4.1 多層 FCFS 佇列（靜態優先權）

[09:31]–[11:30]

```
 high   │■■■│ ──┐
 medium │■■ │ ──┼─→ 永遠先清最高的非空佇列
 low    │■■■│ ──┘
```

- 每個優先權等級一個 FCFS 佇列；每條連線指定一個優先權，封包放進對應佇列；多條連線可以同一優先權（同等級內可以 FCFS 或多層 round robin）。
- 永遠送最高的非空佇列 → 不同連線有不同程度的延遲上界，高優先權的上界較緊。
- 老師：和 Linux 即時排程的變體一樣（第 7 支影片的 `rt_prio_array`）。

### 4.2 Fluid fair queuing 與 WFQ

[11:30]–[17:30]（課本 §13.3，p.166）

- **FFQ（fluid fair queuing，也叫 PGPS）**：每條**連線**一個 FIFO 佇列（不是依優先權）。
- 恰好有 N 個非空佇列時，同時以 **1/N 的鏈路速度**服務 N 個佇列頭的封包——像 round robin。
- 可以給不同連線不同份額（例如 c1 優先就從它的佇列拿兩個、其他各一個）——像 Linux 把長時間區間依優先權分給任務。

> **第 i 個非空佇列的服務速率 = (φᵢ / Σ_{j∈B(τ)} φⱼ) × C**

- φ₁…φ_N 是正實數（份額，和優先權相關）；B(τ) 是此刻的非空佇列集合；C 是鏈路速度（頻寬）。
- 老師：和上一支影片 802.4 的 Hᵢ = (TTRT − θ) × (Cᵢ/Tᵢ)/Σ(Cⱼ/Tⱼ) 幾乎一樣——那裡的權重是頻寬，這裡的權重是優先權的代理。
- 兩個假設：能**同時**服務所有非空連線（這裡是**分頻寬**，不是分時間：10 MB/s 給兩條同優先權的流各 5 MB/s——CPU 做不到，多核才行）；流量**無限可分**（實際上要取整數）。
- **WFQ** 由 FFQ 演化而來：實際封包系統一次只能服務一條連線、一個封包要整個送完才能送下一個。

⚠️ 課本說 FFQ「不實際」；老師說他**不完全同意**：以今天的技術（尤其是 FPGA）是可行的，以前 10–15 年前的網路主要是分時間，現在 4G、5G 行動數據就是在做類似的頻寬分割。這是老師個人的看法。

### 4.3 Jitter-EDD

[17:30]–[18:30]（課本 p.167）

- **Non-work-conserving**。
- 封包在一個 server 服務完後，在標頭蓋上「deadline − 實際完成時間」（提早了多少，PreAhead）。
- 下一個 server 入口的 **regulator** 把封包**扣住**這麼久，才讓它 eligible。

```
Switch i−1: Arrival ── Departure ──(PreAhead)── Deadline
                          ╲                       ╲
Switch i:              Arrival ──(Holding)── Eligible ──(Delay bound)── Deadline
```

- 相鄰 server 的 eligibility time 之間延遲固定 → 封包流有 **delay-jitter 上界**；假設目的地沒有 regulator，端到端 jitter 上界 = 最後一個 server 的局部延遲上界（課本）。

### 4.4 Stop-and-Go 與多層 framing

[18:30]–[24:01]（課本 p.167–168）

**單層 framing**：

- 時間軸切成長度 **T** 的 frame（不同節點的 frame 不必同步）；每條連線分到 frame 時間的一部分（= 頻寬）。
- 每個交換器把**到達 frame** 對應到出口鏈路的**離開 frame**，中間有固定延遲 θ（0 ≤ θ < T）。
- 規則：在 frame f 到達的封包，**一定等到下一個 frame 開始**才送——就算有封包在排、server 也閒著等。
- 早到、稍晚到都沒差：時間被離散化，只在固定時刻送 → **大幅平滑**流量。
- 代價：額外緩衝、增加延遲。單一交換器的延遲最多**兩個 frame 時間**（運氣差：剛好在 frame 開頭到，要等到下下個 frame）。

**延遲與頻寬粒度的耦合**：

- 要延遲小 → T 小。
- 封包大小固定 P、每個 frame 送一個 → 頻寬分配的最小粒度 = **P/T** → 要分配得細 → T 大。
- 兩者**不能同時**達成。

**多層 framing**：

- n 層 frame 大小 T₁…Tₙ，T_{m+1} 是 T_m 的整數倍（課本 Fig.21：T₂ = 3 × T₁）。
- 第 p 層的連線用 frame 大小 T_p 的 Stop-and-Go 規則：在某個 T_p frame 到達的封包，要到下一個 T_p frame 開始才 eligible。
- 老師：各層彼此**調和**（回想 RMS 的調和週期，第 7 支影片），所以很好安排。
- 課本：frame 小的封包對 frame 大的封包有**不可搶占**的優先權；把要低延遲的連線放在小 frame、要細頻寬分配的放在大 frame。但每個 frame 內的耦合仍在；可以在網路入口加整形機制，讓多條連線共用一個 frame slot。

### 4.5 RCSP：rate-controlled static priority

[24:01]–[29:30]（課本 p.168–169）

- 前面的方法：sorted priority queue（WFQ）複雜難實作；framing（Stop-and-Go）有排隊延遲與頻寬粒度的依存；非 work-conserving 會增加延遲與緩衝、而且常假設能細分時間或頻寬。
- **RCSP**（Zhang 與 Ferrari 提出）：把 **rate control** 和 **delay control** **解耦**。

```
           Rate controller                         Scheduler
 input → [Regulator 1] ─┐                    ┌→ priority 1 │■■│ ─┐
         [Regulator 2] ─┼─ regulated traffic ┼→     …         ├→ output
         [Regulator h] ─┘                    └→ priority n │■ │ ─┘
```

| 元件 | 作用 |
|---|---|
| **Rate controller** | 每條連線一個 **regulator**，把輸入流量整形成想要的樣式；封包到達時算出 eligibility time，**扣住**到時間到（non-work-conserving），再交給 scheduler |
| **Scheduler** | **靜態優先權**、**不可搶占**；多層 FCFS 佇列：永遠送最高的非空佇列的第一個封包；每個優先權等級對應一個延遲上界；建立連線時指定優先權 |

兩種 regulator：

| | 作用 |
|---|---|
| **Rate-jitter（RJ）** | **部分**重建流量：輸出滿足 (Xmin, Xavg, I) 模型 |
| **Delay-jitter（DJ）** | **完全**重建流量：輸出和上一個 server 的 regulator 輸出**一模一樣** |

課本：若流量在網路入口滿足 (Xmin, Xavg, I)，兩種 regulator 都確保 scheduler 的輸入滿足同樣的特性；RCSP 容易實作、沒有其他排程器的問題，是比較好的演算法。

⚠️ 課本 p.169 描述 RCSP scheduler 的那一段**重複了兩次**（排版錯誤）。

老師的總結【口頭】[27:30]–[29:30]：service discipline 的關鍵是**不要用 work-conserving**，要刻意延遲、整形流量 → 控制爆發 → 控制擁塞 → 減少**丟包**。即時通訊最敏感的就是丟包（TCP 一遇到丟包就大幅降速）；只有確保流量分布、照著預留資源，才能避免丟包。整形之後再**強制優先權**（多層 FCFS）。

---

## 5. QoS 模型：IntServ 與 DiffServ

[29:30]–[38:32]（課本 §14，p.169–172）

這些在網路層級可以，但在**整個網際網路**需要另一層抽象來保證應用的 QoS。**IETF** 提出三個主要 QoS 模型：**IntServ**、**DiffServ**、**MPLS**（老師只講前兩個）。

### 5.1 IntServ

[30:00]–[33:30]

- 一個同時給即時與非即時流量的共同平台；延伸既有協定（第三層的 IP）。
- **每條流（per flow）**；不靜態預留，而是用 **RSVP** 動態預留（上一支影片）：流量開始前先把資源預留好。

兩種服務：

| 服務 | 保證 | 例子 |
|---|---|---|
| **Guaranteed** | 端到端**排隊延遲**有上界——寫在石頭上的絕對保證；可支援硬即時 | 遠距手術：醫生在 2000 公里外操控機器人，網路不能有 jitter |
| **Predictive** | 機率 / 統計保證 | 視訊會議等大多數應用夠用 |

**限制**：

- 流越多，狀態資訊越多（path state、reservation state）→ 路由器要很「聰明」、處理量大，在網際網路核心擴展不了。
- 所有路由器（不同廠商）都要合作：實作 RSVP、允入控制、依優先權分類封包（MF classification）、封包排程。
- 保證型服務需要**全面佈署**，很難**漸進**佈署：路上任何一個路由器不支援就不行。大型企業網路全在自己掌控下還好，跨很多私人 ISP 的網際網路就難了。

【課本補充】（p.170–171）

- IntServ 實作四個元件：**signaling protocol（RSVP）**、**admission control**、**classifier**、**packet scheduler**。允入通過後，封包到達時 classifier 分類、放進特定佇列，scheduler 依 QoS 排程。
- 路由器分兩部分：**forwarding path**（每個封包都要走，要高度最佳化，商用路由器多用硬體實作：input driver → Internet forwarder（解析 IP 標頭、分類）→ output driver（封包排程））和 **background code**（一般 CPU 執行：routing agent、reservation setup agent、network management agent）。
- Guaranteed 和 predictive 服務分別定義在 guaranteed service RFC 和 controlled load service RFC。
- 漸進佈署：Controlled-Load Service 可以只在網域的瓶頸節點佈署 RSVP 功能，其他部分以 tunnel 傳 RSVP 訊息。

### 5.2 DiffServ

[33:30]–[38:32]

- 被認為是 IntServ 的改良版、今天的主流：**更簡單、更可擴展**。
- 重新定義 IPv4 標頭的 **TOS（type of service）** 欄位 / IPv6 的 Traffic Class 位元組為 **DS（differentiated service）欄位**；前 **6 位元**是 **DSCP（DS code point）**：告訴路上每個節點要什麼服務——有點像**即時優先權**。
- 排程和緩衝管理需要的資訊**就在封包裡**：不需要特別的 signaling 協定；每個節點要維護的狀態和**服務類別數**成正比，不是和流的數量成正比。例：10 個服務類別，只要知道「這條流是第 7 類」，路上的路由器就照對待所有第 7 類的方式處理。

**設計原則：核心保持簡單，複雜度推到邊緣**：

- IntServ：用 RSVP 預留、每條流標記、路由器維護每條流的資訊。
- DiffServ：複雜處理放在網路**邊緣**（傳送端和接收端的主機與 edge router），網路核心大致不改。
- Edge router 處理的使用者有限（不像核心路由器要服務整個網際網路），由它們做複雜工作：**分類**封包（依規則蓋上優先權）、**traffic conditioning**（核心路由器已經很忙，不該給它們做）。大多在傳送端那邊做。

兩種服務模型（類似 IntServ 的 guaranteed / predictive）：

| 服務 | 說明 | 例子 |
|---|---|---|
| **Premium** | 在某個峰值速率下：只要流量樣式規則（傳送者或 edge router 整形守住約定）→ 幾乎沒有排隊延遲、可保證端到端上界 | **virtual leased line**（地理上相隔很遠的兩端像有專線）、遠距醫療、機器手術 |
| **Assured** | 統計保證 | 網路影片廣播、視訊會議 |

【課本補充】（p.172）

- 客戶要和 ISP 有 **SLA（service level agreement）**：規定支援的服務類別和每類允許的流量；可以是靜態（每月、每年協商）或動態（用 RSVP 等 signaling 隨需要求）。
- 功能元件：
  - **Packet classifier**（在 DS 網域邊緣）：**BA（behavior aggregate）**只看 DSCP；**MF（multi-field）**看多個欄位（來源 / 目的位址、TOS、協定 ID、來源 / 目的 port）。
  - **Traffic conditioner**：**meter**（量測時間性質）、**marker**（設定 DSCP）、**shaper**（延遲封包使符合規格）、**dropper / policer**（依規則丟包）——上一支影片的 shaping / policing。
  - **PHB（per-hop behavior）**：同一個 DSCP 的封包在節點上可觀察到的轉送行為。三種：**Default（DE）**＝今天的 best effort；**Assured Forwarding（AF）**＝不同等級的轉送保證；**Expedited Forwarding（EF）**＝高優先權行為：任何 DS 節點上該類封包的離開速率必須**大於等於**一個可設定的速率，而且不受其他流量強度影響（p.172）。
- 課本 Summary（p.172）：在網際網路環境中，只支援**軟**即時通訊。

（最後一講：即時資料庫。）

---

## 6. 【實測】五跳網路：FCFS vs Stop-and-Go

模擬一條 CBR 流（每 10 個時間單位一個封包）經過 5 個交換器；每跳有傳播延遲 1 和隨機的交叉流量排隊延遲（0–8）。比較 work-conserving 的 FCFS 和 non-work-conserving 的 Stop-and-Go（frame T = 10，到達後等到**下一個** frame 開頭才送）：

```python
q_delay = random.choice([0, 0, 0, 1, 2, 8])  # cross traffic at this hop (0..8)
arrive = t + 1 + q_delay                      # 1 = propagation
if policy == "SG":                            # hold until start of NEXT frame
    arrive = (arrive // T + 1) * T
```

```
work-conserving FCFS              : delay   5.0.. 33.0  jitter 28.0  min gap  0.5  max pkts in 10 units 4
Stop-and-Go (non-work-conserving) : delay  50.0.. 50.0  jitter  0.0  min gap 10.0  max pkts in 10 units 1
source: 1 packet per 10 units (gap 10, 1 pkt per window)
```

看到什麼：

- **FCFS**：平均延遲小（最小 5），但 jitter 28；原本間隔 10 的封包，最後有的只差 **0.5**（擠在一起），某 10 單位內出現 **4 個**——traffic distortion，σ 變大了。
- **Stop-and-Go**：延遲較長（固定 50 = 每跳一個 frame），但 **jitter 0**、間隔恢復成 10、每個視窗只有 1 個——流量特性完全重建。
- 正是課本的比較：non-work-conserving 延遲較長、緩衝較多，但能給緊的 jitter 上界、不會越走越爆發。（每跳延遲都 < T，所以每跳剛好延遲一個 frame；若某跳延遲超過 T，會延遲兩個 frame，仍在「每跳最多 2T」之內。）

---

## 7. 【原始碼】【實測】Linux：DRR 與 DSCP

**WFQ 的實用近似：DRR（Deficit Round Robin）**。WFQ 要排序（O(log N)）；DRR 只要 round robin 加一個「赤字」計數器，O(1)，按各類別的 quantum（份額）分頻寬：

`net/sched/sch_drr.c:385`（`drr_dequeue()`）

```c
		len = qdisc_pkt_len(skb);
		if (len <= cl->deficit) {
			cl->deficit -= len;
			skb = qdisc_dequeue_peeked(cl->qdisc);
			if (unlikely(skb == NULL))
				goto out;
			if (cl->qdisc->q.qlen == 0)
				list_del(&cl->alist);
			...
			return skb;
		}

		cl->deficit += cl->quantum;
		list_move_tail(&cl->alist, &q->active);
```

- 輪到某類別：赤字夠送下一個封包就送、扣掉長度；不夠就加一個 **quantum**、移到隊尾。
- 長期來看各類別拿到的頻寬 ∝ quantum——就是 FFQ 的 φᵢ / Σφⱼ，只是以封包為單位近似。
- 本機 `net/sched/` 也有 `sch_qfq.c`（quick fair queuing）、`sch_hfsc.c`（有延遲保證的階層式公平服務曲線）、`sch_cbs.c`（credit-based shaper，TSN 用）等。

**DiffServ 從使用者空間標記**：把 socket 的封包標成 DSCP **EF（46）**——VoIP 用的 expedited forwarding：

```c
int tos = 46 << 2;                                        /* DSCP EF = 46 in the upper 6 bits */
int r = setsockopt(s, IPPROTO_IP, IP_TOS, &tos, sizeof tos);
...
getsockopt(s, SOL_SOCKET, SO_PRIORITY, &prio, &l);
```

```
setsockopt(IP_TOS=0xb8) -> 0; kernel TOS byte = 0xb8 (DSCP 46, ECN 0); SO_PRIORITY = 4
```

- 一般使用者就能設：DS 欄位 = 0xb8（DSCP 46）。這就是課本說的「前 6 位元是 DSCP」。
- 但 kernel 把它轉成本機的封包優先權時用的是**舊的 TOS 語意**：

  `include/net/route.h:271`

  ```c
  static inline char rt_tos2priority(u8 tos)
  {
  	return ip_tos2prio[IPTOS_TOS(tos)>>1];
  }
  ```

  0xb8 & 0x1E（`IPTOS_TOS_MASK`）= 0x18 → 右移 1 = 12 → `ip_tos2prio[12]` = **TC_PRIO_INTERACTIVE_BULK（4）**（`net/ipv4/route.c:173`）→ 在 pfifo_fast 是 `prio2band[4]` = **band 1**（一般優先），不是最高的 band 0（第 27 支影片筆記）。
- 也就是說，**主機本地**的佇列並不懂 DiffServ 的 EF；DSCP 標記主要是給**路上的路由器**（edge / core）看的——正是 DiffServ「核心路由器照 DSCP 決定 PHB」的設計。要在本機也給 EF 優先，要另外用 `tc` 依 DSCP 分類（需要 root）。

---

## 8. 一頁速查表

| 主題 | 重點 |
|---|---|
| Service discipline | 交換器 = 佇列；封包排程器；rate-based |
| Distortion | (σ, ρ) → (σ + Σρ·d_j^max, ρ)；排隊讓封包擠在一起 |
| 控制 | rate controller（regulator）＋ scheduler |
| WCD vs NWCD | 不閒置、緩衝少、延遲短、jitter 無法限制 vs eligibility time、緩衝多、延遲長、jitter 可限制 |
| 多層 FCFS | 靜態優先權；延遲上界種類 = 等級數 |
| FFQ / WFQ | 服務速率 = φᵢ/Σφⱼ × C；FFQ 假設同時服務、無限可分；WFQ 是封包版 |
| Jitter-EDD | 標頭記提早量，下一站扣住 → jitter 上界 |
| Stop-and-Go | frame T；下一個 frame 才送；每跳 ≤ 2T；延遲與頻寬粒度 P/T 耦合；多層 framing（調和） |
| RCSP | 解耦：regulator（RJ 部分重建、DJ 完全重建）＋ 不可搶占靜態優先權多層 FCFS |
| IntServ | RSVP、per-flow；guaranteed / predictive；狀態隨流數成長、要全面佈署 |
| DiffServ | DS 欄位前 6 位元 DSCP；狀態隨類別數；複雜度在邊緣；premium（virtual leased line）/ assured；SLA、classifier、conditioner、PHB |
| 實測 | 5 跳：FCFS jitter 28、封包擠到 0.5 間隔；Stop-and-Go jitter 0、延遲固定 50 |
| Linux | DRR（deficit ＋ quantum ≈ WFQ）；IP_TOS 可設 EF，但本機 `rt_tos2priority` 把它映射到 band 1 |

---

## 9. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **即時資料的時間性（資料多久會過期）** | **第 31 支**：Real Time Data Stores |

---

## 附：本講一句話總結

> **Service discipline 就是路由器的封包排程器：每個交換器是一個佇列，排隊會讓封包擠在一起，使 (σ, ρ) 流量到第 i 站時變成 (σ + Σρ·d_j^max, ρ)，所以要用 regulator 重建流量特性、再用 scheduler 提供延遲上界。Work-conserving 的做法（FCFS、WFQ）從不閒著、緩衝少延遲短，卻無法限制 jitter；non-work-conserving 的做法給每個封包 eligibility time、早到也要等，緩衝較多延遲較長，卻能把 jitter 壓緊——Jitter-EDD 在標頭記下提早量讓下一站扣住，Stop-and-Go 只在下一個 frame 開頭才送（每跳最多 2T，但延遲與頻寬粒度 P/T 互相牽制，多層調和的 frame 可以緩解），RCSP 則把每條連線的 regulator（RJ 部分重建、DJ 完全重建）和不可搶占的靜態優先權多層 FCFS 解耦，簡單又有效。WFQ 源自以 φᵢ/Σφⱼ × C 分配頻寬的 fluid fair queuing。網際網路層級的 QoS 有兩個模型：IntServ 用 RSVP 為每條流預留資源、提供 guaranteed 與 predictive 服務，但狀態隨流數成長、必須全面佈署；DiffServ 只在 IP 標頭的 DS 欄位放 6 位元 DSCP，狀態只和服務類別數有關，把分類與整形推到網路邊緣，提供 premium（virtual leased line）與 assured 服務。實測五跳網路上 FCFS 讓原本間隔 10 的封包擠到只差 0.5、jitter 28，Stop-and-Go 則 jitter 為 0；Linux 用 DRR 近似 WFQ，而一般使用者能把封包標成 EF，但主機本地的 rt_tos2priority 只把它放到普通優先的佇列——DSCP 是給路上的路由器看的。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 30 Part II 錄影逐字稿（YouTube 自動字幕，39:13）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch6 §13–14 整理。原始碼為 Linux v6.2.12；實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
