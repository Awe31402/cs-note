# Advanced Computer Architecture 第三十二支影片完整筆記：Multicore（一）：平行程式、Amdahl 與 Gustafson、Flynn 分類與 hardware thread（Chapter 9, Part I）

> **課程**：Advanced Computer Architecture — Chapter 9: Multicore Systems (Part I) -- Parallel Programming, Hardware Threads（播放清單第 32 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（1:18:12）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch9** §9.1（p.415–425）
> **章號**：影片 Chapter 9 = 課本 Ch9。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、6 個 P-core（各 2 個 hardware thread）＋ 8 個 E-core、Ubuntu、kernel 6.5.0、gcc 11.4、OpenMP、Open MPI 4.1.2）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕把「Amdahl」寫成「amal／amel／amdel」、「Gustafson-Barsis」寫成「Gustafson baris」、「Flynn」寫成「Flint」、「SISD」寫成「CISD」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 這一章](#1-這一章)
- [2. 單核心的效能飽和了](#2-單核心的效能飽和了)
- [3. Process 與 thread](#3-process-與-thread)
- [4. 例子：把一個大陣列加起來](#4-例子把一個大陣列加起來)
- [5. Shared memory：OpenMP](#5-shared-memoryopenmp)
- [6. Message passing：MPI](#6-message-passingmpi)
- [7. 兩種比較](#7-兩種比較)
- [8. Amdahl's law](#8-amdahls-law)
- [9. Gustafson-Barsis's law](#9-gustafson-barsiss-law)
- [10. Flynn 分類](#10-flynn-分類)
- [11. SPMD 與 MPMD](#11-spmd-與-mpmd)
- [12. Hardware thread](#12-hardware-thread)
- [13. Coarse-grained 與 fine-grained multithreading](#13-coarse-grained-與-fine-grained-multithreading)
- [14. Pipeline 要改哪些](#14-pipeline-要改哪些)
- [15. SMT 與 hyperthreading](#15-smt-與-hyperthreading)
- [16. 小結](#16-小結)
- [17. 【實測】](#17-實測)
- [18. 一頁速查表](#18-一頁速查表)
- [19. 與後續講次的連結](#19-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
單核心效能 2008 年後飽和 → 大問題切成小問題、分給多個 core
process（自己的位址空間）vs thread（共用 heap、程式碼；自己的 stack、暫存器）
map（切開、各算部分和）→ reduce（加起來）
shared memory（OpenMP；好寫、不好擴充）vs message passing（MPI；難寫、可以到幾十萬個節點）
Amdahl：speedup = 1 / (f_seq + (1 − f_seq)/P) → 上限 1/f_seq
Gustafson：問題跟著變大 → speedup = f_seq + (1 − f_seq)·P → 跟 P 成正比
Flynn：SISD、SIMD、MISD（飛機）、MIMD（SPMD、MPMD）
hardware thread：一個 core 跑好幾個 process，填滿閒著的 issue slot
  coarse-grained、fine-grained、SMT（動態分 issue slot）、hyperthreading（靜態切兩半）
```

---

## 1. 這一章

[00:00] 「目前為止**最長**、也**最複雜**的一章」，但會切成很簡單的小節。它結合了前面所有的東西：pipeline、cache、on-chip network。

[00:50] 背景：基本的資料結構，特別是**圖**：**有沒有 cycle**、**topological sort**。這一章大量建立在 Ch7、Ch8 上。

[01:30] 內容：

1. 平行程式概論（本講）。
2. **理論基礎**（很多張投影片，「為整章打基礎，非常重要」）。
3. **Cache coherence** 和 **memory model**：懂了理論基礎就很容易。
4. **Data race**，還有一個對整個系統的正確性非常重要的定理。
5. **Transactional memory**：「嚴格來說是選讀的」。

「懂了前四個，大概就夠當一個多核心工程師生存、發展了。」

---

## 2. 單核心的效能飽和了

[02:47] 公開資料：x 軸 1999–2012 年，y 軸是 **SPECint 2006** 分數（SPEC 發的標準 benchmark；分數是相對某台舊機器的執行時間比，當作任意單位）。每一點是一顆 CPU。

[04:05] **單核心效能逐漸飽和**，2008、2010 年後大致不變。還會長一點，是因為電晶體變小、塞得下更多 cache；但效率沒有跟著成比例地提高：

- [04:39] **design rule**（VLSI 製程裡擺放電晶體、邏輯閘的規則）越來越**嚴格**，不能隨便把不同的結構擺在一起 → 效能成長是**次線性**的。

[05:48] 解法：大問題切成**小問題**，交給**多個 core** 合作解決。每個小問題先分給一個 **thread**，再對到一個 core。

---

## 3. Process 與 thread

[06:26]

| | Process | Thread |
|---|---|---|
| 是什麼 | 執行中的程式的一個實例（例：雙擊 PowerPoint） | **輕量的 process**，有一群「姊妹 thread」 |
| 位址空間 | 自己的虛擬記憶體、開的檔案、網路連線 | **大部分共用**：heap（動態配置的陣列）、程式碼 |
| 私有的 | | **自己的 stack 和暫存器** |
| 溝通 | | 讀寫**共用的記憶體** |

[07:34] Linux 有 **thread group**：不是一個 process，而是一個 thread group，裡面好幾個 thread。

【課本補充】課本 p.416：thread 有自己的 program counter 和 local stack，用來放區域變數。

---

## 4. 例子：把一個大陣列加起來

[08:51] 一個很大的陣列 `numbers`（例如十億個元素），算所有元素的和（不管 overflow）：

```
把陣列切成 N 份 → 每份給一個 thread 算部分和（partial sum）→ 把 N 個部分和加起來
       └──────────── map ────────────┘                    └──── reduce ────┘
```

[10:54] 這就是流行的程式範式 **MapReduce**。

【口頭】老師說課本有完整的程式；影片只介紹兩種範式，語法「會很像，但不是精確的」。

---

## 5. Shared memory：OpenMP

[11:37] **Shared memory** 範式：N 個 core 透過共用的記憶體溝通：一個 core 寫 `x = 5`，之後另一個讀 `x` 就拿到 5。

- 【口頭】「什麼時候寫、什麼時候讀，之後會有 50 張投影片解釋；現在先假設照直覺的方式發生。」
- 共用的變數：**不是**宣告在 stack 上的，通常在 heap，或至少對 thread 跑的程式碼是「全域」的。

[12:47] **OpenMP**：C/C++ 的擴充，幾乎所有 C/C++ compiler 都支援，會自動編成平行的程式碼。

```c
#define N ...                       /* thread 數 */
int partialSums[N];                 /* 假設都初始化成 0 */
int numbers[SIZE];
int result = 0;

#pragma omp parallel                /* 大括號裡的程式碼複製 N 份，平行跑 */
{
    int myId = omp_get_thread_num();          /* 0 到 N−1 */
    int startIdx = myId * SIZE / N;
    int endIdx = startIdx + SIZE / N;
    for (int j = startIdx; j < endIdx; j++)
        partialSums[myId] += numbers[j];
}
/* 循序的部分：只有原來那個 thread */
for (int i = 0; i < N; i++)
    result += partialSums[i];
```

- [14:43] core 比 thread 少的話，一個 core 要用分時的方式跑好幾個 thread。
- [17:58] 結構：**平行的部分**（map）→ **循序的部分**（reduce）。這個簡單的指令（directive）其實做了很多：建立 thread、設定狀態、把程式碼派給它們。
- [19:05] 流程：parent thread → spawn 一群 child thread → 各自算、寫進 partialSums → **thread join**（結束、回到 main thread）→ main thread 加總。

（本機跑了這個程式，見 [§17.1](#171-openmp影片的程式)。）

---

## 6. Message passing：MPI

[19:40] 另一種範式：**message passing**。不太用在多核心，而是用在**多處理器**系統：處理器之間用網路（例如 Ethernet）連，用一般網路的 TCP/IP 協定互送訊息。**記憶體不共用**，所有溝通都靠送收訊息。

[21:41] **MPI**（Message Passing Interface）的兩個基本函數（語法像 MPI 但不是精確的）：

| 函數 | 意思 |
|---|---|
| `send(pid, val)` | 把整數 val 送給 id 為 pid 的 process |
| `receive(pid)` | 從 pid 收一個整數；**blocking**（沒收到就等）；pid = ANYSOURCE 表示誰送來的都可以 |

[22:57] 程式：

```c
/* 啟動所有平行的 process */
int myId = getMyProcessId();        /* 0 到 N−1 */
int partialSum = 0;                 /* 沒有陣列：不共用記憶體 */
... 加自己那一份 ...
if (myId != 0) send(0, partialSum);          /* 不是 root：送給 root（process 0） */
else {
    int sum = partialSum;
    for (int i = 0; i < N - 1; i++)          /* 一定會收到 N−1 個訊息 */
        sum += receive(ANYSOURCE);
}
/* 收尾：關掉所有 process，回傳 sum */
```

（本機用真的 Open MPI 跑了，見 [§17.2](#172-mpi)。）

---

## 7. 兩種比較

[25:27]

| | Shared memory | Message passing |
|---|---|---|
| 速度 | 快 | **慢**（送訊息的負擔） |
| 擴充 | 大概到 200 個 core | **幾千、幾十萬個節點** |
| 好不好寫 | **好寫** | 難寫 |
| 可攜性 | 好（thread 由 scheduler 排到 core 上） | MPI 是標準，介面把 IP 位址抽象掉了，也算好；寫死 IP 就完全不可攜 |
| 適合 | **多核心**（本章的重點） | 資料中心等級的超級電腦 |

---

## 8. Amdahl's law

[28:16] 處理器變多，能快多少？受**循序部分的比例** f_seq 限制：

```
循序執行的時間 = f_seq（循序部分）＋ (1 − f_seq)（平行部分）     （單位：1）
P 個處理器：平行部分理想地快 P 倍（線性加速；一般其實做不到，但這個陣列的例子做得到）
T_par = f_seq ＋ (1 − f_seq)/P
speedup = 1 / (f_seq ＋ (1 − f_seq)/P)
P → ∞：speedup → 1 / f_seq
```

[31:31] 重點：**speedup 會飽和**。不管加多少 core，到某一點之後就不會更快。

[32:08] 老師的圖（x 軸 1–200 個處理器）：

| f_seq | 上限 1/f_seq | 情況 |
|---|---|---|
| 10% | 10 | 很早就飽和 |
| 5% | 20 | 約 25 個處理器開始飽和，大約 42、43 個以後沒用 |
| 2% | 50 | 150、200 個處理器才慢慢飽和；150 個處理器時約 37–38 倍 |

[33:58] 結論：f_seq 決定最終的 speedup。f_seq 很高就不值得投資很多處理器。shared memory 和 message passing 都適用；在一萬個 core 的超級電腦上跑的問題，**循序部分必須非常小**。

【課本補充】課本 p.421 §9.1.3。

---

## 9. Gustafson-Barsis's law

[34:33] 修正 Amdahl 的一個關鍵缺點：Amdahl 假設**問題大小不變**。但有一萬個處理器時，不會還在解同一個問題：筆電上寫作業，超級電腦上預測氣候變遷。

[35:54] 平行部分隨 P **放大**：

```
新的工作量 W_new = f_seq·W ＋ (1 − f_seq)·W·P
一個處理器：T_seq = α·W_new
P 個處理器：平行部分 ÷ P → T_par = α(f_seq·W ＋ (1 − f_seq)·W) = α·W
speedup = T_seq / T_par = f_seq ＋ (1 − f_seq)·P
```

[39:18] P 很大時，speedup **跟 P 成正比**，f_seq 不再是上限，只決定**斜率**（1 − f_seq）：處理器加倍能多得多少。「某種意義上是 Amdahl's law ++」。

【課本補充】課本 p.422：例如問題是在網格上模擬，處理器越多，就用越細的網格，整體的計算量跟著增加。

---

## 10. Flynn 分類

[41:18] 多處理器的整個設計空間（**Flynn's classification**）：

| 類別 | 指令流 | 資料流 | 例子 |
|---|---|---|---|
| **SISD** | 1 | 1 | 一般的單核心處理器 |
| **SIMD** | 1 | 多 | 以前的 vector processor；現在 x86 的 MMX、SSE、AVX，ARM 的向量指令 |
| **MISD** | 多 | 1 | **飛機** |
| **MIMD** | 多 | 多 | 多核心（本章） |

[42:39] **SIMD**：4 個 32-bit 整數 x、y、z、u 包進一個 128-bit 的 **vector register**，另外 4 個 x′、y′、z′、u′ 包進另一個；一條指令做兩兩相加 → (x＋x′, y＋y′, z＋z′, u＋u′)。大量用在數值、圖形、遊戲裡最關鍵的計算。

[43:56] **MISD**：很少見，但很重要的例子是**飛機**：

- 現代飛機大多由電腦控制（自動駕駛、緊急反應）。電腦會出錯：線會斷、程式有 bug。
- 用**三個不同廠商**的處理器（例如 Intel、MIPS、IBM），同一組感測器資料給三個，各自算出結果（例如「左轉 70°」）。
- **投票電路**用**多數決**給最後的結果：三個裡有兩個一樣就選它（假設不會同時兩個壞掉）。
- 「在飛機上沒有出錯的空間。」

【課本補充】課本 p.423：Flynn 在 **1966** 年提出這個分類。Basic 課程在本機試過 SIMD（AVX）：[Basic 第 41 支](../basic/lecture41_note.md)。

---

## 11. SPMD 與 MPMD

[48:27] MIMD 有兩種：

| | 說明 |
|---|---|
| **SPMD**（single program, multiple data） | **最常見**：程式寫一次（像 OpenMP），每個 core 跑同一個程式、處理不同的資料；用 get thread id 之類的知道自己是誰、拿哪一份資料 |
| **MPMD**（multiple program, multiple data） | 比較少見：不同種類的處理器（core 和加速器）各跑不同的程式。例：**CUDA** 程式最後是兩個程式，一個給 CPU、一個給 GPU（[第 19 支](lecture19_note.md)）；IBM 的 **Cell** 處理器：一個大的 master core ＋ 很多小的 slave core |

---

## 12. Hardware thread

[50:49] core 越來越大：issue width、fetch width、commit width 都很大。但程式的 **ILP 很低**時，一個 6-issue 或 8-issue 的處理器大部分的 issue slot 都**浪費**掉了。

[51:29] 為什麼不讓**好幾個 process 同時**跑在同一個 core 上？

- ⚠️ 這些**不是 software thread**（不一定共用位址空間），而是**彼此可以無關的 process**，在同一段時間裡共用同一個 core。叫 **hardware thread**。
- [53:31] 【口頭】「今天（2020 年 12 月 22 日），很少有伺服器處理器不在某種程度上支援 hardware thread。」

[54:07] 為什麼程式的 IPC 低？大多是因為**記憶體 miss**（cache 的 locality 差），有時是相依的結構像一條蛇，很多相依。

---

## 13. Coarse-grained 與 fine-grained multithreading

[54:39] **Coarse-grained**：

```
thread 1 跑 k 個 cycle（發了幾個記憶體請求，資料還沒回來，做不了多少）
→ thread 2 跑 k 個 cycle → thread 3 → thread 4 → 回到 thread 1（資料多半回來了）
```

[58:10] 遇到**很長的延遲**（例如 L2 miss）也可以**馬上**換，不用等滿 k 個 cycle。適合幾百個 cycle 的事件。

[58:44] **Fine-grained**：k 縮到 **1–5 個 cycle**，很快地在 thread 之間切換，像很快的 context switch。連 **L1 miss** 這種事件也能處理（OOO 處理器裡 L1 miss 不嚴重，但如果碰到 ILP 很低的階段，就換另一個 thread）。

---

## 14. Pipeline 要改哪些

[55:13]

| 結構 | 做法 |
|---|---|
| PC | **每個 hardware thread 一個** |
| ROB | 最好**分開**（共用的話，要讓 thread 2 的指令可以比 thread 1 先 retire）；checkpoint 狀態、retirement register file 也分開 |
| Branch predictor | **分開**，不然會互相污染 |
| Decode | 可以**共用**（沒有狀態） |
| Rename table | 可以共用一個，但架構暫存器 ID = thread ID ＋ 架構暫存器 ID |
| 實體暫存器 | 兩種：**各自的 register file**（用 thread ID 找）或**共用**（一個 free list，rename table 保證不會產生假的相依） |
| Instruction window | instruction packet 帶 thread ID；rename 讓 tag 不會讓 thread 2 叫醒 thread 1 的值 |
| Select | 要決定從哪個 thread 選（也可以 fine-grained 地輪流） |
| 功能單元 | 有些有狀態（例如 x86 的浮點單元、flags 暫存器），每個 thread 要分開管理 |
| LSQ | 存實體位址就不用改（甚至可以跨 thread 轉送）；存虛擬位址就要加 thread ID（[第 25 支](lecture25_note.md#9-位址還沒算出來用-xor-估)） |
| Retire | 分開的 RRF 等 |

[63:03] 每一個結構都要在「**共用**」和「**分開**」之間選擇，各有好處。

---

## 15. SMT 與 hyperthreading

[63:44] 兩個極端：

| | 做法 |
|---|---|
| **SMT**（simultaneous multithreading） | 最有彈性：例如 6 個 issue slot、4 個 thread，**每個 cycle 動態地分**：這個 cycle 從 thread 1 拿 2 條、thread 3 拿 2 條、另外兩個各 1 條 |
| **Hyperthreading**（Intel） | 另一個極端：**靜態**地把所有資源**切成兩半**（instruction window、功能單元、register file、issue slot……）→ 兩個**虛擬 core**，各跑一個 hardware thread |

[65:27] 在大的 OOO 處理器裡，就算用 coarse 或 fine-grained，因為從 fetch 到 commit 隔很久，pipeline 裡本來就會同時有好幾個 thread 的指令 → 自然走向 SMT。只有短 pipeline 的 in-order 處理器，每 k 個 cycle flush 一次，才適合把整個處理器給一個 thread。

[66:36] SMT 要考慮所有 thread，有公平的條件，再加上其他條件，聰明地分配 issue slot（和其他所有時間槽）：

- 公平地排程。
- 重要的指令（例如 load，能讓之後的 ILP 提高）先排。
- 事先的 thread 優先順序（例如處理影片的即時 thread 優先）。
- 學到的指令樣式（例如某些 add 很關鍵，因為它們算記憶體位址）。

[68:24] **Issue slot 的分配**是 SMT 處理器最重要的部分。

[69:02] Hyperthreading：彈性比較小，但工作負載**均質**時很有用，能提高吞吐量。大部分 Intel 機器都預設有；IBM POWER8、POWER9 這種大的伺服器處理器用 SMT。

[70:15] 圖（4 個 thread，每 cycle 最多 issue 好幾條）：

| 方式 | issue slot 的使用 |
|---|---|
| Coarse-grained | 每 2 個 cycle 換一個 thread；很多空著的 slot |
| Fine-grained | 每個 cycle 換；還是有很多空的 |
| **SMT** | 聰明地把各 thread 的指令**塞滿**；指令數一樣，只是排得更緊，幾乎沒有浪費 |

[72:51] SMT 為什麼沒有到處都是？負擔：執行時要有能做到這件事的排程器，還有指令的重要性、各種優先順序機制。「這曾經是很熱門的研究領域」，很多問題已經解決得差不多了。

【課本補充】課本 p.424 §9.1.6。

（本機量了 hyperthreading 什麼時候有用，見 [§17.3](#173-hardware-thread什麼時候有用)。）

---

## 16. 小結

[74:35]

- **OpenMP**：大部分機器上寫平行程式的首選；也有比較低階的 **pthreads**（更有彈性，但 OpenMP 比較流行）。兩者都是 C/C++ 的擴充。message passing 用 **MPI**。
- **Flynn 分類**：SISD、SIMD、MISD、MIMD。
- **Amdahl、Gustafson**：speedup 的界限。
- **Hardware thread**：不是 software thread，而是好幾個程式同時使用一個「過度設計」的 core 的資源：讓一個 thread 的低 ILP 階段跟另一個的高 ILP 階段重疊。對伺服器的大量吞吐量最有用，所以 SMT 是主要技術。

[76:34] 一台現代的伺服器：很大的 cache、**16–128 個 core**（強弱不一、種類不一）、每個 core 跑好幾個 hardware thread（4 個 hardware thread 就像 4 個虛擬 core）、一大片 cache bank、一個 on-chip network……「處理器的封裝裡是一個非常複雜的環境」。

[77:09] 在這種環境裡能**正確地**寫平行程式嗎？「除非懂理論基礎，否則不行。」下一講：平行程式和平行系統的理論基礎。

---

## 17. 【實測】

程式在 `experiments/lecture32/`。本機有 6 個 P-core（cpu0–11，每個核心 2 個 hardware thread）和 8 個 E-core（cpu12–19），共 20 個 hardware thread。

### 17.1 OpenMP：影片的程式

`sum_omp.c`：照 [§5](#5-shared-memoryopenmp) 的程式，2²⁴ 個 `double`，每個元素做 50 次乘加（讓它是**計算**而不是搬記憶體）。`OMP_PROC_BIND=true OMP_PLACES=cores`（每個 thread 綁一個核心）：

| thread 數 | 時間 | speedup |
|---|---|---|
| 1 | 0.504 s | 1.0 |
| 2 | 0.252 | 2.0 |
| 4 | 0.130 | 3.9 |
| 6 | 0.090 | **5.6** |
| **8** | **0.121** | **4.2**（比 6 個慢！） |
| 12 | 0.081 | 6.2 |
| 14 | 0.069 | 7.3 |
| 16 | 0.061 | 8.3 |
| 20 | 0.052 | 9.7 |

- 6 個 thread 以內（都在 P-core 上），幾乎是**線性**的加速。
- ⚠️ **8 個 thread 反而比 6 個慢**：程式把陣列**平均**切成 N 份，第 7、8 個 thread 跑在比較慢的 **E-core** 上。大家都要等最慢的那個做完（thread join），所以 E-core 拖慢了全部。這是 Amdahl's law 沒考慮的情況：處理器**不一樣快**時，平均切工作會造成負載不平衡。影片的程式是在同質的 core 上設計的。
- 20 個 thread（6 個 P-core × 2 ＋ 8 個 E-core）才到 9.7 倍。
- thread 數不同時，sum 的最後幾位數會變（7.907678 → 7.907672）：浮點數加法的順序不同，捨入就不同。

### 17.2 MPI

`sum_mpi.c`：照 [§6](#6-message-passingmpi) 的程式，用真的 **Open MPI 4.1.2**（`MPI_Send`、`MPI_Recv(MPI_ANY_SOURCE)`），每個 process 自己算自己那一份，不共用陣列：

```
 1 processes：0.556 s（sum = 7.907678e+08）
 2 processes：0.288 s（sum = 7.907678e+08）
 4 processes：0.146 s（sum = 7.907678e+08）
 6 processes：0.118 s（sum = 7.907678e+08）
```

結果和 OpenMP 版一樣，加速也差不多。這個問題每個 process 只送**一個**數字，訊息的負擔幾乎看不到。

`pingpong_mpi.c`：兩個 MPI process 互傳一個 `double`，量單程（跑兩次）：

```
MPI 單程：214 ns
MPI 單程：215 ns
```

同一台機器上，Open MPI 透過共用記憶體傳訊息，單程約 **215 ns**；而 [第 27 支 §17.3](lecture27_note.md#173-本機晶片上的網路核心之間換手多久) 量到直接用 shared memory 換手一條 cache line 只要 **32–48 ns**。message passing 慢 5–6 倍：就是 [§7](#7-兩種比較) 說的「送訊息的負擔」。跨機器走網路的話會慢得多。

### 17.3 Hardware thread：什麼時候有用

`smt.c`：兩種工作，各跑 2 億圈：

- **low ILP**：一條**相依**的 `imul` 鏈（每條要等上一條，issue slot 大部分閒著）。
- **high ILP**：8 條**互不相干**的 `add` 鏈（一個 thread 就能把 ALU 塞滿）。

比 1 個 thread、2 個 thread 在**同一個 P-core** 的兩個 hardware thread（cpu2、cpu3）、2 個 thread 在**不同的 P-core**（cpu2、cpu4）。總吞吐量 = 2 × (1 個 thread 的時間) ÷ (2 個 thread 的時間)。跑兩次：

| 工作 | 1 個 thread | 同一個核心（SMT） | 不同核心 |
|---|---|---|---|
| low ILP（相依的 imul） | 0.13 s | 0.13 s → **2.00–2.02 倍** | 0.13 s → 2.00–2.03 倍 |
| high ILP（8 條獨立的 add） | 0.09 s | 0.17 s → **1.02–1.03 倍** | 0.09 s → 1.99–2.00 倍 |

- **low ILP**：兩個 thread 擠在**同一個核心**上，各自還是 0.13 s：**總吞吐量變兩倍**，跟用兩個核心一樣。一個 thread 的相依鏈讓 issue slot 閒著，另一個 thread 剛好填進去。這就是 [§12](#12-hardware-thread) 的想法：讓一個 thread 的低 ILP 跟另一個重疊。
- **high ILP**：一個 thread 已經把 ALU 塞滿（約每 cycle 3.8 個 add，[第 11 支](lecture11_note.md#163-真的-cpu-有幾個功能單元) 量過約 3.5 個），第二個 hardware thread 沒有空位可用，兩個 thread 要 0.17 s：**總吞吐量幾乎沒增加**（1.02 倍）。只有真的第二個核心才有用。
- 所以 hyperthreading 的好處完全看工作負載：「core 有閒著的資源」才有用。

---

## 18. 一頁速查表

| 主題 | 重點 |
|---|---|
| 動機 | 單核心效能約 2008 年後飽和（design rule 越來越嚴） |
| Process／thread | thread 共用 heap、程式碼；自己的 stack、暫存器；Linux 的 thread group |
| Map／reduce | 切開各算部分和 → 加總 |
| OpenMP | `#pragma omp parallel`、`omp_get_thread_num()`；平行段 → 循序段 |
| MPI | `send`、`receive`（blocking、ANYSOURCE）；不共用記憶體 |
| 比較 | shared memory 快、好寫、約 200 core；message passing 慢、難寫、幾十萬節點 |
| Amdahl | 1 / (f_seq ＋ (1 − f_seq)/P) → 上限 1/f_seq |
| Gustafson | f_seq ＋ (1 − f_seq)·P → 跟 P 成正比 |
| Flynn | SISD、SIMD（vector、SSE/AVX）、MISD（飛機的三個處理器投票）、MIMD |
| SPMD／MPMD | 同一個程式（最常見）／不同的程式（CUDA、Cell） |
| Hardware thread | 不同 process 共用一個 core，填滿 issue slot；伺服器幾乎都有 |
| Coarse／fine | 每 k 個 cycle（或 L2 miss 時）換／每 1–5 個 cycle 換 |
| 要改的 | 分開：PC、ROB、branch predictor；共用：decode；可選：實體暫存器；加 thread ID |
| SMT | 每個 cycle 動態分 issue slot；最有彈性；POWER8/9 |
| Hyperthreading | 靜態切兩半 → 兩個虛擬 core；Intel |
| 實測 | OpenMP 6 個 P-core 5.6 倍、加 2 個 E-core 反而變慢；MPI 單程 215 ns（shared memory 32–48 ns）；SMT：低 ILP 2.0 倍、高 ILP 1.02 倍 |

---

## 19. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| 理論基礎：sequential consistency、PLSC | [第 33 支](lecture33_note.md)、[第 34 支](lecture34_note.md) |
| Execution witness | [第 35 支](lecture35_note.md) |
| Coherence 協定 | [第 36 支](lecture36_note.md)、[第 37 支](lecture37_note.md) |
| Memory model、data race | [第 38 支](lecture38_note.md)、[第 39 支](lecture39_note.md) |
| Transactional memory | [第 40 支](lecture40_note.md) |
| 核心之間換手的時間 | [第 27 支](lecture27_note.md) |
| CUDA（MPMD） | [第 19 支](lecture19_note.md) |
| ROB、RRF、rename table | [第 10 支](lecture10_note.md)、[第 13 支](lecture13_note.md) |
| SIMD（大學部，含本機實測） | [Basic 第 41 支](../basic/lecture41_note.md) |

---

## 附：本講一句話總結

> **單核心效能在 2008 年後飽和，所以把大問題切成小問題交給多個 core：thread 是共用 heap 和程式碼、只有自己 stack 和暫存器的輕量 process，平行程式可以用 shared memory（OpenMP：`#pragma omp parallel` 加 `omp_get_thread_num()`，好寫但約 200 core 就到頭）或 message passing（MPI：send/receive，難寫但能擴充到幾十萬個節點），典型的結構是平行的 map 加循序的 reduce；Amdahl's law 說 speedup 上限是 1/f_seq，Gustafson-Barsis's law 說問題跟著變大時 speedup 跟 P 成正比；Flynn 把多處理器分成 SISD、SIMD（向量指令）、MISD（飛機的三台電腦投票）、MIMD（SPMD 最常見，MPMD 像 CUDA）；而 hardware thread 讓好幾個 process 共用一個很寬的 core，用 coarse-grained、fine-grained、最有彈性的 SMT 或靜態切兩半的 hyperthreading 填滿閒著的 issue slot。本機實測：影片的 OpenMP 程式在 6 個 P-core 上快 5.6 倍，但加上 2 個 E-core 反而變慢（平均切工作，大家都等最慢的那個）；真的 Open MPI 單程傳一個數要 215 ns，比 shared memory 換手一條 cache line 慢 5–6 倍；兩個 thread 擠在同一個核心上，相依的 imul 鏈總吞吐量翻倍，已經塞滿 ALU 的 add 鏈卻只多 2%。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 9: Multicore Systems (Part I) -- Parallel Programming, Hardware Threads 的 YouTube 自動字幕（1:18:12）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch9 §9.1（p.415–425）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、OpenMP、Open MPI 4.1.2）結果。*
