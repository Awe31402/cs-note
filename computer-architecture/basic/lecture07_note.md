# Basic Computer Architecture 第七支影片完整筆記：SimpleRisc 的指令（Chapter 3, Part 2/3）

> **課程**：Basic Computer Architecture — Chapt 3: Assembly Language, Part 2/3（播放清單第 7 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（1:26:30，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 Ch3 §3.3.1–3.3.9、§3.3.13（p.107–114、p.124–125）
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
- [1. 為什麼先學 SimpleRisc](#1-為什麼先學-simplerisc)
- [2. 各家指令集一覽](#2-各家指令集一覽)
- [3. SimpleRisc 的機器模型](#3-simplerisc-的機器模型)
- [4. mov](#4-mov)
- [5. 算術指令：add、sub、mul、div、mod](#5-算術指令addsubmuldivmod)
- [6. cmp 與 flags](#6-cmp-與-flags)
- [7. 把 C 翻成組合語言](#7-把-c-翻成組合語言)
- [8. 邏輯指令：and、or、not](#8-邏輯指令andornot)
- [9. 位移指令：lsl、lsr、asr](#9-位移指令lsllsrasr)
- [10. 用位移代替乘除](#10-用位移代替乘除)
- [11. ld 與 st](#11-ld-與-st)
- [12. 分支：b、beq、bgt](#12-分支bbeqbgt)
- [13. 例子：階乘](#13-例子階乘)
- [14. Food for thought：Ramanujan number](#14-food-for-thoughtramanujan-number)
- [15. Modifier：u 與 h](#15-modifieru-與-h)
- [16. 【實測】](#16-實測)
- [17. 一頁速查表](#17-一頁速查表)
- [18. 與後續講次的連結](#18-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

我們要**從零設計**一套組合語言，叫 **SimpleRisc**。這一講把它的指令一條一條加上去：

```
16 個暫存器 ＋ flags（E、GT）
  → mov
  → 算術：add sub mul div mod cmp
  → 邏輯：and or not
  → 位移：lsl lsr asr
  → 記憶體：ld st
  → 分支：b beq bgt
  → modifier：u h（載入 32 位元常數）
```

下一講（第 8 支）補上函式（call、ret）、nop 和指令編碼。

---

## 1. 為什麼先學 SimpleRisc

[00:45]–[02:06]　課本 §3.3（p.107）

- SimpleRisc 是一個**非常簡單的 RISC 式**語言：指令少，結構規律。
- 【口頭】「最好先學一個非常簡單的組合語言，抓到感覺，再去學 ARM、x86 這些真實的語言。真實的語言有很多細節，現在會把讀者淹沒。」
- 路線：設計 SimpleRisc → 設計它的 binary 編碼（下一講）→ 在處理器那章（課本 Ch9）做出一顆實作 SimpleRisc 的處理器。
- 【課本補充】SimpleRisc 一共只有 **21 條指令**，但抓住了完整組合語言的大部分特色。它在概念上最接近 **ARM 和 MIPS**。

---

## 2. 各家指令集一覽

[02:06]–[08:14]　課本 §3.3.1、Table 3.1（p.107）

| ISA | 類型 | 年份 | 廠商 | 位元 | Endian | 暫存器 |
|---|---|---|---|---|---|---|
| VAX | CISC | 1977 | DEC | 32 | little | 16 |
| SPARC | RISC | 1986 / 1993 | Sun | 32 / 64 | big / bi | 32 |
| PowerPC | RISC | 1992 / 2002 | Apple、IBM、Motorola | 32 / 64 | bi | 32 |
| PA-RISC | RISC | 1986 / 1996 | HP | 32 / 64 | big | 32 |
| m68000 | CISC | 1979 | Motorola | 16 / 32 | big | 16 |
| MIPS | RISC | 1981 / 1999 | MIPS | 32 / 64 | bi | 32 |
| Alpha | RISC | 1992 | DEC | 64 | bi | 32 |
| x86 | CISC | 1978 / 1985 / 2003 | Intel、AMD | 16 / 32 / 64 | little | 8 / 8 / 16 |
| ARM | RISC | 1985 / 2011 | ARM | 32 / 64 | bi（預設 little） | 16 / 31 |

（表格依課本 Table 3.1。）

口頭補充：

- **32 位元 ISA 是什麼意思**：暫存器是 32 位元。⚠️ 影片也說「在 RISC 架構裡，指令的大小是 64 位元」，這不對：64 位元的 RISC ISA（例如 ARM64、MIPS64）指令通常**還是 32 位元**，變成 64 位元的是暫存器和位址。
- HP 的 PA-RISC 還用在高階 HP 伺服器；Motorola 已經不做處理器了。
- MIPS 現在用在一些小型嵌入式處理器，但大部分嵌入式處理器已經轉到 Intel 和 ARM。
- 【口頭】**DEC Alpha** 是老師最喜歡的指令集，因為是他學的第一個。
- **x86**：Intel 1978 年從 16 位元開始，1985 年轉 32 位元，2003 年轉 64 位元。**AMD 先做出 x86-64**（Opteron），Intel 才跟進。
  - ⚠️ 影片說 x86「現在是開放標準，讀者可以自己實作 x86 處理器」。課本沒有這個說法；實際上 x86 是 Intel 和 AMD 之間互相授權的，不是可以自由實作的開放標準。
- **ARM**：ARM 公司**不自己做晶片**，它設計處理器、把設計賣給 Texas Instruments、Qualcomm、Samsung 等公司，由它們加上其他元件、製造晶片。ARM 1985 年就有 32 位元指令集，近年因為手機爆炸性成長而變得重要；2011 年推出 64 位元指令集，給平板、伺服器等比較大的裝置。
- 【課本補充】截至 2012 年，約 90% 的行動裝置用 ARM；90% 以上的桌機和筆電用 x86。大型伺服器常用 IBM PowerPC、Sun（現 Oracle）UltraSparc、HP PA-RISC。

---

## 3. SimpleRisc 的機器模型

[08:14]–[11:50]　課本 §3.3.2（p.108）

- **16 個暫存器**：r0～r15，每個 **32 位元**。
  - r0～r13：一般用途
  - **r14 = sp**（stack pointer）
  - **r15 = ra**（return address register）
  - sp 和 ra 的意義下一講講函式時再說。
- **記憶體**：Von Neumann 模型，一個大 byte 陣列，放資料和程式。假設程式獨占整個記憶體。
  - 【課本補充】多 byte 資料用 **little endian** 存。每條指令編碼成 **32 位元**（4 byte）。
- **flags 暫存器**：存**最後一次比較**的結果。
  - **組合語言程式設計師看不到**，但硬體設計者看得到。
  - 兩個欄位：**flags.E**（equal）和 **flags.GT**（greater than）。預設都是 0。

```
cmp r1, r2
  r1 == r2  →  flags.E = 1, flags.GT = 0
  r1 >  r2  →  flags.E = 0, flags.GT = 1
  r1 <  r2  →  flags.E = 0, flags.GT = 0
```

---

## 4. mov

[11:50]–[14:38]　課本 §3.3.3（p.108）

| 寫法 | 例子 | 意思 |
|---|---|---|
| `mov reg, (reg/imm)` | `mov r1, r2` | r1 ← r2 |
| | `mov r1, 3` | r1 ← 3 |

- 格式：**先寫 opcode，再寫目的地，再寫來源**。
- 類比 C：`x = 3` 就像 `mov r1, 3`；`x = y` 就像 `mov r1, r2`。
- 立即值**直接嵌在指令裡**。
- SimpleRisc 的立即值是 **16 位元的 2's complement**，範圍 **−2¹⁵ 到 2¹⁵ − 1**。

---

## 5. 算術指令：add、sub、mul、div、mod

[14:38]–[20:25]　課本 §3.3.4（p.108–109）

| 寫法 | 例子 | 意思 |
|---|---|---|
| `add reg, reg, (reg/imm)` | `add r1, r2, r3` | r1 ← r2 + r3 |
| | `add r1, r2, 10` | r1 ← r2 + 10 |
| `sub reg, reg, (reg/imm)` | `sub r1, r2, r3` | r1 ← r2 − r3 |
| `mul reg, reg, (reg/imm)` | `mul r1, r2, 10` | r1 ← r2 × 10 |
| `div reg, reg, (reg/imm)` | `div r1, r2, r3` | r1 ← r2 / r3（商） |
| `mod reg, reg, (reg/imm)` | `mod r1, r2, r3` | r1 ← r2 mod r3（餘數） |
| `cmp reg, (reg/imm)` | `cmp r1, r2` | 設定 flags |

- **3-address format**。限制：
  - 目的地一定是**暫存器**
  - 第一個來源一定是**暫存器**
  - **只有第二個來源**可以是暫存器**或**立即值
- 【口頭】和高階語言很像：`x = y + z` 就直接翻成 `add`。相同點是目的地都在前面；不同點是組合語言**先寫運算**，再寫運算元。
- 【課本補充】div 算**商**：30 / 7 = 4。mod 算**餘數**：30 mod 7 = 2。

---

## 6. cmp 與 flags

[20:25]–[22:17]　課本 p.109

- cmp 在高階語言裡**沒有對應**，所以比較特別。
- 做法：算 r1 − r2。
  - 結果 = 0 → flags.E = 1
  - 結果 > 0 → flags.GT = 1
  - 結果 < 0 → 兩個都是 0（可以推出「小於」）
- **只有 cmp 會設定 flags**。flags 保持原值，**直到下一個 cmp**。中間可以有很多其他指令，後面的指令（分支）還是能用這些 flags。
- 【課本補充】cmp 是 2-address：第一個來源是暫存器，第二個可以是暫存器或立即值。

---

## 7. 把 C 翻成組合語言

[22:17]–[30:14]　課本 Example 26、27（p.109）

**一般方法**：

1. **把變數分配給暫存器**（register assignment）
2. 每一行高階敘述翻成組合語言

**例 1**：

```c
a = 3;  b = 5;  c = a + b;  d = c - 5;
```

```
mov r0, 3        @ a → r0
mov r1, 5        @ b → r1
add r2, r0, r1   @ c → r2
sub r3, r2, 5    @ d → r3
```

**例 2**：c = a × b，d = c mod 5

```
mov r0, 3
mov r1, 5
mul r2, r0, r1
mod r3, r2, 5
```

**例 3**：比較

| 程式 | flags.E | flags.GT |
|---|---|---|
| `mov r0, 3; mov r1, 5; cmp r0, r1` | 0 | 0 |
| `mov r0, 5; mov r1, 3; cmp r0, r1` | 0 | 1 |
| `mov r0, 5; mov r1, 5; cmp r0, r1` | 1 | 0 |

**例 4**：31 / 29 − 50，存到 r4（整數除法，31 / 29 = 1）

```
mov r1, 31
mov r2, 29
div r3, r1, r2
sub r4, r3, 50
```

- 這個例子很簡單，4 行高階敘述剛好 4 行組合語言；一般來說組合語言會**多很多**。
- 【口頭】**只有多寫才會寫得好**。不管讀多少書、上多少線上課程，「熟能生巧」。
- 【口頭】書的網站上有 **SimpleRisc emulator**，可以一行一行跑你的程式、印出結果。（§16.1 實測自己寫了一個。）
- 【口頭】老師的建議：**這門課結束時，至少要寫過 3,000 行組合語言**，SimpleRisc、ARM、x86 各 1,000 行。

---

## 8. 邏輯指令：and、or、not

[30:14]–[33:36]　課本 §3.3.5（p.110）

| 寫法 | 例子 | 意思 |
|---|---|---|
| `and reg, reg, (reg/imm)` | `and r1, r2, r3` | r1 ← r2 AND r3（逐位元） |
| `or reg, reg, (reg/imm)` | `or r1, r2, r3` | r1 ← r2 OR r3 |
| `not reg, (reg/imm)` | `not r1, r2` | r1 ← ~r2 |

- and、or 和算術指令一樣是 3-address；**not 是 2-address**（只有一個來源，可以是暫存器或立即值）。
- 逐位元 AND 的例子：`1100` AND `0101` = `0100`。
- not 的例子：r2 = `0010` → `1101`。
- 【口頭】一般規則：**只有一個來源運算元可以是立即值**，不能兩個都是。
- 例：A OR B（a 在 r0、b 在 r1，結果放 r2）：

```
or r2, r0, r1
```

【課本補充】課本 Example 28 是另一題：算 **NOT(a OR b)**（題目寫成 a ∨ b 上面加一條橫線，也就是 NOR）。所以 or 之後多一條 not：

```
or  r3, r0, r1
not r2, r3
```

§16.1 實測：a = `1100`、b = `1010` 時，影片的 a OR b 得到 14（`1110`），課本的 NOT(a OR b) 得到 −15（`…11110001`）。

（用 pdftotext 抽出來的文字會把橫線弄丟，看起來就像「a ∨ b」，要看 PDF 原頁才看得出來。）

---

## 9. 位移指令：lsl、lsr、asr

[33:36]–[46:44]　課本 §3.3.6（p.110）

**Left shift（左移）**：

- 每個位元往左移 n 格，右邊補 0。
- `0010` 左移 2 → `1000`：2 變成 8。
- `0011` 左移 1 → `0110`：3 變成 6。
- **左移 n 位 = 乘以 2ⁿ**（沒有 overflow 的話）。
- 證明（[35:59]）：A = Σ xᵢ 2ⁱ⁻¹。左移 k 位後，係數不變，但每一項的指數都加 k，所以整個數乘上 2ᵏ。
- **對負數也成立**（2's complement）：
  - −2 = `1110` 左移 1 → `1100` = −4
  - −3 = `1101` 左移 1 → `1010` = 10 − 16 = −6

**Right shift（右移）有兩種**：

| 種類 | 符號 | 左邊補什麼 | 意思 |
|---|---|---|---|
| **Arithmetic right shift（算術右移）** | `>>` | **複製符號位** | 有號數除以 2ⁿ |
| **Logical right shift（邏輯右移）** | `>>>` | **補 0** | 無號數除以 2ⁿ |

- `>>>` 這個寫法來自 **Java**。
- 例：`0100`（4）算術右移 2 → `0001`（1）。
- 例：−6 = `1010` 算術右移 1 → `1101` = 13 − 16 = −3。
- ⚠️ 「算術右移 = 除以 2ⁿ」只在**整除**的時候完全成立。負的奇數右移會**往負無窮大捨入**，C 的除法則是**往 0 捨入**：−3 >> 1 = −2，但 −3 / 2 = −1。§16.2 實測。

**Food for thought：為什麼沒有算術左移？**

- 左移時，符號位**一定會被右邊移過來的位元蓋掉**，複製符號位沒有意義。所以左移只有一種，右移有兩種。

**SimpleRisc 的三條位移指令**（3-address，和 add 同格式）：

| 寫法 | 意思 |
|---|---|
| `lsl r3, r1, r2`／`lsl r3, r1, 4` | logical shift left |
| `lsr r3, r1, r2`／`lsr r3, r1, 4` | logical shift right |
| `asr r3, r1, r2`／`asr r3, r1, 4` | arithmetic shift right |

---

## 10. 用位移代替乘除

[46:44]–[51:10]

**例 1**：101 × 6，只用位移

```
mov r0, 101
lsl r1, r0, 1    @ r1 = 101 × 2
lsl r2, r0, 2    @ r2 = 101 × 4
add r3, r1, r2   @ r3 = 101 × 6
```

**例 2**：102 × 7.5 = 102 × 8 − 102 × 0.5

```
mov r0, 102
lsl r1, r0, 3    @ r1 = 102 × 8
asr r2, r0, 1    @ r2 = 102 / 2（102 是正數，lsr 和 asr 一樣）
sub r3, r1, r2   @ r3 = 102 × 7.5
```

**為什麼要這麼麻煩？**（[48:27]）

- 【口頭】有人會問：直接 `mul r3, r0, 6` 不就好了？
- 答案：硬體裡**乘法和除法很貴**（很花時間），**位移超級快**。
- 所以很多 compiler 和好的程式設計師會盡量用位移代替乘除。
- §16.2 實測：gcc 把 `x * 6` 編成 `lea`（x + 2x）加 `add`，完全沒有乘法指令；`x / 2` 編成位移加修正。

---

## 11. ld 與 st

[51:10]–[61:21]　課本 §3.3.7（p.111）

| 寫法 | 例子 | 意思 |
|---|---|---|
| `ld reg, imm[reg]` | `ld r1, 10[r2]` | r1 ← [r2 + 10]（**load**：記憶體 → 暫存器） |
| `st reg, imm[reg]` | `st r1, 10[r2]` | [r2 + 10] ← r1（**store**：暫存器 → 記憶體） |

- 2-address format，用 **base-offset 定址**。
  - ⚠️ 老師一開始口誤說「base index」，馬上改口成 base-offset。
- ld：r2 的值加 10 → 得到位址 → 讀記憶體 → 放進 r1。32 位元系統讀 4 byte（64 位元系統是 8 byte）。
- st：**沒有暫存器的目的地**，目的地是記憶體。先寫要**讀**的暫存器，再寫記憶體位置。
- 【課本補充】算出來的位址指向整數的**第一個 byte**；因為是 little endian，那個 byte 是 LSB。

**陣列的例子**（[55:07]）：

- 10 個 int 的陣列 = **連續 40 byte**。base address 在 r0，設 r0 的值是 V。
  - arr[0] 在 V～V+3
  - arr[1] 在 V+4～V+7
  - **arr[n] 從 r0 + 4n 開始**
  - arr[9] 從 V + 36 開始

```c
arr[3] = 5;
arr[4] = 8;
arr[5] = arr[4] + arr[3];
```

```
mov r1, 5
st  r1, 12[r0]   @ arr[3] = 5（3 × 4 = 12）
mov r2, 8
st  r2, 16[r0]   @ arr[4] = 8
add r3, r1, r2
st  r3, 20[r0]   @ arr[5] = arr[4] + arr[3]
```

- 【口頭】這就是 **base-offset 定址的好處**：base address 放在一個暫存器，用 offset 存取不同元素。要記得元素的大小：int 是 4 byte，所以第 n 個在 base + 4n。

---

## 12. 分支：b、beq、bgt

[61:21]–[66:10]　課本 §3.3.8–3.3.9（p.111–112）

有了分支，才能實作 if、for、while。

| 寫法 | 例子 | 意思 |
|---|---|---|
| `b label` | `b .foo` | 無條件跳到 .foo |
| `beq label` | `beq .foo` | flags.E = 1 就跳到 .foo |
| `bgt label` | `bgt .foo` | flags.GT = 1 就跳到 .foo |

- b：PC 本來指著 b 這一行，之後改成指著 .foo 那一行，從那裡繼續執行。
- beq、bgt 看的是**最近一次 cmp** 設的 flags。中間可以有很多不是 cmp 的指令。
- 【課本補充】真實的指令集分支指令比較多；但只要 beq 和 bgt（加上 b）就能實作**所有**種類的分支，代價是程式碼變長。條件不成立時叫做分支**失敗**（fail），繼續執行下一行。

**例**：if (r1 > r2) r3 = 4; else r3 = 5;

```
	cmp r1, r2
	bgt .gtlabel
	mov r3, 5
	…
.gtlabel:
	mov r3, 4
```

- ⚠️ 逐字稿看不到投影片上的完整程式（例如 else 分支後面有沒有 `b` 跳過 .gtlabel）。上面只寫出影片口頭描述的部分。

---

## 13. 例子：階乘

[66:10]–[73:12]　課本 Example 29（p.113）

**Step 0**：程式複雜時，**先用 C 寫一遍**，比較容易看清楚每個部分。

```c
int prod = 1;
int idx;
for (idx = num; idx > 1; idx--) {
	prod = prod * idx;
}
```

**Step 1**：變數分配 r0 = num、r1 = prod、r2 = idx。

**Step 2**：翻譯。

```
	mov r1, 1        /* prod = 1 */
	mov r2, r0       /* idx = num */
.loop:
	mul r1, r1, r2   /* prod = prod * idx */
	sub r2, r2, 1    /* idx = idx - 1 */
	cmp r2, 1        /* compare (idx, 1) */
	bgt .loop        /* if (idx > 1) goto .loop */
```

- idx > 1 時 flags.GT = 1，bgt 跳回 .loop；idx 變成 1 時 bgt 失敗，往下掉出迴圈。迴圈跑 num − 1 次。
- 假設 num 夠大，不檢查特殊情況。
- 【口頭】「這個程式只有 6 行，但難度是 10⁶ 倍，因為這是你第一次看到一個相當複雜的組合語言程式。請看 5 遍、10 遍、需要的話 100 遍，用紙筆走一次，確定懂了再往下。」
- §16.1 實測：5! = 120（18 步）、10! = 3628800（38 步）。

【課本補充】課本還有兩個例子，§16.1 也實測了：

- **Example 30**：判斷 r1 是不是質數（r1 > 3）。從 2 開始一個一個除，`mod` 為 0 就不是質數。
- **Example 31**：求 r1 和 r2 的最小公倍數。L = A × idx，idx 從 1 開始加，直到 L 能被 B 整除。

---

## 14. Food for thought：Ramanujan number

[73:12]–[75:36]

- 題目：寫一個 SimpleRisc 程式找出 **Ramanujan number**。「很難，但做得到。」
- 故事：Ramanujan 生病時，他的指導教授 Hardy 來探望他，說他坐的計程車車牌是 **1729**，問這個數有什麼特別。Ramanujan 說：1729 是**能用兩種不同方式寫成兩個立方數之和的最小的數**：

```
1729 = 10³ + 9³ = 12³ + 1³
```

- ⚠️ 逐字稿把 Hardy 聽成「Harvey」。
- 方法：**先用 C 寫**（從 2 開始，對每個數用好幾層 for 迴圈檢查能不能寫成兩種立方和），**再做暫存器分配，再翻成組合語言**。

---

## 15. Modifier：u 與 h

[75:36]–[86:18]　課本 §3.3.13（p.124–125）

**問題**：SimpleRisc 的立即值只有 **16 位元**，暫存器是 32 位元。`mov r0, -3` 時，高 16 位放什麼？

- **預設**：16 位元立即值當成**有號數**，做 **sign extension**。這樣 −3 存進 r0 還是 −3。

**兩個 modifier**（加在指令名稱後面）：

| 寫法 | 16 位元立即值變成 32 位元的方式 | `0xAB12` 變成 |
|---|---|---|
| `mov`（預設） | sign extension | `0xFFFFAB12` |
| `movu` | **unsigned**：高 16 位補 0 | `0x0000AB12` |
| `movh` | **high**：放到高 16 位，低 16 位補 0（等於左移 16） | `0xAB120000` |

- 機制：處理器把指令裡的立即值拿出來，放進一個看不見的內部暫存器，**轉成 32 位元**，再拿去運算。modifier 控制這個轉換怎麼做。
- 影片：modifier 可以用在**算術、邏輯指令和 mov**（所有用立即值的指令）。
  - 【課本補充】課本更精確：可以用在**所有 ALU 指令，除了位移指令**。

**為什麼要有 modifier**：用**最少的指令**把任意 32 位元常數放進暫存器。

例：要讓 r0 = `0x012AA92D`：

```
movh r0, 0x12A       @ r0 = 0x012A 0000
addu r0, r0, 0xA92D  @ r0 = r0 + 0x0000 A92D = 0x012A A92D
```

- 第二行如果沒有 `u`：`0xA92D` 的最高位是 1，sign extension 會變成 `0xFFFFA92D`，加起來就錯了。§16.1 實測：不加 u 得到 `0x0129A92D`，加了 u 才得到 `0x012AA92D`。
- 兩條指令就能放進 32 位元。

【課本補充】沒有 modifier 的話要 **6 條指令**（課本的例子是 `0xFB12CDEF`）：

```
mov r0, 0xFB12
lsl r0, r0, 16
mov r1, 0xCDEF
lsl r1, r1, 16
lsr r1, r1, 16      /* 高 16 位清成 0 */
add r0, r0, r1
```

有了 modifier 只要 2 條：`movh r0, 0xFB12` ＋ `addu r0, r0, 0xCDEF`。課本也提到：assembler 通常有直接載入常數的 directive，但它最後還是要翻成這些基本指令，沒有從根本解決問題。

---

## 16. 【實測】

本機：Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10。程式在 `experiments/simplerisc/`（直譯器，下一講會再用）和 `experiments/lecture07/`。

### 16.1 自己寫一個 SimpleRisc 直譯器

`experiments/simplerisc/simplerisc.py` 實作了課本的 21 條指令、flags、u／h modifier、little endian 記憶體，約 80 行 Python。跑本講的例子（程式在 `experiments/simplerisc/l07/`）：

```
$ python3 simplerisc.py fact.s r0=5
fact.s: 18 steps; r0=5 r1=120 r2=1 | E=1 GT=0
$ python3 simplerisc.py fact.s r0=10
fact.s: 38 steps; r0=10 r1=3628800 r2=1 | E=1 GT=0
$ python3 simplerisc.py prime.s r1=97
prime.s: 574 steps; r0=1 r1=97 r2=97 r3=1 | E=1 GT=0
$ python3 simplerisc.py prime.s r1=91
prime.s: 36 steps; r1=91 r2=7 | E=1 GT=0
$ python3 simplerisc.py lcm.s r1=12 r2=18
lcm.s: 18 steps; r0=36 r1=12 r2=18 r3=3 r4=36 | E=1 GT=0
```

- 階乘：5! = 120、10! = 3628800。最後 r2 = 1、E = 1（cmp r2, 1 相等），所以 bgt 失敗、離開迴圈。
- 質數：97 是質數（r0 = 1）；91 = 7 × 13 不是（r0 = 0，在 r2 = 7 時發現）。
- 最小公倍數：lcm(12, 18) = 36。

```
$ python3 simplerisc.py cmp35.s   → E=0 GT=0
$ python3 simplerisc.py cmp53.s   → E=0 GT=1
$ python3 simplerisc.py cmp55.s   → E=1 GT=0
```

和 §7 例 3 的表一樣。

**影片的 a OR b 與課本 Example 28 的 NOT(a OR b)**（a = `1100` = 12、b = `1010` = 10）：

```
$ python3 simplerisc.py ex28.s
ex28.s: 5 steps; r0=12 r1=10 r2=-15 r3=14 r4=14
```

- 影片的 `or r4, r0, r1` → 14 = `1110`，就是 a OR b。
- 課本的 `or r3, r0, r1; not r2, r3` → r2 = −15 = `…11110001`，就是 NOT(a OR b)（32 位元全部取反）。

**位移例子**：

```
$ python3 simplerisc.py shift.s
shift.s: 8 steps; r0=101 r1=202 r2=404 r3=606 r4=102 r5=816 r6=51 r7=765
```

101 × 6 = 606；102 × 7.5 = 816 − 51 = 765。

**Modifier**：

```
mov  r0, 0xAB12          r0 = 0xFFFFAB12   ← sign extension
movu r1, 0xAB12          r1 = 0x0000AB12   ← unsigned
movh r2, 0xAB12          r2 = 0xAB120000   ← high
movh r3, 0x12A           r3 = 0x012A0000
add  r4, r3, 0xA92D      r4 = 0x0129A92D   ← 錯：0xA92D 被 sign-extend 成 0xFFFFA92D
addu r5, r3, 0xA92D      r5 = 0x012AA92D   ← 對
mov  r6, -3              r6 = 0xFFFFFFFD
```

### 16.2 右移 ≠ 除法（負的奇數）；gcc 怎麼代替乘除

```c
int div2(int x) { return x / 2; }
int sar1(int x) { return x >> 1; }
int mul6(int x) { return x * 6; }
unsigned udiv2(unsigned x) { return x / 2; }
```

```
$ ./shift
x=-7  x/2=-3  x>>1=-4
x=-5  x/2=-2  x>>1=-3
x=-3  x/2=-1  x>>1=-2
x=-1  x/2= 0  x>>1=-1
x= 1  x/2= 0  x>>1= 0
x= 3  x/2= 1  x>>1= 1
...
```

- 正數兩者一樣；**負的奇數差 1**：算術右移往負無窮大捨入（−3 >> 1 = −2），C 的除法往 0 捨入（−3 / 2 = −1）。
- 影片的例子 −6 >> 1 = −3 剛好是整除，所以沒看出差別。

```
$ gcc -O2 -c shift.c && objdump -d shift.o
<div2>:   mov %edi,%eax
          shr $0x1f,%eax      ← 取出符號位（負數是 1）
          add %edi,%eax       ← 負數先加 1
          sar %eax            ← 再算術右移
<sar1>:   mov %edi,%eax
          sar %eax            ← 就是一條 asr
<mul6>:   lea (%rdi,%rdi,2),%eax   ← x + 2x = 3x
          add %eax,%eax            ← 3x × 2 = 6x
<udiv2>:  mov %edi,%eax
          shr %eax            ← 無號數：一條 lsr
```

- **有號數 x / 2**：gcc 用位移，但多兩條指令修正負數的捨入方向。
- **無號數 x / 2**：一條**邏輯右移**（shr = lsr）就夠了。
- **x × 6**：完全沒用乘法，用 lea 和 add，和 §10 的想法一樣（只是 6x = 3x × 2）。

---

## 17. 一頁速查表

| 主題 | 重點 |
|---|---|
| SimpleRisc | 21 條指令；RISC；最接近 ARM、MIPS |
| 暫存器 | r0–r15，32 位元；r14 = sp，r15 = ra |
| flags | E、GT；只有 cmp 會設定；程式設計師看不到 |
| 立即值 | 16 位元 2's complement：−2¹⁵ ～ 2¹⁵ − 1 |
| mov | `mov reg, (reg/imm)` |
| 算術 | `add/sub/mul/div/mod reg, reg, (reg/imm)`；只有最後一個可以是立即值 |
| cmp | `cmp reg, (reg/imm)`：=0 → E；>0 → GT；<0 → 都 0 |
| 邏輯 | `and/or reg, reg, (reg/imm)`；`not reg, (reg/imm)` |
| 位移 | `lsl/lsr/asr reg, reg, (reg/imm)`；左移 = ×2ⁿ；asr = 有號 ÷2ⁿ（向下捨入）；lsr = 無號 ÷2ⁿ |
| 沒有算術左移 | 符號位一定會被蓋掉 |
| 位移代替乘除 | 乘除很慢、位移很快；6x = 2x + 4x；7.5x = 8x − x/2 |
| ld / st | `ld reg, imm[reg]`：reg ← [reg + imm]；`st reg, imm[reg]`：[reg + imm] ← reg |
| 陣列 | arr[n] 在 base + 4n |
| 分支 | `b`、`beq`（E = 1）、`bgt`（GT = 1） |
| 翻譯方法 | 先寫 C → 暫存器分配 → 一行一行翻 |
| Modifier | 預設 sign-extend；u：補 0；h：放高 16 位；不能用在位移指令 |
| 32 位元常數 | `movh r0, 高16位` ＋ `addu r0, r0, 低16位` |
| NOR | 課本 Example 28：NOT(a OR b) = or ＋ not |

---

## 18. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| sp（r14）、ra（r15） | **第 8 支**：函式、call、ret、stack |
| 指令怎麼編成 32 位元 | **第 8 支**：SimpleRisc 的編碼 |
| ARM 的 cmp、條件分支、位移 | **第 9–10 支**：Ch4 ARM |
| x86 的 flags 暫存器 | **第 11–14 支**：Ch5 x86 |
| RISC-V 用 lui＋addi 載入 32 位元常數（和 movh＋addu 一樣的想法） | **第 15–17 支**：Ch6 RISC-V |
| 硬體乘除法為什麼慢 | **第 21–26 支**：課本 Ch8 計算機算術 |
| 實作 SimpleRisc 的處理器 | **第 27–29 支**：課本 Ch9 處理器設計 |

---

## 附：本講一句話總結

> **SimpleRisc 是本書從零設計、只有 21 條指令的 RISC 式組合語言（最接近 ARM 和 MIPS）：16 個 32 位元暫存器（r14 = sp、r15 = ra）加上程式設計師看不到、只由 cmp 設定的 flags.E 與 flags.GT；mov、3-address 的 add／sub／mul／div／mod／and／or／lsl／lsr／asr（只有最後一個來源可以是 16 位元有號立即值）、2-address 的 not 與 cmp；用 base-offset 定址的 ld／st 存取陣列（arr[n] 在 base + 4n）；b、beq、bgt 做分支。翻譯 C 的方法是先寫 C、做暫存器分配、再逐行翻譯（實測階乘、質數、最小公倍數都正確）。左移等於乘 2ⁿ、算術右移等於有號數除以 2ⁿ（實測負的奇數會向下捨入，和 C 的除法差 1）、邏輯右移是無號數除以 2ⁿ，因為乘除很慢，compiler 會用位移代替（gcc 把 x × 6 編成 lea 加 add）。16 位元立即值預設做 sign extension，u modifier 改成補 0、h modifier 放到高 16 位，所以 movh 加 addu 兩條指令就能載入任意 32 位元常數。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 3: Assembly Language, Part 2/3 錄影逐字稿（YouTube 自動字幕，1:26:30）、Sarangi *Basic Computer Architecture* v3.09 Ch3 §3.3.1–3.3.9、§3.3.13（p.107–114、p.124–125）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
