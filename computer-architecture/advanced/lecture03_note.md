# Advanced Computer Architecture 第三支影片完整筆記：導論：為什麼要 out-of-order、multicore 與這本書的地圖（Chapter 1）

> **課程**：Advanced Computer Architecture — Chapter 1: Introduction（播放清單第 3 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（24:12，自動字幕）＋ 課本 *Next-Gen Computer Architecture: Till the End of Silicon* v3.2 **Ch1**（p.13–21）
> **章號**：影片 Chapter 1 = 課本 Ch1。
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
- [1. 先修知識](#1-先修知識)
- [2. 原子、位元、基因](#2-原子位元基因)
- [3. Moore's law 快停了](#3-moores-law-快停了)
- [4. Dennard scaling](#4-dennard-scaling)
- [5. 一次發很多條 in-order：效果不好](#5-一次發很多條-in-order效果不好)
- [6. Out-of-order：不照順序做](#6-out-of-order不照順序做)
- [7. 單核到頂 → multicore、accelerator、GPU](#7-單核到頂--multicoreacceleratorgpu)
- [8. 第二部分：記憶體系統](#8-第二部分記憶體系統)
- [9. 第三部分：功耗、可靠度、安全、ML](#9-第三部分功耗可靠度安全ml)
- [10. 【實測】](#10-實測)
- [11. 一頁速查表](#11-一頁速查表)
- [12. 與後續講次的連結](#12-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

這是整本書的導論。老師用兩個數字說明為什麼要學進階課：

1. 一次發 4 條指令、但**照順序**做：只快 **37.5%**。
2. 改成 **out-of-order**（不照順序做）：快 **2.3–2.4 倍**。

然後介紹書的三個部分：

| 部分 | 章 | 內容 |
|---|---|---|
| Part I：處理器 | Ch2–6 | out-of-order pipeline（Ch2–5）、GPU（Ch6） |
| Part II：記憶體系統 | Ch7–10 | cache、on-chip network、multicore 的 coherence／consistency、main memory |
| Part III：其他議題 | Ch11–14 | 功耗與溫度、可靠度、安全、ML 處理器 |

【口頭】老師說錄影時是 **2021 年**。

---

## 1. 先修知識

[00:23] 要看懂這本書，需要：

- **組合語言**：ARM、x86 都可以，至少要懂基本概念。
- **基本的處理器設計**。
- **基本的 pipeline 設計**。

[00:56] 不懂的人，請看老師的前一本書（大學部的 *Basic Computer Architecture*）。在本 repo 就是 [Basic 課程](../basic/README.md)。

---

## 2. 原子、位元、基因

[01:27] 【口頭】過去 50 年真正改變人類生活的，是電腦：更好、更快、更省電。老師的說法：

| 時代 | 年份 | 主角 |
|---|---|---|
| 原子的時代 | 大約 1850／1900–1950 | 原子物理、材料 |
| **位元的時代** | 1950–（希望再 100 年） | 電腦 |
| 基因的時代 | 以後 | 生物學（「我還看不了那麼遠」） |

---

## 3. Moore's law 快停了

[02:42] Gordon Moore（Intel 共同創辦人）預測：每顆晶片的電晶體數量**每年**翻倍；後來慢慢變成每 **2 年**。1970–2015 在 log 圖上大致是一條直線，也就是指數成長。現在一顆晶片超過 **100 億個**電晶體。

[03:51] 計算機結構有兩條很重要的經驗法則（empirical observation），第一條就是 Moore's law：**電晶體數每 1–2 年翻倍**。到現在大致成立，但預期會**突然停下來**。

[04:17] 原因：電晶體已經非常小了。**Feature size**（矽上能做出的最小結構）在 2021 年大約 **7 nm**，也許還能到 5 或 3 nm，就到底了。

⚠️ **幾個原子寬**：

| 來源 | 說法 |
|---|---|
| 影片 [04:45] | 「矽原子大約 200 nanometers」（顯然是口誤或字幕錯，應是約 0.2 nm），所以 7 nm 只有 **35–40 個原子** |
| 課本 p.14 | 「5 nm 的結構只有 **25 個矽原子**寬」 |
| Basic 第 47 支 [26:31] | 原子約 1 Å，5 nm ≈ **50 個原子** |

三個數字都不一樣，但結論相同：沒剩幾個原子了。

[05:15] 這本書要回答的問題：**在電晶體不再變小的情況下，怎麼讓電腦拿到最大的效能？** 這就是現代計算機結構。

【課本補充】課本 Definition 1（p.14）：Moore's law 從 1965 年提出，大約**到 2012 年**都成立；feature size 每一代縮小 √2 倍。⚠️ 影片說到 **2015 年**都大致成立。

---

## 4. Dennard scaling

[05:27] 第二條法則是 Moore's law 的「姊妹」：**Dennard scaling**。

影片和課本說法不太一樣：

| 來源 | Dennard scaling 是什麼 |
|---|---|
| 影片 [05:43] | **功率密度（power density）不變** |
| 課本 p.15 | Robert Dennard 在 **1974** 年提出：**每瓦效能（performance per Watt）也指數成長**，跟著 Moore's law 走 |

影片其實把兩個串起來講：

1. [05:55] 晶片面積因為良率（yield）一直差不多：伺服器晶片大約 2 cm × 2 cm = **400 mm²**，桌機／筆電大約 **150–400 mm²**。
2. [06:25] 功率密度不變 + 面積不變 → **總功率不變**（由散熱速度等因素決定）。
3. [06:38] 效能指數成長、瓦數不變 → **每瓦效能也指數成長**。

[07:15] 結束了：Moore's law 變慢，每瓦效能也不再指數成長。**Dennard scaling 在 2006 年結束**（影片和課本 p.15 一致）。

【課本補充】課本 p.15 說：每瓦效能要指數成長，必須每一代都提高**頻率**和**每個 cycle 做的指令數**；2006 年以後做不到，因為晶片的功耗成長超過效能成長。

[07:45] 以前的創新大多只是「因為可以多放電晶體」；2006 年後這 15 年，計算機結構的思考方式變很多。

---

## 5. 一次發很多條 in-order：效果不好

[08:11] 大學部的課停在 **5 級 in-order pipeline**：指令一條一條流過 5 個 stage，**沒有指令會超車**。

[08:38] 最直接的加速法：一次**發（issue）**好幾條指令下去。指令之間的相依、額外的排程和 forwarding 會有成本，但多了平行度，應該會變快吧？

[09:01] 用 **SPEC** benchmark（Standard Performance Evaluation Corporation 的標準測試程式）比較 issue width 1 和 4：

- 平均（取 **geometric mean**，幾何平均）只快 **35–40%**（影片 [09:37]）；課本 Figure 1.2（p.15）說 **37.5%**。
- 大多數程式快 20–50%；bzip2、mcf、omnetpp 多一點。
- 【口頭】[10:05] 這點加速，等 2–4 年電晶體變快就自然有了，不值得。

【課本補充】課本 p.15–16：4-issue 要 4 倍的功能單元（例如 4 個加法器）；數字是用老師的模擬器 **Tejas** 跑 SPEC CPU 2006 得到的（Appendix B）。「效能」定義為執行時間的倒數。

---

## 6. Out-of-order：不照順序做

[10:17] 下一個做法是這本書的重點：**out-of-order（OOO，亂序）**處理器。

[10:32] 課本 Figure 1.3（p.16）的程式，`add r1, r2, r3` 表示 r1 ← r2 + r3：

```
add r1, r2, r3
mul r4, r1, r1     ← 用 r1（RAW）
add r5, r4, r1     ← 用 r4、r1
mov r8, 5
add r9, r8, r8     ← 用 r8
sub r10, r9, r8    ← 用 r9、r8
```

兩條**互不相干的相依鏈**：前三條一串、後三條一串（RAW = read after write，寫了才能讀）。

[11:13] **2-issue in-order**：只有第 3、4 條能一起做，總共 **5 個 cycle**。一條一條做是 6 個 cycle，只省了 1/6。

[12:03] **2-issue OOO**：只要不違反相依，就可以挑別的指令先做：

| Cycle | 做什麼 |
|---|---|
| 1 | `add r1` ＋ `mov r8` |
| 2 | `mul r4` ＋ `add r9` |
| 3 | `add r5` ＋ `sub r10` |

只要 **3 個 cycle**。怎麼挑是另一回事，第 4、5 章會細講。

⚠️ **快多少**：影片 [13:02] 說這是 **2 倍**加速（跟一條一條做的 6 cycle 比）；課本 p.16 說「5 cycle → 3 cycle，**效能提高 40%**」（跟 2-issue in-order 比）。比較的基準不一樣。另外，5 → 3 其實是時間少 40%、速度變 5/3 ≈ 1.67 倍。

[13:15] 用 SPEC 比較 4-issue in-order 和 4-issue OOO：

- 影片 [13:42]：平均 **2.4 倍**，有些程式更多。
- 課本 Figure 1.4（p.17）：平均效能**提高 133%**（= 2.33 倍）。兩個大致一致。

原因：OOO 處理器有很多 in-order 處理器沒有的技巧。

【課本補充】課本 p.16：OOO 不照程式順序執行，但**一定遵守所有資料相依和控制相依**。困難在於：要確保沒有違反相依、所有 branch 都解對、沒有錯的東西寫進記憶體。課本用 4 章來講。

【課本補充】課本的註腳（p.16）：書裡一律用 *dependence*，不用 *dependency*，因為後者通常指「被別國控制的地區」。

---

## 7. 單核到頂 → multicore、accelerator、GPU

[14:35] 到第 5 章結束時會看到：業界和學術界花了很多力氣讓 OOO 處理器更快。

[15:02] 2001–2012 每顆處理器的 SPECint 分數：一直往上爬，然後**飽和**。

⚠️ **何時飽和**：影片 [15:15] 說 **2012 年以後**；課本 Figure 1.5（p.17）說 **2009 年起**。

[15:26] **Core（核心）**：一條 pipeline ＋ 它的 cache（data cache、instruction cache）。

[15:52] 單核到頂，大家往上一層走：

| 階段 | 內容 |
|---|---|
| Multicore | 一顆晶片上很多個小處理器，每個能跑一個程式 |
| Multicore ＋ accelerator | **Accelerator（加速器）**：做特定工作的硬體，例如神經網路、語音處理、XML、加密、壓縮 |
| **GPU** | Multicore 的一個重要分支，自成一格；原本只做圖形，後來也拿來跑一般程式（Ch6） |

[16:45] 2010–2020 這十年屬於各式各樣的 multicore。

【課本補充】課本 §1.2（p.17–19）：

- 平行程式：N 個互不相干的迴圈 iteration 可以分給 N 個 core，理論上快 N 倍，實際上少很多，因為 iteration 之間常有相依。
- GPU 有 512–4096 個很小的 core（2020 年）；伺服器處理器約 10 GFLOPS，最新 GPU 約 15 TFLOPS。只適合大矩陣運算，不適合溝通複雜的平行程式。
- 大型 multicore（10–20 個 core，每個都是複雜的 OOO pipeline）需要 **cache coherence** 和 **memory consistency**（Ch9）。
- **Transactional memory**：把一段程式標起來，讓它看起來「一瞬間做完」，初學者也能寫平行程式；分 STM（軟體）和 HTM（硬體）（Ch9）。

---

## 8. 第二部分：記憶體系統

[17:31] Multicore 需要**高頻寬、低延遲**的記憶體系統，所以記憶體系統變得非常複雜，尤其是 on-chip cache 的存取協定。

| 章 | 內容 |
|---|---|
| Ch7 | **Cache** 設計（on-chip 記憶體） |
| Ch8 | **On-chip network**：很多 cache 要連起來，本身就是很深的領域 |
| Ch9 | **Multiprocessor 的記憶體系統** |
| Ch10 | **Off-chip 記憶體**：DRAM，以及 ReRAM、STT、flash 這類 **NVM**（non-volatile memory，斷電也記得） |

【課本補充】課本 §1.3（p.19）：on-chip cache 用 SRAM、off-chip 用 DRAM；core 少的時候 NoC（network-on-chip）不重要，20 個以上 core 時就很重要；Intel Xeon Phi 有 70–80 個 core。

---

## 9. 第三部分：功耗、可靠度、安全、ML

[19:48]

| 章 | 內容 |
|---|---|
| Ch11 | **功耗與溫度**：它們讓處理器沒辦法再變複雜；也是造成可靠度問題最大的原因 |
| Ch12 | **可靠度**：短暫的錯和永久的錯（hard error）；宇宙射線造成的錯；**parameter variation**（製造或操作條件不理想，電晶體規格偏離；電壓和溫度也會變動） |
| Ch13 | **安全** |
| Ch14 | **ML 處理器**：下一個十年（⚠️ 影片 [22:04] 說「2020 to 2013」，應是 2030）的大晶片會有很多 on-chip accelerator：電腦視覺、機器學習、語音 |

[22:28] 【口頭】例子：**手勢操作電視**。站在電視前揮手，頻道就換。小攝影機抓到影像 → 送給處理器 → 一般處理器算不動 → 小的 ML accelerator 處理 → 把結果送給 host 程式 → host 程式把頻道加 1 或減 1。

【課本補充】課本 §1.4（p.19–20）：

- **Leakage power（漏電功率）**是溫度的**超線性**函數 → 功率↑ → 溫度↑ → 漏電↑ → 正回饋；嚴重時叫 **thermal runaway**。
- 用 193 nm 的光做 14 nm 的結構，長方形會印成圓角（Basic 第 47 支也講過）。
- 高溫讓線和電晶體壞掉的速率**指數上升**，也會造成電晶體 **ageing**（老化）。
- **Soft error**：alpha 粒子或宇宙射線造成電流脈衝，翻轉 ALU 或 latch 裡的 bit。

【課本補充】課本 §1.5（p.20–21）：安全的例子是駭客入侵心律調節器。§1.6（p.21）：feature size 不會低於 5 nm，所以下一步是 application-specific 的處理器，最主要是 CNN 的 accelerator。

---

## 10. 【實測】

程式在 `experiments/lecture03/`。

### 10.1 課本 Figure 1.3 的排程

`sched.py` 把 Figure 1.3 的 6 條指令，用兩種規則排一次（每條 1 個 cycle）：

- in-order：最舊的指令卡住，後面的也不能走。
- OOO：從還沒做的指令裡，挑最多 2 條「來源都好了」的。

```
2-issue in-order：5 cycles
  cycle 1: add r1, r2, r3
  cycle 2: mul r4, r1, r1
  cycle 3: add r5, r4, r1 | mov r8, 5
  cycle 4: add r9, r8, r8
  cycle 5: sub r10, r9, r8
2-issue OOO：3 cycles
  cycle 1: add r1, r2, r3 | mov r8, 5
  cycle 2: mul r4, r1, r1 | add r9, r8, r8
  cycle 3: add r5, r4, r1 | sub r10, r9, r8
```

跟課本、影片完全一樣。

### 10.2 本機的 out-of-order 視窗有多大

OOO 最大的用處：一個 load 要等記憶體幾百個 cycle 時，後面不相干的指令可以先做。但處理器能「往前看」多遠是有限的（課本 p.136 說，指令要等前面的都做完才能離開 **ROB**，reorder buffer；ROB 滿了就卡住）。

`window.c`：兩條**互不相干**的 pointer chase（每一步都 cache miss，陣列 512 MB），中間各夾 K 個 `nop`。`experiments/lecture03/window.c:26-29`：

```c
	for (int i = 0; i < ITER; i++)
		asm volatile("mov (%0), %0\n\t.rept " STR(K) "\n\tnop\n\t.endr\n\t"
			     "mov (%1), %1\n\t.rept " STR(K) "\n\tnop\n\t.endr"
			     : "+r"(p), "+r"(q));
```

- K 小：第二個 load 在視窗裡，兩個 miss **同時**等 → 每圈約 1 次 miss 的時間。
- K 超過視窗：第二個 load 進不來，要等第一個做完 → 每圈 2 次 miss。

```
P-core（cpu2）                  E-core（cpu14）
K=   0    99.1 ns/圈            K=   0   112.6 ns/圈
K= 256   109.0                  K= 240   133.0
K= 384   116.4                  K= 248   141.6
K= 480   122.2                  K= 252   230.1   ← 跳
K= 504   128.1                  K= 256   230.5
K= 512   202.6   ← 跳           K= 384   243.6
K= 768   229.2                  K= 768   288.8
```

- **P-core**：K 到 504 都還是約 1 次 miss（100–128 ns），K = 512 突然跳到 203 ns（約 2 次）。視窗大約 **500 多條指令**。
- **E-core**：K = 248 → 252 跳。視窗大約 **250 條指令**，是 P-core 的一半。
- 兩條 chase 能同時等，就是 OOO 的好處：in-order 處理器在第一個 miss 就停住，永遠是每圈 2 次 miss。
- ⚠️ 課本 p.136 說「現代 OOO 處理器的 ROB 大約 **100–200** 個 entry」；本機量到的視窗大很多（P-core 約 500、E-core 約 250）。這裡量的是「能夾多少個 nop」，和 ROB 大小相關，但不一定剛好等於 ROB entry 數。（[第 13 支 §14.1](lecture13_note.md#141-本機的-rob-有多大) 對照後認為：夾 `nop` 量到的上限最可能就是 ROB 大小。）

---

## 11. 一頁速查表

| 主題 | 重點 |
|---|---|
| 先修 | 組合語言、基本處理器、基本 pipeline（Basic 課程） |
| Moore's law | 每 1–2 年翻倍；課本說到 2012、影片說到 2015；7 nm（2021），也許到 5、3 nm |
| 原子數 | ⚠️ 影片 7 nm ≈ 35–40 個；課本 5 nm = 25 個矽原子 |
| Dennard scaling | 影片：功率密度不變；課本：每瓦效能指數成長（1974）；2006 年結束 |
| 4-issue in-order | 只快 37.5%（課本）／35–40%（影片） |
| Figure 1.3 | 2-issue in-order 5 cycle，2-issue OOO 3 cycle |
| 4-issue OOO vs in-order | 2.33 倍（課本 133%）／2.4 倍（影片） |
| 單核飽和 | 課本 2009 年起、影片 2012 年後 |
| Core | 一條 pipeline ＋ 它的 cache |
| 之後 | multicore → multicore ＋ accelerator → GPU |
| Part I | Ch2–5 OOO、Ch6 GPU |
| Part II | Ch7 cache、Ch8 NoC、Ch9 multicore 記憶體、Ch10 DRAM／NVM |
| Part III | Ch11 功耗溫度、Ch12 可靠度、Ch13 安全、Ch14 ML |
| 實測 | Figure 1.3 排程重現；本機 OOO 視窗 P-core 約 500、E-core 約 250 條指令 |

---

## 12. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| 5 級 in-order pipeline、RAW hazard | [Basic 第 30 支](../basic/lecture30_note.md)、[Basic 第 32 支](../basic/lecture32_note.md) |
| Moore's law、multicore、Amdahl | [Basic 第 38 支](../basic/lecture38_note.md) |
| 過去、現在、未來（同一套故事的長版） | [Basic 第 47 支](../basic/lecture47_note.md) |
| OOO pipeline 概觀 | [第 4 支](lecture04_note.md)（Ch2） |
| ROB、commit | [第 13 支](lecture13_note.md)（Ch4） |
| GPU | [第 18 支](lecture18_note.md)（Ch6）；基礎見 [Basic 第 42 支](../basic/lecture42_note.md) |
| Cache、NoC、coherence、DRAM | [第 21 支](lecture21_note.md)、[第 27 支](lecture27_note.md)、[第 32 支](lecture32_note.md)、[第 41 支](lecture41_note.md) |
| 功耗、可靠度、安全、ML | [第 46 支](lecture46_note.md)、[第 49 支](lecture49_note.md)、[第 52 支](lecture52_note.md)、[第 55 支](lecture55_note.md) |

---

## 附：本講一句話總結

> **Moore's law 快停了（7 nm，也許到 5、3 nm 就只剩幾十個原子），Dennard scaling 在 2006 年就結束了，所以這本書要回答「電晶體不再變小，怎麼還拿到最多效能」；第一個答案是 out-of-order：一次發 4 條但照順序做只快 37.5%，不照順序做（只要遵守相依）卻快 2.3–2.4 倍，像課本 Figure 1.3 的 6 條指令從 5 cycle 變 3 cycle；單核在 2009–2012 年飽和以後，業界改走 multicore、accelerator 和 GPU，這又需要高頻寬的記憶體系統（cache、on-chip network、coherence、DRAM／NVM），最後還要面對功耗、溫度、可靠度、安全，以及 ML accelerator 這個新方向。本機實測：P-core 能在一個 cache miss 後面再往前看約 500 條指令，E-core 約 250 條。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 1: Introduction 錄影逐字稿（YouTube 自動字幕，24:12）、Sarangi *Next-Gen Computer Architecture: Till the End of Silicon* v3.2 Ch1（p.13–21）、p.136 整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
