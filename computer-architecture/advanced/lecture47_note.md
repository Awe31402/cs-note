# Advanced Computer Architecture 第四十七支影片完整筆記：功率與溫度（二）：晶片散熱封裝、Fourier 熱傳導方程、FDM／電路類比、FEM 與 Green's function（Chapter 11, Part II）

> **課程**：Advanced Computer Architecture — Chapter 11: Power and Temperature (temperature modeling) --- Part II/III（播放清單第 47 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（52:00）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch11** §11.2（p.617–625）
> **章號**：影片 Chapter 11 = 課本 Ch11。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10、NumPy 1.26）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕把「BGA (ball-grid array)」寫成「ball grade array」、「adiabatic」寫成「diabetic」、「$\nabla^2$ (nabla squared)」寫成「nebula／navbla／Lambda Square」、「Dirac delta」寫成「direct Delta」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 晶片封裝與散熱結構](#1-晶片封裝與散熱結構)
- [2. 三種熱傳機制與垂直／水平傳導](#2-三種熱傳機制與垂直水平傳導)
- [3. Fourier 熱傳導定律與三維偏微分方程推導](#3-fourier-熱傳導定律與三維偏微分方程推導)
- [4. 有限差分法（FDM）：穩態 T = AP 與暫態 GT + P = CT′](#4-有限差分法fdm穩態-t--ap-與暫態-gt--p--ct)
- [5. 熱傳問題的電路類比（electrical analog）](#5-熱傳問題的電路類比electrical-analog)
- [6. 有限元素法（FEM）](#6-有限元素法fem)
- [7. Green's function：線性疊加、平移不變性與 2D 摺積](#7-greens-function線性疊加平移不變性與-2d-摺積)
- [8. 漏電–溫度回授迴圈與 leakage-aware Green's function](#8-漏電溫度回授迴圈與-leakage-aware-greens-function)
- [9. 【原始碼】Linux `coretemp` 怎麼讀晶片溫度與 1 秒快取](#9-原始碼linux-coretemp-怎麼讀晶片溫度與-1-秒快取)
- [10. 【實測】本機核心橫向傳熱與 2D FDM vs Green's function](#10-實測本機核心橫向傳熱與-2d-fdm-vs-greens-function)
- [11. 一頁速查表](#11-一頁速查表)
- [12. 與後續講次的連結](#12-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
封裝由下而上：PCB ─ BGA ─ Silicon die（約 2×2 cm）─ [TIM] ─ Heat spreader（銅鎳合金）─ [TIM] ─ Heat sink（散熱鰭片）─ Fan
  大部分熱往上傳導（vertical），少部分橫向傳導（lateral，造成鄰近核心互相加熱）；側面視為絕熱（adiabatic）

物理推導（Fourier's Law）：
  1D：q_x = −k A (dT/dx)
  3D 微小體積 dV = dx dy dz 留下的熱功率 + 內部發熱 = k ∇²T dV + Q' dV = ρ c_p dV (∂T/∂t)
  → 偏微分方程：∇²T + Q'/k = (1/α)(∂T/∂t)，其中 α = k/(ρ c_p)；穩態（∂T/∂t = 0）：∇²T = −Q'/k

三種數值解法（精確度 vs 速度）：
  1. FDM（有限差分法）：切成 N 格，二階導數換差分
     穩態：T = A P（N×N 矩陣乘向量，O(N²)）；暫態：G T + P = C T'
     → 電路類比（T ↔ 電壓 V、P ↔ 電流 I、G ↔ 熱電導、C ↔ 對地熱電容）→ HotSpot、SPICE 可解
  2. FEM（有限元素法）：乘測試函數 v() 轉積分方程 → 三角形／四面體網格（meshing）→ 基底函數加權和；最準但遠慢於 O(N²)
  3. Green's function：利用線性疊加 + 平移不變性（邊界外），把 1 W 點熱源（Dirac δ）的溫度場當 G
     → T = P ⋆ G（2D 摺積，轉 Fourier 頻域變乘法，大幅加速；邊緣用鏡像法 method of images 修正）
```

---

## 1. 晶片封裝與散熱結構

[00:21] 第一講談了功率模型，這一講要回答：**給定晶片各處消耗的功率分布（power profile），怎麼算出整片矽晶粒的溫度地圖（temperature map）？**

[00:37]（課本圖 11.5）看一顆處理器封裝的剖面：

```
            ┌───────────────────────────┐
            │        Fan（風扇）         │  ← 吹風帶走熱空氣（對流）
          ┌─┴─┬──┬──┬──┬──┬──┬──┬──┬──┬─┴─┐
          │   │  │  │  │  │  │  │  │  │   │  ← Heat sink（散熱器，大量鰭片拉大表面積）
          ├───┴──┴──┴──┴──┴──┴──┴──┴──┴───┤
              ┌───────────────────────┐      ←（中間夾 TIM 導熱膏）
              │     Heat spreader     │      ← 銅鎳合金，把局部熱點攤平
              └───────┬───────┬───────┘      ←（中間夾 TIM 導熱膏）
                      │  Die  │              ← 矽晶片（~400–500 mm² ≈ 2 cm × 2 cm）
                      └─ooooo─┘              ← BGA 錫球接腳
          ═════════════════════════════════  ← PCB 主機板
```

- **Silicon die（矽晶片）** [00:42]：連大顆的伺服器晶片也不過 400–500 mm²（約 **2 cm × 2 cm**）。
  - 【課本補充】課本 p.617：功耗主要發生在有電晶體的邏輯層，所以晶片通常**倒裝（upside down）**：電晶體層朝上貼近散熱端，金屬連線層朝下接 BGA。
- **BGA（ball-grid array）與 PCB** [00:58]：晶片底部透過金屬錫球陣列接到主機板（傳 I/O 訊號與電源／地線）。
- **Heat spreader（均熱片／頂蓋）** [01:20]：晶片內部會形成局部高溫**熱點（hotspots）**，所以在上面蓋一塊面積更大的均熱片把熱攤勻。
  - ⚠️ **尺寸數字**：影片 [01:37] 口頭比劃說約 5×5 或 6×6 cm（「大概大拇指大小」）；課本 p.618 寫典型值為 **4 cm × 4 cm、厚約 5 mm**。
  - 【口頭】[02:51]、[03:42] 材質是**鎳銅合金（nickel-copper alloy）**，熱傳導率約 **369 W/(m·K)**（老師在 [02:57] 不小心講成 W/K，[03:42] 特別回頭更正單位是 W/(m·K)，並補充這是他們實驗室親自拆開一顆處理器量出來的數字，誤差約 ±10%）。
  - 【課本補充】課本 p.618：固體表面接觸有微小空隙，所以晶片與 spreader、spreader 與 heat sink 之間都要塗一層膠狀的 **TIM**（thermal interface material，導熱介面材料／散熱膏）確保接觸導熱。
- **Heat sink（散熱器）與風扇** [02:00]、[04:19]：上面長滿**鰭片（fins）**，目的是人為大幅增加跟空氣接觸的表面積；頂端再加風扇強制空氣流通（[04:30] 投影片展示實際主機板上的記憶體、大型鰭片散熱器、風扇與水冷管線）。

---

## 2. 三種熱傳機制與垂直／水平傳導

[05:23]（課本 p.619）基礎物理的三種熱傳方式：

1. **Conduction（傳導）**：直接接觸傳熱——晶片與封裝**內部最主要**的熱傳機制。
2. **Convection（對流）**：靠流體（空氣或水）受熱膨脹上升、冷流體遞補循環——發生在 **heat sink 鰭片與空氣／風扇**交界處 [05:39]、[06:30]。
3. **Radiation（輻射）**：紅外線等電磁波——研究恆星、黑體時很重要，但在晶片散熱裡微乎其微，可忽略 [06:09]。

**垂直 vs 橫向傳導與邊界條件** [02:34]：

- **垂直熱傳導（vertical）**佔絕大多數：熱往上穿過 spreader 到 heat sink 散進空氣。
- **橫向熱傳導（lateral）**：雖然量比垂直小，但會讓熱在矽晶片或 spreader 裡往旁邊擴散，是決定工作排程與元件擺放（[第 48 支 §8](lecture48_note.md)）的關鍵！
- **邊界假設**：側面沒有熱流穿過，稱為 **adiabatic（絕熱）邊界** [03:21]。
  - ⚠️ **底部 PCB 熱流的說法差異**：影片 [03:08] 說實際上橫向加上往下經 PCB 流走的熱「不超過 **20–30%**」；課本 p.618（Definition 94）做架構層級建模時，直接把**側面和底部（接 PCB 那一面）都近似成 adiabatic 絕熱邊界**，而 heat sink 對環境空氣則是恆溫的 **isothermal（等溫）邊界**（環境溫度 ambient temperature 視為常數）。

【課本補充】課本 p.618–619（Definition 95）區分兩種分析：
- **Steady-state analysis（穩態分析）**：假設功率分布長期不變，求穩定後的溫度分布（最常用）。
- **Transient analysis（暫態分析）**：求溫度隨時間的變化。晶片的**熱時間常數約在毫秒（ms）等級**，關心 ms 級別變化時才需要算暫態，但計算量比穩態貴得多。

---

## 3. Fourier 熱傳導定律與三維偏微分方程推導

[06:41]（課本圖 11.6，式 11.10–11.16）看一塊截面積 $A$、沿 $x$ 軸厚度 $\Delta x$（取極限寫成 $dx$）的薄立方體，左側面溫度 $T_1$ 高於右側面 $T_2$：

**第一步：1D Fourier 熱傳導定律** [08:06]（課本式 11.10）：

$$q_x = -k A \frac{dT}{dx}$$

- $q_x$：從左側面流入的熱功率（單位 **W**）；$q_x / A$ 是熱通量（heat flux）。
- $k$：材料的**熱傳導率（thermal conductivity）**。
- 為什麼有**負號**？因為熱從高溫流向低溫：若沿 $+x$ 方向熱往右流（$q_x > 0$），溫度沿 $+x$ 是下降的（$dT/dx < 0$）[09:03]。（【口頭】[08:18] 老師自嘲投影片把左側面標成 A、面積也寫成 $A$ 是壞習慣，請大家別學。）

**第二步：3D 微小體積內留下的熱功率** [10:21]（課本式 11.11–11.12）：

取邊長 $dx, dy, dz$（體積 $dV = dx\,dy\,dz$）的小方塊，$x$ 方向截面積 $A = dy\,dz$，且溫度同時隨 $x, y, z, t$ 變，改寫偏導數 $q_x = -k (dy\,dz) \frac{\partial T}{\partial x}$。從左面流進 $q_x$、右面流出 $q_{x+dx}$，**留在小方塊裡**的淨熱功率：

$$q_x - q_{x+dx} = -dx \frac{\partial q_x}{\partial x} = -dx \frac{\partial}{\partial x}\left(-k\,dy\,dz\,\frac{\partial T}{\partial x}\right) = k\,dV \frac{\partial^2 T}{\partial x^2}$$

[13:06] 熱能不像電磁波會相消干涉，三個軸留下的熱功率直接相加 [13:31]；定義 Laplace 算子 $\nabla^2 = \frac{\partial^2}{\partial x^2} + \frac{\partial^2}{\partial y^2} + \frac{\partial^2}{\partial z^2}$ [14:09]，三軸合計留在 $dV$ 內的熱功率就是 $k\,dV\,\nabla^2 T$。

**第三步：加上內部發熱，連結時間導數** [15:14]（課本式 11.13–11.16）：

- 若 $dV$ 裡面有電晶體在耗散動態與靜態功率，設**單位體積內部發熱功率**為 $Q'$（單位 $\text{W/m}^3$），則流入＋自產的總儲存熱功率 $P_{\text{st}} = (k \nabla^2 T + Q') dV$。
- [17:33] 由比熱定義，質量 $\rho\,dV$、比熱 $c_p$ 的小方塊每秒升溫 $\frac{\partial T}{\partial t}$（注意大寫 $T$ 是溫度、小寫 $t$ 是時間 [18:38]）所需的功率為 $P_{\text{st}} = \rho\,dV\,c_p \frac{\partial T}{\partial t}$。
- 兩式相等並消掉 $dV$，得到 **Fourier 熱傳導方程**（課本式 11.14–11.15）：

$$k \nabla^2 T + Q' = \rho c_p \frac{\partial T}{\partial t} \quad\Longleftrightarrow\quad \nabla^2 T + \frac{Q'}{k} = \frac{1}{\alpha} \frac{\partial T}{\partial t}, \qquad \alpha = \frac{k}{\rho c_p}$$

兩個重要特例 [19:44]：
1. 無內部熱源（$Q' = 0$）：$\nabla^2 T = \frac{1}{\alpha} \frac{\partial T}{\partial t}$。
2. **穩態（steady state，$\frac{\partial T}{\partial t} = 0$）**：$\nabla^2 T = -\frac{Q'}{k}$（課本式 11.16）——內部產生的熱剛好被往外擴散的空間曲率抵銷。

---

## 4. 有限差分法（FDM）：穩態 T = AP 與暫態 GT + P = CT′

[20:52]（課本 §11.2.3，p.621–622）怎麼在電腦上解上面的偏微分方程？最直接的方法是**有限差分法（Finite Difference Method, FDM）**：把晶片切成 $N$ 個小格子（2D 方格或 3D 小方塊），把二階偏導數換成相鄰格子的中央差分（課本式 11.17）：

$$\left.\frac{\partial^2 T}{\partial x^2}\right|_{i, t} \approx \frac{\frac{T_{i+1,t} - T_{i,t}}{\Delta x} - \frac{T_{i,t} - T_{i-1,t}}{\Delta x}}{\Delta x} = \frac{T_{i+1,t} - 2 T_{i,t} + T_{i-1,t}}{\Delta x^2}$$

$y, z$ 軸同理（邊界格依邊界條件另外列式），整個 PDE 就變成一組**線性代數方程組**！

- **穩態解** [22:30]（課本式 11.18）：把 $N$ 格的發熱功率排成 $N \times 1$ 行向量 $\mathbf{P}$、溫度排成 $\mathbf{T}$，線性轉換可寫成：
  $$\mathbf{T} = \mathbf{A} \mathbf{P}$$
  其中 $\mathbf{A}$ 是由幾何與材料決定的 $N \times N$ 矩陣（可由差分方程推導求逆，或由量測學習）。一旦有了 $\mathbf{A}$，每次給新的 $\mathbf{P}$ 做一次矩陣乘向量要 **$O(N^2)$** 時間——當 $N$ 很大時還是嫌慢 [24:26]。
- **暫態解** [25:04]（課本式 11.19–11.20）：保留時間導數項 $\mathbf{T}' = d\mathbf{T}/dt$，$k\nabla^2 \mathbf{T}$ 寫成矩陣 $\mathbf{G}\mathbf{T}$，各格的 $\rho c_p dV$ 排成對角矩陣 $\mathbf{C}$：
  $$\mathbf{G}\mathbf{T} + \mathbf{P} = \mathbf{C}\mathbf{T}'$$
  把時間切成 $\Delta t$，用 $\mathbf{T}' \approx (\mathbf{T}_{k+1} - \mathbf{T}_k)/\Delta t$ 就能一步步往後推進求解 [26:39]。

---

## 5. 熱傳問題的電路類比（electrical analog）

[27:06]（課本 §11.2.4，圖 11.7，p.622–623）仔細看 $\mathbf{G}\mathbf{T} + \mathbf{P} = \mathbf{C}\mathbf{T}'$，它的數學結構跟 RC 電路一模一樣！我們直接做一對一對應：

| 熱學量（Thermal） | 電學類比（Electrical） | 物理公式對照 |
|---|---|---|
| **溫度 $T$** | **電壓 $V$** | 節點電位代表該格溫度 |
| **熱功率 $P$（W）** | **電流 $I$（A）** | 發熱單元 = 往節點灌電流的**電流源**；恆溫環境 = 固定**電壓源** |
| **熱電導矩陣 $\mathbf{G}$**（熱阻倒數） | **電導 $G = 1/R$** | 1D 熱阻 $\frac{\Delta x}{k A} \leftrightarrow$ 電阻 $\frac{\rho l}{A}$（歐姆定律 $\Delta T = q \cdot R_{\text{th}} \leftrightarrow \Delta V = I R$） |
| **熱電容矩陣 $\mathbf{C}$** | **對地電容 $C$** | $P_{\text{st}} = (\rho\,dV\,c_p)\frac{\partial T}{\partial t} \leftrightarrow I = C \frac{dV}{dt}$（每個格子接一顆**對地電容**） |
| **Adiabatic 絕熱邊界** | **開路（open circuit）** | 邊緣格子不接往外界的電阻，電流（熱流）出不去 |

[29:34] 這樣就把整顆晶片與散熱器轉成一張**等效熱電路（thermal circuit）**！好處是電子工程界幾十年來已經把電路模擬器（如 SPICE）最佳化到極致；電腦架構界最著名的溫度模擬器 **HotSpot**（Huang et al., 2006，[37:42]）正是建立這個等效 RC 電路來解溫度。

---

## 6. 有限元素法（FEM）

[32:04]（課本 §11.2.5，p.623–624）如果要連 TIM 膠層、PCB 走線、複雜幾何形狀與空間變化的材料特性都精確算進去，商用的計算流體力學（CFD）工具會改用**有限元素法（Finite Element Method, FEM）**：

1. **轉成積分方程** [32:25]：把原微分方程兩邊乘上一個平滑的**測試函數（test function）** $v()$ 再積分。
2. **網格剖分（meshing）** [33:07]：把幾何區域切成許多小**元素（elements）**——2D 常用**三角形**（三個頂點必共面，能拼出任意複雜曲面，跟 [第 18 支](lecture18_note.md) GPU 圖學管線用三角形是同一個道理 [33:44]！），3D 常用**四面體（tetrahedrons）**或長方體；元素相接的頂點稱為 **nodes（節點）**。
3. **基底函數展開** [34:38]：把元素內任意點 $x$ 的未知溫度函數 $f(x)$ 寫成周圍節點值 $f_k$ 乘上各節點對應**基底函數（basis functions）** $v_k(x)$ 的加權和：
   $$f(x) = \sum_{k=1}^n f_k v_k(x)$$
   （就像 Fourier 級數用 $\sin, \cos$ 組出複雜波形 [35:19]；基底函數可選線性或多項式）。
4. **解線性方程組** [36:36]：代回積分方程與邊界條件，解出所有節點的 $f_k$。

[37:17] 缺點：**非常慢**（複雜度遠高於 $O(N^2)$ [38:04]），所以架構模擬器很少直接跑 FEM。

---

## 7. Green's function：線性疊加、平移不變性與 2D 摺積

[38:10]（課本 §11.2.6，式 11.21–11.23，p.624–625）能不能比 FDM 的 $O(N^2)$ 更快？最快的方法是 **Green's function**，靠 Fourier 熱方程的兩個性質：

1. **Linearity & Superposition（線性與疊加性）** [38:25]：功率分布 $P = P_x + P_y$ 產生的穩態溫升就是個別溫升相加 $T_x + T_y$。
2. **Shift invariance（平移不變性）** [38:57]：只要**遠離晶片的邊緣（rim）與四個角落（corners）**，在晶片內部任何位置放一個小小 1 W 的熱源，它在周圍產生的徑向對稱（isotropic，$G(x,y) = G(-x,y)$）溫度擴散形狀都一樣——把熱源平移，溫度場就跟著平移！

[40:21] 設晶片切成網格（⚠️ [40:00] 老師提醒這裡投影片一度把網格寫成 $N \times N$，跟前面 $N$ 代表總格數不同；到 [49:41] 算複雜度時 $N$ 又指總格點數，以下統一區分清楚）：

- 離散疊加（課本式 11.21）：點 $(x_p, y_q)$ 的溫度是所有位置 $(x_i, y_j)$ 的功率乘上距離對應的擴散函數：
  $$T(x_p, y_q) = \sum_i \sum_j P(x_i, y_j)\, G(x_p - x_i, y_q - y_j)$$
- **連續極限與 Dirac delta** [43:51]：把小格子縮到無限小，但功率積分維持 1 W，熱源就趨近於**單位脈衝 Dirac delta 函數** $\delta(x)$（只在 0 有尖峰、其餘為 0、$\int \delta(x)\,dx = 1$）。
- **Green's function $G$ 的定義** [46:34]（課本 Definition 97）：就是**對 1 W Dirac delta 點熱源的溫度脈衝響應（impulse response）**（可事先用解析解或離線跑一次 FEM／FDM 算好）！（[45:02]、[46:30] 老師現場擦掉投影片上一行筆誤，強調是整個功率分布可視為許多小小 Dirac delta 的集合。）
- 於是整片晶片的溫度分布就簡化成一個 **2D 摺積（convolution）**（課本式 11.22–11.23）：

$$T(x', y') = \iint P(x, y)\, G(x' - x, y' - y)\,dx\,dy \quad\Longleftrightarrow\quad T = P \star G$$

⚠️ **時間複雜度與邊界修正**：
- 影片 [47:45]、[49:29] 直接說摺積把原本 FDM 矩陣乘法的 $O(N^2)$（$N$ 為總格點數）降成**線性 $O(N)$**（「10,000 格就快 10,000 倍」，當只取固定大小的局部擴散視窗時確實是 $O(N)$）。課本 p.625 則補充標準全局加速做法：把 $T = P \star G$ 轉到 **Fourier 頻域（FFT）**，摺積直接變成逐點相乘（複雜度 $O(N \log N)$，同樣遠快於 $O(N^2)$）！
- 【課本補充】**邊緣怎麼辦？鏡像法（method of images）**（課本 p.625）：晶片四邊是絕熱牆（熱散不出去），靠近邊緣的熱源會特別燙，破壞平移不變性。解法：若熱源離絕熱邊牆距離為 $x$，就在牆外對稱位置 $-x$ 放一顆**同功率的鏡像熱源（image source）**，兩者溫度場疊加起來在牆上的法向導數剛好抵銷為 0（絕熱）！

---

## 8. 漏電–溫度回授迴圈與 leakage-aware Green's function

[50:15] 把上一講的漏電拉回來：

```
Dynamic + Short-circuit power ──► (+) ──► Total power ──► Temperature
                                   ▲                           │
                                   └─── Leakage power ◄────────┘
```

- 因為溫度會推高漏電功率、漏電又反過來推高溫度，傳統做法必須「算溫度 → 更新漏電 → 再算溫度」反覆迭代（若發散就是 **thermal runaway**）。
- 【口頭】[51:03] 老師提到他自己參與的研究（課本 p.635 引用 *Sarangi et al., 2014*）：推導出 **leakage-aware Green's function**，利用工作溫度區間內漏電對溫度近似線性的特性，把動態＋漏電一次收進修正後的 Green's function，完全省掉迭代迴圈！

---

## 9. 【原始碼】Linux `coretemp` 怎麼讀晶片溫度與 1 秒快取

課本 p.619 說矽晶片的熱時間常數在 **ms 等級**；那為什麼在 Linux 下讀 `/sys/class/hwmon/hwmon*/temp*_input` 時，每毫秒狂讀卻看到數字每秒才跳一次？看 Linux v6.2.12 的 Intel 數位溫度感測器驅動 `drivers/hwmon/coretemp.c:408-420`：

```c
	tjmax = get_tjmax(tdata, dev);
	/* Check whether the time interval has elapsed */
	if (time_after(jiffies, tdata->last_updated + HZ)) {
		rdmsr_on_cpu(tdata->cpu, tdata->status_reg, &eax, &edx);
		/*
		 * Ignore the valid bit. In all observed cases the register
		 * value is either low or zero if the valid bit is 0.
		 * Return it instead of reporting an error which doesn't
		 * really help at all.
		 */
		tdata->temp = tjmax - ((eax >> 16) & 0x7f) * 1000;
		tdata->last_updated = jiffies;
	}
```

- **硬體怎麼報溫度**：跨核心讀 `IA32_THERM_STATUS` MSR（`rdmsr_on_cpu`），bits `22:16` 存的是「目前距離最高允許接面溫度 `TjMax`（本機為 100°C）還差幾度」，所以實際攝氏溫度（毫度）是 `tjmax - ((eax >> 16) & 0x7f) * 1000`。
- **軟體的 `HZ` 快取**：`time_after(jiffies, tdata->last_updated + HZ)` 刻意限制**至少隔 `HZ` 個 jiffies（剛好 1 秒）**才真的去讀一次硬體 MSR，避免使用者程式狂讀 sysfs 時不斷發送跨核心 IPI 中斷拖慢系統。因此晶片內部的 ms 級熱暫態只能由硬體自己的熱管理單元即時反應，OS 透過 `hwmon` 看到的是 1 秒取樣一次的軌跡。

---

## 10. 【實測】本機核心橫向傳熱與 2D FDM vs Green's function

腳本在 `experiments/lecture47/thermal_model.py`，做兩組實驗：

### 10.1 本機單一核心燒機時的橫向熱傳導（lateral heat conduction）

將單一 Python/NumPy 向量迴圈用 `taskset -c 0` 綁死在 `Core 0` 燒 3.3 秒再停下來冷卻，每隔 1.1 秒（跨過 [§9](#9-原始碼linux-coretemp-怎麼讀晶片溫度與-1-秒快取) 的 `HZ` 快取）讀取同一顆晶片上四顆 P-core 的溫度（`thermal_model.py:18-50`）：

| 時間 (s) | 階段 | Core 0（燒機核心） | Core 4（鄰近 P-core） | Core 12 | Core 20（遠端 P-core） |
|---|---|---|---|---|---|
| 0.00 | idle | **63°C** | 56°C | 60°C | 60°C |
| 1.10 | burn | **78°C**（+15°C） | **64°C**（+8°C） | 64°C（+4°C） | 63°C（+3°C） |
| 2.20 | burn | **79°C**（+16°C） | **65°C**（+9°C） | 65°C（+5°C） | 60°C（+0°C） |
| 3.30 | burn | **79°C** | **65°C** | 65°C | 64°C |
| 4.40 | cool | **64°C** | 59°C | 60°C | 58°C |
| 5.50 | cool | **63°C** | 59°C | 60°C | 59°C |

- 只燒 `Core 0` 一顆核心，1 秒內它自己跳升 **16°C**（63°C → 79°C），停機後 1 秒內又掉回 64°C（驗證熱時間常數遠小於 1 秒）。
- 完全沒跑工作的鄰居 `Core 4` 跟著被**橫向熱傳導**加熱了 **+9°C**（56°C → 65°C），而離得較遠的 `Core 20` 幾乎只浮動 0–4°C——直接在真實矽晶片上看到 [§2](#2-三種熱傳機制與垂直水平傳導) 的空間溫度梯度！

### 10.2 2D 晶片穩態模型：FDM（`T = AP`）vs 鏡像 Green's function（`T = P ⋆ G`）

在 `thermal_model.py:52-116` 建立一片 $2\text{ cm} \times 2\text{ cm}$、切成 $64 \times 64$（$N = 4096$ 格）、四邊絕熱（adiabatic）且每格有垂直散熱電導通往 45°C 環境的矽晶片模型，對照展開 $4096 \times 4096$ 稠密矩陣乘法 $\mathbf{T} = \mathbf{A}\mathbf{P}$ 與加上四邊鏡像反射（`2m × 2m` method of images）的 2D FFT Green's function 摺積：

```python
# experiments/lecture47/thermal_model.py:70-74
def solve_greens_fft(P_2d, G_fft):
    m = P_2d.shape[0]
    P_mir = np.block([[P_2d, P_2d[:, ::-1]],
                      [P_2d[::-1, :], P_2d[::-1, ::-1]]])
    return np.fft.irfft2(np.fft.rfft2(P_mir) * G_fft, s=(2 * m, 2 * m))[:m, :m]
```

| 測試項目（$64 \times 64, N = 4096$，單執行緒計時） | 結果 |
|---|---|
| `max |T_FDM − T_Green(mirror)|` | **$1.99 \times 10^{-13}\text{ °C}$**（達浮點數雙精度極限，完全等價！） |
| 單一 15 W 熱源：**放在正中央** vs **放在絕熱角落** | 中央 **75.75°C** vs 角落 **102.32°C**（兩道絕熱牆擋住橫向散熱，飆高 **+26.6°C**！） |
| 兩個 15 W 熱源：**對角遠離** vs **並排緊貼** | 遠離 **75.81°C** vs 並排 **86.13°C**（橫向疊加讓峰值多出 **+10.3°C**，預告 [第 48 支](lecture48_note.md) 的排程擺放問題） |
| 單次求解時間：稠密 FDM `A @ P` vs FFT `P ⋆ G` | FDM **4009.8 µs** vs Green's FFT **297.3 µs**（快 **13.5 倍**；$N$ 越大差距越懸殊） |

---

## 11. 一頁速查表

| 主題 | 重點 |
|---|---|
| 封裝結構 | PCB ↔ BGA ↔ 倒裝矽晶片（~400 mm²）↔ TIM ↔ 鎳銅均熱片（~369 W/m·K）↔ TIM ↔ 鰭片散熱器 ↔ 風扇 |
| 熱傳與邊界 | 晶片內靠**傳導**（垂直為主、橫向影響鄰居），散熱器對空氣靠**對流**；側面（與簡化模型下的底部）視為 **adiabatic**（無熱流），散熱器對環境視為 **isothermal** |
| 穩態 vs 暫態 | 穩態看長期固定功率的最終溫度；暫態看隨時間變化（晶片熱時間常數約 **ms** 級） |
| Fourier 定律 | 1D：$q_x = -k A \frac{dT}{dx}$；3D PDE：$k \nabla^2 T + Q' = \rho c_p \frac{\partial T}{\partial t}$（$\alpha = k/\rho c_p$）；穩態 $\nabla^2 T = -Q'/k$ |
| FDM | 二階導數換差分 $\frac{T_{i+1} - 2T_i + T_{i-1}}{\Delta x^2}$；穩態 $\mathbf{T} = \mathbf{A}\mathbf{P}$（$O(N^2)$），暫態 $\mathbf{G}\mathbf{T} + \mathbf{P} = \mathbf{C}\mathbf{T}'$ |
| 電路類比 | $T \leftrightarrow V$、$P \leftrightarrow I$、熱電導 $\mathbf{G} \leftrightarrow 1/R$、對地熱電容 $\mathbf{C} \leftrightarrow C$；可用 SPICE／HotSpot 快速求解 |
| FEM | 乘測試函數轉積分 → 三角形／四面體網格（elements + nodes）→ 基底函數加權和 $\sum f_k v_k(x)$；最準但極慢 |
| Green's function | 靠**線性疊加**＋**平移不變性**；$G$ 為 1 W Dirac $\delta$ 點熱源的溫度響應，$T = P \star G$（轉 FFT 變乘法）；邊緣絕熱牆用**鏡像法（method of images）**修正 |
| 原始碼與實測 | Linux `coretemp` 用 `TjMax - offset` 讀 MSR 並快取 1 秒（`HZ`）；本機單燒 Core 0 升 16°C 連帶把鄰居 Core 4 烤高 9°C；$N=4096$ 下鏡像 FFT Green's 與 FDM 誤差 $2\times 10^{-13}$°C 且快 13.5 倍 |

---

## 12. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| 動態功率、次臨界漏電與溫度正回授 | [第 46 支](lecture46_note.md) |
| 3D 堆疊記憶體（HBM／eDRAM）把熱困在內層的問題 | [第 45 支 §9](lecture45_note.md#9-3d-記憶體與-edram)、[第 48 支](lecture48_note.md) |
| 三角形網格共面性質（GPU 圖學管線） | [第 18 支](lecture18_note.md) |
| 2D 摺積（convolution）在機器學習加速器的重用 | [第 55 支](lecture55_note.md)–[第 57 支](lecture57_note.md) |
| 功率與溫度管理（DVFS、熱點移轉、job placement） | [第 48 支](lecture48_note.md) |
| 溫度對老化（aging）與硬錯誤率的指數影響 | [第 51 支](lecture51_note.md) |

---

## 附：本講一句話總結

> **晶片產生的熱主要經導熱膏、鎳銅均熱片與鰭片散熱器垂直傳導並由風扇對流帶走，少部分橫向傳導則會加熱鄰近單元；由 Fourier 熱傳導定律推得三維偏微分方程 $k\nabla^2 T + Q' = \rho c_p \frac{\partial T}{\partial t}$，切成網格用有限差分法（FDM）可化為穩態 $O(N^2)$ 的 $\mathbf{T}=\mathbf{A}\mathbf{P}$ 與暫態 $\mathbf{G}\mathbf{T}+\mathbf{P}=\mathbf{C}\mathbf{T}'$，並將溫度對映電壓、功率對映電流轉成等效 RC 熱電路交給 HotSpot／SPICE 求解，複雜幾何可用三角形網格與基底函數的有限元素法（FEM），而最快的方法是利用線性疊加與平移不變性將功率與 1 W 點熱源脈衝響應（Green's function）做 2D 摺積 $T = P \star G$（邊界輔以鏡像法）。本機實測：Linux `coretemp` 每隔 1 秒讀取 `TjMax` 差值，單核燒機讓 Core 0 升溫 16°C 並橫向烤熱鄰核 9°C；在 4096 格 2D 模型上，加了鏡像法的 FFT Green's function 與稠密 FDM 誤差僅 $2\times 10^{-13}$°C、速度快 13.5 倍，且清楚呈現絕熱角落與熱源並排造成的劇烈溫升。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 11: Power and Temperature (temperature modeling) --- Part II/III 的 YouTube 自動字幕（52:00）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch11 §11.2（p.617–625）、Linux v6.2.12 原始碼整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*