# Basic Computer Architecture 第九支影片完整筆記：ARM 的機器模型與資料處理指令（Chapter 4, Part 1/2）

> **課程**：Basic Computer Architecture — Chapt 4: ARM assembly, Part 1/2（播放清單第 9 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（55:59，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 Ch4 §4.1–4.2.5（p.139–148）
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、GNU as `arm-none-eabi-as`、gcc 11.4、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點
>
> ⚠️ 這支影片的英文字幕品質很差，看起來像是從別的語言**機器翻譯**回英文的（例如把 Von Neumann 寫成「Vonnegut」、把 link register 寫成「reference register」、把 ror 寫成「right turn」）。下面的內容以課本對照校正；逐字稿聽不清楚的地方標 ⚠️。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 為什麼要學 ARM](#1-為什麼要學-arm)
- [2. ARM 的機器模型：16 個暫存器](#2-arm-的機器模型16-個暫存器)
- [3. mov 與 mvn](#3-mov-與-mvn)
- [4. 算術指令：add、sub、rsb](#4-算術指令addsubrsb)
- [5. 邏輯指令：and、eor、orr、bic](#5-邏輯指令andeororrbic)
- [6. 乘法：mul、mla、smull、umull](#6-乘法mulmlasmullumull)
- [7. 【課本補充】除法：sdiv、udiv](#7-課本補充除法sdivudiv)
- [8. Shifter operand：lsl、lsr、asr、ror](#8-shifter-operandlsllsrasrror)
- [9. 比較指令與 CPSR](#9-比較指令與-cpsr)
- [10. s 後綴：讓一般指令設定 flags](#10-s-後綴讓一般指令設定-flags)
- [11. 用到 carry 的指令：adc、sbc、rsc](#11-用到-carry-的指令adcsbcrsc)
- [12. 【實測】](#12-實測)
- [13. 一頁速查表](#13-一頁速查表)
- [14. 與後續講次的連結](#14-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

ARM 這章分五部分（[05:36]）：

1. 基本指令 ← **本講**
2. 進階指令（shifter operand、比較、flags）← **本講**
3. 分支指令 ← 第 10 支
4. 記憶體（load／store）指令 ← 第 10 支
5. 指令編碼 ← 第 10 支

和 SimpleRisc 最大的不同：

| | SimpleRisc | ARM |
|---|---|---|
| 暫存器 | 16 個，PC 看不到 | 16 個，**PC 是 r15，看得到** |
| 立即值 | 直接寫 | 前面加 **`#`** |
| 位移 | 獨立的 lsl／lsr／asr 指令 | **嵌在運算元裡**（shifter operand） |
| flags | 只有 cmp 設定；E、GT | cmp 等 4 種比較指令 ＋ **任何加 s 的指令**；N、Z、C、V |
| 特殊指令 | 沒有 | rsb、bic、mla、smull、adc… |

---

## 1. 為什麼要學 ARM

[00:34]–[05:44]　課本 Ch4 開頭（p.139）

- ARM 是非常流行的指令集：截至 2016 年，幾乎所有手機、平板，以及各種不是桌機或筆電的小型運算裝置都用 ARM 處理器。
  - 【課本補充】截至 2012 年，ARM 在智慧型手機和平板的市占超過 90%；硬碟、電視的機上盒也大量使用 ARM。
- ARM 是英國劍橋的公司。它**授權**自己的設計，由其他廠商做成晶片。
  - 【口頭】例：ARM 把處理器設計賣給手機公司，手機公司可以在旁邊加上處理相機輸入的電路、和加速度計溝通的電路等。
- **先修**：要讀過 Chapter 3，懂組合語言怎麼寫、指令怎麼運作、函式、stack、編碼。
  - 【口頭】「同時介紹組合語言和教一個複雜的組合語言是不可能的，最好分開。」所以這章的速度會比較快。
- ARM 指令集很多樣：整數指令、浮點指令、**向量擴充**（一次做好幾個加法）。影片說 Chapter 11 會講向量指令，⚠️ 課本 v3.09 是 **Ch12**。這些擴充本講不討論。
- **Thumb** 指令集：比本章講的還簡單一點的子集，很多 ARM 程式其實用 Thumb。也不討論。
  - 【課本補充】課本註腳：大部分跑在處理器上的 ARM 程式碼其實是 **Thumb-2**，所以先徹底理解本章的內容很重要。
- 【課本補充】ARM 是 **32 位元**指令集（暫存器和位址都是 32 位元）；RISC，結構非常規律；**每條指令都是 32 位元**；所有算術和邏輯運算**只用暫存器**；暫存器和記憶體之間**只能靠 load 和 store** 溝通。

---

## 2. ARM 的機器模型：16 個暫存器

[05:44]–[08:44]　課本 §4.1、Table 4.1（p.139–140）

- 和 SimpleRisc 一樣有 **16 個**暫存器 r0～r15，每個 32 位元。
- 記憶體是標準的 Von Neumann 架構：指令和資料在同一個記憶體。
- **和 SimpleRisc 不同：PC 是程式設計師看得到的暫存器**，軟體可以直接讀寫它。

| 暫存器 | 縮寫 | 名稱 |
|---|---|---|
| r11 | fp | **frame pointer** |
| r12 | ip | intra-procedure-call scratch register |
| r13 | **sp** | stack pointer |
| r14 | **lr** | **link register**（= return address register） |
| r15 | **pc** | program counter |

- r12：存暫存值的 scratch 暫存器；【課本補充】**caller 和 callee 都不需要保存它**。
- r11（frame pointer）：【課本補充】指向 activation block 的頂端。影片說下一章 x86 會講更多。
- 【課本補充】**r11 和 r12 的特殊用途是 GNU compiler 約定的，不是 ARM ISA 規定的**。r0～r12 都是一般用途；真正有特殊角色的只有 sp、lr、pc。
- 本章用 **little endian** 版本的 ARM，語法用 **GNU ARM assembler**。
- 【口頭】這章每種指令都用一張表說明：語意、例子、用 register transfer notation 的解釋。

---

## 3. mov 與 mvn

[08:44]–[11:05]　課本 Table 4.2（p.140）

| 語意 | 例子 | 解釋 |
|---|---|---|
| `mov reg, (reg/imm)` | `mov r1, r2` | r1 ← r2 |
| | `mov r1, #3` | r1 ← 3 |
| `mvn reg, (reg/imm)` | `mvn r1, r2` | r1 ← ~r2 |
| | `mvn r1, #3` | r1 ← ~3 |

- **ARM 的立即值前面一定要加 `#`**。【口頭】「# 在鍵盤的 3 上面。」
- **mvn**（move not）：把每個位元**反過來**再搬，就像 SimpleRisc 的 not。例：`0110` 的反是 `1001`。
- mov 和 mvn 都是 **2-address** 指令。

---

## 4. 算術指令：add、sub、rsb

[11:05]–[17:13]　課本 Table 4.3、Example 37（p.140–141）

| 語意 | 例子 | 解釋 |
|---|---|---|
| `add reg, reg, (reg/imm)` | `add r1, r2, r3` | r1 ← r2 + r3 |
| `sub reg, reg, (reg/imm)` | `sub r1, r2, r3` | r1 ← r2 − r3 |
| `rsb reg, reg, (reg/imm)` | `rsb r1, r2, r3` | r1 ← **r3 − r2** |

- 和 SimpleRisc 一樣：第一個是目的地，第二個是第一個來源（暫存器），第三個可以是暫存器或立即值。例：`add r1, r2, #3`。
- 【口頭】「我故意講快一點，因為假設大家已經懂第 3 章。」

**為什麼要 rsb（reverse subtract，反向減法）？**（[13:53]）

- 想算 **3 − r1**。但 `sub r2, 3, r1` **不合法**：第二個運算元一定要是暫存器，只有最後一個可以是立即值。
- 在 SimpleRisc 要兩條指令：先算 r1 − 3，再乘 −1。
- ARM 用 `rsb r2, r1, #3` 一條就算出 3 − r1。【口頭】「ARM 的設計者很聰明。」

**Example 37**：算 4 + 5 − 19，存到 r1

```
@ 簡單但不是最好的寫法
mov r1, #4
mov r2, #5
add r3, r1, r2
mov r4, #19
sub r1, r3, r4

@ 最好的寫法
mov r1, #4
add r1, r1, #5      @ r1 = 9
sub r1, r1, #19     @ r1 = −10
```

---

## 5. 邏輯指令：and、eor、orr、bic

[17:13]–[22:18]　課本 Table 4.4、Example 38（p.141–142）

| 語意 | 例子 | 解釋 |
|---|---|---|
| `and reg, reg, (reg/imm)` | `and r1, r2, r3` | r1 ← r2 AND r3 |
| `eor reg, reg, (reg/imm)` | `eor r1, r2, r3` | r1 ← r2 **XOR** r3 |
| `orr reg, reg, (reg/imm)` | `orr r1, r2, r3` | r1 ← r2 OR r3 |
| `bic reg, reg, (reg/imm)` | `bic r1, r2, r3` | r1 ← r2 AND (~r3) |

- **eor** = exclusive OR。
- 【口頭】ARM 的指令名稱都是**三個字母**：SimpleRisc 有 `or`、`ld`、`st` 這種兩個字母的，ARM 統一成三個字母，所以 or 變成 **orr**。
- **bic**（bit clear，清除位元）：**r3 裡是 1 的那些位置，在結果裡都變成 0**；其他位置照抄 r2。
  - 例：r2 = `1101`、r3 = `0110` → ~r3 = `1001` → `1101` AND `1001` = `1001`。r3 的第 1、2 位是 1，所以結果的第 1、2 位被清掉。

**Example 38**：計算 NOT(A ∨ B)，A、B 是 1 位元的 Boolean 值，A = 0、B = 1，結果放 r0

```
mov r0, #0x0        @ A
orr r0, r0, #0x1    @ A OR B
mvn r0, r0          @ NOT(A OR B)
```

- ARM 也接受**十六進位**的立即值：`#0x…`。
- ⚠️ 因為 A、B 是「1 位元」的 Boolean 值，正確答案應該是 0；但 `mvn` 會把 **32 個位元全部反過來**，r0 實際上是 `0xFFFFFFFE`（−2），只有最低位是 0。如果要真的得到 1 位元的 0，可以改用 `eor r0, r0, #1`（只反最低位）。§12.2 實測。

---

## 6. 乘法：mul、mla、smull、umull

[22:18]–[30:18]　課本 Table 4.5、Example 39（p.142–143）

| 語意 | 例子 | 解釋 |
|---|---|---|
| `mul reg, reg, reg` | `mul r1, r2, r3` | r1 ← r2 × r3 |
| `mla reg, reg, reg, reg` | `mla r1, r2, r3, r4` | r1 ← r2 × r3 + r4 |
| `smull reg, reg, reg, reg` | `smull r0, r1, r2, r3` | r1:r0 ← r2 ×**有號** r3（64 位元） |
| `umull reg, reg, reg, reg` | `umull r0, r1, r2, r3` | r1:r0 ← r2 ×**無號** r3（64 位元） |

**mla（multiply and accumulate，乘加）**：

- 【口頭】ARM 是 RISC，但也有一些比較複雜的指令。這是一個取捨：永遠追求簡單，還是讓指令稍微複雜一點、照顧程式裡常見的模式？
- d = a + b × c 這種模式在**線性代數**、矩陣運算裡非常常見。
- mla 是 **4-address** 指令。

**為什麼要 smull／umull**（[24:23]）：

- 兩個 32 位元數相乘，結果可能需要 **64 位元**：
  - 有號：(−2³¹) × (−2³¹) = **2⁶²**，遠超過 32 位元的範圍 → overflow。
  - 無號：最大的 32 位元無號數是 2³² − 1，平方 = 2⁶⁴ − 2³³ + 1。
  - 不管有號還是無號，**64 位元一定放得下**。
- mul 和 mla 只留低 32 位，其餘**丟掉**。
- smull／umull 把 64 位元的結果存在**兩個暫存器**：第一個（r0）放**低 32 位**，第二個（r1）放**高 32 位**。
- smull 和 umull 唯一的差別：一個當有號數乘，一個當無號數乘。

【課本補充】所有乘法指令的運算元**都必須是暫存器**（不能用立即值）；而且**第一個來源暫存器不能和目的暫存器一樣**。§12.1 實測：在 ARMv4 上寫 `mul r1, r1, r2`，GNU as 會警告 `Rd and Rm should be different in mul`。

**Example 39**：計算 12³ + 1，存到 r3

```
/* load test values */
mov r0, #12
mov r1, #1
/* perform the logical computation */
mul r4, r0, r0        @ 12*12
mla r3, r4, r0, r1    @ 12*12*12 + 1
```

- ARM 組合語言用 **`@`** 開始註解。
- 載入數值之後，只要**兩條指令**。【口頭】要多看看 ARM 提供的這些擴充指令（mla、smull、umull），可以簡化計算、減少程式碼行數。「在組合語言裡，行數越少，程式越有效率。」

---

## 7. 【課本補充】除法：sdiv、udiv

課本 Table 4.6（p.143），影片沒講。

| 語意 | 例子 | 解釋 |
|---|---|---|
| `sdiv reg, reg, reg` | `sdiv r1, r2, r3` | r1 ← r2 ÷ r3（有號） |
| `udiv reg, reg, reg` | `udiv r1, r2, r3` | r1 ← r2 ÷ r3（無號） |

- **比較新的** ARM 才有。兩者都只算**商**；餘數 = 被除數 − 除數 × 商。
- §12.1 實測：`.arch armv4` 下組譯 sdiv 會失敗（`selected processor does not support`）；`.arch armv7ve` 才可以。

---

## 8. Shifter operand：lsl、lsr、asr、ror

[30:18]–[41:10]　課本 §4.2.2、Figure 4.1（p.143–145）

資料處理指令的第二個運算元（operand 2）可以是三種：**暫存器、立即值、shifter operand**。

```
reg1, {lsl | lsr | asr | ror} {#shift_amt | reg2}
```

- 暫存器後面可以接一個**位移操作**：
  - 位移量是**立即值**（0～31），或
  - 位移量放在**另一個暫存器**裡

| 操作 | 意思 | 例子（5 位元，移 1 位） |
|---|---|---|
| **lsl** | logical shift left | `10110` → `01100` |
| **lsr** | logical shift right（MSB 補 0） | `10110` → `01011` |
| **asr** | arithmetic shift right（複製符號位） | `10110` → `11011` |
| **ror** | rotate right（從右邊掉出去的放回左邊） | `10110` → `01011` |

- 和 SimpleRisc 一樣，**沒有算術左移**。
- **ror 和其他三種的差別**：lsl、lsr、asr 會移入**原本沒有的新位元**（0 或符號位）；ror **不會**，掉出 LSB 的位元直接放回 MSB。
- 【課本補充】`ror #0` 有特殊意義，叫 **rrx**：右移一位，把 **carry flag** 放進 MSB，再把移出去的 LSB 放進 carry flag。§12.1 實測：`mov r1, r2, rrx` 的編碼就是位移量 0 的 ror。
- 【課本補充】負的奇數做 asr 要小心：−5 = `1011` asr 1 → `1101` = −3。「−5/2 = −3 算不算對，要看程式語言的語意。」（[第 7 支 §9](lecture07_note.md#9-位移指令lsllsrasr) 實測過，C 的 −5/2 是 −2。）

**ARM 沒有獨立的位移指令**（[37:14]）：

- lsl、lsr、asr、ror 是**位移的指示**，不是獨立的指令。位移**摺疊進運算元的定義裡**。

**Example 40**：r1 = r2 / 4（假設整除）

```
mov r1, r2, asr #2
```

- 用 **mov** 指令，`r2, asr #2` 雖然中間有逗號，但**整個算一個運算元**。
- 硬體先算這個運算元的值（r2 算術右移 2 位 = 除以 4），再搬到 r1。
- 除以 4 要用 **asr**，不是 lsr（有號數）。

**Example 41**：r1 = r2 + r3 × 4

```
add r1, r2, r3, lsl #2
```

- 乘以 4 = 左移 2 位。整個 `r3, lsl #2` 是**一個**運算元。
- 【口頭】這讓組合語言的執行**非常有效率**：一條指令做完加法和位移。

【課本補充】到目前為止的定址模式：

1. register：`r1`、`r2`
2. immediate：`#1`、`#2`
3. **scaled-register**：`r1, lsl #2`、`r1, lsl r2`

- 位移不會改變 r1 本身的值，只影響這個運算元。

---

## 9. 比較指令與 CPSR

[41:10]–[47:50]　課本 §4.2.3、Table 4.7（p.146–147）

| 語意 | 例子 | 解釋 |
|---|---|---|
| `cmp reg, (reg/imm)` | `cmp r1, r2` | 算 r1 − r2，設定 flags |
| `cmn reg, (reg/imm)` | `cmn r1, r2` | 算 r1 **+** r2，設定 flags（compare negative：和 −r2 比） |
| `tst reg, (reg/imm)` | `tst r1, r2` | 算 r1 **AND** r2，設定 flags |
| `teq reg, (reg/imm)` | `teq r1, r2` | 算 r1 **XOR** r2，設定 flags（測試相等） |

- 都是 2-address，結果**不存**，只設定 flags。
- SimpleRisc 的 flags 暫存器在 ARM 叫 **CPSR**（current program status register）。
- 【口頭】tst、teq、cmn 不常用，但可以用來減少指令數。各種情況的完整說明見 ARM 手冊。我們大部分只用 cmp。

**CPSR 的四個 flag**：

| Flag | 意思 |
|---|---|
| **N**（Negative） | 結果是負數（2's complement）時為 1 |
| **Z**（Zero） | 結果是 0 時為 1；比較時兩數相等就是 1 |
| **C**（Carry） | 見下面 |
| **V**（Overflow） | 真正的**有號** overflow／underflow |

- 【課本補充】CPSR 是 32 位元暫存器，這四個位元在第 29～32 位（課本從 1 算）。也叫 **condition code flags**。

**Carry flag 的規則**（[43:34]，【口頭】「這有點反直覺，所以我畫了一個大箭頭」）：

- **加法**：產生進位時 C = 1。
- **減法**：**要借位時 C = 0；不用借位時 C = 1**。
  - 例：0 − 5 要借位 → C = 0。
- 【課本補充】
  - 加法的 C 也可以理解成**無號 overflow**：−1（`1111`）+ −2（`1110`）= −3（`1101`），有號數沒有 overflow，但 MSB 有進位出去，C = 1。
  - 減法的 C = 0 也叫**無號 underflow**：0 − 1 對有號數沒問題，但 `0000` − `0001` 要借位。
  - **位移操作**：C 等於最後一個被移出去的位元。

§12.2 實測用 Python 照 ARM 的定義算了 NZCV。

---

## 10. s 後綴：讓一般指令設定 flags

[47:50]–[49:01]　課本 §4.2.4（p.147）

- SimpleRisc 只有 cmp 會設定 flags；ARM 除了比較指令，**任何資料處理指令加上 `s` 後綴**都會設定 CPSR flags。
- 例：`adds`、`subs`：照常做加法、減法，**另外**依結果設定 flags（負數？零？有進位？overflow？）。
- 一般的 `add`、`sub` **不會**設定 flags。

---

## 11. 用到 carry 的指令：adc、sbc、rsc

[49:01]–[55:51]　課本 §4.2.5、Table 4.8、Example 42、43（p.147–148）

| 語意 | 例子 | 解釋 |
|---|---|---|
| `adc reg, reg, reg` | `adc r1, r2, r3` | r1 = r2 + r3 + **C** |
| `sbc reg, reg, reg` | `sbc r1, r2, r3` | r1 = r2 − r3 − **NOT(C)** |
| `rsc reg, reg, reg` | `rsc r1, r2, r3` | r1 = r3 − r2 − NOT(C) |

- **adc**（add with carry）：如果前面產生了進位，這條指令會把它加進來。
- **sbc**（subtract with carry）：減完再減掉 **NOT(C)**。
  - 為什麼是 NOT(C)？因為減法要借位時 C = 0，所以 NOT(C) = 1 = **借位**。「NOT(C) 可以看成借位位元，兩者的值剛好相反。」
- **rsc**：反向減法 ＋ 借位，像 rsb。

**64 位元加法**（Example 42）：

- 【口頭】「我常把這題出成考試題，叫學生用 SimpleRisc 寫，很複雜、要很多行，因為 SimpleRisc 沒有支援。」
- smull、umull 會把結果放在一對暫存器，所以 64 位元數（long）常常存在兩個暫存器裡。
- 第一個 long 在 **r2:r1**（r1 放低 32 位、r2 放高 32 位），第二個在 **r4:r3**，結果放 **r6:r5**。

```
adds r5, r1, r3     @ 低 32 位相加，s：記錄 carry
adc  r6, r2, r4     @ 高 32 位相加，再加上 carry
```

- **adds 的 s 很重要**：我們只在乎 **carry flag**。有進位就要記下來，給 adc 用。
- 【口頭】如果沒有 adds 和 adc，這件事會相當困難。程式裡如果有很多 smull、umull 產生 64 位元結果，沒有這套機制，加減法會非常複雜。

**64 位元減法**（Example 43，影片留給讀者當習題，只給提示：「把 adds 換成 subs，再回到指令集找 sbc」）：

```
subs r5, r1, r3     @ 低 32 位相減，記錄借位（C = 0 表示借位）
sbc  r6, r2, r4     @ 高 32 位相減，再減掉 NOT(C)
```

§12.2 實測兩個都對；§12.3 看 x86 的 32 位元模式也是同樣的 add／adc 組合。

---

## 12. 【實測】

本機：Intel i7-13650HX、Ubuntu、kernel 6.5.0。沒有 32 位元 ARM 的 gcc 和模擬器，所以用 GNU ARM assembler（`arm-none-eabi-as`）確認指令的寫法和編碼，用 Python 模擬 flags。程式在 `experiments/lecture09/`。

### 12.1 本講所有指令都組譯得出來

```
$ arm-none-eabi-as -o basic.o basic.s && arm-none-eabi-objdump -d basic.o
   0:	e1a01002 	mov	r1, r2
   4:	e3a01003 	mov	r1, #3
   8:	e1e01002 	mvn	r1, r2
   c:	e3e01003 	mvn	r1, #3
  10:	e2821003 	add	r1, r2, #3
  14:	e2612003 	rsb	r2, r1, #3          ← r2 = 3 − r1
  18:	e1c21003 	bic	r1, r2, r3
  1c:	e0221003 	eor	r1, r2, r3
  20:	e1821003 	orr	r1, r2, r3
  24:	e0040090 	mul	r4, r0, r0
  28:	e0231094 	mla	r3, r4, r0, r1
  2c:	e0c10392 	smull	r0, r1, r2, r3
  30:	e0810392 	umull	r0, r1, r2, r3
  34:	e1a01142 	asr	r1, r2, #2          ← 原文是 mov r1, r2, asr #2
  38:	e0821103 	add	r1, r2, r3, lsl #2
  3c:	e1a010e2 	ror	r1, r2, #1          ← 原文是 mov r1, r2, ror #1
  40:	e1a01062 	rrx	r1, r2              ← 原文是 mov r1, r2, rrx
  44:	e1510002 	cmp	r1, r2
  48:	e3710001 	cmn	r1, #1
  4c:	e3110001 	tst	r1, #1
  50:	e1310002 	teq	r1, r2
  54:	e0915003 	adds	r5, r1, r3
  58:	e0a26004 	adc	r6, r2, r4
  5c:	e0515003 	subs	r5, r1, r3
  60:	e0c26004 	sbc	r6, r2, r4
```

- 每條指令都是 **4 byte**（位址每次加 4），和 SimpleRisc 一樣。
- `mov r1, r2, asr #2` 被反組譯器顯示成 `asr r1, r2, #2`：較新的 ARM 語法提供了 asr、ror 這些「指令」寫法，但看編碼（`e1a0…`），它其實就是 **mov 加 shifter operand**，和課本說的一樣。
- `rrx` 的編碼（`…062`）就是位移量 0 的 ror。
- 比較指令 cmp、cmn、tst、teq 的編碼裡第 20 位（S 位元）都是 1：它們一定會設定 flags。adds、subs 也是；add、adc 就不一定（`adc` 這裡沒加 s）。

**規則檢查**：

```
$ arm-none-eabi-as mulbad.s        （.arch armv4；mul r1, r1, r2）
mulbad.s:3: Rd and Rm should be different in mul

$ arm-none-eabi-as div4.s          （.arch armv4；sdiv r1, r2, r3）
div4.s:3: Error: selected processor does not support `sdiv r1,r2,r3' in ARM mode

$ arm-none-eabi-as div7.s          （.arch armv7ve）
   0:	e711f312 	sdiv	r1, r2, r3
```

- 舊的 ARM：mul 的目的暫存器和第一個來源不能一樣（課本說的限制）。
- sdiv 只有**比較新的** ARM 才有（課本說的「newer versions」）。

### 12.2 NZCV flags 與 64 位元加減法

`flags.py` 照 ARM 的定義用 32 位元算（減法 a − b 在硬體上是 a + ~b + 1）：

```
$ python3 flags.py
cmp 0, 5         ->           -5  N=1 Z=0 C=0 V=0
cmp 5, 0         ->            5  N=0 Z=0 C=1 V=0
cmp 5, 5         ->            0  N=0 Z=1 C=1 V=0
-1 + -2          ->           -3  N=1 Z=0 C=1 V=0
0x7FFFFFFF + 1   ->  -2147483648  N=1 Z=0 C=0 V=1
64-bit add: 0x00000001FFFFFFFF + 0x0000000200000001 = 0x0000000400000000  (python: 0x0000000400000000)
64-bit sub: 0x0000000500000000 - 0x0000000100000001 = 0x00000003FFFFFFFF  (python: 0x00000003FFFFFFFF)
bic 0b1101, 0b0110 = 0b1001
```

- **0 − 5**：要借位 → **C = 0**；**5 − 0**：不用借位 → **C = 1**。這就是影片說的「反直覺」。
- **5 − 5**：Z = 1，而且 C = 1（沒借位）。
- **−1 + −2**：C = 1（無號 overflow），但 V = 0（有號數沒問題），和課本的例子一樣。
- **0x7FFFFFFF + 1**：C = 0，但 **V = 1**（兩個正數相加變負數）。C 和 V 是兩件不同的事。
- 64 位元加法：低 32 位 `0xFFFFFFFF + 0x00000001` 產生進位，adc 把它加到高 32 位，結果和 Python 直接算的一樣。
- 64 位元減法：低 32 位 `0x00000000 − 0x00000001` 要借位（C = 0），sbc 從高 32 位多減 1。

Example 38 的 mvn：`mvn` 會把 32 位元全部取反，所以 NOT(0 OR 1) 在 r0 裡是 `0xFFFFFFFE`，不是 0。

### 12.3 x86 也是一樣的做法

在 x86 的 **32 位元模式**下，64 位元整數也要放在兩個暫存器裡：

```
$ gcc -m32 -O2 -S -masm=intel add64.c     （unsigned long long add64(a, b) { return a + b; }）
add64:
	mov	eax, DWORD PTR 12[esp]
	mov	edx, DWORD PTR 16[esp]
	add	eax, DWORD PTR 4[esp]      ← 低 32 位（會設定 CF）
	adc	edx, DWORD PTR 8[esp]      ← 高 32 位 ＋ CF
	ret
```

- 和 ARM 的 `adds`／`adc` 完全同樣的想法。（x86 的 add 一定會設定 flags，所以不需要 s 後綴。）

---

## 13. 一頁速查表

| 主題 | 重點 |
|---|---|
| ARM | 32 位元 RISC；每條指令 32 位元；只有 load／store 碰記憶體 |
| 暫存器 | r0–r15；r11 fp、r12 ip、r13 sp、r14 lr、r15 pc（PC 看得到） |
| 立即值 | 前面加 `#`；十六進位 `#0x…` |
| 註解 | `@` |
| mov／mvn | mvn = 取反再搬 |
| 算術 | add、sub、**rsb**（r1 = r3 − r2，用來算「常數 − 暫存器」） |
| 邏輯 | and、**eor**（XOR）、**orr**（OR）、**bic**（r2 AND ~r3） |
| 名稱 | 指令名稱都是三個字母 |
| 乘法 | mul；**mla**（×＋，4-address）；**smull／umull**（64 位元結果放兩個暫存器，低位在前） |
| 乘法限制 | 只能用暫存器；Rd ≠ 第一個來源（舊 ARM） |
| 除法 | sdiv、udiv（新版 ARM 才有），只算商 |
| Shifter operand | `reg, lsl/lsr/asr/ror #n 或 reg`；嵌在運算元裡，沒有獨立的位移指令 |
| ror／rrx | 掉出 LSB 的放回 MSB；ror #0 = rrx（經過 carry） |
| 例子 | `mov r1, r2, asr #2`（÷4）；`add r1, r2, r3, lsl #2`（r2 + 4·r3） |
| 比較 | cmp（−）、cmn（+）、tst（AND）、teq（XOR） |
| CPSR | N、Z、C、V |
| Carry | 加法：有進位 = 1；**減法：借位 = 0，不借位 = 1** |
| s 後綴 | adds、subs… 讓任何資料處理指令設定 flags |
| 用到 C | adc（+ C）、sbc（− NOT C）、rsc |
| 64 位元 | adds ＋ adc；subs ＋ sbc |

---

## 14. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| CPSR 的 N、Z、C、V 怎麼被分支用 | **第 10 支**：16 種條件碼 |
| lr、pc 看得到 → 用 mov pc, lr 回傳 | **第 10 支**：bl、bx |
| Shifter operand 用在 ldr／str 的定址 | **第 10 支**：base-scaled-index |
| 12 位元立即值、shifter operand 的編碼 | **第 10 支**：ARM 編碼 |
| x86 的 eflags（OF、CF、ZF、SF）、adc／sbb | **第 11–12 支**：Ch5 x86 |
| 乘法、乘加的硬體 | **第 21–26 支**：課本 Ch8 計算機算術 |
| ARM 的向量擴充（NEON） | **第 41 支**：Ch12 向量處理 |
| 真實的 ARM 處理器（Cortex-M3、A8、A15） | 課本 Appendix A |

---

## 附：本講一句話總結

> **ARM 是 32 位元的 RISC 指令集（每條指令 32 位元，只有 load／store 碰記憶體），和 SimpleRisc 一樣有 16 個暫存器，但 r13 = sp、r14 = lr、r15 = pc 都是程式設計師看得到的（r11 fp、r12 ip 只是 GNU compiler 的約定），立即值要加 #。資料處理指令除了 mov、add、sub、and、orr 之外，還有 mvn（取反）、rsb（反向減法，算「常數 − 暫存器」）、eor（XOR）、bic（清除位元）、mla（乘加，線性代數常用）、smull／umull（32 × 32 → 64 位元，結果放兩個暫存器）；ARM 沒有獨立的位移指令，而是把 lsl／lsr／asr／ror 嵌進第二個運算元（mov r1, r2, asr #2；add r1, r2, r3, lsl #2）。cmp、cmn、tst、teq 和任何加 s 後綴的指令會設定 CPSR 的 N、Z、C、V，其中減法借位時 C = 0、不借位時 C = 1（實測 0 − 5 的 C = 0）；adc、sbc 用 carry 做多倍精度運算，adds ＋ adc 就能把兩個 64 位元數相加（x86 的 32 位元模式也是 add ＋ adc）。課本 Example 38 求 NOT(A ∨ B) 用 mvn 會把 32 位元全部反轉，r0 得到 0xFFFFFFFE。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 4: ARM assembly, Part 1/2 錄影逐字稿（YouTube 自動字幕，55:59；字幕疑似機器翻譯，品質差）、Sarangi *Basic Computer Architecture* v3.09 Ch4 §4.1–4.2.5（p.139–148）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、GNU as `arm-none-eabi-as`、gcc 11.4、Python 3.10）結果。*
