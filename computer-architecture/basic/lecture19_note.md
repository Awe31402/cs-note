# Basic Computer Architecture 第十九支影片完整筆記：循序邏輯：SR latch、clock、D 與 JK flip-flop（Chapter 6, Part 2/3 → 課本 Ch7）

> **課程**：Basic Computer Architecture — Chapt 6: Digital Logic, Part 2/3（播放清單第 19 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕一直下載不到）（1:00:48）＋ 課本 *Basic Computer Architecture* v3.09 **Ch7** §7.3（p.277–286）
> **章號**：影片的 Chapter 6 是課本 v3.09 的 **Chapter 7**。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 循序邏輯：先存起來，再算](#1-循序邏輯先存起來再算)
- [2. SR latch](#2-sr-latch)
- [3. S = R = 1：不合法](#3-s--r--1不合法)
- [4. 為什麼需要 clock](#4-為什麼需要-clock)
- [5. Clock、clock period、頻率](#5-clockclock-period頻率)
- [6. Clocked SR latch](#6-clocked-sr-latch)
- [7. D flip-flop](#7-d-flip-flop)
- [8. Master-slave D flip-flop](#8-master-slave-d-flip-flop)
- [9. 預告：pipeline](#9-預告pipeline)
- [10. Master-slave JK flip-flop](#10-master-slave-jk-flip-flop)
- [11. 重點整理](#11-重點整理)
- [12. 【課本補充】metastability 與 register](#12-課本補充metastability-與-register)
- [13. 【實測】](#13-實測)
- [14. 一頁速查表](#14-一頁速查表)
- [15. 與後續講次的連結](#15-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

上一講（[第 18 支](lecture18_note.md)）的組合邏輯沒有記憶。這一講要**存住一個 bit**：

```
SR latch（兩個交叉耦合的 NAND）
  → 加上 clock：clocked SR latch
    → 只剩一個輸入：D flip-flop
      → 兩個串起來：master-slave D flip-flop（只在 clock 的負緣更新）
  → 加上回授：master-slave JK flip-flop（多了 toggle）
```

---

## 1. 循序邏輯：先存起來，再算

[00:00] **循序邏輯（sequential logic）**跟組合邏輯不同：先把值**存**在某個結構裡，再拿去計算（老師畫成一朵「邏輯雲」，裡面很多閘），算完再存進另一個儲存元件。

[00:57] 第一個問題：怎麼存**一個 bit**，而且想存多久就存多久？

【課本補充】課本 p.277 列了三個需求：(1) 電路能自己維持，外部輸入撤掉後值還在；(2) 能讀出值而不破壞它；(3) 能把值設成 0 或 1。最好的方法是**回授（feedback）**：把輸出接回輸入。

---

## 2. SR latch

[01:23] 用**兩個交叉耦合的 NAND 閘**存一個穩定的值，叫 **SR latch**：

- **S**（set）：把輸出 Q 設成 1。
- **R**（reset）：把 Q 設成 0。
- 兩個輸出：Q 和 Q̄（同一個值的正常和補數形式）。
- [02:25] **交叉耦合（cross-coupled）**：N1 的輸出接到 N2 的輸入，N2 的輸出接到 N1 的輸入。

```
S̄ ──┤N1├──┬── Q
     ┌─────┘│
     │  ┌───┘
R̄ ──┤N2├──── Q̄
```

[27:51] ⚠️ 老師特別圈起來強調：「很多學生在這裡犯錯」，**輸入不是 S 和 R，是 S̄ 和 R̄**。

NAND 的關鍵性質：**只要有一個輸入是 0，輸出一定是 1**。

[02:50] 逐一分析（課本 Table 7.2，p.278）：

| S | R | S̄ | R̄ | 過程 | Q | Q̄ | 動作 |
|---|---|---|---|---|---|---|---|
| 1 | 0 | 0 | 1 | N1 有一個 0 → Q = 1；N2 輸入 1、1 → Q̄ = 0 | 1 | 0 | **set** |
| 0 | 1 | 1 | 0 | N2 有一個 0 → Q̄ = 1；N1 輸入 1、1 → Q = 0 | 0 | 1 | **reset** |
| 0 | 0 | 1 | 1 | Q = NAND(1, Q̄) = Q̄̄ = Q；Q̄ 也不變 | 舊值 | 舊值 | **保持** |
| 1 | 1 | 0 | 0 | 兩個 NAND 都有 0 → 都輸出 1 | 1 | 1 | **不合法** |

[07:43] 保持的情況，老師用 De Morgan 化簡：NAND(1, Q̄) = 1̄ + Q = 0 + Q = **Q**。

[09:41] 「這是很棒的想法」：有方法 set、有方法 reset、還有方法**保持**。S 和 R 都接地，latch 就**永遠**保持它的值，這正是我們要的。

[11:39] 老師建議：四種組合都自己算一遍，說服自己。

---

## 3. S = R = 1：不合法

[10:31] 「這是考試很常考的題目。」S = R = 1 → S̄ = R̄ = 0 → 兩個 NAND 都輸出 1 → **Q 和 Q̄ 都是 1**，不再互補。這個輸入不合法，不能同時 set 又 reset。

【課本補充】課本 p.278 說得更深：從 11 放開時，如果**先**放 S 再放 R（11 → 01 → 00），Q 會是 0；**先**放 R（11 → 10 → 00），Q 會是 1。實際上兩個訊號不可能完全同時變，所以結果無法預測，這叫 **race condition**。（本機模擬：如果真的「同時」放開，理想的閘會一直在 00 和 11 之間振盪，見 [§13.1](#131-sr-latch)。）

---

## 4. 為什麼需要 clock

[12:58] 一個數位系統有數十億個元件：加法器、SR latch、乘法器、encoder……資料要在它們之間流動。

[14:09] 設計者要確保**對的輸入在同一時間到達對的電路**。數十億個電晶體，這非常難，需要某種**排程**。

[14:28] 每個單元的延遲都不一樣：例如 SR latch 1 ns、加法器 1.6 ns、乘法器 2.9 ns。某個加法器的一個輸入在 2.6 ns 到、另一個在 2.9 ns 到……管理這些時間非常困難。

[15:17] 【口頭】學校怎麼做？**課表**。老師的學校（印度）一天分成 7 節（period），一週上 6 天，排成一個矩陣：第一節數學、第二節物理、第三節化學、然後去踢足球。物理老師上完第二節，去別班上第三節，排程變得非常容易。

[17:00] 【口頭】90 年代後期，不是每個學生都有手錶，有手錶也不一定準。所以每節課結束時**打鐘**。鐘一響，學生換位置、老師換教室。數位電路能不能也有一個「鐘」？

---

## 5. Clock、clock period、頻率

[17:47] 有：一個叫 **clock** 的訊號，送到晶片的每個地方，所有邏輯元件都收得到。

- 它是 0 和 1 之間的**方波**，0 和 1 的時間一樣長。
- [18:23] **負緣（negative edge）**：從 1 變 0 的那一刻，就是那個「鐘聲」。所有元件都以它來協調。
- [18:50] **Clock period**：從一個負緣到下一個負緣。
- [19:44] **頻率** = 1 / clock period。

| Clock period | 頻率 |
|---|---|
| 10 ms | 100 Hz |
| 500 ps | 2 GHz |
| 約 333 ps | 3 GHz |

[20:56] 買電腦說「3 GHz 處理器」：晶片裡每秒有 30 億個負緣。

⚠️ [19:44] 老師先把頻率說成「每秒的 clock period 數」，馬上更正成「1 除以 clock period」。兩個其實是同一件事（每秒有幾個 period），只是他想用後面的寫法。

[21:35] 所有元件和**設計者**都以負緣協調。例如加法器的設計者不會說「我的輸出 7.3 ns 後給你」，而是說「**5 個 clock cycle** 後給你」（clock period 和 clock cycle 可以互換）。這讓設計數位電路變得非常簡單，所以幾乎所有數位電路都有 clock。

---

## 6. Clocked SR latch

[22:23] 在 SR latch 前面再加兩個 NAND，各接一個 clock：

```
S̄′ = NAND(S, clk)     R̄′ = NAND(R, clk)     → 再接到 SR latch
```

| clock | 前面兩個 NAND 的輸出 | latch 的行為 |
|---|---|---|
| 1 | NAND(S, 1) = S̄、NAND(R, 1) = R̄ | 就是一般的 SR latch：可以 set、reset |
| 0 | 兩個都是 1 | 等於 S = R = 0：**保持** |

[24:40] 老師的慣例：**有 clock 的 latch 也叫 flip-flop**。所以這個也可以叫 SR flip-flop。

【課本補充】課本 p.280 也說「clocked latch 通常叫 flip-flop」。課本再區分兩種：**level sensitive**（clock 是 1 的整段時間，輸入都會影響輸出，例如這個）和 **edge sensitive**（只在某個 clock 邊緣才把輸入反映到輸出）。

---

## 7. D flip-flop

[25:10] SR 要兩個輸入（set 時 S = 1、R = 0；reset 時 S = 0、R = 1），很麻煩。把它們**統一成一個輸入 D**。

[25:56] 老師貼了「important」標籤：D flip-flop 用得**非常非常多**。SR latch 適合拿來教學，實際上用的大多是 D flip-flop。

電路（課本 Figure 7.21，p.283）：

```
I1 = NAND(D, clk)
I2 = NAND(I1, clk)
I1、I2 接到 SR latch 的 S̄、R̄
```

| clock | I1 | I2 | S | R | Q |
|---|---|---|---|---|---|
| 1 | D̄ | D | D | D̄ | **= D** |
| 0 | 1 | 1 | 0 | 0 | 保持 |

- [28:35] clock 是 1：D 是 1 → S = 1 → Q = 1；D 是 0 → R = 1 → Q = 0。Q 就是 D，只是經過閘有一點延遲，所以叫 **D（delay）**。
- [30:53] 負緣之後 clock 是 0，值穩定下來；只要 clock 是 0，flip-flop 就保持它的值。

【課本補充】課本 p.283：它基本上就是 R = S̄ 的 SR flip-flop。代價是**沒有「保持」的輸入組合**：每個負緣都會把 D 反映到輸出。很多情況這就夠了。

---

## 8. Master-slave D flip-flop

[31:19] 還沒把負緣用到極致。再複雜一點：**兩個 D flip-flop 串起來**。

- 第一個叫 **master**，接 **clock**。
- 第二個叫 **slave**，接 **clock 的補數**。

[32:54] 運作：

| clock | master | slave | 結果 |
|---|---|---|---|
| 1 | 打開：D 傳到中間點（D′ = D） | 關著：Q 保持 | Q 不變 |
| 1 → 0（負緣） | 關上：中間點固定 | 打開：Q ← D′ | **Q 變成負緣前一刻的 D** |
| 0 | 關著：D 怎麼變都不影響 | 打開，但中間點不變 | Q 不變 |

[34:06] 在負緣附近看：clock 還是 1 時中間點是 D，負緣一過，Q 就被設成 D。**只有這一刻**值會從一邊移到另一邊。負緣就是學校的鐘聲。

[34:57] 把 clock 停掉，值就一直保持。

⚠️ [37:55] 老師想了一下說：D flip-flop 要保持值，**只能停掉 clock**；SR flip-flop 可以設 S = R = 0，但 D flip-flop 沒有這種組合，D 每個負緣都會被讀進去。

【課本補充】課本 Figure 7.22（p.283–284）的 master-slave D flip-flop 是「一個 D flip-flop 接一個 SR flip-flop」，跟影片「兩個 D flip-flop」畫法不同，效果一樣。課本 p.284：master-slave flip-flop 用 **8 個 NAND ＋ 1 個反相器**；master-slave D flip-flop 要 **34 個電晶體**、JK 要 **38 個**，「只存 1 個 bit 太浪費了」。之後會看到 1 個電晶體就能存 1 bit，但很慢。flip-flop 很快、但很耗電晶體和功率；要存大量資料，就用 SRAM、DRAM（[第 20 支](lecture20_note.md)）。

（本機模擬見 [§13.2](#132-master-slave-d-flip-flop)。）

---

## 9. 預告：pipeline

[35:30] 為什麼要 master-slave？第 9 章（課本 Ch10）會更清楚，先預告：

```
[FF] → （邏輯雲） → [FF] → （邏輯雲） → [FF]
```

1. 負緣：資料出現在一組 flip-flop 的輸出。
2. 一個 clock cycle 內，邏輯閘把值算好。
3. 下一個負緣之前值已經準備好，存進下一組 flip-flop。
4. 下一個 cycle，它們出現在那組 flip-flop 的輸出。

[36:33] 這是數位電路非常重要的設計策略，叫 **pipelining**。

[37:18] 唯一要記住的：輸入要在負緣**之前一點點**穩定下來；負緣一到，輸出反映輸入。

---

## 10. Master-slave JK flip-flop

[38:27] 跟 master-slave D flip-flop 類似，稍微複雜一點。拆成兩半：

- **Slave**：就是一個 clocked SR latch，接 clock 的補數。
- **Master**：也是 clocked SR latch，但最前面兩個 NAND 是**三個輸入**：
  - I1 = NAND(**J**, clk, **Q̄**)
  - I2 = NAND(**K**, clk, **Q**)
  - 輸入叫 J、K，不叫 S、R；Q、Q̄ 從輸出**回授**回來。
- 中間點叫 T1（只是某條線上的值，不是另外的元件）。

[40:54] 一樣是負緣觸發。clock = 1 時 master 打開、slave 保持，只看 master：

| J | K | 分析（clock = 1） | 負緣前的 T1 | 負緣後的 Q |
|---|---|---|---|---|
| 0 | 0 | I1 = I2 = 1 → 保持；T1 原本就等於 Q | Q | Q_old（**保持**） |
| 1 | 0 | I2 = 1、I1 = NAND(1, 1, Q̄) = Q。分 Q = 0（T1 被 set 成 1）和 Q = 1（保持，T1 = Q = 1）兩種，都是 1 | 1 | 1（**set**） |
| 0 | 1 | 對稱，兩種情況 T1 都是 0 | 0 | 0（**reset**） |
| 1 | 1 | I1 = Q、I2 = Q̄ → S = Q̄、R = Q | Q̄ | Q̄_old（**toggle**） |

[52:20] J = K = 1 時，T1 被設成 Q 的補數：**toggle（翻轉）**。這是 SR latch 沒有的新功能。

[54:30] 負緣之後 master 關、slave 開，slave 只是把 T1 搬到 Q。

【課本補充】課本 Table 7.3（p.283）跟上表一樣，並強調：J = K = 1 時**不會**有 race condition（SR 的 1、1 會有）。

---

## 11. 重點整理

[57:06]

1. **為什麼要 clock**：跟學校打鐘一樣，同步所有邏輯元件。輸入在負緣前準備好，負緣時（我們假設負緣觸發）新的 cycle 開始，輸入傳到輸出。
2. **能跨 cycle 保持值**：例如 JK 設 J = K = 0。
3. [58:56] 有了這些，可以做出**計數器**、**除頻器（frequency divider）**和很多數位電路。老師推薦 **Kohavi** 的 switching theory。【口頭】例如紅綠燈控制器：一組 flip-flop、一個 clock、一些輸入，資料在 flip-flop 之間流動。
4. [60:05] Flip-flop 很快，但電晶體很多。**記憶體單元**密得多，同樣面積能存多很多資料，下一講。

---

## 12. 【課本補充】metastability 與 register

影片沒講，課本 §7.3.8–7.3.9（p.284–286）：

**Metastability（亞穩態）**：每個數位電路本質上都是類比電路。如果輸入在**負緣附近**改變，輸出會不確定，甚至振盪一段時間。所以輸入必須：

- 在 clock 邊緣**之前** t_setup 時間內不變（**setup time**）。
- 在 clock 邊緣**之後** t_hold 時間內不變（**hold time**）。

這個時間窗叫 **keep out region**，通常小於 cycle 的 10%。I/O 電路會特別注意這件事。

**Register（暫存器）**：n 個 master-slave D flip-flop 並排。

| 種類 | 怎麼接 |
|---|---|
| parallel in–parallel out | 每個 flip-flop 各有輸入和輸出線，n bit 一次寫、一次讀 |
| serial in–parallel out（**shift register**） | 只有一個輸入，每個 cycle 往右移一格，n 個 cycle 載完 n bit，再一次讀出 |

---

## 13. 【實測】

`experiments/lecture19/nandsim.py`：一個**只有 NAND 閘**的電路模擬器。每個閘延遲 1 個時間單位：下一個時間點的輸出 = 這個時間點輸入的 NAND（所有閘同時更新）。所有電路都照影片／課本的接法，只用 NAND 組出來（反相器 = NAND(x, x)）。

### 13.1 SR latch

```
SR latch：
  S=1 R=0（set）  → Q=1 Q̄=0
  S=0 R=0（保持） → Q=1 Q̄=0
  S=0 R=1（reset）→ Q=0 Q̄=1
  S=0 R=0（保持） → Q=0 Q̄=1
  S=1 R=1         → Q=1 Q̄=1 ← Q 和 Q̄ 都是 1，不是互補
  11 → 10 → 00（不同時放開）→ Q=1 Q̄=0
  11 → 01 → 00（不同時放開）→ Q=0 Q̄=1
  11 → 00（同時放開），每一步的 QQ̄： 00 11 00 11 00 11 00 11 ← 一直振盪，停不下來
```

- set、reset、保持都對，跟 Table 7.2 一樣。
- S = R = 1：Q = Q̄ = 1，影片說的「不合法」。
- 從 11 放開：**誰先放開決定結果**（課本的 race condition）。
- 如果兩個閘完全一樣快、而且真的同時放開，兩個 NAND 會一起從 1 變 0、再一起變 1……永遠振盪。真實的閘總有一點差別，最後會落到某一邊，但落到哪邊無法預測。這也是 [§12](#12-課本補充metastability-與-register) metastability 的一個縮影。

### 13.2 Master-slave D flip-flop

每格是半個 clock 週期，記錄那半週期結束時的值（起始值都是 1）：

```
  clk           1  1  0  0  1  1  0  0  1  1  0  0  1  1  0  0
  D             1  1  0  0  0  1  1  1  1  0  0  0  0  1  1  1
  主 (master)    1  1  1  1  0  1  1  1  1  0  0  0  0  1  1  1
  Q (slave)     1  1  1  1  1  1  1  1  1  1  0  0  0  0  1  1
```

- **master** 在 clock = 1 時跟著 D 走（第 5 格 D = 0 → master = 0；第 6 格 D 又變 1 → master = 1）。
- **slave（Q）只在 clock 從 1 變 0 之後才改**：第 5 格的 D = 0 **沒有**被存下來，因為負緣前 D 已經變回 1。
- clock = 0 時 D 變了（第 3 格），master 和 Q 都不理它。
- 這就是「只在負緣取樣」：負緣**前一刻**的 D 才算數。

### 13.3 JK flip-flop：除頻和計數

J = K = 1（toggle），三個 JK 串成**漣波計數器（ripple counter）**：下一級的 clock 接上一級的 Q。每個負緣之後讀一次：

```
  半週期 12 個時間單位：第 1 級 Q = 1 0 1 0 1 0 1 0 1 ｜計數 = 1 2 3 0 5 6 7 4 1
  半週期 30 個時間單位：第 1 級 Q = 1 0 1 0 1 0 1 0 1 ｜計數 = 1 2 3 4 5 6 7 0 1
```

- 第 1 級：每個負緣翻一次，**頻率變一半**。這就是影片 [58:56] 說的除頻器。
- 三級串起來：0 → 7 一直數下去，就是**計數器**。
- **半週期太短（12）時，讀到的數是錯的**（3 之後讀到 0、7 之後讀到 4）：第 3 級要等第 1、2 級一級一級翻完才會翻，漣波的延遲累積起來超過了 12 個時間單位，讀的時候還沒翻完。半週期加長到 30 就對了。這正是 [§4](#4-為什麼需要-clock) 說的「每個單元的延遲不同，要靠 clock 給足夠的時間」：clock period 一定要比最長的那條路徑還長。

⚠️ 寫這個模擬時，我一開始讓所有線都從 1 開始，結果第 2 級的 clock 和反相 clock **同時是 1**，master 和 slave 一起打開，掉進跟 §13.1 一樣的 11 ↔ 00 振盪，計數器完全不動。改成一致的起始狀態（clock = 0、每一級 Q = 0）才正常。真實電路也一樣：開機要有 **reset**，把 flip-flop 放到已知的狀態。

### 13.4 本機的 clock period

本機 P-core 最高 4.7 GHz（[第 47 支 §17](lecture47_note.md#17-實測)），clock period = 1 / 4.7 GHz ≈ **213 ps**。影片的例子 2 GHz 是 500 ps。這 213 ps 內，從一組 flip-flop 出發的訊號要穿過整朵邏輯雲、在下一個負緣前（扣掉 setup time）到達下一組 flip-flop。

---

## 14. 一頁速查表

| 主題 | 重點 |
|---|---|
| 循序邏輯 | 存起來再算；靠回授 |
| SR latch | 兩個交叉耦合的 NAND；輸入是 **S̄、R̄** |
| SR 表 | 10 set、01 reset、00 保持、11 不合法（Q = Q̄ = 1） |
| Race condition | 從 11 放開時，誰先放決定結果 |
| Clock | 方波；負緣（1 → 0）是「鐘聲」 |
| 頻率 | 1 / clock period；2 GHz = 500 ps |
| 用 cycle 溝通 | 「5 個 cycle 後給你」，不說幾 ns |
| Clocked SR | 前面多兩個 NAND(S, clk)、NAND(R, clk)；clk = 0 保持 |
| Flip-flop | 有 clock 的 latch；level sensitive vs edge sensitive |
| D flip-flop | 一個輸入；clk = 1 時 Q = D；保持只能停 clock |
| Master-slave D | master 接 clk、slave 接 clk̄；只在負緣更新 |
| 電晶體數 | master-slave D 34 個、JK 38 個 |
| JK | 三輸入 NAND（J、clk、Q̄）（K、clk、Q）；00 保持、10 set、01 reset、11 **toggle** |
| 應用 | pipeline、計數器、除頻器、紅綠燈 |
| Metastability | 輸入要避開 setup time ＋ hold time（keep out region） |
| Register | n 個 D flip-flop；parallel in–parallel out、shift register |
| 實測 | NAND 模擬驗證 SR 表與 race；D 只取負緣前的值；JK 除頻、計數；clock 太快計數器讀錯 |

---

## 15. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| NAND 閘、組合邏輯 | [第 18 支](lecture18_note.md) |
| SRAM、DRAM：更密的儲存 | [第 20 支](lecture20_note.md) |
| 處理器裡的 register file、PC（都是 flip-flop） | [第 27 支](lecture27_note.md) |
| Pipeline register、在負緣寫入 | [第 30 支](lecture30_note.md) |
| Clock period 和 pipeline 級數 | [第 32 支 §13](lecture32_note.md#13-理想-pipeline-的-cpi-與頻率) |
| 頻率、功耗 | [第 47 支 §11](lecture47_note.md#11-現在power-wall-與-think-parallel) |
| I/O 的同步、metastability | [第 43 支](lecture43_note.md) |

---

## 附：本講一句話總結

> **循序邏輯靠回授存住 bit：兩個交叉耦合的 NAND 就是 SR latch（輸入其實是 S̄ 和 R̄），SR = 10 set、01 reset、00 保持，11 不合法（Q 和 Q̄ 都變 1，放開時還有 race condition）；為了讓數十億個延遲各不相同的元件同步，數位電路像學校打鐘一樣用 clock，以負緣為準，頻率是 clock period 的倒數；在 SR latch 前加兩個接 clock 的 NAND 就是 clocked SR latch（clock 為 0 時保持），把 S、R 合成一個輸入就是 D flip-flop（clock 為 1 時 Q = D），兩個串起來、slave 接反相的 clock 就是只在負緣更新的 master-slave D flip-flop，這是 pipeline 的基礎；master 前面的 NAND 改成三輸入並把 Q、Q̄ 回授回來，就是多了 toggle（J = K = 1）的 JK flip-flop，可以做除頻器和計數器。本機用只有 NAND 的模擬器驗證了 SR 表和 race、D flip-flop 只取負緣前的值、JK 的除頻與計數，也看到 clock 太快時漣波計數器會讀錯。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 6: Digital Logic, Part 2/3 的 whisper 轉錄稿（本機 `large-v3-turbo`，1:00:48）、Sarangi *Basic Computer Architecture* v3.09 Ch7 §7.3（p.277–286）整理。實測為本機（Python 3.10）結果。*
