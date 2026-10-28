# Basic Computer Architecture 第二十支影片完整筆記：記憶體單元：SRAM、DRAM、CAM（Chapter 6 Digital Logic, Part 3/3 → 課本 Ch7）

> **課程**：Basic Computer Architecture — Chapt 6: Digital Logic, Part 3/3（播放清單第 20 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（52:33，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 **Ch7** §7.4.1–7.4.3（p.286–293）
> **章號**：影片的 Chapter 6 是課本 v3.09 的 **Chapter 7**（Logic Gates, Registers, and Memories）。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 為什麼需要記憶體單元](#1-為什麼需要記憶體單元)
- [2. SRAM 單元：6 個電晶體](#2-sram-單元6-個電晶體)
- [3. SRAM 陣列](#3-sram-陣列)
- [4. 問題：bit line 很長、很慢](#4-問題bit-line-很長很慢)
- [5. 解法：pre-charge ＋ sense amplifier](#5-解法pre-charge--sense-amplifier)
- [6. 完整的 SRAM 陣列](#6-完整的-sram-陣列)
- [7. DRAM 單元：1 個電晶體 ＋ 1 個電容](#7-dram-單元1-個電晶體--1-個電容)
- [8. DRAM 陣列與 refresh](#8-dram-陣列與-refresh)
- [9. CAM：用內容找資料](#9-cam用內容找資料)
- [10. CAM 陣列](#10-cam-陣列)
- [11. 【課本補充】ROM、PROM、EPROM](#11-課本補充rompromeprom)
- [12. Chapter 6（課本 Ch7）總結](#12-chapter-6課本-ch7總結)
- [13. 【實測】](#13-實測)
- [14. 一頁速查表](#14-一頁速查表)
- [15. 與後續講次的連結](#15-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

數位邏輯這章的最後一部分。前兩支（第 18、19 支）講了電晶體、邏輯閘、組合邏輯、latch 和 flip-flop；這一講講**存大量位元**的記憶體單元：

| 單元 | 電晶體數 | 核心 | bit line | 特色 |
|---|---|---|---|---|
| Flip-flop | 多 | 交叉耦合的 NAND 閘 | 沒有 | 最快 |
| **SRAM** | **6** | 交叉耦合的反相器 | **2 條**（BL、BL̄） | 快；用在 cache、register file |
| **DRAM** | **1 ＋ 1 個電容** | 電容 | **1 條** | 最密、最慢；會漏電，要 refresh |
| **CAM** | **10** | SRAM ＋ 4 個比較用的電晶體 | 2 條 ＋ match line | 用**內容**找資料 |

本講的關鍵技巧：**pre-charge ＋ sense amplifier**，不用等 bit line 完全充放電就能讀出結果。

---

## 1. 為什麼需要記憶體單元

[00:00]–[01:31]

- 記憶體是比 flip-flop **密得多**的儲存技術，所以處理器裡要存大量資料（不是幾個位元，而是幾千、幾百萬個位元）的結構都用記憶體。
- 兩種常用的技術：**SRAM**（static RAM）和 **DRAM**（dynamic RAM）。
- 設計理念：要存幾百萬、甚至幾十億個位元，所以：
  1. **每個位元用的電晶體越少越好**
  2. **越快越好**

---

## 2. SRAM 單元：6 個電晶體

[01:31]–[09:19]　課本 §7.4.1、Figure 7.26（p.286–287）

**核心：交叉耦合的反相器**

```
        ┌──▷o──┐
   X ───┤      ├─── X̄
        └──o◁──┘
```

- 為了減少電晶體，**不用 flip-flop 那種交叉耦合的 NAND 閘**，改用**一對交叉耦合的反相器**（inverter）：一個的輸出接到另一個的輸入。
- 一邊存 X，另一邊就是 X̄。這樣就能存一個位元。
- 一個反相器 = 2 個電晶體（一個 PMOS、一個 NMOS），所以這裡是 **4 個電晶體**。

**加上兩個存取電晶體**：

```
           Word line (WL)
      ┌──────┬─────────────────┬──────┐
      │     ─┴─               ─┴─     │
  BL ─┤     W1 ── [ 4T 反相器對 ] ── W2  ├─ BL̄
      │                               │
```

- 只有 4 個電晶體**沒辦法讀寫**，所以再加兩個 NMOS 電晶體 **W1、W2**，叫 **word line 電晶體**。
- **Word line = 0**：W1、W2 關閉，這個單元和外界**斷開**，不能讀也不能寫，裡面的值保持住。
- **Word line = 1**：W1、W2 打開，可以讀寫。
- 兩邊各接一條銅線，叫 **bit line**：**BL** 和 **BL̄**（存值的互補）。
- 總共 **6 個電晶體**：這就是 **6T SRAM 單元**。

**寫入**：

- 寫 1：word line 設成 1；BL 驅動到 **1**（供應電壓）、BL̄ 驅動到 **0**（接地）。只要驅動電路夠強，4T 反相器對就會被設定成這一邊 1、另一邊 0。
- 寫 0：BL 驅動到 0、BL̄ 驅動到 1。

**讀取**：

- word line 設成 1，**不**主動驅動 BL 和 BL̄，讓它們的電壓**跟著單元裡的電壓走**。
- 例：單元這邊是 1、那邊是 0，過一陣子 BL 會變成 1、BL̄ 變成 0，就知道存的是 1。
- 讀的時候 bit line 是**浮接**（floating）：不接供應電壓也不接地。

【課本補充】讀者可能會想：既然反相器比 NAND 省電晶體，為什麼 latch 不用交叉耦合的反相器？因為 SRAM 讀寫需要的周邊電路（下面會講）**負擔很大**，只存一個位元的話划不來。

---

## 3. SRAM 陣列

[09:19]–[14:33]　課本 Figure 7.27（p.287）

- 把 SRAM 單元排成**矩陣**：
  - **同一列**（row）的所有單元共用**同一條 word line**
  - **同一行**（column）的所有單元共用**同一對 bit line**

```
              BL BL̄    BL BL̄    BL BL̄
              │  │     │  │     │  │
  WL ─────── [SRAM]── [SRAM]── [SRAM]
  WL ─────── [SRAM]── [SRAM]── [SRAM]     ← decoder 依位址只打開一條 WL
  WL ─────── [SRAM]── [SRAM]── [SRAM]
              │  │     │  │     │  │
            ┌─────── column mux／demux ───────┐
            │  write driver    sense amplifier │
            └──── data in ──────── data out ───┘
```

**讀寫一個位址**：

1. 用 **decoder** 把**一條** word line 設成 1（依位址），其他都是 0。
   - 例：每列存一個 int（4 byte = 32 位元 → 32 個 SRAM 單元），有 128 列 = 2⁷，所以用 **7 位元**的位址送進 decoder。
   - **同一時間只能打開一列**。
2. 打開的那一列，所有單元都會去設定自己 bit line 的電壓。
3. 只要讀其中一部分（例如 2 byte）時，用 **column multiplexer** 選出要的那幾行。寫入時也一樣，用 **driver 電路**加上 mux 選出要寫的單元。

**寫入模式**：driver 電路設定 BL、BL̄；同時 decoder 打開 word line；值就傳進 SRAM 單元。

**讀取模式**：先用電晶體把 bit line 和供應電壓、接地**斷開**（浮接）；再打開 word line；bit line 的值就依單元的內容變成 1／0。

- 【課本補充】一列可能有 100 多個 SRAM 單元；用位址的一部分選列（decoder）、另一部分選行（column mux），這種設計叫 **2.5D 記憶體組織**。陣列要盡量接近**正方形**，太不對稱的話，word line 或 bit line 的電容負載會太大。

---

## 4. 問題：bit line 很長、很慢

[14:33]–[16:45]

- 實際的陣列很大。例：**256 KB** 的 SRAM 陣列 = 2⁸ × 2¹⁰ = 2¹⁸ byte = **2²¹ 位元**。
- 就算排成接近正方形（2¹¹ 列 × 2¹⁰ 行），每條 bit line 還是接了 **2048 個** SRAM 單元。
  - ⚠️ 逐字稿把 2048 聽成「248」。
- 一條長銅線接了這麼多東西，**電阻和電容**都很大：【口頭】「記得大一學過的：線越長、接的元件越多，R 和 C 就越大。」
- RC 延遲大 → 把線拉到 0 或 1 要花**很多時間**。
- 而**讀取**時，是那個小小的 SRAM 單元要去驅動這條長長的 bit line，所以讀取很慢。
- **寫入不慢**：可以接很強的 write driver。

---

## 5. 解法：pre-charge ＋ sense amplifier

[16:45]–[24:38]　課本 p.288–289

**第一步：pre-charge（預充電）**

- 讀之前，用**很強的 pre-charge 電路**把 BL 和 BL̄ **都充到 0.5 V**（假設邏輯 1 = 1 V，也就是 V_dd／2）。
- 到 0.5 V 後，用電晶體斷開 pre-charge 電路，bit line 變成浮接，再打開 word line。

**第二步：監看兩條線的差**

- t = 0 時兩條都是 0.5 V；之後一條往 0 V 走、另一條往 1 V 走。
- **不用等**它們真的到 0 V 和 1 V！
- 【口頭】比喻：
  - 學生修一門有五次考試的課，考完兩次就知道沒希望及格了，不用等五次都考完就可以退選。
  - 下西洋棋不會等國王真的被吃掉，早就知道輸了就投降。
  - 選舉開票：一開始兩個候選人不相上下，開票過程也有雜訊；但其中一個漸漸大幅領先時，不用等所有票開完就可以宣布當選。
- 監看 **Δ = V_BL − V_BL̄**。
- **雜訊**：電路裡會有一點電的雜訊。【口頭】「你一定看過，開微波爐或洗衣機時手機會有一點雜訊，那是因為有一點電感。」定義一個**雜訊門檻 T**。
- **|Δ| 超過 T，就宣布結果**：Δ > 0 → 1；Δ < 0 → 0。
- 例：1 ns 後 BL = 0.6 V、BL̄ = 0.4 V，就可以確定 BL 最後會到 1、BL̄ 最後會到 0。
  - ⚠️ 老師口誤說「0.4 最後會到 1」，馬上更正：「應該是邏輯 0。」
- 【口頭】如果要等一條線完全放電、另一條完全充電要 20 ns，用這個方法 1 ns 就有結果。這是**硬體裡非常標準的做法**。

【課本補充】為什麼這麼快：

1. pre-charge 電路可以灌入很大的電流，**充電很快**。
2. 判斷結果需要的電壓擺幅（門檻）**遠小於**供應電壓。bit line 的電容很大，充放電的時間非常關鍵，所以減少需要的擺幅，差別就很大。

§13.2 用 RC 模型算：等到接近滿擺幅要 4.6 RC，等到差 0.1 V 只要 0.1 RC，**快約 44 倍**。

---

## 6. 完整的 SRAM 陣列

[24:38]–[27:04]

比 §3 多了三樣東西：

1. **Pre-charge driver**（最上面）：讀之前把 bit line 預充電。
2. **Write driver**：很強的電路，寫入時把 bit line 設成 0 或 1。
3. **Sense amplifier**：監看兩條 bit line 的電壓差，超過門檻就輸出 0 或 1。

- **Column mux**：陣列很大時，為了電路設計的考量，要把它排成正方形而不是一長條，所以每一列要用 mux 選出其中一部分的行。

【課本補充】要有很多 SRAM 單元，才值得用這些 pre-charge 電路、write driver 和 sense amplifier。所以 SRAM 適合 **register file 和晶片上的記憶體**（cache）；只存幾個位元的話，flip-flop 比較好。

---

## 7. DRAM 單元：1 個電晶體 ＋ 1 個電容

[27:04]–[30:21]　課本 §7.4.3、Figure 7.30（p.291）

```
     Word line (WL)
          │
  BL ────┤ ├──── ┤├ ──┐
         電晶體   電容    ┴ 接地
```

- 比 SRAM **更小**：單元越小，同樣面積能放越多，**越密**。
- 但單元越小，**驅動 bit line 的能力越弱**，所以越**慢**。
  - 【口頭】latch 和 flip-flop 最快（電晶體多、沒有 bit line）；SRAM 還算快；DRAM 慢。
- 沒有交叉耦合的反相器，**核心是一個電容**：
  - 電容兩端的電壓接近供應電壓 → **1**
  - 接近 0 V → **0**
- 一個電晶體由 word line 控制，打開時電容直接接到 bit line。
- **只有一條 bit line**，不是兩條。
- **寫入**：用 driver 電路灌入電荷（寫 1）或抽出電荷（寫 0）。之後關掉 word line 電晶體，電容斷開，電壓就保持住。
- 【課本補充】DRAM 只用一個電晶體存一個位元，**非常密、省面積、省電**，但比 SRAM 和 latch 慢得多，適合**大的、晶片外**的記憶體（主記憶體）。

---

## 8. DRAM 陣列與 refresh

[30:21]–[38:27]　課本 Figure 7.31（p.292–293）

**DRAM 陣列**和 SRAM 陣列很像：

- 寫入：打開 word line，用 driver 給 bit line 充電，電荷傳到電容。
- 讀取：**pre-charge** bit line 到 0.5 V（現在大部分的供應電壓接近 1 V），打開 word line，用 **sense amplifier** 比較 bit line 和**固定的 0.5 V**，差距超過門檻（幾 mV 的雜訊）就宣布結果。
  - SRAM 比較的是 BL 和 BL̄；DRAM 只有一條線，所以和固定電壓比。

**問題：電容會漏電**（[33:39]）

1. **讀取會損失電荷**：讀的時候，電容的電荷要和很長的 bit line 分享，電容的電壓就降低了。
2. **自然漏電**：沒有完美的介電質（dielectric），兩片平行板之間總會有一點漏電；電晶體關著時也會漏一點電流。
   - 【口頭】例：t = 0 存了 1 V，2 ns 後可能剩 0.8 V，之後整個電壓都會不見，就出錯了。

**解法：refresh（刷新）**：

- **每次讀取之後**，要把讀到的資料**寫回去**。
- 也要**定期**讀每一個 DRAM 單元、再寫回同樣的資料。例：50 ns 後讀到 0.7 V，還是可以判斷是 1，但知道電壓降了，所以寫回 1，電壓又恢復成 1 V。
- 恢復一個單元的值叫 **restore**；讀取整個 DRAM 陣列、刷新所有內容的過程叫 **DRAM refresh**（refresh cycle）。
- 不是一次讀完整個陣列再寫回去，而是一次讀一個 byte 或 64 byte 再寫回去，**定期**輪流做。

⚠️ 影片舉的時間（「2 ns 後 0.8 V」「20 ns、50 ns、1 µs 後整個電壓都不見」）只是示意。實際 DRAM 單元能保存電荷的時間長得多，一般的規格是**每 64 ms** 把整個陣列刷新一遍。課本沒有給具體數字。

**DRAM 陣列和 SRAM 陣列的差別**：

1. 一條 bit line，不是兩條。
2. 多了 **refresh 電路**：定期讀每一列再寫回去，恢復原本的電壓。
3. 【課本補充】**sense amplifier 在 column mux 之前**，而且會**暫存整列（DRAM row，也叫 DRAM page）的資料**，所以之後存取同一列會很快，可以直接從 sense amplifier 拿。

【課本補充】DRAM 的時序：

- 以前 DRAM 是**非同步**存取的，不保證時序；現在每個 DRAM 操作都和系統時脈同步，叫 **SDRAM**（synchronous DRAM）。
- **DDR**（Double Data Rate）：在時脈的**上升和下降緣**都傳資料（double pumped）。DDR1 每次傳 8 byte，峰值 1.6 GB/s；DDR2 兩倍（3.2 GB/s）；DDR3（2007 年起）再兩倍（6.4 GB/s）。現在用 DDR4、DDR5。

---

## 9. CAM：用內容找資料

[38:27]–[45:00]　課本 §7.4.2、Figure 7.28（p.289–290）

**問題**：一般的記憶體是用**位址**找資料。能不能反過來，用**內容**找？例：找出哪一列存的是 `1011`（假設只有一列）。

- 簡單的做法：像 for 迴圈，一列一列讀出來比較。但這樣要讀整個陣列，很慢。
- 更好的做法：**content-addressable memory（CAM）**，用內容定址的記憶體。

**10T CAM 單元** = 6T SRAM 單元 ＋ 4 個電晶體：

```
     SRAM 單元（存 V、V̄）
         │       │
 match ──┼───────┼─────── （一條共用的 match line）
         T1      T3
         │       │
         T2      T4
         └───┬───┘
            接地
```

- T1 由 **V**（存的值）控制，T2 由 **Ā**（要比較的輸入位元的反）控制，兩個**串聯**。
- T3 由 **V̄** 控制，T4 由 **A** 控制，兩個串聯。
- 兩條路徑的一端接**接地**，另一端接 **match line**。

**A == V 時**：

- 一組是 (V, Ā)：A == V，所以 V 和 Ā 其中一個一定是 0 → T1 或 T2 有一個關著 → 這條路斷了。
- 另一組 (V̄, A) 也一樣，一定有一個是 0 → 這條路也斷了。
- 所以 **match line 和接地斷開**（浮接）。

**A ≠ V 時**：

- 例：A = 1、V = 0 → V̄ = 1、A = 1 → T3、T4 都打開。
- 例：A = 0、V = 1 → V = 1、Ā = 1 → T1、T2 都打開。
- 不管哪種，一定有一組是 (1, 1)，兩個電晶體都開 → **match line 直接接到地，被拉到 0 V**。

> **重點**：A == V → match line 保持浮接；A ≠ V → match line 被拉到 0。

- 【課本補充】課本的說明和影片一樣；課本的文字用 Āᵢ、V̄ 這些**上面有橫線**的符號，用 pdftotext 抽出來時橫線會不見，讀起來像自相矛盾，要看原頁。
- 【課本補充】CAM 通常用來在硬體裡實作 **hashtable**：用 key 找 value，不限於整數索引。課本說 Chapter 11 會用 CAM 陣列做一些記憶體結構（例如 TLB）。

---

## 10. CAM 陣列

[45:00]–[50:11]　課本 Figure 7.29（p.290）

- **和 SRAM 陣列幾乎一樣**：CAM 也可以當一般的 SRAM 用（有 bit line、write driver、sense amplifier）。
- 多了一個 **CAM 模式**：所有 word line 都可以關掉；第 1 行的每個單元接 A₁ 和 Ā₁、第 2 行接 A₂ 和 Ā₂……第 n 行和第 n 個位元比較。
- **一列的所有 CAM 單元接同一條 match line**。

**比較的流程**：

1. 把所有 match line **pre-charge 到 1**，然後斷開 pre-charge 電路。
2. 開始比較。一列裡**只要有一個位元不一樣**，那個單元就把 match line 拉到 0。
3. **全部位元都一樣**的那一列，match line 不會被拉低，保持 pre-charge 的 1。

**之後**：

- 把所有 match line 送進一個 **OR 閘**：結果是 1 就表示**有找到**，0 就表示沒有任何一列符合。
- 用 **encoder** 找出是**哪一列**符合（encoder：2ⁿ 個輸入裡只有一個是 1，輸出 n 位元的編號）。
- 如果**好幾列**同時符合，用某種優先順序選一列：**priority encoder**。【口頭】「有興趣可以 Google 一下 priority encoder。」

---

## 11. 【課本補充】ROM、PROM、EPROM

課本 §7.4.4（p.293–295），影片沒講。

- **ROM**（read-only memory）：存永遠不該被修改的資料，例如手機的安全資訊、主機板的 BIOS、某些微控制器的程式。
  - 做法：把 DRAM 單元的**電容拿掉**，直接把電晶體的一端接到 V_dd（1）或接地（0）。在設計和製造晶片時就決定了。沒有 write driver 和 refresh 電路。
- **PROM**（programmable ROM）：只能程式化**一次**。用 **antifuse**（反熔絲）：平常是很差的導體；灌入很大的電流後結構改變，變成導通。
- **EPROM**（可用紫外線清除）、**EEPROM**（可用電清除）：用特殊的 floating gate 電晶體存電荷。現在已被 **flash** 記憶體取代（課本 §13.8.4）。

---

## 12. Chapter 6（課本 Ch7）總結

[50:11]–[52:22]

【口頭】「一個很短、很快的三小時數位邏輯課。」

- **電晶體**的基礎
- **邏輯閘**（書裡還有 **Karnaugh map**，影片沒講）
- **Latch 和 flip-flop**：flip-flop 就是**有時脈的 latch**（課本 Definition 也是「clocked latch」）
- **記憶體單元**：
  - **SRAM**：交叉耦合的反相器 ＋ 兩條 bit line。用兩條線的**電壓差**，不用等 bit line 完全到 0 或 1，從電壓變化的方向就能推出存的值。
  - **DRAM**：資料存在電容裡；電容會漏電，要定期讀出再寫回（**refresh**）。
  - **CAM**：10 個電晶體，用 **match line** 找內容相同的列。

下一章：**計算機算術**：硬體怎麼做加法、減法、乘法。

---

## 13. 【實測】

本機：Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10。程式在 `experiments/lecture20/`。

### 13.1 SRAM（cache）和 DRAM 的速度差多少

本機處理器的 cache 都是用 SRAM 做的：

```
$ for i in 0 1 2 3; do ... /sys/devices/system/cpu/cpu0/cache/index$i ...; done
L1 Data: 48K, 12-way, line 64 B
L1 Instruction: 32K, 8-way, line 64 B
L2 Unified: 1280K, 10-way, line 64 B
L3 Unified: 24576K, 12-way, line 64 B
$ free -h
Mem:            38Gi ...                  ← 主記憶體是 DRAM
```

用 **pointer chasing**（每次讀取的位址都取決於上一次讀到的值，所以處理器沒辦法提前讀，量到的是真正的延遲），在不同大小的陣列裡隨機跳：

```
$ gcc -O2 -o chase chase.c && ./chase
      16 KB :   1.30 ns     ← L1（SRAM）
      32 KB :   1.03 ns
     256 KB :   2.76 ns     ← L2（SRAM）
    1024 KB :   4.46 ns
    2048 KB :   8.14 ns     ← L3（SRAM）
    8192 KB :  13.57 ns
   16384 KB :  23.25 ns
   65536 KB :  77.79 ns     ← 超過 L3：要去 DRAM
  262144 KB :  95.44 ns
```

- 資料放得進 L1（48 KB）時，一次存取約 **1 ns**。
- 放不下 L3（24 MB）時，每次都要去 DRAM，約 **80～95 ns**，慢了快 100 倍。
- 這就是影片說的：SRAM 快、DRAM 慢但密（L3 只有 24 MB，主記憶體有 38 GB）。

### 13.2 Sense amplifier 快多少：RC 模型

```
$ python3 senseamp.py
wait until Δ = 0.90 V : t = 2.303 RC
wait until Δ = 0.99 V : t = 4.605 RC
sense amp threshold 0.10 V : t = 0.105 RC
sense amp threshold 0.05 V : t = 0.051 RC
speed-up (0.99 V vs 0.1 V) = 44x
```

- 模型：兩條 bit line 從 0.5 V 開始，各自以 RC 時間常數往 1 V／0 V 走，差 Δ(t) = 1 − e^(−t/RC)。
- 等到幾乎滿擺幅（0.99 V）要 **4.6 RC**；sense amplifier 只要差 **0.1 V** 就能判斷，只要 **0.105 RC**：快約 **44 倍**。這和影片說的「20 ns 變成 1 ns」是同一個數量級。

---

## 14. 一頁速查表

| 主題 | 重點 |
|---|---|
| 設計目標 | 每位元電晶體少、速度快 |
| SRAM 單元 | 交叉耦合反相器（4T）＋ 2 個 word line 電晶體 = 6T |
| Word line | 0：斷開（保持值）；1：可讀寫 |
| Bit line | BL、BL̄（互補） |
| 寫 | write driver 強制設定 BL、BL̄ |
| 讀 | bit line 浮接，讓單元決定電壓 |
| 陣列 | 同一列共用 word line（decoder 選列）；同一行共用 bit line（column mux 選行） |
| 慢的原因 | bit line 很長、接很多單元 → RC 大 |
| Pre-charge | 讀之前把 bit line 充到 V_dd／2 |
| Sense amplifier | 監看電壓差，超過雜訊門檻就判斷，不用等完全充放電 |
| 用途 | SRAM：register file、cache；少量位元用 flip-flop |
| DRAM 單元 | 1 電晶體 ＋ 1 電容；1 條 bit line；最密、最慢 |
| DRAM 讀 | pre-charge 到 0.5 V，和固定的 0.5 V 比 |
| 漏電 | 讀取分享電荷 ＋ 介電質、電晶體漏電 |
| Refresh | 讀完寫回；定期輪流讀寫所有列 |
| DRAM 陣列 | 多 refresh 電路；sense amp 暫存整列（DRAM page） |
| SDRAM／DDR | 同步；上升緣和下降緣都傳資料 |
| CAM 單元 | 6T SRAM ＋ T1T2（V、Ā）＋ T3T4（V̄、A）= 10T |
| Match line | A == V：浮接；A ≠ V：拉到 0 |
| CAM 陣列 | match line pre-charge 到 1；全部一樣的列保持 1；OR → 有沒有找到；(priority) encoder → 哪一列 |
| ROM | 電晶體直接接 V_dd 或接地；PROM 用 antifuse |

---

## 15. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| 加法器、乘法器的電路 | **第 21–26 支**：課本 Ch8 計算機算術 |
| Register file（用 SRAM 陣列做） | **第 27–29 支**：課本 Ch9 處理器設計 |
| Cache（SRAM）、主記憶體（DRAM）、TLB（CAM） | **第 34–37 支**：課本 Ch11 記憶體系統 |
| Flash、SSD、DRAM 的時序 | **第 46 支**：課本 Ch13 儲存裝置 |
| SRAM 陣列、CAM、Cacti 模型、Elmore delay | Advanced 課程 Ch7 Caches（Part III、IV） |
| DRAM 的裝置、陣列、DDR4 時序、記憶體控制器 | Advanced 課程 Ch10 Main Memory |

---

## 附：本講一句話總結

> **記憶體單元要用最少的電晶體存最多的位元：SRAM 用一對交叉耦合的反相器（4 個電晶體）加上兩個 word line 電晶體組成 6T 單元，排成陣列時同一列共用 word line（decoder 選列）、同一行共用一對 bit line（column mux 選行）；因為 bit line 很長、RC 很大，小小的 SRAM 單元很難把它拉到 0 或 1，所以讀之前先 pre-charge 到 V_dd／2，再用 sense amplifier 監看 BL 和 BL̄ 的電壓差，超過雜訊門檻就判斷（RC 模型算出約快 44 倍）。DRAM 只用一個電晶體加一個電容，最密但最慢，只有一條 bit line，和固定的 0.5 V 比較；電容會因讀取和漏電失去電荷，所以每次讀完要寫回，並定期 refresh 整個陣列。CAM 是 SRAM 加 4 個電晶體的 10T 單元，輸入和存的值不同時會把 match line 拉到 0，所以把所有 match line pre-charge 到 1 後，全部位元相同的那一列會保持 1，用 OR 判斷有沒有找到、用 (priority) encoder 找出是哪一列。實測本機 L1 cache（SRAM）一次存取約 1 ns，超過 24 MB 的 L3 之後要去 DRAM，約 80～95 ns。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 6: Digital Logic, Part 3/3 錄影逐字稿（YouTube 自動字幕，52:33）、Sarangi *Basic Computer Architecture* v3.09 Ch7 §7.4（p.286–295）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
