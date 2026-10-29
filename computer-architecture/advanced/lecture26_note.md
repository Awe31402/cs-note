# Advanced Computer Architecture 第二十六支影片完整筆記：Trace cache、instruction prefetching、data prefetching 與 runahead execution（Chapter 7, Part VI/VI）

> **課程**：Advanced Computer Architecture — Chapter 7: Caches (Part VI/VI) -- Trace caches, instruction and data prefetching（播放清單第 26 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（1:10:49）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch7** §7.5–7.8（p.313–331）
> **章號**：影片 Chapter 7 = 課本 Ch7。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10、valgrind 3.18）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕有不少錯字：「PM 4」= Pentium 4、「iash／iach」= i-cache、「pel and wiser」= Peleg and Weiser、「Melvin atal」= Melvin et al.、「chaomin／Chine」= chow mein（炒麵）、「marov／markof」= Markov、「genome toolkit」= GNOME toolkit，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Basic block 與 trace](#1-basic-block-與-trace)
- [2. Trace cache 的想法](#2-trace-cache-的想法)
- [3. 兩個前身](#3-兩個前身)
- [4. Trace 的結構](#4-trace-的結構)
- [5. 存的規則](#5-存的規則)
- [6. Tag array 的欄位](#6-tag-array-的欄位)
- [7. 讀 trace 和建 trace](#7-讀-trace-和建-trace)
- [8. 為什麼要 prefetch](#8-為什麼要-prefetch)
- [9. Next line prefetching](#9-next-line-prefetching)
- [10. Markov prefetching](#10-markov-prefetching)
- [11. Call graph prefetching](#11-call-graph-prefetching)
- [12. Data prefetching：為什麼不一樣](#12-data-prefetching為什麼不一樣)
- [13. Stride 的 prefetcher](#13-stride-的-prefetcher)
- [14. 提早幾圈](#14-提早幾圈)
- [15. Linked list：pointer chasing](#15-linked-listpointer-chasing)
- [16. Runahead execution](#16-runahead-execution)
- [17. Helper thread](#17-helper-thread)
- [18. Chapter 7 總結](#18-chapter-7-總結)
- [19. 【實測】](#19-實測)
- [20. 一頁速查表](#20-一頁速查表)
- [21. 與後續講次的連結](#21-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
trace cache（Pentium 4，約 2000 年）：存「解碼好的一串 basic block」，跳過 branch predictor 和 decoder
  cache line = trace segment；trace = head → body… → tail，放在連續的 set，只存下一個 way
prefetching：猜之後要的位址，「剛好在用到之前」拿來；只看 miss 的序列
  instruction：next line → Markov（miss X 之後常 miss Y）→ call graph（以函數為單位）
  data：stride（PC → 上次位址、stride、狀態）；提早 k 圈；linked list 用 pointer chasing
  L2 miss 時：runahead（用垃圾值繼續跑，順便 prefetch、訓練 predictor）、helper thread
```

---

## 1. Basic block 與 trace

[00:00] 程式一個 **basic block** 接一個 basic block 地執行：

- **Basic block**：一串組合語言指令，**只有一個入口**（只能從開頭進來）和**一個出口**（只能從結尾離開）：沒有分支跳到中間，也沒有分支從中間跳出去。

[01:36] 從 i-cache 抓出來的一串 basic block 叫 **trace**：等於一段**展開好**的執行過程。

---

## 2. Trace cache 的想法

[02:18] 存 trace，而且存**解碼好**的指令 → 把（照之前的預測，最可能的）trace 直接送進 **rename**。

| | i-cache | trace cache |
|---|---|---|
| 存什麼 | 最常用的**位址** | 最可能的**執行順序** |
| 要不要預測分支、解碼 | 要 | **不用** |

[02:55] Intel 第一次用在 **Pentium 4**（約 2000 年），大致上很準。

[03:28] 【口頭】比喻：一盒**炒麵**，每一根麵條是一個 trace，拿起一根「吸」出來，就是把一整個 trace 讀出來送進 pipeline。

【課本補充】課本 p.314：課本照 Intel 的專利（**Krick et al. 2000**）介紹，並做了簡化。

---

## 3. 兩個前身

[04:08] Trace cache 大致是兩個舊想法的結合：

| 前身 | 做法 | 限制 |
|---|---|---|
| **Peleg and Weiser** | 一條 cache line（例如 128 byte）裡放**連續執行的 basic block**：一個 basic block，加上最可能接著執行的那一個 | 一個 trace segment 不能跨好幾條 cache line |
| **Melvin et al.** | CISC 指令會解碼成 micro-op（[第 9 支](lecture09_note.md)）；cache line 裡存**解碼好的 micro-op** | 是 [第 9 支](lecture09_note.md) predecode 的進一步發展 |

[05:34] 能不能結合、擴充？可以 → trace cache。

---

## 4. Trace 的結構

[06:09] 一個長的 trace 跨好幾條 cache line：像一個由 cache line 組成的 **linked list**，每條 line 是一個 **trace segment**。

[06:48] 一個 trace = **trace head** → 幾個 **trace body** → **trace tail**。不改 i-cache 的組織方式，在現有的結構上建 linked list。

[07:28] 硬體：data array、tag array、一個「聰明的」**controller**（實作 trace cache 的邏輯）、一個 **fill buffer**（在本地組出一個 trace segment，組好了才搬進 data array，tag array 也建一格）。

[08:40] 存法（以 4-way 為例，每一列是一個 set）：

```
set i    ：[ ] [head] [ ] [ ]
set i+1  ：[body] [ ] [ ] [ ]
set i+2  ：[ ] [ ] [body] [ ]
set i+3  ：[ ] [tail] [ ] [ ]
```

[10:00] 重要的樣式：

- trace segment 放在**連續的 set**。
- 所以下一個 segment 一定在 set i+1，**只要存下一個 set 裡的 way 編號**。

【課本補充】課本 p.316：如果在 block 裡存完整的指標，64 byte 的 block 存一個 8 byte 的指標，負擔是 12.5%，太大了；所以限制成放在連續的 set、只存 way 編號。

---

## 5. 存的規則

[10:37] 每條 data line 最多存 **6 個** micro-op：解碼好的 micro-op 要佔空間，而且 rename 的頻寬有限。

[11:15] 規則：

1. 一條指令的 micro-op **不分散**到不同的 cache line。
2. branch micro-op 超過門檻就結束這條 line。
3. 遇到下面的情況，**結束建 trace**：
   - **indirect branch**（目標在暫存器裡）：目標很難猜，而猜錯方向或目標都會結束 trace。
   - branch 猜錯，或來了 interrupt。
   - trace 長度到上限：**64 個 set**。branch prediction 本來就有準確度的限制，trace 也不該無限長。

---

## 6. Tag array 的欄位

[13:02]

| 欄位 | 意思 |
|---|---|
| tag | 一般的 tag |
| valid | |
| **type** | head、body 或 tail |
| **next way**、**prev way** | 下一個／上一個 set 裡的 way 編號（雙向 linked list） |
| **NLIP** | 下一條 **CISC** 指令的位址（例如 load effective address 這種指令需要位址資訊） |
| **micro IP** | 指到 **microcode 表**的索引：有些複雜的指令不解碼，要讀 microcode 表 |

[14:57] 【口頭】「鼓勵大家去讀 Intel 原始的 trace cache 專利，所有細節都在裡面。」

---

## 7. 讀 trace 和建 trace

### 7.1 讀

[15:36] 狀態機：

```
head lookup：找 trace head → 找到 → body lookup：一直讀 body
  body 找不到（被趕出去了）→ body miss：從這裡開始一個新的 trace
  遇到複雜指令 → 讀 microcode 表 → 回到 body lookup
  讀到 tail → 回到 head lookup，找下一個 trace
```

### 7.2 建

[17:23] trace cache 旁邊還有一般的 i-cache（接到記憶體系統），優先用 trace cache（比較快）。

```
從 i-cache 抓指令 → 等 micro-op 解碼出來 → 放進 fill buffer
  → 碰到 trace segment 結束的條件（例如這條 line 滿了）→ 搬進 trace cache；trace 還沒結束就繼續
  → 碰到複雜的 CISC 指令：放一個指到 microcode 表的間接指標，或讀出 micro-op 組成 segment
```

[19:12] 【口頭】Intel 的專利沒有公開很多細節；照計算機結構的常識，如果 CISC 指令只轉成幾個 micro-op，就直接放進 fill buffer；否則放間接指標。

[21:00] 為什麼要 trace cache？i-cache 給的吞吐量和速度不夠：i-cache「是很笨的資料組織方式」，trace cache 用更聰明的方式組織。

⚠️ 本機的 CPU 沒有辦法看 trace cache：Pentium 4 之後 Intel 改用別的結構（例如 decoded µop cache），本機沒有 perf counter 可以看它（perf 被擋），**待補**。

---

## 8. 為什麼要 prefetch

[21:39] 比 trace cache 流行得多的是 **prefetching**：猜之後會存取的位址（指令或資料），提早去拿。

[22:13] miss 的代價：

| miss | 後果 |
|---|---|
| **i-cache miss** | 很嚴重：pipeline 的前半段是 **in-order** 的，藏不住 |
| L1 miss | 去 L2：10–50 cycle |
| L2 miss | 去主記憶體：幾百 cycle 的浪費，IPC 受苦 |

[22:48] 時機：prefetch 的資料要**剛好在用到之前**到：

| 太早 | 把有用的資料趕出 cache → miss 變多 |
|---|---|
| **太晚** | 還是要等 |

[23:22] 做法：找出存取序列的**樣式**，用它來 prefetch。

---

## 9. Next line prefetching

[24:00] 最基本、「出乎意料地很好用」的 instruction prefetching。樣式：**spatial locality**：存取 line X，很可能接著存取 X+1、X+2、X+3（函數的程式碼通常跨好幾條 line）。

[25:14] 做法：line X **miss** 時，從 L2 讀 X 和接下來的幾條。

[25:51] 重要：prefetcher **只看 miss 的序列**，不看存取的序列。看存取序列的話，prefetcher 幾乎一直在運作，很耗電。

[26:32] 變化：miss X 時 prefetch X+k（X+1 可能太晚）、一次抓好幾條……要用模擬找出最好的；很多 prefetcher 會**動態調整** k。k 太大會抓很多沒用的資料。

[27:09] 限制：只看下一條，「觸及的範圍有限，不太聰明」。

（本機模擬了 next line 和 Markov，見 [§19.2](#192-next-line-對上-markov)。）

---

## 10. Markov prefetching

[27:46] i-cache 的 miss 序列**不一定是連續的 line**，但**重複性很高**：

```
miss X → miss Y → … → miss X → miss Y → …
（X 和 Y 不一定相鄰：例如 X 是一個函數，它呼叫了遠處的另一個函數 Y）
```

[28:21] 看到 X，就可以馬上開始 prefetch Y：記住這些關聯 → 像機率裡的 **Markov chain**。

[29:35] **Markov 表**：

| miss 的 line | 下一個 miss：次數 |
|---|---|
| X | Y：3、Z：1、… |

miss X 時，查表，prefetch **次數最多的**（Y）；也可以 Y、Z 都抓。計數器會飽和，要定期減少（之前看過很多次的樣式）。

[30:46] 進階：**n 階**的歷史：「miss X、Y、Z 之後，下一個是 U」：選讓條件機率 P(U | X, Y, Z) 最大的。

[31:25] 每次 miss：查 prefetch 表、找接下來要抓的位址、更新前 n−1 個 miss 的次數。要抓的請求放進 **prefetch queue**：優先權比真的 miss 低，慢慢送出去。

---

## 11. Call graph prefetching

[32:46] basic block 的順序大致可預測，但**函數呼叫的順序更可預測**。以函數為單位：預測下一個會呼叫的函數，prefetch 它所有的 basic block。

[33:23] 例子（受 GNOME toolkit 啟發的程式）：

```
show_screen()
  ├─ 1. prepare_window()
  │     ├─ 1. set_buttons()
  │     └─ 2. set_text_boxes()
  ├─ 2. update()
  └─ 3. clear()
```

call graph：每個節點是一個函數，每條邊是一次呼叫。

### 11.1 軟體

[34:35] compiler 插 prefetch 指令：進入 `show_screen` 時 prefetch 第 1 個函數；第 1 個回來時 prefetch 第 2 個；依此類推。幾乎所有 ISA 都有 prefetch 指令；【口頭】「可以去看 Linux 上 GCC 的 intrinsics，有內建的 macro」（例如 `__builtin_prefetch`）。

[35:46] 為什麼 instruction miss 比 data miss 更貴：data miss 時，OOO 還有別的指令可以做；指令卡住的話，它們在 pipeline 的 in-order 部分，fetch 直接停住，影響整條 pipeline。

### 11.2 硬體：CGHC

[36:22] **Call graph history cache（CGHC）**：

| | 內容 |
|---|---|
| tag | **function id**（例如函數第一條指令的位址） |
| data | 這個函數**會呼叫**的函數的位址（照順序） |
| index | 指到「下一個要被呼叫」的那一個；一開始是 1 |

[38:22] 運作：

| 事件 | 做什麼 |
|---|---|
| X 呼叫 Y | 查 Y 的 entry：沒有就建一個；有就 prefetch Y 的呼叫序列裡**第一個**函數。把 Y 放進 X 的呼叫序列的對應位置（需要的話） |
| Y 回到 X | Y 這一次的生命結束 → Y 的 index 重設成 1；X 的 index +1 → prefetch X 的下一個函數（例如 Z） |

【課本補充】課本 p.321：call graph prefetching 和 CGHC 是 **Annavaram et al. 2003** 提出的。

[39:44] 三種的關鍵：

| 方法 | 洞見 |
|---|---|
| Next line | spatial locality |
| Markov | miss 序列重複性很高，但不是連續的 line；記錄（n 個之前 → m 個之後）的次數 |
| Call graph | 以函數為單位，一次抓整個函數 |

[41:39] 最近的研究再往上一層：看比 trace 更大的單位，一次 prefetch 一群函數、一整個 page 或一串 page。

【課本補充】課本 p.324 引用 Ferdman et al. 2011、Kolli et al. 2013、Kallurkar and Sarangi 2017 這方面的研究。

---

## 12. Data prefetching：為什麼不一樣

[42:17] 指令 miss 時，後面的指令都抓不了，很敏感；資料 miss 時，OOO 比較**有彈性**：一個 load miss，可能還有足夠多跟它無關的指令可以做。

[44:04] 命中率（老師的數字）：

| Cache | 命中率 |
|---|---|
| I-cache | 90–95%，甚至 96–98% |
| L1 D-cache | 80–90%（local） |
| L2 | 50–70%（local） |

[44:45] I-cache 的改進「像低利潤的生意」，data 是「高利潤的生意」：改進空間大很多，但敏感度比較低：同樣的改進，在 i-cache 得到的效能比較多。幾乎所有處理器都有 instruction prefetcher 和 data prefetcher。

---

## 13. Stride 的 prefetcher

[45:19] 例子：

```c
for (i = 0; i < N; i++)
    c[2 * i] = a[i] + b[i];
```

- a、b 每圈位址差 **4 byte**（int 是 4 byte）；c 差 **8 byte**。
- 硬體看到的是：同一個 PC，位址每次差 4（或 8）。這個差叫 **stride**。

[46:34] **Stride prediction table**（也叫 reference prediction table）：

| 欄位 | |
|---|---|
| PC → 指令的 tag | |
| **previous address** | |
| **stride** | |
| **state**（飽和計數器） | 00：不是 stride 樣式；漸漸變成 10、11 → 是 |

- [47:10] 每次 miss：現在的位址 − previous address，等不等於 stride？不等就更新 stride；連續兩三次一樣，state 往上加。
- [48:30] state 穩定時，下次抓到這個 PC，就 prefetch **previous address ＋ stride**（state 只決定要不要加，不是加進去）。

[49:02] 這個基本的硬體就能處理大部分的 stride 存取樣式。（跟 [第 14 支 §4](lecture14_note.md#4-位址預測stride) 的 stride 位址預測是同一個結構。）

（本機量了 hardware prefetcher 抓得到哪些 stride，見 [§19.1](#191-本機的-data-prefetcher-抓得到哪些-stride)。）

---

## 14. 提早幾圈

[49:42] 不要只 prefetch X + s，可以 prefetch **X + k·s**：提早 k 圈。

[49:42] 怎麼決定 k：監控「prefetch 的資料**到達**」和「真的**用到**」之間差幾個 cycle：

| 差值 | 意思 |
|---|---|
| 負的 | 太晚到 |
| 很大的正數 | 太早到，可能把有用的資料趕出去 |
| **小的正數** | 剛好 |

[50:52] 例：迴圈的下一圈很快就到了，資料還很久才來 → 應該 prefetch 好幾圈之後的位址。唯一的旋鈕就是「提早幾圈」，依監控的結果調整。

---

## 15. Linked list：pointer chasing

[52:05] 從陣列進到 linked list：

```c
for (temp = start; temp != NULL; temp = temp->next) {
    prefetch(temp->next);      /* 軟體：compiler 幫忙插的 prefetch */
    process(temp);
}
```

[52:40] **Pointer chasing**：追著 linked list 的指標 prefetch 下一個節點。

[53:17] 想 prefetch `temp->next->next`？要先拿到 `temp->next` 的資料才知道，**沒有捷徑**，所以不實際。除非有另外一個只有指標、沒有資料的索引結構。

[53:54] linked list 不能用 stride：節點散落在記憶體各處，沒有固定的 stride。但加一點 compiler 的幫忙（插 prefetch 指令）就能讓資料提早來。

【課本補充】Basic 課程在本機試過 software prefetch：[Basic 第 36 支 §16.1](../basic/lecture36_note.md#161-software-prefetch-真的有用嗎)。

---

## 16. Runahead execution

[54:29] L2 miss → 去主記憶體，200–400 cycle → 整條 pipeline 停住、ROB 滿了。能做什麼？

[55:09] **Runahead mode**（不算很新的想法，但很不一樣）：

1. L2 miss 了，不等：讓 load 回傳一個**垃圾值**，繼續執行。
2. 像 [第 15 支](lecture15_note.md) 的 poison bit：用到垃圾值的指令（整個 forward slice）也產生垃圾值，加一個 **invalid bit**。
3. 不在 forward slice 裡的指令照常、**正確地**執行：
   - **訓練 predictor**。
   - 存取資料 → 把資料從下層拉上來 = **prefetch**。
4. 不更新架構狀態。
5. 真的資料回來 → flush 這個 L2 miss 的 forward slice（或大部分 pipeline），用 replay 重做，回到正常模式。

[58:13] 進入 runahead mode 之前，先對**架構 register file、branch history register、return address stack** 做 checkpoint，免得被弄亂。

[58:47] invalid 的值只留在 pipeline 裡嗎？store 存了垃圾值怎麼辦？

| 做法 | |
|---|---|
| 只限在 pipeline | 所有 invalid 的 store 都作廢，不寫任何架構狀態 |
| 可以到 L1，**絕不到 L2** | L2 的話狀態管理太複雜；回到正常模式時，用 **flash clear** 把所有 invalid 的 line 一次清掉 |

[59:57] 要深入思考兩者的取捨。

[60:34] invalid 的資料放進一般的 L1 會有問題：同一條 64 byte 的 line 可能 4 byte 有效、4 byte 無效。簡單的解法：另外一個 **runahead cache**，只放 invalid 的資料。

| 情況 | 做什麼 |
|---|---|
| store 的**位址** invalid | 忽略 |
| store 的**資料** invalid（位址是對的） | 寫進 runahead cache，**同時 prefetch** 那個 block 到 L1；line 裡可以標記哪些部分 invalid |
| load | 先試 LSQ 轉送；不行就**平行**查 runahead cache 和 L1（runahead cache 優先）；資料 invalid 就把 load 標記 invalid；L1 miss 就從 L2 拿上來（= prefetch） |

[63:37] L2 miss 的那個 load 之前的指令照常 retire；之後的全部在 runahead mode，不 retire、不 commit。回到正常模式（像分支猜錯一樣）時恢復 checkpoint，從那裡重新開始。**正確性沒有犧牲。**

[64:52] 關鍵洞見：L2 miss 之後的 300–400 cycle，pipeline 本來就會閒著，**拿來做有用的事**：資料可能是垃圾，但**位址**是對的 → 資料被搬到 cache 裡，回到正常模式後省很多時間。

【課本補充】課本 p.328：runahead execution 是 **Mutlu et al. 2003** 提出的。

---

## 17. Helper thread

[65:29] 另一組人提出的類似想法：**helper thread**（在多核心上）。

- 程式裡有些 load **常常 miss**。
- 那個 load 的 **backward slice**：計算它位址的那些指令。
- 把 backward slice（指令和資料）搬到**另一個 core** 上的一個小 thread，平行執行，算出位址就 prefetch。
- [66:35] 不影響正確性和效能（另外跑）。哪些 load 要這樣做：要 compiler 幫忙，或先跑一次 profiling，找出位址很難預測的 load。等於每個這種 load 都有一個「奈米程式」幫它算位址、prefetch。

[67:08] 【口頭】「我不知道有哪個處理器**直接**用 runahead 或 helper thread，但這是很有前途的想法，不同的版本影響了很多年來的設計決策。而且現在處理器的很多技巧**沒有公開**，它們很可能已經在裡面了，只是我們不知道。」

---

## 18. Chapter 7 總結

[67:47]

| 重點 | 在哪一講 |
|---|---|
| cache = tag array ＋ data array；fully、set、direct mapped；set associative 最常見 | [第 21 支](lecture21_note.md) |
| 用 Elmore delay model 估延遲和功耗 | [第 24 支](lecture24_note.md) |
| pipelining、non-blocking、way prediction、tiling…… | [第 25 支](lecture25_note.md) |
| trace cache：存解碼好的 trace，跳過 predictor 和 decoder | 本講 |
| instruction 和 data prefetcher；剛好在用到前到；instruction 很敏感、data 改進空間大；陣列容易、linked list 和 `a[b[i]]` 這種不規則的很難 | 本講 |

[70:10] 下一章（Ch8）：**network on chip（NoC）**。

---

## 19. 【實測】

程式在 `experiments/lecture26/`。

### 19.1 本機的 data prefetcher 抓得到哪些 stride

`pf.c`：在 1 GiB 的陣列上做一條 **pointer chase**：每一步的位址 = 上一步讀到的值，互相相依，OOO 沒辦法提早發 load；但位址剛好排成**固定的 stride**。prefetcher 認得出來的話，延遲會比 DRAM 小很多；認不出來就跟隨機一樣（每一步都是一次 DRAM miss）：

| stride | P-core（cpu2） | E-core（cpu14） |
|---|---|---|
| 64 B（下一條 line） | **3.65** ns | **6.82** ns |
| −64 B（往回） | 3.62 | 6.80 |
| 128 B | 6.40 | 16.86 |
| 256 B | 7.53 | 22.89 |
| 512 B | 9.39 | 27.34 |
| 1024 B | 9.40 | 24.82 |
| 2048 B | 14.77 | 26.66 |
| **4096 B** | **34.96** | **101.50** |
| 8192 B | 45.16 | 104.74 |
| 隨機（對照） | **105.11** | **118.96** |

- **stride 在一個 4 KB page 以內，prefetcher 都抓得到**：比隨機快 7–29 倍（P-core）。往前、往後都行（−64 B 一樣快）。這就是 [§13](#13-stride-的-prefetcher) 的 stride prefetcher。
- **stride 4096 B 以上**（每一步跨一個 page）：E-core 完全抓不到（101 ns，跟隨機一樣）；P-core 還剩一點效果（35–45 ns）。prefetcher 通常不跨 page（下一個 page 的實體位址不知道），P-core 那一點好處的原因這裡不確定（可能有跨 page 的 prefetcher，或 DRAM 的 row buffer 剛好命中），**待補**。
- stride 越大越慢：每條 line 只用一次，prefetcher 要提早更多才來得及（[§14](#14-提早幾圈)）。

### 19.2 Next line 對上 Markov

`ipf.py`：用 `work.c`（[第 14 支](lecture14_note.md)）的 valgrind trace 裡的**指令位址**，模擬一個故意做小的 I-cache（8 KB、64 B、4-way、LRU），prefetcher 只看 miss 的序列：

```
I-cache 存取（line 層級）1554740 次；沒有 prefetch：miss 57933 次（3.73%）
  next line（一次抓 1 條）：miss 46220 次（少了 20.2%）；發了 21868 個 prefetch，用到 14595 個（67%）
  next line（一次抓 2 條）：miss 57544 次（少了  0.7%）；發了 74975 個 prefetch，用到 29005 個（39%）
  next line（一次抓 4 條）：miss 51790 次（少了 10.6%）；發了 158320 個 prefetch，用到 51945 個（33%）
  Markov（一次抓 1 條）：miss 33958 次（少了 41.4%）；發了 23999 個 prefetch，用到 23976 個（100%）
  Markov（一次抓 2 條）：miss 24601 次（少了 57.5%）；發了 34876 個 prefetch，用到 34846 個（100%）
```

- **Next line**（抓 1 條）少了 20% 的 miss，2/3 的 prefetch 有用到。
- **抓更多條反而變差**（抓 2 條只少了 0.7%）：在這麼小的 cache 裡，抓進來的沒用的 line 把有用的趕出去了：就是 [22:48] 說的「太早／太多會趕走有用的資料」。
- **Markov** 少了 41–58% 的 miss，而且幾乎每一個 prefetch 都用到。`work.c` 是 qsort 的迴圈，miss 的序列重複性非常高，正是 [27:46] 說的情況。
- ⚠️ 模擬裡的 Markov 表**沒有大小限制**（記住所有的 miss 對），真的硬體表很小；而且這個程式特別規律，真實的效果會比這裡差。

---

## 20. 一頁速查表

| 主題 | 重點 |
|---|---|
| Basic block | 一個入口、一個出口 |
| Trace | 一串 basic block；trace cache 存解碼好的 trace，跳過 predictor、decoder；Pentium 4 |
| 前身 | Peleg & Weiser（line 裡放連續的 basic block）＋ Melvin et al.（存解碼好的 micro-op） |
| 結構 | head → body → tail；連續的 set，只存 next/prev way；fill buffer；每條 line 最多 6 個 micro-op |
| 結束 trace | indirect branch、猜錯、interrupt、64 個 set |
| Tag 欄位 | tag、valid、type、next/prev way、NLIP、micro IP（microcode 表） |
| Prefetch 時機 | 剛好在用到之前；太早趕走有用的，太晚沒用；只看 miss 序列 |
| Next line | miss X → 抓 X+1（或 X+k、好幾條） |
| Markov | 表：miss X 之後各個 miss 的次數；抓最多的；可以 n 階；prefetch queue 低優先 |
| Call graph | 以函數為單位；軟體插 prefetch；硬體 CGHC（function id → 呼叫的函數序列 ＋ index） |
| Data vs instruction | instruction 更敏感（in-order 前端）；data 命中率低、改進空間大 |
| Stride | PC → previous address、stride、飽和計數器；prefetch previous ＋ stride（或 ＋ k·stride） |
| Linked list | pointer chasing（軟體 prefetch next）；沒有 stride |
| Runahead | L2 miss → 用垃圾值繼續跑（invalid bit）→ prefetch ＋ 訓練 predictor；checkpoint；runahead cache；不寫 L2 |
| Helper thread | 另一個 core 跑難 load 的 backward slice，算位址、prefetch |
| 實測 | page 內的 stride 都被 prefetch（P-core 3.6–15 ns vs 隨機 105 ns），跨 page 就不行（E-core）；I-cache 模擬：next line −20%、Markov −41–58% |

---

## 21. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Fetch、predecode、micro-op、indirect branch | [第 7 支](lecture07_note.md)、[第 8 支](lecture08_note.md)、[第 9 支](lecture09_note.md) |
| Stride 位址預測、value prediction | [第 14 支](lecture14_note.md) |
| Poison bit、replay | [第 15 支](lecture15_note.md) |
| 回復用的 checkpoint | [第 13 支](lecture13_note.md) |
| Cache 的基本、本機的延遲 | [第 21 支](lecture21_note.md) |
| Software prefetch（大學部，含本機實測） | [Basic 第 36 支](../basic/lecture36_note.md) |
| NoC、L2 的延遲 | [第 27 支](lecture27_note.md)（Ch8） |
| 多核心、thread | [第 32 支](lecture32_note.md)（Ch9） |
| DRAM 的 row buffer | [第 43 支](lecture43_note.md)（Ch10） |

---

## 附：本講一句話總結

> **Trace cache（Pentium 4）結合 Peleg & Weiser 和 Melvin et al. 的想法，把解碼好的一串 basic block 存成由 cache line 串起來的 trace（head、body、tail 放在連續的 set，只記 next/prev way），讀出來直接送進 rename，跳過 branch predictor 和 decoder；更普遍的是 prefetching：只看 miss 序列、讓資料剛好在用到前到達，指令方面有 next line、記錄「miss X 之後常 miss Y」的 Markov、以函數為單位的 call graph prefetching（CGHC），資料方面有用 PC 記住上次位址和 stride 的 stride prefetcher（可以提早 k 圈）、linked list 的 pointer chasing，以及 L2 miss 時用垃圾值繼續跑、順便 prefetch 和訓練 predictor 的 runahead execution，和在別的 core 上算難 load 位址的 helper thread。本機實測：dependent 的 pointer chase 只要 stride 在一個 4 KB page 內，硬體 prefetcher 都抓得到（P-core 3.6–15 ns，隨機是 105 ns），跨 page 就抓不到（E-core 回到 101 ns）；在一個小 I-cache 的模擬裡，next line 少了 20% 的 miss、抓太多反而變差，而 Markov 少了 41–58%。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 7: Caches (Part VI/VI) -- Trace caches, instruction and data prefetching 的 YouTube 自動字幕（1:10:49）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch7 §7.5–7.8（p.313–331）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10、valgrind 3.18）結果。*
