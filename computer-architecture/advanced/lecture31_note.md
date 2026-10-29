# Advanced Computer Architecture 第三十一支影片完整筆記：On-chip network（五）：NUCA 與 synthetic traffic（Chapter 8, Part V/V）

> **課程**：Advanced Computer Architecture — Chapter 8: The On-chip Network (Part V/V) -- Non-Uniform Caches, Synthetic Traffic（播放清單第 31 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（38:28）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch8** §8.5–8.7（p.400–411）
> **章號**：影片 Chapter 8 = 課本 Ch8。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Python 3.10、valgrind 3.18）實際執行的模擬結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕把「NUCA」寫成「Noka／Nuka／snooka／denuca」、「cache bank」寫成「cash prank／cashback／cash max」，筆記直接改正。開頭的「foreign」是字幕的雜訊。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 大的 LLC 是 non-uniform 的](#1-大的-llc-是-non-uniform-的)
- [2. S-NUCA：bank id 放在位址的哪裡](#2-s-nucabank-id-放在位址的哪裡)
- [3. D-NUCA：bank set 與 home bank](#3-d-nucabank-set-與-home-bank)
- [4. 三種搜尋策略](#4-三種搜尋策略)
- [5. Promotion](#5-promotion)
- [6. Miss 時放哪裡、evict 時去哪裡](#6-miss-時放哪裡evict-時去哪裡)
- [7. 更進階的設計](#7-更進階的設計)
- [8. 評估 NoC 的指標](#8-評估-noc-的指標)
- [9. 架構模擬](#9-架構模擬)
- [10. Synthetic traffic](#10-synthetic-traffic)
- [11. Chapter 8 總結](#11-chapter-8-總結)
- [12. 【實測】](#12-實測)
- [13. 一頁速查表](#13-一頁速查表)
- [14. 與後續講次的連結](#14-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
L2、L3 很大，bank 散在整個晶片上 → 到不同 bank 的時間差很多（router 很慢）→ NUCA
S-NUCA：用實體位址的某幾個 bit 決定 bank（bit 越低越分散，越高越集中）
D-NUCA：一行 bank 是一個 bank set；home bank = bank set 裡離 core 最近的
  搜尋：sequential、two-way、broadcast；hit 就 promote（往 home bank 搬）；evict 就 demote
評估 NoC：延遲、吞吐量、能量、面積、繞線複雜度
  架構模擬（準但慢）或 synthetic traffic（快）：random、bit complement、transpose、bit reverse、
  bit rotation、shuffle、tornado
```

---

## 1. 大的 LLC 是 non-uniform 的

[00:00] 這一講比較短：**NUCA**（non-uniform cache architecture）：NoC 怎麼跟 cache 階層一起設計；最後是效能的評估。

[00:59] L2、L3 很大：core 旁邊是**一片 cache bank**。訊息要經過 link 和 router 才到得了，而 router 的延遲很可觀（完整的 pipeline 5 個 cycle，最佳化過也要 3、4 個）。

[01:29] 遠的 bank 可能要 **20–50 cycle** 以上，看網路大小和擁塞。所以 **LLC** 的存取時間**很不一致**：資料散布在整個晶片上 → **non-uniform cache**。

[02:03] 這會變慢，但能不能**利用**它？**把常用的資料放在離 core 近的地方**，降低感受到的延遲。怎麼做是這一講的主題。

---

## 2. S-NUCA：bank id 放在位址的哪裡

[02:38] 最簡單的：**static NUCA（S-NUCA）**。LLC 有很多 bank，排成矩陣。問題：**一個 cache block 要放在哪個 bank？** 這本身就不簡單，而且要讓存取**均勻**，不要有熱點（某個 bank 很擠、旁邊的很閒）。

[04:21] 實體位址（翻譯後）多切一個 **bank id**（16 個 bank → 4 bit）：

```
做法 1：| tag | bank id | set id | byte offset |
做法 2：| tag | set id | bank id | byte offset |
```

[04:51] bank id 的位置：

| bank id 的位置 | bank 層級 | set 層級 |
|---|---|---|
| **越往左**（越高位） | **越集中**：少數幾個 bank 被常用（可以在 bank 旁邊放一個小 cache 存常用的 block，雖然實際上不這樣做） | set 在 bank 裡分布得均勻 |
| **越往右**（越低位） | **越分散**、越隨機 → bank 之間很**均勻** | set 比較集中 → 容易 conflict miss |

[06:26] 各有優缺點，「說不清楚哪個比較好」。

（本機用真的 trace 算了兩種，見 [§12.1](#121-s-nucabank-id-的位置)。）

---

## 3. D-NUCA：bank set 與 home bank

[06:59] **Dynamic NUCA（D-NUCA）**：把常用的資料搬到**發出請求的 core 附近**。

[07:38] **Bank set**：一群 bank（例如**一行**是一個 bank set；不要跟 bank 裡的 set 搞混）。

- 4 個 bank set → 從實體位址取 **2 bit** 決定在哪個 bank set。
- [08:12] **Home bank**：bank set 裡**離發出請求的 core 最近**的那個 bank（從 core 畫一條垂直線到那一行）。
- [08:47] 關鍵：block **可能在 bank set 裡的任何一個 bank**。所以要**搜尋**。

[09:19] 位址：| bank set id | set id | byte offset |。就算 bank set 層級有些集中也沒關係（一個 bank set 裡有很多 bank），比較重要的是 bank **裡面**要均勻，所以 set id 要比較隨機。

[09:54] 請求從 core 先到 **home bank**；找到 → hit；沒找到 → 照搜尋策略找其他的 bank；整個 bank set 都沒有 → 去下一層（cache miss）。

---

## 4. 三種搜尋策略

[10:26] 都先查 home bank：

| 策略 | 做法 | 優點 | 缺點 |
|---|---|---|---|
| **Sequential** | 一邊一個一個查下去（步驟 2、3、4、5），再查另一邊 | bank 的負載低（不平行） | **很慢** |
| **Broadcast** | 同時送訊息給所有的 bank，叫它們查、回報 | **最快** | 流量大增（「大量的流量 ++」）、bank 的負載和競爭大增 |
| **Two-way** | 查完 home bank，**同時往兩邊各查一個**，再兩邊各查下一個…… | 介於兩者之間；實際上效果「相當好」 | |

[12:40] two-way：速度是 sequential 的兩倍，延遲少很多，但 bank 的存取（也代表耗電）沒有增加太多。

---

## 5. Promotion

[13:18] D-NUCA 的「dynamic」：hit 之後，把 block **promote**（升遷）：搬到**比較靠近 home bank** 的 bank，或直接搬到 home bank。

[13:45] 為什麼：spatial locality → 之後存取同一個 block 的其他 byte，會在**離 core 近**的地方 hit，訊息不用走那麼遠。

[14:19] 目標 bank 有空位就放；沒有的話：**evict** 一個，或**交換**（swap）。

[14:48] 也符合 **temporal locality**：同一個 block 很可能再被存取，下次會在更近的地方找到。

---

## 6. Miss 時放哪裡、evict 時去哪裡

[14:48] **Miss**（整個 bank set 都沒有）→ 從下一層拿。放哪裡？

| 選擇 | |
|---|---|
| 放進**最近**的 bank（home bank） | |
| 放進**最遠**的 bank，讓它隨著使用慢慢被 promote | |

看存取的樣式、工作負載的性質，要靠模擬決定。

[15:53] **Eviction**：

| 選擇 | |
|---|---|
| 送到下一層 | |
| **Demote**（降級）：搬到 bank set 裡**離 home bank 更遠**的 bank | 「軟性的 eviction」：還留在 bank set 裡，只是下次存取比較慢。它在原來的 bank 本來就要被趕出去了，表示優先權不高 |

[17:03] 好處：依使用情況把資料留在 core 附近；evict 時給 block 多一次機會。

[17:36] 搜尋策略、找 bank 的方式、migration 和 eviction 的政策，都是 D-NUCA 的重要設計決定。現在有很多種 D-NUCA，各自針對不同的工作負載最佳化。

（本機用真的 trace 簡單模擬了 D-NUCA，見 [§12.2](#122-d-nuca)。）

---

## 7. 更進階的設計

[17:36]

| 資料的種類 | 放法 |
|---|---|
| **Private**（只有一個 core 的 thread 用） | 只放在那個 core 的 **tile** 裡的 bank |
| **Shared** | 像 S-NUCA 一樣照實體位址放 |
| **Read-only** | 可以**複製**到不同的 bank |

[18:44] 怎麼知道哪些資料是哪一種？compiler 幫忙（平行化程式時知道），或 **profiler**（先試跑一次，看哪些資料結構是 read-only、shared、private）。

[19:13] **程式碼**基本上是 read-only（除非是會修改自己的特殊程式）→ 有限的複製能讓**指令的 hit rate** 很好，而效能對指令的 hit rate 很敏感（[第 26 支](lecture26_note.md#8-為什麼要-prefetch)）。複製可讀寫的資料就麻煩了：要同步所有副本 → 下一章（Ch9）。

【課本補充】課本 p.405 §8.5.3 Advanced Schemes。

---

## 8. 評估 NoC 的指標

[20:13] 「網路」不只是 topology：還有 routing 協定、router 的設計、各種最佳化、flow control（on/off 還是 credit）……指標：

| 指標 | |
|---|---|
| **延遲** | 平均從 A 到 B 要多久 |
| **吞吐量** | MB/s 或 GB/s：整個網路最多能流多少資料 |
| 能量、功耗 | |
| 面積 | |
| **繞線複雜度**（routing complexity） | ⚠️ 這裡的「routing」是 EDA（electronic design automation）的意思：**擺放導線**，不是找路徑 |

---

## 9. 架構模擬

[22:19] **Architectural simulator**：

- 輸入一個程式（和它的參數），在一個**虛擬的、假設的 CPU** 上跑：core、cache bank、ALU、排程器……都可以隨意改。「就是一個軟體的 CPU」。
- 測試新設計的正確方法：在模擬器上實作設計，跑一組（甚至幾百個）程式（benchmark suite）。

[23:59] 兩種：

| 方式 | 說明 |
|---|---|
| **Trace** | 先在 **CPU emulator** 上跑程式（只給結果，不模擬 pipeline、NoC），記下所有指令和結果，把 trace 給模擬器 |
| 線上 | emulator 執行時**即時**給 trace（例如：「我剛跑了 4 個 branch，PC 是這些、結果是這些」） |

[25:42] 架構模擬：程式 → 模擬的 CPU 給出網路的資料和位址 → 模擬 NoC 的所有部分（router、link、flow control 演算法）→ 延遲、吞吐量、功耗。

| ＋ | − |
|---|---|
| 對**真實工作負載**模擬得很準 | **很慢** |

[25:08] 比較快的：**獨立的網路模擬器**，資料來自 trace 或**合成的流量**（synthetic traffic）。

【課本補充】課本 p.407 §8.6.2 Simulation Methodologies。（本機做的 trace 就是用 valgrind 的 lackey 工具，見 [第 14 支](lecture14_note.md#121-位址預測器猜得多準)。）

---

## 10. Synthetic traffic

[26:17] 真實的資訊是「某個時間、從某個 source 送到某個 destination」。synthetic traffic 產生**同樣形式**的資訊，但照某種**分布**。

[27:25] mesh 上，source 的座標 (s_x, s_y)、destination 的座標 (d_x, d_y)，每個座標 n bit：

| Pattern | 定義 | 用途／特性 |
|---|---|---|
| **Random** | 隨機選 destination | 隨機實驗總有一點價值 |
| **Bit complement** | d_x = NOT s_x，d_y = NOT s_y | 送到很遠的地方（例 0,0 → 3,3）；**測吞吐量**，特別是中間的部分 |
| **Transpose** | d_x = s_y，d_y = s_x | 流量垂直於對角線；也拿來測吞吐量 |
| **Bit reverse** | 把 s = s_x 接 s_y 整串 bit 反過來：d_i = s_(2n−1−i) | **FFT** 會出現；看起來有點像 transpose |
| **Bit rotation** | 整串**右移**一位（最低位繞到最高位） | 大約是除以 2 → 流量往**原點**集中：計算結束、把結果收集過來（reduce）時 |
| **Shuffle** | 整串**左移**一位 | 大約乘 2 → 往外**散開**：把計算分散出去時 |
| **Tornado** | d_y = s_y，d_x = (s_x ＋ ⌈k/2⌉ − 1) mod k | 每一格往右移大約一半的距離；平行解**微分方程**時會看到，像龍捲風一樣大量資料往右，到底再繞回來 |

（本機在 8 × 8 mesh 上算了每種 pattern 的平均 hop 數和最忙的 link，見 [§12.3](#123-synthetic-traffic)。）

【課本補充】課本 p.408 列了這些 pattern 的公式。

---

## 11. Chapter 8 總結

[36:25]「五講之後終於到了這一章的結尾，該慶祝一下。」

- 現代 NoC 有複雜的節點和 link 網路。
- 階層：message → packet（同一條路）→ head／body／tail flit；flit 可能要好幾個 cycle 傳（連續），flit 是不可分割的單位，再分成 phit。
- 主要問題：deadlock、livelock、starvation；有 deadlock-free 的 routing 協定，靠禁止某些轉彎，不讓順時針或逆時針的 cycle 形成。
- router 有 5 級：BW、RC、VA、SA、ST，可以用 look-ahead routing、speculative VC allocation、bypassing 等最佳化。
- 評估效能：架構模擬，或 synthetic traffic generator。

[38:06] 下一章：**multiprocessor**。

---

## 12. 【實測】

程式在 `experiments/lecture31/`。都是模擬。

### 12.1 S-NUCA：bank id 的位置

`nuca.py`：用 `work.c`（[第 14 支](lecture14_note.md)）的 valgrind trace 的 load／store，先經過一個 32 KB 的 L1（8-way、LRU），只有 L1 miss 的 17826 次送到 LLC。LLC 是 4 × 4 = 16 個 bank，core 在左上角，到一個 bank 要走「曼哈頓距離 ＋ 1」個 hop：

```
load/store 4524404 次，L1 miss（送到 LLC）17826 次
S-NUCA，bank id 用低位：16 個 bank 的存取數 最多 1147、最少 1088；平均 3.99 hop
S-NUCA，bank id 用高（第 9–12）位：16 個 bank 的存取數 最多 4993、最少 0；平均 3.32 hop
```

- bank id 用 block address 的**低位**：16 個 bank 的存取數幾乎一樣（1088–1147），非常**均勻**。
- 用比較**高**的位：最多的 bank 有 4993 次、有的 bank **0 次**：非常**集中**。跟 [§2](#2-s-nucabank-id-放在位址的哪裡) 說的一樣。
- 這個例子裡集中的那幾個 bank 剛好離 core 比較近，平均 hop 反而少（3.32 對 3.99）；但熱點的 bank 會擁塞，這裡的模型沒有算排隊。

### 12.2 D-NUCA

同一個程式。每個 bank 很小（32 set × 4-way），bank set = 一行，home bank = 那一行最上面（離 core 最近）的 bank；用 broadcast 搜尋，hit 就往 home bank 升一格，set 滿了就把最舊的往外降一格：

```
S-NUCA（bank id 用低位）：LLC hit 70.7%；hit 平均 3.98 hop
D-NUCA（broadcast 搜尋、hit 就往 home 升一格；miss 時放最遠的 bank）：LLC hit 67.1%；hit 平均 4.18 hop；在 home bank hit 的比例 22%
D-NUCA（broadcast 搜尋、hit 就往 home 升一格；miss 時放最近的 home bank）：LLC hit 29.1%；hit 平均 3.21 hop；在 home bank hit 的比例 30%
```

- ⚠️ 在這個工作負載上，**D-NUCA 沒有比 S-NUCA 好**：
  - miss 時放**最遠**的 bank：hit 的平均距離 4.18 hop，比 S-NUCA 的 3.98 還遠；只有 22% 的 hit 在 home bank。
  - miss 時放**最近**的 bank：hit 平均只要 3.21 hop，但 hit rate 掉到 29%：新的 block 一直擠進 home bank，把有用的 block 推出去。
- 原因：`work.c`（qsort）在 L1 之後的存取大多是**掃過去**的，同一個 block 在 LLC 很少被重複用到，promotion 沒機會發揮；而 D-NUCA 的一個 bank set 只有一行 bank，容量的分配方式也不同。這就是 [14:48] 說的「要看工作負載，靠模擬決定」。
- 這是很粗的模型（demote 只做一層、沒有排隊、沒有 two-way 搜尋的時間），只能看趨勢。

### 12.3 Synthetic traffic

`traffic.py`：8 × 8 mesh，XY routing，每個節點送一個 flow 到 pattern 決定的目的地。算平均 hop 數，和**最忙的一條 link 上有幾個 flow**（它決定吞吐量的上限：每個節點每個 cycle 最多送 1 ÷ 那個數字個 flit）：

```
pattern                    平均 hop最忙 link 的 flow 數（隨機的取 50 次平均）    吞吐量上限（flit/節點/cycle）
隨機（uniform random）           5.17               2.2                   0.446
bit complement               8.00               4.0                   0.250
transpose                    5.25               7.0                   0.143
bit reverse                  5.25               7.0                   0.143
tornado                      3.75               3.0                   0.333
shuffle                      4.00               4.0                   0.250
bit rotation                 4.00               4.0                   0.250
```

- **Bit complement** 的平均 hop 最多（**8.00**，每個節點都送到對角的另一邊）：跟 [28:32] 說的「送到很遠的地方」一致。
- **Transpose** 和 **bit reverse** 平均只走 5.25 hop，但**最忙的 link 有 7 個 flow**，吞吐量上限最低（0.143）：XY routing 讓大家在對角線附近擠同一些 link。所以「拿來測吞吐量」的 pattern，重點是會不會把流量擠在某些 link 上，不一定是距離。
- **Tornado** 平均 3.75 hop、最忙的 link 3 個 flow：大家同時往右，流量很整齊。
- 隨機流量的瓶頸最小（0.446）：沒有特別的結構，流量最分散。

---

## 13. 一頁速查表

| 主題 | 重點 |
|---|---|
| NUCA | 大 LLC 的 bank 散在晶片上，遠的 20–50 cycle 以上；把常用的放近一點 |
| S-NUCA | 位址的某幾 bit 決定 bank；越低位越均勻（但 set 容易集中），越高位越集中 |
| D-NUCA | bank set（一行）；home bank = 離 core 最近的；block 可能在 bank set 的任何 bank |
| 搜尋 | sequential（慢、省）、broadcast（快、流量大）、two-way（折衷，實際上好） |
| Promotion | hit 就往 home bank 搬；滿了就 evict 或 swap |
| Miss／evict | miss 放最近或最遠；evict 可以 demote（還留在 bank set） |
| 進階 | private 放自己的 tile；shared 照位址；read-only（含程式碼）可以複製 |
| 指標 | 延遲、吞吐量、能量、面積、繞線複雜度（EDA 的 routing） |
| 評估方法 | 架構模擬（trace 或線上；準但慢）；獨立網路模擬器 ＋ synthetic traffic（快） |
| Traffic | random、bit complement、transpose、bit reverse（FFT）、bit rotation（往原點）、shuffle（往外）、tornado（微分方程） |
| 實測（模擬） | bank id 低位：16 個 bank 1088–1147 次，高位：0–4993 次；這個 workload 的 D-NUCA 沒比 S-NUCA 好；8 × 8 mesh：bit complement 平均 8 hop，transpose 最忙的 link 7 個 flow |

---

## 14. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Cache 的 bank、LLC | [第 24 支](lecture24_note.md)、[第 21 支](lecture21_note.md) |
| Router 的延遲、pipeline | [第 29 支](lecture29_note.md)、[第 30 支](lecture30_note.md) |
| Topology、mesh | [第 27 支](lecture27_note.md) |
| 指令 miss 很貴 | [第 26 支](lecture26_note.md) |
| Shared data 的副本怎麼同步：coherence | [第 33 支](lecture33_note.md)–[第 37 支](lecture37_note.md)（Ch9） |
| 多處理器、thread | [第 32 支](lecture32_note.md)（Ch9） |

---

## 附：本講一句話總結

> **大的 LLC 由散布在整個晶片上的 bank 組成，到遠的 bank 要經過很多個很慢的 router，所以存取時間很不一致，這就是 NUCA：S-NUCA 用實體位址的某幾個 bit 決定 bank（越低位越均勻、越高位越集中），D-NUCA 則把一行 bank 當成一個 bank set、離 core 最近的那個當 home bank，block 可以在 bank set 的任何一個 bank 裡，用 sequential、broadcast 或折衷的 two-way 策略搜尋，hit 就往 home bank promote、evict 時 demote 到比較遠的 bank，進階的設計還把 private 資料放在自己的 tile、把 read-only 的資料（例如程式碼）複製到多個 bank；評估一個 NoC 看延遲、吞吐量、能量、面積和繞線複雜度，可以用準但慢的架構模擬，或用 random、bit complement、transpose、bit reverse、bit rotation、shuffle、tornado 這些 synthetic traffic 快速測試。本機用真實的 trace 模擬：bank id 用低位時 16 個 bank 的存取數幾乎一樣，用高位時有的 bank 一次都沒有；這個 qsort 程式的 LLC 存取幾乎沒有重複利用，所以簡化的 D-NUCA 並沒有比 S-NUCA 好；在 8 × 8 mesh 上，bit complement 平均走 8 hop，而 transpose 雖然只走 5.25 hop，卻讓一條 link 擠了 7 個 flow，吞吐量上限最低。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 8: The On-chip Network (Part V/V) -- Non-Uniform Caches, Synthetic Traffic 的 YouTube 自動字幕（38:28）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch8 §8.5–8.7（p.400–411）整理。實測為本機（Python 3.10、valgrind 3.18）模擬結果。*
