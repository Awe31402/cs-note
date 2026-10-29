# Basic Computer Architecture 第三十九支影片完整筆記：Flynn 分類、coherence 與 memory consistency（Chapter 11 Coherence and Consistency, Part 2/5 → 課本 Ch12）

> **課程**：Basic Computer Architecture — Chapt 11: Coherence and Consistency, Part 2/5（播放清單第 39 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（1:23:11，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 **Ch12** §12.3–12.4.3（p.578–591）
> **章號**：影片的 Chapter 11 是課本 v3.09 的 **Chapter 12**；影片說的「chapter 8／9」是課本 Ch9／Ch10。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、pthreads）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Flynn 分類（接上一講）](#1-flynn-分類接上一講)
- [2. MIMD：SPMD 與 MPMD](#2-mimdspmd-與-mpmd)
- [3. Shared memory 多處理器的邏輯觀點](#3-shared-memory-多處理器的邏輯觀點)
- [4. 為什麼「統一的記憶體」很難做](#4-為什麼統一的記憶體很難做)
- [5. 同一個位置：coherence](#5-同一個位置coherence)
- [6. Data race](#6-data-race)
- [7. 寫兩次、讀兩次：哪些結果合理](#7-寫兩次讀兩次哪些結果合理)
- [8. Coherence 的兩個公理](#8-coherence-的兩個公理)
- [9. Coherence 的意義：看起來像一個大 cache](#9-coherence-的意義看起來像一個大-cache)
- [10. 不同的位置：memory consistency](#10-不同的位置memory-consistency)
- [11. Program order 與 memory model](#11-program-order-與-memory-model)
- [12. Sequential consistency](#12-sequential-consistency)
- [13. SC 的代價](#13-sc-的代價)
- [14. Weak consistency 與 fence](#14-weak-consistency-與-fence)
- [15. 自己寫平行加總：SC 機器](#15-自己寫平行加總sc-機器)
- [16. 【原始碼】Linux 的 memory barrier](#16-原始碼linux-的-memory-barrier)
- [17. 【實測】](#17-實測)
- [18. 一頁速查表](#18-一頁速查表)
- [19. 與後續講次的連結](#19-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
Flynn：SISD、SIMD、MISD（飛機投票）、MIMD（SPMD、MPMD）
shared memory 的難處：很多 cache、buffer、在路上的訊息 → 順序不確定
coherence（同一個位置）：completion axiom ＋ order axiom
memory consistency（不同位置）：
  sequential consistency（SC）：所有 thread 依 program order 交錯
  SC 很慢 → weak consistency（WC）＋ fence
自己寫平行加總：doneInit、finished[] 旗標（在 SC 機器上才對）
```

---

## 1. Flynn 分類（接上一講）

[00:00]–[06:39]　課本 §12.3（p.578）

| | 意思 | 例子 |
|---|---|---|
| **SISD** | 一個指令流、一個資料流 | 一般的單處理器（第 27–33 支的 pipeline），「對我們來說沒什麼好講」 |
| **SIMD** | 一條指令處理很多份資料 | 一條 add 同時算 a+b、c+d、e+f；一條指令同時算 4 個值的 sin⁻¹；vector processor（第 41 支） |
| **MISD** | 很多指令流、同一份資料 | 非常少見，見下 |
| **MIMD** | 很多指令流、很多資料流 | 上一講的平行加總 |

**MISD 的經典例子：飛機**（【口頭】[03:07]）：
- 處理器可能有 bug，在飛機上可能造成大量傷亡：機長要左轉，電腦卻右轉。
- 現代飛機有**三個**處理器：MIPS、ARM、x86，吃**同一份**資料（飛機感測器、操控的資料）。
- 飛機公司找**三組不同的程式設計師**各寫一份，分別編成 MIPS、ARM、x86。目標、輸入輸出格式都一樣，但寫的人不同，「至少不會犯一樣的錯」。
- 最後**多數決**：兩個說左轉就左轉。

---

## 2. MIMD：SPMD 與 MPMD

[06:39]–[12:32]　課本 §12.3（p.578–579）

**為什麼上一講的平行加總是 MIMD**（【口頭】[07:30]）：雖然大家跑同一個程式，但重點是「**同一時間**是不是在執行同一條指令」。不同處理器速度不同（例如一個在慢的處理器上），而且 root 和非 root 做的事不一樣 → 指令流不同。每個 thread 處理陣列的不同部分 → 資料流也不同。

| | 意思 | 例子 |
|---|---|---|
| **SPMD**（single program multiple data） | 同一個編譯好的程式，依 thread／process id 做不同的事 | 上一講的 OpenMP、MPI 程式；**大部分**平行程式 |
| **MPMD**（multiple program multiple data） | 程式真的不一樣：一個 master 程式分工給 slave 程式 | **IBM Cell** 處理器（【口頭】「至少到 2017 年已經不在市面上了」） |

這講之後主要看 SPMD。

---

## 3. Shared memory 多處理器的邏輯觀點

[12:32]–[14:26]　課本 §12.4.1、Fig 12.5（p.579–580）

和 Von Neumann 機器類似，但同一顆晶片裡有 **n 個處理器**，各有自己的 pipeline，**共用**整個記憶體空間：

```
處理器 1   處理器 2   …   處理器 n
    \         |            /
         共用的記憶體
```

**統一的記憶體視角**：處理器 1 寫 x = 5，之後處理器 2 讀 x 會得到 5；處理器 2 寫 y = 10，之後處理器 1 讀 y 得到 10。thread 之間就靠這個溝通。【口頭】「這是理想狀況，實際上會有不同的實作。」

【課本補充】p.580：
- 如果是不同虛擬位址空間的 process，各用各的資料，沒有問題。難的是**共享資料的 thread**。（不同 process 也能把不同的虛擬 page 對到同一個 frame 來共享，[第 37 支](lecture37_note.md#15-進階功能shared-memory-與-protection)，算是特例。）
- thread 自己的資料在 stack 上。兩種做法：大家用同一個虛擬位址空間、stack pointer 從不同地方開始（要確保 stack 不會長到重疊）；或者讓每個 thread 的 stack 部分在 page table 裡對到不同的 frame，其他部分（程式碼、常數、heap）共用。
- 唯讀的程式碼、不共享的區域變數都不是問題；**可能被多個 thread 共享的資料**才是。

---

## 4. 為什麼「統一的記憶體」很難做

[14:26]–[19:33]　課本 §12.4.2（p.581）

【口頭】[14:40] 實際的記憶體系統不是一個結構，而是**很多很多**結構：
- 每個處理器有自己的小 L1 cache，再接到一個大的 L2。
- cache 之間有線路，線路上有 buffer；還有 victim cache、write buffer（[第 36 支](lecture36_note.md)）。
- 和記憶體系統溝通是**送訊息**：讀訊息或寫訊息。同一時間有很多訊息**還在路上**（in flight）。
- 對編譯器來說讀寫只是一條指令，但從 MA 級出去後，可能到一個 cache、再到 victim cache、再到另一個 cache……一連串訊息。

**順序不確定**（【口頭】[17:31]）：
- 處理器 1 讀 x、處理器 2 在差不多的時間寫 x = 5（x 初值 0）。
- 同一個程式跑很多次：有時讀在寫之前（讀到 0），有時寫在讀之前（讀到 5）。
- 這叫 **non-deterministic**（不確定的）。很多東西平行在跑，每次的條件不可能完全一樣。
- **不同處理器**之間的訊息順序沒有定義；就算同一個處理器，在某些系統也不一定。

記憶體系統的行為分成兩類：
- 對**同一個位置**的存取 → **coherence**（也叫 cache coherence）
- 對**不同位置**的存取 → **memory consistency**

【課本補充】p.581：每個核心有一個小的私有 L1，這些 L1 合起來要**看起來像一個** L1。所以一個位址 x 可能同時有好幾份**複本**散在各個私有 cache 裡，任何寫入都要更新所有複本：同步這些複本就是 cache coherence 要解決的問題。訊息有時要幾十個週期才到，流量大時更久，還可能被後送的訊息**超車**。

---

## 5. 同一個位置：coherence

[19:33]–[20:51]　課本 §12.4.2（p.581）

接下來所有例子的約定：
- 所有 **global 變數一開始都是 0**。
- 以 **t** 開頭的變數（t1、t2……）是**區域變數**，放在暫存器裡，不在記憶體。

---

## 6. Data race

[20:51]–[24:28]　課本 p.581

```
Thread 1:      Thread 2:
x = 1          t1 = x
```

t1 一定是 1 嗎？**不一定**，可能是 0 或 1，看兩條指令誰先。

- 這種程式碼叫 **data race**（資料競爭），這裡競爭的是 x。
- 「讀」和「寫」都是對 **global 變數**說的：`t1 = x` 是對 x 的**讀**（寫 t1 不算），`x = 1` 是對 x 的**寫**。
- 中間沒有任何東西規定誰先：可能讀的那邊 cache miss、寫的那邊 hit，或反過來。

【課本補充】p.581：如果 thread 2 是 2 ns 或 10 ns 之後才讀呢？寫入要多久才傳到別的 thread，取決於記憶體系統的實作（匯流排、cache 快不快）。

---

## 7. 寫兩次、讀兩次：哪些結果合理

[24:28]–[30:14]　課本 Example 151（p.582）

```
Thread 1:      Thread 2:
x = 1          t1 = x
x = 2          t2 = x
```

**(t1, t2) = (2, 1) 可以嗎？** 這表示讀或寫被調換了順序。

【口頭】[25:05]「答案很明確：**不可以**，這個結果一點都不直覺。」thread 1 依序寫 1 再寫 2，其他 thread 也應該照這個順序看到。

| 結果 | 怎麼發生 | 合法？ |
|---|---|---|
| (0, 0) | 兩個讀都在兩個寫之前 | ✓ |
| (0, 1) | 讀 t1 → 寫 1 → 讀 t2 | ✓ |
| (0, 2) | 讀 t1 → 兩個寫很快做完（thread 2 被中斷卡住）→ 讀 t2 | ✓ |
| (1, 1) | 寫 1 → thread 1 被中斷 → thread 2 很快讀兩次 | ✓ |
| (1, 2) | 寫 1 → 讀 → 寫 2 → 讀 | ✓ |
| (2, 2) | 兩個寫都做完才讀 | ✓ |
| (2, 1)、(1, 0)、(2, 0) | 「先看到後來的值，再看到之前的值」 | ✗ |

規律：x 的值依序是 0 → 1 → 2，所以 thread 2 讀到的值**只能不變或變大**，不能倒退。

**時間夠久的話**（【口頭】[30:29]）：寫 x = 1 之後過了很久（10 天、3 週……對處理器來說 2 分鐘就是無限久），中間沒有別人改 x，再讀 x 應該要讀到 1：**寫入不能消失**。

【課本補充】p.582：
- 課本列出的六個合法結果和上表一樣。
- 「如果允許 (2,1)，多處理器的記憶體系統會比較好做，但平行程式會很難寫、很難推理」，所以大部分多處理器不允許。
- 課本鼓勵讀者用 OpenMP 或 pthreads 寫個程式看看會出現哪些結果。（【實測】見 [§17.1](#171-coherencex86-從來沒出現-21)。）

---

## 8. Coherence 的兩個公理

[30:14]–[39:38]　課本 Definition 122（p.582–583）

**Coherence**：對**同一個記憶體位址**的存取行為。兩個公理（axiom）：

| 公理 | 內容 |
|---|---|
| **Completion**（完成） | 寫入**最終一定會完成**：最後所有處理器都看得到。「最終」是有限的時間，可以是 1 個週期，也可以是 1 年，理論上不規定 |
| **Order**（順序） | 對同一個位址的**所有寫入**，**所有 thread 看到的順序都一樣** |

換個說法（【口頭】[35:16]）：
- completion：讀寫訊息**不會遺失**，一定會到目的地。
- order：對同一位置的寫入訊息**不會被不同的 thread 看成不同的順序**。

【口頭】[38:47] 老師當場改投影片：原本寫「寫入訊息不會被**重新排序**」，改成「不會被**看成不同的順序**」。

**四個 thread 的例子**（【口頭】[35:52]）：

```
T1: x = 1     T2: x = 2     T3: t1 = x; t2 = x     T4: t3 = x; t4 = x
```

- 兩個寫之間沒有先後規定，哪個先都可以。
- 但**同一次執行**裡，T3 讀到 (1, 2)（表示 1 在 2 之前）、T4 卻讀到 (2, 1)（表示 2 在 1 之前）→ **違反 order axiom**。
- 「我不能說我現在手上有拿筆、又說我現在手上沒有拿筆。」兩者矛盾，只能有一個是真的。
- T3、T4 讀到 (0, 1)、(0, 2) 都可以。

【課本補充】p.583：completion 的意思是「寫 10 到 x 的請求不會被記憶體系統丟掉」，它可能之後被別的寫入蓋掉，但一定會先更新到那個位置。「理論上這只是冰山一角」，作者的進階教科書有更多。

---

## 9. Coherence 的意義：看起來像一個大 cache

[39:38]–[44:14]　課本 p.583

這兩個公理保證：一個 coherent 的記憶體系統，看起來就像**所有處理器共用一個 cache、每個記憶體字只有一個存放位置**。

【口頭】[40:06] 想像所有處理器都接到一個大記憶體：
- **completion**：寫入請求在佇列裡，最終會到那個格子。
- **order**：有個外部觀察者一直盯著 x 那一格，看到它 0 → 1 → 2。如果有人說「我先讀到 2 再讀到 1」，觀察者會說「不可能，x 是先變 1 再變 2」。

【口頭】[42:23]「給不想深入、只想知道 coherence 到底在講什麼的人」：
- 系統 A：很多核心，記憶體系統非常複雜，一堆結構和線路。
- 系統 B：很多核心接到**一個大 cache**，沒有子 cache、沒有 buffer，每個 global 變數有唯一的位置。
- 外部觀察者看同一個程式在 A 和 B 上的行為，應該**一模一樣**：不能在 A 上看到任何不直覺的事。只要 A 遵守 completion 和 order，就「通過 coherence 測試」。

【口頭】讀者可以試著證明：遵守 coherence 公理的複雜系統和簡單系統行為大致相同，「但這當然很複雜」。

---

## 10. 不同的位置：memory consistency

[44:14]–[49:36]　課本 §12.4.3、Example 152、Fig 12.6（p.583–584）

coherence 管**一個**位置；**consistency 管好幾個不同的位置**。

```
Thread 1:      Thread 2:
x = 1          t1 = y
y = 1          t2 = x
```

| (t1, t2) | 怎麼發生 |
|---|---|
| (0, 0) | thread 2 的兩條先做完，thread 1 後做 |
| (0, 1) | x = 1 → t1 = y → t2 = x → y = 1 |
| (1, 1) | thread 1 的兩條先做完 |
| **(1, 0)？** | t1 = 1 表示 y = 1 已經做了，所以 x = 1 也做了（在它前面）→ t2 一定是 1 |

【口頭】[47:59] 只要把兩個 thread 的指令**交錯**，而且**不改變**同一個 thread 裡的順序，(1, 0) 就不可能出現。這很合理：指令依程式順序經過 pipeline，再從 MA 級送到記憶體系統。

【課本補充】p.583–584：(1, 0) 會在「寫 x 被記憶體系統延遲、寫 y 很快完成」時發生。要不要允許它，「沒有對錯，取決於我們想怎麼寫軟體、硬體設計者想做出什麼」：允許的話軟體難寫，但硬體好做。

---

## 11. Program order 與 memory model

[49:36]–[54:41]　課本 Definition 123–124（p.584）

**Program order**（程式順序）：和每個 thread 的語意（指令的先後）一致的順序。

【口頭】[51:05] 例子：`add r1, r2, r3`（指令 1）在 `ld`（指令 2）前面 → 處理器應該先執行 1 再執行 2。有迴圈時是**動態**指令序列。單週期處理器（課本 Ch9）或 pipeline（課本 Ch10）就是照指令進來的順序執行、離開：這就是 program order。我們只關心其中的讀和寫。

**Memory model**（記憶體模型，也叫 **memory consistency model**）：決定平行程式**有哪些可能（合法）結果**的記憶體系統模型。

- 一個平行程式可以有很多種執行方式；model 說哪些合法、哪些不合法。
- 上例的 model：(0,0)、(0,1)、(1,1) 合法，(1,0) 不合法。
- 不同機器可以有不同的 model，有的機器可能允許 (1,0)。
- 機器遵守某個 model → 任何執行都只會產生那個 model 允許的結果。
- 【口頭】[54:06] 老師當場把投影片的「likely」補上「valid」：likely／valid outcomes。

---

## 12. Sequential consistency

[54:41]–[57:53]　課本 Definition 125（p.584–585）

把兩個 thread 的指令（1、2、3 和 1′、2′、3′）任意交錯，例如：

```
1  1′ 2  3  2′ 3′
1  2  1′ 2′ 3  3′
...
```

每種交錯都有一個性質：**同一個 thread 裡**，1 一定在 2 前、2 一定在 3 前（program order 只講 thread 內，不講 thread 之間）。

【口頭】[56:17]「就像把兩隻手的五根手指交叉：交叉的方式很多，但同一隻手的手指順序不會變。」

- 這種保留 program order 的交錯叫 **sequentially consistent interleaving**（1978 年提出的老名詞）。
- **只允許**這種交錯的 memory model 叫 **sequential consistency（SC）**。
- 上例的 (1, 0) 不是 SC 允許的結果。

【課本補充】p.585：
- 「sequential」的意思：結果等於**一個處理器**依某種順序執行所有 thread 的指令（每個 thread 的 program order 都保留）。所以一個單處理器輪流跑很多 thread，產生的一定是 SC 執行。
- SC **不代表每次結果都一樣**，那取決於交錯方式和各 thread 的延遲；它只說某些結果不允許。

---

## 13. SC 的代價

[57:53]–[66:02]　課本 p.585

【口頭】「這是進階課程的題目，這裡只講一點點。」

**例 1：兩個寫**

```
x = 1    ← x 不在 L1，在 L2
y = 1    ← y 在 L1
```

- SC 要求 x 的寫完全完成，才能送出 y 的寫 → MA 級要等 L2 來回。
- 很多處理器的做法：x 送出去就執行下一條，y 在 L1 很快就完成。外面的人會看到「先 y 再 x」。
- 這個 thread 自己的執行完全正確，只是**別的處理器看到的順序變了**：效能換來的代價是 memory model 改變。

**例 2：先寫後讀**

```
x = 1      ← 進了 write buffer（只有自己看得到）
t1 = y     ← 直接去 L1 或更下層拿到值
```

- write buffer：小小的 4 或 8 個 entry 的 cache，吸收寫入。【口頭】一個 64-byte block 裡每個 int 都寫一次，不想送 16 次寫到 L1，就先在 write buffer 收集起來一次送。
- write buffer 是**私有**的，外面看不到裡面的寫。外面的觀察者會看到**先讀 y、後寫 x**。
- 不同位置的話會這樣；同一個位置則有 coherence 規則管。

**結論**（【口頭】[64:02]）：SC 不允許很多提升效能的設計：
- 不能用 write buffer（寫會延後，讀可能先完成）。
- 不能把寫入重新排序（x 在主記憶體、y 在 L1，也要乾等）。

「大部分時候，等目前的記憶體請求完成才送下一個，是**非常嚴格而且人為**的限制，通常誰先完成根本沒差」，只要換個方式寫程式。所以**幾乎所有商用系統都不遵守 SC**。【口頭】「現在連手機都有 4 或 8 核」，大家至少要有點概念。

【課本補充】p.585：一個寫請求 W **完成**，是指之後任何處理器的讀都保證拿到 W 的值（或之後對同位置的寫）。讀請求完成，是指讀到資料、而且寫那個資料的寫請求也完成了。

---

## 14. Weak consistency 與 fence

[66:02]–[70:09]　課本 Definition 126–127（p.585–587）

**Weak consistency（WC，弱一致性）**：不遵守 SC，允許（在某些限制下）**任意的順序**。

- **同一個 thread、同一個位置**一定照順序：寫 x = 1 再讀 x，一定讀到 1（或之後的寫），coherence 和單 thread 的順序會保證。
- 同一個 thread、**不同位置**：寫 x 再讀 y，對單一 thread 誰先都沒差，但對多處理器有差，而 WC 允許調換。
- 好處：可以放進更多提升效能的設計，效能比較高。

【口頭】[67:36]「到今天沒有人用 SC。」IBM、HP 這些大伺服器公司的 memory model **非常寬鬆**；Intel 和 ARM「某種程度上沒那麼弱」，但這裡沒辦法講到底多弱。

**安全的假設**（【口頭】[68:07]）：假設系統是 WC，實際上比較強，程式還是對的；反過來假設是 SC、實際上是 WC，程式就可能錯。為了簡單，假設 WC = 不同位置的讀寫**完全不保證順序**。

**Fence**（柵欄）：WC 系統裡的**同步指令**（synchronization instruction）。

```
讀、寫、讀、寫……
fence      ← 等前面所有記憶體存取都完成，fence 才執行
讀、寫……   ← fence 完成之後才能開始
```

- 讀的「完成」= 拿到值回來；寫的「完成」= 真的改到那個記憶體位置。
- fence 前後的記憶體請求，生命期**完全分開**。

【課本補充】p.586–587：
- WC 為什麼會產生 (1, 0)：x 的位置靠近核心 2、y 的位置靠近核心 1，送請求要幾十個週期而且不固定。核心 1 把寫 x 交給記憶體系統就算完成、進 RW 級，下個週期再交寫 y；寫 y 很快到了，寫 x 還在路上。核心 2 剛好在這時讀 y（新值 1）、讀 x（舊值 0）。
- 「一般來說別的 thread 不會剛好同時讀這些位置」，執行時又分不出來，所以為了效能，不值得每個請求都等前一個完成。
- **Atomicity**（Definition 127）：記憶體請求在送出後的某個時間點，被**所有** thread 看成**瞬間**完成。
- Important Point 18：每個請求有三個時間點：**start**（MA 級送給 L1）、**finish**（指令進到 RW 級）、**complete**（真的到了記憶體位置、所有處理器都看得到）。簡單處理器的 complete 在 start 和 finish 之間；高效能處理器很少這樣。甚至有些 memory model 的寫入**不是 atomic**（不同 thread 看到它在不同時間完成）。
- **Trivia 4**：作者帳戶有 200 美元，在 ATM 存了一張朋友給的 300 美元支票；3 天後開了 400 美元的房租支票寄給房東；隔天房東打電話來罵：支票跳票了。原來暴風雪讓銀行好幾天沒去 ATM 收支票。投進 ATM 是 finish，5 天後入帳才是 complete，中間房東（另一個 thread）去兌現 → 現實生活的 weak consistency。

---

## 15. 自己寫平行加總：SC 機器

[70:09]–[83:11]　課本 Example 153（p.588–590）

【口頭】[71:29]「OpenMP 是函式庫，是別人寫好給我們用的。好的電腦科學家可以用別人的程式碼，但要知道它怎麼運作。」所以不用 OpenMP，自己寫。先假設機器是 **SC**（一定要先假設一個 memory model，程式才會對）。

【課本補充】p.588：不用 OpenMP 的另一個原因：OpenMP 在背後做了很多事，讓程式在 weak memory model 的機器上也能正確執行。

```c
int partialSums[N];
int finished[N];
int numbers[SIZE];
int result = 0;
int doneInit = 0;
/* 把 partialSums 和 finished 全部設成 0 */
...
doneInit = 1;
parallel {                                    /* 假設的指令：N 份同時執行 */
    while (!doneInit) {};                     /* 等初始化完成 */
    int myId = getThreadId();
    int startIdx = myId * SIZE/N;
    int endIdx = startIdx + SIZE/N;
    for (int jdx = startIdx; jdx < endIdx; jdx++)
        partialSums[myId] += numbers[jdx];
    finished[myId] = 1;                       /* 我做完了 */
}
/* 等所有 thread 做完 */
do {
    flag = 1;
    for (int i = 0; i < N; i++)
        if (finished[i] == 0) { flag = 0; break; }
} while (flag == 0);
/* 算最後結果 */
for (int idx = 0; idx < N; idx++)
    result += partialSums[idx];
```

⚠️ 投影片上的 `while (!doneInit()){};` 多了一對括號，【口頭】[74:55]「這個括號不該在這裡」。課本 p.589 也印成 `doneInit()`。

**為什麼要 doneInit**（【口頭】[75:10]）：
- SC 只保證**同一個 thread 裡**照 program order。
- 新開的 thread 有自己的讀寫，和開它的 thread 的初始化寫入之間**沒有順序保證**。
- 所以用一個旗標：看到 doneInit = 1，依 SC，它前面的初始化一定都做完了。

【口頭】[74:06] 真的要寫，可以用 Linux 的 **pthreads** 取代 `parallel`，「但那就太技術細節了」。

**為什麼看到 finished[i] = 1 就能放心加 partialSums[i]**（【口頭】[81:14]）：在 SC 機器上，`finished[myId] = 1` 已經執行，表示同一個 thread 在它前面對 partialSums 的寫也都完成了。

和 OpenMP 的差別（【口頭】[82:03]）：只有平行迴圈的**開頭**（等初始化）和**結尾**（等大家做完）需要自己寫，中間完全一樣。OpenMP 把這些藏起來了。

【課本補充】p.590：在 **weak** memory model 上，這個程式**不對**：記憶體系統可能把 finished[i] 和 partialSums[i] 的寫調換，看到 finished[i] = 1 時 partialSums[i] 還沒更新。Important Point 19：WC 下同一個 thread 自己看到的一定是 program order，但**別的 thread** 可能看到不同的順序。解法是加 fence（Example 154，下一講）。

---

## 16. 【原始碼】Linux 的 memory barrier

Linux 核心用 `smp_mb()`（完整 fence）、`smp_rmb()`（讀的 fence）、`smp_wmb()`（寫的 fence）讓程式在各種 memory model 上都正確。x86 版本，`linux-src/arch/x86/include/asm/barrier.h:57-60`：

```c
#define __smp_mb()	asm volatile("lock; addl $0,-4(%%" _ASM_SP ")" ::: "memory", "cc")

#define __smp_rmb()	dma_rmb()
#define __smp_wmb()	barrier()
```

ARM64 版本，`linux-src/arch/arm64/include/asm/barrier.h:116-118`：

```c
#define __smp_mb()	dmb(ish)
#define __smp_rmb()	dmb(ishld)
#define __smp_wmb()	dmb(ishst)
```

- **x86 的 `smp_wmb()` 只是 `barrier()`**：barrier() 只叫編譯器不要調換順序，**不產生任何指令**。因為 x86 硬體本來就不會把兩個寫調換順序。`smp_rmb()`（dma_rmb() 在 x86-64 也是 barrier()）也一樣。
- **x86 的 `smp_mb()` 是真的指令**：`lock; addl $0, -4(%rsp)`（對 stack 上一個位置加 0，加上 lock 前綴就有完整 fence 的效果）。x86 唯一會調換的是「先寫後讀不同位置」，要擋就要這個。
- **ARM64 三個都是真的指令**（`dmb`，data memory barrier 的不同版本）：ARM 是比較弱的 memory model，讀讀、寫寫都可能被調換。

這正好對應影片說的「Intel 和 ARM 沒那麼弱，但還是會調換」：x86 只會發生 §13 的例 2（write buffer），不會發生例 1。

---

## 17. 【實測】

`experiments/lecture39/litmus.c`：用兩個 thread（綁在兩個不同的 P-core）把三種「litmus test」各跑 **200 萬次**，數每種 (t1, t2) 出現幾次。每次開始前隨機等一下，讓兩邊的先後每次不同。`experiments/lecture39/litmus.c:29-37`：

```c
		if (!strcmp(test, "coh")) {
			if (me == 0) { x = 1; x = 2; } else { r1 = x; r2 = x; }
		} else if (!strcmp(test, "mp")) {
			if (me == 0) { x = 1; y = 1; } else { r1 = y; r2 = x; }
		} else {
			int fence = !strcmp(test, "sbf");
			if (me == 0) { x = 1; if (fence) asm volatile("mfence" ::: "memory"); r1 = y; }
			else         { y = 1; if (fence) asm volatile("mfence" ::: "memory"); r2 = x; }
		}
```

```
$ gcc -O2 -pthread -o litmus litmus.c
$ for t in coh mp sb sbf; do ./litmus $t; done
coh:  (0,0)=1509871  (0,2)=944  (1,1)=11383  (1,2)=1  (2,2)=477801
mp :  (0,0)=1767470  (0,1)=2038  (1,1)=230492
sb :  (0,0)=627896  (0,1)=711126  (1,0)=660978
sbf:  (0,1)=897890  (1,0)=1096790  (1,1)=5320
```

### 17.1 Coherence：x86 從來沒出現 (2,1)

`coh` 就是 Example 151。出現的 (0,0)、(0,2)、(1,1)、(1,2)、(2,2) 都在課本的合法清單裡；**(2,1)、(1,0)、(2,0) 一次都沒有**。order axiom 成立。（(0,1) 剛好沒出現，只是時間點沒湊到。）

### 17.2 寫寫、讀讀：x86 沒有調換

`mp` 就是 Example 152（影片 §10）。**(1,0) 在 200 萬次裡一次都沒有**。x86 不會把兩個寫調換，也不會把兩個讀調換，所以這個例子在 x86 上和 SC 一樣。這也是為什麼 Linux 在 x86 上 `smp_wmb()`、`smp_rmb()` 不用產生指令（§16）。

### 17.3 先寫後讀：write buffer 真的會讓 SC 失效

`sb` 是影片 §13 的例 2，兩邊對稱地寫：

```
Thread 1:      Thread 2:
x = 1          y = 1
t1 = y         t2 = x
```

用 SC 推理：不管怎麼交錯，至少有一邊的寫會在另一邊的讀之前，**不可能 (0,0)**。

實測：**(0,0) 出現了 627,896 次**（約 31%）！兩邊的寫都還在各自的 write buffer 裡，讀就先拿到了舊值。這就是影片說的「write buffer 讓讀比寫先完成」，x86 的 memory model 允許它。

`sbf` 在寫和讀之間加 `mfence`：**(0,0) 變成 0 次**，而且出現了以前沒有的 (1,1)。fence 讓寫一定先完成才讀，回到 SC 的行為。

### 17.4 編譯器也會改順序

第一版程式的 r1、r2 沒宣告成 `volatile`，結果四個測試**全部** 200 萬次都是 (0,0)。原因：主程式自己從來不寫 r1、r2，gcc -O2 就把讀 r1、r2 的動作**搬到迴圈外面**，只讀一次。所以 memory model 不只是硬體的事，編譯器也會調換、合併記憶體存取；Linux 的 `barrier()`（§16）就是在擋編譯器。

---

## 18. 一頁速查表

| 主題 | 重點 |
|---|---|
| SISD／SIMD／MISD／MIMD | 單處理器／一條指令多份資料／飛機三處理器投票／multicore |
| SPMD／MPMD | 同一程式依 id 做不同事（最常見）／不同程式，master + slave（IBM Cell） |
| 邏輯觀點 | 很多處理器共用一個記憶體；寫了別人要讀得到 |
| 難處 | 很多 cache、buffer、在路上的訊息；順序不確定 |
| Data race | 讀寫同一變數、沒有規定先後 |
| Coherence | 同一個位址；completion（寫一定完成）＋ order（大家看到同樣的寫入順序） |
| Example 151 | x=1;x=2 ‖ t1=x;t2=x：(2,1) 不合法 |
| Coherent 的意義 | 看起來像所有處理器共用一個大 cache |
| Consistency | 不同位址的存取順序 |
| Example 152 | x=1;y=1 ‖ t1=y;t2=x：SC 不准 (1,0) |
| Program order | thread 內指令的順序 |
| Memory model | 決定哪些結果合法 |
| SC | 只允許保留 program order 的交錯；等於一個處理器輪流執行 |
| SC 的代價 | 不能用 write buffer、不能調換寫入 → 慢；商用系統都不用 |
| WC | 不同位置任意順序；同位置同 thread 一定照順序 |
| Fence | 前面全部完成才執行，後面等它完成才開始 |
| 自寫平行加總 | doneInit、finished[]；只在 SC 上對，WC 要加 fence |
| Linux x86 | smp_wmb／smp_rmb = 只擋編譯器；smp_mb = lock addl |
| 實測 | x86：(2,1)、MP 的 (1,0) 從沒出現；SB 的 (0,0) 出現 31%，加 mfence 變 0 |

---

## 19. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| 用 fence 改寫平行加總（Example 154）、cache coherence protocol（snoopy、MSI、directory） | [第 40 支](lecture40_note.md)：Cache Coherence Protocols |
| SIMD、vector processor | [第 41 支](lecture41_note.md) |
| write buffer | [第 36 支](lecture36_note.md#13-降低-miss-penaltywrite-buffer) |
| false sharing（coherence 的效能代價） | [第 38 支](lecture38_note.md#144-false-sharing-的影子) |
| 共享記憶體的 OS 機制 | [第 37 支](lecture37_note.md#15-進階功能shared-memory-與-protection) |
| 更正式的 memory model（TSO、release consistency）、證明 | Advanced Computer Architecture 課程（[進階課程筆記](../advanced/README.md)） |

---

## 附：本講一句話總結

> **Flynn 把機器分成 SISD、SIMD、MISD（像飛機用三個處理器跑三份不同的程式再投票）、MIMD（又分 SPMD 和 MPMD）；shared memory 多處理器邏輯上是很多處理器共用一個記憶體，但實際上有很多私有 cache、buffer 和在路上的訊息，所以要規定行為：對同一個位置的叫 coherence，有 completion（寫入最終一定完成）和 order（所有 thread 看到同一位置的寫入順序都一樣）兩個公理，讓複雜的系統看起來像一個大 cache；對不同位置的叫 memory consistency，sequential consistency 只允許把各 thread 依 program order 交錯（所以 x=1;y=1 ‖ t1=y;t2=x 不會有 (1,0)），但它不允許 write buffer 和調換寫入，太慢，所以商用系統都用較弱的 model，再用 fence 指令在需要的地方強制順序；不用 OpenMP 自己寫平行加總時，要用 doneInit 和 finished[] 旗標，而且只在 SC 機器上才對。實測在本機 x86 上跑 200 萬次：coherence 的 (2,1) 和 message passing 的 (1,0) 從沒出現，但「先寫後讀」的 (0,0) 出現了 31%，加 mfence 就消失，正好對應 Linux 在 x86 上 smp_wmb 只擋編譯器、smp_mb 才用 lock addl。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 11: Coherence and Consistency, Part 2/5 錄影逐字稿（YouTube 自動字幕，1:23:11）、Sarangi *Basic Computer Architecture* v3.09 Ch12 §12.3–12.4.3（p.578–591）、Linux v6.2.12 `arch/x86/include/asm/barrier.h`、`arch/arm64/include/asm/barrier.h` 整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、pthreads）結果。*
