# Advanced Computer Architecture 第三十四支影片完整筆記：Multicore（三）：sequential consistency、PLSC、coherence 的兩個公理與 fence（Chapter 9, Part III）

> **課程**：Advanced Computer Architecture — Chapter 9: Multicore Systems (Part III) -- Sequential consistency, PLSC, Coherence（播放清單第 34 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕下載不到）（1:11:55）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch9** §9.3.2–9.3.5（p.435–446）
> **章號**：影片 Chapter 9 = 課本 Ch9。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 把「writes」有時聽成「rights」，筆記直接改正。[28:02] 老師說「SC does not imply PLSC」後馬上改口「sorry, it is the reverse」，筆記照改正後的寫。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Sequential consistency 的定義](#1-sequential-consistency-的定義)
- [2. SC = atomicity ＋ program order](#2-sc--atomicity--program-order)
- [3. SC 很直覺，但不實際](#3-sc-很直覺但不實際)
- [4. 單一變數的例子](#4-單一變數的例子)
- [5. PLSC](#5-plsc)
- [6. PLSC 不等於 SC](#6-plsc-不等於-sc)
- [7. 打破 atomicity 的例子](#7-打破-atomicity-的例子)
- [8. 觀察者的觀點](#8-觀察者的觀點)
- [9. 紅線：PLSC 不能妥協](#9-紅線plsc-不能妥協)
- [10. 從 PLSC 到 coherence](#10-從-plsc-到-coherence)
- [11. Coherence 的兩個公理](#11-coherence-的兩個公理)
- [12. 用 fence 強迫 SC](#12-用-fence-強迫-sc)
- [13. Acquire 與 release](#13-acquire-與-release)
- [14. 【實測】](#14-實測)
- [15. 一頁速查表](#15-一頁速查表)
- [16. 與後續講次的連結](#16-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
SC（Lamport，70 年代末）：parallel execution 等價於一個 legal sequential execution，
                          而且每個 thread 的操作照程式順序 → 「把各 thread 的指令交錯」
SC = write 的 atomicity ＋ program order；很直覺，但幾乎所有效能最佳化都會打破它
PLSC（per location SC）：只看單一位址時是 SC → distributed cache 看起來像 shared cache 的充要條件
  SC ⇒ PLSC，PLSC ⇏ SC；PLSC 是紅線，program order、atomicity 可以讓設計者打破
coherence 的兩個公理：write serialization（同一位址的寫有全域順序）＋ write propagation（寫不會遺失）
非 SC 的機器上用 fence（或 store barrier、acquire／release）強迫需要的順序
```

---

## 1. Sequential consistency 的定義

[00:00] 上一講建立了 parallel execution 和 sequential execution 的等價，但還沒變成一個**正確性的標準**（memory model）。第一個：**sequential consistency（SC）**。

[01:12] 「相當老，70 年代末 **Leslie Lamport** 提出，第一個廣泛被接受的 memory model。最直覺，但最不實際。」

**定義**：一個 parallel execution 等價於一個 **legal** sequential execution，而且這個 sequential execution 裡每個 thread 的操作順序都**照程式順序**（program order），就說這個 execution 是 **sequentially consistent**（屬於 SC）。

- [01:56] 跨 thread 的程式順序沒有意義；同一個 thread 的程式順序就是 fetch（或 retire）的順序。

[03:54] 換個方式看：把不同 thread 的操作**交錯**（interleave，像十指交扣），得到一個 sequential execution；如果它 legal（每個讀拿到最近一次寫），而且每個 thread 的操作都照程式順序，就是 SC。

[06:15] 一個 SC 的系統還是可以有**很多種結果**，只是每一種都要能用這樣的交錯產生。

【課本補充】課本 p.435 Definition 69：SC 有好幾種等價的定義。

---

## 2. SC = atomicity ＋ program order

[06:15] 兩個性質：

| 性質 | 意思 |
|---|---|
| **Atomicity**（write 的） | 所有操作能排在一條時間軸上 → 每個 write 看起來在某一個**確切的時刻**完成：之前的操作都看不到它，之後的都看得到。所有 core 都**同意**這個時刻（不在乎是哪一刻，只要存在）。read 本來就是 core 私有的動作，沒問題；write 要讓所有人看到，atomic 才重要 |
| **Program order** | 每個 thread 的操作在 sequential execution 裡照程式順序 |

[10:04] **SC = atomicity ＋ program order**。【口頭】「請大家去查字典，徹底了解『interleave』這個字：每個 thread 自己的順序不變，只是把它們交錯起來。」

[10:58] 「這是所有 consistency model 的**黃金標準**，最直覺。」能轉成這樣的 sequential execution，就能推理它的正確性。

---

## 3. SC 很直覺，但不實際

[11:45] 「難過的是」：**幾乎所有我們提出的最佳化都會打破 SC**：

| 最佳化 | 打破 |
|---|---|
| OOO 處理器、load 提早送出 | **program order** |
| NoC 等方法（這一章還會講） | **atomicity** |

[12:25] **單執行緒**的程式在多核心上跑，結果跟單核心完全一樣，沒問題。問題出在**共用變數的多執行緒**程式：存取**不同的位址**可以重排。

[13:32] 例：`x = 1; y = 1;` 對單一 thread 來說可以先送 y 再送 x，compiler 會重排、處理器一定會重排，不改變程式的結果（畢竟是不同的變數）。「但在多處理器系統裡會造成浩劫」：這種「無害的重排」，我們一直很隨便地在做，完全沒想過多處理器。load 提早送出 → [第 33 支 §7](lecture33_note.md#7-例子二先寫再讀) 的 t1 = 0、t2 = 0 → 不是 SC。

[14:31] 【口頭】「你在某處做了好事，別處就發生壞事。我們在記憶體系統和 pipeline 做了很棒的事，寫多執行緒程式時壞事就來了。」

---

## 4. 單一變數的例子

[15:22] 單一變數不會有「不同位址重排」的問題。

**例子 A**（屬於 SC）：

```
T1：Wx1、Wx2　　T2：Rx0？…Rx1、Rx2　　T3：Rx1、Rx2
排得出來：Rx0、Wx1、Rx1、Wx2、Rx2（T2）、Rx2（T3）：legal、照程式順序
```

[16:03] **例子 B**（不屬於 SC）：

```
T1：Wx1、Wx2
T2：Rx1、Rx2      ← 看到 1 再 2
T3：Rx2、Rx1      ← 看到 2 再 1（反過來）
```

「小作業：請驗證這不是 SC。」怎麼排都排不出合法的順序。

[17:07] 而且很不直覺：同一個 x 好像有**不同的儲存位置**：T2 看到正確的寫入順序，T3 看到相反的。坐在 shared cache 裡 x 那一格的觀察者，一定看到先寫 1 再寫 2；T3 不同意，所以 T3 的 execution 不合理。

[18:15] 單一變數（或多變數程式裡對單一變數的存取）如果不是 SC，看起來就**不像只有一個儲存位置**：這正是我們要 distributed cache 看起來像 shared cache 時想避免的。

（本機的檢查程式驗證了這兩個例子，見 [§14.1](#141-sc-和-plsc-的檢查)。）

---

## 5. PLSC

[19:08] **PLSC**（per location sequential consistency）：只看**對同一個變數（位置）的所有存取**，這個 execution 永遠是 SC。

- [19:57] 為什麼：一個 shared cache 裡 x 只有一個位置，坐在那裡的觀察者看到的一定是 SC：請求一個一個進來，每次讀拿到最近一次寫（legal）；同一個位址不會被重排（program order）。
- [21:49] 所以 **PLSC 是讓 distributed cache 對外面看起來跟 shared cache 一樣的充要條件**（necessary and sufficient）。課本有更正式的推理。
- [22:48] 「PLSC 我們**不會妥協**。」

【課本補充】課本 p.436 Definition 70：E|x 是只包含位置 x 的操作的 execution；所有 x 的 E|x 都是 SC，E 就是 PLSC。

---

## 6. PLSC 不等於 SC

[23:42] 回到 [第 33 支 §7](lecture33_note.md#7-例子二先寫再讀) 的例子：

```
T1：x = 1; t1 = y     T2：y = 1; t2 = x     結果 t1 = 0、t2 = 0
```

- 整體：**不是 SC**（OOO 處理器上會發生）。
- 對 x 單獨看：Rx0、Wx1 → SC；對 y：Ry0、Wy1 → SC → **是 PLSC**。

[24:38] 【口頭】為什麼需要理論？「以前人們不懂行星和星星怎麼動，就說獵戶座是在獵蠍子、獅子或公牛，創造了星座。後來發現大部分都能用牛頓或愛因斯坦的定律解釋（雖然還有暗物質、暗能量）。理論的目的是解釋實驗結果、工程上的決定。」面對這麼複雜的工程系統，沒有規格就會迷路。

[29:40] 結論：

```
SC ⇒ PLSC（整體照順序又合法，每個位址當然也是）
PLSC ⇏ SC（例：上面的例子）
→ PLSC 比較弱、SC 比較強
```

[30:28] PLSC 推不出 SC，是因為 load 提早送到 cache、store 在 commit 時才寫，load **超過**了 store。要 SC 的話，load 也得在 commit 時才送 → 沒有 OOO 執行、LSQ 的好處也沒了，「前面幾章學的效能提升全部歸零」。所以為了效能，**放棄 SC**，但不放棄 PLSC。

---

## 7. 打破 atomicity 的例子

[31:54] 上面打破的是 program order。這個打破的是 **atomicity**：

```
T1：x = 1
T2：while (x != 1); y = 1
T3：while (y != 1); t = x      → 讀到 x = 0？
```

[32:53] 假設第一次讀 while 的條件就是 1（不用繞圈）：

```
T1：Wx1
T2：Rx1 → Wy1
T3：Ry1 → Rx0
```

- 有一條很清楚的**因果鏈**：x 寫了 → T2 看到 → T2 寫 y → T3 看到 y → T3 應該看到 x = 1，卻讀到 0。
- **不是 SC**（違反 legality）。
- [33:39] **是 PLSC**（「請驗證，很簡單」）：x 單獨看：Rx0、Wx1、Rx1 → SC；y：Wy1、Ry1 → SC。

[34:30] 「好玩的是」：這次打破的是 **atomicity**：對 x 的寫好像在**兩個不同的時間**發生：T2 已經看到新的值，T3 還看到舊的值 → **不 atomic**。

（本機的檢查程式也驗證了，見 [§14.1](#141-sc-和-plsc-的檢查)。）

---

## 8. 觀察者的觀點

[35:05] 「PLSC 不是表示 write 是 atomic 嗎？」**看觀察者的觀點**：

| 觀察者 | write 是不是 atomic |
|---|---|
| 只看**對 x 的存取** | **是**（PLSC 成立） |
| 看**所有變數的存取**（例如透過 y 推出時間關係） | **不是** |

[36:26] 「這就是為什麼 memory consistency model 有點難：重要的是**觀察者和他們的觀點**。」

[37:26] 很多架構（**IBM、ARM**）的 write 不是 atomic，也是為了效能：

- [38:30] x 在好幾個地方有副本；core 2 離 core 1 比較近，先看到；core 3 比較遠，比較晚看到。有 NoC 的系統很容易發生。
- 「你在手機上跑程式，很可能就看到這種 execution。」
- [39:25] **Intel 的機器 atomicity 成立，IBM 和 ARM 的不成立**。

[41:50] 怎麼發現 write 不 atomic？**透過其他變數的存取**（這裡是 y）。只看 x 的話，write 最後一定會傳到 T3，之後的存取都會看到 1，永遠看不到不 atomic 的 execution。

（本機的 x86 測試了，見 [§14.2](#142-本機的-x86write-是-atomic-的嗎)。）

---

## 9. 紅線：PLSC 不能妥協

[39:25] **不是 PLSC** 的行為（例：[§4](#4-單一變數的例子) 的例子 B：T3 看到寫入的順序相反）會**完全破壞 shared memory 的概念**：外面的人看到先寫 1 再寫 2，大家最好都同意；T3 不同意，x 好像不只存在一個地方。

[44:14] 所以：

| | 處理器設計者 |
|---|---|
| **PLSC** | **紅線，不准跨過** |
| program order、atomicity | 看你做了哪些最佳化，**可以**打破（由 memory model 規定） |

[45:34] 小結：SC = atomicity ＋ program order；SC ⇒ PLSC；PLSC ⇏ SC；有 atomic write 的系統、沒有 atomic write 的系統都要有 PLSC；PLSC 提供 distributed cache「單一記憶體位置」的假象。

---

## 10. 從 PLSC 到 coherence

[45:34] PLSC 大約已經帶我們走了 95%：distributed cache 和 shared cache 對外面看起來一樣。拆成兩部分：

| | 怎麼做到 |
|---|---|
| 每個位置的 **program order** | 處理器、cache controller 不重排**同一個位址**的存取：cache 本來就是請求的 FIFO queue；pipeline 也不重排同一位址（LSQ 保證） |
| 每個位置的 **atomicity** | 坐在記憶體位址上的觀察者，只看那個位址時，一定看到 atomic 的 write；坐在 core 上的觀察者，因為中間有別的位址的存取，可能看到不 atomic 的 |

[47:03] 定義兩種順序：

| 順序 | 意思 |
|---|---|
| **Global order** | **所有**觀察者都記錄 O1 在 O2 之前（早一秒或一萬秒都沒關係） |
| **Local order** | 只有一部分觀察者這樣記錄 |

[47:52] 觀察者坐在 **core** 上，看**同一個變數**、不同 thread 的操作之間的順序：

| 順序 | global 還是 local | 理由 |
|---|---|---|
| read → read | 不重要 | 中間沒有 write 的話讀到一樣的值 |
| **write → read** | **local**（write 不 atomic 時） | 一個 core 可能**提早**讀到另一個 core 的 write，其他 core 還沒看到。例：[§7](#7-打破-atomicity-的例子) 裡 core 2 說「write 完成了我才讀」，core 3 不同意：「你讀得太早了，write 還在完成的過程中」 |
| **write → write** | **global** | 不然：T2 記錄 x = 1 先、x = 2 後，T3 記錄相反 → x 有**兩個最後的狀態** → 完全破壞單一位置的概念 |
| **read → write** | **global** | 某個 thread 讀到 x 的舊值，之後另一個 thread 寫了 x。如果有別的 thread 記錄相反的順序（寫在讀之前），表示它看到一連串的動作從那個 write 一路到這個 read；write 有全域順序，那個 read 就應該讀到那個 write 的值，但它沒有 → 矛盾 |

[59:29] 總結：write → write、read → write 是 global；write → read **只有在 write 是 atomic 的機器上**才是 global。

【課本補充】課本 p.442 §9.3.4 有更詳細的推導，分成好幾種情況（case I–IV）。

---

## 11. Coherence 的兩個公理

[60:43] 從 PLSC 提煉出 **coherence 的公理**：

| 公理 | 內容 | 來源 |
|---|---|---|
| **Write serialization**（WS） | 對**同一個位置**的所有 write 有一個**全域的順序**（大家都同意「先寫 1、再 2、再 3、再 5……」，坐在不同 core 的觀察者記錄的沒有矛盾）；由此 read → write 也是 global | PLSC |
| **Write propagation** | 一個 write **最後一定**會被所有 thread 看到：write 訊息永遠不會被系統丟掉 | 「write 不會遺失」 |

[61:57] coherence 大致等於 PLSC，再多一個 write propagation。cache 是 coherent 的，就自動符合 PLSC，而且保證 write 最後會傳到、完成。

【課本補充】課本 p.444：Write Serialization（WS）Axiom：所有對同一位址的 write，所有 thread 看到的順序都一樣；課本也叫它 Write Order Axiom。

---

## 12. 用 fence 強迫 SC

[61:57] 下一個問題：在有很多最佳化的系統上，怎麼**強迫** SC？用 **synchronization instruction**（sync 指令）。

[63:22] 常見的跨 thread 傳值寫法：

```c
T1：                     T2：
value = 3;               while (status != 1);
status = 1;              t = value;
```

在非 SC 的機器上**不一定對**：

1. 沒有 program order：寫入可能被重排；T2 的讀也可能被重排。
2. 沒有 atomicity：status 已經傳到 T2，value 還沒有。

「你可以試試看，不會 100% 正確。」

[64:41] 加 **fence** 指令：

| fence 的規則 |
|---|
| fence 之前（程式順序）的所有指令都要**完全完成**，fence 才執行：讀拿到值、寫不只 commit，還要**傳到所有 core**（更新變數的所有實體位置） |
| fence 之後的指令在 fence 完成前**都不能開始** |

[65:45] 所有架構都支援 fence。

```c
T1：                     T2：
value = 3;               while (status != 1);
fence;                   fence;
status = 1;              t = value;
```

- T1 的 fence：寫 status 時，value 已經**完全寫好**（也叫 globally completed）。
- T2 的 fence：讀 value 之前，確定讀到 status = 1 了。
- **不管底層的 memory model** 保證了什麼，這段程式都會對。

[67:05] 「工程師寫平行程式的方法就是插 fence。」但 fence **很貴**：「大概要 **200–300 cycle**」，像 flush pipeline：前面的都要完成，要等記憶體系統的 acknowledgement，或清空 write buffer、MSHR。

⚠️ 課本 p.446 寫的是「100–300 cycle」，影片說 200–300。本機量到的 `mfence` 大約只要 **46 cycle**（P-core）、23 cycle（E-core），比 200–300 少很多；那是因為測試裡前面的 store 都在 L1 裡。前面有 store 在等 cache miss 時才會很貴。見 [§14.3](#143-fence-要多久)。

[67:05] fence 是 **memory barrier** 的一種：對它前面的指令加上條件。其他種類：

| Barrier | 規則 |
|---|---|
| **Fence** | 最一般：不分讀寫，前面的全部完成，後面的都不開始 |
| **Store barrier** | 前面的 **store** 都要完全完成，後面的 store 才能開始（不管 load）→ 上面的 T1 用它就夠了，比較有效率 |
| **Load barrier** | 放在兩個 load 之間 |

---

## 13. Acquire 與 release

[69:20] 一對特別的 fence（之後講 critical section、lock、unlock 時會用到）：

| 指令 | 規則 |
|---|---|
| **Acquire** | 進入 **critical region** 時：acquire **之後**的指令，在 acquire 完成前都不能執行（先拿到某個權限或 lock） |
| **Release** | 離開時：release **之前**的所有指令（讀、寫）都要完全完成，release 才能完成（load：拿到結果；store：值送到所有 core）。release **之後**的指令可以在 release 完成前先執行 |

[70:35] 有一個 memory model 叫 **release consistency** 用到這個（[第 38 支](lecture38_note.md)）。

[70:35] 總結：SC、PLSC、PLSC 和 coherence 的關係、coherence 的兩個公理（write serialization、write propagation）、sync 指令。「現在我們有了深入 memory model 理論的所有基本材料。」

---

## 14. 【實測】

程式在 `experiments/lecture34/`。

### 14.1 SC 和 PLSC 的檢查

`check.py`：給一個 execution（每個 thread 一串讀寫，讀附上讀到的值），用搜尋（加記憶化）找有沒有一種「照每個 thread 的程式順序」交錯、而且 legal 的排法 → SC；每個位址單獨拿出來都是 SC → PLSC：

```
[15:22] 單一變數，T2、T3 看到一樣的順序：SC？True　PLSC？True
[16:03] 單一變數，T3 看到相反的順序：SC？False　PLSC？False
[23:42] SB：兩個 load 都讀到 0：SC？False　PLSC？True
[32:25] 三個 thread（write 不 atomic）：SC？False　PLSC？True
MP：t1 = 1、t2 = 0：SC？False　PLSC？True
```

- 老師的兩個「請驗證」都對：[16:03] 的例子 B 不是 SC（連 PLSC 都不是）；[33:39] 三個 thread 的例子不是 SC 但是 PLSC。
- SB、MP 都是「PLSC 但不是 SC」：PLSC ⇏ SC。
- 沒有一個例子是「SC 但不是 PLSC」：SC ⇒ PLSC。

### 14.2 本機的 x86：write 是 atomic 的嗎

`litmus3.c`：三個常見的多執行緒例子，各跑 100 萬次（thread 綁在 cpu2、4、6、8 四個不同的 P-core 上，每一輪用新的變數），跑兩次：

| 例子 | 問的是 | 「有意思」的輪 | 不是 SC 的結果 |
|---|---|---|---|
| **WRC**（[§7](#7-打破-atomicity-的例子)）：T1：x=1　T2：t1=x; y=1（如果 t1=1）　T3：t2=y; t3=x | t1=1、t2=1、t3=0？（write 不 atomic） | 4235／3914 | **0／0** |
| **IRIW**：T1：x=1　T2：y=1　T3：讀 x 再讀 y　T4：讀 y 再讀 x | T3 看到「x 先」、T4 看到「y 先」？ | 108866／107620 | **0／0** |
| **CoRR**（[§4](#4-單一變數的例子) 例子 B）：T1：x=1　T2：x=2　T3、T4：各讀 x 兩次 | T3 看到 1 → 2、T4 看到 2 → 1？（違反 write serialization） | 31546／31846 | **0／0** |

（「有意思」的輪：讀的 thread 真的看到了寫入在中途發生，這種輪才有可能出現不是 SC 的結果。）

- **WRC 和 IRIW 從來沒出現**：本機 x86 的 write 是 **atomic** 的，跟 [39:25]「Intel 的機器 atomicity 成立」一致。IBM、ARM 上這兩種可能出現（本機沒有，**待補**）。
- **CoRR 從來沒出現**：兩個 reader 看到的 x 的寫入順序一定一樣，這就是 **write serialization**、PLSC 的紅線。這一條連 IBM、ARM 都不准打破。
- 配合 [第 33 支 §17.1](lecture33_note.md#171-兩個例子在本機跑-100-萬次)：本機 x86 會打破 SC，但**只**打破「store → 後面的 load」這一種 program order（SB）；其他的都守住了。

### 14.3 Fence 要多久

`fence.c`：每圈一個 store（到 L1 裡的變數），後面接不同的 fence，量每圈多少 ns：

| | P-core（cpu2） | ≈ cycle（4.7 GHz） | E-core（cpu14） | ≈ cycle（3.6 GHz） |
|---|---|---|---|---|
| 沒有 fence | 0.28 ns | 1 | 0.31 ns | 1 |
| `mfence` | 9.77 | **46** | 6.27 | **23** |
| `sfence` | 1.50 | 7 | 0.84 | 3 |
| `lfence` | 3.64 | 17 | 7.87 | 28 |
| `lock add`（atomic 指令，也有 fence 的效果） | 5.25 | 25 | 5.30 | 19 |

- `mfence`（完整的 fence）約 **46 cycle**，比 store 本身貴 35 倍，但比影片 [67:05] 說的 200–300 cycle 少很多：這裡的 store 都在 L1 裡，fence 只要等 store buffer 清空。前面有 store 在等 cache miss（要去 L3 或記憶體）時，fence 就要等那麼久。
- `sfence`（store barrier）便宜很多：x86 的一般 store 本來就照順序，sfence 主要是給 non-temporal store（[第 21 支 §14.2](lecture21_note.md#142-只寫也要先讀整個-block)）用的。
- `lock add` 比 `mfence` 便宜，所以 compiler 常用 `lock` 開頭的指令代替 `mfence`。

---

## 15. 一頁速查表

| 主題 | 重點 |
|---|---|
| SC | Lamport；等價於 legal sequential execution 且每個 thread 照程式順序；= 把各 thread 的指令交錯 |
| SC 的兩個性質 | write 的 atomicity ＋ program order |
| 為什麼不實際 | OOO、load 提早送出打破 program order；NoC 等打破 atomicity |
| PLSC | 每個位址單獨看是 SC；distributed cache = shared cache 的充要條件；紅線 |
| 關係 | SC ⇒ PLSC；PLSC ⇏ SC（SB、WRC 都是 PLSC 但不是 SC） |
| Atomicity 被打破 | WRC：T2 看到 x 的新值、T3 還看到舊的；IBM、ARM 會，Intel 不會 |
| 觀察者 | 只看 x：atomic；看所有變數：可能不 atomic |
| Global／local order | 同一位址：W→W、R→W 是 global；W→R 只在 atomic write 時是 global |
| Coherence 公理 | write serialization（同一位置的寫有全域順序）＋ write propagation（寫最後一定被看到） |
| Fence | 前面全部完成才執行、後面全部等它；「200–300 cycle」；store barrier、load barrier |
| Acquire／release | acquire：後面的等它；release：等前面的全部完成，後面的可以先跑 |
| 實測 | 檢查程式：例子 B 不是 PLSC，WRC、SB、MP 是 PLSC 但不是 SC；x86 上 WRC、IRIW、CoRR 各 100 萬次 0 次；mfence 約 46 cycle |

---

## 16. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Execution、legal sequential execution、等價 | [第 33 支](lecture33_note.md) |
| Execution witness、happens-before | [第 35 支](lecture35_note.md) |
| 實作 coherence 的協定（snoopy、directory） | [第 36 支](lecture36_note.md)、[第 37 支](lecture37_note.md) |
| Atomic 指令、lock | [第 37 支](lecture37_note.md) |
| 真的 memory model（TSO、release consistency）、data race | [第 38 支](lecture38_note.md) |
| LSQ 不重排同一位址 | [第 12 支](lecture12_note.md) |

---

## 附：本講一句話總結

> **Sequential consistency（Lamport）要求一個 parallel execution 等價於一個每個 thread 都照程式順序、而且每個讀都拿到最近一次寫的 sequential execution，也就是把各 thread 的指令交錯起來，等於「write 的 atomicity 加上 program order」，最直覺卻最不實際，因為 OOO 的 load 提早送出打破 program order、NoC 和分散的副本打破 atomicity；比它弱的 PLSC 只要求每個位址單獨看是 SC，是 distributed cache 看起來像 shared cache 的充要條件，所以是處理器設計者不准跨的紅線（SC 推得出 PLSC，反過來不行），由它可以推出 coherence 的兩個公理：同一位置的寫有全域順序（write serialization）、寫一定會傳到（write propagation）；在不是 SC 的機器上，要用 fence、store/load barrier 或 acquire/release 強迫需要的順序。本機實測：檢查程式確認了老師留的兩個「請驗證」，x86 上 WRC、IRIW、CoRR 各跑 100 萬次都沒出現不是 SC 的結果（write 是 atomic 的、同一位址的寫順序大家都同意），而一個 mfence 在 store 都在 L1 裡時約 46 個 cycle，比影片說的 200–300 cycle 少。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 9: Multicore Systems (Part III) -- Sequential consistency, PLSC, Coherence 的 whisper 轉錄稿（本機 `large-v3-turbo`，1:11:55）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch9 §9.3.2–9.3.5（p.435–446）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
