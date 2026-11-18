# F.2 Floating-Point Division and Square Root

> 原書 p.784.e201–784.e258（Appendix F: Floating-Point Implementation）
> 涵蓋 F.2.1 Recurrence Division Algorithm、F.2.2 Radix-2、F.2.3 Radix-4、F.2.4 Divider Latency、F.2.5 Quotient Conversion、F.2.6 Early Termination、F.2.7 Subnormal Preprocessing、F.2.8 FP Postprocessing、F.2.9 Integer Division、F.2.10 Square Root、F.2.11 Unified Divide and Square Root、F.2.12 Multiplicative Algorithms

## 這節在講什麼

全書最長的一節（58 頁）。第 16.2.3 節只用兩頁講了 `fdivsqrt` 的概念，這裡給出完整的數學推導與實作。

核心主線是：**一個統一的遞迴硬體，同時服務浮點除法、浮點開根號、整數除法。** 這節逐步建立起這件事的每個環節——如何保證部分餘數不發散（containment）、如何用少數幾個位元猜出商數位元（QSLC 與 P-D 圖）、如何邊算邊轉換（OTFC）、如何提早結束、如何把開根號套進同一套硬體。

## 關鍵點

---

## F.2.1 遞迴除法：三個要解決的問題

### 為什麼簡單除法慢

> "This time is dominated by the CPA delay. The number of cycles could be reduced to n = N/k by performing k steps in each clock cycle. However, this increases the required cycle time"
> — p.784.e202

中文解釋：第 15.1.4 節的簡單除法每步要一次完整的 N 位元比較（式 F.6：`T_c ≥ t_CPA-N + t_buf + t_mux + t_reg`）。**進位傳遞加法（CPA）主宰了延遲**。

複製 k 份只是把 k 次 CPA 串起來（式 F.7），沒有真正變快。

### 三層解法

> "To eliminate the slow CPA, SRT dividers keep the partial remainder in carry-save redundant form. Specifically, an N-bit partial remainder W is represented using two redundant N-bit numbers WS and WC (sum and carry) such that W = WS + WC."
> — p.784.e202

中文解釋：**第一層：部分餘數用 carry-save 保存**，每步的加法只剩一個 full adder 延遲。

> "Radix-2 produces one quotient bit per step and thus requires approximately N steps... To reduce the latency, some dividers use a higher radix to select a quotient digit representing more than one bit of information. Radix-R = 2^r division produces r quotient bits per step and requires n = N/r steps."
> — p.784.e202

中文解釋：**第二層：提高基數**，一次出 r 個位元。

> "Experience has shown that the most practical redundant quotient digit sets are radix-2 {+1, 0, −1} and minimally redundant radix-4 {+2, +1, 0, −1, −2}. Higher-radix digits are expensive to select and to generate divisor multiples"
> — p.784.e202

中文解釋：**第三層：冗餘商數位元集合**——這是讓 carry-save 可行的關鍵（因為看不出正確的商，只能猜，猜錯用負的位元修正）。

### 遞迴關係式

> "W_j = R^{j+1}[X − DQ_j]"
> — p.784.e203（式 F.10）

中文解釋：推導出式 (F.11)：

```
W_j = R[W_{j−1} − Dq_j]
```

**「減掉 q_j 倍的除數，然後左移一位」**——這就是長除法的形式化。Fig. F.6 的演算法只有四行：初始化、QSLC 猜商、CSA 減、左移。

---

## F.2.1.3 Containment：整個推導的地基

**問題**：商數位元是猜的，怎麼保證部分餘數不會越來越大而發散？

> "Each quotient digit must be chosen to keep the partial remainder bounded: |W_j| ≤ M. To find the upper bound M, suppose W_{j−1} ≤ M. If W_{j−1} has the largest value (M); then the divider must choose the largest quotient digit q_j = a; hence W_j = (M − aD)R"
> — p.784.e205–e207

中文解釋：**這個推導很漂亮**——假設上界是 M，則 `M = (M − aD)R`，解出 `M = RDa/(R−1) = ρRD`，其中 **`ρ = a/(R−1)` 叫 redundancy factor（冗餘因子）**。

於是式 (F.12)：`|W_j| ≤ ρRD`。

再解出「選 `q_j = k` 時 `W_{j−1}` 必須落在什麼範圍」（式 F.14）：

```
L_k ≤ W_{j−1} ≤ U_k
L_k = (k − ρ)D
U_k = (k + ρ)D
```

**關鍵洞察**：

> "Observe that the region for q = 0 overlaps the bottom part of the region for q = 1 and the top part of the region for q = −1."
> — p.784.e209

中文解釋：**這些區間是重疊的**——所以同一個 `W_{j−1}` 可能有兩個合法的商數位元可選。**這個重疊就是「可以只看高位元來猜」的空間**。冗餘度越高（ρ 越大），重疊越多，猜測越容易。

### P-D 圖：把選擇問題畫出來

> "[Atkins68] developed the graphical method called P-D diagrams to show how to choose a quotient digit based on the Partial remainder and Divisor D."
> — p.784.e208

中文解釋：Fig. F.8（radix-2）、Fig. F.12（radix-4）。**橫軸是除數 D，縱軸是截斷後的部分餘數 W，畫出每個商數位元的合法區域**。重疊的深藍色區域就是「隨便選哪個都對」。

**設計 QSLC 就是「在重疊區裡畫一條水平線」**——水平線表示「只看 W，不看 D」，實作最便宜。

---

## F.2.2 Radix-2：只看四個位元

> "For radix-2, ρ = 1, R = 2, and 1 ≤ D < 2. Hence |W| ≤ ρRD < 4. We estimate W based on the most significant bits WC_msbs, WS_msbs. This truncation drops the fraction bits, so the error between the true and truncated partial remainders is in the range [0, 1)."
> — p.784.e208

中文解釋：**radix-2 只需要 4 個高位元，而且完全不用看除數 D**（式 F.17）：

```
q_j = +1   當 W_msbs > −1
q_j =  0   當 W_msbs = −1
q_j = −1   當 W_msbs < −1
```

**為什麼門檻是 −1 而不是 0？** 書上解釋得很清楚：

> "Other QSLC logic could have been used, such as that in Equation F.18. However, this logic would have had to bound the error on W_msbs to under 1 instead of 2 to remain within the containment bounds when W_msbs = 0. Hence, we would have needed 1 extra bit, keeping W_msbs in Q4.1 form instead, which is slower and uses more hardware."
> — p.784.e209

中文解釋：**用 −1 當門檻可以少一個位元**。這是一個具體的「多想一點省硬體」的例子。

### QSLC 的邏輯化簡

> "A brute force QSLC implementation would compute W_msbs = WC_msbs + WS_msbs on the top 4 bits of the partial remainder and then compare to −1... A faster and smaller implementation selects the quotient digit directly from WC_msbs and WS_msbs using carry chain logic to determine whether the sum is −1, larger, or smaller, without having to compute the exact sum first."
> — p.784.e210

中文解釋：**不算和，直接判斷「是不是等於 −1」**。式 (F.19)–(F.25)：

```
qz   = p₂ · p₁ · p₀              # 和是 1111 = −1（因為沒有進位輸入）
cout = g₂ + p₂(g₁ + p₁g₀)
sign = p₃ ⊕ cout
qp   = ~sign · ~qz               # 正且不等於 −1
qn   =  sign · ~qz               # 負且不等於 −1
```

Fig. F.11 顯示這只是十幾個閘。

---

## F.2.3 Radix-4：用查表換速度

> "The digit set for the radix-4 quotient digit q_j may be maximally redundant (a = 3, ρ = 1) {−3, −2, −1, 0, +1, +2, +3} or minimally redundant (a = 2, ρ = 2/3) {−2, −1, 0, +1, +2}. Minimally redundant divisor multiples are easier to form because each choice comes from shifting and/or negating D; maximally redundant multiples include 3D, which requires a CPA to compute 3D = D + 2D."
> — p.784.e213

中文解釋：**取捨很清楚**——最大冗餘讓 QSLC 容易（重疊區大），但需要算 `3D`（要一次 CPA，貴）。最小冗餘只需要 `D`、`2D`（移位）和它們的負值（反相），但 QSLC 難。**大多數實作選最小冗餘。**

### 選擇常數與 P-D 圖的精細分析

> "Careful inspection of the P-D diagram shows that we need the 7 most significant partial remainder bits W_msbs in Q4.3 format and the 3 most significant fractional divisor bits in U1.3 format to distinguish quotient digits."
> — p.784.e214

中文解釋：radix-4 **必須看除數**（radix-2 不用）。所以 QSLC 變成一張二維表：Table F.8（所有可行的選擇常數）、Table F.9（挑選出的偏好常數）。

**選常數的原則**（Table F.9）：

> "When multiple viable constants exist for a cell, pick constants to minimize the selection complexity (and area and delay). A straightforward option, as shown in Table F.9, is to pick constants that are a large power of two or that are equal in adjacent cells when possible."
> — p.784.e216

中文解釋：**選 2 的冪次或讓相鄰格子相同**——前者讓比較器變簡單（只要看幾個位元），後者讓查表的邏輯能合併。這是典型的「在合法範圍內挑最好實作的那個」。

**一個重要的伏筆**（邊欄）：

> "Remarkably, the selection constants in Table F.9 also work for square root, as shown in Section F.2.10."
> — p.784.e216

中文解釋：**同一張表能同時用於除法和開根號**——這是 F.2.11 統一硬體的關鍵。

### 三種實作方式

> "The 10-input logic function may be implemented in various ways. The simplest is to give a SystemVerilog table of Q as a function of W_msbs and D_msbs and allow the logic optimization tool to map it onto the best implementation for a give technology. That implementation could be a minimal sum-of-products equation, a multilevel network of NANDs and NORs on an ASIC, or a block RAM on an FPGA."
> — p.784.e217

中文解釋：**把真值表交給合成工具**是最簡單的做法，而且能自動適應不同製程（ASIC 用邏輯閘、FPGA 用 block RAM）。

> "Because D is constant during division, another option is to pick four selection constants based on D_msbs during preprocessing. Then the QSLC simply compares W_msbs to the four constants. ... This is faster than the design from Fig. F.15 but takes more area [Harris23]."
> — p.784.e217

中文解釋：**第二種做法利用了「除數在整個除法過程中不變」**——預處理時就把四個常數挑好，之後每步只要比較。Wally 現在用的是比較器版本（`fdivsqrtqsel4cmp.sv`）。

---

## F.2.4–F.2.5 延遲與商數轉換

### 延遲公式

> "n + 1 = k⌈(r + b)/(rk)⌉"
> — p.784.e219（式 F.28）

中文解釋：`b` 是需要的小數位元數（至少 `N_f + 2`，含 guard 和 round），`r` 是每位的位元數，`k` 是複製份數。

> "(n + 1) may not be divisible by k. It is harmless to produce more than the minimal number of quotient bits; the least significant bits are OR'd into the sticky bit."
> — p.784.e219

中文解釋：**多算幾位無害**——多出來的位元 OR 進 sticky bit。這比「精確控制步數」簡單得多。

### OTFC：避免最後一次 CPA

**問題**：商數位元有負值，直接拼接不行。

> "A simple implementation is to subtract the weighted negative quotient digits from the positive digits... On-the-fly conversion (OTFC) saves the cost and delay of this subtractor by accumulating the quotient digits as they appear on each step."
> — p.784.e220

中文解釋：**核心技巧——同時維護兩個版本**：

> "To get around this addition, we keep track of two versions of the running quotient, Q and QM. At any given step, QM = Q − 1."
> — p.784.e220

中文解釋：`QM = Q − 1` 永遠成立。這樣：
- `q_j > 0`：`Q_j = Q_{j−1}·R + q_j`（直接拼接）
- `q_j < 0`：`Q_j = QM_{j−1}·R + (R + q_j)`——**因為 `R + q_j` 是正的，所以還是拼接！**

**負數位元被轉換成「在 QM 上拼接一個正數」**，完全不需要減法。代價是多一個暫存器。

Fig. F.17（radix-2）和 Fig. F.18（radix-4）簡化成純粹的移位暫存器——**每步只是塞進 1 或 2 個位元**。

---

## F.2.6 Early Termination：一個微妙的 radix-2 問題

> "If the partial remainder at the start of step j is 0, the quotient is exact, and no more bits need to be computed."
> — p.784.e222

中文解釋：檢查 `WS + WC == 0`。但用 CPA 比較太慢，所以用式 (F.30) 的技巧：

> "A more efficient approach is to check whether a column's carry-in required to produce a sum of 0 is equal to the carry-out of the previous column when its sum is 0. If this is true for all columns, the result must be 0."
> — p.784.e222–e223

中文解釋：**不算和，只檢查「每一欄要產生 0 所需的進位輸入」和「前一欄產生 0 時的進位輸出」是否一致**——純組合邏輯，N 個 2 輸入 XOR/OR 加一次相等比較。

### radix-2 的陷阱

> "Radix-2 digit selection introduces another problem that when the truncated partial remainder is 0, the next digit selected will be 1 rather than 0, according to Equation F.17. This produces a partial remainder of W_j = (W_{j−1} − q_jD) << r = (0 − D) << 1 = −2D. Each subsequent digit will be −1, and the partial remainder will remain −2D. This is a problem because it will set the Inexact flag and round improperly even though the division is exact."
> — p.784.e223

中文解釋：**這是一個真實的 bug 陷阱**。因為式 (F.17) 的門檻是 −1（前面說為了省一個位元），所以 `W = 0` 時會選 `q = +1`，把餘數變成 `−2D`，然後永遠卡在那裡。

**後果不是答案錯，而是 Inexact 旗標錯設、捨入錯誤**——這種 bug 只有 TestFloat（第 16.5 節）抓得到。

修法：

> "The problem can be solved by modifying the early termination check to look for either WS + WC = 0 or WS + WC + 2D = 0."
> — p.784.e223

中文解釋：多檢查一個條件，用 3:2 CSA 加上既有的比較器。**注意這個問題在 k > 1 時更隱蔽**——第一階段已經精確了，但要到第二階段結束才檢查，那時餘數已經是 `−2D`。

### 一個安全性註記

> "With early termination, the latency of division depends on the operands. Some processors could potentially deduce information about the operands by measuring this latency. A malicious program could potentially deduce information about the operands by measuring this latency. Some processors, especially those running cryptographic programs, avoid early termination to prevent leaking this side-channel information."
> — p.784.e223（邊欄）

中文解釋：**提早結束是一個 side channel**——這正是第 18 章 Zkt 擴充把除法排除在外的原因（「this list excludes division and remainder」）。效能與安全的直接衝突。

---

## F.2.7–F.2.8 Subnormal 與後處理

> "Subnormal floating-point numbers D and/or X must be left shifted by ℓ and m to the range [1, 2) to apply the recurrence algorithm."
> — p.784.e223

中文解釋：這就是第 16.2.3 節說的「preshifter」。**而且因為 subnormal 已經需要它，整數除法蹭一下不用再加硬體**——這是統一硬體的第一個理由。

> "Because both X and D could be subnormal, the divider can either provide two copies of this expensive hardware or use two cycles and one copy of the hardware."
> — p.784.e223–e224

中文解釋：又一個面積/延遲取捨。

後處理（F.2.8）與其他浮點運算共用（第 16.2.6 節）：符號、指數、正規化、捨入、特例、旗標。指數的式 (F.32)：`Q_e = X_e − D_e + bias + ℓ − m`——**subnormal 的預移量直接加進指數**。

---

## F.2.9 整數除法：借用浮點硬體

> "Recurrence division aims for more speed by looking at the most significant bits of a redundant partial remainder. Moreover, the core saves area by reusing the floating-point divider for integer division."
> — p.784.e225

中文解釋：要把整數除法塞進浮點硬體，前處理要做四件事（Fig. F.21）：
1. W64 指令截成 32 位元
2. 有號數取絕對值
3. **用 LZC 數前導零，左移正規化到 [1, 2)**
4. 記下移位量 `ℓ`、`m` 供後處理還原

### 一個容易忽略的細節：right shift

> "Unlike floating-point division, it is problematic for integer division to produce extra trailing quotient digits because the extra steps also affect the residual, changing the remainder. To prevent this problem, integer division right shifts X to introduce leading zeros in X and Q such that the number of digits is exactly right"
> — p.784.e227

中文解釋：**浮點多算幾位無害（進 sticky），整數多算會弄錯餘數**。所以要先右移 `rightshiftx = rk − 1 − ((r + p − 1) mod rk)`（式 F.34），讓總位元數剛好是 `rk` 的倍數。

Fig. F.22 給出完整的整數除法演算法（前處理 + 迭代 + 後處理）。Table F.12 列出三個特例：`|A| << |B|`（商為 0）、除以 0（商全 1、餘數 A）、有號溢位（最負數 ÷ −1）。

> "The special case of the most negative number divided by −1 produces the necessary QUOT and REM with the regular division path, so the hardware doesn't have to check for it."
> — p.784.e229（邊欄）

中文解釋：**有號溢位不用特別處理，一般路徑算出來剛好就對**——很幸運的巧合。

---

## F.2.10 開根號：同一套硬體的另一半

### 為什麼範圍是 [1/4, 1)

> "Plausible choices of significand ranges are [1, 4), [1/2, 2), or [1/4, 1). [1/2, 2) complicates digit selection because the square root may or may not have a leading 1 in the integer digit. [1, 4) is also troublesome because the square root is in the range [1, 2), so the integer digit may need to be either 1 or 2 given that fraction digits may be negative. Hence, square root generally preshifts the significand of X right by 1 or 2 bits to the range [1/4, 1) and increments the exponent of X by 1 or 2 so that the exponent is even."
> — p.784.e234

中文解釋：**選 [1/4, 1) 是因為它讓結果 S 落在 [1/2, 1)，整數位元一定是 0**——digit selection 最單純。其他兩個選擇都會讓整數位元不確定。

指數要偶數才能除以 2，所以右移 1 或 2 位調整。

### 推導：和除法幾乎一樣

> "W_j = R^{j+1}(X − S_j²)"
> — p.784.e234（式 F.37）

中文解釋：**把除法的 `X − DQ_j` 換成 `X − S_j²`**。展開後得到式 (F.38)：

```
W_j = R[W_{j−1} − (2S_{j−1}s_j + s_j²R^{−j})]
```

定義 `F_j = −(2S_{j−1}s_j + s_j²R^{−j})`，就得到和除法一模一樣的形式（式 F.40）：

```
W_j = R[W_{j−1} + F_j]
```

**差別只在「加什麼」**——除法加 `−q_jD`（固定的除數倍數），開根號加 `F_j`（依賴於目前的近似值 `S_{j−1}` 和步數 `j`）。

### 為什麼同一張選擇表能用

> "Remarkably, a careful case analysis [Harris23] shows that the square root digit can be selected according to Table F.9. In brief, in the small j cases, the possibilities for S_{j−1} or s_j are restricted, making digit selection easier even though the additional term is larger. For j > 3, the selection equation is almost identical to Equation F.26 because the 4^{−j} term is tiny."
> — p.784.e238

中文解釋：**這是整節最漂亮的結果**。式 (F.52) 的開根號選擇邊界比除法的式 (F.26) 多一個 `4^{−j}` 項——但：
- **j 大時那項小到可以忽略**
- **j 小時因為 `S_{j−1}` 的可能值受限，反而更好選**

所以**同一張 Table F.9 兩邊都能用**。只要把式 (F.26) 的 `D` 換成 `2S_{j−1}`（Table F.13 的 ASEL 區塊負責挑）。

### FSEL：用溫度計碼消掉變動移位

**問題**：`F_j` 裡的 `K_j = R^{−j}` 每步位置都在變，而且要做二補數減法。

> "FSEL cleverly avoids this carry propagate addition by also using SM_{j−1} = S_{j−1} − R^{−(j−1)}"
> — p.784.e239

中文解釋：**又是 OTFC 那招——維護 `S` 和 `SM = S − ulp` 兩份**，靠恆等式 `−S = ~S + 2^{−r(j−1)}` 把減法變成反相。Table F.14/F.15 給出每個 `s_j` 對應的位元字串（全是反相和附加常數，**沒有加法器**）。

**但還有變動移位的問題**：

> "Unfortunately, FSEL still involves shifting strings of bits by a variable amount j, which is a costly operation. To reduce FSEL to simple Boolean logic, we recode the walking 1 K_j in thermometer format: C_j = −K_j = −R^{−j} = 1111.11..11"
> — p.784.e241（式 F.53）

中文解釋：**溫度計碼（`C_j` 是一串 1）可以用移位暫存器每步右移 r 位、從左邊補 1 來產生**——不需要變動移位器。然後 Table F.16/F.17 把位元字串表達成 `S_{j−1}`、`SM_{j−1}`、`C_j` 的**常數移位 + AND/OR/NOT**。

Example F.20 完整走了一遍：`F_3 = (~0.11101 << 2) & (1111.111111 << 2) = 1100.101111 & 1111.111100 = 1100.101100`——**全部是移位和邏輯運算，零加法器**。

### SOTFC 與開根號的 early termination

SOTFC（Fig. F.26–F.28）和除法的 OTFC 幾乎一樣，差別在**把位元放在最高位而非最低位**（因為 `S_{j−1}` 要立刻用於下一步的 digit selection）。

開根號也有和 radix-2 除法類似的 early termination 陷阱（式 F.54–F.55），要多檢查 `W_j + 2(2S_j − K_j) = 0`。

---

## F.2.11 統一：三種運算一套硬體

> "With small modifications, integer divide, floating-point divide, and square root can all share the same datapath. The preprocessing shifters are used both to prescale integer operands and to prenormalize deormalized floating-point operands. The digit selection table is shared by division and square root. The addend generation differs for divide (q_jD) and square root (F_j). UOTFC places the result digits in the most significant bits, as required for square root."
> — p.784.e249

中文解釋：**三個共用、一個不同**：

| 元件 | 三種運算共用嗎 |
| --- | --- |
| 前處理移位器（LZC + shifter） | **共用**（整數正規化 / subnormal 正規化 / 開根號右移） |
| digit selection 表 | **共用**（Table F.9） |
| CSA、暫存器、移位 | **共用** |
| addend 產生 | **不同**（`−q_jD` vs `F_j`） |
| OTFC | 統一成 UOTFC（放最高位） |
| 後處理 | **共用**（正規化、捨入、特例、旗標、變動右移器） |

> "Sharing the three costly input and result variable shifters is a significant advantage of unifying integer and floating-point division."
> — p.784.e253

中文解釋：**三個變動移位器（最貴的部分）全部共用**——這就是統一設計的主要回報。

Fig. F.32 是完整的統一演算法（三頁），Fig. F.35 是完整的方塊圖（`fdivsqrtpreproc` + `fdivsqrtiter` + `fdivsqrtpostproc`）。

### 延遲

> "The unit requires Cycles + 2 cycles for the entire operation: one cycle of preprocessing, Cycles to compute the result U, and one cycle for postprocessing."
> — p.784.e255–e257

中文解釋：這正是第 16.6.4 節的式 (16.8)。Example F.26 算出 rv64gc（radix-4、k=4）需要 **9 個 cycle** 產生 72 位元。

> "The present FSM implementation adds an extra cycle in a DONE state that may be possible to optimize away."
> — p.784.e257

中文解釋：**書上誠實指出還有一個可以省的 cycle**。

---

## F.2.12 另一條路：Newton-Raphson

> "A completely different way of performing division and square root is with the Newton-Raphson method of approximation. ... The method generally converges quadratically, meaning it doubles the number of accurate bits on each step."
> — p.784.e257

中文解釋：**二次收斂**——每步準確位元數翻倍，聽起來比遞迴除法（每步固定 r 位）好得多。

算倒數 `R = 1/D`（式 F.60）：`R_{n+1} = R_n(2 − DR_n)`——**每步兩次 FMA**。從查表得到 8–14 位初值，雙精度只要 2–3 步。

算倒數平方根（式 F.61）：`A_{n+1} = (A_n/2)(3 − XA_n²)`——每步三次 FMA。

### 為什麼大多數處理器不用它

> "These multiplicative algorithms are tempting in an architecture such as Itanium or POWER with a high-performance FMA and a few extra bits to facilitate correct rounding [Cornea04]. Each FMA operation depends on the previous one, and a fast FMA has a many-cycle latency, so Itanium requires 24−40 cycles for various precisions of divide and square root, though the throughput for concurrent operations can be much higher."
> — p.784.e257–e258

中文解釋：**關鍵問題是「每步依賴前一步」**。FMA 本身有好幾個 cycle 的延遲，鏈起來就變成 24–40 cycles——**比遞迴除法還慢**（Wally 只要 9）。

而且：

> "Nevertheless, FMA is power hungry, and the extra bits for rounding may not be available to software-based algorithms. Most processors presently use recurrence division [Liu12]."
> — p.784.e258

中文解釋：三個缺點——**慢（延遲鏈）、耗電（FMA 很大）、難正確捨入（需要額外的位元）**。所以現代處理器主流仍是遞迴除法。

**但注意那句「throughput for concurrent operations can be much higher」**——如果有很多獨立的除法要做，FMA 可以管線化，吞吐量會贏。這是延遲 vs 吞吐量的典型取捨。

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| SRT division | Sweeney/Robertson/Tocher 的遞迴除法 |
| partial remainder / residual | 部分餘數（除法）／殘差（開根號） |
| carry-save redundant form | `W = WS + WC` 的冗餘表示 |
| containment | 保證部分餘數有界不發散 |
| ρ (redundancy factor) | `a/(R−1)`，冗餘因子 |
| maximally / minimally redundant | 最大／最小冗餘數位集合 |
| P-D diagram | 以 Partial remainder 與 Divisor 為軸的商數選擇圖 |
| QSLC / SSLC / USLC | 商／開根號／統一的數位選擇邏輯 |
| selection constant m_k | QSLC 比較用的門檻常數 |
| OTFC / SOTFC / UOTFC | on-the-fly conversion，除法／開根號／統一版 |
| `Q` / `QM` | 商與商減一，OTFC 同時維護的兩份 |
| early termination | 餘數為 0 時提早結束 |
| side-channel | 靠延遲等側通道洩漏運算元資訊 |
| ASEL / FSEL / FGEN | 開根號的 addend 選擇與產生邏輯 |
| thermometer code C_j | `−K_j` 的溫度計編碼，消掉變動移位 |
| `K_j` | `R^{−j}`，開根號中滑動的 1 |
| `S` / `SM` | 開根號近似值與它減一個 ulp |
| `rightshiftx` | 整數除法的預先右移量 |
| `ℓ` / `m` | 被除數／除數的前導零數 |
| W64 | RV64 的 32 位元整數運算指令 |
| Newton-Raphson | 二次收斂的迭代逼近法 |
| Goldschmidt's algorithm | Newton-Raphson 的平行化變體 |
| quadratic convergence | 二次收斂，每步準確位元數加倍 |

## 所以呢

這一節的主線是「**一套硬體服務三種運算**」，而讓它成立的是一連串環環相扣的設計：

1. **containment 分析**給出「哪些商數位元是合法的」——重疊區就是猜測的空間
2. **P-D 圖**把選擇問題視覺化，讓人能在重疊區裡挑最好實作的門檻
3. **OTFC** 用「同時維護 Q 和 Q−1」消掉最後一次 CPA
4. **溫度計碼**消掉開根號的變動移位
5. **Table F.9 對除法和開根號同時有效**（一個意外但關鍵的數學事實）——這是統一的關鍵

反覆出現的手法是**「多維護一份資料，換掉一次加法」**：`Q`/`QM`、`S`/`SM`、`WS`/`WC`。這比加一個加法器便宜得多。

最後 F.2.12 給了一個有價值的反例：Newton-Raphson 在數學上收斂更快（二次 vs 線性），但因為每步依賴前一步而 FMA 延遲長，實際上更慢——**演算法的漸近複雜度和硬體的實際延遲是兩回事**。

下一節 F.3 講轉換單元。
