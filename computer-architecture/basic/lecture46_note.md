# Basic Computer Architecture 第四十六支影片完整筆記：光碟（CD、DVD、Blu-ray）與 flash／SSD（Chapter 12, Part 4/4 → 課本 Ch13）

> **課程**：Basic Computer Architecture — Chapt 12: Optical Drives and Solid State Drives, Part 4/4（播放清單第 46 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（41:53，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 **Ch13** §13.8.3–13.8.4（p.711–722）
> **章號**：影片的 Chapter 12 是課本 v3.09 的 **Chapter 13**；影片說的「chapter 6」（pre-charge、sense amplifier）是課本 Ch7。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 為什麼需要光碟](#1-為什麼需要光碟)
- [2. 光碟的四層結構：land 與 pit](#2-光碟的四層結構land-與-pit)
- [3. 光碟機怎麼讀](#3-光碟機怎麼讀)
- [4. 固定線速度（CLV）vs 固定角速度（CAV）](#4-固定線速度clvvs-固定角速度cav)
- [5. 三代光碟](#5-三代光碟)
- [6. Floating gate transistor](#6-floating-gate-transistor)
- [7. 怎麼讀一個 floating gate transistor](#7-怎麼讀一個-floating-gate-transistor)
- [8. NOR flash 與 NAND flash](#8-nor-flash-與-nand-flash)
- [9. Flash 是儲存裝置，不是記憶體](#9-flash-是儲存裝置不是記憶體)
- [10. Page、block、program／erase cycle](#10-pageblockprogramerase-cycle)
- [11. Wear leveling](#11-wear-leveling)
- [12. Read disturbance](#12-read-disturbance)
- [13. 課程結尾](#13-課程結尾)
- [14. 【原始碼】Linux 的 wear leveling 門檻](#14-原始碼linux-的-wear-leveling-門檻)
- [15. 【實測】](#15-實測)
- [16. 一頁速查表](#16-一頁速查表)
- [17. 與後續講次的連結](#17-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
光碟：polycarbonate → reflective（land／pit）→ lacquer → surface（標籤）
      land = physical 1、pit = physical 0；NRZI；Reed-Solomon 錯誤更正
      光碟機：spindle＋沿導軌直線移動的雷射頭；固定線速度（CLV）
      CD 700 MB／780 nm／153 KB/s；DVD 4.7 GB／650 nm／1.39 MB/s；Blu-ray 25 GB／405 nm／4.5 MB/s
flash：floating gate transistor（電子存在浮動閘 → 門檻電壓變高 VT⁺）
      讀：控制閘給 VT 和 VT⁺ 之間的電壓，看導不導通
      NOR（並聯，快但密度低）、NAND（串聯，密度高，實際都用這個）
      page（512 B–4 KB）讀寫、block（32–128 page）erase；P/E cycle 有限 → wear leveling；read disturbance
```

---

## 1. 為什麼需要光碟

[00:00]–[01:47]　課本 §13.8.3（p.711）

硬碟的問題：
- **不好攜帶**：大多固定在桌機或筆電裡（現在也有接 USB 的外接硬碟，但一般還是很笨重）。
- 有**機械零件**，「非常精密的工程設計」，摔一下就可能弄壞馬達或讀寫頭。
- **貴**。

所以需要一種**便宜**的儲存媒體，特別是電影、歌曲這類：**光學媒體**（optical media）：CD、DVD、Blu-ray。

---

## 2. 光碟的四層結構：land 與 pit

[01:47]–[05:32]　課本 Fig 13.39（p.712–713）

由下往上（雷射從**下面**打上來，光碟在轉）：

| 層 | 作用 |
|---|---|
| **Polycarbonate layer**（聚碳酸酯層） | 保護記錄面的一側；對光幾乎是透明的 |
| **Reflective layer**（反射層） | 上面有 **land**（平的地方）和 **pit**（凹洞）：所有的 bit 都記錄在這裡 |
| **Lacquer layer**（漆層） | 支撐 land 和 pit |
| **Surface layer**（表面層） | 一層塑膠，蓋住整片光碟；印電影或軟體名字的標籤貼在這裡 |

- **land**：反射率**高**，把光反射回去。
- **pit**：反射率**低**，而且讓光產生**相位變化**。
- 接收端從反射光的樣子判斷是 land 還是 pit。
- **land = physical 1、pit = physical 0**；和硬碟、USB 一樣用 **NRZI** 編碼（[第 43 支](lecture43_note.md#11-nrz-與-nrzi)），也有時脈與資料回復的機制。
- **錯誤更正**：用非常強的 **Reed-Solomon** 碼。光碟表面一刮傷，那裡的反射就會不規則，所以錯誤保護要做得很好。

⚠️ 字幕把 encoding 寫成「encryption」（加密），老師講的是**編碼**。

【課本補充】p.712–713：
- 可錄式光碟（**CD-R**、**DVD-R**，寫一次、讀很多次）：polycarbonate 層塗上對光敏感的**有機染料**，寫入時用高功率雷射改變染料的反射率。
- 可重複寫的光碟：反射層是**銀-銦-銻-碲合金**。加熱到 **500 °C** 那一點變成非晶態、失去反射率（pit）；加熱到 **200 °C** 回到多晶態、恢復反射（land）。
- 雙層光碟：第一層塗上部分透光的材料；也可以兩面都記錄（把兩片單面光碟背對背黏起來，要兩組雷射頭讀）。
- 第三代光碟的反射層非常靠近底面。

---

## 3. 光碟機怎麼讀

[05:32]–[09:33]　課本 Fig 13.40（p.713–714）

- 光碟中間的洞套在 **spindle** 上，**spindle motor** 帶動旋轉。
- **雷射頭**（laser assembly）裡有雷射和讀取頭，沿著**兩條導軌**移動。導軌像螺絲一樣：往一個方向轉，雷射頭往一邊走；往另一個方向轉，往另一邊走。
- **移動是沿著半徑的直線**。【口頭】[06:52] 對比：硬碟的讀寫頭是**轉一個角度**（沿圓弧，[第 45 支](lecture45_note.md#8-硬碟的結構plattertracksector)）。
- 光碟也有自己的 sector 和 track，只是組織方式不同（細節在課本）。
- 讀出的 bit 經過光碟機的電子電路組成封包，經 bus 送到 CPU 或記憶體。

**光碟為什麼便宜**（【口頭】[08:12]）：硬碟的讀寫機構是硬碟的一部分；光碟的**讀取機構在電腦（主機）那邊**，光碟本身只是一片有 land 和 pit 的塑膠。所以 CD、DVD 很便宜，可以**燒錄**（burn）一次，運送到全世界，只要有光碟機就能讀。

【課本補充】p.713–714：硬碟用 **rotary actuator**（轉動的手臂），光碟機用 **linear actuator**（線性致動器）；actuator 的馬達非常精確地轉動，齒輪把轉動變成雷射頭的直線運動。雷射經過透鏡聚焦在反射層上，反射光被讀取頭裡的 **photodetector**（光偵測器）轉成電訊號。

---

## 4. 固定線速度（CLV）vs 固定角速度（CAV）

[09:33]–[13:56]　課本 p.714–715

```
v = ω × r      （線速度 = 角速度 × 半徑）
```

| | 硬碟 | 光碟 |
|---|---|---|
| 固定的是 | **角速度 ω**（5400 或 7200 RPM） | **線速度 v** |
| 結果 | 外圈的 v 比較大 → 外圈的 bit 比較快經過讀寫頭 → 內外圈要用不同速度感測 → 要 **zoning**（[第 45 支](lecture45_note.md#8-硬碟的結構plattertracksector)） | 雷射頭往外走，轉速要**變慢**；往內走，轉速要**變快** |
| 複雜在哪 | 讀寫頭 | **spindle motor**：要能在很大的速度範圍內變速 |

**為什麼兩者不同**（【口頭】[09:51]）：
- 硬碟技術很老，至少有 30 年了。那時候做出一顆**穩定**跑 5400 RPM、又夠便宜的固定轉速馬達，就已經是很大的挑戰（IBM、Seagate 等先驅做到了）。
- CD、DVD 出現得晚很多，那時馬達技術已經很進步，可以做出精密的變速馬達。

**固定線速度的好處**：不管在哪個半徑，bit 經過讀寫頭的**速率都一樣** → 沒有時序問題，**讀取頭可以很簡單**，也不用 zoning。光碟的讀取頭本身就已經有雷射、分析反射光的電路、NRZI 解碼，頭簡單一點很重要。

【口頭】[12:06]「zoning 我在課堂上沒講，怕太複雜，課本裡有。」

【課本補充】p.715：為了簡化邏輯，**高效能的光碟機**也會做和硬碟類似的 zoning。

---

## 5. 三代光碟

[13:56]–[17:20]　課本 Table 13.9（p.715）

| | CD | DVD | Blu-ray |
|---|---|---|---|
| 世代 | 第 1 代 | 第 2 代 | 第 3 代 |
| 容量 | **700 MB** | **4.7 GB** | **25 GB** |
| 用途 | 音樂 | 影片 | 高畫質影片 |
| 雷射波長 | **780 nm** | **650 nm** | **405 nm** |
| 原始 1X 傳輸率 | **153 KB/s** | **1.39 MB/s** | **4.5 MB/s** |

- 「物理原理這幾代都沒變」，只是技術讓 land 和 pit **更小**、排得更密，一片光碟就能放更多資料。
- 要偵測更小的結構，就要用**更短的波長**。
- **1X** 是剛推出時的原始速度；常看到的 8X、10X、52X 是之後改進的倍數。
- CD：放得下很多歌，或電影的一部分（不是高畫質）；153 KB/s 適合音樂、不夠放影片。DVD：一整部電影（不一定高畫質）。Blu-ray：3D、高畫質、4K 影片。
- 【口頭】[17:07] Blu-ray「不知為什麼一直不太流行，但這和技術無關，比較是授權和標準化的問題。」

⚠️ 字幕把 CD 的傳輸率聽成「15 23 KB/s」，DVD 的波長只說「稍微小一點」；上表用課本 Table 13.9 的數字。

---

## 6. Floating gate transistor

[17:20]–[22:11]　課本 §13.8.4、Fig 13.41（p.715–717）

**SSD**（solid state drive，固態硬碟）、flash drive：所有的隨身碟、USB 碟都是。「幾乎人人都有一支隨身碟……我們應該知道它怎麼運作。」核心是一種新的元件：**floating gate transistor**（浮動閘電晶體）。

```
        控制閘（control gate）
       ─────────────────
         SiO₂（絕緣）
       ─────────────────
        浮動閘（floating gate）
         SiO₂（絕緣）
  source ═══════════════ drain
              基板
```

- 很像一般的 NMOS，但有**兩個閘**：上面是 **control gate**（控制閘，等於一般 MOS 的閘），下面多一個 **floating gate**（浮動閘），四周都被 SiO₂ 絕緣包住。
- 符號：一般電晶體多畫一條橫線，代表浮動閘。

**Program（寫入 0）**：在控制閘和基板之間加一個**非常高的正電壓** → 電子從通道穿過去、**卡在浮動閘裡** → 電晶體的**門檻電壓變高**。這叫電晶體被 **programmed**。

| 狀態 | 浮動閘 | 門檻電壓 | 值 |
|---|---|---|---|
| 沒 program | 沒有電子 | VT | **1** |
| programmed | 有電子 | **VT⁺**（> VT） | **0** |

一開始是 1；program 之後變 0。

【口頭】[20:42]「我要更正剛剛說錯的話」：老師先說閘的材料不一定是多晶矽，後來改口：至少到 2017 年的技術，**浮動閘和控制閘都是多晶矽**（polysilicon），基板、source、drain 是摻雜的矽，「以後可能會變」。

【課本補充】p.715–717：
- flash 是一種 **EEPROM**（電子可抹除可程式唯讀記憶體）；傳統的 EPROM 用**紫外線**抹除，已經被 flash 取代。
- flash 用在隨身碟、大部分可攜裝置、醫療設備、工業電子、高階伺服器的磁碟 cache。
- 浮動閘被絕緣包住，電荷可以保持**非常久**，正常情況下幾乎沒有電流流進流出。

---

## 7. 怎麼讀一個 floating gate transistor

[22:11]–[23:31]　課本 p.717

在控制閘加一個介於 **VT 和 VT⁺ 之間**的電壓：

| 存的值 | 門檻 | 結果 |
|---|---|---|
| 1（沒 program） | VT | 電壓 > VT → **導通** |
| 0（programmed） | VT⁺ | 電壓 < VT⁺ → **不導通** |

看導不導通就知道是 0 還是 1。

---

## 8. NOR flash 與 NAND flash

[23:31]–[28:16]　課本 Fig 13.42–13.43（p.717–719）

**NOR flash**：像 CMOS NOR 閘的接法：好幾個 floating gate transistor 各接自己的 **word line**，一端接地、另一端都接到同一條 **bit line**。
- 讀 WL2：WL2 給 VT 和 VT⁺ 之間的電壓，其他關掉。bit line 的電壓**趨近 0** → WL2 那顆導通 → 沒 program → **1**；不趨近 0 → **0**。
- 可以用和 SRAM、DRAM 一樣的 **pre-charge ＋ sense amplifier**（[第 20 支](lecture20_note.md#5-解法pre-charge--sense-amplifier)，「該 Google 的關鍵字就是 pre-charging 和 sense amplifier」）。
- 比較**快**，但**密度低、頻寬低**，「很少用」。

**NAND flash**（實際上都用這個）：像 CMOS NAND 閘的接法：
```
bit line ─ [bit line select] ─ FG₁ ─ FG₂ ─ … ─ FG₈ ─ [ground select] ─ 地
```
- **8 個** floating gate transistor **串聯**，各有自己的 word line；兩端各有一個普通電晶體當開關（bit line select、ground select）。
- 讀第 6 顆：其他 7 顆的控制閘給**高於 VT⁺** 的電壓，不管有沒有 program 都會導通；打開兩端的開關；第 6 顆給 VT 和 VT⁺ 之間的電壓，看整串導不導通：導通 → 1，不導通 → 0。
- 好處：很多電晶體共用一條 bit line，**bit line 少很多** → **密度高**。

【課本補充】p.717–719：
- NOR flash 的優點是很像傳統的 DRAM cell，可以像 DRAM 陣列一樣存取每一個 cell。
- NAND 的一串通常有 **8 或 16** 個電晶體；NAND flash 的密度比 NOR 至少高 **40–60%**，所以現在大部分 flash 裝置都用 NAND。

---

## 9. Flash 是儲存裝置，不是記憶體

[28:16]–[30:34]　課本 p.719

【口頭】[28:30]「所有學生都要知道一件**非常非常重要**的事：flash drive、SSD **不是記憶體裝置**，是**儲存裝置**。」

| | 記憶體裝置（SRAM、DRAM） | 儲存裝置（flash、硬碟） |
|---|---|---|
| 使用 | 程式執行時**一直**在用，強烈影響程式效能 | 主要用來**保存**：作業放進隨身碟帶走、歌放進去帶走 |
| 重視 | 速度 | **可靠**（資料不能消失）、**容量**、密度 |

所以 flash 可以**很慢**、可以很省 bit line：例如讀一串 8 個 bit，要「打開其他 7 個讀 1 個，再打開其他 7 個讀下一個」，這很慢，但儲存裝置負擔得起。

---

## 10. Page、block、program／erase cycle

[30:34]–[34:11]　課本 p.719–720

| 單位 | 大小 | 做什麼 |
|---|---|---|
| **Page**（頁） | 512 B–4 KB | **讀、寫**（program）的最小單位：不提供 byte 或 8、16 byte 的存取，只做大塊的傳輸（歌、影片） |
| **Block**（區塊） | 32–128 個 page，**16–512 KB** | **erase**（抹除）的最小單位 |

- 每個 page 還有很多額外的 bit 做**錯誤更正**，用 **CRC**（對 burst error 很有效，[第 44 支](lecture44_note.md#3-更強的碼secsecdedcrc)）。
- **erase** = 加一個很強的反向電壓，把電子從浮動閘拉出來，回到 1。要很大的電路，對單一電晶體或單一 page 做很沒效率，所以一次抹一整個 block。

**Program／erase cycle**（P/E cycle）：
- 1 → 0（program）比較容易，以 page 為單位；但**一旦變成 0，就不能直接變回 1**，只能 erase 整個 block，而 erase 很貴。
- 所以：寫一個 page、再寫另一個 page……不要的資料就留著；最後才把整個 block（32–128 個 page）一次 erase，重新開始寫。
- 每個 page 的一生：program → 讀很多次 → erase → 再 program → ……

⚠️ [31:43] 老師說「so we can erase data at the level of blocks okay sorry it should be off」，投影片上有個字打錯當場改。

【課本補充】p.720：例如一個 block 裡已經寫了 0110，就不能直接寫成 1001（要把 0 變回 1）；必須先 erase 整個 block。（【實測】見 [§15.1](#151-nand-的規則01-只能靠-erase)。）

---

## 11. Wear leveling

[34:11]–[39:11]　課本 Definition 163（p.720–721）

**問題**：flash 只能承受**有限次**的 P/E cycle（影片：通常 **100,000** 次，現在更多；課本：2013 年是 10 萬到 100 萬次）。每次 P/E 都會傷到包住浮動閘的 **SiO₂**，最後它**不再絕緣**，電晶體留不住電荷 → 壞掉。

**解法**：每個 block 一個**計數器**，每次 P/E 加 1；讓所有 block 的 P/E 次數差不多。否則某些 block 流量太大、很快就壞，別的 block 卻幾乎沒用到。

**Wear leveling**（磨損平均）：
1. 和硬碟一樣有一張**對應表**：軟體看到的是一個大 byte 陣列，給 **logical block** 編號；flash 內部對應到 **physical block**。
2. 某個 physical block 的 P/E 次數比別人高出很多時，找一個 P/E 次數低的 block，**把兩個的內容對調**：
   - 一定要留一些**空的 block**。
   - 把 A 的內容搬到空 block（暫存）→ erase A → 把 B 的內容搬到 A → erase B → 把暫存的內容搬到 B。
3. 改對應表。使用者完全不知道。

這樣整支 flash 會**均勻地**老化，幾年後才開始出錯；「反正隨身碟的壽命本來就沒那麼長」，但在它的壽命內盡量延長。

【口頭】[38:38]「計算機架構裡一直有這種機制：page table、TLB……各種對應表：程式給 logical address，表給 physical address，physical address 可以依裝置的限制改變。」

【課本補充】p.720–721：
- 為什麼需要：資料存取有時間和空間局部性，少數 block 會被存取得最多，「這正是我們想避免的」。
- 例子：隨身碟裡存著歌，大部分人不會輪流聽每一首，而是一直聽**最愛的幾首**，存那幾首歌的 block 會最先壞。
- 觸發條件：某 block 的計數比其他 block 多出一個**預先定的門檻**。
- （【實測】見 [§15.2](#152-wear-leveling延長-6-倍壽命但暫存-block-會變熱點)；Linux 的實作見 §14。）

---

## 12. Read disturbance

[39:11]–[41:10]　課本 p.721–722

**Read disturbance**（讀取干擾）：一直讀同一個電晶體，**其他**電晶體的內容會改變。
- NAND 一串電晶體：讀其中一個時，其他 7 個的控制閘要加高於 VT⁺ 的電壓。一直這樣讀，它們的浮動閘可能**失去**一些電荷（被 deprogram），或**多了**電荷（被 program）。
- 解法：每個 page 或 block 也記錄**讀了幾次**。超過門檻：
  - 把 block 對調到別的地方，或
  - 像 **DRAM refresh** 一樣，把 block 的內容讀出來再寫回去。
- 「我們不能什麼都不做，不然資料就會出錯。」

【課本補充】p.721：讀取時其他電晶體的控制閘電壓雖然高於 VT⁺，但沒有 program 需要的那麼高、時間也短；只是重複太多次，鄰近的電晶體就會慢慢被 program。

---

## 13. 課程結尾

[41:10]–[41:53]

【口頭】「終於，我們到了 12 章的最後。至少在這本書的這一版（第一版），12 章和每一章的課都完成了，大約 45–50 個小時。希望你對計算機架構的基礎有了非常好的理解。這絕對不是終點：我正在寫另一本**進階計算機架構**的書，大概一年後上市，會從這裡開始。」

（那本書就是 `books/computer-arch/nextgen-comp-arch.pdf`，對應 [Advanced Computer Architecture 課程](../advanced/README.md)。v3.09 版的課本已經擴充到 13 章，多了 RISC-V。）

---

## 14. 【原始碼】Linux 的 wear leveling 門檻

SSD 和隨身碟的 wear leveling 在裝置自己的控制器裡做，作業系統看不到。但 Linux 也能直接管理**裸的 NAND flash**（嵌入式系統常見），用的是 **UBI**（Unsorted Block Images）層，它自己做 wear leveling。`linux-src/drivers/mtd/ubi/wl.c:99-104`：

```c
 * Maximum difference between two erase counters. If this threshold is
 * exceeded, the WL sub-system starts moving data from used physical
 * eraseblocks with low erase counter to free physical eraseblocks with high
 * erase counter.
 */
#define UBI_WL_THRESHOLD CONFIG_MTD_UBI_WL_THRESHOLD
```

`linux-src/drivers/mtd/ubi/Kconfig:14-26` 的預設值和說明：

```
config MTD_UBI_WL_THRESHOLD
	int "UBI wear-leveling threshold"
	default 4096
	range 2 65536
	help
	  This parameter defines the maximum difference between the highest
	  erase counter value and the lowest erase counter value of eraseblocks
	  of UBI devices. When this threshold is exceeded, UBI starts performing
	  wear leveling by means of moving data from eraseblock with low erase
	  counter to eraseblocks with high erase counter.

	  The default value should be OK for SLC NAND flashes, NOR flashes and
	  other flashes which have eraseblock life-cycle 100000 or more.
```

- 和課本的做法一樣：每個 erase block 一個 **erase counter**（就是 P/E 計數），**最大和最小差超過門檻**（預設 4096）就搬資料。
- 搬的方向有巧思：把 erase 次數**低**的 block 上的資料（通常是很少改的「冷」資料）搬到 erase 次數**高**的空 block 上，讓冷資料去佔用磨損多的 block，磨損少的 block 空出來給常改的資料用。
- 說明裡的「erase block 壽命 100000 次以上」，正是影片說的 10 萬次。

---

## 15. 【實測】

程式在 `experiments/lecture46/`。

### 15.1 NAND 的規則：0→1 只能靠 erase

`flash.py` 的第一部分照課本 p.720 的例子：

```
$ python3 -I flash.py
erase 後寫入 0110： [0, 1, 1, 0]
再把一個 1 改成 0（0010）：可以 → [0, 0, 1, 0]
想寫成 1001： 要把 0 變回 1：必須先 erase 整個 block
erase 之後才能寫 1001：[1, 0, 0, 1]，這個 block 的 P/E = 1
```

已經寫了 0110 之後，只能繼續把 1 變 0；想變成 1001 就要先 erase 整個 block，P/E 計數加 1。

### 15.2 Wear leveling：延長 6 倍壽命，但暫存 block 會變熱點

100 個 block（最後一個當對調用的暫存 block），30 萬次寫入，「最愛的歌」模式：**90%** 的寫入落在 **10%** 的 logical block。照課本的做法：某 block 的 P/E 比最少的那個多出門檻，就用暫存 block 對調。

```
沒有 wear leveling      ：資料 block 的 P/E 最大  27219、最小   294；暫存 block      0（對調 0 次） → 最先壞掉的 block 撐到 10 萬次時，總共寫了 1.1 百萬次
門檻 100 的 wear leveling：資料 block 的 P/E 最大   3163、最小  3062；暫存 block   4418（對調 4418 次） → 最先壞掉的 block 撐到 10 萬次時，總共寫了 6.8 百萬次
門檻  20 的 wear leveling：資料 block 的 P/E 最大   3385、最小  3361；暫存 block  16876（對調 16876 次） → 最先壞掉的 block 撐到 10 萬次時，總共寫了 1.8 百萬次
```

- **沒有 wear leveling**：熱門的 block 被寫了 27,219 次，冷的只有 294 次，差了 90 倍。以 10 萬次壽命算，熱門 block 寫了約 110 萬次就壞了。
- **門檻 100**：所有資料 block 都在 3062–3163 之間，**非常平均**；最先壞的變成暫存 block（4418 次），整體壽命延長到約 **680 萬次**寫入，是原來的 **6 倍**。
- **門檻 20**：資料 block 更平均（3361–3385），但對調太頻繁（16,876 次），每次對調都要 erase 暫存 block，暫存 block 反而成了最熱的那個，壽命只剩 180 萬次。
- ⚠️ 課本只寫「用一個暫存 block 對調」，沒提到暫存 block 本身也會磨損；實際的 flash 控制器（像 Linux UBI 把資料搬到「空的 block」，§14）會輪流使用不同的空 block。門檻不是越小越好，這裡的數字是我的模擬，不是真實裝置。

### 15.3 本機的 SSD

`/sys/block/nvme0n1/queue/discard_granularity` 是 **512**、`discard_max_bytes` 約 2 TB：作業系統可以告訴 SSD「這些 block 我不要了」（**discard**，也叫 TRIM），讓 SSD 控制器提早 erase、做 wear leveling。兩顆 NVMe SSD 的 block 大小都回報 512 bytes（[第 45 支 §16.3](lecture45_note.md#163-本機的儲存裝置)）。⚠️ SSD 內部真正的 page 和 erase block 大小、P/E 次數，一般使用者讀不到（要 root 和 `nvme smart-log`），「待補」。

---

## 16. 一頁速查表

| 主題 | 重點 |
|---|---|
| 為什麼要光碟 | 硬碟不好攜帶、怕摔、貴；光碟便宜，適合電影和歌 |
| 四層 | polycarbonate、reflective（land／pit）、lacquer、surface |
| land／pit | 反射高 = physical 1／反射低、有相位變化 = physical 0；NRZI；Reed-Solomon |
| 光碟機 | spindle＋沿導軌直線移動的雷射頭；讀取機構在主機那邊 → 光碟很便宜 |
| CAV／CLV | 硬碟固定角速度（要 zoning）／光碟固定線速度（馬達變速，讀取頭簡單） |
| 三代 | CD 700 MB、780 nm、153 KB/s；DVD 4.7 GB、650 nm、1.39 MB/s；Blu-ray 25 GB、405 nm、4.5 MB/s |
| Floating gate | 兩個閘；電子卡在浮動閘 → VT⁺；沒電子 = 1、有電子 = 0 |
| 讀 | 控制閘加 VT 和 VT⁺ 之間的電壓，導通 = 1 |
| NOR／NAND | 並聯、快、密度低（少用）／8 顆串聯、密度高 40–60%（都用這個） |
| 儲存≠記憶體 | flash 重視可靠和容量，可以慢 |
| Page／block | 512 B–4 KB 讀寫／32–128 page（16–512 KB）erase |
| P/E cycle | 1→0 用 program，0→1 只能整個 block erase |
| 壽命 | 10 萬（–100 萬）次 P/E；SiO₂ 被破壞 |
| Wear leveling | 每 block 一個計數器；超過門檻就用空 block 對調；logical→physical 對應表 |
| Read disturbance | 一直讀會改到鄰居；記讀取次數，超過就搬走或讀出再寫回 |
| Linux | UBI 的 UBI_WL_THRESHOLD（預設 4096） |
| 實測 | 「最愛的歌」寫入下，wear leveling 把壽命延長約 6 倍；門檻太小時暫存 block 變熱點 |

---

## 17. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| 計算機架構的過去、現在、未來 | [第 47 支](lecture47_note.md)：Computer Architecture: Past, Present, and Future |
| 進階計算機架構（老師說的下一本書） | [Advanced Computer Architecture 課程](../advanced/README.md)（`nextgen-comp-arch.pdf`） |
| 硬碟、zoning、LBA 對應表 | [第 45 支](lecture45_note.md) |
| NRZI、CRC | [第 43 支](lecture43_note.md)、[第 44 支](lecture44_note.md) |
| pre-charge、sense amplifier、ROM／EPROM | [第 20 支](lecture20_note.md) |
| page table、TLB（同樣的對應表想法） | [第 37 支](lecture37_note.md) |
| DRAM refresh（read disturbance 的解法之一） | [第 20 支](lecture20_note.md#8-dram-陣列與-refresh) |

---

## 附：本講一句話總結

> **光碟是便宜、好運送的儲存媒體：polycarbonate、反射、lacquer、表面四層，資料記錄在反射層的 land（反射高，physical 1）和 pit（反射低、有相位變化，physical 0）上，用 NRZI 編碼和 Reed-Solomon 更正刮傷造成的錯誤；光碟機把雷射頭沿導軌直線移動，而且用固定線速度（越往外轉越慢），讀取頭因此可以很簡單、不用 zoning，CD、DVD、Blu-ray 三代靠越來越短的雷射波長（780、650、405 nm）把容量從 700 MB 提高到 4.7 GB、25 GB。flash 的核心是 floating gate transistor：用高電壓把電子塞進被絕緣包住的浮動閘，門檻電壓從 VT 升到 VT⁺ 代表 0，讀的時候在控制閘加 VT 和 VT⁺ 之間的電壓看導不導通；NAND flash 把 8 個串起來省 bit line、密度比 NOR 高，以 page（512 B–4 KB）讀寫、以 block（32–128 page）erase，0 只能靠 erase 整個 block 變回 1，而 P/E cycle 只有大約 10 萬次，所以要用 logical→physical 對應表做 wear leveling（P/E 次數差超過門檻就用空 block 對調），也要記錄讀取次數對付 read disturbance；最重要的是記住 flash 是儲存裝置、不是記憶體。實測模擬「最愛的歌」寫入時，wear leveling 把最先壞掉的時間延長約 6 倍，但門檻太小反而讓對調用的暫存 block 先磨壞；Linux 的 UBI 也用同樣的「erase counter 差超過門檻就搬」做法，預設門檻 4096。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 12: Optical Drives and Solid State Drives, Part 4/4 錄影逐字稿（YouTube 自動字幕，41:53）、Sarangi *Basic Computer Architecture* v3.09 Ch13 §13.8.3–13.8.4（p.711–722）、Linux v6.2.12 `drivers/mtd/ubi/wl.c`、`drivers/mtd/ubi/Kconfig` 整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
