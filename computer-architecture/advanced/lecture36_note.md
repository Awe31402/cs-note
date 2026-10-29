# Advanced Computer Architecture 第三十六支影片完整筆記：Multicore（五）：snoopy 協定（write-update、MSI、MESI、MOESI）與 directory 協定（Chapter 9, Part V）

> **課程**：Advanced Computer Architecture — Chapter 9: Multicore Systems (Part V) -- Coherence protocols, Snoopy and Directory based schemes（播放清單第 36 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（1:05:28）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch9** §9.4.1–9.4.5（p.459–475）
> **章號**：影片 Chapter 9 = 課本 Ch9。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕把「cache coherence」寫成「cash Corin／cash cence」、「MESI」寫成「Messi／message」、「MOESI」寫成「moi」、「NoC」寫成「nooc」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 要守住的兩個公理](#1-要守住的兩個公理)
- [2. Snoopy bus](#2-snoopy-bus)
- [3. 狀態、事件、訊息](#3-狀態事件訊息)
- [4. Write-update 協定](#4-write-update-協定)
- [5. Write-invalidate 協定（MSI）](#5-write-invalidate-協定msi)
- [6. MESI：加一個 E 狀態](#6-mesi加一個-e-狀態)
- [7. MOESI：加一個 owner 狀態](#7-moesi加一個-owner-狀態)
- [8. MOESI 的 St、Se 暫時狀態與 Probe](#8-moesi-的-stse-暫時狀態與-probe)
- [9. Directory 協定](#9-directory-協定)
- [10. Directory entry 的狀態轉換](#10-directory-entry-的狀態轉換)
- [11. 【原始碼】Linux 怎麼避開 false sharing](#11-原始碼linux-怎麼避開-false-sharing)
- [12. 【實測】](#12-實測)
- [13. 一頁速查表](#13-一頁速查表)
- [14. 與後續講次的連結](#14-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
目標：讓 distributed cache 看起來像 shared cache ＝ 守住 write serialization ＋ write propagation
snoopy bus（2–8 個 cache）：
  write-update（MSI）：每個寫都廣播，大家更新 → 耗電
  write-invalidate（MSI）：single writer 或 multiple readers；S→M 要送 WrX.u 讓別人 invalidate
  MESI：加 E（只有我有），E→M 不用送訊息
  MOESI：加 O（owner 負責送資料）→ 不用 arbitration、M→O 不用 write back；owner 不見時用 St、Se、Probe
directory（大系統，NoC）：每個 block 記 state ＋ sharer 的 bit vector；directory 決定寫的順序
```

---

## 1. 要守住的兩個公理

[00:00] coherence 協定：讓 distributed cache 表現得像 shared cache。從最簡單的 write-update 協定開始。

[00:49] 要在硬體裡守住 PLSC → coherence 的兩個公理（[第 34 支 §11](lecture34_note.md#11-coherence-的兩個公理)）：

| 公理 | 意思 |
|---|---|
| **Write serialization**（WS） | 同一個位置的寫，所有 thread 看到的順序一樣（寫 1、2、3……大家都同意）。所以 execution witness 裡 ws、fr 是 global（[第 35 支](lecture35_note.md)） |
| **Write propagation**（WP） | 所有寫最後都會完成：寫的訊息**永遠不會遺失** |

---

## 2. Snoopy bus

[02:19] 最簡單的系統：所有 sister cache 接在一條 **shared bus** 上。

- bus 就是一組**銅線**。cache 1 不是「送訊息給」cache 2，而是把 bus 設成它要送的訊息，**所有 cache 都讀得到**。
- [02:44] 一個寫、很多讀：大部分 cache 會忽略（不是給它的），有興趣的直接讀。bus 本身就是**廣播**媒介。
- [03:11] 其他 cache 等於在**偷聽**（snoop）→ 叫 **snoopy bus**；用它的協定叫 **snoopy protocol**。
- [05:35] 假設有一個 **bus master**（聰明的電路）：一次只讓一個人寫；寫了馬上所有 cache 都看得到。

規模：

| | 影片 | 課本 |
|---|---|---|
| 能撐幾個 cache | [03:11]「不會超過 8 個」；[08:46]「不超過 4 到 8 個 core」；2–4 個可以用 | p.472：「2–4 個 cache 用 bus 是好主意」 |

---

## 3. 狀態、事件、訊息

[03:38] 每個 cache line 加三個狀態（之後一直沿用）：

| 狀態 | 意思 |
|---|---|
| **M**（modified） | 被寫過 |
| **S**（shared） | **可能**跟別的 sister cache 共用 |
| **I**（invalid） | 沒有合法的資料 |

[04:07] 狀態圖上的標記：「**事件 | 動作**」（發生某事件就送某訊息、做某事），或「**訊息 | 動作**」（從 bus 收到某訊息就做某事）。「−」表示什麼都不做。

[05:06] 本地的事件和動作：

| 符號 | 意思 |
|---|---|
| Rd | 讀請求 |
| Wr | 寫請求 |
| Evict | 要把 block 趕出去 |
| Wb | write back 到下一層 |
| Update | 從 bus 收到更新（例如 x = 5），更新本地副本 |

[06:31] bus 上的訊息（之後換成 NoC 也一樣）：

| 訊息 | 意思 |
|---|---|
| **RdX** | read miss |
| **WrX** | write miss：寫之前要先拿到整個 block 的副本（[第 21 支](lecture21_note.md)） |
| **WrX.u** | [07:26] 寫的**升級**（upgrade）：我已經有副本，只是要寫的**權限** |
| **Broadcast** | 廣播一個寫：「我把 x 設成 5 了」 |
| **Send** | 把 block 的副本送給要的 cache（如果自己有） |

[07:03] 為什麼讀和寫的 miss 要分開？讀和寫的處理不一樣：同一位置的寫的**順序**很重要。

[08:15] 包含 x 的 block 可能在好幾個 cache 裡，這些**副本**要保持同步（coherent）。

【課本補充】課本 p.464 Table 9.3 是所有符號的對照表。

---

## 4. Write-update 協定

### 4.1 讀

[09:44]（課本圖 9.34）

| 狀態 | 事件 | 動作 → 新狀態 |
|---|---|---|
| I | Rd | 送 **RdX**；有人回就用它的，[11:06] 一段時間沒人回就從下一層讀 → **S** |
| S | Rd | − （直接讀） |
| S | Evict | − → I：**seamless eviction**（[10:11] 直接丟掉，什麼都不用做）。「seamless eviction 之後會變成很大的事」 |

### 4.2 寫

[11:32]（課本圖 9.35–9.37）

| 狀態 | 事件 | 動作 → 新狀態 |
|---|---|---|
| I | Wr | 送 **WrX**，拿到副本 → **M** |
| M | Rd | − |
| M | Wr | **先 Broadcast 再寫**（所以叫 update 協定） |
| M | Evict | **Wb**（寫過了，不寫回去更新會遺失）→ I：**不是** seamless |
| S | Wr | **Broadcast** → **M** |

[12:55] 為什麼**先廣播、再寫**，不能反過來？兩個寫同時發生：一個要 x = 3、另一個要 x = 5。如果各自先寫再廣播，一個 cache 先看到 3，另一個先看到 5 → 違反 WS。[13:46] 先廣播的話，bus master 讓其中一個先拿到 bus，大家都照拿到 bus 的順序記錄 → **全域的寫順序**。

【課本補充】課本 p.463 把這個錯誤的做法講得更細：C1 寫 1、C2 寫 2，C1 看到 (1, 2)、C2 看到 (2, 1)。改成「拿到 bus 才廣播，連自己也是收到廣播時才真的寫」→ write 變成 atomic，所有 cache 同時看到。課本也說 write-update 協定**永遠不會**從 M 轉回 S。

### 4.3 從 bus 收到的訊息

[17:30]（課本圖 9.38）

| 狀態 | 收到 | 動作 |
|---|---|---|
| S、M | Broadcast | **Update**（更新本地的值） |
| S、M | RdX、WrX | **Send**（送副本） |

[18:29] 細節：兩個 cache 都有副本，第三個要，**誰送**？[18:56] 誰先拿到 bus 誰送，其他人偷聽到「已經送了」就閉嘴。

### 4.4 為什麼對、為什麼慢

[19:22] 公理怎麼守住：

- **WS**：一次只有一個 cache 能用 bus → 拿到 bus 的順序就是全域的寫順序。
- **WP**：bus master **不讓人餓死**（每個寫最後都拿得到 bus）→ 寫都會完成。

[15:10] 代價：**每個寫都要廣播**（時間、電、效能）。[17:03] 而且每個 cache 看到廣播都要**查自己有沒有**那個 block，有就更新 → 第二個耗電來源。「這是可以避免的。」

（本機的模擬見 [§12.1](#121-四種-snoopy-協定的模擬)。）

---

## 5. Write-invalidate 協定（MSI）

### 5.1 想法

[19:50] 實際上**沒有那麼多共用**：變數放在 critical section 裡保護，通常是一個人存取一陣子，然後換別人。常見的樣式：

| 情況 | |
|---|---|
| **single writer** | 一個寫的人，沒有讀的人 |
| **multiple readers** | 很多讀的人，沒有寫的人 |

[20:44] 想像每個 block 有一個 **token**，拿到 token 的 cache 才能寫：

- token 傳遞的順序 = 同一個位置的**全域寫順序** → WS。
- 想寫的 cache 最後一定拿得到 token → 不會餓死 → WP。

【課本補充】課本 p.466：**conflicting access**：同一個 block 的兩個存取，至少一個是寫。write-invalidate 協定**不支援同時又衝突的存取**。

### 5.2 讀、寫、趕出

[22:00]（課本圖 9.39）三個狀態一樣，意思稍微變了：**只有一個 cache 能把 block 放在 M**，那時別的 cache 都沒有合法副本；S 可以很多個，但只能讀。

| 狀態 | 事件 | 動作 → 新狀態 |
|---|---|---|
| I | Rd | RdX → S |
| I | Wr | WrX → M |
| S | Rd | −（[23:54] S 是**唯讀**狀態） |
| S | Wr | **WrX.u**（upgrade）→ 別人 invalidate → **M** |
| S | Evict | −（seamless） |
| M | Rd、Wr | **−**：[24:49]「好玩的地方」：M 裡讀寫都**不用送任何訊息** |
| M | Evict | Wb |

[25:17] 這就是它省電的地方：只有一個 cache 有 M，確定別人都沒有副本，想寫就一直寫。

[26:36] 讀和寫的人**不能並存**；要寫就必須是**唯一的擁有者**（M 保證這一點）。cache **進入 M 的順序**是全域看得到的順序 → WS。

### 5.3 從 bus 收到的訊息

[27:35]（課本圖 9.40）

| 狀態 | 收到 | 動作 → 新狀態 |
|---|---|---|
| S | RdX | Send（如果沒人先送） |
| S | **WrX.u** | **→ I**（invalidate 自己：協定名字的由來） |
| S | WrX | Send → I |
| M | WrX | Send → I。[28:53] **不用** write back：改過的內容整個交給對方了 |
| M | RdX | **Send ＋ Wb** → S |

[29:18] M 收到 RdX 為什麼要 write back？S 是 **seamless eviction**：如果 M → S 不寫回去，之後 S 的副本被直接丟掉，更新就不見了。

---

## 6. MESI：加一個 E 狀態

[30:40] MSI 的浪費：一個 core 讀一個**沒人共用**的 block（進 S），之後要寫，還是要送 WrX.u 才能到 M。「先讀後寫」很常見，這是多出來的開銷。

[31:10] 加 **E**（exclusive）狀態：這個 block **只有這個 cache 有**。

[31:38] 怎麼進 E：read miss 送了 RdX，**沒有任何 sister cache 有副本**，從下一層讀 → 直接進 **E**。

[33:04]（課本圖 9.41）讀、寫、趕出：

| 狀態 | 事件 | 動作 → 新狀態 |
|---|---|---|
| I | Rd（從 sister cache 拿到） | RdX → S |
| I | Rd（從**下一層**拿到） | RdX → **E** |
| E | Rd | − |
| E | **Wr** | **−** → **M**：[34:24]「完全 seamless，bus 上什麼都不送」：這是這個協定最重要的新東西 |
| E | Evict | −（沒寫過，seamless） |
| M、S、I 其他 | | 跟 MSI 一樣 |

[35:47]（課本圖 9.42）從 bus 收到的：

| 狀態 | 收到 | 動作 → 新狀態 |
|---|---|---|
| E | WrX | Send → I（對方一定沒有副本） |
| E | RdX | Send → **S**（多個 cache 只讀 → 都要在 S） |

---

## 7. MOESI：加一個 owner 狀態

[37:10]「還是不滿意。」兩個機會：

| | 問題 |
|---|---|
| **機會一** | [37:45] 好幾個 cache 有副本時**誰送資料**？大家都向 bus master 要 bus（**arbitration**）；先拿到的送，其他人偷聽到之後取消。要額外的狀態機，花時間、電、面積 |
| **機會二** | [38:38] M → S 時一定要 **write back**（為了 S 的 seamless eviction）。能不能晚一點、**懶惰地**做，等下一層比較閒的時候？ |

【課本補充】課本 p.470 Definition 75：**arbitration**：從很多個有興趣的單位中挑一個。

[40:00] 加 **O**（owner）狀態 → **MOESI**：資料可以共用，很多 cache 有副本，但「**我是 owner，送資料是我的工作**」。

[40:58] 老師故意**先講 bus 訊息**再講讀寫（比較好懂）。（課本圖 9.43）

| 狀態 | 收到 | 動作 → 新狀態 |
|---|---|---|
| **E、M** | RdX | Send → **O**（以前是 → S） |
| E、M | WrX | Send → I |
| **O** | RdX | **Send**（owner 回應；不用跟別人搶 bus） |
| O | WrX | Send → I |
| O | WrX.u | → I |
| **S** | RdX、WrX | **−**：[44:00] 有 owner 會送，S 不用管 |
| S | WrX.u | → I |
| S | **Probe** | **Send**（處理特殊情況，見 [§8](#8-moesi-的-stse-暫時狀態與-probe)） |

- [42:17] O 也是**唯讀**狀態。
- [43:11]「我們不再在 bus 上競爭」→ 這部分非常省電。[44:56] 送副本時不用 arbitration，狀態機簡單很多。

[45:25]（課本圖 9.44）讀、寫、趕出：

| 狀態 | 事件 | 動作 → 新狀態 |
|---|---|---|
| O | Rd | − |
| O | Wr | **WrX.u** → M（要 invalidate 別人的副本） |
| O | Evict | **Wb**：[48:05] O 是從 M 來的（M → O **沒有** write back），可能有**改過**的資料 |
| M、E、S | | 跟 MESI 一樣 |

- [46:19] 沒有從本地事件**進入** O 的箭頭：只有別人要讀時才會變成 owner。
- [48:33] 【口頭】聰明的版本：進了 O 之後，趁下一層有空時**偷偷**寫回去。
- [49:01] 資料一直沒離開 cache 的話：M → O → M → O……完全不用 write back。

---

## 8. MOESI 的 St、Se 暫時狀態與 Probe

[49:28] O 被趕出（寫回）之後的問題：別的 cache 可能還有 S 的副本，但**沒有 owner** 了。

[49:55] 【口頭】複雜的版本可以搜尋其他 cache，指定一個副本當新的 owner，「不難，但開銷比較大」。這裡的做法：加兩個**暫時狀態** St、Se。

[50:45] I 的 write miss：invalidate 所有副本；有 owner 就由 owner 送；沒有的話某個 cache 送，或直接從下一層讀。[51:12]「寫通常不在 critical path 上，從下一層讀也可以。」

[51:39] I 的 **read miss**：

```
I ──Rd | RdX──▶ St ──有人回（owner 存在）──▶ S
                 │
                 └─逾時 | Probe──▶ Se ──有 S 的 cache 回（arbitration 一下）──▶ S
                                    │
                                    └─逾時 | 從下一層讀──▶ E
```

- [52:04] St 逾時 = 沒有 owner（沒有 O、E、M）→ 送 **Probe**。
- [52:52] 有好幾個 S 的副本就像以前一樣 arbitration，「但 Probe 很少見，額外的電不多」。
- [53:21] Se 也逾時 = 沒有任何 cache 有副本 → 從下一層讀 → **E**。

[53:45] 理想上應該要有 **ownership transfer**：有一個 **MESIF** 協定（F = forwarding）做了一部分，「有空可以去看看」。

【課本補充】課本 p.471–472：沒有 St、Se 還有一個**正確性**問題：沒有 owner 時從下一層讀進來，該進 E 還是 S？不知道別的 cache 是只有 S 的副本，還是根本沒有，「沒辦法分辨」。St、Se 就是用來分辨的。課本也說 O 被趕出時，如果別的 cache 有副本，理論上**不用** write back，只要轉移 ownership，但那需要 arbitration；MESIF 的 F 狀態做了 ownership transfer，「請讀者上網查」。（本機沒有 MESIF 的資料，**待補**。）

---

## 9. Directory 協定

[54:37] bus 的協定都有規模的問題 → **directory 協定**：「大系統的 state of the art」。

[55:06] 不用 bus，用一個專門的結構 **directory**：它是 NoC 上的一個節點。cache 散在 NoC 各處，directory 是一個**集中**的協調者：跟 cache 和下一層收送訊息，指揮整個協定。

| | snoopy | directory |
|---|---|---|
| 小系統 | **比較快** | 一開始開銷就比較大 |
| 大系統 | 撐不住 | **撐得住** |

[56:24] 基本想法：集中的 directory 為**每個 block** 記一份 **sharer**（有這個 block 副本的 sister cache）的清單。

[56:53] directory entry：

```
| state | block address | list of sharers（bit vector：第 i 個 bit = 第 i 個 sister cache 有沒有） |
```

例：64 個 core（每個 core 一個 sister cache）→ 64 個 bit。

[57:51] directory 收到的訊息一樣：RdX、WrX、WrX.u、Evict。[58:15] **每個 cache 的狀態圖不變**，只是原本丟到 bus 上的訊息，改成**送給 directory**。

| 收到 | directory 做什麼 |
|---|---|
| RdX | 找一個 sharer，請它把 block 送過去 |
| WrX | 請所有 sharer invalidate，也請其中一個把副本送給要寫的 cache |
| Evict | [59:08] 有的版本不通知 directory，有的會；這裡假設會 → 把它從 sharer 清單刪掉 |

【課本補充】課本 p.472–475：

- 為什麼不能直接用 NoC：bus 自然地幫所有請求排了順序（WS 很容易），NoC 可以同時跑很多對傳輸，頻寬大很多，**卻失去了排順序的能力**。所以加一個 directory 節點專門**排順序**（serialization）。
- directory 像 cache 一樣組織：有 tag array 和 data array。
- 每個 cache 一個 bit 的 sharer 清單叫 **fully mapped scheme**（Definition 76），之後會講更省的做法。
- NoC 上廣播非常貴，所以要明確記 sharer，改成點對點送訊息。
- directory 處理寫的順序 = 全域的寫順序（WS）；directory 要公平、不能無限延後寫請求（WP），「用一個 **FIFO queue** 就做得到」。

---

## 10. Directory entry 的狀態轉換

[59:34] directory 的三個狀態：

| 狀態 | 意思 |
|---|---|
| **U**（uncached） | 沒有 cache 有 |
| **S**（shared） | 一個或多個 cache 用 S 狀態持有 |
| **E**（exclusive） | 一個 cache 用 E **或 M** 持有 |

假設每個 cache 跑 **MESI**：[59:34]「有 directory 就不需要 owner。」

[60:00]（課本圖 9.47）從 U、S 出發（P = 送訊息來的 cache；LL = lower level）：

| 狀態 | 收到 | 動作 → 新狀態 |
|---|---|---|
| U | RdX | sharers = {P}；從 LL 讀 → **E** |
| U | WrX | sharers = {P}；從 LL 讀 → **E** |
| S | Evict | sharers −= {P}；清單空了 → **U** |
| S | RdX | 把 RdX 轉給**一個** sharer，請它**直接**把副本送給 P（[61:21] 透過 NoC 直接送，不經過 directory，比較有效率）；sharers += {P} |
| S | WrX.u | 把 WrX.u 送給 P 以外的**所有** sharer（invalidate，不用送副本）；sharers = {P} → **E** |
| S | WrX | 送 WrX 給所有 sharer；請其中一個把副本送給 P；sharers = {P} → **E** |

[60:26] 為什麼 directory 只有一個「E」，不分 E 和 M？**E → M 是靜悄悄的**（不送訊息），directory 根本不知道有沒有發生。

[63:05]（課本圖 9.48）從 E 出發：

| 狀態 | 收到 | 動作 → 新狀態 |
|---|---|---|
| E | Evict | sharers = {} → **U** |
| E | WrX | 送 WrX 給那個 sharer，請它把（可能改過的）副本送給 P；sharers = {P}，**還是 E** |
| E | RdX | 送 RdX 給那個 sharer，請它送副本；sharers += {P} → **S** |

[63:59]「沒有什麼高深的東西，非常簡單直接」，只是多了一個專門的 directory 為**整個系統的每個 block** 記狀態：這會變成開銷。

[64:27] 下一講：directory 的進階版、coherence 協定可以拿來實作 synchronization 等更高階的操作。

---

## 11. 【原始碼】Linux 怎麼避開 false sharing

coherence 是以 **block**（cache line）為單位的：兩個 core 寫**同一個 line 裡不同的變數**，協定還是會把 line 搶來搶去（**false sharing**，本機實測見 [§12.2](#122-本機的-false-sharing)）。Linux 用對齊和 padding 把常常被寫的欄位放到不同的 line。

`include/linux/cache.h:40-50`：

```c
#ifndef ____cacheline_aligned
#define ____cacheline_aligned __attribute__((__aligned__(SMP_CACHE_BYTES)))
#endif

#ifndef ____cacheline_aligned_in_smp
#ifdef CONFIG_SMP
#define ____cacheline_aligned_in_smp ____cacheline_aligned
#else
#define ____cacheline_aligned_in_smp
#endif /* CONFIG_SMP */
#endif
```

- `SMP_CACHE_BYTES` = `L1_CACHE_BYTES`（`include/linux/cache.h:13`）；x86 是 `1 << CONFIG_X86_L1_CACHE_SHIFT`（`arch/x86/include/asm/cache.h:8`），一般的 x86-64 設定是 6 → **64 B**。
- 只有 SMP（多處理器）才對齊：單處理器沒有別的 cache 跟它搶，對齊只是浪費空間。

`include/linux/mmzone.h:819-832`（記憶體 zone 的結構）：

```c
	/* Write-intensive fields used from the page allocator */
	CACHELINE_PADDING(_pad1_);

	/* free areas of different sizes */
	struct free_area	free_area[MAX_ORDER];

	/* zone flags, see below */
	unsigned long		flags;

	/* Primarily protects free_area */
	spinlock_t		lock;

	/* Write-intensive fields used by compaction and vmstats. */
	CACHELINE_PADDING(_pad2_);
```

`CACHELINE_PADDING` 是一個大小 0、但要求對齊到 cache line 的結構（`include/linux/cache.h:93-96`）：插在兩群欄位中間，讓「page allocator 常寫的」和「compaction、vmstats 常寫的」落在不同的 line，不同 core 各自寫自己那群時不會互相 invalidate。

---

## 12. 【實測】

程式在 `experiments/lecture36/`：`coh.py`（協定模擬）、`fs.c`（false sharing）。

### 12.1 四種 snoopy 協定的模擬

`coh.py`：4 個 cache 接在 bus 上，照課本圖 9.37–9.44 實作 write-update、MSI、MESI、MOESI（含 St／Se／Probe）。**每一步都檢查**：① 每個讀都拿到最新寫的值；② invalidate 協定裡有 M 或 E 時沒有別的副本、O 最多一個。六種樣式全部跑完，**沒有任何檢查失敗**。

計數（bus 訊息含 RdX、WrX、WrX.u、Broadcast、Probe）：

| 樣式 | 協定 | bus 訊息 | Broadcast | 別人被迫更新 | WrX.u | Probe | arbitration | write back |
|---|---|---|---|---|---|---|---|---|
| **私有**：每個 core 先讀再寫自己的 block（800 次） | update | 404 | 400 | 0 | 0 | 0 | 0 | 0 |
| | MSI | 8 | 0 | 0 | **4** | 0 | 0 | 0 |
| | MESI | **4** | 0 | 0 | **0** | 0 | 0 | 0 |
| | MOESI | 8 | 0 | 0 | 0 | **4** | 0 | 0 |
| **生產者-消費者**：core0 寫，core1–3 讀（400 次） | update | **104** | 100 | 297 | 0 | 0 | 2 | 0 |
| | MSI | 400 | 0 | 0 | 99 | 0 | **200** | **100** |
| | MESI | 400 | 0 | 0 | 99 | 0 | 200 | 100 |
| | MOESI | 400 | 0 | 0 | 99 | 0 | **0** | **0** |
| **遷移**：輪流讀再寫同一個 block（800 次） | update | 404 | 400 | 1194 | 0 | 0 | 2 | 0 |
| | MSI | 800 | 0 | 0 | 400 | 0 | 0 | 399 |
| | MESI | 799 | 0 | 0 | 399 | 0 | 0 | 399 |
| | MOESI | 800 | 0 | 0 | 399 | 1 | 0 | **0** |
| **core0 連寫 100 次**（別人先讀過一次） | update | **104** | 100 | **300** | 0 | 0 | 2 | 0 |
| | MSI | **5** | 0 | 0 | 1 | 0 | 2 | 0 |
| | MESI | 5 | 0 | 0 | 1 | 0 | 2 | 0 |
| | MOESI | 6 | 0 | 0 | 1 | 1 | 0 | 0 |

觀察：

- **write-update 的問題**（[15:10]）：「core0 連寫」時 100 次寫 → 100 次廣播、別人被迫更新 300 次；invalidate 協定只要 1 次 WrX.u。
- **MESI 的好處**（[30:40]）：「私有」樣式 MSI 每個 block 要多一個 WrX.u，MESI 完全不用（E → M 靜悄悄）。
- **MOESI 的好處**（[37:45]、[38:38]）：「生產者-消費者」裡 MSI、MESI 有 200 次 arbitration、100 次 write back，MOESI 都是 **0**（owner 負責送，M → O 不用寫回）。「遷移」樣式也省掉 399 次 write back。
- **MOESI 的代價**：冷的 read miss 沒有 owner，要等 St 逾時再送 Probe（「私有」多了 4 個 Probe，每個還要等兩次逾時）。
- ⚠️ 「生產者-消費者」樣式下 **write-update 反而最省**（104 vs 400 個訊息）：寫一次、三個人讀，invalidate 協定每輪都要 invalidate 再重新 miss。影片 [19:50] 說 invalidate 比較好的前提是「實際上沒那麼多共用」；共用很頻繁時不一定成立。（這只算訊息個數；每個訊息的大小、耗電沒有模擬。）

### 12.2 本機的 false sharing

`fs.c`：兩個 thread 綁在不同的 CPU 上，各自加**自己的**計數器 5000 萬次，改變兩個計數器之間的距離（ns／次，越小越好）：

| CPU 組合 | 1 個 thread `lock add` | 相距 8／32／56 B | 相距 64／128／256 B | 加**同一個**計數器 | 只讀同一個 line |
|---|---|---|---|---|---|
| cpu2 ＋ cpu4（兩個 P-core） | 3.86 | **18.2** | **3.87** | 18.2 | 0.33 |
| cpu12 ＋ cpu16（不同 cluster 的 E-core） | 5.30 | **19.5** | **5.30** | 19.4 | 0.51 |
| cpu12 ＋ cpu13（同一個 cluster 的 E-core） | 5.31 | **22.2** | **5.30** | 22.2 | 0.51 |
| cpu2 ＋ cpu12（P-core ＋ E-core） | 3.91 | **20.3** | 5.30 | 20.3 | 0.51 |
| cpu2 ＋ cpu3（同一個 P-core 的兩個 SMT thread） | 3.88 | 9.9 | 9.1 | 9.9 | 0.65 |

- **界線剛好在 64 B**：相距 56 B 還是慢 4.7 倍，相距 64 B 就恢復正常 → 本機的 coherence 單位（block）是 **64 B**，跟 Linux 的 `SMP_CACHE_BYTES` 一樣。
- **false sharing 跟真的共用一樣慢**（18.2 vs 18.2）：協定只看 block，分不出兩個 core 寫的是不是同一個變數。
- **只讀不會變慢**：兩個 core 都留在 S（multiple readers），不用搶。
- 同一個 P-core 的兩個 SMT thread：不管距離都約 9–10 ns。它們共用同一個 L1，line 不用在 cache 之間搬；變慢是兩個 `lock add` 在同一個 core 裡互相等。
- ⚠️ 改成**一般的 store**（`*p = *p + 1`，不用 `lock`）時，兩個不同的 P-core 在同一個 line 上只從 0.25 變成 0.42 ns，幾乎沒變慢；同一個 P-core 的兩個 SMT thread 反而從 0.47 變成 3.6 ns。本機不能用 perf 看原因，**待補**。

---

## 13. 一頁速查表

| 主題 | 重點 |
|---|---|
| 兩個公理 | WS（同位置的寫有全域順序）＋ WP（寫不會遺失） |
| Snoopy bus | 銅線、廣播、大家偷聽；bus master 一次只讓一個寫、不讓人餓死；影片說最多 4–8 個 cache，課本說 2–4 個 |
| 符號 | 事件｜動作；RdX、WrX、WrX.u、Broadcast、Send、Wb、Update |
| Write-update | 每個寫先廣播再寫；S、M 收到 Broadcast 就更新；M evict 要 Wb；耗電 |
| Write-invalidate（MSI） | single writer 或 multiple readers；S 唯讀；S → M 送 WrX.u；M 讀寫不用訊息；M 收到 RdX：Send＋Wb → S |
| Seamless eviction | S（和 E）直接丟；M（和 O）要 Wb |
| MESI | 從下一層讀進來 → E；E → M 不用訊息；E 收到 RdX → S |
| MOESI | E、M 收到 RdX → O；O 負責送資料（不用 arbitration）；M → O 不用 Wb；O evict 要 Wb |
| St、Se、Probe | 沒有 owner 時：St 逾時 → Probe → Se；有 S 回應 → S；再逾時 → 從下一層讀 → E；MESIF 有 ownership transfer |
| Directory | NoC 上的集中節點；每個 block：state ＋ sharer bit vector；決定寫的順序；用 FIFO 保證 WP |
| Directory 狀態 | U、S、E（E 包含 M，因為 E → M 是靜悄悄的） |
| 原始碼 | `____cacheline_aligned_in_smp`、`CACHELINE_PADDING`：把常寫的欄位分到不同的 64 B line |
| 實測 | 模擬：MESI 省 WrX.u、MOESI 省 arbitration 和 write back、update 在頻繁共用時反而省；本機 false sharing：`lock add` 慢 4.7 倍，界線剛好在 64 B |

---

## 14. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| WS、WP 公理、PLSC | [第 34 支](lecture34_note.md) |
| ws、fr 邊為什麼是 global | [第 35 支](lecture35_note.md) |
| Write miss 要先讀整個 block（RFO） | [第 21 支](lecture21_note.md) |
| NoC | [第 27 支](lecture27_note.md)–[第 30 支](lecture30_note.md) |
| Core 之間搬一個 line 要多久 | [第 27 支](lecture27_note.md) |
| Directory 的優化、sharer 清單的省法、atomic 指令、lock | [第 37 支](lecture37_note.md) |
| Memory model、data race | [第 38 支](lecture38_note.md) |

---

## 附：本講一句話總結

> **coherence 協定要讓 distributed cache 看起來像 shared cache，也就是守住 write serialization 和 write propagation：在 snoopy bus 上，write-update 協定每個寫都先廣播再寫（拿到 bus 的順序就是寫的順序），但很耗電；write-invalidate 的 MSI 改成「一個寫的人或很多讀的人」，S → M 送一個 WrX.u 讓別人 invalidate，之後在 M 裡讀寫都不用送訊息；MESI 加了 E，讓「先讀後寫」不用送訊息；MOESI 加了 owner，讓一個 cache 負責送資料（不用 arbitration）、M → O 不用 write back，owner 不見時靠 St、Se 和 Probe 補救；系統變大時 bus 撐不住，改用 NoC 上的 directory，為每個 block 記 state 和 sharer 的 bit vector，由它決定寫的順序。本機實測：模擬確認了 MESI、MOESI 各省掉什麼，也發現頻繁共用時 write-update 的訊息反而最少；兩個 core 用 `lock add` 各加自己的計數器，只要兩個計數器在同一個 64 B 的 line 裡就慢 4.7 倍，跟真的共用一個變數一樣慢，所以 Linux 用 `____cacheline_aligned_in_smp` 和 `CACHELINE_PADDING` 把常寫的欄位分開。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 9: Multicore Systems (Part V) -- Coherence protocols, Snoopy and Directory based schemes 的 YouTube 字幕（1:05:28）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch9 §9.4.1–9.4.5（p.459–475）、Linux v6.2.12 原始碼整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
