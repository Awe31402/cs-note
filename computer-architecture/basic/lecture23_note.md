# Basic Computer Architecture 第二十三支影片完整筆記：O(log² n) 乘法器、carry save adder 與 Wallace tree（Chapter 7 Computer Arithmetic, Part 3/6 → 課本 Ch8）

> **課程**：Basic Computer Architecture — Chapt 7: Computer Arithmetic, Part 3/6（播放清單第 23 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（22:09，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 **Ch8** §8.2.4–8.2.5（p.330–333）
> **章號**：影片的 Chapter 7 是課本 v3.09 的 **Chapter 8**。
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
- [1. O(log² n) 的乘法器：加法器樹](#1-olog²-n-的乘法器加法器樹)
- [2. Carry save adder：三個數變兩個數](#2-carry-save-adder三個數變兩個數)
- [3. 多位元的 carry save adder](#3-多位元的-carry-save-adder)
- [4. Wallace tree multiplier：O(log n)](#4-wallace-tree-multiplierolog-n)
- [5. 乘法器總結](#5-乘法器總結)
- [6. 【實測】](#6-實測)
- [7. 一頁速查表](#7-一頁速查表)
- [8. 與後續講次的連結](#8-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

上一講的 iterative 和 Booth 乘法器是 **O(n log n)**：n 個部分和**一個接一個**加。這一講利用**平行**：

| 乘法器 | 怎麼加 n 個部分和 | 時間 |
|---|---|---|
| Iterative／Booth（上一講） | 一個接一個，n 次加法 | O(n log n) |
| **加法器樹** | 兩兩相加，log n 層，每層一次 CLA | **O(log² n)** |
| **Wallace tree** | 用 **carry save adder** 三個變兩個，log₃/₂ n 層，每層 O(1)；最後一次 CLA | **O(log n)** |

---

## 1. O(log² n) 的乘法器：加法器樹

[00:46]–[03:49]　課本 §8.2.4、Figure 8.12（p.330）

**Step 1：同時產生所有部分和**

- n 位元的乘數和 n 位元的被乘數，會有 **n 個部分和**：乘數的某一位是 1 就寫下被乘數（左移對應的位數），是 0 就寫 0。
- **所有部分和可以同時產生**，**常數時間**。
- 【課本補充】產生第 i 個部分和，就是把乘數的第 i 位和被乘數的**每一位做 AND**，O(1)。

**Step 2：用加法器樹把 n 個數加起來**

```
 P_n  P_n-1   P_n-2  P_n-3   …   P_2   P_1
   \   /        \   /             \   /
   [ + ]        [ + ]     …       [ + ]
       \        /                   /
        [  +  ]       …         …
             \                /
                [    +    ]          ← log n 層
                     │
                   乘積
```

- 第 1、2 個相加，第 3、4 個相加……兩兩一組，**同一層同時做**。
- 每一層，要加的數就**少一半**，所以有 **log n 層**。
- 數字的大小：第一個部分和 n 位元，最後一個是 **2n − 1** 位元（後面補了 n − 1 個 0）。加 2n 位元的數用 CLA 要 O(log 2n) = **O(log n)**（忽略常數，對數的底也不重要）。
- **總時間 = 層數 × 每層時間 = log n × log n = O(log² n)**。
- 【口頭】這比之前的 O(n log n) **好得多**。「但我們還能做得更好，好得多。」

【課本補充】課本這一節「讓生活簡單一點」：**乘積也只存成 n 位元**、忽略 overflow，所以直接用無號數乘法（[第 4 支](lecture04_note.md#7-加法減法乘法) 證明過，沒有 overflow 時有號數也對）。「在高效能乘法器裡偵測 overflow 相當複雜，超出本書範圍。」

---

## 2. Carry save adder：三個數變兩個數

[03:49]–[07:59]　課本 Figure 8.13（p.331）

【口頭】「在討論怎麼做得更好之前，要介紹一個新裝置：**carry save adder**（CSA）。」

```
A ─┐
B ─┤  CSA  ├─ D
C ─┘       └─ E          A + B + C = D + E
```

- 輸入**三個數** A、B、C，輸出**兩個數** D、E，而且 **A + B + C = D + E**。

**1 位元的 CSA**：三個 1 位元數 a、b、c，找 d、e（都是 1 位元）使得

```
a + b + c = 2d + e
```

- 最小：0 + 0 + 0 = 0 → d = 0、e = 0。
- 最大：1 + 1 + 1 = 3 = `11` = 2 × 1 + 1 → d = 1、e = 1。
- 0～3 都能寫成 2d + e。
- 三個 1 位元數相加，結果最多 2 位元：**左邊是 carry、右邊是 sum**。所以：
  - **d = carry 位元**（比較高的那位）
  - **e = sum 位元**
- 這不就是 **full adder** 嗎？對，一個 1 位元的 CSA 就是一個 full adder，只是輸出的解讀不同。

---

## 3. 多位元的 carry save adder

[07:59]–[14:28]　課本 Equation 8.12–8.14（p.331–332）

```
A + B + C = Σ Aᵢ 2ⁱ⁻¹ + Σ Bᵢ 2ⁱ⁻¹ + Σ Cᵢ 2ⁱ⁻¹
          = Σ (Aᵢ + Bᵢ + Cᵢ) 2ⁱ⁻¹
          = Σ (2Dᵢ + Eᵢ) 2ⁱ⁻¹            ← 每一位用 1 位元 CSA
          = Σ Dᵢ 2ⁱ   +   Σ Eᵢ 2ⁱ⁻¹
            └── 2D ──┘   └── E ──┘
```

**做法**：

1. 對**每一位** i，把 Aᵢ、Bᵢ、Cᵢ 送進一個 full adder，**所有位同時做**：
   - Dᵢ = carry 位元
   - Eᵢ = sum 位元
2. **E** = 把所有 Eᵢ 排在一起。
3. **D′ = 2D** = 把所有 Dᵢ 排在一起，再**左移一位**。
4. A + B + C = D′ + E。

**為什麼是 O(1)**：

- 每一位的加法互相**獨立**，同時進行：常數時間。
- 把位元排成數字：只是接線，常數時間。
- 左移一位：只是接線，常數時間。
- 【口頭】**關鍵**：CSA **不把 carry 往左傳**（不 ripple），而是把所有的 carry **存起來**（save）成另一個數 D。這就是名字的由來，也是它能 O(1) 的原因。
- 【課本補充】硬體成本不高：**n 個 full adder**，再把線接好。

---

## 4. Wallace tree multiplier：O(log n)

[14:28]–[20:40]　課本 §8.2.5、Figure 8.14（p.332–333）

**Step 1**：同時產生 n 個部分和：乘數第 i 位是 0 → Pᵢ = 0；是 1 → Pᵢ = 被乘數左移 i − 1 位。O(1)。

**Step 2**：用一棵 **CSA 樹**把 n 個數減少成 **2 個**

```
 P_n P_n-1 P_n-2   P_n-3 P_n-4 P_n-5   …   P_3 P_2 P_1
   \   |   /         \   |   /              \  |  /
    [CSA]             [CSA]         …        [CSA]
    /   \             /   \                  /   \
       ……  每一層：每 3 個數變 2 個  ……
                     │
                   [CSA]
                   /   \
                  X     Y        X + Y = 所有部分和的總和
```

- 把部分和每 **3 個一組**送進 CSA，變成 2 個。所以每一層，數字的個數變成原來的 **2/3**（除以 3/2）。
  - 課本：第 1 層用 n/3 個 CSA，剩 2n/3 個數；第 2 層用 2n/9 個 CSA，剩 4n/9 個……
- 一直做到**只剩 2 個數** X、Y 為止：約 **log₃/₂ n 層**。
- 每一層的 CSA **同時**運作，每層 **O(1)**。
- 所以 Step 2 是 O(log₃/₂ n) = **O(log n)**（對數的底只差一個常數倍）。

**Step 3**：用一個 **carry lookahead adder** 把 X 和 Y 加起來，O(log n)。（X、Y 大約 2n 位元，不影響複雜度。）

**總時間 = O(log n) + O(log n) = O(log n)**。

- 這棵 CSA 樹叫做 **Wallace tree**。
- 【口頭】「這是**最快**的乘法器，在商用電路裡非常、非常常用。它快的原因就是 carry save adder：一個很快的單元，三個輸入、兩個輸出、總和不變。」
- 【課本補充】以漸近時間複雜度來說，這是**最快的可能**的乘法器。稍微修改設計可以減少 full adder 的數量，叫 **Dadda multiplier**。

⚠️ 影片和課本這一講都把部分和當成「被乘數左移」，這是**無號數**的做法。要算有號數的 2n 位元乘積，最後一個部分和（乘數的 MSB）要像上一講的 iterative multiplier 一樣用**減的**（或者用 Booth 編碼、Baugh-Wooley 等方法處理符號），影片沒有講。§6.1 實測只做無號數。

---

## 5. 乘法器總結

[20:40]–[21:46]

- 用一排 CSA 把三個數變兩個數，每一層數字的個數乘以 2/3。
- log₃/₂ n 層之後只剩兩個數，用 carry lookahead adder 加起來，O(log n)。
- 總時間 **O(log n)**。

【口頭】前三支影片講了加法和乘法；接下來講**除法、浮點數加法、浮點數乘法、浮點數除法**。這章很長，所以投影片分成兩份：「Chapter 7 Part 1」到這裡結束，接下來用「Chapter 7 Part 2」的投影片。

| 方法 | 時間 |
|---|---|
| Iterative multiplier | O(n log n) |
| Booth multiplier | O(n log n)（實際上常常比較快） |
| 加法器樹 | O(log² n) |
| **Wallace tree** | **O(log n)** |

---

## 6. 【實測】

本機：Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10。程式在 `experiments/lecture23/`。

### 6.1 三種乘法器的閘延遲

沿用[第 21 支](lecture21_note.md#12-實測)的閘級模型（每個 AND／OR／XOR 延遲 1），`wallace.py` 實作三種**無號**乘法器（乘積 2n 位元）：

- **iterative**：n 個部分和用 CLA 一個接一個加。
- **log² tree**：兩兩相加的 CLA 樹。
- **Wallace**：每一位一個 full adder 的 CSA，每 3 個變 2 個，剩 2 個時用一個 CLA。

```
$ python3 wallace.py
  n  iterative  log² tree  Wallace  CSA levels  correct
  4         21         16       14           2  True
  8         53         27       22           4  True
 16        125         41       30           6  True
 32        287         59       38           8  True
```

- 三種都算對（包括 (2ⁿ − 1)² 這種最大的情況）。
- **iterative**：n 加倍，延遲變成兩倍多（n 次加法，每次 log n）：O(n log n)。
- **log² tree**：n 每加倍，延遲多的量越來越大（14、18）：O(log² n)。
- **Wallace**：n 每加倍，延遲固定多 **8**：O(log n)。其中 CSA 的層數每次多 2（約 log₁.₅ 的速度），每層一個 full adder（4 個閘延遲）。
- n = 32 時，Wallace 只要 38 個閘延遲，iterative 要 287 個。

### 6.2 真正的處理器：乘法只比加法慢 3 倍

一長串**互相依賴**的指令（每條都要等上一條的結果），量的是延遲（latency）：

```
$ gcc -O2 -o lat lat.c && taskset -c 2 ./lat
add : 0.60 TSC ticks per instruction
imul: 1.80 TSC ticks per instruction
imul / add = 3.0   (x=0)
```

- 一條 64 位元的 `imul` 的延遲剛好是 `add` 的 **3 倍**。add 是 1 個時脈週期，所以 imul 是 **3 個週期**。（TSC 的頻率和核心的頻率不同，所以只看比例。）
- 如果是 iterative 乘法器，64 位元要做約 64 次加法；3 個週期做完 64 位元乘法，表示硬體用的是 **Wallace tree 這類的樹狀乘法器**，再加上 pipeline。
- （x = 0 是因為前面 add 的迴圈一直把 x 乘 2，加 64 次之後就溢位成 0，之後的 imul 都是 0 × 0。這不影響量到的時間：x86 的 imul 是固定延遲，不會因為輸入是 0 而變快。）

---

## 7. 一頁速查表

| 主題 | 重點 |
|---|---|
| 產生部分和 | 乘數每一位 AND 被乘數，n 個同時做，O(1) |
| 加法器樹 | 兩兩相加，log n 層，每層 O(log n) 的 CLA → O(log² n) |
| Carry save adder | A + B + C = D + E；三個數變兩個 |
| 1 位元 CSA | 就是 full adder：d = carry、e = sum；a + b + c = 2d + e |
| n 位元 CSA | 每一位一個 full adder，同時做；E = sum 位元；D = carry 位元左移一位；O(1) |
| 為什麼快 | carry 不往左傳，而是「存」成另一個數 |
| Wallace tree | CSA 樹：每層 × 2/3，log₃/₂ n 層剩 2 個數，再一個 CLA → O(log n) |
| Dadda | Wallace 的變形，用比較少的 full adder |
| 有號數 | 部分和要處理符號（最後一列減、Booth 編碼等） |
| 實際 | x86 的 64 位元 imul 約 3 個週期 |

---

## 8. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| 除法：restoring、non-restoring | [第 24 支](lecture24_note.md)：Computer Arithmetic Part 4 |
| 浮點數乘法要把兩個 mantissa 相乘 | [第 25](lecture25_note.md)–[26 支](lecture26_note.md) |
| 乘法單元在 pipeline 裡占好幾個 stage | [第 30](lecture30_note.md)–[33 支](lecture33_note.md)：課本 Ch10 pipeline |
| 硬體裡的 latency 和 throughput | Advanced 課程 Ch2 out-of-order pipeline |
| 機器學習加速器的乘加（MAC）陣列 | Advanced 課程 Ch14 |

---

## 附：本講一句話總結

> **n 位元乘法的 n 個部分和可以同時產生（乘數每一位 AND 被乘數，O(1)），如果用兩兩相加的加法器樹，log n 層、每層一次 O(log n) 的 carry lookahead 加法，得到 O(log² n)；更好的做法是 carry save adder：每一位用一個 full adder 同時把 A、B、C 三個數加成 sum 位元組成的 E 和 carry 位元組成再左移一位的 D，A + B + C = D + E，因為 carry 不往左傳而是存起來，所以只要 O(1)。Wallace tree 用 CSA 每層把數字的個數乘以 2/3，log₃/₂ n 層後只剩兩個數，再用一個 carry lookahead adder 加起來，總共 O(log n)，是漸近上最快、商用電路最常用的乘法器（Dadda 是用較少 full adder 的變形；有號數要另外處理符號）。實測閘級模型 32 位元時 iterative 要 287 個閘延遲、加法器樹 59、Wallace 只要 38；本機 64 位元 imul 的延遲只有 add 的 3 倍（3 個週期），正是樹狀乘法器的效果。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 7: Computer Arithmetic, Part 3/6 錄影逐字稿（YouTube 自動字幕，22:09）、Sarangi *Basic Computer Architecture* v3.09 Ch8 §8.2.4–8.2.5（p.330–333）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
