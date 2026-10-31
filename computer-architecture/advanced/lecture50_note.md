# Advanced Computer Architecture 第五十支影片完整筆記：可靠度（二）：固有非確定性、確定性匯流排、設計缺陷（Errata）、形式驗證與矽後除錯（Chapter 12, Part II）

> **課程**：Advanced Computer Architecture — Chapter 12: Reliability (non-determinism and design faults) -- Part II/III（播放清單第 50 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（43:28）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch12** §12.3–12.4（p.649–656）
> **章號**：影片 Chapter 12 = 課本 Ch12。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕把「ALU operations」寫成「alien operations」、「boot time」寫成「good time」、「on-chip bus」寫成「launcher bus」、「Verilog or VHDL」寫成「very logger vhdl」、「burn-in testing」寫成「burning testing」、「$I_{DDQ}$ and shmooing」寫成「iddq and schmooing」、「DVFS table」寫成「dvfstick」、「boot an OS」寫成「boot a noise」、「suffers a miss」寫成「suffers MS」、「two AAM (adjust-after-multiply)」寫成「2 am／address after multiply」、「critical-concurrent」寫成「critical conquering」、「DfD」寫成「DFD」、「aging」寫成「edging」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 固有非確定性（Inherent Nondeterminism）：為什麼同一支程式每次跑的內部狀態都不同？](#1-固有非確定性inherent-nondeterminism為什麼同一支程式每次跑的內部狀態都不同)
- [2. 打造確定性處理器與確定性匯流排（$t_s + \theta_2$ 最壞延遲緩衝）](#2-打造確定性處理器與確定性匯流排t_s--theta_2-最壞延遲緩衝)
- [3. 設計缺陷（Design Faults）：Pentium FDIV、AMD TLB 與微碼／軟體修補](#3-設計缺陷design-faultspentium-fdivamd-tlb-與微碼軟體修補)
- [4. 設計期驗證（Verification）vs 矽後驗證（Validation）](#4-設計期驗證verificationvs-矽後驗證validation)
- [5. 公開勘誤表（Errata）實例與兩套正交缺陷分類法](#5-公開勘誤表errata實例與兩套正交缺陷分類法)
- [6. 訊號擷取修補（Signal Tapping）與 DfD 追蹤緩衝區壓縮／摘要](#6-訊號擷取修補signal-tapping與-dfd-追蹤緩衝區壓縮摘要)
- [7. 【原始碼】Linux x86 `cpu/amd.c` 如何替公開 CPU Errata 打作業系統補丁](#7-原始碼linux-x86-cpuamdc-如何替公開-cpu-errata-打作業系統補丁)
- [8. 【實測】本機執行期非確定性、匯流排確定化、Model Checking 抓反例與 DfD 壓縮](#8-實測本機執行期非確定性匯流排確定化model-checking-抓反例與-dfd-壓縮)
- [9. 一頁速查表](#9-一頁速查表)
- [10. 與後續講次的連結](#10-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
一、固有非確定性（Inherent Nondeterminism，§12.3）
  ├── 現象：C 程式外部輸出（Hello World）不變，但執行週期數、記憶體映射、每支腳位與正反器內部狀態每次跑都變！
  ├── 五大來源：
  │     1. CPU/Cache：隨機快取置換、匯流排仲裁亂數、未初始化正反器、DVFS/Clock/Power gating 喚醒延遲浮動（甚至漏接中斷）
  │     2. DRAM：機會主義排程的 Refresh、軟錯誤觸發的 ECC Scrubbing
  │     3. I/O：傳統硬碟機械尋軌與旋轉延遲
  │     4. 匯流排／NoC：溫度漂移、時脈偏移與 Source-synchronous 鎖相造成 ±1–2 cycles 抖動
  │     5. 暫態錯誤回滾重跑
  └── 強制確定化（為了在實驗室重現百萬分之一的現場 bug）：
        • CPU/I/O：重設所有正反器／快取／TLB，精確記錄（log）並重播（replay）中斷與 DVFS 時間點
        • 匯流排：傳送端附帶本地時間 t_s，接收端到達範圍 [t_s+θ₁, t_s+θ₂] → 提早到的律緩衝等到最壞時間 t_s+θ₂ 才處理！

二、先天設計缺陷（Design Faults / Hardware Bugs，§12.4，即便嚴測仍有 50–100 個 bug 流進市售晶片！）
  ├── 慘痛歷史：1994/95 Intel Pentium FDIV 查表錯誤（召回賠 4.75 億美元）、AMD Barcelona/Phenom TLB 死鎖
  ├── 設計期 Verification（佔 50–60% 工時，矽前）：
  │     • Formal Verification（黃金標準）：Automated Theorem Proving（公理推導）vs Model Checking（如 SPIN，可吐反例！）
  │     • 測試向量模擬：ATPG 衝高 coverage，跑在 RTL simulator 或更快的 FPGA prototype
  ├── 製造後 Validation（矽後）：
  │     Wafer probe → Burn-in（200–250°C 烤機）→ Scan chain（JTAG 串列讀寫正反器，但會停機）
  │     → I_DDQ（靜態電流抓短路）→ Shmooing / Binning（掃 V-f 圖分級定價與建每顆專屬 DVFS 表）→ Functional testing（開 OS）
  └── 缺陷分類與矽後補救（Errata Workarounds & DfD）：
        • 軸一：Non-critical（效能計數器） / Critical-concurrent（同拍布林組合） / Critical-complex（跨拍時序狀態機）
        • 軸二：Logic design（RTL 寫錯） / Algorithmic（規格概念錯，如捨入方向） / Timing（路徑超時）
        • 補救：BIOS 關功能、OS／編譯器避開指令序列、Microcode 韌體替換、可程式化訊號攔截（Signal Tapping + PLA）
        • 原速除錯（At-speed DfD）：環狀 Trace Buffers + Spatial / Temporal Summarization（壓縮與睡眠驗證後清空）
```

---

## 1. 固有非確定性（Inherent Nondeterminism）：為什麼同一支程式每次跑的內部狀態都不同？

[00:25]（課本 §12.3，p.649）如果給處理器一模一樣的輸入，它每次都會在完全相同的 cycle 數內、經過完全相同的內部狀態、吐出相同的輸出嗎？**不會！**
- 對一般軟體工程師來說，C 程式印出 `"Hello World"` 每次都對就夠了，沒人在乎它這次跑了 $10{,}000$ 拍還是 $10{,}003$ 拍，也不在乎虛擬記憶體映射位址變了 `[01:48]`。
- 但對**硬體除錯與驗證工程師**來說，非確定性是場惡夢 `[02:29]`：假設客戶現場回報「跑了一百萬次出現一次死機或某支記憶體腳位在某拍變成 `1`」，你把同一支程式搬回實驗室測試板重跑，因為每次內部時序都差一兩拍，**那百萬分之一的微妙交錯（corner case，例如某介面原本預期 3 拍完成，在特定電壓溫度組合下突然變 4 拍 `[08:01]`）再也重現不出來！**

### 1.1 系統中的五大非確定性來源

[03:37]（課本 §12.3.1，p.649–650，整理自 Sarangi et al., 2006a）：
1. **CPU 與快取內部** `[03:37]`：
   - **隨機置換策略（Random cache replacement）**與**匯流排仲裁器（bus arbiters）**在開機時用亂數種子決定衝突贏家，只要前後差一兩拍，整座快取的內容與命中率就會徹底分岔。
   - **未初始化的狀態元件（uninitialized flip-flops/latches）** `[04:13]`：開機殘留的隨機 `0/1` 會改變內部轉態與微架構路徑。
   - **省電技術（Power gating、Clock gating、DVFS）** `[04:49]`：受外部電壓與溫度影響，睡眠電晶體喚醒或 PLL 重新鎖相（relock）所需時間每次略有浮動——`【口頭】` 有時某次 DVFS 恢復剛好慢了一點，不幸漏接一個硬體中斷（hardware interrupt）引發當機，下次跑 PLL 鎖得快又完全正常！
2. **DRAM 主記憶體** `[06:07]`：如[第四十三講](lecture43_note.md)所學，**DRAM Refresh** 會看記憶體流量空檔做機會主義排程；再加上宇宙射線軟錯誤累積的位元翻轉數量無法預測，背景執行 **ECC Scrubbing（巡檢修復）** 花掉的時間天生就是非確定性的。
3. **I/O 與中斷** `[06:49]`：傳統機械硬碟的尋軌（seek）與碟片旋轉延遲根本不可能在奈秒級別預測。
4. **晶片內／主機板匯流排（Buses & NoC）** `[06:56]`：`【口頭】` 像 HyperTransport 這類高速互連常採用**源同步匯流排（source-synchronous bus）**——把時脈訊號跟著資料一起傳過去讓接收端鎖定；但物理傳輸延遲受環境溫度影響，加上傳送端與接收端時脈會有微小相對漂移（clock drift，課本 p.650），常讓訊息在這次第 10 拍到、下次變第 11 拍到。
5. **暫態錯誤恢復** `[05:46]`：軟錯誤或電感雜訊觸發的管線 flush 與 checkpoint 重跑。

---

## 2. 打造確定性處理器與確定性匯流排（$t_s + \theta_2$ 最壞延遲緩衝）

[08:28]（課本 §12.3.2，p.650）為了讓現場抓到的硬體／韌體 bug 能在實驗室**逐拍（cycle-by-cycle）精確重現**，我們需要「確定性模式（Deterministic Mode）」：

1. **確定性 CPU 與事件重播（Record & Replay）** `[09:30]`：
   - 提供專用除錯指令，在開始前把所有正反器歸零重設、清空快取與 TLB，並在除錯模式下關閉隨機置換與非確定性省電功能。
   - 用特規主機板把執行期間發生的非確定性外部事件（中斷、DVFS 切換、I/O 回應）連同**精確發生的 cycle 時間戳記全部記錄（log）下來**，回實驗室照表重播（replay）。
2. **確定性匯流排（Deterministic Buses）：一律等到最壞到達時間！** `[10:49]`
   假設傳送端與接收端時脈頻率相同但有鬆散相位差與傳輸抖動：當傳送端在自己的本地時間 $t_s$ 送出一筆訊息，接收端在自己的本地時鐘保證會在區間 $[t_s + \theta_1,\; t_s + \theta_2]$ 內收到（例如 $\theta_1 = 4,\; \theta_2 = 5$）。

```
  傳送端在本地時間 t_s 發送（隨封包附上 t_s 標籤）
         │
         ▼  經過帶抖動的匯流排 / NoC（可能 +4 拍就早到，最慢 +5 拍必到）
  ┌─────────────────────────┐
  │ 接收端提早於 t_s+4 收到 │ ──► 先塞進緩衝區按兵不動！一律等到接收端本地時鐘 = t_s + θ₂ (t_s+5) 才取出處理！
  └─────────────────────────┘
```

   - **效果**：把所有早到的訊息故意壓到**絕對最壞延遲 $t_s + \theta_2$** 才交給處理器，徹底消滅匯流排抖動帶來的非確定性！
   - `【課本補充】`（p.650）兩個實作改良（Sarangi et al., 2006a）：(a) 若兩端處於不同頻率域，只要頻率比值固定即可按比例換算；(b) 不必每筆封包都傳完整的 64-bit 週期計數器 $t_s$——既然相對時脈偏移被夾在有限區間 $[\theta_1, \theta_2]$ 內，只需傳最低幾個 LSB 位元即可！

---

## 3. 設計缺陷（Design Faults）：Pentium FDIV、AMD TLB 與微碼／軟體修補

[13:46]（課本 §12.4，p.651）硬體並不是神造的，現代處理器本質上就是**幾百萬行用 Verilog 或 VHDL 寫出來的大型程式**！人類寫 C++/Java 會寫出 bug，寫 Verilog 當然也會寫出**先天設計缺陷（design faults）**；更慘的是軟體有 bug 發個更新檔就修好了，矽晶片一旦光罩蝕刻出廠，裡面的硬體線路就永遠改不了！

[15:00] **歷史上兩次震驚業界的處理器設計缺陷**：
- **Intel Pentium FDIV 浮點除法錯誤**（⚠️ 影片 `[15:08]` 說 1995 年，課本 p.651 寫 1994 年並引 Pratt 1995 年論文——史實為 1994 年底爆發並提列損失、1995 年發表分析論文）：除法器內部查表（SRT lookup table）漏填了幾個項目，導致極少數浮點除法在小數點後第四位以後算錯；Intel 最終被迫免費全面召回更換，在當年就慘賠 **$4.75\text{ 億美元}$（折合今日逾十億美元）**！
- **AMD Barcelona 與 Phenom 的 TLB 死鎖錯誤** `[15:29]`：快取／TLB 特定互動條件下會害整顆處理器卡死（hang/lock up；見 §7 Linux 核心原始碼中的 `amd_erratum_383`！）。

[15:58] **微碼修補（Microcode Patching）**：現代 CISC 處理器（如 x86）會把複雜指令翻譯成儲存在韌體陣列裡的**微碼（microcode）**序列；若發現某條複雜指令硬體實作有 bug，廠商可透過 BIOS／OS 開機載入微碼更新（例如本機 i7-13650HX 的 `/proc/cpuinfo` 顯示已載入 `microcode : 0x133`），把該指令改導向一段較慢但正確的微碼序列，免去召回晶片的滅頂之災。

---

## 4. 設計期驗證（Verification）vs 矽後驗證（Validation）

[16:40]（課本 §12.4.1，p.651–653）硬體公司往往投入 **50%–60% 的研發時間與過半數工程師**專門抓錯。課本嚴格區分矽前與矽後兩個階段：

### 4.1 Verification（設計期驗證：晶片下線製造「前」）

[17:21] 檢查設計圖／RTL 是否符合規格。除了用 **ATPG（自動測試向量產生器）**在慢速 RTL 模擬器或快速 **FPGA 原型板**上衝高覆蓋率（coverage，課本 p.652）外，加法器、FPU 與關鍵協定會動用黃金標準——**形式驗證（Formal Verification）**，在數學上證明「實作電路 $\equiv$ 理想規格」對所有可能輸入皆成立：

| 形式驗證流派 | 運作方式 | 優缺點 |
|---|---|---|
| **Automated Theorem Proving（自動定理證明）** `[18:45]` | 從邏輯閘基本公理（axioms）出發，推導證明電路輸出滿足一組充分的數學性質（定理）。 | 數學保證最強，但極度仰賴專家手工技巧且非常慢。 |
| **Model Checking（模型檢驗，如 `SPIN`）** `[19:56]` | 用形式語言建立有限狀態機模型，從初始狀態自動窮舉搜尋是否會走到非法／死鎖狀態。 | **速度快且廣受歡迎；最大殺手鐧是抓到錯時會自動吐出「反例路徑（counterexample）」**（見 §8 實測），直接告訴工程師哪一串輸入會爆！ |

### 4.2 Validation（矽後驗證與特性分析：晶片製造出來「後」）

[22:13]（課本 p.652–653）拿到實體晶圓與封裝好的測試晶片後，依序執行五道關卡：
1. **Wafer-level probing（晶圓級探針測試）**：晶圓剛切好就紮探針灌測試向量，篩掉明顯做壞的晶粒（die）。
2. **Burn-in testing（高溫老化烤機測試）** `[22:42]`：`【口頭】` 故意把晶片加熱到 **$200\text{–}250\text{°C}$** 的極端環境灌測試向量——體質脆弱有先天瑕疵的晶片會提早陣亡，撐過來的才算可靠。
3. **Scan chain testing（掃描鏈測試）** `[23:19]`：晶片內絕大多數正反器都串在一條專用串列匯流排（**scan chain**）上，透過 **JTAG 埠** `[24:04]` 把已知初始位元一格格 shift 進去，跑 $K$ 個週期後再全部 scan 出來交給 EDA 工具逐顆正反器核對！
4. **電氣特性測試：$I_{DDQ}$ 與 Shmooing／Binning** `[24:23]`：
   - **$I_{DDQ}$ 測試**：讓晶片完全靜止不轉態（quiescent state），量測靜態供應電流 $I_{DD}$ 是否等於預期總漏電流；若異常暴衝，代表內部有**電源對地短路（short circuit）**！
   - **Shmooing 與 Binning（體質分級）**：掃描各種 $(V, f)$ 電壓–頻率組合畫出通過區（shmoo plot）——體質只能穩跑 $2.5\text{ GHz}$ 的就當平價版賣，能上 $3.0\text{ GHz}$ 的當高階賣（**binning**），同時為每一顆晶粒量身打造專屬的 **DVFS 電壓–頻率表**！
5. **Functional testing（功能測試）** `[25:18]`：插上特製驗證主機板（如 Intel 專用板），固定初始狀態後直接跑超長測試（例如**實際開機載入作業系統 boot an OS** `[25:42]`），核對外部行為。

---

## 5. 公開勘誤表（Errata）實例與兩套正交缺陷分類法

[26:30]（課本 §12.4.2、表 12.1，p.653–654）即便經過上述天羅地網，`【口頭】` **一顆市售量產處理器平均仍會漏掉 50–100 個 bug！** 廠商會定期出版公開的**勘誤表（Errata sheets）**供 BIOS、OS 與編譯器開發者避雷。課本表 12.1 列出三個真實勘誤案例：

| 編號 | 處理器 | 觸發條件與後果 | 可行的軟體／韌體繞道（Workaround） |
|---|---|---|---|
| **Fault 1** | **IBM PowerPC G3** | L1 cache 發生 miss、**同時** L2 cache 正在沖刷（flushing；⚠️ 影片 `[27:08]` 口頭簡說成 using L2）、且動態電源管理開啟 $\Rightarrow$ L2 資料可能毀損 | OS／BIOS 關閉動態電源管理（或下一版硬體錯開 L1 miss 與 L2 存取） |
| **Fault 2** | **Intel Pentium 4** | 命中處於 `M` 態的快取行、**同時**匯流排正在做 snoop 探詢、且有待處理的匯流排重新初始化（bus re-init）請求 $\Rightarrow$ **系統死鎖（Deadlock）** | 韌體在重新初始化匯流排期間暫時凍結快取／snoop 存取（見 §8 形式驗證實測！） |
| **Fault 3** | **AMD Athlon 64** | **4 條指令視窗內**出現兩條 `AAM`（adjust-after-multiply）指令，或 **7 條指令視窗內**出現 `DIV` 後面跟著 `AAM` $\Rightarrow$ ALU 算錯結果 | **編譯器（Compiler）**在產生機械碼時插入指令或重排，保證絕不生出該指令組合（已釋出的舊執行檔則需二進位修補） |

### 兩套正交（Orthogonal）的分類法

[29:48]（課本 p.653–654）：
- **分類軸一：依觸發條件結構（Sarangi et al., 2006b）**
  1. **Non-critical（非關鍵缺陷）**：出在效能計數器（performance counters）或除錯暫存器，不影響程式正確性（見 §7 `amd_erratum_1054`）。
  2. **Critical-concurrent（關鍵共時缺陷，如 Fault 1、Fault 2）**：影響正確性，且觸發條件是多個微架構布林事件（稱為 **signals**）在**同一個時脈週期同時為真**。
  3. **Critical-complex（關鍵複雜時序缺陷，如 Fault 3）**：條件之間有**跨週期的先後時序關係**（事件 A 發生後 $n$ 拍內發生事件 B），必須靠有限狀態機才能偵測，最難抓！
- **分類軸二：依根本成因（Constantinides et al., 2008）** `[32:02]`
  1. **Logic design faults**：RTL 程式碼手誤寫錯邏輯。
  2. **Algorithmic faults**：RTL 照規格寫對了，但高階演算法／協定規格本身想錯（`【口頭】` 例如規格本應向 $-\infty$ 捨入，設計師卻寫成向 $+\infty$ 捨入）。
  3. **Timing faults**：關鍵路徑時序分析漏算（例如週期設 $3.5\text{ ns}$，某條罕見路徑實際卻跑了 $4.0\text{ ns}$ 來不及鎖進 latch），需降頻或改版補正反器。

---

## 6. 訊號擷取修補（Signal Tapping）與 DfD 追蹤緩衝區壓縮／摘要

### 6.1 可程式化硬體補丁（Signal Tapping + PLA）

[34:10]（課本 p.654–655）如果在 BIOS 裡把整個 DVFS 或硬體預取「永久關掉」來避開 Fault 1，效能損失太冤枉！研究界提出預先在晶片高風險區（如電源管理與快取一致性模組）拉出重要布林事件腳位（**signals**，如 `L1_miss`、`L2_flush`、`DVFS_active`），接進一塊**可程式化邏輯陣列（PLA / programmable combinational hardware）**：
- 出廠後一旦發現某組共時條件 $S_1 \land S_3 \land S_8$ 會觸發新 bug，廠商只要燒錄更新 PLA 規則；執行期偵測到條件即將成立的瞬間，**短暫關掉其中一個開關幾拍、或觸發 pipeline flush／checkpoint 回滾**，就能無痛閃過硬體 bug！（課本 p.655 註記：因全晶片潛在訊號上千條、拉線成本高，截至 2020 年業界商用仍以微碼與韌體修補為主。）

### 6.2 原速除錯（At-Speed Debug）與 DfD 追蹤緩衝區摘要

[37:01]（課本 §12.4.3，p.655–656）傳統 scan chain 讀資料時必須把 CPU 停下來，做不到**原速除錯（at-speed debug）**。因此現代晶片內部會埋設 **DfD（Design-for-Debug）硬體**——一組環狀的**追蹤緩衝區（circular trace buffers）**，在晶片全速奔跑時持續側錄關鍵 signals，等當機時倒帶追查死因。但晶片一秒跑數十億拍，幾 KB 的緩衝區瞬間就會溢位覆寫！

⚠️ **影片與課本對 Spatial / Temporal 壓縮的互補視角**：
- **影片 `[38:38]–[40:05]` `【口頭】` 矩陣壓縮視角**：把 `各核心／功能單元（列）× 各類訊號（欄）` 排成矩陣——對單一核心軌跡內部做壓縮稱為 **row-wise / spatial**；跨多核心或跨時間消除重複欄位（例如所有核心都在同一 DVFS 狀態只存 1 bit）稱為 **column-wise / temporal**。
- **課本 §12.4.3（p.655–656）過濾摘要（Summarization）視角**：
  - **Spatial Summarization（空間摘要）**：工程師指定只錄感興趣的訊號子集，並設定起訖訊號特徵碼（signatures）只框出可疑活動區段。
  - **Temporal Summarization（時間摘要）**：跨緩衝區聯合過濾，更聰明的是**一旦處理器順利抵達某個預期正確狀態（例如平安進入睡眠模式），就把先前為了防範該階段出錯而暫存的所有軌跡事件直接清空釋放空間**！

---

## 7. 【原始碼】Linux x86 `cpu/amd.c` 如何替公開 CPU Errata 打作業系統補丁

對照 `[15:29]` 提到的 **AMD Family 10h（Barcelona／Phenom）TLB 錯誤**與 `[29:48]` 的**非關鍵效能計數器錯誤**，我們直接打開 Linux v6.2.12 的 `arch/x86/kernel/cpu/amd.c`，親眼見證作業系統開發者如何照著廠商公開的 Errata 文件寫出真實繞道補丁：

```c
/* os/linux-src/arch/x86/kernel/cpu/amd.c:726-734, 769-770, 998-1005 */
	/*
	 * Disable TLB flush filter by setting HWCR.FFDIS on K8
	 * bit 6 of msr C001_0015
	 *
	 * Errata 63 for SH-B3 steppings
	 * Errata 122 for all steppings (F+ have it disabled by default)
	 */
	msr_set_bit(MSR_K7_HWCR, 6);
/* ... (in init_amd_gh() for Family 10h Barcelona/Phenom): ... */
	if (cpu_has_amd_erratum(c, amd_erratum_383))
		set_cpu_bug(c, X86_BUG_AMD_TLB_MMATCH);
/* ... (in init_amd()): ... */
	/*
	 * Turn on the Instructions Retired free counter on machines not
	 * susceptible to erratum #1054 "Instructions Retired Performance
	 * Counter May Be Inaccurate".
	 */
	if (cpu_has(c, X86_FEATURE_IRPERF) &&
	    !cpu_has_amd_erratum(c, amd_erratum_1054))
		msr_set_bit(MSR_K7_HWCR, MSR_K7_HWCR_IRPERF_EN_BIT);
```

三處程式碼精準對應課本 §12.4.2 的每一句話：
1. **局部關閉硬體最佳化（Errata 63 / 122）**：第 733 行直接寫入硬體模型特定暫存器 `MSR_K7_HWCR` 的第 6 bit，關掉有 bug 的「TLB flush filter」——正如老師在 `[34:14]` 強調：「某個硬體特性必須被關掉，打破錯誤成立的條件之一」。
2. **Barcelona／Phenom 的 TLB 勘誤（Erratum 383）**：第 769 行檢查步進（stepping）與 OSVW（OS Visible Workaround）暫存器，標記 `X86_BUG_AMD_TLB_MMATCH` 讓虛擬記憶體子系統避開混用大頁與小頁同位址快取的死鎖陷阱。
3. **Non-critical 缺陷實例（Erratum 1054）**：第 1000 行正是課本 p.653 所分類的「效能計數器不準確（Instructions Retired Performance Counter May Be Inaccurate）」——不傷程式正確性，Linux 只要在受影響晶片上跳過啟用該免費計數器即可。

---

## 8. 【實測】本機執行期非確定性、匯流排確定化、Model Checking 抓反例與 DfD 壓縮

我們撰寫 [`experiments/lecture50/nondet_and_verification.py`](experiments/lecture50/nondet_and_verification.py)，在本機 **i7-13650HX** 上驗證三個核心主題：(1) 綁定 `cpu0` 連續執行 12 次完全不含亂數的純算術迴圈，觀察外部輸出、記憶體位址（ASLR）與執行時間的非確定性，並測試 $t_s + \theta_2$ 匯流排緩衝；(2) 針對課本表 12.1 **Fault 2（`M` 態快取＋Snoop＋Bus re-init 死鎖）** 寫一個 **BFS Model Checker**，測試未修補、天真修補與完整韌體修補；(3) 模擬 512-byte DfD 環狀追蹤緩衝區結合空間與時間摘要後的回溯視窗放大倍率：

```python
# experiments/lecture50/nondet_and_verification.py:55-84
def model_check_erratum(mode="unpatched"):
    # State = (line_state in {'I','S','M'}, snoop_active in {0,1}, bus_reinit in {0,1})
    start = ("I", 0, 0)
    def next_states(st):
        ls, snp, reinit = st
        # Fault 2 (Table 12.1): M-hit & snoop_active & bus_reinit simultaneously -> DEADLOCK (no outgoing edges!)
        if ls == "M" and snp == 1 and reinit == 1:
            return []
        nxt = []
        if not (mode == "full_fix" and reinit == 1):  # [28:08] freeze accesses while bus re-initializes
            for new_ls in ("I", "S", "M"):
                nxt.append((f"cpu_to_{new_ls}", (new_ls, snp, reinit)))
            nxt.append(("snoop_toggle", (ls, 1 - snp, reinit)))
        if mode == "unpatched" or (mode == "naive_fix" and not (ls == "M" and snp == 1)) or (mode == "full_fix" and snp == 0):
            nxt.append(("bus_reinit_req", (ls, snp, 1)))
        nxt.append(("bus_reinit_done", (ls, snp, 0)))
        return nxt
    # ... BFS explores state graph and returns shortest path to len(edges) == 0 ...
```

### 實測輸出

```text
=== 1) Inherent Nondeterminism vs Deterministic Bus Buffering ===
Real CPU (12 runs): distinct outputs=1 | distinct stack addrs=12/12 | time=3638.6..4093.2 us (std=139.7 us)
Raw bus (jitter 3..5 cyc) : identical internal trace=False | avg latency=4.06 cyc
Buffered to ts+theta2 (5) : identical internal trace=True  | avg latency=5.00 cyc

=== 2) BFS Model Checking on Fault-2-style Snoop/Bus-Reinit FSM ===
  unpatched : explored 12 states -> Counterexample: [('cpu_to_M', ('M', 0, 0)), ('snoop_toggle', ('M', 1, 0)), ('bus_reinit_req', ('M', 1, 1)), ('DEADLOCK_AT', ('M', 1, 1))]
  naive_fix : explored 12 states -> Counterexample: [('cpu_to_M', ('M', 0, 0)), ('bus_reinit_req', ('M', 0, 1)), ('snoop_toggle', ('M', 1, 1)), ('DEADLOCK_AT', ('M', 1, 1))]
  full_fix  : explored  9 states -> Counterexample: None

=== 3) DfD 512-Byte Trace Buffer Effective Lookback Window ===
Uncompressed raw signals: 128 cycles | Spatial + Temporal summarization: 1590 cycles (12.4x window)
```

### 三項關鍵觀察

1. **外部確定 vs 內部非確定性，以及 $t_s + \theta_2$ 的代價**：本機 12 次執行算出的整數結果 100% 唯一（`distinct outputs=1`），但每次位址空間布局都不同（`12/12` 全異），執行時間也在 $3638.6\text{–}4093.2\ \mu\text{s}$ 間跳動逾 $450\ \mu\text{s}$（呼應 `[01:48]`）。在抖動為 $3\text{–}5$ 拍的匯流排模擬中，緩衝至最壞時間 $t_s + 5$ 僅多付 **$0.94$ 拍平均延遲**（$4.06 \to 5.00\text{ cyc}$），就讓兩次獨立執行的內部收訊序列從分岔（`False`）變成**百分之百逐拍一致（`True`）**！
2. **Model Checking 自動吐反例抓出人類盲點的威力**：
   - 未修補時，Model Checker 只花 3 步就找出死鎖反例：`cpu_to_M → snoop_toggle → bus_reinit_req`。
   - 如果人類工程師天真地只補一條規則「當 `M` 且 `snoop` 時禁止發 `bus_reinit_req`」（`naive_fix`），**Model Checker 立刻抓出另一條人類漏想的交錯反例**：先發 `bus_reinit_req` 進入 `('M', 0, 1)`，接著外部 `snoop_toggle` 殺進來照樣死鎖在 `('M', 1, 1)`！
   - 照著老師 `[28:08]` 所說「匯流排重設期間凍結存取、且無 snoop 時才准重設」（`full_fix`）後，Model Checker 窮舉全部可達狀態回報 `Counterexample: None`，數學上證明死鎖徹底不可達！
3. **DfD 追蹤視窗放大 $12.4\times$**：小小的 512-byte 環狀緩衝區原本錄 4 核 × 8 訊號只能存 **128 拍**就會被覆蓋；加上欄位空間壓縮與「平安睡著驗證通過即清空（Temporal Summarization）」後，可回溯視窗暴增到 **1,590 拍（$12.4\times$）**，大幅提高抓到跨拍複雜錯誤的機率。

---

## 9. 一頁速查表

| 主題 | 核心問題 | 關鍵解法／工具 | 代價與限制 |
|---|---|---|---|
| **固有非確定性** | 隨機置換、DVFS/Gating、DRAM refresh/scrub、匯流排漂移讓 bug 無法重現 | 重設正反器／快取 ＋ 中斷時間戳記 Log & Replay ＋ 匯流排緩衝至 **$t_s + \theta_2$** | 匯流排平均延遲上升至最壞上界；需傳時間標籤 |
| **設計期 Verification** | 晶片下線前抓出 RTL／協定邏輯錯誤（佔 50–60% 研發時間） | **Theorem Proving**、**Model Checking**（吐反例！）、ATPG + RTL/FPGA 模擬 | 形式驗證對超大型狀態空間極慢，僅能驗子模組 |
| **矽後 Validation** | 晶片出廠後抓製造缺陷、短路與體質差異 | Wafer probe、**Burn-in**（200–250°C）、**Scan chain**（JTAG）、**$I_{DDQ}$**、**Shmooing/Binning** | Scan chain 必須停下時脈，無法做原速（at-speed）除錯 |
| **缺陷分類（兩軸）** | 釐清勘誤表（50–100 個漏網 bug）的性質 | 軸一：Non-critical / **Critical-concurrent** / **Critical-complex**<br>軸二：Logic / Algorithmic / Timing | Critical-complex 具跨拍時序相依，最難偵測與繞道 |
| **矽後修補與 DfD** | 晶片已賣出，如何在不召回（省數億美元）下避開 bug 與抓現場死因？ | BIOS/OS 關特性、編譯器避開指令組合、**Microcode 更新**、Signal Tapping + PLA、環狀 **Trace Buffer + 空間／時間摘要** | 軟體繞道常犧牲效能；全晶片拉線成本高，緩衝區容量有限 |

---

## 10. 與後續講次的連結

- **第五十一講（Ch12 Reliability Part III：製程變異與硬體老化）**：本講 §4.2 談到的 Burn-in 高溫烤機（為何高溫能加速篩出早夭瑕疵？）與 Shmooing/Binning（為何同一片晶圓切出來的晶片最高安全頻率都不同？），將在下一講得到完整的物理與統計解答——從 193 nm 微影繞射造成的**製程變異（process variation）**，一路推導到四大永久老化機制（HCI、NBTI、TDDB、電遷移）與**浴缸曲線（bathtub curve）**！

---

## 附：本講一句話總結

> 為了制服讓現場臭蟲無法重現的硬體非確定性與動輒價值數億美元的設計缺陷，現代處理器在矽前靠形式模型檢驗抓反例、在矽後靠烤機／掃描鏈／Shmoo 分級把關、在除錯模式靠 $t_s + \theta_2$ 最壞延遲緩衝鎖定逐拍狀態，並在出廠後聯手微碼、OS／編譯器勘誤補丁與壓縮追蹤緩衝區守住最後一道防線。

---

*筆記整理自 Smruti R. Sarangi 教授 Advanced Computer Architecture 第 50 講影片與 Next-Gen Computer Architecture v3.2 第 12 章 §12.3–12.4，搭配 Linux v6.2.12 原始碼與本機實測驗證。*
