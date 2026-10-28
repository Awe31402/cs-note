# Real Time Systems 第二十八支影片完整筆記：LAN 上的即時通訊（Lecture 29）

> **課程**：Real Time Systems — Lecture 29：Real Time Communication in LANs（播放清單第 28 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（53:53，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch6 §4–7（PDF p.145–155）
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
- [1. Ethernet 為什麼不行；硬即時的原則](#1-ethernet-為什麼不行硬即時的原則)
- [2. 三大類協定](#2-三大類協定)
- [3. Global priority 的困難](#3-global-priority-的困難)
- [4. 四種協定的概念](#4-四種協定的概念)
  - [4.1 Calendar-based](#41-calendar-based)
  - [4.2 Bounded access](#42-bounded-access)
  - [4.3 Countdown](#43-countdown)
  - [4.4 Virtual time](#44-virtual-time)
- [5. IEEE 802.5：優先權 token ring](#5-ieee-8025優先權-token-ring)
- [6. Window-based 協定](#6-window-based-協定)
- [7. IEEE 802.4：timed token](#7-ieee-8024timed-token)
- [8. RETHER](#8-rether)
- [9. 效能比較：ABU 與 GP(U)](#9-效能比較abu-與-gpu)
- [10. 【課本補充】軟即時：traffic smoothing](#10-課本補充軟即時traffic-smoothing)
- [11. 【實測】802.4 token 持有時間、802.5 效率、countdown](#11-實測8024-token-持有時間8025-效率countdown)
- [12. 【原始碼】Linux 的 calendar：taprio（IEEE 802.1Qbv）](#12-原始碼linux-的-calendartaprioieee-8021qbv)
- [13. 一頁速查表](#13-一頁速查表)
- [14. 與後續講次的連結](#14-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

LAN 上怎麼做硬即時通訊？

| 類別 | 想法 | 類比 CPU 排程 | 代表 |
|---|---|---|---|
| **Global priority** | 任何時候傳整個網路最高優先權的訊息 | EDF / RMA | CAN、countdown、virtual time、**IEEE 802.5** |
| **Calendar-based** | 大家共享一張「誰何時傳」的時間表 | cyclic scheduler | dynamic reservation |
| **Bounded access** | 限制每個節點佔用通道的時間 | 時間片 / 比例分配 | **IEEE 802.4**、**RETHER** |

結論：**低頻寬用 global priority（802.5），高頻寬用 bounded access（802.4）**。

---

## 1. Ethernet 為什麼不行；硬即時的原則

[00:00]–[02:30]

- Bus 協定如 Ethernet：高負載時延遲增加、吞吐量下降——原因是**競爭**：多個 **station**（接在匯流排上的節點）同時傳 → 碰撞率高 → 重傳。
- 所以即時應用通常**不建議**用 Ethernet。
- 硬即時應用（特別是可預測的 CBR 流量）：網路使用率**刻意保持低**——確定性比提高使用率重要得多（和 CPU 排程一樣，否則無法保證可排程）。
- 硬即時、CBR 流量之外的空檔，才讓軟即時和非即時流量傳——就像 CPU 排程中，沒在跑即時任務的空檔拿來跑別的。

課本 p.147：硬即時應用中，網路延遲的**確定性可預測**優先於網路使用率。

---

## 2. 三大類協定

[02:30]–[03:00]（課本 §5，p.147–148）

1. **Global priority protocols**：設計哲學類似 EDF、RMA。
2. **Bounded access scheduling**：某種**速率控制**，讓網路不會被一個節點佔走。
3. **Calendar-based protocols**：設計哲學和 bounded access 類似。

---

## 3. Global priority 的困難

[03:00]–[05:30]（課本 p.147）

- 每個訊息有一個優先權；MAC 層協定確保任何時刻服務的是**最高優先權**的訊息（CAN 就是例子）→ 理論上可以用 RMA、EDF 排程訊息。
- 但實務上讓 RMA / EDF 沒效率：

| CPU 排程 | 網路 |
|---|---|
| 全部在同一個硬體上，所有優先權、所有執行意圖都看得到 | **分散式系統**：訊息從不同節點產生，沒有中央仲裁者，沒有全域視野 → 很難知道某時刻誰的優先權最高 |
| 任務可以中途**搶占** | 封包傳到一半**不能搶占**（課本：剩下的位元不能之後再傳） |

課本：這兩點違反了 RMA 最佳性的基本前提；硬用 RMA 排程訊息，可達到的通道使用率會很低。實際上得到的優先權資訊多少是過時的。

---

## 4. 四種協定的概念

### 4.1 Calendar-based

[05:30]–[08:00]（課本 p.148、151）

- **Dynamic reservation** 技術：所有接在匯流排上的節點**事先已知**。
- 每個節點有一份**行事曆（calendar）**：每個時段該哪個節點傳；大家共享一個**全域時鐘**——節點 1 傳這段、節點 2 傳下一段……然後循環。
- 等於網路版的 **cyclic scheduler**。對**週期性**（CBR）訊息很有效率。
- 沒有預約的訊息（非週期、偶發）到了：查本地行事曆找空檔 → **廣播**「我要用這段沒人用的時間」→ 各節點更新自己的行事曆。（自己的時段夠用就直接用；不夠才要大家同意。）
- 很難處理非週期與偶發訊息。

課本：簡單、有效率；處理確定性的週期訊息很有效、沒有優先權仲裁的開銷；但非週期和偶發訊息很難處理，所以只用在非常簡單的系統。

### 4.2 Bounded access

[08:00]–[09:00]

- 改善 calendar 的問題：**限制**每個節點存取通道的時間——像 round robin：節點 1 一段時間、節點 2、節點 3……但不維護明確的表，只設**上限**。這也限制了可以傳的資料量。
- 節點內有多個封包排隊時，用**本地排程演算法**決定先送哪個。

課本 p.147：限制每個節點的存取時間 → 封包要等的時間有上限。

### 4.3 Countdown

[09:00]–[11:30]（課本 p.148–149）

- 回到 global priority：把時間切成固定大小的 **slot**；每個 slot 開始時做**優先權仲裁**，找出全網最高優先權的訊息，傳它。
- **Slot 大小**很關鍵：
  - 至少要等於**端到端傳播延遲**：訊息要在別人開始傳之前到達目的地，否則碰撞偵測不到。
  - 太大 → 浪費時間（像記憶體的 internal fragmentation）。
- CAN 比較像 countdown：切 slot、送出優先權。

課本 Fig.6（數字越大優先權越高；**同時傳輸是 OR 邏輯**）：N1 = 10、N2 = 16、N3 = 20（5 位元，MSB 先送）。

```
         msb               lsb
N1 = 10:  0 | 1   0   1   0     ← 送 0 聽到 1 → 第 1 位就退出
N2 = 16:  1 | 0   0 | 0   0     ← 第 3 位送 0 聽到 1 → 退出
N3 = 20:  1 | 0   1   0   0     ← 送完最後一位，知道自己最高 → 開始傳
```

- 和 CAN 剛好相反：CAN 是 wired **AND**（0 顯性、**ID 小**優先）；這裡是 **OR**（1 顯性、**數字大**優先）。原理一樣：送了弱勢的位元卻聽到強勢的位元，就知道有人比自己高。

### 4.4 Virtual time

[11:30]–[13:01]（課本 p.149）

- 用**通道狀態**推論其他節點的待送封包。
- 有封包要送的節點先**等一段時間**，時間長度和它最高優先權訊息的優先權**成反比**：優先權越高、等越短。
- 等完感測通道：忙 → 代表有更高優先權的在傳 → 等下一個閒置期；閒 → 傳。
- 和一般的指數退避（0–x、0–2x、0–4x 隨機等）不同，等待時間由優先權決定，比較符合即時的想法。

【課本補充】（p.149 Fig.8）兩個優先權只差 1 的節點，等待時間至少要差一個**傳播時間**：N1 開始傳之後，N2 若在傳播時間內就醒來，還偵測不到 N1 的訊號 → 也開始傳 → 碰撞、仲裁錯誤。所以傳播延遲限制了相鄰優先權的等待時間差。

---

## 5. IEEE 802.5：優先權 token ring

[13:01]–[19:01]（課本 §5.2，p.149–150）

- **邏輯環**（不一定是實體環）：每個節點知道左右鄰居。
- Token 的標頭有**保留（reservation / priority）欄位**和**模式位元**（reservation mode / free mode）；payload 和存取通道無關。
- 訊息切成 **frame**。
- 運作：
  1. Token 繞環，每個節點檢查保留欄位。
  2. 自己有**更高**優先權的訊息 → 把自己的優先權**寫進**保留欄位（假設大家都誠實、合作）。
  3. Token 回到原傳送者 → 它知道全網誰的優先權最高。
  4. 如果還是自己 → 繼續傳；否則把 token 設成 free 放出去。
  5. 做了保留的那個高優先權節點**抓住** token、設成 reservation mode、開始傳。

課本：reservation mode 中傳送封包，正在傳的訊息的優先權登記在 token 標頭；封包經過環時，有待送訊息的節點檢查保留欄位，優先權較高者登記上去；token 回到傳送者時，它把 token 設成 free 放出；經過環時，做保留的節點抓住它、設成 reservation mode、開始傳。

**兩個定理**：

> **Theorem 1**：完成一個 frame 的傳送（也就是可以開始送下一個）最少要 **max(F, θ)**。F = frame 傳送時間，θ = 傳播時間。

- 節點要等兩件事都完成才送下一個：(1) 最後一個位元送出（F）；(2) token 的標頭繞回來（θ）。兩個都是必要條件 → 取較大者。

> **Theorem 2**：高優先權封包最多遭受 **2 × max(F, θ)** 的 inversion。

- 不幸的情況：低優先權節點剛開始傳，高優先權訊息就到了。傳送不能搶占 → 等 max(F, θ)；然後 token 要帶著新的保留繞回來 → 再 max(F, θ)。

---

## 6. Window-based 協定

[19:01]–[23:00]（課本 §5.3，p.150–151）

- 時間切成 frame（像 countdown、CAN 的 slot）。
- 目前的傳送視窗是一個二元組 **(low, high)**：只有優先權在 low 與 high 之間的訊息能傳。
- 偵測到**碰撞** → 每個節點把 low 加一 → 視窗縮小 → 能傳的訊息變少 → 碰撞機會降低、成功機會提高。
- **空的 frame**（沒人傳但有封包在等）→ low 減一 → 視窗放大。

老師的整理【口頭】：

- 兩大陣營：**global priority**（slot 式如 countdown、CAN；或每個訊息結束時仲裁，如 token ring）與 **bounded access**（限制速率，讓碰撞機率為 0 或大幅降低）。
- Slot 式：固定時刻仲裁，訊息傳完但下一個 slot 還沒到就只能閒著；token ring：一個訊息傳完立刻重新繞 token。訊息長度固定時兩者差不多（傳送開始後都不能搶占）。
- Token ring 在仲裁很快時有效；訊息長時 inversion 時間（2·max(F, θ)）也長。
- **Window-based 是混合**：有 bounded access 的味道（只有視窗內的優先權能傳，視窗可大可小），又是 slot 式的。

---

## 7. IEEE 802.4：timed token

[23:00]–[34:01]（課本 §6.2，p.152–153）

Bounded access 的代表（global priority 的代表是 802.5）。

- 用在 token ring / token bus 網路；**timed token protocol**：一樣要持有 token 才能傳，但**傳送時間有上限**——擁有 token 也沒有絕對權利，這才能提供即時保證。
- **TTRT（target token rotation time）**：token **連續兩次**造訪同一個節點之間的預期時間——主要的設計參數。
- 週期訊息叫 **synchronous**；非週期、偶發的叫 **asynchronous**。
- 每個節點分到一部分 **synchronous bandwidth** Hᵢ（最多持有 token 傳週期訊息的時間）。
- 拿到 token：先傳同步（即時）訊息；做完之後如果還有時間（token **提早**到）才能傳非同步訊息。

**TTRT 的組成**：

> **TTRT = θ + Σ Hᵢ**

- θ：token 繞整個匯流排一圈的傳播時間（至少要繞一圈回到自己）。
- Σ Hᵢ：每個有同步流量的節點輪流用完自己的時間。

**Asynchronous overrun**：非同步訊息可能讓 token 晚到；設計上兩次造訪之間最多 **2 × TTRT**（容忍一點 jitter）。只用同步模式的節點，兩次造訪之間仍在 TTRT 內。

**選 TTRT**：所有訊息中最短的 deadline 是 Δ → 最壞 2·TTRT 內要拿回 token：

> **TTRT ≤ Δ / 2**

TTRT > Δ/2 → token 晚到 → 錯過 deadline。

**分配 Hᵢ**：

> **Hᵢ = (TTRT − θ) × (Cᵢ/Tᵢ) / Σⱼ (Cⱼ/Tⱼ)**（課本式 6.1；課本寫成 TTRT × …，即 θ ≈ 0）

- Cᵢ = 節點 i 在 Tᵢ 內要傳的資料量 → Cᵢ/Tᵢ 是它的**頻寬需求**（課本：節點 i 造成的通道使用率）。
- 依**相對頻寬需求**按比例分 TTRT − θ；所有比例加起來是 1 → Σ Hᵢ = TTRT − θ，和上面的定義一致。

老師的例子（θ = 0）：節點 3 每 200 ms 要送資料 → 最短 deadline 200 ms → TTRT = 200 / 2 = **100 ms**；算出各節點的 Cᵢ/Tᵢ、按比例乘 100，三個 H 加起來是 100。（§11 實測課本 Ex 6.1。）

**和 802.5、RMA 的對照**【口頭】[34:01]：802.4 = token ring ＋ bounded access，給的時間依**相對頻寬需求**（要得多給得多）——和 RMA「週期越短越優先」同一種哲學；802.5 是純 global priority：每次都仲裁出最高優先權的訊息。

---

## 8. RETHER

[34:30]–[44:00]（課本 §6.1，p.151–152）

**RETHER = Real-time ETHERnet**：在 Ethernet 和 token 協定之間切換的**混合**協定。

- 平常是 **CSMA/CD 模式**（一般 Ethernet）。
- 有即時訊息時切到 **RETHER 模式**（token 傳遞）；即時 session 都結束後切回。
- 切換：節點收到本地應用的即時請求 → 若網路還不在 RETHER 模式，**廣播** Switch-to-RETHER 訊息 → 各節點設成 RETHER 模式；正在傳的節點**等目前封包傳完**，再回 ACK 給發起者（表示願意切換、沒有資料還在 CSMA/CD 退避中）→ 發起者收齊 ACK 後**建立 token、開始繞**。
- 多個發起者同時發起：只 ACK **ID 最小**的發起者；發起者只 ACK ID 比自己小的其他發起者。ACK 遺失、有節點沒收到廣播或已死 → 發起者逾時重試；重試固定次數後認定沒回應的節點死了，並通知其他存活節點。

⚠️ 老師說「發起者 A 只在 B 的 ID **比 A 小**時才 ACK B」[37:01]，和課本一致；但他接著說「A 有比較大的 node ID」——同一件事的另一種說法，結論是 **ID 最小的發起者勝出**。

**RETHER 模式**：

- 用 **timed token** 提供頻寬保證；任何時刻只有一個節點能傳；每個節點**只能有一個**即時請求（不能一直塞請求霸佔網路）。
- 每個即時請求事先說明在固定時間 **MTRT**（課本：maximum token rotation time）內要傳多少資料 → 算出每個節點的 **MTHT**（mean / maximum token holding time），為每個 session 保留平均固定頻寬。
- 節點分成 **RTS（real-time set，有預約頻寬的）** 和 **NRTS（non-real-time set）**。
- Token 每圈依序造訪 RTS 的節點；拿到 token 的送一單位即時資料，交給 RTS 的下一個；RTS 最後一個把 token 交給 NRTS。
- Token 帶一個欄位：

  > **TimeToDeadline = MTRT − Σ_{i∈RTS} MTHTᵢ**

  例：每 200 ms 要拿到 token 一次，但所有即時節點只用 100 ms → 每 200 ms 有 100 ms slack，可以排非即時流量（又是 EDF / RMA 的「空檔給非即時」）。
- NRTS 節點收到 token：時間夠送一個封包 → 送、扣 TimeToDeadline、交給 NRTS 下一個；不夠 → 告訴 RTS 最後一個「下一圈 NRTS 由我先拿」、把 token 交給 RTS 第一個。
- **允入控制**（admission control）——像 CPU 的使用率加總：

  > **Σ_{i∈RTS} MTHTᵢ + MTHT_new + TBNRT ≤ MTRT**

  TBNRT = 保留給 NRTS 的頻寬（把所有非即時流量當成另一個任務）。用**時間當頻寬的代理**：在 1000 ms 視窗裡拿到 20 ms，有效頻寬就是 20/1000 × 通道峰值頻寬。
- 允入控制要等節點**拿到 token** 才做；即時連線結束時，節點只要把自己從 token 上的 RTS 資訊移除。

課本：RETHER 增強 TCP/IP，**不必修改既有的 Ethernet 硬體**就提供即時保證；沒有即時 session 時用 CSMA/CD，減少對非即時流量的影響。

---

## 9. 效能比較：ABU 與 GP(U)

[44:00]–[53:30]（課本 §7，p.154–155）

**訊息集合的三類**（課本 Fig.12）：

| 類別 | 意思 |
|---|---|
| **Unsaturated schedulable** | 可排程，而且訊息稍微變大**仍然**可排程（有 slack，通道使用率低） |
| **Saturated schedulable** | 可排程，但任何訊息稍微變大就有訊息錯過 deadline（在邊界上） |
| **Unschedulable** | 有訊息會錯過 deadline |

⚠️ 老師一開始把 unsaturated 說成「不可排程」，隨即更正 [44:30]–[45:01]。

**兩個指標**：

| 指標 | 意思 |
|---|---|
| **ABU（absolute breakdown utilization）** | 訊息集合開始錯過 deadline 時的使用率期望值；U = Σ Cᵢ/Tᵢ。也就是網路在沒有訊息錯過 deadline 下能承載的最大流量 |
| **GP(U)（guarantee probability）** | 使用率為 U 時，訊息集合**全部趕上** deadline 的機率；U < ABU 時接近 1，U > ABU 時趨近 0 |

課本：設計時通常不知道訊息集合的細節，但知道各即時來源的通道使用率估計，所以用這種以使用率為基礎的指標。GP(U) 在 ABU 附近急降 → ABU 是即時網路平均效能的穩健指標。

**802.5 vs 802.4**（課本 Fig.13、14）：

| 頻寬 | 較好的 | 原因 |
|---|---|---|
| **低** | **802.5（global priority）** | 傳送慢時，bounded access 的輪流方式造成的 priority inversion 會傷害 deadline 短的訊息——給你的時間有限，送不完 |
| **高** | **802.4（bounded access）** | 802.5 每個訊息都要仲裁：token 要繞全部節點、看保留、改寫，比送一個 frame 還久 |

- 802.4 的 ABU 隨頻寬**單調增加**；802.5 先增後**下降**——違反「頻寬越大越好」的直覺。
- 原因：Theorem 1 的 **max(F, θ)**。低頻寬時 F（frame 傳送時間）大，max = F；高頻寬時 F 變小、θ（token 繞一圈）不變 → max = θ → 傳完 frame 還要等 token 回來。課本：浪費比例 **(θ − F)/F**，θ 和頻寬無關、F 隨頻寬變小 → 頻寬越大浪費越多。Timed token 沒有這個問題：節點可以在自己的時段裡連續傳。

---

## 10. 【課本補充】軟即時：traffic smoothing

（課本 §4，p.145–147；課堂沒講）

- 軟即時通訊不給絕對保證，只優先處理即時訊息、讓 deadline miss 比例低，對延遲給**統計**保證。
- 假設軟即時流量是 CBR / VBR、速率遠低於通道容量；非即時訊息爆發式地非週期到達——爆發會讓保證難以維持 → 要**平滑**爆發。
- **Kweon 與 Shin 的固定速率平滑**：在 MAC 和 TCP/IP 之間放一個 traffic smoother，以 leaky bucket 為基礎的 **Credit Bucket Depth（CBD）** 演算法：
  - 參數 **CBD**（每次補充的最大 credit 數、也是桶子上限）和 **RP**（refresh period）。CBD/RP = 非即時訊息的平均保證吞吐量。
  - 桶子裡的 credit 叫 **CNS（current network share）**；每次補充 CNS = min(CNS + CBD, CBD)。CNS 可以為負（借用）。
  - 非即時訊息來時：CNS > 0 → 扣掉訊息大小、送出；否則放在緩衝等到 CNS > 0。
  - 缺點：不彈性；節點越多每站的限制越低；以最壞情況計算，很悲觀。
- **Adaptive traffic smoothing**：用**單位時間的碰撞數**估計網路使用率。發生碰撞 → 立刻清空 credit 桶（暫停非即時封包）、RP 加倍（上限 RPmax）；一段時間沒有碰撞 → RP 定期減一個固定值（下限 RPmin）。實驗中即時訊息的 deadline miss 比例遠低於毫秒範圍；但不能提供確定性的即時通訊，只適合軟即時。
- （這是下一支影片「traffic shaping」的前身。）

---

## 11. 【實測】802.4 token 持有時間、802.5 效率、countdown

**802.4（課本 Ex 6.1）**，TTRT = 100 ms、θ = 0：

```python
u = [c * 8 / t for c, t in nodes]                  # Mb per ms
s = sum(u)
return [TTRT * x / s for x in u], s
```

```
as solved in book (N3: 2 MB / 200 ms): sum Ci/Ti = 0.1259 Mb/ms, H = 21.19, 15.25, 63.56 ms (sum 100.0)
as stated in problem (N3: 1.2 MB / 500 ms): sum Ci/Ti = 0.0651 Mb/ms, H = 40.98, 29.51, 29.51 ms (sum 100.0)
```

- 照課本解法的數字算，和課本的 21.18、15.25、63.56 ms 一致（21.18 vs 21.19 是四捨五入）。

⚠️ 課本 Ex 6.1 題目說「N3 每 **500 ms** 傳 **1.2 MB**」，解答卻用「每 **200 ms** 傳 **2 MB**」，而且選 TTRT 時說「最短 deadline 是 200 ms」——照題目，最短 deadline 應是 300 ms（N1），TTRT 可取 150 ms。解答的第三個 H 也誤標成 H₁。照題目的數字，H 會是 41.0、29.5、29.5 ms（若 TTRT 取 100）。

**802.5 的效率 vs 頻寬**（1500 bytes 的 frame、token 繞環的傳播時間 θ = 100 μs）：

```
      1 Mb/s: F =  12000.0 us  max(F,theta) =  12000.0 us  useful fraction = 1.000  worst inversion 2*max = 24000 us
     16 Mb/s: F =    750.0 us  max(F,theta) =    750.0 us  useful fraction = 1.000  worst inversion 2*max = 1500 us
    100 Mb/s: F =    120.0 us  max(F,theta) =    120.0 us  useful fraction = 1.000  worst inversion 2*max = 240 us
   1000 Mb/s: F =     12.0 us  max(F,theta) =    100.0 us  useful fraction = 0.120  worst inversion 2*max = 200 us
```

- 低頻寬：F 遠大於 θ，通道一直在傳資料，但最壞 inversion 很長（1 Mb/s 時 24 ms）。
- 1 Gb/s：F = 12 μs < θ = 100 μs → 每個 frame 之後要等 token 繞回來，只有 **12%** 的時間在傳資料——課本 (θ − F)/F 的浪費，也就是 802.5 的 ABU 在高頻寬時下降的原因。

**Countdown 仲裁（課本 Fig.6，OR 邏輯）**：

```
   bit 0: bus=1 drop ['N1']
   bit 2: bus=1 drop ['N2']
countdown (OR logic) N1=10 N2=16 N3=20 -> ['N3']
```

和課本描述完全一致：N1 第 1 位退出、N2 第 3 位退出、N3 勝出。

---

## 12. 【原始碼】Linux 的 calendar：taprio（IEEE 802.1Qbv）

課本的 calendar-based 協定在現代以 **TSN（Time-Sensitive Networking）** 的形式回到 Ethernet：IEEE 802.1Qbv 的 **time-aware shaper**——大家同步時鐘（PTP），交換器和網卡依一張**閘門時間表（gate control list）**決定每個時段哪些流量類別可以送。Linux 的實作是 `taprio` qdisc：

`net/sched/sch_taprio.c:37`

```c
struct sched_entry {
	struct list_head list;

	/* The instant that this entry "closes" and the next one
	 * should open, the qdisc will make some effort so that no
	 * packet leaves after this time.
	 */
	ktime_t close_time;
	ktime_t next_txtime;
	atomic_t budget;
	int index;
	u32 gate_mask;
	u32 interval;
```

取封包時，只有閘門開著的流量類別能送：

`net/sched/sch_taprio.c:535`

```c
		prio = skb->priority;
		tc = netdev_get_prio_tc_map(dev, prio);

		if (!(gate_mask & BIT(tc)))
			continue;

		return skb;
```

對照：

| 課本 calendar-based | Linux taprio |
|---|---|
| 每個節點一份行事曆：哪個時段誰能傳 | 一串 `sched_entry`，每個 `interval` 長、`gate_mask` 標出哪些流量類別可以送 |
| 共享全域時鐘 | 以 PTP 同步的時鐘為基準（`close_time` 是絕對時間） |
| 時表循環（cyclic scheduler） | entry 依序執行、整張表週期性重複 |
| 時段結束不能再送 | 註解：entry「關閉」時，盡量讓沒有封包在這之後離開 |

（設定 taprio 要 root 和支援的網卡；這裡只看原始碼。）

---

## 13. 一頁速查表

| 主題 | 重點 |
|---|---|
| Ethernet | 碰撞 → 重傳 → 高負載延遲爆增；硬即時刻意保持低使用率 |
| Global priority 的困難 | 沒有全域視野、封包不能搶占 → RMA 使用率很低 |
| Calendar | 共享時間表 ＋ 全域時鐘；網路版 cyclic scheduler；週期訊息很好、非週期難 |
| Bounded access | 限制每個節點的存取時間；節點內再本地排程 |
| Countdown | slot ≥ 端到端傳播延遲；每個 slot 開頭位元仲裁（課本 OR 邏輯，大者勝；CAN 是 AND，小者勝） |
| Virtual time | 等待時間 ∝ 1/優先權；相鄰優先權至少差一個傳播時間 |
| 802.5 | token 保留欄位改寫成更高優先權；最少 max(F, θ)；最壞 inversion 2·max(F, θ) |
| Window | (low, high)；碰撞 low++、空 frame low−−；混合型 |
| 802.4 | TTRT = θ + ΣHᵢ ≤ Δ/2；Hᵢ ∝ Cᵢ/Tᵢ；asynchronous overrun 最壞 2·TTRT |
| RETHER | CSMA/CD ↔ token 模式；ID 最小的發起者；RTS / NRTS；TimeToDeadline = MTRT − ΣMTHT；允入 ΣMTHT + new + TBNRT ≤ MTRT |
| 效能 | unsaturated / saturated / unschedulable；ABU、GP(U)；低頻寬 802.5、高頻寬 802.4；802.5 浪費 (θ − F)/F |
| 軟即時（課本） | CBD leaky bucket；adaptive：碰撞就清桶、RP 加倍 |
| Linux | taprio = calendar（802.1Qbv）：`sched_entry` 的 `gate_mask` ＋ `interval` |
| 實測 | Ex 6.1 H = 21.19、15.25、63.56；802.5 在 1 Gb/s 只有 12% 時間傳資料 |

---

## 14. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **流量平滑（CBD、leaky bucket）** | **第 29 支**：Traffic Characterization and Shaping |
| **封包交換網路的排程（WAN 層級）** | **第 30 支**：Service Disciplines |

---

## 附：本講一句話總結

> **Ethernet 的 CSMA/CD 在高負載時碰撞、重傳讓延遲不可預測，所以 LAN 上的硬即時通訊改用三類協定：global priority（像 EDF/RMA，但網路沒有全域視野、封包又不能搶占）、calendar-based（大家共享時間表與全域時鐘，是網路版 cyclic scheduler，週期訊息很好、非週期難處理）和 bounded access（限制每個節點佔用通道的時間）。Global priority 的例子有每個 slot 開頭做位元仲裁的 countdown（slot 至少要等於端到端傳播延遲）、等待時間與優先權成反比的 virtual time，以及 IEEE 802.5 優先權 token ring——token 繞一圈收集最高優先權的保留，送一個 frame 最少要 max(F, θ)，高優先權封包最多被反轉 2·max(F, θ)；window-based 協定則以碰撞與空 frame 調整 (low, high) 視窗，是混合型。Bounded access 的代表 IEEE 802.4 timed token 以 TTRT = θ + ΣHᵢ ≤ Δ/2 為設計參數、依 Cᵢ/Tᵢ 按比例分配 token 持有時間；RETHER 平時跑 Ethernet、有即時 session 時切到 timed token，以 ΣMTHT + MTHT_new + TBNRT ≤ MTRT 做允入控制。比較 ABU 與 GP(U) 可知低頻寬時 802.5 較好、高頻寬時 802.4 較好，因為頻寬一高 frame 時間 F 比 token 繞環時間 θ 還短，802.5 就得空等（實測 1 Gb/s 時只剩 12% 時間在傳資料）；而 calendar 的想法今天以 TSN 802.1Qbv 的形式出現在 Linux 的 taprio qdisc 裡。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 29 錄影逐字稿（YouTube 自動字幕，53:53）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch6 §4–7 整理。原始碼為 Linux v6.2.12；實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
