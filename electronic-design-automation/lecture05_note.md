# 設計自動化（一）第五支影片完整筆記：Spanning tree 與 rectilinear Steiner tree（ch2 Steiner tree）

> **課程**：Electronic Design Automation (1)（清大 CSR 5302）— Lecture 5 | ch2. Steiner_Tree（播放清單第 5 支）
> **講者**：李炯霆（JT Li），清大半導體學院，2026 春，NTHU OCW 錄影
> **來源**：YouTube 錄影（10:58，無字幕，whisper 轉錄）＋ 投影片截圖（Unit 1A 最後幾頁）＋ 課本 Wang, Chang, Cheng（eds.）*Electronic Design Automation* Ch12 §12.4.3 Steiner trees（p.700–704）
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、投影片和課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿辨識不清、或影片／投影片／課本說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Spanning tree vs Steiner tree](#1-spanning-tree-vs-steiner-tree)
- [2. Blockage 與現成的工具](#2-blockage-與現成的工具)
- [3. 怎麼改成 rectilinear minimum Steiner tree：翻 L 形](#3-怎麼改成-rectilinear-minimum-steiner-tree翻-l-形)
- [4. 其他幾何：any-angle、rubber band、escape routing](#4-其他幾何any-anglerubber-bandescape-routing)
- [5. CAD 相關的 conference 與 journal](#5-cad-相關的-conference-與-journal)
- [6. Unit 1A 總結與近期產業新聞](#6-unit-1a-總結與近期產業新聞)
- [7. 【實測】MST vs MRST](#7-實測mst-vs-mrst)
- [8. 一頁速查表](#8-一頁速查表)
- [9. 與前後講次的連結](#9-與前後講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
MST（任意角度）→ layout 只能走水平垂直 → rectilinear spanning tree（Manhattan 距離）
   → 加 Steiner point → Steiner tree → minimum rectilinear Steiner tree（MRST，NP-complete）
   → MST 當 approximation（最多差 50%）
   → 實用做法：找 L 形翻過去，讓線段共用
   → 真實情況：blockage、換 layer、any-angle / rubber band（packaging）
```

---

## 1. Spanning tree vs Steiner tree

[00:00]–[03:54]　投影片 Spanning Tree v.s. Steiner Tree；課本 §12.4.3（p.700–702）

- **Manhattan distance**：兩點 (x₁, y₁)、(x₂, y₂) 的距離 $d_{12} = |x_1 - x_2| + |y_1 - y_2|$。
- **Rectilinear spanning tree**：用 Manhattan path（只走水平垂直）連接各點的 spanning tree（圖 b）。
  - 【口頭】前面講的 MST 如果用任意角度的線，layout 上很不方便，所以要變成 orthogonal（正交）。
- **Steiner tree**：連接各點的 tree，**可以額外加點（Steiner points）**來幫忙連接（圖 c）。
  - 【口頭】它還是一棵 tree，沒有 loop；加了 Steiner point 之後，拉出來的 tree 總線長變短。總線長最短的叫 **minimal rectilinear Steiner tree**。
- **Minimum rectilinear spanning tree 是 P；minimum rectilinear Steiner tree（MRST）是 NP-complete。**
  - **Spanning tree 演算法可以當 Steiner tree 問題的 approximation（最多離最佳解 50%）。**

圖（三個 8×8 格子）：(a) 五個點；(b) rectilinear spanning tree；(c) 加了 Steiner points（×）的 rectilinear Steiner tree，線更短。

- 【口頭】問題是 Steiner point 怎麼找？沒找到的話，得到的 rectilinear Steiner tree 就不是最好的。
- 【課本補充】
  - $MRST(P) = MST(P \cup S)$，P 是原本的點、S 是 Steiner point（課本式 12.2）。
  - Steiner point 可能有無限多個候選，但 **Hanan's theorem**（1966）證明：一定存在一棵 MRST，所有 Steiner point 都在 **Hanan grid** 上——過每個 pin 畫水平線和垂直線，交點就是 Hanan grid。
  - 課本也說：多 pin 的 net 一個天真的做法是先用 MST 把 net 拆成一堆兩點連線再一條條繞，但結果取決於拆法、常常不是最佳；用 Steiner tree 更自然（p.700–701）。

---

## 2. Blockage 與現成的工具

[03:24]–[05:17]　投影片 What if there is a blockage?、RMST-Pack、FLUTE、Prim-Dijkstra Revisited

- **有 blockage（障礙物）怎麼辦？**（圖：五個點中間有一塊長方形障礙）
  - 【口頭】真實做 layout 時會有 blockage 擋住，要得到好的 tree 不容易；而且 minimum Steiner tree 是 NP-complete，點多時花很久，所以通常用 **heuristic** 取代。
- 現成資源（【口頭】這些已經研究很多，網路上有 paper 和程式）：
  - **RMST-Pack**：Rectilinear Minimum Spanning Tree 演算法（UCSD VLSI CAD 網站）。
  - GitHub `rectilinear_spanning_graph`：建 rectilinear spanning graph、找 MST，並實作 binary search tree。
  - **FLUTE**（Iowa State，Chris Chu）：**Fast Lookup Table Based Technique** for Rectilinear Steiner Minimal Tree construction and wirelength estimation。【口頭】用很快的查表找 rectilinear Steiner tree，可以拿來建 routing，甚至用來 estimate wirelength。
  - **Prim-Dijkstra Revisited: Achieving Superior Timing-driven Routing Trees**（Alpert, Chow, Han, Kahng, Li, Liu, Venkatesh，ISPD 2018）。【口頭】Prim 和 Dijkstra 的演算法在 2018 年又被 revisit，因為在 routing 上被大量使用。

---

## 3. 怎麼改成 rectilinear minimum Steiner tree：翻 L 形

[05:17]–[07:33]　投影片 How To Transform Into Rectilinear Min Steiner Tree

圖：一棵 rectilinear spanning tree 經過三步變成 Steiner tree。

1. 找到一個 **L 形**的連線，把它**翻過去（flip）**：新的 tree 少了一段垂直線，線長變短。
2. 另一個 L 形翻上去，和既有的線段**共用（share）**一段。
3. 再翻一個下來。最後標出 Steiner point。

- 【口頭】這是一個 practical 的方法：找 L 形、看能不能翻過去讓線段共用。
- 投影片：**Can we improve more?**（和前面的最佳圖 (c) 比，得到的結果不完全一樣。）
- 更複雜的情況：(1) **blockage**；(2) **layer change**（【口頭】前面都假設在同一層；要換層時遇到障礙物又更難）。

---

## 4. 其他幾何：any-angle、rubber band、escape routing

[07:33]–[08:42]　投影片 More Geometry Variations、Just A Preview – Different Kind Of Routing、Trunk Route of Escape Routing

- 前面講的都是 **Manhattan** 風格的 wiring；在 **flat panel 和 packaging** 可以看到很多不同的 wiring style。
- **Any angle wiring、rubber banding、piecewise linear wiring**：投影片引用美國專利 US 8,875,083 B2（Chang et al., 2014）"Routing method for flip chip package and apparatus using the same" 的圖。
  - 【口頭】advanced packaging 的 routing 常常是在一個平面上的 **single layer routing**，不能有 crossing，要怎麼做到？
- **Escape routing 的 trunk route**（Cadence，PCB West 2016「Routing DDR4 Interfaces Quickly and Efficiently」）：
  - bundle 兩端的 breakout 完成後，就能直接拉一條「trunk route」把連線接起來。
  - trunk route 用最小 DRC 間距，之後可以用使用者定的值**重新拉開間距（re-space）**，留更大的 spacing margin。
  - 【口頭】把線 bundle 在一起，有空間的地方 re-space 一下，減少 coupling。

---

## 5. CAD 相關的 conference 與 journal

[08:42]–[09:00]　投影片 CAD Related Conferences/Journals

**重要會議**：
- **DAC**（ACM/IEEE Design Automation Conference）
- **ICCAD**（IEEE/ACM Int'l Conference on Computer-Aided Design）
- ASP-DAC（Asia and South Pacific Design Automation Conf.）
- DATE（Design, Automation, and Test in Europe）
- ICCD（IEEE Int'l Conference on Computer Design）
- CICC（IEEE Custom Integrated Circuits Conference）
- ISCAS（IEEE Int'l Symposium on Circuits and Systems）
- **ISPD**（ACM Int'l Symposium on Physical Design）
- ITC（IEEE Int'l Test Conference）
- 其他：VLSI Design/CAD Symposium（Taiwan）

**重要期刊**：
- **IEEE TCAD**（Transactions on Computer-Aided Design）
- **ACM TODAES**（Transactions on Design Automation of Electronic Systems）
- **IEEE TVLSI**（Transactions on VLSI Systems）
- **IEEE TC**（Transactions on Computers）
- IEE Proceedings – Circuits, Devices and Systems；IEE Proceedings – Digital Systems
- INTEGRATION: The VLSI Journal

【口頭】有興趣往這邊鑽研的話，可以去看這些會議和期刊的好 paper。

---

## 6. Unit 1A 總結與近期產業新聞

[09:00]–[10:54]　投影片 Summary So Far、Lately we saw

**Summary So Far**：
- 演算法類型（P、NP、NP-Complete、NP-Hard…）
- Traveling salesman problem
- Minimum spanning tree
- Minimum Steiner tree
- Hamiltonian circuit
- Circuit-Satisfiability Problem（SAT）
- Search、dynamic programming
- Bin packing
- Minimum rectilinear Steiner tree
- **Steiner tree（和 MST）在 layout 很有用；SAT 在 logic synthesis、formal verification 很有用。**

**Lately we saw**（投影片附短網址）：
- TSMC N2 Design Flow Certification
- Analog Design Migration Advancements
- Automotive-Grade IP for N5A Process
- Multi-Die System Design Advancements
- 【口頭】TSMC 走到 N2，N2 怎麼設計？analog design 怎麼 migration？車用 IP 越來越重要；advanced packaging 多個 die 合在一起（chiplet）。EDA 扮演重要角色，讓 IC design flow 更順、design productivity 增加。

---

## 7. 【實測】MST vs MRST

程式：`experiments/lecture05/steiner.py`，輸出：`experiments/lecture05/output.txt`。

做法：rectilinear MST 用 Prim（Manhattan 距離）；MRST 依 Hanan's theorem，在 Hanan grid 上窮舉最多 n − 2 個 Steiner point，取 MST(P ∪ S) 最小者（小 net 才跑得動，這本身就說明 MRST 是 NP-complete 的難處）。

```
slide-like 5-pin net: rectilinear MST = 18, MRST = 14, Steiner points ((5, 3), (5, 5)), ratio 1.286
cross 4-pin net:       rectilinear MST = 6, MRST = 4, Steiner points ((1, 1),), ratio 1.500
300 random nets (3-6 pins): worst MST/MRST ratio = 1.318  (bound 1.5)
```

- 第一行的 5 個點是依投影片圖 (a) **目測**的座標（⚠️ 不保證和投影片完全一樣，所以投影片 (c) 畫了 3 個 Steiner point，這裡找到 2 個）。加 Steiner point 讓線長從 18 降到 14，少了 22%。
- 十字形 4 點（上下左右各一點）是 MST/MRST 最差的例子：MST = 6、MRST = 4（中心放一個 Steiner point），比值**剛好 1.5**——就是投影片「at most 50% away from the optimum」的那個界。
- 300 個隨機 net 的最差比值 1.318，都在 1.5 以內。
- 5 個點、25 個 Hanan grid 點，窮舉 3 個 Steiner point 已經要 C(20,3) = 1140 次 MST；300 個隨機 net 總共跑了 15 秒。點數再多就要靠 FLUTE 這類 heuristic／查表法。

---

## 8. 一頁速查表

| 主題 | 重點 |
|---|---|
| Manhattan 距離 | \|x₁ − x₂\| + \|y₁ − y₂\| |
| Rectilinear spanning tree | 只走水平垂直的 spanning tree；最小的是 P |
| Steiner tree | 可加 Steiner point 的 tree |
| MRST | minimum rectilinear Steiner tree；NP-complete |
| MST 近似 | rectilinear MST ≤ 1.5 × MRST |
| Hanan grid | 過每個 pin 的水平垂直線交點；MRST 的 Steiner point 可只從這裡挑 |
| 實用改善 | 找 L 形翻過去，讓線段共用 |
| 真實難處 | blockage、layer change |
| 工具 | RMST-Pack、FLUTE（查表）、Prim-Dijkstra revisited（ISPD 2018） |
| Packaging | any-angle、rubber band、piecewise linear；single layer、不交叉；escape routing |
| 會議／期刊 | DAC、ICCAD、ISPD…；TCAD、TODAES、TVLSI… |

---

## 9. 與前後講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| MST、Prim、approximation | [第 3 支](lecture03_note.md) §11–12 |
| SAT 用在 logic synthesis、formal verification | 第 6 支：ch10 synthesis |
| Steiner tree 用在 global routing、wirelength 估計 | 第 10 支：placement（wirelength）；第 11–12 支：routing |
| clock tree（另一種 tree 的建法） | 第 14–15 支：clock tree synthesis |

---

## 附：本講一句話總結

> **layout 只能走水平垂直線，所以 MST 要改成用 Manhattan 距離的 rectilinear spanning tree（P），再允許加 Steiner point 就得到 Steiner tree，而總線長最短的 minimum rectilinear Steiner tree 是 NP-complete，只好用 MST 當 approximation（最多差 50%，本機用十字形四點實測正好 1.5 倍）或找 L 形翻過去讓線段共用；真實情況還有 blockage、換 layer，packaging 甚至用 any-angle／rubber band 的 single layer routing，FLUTE、RMST-Pack 和 2018 年重新檢視 Prim-Dijkstra 的論文都是現成資源；這一單元的 TSP、MST、Steiner tree、HC、SAT、DP、bin packing 之後會在 layout（Steiner tree）和 logic synthesis／formal verification（SAT）用到。**

---

*筆記依據李炯霆「Electronic Design Automation (1)」Lecture 5 | ch2. Steiner_Tree 錄影（NTHU OCW，10:58；影片無字幕，以 whisper large-v3-turbo 轉錄）、投影片截圖、Wang, Chang, Cheng（eds.）*Electronic Design Automation: Synthesis, Verification, and Test* Ch12 §12.4.3 整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
