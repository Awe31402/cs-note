# Advanced Computer Architecture 第四十九支影片完整筆記：可靠度（一）：Fault／Error／Failure、軟錯誤物理（Hazucha–Svensson）、遮罩機制、DIVA Checker 與電感雜訊（Chapter 12, Part I）

> **課程**：Advanced Computer Architecture — Chapter 12: Reliability (Soft errors and inductive noise) -- Part I/III（播放清單第 49 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕下載不到）（1:01:10）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch12** §12.1–12.2（p.637–649）
> **章號**：影片 Chapter 12 = 課本 Ch12。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10、NumPy 1.26）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 把「processors」寫成「processes」、「fail-stop」寫成「failed stop」、「hole」寫成「whole」、「Svensson」寫成「Swenson」、「$Q_{\text{crit}}$」寫成「Q-crate」、「soft-error resilience／strike」寫成「software resilience／strike」、「ECC」寫成「error checking ports」、「MSB」寫成「MSP」、「scot-free」寫成「squat free」、「`COMM`／`COMP`」都寫成「COM」，筆記直接改正；另 `[61:01]–[61:28]` 為片尾音樂期間 whisper 幻覺出的亂碼句子，予以略過。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 可靠度基本定義：Fault、Error、Failure 與 FIT／MTTF／MTBF／MTTR](#1-可靠度基本定義faulterrorfailure-與-fitmttfmtbfmttr)
- [2. 軟錯誤（Soft Errors）物理：輻射源、LET 與 Hazucha–Svensson 模型](#2-軟錯誤soft-errors物理輻射源let-與-hazuchasvensson-模型)
- [3. 元件級與電路級防護：SOI、Triple-Well 與三大天然遮罩機制](#3-元件級與電路級防護soitriple-well-與三大天然遮罩機制)
- [4. 架構級分析：TVF、AVF 與 DUE vs SDC 分類樹](#4-架構級分析tvfavf-與-due-vs-sdc-分類樹)
- [5. Checker 架構設計空間：MultiMaster（IBM G5／HP NonStop）、SingleSlave（DIVA）與 MultiSlave](#5-checker-架構設計空間multimasteribm-g5hp-nonstopsingleslavediva與-multislave)
- [6. 電感雜訊（Inductive Noise）：RLC 電源網格共振與 Pipeline Damping](#6-電感雜訊inductive-noiserlc-電源網格共振與-pipeline-damping)
- [7. 【原始碼】Linux x86 `mce/severity.c` 如何區分已修正錯誤、DUE 與 Fail-Stop Panic](#7-原始碼linux-x86-mceseverityc-如何區分已修正錯誤due-與-fail-stop-panic)
- [8. 【實測】Parity vs SECDED 遮罩率、矩陣不變量檢查加速與 50 MHz RLC 共振阻尼](#8-實測parity-vs-secded-遮罩率矩陣不變量檢查加速與-50-mhz-rlc-共振阻尼)
- [9. 一頁速查表](#9-一頁速查表)
- [10. 與後續講次的連結](#10-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
硬體並非絕對可靠（大型超級電腦每幾小時就會遇上一次暫態故障）
  │
  ├── 基本語彙：Fault（缺陷）─► Error（錯誤內部狀態）─► Failure（外部可見異常：Fail-stop vs Byzantine）
  │   量化指標：1 FIT = 10⁹ 小時一次故障；MTTF（不可修）、MTBF（可修兩次故障間隔）、MTTR（修復時間）
  │
  ├── 暫態故障一：軟錯誤（Soft Errors / SEU，§12.1）
  │   ├── 物理成因：封裝鈾／釷、²¹⁰Pb、BPSG 介電層 ¹⁰B（受中子撞擊分裂）、宇宙射線中子
  │   │             LET > 20 MeV·cm²/mg → 電場漂移（drift）+ 擴散（diffusion）產生 200–600 ps 電流脈衝
  │   │             Hazucha–Svensson：SER = F × CS，其中 CS ∝ A · exp(−Q_crit / Q_S)
  │   ├── 元件級（Radiation Hardening）：純化材料、加大電晶體／V_dd／電容提高 Q_crit、Triple-well 與 SOI 壓低 Q_S
  │   ├── 電路級：ECC + 三大遮罩（Electrical 衰減、Logical 邏輯遮蔽、Timing-window 非 setup/hold 視窗）
  │   └── 架構級：SE_fail = SER × TVF × AVF；未修正錯誤分為 DUE（偵測但無法救）與最危險的 SDC（無聲資料毀損）
  │         Checkers（覆蓋度：Complete / Subset / Invariant / Symptom）：
  │           • MultiMaster：IBM G5（雙管線鎖步逐拍比對 flush）、HP NonStop（只在 I/O 比對，DMR 回滾或 TMR 投票）
  │           • SingleSlave（DIVA）：「驗證比計算快」— 胖 OOO 主核配 2-stage 循序從核（COMM + COMP + L0 + 私有 SQ/RF）
  │           • MultiSlave：主核 IPC 太高時切 epoch 交給多顆慢速 checker 輪流平行驗證
  │
  └── 暫態故障二：電感雜訊（Inductive Noise / dI/dt Noise，§12.2）
      電源網格等效為 RLC 電路（共振頻率 f₀ ≈ 10–100 MHz）；程式迴圈頻率撞上 f₀ 會使阻抗達極值、去耦電容失效並引發劇烈電壓尖峰
      → Pipeline Damping：後端計數器估算每 epoch 電流，高谷節流（throttling）＋ 低谷塞虛假指令（dummy work）撫平振幅
```

---

## 1. 可靠度基本定義：Fault、Error、Failure 與 FIT／MTTF／MTBF／MTTR

[00:19] 過去十幾章我們都假設硬體百分之百照規格運作，但真實世界並非如此：`【口頭】` 大型現代超級電腦或數萬台機器的資料中心，**平均每三、四個小時就會發生一次暫態故障**。硬體故障按生命週期分為兩大類（課本 p.638）：
- **暫態故障（Transient faults）**：短暫出現幾百 ps 到幾個 cycle 就消失（本講的軟錯誤與電感雜訊，以及下一講的非確定性故障）。
- **永久故障（Permanent faults）**：一旦出現就不會自己消失，又分**先天缺陷（congenital，如 RTL 設計 bug、製程變異）**與**老化磨損（ageing / wear-out，如電晶體或導線隨時間斷路／短路）**。

### 1.1 Fault、Error 與 Failure 的嚴格區分

[02:00]（課本 Definition 98，p.637–638）這三個詞日常混用，但在可靠度工程裡層次分明：

```
  Fault（缺陷）       ──不一定觸發──►  Error（內部狀態錯誤）  ──不一定外洩──►  Failure（外部可見失效）
  硬體／軟體偏離規格                   被執行到而產生錯誤暫存器               輸出算錯、當機或無回應
  （例如一顆電晶體被打中）              （例如綠燈沒亮變紅燈、R1 值錯）        （例如印表機印錯帳單、系統崩潰）
```

- **Fault 未必變成 Error**：缺陷所在路徑沒被執行到就被遮蔽（masked）。
- **Error 也未必變成 Failure**：`【課本補充】`（p.637）例如某暫存器 `R1` 的值翻錯了（Error），但下一條指令剛好是 `DIV R2, R1, R1`（自己除以自己恆得 `1`，只要不變 `0`），最終外部輸出完全正確，就不構成 Failure！
- **Failure 的兩種行為模式** `[03:53]`：
  1. **Fail-stop（失效即停止）**：最常見也最理想——子系統偵測到不對勁就直接停機，不往外吐垃圾。
  2. **Byzantine failure（拜占庭失效）**：處理器或節點不但不停機，還像中了病毒或惡意串通（collude）一樣，對其他節點發送前後矛盾、混淆視聽的錯誤訊息，極難偵測與修復（在純硬體系統較罕見，多見於分散式／安全攻擊情境）。

### 1.2 四大可靠度量化指標

[04:57]（課本 Definition 99，p.638）：
- **FIT（Failures In Time）**：每 **$10^9$（十億）運轉小時**預期發生的失效次數（例如 `100 FIT` = 十億小時壞 100 次）。
- **MTTF（Mean Time To Failure，平均失效時間）**：假設系統**不可修復（non-repairable）**，從啟用到壞掉的期望時間。
- **MTBF（Mean Time Between Failures，平均失效間隔）**：假設系統**可修復（repairable）**，兩次失效之間的期望時間（$\text{MTBF} = \text{MTTF} + \text{MTTR}$）。
- **MTTR（Mean Time To Repair，平均修復時間）**：失效發生後，修復狀態並重新上線所需的期望時間。

---

## 2. 軟錯誤（Soft Errors）物理：輻射源、LET 與 Hazucha–Svensson 模型

### 2.1 四大輻射粒子來源

[07:08]（課本 §12.1.1，p.638–639）1960–70 年代工程師發現核設施周邊與太空任務的電子設備常無故當機，1980 年代連海平面的 DRAM 也開始出錯，微縮後更蔓延到 SRAM 與邏輯閘。這類由高動量粒子撞擊引發、不損壞硬體結構卻把位元從 $0 \leftrightarrow 1$ 翻轉的暫態錯誤稱為**軟錯誤（soft errors）**或**單一事件翻轉（SEU, Single-Event Upset）**：

1. **封裝材料中的鈾（Uranium）與釷（Thorium）微量雜質** `[08:27]`：自發衰變射出 $\alpha$ 粒子（2 質子 + 2 中子，質量大），能量高達 **$4\text{–}9\text{ MeV}$**；`【課本補充】`（p.639）其在矽中的最大穿透深度約 **$100\ \mu\text{m}$**，所以只有封裝「內部」雜質射出的 $\alpha$ 粒子打得到晶片，封裝外的擋得住。
2. **不穩定鉛同位素 $^{210}\text{Pb}$（錫球焊料雜質）** `[09:03]`：⚠️ 課本 p.639 寫 $^{210}\text{Pb}$ 衰變為發射 $\alpha$ 粒子的 $^{210}\text{Po}$，影片 `[09:24]` 則直接講到整條衰變鏈的穩定終點 $^{206}\text{Pb}$（$^{210}\text{Pb} \to {}^{210}\text{Bi} \to {}^{210}\text{Po} \xrightarrow{\alpha} {}^{206}\text{Pb}$），兩者描述的是同一條衰變鏈。
3. **介電層 BPSG 中的硼同位素 $^{10}\text{B}$（二次輻射）** `[09:50]`：p 型摻雜與介電材料 **BPSG（boron phosphosilicate glass，硼磷矽玻璃）**含有稀有同位素 $^{10}\text{B}$。⚠️ 影片口頭簡稱為產生 $\alpha$ 粒子，課本 p.639 完整寫出它是**二次輻射源**：當外來中子撞擊 $^{10}\text{B}$ 原子核，會把它分裂成一顆 **$\alpha$ 粒子**與一顆**鋰原子核（$^7\text{Li}$）**往反方向飛出，兩者都能翻轉位元！
4. **宇宙射線中子（Cosmic Neutrons）** `[10:07]`：隨製程純化技術進步，今天地表與高空最主要的軟錯誤元凶是來自銀河系的宇宙射線中子——**海拔越高（高山機房、9–11 km 民航機、無大氣層保護的太空船 `[20:20]`、`[36:53]`），中子通量與軟錯誤率就呈倍數飆升！**

### 2.2 粒子撞擊動力學：LET、漂移與擴散電流脈衝

[10:52]（課本圖 12.1、式 12.1–12.2，p.639–640）粒子撞進矽基板時沿路損失能量，定義單位距離的能量損失率除以材料密度為 **LET（Linear Energy Transfer）**。當：
$$\text{LET} > 20\ \text{MeV}\cdot\text{cm}^2/\text{mg}$$
時（特別是打中處於關閉狀態 off-state 電晶體的**空乏區 depletion region** 周圍 $2\text{–}3\ \mu\text{m}$ 內），撞出的大量電子–電洞對（electron-hole pairs）會經歷兩階段收集：
1. **Drift（電場漂移階段，前幾百 ps）**：空乏區強電場迅速拉開電荷，衝出高聳陡峭的主電流尖峰。
2. **Diffusion（濃度擴散階段，長尾）**：多餘載子從高濃度往低濃度緩慢擴散，形成長尾巴。

整道電流脈衝持續約 **$200\text{–}600\text{ ps}$**（影片舉例 $3\text{ GHz}$ 時脈週期才 **$333\text{ ps}$**，脈衝寬度幾乎跟整個時脈週期一樣長！）。文獻常用兩種經驗公式近似電流波形 $I(t)$：
- **雙指數模型（Wang & Agrawal, 2008，式 12.1）**：
  $$I(t) = \frac{Q_{\text{total}}}{\tau_\alpha - \tau_\beta}\left(e^{-t/\tau_\alpha} - e^{-t/\tau_\beta}\right)$$
  其中總位移電荷 $Q_{\text{total}} \in [50, 500]\text{ fC}$（femtocoulombs），$\tau_\alpha \approx 164\text{ ps}$ 為取決於電晶體特性的電荷收集時間常數，$\tau_\beta \approx 50\text{ ps}$ 取決於粒子入射軌跡。
- **單參數模型（Hazucha & Svensson, 2000，式 12.2）**：$I(t) = \frac{2}{\eta\sqrt{\pi}}\sqrt{\frac{t}{\eta}}\,e^{-t/\eta}$，僅用單一經驗參數 $\eta$ 控制形狀。

### 2.3 Hazucha–Svensson 軟錯誤率模型

[16:38]（課本式 12.3–12.4，p.640–641）單算 $Q_{\text{total}}$ 還不夠，因為輸出端電容會吸收脈衝。以標準 6T SRAM cell 為基準，**Hazucha–Svensson 模型**給出軟錯誤率（單位 FIT）：
$$\text{SER} = F \times CS, \qquad CS \propto A \times e^{-Q_{\text{crit}} / Q_S}$$
- $F$：粒子通量（flux，單位面積每秒有效撞擊率，隨海拔上升）。
- $CS$（Critical Section）：真正脆弱易受攻擊的等效面積；$A$ 為電路敏感區面積。
- $Q_{\text{crit}}$（Critical Charge，臨界電荷）：把記憶胞位元翻轉所需的最小位移電荷量。
- $Q_S$（Charge Collection Efficiency，電荷收集效率，單位 $\text{fC}$）：電路把粒子撞擊轉成電流脈衝的效率。
  - 極限檢驗：若 $Q_S \ll Q_{\text{crit}}$（幾乎收集不到電荷），$e^{-Q_{\text{crit}}/Q_S} \to 0 \Rightarrow CS \to 0$（免疫）；若 $Q_S \gg Q_{\text{crit}}$，$e^0 = 1 \Rightarrow CS \propto A$（整塊面積一打就翻）。`【課本補充】`（p.641）原論文是在 $600\text{ nm}$ 製程實測標定，每換新世代製程都需重新用元件模擬或中子束轟擊實驗（neutron beam irradiation）校準。

---

## 3. 元件級與電路級防護：SOI、Triple-Well 與三大天然遮罩機制

### 3.1 元件級抗輻射強化（Radiation Hardening）

[21:20]（課本 §12.1.2、圖 12.2，p.641–642）看著公式 $CS \propto A e^{-Q_{\text{crit}}/Q_S}$，晶圓廠有三招：
1. **消除輻射源**：純化封裝與焊料去除 U、Th、$^{210}\text{Pb}$；停用含 $^{10}\text{B}$ 的 BPSG 介電層（或只放在遠離矽晶層的上層金屬）。
2. **拉高 $Q_{\text{crit}}$**：加大電晶體尺寸、提高供應電壓 $V_{dd}$、或加大輸出電容——代價是耗電與面積暴增、時脈變慢（一般商用晶片不愛，但**航太／太空船處理器**必用 `[22:32]`；反過來說，先進製程不斷微縮正讓 $Q_{\text{crit}}$ 越來越小！）。
3. **壓低 $Q_S$（減少電荷收集體積）**：
   - **Triple-well 製程**（圖 12.2a）：在 p 基板下方多埋一層深層 **buried n-well**，阻擋深處基板產生的載子往上收集，但電晶體體積會變大。
   - **SOI（Silicon-on-Insulator，絕緣層上覆矽）製程**（圖 12.2b）：直接在通道下方墊一層**埋入式氧化絕緣層（buried oxide）**，把電晶體本體與下方基板徹底隔離！既能做出緊湊電晶體，又大幅砍掉電荷收集體積 $Q_S$。

### 3.2 電路級：三大天然遮罩機制（Masking）與敏感路徑強化

[23:59]（課本圖 12.3，p.642–643）在記憶體陣列（DRAM／SRAM）裡，加 **ECC（錯誤更正碼）或同位元（parity）**就能輕鬆抓錯；但在組合邏輯（combinational logic）裡，粒子打中邏輯閘產生的電流脈衝必須**一路闖過三道關卡**才會真正翻轉後端管線閂鎖器（pipeline latch）的位元：

```
  (a) Electrical Masking       (b) Logical Masking          (c) Timing-Window Masking
   脈衝經過多級邏輯閘逐級衰減     AND 閘另一腳為 0，脈衝被吃掉   脈衝沒落在負緣前後的 [setup, hold] 視窗內
   ──/\──►[Gate]─►[Gate]─►─..   ──/\──┐                      Clk  ──────┐      ┌──────
                                  0 ──┴─[AND]──► 恆為 0                 └──────┘
                                                             Data ──/\──────[setup|hold]── (安全!)
```

1. **Electrical Masking（電氣遮罩）** `[24:07]`：脈衝穿過一串邏輯閘時被閘極電容與電阻濾波衰減，到 latch 前已微弱到翻不動位元。
2. **Logical Masking（邏輯遮罩）** `[24:59]`：例如 `AND` 閘另一個輸入固定是 `0`（或 `OR` 閘另一輸入是 `1`），受撞擊腳位就算短暫從 $0 \to 1$，邏輯輸出依然紋風不動。
3. **Timing-Window Masking（時序視窗遮罩）** `[25:44]`：管線 latch 只在時脈負緣（或觸發緣）前後極窄的 **setup time 到 hold time 視窗**內取樣資料；脈衝若提早結束或太晚才到，鎖進去的仍是正確值！
4. **敏感路徑強化（Sensitive Paths）** `[27:28]`：找出缺乏上述三種遮罩的脆弱邏輯路徑，只針對這些路徑局部加大電晶體、電容或電壓以提高 $Q_{\text{crit}}$。

---

## 4. 架構級分析：TVF、AVF 與 DUE vs SDC 分類樹

[28:30]（課本 §12.1.3、式 12.5、圖 12.4，p.643–644）就算脈衝真的翻轉了某個 latch 或 SRAM 位元，最終導致程式外部失效的機率 $\text{SE}_{\text{fail}}$ 還要再打兩次折：
$$\text{SE}_{\text{fail}} = \text{SER} \times \text{TVF} \times \text{AVF}$$
- **TVF（Timing Vulnerability Factor，時序脆弱因子 $\in [0, 1]$）**：該單元在被打中的那一拍根本沒在用（例如跑純整數程式時 FPU 被打中，或單元正處於休眠）。
- **AVF（Architectural Vulnerability Factor，架構脆弱因子 $\in [0, 1]$）**：位元雖然在活躍單元裡翻轉了，但**最終會不會影響架構正確性**？
  - **$\text{AVF} = 0$ 的免疫結構**：**分支預測器（branch predictor）**、預取緩衝區（prefetch buffer）、效能計數器（performance counters）——裡面的位元翻錯頂多猜錯分支慢幾拍，絕不會算錯答案！
  - **動態無效狀態**：在錯誤分支路徑（wrong-path）上稍後會被整個 flush 掉的指令 `[30:14]`、動態死碼（dynamically dead instructions）、或 64-bit 暫存器裡程式根本沒讀到的高階位元組 `[30:49]`，翻錯也全身而退（scot-free）。

[31:18]（課本圖 12.4，p.644）**AVF 決策樹：區分無錯誤、DUE 與 SDC**

```
                [ 發生位元翻轉 (Bit flip) ]
                            │
                     該位元有被讀取嗎？
                     ↙ No         ↘ Yes
               [ No error ]     有受 Parity / ECC 保護嗎？
                                 ↙ Yes                ↘ No
                         只能偵測、無法更正嗎？       該位元會影響最終架構輸出嗎？
                         ↙ Yes         ↘ No (已更正)    ↙ Yes              ↘ No
                    【 DUE 】        [ No error ]    【 SDC 】          [ No error ]
          (Detected but Unrecoverable)         (Silent Data Corruption)
```

- **DUE（Detected but Unrecoverable Error）**：例如只加 1-bit parity（知道錯了卻不知錯哪一 bit），或 SECDED 遇上罕見的 2-bit 同時翻轉——系統知道出事了，可以觸發例外停機或回滾。`[34:04]` `【口頭】` 若記憶結構已配備強 ECC（例如能修 1–2 bits），要在同一時間視窗同一筆資料連翻 3 bits 極度罕見，這時設計者真正害怕的反而是沒保護結構（如部分暫存器、旗標或內部閂鎖器）引發的 **SDC**！
- **SDC（Silent Data Corruption，無聲資料毀損）**：完全沒警報、程式也沒崩潰，卻默默吐出錯誤存款餘額或錯誤導航座標——這正是太空船失事與大型銀行主機最恐懼的頭號大敵！

---

## 5. Checker 架構設計空間：MultiMaster（IBM G5／HP NonStop）、SingleSlave（DIVA）與 MultiSlave

[36:25]（課本 §12.1.3，p.644–648）為了攔截邏輯電路與未保護結構的 SDC，架構師設計了專門的檢查器（Checkers，分類法出自 Kalayappan & Sarangi, 2013）。

### 5.1 維度一：依檢查覆蓋度（Coverage）分四類

[37:32]
1. **Complete（完全檢查）**：每一條指令、每一次運算全部重驗（大型主機最常用，最安全也最貴）。
2. **Subset（子集檢查）**：抽查部分關鍵指令。
3. **Invariant（不變量檢查）** `[37:49]`：不重算整個問題，只檢查輸出是否滿足數學恆等式！例如花大把時間算完大矩陣反演 $x = A^{-1} b$，檢查器只需快速算幾列（或整條）$A x$ 看是否等於 $b$（見 §8 實測，快上近 **500 倍**！）。
4. **Symptom（症狀檢查）** `[39:09]`：反過來盯「不該出現的紅旗警訊」——例如正常區段突然冒出非預期的算術例外或 segmentation fault（非法記憶體存取），就推論發生了位元翻轉。

### 5.2 維度二：依硬體拓樸分三類（MultiMaster / SingleSlave / MultiSlave）

#### (A) MultiMaster 系統：對稱冗餘執行

[39:51]（課本 p.645–646）跑兩份或三份一模一樣的主執行緒：
- **DMR（Dual Modular Redundancy，雙模組冗餘）＋ Backward Error Recovery（向後回滾）**：兩份結果不合時，**無法判斷誰對誰錯**，只能把兩邊一起退回上一個安全的**檢查點（checkpoint，可直接複用[第四十講](lecture40_note.md)硬體交易記憶體的檢查點機制）**重跑；因為軟錯誤是暫態脈衝，重跑幾乎必過。
- **TMR（Triple Modular Redundancy，三模組冗餘）＋ Forward Error Recovery（向前多數決投票）**：高輻射環境（飛機、太空船）嫌回滾太慢，直接跑三份做 **2-out-of-3 投票（voting）**，少數服從多數並把正確狀態拷貝給算錯的那一顆，完全不必停頓回滾！
- **兩大商用經典實作對比**：
  - **IBM S/390 G5** `[42:37]`：晶片內建**兩條鎖步（lockstep）平行的管線**，**每一個 cycle** 比對所有暫存器寫入與 store 值；一不合立刻 flush 兩條管線重來，只准驗證過的值寫進 L1 cache。缺點：100% 硬體面積代價，且現代複雜多核心＋NoC 極難維持跨核心逐拍鎖步同步。
  - **HP NonStop 伺服器** `[43:22]`：放棄逐拍鎖步，允許錯誤暫時流進各自獨立位址空間的記憶體，**只在對外界發動 I/O 時才停下來比對輸出**！（⚠️ 影片 `[43:49]` 直接介紹其 TMR 三路投票版，課本 p.645–646 則先講雙核 DMR + checkpoint 回滾再延伸到 TMR 投票；實機兩種組態皆有支援。）大幅省下每拍通訊頻寬並榨出完整多核吞吐量。

#### (B) SingleSlave 系統：DIVA 動態驗證核心（Austin, 1999）

[44:28]（課本圖 12.5，p.646–647）能不能不要花 100% 面積複製整顆肥大的亂序（OOO）核心？**DIVA** 的神來一筆建立在一個洞察：**「給定運算元與答案，驗證永遠比從頭找相依性計算快得多！」**
- 肥大的 **Out-of-Order 主核心**照常做分支預測、rename、wakeup-select（SPECint 平均實際 IPC 其實只有 **$1.5\text{–}2$**），但在指令準備 commit 前，把 `(PC, opcode, 運算元值, 計算結果)` 打包塞進**未驗證指令佇列（queue of unverified instructions）**，送給旁邊一顆用**大尺寸抗輻射電晶體**打造的極簡 **2-stage 循序（in-order）雙發射 DIVA 從核心**：

```
  ┌──────────────┐   (PC, op, src, res)   ┌─────────────────────────────┐
  │ 胖 OOO 主核心 ├──────────────────────►│   DIVA 2-stage 循序從核心    │
  │ (放大 ROB/SQ) │   未驗證指令緩衝佇列     │  Stage 1 (平行雙子管線)：     │
  └──────────────┘                        │    ├─ COMM：查私有 Checker RF│
                                          │    │        + L0 cache + 私有 SQ
                                          │    └─ COMP：專用 ALU 重算比對│
                                          │  Stage 2：Commit 寫入正式狀態│
                                          └─────────────────────────────┘
```

- **`COMM`（通訊驗證子階段）**：主核心傳來的運算元有沒有在暫存器或快取裡讀錯？DIVA 自己維護一份乾淨的 **Checker RF（架構暫存器檔副本）**、減輕 L1 壓力的微型 **L0 cache**，以及一份**私有 Store Queue 副本**（暫存尚未 commit 的 store 供後續 load 轉發），重讀一次核對！
- **`COMP`（計算驗證子階段）**：與 `COMM` **同時平行進行**！拿主核心餵進來的運算元在專用 ALU 重算一次，看跟主核心算出的 `res` 是否一致。完全不必做 wakeup/select/rename！
- 若兩邊都吻合才正式 commit 寫入記憶體；一旦抓包不合就 flush 重跑。代價是主核心的 ROB 與 Store Queue 必須加大以容納排隊等驗證的指令。

#### (C) MultiSlave 系統

[51:01]（課本圖 12.6，p.647–648）如果主核心跑浮點密集程式，IPC 衝破 $3\text{–}4$，單顆極簡 DIVA 從核心追不上怎麼辦？把執行時間切成固定長度的 **epochs（A → B → C → D）**，配多顆慢速 checker 輪流平行接棒：Checker 1 慢慢驗 epoch A–B 時，主核心繼續衝 B–C 並交給 Checker 2 驗，等主核心到 C–D 時 Checker 1 剛好驗完空出來接手！只要主從速度比是 $4:1$，配 4 顆小 checker 就能無縫跟上（但回滾需搭配 epoch 級 checkpoint）。

---

## 6. 電感雜訊（Inductive Noise）：RLC 電源網格共振與 Pipeline Damping

[52:54]（課本 §12.2，p.648–649；⚠️ 影片 `[53:00]` 與 `[53:54]` 老師兩度口誤說「我們在第 12 章討論過電源網格與去耦電容」，實際是指前一章 **Chapter 11** §11.3.2，課本 p.648 正確寫 Section 11.3）

### 6.1 為什麼去耦電容在共振點會救不了電壓？

[53:27] 真實晶片的電源與接地網格（power/ground grids）加上上百支封裝腳位並非零阻抗理想電壓源：長導線網格帶有顯著**電感 $L$**（依 Lenz 定律 $V = L\,\frac{dI}{dt}$ 抵抗電流突變），加上導線**電阻 $R$** 與我們為了穩壓特地加裝的**去耦合電容 $C$（decoupling capacitors）**，整張電源網格等效上就是一座龐大的 **RLC 振盪電路**！
- 電感電抗 $j\omega L$ 與電容電抗 $\frac{1}{j\omega C}$ 相位剛好差 $\pm 90^\circ$ `[59:51]`。在角頻率 $\omega_0 = \frac{1}{\sqrt{LC}}$ 的**共振頻率（resonant frequency，典型值落在 $10\text{–}100\text{ MHz}$）**處，虛部電抗互相抵銷，並聯等效阻抗 $|Z(\omega)|$ 衝上**極大值（extremum）**！
- **致命後果**：一顆 $3\text{ GHz}$ 處理器若剛好執行一段長約 30–300 cycles 的迴圈（或遭惡意攻擊程式刻意構造迴圈 `[57:06]`），使電流需求剛好以 **$10\text{–}100\text{ MHz}$** 週期性忽高忽低，就會激發晶片級 RLC 共振——**去耦電容在此頻率失去穩壓能力，晶片內部電壓出現巨大正負尖峰與壓降（voltage spikes / droops）**，直接害邏輯閘與記憶胞翻轉出錯！

### 6.2 Pipeline Damping（管線阻尼）

[57:19]（課本 §12.2.2，p.648–649，出自 Powell & Vijaykumar, 2003）前端（fetch/decode）耗電通常很平穩，真正大起大落的是**後端（issue / select / execute / commit）**。硬體在每個固定時間區間（epoch）用後端事件計數器即時估算抽取的電流，強制把相鄰 epoch 的電流變化量壓在安全上限 $\Delta$ 內，手段是「**削峰填谷**」將劇烈震盪抹平：
1. **削峰（高活躍期）**：啟動 **issue / select throttling** 壓低指令發射率（不能靠 DVFS 調頻，因為 PLL 變頻要 $10\text{–}20\ \mu\text{s}$ 遠遠來不及救 `50 MHz` 震盪 `[57:53]`！）。
2. **填谷（低活躍期）**：故意塞入不做有用功的**虛假指令（dummy / extraneous instructions）**白燒一點電流把低谷墊高！雖然多塞假指令會稍微耗電，但換來平坦穩定的電流曲線，徹底拆掉 RLC 共振炸彈。

---

## 7. 【原始碼】Linux x86 `mce/severity.c` 如何區分已修正錯誤、DUE 與 Fail-Stop Panic

對照 §1.1 的 Fail-stop 與 §4 圖 12.4 的「Corrected vs DUE」決策樹，真實 x86 處理器透過 **MCA（Machine Check Architecture）** 硬體暫存器向作業系統回報軟硬體錯誤。我們看 Linux v6.2.12 的 `arch/x86/kernel/cpu/mce/severity.c` 如何查表判定嚴重度：

```c
/* os/linux-src/arch/x86/kernel/cpu/mce/severity.c:80-105, 166-180 */
	MCESEV(
		PANIC, "Processor context corrupt",
		BITSET(MCI_STATUS_PCC)
		),
	/* ... (略過 handler 狀態檢查) ... */
	MCESEV(
		KEEP, "Corrected error",
		NOSER, BITCLR(MCI_STATUS_UC)
		),
	/* ... (略過 scrub / UCNA 規則) ... */
	MCESEV(
		AR, "Action required: data load error in a user process",
		SER, MASK(MCI_STATUS_OVER|MCI_UC_SAR|MCI_ADDR|MCACOD, MCI_UC_SAR|MCI_ADDR|MCACOD_DATA),
		USER
		),
	MCESEV(
		PANIC, "Data load in unrecoverable area of kernel",
		SER, MASK(MCI_STATUS_OVER|MCI_UC_SAR|MCI_ADDR|MCACOD, MCI_UC_SAR|MCI_ADDR|MCACOD_DATA),
		KERNEL
		),
```

這張表幾乎就是課本圖 12.4 在作業系統核心裡的逐行實作：
1. **硬體 ECC 已自動更正（`BITCLR(MCI_STATUS_UC)`）**：Uncorrected 旗標為 `0`，分級為 `KEEP`（對應圖 12.4 `No, also corrected -> No error`，僅背景記錄供預警）。
2. **偵測到但無法更正的 DUE（`MCI_STATUS_UC` 為 `1`）**：
   - 若發生在 **User process** 載入資料（第 166 行 `AR, USER`），Linux 不必整機重開，而是隔離中毒實體頁並只殺掉該行程（局部 Fail-stop）。
   - 若發生在**核心不可恢復區（第 176 行 `KERNEL`）**或**處理器內部脈絡已毀損（第 80 行 `MCI_STATUS_PCC`）**，Linux 立即觸發整機 **`PANIC`（嚴格 Fail-stop）**，寧可當機也絕不讓錯誤狀態變成竄改磁碟與帳務的 SDC！

---

## 8. 【實測】Parity vs SECDED 遮罩率、矩陣不變量檢查加速與 50 MHz RLC 共振阻尼

我們撰寫 [`experiments/lecture49/soft_error_and_damping.py`](experiments/lecture49/soft_error_and_damping.py)，一口氣定量驗證本講三個核心機制：(1) 64-bit 資料在 `AVF = 0.4` 下無保護、單一同位元（Parity）與標準 **SECDED Hamming(72, 64)** 對 1/2/3-bit 翻轉的攔截結果；(2) $1024 \times 1024$ 矩陣反演求解 vs **Invariant Checker（$\|Ax - b\|_\infty$）** 的速度比與單一浮點尾數位元翻轉偵測力；(3) 晶片電源網格並聯 RLC 模型（$L = 50\text{ pH}, C = 200\text{ nF}, R = 25\text{ m}\Omega \Rightarrow f_0 \approx 50.3\text{ MHz}$）在共振點有無 Pipeline Damping 的電壓尖峰：

```python
# experiments/lecture49/soft_error_and_damping.py:81-98
def sim_power_grid_rlc():
    # Package inductance L = 50 pH, on-chip decap C = 200 nF, damping R = 25 mOhm -> f0 ~ 50.3 MHz
    L, C, R = 50e-12, 200e-9, 0.025
    f0 = 1.0 / (2 * np.pi * np.sqrt(L * C))
    results = []
    for label, freq, i_hi, i_lo in [
        ("Off-resonance slow loop (5 MHz)",    5e6,  30.0,  5.0),
        ("At resonance, undamped  (50 MHz)",  50e6,  30.0,  5.0),
        ("At resonance, damped    (50 MHz)",  50e6,  20.0, 15.0),
        ("Off-resonance fast loop (500 MHz)", 500e6, 30.0,  5.0),
    ]:
        w = 2 * np.pi * freq
        Z = 1.0 / ((1.0 / R) + 1j * w * C + 1.0 / (1j * w * L))
        dv_peak = abs(Z) * (2.0 / np.pi) * (i_hi - i_lo)
        results.append((label, abs(Z) * 1e3, dv_peak))
    return f0, results
```

### 實測輸出

```text
=== 1) AVF / ECC Outcome Breakdown (AVF = 0.40, 20,000 trials) ===
[1-bit flip]
  Unprotected   : No-error= 61.4% | DUE=  0.0% | SDC= 38.6%
  Parity        : No-error=  0.0% | DUE=100.0% | SDC=  0.0%
  SECDED(72,64) : No-error=100.0% | DUE=  0.0% | SDC=  0.0%
[2-bit flip]
  Unprotected   : No-error= 37.0% | DUE=  0.0% | SDC= 63.0%
  Parity        : No-error= 37.2% | DUE=  0.0% | SDC= 62.8%
  SECDED(72,64) : No-error=  0.0% | DUE=100.0% | SDC=  0.0%
[3-bit flip]
  Unprotected   : No-error= 22.5% | DUE=  0.0% | SDC= 77.5%
  Parity        : No-error=  0.0% | DUE=100.0% | SDC=  0.0%
  SECDED(72,64) : No-error= 13.9% | DUE= 24.2% | SDC= 61.8%

=== 2) Compute vs Invariant Check (N=1024 A^-1 b vs ||Ax-b||_inf) ===
Matrix-inv solve: 167.68 ms | Invariant check: 0.34 ms (492.0x faster)
Residual clean  : 7.99e-15  | Residual after 1 mantissa bit-flip: 7.45e-09

=== 3) RLC Power-Grid Resonance (f0 = 50.3 MHz, Vdd = 1.0 V) ===
  Off-resonance slow loop (5 MHz)    | |Z|= 1.58 mOhm | peak dV = +/-0.025 V (2.5% Vdd)
  At resonance, undamped  (50 MHz)   | |Z|=24.99 mOhm | peak dV = +/-0.398 V (39.8% Vdd)
  At resonance, damped    (50 MHz)   | |Z|=24.99 mOhm | peak dV = +/-0.080 V (8.0% Vdd)
  Off-resonance fast loop (500 MHz)  | |Z|= 1.60 mOhm | peak dV = +/-0.026 V (2.6% Vdd)
```

### 三項關鍵觀察

1. **AVF 遮罩與 SECDED 的精準邊界**：單一位元翻轉時，無保護暫存器因 `60%` 位元不影響架構狀態而天然遮蔽了 `61.4%`，但仍有 `38.6%` 變成致命 **SDC**；加 1-bit Parity 能把 1-bit SDC 100% 轉成可攔截的 **DUE**，遇上 2-bit 偶數翻轉卻完全失明（`62.8% SDC`）；**SECDED(72,64)** 則完美做到 **1-bit 100% 自動修復、2-bit 100% 抓成 DUE（零 SDC）**，直到極罕見的 3-bit 同時翻轉才會因 syndrome 碰撞誤修而漏出 SDC。
2. **「驗證遠快於計算」的定量鐵證**：在 i7-13650HX 單核上算 $1024 \times 1024$ 矩陣反演要 **$167.68\text{ ms}$**，而 Invariant Checker 驗證 $\|Ax - b\|_\infty$ 只要 **$0.34\text{ ms}$（快了 $492\times$！）**；我們只偷偷翻轉解答向量 `x[100]` 尾數的第 25 個 bit，殘差就從 $7.99 \times 10^{-15}$ 暴衝六個數量級到 $7.45 \times 10^{-9}$，瞬間抓包！
3. **50 MHz 共振殺手與 Pipeline Damping 威力**：在偏離共振的 `5 MHz`（電感低阻抗主導）與 `500 MHz`（去耦電容成功吸掉高頻漣波），阻抗僅 `~1.6 mΩ`、電壓波動只有安全的 `±2.5%`。一旦迴圈頻率撞上 **`50 MHz` 共振點**，電感與去耦電容電抗抵銷，阻抗暴增 **$15.8\times$** 到 `24.99 mΩ`，未阻尼電流震盪（`5 A ↔ 30 A`）直接打出 **`±0.398 V`（近 40% $V_{dd}$！）** 的致命壓降；啟動 Pipeline Damping（峰值節流到 `20 A`、低谷塞 dummy 指令墊到 `15 A`）後，就算卡在共振頻率，電壓波動也立刻縮小 **5 倍**壓回 `±0.080 V`。

---

## 9. 一頁速查表

| 概念／機制 | 核心公式或架構要點 | 工程意義與取捨 |
|---|---|---|
| **Fault → Error → Failure** | 缺陷 $\to$ 錯誤內部狀態 $\to$ 外部可見異常（Fail-stop vs Byzantine） | 遮罩機制可在任一關卡擋下，未必演變成 Failure |
| **FIT / MTTF / MTBF / MTTR** | $1\text{ FIT} = 10^{-9}\text{ failure/hr}$；$\text{MTBF} = \text{MTTF} + \text{MTTR}$ | 伺服器與航太可靠度標準度量語言 |
| **Hazucha–Svensson 模型** | $\text{SER} = F \cdot CS,\; CS \propto A e^{-Q_{\text{crit}}/Q_S}$ | 加大尺寸／$V_{dd}$ 升 $Q_{\text{crit}}$（耗電）；SOI／Triple-well 降 $Q_S$ |
| **三大電路遮罩** | Electrical（閘延衰減）、Logical（控制值遮蔽）、Timing-window（避開 setup/hold） | 組合邏輯比 SRAM 更不易把脈衝鎖成真錯誤的原因 |
| **$\text{SE}_{\text{fail}}$、DUE 與 SDC** | $\text{SE}_{\text{fail}} = \text{SER} \times \text{TVF} \times \text{AVF}$（分支預測器 $\text{AVF}=0$） | DUE 知錯可回滾；**SDC 無聲算錯最致命** |
| **MultiMaster Checkers** | **IBM G5**（雙管線逐拍鎖步 flush）vs **HP NonStop**（僅 I/O 比對，DMR 回滾／TMR 投票） | 100%–200% 硬體成本，換取銀行／航太最高等級保障 |
| **SingleSlave (DIVA) / MultiSlave** | 胖 OOO 主核 ＋ 抗輻射 2-stage 循序從核（`COMM` 驗讀寫 + `COMP` 驗運算）；高 IPC 改多從核輪驗 epoch | 利用「驗證比計算快」大幅壓低冗餘硬體面積 |
| **Inductive Noise & Damping** | 電源網格 RLC 在 $10\text{–}100\text{ MHz}$ 共振致電壓暴跌；高谷節流＋低谷塞 dummy 指令抹平 $\Delta I$ | 犧牲微幅效能與額外功耗，防止共振翻轉位元 |

---

## 10. 與後續講次的連結

- **第五十講（Ch12 Reliability Part II：非確定性與設計缺陷）**：本講談完了外來粒子（軟錯誤）與電源網格共振（電感雜訊）兩大暫態故障；下一講將進入第三種暫態問題——晶片內部跨時脈域／NoC／OS 排程帶來的**固有非確定性（inherent nondeterminism，讓矽後除錯抓不到可重現快照）**，以及先天的**設計錯誤（RTL bugs、形式驗證與矽後驗證）**。
- **第五十一講（Ch12 Reliability Part III：製程變異與硬體老化）**：本講 §1 預告的先天製程變異（process variation）與隨時間劣化的永久性硬體故障（HCI、NBTI、TDDB、電遷移與浴缸曲線系統可靠度）將在第 51 講完整展開。

---

## 附：本講一句話總結

> 面對宇宙中子撞擊（$\text{SER} \propto F A e^{-Q_{\text{crit}}/Q_S}$）與 $10\text{–}100\text{ MHz}$ 電源網格 RLC 共振這兩大暫態殺手，現代處理器結合 SOI 元件隔離、三大電路遮罩、ECC、利用「驗證快於計算」的 DIVA／多主冗餘檢查器消滅無聲資料毀損（SDC），並以「削峰節流＋填谷假指令」的管線阻尼撫平電流突變。

---

*筆記整理自 Smruti R. Sarangi 教授 Advanced Computer Architecture 第 49 講影片與 Next-Gen Computer Architecture v3.2 第 12 章 §12.1–12.2，搭配 Linux v6.2.12 原始碼與本機實測驗證。*
