# Advanced Computer Architecture 第十三支影片完整筆記：Instruction commit、ROB 與四種狀態回復（Chapter 4, Part 4/4）

> **課程**：Advanced Computer Architecture — Chapter 4: The issue, execute, and commit stages - part 4/4 (Instruction commit)（播放清單第 13 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕下載不到）（1:16:43）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch4** §4.4（p.133–145）
> **章號**：影片 Chapter 4 = 課本 Ch4。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 在 [04:36] 把「a **load** instruction finishes…」聽成「So what instruction finishes…」，照前後文和課本 p.135 改正。[36:52]–[37:50] 老師自己說錯又改口（「in the ROB entry for the branch, I am sorry, I take the previous sentence back」），筆記照改口之後的意思寫。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 裡面不照順序，外面照順序](#1-裡面不照順序外面照順序)
- [2. Reorder buffer（ROB）](#2-reorder-bufferrob)
- [3. 怎麼 commit](#3-怎麼-commit)
- [4. 什麼時候釋放實體暫存器](#4-什麼時候釋放實體暫存器)
- [5. Load 和 store 的 commit](#5-load-和-store-的-commit)
- [6. 中斷、例外、猜錯分支](#6-中斷例外猜錯分支)
- [7. 回復程序](#7-回復程序)
- [8. 方法一：RRF](#8-方法一rrf)
- [9. 方法二：RRAT](#9-方法二rrat)
- [10. 方法三：在分支時存 RAT（SRAM）](#10-方法三在分支時存-ratsram)
- [11. 方法四：CAM 做的 RAT，只存 valid bit](#11-方法四cam-做的-rat只存-valid-bit)
- [12. 四種方法比較](#12-四種方法比較)
- [13. Chapter 4 總結](#13-chapter-4-總結)
- [14. 【實測】](#14-實測)
- [15. 一頁速查表](#15-一頁速查表)
- [16. 與後續講次的連結](#16-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
decode → 進 ROB（照順序）→ …執行… → 做完：ROB 那格設 ready
每個 cycle 從 ROB 頭看最多 W 條（commit width）：ready 就 commit，不 ready 就停
commit 時：釋放「上一個」實體暫存器、store 才寫記憶體、load/store 離開 LSQ
例外／中斷／猜錯分支：標記 → 等它到 ROB 頭 → flush 整條 pipeline → 回復暫存器狀態
回復的四種方法：RRF（存值）→ RRAT（存對應）→ 分支時存 RAT（SRAM）→ CAM 只存 valid bit
```

---

## 1. 裡面不照順序，外面照順序

[00:30] Commit 這一步用來：**釋放資源**、把指令**移出 pipeline**、保證 **precise exception**。

[00:46] **Out of order inside, in order outside**：指令照順序進來，外面的人**看起來**它們也照順序離開，雖然裡面其實不照順序執行。

[01:11] 推論：外面的人可以在任何一點**暫停**指令流，之後從**完全一樣的地方**繼續。這是 OOO 最重要的一點。

【課本補充】課本 §4.4.1（p.134）用 page fault 當例子：存 context → 跑 OS 的 interrupt handler 把資料搬進記憶體 → 載回 context → 重做出錯的那條指令。課本 Definition 22（p.136）：commit 是「不能回頭的點」；commit 之前，指令不能永久改變記憶體或架構暫存器。

---

## 2. Reorder buffer（ROB）

[01:44] **ROB**：**每一條**抓進來的指令都有一格。也是 queue，用 **circular queue** 做。

| 時間點 | 做什麼 |
|---|---|
| [02:12] **Decode** 之後 | 放進 ROB（照順序） |
| [03:42] 指令**做完** | 那一格的 **ready bit** 設 1 |
| [05:34] **Commit／retire** | 從 ROB 頭拿掉（照順序） |

[02:47] ROB 滿了（指令因為某些原因出不去）→ ROB 變成瓶頸 → decode 停 → fetch 停 → 整條 pipeline 停。

[03:14] ROB 記錄所有「**在飛（in-flight）**」的指令，也就是還在 pipeline 裡的。

[04:11] 什麼叫「做完」：

| 指令 | 做完的意思 |
|---|---|
| ALU | 算出輸出暫存器的值 |
| Branch | 算出方向和目標 |
| Load | 讀到值 |
| Store | 特別：要等它是 ROB 最上面（最老）的一條才送去記憶體（[§5](#5-load-和-store-的-commit)） |

【課本補充】課本 p.135、p.139 的 ROB entry 最後長這樣：

| PC | nextPC | type | finished | preg | exception bit |
|---|---|---|---|---|---|

課本 p.136：現代 OOO 處理器的 ROB 大約 **100–200** 格。（⚠️ 本機量到的比這個大很多，見 [§14.1](#141-本機的-rob-有多大)。）

---

## 3. 怎麼 commit

[06:22] 每個 cycle：

```
看 ROB 最舊的一條：ready → 拿掉，看下一條；不 ready → 停
最多看 W 條（W = commit width，通常 2、4、6）
```

[07:41] Commit **完全照順序**：碰到第一條沒 ready 的就停，**絕不跳過**。不會發生「後面的 commit 了，前面的還在 ROB」。

[07:15] Commit width 決定指令離開 pipeline 的速度。通常 **fetch width = commit width**：平均每個 cycle 出去 4 條，就該進來 4 條，不然速率對不上。

【課本補充】課本 p.136：理想上每個 cycle 都 commit κ 條，IPC = κ；但因為相依和 cache miss，大部分處理器的 IPC 比 commit width 低很多。一條 L2／L3 miss 的指令會擋住後面一大串；ROB 滿了就停止 fetch，這是對 decode、fetch 的「**back-pressure**」（反壓）。

---

## 4. 什麼時候釋放實體暫存器

[09:16] 終於回答 [第 10 支](lecture10_note.md#10-free-list) 留下的問題。

例子：指令 J：`r1 = r2 + r3`。J 之前 r1 對到 **px**；J rename 時從 free list 拿到 **py**，r1 改對到 py。**px 什麼時候可以放回 free list？**

[10:16] 答案：**J commit 的時候**。理由：

1. J commit 時，ROB 保證**比 J 早的指令都已經 commit、離開 pipeline**。
2. 會用 px 的指令都在 J 之前（J 之後的都看到 py）。
3. 所以已經沒有人需要 px → 可以釋放。

[12:13] 注意：J commit 時**不能**釋放 py，後面還有 py 的 consumer。

[12:42] 要改的地方：rename 一條有目的暫存器的指令時，順便**讀出舊的對應**（px），存進 J 的 ROB entry（課本的 **preg** 欄位）。J commit 時把 px 還給 free list。

[13:01] 【口頭】這會讓 rename table 更複雜，要多一些 port。

【課本補充】課本 Important Point 8（p.137）：一個實體暫存器在「**覆蓋它對應的架構暫存器的那條指令** commit 時」釋放。

（本機試了「不等 commit、rename 時就釋放」會怎樣，見 [§14.2](#142-四種回復方法對不對)。）

---

## 5. Load 和 store 的 commit

[14:00] **Store 什麼時候寫記憶體？** 確定它不在分支的錯誤路徑上、而且它前面不會有中斷。只要前面還有指令，那條指令就可能出 fault。所以：

- **Commit 時**才把 store 送去記憶體，這時才真的生效，自然照程式順序。
- 同時把它從 **store queue** 移掉：store queue 只是暫存的地方（讓 load 先查，[第 12 支](lecture12_note.md#7-load-可以馬上走store-要等)）。

[15:30] **Load 什麼時候離開 LSQ？** 也是 commit 時，同時離開 ROB。

【課本補充】課本 p.139：store 要在 LSQ 裡待到 commit，就算位址早就算好了。提早送可以讓 IPC 高一點，但就沒有 precise exception 了。好消息：只要把 store「**交給**」記憶體系統就好，不用等它真的寫完（什麼時候可以這樣，Ch9 再談）。

---

## 6. 中斷、例外、猜錯分支

[15:55] 【口頭】「很多人在講 branch predictor 時就問：猜錯了怎麼回復？現在回答。」

[16:12] **中斷、例外、猜錯分支**都當成同一類「例外事件」。出現時，pipeline 裡一定有一些**錯誤路徑**上的指令，它們不能 commit，要移掉。

[17:29] 做法（跟 in-order pipeline 一樣）：

1. 出事的指令只**標記**一下（課本：ROB entry 的 **exception bit**），別的什麼都不做。
2. 等它到 **ROB 頭**（變成最老的一條）。
3. 這時開始**回復程序**（[§7](#7-回復程序)）。

[18:10] **中斷**可以標記任何一條，但想盡快處理，就標記 ROB 最上面那條（最老的）。

[18:48] 動畫：抓了 5 條，第 5 條猜錯。前 4 條在正確路徑上，照常 commit；猜錯的那條和它後面的都不能 commit。等猜錯的到 ROB 頭，就 flush。

【課本補充】課本 p.138：例外和 system call 也一樣標記。回復時用「**上一條 commit 的指令的 nextPC**」當成程式要從哪裡繼續。

---

## 7. 回復程序

[22:09]

1. **Flush 整條 pipeline**：ROB、LSQ、instruction window、所有 pipeline register、buffer 全部清空。
2. **回復狀態**。

[22:46] 架構狀態有三樣：

| 狀態 | 要不要特別處理 |
|---|---|
| **記憶體** | 不用：store 只在 commit 時寫，所以記憶體一定是對的 |
| **PC** | 不用：每個 ROB entry 都存 PC，從那裡重新開始 |
| **架構暫存器** | **要**：16 個架構暫存器要回到「分支之前最後一條 commit 的指令」時的值 |

[23:20] 好消息：分支 rename 時的那些實體暫存器**還沒被動過**，值都還在（理由在 [§9](#9-方法二rrat)）。其他實體暫存器的內容不重要，只關心那 16 個。

[24:13] 重新執行時，至少這個分支不會再猜錯：已經知道正確的方向和目標了。

[25:24] 四種方法，越後面越複雜。**前兩種比較常用**，後兩種沒那麼常用。

【課本補充】課本 Important Point 9（p.140）：在任何時刻，只需要追蹤「所有已 commit 的指令」造成的架構暫存器、nextPC、記憶體。nextPC 和記憶體已經解決了，剩下的叫 **precise register state**。

---

## 8. 方法一：RRF

[26:14] 加一個 **retirement register file（RRF）**：架構暫存器有 16 個，它就 16 格，存**值**。

- 每個 ROB entry 多存一個欄位：**目的暫存器的值**。
- [26:45] 例：`add r4, r5, r6` 算出 10。commit 時把 10 寫進 RRF 的 r4。
- [27:41] RRF 永遠是「**所有 commit 了的指令**」的暫存器狀態。這就是一個 **checkpoint**（狀態的紀錄）。

[28:14] 回復：flush，然後把 RRF 整個**搬回** physical register file。因為整條 pipeline 都清空了，可以重新來：

```
r0 → p0, r1 → p1, …, r15 → p15
RRF 的 r0 寫進 p0，r1 寫進 p1，…
```

[30:40] 【口頭】「欣賞一下這種**蠻力**的簡單：保證會對，沒有任何花招。」硬體常常喜歡簡單的設計：容易設計，也容易驗證。

[31:06] 缺點：

1. 每個 ROB entry 多 64 bit（ROB 通常約 200 格）。
2. 回復很慢：要搬 16 個 64-bit 的值，還要改 rename table。

---

## 9. 方法二：RRAT

[31:44] 「漢堡」：這裡有東西要證明。

[32:49] 問題：為什麼要把**值**寫進 RRF？**值本來就在**實體暫存器裡了。

[33:28] 想一想：分支 rename 時 r1 對到 px。等分支到 ROB 頭時：

1. px 還是 r1 的最新值嗎（從分支的角度）？
2. px 的值有被改過嗎？

[34:09] px 什麼時候會被釋放？**某條寫 r1 的指令 commit 時**（[§4](#4-什麼時候釋放實體暫存器)）。那條指令一定在分支**之後**，而分支還沒 commit，它也不可能 commit。所以分支到 ROB 頭時，**px 一定還在、值沒被動過**。只要把 r1 的對應改回 px 就好。

[37:11] 做法：加一個 **retirement RAT（RRAT）**，16 格，存**對應**：

- ROB entry 多存：目的架構暫存器、新拿到的實體暫存器。
- Commit 時：RRAT[r1] = px（「從所有 commit 了的指令看，r1 最新的值在 px」）。
- [41:07] 回復：分支到 ROB 頭時，把 RRAT 抄回 RAT。例如 RAT 裡 r1 已經被錯誤路徑改成 py，擦掉，寫回 px。16 個架構暫存器都這樣做。

[43:30] 搬的是 7-bit 的編號（128 個實體暫存器），不是 64-bit 的值：

| | 每次回復搬多少 |
|---|---|
| RRF | 16 × 64 = 1024 bit |
| RRAT | 16 × 7 = 112 bit |

[44:23] 少了大約 **9 倍**：更快、更省電，而且用的是 OOO 本來就有的機制。

[43:50] 老師說給了「足夠的提示」，更嚴謹的證明留給大家。

---

## 10. 方法三：在分支時存 RAT（SRAM）

[45:16] 前兩種的共同缺點：**每條指令 commit 時**都要讀、寫某個東西，費電。

[45:34] 想法：大部分要回復的情況都是**分支猜錯**，那就**只在碰到分支時**存 checkpoint。

[46:37] 碰到分支 → 把 rename table 拍一張照：16 個 7-bit 編號，共 **112 bit**。

[46:54] Rename table 改成：每一格不是一個 7-bit 欄位，而是一串（「像 linked list」，實際上是 **circular shift register**）：

```
       最新的對應   分支 3 的照片   分支 2 的照片   分支 1 的照片
r0     p6          p6             …              …
r1     …
…
```

- [48:08] 拍照：每一列都往右移一格，最新的那一欄複製一份到旁邊。拍好的照片**不能再改**（「不能改的照片」）。最新那一欄照常繼續改。
- [48:40] 最右邊那一欄：ROB 頭那個分支的照片。
- [48:59] 回復：把「最新對應」的指標移到那個分支的照片那一欄，後面的照片都丟掉。**很快**。

[52:11] 缺點：

1. Rename table 的每一格變成一串（shift register），變寬。
2. 一般指令出 fault（不是分支）時，只能回到**最近的分支**；中間那幾條的狀態差要另外記。老師說這可以解決。

【課本補充】課本 p.142：如果每一列的 circular queue 有 N 格，每列要 7N bit。課本也提到「每條指令都拍照」太貴；「只回到最近的分支」又讓正確性更複雜，所以課本先假設只處理分支猜錯。

---

## 11. 方法四：CAM 做的 RAT，只存 valid bit

[53:10] 【口頭】「很多人可能沒有足夠的背景」：要懂 SRAM 和 CAM。可以看本書 Ch7，或老師 2015 年的 *Computer Organization and Architecture*（McGraw-Hill）第 10 章（⚠️ 老師前面說「chapter 6 of the older book」，後面又說「chapter number 10」）。

[54:00]

| | 像什麼 | 怎麼找 |
|---|---|---|
| **SRAM** | 一般的陣列 | 用 index（第 3 格） |
| **CAM** | hash table | 用**內容**（key）找；很多時候「找到的那一格的 index」就是答案 |

[55:43] RAT 用 CAM 做：

- **128 格**（一個實體暫存器一格），不是 16 格。
- 每格存：**架構暫存器編號**（4 bit）＋ **valid bit**。
- [56:35] 找 r5 對到誰：用 `0101` ＋ `1` 去找，**只會有一格**符合；那一格的 index 就是實體暫存器編號（例如第 28 格 → p28）。實體暫存器編號不用存在格子裡。

[60:44] **Valid bit 是關鍵**：pipeline 裡可能同時有好幾條寫 r5 的指令，r5 → px、→ py、→ pz 三個對應都還在，但只有**最新的** pz 的 valid bit 是 1。

[62:30] 可以證明：

- 128 個 valid bit 裡**剛好 16 個**是 1。
- 每個架構暫存器剛好一個。

[63:48] 關鍵想法（跟方法三一樣）：暫存器的值會一直留著，直到另一條寫同一個架構暫存器的指令 **commit**。所以 checkpoint **只要存那一欄 valid bit** 就夠了：128 個 1-bit，而不是 16 個 7-bit。

[66:13] 例子：r5 → px（已 commit）、→ py（寫了但沒 commit）、→ pz（最新）。在 py 之前有一個分支猜錯。分支到 ROB 頭時，用它的 valid bit 照片：px 那格變回 1，pz 那格變 0。這跟方法三「把對應改回去」是同一件事，只是動的是 1 bit。

[70:57] 缺點：CAM 本身比 SRAM 慢（hash table 永遠比陣列慢）。

[71:19] 一樣用 shift register／circular queue 存每個分支的 valid bit 照片；回復時拿 queue 頭（最舊的那張）。

【課本補充】課本 p.144–145：CAM 也比較**耗電**；而且有方法三一樣的「只能在分支拍照」問題。

⚠️ 課本 p.145 說 checkpoint 的負擔「低 7 倍（如果有 128 個實體暫存器）」。這是**每一列**來看：每列從 7 bit 變 1 bit。但 CAM 有 128 列、SRAM 只有 16 列，**一張照片的總 bit 數**是 128 對 112，CAM 反而多一點。好處是每一列只要動 1 bit，比較簡單、快。（見 [§14.2](#142-四種回復方法對不對) 的數字。）

---

## 12. 四種方法比較

[72:50]

| 方法 | 優點 | 缺點 |
|---|---|---|
| **RRF** | 簡單；把 checkpoint 搬回去很直接 | 每次 commit 都多一次暫存器寫入，費電；ROB 要存「胖」的值，佔空間。高效能處理器不太會用 |
| **RRAT** | ROB 要的空間比 RRF 少；搬對應比搬值快 | 還是**幾乎每個 cycle** 都有寫入；ROB 要存目的暫存器和實體暫存器編號 |
| **SRAM checkpoint** | **只在分支**時有動作；回復比 RRF、RRAT 容易 | RAT 每列更寬；一般指令出例外要回到最近的分支、中間要另外記；很久沒有分支的程式要強迫拍照 |
| **CAM checkpoint** | 拍照很簡單：只存 1 bit | CAM 比 SRAM 慢、耗電多 |

[74:13] 「中間的指令怎麼辦、很久沒分支怎麼辦」：老師說有簡單的解法，要一些記帳或強迫拍照，**留給觀眾自己想**。

（課本 p.145 的 Summary of Register Checkpointing Schemes 是同一張表。）

---

## 13. Chapter 4 總結

[74:47] 【口頭】「這個很長的 chapter 結束了。開心的是，OOO pipeline **完成了**。」

| 機制 | 做什麼 |
|---|---|
| RAT | 架構暫存器 → 實體暫存器；要 free list 和 dependence check logic（[第 10 支](lecture10_note.md)） |
| Wakeup、select、broadcast | 指令排程的核心（[第 11 支](lecture11_note.md)） |
| LSQ | 管 load、store 之間的相依，在飛的 load、store 之間轉送值（[第 12 支](lecture12_note.md)） |
| ROB | 追蹤在飛的指令；做完就 commit |
| 狀態回復 | 回到某個分支 rename 時的狀態 |

[75:58] 下一章（Ch5）：更複雜的 OOO 機制，以及 **compiler** 怎麼幫處理器變快。

---

## 14. 【實測】

程式在 `experiments/lecture13/`。

### 14.1 本機的 ROB 有多大

這一講不用新的程式，把前面量過的兩個結果放在一起看：

| 測試 | P-core | E-core |
|---|---|---|
| [第 3 支 §10.2](lecture03_note.md#102-本機的-out-of-order-視窗有多大)：兩個 cache miss 中間夾 `nop` | 約 **500** 條 | 約 **250** 條 |
| [第 10 支 §13.2](lecture10_note.md#132-實體暫存器先用完)：中間夾會寫暫存器的 `add` | 約 230–240 條 | 約 200 條 |

- `nop` 不用實體暫存器，但**一定要佔一格 ROB**（每條指令都有一格，[§2](#2-reorder-bufferrob)）。所以夾 `nop` 量到的上限，最可能就是 **ROB 的大小**：P-core 約 500、E-core 約 250。
- 夾 `add` 時先用完的是實體暫存器，比 ROB 早停。
- ⚠️ 課本 p.136 說 ROB「大約 100–200 格」。本機的 P-core 大了一倍多。這回答了第 3 支留下的問題：那個「視窗」差不多就是 ROB。Intel 實際公佈的數字這裡沒有，**待補**。

### 14.2 四種回復方法對不對

`recov.py`：隨機產生 20000 條指令（15% 是分支，其中 30% 會猜錯）。猜錯時抓進 1–39 條錯誤路徑上的指令（rename 了、也寫了實體暫存器，但不 commit），然後 flush、用四種方法之一回復。最後跟「照順序、只跑正確路徑」的結果比：

```
程式：20000 條，其中猜錯的分支 879 個、commit 的 ALU 指令 16992 條
方法        結果對嗎回復＋存 checkpoint 共搬幾 bit      每個猜錯平均   commit 時多寫幾 bit
RRF       True                900096          1024           1087488
RRAT      True                 98448           112            118944
SRAM      True                 98448           112                 0
CAM       True                112512           128                 0（valid bit 不是剛好 16 個的次數：0）
對照：rename 時就把舊的實體暫存器還給 free list（不等 commit）：
RRAT  結果對嗎：False，16 個架構暫存器裡錯了 16 個
SRAM  結果對嗎：False，16 個架構暫存器裡錯了 16 個
```

- **四種都對**：16 個架構暫存器的值跟正確答案一樣。
- 每次猜錯要搬的量：RRF **1024** bit、RRAT **112** bit，差 9 倍多，跟影片 [44:23] 一樣。
- **commit 時多做的事**：RRF 每次 commit 寫 64 bit、RRAT 寫 7 bit；SRAM 和 CAM 是 **0**，只在分支時動作。這就是 [§10](#10-方法三在分支時存-ratsram) 說的省電理由。
- CAM 每張照片 128 bit，比 SRAM 的 112 bit 還多一點（[§11](#11-方法四cam-做的-rat只存-valid-bit) 的 ⚠️）。
- CAM 的 valid bit 在每次回復前後都**剛好 16 個**是 1（[62:30] 的性質）。
- **對照組**：如果 rename 時就把舊的實體暫存器還回去（不等 commit），錯誤路徑上的指令會拿到、蓋掉那些還存著「已 commit 狀態」的暫存器，回復後 **16 個全錯**。證明了 [§4](#4-什麼時候釋放實體暫存器) 和 [§9](#9-方法二rrat) 的「值一定還在」靠的就是「commit 時才釋放」。
- 簡化：模擬裡正確路徑的指令一 rename 就 commit，只有錯誤路徑的指令會「在飛」。這樣 SRAM、CAM 的照片剛好等於已 commit 的狀態；真的處理器裡分支前面可能還有沒 commit 的指令，課本 p.142 的推理說明那樣也對，這裡沒有模擬。

### 14.3 Precise exception

[第 6 支 §13.3](lecture06_note.md#133-precise-exception除以-0-之後-sub-做了嗎) 已經量過：`div` 除以 0 時，SIGFPE 的 RIP 正好指著 `div`，後面 100 條無關的 `add` 一條都沒有「看得到的」效果，就算 CPU 很可能早就算好了。這就是本講 ROB ＋ 照順序 commit 做到的事。

---

## 15. 一頁速查表

| 主題 | 重點 |
|---|---|
| 原則 | out of order inside, in order outside；隨時可以暫停、從同一點繼續 |
| ROB | 每條指令一格；decode 進、做完設 ready、commit 出；circular queue；滿了 pipeline 停 |
| Commit | 從頭看最多 W 條；ready 就拿掉，不 ready 就停；不跳過；fetch width ≈ commit width |
| 釋放實體暫存器 | J 把 r1 從 px 改到 py；J **commit** 時釋放 **px**（ROB 存舊對應） |
| Store | commit 時才寫記憶體，同時離開 store queue |
| Load | commit 時離開 LSQ |
| 例外事件 | 中斷、例外、猜錯分支：標記 → 等到 ROB 頭 → flush 全部 → 回復 |
| 不用回復的 | 記憶體（store 在 commit 才寫）、PC（ROB 有存） |
| RRF | 存值；commit 寫 64 bit；回復搬 16×64 |
| RRAT | 存對應；commit 寫 7 bit；回復搬 16×7（少 9 倍） |
| SRAM checkpoint | 分支時拍 RAT 照片（shift register）；回復移指標；只能回到分支 |
| CAM checkpoint | 128 格（arch id ＋ valid）；照片只存 valid bit；剛好 16 個 1；CAM 慢、耗電 |
| 實測 | 本機 ROB 約 500（P）／250（E），比課本大；四種回復都對；rename 時就釋放 → 16 個全錯 |

---

## 16. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| RAT、free list、實體暫存器數量 | [第 10 支](lecture10_note.md) |
| Store queue、LSQ | [第 12 支](lecture12_note.md) |
| Precise exception 的定義與實測 | [第 6 支](lecture06_note.md) |
| OOO 視窗的實測 | [第 3 支](lecture03_note.md) |
| 更積極的推測、replay、compiler 技巧 | [第 14 支](lecture14_note.md)、[第 15 支](lecture15_note.md)、[第 16 支](lecture16_note.md)（Ch5） |
| SRAM、CAM 陣列 | [第 23 支](lecture23_note.md)（Ch7） |
| Page fault | [第 22 支](lecture22_note.md)（Ch7） |
| Store 什麼時候可以「交給記憶體就走」 | [第 33 支](lecture33_note.md)、[第 38 支](lecture38_note.md)（Ch9） |

---

## 附：本講一句話總結

> **OOO 處理器「裡面不照順序、外面照順序」靠的是 reorder buffer：每條指令 decode 後照順序進 ROB、做完設 ready，每個 cycle 從 ROB 頭照順序 commit 最多 W 條、碰到沒 ready 的就停；一條把 r1 從 px 改到 py 的指令 commit 時才釋放 px（因為這時用 px 的指令都已經離開），store 也只在 commit 時寫記憶體；中斷、例外和猜錯的分支都只標記、等它到 ROB 頭再 flush 整條 pipeline，記憶體和 PC 本來就對，只有架構暫存器要回復，方法有四種：RRF 存值（簡單但每次 commit 寫 64 bit）、RRAT 存對應（搬的量少 9 倍）、在分支時把 SRAM 的 RAT 拍照（只在分支時動作）、以及用 CAM 做 RAT 只存 128 個 valid bit（拍照最簡單但 CAM 慢又耗電）。本機實測：模擬裡四種方法都回到正確狀態，RRF 每次回復搬 1024 bit、RRAT 112 bit；如果不等 commit 就釋放舊暫存器，16 個架構暫存器全錯；而前面量到的 OOO 視窗顯示本機 P-core 的 ROB 約 500 格，比課本說的 100–200 大得多。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 4: The issue, execute, and commit stages - part 4/4 (Instruction commit) 的 whisper 轉錄稿（本機 `large-v3-turbo`，1:16:43）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch4 §4.4（p.133–145）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
