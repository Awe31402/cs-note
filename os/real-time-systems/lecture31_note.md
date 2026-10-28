# Real Time Systems 第三十一支影片完整筆記：即時資料儲存（Lecture 31）

> **課程**：Real Time Systems — Lecture 31：Real Time Data Stores（播放清單第 31 支，最後一講）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（42:12，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch7（PDF p.178–188）
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
- [1. 即時資料和傳統資料庫有什麼不同](#1-即時資料和傳統資料庫有什麼不同)
- [2. 時間性資料與 temporal consistency](#2-時間性資料與-temporal-consistency)
  - [2.1 三元組表示](#21-三元組表示)
  - [2.2 Absolute validity 與 relative consistency](#22-absolute-validity-與-relative-consistency)
  - [2.3 【課本補充】例題](#23-課本補充例題)
- [3. 應用例子](#3-應用例子)
- [4. 資料庫基本觀念](#4-資料庫基本觀念)
  - [4.1 交易、排程、衝突](#41-交易排程衝突)
  - [4.2 ACID](#42-acid)
  - [4.3 Abort、rollback、cascaded abort](#43-abortrollbackcascaded-abort)
  - [4.4 Task 排程 vs transaction 排程；serializability](#44-task-排程-vs-transaction-排程serializability)
- [5. 悲觀式：鎖](#5-悲觀式鎖)
  - [5.1 2PL](#51-2pl)
  - [5.2 2PL-WP](#52-2pl-wp)
  - [5.3 2PL-HP](#53-2pl-hp)
  - [5.4 【課本補充】資料庫的 PCP](#54-課本補充資料庫的-pcp)
- [6. 樂觀式：OCC](#6-樂觀式occ)
  - [6.1 Forward OCC](#61-forward-occ)
  - [6.2 OCC-Sacrifice](#62-occ-sacrifice)
  - [6.3 OCC-BC（broadcast commit）](#63-occ-bcbroadcast-commit)
  - [6.4 【課本補充】Speculative CC 與比較](#64-課本補充speculative-cc-與比較)
- [7. 【課本補充】商用即時資料庫](#7-課本補充商用即時資料庫)
- [8. 【原始碼】Linux kernel 裡的樂觀並行控制：seqlock](#8-原始碼linux-kernel-裡的樂觀並行控制seqlock)
- [9. 【實測】時間有效性檢查與並行控制模擬](#9-實測時間有效性檢查與並行控制模擬)
- [10. 一頁速查表](#10-一頁速查表)
- [11. 與後續講次的連結：整門課的回顧](#11-與後續講次的連結整門課的回顧)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

1. 即時資料儲存的資料**會過期**：每筆資料有時間戳記和有效期。
2. **Temporal consistency**：absolute validity（單筆夠新）＋ relative consistency（一組資料是同一時刻的）。
3. 資料庫基本觀念：交易、ACID、rollback、serializability。
4. 即時並行控制：
   - **悲觀式**（鎖）：2PL → 2PL-WP（繼承優先權）→ **2PL-HP**（高優先權直接 abort 低優先權）。
   - **樂觀式**（驗證）：forward OCC、**OCC-Sacrifice**、**OCC-BC**。
5. 低負載用樂觀式、高負載用悲觀式。

---

## 1. 即時資料和傳統資料庫有什麼不同

[00:16]–[02:00]（課本 p.178、181）

- 很多部署在真實世界的嵌入式即時應用產生大量資料：取資料、更新內部資料結構、處理。例：網路管理系統、工業控制系統、自動駕駛（autopilot）系統。
- 老師不叫它即時資料庫，而叫**即時資料儲存（real-time data store）**。

| | 傳統資料庫 | 即時資料儲存 |
|---|---|---|
| 資料 | 靜態 | 從環境感測、每筆有**時間戳記**；過一段時間就**過期**（小塊資料，帶時間和有效期） |
| 操作 | 沒有時間限制 | 有時間限制 |
| 效能 | 重要 | 更重要；嵌入式裝置能力有限 |

【課本補充】（p.178、181）

> 傳統資料庫和即時資料庫的主要差別：**資料的時間特性**、**資料庫操作的時間限制**、**效能目標**。

- 效能指標：傳統資料庫看**平均**回應時間（單位時間完成的交易數）；即時資料庫看**單位時間錯過 deadline 的交易數**。
- 交易和任務都是工作與排程的單位，但交易可能要以獨占模式存取很多筆資料；任務的執行時間大致可確定，交易的執行時間難預測得多，尤其要存取磁碟時。

---

## 2. 時間性資料與 temporal consistency

### 2.1 三元組表示

[01:30]–[05:00]（課本 §4，p.181–182）

**時間性資料**的例子：

- 溫度感測器週期性讀數：新讀數來了，舊的就**陳舊**了（課本：每 100 ms 送一次，舊資料變成存檔資料）。
- 股價報價：新價格來了，舊報價就過時。
- Fly-by-wire 飛機：每幾毫秒收到高度、速度、加速度；要根據瞬時值做決定時，幾毫秒前的值就完全沒用；算位置、和理想位置的偏差，都只能用最近的資料。
- 錯過 deadline，資料就全部或部分失效。

資料的種類：

| 種類 | 說明 |
|---|---|
| 動態資料 | 反映環境狀態（感測器資料）；或從其他參數**衍生**的（例如算導數）；處理後送給致動器（例如控制化工廠的注入速率） |
| 靜態 / 存檔資料 | 固定的設定資料，收集後儲存；即時系統也可能要快速存取大量靜態資料 |

一個演算法可能同時需要動態和靜態資料，兩者都要符合即時需求。課本例：飛機的目前位置、高度、速度、加速度是時間性資料；預定航線是傳統的非時間性資料。

**表示法**：

> **d = (value, avi, timestamp)**

- value：值；**avi**：absolute validity interval（絕對有效區間）；timestamp：收集的時間。
- 例：(120, 5 ms, 100 ms) → 值 120、100 ms 時收集、有效 5 ms → 在 **100–105 ms** 間有效。

### 2.2 Absolute validity 與 relative consistency

[05:00]–[08:30]（課本 §4.1，p.182）

系統 = 環境 ＋ 控制器 ＋ 致動器 ＋ **即時資料管理儲存**。一致性的意思和傳統資料庫不同：要 **temporally consistent（時間上一致）**——所有存的資料都滿足有效性限制。

| 要求 | 意思 | 條件 |
|---|---|---|
| **Absolute validity** | 環境和資料庫中的反映一致：t₀ 讀的資料，現在還在 [t₀, t₀ + avi] 內 | **current time − d.timestamp ≤ d.avi** |
| **Relative consistency** | 用來衍生新資料的一**組**資料彼此一致、對應同一個系統狀態 | 集合 R 中任兩筆 d、d′：**\|d.timestamp − d′.timestamp\| ≤ R_rvi** |

- 老師：拿今天的壓力配昨天的溫度沒有意義；壓力和溫度要在同一時刻收集。
- 例：**反飛彈系統**用目前的速度和位置預測新位置——不能用早一點的位置配現在的速度，會算錯；可以容忍一點點差距，但不能太多。
- **R_rvi**：relative validity interval（相對有效區間）；R 叫 relative consistency set。

⚠️ 老師說 absolute validity 是「現在時間 − 時間戳記 **小於** avi」[07:32]；課本寫 **≤**。

### 2.3 【課本補充】例題

（p.183）

- **Ex 7.1**：d = (10, 2500 ms, 100 ms)、現在 2700 ms → 有效期 2500–2600 → **不**絕對有效。
- **Ex 7.2**：R = {溫度, 壓力}、R_rvi = 2。溫度 (347°C, 5 ms, 95 ms)、壓力 (50 bar, 10 ms, 97 ms) → |95 − 97| = 2 ≤ 2 → **相對一致**；壓力改成 92 ms → |95 − 92| = 3 → **不一致**。
- **Ex 7.3**：R = {位置, 速度, 加速度}、R_rvi = 100 ms、現在 2600 ms。位置 (25 m, 2500, 200)、速度 (300 m/s, 2550, 300)、加速度 (20 m/s², 2425, 200) → 三筆都絕對有效（100 < 200、50 < 300、175 < 200）；但速度與加速度差 2550 − 2425 = 125 > 100 → **不**相對一致。

⚠️ 課本三元組的順序前後不一：定義是 (value, avi, timestamp)，Ex 7.2 照這個順序；但 Ex 7.1、7.3 寫成 (value, **timestamp, avi**)（例如 (10, 2500 ms, 100 ms) 的 avi 是 100、時間戳記是 2500）。上面的計算依各例題的解答推定欄位。（§9 實測。）

---

## 3. 應用例子

[08:30]–[10:32]（課本 §1，p.178–179）

| 應用 | 特點 |
|---|---|
| **程序控制** | 資料與控制器設定要同步（relative consistency）；資料量可達好幾 MB；要幾毫秒內回應 → 很快的資料結構，排程演算法在最壞情況下也要給出毫秒級回應 |
| **太空船控制** | 用天線、接收器、發射器和地面保持聯繫；監控太空船健康、計算追蹤；為了可靠加入**冗餘**（額外元件，只提高可靠度）→ 資料量變兩倍 → 越可靠資料越多，越難保證即時正確性 |

【課本補充】（p.178–179）另有 **Internet service management**（ISP 的服務管理系統 SMS：認證、授權、計費，上百萬用戶的 session 狀態）和 **網路管理系統**（拓樸、設定、設備、流量與故障資料）。時間需求從微秒（網管的路由決策）、毫秒（工業程序控制開關閥門）到秒（工廠物料搬運）；不管大小，交易沒趕上就失效。

---

## 4. 資料庫基本觀念

### 4.1 交易、排程、衝突

[10:32]–[14:30]（課本 §2，p.179–180）

- 多個平行的執行緒讀寫資料庫，怎麼確保正確？任何時刻看資料庫，都要處於**一致**的狀態（符合規格 = 正確）。
- 並行交易可以**交錯**執行（提高吞吐量和資源使用率）→ 希望並行交易數越多越好。
- **Database schedule**：各並行交易動作的一種排序。
- 有些排程會產生錯誤結果、違反資料庫的一致性與完整性。

老師的例子：`account.balance` 原本 100，兩個交易各要加 1。

```
T1: read 100 ──── +1 → 101 ──── write 101
T2:     read 100 ──── +1 → 101 ──── write 101
結果 101，應該是 102
```

⚠️ 老師說「最後的值會是 **1**」[13:01]，隨後又說是 101；正確是兩個都寫入 101，應為 102。

- 所以存取同一筆資料的並行交易要受限制 → **並行控制協定（concurrency control protocols）**：偵測到同一變數被存取時，讓一個交易做完再讓另一個開始。
- **衝突（conflict）**：兩個交易在差不多同一時間、沒有先後順序地存取**同一個位置**，而且**其中至少一個是寫**。

### 4.2 ACID

[14:30]–[16:01]（課本 p.180）

| 性質 | 意思 |
|---|---|
| **Atomicity（原子性）** | 交易要嘛全做、要嘛全不做；看起來像沒開始過 |
| **Consistency（一致性）** | 任何時候資料庫內容看起來都正確（維持完整性限制） |
| **Isolation（隔離性）** | 並行交易互不干擾，看不到對方寫到一半的狀態（例如寫 5 列，不會看到 3 列更新、2 列還沒更新） |
| **Durability（持久性）** | 交易完成（**commit**）後永久存在，系統當掉也還在 |

課本：rollback 協定確保 atomicity 和 durability；隔離性靠鎖和 rollback 協定。

### 4.3 Abort、rollback、cascaded abort

[16:01]–[19:30]

- 衝突時，其中一個交易要**完全**終止（**abort**，交易失敗）→ **rollback**：把它寫過的所有位置換回舊值。
- **Cascaded rollback / abort**：交易 t_j 讀了 t_i 寫的值，t_i 被 abort → t_j 也要 abort（才能保證 atomicity）；接著 T3……一路連鎖。
- 對即時應用很嚴重：一個交易跑了很久，因為別人被 rollback 自己也被 rollback → 重做的延遲讓交易錯過 deadline。
- 解法：**每個交易只讀已 commit 的交易寫的值** → 不會連鎖 abort（已 commit 的交易永遠不會 abort）。
- Durability：commit 後不能 abort、效果也不會因連鎖 abort 而改變；所有變更寫進永久儲存（硬碟）。

### 4.4 Task 排程 vs transaction 排程；serializability

[19:30]–[22:03]

- CPU 是 serially reusable 的單一資源；資料庫的資源（資料項、記憶體位置、表格列）**數量非常多** → CPU 的資源共享演算法擴展不了；交易也可能很長。
- 並行控制的正確性標準：**serializability（可序列化）**——最終結果（以及任何中間狀態）等同於這些交易**某種序列執行**的結果（例如 T1 → T5 → T6 → T7）。
- 老師的例子：T1 先或 T2 先都得 102，101 不等於任何序列排程 → 錯。

即時並行控制要：serializability、某種 priority inheritance、沒有死結、保證在時限內完成。兩大類：

| | 樂觀式（optimistic） | 悲觀式（pessimistic） |
|---|---|---|
| 做法 | 讓所有交易**不受限制**地進行，有問題再修正（剪掉某些交易） | 覺得有衝突、可能被 abort 的交易**不讓它開始**；做任何操作前**先取得許可**（通常是上鎖） |
| 類比 | — | 對交易的 rate control |

---

## 5. 悲觀式：鎖

### 5.1 2PL

[23:31]–[26:32]（課本 §5.1，p.184）

**Two-phase locking**：

```
locks
  │      ╱‾‾‾‾‾‾‾‾╲
  │    ╱            ╲
  │  ╱  growing       ╲  shrinking
  └──────────────────────── time
     ↑ 只取得           ↑ 第一次釋放後就只能釋放
```

- **Growing phase**：取得所有需要的鎖 → 處理 → **shrinking phase**：釋放鎖。一旦釋放了一個鎖，就不能再取得新鎖。
- 老師：2PL 最重要的並行控制鎖協定；「只要依某個**全域順序**取得鎖，就保證沒有死結」。

【課本補充】（p.184）

- **Strict 2PL**（商用資料庫最常見）：交易結束（commit 或 abort）**之後**才能釋放任何鎖。實作簡單但太嚴格，擋掉一些其實不會違反一致性的並行執行，增加延遲。
- 傳統 2PL 不適合即時：**priority inversion**（低優先權交易拿著鎖，高優先權交易等——交易通常很長，等很久）、長時間阻擋、不考慮時間資訊、**死結**。
- 死結例子（和第 11 支影片的 PIP 死結一樣）：T1 鎖 d1 再鎖 d2；T2 鎖 d2 再鎖 d1；T1 優先權較高；T2 先鎖 d2，T1 鎖 d1 後要 d2 → 阻擋，T2 要 d1 → 死結。

⚠️ 老師先說 2PL「保證沒有死結」[23:31]，隨後補充「要依全域順序取得鎖」，又說「一般的 2PL **會**有死結」[25:34]。課本：2PL 會死結（上例）。依全域順序上鎖是避免死結的另一個條件，不是 2PL 本身保證的。

### 5.2 2PL-WP

[26:32]–[29:31]（課本 p.184–185）

**2PL-WP（wait promote）**——有 priority inheritance 的 2PL：

```
/* pri(T) denotes priority of transaction T*/
if(pri(TR)>Pri(TH)) then  /* TH holds the lock requested by TR */
      TR waits;
      TH inherits priority of TR;
else
      TR waits;
endif
```

- T_R 是請求者，T_H 是持有者。高優先權的 T_R 等，但 T_H **繼承** T_R 的優先權。
- 和純粹的 PIP 不同：高優先權交易在被擋期間被 abort 了，被提升的交易**保留**提升後的優先權直到它結束。
- 問題（老師、課本一致）：高負載 / 高資料競爭時，大多數交易都被拉到很高的優先權 → 大家一樣高 → 退化成一般 2PL。低負載時比 2PL 好。（老師：像 CPU 的 HLP，不必要地拉高優先權會擋住很多任務。）

⚠️ 老師說 2PL-WP「和純粹的 priority inheritance 不同，只在高優先權交易被擋期間被 abort 時**才套用**繼承」[27:00]。照課本和虛擬碼，繼承是**每次**高優先權交易等待時都發生；不同之處在於「高優先權交易 abort 後，被提升的交易**仍保留**提升的優先權」。老師的說法把條件弄反了。

### 5.3 2PL-HP

[29:31]–[32:31]（課本 p.185）

**2PL-HP（high priority）**，也叫 **priority abort（PA）**：

```
if (no conflict) then TR accesses D /* TR is the requesting transaction */
      else                          /* transaction TH is holding the data item, */
                                    /* resolve the conflict as follows. */
if (Pri(TR)>Pri(TH)) then abort  TH
      else TR waits for the lock;  /* TR blocks */
```

- 高優先權請求低優先權持有的鎖 → **直接 abort 持有者**。
- 沒有 priority inversion、也沒有死結（課本）。
- 實驗結果：2PL-HP **勝過** 2PL 和 2PL-WP——看似意外，因為 abort 會浪費工作。
- 老師：像可搶占的 CPU 排程，讓高優先權交易**立刻**執行，即使要犧牲低優先權交易做過的工作。（要看工作量，不是能證明的一般結論，但很多實際情境觀察到。）
- 課本的解釋：任務只是**間歇地**共享資源，但 2PL 下交易取得資料後要一直拿到結束或 abort——持有時間和交易生命期相當（通常很長）→ 有資源競爭的交易在 2PL 下其實是**序列執行**而不是可序列化執行 → 高優先權交易錯過 deadline 的機會比 2PL-HP 大。2PL 的連鎖 rollback 機會較少，但平均交易執行時間較長。

### 5.4 【課本補充】資料庫的 PCP

（p.185–186；課堂沒講）

- 和任務的 PCP 不同，**不用** priority inheritance；核心是在所有交易之間建立**全序**的優先權。每個資料物件有三個值：
  - **Read ceiling**：可能**寫**這個物件的最高優先權交易的優先權。
  - **Absolute ceiling**：可能讀或寫它的最高優先權。
  - **Read-write ceiling**：執行期動態設定——交易寫入時設成 absolute ceiling，讀取時設成 read ceiling。
- 規則：交易請求某資料物件，若且唯若它的優先權**高於所有**資料物件的 read-write ceiling 才給。
- 沒有死結、single blocking。優先權 ≤ read ceiling 的交易即使相容模式也不能讀（確保未來的高優先權交易不會被低優先權寫者擋住）；寫入後，原寫者結束前別人不能讀寫該物件。
- **Ex 7.4**：read ceiling 20、absolute ceiling 40；任何交易讀 d 後 read-write ceiling = 20 → 擋住需要讀 d 的交易。

---

## 6. 樂觀式：OCC

[32:31]–[41:30]（課本 §5.2，p.186–187）

- 交易不受檢查地自由存取資料；在**快要 commit 時**做**驗證測試（validation，也叫 certification）**：檢查這個交易和其他交易（已 commit 的、包括正要 commit 的）的存取，能不能組成可序列化的排程。

### 6.1 Forward OCC

- 交易自由讀寫，但更新存在**私有區域**（為了隔離性），**commit 時**才公開。
- commit 前要通過驗證：和其他（自它開始以來）已 commit 的交易有沒有衝突。沒衝突 → commit；有 → **abort**。
- 保證 atomicity 和 durability（課本）；因為寫入只在 commit 時發生，序列化順序就是 commit 的順序。

### 6.2 OCC-Sacrifice

- 明確考慮交易**優先權**；引入 **conflict set**：所有和正在驗證的交易衝突的**目前執行中**交易。
- 驗證時：conflict set 中有任何一個優先權**比自己高** → 自己 **abort** 並重新開始；否則（自己最高）→ 自己 commit，conflict set 裡的交易全部 abort 並重新開始。
- 問題：一個交易可能為了另一個交易犧牲，而那個交易之後又被犧牲……→ 浪費計算、錯過 deadline。

老師的例子【口頭】：驗證中的 V 因為 T1 優先權較高而被犧牲；之後 T2 做了某個存取和 T1 衝突，T2 進入驗證而且優先權更高 → T1 也被犧牲 → 連鎖。

### 6.3 OCC-BC（broadcast commit）

- 交易 commit 時**通知所有**執行中的交易；每個執行中的交易檢查自己和 commit 的交易有沒有衝突，有就**立刻 abort 自己**、重新開始。
- commit 的交易**不必**和已 commit 的交易檢查：如果衝突，它早就被 abort 了。
- 所以 OCC-BC 中，交易**一到驗證階段就保證 commit**。
- 老師：讓一般 OCC 加了一點悲觀的味道（廣播）。

課本：和 forward OCC 相比，比較早重新開始、浪費的計算較少，應該更能趕上 deadline；但**不考慮交易優先權**——明確考慮優先權可能更好（→ OCC-Sacrifice）。

**負載的影響**（老師與課本）：

- 低負載：大多數交易都能通過驗證。
- 高負載：大量交易驗證失敗 → 吞吐量大降、deadline miss 大增 → OCC 效能明顯下降。
- 老師的結論（類比 802.4 vs 802.5）：**低負載時 OCC 較好**（不需要事先檢查）；**高負載時悲觀式較好**（事先檢查減少浪費的工作和 abort）。

### 6.4 【課本補充】Speculative CC 與比較

（p.187）

- **SCC（speculative concurrency control）**：OCC 直到驗證階段才發現衝突，可能已來不及。SCC 在**每次讀寫**時檢查衝突；一偵測到衝突，就為衝突的交易各開一個 **shadow** 版本：primary 照 OCC 執行（忽略衝突），shadow 照悲觀式執行（遇到衝突就阻擋）。primary commit → 丟掉 shadow；primary abort → shadow 變成新的 primary 繼續以 OCC 執行。所有更新都在本地副本上，commit 前不可見。
- 比較（課本 Fig.1）：零競爭時三類一樣；**低競爭時 OCC 勝過悲觀式**；負載（衝突）高時**悲觀式較好**；交易 deadline 很緊、負載中等時 **SCC** 比兩者都好。
- 2PL-HP 中交易可能被一個之後會被 abort 的交易重新開始或擋住 → 效能下降；OCC-BC 只有驗證中的交易會讓別人重新開始，而它們保證 commit → 所有重新開始都有用。OCC 不阻擋、沒有死結——對即時應用很有吸引力。

---

## 7. 【課本補充】商用即時資料庫

（p.187–188）

- 要避免任何會帶來不可預測延遲的東西：**磁碟 I/O、訊息傳遞、垃圾回收**。
- 所以即時資料庫常是**記憶體內（in-memory）資料庫**：完全不碰磁碟 I/O；設計簡化減少訊息傳遞；仍用磁碟做 log 和定期 checkpoint 保持完整性；有些提供資料複製以達高可用。
- 例：McObject 的 **eXtremeDB**：用於電信、工廠自動化、程序控制、消費電子、醫療設備；資料庫**直接映射到應用的位址空間**（不需要昂貴的緩衝管理）；runtime 直接連結進應用（執行路徑上沒有 RPC）；**自己的記憶體管理器**（不依賴 OS 的記憶體管理，也就不會因分頁換入換出成為瓶頸）；支援 **5** 個交易優先權等級做 deadline 管理。
- 課本 §3.1（p.181）：在硬即時應用用資料庫看似不可行（磁碟延遲、各種協定的回應時間難預測、rollback 連鎖），但有轉機：記憶體內資料庫讓很多問題消失；即時應用的交易集合簡單、事先已知（例如週期性感測器更新），每次用同樣種類與數量的資料 → 可以規劃資源，達到確定性的執行。

---

## 8. 【原始碼】Linux kernel 裡的樂觀並行控制：seqlock

本講的 OCC（先做、最後驗證、衝突就重來）在 Linux kernel 裡有一個每天都在用的版本：**seqlock / seqcount**。

`include/linux/seqlock.h:3`

```c
/*
 * seqcount_t / seqlock_t - a reader-writer consistency mechanism with
 * lockless readers (read-only retry loops), and no writer starvation.
```

讀 64 位元 jiffies 的例子：

`kernel/time/jiffies.c:48`

```c
u64 get_jiffies_64(void)
{
	unsigned int seq;
	u64 ret;

	do {
		seq = read_seqcount_begin(&jiffies_seq);
		ret = jiffies_64;
	} while (read_seqcount_retry(&jiffies_seq, seq));
	return ret;
}
```

對照：

| 本講 | seqlock |
|---|---|
| 樂觀式：讀者不上鎖、自由讀 | `read_seqcount_begin()` 記下序號，直接讀資料 |
| 驗證測試 | `read_seqcount_retry()`：序號變了（期間有寫者）→ 衝突 |
| 驗證失敗 → abort 重來 | `while (...)` 迴圈重讀 |
| 寫者 | 寫者之間仍要互斥（seqlock_t 內含 spinlock），寫的時候序號加一（變奇數）、寫完再加一 |
| 低競爭時樂觀式最好 | jiffies 每個 tick 才寫一次、讀非常頻繁 → 讀者幾乎從不重試；寫者也不會被讀者餓死 |

另外第 18 支影片筆記的時間（`clock_gettime` 的 vDSO 資料）、第 16 支影片 `tick_periodic()` 裡的 `write_seqcount_begin(&jiffies_seq)`，就是這個機制的寫入端。（kernel 另一個「讀者不擋寫者」的機制是 RCU，OS 課 `../linux-os/` 第 23 講。）

課本說即時資料庫要放在記憶體裡、自己管記憶體——對應前面幾講的 `mlock`（第 17 支影片筆記）。

---

## 9. 【實測】時間有效性檢查與並行控制模擬

**時間有效性**（課本例題）：

```python
def abs_valid(now, ts, avi): return now - ts <= avi
def rel_consistent(ts_list, rvi): return max(ts_list) - min(ts_list) <= rvi
```

```
Ex7.1 d: ts=2500 avi=100 now=2700 -> absolutely valid? False
Ex7.2 temp ts=95, pressure ts=97, Rrvi=2 -> True ; pressure ts=92 -> False
Ex7.3 now=2600 abs: [True, True, True]  rel (Rrvi=100): False
```

（「任兩筆時間戳記差 ≤ R_rvi」等同於「最大減最小 ≤ R_rvi」。）三題結果都和課本一致。

**並行控制模擬**：400 個交易、每個依序寫 4 個資料項（每項 1 個時間單位），deadline = 到達 + 8–20；以 **EDF** 決定優先權（deadline 越早越高）；超過 deadline 就丟掉（firm）。改變資料庫大小來改變競爭程度；比較 2PL（等待，用逾時打破死結）、2PL-HP、OCC-BC（commit 時 abort 所有衝突的執行中交易）。括號內是平均每個交易重新開始的次數。

```
C = unlimited transactions can run per tick
  DB size 400: 2PL  0.0% (0.00)   2PL-HP  0.0% (0.04)   OCC-BC  0.0% (0.06)
  DB size  40: 2PL 29.0% (0.29)   2PL-HP  0.5% (0.43)   OCC-BC  2.0% (0.74)
  DB size  10: 2PL 93.2% (0.93)   2PL-HP 68.2% (1.64)   OCC-BC 35.2% (3.47)
C = 5 transactions can run per tick
  DB size 400: 2PL  0.0% (0.00)   2PL-HP  0.0% (0.03)   OCC-BC  0.0% (0.06)
  DB size  40: 2PL 26.5% (0.27)   2PL-HP  3.0% (0.36)   OCC-BC  8.8% (0.54)
  DB size  10: 2PL 92.8% (0.93)   2PL-HP 68.2% (1.51)   OCC-BC 48.0% (1.47)
```

看到什麼：

- **低競爭**（DB 400）：三者都沒錯過；OCC-BC 的重新開始也很少——低負載時樂觀式不吃虧。
- **中競爭**（DB 40）：**2PL 錯過 29%**，2PL-HP 只有 0.5–3%——和課本「2PL-HP 勝過 2PL」一致：2PL 下拿到鎖的低優先權交易一路拿到底，高優先權交易只能等。
- 處理能力有限（每單位時間只能跑 5 個）時，OCC-BC 在中競爭變得比 2PL-HP 差（8.8% vs 3.0%）：重新開始的工作要佔用有限的處理能力——這就是課本說樂觀式在負載高時變差的原因。
- **高競爭**（DB 10）：這個簡化模型裡 OCC-BC 仍比 2PL-HP 少錯過。⚠️ 這和課本「高負載時悲觀式較好」不一致。模型很簡化（所有存取都是寫、交易很短、只考慮處理能力不考慮 I/O、firm deadline 會提早丟掉落後的交易），只能說明趨勢；課本 Fig.1 的結論來自它引用的效能研究，這裡沒有重現。

---

## 10. 一頁速查表

| 主題 | 重點 |
|---|---|
| 差別 | 時間性資料、操作的時間限制、效能指標（錯過 deadline 的交易數 vs 平均回應時間） |
| 表示 | d = (value, avi, timestamp)；(120, 5, 100) → 100–105 有效 |
| Absolute validity | now − timestamp ≤ avi |
| Relative consistency | 集合中任兩筆 \|ts − ts′\| ≤ R_rvi（反飛彈：位置配速度） |
| ACID | atomicity、consistency、isolation、durability |
| Cascaded abort | 讀了被 abort 交易的值也要 abort；只讀已 commit 的值即可避免 |
| Serializability | 結果等同某種序列執行；衝突 = 同一位置、至少一個寫 |
| 2PL | growing / shrinking；strict 2PL；priority inversion、死結 |
| 2PL-WP | 持有者繼承請求者的優先權；高負載退化成 2PL |
| 2PL-HP | 高優先權直接 abort 持有者；無 inversion、無死結；實驗最好 |
| DB 的 PCP（課本） | read ceiling、absolute ceiling、read-write ceiling；全序、無繼承 |
| Forward OCC | 私有區域更新、commit 時驗證，衝突就 abort |
| OCC-Sacrifice | conflict set 有更高優先權就犧牲自己，否則犧牲對方；可能連鎖 |
| OCC-BC | commit 時廣播，衝突者自我 abort；到驗證就保證 commit |
| SCC（課本） | shadow 版本；deadline 緊、負載中等時最好 |
| 結論 | 低負載樂觀式、高負載悲觀式；即時資料庫多為記憶體內（eXtremeDB） |
| Linux | seqlock：讀者樂觀讀、序號驗證、失敗重讀（`get_jiffies_64`） |
| 實測 | 中競爭 2PL 錯過 29%、2PL-HP 0.5%；處理能力有限時 OCC-BC 在中競爭變差 |

---

## 11. 與後續講次的連結：整門課的回顧

這是最後一講，沒有後續講次；下表把本講的觀念連回前面各講，並回顧整門課。

| 本講觀念 | 前面在哪裡出現過 |
|---|---|
| **2PL-WP 的優先權繼承、資料庫版 PCP** | **第 10–13 支**：PIP、HLP、PCP |
| **2PL-HP 像可搶占排程、EDF 優先權** | **第 6 支**：EDF |
| **時間性資料、時間戳記** | **第 16–17 支**：時鐘同步 |
| **記憶體內資料庫、避開 page fault** | **第 17 支**：memory locking |
| **低負載樂觀、高負載悲觀的取捨** | **第 28 支**：低頻寬 802.5、高頻寬 802.4 |

| 播放清單 | 主題 |
|---|---|
| 1–4 | 導論、設計空間、時間限制建模、排程基本名詞 |
| 5–9 | Clock-driven、EDF、RMA 及其延伸、jitter 與有限優先權 |
| 10–13 | Priority inversion、PIP、HLP、PCP |
| 14–17 | 分散式與多處理器排程、時鐘同步 |
| 17–26 | RTOS：要求、計時器、kernel 架構、Windows NT、POSIX、各種 RTOS、Linux 系 RTOS、Windows CE、benchmark |
| 27–30 | 即時通訊：QoS、CAN、LAN 協定、流量整形、service discipline、IntServ / DiffServ |
| 31 | 即時資料儲存 |

貫穿全課的一條線：**排程**——CPU 上排任務（EDF、RMA）、鎖上排資源（PIP、PCP）、網路上排封包（802.4、802.5、WFQ、Stop-and-Go）、資料庫裡排交易（2PL-HP、OCC）；每一層都在同樣的取捨裡選擇：要不要讓高優先權立刻搶占、要不要刻意等待以換取可預測性、用多少資源預留換保證。

---

## 附：本講一句話總結

> **即時資料儲存和傳統資料庫的差別在於資料會過期、操作有時間限制、效能看的是錯過 deadline 的交易數：每筆資料以 (value, avi, timestamp) 表示，要滿足 absolute validity（現在時間減時間戳記不超過 avi）和 relative consistency（一起用來推導的資料，任兩筆時間戳記差不超過 R_rvi，例如反飛彈系統的位置和速度必須來自同一時刻）。並行交易要靠 ACID 與 serializability 保證正確，而 rollback 可能連鎖、讓交易錯過 deadline。悲觀式的 2PL 會 priority inversion 和死結，2PL-WP 讓持有者繼承請求者的優先權但高負載時退化，2PL-HP 讓高優先權交易直接 abort 低優先權持有者，沒有 inversion 也沒有死結，實驗上表現最好；樂觀式則讓交易自由執行、commit 前驗證：forward OCC 衝突就 abort，OCC-Sacrifice 依 conflict set 中的優先權決定犧牲誰但可能連鎖，OCC-BC 在 commit 時廣播讓衝突者自我 abort，使交易一到驗證階段就保證 commit。一般而言低負載時樂觀式較好、高負載時悲觀式較好（課本另有 SCC 與資料庫版的 PCP），而商用即時資料庫多半放在記憶體裡、避開磁碟、訊息傳遞與垃圾回收。Linux kernel 每天都在用的 seqlock 正是樂觀並行控制：讀者不上鎖地讀、用序號驗證、衝突就重讀；模擬也顯示中等競爭時 2PL 錯過 29% 的 deadline，而 2PL-HP 只錯過 0.5%。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 31 錄影逐字稿（YouTube 自動字幕，42:12）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch7 整理。原始碼為 Linux v6.2.12；實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
