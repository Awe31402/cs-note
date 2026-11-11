# 設計自動化（一）第三支影片完整筆記：計算複雜度、NP-completeness 與搜尋演算法（ch2 complexity）

> **課程**：Electronic Design Automation (1)（清大 CSR 5302）— 第3講｜ch2.complexity（播放清單第 3 支）
> **講者**：李炯霆（JT Li），清大半導體學院，2026 春，NTHU OCW 錄影
> **來源**：YouTube 錄影（2:12:40，無字幕，whisper 轉錄）＋ 投影片截圖（Unit 1A: Computational Complexity，p.3–85；投影片頁尾註明改編自 Chang、Huang、Li、Lin、Liu）＋ 課本 Wang, Chang, Cheng（eds.）*Electronic Design Automation* Ch4 §4.2–4.4（p.175–221）
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
- [1. 為什麼 EDA 要談 complexity](#1-為什麼-eda-要談-complexity)
- [2. Big-O 與 asymptotic function](#2-big-o-與-asymptotic-function)
- [3. Running time 比較表](#3-running-time-比較表)
- [4. Optimization problem：MST、LRP、TSP](#4-optimization-problemmstlrptsp)
- [5. Decision problem 與 Circuit-SAT](#5-decision-problem-與-circuit-sat)
- [6. P、NP 與 Turing machine](#6-pnp-與-turing-machine)
- [7. NP-complete、Cook–Levin、Karp](#7-np-completecooklevinkarp)
- [8. Polynomial-time reduction](#8-polynomial-time-reduction)
- [9. NP-hard 與 NP-complete 的差別](#9-np-hard-與-np-complete-的差別)
- [10. 證明 NP-complete：HC ≤P TSP](#10-證明-np-completehc-p-tsp)
- [11. 怎麼對付 NP-hard 問題](#11-怎麼對付-np-hard-問題)
- [12. Minimum spanning tree 與 Prim](#12-minimum-spanning-tree-與-prim)
- [13. Backtracking 與 branch-and-bound（TSP 例子）](#13-backtracking-與-branch-and-boundtsp-例子)
- [14. DFS 與 BFS](#14-dfs-與-bfs)
- [15. 演算法典範與 DP vs divide-and-conquer](#15-演算法典範與-dp-vs-divide-and-conquer)
- [16. Transportation 例子：backtracking、DP、UCS](#16-transportation-例子backtrackingdpucs)
- [17. Bin packing：FFD 與 ILP](#17-bin-packingffd-與-ilp)
- [18. Recap](#18-recap)
- [19. 【實測】](#19-實測)
- [20. 一頁速查表](#20-一頁速查表)
- [21. 與後續講次的連結](#21-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
complexity：big-O、running time 表 → 為什麼 EDA 程式要 scale
問題分類：optimization vs decision → P、NP、NP-complete、NP-hard
   → polynomial-time reduction → 例：HC ≤P TSP
對付 NP-hard：approximation、pseudo-polynomial、restriction、B&B、local search、heuristic
演算法：MST（Prim）→ TSP 的 backtracking / branch-and-bound → DFS / BFS
   → DP vs divide-and-conquer → transportation 例子（backtracking、DP、UCS）
   → bin packing（FFD、ILP）
```

---

## 1. 為什麼 EDA 要談 complexity

[0:00]–[2:51]　投影片 Unit 1A: Computational Complexity；課本 §4.1（p.173–175）

- 內容：
  - Computational complexity：time、space
  - NP-completeness
  - Algorithmic paradigms：spanning tree、Steiner tree、rectilinear spanning／Steiner tree；search（exhaustive、branch-and-bound、breadth、depth、dynamic），搭配 Stanford AI 課程影片
  - 歡迎多讀 graph theory
- 閱讀：參考書 Chapters 3、4、5（【口頭】指 Gerez 的書）。
- 【口頭】EDA 工具都要看用什麼 algorithm、complexity 多少；設計越來越大時，complexity 太高根本跑不完，memory 吃太大也跑不起來。EDA 裡很多問題找不到 polynomial time 的最佳解，只好用夠 effective 的 heuristic。

---

## 2. Big-O 與 asymptotic function

[2:51]–[9:27]　投影片 O: Upper Bounding Function、Big-O Notation、Computational Complexity、Asymptotic Functions；課本 §4.2.1（p.175–178）

**定義**：$f(n) = O(g(n))$ 若存在 $c > 0$、$n_0 > 0$，使得對所有 $n \ge n_0$，$0 \le f(n) \le c\,g(n)$。

- 例：$2n^2 + 3n = O(n^2)$、$2n^2 = O(n^3)$、$3n \lg n = O(n^2)$。
- 直覺：忽略常數倍和小的 n 時，f(n)「≤」g(n)。
- 更多例子：$\frac13 n^2 = O(n^2)$；$0.02n^2 + 127n + 1923 = O(n^2)$；$3n\log n + n = O(n\log n)$；$5n^3 = O(\frac15 n^3) = O(n^4) = O(n^3 + n^2)$。
  - 【口頭】big-O 是 **upper bound**，通常用一個比較簡單的函數來代表；$n^3$ 一定被 $n^4$ bound 住，寫成 $n^3 + n^2$ 有點多餘但也對。
- 另一種看法：$f(n) = O(g(n))$ iff $\lim_{n\to\infty} f(n)/g(n) = c$，$c \ge 0$。
- 「演算法的 **worst-case** running time 是 O(f(n))」：存在常數 c，使得 n 夠大時，**每一次**在大小 n 的輸入上執行**最多**花 c·g(n) 時間。

**Computational complexity**：執行演算法所需時間與空間的抽象量度，表示成**輸入大小**的函數。

| 輸入 | 輸入大小 |
|---|---|
| 排序 n 個有界長度的字 | n |
| 整數 n | lg n（位元數） |
| 圖 G(V, E) | \|V\| 和 \|E\| |

- **時間** complexity 用基本計算步驟數（加法、乘法、指標取值）；**空間** complexity 用記憶體位置數（bit、byte、word）。

**Asymptotic functions**：
- Polynomial：$O(n^k)$，k 是常數——999（constant）、lg n（logarithmic）、√n（sublinear）、n（linear）、n lg n（loglinear）、n²（quadratic）、n³（cubic）。
- Non-polynomial：2ⁿ、3ⁿ（exponential）、n!（factorial）。

---

## 3. Running time 比較表

[2:51]、[9:27]–[13:15]　投影片 Running-time Comparison（假設 1000 MIPS，一個運算一道指令）

| Time | Big-O | n = 10 | n = 100 | n = 10³ | n = 10⁶ |
|---|---|---|---|---|---|
| 500 | O(1) | 5×10⁻⁷ s | 5×10⁻⁷ s | 5×10⁻⁷ s | 5×10⁻⁷ s |
| 3n | O(n) | 3×10⁻⁸ s | 3×10⁻⁷ s | 3×10⁻⁶ s | 0.003 s |
| n lg n | O(n lg n) | 3×10⁻⁸ s | 2×10⁻⁷ s | 3×10⁻⁶ s | 0.006 s |
| n² | O(n²) | 1×10⁻⁷ s | 1×10⁻⁵ s | 0.001 s | 16.7 min |
| n³ | O(n³) | 1×10⁻⁶ s | 0.001 s | 1 s | 3×10⁵ 世紀 |
| 2ⁿ | O(2ⁿ) | 1×10⁻⁶ s | 3×10¹⁷ 世紀 | ∞ | ∞ |
| n! | O(n!) | 0.003 s | ∞ | ∞ | ∞ |

- 讀表的方法：n 乘 10 倍，O(n) 的時間也乘 10；O(n²) 的時間乘 100（T → 10²T → 10⁴T）；O(n³) 乘 1000。
- 投影片註記：n² 在 n = 10⁶ 時約 1000 秒（= 16.7 分）；10⁹ 秒 = 0.316887 世紀。
- 【口頭】反過來，從「輸入大小變化」和「花的時間變化」就能推出演算法的 complexity。老師提到表上某格「答案有點不太對」，但逐字稿聽不出是哪一格。⚠️
  - 檢查一下：n³ 在 n = 10⁶ 是 10¹⁸ 個運算 ÷ 10⁹ = 10⁹ 秒 ≈ 31.7 年 ≈ 0.317 世紀，不是 3×10⁵ 世紀。投影片上「1,000,000,000 s = 0.316887 c」正是在算這一格，所以表上的「3×10⁵ cent.」應該是錯的（這是依表格本身的假設算出來的，不是外部資料）。

---

## 4. Optimization problem：MST、LRP、TSP

[13:15]–[22:15]　投影片 Optimization Problems、Min Spanning Tree、RLC model、TSP；課本 §4.2.2（p.178–）

- **Problem**：一整類問題，例如「有向無環圖（DAG）上的最短路徑問題」。
- **Instance**：問題的一個具體情況，例如「某一張圖上、**給定兩點之間**的最短路徑」。
  - 【口頭】instance 給了 source 和 target，搜尋範圍就小一點。
- **Optimization problem**：找一個合法的 configuration，使 cost 最小（或 gain 最大）。
  - 例：MST——給圖 G = (V, E)，求最小生成樹的 cost。
- 一個 instance I = (F, c)：F 是 **feasible solution** 的集合，c: F → ℝ 是 **cost function**。
- 解 = cost 最佳（最小或最大）的 feasible solution。
- 注意：EDA 社群說的 optimal（最佳）演算法，在理論 CS 社群叫 **exact**（optimal ≠ exact）。**Exact vs approximate。**
  - 【口頭】最佳解不一定唯一。

**MST 圖例**：平面圖和它的最小生成樹（粗線），每條邊的權重約略和長度成正比。

**應用：power/ground 網路的 LRP**（投影片：RLC model of supply grid network）
- 抽出 P/G 網路的電阻、電容，得到一張 RLC 網格；有**多個電源**。
- 問題：給一個 instance pin，找 **LRP（least resistive path）**。
- 【口頭】找到 LRP 後檢查合不合理：如果某處電阻太小，電流會都往那裡跑；我們希望整個 P/G 網路很 **balanced**，每一點的 IR drop 差不多。這是 MST 類問題的延伸，source 比較多、比較複雜。

**Traveling Salesman Problem（TSP）**：給一組城市和兩兩之間的距離，求從指定城市出發、**每個城市恰好拜訪一次**、回到起點的**最短 tour**。
- 【口頭】n 個城市兩兩有距離，邊數是 n(n−1)/2 ≈ n²，所以輸入大小就有 n²。
- 圖例的 5 城市圖並不是每兩城都有邊（例如 A 到 C 沒有邊），投影片示範一個 **greedy 解（approximation）**：每次走最短的邊。

---

## 5. Decision problem 與 Circuit-SAT

[22:15]–[28:06]　投影片 Decision Problem、The Circuit-Satisfiability Problem

- **Decision problem**：只能回答 yes 或 no 的問題。
  - MST：給圖 G 和 bound K，有沒有 cost **至多 K** 的生成樹？
  - TSP：給城市、距離和 bound B，有沒有從指定城市出發、每城恰訪一次、總距離**至多 B** 的路線？
- 一個 decision problem Π 的 instance 是 I = (F, c, k)；答案是 yes 的 instance 集合記為 $Y_\Pi$。
- 子任務 **solution checking**：給 f ∈ F，檢查 cost 是否小於 k。
- **可以對 decision problem 做 binary search 來解 optimization problem。**
  - 【口頭】把可能的 bound 排好，從中間試；cost 太大就往一邊、否則往另一邊。
- **NP-completeness 是針對 decision problem 定義的。**
  - 【口頭】比較複雜，不太懂也沒關係；重點是 EDA 裡很多問題是 NP-complete，找不到 n²、n³ 這種 polynomial time 演算法，都要變 exponential。

**Circuit-SAT**：
- Instance：由 AND、OR、NOT 組成的組合電路 C。
- Question：有沒有一組輸入的 Boolean 值，讓 C 的輸出為 1？
- 電路 **satisfiable** = 存在一組輸入讓輸出為 1。投影片 (a) 在 ⟨x₁, x₂, x₃⟩ = ⟨1, 1, 0⟩ 時輸出 1，所以 satisfiable；(b) 長得很像，但不管怎麼給輸入輸出都是 0，unsatisfiable。
  - 【口頭】(b) 是把 (a) 裡的一個 gate 換掉（逐字稿說把 NOR 換成 AND）造成的。⚠️ 換的是哪個 gate 以投影片為準，逐字稿聽不清楚。

---

## 6. P、NP 與 Turing machine

[28:06]–[37:54]　投影片 Complexity Class P、Complexity Class NP、輕鬆談演算法的複雜度分界；課本 §4.2.2

**Class P**：在輸入大小的 polynomial time 內**可以解**的問題。
- 輸入大小 = 編碼成 binary 字串的長度。
- Edmonds：P 裡的問題被視為 **tractable**。
- **MST ∈ P。**
- 這裡的電腦是 **deterministic Turing machine**：每一步都可預測；Turing machine 是通用電腦的數學模型（在一台 TM 上 polynomial time 的計算，在任何其他機器上也是 polynomial time）。

**Class NP（Nondeterministic Polynomial）**：在輸入大小的 polynomial time 內**可以驗證**的問題。
- 如果 solution checking 在 deterministic machine 上是 polynomial time，那問題就能在 **nondeterministic** Turing machine 上 polynomial time 解。
- Nondeterministic：機器會猜（而且猜對），或者說機器能**平行地評估所有可能**。
  - 【口頭】想像一台非常 powerful 的機器，一次把所有 possibility 展開，每一個去檢查。
- **TSP ∈ NP 嗎？** 要在 polynomial time 內檢查一個解：
  1. 猜一個 tour
  2. 檢查每個城市恰訪一次
  3. 檢查回到起點
  4. 檢查總距離 ≤ B
  - 全部 O(n) 時間 → **TSP ∈ NP**。

**中文補充**（投影片引用 ycc.idv.tw「輕鬆談演算法的複雜度分界」）：
- DTM：給定 state 和 symbol，下一步只有一種可能。
- NTM：下一步可能有很多種，像讀寫頭一分為二、分支可以無限多個，只要某個分支到達 halt state 就解完。
- 用 DTM 計算是 polynomial time → P；用 NTM 計算是 polynomial time → NP。

---

## 7. NP-complete、Cook–Levin、Karp

[37:54]–[43:36]　投影片 SAT is the first problem…、NP-Complete 定義、Why powerful?

- **SAT 是第一個被證明是 NP-complete 的問題**（Cook–Levin theorem）。NP 裡所有問題（包括很多自然的 decision 和 optimization 問題）都**不比 SAT 難**。
  - 【口頭】老師記得這兩位當時都在 MIT。⚠️ 這和史實不同：Stephen Cook 是在多倫多大學（1971 年論文「The complexity of theorem-proving procedures」）、Leonid Levin 是在蘇聯（1973 年）各自獨立證明的，所以才叫 Cook–Levin theorem。
- 圖：P ⊂ NP；NP 和 NP-Hard 的交集是 NP-Complete；難度由 easy（P）到 hardest（NP-Hard）。

**NP-complete 的定義**：decision problem C 是 NP-complete，若
1. C ∈ NP，而且
2. NP 裡**每一個**問題都能在 polynomial time 內 **reduce** 到 C。

**為什麼這個概念這麼強？**
- 如果證明 SAT ∈ P，那任何 NP 問題都能在 polynomial time 轉成 SAT 再解掉 → **P = NP**。所以 NP-complete 是解決 P = NP 問題的關鍵。
- **1972 年 Richard Karp** 證明了 **21 個**不同但都很難的組合數學與圖論問題是 NP-complete；其中任何一個被證明在 P，就間接證明 P = NP。
  - 【口頭】Karp 在 UC Berkeley 的 CS 和數學系。
- 圖：若 P ≠ NP（最可能的情況），P 和 NPC 是 NP 裡兩塊不相交的區域；若 P = NP，則 P = NP = NPC。

---

## 8. Polynomial-time reduction

[43:36]–[46:54]　投影片 Polynomial-time Reduction、Significance of Reduction、Fig. 4.4；課本 §4.2.3

- 動機：L₁、L₂ 是兩個 decision problem，演算法 A₂ 能解 L₂。能不能用 A₂ 解 L₁？
- **Polynomial-time reduction f from L₁ to L₂**，記作 $L_1 \le_P L_2$：
  - f 把 L₁ 的輸入轉成 L₂ 的輸入，使得轉換後是 L₂ 的 yes 輸入 **iff** 原輸入是 L₁ 的 yes 輸入。
  - 形式化：存在 polynomial-time 可計算的 $f: \{0,1\}^* \to \{0,1\}^*$，使得 $x \in L_1 \iff f(x) \in L_2$，∀x。
  - **L₂ 至少和 L₁ 一樣難。**
- 圖：A₁（L₁ 的演算法）= 先用 F（reduction）把 x 變成 f(x)，再用 A₂ 判斷 f(x) ∈ L₂ 的答案，就是 x ∈ L₁ 的答案。
- **意義**：
  - L₂ 有 polynomial-time 演算法 ⇒ L₁ 也有（L₂ ∈ P ⇒ L₁ ∈ P）。
  - L₁ 沒有 polynomial-time 演算法 ⇒ L₂ 也沒有（L₁ ∉ P ⇒ L₂ ∉ P）。
  - $\le_P$ 有**遞移性**：L₁ ≤P L₂ 且 L₂ ≤P L₃ ⇒ L₁ ≤P L₃。
- Fig. 4.4（Gerez）：instance I →（polynomial 的 transformation f）→ f(I)，$I \in Y_{\Pi_1} \iff f(I) \in Y_{\Pi_2}$。
  - 【口頭】如果 transformation 本身要 exponential time，那就沒用了。

---

## 9. NP-hard 與 NP-complete 的差別

[46:54]–[56:18]　投影片 NP-Completeness、baeldung 分類、NP-hard vs NP-complete

- 尚未解決：P ⊂ NP 還是 P = NP？大家**強烈相信 P ≠ NP**，因為存在 NP-complete 問題。
- **NPC** 由 S. Cook 和 R. Karp 在 1970 年代初發展。所有 NPC 問題難度相同：**任何一個** NPC 問題能 polynomial time 解 ⇒ **所有** NP 問題都能。
  - 【口頭】到目前為止據老師所知還沒有決定。

**分類**（投影片引用 baeldung.com）：

| 類別 | 難度 | 例子 |
|---|---|---|
| P | easy | 基本算術、質數測試、hash table lookup、字串操作、排序、最短路徑（Dijkstra、Bellman-Ford、Floyd-Warshall）、linear／binary search |
| NP | medium | integer factorization、graph isomorphism |
| NP-complete | hard | TSP、knapsack、graph coloring、SAT |
| NP-hard | hardest | K-means clustering、TSP、graph coloring |

- 【口頭】graph isomorphism：找一組 vertex 對應，使原圖有邊的兩點，對應過去也有邊。graph coloring：相鄰的點不同色，求最少顏色數。K-means 在 machine learning 常用。TSP 雖然沒有 polynomial time 演算法，但**實務上還是有有效率的演算法**。
- ⚠️ 表上 TSP 和 graph coloring 同時出現在 NP-complete 和 NP-hard——decision 版本是 NP-complete，optimization 版本是 NP-hard（見下）。

**口訣**：
- P：解得快
- NP：驗得快、解得慢
- NP-complete：驗得快、解得慢，而且能 reduce 到任何其他 NP-complete 問題
- NP-hard：驗得慢、解得慢，任何 NP 問題都能 reduce 到它

**例**：「求 TSP 所有 instance 中最短路徑的長度」是 **optimization** 問題，不是 decision 問題，所以是 **NP-hard**——解得了它就能解 decision 版本的 TSP（NP-complete），進而能 polynomial time 解任何 NP 問題。

| | NP-hard | NP-complete |
|---|---|---|
| 要在 NP 裡嗎 | 不必 | 一定在 NP 而且是 NP-hard |
| 要是 decision problem 嗎 | 不必 | 只能是 decision problem |

圖：所有 decision problem ⊃ NP ⊃ P、NPC；NP-hard 是一塊延伸到 decision problem 以外的虛線區域，NPC 是 NP 和 NP-hard 的交集。

---

## 10. 證明 NP-complete：HC ≤P TSP

[56:18]–[1:07:48]　投影片 Proving NP-Completeness、Polynomial-time Reduction（HC）、HC ≤P TSP Step 1–2、HC → TSP、Is HC NP-Complete?

**證明 L 是 NP-complete 的五個步驟**：
1. 證明 L ∈ NP。
2. 選一個**已知的** NP-complete 問題 L'。
3. 建構 reduction f，把 L' 的**每一個** instance 轉成 L 的 instance。
4. 證明對所有 x ∈ {0,1}*，$x \in L' \iff f(x) \in L$。
5. 證明 f 是 polynomial-time transformation。

圖：**已知的 NP-complete 問題 L'** —f, reduce→ **要證明的問題 L**。
- 【口頭】要記得：是**從已知的 NP-complete 問題 reduce 到新問題**，不是反過來。x ∈ {0,1}* 的意思是答案是 yes 或 no：這邊 yes 那邊也 yes、這邊 no 那邊也 no。

**Hamiltonian Circuit（HC）問題**：
- Instance：無向圖 G = (V, E)。
- Question：G 裡有沒有一個包含**每個頂點恰好一次**的 cycle？
- 圖：Hamiltonian（十二面體圖，有一筆畫的 cycle）vs nonhamiltonian（二分圖）。

**怎麼證明 HC ≤P TSP？**
1. 定義 f 把任意 HC instance 轉成 TSP instance，並證明 f 可以 polynomial time 計算。
2. 證明 G 有 HC ⟺ 轉換後的 instance 有距離 ≤ B 的 TSP tour。

**Step 1**：給 n 個頂點的 HC instance G = (V, E)：
- 建立 n 個城市，名字就是 V。
- 距離：
$$d(u,v) = \begin{cases} 1 & (u,v) \in E \\ 2 & (u,v) \notin E \end{cases}$$
  （投影片把 2 劃掉，註記「**Assign a big number, say 1M**」）
- bound B = n。
- f 可以在 O(V²) 時間算出。
- 例：HC ⟨1, 5, 2, 3, 4, 1⟩ → TSP tour ⟨1, 5, 2, 3, 4, 1⟩，distance bound B = 5。

**Step 2（⇒）**：x ∈ HC ⇒ f(x) ∈ TSP。HC h = ⟨v₁, …, vₙ, v₁⟩ 也是轉換後 TSP 的一個 tour，n 條連續的邊都在 E 裡、每條距離 1，所以 tour 長 n = B。
**Step 2（⇐）**：f(x) ∈ TSP ⇒ x ∈ HC。有一個距離 ≤ n = B 的 tour，它有 n 條邊，所以每條都只能是距離 1 的邊（都在 E 裡），所以它就是 Hamiltonian cycle。

**HC → TSP 的白話**：要在任意圖 A 找 HC，就建一個同樣點數的 complete graph，A 裡相鄰的點連權重 1 的邊，其他給很大的數，bound 設成 N，然後去找 TSP。
- 【口頭】設 100 萬的話，TSP 一走到任何 100 萬的邊，整個解就超過 100 萬，絕對不是好解。

**Is HC NP-complete?**
- 投影片：「Shall we try to prove TSP → HC?」（被劃掉）。真正用來證 HC 是 NP-complete 的是 Hamiltonian path → HC、vertex cover → HC、3-SAT → HC。
- 【口頭】**這組投影片有點 misleading**：證明 HC ≤P TSP 不代表 HC 是 NP-complete；要證 HC 是 NP-complete，必須從一個已知的 NP-complete 問題 reduce **到 HC**。老師上網查，看到的是從 3-SAT、vertex cover、Hamiltonian path reduce 過來的，沒看到從 TSP 過來的。
- 【課本補充】HC ≤P TSP 證明的其實是「**TSP 是 NP-hard**」（前提是已知 HC 是 NP-complete），再加上 TSP ∈ NP，TSP 就是 NP-complete。投影片「We have shown that TSP is NP-complete」正是這個意思。

**NP-completeness 與 NP-hardness**：
- NP-completeness 是 decision problem 的 **worst-case** 分析。
- L 是 NP-complete 若：L ∈ NP，且（**NP-hard**）對每個 L' ∈ NP，L' ≤P L。
- 只滿足第二個條件（不一定在 NP）就叫 **NP-hard**。
- 設 L ∈ NPC：若 L ∈ P，則每個 NP 問題都有 polynomial-time 演算法（P = NP）；若 L ∉ P，則任何 NPC 問題都沒有 polynomial-time 演算法（P ≠ NP）。
- 【口頭】四十多年前這是很紅的問題。

投影片：**Studying algorithms is exciting. It can be applied to lots of areas. Down the road you can make a difference in the environment/industry you are in.**

---

## 11. 怎麼對付 NP-hard 問題

[1:08:21]–[1:13:18]　投影片 Coping with NP-hard problems、0/1 Knapsack Problem；課本 §4.4（p.208–）

| 方法 | 說明 | 例子 |
|---|---|---|
| **Approximation algorithms** | 保證離最佳解在固定百分比以內 | 用 minimum spanning tree 近似 **minimum Steiner tree** |
| **Pseudo-polynomial time algorithms** | complexity 長得像 polynomial，但不是問題大小的 polynomial | 0-1 knapsack 的 O(nW) |
| **Restriction** | 只處理原問題的一個子集 | DAG（有向**無環**圖）上的 longest path |
| **Exhaustive search／Branch and bound** | 只有問題很小時可行 | |
| **Local search** | simulated annealing（hill climbing）、genetic algorithm 等 | |
| **Heuristics** | 不保證效能 | |

- 【口頭】
  - approximation 很給人信心（例如保證 10%、15% 以內），但很難證明。minimum Steiner tree 是 NP-hard，用 MST 做出來的答案勉強可以，但想知道差距多少；Steiner tree 對 routing 非常重要（[第 5 支](lecture05_note.md)）。
  - knapsack 的 O(nW) 很有效率，但 W 是背包容量、不是問題大小，所以不算 polynomial time。
  - longest path 限制在 DAG 上（事先知道沒有 cycle）就好解。
  - heuristic 在實務上的效果不一定抓得到最佳解。

**0/1 Knapsack Problem**：N 個物品，各有重量和 profit；背包最多裝重量 W。要放哪些物品讓 profit 總和最大？每個物品只能放或不放（0/1）。
- 【口頭】像帶東西去市場賣，包包裝不下，要讓帶回來的 profit 最多。有很好的 **dynamic programming** 解法，投影片附了 NTNU 和 GeeksforGeeks 的連結。

---

## 12. Minimum spanning tree 與 Prim

[1:13:18]–[1:19:00]　投影片 Minimum Spanning Tree、Prim 例子；課本 §4.3（p.185–）

- **MST**：連通、邊有權重的**無向**圖中，連接所有頂點、**沒有 cycle**、總權重最小的邊子集。
- **唯一性**：每條邊權重都不同的話，MST 唯一。
- 【口頭】如果圖不連通，每個連通塊都要找生成樹；樹從哪一點當 root 都沒關係。

**Prim's algorithm**（Jarník 1930 發明，Prim 1957、Dijkstra 1959 重新發現）：
- 一次長一條邊。一開始 T 只有任意一個頂點。
- 每一步加入權重最小的邊 (x, y)，其中 x 已在 T、y 還不在 T。
- 由 **cut property**，加入的邊都屬於 MST。
- 時間 O(E log V) 或 O(E + V log V)，看用什麼資料結構。
  - 【口頭】最直接的寫法是 O(V²)。這是在 input size 的 polynomial，和 knapsack 的 nW 不一樣。

**投影片例子**（「請忽略方向」）：老師從右上角一點開始，每次挑接到目前樹的最小邊：5 → 6 → 5 → 3 → 4 → 8，得到 MST，**總權重 31**。
- 【口頭】每一步都看「接到目前這棵樹的所有邊」，例如第二步看到 10、9、6，就挑 6。

---

## 13. Backtracking 與 branch-and-bound（TSP 例子）

[1:19:00]–[1:29:54]　投影片 Backtracking Search – Exhaustive、Another Stopping Criterion、Exhaustive Search v.s. Branch and Bound（Gerez Fig. 5.3–5.4）；課本 §4.4.1

TSP 例子（Gerez Fig. 5.3），6 個城市 A–F，邊與距離：

| 邊 | A–B | A–E | A–F | B–C | B–F | C–D | C–F | D–E | D–F | E–F |
|---|---|---|---|---|---|---|---|---|---|---|
| 距離 | 9 | 3 | 5 | 5 | 4 | 2 | 8 | 1 | 7 | 5 |

（A–C、A–D、B–D、B–E、C–E 沒有邊。邊權是依投影片圖讀出，並用 search tree 葉子的 cost 驗證過，見 [§19](#19-實測)。）

**Backtracking（exhaustive）**：
- 從 A 出發，依序展開（f₁ … f₇ 是 tour 的第 1 到第 7 個城市）。
- 第一條：A → B → C → D → E → F → A，cost 9 + 5 + 2 + 1 + 5 + 5 = **27**。
- 走到底後**退一步**（backtrack），看有沒有其他分支可走；X 表示**因為要拜訪某城市第二次而停止**。
- 走完所有可能，葉子 cost 是 27、31、33、27、20、27、33、31、20、27，最小是 **20**。
  - 【口頭】才幾個城市就要走這麼多條路，complexity 不夠好，應該有更好的方法。

**另一個停止條件**：**部分路徑的總 cost ≥ 目前找到的最佳 cost**，就不必往下。

**Branch-and-bound**：在 backtracking 加上「殺掉部分解」的修改。
- 圖上節點標示「已走 cost + 剩下估計」，例如 23 + 8 = 31 已經超過 27，就不往下；22 + 9、21 + 6、14 + 10、11 + 9、8 + 16、5 + 15 等。
- 走到 cost 20 那條時記住它，其他更差的全丟掉。
- 【口頭】搜尋樹比 exhaustive 小，所以快一點。

**其他問題**：
- 如果有些邊權是**負的**怎麼辦？【口頭】那就必須全部走下去才看得到，另外頭痛。
- 有些程式要求圖是 **complete**。「騙」它的方法：缺的邊給「無限大」的權重。【口頭】例如 A 到 C 沒有邊，就給一個非常大的數，TSP 就不會選它；以前老師出過這種程式作業，網路上找的 search 程式常遇到這問題。

---

## 14. DFS 與 BFS

[1:29:54]–[1:36:54]　投影片 Depth first search、Breadth first search（Gerez Fig. 3.9–3.12）；課本 §4.3.1

有向圖（Fig. 3.9）：e₁ = (v₁, v₂)、e₂ = (v₁, v₃)、e₃ = (v₂, v₄)、e₄ = (v₄, v₃)、e₅ = (v₂, v₅)，用 **adjacency list** 表示（每個頂點一串 linked list 存出去的邊）。

**DFS**（Fig. 3.10）：
```
dfs(v1)
  → e1 = (v1, v2): dfs(v2)
       → e3 = (v2, v4): dfs(v4)
            → e4 = (v4, v3): dfs(v3)
       → e5 = (v2, v5): dfs(v5)
  → e2 = (v1, v3)   （v3 已拜訪）
```

**BFS**（Fig. 3.12）：

| Q | w | 處理的邊 |
|---|---|---|
| (v₁) | v₁ | e₁ = (v₁, v₂)、e₂ = (v₁, v₃) |
| (v₂, v₃) | v₂ | e₃ = (v₂, v₄)、e₅ = (v₂, v₅) |
| (v₃, v₄, v₅) | v₃ | — |
| (v₄, v₅) | v₄ | e₄ = (v₄, v₃) |
| (v₅) | v₅ | — |

- 【口頭】BFS 用 queue，first in first out：pop 出 v₂，把它的邊能到的新點放進 queue。
- 【口頭】前面 TSP 的 exhaustive 搜尋樹，一層一層展開看起來有點像 BFS。

---

## 15. 演算法典範與 DP vs divide-and-conquer

[1:36:54]–[1:43:00]　投影片 Algorithmic Paradigms、DP v.s. Divide-and-Conquer

| 典範 | 說明 |
|---|---|
| Exhaustive search | 搜尋整個解空間 |
| Branch and bound | 有剪枝的搜尋 |
| Greedy method | 每一步挑局部最佳 |
| **Dynamic programming** | 把問題切成子問題、解子問題、合併；**適用於子問題不獨立的情況** |
| Hierarchical approach | divide-and-conquer |
| Mathematical programming | 在限制條件下解目標函數 |
| Simulated annealing | 自適應、迭代、非確定的演算法，允許「uphill」的移動來跳出 local optimum |
| Tabu search | 類似 SA，但整個搜尋過程不降低 uphill 移動的機會 |
| Genetic algorithm | 保存一群解，透過 mutation、crossover 等一代代演化 |

- 【口頭】simulated annealing 像逐漸 cooling，有 randomness 讓 uphill move 可以發生，用來逃出 local minimum。

**DP vs Divide-and-conquer**：都靠合併子問題的解。
- Divide-and-conquer：把問題切成**獨立**的子問題，遞迴解，再合併（例如 Quicksort）。**如果同一個子問題被解很多次就沒效率。**
- DP：適用於子問題**不獨立**；**每個子問題只解一次**。
- 圖：1..4 的子問題樹，同顏色的子問題（如 2..2、3..3、1..1）重複出現。

---

## 16. Transportation 例子：backtracking、DP、UCS

[1:43:00]–[2:04:18]　投影片 Transportation example、Exploring Solution Space、Backtracking search、Tree search algorithms、Dynamic programming、Uniform cost search（取自 Stanford CS221 影片）

**問題**：街道 block 編號 1 到 n。走路 s → s+1 花 1 分鐘；搭魔法電車 s → 2s 花 2 分鐘。從 1 到 n 最少花多久？
- 方法：backtracking、DFS、BFS、DP、uniform cost search。
- 【口頭】走 3 格要 3 分鐘，但從 3 搭電車到 6 只要 2 分鐘；從 6 可以搭到 12。老師覺得這個例子很好。

**探索解空間**（投影片）：

| n | 路線 | 時間 |
|---|---|---|
| 10 | 一路走 | 9 |
| 10 | 1, 2, 4, 8, 9, 10 | 7（1+2+2+1+1） |
| 10 | 1, 2, 3, 4, 5, 10 | **6**（1+1+1+1+2） |
| 20 | 一路走 | 19 |
| 20 | 1, 2, 4, 8, 16, 17, 18, 19, 20 | 13 |
| 20 | 1, 2, 4, 5, 10, 20 | **8**（1+2+1+2+2） |

- 影片參考：CS221 影片 18:10 起講 transportation；23:20 起 backtracking；50:57 起 DP；Python 程式 `trans1.py`（不調高 recursion limit 的話，N = 400 會失敗）。

**Backtracking search**：每個 state 有 b 個 action、最大深度 D：
- Memory：O(D)（小）
- Time：O(b^D)（巨大）——2⁵⁰ = 1,125,899,906,842,624
```
def backtrackingSearch(s, path):
    If IsEnd(s): update minimum cost path
    For each action a ∈ Actions(s):
        Extend path with Succ(s, a) and Cost(s, a)
        Call backtrackingSearch(Succ(s, a), path)
    Return minimum cost path
```

**Tree search 演算法比較**（b：每 state 的 action 數、d：解的深度、D：最大深度）：

| 演算法 | Action cost | Space | Time |
|---|---|---|---|
| Backtracking | any | O(D) | O(b^D) |
| DFS | zero | O(D) | O(b^D) |
| BFS | 常數 ≥ 0 | O(b^d) | O(b^d) |
| DFS-ID | 常數 ≥ 0 | O(d) | O(b^d) |

- **永遠是 exponential time**；用 **DFS-ID**（iterative deepening：限定深度的 DFS，有點像 BFS）避免 exponential space。
- 投影片註記：「DFS cost = 0 was given in this problem」——DFS 要求 action cost 是 0（找到任何解就停）。
- 【口頭】iterative deepening 例如限定深度只 search 到 5。

**我們注意到的事**：從 1 到某個 state S 有很多條路；一旦從某條路到了 S、算出 S 到 n 的 cost，就把它**存起來**；下次從另一條路到 S，不用再算 S → n。

**Dynamic programming**：
$$\text{FutureCost}(s) = \begin{cases} 0 & \text{IsEnd}(s) \\ \min_{a \in \text{Actions}(s)} [\text{Cost}(s,a) + \text{FutureCost}(\text{Succ}(s,a))] & \text{otherwise} \end{cases}$$

```python
def dynamicProgramming(problem):
    cache = {}  # state -> futureCost(state)

    def futureCost(state):
        # base case
        if problem.isEnd(state):
            return 0
        if state in cache:  # exponential saving
            return cache[state]
        # actually doing work
        result = min(cost + futureCost(newState)
                     for action, newState, cost in problem.succAndCost(state))
        cache[state] = result
        return result

    return (futureCost(problem.startState()), [])
```
（投影片上的程式；a 是 action，Succ 是採取 action 後的 successor。）

- **Key idea：state** 是「過去所有 action 的摘要，足以**最佳地**選擇未來的 action」。例：TSP 的 DP 裡，state 不是過去走過的序列而是**目前所在城市**（過去 1 3 4 6 → state 6）。**時間和空間都省下 exponential 倍。** 投影片註：類似控制理論的 causality。
  - ⚠️ 這張投影片的 DP 圖是一個 DAG（只能往編號大的城市走），所以「目前城市」夠當 state；一般的 TSP 每城恰訪一次，state 還要記「已拜訪的集合」。投影片沒說明，這裡依投影片的 DAG 圖補充。

**有 cycle 時**：DP 不能用（會無窮遞迴），用 **uniform cost search（UCS）**。白板例子：A–B 1、A–C 100、B–C 1、B–D 100、C–D 1。frontier／explored 的變化：A 0 → B 1、C 100 → C 改成 2 → D 3。

**UCS [Dijkstra, 1956]**：
```
Add s_start to frontier (priority queue)
Repeat until frontier is empty:
    Remove s with smallest priority p from frontier
    If IsEnd(s): return solution
    Add s to explored
    For each action a ∈ Actions(s):
        Get successor s' ← Succ(s, a)
        If s' already in explored: continue
        Update frontier with s' and priority p + Cost(s, a)
```

**CS221 的問題**：從城市 1 走到 n（只往前）再回到 1（只往後），$c_{ij} \ge 0$。DFS、BFS、DP、UCS 哪些可以找最小 cost 路徑？（課堂問題，筆記不給答案。）

---

## 17. Bin packing：FFD 與 ILP

[2:04:18]–[2:11:30]　投影片 Example: Bin Packing、Algorithms for Bin Packing、First-Fit Decreasing、From Wikipedia、ILP Formulation

**Bin-packing problem**：物品 U = {u₁, …, uₙ}，uᵢ 的大小是整數 sᵢ；一組 bin，每個容量 b。**把所有物品裝進去，使用的 bin 數最少（NP-hard！）**。
- 例：b = 6，$\vec s$ = (1, 4, 2, 1, 2, 3, 5)（這些是物品**大小**，不是編號；共 7 個）。最佳解 3 個 bin：{2, 4}、{3, 2, 1}、{5, 1}。
- 應用：裝資源回收桶、裝卡車、錄混音帶（歷史應用）。
  - 【口頭】卡車最多只能載幾個回收桶。

**演算法**：
- Greedy approximation：**First-Fit Decreasing（FFD）**——$FFD(\Pi) \le \frac{11}{9}OPT(\Pi) + 4$。物品由大到小排序，一個個放進第一個放得下的 bin。
- DP？Hierarchical approach？Genetic algorithm？…
- Mathematical programming：用 **integer linear programming（ILP）**，以 |B| 個 bin 找解，再找最小的可行 |B|。

**FFD 例子**：排序 → 5, 4, 3, 2, 2, 1, 1。

| 步驟 | 物品 | 放到 | 狀態（剩餘） |
|---|---|---|---|
| 1 | 5 | B1 | B1: 5 (1) |
| 2 | 4 | B2 | B2: 4 (2) |
| 3 | 3 | B3 | B3: 3 (3) |
| 4 | 2 | B2 | B2: 4, 2 (0) |
| 5 | 2 | B3 | B3: 3, 2 (1) |
| 6 | 1 | B1 | B1: 5, 1 (0) |
| 7 | 1 | B3 | B3: 3, 2, 1 (0) |

結果 3 個 bin，和最佳解相同。

**Wikipedia 的分類**：
- Online heuristics：next fit、next-k fit、first fit、best fit、worst fit、almost worst fit、refined first fit…
- Offline heuristics（multiplicative approximation）：first fit decreasing、next fit decreasing、modified first fit decreasing…
- 投影片：**如果有些物品不能放在同一個 bin？** **有時候不能事先排序。**（後者就是 online 的情況。）

**ILP formulation**：0-1 變數 $x_{ij} = 1$ 若物品 uᵢ 放在 bin bⱼ，否則 0。

$$\max \sum_{(i,j)\in E} w_{ij}x_{ij}$$
subject to
$$\sum_{\forall i\in U} w_{ij}x_{ij} \le b_j,\ \forall j \in B \quad \text{(capacity)}$$
$$\sum_{\forall j\in B} x_{ij} = 1,\ \forall i \in U \quad \text{(assignment)}$$
$$\sum_{ij} x_{ij} = n \quad \text{(completeness)}$$
$$x_{ij} \in \{0,1\}$$

- Step 1：|B| 設成 bin 數的**下界**。Step 2：用 ILP 找可行解。Step 3：有解就是 |B| 個 bin，結束。Step 4：否則 |B| ← |B| + 1，回到 Step 2。
- 投影片：**How to guess the lower bound?**（一個簡單的下界是 ⌈Σsᵢ / b⌉，見 [§19](#19-實測)。）

---

## 18. Recap

[2:11:30]　投影片 Recap

- 怎麼把一張圖的 HC 問題 reduce 到 TSP。
- MST 演算法。
- 用 TSP 說明 exhaustive search、backtracking（BT）、branch-and-bound；DFS、BFS、DP、uniform cost（UC）。
- 用 transportation 問題說明 BT、DP、UC——有 Python 程式，歡迎研讀。
- Bin packing。

---

## 19. 【實測】

程式：`experiments/lecture03/search.py`，輸出：`experiments/lecture03/output.txt`。

**(1) TSP：exhaustive vs branch-and-bound**（Gerez Fig. 5.3 的圖，從 A 出發、鄰居照字母順序）：

```
TSP exhaustive     : best 20 via AEDCBFA; search nodes 62; complete tours 10 costs [20, 20, 27, 27, 27, 27, 31, 31, 33, 33]
TSP branch-and-bound: best 20 via AEDCBFA; search nodes 60; complete tours 6 costs [20, 20, 27, 27, 27, 27]
```

- 10 條完整 tour 的 cost 正好是投影片 search tree 葉子的那 10 個數（27、31、33、27、20、27、33、31、20、27）——這也驗證了 §13 從圖上讀出的邊權。最佳 tour A–E–D–C–B–F–A = 3 + 1 + 2 + 5 + 4 + 5 = 20（每條 tour 正反各算一次，所以 20 出現兩次）。
- branch-and-bound 只完成 6 條 tour（31、33 那 4 條在中途就被砍掉）。這個小例子搜尋節點只從 62 降到 60，因為照字母順序第一條就是 27，bound 不夠緊；投影片的 B&B 樹加上了「剩餘 cost 的估計」，剪得更多。

**(2) Transportation**：

```
transport n=10: DP 6  UCS 6  path (1, 2, 3, 4, 5, 10)
transport n=20: DP 8  UCS 8  path (1, 2, 3, 4, 5, 10, 20)
transport n=100: DP 13  UCS 13  path (1, 2, 3, 6, 12, 24, 25, 50, 100)
transport n=400: DP RecursionError (default limit 1000)  UCS 17  path (1, 2, 3, 6, 12, 24, 25, 50, 100, 200, 400)
```

- n = 10、20 的最佳值 6 和 8 和投影片一致（n = 20 有兩條同樣 8 分鐘的路：1,2,4,5,10,20 和 1,2,3,4,5,10,20）。
- **n = 400 時遞迴版 DP 爆掉 Python 預設的 recursion limit**——正是投影片「comment out recursion limit → then, N=400 will fail」說的現象（futureCost 會遞迴 s, s+1, …，深度到 ~400 層，每層還有 generator 的 frame）。UCS 是迴圈版，不受影響。

**(3) Bin packing**：

```
FFD (1, 4, 2, 1, 2, 3, 5) cap 6: 3 bins [[5, 1], [4, 2], [3, 2, 1]]; exact optimum 3; lower bound ceil(sum/cap) = 3
```

- FFD 得到 3 個 bin，和投影片手算一樣；暴力窮舉的最佳值也是 3。
- 下界 ⌈18/6⌉ = 3，所以 ILP 流程從 |B| = 3 開始第一次就成功。

**(4) HC → TSP**：

```
HC->TSP (slide graph): (5, (1, 4, 3, 2, 5))   bound B = n = 5
HC->TSP (5-cycle with one edge removed): (6, (1, 4, 3, 2, 5))
```

- 有 HC 的圖，轉換後 TSP 最短 tour = 5 = B（tour 是 ⟨1,5,2,3,4,1⟩ 反過來走）；拿掉一條邊變成沒有 HC，最短 tour = 6 > B。正好是 §10 Step 2 的兩個方向。

---

## 20. 一頁速查表

| 主題 | 重點 |
|---|---|
| Big-O | f ≤ c·g（n ≥ n₀）；upper bound；worst case |
| 輸入大小 | n 個字 → n；整數 n → lg n；圖 → \|V\|, \|E\| |
| Polynomial | 1、lg n、√n、n、n lg n、n²、n³；非 polynomial：2ⁿ、n! |
| 1000 MIPS 下 | n² 在 10⁶ 是 16.7 分；2ⁿ 在 100 已是天文數字 |
| Optimization vs decision | 求最佳 cost vs 回答 cost ≤ k 嗎；可用 binary search 互轉 |
| P | polynomial time 可解；tractable；MST ∈ P |
| NP | polynomial time 可驗證（NTM 可解）；TSP ∈ NP |
| NP-complete | ∈ NP 且所有 NP 問題可 ≤P 它；SAT 第一個（Cook–Levin）；Karp 1972 的 21 個 |
| NP-hard | 只要求所有 NP 問題可 ≤P 它；可以不是 decision problem |
| L₁ ≤P L₂ | x ∈ L₁ ⟺ f(x) ∈ L₂，f polynomial；L₂ 至少和 L₁ 一樣難；有遞移性 |
| 證 NPC 五步 | ∈ NP；選已知 NPC L'；建 f；證 iff；證 f polynomial |
| HC ≤P TSP | 有邊距離 1、沒邊距離大數，B = n；O(V²) |
| 對付 NP-hard | approximation、pseudo-polynomial、restriction、B&B、local search、heuristic |
| MST／Prim | 每次加接到樹的最小邊；cut property；O(E log V) |
| Backtracking | 走到底再退一步；X = 城市重複 |
| B&B | 部分 cost ≥ 目前最佳就剪掉 |
| DFS／BFS | stack（遞迴）／queue（FIFO） |
| DP | 子問題不獨立、每個只解一次；FutureCost + cache；state = 足以決定未來的過去摘要 |
| UCS | Dijkstra；priority queue；可處理 cycle（cost ≥ 0） |
| Tree search | backtracking/DFS：O(D) 空間、O(b^D) 時間；BFS：O(b^d)；DFS-ID：O(d) 空間 |
| Bin packing | NP-hard；FFD ≤ 11/9·OPT + 4；ILP 從下界逐次加 bin |

---

## 21. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| Python 示範（transportation 等） | 第 4 支：ch2 Python 範例 |
| MST 近似 minimum Steiner tree | [第 5 支](lecture05_note.md)：Steiner tree |
| BDD、SAT、equivalence checking、DP 做 technology mapping | 第 6 支：ch10 synthesis |
| partitioning（NP-hard 的 heuristic、local search） | 第 8 支：ch4 partitioning |
| floorplanning 的 simulated annealing、B\*-tree | 第 9 支：ch5 floorplanning |
| placement 的 mathematical programming（quadratic placement） | 第 10 支：ch6 placement |
| maze routing（BFS）、A\*、shortest path、Steiner tree | 第 11–12 支：routing |
| bin packing／ILP 的想法 | floorplan packing、global routing 的資源分配 |

---

## 附：本講一句話總結

> **EDA 工具的設計越來越大，演算法的 complexity 決定程式跑不跑得完：big-O 是 worst-case 的 upper bound，在 1000 MIPS 下 n² 到 10⁶ 要 16.7 分鐘、2ⁿ 到 100 就是天文數字；問題分成 optimization 和 decision 兩種，P 是 polynomial time 可解、NP 是 polynomial time 可驗證，NP-complete 是在 NP 裡而且所有 NP 問題都能 polynomial-time reduce 過來的問題（SAT 第一個、Karp 1972 再證 21 個），NP-hard 則不必是 decision problem；證明 NP-complete 要從已知的 NPC 問題 reduce 到新問題，例如 HC ≤P TSP（有邊給 1、沒邊給大數、B = n），這證的是 TSP 的難度而不是 HC 的（老師也指出投影片這裡有點 misleading）；對付 NP-hard 問題可以用 approximation（MST 近似 Steiner tree）、pseudo-polynomial（knapsack 的 O(nW)）、restriction、branch-and-bound、local search 或 heuristic；演算法部分講了 Prim 的 MST、TSP 的 backtracking 與 branch-and-bound（本機實測最佳 tour 20，和投影片一致）、DFS/BFS、子問題不獨立時用的 DP（state 是足以決定未來的過去摘要；遞迴版在 n = 400 時真的撞到 recursion limit）、有 cycle 時用的 UCS，以及 bin packing 的 FFD（11/9·OPT + 4）和 ILP 解法。**

---

*筆記依據李炯霆「Electronic Design Automation (1)」第3講｜ch2.complexity 錄影（NTHU OCW，2:12:40；影片無字幕，以 whisper large-v3-turbo 轉錄）、投影片截圖（Unit 1A）、Wang, Chang, Cheng（eds.）*Electronic Design Automation: Synthesis, Verification, and Test* Ch4 整理。投影片部分例子取自 Gerez *Algorithms for VLSI Design Automation* Ch3、Ch5 與 Stanford CS221。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
