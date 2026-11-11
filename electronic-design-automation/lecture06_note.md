# 設計自動化（一）第六支影片完整筆記：Logic synthesis——BDD、two-level 最佳化、technology mapping、timing 與 low power（ch10 synthesis）

> **課程**：Electronic Design Automation (1)（清大 CSR 5302）— 第6講｜ch10.synthesis（播放清單第 6 支）
> **講者**：李炯霆（JT Li），清大半導體學院，2026 春，NTHU OCW 錄影
> **來源**：YouTube 錄影（3:40:34，無字幕，whisper 轉錄）＋ 投影片截圖（以黃鐘揚 Chung-Yang (Ric) Huang 2006 年「EDA 導論 Synthesis」投影片為底，改編自張耀文教授投影片，老師再加入江介宏、Kurt Keutzer、Steven Nowick 的教材）＋ 課本 Wang, Chang, Cheng（eds.）*Electronic Design Automation* Ch6 Logic synthesis in a nutshell（p.299–392）
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、投影片和課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿辨識不清、或影片／投影片／課本說法不一致之處
> - `[h:mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Synthesis 的種類與 logic synthesis 流程](#1-synthesis-的種類與-logic-synthesis-流程)
- [2. Domain translation：3-address code 與 functional unit allocation](#2-domain-translation3-address-code-與-functional-unit-allocation)
- [3. Logic optimization：technology-independent vs dependent](#3-logic-optimizationtechnology-independent-vs-dependent)
- [4. Boolean function 的基本名詞](#4-boolean-function-的基本名詞)
- [5. Canonical form 與 BDD](#5-canonical-form-與-bdd)
- [6. 從 truth table 到 ROBDD：三條化簡規則](#6-從-truth-table-到-robdd三條化簡規則)
- [7. 建 BDD 的例子與 variable ordering](#7-建-bdd-的例子與-variable-ordering)
- [8. BDD 的用途、ITE、satisfiability](#8-bdd-的用途itesatisfiability)
- [9. 用 ROBDD 做 two-level 實作：isop](#9-用-robdd-做-two-level-實作isop)
- [10. Two-level logic synthesis 與 set covering](#10-two-level-logic-synthesis-與-set-covering)
- [11. Quine-McCluskey：essential、row／column dominance](#11-quine-mccluskeyessentialrowcolumn-dominance)
- [12. Cyclic core 與 Petrick's method](#12-cyclic-core-與-petricks-method)
- [13. Heuristic：Espresso；multi-level：MIS、ABC](#13-heuristicespressomulti-levelmisabc)
- [14. Technology mapping：subject graph 與 pattern graph](#14-technology-mappingsubject-graph-與-pattern-graph)
- [15. Rule-based mapping](#15-rule-based-mapping)
- [16. DAGON：tree covering 與 dynamic programming](#16-dagontree-covering-與-dynamic-programming)
- [17. Keutzer 的 optimal tree covering 例子](#17-keutzer-的-optimal-tree-covering-例子)
- [18. Partition、Boolean matching、Liberty、min-delay mapping](#18-partitionboolean-matchingliberty-min-delay-mapping)
- [19. FPGA 的 technology mapping](#19-fpga-的-technology-mapping)
- [20. Low power：功耗模型與 low-power mapping](#20-low-power功耗模型與-low-power-mapping)
- [21. Design Compiler 流程](#21-design-compiler-流程)
- [22. Delay model 與 timing analysis](#22-delay-model-與-timing-analysis)
- [23. 總結](#23-總結)
- [24. 【實測】](#24-實測)
- [25. 一頁速查表](#25-一頁速查表)
- [26. 與後續講次的連結](#26-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
HDL ─domain translation─> generic Boolean network ─logic optimization─> optimized network ─technology mapping─> netlist
       (3-address code、MUX)        │                                         │
                                    ├ two-level：K-map、Quine-McCluskey（exact）、Espresso（heuristic）
                                    ├ multi-level：factoring、resubstitution（MIS、ABC）
                                    └ 表示法：truth table、cover、BDD（canonical）
technology mapping：rule-based（local transform）或 DAGON（拆成 tree，DP 求最小 cost cover，O(n)）
其他：FPGA LUT mapping、low-power mapping、Liberty timing model、STA（arrival／required／slack）
```

【口頭】這一講用黃鐘揚（Ric Huang）老師的投影片當底，加入江介宏（Roland Jiang）、Kurt Keutzer、Steven Nowick 的教材。目前商用 synthesis 工具以 Synopsys Design Compiler 和 Cadence 的工具為主。除了參考書，Hachtel & Somenzi 的 *Logic Synthesis and Verification Algorithms* 也很有名，這講用到它的 Ch11、12。

---

## 1. Synthesis 的種類與 logic synthesis 流程

[0:00]–[6:30]　投影片 Outline、Y Chart、Type of Synthesis、Logic Synthesis & Verification；課本 §6.1（p.299–）

- 內容：logic synthesis 基礎、Boolean logic、logic optimization、technology mapping、timing & power optimization、high-level synthesis 基礎（data flow、scheduling／allocation／assignment）。
- 流程：original function → **logic optimization** → optimized function → **technology mapping** → optimized circuit。

**Synthesis 的種類**（Y chart 上從 behavioral 走到 structural）：

| Synthesis | Behavioral | → Structural |
|---|---|---|
| System synthesis | Systems | Processors |
| Register-transfer synthesis | Algorithms／register transfers | ALUs、RAM |
| **Logic synthesis** | Logic | Gates、flip-flops |
| Circuit synthesis | Transfer functions | Transistors |

**Logic synthesis**：把 Boolean expression 或 RTL 描述（Verilog/VHDL/C）轉成某個 library 的 logic gate network（netlist）。
- 三個任務：two-level combinational synthesis、multilevel combinational synthesis、sequential synthesis。
- 最佳化目標：面積、delay、power 最小。
- **Verification**：檢查 specification 和 implementation 是否等價。

---

## 2. Domain translation：3-address code 與 functional unit allocation

[6:30]–[14:51]　投影片 HDL Synthesis、Typical Domain Translation Flow、Functional Unit Allocation、Combinational Circuit Generation、Interconnection Binding、Synthesize Logic to Implement equations

- HDL synthesis = domain translation + optimization（[第 2 支](lecture02_note.md) §14）。
- **典型的 domain translation 流程**：HDL → **3-address code** → special element inferences（例如推出 flip-flop、latch）→ combinational circuit generation → initial structure netlist。逐個 process 做（local view）。
- **3-address code**：一般形式 `x = y op z`——一個 functional unit `op`，輸入 y、z，輸出 x。

```
x = c + d + e;          t = c + d;
if (a == b) x = e - f;  x = t + e;
y = x;           ──>    s = (a == b);
                        if (s) x = e - f;
                        y = x;
```
→ 兩個加法器、一個比較器、一個減法器，加一個 **implicit multiplexer**（s = 0 選 t + e，s = 1 選 e − f）。
- **Combinational circuit generation**：
  - functional unit allocation：照 3-address code 直接對應。
  - **interconnection binding**：需要 functional unit 之間的相依資訊 → 用 **control/data flow analysis**（compiler 常用的技術，靜態計算到達程式某一點的 assignment 集合）。
- Technology mapping 問題（SRST 的例子，同[第 2 支](lecture02_note.md) §15）。
- 一個 block = flip-flop ＋ combinational logic；combinational 的部分用 Boolean equation 描述，就是 logic synthesis 要處理的對象。

---

## 3. Logic optimization：technology-independent vs dependent

[14:51]–[19:12]　投影片 Logic Optimization、Two-Level Logic Optimization、Multi-Level Logic Optimization；課本 §6.3（p.332–）

| | Technology-independent | Technology-dependent（technology mapping／library binding） |
|---|---|---|
| 做什麼 | 在 Boolean expression 層級做等價轉換 | 把 Boolean expression map 到某個 cell library |
| 估大小 | literal 數 | cell 面積 |
| 技巧 | don't-care、共同因子抽取（factorization） | 除了單純 mapping，也可能順便最佳化 |
| Delay model | 簡單 | 依 cell 結構、比較準 |

**Two-level**：
- 表示法：sum-of-products（SOP）、product-of-sums（POS）。
- 是 logic optimization 的關鍵技術；有很多有效率的演算法能在實務時間內找到近乎最小的表示；商用多年。
- 最小化準則：**product term 的數目**。
- 例：$F = XYZ + X\bar Y\bar Z + X\bar YZ + \bar XYZ + XY\bar Z \Rightarrow F = X\bar Y + YZ$。⚠️ 投影片上的原式 5 項有一項看不清楚，化簡結果 $X\bar Y + YZ$ 以投影片為準。

**Multi-level**：把 combinational circuit 轉成符合效能或面積限制的形式。
- two-level minimization、共同因子或 **kernel extraction**、共同運算式的 **resubstitution**。商用多年。
- 例：

$$\begin{aligned} f_1 &= abcd + abce + a\bar b\bar c\bar d + \bar a\bar bcd + \bar ac + cdf + abc\bar d\bar e + a\bar b\bar cd\bar f \\ f_2 &= bdg + \bar bdfg + \bar b\bar dg + b\bar deg \end{aligned} \Rightarrow \begin{aligned} f_1 &= c(\bar a + x) + a\bar c\bar x \\ f_2 &= gx \\ x &= d(b+f) + \bar d(\bar b + e) \end{aligned}$$

（抽出共同的 x，兩個輸出都用它。）

---

## 4. Boolean function 的基本名詞

[19:12]–[29:03]　投影片 Boolean Functions、Boolean algebra、Minterms and Cubes、Implicant and Cover、Cover Examples；課本 §6.2

- B = {0, 1}，Y = {0, 1, D}（D = don't care）。Boolean function $f: B^m \to Y^n$。
- 例（SOP）：$f = \bar x_1\bar x_2 + \bar x_1\bar x_3 + \bar x_2x_3 + x_1x_2 + x_2\bar x_3 + x_1x_3$。
- 輸出值把 $B^m$ 分成三個集合：**on-set**、**off-set**、**dc-set**（don't-care set）。
- Boolean algebra 恆等式：$\overline{ab} = \bar a + \bar b$、$\overline{a+b} = \bar a\bar b$、$(a+b)c = ac + bc$、$ab + c = (a+c)(b+c)$、$aa = a$、$a + a = a$、$a\bar a = 0$、$a + \bar a = 1$、$a\cdot 0 = 0$、$a + 0 = a$、$a\cdot 1 = a$、$a + 1 = 1$。

**Minterm 與 cube**：
- **Minterm**：**所有**輸入變數（或其反相）的乘積——對應 $B^m$ 裡的一個點。例：$x_1\bar x_2x_3$ 就是 101 這一點。
- **Cube**：輸入變數（或其反相）的乘積——變數越少，涵蓋的空間越大。圖：3 維立方體上，$x_1\bar x_2x_3$ 是一個點、$x_1x_3$ 是一條邊、$x_3$ 是一個面。

**Implicant 與 cover**：
- **Implicant**：所有點都在 on-set 或 dc-set 的 cube。
- **Prime implicant**：不被任何其他 implicant 包含的 implicant。
- **Prime cover**：一組 prime implicant，一起涵蓋 on-set 所有點（以及部分或全部 dc-set）。
- **Irredundant**：拿掉任何一個 prime implicant 都不行。
- **Minimal**：prime implicant 數最少的 irredundant prime cover。

**Cover 例子**：同一個 on-set 有兩個不同的 irredundant cover——$f = \bar x_1\bar x_3 + \bar x_2x_3 + x_1x_2$ 和 $f = \bar x_1\bar x_2 + x_2\bar x_3 + x_1x_3$。**兩個式子是同一個函數。**

---

## 5. Canonical form 與 BDD

[29:03]–[37:00]　投影片 Canonical Forms、Binary Decision Diagram、BDD Principles、Shannon Expansion、BDDs for typical functions；課本 §6.2

- **Canonical form**：Boolean function 的**唯一**表示——可以拿來做驗證。
  - truth table、sum of minterms 是 canonical，但隨輸入數**指數成長**。
  - irredundant prime cover **不是** canonical（上一節的例子）。
  - **ROBDD**（reduced ordered binary decision diagram）是 canonical，而且實用。
- **BDD**：R. E. Bryant 1986 年提出（"Graph-Based Algorithms for Boolean Function Manipulation", IEEE TC, Vol. C-35, Aug. 1986）。
  - 用 **DAG** 表示 $f: B^m \to B^n$。
  - 每個非終端節點是一個 decision node，對應一個輸入變數，有 **0-branch** 和 **1-branch**。
  - 兩個終端節點：0-terminal、1-terminal。
  - 例：$f = x_1x_2$（AND）：x₁ 的 0 邊到 0；1 邊到 x₂；x₂ 的 0 邊到 0、1 邊到 1。
- **Cofactor**（restriction）：
  - positive cofactor $f_{x_i} = f(x_1, \dots, x_{i-1}, 1, x_{i+1}, \dots, x_m)$
  - negative cofactor $f_{\bar x_i} = f(x_1, \dots, 0, \dots, x_m)$
- **Shannon expansion**（Boole 已經知道）：

$$f = x_i\cdot f_{x_i} + \bar x_i\cdot f_{\bar x_i}$$

  多變數：$F(x,y,z,w) = xyF_{xy} + \bar xyF_{\bar xy} + x\bar yF_{x\bar y} + \bar x\bar yF_{\bar x\bar y}$。
- 對所有變數連續做 Shannon expansion，直到得到常數 0 或 1，就是 complete expansion。
- 常見函數的 BDD（Hachtel Fig. 6.5）：F = a、a'、ab、a + b、ab' + a'b（XOR）等。

---

## 6. 從 truth table 到 ROBDD：三條化簡規則

[37:00]–[50:27]　投影片 Reduced Ordered BDDs、BDDs from Truth Tables、Example OBDD、Reduction、Reduction Example、Hachtel §6.1

- **ROBDD**：Bryant 1980 年代中期提出，1986 年 IEEE TC 那篇是 EECS 被引用最多的論文之一。應用：logic synthesis、verification、program analysis、AI planning…。Lee（1959）和 Akers（1978）也提過 BDD，但不是 **RO**BDD。有很多變形；和 coding theory 的 trellis 有關。
  - 【口頭】老師研究時發現 Berkeley 研究所的課也在教，投影片附了 Seshia 的 46 頁 BDD 講義連結。
- **流程**：truth table → binary decision tree → BDD → **OBDD**（ordered：每條路徑上的變數順序固定）→ **ROBDD**（簡稱 BDD）。
- **Example OBDD**：complete Shannon expansion 可以畫成一棵 tree（實線 = positive cofactor、虛線 = negative cofactor）。**給定 truth table（或 logic）和 variable ordering，先建 tree，再化簡。**
  - 例：葉子 1 1 1 0 0 1 1 1，$f = \bar x_1\bar x_2\bar x_3 + \bar x_1\bar x_2x_3 + \bar x_1x_2\bar x_3 + x_1\bar x_2x_3 + x_1x_2\bar x_3 + x_1x_2x_3$。

**化簡：找出冗餘**——三條規則：
1. **Merge equivalent leaves**：值相同的葉子合成一個。
2. **Merge isomorphic nodes**：變數相同、0 邊和 1 邊指向同樣子節點的兩個節點合成一個（把指向其中一個的邊改指另一個）。
3. **Eliminate redundant tests**：0 邊和 1 邊指向同一個子節點的節點可以刪掉（直接把邊接到那個子節點）。

**核心運算子只要兩個**：
1. **Restrict(F, v, k)**：F 對 v = k 的 Shannon cofactor。
2. **ITE(I, T, E)**：「if-then-else」運算子。

（Hachtel 的 OBDD → ROBDD 演算法：先把值相同的葉子換成一個；由下往上處理，若 u、v 變數相同、high 子節點相同、low 子節點相同，刪 v 並把邊改指 u；若某個 v 的 high = low，刪 v 並把邊改指它的子節點。）

**BDD 的兩個重要性質**（Hachtel §6.1）：
- **Canonical**：建兩個電路的 BDD，**兩電路等價 iff 兩個 BDD 相同**——這帶來電路驗證、測試、equivalence checking 的突破。
- 表示組合上很大的集合時非常有效。
- 圖 6.1：一個 MUX 電路和它的 BDD。
- 【口頭】老師講 BDD 時用了很多 mux 的概念：每個 BDD 節點就像一個 2-to-1 mux，變數是 select。

---

## 7. 建 BDD 的例子與 variable ordering

[50:27]–[55:03]　投影片 How to Build The BDD? An Example（Hachtel p.249 §6.1.3）、Fig. 6.8–6.12、Variable ordering matters、Some BDD packages

例：$f = abc + \bar bd + \bar cd$，variable ordering $b < c < d < a$。
- 對 b 取 cofactor：$f_b = ac + \bar cd$、$f_{\bar b} = d + \bar cd = d$。
- 再對 c：$f_{bc} = a$、$f_{b\bar c} = d$；$f_{\bar bc} = d$、$f_{\bar b\bar c} = d$。
- 圖 6.9 non-reduced BDD → 圖 6.10 找到兩個 isomorphic subgraph → 圖 6.11 合併 → 圖 6.12 刪掉冗餘節點（c 的兩個邊都到同一個 d）→ 圖 6.8 final BDD：只剩 b、c、d、a 四個節點。

**Variable ordering 很重要**：
- 同一個 $f = abc + \bar bd + \bar cd$：order a < b < c < d（Fig. 6.2）比較大；a < d < b < c（Fig. 6.3）更大；b < c < a < d（Fig. 6.4）最佳。
- $f = ab + cd + ef$：order a ≤ b ≤ c ≤ d ≤ e ≤ f 時是一條小鏈；a ≤ c ≤ e ≤ b ≤ d ≤ f 時大很多。
- 【口頭】當然希望 diagram 越簡單越好。【實測】見 [§24](#24-實測)。

**BDD 套件**：
- **CUDD**：Colorado University，Fabio Somenzi 的組（【口頭】就是 Hachtel & Somenzi 那本參考書作者的研究單位）；有 Perl、Python 等前端。
- **BuDDy**：IT University of Copenhagen。
- 投影片附了一個 CUDD tutorial 連結（「worthwhile」）。

---

## 8. BDD 的用途、ITE、satisfiability

[55:03]–[1:11:12]　投影片 Odd Parity Function、Reduction rules、Final ROBDD、What can BDDs be used for?、(RO)BDDs are canonical、Sensitivity to Ordering、Constructing BDDs in Practice、ITE、ROBDDs and Satisfiability、Project、Applications to Combinatorial Optimization

**Odd parity function** $a \oplus b \oplus c \oplus d$：
- binary decision tree 有 16 個葉子（0 1 1 0 1 0 0 1…）；從 root 到葉子的邊標籤就是一組 a、b、c、d 的 assignment。
- 套三條規則（手寫投影片示範），最後的 ROBDD：每層兩個節點（a；b, b；c, c；d, d），**2n − 1 個非終端節點**——線性大小。

**BDD 可以用來**：
- 唯一表示一個 Boolean function（Boolean function 也能表示集合）
- combinational（或 sequential）電路的 symbolic simulation
- **equivalence checking 與 verification**；SAT solving
- 找出／數出 SAT 問題的**所有**解
- 對「quantified」Boolean formula 做運算

**定理（Bryant）**：若 G、G' 是 k 輸入函數 f 在**同一個 variable ordering** 下的 ROBDD，則 G 和 G' 完全相同。

**對 ordering 的敏感度**：n 輸入的函數，某個 ordering 可能要指數個節點，另一個只要線性。例 $f = x_1x_2 + x_3x_4 + x_5x_6$：$x_1 < x_2 < \dots < x_6$ 很小；$x_1 < x_4 < x_5 < x_2 < x_3 < x_6$ 很大。
- 投影片註：**找讓 ROBDD 最小的 ordering 是 intractable 的問題（精確地說是 co-NP-complete）**。
- ⚠️ 一般文獻說的是「決定某個 ordering 是否最佳」為 NP-hard／co-NP-complete 一類的難題；投影片的說法照錄。

**實作上建 BDD**：定義少數核心運算子（Restrict、ITE），其他運算都寫成 wrapper（例如 AND(a, b) = ITE(a, b, 0)）。
- **ITE** = 程式語言的 if-then-else = 硬體的 **MUX**：$ITE(I, T, E) = I\cdot T + \bar I\cdot E$。
- 怎麼實作 ITE？**Divide and conquer**：對 top variable 做 Shannon cofactor，遞迴算兩邊。

**ROBDD 與 satisfiability**：
- Boolean function satisfiable = 存在讓它為 1 的 assignment。**ROBDD 不等於 '0' 的函數都是 satisfiable。**
- 若 $x_i = 1$ 的 cost 是 $c_i$，**minimum-cost satisfiability** 要最小化 $\sum c_i\mu(x_i)$（$x_i = 1$ 時 $\mu = 1$）。
- 這等於**在 ROBDD 上找最短路徑**（1 邊權重 $c_i$、0 邊權重 0），**線性時間**可解。

**Project**：把 **CUDD** bring up 起來，用幾個 Boolean equation 玩一玩。
- 【口頭】到 GitHub 下載 CUDD、build 起來，給一些 Boolean equation，讓 CUDD 畫出 BDD；也可以換不同的 variable order 看看 BDD 怎麼變。這對應 Prog-1（[第 1 支](lecture01_note.md) §13）。Homework 也會練 BDD。

**組合最佳化的應用**：**0-1 ILP 可以寫成 minimum-cost satisfiability**。
- 例：限制式 $x_1 + x_2 + x_3 + x_4 = 3$ 可以寫成
$$(x_1+x_2)(x_1+x_3)(x_1+x_4)(x_2+x_3)(x_2+x_4)(x_3+x_4)(\bar x_1+\bar x_2+\bar x_3+\bar x_4)$$
  前 6 個 sum：至少 3 個變數是 1（任兩個不能同時為 0）；最後一個：至少一個是 0。
- 很多組合最佳化問題都能直接寫成 SAT 問題。

---

## 9. 用 ROBDD 做 two-level 實作：isop

[1:11:12]–[1:15:12]　投影片 Use of ROBDD for Two-Level Logic Implementation、A Heuristic Based on ROBDDs、Hachtel Ch11 目錄

- ROBDD 是精簡又 canonical 的表示——**能不能直接拿 BDD 做 two-level 實作？**
- 例：BDD（x₁；x₂, x₂；x₃, x₃）列舉通往 1 的路徑得到 cube：$\bar x_1\bar x_2 + \bar x_1x_2\bar x_3 + x_1\bar x_2x_3 + x_1x_2$——目前 4 個 product term，**能不能更好？**（找 minimal cover）
- **isop（Irredundant Sum of Products）**，Minato 提出：
  - 注意：BDD 裡的 cube 是 irredundant，但**不是 prime implicant**。
  - logic optimization 要的是 **irredundant prime cover**。
  - 遞迴：$f = \bar x f_0 + x f_1 + f_d$，其中 $f_0 \subset f_{\bar x}$、$f_1 \subset f_x$、$f_d \subset (f_{\bar x} \cap f_x)$，且 f₀、f₁、f_d 都是 irredundant prime cover。
  - 可以擴充到 incompletely specified function。
  - 細節不講，請看 Hachtel §11.3.2。

---

## 10. Two-level logic synthesis 與 set covering

[1:15:12]–[1:24:24]　投影片 Logic Synthesis in Practice、Two-Level Logic Synthesis、Two-Level Logic Optimization、Cover Examples、Optimality、Set Covering、Covering Matrix、Simplification Rules；課本 §6.3

**實務上的 logic synthesis**：
1. 用 HDL 寫 logic-level 的行為描述。
2. 從描述抽出 Boolean expression，存成適當的內部形式。
3. 操作這些式子，得到最佳化的（two-level 或 multilevel）表示。
4. 做 **technology mapping**：從抽象表示 map 到 library cell 的 netlist。

**Two-level logic synthesis**：
- 任何 Boolean function 都能用兩層實現：AND-OR（SOP）、NAND-NAND 等。
- 用 PLA 直接實作 two-level logic 已不像 nMOS 時代那麼流行。
- 經典問題：**Karnaugh map**（1953）、**Quine-McCluskey** 演算法（1956）。基本想法：$x + \bar x = 1$ 讓我們能合併 $x_1x_2 + x_1\bar x_2 = x_1$。
- 常用 cost：SOP 式子的 literal 數。**目標：找 minimal irredundant prime cover。**
- 例：$F = \bar x_1\bar x_2\bar x_3 + \bar x_1\bar x_2x_3 + x_1\bar x_2\bar x_3 + x_1\bar x_2x_3 + x_1x_2\bar x_3 \Rightarrow F = \bar x_2 + x_1\bar x_3$。【實測】驗證，見 [§24](#24-實測)。
- 方法：**Quine-McCluskey**（exponential time 的 exact 演算法）、**Espresso**（heuristic）。

**Optimality**：兩個 irredundant prime cover，一個是 local minimum、一個是 global minimum——少一個 cube 的那個 cost 是 6（3 個各有 2 個變數的 product term）。**irredundant prime cover 不一定是全域最佳。**

**Set covering**（一個組合最佳化問題）：
- 給集合 S = {s₁, …, sₘ} 和 K = {K₁, …, Kₙ}，每個 Kⱼ ⊆ S，找 K 的子集 Γ 使 Γ 的**聯集涵蓋 S**。
- cover 的 cost = Γ 裡各 Kⱼ 的 cost cⱼ 之和；例如 cⱼ = 1 或 cⱼ = |Kⱼ|。
- 對大部分 cost function 是 **NP-complete**。

**Covering matrix**（Hachtel p.221）：

| | K₁ | K₂ | K₃ | K₄ | K₅ | K₆ |
|---|---|---|---|---|---|---|
| s₁ | 1 | 1 | 0 | 0 | 0 | 1 |
| s₂ | 1 | 0 | 0 | 1 | 1 | 1 |
| s₃ | 0 | 1 | 1 | 0 | 1 | 0 |
| s₄ | 0 | 1 | 0 | 1 | 0 | 1 |

- cⱼ = |Kⱼ| 時，Γ = {K₃, K₆} 是最佳解。Γ = {K₁, K₂, K₃} 也是解，但 K₃ 是多餘的。
- covering 問題可以寫成 satisfiability：$(x_1+x_2+x_6)(x_1+x_4+x_5+x_6)(x_2+x_3+x_5)(x_2+x_4+x_6)$（product of sums）。
- 這種 covering 叫 **unate**；允許反相變數的叫 **binate** covering。

**化簡規則**（矩陣例子）：**K₃ is essential**（某一列只有 K₃ 能蓋）；**s₂ dominates s₃**；**K₅ dominates K₄**。

---

## 11. Quine-McCluskey：essential、row／column dominance

[1:24:24]–[1:52:27]　投影片 The Quine-McCluskey Algorithm、QM Exercises、Nowick handout 的例子

**Quine-McCluskey 演算法**：
1. 算出（on-set ∪ dc-set 的）**所有** prime implicant。
2. 用 prime implicant 找 on-set 所有 minterm 的 **minimal cover**。
3. 建 **covering matrix**。
4. 用 **essential** column、**row 與 column dominance** 化簡矩陣。
5. 剩下的是矩陣的 **cyclic core**——再用 branch-and-bound 解。
- 其他方法不先列舉所有 prime implicant，而是用 ROBDD 做 **implicit representation**。

**練習資源**：Columbia CS6861 的 Quine-McCluskey handout（Nowick，**「worthwhile, this one is better, we are using this」**）、tutorialspoint、atozmath、王俊堯教授的 YouTube 影片、中文 Wikipedia。

**Column dominance**（Nowick handout 的 K-map 例子）：
- 先找 **essential**：找只有一個 X 的那一列（畫圓圈），那一欄的 prime implicant 就一定要選。
- column dominance：某一欄涵蓋的列是另一欄的超集合，被支配的欄可以劃掉。

**Row dominance**：prime implicant A'B'(1,2,3)、C'D(1,5)、A'D(1,3,5,7)、A'C(2,3,7)，**沒有 essential prime implicant**。
- 任何包含 row 2 的 prime implicant 也包含 row 3 → **row 3 dominates**。
- **和 column dominance 相反：劃掉 dominating（比較大的）那一列**——只要蓋到 row 2，row 3 一定會被蓋到。
- 劃掉 row 3 之後，row 1 dominates row 5。

**完整例子 1**：$f(A,B,C,D) = \sum m(0, 2, 5, 6, 7, 8, 10, 12, 13, 14, 15)$。
- Column II（兩兩合併）：(0,2) 00-0、(0,8) -000、(2,6) 0-10、(2,10) -010、(8,10) 10-0、(8,12) 1-00、(5,7) 01-1、(5,13) -101、(6,7) 011-、(6,14) -110、(10,14) 1-10、(12,13) 110-、(12,14) 11-0、(7,15) -111、(13,15) 11-1、(14,15) 111-。
- Column III（四個合併）：(0,2,8,10) -0-0、(2,6,10,14) --10、(5,7,13,15) -1-1、(6,7,14,15) -11-、(8,10,12,14) 1--0、(12,13,14,15) 11--。
- Prime implicant：B'D'、CD'、BD、BC、AD'、AB。
- **B'D'、BD 是 essential**（distinguished row 0 和 5）；劃掉它們蓋到的列。
- 接著 column dominance：CD' 和 BC 互相 co-dominate（被支配的欄劃掉）；row dominance…；得到 **secondary essential** prime implicant CD'(\*\*)、AD'(\*\*)。
- **最小 cost 解 = primary 和 secondary essential prime implicant**：$F = B'D' + BD + CD' + AD'$。

**例子 2、3**（歡迎自己做）：
- #2：$F = \sum m(0,2,3,4,5,6,7,8,9,10,11,12,13)$。
- #3（don't care，Roth pp.163–4）：$F = \sum m(2,3,7,9,11,13) + \sum d(1,10,15)$。
- 想知道這類演算法怎麼實作？GitHub 上有一些程式。

**例子 2 的過程**：
- Prime implicant：A'D'、B'D'、C'D'、A'C、B'C、A'B、BC'、AB'、AC'。
- **沒有 primary essential**（沒有只被一個 prime implicant 蓋到的列，每個 minterm 至少被 2 個蓋到），也沒有 column dominance。
- Row dominance：row 2 dominates…、row 4 dominates…、6 dominates 7、8 dominates 9、10 dominates 11、12 dominates 13 → 刪掉 dominating 的列。
- 之後 B'D'、C'D' 被劃掉；**A'D' 成為 secondary essential**（distinguished row 0）。
- 剩下 rows 3、5、7、9、11、13 和 A'C、B'C、A'B、BC'、AB'、AC'：

| | A'C | B'C | A'B | BC' | AB' | AC' |
|---|---|---|---|---|---|---|
| 3 | X | X | | | | |
| 5 | | | X | X | | |
| 7 | X | | X | | | |
| 9 | | | | | X | X |
| 11 | | X | | | X | |
| 13 | | | | X | | X |

- 沒有 row／column dominance 可用 → **cyclic covering problem**，用 (i) **Petrick's method** 或 (ii) branching method 解。

---

## 12. Cyclic core 與 Petrick's method

[1:52:27]–[1:57:09]　投影片 Quick Overview Of Petrick's Method

- p1 = A'C、p2 = B'C、p3 = A'B、p4 = BC'、p5 = AB'、p6 = AC'。
- Boolean 變數 **P1** 為真，表示 prime implicant p1 被選進解——P1 叫 **Boolean proposition**（「我選 p1」）。
- 蓋 row 3 需要 (P1 + P2)；蓋 row 7 需要 (P1 + P3)；…所有需求寫成：

$$P = (P_1+P_2)(P_3+P_4)(P_1+P_3)(P_5+P_6)(P_2+P_5)(P_4+P_6)$$

- 展開成 SOP：$P_1P_4P_5 + P_1P_3P_5P_6 + P_2P_3P_4P_5 + P_2P_3P_5P_6 + P_1P_2P_4P_6 + P_1P_2P_3P_6 + P_2P_3P_4P_6 + P_2P_3P_6$。
- 找 **P 變數最少**的 product term：$P_1P_4P_5$ 和 $P_2P_3P_6$——兩個解 prime implicant 數一樣少，任選一個，再加上 essential 的 A'D'：

$$F = A'D' + A'C + BC' + AB' \quad\text{或}\quad F = A'D' + B'C + A'B + AC'$$

- 【實測】窮舉發現例子 2 一共有 **6 個** 4 項的最小解，投影片的流程（先用 dominance 刪 B'D'、C'D'）找到其中兩個，見 [§24](#24-實測)。

---

## 13. Heuristic：Espresso；multi-level：MIS、ABC

[1:57:09]–[2:03:09]　投影片 Heuristic Optimization、ESPRESSO（江介宏）、So Far We Had Covered、Technology-Independent Logic Optimization、Multi-Level Logic Optimization、ABC

**Heuristic optimization**：
- 產生**所有** prime implicant 不實際——n 變數函數的 prime implicant 數是指數的。
- 找 **exact** minimum cover 是 **NP-hard**。
- heuristic：**避免產生所有 prime implicant**。
- 程序：挑 ON set 的一個 minterm，擴展（expand）到變成 prime implicant；把它放進最後的 cover，刪掉它蓋到的 minterm；重複直到 ON set 都被蓋到。
- **Espresso**：UC Berkeley 開發，是現代 synthesis 工具的核心。GitHub 上有可編譯的 Espresso 重新發行版。

**ESPRESSO 的迴圈**（江介宏投影片）：local minimum → **REDUCE** → **EXPAND** → **IRREDUNDANT** → local minimum，反覆直到不再改善。

**目前講過的**：domain translation 簡介；binary decision tree → BDD（reduced）；CUDD（歡迎多給幾個 logic equation、換 variable order）；two-level optimization（Quine-McCluskey：essential、column dominance、row dominance、Petrick 的 Boolean proposition）。

**Technology-independent 最佳化**：
- Two-level：最小化 product term 數。
- **Multi-level：最小化 literal 數、變數數。** 例：

```
t1 = a + bc;                  t1 = d + e;
t2 = d + e;     logic         t2 = b + h;
t3 = ab + d;  optimization    t3 = a t2 + c;
t4 = t1 t2 + fg;   ──>        t4 = t1 t3 + f g h;
t5 = t4 h + t2 t3;            F = t4';
F = t5';
```
  右邊是最佳化後式子的 subject graph（後面 technology mapping 用）。
- 方法／工具：Quine-McCluskey（exponential exact）、**Espresso**（two-level heuristic）、**MIS**（multi-level heuristic）、Synopsys 等。
- Multi-level：共同因子抽取、共同運算式 resubstitution、最小化 literal 數；商用：Synopsys、MIS。
- **ABC**：A System for Sequential Synthesis and Verification，Berkeley Logic Synthesis and Verification Group。

---

## 14. Technology mapping：subject graph 與 pattern graph

[2:03:09]–[2:14:03]　投影片 Technology Mapping、Pattern Graphs for an Example Library、Trivial Covering、Fig. 6.27、Best Covering、Subject Graph Covering (1)–(3)、DAG Covering；課本 §6.4（p.362–）

**三種 technology mapping**：
- **Library-based**：standard cell 設計——把函數 map 到一組有限的預先設計好的 cell。
- **Lookup table-based**：Altera、Lucent、Xilinx 的 CPLD／FPGA——每個 LUT 能實作非常多函數（例如所有 4 輸入 1 輸出的函數）。
- **Multiplexer-based**：Actel FPGA——logic module 由 mux 組成。

**範例 library 的 pattern graph**（cell 名稱與 cost）：inv(1)、nand2(2)、and2(3)、or2(3)、nand3(3)、nor2(2)、aoi21(3)、aoi22(4)、nand4(4)、nor3(3)、xor(5)、xnor(5)、nor4(4)…——**每個 cell 都用 NAND2 + INV 表示成 pattern graph**。

**Trivial covering**：把電路（**subject graph**）直接 map 成 2 輸入 NAND 和 1 輸入 inverter：8 個 NAND2 + 7 個 INV，面積 cost **23**。最好的 covering 呢？

**Technology mapping 的定義**：從 library 所有 gate 的 pattern graph 中挑選，找 subject graph 的 **minimum cost covering**。
- **cover**：一組 pattern graph，subject graph 的每個節點都至少被其中一個包含。
- 進一步限制：每個 pattern graph 需要的**輸入**，必須是某個其他 pattern graph 的**輸出**。
- Fig. 6.27：(a) legal covering（NAND2、AOI21、AOI21）；(b) **illegal**——某個 pattern 需要的內部訊號沒有被任何 pattern 輸出。

**Best covering**：面積 **15**，用 **dynamic programming** 得到。各節點的選擇：nand3 = min{2+1+2, 3} = 3；or2 = min{4, 3} = 3；nor3 = min{3+2, 3} = 3；再加上 nand2（2）、inv（1）、and2（3）。

**Subject graph covering 三種結果**（t₁ = d + e、t₂ = b + h、t₃ = at₂ + c、t₄ = t₁t₃ + fgh、F = t₄'）：

| Covering | 用的 cell | Total cost |
|---|---|---|
| (1) trivial | NAND2 + INV | 23 |
| (2) | and2(3)、or2(3)、or2(3)、aoi22(4)、nand2(2)、nand2(2)、inv(1) | 18 |
| (3) | nand3(3)、and2(3)、oai21(3)、oai21(3)、nand2(2)、inv(1) | **15** |

- 【口頭】最好的 covering 裡最長的 logic path **經過 4 個 cell**，做 timing analysis 時要了解它。也因為 mapping 要判斷功能等價（例如三輸入 NAND 和 NAND2 + INV + NAND2 的組合），所以要大家了解 CUDD／BDD。

**DAG covering**：
- 輸入：technology-independent 最佳化後的 logic network、一個 gate library（附 cost）。
- 輸出：最小化總 cost 的 gate netlist。
- 一般做法：建 network 的 subject DAG；把每個 library gate 表示成 pattern DAG；用這些 pattern DAG 找 subject DAG 的最佳 cost covering。
- **Complexity：NP-hard**；就算節點 out-degree ≤ 2 也還是 NP-hard；**如果 subject DAG 和 pattern DAG 都是 tree，有有效率的演算法**（dynamic programming）。

---

## 15. Rule-based mapping

[2:14:03]–[2:19:03]　投影片 Rule-Based Technology Mapping

- Technology mapping = 一連串基於 **rule** 的**局部轉換**。例子：
  - INV + AND + INV → 輸入反相的 OR（bubble pushing）
  - NAND + OR 的組合改寫
  - 兩個串接的 inverter → 直接拿掉
  - **慢的訊號**移到靠近輸出的輸入（slow 的 b 換到後面）
  - 兩個並聯的 inverter → 一個 inverter fanout
  - 負載大的 inverter → 換成大尺寸的 inverter
- 主要操作：
  - **Matching**：找出可以套用到目前 network 的 rule
  - **Cost function evaluation**：每個可能的套用都算 cost，判斷結果品質
  - **Selection**：決定套用哪個 rule
  - **Replacement**：套用選定的 rule，轉換 network
  - 例：target（size 11）→ replacement（size 9）；size = 電晶體 pair 數；patterns 存在資料庫。
- **Rule 的順序很重要**：size 11 → 9，或 11 → 9 → 7。
  - 【口頭】先換成 9，後來發現兩個可以再簡化成 7。
- 搜尋策略：state space search；**greedy**：
```
order rules in the knowledge base
do {
    for each rule R {
        for each gate G {
            if rule R matches G
                if cost improves: apply_rule
        }
    }
} while improving
--> local minimum
```

---

## 16. DAGON：tree covering 與 dynamic programming

[2:19:03]–[2:37:36]　投影片 Graph Covering DAGON、Tree Matching、Weak Points、Simplification、Boolean matching、Keutzer 7-1-techmap、Reasonable Library、Problem statement、History、Code generation、Problem Formulation、Step 1–4、Solution formulation、For each tree – DP

- **DAGON**：K. Keutzer, "DAGON: technology binding and local optimization by DAG matching," Proc. 24th DAC, 1987。
  - 給：Boolean network（DAG）、library（每個 cell 是有 cost 的 DAG）。
  - 求：Boolean network 的最小 cost covering——需要 DAG matching（NP-complete）。
  - 【口頭】1980 年代這是很嚴重的問題；Keutzer 當時在 AT&T Bell Labs，1987 年發表 DAGON，是經典之作。他後來去 Synopsys，再到 Berkeley 教書；老師在網路上找到他的投影片，所以這段用他的教材（7-1-techmap.pdf，感謝 S. Devadas）。

**Tree matching**（例 cost(INV) = 2、cost(NAND) = 3、cost(AOI21) = 4）：
- 每個節點算出所有 matching；depth-first 挑最好的。
  - 葉子：一個 NAND 或 NOT 的 cost。
  - 內部節點：這個 matching tree 的 cost + 子樹的 cost。
- 例：ND2(3) → ND2(6) = 3 + 3 → INV(2)、ND2(11) = 3 + 2 + 6 → 在 A 點：
  - 不用 AOI21：11 + 2 = **13**
  - 用 AOI21：3 + 4 = **7** → 選 AOI21(7)
- matching 從 primary input 開始、照 topological order。投影片問：**怎麼知道右邊的電路和左邊相同？**（→ Boolean matching）

**弱點**：
- 把 network 拆成 tree → 只得到 local optimum。
- 用 tree 表示 cell → **XOR 沒辦法用 tree 表示**（XOR 有 reconvergent fanout）。

**簡化**：
- 把 DAG 拆成 forest：**節點 fanout > 1 就在那裡切開**，fanout > 1 的 gate 變成一棵 tree 的 root。拆的 complexity 是線性的。
- library cell 也用 tree 表示。
- **用 canonical form：NAND 和 NOT**。
- tree 對 tree 做 matching；用 **dynamic programming** 找最小 cost match。

**Keutzer 的投影片**：
- **合理的 library**：inverter、buffer；ND2–ND4、NOR2–NOR4、AND2–AND4；AOI21–AOI333、OAI21–OAI333；XOR、XNOR；MUX、full adder；負緣、正緣 D-FF；J-K FF；以上加各種 clear、enable；各自的 scan 版本；大部分有 6 種 power size：1x、2x、4x、6x、8x、16x。
- **問題**：把電路「最佳地」（面積、delay、power）map 到 technology library。
- **歷史 1**：1986 年 technology mapping 是大問題。每個設計團隊（例如 AT&T）都有自己的 library：ASIC 400 個 cell、microprocessor/DSP 200 個 base cell、government 200+ 個 cell。每個團隊有自己的 mapping 方法：「do what you have to do」的手工 mapper；rule-based 系統（例如 GE Socrates）——很慢的 expert system，不保證結果品質。
  - 【口頭】AT&T 也幫 government 做 project。
- **歷史 2**：technology mapping 對面積、速度、功耗影響很大；有 200 多家半導體公司，每家有好幾套 library——怎麼做一個能用各種 library 的工具？
- **類似的問題：compiler 的 code generation**（用 tree covering 產生指令）：能處理複雜指令集 → 能處理複雜 library；容易移植到其他指令集 → 容易移植到其他 library。

**DAGON 的 formulation**：
- 輸入 netlist 用 **normal form** 表示 → **subject DAG**。
- 每個 library gate 的邏輯函數用 normal form 表示 → **primitive DAG**，各有 cost。
- 目標：用 primitive DAG 找 subject DAG 的最小 cost covering。
- **Normal form：2 輸入 NAND 和 inverter。**

**步驟**：
1. **抽出 combinational logic**：flip-flop 不需要和周圍 logic 一起最佳化，先切出去。
2. **Normalize circuit netlist**：化成 ND2（和 INV），得到 subject DAG。
3. **Normalize library**：INVERTER 2、NAND2 3、NAND3 4、NAND4 5（NAND4 有兩種 tree 形式）、AOI21 4、AOI22 5。
4. **DAG covering**：完整的演算法方法是 NP-hard 最佳化問題（Keutzer & Richards, 1989）；遇到 multiple fanout 時，用 **tree covering heuristic**：subject 和 primitive DAG 是 tree 時，有效率的演算法能找到最佳 cover → DP formulation。

**Solution formulation**：
1. 把輸入 netlist 拆成 **trees 的 forest**（在 multi-fanout 處打叉切開）。
2. 用 tree covering **最佳地**解每棵 tree。
3. 把 tree 接回去。

**每棵 tree：dynamic programming**（投影片重用[第 3 支](lecture03_note.md)的 FutureCost 圖）：
- **Principle of optimality**：一棵 tree 的最佳 cover = **root 的最佳 match** ＋ 該 match 每個輸入開始的子樹的**最佳 cover**。
- 例：INV 11 + 2 = 13 vs AOI21 4 + 3 = 7（同上面的 tree matching）。

---

## 17. Keutzer 的 optimal tree covering 例子

[2:38:24]–[2:52:36]　投影片 DAG covering in detail、Normalize netlist、Generate candidate matches、Optimal tree covering 1–7、Cover 1–3、Optimal Cover was Cover 2、Computational Complexity

**DAG covering 細節**：
1. 把 DAG 拆成 forest of trees
2. normalize netlist（化成 2 輸入 NAND 和 INV；**每棵 tree 做成 left-oriented**）
3. 最佳地 cover 每棵 tree：(a) **產生所有 candidate match**；(b) 用 DP 找最佳 match

**產生 candidate match**：結束時 subject tree 的每個 gate 都標上所有能以它為 root 的 library cell。方法：
- Naive：每個 cell 和每個節點都試（library 可能很大——小心大常數！）
- Better：建表，只檢查可能的 candidate
- Best：花俏的字串比對（CLRS pp.862–869）
- 投影片問：每種方法的 complexity？

**一步步算**（cost：INV 2、NAND2 3、NAND3 4、NAND4 5、AOI21 4）：

| 節點 | 選項 | Cost | 選擇 |
|---|---|---|---|
| 葉子 | NAND2、INV、INV、NAND2 | 3、2、2、3 | |
| 第二層 NAND2 | NAND2 + 子樹 | 8 | 8 |
| 下方 NAND（ND2 or ND3?） | 1 NAND2 3 + 子樹 5 = 8；1 NAND3 = 4 | | **4** |
| 上方 NAND | | 13 | 13 |
| INV（INV or AO21?） | 1 INV 2 + 子樹 13 = 15；1 AO21 4 + 子樹 3 + 子樹 2 = 9 | | **9** |
| NAND（ND2 or ND3?） | NAND2：9 + 4 + 3 = 16；NAND3：8 + 2 + 4 + 4 = 18 | | **16** |
| INV（INV or AOI21?） | INV：16 + 2 = 18；AOI21：13 + 5 + 4 = 22 | | **18** |
| Root（ND2／ND3／ND4?） | Cover 1 NAND2：18 + 0 + 3 = 21；Cover 2 NAND3：9 + 4 + 0 + 4 = **17**；Cover 3 NAND4：8 + 2 + 4 + 0 + 5 = 19 | | **17** |

- **最佳是 Cover 2**：INV 2 + ND2 3 + 2 個 ND3 8 + AOI21 4 = **17**。
- 投影片：**很明顯 greedy 不太行**（每一步選當下最小，root 前一步選了 18，但最佳解沒有用那個選擇）。
- **Complexity**：每個節點只需要考慮 root 的最佳 match（**對 match 的 cell 數是常數時間**）加上每個輸入子樹的最佳 cover（**對 fan-in 是常數時間**）→ **O(n)——amazing!**

---

## 18. Partition、Boolean matching、Liberty、min-delay mapping

[2:52:36]–[3:03:39]　投影片 Tree Covering、Tree-Covering by DP (DAGON)、Tree Covering Example、Partition Affects Results、Boolean matching、How Logic Functions Described For Pattern Graphs、Min-Delay Technology Mapping、Timing Models、Synopsys characterization

**Tree covering 演算法**（江介宏投影片）：
- 對 subject tree 的每個節點 N：**遞迴假設**——N 的所有子節點的最佳 cost match 都已知。
- 算每個在 N 匹配的 pattern tree 的 cost = pattern 每個輸入的最佳 cost 總和 + pattern 本身的 cost（葉子 cost 為 0）。
- 選 cost 最小的 pattern 實作 N。
- **subject DAG 不是 tree 時**：拆成 forest、各自用 DP 最佳地 cover——**整體只是近似**。
- **Optimality**：最佳決策序列的性質——不管初始狀態和決策是什麼，剩下的決策對第一個決策造成的狀態也必須是最佳的（principle of optimality）。tree T 的最小面積 cover 可以由 T 的 root 以下每個節點的最小面積 cover 推出。
- **Tree covering 例子**（library：nand2 = 3、inv = 2、nand3 = 4、nand4 = 5、and2 = 4、aoi21 = 4、oai21 = 4）：每個節點標上候選 cell 和累計 cost，例如 nand2(3)、inv(2)、nand2(8)、nand2(13)、inv(5)/and2(4)、nand2(7)/nand3(4)、inv(6)/and2(8)、nand2(22)/nand3(21)/nand4(18)、inv(20)/aoi21(18)，root 是 nand2(21)/nand3(23)/nand4(22)——投影片留作練習。

**Partition 影響結果**：
- 在所有 multiple-fanout 點切開：沒有重複或重疊，但有時會得到很多小 tree（例子切成 3 棵）。
- **Single-cone partition**：從一個輸出往回追到 primary input 形成一棵 tree；下一個輸出 map 到碰到前面輸出的 match 為止。會**重複一些 logic**（tree 重疊），但 tree 大很多，可能更好。

**Boolean matching**：判斷兩個 Boolean function 在**輸入、輸出的排列與反相**下是否功能等價。
- 江介宏老師的說明：BDD 和／或 SAT 常用來檢查電路等價，前提是**兩個電路的輸入對應已知**；technology mapping 一般**不知道輸入對應**，所以要做 Boolean matching，測試兩個電路在輸入排列下等價（**P-equivalence**）、輸入反相加排列下等價（**NP-equivalence**）、輸入反相、排列加輸出反相下等價（**NPN-equivalence**）。BDD 和 SAT 都能用，但需要多很多次比較。

**Pattern graph 的邏輯函數怎麼描述**：從 **Liberty** timing reference——每個 output pin 有 `function` 屬性：

```
pin(Y) {
    direction : output;
    function : "(A * B)'";
    timing() {...}
}
function : "A S + B S'";
function : "A & S | B & !S";
function : "(A * S) + (B * S')";
```

**Min-delay technology mapping**：
- Tree：和 min-area covering 一樣，只是在 DP 裡用最佳 delay 值。
- DAG：**如果 delay 和 fanout 數無關**，用 tree 的 DP 就能在 polynomial time 得到最佳解（假設可以 replicate logic）。
- 合併目標：例如 delay 為第一準則、面積第二；配合 STA 專注在 critical path。

**Timing model**：
- composite current source（CCS）、effective current source model（ECSM）、**non-linear delay model（NLDM）**。
- corners（TypicalT、SlowS、FastF）；on-chip variation（OCV）；Liberty variation format（LVF）。
- Liberty 的表格模板：`lu_table_template`，`variable_1 : input_net_transition`、`variable_2 : total_output_net_capacitance`；pin 的 `timing()` 裡有 `cell_rise`、`rise_transition`、`cell_fall`、`fall_transition` 等表格。

**Synopsys 的 library characterization**：netlist、SPICE models、input Liberty → characterization tool（找出要量的 arc、產生 characterization plan）+ simulator（執行）→ 輸出 NLDM/NLPM、CCST/CCSN、LVF、Verilog、IBIS 等 model。

---

## 19. FPGA 的 technology mapping

[3:03:39]–[3:15:30]　投影片 Technology mapping for area/timing/FPGA/power、Conceptual FPGA Architecture、Xilinx XC4000、Lookup Table-Based Technology Mapping、Technology Mapping for FPGAs、Multiplexer-Based Technology Mapping

- Technology mapping 可以針對面積、timing、FPGA、power。投影片問：**先為面積做 mapping，再為 timing 或 power 最佳化？哪種比較好？**
- **FPGA = logic module + routing resources + I/O cells。**
- **Xilinx XC4000 logic module**：兩個 4 輸入 LUT、一個 3 輸入 LUT、兩個 DFF；可實作任兩個至多 4 變數的函數、一個至多 5 變數的函數，或某些至多 9 變數的函數。
- **LUT-based mapping**：k 輸入 LUT（k-LUT）能實作任何至多 k 輸入的函數。例：用 4 個 4-LUT、delay depth 3 LUT 的 mapping vs **最佳的 3 個 4-LUT、delay depth 2**。
- 對 5-LUT 的 mapping 技巧：
  - **Decomposition**：不分解要 4 個 LUT，分解後只要 2 個（常常要這樣做才放得進去）。
  - **Local reconvergent paths**：reconvergent path 放在同一個 LUT 裡實現，只用 2 個 LUT。
  - **Replication of logic**：不複製要 3 個 LUT，複製一個 gate 到兩邊後只要 2 個。
- **Multiplexer-based mapping**（Actel）：要實作 $f = ab + \bar ac$，設 $d_0 = d_1 = s_3 = x$、$d_2 = c$、$d_3 = b$、$s_0 = a$、$s_1 = s_2 = 1$。module 輸出
$$z = \overline{(s_3+s_2)}\,\overline{s_1s_0}\,d_0 + \overline{(s_3+s_2)}\,s_1s_0\,d_1 + (s_3+s_2)\,\overline{s_1s_0}\,d_2 + (s_3+s_2)\,s_1s_0\,d_3$$
  - ⚠️ 投影片上 $d_0 = d_1 = s_3 = x$ 的「x」看起來是「不在乎」的意思；代入：s₃ + s₂ = 1（s₂ = 1），所以 z = (s₁s₀)' d₂ + s₁s₀ d₃ = a'c + ab，成立。
  - 投影片：更依賴特定架構。

---

## 20. Low power：功耗模型與 low-power mapping

[3:15:30]–[3:28:12]　投影片 Power Dissipation、Power Dissipation Model、Logic Optimization for Low Power、Low Power Is a 1st-Order Cost Metric、Technology Mapping for Low Power、Minimum-Area/Power Mapping

**功耗的三種來源**：
- **Leakage power**：漏電流造成的靜態功耗；通常比其他小；**在 deep-submicron 製程越來越大**。
- **Short-circuit power**：transition 時 PMOS 和 NMOS 都開著造成的短路電流；通常比 dynamic power 小。
- **Dynamic power**：負載電容的充放電；**通常是總功耗的主要部分**。

$$P = \frac12 C V_{dd}^2 f$$

- 傳統應用用 dynamic power 代表總功耗。P：gate 的功耗；C：負載電容；V_dd：電源電壓；f：transition 頻率。
- 要算電路功耗，需要：每個節點的電容（從 layout 得到）、每個節點的 transition 頻率（計算得到）。

**Logic optimization for low power**：同一個函數，不同 cover 功耗不同：
- (a) $f = \bar a\bar b + a\bar c + bc$，P = 108.7 µW
- (b) $f = \bar b\bar c + \bar ac + ab$，P = 115.5 µW

**Low power 是一級 cost**：logic minimization 通常最小化 product term 數、輸入部分 literal 數、輸出部分 literal 數；low-power synthesis 要把功耗加進 cost function。timing、area、power 三者 **tradeoff**（PPA）。

**Technology mapping for low power**：
- 電路：G1（a, b, c，Pt = 0.109）、G2（d, e, f，Pt = 0.109）→ G3（Pt = 0.179）→ INV → INV → out（Pt = 0.179）。Pt：transition probability。
- Library：

| Gate | Area | Intrinsic Cap. | Input Load |
|---|---|---|---|
| INV | 928 | 0.1029 | 0.0514 |
| NAND2 | 1392 | 0.1421 | 0.0747 |
| NAND3 | 1856 | 0.1768 | 0.0868 |
| AOI33 | 3248 | 0.3526 | 0.1063 |

- **Minimum-area mapping**：AOI33 + INV，area **4176**、power **0.0907**。
- **Minimum-power mapping**：NAND3、NAND3、NAND2，area **5104**、power **0.0803**。（面積 1856 + 1856 + 1392 = 5104，正好三個 cell、沒有 INV——NAND2 直接得到 AND 的反相再反相的效果，取代 AOI33 + INV。）
- 【口頭】老師用試算表示範 power 怎麼算（投影片：「Let's think about how the power cost is calculated」）：每個節點的 power ∝ 該節點總電容（驅動 gate 的 intrinsic cap ＋ 下一級的 input load）× transition probability。
  - AOI33 輸出節點：0.3526 + 0.0514 = 0.404，× 0.179 = 0.072316；INV 輸出：0.1029 × 0.179 = 0.018419；合計 **0.09074**（≈ 0.0907）。
  - NAND3 輸出：0.1768 + 0.0747 = 0.2515，× 0.109 = 0.02741；另一個 NAND3 同樣 0.02741；NAND2 是最後一級、沒有下一級負載：0.1421 × 0.179 = 0.02543（就是老師試算表上那一格）；合計 **0.08026 ≈ 0.0803**，和投影片一致。
  - 所以 power 的算法是：每個 gate 輸出節點的（自己的 intrinsic cap ＋ 下一級的 input load）× 該節點的 transition probability，再加總。min-area 那組多了一級 INV，而且 AOI33 的 intrinsic cap 很大又接在高 transition probability（0.179）的節點上，所以比較耗電；min-power 那組把大電容放在 transition probability 較低（0.109）的節點上。
- 【口頭】也可以從 MOSFET 的 small-signal model 去想 power cost（投影片附 Berkeley 講義：transconductance $g_m = 2I_D/(V_{GS}-V_{TN})$、output resistance $r_o \approx 1/(\lambda I_D)$）。
- 投影片再問：先為面積 mapping 再最佳化 timing/power？並附上 WSJ「The Chips That Rebooted the Mac」（Apple M1 筆電風扇不轉）和台積電代工 M3 晶片的蘋果實驗室影片。

---

## 21. Design Compiler 流程

[3:28:12]–[3:31:12]　投影片 Terminology（江介宏）、HDL > Generic Boolean logic > Optimize > Map > Netlist、Design Compiler flow、DC topographical mode

**名詞**（江介宏，NTU LSV lec07）：
- Logic network：PI/PO、logic node、fanin／fanout、transitive fanin／fanout cone（TFI／TFO）。
- **Structural cut**：在 network 中把某節點和 PI 隔開的邊界；邊界上的節點是 **leaves**，該節點是 **root**；**K-feasible cut** 有 K 個或更少的 leaf；cut 的函數 = root 用 leaf 表示的函數。（這是 LUT mapping 的基礎。）

**Synthesis = Translation + Optimization + Mapping**：

```verilog
residue = 16'h0000;
if (high_bits == 2'b10)
    residue = state_table[index];
else state_table[index] = 16'h0000;
```
HDL → translate（沒有 timing 資訊）→ generic Boolean → optimize + map（有 timing 資訊）→ target technology。

**Synopsys Design Compiler 流程**：
1. Develop HDL files
2. **Specify libraries**（載入 tech library）：`link_library`、`target_library`、`symbol_library`、`synthetic_library`
3. **Read design**（read、analyze、elaborate）：`analyze`、`elaborate`、`read_file`
4. **Define design environment**：`set_operating_conditions`、`set_wire_load_model`、`set_drive`、`set_driving_cell`、`set_load`、`set_fanout_load`、`set_min_library`
5. **Set design constraints**：
   - design rule：`set_max_transition`、`set_max_fanout`、`set_max_capacitance`
   - optimization：`create_clock`、`set_clock_latency`、`set_propagated_clock`、`set_clock_uncertainty`、`set_clock_transition`、`set_input_delay`、`set_output_delay`、`set_max_area`
6. **Select compile strategy**：top down、bottom up
7. **Optimize the design**：`compile` 或 `compile_ultra`（必要時重複）
8. **Analyze and resolve design problems**：`check_design`、`report_area`、`report_constraint`、`report_timing`
9. **Save the design database**：`write`

**DC topographical mode**：輸入 RTL、constraints、logic library、physical library、floorplan physical constraints（optional）、UPF（power intent，給 power-aware synthesis）；輸出 netlist、reports（QoR、area、timing）、UPF。
- 【口頭】physical information 也可以進來，所以是 optional、做得更好。做 technology mapping 時不知道 wire loading，所以可以設一個 **wire load model** 把它考慮進去。

---

## 22. Delay model 與 timing analysis

[3:31:12]–[3:39:54]　投影片 Delay Model at Logic Level、Linear Delay Model、Tabular Delay Model、Setup Time & Hold Time Check、Arrival Time and Required Time；課本 §6.5（p.371–）

**Logic level 的 delay model**：
- **Unit delay model**：每個 gate delay = 1。
- **Unit fanout delay model**：每多一個 fanout 加一點 delay。
- **Library delay model**：用 library 的 delay 資料，比較準；可以是 linear 或 non-linear（表格）。
- 【口頭】一路演變；現在大部分用 non-linear 的 timing delay model，存在 `.lib` 檔。

**Linear delay model**（through cells）：

$$D = D_{slope} + D_{intrinsic} + D_{transition} + D_{wire}$$

- $D_{slope}$：B 的 transition 在輸入 A 造成的 delay。
- $D_{intrinsic}$：cell 從輸入到輸出本身的 delay。
- $D_{transition}$：輸出 pin 的負載和驅動力造成的 delay。
- $D_{wire}$：C 點狀態轉變到 D 點狀態轉變的時間。
- 投影片註：timing 資料要跑很多次 SPICE。

**Tabular delay model**：
- delay 由 **look-up table** 得到：
  - 二維表：對 input slope（m）和 total output capacitance（n）
  - 一維表：output slope 對 total output capacitance
  - 每個值都是實際量測（模擬）的
- 例：

| Input transition (ns) \ Total output load (fF) | 0.2 | 0.3 | 0.4 | 0.5 |
|---|---|---|---|---|
| 0 | 3 | 4.5 | **6** | 7 |
| 0.1 | 5 | 8 | 10.7 | 13 |

  input transition 0.0 ns、負載 0.4 fF → cell delay **6.0 ps**。
- 比 linear model 準；表越大越準；但要更多空間存表。

**Setup／hold check**：FF1 → combinational logic → FF2，clock 經過 clock path 到兩個 FF。
- setup check（下一個 capture edge）：$T_{ck\to q} + T_{prop} + T_{setup} - T_{skew} \le T$（clock 週期）。
- hold check（同一個 edge）：$T_{ck\to q} + T_{prop} > T_{hold} + T_{skew}$。
- ⚠️ 投影片兩個式子右半部被裁掉，右邊的「≤ T」和「+ T_skew」是依 setup／hold 的標準定義補上的。

**Arrival time 與 required time**：
- arrival time 從輸入往輸出算；required time 從輸出往輸入算；**slack = required − arrival**。

$$A(j) = \max_{k\in FI(j)}[A(k) + D(j,k)]$$
$$r(j,k) = R(j) - D(j,k), \qquad R(k) = \min_{j\in FO(k)}[r(j,k)]$$
$$S(k) = R(k) - A(k)$$

（A：arrival time；R：required time；S：slack；D(j,k)：節點 j 從輸入 k 的 delay；FI：fanin；FO：fanout。）

---

## 23. 總結

[3:39:54]　投影片 Summary So Far

- 講了 BDD、Quine-McCluskey logic minimization、standard cell 的 technology mapping，簡單提了 FPGA lookup table。
- 碰了一下 delay analysis。
- 也簡單談了 timing optimization、low power。
- **非常重要的領域。**

---

## 24. 【實測】

程式：`experiments/lecture06/bdd_qm.py`，輸出：`experiments/lecture06/output.txt`。

**(1) ROBDD 節點數**（自己寫的小 ROBDD：unique table 合併 isomorphic node、`mk` 刪掉 lo == hi 的冗餘節點、用 ITE 建 AND/OR/NOT/XOR）：

```python
    def mk(self, v, lo, hi):
        if lo == hi:                     # rule: eliminate redundant test
            return lo
        key = (v, lo, hi)
        if key not in self.unique:       # rule: merge isomorphic nodes
            n = len(self.nodes)
            self.nodes[n] = key
            self.unique[key] = n
        return self.unique[key]
```
（`experiments/lecture06/bdd_qm.py:18-26`）

```
  f = abc + b'd + c'd   order b<c<d<a: 4
  f = abc + b'd + c'd   order a<b<c<d: 6
  f = abc + b'd + c'd   order a<d<b<c: 7
  f = abc + b'd + c'd   order b<c<a<d: 4
  f = ab + cd + ef      order a<b<c<d<e<f: 6
  f = ab + cd + ef      order a<c<e<b<d<f: 14
  odd parity of 4 vars: 7  (2n-1 = 7)
  odd parity of 8 vars: 15  (2n-1 = 15)
  odd parity of 16 vars: 31  (2n-1 = 31)
```

- $abc + \bar bd + \bar cd$：b < c < d < a（Fig. 6.8）和 b < c < a < d（Fig. 6.4「optimal」）都是 4 個節點；a < b < c < d（Fig. 6.2）6 個；a < d < b < c（Fig. 6.3）7 個——和投影片圖的大小順序一致。
- $ab + cd + ef$：好的 order 6 個節點，壞的 order 14 個。這種「成對」的函數，n 對變數時好 order 是 2n、壞 order 是 $2^{n+1} - 2$，指數成長。
- Odd parity 永遠是 2n − 1，和投影片一致。

**(2) Quine-McCluskey + Petrick（精確解）**：

```
QM example 1  Σm(0,2,5,6,7,8,10,12,13,14,15) primes: ['AB', "AD'", "B'D'", 'BC', 'BD', "CD'"]
   minimum covers: [['AB', "B'D'", 'BD', "CD'"], ['AB', "B'D'", 'BC', 'BD'], ["AD'", "B'D'", 'BC', 'BD'], ["AD'", "B'D'", 'BD', "CD'"]]
QM example 2  Σm(0,2,3,4,5,6,7,8,9,10,11,12,13) primes: ["A'B", "A'C", "A'D'", "AB'", "AC'", "B'C", "B'D'", "BC'", "C'D'"]
   minimum covers: [["A'B", "A'D'", "AC'", "B'C"], ["A'C", "AB'", "BC'", "C'D'"], ["A'C", "AB'", "B'D'", "BC'"], ["A'B", "AC'", "B'C", "B'D'"], ["A'C", "A'D'", "AB'", "BC'"], ["A'B", "AC'", "B'C", "C'D'"]]
QM example 3  Σm(2,3,7,9,11,13)+d(1,10,15) primes: ['AD', "B'C", "B'D", 'CD']
   minimum covers: [['AD', "B'C", 'CD']]
two-level example primes: ["x1x3'", "x2'"]  cover: [["x1x3'", "x2'"]]
```

- **例 1**：6 個 prime implicant 和投影片 Column III 一致；投影片的答案 $B'D' + BD + CD' + AD'$ 是 4 個最小解之一（B'D'、BD 每個解都有，因為是 essential）。
- **例 2**：9 個 prime implicant 和投影片一致；投影片用 Petrick 得到的兩個解 $A'D' + A'C + BC' + AB'$、$A'D' + B'C + A'B + AC'$ 都在清單裡。但精確窮舉顯示**共有 6 個 4 項的最小解**，其他 4 個用 B'D' 或 C'D' 取代 A'D'——因為 A'D'、B'D'、C'D' 三者互相「co-dominate」，投影片的流程選擇刪掉 B'D'、C'D'，只是其中一種走法。
- **例 3**（含 don't care）：唯一最小解 $AD + B'C + CD$（筆記不提供作業解答，這裡只是投影片上標示的練習）。
- Two-level 例子：$F = \bar x_2 + x_1\bar x_3$，和投影片一致。

---

## 25. 一頁速查表

| 主題 | 重點 |
|---|---|
| Logic synthesis | RTL → gate netlist；optimization（tech-indep）＋ technology mapping（tech-dep） |
| Domain translation | 3-address code `x = y op z`；if → implicit MUX；control/data flow analysis |
| Two-level | 最小化 product term；K-map、QM（exact）、Espresso（heuristic） |
| Multi-level | 最小化 literal；factoring、kernel extraction、resubstitution；MIS、ABC |
| 名詞 | minterm、cube、implicant、prime implicant、prime／irredundant／minimal cover |
| Canonical | truth table（指數大）、ROBDD；irredundant cover 不是 |
| BDD | Bryant 1986；DAG；Shannon f = x f_x + x' f_x' |
| ROBDD 規則 | 合併相同葉子、合併 isomorphic 節點、刪掉 lo = hi 的節點 |
| 核心運算 | Restrict、ITE（= MUX） |
| Ordering | 影響大小（線性 vs 指數）；找最佳 order 很難 |
| BDD 用途 | equivalence checking、SAT、min-cost SAT（最短路徑，線性時間） |
| Set covering | NP-complete；unate／binate；essential、row／column dominance |
| QM | 全部 PI → covering matrix → essential、dominance → cyclic core → Petrick／branching |
| Petrick | 每列寫成 (P_i + P_j)，乘開取變數最少的項 |
| Espresso | REDUCE → EXPAND → IRREDUNDANT 迴圈；不列舉所有 PI |
| Tech mapping | subject graph（NAND2+INV）用 pattern graph 做最小 cost cover；每個輸入必須是某 pattern 的輸出 |
| DAG covering | NP-hard；tree 時 DP O(n)；DAGON 在 multi-fanout 處切開 |
| Rule-based | local transform；順序重要；greedy → local minimum |
| Boolean matching | P／NP／NPN equivalence |
| FPGA | k-LUT 實作任意 k 輸入函數；decomposition、reconvergence、replication |
| Power | P = ½CV²f；leakage、short-circuit、dynamic；min-area ≠ min-power |
| Delay model | unit、unit fanout、library（linear D = slope + intrinsic + transition + wire；NLDM 表格） |
| STA | A = max(A + D)；R = min(R − D)；slack = R − A |

---

## 26. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| CUDD（Prog-1）、BDD（Hw-2） | 作業與程式（[第 1 支](lecture01_note.md) §13） |
| 面積／wire load model | 第 7 支：compaction；第 10 支：placement（wirelength） |
| partitioning（把 network 切成小塊） | 第 8 支：ch4 partitioning |
| DP、principle of optimality | 第 9 支：floorplanning（slicing tree 的 shape function） |
| timing（arrival／required／slack）、interconnect delay | 第 10–12 支：timing-driven placement／routing |
| clock、skew、setup／hold | 第 14–15 支：clock tree synthesis |

---

## 附：本講一句話總結

> **Logic synthesis 把 HDL 先做 domain translation（3-address code、if 變 MUX），再做 technology-independent 最佳化，最後 technology mapping 到 cell library：two-level 最佳化的目標是 minimal irredundant prime cover，Quine-McCluskey 先列出所有 prime implicant 再用 essential、row／column dominance 化簡 covering matrix，剩下的 cyclic core 用 Petrick's method 或 branching 解（本機精確窮舉驗證投影片答案，也發現例 2 其實有 6 個最小解），實務上用不列舉所有 PI 的 Espresso（REDUCE-EXPAND-IRREDUNDANT），multi-level 則用 MIS、ABC；Bryant 的 ROBDD 靠合併相同葉子、合併 isomorphic 節點、刪除冗餘測試三條規則得到 canonical form，所以兩電路等價 iff BDD 相同，但大小非常依賴 variable ordering（實測 ab+cd+ef 是 6 vs 14 個節點、odd parity 恆為 2n−1）；technology mapping 是用 pattern graph 對 NAND2+INV 的 subject graph 做最小 cost covering，一般 DAG 是 NP-hard，DAGON 在 multi-fanout 處把 DAG 切成 tree、對每棵 tree 用 dynamic programming（principle of optimality）在 O(n) 時間求最佳解，Keutzer 的例子顯示 greedy 得不到最佳的 17；另外還有 FPGA 的 LUT mapping、P = ½CV²f 的 low-power mapping（最小面積不等於最小功耗）、Liberty 的 NLDM 表格模型，以及 STA 用 arrival、required time 算 slack。**

---

*筆記依據李炯霆「Electronic Design Automation (1)」第6講｜ch10.synthesis 錄影（NTHU OCW，3:40:34；影片無字幕，以 whisper large-v3-turbo 轉錄）、投影片截圖（黃鐘揚、張耀文、江介宏、Kurt Keutzer、Steven Nowick 的教材）、Wang, Chang, Cheng（eds.）*Electronic Design Automation: Synthesis, Verification, and Test* Ch6 整理，部分例子取自 Hachtel & Somenzi *Logic Synthesis and Verification Algorithms* Ch6、Ch11。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
