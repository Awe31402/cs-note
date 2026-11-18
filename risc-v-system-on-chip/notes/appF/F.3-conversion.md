# F.3 Conversion

> 原書 p.784.e258–784.e263（Appendix F: Floating-Point Implementation）
> 涵蓋 F.3.1 Result Exponent、F.3.2 Shift Amount Selection、F.3.3 Examples

## 這節在講什麼

第 16.2.4 節只用一頁講轉換單元（`fcvt`），這裡給出完整的邏輯（Table F.21）。

核心成果是**一組統一的公式**——float-to-float、float-to-int、int-to-float 三種轉換，用**同一個指數計算式**和**同一個移位器**完成。這是「把三件看起來不同的事變成同一件事」的漂亮示範。

## 關鍵點

### 統一的關鍵：定義一個「無界指數」E

> "Let E be the unbounded exponent of the input. For normalized floating-point numbers, E is the unbiased exponent. For subnorms, E is the exponent if the subnormal value were normalized with an exponent less than the minimum representable value. For integers, E is the exponent of the integer represented in normalized form. For example, 1.101 × 2³ has E = 3. 0.001 × 2^−15 = 1.0 × 2^−18 has E = −18. 101 = 1.01 × 2² has E = 2."
> — p.784.e258

中文解釋：**`E` 是「如果把這個數寫成 1.xxx × 2^E 的形式，指數是多少」**——不管輸入是正規化浮點數、subnormal、還是整數，都能用同一個概念描述。

**這個定義就是統一的基礎。** 三個例子涵蓋三種情況：
- 正規化浮點：`E` = unbiased exponent
- subnormal：`E` 比最小可表示指數還小（`0.001 × 2^−15` 要左移 3 位才正規化，所以 `E = −18`）
- 整數：`101` 寫成 `1.01 × 2²`，所以 `E = 2`

### 三種轉換用同一個公式

> "In all cases, E = ExpIn − bias − LeadingZeros and C_e = E + Bias_out. ExpIn = bias + XLEN − 1 for integer inputs, or the unpacked biased exponent for floating-point inputs. Bias_out = 1 for integer results, or the floating-point bias for floating-point results."
> — p.784.e261

中文解釋：**這是整節的核心**——兩行公式涵蓋全部：

```
E   = ExpIn − bias − LeadingZeros
C_e = E + Bias_out
```

差別只在兩個輸入參數怎麼設：

| 轉換類型 | `ExpIn` | `Bias_out` |
| --- | --- | --- |
| float → float | 解包出的 biased exponent | 輸出格式的 bias |
| float → int | 解包出的 biased exponent | **1** |
| int → float | **`bias + XLEN − 1`（設計時常數）** | 輸出格式的 bias |

**`ExpIn = bias + XLEN − 1` 這個技巧很巧妙**。整數沒有指數欄位，但如果把一個 XLEN 位元的整數看成「小數點在最右邊」的定點數，它的有效指數就是 `XLEN − LeadingZeros − 1`——代進公式剛好成立。而且這是**設計時常數**，硬體上是接線不是計算。

`Bias_out = 1` 也是一個定義出來的技巧：

> "For reasons described in Section F.4.1, the shifter first right shifts by 1 before left shifting by E + 1. Hence, for conversions to int, define Bias_out = 1 and use a shift amount C_e = E + Bias_out."
> — p.784.e260

中文解釋：轉整數本來沒有「指數」這回事，但**因為移位器先右移 1 位再左移，所以「移位量」剛好等於 `E + 1`**——於是把 `Bias_out` 定義成 1，`C_e` 就同時是「輸出指數」和「移位量」。**一個訊號兩用。**

### 三種轉換的移位量

> "Float-to-float conversion only requires shifting when the input or output is subnormal."
> — p.784.e261

中文解釋：三種情況的 `ShiftAmt`：

| 情況 | `ShiftAmt` | 為什麼 |
| --- | --- | --- |
| **float→float，兩邊都正規化** | **0** | 只要調 bias，位元不用動 |
| float→float，輸入是 subnormal | `LeadingZeros` | 左移到正規化 |
| float→float，輸出是 subnormal（`C_e < 0`） | `C_e + N_f − 1` | 淨效果是右移 `\|C_e\|` |
| **float→int** | `max(C_e, 0)` | 先預右移 1 位，再左移 `C_e` |
| **int→float** | `LeadingZeros` | 左移到正規化 |

> "If C_e ≤ 0, the integer part is 0, and the remaining bits are used for rounding."
> — p.784.e261

中文解釋：**float→int 時 `C_e ≤ 0` 表示這個浮點數的絕對值小於 1**——整數部分是 0，剩下的位元只用來決定捨入（例如 0.6 捨入成 1）。

### 為什麼「先右移再左移」

這個看似多餘的步驟其實是 F.1.4 那個技巧的重演——**用單向移位器代替雙向移位器**。轉換可能需要左移（int→float 正規化）也可能需要右移（float→int 對齊、輸出 subnormal），先預右移一個固定量，之後就只需要左移。

`ResSubnormUf` 旗標（`C_e ≤ 0` 且輸入非零）通知後處理器「結果是 subnormal 或下溢」，讓它調整正規化移位。

### 四個例子走一遍

Example F.27–F.30 涵蓋四種情況，假設 FPU 支援 half 和 single（half bias = 15、single bias = 127）：

**F.27 single→half（會損失精度）**
`X = 0xC17FF00 = −1.1111 1111 111 × 2³ = −15.9961`
- `ExpIn = 130`、`LeadingZeros = 0` → `E = 130 − 127 − 0 = 3`
- `C_e = 3 + 15 = 18`
- **兩邊都正規化，所以 `ShiftAmt = 0`**
- 但 half 只有 10 個小數位元，`1.1111111111 1` 捨入成 `10.0000 0000 0`
- 重新正規化：指數變 19，結果 `−1.0 × 2⁴ = 0xCC00 = −16`

**這個例子的重點**：捨入造成進位溢出到整數位元（`1.111... → 10.0`），指數要加 1。

**F.28 half(subnormal)→single（變成正規化）**
`X = 0x0060 = 0.00011 × 2^−14 = 5.72205 × 10^−6`
- `ExpIn = 113`、**`LeadingZeros = 4`** → `E = 113 − 127 − 4 = −18`
- `C_e = −18 + 127 = 109`
- **輸入是 subnormal，所以 `ShiftAmt = LeadingZeros = 4`**
- 結果 `1.10 × 2^−18 = 0x36C00000`，**精確且已正規化**

**這個例子示範了為什麼要有 `LeadingZeros`**——subnormal 的「真實指數」比存在欄位裡的小。

**F.29 half→int（RTZ）**
`X = 0xC8C0 = −1.0011 × 2³ = −9.5`
- `E = 130 − 127 − 0 = 3`
- **`C_e = E + 1 = 4`**（`Bias_out = 1`）
- `ShiftAmt = 4`：`1.0011` 先右移 1 位，再左移 4 位得 `1001.1`
- 朝零捨入 → `1001 = 9`，加負號 → **−9**

**F.30 int→half**
`A = −5 = 111...1111011`（32 位元二補數）
- 取絕對值：`0000...0101`
- **`LeadingZeros = 29`**，左移 29 位得 `1010 0000...`，取高位成 `1.0100 0000 00`
- **`ExpIn = bias + XLEN − 1 = 127 + 32 − 1 = 158`**（設計時常數）
- `E = 158 − 127 − 29 = 2`
- `C_e = 2 + 15 = 17`
- 結果 `−1.01 × 2² = −5.0 = 0xC500`，**精確**

**這個例子完整驗證了 `ExpIn = bias + XLEN − 1` 這個公式。**

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| `E` | unbounded exponent，把數寫成 1.xxx × 2^E 的指數 |
| `ExpIn` | 有效的輸入 biased exponent（整數輸入為常數 `bias + XLEN − 1`） |
| `Bias_out` | 輸出格式的 bias（轉整數時定義為 1） |
| `C_e` | 轉換結果的指數，或轉整數時的移位量 |
| `LeadingZeros` | 待轉換數的前導零數（由 LZC 產生） |
| `LzcInFull` / `LzcIn` | 送進 LZC 的數（整數用 `TrimA`、浮點用 `X_m`） |
| `PosA` / `TrimA` | 整數輸入的絕對值／RV64 上截成 32 位元的版本 |
| `ShiftAmt` | 送給正規化移位器的移位量 |
| `ResSubnormUf` | 結果是 subnormal 或下溢的旗標 |
| `IntZero` | 截斷後的整數為 0 |
| `cvtshiftcalc` | 後處理器中負責轉換移位計算的模組 |

## 所以呢

這節示範了一種常見但不容易做到的設計手法：**把三種看起來不同的運算，用一組參數化的公式統一起來。**

關鍵在於三個定義上的巧思：
1. **`E`（unbounded exponent）**——一個對正規化浮點、subnormal、整數都成立的共同概念
2. **`ExpIn = bias + XLEN − 1`**——把「整數沒有指數」這件事變成一個設計時常數
3. **`Bias_out = 1`**——讓「轉整數的移位量」和「轉浮點的輸出指數」共用同一個訊號

結果是三種轉換共用一個 LZC、一個移位器、一組加法邏輯。這正是第 16.2.4 節說的「Fig. 16.14 顯示這些元件大部分可以在各種轉換之間共用」的具體內容。

下一節 F.4 是後處理——FMA、divsqrt、cvt 三個單元的結果最後都要經過它。
