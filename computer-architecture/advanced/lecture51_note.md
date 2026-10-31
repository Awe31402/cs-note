# Advanced Computer Architecture 第五十一支影片完整筆記：可靠度（三）：光微影與參數變異（Varius 模型、SSTA、OPC／PSM）、電晶體老化（NBTI／HCI）、硬錯誤（RAMP 模型）與浴缸曲線（Chapter 12, Part III）

> **課程**：Advanced Computer Architecture — Chapter 12: Reliability (Variation and Hard Errors) -- Part III/III（播放清單第 51 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（1:18:33）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch12** §12.5–12.7（p.656–668）
> **章號**：影片 Chapter 12 = 課本 Ch12。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10、NumPy 1.26）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕把「blunt knife」寫成「plant knife」、「Varius model」寫成「various model」、「$\sigma_{\text{sys}}$」寫成「Sigma CIS／Sigma Six」、「$\phi$」寫成「5／Phi」、「Gumbel」寫成「dumbbell」、「off-axis illumination」寫成「off access elimination」、「PSM」寫成「esm」、「$t_{\text{rec}}$」寫成「T Rank／T-Rex」、「hot carrier injection」寫成「what carrier in injection」、「hard error」寫成「hardware」、「electromigration」寫成「electromagnation」、「Black's equation」寫成「blacks equations」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. VLSI 光微影製程與參數變異（PVT Variation）成因](#1-vlsi-光微影製程與參數變異pvt-variation成因)
- [2. 系統性變異（CMP、Rayleigh 繞射極限）vs 隨機變異（光子通量 LER、原子摻雜）](#2-系統性變異cmprayleigh-繞射極限vs-隨機變異光子通量-ler原子摻雜)
- [3. Varius 數學模型：球型空間相關多變量常態分布與 SSTA（Gumbel 極值分布）](#3-varius-數學模型球型空間相關多變量常態分布與-sstagumbel-極值分布)
- [4. 對抗參數變異：四大微影修正（OPC、離軸照明、SRAF、PSM）與架構級對策](#4-對抗參數變異四大微影修正opc離軸照明srafpsm與架構級對策)
- [5. 電晶體老化（Aging）：PMOS 的 NBTI 與 NMOS 的熱載子注入（HCI）](#5-電晶體老化agingpmos-的-nbti-與-nmos-的熱載子注入hci)
- [6. 永久硬錯誤（Hard Errors，RAMP 模型）：電遷移、應力遷移、TDDB 與熱循環](#6-永久硬錯誤hard-errorsramp-模型電遷移應力遷移tddb-與熱循環)
- [7. 系統級浴缸曲線（Bathtub Curve）、硬錯誤防護與第 12 章總回顧](#7-系統級浴缸曲線bathtub-curve硬錯誤防護與第-12-章總回顧)
- [8. 【原始碼】Linux `mm/memory-failure.c` 的 `soft_offline_page()` 老化頁遷移退役](#8-原始碼linux-mmmemory-failurec-的-soft_offline_page-老化頁遷移退役)
- [9. 【實測】Varius 空間相關與 Gumbel 時序裕度、NBTI 恢復極限與 RAMP 高溫折壽模擬](#9-實測varius-空間相關與-gumbel-時序裕度nbti-恢復極限與-ramp-高溫折壽模擬)
- [10. 一頁速查表](#10-一頁速查表)
- [11. 與後續講次的連結](#11-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
一、參數變異（Parameter / PVT Variation = Process + Voltage + Temperature，§12.5）
  ├── 物理根源：「拿鈍刀雕細花」（193 nm / 13.5 nm 光波刻 3–10 nm 線寬）＋ 單顆電晶體只剩數十顆矽原子（0.22 nm）
  ├── 製程變異加法分解：ΔP = ΔP_D2D + ΔP_WID = ΔP_D2D + ΔP_sys + ΔP_rand
  │     • 系統性（Systematic，具空間相關）：CMP 研磨厚度不均 ＋ 光波繞射（低於 Rayleigh 極限 λ/NA，轉角變圓）
  │     • 隨機性（Random，鄰近電晶體也獨立）：光粒子性造成 LER（線邊緣粗糙）＋ 摻雜原子數／閘極氧化層厚度隨機起伏
  ├── Varius 統計模型與 SSTA（Statistical Static Timing Analysis）：
  │     • 閘延遲：T_g ∝ L_eff V / [μ(T) (V − V_th(T))^α]，μ ∝ T^−1.5 與 dV_th/dT ≈ −2.5 mV/°C 互相拔河
  │     • 系統性變異：多變量高斯 N(μ, Σ)，球型相關 ρ(r) = 1 − 3r/(2ϕ) + r³/(2ϕ³)（r < ϕ）；隨機變異：獨立高斯
  │     • 單一路徑延遲 = 高斯和（仍為高斯）；全電路最慢路徑 = 多條高斯取 Max → 服從非高斯的 **Gumbel 分布**！
  └── 防護：微影四招（OPC 預扭曲、離軸照明、SRAF 輔助次解析條紋、PSM 相移破壞性干涉）＋ 架構三招（Binning、V/f 島、Body Biasing）

二、電晶體老化（Aging）與永久硬錯誤（Hard Errors，§12.6）
  ├── 老化（使 V_th 升高、電晶體變慢）：
  │     • NBTI（PMOS 負偏壓）：Si–H 鍵斷裂留正電於介面；ΔV_th ∝ t_stress^0.25，斷電可部分恢復但仍有永久殘留
  │     • HCI（NMOS 熱載子注入）：高動能電子卡進 SiO₂ 閘極；⚠️ 課本 ΔV_th ∝ t^0.5（影片寫 t^1.5），幾乎無法恢復！
  ├── 四大硬錯誤（RAMP 模型，全部隨溫度升高而指數／冪次折壽！）：
  │     1. Electromigration（導線電子風吹走金屬原子：一邊變細斷路、一邊變胖擠出短路；Black's eq. ∝ (J−J_crit)^−n e^{E_a/kT}）
  │     2. Stress Migration（接點異質材料熱膨脹應力龜裂：∝ |T₀−T|^−n e^{E_a/kT}）
  │     3. TDDB（薄閘極氧化層穿隧擊穿變導體）
  │     4. Thermal Cycling（封裝錫球冷熱交替金屬疲勞：Coffin–Manson ∝ (T − T_amb)^−q）
  └── 系統浴缸曲線（Bathtub Curve）：早夭期（Burn-in 篩除）→ 正常期（7–10 年，多數先淘汰）→ 磨損期
      銀彈對策：**降溫** ＋ 銅線包覆 **TaN（氮化鉭）** ＋ 記憶體備用列重映射（Spare Rows）＋ 核心降級運作（Degraded Mode）
```

---

## 1. VLSI 光微影製程與參數變異（PVT Variation）成因

[00:35]（課本 §12.5.1，p.656）要理解晶片為什麼天生每顆電晶體都長得不一樣，得先看 **VLSI 光微影（photolithography）**怎麼造晶片：
1. 取一片直徑 **$200\text{–}300\text{ mm}$（約一英尺）**的圓形矽晶圓（wafer），塗上一層感光材料**光阻劑（photoresist）**。
2. 在光源前放上畫好電路圖案的**光罩（photomask）**曝光，再用化學溶劑把曝光區（或未曝光區）洗掉——這步稱為**蝕刻（etching）**，露出底下的矽溝槽。
3. 注入氣態／液態的 p 型（硼）或 n 型（磷）**摻雜物（dopants）**滲入矽基板形成電晶體源極／汲極，或沉積 $\text{SiO}_2$ 絕緣層、打垂直導通孔（vias）並沉積銅金屬層。重複數十層就蓋出數十億顆電晶體的大樓。

[06:03] **「拿鈍刀做微雕」的物理困境**：
- 我們想刻出 $10\text{ nm}$、$7\text{ nm}$ 甚至 $3\text{ nm}$ 的精細線條，但傳統微影用的深紫外光波長卻高達 **$193\text{ nm}$（或 $157\text{ nm}$）**！
  - ⚠️ **課本與影片對 $13.5\text{ nm}$ EUV 的時間差**：課本 p.657（撰寫較早）寫「縮短波長至 $13.5\text{ nm}$ 極紫外光（EUV）的努力尚未成功，仍卡在 $193\text{ nm}$」；影片 `[06:54]–[07:20]`（2021–22 錄製）老師口頭更新指出 **$7\text{ nm}$／$3\text{ nm}$ 製程已實際改用 $13.5\text{ nm}$ EUV 光源**——但即便降到 $13.5\text{ nm}$，波長依然是 $3\text{ nm}$ 特徵尺寸的 $4\times$ 以上！
- 再加上一顆矽原子直徑約 **$0.22\text{ nm}$**（課本 p.657；影片口頭抓數量級約 $0.1\text{ nm} = 1\text{ \AA}$），一條 $5\text{–}10\text{ nm}$ 的通道橫向只排了 **50–100 顆原子**！只要摻雜時隨便多跑進或少跑進 10 顆原子，電晶體尺寸與電氣特性就偏離 **$\pm 10\%\text{–}20\%$**！

[11:13] **PVT 參數變異大傘（Process, Voltage, Temperature）**：
除了先天的 **(1) 製程變異（Process variation）**，執行期還有電源網格 $IR$ 壓降與 $L\,\frac{dI}{dt}$ 電感壓降造成的 **(2) 電壓變異（Voltage variation）**，以及晶片熱點造成的 **(3) 溫度變異（Temperature variation）**（⚠️ `[11:45]` 老師口誤把前一章溫度模型說成 Chapter 12，實指 Chapter 11）。三者合稱致命的 **PVT variation**：有的區域 $V_{th}$ 偏低而**漏電暴增**，有的區域 $V_{th}$ 偏高變慢而引發**時序錯誤（timing faults）**！

---

## 2. 系統性變異（CMP、Rayleigh 繞射極限）vs 隨機變異（光子通量 LER、原子摻雜）

[20:56]（課本式 12.7、圖 12.7，p.657–659）任何電晶體參數（如 $V_{th}$ 或通道長度）的總製程偏移量 $\Delta P$ 可近似拆成三個相加項：
$$\Delta P = \Delta P_{\text{D2D}} + \Delta P_{\text{WID}} = \Delta P_{\text{D2D}} + \Delta P_{\text{sys}} + \Delta P_{\text{rand}}$$
其中 $\Delta P_{\text{D2D}}$（Die-to-Die，晶粒間變異）對同一顆晶片是個常數平移；晶片內部（Within-Die, WID）則分成性質截然相反的兩塊——有趣的是，**光的波動性害了第一塊，光的粒子性又害了第二塊**！

| 類型 | 物理成因 | 幾何與統計特徵 |
|---|---|---|
| **系統性變異（Systematic，$\Delta P_{\text{sys}}$）** `[13:20]` | 1. **CMP（Chemical Mechanical Polishing，化學機械研磨）**：晶片沒佈線的空白區較軟易塌，雖填入不接電的虛假銅柱（dummy metal fills）支撐再用旋轉墊＋化學液研磨拋光，因 $\text{SiO}_2$（硬）與銅／矽軟硬不同，磨完各區厚度仍有起伏。<br>2. **光的波動性與繞射（Diffraction）**：依 **Rayleigh 準則**，最小可解析特徵尺寸 $\propto \frac{\lambda}{NA}$（波長除以數值孔徑）。低於此極限時，穿過光罩百萬條微縫的光發生嚴重繞射，比 CMP 嚴重得多。 | **具空間相關性（Spatial Correlation）**：相鄰區塊一起偏高或一起偏低（低頻平滑變化）；理想直角矩形印出來變成**邊緣縮短與圓角（edge shortening & rounded corners，圖 12.7a）**。 |
| **隨機變異（Random，$\Delta P_{\text{rand}}$）** `[17:34]` | 1. **光的粒子性（Photon Shot Noise）＋ 蝕刻酸液擴散不均**：光子是離散粒子，奈米尺度下單位時間打在直線各處的光子通量不均勻。<br>2. **離散摻雜原子波動（RDF）與閘極氧化層厚度起伏**：$2\text{–}3\text{ nm}$ 薄氧化層與數十顆摻雜原子無法精準排隊。 | **無空間相關性**：就算緊貼在隔壁的兩顆電晶體也完全獨立隨機；直線印出來變成鋸齒狀的**線邊緣粗糙度（LER, Line Edge Roughness，圖 12.7b）**。 |

---

## 3. Varius 數學模型：球型空間相關多變量常態分布與 SSTA（Gumbel 極值分布）

[21:31]（課本 §12.5.2，p.658–661）老師介紹自己團隊提出、學界廣泛採用的參數變異模擬框架 **Varius**（Sarangi et al., 2008）。

### 3.1 邏輯閘延遲與溫度的雙向拔河

[21:45]（課本式 12.6，p.658）由 $\alpha$-power law，單一邏輯閘轉態延遲 $T_g$ 為：
$$T_g \propto \frac{L_{\text{eff}}\, V}{\mu(T)\,(V - V_{th}(T))^\alpha}, \qquad \alpha \approx 1.3 \in [1.1, 1.5]$$
當溫度 $T$ 升高時，式中有兩股**方向相反**的力量在拔河：
1. **載子遷移率下降（使電晶體變慢）**：矽晶格熱震動加劇，$\mu(T) \propto T^{-1.5}$ $\Rightarrow$ 分母變小，$T_g$ 變長。
2. **臨界電壓下降（使電晶體變快、同時漏電暴增）**：溫度每升高 $1\text{°C}$，$V_{th}$ 約下降 **$2.5\text{ mV/°C}$**（升高 $10\text{°C}$ 就掉 $25\text{ mV}$！）$\Rightarrow$ $(V - V_{th})^\alpha$ 變大，$T_g$ 縮短。
- 在傳統電壓區間，第一項（遷移率下降）通常佔上風，所以**升溫整體讓電晶體變慢**（但在低電壓區間也可能反轉，稱 inverted temperature dependence）。

### 3.2 系統性變異的多變量常態分布與球型相關函數

[25:06]（課本式 12.8–12.12，p.659–660）Varius 將晶片表面切成 $k = N \times N$ 格網格，假設在系統性變異下 $L_{\text{eff}}$ 的相對偏移量剛好是 $V_{th}$ 相對偏移量 $d$ 的一半（$\Delta L_{\text{eff}}/L_0 = 0.5\, d$，$V_{th} = V_{th0}(1 + d)$）。全晶片 $k$ 格的無因次偏移向量 $X \in \mathbb{R}^k$ 服從**多變量常態分布（Multivariate Normal Distribution）**：
$$\text{pdf}(X) = \frac{1}{\sqrt{(2\pi)^k |\Sigma|}} \exp\!\left(-\frac{1}{2}(X - \mu)^T \Sigma^{-1} (X - \mu)\right)$$
假設各格標準差皆為 $\sigma_{\text{sys}}$，且共變異數矩陣 $\Sigma(U, V) = \sigma_{\text{sys}}^2 \rho(U, V)$ 具**等向性（isotropic，不看方向只看兩格歐氏距離 $r$，以晶片寬度正規化為 $1$）**，實測晶圓吻合**球型相關函數（Spherical Correlation Function，式 12.12）**：
$$\rho(r) = \begin{cases} 1 - \dfrac{3r}{2\phi} + \dfrac{r^3}{2\phi^3}, & r < \phi \\[8pt] 0, & r \ge \phi \end{cases}$$
其中相關半徑 $\phi \in [0.1, 0.5]$：$r = 0$ 時完全相關（$\rho = 1$），隨距離拉遠平滑遞減，超過半徑 $\phi$ 以外的區塊則完全獨立不相關（$\rho = 0$）。

### 3.3 隨機變異合成與統計靜態時序分析（SSTA）

[39:04]（課本式 12.13–12.14，p.660–661）隨機變異下 $V_{th}$ 與 $L_{\text{eff}}$ 不再線性連動，各自服從獨立單變量常態分布 $\mathcal{N}(\mu, \sigma_{\text{rand}}^2)$。總標準差由高斯獨立可加性合成為：
$$\sigma_{\text{total}} = \sqrt{\sigma_{\text{rand}}^2 + \sigma_{\text{sys}}^2} \qquad \text{（2020 年代典型模擬參數：}\frac{\sigma_{\text{total}}}{\mu} = 0.09,\; \sigma_{\text{sys}} = \sigma_{\text{rand}}\text{）}$$

[40:10] **為什麼傳統 STA 不夠用，必須改用 SSTA（Statistical Static Timing Analysis）？**
- 在單一條由多顆邏輯閘串聯成的路徑 $P_i$ 上，每顆閘延遲是常態分布，**高斯隨機變數的總和依然是常態分布**。
- **關鍵數學陷阱** `[42:03]`：一塊電路裡有成千上萬條候選路徑 $P_1, P_2, \dots, P_m$，決定晶片能不能跑過關的**關鍵路徑延遲是所有路徑的最大值 $D_{\text{max}} = \max_i P_i$**——而**「多個常態分布取最大值」不再是常態分布，而是服從右偏長尾的 Gumbel 極值分布（Gumbel distribution）**！SSTA 算出這條最終分布後，才能在右尾挑一個安全的保守上界訂為時脈週期（見 §9 實測）。

---

## 4. 對抗參數變異：四大微影修正（OPC、離軸照明、SRAF、PSM）與架構級對策

[45:10]（課本 §12.5.3、圖 12.8，p.661–662）如果放任變異不管、只靠拉大時脈週期保守硬吞，處理器頻率會慘跌！因此必須從微影端與架構端雙管齊下：

### 4.1 微影級四大解析度強化技術（RET，圖 12.8）

```
  (a) OPC 光學鄰近校正          (c) SRAF 次解析度輔助條紋        (d) PSM 相移光罩
   ┌──┐  ┌──┐                   ║  ┌──────┐  ║                 光線 A (0°)    光線 B (180° 反相!)
   └──┘──└──┘ (光罩預先加狗骨頭耳朵) ║  │ 主線 │  ║ (極細輔條自己印不出來  ▼              ▼
       ▼ 繞射後印在矽晶圓上：       ║  └──────┘  ║  但繞射補光讓主線飽滿) ████  [中間破壞性干涉全黑!]  ████
   ┌────────┐ (還原成漂亮直角矩形!)                                    (徹底切斷兩鄰線沾黏短路!)
   └────────┘
```

1. **OPC（Optical Proximity Correction，光學鄰近校正）** `[45:44]`：既然直角印出來會縮圓，就在光罩上**反向預先扭曲（pre-distort）**——四個角落故意多長出「狗骨頭」方耳、曝光過強處挖凹，算準繞射後印在矽上剛好變回標準矩形（背後需極龐大的反演運算）。
2. **Off-Axis Illumination（離軸照明）** `[46:53]`：反直覺地**不把光源擺在光罩正中央垂直照射，而是斜角入射**！這樣透鏡能捕捉到更高階的繞射條紋（higher diffraction orders），讓更多有效光能量聚焦到晶圓圖案上。
3. **SRAF（Sub-Resolution Assist Features，次解析度輔助特徵）** `[48:45]`：在孤立主線條旁邊畫幾條**細到低於解析度極限的極窄細縫**——細縫本身在矽上完全不會印出東西，但透過去的光會跟主線產生建設性干涉補光，把原本乾癟變形的線條撐回飽滿矩形！
4. **PSM（Phase-Shift Masking，相移光罩）** `[50:37]`：兩條靠太近的線容易因繞射光疊加而「黏在一起」短路。透過改變光罩局部厚度或材質改變光程，讓相鄰兩區透過去的光產生**反相相位差**（⚠️ 影片 `[51:37]–[53:22]` 四度明確講 **$180^\circ$ 反相**，課本 p.661 括號則寫「常移相 $90^\circ$」——物理上兩道相鄰光束要達成完全破壞性干涉，彼此相對相位差須為 $180^\circ$，例如一側 $+90^\circ$ 另一側 $-90^\circ$，或單側移相 $180^\circ$），使兩線中間夾縫發生**破壞性干涉（destructive interference）變全黑**，銳利切開邊界！

### 4.2 架構級三大旋鈕

[53:40]（課本 p.661–662）：
1. **Frequency Binning（頻率分級）**：出廠測試每顆晶粒的最高安全頻率與專屬 DVFS 表分級販售。
2. **Voltage–Frequency Islands（電壓–頻率島）**：同一顆多核晶片裡，落在系統性變異「快區」的核心跑高頻、落在「慢區」的核心跑低頻（代價是跨島通訊需同步器，驗證難度大增）。
3. **Adaptive Body Biasing（ABB，動態基體偏壓）** `[55:03]` `【口頭】`：直接複用[第四十八講](lecture48_note.md)學過的 $V_{BS}$ 旋鈕——偏慢的區塊加 Forward Body Bias 降 $V_{th}$ 救速度，偏快太漏電的區塊加 Reverse Body Bias 升 $V_{th}$ 止血！

---

## 5. 電晶體老化（Aging）：PMOS 的 NBTI 與 NMOS 的熱載子注入（HCI）

[56:35]（課本 §12.6.1，p.663）就算晶片剛出廠時健康無瑕，用個幾年也會像生物一樣**老化（aging，使臨界電壓 $V_{th}$ 逐年升高、電晶體越來越慢）**：

### 5.1 NBTI（Negative Bias Temperature Instability，負偏壓溫度不穩定性）— 專傷 PMOS

[56:51] 製程中大量使用氫鈍化矽表面懸鍵（形成穩定的 $\text{Si–H}$ 鍵）。當 **PMOS 電晶體導通（閘極相對源極為負偏壓，Stress 階段）**時：
- 負電場把解離出的氫離子 $\text{H}^+$（質子）往 $\text{Si–SiO}_2$ 閘極介面拉過去，撞斷介面上的 $\text{Si–H}$ 鍵結合成 $\text{H}_2$ 分子跑掉，在矽表面與氧化層內留下**正電荷累積（介面陷阱）** $\Rightarrow$ **PMOS 的 $|V_{th}|$ 升高、轉態變慢**！
- **壓力與部分恢復冪次律** `[60:36]`（Tiwari & Torrellas, 2008 *Facelift*）：
  $$\Delta V_{th,\text{stress}} \propto t_{\text{stress}}^{0.25}, \qquad \text{未恢復殘留比例} \propto 1 - \sqrt{\frac{\eta\, t_{\text{rec}}}{t_{\text{rec}} + t_{\text{stress}}}}\quad (\eta \approx 0.35)$$
  好消息是一旦關閉閘極電壓（**Recovery 階段**，$t_{\text{rec}}$），部分質子會退回修復；壞消息是就算休息無限久（$t_{\text{rec}} \to \infty$），上式仍收斂到 $1 - \sqrt{\eta} \approx 40.8\% > 0$，**永遠有一部分正電荷陷阱救不回來**，長年累月終究拖垮時序（對講求鄰近電晶體特性對稱的類比電路尤其致命，課本 p.663）。

### 5.2 HCI（Hot Carrier Injection，熱載子注入）— 專傷 NMOS

[62:10]（課本 p.663）複習[第四十六講](lecture46_note.md)的漏電機制：當 **NMOS** 閘極加正電位頻繁切換時，通道電子獲得極高動能變成「熱電子」，直接穿隧衝進閘極 $\text{SiO}_2$ 絕緣層裡**永久卡死（trapped）**，並撞毀介面 $\text{Si–H}$ 鍵 $\Rightarrow$ **NMOS 的 $V_{th}$ 升高變慢**！
- **與 NBTI 的兩大差異**：(1) **幾乎完全無法恢復（irreversible）**，傷害單向累加；(2) 對切換時間的冪次依賴比 NBTI（$0.25$ 次方）更強：
  - ⚠️ **課本與影片指數差異**：課本 p.663 明確寫 **$\Delta V_{th} \propto t^{0.5}$**（與經典 Takeda／Facelift 半次方模型一致），影片 `[63:50]` 投影片與口頭則唸成 **`t^{1.5}`**；兩者都在強調 HCI 隨活躍轉態時間成長的斜率強於 NBTI 的 $t^{0.25}$。

---

## 6. 永久硬錯誤（Hard Errors，RAMP 模型）：電遷移、應力遷移、TDDB 與熱循環

[64:07]（課本 §12.6.2，p.664–665）老化的終點就是電晶體擊穿或金屬線扯斷，造成永遠無法復原的**硬錯誤（Hard Errors）**。課本採用著名的 **RAMP 可靠度模型**（Srinivasan et al., 2004/2005）列出四大殺手（以下公式中 $T$ 皆為**絕對溫度 Kelvin**，$k$ 為 Boltzmann 常數）：

1. **Electromigration（電遷移，銅／鋁導線）** `[64:58]`：
   直流電子長年單向奔流，動量一點一滴把金屬原子往前「吹」走——**電子流出端越變越細（電阻暴增，最後啪一聲斷成開路 open circuit），流入端越堆越胖甚至往旁擠出金屬鬚（extrusion，碰觸隔壁導線變成短路 short circuit！）**。平均失效時間由著名的 **Black's Equation（式 12.15）** 給出：
   $$\text{MTTF}_{\text{em}} \propto (J - J_{\text{crit}})^{-n}\, e^{\frac{E_{a\text{EM}}}{k T}} \qquad (\text{銅導線 } n = 1.1,\; E_{a\text{EM}} = 0.9\text{ eV})$$
2. **Stress Migration（應力遷移，導線接點 contacts）** `[67:23]`：
   導線與接點由不同材料構成，熱膨脹係數不同，冷熱應力累積驅動金屬原子遷移產生裂縫（式 12.16）：
   $$\text{MTTF}_{\text{sm}} \propto |T_0 - T|^{-n}\, e^{\frac{E_{a\text{SM}}}{k T}} \qquad (T_0 \approx 500\text{ K 無應力基準溫度},\; n = 2.5,\; E_{a\text{SM}} = 0.9\text{ eV})$$
   雖然升溫時 $|T_0 - T|^{-n}$ 略增，但指數項 $e^{E_a/kT}$ 跌得兇得多，整體 MTTF 仍隨升溫劇減！
3. **TDDB（Time-Dependent Dielectric Breakdown，隨時間閘極介電層擊穿）** `[69:19]`：
   只有幾 nm 厚的 $\text{SiO}_2$ 閘極絕緣層長年受穿隧電子轟擊，化學結構逐漸變得多孔，最終徹底擊穿變成一條永久導電短路通道（式 12.17）：
   $$\text{MTTF}_{\text{tddb}} \propto \left(\frac{1}{V}\right)^{a - b T} \exp\!\left(\frac{X + Y/T + Z T}{k T}\right)$$
   （RAMP 常數：$a = 78,\; b = -0.081,\; X = 0.759\text{ eV},\; Y = -66.8\text{ eV}\cdot\text{K},\; Z = -8.37 \times 10^{-4}\text{ eV/K}$。）
4. **Thermal Cycling（熱循環金屬疲勞，封裝 BGA 錫球焊點）** `[70:58]`：
   開關機（低頻大溫差，殺傷力最大）與負載波動讓封裝錫球反覆熱脹冷縮，最終金屬疲勞龜裂，服從 **Coffin–Manson 方程式（式 12.18）**：
   $$\text{MTTF}_{\text{tc}} \propto \left(\frac{1}{T - T_{\text{ambient}}}\right)^q \qquad (q \approx 2.35)$$

---

## 7. 系統級浴缸曲線（Bathtub Curve）、硬錯誤防護與第 12 章總回顧

### 7.1 浴缸曲線與四大硬錯誤對策

[72:21]（課本 §12.6.3–12.6.4、圖 12.9，p.665–666）若處理器內部沒有硬體冗餘，整顆晶片就是一個**串聯失效系統（series-failure system：壞一個元件整顆報銷）**。電子產品一生的失效率呈現經典的**浴缸曲線（Bathtub Curve）**：

```
  失效率 ▲
         │\                                              /
         │ \  1. 早夭期         2. 正常服役期            /  3. 老化磨損期
         │  \ (Infant Mortality)  (Normal Life)        /   (Wear-out, 7–10 年後)
         │   \__________________                      /
         │                      ─────────────────────
         └─────────────────────────────────────────────────► 時間 Time
             ▲ 出廠前用高溫 Burn-in 烤機提前逼死淘汰！    ▲ 多數手機／電腦在此之前就因過時換機了
```

正因為多數消費級 CPU 用不到 7–10 年就被淘汰換新，一般處理器不會花大面積做核心級硬體備援，而是靠四招經濟實惠的防護 `[74:25]`：
1. **終極銀彈：降低工作溫度！**（⚠️ `[74:37]` 老師口誤把「大幅延長 MTTF／降低失效率」說成 reduces MTTF，對照課本 p.665 *"reduce the probability of developing such permanent faults"* 與前面四條公式，降溫會使 $e^{E_a/kT}$ **指數級拉長 MTTF 壽命**！）
2. **銅導線包覆氮化鉭（$\text{TaN}$, Tantalum Nitride）阻障層** `[74:48]`：像給銅線套一層緊身硬管，既阻止銅原子擴散到旁邊矽晶，也壓制導線變胖擠出（extrusion），一併抑制另一端變細斷裂！
3. **記憶體備用列重映射（Spare Rows）** `[75:41]`：在 DRAM／快取陣列預留幾排未映射的備用列，一旦某列發生永久硬錯誤，解碼器就像虛擬記憶體一樣把壞列位址永久重導向備用列。
4. **降級運作模式（Degraded Mode）** `[76:16]` `【口頭】`：壞掉一顆加法器就靠剩下的加法器撐，8 核壞了 4 核就屏蔽壞核當 4 核處理器繼續跑。

[76:38] 最後老師回顧第 12 章全部五塊拼圖：軟錯誤（粒子撞擊）、電感雜訊（RLC 共振）、設計缺陷（RTL 勘誤）、製程與執行期參數變異（Varius 與 SSTA）、以及電晶體老化與硬錯誤（NBTI/HCI 與 RAMP），並再次點出**溫度控制貫穿了第 11 與第 12 兩整章的命脈**。

---

## 8. 【原始碼】Linux `mm/memory-failure.c` 的 `soft_offline_page()` 老化頁遷移退役

對照 `[75:41]`「像虛擬記憶體一樣把老化／故障記憶體重映射並除役（decommission）」的思想，除了硬體陣列內的 spare rows，作業系統也能在實體頁累積過多修正錯誤（預示即將發生永久硬錯誤）時提前介入！我們看 Linux v6.2.12 的 `mm/memory-failure.c`：

```c
/* os/linux-src/mm/memory-failure.c:2519-2526, 2553-2558 */
	if (isolate_page(hpage, &pagelist)) {
		ret = migrate_pages(&pagelist, alloc_migration_target, NULL,
			(unsigned long)&mtc, MIGRATE_SYNC, MR_MEMORY_FAILURE, NULL);
		if (!ret) {
			bool release = !huge;

			if (!page_handle_poison(page, huge, release))
				ret = -EBUSY;
		}
/* ... 函式說明註解 ...
 * Soft offline a page, by migration or invalidation, without killing anything.
 * This is for the case when a page is not corrupted yet (so it's still valid
 * to access), but has had a number of corrected errors and is better taken out.
 */
```

當硬體巡檢發現某個實體頁老化不穩（修正錯誤頻繁飆高、瀕臨硬擊穿）時，Linux 透過 `isolate_page()` 鎖定該頁、呼叫 `migrate_pages()` 把上面的有效資料透明搬移到健康的新實體頁並更新頁表，最後用 `page_handle_poison()` 將該老化實體頁永久打上 `HWPoison` 標記除役——應用程式連一毫秒中斷都感覺不到，就在作業系統層完成了完美的「無痛備援重映射」！

---

## 9. 【實測】Varius 空間相關與 Gumbel 時序裕度、NBTI 恢復極限與 RAMP 高溫折壽模擬

我們撰寫 [`experiments/lecture51/varius_and_ramp.py`](experiments/lecture51/varius_and_ramp.py)，把課本 §12.5–12.6 的 **Varius 球型空間相關模型（式 12.12，$\sigma_{\text{total}}/\mu = 0.09,\; \sigma_{\text{sys}} = \sigma_{\text{rand}}$）**、**NBTI 壓力恢復公式（$\eta = 0.35$）**與 **RAMP 四大硬錯誤公式（式 12.15–12.18 全部代入課本原始常數）**寫成數值模擬（5,000 顆蒙地卡羅晶粒，每顆 32 條獨立候選關鍵路徑、每條 16 級邏輯閘）：

```python
# experiments/lecture51/varius_and_ramp.py:16-24, 52-68
    xs = np.linspace(0, 1, grid_n)
    pts = np.array([(x, y) for x in xs for y in xs])
    r = np.hypot(*(pts[:, None, :] - pts[None, :, :]).transpose(2, 0, 1))
    rho = np.where(r < phi, 1.0 - 1.5 * (r / phi) + 0.5 * (r / phi) ** 3, 0.0)
    L = np.linalg.cholesky((sigma_sys ** 2) * rho + 1e-12 * np.eye(grid_n * grid_n))
# ... RAMP MTTF ratios between T1 = 65 C (338.15 K) and T2 = 85 C (358.15 K):
    em_1, em_2 = np.exp(0.9 / (k_B * T1)), np.exp(0.9 / (k_B * T2))
    sm_1 = (abs(500.0 - T1) ** -2.5) * np.exp(0.9 / (k_B * T1))
    sm_2 = (abs(500.0 - T2) ** -2.5) * np.exp(0.9 / (k_B * T2))
```

### 實測輸出

```text
=== 1) Varius Spatial Correlation & SSTA Gumbel Max-Path Delay (Ideal = 16.00) ===
  Single path (uncorrelated random only) : mean=16.02 | 99.9th pct=16.75 (+4.7% guardband)
  Single path (Varius sys + rand)        : mean=16.04 | 99.9th pct=19.20 (+20.0% guardband)
  Chip max of 32 paths (Gumbel SSTA)     : mean=17.89 | 99.9th pct=20.32 (+27.0% guardband)

=== 2) NBTI Stress-Recovery Remaining Degradation Fraction (eta = 0.35) ===
  t_rec/t_str=0.0: 100.0% | t_rec/t_str=0.5: 65.8% | t_rec/t_str=1.0: 58.2% | t_rec/t_str=10.0: 43.6% | t_rec/t_str=inf: 40.8%

=== 3) RAMP Hard-Error MTTF Lifespan Loss: 65 C -> 85 C (+20 C Hotspot, Tamb=30 C) ===
  Electromigration (Black)  : MTTF(65C) / MTTF(85C) =  5.61x shorter life at 85 C
  Stress Migration          : MTTF(65C) / MTTF(85C) =  4.03x shorter life at 85 C
  TDDB (at V = 1.05 V)      : MTTF(65C) / MTTF(85C) =  2.22x shorter life at 85 C
  Thermal Cycling (Coffin)  : MTTF(65C) / MTTF(85C) =  2.89x shorter life at 85 C
```

### 三項關鍵觀察

1. **為什麼忽略「空間相關」與「Gumbel 極值」會算錯時脈？**
   - 若誤以為 16 級邏輯閘全是獨立隨機變異，快慢正負抵銷（除以 $\sqrt{16}$）後 99.9% 良率只需留 **`+4.7%`** 時脈裕度（guardband）。
   - 一旦放入 **Varius 系統性空間相關**，同一區塊的 16 顆閘會「同進退一起變慢」，裕度瞬間暴增四倍到 **`+20.0%`**！再對全晶片 32 條候選路徑取最大值（**Gumbel 分布**），連「平均最慢路徑」都從 `16.04` 右移到 `17.89`，99.9% 時脈裕度推高到 **`+27.0%`**——直接量化了為什麼晶圓廠非用 OPC／PSM／ABB 把變異壓下來不可！
2. **NBTI 休息也救不回的永久殘留**：即使給 PMOS 跟工作時間一模一樣長的斷電休息（`t_rec / t_str = 1.0`），仍只救回約 $41.8\%$，剩 **`58.2%`** 劣化留存；就算休息時間趨近無限大，也還有 **`40.8%`** 的 $\Delta V_{th}$ 永久殘留。
3. **只升溫 $+20\text{°C}$，硬體壽命直接砍掉 2 到 5.6 倍**：工作溫度從涼爽的 $65\text{°C}$ 升到常見熱點 $85\text{°C}$，銅導線**電遷移壽命暴跌 $5.61\times$**、接點**應力遷移折壽 $4.03\times$**、錫球**熱循環疲勞折壽 $2.89\times$**、閘極氧化層 **TDDB 折壽 $2.22\times$**！數字鐵證完美詮釋何謂「降溫就是抗硬錯誤的唯一銀彈」。

---

## 10. 一頁速查表

| 類別／機制 | 核心物理或數學公式 | 關鍵防護對策 |
|---|---|---|
| **系統性變異（$\Delta P_{\text{sys}}$）** | CMP 軟硬厚度差 ＋ 光繞射（Rayleigh $\lambda / NA$）；球型相關 $\rho(r) = 1 - \frac{3r}{2\phi} + \frac{r^3}{2\phi^3}$ | Dummy 銅填料、**OPC**、**離軸照明**、**SRAF**、**PSM 相移光罩**；架構用 Binning／$V$-$f$ 島／ABB |
| **隨機變異（$\Delta P_{\text{rand}}$）** | 光子粒子性 LER ＋ 數十顆摻雜原子波動；$\sigma_{\text{total}} = \sqrt{\sigma_{\text{rand}}^2 + \sigma_{\text{sys}}^2}$ | **SSTA**（單路徑高斯和、多路徑最大值服從 **Gumbel 分布**）留足保守時序裕度 |
| **NBTI（PMOS 老化）** | 負閘壓拉質子斷 $\text{Si–H}$ 鍵積聚正電；$\Delta V_{th} \propto t_{\text{stress}}^{0.25}$ | Power gating 斷電進入 Recovery 階段可部分逆轉（但仍有 $\sim 41\%$ 永久殘留） |
| **HCI（NMOS 老化）** | 高動能電子衝入 $\text{SiO}_2$ 卡死；⚠️ 課本 $\Delta V_{th} \propto t^{0.5}$（影片 $t^{1.5}$） | 幾乎不可逆；靠降壓降頻減少高能載子轉態 |
| **電遷移 / 應力遷移** | 電子動量吹走銅原子（Black's eq. $e^{E_a/kT}$）／異質熱膨脹應力龜裂 | **降溫** ＋ 銅線包覆 **$\text{TaN}$（氮化鉭）** 阻障層限制形變與擴散 |
| **TDDB / 熱循環** | 薄氧化層穿隧擊穿 $\left(\frac{1}{V}\right)^{a-bT}$ ／ BGA 錫球熱脹冷縮疲勞 $(T - T_{\text{amb}})^{-q}$ | **降溫**、避免過高電壓、出廠 **Burn-in** 篩除浴缸曲線早夭期瑕疵、DRAM 備用列重映射 |

---

## 11. 與後續講次的連結

- **第五十二至五十四講（Ch13 Secure Processor Architectures：安全處理器架構）**：第 11、12 章探討的是大自然與物理定律（溫度、宇宙中子、RLC 共振、微影繞射、原子遷移）如何無意間破壞正確性；從下一講進入第 13 章起，對手將換成**聰明且懷有惡意的人類攻擊者**——如何用硬體加密與雜湊保護機密與完整性（第 52–53 講 Intel SGX），以及攻擊者如何反過來利用我們前面學過的快取時序、功耗與錯誤注入發動旁道與故障攻擊（第 54 講）！

---

## 附：本講一句話總結

> 當電晶體微縮到只剩數十顆原子寬度時，光的波動繞射與粒子起伏造就了必須用球型相關多變量高斯與 Gumbel 極值分布建模的參數變異（靠 OPC／PSM 與體質分級化解），而隨時間推進的 NBTI／HCI 電晶體老化與四大指數依賴溫度的 RAMP 硬錯誤則再次證明「壓低晶片溫度」就是延長硬體壽命的最強銀彈。

---

*筆記整理自 Smruti R. Sarangi 教授 Advanced Computer Architecture 第 51 講影片與 Next-Gen Computer Architecture v3.2 第 12 章 §12.5–12.7，搭配 Linux v6.2.12 原始碼與本機實測驗證。*
