# 設計自動化（一）第一支影片完整筆記：課程介紹——EDA 是什麼、產業與課程安排（ch1 Introduction）

> **課程**：Electronic Design Automation (1)（清大 CSR 5302）— Lecture 1 | Ch1. Introduction（播放清單第 1 支）
> **講者**：李炯霆（JT Li），清大半導體學院，2026 春，NTHU OCW 錄影
> **來源**：YouTube 錄影（42:06，無字幕，whisper 轉錄）＋ 投影片截圖 ＋ 課本 Wang, Chang, Cheng（eds.）*Electronic Design Automation* Ch1 §1.1（p.1–12）
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、投影片和課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【實測】`：在本機實際執行的結果（本講沒有）
> - `⚠️`：逐字稿辨識不清、或影片／投影片／課本說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 教材的來歷與致謝](#1-教材的來歷與致謝)
- [2. 上課資訊與參考書](#2-上課資訊與參考書)
- [3. 課程目標](#3-課程目標)
- [4. 為什麼需要 EDA：晶片越來越大](#4-為什麼需要-eda晶片越來越大)
- [5. 車用電子也需要 EDA](#5-車用電子也需要-eda)
- [6. EDA 產業有多大](#6-eda-產業有多大)
- [7. David Pan 的 EDA 介紹](#7-david-pan-的-eda-介紹)
- [8. EDA = 硬體和軟體的交會](#8-eda--硬體和軟體的交會)
- [9. 為什麼做 EDA 工具很刺激](#9-為什麼做-eda-工具很刺激)
- [10. 職涯路徑與職缺要求](#10-職涯路徑與職缺要求)
- [11. 這門課在整個 EDA 的位置](#11-這門課在整個-eda-的位置)
- [12. 課程大綱](#12-課程大綱)
- [13. 評分、作業與程式](#13-評分作業與程式)
- [14. 一頁速查表](#14-一頁速查表)
- [15. 與後續講次的連結](#15-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

這一講沒有演算法，是整門課的開場：

```
教材從哪來 → 上課資訊、參考書
   → 為什麼要 EDA：晶片從 A10 的 33 億到 A15 的 150 億顆電晶體，沒有工具做不出來
   → EDA 產業：約占半導體營收 2.5%，也約占軟體產業營收 2.4%
   → EDA = EE × CS（＋數學），要求正確、生產力、最佳化、可擴展
   → 職涯：technical fellow／product specialist／management／marketing & sales
   → 本學期：digital SoC 的 physical design（compaction、floorplan、placement、routing）＋ logic synthesis
   → 評分：作業 18%、程式 60%、期中 12%、參與 10%
```

---

## 1. 教材的來歷與致謝

[00:00]–[03:46]　投影片第 1–2 頁

- 投影片標題：**CSR 5302 Electronic Design Automation (1)**，李炯霆（JT Li），Spring 2026。
- 教材的底子是**二十多年前教育部「超大型積體電路與系統設計教育改進計畫」（EDA 聯盟）**請各校 EDA 教授一起編的教材。投影片列出的教授：
  - 台灣大學電機系 張耀文
  - 清華大學電機系 黃錫瑜
  - 交通大學資科系 李毅郎
  - 中央大學電機系 劉建男
  - 元智大學資工系 林榮彬
  - 台灣大學 Ric Huang、Roland Jiang
- 【口頭】當年國家為了推 VLSI 與 EDA，請這些教授合作，提升台灣在這方面的水準。教材有點舊，老師加了新的內容。
- 【口頭】老師也去問了台大的教授他們 EDA 教哪些東西；去年到中山大學分享過這份教材，所以決定把 DA1 **開源**（放上 OCW）。
- 其他來源（Acknowledgement 頁）：
  - Stanford、UT Austin、UC Berkeley、Georgia Tech 的 EDA 課程教材；Synopsys、Cadence、Siemens（Mentor）、Ansys、Altium 的公開網站、演講、webinar。
  - 【口頭】EDA 廠商在網站上介紹新功能，老師會想「這些功能是怎麼實作的」，再依自己的了解寫下來分享。
  - Raul Camposano、Antun Domic、Patrick Groeneveld（在 Stanford 教 EE292A）。【口頭】Antun Domic 是老師以前的主管，把 EE292A Chapter 1 的投影片送給老師。
  - Tung-Chieh Chen：【口頭】老師以前的 manager，分享了台大 Design Automation 訓練課程的教材。⚠️ 逐字稿寫成「陳同杰」，中文名待確認。（他也是課本 Ch10 Floorplanning 的作者。）
- 感謝指導教授 **Ernest Kuh**（Berkeley）：【口頭】帶老師進 EDA 領域。

---

## 2. 上課資訊與參考書

[03:46]–[06:12]　投影片 Logistics

| 項目 | 內容 |
|---|---|
| 時間地點 | 星期一 14:20–17:20 @LRB 教室 |
| 講者 | JT Li（jtli@mx.nthu.edu.tw）；辦公室：綜三 4 樓 C07 |
| Office hour | 星期一課前課後，或另約 |
| 助教 | 待宣布 |
| 先修 | logic design、data structures（＋基本的 transistor design） |

- 【口頭】最重要的先修是 **data structure**；transistor design 要懂 gate level。
- 參考書（投影片綠框）：
  1. S. H. Gerez, *Algorithms for VLSI Design Automation*, Wiley, 1999。【口頭】雖然是二十多年前的書，圖書館應該有電子版。
  2. Y.-W. Chang, K.-T. Cheng, L.-T. Wang（eds.）, *Electronic Design Automation: Synthesis, Verification, and Test*, Elsevier, 2009 ——**本筆記對照的課本**（`../books/eda/eda.pdf`）。
  3. Naveed A. Sherwani, *Algorithms for VLSI Physical Design Automation*（投影片附了一個網路上的 PDF 連結）。
  4. Gary D. Hachtel, Fabio Somenzi, *Logic Synthesis and Verification Algorithms* ——講 synthesis 時會用到。
  5. Sung Kyu Lim, *Practical Problems in VLSI Physical Design Automation*。【口頭】Lim 教授在 Georgia Tech 教 physical design automation，用很實際的問題來講。
- 【口頭】老師會盡量用白話把經典演算法講清楚。

---

## 3. 課程目標

[06:12]–[08:06]　投影片 Course Info (1)

1. 了解 EDA 是什麼——**領域非常廣**。【口頭】老師做了三十多年也只碰到一小部分。
2. 這學期偏 **digital SoC** 設計。
3. 學幾個 **physical design** 的重要演算法。
4. 引起興趣——**EDA 很需要軟體人才**。
5. 學會**問題建模（problem formulation）與設計演算法來解**，這能延伸到別的領域。
- 【口頭】EDA 裡很多問題是 **NP-complete／NP-hard**：找不到 polynomial time 的演算法。所以要學的是怎麼把 IC 設計問題 formulate 起來、想出什麼演算法來解、看結果、再往下走。
- 投影片：**Welcome questions! They will make us to understand more clearly.**

---

## 4. 為什麼需要 EDA：晶片越來越大

[08:06]–[11:33]　投影片 iPhone Processors

- 投影片圖（取自 Stanford 的課）：Apple A4 → A10 每一代的晶粒大小疊在一起。
  - A10：**3.28 B（32.8 億）顆電晶體**
  - A12：**70 億**顆，7 nm（【口頭】將近 double）
  - A15（iPhone 13）：**150 億**顆，5 nm
- 投影片：**EDA plays a critical role!** 晶片越來越大，所以要問：

| 問題 | 【口頭】老師的說明 |
|---|---|
| How to synthesize? | 這麼大的晶片不靠 EDA 工具不可能做出來 |
| How to place? | 這麼多 gate 要怎麼擺、擺好 |
| How to route? | 擺好才能拉線；拉線要有空間 |
| DRC / LVS | 不能有 DRC（design rule check）violation，不然做不出來；拉完要做 LVS（layout vs. schematic），確定做出來的功能和原本要的一致 |
| RC extraction | 線擺得很近，上面帶電容、電阻，影響 timing delay |
| How to fix timing? | 這麼大的設計怎麼分析 timing → **STA（static timing analysis）**很重要；找到問題怎麼修 |
| Database | 設計很大，要 zoom in／zoom out、highlight 一條 net 走過的地方來 trace connectivity，GUI 和資料庫很重要 |
| ECO | Engineering Change Order：修了 timing 以後 layout 要改，改了要重新擺、重新拉 |

⚠️ 逐字稿把 STA 辨識成「Steady Timing Analysis」，正確是 **Static** Timing Analysis。

- 【課本補充】課本 §1.1.1（p.2–4）講 EDA 的歷史：1960 年代初 IC 出現後，logic design 和 physical design 都是手工；1964 年第一屆 DAC；1970 年代中期才有 SPICE 電路模擬、layout verification、GDSII 格式；之後有 logic simulator、自動 place & route（P&R）。

---

## 5. 車用電子也需要 EDA

[11:33]–[14:27]　投影片 10 Top Automotive Industry Trends、From 宣明智（10/26/2021）

- 投影片引用**宣明智**（10/26/2021，UMC 投影片）：**電動車要用多少種 IC？** 油車 40 種以上；油電／電動車／自駕車 150 種以上。「有 IC 產業國家已不多，台灣有最具競爭力的 IC Design + Foundry。」
  - ⚠️ 逐字稿寫成「薛明志」，投影片寫「宣明智」，以投影片為準。【口頭】老師說他是聯電的 founder。
- 投影片結論：
  - 車用電子市場很大
  - 很多 **custom design** 元件
  - **不一定要用最先進的 3 nm 製程**
  - 需要大量 **simulation**、客製化的 **PDK**（process design kit）
  - → EDA also plays a critical role!
- 【口頭】自駕需要很多 image sensor；幾年前 IC 短缺時車子做不出來。電動車電壓高，有 high power 的 IC，PDK 要客製化。想想 Tesla 這種公司需要多少新設計，這些都需要 EDA 軟體、也需要 EDA 軟體不斷改進。

---

## 6. EDA 產業有多大

[14:27]–[16:55]　投影片 EDA Industry: Semiconductor + Software

- EDA = 半導體產業 × 軟體產業的**交集**：它幫半導體做設計，而它本身是寫程式。
- 投影片表格（約 2017 年）：

| 公司 | 員工 | 年營收 | 市值 | 主要據點（矽谷以外） |
|---|---|---|---|---|
| Synopsys | 15,000 | $3.7B | $52B | India, China, Europe, Armenia, Taiwan |
| Cadence | 10,000 | $2.7B | $50B | India, China, Europe, Taiwan |
| Siemens EDA（Mentor） | ~6,000 | ~$1.6B | ~$20B | Oregon, India |
| Other EDA | ~20,000 | ~$2B | $10B | Worldwide |
| **合計** | **50,000** | **$10B** | **$132B** | Worldwide |

- 兩張圓餅圖（2020）：
  - 半導體產業營收（總計 $510B）：**EDA = 2.5% of semiconductors**
  - 軟體產業營收（總計 $507B，Microsoft、IBM、Oracle、SAP…）：**EDA = 2.4% of software**
- 2024：半導體總營收約 **$650B**，EDA 約 **$16B**。【口頭】比例還是差不多 2.4–2.5%。

---

## 7. David Pan 的 EDA 介紹

[16:55]–[23:06]　投影片 Semiconductor Design 101 Course

【口頭】老師分享 UT Austin **David Pan** 教授在一門「Semiconductor Design 101」線上課程裡的三張投影片：

**What is EDA?**
- Electronic Design Automation 的縮寫。
- 由軟體、硬體、IP、服務組成的**關鍵市場**，用在半導體元件（晶片）的定義、規劃、設計、實作、驗證與製造。
- "EDA is at the **heart** of the **heart** of modern information technology" —— Aart de Geus（Synopsys 共同創辦人）。晶片是 IT 的心臟，EDA 是晶片的心臟。
- **No EDA → No chip** or semiconductor industry as we know of today.
- 2024：EDA $16B；半導體 $628B；全球 IT $11.7T；全球 GDP $110T。
  - ⚠️ 這裡半導體寫 $628B，前一張投影片寫 $650B。【口頭】老師說看到不同的數字，差不多。

**Chip Design / EDA is not for Fainted Heart**
- 現代晶片是 **SoC**（system-on-chip）：digital（CPU、GPU、memory、control logic）＋ analog（RF、sensor、mixed-signal）。
- 複雜度驚人：**數十億顆電晶體、數百公里的導線，塞在指甲大小裡**。
- **Tapeout 很貴**（5 nm 約 $50M）→ verify, verify!
- 專門的晶圓廠（TSMC、GlobalFoundries）；fabless 公司（Nvidia、AMD、Qualcomm、Broadcom）。
- 雙方透過 **PDK**（process design kit）溝通——也很複雜。【口頭】每個製程都有一套 PDK，DA2（老師另一門課）會稍微講。
- 應付這種複雜度 → **divide and conquer**。【口頭】把題目切開，每一塊再想辦法解。

**EDA Ecosystem**
- 從 spec 到 tapeout 的軟體、工具、方法（用不同抽象層來處理複雜度）：design tools、simulation tools、verification tools。
- 範圍：digital、analog、mixed-signal、RF、packaging、PCB。
- DFM（design for manufacturability）、reliability、security…
- **IP reuse**——不要重新發明輪子。
- EDA 高度跨領域（EE、CS、數學、物理、AI），所以 PhD 永遠不會「overqualified」。
- 會 EDA 就能做任何工作（軟體工程師、硬體工程師、晶片設計、AI、華爾街）。
  - 【口頭】老師說真的有人因為熟演算法和機率，後來跑去金融市場賺了很多錢。

---

## 8. EDA = 硬體和軟體的交會

[23:06]–[25:42]　投影片 Introduction Of EDA

```
   Electrical Engineering          Computer Science
   ─────────────────────          ─────────────────
   Circuit theory                  Data structure, Algorithms  ← 標成重點
   VLSI design                     Programming language
   Microelectronics                Computational software
   DSP/Communications              Database
   ElectroMagnetic/RF              Numerical analysis
                    ↘  Linear Algebra, Statistics  ↙
```

- EDA = where HW and SW meet each other → **Productivity ↑**。
- EDA 關心 HW/SW 設計的：
  1. **Correctness**（正確）
  2. **Productivity**（生產力）
  3. **Optimality**（最佳化：速度快、面積小）
  4. **Scalability**（可擴展）
- 【口頭】
  - 老師剛到 Berkeley 時，指導教授叫他先修 data structure 和 algorithm 兩門課。Programming 是自己學的，**多讀別人寫得好的 code**。
  - SPICE simulation 要解矩陣，所以 numerical analysis 很重要；大一大二的線性代數、統計也很重要。
  - **Scalability**：程式只能處理小 case 不行；設計變大以後跑很久都跑不完也不行。這就是**為什麼寫 EDA 程式要懂 complexity**（[第 3 支](lecture03_note.md)）。
- 投影片：Cadence CEO **Anirudh Devgan** 說 EDA 是 **CS + Math（+ EE）**。【口頭】老師認為要加 EE，David Pan 也這麼說。

---

## 9. 為什麼做 EDA 工具很刺激

[25:42]–[29:12]　投影片 Why EDA Tool Development Is Exciting

- 很多問題要解、要改進：
  - 工具效能要突飛猛進（leaps and bounds）
  - 有競爭對手是好事——不能停下來
- 跨領域：programming、IC 設計與製造、數學、compute environment、統計、物理。
  - 【口頭】compute environment 的例子：OPC（optical proximity correction）可能要上百台電腦一起算，要知道怎麼把 job 送出去、monitor 做完沒、沒做完要不要轉送到別台。
- 很刺激；**客戶的問題常常讓你意外**。
  - 【口頭】你覺得自己寫得很好，到客戶那邊還是出問題，這時要想怎麼 debug。
- **Customers pay you and say "Thank you", if you indeed solve their problems.**
- **新趨勢常常要求現有方法重新思考、重新實作。**
  - 【口頭】例：現在要用 GPU 縮短 runtime，整個程式就要改寫。
- I guess pay is not bad ☺

---

## 10. 職涯路徑與職缺要求

[29:12]–[32:33]　投影片 Career Growth Path、Synopsys／Nvidia 職缺

```
                   ┌→ Technical Fellow      （走 technical track 一直深入）
Software Engineer ─┼→ Product Specialist    （用工具和知識幫客戶解決問題）
                   ├→ Management            （帶人、帶產品）
                   └→ Marketing / Sales     （把新功能的 key message 傳出去）
```

**Synopsys Staff R&D Engineer**（2024-11-26 張貼，Sunnyvale，年薪 $138,000–$207,000）要求：
- 精通 **C++**
- 懂 **static timing analysis、digital design／VLSI flow**；懂 synthesis／P&R 最佳化更好
- 懂 **AI 和 machine learning**（【口頭】現在實在沒辦法避免）
- 懂電子與電路設計
- 很好的**溝通**能力
- 團隊目標：Fusion Compiler、ICC Compiler II 的 quality of results，同時 **runtime 要短、memory 要省**。
  - 【口頭】memory 用太多，整台機器不夠用就會 swap，runtime 更長。

**Nvidia**（technews.tw 2024/06/30 的文章）：
- 必備扎實技術基礎：**data structures** 與**演算法**；面試前幾輪寫 code 的技術測驗占很大比例。
- 用 STAR 原則（Situation、Task、Action、Result）描述工作經歷。
- 了解公司文化、展現特色但避免過度自信、接受失敗並持續成長。

---

## 11. 這門課在整個 EDA 的位置

[32:33]–[33:27]　投影片 EDA for Digital SoC

| 學期 | 範圍 | 驗證 | 實作 |
|---|---|---|---|
| **Spring 2026（本課 DA1）** | **EDA for Digital SoC** | pre-layout、post-layout | **Physical Design** ← 本課重點 |
| Fall 2026（DA2） | EDA for Custom/Analog Designs | DRC、RC、IR-drop、SPICE | Physical Design |
| Fall 2026（DA2） | Packaging/3DIC | Multiphysics analysis | Physical implementation |

- 【口頭】這學期教 digital 的 physical design：layout 是怎麼做出來的、主要演算法。秋季的 DA2 教 analog custom design 的 layout、DRC、RC、IR-drop、SPICE，還有 3D 的 advanced packaging。

---

## 12. 課程大綱

[33:27]–[35:39]　投影片 Course Info (2)

| 主題 | 週數 | 對應講次 | 課本 |
|---|---|---|---|
| EDA 介紹；technology node；基本 CMOS 設計 | 1 | [第 1](lecture01_note.md)–[2 支](lecture02_note.md) | Ch1、Ch2 |
| 演算法、graph 與 complexity | 1 | 第 3–5 支 | Ch4 |
| Physical design：compaction、floorplan、placement、routing（Gordian placer 為例） | 6 | 第 7–15 支 | Ch10–Ch13 |
| Logic synthesis、optimization、technology mapping | 2 | 第 6 支 | Ch6 |
| 期中小考（日期待定） | — | — | — |
| Timing、delay calculation、clock tree synthesis、simulation | 2 | 第 14–15 支 | Ch13 |
| 程式 demo／presentation | 1 | — | — |
| Design rules、nanometer design（有時間的話） | — | — | — |

- 【口頭】
  - technology node 會從以前的 planar 講到 FinFET、GAA。
  - 進到 algorithm 自然會講 graph、在 graph 上 search，分析 complexity——設計大時希望 complexity 控制得住，不然 runtime 太長。
  - placement 會用經典的 placement 程式來講背後的數學與最佳化技巧。
  - technology mapping 會用 **dynamic programming**。
  - 最後一週請同學 present 自己寫的 **2D global router** 並 demo。
- ⚠️ 播放清單的實際順序和大綱不同：synthesis（第 6 支，影片標 ch10）排在 compaction（第 7 支）前面。

---

## 13. 評分、作業與程式

[35:39]–[42:03]　投影片 Course Info (3)、Homework, Programs, Presentation, Exam

| 項目 | 比例 | 說明 |
|---|---|---|
| Homework | 18% | 3 份，每份 6%；可以討論，但要自己寫 |
| Programs & demo／presentation | 60% | 3 個程式，每個 20%；**最多 2 人一組**；尤其是 global router |
| 期中小考 | 12% | open note——可帶**一張紙**（雙面）；可用平板看上課教材 |
| 課堂參與 | 10% | 問問題、demo、幫忙 |

- 最後成績會**依 curve 調整**。
- 【口頭】
  - 期中考**不能上網 Google，也不能問 AI**。
  - 單單出席不代表參與，希望大家來、有問題就問；點名大概只會有一次（對名字和臉）。
  - 3 人一組有點多，真的沒辦法再來談；一個人做也可以。

**作業與程式**（需要對部分步驟寫報告）：

| 作業（18%） | 程式（60%） |
|---|---|
| Hw-1：logic equation 與 gate、search 演算法 | Prog-1（20%）：把 **CUDD** bring up，跑 Boolean 例子（equivalence checking、technology mapping） |
| Hw-2：truth table、**BDD** | Prog-2（20%）：把 quadratic placer（**Gordian**、**RePlAce**）bring up（GitHub 上，可能要用 Visual Studio；歡迎 build **OpenROAD**） |
| Hw-3：floorplan representation、**B\*-tree** packing、partitioning | Prog-3（20%）：寫一個 **2D global router** 並 demo |

- 【口頭】
  - CUDD 是 University of Colorado 實作的 BDD 套件，很經典，常用在 **equivalence checking**（兩個 Boolean equation 是否相等）；講 technology mapping 時也會用到。
  - Gordian 的程式是在 Windows 上跑的，希望同學能在 Linux workstation 上跑起來。
  - **OpenROAD**：UC San Diego 的教授發起，目標是從 behavior description 一路到 tapeout 的開源 EDA 流程。**RePlAce** 是 UCSD 鄭仲寬（Chung-Kuan Cheng）教授的組發展的，也是 OpenROAD 的一部分。（逐字稿把名字轉成「陳仲冠」；[第 10 支](lecture10_note.md) §12 的投影片列出 RePlAce 論文作者 Chung-Kuan Cheng、Andrew B. Kahng、Ilgweon Kang、Lutong Wang，可以確認是 Cheng 教授。）
- ⚠️ 逐字稿最後一行「字幕志願者 楊茜茜」是 whisper 常見的幻覺（影片結尾的片頭動畫沒有人聲），不是影片內容。

---

## 14. 一頁速查表

| 主題 | 重點 |
|---|---|
| EDA | Electronic Design Automation；晶片的設計、驗證、測試自動化 |
| 為什麼需要 | A10 32.8 億 → A12 70 億 → A15 150 億顆電晶體；沒有工具做不出來 |
| EDA 要解的問題 | synthesis、place、route、DRC/LVS、RC extraction、timing（STA）、database／GUI、ECO |
| 車用 | 油車 40+ 種 IC、電動車 150+ 種；custom design、不一定最先進製程、大量 simulation、客製 PDK |
| 產業規模 | 2017 約 $10B；2024 約 $16B；≈ 半導體的 2.5%、≈ 軟體的 2.4% |
| 三大廠 | Synopsys、Cadence、Siemens EDA（Mentor） |
| SoC | digital（CPU/GPU/memory/control）＋ analog（RF/sensor/mixed-signal） |
| Tapeout | 5 nm 約 $50M → verify, verify |
| PDK | fab 與 fabless 之間的介面 |
| 應付複雜度 | divide and conquer |
| EDA 的四個目標 | correctness、productivity、optimality、scalability |
| EDA = | CS + Math（+ EE）；核心是 data structure 與 algorithm |
| 本課範圍 | digital SoC 的 physical design ＋ logic synthesis |
| 評分 | 作業 18%、程式 60%（CUDD、Gordian/RePlAce、2D global router）、期中 12%、參與 10% |

---

## 15. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| technology node、CMOS 基本設計 | [第 2 支](lecture02_note.md)：ch1 intro、transistor design |
| NP-complete、complexity、scalability | 第 3 支：ch2 complexity |
| Steiner tree（routing 的基礎） | 第 5 支：Steiner tree |
| logic synthesis、BDD、technology mapping（Prog-1、Hw-2） | 第 6 支：ch10 synthesis |
| partitioning、floorplan、B\*-tree（Hw-3） | 第 8–9 支：partitioning、floorplanning |
| Gordian／RePlAce placer（Prog-2） | 第 10 支：placement |
| 2D global router（Prog-3） | 第 11–12 支：routing |
| clock tree synthesis、P/G | 第 14–15 支：channel routing、clock、P/G |

---

## 附：本講一句話總結

> **這門課是清大李炯霆老師的 Design Automation (1)，教材源自二十多年前教育部 EDA 聯盟各校教授合編的投影片，再加上 Stanford、UT Austin、Berkeley、Georgia Tech 與 EDA 廠商的新內容；EDA 存在的理由是晶片太大（A15 已有 150 億顆電晶體），synthesis、placement、routing、DRC/LVS、RC extraction、STA、ECO 沒有工具根本做不出來，連車用電子（電動車要 150 種以上 IC）也離不開它；EDA 產業約 $16B，約占半導體營收 2.5%、軟體營收 2.4%，由 Synopsys、Cadence、Siemens EDA 主導；它是 EE 和 CS（加數學）的交會，要求正確、生產力、最佳化與可擴展，所以寫 EDA 程式一定要懂 data structure、algorithm 與 complexity；本學期專注 digital SoC 的 physical design（compaction、floorplan、placement、routing）與 logic synthesis，評分以三個程式（CUDD、Gordian/RePlAce、2D global router）占 60% 為主。**

---

*筆記依據李炯霆「Electronic Design Automation (1)」Lecture 1 | Ch1. Introduction 錄影（NTHU OCW，42:06；影片無字幕，以 whisper large-v3-turbo 轉錄）、投影片截圖、Wang, Chang, Cheng（eds.）*Electronic Design Automation: Synthesis, Verification, and Test* Ch1 §1.1（p.1–12）整理。*
