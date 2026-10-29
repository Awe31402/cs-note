# Advanced Computer Architecture 第四十五支影片完整筆記：主記憶體（五）：STT-MRAM、PCM、ReRAM、3D 記憶體與 roofline model（Chapter 10, Part V）

> **課程**：Advanced Computer Architecture — Chapter 10: Main Memory (MRAMs, PCM, ReRAM, Roofline model) -- Part V（播放清單第 45 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕下載不到）（42:36）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch10** §10.5.3–10.6（p.585–598）
> **章號**：影片 Chapter 10 = 課本 Ch10。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 把「chalcogenide」寫成「chalcogenite」、「ReRAM」寫成「re-ramp」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. MRAM 的構造](#1-mram-的構造)
- [2. Spin-transfer torque](#2-spin-transfer-torque)
- [3. STT-MRAM cell 的讀寫](#3-stt-mram-cell-的讀寫)
- [4. PCM 的物理](#4-pcm-的物理)
- [5. PCM cell 與慢的寫](#5-pcm-cell-與慢的寫)
- [6. PCM 的 wear leveling：start-gap](#6-pcm-的-wear-levelingstart-gap)
- [7. ReRAM：redox ReRAM](#7-reramredox-reram)
- [8. ReRAM：CBRAM](#8-reramcbram)
- [9. 3D 記憶體與 eDRAM](#9-3d-記憶體與-edram)
- [10. Roofline model：三個量](#10-roofline-model三個量)
- [11. Log-log 圖與 ceiling](#11-log-log-圖與-ceiling)
- [12. 怎麼用 roofline 找瓶頸](#12-怎麼用-roofline-找瓶頸)
- [13. 第 10 章總結](#13-第-10-章總結)
- [14. 【實測】本機的 roofline](#14-實測本機的-roofline)
- [15. 一頁速查表](#15-一頁速查表)
- [16. 與後續講次的連結](#16-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
STT-MRAM：pinned layer ＋ 1–2 nm 的 MgO ＋ free layer；平行（低電阻，1）vs 反平行（高電阻，0）
  寫：電流先過 pinned layer → 自旋偏極化 → 把自旋轉給 free layer（反方向：被彈回來的電子累積）
PCM：GST（chalcogenide glass）；結晶（低電阻，1）vs 非晶（高電阻，0）
  短而強的電流 → 熔化急冷 → 非晶；長而弱 → 結晶；寫很慢 → write cancellation、write pausing；start-gap wear leveling
ReRAM：redox（氧空缺形成導電細絲）、CBRAM（銀離子形成細絲）；unipolar（加熱）vs bipolar（反向電壓）重設
  應用：V × G = I（乘法）、電流相加（點積）→ 神經網路
3D：HBM、HMC（TSV、microbump、vault、interposer）；eDRAM（同一個封裝）
Roofline：performance（FLOPS）= bandwidth（B/s）× operational intensity（FLOP/B）
  log-log 圖：頻寬是 45° 的線、峰值效能是水平線 → 找瓶頸
```

---

## 1. MRAM 的構造

[00:00]「主記憶體系列的第五講。」先講更多非揮發記憶體，尤其是 **MRAM**（magnetoresistive RAM）。

（課本圖 10.32）三層：

| 層 | 說明 |
|---|---|
| **Pinned layer** | 鐵磁材料，**磁化方向固定**。[01:16] 鐵磁體：有天然的磁偶極矩（「磁單極不存在，或至少不知道存在，它們總是成對」） |
| **絕緣層** | [01:16] **非常薄**：1–2 nm，例如 **MgO** |
| **Free layer** | 鐵磁材料，磁化方向可以**反轉**（看通過的電流） |

[02:10] 兩種狀態：

| 狀態 | 磁化方向 | 電阻 | 邏輯值 |
|---|---|---|---|
| **Parallel**（平行） | free layer 跟 pinned layer 同方向 | **低**：電流可以**穿隧**過薄薄的絕緣層 | **1** |
| **Anti-parallel**（反平行） | 相反 | **高** | **0** |

[02:57] 讀很簡單：加一個電壓看電流，或通一個已知的電流看電壓 → 知道電阻。

【課本補充】課本 p.585–586：絕緣層很薄，所以有一個量子力學效應叫 **TMR**（tunneling magnetoresistance）：古典物理不允許的穿隧，讓電流可以通過。圖 10.32 畫的是磁化方向**平行於**薄膜的版本；越來越多元件改用**垂直於**薄膜的磁化，操作方式一樣。傳統的寫法是通很大的電流產生磁場，但不能隨製程縮小，而且會干擾旁邊的 cell。

---

## 2. Spin-transfer torque

[02:57] 要改變磁化方向，就是 **STT**（spin-transfer torque）這種 MRAM 變體的關鍵：**STT-MRAM**。

[03:47] 電流通過整個「三明治」，**先**通過 pinned layer 時：

- 電子的**自旋**在量子力學裡有兩個值：+½、−½。一般的電流裡兩種各一半。
- [04:38] 通過磁化方向固定的 pinned layer 後，**大部分電子的自旋變成一樣**：**自旋偏極化電流**（spin-polarized current）。[05:34]「不再是 50／50，可能是 90／10、95／5。」
- 這些電子到了 free layer，把自旋（角動量）**轉移**給 free layer 的電子 → free layer 的磁化方向跟著改 → 在平行、反平行之間切換 = **寫 0 或 1**。

【課本補充】課本 p.586：帶電粒子有角動量就有磁矩。轉移角動量的過程也叫對 free layer 的電子施加**力矩**（torque）→ 名字由來。「延遲和功耗都遠勝過用大磁場的舊方法。」

---

## 3. STT-MRAM cell 的讀寫

[05:34]（課本圖 10.33）cell 看起來像 DRAM 的電容，但其實是一個**電阻**（所以叫 magneto**resistive**，不是 magneto-capacitive）：

```
bit line ── free layer ─ 絕緣層 ─ pinned layer ── access transistor（word line）── select line
```

- [06:27] 新的一條線：**select line**。
- **讀**：加電壓量電流 → 低電阻是 1、高電阻是 0。

**寫**：

| 方向 | 電子怎麼流 | 發生什麼 |
|---|---|---|
| **反平行 → 平行** | [07:17] 從 **pinned layer** 流到 **free layer** | 電子已經被 pinned layer 偏極化，把同樣的方向「教」給 free layer |
| **平行 → 反平行** | [07:17] 從 **free layer** 流到 **pinned layer** | [08:16] 自旋**不對**的電子過不了 pinned layer，被**彈回來**累積在 free layer → 把相反的自旋轉給 free layer → [09:06]「越來越多被拒絕的『移民』電子累積」，自我回授，最後磁化翻轉 |

[09:06]「STT-RAM 講完了：很簡單，兩個狀態 HRS、LRS。」

【課本補充】課本 p.587–588：

- 反平行 → 平行：bit line 電位**高於** select line；平行 → 反平行：select line 電位高於 bit line。彈回來的電子是在 MgO 和 pinned layer 的**交界**被反射的。
- 優點：**幾乎沒有漏電**（SRAM、DRAM 的漏電是功耗的一大部分，[第 46 支](lecture46_note.md)）；讀寫要的電流都小；**耐用度很高**。
- 缺點：沒有 DRAM 快：要等 free layer 的磁場反轉並達到一定強度；兩種狀態的電阻可能只差 **20%** → 要很精密的感測電路；die 溫度變化大時有**熱穩定性**的問題。
- 「STT-MRAM 到今天算是相當成熟的技術」，但 2020 年時大量生產還有挑戰。

---

## 4. PCM 的物理

[09:06] **PCM**（phase change memory）用的是另一種物理。

- 材料：**chalcogenide glass**，最流行的是 **GST**（germanium-antimony-tellurium，GeSbTe）。[09:06]「又是稀有元素。」
- [10:06] 兩種狀態：

| 狀態 | 電阻 | 邏輯值 |
|---|---|---|
| **結晶**（crystalline） | 低（電流很容易通過） | **1** |
| **非晶**（amorphous，混亂的） | 高 | **0** |

[11:00] 讀很簡單、很快：加電壓量電流。[11:00] 寫普遍很慢：

| 技術 | 讀 | 寫 |
|---|---|---|
| Flash | 中等（NAND、MLC 比較難） | 非常慢 |
| FeRAM | 有點慢 | 慢（要把電壓升到某個點），但比 flash 快 |
| MRAM | 快 | 慢：要等磁化方向改變 |
| PCM | 快 | 慢 |

[11:58]「一般來說非揮發記憶體**讀快、寫慢**，這是它們的缺點之一。」

[11:58] 怎麼切換：

| 電流脈衝 | 結果 | 寫入 |
|---|---|---|
| **短、強** | 產生大量的熱、材料熔化 → 急冷，原子來不及排好 → **非晶** | 0（**reset**） |
| **長、弱** | 慢慢冷 → 排得整齊 → **結晶**（比較花時間） | 1（**set**） |

[12:53] 也可以做 **multi-level** PCM：加熱到不同程度，調整非晶和結晶的比例。

【課本補充】課本 p.588：chalcogenide glass 是由一種或多種 **chalcogen** 元素（硫、硒、碲）組成的非晶態固體。結晶態結構有序、有很多自由電子 → 電阻低。資料存在**相**裡，可以保存很久：2020 年的商用 PCM 至少可以保存 **10 年**。可以**直接覆寫**（不像 flash 要整個 block erase）；讀**不是**破壞性的（不像 DRAM、FeRAM）。multi-level 例：全結晶、全非晶之間再加兩個部分結晶的狀態 → 4 種狀態 = 2 bit。

---

## 5. PCM cell 與慢的寫

[12:53]（課本圖 10.34、10.35）兩個電極夾著 GST，有一小塊**可程式化的區域**會改變結構；下面有一個**加熱器**（電阻）。cell 是 **1T1R**（一個 transistor、一個電阻）。

[13:55] 讀：加電壓量電流。寫：從 bit line 送一個電流脈衝進 cell。

[13:55] 很多設計把 access transistor 放在 **cell 和地之間**，而不是 bit line 和 cell 之間：[14:44] program 常常需要高電壓或大電流，不想讓 transistor 削弱它；「刺激比小 transistor 撐得住的還大」→ 讓 cell 直接接到電流脈衝。

[14:44] 問題：寫很慢，可靠度跟 flash 類似。標準做法：**write cache**、write buffer，吸收連續的寫。

[15:37] PCM 的寫可以**中途停下來**：寫就是一直加電流脈衝、慢慢改變材料結構。

- **Write pausing**：寫到一半，來了一個在 critical path 上的讀 → 停下寫、先服務讀、再回來寫完。
- [16:21] cell **老化**後，加了一段時間的脈衝也不一定完全變成結晶 → 加一段、**驗證**、還沒好就繼續加，直到確定寫對了；中間有讀就先去讀。

【課本補充】課本 p.589–590：讀：先 precharge bit line 再打開 transistor；低電阻的話 bit line 很快放電。memory controller 可以降低寫的優先順序、加 DRAM write cache、丟掉**沒效果的寫**（連續寫同一個位置，第一個寫就沒效果）。課本也講 **write cancellation**：寫到一半直接取消（把 write enable 拉低），cell 會停在不確定的狀態，但我們知道要寫什麼，先放在 write buffer 裡，之後再寫。write pausing 的背景：用久了，結晶速率變得不一樣，寫的時間越來越**不確定**；multi-level cell 更複雜 → 一般都是一輪一輪地寫、每輪驗證。

---

## 6. PCM 的 wear leveling：start-gap

[17:21] PCM 也一定要 wear leveling（像 flash，[第 44 支 §9](lecture44_note.md#9-flash-translation-layer-與-wear-leveling)）。

**Start-gap wear leveling**：定期把位置 L 的 block 搬到 **L + 1**，L + 1 的搬到下一個……

- 「可以半數學地證明」這樣所有的 block 都會被搬到，整體的磨損大致**均勻**。
- [18:14] 不像根據精確計數和預測的方法那麼聰明，但**非常簡單、開銷很低**，而且有一定的保證。

【課本補充】課本 p.590：高的寫入電流和材料反覆的熱循環大幅降低 PCM 的可靠度；PCM 更在乎**寫**（比讀激烈得多）。start-gap：定期把 block L 搬到 L′，L 和 L′ 是一對一的對應；最簡單的是 L′ = L + 1，也可以用**密碼學**的函數得到更好的保證。

---

## 7. ReRAM：redox ReRAM

[18:14] **ReRAM**（resistive RAM）「現在很重要，因為在**神經網路**裡找到了用途」。

- 低電阻是 1、高電阻是 0。
- 電阻是過去加過的**電壓**的函數：對**電位**敏感，不是對電流。
- 兩種：**redox ReRAM** 和 **CBRAM**。

[18:14] **Redox ReRAM**：兩個電極夾一層**過渡金屬氧化物**（寫成 VO₂，V 代表某種金屬：鎳、鈦等）。

- [19:15] 兩種電荷載子：**氧離子**（O²⁻）和**氧空缺**（oxygen vacancy，帶正電，「就像電洞是電子的空缺」）。
- [19:15] 加高電壓：氧化層分解成氧離子和氧空缺（「跟摻雜的矽非常像」）。
- [20:20] 氧離子往 **anode** 跑、累積在那裡（也可能跟 anode 的材料反應形成一層氧化物）；氧空缺留下來，沿著 anode 和 cathode 之間**排成一條**：**導電細絲**（conductive filament）。
- [21:15] 電子沿著氧空缺「跳、跳、跳」從 cathode 到 anode；會形成好幾條細絲 → **低電阻**。

[21:15] 把細絲**打斷**回到高電阻（reset）：

| 方式 | 做法 | 特點 |
|---|---|---|
| **Unipolar switching** | [22:13] 送一個很大的 reset 電流 → 局部**加熱**（像 PCM）→ 困在 anode 的氧離子被釋放、回到氧化層、跟氧空缺結合 → 細絲斷了 | 靠加熱，慢 |
| **Bipolar switching** | [22:13] anode 加**負**電壓 → 把氧離子推回氧化層 → 跟細絲結合、斷掉 | [23:04] 比較可控、比較快；但要更多電子電路 |

「這不只是 ReRAM 才有，這一家族的元件都有這兩種。」

【課本補充】課本 p.590–592：常見材料 NiO、TiOx；電極用鉑（Pt）、銥（Ir）、銀（Ag）。I-V 曲線有 hysteresis。HRS → LRS（0 → 1）叫 **set**，反過來叫 **reset**。每個 cell 一開始要經過一次 **forming**：加很大的電壓讓 dielectric 崩潰、形成第一條細絲。anode 上的氧化層通常是非晶、顆粒大，細絲在顆粒的**邊界**形成。unipolar 只看電壓大小，bipolar 還看正負號；bipolar 的 reset 電流比較小。名字來自一連串的 **redox**（氧化還原）反應。Important Point 20 是這段的總結。

---

## 8. ReRAM：CBRAM

[23:04] **CBRAM**（conductive bridging RAM）：

| 部分 | 材料 |
|---|---|
| **電化學活性電極**（會跑進中間層） | [23:52] **銀**（最好）、銅、鎳 |
| **惰性電極** | 鉑、銥（「鉑是珠寶的材料」） |
| 中間的**電解質** | 鍺硫化合物、SiO₂、TiO₂、氧化鉭…… |

[24:48]

- **Set**：活性電極加很高的**正**電壓 → 銀分解成 Ag⁺ 和電子 → Ag⁺ 往惰性電極跑 → 形成一條**銀**的細絲 → 導電。
- **Reset**：[25:43] 活性電極加**負**電壓（bipolar）→ 銀離子跑回活性電極沉積 → 細絲斷 → 高電阻。

[25:43] 跟 redox 的差別：redox 是**氧化層**自己分解；CBRAM 是**電極**自己分解、搭出一座橋（所以叫 active electrode）。

[26:38]「非揮發記憶體的短討論結束：有很多化學、各種材料，靠熱、電化學、磁的效應形成和打斷細絲。」

【課本補充】課本 p.592–593：CBRAM 的電解質也可以是 Ta₂O₅、ZrO₂。Ag⁺ 到了惰性電極（cathode）吸收一個電子、沉積在表面，慢慢形成一條 Ag 原子的通道；電源關掉後細絲還在 → 邏輯 1。

**ReRAM 用在神經網路**（課本 p.593）：ReRAM 可以有很多種中間的電阻值。電導是 G 的話，**V × G = I**（歐姆定律）→ 輸入電壓就做了一次**乘法**；一整個陣列的電流加在一起（克希荷夫定律）→ 一個**點積**：深度神經網路最吃效能的運算（[第 56 支](lecture56_note.md)、[第 57 支](lecture57_note.md)）。

---

## 9. 3D 記憶體與 eDRAM

[26:38]「DRAM 的未來：增加容量。」

**HMC**（hybrid memory cube）和 **HBM**（high bandwidth memory）：**3D 疊起來**的 DRAM 陣列：

- [27:37] 像大樓的好幾層，每層一個陣列；層之間用 **TSV**（trans-silicon via）或 **microbump**（一邊一個銅的凸塊，互相接觸）連接。
- 每一層切成很多 block；**一整柱**的 block 叫一個 **vault**：整個 3D 結構裡的基本單位。
- 跟 CPU 之間用 **interposer**：非常高頻寬的連線，[28:34]「大約是一般 DDR channel 的兩倍頻寬：128 bit」，也可以分成兩個子 channel。沒有長長的銅線，用 microbump 這類東西接 → 快、低延遲、高頻寬。

[28:34] **eDRAM**（embedded DRAM）：一層矽放 transistor，上面是好幾層金屬，再上面是一層 DRAM。

- [29:29] 以前很難做：晶片和 DRAM 的**製程不同**。現在可以分開做，再放進**同一個封裝**；或者用能同時處理兩者的先進製程。
- 結果：transistor 和 DRAM 之間有**超高頻寬**的連線。
- [30:21] 問題：**功率和溫度**：以前 DRAM 是分開的，現在 CPU 加熱 DRAM、DRAM 也加熱 CPU（[第 47 支](lecture47_note.md)）。

【課本補充】課本 p.593–594：最常見的整合方式是晶片和 3D 記憶體**並排**放，用 interposer 連接，interposer 通常有**好幾條** 128 bit 寬的連結。頻寬高的兩個原因：每個 vault 可以平行讀很多 bit；CPU 和 3D 記憶體離得很近，可以用很寬的 bus。eDRAM：處理器和 DRAM 分別做成兩個 die、放在同一個封裝裡叫 **2.5D 整合**，被認為比較可行；封裝裡也要有類似 interposer 的高頻寬網路。

---

## 10. Roofline model：三個量

[30:21]「記憶體系統講完了，剩下很小的一節：**roofline model**。」一個**粗估**（back of the envelope）的效能模型：用一張簡單的圖比較兩個電腦系統。

[31:18] 三個量：

| 量 | 單位 |
|---|---|
| **Performance** | 每秒幾個運算（整數或浮點，通常是 FLOPS） |
| **Memory bandwidth** | bytes／秒（實際做到的頻寬） |
| **Operational intensity** | 每從記憶體讀一個 byte，做幾個運算（FLOP／byte） |

```
Performance = Memory bandwidth × Operational intensity
 FLOP/s    =    byte/s        ×    FLOP/byte          （課本式 10.1）
```

[32:12] 只有兩個是獨立的。operational intensity 是新東西：讀一個 byte 進來，是看一眼就丟，還是對它做 10 個運算？

[33:03] **roofline 的關鍵創新：把 operational intensity 放在正中央**（x 軸）。

- cache 加大 → 更能利用從記憶體來的資料 → operational intensity 變高。
- 程式寫得好（cache locality 好、資料留在手邊久一點）也一樣 → roofline 圖是**硬體加軟體**一起的。

【課本補充】課本 p.594–595：roofline model 出自 **Williams et al., 2009**。其他把 DRAM 吞吐量跟效能連起來的模型，缺點是跟 workload 有關、常數太多、不直覺；roofline 很簡單地回答「這個 workload 是 memory bound 還是 compute bound，還是兩者都是」。operational intensity 反映 CPU 的處理能力和 **cache 的效率**；memory bandwidth 包含 caching、memory controller 的最佳化、prefetching 的效果；兩者通常用**效能計數器**量。課本說明「FLOP」是一個浮點運算、「FLOPs」是複數、「FLOPS」是每秒幾個。

---

## 11. Log-log 圖與 ceiling

[34:00] 固定頻寬 B 時：y = Bx：一條過原點、斜率 B 的直線（課本圖 10.39）。

[34:56] 兩邊取 log：

```
log y = log x + log B          （課本式 10.2）
```

→ 在 **log-log** 圖上，所有固定頻寬的線斜率都是 1（**45°**），頻寬變成**截距**：截距越高、頻寬越大（課本圖 10.40）。

[35:42] **Ceiling**（天花板）：

| 天花板 | 在圖上 | 為什麼 |
|---|---|---|
| **峰值頻寬** | 45° 的線 | 不可能在它上面 |
| **峰值效能** | 水平線 | [35:42] 效能不只看記憶體，還看 ILP、功能單元數…… |

[35:42] 兩條線以下就是**可行區域**（feasible region）。[36:30] 在可行區域裡，想做到峰值效能、盡量有效地使用資料。

⚠️ [35:42] 老師說 operational intensity 也有上限，所以可行區域是「**五邊形**」（「不是梯形：一、二、三、四、五個邊」）。課本 p.596 說兩條 ceiling 圍出的是**梯形**區域，沒有提 operational intensity 的上限。

[36:30]（課本圖 10.42）可以有**好幾條** ceiling：

| 頻寬 ceiling | 效能 ceiling |
|---|---|
| B1：沒有 prefetching（naive） | P1：預設（沒有 SIMD） |
| B2：有 prefetching | P2：有 SIMD（向量指令） |
| B3：理論最大值 | P3：理論峰值 |

[37:24]「這裡的頻寬是**實際做到的**頻寬。」[38:15] 不同的架構技術 → 很不同的 ceiling 和可行區域。

【課本補充】課本 p.596：峰值效能有兩種量法：看處理器的 data sheet 算理論最大值，或寫專門的 micro-benchmark 把處理器操到極限、用效能計數器量。除非改變系統的設定（開新的架構最佳化或提高頻率），不可能超過峰值。這些 ceiling 就是 **roofline**（屋頂線），這種圖叫 **roofline diagram**。

---

## 12. 怎麼用 roofline 找瓶頸

[39:13]（課本圖 10.43）

| 點 | 位置 | 該做什麼 |
|---|---|---|
| **P** | 很靠近**頻寬** ceiling | 同樣的 operational intensity 沒辦法再快 → 投資在**提高頻寬**（例如 prefetching 從 B1 拉到 B2，就能沿著垂直方向往上） |
| **Q** | 很靠近**效能** ceiling | [40:04] 改 operational intensity 也沒用，要提高峰值效能（例如峰值被功率限制住，就要降功率） |
| **R** | 離兩者都很遠 | 「完全沒有最佳化」：有很多空間，提高 operational intensity、提高效能 |

[39:13]「一個很好的工具：告訴你要做哪些軟硬體最佳化，才能在比較有利的點運作，也清楚指出瓶頸。」

【課本補充】課本 p.597–598：Q 已經用完所有計算能力，記憶體頻寬不是問題（沒有用到它能用的全部）→ 要開更新的架構最佳化增加計算能力。也可以反過來用：給定 operational intensity，要維持某個效能最少需要多少頻寬；也可以定一條**下限**，保證一定的效能。後來有很多延伸，例如加入**能量**（Choi et al., 2013）。

---

## 13. 第 10 章總結

[40:04]「這是我們最喜歡看的最後一張投影片。」

- [40:53] DRAM cell 看起來很簡單（確實很簡單），但一點也不簡單：陣列設計、channel／rank／bank group／bank／array／row／column 的整個階層，「那是一個世界」。
- 要管 DRAM 就要懂 DDR 協定：非同步、同步協定、狀態圖、時序。
- memory controller：[40:53]「大約十五年前還是一顆獨立的晶片」，現在在處理器裡；要完全知道 DRAM 的狀態、控制每個面向；「它本身就像一個小處理器」：有小的 write cache，盡量最佳地排程 transaction 和命令。
- [41:45] 非揮發記憶體：很多人說它是未來，也有人把它看成**神經網路**的基本元件（電阻可調）；它們通常有兩種物理狀態，但可以有更多；「至少是很快的儲存裝置」。

[41:45] 課本的第二部分結束。第三部分：**可靠度、功率、溫度、安全的架構、機器學習的架構**。

---

## 14. 【實測】本機的 roofline

程式在 `experiments/lecture45/roofline.c`（`gcc -O3 -march=native -fopenmp`）：

- 一個 512 MB 的 float 陣列（遠大於 24 MB 的 L3），每個元素做 **k 次** `x = x * a + b`：2k 個 FLOP，讀 4 B、寫 4 B。
- → **operational intensity = 2k ／ 8 = k／4 FLOP/byte**。k 從 1 到 1024，在同一台機器上掃過整條 roofline。
- 每 64 個元素一組（8 個 AVX2 向量）：k 次運算時資料都在暫存器或 L1 裡，只有開頭讀一次、最後寫一次會碰到 DRAM。組語裡有 24 個 `vfmadd` 指令（FMA）。
- thread 綁在 P-core（cpu0、2、4、6、8、10），每個 k 跑 3 次取最快的。

| k | OI（FLOP/B） | 1 個 core：GFLOPS | 1 個 core：GB/s | 6 個 core：GFLOPS | 6 個 core：GB/s |
|---|---|---|---|---|---|
| 1 | 0.25 | 7.9 | 31.8 | 9.7 | 38.8 |
| 2 | 0.5 | 15.5 | 31.0 | 20.5 | 41.1 |
| 4 | 1 | 30.2 | 30.2 | 41.3 | 41.3 |
| 8 | 2 | 51.6 | 25.8 | 80.6 | 40.3 |
| 16 | 4 | 77.3 | 19.3 | 164.7 | **41.2** |
| 32 | 8 | 99.9 | 12.5 | 325.5 | 40.7 |
| 64 | 16 | 114.7 | 7.2 | 587.6 | 36.7 |
| 128 | 32 | 124.7 | 3.9 | 640.0 | 20.0 |
| 256 | 64 | 130.8 | 2.0 | 666.9 | 10.4 |
| 512 | 128 | 133.9 | 1.0 | 686.8 | 5.4 |
| 1024 | 256 | **135.8** | 0.5 | **691.5** | 2.7 |

把 6 個 core 那兩欄畫在 log-log 圖上，就是 [§11](#11-log-log-圖與-ceiling) 的屋頂：

```
log GFLOPS
  700 ┤                        ●───●───●───●   ← 效能 ceiling（約 690 GFLOPS）
      │                    ●
  300 ┤                ●
      │            ●                ← 45° 的頻寬 ceiling（約 41 GB/s）
   80 ┤        ●
   40 ┤    ●
   10 ┤●
      └────┬────┬────┬────┬────┬────┬── log OI（FLOP/B）
          0.5   2    8   32  128
```

- **左邊（OI ≤ 8）是 memory bound**：6 個 core 的頻寬一直卡在 **約 41 GB/s**，GFLOPS 跟著 OI 成正比（每加倍 OI，GFLOPS 也加倍）= 圖上那條 45° 的線（[§11](#11-log-log-圖與-ceiling)）。
- **右邊（OI ≥ 64）是 compute bound**：GFLOPS 停在 **約 690**，再提高 OI 也沒用，頻寬反而越用越少 = 水平的 ceiling。
- **轉折點**（ridge point）≈ 690 ÷ 41 ≈ **17 FLOP/B**：本機要每讀一個 byte 做 17 個以上的浮點運算，才能把運算單元餵飽。大部分的「掃一遍陣列」的程式（OI < 1）在本機都被記憶體卡住。
- **1 個 core 的效能 ceiling** 是 136 GFLOPS：每個 P-core 每 cycle 能做 2 個 FMA × 8 個 float × 2 FLOP = **32 FLOP**，136 ÷ 32 ≈ **4.25 GHz**，接近 P-core 的 turbo 頻率。6 個 core 一起跑時每個 core 約 115 GFLOPS：全核心一起跑時頻率比較低。
- 1 個 core 的頻寬只有約 31 GB/s，6 個 core 才到 41 GB/s：跟 [第 42 支 §11.2](lecture42_note.md#112-本機的記憶體頻寬) 一樣，一個 core 被它自己能同時等的 miss 數限制。這裡的頻寬算了讀＋寫（每個元素 8 B），所以跟第 42 支只讀的 50 GB/s 不能直接比。
- 這兩條 ceiling 對應課本圖 10.42 的哪一條：這裡用了 SIMD（AVX2 FMA）和硬體 prefetching，所以大約是「有 SIMD」的效能 ceiling 和「有 prefetching」的頻寬 ceiling；理論的峰值（P3、B3）要知道頻率和 DIMM 規格才能算，**待補**。

---

## 15. 一頁速查表

| 主題 | 重點 |
|---|---|
| STT-MRAM | pinned ＋ MgO（1–2 nm）＋ free；平行（低電阻、1）、反平行（高電阻、0）；TMR 穿隧 |
| STT 寫 | 電流先過 pinned → 自旋偏極化 → 轉給 free；反方向：被彈回來的電子累積在 free；幾乎沒漏電、耐用；電阻差可能只有 20% |
| PCM | GST；結晶（1）vs 非晶（0）；短強電流 → 非晶（reset）、長弱 → 結晶（set）；保存 10 年；不破壞、可直接覆寫 |
| PCM 的寫 | 慢 → write cache、丟掉沒效果的寫、write cancellation、write pausing（寫一輪、驗證、可插入讀） |
| Start-gap | 定期把 L 搬到 L+1；簡單、開銷低、有保證 |
| Redox ReRAM | 過渡金屬氧化物；氧空缺形成導電細絲；unipolar（加熱）vs bipolar（反向電壓）reset |
| CBRAM | 活性電極（Ag、Cu、Ni）分解出離子搭橋；惰性電極（Pt、Ir）；bipolar reset |
| ReRAM ＋ 神經網路 | V × G = I（乘）、電流相加（點積） |
| 3D | HBM、HMC：TSV、microbump、vault、interposer（好幾條 128 bit）；eDRAM／2.5D：同一封裝、要顧溫度 |
| Roofline | FLOP/s = byte/s × FLOP/byte；x 軸是 operational intensity；log-log：頻寬 45°、峰值效能水平 |
| Ceiling | 有／沒有 prefetching、有／沒有 SIMD；P 靠頻寬、Q 靠效能、R 都很遠 |
| 實測 | 本機 6 個 P-core：頻寬 ceiling 約 41 GB/s、效能 ceiling 約 690 GFLOPS、轉折點約 17 FLOP/B；1 個 core 136 GFLOPS ≈ 32 FLOP/cycle × 4.25 GHz |

---

## 16. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Flash、FeRAM、wear leveling | [第 44 支](lecture44_note.md) |
| DRAM 頻寬、prefetching | [第 42 支](lecture42_note.md)、[第 26 支](lecture26_note.md) |
| 大量平行的運算單元（GPU） | [第 19 支](lecture19_note.md)、[第 20 支](lecture20_note.md) |
| 漏電功率 | [第 46 支](lecture46_note.md) |
| 溫度（3D 疊起來的熱） | [第 47 支](lecture47_note.md) |
| 機器學習的運算（點積、矩陣乘法） | [第 55 支](lecture55_note.md)–[第 58 支](lecture58_note.md) |

---

## 附：本講一句話總結

> **STT-MRAM 用 1–2 nm 的 MgO 隔開固定的 pinned layer 和可翻轉的 free layer，平行是低電阻、反平行是高電阻，用先經過 pinned layer 而自旋偏極化的電流（或反方向被彈回來的電子）把自旋轉給 free layer 來寫；PCM 用 GST 的結晶和非晶兩相，短而強的電流熔化急冷成非晶、長而弱的電流慢慢結晶，寫很慢所以有 write cancellation、write pausing 和 start-gap wear leveling；ReRAM 靠氧空缺（redox）或銀離子（CBRAM）形成、打斷導電細絲，可調的電阻還能直接做乘法和點積給神經網路用；DRAM 的未來是 HBM、HMC 這種 3D 堆疊和同封裝的 eDRAM；roofline model 把 operational intensity 放在 x 軸，效能等於頻寬乘以 operational intensity，log-log 圖上 45° 的頻寬線和水平的峰值效能線圍出可行區域，看一個點靠近哪條線就知道瓶頸在哪。本機實測：同一個 kernel 掃過 0.25 到 256 FLOP/byte，6 個 P-core 先被約 41 GB/s 的頻寬卡住、再被約 690 GFLOPS 的運算能力卡住，轉折點約 17 FLOP/byte，單一 core 的 136 GFLOPS 剛好是每 cycle 32 個 FLOP 乘上約 4.25 GHz。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 10: Main Memory (MRAMs, PCM, ReRAM, Roofline model) -- Part V 的 whisper 轉錄稿（本機 `large-v3-turbo`，42:36）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch10 §10.5.3–10.6（p.585–598）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4）結果。*
