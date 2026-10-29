# Basic Computer Architecture 第十六支影片完整筆記：RISC-V 的分支、跳躍與 load／store（RISC-V Chapter, Part 2/3 → 課本 Ch6）

> **課程**：Basic Computer Architecture — RISC-V Chapter: Branch and Memory Instructions (Part II/III)（播放清單第 16 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕一直下載不到）（1:00:48）＋ 課本 *Basic Computer Architecture* v3.09 **Ch6** §6.3（p.235–245）
> **章號**：影片的「RISC-V Chapter」沒有章號，是課本 v3.09 新加的 **Chapter 6**。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（llvm-mc 14 組譯 RV32IM、自己寫的 `rv32sim.py` 執行；Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 沒有 flags 暫存器](#1-沒有-flags-暫存器)
- [2. slt 家族：把比較結果放進一般暫存器](#2-slt-家族把比較結果放進一般暫存器)
- [3. 條件分支：當場比較](#3-條件分支當場比較)
- [4. 兩種做法的取捨](#4-兩種做法的取捨)
- [5. 例子：用 sltu 做 64 位元加法](#5-例子用-sltu-做-64-位元加法)
- [6. 例子：1 加到 10](#6-例子1-加到-10)
- [7. jal：跳躍兼呼叫](#7-jal跳躍兼呼叫)
- [8. jalr：回來兼間接跳躍](#8-jalr回來兼間接跳躍)
- [9. 例子：完全平方數、質數、數 1](#9-例子完全平方數質數數-1)
- [10. 函式呼叫](#10-函式呼叫)
- [11. lw、sw 與 la](#11-lwsw-與-la)
- [12. 例子：位址計算、複製陣列、陣列加總](#12-例子位址計算複製陣列陣列加總)
- [13. 遞迴的階乘](#13-遞迴的階乘)
- [14. 【實測】](#14-實測)
- [15. 一頁速查表](#15-一頁速查表)
- [16. 與後續講次的連結](#16-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

RISC-V 的第 2 講。上一講（[第 15 支](lecture15_note.md)）是機器模型和整數指令，這一講是**控制轉移**和**記憶體**指令：

| 類別 | 指令 |
|---|---|
| 比較 | `slt`、`slti`、`sltu`、`sltiu` |
| 條件分支 | `beq`、`bne`、`bge`、`blt`、`bgeu`、`bltu` |
| 無條件跳躍 | `jal`、`jalr` |
| 記憶體 | `lw`、`sw`；組譯器指令 `la` |

[01:48] 【口頭】這是書裡第四章組合語言了，老師假設大家都熟了，所以講得比較快。

---

## 1. 沒有 flags 暫存器

[02:20] 其他架構（SimpleRisc、ARM、x86）的做法：

1. 比較兩個暫存器。
2. 結果存在 **flags 暫存器**（處理器內部的、看不到）。
3. 後面的條件分支看 flags 決定跳不跳。

[03:32] 好處：**比一次，很多個分支都能用**，而且可以隔很多條指令。

[04:07] RISC-V 有 32 個暫存器，想要更多控制，而且隱藏的暫存器有一個問題：它必須是 **context** 的一部分。process 切換時，要用特權指令讀出來、存起來、再還原。RISC-V 避開了這些麻煩。

【課本補充】課本 p.235：RISC-V 沒有 flags 暫存器；比較的運算元通常直接寫在 branch 指令裡，連同 branch target。

---

## 2. slt 家族：把比較結果放進一般暫存器

[04:32] 課本 Table 6.8（p.235）：

| 指令 | 例子 | 意思 |
|---|---|---|
| `slt rd, rs1, rs2` | `slt x1, x2, x3` | x2 < x3 就把 x1 設成 1，否則 0 |
| `slti rd, rs1, imm` | `slti x1, x2, 5` | x2 < 5 |
| `sltu rd, rs1, rs2` | `sltu x1, x2, x3` | 當成 **unsigned** 比 |
| `sltiu rd, rs1, imm` | `sltiu x1, x2, 5` | unsigned、跟常數比 |

- [05:00] 比較結果放在一個**看得到**的暫存器裡（不是隱藏的 flags），所以 context 存還原很簡單。
- [05:25] 還多一個彈性：可以**自己手動**設那個暫存器。
- [07:00] 之後的分支就拿它跟 zero 比：是 1 就表示「小於」，是 0 就是「大於或等於」。

⚠️ 課本 Table 6.8 寫成 **`sltui`**，影片也說「sltui」。本機 llvm-mc 不認：`unrecognized instruction mnemonic, did you mean: … sltiu`。正確的指令名稱是 **`sltiu`**（i 在前、u 在後）。見 [§14.1](#141-組譯器的意見)。

[09:40] 課本 Example 101（p.235–236）：2 < 5 嗎？

```
addi t0, zero, 2
addi t1, zero, 5
slt  t2, t0, t1      # t2 = (t0 < t1) = 1
```

---

## 3. 條件分支：當場比較

[07:18] 課本 Table 6.9（p.236）：6 條，都是**兩個暫存器 ＋ 一個 label**：

| 指令 | 跳的條件 |
|---|---|
| `beq rs1, rs2, label` | rs1 = rs2 |
| `bne` | rs1 ≠ rs2 |
| `bge` | rs1 ≥ rs2（有號） |
| `blt` | rs1 < rs2（有號） |
| `bgeu` | rs1 ≥ rs2（無號） |
| `bltu` | rs1 < rs2（無號） |

[07:47] 例如 `beq x1, x2, .foo`：x1 等於 x2 就跳到 `.foo`。不依賴之前的比較，**當場**比。

【課本補充】課本 p.237：ARM 也有類似的 unsigned 比較，但是靠特別的 flags 做的。

---

## 4. 兩種做法的取捨

[08:51]

| 做法 | 好處 |
|---|---|
| 先 `slt` 存起來，之後拿它跟 zero 比 | 比一次，可以給好幾個分支用 |
| branch 指令當場比 | 大部分情況只比一次，這樣最直接 |

[09:18] 大部分程式只比一次，所以第二種比較有道理。

---

## 5. 例子：用 sltu 做 64 位元加法

[10:43] 課本 Example 102（p.236）：兩個 64 位元的數，各用兩個 32 位元暫存器存：⟨t1, t0⟩ + ⟨t3, t2⟩ = ⟨t5, t4⟩（t0、t2 是低位）。

[11:21] 分開加低位和高位都簡單，**唯一的問題是低位有沒有進位**。

```
add  t4, t0, t2      # 低 32 位元
add  t5, t1, t3      # 高 32 位元
sltu t6, t4, t0      # t4 < t0（unsigned）→ 有進位
add  t5, t5, t6      # 把進位加上去
```

[12:46] 為什麼 `t4 < t0` 就表示有進位？畫一條 0 到 2ⁿ − 1 的數線（n = 32），從 t0 開始往右走 t2 步。走過 2ⁿ − 1 就會**繞回** 0 那邊（溢位），這時結果 t4 會比 t0 **小**。這就是進位。（跟 t2 比也一樣。）

[14:09] 學到的：`sltu` 本身就很有用，它存的那 1 個 bit 在這裡剛好就是**進位**。

（本機實測：0x2_FFFFFFFF + 1 = 0x3_00000000，見 [§14.2](#142-課本的例子)。）

---

## 6. 例子：1 加到 10

[15:10] 課本 Example 104（p.237）：

```
addi t0, zero, 1      # index = 1
addi s0, zero, 0      # sum = 0
addi t1, zero, 10     # 結束值
.loop:
add  s0, s0, t0
addi t0, t0, 1
bge  t1, t0, .loop    # 10 ≥ index 就繼續
```

[15:33] ⚠️ 老師口誤說「t1 小於等於」，馬上更正：是 t1（10）**大於等於** t0（index）就繼續。

[16:07] 6 行，真正的迴圈只有 3 行。跟之前的組合語言一模一樣。

【課本補充】課本 Example 103（p.237）用同樣的結構算階乘：`mul a0, a0, t0`、`addi t0, t0, 1`、`bge a1, t0, .loop`。

---

## 7. jal：跳躍兼呼叫

[16:56] 【口頭】RISC-V 的指令都很「聰明」：一條指令能做很多事。

[17:21] **jal**（jump and link）：一個暫存器 ＋ 一個 label。

```
jal rd, label        # 跳到 label，把 return address（pc + 4）存進 rd
```

| rd | 效果 |
|---|---|
| `x0`（zero） | 什麼都不存 → 一般的**無條件跳躍** |
| 其他（例如 ra） | 存了 return address → **函式呼叫** |

[17:39] 為什麼 x0 就不存？x0 是寫死的 0，寫不進去。

【課本補充】課本 p.240：label 會被編成 PC 相對的 offset，`jal` 能跳 **±1 MB** 的範圍。

---

## 8. jalr：回來兼間接跳躍

[18:37] **jalr**：兩個暫存器 ＋ 一個 offset。

```
jalr x1, x2, 20      # 跳到 x2 + 20，return address 存進 x1
```

- [19:08] 可以當**間接跳躍**（indirect branch）：目標是「暫存器 ＋ 固定 offset」。在其他指令集裡這有點麻煩，這裡很直接。【口頭】函式指標就靠這個：指標放在 x2。
- [19:53] rd = 0：不存 return address，就是單純的間接跳躍。
- [20:22] 當 **return** 用：

| return address 在哪 | 怎麼寫 |
|---|---|
| ra | `jalr zero, 0(ra)` |
| stack 上（例如 sp − 8） | 先讀出來，或用 sp 和 offset |

【課本補充】課本 p.240：`jalr` 可以當函式的 return，跳到 `0(ra)`。⚠️ 課本 Table 6.10 的標題寫成「Jump instructions in **x86**」，應該是 RISC-V；內文的「jal x1, x2, 20」也應該是 **jalr**。兩個都是筆誤。

⚠️ 本機的 llvm-mc 會把 `jalr zero, 0(ra)` 顯示成 **`ret`**：`ret` 是 RISC-V 的 pseudo instruction，就是這一條。見 [§14.1](#141-組譯器的意見)。

---

## 9. 例子：完全平方數、質數、數 1

### 9.1 完全平方數

[21:41] 課本 Example 107（p.239）：a1 是不是完全平方數，結果放 a0。

```
addi a0, zero, 0       # 先假設不是
addi t1, zero, 1       # index
.loop:
mul  t2, t1, t1        # index²
beq  t2, a1, .square   # 相等：是
addi t1, t1, 1
blt  a1, t2, .end      # a1 < index²：已經超過了，不是
jal  zero, .loop
.square:
addi a0, a0, 1
.end:
```

[23:52] 例：a1 = 11。3² = 9 不是；4² = 16 > 11，「錯過機會了」，不是完全平方數。

[24:33] `jal zero, .loop`：不用存 return address，所以第一個參數是 zero。

### 9.2 質數

[24:56] 課本 Example 105（p.237–238）：從 2 試到 a1 − 1，有一個能整除就不是質數。

```
addi t0, zero, 2        # 除數從 2 開始
.loop:
rem  t1, a1, t0         # 餘數
beq  t1, zero, .notPrime
addi t0, t0, 1
bne  t0, a1, .loop      # 試到 a1 − 1
addi a0, zero, 1        # 是質數
jal  x0, .end
.notPrime:
addi a0, zero, 0
.end:
```

[26:27] `jal` 的 l 是 link：跳過去並把 return address「連」起來。這裡第一個暫存器是 x0，所以沒存。

⚠️ 本機實測：a1 = **2** 時這段程式回答「不是質數」（2 % 2 = 0，馬上跳到 `.notPrime`）。課本題目沒說 a1 要大於 2；要正確處理 2，迴圈要先檢查除數是否已經等於 a1。見 [§14.2](#142-課本的例子)。

### 9.3 數 1

[27:08] 課本 Example 106（p.238–239）：a1 是 32 位元的數，有幾個 1？

```
addi t0, zero, 0       # 計數器
addi t1, zero, 32      # 做 32 次
addi t2, zero, 1
addi a0, zero, 0       # 結果
.loop:
andi t3, a1, 1         # 取 LSB
srli a1, a1, 1         # 邏輯右移一位
beq  t3, t2, .inc      # LSB 是 1 → 加一
.lret:
addi t0, t0, 1
beq  t1, t0, .end
jal  zero, .loop
.inc:
addi a0, a0, 1
jal  zero, .lret
.end:
```

- [28:36] 跟 1 做 AND，其他位都變 0，只剩 LSB。
- [28:59] 一定要用**邏輯**右移（`srli`）：如果 MSB 是 1，算術右移會一直補 1 進來，次數就錯了。

---

## 10. 函式呼叫

[31:15] 課本 Example 108（p.240）：

```c
int foo() { return 2; }
void main() { int x = 3; int y = x + foo(); }
```

```
.foo:
    addi a0, zero, 2        # 回傳值 2
    jalr zero, 0(ra)        # return
.main:
    addi s0, zero, 3        # x = 3；用 s（saved）暫存器，因為中間要呼叫函式
    jal  ra, .foo           # 呼叫，return address 存在 ra
    add  s1, s0, a0         # y = 3 + 2 = 5
```

- [32:15] 這時 s 系列暫存器就重要了：跨過函式呼叫還要保留的值放 s。
- [32:38] 寫法是 `0(ra)`，不是 `ra, 0`，這是這條指令的語法。
- [32:55] 回來時不用再存 return address，所以 rd 是 zero。

[33:42] 課本 Example 109（p.240–241）：x 在 a1、n 在 a2，算 xⁿ 放 a0。

```
.power:
    addi a0, zero, 1       # 結果從 1 開始
    add  t1, zero, a2      # t1 = n（複製一份，不改參數）
    beq  t1, zero, .end    # n = 0 → 答案就是 1
.loop:
    mul  a0, a0, a1
    addi t1, t1, -1        # 沒有 subi，用 addi 加 −1
    bne  t1, zero, .loop
.end:
    jalr zero, 0(ra)
```

- [36:26] 很多指令集有一條簡單的 `ret`，return address 是隱含的。RISC-V「**什麼都不隱含，全部明寫**」。【口頭】這讓 compiler 比較好編譯、分析、最佳化：相依關係一眼就看得出來。
- [37:48] 【口頭】好習慣：**不要改參數**，複製到 t1 再改。除錯時隨時印參數都還是原來的值。

---

## 11. lw、sw 與 la

[38:41] 課本 Table 6.11（p.241），跟其他 RISC 指令集幾乎一模一樣：

| 指令 | 例子 | 意思 |
|---|---|---|
| `lw rd, imm(rs1)` | `lw x1, 32(sp)` | x1 ← mem[sp + 32] |
| `sw rs2, imm(rs1)` | `sw x1, 32(sp)` | mem[sp + 32] ← x1 |
| `la rd, label` | `la x1, pi` | x1 ← pi 的**位址**（組譯器指令） |

- [39:07] **store 永遠是特例**：它沒有目的暫存器，卻有兩個來源暫存器加一個 immediate，所以要另外一種格式。RISC-V 怎麼解決，第 17 支講編碼時看。
- [39:27] **la** 是 pseudo instruction（組譯器指令）。常常要在組合語言檔案裡定義常數，例如 `.pi 3.14`、`.C 452`。給常數一個位址，就能用一般的 load 讀它。其實內部本來就是這樣：常數存在記憶體某處，有一個位址。
- [40:38] 浮點數常數特別需要這樣（下一講）。
- [41:30] ⚠️ 老師提醒：`la` 只把**位址**放進暫存器；要拿到常數的值，還要自己再寫一條 load。

[42:14] 依機器是 little endian 還是 big endian，byte 的存放順序不同。

[43:18] 還有讀半字（half word）、byte 的指令，這裡不講。

【課本補充】課本 Example 114（p.245）：

```
val: .word 17          # 32 位元整數常數；浮點數用 .float
la   t1, val
lw   s0, 0(t1)         # s0 = 17
```

課本 p.244 說 `la` 實際上會被翻成好幾條組合語言指令。

---

## 12. 例子：位址計算、複製陣列、陣列加總

### 12.1 位址計算

[42:14] 課本 Example 110（p.242）：a0 ← mem[sp − s0 × 4 − 12]。

```
slli s0, s0, 2         # s0 × 4：左移 2 位就好，不用乘
add  s0, s0, 12        # s0 + 12
sub  t0, sp, s0
lw   a0, 0(t0)
```

⚠️ 課本第二行寫 `add s0, s0, 12`（`add` 配常數）。照 ISA 應該寫 `addi`；本機 llvm-mc 會自動把它當成 `addi`（組出來顯示 `addi s0, s0, 12`），所以可以組譯，但嚴格說是組譯器的寬容。

### 12.2 複製陣列

[44:27] 課本 Example 111（p.242）：a1 指向來源、a2 指向目的地，複製 10 個元素。

```
addi t1, zero, 0       # 計數器
addi t2, zero, 10
.loop:
lw   t0, 0(a1)
sw   t0, 0(a2)
addi a1, a1, 4         # 指到下一個元素
addi a2, a2, 4
addi t1, t1, 1
bne  t1, t2, .loop
```

[45:02] 【口頭】這裡老師「稍微違反了自己的規則」，直接改 a1、a2（參數），因為假設之後用不到，而且比較有效率。

[46:54] 為什麼加 4？一個元素是 32 位元 = 4 byte：0、4、8、12、16……

### 12.3 陣列加總

[46:54] 課本 Example 112（p.242–243）：`sum += a[idx]`，idx 從 0 到 9。

```
addi t0, zero, 0       # index
addi a0, zero, 0       # 結果
addi t1, zero, 10
.loop:
lw   t2, 0(a1)
add  a0, a0, t2
addi a1, a1, 4         # 移動陣列指標
addi t0, t0, 1
bne  t0, t1, .loop
```

[48:53] 為什麼移動指標、不用 index 算位址？index 每次加 1，位址要加 4。用 index 的話要「index 加 1、乘 4、加上起始位址」三條；移動指標只要兩條（指標加 4、index 加 1）。

[49:16] 【口頭】ARM 有捷徑：shifter operand，加法時可以順便把另一個運算元左移，就是為了這種情況設計的。RISC-V 沒有，但改用移動指標的方法一樣省一條。不同的指令集對常見模式（例如讀陣列元素）有不同的解法。

---

## 13. 遞迴的階乘

[51:00] 【口頭】「這是傳統」，前面三章都有，怎麼能不做。也是這一講最難的，之後就是浮點和編碼。

課本 Example 113（p.243–244）：

```
.fact:
    addi t1, zero, 1
    bge  t1, a1, .ltone      # 1 ≥ n（n ≤ 1）→ 答案 1
    add  s0, a1, zero        # s0 = n
    addi a1, a1, -1          # a1 = n − 1（新的參數）
    addi sp, sp, -8          # stack 上開 2 格
    sw   ra, 0(sp)
    sw   s0, 4(sp)
    jal  ra, .fact           # 遞迴：結果在 a0
    lw   s0, 4(sp)
    lw   ra, 0(sp)
    addi sp, sp, 8           # 把 stack 收回來
    mul  a0, a0, s0          # fact(n) = n × fact(n − 1)
    jalr zero, 0(ra)
.ltone:
    addi a0, zero, 1         # 0! = 1! = 1
    jalr zero, 0(ra)
.main:
    addi a1, zero, 5
    jal  ra, .fact
```

- [53:18] 終止條件：n ≤ 1（0! 和 1! 都是 1）。
- [55:04] 用 **s0** 存 n，因為要跨過遞迴呼叫保留它。
- [55:59] stack 上開兩格：**ra**（一定要存，不然回不去）和 **s0**（n 的值）。
- [56:59] 老師說這是 **caller saved** 的做法：呼叫的人（這一層）自己存、自己還原。
- [58:23] 還原後 a0 = fact(n − 1)、s0 = n，相乘就是 fact(n)。

[58:42] 兩個不變量：

1. 參數永遠在 **a1**：main 放 5，每次遞迴放 n − 1。
2. 回傳值永遠在 **a0**：終止時回傳 1，遞迴時回傳 n × fact(n − 1)。

⚠️ 照 [第 15 支 §7](lecture15_note.md#7-32-個整數暫存器) 的慣例，**s 暫存器是 callee saved**：被呼叫的函式如果要用 s0，應該先存起來、回去前還原給呼叫者。這段 `.fact` 一進來就改 s0，存的是**新的** s0，所以呼叫 `.fact` 的人（main）原本的 s0 會被蓋掉。本機實測：main 先放 s0 = 99，呼叫完 s0 變成 5。在這個例子 main 不在乎 s0 所以沒差，但不符合慣例。見 [§14.2](#142-課本的例子)。

[60:34] 下一講（第 3 講，RISC-V 的最後一講）：浮點指令和指令編碼。

---

## 14. 【實測】

程式在 `experiments/riscv/l16/`。用 llvm-mc 組譯成 RV32IM 機器碼（`experiments/riscv/build.sh`），再用第 15 支寫的 `experiments/riscv/rv32sim.py` 執行。程式從位址 0 開始跑，遇到 `ecall` 停。印出來的暫存器只列不是 0 的。

### 14.1 組譯器的意見

```
[sltui x1, x2, 5]    → error: unrecognized instruction mnemonic, did you mean: lui, slt, slti, sltiu, sltu?
[sltiu x1, x2, 5]    → sltiu ra, sp, 5      # encoding: [0x93,0x30,0x51,0x00]
[add s0, s0, 12]     → addi  s0, s0, 12     # encoding: [0x13,0x04,0xc4,0x00]
[jalr zero, 0(ra)]   → ret                  # encoding: [0x67,0x80,0x00,0x00]
[jalr x1, x2, 20]    → jalr  20(sp)         # encoding: [0xe7,0x00,0x41,0x01]
[jalr x1, 20(x2)]    → jalr  20(sp)
```

- 課本和影片的 `sltui` 不存在，是 `sltiu`。
- 課本的 `add s0, s0, 12` 被當成 `addi`。
- `jalr zero, 0(ra)` 就是 pseudo instruction `ret`；`jalr x1, x2, 20` 和 `jalr x1, 20(x2)` 兩種寫法都接受，組出同一串 byte（rd 是 ra 時，llvm-mc 省略不印）。

### 14.2 課本的例子

| 程式 | 課本 | 輸入 | 結果 |
|---|---|---|---|
| `add64.s` | Ex 102 | ⟨0, 1⟩ + ⟨2, 0xFFFFFFFF⟩ | t4 = 0、t5 = 3、t6（進位）= 1 → **0x3_00000000** ✓ |
| `fact_loop.s` | Ex 103 | a1 = 6 | a0 = **720** ✓ |
| `sum10.s` | Ex 104 | — | s0 = **55** ✓ |
| `prime.s` | Ex 105 | a1 = 7、97 | a0 = 1 ✓（97 要跑 384 條指令） |
| | | a1 = 9 | a0 = 0 ✓ |
| | | a1 = **2** | a0 = 0 ❌（2 是質數） |
| `popcount.s` | Ex 106 | 0xF0F0F0F0、0xFFFFFFFF、1 | a0 = 16、32、1 ✓ |
| `square.s` | Ex 107 | 101、100、11、16 | a0 = 0、1、0、1 ✓ |
| `call.s` | Ex 108 ＋ 109 | — | s1 = **5**、a0 = 7³ = **343** ✓ |
| `array.s` | Ex 111 ＋ 112 ＋ 114 | 1..10 | 複製後加總 a0 = **55**、`la`＋`lw` 讀到 s0 = **17** ✓ |
| `fact_rec.s` | Ex 113 | a1 = 5 | a0 = **120** ✓ |
| | | 先放 s0 = 99 | 呼叫完 s0 = **5**（被蓋掉） |

- 全部照抄課本（只把 `.main` 搬到最前面、結尾加 `ecall`），答案都對。
- 兩個邊界情況：質數測試對 2 回答錯（[§9.2](#92-質數)）；遞迴階乘沒有替呼叫者保留 s0（[§13](#13-遞迴的階乘)）。
- `popcount` 不管輸入是什麼都固定跑 32 圈：198–260 條指令，差別只在有幾個 1 要多跳去 `.inc`。

---

## 15. 一頁速查表

| 主題 | 重點 |
|---|---|
| flags | RISC-V 沒有；比較結果放一般暫存器或當場比 |
| 為什麼 | 隱藏的 flags 要算進 context，切換時麻煩 |
| slt 家族 | slt、slti、sltu、**sltiu**（⚠️ 課本寫 sltui） |
| 條件分支 | beq、bne、bge、blt、bgeu、bltu；兩個暫存器 ＋ label |
| 64 位元加法 | 低位和高位分開加；`sltu t6, t4, t0` 就是進位 |
| jal rd, label | 存 pc + 4 到 rd；rd = zero 就是一般跳躍；±1 MB |
| jalr rd, rs1, off | 跳到 rs1 + off；return：`jalr zero, 0(ra)` = `ret`；函式指標 |
| 全部明寫 | 沒有隱含的 ret，方便 compiler 分析 |
| s 暫存器 | 跨呼叫保留的值放這裡（callee saved） |
| lw／sw | `lw rd, imm(rs1)`、`sw rs2, imm(rs1)`；store 要特別的格式 |
| la | 載入 label 的位址（pseudo）；還要自己 lw 才拿到值 |
| 常數 | `val: .word 17`、`.float` |
| 陣列 | 移動指標（+4）比用 index 算位址省一條 |
| 遞迴 | ra 和 s0 存在 stack；參數在 a1、回傳在 a0 |
| 實測 | 課本例子全部跑對；sltui 不存在；質數測試 2 會錯；fact 會蓋掉呼叫者的 s0 |

---

## 16. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| RISC-V 暫存器、ABI 名稱、caller／callee saved | [第 15 支](lecture15_note.md) |
| SimpleRisc 的 cmp、flags、call／ret | [第 7 支](lecture07_note.md)、[第 8 支](lecture08_note.md) |
| ARM 的 flags、shifter operand | [第 9 支](lecture09_note.md)、[第 10 支](lecture10_note.md) |
| x86 的 cmp／jcc 和 unsigned 條件碼 | [第 12 支](lecture12_note.md) |
| 浮點指令、編碼（store 的 S 格式、branch 的 B 格式） | [第 17 支](lecture17_note.md) |
| 為什麼 PC 相對、context 要存什麼 | [Linux OS 第 2 講 §7](../../os/linux-os/lecture02_note.md#7-context要存哪些東西) |
| Branch 在 pipeline 裡的代價 | [第 30 支](lecture30_note.md)、[第 31 支](lecture31_note.md) |

---

## 附：本講一句話總結

> **RISC-V 沒有 flags 暫存器：比較結果可以用 slt／slti／sltu／sltiu 明確放進一般暫存器（好處是 context 好存，而且 sltu 存的那個 bit 剛好就是 64 位元加法的進位），或者直接用 beq／bne／bge／blt／bgeu／bltu 拿兩個暫存器當場比較再跳；無條件跳躍只有兩條：jal rd, label 跳過去並把 pc + 4 存進 rd（rd 是 zero 就是一般跳躍、是 ra 就是函式呼叫），jalr rd, rs1, offset 跳到 rs1 + offset（可以當 return `jalr zero, 0(ra)`，也可以當函式指標的間接跳躍），什麼都明寫、沒有隱含的 ret；記憶體只有 base-offset 的 lw、sw（store 因為兩個來源暫存器要另一種格式），常數用 `.word` 定義、用 la 拿位址再 lw。本機用 llvm-mc 和自己的模擬器跑課本所有例子都對，但也發現課本的 sltui 應該是 sltiu、質數測試對 2 會答錯、遞迴階乘沒有替呼叫者保留 callee-saved 的 s0。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」RISC-V Chapter: Branch and Memory Instructions (Part II/III) 的 whisper 轉錄稿（本機 `large-v3-turbo`，1:00:48）、Sarangi *Basic Computer Architecture* v3.09 Ch6 §6.3（p.235–245）整理。實測為本機（llvm-mc 14、`rv32sim.py`、Python 3.10）結果。*
