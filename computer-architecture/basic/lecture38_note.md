# Basic Computer Architecture 第三十八支影片完整筆記：多處理器概論：Moore's law、shared memory 與 message passing、Amdahl's law、Flynn 分類（Chapter 11 Multiprocessor Systems, Part 1/5 → 課本 Ch12）

> **課程**：Basic Computer Architecture — Chapt 11: Overview of Multiprocessor Systems, Part 1/5（播放清單第 38 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（1:01:44，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 **Ch12** 開頭、§12.1–12.3（p.565–579）
> **章號**：影片的 Chapter 11 是課本 v3.09 的 **Chapter 12**；影片說的「chapter 9」（頻率的限制）是課本 Ch10。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4 + OpenMP、Open MPI）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 動機：頻率和單核效能都停了](#1-動機頻率和單核效能都停了)
- [2. Multiprocessor 與 multicore](#2-multiprocessor-與-multicore)
- [3. Symmetric 與 asymmetric multiprocessing](#3-symmetric-與-asymmetric-multiprocessing)
- [4. Moore's law](#4-moores-law)
- [5. Feature size](#5-feature-size)
- [6. Loosely coupled 與 strongly coupled](#6-loosely-coupled-與-strongly-coupled)
- [7. Shared memory 與 message passing](#7-shared-memory-與-message-passing)
- [8. Process 與 thread](#8-process-與-thread)
- [9. Shared memory 程式：OpenMP 平行加總](#9-shared-memory-程式openmp-平行加總)
- [10. Thread 的 spawn 與 join](#10-thread-的-spawn-與-join)
- [11. Message passing 程式：MPI 風格的平行加總](#11-message-passing-程式mpi-風格的平行加總)
- [12. Amdahl's law](#12-amdahls-law)
- [13. Flynn 分類](#13-flynn-分類)
- [14. 【實測】](#14-實測)
- [15. 一頁速查表](#15-一頁速查表)
- [16. 與後續講次的連結](#16-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

本章（影片 Ch11）分六部分：概論、Amdahl's law、多處理器技術、multithreading、vector processor、interconnect（把很多處理單元接起來的線路）。【口頭】[01:14]「多處理器其實是進階計算機架構的內容」，這裡只講一點點。

```
頻率卡在 2–3 GHz（功耗 ∝ f³），單核效能也停了
→ Moore's law 給的多餘電晶體拿去放更多核心：multicore
SMP（大家平等）vs AMP（master／slave）
loosely coupled（不相干的程式）→ message passing
strongly coupled（共享資料）     → shared memory（thread）
Amdahl's law：S = 1 / (f_seq + (1 − f_seq)/P) → 最多 1/f_seq
Flynn：SISD、SIMD、MISD、MIMD
```

---

## 1. 動機：頻率和單核效能都停了

[01:28]–[06:24]　課本 Ch12 開頭（p.565–566，Fig 12.1–12.2）

**頻率**（【口頭】[01:42]，1999–2012 的圖）：2001 年大多 1–2 GHz；2003–2004 年起在 1.5–4 GHz；過去十幾年的處理器幾乎都落在 **1.5–3.5 GHz** 這一帶。

原因（影片的 chapter 9 = 課本 Ch10，[第 32 支](lecture32_note.md#19-pipeline-的好處到底在哪裡)）：**功耗 ∝ 頻率的三次方**，頻率加倍功耗變 8 倍。所以伺服器處理器的頻率大概停在 2–3 GHz。

**效能**（【口頭】[03:18]，用 SPEC 2006 分數）：
- **SPEC**（Standard Performance Evaluation Corporation）出了一組 benchmark：**SPECint**（整數）和 **SPECfp**（浮點）。
- 分數的算法：用一台大約 20 年前的舊 CPU 當基準，每個 benchmark 算「舊 CPU 時間 ÷ 新 CPU 時間」，再取所有比值的**幾何平均**。
- 效能一直漲到大約 2008–2009 年，之後就**飽和**了。
- 【口頭】[05:07]「細心的讀者會發現」：某兩個時間點之間頻率一樣，效能卻漲了 **7 倍**，這是計算機架構的功勞：更好的 cache 管理、更大的 cache、更好的 forwarding、一次發好幾條指令……但之後架構也到極限了。

【口頭】[06:24]「計算機架構死了嗎？沒有。」電晶體一直變小，每顆晶片放得下更多電晶體。以前拿來讓一個處理器更複雜；現在拿來**多放幾個處理器**。

【課本補充】p.565–566：
- 就算是很複雜的 superscalar 處理器和高度最佳化的記憶體系統，IPC 通常也很難再提高超過 50%。
- 課本的圖是 Intel、AMD、Sun、Qualcomm、Fujitsu 2001–2010 年的處理器，頻率大多在 **1–2.5 GHz**，預期近期也會限制在 3 GHz。
- 2005 年之後設計者改用比較簡單的處理器，一顆晶片放好幾個。

⚠️ 頻率範圍：影片說 1.5–3.5 GHz，課本說 1–2.5 GHz、上限約 3 GHz。本機的 P-core 最高到 **4.9 GHz**（見 [§14.1](#141-本機是一個-multicore而且核心不一樣)），只在短時間、少數核心時才跑到。

---

## 2. Multiprocessor 與 multicore

[06:24]–[08:30]　課本 Definition 116（p.566–567）

- 大約 2006–2007 年以前，一顆晶片只有一個處理器（一條 pipeline）；現在一顆晶片有 2、4、8、16 個。
- 晶片上的每個處理器叫一個 **core**（核心）。【口頭】「processor 和 core 是同一件事」。
- 這種處理器叫 **multicore processor**（多核心處理器）。

| 詞 | 意思 |
|---|---|
| **Multiprocessing** | 好幾個處理器**平行**工作（通用的說法，可以在同一顆晶片或不同晶片） |
| **Multicore** | multiprocessor 的一種：所有處理器都在**同一顆晶片**上 |

【口頭】[08:16] multicore 和 **many-core**（每顆晶片很多處理單元）現在很熱門。

【課本補充】p.567：每顆晶片的核心數大約**每兩年加倍**；大部分專家認為計算的未來在多處理器。

---

## 3. Symmetric 與 asymmetric multiprocessing

[08:30]–[10:30]　課本 §12.1、Definition 117（p.567–568）

| | **Symmetric（SMP）** | **Asymmetric（AMP）** |
|---|---|---|
| 處理器的地位 | **全部一樣**，沒有誰比較特權 | 不一樣 |
| 作業系統、I/O（滑鼠、鍵盤） | 每個都能平等存取 | 只有 **master** 有獨占控制權 |
| 分工 | — | master 分派工作給 **slave**，slave 做完交回 |
| 現況 | **遠比較常見** | 不常見；【口頭】「但明天可能會變」 |

【課本補充】p.567–568 歷史：
- 60、70 年代大電腦主要給銀行、金融機構用，一個處理器不夠用，就放好幾個。
- 最早的之一是 **Burroughs 5000**：A 是主處理器，B 是輔助處理器，負載高時 A 分工作給 B。IBM 370、PDP 11/74、VAX-11/782、Univac 1108-II 也都有。第二顆 CPU 在另一顆晶片上，用電線或纜線接。
- 80 年代末、90 年代初開始把多個處理器放在同一塊**主機板**上；90 年代末可以放 4 或 8 個。
- **IBM Power 4**（2001）是第一個雙核心處理器；Intel、AMD 2005 年跟進；2012 年有 8、10 核。

---

## 4. Moore's law

[10:30]–[13:39]　課本 §12.1.1、Historical Note 3（p.568）

【口頭】「Moore's law 是一個『定律』，但它沒有證明，只是經驗觀察。」

- 今天手機裡的處理器比 60 年代最先進的 **IBM 360** 快 **160 萬倍**。IBM 360 有一整個房間大；手機處理器大概 0.5 × 0.5 cm 到 1 × 1 cm。
- 原因：電晶體在 60、70 年代有**幾毫米**（10⁻³ m）長，現在只有**幾奈米**（10⁻⁹ m）→ 小了大約一百萬倍，也快了很多。
- 電晶體越小 → 越快、越省電、一顆晶片放得越多。

**Moore's law**：Intel 共同創辦人 **Gordon Moore** 說，每顆晶片上的電晶體數**大約每兩年加倍**。
- 他一開始（1965 年）說**每年**加倍。
- 後來慢下來變成 **18 個月**，現在大約 **2 年**。

【課本補充】p.568：1971 年一顆晶片約 2000–3000 顆電晶體，現在有幾十億顆。電晶體通道長度 60 年代是幾毫米，現在約 20–30 nm。

---

## 5. Feature size

[13:39]–[15:41]　課本 Table 12.1（p.569）

**Feature size**（特徵尺寸）：晶片上能精確做出來的**最小結構**的大小（簡化的定義）。

| 年份 | 2001 | 2003 | 2005 | 2007 | 2009 | 2011 | 2014 | 2017（影片的預測） |
|---|---|---|---|---|---|---|---|---|
| feature size | 130 nm | 90 | 65 | 45 | 32 | 22 | 14 | 10 |

（2014、2017 是影片口述的；課本表只到 2011。）

- 130 ÷ 90 ≈ **√2**。兩個方向各縮小 √2 → 面積變 1/√2 × 1/√2 = **一半** → 同樣面積放得下**兩倍**電晶體。這就是每兩年加倍的來源。
- 【口頭】[15:05]「2017，就是錄這支影片的這一年，預測會到 10 nm。」
- 【口頭】一顆原子大約 **0.1 nm**，10 nm 大概只有 **100 顆原子**寬，「非常非常小」。

【實測】用上表算相鄰兩代的比值：1.44、1.38、1.44、1.41、1.45（22 → 14 是 1.57），都在 √2 = 1.414 附近。

【課本補充】§12.1.2（p.569）：
- 以前多出來的電晶體用來加複雜度：更大的 cache、更寬的 issue、更多功能單元；pipeline 級數和頻率一直增加到大約 **2002 年**。
- 之後功耗超過 **100 W**、晶片溫度超過 **100 °C**，頻率和複雜度就停了。改成每兩年核心數加倍，每個核心的電晶體數不變。
- 預期 2020 年前每顆晶片會到 32 或 64 核。作者寫書時用的是 32 核伺服器（4 顆晶片 × 8 核）。
- 另一個方向：GPU 有 64–256 個很小的核心；還有 **multithreaded processor**（一個處理器支援兩個 PC、同時跑兩個程式），第 41 支會講。

---

## 6. Loosely coupled 與 strongly coupled

[15:41]–[17:23]　課本 §12.2.1、Definition 118–119（p.570–571）

| | **Loosely coupled**（鬆耦合） | **Strongly coupled**（緊耦合） |
|---|---|---|
| 跑什麼 | 好幾個**不相干**的程式 | 一組**互相合作**的程式 |
| 程式之間交換資訊 | 幾乎沒有 | 共享資料、程式碼、檔案、網路連線，幾乎什麼都共享 |
| 目的 | — | 一起完成一個更大的目標 |

【口頭】[17:08] 大部分多處理器系統偏向支援 strongly coupled：它比較複雜，但好處也比較多。

【課本補充】p.570–571：
- loosely coupled 的例子：文書編輯器在處理器 1、瀏覽器在處理器 2；或在 128 個處理器上用 128 組參數跑 128 份天氣模擬 → 128 倍加速。
- 這和一群用區域網路連起來的電腦（**cluster**）沒什麼不同，只是延遲比較低。這種一群電腦叫 **multicomputer**（Definition 118）：程式之間不共享記憶體空間。
- 本書主要看 strongly coupled，「它才發揮多處理器真正的力量」。

---

## 7. Shared memory 與 message passing

[17:23]–[20:14]　課本 §12.2.2（p.571–572）

| | **Shared memory**（共享記憶體） | **Message passing**（訊息傳遞） |
|---|---|---|
| 適合 | strongly coupled | loosely coupled |
| 怎麼溝通 | 大家**共用虛擬位址空間**，讀寫共用的記憶體 | **送、收訊息** |
| 共享什麼 | 記憶體，也因此共享 I/O（例如硬碟）的同一個視角 | 什麼都不共享，連記憶體位址都沒有 |

【口頭】[18:13] strongly coupled 系統「通常會」實作 shared memory，雖然不是 100% 必要。共享位址空間的這些程式叫 **thread**（更精確地說是 software thread）。

【課本補充】p.571–572 工廠的比喻：
- 工人在同一個地方，看得到、聽得到彼此，有問題大叫一聲大家就來幫忙 → shared memory。A 把 x 改成 5，B **立刻**看得到。
- 工人不在一起，只能寫信、打電話、寄 email，要知道對方是誰才能送訊息；但可以支援更多工人、分布在世界各地，**更有彈性、更好擴充** → message passing。
- 也可以在 strongly coupled 的機器上做 message passing，或在 loosely coupled 的機器上模擬 shared memory（叫 **distributed shared memory**），但不是常態。

---

## 8. Process 與 thread

[20:14]–[21:36]　課本 Definition 120–121（p.573–574）

| | 位址空間 |
|---|---|
| **Process** | 程式的執行實例；**和別的 process 不同** |
| **Thread** | 也是程式的執行實例；**和同一組的 thread 共用** |

【口頭】[21:19]「這是要記住的重要區別。」

thread 各自有：自己的 **PC**、自己的**暫存器**、自己的 **stack**（區域變數）。其他都可以共享：常數、唯讀變數、heap 上的資料、global 變數。一個 process 可以有好幾個 thread。thread 之間靠**寫記憶體**或**同步操作**溝通。

---

## 9. Shared memory 程式：OpenMP 平行加總

[21:36]–[31:56]　課本 Example 149（p.572–573）

問題：陣列 `numbers` 有 SIZE 個數（例如一百萬個），用 N 個 thread 加總。

想法：把陣列切成 N 等份，每份給一個核心算 **partial sum**（部分和），最後把 N 個 partial sum 加起來。

課本的程式（C++ ＋ **OpenMP**）：

```c
int partialSums[N];
int numbers[SIZE];
int result = 0;
/* initialize arrays */
...
#pragma omp parallel {
    int myId = omp_get_thread_num();          /* 0 … N−1 */
    int startIdx = myId * SIZE/N;
    int endIdx = startIdx + SIZE/N;
    for (int jdx = startIdx; jdx < endIdx; jdx++)
        partialSums[myId] += numbers[jdx];
}
/* sequential section */
for (int idx = 0; idx < N; idx++)
    result += partialSums[idx];
```

- `#pragma omp parallel`：大括號裡的程式碼會**複製 N 份**，每個處理單元一份。全部做完才離開，回到循序的部分。
- 大家看的是同一段程式碼，怎麼知道自己是誰？`omp_get_thread_num()` 給每個 thread 一個 0 到 N − 1 的編號 `myId`。
- 每份大小 SIZE/N，起點 `myId × SIZE/N`，終點（不含）`起點 + SIZE/N`。
- 【口頭】[28:05] 老師在投影片上把變數改名成 jdx：「jdx 比較好」。
- 不用管加法溢位，「那很容易處理」。

**Scatter 與 gather**（【口頭】[30:42]）：
- 把加法**分散**給各個 thread → **scatter**。
- 把 partial sum **收集**起來加成一個值 → **gather**，也叫 **reduce**（化簡）。
- 收集那一段是**循序**的；但如果有一百萬個數，平行的部分會帶來很大的加速。

【課本補充】p.573：除了 `#pragma omp parallel` 這一行，看起來就像一般的循序程式。thread 數是事先設好的系統參數，通常等於處理器數。最後的循序部分可以在任何處理器上跑，由作業系統或平行程式框架在執行時決定。

⚠️ 課本這個程式**默默假設 SIZE 能被 N 整除**（message passing 的 Example 150 才寫出這個假設）。整除不了時，最後 SIZE mod N 個數沒有人加，答案會錯。【實測】見 [§14.2](#142-課本的-openmp-程式)。

---

## 10. Thread 的 spawn 與 join

[31:56]–[34:46]　課本 Fig 12.3（p.573）

```
parent thread ──初始化──→ spawn（產生）child threads
                            ├── thread 0：加第 0 份
                            ├── thread 1：加第 1 份
                            ├── ...
                            └── thread N−1
                         ←── thread join（等全部做完）
parent thread ──把 partial sum 加起來（循序）──→ 結束
```

- **spawn**：啟動一組平行的 thread。
- **join**：等所有被 spawn 的 thread 都做完，parent thread 才接手。join 是一種**同步**（synchronization）操作。

【課本補充】p.573：child thread 結束時 stack 上的區域變數全部消失，所以要用**大家都存取得到的**變數（例如 global 的 partialSums）傳資料。同步操作有很多種，細節見 Culler 等人的書。

---

## 11. Message passing 程式：MPI 風格的平行加總

[34:46]–[46:09]　課本 Example 150、Table 12.2（p.574–576）

message passing 裡每個 process 只能做兩件事：

| 函式 | 意思 |
|---|---|
| `send(pid, val)` | 把整數 val 送給 process pid（**unicast** 一對一；也可以 **multicast** 一對多） |
| `receive(pid)` | 收 process pid 送來的整數；**收到之前會一直等**；pid = ANYSOURCE 表示誰送的都收 |

假設：每個 process 都有一份 `numbers`；SIZE 能被 N 整除（例如 N = 100、SIZE = 10,000）。語法參考 **MPI**（Message Passing Interface），但簡化很多。

```c
SpawnAllParallelProcesses();
int myId = getMyProcessId();              /* 0 … N−1 */
int startIdx = myId * SIZE/N;
int endIdx = startIdx + SIZE/N;
int partialSum = 0;                       /* 每個 process 自己的變數，不共享 */
for (int jdx = startIdx; jdx < endIdx; jdx++)
    partialSum += numbers[jdx];
if (myId != 0) {
    send(0, partialSum);                  /* 非 root：送給 root */
} else {
    int sum = partialSum;                 /* root 自己也算了一份 */
    for (int pid = 1; pid < N; pid++)
        sum += receive(ANYSOURCE);        /* 收 N−1 個訊息 */
    shutDownAllProcesses();
    return sum;
}
```

- id = 0 的當 **root**，其他的是 non-root。
- root 要等 **N − 1** 個訊息。

【口頭】[45:09] 兩種程式的**結構一樣**：一開始一個 thread 循序做一點事 → 啟動一堆平行的 thread／process 做大部分工作 → 一個 reduce 把結果合起來（循序）。差別：OpenMP 用**共享變數**溝通，message passing 用**訊息**。

---

## 12. Amdahl's law

[46:09]–[60:46]　課本 §12.2.3、Eq 12.1–12.2、Fig 12.4（p.576–577）

理想上 P 個處理器 → 加速 P 倍（T 變 T/P）。但程式總有一部分只能**循序**執行（初始化資料結構、最後把 partial result 合起來）。

把原本的時間 T_old 分成兩部分：
- 循序部分：T_old × **f_seq**（不能平行化的時間比例）
- 可平行部分：T_old × (1 − f_seq)，在 P 個處理器上平均分 → 除以 P

```
T_new = T_old × (f_seq + (1 − f_seq)/P)

            T_old              1
S  =  ─────────  =  ─────────────────────     ← Amdahl's law
            T_new      f_seq + (1 − f_seq)/P
```

檢查（【口頭】[50:48]）：
- P = 1：S = 1，沒有好處。「很明顯」。
- P = 2：分母 < 1，S > 1。
- **P → ∞：S → 1/f_seq**。時間全被循序部分決定。

【口頭】[55:04] 例子：f_seq = 0.25，可平行部分 0.75；分給 10 個處理器 → 0.25 + 0.075。

【實測】照公式算（f_seq = 10%、5%、2%）：

| P | 10 | 35 | 50 | 100 | 200 | ∞ |
|---|---|---|---|---|---|---|
| f_seq = 10% | 5.3 | 8.0 | 8.5 | 9.2 | 9.6 | **10** |
| f_seq = 5% | 6.9 | 13.0 | 14.5 | 16.8 | 18.3 | **20** |
| f_seq = 2% | 8.5 | 20.8 | 25.3 | 33.6 | 40.2 | **50** |

- 【口頭】10% 的很快就飽和在 10；5% 到 60–80 個處理器還在長，理論上限 20；2% 到 200 個處理器有約 40 倍，上限 50。和上表一致。
- 「最重要的是：處理器很多時，加速被**循序部分**決定。」所以要**減少循序部分、盡量平行化**。這取決於工作的性質和寫演算法的人。

⚠️ 課本 p.577 說 f_seq = 5% 時「35 個處理器和 200 個處理器沒有明顯差別」；但照公式算是 **13.0 和 18.3**，還差 40%。影片說 5% 的曲線要到 60–80 個處理器才平。我照公式的數字為準。

⚠️ [59:26] 老師說到一半「好、好、好，我重說一次」：處理器很多時是**平行部分的時間**趨近 0，不是平行部分本身變小。

【課本補充】p.577：
- Amdahl's law 是加速的**理論估計**（大多是**上限**）。
- 縮短循序部分要從演算法和系統兩方面下手：例如 Example 149、150 的**初始化也可以平行做**；另外要有**快**的處理器來跑循序部分，這就是前面幾章（處理器、pipeline、記憶體）在做的事。

---

## 13. Flynn 分類

[60:46]–[61:44]　課本 §12.3（p.578–579）

- **Instruction stream**（指令流）：處理器執行的那串指令。
- **Data stream**（資料流）：指令處理的資料。

| | 意思 |
|---|---|
| **SISD** | single instruction single data |
| **SIMD** | single instruction multiple data |
| **MISD** | multiple instruction single data |
| **MIMD** | multiple instruction multiple data |

⚠️ 影片在定義完四個名字時就結束了，例子在下一講。

【課本補充】p.578–579（Michael J. Flynn，1966）：
- **SISD**：一般的單處理器、一條 pipeline（第 27–33 支）。
- **SIMD**：一條指令處理好幾組資料，例如一條指令加 4 組數。x86 的 **SSE**、GPU、vector processor 都是。
- **MISD**：很少見，用在**可靠度**要求極高的地方。例：大型客機用 MIPS、ARM、x86 三個處理器各跑**不同版本**的自動駕駛程式，同一份資料，最後**投票**。一個因為 bug 想左轉，另兩個說右轉 → 右轉。
- **MIMD**：現在最普遍，multicore 和大型伺服器都是。Example 149、150 都是 MIMD。分兩種：
  - **SPMD**（single program multiple data）：很多份**同一個程式**，各自有 PC，依 thread id 做不同的事。大部分平行程式都是這種。
  - **MPMD**（multiple program multiple data）：不同處理器跑**不同的程式**，常見於異質處理單元：一個 master 程式分派工作給 slave 程式。

---

## 14. 【實測】

程式在 `experiments/lecture38/`。

### 14.1 本機是一個 multicore，而且核心不一樣

```
$ lscpu
CPU(s):                             20
Thread(s) per core:                 2
Core(s) per socket:                 14
Socket(s):                          1
$ cat /sys/devices/system/cpu/cpu*/cpufreq/cpuinfo_max_freq
cpu0–cpu11 : 4.7–4.9 GHz（6 個 P-core，每個 2 個 thread）
cpu12–cpu19: 3.6 GHz    （8 個 E-core，每個 1 個 thread）
```

- 一顆晶片 **14 個核心、20 個 hardware thread**：就是影片說的 multicore。
- 作業系統把 20 個都當**一樣**的 CPU 來排程（SMP：都能跑作業系統、都能做 I/O）。但硬體其實有兩種核心：6 個快的 P-core（performance）、8 個比較慢、比較省電的 E-core（efficient）。這不是影片說的 master／slave 那種 asymmetric，而是「地位對稱、能力不對稱」。

### 14.2 課本的 OpenMP 程式

`sum_omp.c` 把課本 Example 149 原樣寫出來（SIZE = 2²⁶ = 6400 萬個 int、256 MB；`volatile` 讓每次加法都真的寫回 partialSums，跟課本寫法的行為一樣）。thread 綁在不同核心上（`OMP_PLACES=cores OMP_PROC_BIND=close`，前 6 個是 P-core）。`experiments/lecture38/sum_omp.c:22-40`：

```c
	#pragma omp parallel
	{
		int myId = omp_get_thread_num();
		int startIdx = myId * (SIZE / N), endIdx = startIdx + SIZE / N;
		partialSums[myId * gap] = 0;
		if (mode[0] == 'b') {
			for (int jdx = startIdx; jdx < endIdx; jdx++) partialSums[myId * gap] += numbers[jdx];
		} else if (mode[0] == 'l') {
			long s = 0;
			for (int jdx = startIdx; jdx < endIdx; jdx++) s += numbers[jdx];
			partialSums[myId * gap] = s;
		} else {
			long s = 0;
			for (int jdx = startIdx; jdx < endIdx; jdx++) s += mix(numbers[jdx]) & 7;
			partialSums[myId * gap] = s;
		}
	}
	long result = 0;                      /* sequential section：把 partial sums 加起來 */
	for (int idx = 0; idx < N; idx++) result += partialSums[idx * gap];
```

**SIZE 不能被 N 整除時答案錯了**：

```
book gap=1 N= 4:     27.6 ms  result=234881024   ← 正確
book gap=1 N= 6:     26.3 ms  result=234881002   ← 少了 22
```

2²⁶ ÷ 6 有餘數 4，最後 4 個數（值 4+5+6+7 = 22）沒人加。這就是 §9 的 ⚠️。

**三種寫法的加速**（ms；N 取 2 的次方，答案都正確）：

| 寫法 | N=1 | N=2 | N=4 | N=8 |
|---|---|---|---|---|
| 課本：每次加到 partialSums[myId] | 100.4 | 54.6 | 30.9 | 29.3 |
| 先加到區域變數，最後寫一次 | 18.8 | 10.2 | 7.8 | 9.1 |
| 每個元素多做 16 次整數乘加（運算量大） | 238.6 | 124.5 | 63.0 | 64.1 |

- 純加總：4 個 thread 之後就不再變快（7.8 ms，等於每秒讀約 34 GB）：瓶頸是**記憶體頻寬**，不是處理器數。這是 Amdahl's law 之外另一種限制。
- 寫法本身影響很大：每次都寫回記憶體的課本寫法，單 thread 就慢 5 倍。
- 運算量大的版本：4 個 thread 快 3.8 倍，很接近理想的 4。

### 14.3 用實測反推 f_seq，以及不一樣快的核心

運算量大的版本，thread 從 1 加到 20：

| N | 1 | 2 | 4 | 6 | 8 | 10 | 12 | 14 | 20 |
|---|---|---|---|---|---|---|---|---|---|
| 時間（ms） | 239.8 | 121.2 | 61.9 | **44.3** | 64.2 | 51.4 | 48.8 | 39.6 | 36.2 |

- 6 個 thread（全在 P-core）：加速 **5.41 倍**。代入 Amdahl's law 反推：1/5.41 = f + (1 − f)/6 → **f_seq ≈ 2.2%**（thread 啟動、最後的加總等）。
- **8 個 thread 反而變慢**（64.2 ms）：第 7、8 個 thread 跑在 E-core 上。課本的程式把陣列**平均**切，每份一樣大；E-core 做這份要花的時間大約是 P-core 的 2 倍，大家都要等最慢的那個 → 整體被拖慢。Amdahl's law 假設「可平行部分平均分給 P 個**一樣**的處理器」，在這台機器上不成立。
- 20 個 thread 時 36.2 ms，只比 6 個好一點。

### 14.4 False sharing 的影子

課本的寫法讓每個 thread 一直寫 `partialSums[myId]`，相鄰 thread 的格子在**同一條 64-byte cache line** 上。把格子隔開 8 個 long（64 bytes，`gap = 8`）再比：

| N | 1 | 2 | 4 | 6 |
|---|---|---|---|---|
| gap = 1（課本，同一條 line） | 100.1 | 54.6 | 27.6 | **26.3** |
| gap = 8（不同 line） | 100.1 | 49.8 | 25.3 | **17.7** |

1 個 thread 時一模一樣；thread 越多，同一條 line 的版本越慢（6 個 thread 時慢約 50%）。不同核心一直寫同一條 cache line，那條 line 要在核心的 cache 之間搬來搬去，這叫 **false sharing**（偽共享：資料其實沒共用，只是剛好在同一條 line）。為什麼要搬，要等第 39–40 支的 **cache coherence** 才講得清楚。

### 14.5 真正的 MPI

`sum_mpi.c` 把 Example 150 改成真正的 MPI（Open MPI）：`MPI_Comm_rank` = getMyProcessId、`MPI_Send` = send、`MPI_Recv(..., MPI_ANY_SOURCE, ...)` = receive(ANYSOURCE)。每個 process 有**自己的** numbers（不共享記憶體）。

```
$ mpicc -O2 -o sum_mpi sum_mpi.c && mpirun -np 4 ./sum_mpi
N=1 processes: sum=234881024  19.4 ms
N=2 processes: sum=234881024  10.1 ms
N=4 processes: sum=234881024  7.8 ms
```

答案正確；速度和 OpenMP 的「區域變數」寫法幾乎一樣（18.8、10.2、7.8 ms），也一樣被記憶體頻寬限制在 4 個左右。

---

## 15. 一頁速查表

| 主題 | 重點 |
|---|---|
| 動機 | 頻率卡在 2–3 GHz（功耗 ∝ f³），單核效能約 2008 年後飽和 |
| SPEC 分數 | 和舊 CPU 的時間比值，取幾何平均 |
| Multiprocessing | 多個處理器平行；multicore = 同一顆晶片，每個叫 core |
| SMP／AMP | 全部平等（常見）／master 分工給 slave |
| Moore's law | 電晶體數每 1 年 → 18 個月 → 2 年加倍；經驗定律 |
| Feature size | 每代 ÷ √2 → 面積減半；130→90→65→45→32→22→14→10 nm |
| Loosely coupled | 不相干的程式；message passing；multicomputer／cluster |
| Strongly coupled | 共享資料的合作程式；shared memory |
| Process／thread | 位址空間各自一份／共用；thread 有自己的 PC、暫存器、stack |
| OpenMP | `#pragma omp parallel`、`omp_get_thread_num()`；scatter 再 gather／reduce |
| spawn／join | 開出 child thread／等全部做完 |
| Message passing | send(pid, val)、receive(pid 或 ANYSOURCE)；root 收 N − 1 個 |
| Amdahl's law | S = 1/(f_seq + (1 − f_seq)/P) → 1/f_seq |
| Flynn | SISD、SIMD（SSE、GPU）、MISD（投票）、MIMD（SPMD、MPMD） |
| 實測 | 14 核 20 thread、P/E 兩種核心；課本程式不能整除時會漏數；反推 f_seq ≈ 2.2%；E-core 拖慢平均切的工作；false sharing 慢 50% |

---

## 16. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| Shared memory 的正確性：coherence、memory consistency | [第 39 支](lecture39_note.md)：Coherence and Consistency |
| False sharing、cache coherence protocol（snoopy、directory） | **第 40 支**：Cache Coherence Protocols |
| Multithreaded processor、SIMD／vector processor | **第 41 支**：Multithreading and Vector Processing |
| GPU、interconnection network | **第 42 支**：GPUs and Interconnects |
| 頻率的限制、功耗 ∝ f³ | [第 32 支](lecture32_note.md#19-pipeline-的好處到底在哪裡) |
| Process、virtual memory、shared memory 的 OS 機制 | [第 37 支](lecture37_note.md) |
| 計算機架構的過去、現在、未來 | **第 47 支** |

---

## 附：本講一句話總結

> **因為功耗大約和頻率的三次方成正比，處理器頻率卡在 2–3 GHz，單核效能在 2008 年左右也飽和了，於是 Moore's law（電晶體數每兩年加倍，feature size 每代 ÷ √2）給的多餘電晶體改拿來在同一顆晶片上放更多核心，也就是 multicore；多處理器有 SMP（大家平等，最常見）和 AMP（master 分工給 slave），用法有 loosely coupled（不相干的程式，配 message passing：send／receive）和 strongly coupled（合作的程式，配 shared memory：共用位址空間的 thread），兩種平行加總的程式結構都是「循序初始化 → 平行算 partial sum（scatter）→ 循序合併（gather／reduce）」；Amdahl's law S = 1/(f_seq + (1 − f_seq)/P) 告訴我們處理器再多，加速也被循序部分限制在 1/f_seq；Flynn 把機器分成 SISD、SIMD、MISD、MIMD（SPMD、MPMD）。實測本機一顆晶片有 6 個 P-core 和 8 個 E-core 共 20 個 thread，課本的 OpenMP 程式在 SIZE 不能被 N 整除時會漏加、平均切工作時慢的 E-core 會拖累大家、相鄰 thread 寫同一條 cache line 慢了 50%（false sharing），運算量大的版本在 6 個 P-core 上快 5.41 倍，反推 f_seq ≈ 2.2%。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 11: Overview of Multiprocessor Systems, Part 1/5 錄影逐字稿（YouTube 自動字幕，1:01:44）、Sarangi *Basic Computer Architecture* v3.09 Ch12 開頭與 §12.1–12.3（p.565–579）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4 + OpenMP、Open MPI）結果。*
