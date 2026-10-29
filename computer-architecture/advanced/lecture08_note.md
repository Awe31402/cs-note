# Advanced Computer Architecture 第八支影片完整筆記：Branch prediction：bimodal、saturating counter、GAg／GAp／PAg／PAp、gshare、tournament、BTB 與 RAS（Chapter 3, Part 2/3）

> **課程**：Advanced Computer Architecture — Chapter 3: The Fetch and Decode Stages: Part II / III (Branch prediction)（播放清單第 8 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（1:20:44，自動字幕）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch3** §3.3–3.4（p.68–83）
> **章號**：影片 Chapter 3 = 課本 Ch3。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 什麼需要預測](#1-什麼需要預測)
- [2. 例子：for 迴圈](#2-例子for-迴圈)
- [3. 1-bit 的 bimodal predictor](#3-1-bit-的-bimodal-predictor)
- [4. 2-bit saturating counter](#4-2-bit-saturating-counter)
- [5. Global history register（GHR）](#5-global-history-registerghr)
- [6. GAp predictor](#6-gap-predictor)
- [7. GAp 還解決不了的兩種情況](#7-gap-還解決不了的兩種情況)
- [8. GAg、PAg、PAp](#8-gagpagpap)
- [9. Gshare](#9-gshare)
- [10. PAp 怎麼解決那兩種情況](#10-pap-怎麼解決那兩種情況)
- [11. Tournament predictor](#11-tournament-predictor)
- [12. 其他做法與理論上限](#12-其他做法與理論上限)
- [13. BTB：預測 target](#13-btb預測-target)
- [14. Return address stack（RAS）](#14-return-address-stackras)
- [15. 整個 fetch 系統](#15-整個-fetch-系統)
- [16. 【課本補充】loop predictor](#16-課本補充loop-predictor)
- [17. 【實測】](#17-實測)
- [18. 一頁速查表](#18-一頁速查表)
- [19. 與後續講次的連結](#19-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

[第 7 支](lecture07_note.md)解了 Problem 1（是不是 branch，用 IST）。這一講解：

| 問題 | 方法 |
|---|---|
| **Problem 2**：taken 還是 not taken？ | 1-bit bimodal → 2-bit saturating counter → 加上 global history（GAp）→ GAg／PAg／PAp 的設計空間 → gshare → tournament |
| **Problem 3**：target 在哪？ | 把 IST 擴充成 **BTB**；call／return 用 **RAS** |

[00:22] 「預測 branch 的結果」是 branch prediction 裡**研究最多、最難**的部分；其他兩個問題比較好處理。

---

## 1. 什麼需要預測

[00:59] 一樣當成黑盒子：PC 進去，taken／not taken 出來。

| Branch 類型 | 要預測方向嗎？ |
|---|---|
| 無條件 branch | 不用，一定 taken |
| call、return | 不用，一定 taken |
| **條件 branch**（beq、bne…） | **要**：看上一個 compare 設的 flags |

[02:23] Not taken = 照順序（PC + 4）；taken = 跳到 branch target。

---

## 2. 例子：for 迴圈

[02:35] 課本 Figure 3.8（p.70），SimpleRisc（Basic [第 7 支](../basic/lecture07_note.md)）：

```c
void foo() {
    for (i = 0; i < 5; i++) { ... }
}
```

```
    mov r1, 0           # i = 0
.loop:
    cmp r1, 5
    beq .exit           # i == 5 就離開
    ...
    add r1, r1, 1
    b .loop
.exit:
    ret
```

[04:19] 唯一的條件 branch 是 `beq .exit`：前 5 次 **not taken**（進迴圈），第 6 次 **taken**（離開）。大部分時間 not taken。

---

## 3. 1-bit 的 bimodal predictor

[05:18] 跟 IST 一樣的結構：PC 的最低 n 個 bit 去查 2ⁿ 個 entry 的表，每格 **1 bit**：**上一次的結果**（0 = not taken、1 = taken），這次就猜一樣。

[06:46] 評估：`beq` 每次呼叫 `foo` 被執行 6 次（5 次真的進迴圈、第 6 次離開）。

| 第幾次 | 1 | 2 | 3 | 4 | 5 | 6 |
|---|---|---|---|---|---|---|
| 真的 | N | N | N | N | N | T |
| 猜（從 N 開始） | N | N | N | N | N | **N ✗** |
| 再呼叫一次 foo，猜 | **T ✗** | N | N | N | N | **N ✗** |

[08:51] 一直呼叫 `foo`：每 6 次錯 2 次（**進迴圈錯一次、出迴圈錯一次**），猜錯率 **1/3**，準確率只有 **66.7%**。

[09:06] ⚠️ 老師說第 2 章要求準確率「96、97、98% 以上」；[第 5 支](lecture05_note.md)說 98–99%、課本說 99% 以上。總之 66.7% 差太遠了。

[09:32] 內建的缺陷：**永遠猜上一次的結果**。這個 branch 本質上偏向 not taken，迴圈結尾那一次例外不應該改變我們對它的看法。

---

## 4. 2-bit saturating counter

[10:45] 加一點**歷史**、一點**遲滯（hysteresis）**：每格改成 **2-bit 的 saturating counter**：

- taken 就 +1，到 **11** 就不再加（**saturate**，飽和）。
- not taken 就 −1，到 **00** 就不再減。
- [16:18] 看 **MSB**：0（00、01）猜 not taken，1（10、11）猜 taken。

| 狀態 | 名稱 | 猜 |
|---|---|---|
| 00 | strongly not taken | N |
| 01 | weakly not taken | N |
| 10 | weakly taken | T |
| 11 | strongly taken | T |

[13:41] 偏向 not taken 的 branch 大多停在 00；偶爾一次 taken 只會到 01，還是猜 not taken。要**連續兩次** taken 才會改變預測。

[17:02] 同一個迴圈：

| 第幾次 | 1 | 2 | 3 | 4 | 5 | 6 |
|---|---|---|---|---|---|---|
| 真的 | N | N | N | N | N | T |
| 狀態（進來前） | 01 | 00 | 00 | 00 | 00 | 00 |
| 猜 | N | N | N | N | N | **N ✗** |

離開時狀態變 01，下次呼叫 `foo` 第一次還是猜 N，**對了**。猜錯率從 2/6 降到 **1/6**（約 16%），「減半」。

[20:10] 為什麼有用：它記下了 branch 的**本性**（inherent behavior）。適合「**大部分走同一個方向、偶爾例外**」的 branch。

【課本補充】課本 p.71：saturating counter 的定義：C ← min(C + 1, U)、C ← max(C − 1, L)。課本 p.72：也可以用 3、4 bit 的 counter，但表會變大，而且對行為改變的反應變慢（從 0 要很多次才翻過來）；架構師會跑大量模擬找最好的設定。

---

## 5. Global history register（GHR）

[21:15] 再複雜一點的程式（課本 p.74）：迴圈最後一圈呼叫 `foobar`：

```c
for (i = 0; i < 5; i++) {
    if (i == 4) foobar();
}
```

```
.loop:
    cmp r1, 5
    beq .exit           # ← 我們關心的 branch
    cmp r1, 4
    bne .cont           # i != 4 就跳過（大部分 taken）
    call .foobar
.cont:
    add r1, r1, 1
    b .loop
```

[22:44] 目標：把 `beq .exit` 的猜錯率降到 **0%**。

[23:00] **Global history register（GHR）**：一個 **shift register**，記住**最近 n 個 branch** 的結果（**不管 PC**，動態指令序列裡最近的就算）；taken = 1、not taken = 0。

[24:35] 假設 GHR 只有 2 bit，而且只記**條件** branch（比較準）。在每一圈開頭看 GHR（上一圈的兩個條件 branch）：

| 哪一圈的開頭 | 上一圈的 beq、bne | GHR |
|---|---|---|
| 第 2–5 圈 | N、T | **01** |
| 第 6 次（要離開） | N、**N**（i == 4，bne 不跳） | **00** |

[26:10] GHR 等於把一個 branch 的**上下文（context）**編碼成前面 n 個 branch 的結果。第 6 次的 00 跟其他時候都不一樣，**獨一無二**，可以用來預測。

⚠️ 影片用 **2-bit 的 GHR**、離開時是 **00**；課本 p.74 用 **3-bit 的 BHR**（branch history register，就是 GHR），離開時是 **001**。名字、長度不同，道理一樣。

---

## 6. GAp predictor

[27:39] 把 PC 的 n 個 bit 和 GHR 的 k 個 bit **接起來**（concatenate），變成 n + k bit，去查 **2ⁿ⁺ᵏ** 個 saturating counter 的表：

```
[ PC 的 n bit | GHR 的 k bit ] ──→ 2ⁿ⁺ᵏ 個 2-bit counter ──→ T／N
```

- [28:35] 同一個 PC，不同的 GHR 值有不同的 counter。2-bit 的 GHR 就是每個 PC 有 4 個 counter。
- [29:15] 最常見的兩個值：01（訓練成 not taken）、00（只在離開時出現，訓練成 **taken**）。
- [30:23] 代價：表大了 2ᵏ 倍。好處：同時用了**全域歷史**（global history）和**每個 PC 自己的資訊**。

[32:18] ⚠️ 還有一個小問題：**第一圈**。進 `foo` 時 GHR 是呼叫它之前的兩個 branch 決定的，不在我們控制內。如果剛好是 00，就會猜 taken，錯了。老師說機率上不常發生（4 種狀態只有一種），之後再回來處理。

[38:40] 為什麼叫 **GAp**：**G** = 第一層是 global（GHR 不用 PC 選）；**p** = 第二層 per-address（表用 PC 的 bit）。第二層的表叫 **PHT**（pattern history table）。也可以在 entry 裡存一部分 tag 減少 aliasing（跟 IST 一樣）。

【課本補充】課本 §3.3.5（p.74–75）把這種叫 **2-level predictor**：第一層讀 global history，第二層用 history ＋ PC 的 bit 查 saturating counter 的表（PHT）。

---

## 7. GAp 還解決不了的兩種情況

[35:11]

1. **交錯的樣式** T、N、T、N……（例如程式裡有亂數時）。saturating counter 是設計來抓「穩定行為」的，抓不到交錯。GAp 抓不抓得到，要看 GHR 對 T 和 N 有沒有**獨特的簽名**；GHR 是最近 k 個 branch，可以是任何 PC 的，所以不一定有。
2. [37:15] **第一圈的上下文**：從函式外面帶進來，不在我們控制內（§6 的問題）。

---

## 8. GAg、PAg、PAp

[38:14] 建立一個 predictor 的**設計空間**（課本 §3.3.6，p.75–78，來自 Yeh 和 Patt，1992）：每一層可以是 **G**（global，不用 PC 選）或 **P**（per-address，用 PC 的 bit）。

| 名稱 | 第一層（history） | 第二層（PHT 的 index） | 表大小 |
|---|---|---|---|
| **GAg** | 一個 GHR | 只用 GHR | 2ᵏ |
| **GAp** | 一個 GHR | GHR ＋ PC | 2ⁿ⁺ᵏ |
| **PAg** | **很多個 GHR**，用 PC 的 n₁ 個 bit 選 | 只用選出來的 GHR | 2ᵏ |
| **PAp** | 很多個 GHR，用 PC 選 | GHR ＋ PC 的 n 個 bit | 2ⁿ⁺ᵏ |

### 8.1 GAg

[40:03] 最簡單，完全不用 PC。「不實用、沒人用」，但它是設計空間的正式成員。沒有任何 per-branch 的資訊，當然不好。

### 8.2 PAg：很多個 GHR

[40:56] 從 PC 取 n₁ 個 bit（例如 3 個 → 8 個 GHR）去選一個 GHR，再用它查 PHT。

[42:23] 為什麼要多個 GHR？【口頭】大程式會在一組函式附近待一陣子：例如 PowerPoint 啟動時跑初始化的程式碼、做投影片時跑另一段、放投影片時又是另一段。這些程式碼在位址上**擠在一起**，所以：

- [43:34] **每一塊程式碼區域給一個自己的 GHR**，那一區的上下文比較相關，不會被別區的 branch 污染。
- [43:49] **不要用最低的 bit**選 GHR：n₁ = 8 的話，就是「最低 8 bit 相同的指令共用一個 GHR」，這些指令彼此通常沒關係。
- **也不要用最高的 bit**：可能一直不變，等於只有一個 GHR。
- 用**中間的 bit**：哪幾個 bit 要靠模擬決定。老師的例子：offset 4 個 bit，就是 16 byte 的區域共用一個 GHR。

[45:45] GHR 的哲學：抓**相關的 branch** 的行為。什麼叫相關？同一個位址範圍裡的 branch，可能屬於同一個函式或同一組函式。

⚠️ [47:29] 老師說「PAg predictor 結合了所有我們看過的東西」，接著描述的（直接用 PC 的 n 個 bit ＋ 多個 GHR）其實是 **PAp**；[48:24] 他說「PAg 是四個裡面最大、最有表達力的」，也是指 PAp。照課本：PAg 的 PHT 只用 GHR 當 index，PAp 才同時用 PC。

【課本補充】課本 p.77：取 bit 11、12、13（從 1 開始數）選 BHR，就是每 1 KB 的程式碼一個 BHR。因為 spatial locality，一段時間只會碰一小塊（例如 5 KB），分到 5 個 BHR，不會互相干擾；換到別的程式碼區時，BHR 會被重新填。

### 8.3 PAp

[47:41] 直接用 PC 的 n 個 bit，再加上（用 PC 中間的 bit 選出來的）GHR 的 k 個 bit，接起來查 2ⁿ⁺ᵏ 的 PHT。「最大、最有表達力」。

【課本補充】課本 p.78：PAp 不一定最好：表很大、很慢、耗電，而且**訓練要很久**。1024 entry 的 1-bit bimodal 只要 1024 次不同的存取就訓練好；2-bit counter 每格最多 3 次；有 BHR 的話每種歷史樣式都要訓練，要更多次。程式碼小、或在同一區待很久，PAp 才划算。

---

## 9. Gshare

[48:41] 另一種結合 PC 和 GHR 的方法：不是接起來，而是 **XOR**：

```
PC 的 n bit ⊕ GHR 的 k bit ──→ 2^max(n,k) 個 counter
```

- [49:09] 是 GAp 的延伸：XOR 也能把兩種資訊「攪在一起」，但用**少很多**的空間。
- 表的大小是 2^max(n, k)，不是 2ⁿ⁺ᵏ。

【課本補充】課本 p.79：XOR 之後**很可能**得到獨特的組合，還是可能 aliasing；但 n、k 夠大、程式又有 locality 時，一小段時間內 XOR 的結果大多能唯一代表那對（PC、歷史）。表比較小、比較快、比較省電。

---

## 10. PAp 怎麼解決那兩種情況

### 10.1 第一圈

[50:13] 問題：進 `foo` 前的 branch 決定了 GHR，可能來自 `foobar`、`foobarbar` 這些不同的呼叫者，上下文不一樣，甚至可能剛好跟「離開迴圈」時一樣是 00，造成 aliasing。

[52:31] PAp 有**多個 GHR**：

- `foobar` 裡的 branch 用一個 GHR、`foobarbar` 的用另一個、`foo` 的用 **GHR3**（位址都在同一區）。
- [54:04] 預測 `beq .exit` 時用 GHR3：裡面是**上次 `foo` 執行時**的 branch 結果，不受其他區域影響。
- [54:48] 上次 `foo` 最後兩個 branch：bne（not taken）、beq（taken）→ **01**；01 一律對應 not taken，所以第一圈猜對。

[56:20] 給每一區程式碼一個 GHR，不會被別區的 branch 污染；離開 `foo`、做別的事，再回來時用的還是這一區的 GHR。

### 10.2 交錯的樣式

[57:00] T、N、T、N……，中間可能夾著任意的程式碼。要的是「**這個 branch 自己**上一次的結果」：上次 1 就猜 0、上次 0 就猜 1。

- [57:51] saturating counter 做不到：它是給偏向一邊的 branch 用的，這個是 50/50 交錯。
- [58:17] 需要歷史，而且是**只屬於這個 PC** 的歷史，不能被別人污染 → 多個 GHR。
- [58:29] 用 PC 的 n 個 bit ＋ **1 bit 的 GHR**（上一次的結果）：entry ⟨n, 0⟩ 永遠是 1、⟨n, 1⟩ 永遠是 0。counter 飽和之後，PAp 就完美學會交錯。

⚠️ [60:25] 老師先說「交錯的樣式要用 GAp」，馬上更正：「我收回，應該是 **PAp**」。

[59:51] 總結：多個 GHR 避免**破壞性干擾**。偏向一邊的 branch 用 saturating counter 最好；依賴上下文的（例如交錯）用 PAp 這類。

（本機模擬：每個 predictor 在這幾個例子上的猜錯率，見 [§17.1](#171-照影片的例子模擬-predictor)。）

---

## 11. Tournament predictor

[60:41] 不同的程式區域適合不同的 predictor：偏向一邊的區域用 saturating counter 最好（PAp 也行，但越大的 predictor 訓練越久、資源越多）；別的區域可能 GAp 或 gshare 最好。所以：**每個 PC 自己選要用哪個 predictor**。

[61:53] **Tournament predictor**：

- 一個 **choice array**（課本叫 selector array）：PC 的 n 個 bit 去查，每格是一個 2-bit saturating counter。
- counter 是 00、01 → 用 predictor 1；10、11 → 用 predictor 2。
- 兩個 predictor **都跑**，選中的那個給最後的預測。

[64:31] 訓練：

| 情況 | choice array |
|---|---|
| 兩個 predictor 猜一樣 | 不動 |
| 不一樣、predictor 1 對 | −1（往 predictor 1 那邊） |
| 不一樣、predictor 2 對 | +1 |

兩個 predictor 本身都照真的結果訓練。

[65:28] 哪個方向加、哪個方向減無所謂，只要一致。對某個 PC，counter 會飽和在比較準的那一邊；行為改變了，counter 也會跟著改。

⚠️ [62:57] 老師先說「pattern table」，馬上改口叫 **choice array**：pattern table 是 predictor 內部的表。

【課本補充】課本 p.81：tournament predictor 在功耗、延遲、面積上有代價，但在課本介紹過的所有 predictor 裡**最準**，所以大多數情況下是首選。商用晶片的 predictor 用更多表、更多 bit 的組合，以及多層的 predictor。

---

## 12. 其他做法與理論上限

[65:45] 其他方法：

- 加一點 **tag bit** 減少 aliasing。
- 不同的 branch 子集用不同的 predictor：可以動態決定（像 tournament），也可以靜態由 **compiler** 決定。
- 每個 branch 加一個 **bias bit**（compiler 告訴硬體它最可能的方向），predictor 只要預測「同不同意 bias bit」。
- 把 branch 分成**高信心**和**低信心**：高信心的用少 bit 的 predictor、低信心的用多 bit 的（bit 的預算有限，晶片面積有限）。
- [67:10] 老師點名：**bi-mode、agree、skew、YAGS**，以及「stage」（⚠️ 字幕這樣寫，可能是某個 predictor 的名字被聽錯，待查），說它是非常準的 predictor；還有受早期神經網路啟發的 **perceptron** predictor，準確率極高。
- [67:48] **Skewed predictor**：用不同的函式把位址 bit 和 global history「攪成」幾種組合，每種一個 predictor，取**多數決**（三個裡兩個說 not taken，就猜 not taken）。gshare 概念上的延伸。

⚠️ [68:00] 老師舉多數決的例子時口誤：「兩個說 not taken、一個說 taken，我們猜 taken」，應該是猜 **not taken**。

[68:29] **有沒有理論分析？** 有。【口頭】資訊理論裡，預測和**壓縮**很相關：

- 把（PC, 結果）的序列存成檔案，用 zip 或 RAR 壓縮。例如 100 KB 壓成 20 KB，壓縮比 5。
- **Fano's inequality**：一個 branch trace 有一個**理論上最高的預測準確率**，跟序列的可壓縮性（**entropy**，熵）有關。
- [70:38] 不管怎麼做，序列裡就是沒有足夠的資訊讓你猜得更準。

【課本補充】課本 p.68：branch 如果真的完全隨機（像丟銅板），不管用什麼方法都不可能超過 50%，這也可以用 Fano's inequality 證明。但 branch 通常**不是**隨機的，所以可以做到 90–95% 以上。

---

## 13. BTB：預測 target

[71:07] 預測 target 很**簡單**：把第 7 支的 IST **擴充**，每格除了類型，再存 **target**（也可以存 tag）。這叫 **BTB**（branch target buffer）。

```
PC 的最低 n bit ──→ 2ⁿ 個 entry：[ tag | 類型 | target ]
```

[72:12] BTB 一次解決兩個問題：**Problem 1**（是不是 branch、什麼類型）和 **Problem 3**（target）。看到 branch 就把它的 target 記下來。

[72:47] 無條件 branch 不用猜方向，但**要猜 target**，所以 BTB 很重要。

[73:00] 【口頭】現代處理器有**兩層 BTB**：L1 BTB 小而快，存最常出現的 PC；L2 BTB 比較慢，存比較少出現的。

【課本補充】課本 p.81–82：大多數 ISA 的 target 是寫死在指令裡、相對於 PC 的固定 offset；有些 ISA 支援 **indirect branch**（target 放在暫存器裡），這種比較難預測，課本這裡不討論。

---

## 14. Return address stack（RAS）

[73:36] 無條件 branch 只要找 target。**call 和 return 很有趣**：

```
foo → foobar → foobarbar → foobarbarbar
         然後一路 return 回來
```

[74:25] Call 的 target 可以用 BTB（call 也是無條件 branch）。Return 也用 BTB 嗎？有**更準**的方法。

[74:49] 觀察：這是 **後進先出（LIFO）**：foobarbarbar 最後進、最先出。看到 LIFO 就用 **stack**：

- [75:15] 每次 **call**：把 **return address** push 進 stack。
- 每次 **return**：pop 出來，最上面那個就是要回去的地方，送給 pipeline。

[75:48] 例子：`foo` 呼叫 `foobar`，return address 0xFC0 進 stack；`foobar` 呼叫 `foobarbar`，0xFF4 進 stack；再呼叫一層再 push；return 時一個一個 pop。中間又 call 別的函式也沒問題，再 push 就好。

[77:37] BTB 對 return「不太準」（同一個 return 指令可能回到不同地方）；硬體裡只要一個小 stack，就準得多。這叫 **return address stack（RAS）**。

【課本補充】課本 p.82–83：拿到 PC、預測它是 return，就用 RAS 最上面的當 target；decode 後確定真的是 return，才 pop。RAS 比 BTB 準，因為沒有破壞性干擾。限制：**RAS 的大小有限**，很深的遞迴可能不夠；研究者提出了處理遞迴的修改，也可以在溢位時把一部分 RAS 搬到記憶體。

（本機實測：P-core 的 RAS 大約 14 層，見 [§17.3](#173-return-address-stack-有幾層)。）

---

## 15. 整個 fetch 系統

[78:22] 總結（三個模組一起決定下一個 PC）：

| 模組 | 給誰用 | 做什麼 |
|---|---|---|
| **BTB** | 每個 PC；也給無條件 branch、call | 類型 ＋ target |
| **RAS** | return | return address |
| **Branch predictor**（方向） | 條件 branch | taken／not taken；最難，我們看了很多設計 |

[80:02] 實際上還要擴充：**同時做好幾個 branch prediction**，一個 cycle 就選出下 2、3、4 個 PC（[第 7 支](lecture07_note.md)的 multi-way branch）。

[80:16] 下一講：decode stage，比較短。

---

## 16. 【課本補充】loop predictor

影片沒講，課本 §3.3.3（p.72–73）：如果 compile 時就知道迴圈跑幾次（例如 100 次），可以做一個 **loop predictor**：一個暫存器加一個小加法器，進迴圈時（compiler 加一條指令）設成 100，每一圈減 1，到 0 就預測離開。硬體也可以自己認出迴圈：迴圈大多用**往回跳**的 branch（offset 是負的，跟 [第 6 支](lecture06_note.md) 一樣的觀察）。

缺點：不通用。大多數迴圈的次數 compile 時不知道，還有 break、巢狀迴圈、迴圈裡呼叫有迴圈的函式。而且要 compiler 配合的方法，程式就綁死在支援那些特殊指令的處理器上。

---

## 17. 【實測】

程式在 `experiments/lecture08/`。

### 17.1 照影片的例子模擬 predictor

`bpsim.py` 實作了本講所有的 predictor（1-bit bimodal、2-bit saturating counter、GAg、GAp、PAg、PAp、gshare、tournament），GHR 都是 2 bit、表的 index 用 PC 的 10 bit、PAg／PAp 用 PC 的 bit 4–6 選 GHR。拿影片的四個例子跑（每個例子重複很多次，算穩定後的猜錯率）：

| Predictor | for 迴圈（Fig 3.8） | 加上 if (i == 4) | 再加上呼叫前 2 個亂數 branch | T、N 交錯，中間夾亂數 branch |
|---|---|---|---|---|
| 1-bit bimodal | **0.333** | 0.333 | 0.333 | **1.000** |
| 2-bit saturating | **0.167** | 0.167 | 0.167 | 0.500 |
| GAg | 0.167 | 0.000 | 0.083 | 0.054 |
| GAp | 0.167 | **0.000** | 0.042 | 0.001 |
| PAg | 0.167 | 0.000 | 0.011 | 0.001 |
| PAp | 0.167 | 0.000 | **0.000** | **0.000** |
| gshare | 0.167 | 0.000 | 0.042 | 0.001 |
| tournament（2-bit ＋ PAp） | 0.167 | 0.001 | 0.001 | 0.001 |

- **1/3 和 1/6**：跟影片、課本（p.70、p.72）完全一樣。
- **加上 `if (i == 4)`**：有 GHR 的 predictor 全部變成 **0**。`bne` 提供了「快要離開」的上下文，就是 [§5](#5-global-history-registerghr) 說的。
- **呼叫 `foo` 前先跑兩個亂數 branch**：GAp、gshare 還有約 4% 錯（第一圈的 GHR 被外面的亂數 branch 污染，就是 [§7](#7-gap-還解決不了的兩種情況) 的第二種情況）；**PAp 是 0**：`foo` 這一區有自己的 GHR，外面的亂數 branch 碰不到它（[§10.1](#101-第一圈)）。
- **T、N 交錯、中間夾一個別處的亂數 branch**：1-bit bimodal 每次都錯（**100%**，永遠猜「上一次」，剛好每次都相反）；2-bit counter 50%；只有一個 GHR 的 GAg 5%（GHR 被亂數 branch 攪亂）；**PAp 0%**（[§10.2](#102-交錯的樣式)）。
- 寫這個模擬時我踩到 **aliasing**：一開始把亂數 branch 放在 PC 0x900，最低 10 bit 跟迴圈的 0x100 一樣，兩個 branch 擠在同一格，連 2-bit counter 的結果都被弄亂了。換到 0x5f0、0x6f0 才正常。這正是第 7 支 [§9](lecture07_note.md#9-aliasing-與-tag) 的 destructive interference。

### 17.2 真的 predictor 能背多長的樣式

`pattern.c`：一個 branch 照著長度 L 的**亂數** T／N 樣式一直重複（`-fno-if-conversion` 確保真的是 branch，不是 cmov）。L 小時 predictor 能把整個樣式背起來；L 大到記不住，就跟真的亂數一樣，大約一半猜錯：

| L | P-core（cpu2） | E-core（cpu14） |
|---|---|---|
| 2–8 | 0.44–0.57 ns | 1.19–1.39 ns |
| 256 | 0.73 ns | **0.86 ns** |
| 512 | 0.73 ns | 1.18 ns |
| 1024 | 0.76 ns | 2.29 ns |
| 4096 | **0.77 ns** | 3.67 ns |
| 8192 | 1.08 ns | 3.94 ns |
| 16384 | 2.49 ns | 4.05 ns |
| 32768 以上 | 3.0–3.4 ns | 4.1–4.2 ns |

- **P-core 能背住約 4096 步的亂數樣式**，到 8192 才開始變慢、32768 以上就跟亂數一樣。**E-core** 大約 256–512 步。課本裡 2 bit、3 bit 的 GHR 跟這個比是天壤之別：真的 predictor 用的歷史很長（實際做法 Intel 沒有公開，**待補**）。
- 猜錯的代價：P-core 背不住時多了約 2.7 ns，一半的 branch 猜錯 → 每次猜錯約 **5.4 ns ≈ 25 個 cycle**（4.7 GHz）。跟 Basic [第 47 支 §17.3](../basic/lecture47_note.md#173-speculation猜錯要付多少) 的估計（約 20 個 cycle）差不多。
- 這也回答了第 5 支 [§13.3](lecture05_note.md#133-pn-的數字) 留下的問題：真的 predictor 比模型假設的「每個 branch 獨立、99.5% 準」強很多，所以本機的 OOO 視窗能撐到 500 條。

### 17.3 Return address stack 有幾層

`ras.c`：遞迴深度 D（每一層一個 call、一個 ret），重複很多次，算每一對 call＋ret 的平均時間。D 超過 RAS 能存的層數，最外面幾層的 return address 就被擠掉，那些 ret 會猜錯：

| 深度 D | P-core（cpu2） | E-core（cpu14） |
|---|---|---|
| 2–13 | **0.34–0.36 ns** | 1.15–1.74 ns |
| 14–20 | **0.78–0.79 ns** | 1.15–1.20 ns |
| 32–256 | 0.90–1.14 ns | 1.14–1.38 ns |

- **P-core**：深度 13 還很快，**14 突然變兩倍多**。每次 `rec(D)` 實際有 D + 1 層，所以 P-core 的 RAS 大約能存 **14 層**；更深的 return 就要靠別的方法（慢很多）。這正是課本 p.83 說的「RAS 的大小有限，很深的遞迴會不夠」。
- ⚠️ **E-core** 到 256 層都沒有明顯的跳躍，看不出 RAS 的大小，可能它的 RAS 很大，或者處理 return 的方式不同，**待補**。

---

## 18. 一頁速查表

| 主題 | 重點 |
|---|---|
| 要預測方向的 | 只有條件 branch |
| 1-bit bimodal | 猜上一次；for 迴圈錯 1/3（進、出各錯一次） |
| 2-bit saturating | 00／01 猜 N、10／11 猜 T；錯 1/6；適合偏一邊、偶爾例外 |
| GHR | 最近 k 個 branch 的結果（不管 PC）；當上下文 |
| GAp | PC ⧺ GHR → 2ⁿ⁺ᵏ 的 PHT；`if (i == 4)` 的例子錯 0 |
| G／P | 第一層：一個 GHR／用 PC 選 GHR；第二層：PHT 用不用 PC |
| GAg | 只用 GHR，沒人用 |
| PAg | 很多 GHR（用 PC 中間的 bit 選，一區一個）；PHT 只用 GHR |
| PAp | 很多 GHR ＋ PC；最大、最有表達力、訓練最久 |
| Gshare | PC ⊕ GHR → 2^max(n,k)；省空間 |
| PAp 解決 | 第一圈的上下文（GHR 不被外面污染）、交錯樣式（1 bit 的自己的歷史） |
| Tournament | choice array（2-bit counter）選 predictor；猜法不同時才更新；最準 |
| 其他 | tag、bias bit、高／低信心、bi-mode、agree、skew、YAGS、perceptron |
| 理論上限 | Fano's inequality；跟可壓縮性（entropy）有關；隨機 branch 最多 50% |
| BTB | IST ＋ target；解 Problem 1 和 3；L1／L2 兩層 |
| RAS | call push、return pop；比 BTB 準；大小有限 |
| 實測 | 模擬重現 1/3、1/6、0；PAp 在受干擾時仍是 0；P-core 背得住 4096 步的亂數樣式、猜錯約 25 cycle；P-core RAS 約 14 層 |

---

## 19. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| IST、aliasing、tag、三個問題 | [第 7 支](lecture07_note.md) |
| 為什麼要 98–99% 以上準 | [第 5 支 §12](lecture05_note.md#12-要填滿-windowbranch-一定要猜得很準) |
| 猜錯時怎麼復原（precise exception、commit） | [第 6 支](lecture06_note.md)、[第 13 支](lecture13_note.md) |
| Decode 時的最佳化 | [第 9 支](lecture09_note.md) |
| 更激進的 speculation | [第 14 支](lecture14_note.md)（Ch5） |
| In-order pipeline 的 predict not taken | [Basic 第 31 支 §11](../basic/lecture31_note.md#11-branch-lockpredict-not-taken) |
| Basic 課程的 branch 猜錯實測 | [Basic 第 47 支 §17.3](../basic/lecture47_note.md#173-speculation猜錯要付多少) |
| Spectre 這類利用 branch predictor 的攻擊 | [第 54 支](lecture54_note.md)（Ch13 side channel） |

---

## 附：本講一句話總結

> **預測條件 branch 的方向最難：只記上一次結果的 1-bit bimodal predictor 在 for 迴圈上進出各錯一次（錯 1/3），改成 2-bit saturating counter（00、01 猜 not taken，10、11 猜 taken，偶爾一次例外不會翻）錯 1/6；再加上記住最近幾個 branch 結果的 GHR 當上下文，把 PC 和 GHR 接起來查 PHT 的 GAp 就能預測迴圈的最後一圈，依兩層是 global 還是 per-address 又有 GAg、PAg、PAp（每個程式區域一個 GHR，能解第一圈和交錯樣式），用 XOR 代替接起來的 gshare 比較省空間，用 choice array 替每個 PC 挑 predictor 的 tournament 最準，而 Fano's inequality 告訴我們任何 predictor 都有資訊理論上的上限；target 則把 IST 擴充成存了 target 的 BTB，return 用 call push、return pop 的 return address stack。本機模擬重現了課本的 1/3、1/6、0，PAp 在有干擾時仍然 0 錯；真的 P-core 能背住長達 4096 步的亂數樣式、猜錯一次約 25 個 cycle，RAS 大約 14 層。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 3: The Fetch and Decode Stages: Part II / III (Branch prediction) 錄影逐字稿（YouTube 自動字幕，1:20:44）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch3 §3.3–3.4（p.68–83）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
