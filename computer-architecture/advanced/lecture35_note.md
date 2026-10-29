# Advanced Computer Architecture 第三十五支影片完整筆記：Multicore（四）：execution witness、access graph、causal graph 與 thin air read（Chapter 9, Part IV）

> **課程**：Advanced Computer Architecture — Chapter 9: Multicore Systems (Part IV) -- Execution Witnesses（播放清單第 35 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（1:01:30）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch9** §9.3.6–9.3.9（p.446–459）
> **章號**：影片 Chapter 9 = 課本 Ch9。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕把「writes」寫成「rights」、「PLSC」寫成「plsc」、「fr」寫成「F FR」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 為什麼需要新工具](#1-為什麼需要新工具)
- [2. 三步：parallel → execution witness → sequential](#2-三步parallel--execution-witness--sequential)
- [3. Happens-before：global 與 local](#3-happens-beforeglobal-與-local)
- [4. Program order 邊：po](#4-program-order-邊po)
- [5. Read-from 邊：rf（rfe、rfi）](#5-read-from-邊rfrferfi)
- [6. Write serialization 邊：ws](#6-write-serialization-邊ws)
- [7. From-read 邊：fr](#7-from-read-邊fr)
- [8. Synchronization 邊：so](#8-synchronization-邊so)
- [9. 全部放在一起：ghb](#9-全部放在一起ghb)
- [10. 為什麼「沒有環」這麼重要：topological sort](#10-為什麼沒有環這麼重要topological-sort)
- [11. Access graph：單一位址的 PLSC](#11-access-graph單一位址的-plsc)
- [12. Thin air read](#12-thin-air-read)
- [13. Causal graph](#13-causal-graph)
- [14. 一個 execution 正確的三個條件](#14-一個-execution-正確的三個條件)
- [15. 【實測】](#15-實測)
- [16. 一頁速查表](#16-一頁速查表)
- [17. 與後續講次的連結](#17-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
SC：parallel execution ──（直接）──▶ legal sequential execution         （兩步）
非 SC：parallel execution ─▶ execution witness（圖）─▶ sequential execution（不一定 legal）（三步）
execution witness：節點 = 讀、寫、sync；邊 = global 的 happens-before
  po（同 thread）、rf（寫 → 讀到它的讀）、ws（同位址的寫）、fr（讀 → 較晚的寫）、so（sync 之間）
  ghb = gpo ∪ grf ∪ ws ∪ fr；ws、fr 永遠 global（PLSC）；gpo、grf 由 memory model 決定
  沒有環 → topological sort → 一個 sequential execution → 這個結果在這個 memory model 下可行
access graph：只看一個位址，邊 = up ∪ rf ∪ ws ∪ fr → 沒有環 ⇔ PLSC
causal graph：rf ∪ gpo ∪ dep → 沒有環 ⇔ 沒有 thin air read（value prediction 可能造成）
正確 = execution witness 沒有環 ＋ 所有 access graph 沒有環 ＋ causal graph 沒有環
```

---

## 1. 為什麼需要新工具

[00:41] 材料都準備好了，接下來要描述**所有 memory model 構成的空間**。現況：

- [01:03] SC 不實際：處理器為了效能，會重排存取**不同位址**的指令。單處理器沒差，多處理器就會出現很不直覺的行為。
- [01:46] pipeline、non-blocking cache 這麼複雜，write 也可能**不 atomic**；IBM、ARM 就是這樣。

[02:09] 問題：一個**不是 SC** 的系統（也就是任何實際的系統），怎麼推理它的正確性？

[02:32] SC 的做法：把 parallel execution 對到 legal sequential execution。一段程式在 SC 機器上的所有 legal sequential execution（LSC1、LSC2……）如果都滿足某個性質（例如結束時 y = 3），不管怎麼跑，結果都滿足。平行程式可以有很多種結果，「只要每一種結果裡 y 都是 3，我就滿意」。

[03:46] 非 SC：「事情變複雜很多」：連列出所有 sequential execution 都很難，而且很多不是 legal 的。

---

## 2. 三步：parallel → execution witness → sequential

[04:07] 目標：做一個跟 SC 類似的理論工具。

[04:28] SC 的情況很簡單又優雅：對到的 sequential execution ① legal、② 照程式順序。非 SC 的 model「沒那麼好運」：

```
parallel execution ─▶ execution witness（一種特別的圖）─▶ sequential execution（可能 legal、可能不 legal）
```

[05:09] 好處：如果能從一個 parallel execution 經過 execution witness 做出 sequential execution，至少可以說：**對這個 memory model，這個結果是 valid 的**。所以可以用它列出所有 valid 的結果，看它們是不是都滿足某個性質。

[06:07] 兩步的 SC 是三步的**特例**。

[06:27] 一段平行程式有很多 execution、很多結果：分成 **valid** 和 **invalid**。違反 memory model 的（invalid）在那個架構上**看不到**；valid 的才看得到。

【課本補充】課本 p.447 Definition 71：**memory model** = 規範 core 和記憶體系統存取**多個**位址時的規則，定義了任何程式（循序或平行）所有 valid 的結果。課本 p.448：這套框架採用 **Alglave**（2012）的理論，「涵蓋到 2020 年為止所有的 memory model」。

---

## 3. Happens-before：global 與 local

[07:08] 例子：`x=1; t1=y ‖ y=1; t2=x`，SC execution：`x=1, t1=y（0）, y=1, t2=x（1）`。

- [07:49] 節點：每個指令（這裡 4 個）。
- 邊：依照 memory model 保證的順序加。不同的 memory model 對 execution witness 有不同的限制。

[08:11] 邊都是 **happens-before**（hb）邊：A → B 表示 A 先發生、B 後發生。

- [08:56] **不管時間差多少**：B 可以緊接在 A 後面，也可以過了很久。
- [09:15] 「我們永遠只看順序，不看確切的時間」：CPU 裡不確定的事件太多，對時間加限制會讓關係變得很複雜，也保證不了。

[09:36] 兩種 hb 邊：

| 種類 | 意思 | 符號 |
|---|---|---|
| **Global**（ghb） | 所有 thread（每個 thread 上坐一個觀察者）都同意 A 在 B 之前 | A →ghb B |
| **Local**（lhb） | 只有某些 thread 這樣認為，別的 thread 可能看到 B → A | A →lhb B |

【課本補充】課本 p.450：hb 有**遞移性**（A → B、B → C ⇒ A → C）。同一個觀察者記錄的 hb 邊**不能有環**：A → B → C → A 就表示 A → A，不可能。只有 ghb 邊的圖也一樣不能有環。課本 p.450–451 也用「tuple 的集合」定義 relation，介紹 union（∪）、intersection（∩）、subset（⊆）。

---

## 4. Program order 邊：po

[10:19] **po 邊**：**同一個 thread** 發出的記憶體操作之間的邊，**位址相不相同都可以**。

[11:01] 六種：

| 名字 | 從 → 到 | 是不是 global |
|---|---|---|
| poRW | 讀 → 寫 | 看 memory model |
| poRR | 讀 → 讀 | 看 memory model |
| poWR | 寫 → 讀 | 看 memory model；[11:26]「常常被違反」 |
| poWW | 寫 → 寫 | 看 memory model |
| **poIS** | 任何指令 → sync | **永遠 global**：前面的指令完全完成、大家都看到結果，sync 才執行 |
| **poSI** | sync → 任何指令 | **永遠 global** |

- [11:45] **execution witness 裡只放 global 的邊**。
- [12:05] SC 的所有 po 邊都是 global（尊重程式順序）。
- [12:45] 如果 sync 不是純粹的 fence（例如 [第 34 支 §13](lecture34_note.md#13-acquire-與-release) 的 acquire、release），這兩種邊的性質會跟著改，「看 sync 操作的種類」。

[13:34] 例：單一 thread `x=1; y=1; t1=x`（t1 = 1），在 SC 下：Wx1 →po Wy1 →po Rx1。

【課本補充】課本 p.451：**x86** 的 poWW 是 global，**PowerPC、ARM** 的不是。哪些 po 是 global，決定了 pipeline 和記憶體系統可以做哪些最佳化。

---

## 5. Read-from 邊：rf（rfe、rfi）

[14:18] **rf 邊**：**同一個位址**的寫 → 讀（讀到那個寫的值）。可以跨 thread。

[14:37] 兩種：

| 名字 | 寫和讀 | 是不是 global |
|---|---|---|
| **rfe**（external） | 不同 thread | **write atomic 時是 global**；不 atomic 時不是：別的 thread 會覺得這個寫在不同時間發生 |
| **rfi**（internal） | 同一個 thread | [17:03]「通常**不是** global」 |

[15:25] 例：`x=1 ‖ t1=x`，t1 = 1 → Wx1 →rf Rx1。

[16:41] 「細心的讀者會問：寫沒完成，你怎麼讀到的？」答：對**這個** thread 來說完成了，對**別的** thread 還沒有 → 這個寫不是 global。

[17:27] 為什麼 rfi 不是 global：**store-to-load forwarding**（[第 12 支](lecture12_note.md)）。有 LSQ 時，pipeline 裡把寫的值直接轉給讀，外面的世界很晚才看到 → core 可以**比別的 core 早**讀到自己寫的值。[18:11]「只要你**提早**讀到一個值，那條邊就不再是 global。」

⚠️ 影片 [17:27] 先說「load to load」，馬上改成「store to load forwarding」，筆記照改正後的。

【課本補充】課本 p.453：rf = rfe ∪ rfi；兩個都「不一定 global，看 memory model」。

---

## 6. Write serialization 邊：ws

[18:38] 例（課本圖 9.25）：

```
T1：x = 1      T2：x = 2      T3：t1 = x; fence; t2 = x      結果 t1 = 1、t2 = 2
```

[19:00] execution witness：

```
Wx1 ──ws──▶ Wx2
 │rf          │rf
 ▼            ▼
Rx1 ─po─▶ fence ─po─▶ Rx2
```

- **ws 邊**（write serialization）：同一個位址的兩個寫。因為要先 1 後 2，加 Wx1 →ws Wx2。
- [19:24] ws **永遠是 global**：直接來自 PLSC 和 coherence 的要求（[第 34 支 §11](lecture34_note.md#11-coherence-的兩個公理)）。

[20:10] 這個圖**沒有環** → 用 topological sort 排成：`Wx1, Rx1, Wx2, fence, Rx2`。[21:10] 這個順序照程式順序，**而且 legal** → 這個 execution 在 SC 裡。

---

## 7. From-read 邊：fr

[21:37] 例（課本圖 9.26）：

```
T1：x = 1; x = 2      T2：t1 = x      結果 t1 = 1
```

- Wx1 →ws Wx2；Wx1 →rf Rx1。
- [22:23] 讀到 1，之後同一個位址被寫成 2 → 加 **fr 邊**（from-read）Rx1 →fr Wx2：讀 → 比它讀到的那個寫**更晚**的寫。（不然就會讀到 2。）

[23:09] **不管 memory model 是什麼，fr 和 ws 都是 global**：

- [23:29] 直接來自 PLSC：write atomic 不 atomic 都沒關係；同一個位址的寫有一個大家都同意的全域順序 → 同一個位址的 讀 → 寫、寫 → 寫 也有全域順序（[第 34 支 §10](lecture34_note.md#10-從-plsc-到-coherence) 的 R→W、W→W）。
- [24:36] 為什麼一定要守住：要維持「每個變數只有一個位置」的抽象。[24:59]「一個變數可能有 20 個實體位置，但外面的觀察者**永遠不該知道**。ws 和 fr 就是從這裡來的。」
- po 和 rf 要不要放進來，看 memory model。

---

## 8. Synchronization 邊：so

[25:22] 例（課本圖 9.27）：

```
T1：x = 1; y = 1（y 是 sync 變數）      T2：t1 = y; t2 = x      結果 t1 = 1、t2 = 1
```

- [25:48] 假設 **y 是 synchronization 變數**（之後的講次會講怎麼指定）。
- [26:11] 用特別的 sync 版本的 load／store。**x86** 的 `lock` prefix：加在 load 或 store 前面，那個位址就變成 sync 變數。
- [26:31] 邊：Wx1 →po Wy1（不同位址，沒有 ws）；Wy1 →rf Ry1，**也是 so 邊**（「一條邊可以同時有好幾種類型」）；Ry1 →po Rx1。
- [27:11] 加上 Wx1 →rf Rx1 也沒有環 → valid，而且 legal、照程式順序。

[27:52] 規則：

- sync 變數的所有更新都是**全域有順序**的。
- [28:17] 很多 memory model 假設：**所有 sync 存取彼此之間是 SC**。這裡也這樣假設。

【課本補充】課本 p.454：對 sync 操作，rf 和 po 都假設是 global；ws、fr 因為 PLSC 本來就是 global。

---

## 9. 全部放在一起：ghb

[28:37] 五種邊：

| 邊 | 在哪裡 | 位址 |
|---|---|---|
| **po** | 同一個 thread | 相不相同都可以 |
| **rf** | 可以跨 thread | 相同 |
| **ws** | 可以跨 thread | 相同 |
| **fr** | 可以跨 thread | 相同 |
| **so** | sync 操作之間 | 同一個 sync 變數 |

[29:25] 跨 thread 沒有「讀 → 讀」的順序：誰先讀不重要。

[30:55] 更大的總結：

| 邊 | global 嗎 | 理由 |
|---|---|---|
| ws、fr | **永遠** | PLSC |
| so | **永遠** | 硬體實作 sync 的方式 |
| po | 一部分：**gpo** | memory model 決定 |
| rf | 一部分：**grf** | memory model 決定 |

[31:20] SC：grf = rf（rfe、rfi 都 global）、gpo = po。

```
ghb = gpo ∪ grf ∪ ws ∪ fr          （課本式 9.6）
SC  = po  ∪ rf  ∪ ws ∪ fr          （課本式 9.7）
```

[32:08] 做法：拿一個 parallel execution → 建 execution witness，邊照上面的規則加（gpo、grf 由 memory model 規定）→ **找環**。

⚠️ [31:20] 影片投影片先用「**PPO**（preserved program order）」，[55:49] 老師說「PPO 沒錯，但更適合的是 **GPO**（global program order），書裡也用 GPO」。筆記一律寫 gpo。

【課本補充】課本 p.455 的例子：如果 poSI、poIS、poWW 是 global，poWR、poRW、poRR 是 local，那 gpo = poSI ∪ poIS ∪ poWW；如果 rfe global、rfi local，grf = rfe（這就是 x86 的樣子，見 [§15.1](#151-execution-witness-檢查程式)）。

---

## 10. 為什麼「沒有環」這麼重要：topological sort

[32:30] 用經典的電腦科學結果 **topological sort**：

> 一個有向圖如果**沒有環**，就能把所有節點排成一列，使得：圖裡有 A 到 B 的路徑，A 就排在 B 前面。（課本 Theorem 9.3.6.1）

[33:10] 「A 到 B 有路徑」= 一串 global hb 邊 = A 先、B 後。排出來的順序就是**一種可能的完成時間順序**：照這個順序完成的話，沒有任何 hb 關係被違反 → 這是一個 sequential execution，而且是**可行的**（feasible）。

[35:05] 有環就不行：一個 hb 的環表示「某個操作發生在它自己之前」。

[36:09] 但排出來的順序**不一定 legal**：

- write atomic 時 legal。
- write 不 atomic 時（同一個寫對不同 thread 在不同時間完成），legality 會被違反。

[36:28] 「重要的理論點」：我們建立了 parallel execution 和 sequential execution（不一定 legal）的等價，而且這個 execution 在這個 memory model 下**可行**。原本的目標（找出所有 valid 的結果）還是達成了：

- [36:49] 列出一段程式所有可能的跑法，建出 execution witness，全部可以用**軟體工具自動做**，然後一個一個找環。
- [37:34] 甚至不用真的排成 sequential：**execution witness 沒有環，這個結果就是 valid 的**。

【課本補充】課本 p.455：證明某個 memory model **不會**出現某個結果的經典方法：證明它所有的 execution witness 都有環。課本 p.456：`SC = po ∪ rf ∪ fr ∪ ws` 裡，po 讓排出來的順序照程式順序；rf ∪ fr ∪ ws 讓它 legal（寫看起來 atomic、每個讀拿到最新的寫）。

（本機寫了一個自動找環的程式，見 [§15.1](#151-execution-witness-檢查程式)。）

---

## 11. Access graph：單一位址的 PLSC

[38:53] 再看兩個條件：單一位址的存取、資料和控制相依。

[39:16] **Access graph**：跟 execution witness 很像，但**只包含對同一個位址**的存取（可以多個 thread）。

[39:37] 新的邊：**up**（uniprocessor）邊：同一個 thread、**同一個位址**的兩個存取（讀讀、讀寫、寫寫都算）。它也代表程式順序，但只限同一個位址。

[40:18] 例（課本圖 9.29）：單一 thread `x=1; x=2; y=3; z=x+y; x=4`，只看 x：Wx1 →up Wx2 →up Rx2 →up Wx4。

[41:04] 類比 SC：

```
SC   = po ∪ rf ∪ fr ∪ ws
PLSC = up ∪ rf ∪ fr ∪ ws          （課本式 9.8）：po 換成 up
```

[42:34] **PLSC 不管 memory model 都要守住** → access graph 就是「只限一個位址的 execution witness」，裡面的 rf 也都是 global。[43:16]「對單一位址的存取**沒有 memory model 可言**」：access graph **不能有環**，有環就違反 PLSC。

⚠️ [43:16] 老師說「回想第 74 張投影片」，那是投影片的頁碼，筆記看不到投影片，**待補**。

[44:34] 例：

```
T1：x = 1; t1 = x（讀到 2）     T2：x = 2     T3：t2 = x（讀到 1）; t3 = x（讀到 0）
```

[45:31] T1：Wx1 →up Rx2；Wx1 →ws Wx2 →rf Rx2：沒有環。[45:56] T3 才出問題：

```
Wx1 ──rf──▶ T3:Rx1 ──up──▶ T3:Rx0 ──fr──▶ Wx1      → 環 → 違反 PLSC → 不允許
```

（T3 先讀到 1、再讀到更舊的 0：違反 PLSC。本機的程式也找到這個環，見 [§15.2](#152-access-graph)。）

【課本補充】課本 p.456–457：up 的由來：單一 thread 的程式在多核心上跑，結果不能跟 memory model 有關，程式設計師不該知道自己跑在單處理器還是多處理器上。**uniprocessor access constraint**：同一個 thread 對同一個變數的存取不重排，看起來照程式順序生效 = 單一 thread 的 PLSC。不同變數（例如 x 和 y）可以重排。課本例 11（圖 9.30）是另一個違反 PLSC 的 access graph。

---

## 12. Thin air read

[46:41] 「還沒完：有一種東西叫 **thin air read**。」

[47:22] 例子（影片的版本），假設有 **value prediction**（[第 14 支](lecture14_note.md)）：

```
T1：t1 = x; if (t1 == 1) y = 2;      T2：t2 = y; fence; x = 1;
結果：t1 = 1、t2 = 2
```

- [47:57] 單處理器上 value prediction 絕對正確：commit 前一定驗證。多處理器呢？
- [48:41] 假設 write atomic；memory model 沒有「讀 → 寫」（不同位址、同一 thread）的順序：[49:06]「很多叫做 **weak memory model** 的都沒有」→ T1 裡不加邊。
- [49:31] T2 有 fence → 兩條 po 邊。邊：Wy2 →rf Ry2、Wx1 →rf Rx1。
- [49:53] 這個 execution witness **沒有環** →「可能會想說它 legal 又可行，答案是**不**。」

[50:13] 因為**因果**（causality：cause-effect）斷了：

```
T1 讀到 x = 1 → 所以寫 y = 2 → T2 讀到 y = 2 → fence → T2 寫 x = 1
```

[51:21] 可是因果鏈的**最後**才把 x 設成 1，鏈的**開頭** x 根本不是 1。

[51:49] 怎麼發生的：

1. **預測** x = 1，繼續跑。
2. [52:15] **投機地**提早寫 y = 2，沒等前面的指令 commit。「在**極高效能**的處理器上真的會發生」：讀 → 寫的順序被違反，提早寫。
3. [52:38] T2 讀到 y = 2 → fence → 寫 x = 1。
4. [52:59] T1 驗證預測時讀到 x = 1 →「驗證通過」。

[53:23] 但 T1 原本讀的時候 x 是 0：t1 = 1 好像是**從空氣中**（out of thin air）讀到的 → 叫 **thin air read**。x 變成 1，**只是因為我們假設了** x 是 1。

[53:44] 照 execution witness 的邏輯這個 execution 是 valid 的（沒有環），但我們還是要**禁止**它。

[54:07] 【口頭】實作 **virtual machine**（例如 **Java virtual machine**）時也有 thin air read 的問題，所以這是一個一般的理論概念：「一個變數被假設成某個值，之後被證明是對的，**只因為**我們這樣假設」，是循環論證。

[54:48]「請大家看看這個例子，說服自己它不對；但 execution witness 方法說它對，所以要**加別的東西**。」

⚠️ 影片和課本用的例子**不一樣**：

| | 影片 [47:22] | 課本 p.458 圖 9.31 |
|---|---|---|
| T1 | `t1 = x; if (t1 == 1) y = 2` | `t1 = x; if (t1 == 1) t2 = y` |
| T2 | `t2 = y; fence; x = 1` | `y = 1; fence; x = 1` |
| 結果 | t1 = 1、t2 = 2 | t1 = 1、t2 = 0 |
| if 裡面 | **寫** | **讀** |

【課本補充】課本 p.459 Definition 74：**thin air read** = 沒看到前面的寫，就讀到了一個值。

---

## 13. Causal graph

[55:06]「我們很愛做圖」：**causal graph**，三種邊：

| 邊 | 為什麼 |
|---|---|
| **rf** | 資料的傳遞：寫了才讀得到 |
| **gpo** | global 的程式順序 |
| **dep**（新） | **資料相依和控制相依**：讀 → 用到它的值的指令；條件判斷 → 它的 body |

[56:27] 觀察者的觀點：[57:52]「把觀察者想成坐在讀的 **forward slice**（[第 15 支](lecture15_note.md)）上」：所有受這個讀影響的（資料或控制），觀察者都在追。這時 rf 邊算 global，因為觀察者不在乎別的 thread 怎麼看那個寫，只看資料的流向。

[58:11] thin air read = **一個讀在它自己的 forward slice 裡**：從這個讀出發，經過別的節點，又回到這個讀 → 不可能。

```
NoDepCycle = acyclic(rf ∪ gpo ∪ dep)          （課本式 9.9）
```

影片的例子：Rx1 →dep Wy2 →rf Ry2 →po fence →po Wx1 →rf Rx1 → **環** → 禁止。

⚠️ 課本的式 9.9 跟課本自己的圖 9.32 不一致（已看過 PDF p.459 的圖）：圖 9.32 的環是 `Rx1 →dep if →dep Ry0 →fr Wy1 →po fence →po Wx1 →rfe Rx1`，**用到了 fr 邊**，可是式 9.9 只有 rf ∪ gpo ∪ dep，沒有 fr。只用式 9.9 的話，課本這個例子**沒有環**；影片的例子（if 裡面是寫）只用 rf ∪ gpo ∪ dep 就有環。本機程式驗證了這兩點，見 [§15.3](#153-thin-air-read-與-causal-graph)。

---

## 14. 一個 execution 正確的三個條件

[59:12]「把全部放在一起」：從不同觀點（不同的觀察者）看一個 execution 是不是 valid：

| 條件 | 測試 |
|---|---|
| **符合 memory model** | execution witness **沒有環** |
| **所有位址都 PLSC** | 所有 access graph **沒有環**（不然「單一 shared memory」的概念就不成立） |
| **沒有 thin air read**（NoDepCycle） | causal graph **沒有環** |

[60:37] 「這大概是多處理器系統上任何 execution 正確的**三個最重要條件**。」

[60:37] 單處理器（或多處理器上只跑一個 thread）只需要 **PLSC**：每個讀只要拿到最新的寫。所以 PLSC 成立，**所有單執行緒程式在多處理器上都會正確**。多執行緒、多核心才需要另外兩個。

[61:16] 下一講：cache coherence。「這一章的前兩部分講完了。」

【課本補充】課本 p.459 Table 9.2 就是上面這張表。

---

## 15. 【實測】

程式在 `experiments/lecture35/`：`ew.py`（建圖、找環）、`run.py`（例子）。

### 15.1 Execution witness 檢查程式

`ew.py` 照 [§9](#9-全部放在一起ghb) 的規則建 execution witness：

- 節點：每個讀、寫、fence，加上每個位址一個「初值寫入」節點。
- rf 從讀到的值找出是哪個寫；**ws 的順序每一種都試**；fr 從 rf 和 ws 推出來。
- 只要有一種 ws 順序讓 ghb 沒有環，就說「可行」。

五個 memory model（只是用來比較的簡化版，不是哪個真實架構的完整規格）：

| Model | global 的 po | rfe | rfi |
|---|---|---|---|
| SC | 全部 | ✓ | ✓ |
| TSO（x86 的樣子） | RR、RW、WW（放掉 WR） | ✓ | ✗ |
| PSO | RR、RW（再放掉 WW） | ✓ | ✗ |
| WEAK | 只有碰到 fence 的 | ✓ | ✗ |
| WEAK-NA | 只有碰到 fence 的 | ✗（write 不 atomic） | ✗ |

WEAK-NA 多加一條規則：照 [第 34 支 §12](lecture34_note.md#12-用-fence-強迫-sc) 的 fence 定義（fence 前的寫都傳到所有 core，fence 後的寫才開始），有人讀到 fence 後的寫，那個 fence 一定已經完成 → 加一條 fence → 那個讀 的 global 邊。

結果（「✗」= 每一種 ws 都有環 = 這個 model 不允許）：

| 例子 | SC | TSO | PSO | WEAK | WEAK-NA | PLSC | 本機 x86 看過嗎 |
|---|---|---|---|---|---|---|---|
| [07:08] `x=1;t1=y ‖ y=1;t2=x`，0、1 | 可行 | 可行 | 可行 | 可行 | 可行 | 是 | |
| [18:38] 圖 9.25 | 可行 | 可行 | 可行 | 可行 | 可行 | 是 | |
| [21:37] 圖 9.26 | 可行 | 可行 | 可行 | 可行 | 可行 | 是 | |
| **SB**（兩個都讀到 0） | ✗ | **可行** | 可行 | 可行 | 可行 | 是 | **看過**（[第 33 支](lecture33_note.md)） |
| SB＋fence | ✗ | ✗ | ✗ | ✗ | ✗ | 是 | 沒看過（mfence） |
| **MP**（1、0） | ✗ | **✗** | 可行 | 可行 | 可行 | 是 | **沒看過**（第 33 支） |
| MP＋兩邊 fence | ✗ | ✗ | ✗ | ✗ | ✗ | 是 | |
| MP＋只有寫的那邊 fence | ✗ | ✗ | ✗ | 可行 | 可行 | 是 | |
| LB（`t1=x;y=1 ‖ t2=y;x=1`，1、1） | ✗ | ✗ | ✗ | 可行 | 可行 | 是 | |
| **WRC** | ✗ | **✗** | ✗ | 可行 | 可行 | 是 | **沒看過**（[第 34 支](lecture34_note.md#142-本機的-x86write-是-atomic-的嗎)） |
| WRC＋fence | ✗ | ✗ | ✗ | ✗ | **可行** | 是 | |
| IRIW＋fence | ✗ | ✗ | ✗ | ✗ | **可行** | 是 | |
| **CoRR**（同一位址看到相反順序） | ✗ | ✗ | ✗ | **可行** | 可行 | **否** | **沒看過**（第 34 支） |

找到的環（節錄）：

```
SB，SC 的環：   T1:Wx1 → T1:Ry0 → T2:Wy1 → T2:Rx0 → T1:Wx1     （po、fr、po、fr）
MP，TSO 的環：  T1:Wx1 → T1:Wy1 → T2:Ry1 → T2:Rx0 → T1:Wx1     （poWW、rfe、poRR、fr）
WRC＋fence，TSO 的環：T1:Wx1 → T2:Rx1 → fence → T2:Wy1 → T3:Ry1 → fence → T3:Rx0 → T1:Wx1
```

- 「SC」那一欄跟第 34 支的暴力交錯法（`is_sc_bruteforce`）**每一列都一樣**（程式裡有 assert）：兩步的 SC 確實是三步的特例（[06:07]）。
- **TSO 那一欄跟本機 x86 看到的完全吻合**：SB 可行（看過）、MP、WRC、IRIW、CoRR 不可行（都沒看過）。
- SB 的環裡只有 po 和 fr：TSO 一放掉 poWR，環就斷了。這正是 store buffer 造成的（[第 33 支](lecture33_note.md)）。
- **WRC＋fence、IRIW＋fence 在 WEAK-NA 下可行**：照影片的 fence 定義，fence 只保證**自己 thread** 的寫傳出去了；T2 讀到 T1 的寫之後才 fence，T1 的寫對 T3 不一定已經可見。真實的 IBM、ARM 的 fence 是不是比這更強，本機沒有那些機器，**待補**。
- **CoRR 在 WEAK 下「execution witness 沒有環」，但 PLSC 是「否」**：WEAK 不保留同一個 thread 兩個讀之間的 po，所以 execution witness 抓不到；要靠 [§11](#11-access-graph單一位址的-plsc) 的 access graph（up 邊）才抓得到。這就是為什麼 [§14](#14-一個-execution-正確的三個條件) 要**三個**條件，不能只看 execution witness。

### 15.2 Access graph

```
[45:01] 影片的例子：PLSC？False；SC？False；WEAK-NA 的 execution witness 可行？True
課本例 11（圖 9.30）：PLSC？False
```

- 影片的例子在最弱的 WEAK-NA 下 execution witness 也沒有環，但 access graph 有環（Wx1 →rf Rx1 →up Rx0 →fr Wx1）→ 還是不允許。再一次說明：**PLSC 要另外檢查**。

### 15.3 Thin air read 與 causal graph

```
[47:22] 影片的 thin air：WEAK 的 execution witness 可行？True
        causal graph 的環：T1:Rx1 → T1:Wy2 → T2:Ry2 → fence → T2:Wx1 → T1:Rx1
        TSO 可行？False（R→W 的 po 是 global）
課本圖 9.31：WEAK 可行？True
        式 9.9（rf ∪ gpo ∪ dep）的環：沒有
        再加上 fr 邊的環：T1:Rx1 → T1:Ry0 → T2:Wy1 → fence → T2:Wx1 → T1:Rx1
```

- 影片的例子：execution witness 沒有環，causal graph 有環 → thin air read，跟 [53:44]、[58:11] 一致。
- 在 TSO（x86 的樣子）下這個結果**連 execution witness 都不可行**：x86 保留「讀 → 寫」的順序，提早寫的那一步（[52:15]）就不被允許。
- 課本的例子：只用式 9.9 沒有環，要加上 fr 才有環（跟圖 9.32 畫的一樣）。也就是 [§13](#13-causal-graph) 的 ⚠️。

---

## 16. 一頁速查表

| 主題 | 重點 |
|---|---|
| 為什麼要新工具 | 非 SC 的系統對不到 legal sequential execution |
| 三步 | parallel execution → execution witness → sequential execution（不一定 legal）；SC 的兩步是特例 |
| Memory model | 存取多個位址時的規則；定義所有 valid 的結果 |
| Happens-before | 只看順序不看時間；global（大家都同意）vs local |
| po | 同 thread；RW、RR、WR、WW 看 model；IS、SI（碰到 sync）永遠 global |
| rf | 寫 → 讀到它的讀；rfe 在 write atomic 時 global；rfi 通常不是（store forwarding） |
| ws、fr | 同位址的寫的順序；讀 → 較晚的寫；永遠 global（PLSC） |
| so | sync 變數的存取；x86 的 `lock` prefix；sync 之間假設 SC |
| ghb | gpo ∪ grf ∪ ws ∪ fr；SC = po ∪ rf ∪ ws ∪ fr |
| 沒有環 | topological sort → 一種可能的完成順序 → 結果 valid；有環 = 某事發生在自己之前 |
| Access graph | 單一位址；up ∪ rf ∪ fr ∪ ws；沒有環 ⇔ PLSC |
| Thin air read | value prediction ＋ 提早寫 → 因果循環；JVM 也有這問題 |
| Causal graph | rf ∪ gpo ∪ dep 沒有環；觀察者坐在讀的 forward slice 上 |
| 正確的三個條件 | execution witness、access graph、causal graph 都沒有環；單 thread 只需要 PLSC |
| 實測 | TSO 模型的判斷跟本機 x86 看到的完全吻合；CoRR 只有 access graph 抓得到；課本式 9.9 少了 fr |

---

## 17. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| SC、PLSC、coherence 的兩個公理、fence | [第 34 支](lecture34_note.md) |
| 實作 ws（coherence 協定：snoopy、directory） | [第 36 支](lecture36_note.md)、[第 37 支](lecture37_note.md) |
| sync 變數、atomic 指令（`lock` prefix） | [第 37 支](lecture37_note.md) |
| 用 rf、po 分類真實的 memory model（TSO 等）、data race | [第 38 支](lecture38_note.md) |
| Value prediction | [第 14 支](lecture14_note.md) |
| Forward slice | [第 15 支](lecture15_note.md) |
| Store-to-load forwarding（rfi） | [第 12 支](lecture12_note.md) |

---

## 附：本講一句話總結

> **非 SC 的系統對不到 legal sequential execution，所以多走一步：把 parallel execution 畫成 execution witness，節點是讀、寫、sync，邊是 global 的 happens-before 關係：同 thread 的 po（哪幾種是 global 由 memory model 決定，碰到 sync 的永遠是）、寫到讀的 rf（rfe 在 write atomic 時 global，rfi 因為 store forwarding 通常不是）、同位址寫之間的 ws 和讀到較晚寫的 fr（PLSC 規定永遠 global）、sync 之間的 so，ghb = gpo ∪ grf ∪ ws ∪ fr 沒有環，就能用 topological sort 排出一種可能的完成順序，證明這個結果在這個 memory model 下 valid；另外每個位址的 access graph（up ∪ rf ∪ fr ∪ ws）沒有環才是 PLSC，causal graph（rf ∪ gpo ∪ dep）沒有環才沒有 value prediction 造成的 thin air read，三個都成立 execution 才正確。本機實測：照這套規則寫的自動找環程式，TSO 模型的判斷跟本機 x86 實際看到的結果完全吻合，CoRR 這種違反 PLSC 的結果只有 access graph 抓得到，而課本的式 9.9 少了 fr 邊，算不出課本自己圖 9.32 的那個環。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 9: Multicore Systems (Part IV) -- Execution Witnesses 的 YouTube 字幕（1:01:30）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch9 §9.3.6–9.3.9（p.446–459）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
