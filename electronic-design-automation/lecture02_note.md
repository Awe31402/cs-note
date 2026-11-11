# 設計自動化（一）第二支影片完整筆記：VLSI 設計流程、design style 與 CMOS 電晶體設計（ch1 intro + transistor design）

> **課程**：Electronic Design Automation (1)（清大 CSR 5302）— 第2講｜ch1.intro_xtor_design（播放清單第 2 支）
> **講者**：李炯霆（JT Li），清大半導體學院，2026 春，NTHU OCW 錄影
> **來源**：YouTube 錄影（2:11:31，無字幕，whisper 轉錄）＋ 投影片截圖（投影片 Unit 1，p.1–112）＋ 課本 Wang, Chang, Cheng（eds.）*Electronic Design Automation* Ch1 §1.1.2（p.4–8）、Ch2 §2.2–2.3、§2.5（p.40–57、71–76）
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、投影片和課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有；[1:11:06] 的 recap 提到「有同學問了 emulation」，但提問本身不在錄影裡）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿辨識不清、或影片／投影片／課本說法不一致之處
> - `[h:mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. IC 產業里程碑與 Apple A12 的例子](#1-ic-產業里程碑與-apple-a12-的例子)
- [2. 一顆晶片背後要回答的問題](#2-一顆晶片背後要回答的問題)
- [3. Moore's law 與更多里程碑](#3-moores-law-與更多里程碑)
- [4. 課程主題順序](#4-課程主題順序)
- [5. IC 設計流程：silicon compiler 與 VLSI design cycle](#5-ic-設計流程silicon-compiler-與-vlsi-design-cycle)
- [6. Digital SoC 設計流程的主要元件與 Y chart](#6-digital-soc-設計流程的主要元件與-y-chart)
- [7. 三種 design view 與各層的設計工具](#7-三種-design-view-與各層的設計工具)
- [8. Design metrics：PPA 與 interconnect delay](#8-design-metricsppa-與-interconnect-delay)
- [9. EDA 在 digital SoC 要顧的事、Broadcom 的要求](#9-eda-在-digital-soc-要顧的事broadcom-的要求)
- [10. Custom／analog 設計流程與 OPC](#10-customanalog-設計流程與-opc)
- [11. 製程：planar → FinFET → GAA → CFET、EUV](#11-製程planar--finfet--gaa--cfeteuv)
- [12. 小結：EDA 與 AI/ML](#12-小結eda-與-aiml)
- [13. 規格之後的五種 design action](#13-規格之後的五種-design-action)
- [14. HDL synthesis = domain translation + optimization](#14-hdl-synthesis--domain-translation--optimization)
- [15. Technology mapping 與 standard cell library](#15-technology-mapping-與-standard-cell-library)
- [16. Transistor level 與 physical design 的工具](#16-transistor-level-與-physical-design-的工具)
- [17. 用 SPICE 模擬 CMOS inverter](#17-用-spice-模擬-cmos-inverter)
- [18. Physical design：從電路到幾何](#18-physical-design從電路到幾何)
- [19. 現代的 inverter layout：fingers、folding、Pcell](#19-現代的-inverter-layoutfingersfoldingpcell)
- [20. Physical design flow 與實例](#20-physical-design-flow-與實例)
- [21. Technology roadmap 與 nanometer 設計挑戰](#21-technology-roadmap-與-nanometer-設計挑戰)
- [22. Power、製造、生產力的挑戰](#22-power製造生產力的挑戰)
- [23. Recap（第二次上課開頭）](#23-recap第二次上課開頭)
- [24. Design styles](#24-design-styles)
- [25. MOS 電晶體與 CMOS](#25-mos-電晶體與-cmos)
- [26. CMOS inverter 與 CMOS 的性質](#26-cmos-inverter-與-cmos-的性質)
- [27. NAND、NOR 與基本 CMOS logic library](#27-nandnor-與基本-cmos-logic-library)
- [28. Compound gate 的建法](#28-compound-gate-的建法)
- [29. Stick diagram](#29-stick-diagram)
- [30. FinFET 與 planar／FinFET layout 的差別](#30-finfet-與-planarfinfet-layout-的差別)
- [31. Design rules：micron rules、λ rules、MOSIS](#31-design-rulesmicron-rulesλ-rulesmosis)
- [32. 【實測】compound gate 的 switch-level 檢查](#32-實測compound-gate-的-switch-level-檢查)
- [33. 一頁速查表](#33-一頁速查表)
- [34. 與後續講次的連結](#34-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

這支影片其實是**兩次上課**（[1:11:06] 開始是第二次的 recap），投影片是「Unit 1: Intro. to Electronic Design Automation」。

```
上半（[0:00]–[1:11]）：EDA 在整個 VLSI 流程的位置
  IC 里程碑 → A12 晶片背後的問題 → silicon compiler 流程 → Y chart、三種 view
  → PPA、interconnect delay → custom/analog 流程、OPC → FinFET/GAA/CFET
  → synthesis = domain translation + optimization → technology mapping
  → SPICE 模擬 inverter → physical design 把電路變幾何 → 各種挑戰

下半（[1:11]–[2:11]）：design style 與 CMOS 電晶體設計
  full custom / standard cell / gate array / FPGA / SPLD 比較
  → MOS、CMOS 的性質（P = ½CV²f）→ NAND、NOR、logic library
  → compound gate：AND ↔ 串聯、OR ↔ 並聯，n 網路和 p 網路互為 dual
  → stick diagram → FinFET（W_eff 量子化）→ design rules（λ rules、MOSIS）
```

---

## 1. IC 產業里程碑與 Apple A12 的例子

[0:00]–[4:12]　投影片 1–3、A12；課本 §1.1.1（p.2–4）

- 本單元內容：VLSI 設計流程／方法／style、VLSI 設計自動化工具、半導體技術 roadmap、CMOS 技術。閱讀：參考書 Chapters 1–2。
- 封面圖：system specification → chip，可以 **manual**，也可以 **automation**。【口頭】我們當然希望 automation 能進來。

**里程碑**（投影片）：

| 年 | 事件 |
|---|---|
| 1947 | Bardeen、Brattain、Shockley 發明電晶體（IC 產業的基礎） |
| 1952 | Sony 推出第一台電晶體收音機 |
| 1958 | Kilby 發明 IC |
| 1965 | Moore's law |
| 1968 | Noyce 與 Moore 創立 Intel |
| 1970 | Intel 推出 1K DRAM |
| 1971 | Intel 4004 微處理器（2,250 顆電晶體） |
| 1976/81 | Apple II／IBM PC |
| 1985 | Intel 轉向專注微處理器 |
| 1987 | **TSMC** 成立（使 **fabless** IC 設計成為可能） |
| 1991 | ARM 推出第一個可嵌入的 RISC IP core（**chipless** IC 設計） |

- 【口頭】Intel 剛成立時做 DRAM，1985 年退出 DRAM 生意專做微處理器；從 PC 開始 Windows 和 Intel 就綁在一起。

**Apple A12（iPhone Xs，2018）**（取自 Stanford EE292A Lecture 1）：

| 項目 | 數值 |
|---|---|
| 製程 | 7 nm TSMC FinFET |
| 面積 | 9.89 × 8.42 = 83.27 mm² |
| 電晶體 | 69 億 |
| GPU | 4 核（~18%），9 個 block |
| CPU | 6 核（~14%），13 個 block（4 個 Tempest 低功耗、2 個 Vortex 高效能、L2/L3 cache） |
| NPU/TPU | 8 核（~7%），4 個 block |
| DDR | ~3%，1 個 block |
| 其他 | ~57%，50 個 unique block |
| 合計 | **~75 個 unique block** |

- 投影片問：**Apple 的 IP 都是自己設計的嗎？**
  - 【口頭】CPU core 看起來像是 license ARM 的東西。如果 IP 是外面買的，怎麼相信一家 IP design house 做的東西能和其他元件搭在一起？這是 IC 設計上很重要的問題。
  - 【口頭】圖上兩個高效能 CPU 看起來一樣，但其中一個是**翻轉（flip／mirror）**過的。

---

## 2. 一顆晶片背後要回答的問題

[4:12]–[8:11]　投影片 Questions Are（design flow／methodology）

從 A12 這張圖要問的問題（每一個都需要 EDA）：

1. **整顆晶片怎麼 partition？** functional level 怎麼驗證？
   - 【口頭】先用 HDL 描述每個 block 的功能、做 simulation 驗證，再走到 gate level。
2. **要不要用 emulation 驗證功能？**
   - 【口頭】emulation 比 software simulation 快很多，可以驗功能，現在也能做 timing。
3. **用哪個 technology node？** 新 node 還是現有 node？foundry ready 了嗎？
   - 【口頭】太新怕做不出來、良率（yield）低；太保守怕 performance 達不到。晶片太大良率不高又貴。chip architect 要衡量 pros and cons。
4. **floorplan 怎麼決定？** HP CPU1、CPU2 好像是 mirror 的？要試幾種 floorplan？
   - 【口頭】擺在那裡是因為要和鄰居有很強的連接。
5. **有沒有 routing channel？** 還是 block 直接相接（abut）？每個 block 怎麼做 **pin assignment**？線可以穿過 block 嗎？
6. **DRC、LVS、RC extraction 怎麼做？** 整顆做還是 block level？
   - block 合在一起時跑出來的 **density** 問題怎麼解？【口頭】每個 block 都滿足 density constraint，合起來卻不滿足，這時誰來修？
7. **整顆晶片怎麼驗證？用 STA？** 要修的話，**ECO** 怎麼處理？重新 route？重新 place？
   - 【口頭】在原本的 block 裡重新 place & route，線會不會拉不出來？
8. 晶片怎麼測試？要不要考慮發熱、老化（aging）？

【口頭】這只是老師想到的，還有很多細節。

---

## 3. Moore's law 與更多里程碑

[8:11]–[9:48]　投影片 Moore's Law、Milestones (Cont'd)、Apple M2

- **Moore's law：電晶體數每 2 年加倍**（圖：1970–2020 的電晶體數，取自 Stanford EE292A 2022）。
- TSMC 前董事長劉德音：5G、AI、IoT 的興起正在為半導體產業鋪出「golden era」。
- 續里程碑：
  - 1996 Samsung 1G DRAM；1998 IBM 1 GHz 實驗性微處理器；1999 年前後出現 **SoC** 方法。
  - Intel P4 有 4,200 萬顆電晶體（2005 年到 10 億）。
  - 2021 Apple／TSMC：A15 有 150 億顆電晶體（5 nm）。
- **Apple M2**（TSMC 第二代 5 nm）：**200 億**顆電晶體、8 核 CPU、10 核 GPU、16 核 neural engine（每秒 15.8 兆次運算）、最多 24 GB LPDDR5、100 GB/s 記憶體頻寬。
  - ⚠️ 逐字稿聽成「5.8 的 trillion transistor」，投影片是 **20 billion transistors**、15.8 trillion operations per second。
- 【口頭】memory 有自己的一章，現在最紅的是 **HBM**。

---

## 4. 課程主題順序

[9:48]–[10:24]　投影片 Course Ordering Of The Major Topics

1. Digital design flow
2. Basic logic synthesis、technology mapping
3. CMOS logic gates 與 Boolean equations
4. Algorithms 與 complexity
5. Synthesis、technology mapping
6. Compaction
7. Partitioning
8. Floorplanning
9. Placement
10. Routing
11. ~~High level synthesis~~（灰色，這學期不教）

前三項（highlight）就是這一講的內容。

---

## 5. IC 設計流程：silicon compiler 與 VLSI design cycle

[10:24]–[14:42]　投影片 IC Design Flow – Silicon Compiler、Typical VLSI Design Cycle；課本 §1.1.2（p.4–8）、Fig. 1.3–1.4

**IC Design Flow – Silicon Compiler**：

```
System Specification
  → Architectural Design
  → Functional & Logic Design            ┐
  → Logic Synthesis                      │ fabless 公司
  → Physical Design ──┐                  │ ＋ EDA 工具
  → Mask Synthesis & Verification（DRC、LVS、STA）┘
  → Fabrication
  → Packaging and Testing

Physical Design 的迴圈：
  Floorplanning → Placement → Clock Tree Synthesis → Signal Routing
  → Timing Closure → DFM Closure →（回到 floorplanning）
```

- 【口頭】
  - architectural design 決定有哪些 function block；每個 block 做 logic synthesis，map 到 gate；然後是 physical design。
  - **clock tree synthesis**：一個 clock 訊號要 drive 很多 flip-flop，直接接是不行的，一定要長一棵 tree。用什麼 clock buffer、幾層。
  - signal routing 也包含 power/ground routing；之後看 timing 和 constraint 有沒有滿足；layout 上要考慮 **DFM** 提高良率。
  - mask 要驗 DRC、LVS；找出 **hot spot**（危險的地方）稍微修一下提高良率，然後進 manufacturing、packaging、testing。

**Typical VLSI Design Cycle**（三個層級）：

| 層級 | 步驟 | 往下傳的東西 | 回頭驗證 |
|---|---|---|---|
| System level | System specification → Function/architecture design | specification → behavioral representation | functional simulation |
| Logic level | Logic synthesis（例 Z = a + bc + de 的 gate 圖） | structural representation | logic simulation |
| Building block | Circuit design（電晶體圖） | structural representation | circuit analysis |
| Physical layout level | Physical synthesis | physical representation | extraction & verification |
| Manufacturing | Fabrication → Packaging | | |

- 【口頭】Z 的電路裡，pull-down 的 nMOS 是**串聯（in series）**，對應的 pMOS 是**並聯（in parallel）**——下半場會細講（[§28](#28-compound-gate-的建法)）。
- 【口頭】building block 做好以後可以變成一個大 cell 給 designer 用；有了 building block 下一步就是怎麼擺（floorplanning、I/O 在哪）、怎麼連，擺和拉線都要符合 design rule，最後寫出 mask 給 manufacturing。
- 【課本補充】課本 Fig. 1.4（p.7）把 physical design flow 簡化成 floorplanning → placement → routing → extraction & verification；Fig. 1.5（p.8）是另一種 EDA flow：modeling → design verification → logic synthesis → physical design → fabrication → manufacturing test，每一步之間做 regression test。

---

## 6. Digital SoC 設計流程的主要元件與 Y chart

[14:42]–[16:51]　投影片 Major Components In SoC Design Flows

**Digital SoC 流程的主要元件**：
1. **System level design**：simulation（behavioral、function level）。
2. **Logic synthesis**，然後 **physical synthesis with constraints**（可能要 gate-level simulation 或 formal verification）。
   - 【口頭】現在做 synthesis 也要考慮 physical information，這就是 physical synthesis。
3. **Layout**：floorplan、**place-and-route**、timing optimization。
   - formal verification 確認 layout 階段的 logic 修改是對的。【口頭】為了 timing 改了 design，要確定改完和原本一樣。
4. **Verification**：
   - R/C parasitic **extraction with corners**（【口頭】fast-fast、slow-slow 等 corner）
   - **static timing analysis**
   - layout verification（DRC、LVS、PERC…）
5. **Power network analysis 與 IR-drop**，含 electro-migration。

**Gajski and Kuhn's Y Chart**：三條軸 Behavioral、Structural、Physical/Geometry，同心圓由外到內是 Architectural → Algorithmic → Functional Block → Logic → Circuit：

| 層 | Behavioral | Structural | Physical |
|---|---|---|---|
| 外 | Systems | Processor | Physical partitions |
| | Algorithms | Hardware modules | Clusters |
| | Register transfer | ALUs, Registers | Floor plans |
| | Logic | Gates, FFs | Cell, module plans |
| 內 | Transfer functions | Transistors | Rectangles |

- 【口頭】ECO change 很重要，要能很快 handle，不然又要等。

---

## 7. 三種 design view 與各層的設計工具

[16:51]–[20:18]　投影片 Three Design Views、Design Issues and Tools

**Three Design Views**（4-bit adder）：

```verilog
module add4 (s, c4, ci, a, b);
  input [3:0] a, b;
  input ci;
  output [3:0] s;
  output c4;
  wire [2:0] co;
  add f0 (co[0], s[0], a[0], b[0], ci);
  add f1 (co[1], s[1], a[1], b[1], co[0]);
  add f2 (co[2], s[2], a[2], b[2], co[1]);
  add f3 (c4,    s[3], a[3], b[3], co[2]);
endmodule
```

- **Behavior**：上面的 HDL。
- **Structural**：四個 `add` 串起來，carry 從 ci 經 co[0..2] 到 c4。
- **Physical**：f3 f2 f1 f0 排成一列，座標 (0,0) 到 (400,400)。
  - 【口頭】它是用 **abutment**（直接相接）的方式，訊號從左右進來。

**Design Issues and Tools**：

| 層級 | 工作 |
|---|---|
| System-level | 硬體／軟體 partition、co-design、co-simulation；成本估計、design-space exploration |
| Algorithmic-level | behavioral description（Verilog、VHDL）；high-level simulation、waveform viewing |
| Algorithm → hardware module | high-level（architectural）synthesis |
| Logic design | schematic entry；RTL 與 logic synthesis；gate-level simulation（功能、power）與 waveform viewing；timing analysis；formal verification |

- 【口頭】system level 要決定哪些用硬體、哪些用軟體做，也要分析成本。synthesis 的入口可以是 HDL，也可以直接畫 schematic。high level 就要估一下 power，不能太熱；waveform viewing 幫你很快 debug。

---

## 8. Design metrics：PPA 與 interconnect delay

[20:18]–[22:03]　投影片 Design Metrics

- **Design metrics：PPA**（speed、power dissipation、area），以及 noise／crosstalk、design time、testability 等。
  - 【口頭】crosstalk 太嚴重會讓功能出錯，也和 timing 有關。
- **Design evolution：在 deep submicron 時代，interconnect（不是 gate）的 delay 主導電路效能。**
  - interconnect 是在 physical design 決定的
  - 要在早期設計階段就考慮 interconnect（【口頭】這就是為什麼後來叫 **physical synthesis**）
  - post-layout simulation 的結果常常和 pre-layout 不一樣（【口頭】因為有了 RC 資訊；但還是希望能 meet spec）
- testing、formal verification 等其他工作這裡不談。

---

## 9. EDA 在 digital SoC 要顧的事、Broadcom 的要求

[22:03]–[25:15]　投影片泡泡圖、The Next Incarnation Of EDA（semiengineering.com，2022-08）

**EDA plays a critical role in digital SoC**（泡泡圖）：

| 面向 | 細項 |
|---|---|
| Timing | setup、hold、WNS、TNS、NVP；scenarios（modes、corners） |
| Power | dynamic、leakage、glitch |
| Area | wirelength；standard cell（clock buffers、comb cells、seq cells）；macros |
| Clock | skew、insertion delay、latency |
| IR-Drop | Vdrop |
| Compute | memory、disk、TTR（turnaround time）、throughput |

- 【口頭】leakage 太嚴重會限制電池壽命；compute 是指 IT 提供的機器有沒有足夠的 memory 和 disk 存資料。

**Broadcom 對 EDA 的要求**（Fig. 1: Design complexity and EDA）：
- 設計規模：~800 mm² core die、50B+ 電晶體、4Gb+ on-die SRAM、16 個 STA corner、1000+ TB storage、1M+ machine hours。
- Layout DRC 規則數：16 nm 約 6,000 條 → 3 nm 15,000+ 條。
- Signoff 時間要求：physical verification ~3 天+、extraction ~2 天+、STA ~1 週+、power ~1 週+。
- EDA 要能支援 hierarchical、split signoff，並最佳化 storage 和 server 需求。
- 【口頭】**EDA solutions must scale with design complexity**：設計越大，演算法要跟著 scale，最好是 linear proportional，不要變成 exponential。

**State-of-the-Art Chips**：A12 的 floorplan；"Google says the SoC is the new motherboard"（The Next Platform）。
- 【口頭】大 SoC 把很多元件放在一起；現在還有 **chiplet**、3DIC 的 heterogeneous integration，越來越重要。

---

## 10. Custom／analog 設計流程與 OPC

[25:15]–[28:30]　投影片 Major Components In Custom Design Flows (2)

**Custom/analog 設計**：
1. Design specification（schematic、AMS Verilog…）：netlisting 輸出、**simulation**（SPICE、co-sim）
   - 【口頭】Verilog 不只給 digital，還有給 analog mixed-signal 的 AMS Verilog；裡面有 digital block 的話要做 co-simulation。
2. Layout：**matching** 的 placement（constraint based）、**special routing**、in-design analysis
   - 【口頭】analog 很重要的是 **device matching**；routing 中就做 RC 分析，一步步控制 post-layout 和 pre-layout 的差距。
3. Layout verification：DRC、LVS、RC extraction
4. Post-layout simulation（RC 資料很大）
5. Power network analysis、IR-drop、electro-migration
6. ECO fixes（schematic、sim、layout、verification）
   - 【口頭】改要從 schematic 改，重新 simulate、改 layout、再 verify。

**Many software development talents are needed; PDK development also needs talents.**

**Manufacturing 也一樣**：**OPC（Optical Proximity Correction）**
- 用 lithography simulation 找 hot spot（用 CPU farm）
- 在 mask 上加額外的形狀
- 【口頭】這個 simulation 要很多時間，用幾百顆 CPU 在跑。

**Wafer & Chip**、**IC 製造流程**（工研院電子所 1993 年的圖）：【口頭】很老的圖，可以看出 IC 從邏輯設計、電腦佈局、資料送出、光罩、晶圓製造到封裝測試的流程。

---

## 11. 製程：planar → FinFET → GAA → CFET、EUV

[28:39]–[32:42]　投影片 Planar vs FinFET vs GAA、TSMC roadmap、Evolution of a CMOS Device Architecture、CFET、Lithography

- **Planar → FinFET → Gate-All-Around**（Lam Research 圖）
  - 【口頭】為了增加有效寬度（W），乾脆把 diffusion 立起來；FinFET 是 Intel 在 22 nm 首先做出來；TSMC 到 2 nm 開始用 GAA，gate 包住整個 channel。
- **TSMC 技術 roadmap**：N7（2018）→ N5、N5P、N6 → N4P、N4X、N6RF → N3、N3E、N3P、N3X（FinFET）→ **N2（nanosheet，2025）**。
- **Evolution of a CMOS Device Architecture**（Ryckaert et al., IEDM 2019；2020 VLSI Symposium）：

| 架構 | Node | Poly pitch／Metal pitch | Cell 高度 |
|---|---|---|---|
| FinFET | N3 | PP 44–48、MP 21–24 | 6T（2 fins） |
| Nanosheet | N2 | PP 40–44、MP 16–21 | 5T（1 nanosheet） |
| Forksheet | N1 | PP 36–40、MP 12–16 | 5T → 4.3T |
| CFET | — | PP 36–40 | 4T／3T |

  - **N/P 間距縮小 → 面積縮小**。
  - 【口頭】電晶體大概就走到這麼小了，再下去非常辛苦。
- **CFET Density Scaling ~1.5–2×**（ISSCC 2024）：**NMOS 疊在上面、PMOS 在下面**。
  - 【口頭】每個 cell 面積變小，但 **pin access**（pin 怎麼拉出去）變難；疊在一起也有製造上的挑戰。
- **EUV**（ASML）：組好的 EUV 系統約 **100,000 公斤**，**每台起價 $119M**；運送要用**六架飛機**。The Center of Gravity Matters!

---

## 12. 小結：EDA 與 AI/ML

[32:42]–[34:21]　投影片 Summary So Far

- EDA 工具有很多軟體元件 → **需要大量軟體人才**。
- 這些經驗可以應用、擴展到別的領域；跨領域研究很常見。
- **AI／ML 是大趨勢**，但目前**不是每個問題都能被 ML 改善**。
  - **ML-inside**：【口頭】把演算法的 kernel 換成 ML 的方法；但結果要比原本好才值得換。
  - **ML-outside**：【口頭】一個發展多年的程式有很多參數，用 ML 從外面 predict 這個設計該用什麼參數。據老師了解，**ML-outside 成功的例子比較多**。

---

## 13. 規格之後的五種 design action

[34:21]–[36:03]　投影片 Design Actions – After Specifications

| Action | 意思 | 例子 |
|---|---|---|
| **Synthesis** | 加入更多細節，增加設計的資訊 | logic synthesis、physical synthesis |
| **Analysis** | 收集設計品質的資訊 | timing analysis |
| **Verification** | 檢查 synthesis 步驟有沒有保持 spec 不變 | layout verification |
| **Optimization** | 在給定的描述裡重新安排，提升品質 | logic optimizer、timing optimizer |
| **Design Management** | 設計資料的儲存、工具之間的合作、design flow | database |

- 【口頭】design management 要存不同版本的結果，做 what-if analysis（這裡改一改結果會怎樣）；驗證時要很快開啟資料、zoom in 到那個地方——這些都需要有經驗的 CS 人來設計。

---

## 14. HDL synthesis = domain translation + optimization

[36:03]–[39:00]　投影片 HDL Synthesis（改編自 Ric Huang 的投影片）；課本 §1.2.3（p.17）、Fig. 1.8

```
--VHDL                        //Verilog
if (A='1') then               if (A==1)
  Y <= C + D;                   Y = C + D;
elsif (B='1') then            else if (B==1)
  Y <= C or D;                  Y = C | D;
else Y <= C;                  else Y = C;
endif
```

```
Behavioral domain ──RTL synthesis（domain translation）──> Structural domain ──optimization（area、timing、power…）──> gates
                                                         （adder、mux…）
```

- **Domain translation**：把 HDL（behavioral domain）換成 structure（adder、mux…）。【口頭】這一步比較 straightforward。
- **Optimization**：在 gate level 做最佳化，目標可以是 area、timing 或 power。
- 投影片註記：
  - 變數 A、B、C、D、Y 都**分配到 register**。
  - **需要 MUX**：【口頭】high-level translation 大部分都用 mux——select 訊號是什麼就選哪個值出來。
  - **Logic gate 可以有不同 size 來調 strength**（drive strength）：【口頭】用不同 size 的 gate 達到 timing，同時注意 area constraint。
- 【課本補充】課本 Fig. 1.8 的 logic synthesis flow：RTL to Boolean functions → technology-independent optimizations → technology mapping → technology-dependent optimizations → test logic insertion。

---

## 15. Technology mapping 與 standard cell library

[39:00]–[46:15]　投影片 Technology Mapping Problem、What If We Modify Verilog Statement(s)、SAED 32/28nm library

```vhdl
process
begin
    wait until rising_edge(CLOCK);
    if (SRST='0') then
        DOUT1 <= '0';
    else
        DOUT1 <= DIN1 nand DIN2;
    end if;
    DOUT2 <= SRST and (DIN1 nand DIN2);
end process;
```

- **logic synthesis**：直接翻譯成 NAND（DIN1、DIN2）＋ AND（和 SRST），結果存進兩個 flip-flop（FD1）。
  - 【口頭】SRST = 0 時 AND 輸出 0，存進 DOUT1；SRST = 1 時 NAND 的結果傳過去。DOUT2 直接拿 AND 的結果。
- **technology mapping**：在 library 裡找**功能相等、cost 比較低**的 cell 替換——投影片換成「SRST 經過 inverter，和 NAND 的結果一起進 NOR」的結構。
  - 投影片：**Need to find equ. cells from library.**
  - 【口頭】變成一種類似 **AOI**（AND-OR-Invert）的東西。
- **如果把 `SRST='0'` 改成 `SRST='1'`？** 怎麼把 statement map 成 combinational logic？
  - 直接翻譯：SRST 經 inverter 再和 NAND 的結果 AND——**No simplification**。
  - 另一種：**常用 mux 來 model if-else**——mux 的一個輸入接常數 '0'、另一個接 NAND 的結果，SRST 當 select。
  - 【口頭】第一步 translation 用 mux 的話，就是用 control signal（SRST）來控制它。

**Synopsys SAED_EDK32/28 32/28nm 標準 cell library 的 inverter**（VDD = 1.05 V、25°C、500 MHz、Csl = 4 fF）：

| Cell | Cload | Prop delay (ps) | Leakage (nW) | Dynamic (nW/MHz) | Area (µm²) |
|---|---|---|---|---|---|
| INVX0 | 0.5 × Csl | 38 | 24 | 0.80 | 1.27072 |
| INVX1 | 1 × Csl | 30 | 49 | 1.30 | 1.27072 |
| INVX2 | 2 × Csl | 29 | 100 | 2.32 | 1.524864 |
| INVX4 | 4 × Csl | 30 | 240 | 4.19 | 2.033152 |
| INVX8 | 8 × Csl | 28 | 549 | 15.80 | 3.049728 |
| INVX16 | 16 × Csl | 29 | 1220 | 15.40 | 5.08288 |
| INVX32 | 32 × Csl | 29 | 2580 | 30.20 | 9.149184 |

- 看法：每一級的負載和 drive strength 一起放大，所以 delay 幾乎不變（28–38 ps），但面積和 leakage 一路變大。
- 【口頭】任何 cell 都有這些資訊，要 pass 給 EDA 工具；synthesis 時根據 timing 挑哪一種 drive strength。

---

## 16. Transistor level 與 physical design 的工具

[46:15]–[48:39]　投影片 Design Issues and Tools (Cont'd)

| 層級 | 工作 |
|---|---|
| Transistor-level design | switch-level simulation（【口頭】只看 0/1 的開關，快）；circuit simulation（【口頭】比較仔細） |
| Physical (layout) design | partitioning；floorplanning & placement；routing；layout editing & compaction；design-rule checking；layout extraction |
| Design management | databases、frameworks |
| **Silicon compilation** | 從 algorithm 到 mask patterns——這個 idea 越來越接近，但離一個 push-button 操作還很遠 |

- 【口頭】**partitioning** 的概念：把設計切開，讓每一塊裡面連接很強、和外面連接少。
- 【口頭】silicon compilation 是 UCSD 的 OpenROAD 計畫要做的事。四十多年前老師念書時就有一家公司叫 Silicon Compiler，但太多細節、太多步驟，後來沒有成功；在學校可以當一個比較高的目標。

---

## 17. 用 SPICE 模擬 CMOS inverter

[48:39]–[50:48]　投影片 Circuit Simulation of a CMOS Inverter (0.6 µm)

```spice
M1 3 2 0 0 nch W=1.2u L=0.6u AS=2.16p PS=4.8u AD=2.16p PD=4.8u
M2 3 2 1 1 pch W=1.8u L=0.6u AS=3.24p PS=5.4u AD=3.24p PD=5.4u
CL 3 0 0.2pF

VDD 1 0 3.3
VIN 2 0 DC 0 PULSE (0 3.3 0ns 100ps 100ps 2.4ns 5ns)

.LIB '../mod_06' typical

.OPTION NOMOD POST INGOLD=2 NUMDGT=6 BRIEF
.DC VIN 0V 3.3V 0.001V
.PRINT DC V(3)
.TRAN 0.001N 5N
.PRINT TRAN V(2) V(3)
.END
```

- 節點：1 = VDD、2 = Vin、3 = Vout、0 = GND。M1 是 nMOS（W/L = 1.2/0.6 µm），M2 是 pMOS（W/L = 1.8/0.6 µm），負載 CL = 0.2 pF。
- `.DC` 掃 Vin 0 → 3.3 V 得到 **VTC**（voltage transfer curve）；`.TRAN` 跑 5 ns 的暫態，Vin 是週期 5 ns 的 pulse。
- 投影片：**Is this circuit design good?**
  - 【口頭】input 起來時 output 應該趕快下來，看起來下來得不夠快 → 通常把**電晶體的 W 變大**再重新模擬。
  - 【口頭】SPICE netlist 背後的 simulator 是怎麼寫的，之後會找專家來講。
- 【課本補充】課本 §2.3.1（p.49–51）用 rise time t_r、fall time t_f、propagation delay t_p 來衡量 inverter；要讓 t_r = t_f，W_p/W_n 在大部分 CMOS 技術是 1.5 到 3（因為電洞遷移率比電子低）。這裡 W_p/W_n = 1.8/1.2 = 1.5。

---

## 18. Physical design：從電路到幾何

[50:48]–[56:57]　投影片 Physical Design

- 例子是一個 5 輸入的 complex gate Z（輸入 a、b、c、d、e），左邊是電晶體電路，右邊是 layout（poly 直條，上排 p-diffusion 接 VDD、下排 n-diffusion 接 VSS）。
- **Physical design 把電路描述轉成幾何描述；這個描述用來製造晶片。**
- 【口頭】怎麼看 layout：
  - nMOS 那邊 c 和 b 串聯，所以 **b 和 c 共用一塊 diffusion**（節點 3）；c 的另一邊接 VSS，b 的另一邊接 output。
  - output 要用 wire 把好幾塊 diffusion 接起來；a 的 source 接 VSS。pMOS 那邊同理。
- 投影片：**Welcome to change input ordering to, e.g., a,b,c,d,e**——輸入順序改成 a、b、c、d、e 的話，layout 會長什麼樣？
- **Physical design cycle**：
  1. Logic partitioning
  2. Floorplanning and placement
  3. Routing
  4. DRC fixing、compaction…
- 其他：circuit extraction、**LVS**、timing 與 EM/IR verification、post-layout 分析與 debug、ECO…
  - 【口頭】compaction 是「二十多年前的古老時代」常做的，把 layout 壓得更緊。
  - 【口頭】**LVS（layout vs. schematic）**：從 layout 反推回來——poly 跨過 diffusion 就是一顆電晶體，看 source/drain 怎麼接——重建一份 schematic，和原本的比較。一定要通過才能放心繼續。
  - 【口頭】VDD 有 IR drop（掉 10%、15%）會影響電晶體效能。**EM（electro-migration）**：電流來來回回 toggle 太多，導線久了會變細、甚至斷掉。
  - 【口頭】這一連串工具都要由 EDA 公司提供，重點是**縮短 designer 的 turnaround time**。
- **Same or not?**（[56:57]）：兩個 complex gate 的 pull-up 網路裡，a 的位置上下對調。
  - 【口頭】請想一下：功能一樣嗎？電路效能哪個好、哪個壞、還是差不多？

---

## 19. 現代的 inverter layout：fingers、folding、Pcell

[57:24]–[59:42]　投影片 Nowadays Typical Inverter Layout I Saw

- **Poly 是垂直的。**
- **用 finger 複製電晶體來得到更大的寬度**——**transistor folding**：例如 16/2 的電晶體摺成兩個 8/2，或摺四次。
  - 【口頭】W 太大的話電晶體太高，和其他 cell 排在一起會有 alignment 問題，所以摺起來；摺了以後 poly 要用橫的接過來，或用 wire 接。
- **Graphical Pcell**（parameterized cell）：
  - 加上 **stretch handle**（圖上的 ×）方便編輯。【口頭】editor 很快 zoom in，選這個往上拉或往下拉就改變電晶體寬度。
- **PDK 是基礎**。【口頭】這些都放在 PDK 裡；PDK 要靠人力發展，是整個 IC 設計最 fundamental 的東西。

---

## 20. Physical design flow 與實例

[59:42]–[1:01:18]　投影片 Physical Design Flow、Floorplan Examples、Routing Example

- **Physical Design Flow**：Partitioning → Floorplanning & Placement → Routing → Compaction → Extraction & Verification。
  - 右邊兩張截圖：**B\*-tree based floorplanning system**、a routing system。【口頭】這張圖來自台大的教授。
- **Floorplan 實例**：PowerPC 604（IBM／Motorola）、Pentium 4（Intel）、一個有 9,800 個 block 的 floorplan。
- **Routing 實例**：0.18 µm 技術，pitch = 1 µm，2,774 條 net。
  - 【口頭】routing 不只要避免 DRC violation，還要能自動把 violation 解掉；鼓勵大家往這方面發展。

---

## 21. Technology roadmap 與 nanometer 設計挑戰

[1:01:18]–[1:06:06]　投影片 "Moore's" Law、Technology Roadmap、Nanometer Design Challenges

- **Moore's law**：logic capacity 以固定間隔加倍。Moore：每兩年加倍（1975）；D. House：電腦效能每 18 個月加倍（1975）。

**ITRS 2002 roadmap**（投影片表格）：

| 年 | 1997 | 1999 | 2002 | 2005 | 2008 | 2011 | 2014 |
|---|---|---|---|---|---|---|---|
| Node (nm) | 250 | 180 | 130 | 100 | 70 | 50 | 35 |
| On-chip clock (GHz) | 0.75 | 1.25 | 2.1 | 3.5 | 6.0 | 10 | 16.9 |
| 晶片面積 (mm²) | 300 | 340 | 430 | 520 | 620 | 750 | 901 |
| 電晶體數 | 11M | 21M | 76M | 200M | 520M | 1.40B | 3.62B |
| 每顆成本 (10⁻⁸ USD) | 3000 | 1735 | 580 | 255 | 110 | 49 | 22 |
| DRAM bits/chip | 256M | 1G | 4G | 16G | 64G | 256G | 1T |
| Wiring level | 6 | 6–7 | 7 | 7–8 | 8–9 | 9 | 10 |
| Supply voltage (V) | 1.8–2.5 | 1.5–1.8 | 1.2–1.5 | 0.9–1.2 | 0.6–0.9 | 0.5–0.6 | 0.37–0.42 |
| Power (W) | 70 | 90 | 130 | 160 | 170 | 175 | 183 |

- **Deep submicron**：node（feature size）< 0.25 µm；**nanometer**：node < 0.1 µm。
- 【口頭】這份資料準備時是 0.13 µm，表上 2014 年是 35 nm，現在已經到 7、5、3 nm。clock 越來越高、電晶體越來越多、supply voltage 一直降。
- ⚠️ 這是 2002 年的預測，實際上 clock 在 2005 年後就停在幾 GHz，沒有到 10–16.9 GHz（power 牆，見 [§22](#22-power製造生產力的挑戰)）。投影片沒有說明，這裡依常識補一句。

**Nanometer Design Challenges (1)**（2005 年：feature 0.1 µm、3.5 GHz、520 mm²、2 億顆、8 層金屬、1 V、160 W）：
- Feature size：sub-wavelength **lithography**（process variation 的影響）？noise？crosstalk？reliability？
- Frequency、尺寸：interconnect delay？電磁場效應？timing closure？
- 照片：electromigration 造成的 line-open、contact plug 裡的 open failure（N. Cheung、A. Tao，UC Berkeley）。
  - 【口頭】contact 很小，裡面金屬沒填好、中間有空洞，電流就過不去，造成 open。
  - 【口頭】老師提到「有名的院長的 immersion 技術」：指的應是**林本堅**（Burn J. Lin，前台積電研發副總、清大半導體研究學院院長），他提出浸潤式（immersion）微影，讓 193 nm 光源延用到更小的節點。⚠️ 逐字稿只聽得出「院長」，名字是依「immersion」和清大半導體學院院長推斷的。

**Nanometer Design Challenges (2)**：
- Chip complexity：大型系統的設計方法？
- Supply voltage：**signal integrity**（noise、IR drop）？
- Wiring level：manufacturability、yield（DFM）？3D layout？
- Power consumption：power、IR drop、thermal？
  - 【口頭】現在 power/ground 甚至走到晶片**背面**（backside）。
- **Design issues**：design space exploration；更有效率的最佳化演算法。
- **Verification issues**：state explosion；現代設計 **60%–80% 的設計時間花在 verification**；驗證工程師和邏輯設計師的人數比 **3 : 1**。
  - 【口頭】現在有 agentic AI 進來，可以想像設計環境會改變；幾家 EDA 公司都在和設計公司合作。

---

## 22. Power、製造、生產力的挑戰

[1:06:06]–[1:11:06]　投影片 Power Dissipation Challenges、MediaTek、Apple M1、Semiconductor Fabrication Challenges、Design Productivity Challenges

- **Power density 指數成長**（Fred Pollack, Micro32 1999 keynote）：power 每 4 年加倍；5 年預測 200 W、125 W/cm²；曲線從 i386、i486、Pentium 一路往 hot plate → nuclear reactor → rocket nozzle → sun's surface 走。P = VI：75 W @ 1.5 V = 50 A！
- **MediaTek Dimensity 9000**（4 nm）熱像：玩遊戲時比 2021 年的旗艦機涼。
- **Apple M1**（WSJ "The Chips That Rebooted the Mac"）：MacBook 又快又涼，風扇不用轉。
- 投影片：**Any thoughts about how to reduce power consumption?**
  - 【口頭】**clock gating**：某段時間某區域用不到，clock 就不要送進去；clock 沒進來，flip-flop 就不會 toggle，就不耗 dynamic power。
- **Semiconductor Fabrication Challenges**：feature size 縮小接近物理極限。微影步驟：photoresist coating → exposure → development。mask 圖形印到 wafer 上會失真；**proximity corrected layout** 印出來就比較準。
  - 【口頭】OPC 的做法：在凸出的角加「小耳朵」，凹進去的地方再多凹一點，印出來就比較接近原本的形狀。
- **Design Productivity Challenges**：電晶體數（complexity）每年成長 **58%**，designer 的生產力每年只成長 **21%** → 中間出現 gap。
  - **Human factors may limit design more than technology.**
  - 解法：**hierarchical design、abstraction、CAD（tool & methodology）、IP reuse**。【口頭】現在還有 agentic AI。

---

## 23. Recap（第二次上課開頭）

[1:11:06]–[1:13:24]　投影片 Recap

- 說明了 EDA 的重要性。
- 介紹了設計流程的主要元件：digital SoC、custom/analog design、OPC、advanced packaging。
- 開發 EDA 工具需要什麼：現在要學 AI/ML；**有創業機會**。
- Synthesis 簡介：domain translation 大量使用 **mux**。
- 電晶體設計與 layout 的圖：歡迎想想看這個 layout 夠不夠好；也講了 **transistor folding（multi-fingers）**。
- 開始講 design style——**很高興有人問了 emulation**（【Q&A】提問本身不在錄影裡）。

---

## 24. Design styles

[1:13:24]–[1:34:27]　投影片 Design Styles、SSI/SPLD、Full Custom、Standard Cell、Gate Array、FPGA、Comparisons

- 各種 design style：full custom、standard cell、sea of gates、FPGA…。**為什麼要有不同的 design style？**
- **特定的 design style 需要特定的 CAD 工具。**

```
              Issues of VLSI circuits
      Performance   Area   Cost   Time-to-market
                         ↓
               Different design styles
Full custom | Standard cell | Gate array | FPGA | CPLD | SPLD | SSI
←──────── Performance、Area efficiency、Cost、Flexibility 越往左越好
```

- 【口頭】full custom 最費力、最耗時，但效果最好（面積可以擠得最小）；standard cell 是先做好的 cell 兜起來再拉線；1980 年代 IBM 有 **sea of gates／gate array**，後來被 FPGA 取代；time-to-market 最好的是 FPGA。

**SSI／SPLD**：4-bit comparator 的三種做法——(a) logic 圖；(b) SSI 實作（74LS86、74LS02、74LS00 等 IC 插在板子上接線）；(c) SPLD（PLA）實作。**SPLD 是 FPGA 的前身。**

**Full custom**：
- designer 可以控制所有 mask pattern 的形狀，可以 specify 到每一顆電晶體。
- 圖：data path、PLA、I/O、ROM/RAM、controller、A/D converter、random logic，加上 over-the-cell routing、via、pins、I/O pads。
- 【口頭】據老師所知，現在還是 full custom 的大概是軍用晶片，因為數量不大。

**Standard cell**：
- 選用預先設計好、**同樣高度**的 cell 來實作邏輯。
- cell 排成同高度的 row；**routing channel 的高度會變 → chip size 不固定**（但不是現在的 style）。
- 圖上有 **feedthrough cell**：【口頭】當年金屬層少、不允許 over-the-cell routing，線要從 cell 中間穿過去。
- 【口頭】FinFET 時代有 **hybrid row**（不同 cell height）。現在的 style 是 chip size 固定，看 cell **utilization** 設多少——90% 太密就放鬆到 85%，routing 比較容易；同時跑好幾種 utilization 的 floorplan。
- **Standard cell 例子**：row 背對背（**back-to-back**）共用 VDD 或 GND，背對背的地方就沒有 routing channel。
- 【口頭】cell-based 最重要的是 **cell library**：每個 cell 的 performance、area、pin access（routing 拉得出來）。Intel、TSMC、Samsung 都有很大的 standard cell library 團隊。

**Gate array**：
- **預先做好電晶體陣列**，只要**客製化 wiring** 就能實作邏輯。
- cell row 高度相同、row 間距相同 → **chip size 固定**。
- 【口頭】placement 就是把 logic map 到每個小 block 裡，再在有限空間裡拉線。**老師當年的博士論文就是做 gate array 的 routing system。**

**FPGA**：
- **邏輯和 interconnect 都預先做好**（以 symmetric array-based FPGA 為例）：logic blocks、routing tracks、switches。
- **下載 0101… 的 bitstream，晶片的功能就改變。**
  - 【口頭】logic block 裡是 SRAM 存的 truth table：2 個 input 有 2² = 4 個 bit，改這 4 個 bit 的內容就得到不同的 function。interconnect 上每個點叫 **PIP（programmable interconnect point）**，由一個 SRAM bit 控制 nMOS pass gate 開或關。
  - 【口頭】1984–85 年 Xilinx 出來，後來很多公司跳進來；Altera 被 Intel 買進去，AMD 買了 Xilinx，現在 Intel 又要把 Altera 分出去——生意上分分合合。
- **現在是 emulation 背後的主力。**【口頭】emulation 就是 prototyping：晶片還沒做回來之前就有一顆可以執行那個功能的晶片，軟體團隊可以先在上面發展。
- **FPGA logic cell 的簡化圖**：兩個 3-LUT（輸入 a、b、c、d）＋ full adder（carry in／carry out）＋ mux ＋ DFF ＋ output mux。
  - 【口頭】3 個 input 的 LUT 有 2³ = 8 個 bit。不做加法時 full adder 被 bypass，面積就浪費了——所以 FPGA 通常比較大，performance 也還可以。
- **Lucent 15K ORCA FPGA**：0.5 µm 3 層金屬 CMOS、245 萬顆電晶體、1,600 個 flip-flop、25K bit user RAM、320 個 I/O。【口頭】Lucent 是 AT&T 的 spin-off。
- **FPGA 設計流程**：system design → partitioning & technology mapping → placement → routing → customization（logic + layout synthesis）。**不需要製造。**
  - 另一個設計進來就重新 map、place、route。**如果設計放得進一顆 FPGA 很好；放不進呢？**
  - 【口頭】大設計就要 partition 到多顆 FPGA，這時又被 FPGA 的 pin count 限制住；emulation 就是這樣一步步發展的。

**Design style 比較**：

| | Full custom | Standard cell | Gate array | FPGA | SPLD |
|---|---|---|---|---|---|
| Cell size | variable | fixed height* | fixed | fixed | fixed |
| Cell type | variable | variable | fixed | programmable | programmable |
| Cell placement | variable | in row | fixed | fixed | fixed |
| Interconnections | variable | variable | variable | programmable | programmable |

\* 也有不同高度的 cell。

| （+ 好，− 不好） | Full custom | Standard cell | Gate array | FPGA | SPLD |
|---|---|---|---|---|---|
| Fabrication time | − − − | − − | + | + + + | + + |
| Packing density | + + + | + + | + | − − | − − − |
| Unit cost（大量） | + + + | + + | + | − − | − |
| Unit cost（少量） | − − − | − − | + | + + + | + + |
| Easy design and simulation | − − − | − − | − | + + | + |
| Easy design change | − − − | − − | − | + + | + + |
| Accuracy of timing simulation | − | − | − | + | + + |
| Chip speed | + + + | + + | + | − | − − |

- 【口頭】現在大部分都走 standard cell：速度不錯、製造時間不像 full custom 那麼長、density 還可以、大量時成本也不錯，只有少量時不如 FPGA。

**Design Style Trade-offs**（圖）：x 軸 logic capacity（gates，1 到 10⁷），y 軸 turnaround time（days，1 到 10⁴）。SSI 在左下（容量小、時間短）；SPLD、CPLD、FPGA 往右上；semi-custom、full custom 在右上（容量大、時間長）。右下角（容量大、時間短）是「optimal solution」。

---

## 25. MOS 電晶體與 CMOS

[1:34:27]–[1:37:06]　投影片 MOS Transistors、Complementary MOS；課本 §2.2.1（p.41–44）

| | pMOS | nMOS |
|---|---|---|
| 結構 | n− substrate 上的 p+ diffusion | p− substrate 上的 n+ diffusion |
| 何時 ON | gate 加 0（**GND 加在 gate 就導通**） | gate 加超過 threshold 的電壓（**VDD 加在 gate 就導通**） |
| 傳得好的訊號 | **"1"** | **"0"** |

- gate 是控制輸入；drain、source 是開關的兩端（物理上等價）。gate 是 polysilicon，下面是絕緣層 SiO₂。
- **CMOS（Complementary MOS）**：
  - 最普及的 VLSI 技術（相對於 BiCMOS、nMOS）。
  - 同時用 n-channel 和 p-channel 電晶體。
  - 優點：**低功耗**、規則性高、效能可靠、noise margin 高、fanout 大…
  - 每種電晶體都要坐在**相反型態**的材料裡（逆偏的二極體防止不要的電流）。
  - 圖：p-well CMOS 結構——n− substrate 接 VDD，p-well 接 GND。

---

## 26. CMOS inverter 與 CMOS 的性質

[1:37:06]–[1:43:51]　投影片 A CMOS Inverter、CMOS Properties、Cross Section；課本 §2.3.1（p.49–52）

**CMOS inverter**：B = A'。

| A | B |
|---|---|
| 1 | 0 |
| 0 | 1 |

- Layout 圖例：metal 1 藍、metal 2 棕、polysilicon 紅、contact/via 黑、p-diffusion 黃、n-diffusion 綠。
- 【口頭】A = 1：nMOS 導通，GND 接到 output → B = 0。A = 0：nMOS 關、pMOS 開，VDD 接到 output → B = 1。
- 投影片：**只有很短的時間存在一條從 VDD 到 GND 的導通路徑；因此 leakage current 少。**【口頭】input 走到 VDD/2 附近時，nMOS 正從 off 變 on、pMOS 正從 on 變 off，兩個都半開。
  - ⚠️ 投影片和老師把這段電流叫 leakage；課本 §2.3.1（p.51–52）把它叫 **short-circuit（direct-path）current**，另外的 leakage 指電晶體關著時的漏電。兩者不同。

**CMOS 的性質**：
1. 永遠有一條路從一個 supply（VDD 或 GND）到 output。
2. **永遠沒有一條路從一個 supply 到另一個 supply**——這是 CMOS 低功耗的基礎，**幾乎沒有 static power**。
3. gate 切換時有**短暫的電流**（所以耗電）→ CMOS 有 **dynamic power**，大小取決於切換頻率：

$$P = \frac{1}{2} C V_{dd}^2 f$$

- 【口頭】和負載電容、頻率有關；toggle 頻率越高的節點越耗電。

**Cross section 與 layout**：p-diff 在 n-substrate 裡（pMOS），n-diff 在 P-well 裡（nMOS），兩個 poly gate 接同一個 input，pMOS 和 nMOS 的 drain 接成 output。
- 【口頭】早期 poly 是橫的；現在的 layout 大部分 **poly 是垂直的**，上面 p-type、下面 n-type diffusion，pMOS 接 VDD、nMOS 接 GND。W 太高就 folding（摺 2 次、摺 4 次）。

---

## 27. NAND、NOR 與基本 CMOS logic library

[1:43:51]–[1:49:24]　投影片 A CMOS NAND Gate、A CMOS NOR Gate、Basic CMOS Logic Library；課本 §2.3.2（p.52–55）

**NAND：C = (A·B)'**

| A | B | C |
|---|---|---|
| 0 | 0 | 1 |
| 0 | 1 | 1 |
| 1 | 0 | 1 |
| 1 | 1 | 0 |

- nMOS **串聯**、pMOS **並聯**。只有 A、B 都是 1 時兩顆 nMOS 都通，output 接到 GND；只要有一個 0，那顆 nMOS 關、對應的 pMOS 開，output 接到 VDD。
- 【口頭】layout 上兩顆 nMOS 共用一塊 diffusion。

**NOR：C = (A + B)'**

| A | B | C |
|---|---|---|
| 0 | 0 | 1 |
| 0 | 1 | 0 |
| 1 | 0 | 0 |
| 1 | 1 | 0 |

- nMOS **並聯**、pMOS **串聯**。只要有一個是 1，某顆 nMOS 就通，output = 0。

**Basic CMOS Logic Library**：

| Gate | 式子 | Cost（電晶體數） | Scaled gate delay (ps) |
|---|---|---|---|
| AND | F = XY | 6 | 24 |
| OR | F = X + Y | 6 | 24 |
| NOT | F = X' | 2 | 10 |
| Buffer | F = X | 4 | 20 |
| **NAND** | F = (XY)' | **4** | **14** |
| NOR | F = (X + Y)' | 4 | 14 |
| XOR | F = XY' + X'Y | 14 | 42 |

- **AND = NAND + NOT**：cost 4 + 2 = 6，delay 14 + 10 = 24（投影片用箭頭標出）。Buffer = 兩個 NOT：4 顆、20 ps。
  - ⚠️ 逐字稿把 NOT 的 delay 聽成「4 picosecond」、AND 的 delay 聽成「4 加 10」，對照投影片是 NOT 10 ps、NAND 14 ps、AND = 14 + 10 = 24 ps。
- 【口頭】這是 first-order approximation 的 propagation delay；cost 通常就是電晶體數。這張表在做最佳化、尤其 technology mapping 時很常用。

---

## 28. Compound gate 的建法

[1:49:24]–[1:54:09]　投影片 Construction of Compound Gates、Homework Is Coming、A Complex CMOS Gate

例子：$F = \overline{A\cdot B + C\cdot D}$

1. **Step 1（n 網路）**：把 F 反相得到 n 網路：$\overline{F} = A\cdot B + C\cdot D$。
2. **Step 2（n 網路）**：接電晶體——**AND ⟺ 串聯、OR ⟺ 並聯**。
   - A、B 串聯；C、D 串聯；兩組再並聯。
3. **Step 3（p 網路）**：展開 F 得到 p 網路：$F = \overline{AB+CD} = \overline{AB}\cdot\overline{CD} = (\overline{A}+\overline{B})\cdot(\overline{C}+\overline{D})$——**每個輸入都反相**（正好對應 pMOS 是 gate = 0 時導通）。
4. **Step 4（p 網路）**：接電晶體，規則同 Step 2。
   - A、B 並聯；C、D 並聯；兩組再串聯。
5. **Step 5**：n 網路接 GND（通常 0 V），p 網路接 VDD（5 V、3.3 V、2.5 V 等）。

```
General structure:           CMOS compound gate for F = (AB + CD)'
   VDD                          VDD
    │                         ┌──┴──┐
 [F using p-type]           A ┤p   p├ B      ← A、B 並聯
    │                         └──┬──┘
    ├── F                     ┌──┴──┐
    │                       C ┤p   p├ D      ← C、D 並聯；兩組串聯
 [F' using n-type]            └──┬──┘
    │                            ├──── F
   GND                      ┌────┴────┐
                          A ┤n       n├ C    ← A-B 串聯、C-D 串聯；兩組並聯
                          B ┤n       n├ D
                            └────┬────┘
                                GND
```

- n 網路和 p 網路是 **dual**（串聯 ↔ 並聯）。【實測】見 [§32](#32-實測compound-gate-的-switch-level-檢查)。
- **Homework 預告**：logic gate F = (A + B)(CD + E)，(1) 用 gate 實作；(2) 用 P、N 電晶體實作同一個邏輯。（作業，筆記不提供解答。）

**A Complex CMOS Gate**：
- n、p 網路實現的函數必須**互補**，而且**每一種輸入組合都要有一個網路導通**。
- **Duality 不是必要的。**
- 投影片的例子：輸入 x、y、z（和它們的反相），兩個輸出 F、G，上面兩顆 pMOS 交叉耦合（cross-coupled）。真值表只給了 xyz = 000 時 F = 0、G = 1。
  - 【口頭】xyz = 000：x' 和 y' 是 1，對應的 nMOS 通，F 拉到 GND → F = 0；F = 0 讓 G 那邊的 pMOS 通 → G = 1。
  - 投影片：**Please complete this truth table and two functions.**（課堂練習，筆記不提供解答。）

---

## 29. Stick diagram

[1:54:09]–[1:58:39]　投影片 Stick Diagram、CMOS Inverter Stick Diagrams、CMOS NAND/NOR Stick Diagrams；課本 §2.5.2（p.75–）

- **介於電晶體層級和 mask（layout）層級之間的表示法。**
- 提供拓樸資訊（哪些 layer、彼此關係）。
- **假設導線沒有寬度。**
- 可以自動轉成符合 **design rules** 的 layout。【口頭】這是當年的期望。
- p-channel switch：黃色 p-diff 被紅色 poly 橫過；n-channel switch：綠色 n-diff 被 poly 橫過。

**規則**：
- **同一層的材料碰到或交叉 → 相連，是同一個電氣節點。**
- **Poly 跨過 N 或 P diffusion → 形成一顆 N 或 P 電晶體。**
  - poly 畫在 diffusion 上面；diffusion 要畫成連接 source 和 drain；gate 在製造時**自動對準（self-aligned）**。
  - 【口頭】diffusion 是整塊打下去、中間沒有斷，poly 蓋過去的地方自然變成 channel。
- **metal 要接到其他三種導體（p-diff、n-diff、poly）時，需要 contact cut（via）**（圖上的 ×）。

**CMOS inverter 的兩種 stick diagram**：
- Basic layout：diffusion 垂直，poly 繞成 ⊐ 形接到兩顆電晶體。
- **More area efficient layout**：VDD、Vss、In（poly）、Out 都是水平線，p-diff 和 n-diff 是垂直的。
- 【口頭】現在大部分是 poly 垂直、上 p-diff 下 n-diff，再加 folding。

**CMOS NAND/NOR stick diagrams**：每個各兩種畫法——**area efficient? contacts?**
- 【口頭】第二種畫法 GND 和 VDD 比較靠近，面積會小一點。

---

## 30. FinFET 與 planar／FinFET layout 的差別

[1:58:39]–[2:05:15]　投影片 From Planar To FinFet、FinFET Advantages & Considerations（Synopsys 2014）、Planar to FinFET Layout Differences

**From Planar to FinFET**：
- 【口頭】planar：poly gate 跨過去，控制 drain 到 source 之間的 channel。為了增加 drive strength、減少 short channel effect，把 diffusion 立起來像一座山，gate 從三面包住。

$$W_{eff} = (2 \times \text{Fin height} + \text{Fin width}) \times N, \quad N = \text{fin 的數目}$$

- **寬度可以變大，但被量子化（quantized）**——只能是一根 fin 的整數倍。
- 【口頭】FinFET 的發明人是 UC Berkeley 的**胡正明（Chenming Hu）**教授。⚠️ 逐字稿寫「Chamming 胡」。
- 參考：IEEE Xplore 6649058 "FinFETs – Technology and circuit design challenges"。

**FinFET 的優點與考量**（Synopsys, 2014）：

| 優點 | 考量 |
|---|---|
| Lower leakage | Quantized widths |
| Less variability | No body biasing |
| Lower voltage | Higher parasitics（【口頭】立起來以後 coupling 增加） |
| | Aging |

**Planar vs FinFET 的 inverter layout**（simplified generic inverter）：
- Planar：pmos w = 398 nm、l = 45 nm；nmos w = 268 nm、l = 45 nm。poly 垂直，pMOS 的 source 和 nMOS 的 drain 用 metal 接起來，打很多 contact，中間拉出 output。
- FinFET：pFinFET nFins = 6、nFinFET nFins = 5，L = 18 nm。橫的是 fin rows。
  - ⚠️【口頭】老師在圖上數 nMOS 有 7 根 fin（「1234567」），但投影片電路圖上標 nFins = 5，以投影片標示為準，圖上多出來的 fin 可能是 dummy。
  - 【口頭】左右兩邊多了兩條 **dummy poly**，讓 FinFET 結構比較穩；中間 poly 被截斷的地方叫 **cut poly**。
  - 【口頭】diffusion 先用 **local interconnect**（LI1）連起來；**V0** 是把 poly 拉出來的 via；FinFET 設計裡 **metal 1 是垂直走**。
  - 圖例 layer：NWell、Active、Pimplant、Nimplant、Poly、LI1、LIPO、Metal1、CutPoly、V0、FinArea。
- 【口頭】**FinFET 的 layout 複雜得多。**

---

## 31. Design rules：micron rules、λ rules、MOSIS

[2:05:15]–[2:11:27]　投影片 Design Rules (1)(2)、SCMOS Design Rules、MOSIS Layout Design Rules、Summary；課本 §2.5.1（p.72–75）

**Design Rules (1)**：
- layout rule 用來準備製造用的 mask。
- 製程本身的精確度有限。
- design rule 規定 mask 的幾何，以最佳化**良率和可靠度**（取捨：面積、良率、可靠度）。
- **三大規則**：
  1. **Wire width**：某種 feature 的最小尺寸
  2. **Wire separation**：允許的間距
  3. **Contact**：overlap 規則
- **三種基本 DRC check**：width、spacing、**enclosure**。【口頭】via 打下去，metal 要包住它留足夠空間，via 才牢固；現在製程更好，enclosure 可能會放寬。
- 也要處理 **non-Manhattan**（非水平垂直）的形狀。

**Design Rules (2)**：
- 兩種做法：
  - **"Micron" rules**：用 micron 精度寫死的絕對值。
  - **λ rules**：簡化的 micron rules，有有限的 **scaling** 能力。
- **λ 可以看成最小 feature 的大小**——可以看成「參數」。
  - 【口頭】λ 一改，layout 就能重新產生；從一家 foundry 換到另一家，只要改 λ。當時也有人提議水平、垂直用兩個不同的 λ（λ1、λ2）。
- **Design rule 代表一個容許範圍，保證很高的機率能正確製造**（不是正確／錯誤之間的硬邊界）。
- design rule 由**經驗和實驗**決定：foundry 跑很多 test pattern → **test chip**。
  - 【口頭】test pattern 上有各種不同的 spacing、width，做出來以後找出一組良率很高的值。

**SCMOS（Scalable CMOS）Design Rules** 圖：poly 寬 2λ、間距 2λ；metal 1 寬 3λ、間距 3λ；metal 2 寬 3λ、間距 4λ；n-diff 和 p-diff 之間 10λ；還有 tub tie、via。
- 投影片：**以前有用 λ rule 的 layout editor：給定 λ 的真實值，就產生新的 layout。**
- 【口頭】當時已經有共用的 fab 提供 foundry service 的概念，只是名聲不太好，因為要接各式各樣的 layout，技術還沒做到那麼好。

**MOSIS layout design rules**（SCMOS rules，http://www.mosis.org）：

| 規則 | 內容 | 值 |
|---|---|---|
| R1 | Min active area width | 3λ |
| R3 | Min poly width | 2λ |
| R4 | Min poly spacing | 2λ |
| R5 | Min gate extension of poly over active | 2λ |
| R8 | Min metal width | 3λ |
| R9 | Min metal spacing | 3λ |
| R10 | Poly contact size | 2λ |
| R11 | Min poly contact spacing | 2λ |

- 【課本補充】課本 Table 2.5（p.73，取自 Weste 1994）列出 n-well 製程的 λ-rule 與 µ-rule 兩組規則（well、active、poly、contact、metal 等），並把規則分成 **feature size**、**separation**、**overlap** 三類，和投影片的 width／separation／contact 對應。
- ⚠️ 課本 p.72 寫「λ（which is typically twice the channel feature size）」；但 MOSIS 的 R3 是 min poly width = 2λ，也就是最小通道長度 = 2λ、**λ 是通道長度的一半**。兩者說法相反，依 MOSIS 規則應為「通道長度 = 2λ」。

**Summary So Far**：digital design flow 的步驟；Boolean functions；基本 CMOS 電晶體電路與 logic gate。

---

## 32. 【實測】compound gate 的 switch-level 檢查

程式：`experiments/lecture02/cmos_gate.py`，輸出：`experiments/lecture02/output.txt`。

做法：把 $\overline{F}$ 寫成 AND/OR 的樹，照 §28 的規則建 n 網路（AND → 串聯、OR → 並聯），p 網路取 dual；然後窮舉所有輸入，用 switch-level 模型（nMOS gate = 1 導通、pMOS gate = 0 導通）檢查**恰好一個網路導通**、而且 output 等於 F。

```python
def n_network(expr):
    """Pull-down network for output F where expr = F' (no inversions inside)."""
    if expr[0] == 'var':
        return ('nmos', expr[1])
    kind = 'series' if expr[0] == 'and' else 'parallel'
    return (kind,) + tuple(n_network(e) for e in expr[1:])


def dual(net):
    """p-network = dual of n-network: series <-> parallel, nMOS -> pMOS."""
    if net[0] == 'nmos':
        return ('pmos', net[1])
    kind = 'parallel' if net[0] == 'series' else 'series'
    return (kind,) + tuple(dual(n) for n in net[1:])
```
（`experiments/lecture02/cmos_gate.py:17-30`）

結果：

| Gate | n 網路 | p 網路 | 電晶體 | 全部輸入都對？ |
|---|---|---|---|---|
| INV | A | A | 2 | ✓（2 種） |
| NAND2 | A、B 串聯 | A、B 並聯 | 4 | ✓（4 種） |
| NOR2 | A、B 並聯 | A、B 串聯 | 4 | ✓（4 種） |
| (AB + CD)' | (A–B 串) ∥ (C–D 串) | (A∥B) 串 (C∥D) | 8 | ✓（16 種） |

- 電晶體數和 §27 的 library 一致（INV 2、NAND/NOR 4）；slide 91 的 AOI22 是 8 顆——比「2 個 AND + 1 個 NOR」的 6 + 6 + 4 = 16 顆少一半。這就是 compound gate 的好處。
- dual 構造保證兩邊互補：n 網路導通的條件是 $\overline{F}$，p 網路導通的條件（每個輸入反相、串並對調）由 De Morgan 正好是 F。

---

## 33. 一頁速查表

| 主題 | 重點 |
|---|---|
| 里程碑 | 1947 電晶體、1958 IC、1965 Moore、1971 4004、1987 TSMC（fabless）、1991 ARM IP（chipless） |
| A12 | 7 nm、83 mm²、69 億顆、~75 個 unique block |
| 設計流程 | spec → architecture → logic → synthesis → physical design（floorplan→place→CTS→route→timing→DFM）→ mask & verification → fab → package & test |
| Y chart | behavioral／structural／physical 三軸 × 五個抽象層 |
| 三種 view | behavior（HDL）、structural（netlist）、physical（幾何） |
| PPA | performance、power、area；deep submicron 時 **interconnect delay 主導** |
| Signoff | DRC、LVS、RC extraction（corners）、STA、IR-drop、EM |
| ML in EDA | ML-inside（換 kernel）、ML-outside（調參數，較成功） |
| 5 種 design action | synthesis、analysis、verification、optimization、design management |
| HDL synthesis | domain translation（大量用 mux）＋ optimization |
| Technology mapping | 在 library 找功能相等、cost 較低的 cell |
| Cell library | 同一 cell 多種 drive strength：delay 差不多，面積和 leakage 變大 |
| Physical design cycle | partitioning → floorplan & placement → routing → DRC fixing／compaction；再 extraction、LVS、EM/IR、ECO |
| Folding | W 太大就摺成多個 finger |
| Design styles | full custom、standard cell、gate array、FPGA、SPLD、SSI；現在主流 standard cell |
| FPGA | logic（LUT）和 interconnect（PIP）都預製；下載 bitstream；emulation 主力 |
| pMOS／nMOS | pMOS gate = 0 導通、傳 1 好；nMOS gate = 1 導通、傳 0 好 |
| CMOS | 沒有 VDD→GND 的靜態路徑 → 幾乎沒 static power；dynamic P = ½CV²f |
| NAND／NOR | NAND：n 串 p 並；NOR：n 並 p 串；NAND 4 顆 14 ps，AND = NAND + NOT = 6 顆 24 ps |
| Compound gate | F' 建 n 網路（AND 串、OR 並），p 網路取 dual，輸入反相 |
| Stick diagram | 線沒寬度；同層相交即相連；poly × diffusion = 電晶體；metal 接其他層要 contact |
| FinFET | W_eff = (2H_fin + W_fin) × N，寬度量子化；低漏電、低變異、低電壓；無 body bias、寄生大 |
| Design rules | width、spacing、enclosure；micron rules vs λ rules；MOSIS：poly 2λ、metal 3λ |

---

## 34. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| algorithms、complexity、scalability | 第 3–5 支：ch2 complexity、Python 範例、Steiner tree |
| logic synthesis、technology mapping、cell library | 第 6 支：ch10 synthesis |
| compaction、design rules | 第 7 支：ch3 compaction |
| partitioning（連接強的放一起） | 第 8 支：ch4 partitioning |
| floorplanning、B\*-tree | 第 9 支：ch5 floorplanning |
| placement、standard cell row、utilization | 第 10 支：ch6 placement |
| routing、routing channel、pin assignment | 第 11–13 支：routing、pin assignment |
| clock tree synthesis、power/ground、IR-drop | 第 14–15 支：channel routing、clock、P/G、CTS |
| 作業 F = (A+B)(CD+E) 的 gate／電晶體實作 | Hw-1（[第 1 支](lecture01_note.md) §13） |

---

## 附：本講一句話總結

> **一顆像 A12 這樣有 69 億顆電晶體、75 個 unique block 的晶片，從 partition、選 node、floorplan、pin assignment、DRC/LVS/RC extraction、STA 到 ECO 每一步都要 EDA；整個流程是 spec → architecture → logic synthesis → physical design（floorplan、placement、CTS、routing、timing、DFM）→ mask verification → fab，可以用 Gajski-Kuhn 的 Y chart 和 behavior／structural／physical 三種 view 來看，設計品質用 PPA 衡量，而 deep submicron 以後 interconnect delay 主導效能，所以 synthesis 要帶 physical 資訊；HDL synthesis 是 domain translation（大量用 mux）加 optimization，technology mapping 則是在 cell library 裡找功能相等、cost 較低的 cell；design style 從 full custom、standard cell、gate array 到 FPGA，用 performance／area／cost 換 time-to-market，現在主流是 standard cell，FPGA 則是 emulation 的主力；CMOS 因為 VDD 和 GND 之間沒有靜態路徑，只有 P = ½CV²f 的 dynamic power，gate 的建法是 AND 串聯、OR 並聯、n 網路和 p 網路互為 dual（本機實測對 INV、NAND、NOR、AOI22 窮舉驗證成立），layout 從 stick diagram 開始，受 width／spacing／enclosure 的 design rule（λ rules、MOSIS）約束，而 FinFET 讓電晶體寬度變成 fin 數的整數倍。**

---

*筆記依據李炯霆「Electronic Design Automation (1)」第2講｜ch1.intro_xtor_design 錄影（NTHU OCW，2:11:31；影片無字幕，以 whisper large-v3-turbo 轉錄）、投影片截圖（Unit 1）、Wang, Chang, Cheng（eds.）*Electronic Design Automation: Synthesis, Verification, and Test* Ch1 §1.1.2、§1.2.3，Ch2 §2.2–2.3、§2.5 整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
