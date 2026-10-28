# Basic Computer Architecture 第二十一支影片完整筆記：加法器：ripple carry、carry select、carry lookahead（Chapter 7 Computer Arithmetic, Part 1/6 → 課本 Ch8）

> **課程**：Basic Computer Architecture — Chapt 7: Computer Arithmetic, Part 1/6（播放清單第 21 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（1:29:40，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 **Ch8** §8.1（p.305–318）
> **章號**：影片的 Chapter 7 是課本 v3.09 的 **Chapter 8**（Computer Arithmetic）。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 兩個 1 位元數相加：half adder](#1-兩個-1-位元數相加half-adder)
- [2. 三個 1 位元數相加：full adder](#2-三個-1-位元數相加full-adder)
- [3. 兩個 n 位元數相加](#3-兩個-n-位元數相加)
- [4. Ripple carry adder](#4-ripple-carry-adder)
- [5. 漸近時間複雜度與 big-O](#5-漸近時間複雜度與-big-o)
- [6. Carry select adder：O(√n)](#6-carry-select-adderon)
- [7. Carry lookahead adder 的核心：generate 與 propagate](#7-carry-lookahead-adder-的核心generate-與-propagate)
- [8. 多位元的 G 和 P](#8-多位元的-g-和-p)
- [9. 把兩塊的 G、P 合併](#9-把兩塊的-gp-合併)
- [10. Carry lookahead adder 的 Stage I：O(log n)](#10-carry-lookahead-adder-的-stage-iolog-n)
- [11. 【課本補充】Stage II](#11-課本補充stage-ii)
- [12. 【實測】](#12-實測)
- [13. 一頁速查表](#13-一頁速查表)
- [14. 與後續講次的連結](#14-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

計算機算術這章用前兩章的知識（Ch2 位元的語言、數位邏輯）設計**加法器、乘法器、除法器**。分六部分（[01:01]）：

1. **整數加法** ← 本講
2. 整數乘法
3. 整數除法
4. 浮點數加減法
5. 浮點數乘法
6. 浮點數除法

本講的主線：**怎麼讓加法變快**。

| 加法器 | 時間 | 核心想法 |
|---|---|---|
| Ripple carry | **O(n)** | carry 一位一位往左傳 |
| Carry select | **O(√n)** | 分塊；每塊**同時**算 carry in = 0 和 = 1 的結果，再選 |
| Carry lookahead | **O(log n)** | 不做加法，直接用 generate／propagate **快速算出 carry** |

本講的影片講到 carry lookahead 的 **Stage I** 為止，Stage II 在下一支（第 22 支）。

---

## 1. 兩個 1 位元數相加：half adder

[01:15]–[08:01]　課本 §8.1.1（p.305–306）

- 兩個 1 位元數相加，最大是 1 + 1 = 2 = `10`，需要 **2 個位元**。

| a | b | carry | sum |
|---|---|---|---|
| 0 | 0 | 0 | 0 |
| 0 | 1 | 0 | 1 |
| 1 | 0 | 0 | 1 |
| 1 | 1 | **1** | **0** |

- 結果的 **LSB 叫 sum**，**MSB 叫 carry**（進位）。
- 二進位加法和十進位加法**完全一樣**：從右邊開始，放不下就進位。
  - 【口頭】例：`1011` + `0010` = 11 + 2 = 13 = `1101`。十進位的例子：個位 2 + 9 = 11，寫 1 進 1。

**公式**：

```
sum   = a ⊕ b = a̅·b + a·b̅
carry = a · b
```

- 【口頭】⊕ 是 XOR；+ 是 OR；· 是 AND。
- **Half adder（半加器）**：兩個 AND、一個 OR 算 sum（a̅b 和 ab̅ 再 OR），一個 AND 算 carry。

---

## 2. 三個 1 位元數相加：full adder

[08:01]–[18:18]　課本 §8.1.2、Definition 54（p.306–308）

**為什麼要加三個位元**：兩個多位元數相加時，除了最低位，其他每一位都要加 **a、b，還有右邊進來的 carry**。

- 三個 1 位元數最大是 1 + 1 + 1 = 3 = `11`，還是 **2 個位元**就夠。
- 輸入：**a、b、C_in**（carry in）；輸出：**s**（sum）和 **C_out**（carry out）。
- 結果 = 2 × C_out + s。

| a | b | C_in | C_out | s |
|---|---|---|---|---|
| 0 | 0 | 0 | 0 | 0 |
| 0 | 0 | 1 | 0 | 1 |
| 0 | 1 | 0 | 0 | 1 |
| 0 | 1 | 1 | 1 | 0 |
| 1 | 0 | 0 | 0 | 1 |
| 1 | 0 | 1 | 1 | 0 |
| 1 | 1 | 0 | 1 | 0 |
| 1 | 1 | 1 | 1 | 1 |

**sum**：三個裡面**奇數個**是 1 時才是 1。

```
s = a ⊕ b ⊕ C_in
  = a·b̅·C̅_in + a̅·b·C̅_in + a̅·b̅·C_in + a·b·C_in
```

- 1 ⊕ 1 ⊕ 1：先 1 ⊕ 1 = 0，再 0 ⊕ 1 = 1。
- XOR 滿足結合律，可以一路展開。

**carry out**：三個裡面**至少兩個**是 1 時才是 1。

```
C_out = a·b + a·C_in + b·C_in
```

**電路**：

- C_out：三個 AND 閘（a·b、a·C_in、b·C_in）＋ 一個 OR 閘。
- sum：四個 AND 閘（四個乘積項）＋ 一個 **4 輸入的 OR 閘**。
- 【口頭】算 sum 比算 carry out 需要更多硬體。
- 【課本補充】Definition 54：能加 3 個位元的加法器叫 **full adder（全加器）**。

---

## 3. 兩個 n 位元數相加

[18:18]–[20:32]

例：

```
   1 1 1 1
 + 0 0 0 1
 ---------
 1 0 0 0 0
```

- 從 LSB 開始，加對應的兩個位元和 carry in，產生 sum 和 carry out。
- **第 i 位的 carry out 就是第 i + 1 位的 carry in**。
- 最後一步如果還有 carry，它就變成結果的 MSB。
- carry 像**漣漪**（ripple）一樣，從右往左一位一位傳過去。

---

## 4. Ripple carry adder

[20:32]–[23:47]　課本 §8.1.3（p.309）

```
     a_n b_n        a_3 b_3   a_2 b_2   a_1 b_1
       │ │            │ │       │ │       │ │
C_out ─[FA]─ … ──────[FA]────[FA]────[HA]
       │              │        │        │
      s_n            s_3      s_2      s_1
```

- 最低位**沒有 carry in**，所以用 **half adder**；其他都用 **full adder**。
- carry 從一個加法器傳到下一個，像一道從右往左的波。
- **時間**：設 half adder 要 t_h、full adder 要 t_f。它們是**串聯**的，所以：

```
f(n) = t_h + (n − 1) · t_f
```

---

## 5. 漸近時間複雜度與 big-O

[23:47]–[35:56]　課本 Definition 55、Example 121–122（p.310–312）

**想法**：大多時候我們只在乎函數的**階**（order）。

- 例：2n² + 3n + 4。n 很大時，**2n² 主導**整個式子，其他項可以忽略；連常數 2 也可以忽略，說它「像 n² 一樣成長」。
- 【口頭】「這在數學上不精確，但我們會看到它有好處。」

> **Definition 55（big-O）**：如果存在正常數 c 和 n₀，使得對所有 n > n₀，|f(n)| ≤ c·|g(n)|，就說 **f(n) = O(g(n))**。

- 白話：超過某個 n 之後，g(n) 乘上某個常數就**蓋得住** f(n)。g(n) 是 f(n) 的一個**上界**（envelope，包絡線），忽略加法和乘法的常數。
- 例：2n² + 3n + 4 ≤ 10n²（n > 100）→ c = 10、n₀ = 100 → O(n²)。

**Example 121**：f(n) = 3n² + 2n + 3

```
f(n) = 3n² + 2n + 3 ≤ 3n² + 2n² + 3n² = 8n²   （n > 1）
```

所以 f(n) = **O(n²)**，c = 8、n₀ = 1。畫出來，n > 1 之後 8n²（綠線）永遠在 f(n)（紅線）上面。

**Example 122**：f(n) = 0.00001n¹⁰⁰ + 10000n⁹⁹ + 234344

- 雖然第一項的係數很小、第二項很大，但 n 夠大時，n¹⁰⁰ 一定會主導。
- f(n) = **O(n¹⁰⁰)**。
- 簡單的規則：**多項式就取最高次方**。
- ⚠️ 影片口頭說係數是「0.1」，課本是 0.00001；結論一樣。

**學生常問的問題**（[32:20]）：

- 例：0.1n² + 300n + 20。對很多 n 值，300n 那項比較大。它還是 O(n²) 嗎？
- 是。big-O 是**近似的**表示法，說的是 n **夠大**時被什麼函數包住。n₀ 和 c 當然重要，但它給了一個很好的概念。
- 【口頭】實際上，很少發生「漸近複雜度比較高的反而比較快」。O(n) 的演算法或電路通常比 O(n²) 快得多。當然可以舉出病態的例子，但那很少見。

**Ripple carry adder 的複雜度**：t_h + (n − 1)t_f，t_h、t_f 是常數，所以是 **O(n)**。32 位元大約 k × 32 步，64 位元大約 k × 64 步。

【課本補充】課本說最壞的情況是 carry 從 LSB 一路傳到 MSB。

---

## 6. Carry select adder：O(√n)

[35:56]–[50:21]　課本 §8.1.4（p.313–314）

**想法**：把位元分成大小為 k 的**塊**（block）。

- 例：32 位元、k = 4：A₁～A₄ 和 B₁～B₄ 一塊，A₅～A₈ 和 B₅～B₈ 一塊……
- 每塊用一個小的 ripple carry adder 加。
- carry 在**塊之間**傳。光這樣還是 O(n)。

**關鍵**（[38:32]，【口頭】「這是 carry select adder 的精彩想法，非常重要」）：

- 每一塊的輸入只有兩種可能的 carry in：**0 或 1**。
- 所以每一塊**同時算兩個結果**：
  - 結果 1：假設 carry in = 0
  - 結果 2：假設 carry in = 1
  - 每個結果有 k 個 sum 位元 ＋ 一個 carry out。
- 【口頭】這是硬體，**所有東西都可以平行做**，不用一個接一個。

**兩個 stage**：

- **Stage 1**：所有的塊**同時**算出兩個結果。每塊是 k 位元的 ripple carry adder，所以要 **O(k)**。
- **Stage 2**：從最低的那塊開始，carry in 是 0，**立刻選**出對應的結果（用 multiplexer），把它的 carry out 送到下一塊；下一塊又立刻選……
  - 每一塊只要常數時間 O(1)，共 n／k 塊，所以 **O(n／k)**。
  - carry 現在是**以塊為單位**在傳，步伐比較大。

**總時間**：

```
T(k) = k + n/k
```

- n 是固定的，k（塊的大小）可以選。
- 微分求最小值：dT/dk = 1 − n/k² = 0 → **k = √n**。
- 代回去：√n + n/√n = **2√n = O(√n)**。
- 例：32 位元加法，√32 在 5 和 6 之間，用 5 位元的塊。§12.2 實測：64 位元時 k = 8（= √64）最快。

- 【口頭】病態的例子當然有，但一般情況下 carry select adder 真的比 ripple carry adder 快；「我鼓勵學生自己用硬體實作看看」。位元數越多（64、128），差距越明顯。
- 【課本補充】Stage 2 每塊用一個 **multiplexer** 選結果；選擇器的輸入就是兩種情況產生的 C_out。

---

## 7. Carry lookahead adder 的核心：generate 與 propagate

[50:21]–[61:24]　課本 §8.1.5、Equation 8.2–8.4、Example 123（p.314–315）

- 【口頭】「問一條魔法魚：能不能更好？魔法魚說可以，好得多。」→ **O(log n)**。
- 這是**大部分商用處理器**用的加法器，也最複雜，「要慢慢來」。
- 【課本補充】O(log n) 已經被證明是兩個 n 位元數相加的**理論下界**。log 預設以 2 為底；不同底的對數只差一個常數倍，在 big-O 裡沒差。

**洞見**：加法的問題在於 **carry**，它是一個循序的瓶頸。carry select 的加速也是來自「很快算出 carry」。所以要專心**快速算出 carry**，甚至不用先做加法。

**看一個位元對（bit pair）aᵢ、bᵢ**：

| aᵢ bᵢ | C_in = 0 → C_out | C_in = 1 → C_out | 叫做 |
|---|---|---|---|
| 0 0 | 0 | 0 | carry 被**吸收**（absorbed） |
| 0 1 | 0 | 1 | carry 被**傳遞**（propagate）：C_out = C_in |
| 1 0 | 0 | 1 | 同上 |
| 1 1 | 1 | 1 | carry 被**產生**（generate）：不管 C_in 都是 1 |

```
gᵢ = aᵢ · bᵢ         （generate）
pᵢ = aᵢ ⊕ bᵢ         （propagate）
C_out = gᵢ + pᵢ · C_in
```

- C_out 是 1 只有兩種可能：這一位**自己產生** carry，或者這一位**傳遞**了進來的 carry 而且進來的是 1。
- 【口頭】以一個「塊」來說：例如 `11` + `11`，不管 carry in 是 0 還是 1，carry out 都是 1，這一塊就在 generate。

**Example 123**：aᵢ = 0、bᵢ = 1

```
gᵢ = 0 · 1 = 0
pᵢ = 0 ⊕ 1 = 1
C_out = 0 + 1 · C_in = C_in
```

---

## 8. 多位元的 G 和 P

[61:24]–[68:45]　課本 Equation 8.6–8.8、Table 8.3（p.315）

把好幾個位元對看成一塊，也可以定義這一塊的 G 和 P。

**2 位元**：

```
C_out² = g₂ + p₂ · C_out¹
       = g₂ + p₂ · (g₁ + p₁ · C_in)
       = (g₂ + p₂·g₁) + (p₂·p₁) · C_in
          └── G₂ ──┘    └ P₂ ┘
```

- **G₂**：第 2 對自己產生，**或**第 1 對產生、第 2 對傳遞。
- **P₂**：每一對都傳遞，進來的 carry 才會出現在輸出，所以是 AND。

**3 位元**：

```
G₃ = g₃ + p₃·g₂ + p₃·p₂·g₁
P₃ = p₃·p₂·p₁
```

**4 位元**：

```
G₄ = g₄ + p₄·g₃ + p₄·p₃·g₂ + p₄·p₃·p₂·g₁
P₄ = p₄·p₃·p₂·p₁
```

**n 位元**：

```
C_outⁿ = Gₙ + Pₙ · C_in
```

- 規律：Gₙ 是「某一對產生，而且它左邊所有的對都傳遞」的 OR；Pₙ 是所有 p 的 AND。
- 如果能**事先算好** Gₙ 和 Pₙ，就能一步從 C_in 得到 C_out。
- 問題：從 4 位元的例子可以看出，n 很大時這些式子很難直接算。

---

## 9. 把兩塊的 G、P 合併

[68:45]–[77:47]　課本 Figure 8.7、Equation 8.9（p.315–316）

把 n 位元的一塊分成兩部分：右邊 1～m、左邊 m+1～n。中間傳過去的 carry 叫 **C_sub**。

```
 C_out ← [ 左：m+1 … n ] ← C_sub ← [ 右：1 … m ] ← C_in
```

```
C_out = G_{m+1,n} + P_{m+1,n} · C_sub
      = G_{m+1,n} + P_{m+1,n} · (G_{1,m} + P_{1,m} · C_in)
      = (G_{m+1,n} + P_{m+1,n}·G_{1,m}) + (P_{m+1,n}·P_{1,m}) · C_in
         └───────── G_{1,n} ─────────┘   └───── P_{1,n} ─────┘
```

> **合併規則**：
> - **G_{1,n} = G_左 + P_左 · G_右**：左邊自己產生，**或**右邊產生、左邊傳遞。
> - **P_{1,n} = P_左 · P_右**：兩邊都傳遞。

- 【口頭】直覺：整塊的 carry out 是 1，只有兩種情況：整塊 generate（不管 C_in），或整塊 propagate 而且 C_in 是 1。
- 合併只要**一個 OR 和一個 AND**（G）、**一個 AND**（P）：**常數時間**。
- 【課本補充】「這是一個非常強大的性質，是 carry lookahead adder 的基礎。」

---

## 10. Carry lookahead adder 的 Stage I：O(log n)

[77:47]–[89:26]　課本 Figure 8.8（p.316）

**想法**：**divide and conquer**（分治法）：先解很小的塊，再一路合併成越來越大的塊。

```
level 1:  (2-1) (4-3) (6-5) … (32-31)      16 個 (G,P) 電路
level 2:  (4-1) (8-5) …       (32-29)        8 個
level 3:  (8-1) (16-9) (24-17) (32-25)        4 個
level 4:  (16-1)        (32-17)              2 個
level 5:  (32-1)                             1 個
```

- 先**平行**算出每一個位元對的 g、p（或每兩個位元對一塊）。
- 每一層把兩個相鄰的塊用 §9 的規則合併。
- **同一層的所有合併都同時進行**：【口頭】「硬體天生就是平行的，不是串聯的。」它們之間沒有相依性。
- 每一層只要**常數時間** O(1)（一個 OR、一個 AND）。
- 每往上一層，塊的數量減半：16、8、4、2、1，所以有 **O(log n)** 層。
- 所以 Stage I 要 **O(log n)**。
- 最後得到整個 32 位元的 G₃₂ 和 P₃₂，C_out = G₃₂ + P₃₂ · C_in。
- 結果是一個**倒三角形**的 (G,P) 函數樹，每一個節點代表一段範圍的位元。

【口頭】問題是：要怎麼用這棵樹**真的做出加法**？（下一支影片。）

---

## 11. 【課本補充】Stage II

課本 Figure 8.9（p.317–318），影片的這一支還沒講到（第 22 支會講）。

- 用 Stage I 的 (G,P) 樹算出**每一塊**真正的 carry in。
- 每一層最右邊的 (G,P) 塊範圍都從 1 開始，所以可以直接用 C_in¹ 算出自己的 carry out：C_out = G + P · C_in¹。
- 範圍是 (r2, r1) 的塊，它的 carry out 就是範圍 (r3, r2+1) 那些塊的 carry in。正確的 carry 值沿著樹往下傳到葉子。
- 最底層（level 0）是一排 **2 位元的 ripple carry adder**，拿到正確的 carry in 後算出 sum 位元。
- 也有 O(log n) 層、每層常數時間，所以 Stage II 也是 **O(log n)**。整個 carry lookahead adder 是 **O(log n)**。
- 【課本補充】通常最低位的 carry in 是 0，但有些特殊指令（例如 ARM 的 **ADC**，[第 9 支](lecture09_note.md#11-用到-carry-的指令adcsbcrsc)）會用非零的 C_in¹。

> **Way Point 5**：ripple carry adder O(n)；carry select adder O(√n)；carry lookahead adder O(log n)。

---

## 12. 【實測】

本機：Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10。程式在 `experiments/lecture21/`。

`adders.py` 用**閘級**模型實作三種加法器：每個訊號記錄（值, 到達時間），每個 AND／OR／XOR 閘延遲 1，multiplexer 延遲 2。然後量**最長路徑**（critical path）。

- ripple：一串 full adder。
- carry select：每塊兩個 ripple adder（carry in = 0 和 1）同時算，再用 mux 選。k = √n。
- carry lookahead：用 **parallel prefix**（Kogge-Stone 式）實作：log n 層的 (G,P) 合併，同時得到每一位的 carry。課本的 Stage II 是另一種把 carry 往下傳的方式，但複雜度一樣是 O(log n)。

### 12.1 三種加法器的延遲

每個 n 測了 201 組（包括 carry 會從頭傳到尾的 `11…1 + 1`）：

```
$ python3 adders.py
   n  ripple  select(k=√n)  lookahead   correct?
   4       9             9          7   True
   8      17            13          9   True
  16      33            17         11   True
  32      65            25         13   True
  64     129            33         15   True
 128     257            47         17   True
```

- **三種都算對**（correct = True）。
- **ripple**：每多一位多 2 個閘延遲（2n + 1），**O(n)**。n 加倍，延遲加倍。
- **carry select**：n 從 16 到 64（4 倍），延遲從 17 到 33（約 2 倍）：**O(√n)**。
- **carry lookahead**：n 每加倍，延遲只多 **2**（每多一層合併，多一個 AND ＋ OR）：**O(log n)**。128 位元只要 17 個閘延遲，ripple 要 257 個。

### 12.2 carry select 的最佳塊大小

n = 64，改變 k：

```
$ python3 kopt.py
n=64 k= 1: delay 131
n=64 k= 2: delay 69
n=64 k= 4: delay 41
n=64 k= 6: delay 35
n=64 k= 8: delay 33      ← 最小：k = √64
n=64 k=11: delay 35
n=64 k=16: delay 41
n=64 k=32: delay 69
n=64 k=64: delay 131
```

- **k = 8 = √64 時最快**，和 §6 的微分結果一樣。
- 兩邊對稱：k = 2 和 k = 32 一樣慢、k = 4 和 k = 16 一樣，因為 T(k) = k + n/k 本身就對 k ↔ n/k 對稱。
- k = 1 或 k = 64 退化成 ripple carry adder（甚至更慢，因為多了 mux）。

---

## 13. 一頁速查表

| 主題 | 重點 |
|---|---|
| Half adder | sum = a ⊕ b；carry = a · b |
| Full adder | s = a ⊕ b ⊕ C_in（奇數個 1）；C_out = ab + aC_in + bC_in（至少兩個 1） |
| Ripple carry | 最低位 half adder，其他 full adder；t_h + (n−1)t_f = **O(n)** |
| big-O | f(n) = O(g(n))：存在 c、n₀，n > n₀ 時 \|f\| ≤ c\|g\|；多項式取最高次 |
| Carry select | 分成 k 位元的塊；每塊同時算 C_in = 0 和 1；再依序用 mux 選 |
| 時間 | k + n/k；k = √n 時最小 = 2√n = **O(√n)** |
| Generate | gᵢ = aᵢ · bᵢ：不管 C_in，C_out 都是 1 |
| Propagate | pᵢ = aᵢ ⊕ bᵢ：C_out = C_in |
| Absorb | aᵢ = bᵢ = 0：C_out = 0 |
| Carry | C_out = g + p · C_in |
| 多位元 | Gₙ = gₙ + pₙgₙ₋₁ + … + pₙ…p₂g₁；Pₙ = pₙ…p₁ |
| 合併 | G = G_左 + P_左·G_右；P = P_左·P_右（常數時間） |
| CLA Stage I | (G,P) 樹，log n 層，每層同時合併 → **O(log n)** |
| CLA Stage II | 沿樹把正確的 carry 往下傳到 2 位元 ripple adder → O(log n) |
| 下界 | O(log n) 是 n 位元加法的理論下界 |

---

## 14. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| Carry lookahead 的 Stage II | **第 22 支**：Computer Arithmetic Part 2 |
| 用加法器做乘法：iterative、Booth、Wallace tree | **第 22–23 支** |
| 除法：restoring、non-restoring | **第 24 支附近** |
| 浮點數加減乘除的硬體 | **第 25–26 支** |
| 處理器裡的 ALU | **第 27–29 支**：課本 Ch9 處理器設計 |
| ARM 的 adc、x86 的 adc（用到 carry in） | [第 9 支](lecture09_note.md#11-用到-carry-的指令adcsbcrsc)、[第 12 支](lecture12_note.md#7-加減法addsubadcsbb) |

---

## 附：本講一句話總結

> **兩個 1 位元數相加用 half adder（sum = a ⊕ b、carry = a · b），加上進來的 carry 就要用 full adder（s = a ⊕ b ⊕ C_in、C_out = ab + aC_in + bC_in）；把它們串起來就是 ripple carry adder，carry 一位一位往左傳，時間 t_h + (n − 1)t_f = O(n)（big-O 忽略常數、只看 n 夠大時的上界）。Carry select adder 把位元分成 k 位元的塊，每塊同時算出 carry in 為 0 和 1 的兩個結果，再從最低塊開始用 mux 依序選，時間 k + n/k，在 k = √n 時最小，為 O(√n)。Carry lookahead adder 改成直接算 carry：每個位元對的 generate gᵢ = aᵢbᵢ、propagate pᵢ = aᵢ ⊕ bᵢ，C_out = g + p · C_in，而兩塊可以用 G = G_左 + P_左·G_右、P = P_左·P_右 在常數時間內合併，於是用一棵 log n 層的 (G,P) 樹平行算出整個範圍的 G、P（Stage I，O(log n)），下一講再把 carry 傳回各塊（Stage II）。實測閘級模型：128 位元時 ripple 要 257 個閘延遲、carry select 47 個、carry lookahead 只要 17 個；carry select 在 n = 64 時以 k = 8 最快。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 7: Computer Arithmetic, Part 1/6 錄影逐字稿（YouTube 自動字幕，1:29:40）、Sarangi *Basic Computer Architecture* v3.09 Ch8 §8.1（p.305–318）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
