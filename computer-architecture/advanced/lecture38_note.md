# Advanced Computer Architecture 第三十八支影片完整筆記：Multicore（七）：memory model 的分類與 data race（Chapter 9, Part VII）

> **課程**：Advanced Computer Architecture — Chapter 9: Multicore Systems (Part VII) -- Memory Models and Data Races（播放清單第 38 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 的字幕是亂翻的機器翻譯，不能用）（1:16:37）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch9** §9.5–9.6.4（p.489–504）
> **章號**：影片 Chapter 9 = 課本 Ch9。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 把「SO edge」寫成「SOH」，筆記直接改正成 so 邊。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 回顧：哪些順序可以放掉](#1-回顧哪些順序可以放掉)
- [2. 放掉 rfi：先讀到自己的寫](#2-放掉-rfi先讀到自己的寫)
- [3. 放掉 rfe：先讀到別人的寫](#3-放掉-rfe先讀到別人的寫)
- [4. 放掉 W→W](#4-放掉-ww)
- [5. 放掉 R→R](#5-放掉-rr)
- [6. 放掉 R→W](#6-放掉-rw)
- [7. 特例：SC 也可以放掉 rfi](#7-特例sc-也可以放掉-rfi)
- [8. 常見的 memory model](#8-常見的-memory-model)
- [9. 誰需要懂 memory model](#9-誰需要懂-memory-model)
- [10. Data race](#10-data-race)
- [11. DRF 的三個定理](#11-drf-的三個定理)
- [12. Properly synchronized 程式與語言的 memory model](#12-properly-synchronized-程式與語言的-memory-model)
- [13. 【原始碼】Linux 的 READ_ONCE／WRITE_ONCE](#13-原始碼linux-的-read_oncewrite_once)
- [14. 【實測】](#14-實測)
- [15. 一頁速查表](#15-一頁速查表)
- [16. 與後續講次的連結](#16-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
ws、fr 永遠 global（PLSC）；能放掉的只有 rf（rfi、rfe）和 po（WR、WW、RR、RW）
  rfi：LSQ forwarding、write buffer → 幾乎每個 model 都放掉
  rfe：write 不 atomic（tile 裡共用 bus、MSHR）
  W→R：OOO 的 load 越過 store → 幾乎都放掉
  W→W：non-blocking cache 的 MSHR → 很多 model 放掉（x86 不放）
  R→R：LSQ 的相依預測 → 很多 model 放掉（x86 不放）
  R→W：最難放掉（要提早寫）
  SC 放掉 rfi 也還是 SC
model：SC → TSO（x86）→ PC → PSO → weak → RC → PowerPC、ARM（全放）
data race：一般變數上兩個衝突的存取之間沒有經過 so 邊的路徑
  SC ⇏ DRF；DRF ⇒ SC（不管 memory model）；有 race ⇒ 有一個 SC 的 execution 也有 race
properly synchronized：每個共用變數都用同一組鎖保護 → DRF → 看起來就是 SC
```

---

## 1. 回顧：哪些順序可以放掉

[00:00]「最後用 data race 收尾。data race 和 memory model 關係很密切。」

- **memory model**（memory consistency model）：規定一段程式**允許**哪些 execution、哪些結果 valid。
- [01:06] 依處理器架構而定，但也可以替 virtual machine、軟體、compiler 定義。
- 最佳化越激進，memory model 越**弱**；最強的是 **SC**（黃金標準）。

[01:43] 已知：

| | |
|---|---|
| ws、fr | **global**：PLSC 的直接結果；所有流行的架構都靠 coherence 守住 PLSC |
| thin air read | 很多架構禁止（value prediction 加幾個 pipeline 最佳化就可能發生，[第 35 支 §12](lecture35_note.md#12-thin-air-read)） |
| **rf**、**po** | 可以變成 local：**真正能改的只有它們** |

[02:28]

| | 白話 |
|---|---|
| **rfi** | **先讀到自己的寫**（read your own write early） |
| **rfe** | **先讀到別人的寫**（read others' write early）= write 不 atomic |
| po | 每一種都可以放掉（relaxed = 不遵守） |

[03:05]【口頭】「建議讀者去書裡仔細看這些例子。」

【課本補充】課本 p.489 Way Point 12；式 9.10：**MM = gpo ∪ ws ∪ fr ∪ grf**（p.496）。課本 p.490 說明「一個順序被 **respected** = 它是 global 的」。

（本機用第 35 支的找環程式，照課本表 9.4 把下面每個例子都跑過一遍，見 [§14.1](#141-照表-94-判斷課本的六個例子)。）

---

## 2. 放掉 rfi：先讀到自己的寫

[03:05] 最常被放掉的是「寫 → 讀」：OOO pipeline、LSQ、forwarding、write buffer 的直接結果。寫在 commit 時才真的做，讀早就做了 → 讀比寫早被別的處理器看到。

[04:26] 例子（課本圖 9.52(a)）：

```
T1：(a) x = 1; (b) t1 = x; (c) y = 1      T2：(d) t2 = y; (e) t3 = x
結果：t1 = 1、t2 = 1、t3 = 0？
```

- [05:13] 很多架構上 core 會**比別的 core 早**讀到自己的寫：t1 = 1。T2 去讀 x 時拿到舊值 0 → Rx0 →fr Wx1。
- [06:00] 邊：Wx1 →rfi Rx1 →po Wy1 →rfe Ry1 →po Rx0 →fr Wx1。write atomic（rfe global）、R→W、R→R 都守住、fr 本來就 global → 只能放掉 **rfi**。
- rfi 是 global 的話有環 → 不允許；但 OOO pipeline 上 rfi 通常**不是** global → 允許。
- [06:45]「這是合理的：我們應該可以先讀到自己的寫」：LSQ forwarding、write buffer 給的。load 在 critical path 上，越早做完越好。
- [07:23]「幾乎所有有一點 OOO 的架構都會接受這個 execution。」

⚠️ 這個說法對 **x86（TSO）不成立**：這個例子還有一條沒畫出來的 **Wx1 →po(W→W) Wy1**。x86 守住 W→W（[§8](#8-常見的-memory-model)），所以 Wx1 → Wy1 → Ry1 → Rx0 → Wx1 還是一個環，只放掉 rfi 不夠。本機的找環程式說 TSO 不允許、PSO 才允許；本機 x86 跑 100 萬次也一次都沒出現（[§14.1](#141-照表-94-判斷課本的六個例子)、[§14.2](#142-本機-x86-跑三個新例子)）。課本 p.491 的假設是「沒有寫 → 讀的 po 邊」，也沒提到 W→W。

---

## 3. 放掉 rfe：先讀到別人的寫

[07:23] write 不 atomic 怎麼發生：

- [08:15] 晶片切成 **tile**（core ＋ cache bank）；同一個 tile 裡的另一個 core 可能**先**讀到它的寫，別的 core 還沒看到。尤其是它們共用一條 snoopy bus 的時候。
- 共用 MSHR 之類的結構時也會：一個 core 先看到寫，就依它做決定。

[09:03] 例子（課本圖 9.52(b)，就是 [第 34 支 §7](lecture34_note.md#7-打破-atomicity-的例子) 的三個 thread 例子）：

```
T1：x = 1      T2：t1 = x（1）; y = 1      T3：t2 = y（1）; t3 = x（0）
```

- write atomic 的話會讀到 1；讀到 0 → 加 Rx0 →fr Wx1 → 有環。
- [09:50] write 不 atomic 時沒有那條 rfe 邊 → 沒有環 → 允許。
- **不同位置**的寫被看到的順序不一樣：T2 看到 x 先、y 後；T3 看到 y 先、x 後：「write 不 atomic 的招牌性質」。
- [10:38] 但**同一個位置**的寫還是大家順序一樣（PLSC）。

---

## 4. 放掉 W→W

[10:38] **non-blocking cache**：第一個寫卡在 **MSHR**，第二個寫因為各種原因先做了，或繞過 MSHR 送到下一層 → 別的 core 先看到第二個寫。同一個 thread、不同位址的寫的順序就被打破了。

[11:54] 例子（課本圖 9.54，MP）：

```
T1：x = 1; y = 1      T2：t1 = y（1）; t2 = x（0）
```

- 假設 write atomic、R→R 守住。Wx1 →po Wy1 →rfe Ry1 →po Rx0 →fr Wx1 → 有環。
- [12:39] 沒有 W→W 那條邊 → 沒有環 → 允許。T1 看到「先 x 後 y」，T2 看到反過來。

[13:20] 所以**多個位址**時，不加 fence 別人看不到正確的資料。[14:07] 常見的傳值寫法：

```
T1：result = 57; status = 1         T2：if (status == 1) temp = result
```

[15:04] 兩個寫可能被**對調** → `temp` 不保證是 57 → 要在中間加 **fence**。

【課本補充】課本 p.491–492：W→W 被打破的原因還有：write buffer 裡已經有 W1 位址的 entry、W2 沒有，W2 可以直接送記憶體；NoC 裡寫的訊息被重排。一定能守住的辦法：寫變成全域可見後回 **acknowledgement**，W1 的 ack 回來才送 W2，但寫會變貴、擋住 pipeline、降低 IPC。「大部分現在的 weak memory model 都不保證 W→W，**Intel TSO 是例外**。」

---

## 5. 放掉 R→R

[15:04]「很容易打破」：兩個讀不同位址的 load，在有 LSQ 的 OOO 處理器上「總是被放掉」。

[15:55] 例：LSQ 有相依預測器（[第 14 支](lecture14_note.md)），預測讀 x 會撞到前面的 store → 卡住；預測讀 y 不會 → 先送出 → 讀的順序就反了。「第 5 章就看過了。」

[16:41] 例子（課本圖 9.55）：

```
T1：t1 = x（1）; t2 = y（0）      T2：y = 1      T3：t3 = y（1）; x = 1
```

- 邊：Rx1 →po Ry0 →fr Wy1 →rfe Ry1 →po Wx1 →rfe Rx1 → 有環。
- [17:36] 讀 y 先做、讀到舊的 0；x 之後回傳 1 → 沒有 R→R 那條邊就允許。
- [18:29] **Intel 處理器守住 R→R** → 不允許；其他大部分 OOO 處理器沒有這條邊 → 允許。

【課本補充】課本 p.492：要守住 R→R 只能**照程式順序**發 load：後面 load 的位址比較早算出來也要等，「高效能處理器不能接受」。

---

## 6. 放掉 R→W

[18:29]「其實很難打破」：要讓讀在寫**之後**才做。但寫要到 ROB 最前面才做，那時讀早就拿到值、離開 pipeline 了。

[19:40] 可能的情況：

- **投機的寫**：激進的最佳化，指令還沒到 pipeline 尾端就寫。
- 兩個硬體 thread **共用同一個 LSQ**（存實體位址）：寫可以提早被看見。

[20:23] 例子（課本圖 9.56，LB）：

```
T1：t1 = x（1）; y = 1      T2：t2 = y（1）; x = 1
```

- 兩邊的寫都在**後面**，兩個讀都讀到 1 → 兩條 po、兩條 rfe → 有環。
- [21:05] 簡單的處理器守住 R→W，看不到這個結果；寫可以提早被看見的激進實作就看得到。
- [21:43]「最難打破的順序」：違反 OOO pipeline 教的一切。

【課本補充】課本 p.493：另一個情況是 **load value prediction**，在 load commit 之後才驗證。

---

## 7. 特例：SC 也可以放掉 rfi

[21:43] 一直說「SC 所有關係都是 global」。問題：只放掉 rfi、其他都 global，**SC 會壞嗎**？

[22:34] **不會**。證明在書裡，大意：

- 一個寫 W、之後同一個 thread 讀到它的讀 R。另一個 core 看到 R 的效果，再繞一圈回到 W → 才會有環。
- [23:37] 但別的 thread 要看到 R 的效果，路徑要嘛經過 R（R 之後只會接到同位址的寫：R →fr W′，而 W → W′ 也成立），要嘛經過同一個 thread 的其他指令（W →po 那些指令）→ W 一定排在它們前面 → 沒有環。
- [24:10] 別的 core **分不出來** rfi 有沒有被放掉。

【課本補充】課本 p.493–494 §9.5.6：W 和 R 之間如果有別的操作 O1…On，W → O1 → … → R 本來就是全域順序，rfi 是不是 global 都沒差；只有 W、R **緊鄰**時才有關係，而那時別的 thread 的第一個看得到 R 的操作 O，也一定看得到 W。結論：「在 SC 裡，thread 可以**提早**讀到自己的寫。」所以 SC 也能用 LSQ forwarding。

---

## 8. 常見的 memory model

[24:43]（課本表 9.4，取自 Adve and Gharachorloo, 1996；✓ = **放掉**）

| Model | poWR | poWW | poRR | poRW | rfe | rfi |
|---|---|---|---|---|---|---|
| **SC** | | | | | | ✓ |
| **TSO** | ✓ | | | | | ✓ |
| **PC** | ✓ | | | | ✓ | ✓ |
| **PSO** | ✓ | ✓ | | | | ✓ |
| **Weak** | ✓ | ✓ | ✓ | ✓ | | ✓ |
| **RC** | ✓ | ✓ | ✓ | ✓ | | ✓ |
| **PowerPC** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **ARM**（v7） | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |

（已看過 PDF p.494 的表格圖，跟上表一致。）

| Model | 說明 |
|---|---|
| **SC** | [25:09] 最上面，什麼都不放（除了 rfi，[§7](#7-特例sc-也可以放掉-rfi)）；write atomic |
| **TSO**（total store order） | [25:09] **Intel** 的 memory model：只放掉 W→R（OOO pipeline 一定要放）和 rfi（[25:51]「所有 model 都放掉 rfi，不然 OOO 的好處就沒了」） |
| **PC**（processor consistency） | [25:51] 在分散式系統也很流行：跟 TSO 一樣，再加上 **write 不 atomic**；但守住 W→W：「一個處理器即使 write 不 atomic，也守住它**自己**的寫的順序」，名字由此而來 |
| **PSO**（partial store order） | [26:31] write atomic，但不守 W→W（non-blocking cache 等） |
| **Weak ordering** | [27:13] 什麼 po 都不守，但 write atomic；有一般的 **fence** |
| **RC**（release consistency） | [27:13] 跟 weak 一樣，但有一對 fence：**acquire** 和 **release**（[第 34 支 §13](lecture34_note.md#13-acquire-與-release)） |
| **IBM**（PowerPC）、**ARM** | [28:56]「異類」：write 不 atomic、什麼 po 都不守；但 fence 支援很完整：store 之間、load 之間……各種 fence → [29:34]「才能寫出相當有效率的程式碼」 |

RC 的 acquire／release：

- [27:47] acquire：拿到鎖之後 critical section 的指令才能執行。
- [28:22] release：critical section 裡所有指令（包括 acquire）都做完了，告訴外面「可以讀它們的資料了」。
- [28:56] 跟一般 fence 不同：release **不禁止**後面的指令在它之前完成，「允許那麼一點亂序」。

⚠️ [27:13] 老師先說「RELEASE 和 ACQUIRE」，馬上改口「應該是 ACQUIRE 再 RELEASE」。

【課本補充】課本 p.495–496：

- memory model 不一定是硬體的產物：新聞底下的留言，先看到回覆、後看到被回覆的留言，就是軟體世界的 consistency 問題。**JVM** 也要實作 memory model；compiler 的最佳化也會重排指令。「從 compiler、virtual machine 到硬體，任何一層都可能重排。」
- 主流處理器幾乎都不支援 SC，唯一的例外是 **MIPS R10000**。
- **x86 和 Sun SPARC v8** 大致像 TSO（Alglave, 2012）；PSO 在一些 SPARC v8、v9 上用過。
- PSO 為什麼放掉 W→W：讀是**同步**的（core 知道值什麼時候回來），寫天生是**非同步**的（沒有 ack 就不知道寫什麼時候生效）→ 很難讓寫跟別的操作排順序。
- 所有 model 都守 `synch → (read | write | synch)` 和 `(read | write | synch) → synch`；RC 再加 `acquire → (read | write | synch)`、`(read | write | synch) → release`。
- 「放得太多不一定好」：軟體要插很多 fence，又影響效能。

（本機的 x86 照 TSO 該禁止的都沒出現；照課本表 9.4 判斷六個例子，見 [§14.1](#141-照表-94-判斷課本的六個例子)。）

---

## 9. 誰需要懂 memory model

[30:03] **每個寫多執行緒程式的人**：不懂就會假設系統照某種方式運作，但處理器內部會重排。

- [30:29] 一段有 bug 的程式可能某一台機器上正常、另一天就不正常，產生程式設計師解釋不了的結果。
- [30:29] library、OS、compiler 的作者，寫低階程式碼的人：要懂得非常細，**目的是減少 fence 的數量**。
- [31:07] 「減少 fence 的數量」本身是**不可判定**（undecidable）的問題，非常難；有工具可以建議哪裡不用 fence，它自己就是一個研究題目。
- [31:46]「**黑帶程式設計師**」知道哪裡要 fence、哪裡不用。

[32:22] 但不能期待大部分程式設計師都是黑帶。【口頭】一般程式設計師的心聲：「memory model、coherence、SC、RC、PC 我都懂，但說到底**我不在乎**。我要寫一段在**所有機器**上都能跑的平行程式，ARM 上寫的在 Intel 上也要能跑；慢一點、多幾個 fence 都沒關係，我要的是**簡單**和**可攜**。」

---

## 10. Data race

### 10.1 counter++

[33:33] `counter++` 在 RISC ISA 上是三個指令：讀、加、寫 → 多個 thread 一起跑會錯。

| 做法 | |
|---|---|
| **atomic 指令**（fetch-and-increment） | [34:43] 簡單的情況可以用：看起來瞬間完成 |
| **critical section**（lock、unlock） | [35:16] 邏輯複雜時用；同一個 counter 在別處用到也要用**同一個**鎖變數 |

### 10.2 什麼時候需要鎖

[37:17] 兩個哲學問題：

1. 沒有共用變數，還需要鎖嗎？**不需要**。bug 只因為 counter 是共用的（兩個人都讀到 100、都寫 101，最後應該是 102）。
2. [38:16] 什麼時候需要？兩段程式碼對同一個位址做**衝突**（conflicting）而且**同時**（concurrent）的存取時。

| 名詞 | 定義 |
|---|---|
| **衝突**（conflicting） | [38:16] 同一個位址的一對存取，**至少一個是寫**（兩個讀不會有正確性問題） |
| **同時**（concurrent） | [39:42] 常識的意思是「同一時間」，「但可以定義得更好」→ [§10.4](#104-data-race-的定義) |

### 10.3 兩個 execution witness 的比較

[40:22]（課本圖 9.57(a)(b)）兩個 thread 都做 x++，都讀到 0、都寫 1：

```
T1：Rx0 ─po→ Wx1         T2：Rx0 ─po→ Wx1
兩個 Rx0 都 ─fr→ 對方的 Wx1；兩個 Wx1 之間 ws
```

[40:49] **沒有環**，而且**是 SC**（排得出來）。[41:33]「就算是 SC 我們也不開心：x 最後應該是 2，不是 1。」→ execution witness 需要多一個工具。

[42:04]（課本圖 9.57(c)(d)）從 T1 傳一個值給 T2，**y 是 sync 變數**（只用 atomic 指令更新）：

```
T1：x = 1; y = 1              T2：while (y != 1) {}; t1 = x
Wx1 ─po→ Wy1 ─so（也是 rf）→ Ry1 ─po→ Rx1
```

- [43:48] y 是 sync 變數，在**任何** memory model 裡 Wx1 → Wy1 都成立；sync 操作之間是 SC → Wy1 →so Ry1；Ry1 →po Rx1。
- [44:23]「不管 rf 是不是 global，這時一定讀到 x = 1」：跟 memory model 無關。
- [44:55]「**祕方**」：從 Wx1 到 Rx1 有一條**經過 so 邊**的路徑。

[46:09] so 邊抓住的是**程式設計師的意圖**：她希望存取 sync 變數 y 之前，前面的存取都做完；另一邊的 sync 存取一定在 y = 1 之後；之後的動作再在那之後。[47:23] 衝突的存取被**程式設計師插的指令**控制著。

### 10.4 Data race 的定義

[45:27]、[47:56] 兩個存取之間，在 execution witness 裡**沒有一條經過 so 邊的路徑** → 它們是 **concurrent** 的。

> **Data race**：同一個**一般**（非 sync）變數上，一對**衝突**而且 **concurrent** 的存取。（課本 Definition 84）

- [48:27] 不經過任何程式設計師指定的結構（lock、unlock）→ 換一個 execution 順序就可能反過來，依賴這個順序的其他變數也不一定跟著反映。
- [49:11] critical section 是 so 邊的一個例子：unlock 之後另一個 thread 進來，前面所有存取都嚴格地在後面所有存取之前。例：一邊設 x = 1、y = 1，另一邊設 x = 2、y = 2；別的 thread 拿了鎖才能存取，所以**永遠看到 x = y**，看不到一半的狀態。
- [50:36] counter 的例子就是 data race：沒有 so 邊，最後是 1 不是 2。
- [51:23]「data race 是壞東西：它們就是 **bug**。」

[52:06] 兩個概念：**execution 裡**的 data race 和**程式裡**的 data race，「意思不同，但美妙地連在一起」。

- [52:44] execution 的 data race：建 execution witness，看所有一般變數上的衝突存取對是不是 concurrent。同一個程式有很多 execution，有的有 race、有的沒有。
- [53:27] 程式的 data race：要更多理論工具。execution witness 的邊屬於某個 memory model，「你會以為 data race 跟 memory model 有關，**其實沒有**。」

【課本補充】課本 p.497 Definition 83（conflicting）；p.498：電腦架構裡的「concurrent」**不是**「同一時間」的意思。課本 p.499：sync 變數之間允許衝突又 concurrent 的存取，因為假設它們的更新是 SC 的；但一般變數之間只要有 rf、fr、ws 邊跨 thread，就一定要有另一條經過 so 邊的路徑。

（本機看 data race 會發生什麼事，見 [§14.4](#144-data-race-讓-compiler-把程式改壞)。）

---

## 11. DRF 的三個定理

### 11.1 SC ⇏ DRF

[55:50]（課本圖 9.58）同一段程式（S1、S2 是 sync 操作）的兩個 SC execution：

| Execution | 順序 | |
|---|---|---|
| 1 | Rx0、S1、S2、Wx1 | Rx0 和 Wx1 之間有 so 邊 → **沒有** race |
| 2 | S2、Wx1、Rx1、S1 | Wx1 和 Rx1 之間**沒有** so 邊 → **有** race |

[56:52] 兩個都是 legal 的 sequential execution，但第二個有 race → **SC 不保證 data-race-freedom**。[57:22]「data race freedom 看起來比 SC 更強。」

### 11.2 DRF ⇒ SC

[57:46] 假設程式的**所有** execution 都沒有 data race，不管 memory model 就跑它 → [58:27]「**所有** execution 都是 SC 的。」

【口頭】「這是我認為**整個電腦架構最重要的理論結果**（important 的 I 要大寫）。有人可能不同意。」

[59:07] 證明大意（課本 Theorem 9.6.2.2）：

1. DRF → 跨 thread 的每對衝突存取之間，至少有一條路徑有 so 邊。
2. [59:34] 不管 memory model，把 SC 的邊（所有 po、rfe）全加進 execution witness。沒有環 → 是 SC。
3. [60:07] 假設有環 → 可以證明會有一個**只由 sync 操作組成**的 happens-before 環 S1 → S2 → … → S1。
4. [60:40] 不可能：sync 操作本身是 SC 的（有 fence → program order；atomic 指令 → atomicity）→ 矛盾。

[61:24] 意義：**memory model 不重要**。

- [62:14] 每對衝突的存取都經過 so 邊（例如放進 critical section）→ 不管底層的 model，執行起來**永遠是 SC**。
- [62:55] memory model 交給架構師，**要多弱就多弱**，換效能；程式只要加夠多 fence、atomic 指令（越少越好，它們很貴），避開 data race 就好。
- [62:55] 原本就想把平行 execution 等價成循序的；用 execution witness 做不到 legal，[63:44] 但 DRF 的話就能等價成 **legal** 的 sequential execution。「人類喜歡循序地想，不喜歡平行地想。」

【課本補充】課本 p.500 Theorem 9.6.2.1：DRF 的 execution 裡，e1 → e2（不同 thread T1、T2）就一定有 sync 操作 s1 ∈ T1、s2 ∈ T2，使得 e1 → s1 → s2 → e2，而且 e1 → s1、s2 → e2 各有一條 po 邊。

### 11.3 有 race ⇒ 有一個 SC 的 execution 也有 race

[64:21] 第二個定理（課本 Theorem 9.6.2.3）：**程式有 data race，就能做出一個也有 data race 的 SC execution。**

- [64:57]【口頭】「跟數學不同，電腦架構裡定理很少，少數幾個都相當有威力。」
- [65:54] 取**逆否命題**（contrapositive）：**沒有任何 SC execution 有 data race ⇒ 程式沒有 data race**。
- [65:54] 一個程式可能有十億個 SC execution，人做不到，**自動化工具**做得到。
- [66:31]【口頭】「這很重要，我不介意重複二十遍。」
- [67:38] data race 也就變成**程式**的性質。
- [68:51] 有 race 時可能 100 次都沒問題，第 101 或 1001 次才出事 → 要工具：列舉 SC 的 execution，在裡面找 data race（[第 39 支](lecture39_note.md)）。

[69:59] 總結：

| 性質 | 根據 |
|---|---|
| SC ⇏ DRF | 圖 9.58 |
| **DRF ⇒ SC**（不管 memory model） | Theorem 9.6.2.2 |
| 不是 SC 的 execution ⇒ 有 data race | 上一條的逆否命題 |
| 程式有 race ⇒ 有一個 SC execution 也有 race | Theorem 9.6.2.3 |

【課本補充】課本 p.501–502 證明 Theorem 9.6.2.3：一直跑到出現**第一個** race 的操作 ej，之前的部分 DRF → 是 SC；再證明加上 ej 也不會有環（ej 是寫：之前沒人讀過它，當它是最新的寫；ej 是讀：環會不經過 ej 就存在，矛盾）；再把略過的指令照程式順序補進去，每一步都維持 SC。

---

## 12. Properly synchronized 程式與語言的 memory model

[70:35] 寓意：沒有 data race 就好：寫出來的就是 SC 的程式。代價：到處加了 sync 指令，效能下降，要盡量少。

[71:08] **程式語言也定義 memory model**：例如 **C++11、C++17**，也定義了 sync 的基本操作，告訴程式設計師怎麼寫出沒有 data race 的程式碼。[71:36] C++、Java……幾乎所有現代語言都有：這是**軟體的 memory model**。

[72:09] **Properly synchronized**（PS）程式：DRF 的一種寫法：

- 存取共用變數（critical section）前拿鎖，做完放鎖。
- 同一個變數**永遠用同一組鎖**保護：不會有兩把不同的鎖保護同一個變數。
- [72:44] → 每次存取前都要拿到那些鎖 → 有嚴格的 so 邊 → **properly synchronized → DRF** → 沒有衝突又 concurrent 的存取。

[73:17] 兩種人：

| | |
|---|---|
| **低階**（OS、library 作者） | [73:48] 在乎效能，不想有很多 fence（總要有人寫鎖的程式碼；少幾個 sync 指令，鎖就快很多）→ 要懂 memory model |
| **高階**程式設計師 | [74:14] 用 data race 的觀念寫 PS 程式碼：所有共用變數包在 critical section 裡 → 不管底層 model，到處都是 SC |

[74:37] 代價：效能比較低（很多像 fence 的慢指令）；手工打造的 library 用底層 model 的特性建立相依，sync 指令很少。**簡單、好寫 vs 效率**的取捨。

[75:18]【口頭】OpenMP 的 directive 裡也包了 critical section，程式設計師通常看不到。[76:01] 下一講更簡單的模型：**transactional memory**，天生 DRF，把負擔從程式設計師移到系統軟體或硬體。

【課本補充】課本 p.502–504：

- Definition 85：PS 程式。「這可以說是現代平行計算和平行架構**最有影響力**的結果之一」：硬體設計者可以做各種效能最佳化，高階程式碼仍然直覺。
- 大部分平行程式存取共用變數的比例不高，大部分是私有資料，所以 properly synchronize 的開銷「相當溫和」。
- 判斷一個程式是不是 properly synchronized 是**不可判定**的；但可以找 data race 的證據：找到就知道它不是，還能指出位址；幾次測試沒找到不代表它是。
- §9.6.4 **DRF memory model**：在 C++、Java 這種語言層級定義的 model；sync 操作不一定是一般的 fence，s1 是 release、s2 是 acquire 也可以，定理一樣成立。

---

## 13. 【原始碼】Linux 的 READ_ONCE／WRITE_ONCE

[§14.4](#144-data-race-讓-compiler-把程式改壞) 會看到：一般變數上的 data race 讓 compiler 把 `while (y != 1) {}` 改成只讀一次 y 的無窮迴圈。Linux 核心自己的同步程式碼用 `READ_ONCE`／`WRITE_ONCE` 擋住這種 compiler 改寫。`include/asm-generic/rwonce.h:44-62`：

```c
#define __READ_ONCE(x)	(*(const volatile __unqual_scalar_typeof(x) *)&(x))
#endif

#define READ_ONCE(x)							\
({									\
	compiletime_assert_rwonce_type(x);				\
	__READ_ONCE(x);							\
})

#define __WRITE_ONCE(x, val)						\
do {									\
	*(volatile typeof(x) *)&(x) = (val);				\
} while (0)

#define WRITE_ONCE(x, val)						\
do {									\
	compiletime_assert_rwonce_type(x);				\
	__WRITE_ONCE(x, val);						\
} while (0)
```

- 做法就是透過 `volatile` 指標存取：每次都真的讀寫記憶體，不能合併、不能重讀。
- 檔頭的註解（`include/asm-generic/rwonce.h:3-18`）寫得很清楚：它防止 compiler **合併或重新讀取**；只有在 compiler 知道順序時才不重排連續的 READ_ONCE、WRITE_ONCE；**硬體**的順序要靠「明確的 memory barrier 或 atomic 指令」。
- 也就是說：`READ_ONCE` 只管 compiler 那一層，硬體那一層（[§8](#8-常見的-memory-model) 的表）還是要 fence 或 acquire／release（[第 37 支 §14](lecture37_note.md#14-原始碼linux-的-spinlock-與-mutex) 的 `smp_store_release`）。課本 p.495 說的「從 compiler 到硬體每一層都可能重排」在 Linux 裡就是分成這兩種工具。

---

## 14. 【實測】

程式在 `experiments/lecture38/`：`models.py`（照表 9.4 判斷）、`litmus4.c`（x86 實跑）、`race.c`（data race）。

### 14.1 照表 9.4 判斷課本的六個例子

`models.py` 用 [第 35 支](lecture35_note.md#151-execution-witness-檢查程式) 的找環程式，照表 9.4 設定八個 model（放掉的 po 不加邊；rfe 被放掉就不加 rfe 邊），判斷 §9.5 的六個例子（「✗」= 一定有環 = 不允許）：

| 例子 | SC | TSO | PC | PSO | Weak | RC | PowerPC | ARM | 課本：要放掉 | 本機 x86 |
|---|---|---|---|---|---|---|---|---|---|---|
| 圖 9.52(a)（[§2](#2-放掉-rfi先讀到自己的寫)） | ✗ | **✗** | 可行 | **可行** | 可行 | 可行 | 可行 | 可行 | rfi | **沒看過**（[§14.2](#142-本機-x86-跑三個新例子)） |
| 圖 9.52(b)（WRC） | ✗ | ✗ | 可行 | ✗ | 可行 | 可行 | 可行 | 可行 | rfe | 沒看過（[第 34 支](lecture34_note.md#142-本機的-x86write-是-atomic-的嗎)） |
| 圖 9.53（SB） | ✗ | **可行** | 可行 | 可行 | 可行 | 可行 | 可行 | 可行 | poWR | **看過**（[第 33 支](lecture33_note.md)） |
| 圖 9.54（MP） | ✗ | ✗ | 可行 | 可行 | 可行 | 可行 | 可行 | 可行 | poWW | 沒看過（第 33 支） |
| 圖 9.55 | ✗ | ✗ | 可行 | ✗ | 可行 | 可行 | 可行 | 可行 | poRR | 沒看過（[§14.2](#142-本機-x86-跑三個新例子)） |
| 圖 9.56（LB） | ✗ | ✗ | 可行 | ✗ | 可行 | 可行 | 可行 | 可行 | poRW | 沒看過（§14.2） |

- 每個例子在 SC 都不允許；在 PowerPC、ARM 都允許：一個是最強、一個是最弱。
- **TSO 那一欄跟本機 x86 看到的完全吻合**：只有 SB 可行，SB 也真的看過；其他五個都沒看過。
- 圖 9.52(a)：TSO 已經放掉 rfi，**還是不可行**：因為 TSO 守住 W→W（[§2](#2-放掉-rfi先讀到自己的寫) 的 ⚠️）；要到 PSO（再放掉 W→W）才可行。
- ⚠️ PC 在這裡讓 MP、圖 9.55、LB 都變成可行：因為我把「rfe 被放掉」做成**整個不加 rfe 邊**，這是最極端的讀法。真實的 PC 在這些例子上允不允許，本機沒有可以查證的資料，**待補**。

### 14.2 本機 x86 跑三個新例子

`litmus4.c`（寫法同第 34 支的 `litmus3.c`：thread 綁在 cpu2、4、6，每一輪用新的變數），各 100 萬次，跑兩次：

| 例子 | 問的結果 | 「差一點」的輪 | 問的結果出現 |
|---|---|---|---|
| 圖 9.52(a) | t1 = 1、t2 = 1、t3 = 0 | 469091／293742（T2 讀到 y = 1） | **0／0** |
| 圖 9.55 | t1 = 1、t2 = 0、t3 = 1 | 31922／81466（t1 = 1 且 t3 = 1） | **0／0** |
| 圖 9.56（LB） | t1 = 1、t2 = 1 | 466395／403905（至少一個讀到 1） | **0／0** |

- 「差一點」的輪很多（thread 確實在同時跑、確實看到了對方的寫），但問的結果**從來沒出現**：x86 守住了 W→W、R→R、R→W，跟影片 [18:29]「Intel 守住 R→R」、課本 p.492「Intel TSO 守住 W→W」一致。
- 加上第 33、34 支的結果：本機 x86 在 8 個經典例子裡**只**打破了 SB 一種（W→R），跟 TSO 一樣。

### 14.3 C 的 release／acquire 在 x86 上變成什麼

`race.c` 的 ATOMIC 版本用 `__atomic_store_n(&y, 1, __ATOMIC_RELEASE)` 和 `__atomic_load_n(&y, __ATOMIC_ACQUIRE)`。`gcc -O2 -S` 出來的 producer：

```
producer:
	movq	$57, x(%rip)
	movq	%rdi, %rax
	movl	$1, y(%rip)        ← release store：一般的 mov，沒有 fence
	ret
```

- 語言層級的 release／acquire（[§12](#12-properly-synchronized-程式與語言的-memory-model) 的 DRF model）到了 x86 **不用任何 fence**：TSO 本來就守 W→W、R→R、R→W（[§14.2](#142-本機-x86-跑三個新例子)）。
- 同樣的 release 在 ARM 上要 `stlr`、RISC-V 上要 `fence rw,w`（[第 37 支 §15.1](lecture37_note.md#151-atomic-操作編譯成什麼指令)）：同一份 DRF 的程式，每個平台由 compiler 補上需要的東西。這就是 [33:33]「ARM 上寫的在 Intel 上也要能跑」的實現方式。

### 14.4 data race 讓 compiler 把程式改壞

`race.c` 是課本圖 9.57 的兩個程式：兩個 thread 各做 `counter++` 一百萬次（`counter` 用 `volatile`，不然 compiler 會把迴圈合成一次加法）；再來是 `x = 57; y = 1` ‖ `while (y != 1) {}; t = x`。

各跑三次（`timeout 3` 秒）：

| 版本 | counter（應該是 2000000） | 傳值 |
|---|---|---|
| **PLAIN**：y 是一般的 int | 1057312、1008498、1072564 | 1 次印出 57；**2 次卡住**，3 秒後被砍掉 |
| **ATOMIC**：y 用 release／acquire | 1061172、1031036、1006509 | 3 次都印出 57 |

PLAIN 版 consumer 的組語：

```
consumer:
	movl	y(%rip), %eax      ← 迴圈外只讀一次 y
	...
.L5:
	cmpl	$1, %eax
	jne	.L5                ← 迴圈裡只比較暫存器：y 永遠不會再讀
```

ATOMIC 版把 `movl y(%rip), %eax` 放在 `.L5:` **裡面**，每圈都重讀。

- **counter 少掉約一半**：兩個版本都一樣（counter 本身就有 race）。
- **PLAIN 版的旗子變成無窮迴圈**：一般變數上的 data race，compiler 可以假設沒有別的 thread 會改 y，就把讀 y 提到迴圈外。在 consumer 先開始轉的那幾次，它永遠看不到 y = 1。這不是硬體重排，x86 的 TSO 也救不了：課本 p.495「compiler 也會重排」。
- 把 y 換成 sync 變數（atomic），這一對存取就沒有 race，結果正確，而且組語裡**沒有**多任何 fence（[§14.3](#143-c-的-releaseacquire-在-x86-上變成什麼)）。

用 **ThreadSanitizer**（`gcc -O1 -g -fsanitize=thread`）跑：

```
ATOMIC 版：SUMMARY: ThreadSanitizer: data race .../race.c:18 in inc
PLAIN 版： SUMMARY: ThreadSanitizer: data race .../race.c:18 in inc
           SUMMARY: ThreadSanitizer: data race .../race.c:19 in producer
```

- `race.c:18` 是 `counter++` 那一行；`race.c:19` 是 producer 的 `x = 57; y = 1`。ATOMIC 版只剩 counter 的 race（旗子的 race 消失了），PLAIN 版兩個都抓到。
- 在 TSan 底下那一次的 counter **剛好是 2000000**（TSan 讓程式慢很多，兩個 thread 幾乎沒重疊），但它**還是報了 race**：跟 [68:51]「100 次沒事、第 101 次才出事，所以要靠工具」一致：工具看的是有沒有 happens-before，不是結果對不對（[第 39 支](lecture39_note.md) 講這類演算法）。

---

## 15. 一頁速查表

| 主題 | 重點 |
|---|---|
| 能放掉的 | 只有 rf（rfi、rfe）和 po（WR、WW、RR、RW）；ws、fr 永遠 global |
| rfi | 先讀自己的寫；LSQ forwarding、write buffer；所有 model 都放掉；SC 放掉也還是 SC |
| rfe | 先讀別人的寫 = write 不 atomic；tile 裡共用 bus、MSHR |
| W→R | OOO 的 load 越過 store；幾乎都放掉（SB） |
| W→W | MSHR、write buffer、NoC；x86 守住（MP）；要守住就要 ack |
| R→R | LSQ 相依預測；Intel 守住 |
| R→W | 最難放掉：投機的寫、共用 LSQ、value prediction（LB） |
| 表 9.4 | SC、TSO（x86、SPARC v8）、PC（write 不 atomic、守 W→W）、PSO（放 W→W）、weak、RC（acquire／release）、PowerPC、ARM（全放，但 fence 很多種） |
| 誰要懂 | library、OS、compiler 作者（減少 fence 是不可判定的）；一般人要簡單、可攜 |
| Data race | 一般變數上衝突（至少一個寫）而且 concurrent（沒有經過 so 邊的路徑）的存取 |
| 三個定理 | SC ⇏ DRF；DRF ⇒ SC（不管 model）；有 race ⇒ 有一個 SC execution 也有 race |
| PS 程式 | 共用變數都放在 critical section、同一個變數同一組鎖 → DRF → SC |
| 語言 | C++11、C++17、Java 都有 DRF memory model |
| 原始碼 | `READ_ONCE`／`WRITE_ONCE`：用 volatile 擋 compiler；硬體順序還是要 barrier |
| 實測 | TSO 模型跟 x86 完全吻合（8 個例子只破 SB）；圖 9.52(a) 在 TSO 不可行；release 在 x86 是一般 mov；一般變數的旗子被 compiler 改成無窮迴圈；TSan 抓到 race |

---

## 16. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Execution witness、找環、so 邊 | [第 35 支](lecture35_note.md) |
| SC、PLSC、fence、acquire／release | [第 34 支](lecture34_note.md) |
| Atomic 指令、鎖 | [第 37 支](lecture37_note.md) |
| SB、MP 在本機的實測 | [第 33 支](lecture33_note.md) |
| LSQ 的相依預測（R→R） | [第 14 支](lecture14_note.md) |
| Data race 偵測演算法 | [第 39 支](lecture39_note.md) |
| Transactional memory | [第 40 支](lecture40_note.md) |

---

## 附：本講一句話總結

> **因為 PLSC，ws 和 fr 永遠是 global，memory model 能放掉的只有 rf 和 po：先讀到自己的寫（rfi，LSQ forwarding、write buffer，連 SC 放掉都沒差）、先讀到別人的寫（rfe，write 不 atomic）、W→R（OOO 的 load 越過 store）、W→W（MSHR、NoC）、R→R（相依預測）、R→W（最難，要提早寫），從 SC、TSO（x86）、PC、PSO、weak、RC 到全部放掉的 PowerPC、ARM；一般程式設計師不想懂這些，所以有 data race 的觀念：一般變數上衝突的存取之間如果都有經過 so 邊的路徑，程式就是 DRF，而 DRF 不管 memory model 都等於 SC，程式有 race 的話在某個 SC execution 裡一定找得到，所以只要把共用變數都用同一組鎖保護（properly synchronized），就能在任何機器上當成 SC 來想。本機實測：照表 9.4 的 TSO 判斷跟 x86 實跑的結果完全吻合（8 個經典例子只打破 SB），課本說只要放掉 rfi 就可行的圖 9.52(a) 在 x86 上因為 W→W 而不可行；C 的 release／acquire 在 x86 上不用任何 fence，而一般變數當旗子的 data race 會讓 compiler 直接把等待迴圈改成無窮迴圈，ThreadSanitizer 抓得到這種 race。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 9: Multicore Systems (Part VII) -- Memory Models and Data Races 的 whisper 轉錄稿（本機 `large-v3-turbo`，1:16:37）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch9 §9.5–9.6.4（p.489–504）、Linux v6.2.12 原始碼整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
