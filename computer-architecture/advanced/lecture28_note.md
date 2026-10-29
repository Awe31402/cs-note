# Advanced Computer Architecture 第二十八支影片完整筆記：On-chip network（二）：flit 與 phit、credit 與 on/off flow control、circuit／packet／wormhole／virtual channel（Chapter 8, Part II/V）

> **課程**：Advanced Computer Architecture — Chapter 8: The On-chip Network (Part II/V) -- Flow Control and Flit/Packet Level Switching（播放清單第 28 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（1:03:54）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch8** §8.2（p.346–361）
> **章號**：影片 Chapter 8 = 課本 Ch8。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Python 3.10）實際執行的模擬結果（這一講都是網路協定，本機的 CPU 看不到這一層，只做模擬）
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕把「flit」寫成「fleet／plate／flute／lit」、「phit」寫成「fit」、「tph」寫成「tph／pH」，筆記直接改正。開頭的「foreign」是字幕的雜訊。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Message、packet、flit、phit](#1-messagepacketflitphit)
- [2. On-chip network 不能丟封包](#2-on-chip-network-不能丟封包)
- [3. 最重要的是 buffer 空間](#3-最重要的是-buffer-空間)
- [4. Credit-based flow control](#4-credit-based-flow-control)
- [5. 符號與假設](#5-符號與假設)
- [6. Credit round trip delay](#6-credit-round-trip-delay)
- [7. 要幾格 buffer 才不會停](#7-要幾格-buffer-才不會停)
- [8. On/off flow control](#8-onoff-flow-control)
- [9. 三個門檻的條件](#9-三個門檻的條件)
- [10. Circuit switching](#10-circuit-switching)
- [11. Store and forward](#11-store-and-forward)
- [12. Virtual cut through](#12-virtual-cut-through)
- [13. Wormhole flow control](#13-wormhole-flow-control)
- [14. Head-of-line blocking](#14-head-of-line-blocking)
- [15. Virtual channel](#15-virtual-channel)
- [16. 【實測】](#16-實測)
- [17. 一頁速查表](#17-一頁速查表)
- [18. 與後續講次的連結](#18-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
message → packet（head flit、body flit…、tail flit；同一個 packet 走同一條路）→ flit → phit
NoC 不能丟封包 → 送之前一定要確定對方有 buffer
一條 link：credit-based（每空一格回一個 credit）→ on/off（兩個門檻，少很多訊息）
  credit round trip TD = tph + tf + 2tpr；buffer ≥ TD/tf 才不會停
整條路徑：
  circuit switching：3K + L/B − 1（先 probe、回覆、再送）
  store and forward：K·L/B（每一站收完整個 packet）
  virtual cut through：K + L/B − 1（還是要整個 packet 的空間）
  wormhole：以 flit 為單位要空間 → head-of-line blocking
  virtual channel：一條實體 channel 分成好幾個「車道」，各有自己的 buffer
```

---

## 1. Message、packet、flit、phit

[00:00] NoC 傳的是一個階層的訊息：

| 單位 | 說明 |
|---|---|
| **Message** | 很長的訊息，切成 packet；packet 理論上可以走不同的路，在接收端重組（像一般的網路） |
| **Packet** | 一串 flit，像 linked list：**head flit** → 一串 **body flit** → **tail flit**。同一個 packet 的所有 flit **走同一條路** |
| **Flit**（flow digit） | router 裡 buffer 的單位；邏輯上**不可分割** |
| **Phit**（physical digit） | 一個 cycle 在 link 上實際傳的位元 |

[01:03] 例：64 條平行的銅線 → 每個 cycle 傳 8 byte = 一個 **phit**。flit 是 16 byte → 一個 flit = 2 個 phit，**連續兩個 cycle** 傳完；接收端把整個 flit 存進 buffer。

[05:24] 很多時候一個 message 就是一個 packet。**只有 head flit** 做 routing 的決定（選路），其他 flit 跟著它走。

【課本補充】課本 p.346：大部分 on-chip network 用 **8 或 16 byte** 的 flit；packet 是 64–128 byte（一個 cache block 加上 header）。router 不認得 phit，只收送完整的 flit；有小電路負責把 flit 拆成 phit、再組回來。

---

## 2. On-chip network 不能丟封包

[04:15] 【口頭】老師說這太重要了要寫下來：**不准丟封包**。

| | 丟封包 |
|---|---|
| 一般的網路 | 可以：用丟封包來發現**擁塞**，再做處理 |
| **On-chip network** | **絕對不行** |

「這在哲學上跟一般網路非常不同。」

---

## 3. 最重要的是 buffer 空間

[06:37] 整個 on-chip network 裡最重要的觀念：**buffer 空間**。

- 不能丟 → router 的儲存空間有限 → 只有**確定接收端有空的 buffer** 才送 flit。
- 否則 flit 到了沒地方放，只能丟掉，而丟是不允許的。
- 所以發送端要知道接收端有幾個**空的 flit buffer**：以為有 5 個就送 5 個，然後以為沒了就安靜。

---

## 4. Credit-based flow control

[08:21] 發送端 A 維護一個估計：接收端 B 有幾個空的 buffer（例如 3 個）。

- A 送一個 flit，估計就 −1；估計到 0 就停。
- B 把一個 flit 往下送、**空出一格** → 回送一個 **credit** 給 A（一個 phit 就夠）→ A 的估計 +1。
- 這樣 A 就有一個（大致）最新的估計。

---

## 5. 符號與假設

[10:36]

| 符號 | 意思 |
|---|---|
| **tph** | 一個 phit 走過 A → B 這條 link 的時間 |
| **tf** | 一個 flit 走過這條 link 的時間 |
| **tpr** | 收到任何訊息後，處理它的時間（假設所有訊息都一樣） |
| **TD** | credit round trip delay（下一節） |

[11:48] 假設：

1. router 的時脈**同步**：A 數 10 個 cycle，B 也數 10 個。時間以 **router cycle** 為單位。
2. 收到訊息不知道內容，先處理（tpr），再放進 buffer。
3. 所有的狀態訊息（credit、on、off）都只有**一個 phit**。

---

## 6. Credit round trip delay

[13:07] 時間軸（往下是時間）：

```
B：t = 0 時空出一格 → 送 credit（tph）→ A 處理（tpr）→ A 送一個 flit（tf）→ B 處理（tpr）→ 存進那一格
TD = tph + tpr + tf + tpr = tph + tf + 2·tpr
```

[15:02] 理想上空一格、下一個 cycle 就有 flit 來；實際上要 TD 才補上。

【課本補充】課本 p.349 式 8.2 是同一個式子。

---

## 7. 要幾格 buffer 才不會停

[15:33] 在 TD 這段時間裡，希望 B 不要閒著，一直有 flit 來。

- 一個 flit 佔用 link **tf** 個 cycle（它的 phit 連續地傳）。
- TD 時間內最多收到 **TD / tf** 個 flit。

[17:24] 所以：如果 B 在 t = 0 時至少有 **TD / tf** 個空 buffer（而且這段時間 B 沒有再空出更多），A 就**不會停下來**：它可以一直送。

[19:06] 問題：B 每往下送一個 flit，就回一個 credit → **訊息數變兩倍**，不省電、也不划算。

[20:18] 另一種看法：t = 0 時 B 送出一個 flit、空一格；那格要 TD 之後才會被補上。這段期間 A 想讓 link 一直忙，就要**在之前**已經有 TD/tf 個 credit；而 B 也要有 TD/tf 個空 buffer 收這些 flit → B 的 buffer 總數也至少 TD/tf。

（本機模擬驗證了這個下限，見 [§16.1](#161-credit-based幾格-buffer-才跑得滿)。）

---

## 8. On/off flow control

[23:47] 改進：不要每次都回 credit，用**兩個門檻**：

| 門檻 | B 的空 buffer 數到了這裡 | 送什麼 |
|---|---|---|
| **N_off** | 只剩這麼多（下限） | **OFF**：叫 A 停 |
| **N_on** | 又空出這麼多 | **ON**：叫 A 開始送 |

[24:20] 流程：

```
B 的空 buffer 降到 N_off → 送 OFF → A 收到才停
  但 A 在收到之前送出的 flit 還在路上 → B 要有空間放它們（不能丟）
B 往下送了很多 flit → 空 buffer 升到 N_on → 送 ON → A 又開始送
```

### 8.1 OFF 之後的「危險期」

[25:25] 設 B 在時間 t 送 OFF：

- A 在 t + tph + tpr 才知道要停。
- 而在 t 之後才到 B 的 flit，最早是 A 在 t + 1 − tf − tpr 送出的。
- 危險期的長度 = (t + tph + tpr) − (t + 1 − tf − tpr) + 1 = **tph + tf + 2tpr = TD**。

[28:26] 又是 TD。危險期內最多來 TD/tf 個 flit，所以 **N_off ≥ TD / tf**。

---

## 9. 三個門檻的條件

[29:39] **N_on > N_off**：常識。極端情況 N_on = N_off + 1：送出一個就送 ON、收一個又送 OFF，跟 credit-based 沒兩樣（老師「不喜歡」）。要避免頻繁的 ON/OFF，兩個門檻之間要有**相當的距離**。

[32:03] **總 buffer 數 N** 要夠大：

- 送 ON 的那一刻有 N_on 個空的 → **N − N_on 個是滿的**。
- 在 ON 的「陰影」（TD 個 cycle，還沒有新的 flit 來）裡，B 不想閒著 → 要有 TD/tf 個 flit 可以往下送。
- 所以 **N − N_on ≥ TD / tf**。

[36:12] 合起來：

```
N_off ≥ TD/tf          （OFF 之後在路上的 flit 放得下）
N_on  > N_off
N − N_on ≥ TD/tf       （ON 之後不會閒著）
→ N ≥ N_on ＋ TD/tf > N_off ＋ TD/tf ≥ 2·TD/tf
```

[38:05] 到這裡講完了「單一發送端、單一接收端」的兩種 flow control：credit-based 比較浪費（雖然 credit 也可以批次送），on/off 就是在批次處理，減少訊息數。

（本機模擬了 on/off 的門檻，見 [§16.2](#162-onoffn_off-太小就會掉-flit)。）

---

## 10. Circuit switching

[38:48] 現在看整個網路（mesh 上的一個 source 和 destination）。最簡單的想法：**circuit switching**（舊的電話網路）。

[39:25] 從 source 到 destination，沿路**預留** router 的 buffer：

1. 送一個小的 **probe flit**，走一條路，把整條路的 buffer 預留下來。
2. destination 回一個 **acknowledgment**：「已經幫你預留好了」。這條路叫一個 **circuit**。
3. 整個 message 一路送過去（像蟲一樣 pipeline 地移動），中途沒有人會擋。
4. 送完了清掉預留。

[40:30] 時間（K 個 hop、每 hop 1 cycle；message 長 L 個 flit、channel 頻寬每 cycle B 個 flit）：

| 階段 | 時間 |
|---|---|
| probe | K |
| 回覆 | K |
| 送 message（pipeline） | K ＋ L/B − 1 |
| **合計** | **3K ＋ L/B − 1** |

- [42:14] 兩種變化：送 probe 時就預留（簡單，但佔住資源更久），或回覆時才預留。性能和簡單的取捨。
- [44:40] −1 是因為有一個 cycle 被算了兩次。B < 1 時常數會變，老師不把公式弄得更複雜。

[45:59] 缺點：建立 circuit 就要 2K 個 cycle，而且**長時間佔住**資源（router 的 buffer）。適合大量的傳輸；on-chip network 有很多**短**的傳輸，不適合。

---

## 11. Store and forward

[47:15] **Packet** 層級的 flow control：

- 每個 router 都有空間放**整個 packet**（所有的 flit）。
- 整個 packet 到了、存好了，才往下一個 router 送。

[48:20] 時間：每一 hop 要 L/B，共 K 個 hop → **K · L/B**。「比 circuit switching 還慢很多」，不會佔住資源很久，但很慢，所以實際上沒人用基本的 store and forward。

[48:54] 另一個限制：packet 有 4 個 flit，下一站只有 3 格空間 → **一個 flit 都不能送**。

---

## 12. Virtual cut through

[49:27] **VCT**（virtual cut through）：

| 跟 store and forward 一樣 | 不一樣 |
|---|---|
| 下一站要有**整個 packet** 的空間才能送 | **不用等整個 packet 到了**才往下送：一個 flit 到了就往下一站送（pipeline） |

[51:38] 時間：**K ＋ L/B − 1**，跟 circuit switching 送 message 的那一段一樣。

[52:12] 問題：四個 router A、B、C、D。黑色的 flit（packet 1）要到 C；B 裡有橘色的 flit 要往 D；藍色的要去 D 之後。B 有 2 個空 buffer，但 packet 1 需要放整個 packet 的空間 → **一個黑色的 flit 都不能送**，明明有空位。浪費資源。

[54:05] 比較：

| 方法 | 時間 |
|---|---|
| Circuit switching | 3K ＋ L/B − 1 |
| Store and forward | K · L/B |
| **VCT** | **K ＋ L/B − 1**（最好） |

（本機用逐 flit 的模擬驗證了 VCT 的公式，見 [§16.3](#163-k-個-hop-的延遲)。）

---

## 13. Wormhole flow control

[55:19] 解法之一：以 **flit** 為單位做 buffer。不用等整個 packet 的空間，**有一格就送一個 flit**：像蟲（worm）一樣移動。

[55:57] 同一個例子：黑色的 flit 可以送進 B 的空格，再往前走。有效率多了。

---

## 14. Head-of-line blocking

[56:32] Wormhole 的問題：

```
A → B → C
B 的 buffer（一條 queue）：[ 橘、橘 | 黑、黑 ]
  橘色要往下（D），但下面堵住了 → 動不了
  黑色要往 C，C 是空的 → 本來可以走
```

[57:04] 黑色的 flit 在 queue 的後面，前面的橘色**擋住整條 queue** → 黑色的也被卡住。這是典型的 **head-of-line blocking**（HoL）。

---

## 15. Virtual channel

[58:14] 最精緻的做法，像把一條路分成**好幾個車道**：

- A 和 B 之間只有**一條實體 channel**（同一組銅線），但邏輯上分成好幾個 **virtual channel（VC）**。
- **每個 VC 在接收端 B 有自己的一組 flit buffer**。
- 每個 flit 標記它屬於哪個 VC，到了 B 就放進那個 VC 的 buffer。

[59:20] 【口頭】道路的比喻：一條路分成好幾個車道，右轉專用道、左轉專用道（印度是靠左行駛，所以是左轉專用道），其他直行；左轉的車不會擋到直行的。有些路有地下道、高架橋：一個車道遇到紅燈，另一個車道走高架橋，交通不受影響。

[60:25] 同一個例子，兩條 queue：橘色的還是卡住，但黑色的在**另一個 VC**，可以一直走到 C → **沒有 HoL blocking**。

[62:05] 優缺點：

| 優點 | 缺點 |
|---|---|
| 解決 blocking；一條實體 channel 被多個 VC 多工 → **吞吐量大增** | router 變複雜很多：要分配、追蹤 VC，最佳地分配很難（但「實際上沒那麼難」，[第 29 支](lecture29_note.md)、[第 30 支](lecture30_note.md)） |
| 一個 VC 卡住，其他 VC 的 packet 照樣能走（往別的方向，或目標 router 上別的 VC 是空的） | |

[63:19] 下一講：routing 演算法。

---

## 16. 【實測】

這一講是 router 之間的協定，本機的 CPU 看不到這一層，所以用一個逐 cycle 的小模擬驗證影片的公式。程式是 `experiments/lecture28/flow.py`。

### 16.1 Credit-based：幾格 buffer 才跑得滿

一條 link A → B，B 每個 cycle 往下送一個 flit（如果有），每空一格就回一個 credit。量 A 送 flit 的速率（1.00 = link 跑滿）：

```
1. credit-based：tf = 2、tph = 1、tpr = 1 → TD = tph + tf + 2·tpr = 5，TD/tf = 2.5
   B 有 1 格 buffer：link 使用率 0.40
   B 有 2 格 buffer：link 使用率 0.80
   B 有 3 格 buffer：link 使用率 1.00
   B 有 4 格 buffer：link 使用率 1.00
   B 有 6 格 buffer：link 使用率 1.00
   tf = 1、tph = 2、tpr = 2 → TD = 7，TD/tf = 7
   B 有 3 格 buffer：link 使用率 0.43
   B 有 5 格 buffer：link 使用率 0.71
   B 有 7 格 buffer：link 使用率 1.00
   B 有 8 格 buffer：link 使用率 1.00
```

- buffer **≥ TD/tf**（第一組 ≥ 2.5 → 3 格；第二組 ≥ 7 格）時 link 跑滿；不夠時，使用率剛好是 **N ÷ (TD/tf)**（例：3/7 = 0.43、5/7 = 0.71）。這就是 [§7](#7-要幾格-buffer-才不會停) 的結論：N 格 buffer 在一個 TD 裡只能撐 N 個 flit。

### 16.2 On/off：N_off 太小就會掉 flit

TD = 4、tf = 1 → TD/tf = 4。B 每 2 個 cycle 才往下送一個（所以一定會塞住，A 最多只能用一半的 link）：

```
2. on/off（tf = 1、tph = 1、tpr = 1 → TD = 4；B 每 2 個 cycle 才往下送 1 個，所以一定會塞住）
   N = 12、n_off = 4、n_on =  8：使用率 0.50（上限 0.50）、控制訊息 1538 個
   N = 12、n_off = 2、n_on =  8：使用率 0.50（上限 0.50）、控制訊息 1176 個
   N =  6、n_off = 4、n_on =  5：使用率 0.50（上限 0.50）、控制訊息 2857 個
   N = 20、n_off = 4、n_on = 16：使用率 0.50（上限 0.50）、控制訊息 689 個
   B 完全不往下送（最壞情況）時，n_off 太小會不會 overflow？（影片：n_off ≥ TD/tf = 4）
   n_off = 2：overflow！（掉 flit 了）
   n_off = 3：沒有 overflow
   n_off = 4：沒有 overflow
   n_off = 5：沒有 overflow
   對照：credit-based 每送走一個 flit 就回一個 credit → 2 萬個 cycle 裡約 1 萬個 credit 訊息
```

- 2 萬個 cycle 裡，on/off 只要 **689–2857 個**控制訊息；credit-based 每送走一個 flit 就要一個（約 1 萬個）。
- **N_on 和 N_off 離得越遠，控制訊息越少**（N = 6、5 和 4 只差 1：2857 個；N = 20、16 和 4：689 個）：就是 [29:39] 說的「兩個門檻之間要有相當的距離」。
- **最壞情況**（B 完全不往下送）：n_off = 2 時，OFF 送出之後還在路上的 flit 沒地方放 → **overflow**（在 NoC 裡等於掉 flit，不允許）。n_off ≥ 3 就安全。
- ⚠️ 公式說 n_off ≥ TD/tf = 4，模擬裡 3 就夠了：差 1 是模擬裡同一個 cycle 內「收、送、判斷」的先後順序造成的邊界差異（[25:25] 的推導算進了一個 +1）。公式是安全的那一邊。

### 16.3 K 個 hop 的延遲

```
3. K 個 hop、L 個 flit（B = 1 flit/cycle）
   K = 3、L =  4：circuit  12（3K + L − 1 = 12）、store and forward  12（K·L = 12）、VCT   6（K + L − 1 = 6）
   K = 5、L =  8：circuit  22（3K + L − 1 = 22）、store and forward  40（K·L = 40）、VCT  12（K + L − 1 = 12）
   K = 8、L = 16：circuit  39（3K + L − 1 = 39）、store and forward 128（K·L = 128）、VCT  23（K + L − 1 = 23）
```

- VCT 是**逐 flit** 模擬的（每個 flit 一到就往下一 hop 送，link 一次一個），結果剛好是 K ＋ L − 1。
- circuit switching 和 store and forward 在這個模型裡就是直接照定義算（probe ＋ 回覆 ＋ pipeline；每一站收完整個 packet），不算獨立的驗證。
- 差距隨 K、L 變大：K = 8、L = 16 時 store and forward 要 128 cycle，VCT 只要 23。

---

## 17. 一頁速查表

| 主題 | 重點 |
|---|---|
| 階層 | message → packet（head、body、tail；同一條路）→ flit（buffer 單位，8–16 B）→ phit（一個 cycle 傳的） |
| 不能丟 | NoC 絕對不丟封包 → 送之前要確定有 buffer |
| Credit-based | 每空一格回一個 credit；訊息數變兩倍 |
| TD | tph ＋ tf ＋ 2tpr |
| Buffer 下限 | ≥ TD/tf 才不會讓 link 閒著 |
| On/off | 空格 ≤ N_off 送 OFF、≥ N_on 送 ON；N_off ≥ TD/tf、N_on > N_off、N − N_on ≥ TD/tf → N ≥ 2TD/tf |
| Circuit switching | probe、回覆、送：3K ＋ L/B − 1；佔住資源；適合大量傳輸 |
| Store and forward | 每站收完整個 packet：K·L/B；最慢 |
| VCT | 要整個 packet 的空間，但一到就往下送：K ＋ L/B − 1 |
| Wormhole | 以 flit 為單位要空間 |
| HoL blocking | queue 的頭卡住，後面能走的也走不了 |
| Virtual channel | 一條實體 channel 分成好幾個 VC，各有 buffer；解 HoL、吞吐量大；router 變複雜 |
| 實測（模擬） | buffer ≥ TD/tf 才跑滿，否則使用率 = N/(TD/tf)；on/off 控制訊息少 4–15 倍；n_off 太小會掉 flit；VCT = K ＋ L − 1 |

---

## 18. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Router、channel、topology | [第 27 支](lecture27_note.md) |
| Routing、deadlock、用 VC 避免 deadlock | [第 29 支](lecture29_note.md) |
| VC allocation、switch allocation、arbiter | [第 29 支](lecture29_note.md)、[第 30 支](lecture30_note.md) |
| Synthetic traffic、吞吐量和延遲 | [第 31 支](lecture31_note.md) |
| 記憶體的同步／非同步通訊（另一種 flow control） | [第 42 支](lecture42_note.md)（Ch10） |

---

## 附：本講一句話總結

> **NoC 把 message 切成 packet、packet 切成 flit（buffer 的單位）、flit 再切成 phit（一個 cycle 在 link 上傳的位元），同一個 packet 的 flit 跟著 head flit 走同一條路；跟一般網路最大的不同是絕對不能丟封包，所以送之前一定要知道對方有空的 buffer：credit-based flow control 每空一格就回一個 credit，credit round trip TD = tph + tf + 2tpr，接收端至少要 TD/tf 格 buffer 才不會讓 link 閒著，而 on/off flow control 用 N_off ≥ TD/tf、N_on > N_off、N − N_on ≥ TD/tf 這幾個條件換來少很多的控制訊息；整條路徑上，circuit switching 要 3K + L/B − 1 而且佔住資源，store and forward 要 K·L/B，virtual cut through 只要 K + L/B − 1 但仍要整個 packet 的空間，wormhole 以 flit 為單位卻會 head-of-line blocking，virtual channel 則把一條實體 channel 分成好幾個有各自 buffer 的「車道」來解決。本機模擬：buffer 不到 TD/tf 時 link 使用率剛好是 N ÷ (TD/tf)，on/off 的控制訊息比 credit 少 4 到 15 倍、但 N_off 太小就會在最壞情況下掉 flit，VCT 的逐 flit 模擬剛好是 K + L − 1 個 cycle。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 8: The On-chip Network (Part II/V) -- Flow Control and Flit/Packet Level Switching 的 YouTube 自動字幕（1:03:54）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch8 §8.2（p.346–361）整理。實測為本機（Python 3.10）模擬結果。*
