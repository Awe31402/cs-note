# Advanced Computer Architecture 第二十三支影片完整筆記：SRAM 與 CAM 陣列：6T cell、precharge、sense amplifier、10T CAM cell（Chapter 7, Part III）

> **課程**：Advanced Computer Architecture — Chapter 7: Caches (Part III) -- SRAM and CAM arrays（播放清單第 23 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕下載不到）（46:05）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch7** §7.3.1（p.277–289）
> **章號**：影片 Chapter 7 = 課本 Ch7。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Python 3.10）實際執行的模擬結果（本機沒有 SPICE 之類的電路模擬器，只做開關層級和 RC 的計算）
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 的結尾 [45:45] 之後是沒有意義的字（「Peace out! and away ANY belga…」），影片在 [45:45]「接下來是 CACTI」就結束了。[42:09] 老師說投影片標題寫錯了（複製貼上投影片的錯），應該是「CAM array」。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 背景：latch 與 flip-flop](#1-背景latch-與-flip-flop)
- [2. 交叉耦合的反相器](#2-交叉耦合的反相器)
- [3. 6T SRAM cell](#3-6t-sram-cell)
- [4. SRAM 陣列](#4-sram-陣列)
- [5. 讀與寫](#5-讀與寫)
- [6. 用 pass transistor 做 mux](#6-用-pass-transistor-做-mux)
- [7. 把 cache 放上 SRAM 陣列](#7-把-cache-放上-sram-陣列)
- [8. 為什麼要 precharge](#8-為什麼要-precharge)
- [9. Sense amplifier](#9-sense-amplifier)
- [10. 10T CAM cell](#10-10t-cam-cell)
- [11. CAM 陣列](#11-cam-陣列)
- [12. SRAM 和 CAM 各用在哪](#12-sram-和-cam-各用在哪)
- [13. 【實測】](#13-實測)
- [14. 一頁速查表](#14-一頁速查表)
- [15. 與後續講次的連結](#15-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
設計 cache = 面積、時間、功耗、效能的取捨
兩種晶片上的記憶體技術：SRAM 陣列、CAM 陣列
SRAM cell = 交叉耦合的兩個反相器（4 顆）＋ 兩顆 access transistor = 6T
陣列：row decoder → word line；每一行一對 bit line → column mux／demux → sense amplifier／write driver
cell 很弱 → precharge bit line 到 0.5 V，差超過雜訊門檻就判斷 → sense amplifier 放大差異
CAM cell = 6T SRAM ＋ 4 顆比較用的 NMOS = 10T；match line precharge 成 1，不相等就被拉到地
CAM 陣列：所有 match line → OR（hit）＋ priority encoder（index）
```

---

## 1. 背景：latch 與 flip-flop

[00:00] 設計 cache 是很複雜的工程問題：**面積、時間、功耗**對**效能**的影響互相牽扯。兩個基本技術：**SRAM 陣列**和 **CAM 陣列**。

[01:19] 需要先知道（數位電子學的基本課程）：

| 元件 | 說明 |
|---|---|
| **SR latch** | 一對**交叉耦合**的 NAND gate：一個的輸出是另一個的輸入；輸出通常互補 |
| **JK flip-flop** | |

[02:07] 【口頭】latch 和 flip-flop 常被混用（「誤用、濫用、或兩者的任意組合」）。flip-flop 通常指 negative edge triggered 的電路；pipeline 的「pipeline latch」其實是 edge triggered 的。

---

## 2. 交叉耦合的反相器

[03:06] 一對 NAND gate 的電晶體太多，做不出**又密又小**的記憶體陣列。

[03:30] 改用**一對交叉耦合的 CMOS 反相器**：

- 一邊是 Q，另一邊就是 Q̅。
- 跟 clock **無關**，也不是 edge triggered。
- 重點：**任何交叉耦合的結構，因為互相回饋，都能存一個 bit。**

[04:40] 只要 **4 顆電晶體**。兩種穩定狀態：左 0 右 1，或左 1 右 0。

【課本補充】Basic 課程用開關模型模擬過 CMOS 閘和 latch：[Basic 第 18 支](../basic/lecture18_note.md)、[Basic 第 19 支](../basic/lecture19_note.md)。

---

## 3. 6T SRAM cell

[06:03] 老師改口：那 4 顆還不是完整的 SRAM cell。再加兩顆 **access transistor**（W1、W2，也叫 **word line transistor**）→ **6 顆電晶體**的 **SRAM cell**。

[06:44] 電晶體就是開關：

| Word line | Access transistor | 結果 |
|---|---|---|
| 1 | 導通 | 交叉耦合的反相器接到兩條 **bit line**（BL、BL̅，在 cell 兩側，值互補） |
| 0 | 斷開 | cell 的值不受 bit line 的電壓影響 |

[07:35] 存取一個 cell：把 word line 設成 1。

---

## 4. SRAM 陣列

[07:35]

```
          BL BL̅    BL BL̅    BL BL̅
row decoder ─ word line 0 ─ [cell] ─ [cell] ─ [cell] …
（n bit 的  ─ word line 1 ─ [cell] ─ [cell] ─ [cell] …
 row address）    …
                  ↓        ↓        ↓
              column mux／demux
                  ↓
         sense amplifier（讀）／ write driver（寫）
```

- 每一**行**（column）的 cell 共用一對 bit line。
- 每一**列**（row）共用一條 word line。
- [08:24] **Decoder**：n bit 的輸入 → 2ⁿ 條輸出，只有一條是 1（例：0011 → 第 3 條）。row address n bit → 2ⁿ 列。被選中的那一列，所有 cell 都接到 bit line。

---

## 5. 讀與寫

### 5.1 讀

[09:01] 被選中那一列的每個 cell 把自己的值放上 bit line（一對可能是 0 1、另一對 1 0）。

[09:51] 一列可能很寬（例如 64 byte = **512** 個 cell），但只要讀 4 byte（**32** 個）→ **column mux** 從 512 個選 32 個。

[10:42] 再送到 **sense amplifier**：感測一對 bit line 的**電壓差**，轉成邏輯 0 或 1，送給 CPU。SRAM cell 是很弱的驅動者（[§8](#8-為什麼要-precharge)）。

### 5.2 寫

[11:32] 很類似，反過來：

1. 不用 sense amplifier；要寫的資料送到 **write driver**，由它設定 bit line 的電壓。
2. 用 **column demux**：要寫的 32 bit 送到 512 個裡正確的那 32 行。

---

## 6. 用 pass transistor 做 mux

[13:18] 一般的 mux／demux 用邏輯閘；SRAM 陣列裡**不用**，改用 **pass transistor**：

```
輸入 00 ─┤開關├─┐
輸入 01 ─┤開關├─┤
輸入 10 ─┤開關├─┼─ 輸出
輸入 11 ─┤開關├─┘
（控制電路只把其中一個開關的 gate 設成 1）
```

- 每個輸入接一個開關（像 word line transistor），一個控制電路只打開其中一個。
- 【口頭】「電晶體有類比的性質，但做數位邏輯，只要知道它是一個有開、關兩種狀態的開關。」
- [14:47] 快、又省面積。電流反過來流就是 demux，控制邏輯一樣。

---

## 7. 把 cache 放上 SRAM 陣列

[15:41] 一個 cache = **tag array** ＋ **data array**：

- 兩個可以**共用一個 decoder**，沒有理由各用一個。
- Decoder 打開對應的 word line → 值流進 column mux、sense amplifier。
- [16:24] **Set associative** 的話：一個 set 的所有 tag 放在 tag array 的**同一列**，所有 block 放在 data array 的同一列 → **一次讀出來**，比完 tag 再用 mux 選資料。

[17:13] 這就是把 [第 21 支 §9](lecture21_note.md#9-set-associative-cache) 的觀念接到 cache 的實體配置上：同一個 set 的 block 放在陣列的同一列，是我們想要的性質。

---

## 8. 為什麼要 precharge

[17:13] 簡化：一個 cell，兩側各一條 bit line。cell 存 1 和 0：

- 接上之後，一條往 1 V（邏輯 1 ≈ 1 V）、一條往 0 V。
- 等到真的變成 1 V 和 0 V，sense amplifier 就知道了。

[18:14] 問題：**很慢**。為了在晶片上塞進最多記憶體，cell 的電晶體越小越好；**越小越弱**，能推的電流越少，要把 bit line 充到 1 V 要很久。「我們是效能狂。」

[19:50] **天平**的比喻：

- 想知道哪邊比較重，要等桿子撞到地上嗎？那要很久。
- 聰明的做法：看桿子傾斜的**方向**。一開始風會讓它晃一點（要忽略），但傾斜超過正常的晃動，就可以確定哪邊重。
- [21:26] 【口頭】老師小時候，菜販沒有電子秤：一邊放馬鈴薯、一邊放 1 kg、半 kg 的砝碼，**從不等**一邊撞到地，看到傾斜就趕快換砝碼，直到差不多平衡。

[22:41] **Precharge**：

1. 用很「胖」的 **precharge driver**，很快把 BL 和 BL̅ 都設成 **0.5 V**（不是靠弱的 cell 推，所以很快）。
2. 接上 SRAM cell。
3. 看兩條的**電壓差**：超過門檻 **δ**（雜訊容限）就判斷。例：BL 到 550 mV、BL̅ 到 450 mV，差 100 mV → 邏輯 1；反過來 → 邏輯 0。

[23:11] 門檻 δ 為什麼不能是 0：bit line 是長長的銅線，會撿到電磁干擾（例如開微波爐，變化的磁場感應出電位差）；相鄰平行的 bit line 有電流時也會感應出電流，叫 **crosstalk noise**。

[25:37] 不用等到 1000 mV → 「可以快 5 到 10 倍，不是小改進」。

【課本補充】課本 p.283：兩條 bit line 靠得很近，受到的雜訊差不多，所以看**差值**時，電磁和 crosstalk 雜訊大多會**互相抵消**。δ 越小電路越快，受雜訊限制。

（本機用 RC 模型算了快幾倍，見 [§13.3](#133-precharge-快幾倍)。）

---

## 9. Sense amplifier

[26:35] 細節超出範圍，老師只講大意。電路裡有三個 **differential amplifier**（差動放大器），輸入是 V_BL 和 V_BL̅。

[27:28] 假設 V_BL̅ > V_BL：

1. T3 變得更導通 → 多流 I_d 的電流。
2. T1、T2 是 **current mirror**（電流鏡）：一邊流多少，另一邊也流多少 → 另一邊也多 I_d。
3. 下面那顆電晶體在 **saturation**，總電流固定 → 兩個分支加起來不變，另一個分量要少 I_d。
4. [28:55] 唯一的可能：**2 I_d** 流進 T4 的 gate → 那裡的電壓上升（暫態現象）。
5. [29:53] 一級一級傳下去，最後經過一個反相器，**輸出變 0**。這正是我們要的：V_BL̅ > V_BL 表示 cell 存 0。

反過來 V_BL > V_BL̅ → 輸出 1。

[30:30] 調整電路參數（電晶體的尺寸、導線的電阻等），讓電壓差稍微超過雜訊門檻時，輸出就變成邏輯 0 或 1。

[31:21] Sense amplifier 很複雜，但**數量不多**（每一行一個，不是每個 cell 一個），所以負擔得起。

[31:52] 「**最最最重要**」的觀念：**有 precharge，就要 sense amplifier 放大差異。** DRAM 也一樣（[第 41 支](lecture41_note.md)），雖然 sense amplifier 的設計不同，但基本觀念相同：precharge，然後看偏離 precharge 電壓多少。

---

## 10. 10T CAM cell

[32:45] **CAM**（content addressable memory）的核心 = SRAM cell 再多一點：

- match line 上面是一般的 **6T SRAM cell**（「現在我們都是專家了」）。
- 多一個**輸入 bit A**：想檢查存著的 **Q** 等不等於 A。

[34:03] 比較的傳統做法是 XOR gate，但電晶體多、比很多值時效率差。改用另一種接法：

```
match line ──┬───────────┬──
             │           │
           [Q]         [Q̅]      ← NMOS，gate 接 Q、Q̅
             │           │
           [A̅]         [A]      ← NMOS，gate 接 A̅、A
             │           │
            地           地
```

- 輸入 A 和 A̅ 都送進來。
- [35:18] 問題：什麼時候 match line 到地之間有**導通的路徑**（match line 被拉到 0 V）？
- 每一串有兩顆**串聯**：兩顆都導通才通。

[36:34] 真值表（[§13.1](#131-cam-cell-的真值表) 實際算過）：

| Q | A | Q·A̅ 那串 | Q̅·A 那串 | match line |
|---|---|---|---|---|
| 0 | 0 | 斷 | 斷 | 保持 1 |
| 0 | 1 | 斷 | 通 | **拉到 0** |
| 1 | 0 | 通 | 斷 | **拉到 0** |
| 1 | 1 | 斷 | 斷 | 保持 1 |

[37:23] 結論：**只有 Q ≠ A 時**才有導通路徑。match line 先 **precharge 成 1 V**：相等 → 保持 1；不相等 → 很快放電到 0。

[39:02] 這就是 **10 顆電晶體的 CAM cell**：6T SRAM ＋ 4 顆（T1–T4）檢查 Q 和 A 一不一樣。

---

## 11. CAM 陣列

[39:57] 跟 SRAM 陣列一樣，只是把 SRAM cell 換成 CAM cell，另外每一行除了 BL、BL̅，還要送進要比較的 bit（A1…An）。

- 所有 match line **precharge 成 1**。
- 一列裡**只要一個** cell 不相等，那一列的 match line 就放電到 0 → 沒 match。

[40:44] Decoder 有兩種模式：

| 模式 | 做什麼 |
|---|---|
| **SRAM 模式** | 寫入、讀出資料（不用 CAM 功能）；A 線停用、不看 match line；照樣用 column mux／demux、write driver、sense amplifier |
| **CAM 模式** | ⚠️ 老師先說「打開所有 word line」，馬上改口：**關掉所有 word line**（關掉 SRAM 功能）；precharge 所有 match line，讓它們比較 |

[41:29] 比完：

- 沒有重複的話，最多一條 match line 是 1。所有 match line 接一個 **OR gate** → 有沒有 match（hit）。
- [42:25] 可能有**重複**：電路照樣能用，但要用 **priority encoder**（不是一般的 encoder）：多條是 1 時，選**優先權最高**的那條（例如最早的那一列，或依其他狀態決定），輸出它的 **row index**。

[43:16] 用途：**fully associative cache** 的 tag array 用 CAM，比到的 index 拿去讀用 SRAM 做的 data array（64 byte 的 block，比 tag 大很多）。

---

## 12. SRAM 和 CAM 各用在哪

[44:06]

| | SRAM | CAM |
|---|---|---|
| 每個 cell | 6 顆 | 10 顆，還要多的線 |
| 速度、面積、功耗 | **快很多**、小 | 慢、大、沒效率 |
| 用在 | 幾乎所有地方的 **data array** | 偶爾用在 **tag array**，特別是格數少、需要像 hash table 那樣 fully associative 的結構 |

[45:45] 下一講：**CACTI** 工具，自動探索 cache 的設計空間。

【課本補充】課本 p.278：書裡主要用 6T SRAM cell 和 10T CAM cell，也有其他種記憶體 cell。課本 p.287 說明了 CAM 陣列從 SRAM 陣列繼承的部分：precharge 電路、write driver、output driver。

---

## 13. 【實測】

本機沒有 SPICE 之類的電路模擬器，所以只用 Python 做開關層級的邏輯和 RC 的計算。程式是 `experiments/lecture23/sram.py`。

### 13.1 CAM cell 的真值表

照 [§10](#10-10t-cam-cell) 的接法（兩串各兩顆串聯的 NMOS，NMOS 的 gate 是 1 就導通）：

```
1. CAM cell（match line 先 precharge 成 1）
   Q  A | Q·A̅ 那串  Q̅·A 那串 | match line
   0  0 |    斷        斷     |   1（Q = A，保持 1）
   0  1 |    斷        通     |   0（Q ≠ A，被拉到地）
   1  0 |    通        斷     |   0（Q ≠ A，被拉到地）
   1  1 |    斷        斷     |   1（Q = A，保持 1）
```

跟影片 [36:34] 的表一樣：只有 Q ≠ A 時 match line 被拉到地。

### 13.2 CAM 陣列與 priority encoder

8 列、每列 4 bit，找 key = `1011`：

```
2. 8 列 × 4 bit 的 CAM 陣列，找 key = 1011
   match lines = 01001000 → OR = 1 ；priority encoder（選最小的 index）→ 1 （第 4 列也符合，重複時要 priority encoder）
```

第 1 列和第 4 列都存著 `1011`，兩條 match line 都是 1；OR 說「有 hit」，priority encoder（這裡選最小的 index）給出 1。一般的 encoder 遇到兩條 1 會輸出錯的值，這就是 [42:25] 要 priority encoder 的原因。

### 13.3 Precharge 快幾倍

把 bit line 想成一個電容 C，被很弱的 cell（電阻 R）充放電，電壓照 1 − e^(−t/RC) 變化，時間以 RC 為單位：

```
3. Bit line：cell 很弱，bit line 的電容很大 → 用 RC 模型，時間以 RC 為單位
   沒有 precharge，等 bit line 到 0.50 V：0.693 RC
   沒有 precharge，等 bit line 到 0.90 V：2.303 RC
   沒有 precharge，等 bit line 到 0.95 V：2.996 RC
   precharge 到 0.5 V，等兩條差 50 mV：0.051 RC
   precharge 到 0.5 V，等兩條差 100 mV：0.105 RC
   precharge 到 0.5 V，等兩條差 200 mV：0.223 RC
   快幾倍（以差 100 mV 為準）：對 0.5 V 是 6.6 倍，對 0.9 V 是 21.9 倍
```

- Precharge 到 0.5 V 後，BL 往 1 V、BL̅ 往 0 V 同時走，兩條的**差**長得比一條自己走快；只要差到 100 mV（[§8](#8-為什麼要-precharge) 的例子）就能判斷：**0.105 RC**。
- 沒有 precharge，如果只等 bit line 過一半（0.5 V）就判斷，要 0.693 RC → precharge **快 6.6 倍**；如果像 [18:14] 說的等到接近 1 V（0.9 V），要 2.3 RC → **快 22 倍**。
- 影片 [25:37] 說「快 5 到 10 倍」，落在這兩個數字之間，看「沒有 precharge 時要等到多少才算」。
- 門檻 δ 越小越快（50 mV 只要 0.051 RC），但 δ 不能小於雜訊，這就是 [23:11] 說的限制。
- 這是很粗的模型：真的 cell 不是固定電阻、sense amplifier 本身也要時間。

---

## 14. 一頁速查表

| 主題 | 重點 |
|---|---|
| 設計 cache | 面積、時間、功耗、效能的取捨 |
| 背景 | SR latch（交叉耦合的 NAND）、JK flip-flop；latch／flip-flop 常混用 |
| 存一個 bit | 交叉耦合的兩個反相器（4 顆），跟 clock 無關 |
| 6T SRAM | ＋ 2 顆 access transistor；word line = 1 → 接到 BL、BL̅ |
| 陣列 | row decoder（n bit → 2ⁿ 列）、word line（列）、bit line 對（行）、column mux／demux、sense amp／write driver |
| 讀 | 512 個 cell 選 32 個 → sense amplifier |
| 寫 | write driver 設 bit line → demux 到對的行 |
| Mux | pass transistor：快、省面積 |
| Cache 配置 | tag array 和 data array 共用 decoder；一個 set 放在同一列 |
| Precharge | 胖的 driver 把 BL、BL̅ 設 0.5 V → 接 cell → 差 > δ（雜訊）就判斷；快 5–10 倍 |
| Sense amplifier | 差動放大器、current mirror；數量少所以可以很複雜；DRAM 也要 |
| 10T CAM | 6T ＋ 兩串 NMOS（Q·A̅、Q̅·A）；match line precharge 1，Q ≠ A 才拉到地 |
| CAM 陣列 | SRAM 模式／CAM 模式（關 word line）；OR → hit；priority encoder → index |
| 用途 | SRAM：data array；CAM：小的 fully associative tag array |
| 實測（模擬） | CAM 真值表符合影片；重複時要 priority encoder；RC 模型下 precharge 快 6.6–22 倍 |

---

## 15. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Cache 的 tag／data array、set associative、fully associative | [第 21 支](lecture21_note.md) |
| CAM-based RAT | [第 13 支](lecture13_note.md) |
| LSQ 用 CAM 找相同位址 | [第 12 支](lecture12_note.md) |
| CMOS、latch 的開關模型 | [Basic 第 18 支](../basic/lecture18_note.md)、[Basic 第 19 支](../basic/lecture19_note.md) |
| CACTI、Elmore delay：把陣列的延遲算出來 | [第 24 支](lecture24_note.md) |
| DRAM 的 sense amplifier 和 precharge | [第 41 支](lecture41_note.md)（Ch10） |
| 雜訊、可靠度 | [第 49 支](lecture49_note.md)（Ch12） |

---

## 附：本講一句話總結

> **晶片上的記憶體有兩種基本技術：SRAM cell 是交叉耦合的兩個反相器（4 顆電晶體，靠互相回饋存一個 bit）加上兩顆由 word line 控制的 access transistor，排成陣列後由 row decoder 選一列、每一行一對互補的 bit line，經過用 pass transistor 做的 column mux／demux 接到 sense amplifier（讀）或 write driver（寫），cache 的 tag array 和 data array 共用 decoder、一個 set 放在同一列；因為 cell 又小又弱、bit line 又長，讀的時候先用強的 driver 把兩條 bit line precharge 到 0.5 V，再看兩者的差超過雜訊門檻就判斷，由差動放大器和電流鏡組成的 sense amplifier 放大這個差異，比等 bit line 充到底快 5–10 倍；CAM cell 則是 6T SRAM 再加兩串 NMOS（Q·A̅、Q̅·A），match line precharge 成 1，只有存的值和輸入不相等時才被拉到地，一列有任何一個不相等就沒有 match，所有 match line 經過 OR 得到 hit、經過 priority encoder 得到 index，但 CAM 比 SRAM 大、慢、耗電，只用在小的 fully associative tag array。本機模擬：CAM cell 的真值表和影片一致，重複的 key 需要 priority encoder，而用 RC 模型算，precharge 加 100 mV 門檻比沒有 precharge 快 6.6 到 22 倍，看沒有 precharge 時要等到多少電壓。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 7: Caches (Part III) -- SRAM and CAM arrays 的 whisper 轉錄稿（本機 `large-v3-turbo`，46:05）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch7 §7.3.1（p.277–289）整理。實測為本機（Python 3.10）模擬結果。*
