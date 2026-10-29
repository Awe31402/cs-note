# Advanced Computer Architecture 第三十九支影片完整筆記：Multicore（八）：data race 偵測：lock set 演算法與 vector clock（Chapter 9, Part VIII）

> **課程**：Advanced Computer Architecture — Chapter 9: Multicore Systems (Part VIII) -- Data Race Detection Algorithms（播放清單第 39 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（46:37）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch9** §9.6.5–9.6.6（p.504–508）
> **章號**：影片 Chapter 9 = 課本 Ch9。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕把「lock set」寫成「log set／knob／mock」、「data race」寫成「database／data rays」、「precede」寫成「proceed」，筆記直接改正。[00:00] 老師說「lecture eight」：這是 Chapter 9 的第 8 講。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 為什麼要偵測 data race](#1-為什麼要偵測-data-race)
- [2. 靜態 vs 動態](#2-靜態-vs-動態)
- [3. Lock set 演算法（基本版）](#3-lock-set-演算法基本版)
- [4. 三種假警報](#4-三種假警報)
- [5. 加一個狀態機](#5-加一個狀態機)
- [6. Vector clock：從分散式系統借來的想法](#6-vector-clock從分散式系統借來的想法)
- [7. Vector clock 的比較](#7-vector-clock-的比較)
- [8. 訊息與 vector clock 的更新](#8-訊息與-vector-clock-的更新)
- [9. Vector clock 與因果關係](#9-vector-clock-與因果關係)
- [10. 用 vector clock 偵測 data race](#10-用-vector-clock-偵測-data-race)
- [11. 【實測】](#11-實測)
- [12. 一頁速查表](#12-一頁速查表)
- [13. 與後續講次的連結](#13-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
data race 是程式本身的性質，跟 memory model 無關；有 race 的話 SC 的 execution 裡一定找得到（第 38 支）
靜態分析太難 → 動態：instrument 每個讀寫，跑很多次
lock set：L(v) = L(v) ∩ L(T)；最後 L(v) 空了 → 可能有 race
  假警報：初始化、唯讀變數、reader-writer → 加狀態機 Start → Exclusive → Shared → Modified，只在 Modified 報
vector clock（Lamport）：每個 thread 一個 n 元素的向量；送訊息前 +1、收到後 +1 並取 max
  Vi ≺ Vj ⇔ ei → ej（happens-before）；比不出大小 = concurrent
  acquire：CT[tid]++、CT = CL = CT ∪ CL；讀：Wv ⪯ CT；寫：Wv ⪯ CT 且 Rv ⪯ CT；不成立 → data race
```

---

## 1. 為什麼要偵測 data race

[00:00] 這一講：偵測程式裡 data race 的方法。

[00:48] 上一講（[第 38 支](lecture38_note.md)）知道：**data race 是程式本身的性質**，跟 memory model 無關。目標：在程式裡找出 data race。

需要一個演算法測試程式碼：

- 跑過所有可能的控制路徑、給所有可能的輸入、做出盡量多的交錯（interleaving）。
- [01:17] 上一講證明了：程式有 data race 的話，**SC 的 execution 裡一定會出現**。
- [01:49] 所以只要照 SC 去做交錯，「容易很多」；照複雜的 memory model 去做交錯「會是很多工作」。
- 但還是要跑過盡量多的路徑，而且要**確定地證明**沒有 data race，「相當難」。

---

## 2. 靜態 vs 動態

| 做法 | 怎麼做 | 問題 |
|---|---|---|
| **靜態**（static） | [02:16] 用 compiler 分析程式碼 | 計算上很難：多項式次數的嘗試內得不到答案；[02:42] 有些問題難到演算法可能**永遠不會結束** |
| **動態**（dynamic） | [02:42] **instrument** 程式：回報讀寫了哪些變數，看有沒有 data race | [03:06] 跑幾百萬次，有 race 的話「很有可能會出現」 |

這一講講兩個動態的演算法：**lock set** 和 **vector clock**。

【課本補充】課本 p.505：這類方法大多是軟體：compiler 或執行期的模組 instrument 讀寫、記錄它們的相依關係，看到 data race 的徵兆就報錯。硬體的動態偵測也有，但很貴，而且最多只給**機率上**的保證。

---

## 3. Lock set 演算法（基本版）

[03:32] 兩個集合（演算法要模擬它們）：

| 符號 | 意思 |
|---|---|
| **L(v)** | 記憶體位置 v 的 lock set：保護 v 的鎖 |
| **L(T)** | thread T **現在拿著**的鎖 |

鎖用它的**位址**識別（每把鎖的位址不一樣，[第 37 支](lecture37_note.md) 講過怎麼 atomic 地設、清）。

[03:55] 每次存取 v 之後：

```
L(v) = L(v) ∩ L(T)          （課本式 9.11）
```

- [04:25] 一開始假設每個位置的 L(v) 是「**所有的鎖**」（無限大的集合）。第一個 thread 存取後，L(v) 就變成那個 thread 拿著的鎖；之後越來越小。
- [04:53] 意思：找出「要存取這個位置**必須**拿哪些鎖」。
- [05:21] 例：目前 L(v) = {a, b, c}；另一個 thread 只拿著 {b, c} 就來存取 → 看來 a 不是必要的 → L(v) = {b, c}。

[05:54] 程式結束時：

| L(v) | 意思 |
|---|---|
| 不是空的 | 找到了保護 v 的鎖 |
| **空的** | 有 thread 沒拿鎖就存取了 v，或兩次存取拿的鎖**沒有交集**：v 沒有被**同一組**鎖保護 → 可能有 data race |

[06:25] 標準做法：在多執行緒程式裡加 **annotation**：每個共用變數的讀寫、每次拿鎖放鎖都記錄。thread 的 lock set 一開始是空的；拿鎖時加進去、放鎖時拿掉。

[07:18] L(v) 空了「**可能**」涉及 data race：表示存取之間沒有同步。

【課本補充】課本 p.504：這個演算法出自 **Savage et al., 1997**。實作通常是軟體：每個共用變數的存取都加一段縮小 lock set 的程式碼，最後把 lock set 空了的變數報給程式設計師。

---

## 4. 三種假警報

[07:44] 不是每個 lock set 空了的情況都有害：有 **false positive**（假警報）。

| 樣式 | 為什麼被誤報 | 為什麼其實沒事 |
|---|---|---|
| **初始化** | [08:12] 一開始通常不加鎖就初始化變數 | 只有初始化的 thread 在用 |
| **唯讀變數** | [08:37] 初始化時寫一次，之後只讀 | 只讀不會衝突 |
| **Reader-writer** | 很多 thread 同時讀一個變數 | [09:01] 一個人寫、很多人一直讀，沒問題 |

---

## 5. 加一個狀態機

[09:30]（課本圖 9.59）每個變數一個 FSM：

```
Start ──寫──▶ Exclusive ──別的 thread 讀──▶ Shared ──寫──▶ Modified
               │（同一個 thread                │（讀）           （所有存取）
               │   的讀寫：留在這裡）            ▼                    ▲
               └────────────別的 thread 寫────────────────────────────┘
```

| 狀態 | 意思 | 做什麼 |
|---|---|---|
| **Start** | 還沒被存取 | [09:30] 第一次存取**一定是寫**（要先設初值） |
| **Exclusive** | [09:55] 一個 thread 獨有 | 同一個 thread 的讀寫都留在這裡，**不檢查**（初始化） |
| **Shared** | [10:21] 別的 thread 來**讀** | 一直讀就一直留著（[10:44]「寫一次、一直讀」的 reader-writer、唯讀變數）；**不報** |
| **Modified** | [11:18] 別的 thread 來**寫**（從 Exclusive 或 Shared） | [11:45] 跑一般的 lock set 演算法；之後所有存取都留在這裡；**只在這裡報** |

[12:37] 重點：

- 演算法很簡單：動態追蹤 L(v) 和 L(T)，每次存取算交集。
- [13:08] 單純這樣假警報太多 → 第一次寫的 thread、之後別人的讀都先不管 → 別的 thread 一寫就進 Modified，開始跑 lock set → **去掉大量的假警報**。
- [13:34] 「有大量的文獻討論各種 false positive、false negative，我不談。」狀態機的目的：把可能的 data race 縮到比較精確的一小群。

【課本補充】課本 p.505：在 Shared 狀態，即使很多 thread 在讀，也**不報** data race。

（本機實測狀態機去掉了哪些假警報、又漏掉了哪些真的 race，見 [§11](#11-實測)。）

---

## 6. Vector clock：從分散式系統借來的想法

[13:58] 第二個想法直接來自**分散式系統**：把一般的多執行緒執行環境想成一個分散式系統。

- 分散式系統**沒有全域時間**：每個 thread 有自己的**本地時鐘**。
- [14:29] thread 之間有互動時，本地時鐘照某個規則更新。

[14:51] n 個 thread，每個 thread 維護一個 **n 元素的向量** = **vector clock**：

- thread i 的 vector clock 是 **Vi**。
- [15:21] **Vi[j]**：i 對 j 的本地時鐘的**最佳估計**。
- **Vi[i]** 是唯一**準確**的元素（i 對自己時間的估計）；其他的可能不是最新的。

【課本補充】課本 p.506：在多核心、各有各的時鐘時，「時間」的定義本身就很模糊，所以兩個 concurrent 的事件不一定是同時發生的，要另外定義。事件分兩種：**內部**事件（別的 process 看不到）和**外部**事件（用送、收訊息表示）。

---

## 7. Vector clock 的比較

[16:23] 在 vector clock 上定義一套代數：

| 關係 | 定義 |
|---|---|
| **相等** | Vi = Vj ⇔ 所有 k，Vi[k] = Vj[k] |
| **先於**（≺） | [16:50] Vi ≺ Vj ⇔ Vi ≠ Vj，而且所有 k，Vi[k] ≤ Vj[k] |
| **先於或相等**（⪯） | [17:19] Vi = Vj 或 Vi ≺ Vj |

[17:19] 兩個 vector clock **不一定比得出大小**：

| 例子 | 結果 |
|---|---|
| Vi = (3, 4, 5)，Vj = (4, 5, 6) | Vi ≺ Vj |
| Vi = (3, 4, 5)，Vj = (2, 5, 6) | 第一個元素 3 > 2，第二個 4 < 5 → **比不出來** |

⚠️ [16:50] 老師口誤說 ≺ 是「total order」（全序），但他自己馬上舉了比不出大小的例子：≺ 其實是**偏序**（partial order）。

---

## 8. 訊息與 vector clock 的更新

[18:53] 每個 thread 看成一個 **process**（分散式系統的意思，不是 OS 的 process）。

[19:15] **訊息**：兩個 thread 之間的**互動**。

- 分散式系統：一個 thread 經過網路送訊息給另一個。
- [19:42] 多核心系統沒有這種直接的訊息，但 thread i 寫一個變數、thread j 讀它，也是一種互動 → 符合訊息的理論定義。
- [20:12] 這就是之前講的 **happens-before** 關係：x = 1 和另一個 thread 的 t1 = x 之間（受 memory model 限制；這裡主要看 SC）。

[21:30] i 送訊息給 j：

```
送之前：Vi[i]++                          ← 開始一個新事件
j 收到後：Vj[j]++，然後 對所有 k：Vj[k] = max(Vi[k], Vj[k])     （課本式 9.12）
```

- [22:56] i 送訊息時附上自己的 vector clock（timestamp）。
- [23:28] j 收到後**逐元素取最大值**（merge，記作 Vi ∪ Vj，課本叫 union）。
- [23:54] 目的：收的人**至少跟送的人一樣新**：送的人看過的，收的人至少也看過；還多記了一個「收到」事件。

---

## 9. Vector clock 與因果關係

[24:47] 因果（causality）：事件 i 導致事件 j、j 導致 k……

[25:13] 事件 ei（thread i，時間 Vi）、ej（thread j，時間 Vj）：

```
Vi ≺ Vj  ⇔  ei → ej（happens-before）          （課本式 9.15）
```

- [26:12] ei → ej ⇒ Vi ≺ Vj（「比較容易證」）：happens-before 有遞移性，中間有一串節點 k、k′……；每送一次訊息、取一次 max，就有 Vi ≺ Vk ≺ Vk′ ≺ … ≺ Vj。
- [27:09] 反方向：沒有一串 happens-before 邊的話，兩個 vector clock **比不出大小**，「這方向稍微難一點」。
- [28:03]「相信我：Vi ≺ Vj 就有 happens-before，反過來也是。」分散式系統一個非常有名的經典結果；vector clock 也叫 **Lamport 的 vector clock**。

ei、ej 比不出大小（Vi ⊀ Vj 而且 Vj ⊀ Vi）→ **concurrent**（記作 Vi ‖ Vj）。

⚠️ [28:03] 老師把 vector clock 歸給 Lamport；課本 p.506–507 只說是「經典的分散式系統文獻」，沒有寫是誰提出的。發明者是誰本機的資料查不到，**待補**。

【課本補充】課本 p.507 Definition 86：vector clock 一開始全部是 0；Theorem 9.6.6.1、9.6.6.2 是兩個方向。

---

## 10. 用 vector clock 偵測 data race

[28:30] 符號：

| 符號 | 意思 |
|---|---|
| **CT** | 目前 thread 的 vector clock |
| **CL** | 目前這把鎖的 vector clock |
| **Rv**、**Wv** | 變數 v 的**讀**時鐘、**寫**時鐘 |
| **tid** | thread id |
| **CT.inLock** | 目前在 critical section 裡嗎 |

**Lock acquire**（課本 Algorithm 1）：

```
CT[tid] ← CT[tid] + 1
CT ← CT ∪ CL
CL ← CT
CT.inLock ← True
```

- [29:28] 先把自己的時間 +1（像送訊息前）。
- [29:57] CT 和 CL 取 max，兩邊都設成它 → [30:48]「想成一個**會合點**（rendezvous）」：這時 thread 和鎖的資訊都是最新的、一樣的。
- [44:24] 也可以想成 thread 和鎖**互相送了訊息**：「我們要一樣新。」

**Lock release**（Algorithm 2）：[31:16] 只有 `CT.inLock ← False`。

**讀**（Algorithm 3）：

```
if not CT.inLock: CT[tid] ← CT[tid] + 1
if Wv ⪯ CT: Rv ← Rv ∪ CT
else: 宣告 data race
```

- [32:16] 不在 critical section 裡才 +1。
- [32:47] **寫時鐘要先於或等於現在的時間**：之前的寫跟現在的讀之間要有一串 happens-before（經過同步操作）。
- [34:11] 不成立 → 那個寫和這個讀之間**沒有** happens-before 鏈 → **data race**。
- [35:01] 成立 → 安全；把 Rv 跟 CT 取 max，讓之後的存取看得到「這個 thread 讀過了」。
- [43:01] 讀**不檢查** Rv：允許很多人同時讀（reader-writer）。

**寫**（Algorithm 4）：

```
if not CT.inLock: CT[tid] ← CT[tid] + 1
if Wv ⪯ CT and Rv ⪯ CT:
    Rv ← Rv ∪ CT;  Wv ← Wv ∪ CT
else: 宣告 data race
```

- [38:26] 之前的**寫**和之前的**讀**都要先於現在的時間：「兩個讀的順序不重要，但寫跟之前的讀、寫都要有順序。」
- [40:39] 其實 Rv ∪ CT 就等於 CT（Rv ⪯ CT 已經成立），直接設成 CT 也對，「寫成 union 比較好看」。
- [41:09] 寫完 Wv = CT，所以同一個 thread 之後的讀會有 Wv **等於** CT：這就是條件裡要有「等於」的原因。

[35:55] 為什麼要給鎖打時間戳：第一次 acquire 時鎖的時鐘是 v1，之後別人 acquire 時一定 > v1（v1 ≺ v2）→ 鎖的 acquire 之間的 happens-before **自動**被記下來。

[43:57] 換個說法：這裡被當成「**訊息**」的，就是**同步邊**（so 邊，[第 35 支 §8](lecture35_note.md#8-synchronization-邊so)）。

[45:47] 下一講：**transactional memory**，一個讓平行程式好寫很多的新做法。

【課本補充】課本 p.507：sync 變數（鎖）之間允許有 data race，因為系統保證它們的存取是 SC 的。

---

## 11. 【實測】

程式在 `experiments/lecture39/detect.py`：照課本寫了**基本版 lock set**、**加狀態機的 lock set**、**vector clock**（Algorithm 1–4）三個偵測器，跑 9 個 trace。trace 照順序執行（一種 SC 的交錯）。

「真的 race」那一欄不是手標的：程式照課本 Definition 84 自己算（同 thread 的程式順序 ∪ 同一把鎖的「release → 下一個 acquire」，取遞移閉包；不同 thread、至少一個寫、之間沒有 happens-before → race），而且 assert 跟手標的一樣。

| trace | 真的 race | lock set 基本版 | lock set＋狀態機 | vector clock |
|---|---|---|---|---|
| 都用同一把鎖保護 | − | − | − | − |
| 不保護的 counter++ | c | c | c | c |
| 用**不同**的鎖保護 | x | x | **−（漏掉）** | x |
| 初始化後大家只讀（中間沒有同步） | k | k | **−（漏掉）** | k |
| 初始化後大家只讀（建 thread 當作同步） | − | **k（假警報）** | − | − |
| 先不加鎖初始化，之後都加鎖 | − | **x（假警報）** | − | − |
| 用加鎖的旗子交棒，資料本身不加鎖 | − | **d（假警報）** | **d（假警報）** | − |
| 一個加鎖寫、一個不加鎖讀 | x | x | **−（漏掉）** | x |
| 先不加鎖寫，很久之後別人加鎖讀、自己再加鎖寫 | x | x | **−（漏掉）** | x |

- **vector clock 9 個全對**：它直接追蹤 happens-before，就是 data race 的定義本身。
- **基本版 lock set**：沒漏掉任何真的 race，但有 3 個假警報：初始化（[08:12]）、唯讀（[08:37]），還有「用加鎖的旗子交棒」：資料本身沒被任何鎖保護，但同步是經過旗子的鎖做的，lock set 看不出來。
- **加了狀態機**（[09:30]）：初始化、唯讀的假警報不見了，但**漏掉了 4 個真的 race**：在 Exclusive 狀態的存取不算進 lock set、在 Shared 狀態不報。所以狀態機是拿 false negative（漏報）換 false positive（假警報）。
- ⚠️ 「初始化後大家只讀」的例子：影片 [08:37] 和課本 p.505 說它「不是 data race」；照課本 Definition 84，初始化和讀之間**一定要有同步**（例如建 thread）才不算 race。實際程式裡通常是先初始化再建 thread，所以沒事；trace 裡沒有那個同步時，它照定義就是 race。
- 這只是我寫的簡化版（每個 trace 只跑一種交錯），不是 Savage et al. 的完整工具。

（第 38 支用 ThreadSanitizer 抓過真的 C 程式的 data race，見 [第 38 支 §14.4](lecture38_note.md#144-data-race-讓-compiler-把程式改壞)。）

---

## 12. 一頁速查表

| 主題 | 重點 |
|---|---|
| 目標 | data race 是程式的性質；有 race 的話 SC execution 裡一定找得到 |
| 靜態 vs 動態 | 靜態太難（可能不會結束）；動態：instrument、跑很多次 |
| Lock set | L(v) = L(v) ∩ L(T)；一開始是所有鎖；空了 → 可能有 race；Savage et al. 1997 |
| 假警報 | 初始化、唯讀變數、reader-writer |
| 狀態機 | Start →（寫）Exclusive →（別人讀）Shared →（寫）Modified；別人寫 → Modified；只在 Modified 報 |
| Vector clock | 每個 thread 一個 n 元素向量；Vi[i] 準確，Vi[j] 是估計 |
| 更新 | 送前 Vi[i]++；收後 Vj[j]++、逐元素 max |
| 比較 | Vi ≺ Vj：不相等且每個 ≤；可能比不出來（concurrent） |
| 因果 | Vi ≺ Vj ⇔ ei → ej |
| 偵測 | acquire：CT[tid]++、CT = CL = CT ∪ CL；讀：Wv ⪯ CT；寫：Wv ⪯ CT 且 Rv ⪯ CT；被當成訊息的是同步邊 |
| 實測 | vector clock 9 個 trace 全對；基本 lock set 3 個假警報；加狀態機後少了假警報，但漏掉 4 個真的 race |

---

## 13. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Data race 的定義、DRF ⇒ SC、有 race 就有 SC 的 race execution | [第 38 支](lecture38_note.md) |
| Happens-before、so 邊 | [第 35 支](lecture35_note.md) |
| 鎖、atomic 指令 | [第 37 支](lecture37_note.md) |
| 不用鎖的另一條路：transactional memory | [第 40 支](lecture40_note.md) |

---

## 附：本講一句話總結

> **data race 是程式本身的性質，而且有的話在 SC 的 execution 裡一定找得到，所以可以用動態的方法 instrument 每個讀寫、跑很多次去找：lock set 演算法對每個變數算「每次存取時拿著的鎖」的交集，交集空了就可能有 race，再加上 Start、Exclusive、Shared、Modified 的狀態機排除初始化、唯讀變數、reader-writer 這些假警報；vector clock 從分散式系統借來，每個 thread 一個向量，送訊息前加一、收到後逐元素取最大值，Vi ≺ Vj 剛好等於 happens-before，所以 acquire 時讓 thread 和鎖的時鐘會合，讀時檢查寫時鐘先於現在、寫時再加查讀時鐘，不成立就是 data race。本機實測：vector clock 在 9 個 trace 上全對，基本版 lock set 有 3 個假警報，加了狀態機之後假警報少了，卻漏掉 4 個真的 race。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 9: Multicore Systems (Part VIII) -- Data Race Detection Algorithms 的 YouTube 字幕（46:37）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch9 §9.6.5–9.6.6（p.504–508）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
