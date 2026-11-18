# F.1 Fused Multiply-Add

> 原書 p.784.e193–784.e200（Appendix F: Floating-Point Implementation）
> 涵蓋 F.1.1 Multiply Significands、F.1.2 Product and Addend Signs、F.1.3 Add Exponents、F.1.4 Alignment Shift、F.1.5 Add Aligned Significands、F.1.6 LZA、F.1.7 Multipath FMA Optimization

## 這節在講什麼

附錄 F 是第 16 章浮點的實作細節，「為了簡潔而移出正文」。F.1 把 Fig. 16.11 的 FMA 方塊圖逐塊拆開，給出每個訊號的**格式、邏輯式、用途**（Table F.1–F.5）。

最有價值的兩部分是：**F.1.4 的對齊移位**（示範了「用重新命名代替硬體」的技巧）和 **F.1.6 的 LZA**（用正規表示式推導出一個純組合邏輯的前導零預測器，是全書最精巧的推導之一）。

## 關鍵點

### 符號慣例

> "The FMA computes X × Y + Z. The mantissas (significands) are X_m, Y_m, Z_m, the exponents are X_e, Y_e, Z_e, and the signs are X_s, Y_s, Z_s. The intermediate product is P = X × Y. N_f and N_e are the number of fraction and exponent bits, respectively."
> — p.784.e193（邊欄）

中文解釋：下標 m/e/s 分別是 significand/exponent/sign。這個記號貫穿整個附錄。

（另一個邊欄：**附錄中的訊號名用斜體而非等寬字**，以配合理論推導的排版習慣。）

### F.1.1–F.1.3：乘法、符號、指數（三個平行進行的簡單步驟）

> "The significands of X and Y are multiplied using an unsigned integer multiplier to produce P_m in U2.2N_f form"
> — p.784.e193

中文解釋：**U1.N_f × U1.N_f = U2.2N_f**——兩個「1.xxx」相乘，結果在 [1, 4) 範圍，所以整數部分需要 2 位元。這呼應第 16.2.2 節講的「乘積可能要右移一位」。

> "The product's sign P_s is negative if X or Y but not both are negative. For subtraction, the sign of Z is flipped to produce the addend sign A_s. If P_s and A_s have different signs, InvA indicates that an effective subtraction is taking place."
> — p.784.e193

中文解釋：Table F.2 的三個 XOR：
- `P_s = X_s ⊕ Y_s`
- `A_s = Z_s ⊕ sub`（`sub` 是指令要求減法）
- `InvA = P_s ⊕ A_s`

**`InvA` 是關鍵訊號**——它表示「實際上在做減法」，不管指令寫的是加還是減。後面的加法器路徑完全取決於它。

> "The product's biased exponent is in the range [-bias + 2, 3bias], which requires N_e + 2 bits to represent it in two's complement form."
> — p.784.e194

中文解釋：Table F.3。**為什麼要 N_e + 2 位元**：兩個指數相加可能溢出一位，再加上要用二補數表示負值（下溢的情況），所以比原本多兩位。

`S_e`（和的指數）的選擇邏輯很精巧：
- 一般情況 `S_e = P_e`（乘積的指數，因為加數會被對齊到乘積）
- 但如果 `A_e − P_e > N_f`（乘積小到會被完全丟棄），`S_e = Z_e`

### F.1.4 對齊移位：用重新命名代替硬體

**問題**：`Z` 可能比 `P` 大很多或小很多，所以理論上要能雙向移位。但：

> "Z may need shifting to the left or right, but it is more difficult to build a bidirectional shifter than a unidirectional shifter. Hence, we preshift Z left by the maximum amount that might be needed, then shift right by the desired amount plus the preshift amount."
> — p.784.e194–e195

中文解釋：**這是一個標準的硬體技巧——「先預移到最左，之後只要右移」**，雙向移位器的成本因此減半。

**而且預移是免費的**：

> "The maximum interesting preshift is N_f + 2, which would put the most significant bit of the product in the guard position of the result. This preshift is a trivial renaming with no hardware required."
> — p.784.e195

中文解釋：**「trivial renaming with no hardware required」**——左移一個固定的常數量，在硬體上就是接線接到不同的位置，零成本。這和第 18 章 ShiftRows「常數旋轉只是把位元換個名字」是同一個道理。

所以移位量重新定義為 `A_cnt = (P_e − Z_e) + N_f + 2`（Table F.4）。三種情況：
- **`A_cnt < 0`**：`P` 太小，`KillProd` 拉起，乘積被丟掉（只留 sticky）
- **`A_cnt > 3N_f + 3`**：`Z` 太小，`KillZ` 拉起，`Z` 設成 0（只留 sticky）
- **其他**：右移 `A_cnt` 位

Fig. F.1 用 `N_f = 4` 畫出整個過程：`Z_m` 先左移 6 位（`N_f + 2`），再右移 `A_cnt`（範圍 [0, 15]），結果 `Z_m^shifted` 有 `N_f + 3` 個整數位元和 `3N_f + 1` 個小數位元。

**一個誠實的勘誤**（邊欄）：

> "After this section was written, TestFloat revealed an FMA bug (cvw issue #578) related to subtraction of a small value. Z_m^shifted had to be widened to N_f + 4 integer bits and 3N_f + 2 fraction bits to obtain a correct guard bit. The extra integer and fraction bit propagate through the rest of the FMA and postprocessing. The root cause has not yet been fully understood."
> — p.784.e195（邊欄）

中文解釋：**這段很值得注意**——書上直接承認「有一個 bug，加寬一位就好了，但根本原因還沒完全搞清楚」。這印證了第 16 章反覆說的「浮點的 corner case 惡名昭彰」，也顯示 TestFloat（第 16.5 節）確實在抓真實的 bug。

### F.1.5 加法：兩個加法器並行，選正的那個

**問題**：`P` 和 `A` 是**符號-數值（sign-magnitude）**形式，不是二補數。

> "The adder computes S = P + A, where S, P, and A are all in sign-magnitude form. This is trickier than ordinary two's complement addition."
> — p.784.e196

中文解釋：Fig. F.2 的「簡單實作」需要：條件取負 → 加法 → 取絕對值 → LZC。**這條路徑上有三個加法器**（取負要加 1、加法、取絕對值又要加 1），太慢。

Fig. 16.11 的快速做法（Table F.5）：

> "Negation is replaced by an XOR to conditionally invert A, and the extra 1 is provided as a carry input to the significand adder. Instead of taking the absolute value, the circuit computes P + A and A − P in parallel and selects the positive one."
> — p.784.e196

中文解釋：**兩個技巧**：
1. **取負 = XOR 反相 + 進位加 1**——把「+1」塞進加法器的進位輸入，不用額外的加法器
2. **同時算 `PreSum = P − A` 和 `NegPreSum = A − P`，選正的那個**——用面積換掉取絕對值的那一級延遲

`NegSum`（`PreSum` 的符號位元）決定選哪個，也決定最終的 `S_s = P_s ⊕ NegSum`。

**negative sticky bit 的處理**：

> "If an effective subtraction occurs and the sticky bit of the subtrahend is 1, the result must be decremented by 1. A_sticky comes from P when KillProd = 1, or from Z when KillProd = 0. This negative sticky bit can be handled now in the two's complement adders by setting the carry in to 0 for P − A − 1 and A − P − 1."
> — p.784.e197

中文解釋：**「減 1」靠把進位輸入設成 0 來實現**——又一次把額外運算塞進既有的加法器，不加硬體。

Example F.1 走了一遍 `(1.0)(1.0) − 2^−13`（半精度）——減數小到只能影響 sticky 位元，結果是 `1.1111111111 11 × 2^−1`（比 1.0 小 1 ulp），帶 round 和 sticky 位元。

### F.1.6 LZA：全書最精巧的推導

**問題**：正規化需要知道 `S_m` 有幾個前導零，但 `S_m` 要等加法做完才有。

> "The most direct approach is to compute S_m and then use a priority encoder to count the leading zeros (LZC: leading zero counter). This is on the critical path, so a faster FMA uses an LZA to estimate the number of leading zeros in parallel with the addition."
> — p.784.e198

中文解釋：**LZA 的核心想法是「不用算出和，也能預測前導零」**。

**代價**：

> "The estimate can sometimes be too high by 1 bit, so the postprocessor may have to use a subsequent 1-bit correction shift to compensate"
> — p.784.e198

中文解釋：只是估計，可能多算一位——所以後處理要準備一個 1 位元的修正移位（F.4.1.1）。用「可能差一位」換「不用等加法完成」，非常划算。

**推導的方法：用正規表示式描述位元樣式**

> "Consider N-bit addition S = A + B + C_in. Let P, G, and K indicate whether a column i of the addition will propagate, generate, or kill a carry"
> — p.784.e198

中文解釋：式 (F.1)：`P_i = A_i ⊕ B_i`、`G_i = A_i B_i`、`K_i = ~A_i ~B_i`——這是第 5 章加法器的標準 PGK 訊號。

**關鍵推導**：什麼樣的 PGK 序列會產生 k 個前導零？

> "S is of the form 0^k 1x*, which is a regular expression indicating k leading zeros followed by a 1 and then any number of arbitrary bits. Any pattern leading with the k-bit string K^k or P^m GK^{k−m−1} will have k or k − 1 leading zeros."
> — p.784.e198

中文解釋：**兩種樣式會產生 k 個前導零**：
- **`K^k`**：前 k 欄全部 kill（A 和 B 都是 0），所以和的前 k 位都是 0
- **`P^m G K^{k−m−1}`**：前 m 欄 propagate、第 m+1 欄 generate、之後 kill。這時 propagate 欄各有一個 1，加起來 1+1=0 進位，進位又一路往上傳，**結果前 m+1 欄全是 0**

**所以問題變成「找出這兩個正規表示式在哪裡結束」**：

> "K_i followed by something other than K_{i−1} terminates both K^k and P^m GK^{k−m−1}. G_i followed by something other than K_{i−1} terminates P^m GK^{k−m−1}. P_i followed by K_{i−1} also terminates P^m GK^{k−m−1}."
> — p.784.e199

中文解釋：三個終止條件，合起來化簡成式 (F.3)：

```
F_i = K_i ~K_{i−1} + G_i ~K_{i−1} + P_i K_{i−1}
    = ~P_i ~K_{i−1} + P_i K_{i−1}
    = ~(P_i ⊕ K_{i−1})
```

**最後化簡成一個 XNOR。** 這是整段推導最漂亮的地方——一堆正規表示式的分析，結果只需要**每位一個 XNOR 閘**。

然後用優先編碼器（LZC）數 `F` 的前導零，就得到估計值。

Example F.2 驗證：`S = 000101 + 111101`（實際 `S = 000010`，4 個前導零），LZA 算出 `F = 000001`，前 5 位是 0，所以估計 5 或 4——**正確（估計可能多 1）**。

**負數結果的處理**：

> "Similarly, addition that has a negative sum has k ≥ 1 leading ones, so S is of the form 1^k 0x*."
> — p.784.e199

中文解釋：負數要數**前導一**，樣式變成 `G^k` 和 `P^m KG^{k−m−1}`，式 (F.4)：`F_i = ~(P_i ⊕ G_{i−1})`——**只是把 K 換成 G**，對稱得很漂亮。

**統一版本**：

> "One could perform two separate LZA (F) calculations for positive and negative sums, then pick the estimate based on the sign S. However, this requires two priority encoders. A more compact LZA generates a combined F that applies to both leading zeros and leading ones using Equation F.5"
> — p.784.e199

中文解釋：**用面積換面積的取捨**——兩個 LZA 要兩個優先編碼器（貴），統一版只要一個但每位要看**三欄**（`P_{i+1}`、第 i 欄、第 i−1 欄）。式 (F.5) 選了後者。

Fig. F.5 的示意圖顯示 **LZA 和 CPA 共用 PGK 邏輯**——又省一份。

### F.1.7 Multipath：Wally 沒做的優化

> "The FMA never encounters the worst case of a large alignment shift and a large normalization shift on the same operation."
> — p.784.e200

中文解釋：**這個觀察是 multipath 優化的全部基礎**。三種情況：

| 情況 | 對齊移位 | 正規化移位 |
| --- | --- | --- |
| **product-anchored**（乘積大） | 加數右移很多 | 小 |
| **addend-anchored**（加數大） | 乘積右移很多 | 小 |
| **massive cancellation**（兩者接近但異號） | 小 | **大** |

**沒有任何一種情況兩個移位都很大。** 所以單路徑設計（兩個都用最大寬度的移位器串聯）在關鍵路徑上付了永遠用不到的代價。

> "A multipath design uses separate logic for the product-anchored, addend-anchored, and massive cancellation cases to gain speed at the expense of more hardware [Seidel03, Quinnell07]. Wally does not implement a multipath FMA."
> — p.784.e200

中文解釋：Wally 選了簡單的單路徑——呼應第 16.2.2 節說的「focuses on a simple single-path approach like the classic IBM RS/6000 design」。

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| `P_s` / `A_s` / `InvA` | 乘積符號／加數符號／是否為實際減法 |
| U2.2N_f / U1.N_f / Q(a).(b) | 無號／有號定點格式（第 16.1.5 節的記號） |
| `A_cnt` | 對齊移位量 |
| `KillProd` / `KillZ` | 乘積／加數太小，直接丟棄 |
| preshift | 預先左移固定量，讓之後只需單向右移（零硬體成本） |
| sign-magnitude | 符號-數值表示，與二補數不同 |
| `PreSum` / `NegPreSum` | P−A 與 A−P，並行計算後選正的 |
| `NegSum` | `PreSum` 的符號，決定選哪個結果 |
| negative sticky bit | 減法時被減數的 sticky，靠設進位輸入為 0 處理 |
| LZA / LZC | 前導零預測器／計數器 |
| P, G, K | propagate / generate / kill，加法器的進位訊號 |
| regular expression | 此處用來描述會產生 k 個前導零的位元樣式 |
| correction shift | LZA 估計可能多一位，後處理的 1 位元修正 |
| product-anchored / addend-anchored / massive cancellation | multipath FMA 的三種情況 |
| ulp | unit in the last place |

## 所以呢

三個值得帶走的設計技巧：

1. **用重新命名代替硬體**——預移固定量在硬體上只是接線，把雙向移位器變成單向。
2. **把額外運算塞進既有加法器**——取負的「+1」塞進進位輸入、negative sticky 的「−1」靠進位輸入設 0。
3. **用「可能差一位」換「不用等」**——LZA 與加法並行，代價是後處理要準備一個修正移位。

而 LZA 的推導本身是一個範例：**把「什麼情況會有 k 個前導零」寫成正規表示式，分析它在哪裡終止，最後化簡成每位一個 XNOR**。這種「從樣式分析推出極簡邏輯」的手法，在第 18 章的 `fround` 遮罩和第 15 章的乘法器裡也看得到。

下一節 F.2 是除法與開根號——附錄 F 最長的一節。
