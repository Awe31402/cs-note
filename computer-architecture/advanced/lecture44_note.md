# Advanced Computer Architecture 第四十四支影片完整筆記：主記憶體（四）：非揮發記憶體、flash（NOR、NAND、P/E cycle、wear leveling）與 FeRAM（Chapter 10, Part IV）

> **課程**：Advanced Computer Architecture — Chapter 10: Main Memory (Flash and FeRAM) -- Part IV（播放清單第 44 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（57:56）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch10** §10.5–10.5.2（p.577–585）
> **章號**：影片 Chapter 10 = 課本 Ch10。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕把「DRAM」寫成「Dam」、「NAND」寫成「n／nand」、「P/E」寫成「PE／P Cycle」、「FeRAM」寫成「F Ram」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 為什麼要非揮發記憶體](#1-為什麼要非揮發記憶體)
- [2. 所有 NVM 的共同做法](#2-所有-nvm-的共同做法)
- [3. 五種技術一覽](#3-五種技術一覽)
- [4. Floating gate transistor](#4-floating-gate-transistor)
- [5. 寫入：program 與 erase](#5-寫入program-與-erase)
- [6. NOR flash 與 NAND flash](#6-nor-flash-與-nand-flash)
- [7. Multi-level cell](#7-multi-level-cell)
- [8. Page、block 與 P/E cycle](#8-pageblock-與-pe-cycle)
- [9. Flash translation layer 與 wear leveling](#9-flash-translation-layer-與-wear-leveling)
- [10. Read disturbance](#10-read-disturbance)
- [11. FeRAM 的物理：ferroelectric 與 hysteresis](#11-feram-的物理ferroelectric-與-hysteresis)
- [12. FeRAM cell 的讀寫](#12-feram-cell-的讀寫)
- [13. FeRAM vs DRAM、flash](#13-feram-vs-dramflash)
- [14. 【原始碼】Linux 的 UBI wear leveling](#14-原始碼linux-的-ubi-wear-leveling)
- [15. 【實測】](#15-實測)
- [16. 一頁速查表](#16-一頁速查表)
- [17. 與後續講次的連結](#17-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
DRAM 的問題：斷電就沒了、要 refresh → NVM：比 DRAM 稍慢、比硬碟快很多；本質上是「儲存裝置」
做法：兩個穩定的物理狀態（高電阻／低電阻）→ 排成陣列 → 軟硬體一起處理可靠度
Flash：floating gate 存電子 → 門檻電壓變高；program（寫 0，強正壓）、erase（寫 1，強負壓）
  NOR（像 DRAM，好存取）vs NAND（串聯 8–16 個，密度高 40–60%）；MLC：一個 cell 存好幾個 bit
  以 page（512 B–4 KB）讀寫、以 block（32–128 page）erase；P/E cycle 有限（5–15 萬次）
  FTL：邏輯 → 實體的對應；wear leveling：把很常寫的 block 跟很少寫的對調；read disturbance：讀太多次就搬
FeRAM：ferroelectric 電容（PZT、SBT），hysteresis 讓電壓 0 時還有兩種極化；讀是破壞性的；很耐用但貴
```

---

## 1. 為什麼要非揮發記憶體

[00:00]「這一系列的第四講：**新興的記憶體技術**。有些已經出現了（例如 flash），其他的還在興起中。」大多是**非揮發**的。

DRAM 的主要問題：

1. **斷電資料就不見**：下次開機所有資料都要從硬碟讀 → 很慢。
2. [00:59] 要**頻繁 refresh**。

[00:59] 存一次就一直在：它可以當**儲存裝置**，事實上「主要就是儲存裝置」。可以拿來取代主記憶體，但不是最好的用法：最好還是有一些主記憶體，用 NVM 增加容量，同時當記憶體和儲存。

[01:37] 定位：

```
DRAM  <  NVM（稍微慢一點）  <<  硬碟（NVM 快很多很多）
```

【課本補充】課本 p.577 Definition 91：斷電就丟資料是 **volatile**（DRAM）；不會的是 **nonvolatile**（例如 USB 隨身碟裡的 flash），簡稱 **NVM**。NVM 跟硬碟這類慢的次要儲存競爭，也可以當硬碟的**快取**，或用在不想花時間從硬碟讀冷資料的關鍵應用。

---

## 2. 所有 NVM 的共同做法

[01:37] 不管用什麼技術，高層次的做法都一樣：

1. [02:17] 做一個有**兩種物理狀態**的元件，一種當 1、另一種當 0。通常電氣上表現成**高電阻**和**低電阻**（哪個是 0 哪個是 1 是慣例，不是硬性規定）。
2. [02:58] 把元件排成**陣列**，跟 DRAM 陣列一樣。
3. 可靠度不如 DRAM → 用**軟硬體結合**的方式處理可靠度問題。

---

## 3. 五種技術一覽

[03:46]

| 技術 | 兩種狀態從哪來 |
|---|---|
| **Flash** | transistor 有兩個 gate，其中一個是 **floating gate**：裡面有沒有困住**電子** → 門檻電壓不同 → 高／低電阻（[04:22]「最重要的是門檻電壓改變」） |
| **FeRAM**（ferroelectric RAM） | [04:22] **ferroelectric** 材料的電**極化**程度取決於**過去**加過的電壓 |
| **MRAM**（magnetoresistive RAM） | [04:56] **ferromagnetic** 材料的**磁化方向**取決於過去流過的電流；一個方向高電阻、另一個方向低電阻 |
| **PCM**（phase change memory） | [05:35] 材料是**非晶態**還是**結晶態**：用熱源加熱，突然冷卻 → 原子來不及排好 → 非晶態；慢慢冷 → 結晶態 |
| **ReRAM**（resistive RAM） | [05:35] anode 和 cathode 之間有沒有形成**導電的細絲**（一條離子的路徑）：有 → 低電阻，沒有 → 高電阻 |

這一講講 flash 和 FeRAM；MRAM、PCM、ReRAM 在 [第 45 支](lecture45_note.md)。

---

## 4. Floating gate transistor

[06:19]「最常見的，我們的 USB 隨身碟都用它。」（課本圖 10.27）

```
  control gate（原本的 gate）
  ─────────
  SiO₂
  floating gate（中間多出來的 gate，四周都是 SiO₂ 絕緣）
  SiO₂
  source ──── 通道 ──── drain
```

[07:40] floating gate 裡**困住電子**：

| 狀態 | 門檻電壓 | 邏輯值 |
|---|---|---|
| 沒有電子 | V_T（正常） | **1** |
| 有電子 | **V_T⁺**（比較高） | **0** |

[08:57] 為什麼門檻電壓會變高：NMOS 要導通需要 V_GS > V_T。floating gate 裡的電子帶負電，[09:37] 等於把 gate 對 source 的電壓**拉低**了 Δ：實際感受到的是 V_GS − Δ。要導通就要 V_GS − Δ > V_T，也就是 V_GS > V_T + Δ = **V_T⁺**。

[10:25]【口頭】「我稍微濫用了一下符號：V_th 和 V_T 是同一個東西。」

[11:09] **讀**：加一個介於 V_T 和 V_T⁺ 之間的 V_GS：

| 存的值 | 結果 |
|---|---|
| 1（沒有電子） | V_GS > V_T → **導通** → 低電阻 |
| 0（有電子） | V_GS < V_T⁺ → **不導通** → 高電阻 |

[11:54]「我只用了最基本的 transistor 物理。」

【課本補充】課本 p.577–578：flash 是一種 **EEPROM**（Electrically Erasable Programmable ROM）。傳統的 **EPROM** 用**紫外線**清除資料，已被 flash 取代。硬碟和光碟又大又怕摔（有精密的機械零件），光碟還怕刮傷 → 不適合可攜裝置。floating gate 四周都是 SiO₂，電氣上完全隔離，正常情況下幾乎沒有電流進出，所以電荷可以保存很久。

---

## 5. 寫入：program 與 erase

[12:46]

| 操作 | 寫入 | 做法 |
|---|---|---|
| **Program**（程式化） | 寫 **0**（困住電子） | 在 control gate 加**很強的正電壓**：電子往上跑，有些被困在 floating gate 裡。[13:30] 拔掉電源電子還在 → 插到別台電腦也讀得到 |
| **Erase**（抹除） | 寫 **1**（放出電子） | [14:16] 加**很強的負電壓**，把電子推回 substrate → 回到低電阻 |

[14:56]「要嘛往上拉、要嘛往回推。」

【課本補充】課本 p.578：program 時 control gate 加強正壓，**drain** 加一個比較小的正壓：drain 和 source 之間形成通道，control gate 的電場把通道裡的電子拉進 floating gate。erase 時在 control gate 和 **source** 之間加強負壓，把電子拉回 substrate 和 source。

---

## 6. NOR flash 與 NAND flash

[14:56]（課本圖 10.28、10.29）

| | **NOR flash** | **NAND flash** |
|---|---|---|
| 接法 | [15:39] 兩個 cell（各接自己的 word line）並聯在 bit line 和地之間 | [16:24] 8 個 floating gate transistor **串聯**，兩端各一個 select transistor（bit line select、ground select） |
| 名字的由來 | 像 NOR 閘：任何一個 cell 是低電阻（存 1），bit line 就被放電 | 像 CMOS NAND 閘的串聯 |
| 讀 | [19:14] 打開要讀的 cell，其他的 word line 設 0；存 1 → bit line 被放電 → 讀到的 bit line 電壓要**反過來** | [17:12] 其他 cell 的 V_GS 都設成 **≥ V_T⁺**（不管存什麼都導通，變成一條直線）；打開兩個 select；要讀的那個 cell 設成介於 V_T 和 V_T⁺ 之間，看 bit line |
| 特點 | 簡單，跟 DRAM 很像 | [18:31] **更緊密**，但讀寫比較慢（要打開一整串 transistor） |

[16:24] NAND 的兩個 select transistor 是為了降低漏電、提高 bit line 訊號的品質。

【課本補充】課本 p.579–580：NAND 串通常是 **8 或 16** 個 transistor。NAND 用的線比較少（transistor 直接互相連，只有一個接點接 bit line、一個接地）→ 密度比 NOR **高 40–60%** →「市場經濟選了 NAND」，現在大部分 flash 都是 NAND。

---

## 7. Multi-level cell

[19:57] 現在更常見的是 **multi-level** flash：一個 cell 存**好幾個 bit**。

- 例：存 3 bit → 要 **8 種**不同的電荷量（其中一種是沒有電荷，其他 7 種是不同的電子密度）→ 8 種門檻電壓 V_T1…V_T8。
- [20:45] 讀的時候 controller 要嘛加一個電壓、看電流大小判斷是哪一種；要嘛一個一個試，直到找到 cell 的門檻電壓 → 比較花時間。
- [21:29] 存 n bit 要 **2ⁿ** 種電荷量，還要有辦法在讀的時候分辨 → 密度大增。

[22:14] flash 的代價：

| | |
|---|---|
| 優點 | 密度高；沒有機械零件，比硬碟快很多 |
| 缺點 | **耐用度低**（[§8](#8-pageblock-與-pe-cycle)）；錯誤率高（一點點電荷漏掉就錯）→ 要大量的 **ECC** |

[22:51]「我們的手機沒有硬碟，用的是 flash」：就是因為速度。

---

## 8. Page、block 與 P/E cycle

[22:51] flash 是**儲存裝置**，以 **page** 為單位存取：**512 B 到 4 KB**。

[23:30] 寫入很難：要加很強的負電壓才能把電子推出去 → 寫入之前要先**抹除**。

- [24:13] 抹除的單位是 **block**：**32 到 128 個 page**。
- 「unwritten」的狀態全部是 1，寫入就是把某些 1 變成 0。
- [24:55] 為什麼讀的粒度小、寫的粒度大：不想替每個 cell 做精細的 write driver：太佔面積、太複雜，密度就做不高。[25:40] program、erase、甚至讀都需要電路：很粗的線、很大的電壓 driver。

[26:21] 規則：

1. 讀、寫都以 page 為單位。
2. 但一個 page 寫過之後（某些 1 變成 0），**不能直接再寫**：0 要變回 1 只能 erase。
3. erase 的粒度是 **block**，不是 page。

[27:41] 很多寫打到同一組位址怎麼辦？flash 裡可以有一個小的 **DRAM cache** 吸收一連串的寫，[28:24] 把它們合成**一次**真正的寫 → 像 flash 前面的 write buffer。

[29:08] **P/E cycle**（program/erase cycle）：

- 一個剛被 erase 的 page，一次寫完整個 page（0.5–4 KB）：寫 1 就不動，寫 0 就 program。
- [29:48] erase：拿一整個 **block**，全部設成 1。
- program → erase → program → erase……

[30:32] 要改一個 block 裡的一部分：

1. 把整個 block（除了要改的那個 page）複製到一個**剛 erase 過**的新 block。
2. 把新的 page 寫進去。
3. 改 **FTL** 的對應（[§9](#9-flash-translation-layer-與-wear-leveling)）。
4. （或者先搬到 DRAM cache 裡。）

[33:21] **耐用度**：電壓梯度大到可以讓電子**穿隧**過氧化層，推上推下，氧化層最後會**崩壞** → 只能承受**有限**次的 P/E cycle：**5 萬到 15 萬次**。block 會老化，到了上限就整個壞掉。

【課本補充】課本 p.580–581：

- page-level 存取其實不太浪費：作業系統本來就有記憶體裡的儲存快取（例如硬碟快取），讀寫都是大塊、連續的。例：Linux 的 `sync()` 系統呼叫、Windows 退出 USB 時，作業系統把所有還沒寫出去的東西整批寫進裝置。
- 【課本補充】作者的幽默提醒：「大部分人直接拔 USB，這可能讓資料損壞；你們的作者自己也犯過好幾次。所以**不要這樣做**！！！」
- 每個 page 還有 **CRC** 的額外 bit；block 的總大小 **16–512 KB**。
- 例：寫過 0110 的位置不能直接寫 1001：0 不能變 1。erase 又慢又耗電，所以以大的 block 為單位。

⚠️ P/E cycle 的數字：影片和課本 p.581 都說「5 萬到 15 萬次（2020 年）」；Linux 的 UBI 說明（[§14](#14-原始碼linux-的-ubi-wear-leveling)）寫的是 **SLC** NAND、NOR 有「**10 萬次以上**」，**MLC** NAND「通常**少於 1 萬次**」。課本的數字比較接近 SLC。

---

## 9. Flash translation layer 與 wear leveling

[30:32] 每個 flash 裝置都有 **FTL**（flash translation layer）：把外面看到的**邏輯位址**對應到 flash 裡的**實體位址**（[33:21] 像虛擬記憶體一樣，也可以叫「虛擬 flash 位址」）。

[33:21] **Wear leveling**：讓每個 block 磨損的速度**一樣**，不要有某個實體 block 老得特別快。

- [34:00] 每個 block 有一個 **P/E counter**。
- [34:45] 某個 block 的 P/E 次數太多 → 把它的邏輯 block 改對應到別的、P/E 次數少的實體 block。
- [35:29] 這些全在**裝置裡面**做：USB 隨身碟裡就有對應表和追蹤存取的邏輯。外面看起來是一個「很守規矩」的儲存裝置、一串 byte；「實際上不是那樣」。

[36:23] 怎麼對調（用一個**暫存的空 block**）：

```
1. P/E 次數很高的 block X 的內容 → 複製到一個剛 erase 過的暫存 block
2. erase X
3. 找一個 P/E 次數很低的 block Y，把 Y 的內容搬到 X（改對應）
4. erase Y，把暫存 block 的內容複製到 Y（改對應）
```

[37:08] 原本很常被存取的邏輯 block 現在在 Y（P/E 次數低）；原本冷的資料搬到 X（之後不太會被寫）→ 磨損平均。

【課本補充】課本 p.581–582 Definition 92：wear leveling。課本的例子：隨身碟裡存了很多歌，大部分人**不會**輪流聽所有的歌，而是一直聽最喜歡的那幾首 → 存那幾首的 block 會先壞。課本的計數器是**每個 P/E cycle 加一**；某個 block 比其他的多出一個門檻值，就跟比較少被存取的 block 對調；最後可以把暫存 block 也 erase 掉。

---

## 10. Read disturbance

[38:27] 跟 wear leveling 概念上很像。

- 一直讀同一個 cell：NAND 串裡**其他** cell 的 gate 要加 ≥ V_T⁺ 才能導通。[39:08] 這個電壓沒有 program 那麼高，但足以把一些電子拉上去 → 慢慢累積在 floating gate → **最後被 program 了**。
- [39:51] 在 **MLC**（multi-level cell）flash 裡問題更大。

[40:32] 解法跟 wear leveling 一樣：每個 block 一個**讀的計數器**，超過門檻就把它複製到新的位置、從乾淨的狀態重來（意外 program 的那些電荷就被丟掉了），改對應表。

【課本補充】課本 p.582：讀了**幾千次**之後，旁邊的 transistor 就開始累積負電荷。新的 block 要先 erase 再複製過去；讀的次數又變多時，在到門檻前再搬一次。

---

## 11. FeRAM 的物理：ferroelectric 與 hysteresis

[40:32]「接下來的技術投影片不多，只給基本概念。」

[41:15] 物質裡正負電荷稍微分開 → 形成**電偶極矩**（dipole moment）；偶極矩的密度（含方向）叫**極化**（polarization）。外加電場時，負電往正極跑、正電往負極跑 → 極化跟著電場排列。

[42:37] 有些材料有 **ferroelectric 效應**：它們好像有**記憶**：

- 一般材料：外加電場是 0 時，正負電荷均勻分布，極化總和是 0。
- ferroelectric 材料：電場拿掉後**極化不會變 0**，而且取決於**過去**加過的電壓。

[43:18] 電場為 0 時有**兩種**極化值 → 兩個邏輯狀態。

[44:00] 常見的材料：

| 材料 | |
|---|---|
| **PZT** | lead zirconate ＋ lead titanate（[45:26] 鉛合金，ferroelectric 效應很強） |
| barium titanate | BaTiO₃ |
| **SBT** | 很流行，ferroelectric 效應很強 |

⚠️ [45:26] 老師先說 SBT 是「strontium barium titanate」，再改口「strontium barium tantalum」。課本 p.582：SBT 是 **strontium bismuth tantalate**（SrBi₂Ta₂O₉）。

[44:00]【口頭】小更正：電場是電位的**空間導數的負值**，均勻的話 E ∝ V／d；電壓是 0 時電場就是 0。[45:26]「我們追著鉭和其他稀有金屬跑，就是因為它們用在這些新的記憶體元件。」

[46:15]（課本圖 10.30）**hysteresis 曲線**（x 軸電壓、y 軸極化）：

```
A（−Vdd，負極化）→ 電壓升到 0 → B（電壓 0、負極化：邏輯 1）
 → 繼續升 → C（極化變 0）→ D（+Vdd，正極化）
 → 電壓降回 0 → E（電壓 0、正極化：邏輯 0）← 不走原路！
 → F → A
```

- [47:41] B 和 E 是「**記憶點**」：電壓是 0，但材料記得極化。
- [48:21] 從 B 到 D 是單向的：要回到 B 只能一路走到 A 再回來。「這是 ferroelectric 材料最重要的一張圖。」

【課本補充】課本 p.582：FeRAM 是 flash 的有力競爭者，**1998 年就商品化**了。極化 = 單位體積的偶極矩；大部分材料的極化跟外加電場**成正比**，電場拿掉就變 0。

---

## 12. FeRAM cell 的讀寫

[48:21]（課本圖 10.31）跟 DRAM cell 很像：word line、bit line、sense amplifier；差別是：

- 電容換成 **ferroelectric 電容**（ferroelectric 材料夾在兩個金屬電極中間）。
- 電容的另一端接 **plate line**（PL），不是接地：[49:03] 電壓要能**正也能負**。
- 慣例：電壓 = PL 的電壓 − BL 的電壓。

**寫**：[49:42] 比讀簡單：

| 要寫 | 電壓 | 拿掉電壓後停在 |
|---|---|---|
| **1** | −Vdd → 到 A | **B** |
| **0** | +Vdd → 到 D | **E** |

[50:59] 關掉 word line 後 cell 就停在穩定點，「幾乎永遠」。

**讀**：[50:59]「有點難」：

1. PL = Vdd、BL = 0 → 電壓 +Vdd → 往 D 走。
2. [51:46] 存 1：從 **B 走到 D**（極化反轉，變化大）→ 電流大 → bit line 電壓升 ΔV₁；存 0：從 **E 走到 D**（變化小）→ ΔV₀。
3. ΔV₁ > ΔV₀ → sense amplifier 分辨得出來。

[53:59] 問題：存 1 的話，走到 D 之後電壓拿掉會回到 **E**（邏輯 0），不是 B → **破壞性讀**，跟 DRAM 一樣，讀完要寫回去。

[53:14]【口頭】箭頭不是嚴格單向：在 E 加 Vdd 就沿著 E–D 走，電壓拿掉再沿原路回 E；在 B 加 −Vdd 就沿原路回 A。

【課本補充】課本 p.583–585：電極通常是**鉑**或**銥**。存 0 時從 E 到 D 再回到 E，**不是**破壞性的；只有存 1 的時候才要寫回。sense amplifier 調成偵測介於 ΔV₀ 和 ΔV₁ 之間的電壓。

---

## 13. FeRAM vs DRAM、flash

[54:43]

| | FeRAM | DRAM |
|---|---|---|
| 揮發性 | **非揮發** | 揮發 |
| Refresh | **不用** | 要 |
| 材料 | 稀有、昂貴 | 矽，很好做、很便宜 |
| 容量 | [55:23]【口頭】超過 64–128 KB、頂多半 MB 就「會燒掉你的荷包」 | 大很多 |
| 讀 | 破壞性的 | 破壞性的 |
| 適合 | **小型嵌入式裝置**：很快、不用 refresh | 主記憶體 |

[56:09] 下一講：MRAM（尤其是 STT-MRAM）、其他元件，以及 roofline model。

【課本補充】課本 p.585 為什麼 FeRAM 沒有像 flash 那麼流行：

- flash 大部分用一般的矽製程；FeRAM 要把 PZT、SBT 整合進製程 → 要新的廠房，還有很多**污染**問題。
- 沒辦法用 PZT 做出類似 trench 或很小的 stacked 電容 → 無法再縮小；而且 dielectric 縮小時 ferroelectric 性質**明顯變弱**。
- 優點：不用 refresh（省很多電）；寫很容易，讀和寫的能耗差不多（flash 的寫很貴）；**耐用度高很多**：至少 **10¹²** 次讀寫，flash 只有 **10⁵** 次（2020 年）。

---

## 14. 【原始碼】Linux 的 UBI wear leveling

一般的 SSD、USB 隨身碟把 FTL 和 wear leveling 做在**裝置裡面**（[35:29]），Linux 看不到。但嵌入式系統常直接接**沒有 FTL 的裸 NAND**，這時 Linux 自己的 **UBI**（Unsorted Block Images）層負責 wear leveling。

說明：`drivers/mtd/ubi/wl.c:8-30`（節錄）：

```c
/*
 * UBI wear-leveling sub-system.
 *
 * This sub-system is responsible for wear-leveling. It works in terms of
 * physical eraseblocks and erase counters and knows nothing about logical
 * eraseblocks, volumes, etc. ...
 *
 * The wear-leveling is ensured by means of moving the contents of used
 * physical eraseblocks with low erase counter to free physical eraseblocks
 * with high erase counter.
 *
 * If the WL sub-system fails to erase a physical eraseblock, it marks it as
 * bad.
 *
 * This sub-system is also responsible for scrubbing. If a bit-flip is detected
 * in a physical eraseblock, it has to be moved. ...
 */
```

- 跟 [§9](#9-flash-translation-layer-與-wear-leveling) 一樣：每個實體 erase block 一個 **erase counter**（EC）。
- 做法：把**被使用中、EC 很低**的 block（存的是很少改的冷資料）搬到 **EC 很高的空 block** 上：冷資料放在老的 block 上，之後也不太會再寫它；原本的低 EC block 空出來給常寫的資料用。
- erase 失敗就標成**壞 block**；偵測到 bit flip（[§10](#10-read-disturbance) 那一類的錯誤）也要搬（scrubbing）。

門檻：`drivers/mtd/ubi/wl.c:728-742`（節錄）：

```c
		e1 = rb_entry(rb_first(&ubi->used), struct ubi_wl_entry, u.rb);
		e2 = get_peb_for_wl(ubi);
		if (!e2)
			goto out_cancel;

		if (!(e2->ec - e1->ec >= UBI_WL_THRESHOLD)) {
			dbg_wl("no WL needed: min used EC %d, max free EC %d",
			       e1->ec, e2->ec);

			/* Give the unused PEB back */
			wl_tree_add(e2, &ubi->free);
			ubi->free_count++;
			goto out_cancel;
		}
```

- `e1`：使用中的 block 裡 EC **最低**的；`e2`：空的 block 裡 EC 夠高的。兩者 EC 差 **≥ `UBI_WL_THRESHOLD`** 才搬：這就是課本「超過一個門檻值才對調」。
- `UBI_WL_THRESHOLD` 預設 **4096**（`drivers/mtd/ubi/Kconfig:14-17`）。Kconfig 的說明：這個預設值適合 SLC NAND 和 NOR（P/E 壽命 10 萬次以上）；**MLC** NAND 通常少於 1 萬次，門檻要調低到 128 或 256。

---

## 15. 【實測】

程式在 `experiments/lecture44/flash.c`（`gcc -O2 flash.c -o flash && ./flash <檔名>`）：開一個 512 MB 的檔案，用 **O_DIRECT** 繞過 OS 的 page cache（每次真的到裝置），量：A. 隨機 4 KB 讀 2000 次；B. 隨機 4 KB 寫＋`fdatasync` 300 次；C. 循序 1 MB 讀。

本機有兩種 flash 裝置：

- **NVMe SSD**：系統碟 `/dev/nvme1n1`（`/sys/class/nvme` 顯示是 Samsung MZVL4512HBLU；另一顆是 WD SN740）。
- **USB 隨身碟**：`/dev/sda`，型號 PNY PRO ELITE，這份筆記的檔案就放在它上面（核心把它標成 rotational=1，但它是 flash：USB 介面看不出來）。

| | NVMe（Samsung） | USB（PNY） | 對照：DRAM（[第 41 支 §11.2](lecture41_note.md#112-每隔約-18-µs-就有一次特別慢)） |
|---|---|---|---|
| 隨機 4 KB 讀：中位數 | **77.3 µs** | 178.3 µs | 72.4 **ns**（一個 64 B 的 line） |
| p10–p90 | 72.2–90.8 µs | 160.2–313.8 µs | |
| 隨機 4 KB 寫＋fdatasync：中位數 | **1922.8 µs** | 160.3 µs | |
| p99 | 2041.5 µs | 1068.6 µs | |
| 循序讀 | **1359 MB/s** | 302 MB/s | 約 50 GB/s（[第 42 支 §11.2](lecture42_note.md#112-本機的記憶體頻寬)） |

- **flash 的讀比 DRAM 慢約 1000 倍**（77 µs vs 72 ns），但遠比硬碟的機械尋軌快：符合 [01:37]「比 DRAM 稍慢、比硬碟快很多」：這個「稍慢」其實是三個數量級，所以課本和影片都強調它**本質上是儲存裝置**。
- NVMe 上**寫比讀慢 25 倍**（1.9 ms vs 77 µs），跟 [23:30]「寫很難」的方向一致。但這個數字包含了檔案系統的 journal 和要求裝置把快取寫進 flash 的命令，不全是 flash cell 本身的 program 時間；內部有沒有觸發 erase、搬 block，從外面看不到。
- USB 隨身碟的「寫＋fdatasync」反而只要 160 µs，比它自己的讀還快：很可能它把寫先放在**內部的快取**（[27:41] 的 DRAM cache）就回報完成，沒有真的等寫進 flash。本機沒辦法確認它的內部行為，**待補**。這也是課本 p.580「不要直接拔 USB」的另一個理由。
- 裝置的 SMART 資訊（P/E 次數、wear leveling 的統計）要 root 才讀得到，本機沒有權限，**待補**。

---

## 16. 一頁速查表

| 主題 | 重點 |
|---|---|
| NVM | 斷電不丟資料、不用 refresh；比 DRAM 慢、比硬碟快很多；本質是儲存裝置 |
| 共同做法 | 兩種物理狀態（高／低電阻）→ 陣列 → 軟硬體處理可靠度 |
| 五種技術 | flash（floating gate 的電子）、FeRAM（極化）、MRAM（磁化方向）、PCM（非晶／結晶）、ReRAM（導電細絲） |
| Floating gate | 有電子 → V_T⁺ > V_T → 邏輯 0；讀：V_GS 介於 V_T 和 V_T⁺ |
| Program／erase | 強正壓寫 0、強負壓寫 1 |
| NOR／NAND | NOR 並聯、像 DRAM；NAND 串聯 8–16 個、其他 cell 加 V_T⁺、密度高 40–60%、主流 |
| MLC | n bit 要 2ⁿ 種電荷量；密度高、讀慢、錯誤多（ECC） |
| Page／block | 讀寫：page（512 B–4 KB）；erase：block（32–128 page）；寫過的 page 要 erase 才能再寫 |
| P/E cycle | 5–15 萬次（氧化層被穿隧的電子弄壞）；Linux：MLC 少於 1 萬 |
| FTL | 邏輯 → 實體；改 page 時複製到新 block 再改對應；DRAM cache 合併寫入 |
| Wear leveling | P/E 計數；熱的 block 跟冷的對調（用暫存 block） |
| Read disturbance | 一直讀，旁邊的 cell 被意外 program；讀的計數器，超過就搬 |
| FeRAM | PZT、SBT；hysteresis：B（1）、E（0）；plate line；讀是破壞性的（存 1 時）；10¹² 次 vs flash 10⁵；貴、難縮小、適合嵌入式 |
| 原始碼 | UBI：erase counter；EC 差 ≥ 4096 才把冷資料搬到老 block；erase 失敗標壞 block |
| 實測 | NVMe 隨機讀 77 µs（DRAM 的 1000 倍）、寫＋sync 1.9 ms；USB 讀 178 µs、寫好像被快取了 |

---

## 17. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| DRAM cell、破壞性讀、refresh | [第 41 支](lecture41_note.md) |
| DRAM 的頻寬 | [第 42 支](lecture42_note.md) |
| MRAM、STT-MRAM、PCM、ReRAM、3D 記憶體、roofline model | [第 45 支](lecture45_note.md) |
| 虛擬記憶體（FTL 的類比） | [第 22 支](lecture22_note.md) |
| 製程（lithography）與製程變異 | [第 51 支](lecture51_note.md) |

---

## 附：本講一句話總結

> **DRAM 斷電就丟資料又要 refresh，所以有非揮發記憶體：做一個有高、低電阻兩種穩定狀態的元件排成陣列，再用軟硬體處理可靠度；flash 在 floating gate 裡困住電子讓門檻電壓變高，用強正壓 program（寫 0）、強負壓 erase（寫 1），密度高的 NAND 把 8–16 個串起來，multi-level cell 一格存好幾個 bit，以 page 讀寫、以 block 抹除，P/E cycle 有限，所以裝置裡的 FTL 要做位址對應、wear leveling 和處理 read disturbance；FeRAM 用 ferroelectric 材料的 hysteresis 在零電壓下記住兩種極化，跟 DRAM 一樣是破壞性讀但不用 refresh、耐用很多，只是材料貴、難縮小，適合小型嵌入式裝置。本機實測：NVMe SSD 的隨機 4 KB 讀約 77 µs，是 DRAM 的 1000 倍，寫加上 sync 要 1.9 ms；USB 隨身碟讀 178 µs，寫看起來先進了內部快取；Linux 的 UBI 用 erase counter 差到 4096 才搬資料的方式做 wear leveling。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 10: Main Memory (Flash and FeRAM) -- Part IV 的 YouTube 字幕（57:56）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch10 §10.5–10.5.2（p.577–585）、Linux v6.2.12 原始碼整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4）結果。*
