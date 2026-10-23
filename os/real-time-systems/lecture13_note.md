# Real Time Systems 第十三支影片完整筆記：PCP 的分析（Lecture 14）

> **課程**：Real Time Systems — Lecture 14：Analysis of the PCP Protocol（播放清單第 13 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（13:12，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch3 §6（PDF p.86–88）
> **標記說明**：
> - `【口頭】`：老師在課堂上口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這堂課沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（`檔名:行號`），用來對照觀念
> - `【實測】`：在本機（Ubuntu、kernel 6.5.0、x86-64）實際執行的結果
> - `⚠️`：逐字稿缺字、或課堂／課本／原始碼說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 三種 inversion 複習](#1-三種-inversion-複習)
- [2. 老師的例子：六個任務](#2-老師的例子六個任務)
- [3. Inversion 表的性質](#3-inversion-表的性質)
- [4. 【課本補充】例題 3.2、3.3](#4-課本補充例題-3233)
- [5. 【實測】自動算 inversion 表](#5-實測自動算-inversion-表)
- [6. 加入阻擋時間的 Lehoczky 條件](#6-加入阻擋時間的-lehoczky-條件)
- [7. 動態優先權系統為什麼不用 PCP](#7-動態優先權系統為什麼不用-pcp)
- [8. 三個協定總比較](#8-三個協定總比較)
- [9. 一頁速查表](#9-一頁速查表)
- [10. 與後續講次的連結](#10-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

把 PCP 用在可排程性分析上：

1. 對每個任務，算出它可能遭受的三種 inversion 的最長時間 → 三張**inversion 表**。
2. 任務只會遭受其中**一種、一次** → Bᵢ = 三張表第 i 列的最大值。
3. 把 Bᵢ 加進 Lehoczky 條件。
4. 總結 PIP / HLP / PCP。

---

## 1. 三種 inversion 複習

[00:00]–[01:00]

- PCP 的 inversion 比 HLP **少**：HLP 一拿到資源就升到 ceiling，PCP 不會。
- 高優先權任務仍可能遭受三種 inversion：
  1. **Direct**：等低優先權任務釋放自己需要的資源。
  2. **Inheritance-related**：低優先權任務繼承了更高優先權，擋住中間任務。
  3. **Avoidance-related**：PCP 獨有。要的資源空著，但優先權不 > CSC。

---

## 2. 老師的例子：六個任務

[01:00]–[05:30]

T1…T6，**編號 = 優先權順序**（T1 最高、T6 最低）。⚠️ 老師一開始說「數字越大越高」，隨即更正：這裡的數字只是位置。各邊（任務使用資源的時間）在投影片的圖上，逐字稿唸出的是「5、2、8、4、1、5、8」，但沒唸出哪條邊對哪個資源（圖待補）。以下是老師口頭解釋過的格子：

**Direct**：

| 被擋者 | 擋人者 | 時間 | 原因 |
|---|---|---|---|
| T1 | T2 | 2 | T1 剛到達、要資源，T2 剛好拿著 |
| T1 | T3 | 8 | 同上 |
| T2 | T3 | 8 | T3 剛拿到資源，T2 就來要 |
| T3 | T4 | 1 | — |
| T4 | T6 | 8 | T6 拿著資源 8 單位 |
| T5 | — | — | T5 不需要資源，**永遠不會** direct blocking |

**Inheritance-related**：

- T2 被 T3 擋 8 單位：T3 剛拿到資源 → T1 來了要同一個資源 → T3 繼承 T1 的優先權 → T2 到達，雖然可以跑，但 T3 現在比它高，繼續霸佔 CPU → T2 最多等 8。
- T5 被 T6 擋：同理。T6 拿著資源 → T4 來等 → T6 繼承 T4 的優先權 → T5 跑不了。

**Avoidance-related**：

- T3 被 T4 擋 1 單位：T4 拿著 R2；Ceil(R2) = max(pri(T3), pri(T4)) = pri(T3)。T3 來了要 R1（**空著**），但它沒有設定 CSC、優先權也不 > CSC → 等 T4 做完（1 單位）。
- T4 被 T6 擋：T6 拿著 R3，Ceil(R3) = pri(T4)。T4 不需要 R3，它要 R2（空著），一樣因為不 > CSC 而被擋。

老師接著放了另一組練習（「這不是我要做的，是學生要做的」），跳過。

---

## 3. Inversion 表的性質

[06:00]–[07:31]（課本 p.88）

- 每張表都是**上三角矩陣**：任務只會被**比它低**優先權的任務反轉。T1 不會被任何人反轉；T2 只會被 T3、T4…反轉……下三角全是 0。
- 任務最多遭受**一種**、**一次** inversion（上一講的 single blocking）→

> **Bᵢ = max(direct 表第 i 列, inheritance 表第 i 列, avoidance 表第 i 列)**

課本式 3.5：btᵢ = maxⱼ { b_idj, b_iij, b_iaj }，j 取比 i 低優先權的任務。

【課本補充】（p.88）

- Avoidance 表和 inheritance 表很像，差別在**不需要任何資源**的任務：它們從不請求資源，不會遭受 avoidance inversion（但可能遭受 inheritance-related）。
- 分析的前提：任務**釋放資源後不再取得新資源**（先取得、後釋放兩階段）。不遵守這個紀律，很難算出 inversion 時間的上界（p.86）。

---

## 4. 【課本補充】例題 3.2、3.3

**Ex 3.2**（p.86–87）：pri(T1) = 10、pri(T2) = 7、pri(T3) = 5、pri(T4) = 2。

| 任務 | 資源與時間 |
|---|---|
| T1 | CR1 40 ms、CR2 10 ms |
| T2 | 不用資源 |
| T3 | CR1 60 ms |
| T4 | CR2 20 ms |

Ceil(CR1) = max(10, 5) = 10；Ceil(CR2) = max(10, 2) = 10。

課本 Table 3.1：

| 任務 | Direct T2/T3/T4 | Inheritance T2/T3/T4 | Avoidance T2/T3/T4 |
|---|---|---|---|
| T1 | x / 60 / **40** | x / x / x | x / x / x |
| T2 | x / x / x | x / 60 / 20 | x / x / x |
| T3 | x / x / x | x / x / 20 | x / x / 20 |

**Ex 3.3**（p.87–88）：T1…T6 依優先權遞減；T1 用 CR1 20；T2 用 CR2 30；T3 用 CR1 10、CR3 50；T4 用 CR2 40；T5 不用；T6 用 CR1 10、CR3 10。課本 Table 3.2 見下一節（程式結果完全一致）。

---

## 5. 【實測】自動算 inversion 表

寫一個 Python 程式，只輸入「優先權順序 ＋ 每個任務用哪些資源、用多久」，自動算三張表。規則（j 比 i 低）：

```python
d = max([uj[r] for r in uj if r in ui] or [0])                                  # j holds what i needs
h = max([uj[r] for r in uj if any(pri[k] > pri[i] for k in users[r])] or [0])   # j inherits from someone above i
a = max([uj[r] for r in uj if ceil[r] >= pri[i] and any(r2 != r for r2 in ui)] or [0])  # i asks for a FREE resource, blocked by ceiling of what j holds
```

- Direct：j 拿著 i 也要的資源。
- Inheritance：j 拿著某資源，而有比 **i 更高**的任務也用它（j 可能繼承那個更高的優先權）。
- Avoidance：j 拿著 ceiling ≥ pri(i) 的資源，而 i 要的是**另一個**（空著的）資源。

結果：

```
== Book Ex 3.2
      Direct       | Inherit      | Avoid        | B
        T2  T3  T4 |   T2  T3  T4 |   T2  T3  T4
  T1     x  60  20 |    x   x   x |    x  60  20 | 60
  T2     -   x   x |    -  60  20 |    -   x   x | 60
  T3     -   -   x |    -   -  20 |    -   -  20 | 20
== Book Ex 3.3
      Direct               | Inherit              | Avoid                | B
        T2  T3  T4  T5  T6 |   T2  T3  T4  T5  T6 |   T2  T3  T4  T5  T6
  T1     x  10   x   x  10 |    x   x   x   x   x |    x   x   x   x   x | 10
  T2     -   x  40   x   x |    -  10   x   x  10 |    -  10   x   x  10 | 40
  T3     -   -   x   x  10 |    -   -  40   x  10 |    -   -  40   x  10 | 40
  T4     -   -   -   x   x |    -   -   -   x  10 |    -   -   -   x  10 | 10
  T5     -   -   -   -   x |    -   -   -   -  10 |    -   -   -   -   x | 10
```

- **Ex 3.3 和課本 Table 3.2 完全一致**。
- Ex 3.2 有兩處不同：

⚠️ 課本 Table 3.1 的 T1 被 T4 direct 擋 **40**，但課本自己的文字說「T1 can suffer direct inversion … due to T4 for **20**ms」，而 T4 用 CR2 只有 20 ms。程式算出 20。

⚠️ 課本說 T1「是最高優先權，不會遭受 avoidance inversion」，表中 avoidance 全為 x。但 T1 用**兩個**資源：若 T3 拿著 CR1（Ceil = 10），T1 要的是空著的 CR2，此時 pri(T1) = 10 不 > CSC = 10 → 依 grant clause **會被擋**，這是 avoidance。程式算出 60、20。數值和 direct 一樣，所以 B₁ 都是 60，結論不變。（Ex 3.3 的 T1 只用一個資源，就沒有這個情況，和課本一致。）

---

## 6. 加入阻擋時間的 Lehoczky 條件

[07:31]–[09:04]

Bᵢ = 任務 Tᵢ 因資源共享可能遭受 priority inversion 的最長時間（三張表第 i 列取 max）。Tᵢ 趕上第一個 deadline 的條件：

> **eᵢ + Bᵢ + Σⱼ₌₁^{i−1} ⌈pᵢ/pⱼ⌉·eⱼ ≤ pᵢ**

- 假設 pᵢ = dᵢ；T1 優先權最高、Tₙ 最低，週期也照這個順序（RMA）。
- 唯一的改變：把 inversion 時間**加到執行時間上**。

⚠️ 逐字稿 [08:02] 說「floor of pᵢ/pⱼ」。Lehoczky 條件用的是**上取整** ⌈pᵢ/pⱼ⌉（第 7 支影片推導過），應是口誤或字幕錯誤。

【課本補充】（p.88 式 3.6）再加上 self suspension：**eᵢ + bssᵢ + btᵢ + Σ⌈pᵢ/pⱼ⌉·eⱼ ≤ pᵢ**，bssᵢ 是自我暫停造成的延遲（第 8 支影片筆記的課本補充）。

---

## 7. 動態優先權系統為什麼不用 PCP

[09:04]–[10:02]

- 動態優先權（例如 EDF）下，任務的優先權隨時間改變。
- 每次某任務的優先權改變，**所有它可能用到的資源的 ceiling** 都要更新 → 原本靜態的 ceiling 變成動態。
- 處理開銷高到無法接受 → **很少這樣做**。
- 所以實務上用 **RMS 類**的系統：任務優先權大致固定，排程簡單、也好分析。

---

## 8. 三個協定總比較

[10:02]–[12:30]

| | PIP | HLP | PCP |
|---|---|---|---|
| OS 支援 | 最少，最簡單 | 要維護 ceiling | 要維護 ceiling ＋ CSC |
| Unbounded inversion | ✅ 解決（T_L 升到 T_H） | ✅ | ✅ |
| Chain blocking | ❌ 可能 | ✅ 解決 | ✅ 解決 |
| 死結 | ❌ 可能 | ✅ 解決 | ✅ 解決 |
| Inheritance-related inversion | 有（只升到等待者） | **很多**（升到 ceiling，太高） | 比 HLP 少（只升到等待者） |
| 優先權何時改變 | 有人等時 | **一拿到資源就變** | 有人等時 |
| 新問題 | — | — | Avoidance-related inversion |

老師最後留了一題（學生作答）：最高優先權的任務當然不會遭受 inversion；那 PCP 下**不需要任何資源**的任務呢？老師的提示：它仍可能遭受 priority inversion，因為某個低優先權任務可能因為依賴與繼承而變成高優先權（也就是 inheritance-related inversion，例如 Ex 3.3 的 T5）。

接下來：從單處理器排程轉向**多處理器**排程。

---

## 9. 一頁速查表

| 項目 | 內容 |
|---|---|
| 三種 inversion | Direct（j 拿著 i 要的）、Inheritance（j 拿著某資源、有比 i 高的任務在等它）、Avoidance（i 要空的資源，但 j 拿著 ceiling ≥ pri(i) 的資源） |
| 表的形狀 | 上三角；不需要資源的任務沒有 direct 和 avoidance，但可能有 inheritance |
| 阻擋時間 | Bᵢ = 三張表第 i 列的最大值（只會遇到一種、一次） |
| 可排程性 | eᵢ + Bᵢ + Σⱼ<ᵢ ⌈pᵢ/pⱼ⌉eⱼ ≤ pᵢ（再加 bssᵢ 若有 self suspension） |
| 前提 | 兩階段：先取得、後釋放；靜態優先權（動態的話 ceiling 要一直更新，太貴） |
| 實測 | 程式算出的 Ex 3.3 表和課本完全一致；Ex 3.2 抓到課本 40 應為 20，且 T1 其實可能遭受 avoidance |

---

## 10. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **單處理器 → 多處理器** | **第 14–15 支**：Real Time Distributed Systems、Scheduling in Multiprocessor Systems |
| **多處理器上的資源共享（PCP 的延伸）** | **第 15 支** |
| **RTOS 實作的協定** | **第 21–25 支**：POSIX、各種 RTOS |

---

## 附：本講一句話總結

> **PCP 下的高優先權任務仍可能遭受 direct、inheritance-related 與 PCP 獨有的 avoidance-related 三種 inversion，可以把每種 inversion 的最長時間列成三張上三角的表（任務只會被比它低的任務反轉，不需要資源的任務沒有 direct 與 avoidance、但仍可能有 inheritance）；因為 PCP 下任務只會遭受其中一種、一次，阻擋時間 Bᵢ 就是三張表第 i 列的最大值，直接加進 Lehoczky 條件成為 eᵢ + Bᵢ + Σ⌈pᵢ/pⱼ⌉eⱼ ≤ pᵢ。動態優先權系統每次優先權改變都得更新 ceiling，開銷太大，所以 PCP 搭配 RMS 類的靜態優先權使用。總結三個協定：PIP 最簡單但會 chain blocking 和死結，HLP 解決兩者卻讓中間任務嚴重受害，PCP 解決全部、inheritance-related inversion 只和 PIP 一樣多，代價是 avoidance inversion。實測寫程式自動產生 inversion 表，課本 Ex 3.3 完全吻合，Ex 3.2 則抓到課本表格把 20 寫成 40、並漏了 T1 的 avoidance inversion。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 14 錄影逐字稿（YouTube 自動字幕，13:12）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch3 §6 整理。實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
