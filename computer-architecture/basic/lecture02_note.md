# Basic Computer Architecture 第二支影片完整筆記：計算機結構導論（Chapter 1）

> **課程**：Basic Computer Architecture — Chapt 1: Introduction to Computer Architecture（播放清單第 2 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（1:03:50，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09（Sarangi；`archbook.pdf`）Ch1（p.13–44）
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Architecture 與 Organization](#1-architecture-與-organization)
- [2. 什麼是電腦](#2-什麼是電腦)
- [3. 打開桌機：CPU、記憶體、硬碟](#3-打開桌機cpu記憶體硬碟)
- [4. 電腦很笨，但超快](#4-電腦很笨但超快)
- [5. 指令的語言：compiler、binary、ISA](#5-指令的語言compilerbinaryisa)
- [6. ISA 的四個性質](#6-isa-的四個性質)
- [7. RISC 與 CISC](#7-risc-與-cisc)
- [8. （選讀）怎麼確定一個 ISA 是完備的](#8-選讀怎麼確定一個-isa-是完備的)
  - [8.1 Turing machine](#81-turing-machine)
  - [8.2 Church-Turing thesis 與 Turing complete](#82-church-turing-thesis-與-turing-complete)
  - [8.3 Universal Turing machine → 現代處理器](#83-universal-turing-machine--現代處理器)
- [9. 現代電腦的組成](#9-現代電腦的組成)
- [10. 單一指令 ISA：sbn](#10-單一指令-isasbn)
- [11. 多指令 ISA](#11-多指令-isa)
- [12. 實際的機器：Harvard 與 Von Neumann](#12-實際的機器harvard-與-von-neumann)
- [13. 暫存器](#13-暫存器)
- [14. 【課本補充】Stack 機器與 accumulator 機器](#14-課本補充stack-機器與-accumulator-機器)
- [15. 接下來的路](#15-接下來的路)
- [16. 【實測】](#16-實測)
- [17. 一頁速查表](#17-一頁速查表)
- [18. 與後續講次的連結](#18-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

這一講從「電腦是什麼」出發，**用理論推出現代電腦長什麼樣子**：

```
電腦 = 可程式化、處理資訊、產生有意義的結果
  ↓ 電腦很笨，只懂很簡單的指令
ISA（指令集架構）= 處理器懂的所有指令的語意
  ↓ ISA 一定要「完備」（能寫出所有程式）
Turing machine → Universal Turing machine
  ↓ 把理論機器切成幾塊，每塊對到一個實際零件
CPU ＋ 程式計數器（PC）＋ 指令記憶體 ＋ 資料記憶體
  ↓
Harvard / Von Neumann 架構 ＋ 暫存器  =  現代電腦
```

中間的 Turing machine 部分（影片 [22:52]–[44:47]，課本 §1.6.1–1.6.6 加了 `*`）是**選讀**。老師說可以跳過，直接看結論（§8.3 的對照表）。

---

## 1. Architecture 與 Organization

[01:15]–[02:58]　課本 §1（p.13）

| 名詞 | 意思 | 誰看的 |
|---|---|---|
| **Computer architecture**（計算機結構） | 電腦呈現給**軟體設計者**的樣子：介面、規格 | 軟體工程師 |
| **Computer organization**（計算機組織） | 電腦在**硬體**上的實際實作 | 硬體設計者 |

- 兩個詞常被混用，或者都叫 architecture。但要記得它們是兩回事。
- 【課本補充】課本強調：寫系統軟體（OS、device driver）的人**一定要**懂架構細節，因為這類軟體和底層硬體綁得很緊。這正是 [`os/linux-os`](../../os/linux-os/README.md) 那門課的出發點。

---

## 2. 什麼是電腦

[02:58]–[05:35]　課本 §1.1（p.14）

> **Definition 2**：電腦是一個**通用的**裝置，可以被**程式化**來**處理資訊**，並**產生有意義的結果**。

三個面向缺一不可：

1. **可程式化**：只做固定動作的電路不是電腦。
   - 【口頭】例：天花板上顯示溫度的溫度計，雖然有螢幕，但不能寫程式，所以不是電腦。
2. **處理資訊**：處理從外面給的資訊（鍵盤、滑鼠）。
3. **產生有意義的結果**。

電腦怎麼運作：

```
  程式 ──┐
         ├──▶  電腦  ──▶  結果
資訊庫 ──┘
```

- 【口頭】例：拍了一張紅眼的照片。照片是**資訊**，去紅眼的程式碼是**程式**，修好的照片是**結果**。
- **程式** = 給電腦的指令列表。
- **資訊庫**（information store）= 電腦要處理的所有資料：圖片、檔案、影片。

---

## 3. 打開桌機：CPU、記憶體、硬碟

[05:35]–[09:26]　課本 §1.2（p.16）

打開桌機會看到：

- **主機板**（motherboard）：綠色的電路板，其他電路都插在上面。
- **CPU**（central processing unit，中央處理器）：電腦的大腦。上面有一個小風扇負責散熱。
- **記憶體**（main memory）：暫時存放 CPU 要用的資訊。CPU 從記憶體讀資料、處理、再寫回去。
- **硬碟**（hard disk）：也存資訊。

記憶體和硬碟的差別：

| | 記憶體 | 硬碟 |
|---|---|---|
| 容量 | 小（【口頭】「現在大概 32 GB」） | 大 10～20 倍 |
| 速度 | 快 | 慢很多 |
| 斷電後 | 資料消失 | 資料還在 |

簡單電腦的方塊圖：

```
         ┌──────────┐
硬碟 ◀──▶│   CPU    │◀──▶ 記憶體
         └────┬─────┘
              │
        I/O 裝置（鍵盤、滑鼠、螢幕、印表機、網路、USB）
```

- 開機時，部分資訊從硬碟搬到記憶體；CPU 從記憶體讀、處理、寫回。
- 要成為完整系統，還要加 **I/O 裝置**（input/output，輸入輸出裝置）。
- 【口頭】主機板上還有很多其他小處理器，留到最後一章（影片說 Ch12，⚠️ 課本 v3.09 是 Ch13）。

---

## 4. 電腦很笨，但超快

[09:26]–[11:35]　課本 §1.3（p.17）

- 【口頭】投影片上會有很多「漢堡圖示」（food for thought），讓學生多想一下。
- 問題：最聰明的電腦是什麼？答案：**我們的大腦**。
- 大腦會抽象思考，能想生命的意義、意識。電腦什麼都不會，**只會加、減、乘**。
- 但電腦的優勢很大：
  - 一秒做**十億次**基本運算；人一秒連 10 次加法都做不到。
  - 永遠不累、不無聊、不分心。
- 結論：電腦是 **ultra-fast 但 ultra-dumb** 的機器。

---

## 5. 指令的語言：compiler、binary、ISA

[11:35]–[16:12]　課本 §1.4（p.18）

**從程式到執行**

```
C / C++ / Java 程式 ──compiler──▶ executable（= binary，一串 0 和 1）──▶ 處理器執行 ──▶ 輸出
```

- 電腦**看不懂** C、C++、Java，只懂 **0 和 1** 組成的語言。
- **Compiler（編譯器）**的工作：把 C／C++ 程式翻成一串 0 和 1。
- **處理器**的工作：讀這串 0 和 1，搞懂它要做什麼，做完，產生有意義的輸出。
- Executable 也叫 **binary**。

**電腦懂什麼**

- 不懂「兩個矩陣相乘」「算行列式」「找孟買到德里的最短路徑」，連「現在幾點」都不懂。
- 只懂「a + b 存到 c」「a × b 存到 c」這種很簡單的句子。

> **Definition 3**：處理器支援的**所有指令的語意（semantics）**，稱為 **ISA（instruction set architecture，指令集架構）**。包含指令本身的語意、它們的運算元（operand），以及和周邊裝置的介面。

- **語意（semantics）** = 指令的意思、怎麼用、代表什麼。
- 不同處理器懂的指令可能不同，或寫法不同，所以有不同的 ISA；但概念一樣。

指令的例子：

| 類別 | 例子 |
|---|---|
| 算術 | add、subtract、multiply、divide |
| 邏輯 | and、or、not |
| 資料搬移 | 在記憶體和處理器之間搬資料 |

---

## 6. ISA 的四個性質

[16:12]–[18:33]　課本 §1.5（p.20）

| 性質 | 意思 | 必要／想要 |
|---|---|---|
| **Complete（完備）** | 能寫出使用者想寫的**所有**程式 | **必要** |
| **Concise（精簡）** | 指令數量不要太多 | 想要 |
| **Generic（通用）** | 指令不要太特化，要抓住常見情況 | 想要 |
| **Simple（簡單）** | 每條指令不要太複雜 | 想要 |

- 【口頭】Complete 的例子：有人要讀今天的溫度然後傳上網，有人要拍黑板照片然後辨識文字。不管哪種，ISA 都要寫得出來。
- Concise：不要一萬條指令。影片說「目前的 ISA 大約 32 到 1,000 條」。
  - ⚠️ 課本 §1.5.2 寫「大多 **64 到 1000** 條」：MIPS 64 條，Intel x86「截至 2024 年約 1000 條」。
- Generic：【口頭】不要有 `add14`（只會加 14）這種指令，太特化，沒幾個人會用。
  - 【課本補充】課本的例子是 sin⁻¹(x)：太少用，用函式庫加 Taylor 展開算就好，慢一點也沒關係。
- 「完備」是必要條件；其他三個很主觀，彈性很大。

> **Important Point 1**：ISA 要 complete、concise、generic、simple。Complete 是必要的；其他是想要的（有時還有爭議）。

---

## 7. RISC 與 CISC

[18:33]–[21:23]　課本 §1.5.4（p.21）

| | **RISC**（reduced instruction set computer） | **CISC**（complex instruction set computer） |
|---|---|---|
| 指令數 | 少 | 多 |
| 每條指令 | 簡單、結構規律 | 複雜、不規律；可能一條收一個參數、另一條收三個 |
| 例子 | ARM（幾乎所有手機、平板） | Intel x86、AMD（大部分桌機、筆電） |

- 【課本補充】Definition 4／5 給了數字：RISC 通常 **64～128** 條（ARM、IBM PowerPC、HP PA-RISC）；CISC 通常 **500 條以上**（Intel x86、VAX）。
  - ⚠️ 課本 §1.5.2 又說「大多數 ISA 有 64 到 1000 條」。兩個數字講的範圍不同（一個是 RISC，一個是所有 ISA），不矛盾，但讀的時候別混淆。
- 【課本補充】課本用「加一串數字」解釋差別：
  - RISC 做法：一條指令加兩個數。compiler 要產生比較多指令，但每條很快。
  - CISC 做法：一條指令吃一整串運算元。指令比較少，但每條比較慢。
  - 哪個比較快，不能一概而論。
- 【課本補充】RISC 與 CISC 的爭論到 90 年代末都很激烈，之後大家偏向 RISC。現代處理器多半是**混合式**：有簡單也有複雜的指令，但**在內部會把 CISC 指令翻成 RISC 式的指令**。
- 【口頭】「我們現在還沒辦法評估兩者的取捨，後面有很多機會討論。」

**中場小結**（[21:23]，課本 Way Point 1）

- 電腦是 ultra-dumb 但 ultra-fast 的機器。
- 指令是和處理器溝通用的基本命令。
- Compiler 把 C 這類語言翻成 0 和 1 編碼的指令。
- ISA = 處理器支援的所有指令和它們的語意。
- ISA 一定要完備；最好也精簡、通用、簡單。

---

## 8. （選讀）怎麼確定一個 ISA 是完備的

[22:52]–[44:47]　課本 §1.6（p.22–33，標 `*` 的小節）

- 【口頭】不想碰理論的同學可以跳過，下載投影片後按箭頭直接跳到**第 35 張投影片**。
- 引子：如果只有 **add**，能算 5 − 3 嗎？**不行**。如果只有 **subtract**，能加法嗎？**可以**：

  ```
  a + b = a − (0 − b)
  ```

  所以 subtract 比 add「更強」。我們要找一組指令，讓**沒有任何其他指令比整組更強**，而且所有程式都能用這組實作。
- 這要借助理論計算機科學。

> **Definition 6**：能執行**任何**程式的機器叫做 **universal machine（通用機器）**。

- 機器的每個基本**動作（action）**就可以看成一條**指令**：要處理器加兩個數，就是要它做一個動作。
- 所以「universal ISA」和「universal machine」幾乎是同一件事。課本說它們是**對偶問題**（dual problems）。

### 8.1 Turing machine

[26:59]–[33:22]　課本 §1.6.2（p.23）

- 由 **Alan Turing**（計算機科學之父）提出，是**已知最強的計算機器**。
- 【口頭】Turing 和印度有關係：他出生時，父親在印度文官體系（Indian Civil Service）工作。
- 它是**理論**裝置，不是實際裝置。但理論上能算出幾乎所有我們想寫的程式。

**結構**（課本 Figure 1.6）：

```
      ... │ $ │ 7 │ 3 │ 4 │ 6 │ 9 │ $ │ ...     ← 無限長的紙帶（tape），每格一個符號
                                ▲
                              tape head（可以左移或右移）
  ┌──────────────┐   ┌──────────────┐
  │ state register│   │ action table │
  └──────────────┘   └──────────────┘
```

- **紙帶（tape）**：無限長（兩個方向都無限），由格子組成，每格存一個**符號**。符號是某個有限集合的元素：字母、數字都可以。
- **Tape head**：指著目前的格子，每一步左移或右移一格。
- **State register（狀態暫存器）**：存目前的狀態（有限多種狀態之一）。
- **Action table（動作表）**：用（舊狀態, 舊符號）查出要做什麼。

每一步：

```
(state, symbol) → ({L, R}, new state, new symbol)
```

讀出目前狀態和 tape head 下的符號 → 查表 → 寫入新狀態、覆寫新符號、往左或右移一格。

**例子：把數字加 1**（[30:13]，課本 Example 2）

- 每格存一位數，兩邊用 `$` 隔開。Tape head 從**個位數**開始。
- 狀態只用兩種：**1**（要加 1，也就是有進位）和 **0**（不用加）。一開始是狀態 1。
- 規則：狀態 1 時，把數字 x 換成 (x + 1) mod 10，新狀態 = 進位；然後往左移。
- 影片的例子：個位是 9 → 寫 0，進位 1；下一格是 6 → 寫 7，進位 0；之後狀態都是 0，不用改；碰到 `$` 就結束。
- ⚠️ 影片版「碰到 `$` 就結束」，沒處理**最高位還有進位**的情況（例如 999 + 1）。課本多兩個狀態 **pre-exit** 與 **exit**：碰到 `$` 時，如果進位是 1，就寫 1、左移，再寫 `$`，才進入 exit。見 §16.2 實測。
- ⚠️ 課本 Example 2 的原文是「We first define four states (S0, S1): pre-exit and exit」，括號裡只列了兩個，句子有排版錯誤。實際是 S0、S1、pre-exit、exit 四個。

【口頭】這台機器看起來簡單，**它也真的簡單**，但非常強大：數學問題、工程問題、蛋白質摺疊、遊戲都能解。想知道怎麼做，讀形式語言或自動機理論的書。

【課本補充】課本還有兩個例子（影片沒講）：

- **Example 3**：判斷字串是不是 `aaa…abb…bb` 的形式。兩個狀態 S_a、S_b，加上 exit、error。由右往左掃：
  ```
  (Sb, b) → (L, Sb, b)    (Sa, b) → (L, error, b)
  (Sb, a) → (L, Sa, a)    (Sa, a) → (L, Sa, a)
  (Sb, $) → (L, error, $) (Sa, $) → (L, exit, $)
  ```
- **Example 4**：判斷只含 a、b 的字串是不是**回文**（palindrome，正讀反讀一樣，例如 civic、rotor）。每輪把最右邊的字元換成 `$`，跑到最左邊檢查是否相同，再跑回來；用 L_a、L_b、Rcheck_a、Rcheck_b、Rmove 等狀態。
- 課本的結論：連簡單問題的 Turing machine 都很難設計、很難讀，動作表很快就爆炸。但重點是**可以做到**。

### 8.2 Church-Turing thesis 與 Turing complete

[33:22]–[35:35]　課本 §1.6.2（p.26）

- 和 Turing 同時期，數學家 **Alonzo Church** 提出另一種形式系統，也能寫出任何程式。
- **Thesis（論點）** 不是定理：是**沒有證明**的陳述，等別人找反例。但六十年來沒人找到。

> **Definition 7**（Church-Turing thesis）：任何真實世界的計算，都可以翻成在 Turing machine 上的等價計算。

- 怎麼翻？只要定義：紙帶上的符號集合、狀態集合、動作表。

> **Definition 8**：和 Turing machine 等價的計算系統叫做 **Turing complete**。

- 【課本補充】多條紙帶、多個 tape head、每條紙帶多軌，這些延伸版都**和簡單版一樣強**。
- 【課本補充】有些程式在 Turing machine 上要跑無限久，那它們在其他所有機器上也一樣要無限久。

### 8.3 Universal Turing machine → 現代處理器

[35:35]–[44:47]　課本 §1.6.3–1.6.4（p.27–30）

- Church-Turing thesis 告訴我們：**每個問題**都能設計一台 Turing machine。
- 問題：能不能設計一台**能模擬任何其他 Turing machine** 的機器？這叫 **Universal Turing Machine（UTM）**。
  - 輸入：一台 Turing machine ＋ 它的紙帶內容。
  - 輸出：那台機器本來會產生的輸出。
- 為什麼做得到？因為 Turing machine 的邏輯很簡單（讀狀態和符號 → 查表 → 改狀態、寫符號、移動）。UTM 用自己的動作表、狀態暫存器和紙帶，就能模擬這些步驟。
  - 【口頭】「你們之中有人可以試著自己做一台，一點也不難。」

**UTM 的結構**（[39:28]）：

- 自己的 **generic action table**（通用動作表）與 **generic state register**（通用狀態暫存器）。
- 紙帶上放著：
  - **simulated action table**：被模擬機器的動作表
  - **simulated state register**：被模擬機器的狀態
  - **work area**：被模擬機器的紙帶，加上 UTM 自己暫存用的符號

**這和現代處理器有什麼關係**（[40:14]）—— 本講最重要的對照表：

| UTM 的部分 | 變成現代電腦的 | 為什麼 |
|---|---|---|
| Generic action table | **CPU** | 看被模擬的機器要做什麼，就去做；是通用的計算裝置 |
| Simulated action table | **指令記憶體** | 裡面放的是「要做什麼」，也就是所有指令 |
| Simulated state register | **程式計數器（PC）** | 記錄現在跑到程式的哪裡 |
| Work area | **資料記憶體** | 放所有要處理的資訊和暫存資料 |

- **Program counter（PC，程式計數器）**：告訴硬體和軟體，處理器現在跑到程式的哪一點。程式一執行，它就一直變。
- 【口頭】重點：**把一個理論裝置切成好幾塊，每塊指派一個實際零件，就從理論裝置得到了實際裝置。**

【課本補充】課本 §1.6.4 定義了一台更接近實作的 **modified UTM**（已被證明是 Turing complete）：

1. 紙帶是**半無限**的（只往一個方向無限）。
2. 模擬的狀態是一個**指標**，指向模擬動作表裡的一筆。
3. 每個狀態在動作表裡只有一筆；查表時**不看** tape head 下的符號。
4. 一個動作會去讀 work area 的幾個位置，用簡單的算術函數算出新值，寫到 work area 的新位置。
5. 預設的下一個狀態 = 動作表裡的**下一筆**。
6. 如果某個位置的值小於某個數，動作可以把狀態改到任意位置，也就是從動作表的新地方開始取動作。

- 第 5 點就是「PC + 1」，第 6 點就是「條件跳躍（branch）」。

> **Definition 9**：概念機器的記憶體是一個**半無限的符號陣列**。一部分放程式（基本指令），其餘放資料（程式用的變數和常數）。

⚠️ 影片 [28:19] 說紙帶「兩個方向都無限」，那是**原始** Turing machine；課本的 modified UTM 與 Definition 9 是**半無限**。兩者講的是不同機器，不矛盾。

---

## 9. 現代電腦的組成

[44:47]–[47:38]　課本 §1.6.7（p.32，Figure 1.10）

【口頭】「跳過理論部分的同學，歡迎回來。」

```
┌──────────────── CPU ────────────────┐
│  Program counter │ Control │ Arithmetic │
│       (PC)       │  unit   │    unit    │
└─────────┬───────────────────────────┘
          │
┌──── Memory ─────────────────┐
│  Program（指令） │ Data（資料） │
└─────────────────────────────┘
```

| 零件 | 工作 |
|---|---|
| **CPU** | 讀指令、解讀、執行 |
| 算術單元（arithmetic unit） | 加、減、乘、除 |
| 控制單元（control unit） | 跑有 if 和 for 的程式 |
| 程式計數器（PC） | 現在執行到哪一行 |
| 程式（program） | 指令記憶體裡實際的指令 |
| 資料（data） | 資料記憶體裡的變數、常數、檔案 |

- **記憶體**是一個很大的 **byte 陣列**（byte 在 Ch2 講）。可以分成指令記憶體和資料記憶體。
- 執行完一條指令，PC 就移到下一條。遇到 if 或 goto 時，處理器**跳**到程式的新位置，PC 也跟著變。

---

## 10. 單一指令 ISA：sbn

[47:38]–[54:19]　課本 §1.6.5（p.30）

令人驚訝的事：**只要一條指令就是 Turing complete 的**。

```
sbn a, b, c      ; subtract and branch if negative
```

- 意思：`a = a − b`；如果結果 **< 0**，跳到第 c 行；否則執行下一行。
- a、b 是記憶體位置。

**例 1：a + b**（假設 temp 一開始是 0）

```
1: sbn temp, b, 2      ; temp = 0 − b = −b；不管結果都到第 2 行
2: sbn a, temp, exit   ; a = a − (−b) = a + b；不管結果都離開
```

**例 2：加總 1 到 10**（[50:23]）

影片的版本（6 行；one = 1、index = 10、sum = 0）：

```
1: sbn temp, temp, 2     ; temp = 0
2: sbn temp, index, 3    ; temp = −index
3: sbn sum, temp, 4      ; sum += index
4: sbn index, one, exit  ; index −= 1；變負數就離開
5: sbn temp, temp, 6     ; temp = 0
6: sbn temp, one, 1      ; 0 − 1 < 0，一定跳回第 1 行（= goto）
```

- 第 1–3 行就是例 1 的加法技巧：`sum = sum + index`。
- 第 5–6 行讓 temp 變成 −1，一定跳，就是 **goto**。這樣就做出了 **for 迴圈**。
- 累加的順序：sum += 10、9、8、……、0。
- ⚠️ 影片口頭說「6 行」，但投影片的實際內容逐字稿看不到；上面是依口頭描述還原的。

課本的版本（7 行；多一個 counter = 9 控制迴圈，index = 10）：

```
1: sbn temp, temp, 2       // temp = 0
2: sbn temp, index, 3      // temp = -1 * index
3: sbn sum, temp, 4        // sum += index
4: sbn index, one, 5       // index -= 1
5: sbn counter, one, exit  // loop is finished, exit
6: sbn temp, temp, 7       // temp = 0
7: sbn temp, one, 1        // (0 - 1 < 0), hence goto 1
```

- 兩版答案都是 55（實測見 §16.1）。差別：影片版多加了一次 0，課本版用另一個變數數圈數。

**組合語言**

- 這種只用非常簡單的機器指令、不用高階語法的寫法叫做**低階程式設計（low-level programming）**。
- 剛剛寫的就是我們的**第一個組合語言程式**。
- 【口頭】為什麼會想、或不想只用一條指令？「後面再討論。」
- 【課本補充】其他被證明完備的單一指令 ISA：subtract and branch if less than or equal、reverse subtract and skip if borrow、只用通用記憶體搬移的電腦（參考 Gilreath & Laplante 2003）。

---

## 11. 多指令 ISA

[54:19]–[55:03]　課本 §1.6.6（p.31）

- 只用一條指令寫程式很痛苦，程式會很長。「沒有理由對指令數量那麼小氣。」
- 現代 ISA 會把 sbn 拆成很多條：

> **Important Point 4**：任何指令集**至少**需要三類指令：
> 1. **算術**（data processing）：加減乘除；大多也有邏輯運算（OR、NOT）。
> 2. **資料搬移**（data transfer）：在記憶體位置之間搬值，或把常數載入記憶體。
> 3. **分支**（control）：依運算元的值，從程式的不同位置開始執行。用來實作 if、for、while。

---

## 12. 實際的機器：Harvard 與 Von Neumann

[55:03]–[56:02]　課本 §1.7.1–1.7.2（p.34–36）

```
Harvard：                            Von Neumann：
指令記憶體 ◀─▶ CPU ◀─▶ 資料記憶體      記憶體 ◀─▶ CPU ◀─▶ I/O
              │                    （指令和資料放在同一個記憶體）
             I/O
```

- **Harvard 架構**：指令記憶體和資料記憶體**分開**。
- **Von Neumann 架構**：指令和資料**合在同一個記憶體**。影片說「這是唯一的差別」。

【課本補充】影片沒講的歷史與重點：

- Harvard 架構來自最早的 **Harvard Mark-I**。CPU 分成 **control**（取指令、處理、協調執行）和 **ALU**（arithmetic-logic unit，算術邏輯單元）。
- Von Neumann 架構由 John von Neumann 提出；Eckert 與 Mauchly 獨立想到類似的點子，1946 年做出 **ENIAC**（第一台通用 Turing complete 電腦，「有一個小限制」），用來算美國陸軍的砲彈射表；1949 年接著做出 **EDVAC**。
- 真正的突破是：

> **Definition 10**（**stored-program concept**，儲存程式概念）：程式存在記憶體裡，**指令被當成一般的記憶體值**。

- 好處：處理器和記憶體系統可以用同一套方式處理指令和資料；程式容易儲存、修改、傳送；程式甚至能在執行時改自己或改別的程式。這是 compiler 和 Java 虛擬機（JVM）會動態改寫指令的基礎。§16.4 實測把一個函式當資料讀出來。
- 細節：Von Neumann 和 Harvard 機器的記憶體**不是無限的**，所以嚴格說不完全等價於 Turing machine。所有實際機器都是這樣，學界接受這個近似。

---

## 13. 暫存器

[56:02]–[59:47]　課本 §1.7.3（p.36）

- 問題：記憶體是一個很大的結構，**越大越慢**。
- 解法：在 CPU 裡面放一小組**有名字的儲存位置**，叫做**暫存器（register）**，非常快。

```
┌────── CPU ──────┐
│  暫存器（8～64 個）│ ◀── load ── 記憶體（大、慢）
│       ALU        │ ── store ──▶
└─────────────────┘
```

- 大部分資料從暫存器來。暫存器沒有的，先從記憶體讀進暫存器；暫存器滿了，就寫回記憶體再讀別的。
- 通常有 **8 到 64 個**暫存器。
- 為什麼有用？因為存取有**區域性（locality）**：同一段時間內，常常重複用同幾個變數。
- 從記憶體讀到暫存器用 **load** 指令，寫回用 **store** 指令。
- 慣例上暫存器寫成 r1、r2、r3……

影片的例子（[58:24]）：

```
r1 = mem[b]       // load b
r2 = mem[c]       // load c
r3 = r1 + r2
mem[a] = r3       // store
```

- 「現在的新機器長什麼樣？就是 Von Neumann 架構，只是 CPU 裡面多了暫存器。」

【課本補充】

- 課本的例子是 **a = b³ + c³**：先 load b、c，在暫存器裡做 5 次運算，最後 store 一次。運算越複雜，省下的記憶體存取越多。§16.3 實測用 gcc 編出來看。
  - ⚠️ 課本這段程式的最後一行行號印成 `4:`，應該是 `8:`。
- 課本：大部分程式以純暫存器指令為主，**load 和 store 大約占執行指令的三分之一**。
- 課本還提到廣義的 **register machine**：理論上有無限多個暫存器、可隨機存取、所有指令都用暫存器名稱當運算元。本書看的是「Von Neumann ＋ 暫存器」的混合版。

---

## 14. 【課本補充】Stack 機器與 accumulator 機器

課本 §1.7.3（p.37–38），影片沒講。

**Stack 機器**（硬體堆疊）

- 先把值從記憶體 push 進堆疊，算術運算作用在**堆疊頂端的 k 個元素**，結果取代它們。
- 算術指令**不需要運算元**（運算元隱含在堆疊頂端），所以機器碼非常**緊湊**。
- 缺點：產生程式碼很難、彈性差、會有很多多餘的 load／store。
- 適合：要算長數學式的機器、在意程式大小的系統。
- 實例：Burroughs Large Systems、UCSD Pascal、HP 3000（classic）。**Java** 編譯時假設一台虛擬的 stack 機器，JVM 執行時再動態轉成 register 機器的程式。
- 課本的例子：w = x + y/z − u*v

  ```
  1: push u     6: divide
  2: push v     7: subtract
  3: multiply   8: push x
  4: push z     9: add
  5: push y    10: pop w
  ```

  ⚠️ 課本這兩行的註解寫反了：`push z // load y`、`push y // load z`。

**Accumulator 機器**

- 只有一個暫存器，叫 **accumulator（累加器）**。每條指令只收一個記憶體位置當輸入：例如 add 把 accumulator 和記憶體的值相加，存回 accumulator。
- 五〇年代放不下 register file 的早期機器用它來減少記憶體存取。
- x86 還留著影子：大數的乘除法把 **eax** 當 accumulator。

---

## 15. 接下來的路

[59:47]–[63:50]　課本 §1.8（p.38–41）

**這講推出了什麼**：電腦要有 CPU、PC、儲存位置（暫存器和記憶體）、I/O 裝置。

**ISA 是硬體和軟體之間的介面**：

```
C / C++ / Java ──compiler──▶ 指令 ──▶ 硬體實作的處理器 ──▶ 結果經 I/O 顯示
                    └──── ISA：軟硬體的介面 ────┘
```

- 【課本補充】課本把 ISA 比喻成**軟體和硬體之間的合約**：軟體要保證所有程式都能有效率地翻成基本指令，硬體要保證所有指令都能有效率地實作。

**課程路線圖**：

| 部分 | 影片章號 | 課本 v3.09 章號 | 內容 |
|---|---|---|---|
| 軟體介面 | Ch2 | Ch2 | 位元的語言：怎麼用 0 和 1 表示資訊 |
| | Ch3 | Ch3 | 從零設計的通用組合語言（SimpleRisc） |
| | Ch4 | Ch4 | ARM 組合語言（手機） |
| | Ch5 | Ch5 | x86 組合語言（Intel、AMD） |
| | — | **Ch6** | **RISC-V**（影片版沒有） |
| 處理器設計 | Ch6 | Ch7 | 邏輯閘、暫存器、記憶體（從電路的角度） |
| | Ch7 | Ch8 | 計算機算術：加減乘除 |
| | Ch8 | Ch9 | 設計處理器（從簡單的開始） |
| | Ch9 | Ch10 | Pipelining |
| 系統設計 | Ch10 | Ch11 | 記憶體系統 |
| | Ch11 | Ch12 | 多處理器 |
| | Ch12 | Ch13 | I/O 與儲存裝置 |

- 影片說「這本書分 12 章，我們剛講完第 1 章，還有 11 章。」⚠️ v3.09 是 13 章。
- ⚠️ 課本 v3.09 的 Figure 1.14（路線圖）上的章號還是**舊版**（2～12，沒有 RISC-V）；但同一節的文字（§1.8.1–1.8.3）已經用新章號（Ch7 邏輯閘……Ch13 I/O）。
- 【課本補充】課本說：讀完 Ch3 SimpleRisc 後，ARM 和 x86 **只要讀其中一章**就好。Ch7 邏輯閘如果修過數位設計可以跳過。
- 【課本補充】§1.8.1 的定義，Ch2 會用到：

> **Definition 11**：**bit** = 0 或 1 的值；**byte** = 8 個 bit；**word** = 4 個 byte。

---

## 16. 【實測】

本機：Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4（x86-64）、aarch64-linux-gnu-gcc（ARM 交叉編譯）。

### 16.1 sbn 模擬器：影片版與課本版的 1 加到 10

用 C 寫一個 sbn 直譯器，跑 §10 的兩個程式。核心只有幾行（`experiments/lecture02/sbn.c:7-17`；四個實測程式都在 `experiments/lecture02/`）：

```c
static int run(const struct ins *prog, int n, int *mem)
{
	int pc = 1, steps = 0;
	while (pc != EXIT) {
		const struct ins *i = &prog[pc - 1];
		steps++;
		mem[i->a] -= mem[i->b];
		pc = (mem[i->a] < 0) ? i->c : (pc % n) + 1;
	}
	return steps;
}
```

```
$ gcc -O2 -Wall -o sbn sbn.c && ./sbn
book : sum=55  steps=68
video: sum=55  steps=64
```

- 兩版都得到 55。
- 課本版：10 圈 × 7 行，最後一圈在第 5 行就離開 → 9×7 + 5 = **68** 步。
- 影片版：index 從 10 跑到 0 共 11 圈 × 6 行，最後一圈在第 4 行離開 → 10×6 + 4 = **64** 步。
- 一個加總 1 到 10 的小程式就要 60 多條指令。這就是 §11「單一指令寫程式很痛苦」的意思。

### 16.2 Turing machine 加 1：最高位進位

用 Python 模擬 §8.1 的加 1 機器，比較「碰到 `$` 就結束」（影片）與「pre-exit 狀態」（課本）：

```
$ python3 tm.py
$73469$  video→ $73470$  book→ $73470$
$999$    video→ $000$    book→ $1000$
```

- 一般情況兩版一樣。
- **999 + 1** 時影片版會得到 000（進位被丟掉）；課本版多了 pre-exit 狀態才正確得到 1000。

### 16.3 暫存器省了多少記憶體存取：x86 與 ARM

把課本 §1.7.3 的 a = b³ + c³ 寫成 C：

```c
int a, b, c;
void cubes(void) { a = b * b * b + c * c * c; }
```

```
$ gcc -O2 -c cubes.c && objdump -d --no-show-raw-insn cubes.o
   4:	mov    0x0(%rip),%edx        ← load b
   a:	mov    0x0(%rip),%ecx        ← load c
  10:	mov    %edx,%eax
  12:	imul   %edx,%eax
  15:	imul   %edx,%eax
  18:	mov    %ecx,%edx
  1a:	imul   %ecx,%edx
  1d:	imul   %ecx,%edx
  20:	add    %edx,%eax
  22:	mov    %eax,0x0(%rip)        ← store a
  28:	ret

$ aarch64-linux-gnu-gcc -O2 -c cubes.c && aarch64-linux-gnu-objdump -d --no-show-raw-insn cubes.o
   8:	ldr	w2, [x0]                 ← load b
   c:	ldr	w3, [x4, #4]             ← load c
  10:	mul	w0, w2, w2
  14:	mul	w1, w3, w3
  18:	mul	w1, w1, w3
  1c:	madd	w0, w0, w2, w1           ← w0*w2 + w1，乘加一條做完
  20:	str	w0, [x4, #8]             ← store a
  24:	ret
```

| | 指令數 | 碰記憶體的指令 |
|---|---|---|
| x86 `-O0` | 19 | 7 |
| x86 `-O2` | 12 | **3** |
| ARM `-O0` | 28 | 7 |
| ARM `-O2` | 10 | **3** |

- `-O2` 的結果正好是課本說的「load b、load c，全部在暫存器裡算，store 一次」：**2 次 load ＋ 1 次 store**。
- `-O0`（不最佳化）每次用到變數都重新去記憶體拿，記憶體存取變成 7 次。
- 看得到 RISC 與 CISC 的差別：
  - **ARM 的位址每次加 4**（0, 4, 8, c, …）：每條指令固定 4 byte，結構規律。
  - **x86 的位址間隔不規則**（4, a, 10, 12, 15, …）：指令長度不固定（1～6 byte），結構不規律。
- ⚠️ ARM 是 RISC，但也有 `madd`（乘完再加）這種「一條做兩件事」的指令。這就是課本說的「現代處理器多半是混合式」。

### 16.4 Stored-program：指令就是記憶體裡的 byte

```c
__attribute__((noinline)) int add(int x, int y) { return x + y; }

int main(void)
{
	const unsigned char *p = (const unsigned char *)add;
	for (int i = 0; i < 12; i++)
		printf(" %02x", p[i]);
	...
}
```

```
$ gcc -O2 -o stored stored.c && ./stored
add() at 0x5577daa0f1d0: f3 0f 1e fa 8d 04 37 c3 f3 0f 1e fa
add(2, 3) = 5

$ objdump -d stored | sed -n '/<add>:/,/^$/p'
    11d0:	f3 0f 1e fa          	endbr64
    11d4:	8d 04 37             	lea    (%rdi,%rsi,1),%eax
    11d7:	c3                   	ret
```

- 用一個**資料指標**就能把 `add()` 的指令讀出來，讀到的 byte 和 `objdump` 反組譯的一模一樣。這就是 Definition 10「指令被當成一般的記憶體值」。
- 前 8 個 byte 就是整個 `add()`：`endbr64`（4 byte）、`lea`（3 byte，一條算出 x + y）、`ret`（1 byte）。後面的 `f3 0f 1e fa` 已經是下一個函式的開頭了。
- 反過來，現代 OS 會把程式碼頁設成**唯讀**，所以「程式修改自己」需要特別的權限設定。這部分屬於 OS 的虛擬記憶體與權限，見 [`os/linux-os` 第 5 支：虛擬記憶體](../../os/linux-os/lecture05_note.md)。

---

## 17. 一頁速查表

| 主題 | 重點 |
|---|---|
| Architecture vs Organization | 給軟體看的介面 vs 硬體的實作 |
| 電腦的定義 | 通用、可程式化、處理資訊、產生有意義的結果 |
| 電腦的本質 | ultra-dumb 但 ultra-fast；一秒十億次基本運算 |
| 從程式到執行 | 高階語言 → compiler → binary（0 和 1）→ 處理器 |
| ISA | 處理器支援的所有指令的語意；軟硬體之間的合約 |
| ISA 四性質 | complete（必要）、concise、generic、simple |
| RISC | 指令少（64～128）、簡單、規律；ARM |
| CISC | 指令多（500+）、複雜、不規律；x86；現代內部翻成 RISC 式 |
| Turing machine | 無限紙帶 ＋ tape head ＋ state register ＋ action table |
| Church-Turing thesis | 任何真實計算都能翻成 Turing machine 的計算（無證明、無反例） |
| Turing complete | 和 Turing machine 等價 |
| UTM → 電腦 | generic action table → CPU；simulated action table → 指令記憶體；simulated state → PC；work area → 資料記憶體 |
| sbn a, b, c | a −= b；< 0 就跳到 c；單一指令就 Turing complete |
| 至少三類指令 | 算術、資料搬移、分支 |
| Harvard | 指令、資料記憶體分開 |
| Von Neumann | 合一；stored-program concept |
| 暫存器 | CPU 內 8～64 個；locality；load／store；約占三分之一的指令 |
| Stack／accumulator | 運算元隱含、程式碼緊湊；JVM、x86 的 eax |
| bit／byte／word | 1／8 bit／4 byte |
| 章號 | 影片 Ch6 以後 + 1 = 課本 v3.09 章號 |

---

## 18. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| bit、byte、用 0 和 1 表示資訊 | [第 3](lecture03_note.md)–[5 支](lecture05_note.md)：Ch2 位元的語言 |
| 組合語言、暫存器、load／store | [第 6](lecture06_note.md)–[8 支](lecture08_note.md)：Ch3 SimpleRisc |
| RISC（ARM）與 CISC（x86） | [第 9](lecture09_note.md)–[14 支](lecture14_note.md)：Ch4 ARM、Ch5 x86 |
| 規律的 RISC 指令編碼 | [第 15](lecture15_note.md)–[17 支](lecture17_note.md)：Ch6 RISC-V |
| ALU、暫存器、記憶體的電路 | [第 18](lecture18_note.md)–[20 支](lecture20_note.md)：課本 Ch7 |
| CPU 的控制單元、PC、取指令 | [第 27](lecture27_note.md)–[29 支](lecture29_note.md)：課本 Ch9 處理器設計 |
| 「大就是慢」、locality | [第 34](lecture34_note.md)–[37 支](lecture37_note.md)：課本 Ch11 記憶體系統 |
| 特權暫存器、ring level（OS 怎麼用 ISA） | [`os/linux-os` 第 2 支](../../os/linux-os/lecture02_note.md) |

---

## 附：本講一句話總結

> **計算機結構分成給軟體看的 architecture 與硬體實作的 organization；電腦是能被程式化、處理資訊並產生有意義結果的通用裝置，它很笨卻很快，只懂 ISA 定義的簡單指令，所以 compiler 要把 C 翻成 0 和 1。ISA 一定要完備，最好也精簡、通用、簡單，這引出 RISC（ARM，指令少而規律）與 CISC（x86，指令多而複雜，現代內部翻成 RISC 式）之爭。要判斷 ISA 是否完備，就看它是否 Turing complete：Turing machine 用無限紙帶、tape head、狀態暫存器與動作表就能算任何東西（Church-Turing thesis），而 universal Turing machine 拆開來就是 CPU、指令記憶體、PC 與資料記憶體；連單一指令 sbn（減完是負數就跳）都完備，但實測加總 1 到 10 就要 64～68 步，所以實際 ISA 至少要有算術、資料搬移、分支三類指令。實際機器有指令與資料分開的 Harvard 架構和合一的 Von Neumann 架構（stored-program，實測可直接把函式當 byte 讀出），再加上 CPU 內 8～64 個暫存器利用 locality（實測 b³ + c³ 在 -O2 下只剩 2 次 load、1 次 store），就是現代電腦。影片的 Turing machine 加 1 沒處理最高位進位（999 → 000），課本靠 pre-exit 狀態解決；影片的 12 章在課本 v3.09 變成 13 章，Ch6 以後章號加 1。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 1: Introduction to Computer Architecture 錄影逐字稿（YouTube 自動字幕，1:03:50）、Sarangi *Basic Computer Architecture* v3.09 Ch1（p.13–44）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、aarch64-linux-gnu-gcc）結果。*
