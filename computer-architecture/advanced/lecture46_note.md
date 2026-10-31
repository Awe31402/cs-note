# Advanced Computer Architecture 第四十六支影片完整筆記：功率與溫度（一）：動態功率、ED² 指標、短路功率與六種漏電機制（Chapter 11, Part I）

> **課程**：Advanced Computer Architecture — Chapter 11: Power and Temperature (dynamic and leakage power) --- Part I/III（播放清單第 46 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（47:40）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch11** §11.1（p.607–617）
> **章號**：影片 Chapter 11 = 課本 Ch11。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、NVIDIA RTX 4060 Laptop GPU、driver 580.178.04、nvcc 12.4、Ubuntu、kernel 6.5.0）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕把「PMOS」寫成「Primos」、「millijoules」寫成「millage rules」、「band-to-band」寫成「Bantu band」、「Boltzmann's」寫成「boseman's」、「V_DS」寫成「radius／various」、「Fowler-Nordheim」寫成「follow the nordheim」、「GIDL」寫成「gittle／griddle」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 為什麼在意功率與溫度](#1-為什麼在意功率與溫度)
- [2. 功率的兩大組成：dynamic 與 leakage](#2-功率的兩大組成dynamic-與-leakage)
- [3. 電壓轉換（voltage transition）與等效 RC 電路](#3-電壓轉換voltage-transition與等效-rc-電路)
- [4. 電容充放電推導：CV² 與 βCV²f](#4-電容充放電推導cv²-與-βcv²f)
- [5. 電壓與頻率的關係：Alpha Power Law](#5-電壓與頻率的關係alpha-power-law)
- [6. 怎麼公平比較兩種設計：ED² 指標](#6-怎麼公平比較兩種設計ed²-指標)
- [7. 短路功率（short-circuit power）](#7-短路功率short-circuit-power)
- [8. 漏電功率（leakage／static power）總覽](#8-漏電功率leakagestatic-power總覽)
- [9. I₁：P-N 接面逆向偏壓電流與 band-to-band tunneling](#9-i₁p-n-接面逆向偏壓電流與-band-to-band-tunneling)
- [10. I₂：次臨界漏電（subthreshold leakage）與短通道效應](#10-i₂次臨界漏電subthreshold-leakage與短通道效應)
- [11. I₃ 與 I₄：gate oxide tunneling 與 hot-carrier injection](#11-i₃-與-i₄gate-oxide-tunneling-與-hot-carrier-injection)
- [12. I₅ 與 I₆：GIDL 與 punchthrough current](#12-i₅-與-i₆gidl-與-punchthrough-current)
- [13. 各項功率的比重與小結](#13-各項功率的比重與小結)
- [14. 【實測】GPU 的靜態底線、activity factor、全零輸入與 ED²](#14-實測gpu-的靜態底線activity-factor全零輸入與-ed²)
- [15. 一頁速查表](#15-一頁速查表)
- [16. 與後續講次的連結](#16-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
總功率 P_total = 動態功率 P_dyn（50–70%，含約 10% 的短路功率）＋ 漏電／靜態功率 P_leak（20–40%）

1. 動態功率：只有電位轉換（0→1、1→0）才有電流流過等效電阻產生 I²R 熱損耗
   RC 充電：電源付出 CV²（一半 ½CV² 變熱、一半 ½CV² 存進電容，與 R 大小無關）
   RC 放電：電容裡的 ½CV² 也變熱 → 一次充放電循環共耗散 CV²
   整塊電路：P_dyn ∝ β C V² f（β = activity factor ∈ [0, 1]）
   Alpha Power Law：f ∝ (V − V_th)^α / V
     早期（α≈2, V≫V_th）：f ∝ V → P_dyn ∝ f³；現代（α≈1.1–1.5, V/V_th 變小）：P 隨 f 暴增更快
   ED² = P_dyn D³：在 V ∝ f、D ∝ 1/f 下對 DVFS 不變 → 比較不同設計的基準（越小越好）
   Short-circuit power：CMOS 反相器轉換瞬間 PMOS、NMOS 同時導通，約佔 P_dyn 的 10%

2. 漏電功率：非理想接面在截止狀態仍漏出微小電流，乘上數十億顆電晶體就佔 20–40%
   I₁ P-N 逆向偏壓電流：drain-body 逆偏，高電場把 p 型價帶電子拉進 n 型導帶（BTBT）
   I₂ Subthreshold leakage：V_g < V_th 仍有 I_ds；隨 T（40–80°C 近線性／分段線性）、V_DS 上升而增加（DIBL 雙重打擊）
     → 漏電 ↑ → 功率 ↑ → 溫度 ↑ → 漏電 ↑（發散就是 thermal runaway）
   I₃ Gate oxide tunneling：SiO₂ 極薄（< 2 nm），Fowler-Nordheim（進導帶）＋ direct tunneling（直接穿隧）
   I₄ Hot-carrier injection：高電場加速電子獲得高動能，「助跑跳過峽谷」衝進 gate
   I₅ GIDL：V_GD < 0 把電洞推擠在 drain-gate 交界並流向基板
   I₆ Punchthrough：短通道下 source、drain 逆偏空乏區在基板深處合併，不受 gate 控制
```

---

## 1. 為什麼在意功率與溫度

[00:12] 進入課本第三部分的第一章（第 11 章）：功率與溫度是現代硬體設計的**第一級考量**（first-order concerns）。

- 【口頭】[00:27] 老師說他剛開始錄這支影片時，無線麥克風沒電了，只好換另一支麥克風錄——「連麥克風裡都有一顆小處理器，你就知道電池壽命和功耗有多重要。」
- [00:45] 這一章除了 out-of-order pipeline 和 cache，還會用到一點基礎物理（熱流、流體、微積分）。
- [01:23] 幾十年前電腦只耗幾瓦；現在大型資料中心是 **MW（百萬瓦）等級**，「大到足以拖垮整座城市的供電」。
- [01:51] CPU 有多燙？投影片放了一張在處理器上直接**煎熟兩顆水波蛋**當英式早餐的照片。一般桌機 CPU 約 100 W（加 GPU 更高，筆電較低，整體在 **50–200 W** 之間），而一顆省電燈泡才 8–10 W——一顆處理器的功耗就抵得上一整間房子的燈和電扇。
- [02:56] 為什麼在意：
  1. **電費與冷卻成本**：[03:03] 資料中心買機器花 100 單位的錢，長期下來花在**冷卻**上的電費很容易超過 100 單位。
  2. **行動裝置電池壽命**與燙手的螢幕、狂轉的風扇（[03:31]）。
  3. **可靠度**：[04:01] 溫度越高，可靠度越低——而且故障機率隨溫度呈**指數上升**（詳見第 12 章，[第 51 支](lecture51_note.md)）。

【課本補充】課本 p.607–608：跑浮點程式時 CPU die 溫度會超過 **100°C**；手機長期高溫甚至會提高電池爆炸的機率。大型資料中心維持溫度所需的能量佔總耗能的 **20–40%**；2017 年全美資料中心耗電 900 億度（kWh），全球約 **3%** 的電力用在資料中心（Danilak, 2017）。

---

## 2. 功率的兩大組成：dynamic 與 leakage

[04:11] 第 11 章分三步：功率模型（本講）→ 溫度模型（[第 47 支](lecture47_note.md)）→ 功率與溫度管理（[第 48 支](lecture48_note.md)）。

[04:25] 功率主要有兩部分：

| 組成 | 什麼時候發生 | 原因 |
|---|---|---|
| **Dynamic power**（動態功率） | 處理器有**活動（switching activity）**時 | 電位轉換時電流對電容充放電，流經電阻產生熱損耗 |
| **Leakage／static power**（漏電／靜態功率） | **不管有沒有活動**都在耗散 | 理想上不該導通的接面並不完美，微小漏電流隨時都在流 |

【課本補充】課本 p.608（Way Point 14）先點出兩者透過**溫度**形成的迴圈（影片在 [34:12] 講）：

```
dynamic power ↑ → 總功率 ↑ → 溫度 ↑ → leakage power ↑ → 總功率 ↑ → 溫度 ↑ …
```

多數情況下這個級數會**收斂**；如果不收斂，溫度一路飆升，就叫 **thermal runaway**（熱失控），系統必須立刻強制關機。

---

## 3. 電壓轉換（voltage transition）與等效 RC 電路

[04:47]（課本圖 11.1）看兩個串聯的 CMOS 反相器 `I1`（PMOS `T1`、NMOS `T2`）和 `I2`（`T3`、`T4`）（CMOS 反相器基礎見 [Basic 第 18 支](../basic/lecture18_note.md)）：

- [05:07] 如果輸入**維持原值**，電路裡所有節點的電位都不變，理想上**沒有電流流動** → 動態功率 = **0**。
- [05:17] 如果輸入從 `0 → 1`：`I1` 的輸出要從 `1 → 0`，原本存在 `I2` 閘極電容上的電荷必須放電；反過來 `0 → 1` 時要經由 PMOS 充電。
- [06:06] 只要有電流流過任何媒介，媒介就有等效電阻，產生 **$I^2 R$ 熱損耗**——這就是動態功率。轉換完成進入新穩定態後，電流就停了。
- [07:07] 因此動態功率只取決於**電壓轉換（voltage transitions）的次數與位置**（跟現在的輸入、以及**前一個 cycle 的狀態**都有關，課本 p.610）。不同電晶體大小、不同 fan-out（一個輸出推幾個輸入）每次轉換的能量成本（nJ／pJ）不同；功率模擬器就是統計每個 cycle 的轉換次數乘上各自的能量成本，再除以 cycle time [07:31]。

[08:07] 怎麼算每次轉換的電流與功耗？跟 [第 24 支](lecture24_note.md)（課本 §7.3.3）一樣，把電晶體換成**等效 RC 電路**：閘極對 source、body、drain 各有電容，搭配受 $V_{GS}$、$V_{BS}$ 控制的電流源與等效電阻。

---

## 4. 電容充放電推導：CV² 與 βCV²f

[09:10]（課本圖 11.2）看最基本的串聯 RC 電路：充電時合上 `switch(1)` 接電壓源 $V$；放電時斷開 `switch(1)`、合上 `switch(2)` 接地。

**充電階段**（課本式 11.1–11.3）：由 $I = C \frac{dV_o}{dt}$ 與 $V_o = V - IR$ 解微分方程得：

$$I(t) = \frac{V}{R} e^{-t / RC}$$

其中 $RC$ 是時間常數 [09:38]。整個充電過程電壓源提供的總能量：

$$E_{\text{source}} = \int_0^\infty V \cdot I(t)\,dt = \int_0^\infty V \cdot \frac{V}{R} e^{-t/RC}\,dt = CV^2$$

[10:03] 非常關鍵的結論：**這個 $CV^2$ 跟電阻 $R$ 的大小完全無關！**

- 電容充飽到電壓 $V$ 時，儲存的靜電能是 $\frac{1}{2}CV^2$。
- 剩下的一半 **$\frac{1}{2}CV^2$** 在充電途中經由電阻變成**熱**散掉了 [10:14]。

**放電階段** [10:28]：電容裡存的那 $\frac{1}{2}CV^2$ 流過放電路徑的電阻，**全部**也變成熱散掉。

- 一次完整的「充電＋放電」循環，總共散掉 $\frac{1}{2}CV^2 + \frac{1}{2}CV^2 = CV^2$ 的熱能 [10:50]。
- 假設每個 cycle 電容不是充電就是放電（耗散 $\frac{1}{2}CV^2$），除以週期 $\tau = 1/f$，就得到動態功率與 $CV^2 f$ 成正比（課本式 11.4）。

**Activity factor $\beta$ 與整塊電路** [11:29]：

- 功能單元不是每個 cycle 都在動。乘上 **activity factor** $\beta \in [0, 1]$（`0` = 完全不動，`1` = 每個 cycle 都轉換）。
- [12:55] 對有很多電晶體（電容 $C_1, \dots, C_n$）的大電路，假設活動率相近，把電容合併成等效總電容（lumped capacitance $C$），式子形式不變（課本式 11.5）：

$$E_{\text{dyn}} \propto \beta C V^2, \qquad P_{\text{dyn}} \propto \beta C V^2 f$$

【課本補充】課本 p.612：即使不知道比例常數，只要量出某個 $\beta$ 下的功耗，就能推算不同活動率或不同 $V, f$ 下的功耗；實務上要大幅改變 $\beta$ 不容易，調 **$V$ 和 $f$** 才是最強的旋鈕（**DVFS**，Dynamic Voltage and Frequency Scaling）。

---

## 5. 電壓與頻率的關係：Alpha Power Law

[14:14] 頻率 $f$ 翻倍 → 週期 $T = 1/f$ 減半 → 電容必須在一半的時間內充飽 → 必須**提高供應電壓 $V$** 才能灌進更大電流、充得更快。

[15:00] 經驗法則 **Alpha Power Law**（課本式 11.6，Sarangi et al., 2008）：

$$f \propto \frac{(V - V_{\text{th}})^\alpha}{V}$$

| 世代 | 參數近似 | 推導結果 |
|---|---|---|
| **早期製程**（1990 年代末） | [15:26] $\alpha \approx 2$，且 $V \gg V_{\text{th}}$（可忽略 $V_{\text{th}}$） | $f \propto V \implies P_{\text{dyn}} \propto C V^2 f \propto f^3$（課本式 11.7）：**頻率翻倍，動態功率變 8 倍**（見 [Basic 第 47 支 §11](../basic/lecture47_note.md#11-現在power-wall-與-think-parallel)、[第 5 支](lecture05_note.md)） |
| **現代製程** | $V$ 與 $V_{\text{th}}$ 差距縮小，$\alpha$ 也變小 | [16:37] $P$ 對 $f$ 的次方更高，甚至接近 **$f^6$ 以上**：頻率只拉高 10%，功耗可能暴增 50%！ |

⚠️ **現代參數範圍的細微差異**：影片 [16:20] 說現在 $V$ 大約只有 $V_{\text{th}}$ 的 **2–3 倍**、$\alpha$ 在 **1.1–1.3** 之間；課本 p.612 寫 $V/V_{\text{th}}$ 在 **4–6** 之間、$\alpha$ 在 **1.1–1.5** 之間。兩者結論一致：$f \propto V$ 早已不再精確成立，實務上處理器直接內建一張離散的 **DVFS 電壓–頻率表**（[第 48 支 §1](lecture48_note.md)），而且單靠拉頻率已經走不通，必須靠架構技巧拉高 IPC [17:14]。

---

## 6. 怎麼公平比較兩種設計：ED² 指標

[17:38]（課本 p.612–613，Example 13）假設設計 B 比設計 A **快 10%**（執行時間 $D_B = 0.9 D_A$），但跑同一個程式多花 **20% 的能量**（$E_B = 1.2 E_A$）。誰比較好？

- 單看能量 A 贏，單看速度 B 贏；而且只要調一下 DVFS，任何設計都能用能量換速度。我們需要一個**不受 DVFS 電壓頻率縮放影響**的基準！
- [18:15] 黃金標準：**$ED^2$ 指標**（energy-delay-squared，等價於 $P_{\text{dyn}} D^3$，因為 $E = P_{\text{dyn}} D$）。

在傳統近似（忽略漏電、$V \propto f$、CPU-bound 程式執行時間 $D \propto 1/f$，忽略不隨 CPU 頻率縮放的 DRAM 延遲）下（課本式 11.8）：

$$ED^2 = P_{\text{dyn}} D^3 \propto (V^2 f) D^3 \propto f^3 \cdot \left(\frac{1}{f}\right)^3 = \text{常數}$$

- [19:44] **$ED^2$ 與 $V, f$ 無關**，只反映架構與電路本身的電容和活動率，是設計的本質屬性；**越小越好**。
- 代回例子：設 $ED^2(A) = 1$，則 $ED^2(B) = 1.2 \times 0.9^2 = 0.972 \approx 0.97 < 1$ → **設計 B 比較好** [20:07]。

---

## 7. 短路功率（short-circuit power）

[20:47]（課本圖 11.3）CMOS 反相器在輸入從 `0 → 1` 或 `1 → 0` 的**極短過渡期間**，上方的 PMOS 和下方的 NMOS 會**同時處於導通狀態**。

- 這一瞬間有一條從電源 $V_{DD}$ 直接穿過兩顆電晶體到地的電流路徑，稱為 **short-circuit current**，造成的熱損耗叫 **short-circuit power**。
- [21:48] 它跟充放電容的動態功率路徑不同，但只在信號轉換時發生，所以通常歸在動態功率底下當一個子類別，約佔動態功率的 **10%**。

【課本補充】課本 p.613：一般佔動態功率約 10%（Li et al., 2009）；但在某些 $V_{\text{th}}$ 相對 $V_{DD}$ 較高的設計裡，可能升到約 **25%**（Nose & Sakurai, 2000）。

---

## 8. 漏電功率（leakage／static power）總覽

[22:26] 理想電晶體模型假設電流只在切換時走 drain–source，gate 對 body、drain 對 body 等接面都是完美絕緣體。實際上**沒有完美絕緣體**：就算電晶體處於 **off（截止）狀態**、動態功率為 0，只要接在電源上，微小的漏電流就無時無刻不在流 [23:56]。

- 單顆電晶體的漏電比動態功率小至少一個數量級，但現代晶片有**數十億顆電晶體**（尤其佔大片面積的 **L2、L3 cache**，課本 p.614），而且同一時刻只有一小部分電晶體在切換，全部電晶體卻都在漏電！
- 加總起來，漏電功率佔高階處理器總功耗的 **20–40%** [23:43]。

[24:21]（課本圖 11.4）NMOS 電晶體裡的**六種漏電機制**（$I_1$ 到 $I_6$）：

```
                 Gate (polysilicon)
                   │  ▲
       SiO₂ (<2nm) ╪══╪══  I₃ Gate oxide tunneling / I₄ Hot-carrier injection
      ┌─────────┬──┴──┴──┬─────────┐
      │ Source  │ 通道 I₂ │  Drain  │
      │ (n-type)├────────► (n-type)│
      │         │ ══════►│    │  │ │
      └─────────┘   I₆   └────┼──┼─┘
                    Punch-    │  │ I₅ GIDL (V_GD < 0)
       p-type       through   ▼  ▼
      substrate (body)      I₁ P-N reverse-bias (BTBT)
```

---

## 9. I₁：P-N 接面逆向偏壓電流與 band-to-band tunneling

[26:57] NMOS 的基板（p 型）通常接地或低電位，drain（n 型）接高電位 → drain–body 這道 P-N 接面處於**逆向偏壓（reverse-biased）**。理想上逆偏不導通，實際上在高電場下會透過 **BTBT**（band-to-band tunneling，能帶對能帶穿隧）漏電。

[27:50] 複習固態物理的能帶（課本 p.614–615）：

- 量子力學規定電子只能佔據離散的能階；依 **Pauli 不相容原理**，電子不能擠在同一量子態。絕對零度（0 K）時電子填滿 **Fermi level** 以下的能階，以上全空；溫度升高時一部分電子跳到 Fermi level 以上。
- **價帶（valence band）**在 Fermi level 正下方（電子待在原處不導電）；**導帶（conduction band）**在正上方（電子可自由移動導電）。
  - **金屬**：兩帶重疊，天然導電。
  - **絕緣體**：能隙（band gap）極寬，加電場也跨不過去 [30:45]。
  - **摻雜半導體**：能隙很窄，給一點電位推力電子就能跨過能隙導電——這是電晶體能工作的優點，但反面就是漏電 [31:01]。
- **BTBT 怎麼發生** [29:49]：drain–body 接面電場極強時，直接把 p 型區價帶的電子「拉」進 n 型區的導帶（電子從 body 流向 drain → 等效電流 $I_1$ 從 drain 流向 body）。

---

## 10. I₂：次臨界漏電（subthreshold leakage）與短通道效應

[31:39] 當閘極電壓 $V_g < V_{\text{th}}$（截止區），通道裡仍有一股從 drain 流向 source 的微小電流，在 2015 年前一直是最大的漏電來源（課本式 11.9，Roy et al., 2003）：

$$I_{ds} = \mu_0 C_{\text{ox}} \frac{W}{L} (m - 1) v_T^2 \cdot e^{(V_g - V_{\text{th}}) / (m v_T)} \cdot \left(1 - e^{-V_{DS} / v_T}\right)$$

其中 $v_T = kT / q$ 是**熱電壓（thermal voltage）**，正比於絕對溫度 $T$ [34:48]。不必背整條公式，抓四個架構層級的重點：

1. **溫度 $T$ 越高，漏電越大** [32:41]：公式裡有 $v_T^2$ 和指數項，過去常說漏電隨溫度呈指數成長；但在處理器實際工作的 **40–80°C** 區間內，其實接近**線性或分段線性（piecewise linear）**（或微二次方）！
   - 【課本補充】課本 p.616（Sultan et al., 2018）：40–80°C 用線性模型誤差在 **6%** 內，用分段線性或二次模型誤差在 **1%** 內。
   - [34:12] 這就構成 [§2](#2-功率的兩大組成dynamic-與-leakage) 的正回授迴圈：漏電 ↑ → 功率 ↑ → 溫度 ↑ → 漏電 ↑；若發散就是致命的 **thermal runaway**。
2. **供應電壓（$V_{DS}$）越高，漏電越大** [33:11]：$V_{DS} \uparrow \implies e^{-V_{DS}/v_T} \downarrow \implies (1 - e^{-V_{DS}/v_T}) \uparrow$。
3. **臨界電壓 $V_{\text{th}}$ 越低，漏電越大** [33:59]：$e^{(V_g - V_{\text{th}})/mv_T}$ 指數上升。
4. **短通道效應：DIBL 的「雙重打擊」** [35:20]：現代短通道電晶體 source 和 drain 靠得很近，$V_{\text{th}}$ 不再是常數——**拉高 $V_{DS}$ 會連帶把 $V_{\text{th}}$ 往下拉**（課本 p.616 稱為 **DIBL**，drain-induced barrier lowering）。老師笑稱這就是莫非定律：「為了跑高頻率把電壓拉高，不只直接讓漏電變大，還順便壓低 $V_{\text{th}}$ 再讓次臨界漏電暴增一次（double whammy）！」

【課本補充】課本 p.616 還提到 **body bias**（基板偏壓，[第 48 支](lecture48_note.md) 會細講）：對 body–source 加逆向偏壓會提高 $V_{\text{th}}$（省漏電但變慢）；加順向偏壓會降低 $V_{\text{th}}$（變快但漏電增加）。

---

## 11. I₃ 與 I₄：gate oxide tunneling 與 hot-carrier injection

這兩種都是載子穿過原本應該絕緣的 **$\text{SiO}_2$ 閘極氧化層**（矽基板 ↔ polysilicon 閘極），但物理機制一量子、一古典：

| 機制 | 物理原因 | 細節 |
|---|---|---|
| **$I_3$：Gate oxide tunneling** [36:55] | **量子穿隧** | 氧化層縮到 **< 2 nm**（⚠️ 課本 p.616 寫 < 3–4 nm），距離極小讓電場 $E = V/d$ 極大：<br>1. **Fowler-Nordheim (FN) tunneling**：外加電場把電子拉進 $\text{SiO}_2$ 的導帶而導通。<br>2. **Direct tunneling**：因為電子的波動性與位置不確定性，直接從矽表面穿過薄薄幾 nm 跳進 polysilicon 閘極 |
| **$I_4$：Hot-carrier injection** [39:44] | **古典動能** | 強電場加速載子，少數電子累積足夠動能（變成「熱載子」），像**助跑跳過狹窄峽谷**一樣直接越過 $\text{Si–SiO}_2$ 介面衝進帶正電的閘極（電子往上跳 → 等效電流往下） |

【課本補充】課本 p.616：hot-carrier injection 長期下來還會撞壞閘極氧化層，造成電晶體參數漂移（**aging** 老化）與崩潰（詳見第 12 章，[第 51 支](lecture51_note.md)）。

---

## 12. I₅ 與 I₆：GIDL 與 punchthrough current

- **$I_5$：Gate-induced drain leakage（GIDL）** [41:37]：
  - 發生在閘極電位低於汲極電位（**$V_{GD} < 0$**，例如 gate = 0.1 V、drain = 1 V）時。
  - 電場把帶正電的電洞（holes）往閘極氧化層下方推擠，聚集在 drain–gate 交界處，再往電洞濃度較低的基板（body）遷移 → 形成從 drain 流向基板的漏電流。
  - ⚠️ 【口頭】[42:59] 老師口頭補充說數學上常把這個效應建模成對 $V_{\text{th}}$ 的微擾；課本 p.617 則是直接以少數載子（電洞）聚集在 drain–gate 交界並流向基板來描述。
- **$I_6$：Punchthrough current（穿通電流）** [43:25]：
  - 發生在**短通道**電晶體：drain–substrate 與 source–substrate 兩個接面都逆向偏壓時，各自的空乏區（depletion region）往外擴張，通道太短就**直接合併成同一塊大空乏區** [44:11]（「在哲學意義上變成同一個元件」）。
  - 降低了 source 與 drain 之間的位能障壁，而且路徑位在**基板深處**，閘極電壓很難控制住它（課本 p.617），於是載子直接從 source 穿通到 drain，疊加在次臨界漏電之上。

---

## 13. 各項功率的比重與小結

[45:29] 大致的數量級比重（ballpark figures）：

- **Dynamic power**：**50–70%**（Short-circuit power 約佔其中的 10%，通常直接算在同一桶裡）。
- **Leakage／static power**：⚠️ 投影片最後這頁寫 **20–30%**（「有時更高，靠現代漏電控制壓在這裡」）；前面 [23:46]、[24:17] 與課本 p.608、p.614 則寫 **20–40%**。無論哪個數字，漏電都大到絕不能忽略。

[46:17] 預告下一講進入全書物理與微分方程最重的**溫度模型**（[第 47 支](lecture47_note.md)）。

---

## 14. 【實測】GPU 的靜態底線、activity factor、全零輸入與 ED²

本機 CPU 的 RAPL 功率計數器（`/sys/class/powercap/intel-rapl:0/energy_uj`）在 Ubuntu kernel 6.5 下預設只給 root 讀（`Permission denied`；這是為了防功率旁通道攻擊，見 [第 54 支](lecture54_note.md)），CPU 瓦數**待補**。不過本機的 **NVIDIA RTX 4060 Laptop GPU**（Ada Lovelace，24 個 SM）透過 NVML（`libnvidia-ml`）可以在一般權限下直接讀取毫瓦級功耗、SM 頻率與溫度！

程式在 `experiments/lecture46/gpu_power.cu`：

```cuda
// experiments/lecture46/gpu_power.cu:10-23
__global__ void fma_burn(float *out, float init_a, float init_b, long long iters) {
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    float a = init_a * (1.0f + (tid & 31) * 1e-4f);
    float b = init_b;
    float c0 = a, c1 = -a, c2 = a * 0.5f, c3 = -a * 0.5f;
    for (long long i = 0; i < iters; i++) {
        c0 = __fmaf_rn(c0, b, init_a);
        c1 = __fmaf_rn(c1, b, -init_a);
        c2 = __fmaf_rn(c2, b, init_a);
        c3 = __fmaf_rn(c3, b, -init_a);
        b = -b;
    }
    if ((c0 + c1 + c2 + c3) == 123456.0f) out[tid] = c0;
}
```

我們固定總運算量為 **29.5 TFLOP**（`total_blk_iters = 48 * 3e8` 分給 `b` 個 block 跑），比較三件事：
1. 建立 CUDA P0 context 但跑 **0 個 block**（晶片上電、時脈拉到 2475 MHz，但運算單元 $\beta = 0$）。
2. `Toggle` 模式（`init_a = 0.9999f, init_b = -1.00001f`，每輪翻轉正負號讓浮點數 bit 不斷切換），分別只用 **6、12、24 個 SM**（改變整顆晶片的 $\beta$）。
3. `Zero` 模式（`init_a = 0.0f, init_b = 0.0f`）：同樣 48 個 block 跑滿 24 個 SM、執行**一模一樣數量的 29.5 兆次 FMA 指令**，但暫存器裡永遠是全 0，FMA 資料路徑內**沒有任何 0↔1 電位轉換**（驗證 [§3](#3-電壓轉換voltage-transition與等效-rc-電路)）！

實測結果：

| 狀態／設定（固定 29.5 TFLOP） | 平均功率 $P$ | SM 頻率 $f$ | 執行時間 $D$ | 總能量 $E = PD$ | $ED^2$（$\text{J}\cdot\text{s}^2$） | 溫度 |
|---|---|---|---|---|---|---|
| Idle（未開 CUDA context） | **9.80 W** | 2490 MHz | — | — | — | 59°C |
| Idle（已開 P0 context，0 blocks） | **15.71 W** | 2475 MHz | — | — | — | 59°C |
| Toggle：6 blocks（6/24 SM） | 38.38 W | **2475 MHz** | 8.40 s | 322.3 J | 22719 | 70°C |
| Toggle：12 blocks（12/24 SM） | 44.48 W | 2287 MHz | 4.54 s | 202.1 J | 4172 | 73°C |
| Toggle：24 blocks（24/24 SM） | 44.97 W | 1806 MHz | 2.89 s | 130.1 J | 1089 | 74°C |
| Toggle：48 blocks（24/24 SM） | **45.02 W** | **1814 MHz** | **2.85 s** | **128.5 J** | **1047** | 74°C |
| **Zero**：48 blocks（24/24 SM） | **44.90 W** | **2129 MHz** | **2.41 s** | **108.3 J** | **631** | 73°C |

四個直接對應課本與影片的觀察：

1. **靜態／底線功率佔比約 35%**：光把 GPU 喚醒到 P0 電壓與頻率、一個 kernel 都不跑，就要耗掉 **15.71 W**（佔滿載 45 W 上限的 **35%**，包含數十億顆電晶體的漏電功率與未關閉的背景時脈網路，落在 [§8](#8-漏電功率leakagestatic-power總覽) 說的 20–40% 範圍內）。（如果在 2475 MHz 定頻下只從 1 個 SM 增加到 3、6 個 SM，功率會從 25.3 W → 30.3 W → 35.5 W 隨 $\beta$ 穩定爬升。）
2. **撞上 45 W 功耗牆後的自動 DVFS**：從 12 個 block 到 24 個 block，$\beta$ 翻倍，但筆電 GPU 被韌體鎖在 **45 W** 上限；為了不破表，GPU 自動把 SM 頻率（連同電壓）從 **2475 MHz 降到 1806 MHz**（降了 27%）才把總功率壓在 45 W——反過來印證 [§5](#5-電壓與頻率的關係alpha-power-law) 電壓頻率對功耗的巨大槓桿。
3. **沒有電位轉換就省動態能量（`Toggle` vs `Zero`）**：兩者跑完全相同的指令數與 block 數，但 `Zero` 模式的 FMA 運算元維持 `0.0f` 不翻轉（只有指令擷取、解碼、迴圈計數器還有轉換），完成同樣工作總能量從 **128.5 J 降到 108.3 J**（省了 **15.7%**）！因為每個 cycle 耗能變少，在同樣 45 W 的功率上限下，GPU 自動把 `Zero` 模式的頻率超到 **2129 MHz**（比 `Toggle` 的 1814 MHz 高出 **17.4%**），快了 0.44 秒跑完。
4. **$ED^2$ 與靜態能量的拖累**：只開 6 個 SM 雖然跑在最高頻率 2475 MHz，但拖了 8.40 秒才做完，那 15.7 W 的底線功率跟著白燒了 8.4 秒，總能量高達 322.3 J、$ED^2$ 高達 22719；開滿 24 個 SM 降頻跑，時間縮成 1/3、能量剩 40%、**$ED^2$ 小了 21 倍**（這也直接呼應 [第 48 支](lecture48_note.md) 課本 Example 14 的結論）。

---

## 15. 一頁速查表

| 主題 | 重點 |
|---|---|
| 為什麼在意 | 資料中心達 MW 級（冷卻佔 20–40% 能耗）、CPU 50–200 W（超過 100°C）、行動電池壽命、故障率隨溫度指數上升 |
| 兩大功率 | Dynamic（有電位轉換才耗散，50–70%）＋ Leakage／Static（截止態仍經非理想接面漏電，20–40%） |
| RC 充放電 | 充電電源給 $CV^2$（$\frac{1}{2}CV^2$ 存電容、$\frac{1}{2}CV^2$ 變熱，**與 $R$ 無關**）；放電 $\frac{1}{2}CV^2$ 也變熱 → 一循環 $CV^2$ |
| 動態功率公式 | $P_{\text{dyn}} \propto \beta C V^2 f$（$\beta \in [0, 1]$ 為 activity factor，$C$ 為 lumped capacitance） |
| Alpha Power Law | $f \propto (V - V_{\text{th}})^\alpha / V$；早期 $\alpha \approx 2, V \gg V_{\text{th}} \Rightarrow P \propto f^3$；現代 $\alpha \approx 1.1\text{–}1.5$、$V/V_{\text{th}}$ 小 $\Rightarrow$ 拉頻率功耗暴增更快 |
| $ED^2$ 指標 | $ED^2 = P_{\text{dyn}} D^3$；在 $V \propto f, D \propto 1/f$ 下對 DVFS 縮放為常數，用來比較不同架構設計（越小越好） |
| Short-circuit power | CMOS 反相器切換瞬間 PMOS、NMOS 同時導通直通地；約佔 $P_{\text{dyn}}$ 的 10% |
| $I_1$ P-N 逆偏電流 | drain–body 逆偏，強電場把 p 型價帶電子拉進 n 型導帶（**BTBT**，band-to-band tunneling） |
| $I_2$ Subthreshold | $V_g < V_{\text{th}}$ 的通道電流；40–80°C 對溫度近似線性／分段線性；短通道 **DIBL**（$V_{DS} \uparrow \Rightarrow V_{\text{th}} \downarrow$）雙重打擊 |
| 熱失控 | $P \uparrow \to T \uparrow \to P_{\text{leak}} \uparrow \to P \uparrow$；若不收斂即 **thermal runaway**，必須立刻關機 |
| $I_3$ Gate oxide tunneling | $\text{SiO}_2 < 2\text{ nm}$ 量子穿隧：Fowler-Nordheim（進氧化層導帶）＋ direct tunneling（直接穿到 gate） |
| $I_4$ Hot-carrier injection | 強電場加速電子獲得高古典動能，越過 $\text{Si–SiO}_2$ 介面衝進 gate（也會造成氧化層老化） |
| $I_5$ GIDL / $I_6$ Punchthrough | GIDL：$V_{GD} < 0$ 使電洞聚集在 drain–gate 並流向基板；Punchthrough：短通道下 source/drain 空乏區在深處合併 |
| 實測 | RTX 4060 P0 閒置就吃 15.7 W（~35%）；全零運算元沒有位元轉換，同樣 29.5 TFLOP 省 15.7% 能量、同功耗上限下時脈高 17.4% |

---

## 16. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Power wall、頻率停滯的歷史背景 | [Basic 第 47 支 §11](../basic/lecture47_note.md#11-現在power-wall-與-think-parallel)、[第 5 支](lecture05_note.md) |
| CMOS 反相器、等效 RC 電路 | [Basic 第 18 支](../basic/lecture18_note.md)、[第 24 支](lecture24_note.md) |
| 非揮發記憶體幾乎無漏電 vs SRAM/DRAM | [第 45 支](lecture45_note.md) |
| 溫度模型（Fourier 熱傳導方程、FDM、FEM、Green's function） | [第 47 支](lecture47_note.md) |
| 功率與溫度管理（DVFS、clock/power gating、body bias、drowsy cache） | [第 48 支](lecture48_note.md) |
| 高溫與 hot-carrier injection 對老化與可靠度的影響 | [第 51 支](lecture51_note.md) |
| 利用功耗變化偷金鑰的功率旁通道攻擊 | [第 54 支](lecture54_note.md) |

---

## 附：本講一句話總結

> **處理器功耗分成有電位轉換才發生的動態功率（含約 10% 反相器瞬間直通的短路功率）和截止態仍經非理想接面漏出的靜態漏電功率（乘上數十億顆電晶體佔 20–40%）：RC 充放電一循環必耗散與電阻無關的 $CV^2$，給出 $P_{\text{dyn}} \propto \beta C V^2 f$，搭配 Alpha Power Law 解釋了為何拉高頻率會讓功耗以 $f^3$ 甚至更高次方暴增，而對 DVFS 不變的 $ED^2$ 則是比較設計優劣的基準；漏電涵蓋 BTBT、隨溫度上升並受 DIBL 雙重打擊的次臨界漏電（與溫度形成可能引發 thermal runaway 的正回授）、閘極氧化層量子穿隧、高動能熱載子注入、GIDL 與短通道空乏區合併的 punchthrough。本機 RTX 4060 實測：光開 P0 時脈不跑運算就耗 15.7 W（佔 45 W 上限的 35%），而跑同樣 29.5 兆次 FMA 指令時，全零輸入因資料路徑無位元翻轉比翻轉輸入省下 15.7% 能量、並在同功耗上限下自動超頻 17.4%。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 11: Power and Temperature (dynamic and leakage power) --- Part I/III 的 YouTube 自動字幕（47:40）與 Sarangi *Next-Gen Computer Architecture* v3.2 Ch11 §11.1（p.607–617）整理。實測為本機（Intel i7-13650HX、NVIDIA RTX 4060 Laptop GPU、driver 580.178.04、nvcc 12.4、Ubuntu、kernel 6.5.0）結果。*