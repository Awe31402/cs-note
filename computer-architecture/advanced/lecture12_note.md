# Advanced Computer Architecture 第十二支影片完整筆記：Bypass、漏掉 broadcast 與 load-store queue（Chapter 4, Part 3/4）

> **課程**：Advanced Computer Architecture — Chapter 4: The Issue, Execute and Commit Stages - Part 3/4（播放清單第 12 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕下載不到）（1:01:29）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch4** §4.2.5（p.123–125）、§4.3（p.125–133）
> **章號**：影片 Chapter 4 = 課本 Ch4。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 轉錄在 [03:45]–[04:17] 有幾句只剩「all right」，內容是「broadcast 時也送訊息給 rename table、把目的暫存器那一格的 available bit 設成 1」，照前後文和課本 p.125 補上。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Bypass path](#1-bypass-path)
- [2. Available bit 什麼時候設](#2-available-bit-什麼時候設)
- [3. 漏掉 broadcast](#3-漏掉-broadcast)
- [4. 兩種修法](#4-兩種修法)
- [5. Register read 與 execute](#5-register-read-與-execute)
- [6. 記憶體的相依](#6-記憶體的相依)
- [7. Load 可以馬上走，store 要等](#7-load-可以馬上走store-要等)
- [8. 概念上的 LSQ](#8-概念上的-lsq)
- [9. Store 算出位址時](#9-store-算出位址時)
- [10. Load 算出位址時](#10-load-算出位址時)
- [11. 規則表](#11-規則表)
- [12. 真正的 LSQ：load queue 和 store queue](#12-真正的-lsqload-queue-和-store-queue)
- [13. 用位元向量找「之前」和「之後」](#13-用位元向量找之前和之後)
- [14. 找同位址、沒解析的 entry](#14-找同位址沒解析的-entry)
- [15. 找最左或最右的 1](#15-找最左或最右的-1)
- [16. 【實測】](#16-實測)
- [17. 一頁速查表](#17-一頁速查表)
- [18. 與後續講次的連結](#18-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
前半：暫存器這邊收尾
  bypass path（跟 in-order 的 forwarding 一樣）
  available bit 要跟 broadcast 一起設
  正在 dispatch 的指令可能漏掉 broadcast → 半個 cycle 的做法、或下一個 cycle 補一次
後半：記憶體的相依 → load-store queue（LSQ）
  load 隨時可以走；store 要等到最老才能寫記憶體
  store 算出位址：往後找，轉送給同位址的 load
  load 算出位址：往前找，找同位址的 store 或沒解析的 store
  硬體：load queue ＋ store queue，用位元向量 before/after、CAM、select 樹在 1–2 cycle 內做完
```

---

## 1. Bypass path

[01:13] OOO 的 bypass path 跟 in-order 的 forwarding logic **幾乎一模一樣**：

- 執行單元的輸入前面有 **bypass 多工器**。
- 值可以來自 register file（預設），也可以來自後面 stage 的輸出（經過 latch，圖上沒畫）。

---

## 2. Available bit 什麼時候設

[02:31] RAT 的 available bit（[第 10 支 §12](lecture10_note.md#12-available-bit)）什麼時候設成 1？

| 做法 | 結果 |
|---|---|
| 跟 **register write** 一起設 | ✗ broadcast 之後才進來的指令會讀到 0，又已經錯過 broadcast → **永遠等下去** |
| 跟 **broadcast** 一起設 | ✓ broadcast 時順便送訊息給 rename table，把目的暫存器那一格設成 1 |

【課本補充】課本 p.125：所以 available bit 的意思要改一下：不是「值在 register file 裡」，而是「值在 pipeline 的**某個地方**」（register file 或 bypass network）。

---

## 3. 漏掉 broadcast

[04:17] 跟 broadcast 一起設也不是完全沒問題。看這個時間順序：

| Cycle | Consumer | Producer |
|---|---|---|
| 1 | rename：讀 RAT，p1 的 available bit = **0** | |
| 2 | dispatch：**正在寫進** window | broadcast p1、設 available bit = 1 |

[05:37] 兩件壞事同時發生：

1. 它在 cycle 1 已經讀到 0 了。
2. 它在 cycle 2 還沒完全寫進 window，**沒聽到** broadcast。

→ 它會**永遠等** p1，而 p1 不會再 broadcast 一次。

[12:26] 【口頭】這種「同一個 cycle 一邊讀一邊寫、有相依」的問題叫 **race condition**。老師說 IIT Delhi 有一門課要學生做完整的 OOO 處理器，這種問題「非常常」出現。

[07:42] 【口頭】老師的比喻：班上有人睡著漏聽了，老師就**再上一次課**。

---

## 4. 兩種修法

### 4.1 把 cycle 切成兩半

[08:25]

| 前半 cycle | 後半 cycle |
|---|---|
| dispatch：寫進 window | broadcast |

[09:21] 這樣正在寫進去的指令就不會漏。缺點：為了背靠背執行，broadcast、wakeup、select 要在**同一個 cycle** 做完（[第 11 支 §13](lecture11_note.md#13-什麼時候-broadcast要能背靠背執行)），再擠到半個 cycle 很難。**通常不用這個方法。**

### 4.2 Double broadcast：下一個 cycle 補一次

[09:56]

1. 照常：broadcast 時設 available bit。
2. 把這個 cycle broadcast 的 **tag 記在一個小表**裡。
3. 把這個 cycle 正在 dispatch 的指令記在 **dispatch buffer**（很小，2–4 條，等於 rename width）。
4. **下一個 cycle**：拿記下的 tag 跟 dispatch buffer 比對；比對到，就去 window 裡把那個運算元設成 ready。

[11:53] 「下一個 cycle 的修補」。電路更複雜，但這種事一定要做。

【課本補充】課本 p.125 列了三種：

| 課本的做法 | 跟影片 |
|---|---|
| 前半 cycle 寫、後半 cycle broadcast | 同 §4.1；課本說會讓 clock cycle 變長 |
| 把 tag 存在小 buffer，跟 dispatch 的指令平行比對，晚一點寫 ready bit | 同 §4.2 |
| 把漏掉的 tag **再 broadcast 一次** | 最壞要兩倍的 tag bus；只重送真的被漏掉的 tag 比較好，但平均要多少很難預測 |

【課本補充】課本 §4.2.5 前半（p.123–124）也證明了 [第 11 支 §15](lecture11_note.md#15-執行要好幾個-cycle-的指令) 的「k cycle 的指令在被選中後 k cycle broadcast」：producer 在 cycle 1 被選中，cycle k+2 做完；consumer 最早 cycle k+3 執行，往回推要在 cycle k+1 被選中，所以 producer 最早 cycle k+1 broadcast。實作上每條被選中的指令有一個從 k 倒數到 0 的 **timer**。

---

## 5. Register read 與 execute

[12:40] 「比較簡單的部分」：

```
instruction window（綠／紅） → select（各種 heuristic）
  → register read：讀實體 register file（有些值用不到，因為有 bypass）
  → 很多個執行單元 → 結果寫回 register file
```

[13:47] 到這裡，**以暫存器為運算元**的指令用的重要機制都講完了：rename、dispatch、wakeup、select。它們追蹤相依、強制相依、把「來源好了」從 producer 傳給 consumer。

---

## 6. 記憶體的相依

[14:26] 「完全不同的一回事」。例子：

```
ld r1, 4[r3]
st r2, 10[r5]
```

[15:00] 看暫存器，它們沒關係。但位址是 4 + r3 和 10 + r5（叫 **effective address**），**可能一樣**。一樣的話，它們之間就有相依。

[16:07] 算好位址之後，指令送到 **load-store unit**（之後叫 load-store queue）。

【課本補充】課本 §4.3.1（p.126）：記憶體也有四種：

| I1 → I2（同位址） | 可以對調嗎 |
|---|---|
| load → load | 可以（單處理器）；還可以只送一個去記憶體，把值轉給另一個 |
| load → store | 不行（WAR） |
| store → load | 不行（RAW） |
| store → store | 不行（WAW） |

課本：暫存器可以用 rename 去掉 WAR、WAW，但**記憶體沒辦法 rename**：位址是 32 或 64 位元，表會大到不可能。而且位址要算出來才知道，是**不照順序**產生的，比暫存器難很多。（⚠️ 但本機的 CPU 看起來有某種「記憶體 rename」，見 [§16.3](#163-真的-cpu-的-store-到-load-轉送)。）

---

## 7. Load 可以馬上走，store 要等

[16:31]

| | 規則 | 原因 |
|---|---|---|
| **Load** | 隨時可以執行，不用等別的 load 或 store（除非同位址） | |
| **Store** | 要等到它是 pipeline 裡**最老**的指令，才能送去記憶體；而且照程式順序送 | 它會改變外面看得到的狀態。如果先送了，結果比它老的指令出了 fault，就沒有 precise exception 了（[第 6 支](lecture06_note.md)） |

[18:39] 【口頭】不然會破壞「**裡面不照順序、外面照順序**」（out-of-order inside, in-order outside）。

[19:31] 所以：

1. Store 要等 → 要有地方放它的**位址和值**。叫它 **store queue**（[23:25]）。
2. 後面的 load **不能直接去 cache**：前面可能有同位址的 store 還在等。Load 要先查 store queue（像一個 cache）：
   - 有同位址的 store → **把值轉送（forward）給 load**。
   - 有**還沒算出位址**的 store（**unresolved store**）→ 不知道會不會撞，**要等**。

[24:03] Load 要等到前面所有的 store 都 resolve（算出位址），而且確定不是同一個位址，才能送去記憶體。

---

## 8. 概念上的 LSQ

[24:36] 一條 queue，左邊舊、右邊新；黃色是 load、綠色是 store。

- [25:04] **Decode** 時就配一格（照程式順序）。什麼時候釋放：下一講。
- [25:33] 算出 effective address 時，把位址寫進那一格。**大部分的事都在這時候做。**
- 圖上的符號：⛔ = unresolved；✓ = 算好了；寫著位址（例如 A）也表示算好了。

---

## 9. Store 算出位址時

### 9.1 Case 1

[27:12] Store 算出位址 A → **往後**（比它新的）一格一格看：

- 碰到讀 A 的 load → **轉送值給它**。那個 load 拿到值、寫 register file、broadcast tag，繼續走。
- 碰到另一個寫 A 的 store → **停**。再後面讀 A 的 load 該拿那個 store 的值。

[28:45] 一個 store 的「勢力範圍」：從它開始，到下一個寫同位址的 store 為止。

### 9.2 Case 2

[29:24] 一樣往後看，轉送給第一個讀 A 的 load，然後碰到一個 **unresolved store** → **停**。

[30:28] 它後面讀 A 的 load 不能轉送：那個 store 可能也是寫 A，那 load 該拿它的值。不知道，就不做。

【課本補充】課本 圖 4.25（p.129）是同一個例子：store X 算好後，load A、B 可以拿轉送的值；load C 前面有 unresolved 的 store Y，要等。

---

## 10. Load 算出位址時

### 10.1 Case 1

[31:09] Load 算出位址 A → **往前**（比它舊的）看，碰到一個 **unresolved store** → 停、**等**。就算更前面有寫 A 的 store，也不能轉送。

### 10.2 Case 2

[32:09] 往前看：一個寫 A″ 的 store（不同位址，跳過），然後一個寫 **A** 的 store，中間沒有同位址或 unresolved 的 store → **可以放心轉送**，load 就可以繼續走。

[33:08] 規則：

- Store：往**後**找。
- Load：往**前**找，直到碰到同位址的 store、unresolved store、或到 queue 的開頭。到開頭都沒找到 → 送去 **data cache**。
- 永遠先看 queue 裡的，queue 優先。

---

## 11. 規則表

[35:42]

| 類型 | 找哪裡 | 動作：條件 |
|---|---|---|
| **Load** | 它**之前**的所有 store | 1. 停並轉送：同位址的 store<br>2. 停：unresolved 的 store<br>（都沒有 → 去 data cache） |
| **Store** | 它**之後**的所有 load 和 store | 1. 停：同位址的 store<br>2. 停：unresolved 的 store<br>3. 轉送：同位址的 load |

[36:57] 這張表告訴我們要做出真的 LSQ 需要實作什麼。

【課本補充】課本 p.128：store 的位址算好時，它的值一定也已經有了（RISC 的 store 從暫存器讀值，讀暫存器發生在算位址之前），所以轉送一定拿得到值。

---

## 12. 真正的 LSQ：load queue 和 store queue

[37:17] 為了效率，分成兩條：

| | 放什麼 | 每格存 |
|---|---|---|
| **Load queue** | decode 到的 load | 位址 ＋ 進來時 **store queue 的 tail 指標** |
| **Store queue** | decode 到的 store | 位址 ＋ **值** ＋ 進來時 **load queue 的 tail 指標** |

- [37:43] Store queue 也是值的 cache：load 先查它，能轉送就很快；不行才去 data cache（比較慢，在 pipeline 外面）。
- [38:37] 兩條都是 **circular queue**：head、tail 兩個指標，會繞回來。【口頭】「整個計算機結構裡幾乎都是這樣：要做 queue，幾乎一定是 circular queue。」
- [40:57] 記下對方的 tail 指標：這樣給一個 load，就知道哪些 store 比它早；給一個 store，就知道哪些 load 比它晚。

【課本補充】課本 圖 4.27（p.130）：每格還有 **valid** 和 **resvd**（位址算好了沒）兩個 bit。課本也說 queue 有一個 size 欄位判斷空不空，不看 head、tail。

---

## 13. 用位元向量找「之前」和「之後」

[42:11] 要找「某一格之前／之後、而且符合某條件的所有 entry」。一格一格掃是 O(n)，太慢。改用**位元向量**：n 格就 n 個 bit，硬體可以平行處理。

[43:18] 兩個查表函數（p.131）：

| 函數 | 意思 | 例（N = 8） |
|---|---|---|
| **prec(j)** | 位置 0 到 j−1 是 1（就是 2ʲ − 1） | prec(3) = 1 1 1 0 0 0 0 0 |
| **map(j)** | 只有位置 j 是 1 | map(3) = 0 0 0 1 0 0 0 0 |

（左邊是位置 0。課本 p.131 也是這樣畫，跟一般「右邊是最低位」相反。）

以下 ¬ 是 NOT、∧ 是 AND、∨ 是 OR。（課本 PDF p.134 的圖片確認過這些上橫線。）

| 函數 | 情況 | 式子 |
|---|---|---|
| **before(j)** | j ≥ head（沒繞回來） | ¬prec(head) ∧ prec(j)（式 4.1） |
| | j < head（繞回來） | ¬prec(head) ∨ prec(j)（式 4.2） |
| **after(j)** | j ≤ tail | ¬prec(j) ∧ ¬map(j) ∧ (prec(tail) ∨ map(tail))（式 4.3） |
| | j > tail | (¬prec(j) ∨ prec(tail) ∨ map(tail)) ∧ ¬map(j)（式 4.4） |

[45:09] 為什麼好：硬體很會算 boolean 式子，而且所有 bit 可以**同時**算。

[46:30] 實際用法：一個 load 要找它之前的 store，就用它記下的 store queue tail 指標當 j，算 before(j)。

（本機把所有 head、tail、j 都試過了，見 [§16.2](#162-before-與-after-對不對)。）

---

## 14. 找同位址、沒解析的 entry

[54:12] 再多兩個位元向量：

| 向量 | 意思 |
|---|---|
| **resolved** | 1 = 位址算好了 |
| **match** | 1 = 位址跟要找的一樣 |

[54:47] Match 要平行算：用 **CAM**（content addressable memory，可以「用內容找位置」的陣列）。詳細在 Ch7（[第 23 支](lecture23_note.md)）。

[55:31] 例：找一個 load 之前「同位址或沒解析」的 store：

```
before(j) ∧ (match ∨ ¬resolved)
```

結果裡是 1 的地方，就是搜尋要停的候選。

---

## 15. 找最左或最右的 1

[57:41] 最後拿到像 `0 1 0 0 0 1` 的位元向量，要找**最早**或**最晚**的那個 1。

- [58:41] 一格一格掃要 O(n)；要 O(log n)。
- [59:19] 老師的提示（「漢堡」：想 5 秒）：能不能用**改過的樹狀 select unit**（[第 11 支 §10](lecture11_note.md#10-樹狀的-select-unit)）？老師沒說答案。

【課本補充】課本 p.133 說了：可以。把 choice box 設成「兩邊都是 1 時**永遠選左邊**」，用歸納法可以證明最後選到的就是最左邊的 1；選右邊就找到最右邊的 1。繞回來的情況，先用移位把 head 到 tail 排成不繞回來的樣子（不到一個 cycle）。

[60:50] 【口頭】「我們大約完成了 75% 的工作。」剩下的是把指令移出 pipeline：**commit**（也叫 **retirement**），下一講。

---

## 16. 【實測】

程式在 `experiments/lecture12/`。

### 16.1 漏掉 broadcast

`lsq.py` 第一段照 [§3](#3-漏掉-broadcast) 的時間順序跑：

```
1. 沒修補：ready = 0 （永遠在等）
   有 dispatch buffer：下一個 cycle ready = 1
```

- 沒修補：consumer 寫進 window 的是 cycle 1 讀到的 0，之後再也沒有 p1 的 broadcast，所以一直是 0。
- 加了 [§4.2](#42-double-broadcast下一個-cycle-補一次) 的「記下 tag、下一個 cycle 跟 dispatch buffer 比一次」，就醒了。

（這只是把影片的推理寫成程式，不是真的電路模擬。）

### 16.2 before 與 after 對不對

`lsq.py` 第二段把式 4.1–4.4 寫成 Python，跟「從 head 一格一格走到 tail」比，**所有** head、tail、j 都試：

```
2. N=8：試了 288 組 (head, tail, j)，式子跟一格一格走不一樣的次數：0
2. N=16：試了 2176 組 (head, tail, j)，式子跟一格一格走不一樣的次數：0
```

課本留給讀者證明的式 4.2、「請讀者自己驗證」的式 4.3、4.4，在這些情況下都對。

第三段照 [§11](#11-規則表) 的規則跑課本 圖 4.25 的例子（index 0 最早；X、A、B、C 同位址 a；index 5 是沒解析的 store Y）：

```
3. store X（index 1）算出位址 a：
   after(1)               = 0011111
   之後、同位址或沒解析的 store = 0000010 → 停在 index 5
   範圍內、同位址的 load     = 0001100 → 轉送給 index [3, 4]
   load C（index 6）算出位址 a：before(6) 裡同位址或沒解析的 store = 0100010 → 最晚的是 index 5 ： 沒解析，等
```

- 跟課本一樣：X 轉送給 A（3）、B（4），停在 Y（5）；C（6）要等。
- 「停在哪」用的是 [§15](#15-找最左或最右的-1) 的「找最左／最右的 1」。

### 16.3 真的 CPU 的 store 到 load 轉送

`stfwd.c`：每圈「store x → load x → add 1」，值經過記憶體繞一圈，所以每圈的時間就是這條相依鏈的延遲：

| 測試 | 做什麼 | P-core（cpu2）ns/圈 | ≈ cycle（4.7 GHz） | E-core（cpu14）ns/圈 | ≈ cycle（3.6 GHz） |
|---|---|---|---|---|---|
| `reg` | 對照：值只在暫存器裡走（mov ＋ add） | 0.214 | 1.0 | 0.368 | 1.3 |
| `same` | 存 8 byte、讀同一個 8 byte | 0.215 | **1.0** | 0.279 | **1.0** |
| `idx` | 同上，位址用「基底 ＋ 索引×8」 | 1.44–1.50 | **≈ 7** | 0.557 | ≈ 2 |
| `part` | 存 **4** byte、讀 **8** byte | 3.90–3.92 | **≈ 18** | 3.343 | **≈ 12** |

（每個跑兩次，數字幾乎一樣。）

- **`part`**：store 只寫了一半，store queue **沒辦法轉送**完整的 8 byte。load 只好等 store 真的寫進 cache 再讀，變慢 12–18 倍。這說明 [§9](#9-store-算出位址時) 的「同位址」在真的 CPU 上要比「位址一樣」更細：還要看大小有沒有蓋住。
- **`idx`**：P-core 約 7 cycle，扣掉 add 的 1 cycle，大約 **6 cycle** 是「store 進 store queue → load 找到它 → 轉送」。這比較像影片講的 LSQ 轉送。
- **`same`**：⚠️ 兩種核心都只要 **1 cycle**，跟只用暫存器一樣快，等於 store 和 load **完全沒花時間**。這跟課本 p.126「記憶體沒辦法 rename」看起來不一樣：CPU 好像在 rename 階段就認出「這個 load 讀的就是剛剛那個 store 寫的」，直接把值接過去（一般叫 memory renaming 之類的名字）。課本說的是「做一張完整的記憶體 rename 表不可能」，CPU 可能只是猜少數幾對 store、load；實際怎麼做的 Intel 公開文件這裡沒有，**待補**。
- E-core 的 `idx` 只要 2 cycle、`reg` 卻要 1.3 cycle，原因也**待補**。

---

## 17. 一頁速查表

| 主題 | 重點 |
|---|---|
| Bypass | 跟 in-order forwarding 一樣：多工器選 register file 或後面 stage 的值 |
| Available bit | 跟 broadcast 一起設（不是 register write）；意思是「值在 pipeline 某處」 |
| 漏掉 broadcast | rename 讀到 0、dispatch 時同 cycle broadcast → 永遠等 |
| 修法 | 半個 cycle（太緊，不用）；記下 tag ＋ dispatch buffer，下一個 cycle 補（double broadcast） |
| 記憶體相依 | 位址要算出來才知道；RAW/WAR/WAW 都有；不能 rename |
| Load / store | load 隨時可走；store 要最老才寫記憶體、照順序 |
| Store 算出位址 | 往後找：同位址 load → 轉送；同位址 store 或 unresolved store → 停 |
| Load 算出位址 | 往前找：同位址 store → 轉送並停；unresolved store → 等；都沒有 → data cache |
| 結構 | load queue（記 SQ tail）＋ store queue（位址、值、記 LQ tail）；circular queue |
| 位元向量 | prec(j) = 2ʲ−1、map(j)；before/after 式 4.1–4.4 |
| 找相同 | CAM 給 match 向量；before(j) ∧ (match ∨ ¬resolved) |
| 找最左／右的 1 | 改過的 select 樹：永遠選左（或右） |
| 實測 | 式 4.1–4.4 全部組合都對；P-core：一般轉送 ≈ 6 cycle、只蓋一半 ≈ 18 cycle、最簡單的同位址 ≈ 0（⚠️ 像 memory renaming） |

---

## 18. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Select 樹、broadcast 的時間 | [第 11 支](lecture11_note.md) |
| RAT、available bit、circular queue | [第 10 支](lecture10_note.md) |
| Precise exception | [第 6 支](lecture06_note.md) |
| Commit／retirement、LSQ 什麼時候釋放、store 什麼時候寫記憶體 | [第 13 支](lecture13_note.md) |
| Load 不等 unresolved store、直接猜（aggressive speculation）、猜錯 replay | [第 14 支](lecture14_note.md)、[第 15 支](lecture15_note.md)（Ch5） |
| CAM | [第 23 支](lecture23_note.md)（Ch7） |
| 多處理器時 load → load 也不能隨便對調 | [第 33 支](lecture33_note.md)、[第 34 支](lecture34_note.md)（Ch9） |

---

## 附：本講一句話總結

> **OOO 的 bypass 跟 in-order 的 forwarding 一樣；RAT 的 available bit 要跟 broadcast 一起設，但正在 dispatch 的指令可能已經讀到 0 又漏聽 broadcast 而永遠等下去，所以要把這個 cycle broadcast 的 tag 記下來，下一個 cycle 跟 dispatch buffer 比一次補上；記憶體的相依要等位址算出來才知道，又不能 rename，所以用 load-store queue：load 隨時可以走但要往前找同位址的 store（轉送）或沒解析的 store（等），store 要等到最老才寫記憶體、但算出位址時要往後把值轉送給同位址的 load，直到碰到同位址或沒解析的 store；實作上分成 load queue 和 store queue 兩條 circular queue，用 prec、map 組成的 before/after 位元向量、CAM 的 match 向量、和「永遠選左（右）」的 select 樹，在一兩個 cycle 裡平行做完。本機實測：課本式 4.1–4.4 在所有 head、tail、j 都對；真的 P-core 一般的 store 到 load 轉送約 6 cycle，只蓋到一半的轉送不了、要 18 cycle，而最簡單的同位址情況竟然只要 0 cycle，看起來 CPU 在做某種記憶體 rename（待補）。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 4: The Issue, Execute and Commit Stages - Part 3/4 的 whisper 轉錄稿（本機 `large-v3-turbo`，1:01:29）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch4 §4.2.5、§4.3（p.123–133）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
