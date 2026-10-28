# Real Time Systems 第十六支影片完整筆記：時鐘同步（Lecture 17）

> **課程**：Real Time Systems — Lecture 17：Clock Synchronization（播放清單第 16 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（39:41，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch4 §4–5（PDF p.101–103）
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
- [1. 時鐘拿來做什麼](#1-時鐘拿來做什麼)
- [2. 好時鐘的性質](#2-好時鐘的性質)
- [3. 時間戳記 vs 因果順序](#3-時間戳記-vs-因果順序)
- [4. 為什麼時鐘會漂移：石英振盪器](#4-為什麼時鐘會漂移石英振盪器)
- [5. UTC 與重新同步的間隔](#5-utc-與重新同步的間隔)
- [6. External vs internal 同步](#6-external-vs-internal-同步)
- [7. GPS](#7-gps)
- [8. Centralized 同步](#8-centralized-同步)
- [9. Client 向 time server 要時間（Cristian 式）](#9-client-向-time-server-要時間cristian-式)
- [10. 【原始碼】Linux 怎麼「慢慢調」時鐘](#10-原始碼linux-怎麼慢慢調時鐘)
- [11. 【實測】本機時鐘的漂移與 NTP 狀態](#11-實測本機時鐘的漂移與-ntp-狀態)
- [12. 一頁速查表](#12-一頁速查表)
- [13. 與後續講次的連結](#13-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

分散式即時系統裡每個節點有自己的時鐘，會慢慢分歧。即時系統看的是**實際時間**（不是邏輯時間），所以同步更重要。

1. 時鐘的用途與好時鐘的性質（正確、漂移有界、單調、chronoscopicity）。
2. 為什麼會漂移（石英）、標準時間 UTC、多久要同步一次（δ / 2ρ）。
3. External vs internal 同步；GPS。
4. Centralized 同步；client 向 time server 要時間時怎麼補償網路延遲；時鐘太快時不能往回調，要**慢慢調**。

---

## 1. 時鐘拿來做什麼

[01:01]–[02:31]

| 用途 | 例子 |
|---|---|
| **Timeout** | job 開始時設一個 t 秒的計時器；t 秒還沒做完 → 判定錯過 deadline |
| **時間戳記（time stamping）** | 事件送出時蓋上目前時間；接收者能知道訊息**多舊**、該怎麼處理 |
| **動作排序** | 紅綠燈：綠 20 秒 → 黃 → 紅，固定順序 |

---

## 2. 好時鐘的性質

[02:31]–[06:31]

設 C(t) 是時鐘在實際時間 t 時顯示的值，S(t) 是另一個時鐘。

| 性質 | 式子 | 意思 |
|---|---|---|
| **正確（correctness）** | \|S(t) − C(t)\| < δ | 兩個時鐘的偏差有界 |
| **漂移有界（drift）** | \|dC/dt − 1\| < ρ | 走的速度接近 1（每過 1 秒，時鐘也前進約 1 秒） |
| **單調（monotonicity）** | t > t′ ⇒ C(t) > C(t′) | 時間**絕不倒流**；調整時鐘也不能違反 |
| **Chronoscopicity** | C(t) − C(t′) ≈ t − t′ | 不能突然大跳；修正時要**漸進** |

- 違反單調：很多軟體會出錯。
- 老師順帶提到 **Y2K / rollover 問題**：2000 年以前很多程式用兩位數存年份（75 代表 1975），擔心 99 之後翻成 00。90 年代末花了大量力氣改程式，最後沒出大事——「大概正是因為花了那麼多力氣」。
- 時鐘的值會變有兩個原因：時間自然前進，以及我們為了修正漂移而調整。後者正是 chronoscopicity 要約束的。

---

## 3. 時間戳記 vs 因果順序

[06:31]–[08:01]

- 傳統分散式系統用**因果順序（causal ordering）**：事件 A 導致送出訊息、對方收到後發生事件 B → A **happens before** B。真正的時鐘時間不重要，得到的是**偏序**。
- 本講談的是**真實時鐘**：顯示實際時間的時鐘。

---

## 4. 為什麼時鐘會漂移：石英振盪器

[08:01]–[15:30]

- 兩個時鐘幾乎不可能走得一樣快。
- **Clock skew（時鐘偏差）**：時鐘讀數與實際時間的差。會隨時間**增加**。
- 久了之後，依賴時鐘的操作（timeout、時間戳記）就失去意義 → 要**定期修正**：同步（synchronization）或重新同步（resynchronization）。

**壓電效應**：

- 1880 年由 **Pierre 與 Jacques Curie** 發現。
- 石英晶體加電壓會**變形**；電壓移除後想恢復原狀，又產生一個小電位差 → 機械 ↔ 電的回饋 → 可做成電子電路裡的**振盪器**。
- 石英晶體像一個 **RLC 電路**，有非常精準的共振頻率。等效電路：C₁、L、R 串聯，再並聯一個電容。
- 輸出是穩定的方波 → 時鐘。

⚠️ 年份：逐字稿 [11:02] 說石英晶體「在 **1921** 年被用來做第一個石英鐘」，[12:00] 又說「**1929** 年……花了大約 49 年才做出第一個石英晶體時鐘」。兩處互相矛盾（1880 + 49 = 1929），講義沒有這段可對照，待補。

- 音叉形狀的共振器，調在 **32,768 Hz**（32 kHz）。
- 室溫下很準：漂移約 **6 ppm**（每一百萬秒差 6 秒）。很多手錶一天快或慢半秒；好的共振器 10 年才差 1 秒。
- 影響頻率的因素：老化、環境**溫度**、**加速度**、晶體相對於晶軸的**切割角度**、尺寸。

老師的照片例子【口頭】：兩支錶在 2011-09-18 早上 8 點對準，到 2011-10-23（一個月又五天）分別偏了 84 秒和 108 秒。老師強調這不是在評價品牌，而且這是舊資料，現在的錶準得多，一年頂多差幾秒。

---

## 5. UTC 與重新同步的間隔

[15:30]–[18:30]

- **UTC（Universal Coordinated Time）**：全世界校準時鐘的標準。
- 理想時鐘對 UTC 畫圖是 45° 線：dC/dt = 1。慢的 < 1、快的 > 1。
- 典型漂移 **6–10 ppm**，製造商規格會標出漂移率 ρ。

**多久同步一次**：

- 每個時鐘相對 UTC 最多漂 ρ·Δt，兩個時鐘可能一快一慢 → 兩者最多差 **2ρΔt**。
- 要保證兩個時鐘差距永遠不超過 δ → 每 **δ / (2ρ)** 秒重新同步一次（同步點上兩者相同，之間只有有限的漂移）。

- 取得 UTC：網際網路時間伺服器（再依時區加偏移）；需要極高精度時用 **GPS** 衛星；或專門的無線電台。
- 老師：UTC 在巴黎維護，是全世界實驗室約 **70** 個原子鐘的平均。

【課本補充】（p.102）UTC 以巴黎維護的 **TAI（International Atomic Time）** 為基礎，TAI 是全世界實驗室多個原子鐘的平均。⚠️ 原子鐘的數量課本沒寫，「70」只來自老師口頭，待補。

【課本補充】（p.102）很多分散式即時應用其實只需要各時鐘**彼此一致**，不一定要和世界標準時間一致。

---

## 6. External vs internal 同步

[19:03]–[21:00]

| | External | Internal |
|---|---|---|
| 基準 | 系統**外部**的時鐘（UTC 來源） | 系統**內部**的某個時間基準（例如其中一台電腦），沒有「絕對真相」 |
| 要求 | \|C_UTC(t) − Cᵢ(t)\| 有界 | 任兩個時鐘 \|Cᵢ(t) − Cⱼ(t)\| < Δ |
| 例子 | 桌機、筆電、手機用 time server（**NTP**，network time protocol） | 一群電腦彼此同步 |
| 優缺點 | 好實作、（若每個節點有 GPS）不需通訊；但每個節點都裝 GPS 接收器很貴 | — |

---

## 7. GPS

[21:00]–[32:00]

- 多顆繞行的導航衛星，每顆都有**原子鐘**，知道自己的**精確時間與位置**，並持續廣播。
- 接收器的未知數：自己的 **x、y、z** 和**時鐘偏差 Δt** → 4 個未知數 → 至少要 **4 顆**衛星的資料；更多顆可提高精度。
- 每顆衛星 i 送出 (xᵢ, yᵢ, zᵢ, tᵢ)。訊號以速度 c（光速，視空氣折射率修正）傳播：

  **√((x − xᵢ)² + (y − yᵢ)² + (z − zᵢ)²) = c · (t_recv − tᵢ + Δt)**

  四個方程式解出位置和 Δt。

- 結果：經緯度、高度；多次讀數可得速度、加速度、時鐘漂移。
- 每個訊息有獨特的 **pseudorandom code**，用來判斷是哪個訊息、花了多久到達。
- 衛星上的原子鐘很貴（十萬美元以上）；**接收器的時鐘不必很準**，因為 Δt 本來就是要解的未知數。

歷史【口頭】：美國國防部建立，1969 年後逐步開放；1973 年發展 NAVSTAR；約 2000 年起軍用等級精度開放給所有人 → GPS 應用爆發。

**Assisted GPS**：用手機基地台代替衛星。不要高度 → 3 座基地台三角定位；要高度 → 4 座。打開手機定位時多半是這種；有些手機、平板才有真的 GPS。便宜，但只在有基地台的地方能用。

星座（2000 年前後）：

| 項目 | 值 |
|---|---|
| 衛星數 | 24 顆，6 個軌道面，每面 4 顆 |
| 高度 | 20,200 km，傾角 55° |
| 軌道週期 | 約 12 小時（老師說 11 小時 55 分） |
| 地面任一點可見 | 目標 5–8 顆 |
| 衛星 | 4 個原子鐘、電池、太陽能板；功率約 1 kW；S-band 天線控制、12 個 L-band 天線對使用者；翼展 38 英尺；壽命 10 年 |

為什麼不用同步軌道：衛星跑太快會很快失去訊號，所以不要太低；同步軌道太遠，訊號太弱。

⚠️ 老師說同步軌道「離**地心**約 36,000 km」[27:32]。約 36,000 km 是離**地表**的高度；離地心約 42,000 km（地球半徑約 6,400 km）。

⚠️ 衛星重量逐字稿是「110」，沒有單位，待補。

應用：飛機、地面車輛、船（都是分散式即時系統的例子）、外送、汽車……任何需要準確時間與位置的系統。

---

## 8. Centralized 同步

[32:00]–[35:00]（課本 §5，p.102–103）

Internal 同步有兩類：信任某台機器、由它廣播時間（如 NTP 的 time server）；或不需要參考機器、**把所有時鐘平均**當新時間（純分散式，下一講）。

**Centralized**：

```
            Master（time server）
          ╱    ╱       ╲     ╲
        C1    C2  ...  Cn-1   Cn   ← slave clocks
```

- Master 每 ΔT 廣播一次自己的時間；slave 收到就設成這個時間。
- ΔT 太小：訊息太多。太大：漂移累積太多。
- 漂移率上限 ρ（製造商規格；**無單位**：每單位時間漂多少時間）。
  - 任一 slave 相對 master 最多漂 **ρΔT**。
  - 兩個 slave 可能一正一負 → 彼此最多差 **2ρΔT**。

【課本補充】Ex 4.2（p.103）：ρ = 5×10⁻⁶，6 個時鐘，要求任兩個差距 ε ≤ 1 ms → 2ρΔT < ε → ΔT < 10⁻³ / (2 × 5×10⁻⁶) = **100 s**。

【課本補充】（p.103）上面忽略了**通訊時間**和**設定時鐘所需的時間**。若不補償，同步後的時間會相對外部時鐘越來越慢（彼此之間仍在界限內）。課本把「漂多快」留作習題。

---

## 9. Client 向 time server 要時間（Cristian 式）

[35:00]–[39:30]

設定：一台 time server 有 UTC 接收器，其他機器向它同步。（老師說這「不靠中央」，但例子裡仍有一台 server；可以是 client–server，也可以是兩個 slave 之間。）

1. 每 **δ / (2ρ)** 秒，client 在 T₀ 送請求給 server。
2. Server 處理後回傳目前的 UTC 值 C_UTC。
3. Client 在 T₁ 收到。

- 來回時間很短：直接把 C_UTC 設成 client 的時間。
- 通常不短 → 要估計單程延遲：假設請求和回應花的時間差不多，**單程 ≈ (T₁ − T₀) / 2**；若知道 server 處理（中斷處理）時間 I，可以先扣掉：(T₁ − T₀ − I) / 2。
- 收到時，server 時間應是 C_UTC + 單程延遲；和 client 的 T₁ 比，差值就是**時鐘偏差**，要修正的量。

修正方向：

| Client 時鐘 | 做法 |
|---|---|
| **慢** | 直接往前調（簡單） |
| **快** | **不能往回調**（違反單調）→ 每個 tick **少加一點時間**，慢慢讓它追回來。例：過了 5 秒只加 4 秒 |

時鐘同步還沒講完，下一講繼續。

---

## 10. 【原始碼】Linux 怎麼「慢慢調」時鐘

老師說的「快的時鐘每 tick 少加一點」，就是 Linux `adjtime()` / NTP 的 **slew（漸進調整）**。kernel 每秒最多調 500 μs：

`kernel/time/ntp.c:43`

```c
#define MAX_TICKADJ		500LL		/* usecs */
#define MAX_TICKADJ_SCALED \
	(((MAX_TICKADJ * NSEC_PER_USEC) << NTP_SCALE_SHIFT) / NTP_INTERVAL_FREQ)
```

`kernel/time/ntp.c:473`（在每秒呼叫一次的 `second_overflow()` 裡）

```c
	if (!time_adjust)
		goto out;

	if (time_adjust > MAX_TICKADJ) {
		time_adjust -= MAX_TICKADJ;
		tick_length += MAX_TICKADJ_SCALED;
		goto out;
	}

	if (time_adjust < -MAX_TICKADJ) {
		time_adjust += MAX_TICKADJ;
		tick_length -= MAX_TICKADJ_SCALED;
		goto out;
	}

	tick_length += (s64)(time_adjust * NSEC_PER_USEC / NTP_INTERVAL_FREQ)
							 << NTP_SCALE_SHIFT;
	time_adjust = 0;
```

- `time_adjust` = 還要調多少（μs）。每秒最多 ±500 μs，攤到這一秒的每個 tick（改 `tick_length`）。
- 時鐘太快（要往回調）→ `time_adjust < 0` → 每個 tick 少走一點 → 時間仍單調前進，只是走得稍慢——就是老師的「5 秒只加 4 秒」，只是幅度小很多（每秒最多 0.05%）。
- 頻率修正也有上限：`include/linux/timex.h:136` `#define MAXFREQ 500000 /* max frequency error (ns/s) */`，即 ±500 ppm。

另外 Linux 提供兩種時鐘：

| 時鐘 | 性質 | 對應本講 |
|---|---|---|
| `CLOCK_REALTIME` | 牆上時間；`settimeofday` 等可以直接**跳**（包括往回） | 可能違反單調與 chronoscopicity |
| `CLOCK_MONOTONIC` | 不會往回；會被 NTP 的 slew 影響速度，但不會被跳 | 滿足**單調** |

所以量 timeout、deadline 一定要用 `CLOCK_MONOTONIC`（第一支影片的週期迴圈實測就是用它）。

---

## 11. 【實測】本機時鐘的漂移與 NTP 狀態

用 `adjtimex()`（`modes = 0` 唯讀，不需要 root）讀 kernel 的 NTP 狀態：

```c
struct timex tx = { .modes = 0 };                    /* read-only: no root needed */
int st = adjtimex(&tx);
printf("freq   = %.3f ppm   (kernel's correction of this machine's oscillator)\n", tx.freq / 65536.0);
```

結果：

```
adjtimex state=0 (TIME_OK)
freq   = -10.180 ppm   (kernel's correction of this machine's oscillator)
offset = -58125151 ns, maxerror = 136000 us, esterror = 0 us, tick = 10000 us, status=0x2001
CLOCK_REALTIME  = 1792947944.188814740 (wall clock, can be stepped)
CLOCK_MONOTONIC = 34076.357004807 (never goes backwards)
resolution = 1 ns
```

`timedatectl timesync-status`（systemd-timesyncd，就是一個 NTP client）：

```
       Server: 185.125.190.57 (ntp.ubuntu.com)
Poll interval: 34min 8s (min: 32s; max 34min 8s)
      Stratum: 2
       Offset: -98.926ms
        Delay: 350.248ms
       Jitter: 103.735ms
```

看到什麼：

- **freq = −10.18 ppm**：kernel 估計這台機器的振盪器快了約 10 ppm，正在把它調慢——落在老師說的「6–10 ppm」範圍上緣。一天約 0.88 秒。
- **Delay 350 ms**：就是 §9 的來回時間 T₁ − T₀。NTP 用一半來估計單程延遲，和老師講的方法一樣。
- **Offset −99 ms**：本機比 server 快約 0.1 秒；`status` 裡沒有 STA_UNSYNC，`offset` 欄位顯示 kernel 正在用 slew 慢慢追回（不是往回跳）。
- **Poll interval 34 分鐘**：用 δ/(2ρ) 反推，ρ ≈ 10⁻⁵ 時每 34 分鐘同步一次，兩個這樣的時鐘最多差約 2 × 10⁻⁵ × 2048 s ≈ 41 ms；但頻率誤差已經被 `freq` 補償掉大部分，實際漂移小得多，所以 NTP 敢把間隔拉長。
- **Jitter 104 ms**：網路延遲不穩定，這正是 Cristian 式「假設來回對稱」的弱點。

---

## 12. 一頁速查表

| 主題 | 重點 |
|---|---|
| 用途 | Timeout、時間戳記（訊息年齡）、動作排序 |
| 性質 | 正確 \|S − C\| < δ；漂移 \|dC/dt − 1\| < ρ；單調；chronoscopicity（不突跳） |
| 石英 | 壓電（Curie 兄弟 1880）；32,768 Hz；約 6 ppm；受老化、溫度、加速度、切割角度影響 |
| 重新同步 | 兩時鐘最多差 2ρΔt → 每 δ/(2ρ) 同步一次 |
| UTC | 以巴黎的 TAI（原子鐘平均）為基礎；來源：網路時間伺服器、GPS、無線電台 |
| External / internal | 對外部 UTC 有界 / 彼此有界 |
| GPS | 4 未知數 (x, y, z, Δt) → 4 顆衛星；接收器時鐘不必準 |
| Centralized | Master 每 ΔT 廣播；slave 間最多差 2ρΔT（Ex 4.2：100 s） |
| Cristian 式 | 單程 ≈ (T₁ − T₀)/2；快的時鐘不能倒退，要慢慢調 |
| Linux | slew 每秒最多 500 μs、頻率 ±500 ppm；CLOCK_MONOTONIC 不倒退 |
| 實測 | 本機 −10.18 ppm；NTP 來回 350 ms、offset −99 ms、每 34 分鐘同步 |

---

## 13. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **分散式（平均）同步、Byzantine 時鐘** | **第 17 支**：Distributed Clock Synchronization and RTOS Fundamentals |
| **計時器、時鐘中斷、解析度** | **第 18 支**：Hardware, Timers, and OSes |
| **POSIX 時鐘（CLOCK_REALTIME / MONOTONIC）** | **第 21 支**：POSIX standards |
| **網路延遲的界限** | **第 27–30 支**：Real Time Communication |

---

## 附：本講一句話總結

> **分散式即時系統靠時鐘做 timeout、時間戳記和動作排序，好的時鐘要偏差有界、漂移率 |dC/dt − 1| < ρ、絕不倒流（單調）而且修正時不能突跳；但電腦時鐘靠石英壓電振盪（32,768 Hz、約 6–10 ppm），必然漂移，兩個時鐘最多差 2ρΔt，所以要每 δ/(2ρ) 秒重新同步一次。標準是以巴黎原子鐘平均為基礎的 UTC，可經由網路時間伺服器、無線電或 GPS 取得——GPS 接收器用 4 顆衛星的位置與時間解出自己的 x、y、z 和時鐘偏差。同步可以是對外部 UTC（external，如 NTP）或彼此之間（internal）；centralized 做法由 master 每 ΔT 廣播（slave 間最多差 2ρΔT，課本例題要求 1 ms 時 ΔT < 100 s），client 向 time server 要時間時用來回時間的一半估計單程延遲，而時鐘太快時不能往回調、只能每個 tick 少加一點。Linux 的 NTP slew 每秒最多調 500 μs 正是這樣做，CLOCK_MONOTONIC 保證不倒退；實測本機振盪器快約 10 ppm，NTP 量到 350 ms 的來回延遲和 −99 ms 的偏差，正以 slew 慢慢修正。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 17 錄影逐字稿（YouTube 自動字幕，39:41）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch4 §4–5 整理。原始碼為 Linux v6.2.12；實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
