# Advanced Computer Architecture 第十四支影片完整筆記：Aggressive speculation：位址、相依、延遲與值的預測（Chapter 5, Part I）

> **課程**：Advanced Computer Architecture — Chapter 5: Alternative Approaches to Issue and Commit - Part I (Aggressive Speculation)（播放清單第 14 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（56:03）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch5** §5.1（p.152–162）
> **章號**：影片 Chapter 5 = 課本 Ch5。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10、valgrind 3.18）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕把「load latency speculation」聽成「low latency speculation」（[03:23]），把「CHT」寫成「chd」，筆記照課本改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 這一章要做什麼](#1-這一章要做什麼)
- [2. 四種 aggressive speculation](#2-四種-aggressive-speculation)
- [3. 位址預測：猜上一次的位址](#3-位址預測猜上一次的位址)
- [4. 位址預測：stride](#4-位址預測stride)
- [5. Load-store 相依預測：為什麼猜得到](#5-load-store-相依預測為什麼猜得到)
- [6. Collision history table（CHT）](#6-collision-history-tablecht)
- [7. 加上距離](#7-加上距離)
- [8. Store sets](#8-store-sets)
- [9. Load 延遲預測](#9-load-延遲預測)
- [10. 值的預測](#10-值的預測)
- [11. Confidence table](#11-confidence-table)
- [12. 【實測】](#12-實測)
- [13. 一頁速查表](#13-一頁速查表)
- [14. 與後續講次的連結](#14-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
Ch4 的 OOO 太簡單 → Ch5：先讓它更複雜（aggressive speculation、replay）
                       → 再看一個比較簡單的設計 → 最後看 compiler 能做什麼
本講：對 load 做四種猜測
  位址：last address、stride
  load-store 相依：CHT（撞不撞）、CHT＋距離、store sets（跟哪個 store 撞）
  延遲：猜 L1 命中 → 早點 broadcast
  值：猜 load 讀到什麼（唯一能打破 RAW 的方法）
猜錯了怎麼辦 → 下一講（replay）
```

---

## 1. 這一章要做什麼

[00:15] Ch4 講的 issue、commit 「很簡單」，真的 OOO 處理器複雜得多。這一章：

1. 先把設計**變複雜**：看一些角落的情況（aggressive speculation、replay）。
2. 再看一個**比較簡單**、也比較沒效率的設計。
3. 最後看 **compiler** 和 profiler 怎麼幫忙產生有效率的程式碼。

[01:31] 需要的背景：OOO pipeline、wakeup／select、precise exception 和 commit（[第 10](lecture10_note.md)–[13 支](lecture13_note.md)）。

[02:24] 【口頭】「我們常開玩笑說，硬體只有一堆暫存器和一堆線，就這樣。」就像只用三、四支蠟筆畫出一幅複雜的畫。

[02:53] 之前看過的一種推測：**branch prediction**。猜錯了，等分支到 ROB 頭再 flush。這一章看別種推測。

---

## 2. 四種 aggressive speculation

[05:59]

| 種類 | 猜什麼 | 好處 |
|---|---|---|
| **位址推測** | load 的位址 | 早點把 load 送去記憶體，相依的指令早點醒 |
| **Load-store 相依推測** | load 和 store 之間有沒有相依 | load 可以早送；可以跳過 unresolved 的 store 直接轉送 |
| **延遲推測** | load 會不會在 L1（或 LSQ）命中 | 知道幾個 cycle 拿到值，早點 broadcast |
| **值的預測** | load 讀到的值 | 最難；但是**唯一**能繞過 RAW 相依的方法 |

[04:45] 值的預測：RAW 是真的相依，之前說「沒辦法，producer 做完 consumer 才能做」。但如果能**猜出值**，producer 和 consumer 可以**平行**跑，相依就被打破了。

[05:35] 任何推測都可能猜錯，猜錯就要處理（[第 15 支](lecture15_note.md)）。

【課本補充】課本 Definition 23（p.153）：推測的步驟是：猜輸入 → 用它繼續執行 → 把結果傳給相依的指令 → 之後驗證，猜錯就取消所有受影響的指令。也可以猜一條指令的輸出或它要花多久。

【課本補充】課本 p.153：為什麼特別針對 load？load 的延遲**不確定**（可能從 store 轉送、可能從某一層 cache、可能從記憶體，幾百個 cycle），會讓一堆相依的指令在 window 裡排隊，叫 **convoy effect**（車隊效應）：像路上一台車拋錨，後面塞成一長串。

---

## 3. 位址預測：猜上一次的位址

[07:49] 跟 branch predictor 一樣的「通用預測器結構」（[第 8 支](lecture08_note.md)）：

```
PC 的最低 n 個 bit → 2ⁿ 格的表 → 每格存「這條 load 上次算出的位址」
```

[08:43] 通常只對 load 做（load 在關鍵路徑上），也可以對 store 做。

[09:11] 不像 branch predictor，這裡**不能用飽和計數器**當預測本身：要猜的是一個位址，不是「計數」的問題。

【課本補充】課本 p.154：可以存一些 PC 的 bit 減少 aliasing；也可以**另外**加一個 2-bit 飽和計數器表示「這個位址有多可信」：00 很可能錯、11 很可能對。

---

## 4. 位址預測：stride

[09:36] 課本的例子：

```c
int sum = 0, arr[10];
for (i = 0; i < 10; i++)
    sum += arr[i];
```

```
.loop: cmp r1, 10
       beq .exit
       ld  r3, [r0]       // 讀 arr[i]
       add r2, r2, r3
       add r0, r0, 4      // 位址 +4
       add r1, r1, 1
       b .loop
```

[11:47] 那條 `ld` 的位址每一圈加 **4**（int 是 4 byte）。這個固定的增量叫 **stride**。只猜「上次的位址」在這裡永遠不對。

[12:44] **Stride predictor**：每格存三樣：

| 欄位 | 意思 |
|---|---|
| **A** | 上次的位址 |
| **S** | stride |
| **P** | 是不是 stride 樣式 |

- 猜：P = 1 時猜 **A + S**。
- [14:10] 算出真的位址 A′ 後：比 A′ − A 是不是等於 S。是：A ← A′，S 不變，P 設 1。

[15:06] 不是的時候怎麼辦？老師說 P 可以是**飽和計數器**：

- [15:37] 例子：這段迴圈在某個函數裡。離開函數、很久以後又回來，第一次存取 stride 一定不對（A 是上一次呼叫留下的）。但行為並沒有變，這只是**暫時的異常**。
- 用飽和計數器就能忽略這一次，繼續照 stride 猜，後面都對。
- [16:53] 如果 stride **一直**變了，才改 S。
- [17:47] 要多少 hysteresis（遲滯）沒有一定規則：要在大家會跑的程式上跑**成百上千次模擬**決定。

⚠️ 課本 p.155 的規則比較直接：A′ − A ≠ S 時 P = 0，而且**不管對不對都** A ← A′、S ← A′ − A。影片說 P 可以是飽和計數器、偶爾一次不對不要改 stride。兩種本機都試了，見 [§12.1](#121-位址預測器猜得多準)。

【課本補充】課本 p.155：數學上至少要**三次**迭代才能認出 stride 樣式。

---

## 5. Load-store 相依預測：為什麼猜得到

[18:16] 觀察：一個 load 如果跟 pipeline 裡前面的 store **撞**（同位址），這通常是**穩定、可預測**的行為。

[19:11] 為什麼？很多程式碼是 **register spill**：

- 暫存器不夠用、或呼叫函數時，先把暫存器 store 到記憶體，之後再 load 回來。
- 這種 store-load 對離得不遠，**都還在 pipeline 裡**。
- 所以看到這個 load，可以很有信心地說：前面有個 store 會把值給它。

[20:05] 能猜得準的話：

| 猜 | 做法 |
|---|---|
| **不會撞** | 不用等 unresolved 的 store，直接送去 cache |
| **會跟某個 store 撞** | 就算中間有 unresolved 的 store，也不用等，直接轉送 |

[20:56] 【口頭】重要的哲學：「幾乎所有計算機結構的最佳化，某種意義上都是**以學習為基礎**的。」看程式設計師實際怎麼寫程式，再決定什麼硬體最能利用這些樣式。「如果沒有樣式，任何架構最佳化都不會有用。」而且這個樣式跨語言、跨 compiler、跨處理器都成立。

【課本補充】課本 p.155：沒有推測的話，假設 10 個 load 的位址都算好了，前面卻有 1 個 store 還沒算出位址，10 個 load 全都要等；最後發現沒撞，就是「白等了」。

---

## 6. Collision history table（CHT）

[21:47] 又是同一個樣板：

```
load 的 PC 的 n 個 bit → 2ⁿ 格的 CHT → 每格：撞 / 不撞（1 bit 或飽和計數器）
```

[22:52] 什麼時候查？兩個時間點都可以：**decode** 時，或**算出 load 位址**時。

[23:48] 怎麼用：

| 預測 | 做法 |
|---|---|
| **會撞** | 前面至少有一個同位址的 store。等前面所有 store 都 resolve，然後拿轉送的值、或去 data cache |
| **不會撞** | 位址一好就直接送去 data cache |

[24:43] 前面所有 store 都 resolve 之後，就知道到底撞了沒，**更新 CHT**。如果猜「不撞」但其實撞了，就要回復狀態。

[25:36] 好處：不撞的 load 不用等前面的 unresolved store，可以早送、早點喚醒 consumer → IPC 更高。

【課本補充】課本 p.156 引用 Yoaz et al. 1999。

---

## 7. 加上距離

[26:04] 在 decode 時預測，可以多存一個資訊：**store 到 load 的距離** d（中間隔幾格；分開的 load、store queue 的話，可以算前面有幾個 store）。

[26:34] 做法：load 等到它前面的 store queue 少於 d 格。這時還沒拿到轉送的值，就很可能不會再有了 → 送去 data cache。

[28:16] 用更多資訊 → 更多空間 → 預測更準、資訊更多。

【課本補充】課本 p.156–157 用函數的 save／restore 當例子：如果存暫存器的 store 和恢復它的 load 中間通常隔 10 個記憶體指令，load 就等到前面少於 10 格。課本說這樣主要的好處是**減少 replay**。

---

## 8. Store sets

[28:41] 「這方面最精緻的想法之一」：**store sets**（課本引用 Chrysos and Emer 1998）。

[29:15] 一個 load 的 **store set**：過去曾經把值轉送給它的所有 store。

兩張表：

| 表 | 用什麼查 | 存什麼 |
|---|---|---|
| **SSIT**（store set identifier table） | load 或 store 的 PC（n 個 bit） | **store set id** |
| **LFST**（last fetched store table） | store set id（例如 7 bit → 128 格） | 這個 store set 裡**最後抓進來的 store** 的**指令編號** |

[30:35] 簡單的版本：每個 store 只屬於**一個** store set。（後來的研究擴充成可以屬於很多個。）

[32:00] **指令編號**：pipeline 裡每條指令一個唯一的號碼。用一個繞圈的計數器，範圍要**大於 ROB 大小的兩倍**，這樣一條指令從進 ROB 到出去，不會碰到另一條同號碼的。

⚠️ 老師先說「兩倍」，又改口「比兩倍多」。課本 p.159 的例子是 ROB 160 格、用 8-bit 計數器（mod 256），只比 ROB 大、沒有到兩倍；課本說這樣就保證不會有兩條同號碼。只要唯一，大於 ROB 就夠；要多大才夠用來比新舊，影片和課本說法不同。

[35:45] 做法：

**Load**（decode 時）：

1. 用 PC 查 SSIT，拿到 store set id。
2. 用 id 查 LFST，拿到最後那個 store S 的指令編號。
3. Load 只等 **S**：S 還沒 resolve 就等它，或者 S 一知道要存什麼值，就**直接轉送**，甚至在 load 自己的位址算好之前。
4. [37:46] 中間其他的 store（就算 unresolved）**全部不管**：已經很有信心地猜到相依就是跟 S。

[39:06] 為什麼 store 的位址和值大約同時好：`st rx, 8[ry]` 會同時讀 rx 和 ry，算好位址時兩個一起寫進 LSQ（分開寫要更多 port）。

**Store**（decode 時）：查 SSIT 拿到 store set id，把自己的指令編號寫進 LFST（我是這個 store set 最新的 store）。

[41:23] 更新：偵測到一對 load-store 相依時：

- store set 已經有了 → 把 load（或 store）加進去；store 原本屬於別的就改掉。
- 還沒有 → 建一個新的 store set，讓 SSIT 裡的 store 和 load 都指向它。

[41:53] 比 CHT 更有效：CHT 只猜撞不撞（加距離也不太準，因為距離會變），store sets 猜**哪個 store 跟哪個 load 撞**，可以明確地等那個 store、轉送值。

【課本補充】課本 p.159：store 被選中後，從 register file 讀它要寫的值，直接給 load，load 可以**推測地**提早開始並 broadcast 結果。為此 wakeup／broadcast 要改一點：有 store set id 的 store 可以 broadcast 它的指令編號，等那個編號的 load 醒來拿值。store retire 時，如果 LFST 還指著它，要把那格清掉。

---

## 9. Load 延遲預測

[42:45] 記憶體階層（Ch7 細講），老師用的數字：

| 層 | 延遲 | 命中率 |
|---|---|---|
| L1 data cache | 1–2 cycle | 90% |
| L2 | 10–50 cycle | 50%（**local** hit rate：到 L2 的 100 個請求，滿足 50 個） |
| 主記憶體 | 300–400 cycle | （假設一定找得到） |

[44:13] 我們不知道一個 load 的資料在哪一層。【口頭】「所以我把頭埋在沙裡」，但可以**猜**：

- [44:38] 猜它會在 data cache 命中 → **early broadcast**，喚醒 consumer。
- 猜錯 → 下一講處理。

[45:06] **Hit predictor**：用 load 的 PC 猜 hit／miss，跟 branch predictor 一樣（1 bit 或飽和計數器）。

【口頭】[44:13]「我們很愛猜，所以這一小節叫 aggressive speculation：盡量猜，希望榨出最後一滴 ILP，就像在沙漠找水。」

【課本補充】課本 p.159–161：

- 保守的做法：等 load 真的讀到資料才喚醒 consumer，load 和 consumer 就不能背靠背。
- 假設每個 load 都在 L1 命中（大部分程式 L1 命中率 **85–90%** 以上，L1 命中延遲幾乎固定，**1–3 cycle**），Figure 5.4 的 pipeline 可以讓 consumer **早 2 個 cycle** 執行。
- 「今天幾乎所有高效能 OOO 處理器都有 load latency speculation」。
- 剩下約 10% 沒命中的，consumer 會拿到垃圾值，要取消、重做（replay）。
- 通常只對 L1 做 hit-miss predictor 就夠了：L2、L3 的延遲本來就不固定，猜也不準。

---

## 10. 值的預測

[45:35] 值的預測是打破 producer 和 consumer 之間 RAW 的**唯一**方法。最大的元兇是 **load**。

[46:03] 為什麼值猜得到：

| 來源 | 例子 |
|---|---|
| 資料重複 | 科學程式一直用 e、π |
| Bit masking | 很長的值裡只在乎幾個 bit |
| 常數 | 程式裡的常數 |
| 錯誤檢查的程式 | 幾乎從來不出錯，用的值幾乎都一樣 |
| **Virtual function** | 見下面 |
| Register spill | [§5](#5-load-store-相依預測為什麼猜得到) |

[47:20] Virtual function（C++）：

```cpp
Car *car = ...;           // Car 是 Vehicle 的子類別
Vehicle *v = car;
... v 傳到很遠的地方 ...
v->getType();             // 實際呼叫的是 Car::getType
```

[49:21] 每個物件帶一個小表，`getType` 那格存著真正函數的位址。讀這個表的 load 回傳的值**幾乎不會變** → 很好猜。

[49:52] 【口頭】硬體一直要跟著軟體的樣式走。80 年代中期還沒有 C++ 的市場，到 90 年代末變得很大，「現在大部分正經的軟體還是 C++ 和 .NET 語言寫的」。硬體設計師要讓 C++ 跑得快：

- 支援 function pointer → 要支援 **indirect branch**（目標在暫存器裡，[第 8 支](lecture08_note.md)）。
- 發現很多值猜得到 → 要 value predictor。

[51:38] Value predictor 的三種做法：

1. 猜**上一次的值**。
2. 值每次加固定的 **stride**。
3. **Profiling**：拿一組有代表性的輸入跑程式，程式裡插了一些額外的指令（最後的程式不會有），把值、分支方向等寫到檔案。分析這個檔案，再透過特殊指令「預先設定」硬體：例如「這個 load PC 最常讀到這個值」。

[53:26] 老師建議去看 **gprof** 在做什麼（「我不告訴你，自己去查」）。

【課本補充】課本 p.161–162：

- 早期研究：Lipasti et al. 1996；他們的表叫 **LVPT**（load value prediction table）。
- 課本多列了兩個來源：**輸入集**（例如處理 HTML 檔，開頭結尾的標籤都一樣）、**base address**（陣列、函數、物件的基底位址通常不變）。
- ⚠️ 課本說 stride 樣式比較適合**位址**，不太適合**值**：compiler 會盡量把常常 +1 的變數放在暫存器裡，更新不會到記憶體；只有 spill 時才寫，那時的值沒有固定樣式。影片則把 stride 列為 value predictor 的做法之一，沒有這個保留。
- Profiling 的研究：Gabbay and Mendelson 1997。

---

## 11. Confidence table

[53:53] 不像分支（**一定要**猜），這些東西很多時候**可以不猜**。

```
PC → confidence table（飽和計數器）：值不值得猜？
PC → predictor table（上一次的值、stride、最近 k 個值…）：猜什麼？
兩個同時查
```

[55:13] 不要硬猜低信心的東西，猜錯會浪費 cycle。結果出來後，兩張表**都更新**；也知道有沒有猜錯，猜錯就 replay／flush（下一講）。

---

## 12. 【實測】

程式在 `experiments/lecture14/`。

### 12.1 位址預測器猜得多準

`predict.py` 讀 valgrind lackey 的記憶體 trace（`work.c`：qsort 2 萬個亂數再做一點字串處理，見 [第 7 支](lecture07_note.md)，約 1 千萬條指令），每一個 load 都用三種預測器猜一次（表用完整 PC 當 key，不考慮 aliasing）：

```
load 共 2868046 個
1. last address      ：猜對  55.0%（每個 load 都猜）
2. stride（課本規則）：有猜的  78.0%，猜的裡面對  89.6%，等於全部 load 的  69.9%
   stride（影片規則）：有猜的  78.2%，猜的裡面對  90.5%，等於全部 load 的  70.8%
3. 真的跟最近 32 個 store 撞到的 load： 34.2%
   CHT 猜對（撞／不撞）： 93.4%；猜「不撞」結果撞了（要 replay）：  2.8%
```

- **Last address** 猜對 55%：很多 load 每次都讀同一個地方（例如 stack 上的區域變數）。
- **Stride** 能多猜對約 15 個百分點（55% → 70%）：qsort 掃陣列的 load 每次加固定的量。stride 0 也是一種 stride，所以 stride predictor 自動包含「上次的位址」。
- 課本規則和影片的飽和計數器規則差不多，影片的稍好一點（90.5% 對 89.6%）。
- **相依**：約三分之一的 load 跟最近 32 個 store 撞到位址，大多是 stack 上的 spill／區域變數（[§5](#5-load-store-相依預測為什麼猜得到) 說的）。2-bit 的 CHT 猜對 93%；真正危險的「猜不撞、結果撞了」只有 2.8%。所以 load 大部分時候可以放心不等 unresolved 的 store。
- 「最近 32 個 store」只是近似「還在 pipeline 裡的 store」，真的 store queue 大小這裡不知道。

### 12.2 本機的 load 會不會越過還沒算出位址的 store

`disamb.c`：每一圈先有一個 **store**，它的位址要經過 4 次 `imul`（約 12 cycle）才算出來；然後一個 **load** 做 pointer chase（下一圈要用它的值）。`experiments/lecture14/disamb.c:16-19`：

```c
		asm volatile("mov %0, %1\n\timul $1, %1\n\timul $1, %1\n\timul $1, %1\n\timul $1, %1\n\tand $0, %1\n\t"
			     "mov %3, (%2,%1)\n\t"	/* store：位址 = dst + t，很晚才知道 */
			     "mov (%0), %0"		/* load：q = [q] */
			     : "+r"(q), "=&r"(t) : "r"(dst), "r"((long)cell) : "memory");
```

- **diff**：store 寫另一個陣列，跟 load 不撞。
- **same**：store 寫的就是 load 要讀的那一格（真的相依）。

| | P-core（cpu2） | ≈ cycle | E-core（cpu14） | ≈ cycle |
|---|---|---|---|---|
| diff | 1.07–1.08 ns/圈 | **≈ 5** | 0.836 ns/圈 | **≈ 3** |
| same | 4.28–4.33 ns/圈 | **≈ 20** | 多半 0.64，偶爾 4.0 或 8.2 | ⚠️ 不穩定 |

- **diff 每圈只要 5 cycle**（P-core），就是一次 L1 load 的延遲（[第 4 支](lecture04_note.md) 量到約 5 cycle）。如果 load 要等前面的 store 算出位址，每圈至少要 12 cycle 以上。所以本機的 CPU **真的會猜「不撞」，讓 load 越過 unresolved 的 store**，就是 [§6](#6-collision-history-tablecht) 說的 load-store 相依推測。
- **same 每圈約 20 cycle**（P-core）：約 12（imul）＋ 1（and）＋ 約 6–7（store 轉送給 load，跟 [第 12 支 §16.3](lecture12_note.md#163-真的-cpu-的-store-到-load-轉送) 的 `idx` 差不多）。這時 CPU 好像學到了「這個 load 跟這個 store 撞」，乖乖等，沒有一直猜錯、一直 replay（如果一直猜錯，應該會慢很多）。
- ⚠️ E-core 的 `same` 大多只要 0.64 ns（約 2.3 cycle），比 `diff` 還快，偶爾又跳到 4 或 8 ns。看起來 E-core 在 store 位址還沒算出來前，就把 store 的**值**直接給了 load（像 [§8](#8-store-sets) 說的「位址算好之前就轉送」），而且預測器的狀態會變。實際機制 Intel 的公開資料這裡沒有，**待補**。

---

## 13. 一頁速查表

| 主題 | 重點 |
|---|---|
| 推測 | 猜 → 繼續 → 傳給相依的 → 驗證 → 錯了取消重做 |
| 為什麼 load | 延遲不確定；convoy effect |
| 預測器樣板 | PC 的 n bit → 2ⁿ 格表（＋tag、飽和計數器） |
| 位址：last | 存上次的位址 |
| 位址：stride | 存 A、S、P；猜 A+S；P 可以是飽和計數器（容忍偶發異常） |
| 相依：為什麼猜得到 | register spill、函數呼叫的 store-load 對；架構最佳化都是學程式的樣式 |
| CHT | 猜撞不撞；不撞就不等 unresolved store；撞就等全部 resolve |
| CHT＋距離 | 等到前面少於 d 個 store，還沒轉送就去 cache |
| Store sets | SSIT（PC → store set id）＋ LFST（id → 最後抓的 store 的指令編號）；只等那一個 store |
| 指令編號 | 繞圈計數器；影片說 > 2 × ROB，課本 ROB 160 用 mod 256 |
| 延遲 | 猜 L1 命中 → early broadcast；hit predictor；L1 命中率 85–90% |
| 值 | 唯一打破 RAW 的方法；常數、virtual function、spill；last value、stride、profiling（LVPT） |
| Confidence | 可以不猜：先問 confidence table |
| 實測 | stride 猜對 70%、last 55%；CHT 93%；P-core load 會越過 unresolved store（5 cycle 對 20 cycle） |

---

## 14. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| 通用預測器樣板、飽和計數器、indirect branch | [第 8 支](lecture08_note.md) |
| LSQ、unresolved store、轉送 | [第 12 支](lecture12_note.md) |
| Early broadcast | [第 11 支](lecture11_note.md) |
| 猜錯時怎麼回復：replay | [第 15 支](lecture15_note.md) |
| Compiler、profiling | [第 16 支](lecture16_note.md) |
| 記憶體階層、命中率 | [第 21 支](lecture21_note.md)–[第 26 支](lecture26_note.md)（Ch7） |
| 資料 prefetch（也用 stride） | [第 26 支](lecture26_note.md) |

---

## 附：本講一句話總結

> **Ch5 先把 OOO 變得更積極：對延遲不確定的 load 做四種推測：用跟 branch predictor 一樣的「PC 查表」樣板猜位址（上一次的位址或 A+S 的 stride，用飽和計數器容忍偶發的異常）、猜 load 跟前面的 store 撞不撞（CHT，可以加上距離）或跟哪一個 store 撞（store sets：SSIT 查出 store set id、LFST 查出最後抓進來的 store，只等它、甚至在位址算好前就轉送）、猜 load 會在 L1 命中所以早點 broadcast、以及猜 load 讀到的值（常數、virtual function 表、register spill 都很好猜，這是唯一打破 RAW 的方法），再用 confidence table 決定值不值得猜，猜錯就交給下一講的 replay。本機實測：在一個真的程式的 trace 上，stride predictor 猜對 70% 的 load 位址、CHT 猜對 93% 的撞與不撞；而真的 P-core 會讓 load 越過還沒算出位址的 store（不撞時每圈 5 cycle，撞時乖乖等約 20 cycle）。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 5: Alternative Approaches to Issue and Commit - Part I (Aggressive Speculation) 的 YouTube 自動字幕（56:03）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch5 §5.1（p.152–162）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10、valgrind 3.18）結果。*
