# Advanced Computer Architecture 第五支影片完整筆記：效能模型：performance equation、頻率的極限、multi-issue 與 OOO 的動機（Chapter 2, Part 2/3）

> **課程**：Advanced Computer Architecture — Chapter 2: Out-of-order Pipelines: Part II/III (Performance Models)（播放清單第 5 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（`large-v3-turbo`，本機 GPU；YouTube 自動字幕辨識成錯的語言，不能用）（1:08:07）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch2** §2.2–2.3.3（p.43–53）
> **章號**：影片 Chapter 2 = 課本 Ch2。
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
- [1. A 比 B 快？要看是哪個程式](#1-a-比-b-快要看是哪個程式)
- [2. 一組程式：比值的幾何平均](#2-一組程式比值的幾何平均)
- [3. Performance equation](#3-performance-equation)
- [4. 指令數](#4-指令數)
- [5. 頻率與 pipeline 的級數](#5-頻率與-pipeline-的級數)
- [6. IPC](#6-ipc)
- [7. 頻率為什麼不能一直加](#7-頻率為什麼不能一直加)
- [8. CPI = CPI_ideal + stall rate × stall penalty](#8-cpi--cpi_ideal--stall-rate--stall-penalty)
- [9. Multi-issue in-order（superscalar）](#9-multi-issue-in-ordersuperscalar)
- [10. Out-of-order：照資料相依的順序](#10-out-of-order照資料相依的順序)
- [11. Instruction window](#11-instruction-window)
- [12. 要填滿 window：branch 一定要猜得很準](#12-要填滿-windowbranch-一定要猜得很準)
- [13. 【實測】](#13-實測)
- [14. 一頁速查表](#14-一頁速查表)
- [15. 與後續講次的連結](#15-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

一條推理鏈：

```
效能 ∝ IPC × f / #insts
  #insts：compiler 已經很成熟，改善有限
  f：功耗 ∝ f³、latch delay、stall penalty 變大 → 停在 3–3.5 GHz
  → 只能拉高 IPC
     multi-issue in-order：相依太多、O(n²) 的檢查和 forwarding → 頂多 IPC ≈ 2
     → out-of-order：照資料相依的順序執行，不照程式順序
        → 需要大的 instruction window
           → 需要非常準的 branch predictor（98–99% 以上）
```

最後一步留給第 6 支：OOO 裡還有 WAW、WAR 這些相依。

---

## 1. A 比 B 快？要看是哪個程式

[00:41] 「電腦 A 比電腦 B 快嗎？」很多人答錯：

- 「頻率比較高」：錯。
- 「記憶體比較多」：錯。
- 兩個加起來：還是錯。

[01:25] 正確答案：**這句話沒有意義**。效能一定是**針對某個程式**說的。

[02:09] 做法：同一個程式在 A 和 B 上各跑一次，用 **wall clock**（真實時間的碼表）量。哪邊快，就說「對這個程式而言」那邊快。例如下棋程式在 A 上比較快，就說「下棋的話 A 比 B 快」。

[02:55] 想只比 CPU、不比記憶體和硬碟：程式夠小、大部分時間只用 CPU 自己的資源就行。之後的章節會講怎麼確保這件事。

[03:26] 可以說「超級電腦比手機快」嗎？可以，因為**不管給什麼程式**，超級電腦都會比較快，差距很明顯。但兩台筆電，一台 2 GHz、一台 3 GHz，就說不準了。

（本機實測：同一台機器的 P-core 和 E-core，快多少要看程式，見 [§13.1](#131-快多少要看程式)。）

---

## 2. 一組程式：比值的幾何平均

[05:03] 也可以用一組程式（**suite**）。例如 5 個程式，量出 t₁ᴬ、t₂ᴬ…（程式 1 在 A 上的時間）。

[05:31] 怎麼合起來？

- **算術平均**：不好。一個程式跑 1 秒、另一個跑 1 小時，結果會被長的那個主導。
- **比值的幾何平均**：先算每個程式的時間比 tᵢᴮ / tᵢᴬ，再取幾何平均。計算機結構裡，**只要是比值就取幾何平均**。

[05:56] 結果是一句話：「對這組程式，A 比 B 快 10%（或 15%、20%）。」

【課本補充】課本 p.43：SPEC benchmark 就是這樣用比值來評估程式和 suite 的效能。

（本機示範見 [§13.2](#132-算術平均-vs-幾何平均)。）

---

## 3. Performance equation

[06:41] 「計算機結構裡最有名的式子之一」。效能和時間成反比，做一下單位分析：

```
           1         #insts     #cycles       1
P   =   ──────  =  ───────── × ───────── × ───────
         #secs      #cycles      #secs      #insts
                   └── IPC ──┘ └── f ──┘

           IPC × f
P   ∝   ──────────
           #insts
```

| 符號 | 意思 |
|---|---|
| **#insts** | 程式的**動態**指令數（迴圈 5 條跑 100 次 = 500 條，不是 5 條） |
| **IPC** | 每個 cycle 做幾條指令；是設計（架構）的特性 |
| **f** | 每秒幾個 cycle，也就是時脈頻率 |

[09:14] **IPC 和 CPI 是倒數**：IPC = instructions per cycle，CPI = cycles per instruction。【口頭】很多學生會搞混：IPC 的 I 在前面，I 是 instruction。

[10:02] 效能通常是相對的，比例常數會消掉，有時寫等號、有時寫 ∝ 都沒差。重要的是三個因素之間的互動。

【課本補充】課本 Equation 2.2（p.43）就是這條推導；Basic 第 32 支 §11 也有。

---

## 4. 指令數

[10:29] 指令數越少，效能越高。它取決於：

- **程式設計師**：邏輯本身要多少指令。
- **Compiler**：常常會產生不夠好的程式碼，例如什麼都沒做的 **dead code**；聰明的 compiler 會把它們拿掉。

[17:27] 但 compiler 技術已經很成熟了，這方面不太會再進步。

---

## 5. 頻率與 pipeline 的級數

[11:51] 頻率取決於：

1. **電晶體技術**：電晶體越快，頻率能開越高。
2. **架構**，也就是 **pipeline**。這是最有趣的部分。

[12:13] 設一條指令從頭到尾的工作（**algorithmic work**）要 t 的時間。分成 k 級、每級一樣長（**balanced**），每一級還要加上 pipeline latch 的延遲 δ：

```
最小 clock period  = t/k + δ

                        1
f_max          = ─────────────
                    t/k + δ
```

[14:34] 「非常、非常、非常、非常重要」：

- **k → ∞**：t/k → 0，clock period 只剩 latch delay δ。超過某個點，再加級數完全沒好處。
- **k 不大**（3、4、5、6、7 級）、**t/k ≫ δ**：f_max ≈ k/t，**頻率大約和級數成正比**。

[15:57] Pipeline 唯一的好處就是這個：級數多 → 每級短 → clock period 短 → 頻率高。從單週期處理器變成 5 級 pipeline，頻率大約可以變 5 倍。

[16:25] 注意用字：多級數**允許**我們提高頻率。實際要不要開那麼高，看其他因素。

【課本補充】課本 p.44 特別破除一個迷思：「pipeline 因為同時處理很多條指令，所以比較快」**是錯的**。單週期處理器 IPC = 1，pipeline 的 IPC 最多 1、通常更低（有 stall）。**頻率一樣的話，pipeline 沒有任何好處**，甚至比較慢。好處完全來自「每一級比較短，所以時脈可以開比較快」。

---

## 6. IPC

[17:50] IPC 取決於架構和 compiler：

- **架構**：例如 forwarding，減少 stall，每個 cycle 做的有用工作變多，IPC 上升。
- **Compiler**：重排程式碼，讓 data hazard、control hazard 少浪費幾個 cycle。

[19:51] 例子：load-use hazard。`load` → 停一個 cycle → `use`。聰明的 compiler 在中間塞一條**跟 load 無關**的指令，就不用停。⚠️ 老師一開始說「就算跟 load 有關也行，可以 forward」，馬上收回：只有 store 才能這樣，所以要是無關的指令。

[21:44] 理想的 5 級 in-order pipeline 沒有 stall，出口每個 cycle 出來一條，IPC = 1。實際上：

```
出口看到：指令、stall、stall、指令  → 4 個 cycle 出來 2 條 → IPC = 0.5
```

所以 **pipeline 處理器的 IPC 永遠小於 1**。

[23:33] 拉高 IPC 的方法：

1. Forwarding。
2. 程式裡多一點 **not taken** 的 branch（控制 hazard 比較少）。
3. **更快的指令記憶體和資料記憶體**：記憶體要好幾個 cycle 才回來的話，那些 cycle 都算 stall。

| 因素 | 取決於 |
|---|---|
| #insts | compiler、程式設計師 |
| f | 電晶體技術、pipeline 級數 |
| IPC | stall 的比率、compiler 怎麼重排、forwarding 這類技巧 |

---

## 7. 頻率為什麼不能一直加

[25:04] 能不能開到 100 GHz？不能。原因（第 11 章會細講）：

| 原因 | 說明 |
|---|---|
| **功耗** | 功率 ∝ f³。頻率加倍，功率 8 倍 |
| **溫度** | 溫度上升 ΔT 大約和功率成正比 |
| **設計複雜度** | 很多級 → 很多 hazard → forwarding 邏輯、多工器超複雜 |
| **Latch delay** | 級數很多時，每級的時間幾乎都是 latch delay |
| **Stall penalty** | 級數越多，每次 stall 要等越久（§8） |

[25:56] 【口頭】數字：筆電大約 20–50 W，頂多 100 W；桌機大約 250 W。乘 8 倍是 2 kW，差不多一台冷氣。玩遊戲時晶片溫度可以到 **100°C**，再高整顆晶片就熔了。

[26:50] 功耗是最大的限制。所以到 2020 年為止，過去 10–15 年頻率沒有再增加。

[27:16] 就算泡液態氮、找世界上最好的設計師做 forwarding 電路：級數少時，每級的邏輯很多，latch delay 只佔一小部分；級數多時兩者差不多；級數非常多時，幾乎全是 latch delay。

⚠️ **頻率上限**：課本 p.45 說晶片頻率不會超過「目前的 3–3.5 GHz」，從 **2005** 年起大致不變。本機 P-core 標示最高 **4.7 GHz**（見 Basic 第 47 支 [§17.2](../basic/lecture47_note.md#172-一個-thread-vs-全部-thread)），但只在少數 core、短時間跑；20 個 thread 一起跑時只剩 3.0 GHz。

---

## 8. CPI = CPI_ideal + stall rate × stall penalty

[28:01] 計算時常用 CPI（IPC 比較是測量、做高層決策用的）：

```
CPI = CPI_ideal + stall rate × stall penalty
                  └ 每條指令幾次 stall ┘ └ 每次 stall 幾個 cycle ┘
```

- **CPI_ideal**：沒有 stall 時的 CPI，不考慮其他損失的話是 1。
- 單位：（stall／指令）×（cycle／stall）= cycle／指令。

[29:48] 級數增加時：

- **stall rate**：大致不變（直覺上和實驗上都是），級數很多時稍微增加。
- **stall penalty**：**會變大**。pipeline 很長時，一條指令可能要等 2、3 個 cycle 才拿到資料。

[31:53] 所以 CPI 上升、**IPC 下降**。在我們的 in-order pipeline 裡，級數越多，IPC 從 1 一路往下掉（老師強調這不是一般性的結論）。

[33:37] 結論：級數增加帶來的頻率好處，會被 IPC 下降和複雜度抵消掉。**頻率加不上去，只能拉高 IPC。**

【課本補充】Basic 第 32 支 §15–16 用同一條式子推出最佳級數 k_opt = √(t_max / (l·r·c))。

---

## 9. Multi-issue in-order（superscalar）

[34:17] 最簡單的想法：**一次發好幾條**。

- **Issue**：把指令送到執行單元。
- **Issue width**：每個 cycle 發幾條。現在是 1。
- 一次 fetch、decode、issue 2、4 或 8 條：**superscalar** 處理器，利用指令層級的平行度（ILP）。

[35:12] 老師說最早的 Intel Pentium（1990 年代初，很革命性的晶片）就是這樣：兩條平行的 in-order pipeline。⚠️ 字幕把兩條 pipe 的名字聽成「U-pipe 和 B-pipe」，第二個名字待查。

[36:06] 投影片畫了 3 條 pipeline：沒有相依的話 IPC 最高 3。問題很多：

- [36:28] **相依一定存在**。n-issue 的話，任兩條之間都可能相依 → **O(n²)** 的相依檢查和 forwarding path。
- [36:54] 要達到峰值 IPC = n 不能有 stall；但 hazard 和 taken branch 都會造成 stall，wrong path 的指令全部要作廢。
- [37:12] 一個 bundle 裡的指令之間也要 **internal forwarding**，路徑非常多。
- [38:02] **Load／store 最麻煩**：暫存器有名字，decode 時就看得出相依；記憶體位址要到 EX 才算出來，新的相依會「突然冒出來」，同一個 bundle 裡也可能撞到同一個位址。
- [38:49] 要找到很長、幾乎沒有相依的指令序列不實際。所以 Pentium 只做兩條，三、四條很難。

[39:17] 結論：multi-issue in-order 最好也只能 IPC ≈ 2，通常更低。

【課本補充】課本 §2.2.2（p.45–47）的補充：

- 引用 **Murphy's law**：「會出錯的一定會出錯。」設計者要考慮最壞的情況。
- 多加電路的代價：面積、功耗、延遲。原因是 **placement**（元件擺放）和 **routing**（佈線）變差：關鍵路徑變長、線變長，時脈可能被拖慢。
- 靠 compiler 排好 bundle 不好：程式會綁死在某一種處理器上（為 2-issue 排的程式在 3-issue 上可能很差）。而且大部分程式碼都用到上一行的結果。除了 Pentium，大多數商用處理器沒採用這個做法。

---

## 10. Out-of-order：照資料相依的順序

[39:30] 換個想法：**不照程式順序**處理指令，這就是 **out-of-order pipeline**。

[40:42] 回顧：單週期處理器是一個超長的 cycle；pipeline 是很多短 cycle。Pipeline 的好處**不是來自 IPC**（IPC 反而下降），而是 f_max ∝ k。但 multi-issue 時找不到夠多獨立的指令，因為我們被**程式順序**綁住：就算後面很遠的地方有獨立的指令，也不能先拿出來做。

[42:04] 看相依的結構：前三條一串 RAW、後三條另一串，兩串之間互不相干。

[42:47] 用 **2-issue OOO** 處理器（老師說投影片用「OOO」三個 O 表示）：每個 cycle 從兩串各拿一條；串內需要的值用 forwarding 拿到，不用停。**3 個 cycle** 做完，原本（有 forwarding）要 6 個 cycle，快 2 倍。（就是第 3 支的課本 Figure 1.3。）

[44:41] 執行順序不是**程式順序**，而是**資料相依順序**（data dependence order）：唯一的條件是 consumer 一定在 producer 之後。

【課本補充】課本 §2.3.1（p.47–48）的定義：

- **ILP**（instruction level parallelism）：能同時執行的指令數的量度；兩條指令要同時做，彼此不能有相依。
- **Superscalar 處理器**：同時 fetch、執行多條指令。分成 in-order 和 out-of-order 兩種。
- **Out-of-order 處理器**：執行順序可以和程式順序不同，但**絕不違反資料相依**。
- 為什麼不靠 compiler：compiler 只能分析它看得懂的程式碼，很多行為（例如記憶體位址）要到執行時才知道；而且程式會綁死在某種處理器上。

---

## 11. Instruction window

[45:51] 基本原則：

1. **永遠照程式順序 fetch**。
2. 抓的寬度要配合執行的寬度：issue width 是 4，fetch 也要 4。
3. 抓進來的指令放進一個大的**指令池**（硬體叫 **instruction window**，老師兩個詞混用）。
4. 每個 cycle 從池裡挑：**彼此獨立**、而且**運算元都好了（ready）**的指令。
5. 不照順序執行它們。

[47:26] 這能帶來 2–3 倍的效能。

[47:50] **ILP**：一個 cycle 內能同時執行的「ready 且獨立」的指令數。有些程式 ILP 高、有些低，compiler 也可以想辦法露出更多 ILP。

[49:06] 最重要的是**快速建一個大池子**：池子越大，能找到越多可以平行的指令。

[50:25] Instruction window 就是一個簡單的指令 **queue**：**照程式順序放進去**，**照資料相依順序拿出來**，送到各個功能單元（加法器、乘法器、除法器……）。

⚠️ **Window 多大**：

| 來源 | 大小 |
|---|---|
| 影片 [50:49] | 64–128 條；[65:07] 說實際上限大約 **150** 條 |
| 課本 p.52 | 64–256 個 entry |
| 課本 Summary 1（p.59） | 50–100 條改名後的指令 |
| 本機實測（第 3 支 [§10.2](lecture03_note.md#102-本機的-out-of-order-視窗有多大)） | P-core 一個 cache miss 後面能夾約 **500** 條、E-core 約 250 條 |

---

## 12. 要填滿 window：branch 一定要猜得很準

[51:13] 問題：程式裡有 branch。window 裡的指令必須**全部在正確的路徑上**。任何一個 branch 猜錯，它後面的指令全部是 wrong path，整個 window 就廢了。

[52:19] 例子：一個有 `i < m`、`j < i`、`j % 2 == 0` 的巢狀迴圈，攤開來有很多 branch，每一個都要猜對。

[53:11] 能不能先把 branch 算出來？假設每 5 條指令有 1 個 branch，100 條的 window 就有 20 個 branch。每個 branch 都依賴前面好幾條指令的結果，那些也要先算……等於把整個程式跑一遍。**不可行**。

[54:37] 只剩一個辦法：**預測**。每個 branch 有兩件事要猜：

- **方向**（outcome）：taken 還是 not taken。
- **目標**（target）：跳到哪裡，這也要算，來不及就得猜。

[58:13] 數學（課本 Equation 2.3，p.52–53）：n 條指令有 n/5 個 branch，每個猜錯的機率 p，彼此獨立：

```
全部猜對的機率   = (1 − p)^(n/5)
至少錯一個的機率 Pn = 1 − (1 − p)^(n/5)
```

[60:41] n = 100，看 Pn 對 p 的曲線：

| 要求 | 每個 branch 的猜錯率 p | 準確率 |
|---|---|---|
| 90% 的時間 window 全對（Pn = 10%） | 約 0.5%（老師說「也許 0.55%」） | 約 99.5% |
| 80% 的時間全對（Pn = 20%） | 約 1.1% | 約 98.9% |
| 如果 p = 2.5%（97.5% 準） | → Pn ≈ 40% | 只有 60% 的時間 window 全對 |

[63:55] 實際上猜錯時不會丟掉整個 window，會保留猜錯的 branch 之前的指令。但代價還是很大。

[64:28] 結論：要撐住 100 條的 window，每個 branch 的準確率要 **98–99%**。這也**限制了 window 的大小**：window 是 1000 條的話，Pn 會非常接近 100%，不可能做出那麼準的 predictor。

[65:45] 【口頭】資訊理論裡的 **Fano's inequality** 說，預測一個序列有理論上限，取決於序列的資訊量。所以 branch predictor 的準確率有硬上限，window 也有上限。

[67:31] 問題都解決了嗎？還沒。OOO 裡除了 RAW 還有別種相依，下一講講。

【課本補充】課本 p.53 說每個 branch 的準確率要「**99% 以上**」。⚠️ 課本內文的 17%、7% 比公式略低，見第 6 支 [§5](lecture06_note.md#5-control-dependence)。

---

## 13. 【實測】

程式在 `experiments/lecture05/`。

### 13.1 快多少，要看程式

`perprog.c` 有兩個程式：

- `alu`：4 個整數運算接成一條相依鏈，跑 4 億圈。只看時脈（每圈固定 4 個 cycle）。
- `chase`：512 MB 裡的 pointer chase，跳 2000 萬次，每一步都 cache miss。只看記憶體。

分別綁在 P-core（cpu2）和 E-core（cpu14）上：

```
cpu2                    cpu14
alu   0.344 s           alu   0.446 s
chase 2.001 s           chase 2.146 s
```

| 程式 | P-core 比 E-core 快幾倍 |
|---|---|
| alu | **1.30** 倍 |
| chase | **1.07** 倍 |

- 同一顆晶片、同兩個 core，答案卻差很多。這就是影片 [01:25]：「A 比 B 快」要說是**哪個程式**。
- `alu` 的 1.30 倍剛好等於兩種 core 的最高頻率比（4.7 / 3.6 = 1.31）：程式只受 f 限制時，效能 ∝ f。
- `chase` 幾乎都在等 DRAM。DRAM 對兩種 core 一樣慢，頻率高也沒用。頻率不是效能，效能公式裡 IPC 也會變（這裡 IPC 被記憶體壓得很低）。

### 13.2 算術平均 vs 幾何平均

`means.py` 用三台假想的機器（A、B、C）跑三個程式（時間是我假設的數字）：

```
算術平均時間： {'A': 337.0, 'B': 170.67, 'C': 501.67}
以 A 為基準，比值的幾何平均（越大越快）： {'A': 1.0, 'B': 1.0, 'C': 1.186}
以 B 為基準，比值的幾何平均（越大越快）： {'A': 1.0, 'B': 1.0, 'C': 1.186}
以 C 為基準，比值的幾何平均（越大越快）： {'A': 0.843, 'B': 0.843, 'C': 1.0}
```

- 算術平均說 **C 最慢**、B 快 A 兩倍。其實這只是因為第二個程式（1000 秒那個）蓋過了其他兩個。
- 比值的幾何平均說 **C 最快**（快 18.6%），A 和 B 一樣。
- 換哪台機器當基準，排名都一樣（1 / 0.843 = 1.186）。這就是影片 [05:31] 說要用比值的幾何平均的原因。

### 13.3 Pn 的數字

`means.py` 的後半照 Equation 2.3 算：

```
n=100，要 Pn ≤ 10%：每個 branch 猜錯率 p ≤ 0.525%（準確率 ≥ 99.47%）
n=100，要 Pn ≤ 20%：每個 branch 猜錯率 p ≤ 1.110%（準確率 ≥ 98.89%）
n=100，p=2.5%：Pn = 39.7%
p=0.5%，n= 100：Pn = 9.5%
p=0.5%，n= 150：Pn = 14.0%
p=0.5%，n= 500：Pn = 39.4%
p=0.5%，n=1000：Pn = 63.3%
```

- 影片從圖上讀的 0.5%（0.55%）、1.1%、約 40% 都對。
- Window 從 100 條變 1000 條，就算 99.5% 準，也有 63% 的機會裡面有猜錯的 branch。所以老師說 window 不能太大。
- ⚠️ 可是本機量到 P-core 的視窗約 500 條（第 3 支）。照這個模型，Pn 會是 39%，表示現代的 predictor 要比 99.5% 準很多，或者 branch 之間不是獨立的（模型假設獨立）。待第 8 支實測 predictor 時再看。

---

## 14. 一頁速查表

| 主題 | 重點 |
|---|---|
| 「A 比 B 快」 | 沒意義；要說哪個程式；同一個程式用 wall clock 量 |
| 一組程式 | 比值的幾何平均；不用算術平均 |
| 效能公式 | P ∝ IPC × f / #insts；#insts 是動態指令數 |
| IPC／CPI | 互為倒數；IPC 的 I 在前 |
| #insts | 程式設計師、compiler（去掉 dead code）；已經很成熟 |
| f_max | 1 / (t/k + δ)；t/k ≫ δ 時 ∝ k；k → ∞ 時只剩 δ |
| Pipeline 的好處 | 只來自頻率；IPC 反而 ≤ 1 |
| IPC 拉高 | forwarding、compiler 重排、not taken branch、快的記憶體 |
| 頻率的限制 | 功率 ∝ f³、ΔT ∝ 功率、latch delay、stall penalty、複雜度 |
| CPI | CPI_ideal + stall rate × stall penalty；級數多 → penalty 大 → IPC 降 |
| Superscalar | 一次 fetch／decode／issue 多條 |
| Multi-issue in-order | O(n²) 相依檢查、bundle 內 forwarding、記憶體相依 → IPC 頂多約 2；Pentium 兩條 pipe |
| OOO | 照資料相依順序；consumer 在 producer 之後就好 |
| Instruction window | 照程式順序放、照相依順序拿；ready 且獨立的才發 |
| Window 大小 | ⚠️ 影片 64–128（上限約 150）；課本 64–256；本機約 500 |
| Pn | 1 − (1 − p)^(n/5)；n = 100 要 98–99% 以上準 |
| Fano's inequality | 預測準確率有理論上限 → window 有上限 |
| 實測 | P-core 比 E-core：alu 快 1.30 倍、pointer chase 只快 1.07 倍；幾何平均的排名和基準無關 |

---

## 15. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| 為什麼要 OOO、Figure 1.3 | [第 3 支](lecture03_note.md) |
| 效能公式、最佳級數、CPI 的式子 | [Basic 第 32 支 §11](../basic/lecture32_note.md#11-效能公式) 起 |
| Forwarding、load-use hazard | [Basic 第 31 支](../basic/lecture31_note.md)、[Basic 第 32 支](../basic/lecture32_note.md) |
| 功率 ∝ f³、power wall | [Basic 第 47 支 §11](../basic/lecture47_note.md#11-現在power-wall-與-think-parallel) |
| WAW、WAR、renaming、precise exception | [第 6 支](lecture06_note.md) |
| Branch predictor 怎麼做 | [第 8 支](lecture08_note.md)（Ch3） |
| 功耗與溫度 | [第 46 支](lecture46_note.md)（Ch11） |

---

## 附：本講一句話總結

> **「A 比 B 快」只對某個程式（或一組程式的比值幾何平均）才有意義，而效能 ∝ IPC × f / 動態指令數：指令數靠 compiler，已經很難再降；頻率靠電晶體和 pipeline 級數（f_max = 1/(t/k + δ)，級數不多時大約和 k 成正比，這也是 pipeline 唯一的好處），但功率 ∝ f³、latch delay、越來越大的 stall penalty 和設計複雜度讓頻率停在 3–3.5 GHz；所以只能拉高 IPC，可是一次發很多條的 in-order superscalar 要 O(n²) 的相依檢查和 forwarding，頂多 IPC ≈ 2，於是改用 out-of-order：照程式順序放進 instruction window，照資料相依順序挑出 ready 且獨立的指令執行；window 要大才有 ILP，但 100 條的 window 有約 20 個 branch，Pn = 1 − (1 − p)^(n/5) 表示每個 branch 要 98–99% 以上準，這也限制了 window 的大小。本機實測：P-core 跑純運算比 E-core 快 1.30 倍（正好是頻率比），跑 pointer chase 卻只快 1.07 倍。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 2: Out-of-order Pipelines: Part II/III (Performance Models) 的 whisper 轉錄稿（本機 `large-v3-turbo`，1:08:07）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch2 §2.2–2.3.3（p.43–53）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
