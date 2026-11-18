# F.4 Postprocessing

> 原書 p.784.e263–784.e273（Appendix F: Floating-Point Implementation）
> 涵蓋 F.4.1 Shift Calculation、F.4.2 Initial Result Mux、F.4.3 Normalization Shift、F.4.4 Correction Shift、F.4.5 Sign Logic、F.4.6 Rounding、F.4.7 Special Cases and Flags

## 這節在講什麼

後處理器是 FMA、divsqrt、cvt 三個單元的**共同出口**（第 16.2.6 節）。這節逐塊拆解 Fig. 16.16。

核心挑戰是：**三個單元產生的中間結果格式完全不同，卻要共用同一個正規化移位器**。解法是每個單元各有一個「移位計算」模組，把自己的結果排進統一的欄位格式，再由一個多工器選出當前運算的那一路。

最後 F.4.7 處理浮點最惡名昭彰的部分——**Underflow 旗標的判定**，需要同時看捨入前和捨入後的結果。

## 關鍵點

### 共用移位器造成的複雜度

> "The normalization shifter needs a fixed number of input bits, but each execution unit produces a different number of significand output bits. The shift calculation blocks place each execution unit output in suitable bitfields; then an initial result mux chooses the output for the active execution unit."
> — p.784.e263

中文解釋：**問題是「一個移位器要服務三種不同格式」**。

> "The shift logic is complicated because it shares the same shifter for all three functional units and because the LZA has some uncertainty."
> — p.784.e263

中文解釋：書上誠實承認「移位邏輯很複雜」，而且點出兩個原因：**共用**和 **LZA 的不確定性**（F.1.6 講的「可能多估一位」）。

> "Define the bit width of ShiftIn and other normalization shift buses to be N_m = max(3N_f + 6, b + N_f + 2), the greater of the widths from the FMA and divider. The normalization shifter places the answer in the most significant bits."
> — p.784.e263

中文解釋：**匯流排寬度取兩者的最大值**——又一個「用面積換統一」的取捨。

### 三個移位計算模組

**FMA（`fmashiftcalc`，Table F.25）**

> "It computes the normalization shift inputs, the normalized exponent, and some special cases"
> — p.784.e264

中文解釋：輸入是 FMA 的 `S_m`（和的 significand）、`S_e`（指數）、`S_cnt`（**LZA 估計的前導零數，可能多 1**）。

輸出的 `ShiftAmt_fma` 有兩種情況：
- 結果是 subnormal：`S_e + N_f + 2`
- 否則：**`S_cnt + 1`**（用 LZA 的估計值）

**`PreResultSubnorm`（結果是否 subnormal）的判定發生在 LZA 修正之前**——這是後面要做修正移位的原因。

**Divide/Square Root（`divshiftcalc`，Table F.26）**

> "Excluding integer division and special cases such as 0 or infinity, divsqrt produces a result Q_m ∈ (1/2, 2). If the answer is normalized and less than 1, the result may need a normalization left shift by 1 to bring it into the range [1, 2). If the answer is subnormal, it is preshifted right and then shifted back left into the appropriate position using the normalization shifter. These two cases can be combined by always preshifting right and then shifting left appropriately."
> — p.784.e264

中文解釋：**又是「先預右移、之後只左移」的技巧**（F.1.4、F.3 都用過）。這是附錄 F 反覆出現的手法——**單向移位器比雙向便宜一半**。

`ShiftIn_div = Q_m >> (N_f + 1)`（補零），`ShiftAmt_div` 依是否 subnormal 選擇。

**Convert（`cvtshiftcalc`，Table F.27）**——F.3.2 已經講過。

### F.4.2–F.4.3 多工與移位

> "As shown in Table F.28, the initial result mux chooses ShiftIn, ShiftAmt, and NormExp from one of the three execution units depending on the current operation."
> — p.784.e265

中文解釋：Table F.28 只有三列（`ShiftIn`、`ShiftAmt`、`NormExp`）× 三欄（FMA、divsqrt、cvt）——**三個訊號就把三個單元統一起來了**。

> "The normalization shifter simply shifts ShiftIn left by ShiftAmt"
> — p.784.e265

中文解釋：**移位器本身極簡單——只做左移**。所有複雜度都被推到前面的移位計算模組裡了。這是一個好的模組化：把「難的部分」和「大的部分」分開。

### F.4.4 修正移位：處理 LZA 的誤差

> "The correction shifter (shiftcorrection) performs a possible 1-bit shift after normalization to compensate for LZA error. The LZA estimated the number of leading digits S_cnt but may have overestimated by 1. The LZA error is detected by looking at the most significant bit in the 2-bit integer portion of the Shifted output. If it is asserted, the correction shifter performs a 1-bit right shift."
> — p.784.e265–e267

中文解釋：**這就是 F.1.6 說的「後處理要準備一個修正移位」的具體實作**。

偵測方法很簡單：**正規化之後，如果 2 位元整數部分的最高位是 1，就表示 LZA 多估了一位**（正確的話最高位應該是 0、次高位是 1）。此時右移一位、指數加一。

> "Subnormal results have no integer portion, but their normalization shift depends only on the exponent, not the LZA, so they have no LZA error."
> — p.784.e267

中文解釋：**subnormal 不會有 LZA 誤差**，因為它的移位量是從指數算出來的，不是從 LZA 來的。

**又一個誠實的勘誤**（邊欄）：

> "After this section was written, the correction shifter was modified to fix a fcvt bug in certain configurations (PR #784). The RTL (SystemVerilog code) contains the latest logic. There is likely still room for optimization."
> — p.784.e267（邊欄）

中文解釋：這是附錄 F 的第二個公開勘誤（第一個在 F.1.4 的 FMA 對齊移位）。**兩個 bug 都在移位邏輯上**——這印證了書上說的「移位邏輯很複雜」。

### F.4.5 符號邏輯：零的符號很麻煩

> "The sign logic, shown in Table F.31, is performed by the roundsign and resultsign modules."
> — p.784.e267

中文解釋：Table F.31 裡最複雜的是 **`Zero_s`（零結果的符號）**。IEEE 754 規定 `+0` 和 `−0` 是不同的東西，所以必須算對。規則：

- **乘法**（沒有加減）：符號是乘積的符號 `P_s`
- **有效減法**（乘積與加數異號）：`(P_s ⊕ A_s) & ~Mult`
- **精確的零結果**（round 位元 `R'` 和 sticky 位元 `T'` 都是 0）：`~(R'|T')`
- **有效減法產生精確的零**：
  - `RM`（朝 −∞ 捨入）模式：**符號是負的（1）**
  - 其他模式：**符號是正的（0）**
- 其他情況（乘法與有效加法）：零的符號就是乘積的符號

**「減法得到零時，朝 −∞ 捨入要給 −0，其他模式給 +0」** 是 IEEE 754 的明確規定（例如 `1.0 − 1.0` 在 RM 下是 `−0`）。這種規則不記得就一定會錯，只能查表。

### F.4.6 捨入：溢出自動進位到指數

> "Recall from Section 16.1.3 that rounding chooses between the two nearest values of the fraction. The truncated fraction is called TRUNC, and the next larger fraction is called RND. RND is TRUNC plus an ulp."
> — p.784.e268

中文解釋：Table F.32 的實作重點是這個**極簡的技巧**（邊欄）：

> "When rounding up a number with all 1s in the fraction, the fraction becomes all 0s, and the exponent increases by 1. The rounder handles this gracefully by adding an ulp to a bus comprising the exponent and fraction (see R_e^full). A carry-out of the fraction automatically bumps up the exponent."
> — p.784.e268（邊欄）

中文解釋：**把指數和小數放在同一條匯流排上做加法**——小數全是 1 時進位會自動溢出到指數欄位，**完全不需要偵測與特殊處理**。

這是浮點格式設計的一個回報：指數在高位、小數在低位，所以它們拼起來剛好可以當一個整數加。

`R_e^full = M_e + Plus1`（`B(N_e+2)` 格式）、`R_f = M_f[...] + Plus1`、`R_e` 取 `R_e^full` 的低 `N_e` 位元。

另外注意 `R`、`L`、`G` 位元的定義**在轉整數時不同**（`CorrShiftSz − XLEN − ...` vs `CorrShiftSz − N_f − ...`）——因為整數結果的小數點位置不一樣。

### F.4.7 Underflow：全節最難的部分

> "The flags module also raises Overflow, Underflow, and Inexact flags as described in Section 16.1.4. However, computing the intermediate result for Overflow and Underflow is costly because it would involve a variable normalization shift and a rounding addition. The flags block avoids this by deducing these flags from the results before and after rounding and from the G, R, and T bits."
> — p.784.e272

中文解釋：**問題的根源**：IEEE 754 說 Underflow 要看「中間結果（intermediate result）」是不是 tiny，但中間結果本身需要一次完整的正規化移位加捨入——**太貴**。

**解法是「從捨入前和捨入後的結果反推」**。書上定義：
- **normalized result**（捨入前）：指數 `M_e`
- **rounded result**（捨入後）：指數 `R_e^full`

Overflow 簡單：`R_e^full > e_max + bias`。

Underflow 有**兩種情況**：

> "Case A: The intermediate result is tiny if the rounded result is tiny. The rounded result is tiny if the final exponent R_e^full ≤ 0, meaning it is smaller than the smallest normalized number."
> — p.784.e272

> "Case B: The intermediate result is also tiny if the rounded result is a normal number, the normalized result is subnormal, and the rounding does not push the intermediate result up to the smallest normalized value. This case only occurs when the result is rounded up from the largest subnormal number 0.1111… × 2^emin to the smallest normalized number 1.0 × 2^emin."
> — p.784.e272

中文解釋：**Case B 是那個惡名昭彰的邊界**。Fig. F.36 畫出關鍵差異：

```
0.1111 1111 11 GRT × 2^emin      ← subnormal 結果（捨入前）
1.1111 1111 1 GRT  × 2^(emin−1)  ← 中間結果（捨入前）
```

**同一個數值，兩種寫法的 G/R/T 位元位置差一位**。所以「subnormal 形式捨入後」和「中間結果形式捨入後」可能給出不同答案——這就是為什麼 Table 16.3 要區分 `L' = G, R' = R, T' = T`（中間結果）和 `L' = L, R' = G, T' = R|T`（subnormal 結果）。

三個式子（F.62–F.64）：

```
IntermediateUp = G & UfPlus1
Tiny = (R_e^full ≤ 0) | (R_e^full = 1 & M_e = 0 & ~IntermediateUp)
Underflow = Tiny & Inexact
```

中文解釋：
- **`Tiny` 的第一項是 Case A**（捨入後就是 tiny）
- **第二項是 Case B**：捨入後剛好是最小正規數（`R_e^full = 1`）、捨入前是 subnormal（`M_e = 0`）、而且**中間結果不會被捨入上去**（`~IntermediateUp`）
- **Underflow 需要同時 tiny 且 inexact**——這是第 16.1.4 節說的 `Underflow = (I_e < e_min) & Inexact`

**`UfPlus1` 是用中間結果的捨入位元算出的「會不會進位」**（Table F.32），和一般的 `Plus1` 平行計算。**用兩組捨入判斷代替一次額外的移位與加法**——這就是「deducing these flags from the results before and after rounding」的意思。

### 特例表

Table F.33–F.40 是七張特例表，涵蓋 FMA、divide、square root、compare、min/max、float→int、float→float。幾個值得記的：

**優先順序很重要**：
> "In the event of multiple rows being satisfied, the highest row has priority (e.g., NaN comes before infinities)."
> — p.784.e269

**FMA（Table F.33）**：`∞ − ∞` 和 `0 × ∞` 設 Invalid 並回 NaN。

**Divide（Table F.34）**：`∞/∞` 和 `0/0` 是 Invalid；**`val/0` 設 Divide by Zero 但不是 Invalid**（回 ∞）——這兩個旗標的區別常被搞混。

**Square root（Table F.35）**：**`−0` 開根號回 `−0`**（不是 NaN！），但負數開根號是 Invalid。

**Compare（Table F.36）**：**`flt`/`fle` 遇到 quiet NaN 也會設 Invalid，但 `feq` 不會**——這正是第 16.4.5 節 Zfa 加入 `fltq`/`fleq` 的原因（compareQuiet vs compareSignaling）。

**Min/max（Table F.37）**：一個 NaN 一個正常值時**回正常值**——這是 minimumNumber 語意（第 16.4.5 節提過 Zfa 的 `fminm`/`fmaxm` 改成回 NaN）。

**Float→int（Table F.39）**：**NaN 回 MAXINT**，溢位飽和到 MAXINT/MININT——這正是第 16.5 節列的「RISC-V 偏離 SoftFloat 預設行為」的第 3 點。

### 結果選擇

> "The specialcase module picks Result as described for operation-specific special cases in the earlier tables. Otherwise, it uses Table F.41 to round very large or very small results or pick the normal result."
> — p.784.e273

中文解釋：Table F.41 的三選一——`OfResult`（溢位，依捨入模式回 ∞ 或 MAXNUM）、`UfResult`（下溢，回 0 或最小 subnormal）、`NormRes`（正常）。

**注意溢位不一定回 ∞**——朝零捨入時回 MAXNUM（最大有限數）。這是 Table 16.4 規定的。

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| `N_m` | 正規化移位匯流排寬度，取 FMA 與 divider 的較大者 |
| `fmashiftcalc` / `divshiftcalc` / `cvtshiftcalc` | 三個單元各自的移位計算模組 |
| `ShiftIn` / `ShiftAmt` / `NormExp` | 統一後送給移位器的三個訊號 |
| initial result mux | 依當前運算選擇三個單元之一 |
| `shiftcorrection` | 修正 LZA 多估一位的 1 位元右移 |
| `S_cnt` | LZA 估計的前導零數（可能多 1） |
| `CorrShiftSz` | 修正移位後的小數位元數 |
| normalized result / rounded result | 捨入前／捨入後的結果 |
| `M_e` / `R_e^full` | 捨入前／捨入後的指數 |
| `TRUNC` / `RND` | 截斷值／加一個 ulp 的值 |
| `Plus1` / `UfPlus1` | 一般捨入／下溢判定用的「是否進位」 |
| `IntermediateUp` | 中間結果會被捨入上去 |
| `Tiny` | 中間結果小於最小正規數 |
| Case A / Case B | Underflow 的兩種判定情況 |
| `roundsign` / `resultsign` / `specialcase` / `flags` | 後處理器的四個子模組 |
| MAXNUM / MAXINT / MININT | 最大有限浮點數／最大整數／最小整數 |

## 所以呢

後處理器是整個 FPU 裡「最不起眼但最容易出錯」的部分——**七張特例表加上兩種 Underflow 判定**，每一格錯了就是一個 TestFloat 會抓到的 bug。書上在這一節公開了兩個真實的勘誤（FMA 對齊移位、修正移位的 fcvt bug），而且兩個都在移位邏輯上。

三個值得帶走的技巧：
1. **「先預右移、之後只左移」**——在 F.1.4、F.3、F.4.1 出現三次，是全附錄最常用的手法
2. **把指數和小數放同一條匯流排做捨入加法**——進位自動溢出到指數，零成本處理「小數全 1」的情況
3. **從捨入前後兩個結果反推 Underflow**——用兩組平行的捨入判斷，代替一次昂貴的中間結果計算

附錄 F 到此結束，整本書的正文與附錄也到此完結。
