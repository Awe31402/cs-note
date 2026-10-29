# Advanced Computer Architecture 第十五支影片完整筆記：Instruction replay：forward slice、non-selective、delayed selective 與 token（Chapter 5, Part II）

> **課程**：Advanced Computer Architecture — Chapter 5: Alternative Approaches to Issue and Commit (Part II) (Instruction Replay)（播放清單第 15 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 自動字幕是亂碼）（1:14:23）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch5** §5.2（p.162–173）
> **章號**：影片 Chapter 5 = 課本 Ch5。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有；[55:57] 是老師自己說「學生常問」）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 把「XOR」寫成「ZOR」、「ROB」寫成「ROV」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 為什麼不要每次都 flush](#1-為什麼不要每次都-flush)
- [2. Forward slice](#2-forward-slice)
- [3. Non-selective replay：window of vulnerability](#3-non-selective-replaywindow-of-vulnerability)
- [4. Timer 與 kill wire](#4-timer-與-kill-wire)
- [5. 怎麼重做：留在 window 或放進 replay queue](#5-怎麼重做留在-window-或放進-replay-queue)
- [6. Orphan instruction](#6-orphan-instruction)
- [7. Delayed selective replay：poison bit](#7-delayed-selective-replaypoison-bit)
- [8. 救回 orphan：completion bus](#8-救回-orphancompletion-bus)
- [9. WV 該從哪裡開始算](#9-wv-該從哪裡開始算)
- [10. 為什麼 poison bit 和 WV 都要](#10-為什麼-poison-bit-和-wv-都要)
- [11. Token based replay](#11-token-based-replay)
- [12. 四種做法比較](#12-四種做法比較)
- [13. 【實測】](#13-實測)
- [14. 一頁速查表](#14-一頁速查表)
- [15. 與後續講次的連結](#15-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
上一講：猜位址、相依、延遲、值 → 一定會猜錯
猜錯了：
  flush 整條 pipeline：簡單，但很貴
  只重做「用到錯值」的那些指令（forward slice）：
    non-selective：WV 裡醒過的運算元全部殺掉（殺太多 → orphan）
    delayed selective：加 poison bit，已 issue 的只重做被 poison 的；orphan 用 completion bus 救
    token：每個可能出事的 load 一個 token，沿 forward slice 傳，只殺有 token 的
```

---

## 1. 為什麼不要每次都 flush

[00:00] Aggressive speculation（[第 14 支](lecture14_note.md)）一定會猜錯，這時要 **replay**（重做）。

最簡單的做法：**flush 整條 pipeline**（像分支猜錯一樣）。但每次猜錯都 flush 很貴：

| 代價 | 說明 |
|---|---|
| 清理、回復狀態 | 要好幾個 cycle |
| **浪費的工作** | 更大的問題：做了一大堆指令，全部丟掉重做 |
| 能量 | 結果多花能量 |

[02:30] 新想法：只 flush **一部分**。找出可能用到錯值的指令，**只重做它們**。重做時一定拿到對的值（等 load 真的拿到值，就不用再猜）。

【課本補充】課本 p.163：猜錯的那條後面可能已經抓了 50 條，flush 就全丟了。而 forward slice 通常很小，用 flush 「像拿**大砲打蚊子**」。這一節的做法是 **Kim and Lipasti 2004** 提出的，課本說的是簡化版。

---

## 2. Forward slice

[04:43] 例子：

```
        I0
       /  \
     I1    I2
    /  \
  I3    I4
    \  /
     I5 …
```

I0 產生的值給 I1、I2；I1 給 I3、I4；I4 同時用 I3 和 I1 的結果……這是一個 **DAG**（directed acyclic graph，有方向、沒有迴圈的圖），**不是樹**。

[05:35] 【口頭】「如果你不知道 DAG 是什麼，現在就去資料結構的書查，計算機結構裡躲不掉這個觀念。」

[05:48] 以 I0 為根的整個集合（I0、它的 consumer、consumer 的 consumer……）叫 I0 的 **forward slice**。只有 forward slice 要重做。

[06:21] 跟 I0 同時 issue、但跟它無關的指令，可以照常執行。Commit 還是照程式順序。

[06:56] 「只 flush dispatch 到 execute 之間的部分」也不行：不正確、不實用、很難做。

【課本補充】課本 p.162 Definition 24：動態取消一條指令叫 **squash**。

【課本補充】課本 p.162：load 推測會不會讓**分支**走錯？課本說在現代 OOO pipeline 不會：分支在 commit 時才驗證，那時它前面的推測 load 都已經驗證過了，所以只要看資料相依的 forward slice。

---

## 3. Non-selective replay：window of vulnerability

[07:43] 第一個方案。觀察：不用把 dispatch 到 execute 之間的指令全部移掉，只要處理「**可能拿到錯值**」的那些。

[08:43] **Window of vulnerability（WV）**：load 被選中之後的 **N 個 cycle**。

[09:09] N 由**預測的延遲**決定。例：猜 load 會在 data cache 命中：

| Cycle | Load 在做什麼 |
|---|---|
| 0 | 被選中 |
| 1 | 讀 register file |
| 2 | 算位址 |
| 3–4 | 存取 data cache（假設 2 cycle） |

第 4 個 cycle 結束就知道有沒有命中 → **N = 4**。超過 N 還沒拿到值，就要 replay。

[10:33] 為什麼這樣定義：在 WV 裡醒來的**運算元**，可能是被推測地叫醒的，它可能被選中、往下走、卻拿不到值。

[11:47] 例子：指令 1 是 `ld r1, [r2]`，猜它的值。指令 2、3、4 都有至少一個運算元在指令 1 的 WV 裡醒來（r1 的一定是；其他兩條可能是被別的 broadcast 叫醒的）。猜錯時：

- **squash** 這三條：把它們從 ready 設回 not ready。
- 等指令 1 拿到正確的 r1，重新 issue 它，再重新 issue 這三條。
- [13:08] 兩種情況都一樣處理：運算元醒了但指令還沒被選中（還在 window 裡），或已經被選中往下走了。

---

## 4. Timer 與 kill wire

[13:34] Instruction window 每個來源運算元，除了 tag 和 ready bit，再加一個 **timer**：

```
運算元醒來（tag 比對到）→ timer = N
每個 cycle timer − 1，到 0 為止
timer = 0 → 已經過了 WV，不會被殺
```

[15:02] 猜錯時：**拉高 kill wire**。kill wire 接到 window 的每一格。

[16:13] 電路：一個 NAND gate，兩個輸入是「kill wire」和「timer ≠ 0」：

| kill | timer ≠ 0 | NAND 輸出 | 結果 |
|---|---|---|---|
| 1 | 1 | **0** | ready bit 設 0、timer 歸 0 → **squash** |
| 其他 | | 1 | 什麼都不動 |

[17:13] 所以叫 **non-selective**：它不看指令在不在 forward slice 裡，WV 裡醒過的運算元一律殺。這是**運算元層級**的動作，不是指令層級。

[18:13] 每條推測的指令帶著它預期的延遲 N。例如猜 data cache 命中，結果 miss，超過 N → replay。

---

## 5. 怎麼重做：留在 window 或放進 replay queue

[19:49]

| 做法 | 怎麼做 | 什麼時候可以移掉 |
|---|---|---|
| **方法一：留在 instruction window** | 已經 issue 的指令**不移出** window，讓它照常往下走 | 所有來源運算元的 timer 都歸 0（過了 WV）→ 算「驗證過」，可以移掉。可以用一個小電路，或每 10 個 cycle 掃一次 |
| **方法二：replay queue** | issue 後移出 window，同時放進一個專用的 **replay queue** | 驗證過就從 replay queue 移掉 |

[24:16] Replay queue 是「已 issue 但還沒驗證」的暫存區。要 replay 時，從 replay queue 經過一個多工器重新進 pipeline。

[25:44] 方法一要 replay 時，那些指令本來就在 window 裡，直接往下走，**優先權比其他指令高**。這次驗證一定會過（它們是在至少那個猜錯的 load 的 forward slice 裡，現在值是對的）。

[26:30] **方法二比較常用**：比較簡單、比較好擴充、在硬體裡好做，而且不用再讓本來就很複雜的 window 更複雜。【口頭】「但我不想選邊站。」

【課本補充】課本 Figure 5.7（p.166）畫的就是 replay queue 的做法。

---

## 6. Orphan instruction

[27:14] 很重要的角落情況。只要 squash 的是 forward slice 的**超集合**，就會有這個問題。

例子：

```
1  ld  r1, …        ← 猜它的值
2  add r2, r1, r1   ← 用 r1：在 forward slice
3  sub r3, r2, …    ← 用 r2：在 forward slice
4  xor r5, r6, r7   ← 跟 1、2、3 無關
```

[28:54] 2、3、4 都在 WV 裡，都被 squash。2、3 會被自然地重新叫醒（1 會 broadcast r1，2 會 broadcast r2）。

[29:42] 但 **xor 不在 forward slice 裡**：它只是剛好有個運算元（例如 r6）在 WV 裡醒來。r6 的 ready bit 被設回 0，但寫 r6 的指令**早就 commit、離開了**，沒有人會再 broadcast r6。

[31:16] → xor 永遠沒人叫醒，叫 **orphan instruction**（孤兒指令）。不處理的話，它會**永遠**留在 pipeline 裡。

[32:20] 兩種解法：

| 解法 | 評價 |
|---|---|
| 加一個輔助結構，記下被 squash 的指令，找出哪些不在 forward slice，重新 broadcast 它們的 tag（或一開始就不殺） | 「壞主意的經典例子」：很難做對 |
| **等 orphan 到 ROB 頭** | 簡單。ROB 頭的指令不可能在等暫存器運算元（前面的都 commit 了），它在等就表示被 squash 了 → 直接用 register file 的值執行、commit |

[35:11] 第二種犧牲一點 ILP（本來可以早點做、早點叫醒它的 consumer）。也可以折衷：到 ROB 的某個位置就檢查。但最好**少製造** orphan。

【課本補充】課本 p.167 用的例子是 `ld r1` → `sub r4, r1, r3`、無關的 `mul r5, r6, r7`，說這是一種 **deadlock**。

---

## 7. Delayed selective replay：poison bit

[35:46] 想法：只重做 **forward slice**，其他的讓它過。一樣會有 orphan，但用不同方法處理。

[36:36] 保留 non-selective 的一切（kill wire、WV、timer），**再加上**：

[37:13] 偵測到猜錯時，除了拉 kill wire，還在 load 的**目的暫存器**放一個 **poison bit**：

- 值從 64 bit 變 **65 bit**：register file、bypass path 都加寬一個 bit。
- [38:18] 例如猜 data cache 命中、N 個 cycle 後發現不會從 data cache 來：做一個**假的值**，poison bit 設 1，寫進 register file、也送上 bypass path。
- [39:15] 為什麼要寫假的值？被推測叫醒的 consumer 反正會去讀 register file。與其讀到一個錯的值，不如讀到一個**明確標記為有毒**的值。

[39:55] **傳播**：一條指令讀到任何 poison bit = 1 的運算元（不管從 register file 還是 bypass），它的目的暫存器的 poison bit 也設 1。

```
ld  r1, …         ← 猜錯：r1 有毒
add r2, r1, …     → r2 有毒
sub r3, r2, …     → r3 有毒
…                   整個 forward slice 都被標記
```

[41:39] Pipeline 要改的地方：指令執行完（EX 之後），本來要在 ROB 設「完成」。先檢查 poison bit：

- 有毒 → **squash**：window 的 ready bit 設 0、ROB 的完成 bit **不設**。
- 在 replay queue 的驗證邏輯多加一關：有毒就留在 replay queue，沒毒才移掉。

[43:19] 好處在哪？看「在 WV 裡、但不在 forward slice」的指令，分兩種：

| | 結果 |
|---|---|
| **已經 issue** 的 | 它們的 poison bit 是 0 → 乾淨，直接通過。**少了這些 orphan** |
| **還沒 issue** 的（WV 過了還沒被選中） | 被 kill wire 殺掉一個運算元 → 還是可能變 orphan |

【課本補充】課本 p.168：poison bit 只在**讀值**（register file 或 bypass）時傳播，broadcast tag、wakeup 時**不**傳播。

---

## 8. 救回 orphan：completion bus

[45:00] 剩下的 orphan（還沒 issue 的），可以等它到 ROB 頭。但原始論文用了別的辦法。

[46:30] 情境：

```
時間 →
  I broadcast（early）……… K broadcast，叫醒 J 的運算元 s1 ……… N cycle 後：I 猜錯，拉 kill wire
                                                                → J 的 s1 timer 還沒歸 0，被殺
                                                                → J 還沒 issue，變 orphan
```

[48:02] 加一條 **completion bus**：

- 每條指令（K）在第一次 broadcast 之後 **N 個 cycle**，如果它自己成功執行（沒毒、沒猜錯），在 completion bus 上**再 broadcast 一次** tag。
- [49:28] J 看到 completion bus 上的 tag，把 s1 的 ready bit 再設回 1。
- K **回來救** J。

[50:23] 重點：靠 poison bit，把可能的 orphan 分成「已 issue」（沒問題）和「沒 issue」（等 ROB 頭，或用重複 broadcast 救）。

【課本補充】課本 Figure 5.8（p.169）就是這個情境。

---

## 9. WV 該從哪裡開始算

[51:11] 前面說 WV 從 load **被選中**開始。其實可以更準：

- [51:46] 從 consumer 的角度，重要的是 **producer broadcast** 的那一刻，和「執行完成、或確定不會完成而拉 kill wire」的那一刻。
- [52:46] consumer 收到 broadcast、同一個 cycle 被選中，timer 設 N；到最晚可能收到 kill 的那個 cycle，timer 應該剛好是 1。
- [54:33] 所以 WV 應該**從 producer broadcast 開始**。

[55:12] 從 select 開始算也**不會錯**：N 會比需要的大，WV 往右延伸，只是多一些 squash、多一些 orphan。**正確性的條件**：不能讓一條應該被 squash 的指令完成（不能讓它拿到錯的值）。

---

## 10. 為什麼 poison bit 和 WV 都要

[55:57] 【口頭】「學生常問」：既然有 poison bit，為什麼還要 WV？

[56:44] 只有 poison bit 的話，forward slice **不會停**：load 有毒 → consumer 有毒 → consumer 的 consumer……一直往下長，變得非常大。之後正確的 r1 來了，又要全部重做。

[57:18] WV 結束時拉 kill wire，等於**畫一條線**：在 WV 裡醒來、還沒 issue 的 forward slice 指令被殺掉，不會再往下長。剩下少數 orphan 再處理。

[58:22] 往下的方向：只用 poison bit，但要：

1. **一個 bit 不夠**：同時可能在猜好幾個 load，要分得出是誰的 forward slice。
2. 要在 **rename table** 層級改，只改 instruction window 不夠。

→ token based replay。

---

## 11. Token based replay

[59:07] 擴充 poison bit 的想法。先利用一個程式的樣式：

- [59:24] **大部分的 data cache miss 來自很少數的指令**：常見的經驗法則 **90/10**：90% 的 miss 來自 10% 的指令（也可能是 80/20，不是硬規則）。
- [60:00] 設計一個 **hit-miss predictor**（跟 branch predictor 一樣，用 PC 查），放在 decode 附近，知道是 load 之後就猜。

| 集合 | 是什麼 | 怎麼處理 |
|---|---|---|
| **S1** | 猜會 **miss** 的 | 執行時間不確定、很可能要 replay → 給一個 **token** |
| **S2** | 猜會 hit 的 | 不給 token |

[61:47] 也可以不用 hit-miss predictor，改用 **confidence predictor**：低信心的就不猜，或猜但準備好 replay（給 token）。

### 11.1 拿 token

[62:11] Token 就是一個號碼（例如 10 個 token：0–9）。一個像 free list 的 **token allocator** 發 token。

例：`ld r1, 4[r4]` 猜會 miss → 拿到 token 5，存進它的 instruction packet。它是 token 5 的 **token head**。

### 11.2 傳 token

[63:25] 跟 poison bit 一樣，沿 forward slice 傳：整個 forward slice 都有 token 5。

[64:58] Rename table 每格除了實體暫存器（和 available bit），再加一個 **token vector**（每個 token 一個 bit）。一條指令可以同時有好幾個 token。

[66:31] 規則：

```
add r1, r2, r3：
  tokens(r1) = tokens(r2) ∪ tokens(r3)
ld  r1, [r4]（token head，拿到 token t）：
  tokens(r1) = {t} ∪ tokens(r4)
```

[69:28] 讀來源的 token vector → 取聯集 → 寫回目的暫存器那一格，同時也存進 instruction packet（和 window 的那一格），之後的 pipeline 要用。rename table「又被弄得更複雜了」。

[69:59] 關鍵：不是一個 poison bit，而是**很多個** poison bit，每個是一個 token。

### 11.3 結果出來

[70:30] Token head（load）執行完，檢查有沒有多花時間：

| 情況 | 做什麼 |
|---|---|
| **出事了** | broadcast 這個 token id → 有這個 token 的指令都 replay |
| **沒事** | broadcast token id → window 和 rename table 裡每一格把那個 bit 清成 0 → token 放回 token 的 free list |

[72:24] 這樣 forward slice 就**不會一直長**：token head 一確定沒事，token 就收回。

### 11.4 沒拿 token 的出事了

[72:57] S2 的指令（猜會 hit）萬一 miss 了，它沒有 token，只能用最貴的做法：**flush pipeline**。

---

## 12. 四種做法比較

| 做法 | 殺哪些 | Orphan | 硬體 |
|---|---|---|---|
| **Flush** | 猜錯那條之後**全部** | 沒有 | 本來就有（跟分支一樣） |
| **Non-selective** | WV 裡醒過的運算元 → forward slice 的**超集合** | 有，等 ROB 頭 | 每個運算元一個 timer ＋ kill wire |
| **Delayed selective** | 已 issue 的只殺有毒的；沒 issue 的照 WV 殺 | 少一些，completion bus 救 | ＋ 65-bit 資料路徑（poison bit）＋ completion bus |
| **Token** | 只殺有那個 token 的（就是 forward slice） | 沒有 | ＋ hit-miss predictor、token allocator、rename table 和 window 的 token vector；沒 token 的 miss 還是要 flush |

---

## 13. 【實測】

Replay 發生在 CPU 裡面，從外面很難直接量，所以這一講用模擬。程式在 `experiments/lecture15/`。

`replay.py`：一個 load（猜會在 L1 命中、提早 broadcast），後面 64 條指令，每條 2 個來源，每個來源一半機會來自前面 8 條中的某一條、一半是早就好了的暫存器。每個 cycle 最多選 4 條（最老的先）、ALU 1 cycle、被選中就 broadcast。到 cycle 4（WV = 4）發現 load 沒命中。隨機產生 2000 次取平均：

```
K=64 條、每 cycle 選 W=4 條、WV = 4 cycle，平均 2000 次：
  到 cycle 4 為止已經 issue 的：16.0 條；其中真的在 load 的 forward slice 裡的：7.1 條（整個 forward slice 共 17.4 條）
  做法                       已做、要重做的    orphan
  flush（全部重來）                 16.0         0（而且 K 條全部要重新 fetch）
  non-selective                 10.9       4.2（被殺的運算元所屬指令共 13.0 條）
  delayed selective              7.1       1.1（orphan 靠 completion bus 救回）
  token                          7.1         0
```

- WV 裡已經做了 16 條，真的用到錯值的只有 **7.1 條**。flush 把 16 條全丟（還要重新 fetch 全部 64 條）。
- **Non-selective** 殺了 13 條、其中已做完的 10.9 條要重做，比 forward slice 多約 50%；而且平均有 **4.2 條 orphan**（[§6](#6-orphan-instruction) 的 xor：運算元被殺、producer 卻不會再 broadcast）。
- **Delayed selective** 已做的只重做有毒的，剛好等於 forward slice 的 7.1 條；orphan 降到 **1.1 條**（都是 WV 裡醒來、還沒 issue 的，[§7](#7-delayed-selective-replaypoison-bit) 表格的第二種）。
- **Token** 只動 forward slice，沒有 orphan。代價是 [§11](#11-token-based-replay) 那些額外的表和 bit。
- 這個模型很簡化（沒有真的 cache、所有 ALU 都是 1 cycle、只有一個猜錯的 load），數字只用來比較四種做法的**相對**大小。

---

## 14. 一頁速查表

| 主題 | 重點 |
|---|---|
| 為什麼不 flush | 清理要時間、浪費大量工作、多耗能量；「大砲打蚊子」 |
| Forward slice | 直接或間接用到某條指令結果的所有指令（DAG）；只有它要重做 |
| Squash | 把指令（運算元）從 ready 設回 not ready |
| WV | 推測的指令 broadcast（或被選中）後的 N 個 cycle；N = 預測的延遲 |
| Timer ＋ kill | 運算元醒來 timer = N，每 cycle −1；kill 且 timer ≠ 0 → ready = 0（NAND） |
| 重做的地方 | 留在 window（優先權高）或 replay queue（較常用） |
| Orphan | 被殺的運算元的 producer 早就走了 → 沒人叫醒；等到 ROB 頭直接執行 |
| Poison bit | 65-bit 資料路徑；讀到有毒 → 目的也有毒；有毒的完成時 squash |
| Completion bus | 成功的指令 N cycle 後再 broadcast 一次，救回它叫醒過的 orphan |
| WV 起點 | 從 producer broadcast 算最準；從 select 算不會錯，只是殺比較多 |
| 為什麼要 WV | 只有 poison bit 的話 forward slice 會一直長 |
| Token | 90/10 → hit-miss predictor；猜 miss 的拿 token；rename table 存 token vector、取聯集；成功就收回；沒 token 的 miss → flush |
| 實測（模擬） | 已做 16 條、forward slice 7.1 條：non-selective 重做 10.9＋4.2 orphan；delayed 7.1＋1.1；token 7.1＋0 |

---

## 15. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| 四種推測、hit-miss predictor、confidence | [第 14 支](lecture14_note.md) |
| Wakeup、broadcast、early broadcast | [第 11 支](lecture11_note.md) |
| Early broadcast 的時間、k cycle 後 broadcast | [第 12 支](lecture12_note.md) |
| ROB、flush、回復 | [第 13 支](lecture13_note.md) |
| 比較簡單的 OOO（沒有 register file）、compiler 技巧 | [第 16 支](lecture16_note.md) |
| Cache miss 的來源 | [第 21 支](lecture21_note.md)（Ch7） |

---

## 附：本講一句話總結

> **推測猜錯時每次都 flush 太浪費，所以只重做猜錯那條指令的 forward slice（直接或間接用到它的值的所有指令，是一個 DAG）：non-selective replay 給每個運算元一個 timer，在 window of vulnerability（推測延遲 N 個 cycle）裡醒過的運算元一遇到 kill wire 就設回 not ready，殺的是 forward slice 的超集合，會產生沒人叫醒的 orphan（等到 ROB 頭再直接執行）；delayed selective replay 再加一個隨值傳播的 poison bit，已經 issue 的只重做有毒的，沒 issue 的 orphan 用 completion bus 上的第二次 broadcast 救回，而 WV 用來阻止 forward slice 無限長大；token based replay 利用「90% 的 miss 來自 10% 的指令」，只給猜會 miss 的 load 一個 token，在 rename table 裡沿 forward slice 取聯集傳下去，猜錯就只重做有這個 token 的指令、沒事就收回 token，沒有 token 的意外 miss 才 flush。本機模擬：WV 裡已做的 16 條中只有 7.1 條真的用到錯值，non-selective 要重做 10.9 條還留下 4.2 條 orphan，delayed selective 和 token 都只重做 7.1 條，orphan 分別是 1.1 和 0。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 5: Alternative Approaches to Issue and Commit (Part II) (Instruction Replay) 的 whisper 轉錄稿（本機 `large-v3-turbo`，1:14:23）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch5 §5.2（p.162–173）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）模擬結果。*
