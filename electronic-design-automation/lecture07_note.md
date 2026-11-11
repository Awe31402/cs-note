# 設計自動化（一）第七支影片完整筆記：Layout compaction——design rule、symbolic layout、constraint graph 與 longest path（ch3 compaction）

> **課程**：Electronic Design Automation (1)（清大 CSR 5302）— 第7講｜ch3.compaction（播放清單第 7 支）
> **講者**：李炯霆（JT Li），清大半導體學院，2026 春，NTHU OCW 錄影
> **來源**：YouTube 錄影（1:39:30，無字幕，whisper 轉錄）＋ 投影片截圖（Unit 5F: Layout Compaction，頁尾 Chang, Huang, Li, Lin, Liu）＋ 課本 Wang, Chang, Cheng（eds.）*Electronic Design Automation* Ch2 §2.5.1 Layout design rules（p.72–75）
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、投影片和課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿辨識不清、或影片／投影片／課本說法不一致之處
> - `[h:mm:ss]`：影片時間點

⚠️ **課本沒有 layout compaction 這一章**（課本 Ch3 的「compaction」是測試的 response compaction，完全不同的東西）。投影片寫「Readings: Chapter 6」，指的是參考書 Gerez *Algorithms for VLSI Design Automation* Ch6。本筆記只在 design rule 的部分對照課本。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Design rules](#1-design-rules)
- [2. Geometric layout vs symbolic layout](#2-geometric-layout-vs-symbolic-layout)
- [3. Compaction 的用途與種類](#3-compaction-的用途與種類)
- [4. 1D 與 2D compaction 的例子](#4-1d-與-2d-compaction-的例子)
- [5. 不等式與 constraint graph](#5-不等式與-constraint-graph)
- [6. Maximum-distance constraint 與 backward edge](#6-maximum-distance-constraint-與-backward-edge)
- [7. DAG 上的 longest path](#7-dag-上的-longest-path)
- [8. 有 cycle 的 longest path；longest 與 shortest 的關係](#8-有-cycle-的-longest-pathlongest-與-shortest-的關係)
- [9. Liao-Wong 演算法](#9-liao-wong-演算法)
- [10. 套用到真實 layout：inverter、channel routing、building block](#10-套用到真實-layoutinverterchannel-routingbuilding-block)
- [11. CMOS λ design rules](#11-cmos-λ-design-rules)
- [12. Bellman-Ford（longest 與 shortest）](#12-bellman-fordlongest-與-shortest)
- [13. Topological sort 與 relaxation](#13-topological-sort-與-relaxation)
- [14. 其他考量：jog、hierarchy、constraint generation](#14-其他考量joghierarchyconstraint-generation)
- [15. 總結與 interactive compaction](#15-總結與-interactive-compaction)
- [16. 【實測】](#16-實測)
- [17. 一頁速查表](#17-一頁速查表)
- [18. 與後續講次的連結](#18-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
design rule 很難手動處理 → 先畫 symbolic layout（只有拓樸）→ compactor 依 design rule 產生 mask layout
1D compaction（P）vs 2D compaction（NP-hard）→ 通常重複做 1D
最小距離規則 x_j − x_i ≥ d_ij → constraint graph（DAG）→ longest path = layout 寬度
最大距離規則 → backward edge → 有 cycle → Liao-Wong、Bellman-Ford（可偵測 positive cycle）
```

【口頭】前面講完 logic 部分（Boolean optimization、technology mapping），現在開始進入 layout。做 layout 很 tedious、細節很多，所以一開始先用 symbolic layout 代表，等真正的 design rule 進來後，再用 compaction 變成真正的 layout。過程中會用到 graph theory、longest path，還要處理 graph 有 cycle 的情況。

---

## 1. Design rules

[0:00]–[10:42]　投影片 Unit 5F 首頁、Design Rules (Prerequisite)、Different Width For 45-Degree Wire；課本 §2.5.1（p.72–75）

- 內容：design rules、symbolic layout、constraint-graph compaction。閱讀：Chapter 6。
- 首頁圖：9 個方塊 A–I，先往下壓、再往左壓，I 擠進中間、B 被迫往上移——【口頭】要考慮這種移動是不是 legal。
- **Design rules**：對 mask pattern 的限制，用來**提高成功製造的機率**。
- pattern 和 design rule 常用 **λ** 表示。
  - 【口頭】當時很多家 fab 各有自己的 design rule，同一個 node 每家也有點不同，例如某個 spacing 是 1 µm、另一家是 1.05 或 1.1。老師不想 1 µm 畫一次、1.05 µm 再畫一次，所以用參數表示；畫好之後給定參數值，整個 layout 的座標就產生出來。
- 最常見的 design rule：
  - **minimum width**（某一層 mask pattern 的最小寬度）：(a)
  - **minimum separation**（同一層或不同層 mask pattern 之間）：(b)、(c)、(d)
  - **minimum overlap**（不同層 mask pattern 之間）：(e)
- 三種基本 DRC check：**width、spacing、enclosure**。
- **45 度線的寬度不同**（舊製程）：投影片引用**奇景盃（Himax）IC Layout 競賽**的 design rule（成熟製程 0.13 µm）：
  - M1.W.1：Metal 1 最小寬度 A ≥ 0.15 µm
  - M1.W.2：45 度轉彎的 M1（長度 ≥ 0.6 µm）寬度 A1 ≥ 0.18 µm
  - 投影片：這份 PDF 很值得看，有很多基本資訊。【口頭】有興趣的同學可以看。
- 投影片：**design rule 不容易處理，所以才有 compaction。**
- 【課本補充】課本把 layout design rule 分成 **feature size**（例如 channel length、導線寬度）、**separation**（同一層兩個結構的距離）、**overlap**（相鄰兩層結構必要的重疊，例如 poly 和 metal 1 的 contact，metal 1 要蓋住下面的 poly）三類，並列出 λ-rule 和 µ-rule 兩套 n-well 規則（Table 2.5）。

---

## 2. Geometric layout vs symbolic layout

[10:42]–[17:09]　投影片 Layout: Geometric & Symbolic、Symbolic to real、CMOS Inverter Layout Example

- **Geometric（mask）layout**：layout pattern（長方形）的座標是**絕對的**（或 λ 的倍數）。
- **Symbolic（topological）layout**：只知道 layout 元素之間的**關係**（在下面、在左邊…）。
  - 一個 symbol 可以代表跨好幾層的元素，例如電晶體、contact cut。
  - 導線或其他元素的長度、寬度、甚至 layer 可以不指定。
  - 和電路功能沒有直接關係的 mask layer 不用指定，例如 n-well、p-well。
- **symbolic layout 配上包含目標製程所有 design rule 的 technology file，就能產生 geometric layout。**
- 流程圖：symbolic layout（`ap`）→ **symbolic to real（`s2r`）**，輸入 design rules（`rds`，**under NDA**）→ real layout（**GDSII**）。symbolic 的 METAL2 線寬 2λ。
- **Stick diagram of inverter**（symbolic layout 的例子）：
  - 沒有尺寸的 layout 元素
  - 只有拓樸重要
  - 最後的 layout 由「compaction」程式產生
- **CMOS inverter layout 例子**：symbolic layout、geometric layout（p/n diffusion、polysilicon、contact cut、metal），以及現代「poly 沒有轉彎」的 inverter（vdd、a、b、gnd）。

---

## 3. Compaction 的用途與種類

[17:09]–[19:06]　投影片 Compaction and Its Applications、Aspects of Compaction

- **Compactor**：產生 mask level 的 layout，盡量讓 layout **越密越好**。
- 用途：
  - **Area minimization**：拿掉 mask layout 裡多餘的空間。
  - **Layout compilation**：從 symbolic layout 產生 mask layout。
  - **Redesign**：自動移除 design rule violation。
  - **Rescaling／re-targeting**：把 mask layout 從一個製程轉到另一個（例如 foundry A 的 mask 轉成 foundry B）。
- 維度：
  - **1D compaction**：layout 元素只在一個方向（x 或 y）移動或縮小；通常先做 x 再做 y（或反過來）。
  - **2D compaction**：同時在兩個方向移動和縮小。
- Complexity：**1D 可以 polynomial time；2D 是 NP-hard。**

---

## 4. 1D 與 2D compaction 的例子

[19:06]–[21:54]　投影片 1D Compaction: X Followed By Y、Y Followed By X、2D Compaction

條件：每個方塊 2λ × 2λ，最小間距 1λ，一開始 layout 是 11λ × 11λ。

| 做法 | 結果 |
|---|---|
| 先 x 再 y | 8λ × 11λ |
| 先 y 再 x | 11λ × 8λ |
| 2D | **8λ × 8λ** |

- 1D 的順序會影響結果；2D 更好，但 **2D compaction 是 NP-complete，所以大部分 compactor 用重複的 1D compaction**。

---

## 5. 不等式與 constraint graph

[21:54]–[26:51]　投影片 Inequalities for Distance Constraints、The Constraint Graph

- **Minimum-distance design rule 可以寫成不等式**：$x_j - x_i \ge d_{ij}$。
- 例：最小寬度 a、最小間距 b（三個長方形，左右邊界 x₁…x₆）：
$$x_2 - x_1 \ge a, \quad x_3 - x_2 \ge b, \quad x_3 - x_6 \ge b$$
- **Constraint graph G(V, E)**：
  - 每個變數 $x_i$ 一個頂點 $v_i$。
  - 每個不等式 $x_j - x_i \ge d_{ij}$ 一條邊 $(v_i, v_j)$，權重 $d_{ij}$。
  - 多一個 source 頂點 $v_0$ 在 x = 0，其他頂點都在它右邊。
- **如果所有不等式都是 minimum-distance constraint，graph 是 acyclic（DAG）。**
- **constraint graph 的 longest path 決定 layout 的大小。**
- 例：v₀ →0→ v₁ →a→ v₂ →b→ v₃ →a→ v₄；v₀ →0→ v₅ →a→ v₆ →b→ v₃。

---

## 6. Maximum-distance constraint 與 backward edge

[26:51]–[31:09]　投影片 Maximum-Distance Constraints

- 有時候兩個元素的距離有**上限**，例如使用者要求**最大線寬**、要讓一條線維持接在 via 上等。
- maximum-distance constraint 的不等式：$x_j - x_i \le c_{ij}$，或 $x_i - x_j \ge -c_{ij}$。
- 對 constraint graph 的影響：**backward edge** $(v_j, v_i)$，權重 $d_{ji} = -c_{ij}$——**graph 不再是 acyclic**。
- longest path 仍然決定 layout 大小。
- 例：contact 和 wire：$|x_C - x_W| \le d$（$x_C$：contact 中心；$x_W$：wire 中心）。圖：min distance a、max distance b 對應 v₁ →a→ v₂、v₂ →−b→ v₁。
- 投影片：wire 偏在 contact 裡面可以（○）；wire 跑到 contact 外面（✗）——「**In fact, usually we seldom see this**」。

---

## 7. DAG 上的 longest path

[31:09]–[39:33]　投影片 DAG Longest-Path Example、Longest-Path Algorithm for DAGs

例子：v₀→v₁ (1)、v₀→v₂ (5)、v₁→v₂ (2)、v₁→v₅ (2)、v₂→v₃ (1)、v₂→v₄ (1)、v₅→v₄ (4)。
- **breadth-first** 的方式；$p_i$：$v_i$ 的 in-degree；$x_i$：從 v₀ 到 $v_i$ 的 longest path 長度；Q：queue。時間 **O(V + E)**。

| Q | p₁ | p₂ | p₃ | p₄ | p₅ | x₁ | x₂ | x₃ | x₄ | x₅ |
|---|---|---|---|---|---|---|---|---|---|---|
| 「not initialized」 | 1 | 2 | 1 | 2 | 1 | 0 | 0 | 0 | 0 | 0 |
| {v₀} | 0 | 1 | 1 | 2 | 1 | 1 | 5 | 0 | 0 | 0 |
| {v₁} | 0 | 0 | 1 | 2 | 0 | 1 | 5 | 0 | 0 | 3 |
| {v₂, v₅} | 0 | 0 | 0 | 1 | 0 | 1 | 5 | 6 | 6 | 3 |
| {v₃, v₅} | 0 | 0 | 0 | 1 | 0 | 1 | 5 | 6 | 6 | 3 |
| {v₅} | 0 | 0 | 0 | 0 | 0 | 1 | 5 | 6 | 7 | 3 |
| {v₄} | 0 | 0 | 0 | 0 | 0 | 1 | 5 | 6 | 7 | 3 |

- 步驟說明（投影片黃框）：
  - v₀ → v₁：距離 1；v₀ → v₂：距離 5（兩者 in-degree 減 1）；v₁ 的 in-degree 變 0，加入 Q。
  - v₁ → v₅：1 + 2 = 3；v₁ → v₂：1 + 2 < 5（不更新，但 in-degree 減 1）；v₂、v₅ 加入 Q。
  - v₂ → v₃：5 + 1 = 6；v₂ → v₄：5 + 1 = 6；v₃ 加入 Q；接著從 v₅ 展開。
  - v₅ → v₄：3 + 4 = 7 > 6，更新；v₄ 加入 Q。
- **關鍵**：一個頂點要等所有進來的邊都處理過（in-degree 變 0）才能展開，這時它的 x 已經是最終值。

**演算法**：

```
longest-path(G) {
    for (i ← 1; i ≤ n; i ← i + 1)
        p_i ← "in-degree of v_i";
    Q ← {v_0};
    while (Q ≠ ∅) {
        v_i ← "any element from Q";
        Q ← Q \ {v_i};
        for each v_j "such that" (v_i, v_j) ∈ E {
            x_j ← max(x_j, x_i + d_ij);
            p_j ← p_j − 1;
            if (p_j ≤ 0)
                Q ← Q ∪ {v_j};
        }
    }
}
main() {
    for (i ← 0; i ≤ n; i ← i + 1)
        x_i ← 0;
    longest-path(G);
}
```

- 投影片問：**用 depth-first search 是好主意嗎？**
  - 【口頭】請同學想想看 DFS 能不能抓到同樣的結果。（這個演算法其實是 topological order 的處理；「any element from Q」表示 Q 裡挑哪一個都可以。）

---

## 8. 有 cycle 的 longest path；longest 與 shortest 的關係

[39:33]–[43:57]　投影片 Longest-Paths In Cyclic Graphs、Just a heads up: Longest and Shortest Paths

- 有 maximum-distance constraint 的 constraint-graph compaction，要在**有 cycle 的 graph** 上解 longest path。
- 兩種情況：
  - **有 positive cycle**：longest path **沒有可行解**（繞一圈就變長），要偵測出來。
  - **所有 cycle 都是 negative**：有 polynomial-time 演算法。
- 例：一個方框裡有一個小方塊（x₁…x₄），v₁ →+1→ v₂ →+2→ v₃ →+1→ v₄，加上 max distance 的 v₄ →−3→ v₁：繞一圈是 1 + 2 + 1 − 3 = +1 > 0 → positive cycle，無解。
  - 意思：內部至少要 4，但外框最多只允許 3。

**Longest 與 shortest path**：
- 權重乘以 −1，longest path 變 shortest path，反之亦然。
- **DAG**：兩者都能在**線性時間**解。
- **有 cycle 的有向圖**：
  - 權重全正：shortest path 是 P（Dijkstra）；longest path 沒有可行解。
  - 權重全負：longest path 是 P（Dijkstra）；shortest path 沒有可行解。
  - 沒有 positive cycle：longest path 是 P。
  - 沒有 negative cycle：shortest path 是 P。

---

## 9. Liao-Wong 演算法

[43:57]–[56:00]　投影片 The Liao-Wong Algorithm、Example、Starting From Different Backward Edges?、A More Correct Way、Pseudo Code

- 把 constraint graph 的邊分成兩組：
  - **Forward edges $E_f$**：minimum-distance constraint
  - **Backward edges $E_b$**：maximum-distance constraint
- $G(V, E_f)$ 是 acyclic，可以用 longest-path 算每個頂點的最長距離。
- 重複：
  - 處理 $E_b$ 的邊來更新最長距離；
  - 對 $G(V, E_f)$ 呼叫 longest-path。
- 最差時間 **O(E_b × E_f)**。

**例子**：前面的 DAG 加上三條 backward edge：v₂→v₁ (−3)（x₂ − x₁ ≤ 3）、v₄→v₃ (−1)（x₄ − x₃ ≤ 1）、v₄→v₅ (−4)（x₄ − x₅ ≤ 4）。

| Step | x₁ | x₂ | x₃ | x₄ | x₅ |
|---|---|---|---|---|---|
| Initialize | −∞ | −∞ | −∞ | −∞ | −∞ |
| Forward 1 | 1 | 5 | 6 | 7 | 3 |
| Backward 1 | 2 | 5 | 6 | 7 | 3 |
| Forward 2 | 2 | 5 | 6 | 8 | 4 |
| Backward 2 | 2 | 5 | 7 | 8 | 4 |
| Forward 3 | 2 | 5 | 7 | 8 | 4 |
| Backward 3 | 2 | 5 | 7 | 8 | 4 |

- Backward 1：x₂ − 3 = 2 > 1，x₁ 更新成 2。Forward 2：x₁ 變大傳下去，x₅ = 2 + 2 = 4、x₄ = 4 + 4 = 8。Backward 2：x₄ − 1 = 7 > 6，x₃ 更新成 7。之後沒有變化，結束。
- 投影片：「Consider v4→v5: 8 − 4 = 4 → v5 dist unchanged」。

**從哪一條 backward edge 開始？**
- 前面的表可能讓人以為一次只挑一條 backward edge。那要從哪一條開始？
- 老師用試算表示範：依序只處理 v4→v5、v4→v3、v2→v1，到最後「沒有沒處理過的 backward edge」，但 x₃ 還是 6——除非再用一次 v4→v3 才會更新成 7。**所以要一直重複處理 backward edges。**
  - 【口頭】老師讀這篇時自己想到這個問題，拿來跟大家分享。
- **更正確的做法：每次 longest path 之後，處理所有的 backward edges（不是只一條）**，表格應該是：

| Step | 處理的邊 | x₁ | x₂ | x₃ | x₄ | x₅ |
|---|---|---|---|---|---|---|
| Forward 1 | | 1 | 5 | 6 | 7 | 3 |
| Backward-1 | v2→v1 | 2 | 5 | 6 | 7 | 3 |
| | v4→v3 | 2 | 5 | 6 | 7 | 3 |
| | v4→v5 | 2 | 5 | 6 | 7 | 3 |
| Forward-2 | | 2 | 5 | 6 | 8 | 4 |
| Backward-2 | v2→v1、v4→v3、v4→v5 | 2 | 5 | 7 | 8 | 4 |
| Forward-3 | | 2 | 5 | 7 | 8 | 4 |
| Backward-3 | v2→v1、v4→v3、v4→v5 | 2 | 5 | 7 | 8 | 4 |

**Pseudo code**：

```
count ← 0;
for (i ← 1; i ≤ n; i ← i + 1)
    x_i ← −∞;
x_0 ← 0;
do {
    flag ← 0;
    longest-path(G_f);
    for each (v_i, v_j) ∈ E_b
        if (x_j < x_i + d_ij) {
            x_j ← x_i + d_ij;
            flag ← 1;
        }
    count ← count + 1;
    if (count > |E_b| && flag)
        error("positive cycle");
} while (flag);        // 有任何變化就繼續
```

- 為什麼 count > |E_b| 還有變化就是 positive cycle：沒有 positive cycle 時，最長路徑最多經過 |E_b| 條 backward edge，所以 |E_b| + 1 輪內一定收斂。

---

## 10. 套用到真實 layout：inverter、channel routing、building block

[56:00]–[1:04:54]　投影片 How About Applying To Layout Examples? (1)、How About Channel Routing?、Prior Efforts、Applying Compaction To Building Block Layout

**Inverter**：
- 能列出所有需要考慮的 design rule 嗎？這個 inverter 由 poly、diffusion…等 base layer 的形狀組成。
- **關鍵：怎麼產生 constraint graph？** 試著畫頂點：pMOS 那邊 8 個、nMOS 那邊 4 個。contact 呢？forward edge？backward edge 呢？
- 投影片：**通常 leaf cell 的 layout 是客製的**，只是從 compaction 的角度想一想；**要讓 contact 維持在理想的狀態並不容易**。
  - 【口頭】據老師了解，現在比較少看到用 compaction 做 inverter，因為要考慮 base layer，很辛苦；在 basic standard cell 上並不容易。

**Channel routing**：
- 3 層 channel routing：pin 的位置固定，在 **Y 方向做 compaction**（降低 channel 高度）。
  - 【口頭】淺綠色是垂直走的 layer 2，藍色和紅色虛線是兩個水平 layer。另一個例子實心是 metal 3、空心是 metal 1。
- **早期的做法**（Prior efforts）：帶 contact 的線、Fig. 4「bump propagation」；Fig. 5 initial input → Fig. 6 via minimization 的結果 → Fig. 7 **往下 compaction 的結果，高度 125 units，中間的虛線是 critical path**。
  - 【口頭】當時 contact 都比較大、會突出去，做 compaction 後線會變成凹凸的樣子。

**Building block layout（BBL）**：
- 一個商用工具（約 1988 年）：
  - 擺 block
  - **先做 symbolic routing，再用 compaction 產生最後的座標**
- 圖：Ami33（BBL2）benchmark，左邊是 placement 後，右邊是 detailed routing 後。
- 【口頭】channel 寬度不事先決定，先做 symbolic routing（大致怎麼走），知道 design rule 後再用 compaction——例如三條線要從兩個 block 中間穿過，compaction 就把兩個 block 推開讓線過去。當時線不能從 block 上面穿過，一定要走 channel，所以這種 layout style 下 compaction 確實發揮作用。AMI 那家公司當時用了這個程式。

---

## 11. CMOS λ design rules

[1:04:54]–[1:06:45]　投影片 CMOS Lambda Design Rules（JHU appendix A，6 metal、0.12 µm）

**這種做法用了好幾年。**

| 規則 | 內容 | 值 |
|---|---|---|
| **N-Well** | | |
| r101 | Minimum well size | 12λ |
| r102 | Between wells | 12λ |
| r110 | Minimum well area | 144λ² |
| **Diffusion** | | |
| r201 | Minimum N+ and P+ diffusion width | 4λ |
| r202 | Between two P+ and N+ diffusions | 4λ |
| r203 | Extra nwell after P+ diffusion | 6λ |
| r204 | Between N+ diffusion and nwell | 6λ |
| r205 | Border of well after N+ polarization | 2λ |
| r206 | Between N+ and P+ polarization | 0λ |
| r207 | Border of Nwell for P+ polarization | 6λ |
| r210 | Minimum diffusion area | 24λ² |
| **Polysilicon** | | |
| r301 | Polysilicon width | 2λ |
| r302 | Polysilicon gate on diffusion | 2λ |
| r303 | Polysilicon gate on diffusion for high voltage MOS | 4λ |
| r304 | Between two polysilicon boxes | 3λ |
| r305 | Polysilicon vs. other diffusion | 2λ |
| r306 | Diffusion after polysilicon | 4λ |
| r307 | Extra gate after polysilicon | 3λ |
| r310 | Minimum surface | 8λ² |

- 【口頭】當時做 layout 很習慣用 λ rule：well 和 well 的 spacing 是多少等等。有興趣可以看這個 appendix。
- 注意這份規則裡 poly 寬（= 通道長度）是 2λ，和[第 2 支](lecture02_note.md) §31 的 MOSIS 規則一致。

---

## 12. Bellman-Ford（longest 與 shortest）

[1:06:45]–[1:26:48]　投影片 The Bellman-Ford Algorithm for Longest Paths、Example、for Shortest Paths

**Bellman-Ford（longest path 版）**：
- Liao-Wong 的替代方案。
- 能處理**邊權為負**的情況。
- 若有從 source 可到達的 cycle（這裡指 positive cycle）就回傳 FALSE，否則 TRUE。
- 時間 **O(VE)**。
- 靠 **relaxation**：近似值一直被更好的值取代，直到得到解（Wikipedia）。

```
for (i ← 1; i ≤ n; i ← i + 1)
    x_i ← −∞;
x_0 ← 0;
count ← 0;
S_1 ← {v_0};                     // 維護兩個集合
S_2 ← ∅;
while (count ≤ n && S_1 ≠ ∅) {
    for each v_i ∈ S_1
        for each v_j "such that" (v_i, v_j) ∈ E
            if (x_j < x_i + d_ij) {
                x_j ← x_i + d_ij;
                S_2 ← S_2 ∪ {v_j};
            }
    S_1 ← S_2;
    S_2 ← ∅;
    count ← count + 1;
}
if (count > n)
    error("positive cycle");     // 能偵測 cycle
```

**例子**（同 Liao-Wong 的圖，forward 和 backward edge 一起處理）——重複的「**wave front propagation**」，$S_1$ 是目前的 wave front；**k 次迭代後，算出經過 k − 1 個中間頂點的路徑的 longest-path 值**：

| S₁ | x₁ | x₂ | x₃ | x₄ | x₅ |
|---|---|---|---|---|---|
| 「not initialized」 | −∞ | −∞ | −∞ | −∞ | −∞ |
| {v₀} | 1 | 5 | −∞ | −∞ | −∞ |
| {v₁, v₂} | 2 | 5 | 6 | 6 | 3 |
| {v₁, v₃, v₄, v₅} | 2 | 5 | 6 | 7 | 4 |
| {v₄, v₅} | 2 | 5 | 6 | 8 | 4 |
| {v₄} | 2 | 5 | 7 | 8 | 4 |
| {v₃} | 2 | 5 | 7 | 8 | 4 |

投影片黃框的步驟：
- S₁ = (v₀)：v₀→v₂ (5)、v₀→v₁ (1)；S₂ = (v₁, v₂)。
- S₁ = (v₁, v₂)：從 v₁，v₁→v₂（1 + 2 < 5）、v₁→v₅（1 + 2，加入 S₂）；從 v₂，v₂→v₁（5 − 3 > 1，加入 S₂）、v₂→v₃ (6)、v₂→v₄ (6)；S₂ = (v₁, v₃, v₅, v₄)。
- S₁ = (v₁, v₃, v₄, v₅)：從 v₅，v₅→v₄（3 + 4 = 7 > 6，加入）；從 v₄，v₄→v₃（7 − 1 = 6）、v₄→v₅（7 − 4）；從 v₁，v₁→v₅（2 + 2 = 4 > 3，加入）、v₁→v₂（2 + 2 < 5）；v₃ 沒有；S₂ = (v₅, v₄)。
- S₁ = (v₄, v₅)：v₄→v₃（7 − 1）、v₄→v₅（7 − 4）；v₅→v₄（4 + 4 = 8 > 7，加入 v₄）。
- S₁ = (v₄)：v₄→v₃（8 − 1 = 7 > 6，加入 v₃）。
- S₁ = (v₃)：沒有。結束。

最終結果和 Liao-Wong 一樣：x = (2, 5, 7, 8, 4)。

**Bellman-Ford（shortest path 版）**（Wikipedia；投影片推薦 YouTube 影片 lyw4FaxrwHg，「worthwhile to watch」）：
1. 初始化：所有距離 ∞、predecessor null，source 距離 0。
2. **relax 所有邊 |V| − 1 次**：若 distance[u] + w < distance[v]，更新 distance[v] 和 predecessor[v]。
3. **檢查 negative-weight cycle**：再掃一次所有邊，若還能 relax 就有 negative cycle。
4. （找出 negative cycle）。

CLRS 版本：
```
Bellman-Ford(G, w, s)
1. Initialize-Single-Source(G, s);
2. for i ← 1 to |V[G]| − 1
3.     for each edge (u, v) ∈ E[G]
4.         Relax(u, v, w);
5. for each edge (u, v) ∈ E[G]
6.     if d[v] > d[u] + w(u, v)
7.         return FALSE;
8. return TRUE
```
- 例（CLRS）：照字典序 relax 邊 (u,v)、(u,x)、(u,y)…(z,u)、(z,x)，五個步驟 (a)–(e) 後 u = 2、v = 4、x = 7、y = −2。

---

## 13. Topological sort 與 relaxation

[1:26:48]–[1:33:45]　投影片 Another Type Of Ordering: Topological Order、Topological Sort、Relaxation、Summary

- **Topological sort**：有向圖的頂點線性排序，使得每條邊 u→v 中 u 都在 v 前面。例如頂點是要做的工作、邊是「某工作必須在另一工作之前」的限制，topological order 就是一個合法的工作順序。
  - 每個節點要等它所有的相依都拜訪過才拜訪。
  - **只有 DAG 才有** topological order；不連通的 DAG 也可以。應用很多，例如 ranking、feedback arc set。
  - 例：頂點 5、7、3、11、8、2、9、10 的圖有很多合法排序：5,7,3,11,8,2,9,10（由上到下、由左到右）；3,5,7,8,11,2,9,10（先選編號最小的）；5,7,3,8,11,10,9,2（邊最少的先）；7,5,11,3,10,8,9,2（先選編號最大的）；5,7,11,2,3,8,9,10；3,7,8,5,11,10,2,9（任意）。
- **用 DFS 做 topological sort**（Wikipedia）：

```
L ← Empty list that will contain the sorted nodes
while exists nodes without a permanent mark do
    select an unmarked node n
    visit(n)

function visit(node n)
    if n has a permanent mark then return
    if n has a temporary mark then stop   (not a DAG)
    mark n with a temporary mark
    for each node m with an edge from n to m do
        visit(m)
    remove temporary mark from n
    mark n with a permanent mark
    add n to head of L
```

- topological order 也能用來快速算加權 DAG 的 shortest path。
- CLRS 版：呼叫 DFS 算每個頂點的 finishing time f[v]；每個頂點結束時插到 linked list 的最前面；回傳 list。O(V + E)。例：穿衣服——socks 17/18、undershorts 11/16、pants 12/15、shoes 13/14、watch 9/10、shirt 1/8、belt 6/7、tie 2/5、jacket 3/4，依 finishing time 遞減排列。另附 YouTube 影片 7J3GadLzydI（「visualized and explained, easier」）。

**Relaxation**（CLRS）：
```
Initialize-Single-Source(G, s)
1. for each vertex v ∈ V[G]
2.     d[v] ← ∞;      /* shortest path 從 s 到 v 的權重上界 */
3.     π[v] ← NIL;    /* v 的 predecessor */
4. d[s] ← 0;

Relax(u, v, w)
1. if d[v] > d[u] + w(u, v)
2.     d[v] ← d[u] + w(u, v);
3.     π[v] ← u;
```
- 呼叫 Relax(u, v, w) 之後 d[v] ≤ d[u] + w(u, v)。
- relaxation 過程中 d[v] ≥ δ(s, v)；一旦 d[v] 達到下界 δ(s, v) 就不再變。
- 若 s ⇝ u → v 是最短路徑，且呼叫前 d[u] = δ(s, u)，則呼叫後 d[v] = δ(s, v)。
- 例：u = 5、邊權 2：v = 9 → 7（更新）；v = 6 → 6（不變）。

**Summary：longest and shortest paths**（重複 §8 的整理）。

---

## 14. 其他考量：jog、hierarchy、constraint generation

[1:33:45]–[1:38:18]　投影片 Remarks on Constraint-Graph Compaction、Constraint Generation

- **Noncritical layout elements**：不在 critical path 上的元素有自由選位置 → 可以用這個自由度最佳化某個 cost function。
  - 【口頭】longest path 上的元素是 critical 的；不在上面的 shape 就有空間可以移動。
- **Automatic jog insertion**：自動在線上加 jog（轉彎）可以再改善 layout 品質。
  - 【口頭】把線打個彎，這一塊就能往左推、另一塊就能進來。
- **Hierarchy**：降低 complexity 的方法——hierarchical compaction，例如只考慮 cell。
- **Constraint generation**：constraint 要 **irredundant** 而且有效率地產生。
  - 若邊 $(v_i, v_k)$、$(v_k, v_j)$ 存在且 $w(v_i, v_j) \le w(v_i, v_k) + w(v_k, v_j)$，則邊 $(v_i, v_j)$ 是**多餘的**。
  - 例：A、B 之間和 B、C 之間的最小距離 constraint，讓 A、C 之間的 constraint 變多餘。
  - Doenhardt 和 Lengauer 提出 irredundant constraint generation 的方法，complexity **O(n log n)**。

---

## 15. 總結與 interactive compaction

[1:38:18]–[1:39:27]　投影片 Summary So Far、Some Layout Results After Compaction、Course Ordering

- 1D compaction（2D compaction 需要 heuristic）
- constraint graph → longest path
- forward edge graph、backward edge graph——用 **Liao-Wong** 解 compaction
- **Bellman-Ford**：兩種邊一起考慮
- **Interactive compaction**：但**維持連接（connectivity）不容易**。圖：一條線從上面往左「推」過 block。
  - 【口頭】也可以反過來，不是壓緊而是把東西推開。Fig. 18 是「layout compacted by using interactive compaction router」——找到空的地方一道道切，同時連接必須保持住，這是當時的挑戰。
- 課程順序：compaction 之後下一個是 **partitioning**（之後還有 floorplanning、placement…）。

---

## 16. 【實測】

程式：`experiments/lecture07/longest_path.py`，輸出：`experiments/lecture07/output.txt`。用投影片的圖（forward edge + 三條 backward edge）跑 DAG longest path、Liao-Wong、Bellman-Ford，再故意做一個 positive cycle。

```
DAG longest path (x1..x5), one row per vertex popped from Q:
  pop v0:    1    5    0    0    0
  pop v1:    1    5    0    0    3
  pop v2:    1    5    6    6    3
  pop v5:    1    5    6    7    3
  pop v3:    1    5    6    7    3
  pop v4:    1    5    6    7    3

Liao-Wong (all backward edges each round):
  forward  :    1    5    6    7    3
  backward :    2    5    6    7    3
  forward  :    2    5    6    8    4
  backward :    2    5    7    8    4
  forward  :    2    5    7    8    4
  backward :    2    5    7    8    4

Bellman-Ford longest path, wave fronts S1:
  S1=[0]:    1    5 -inf -inf -inf
  S1=[1, 2]:    2    5    6    6    3
  S1=[1, 3, 4, 5]:    2    5    6    7    4
  S1=[4, 5]:    2    5    6    8    4
  S1=[4]:    2    5    7    8    4
  S1=[3]:    2    5    7    8    4

Make x2-x1 <= 1 (backward v2->v1 weight -1) -> conflicts with x2-x1 >= 2 via v1->v2:
  Liao-Wong : positive cycle
  Bellman-Ford: positive cycle
```

- 三張表和投影片**逐格一致**（DAG longest path 的 x 列、Liao-Wong 的 6 步、Bellman-Ford 的 6 個 wave front）。
- 把 x₂ − x₁ 的上限改成 1，但 v₁→v₂ 要求 x₂ − x₁ ≥ 2 → cycle v₁→v₂→v₁ 權重 2 − 1 = +1 > 0，兩個演算法都報 positive cycle（這種 layout 的 constraint 互相矛盾、不可能滿足）。

---

## 17. 一頁速查表

| 主題 | 重點 |
|---|---|
| Design rule | min width、min separation、min overlap；DRC：width、spacing、enclosure；常用 λ |
| Symbolic layout | 只有拓樸；配 technology file 產生 geometric layout |
| Compaction 用途 | 縮面積、symbolic → mask、修 DRC、換製程 |
| 1D vs 2D | 1D polynomial；2D NP-hard → 重複 1D |
| Min-distance | x_j − x_i ≥ d_ij → forward edge (v_i, v_j)，權重 d_ij |
| Max-distance | x_j − x_i ≤ c_ij → backward edge (v_j, v_i)，權重 −c_ij |
| Constraint graph | 只有 forward edge 時是 DAG；longest path = layout 寬度 |
| DAG longest path | BFS + in-degree；O(V + E) |
| Positive cycle | longest path 無解（constraint 矛盾） |
| Liao-Wong | 交替：G_f 上 longest path、處理所有 backward edge；O(E_b × E_f)；count > \|E_b\| 還在變 → positive cycle |
| Bellman-Ford | wave front；兩種邊一起；O(VE)；count > n → positive cycle |
| Longest ↔ shortest | 權重 × (−1) |
| Topological sort | DAG 才有；DFS 依 finishing time 反序 |
| 其他 | noncritical 元素的自由度、jog insertion、hierarchy、irredundant constraint（O(n log n)） |

---

## 18. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| design rule、DRC | 第 11–12 支：routing（DRC-clean 的 detailed routing） |
| constraint graph、longest path | 第 9 支：floorplanning（sequence pair／B\*-tree 的 packing 用 constraint graph 算座標） |
| channel routing 的 Y 方向壓縮 | 第 14 支：channel routing（left-edge、vertical constraint graph） |
| building block layout | 第 9 支：floorplanning |
| shortest path（Dijkstra、Bellman-Ford） | 第 11–12 支：maze routing、global routing |
| topological order | timing analysis（[第 6 支](lecture06_note.md) §22 的 arrival time 也要照 topological order 算） |

---

## 附：本講一句話總結

> **因為 design rule 很難手動處理，所以先畫只有拓樸的 symbolic layout，再由 compactor 依 design rule 產生最密的 mask layout（也能修 DRC 或把 layout 換到別家 foundry）；1D compaction 是 polynomial、2D 是 NP-hard，所以通常重複做 1D：最小距離規則 x_j − x_i ≥ d_ij 變成 constraint graph 的 forward edge，graph 是 DAG，用 BFS 加 in-degree 在 O(V+E) 算 longest path 就是 layout 寬度；最大距離規則變成負權重的 backward edge，graph 有了 cycle，可以用 Liao-Wong（交替做 forward 的 longest path 和處理所有 backward edge，O(E_b × E_f)）或 Bellman-Ford（wave front，O(VE)）解，兩者都能偵測代表 constraint 矛盾的 positive cycle（本機實測三張表和投影片逐格一致）；compaction 在 channel routing 和 1988 年左右的 building block layout 工具（先 symbolic routing 再 compaction）真的派上用場，但 leaf cell 通常客製、維持 contact 和 connectivity 都不容易。**

---

*筆記依據李炯霆「Electronic Design Automation (1)」第7講｜ch3.compaction 錄影（NTHU OCW，1:39:30；影片無字幕，以 whisper large-v3-turbo 轉錄）、投影片截圖（Unit 5F）、Wang, Chang, Cheng（eds.）*Electronic Design Automation: Synthesis, Verification, and Test* Ch2 §2.5.1 整理；compaction 演算法部分對應參考書 Gerez *Algorithms for VLSI Design Automation* Ch6（課本無此章）。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
