# Basic Computer Architecture 第十支影片完整筆記：ARM 的分支、記憶體指令與編碼（Chapter 4, Part 2/2）

> **課程**：Basic Computer Architecture — Chapt 4: ARM Assembly, Part 2/2（播放清單第 10 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（1:25:08，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 Ch4 §4.2.6–4.5（p.148–170）
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、GNU as `arm-none-eabi-as`、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 簡單的分支：b、beq、bne](#1-簡單的分支bbeqbne)
- [2. 16 個條件碼](#2-16-個條件碼)
- [3. 例子：階乘](#3-例子階乘)
- [4. bl：branch and link](#4-blbranch-and-link)
- [5. 從函式回傳：mov pc, lr 與 bx lr](#5-從函式回傳mov-pc-lr-與-bx-lr)
- [6. 條件執行（predicated instructions）](#6-條件執行predicated-instructions)
- [7. 例子：數 1 的個數](#7-例子數-1-的個數)
- [8. ldr 與 str 的定址模式](#8-ldr-與-str-的定址模式)
- [9. 例子：陣列加總](#9-例子陣列加總)
- [10. Pre-indexed 與 post-indexed](#10-pre-indexed-與-post-indexed)
- [11. stmfd 與 ldmfd：一次存好幾個暫存器](#11-stmfd-與-ldmfd一次存好幾個暫存器)
- [12. 例子：遞迴的 power](#12-例子遞迴的-power)
- [13. 編碼：一般格式](#13-編碼一般格式)
- [14. 資料處理指令的編碼與 12 位元立即值](#14-資料處理指令的編碼與-12-位元立即值)
- [15. Shifter operand 的編碼](#15-shifter-operand-的編碼)
- [16. Load／store 的編碼：I P U B W L](#16-loadstore-的編碼i-p-u-b-w-l)
- [17. 分支的編碼與 PC + 8](#17-分支的編碼與-pc--8)
- [18. 【實測】](#18-實測)
- [19. 一頁速查表](#19-一頁速查表)
- [20. 與後續講次的連結](#20-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

上一講（第 9 支）講了 ARM 的暫存器、資料處理指令、cmp 和 CPSR flags。這一講講 ARM「好玩的部分」：

```
分支：b + 條件碼（16 種）、bl、bx
條件執行：任何指令都能加條件碼（addeq、moveq…）
記憶體：ldr/str 的各種定址模式、pre/post-indexed、stmfd/ldmfd
編碼：cond(4) + type(2) + …；12 位元立即值 = 8 位元 payload 右旋 2×rot；分支目標 = PC + 8 + offset×4
```

和 SimpleRisc 比，ARM 用很多「技巧」把程式寫短。

---

## 1. 簡單的分支：b、beq、bne

[00:43]–[02:58]　課本 §4.2.6、Table 4.9（p.148）

- 和 SimpleRisc 一樣，格式是**分支指令 ＋ label**。label 是分支目標的第一條指令。

| 指令 | 意思 |
|---|---|
| `b .foo` | 無條件跳到 .foo |
| `beq .foo` | 最後一個**設定 flags 的指令**（flag setting instruction）結果是相等，也就是 **Z flag = 1**，就跳 |
| `bne .foo` | Z flag = 0 就跳 |

- 「flag setting instruction」要當成一個詞：通常是 cmp，或是加了 `s` 後綴的指令（上一講）。

---

## 2. 16 個條件碼

[02:58]–[12:09]　課本 Table 4.12（p.153）

ARM 的分支格式是 **b ＋ 條件碼**（beq、bne、bge、bgt……），所有條件碼都是**後綴**：

| 編號 | 後綴 | 意思 | Flag 狀態 |
|---|---|---|---|
| 0 | eq | 相等 | Z = 1 |
| 1 | ne | 不相等 | Z = 0 |
| 2 | cs／hs | carry set／**無號**大於或等於 | C = 1 |
| 3 | cc／lo | carry clear／**無號**小於 | C = 0 |
| 4 | mi | 負數（minus） | N = 1 |
| 5 | pl | 正數或零（plus） | N = 0 |
| 6 | vs | overflow | V = 1 |
| 7 | vc | 沒有 overflow | V = 0 |
| 8 | hi | **無號**大於 | (C = 1) ∧ (Z = 0) |
| 9 | ls | **無號**小於或等於 | (C = 0) ∨ (Z = 1) |
| 10 | ge | **有號**大於或等於 | N = 0 |
| 11 | lt | **有號**小於 | N = 1 |
| 12 | gt | **有號**大於 | (Z = 0) ∧ (N = 0) |
| 13 | le | **有號**小於或等於 | (Z = 1) ∨ (N = 1) |
| 14 | al | always（預設，可以不寫） | |
| 15 | — | 保留 | |

**無號比較為什麼看 C（carry）**（[04:50]）：

- 比較 a 和 b 就是算 a − b。
- 如果 a ≥ b（無號），減法**不需要借位**（borrow = false），所以 **carry = 1**。
- 如果 a < b，要借位，**carry = 0**。carry 和 borrow 的意思**相反**。
- 【課本補充】課本把「無號減法借位」也叫做 unsigned underflow。

**hi 和 ls**：

- hi（無號大於）= hs（≥）而且不相等：C = 1 且 Z = 0。
- ls（無號小於或等於）= lo（<）或相等：C = 0 或 Z = 1。
- 【口頭】老師講 ls 時先講錯，馬上更正：「C = 0 表示 a 嚴格小於 b，再 OR 上相等，就是小於或等於。」

**有號比較看 N**：

- ge：a − b 不是負數 → N = 0。
- gt：再加上不相等 → Z = 0 且 N = 0。
- le：Z = 1 或 N = 1。

⚠️ 兩個要注意的地方：

- 影片 [07:08] 解釋 vs（overflow）時舉的例子是「乘法的結果超過 32 位元」。但 ARM 的 **V flag** 是**有號加減法**的 overflow（[第 4 支的 overflow 定理](lecture04_note.md#10-overflow-定理)：同號相加、結果異號）。
- 課本 Table 4.12 的 ge、lt、gt、le **只看 N，沒有考慮 overflow**。課本自己承認，留給讀者當習題。真正的 ARM 是看 **N == V**（ge）、N ≠ V（lt）等，這樣 overflow 時才不會判斷錯。

- 【口頭】條件碼 14 是 al（always），是預設值，所以 `bal` 和 `b` 一樣。15 保留給以後擴充指令集用。

---

## 3. 例子：階乘

[12:09]–[15:08]　課本 Example 44（p.149）

計算 r0 的階乘（r0 > 1），結果放 r1：

```c
int prod = 1;
for (idx = 1; idx <= val; idx++)
	prod = prod * idx;
```

```
	mov r1, #1        /* prod = 1 */
	mov r3, #1        /* idx = 1 */
.loop:
	mul r1, r3, r1    /* prod = prod * idx */
	cmp r3, r0        /* compare idx, with the input (num) */
	add r3, r3, #1    /* idx ++ */
	bne .loop         /* loop condition */
```

- ARM 的立即值前面要加 `#`。
- 因為 r0 > 1，可以**先乘再檢查**。
- 關鍵：cmp 在 add **前面**，但 bne 在 add **後面**。因為 **add 不會設定 flags**，bne 看到的還是 cmp 的結果。當 idx == num 時，乘完就不用再跑一圈，bne 不跳，離開迴圈。
- ⚠️ 課本內文說「r3 是一個從 **0** 開始的計數器」，但程式碼是 `mov r3, #1`。以程式碼為準。

【課本補充】Example 45：判斷 r0 是不是完全平方數。從 1 開始，一個一個算平方，和 r0 比較。

---

## 4. bl：branch and link

[15:08]–[18:56]　課本 §4.2.7、Table 4.10（p.150）

| 指令 | 意思 |
|---|---|
| `bl .foo` | ① 無條件跳到 .foo；② 把下一條指令的位址（**PC + 4**）存進 **lr** |

- 就是 SimpleRisc 的 **call**。
- **lr**（link register）就是 ARM 的 return address register，也就是 **r14**。
- 【課本補充】ARM 沒有特別的方法標出函式的開頭：任何指令都可以是函式的開頭，只要有 label 就好。

```c
int foo() { return 2; }
void main() { int x = 3; int y = x + foo(); }
```

```
foo:
	mov r0, #2
	mov pc, lr        /* 回傳 */
main:
	mov r1, #3        /* x = 3 */
	bl foo            /* invoke foo */
	add r2, r0, r1    /* y = x + foo() */
```

- 有趣的是：**ARM 沒有 ret 指令**。因為 **PC 和 lr 都是程式設計師看得到的暫存器**（PC 是 r15），把 lr 搬到 PC 就是回傳。
- 回傳值放在 r0。

---

## 5. 從函式回傳：mov pc, lr 與 bx lr

[18:56]–[21:38]　課本 Table 4.11（p.152）

| 指令 | 意思 |
|---|---|
| `bx r2` | 無條件跳到 **r2 裡的位址** |

- 和 b 很像，但 b 接 label，bx 接**暫存器**。
- **現在建議用 `bx lr` 回傳**，不要用 `mov pc, lr`。
  - 【口頭】`mov pc, lr` 只該用在**沒有 bx 指令**的 ARM 處理器上；大部分 ARM 處理器都有 bx。

```
foo:
	mov r0, #2
	bx lr
```

---

## 6. 條件執行（predicated instructions）

[21:38]–[23:01]　課本 §4.2.8（p.152–154）

- 條件碼不只能接在 b 後面。**很多一般指令**（add、sub、mul……）也可以加條件碼：

| 指令 | 意思 |
|---|---|
| `addeq r4, r4, #1` | 只有 eq 成立（Z = 1）時才做加法 |
| `addmi …` | 只有 mi 成立時才做 |
| `subne …` | 只有 ne 成立時才做 |

- 這種指令叫 **predicated instruction**（有前提條件的指令）：條件成立就正常執行，**不成立就像 nop 一樣**什麼都不做。
- 【課本補充】課本說：到 Ch10 會看到，用這種條件指令可以**提升進階處理器的效能**（因為少了分支）。

---

## 7. 例子：數 1 的個數

[23:01]–[27:37]　課本 Example 47（p.154）

數 r1 這個 32 位元數字的二進位裡有幾個 1，結果放 r4。

- 【口頭】例：4 位元的 `0101` 有 2 個 1；`1101` 有 3 個 1。

```
	mov r2, #1        /* idx = 1 */
	mov r4, #0        /* count = 0 */
.loop:
	and r3, r1, #1    /* 取出 LSB */
	cmp r3, #1
	addeq r4, r4, #1  /* LSB 是 1 才加 1 */
	mov r1, r1, lsr #1 /* 右移一位，準備下一輪 */
	add r2, r2, #1
	cmp r2, #32
	ble .loop
```

- **取出 LSB**：和 1 做 AND，上面 31 位都被清掉，只留最低位。
- **addeq**：只有 LSB 是 1 時才把 count 加 1。
- 右移要用 **lsr**（邏輯右移），**不是 asr**：不要複製符號位，不然負數會一直移進 1。
- idx 從 1 到 32，跑 32 次；idx 變成 33 時 ble 不跳，離開。
- 【口頭】**重點是 addeq**：如果沒有它，cmp 之後要再加一個分支，決定要不要加 1。現在整段折疊成一條指令。

---

## 8. ldr 與 str 的定址模式

[27:37]–[32:51]　課本 §4.2.9、Table 4.13（p.155–157）

| 定址模式 | 例子 | 意思 |
|---|---|---|
| register-indirect | `ldr r1, [r0]` | r1 ← [r0] |
| base-offset | `ldr r1, [r0, #4]` | r1 ← [r0 + 4] |
| base-index | `ldr r1, [r0, r2]` | r1 ← [r0 + r2] |
| base-scaled-index | `ldr r1, [r0, r2, lsl #2]` | r1 ← [r0 + (r2 << 2)] |

- `[ ]` 裡面就是位址。
- `str` 完全對稱：`str r1, [r0]` 是 [r0] ← r1。
- ⚠️ 老師講 `[r0, #4]` 時先說成「base index」，馬上改口是 **base-offset**。
- **Base-scaled-index**：r0 是 base，r2 是 index，index 先**位移**再加。
  - 用途：陣列。每個元素 4 byte，index 0、1、2… 要變成 byte offset 0、4、8…，就是乘 4 = 左移 2。
  - `r2, lsl #2` 叫 **shifter operand**（上一講），位移可以是 **lsl、lsr、asr、ror** 四種。
- 【課本補充】base-offset 也有人叫 displacement addressing mode。

【課本補充】Table 4.14：載入／存入 1 byte 和 2 byte（half-word）：

| 指令 | 意思 |
|---|---|
| `ldrb` | 載入 1 個**無號** byte，高 24 位補 0 |
| `ldrh` | 載入 2 個**無號** byte |
| `ldrsb` | 載入 1 個**有號** byte，sign extension |
| `ldrsh` | 載入 2 個**有號** byte |
| `strb`／`strh` | 存 1／2 byte |

存的時候不需要 sign extension（只存低位），所以沒有 strsb。

---

## 9. 例子：陣列加總

[32:51]–[37:30]　課本 Example 48（p.158）

```c
for (idx = 0; idx < 100; idx++)
	sum = sum + a[idx];
```

```
	/* base address of array a in r0 */
	mov r1, #0                /* sum = 0 */
	mov r2, #0                /* idx = 0 */
.loop:
	ldr r3, [r0, r2, lsl #2]  /* r3 = a[idx]，位址 = r0 + idx × 4 */
	add r2, r2, #1            /* idx ++ */
	add r1, r1, r3            /* sum += a[idx] */
	cmp r2, #100              /* loop condition */
	bne .loop
```

- 【口頭】C 只有 4、5 行，組合語言 7 行，「其實不差」。能這麼精簡，全靠那條 ldr：一個式子就算出 r0 + r2 × 4。

---

## 10. Pre-indexed 與 post-indexed

[37:30]–[49:05]　課本 §4.3.1（p.158–159）

**觀察**：存取陣列時，常見的模式是「讀一個元素，然後 index 加一」。能不能合成一條指令？

**Post-indexed**（像 C 的 `i++`：先用，再加）：方括號只包住 base

| 指令 | 意思 |
|---|---|
| `ldr r3, [r0], #4` | r3 ← [r0]；然後 r0 ← r0 + 4 |
| `ldr r3, [r0], r2, lsl #2` | r3 ← [r0]；然後 r0 ← r0 + (r2 << 2) |

**Pre-indexed**（像 C 的 `++i`：先加，再用）：後面加 **`!`**

| 指令 | 意思 |
|---|---|
| `ldr r0, [r1, #4]!` | 先 r1 ← r1 + 4；再 r0 ← [r1] |
| `ldr r3, [r0, r1, lsl #2]!` | r3 ← [r0 + (r1 << 2)]；r0 ← r0 + (r1 << 2) |

- `!` 前面的位址可以是任何形式：register-indirect、base-offset、base-index、base-scaled-index。
- load 和 store 都可以用 pre／post-indexed。

**用 post-indexed 改寫陣列加總**（[43:58]，課本 Example 49）：

```
	/* base address of array a in r0 */
	mov r1, #0          /* sum = 0 */
	add r4, r0, #400    /* address of a[100] */
.loop:
	ldr r3, [r0], #4    /* r3 = [r0]; r0 = r0 + 4 */
	add r1, r1, r3      /* sum += a[idx] */
	cmp r0, r4
	bne .loop
```

- **完全不需要 index 變數**：把 r0 當成一個**移動的指標**，一次往後移一個元素。
- 迴圈結束條件：算出陣列**後面第一個 byte** 的位址（a[100]，一個不合法的位址）= r0 + 100 × 4 = r0 + 400，存在 r4。r0 走到 r4 就停。
  - ⚠️ 老師口誤說成「把 r0 設成 a[100] 的位址」，馬上更正是 **r4**。
- 迴圈從 5 條變 4 條，跑 100 次：動態指令從 500 條變 400 條，**少了 20%**。
- 【課本補充】大部分處理器核心用 auto-update 定址模式**不會多花時間**，所以效能也提升了。

【課本補充】課本也講了 **struct**（Table 4.15）：struct 可以當成陣列，有 base address，每個元素有自己的 offset。但元素大小不同，而且 compiler 會**對齊**（align）：

```c
struct Container { int a; int b; char c; short d; int e; };
```

| 元素 | offset |
|---|---|
| a（int） | 0 |
| b（int） | 4 |
| c（char） | 8 |
| d（short） | **10**（不是 9：對齊到 2 byte） |
| e（int） | 12 |

---

## 11. stmfd 與 ldmfd：一次存好幾個暫存器

[49:05]–[52:14]　課本 §4.3.2、Table 4.16（p.160–161）

- 呼叫函式前常要 spill 好幾個暫存器，回來後再恢復。一個一個 str／ldr 要很多指令。ARM 有兩條指令專門做這件事：

| 指令 | 意思 |
|---|---|
| `stmfd sp!, {暫存器列表}` | 把暫存器 **push** 到 stack；更新 sp |
| `ldmfd sp!, {暫存器列表}` | 從 stack **pop** 到暫存器；更新 sp |

- `!`（像 pre-indexed）：指令結束時**更新 sp**。例：push 3 個暫存器，sp 減 12。
- **順序**：
  - stmfd：硬體先把暫存器**排序**，**由大到小**依序 push（stack 往下長）。例：{r4, r10} → 先 push r10，再 push r4。
  - ldmfd：從 stack 頂端開始 pop，**由小到大**指定給暫存器。先 pop r4，再 pop r10。
- 【課本補充】列表裡暫存器的順序**不重要**，一律重新排成由小到大。另一個角度看：**編號最小的暫存器放在最低的位址**。例：`stmfd sp!, {r2,r3,r1,r4}` 把 r1 放在 sp − 16、r2 在 sp − 12、r3 在 sp − 8、r4 在 sp − 4，然後 sp ← sp − 16。
- 【課本補充】fd 是 **full descending**：stack 往下長（descending），sp 指向最後一個 push 進去的值（full）。不加 `!` 的版本不更新 sp，很少用。
- §18.1 實測：GNU as 把 `stmfd sp!, {…}` 反組譯成 `push {…}`，`ldmfd sp!, {…}` 反組譯成 `pop {…}`。

---

## 12. 例子：遞迴的 power

[52:14]–[63:19]　課本 Example 50（p.162）

計算 xⁿ。x 在 r0、n 在 r1，結果放 r0。

```c
int power(int x, int n) {
	if (n == 0) return 1;
	return x * power(x, n - 1);
}
```

```
power:
	cmp r1, #0           /* compare n with 0 */
	moveq r0, #1         /* return 1 */
	bxeq lr              /* return */

	stmfd sp!, {r4, lr}  /* save r4 and lr */
	mov r4, r0           /* save x in r4 */
	sub r1, r1, #1       /* n = n - 1 */
	bl power             /* recursively call power */
	mul r0, r4, r0       /* power(x,n) = x * power(x,n-1) */
	ldmfd sp!, {r4, pc}  /* restore r4 and return */
```

- **Base case**：用**條件指令** `moveq` 和 `bxeq`，n == 0 時才執行，**完全不用分支**。
- **遞迴**：這是 **callee saved**：
  - lr 一定會被遞迴的 bl 蓋掉，所以要存。
  - r4 會被這個函式改掉，所以要存。
- **ARM ABI**（application binary interface，應用程式二進位介面）的慣例：**參數放 r0、r1、r2…，回傳值放 r0**。
  - r0 同時是參數 x 和回傳值，所以呼叫下一層 power 之前，先把 x 搬到 r4。
- 回來後 r0 = x^(n−1)，乘上 r4（x）就是 xⁿ。
- **最後一行的技巧**（[58:17]）：
  - stack 裡的順序：r4 在低位址，lr 在高位址。
  - ldmfd 由小到大 pop：第一個 pop 給 **r4**（恢復 r4）。
  - 第二個值本來是 lr，但**不放進 lr，而是放進 pc**！這等於 `mov pc, lr`，**直接回傳**。
  - 【口頭】「一石二鳥」：恢復 r4，同時回傳。lr 本來就不需要恢復，因為函式都要結束了。
- 這是寫 ARM 程式非常標準的寫法：**一對 stmfd／ldmfd，push 時存 lr，pop 時放進 pc**。
- 【口頭】C 只有兩行，組合語言 9 行；如果不用 ARM 的技巧會更多。用到的技巧：
  1. eq 條件指令處理 base case，不用分支。
  2. stmfd／ldmfd 存取暫存器，ldmfd 順便回傳。

⚠️ 課本 Example 50 的第 4 行寫成 `bxeq pc, lr`，這不是合法的 ARM 指令（bx 只收一個暫存器）。§18.1 實測：GNU as 回報 `Error: garbage following instruction -- 'bxeq pc,lr'`。應該是 `bxeq lr`（影片說的也是「執行 BX 指令，跳到 return address」）。課本內文也說「在第 10 行遞迴呼叫 power」，其實是第 9 行。

【口頭】caller saved 版本會怎麼做？caller 要存它之後需要的 **r0 和 lr**，呼叫完再恢復。callee saved 則是存**自己會改掉的** r4 和 lr。

---

## 13. 編碼：一般格式

[63:19]–[64:48]　課本 §4.4、Figure 4.4（p.163–164）

每條 ARM 指令都是 **32 位元**。

```
 32    29 28  27 26                                  1
┌────────┬──────┬────────────────────────────────────┐
│cond (4)│type 2│              其他 26 位元             │
└────────┴──────┴────────────────────────────────────┘
```

- **最高 4 位是條件碼**：14 個條件 ＋ al = 15 種，需要 4 位元。**每條指令都有條件欄位**，這就是為什麼任何指令都能條件執行。
- **接著 2 位是指令類型**：資料處理（00）、load／store（01）、分支（10）、其他。

【課本補充】Important Point 6：要唯一編碼 n 個東西，至少要 ⌈log₂ n⌉ 位元。

---

## 14. 資料處理指令的編碼與 12 位元立即值

[64:48]–[77:48]　課本 §4.4.1、Figure 4.5（p.164–166）

```
  4     2   1    4     1    4     4          12
┌──────┬────┬───┬───────┬───┬──────┬──────┬──────────────────────┐
│ cond │ 00 │ I │opcode │ S │  rs  │  rd  │ shifter operand／立即值│
└──────┴────┴───┴───────┴───┴──────┴──────┴──────────────────────┘
 32  29 28 27  26 25  22  21 20  17 16  13 12                   1
```

| 欄位 | 意思 |
|---|---|
| I | 第二個來源是**立即值**還是 **shifter operand**（暫存器，可能有位移） |
| opcode | 哪種運算：add、sub、and、orr… |
| S | **s 後綴**：要不要設定 CPSR flags（例如 adds） |
| rs | 第一個來源暫存器（16 個 → 4 位元） |
| rd | 目的暫存器 |
| 最後 12 位 | shifter operand 或立即值 |

**12 位元的立即值**（[66:37]）：

- 【口頭】「12 是個爛數字：不是 8，也不是 16，不是一個 byte 也不是兩個 byte。」
- 拆成兩個欄位：**4 位元 rot** ＋ **8 位元 payload**。

```
真正的值 = payload 右旋（rotate right）2 × rot 位
```

- **右旋**：從 LSB 那邊移出去的位元，從 MSB 那邊繞回來。
- rot 是 0～15，所以右旋 0、2、4、…、30 位（偶數）。右旋 32 位等於沒動。
- 例：payload = `0xFE`、rot = 2 → 右旋 4 位 → `0xE000000F`（e 跑到最前面，f 留在最後）。
- 例：`0x0000FE00`：`0xFE` 右旋 24 位 → rot = 12 → 編碼 **`0xCFE`**。

**白話解釋**（[74:15]）：

- payload 只有 8 位元，所以一個立即值最多只能設定 **8 個連續的位元**。
- ARM 讓你用剩下的 4 位元**選這 8 個位元放在哪裡**（從偶數的位置開始）。
- 【口頭】「這是 ARM ISA 相當複雜的部分，請讀者花很多時間理解。」

**誰負責轉換**：

- **Assembler**：程式設計師寫 `mov r0, #4`，assembler 要想辦法把它變成 rot ＋ payload。**做不到就要用好幾條指令**。
- **處理器**：執行時把 12 位元拿出來，payload 右旋 2 × rot，展開成 32 位元。

**例子**（[76:02]）：

| 數字 | 編碼 |
|---|---|
| 42 = `0x2A` | 不用旋轉：`0x02A` |
| `0x2A00` | 見下面的 ⚠️ |

⚠️ 影片說 `0x2A00` 是「`0x2A` 右旋 8 位」，所以編碼是 **`0x42A`**。這不對：`0x2A` 右旋 8 位是 `0x2A000000`。要得到 `0x2A00`（左移 8 位），要**右旋 24 位**（= 32 − 8），所以 rot = 12，編碼是 **`0xC2A`**。§18.2 實測：GNU as 編出來是 `e3a00c2a`，最後 12 位就是 `c2a`。（影片前面的 `0xFE00` 例子是用右旋 24 位，那個是對的。）

---

## 15. Shifter operand 的編碼

[77:48]–[79:44]

最後 12 位元有兩種形式（I 表示是暫存器的時候）：

**用立即值當位移量**：

```
   5        2     1    4
┌────────┬──────┬───┬──────┐
│shift amt│ type │ 0 │  rt  │
└────────┴──────┴───┴──────┘
```

**用暫存器當位移量**：

```
   4      1    2     1    4
┌──────┬───┬──────┬───┬──────┐
│  rs  │ 0 │ type │ 1 │  rt  │
└──────┴───┴──────┴───┴──────┘
```

- rt：要被位移的暫存器（最低 4 位）。
- 第 5 位：0 = 位移量是立即值；1 = 位移量在暫存器裡。
- **位移種類**（2 位元）：LSL = 00、LSR = 01、ASR = 10、ROR = 11。
- 位移量：最多 32 位，**5 位元**剛好夠。
- 位移量在暫存器裡時，最高 4 位是那個暫存器的編號。

---

## 16. Load／store 的編碼：I P U B W L

[79:44]–[82:39]　課本 §4.4.2、Figure 4.8、Table 4.17（p.167）

```
  4     2         6            4      4         12
┌──────┬────┬─────────────┬──────┬──────┬──────────────────┐
│ cond │ 01 │ I P U B W L │  rs  │  rd  │ shifter／立即值    │
└──────┴────┴─────────────┴──────┴──────┴──────────────────┘
```

- rs：base 暫存器；rd：load 的目的地，或 store 的資料來源。
- **Load／store 的立即值不是 rot ＋ payload**，而是普通的 **12 位元無號數**（0～4095）。也**不是有號數**。

| 位元 | 0 | 1 |
|---|---|---|
| **I** | 最後 12 位是立即值 | 最後 12 位是 shifter operand |
| **P** | post-indexed | pre-indexed |
| **U** | offset 從 base **減掉** | offset **加到** base |
| **B** | 傳一個 word（4 byte） | 傳一個 byte |
| **W** | 不用 pre／post-indexed（不寫回） | 用 pre／post-indexed（寫回 base） |
| **L** | store | load |

- 注意 **I 的意思和資料處理指令剛好相反**：資料處理時 I = 1 是立即值，這裡 I = 0 是立即值。
- 【口頭】W 可以想成 **write back**：base 暫存器的值會改變。
- 【課本補充】**為什麼需要 U 位元？**（Question 6）12 位元的 offset 是無號數，表示不了 −4、−8。但相對於 frame pointer 或 stack pointer 常常需要負的 offset。U 位元讓 −4 寫成 +4，再告訴處理器用減的。§18.1 實測：`ldr r1, [r0, #-4]` 編出來 U = 0、立即值 = 4。

⚠️ W 位元的實際行為和表格不完全一樣。§18.1 實測：

| 指令 | P | W |
|---|---|---|
| `ldr r0, [r1, #4]!`（pre-indexed） | 1 | **1** |
| `ldr r3, [r0], #4`（post-indexed） | 0 | **0** |

post-indexed 的 W 是 **0**，但它確實會寫回 base。真正的 ARM 規則是：**P = 0（post-indexed）一定寫回**，W 只對 P = 1 有意義（P = 0 且 W = 1 是另一種特殊的指令 ldrt）。課本的「W = 0：不用 pre 或 post-indexed」只對 P = 1 的情況成立。

---

## 17. 分支的編碼與 PC + 8

[82:39]–[85:03]　課本 §4.4.3、Figure 4.9（p.168）

```
  4      3    1             24
┌──────┬─────┬───┬────────────────────────────┐
│ cond │ 101 │ L │      offset（24 位元，有號）   │
└──────┴─────┴───┴────────────────────────────┘
```

- type = 10，再加一個子類型位元 1（大部分常見的分支都是）：101。
- **L（link）位元**：1 = bl（設定 lr，函式呼叫）；0 = b。
- **24 位元 offset**：和 SimpleRisc 一樣，是 **word offset**（每條指令 4 byte、從 4 的倍數開始，最後兩位一定是 00，不用存）。
- 處理器的做法：
  1. offset **sign-extend** 成 32 位元
  2. **左移 2 位**（× 4）
  3. 加到 **PC + 8**，得到分支目標
- 【口頭】**為什麼是 PC + 8，不是 PC？**「這很不直覺，現在還沒辦法回答。」影片說要等到 Chapter 9，⚠️ 課本說 **Ch10**（pipeline）：為了簡化硬體。
- 【課本補充】分支的格式和資料處理、load／store 不同，因為要用更多位元放 offset。SimpleRisc 也一樣。「只要簡單，多一種格式不是壞事。」

§18.1 實測：`bl power` 在位址 0x18，offset = −8 → 0x18 + 8 + (−8) × 4 = **0x0**，正好是 power。

---

## 18. 【實測】

本機：Intel i7-13650HX、Ubuntu、kernel 6.5.0。沒有 32 位元 ARM 的模擬器，所以用 GNU 的 ARM assembler（`arm-none-eabi-as`）和反組譯器（`arm-none-eabi-objdump`）看**編碼**，用 Python 解碼。程式在 `experiments/lecture10/`。

### 18.1 編碼：條件欄位、stmfd／ldmfd、定址模式、分支

```
$ arm-none-eabi-as -o enc.o enc.s && arm-none-eabi-objdump -d enc.o
00000000 <power>:
   0:	e3510000 	cmp	r1, #0
   4:	03a00001 	moveq	r0, #1
   8:	012fff1e 	bxeq	lr
   c:	e92d4010 	push	{r4, lr}
  10:	e1a04000 	mov	r4, r0
  14:	e2411001 	sub	r1, r1, #1
  18:	ebfffff8 	bl	0 <power>
  1c:	e0000094 	mul	r0, r4, r0
  20:	e8bd8010 	pop	{r4, pc}
...
00000038 <mem>:
  38:	e5901000 	ldr	r1, [r0]
  3c:	e5901004 	ldr	r1, [r0, #4]
  40:	e5101004 	ldr	r1, [r0, #-4]
  44:	e7901002 	ldr	r1, [r0, r2]
  48:	e7903102 	ldr	r3, [r0, r2, lsl #2]
  4c:	e6903102 	ldr	r3, [r0], r2, lsl #2
  50:	e5b10004 	ldr	r0, [r1, #4]!
  54:	e4903004 	ldr	r3, [r0], #4
  58:	e5d01002 	ldrb	r1, [r0, #2]
  5c:	e5801000 	str	r1, [r0]

00000060 <loop>:
  60:	02844001 	addeq	r4, r4, #1
  64:	e2944001 	adds	r4, r4, #1
  68:	e1a010a1 	lsr	r1, r1, #1
  6c:	eafffffb 	b	60 <loop>
```

- **條件欄位**（最高的十六進位數字）：一般指令是 **`e`**（14 = al）；`moveq`、`bxeq`、`addeq` 是 **`0`**（0 = eq）。這就是 §13 說的「每條指令都有條件欄位」。
- **S 位元**：`addeq`（`02844001`）和 `adds`（`e2944001`）只差第 21 位（`8` → `9`）：adds 會設定 flags。
- **stmfd／ldmfd**：反組譯成 `push {r4, lr}` 和 `pop {r4, pc}`。`pop {r4, pc}` 就是 §12 的「pop 進 pc 直接回傳」。
- `mov r1, r1, lsr #1` 被反組譯成 `lsr r1, r1, #1`：它其實是 mov 加 shifter operand（`e1a0…`，opcode 是 mov）。
- **分支**：`ebfffff8` → cond = e、101、L = 1（bl）、offset = 0xfffff8 = −8 → 目標 = 0x18 + 8 − 32 = **0x0**；`eafffffb` → L = 0（b）、offset = −5 → 0x6c + 8 − 20 = **0x60**。都是 **PC + 8 + offset × 4**。

用 Python 拆 I、P、U、B、W、L：

```
ldr r3,[r0],#4:   I=0 P=0 U=1 B=0 W=0 L=1      ← post-indexed：P=0，W=0
ldr r0,[r1,#4]!:  I=0 P=1 U=1 B=0 W=1 L=1      ← pre-indexed：P=1，W=1
ldr r1,[r0,#-4]:  U=0 imm=4                    ← −4 存成 U=0、+4
```

**課本 Example 50 的 `bxeq pc, lr`**：

```
$ arm-none-eabi-as -o b2.o bad2.s
bad2.s:2: Error: garbage following instruction -- `bxeq pc,lr'
```

### 18.2 12 位元立即值：rot ＋ payload

```
e3a0002a: cond=e rot=0 payload=0x2a -> 0x0000002a
e3a00c2a: cond=e rot=12 payload=0x2a -> 0x00002a00     ← 0x2A00 是 rot=12，不是 4
e3a004fe: cond=e rot=4 payload=0xfe -> 0xfe000000
e3a002fe: cond=e rot=2 payload=0xfe -> 0xe000000f      ← 影片的 0xFE 右旋 4 位
```

列出**所有**能用 rot ＋ payload 表示的 32 位元常數：

```
$ python3 rot.py
distinct encodable constants: 3073
0x0000002a: rot=0 payload=0x2a -> imm12=0x02a
0x00002a00: rot=12 payload=0x2a -> imm12=0xc2a
0xfe000000: rot=4 payload=0xfe -> imm12=0x4fe
0x00000101: 不能表示
0x000003fc: rot=15 payload=0xff -> imm12=0xfff
0x0000ff00: rot=12 payload=0xff -> imm12=0xcff
0x00000fff: 不能表示
0x12345678: 不能表示
```

- 12 位元有 4096 種組合，但只能表示 **3073 種**不同的常數（有些組合重複，例如 0 不管怎麼轉都是 0）。和 2³² 比起來非常少。
- `0x101`（257）：兩個 1 相隔 9 位，超過 8 位元的範圍，表示不了。連 `0xFFF`（4095）也不行。

**表示不了時 assembler 怎麼辦**：

```
$ arm-none-eabi-as bad3.s          （.arch armv4t，mov r0, #0x101）
bad3.s:3: Error: invalid constant (101) after fixup

$ arm-none-eabi-as bad1.s          （預設架構）
   0:	e3000101 	movw	r0, #257	; 0x101
```

- 舊的 ARM（ARMv4T）直接報錯：要程式設計師自己拆成好幾條指令（影片說的「需要多條指令」）。
- 新的 ARM（ARMv6T2 以後）有 **`movw`**：可以直接載入 16 位元的立即值。GNU as 自動換成 movw。這和 SimpleRisc 的 modifier 是同樣的想法（ARM 還有 `movt` 載入高 16 位，就像 movh）。

---

## 19. 一頁速查表

| 主題 | 重點 |
|---|---|
| 分支 | `b`、`beq`（Z=1）、`bne`（Z=0）；b ＋ 任何條件碼 |
| 條件碼 | 16 個：eq ne hs lo mi pl vs vc hi ls ge lt gt le al（15 保留） |
| 無號 | hs：C=1；lo：C=0；hi：C=1∧Z=0；ls：C=0∨Z=1（carry 和 borrow 相反） |
| 有號（課本） | ge：N=0；lt：N=1；gt：Z=0∧N=0；le：Z=1∨N=1（未考慮 overflow） |
| bl | 跳到 label，lr（r14）← PC + 4 |
| 回傳 | `bx lr`（建議）或 `mov pc, lr`；ARM 沒有 ret |
| 條件執行 | 任何指令加條件碼：addeq、moveq、bxeq；不成立就像 nop |
| ldr／str | `[r0]`、`[r0, #4]`、`[r0, r2]`、`[r0, r2, lsl #2]` |
| byte／half | ldrb、ldrh、ldrsb、ldrsh、strb、strh |
| Post-indexed | `ldr r3, [r0], #4`：先用 r0，再 r0 += 4（i++） |
| Pre-indexed | `ldr r3, [r0, #4]!`：先 r0 += 4，再用（++i） |
| 陣列 | 用移動的指標 ＋ 結尾位址，省掉 index；少 20% 指令 |
| stmfd／ldmfd | `stmfd sp!, {…}` push（小編號在低位址）；`ldmfd sp!, {…, pc}` pop 並回傳 |
| ABI | 參數 r0、r1、r2…；回傳值 r0 |
| 一般格式 | cond(4) ＋ type(2) ＋ … |
| 資料處理 | cond 00 I opcode(4) S rs rd operand(12) |
| 12 位元立即值 | 8 位元 payload 右旋 2 × rot；只能設 8 個連續位元；3073 種 |
| Shifter operand | rt ＋ 位移量（5 位元立即值或暫存器）＋ 種類（LSL LSR ASR ROR） |
| Load／store | cond 01 I P U B W L rs rd imm12（無號 0～4095）；I 意思相反 |
| 分支 | cond 101 L offset(24)；目標 = PC + 8 + offset × 4 |

---

## 20. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| x86 的 flags（ZF、CF、SF、OF）、jcc 條件跳躍、cmov | [第 11](lecture11_note.md)–[14 支](lecture14_note.md)：Ch5 x86 |
| RISC-V 不用 flags，直接比較暫存器分支（beq rs1, rs2） | [第 16 支](lecture16_note.md)：RISC-V 的分支 |
| 條件執行為什麼能提升效能 | [第 30](lecture30_note.md)–[33 支](lecture33_note.md)：課本 Ch10 pipeline |
| PC + 8 的原因 | [第 30](lecture30_note.md)–[33 支](lecture33_note.md)：課本 Ch10 pipeline |
| 真實的 ARM 處理器（Cortex-M3、A8、A15） | 課本 Appendix A |

---

## 附：本講一句話總結

> **ARM 的分支是 b 加上 16 種條件碼之一（eq／ne 看 Z，無號的 hs／lo／hi／ls 看 C 和 Z，因為無號減法不借位時 carry = 1，有號的 ge／lt／gt／le 在課本裡只看 N 和 Z），bl 跳到函式並把 PC + 4 存進 lr，ARM 沒有 ret，用 bx lr（或 mov pc, lr）回傳；條件碼可以加在任何指令上（addeq、moveq），條件不成立就當 nop，省掉分支。ldr／str 支援 register-indirect、base-offset、base-index、base-scaled-index（[r0, r2, lsl #2]），以及像 i++ 的 post-indexed（[r0], #4）和像 ++i 的 pre-indexed（[r0, #4]!），用移動的指標加總陣列可以少 20% 指令；stmfd／ldmfd 一次 push／pop 多個暫存器，ldmfd sp!, {r4, pc} 同時恢復暫存器並回傳。編碼上每條指令最高 4 位是條件碼、接著 2 位是類型；資料處理的 12 位元立即值是 8 位元 payload 右旋 2 × rot（實測只能表示 3073 種常數，0x2A00 要 rot = 12 而非影片說的 4，新的 ARM 改用 movw）；load／store 用 I P U B W L 六個位元和 12 位元無號 offset（U 位元處理負 offset，實測 post-indexed 的 W 是 0）；分支用 24 位元 word offset，目標是 PC + 8 + offset × 4。課本 Example 50 的 bxeq pc, lr 無法組譯，應為 bxeq lr。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 4: ARM Assembly, Part 2/2 錄影逐字稿（YouTube 自動字幕，1:25:08）、Sarangi *Basic Computer Architecture* v3.09 Ch4 §4.2.6–4.5（p.148–170）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、GNU as `arm-none-eabi-as`、Python 3.10）結果。*
