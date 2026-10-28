# Real Time Systems 第二十六支影片完整筆記：RTOS 的 benchmark（Lecture 27）

> **課程**：Real Time Systems — Lecture 27：Benchmarking RTOSs（播放清單第 26 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（40:39，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch5 §8（PDF p.131–135）
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
- [1. 感測器節點與 TinyOS](#1-感測器節點與-tinyos)
- [2. 傳統的效能指標為什麼不夠](#2-傳統的效能指標為什麼不夠)
- [3. Rhealstone](#3-rhealstone)
  - [3.1 各項指標](#31-各項指標)
  - [3.2 Process dispatch time 的八個部分](#32-process-dispatch-time-的八個部分)
  - [3.3 【課本補充】Rhealstone 的公式與缺點](#33-課本補充rhealstone-的公式與缺點)
- [4. 其他要測的 RTOS 服務、tri-dimensional measure、中斷處理開銷](#4-其他要測的-rtos-服務tri-dimensional-measure中斷處理開銷)
- [5. Deterministic benchmark：timer jitter、response、bin time](#5-deterministic-benchmarktimer-jitterresponsebin-time)
- [6. Latency benchmark：sync、message、signal](#6-latency-benchmarksyncmessagesignal)
- [7. 【課本補充】量 kernel 可搶占性：TRIANG](#7-課本補充量-kernel-可搶占性triang)
- [8. 【實測】在本機跑一組迷你 benchmark](#8-實測在本機跑一組迷你-benchmark)
- [9. 結語](#9-結語)
- [10. 一頁速查表](#10-一頁速查表)
- [11. 與後續講次的連結](#11-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

我們看了那麼多 RTOS，怎麼知道它們真的符合即時要求？要 **benchmark**：

1. 典型的即時硬體：感測器節點；常用（但不是即時）的 TinyOS。
2. MIPS、FLOPS、SPEC 等傳統指標為什麼不夠。
3. **Rhealstone**：任務切換、搶占、中斷延遲、semaphore shuffling、priority inversion、datagram throughput、process dispatch 等。
4. 其他：tri-dimensional measure、中斷處理開銷。
5. **Deterministic** benchmark（timer jitter、response、bin time）與 **latency** benchmark（sync、message、signal）。

---

## 1. 感測器節點與 TinyOS

[00:31]–[09:01]

**典型的即時硬體：感測器節點（mote）**：

- 小主機板、少量感測器（溫度、光）、通訊能力（簡單的無線電、藍牙）。
- 很簡單的 CPU（例如 **4 MHz、8 位元**）、很少的 RAM（**0.5 KB 到幾 KB**）。
- 例：到處佈署收集**空氣污染**資料、傳回中央伺服器。一般不太需要即時；但如果裝在移動的公車上，快速經過不同污染區域，就需要時間保證。
- 也用在化工廠、核電廠：回應時間要求嚴格得多，但硬體相似，都要在時間限制內做決定。
- 硬體通常簡單、用現成元件組裝。節點散佈成**感測器網路**；資料可能直接送中央，或因為中央太遠而**多跳（multi-hop）**互相轉送——節點可能移動、網路會變化，叫 **ad hoc sensor network**。

軟體的挑戰：省電、記憶體小、專為應用設計；多條高速資料流（無線電資料、感測器資料）；即時查詢（感測實體環境並即時送出）、即時回饋（感測與致動）；緩衝很少，資料要**即時處理、即時送出**。

**TinyOS**：

- **不是**即時 OS，但非常流行，所以拿來討論——老師：很多情況下實際用的不是 RTOS，而是 TinyOS 這種頂多軟即時的 OS。
- 給記憶體極少的小硬體；無線電硬體幾乎沒有緩衝 → **錯過 deadline 就丟資料**；不提供硬即時保證。
- 專為感測器網路設計；**事件驅動**（像 ROS：事件來了反應，然後安靜）。
- 記憶體小、context switch 開銷很低、大多 **FIFO** 排程、**不可搶占**、資源利用有效率：工作做完就讓處理器**睡覺**。

架構：

- **沒有 kernel**：應用和「kernel」全部放一起，可以直接操作硬體。
- 沒有程序管理（只有一個程序）、沒有虛擬記憶體（單一位址空間）、**不動態配置記憶體**（負擔不起記憶體管理器的複雜度）、沒有軟體信號或例外（沒有 user / kernel 邊界，用函式呼叫）。
- 一個**元件圖**：元件呼叫元件；排程器依先後順序以函式呼叫叫用適當的元件；全部編譯成一個執行檔。
- 單一堆疊：HAL → 致動、感測、通訊模組（三大角色）→ 應用（使用者元件）→ 最上層的排程器 / 分派器。

| TinyOS 的優點 | 不是即時的原因 |
|---|---|
| 記憶體小、不可搶占、很省電、模組化、事件驅動 | 不支援 EDF、RMS，只有 FIFO；任務多於能承受時，連一點點暫時過載的保護都沒有（大多數即時排程演算法都有） |

---

## 2. 傳統的效能指標為什麼不夠

[09:01]–[12:33]（課本 §8，p.131–132）

要為一個應用（例如小型 web 服務）挑電腦，怎麼比？

| 指標 | 問題 |
|---|---|
| **MIPS、FLOPS**、peak MIPS / FLOPS | 給傳統 CPU 用，不是即時系統；peak 只算最快的指令、忽略其他指令 → 看不出效能的分布 |
| **Toy benchmark**（quicksort、merge sort） | 真正的應用一跑，CPU 可能表現不如預期 |
| **Synthetic benchmark** | 分析大量實際程式的指令分布，寫一個符合分布的合成程式（不一定有意義）。但它假設效能變化只來自指令分布；其實記憶體行為（cache hit / miss）影響更大 |
| Whetstone、Dhrystone、LINPACK（科學計算）、SPEC（CPU）、PARSEC / SPLASH（多核）、Rodinia（GPU） | 都不是為即時系統設計的 |

**SPEC**（Standard Performance Evaluation Corporation）：非營利組織，做 CPU 密集的 benchmark 評估處理器（含記憶體系統）效能。歷代 SPEC 89、92、95、2000、2006、2017；內容如股市程式、西洋棋、方程式求解、影像處理。其他：SPECviewperf（3D 繪圖：CAD/CAM、視覺化、內容創作）、SPECjvm、SPECjbb（伺服器端 Java）、SPECweb（web 伺服器動態內容）、SPEC Cloud。

【課本補充】（p.131–132）

- 廠商挑短週期指令跑 MIPS，數字灌水；有人說 MIPS 應該叫「**M**isleading **I**nformation about **P**rocessor **S**peed」。
- 合成 benchmark 例：算術 20%、I/O 10%、暫存器傳輸 10%……照這分布寫程式。SPEC 在 80 年代末成立。

---

## 3. Rhealstone

[12:33]–[22:01]（課本 §8.1，p.132–133）

### 3.1 各項指標

老師列了七項：

| 指標 | 意思 |
|---|---|
| **Task switching time（t_ts）** | 兩個**同等優先權**的獨立任務之間切換的平均時間（例如 round robin 時 T1 用完時間片、阻塞或完成，到 T2 開始） |
| **Task preemption time（t_tp）** | 從低優先權任務把控制權交給到達的高優先權任務的平均時間：要**辨認**使高優先權任務就緒的事件、存取優先權資料結構（第 6、7 支影片的佇列）、做切換 |
| **Interrupt latency time（t_il）** | 中斷發生到 ISR 第一條指令開始 |
| **Semaphore shuffling time（t_ss）** | 存取 semaphore 的時間 |
| **Deadlock breaking time** | 偵測到死結後，打破它要多久 |
| **Datagram throughput（t_dt）** | 網路參數：一個任務每秒能傳多少 KB 給另一個任務；衡量 kernel 共用緩衝的效率，也反映網路堆疊把外部訊息交給程序的時間 |
| **Process dispatch time** | 決定某程序要跑之後，到它真的開始跑要多久 |

另外老師也講了 **unbounded priority inversion prevention time**：高優先權任務請求 semaphore 到真正拿到的最壞延遲。沒有 priority inheritance 時，多個低（中）優先權任務會一直跑，這個時間就很長 → 它反映 mutex 的 priority inheritance 機制有多有效。圖：T1（最低）拿到資源，T2、T3 搶占它；T3 請求資源但 T1 持有；T1 最後跑、釋放；T3 拿到——T3 從請求到拿到的整段就是損失。

⚠️ 項目數不同：老師列七項（含 deadlock breaking time 和 process dispatch time）；課本列**六項**：task switching、task preemption、interrupt latency、semaphore shuffling、**unbounded priority inversion time**、datagram throughput（課本沒有 deadlock breaking 和 process dispatch）。

【課本補充】（p.132–133）各項的定義圖：

- **Task preemption time** = task switching time ＋ 辨認使高優先權任務就緒的事件的時間 ＋ 分派時間 → 通常比 task switching time 大。
- **Interrupt latency**：硬體辨認中斷的延遲、完成目前指令、存目前任務的 context、開始 ISR。
- **Semaphore shuffling time**：T2 持有 semaphore、T1 請求而阻塞；**T2 釋放 semaphore 到 T1 開始跑**的間隔。
- **Unbounded priority inversion time**：t_up = t₁ + t₂，t₁ = OS 辨認出 priority inversion、讓持有資源的任務跑的時間；t₂ = T1（持有者）完成後到 T2 開始的時間。
- **Datagram throughput**：**不透過共享記憶體或指標**，兩個任務之間每秒能傳多少 KB；衡量訊息傳遞原語資料結構的效率。

### 3.2 Process dispatch time 的八個部分

[17:30]–[21:31]

程序被外部（或內部）事件啟動，例如中斷 → 從系統收到中斷請求到任務開始：

```
│← interrupt latency →│← interrupt routine →│← context switch →│
  t1   t2   t3          t4   t5   t6          t7   t8
```

| 部分 | 內容 |
|---|---|
| t1 | 硬體延遲：發現某條中斷線被設成 1 |
| t2 | 完成目前指令（之後才能清管線） |
| t3 | 載入中斷子系統的額外延遲 |
| t4 | 前處理（部分硬體、部分軟體：把暫存器存到記憶體） |
| t5 | ISR 服務中斷 |
| t6 | 後處理：必要時排 bottom half、其他記帳，然後返回 |
| t7 | 決定要 context switch、知道要跑哪個程序 |
| t8 | 找到該程序 context 存在哪、載入、填暫存器與旗標、重新啟動程序 |

**中斷延遲**更細看：CPU 收到中斷到 ISR 第一條指令——硬體延遲（通知 OS）＋ 完成目前指令（離開管線）＋ 其他硬體延遲 ＋ 準備執行 ISR（從 OS 載入中斷服務表、查中斷號、取 PC、把 ISR 程式碼載進快取或記憶體）。

### 3.3 【課本補充】Rhealstone 的公式與缺點

（p.133）

> **Rhealstone = a₁·t_ts + a₂·t_tp + a₃·t_il + a₄·t_ss + a₅·t_up + a₆·t_dt**

a₁…a₆ 是經驗常數。（老師：量完這籃參數，算出一個指標。）

- 它刻畫的是 **kernel ＋ 硬體**的組合，算是全面的 benchmark。
- 缺點：完全**不考慮應用能不能趕上 deadline**（任務排程等）；六個量測類別有點隨意，沒有充分理由說明為什麼只選這六個。
- 來源：Kar 與 Porter，"Rhealstone – a real-time benchmarking proposal"，*Dr. Dobb's Journal*，1989 年 2 月（課本參考文獻 [2]）。

---

## 4. 其他要測的 RTOS 服務、tri-dimensional measure、中斷處理開銷

[22:01]–[24:31]（課本 §8.2–8.3，p.133）

**還可以測的 RTOS 服務**（老師：前面的大多在 Rhealstone 裡，最後一項是新的）：

- 不同情境的 context switch：不可搶占（cooperative）、可搶占。
- 有 / 沒有搶占的中斷處理。
- 訊息傳遞（同一機器上的程序間）。
- Semaphore 處理。
- **記憶體配置與釋放**（新的）。

**Tri-dimensional measure（TM）**：

| 維度 | 意思 |
|---|---|
| MIPS1 | 每秒處理幾百萬指令（CPU） |
| MIPS2 | 每秒處理幾百萬個**中斷**（RTOS 的中斷吞吐量） |
| NIOPS | 每秒 I/O 運算數（I/O 會產生中斷，但還有資料要搬到記憶體，例如 DMA） |

> **TM = (MIPS1 × MIPS2 × NIOPS)^{1/3}**

老師：三者相乘（適當正規化）叫 volume，開三次方叫 measure；涵蓋 RTOS（中斷）、CPU、I/O 三個面向。

**中斷處理開銷**：

> **I_n = (t_n − t₀) / t₀ × 100%**

- t₀ = 沒有中斷時完成某應用的時間；t_n = 有特定中斷注入率時的時間。
- 例：t₀ = 1 s、t_n = 1.6 s → 60%。
- 課本用每秒 **20,000** 個中斷（I₂₀,₀₀₀）；只計算中斷的**立即**部分，不算延後的部分。

---

## 5. Deterministic benchmark：timer jitter、response、bin time

[24:31]–[35:04]

兩大類即時 OS benchmark：**deterministic**（能多準確預測 OS 服務的延遲、吞吐量）和 **latency**（某一項延遲多大）。

| Deterministic benchmark | 量什麼 | 怎麼量 | 反映 |
|---|---|---|---|
| **Timer jitter** | 週期計時器預定到期時間與實際到期時間的差 | 週期 1、10、100 ms；需要另一個準確的外部時鐘 | 中斷回應（的 jitter）：OS 是否空得出來立刻處理計時器中斷，還是在忙別的、過載了 |
| **Response** | 固定的處理負載（≥ 10 ms）反覆執行的時間 | 跑 10、20、50、100 次，畫分布；可以是加數字、複製資料、或刻意壓到系統各部分（算術、記憶體……像 Whetstone） | **整個系統**的確定性：分布很窄 = 好；分布很寬 = 對即時不好 |
| **Bin time** | 反覆呼叫 time-of-day 時鐘、量呼叫之間的間隔 | 在高優先權即時執行緒裡反覆查時間 | **系統呼叫**的回應：kernel 一有機會就做自己的事（「機會主義」），還有中斷被擋住的時間 → 看出 kernel 擋了你多久 |

- 老師：這些量測不一定互斥——對 benchmark 來說互斥不是必要條件，雖然最好有一點。
- 每次呼叫的時間 = 系統呼叫 ＋ 模式切換 ＋ kernel 的時間 ＋ 應用的時間。

**計時工具的解析度**【口頭】：碼錶約 10 ms；基本系統時鐘 15–30 ms；以執行指令計時的軟體時鐘約微秒；計時器晶片最準，約 1 μs；更細要邏輯分析儀、匯流排分析儀（不在考慮範圍）。prof、gprof 等用系統呼叫的 profiling 工具精度不高。替代方案：一個執行緒一直跑迴圈，看它跑到哪裡（只要它不被換出）。

**量 timer jitter**：現代 CPU 有 **timestamp counter**，每個 CPU 週期（或每個 jiffy）更新；POSIX 的 `clock_gettime()` 用它提供相當高精度的時間（第 18 支影片筆記：本機 14 ns 步進、走 vDSO）。

---

## 6. Latency benchmark：sync、message、signal

[35:04]–[39:32]

不像 deterministic 要跑很多次看分布，latency benchmark 量某一個面向：

| Benchmark | 量什麼 |
|---|---|
| **Sync** | 執行緒對執行緒（或程序對程序）**同步**的延遲，例如在 semaphore 上同步時的 context switch 時間 |
| **Message** | 從執行緒 / 程序 A 送訊息到 B 的延遲；以**緩衝大小**為參數；也可看出可能的吞吐量 |
| **Signal** | 程序 A 送**即時信號**給 B 的延遲 |

- 老師：中斷、系統呼叫、例外是應用 → kernel；kernel 要通知應用時送**信號**，方向相反；POSIX 即時信號把它推廣到程序對程序（第 21 支影片）。
- 同步測試可以做：單一系統呼叫的延遲；**來回信號**（signal–wait–signal–wait）的延遲；**低優先權任務被中優先權任務延遲**的情況——模擬 unbounded priority inversion：低優先權任務拿著資源、被中優先權任務換下，高優先權任務只好等。
- 用 semaphore 在執行緒或程序之間傳信號 → 評估 OS 處理 priority inversion（特別是 unbounded）的能力：有發生的話持續多久，或確認不會發生。

（第 10 支影片的實測就是這種測試：一般 Linux 上 4 個中間任務讓高優先權任務等了 3.7 s。）

---

## 7. 【課本補充】量 kernel 可搶占性：TRIANG

（課本 §8.4，p.134；課堂沒講）

- **TRIANG** 模組產生隨機三元組（三角形的三個頂點），寫進共享記憶體。
- n 個即時任務 RT1…RTn 讀頂點、處理、透過訊息傳遞把原始頂點送給 **analyzer**。TRIANG 的優先權比即時任務高。
- 計時器先叫 TRIANG、再呼叫一個系統呼叫（例如建立程序），接著觸發不同的即時任務。
- 若 kernel **不可搶占**，即時任務還沒讀到頂點，下一組值就把它蓋掉了 → analyzer 比對 TRIANG 送的和即時任務送的，發現不一致。
- 改變計時器頻率，觀察**開始漏掉頂點的頻率** → 就是最壞情況的 kernel 搶占時間。

---

## 8. 【實測】在本機跑一組迷你 benchmark

照本講的指標，在本機寫一組迷你測試（一般 SCHED_OTHER、綁在 CPU 3）：

```c
/* semaphore shuffling / task switch: ping-pong through two semaphores = 2 switches per round */
for (int i = 0; i < N; i++) { sem_post(&a); sem_wait(&b); }
/* datagram throughput: bytes per second between two tasks through the kernel (no shared memory) */
for (long sent = 0; sent < total; sent += write(sv[0], msg, sizeof msg)) ;
/* "response": same fixed workload many times -> distribution */
/* "bin time": back-to-back clock reads; big gaps = time stolen by interrupts / kernel */
```

```
semaphore hand-off / task switch (same CPU): 0.81 us per switch
datagram-style throughput (4 KB writes, UNIX socket): 4980 MB/s
response (fixed ~ms workload x300): min 3.414  median 3.427  99% 3.470  max 3.534 ms
bin time over 2 s of back-to-back clock reads: <100ns 131143051, <1us 548, <10us 204, <100us 1, <1ms 0, >=1ms 0; max gap 12.3 us
```

看到什麼：

| 指標 | 結果 | 解讀 |
|---|---|---|
| Semaphore shuffling / task switch | 0.81 μs | 釋放 semaphore 到等待者開始跑；同一顆 CPU 上的兩次切換取平均 |
| Datagram throughput | 約 5 GB/s | 透過 kernel 緩衝（不用共享記憶體），課本的定義 |
| Response | 3.414–3.534 ms，最大比最小多 3.5% | 閒置系統上，固定工作量很穩定 |
| Bin time | 2 秒內讀了 1.3 億次時鐘，99.9994% 的間隔 < 100 ns；有 752 次 > 100 ns，最大 12.3 μs | 這些「洞」就是 kernel 和中斷（計時器 tick 等）偷走的時間——bin time 想抓的正是這個 |

- 這些數字是**閒置系統**上的；benchmark 的意義在於比較不同 OS / 設定，或在負載下看最壞情況（第 22、25 支影片筆記：一般任務在 CPU 被搶時最壞延遲到 24 ms）。
- Timer jitter 的量法就是第 22 支影片筆記的 cyclictest 式程式；unbounded priority inversion 的量法就是第 10 支影片筆記的測試。

---

## 9. 結語

[39:32]–[40:30]

- OS 達成任務時間限制的**主要手段是任務排程**——這門課一路的主力。
- 傳統 OS（Unix、Windows）對即時應用並不令人滿意。
- 嵌入式 RTOS 一般輕量、可重新設定。
- 老師的意見：**應該**做 benchmark 來比較 RTOS 的實際效能。
- RTOS 和 benchmark 到此結束；剩下三講是即時通訊和即時資料庫。

⚠️ 逐字稿 [39:32] 是「traditional operating systems such as unix and windows **are very satisfactory** for real-time applications」。照整門課的內容和課本 Summary（「Unix、Windows 不適合硬即時應用」），應是「**not** very satisfactory」，字幕漏字。

---

## 10. 一頁速查表

| 主題 | 重點 |
|---|---|
| 感測器節點 | 4 MHz 8 位元、0.5–幾 KB RAM、簡單無線電；ad hoc multi-hop 網路 |
| TinyOS | 不是 RTOS：無 kernel、單程序、無虛擬記憶體、不動態配置、事件驅動、FIFO、不可搶占、省電 |
| 傳統指標 | MIPS / FLOPS（灌水）、toy、synthetic（忽略快取）、SPEC 等都不適合即時 |
| Rhealstone | 課本六項：t_ts、t_tp、t_il、t_ss、t_up、t_dt，加權平均；老師另加 deadlock breaking、process dispatch；缺點：不看 deadline、項目隨意 |
| Process dispatch | 中斷延遲（t1–t3）＋ 中斷常式（t4–t6）＋ context switch（t7–t8） |
| TM | (MIPS1 × MIPS2 × NIOPS)^{1/3} |
| 中斷開銷 | (t_n − t₀)/t₀；課本用每秒 20,000 個中斷 |
| Deterministic | timer jitter（中斷回應）、response（整體）、bin time（系統呼叫 / kernel 偷時間） |
| Latency | sync（semaphore）、message（緩衝大小）、signal（即時信號）；也可模擬 priority inversion |
| TRIANG（課本） | 調計時器頻率到開始漏頂點 → 最壞 kernel 搶占時間 |
| 實測 | semaphore 交接 0.81 μs；5 GB/s；response 變動 3.5%；bin time 最大洞 12.3 μs |

---

## 11. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **Datagram throughput、網路堆疊延遲** | **第 27–28 支**：Real Time Communication、即時 LAN |
| **感測器網路的多跳通訊** | **第 27–30 支** |
| **即時資料** | **第 31 支**：Real Time Data Stores |

---

## 附：本講一句話總結

> **要知道 RTOS 是否真的符合即時要求就得 benchmark，而 MIPS、FLOPS、toy 或合成 benchmark、SPEC 這些傳統指標都不合用（只看指令、忽略快取，也不看延遲）；典型的即時硬體是幾 MHz、幾 KB RAM 的感測器節點，上面常跑的 TinyOS 沒有 kernel、事件驅動、只有 FIFO，其實不是 RTOS。Rhealstone 把任務切換、搶占、中斷延遲、semaphore shuffling、unbounded priority inversion、datagram throughput（老師另加 deadlock breaking 與 process dispatch）加權成一個數字，但不看應用能否趕上 deadline、項目也有點隨意；process dispatch 可細分成中斷延遲、中斷常式與 context switch 共八段；另有 (MIPS1·MIPS2·NIOPS)^{1/3} 的 tri-dimensional measure 和以 (t_n − t₀)/t₀ 計算的中斷處理開銷。更實用的是兩類測試：deterministic benchmark 看分布——timer jitter 反映中斷回應、response 反映整體確定性、bin time 抓出 kernel 偷走的時間；latency benchmark 則量 semaphore 同步、訊息與即時信號延遲，也能模擬 priority inversion。本機實測 semaphore 交接 0.81 μs、kernel 傳輸約 5 GB/s、固定工作量變動 3.5%，而 2 秒內 1.3 億次讀時鐘中最大的空洞是 12.3 μs，正是 kernel 和中斷偷走的時間。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 27 錄影逐字稿（YouTube 自動字幕，40:39）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch5 §8 整理。實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
