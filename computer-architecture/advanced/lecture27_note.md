# Advanced Computer Architecture 第二十七支影片完整筆記：On-chip network（一）：節點、router、buffered wire、bisection bandwidth、diameter 與各種 topology（Chapter 8, Part I/V）

> **課程**：Advanced Computer Architecture — Chapter 8: The On-chip Network (Part I/V) -- Network Topologies and Basic Concepts（播放清單第 27 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（59:49）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch8** §8.1（p.335–346）
> **章號**：影片 Chapter 8 = 課本 Ch8。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕的開頭只剩「foreign Networks」，影片一開始說的應該是「on-chip networks」這一章；「Taurus」= torus、「factory」= fat tree、「Clause／cloth」= Clos、「hybrid Cube」= hypercube、「wire」在 [15:18] 應該是 **via**（連接兩層的垂直導線），筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 這一章的內容](#1-這一章的內容)
- [2. 晶片的配置](#2-晶片的配置)
- [3. Node、router、channel](#3-noderouterchannel)
- [4. 長導線很慢 → buffered link](#4-長導線很慢--buffered-link)
- [5. 晶片上的導線是 3D 的](#5-晶片上的導線是-3d-的)
- [6. 三種導線](#6-三種導線)
- [7. Bus 不夠用](#7-bus-不夠用)
- [8. Tile](#8-tile)
- [9. Bisection bandwidth 與 diameter](#9-bisection-bandwidth-與-diameter)
- [10. Chain、ring、fat tree](#10-chainringfat-tree)
- [11. Mesh、torus、folded torus](#11-meshtorusfolded-torus)
- [12. Hypercube](#12-hypercube)
- [13. Clos network](#13-clos-network)
- [14. Butterfly](#14-butterfly)
- [15. 總表](#15-總表)
- [16. 之後要講什麼](#16-之後要講什麼)
- [17. 【實測】](#17-實測)
- [18. 一頁速查表](#18-一頁速查表)
- [19. 與後續講次的連結](#19-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
晶片上有 core、LLC 的 bank、memory controller、加速器 → 要一個 network on chip（NoC）
每個要溝通的 node 有一個 router；router 之間是 channel（導線 ＋ 入口的 buffer）
長導線延遲 ∝ 長度² → 切成 s 段、中間加 buffer → 延遲 ∝ 長度
bus：一次只有一個送 → 不夠；改成 tile（一個 tile 一個 router）用 topology 接起來
評估 topology：bisection bandwidth（越大越好：路徑多樣、不怕塞車）、diameter（越小越好）
chain、ring、fat tree、mesh、torus、folded torus、hypercube、Clos、butterfly
```

---

## 1. 這一章的內容

[00:00] 背景：一點 pipelining（in-order，Ch2）、cache（Ch7）、一點**圖論**。

[00:49] 六節：

1. NoC 概論（本講）。
2. 訊息傳送的基本觀念（[第 28 支](lecture28_note.md)）。
3. **Routing**：怎麼把訊息從晶片的一處送到另一處（[第 29 支](lecture29_note.md)）。
4. **Router** 的設計（[第 29](lecture29_note.md)、[30 支](lecture30_note.md)）。
5. **NUCA**（non-uniform cache architecture）：一種新的 cache 架構（[第 31 支](lecture31_note.md)）。
6. NoC 的效能（[第 31 支](lecture31_note.md)）。

**NoC** = network on chip（大寫 N、小寫 o、大寫 C）。

---

## 2. 晶片的配置

[01:24] 兩種常見的配置：

| 配置 | 樣子 |
|---|---|
| **棋盤式** | core 和 cache bank 交錯排；兩邊有 memory controller |
| **Rim（邊框）式** | core 和 memory controller 在**外圈**，cache bank 在**中間** |

- [02:01] cache bank：[第 24 支](lecture24_note.md) 講過大 cache 會切成很多 bank。最後一層 cache（L2、L3、L4 都可能）叫 **LLC**（last level cache），它的 bank 分散在晶片各處。
- 其他配置也可以，效能各有取捨；但不管怎麼配，**都要一個晶片上的網路**把它們接起來。

---

## 3. Node、router、channel

[03:11] 網路的兩個詞：**node**（節點）和 **link**（連線，畫成圖時也叫 edge）。【口頭】「這是電腦科學的 graph（資料結構），不是工程上的圖表。沒聽過的話，先停下來去讀一下。」

[03:47] **Node**：任何能送、收訊息的東西：core、cache bank、memory controller，或**加速器**（專門的電路，例如 AI 引擎、圖形引擎）。

[04:19] **Router**：每個要溝通的 node 都有一個，是它的**通訊子系統**，幫它處理通訊。router 接到別的 router。

[04:53] router 之間是 **channel**：

| 部分 | 說明 |
|---|---|
| 實體的部分 | 一組平行的銅線 |
| **Data buffer** | 在入口（**ingress**；出口是 **egress**）的小儲存區，放「在飛」的訊息 |

[06:04] 訊息進來 → 放在 data buffer → router 處理：決定要送給哪一個相鄰的 router。

---

## 4. 長導線很慢 → buffered link

[06:42] 兩個 router 之間的一組平行銅線，中間沒有 buffer。[第 24 支](lecture24_note.md#16-導線的延遲跟長度的平方成正比) 的 Elmore delay model 說：**延遲 ∝ 長度²**。長度加倍 → 延遲 4 倍；長度 3 倍 → 延遲 9 倍。

[07:49] 解法：把長導線切成 **s 段**，段與段之間插一個 **buffer**（像接力：拿到一點資料就馬上往下送，像 pipeline；不需要很多儲存空間）。

[08:21] 算延遲（buffer 的延遲 d，比例常數 κ）：

```
buffer：s − 1 個，共 (s − 1)·d
每段：長度 L/s，延遲 κ(L/s)²，共 s 段 → κL²/s
總延遲 = (s − 1)d ＋ κL²/s
```

[09:00] 對 s 微分設成 0（是最小值，老師省略證明）：

```
最佳段數 s* = √(κ/d) · L      ← 跟長度成正比
最佳延遲   = 2√(κd) · L − d   ← 大約跟長度成正比
```

[10:47] 重點：把**平方**的關係變成**線性**的。這就是 **buffered link**。

（本機把公式算了一遍，見 [§17.2](#172-buffered-wire)。）

---

## 5. 晶片上的導線是 3D 的

[11:23] VLSI 晶片像**多層樓的建築**：

- **最底層**全是電晶體（在矽上看起來像一個「＋」：一層 poly 疊在摻雜的矽上）。底層已經沒有空間拉線了。
- 上面加**只有金屬連線**的層：要連兩個電晶體，就往上走、橫著走、再往下走，像在大樓裡爬樓梯。
- [13:07] 導線是在 **3D 空間**裡走的。【口頭】「像高速公路的高架橋和匝道。」

[13:41] 為什麼要很多層金屬層？**同一層的導線不能交叉**（交叉就變成同一個電位）。所以：

```
一層全部沿 x 方向 → 下一層全部沿 y 方向 → 再下一層 x 方向 → …
```

[15:18] 連接兩層的垂直導線叫 **via**（⚠️ 字幕寫成「wire」；老師說它「也叫 TSV」，但 TSV 一般指穿過整片矽的 through-silicon via，跟晶片內層與層之間的 via 不太一樣，這裡照字面記下，待補）。

[15:52] 一顆現代晶片可能有 20 億個電晶體，通常有 **1 層電晶體 ＋ 10–15 層金屬層**，還有幾層特別的：

| 特別的層 | 用途 |
|---|---|
| **電源層** | 很粗的導線往下供電，排成格子（Ch12 可靠度會講） |
| **接地層** | 電流的回程 |
| **時脈層** | 分送 clock |

---

## 6. 三種導線

[17:31]

| 種類 | 連什麼 | 有沒有 buffer |
|---|---|---|
| **Local** | 同一個功能單元裡的電晶體（例如 adder 裡面） | 很短，不用 |
| **Intermediate** | 不同但靠近的功能單元 | 通常有 |
| **Global** | 晶片的一端到另一端 | 通常有 |

---

## 7. Bus 不夠用

[18:05] 最簡單（但天真）的做法：所有 router 共用**同一組銅線**：**bus**。

[18:45] 缺點：

- **一次只能有一個發送者**（同時送會互相干擾）。一送多收可以，但多核心系統裡這種流量不常見。
- 要一個 **bus master** 收所有發送者的請求、給其中一個許可 → 又是一個競爭的點。
- 循序的瓶頸，**不能擴充**到很多核心。

---

## 8. Tile

[19:50] 多核心處理器的配置（棋盤式）：core、cache bank 交錯，兩邊有 memory controller，再加上 router。

- [20:27] 一個 **tile** 包含：2 個 core、2 個 cache bank、1 個 memory controller（五個東西），**一個 tile 一個 router**。
- 為什麼不是每個 core 一個 router？要減少 router 的數量，而且流量也沒有大到需要那麼多 router：把一群 node 合併成一個 node。

[21:00] 三個重點：**tile**、**每個 tile 一個 router**、**router 用某種 topology 接起來**。【口頭】「想的話可以倒回 30 秒再聽一次。」

[21:35] router 的工作：替自己的 tile **送、收**訊息，**也要轉送**其他 tile 發出的訊息（接力）。

[22:09] 之後：router 叫 **node**，相鄰的 node 用 **link** 接起來，router 和 link 組成 on-chip network。

---

## 9. Bisection bandwidth 與 diameter

[22:09] 兩個最重要的指標：

| 指標 | 定義 | 希望 |
|---|---|---|
| **Bisection bandwidth** | 要**剪斷**幾條 link，才能把網路分成（大約）相等的兩半；有很多種剪法時取**最少**的 | **越大越好** |
| **Diameter** | 所有節點對的**最短路徑**中，**最長**的那一條 | **越小越好** |

[23:57] bisection bandwidth 大 → 兩個節點之間有**很多條路徑可選**（path diversity）→ 比較不怕**塞車**。

- 例：一條 chain 的 bisection bandwidth 是 1，任何一點塞住都沒有替代路線。
- 【口頭】城市道路也一樣：平行的路越多，越不塞；只有一條路就會大塞車。

[25:37] diameter 代表網路的「大小」：任兩點之間最多要走幾條 link。**走 link 的次數是網路延遲的主要決定因素**，所以希望 diameter 小（router 之間多接一些線）。

---

## 10. Chain、ring、fat tree

[27:15]

| Topology | 說明 | 用在 NoC？ |
|---|---|---|
| **Chain** | 像 linked list 一個接一個；bisection bandwidth 1、diameter 很大（頭到尾）；任兩點只有一條路 | 幾乎不用 |
| **Ring** | 頭尾接起來；兩點之間有兩條路；bisection bandwidth 2、diameter 大約是一半 | 偶爾會看到 |

[28:52] **Fat tree**：

- 節點（router）是**葉子**，上面有一堆內部節點。從一片葉子到另一片：往上走到共同的祖先，再往下。
- diameter 約 **2 log n**（上去 log n，下來 log n）。
- [29:33] 看起來 bisection bandwidth 只有 1（在根那裡一刀切開），但這就是它叫 **fat** tree 的原因：**越往根，link 越粗**（越多條平行的線，例如有 4 個節點在下面就 4 條）→ bisection bandwidth 其實不小。
- [31:07] fat tree 適合**叢集電腦**，不適合 on-chip network。

---

## 11. Mesh、torus、folded torus

[31:07] **Mesh**：每個節點 degree 4（邊上 3、角落 2）。規則、容易在晶片上做（功能單元之間的連線本來就很像 mesh）→ **很常見**。

[32:16] **Torus**：同一列、同一行的頭尾也接起來 → 沒有「邊、角」的分別，每個節點 degree 4。

[32:50] 問題：那些**長的 link**：從一列的頭接到尾，實體長度是一般 link 的 n 倍（N × N torus）。一般 link 走 1 個 cycle，長 link 要更多 cycle → 網路不再均質。

[33:23] 解法：**folded torus**（老師留作業：自己確認它還是一個 torus）：

```
一般 torus 的一列：0 - 1 - 2 - 3 - 4 - 5 - 6 - 7（還有一條 7 → 0 的長線）
folded torus：     排成「隔一個接一個」，每條 link 的長度都一樣（大約是一般 mesh 的 2 倍）
```

- [34:25] 還是有列、有行的概念；一列裡的節點以「交錯」的方式接起來，行也一樣，角落之間的「交叉連線」也在。
- [35:28] 代價：相鄰節點之間的導線是 mesh 的**兩倍長**；好處：**沒有長 link**。
- [36:00] folded torus 比 torus 常見得多。**Folding** 是一般的網路技巧：把理論上的網路推一推、拉一拉、折起來、重新接，變成能在（大多是 2D、或只有幾層的 3D）晶片上實作的樣子。

（本機算了兩種的線長，見 [§17.1](#171-topology-的-diameter-和-bisection-bandwidth)。）

【課本補充】課本 p.340：N × N 的 mesh 的 diameter 是 **2N − 2**（對角）；torus 變成 **N/2 + N/2 = N**（中心到角落），大約減半。

---

## 12. Hypercube

[36:34] **High radix** topology：每個節點接**超過 4 個**鄰居 → 更多路徑、diameter 更小；但在 NoC 裡很難做，在叢集電腦裡比較容易。現在 NoC 裡也開始出現。

[37:42] **Hypercube** 是 cube 的一般化，**遞迴**地建：

```
H0：一個點
H1：兩個 H0 接起來，編號 0、1
H2：兩個 H1，第一份前面加 0（00、01），第二份加 1（10、11），對應的點接起來
H3：兩個 H2（上、下兩面），對應的點接起來 = 一般的立方體
H4：兩個 H3 …
```

[38:53] 相鄰節點的編號**只差一個 bit**（**Hamming distance** = 1）：同一份裡本來就這樣；新加的邊只連「低位一樣、最高位一個 0 一個 1」的點。

[41:22] 很適合網路規模的叢集電腦，不適合一般的 on-chip network。性質（n 個節點）：

| | 值 | 為什麼 |
|---|---|---|
| diameter | **log₂ n** | 每個節點用 log₂ n 個 bit 編號；每走一步改一個 bit，最多改 log₂ n 個 |
| bisection bandwidth | **n/2** | 兩半各 n/2 個節點，每個節點都有一條 link 連到另一半 |

---

## 13. Clos network

[42:37] **Clos network**：一整個家族，三層，參數 **r、m、n**：

| 層 | 交換器（switch） | 每個 |
|---|---|---|
| 第 1 層 | **r** 個 | n 個輸入 × m 個輸出 |
| 第 2 層 | **m** 個 | r × r |
| 第 3 層 | **r** 個 | m × n（第 1 層反過來） |

- 總共 **nr 個輸入、nr 個輸出**。
- [43:48] **switch 是內部節點**：不產生、也不消耗訊息，只負責把訊息從一邊送到另一邊。
- [47:00] 加了很多額外的 switch，而且看起來在一個集中的地方，不一定適合 NoC；但有「**flattened** Clos」，比較分散。

[47:33] 數學性質：

| 條件 | 性質 |
|---|---|
| **m ≥ n** | 一個沒用到的輸入要接到一個沒用到的輸出：**一定接得起來**，但可能要**重新安排**其他已經在用的連線（rearrangeable） |
| **m ≥ 2n − 1** | 一定接得起來，**不用動**其他連線 |

【課本補充】課本 p.345：Clos 裡不同種類的 switch 有不同的輸入輸出數，不好製造，所以偏好均質的。有名的例子是 **Benes network**（m = n = 2），只用 2 × 2 的 switch，等於用 high radix 的網路做出 low radix 的網路。

---

## 14. Butterfly

[49:06] 照這裡畫的方式，**不是** high radix：每個 switch 都是 **2 × 2**。

- 例：8 個主要的 router（產生和消耗資料），中間 **3 欄**內部的 switch。
- [50:12] 第 1 欄：決定目的地在前 4 個還是後 4 個；第 2 欄：在那 4 個的前 2 個還是後 2 個；第 3 欄：1 個還是另 1 個。
- [50:43] 例：從 6 送到 2：2 在上面 4 個 → 往上；在 1–2 那組 → 往那邊；是 2 → 送到 2。
- [51:49] 16 個節點就要多一欄：**log₂ n 欄**。可以有很多平行的流量，但每個 switch 只有 2 進 2 出。也有用 high radix router 的高效能版本。

【課本補充】課本 p.346：輸入和輸出可以是同一組節點（**folded butterfly**）。butterfly 沒有 path diversity：一對輸入輸出只有**一條**路徑；但 link 數有 N + N log₂N 條（比 torus 的 2N 多），塞車的影響比較小，而且 diameter 是 **log₂N + 1**，比 torus 好得多。

---

## 15. 總表

[52:24]（n 個節點；忽略上取整、下取整、±1）

| Topology | switch 數 | link 數 | diameter | bisection bandwidth | 用在 NoC？ |
|---|---|---|---|---|---|
| Chain | 0 | n − 1 | n − 1 | 1 | 不用 |
| Ring | 0 | n | n/2 | 2 | 用 |
| Fat tree | n − 1 | 2(n − 1) | 2 log n | n/2 | 不用 |
| Mesh | 0 | 2√n(√n − 1) | 2(√n − 1) | √n | **用** |
| Torus | 0 | 2n | √n | 2√n | 用 |
| Folded torus | 0 | 2n | √n | 2√n | **用** |
| Hypercube | 0 | (n log n)/2 | log n | n/2 | 不用 |
| Butterfly | (n log n)/2 | n ＋ n log n | log n ＋ 1 | n/2 | 某些變形（flattened）用 |

- [54:46] torus：每個節點 degree 4 → link 數 = 4n ÷ 2 = 2n；mesh 的邊和角少一些。mesh 的 diameter 是角到角，torus 是角到中心。
- [55:22] bisection bandwidth 要沿著一列、一行剪。
- fat tree 的 link 數假設每條 link 的寬度等於下面子樹的葉子數。

⚠️ 這張表是照影片口頭和投影片的說明整理的，mesh、torus 的公式是本筆記用 √n × √n 的格子代入；課本 §8.1 沒有給同樣的總表（只在 p.340、p.342、p.346 分別講 mesh／torus、hypercube、butterfly 的 diameter 和 link 數）。本機實際算了 n = 16 的情況，見 [§17.1](#171-topology-的-diameter-和-bisection-bandwidth)。

---

## 16. 之後要講什麼

[57:40] 下一講：訊息傳送的基本結果；之後 routing、router 的設計、怎麼跟 LLC 一起設計 NoC、怎麼 benchmark 一個 NoC。

[58:10] diameter 和 bisection bandwidth 是**設計時**的參數，選了 topology 就定了。真正的效能（**吞吐量**：兩點之間每秒最多送幾 byte；**延遲**：要幾個 cycle）還要看 router 的設計、routing 協定、跑的工作負載。【口頭】「我們不丟封包，所以不用傳統網路裡的 goodput 這個詞。」最後一講會用 synthetic benchmark 看效能。

---

## 17. 【實測】

程式在 `experiments/lecture27/`。

### 17.1 Topology 的 diameter 和 bisection bandwidth

`topo.py`：n = 16 個節點，**真的**算出 diameter（每一對節點用 BFS 找最短路徑，取最大）和 bisection bandwidth（試遍所有「分成兩半、各 8 個」的切法，共 6435 種，取要剪最少條 link 的）：

```
topology        link 數  diameter  bisection   公式代入 n=16、每邊 √n=4：(link, diameter, bisection)
chain               15        15          1   (15, 15, 1)
ring                16         8          2   (16, 8, 2)
mesh 4×4            24         6          4   (24, 6, 4)
torus 4×4           32         4          8   (32, 4, 8)
hypercube H4        32         4          8   (32, 4, 8)
```

- 全部跟 [§15](#15-總表) 的公式一樣。
- n = 16 時 torus 和 hypercube 的數字剛好一樣（4 × 4 的 torus 其實就跟 H4 同構：每一維 4 個節點的環，相當於兩個 bit）；n 更大時 hypercube 的 diameter（log n）會比 torus（√n）小很多。

folded torus（一列 8 個節點）：

```
一般 torus：環上 8 條 link 的實體長度 [1, 1, 1, 1, 1, 1, 1, 7]，最長 7
folded torus：環上 8 條 link 的實體長度 [2, 2, 2, 1, 2, 2, 2, 1]，最長 2
```

- 把環 0-1-2-…-7-0 排成 `0 7 1 6 2 5 3 4` 的順序（[33:23] 的「交錯」接法），連接關係完全一樣（還是同一個環，所以還是 torus），但**最長的 link 從 7 變成 2**：就是 [35:28] 說的「每條是 mesh 的兩倍長，但沒有長 link」。

### 17.2 Buffered wire

同一個程式，延遲 = (s − 1)·d ＋ κL²/s，κ = 1、d = 4，試 s = 1–199 找最小值，跟 [§4](#4-長導線很慢--buffered-link) 的公式比：

```
   L =  10：不加 buffer 延遲     100；最佳 s = 5（公式 √(κ/d)·L = 5.0），延遲   36.0（公式 2√(κd)·L − d = 36.0）
   L =  20：不加 buffer 延遲     400；最佳 s = 10（公式 √(κ/d)·L = 10.0），延遲   76.0（公式 2√(κd)·L − d = 76.0）
   L =  40：不加 buffer 延遲    1600；最佳 s = 20（公式 √(κ/d)·L = 20.0），延遲  156.0（公式 2√(κd)·L − d = 156.0）
   L =  80：不加 buffer 延遲    6400；最佳 s = 40（公式 √(κ/d)·L = 40.0），延遲  316.0（公式 2√(κd)·L − d = 316.0）
```

- 最佳段數和延遲都跟公式一樣。
- 長度加倍：不加 buffer 延遲變 **4 倍**（100 → 400 → 1600），加了 buffer 大約變 **2 倍**（36 → 76 → 156）：平方變線性。

### 17.3 本機晶片上的「網路」：核心之間換手多久

`c2c.c`：兩個 thread 分別綁在兩個核心上，輪流寫**同一條** cache line（ping-pong），量一個來回 ÷ 2 = 一條 line 在兩個核心之間換手一次的時間。這條 line 要透過晶片上的網路從一個核心的 cache 搬到另一個。所有 pair 用**隨機順序**量 5 輪，取中位數（跑了兩次）：

| 單程（ns） | cpu2 | cpu4 | cpu6 | cpu8 | cpu10 | cpu12（E） | cpu16（E） |
|---|---|---|---|---|---|---|---|
| cpu0 | 40.8／44.5 | 39.2／45.4 | 34.7／45.2 | 36.9／46.6 | 37.5／48.4 | 62.5／73.7 | 39.3／51.1 |
| cpu2 | | 39.0／31.8 | 35.6／32.2 | 36.7／33.6 | 37.4／34.0 | 39.3／35.9 | 39.1／35.9 |
| cpu4 | | | 35.2／32.6 | 35.4／33.1 | 35.8／33.6 | 37.5／35.0 | 37.4／35.8 |
| cpu8 | | | | | 33.4／35.3 | 35.1／37.1 | 35.2／37.2 |
| cpu12 | | | | | | | 45.4／48.1 |

（cpu0、2、4、6、8、10 是 6 個不同的 P-core；cpu12–15、cpu16–19 是兩組 E-core，各組共用一個 L2。）

- P-core 之間大多是 **32–40 ns**，P-core 和 E-core 之間 35–39 ns，兩組 E-core 之間 45–48 ns。
- **看不出「越遠越慢」的樣式**：如果是 ring，離得遠的核心應該要多走幾個 hop，但這裡的差別都在雜訊範圍裡。可能是每個 hop 只要很短的時間，跟整個 coherence 協定的負擔（[第 33 支](lecture33_note.md)–[第 36 支](lecture36_note.md)）比起來很小。本機晶片內部網路的 topology Intel 的公開資料這裡沒有，**待補**。
- ⚠️ cpu0 那一列比較慢、也比較不穩（例如 cpu0↔cpu12 62–74 ns）：cpu0 通常負責處理很多 interrupt，背景的工作會打斷測量。
- ⚠️ 一開始的版本**照順序**量（每一列從左到右），結果每一列的**第一個** pair 都特別快（約 34 ns，其他約 60 ns），看起來好像「相鄰的核心比較近」；改成隨機順序、多量幾輪後就消失了，是量測順序造成的假象（可能跟時脈的升降有關），不是 topology。
- Basic 課程也量過幾對核心的來回時間：[Basic 第 40 支 §13.2](../basic/lecture40_note.md#132-一條-cache-line-在核心之間搬一次要多久)。

---

## 18. 一頁速查表

| 主題 | 重點 |
|---|---|
| NoC | 接 core、LLC bank、memory controller、加速器 |
| 配置 | 棋盤式、rim 式 |
| Router | 每個 node 一個；送、收、轉送 |
| Channel | 平行銅線 ＋ ingress 的 data buffer |
| Buffered link | s* = √(κ/d)·L；延遲 2√(κd)·L − d → 從 ∝ L² 變 ∝ L |
| 3D 導線 | 1 層電晶體 ＋ 10–15 層金屬（x、y 交替，不能交叉）；via；電源、接地、時脈層 |
| 導線 | local（不 buffer）、intermediate、global（buffer） |
| Bus | 一次一個送、bus master → 不能擴充 |
| Tile | 一個 tile 一個 router |
| Bisection bandwidth | 剪幾條才分兩半（取最少）；大 → path diversity、不怕塞車 |
| Diameter | 最短路徑的最大值；小 → 延遲低 |
| Chain／ring | 1／2；ring 偶爾用 |
| Fat tree | 越往根越粗；叢集電腦用 |
| Mesh／torus／folded torus | NoC 常用；torus 有長 link → folded（每條 2 倍長、沒有長 link） |
| Hypercube | 遞迴；相鄰 Hamming distance 1；diameter log n、bisection n/2 |
| Clos | 三層 r、m、n；m ≥ n 可重排接通；m ≥ 2n−1 不用重排 |
| Butterfly | 2 × 2 switch、log n 欄；一對只有一條路；diameter log n ＋ 1 |
| 實測 | n = 16 的 diameter、bisection 跟公式一致；folded torus 最長 link 7 → 2；buffered wire 長度加倍延遲約 2 倍；本機核心之間換手 32–48 ns，看不出距離 |

---

## 19. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| 導線延遲 ∝ 長度²（Elmore） | [第 24 支](lecture24_note.md) |
| Cache 的 bank、LLC | [第 24 支](lecture24_note.md)、[第 21 支](lecture21_note.md) |
| 訊息傳送、flow control | [第 28 支](lecture28_note.md) |
| Routing、deadlock、router 的 pipeline | [第 29 支](lecture29_note.md) |
| Arbiter、allocator | [第 30 支](lecture30_note.md) |
| NUCA、synthetic traffic | [第 31 支](lecture31_note.md) |
| Cache coherence（核心之間搬 line 的協定） | [第 33 支](lecture33_note.md)–[第 37 支](lecture37_note.md)（Ch9） |
| 電源層、可靠度 | [第 49 支](lecture49_note.md)（Ch12） |

---

## 附：本講一句話總結

> **多核心晶片上的 core、LLC 的 bank、memory controller 和加速器要靠一個 network on chip 連起來：每個 node（實際上是一個 tile）有一個 router 負責送、收和轉送訊息，router 之間的 channel 是平行銅線加上入口的 buffer，而長導線的延遲跟長度平方成正比，所以切成 √(κ/d)·L 段、中間插 buffer，變成跟長度成正比；晶片的導線在 10–15 層 x、y 方向交替的金屬層裡以 3D 的方式走；共用 bus 一次只能一個人送，不能擴充，所以要選 topology，用 bisection bandwidth（越大越不怕塞車）和 diameter（越小越快）評估：chain 幾乎不用、ring 偶爾用、fat tree 和 hypercube 適合叢集電腦、mesh 和（把長 link 折掉的）folded torus 最常用在晶片上，Clos 和 butterfly 用內部的 switch 換取 log n 的 diameter。本機實測：n = 16 的各種 topology 用 BFS 和窮舉切法算出來的 diameter 和 bisection bandwidth 都跟公式一致，folded torus 把最長的 link 從 7 縮到 2，而本機任兩個核心之間換手一條 cache line 約 32–48 ns，看不出離得遠就比較慢。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 8: The On-chip Network (Part I/V) -- Network Topologies and Basic Concepts 的 YouTube 自動字幕（59:49）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch8 §8.1（p.335–346）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
