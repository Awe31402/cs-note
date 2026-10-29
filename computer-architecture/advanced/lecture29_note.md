# Advanced Computer Architecture 第二十九支影片完整筆記：On-chip network（三）：routing、deadlock、turn model 與 router 的 5 級 pipeline（Chapter 8, Part III/V）

> **課程**：Advanced Computer Architecture — Chapter 8: The On-chip Network (Part III/V) -- Routing Algorithms and 5-Stage Router Pipeline（播放清單第 29 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（1:07:49）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch8** §8.3–8.4.5（p.361–387）
> **章號**：影片 Chapter 8 = 課本 Ch8。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Python 3.10）實際執行的模擬結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕有時把「flit」聽成「flip」、「Valiant」聽成「aliens」、「date line」寫成「deadline」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 為什麼 routing 很重要](#1-為什麼-routing-很重要)
- [2. 什麼是好的路徑](#2-什麼是好的路徑)
- [3. Deadlock、livelock、starvation](#3-deadlocklivelockstarvation)
- [4. Starvation freedom 與 deadlock freedom](#4-starvation-freedom-與-deadlock-freedom)
- [5. Aging](#5-aging)
- [6. Resource dependence graph](#6-resource-dependence-graph)
- [7. Channel dependence graph](#7-channel-dependence-graph)
- [8. Turn graph](#8-turn-graph)
- [9. XY routing](#9-xy-routing)
- [10. Oblivious routing：Valiant](#10-oblivious-routingvaliant)
- [11. Adaptive routing：turn model](#11-adaptive-routingturn-model)
- [12. 用 virtual channel 避免 deadlock：dateline](#12-用-virtual-channel-避免-deadlockdateline)
- [13. Router 的輸入輸出](#13-router-的輸入輸出)
- [14. Stage 1：buffer write](#14-stage-1buffer-write)
- [15. Stage 2：route computation](#15-stage-2route-computation)
- [16. Stage 3：VC allocation](#16-stage-3vc-allocation)
- [17. Stage 4：switch allocation](#17-stage-4switch-allocation)
- [18. Stage 5：switch traversal](#18-stage-5switch-traversal)
- [19. 【實測】](#19-實測)
- [20. 一頁速查表](#20-一頁速查表)
- [21. 與後續講次的連結](#21-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
不能丟 flit → flit 在 router 排隊 → 擁塞 → 要繞路；但要避免 deadlock、livelock、starvation
starvation freedom ⇒ deadlock freedom（反過來不成立）；aging 處理 livelock、starvation
分析 deadlock：RDG（packet ↔ channel）→ CDG（channel → channel）→ turn graph（保留方向）
  CDG 有 cycle ⇒ 可能 deadlock
routing：XY（4 個轉彎，沒有路徑多樣性）→ Valiant（隨機中繼點）→ 最小 oblivious
  → adaptive（turn model：順、逆時針各禁一個轉彎，剩 6 個）→ VC ＋ dateline
router 的 5 級 pipeline：buffer write → route computation → VC allocation → switch allocation → switch traversal
```

---

## 1. 為什麼 routing 很重要

[00:22] **Routing**：一個 packet 從 source 到 destination 要走哪條路的演算法。

[00:56] 為什麼重要：

1. **不能丟 flit**（[第 28 支](lecture28_note.md)）→ 每條 link 都有 credit 或 on/off flow control，沒有 buffer 就不送。
2. 所以 flit 在 router 裡**排隊** → **擁塞**。
3. Virtual channel 只能解決一部分。
4. 所以要找**替代路線**，繞過擁塞的地方，讓 flit 盡快到。

[01:55] 同一個 packet 的 flit 走同一條路，同一個 message 的不同 packet 不一定。routing 只對 **head flit** 做。

---

## 2. 什麼是好的路徑

[02:26] **Path diversity**（有很多條路可選）越高，越能容忍擁塞。

[03:00] 最短的**路徑**不一定是最短的**時間**（路上可能塞車）。好的 routing：

- 延遲小。
- **公平**：讓 packet 之間花的時間差距盡量小，不要無緣無故讓某些 packet 延遲。

---

## 3. Deadlock、livelock、starvation

[03:29] 三個要避免的問題。

[04:03] 【口頭】老師放了一張街道大塞車的照片：車子往各個方向擠成一團，誰都動不了。「你不能叫直升機把車吊走。」只能讓某些車走一條很長的路繞出去再回來。NoC 也不能丟封包，所以為了避免擁塞和 deadlock，常常要走很長的路。

| 問題 | 意思 |
|---|---|
| **Deadlock** | **沒有人能前進**。例：4 台車，每一格只能放一台，每台都在等前面那格空出來 → 循環等待（circular wait）。NoC 裡，buffer 只有 1 個 flit 時：A 的 flit 要去 B，B 的要去 C，C 的要去 D，D 的要去 A，全滿 |
| **Livelock** | flit **一直在移動**，但**永遠到不了**目的地（一直繞圈圈）；有進展，但不是有意義的進展 |
| **Starvation** | 某個 flit **永遠不動**：它的優先權最低，router 一直送別的 flit |

[05:56] 最好讓 routing 演算法**根本不會**產生 deadlock，因為發生之後再解決很難。

---

## 4. Starvation freedom 與 deadlock freedom

[07:32] 電腦科學裡一個很直覺的結果：

```
starvation freedom（沒有 flit 會永遠卡住）＋ 沒有 livelock ⇒ deadlock freedom
```

- [08:05] 沒有 packet 永遠卡住、又沒有 livelock → 它一直在動、而且會到目的地 → 不可能有「所有相關的 flit 都永遠動不了」的 deadlock。

[09:11] **反過來不成立**：deadlock freedom 不保證 starvation freedom。反例：一個 router 把某個 flit 放到旁邊的 queue、永遠延遲它 → 它餓死了；但演算法的其他部分可以設計成沒有 deadlock。

---

## 5. Aging

[10:31] 解決 starvation 和 livelock 的一般做法：**aging**。

- flit 在 router 待得越久，**優先權越高**。
- 優先權高的 flit 不再繞路，改走**直的、最短的路**到目的地 → 不會 livelock；優先權高 → 不會餓死。

[10:59] 但 aging 實際上很難做；保證 starvation freedom 也很難：

- [11:28] starvation 是**局部**的性質（一個 router 的事）；deadlock 是**全域**的（整個網路的 buffer 都滿了）。
- 就算不想餓死某個 flit，也要保證鄰居有空間，而只靠局部的決定很難保證。

[12:29] 所以大部分論文**分開**處理：用協定避免 deadlock（全域），用 aging 處理 livelock 和 starvation（局部）。

【課本補充】課本 p.365：處理 livelock 可以學一般的網路：head flit 帶一個 **hop count**，每經過一個 router 就 +1。設一個門檻（例：最短路徑 6 hop，門檻 12 hop），超過了就只准走最短路徑。另一種是 hop count 越高優先權越高。

---

## 6. Resource dependence graph

[14:01] 例子（之後會一直看這張圖）：

```
P1 從 D 要到 B：走過 D → A（channel 1），卡在 A，要 A → B（channel 2）
P2 從 A 要到 C：佔著 channel 2，要 channel 3
P3 從 B 要到 D：佔著 channel 3，要 channel 4
P4 從 C 要到 A：佔著 channel 4，要 channel 1
```

- [14:32] 「P1 **佔著**（hold）channel 1」：它的 flit 在 channel 1 的 buffer 裡。「**想要**」channel 2：要 channel 2 的 buffer 空間。

[15:29] **RDG**（resource dependence graph）：一個**二分圖**，一邊是 packet、一邊是 channel：

| 邊 | 意思 |
|---|---|
| packet Pi → channel j | Pi **想要** channel j |
| channel j → packet Pi | Pi **佔著** channel j（有權在上面傳） |

[17:12] 例子的 RDG：1 → P1 → 2 → P2 → 3 → P3 → 4 → P4 → 1 → 一個 **cycle** → 循環等待 → **deadlock**。

---

## 7. Channel dependence graph

[18:10] **CDG**（channel dependence graph）：把 packet 拿掉，直接在 channel 之間連邊：1 → 2 → 3 → 4 → 1。

[19:43] **CDG 有 cycle ⇒ 循環等待 ⇒ deadlock**。要避免 CDG 裡的 cycle。

[20:13] 壞消息：**有 virtual channel 也可能 deadlock**。改成 **virtual channel dependence graph**：每條實體 channel 有兩個 VC（編號 0、1），所有 VC 0 可能形成一個 deadlock 的 cycle，所有 VC 1 也是。之後會看到一個讓 VC 設計免疫 deadlock 的協定（[§12](#12-用-virtual-channel-避免-deadlockdateline)）。

---

## 8. Turn graph

[21:56] RDG、CDG 的缺點：網路裡的 channel 有**方向**（north、south、east、west），這些**方位**資訊不見了。

[23:06] **Turn graph（TG）**：

1. 在 CDG 裡取一條路徑 C。
2. TG 包含 C 上所有的節點和 channel，**保留**每條 channel 在原本網路裡的方向，不包含其他 channel。
3. [25:39] 每條 channel 的中間加一個 **channel node**。

[26:08] 性質：

- CDG 裡的一條路徑，在 TG 裡有一條經過同樣 channel node 的路徑。
- **CDG 有 cycle，就能建出一個有 cycle 的 TG。**
- [27:13] CDG 的每一條邊，在 TG 裡不是**三點共線**（直走），就是一個**轉彎**。
- 所以 TG 裡的 cycle 可以寫成一串直走和轉彎。

[28:20] 目標：設計一個 routing 協定，讓**不可能**建出有 cycle 的 TG → 所有 CDG 都沒有 cycle → **證明沒有 deadlock**。

---

## 9. XY routing

[29:21] 最簡單（但很沒效率）的：mesh 上每個節點有 (x, y) 座標。從 (x1, y1) 到 (x2, y2)：

```
先沿 x 軸：(x1, y1) → (x2, y1)
再沿 y 軸：(x2, y1) → (x2, y2)
```

叫 **dimension ordered routing** 或 **XY routing**。n 維的話，把維度排好順序，一維一維走。

[31:33] 證明沒有 deadlock（反證法）：

- 假設能建出一個有 cycle 的 TG。任何 cycle 都可以拆成簡單的**順時針**或**逆時針** cycle。
- [32:41] 逆時針的 cycle 裡一定有一個「先走 y、再轉到 x」的轉彎。XY routing **不允許**這種轉彎（只能先 x 後 y）。
- [34:20] 順時針的也一樣（箭頭反過來）。
- 所以建不出有 cycle 的 TG → **XY routing 一定沒有 deadlock**。

[34:54] 缺點：簡單，但**沒有 path diversity**：起點終點確定，路就確定，**不能避開擁塞**。

（本機建了 4 × 4 mesh 的 CDG 檢查，見 [§19.1](#191-turn-model真的-16-種都沒有-deadlock-嗎)。）

---

## 10. Oblivious routing：Valiant

[35:23] **Valiant's algorithm**（oblivious routing 的一種）：

1. **隨機**選一個點 P。
2. source → P（用 XY routing）。
3. P → destination（用 XY routing）。

- P 的選擇很多 → **path diversity** 很高，比較能應付擁塞。
- 每一段都是 XY routing → **還是沒有 deadlock**。
- [35:54] 缺點：路徑可能**很長、不是最佳的**（A 和 B 很近，P 卻在很遠的地方）。

[36:27] **Minimal oblivious routing**：在 B（或 A）周圍畫一個**小窗口**，P 只在窗口裡選。路徑不會太長，但還有一些隨機性和 path diversity：介於 XY 和純 Valiant 之間的取捨。

---

## 11. Adaptive routing：turn model

[37:34] 把方向標成 **N、S、E、W**。

[38:16] 任何 TG 裡的大路徑都可以化成兩種簡單的 cycle：

| Cycle | 4 個轉彎 |
|---|---|
| **順時針** | N → E、E → S、S → W、W → N |
| **逆時針** | N → W、W → S、S → E、E → N |

[38:50] 想法：每個 cycle **拿掉一個**轉彎，最多允許 **6 個**轉彎 → 兩種 cycle 都完成不了。

- [39:24] 順時針 4 種選法 × 逆時針 4 種 = **16 種**。
- [39:55] XY routing 只允許 **4 個**轉彎；這裡允許 **6 個**：越多轉彎越有彈性（不能再多，不然會形成 cycle）。
- 這叫 **turn model**；用 6 個轉彎、禁掉 2 個的 routing 叫 **adaptive routing**。

[41:20] 16 種裡有名的三種：

| 演算法 | 禁止的轉彎 | 意思 |
|---|---|---|
| **West first** | N → W、S → W | 西邊要先走（轉彎的終點不能是西） |
| **North last** | N → E、N → W | 北邊最後走（走了北就不能再轉） |
| **Negative first** | N → W、E → S | 負方向（西、南）先走 |

[43:27] path diversity 高，避開擁塞的能力比 XY 強很多。

⚠️ 影片 [40:53] 和課本 p.374 都說：16 種裡每一種（順、逆時針各拿掉一個）都**沒有 deadlock**。本機把 4 × 4 mesh 的 CDG 實際建出來檢查，**只有 12 種沒有 cycle**，另外 **4 種還是有 cycle**：拿掉的兩個轉彎剛好是同一對方向的「去」和「回」（例：N → E 和 E → N）。原因：「把所有 cycle 化成簡單的順、逆時針 cycle」這一步不完全成立，還有更複雜的 cycle（例如繞成 8 字形、同時用到兩種方向的轉彎）。影片點名的 west first、north last、negative first 都在沒有 cycle 的 12 種裡。見 [§19.1](#191-turn-model真的-16-種都沒有-deadlock-嗎)。

---

## 12. 用 virtual channel 避免 deadlock：dateline

[43:27] VC 最早的用途之一其實就是**避免 deadlock**（後來變成次要用途）。一個很老的演算法：

- ring，每條實體 channel 有 **2 個 VC**，編號 0、1。
- packet 一律從 **VC 0** 開始，一直用 VC 0。
- 畫一條想像的線：**dateline**。head flit 一跨過 dateline，就換到 **VC 1**，之後只用 VC 1，**永遠不回 VC 0**。

[45:05] 引入一種**不對稱**。證明：VC 1 上的 flit **永遠不會等** VC 0（「被升級了，只看人生更高的層次」）；VC 0 可能等 VC 1，但不會反過來 → 不可能循環等待 → **沒有 deadlock**。

（本機建了 ring 的 VC dependence graph，見 [§19.2](#192-dateline)。）

【課本補充】課本 p.377 說明了 dateline，也把這個方法跟 mesh 等其他 topology 結合。

---

## 13. Router 的輸入輸出

[46:09] 不在邊上的 router：

| | 5 個 |
|---|---|
| 輸入 | 來自 N、S、E、W 四個方向的 router ＋ 本地的 tile |
| 輸出 | 往 N、S、E、W 的實體 channel ＋ 本地的 tile |

[47:16] 每條實體 channel 邏輯上可以分成好幾個 VC。

---

## 14. Stage 1：buffer write

[47:51] flit 進 router，**先要存起來**。k 個 VC 時兩種做法：

| 做法 | 說明 |
|---|---|
| **分開的 buffer**（split） | k 條各自的 flit queue；缺點：每條很小（例如 8 格），12 個 flit 時有 4 個只能留在發送端 |
| **共用的 buffer**（shared） | 一大塊 circular buffer，動態地切成各 VC 的 queue（各有 head、tail，不重疊）；彈性大：不是每個 VC 都會同時滿，一條 queue 可以用別人的空間長大 |

[62:28] head flit 進來時已經知道自己的 VC 編號（上一個 router 蓋了章；從本地 tile 來的，進來前就分好了）。

---

## 15. Stage 2：route computation

[50:29] 算出**下一站**是哪個 router。**只對 head flit 做**；body、tail flit 跟著走。所以整個 router 的**關鍵路徑**在 head flit 上。

[51:31] 用 **routing table**：給目的地，查下一站（照 routing 協定）。

[52:08] 例：3 × 3 mesh 的節點 5（中間），用 **north last**（禁止 N → E、N → W）：

```
1 2 3
4 5 6
7 8 9
```

| 目的地 | 下一站 | 為什麼 |
|---|---|---|
| 1 | W | 不能先 N 再轉 W |
| 2 | N | |
| 3 | E | 不能先 N 再轉 E |
| 4 | W | |
| 6 | E | |
| **7** | **S 或 W** | 兩種轉彎都允許 |
| 8 | S | |
| **9** | **S 或 E** | |

[53:10] 有選擇時，用**擁塞資訊**：從 credit 的數量、buffer 清空的速度，或上次用這條 channel 花了多久、flit 能不能連續送。

[55:50] routing table 是寫死的（協定是寫死的），而且可以證明沒有 deadlock。

【課本補充】課本 p.381 的例題就是這張表。

---

## 16. Stage 3：VC allocation

[56:21] 算出要往 E 走，往 E 的實體 channel 有好幾個 VC（例如 4 個）：選哪一個？要看之前分給其他 packet 的 VC，**平均分配**（load balancing）。選好的 VC 寫進 buffer 裡那個 head flit。也只對 head flit 做。

---

## 17. Stage 4：switch allocation

[57:23] 5 條實體 channel × 2 個 VC = **10 個 VC** 要搶 switch 的輸出 port → switch 是 **10 × 5**（5 個輸出：N、S、E、W、local）。

[59:09] **Switch** 把輸入接到輸出；**一個輸出只能接一個輸入**。M × N 的 switch：每一行（輸出）只有一列（輸入）能接上；用 **pass transistor**，控制 bit 是 1 就接通。

[59:40] 先決定這些對應：**預留**通往輸出 port 的路徑。

[60:10] **公平**很重要：某個 VC 的 flit 不能永遠等 → 這裡就是 **starvation freedom**：每個 VC 都保證拿得到 switch。

[60:57] 只有**輸入**有 buffer，輸出沒有（**input buffered**）→ 拿到 switch 之後在輸出不會再卡住。

[63:32] 兩種層級：

| 層級 | 做法 |
|---|---|
| **flit 層級** | 每個 flit 各自搶 switch：送了第 1 個，換別的 VC，再回來送第 2 個……（body、tail 還是跟著 head 的路） |
| packet 層級 | 整個 packet 都到了，一次送完；switch 只分配一次。比較少見 |

[66:23] 還有一個 **flow control** 模組（credit 或 on/off），它的資訊也送進 switch allocator：沒有 credit 就不用分配 switch（反正送不出去）。有 credit 就能「無所畏懼地」送：確定對方有 buffer，不會被丟掉。

---

## 18. Stage 5：switch traversal

[61:25] 通過 switch，從輸出 link 送出去。「很笨的一級」：決定在 stage 4 都做好了，沒有 buffer、不等、不會卡。**tail flit 離開 router 時，收回那個 VC**。

[66:52] 整條 pipeline：

```
buffer write → route computation → VC allocation → switch allocation → switch traversal
（RC、VA、SA 對 head flit 做；body、tail 跟著）
```

5 級可能要 5 個 cycle，相當慢。下一講看怎麼最佳化，以及 switch 的階層式設計（allocator、arbiter）。

---

## 19. 【實測】

程式是 `experiments/lecture29/turns.py`：把 channel dependence graph 真的建出來，用 DFS 找 cycle。

- 4 × 4 mesh 上，每一條**有方向的 link** 是一個 channel（例如 (1,1) → (2,1) 是一條往 E 的 channel）。
- CDG 的邊：channel c1 走到某個 router 之後，接著可以走 c2（直走一定可以、不能迴轉、轉彎要看 routing 允不允許）。
- CDG 沒有 cycle → 不可能 deadlock。

### 19.1 Turn model：真的 16 種都沒有 deadlock 嗎

```
mesh 4 × 4：
   所有轉彎都允許：CDG 有 cycle？ True
   XY routing（只允許 E/W → N/S，共 4 個轉彎）：CDG 有 cycle？ False
   順、逆時針各拿掉一個轉彎（4 × 4 = 16 種）：沒有 cycle 的 12 種，還有 cycle 的 4 種
      還有 cycle：拿掉 N→E 和 E→N
      還有 cycle：拿掉 E→S 和 S→E
      還有 cycle：拿掉 S→W 和 W→S
      還有 cycle：拿掉 W→N 和 N→W
   west first（拿掉 S→W、N→W）：CDG 有 cycle？ False
   north last（拿掉 N→E、N→W）：CDG 有 cycle？ False
   negative first（拿掉 N→W、E→S）：CDG 有 cycle？ False
```

- 所有轉彎都允許 → 有 cycle（可能 deadlock）；**XY routing 沒有 cycle**，跟 [§9](#9-xy-routing) 的證明一致。
- **West first、north last、negative first 都沒有 cycle**。
- ⚠️ 但 16 種裡**有 4 種還是有 cycle**（[§11](#11-adaptive-routingturn-model) 的 ⚠️）。這 4 種拿掉的剛好是「同一個角的去和回」：例如拿掉 N → E 和 E → N，剩下的 6 個轉彎還能組出一個**同時用到順時針和逆時針轉彎**的大 cycle，不能化成單純的順時針或逆時針 cycle。所以「順、逆時針各拿掉一個」是**必要**的，但不是**充分**的；還要挑對組合。

### 19.2 Dateline

一個 8 個節點的單向 ring，每條 link 2 個 VC：

```
ring 8 個節點、每條 link 2 個 VC：
   VC 隨便用：CDG 有 cycle？ True
   dateline（從 VC 0 開始，跨過 dateline 就換到 VC 1、不再回去）：CDG 有 cycle？ False
```

- VC 隨便用：所有的 VC 繞著 ring 形成 cycle（[§7](#7-channel-dependence-graph) 說的「有 VC 也會 deadlock」）。
- 用 dateline：VC 0 的依賴到 dateline 就斷了（轉到 VC 1），VC 1 的依賴也不會再繞回 dateline 的另一邊 → **沒有 cycle**，跟 [§12](#12-用-virtual-channel-避免-deadlockdateline) 一致。
- 這裡的模型假設一個 packet 不會繞 ring 超過一圈（不會跨兩次 dateline）。⚠️ 第一版的程式沒有加這個條件，VC 1 自己就繞成一圈，結果顯示有 cycle；加上去才對。這個條件在影片裡沒有明講，但 dateline 的證明需要它。

---

## 20. 一頁速查表

| 主題 | 重點 |
|---|---|
| 為什麼 routing | 不能丟 flit → 排隊 → 擁塞 → 要繞路 |
| 好的路徑 | 最短時間（不一定最短路徑）、公平 |
| 三個問題 | deadlock（循環等待、誰都不動）、livelock（一直動、到不了）、starvation（某個永遠不動） |
| 關係 | starvation freedom ＋ 沒有 livelock ⇒ deadlock freedom；反之不成立 |
| Aging | 待越久優先權越高、改走最短路；局部的；課本：hop count 門檻 |
| RDG | packet ↔ channel 的二分圖；cycle = deadlock |
| CDG | channel → channel；有 VC 也會有 cycle |
| Turn graph | 保留方向；CDG 的邊 = 直走或轉彎；TG 沒有 cycle ⇒ 沒有 deadlock |
| XY | 先 x 後 y；4 個轉彎；沒有 deadlock；沒有 path diversity |
| Valiant | 隨機點 P、兩段 XY；路長；minimal 版：P 在小窗口裡 |
| Turn model | 順、逆時針各禁一個 → 6 個轉彎；west first、north last、negative first |
| Dateline | VC 0 開始，跨線換 VC 1 不回頭；不對稱 → 沒有 deadlock |
| Router | 5 進 5 出（N、S、E、W、local）；split 或 shared buffer |
| 5 級 pipeline | BW → RC（routing table，只對 head flit）→ VA（load balancing）→ SA（10 × 5 switch，公平；看 credit）→ ST（沒有 buffer，tail 走了收回 VC） |
| 實測 | 4 × 4 mesh：XY 和三種有名的 turn model 都沒有 cycle，但 16 種裡有 4 種還有 cycle（⚠️ 影片、課本說都沒有）；dateline 讓 ring 的 VC 圖沒有 cycle |

---

## 21. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Topology、mesh、ring | [第 27 支](lecture27_note.md) |
| Flow control、credit、VC | [第 28 支](lecture28_note.md) |
| Allocator、arbiter、pipeline 的最佳化 | [第 30 支](lecture30_note.md) |
| NUCA、synthetic traffic | [第 31 支](lecture31_note.md) |
| 多執行緒裡的 deadlock、lock | [第 37 支](lecture37_note.md)（Ch9） |
| Select unit 的公平、starvation | [第 11 支](lecture11_note.md) |

---

## 附：本講一句話總結

> **NoC 不能丟 flit，flit 只好在 router 裡排隊而造成擁塞，routing 要能繞路，但不能產生 deadlock（循環等待）、livelock（一直繞到不了）或 starvation（某個永遠不動）：starvation freedom 加上沒有 livelock 就推得出 deadlock freedom，實務上則用協定避免 deadlock、用 aging 處理另外兩個；分析 deadlock 用 resource dependence graph、channel dependence graph 和保留方向的 turn graph，只要不可能建出有 cycle 的 turn graph 就沒有 deadlock，於是 XY routing（只有 4 個轉彎、沒有路徑選擇）、隨機中繼點的 Valiant routing、只禁掉兩個轉彎的 adaptive routing（west first、north last、negative first）和用 dateline 換 virtual channel 的方法都沒有 deadlock；router 本身是一條 5 級的 pipeline：buffer write、只對 head flit 做的 route computation（查 routing table）和 VC allocation、要公平並參考 credit 的 switch allocation、最後是沒有 buffer 的 switch traversal。本機把 4 × 4 mesh 的 channel dependence graph 建出來檢查：XY 和那三種有名的演算法都沒有 cycle，但「順、逆時針各禁一個轉彎」的 16 種裡其實只有 12 種沒有 cycle，禁掉同一個角的去和回的 4 種還是會 deadlock，跟影片和課本的說法不同；dateline 則確實讓 ring 上的 VC 依賴圖沒有 cycle。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 8: The On-chip Network (Part III/V) -- Routing Algorithms and 5-Stage Router Pipeline 的 YouTube 自動字幕（1:07:49）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch8 §8.3–8.4.5（p.361–387）整理。實測為本機（Python 3.10）模擬結果。*
