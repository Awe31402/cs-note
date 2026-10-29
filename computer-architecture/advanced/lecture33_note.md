# Advanced Computer Architecture 第三十三支影片完整筆記：Multicore（二）：distributed cache、coherence 與 consistency、legal sequential execution（Chapter 9, Part II）

> **課程**：Advanced Computer Architecture — Chapter 9: Multicore Systems (Part II) -- Overview of coherence and consistency（播放清單第 33 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕下載不到）（1:13:50）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch9** §9.2–9.3.1（p.425–435）
> **章號**：影片 Chapter 9 = 課本 Ch9。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 把「write buffer」聽成「right buffer」、「deadlocks、livelocks」聽成「dead logs、live logs」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 理論基礎這十幾年才清楚](#1-理論基礎這十幾年才清楚)
- [2. 想要一個又大又慢又多 port 的 L1？](#2-想要一個又大又慢又多-port-的-l1)
- [3. Distributed cache 跟 multibank cache 不一樣](#3-distributed-cache-跟-multibank-cache-不一樣)
- [4. Coherence](#4-coherence)
- [5. 例子的假設](#5-例子的假設)
- [6. 例子一：先寫 x 再寫 y](#6-例子一先寫-x-再寫-y)
- [7. 例子二：先寫再讀](#7-例子二先寫再讀)
- [8. Memory consistency model](#8-memory-consistency-model)
- [9. 觀察者：start、completion、end](#9-觀察者startcompletionend)
- [10. Execution 與 operation](#10-execution-與-operation)
- [11. Legal sequential execution](#11-legal-sequential-execution)
- [12. 觀察者坐在 core 上](#12-觀察者坐在-core-上)
- [13. Parallel execution](#13-parallel-execution)
- [14. Atomicity](#14-atomicity)
- [15. Execution 的等價](#15-execution-的等價)
- [16. 小結](#16-小結)
- [17. 【實測】](#17-實測)
- [18. 一頁速查表](#18-一頁速查表)
- [19. 與後續講次的連結](#19-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
很多 core 共用一個 L1？太大、太慢、port 太多 → 每個 core 一個私有的 L1（sister cache）
但同一個變數會有好幾個副本 → 要讓它們看起來像一個位置：coherence
就算一個位置沒問題，多個位置的讀寫也會出現「不直覺」的結果（NoC 塞車、write buffer、OOO 的 load 先跑）
→ 處理器要給一個規格：memory consistency model；照規格的 execution 叫 consistent
理論工具：
  execution = 一串 operation（thread、開始、結束、讀/寫、位址、值）
  legal sequential execution：每個讀都拿到最近一次寫的值
  parallel execution 如果等價於一個 legal sequential execution，就是直覺上合理的
```

---

## 1. 理論基礎這十幾年才清楚

[00:00] 【口頭】「我在念書的時候，很多東西還沒被理解，至少沒有理解到可以簡單解釋的程度。但過去 10–15 年有很多理論研究，現在清楚多了。」課本有這些理論研究的參考資料。

---

## 2. 想要一個又大又慢又多 port 的 L1？

[01:19] 問題：cache 很大 → 切成很多 bank、散在晶片上 → 要 NoC 連 → 存取時間 10–50 cycle 不等，還有擁塞；NoC 有自己的問題（deadlock、starvation、livelock），就算都避開了，訊息的時間也很不固定。core 多 → 平行的存取多。

[02:08] 所以我們面對的是一個**又大、又慢、又多 port** 的 cache。

- L3 這樣還做得出來：到 L3 的請求被 L1 過濾掉很多。L2 可能也行。
- **L1 最麻煩**：每個 core 每個 cycle 有 1–2 個存取，16 個 core 就可能 **32 個同時的存取**，而且位址還是共用的（一個 core 寫、另一個讀）。

[03:11] 概念上：兩個 core 一定要用**同一個 L1**，不然一個 core 寫了變數，另一個 core 看不到 → 違反 shared memory 的程式模型。

[05:43] 實際做法：**每個 core 一個私有的 L1**，但讓整組 L1「**看起來**」像一個 cache：對外面（軟體、core、任何在這個框框外面的人）來說，是一組分散的 L1 還是一個 L1，沒有差別。

---

## 3. Distributed cache 跟 multibank cache 不一樣

[06:33] 「為什麼不像大 cache 一樣，把位址空間切開、分給不同的 bank？」

[07:12] 因為：x 可能被放在離我很遠的 bank，寫要很久，**讀也要很久** → core 變很慢。L1 本來就應該**靠近 core**、**1–2 個 cycle** 就能存取。所以好幾個 core 存取同一個 x 時，**每個都要有自己的副本（replica）**。

| | Multibank cache | Distributed cache |
|---|---|---|
| 一個變數 | **一個**副本，可能很遠 | **好幾個**副本，每個 core 旁邊都有一份 |
| 快不快 | 遠的話很慢 | 快：讀自己 L1 裡的副本 |
| 問題 | | 副本要**同步**，不然 shared memory 就壞了 |

[09:06] 每個 L1 叫一個 **sister cache**：小、快；各自獨立存取 → 等於有很多 port；大部分時候不用存取別人的 L1。L1 之間可以用 bus 或 NoC 連。

[10:01] 「房間裡的大象」：**正確性**。好幾個副本怎麼同步？不然我寫 x、別人也寫 x，有人看不到寫入，或有人看到一種順序、別人看到另一種，一團亂。

---

## 4. Coherence

[11:30] 只看**一個**位址（一個變數，例如 x）。只有一個實體位置的話（shared cache），distributed 設計就沒好處了；所以要有好幾個位置存 x，但這些副本要有一樣的值，不然程式的行為會不直覺。

[12:22] **Coherence**：讓 sister cache 裡的**好幾個實體位置**表現得像**一個位置**。做到了，就能做出快速的 distributed cache，多執行緒的程式就能跑得快。

---

## 5. 例子的假設

[13:27] 接下來所有例子的假設：

| 假設 | 說明 |
|---|---|
| **全域變數** | 初始都是 0，名字是 u、v、x、y、z；跨 thread、跨 core 共用 |
| **區域變數** | 對到暫存器，名字是 t1、t2、t3……；寫入**其他 core 看不到** |
| **時間** | 兩條連續指令之間的間隔可以**無限長**（可能 context switch、core 卡住……）。只能說「之前」、「之後」，**不能用 cycle 數** |

[15:49] 【口頭】「大部分學生在這裡犯錯：假設兩件事會同時發生，或在某幾個 cycle 之內發生。在並行系統的理論裡，我們只用『之前』和『之後』。」

---

## 6. 例子一：先寫 x 再寫 y

[15:49]

```
Thread 1      Thread 2
x = 1;        t1 = y;
y = 1;        t2 = x;
```

沒有同步 → 有很多種交錯、很多種結果。

[17:37] 問：**t1 = 1、t2 = 0** 可能嗎？

- 直覺：沒有任何一種**循序的順序**能產生它：t1 = 1 表示 y = 1 已經做了，那 x = 1 一定也做了（它在 y = 1 前面），t2 一定是 1。
- 「這不符合我們直覺的標準。」

[19:21] 但在真的系統裡**可能**（thread i 跑在 core i 上）：

- thread 1 寫 x = 1，送一個更新訊息到 NoC 上 → 這個訊息**塞車**了。
- thread 1 寫 y = 1，這個訊息很快就送到了。
- thread 2 讀 y：1。讀 x：x 的更新還卡在路上 → **0**。

[21:14] 更貼近的例子：**write buffer**（[第 22 支](lecture22_note.md#6-write-buffer)）。x 的寫入放在 core 1 的 write buffer 裡，別的 core 看不到；y 的寫入繞過 write buffer，直接更新了 → core 2 讀到 y = 1、x = 0。

[22:24] 「我們可能不喜歡，但有這些複雜的功能，這個結果**真的、真的、真的可能**。」

⚠️ 這一點跟本機的 x86 不一樣：本機跑了 100 萬次，**一次都沒看到** t1 = 1、t2 = 0（[§17.1](#171-兩個例子在本機跑-100-萬次)）。x86 保證 store 之間的順序、load 之間的順序（它的 memory model 不允許這個結果，[第 38 支](lecture38_note.md) 會講 memory model）。老師說的是「一般的系統可能發生」，x86 只是剛好把它禁止了。

---

## 7. 例子二：先寫再讀

[23:21]

```
Thread 1      Thread 2
x = 1;        y = 1;
t1 = y;       t2 = x;
```

問：**t1 = 0、t2 = 0** 可能嗎？

[23:57] 直覺上不可能：不管怎麼交錯，總有一個 store 先做，之後另一邊的 load 至少有一個會讀到 1。兩個都讀到 0 不可能。

[25:20] 「但是、但是、但是、但是，常識沒那麼常見，碰到多核心時沒有什麼是合理的。」看兩個 core 裡面的 **OOO pipeline**：

- **store** 在 **commit** 時才真的寫（[第 13 支](lecture13_note.md#5-load-和-store-的-commit)）。
- **load** 只要位址算好、LSQ 的條件都滿足，就**提早**執行（[第 12 支](lecture12_note.md)）。
- 兩個是**不同的位址**，不會轉送 → load **超過**了 store。
- 兩邊都一樣 → 兩個 load 都讀到 0。

[27:06] 「任何有 OOO 執行、load 提早送出、用 LSQ 處理相依的系統，都會看到這個結果。」正式的名字叫 **write-to-read ordering**（之後講）。

[25:20] 【口頭】「我邀請大家用 OpenMP 或 pthreads 寫這個程式，在 ARM 或 x86 上跑一百萬次，記下所有結果。我保證你會看到這個結果。」

（本機照做了：真的看到了，見 [§17.1](#171-兩個例子在本機跑-100-萬次)。）

---

## 8. Memory consistency model

[27:59] 多執行緒的程式有很多種結果，有些合理、有些不合理。不合理的是因為有一堆複雜的東西：OOO 處理器、LSQ、write buffer、MSHR、NoC……而且是 distributed cache 不是 shared cache。

[29:28] 關鍵的問題：**給一個平行程式，哪些結果是有效的？** OOO pipeline、記憶體系統、NoC、記憶體控制器都可能「搗蛋」。

[30:10] 「給一個混亂的世界一點秩序」：每個處理器有一份**規格**，說明哪些結果和行為是允許的。

- 一個 **execution**（所有指令讀到的值、寫的值）照著規格 → **consistent**；否則不是。
- 規格由處理器的設計者公布：**memory model**、**memory consistency model**（Intel 的、IBM 的、ARM 的，各不相同）。
- [31:43] 在 Intel 機器上跑的程式，產生的 execution 一定照 Intel 的 memory model（除非有 bug）。

[32:39] 定義：

| 詞 | 定義 |
|---|---|
| **Memory consistency model** | 規定平行多執行緒程式行為的政策。多執行緒程式依 thread 排程的相對順序和記憶體操作的行為，會有很多種結果；memory model **限制**允許的結果。它是一組規定記憶體指令之間互動的規則 |
| **Coherence** | memory model 的一個**子集**：規定**單一位置**的行為。通常用在 cache：讓 shared cache 和 distributed cache 對外面看起來一樣（**cache coherence**） |

【課本補充】課本 p.426 §9.2.2、p.428 §9.2.3：coherence 和 consistency 的差別。

---

## 9. 觀察者：start、completion、end

[35:15] **觀察者**的概念很重要：誰坐在哪裡、觀察並記錄什麼。

坐在 cache 的**某一個位置**上的觀察者：請求進來排成一個 FIFO queue，每個請求有三個時間：

| 時間 | 意思 |
|---|---|
| **start** | 開始（例如進 queue） |
| **completion** | **看起來真的執行**的那一刻：讀的話，值被感測、鎖進 cache 的 buffer；寫的話，SRAM cell 的值真的改了 |
| **end** | 結束，從 queue 移除、清掉狀態 |

[36:38] 同一個位置只有一個 port → 請求之間**不重疊**：start、completion、end、start、completion、end……

---

## 10. Execution 與 operation

[37:34] **Execution** = 一串 **operation**。每個 operation 有 6 個欄位：

| 欄位 | |
|---|---|
| thread ID | 哪個 thread 發的 |
| start time | |
| end time | |
| 類型 | 讀或寫 |
| 位址 | |
| 值 | load：讀到的值；store：寫的值 |

---

## 11. Legal sequential execution

[38:24] **Sequential execution**：所有 operation 都**排好順序**（第 1、2、3……個；理論上叫「跟自然數的 bijection」）。

[39:20] **Legal sequential execution**：每一個**讀**都拿到**同一個位址上最近一次寫**的值（往前找最近的寫，拿它的值）。

[40:10] 坐在一個記憶體位置上的觀察者，應該看到一個 legal sequential execution：請求一個一個來，所以是 sequential；每次讀都拿到最近一次寫的值，才合理（legal）。

【口頭】「懂了的話，深呼吸，喝杯咖啡，下一張會複雜一點。」

【課本補充】課本 p.432 的定義。

---

## 12. 觀察者坐在 core 上

[40:58] 觀察者換到 **core** 上（有 OOO pipeline）：

| | start | completion | end |
|---|---|---|---|
| **load** | 進 pipeline | 在中間某處讀到值 | commit |
| **store** | 進 pipeline | **不知道**：要寫到記憶體系統、**其他所有 core 都看得到**才算完成 | 離開 ROB |

[42:33] store commit 了，可能還卡在 write buffer、MSHR 或 NoC 上的訊息裡，還沒到別的 core。所以可能 **t_start < t_end < t_comp**：completion 在 end 之後。[§7](#7-例子二先寫再讀) 的例子就是：store 的完成很晚，後面的 load 反而先完成。

[44:33] 但是：core 上的觀察者還是期望看到 legal sequential execution：我寫了 x = 1，之後我讀 x，一定讀到 1 或**別的 core 之後寫的值**，絕不能讀到 0。

[45:34] 單執行緒的程式只做**兩個假設**：

1. 指令一條接一條執行。
2. 讀到的永遠是最近一次寫的值。

「就算有別的 core，這個抽象也要成立。」

---

## 13. Parallel execution

[48:16] 多個 core、各跑一個 thread（thread i 在 core i 上）。記號：

| 記號 | 意思 |
|---|---|
| **Rx1** | 讀 x，讀到 1 |
| **Wy2** | 寫 y = 2 |

[48:16] 每個 thread 一個觀察者，各自記錄自己的歷史。

[50:39] 例子：7 個 operation，3 個 thread，畫在時間軸上（框的頭尾是 start、end，中間的藍點是 completion；寫的 completion 可以在 end 之前）。**假設**觀察者知道每個 operation 的 completion time，而且 operation 在那一刻**瞬間**執行。

[51:32] 實際的系統大致是這樣：讀是一個 cycle 的一小部分（把 sense amplifier 的值鎖進 buffer 那一刻）；寫是設定 SRAM cell 那一刻。

[52:35] 所有觀察者最後開會，照 **completion time** 排序：

```
1 Rx0 → 2 Wx1 → 3 Rx1 → 4 Wy1 → 5 Wy2 → 6 Wx3 → 7 Ry2
```

- 可以排成一列 → **sequential**。
- 每個讀都拿到最近一次寫的值（3 讀到 2 寫的 1；7 讀到 5 寫的 2）→ **legal**。
- 雖然這些 operation 來自**不同的 thread**，還是能排成一個 legal sequential execution。

---

## 14. Atomicity

[55:36] 從上面學到的：

| 性質 | 意思 |
|---|---|
| **Atomicity** | 每個 operation 只有**一個** completion time，在那一刻**瞬間**執行 → 叫 **atomic** |
| 多個觀察者 | 單獨一個觀察者不知道別人看到什麼；開完會、排成一條時間軸，才能把 parallel execution 化成 sequential execution，非常直覺 |

是不是所有架構都有 atomicity，之後看。

【課本補充】課本 p.434 Definition 68：memory operation 看起來在某個時間 t 瞬間執行，就叫 atomic。

---

## 15. Execution 的等價

[58:06] 正式地定義「parallel execution P 等價於 sequential execution S」：

```
P|T：P 裡 thread T 發的所有 operation，照原本的順序
S|T：S 裡 thread T 發的所有 operation，照原本的順序（「投影」）
P|T ≡ S|T：兩串之間有一對一的對應（同樣讀寫同樣的值；時間可以不同，只看順序）
P ≡ S：對所有 thread T 都有 P|T ≡ S|T
```

[63:44] 例子：P 裡 thread 1 的是 2、6；S 裡是 2′、6′（Wx1、Wx3），6′ 在 2′ 後面。thread 2 的 1、5 → 1′、5′（Rx0、Wy2）。thread 3 的 3、4、7 → 3′、4′、7′。**每個 thread 都對得上** → P ≡ S。

[66:07] 意義：

- 一個「全能的全域觀察者」能看到所有事。**實際上幾乎 100% 不知道真正的 completion time**，但沒關係。
- 只要能把所有記憶體操作排成一個**合法的順序**（每個讀拿到最近一次寫），這就是**一個可能的、真實系統裡會發生的完成順序**。
- [68:31] 能化成 legal sequential execution → 這個結果直覺、正確；化不成（像 [§6](#6-例子一先寫-x-再寫-y)、[§7](#7-例子二先寫再讀) 的例子）→ 不直覺。但真的處理器**有時候允許**這種化不成的 execution。

【課本補充】課本 p.434–435 Figure 9.7：這是「並行理論裡最有效的工具之一」。

---

## 16. 小結

[69:41]

1. 多核心處理器裡面非常複雜：write buffer、MSHR、NoC、distributed cache……為了效能加了這些結構，**多執行緒**的程式就可能有很奇怪的結果（單執行緒不會）。
2. 所以需要規格（memory model）；照規格的 execution 叫 consistent。
3. 單執行緒：不管觀察者在記憶體位置還是 core 上，看到的都是 legal sequential execution。
4. 多執行緒：如果能把 parallel execution 等價成一個 sequential execution，就能用 sequential 的方式推理；它又是 legal 的話，就更確定它直覺、正確。

[72:18] 目標：把 parallel execution 化成 sequential execution（可能 legal、可能不 legal），在混亂的世界裡建立一個正確性的框架。下一講：**sequential consistency**。

---

## 17. 【實測】

程式在 `experiments/lecture33/`。

### 17.1 兩個例子在本機跑 100 萬次

照 [25:20] 老師的邀請，`litmus.c` 用 pthreads 把兩個例子各跑 **100 萬次**。thread 1 綁在 cpu2、thread 2 綁在 cpu4（兩個不同的 P-core）；每一輪用一組新的 x、y（不同的 cache line、一開始都是 0），兩個 thread 在每一輪開始前用一個計數器會合。另外把例子二再跑一次，在 store 和 load 中間加 x86 的 **`mfence`**。跑兩次：

| 程式 | (0,0) | (0,1) | (1,0) | (1,1) |
|---|---|---|---|---|
| **例子一（MP）**：x=1; y=1 ‖ t1=y; t2=x | 411756／4927 | 121793／15873 | **0／0** | 466451／979200 |
| **例子二（SB）**：x=1; t1=y ‖ y=1; t2=x | **8978／1375** | 512198／984679 | 440225／7052 | 38599／6894 |
| 例子二 ＋ `mfence` | **0／0** | 114041／146353 | 71097／6259 | 814862／847388 |

（MP、SB 是這兩種例子常見的名字：message passing、store buffering。）

- **例子二真的出現了 t1 = 0、t2 = 0**（8978 次、1375 次）：跟 [27:06] 說的一樣，兩邊的 load 都超過了自己的 store。
- 加了 **`mfence`**（強迫前面的 store 完成、別的 core 都看得到之後，後面的 load 才能執行）就**一次都沒有**了。
- ⚠️ **例子一的 t1 = 1、t2 = 0 一次都沒出現**：x86 不允許 store 和 store 之間、load 和 load 之間互換順序，所以 [§6](#6-例子一先寫-x-再寫-y) 說的情況在本機的 x86 上不會發生。在 ARM 這種比較弱的 memory model 上可能看得到（本機沒有 ARM，**待補**）。
- 每種結果的次數兩次差很多（例如例子一的 (0,0) 41 萬次對 5 千次）：取決於兩個 thread 每一輪誰先開始，跟 OS 排程、時脈等很多事有關，不重要；重要的是**哪些結果出現過、哪些從來沒出現**。

### 17.2 哪些結果能化成循序的順序

`sc.py`：把兩個 thread 的指令**所有的交錯**都列出來，一條一條執行，看能得到哪些結果（也就是能不能化成一個 legal sequential execution）：

```
MP：一條一條交錯能得到的 (t1,t2)：[(0, 0), (0, 1), (1, 1)]；得不到的：[(1, 0)]
SB：一條一條交錯能得到的 (t1,t2)：[(0, 1), (1, 0), (1, 1)]；得不到的：[(0, 0)]
```

- 跟 [§6](#6-例子一先寫-x-再寫-y)、[§7](#7-例子二先寫再讀) 的推理一致：MP 的 (1,0)、SB 的 (0,0) **沒有任何**循序的順序能產生。
- 對照 [§17.1](#171-兩個例子在本機跑-100-萬次)：本機的 x86 產生了 SB 的 (0,0)，也就是一個**不能**化成 legal sequential execution 的結果；但沒有產生 MP 的 (1,0)。真的處理器的 memory model 介於「一切都直覺」和「什麼都可能」之間，下一講開始講怎麼精確地描述。

---

## 18. 一頁速查表

| 主題 | 重點 |
|---|---|
| 為什麼要 distributed L1 | 共用的 L1 太大、太慢、port 太多（16 core × 2 = 32 個存取） |
| Sister cache | 每個 core 一個私有 L1，合起來要看起來像一個 cache |
| vs multibank | multibank：一個副本、可能很遠；distributed：每個 core 一份副本、快，但要同步 |
| Coherence | 好幾個實體位置看起來像一個位置（單一位址） |
| 假設 | 全域 u–z 初值 0、區域 t1、t2 是暫存器；只講之前／之後，不講 cycle |
| 例子一（MP） | x=1;y=1 ‖ t1=y;t2=x → t1=1,t2=0？NoC 塞車、write buffer 可能造成 |
| 例子二（SB） | x=1;t1=y ‖ y=1;t2=x → 0,0？OOO 的 load 超過 store：真的會發生 |
| Memory model | 處理器的規格，限制允許的結果；coherence 是它的子集（單一位置） |
| 觀察者 | 在記憶體位置：start、completion、end 不重疊；在 core：store 的 completion 可能在 end 之後 |
| Execution | 一串 operation（thread、start、end、R/W、位址、值） |
| Legal sequential | 排成一列、每個讀拿到最近一次寫的值 |
| Atomicity | 每個 operation 在某一刻瞬間執行 |
| 等價 | P ≡ S ⟺ 對所有 thread T，P\|T ≡ S\|T（一對一、順序一樣） |
| 實測 | 本機 x86 跑 100 萬次：SB 出現 (0,0)、加 mfence 就沒了；MP 的 (1,0) 從沒出現（x86 不允許） |

---

## 19. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| 平行程式、thread | [第 32 支](lecture32_note.md) |
| Write buffer、MSHR | [第 22 支](lecture22_note.md)、[第 25 支](lecture25_note.md) |
| Store 在 commit 時才寫、LSQ 讓 load 先跑 | [第 12 支](lecture12_note.md)、[第 13 支](lecture13_note.md) |
| NoC 的延遲不固定 | [第 27 支](lecture27_note.md)–[第 31 支](lecture31_note.md) |
| Sequential consistency、PLSC | [第 34 支](lecture34_note.md) |
| Execution witness | [第 35 支](lecture35_note.md) |
| Coherence 協定 | [第 36 支](lecture36_note.md)、[第 37 支](lecture37_note.md) |
| x86 這類真的 memory model、fence | [第 38 支](lecture38_note.md) |

---

## 附：本講一句話總結

> **很多 core 共用一個 L1 會又大、又慢、port 又多，所以每個 core 都有自己的私有 L1（sister cache），但同一個變數就有了好幾個副本，要靠 coherence 讓它們看起來像一個位置；而就算是不同的位址，NoC 塞車、write buffer 或 OOO pipeline 裡 load 超過 store，都會讓多執行緒程式出現「沒有任何循序順序能產生」的結果，所以處理器要公布一份 memory consistency model 規定哪些結果合法；分析這些結果的工具是：execution 是一串帶有 thread、開始、結束、讀寫、位址和值的 operation，legal sequential execution 是排成一列且每個讀都拿到最近一次寫的值，而如果一個 parallel execution 的每個 thread 的操作順序都跟某個 legal sequential execution 一樣（兩者等價），它就是直覺上合理的。本機照老師的邀請把兩個例子各跑了 100 萬次：「先寫再讀」真的出現了兩個 load 都讀到 0 的結果，加上 mfence 就消失；「先寫 x 再寫 y、反過來讀」的不直覺結果則一次都沒出現，因為 x86 的 memory model 不允許。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 9: Multicore Systems (Part II) -- Overview of coherence and consistency 的 whisper 轉錄稿（本機 `large-v3-turbo`，1:13:50）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch9 §9.2–9.3.1（p.425–435）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
