# 設計自動化（一）第十五支影片完整筆記：Clock tree synthesis 與課程總複習（ch8 CTS）

> **課程**：Electronic Design Automation (1)（清大 CSR 5302）— Lecture 14 | Chapter 8. Clock Tree Synthesis（播放清單第 15 支）
> **講者**：李炯霆（JT Li），清大半導體學院，2026 春，NTHU OCW 錄影
> **來源**：YouTube 錄影（44:17，無字幕，whisper 轉錄）＋ 投影片截圖（「Clock Tree Synthesis」，主要引用 vlsi-backend-adventure.com/cts.html 等網路資料與 Adam Teman 的投影片）＋ 課本 Wang, Chang, Cheng（eds.）*Electronic Design Automation* Ch13 §13.3 Clock network design（p.763–）
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿辨識不清、或影片／投影片／課本說法不一致之處
> - `[mm:ss]`：影片時間點
>
> ⚠️ 播放清單標題寫「Lecture 14」，在播放清單是第 15 支；檔名依播放清單順序。這是整門課的最後一支。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. CTS 在流程中的位置與它為什麼難](#1-cts-在流程中的位置與它為什麼難)
- [2. CTS 要回答的問題與 clock 的問題](#2-cts-要回答的問題與-clock-的問題)
- [3. CTS 的限制、輸入與目標](#3-cts-的限制輸入與目標)
- [4. Latency／insertion delay](#4-latencyinsertion-delay)
- [5. Clock tree routing：NDR 與 via pillar](#5-clock-tree-routingndr-與-via-pillar)
- [6. Clock tree 的最佳化與 buffer insertion](#6-clock-tree-的最佳化與-buffer-insertion)
- [7. HFNS 與 clock buffer](#7-hfns-與-clock-buffer)
- [8. Pre-CTS 的準備與檢查](#8-pre-cts-的準備與檢查)
- [9. Clock 分布結構：H-tree、grid、spine](#9-clock-分布結構h-treegridspine)
- [10. CCD 與 post-CTS 最佳化](#10-ccd-與-post-cts-最佳化)
- [11. 課程總複習](#11-課程總複習)
- [12. 【實測】](#12-實測)
- [13. 一頁速查表](#13-一頁速查表)
- [14. 與前面講次的連結](#14-與前面講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
CTS 在 placement 和 signal routing 之間：把一個 clock source 經過 buffer／inverter 平衡地送到所有 FF
   目標：zero／minimum skew、insertion delay 合理；限制：max transition、max cap、max fanout…
   clock 吃掉 30–40% 功耗 → clock gating 等 clock logic cell 讓樹更複雜
   routing：clock 比 signal 先 route；NDR（加寬、加間距、shielding）、via pillar
   最佳化：buffer／gate sizing、relocation、delay insertion、useful skew
   結構：H-tree（低 skew、低功耗、floorplan 彈性差）、grid（skew 最低、耗電）、spine（折衷）
   現在：CCD（clock 和 data path 一起最佳化）
最後：整門課的 recap
```

---

## 1. CTS 在流程中的位置與它為什麼難

[00:00]–[05:30]　投影片封面、5.1 Introduction（Kahng 等人）、Clock Tree Synthesis (CTS)、Clock Tree Synthesis Is Challenging!

- 【口頭】老師上網查了資料來分享；沿用 UCSD 那張流程圖：**physical design = partitioning → chip planning → placement → clock tree synthesis → signal routing → timing closure**，CTS 在 placement 和 signal routing 中間。
- **CTS** 是 P&R 最重要的階段之一；**CTS 的 QoR 決定 timing 收斂和功耗**。大部分 IC 的 **clock 吃掉總功耗的 30–40%**，所以有效率的 clock 架構、clock gating、clock tree 實作能幫忙降功耗。
- 分配 clock、平衡負載的過程叫 CTS：把 clock 送到所有 sequential 元件。**CTS 是沿著 ASIC 的 clock path 插入 buffer 或 inverter，達到 zero／minimum skew 或 balanced skew。** CTS 前，所有 clock pin 都由同一個 clock source 驅動；CTS 起點是 clock source，終點是 sequential cell 的 clock pin。
- 圖：**unbuffered clock tree**（一個 CLK 直接拉到所有 FF）→ **buffered／balanced clock tree**（一層層 buffer）。
  - 【口頭】沒有 buffer 時一個 clock 推很多 FF，loading 很大、timing 很辛苦。
- **CTS 很有挑戰**：
  - clock tree 吃掉多達 40% 的功耗 → 積極降功耗 → **clock tree 變複雜，有 clock logic cell（CLC），例如 clock gating、divider、MUX**。
  - 跨 process、voltage、temperature、operating scenario 的複雜 timing constraint。
  - **On-chip variation → 需要更多 design margin。**

---

## 2. CTS 要回答的問題與 clock 的問題

[05:30]–[08:57]　投影片 clock path 圖（how many buffer levels…）、Clock Problem

- Clock tree 是從 clock source（root）到 clock sink（leaf，FF 的 clock pin）的路徑；CTS 就是建這條 clock path 的過程。要回答：
  - **幾層 clock buffer？** 用**哪種** clock buffer？
  - 最下層一個 clock buffer 推幾個 FF？
  - 上層的 buffer 推的是 buffer——幾個 buffer 組成一組？
  - **FF 怎麼分組？**（1）通常在 placement 之後；（2）用什麼 buffer？
  - **buffer 放在哪？**（1）通常在 placement 之後；（2）現在要把新插入的 clock buffer 擺上去。
  - **怎麼 route？**
- **Clock 的問題**：
  - clock skew（例：同一個 system clock 經過 delay 到 Unit B，CLK_A 和 CLK_B 的波形錯開）
  - clock insertion delay 太長
  - 跨 clock 之間的 skew
  - clock net 負載很重
  - **clock 很耗電**
  - clock 對 signal 的 coupling（crosstalk）
  - clock net 的 electromigration

---

## 3. CTS 的限制、輸入與目標

[08:57]–[12:45]　投影片 Constraints To Follow In CTS、buffer 能推幾個 FF、Inputs of CTS、Goals of CTS

**Clock tree constraints**（teamvlsi.com）：
- target skew
- target maximum transition
- target maximum capacitance
- min／max insertion delay（latency）
- clock tree cell list
- maximum fanout
- 偏好的 clock tree routing layer 和 **NDR**
- cell density
- **cell halo**：clock tree instance 周圍要留 halo，避免太密的 placement 造成 IR／crosstalk 問題（在 cell 的 x、y 方向設 halo constraint）。
- 【口頭】clock buffer 要特別設計，rise 和 fall 要非常接近；clock layer 如果比較高，從 clock source 的 pin 浮上去要用 via pillar，DRC violation 常在那裡發生。

**一個 buffer 能推幾個 FF？**
- 需要 cell 的 timing library。
- 給一個 clock buffer、假設它的輸入 transition，看它能推多少總電容而仍滿足**輸出 max transition**。
- 再看 FF 的 input cap，以及估計的線長和 RC。
- → 這個 buffer 能推幾個 FF clock pin。【實測】見 [§12](#12-實測)。

**CTS 的輸入**：technology file（.tf）、netlist、SDC、library file（.lib、.lef）和 TLU+ file、placement DEF、**clock specification file**（insertion delay、skew、clock transition、clock cell、NDR、CTS tree type、CTS exception、buffer／inverter 清單等）。

**CTS 的目標**：
- 滿足 **clock tree design rule constraint**（是上限；不滿足會報 violation）：max transition delay、max load capacitance、max fanout、[max buffer levels]。預設值例：Max Trans 0.5 ns、Max Cap 0.6 pF、Max Fanout 2000、Max Levels（per gate level）20。
- 滿足 **clock tree target**（**最高優先**）：**min skew**、min／max insertion delay（latency）。

---

## 4. Latency／insertion delay

[12:45]–[14:42]　投影片 Clock Latency / Insertion Delay

- **Total latency = source insertion delay + network insertion delay**。
  - **Source latency**：clock source（例如 PLL）到 clock definition point（block 的 port）的時間。
  - **Network latency**：clock definition point 到 design 裡 clock leaf cell 的時間。
- latency 就是 clock 訊號到達 register 輸入的總時間。圖：PLL → source insertion delay（綠色 buffer）→ block port（clock definition point）→ network insertion delay（紅色 buffer）→ FF1、FF2。

---

## 5. Clock tree routing：NDR 與 via pillar

[14:42]–[17:54]　投影片 After Trees Are Built → Routing、Non-Default Rule (NDR)、ICC II Non-Default Rules、VIA Pillar（ISPD 2017）

- clock tree 建好後要 route。投影片：**請想想怎麼合成一棵 clock tree**；**clock tree routing 通常在一般 signal net 之前做，因為它們比較 critical**。
- **NDR**：除了預設 routing rule 之外的使用者自訂規則；常用來「**harden**」敏感的 net（例如 clock net）：
  - NDR 讓 clock route 對 crosstalk 或 EM 比較不敏感；
  - **double／triple width** 避免 electromigration；
  - **double／triple spacing** 避免 crosstalk；
  - NDR 也會改善 insertion delay。
  - 圖：預設規則下 Sig1、Clk、Sig2 等距；NDR 下 Clk 兩側加 Gnd（**ground shielding**），而且 **double spacing、double width**。
- ICC II 可以用 NDR route clock net（double-spacing、double-width、shielding）；clock net 常進一步要求 **100% via optimization**（double via）提高可靠度。
- **Via pillar**（ISPD 2017）：single stacking via vs via pillar；**大 driver + 上層厚金屬 + via pillar，降低電晶體、金屬和 via 的電阻**；via pillar 的支援很複雜，**需要所有實作階段的 EDA 都支援**。

---

## 6. Clock tree 的最佳化與 buffer insertion

[17:54]–[21:54]　投影片 Clock Tree Optimizations、Long Interconnect – Why Adding Buffer（Figure 13.44）

- **Clock tree 最佳化**：Before → After：**gate relocation**、**buffer relocation**、**buffer sizing**、**delay insertion**、**gate sizing**。
- **Useful skew**：故意讓 clock 晚一點到某個 FF，借時間給比較長的 data path（圖：兩級 logic 4 + 2 和 4 + 4，cycle = 10，第二個 FF 的 clock +2）。
  - 【口頭】有時還要插 delay（例如加一個 4X 的東西），都在調這些 delay。
- **長的線為什麼要加 buffer**（課本 Figure 13.44）：
  - (a) clock buffer／driver 的 switch-level 模型：輸入電容 $C_b$、內部延遲 $T_b$、輸出電阻 $R_b$。
  - (b) 插入 buffer 把長線切開：原本 $R_{up} + R_{down}$ 驅動 $C_{up} + C_{down}$，插入後前半段只驅動 $C_{up}$ 和 buffer 的輸入電容、後半段由 buffer 重新驅動。
  - 因為 Elmore delay 隨線長**平方**成長（[第 14 支](lecture14_note.md) §11），切成 k 段就把它變成接近**線性**。【實測】見 [§12](#12-實測)。

---

## 7. HFNS 與 clock buffer

[21:54]–[25:42]　投影片 Difference between HFNS & CTS、Difference between clock buffer and normal buffer、Pre-CTS Optimization

**High fanout net synthesis（HFNS）vs CTS**：
- CTS 用 **rise、fall 時間相等**的 clock buffer 和 clock inverter；HFNS 用一般 buffer／inverter，rise、fall 不一定相等。
- HFNS 主要用在 **reset、scan enable 等靜態訊號**：fanout 很大，但沒有嚴格的平衡和省電要求。
- **clock tree 的功耗要特別注意，因為它一直在切換**；HFNS 的 net 比較靜態，不用太在意功耗。
- clock tree routing 用 NDR。

**Clock buffer vs 一般 buffer**：clock buffer 的 rise、fall 時間相等，**避免 pulse width violation**；設計時讓 rise、fall 對稱，可能讓 clock buffer 比一般 buffer 大。clock buffer 通常設計成輸入 50% duty cycle 時輸出也是 50%。投影片註：**要跑很多 SPICE 模擬**。

**Pre-CTS optimization**：讀入 place opt 的結果，再做 pre-CTS：
- 最佳化指令：don't_use、size_only。
- **HFNS**：high fanout net 在 CTS **之前**合成；HFNS 就是對 high fanout net 做 buffering；high fanout net 的 fanout 可能超過 1000，例如 reset、clear。投影片註：HFNS 也可以在 synthesis 時做；要設合適的 fanout 上限。
- routing rule：shielding、NDR；delay model。

---

## 8. Pre-CTS 的準備與檢查

[25:42]–[28:42]　投影片 Checks need to be done before CTS、Implications on Area

**CTS 前要檢查**：
- placement legality
- power stripe、standard cell rail，並確認 PG 連接
- timing QoR（setup 要在控制內）
- timing DRV（design rule violation）
- high fanout net 已經處理（例如 scan enable 或其他靜態訊號）
- **congestion**（在 congestion 很嚴重或有 hotspot 的設計上做 CTS，會產生更多 congestion 和其他問題（noise、IR））
- 拿掉 clock buffer、inverter 上的 don't_use attribute（【口頭】synthesis 或 HFNS 時可能設成不能用，CTS 時要能用）
- 檢查 clock path 上既有的 cell 是不是 balanced cell（CK\* cell）
- 檢查並確認 clock 元件上的 don't_touch、don't_size attribute

**對面積的影響**（Adam Teman 投影片）：clock network 由 clock generator、clock element、clock wire 組成，都吃面積：
- clock generator（例如 PLL）可能很大；
- clock buffer 散布在整顆晶片；
- **clock wire 吃大量 routing 資源**——例：**Intel Itanium 有 4% 的 M4/M5 用在 clock routing**。
- **routing 資源最關鍵**：要低 RC（為了 transition 和功耗）→ 用高層、寬的金屬；要連到每個 clock element（FF）；要分布到整顆晶片；要用 via stack 從高層下來。

---

## 9. Clock 分布結構：H-tree、grid、spine

[28:42]–[37:30]　投影片 Building an actual Clock Tree、Industrial H-Tree Examples、Lower Skew – Clock Grids、DEC Alpha – Generations of Grids、Clock Spines、Summary of main clock dist. approaches

**建一棵真正的 clock tree**：
- **完全平衡的做法：H-tree**——一個大的中央 driver；遞迴的 H 結構讓線長相等；在分岔點把線寬減半以減少反射。
- 更實際：**tapered H-tree**（越往下 buffer 和線越小），但還是很難做到。
- **標準 CTS 做法**：FF 分布不均勻，要建一棵 balanced tree（圖：PLL 出來，不平衡的樹有 large skew and jitter，平衡後是 medium skew and jitter）。
  - 【口頭】開頭用大 buffer，往下逐漸 tapering 變小。

**工業界的 H-tree**：IBM PowerPC（2002）、Intel Itanium 2（2005，ISSCC 2000 的圖）。

**Clock grid（降低 skew）**：
- 優點：skew 由 grid 密度決定，對負載位置不太敏感；clock 訊號到處都有；**容忍 process variation**；通常 skew 極低。
- 缺點：**大量的線和功耗**——線電容大、需要很強的 driver（pre-driver 電容大）、routing 面積大。要減少這些代價就把 grid pitch 弄粗，但 skew 變差、失去主要優點；不要過度設計，skew 能容忍多大就讓它多大。
- **grid 看起來不適合 SoC**。
- 圖：clock driver tree 跨整個晶片高度，內部各層短接在一起；grid 直接餵 FF、沒有 local buffer。
- **DEC Alpha 各代的 grid**：
  - 21064（EV4）1992：0.75 µm、200 MHz、170 萬顆；大的中央 driver、clock grid；**240 ps skew**。
  - 21164（EV5）1995：0.5 µm、300 MHz、930 萬顆；中央 driver、兩個 final driver、clock grid；總 driver 大小 **58 cm**！**120 ps skew**。
  - 21264（EV6）1998：0.35 µm、600 MHz、1520 萬顆；為 gating 分區；總 driver 大小 40 cm；**75 ps skew**。
  - 【口頭】一般 SoC 不太可行，但 Alpha 的 CPU 確實這樣做。

**Clock spine**：
- clock grid 太耗電（也太耗 routing）；另一個做法是用 spine：**對每條 spine 建 H-tree**，再從 spine 往外做局部 clock 分布。
- **Pentium 4（2001）用 clock spine**；後來的 Pentium 4 用更多 spine（Bindal, ISSCC 2003：PLL → 1st level buffer → 2nd level buffer → 3rd level buffer → spines）。

**主要 clock 分布方式的比較**：

| 結構 | Skew | Cap／面積／功耗 | Floorplan 彈性 |
|---|---|---|---|
| H-tree | 低／中 | 低 | 低 |
| Grid | **低** | **高** | 高 |
| Spine | **高** | 中 | 中 |

- **Tree**：skew 低、routing 電容最小、功耗低，但 floorplan 彈性差。
- **Grid（mesh）**：skew 低，但 routing 電容增加、功耗差；Alpha 用 global clock grid 加 regional clock grid。
- **Spine**：spine 很寬所以 RC delay 小；spine 要平衡 delay，困難；routing 電容比 grid 低、可能比 H-tree 高。
  - 【口頭】spine 擺在哪是個問題；pin 怎麼 tap 到 spine 也很辛苦。
  - ⚠️ 表格說 spine 的 skew「高」，但下面文字說 spine 寬、RC delay 小、可以平衡 delay——指的是 spine 要靠局部平衡才能壓低 skew；投影片沒有細說，照錄。

---

## 10. CCD 與 post-CTS 最佳化

[37:30]–[41:33]　投影片 Concurrent Clock and Data Optimization (CCD)、Comparing Classic CTS versus CCD、Post-CTS Optimization with GRE、Timing Fixes in placement

- **CCD（concurrent clock and data optimization）**（YouTube「Lec 47 - concurrent clock and data optimization」）。
- **現在 clock tree 和 data path 一起考慮**：

| | Classic CTS | Concurrent Clock & Data |
|---|---|---|
| 建 clock tree 時 | 忽略 data path | 完全知道 data path 的 timing |
| 目標 | **最小化 skew** | **滿足 setup／hold timing** |
| data path | CTS 之後才最佳化，clock tree 不動 | 和 clock tree 的 incremental 修改一起最佳化（綠色 buffer） |

- **Post-CTS optimization with GRE**（Synopsys 流程）：place_opt → **clock_opt**（`-from build_clock -to route_clock`；`-from final_opto`：virtual route based optimization → global route → global route based optimization → incremental global route）→ route_auto（track assign、detail route）→ route_opt。
  - 【口頭】最後的結果要考慮 virtual route、global route。
- 參考：YouTube「Lec 43 - Timing Fixes in placement」。

---

## 11. 課程總複習

[41:33]–[44:15]　投影片 Recap（Welcome suggestions, please fill out eval form）、片尾「2026 設計自動化一 Design automation (I)」

- **從演算法和 complexity 分析開始**：
  - 幾種 graph search——traveling salesman problem、breadth first、depth first、branch-and-bound、longest path、shortest path…（[第 3](lecture03_note.md)、[4 支](lecture04_note.md)、[第 7 支](lecture07_note.md)）
  - dynamic programming、bin packing、topological sort
- **Partitioning、floorplan、placement**：
  - min-cut with terminal propagation、Gordian（quadratic programming）、force-directed、clustering、simulated annealing（[第 8](lecture08_note.md)、[10 支](lecture10_note.md)）
  - slicing（Polish expression）、non-slicing（B\*）（[第 9 支](lecture09_note.md)）
- **Routing**：maze routing、line search、global route、track assignment、channel route、detailed route、design rule（[第 11](lecture11_note.md)–[12](lecture12_note.md)、[14 支](lecture14_note.md)）
- **Clock tree routing、synthesis**（[第 14 支](lecture14_note.md)、本講）
- **P/G routing 與 co-optimization**（[第 14 支](lecture14_note.md)）
- **Logic optimization、BDD、technology mapping**（[第 6 支](lecture06_note.md)）
- **Delay、timing**（[第 6](lecture06_note.md)、[14 支](lecture14_note.md)）
- **別忘了修 ML 的課。**
- 【口頭】希望大家填課程意見調查，給老師 feedback。

---

## 12. 【實測】

程式：`experiments/lecture15/cts.py`，輸出：`experiments/lecture15/output.txt`。線的 RC 用[第 14 支](lecture14_note.md)的 0.18 µm 數字（0.075 Ω/µm、0.118 fF/µm）；buffer 的 $R_b$ = 200 Ω、$C_b$ = 4 fF、$T_b$ = 15 ps 是**假設的**示意值。

```
(1) wire length (um): unbuffered delay vs best buffered delay [fs]
    L=  1000: unbuffered       29125   buffered     44125 (k=1)
    L=  2000: unbuffered       66300   buffered     81300 (k=1)
    L=  4000: unbuffered      167200   buffered    162600 (k=2)
    L=  8000: unbuffered      475200   buffered    325200 (k=4)
    L= 16000: unbuffered     1516000   buffered    650400 (k=8)
(2) CKBUFX4 (R=400 ohm): can drive 29 FF clock pins within 100 ps transition
(2) CKBUFX8 (R=200 ohm): can drive 58 FF clock pins within 100 ps transition
(2) CKBUFX16 (R=100 ohm): can drive 117 FF clock pins within 100 ps transition
(3) star (each sink wired straight from source): skew = 35625 fs
    H-tree: every sink is at the same wire distance and sees an identical subtree -> skew = 0 by symmetry
```

- **(1) 為什麼長線要加 buffer**：不加 buffer 時，線長每加倍 delay 約乘 3–4 倍（平方成長）；用最佳的 buffer 數，16 mm 時 delay 從 1516 ps 降到 650 ps，而且大約和長度成正比（每加倍約乘 2）。短線（≤ 2 mm）加 buffer 反而變慢，因為 buffer 本身有內部延遲——所以 buffer 數要依長度選。
- **(2) 一個 buffer 能推幾個 FF**：用投影片的思路（給定輸出 max transition，transition ≈ 2.2·R·C，每個 FF clock pin 負載 = FF input cap 1.5 fF + 20 µm 線的電容），驅動電阻減半，能推的 FF 數就加倍：29、58、117。
- **(3) Skew**：16 個 sink 排成 4×4、clock 從中心直接拉線（star），離中心近和遠的 sink 差 35.6 ps；H-tree 讓每個 sink 到 source 的線長和子樹都一樣，skew 依對稱性為 0——這就是 H-tree「完全平衡」的意思（實際上 FF 分布不均，所以標準 CTS 還是要建 balanced tree）。

---

## 13. 一頁速查表

| 主題 | 重點 |
|---|---|
| 位置 | placement 之後、signal routing 之前 |
| 目的 | 插 buffer／inverter，zero／min skew、合理 insertion delay |
| 為什麼重要 | clock 佔 30–40% 功耗；clock gating 等 CLC 讓樹更複雜；PVT、OCV |
| 限制 | max transition、max cap、max fanout、max buffer levels（上限）；min skew、latency（目標，最高優先） |
| 一個 buffer 推幾個 FF | 看 max transition、FF input cap、線長 RC |
| Latency | source insertion delay + network insertion delay |
| Routing | clock 比 signal 先 route；NDR：double width（EM）、double spacing（crosstalk）、shielding；double via；via pillar |
| 最佳化 | buffer／gate sizing、relocation、delay insertion、useful skew |
| 長線加 buffer | Elmore 平方 → 切段後近似線性 |
| HFNS | reset、scan enable 等靜態 high fanout net；CTS 前做；一般 buffer |
| Clock buffer | rise = fall，避免 pulse width violation |
| Pre-CTS 檢查 | legality、PG、setup QoR、DRV、HFN、congestion、don't_use／don't_touch |
| 結構 | H-tree（低 skew、低功耗、彈性差）、grid（skew 最低、耗電，Alpha）、spine（Pentium 4） |
| CCD | clock 和 data path 一起最佳化，目標是 setup／hold 而不是 skew |

---

## 14. 與前面講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| clock routing 演算法（H-tree、geometric matching、zero skew、DME）、Elmore delay | [第 14 支](lecture14_note.md) §9–12 |
| setup／hold、slack、Liberty timing model | [第 6 支](lecture06_note.md) §18、§22 |
| via pillar、NDR | [第 12 支](lecture12_note.md) §5、[第 14 支](lecture14_note.md) §1 |
| placement、congestion | [第 10 支](lecture10_note.md)、[第 12 支](lecture12_note.md) §13 |
| 整門課的位置與評分 | [第 1 支](lecture01_note.md) |

---

## 附：本講一句話總結

> **Clock tree synthesis 在 placement 和 signal routing 之間，沿著 clock path 插入 rise／fall 對稱的 clock buffer 或 inverter，在 max transition、max cap、max fanout 等限制下把 skew 壓到最小、讓 insertion delay（source + network latency）合理；因為 clock 吃掉 30–40% 功耗又一直在切換，還有 clock gating、跨 PVT 和 OCV 的問題，所以它很難：clock net 比 signal 先 route，用加寬、加間距、shielding 的 NDR 和 via pillar 來「harden」，再用 buffer／gate sizing、relocation、delay insertion、useful skew 最佳化，長線要切段加 buffer（本機實測 16 mm 線從 1516 ps 降到 650 ps）；分布結構有 H-tree（低 skew 低功耗但 floorplan 彈性差）、grid（skew 最低但很耗電，DEC Alpha）、spine（Pentium 4），現在則走向 clock 和 data path 一起最佳化的 CCD；最後老師把整門課——演算法與 complexity、partitioning／floorplan／placement、routing、clock 與 P/G、logic optimization 與 timing——做了總複習。**

---

*筆記依據李炯霆「Electronic Design Automation (1)」Lecture 14 | Chapter 8. Clock Tree Synthesis 錄影（NTHU OCW，44:17；影片無字幕，以 whisper large-v3-turbo 轉錄）、投影片截圖、Wang, Chang, Cheng（eds.）*Electronic Design Automation: Synthesis, Verification, and Test* Ch13 §13.3 整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
