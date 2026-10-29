# Advanced Computer Architecture 第三十支影片完整筆記：On-chip network（四）：switch 的設計、arbiter、allocator 與 router pipeline 的最佳化（Chapter 8, Part IV/V）

> **課程**：Advanced Computer Architecture — Chapter 8: The On-chip Network (Part IV/V) -- Arbiters and Allocators, Pipeline Optimizations（播放清單第 30 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（1:03:40）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch8** §8.4.5–8.4.7（p.384–400）
> **章號**：影片 Chapter 8 = 課本 Ch8。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Python 3.10）實際執行的模擬結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕把「arbiter」寫成「Orbiter／Arbiter」、「rig」= R_ij、「Taurus」= torus、「plate／fleet／click」= flit，筆記直接改正。開頭的「foreign」是字幕的雜訊。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 10 × 5 的 switch 怎麼做](#1-10--5-的-switch-怎麼做)
- [2. 面積和延遲的經驗法則](#2-面積和延遲的經驗法則)
- [3. Dimension-sliced switch](#3-dimension-sliced-switch)
- [4. Arbiter 與 allocator](#4-arbiter-與-allocator)
- [5. Matching：maximal 與 maximum](#5-matchingmaximal-與-maximum)
- [6. Round robin arbiter](#6-round-robin-arbiter)
- [7. Matrix arbiter](#7-matrix-arbiter)
- [8. Separable allocator](#8-separable-allocator)
- [9. Wavefront allocator](#9-wavefront-allocator)
- [10. 5 個 cycle 太慢](#10-5-個-cycle-太慢)
- [11. Pipeline 圖](#11-pipeline-圖)
- [12. Look-ahead routing](#12-look-ahead-routing)
- [13. Bypassing](#13-bypassing)
- [14. Speculative VC allocation](#14-speculative-vc-allocation)
- [15. Late VC selection](#15-late-vc-selection)
- [16. 【實測】](#16-實測)
- [17. 一頁速查表](#17-一頁速查表)
- [18. 與後續講次的連結](#18-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
switch allocation 的 10 × 5 switch：一層大的、或兩層小的；面積 ≈ M·N、延遲 ≈ M + N
dimension-sliced switch：跟 XY routing 一起設計 → 最省
arbiter：n 個請求選 1 個（round robin、matrix）
allocator：n 個請求對 m 個資源做 matching（separable：可能連 maximal 都不是；wavefront：一定 maximal）
router 5 級太慢（10 hop × 5 = 50 cycle）：
  look-ahead routing（RC 移出）→ 4；speculative VC allocation → 3；bypassing（佇列空）→ 2；late VC selection
```

---

## 1. 10 × 5 的 switch 怎麼做

[00:00] 回顧 router 的 5 級：buffer write（每個 flit）→ route computation、VC allocation（只對 head flit）→ **switch allocation**（預留通往輸出 port 的路，公平很重要）→ **switch traversal**（接好了就過去），然後釋放資源。

[01:34] 5 條實體 channel × 2 個 VC = **10 個輸入**；5 個輸出（N、S、E、W、local）→ **10 × 5** 的 switch。做法：

| 做法 | 結構 | 問題 |
|---|---|---|
| 一個大的 10 × 5 | | |
| 「大 switch」 | 2 個 5 × 2 ＋ 一個 4 × 5 | 中間只有 4 條線：內部有**瓶頸**，5 個輸出本來可以同時用 |
| 「小 switch」 | 5 個 2 × 1 ＋ 一個 5 × 5 | 每對 VC 先選一個：兩個請求本來可以走不同的路，被排除了 |

「大 switch、小 switch、怎麼接，永遠有取捨。」

---

## 2. 面積和延遲的經驗法則

[03:19] 不是精確的指標，是**經驗法則**（rule of thumb），用來粗略估計。M × N 的 switch：

| | 公式 | 理由 |
|---|---|---|
| **面積** | M × N | |
| **延遲** | M ＋ N | 訊號要穿過 M 個點、再穿過 N 個點 |

[04:32]

| 設計 | 面積 | 延遲 |
|---|---|---|
| 一個 10 × 5 | **50** | **15** |
| 5 個 2 × 1 ＋ 一個 5 × 5 | **35** | **13** |
| 2 個 5 × 2 ＋ 一個 4 × 5 | 40 | **16** |

[05:10] 「小 switch」的設計又快、面積又小；「大 switch」的面積在中間，但延遲最高，不是好設計。兩層的最佳化空間已經很複雜，三層更複雜。

【課本補充】課本 p.386 Table 8.3 是同一張表。

（本機算了這幾個數字，見 [§16.1](#161-switch-的成本)。）

---

## 3. Dimension-sliced switch

[05:42] 給 **XY routing** 用的特殊 switch，分成兩個：

| switch | 輸入 | 輸出 |
|---|---|---|
| **下面** | E、W、local | E、W，**加一條**送到上面的 switch（要轉到 y 方向的） |
| **上面** | N、S，加上從下面來的那一條 | N、S、local |

- [06:46] XY routing：從 E、W 來的可以轉到 y 方向（所以下面有一條往上的線）；但大網路裡大部分時間只沿 x 或只沿 y 走，**只轉一次**，所以一條就夠。
- [07:18] **從 N、S 來的不能轉到 x 方向**（XY routing 禁止 y → x），所以上面的 switch 沒有往下的線。

[07:55] 成本（老師的數字）：

| | 延遲 | 面積 |
|---|---|---|
| 不轉彎（大部分情況） | **8 或 9** | 15 ＋ 18 = **33** |
| 轉彎（最壞） | 17 | |

[08:29] 33 比之前的 35、40、50 都小，大部分情況的延遲 8、9 也比 13–16 小很多。關鍵是**跟 routing 協定一起設計**、互相最佳化。

【課本補充】課本 p.387：dimension-sliced switch 的面積 33、延遲 17，「是目前看過最省面積的」。

---

## 4. Arbiter 與 allocator

[09:36] 接著看 switch allocation、VC allocation 的過程。

| | 做什麼 |
|---|---|
| **Arbiter** | 從 **n 個請求**中選 **1 個**給資源 |
| **Allocator** | n 個請求、m 個資源：選出一個子集合，做**一對一的對應** |

[10:38] 天真的 allocator 可以用 arbiter 做：第一個資源選一個請求，把它移出競爭，再看第二個資源……但有更有效率的專用結構。

---

## 5. Matching：maximal 與 maximum

[11:17] 理論上：一個**二分圖**，一邊是請求 R、一邊是資源 S。條件：

1. 一個請求最多對到一個資源。
2. 一個資源最多對到一個請求。
3. 只有請求 Ri **有興趣**的資源 Sj 才能對（f(Ri, Sj) 為真）。

[13:34] 例子：R2 對 S1、S2 有興趣；R1 只對 S1 有興趣。

| 對法 | 結果 | |
|---|---|---|
| R2 → S1 | 只有 1 對；R1 只要 S1，沒辦法再對 | **maximal**（不能再多加一對），但不是 maximum |
| R1 → S1、R2 → S2 | **2 對** | **maximum**（最多的） |

[14:51] maximum 一定是 maximal；maximal 不一定是 maximum。目標是對到越多越好，即時在硬體裡算 maximum matching 很難；但這裡的資源不多，所以有好的硬體演算法。

---

## 6. Round robin arbiter

[15:28] 3 個請求、1 個資源；每個請求有一條 **request line** 和一條 **grant line**。電路：每個請求一個 D flip-flop 存 **priority**，加上 AND、OR、NOR gate：

- [16:40] 沒有任何 grant 時，NOR gate 輸出 1，每個 flip-flop 維持原來的 priority。
- [17:50] priority = 1 的那個請求，request 一變成 1，**grant 就變 1**（拿到資源一個 cycle）。
- [18:27] 下一個 cycle：它的 grant、priority 都變 0，**priority 傳給下一個**。
- [20:49] 像**一個 token** 從 grant 1 → grant 2 → grant 3 → grant 1 傳下去。輪到的人沒興趣就等它有興趣；用了一個 cycle 就交給下一個。

[22:31] 優缺點：

- ＋ 簡單，整個電路畫得出來。
- − 沒辦法在請求之間定**優先順序**（嚴格照順序）。
- − 沒興趣的也要拿到 token、再交出去。

---

## 7. Matrix arbiter

[23:03] 概念上跟 round robin 差不多，但有**優先順序**。n 個請求者 → 一個 n × n 的 boolean 矩陣：

```
A[i][j] = 1：i 比 j 優先（對角線不管）
永遠保持 A[i][j] = NOT A[j][i]（轉置性質）
```

[25:23] 每個 cycle：**一整列都是 1**（除了對角線）的那個請求拿到資源：它比所有人都優先。

[26:35] 用完之後：把自己的**列設成 0**、**行設成 1** → 變成最低優先：「大家都比我優先，你們先」。

[27:45] 例子（4 個請求）：

```
初始：      第 1 個拿到 → 第 3 個 → 第 4 個 → 第 2 個 → 回到初始的狀態
x 1 1 0
0 x 0 0
0 1 x 1
1 1 0 x
```

[29:30] 跟 round robin 比：

- 也是一種受限的 round robin，但**有狀態**，所以能保證一定的優先順序。
- 初始狀態可以設定不同的順序。
- 不一定要把自己清到最低：可以留幾個 1，降到**中間**的優先（但可能造成 starvation）。
- 可以在中途修改矩陣。

（本機比了固定優先、round robin、matrix 的公平性，見 [§16.2](#162-arbiter-的公平性)。）

---

## 8. Separable allocator

[30:07] 3 個 agent、3 個資源 → 9 條 request line **R_ij**（agent i 想要資源 j）、9 條 grant line。兩層的 arbiter（3 × 3 的方塊）。

[31:33] 問題：第一層可能讓**所有請求都選同一個資源**（每個 agent 只能對到一個資源，所以每個 agent 的 arbiter 只送出一個請求）。

[32:38] 例子：agent 1 想要資源 1、2；agent 2 只想要資源 1。

```
第一層：agent 1 選了資源 1、agent 2 也選資源 1
第二層：資源 1 選了 agent 2
→ agent 1 什麼都沒拿到；資源 2 沒人用；但明明可以 agent 1 → 資源 2
```

[33:41] 這不只不是 **maximum**，連 **maximal** 都不是（明明還能多對一對）。我們至少希望 allocator 是 maximal。

（本機模擬了，見 [§16.3](#163-allocator)。）

---

## 9. Wavefront allocator

[34:58] 「一個很漂亮的想法」。n 個 agent × n 個資源 → n × n 的格子；格子 (i, j) 上色表示 agent i 對資源 j 有興趣。每格是一個小處理單元，接成 **torus**。

[36:49] 兩種 token：

| Token | 移動 |
|---|---|
| **row token**（粉紅） | 只沿著**列**走，每一輪往左一格 |
| **column token**（藍） | 只沿著**行**走，每一輪往下一格 |

[37:22] 一開始，每個**對角線**的格子各拿一個 row token 和一個 column token。每一輪兩種 token 各移一格，兩種 token 在同一個格子相遇的位置，就像一道**波前**沿著對角線方向傳過去（到了邊就繞回另一邊，因為是 torus）。

[40:05] 【口頭】token 其實各自獨立移動，但因為同步，看起來像水波：水分子各自動，看起來卻像一道波傳過去。

[41:12] 規則：一個**有興趣**的格子，同時拿到 row token **和** column token，就把兩個都**吸收**（移出循環）→ 建立一個對應：agent i → 資源 j。

[41:44] 例：4 × 4，(2,2) 和 (3,1) 有興趣：第 1 輪 (2,2) 拿到兩個 token；第 3 輪 (3,1) 拿到兩個 token。4 輪後結束，對了 (2,2) 和 (3,1)。

[42:58] 性質：

- **正確**：token 被吸收後，同一列（這個 agent）不可能再拿到 row token，同一行（這個資源）不可能再拿到 column token → 一個 agent 最多一個資源，一個資源最多一個 agent。
- [44:01] **一定是 maximal**（證明在課本）：波前會經過**每一個格子**；只要同一列、同一行都還沒配到，那一格一定會同時拿到兩個 token。所以不會漏掉一個本來可以做的對應。

---

## 10. 5 個 cycle 太慢

[46:04] 一個 router 要 5 個 cycle，**10 hop 就要 50 cycle**、20 hop 就要 100 cycle。主記憶體才 200–300 cycle，光在 NoC 上走就要 100 cycle「太多了」→ 要減少每個 router 的 cycle 數。

---

## 11. Pipeline 圖

[47:22] 一張特別的 pipeline 圖：列是 stage（BW、RC、VA、SA、ST），欄是 cycle，格子裡是 flit F1（head）、F2、F3：

```
cycle：    1    2    3    4    5    6    7    8    9
BW       F1   F2             (F2)      F3
RC            F1
VA                 F1   F1                       ← cycle 3 找不到空的 VC，停一個 cycle
SA                           F1   F2   F3   F3   ← cycle 8 搶不到 switch，停一個 cycle
ST                                F1   F2        F3
```

- F2 在 F1 做 RC、VA 時待在 buffer（body flit 不做 RC、VA）。
- F1 離開後 F2 才能做 SA。
- [51:22] 同一個 packet 的 flit **嚴格依序**；head flit 多兩級（RC、VA）。body、tail flit 的負擔是 **3 個 cycle**，head flit 是 **5 個**：**head flit 決定關鍵路徑**。

---

## 12. Look-ahead routing

[51:56] 最常見的最佳化：**算下一站的路**。

- 路徑 1 → 2 → 3 → 4 → 5。node 1 不只知道要送到 2，還**幫 node 2 算好** node 2 要往哪裡送（3）。
- node 2 收到 head flit 時已經知道要送到 3 → **RC 不在關鍵路徑上**了；node 2 再幫 node 3 算……
- routing 的決定跟著 packet 一起送。
- [53:35] 缺點：node 1 沒有 node 2 的全部資訊（例如擁塞），只能用比較舊、比較少的資訊（node 2 可以定期把資訊傳給 node 1）。

[54:16] 結果：RC 和 VA **平行**做 → head flit 從 **5 級變 4 級**；body flit 一樣是 BW、SA、ST。

【課本補充】課本 p.398 Lookahead Routing。

---

## 13. Bypassing

[54:53] 假設 router 的**佇列是空的**（流量不大時很常見）：

- 試著**直接通過 switch**：跳過 buffer write、跳過 switch allocation，直接拿一個空的輸入 port 過去。
- 要有**偵測衝突**的電路；有衝突就退回傳統的做法。
- VA、RC 還是要做（仍然用 look-ahead routing，不在關鍵路徑上）。

[56:00] head flit 從 5 個 cycle 變 **2 個**。但佇列全空的機率要靠模擬看。

---

## 14. Speculative VC allocation

[56:34] **猜**會找到空的 VC：

```
BW → SA（這個 cycle 同時做 look-ahead RC 和 VA）→ ST
VA 成功：把分到的 VC 蓋在 head flit 上，直接通過 switch
VA 失敗：停下來，走傳統路線：等 VC → 再做 SA → 再 ST
```

[59:13] 5 變 **3**。不像 bypassing 那麼激進（還有 buffer write），但「找得到空的 VC」比「佇列全空」合理得多，比較實際。

---

## 15. Late VC selection

[59:45] **VC allocation 跟 switch traversal 一起做**：

- 每條往外的 link 維護一個**空 VC 的 queue**。
- head flit 通過 switch 時，同時從 queue 拿一個 VC，從 switch 出來時把 VC 編號寫上去，送下 link。
- [60:19] queue 空了怎麼辦？兩個 cycle 前就知道某條 link 的 queue 空了，就不去分配 switch。但可能有 race condition（另一個 head flit 搶走了 VC）→ 有小電路偵測，**取消**這次送出，退回傳統的 5 級。
- 跟 speculative VC allocation 原理類似（「多半找得到空的 VC」），但機制很不一樣。

[62:30] 回顧整條 pipeline：RC 用 look-ahead 移出關鍵路徑；VA 用 speculative 或 late VC selection 移出；整個可以用 bypassing（直接通過）。router 全空時，body flit 甚至只要 **1 個 cycle**。機率高低很看 benchmark。

[63:05] 最後一講：cache 和 NoC 的共同設計（NUCA），以及怎麼量 NoC 的效能。

---

## 16. 【實測】

這一講是 router 內部的電路和演算法，本機的 CPU 看不到，只做模擬。程式是 `experiments/lecture30/alloc.py`。

### 16.1 Switch 的成本

```
1. 10 × 5 的 switch 三種做法（面積, 最長延遲）
   一個 10 × 5：50, 15
   5 個 2 × 1 ＋ 一個 5 × 5：35, 13
   2 個 5 × 2 ＋ 一個 4 × 5：40, 16
```

跟影片 [04:32] 和課本 Table 8.3 一致。

### 16.2 Arbiter 的公平性

4 個 agent 搶 1 個資源，每個 cycle 各有 60% 機率想要（沒拿到的下一個 cycle 一定還要），跑 10 萬個 cycle：

```
   固定優先（0 最大）  ：每個 agent 拿到 [60063, 31554, 7713, 646]，最久等了 1059 個 cycle
   round robin ：每個 agent 拿到 [24983, 24952, 24982, 25059]，最久等了 3 個 cycle
   matrix      ：每個 agent 拿到 [24992, 24977, 24988, 25019]，最久等了 3 個 cycle
```

- **固定優先**（對照組：永遠選編號最小的）：agent 3 只拿到 646 次，最久等了 **1059 個 cycle**：就是 [第 29 支](lecture29_note.md#3-deadlocklivelockstarvation) 說的 **starvation**。
- **round robin** 和 **matrix**：四個 agent 各拿到約 1/4，最久只等 **3 個 cycle**（= 前面最多 3 個人）。公平，不會餓死。
- 這個實驗裡 matrix arbiter 每次都把自己降到最低，所以行為跟 round robin 幾乎一樣；它的好處（可以設定優先順序、降到中間）這裡沒有用到。

### 16.3 Allocator

4 個 agent × 4 個資源，每一對有 50% 機率「有興趣」，隨機 2 萬組，跟**最大可能的配對數**（窮舉 4! 種排列）比：

```
3. allocator：4 × 4，每一對有 50% 機率「有興趣」，隨機 2 萬組
   平均配對數：最大可能（maximum）3.52、separable 2.62、wavefront 3.02
   比 maximum 少的比例：separable 71.8%、wavefront 48.5%
   連 maximal 都不是（明明還能多配一對）的比例：separable 45.5%、wavefront 0.0%
   影片的例子：maximum 2；separable 配了 [(1, 0)]（maximal？False）；wavefront 配了 [(0, 0)]（maximal？True）
```

- **Separable**（input-first：每個 agent 先選一個資源，每個資源再從選它的 agent 裡選一個）：平均只配到 2.62 對，**45.5% 的情況連 maximal 都不是**：還有空的資源和想要它的 agent，卻沒配上。跟 [§8](#8-separable-allocator) 一樣。
- **Wavefront**：平均 3.02 對，**一定是 maximal**（2 萬組裡 0 次例外），跟 [§9](#9-wavefront-allocator) 的性質一致。但它不一定是 **maximum**（48.5% 的情況比最大可能少）。
- **影片的例子**（agent 0 想要資源 0、1；agent 1 只想要資源 0）：separable 只配了 agent 1 → 資源 0，agent 0 什麼都沒拿到（不是 maximal）；wavefront 從對角線 (0, 0) 開始，配了 agent 0 → 資源 0，之後 agent 1 沒東西可配了：**是 maximal，但不是 maximum**（maximum 是 2 對）。
- ⚠️ 模擬裡的 wavefront 每次都從同一條對角線開始；真的硬體通常會輪流換起始的對角線，比較公平。

### 16.4 Pipeline 的最佳化

```
4. 10 個 hop，head flit 的延遲（沒有排隊）
   5 級（BW RC VA SA ST）         ：每 hop 5 cycle → 50 cycle
   look-ahead routing（RC 移出）   ：每 hop 4 cycle → 40 cycle
   speculative VC allocation   ：每 hop 3 cycle → 30 cycle
   bypassing（佇列是空的）            ：每 hop 2 cycle → 20 cycle
```

就是 [§10](#10-5-個-cycle-太慢)–[§14](#14-speculative-vc-allocation) 的算術（不含 link 本身的延遲和排隊）。

---

## 17. 一頁速查表

| 主題 | 重點 |
|---|---|
| 10 × 5 switch | 一層 50/15；5 × (2×1) ＋ 5×5：35/13；2 × (5×2) ＋ 4×5：40/16（面積 M·N、延遲 M＋N） |
| Dimension-sliced | 跟 XY routing 一起設計；面積 33、不轉彎延遲 8–9、轉彎 17 |
| Arbiter | n 選 1 |
| Allocator | n 個請求對 m 個資源做 matching |
| Maximal／maximum | maximal：不能再加；maximum：最多的；maximum ⇒ maximal |
| Round robin | token 輪流傳；簡單；沒有優先順序；沒興趣也要經手 |
| Matrix | A[i][j] = 1 表示 i 優先；整列 1 的贏；用完列清 0、行設 1；可以設優先順序 |
| Separable | 兩層 arbiter；第一層可能都選同一個資源 → 不一定 maximal |
| Wavefront | n × n 格子、torus；row token 往左、column token 往下；有興趣又同時拿到兩個 → 配對；一定 maximal |
| 5 級太慢 | 10 hop × 5 = 50 cycle；head flit 是關鍵路徑 |
| Look-ahead routing | 前一站幫下一站算路 → RC 移出 → 4 級 |
| Bypassing | 佇列空就直接過 switch → 2 級；衝突就退回 |
| Speculative VA | 猜找得到 VC，跟 SA 平行 → 3 級 |
| Late VC selection | 通過 switch 時才從空 VC 的 queue 拿；失敗就取消、退回 |
| 實測（模擬） | 固定優先最久等 1059 cycle、round robin/matrix 最久 3；separable 45.5% 不是 maximal，wavefront 0% |

---

## 18. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Router 的 5 級 pipeline、XY routing、starvation | [第 29 支](lecture29_note.md) |
| VC、credit | [第 28 支](lecture28_note.md) |
| Select unit（也是一種 arbiter） | [第 11 支](lecture11_note.md) |
| NUCA、synthetic traffic、NoC 的效能 | [第 31 支](lecture31_note.md) |
| GPU register file 的 crossbar 和 arbiter | [第 20 支](lecture20_note.md) |

---

## 附：本講一句話總結

> **Router 的 switch allocation 要一個 10 × 5 的 switch，可以做成一層或兩層，用「面積 ≈ M·N、延遲 ≈ M + N」的經驗法則比較（一層 50/15、五個 2 × 1 加一個 5 × 5 是 35/13），跟 XY routing 一起設計的 dimension-sliced switch 更省；分配資源要靠 arbiter（n 選 1，round robin 用一個傳來傳去的 token，matrix arbiter 用一個保持轉置性質的優先矩陣）和 allocator（n 個請求對 m 個資源做 matching，兩層 arbiter 組成的 separable allocator 可能連 maximal 都不是，而用 row/column token 在 torus 上傳出一道波前的 wavefront allocator 一定是 maximal）；最後，5 級的 router 讓 10 hop 就要 50 cycle，所以用 look-ahead routing（前一站幫你算路，4 級）、speculative VC allocation（猜找得到 VC，3 級）、bypassing（佇列空就直接過，2 級）和 late VC selection 把 head flit 的關鍵路徑縮短，失敗時都能退回傳統的 5 級。本機模擬：固定優先的 arbiter 讓一個 agent 最久等了 1059 個 cycle，round robin 和 matrix 最久只等 3 個；隨機請求下 separable allocator 有 45.5% 的情況不是 maximal，wavefront 一次例外都沒有，但它也不保證是 maximum。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 8: The On-chip Network (Part IV/V) -- Arbiters and Allocators, Pipeline Optimizations 的 YouTube 自動字幕（1:03:40）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch8 §8.4.5–8.4.7（p.384–400）整理。實測為本機（Python 3.10）模擬結果。*
