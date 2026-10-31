# Advanced Computer Architecture 第五十五支影片完整筆記：機器學習硬體架構（一）：機器學習基礎、非線性活化與卷積神經網路（CNN）（Chapter 14, Part I）

> **課程**：Advanced Computer Architecture — Chapter 14: Architectures for Machine Learning (Part I) -- Basic ML Concepts（播放清單第 55 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（1:14:37）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch14** §14.1（p.703–711）
> **章號**：影片 Chapter 14 = 課本 Ch14。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、NVIDIA GeForce RTX 4060 Laptop GPU、`nvcc` 12.4、Ubuntu、kernel 6.5.0）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕把「face recognizer」寫成「phase recognizer」、「we have some」寫成「vvsm」、「ReLU」寫成「relu／relo／value／radio」、「XOR / XOR gate」寫成「zor／zord／zot／zorgate」、「back edges」寫成「packages」、「an FU / FUs」寫成「a few／fuse」、「ifmap / ofmap / the ifmap」寫成「if map／web map／of map／tie F map」、「pooling」寫成「pulling／pulley」、「merge」寫成「moist」、「neuromorphic」寫成「euromorphic」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 為什麼計算機結構最後一章要專講機器學習？](#1-為什麼計算機結構最後一章要專講機器學習)
- [2. 機器學習本質：隱藏函數 $f^*$、曲線擬合限制與萬有逼近器（Universal Approximator）](#2-機器學習本質隱藏函數-f曲線擬合限制與萬有逼近器universal-approximator)
- [3. 線性模型與幾何投影：線性迴歸與支援向量機（SVM + Kernel Trick）](#3-線性模型與幾何投影線性迴歸與支援向量機svm--kernel-trick)
- [4. 神經元與非線性活化函數：Sigmoid、$\tanh$ 與硬體友善的 ReLU](#4-神經元與非線性活化函數sigmoidtanh-與硬體友善的-relu)
- [5. 經典範例：三層 ReLU 網路解 XOR 與「線性層不可相鄰」鐵律](#5-經典範例三層-relu-網路解-xor-與線性層不可相鄰鐵律)
- [6. 深度神經網路（DNN）：訓練 vs 推論，以及為何 2016 年後才大爆發？](#6-深度神經網路dnn訓練-vs-推論以及為何-2016-年後才大爆發)
- [7. 卷積神經網路（CNN）完整拆解：ReLU、Pooling、FC 與多通道 2D 卷積](#7-卷積神經網路cnn完整拆解relupoolingfc-與多通道-2d-卷積)
- [8. 【實測】XOR 線性塌縮對照、GPU 超越函數代價與 CNN 層級權重／算力剖析](#8-實測xor-線性塌縮對照gpu-超越函數代價與-cnn-層級權重算力剖析)
- [9. 一頁速查表](#9-一頁速查表)
- [10. 與後續講次的連結](#10-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
為什麼通用 OOO CPU 撐不起現代 AI，必須設計領域專用硬體（ML Accelerators）？（Ch14 四講總綱）
  │
  ├── 第一步：搞懂「機器學習到底在算什麼？」（§14.1.1–14.1.2）
  │   ├── 目標：不寫死規則，從大量標註資料（Ground Truth）學習未知的隱藏連續函數 f*(x)
  │   ├── 早期局限：多項式／指數曲線擬合需人工猜函數形式 → 需要不預設形式的「萬有逼近器（Universal Approximator）」
  │   ├── 線性基礎：線性迴歸 y = w^T x + b（核心運算＝向量內積 Dot Product！）
  │   │             ＋ SVM 核技巧（Kernel Trick）：非線性投影至高維空間後找線性分離超平面（最近點＝Support Vectors）
  │   └── 神經網路積木：線性內積 + 非線性活化 g(w^T x + b)
  │       ├── Sigmoid / tanh：需算硬體昂貴的指數 e^{-x}，且在 ±3 或 ±2 外飽和
  │       ├── ReLU = max(0, x)：檢查正負號位元即可！硬體幾乎零成本
  │       └── XOR 鐵證：a ⊕ b = |a - b| = ReLU(a - b) + ReLU(b - a)；兩層線性層直接相接必塌縮成單一線性層 W2·W1！
  │
  └── 第二步：現代加速器頭號標靶——卷積神經網路（CNN）的推論運算特徵（§14.1.3）
      ├── Training（離線反向傳播，週期長、重彈性） vs Inferencing（線上即時預測，2020s 硬體加速器主戰場！）
      ├── 非線性層（0 權重、計算極輕）：ReLU ＋ K×K Max/Avg Pooling（資料量縮小 K^2 倍並賦予平移／縮放容忍度）
      └── 線性層（吃掉幾乎全部參數與算力，但性格截然相反！）：
          ├── 全連接層（FC）：每個輸入配專屬權重 → 參數肥大（吃記憶體頻寬的「白象」），僅留最後 1–2 層（占時間 ~10–15%）
          └── 卷積層（CONV2D）：R×S 小濾波器滑窗共用權重，C 個輸入 ifmap × K 個輸出 ofmap 共 C×K 個濾波器
                                → 參數極省但吃掉 >85–90% 總執行時間！成為第 56–58 講硬體加速的絕對主角
```

---

## 1. 為什麼計算機結構最後一章要專講機器學習？

[00:14]（課本 Ch14 開場，p.703–704）
- **先修基礎** `[00:34]`：進入第 14 章前，我們其實已經湊齊所有硬體積木——循序管線（in-order pipelines）、第 6 章的 **GPU 架構**（[第 18 支](lecture18_note.md)–[第 20 支](lecture20_note.md)）與第 9 章的**多處理器記憶體系統**。`【口頭】` `[00:54]` 老師提到第 14 章是課本第一版的壓軸最後一章（影片錄製於 2022 年 `[40:52]`）。
- `【課本補充】`（p.703 趣味自白）：Sarangi 教授在課本章首透露——**這本厚達七百多頁的課本，絕大多數文字其實是他對著深度神經網路（DNN）語音辨識系統「口述」寫出來的**！從垃圾郵件過濾、人臉辨識、Google 翻譯、自駕車到工廠瑕疵檢測，原本人類寫不出明確 `if-else` 規則的任務，全被機器學習接管。
- **全章五大主題藍圖** `[01:17]`：(1) ML 基礎與 CNN 運算結構（本講）；(2) 多重迴圈與矩陣乘法數學轉換（第 56 講）；(3) 脈動陣列（Systolic Arrays）與資料流硬體架構（第 57 講）；(4) 稀疏修剪、量化、Winograd/FFT 與記憶體內計算（第 58 講）。

---

## 2. 機器學習本質：隱藏函數 $f^*$、曲線擬合限制與萬有逼近器（Universal Approximator）

[02:29]（課本 §14.1–14.1.1，p.704–705）
- **隱藏函數（Hidden Function）$f^*$ 與標準答案（Ground Truth / Golden Data）** `[02:51]`：
  - 不管是辨識照片裡的特定人臉（輸入百萬像素灰階矩陣 $\mathbf{X}$，輸出 `YES=1 / NO=0`）、把英文翻成法文或印地文 `[04:19]`，還是預測未來兩小時降雨機率 `[06:22]`，背後都存在一個未知的理想數學映射 $y = f^*(\mathbf{x})$。
  - 人類專家無法寫出百萬像素的解析公式，但人類看一眼照片就知道是不是那個人——這些人工標註好的正確輸入輸出配對 $(\mathbf{x}_i, y_i)$ 就稱為 **Ground Truth（或 Golden Data）** `[05:13]`。機器學習的目標，就是找出一組參數化逼近函數 $f(\mathbf{x}) \approx f^*(\mathbf{x})$，使其在未見過的新輸入上也能精準預測。
- **為什麼 1980–90 年代的傳統曲線擬合（Curve Fitting）走入死胡同？** `[07:09]`：
  - 早期統計與氣象模型會先「人工假設函數長相」（例如多項式 $y = a + bx + cx^2$ 或指數 $a e^{bx}$），再用最小平方法解係數。一旦高維度真實資料根本不符合你猜的多項式或指數形式，擬合就徹底失敗 `[09:16]`。
- **萬有逼近器（Universal Approximator）** `[09:49]`（課本 Definition 104，p.705）：
  - 我們真正渴望的黑盒子必須滿足三個條件：
    1. 能以任意指定精度逼近 $\mathbb{R}^n$ 緊緻子集上的**任何連續函數**（課本註記：雖無法精確表達不連續跳躍函數，但可用斜率極陡的連續斜坡任意逼近 p.705）；
    2. **不需事先假設（no *a priori* assumption）** 目標函數的形式；
    3. 完全由**有限個可學習參數**決定。
  - 多層前饋神經網路（Multilayer Feed-forward Neural Networks）在數學上已被證明正是這樣的萬有逼近器！

---

## 3. 線性模型與幾何投影：線性迴歸與支援向量機（SVM + Kernel Trick）

[11:26]（課本 §14.1.1，p.705–706）
- **線性迴歸（Linear Regression）與硬體最核心的「內積」** `[11:36]`（課本式 14.1）：
  $$y = \mathbf{w}^T \mathbf{x} + b = \sum_{i=1}^{n} w_i x_i + b$$
  - 其中 $\mathbf{x} \in \mathbb{R}^{n \times 1}$ 是輸入行向量（⚠️ 影片 `[12:44]` 老師口誤把行向量 $\mathbf{x}$ 說成「$n \times n$ 矩陣」，隨即在投影片上寫出正確的單列乘單行點積；應為 $n \times 1$ 行向量），$\mathbf{w} \in \mathbb{R}^{n \times 1}$ 是權重向量，$b$ 是純量偏差（bias）。
  - 從硬體架構師的視角看，這個式子就是整章無所不在的原子操作：**乘積累加（Multiply-Accumulate, MAC）組成的向量內積（Dot Product）** `[13:19]`！
- **支援向量機（SVM, Support Vector Machines）與核技巧（Kernel Trick）** `[14:49]`（課本圖 14.1，p.706）：
  - 假設 2D 平面上有藍點與綠點兩類資料，但邊界彎曲，畫不出任何一條直線將兩者分開（線性不可分）`[15:23]`。
  - **升維投影魔法** `[16:33]`：透過非線性座標轉換（例如把 $(x_1, x_2)$ 映射成三維 $(f_1(\mathbf{x}), f_2(\mathbf{x}), f_3(\mathbf{x}))$），原本在 2D 糾纏的點雲在 3D（或更高維）空間中往往就能用一張平坦的**線性超平面（separating hyperplane）** 乾淨切開！
  - 最貼近這張分離超平面、直接決定平面位置與方向的少數邊界資料點，就叫做**支援向量（Support Vectors）** `[18:56]`；而高維映射函數就是 **Kernel Function**。不過課本 p.706 點出：要為複雜影像或語音挑對 Kernel 依然高度仰賴人工經驗，因此歷史舞台最終交棒給能自動學習特徵的神經網路。

---

## 4. 神經元與非線性活化函數：Sigmoid、$\tanh$ 與硬體友善的 ReLU

[20:12]（課本 §14.1.2，p.706–707）
- **單一神經元計算單元（Neural Unit）** `[20:18]`：先做線性加權和 $z = \mathbf{w}^T \mathbf{x} + b$，再套上一個純量**非線性活化函數（nonlinear activation function）** $y = g(z)$。常見三種活化函數如下：

| 活化函數 | 數學定義（課本式 14.2–14.4） | 值域與敏感區間 | 硬體實作代價 `[22:55]` |
|---|---|---|---|
| **Sigmoid** | $\sigma(z) = \dfrac{1}{1 + e^{-z}}$ | 值域 $(0, 1)$；主要變化集中在 $[-3, 3]$，兩端飽和趨近 $0$ 或 $1$ | **昂貴**：需計算超越函數 $e^{-z}$ 與浮點除法 |
| **雙曲正切 $\tanh$** | $\tanh(z) = \dfrac{e^z - e^{-z}}{e^z + e^{-z}}$ | 值域 $(-1, 1)$；斜率更陡，主要變化集中在 $[-2, 2]$ | **昂貴**：同樣需計算指數超越函數與除法 |
| **ReLU**（Rectified Linear Unit） | $f(z) = \max(0, z)$ | $z < 0$ 輸出 $0$；$z \ge 0$ 原封不動輸出 $z$（如同電子學半波整流器 `[23:39]`） | **極度廉價**：硬體只需檢查浮點／整數的 **Sign Bit（正負號位元）**，為負就清零！ |

---

## 5. 經典範例：三層 ReLU 網路解 XOR 與「線性層不可相鄰」鐵律

[24:20]（課本圖 14.2，p.707–708）
- **為什麼單層線性分類器連 2-bit `XOR` 都算不出來？** `[24:55]`：
  - 把 `XOR` 四個真值點畫在 $(a, b)$ 平面上：$(0,0) \to 0$ 與 $(1,1) \to 0$ 在一條對角線，$(0,1) \to 1$ 與 $(1,0) \to 1$ 在另一條對角線——**在 2D 平面上絕對畫不出任何一條直線把 `{0}` 跟 `{1}` 分在兩側**！
- **用 `ReLU` 拼出絕對值 $|a - b|$** `[26:39]`：
  - 注意到對二元輸入 $a, b \in \{0, 1\}$，互斥或剛好等於絕對值差：$a \oplus b = |a - b|$！
  - 而對任意實數 $x$，絕對值都能拆成兩個半波整流之和：$|x| = \text{ReLU}(x) + \text{ReLU}(-x)$ `[27:23]`！
  - 因此課本圖 14.2 構造出極簡三層網路（輸入層 $\to$ 線性層 $\to$ ReLU 非線性層 $\to$ 線性輸出層）：
    1. **第一線性層**：兩個隱藏節點分別設定權重 $\mathbf{w}_{1,1}^T = [1, -1]$ 與 $\mathbf{w}_{1,2}^T = [-1, 1]$（偏差 $b=0$），算出 $z_1 = a - b$ 與 $z_2 = b - a$。
    2. **中間非線性層（ReLU）**：算出 $h_1 = \text{ReLU}(a - b)$ 與 $h_2 = \text{ReLU}(b - a)$。當 $(a,b)=(1,0)$ 時 $(h_1, h_2)=(1, 0)$；當 $(a,b)=(0,1)$ 時 $(h_1, h_2)=(0, 1)$；當 $a=b$ 時兩者皆為 $0$。
    3. **最後線性層**：權重設為 $\mathbf{w}_2^T = [1, 1]$，輸出 $y = h_1 + h_2 = |a - b| = a \oplus b$，四筆輸入 100% 精確命中！
- `【口頭】` **同一個網路拓樸改權重秒變 `AND` 閘** `[31:14]`：
  - 老師在影片中即興示範：完全不用改硬體連線，只要把第一層權重換成 $[0.5, 0.5]$ 與 $[0, 0]$，對四組輸入算出 $0,\ 0.5,\ 0.5,\ 1.0$，若取整數位元（或加上偏差 $b = -1$ 寫成 $\text{ReLU}(a + b - 1)$），同一個網路就變成 `AND` 邏輯閘！這具體展示了萬有逼近器「**固定拓樸結構、僅靠調整參數就能切換成完全不同函數**」的威力。
- **神經網路架構鐵律：永遠不要把兩層線性層直接疊在一起！** `[33:54]`（課本 p.708）：
  - 如果拿掉中間的 ReLU，把兩層線性層 $\mathbf{W}_1$ 與 $\mathbf{W}_2$ 直接串聯，輸出會變成 $\mathbf{y} = \mathbf{W}_2(\mathbf{W}_1 \mathbf{x}) = (\mathbf{W}_2 \mathbf{W}_1)\mathbf{x} = \mathbf{W}_{\text{eff}}\mathbf{x}$——**在數學上直接塌縮（collapse）回單一矩陣乘法**，表達能力跟只有一層一模一樣，永遠解不出 XOR！
  - 相反地，**連續疊兩層非線性層是完全合法且常見的**（例如後面 CNN 裡的 `ReLU` 緊接著 `Max Pooling`），因為兩個非線性函數合成仍是更複雜的非線性函數。

---

## 6. 深度神經網路（DNN）：訓練 vs 推論，以及為何 2016 年後才大爆發？

[34:31]（課本 §14.1.2 後半，p.708）
- **深度神經網路（DNN）拓樸** `[34:45]`：在輸入與輸出之間堆疊數十至數百層線性與非線性層。最主流為無迴圈的**前饋網路（Feed-forward networks）**；`【口頭】` `[35:55]` 老師也補充現代網路會有跨層跳接的 **Skip Connections**（如 ResNet 殘差連線）或帶著狀態記憶的**回授邊（Back edges，如 RNN）**。
- **訓練（Training）vs 推論（Inferencing）** `[36:25]`：
  - **Training（離線學習）**：餵入數百萬筆標註資料（`【口頭】` `[37:44]` 老師提到工業界常用 Amazon Mechanical Turk 群眾外包標註），透過微積分鏈鎖律做**反向傳播（Backpropagation）** 反覆修正數千萬個權重。通常在雲端 GPU 叢集離線跑數小時至數天。
  - **Inferencing（線上預測）**：權重已經固定，拿一張新照片從頭到尾順向算一次得出答案。手機解鎖、自駕車煞車、語音助理都要求毫秒級低延遲與極低功耗——因此 **2020 年代初期絕大多數專用 ML 硬體加速器（Edge TPU、NPU 等）都優先鎖定加速 Inferencing** `[40:37]`！
- `【口頭】` **歷史之問：神經網路與反向傳播早在 1960–80 年代就發明了，為何經歷兩次 AI 寒冬，直到 2016 年前後才席捲全球？** `[38:34]`（課本 p.708 註記轉折點約為 2014 年）：
  1. **算力與記憶體到位**：早期電腦連存幾百萬個權重都捉襟見肘，直到現代 **GPU 與 FPGA** 提供海量平行 MAC 與頻寬，才算得動深層網路。
  2. **海量數位標註資料**：社群網路、智慧手機與網際網路普及後，人類才第一次累積出上億張照片與語音資料。
  3. **商業應用需求成熟**：雲端搜尋、推薦系統與智慧終端創造了龐大變現市場，形成算力與演算法的正向飛輪。

---

## 7. 卷積神經網路（CNN）完整拆解：ReLU、Pooling、FC 與多通道 2D 卷積

[41:01]（課本 §14.1.3，p.708–711）

### 7.1 基本術語與兩類非線性層（ReLU 與 Pooling）

[41:45] 在影像處理導向的 **CNN（Convolutional Neural Network）** 中，單一運算層／算子常稱為 **FU（Functional Unit）**，一張輸入二維特徵矩陣稱為 **ifmap（Input Feature Map）**，輸出二維特徵矩陣稱為 **ofmap（Output Feature Map）**，矩陣中的每個元素稱為 **pixel（像素）** `[42:35]`。
- **非線性層一：ReLU** `[43:29]`：逐像素把負值歸零（$x \mapsto \max(0, x)$），輸出尺寸不變。
- **非線性層二：$K \times K$ Pooling（池化層）** `[43:54]`（課本圖 14.3，p.709）：
  - 在特徵圖上以不相重疊的 $K \times K$ 視窗（常見 $2 \times 2$ 或 $3 \times 3$）滑動，每個視窗濃縮成 **1 個數值**——取最大值稱為 **Max Pooling**，取算術平均稱為 **Average Pooling**。
  - **雙重硬體與演算法好處** `[44:50]`：(1) **資料降維壓縮**：每經過一層 $K \times K$ Pooling，資料面積立刻縮小 **$K^2$ 倍**（例如 $2 \times 2$ 縮小 4 倍），大幅減輕後續層的記憶體與運算負擔；(2) **空間平移／縮放容忍度（Translation & Scale Invariance）**：物體在畫面裡稍微偏移幾個像素，視窗內的最大特徵值依然相近。

### 7.2 兩類線性層的命運對決：全連接層（FC）vs 二維卷積層（CONV2D）

[46:24]（課本圖 14.4–14.5、表 14.1、式 14.5–14.6，p.709–711）
1. **全連接層（Fully Connected Layer, FC / Multilayer Perceptron）** `[46:35]`：
   - 將整張輸入拉平成長向量，**每一個輸出神經元都對每一個輸入像素配一個獨立權重**。
   - **致命痛點（「記憶體白象」`[68:01]`）**：假設輸入是 $1000 \times 1000 = 10^6$ 像素，光產生 **1 個**輸出點就要 $10^6$ 個權重；若輸出也有 $10^4$ 個點，單層權重就暴衝到 $10^{10}$ 個（40 GB）！不僅記憶體塞不下、每個權重在單張圖推論時**完全無法重用（只乘一次就丟）**，參數太多也極難訓練收斂 `[48:04]`。
   - 因此現代 CNN 只在網路最末端、特徵圖已經被縮得很小時保留 1–3 層 FC 做最終分類合併 `[51:59]`，約占總推論時間的 **10–15%** `[50:15]`。
2. **二維卷積層（2D Convolutional Layer, CONV2D）** `[53:12]`：
   - 借鏡數位訊號處理與邊緣偵測濾波器：改用一個尺寸極小的 **$R \times S$ 濾波器（Filter / Kernel，常見奇數尺寸 $3 \times 3, 5 \times 5, 7 \times 7$）**，在整張 $H \times W$ 影像上逐格滑動做局部內積（課本式 14.5）：
     $$O[h][w] = \sum_{r=0}^{R-1} \sum_{s=0}^{S-1} I[h + r][w + s] \times F[r][s]$$
   - `【課本補充】`（p.710 符號差異提醒）：嚴格數學與 DSP 的「卷積（convolution）」會把濾波器翻轉寫成 $I[h - r][w - s]$，而無翻轉的加號版 $I[h + r][w + s]$ 在數學上其實叫**互相關（Cross-correlation）**；但因為濾波器權重都是機器自己學出來的，翻不翻轉毫無影響，深度學習界一律統稱 Convolution。
   - **多通道與多輸出推廣（課本表 14.1 與式 14.6）** `[58:06]`：
     - 彩色圖片天然就有紅綠藍 $C = 3$ 個輸入通道（到了深層網路 $C$ 常達 $64, 128, 512$ 個 ifmaps）。單一卷積單元（FU）會針對這 $C$ 張 ifmap 配備 **$C$ 個不同的 $R \times S$ 濾波器**，把 $C$ 個通道在同一空間位置的內積**全部加總合併成 1 張二維 ofmap**！
     - 若這一層想同時萃取 $K$ 種不同特徵（例如直線、斜線、圓弧、紋理），就平行擺 **$K$ 個獨立的卷積單元（產生 $K$ 張 ofmap，供下一層當成 $C' = K$ 個輸入通道）**。因此這一層總共擁有 **$C \times K$ 個二維濾波器**（總權重數僅 $C \times K \times R \times S$，與影像長寬 $H, W$ 完全無關！）`[62:38]`。
     - 再加上一次批次處理 $N$ 張獨立圖片（Batch Size $= N$），就得到貫穿第 56–58 講最重要的**七重索引核心公式（課本式 14.6）**：
       $$O[n][k][h][w] = \sum_{c=0}^{C-1} \sum_{r=0}^{R-1} \sum_{s=0}^{S-1} I[n][c][h + r][w + s] \times F[k][c][r][s] \quad (1 \le n \le N,\ 1 \le k \le K)$$
   - **邊界零填充（Zero Padding）** `[65:01]`：當 $R \times S$ 視窗滑到影像最右邊或最底端時會超出邊界（若不補零，輸出長寬會縮水成 $(H - R + 1) \times (W - S + 1)$）；實務上常在外圍補上 `0`（Zero Padding），讓輸出 ofmap 維持與輸入相同的 $H \times W$ 尺寸。
   - **硬體架構師的黃金結論** `[70:46]`：在典型 CNN（如 AlexNet / VGG / ResNet）中，**`CONV2D` 層憑藉大量滑窗內積吃掉了 85%–90% 以上的總運算時間**，卻只占極少數權重參數；而末端的 **`FC` 層吃掉超過 90% 的權重記憶體容量**，運算時間約占 10–15%！下一講開始的所有硬體與編譯器魔法，都將圍繞這兩個鮮明特性展開（課本結尾 `[74:12]` 亦預告除了主流數位加速器外，還有模仿生物腦神經脈衝的類神經形態計算 Neuromorphic Computing，但商業主力仍是數位矩陣／卷積加速器）。

---

## 8. 【實測】XOR 線性塌縮對照、GPU 超越函數代價與 CNN 層級權重／算力剖析

我們撰寫 CUDA 實驗程式 [`experiments/lecture55/cnn_layers_and_xor.cu`](experiments/lecture55/cnn_layers_and_xor.cu)，在本機 CPU 與 **NVIDIA GeForce RTX 4060 Laptop GPU**（Ada Lovelace `sm_89`）上直接驗證本講四項核心命題：(1) 課本圖 14.2 的 `XOR` 解析權重與老師 `[31:14]` 口頭補充的 `AND` 權重；(2) 用隨機初始化跑 4,000 輪反向傳播梯度下降，對照「兩層線性直接疊加（無 ReLU）」與「Linear + ReLU + Linear」能否學會 XOR；(3) 在 GPU 上實測超過 10 億次 `ReLU`、`Sigmoid` 與 `tanh` 的真實硬體吞吐量差距；(4) 實作課本式 14.6 多通道卷積、`ReLU`、$2 \times 2$ `MaxPool` 與末端 `FC` 層，量測各層的**權重記憶體占比 vs FLOPs 占比 vs 真實 GPU 耗時占比**。

核心的多通道卷積與活化基準測試節錄如下（`experiments/lecture55/cnn_layers_and_xor.cu:133-161`）：

```cuda
__global__ void conv2d_eq14_6_kernel(const float *__restrict__ I,
                                     const float *__restrict__ F,
                                     float *__restrict__ O,
                                     int N, int C, int H, int W,
                                     int K, int R, int S, int pad) {
    int w = blockIdx.x * blockDim.x + threadIdx.x;
    int h = blockIdx.y * blockDim.y + threadIdx.y;
    int nk = blockIdx.z;
    if (w >= W || h >= H || nk >= N * K) return;
    int n = nk / K, k = nk % K;
    float sum = 0.0f;
    for (int c = 0; c < C; c++) {
        const float *in_nc = I + ((size_t)n * C + c) * H * W;
        const float *f_kc  = F + ((size_t)k * C + c) * R * S;
        for (int r = 0; r < R; r++) {
            int ih = h + r - pad;
            if (ih < 0 || ih >= H) continue;
            for (int s = 0; s < S; s++) {
                int iw = w + s - pad;
                if (iw >= 0 && iw < W)
                    sum += in_nc[ih * W + iw] * f_kc[r * S + s];
            }
        }
    }
    O[((size_t)n * K + k) * H * W + h * W + w] = sum;
}
```

編譯執行 `nvcc -O3 -arch=sm_89 cnn_layers_and_xor.cu` 的完整實測輸出：

```text
=== 1. Analytical Figure 14.2 (XOR) & [31:14] Oral AND Gate ===
  in=(0, 0) -> Fig14.2 XOR=0.0 | Oral [0.5,0.5] raw=0.0 (int=0) | Bias ReLU(a+b-1)=0.0
  in=(0, 1) -> Fig14.2 XOR=1.0 | Oral [0.5,0.5] raw=0.5 (int=0) | Bias ReLU(a+b-1)=0.0
  in=(1, 0) -> Fig14.2 XOR=1.0 | Oral [0.5,0.5] raw=0.5 (int=0) | Bias ReLU(a+b-1)=0.0
  in=(1, 1) -> Fig14.2 XOR=0.0 | Oral [0.5,0.5] raw=1.0 (int=1) | Bias ReLU(a+b-1)=1.0

=== 2. Backprop Training on XOR (4000 epochs): Linear Collapse vs ReLU ===
  in=(0, 0) target=0 | Linear+Linear pred=0.5062 | Linear+ReLU+Linear pred=0.0000
  in=(0, 1) target=1 | Linear+Linear pred=0.5056 | Linear+ReLU+Linear pred=1.0000
  in=(1, 0) target=1 | Linear+Linear pred=0.4918 | Linear+ReLU+Linear pred=1.0000
  in=(1, 1) target=0 | Linear+Linear pred=0.4912 | Linear+ReLU+Linear pred=0.0000
  Final MSE -> Linear+Linear: 0.250053 (stuck at 0.25!) | Linear+ReLU+Linear: 0.00000000

=== 3. GPU Activation Function Cost on NVIDIA GeForce RTX 4060 Laptop GPU ===
  ReLU    (max(0,x))              : 0.861 ms  (1247.14 Gevals/s, 1.00x)
  Sigmoid (1 / (1 + expf(-x)))    : 3.607 ms  ( 297.67 Gevals/s, 4.19x slower)
  tanh    ((e^x-e^-x)/(e^x+e^-x)) : 6.159 ms  ( 174.34 Gevals/s, 7.15x slower)

=== 4. End-to-End CNN Layer Profile (Parameters vs FLOPs vs GPU Time) ===
  Layer Type | Weights (FP32 MiB) | Weight % | GFLOPs (B=8) | FLOP % | GPU Time (ms) | Time %
  -----------+--------------------+----------+--------------+--------+---------------+-------
  CONV2D x2  |    368640 ( 1.41)  |   2.79%  |      7.399   | 97.30% |      14.809   | 84.01%
  ReLU x2    |         0 ( 0.00)  |   0.00%  |      0.000   |  0.00% |       0.043   |  0.24%
  MaxPool x3 |         0 ( 0.00)  |   0.00%  |      0.000   |  0.00% |       0.028   |  0.16%
  FC (end)   |  12845056 (49.00)  |  97.21%  |      0.206   |  2.70% |       2.748   | 15.59%
```

**實測觀察與硬體對照**：
1. **線性塌縮鐵律完美重現**：即使給了兩層權重矩陣並跑滿 4,000 輪反向傳播，缺少中間 `ReLU` 的 `Linear+Linear` 網路因為 $\mathbf{W}_2 \mathbf{W}_1$ 塌縮成單一平面，四個點全部只能猜平均值 `~0.50`，均方誤差死卡在理論極限 **`0.2500`**；一加上 `ReLU` 立刻收斂至 **`MSE = 0.00000000`（100% 精確）**！（同時也驗證老師 `[31:14]` 口頭給的 `[0.5, 0.5]` 原始輸出在單一 `1` 時為 `0.5`，需取整數截斷 `(int)` 或加上 bias `-1` 寫成 `ReLU(a+b-1)` 才會得到純 `0/1` 的 `AND`。）
2. **為什麼硬體設計師愛死 `ReLU`**：即使在內建專用超越函數單元（SFU, Special Function Units）的現代 RTX 4060 GPU 上，每個 SM 的 SFU 通道數仍遠少於一般 FP32/INT32 ALU（Ada 架構為 1:8），加上浮點除法開銷，算 `Sigmoid` 比 `ReLU` **慢了 `4.19×`**，算 `tanh` **慢了 `7.15×`**！在自訂 ASIC 加速器上省掉整座指數查表／逼近電路更是直接省下可觀面積與功耗。
3. **`CONV` 吃算力、`FC` 吃記憶體頻寬的雙極現象**：在這條典型 VGG 風格管線（兩層 $3\times 3$ `CONV2D` + 池化縮到 $7\times 7\times 256$ + 一層 `12544 -> 1024` `FC`）中，兩層 `CONV2D` 的權重加起來才 **`1.41 MiB`（占總參數 `2.79%`）**，卻包辦了 **`97.30%` 的浮點運算量與 `84.01%` 的 GPU 執行時間**；反觀末端單一層 `FC` 獨吞 **`49.00 MiB`（占總參數 `97.21%`！）**，雖然算力只占 `2.70%`，卻因為權重完全無法跨空間重用、嚴重卡在記憶體頻寬，硬是吃掉 **`15.59%` 的執行時間**——數字比例與老師在 `[50:15]` 及 `[70:46]` 講述的「CONV 約 85–90%、FC 約 10–15%、ReLU+Pool 極小」絲毫不差！

---

## 9. 一頁速查表

| 概念／層級 | 核心數學公式或結構 | 架構與硬體意義 |
|---|---|---|
| **萬有逼近器（Def. 104）** | 多層前饋神經網路 $f(\mathbf{x}) \approx f^*(\mathbf{x})$ | 不需預設函數長相，有限參數即可逼近緊緻集上任意連續函數 |
| **線性迴歸 / SVM** | $y = \mathbf{w}^T \mathbf{x} + b$；SVM 靠 Kernel 升維找超平面 | 最底層核心運算就是**向量內積（Dot Product / MAC）** |
| **Sigmoid / $\tanh$ vs ReLU** | $\sigma(x)=\frac{1}{1+e^{-x}}$ vs $\text{ReLU}(x)=\max(0, x)$ | `ReLU` 免算昂貴 $e^{-x}$（RTX 4060 實測快 `4.2×–7.2×`），硬體僅需看正負號位元 |
| **XOR 與層疊鐵律** | $a \oplus b = \text{ReLU}(a-b) + \text{ReLU}(b-a)$ | **兩層線性不可直接相鄰**（$\mathbf{W}_2\mathbf{W}_1$ 塌縮為單層）；非線性層則可相鄰 |
| **Training vs Inferencing** | 反向傳播學權重（離線） vs 固定權重順向預測（線上） | 終端與雲端專用 ML 加速器首要加速低延遲、低功耗的 **Inferencing** |
| **$K\times K$ Pooling** | 視窗取最大值（Max）或平均（Avg） | **0 權重參數**；每層將特徵圖面積壓縮 **$K^2$ 倍**並提供微小平移容忍度 |
| **全連接層（FC）** | 每個輸入對每個輸出皆有獨立權重 | **記憶體白象**（實測占 `97.2%` 權重卻僅 `2.7%` FLOPs，權重重用率極低），僅放網路末端 |
| **二維卷積層（CONV2D）** | $O[n][k][h][w] = \sum_c \sum_r \sum_s I[n][c][h+r][w+s]F[k][c][r][s]$ | 共 **$C \times K$ 個 $R\times S$ 濾波器**；參數極省但占 **>85% 運算時間**，權重與像素高度重用 |

---

## 10. 與後續講次的連結

- **往回對照 [第 18 支](lecture18_note.md)–[第 20 支](lecture20_note.md)（Ch6 GPU 架構）**：第 6 章學過的 SIMT 執行緒束（Warps）、海量暫存器檔與高頻寬記憶體，正是促成本講 §6 所說「2016 年後深度學習走出 AI 寒冬」的第一代推手。
- **往下接 [第 56 支](lecture56_note.md)（Part II：七重迴圈數學表示法、Tiling 與四種 Stationarity 映射）**：本講最後推導出的 `CONV2D` 是一組高達 **7 層的巢狀 `for` 迴圈**（`n, k, h, w, c, r, s`）。下一講將建立形式化迴圈映射符號（`▷` 循序、`∥` 平行、摺疊執行與切塊 Tiling），剖析邊界 **Halo Pixels** 代價，並推導 **Weight / Input / Output / Row Stationary** 四大快取駐留策略！
- **往下接 [第 57 支](lecture57_note.md)–[第 58 支](lecture58_note.md)（Part III–IV：脈動陣列 TPU/Eyeriss 與稀疏量化／PIM）**：當我們看穿 `CONV2D` 吃算力、`FC` 吃頻寬的本質後，第 57 講將設計 **Weight / Output / Row Stationary 脈動陣列**把晶片內資料搬移壓到極限，第 58 講則用**剪枝（Pruning）與 INT8/FP16 量化**直接砍掉那頭 `49 MiB` 的 FC 記憶體白象！

---

## 附：本講一句話總結

> 深度神經網路之所以能成為不預設形式的萬有逼近器，全靠「線性內積（MAC）」與硬體極度廉價的「ReLU 非線性半波整流」交替堆疊，而在現代 CNN 推論中，參數極省卻吃掉逾八成五算力的多通道 2D 卷積層（`CONV2D`）與參數肥大卡頻寬的末端全連接層（`FC`），共同定義了當代 AI 硬體加速器的全部設計命題。

---

*筆記整理自 Smruti R. Sarangi 教授 Advanced Computer Architecture 第 55 講影片與 Next-Gen Computer Architecture v3.2 第 14 章 §14.1，搭配本機 RTX 4060 GPU 實測驗證。*
