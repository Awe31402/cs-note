# Basic Computer Architecture 第八支影片完整筆記：函式、stack 與指令編碼（Chapter 3, Part 3/3）

> **課程**：Basic Computer Architecture — Chapt 3: Assembly Language, Part 3/3（播放清單第 8 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（1:16:36，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 Ch3 §3.3.10–3.3.14、§3.4（p.115–134）
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
- [1. 函式在記憶體裡長什麼樣](#1-函式在記憶體裡長什麼樣)
- [2. Return address](#2-return-address)
- [3. 第一個函式：call 與 ret](#3-第一個函式call-與-ret)
- [4. 兩個問題：space problem 與 overwrite problem](#4-兩個問題space-problem-與-overwrite-problem)
- [5. Register spilling：caller saved 與 callee saved](#5-register-spillingcaller-saved-與-callee-saved)
- [6. Activation block](#6-activation-block)
- [7. 函式呼叫是後進先出 → stack](#7-函式呼叫是後進先出--stack)
- [8. Stack pointer](#8-stack-pointer)
- [9. call 與 ret 的精確語意](#9-call-與-ret-的精確語意)
- [10. 例子：遞迴的階乘](#10-例子遞迴的階乘)
- [11. nop](#11-nop)
- [12. 指令編碼：opcode](#12-指令編碼opcode)
- [13. 0-address 與 1-address：branch format](#13-0-address-與-1-addressbranch-format)
- [14. 3-address：register format 與 immediate format](#14-3-addressregister-format-與-immediate-format)
- [15. 2-address：cmp、not、mov](#15-2-addresscmpnotmov)
- [16. ld 與 st：st 的例外](#16-ld-與-stst-的例外)
- [17. 格式總整理](#17-格式總整理)
- [18. 【課本補充】設計 ISA 的步驟](#18-課本補充設計-isa-的步驟)
- [19. 【實測】](#19-實測)
- [20. 一頁速查表](#20-一頁速查表)
- [21. 與後續講次的連結](#21-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

到目前為止的指令還寫不出複雜的程式。這一講補上兩件大事：

```
函式 ── 需要記住回來的地方（return address）
     ── 參數太多放不下（space problem）
     ── 被呼叫的函式會蓋掉暫存器（overwrite problem）
     → activation block → 函式呼叫是 LIFO → stack（sp = r14）
     → call / ret（ra = r15）

編碼 ── 21 條指令 → 5 bit opcode
     → 三種格式：branch、register、immediate
     → st 是例外（來源暫存器放在 rd 欄位）
```

---

## 1. 函式在記憶體裡長什麼樣

[01:01]–[02:53]　課本 §3.3.10（p.114–115）

- **函式**：完成某個特定工作的一段程式碼。給它參數，它回傳結果。例：判斷質數的函式，參數是那個數，回傳一個 Boolean。
- 在記憶體裡：函式的**所有指令**放在一段**連續的記憶體**裡；資料放在別的地方。
- 一個函式由它**第一條指令的位址**識別。
- 呼叫函式 = 把 PC 設成那個位址 = 跳到函式的開頭。

---

## 2. Return address

[02:53]–[05:45]　課本 Definition 34（p.115）

- 函式做完之後，要回到**呼叫它的那條指令的下一條**繼續執行。這個位址叫做 **return address**。
- 所以要記兩件事：
  1. 函式的起始位址（例：A）
  2. return address
- **Caller**（呼叫者）：呼叫別的函式的那個函式。**Callee**（被呼叫者）：被呼叫的函式。
  - 【課本補充】caller／callee 的關係不是固定的：foo 可以呼叫 foobar，foobar 也可以呼叫 foo。
- 例：call 指令在位址 P。SimpleRisc 每條指令 4 byte，所以 return address = **P + 4**。
  - call：PC ← A，記住 P + 4
  - ret：PC ← P + 4

---

## 3. 第一個函式：call 與 ret

[05:45]–[09:47]　課本 Example 32、33（p.121–122）

```c
int add(int x, int y) { return x + y; }
```

```
.foo:
	add r2, r0, r1
	ret
.main:
	mov r0, 3
	mov r1, 5
	call .foo
	add r3, r2, 10     @ r3 = 8 + 10 = 18
```

- 慣例：程式從 `.main` 開始（像 C）。label 後面加冒號。
- **函式的寫法**：一個 label，接著所有指令，最後一條 `ret`。
- `call .foo`：跳到 .foo。`ret`：回到 call 的下一條。
- 這裡的參數**用暫存器傳**：caller 放進 r0、r1，callee 讀出來；結果放在 r2。
- 【口頭】**學生常搞混的一點**：整個程式**共用同一組暫存器**（register file）。不是每個函式有自己的暫存器。一個函式寫了某個暫存器，另一個函式讀得到。

§19.1 實測：r3 = 18，7 步。

---

## 4. 兩個問題：space problem 與 overwrite problem

[09:47]–[11:26]，[15:08]–[16:46]　課本 p.116–117

| 問題 | 內容 |
|---|---|
| **Space problem** | 暫存器有限（16 個，一般用途只能用 14 個），參數太多放不下。【口頭】寫 100 個參數的函式很少見，但 8、9、10 個很常見；暫存器還要留給暫存值用。 |
| **Overwrite problem** | callee 會**蓋掉** caller 之後還要用的暫存器。例如遞迴呼叫自己。 |

- Space problem 的初步解法：caller 把參數放在某塊**記憶體**，callee 從那裡讀。
- 【課本補充】課本還列了第三點：函式可能要**回傳很大的資料**（例如 C 的大 struct），暫存器放不下。
- 【課本補充】為什麼不能用一塊**固定**的記憶體傳參數？因為 callee 可能遞迴呼叫自己，會蓋掉。

---

## 5. Register spilling：caller saved 與 callee saved

[11:26]–[14:50]　課本 Definition 36、Figure 3.9（p.117–118）

> **Register spilling**：把暫存器的值存到記憶體，之後再讀回來。

| 方式 | 誰存、誰讀回來 |
|---|---|
| **Caller saved** | **caller** 在呼叫前用 st 把**之後還要用**的暫存器存起來，callee 回來後用 ld 讀回來 |
| **Callee saved** | **callee** 一開始把**它會蓋掉**的暫存器存起來，ret 之前讀回來；caller 什麼都不用做 |

- 例：foo 呼叫 foobar。foo 之後還要用 r1，但 foobar 會把 r1 設成別的值。不 spill 的話，foo 回來讀到的就是被蓋掉的值。
- Callee saved 時，caller 可以假設所有暫存器都保持原值，**除非雙方事先約定**某個暫存器用來放回傳值。
- 【口頭】兩種方式各有優缺點；什麼時候用哪種，課本章末習題有討論，學生應該想一想。§19.3 實測：x86-64 Linux 兩種都用（有些暫存器 caller saved，有些 callee saved）。

**但還有問題**（[17:01]）：

- caller 和 callee 必須**嚴格約定**用哪些記憶體位置。如果 caller 存在某塊記憶體，callee 也存在同一塊，就互相干擾。
- 函式結束後，它用來 spill 的空間要**回收**，給程式其他部分用。
- 這讓事情變得很複雜。

---

## 6. Activation block

[18:20]–[28:28]　課本 Figure 3.10（p.119）

> **Activation block**：一塊**連續的記憶體**，存放一個正在執行的函式的資訊（執行環境，execution context）。

```
┌────────────────────┐
│ Arguments（參數）    │
│ Return address     │
│ Register spill area│
│ Local variables    │
└────────────────────┘
```

- 例：

  ```c
  int f(int arg1) { int a = 3, b = 4, c = a + b + arg1; return c; }
  ```

  arg1 放在參數區；a、b、c 放在區域變數區；需要的話也存 return address 和 spill 的暫存器。
- 函式回傳之後，activation block 就**不需要了**。

**呼叫函式的流程**（caller saved）：

| 誰 | 步驟 |
|---|---|
| Caller | 1. spill 暫存器（存到 **caller 自己的** activation block） |
| | 2. 配置 callee 的 activation block |
| | 3. 參數寫進 callee 的 activation block（只有一兩個參數就用暫存器） |
| | 4. call |
| Callee | 5. 讀參數，需要的話搬到暫存器 |
| | 6. 如果 callee 還會呼叫別的函式，存 return address |
| | 7. 配置區域變數的空間，執行 |
| | 8. 結束：恢復 return address 暫存器；回傳值寫進暫存器（或很大的話寫進 **caller** 的 activation block） |
| | 9. 銷毀自己的 activation block，ret |
| Caller | 10. 從暫存器或自己的 activation block 拿回傳值；恢復 spill 的暫存器；繼續 |

- 【口頭】「我故意在某些地方講得比較模糊，細節在後面幾張投影片。」
- **call 指令**做兩件事：① 跳到 callee 的開頭；② 把下一條指令的位址存起來當 return address。
- **問題**：函式呼叫非常頻繁，一秒鐘可能上百萬次，所以 activation block 的**建立和刪除一定要非常快**。

---

## 7. 函式呼叫是後進先出 → stack

[28:44]–[33:30]　課本 Figure 3.11、Definition 37（p.119–120）

觀察呼叫的順序：

```
main → test → test2          （test2 先結束，再 test）
main → foo → foobar → foobarbar  （foobarbar 先結束，再 foobar，再 foo）
```

- 最後被呼叫的函式最先結束：**後進先出（LIFO, last in first out）**。
- 電腦科學裡一聽到 LIFO 就想到 **stack**（堆疊），就像一疊書：最後放上去的最先拿下來。
- 所以 activation block 可以存成一個 stack。

**Stack 往下長**（downward growing）：新的 activation block 放在**比較低的位址**。

例：stack 從 1000 開始，foo、foobar、foobarbar 的 activation block 各 100 byte：

```
1000 ┌──────────┐
     │   foo    │  901–1000
 900 ├──────────┤
     │  foobar  │  801–900
 800 ├──────────┤
     │foobarbar │  701–800
 700 └──────────┘ ← top of stack
```

foobarbar 結束就只剩 200 byte；foobar 也結束就只剩 900～1000。

> **Definition 37**：**stack** 是存放程式所有 activation block 的記憶體區域。傳統上往下長；呼叫函式前把它的 activation block **push** 上去，函式結束時 **pop** 掉。

- 【課本補充】往下長的意思：stack 的 **top** 是 stack 裡**最小的位址**，代表正在執行的函式；**bottom** 是最大的位址，代表 main。

---

## 8. Stack pointer

[33:30]–[37:15]　課本 Definition 38（p.120）

- 專門用一個暫存器指向 stack 的頂端：SimpleRisc 是 **r14**，也叫 **sp**（stack pointer）。
- sp 指向**最後一個 activation block 的開頭**（stack 結束的地方）。
- **配置** activation block：`sub sp, sp, 大小`。例：減 100，新的 100 byte 就是新的 activation block。
- **釋放**：`add sp, sp, 大小`。
- 兩個都**只要一條指令**，而且大小是事先知道的常數。這就是我們要的：很快。

**Stack 解決了什麼**：

- **Space problem**：參數少就用暫存器；多的放在 activation block。
- **Overwrite problem**：每個函式有自己的記憶體區域放暫存值，不會互相蓋掉。
- **管理變簡單**：建立和刪除只要改 sp，用普通的 add、sub。

**Stack 是軟體的結構**（[36:48]）：

- 大部分指令集只約定「某個暫存器當 sp」，其他都由**軟體**管理：寫組合語言時要**自己**加指令去增減 sp、建立 activation block。
- 【課本補充】例外是 **x86**：硬體知道 stack 的存在，會用它來 push return address 或其他暫存器的值。但即使這樣，硬體也不知道每個 activation block 裡面放什麼，結構還是由程式設計師或 compiler 決定。§19.3 實測。

---

## 9. call 與 ret 的精確語意

[37:15]–[40:55]　課本 §3.3.11、Table 3.9（p.121）

- **ra = r15**：return address register。ARM 和 x86 有類似的概念。

| 指令 | 例子 | 語意 |
|---|---|---|
| `call label` | `call .foo` | ra ← PC + 4；PC ← address(.foo) |
| `ret` | `ret` | PC ← ra |

- `ret` 本質上就是一個跳躍：把 ra 的內容讀進 PC。
- 【口頭】label 前面的 `.` 只是慣例，SimpleRisc 直譯器不要求一定要有。
- **慣例**：r14、r15 保留給特殊用途；其他暫存器給程式用。通常用 **r0、r1 傳參數**，其他放暫存值。
- 【課本補充】call 和 ret 也可以看成**無條件分支**：它們改變 PC，但不看任何條件。ret 是 0-address 指令。

---

## 10. 例子：遞迴的階乘

[40:55]–[49:07]　課本 Example 34（p.122）

```c
int factorial(int num) {
	if (num <= 1) return 1;
	return num * factorial(num - 1);
}
void main() { int result = factorial(10); }
```

```
.factorial:
	cmp r0, 1          /* compare (1,num) */
	beq .return
	bgt .continue
	b .return
.continue:
	sub sp, sp, 8      /* create space on the stack */
	st r0, [sp]        /* push r0 on the stack */
	st ra, 4[sp]       /* push the return address register */
	sub r0, r0, 1      /* num = num - 1 */
	call .factorial    /* result will be in r1 */
	ld r0, [sp]        /* pop r0 from the stack */
	ld ra, 4[sp]       /* restore the return address */
	mul r1, r0, r1     /* factorial(n) = n * factorial(n-1) */
	add sp, sp, 8      /* delete the activation block */
	ret
.return:
	mov r1, 1
	ret
.main:
	mov r0, 10
	call .factorial
```

- **約定**：factorial 的參數在 **r0**，結果放在 **r1**。所有 factorial（包括 base case）都把結果放在 r1。
- **Base case**：r0 == 1 跳 .return；r0 > 1 跳 .continue；剩下 r0 < 1 也跳 .return。.return 設 r1 = 1（1! = 0! = 1）。
- **遞迴部分**：
  1. **建立 activation block**：要存 2 個值（2 個 int = 8 byte），所以 sp 減 8。
  2. **存 r0 和 ra**：因為要呼叫另一份 factorial，它會蓋掉 r0（參數）和 ra（call 會改 ra）。
     ```
     sp + 4 → ra
     sp + 0 → r0
     ```
  3. r0 − 1，呼叫 factorial。回來後，r1 = factorial(num − 1)。
  4. **讀回 r0 和 ra**（sp 沒變，所以位置一樣）。
  5. r1 = r0 × r1，就是 n × (n − 1)!。
  6. **刪掉 activation block**：sp 加 8，回到原來的位置。
  7. ret。
- 這是 **caller saved**：caller 存它自己需要的暫存器，之後再讀回來。
- 【口頭】可以用**數學歸納法**證明這個程式永遠正確：base case 把結果放在 r1；遞迴 case 也把結果放在 r1。
- §19.1 實測：10! = 3628800，125 步，stack 最深到 9 個 activation block（72 byte）。**如果不存 ra**，程式會無限迴圈。

---

## 11. nop

[49:07]–[49:44]　課本 §3.3.12（p.123）

- **nop**：什麼都不做的指令。
- 為什麼要一條什麼都不做的指令？影片說在講 **pipeline 處理器**時解釋，影片說是「Chapter 9」，⚠️ 課本 v3.09 是 **Ch10**（第 30–33 支）。
- 【課本補充】課本：「為了確保執行的正確性，需要一條不做任何事的指令。」

**到此為止**（[49:44]）：組合語言概觀、語法、SimpleRisc、函式與 stack。【口頭】物件導向等其他高階功能，都是這些基本指令的變化，可以用基本指令集實作。大部分指令集只指定兩種特殊暫存器：stack pointer 和 return address；ARM 和 x86 有其他特殊暫存器，之後再講。

---

## 12. 指令編碼：opcode

[50:30]–[52:56]　課本 §3.3.14、Table 3.10（p.125–126）

- 電腦只懂 0 和 1，所以每條組合語言指令都要編成 **32 位元**。
- 21 條指令 → 需要 **5 位元**的 **opcode**（4 位元只能表示 16 條）。
- opcode 放在**最高的 5 位**，其他 27 位編碼其餘部分。

| 指令 | opcode | 指令 | opcode | 指令 | opcode |
|---|---|---|---|---|---|
| add | 00000 | not | 01000 | beq | 10000 |
| sub | 00001 | mov | 01001 | bgt | 10001 |
| mul | 00010 | lsl | 01010 | b | 10010 |
| div | 00011 | lsr | 01011 | call | 10011 |
| mod | 00100 | asr | 01100 | ret | 10100 |
| cmp | 00101 | nop | 01101 | | |
| and | 00110 | ld | 01110 | | |
| or | 00111 | st | 01111 | | |

- 順序：先算術，然後……load／store，最後分支。
- 為什麼這樣排？在講**解碼**（decoding）的那章會清楚。影片說「Chapter 8」，⚠️ 課本 v3.09 是 **Ch9**。
- 【口頭】至少現在可以看出：**分支指令的 opcode 最高位都是 1**，看一個位元就知道是不是分支。

> **Definition 39**：**opcode** 是每條機器指令的唯一識別碼。

**指令摘要**（[52:56]）：rd = 目的暫存器，rs1 = 第一個來源暫存器，rs2 = 第二個來源暫存器，imm = 立即值。

- 算術、邏輯、位移：`op rd, rs1, (rs2/imm)`
- cmp：`cmp rs1, (rs2/imm)`（沒有目的暫存器）
- nop、ret：0-address
- ld：`ld rd, imm[rs1]`
- st：`st rd, imm[rs1]`（rd 其實是**來源**，見 §16）
- b、beq、bgt、call：在組合語言裡寫 label，但**編碼時是相對於 PC 的 offset**

---

## 13. 0-address 與 1-address：branch format

[55:54]–[61:11]　課本 Figure 3.12–3.13（p.126–127）

**0-address：nop、ret**

```
 32        28 27                                    1
┌────────────┬──────────────────────────────────────┐
│ opcode (5) │           沒用到（27）                │
└────────────┴──────────────────────────────────────┘
```

**1-address：b、beq、bgt、call** → **branch format**

```
┌────────────┬──────────────────────────────────────┐
│   op (5)   │           offset (27)                │
└────────────┴──────────────────────────────────────┘
```

- offset 是**有號數**（2's complement），加到 PC 上：PC-relative 定址。可以往前也可以往後跳。
- **Word offset 的技巧**（[57:15]）：
  - 每條指令 4 byte，假設所有指令都從 **4 的倍數**的位址開始（1000、1004、1008……）。
  - 4 的倍數寫成二進位，**最後兩位一定是 00**。
  - 既然知道是 00，就不要浪費兩個位元去存。存 **word offset**（隔了幾個 word，1 word = 4 byte），不存 byte offset。
  - 實際位址 = **PC + offset × 4**。
- 【課本補充】為什麼 27 位元的 offset 夠用？分支大多來自 for、while、if，跳的距離通常只有幾百條指令。27 位元有號數的範圍是 ±2²⁶ 個 word，換成 byte 是 ±2²⁸，**29 位元的 byte offset**。萬一真的要跳更遠，assembler 可以把好幾個分支串起來。
- 【課本補充】0-address 格式可以看成 1-address 格式的特例（offset 欄位沒用到），所以都叫 **branch format**。課本把位元編號成 **1～32**（不是常見的 0～31）：op 是第 28–32 位，offset 是第 1–27 位。

---

## 14. 3-address：register format 與 immediate format

[61:11]–[65:56]　課本 Figure 3.14–3.15（p.127–128）

add、sub、mul、div、mod、and、or、lsl、lsr、asr 都是 3-address。

- 第一個要決定的：第二個來源是暫存器還是立即值？用一個 **I bit**：I = 0 是暫存器，I = 1 是立即值。
- 16 個暫存器 → 每個暫存器要 **4 位元**。

**Register format**（I = 0）：

```
 5      1    4      4      4         14
┌────┬───┬──────┬──────┬──────┬───────────────┐
│ op │ 0 │  rd  │ rs1  │ rs2  │    沒用到      │
└────┴───┴──────┴──────┴──────┴───────────────┘
```

5 + 1 + 4 + 4 + 4 = 18 位元，小於 32，沒問題。

**Immediate format**（I = 1）：

```
 5      1    4      4      2        16
┌────┬───┬──────┬──────┬─────┬──────────────────┐
│ op │ 1 │  rd  │ rs1  │ mod │  imm（16 位元）   │
└────┴───┴──────┴──────┴─────┴──────────────────┘
                        └──── 18 位元 imm ────┘
```

- 5 + 1 + 4 + 4 = 14，剩下 **18 位元**：2 位元 **modifier** ＋ 16 位元**常數**。
- Modifier 編碼（課本）：**00 = 預設，01 = u，10 = h**。
  - ⚠️ 影片 [64:30] 講 modifier 的位元時有點口誤（把 01 說成「sign modifier」又改口）。以課本為準。
- 預設時 16 位元是 2's complement 有號數；u 和 h 時當成無號數。處理器依 modifier 把它擴展成 32 位元。
- 【口頭】這個格式很「緊」：**32 個位元全部用上**。

【課本補充】Example 35：`sub r1, r2, 3`

```
00001 1 0001 0010 00 0000 0000 0000 0011  =  0x0C480003
 sub  I  r1   r2  mod        3
```

§19.2 實測自己寫的 assembler 編出來一模一樣。

---

## 15. 2-address：cmp、not、mov

[65:56]–[67:56]　課本 Figure 3.16（p.129）

- 簡單的解法（「在計算機結構的很多地方都會用到」）：**沿用 3-address 的格式，只是某些欄位不用**。

| 指令 | rd | rs1 | rs2／imm |
|---|---|---|---|
| cmp | **空** | rs1 | 第二個運算元 |
| mov | rd | **空** | 來源 |
| not | rd | **空** | 來源 |

- 這樣**解碼硬體**（了解指令的硬體）會比較簡單，因為只有少數幾種預先定義好的格式：branch、immediate、register。
- 【課本補充】為 2-address 另外設計格式「得到的好處微乎其微」，固定的格式讓解碼邏輯更直接。

---

## 16. ld 與 st：st 的例外

[67:56]–[72:41]　課本 Figure 3.17、Example 36（p.129–130）

**ld rd, imm[rs1]**：直接用 **immediate format**。I = 1；rd 是目的暫存器；base 暫存器放在 rs1；offset 放在 imm。

**st 的問題**：

- st 有**兩個來源暫存器**（要存的值、base address）和**一個立即值**（offset），而且**沒有暫存器目的地**（目的地是記憶體）。
- Immediate format 只能有一個來源暫存器 ＋ 一個立即值，放不下。
- 要為 st 定義一個新格式嗎？**不要**。

**解法**：還是用 immediate format，但**把要存的那個來源暫存器放在 rd 欄位**。

```
st rd, imm[rs1]
     ↑ 其實是來源
```

- 這會讓處理器多一點複雜度，之後處理。
- 格式還是很緊，32 位元全用上。
- 【口頭】這是典型的**工程取捨**：為了**簡單和優雅**、讓解碼硬體保持簡單，寧可在 st 這裡打破規則，也不要多一種格式。
- 【課本補充】課本的說法：要在「優雅」和「效率」之間取捨。為了一條指令新增一種格式太過頭了。

【課本補充】Example 36：`st r8, 20[r2]`

```
01111 1 1000 0010 00 0000 0000 0001 0100  =  0x7E080014
  st  I  r8   r2  mod          20
```

§19.2 實測也一樣。

---

## 17. 格式總整理

[72:41]–[74:36]　課本 Table 3.11（p.131）

| 格式 | 欄位（課本位元編號 1–32） |
|---|---|
| **branch** | op (28–32) ＋ offset (1–27) |
| **register** | op (28–32) ＋ I (27) ＋ rd (23–26) ＋ rs1 (19–22) ＋ rs2 (15–18) |
| **immediate** | op (28–32) ＋ I (27) ＋ rd (23–26) ＋ rs1 (19–22) ＋ imm (1–18) |

**解碼器怎麼判斷格式**（課本）：

1. 看 opcode：是不是 branch format？
2. 不是的話看 **I bit**：1 是 immediate format，0 是 register format。

- ⚠️ 課本這段有兩個小錯：
  - 它說「六個 branch format 指令是 call、ret、beq、bgt、b 和 **nop**」，接著說「五個分支指令的 opcode 最高位都是 1」。但 nop 的 opcode 是 `01101`，最高位是 0，所以 nop 要另外比對（課本自己也說「要知道是不是 nop，解碼器要和 01101 比較」）。
  - 它說「只要看 opcode 的**最高三位**，應該是 1」。應該是看**最高一位**：beq 是 `10000`，最高三位是 100，不是 111。

【口頭】總結：SimpleRisc 真的很簡單，這就是名字的由來。真實的指令集複雜得多：更多定址模式、更多種指令、更多運算元的種類、很多特殊條件和 flags。但重要的是理解**從零設計一套組合語言**會遇到什麼問題，例如 st 那樣的取捨。接下來 Chapter 4、5 是兩個真實的指令集：ARM 和 x86。

---

## 18. 【課本補充】設計 ISA 的步驟

課本 Lessons Learned（p.132），影片沒講：

1. **研究工作負載**（workload）：ISA 要跑什麼程式。SimpleRisc 是跑一般程式，所以要簡單、精簡、通用、完備。
2. **決定指令數量**：除非有很強的理由，不要超過 **64～128 條**，不然解碼器會很複雜。
3. **決定指令的種類**：數值運算多就多放算術指令；文字處理多就多放字串指令。SimpleRisc：6 條算術、3 條位移、3 條邏輯、3 條資料搬移、5 條分支、1 條 nop。
4. **決定每條指令**：讓常見的情況變快（common case fast）；決定運算元的數量和種類（2-address 可以少用暫存器；要一次加 10 個數，甚至可以有 11-address 的指令）。
5. **決定定址模式**：如果算術指令允許 register-indirect，就要多加存取記憶體的硬體；只允許暫存器的話執行很快，但需要更多暫存器和 load／store。
6. **設計編碼**：讓解碼器的工作越少越好；用少數幾種通用的格式。

第 3 點的分類加起來 6 + 3 + 3 + 3 + 5 + 1 = 21 條：6 條算術包括 cmp，3 條邏輯包括 not，3 條資料搬移是 mov、ld、st，5 條分支是 b、beq、bgt、call、ret。

---

## 19. 【實測】

本機：Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10。程式在 `experiments/simplerisc/`（直譯器、assembler；本講的程式在 `l08/`）和 `experiments/lecture08/`。

### 19.1 call、ret 與遞迴的階乘

上一講的直譯器多記錄了 sp 的最小值：

```
$ python3 simplerisc.py l08/foo.s
foo.s: 7 steps; r0=3 r1=5 r2=8 r3=18 | E=0 GT=0 | sp=0x10000 min_sp=0x10000
$ python3 simplerisc.py l08/rfact.s
rfact.s: 125 steps; r0=10 r1=3628800 | E=1 GT=0 | sp=0x10000 min_sp=0xFFB8
```

- §3 的例子：r3 = 18。
- 遞迴階乘：r1 = 10! = **3628800**。
- sp 一開始是 0x10000，最低到 0xFFB8：0x10000 − 0xFFB8 = 72 = **9 × 8 byte**。factorial(10) 到 factorial(2) 各建一個 8 byte 的 activation block，factorial(1) 走 base case 不建。
- 結束時 sp 回到 0x10000：每一層都把自己的 activation block 刪乾淨了。
- r0 結束時又是 10：caller saved 的 r0 一層一層讀回來了。

**如果不存 ra**（把 `st ra, 4[sp]` 和 `ld ra, 4[sp]` 拿掉）：

```
$ python3 simplerisc.py l08/rfact_nora.s
rfact_nora.s: 1000000 steps;  | E=1 GT=0 | sp=0x1F83A8 min_sp=0xFFB8
```

- 跑到直譯器的上限 100 萬步還沒停。
- 原因：最深一層的 call 把 ra 設成「call .factorial 的下一條」。之後每一層 ret 都回到同一個地方，永遠回不到 main。這就是 overwrite problem：call 自己會蓋掉 ra。

### 19.2 自己寫一個 SimpleRisc assembler

`experiments/simplerisc/srasm.py` 依照課本的三種格式把組合語言編成 32 位元：

```
$ python3 srasm.py l08/enc.s
   0: 0C480003  00001 1 0001 0010 000000000000000011   sub r1, r2, 3
   4: 7E080014  01111 1 1000 0010 000000000000010100   st r8, 20[r2]
   8: 0048C000  00000 0 0001 0010 001100000000000000   add r1, r2, r3
  12: 4C02FB12  01001 1 0000 0000 101111101100010010   movh r0, 0xFB12
  16: 0401CDEF  00000 1 0000 0000 011100110111101111   addu r0, r0, 0xCDEF
  20: 2C080001  00101 1 0000 0010 000000000000000001   cmp r2, 1
  24: 68000000  01101 0 0000 0000 000000000000000000   nop
  28: A0000000  10100 0 0000 0000 000000000000000000   ret
```

（欄位：op、I、rd、rs1、最後 18 位元）

- `sub r1, r2, 3` = **0x0C480003**，`st r8, 20[r2]` = **0x7E080014**：和課本 Example 35、36 一樣。
- `st r8`：r8 = `1000` 放在 **rd** 欄位，雖然它是來源。
- `movh`：modifier 是 `10`；`addu`：modifier 是 `01`。
- `cmp r2, 1`：rd 欄位是空的（0000），r2 在 rs1。
- `add r1, r2, r3`（register format）：I = 0，r3 = `0011` 在 rs2，後面 14 位元沒用到。

分支的 offset：

```
$ python3 srasm.py l08/rfact.s | head -5
   0: 90000011  10010 0 0000 0000 000000000000010001   b .main
   4: 2C000001  00101 1 0000 0000 000000000000000001   cmp r0, 1
   8: 8000000D  10000 0 0000 0000 000000000000001101   beq .return
  12: 88000002  10001 0 0000 0000 000000000000000010   bgt .continue
  16: 9000000B  10010 0 0000 0000 000000000000001011   b .return
```

- `bgt .continue` 在位址 12，.continue 在位址 20：byte 差 8，**word offset = 2**。
- `b .main` 在 0，.main 在 68：word offset = 17 = 0x11。
- 存的是 word offset，所以 27 位元可以跳 ±2²⁸ byte。

### 19.3 x86-64 Linux 的 stack 與呼叫慣例

```c
__attribute__((noinline)) int factorial(int num)
{
	int local = num;
	printf("num=%2d  &local=%p  ret=%p\n", num, (void *)&local, __builtin_return_address(0));
	...
}
```

```
$ gcc -O1 -fno-omit-frame-pointer -o stack stack.c && ./stack
num= 5  &local=0x7ffce73ec144  ret=0x55fb801161e6
num= 4  &local=0x7ffce73ec114  ret=0x55fb801161b7
num= 3  &local=0x7ffce73ec0e4  ret=0x55fb801161b7
num= 2  &local=0x7ffce73ec0b4  ret=0x55fb801161b7
num= 1  &local=0x7ffce73ec084  ret=0x55fb801161b7
result = 120
```

- **Stack 往下長**：每深一層，區域變數的位址就**減少 0x30**（48 byte，這就是每個 activation block 的大小）。
- **Return address**：第一層回到 main 裡（…1e6）；之後每一層都回到 factorial 裡**同一個位置**（…1b7，就是 `call factorial` 的下一條），和 SimpleRisc 的 ra = PC + 4 一樣的概念。
- Stack 的位址在 0x7ffc…（很高），程式碼在 0x55fb…：stack 從高位址開始往下長。

看 gcc 產生的組合語言：

```
$ gcc -O1 -fno-optimize-sibling-calls -S fact.c -o -
factorial:
	movl	$1, %eax
	cmpl	$1, %edi          ← 參數在 edi（不是記憶體）
	jle	.L5
	pushq	%rbx              ← callee saved：先把 rbx 存到 stack
	movl	%edi, %ebx        ← 用 rbx 保存 num
	leal	-1(%rdi), %edi    ← num - 1
	call	factorial         ← x86 的 call：硬體把 return address push 到 stack
	imull	%ebx, %eax        ← 結果在 eax
	popq	%rbx              ← 恢復 rbx
	ret                       ← 從 stack pop return address 到 PC
.L5:
	ret
```

- x86 **沒有 ra 暫存器**：`call` 由硬體把 return address **push 到 stack**，`ret` 從 stack pop 回來。這就是課本說「x86 的硬體知道 stack」。
- 參數用**暫存器**傳（edi），結果放在 **eax**。
- 這裡用的是 **callee saved**：factorial 要用 rbx，所以一開始 `push rbx`、結束前 `pop rbx`。x86-64 Linux 的慣例（System V ABI）規定 rbx、rbp、r12–r15 是 callee saved，其他（例如 rdi、rax）是 caller saved。

**Space problem 的解法**：8 個參數的函式

```
add3:
	addl	%esi, %edi        ← 第 1～6 個參數：edi、esi、edx、ecx、r8d、r9d
	addl	%edx, %edi
	addl	%ecx, %edi
	addl	%r8d, %edi
	leal	(%rdi,%r9), %eax
	addl	8(%rsp), %eax     ← 第 7 個參數在 stack 上
	addl	16(%rsp), %eax    ← 第 8 個參數在 stack 上
	ret
```

- 前 6 個參數放暫存器，**放不下的放在 stack**（caller 的 activation block 裡）。這正是影片說的「參數少就用暫存器，多的放記憶體」。
- `8(%rsp)` 是因為 `0(%rsp)` 放的是 return address。

```
$ ulimit -s
8192
```

- Linux 預設每個 thread 的 stack 上限是 **8 MB**。遞迴太深超過這個大小就會 stack overflow（segmentation fault）。

---

## 20. 一頁速查表

| 主題 | 重點 |
|---|---|
| 函式 | 連續的指令；用第一條指令的位址識別 |
| Return address | call 指令的下一條：P + 4 |
| Caller／callee | 呼叫者／被呼叫者；關係不固定 |
| 暫存器 | 整個程式共用一組 |
| Space problem | 暫存器不夠放參數（和大的回傳值） |
| Overwrite problem | callee 蓋掉 caller 要用的暫存器 |
| Spilling | 暫存器存到記憶體再讀回；caller saved／callee saved |
| Activation block | 參數、return address、spill 區、區域變數；函式結束就不要了 |
| Stack | 函式呼叫是 LIFO；往下長；top = 最小位址 |
| sp（r14） | 配置：sub sp, sp, n；釋放：add sp, sp, n |
| Stack 是軟體結構 | 由程式設計師／compiler 管理；x86 的硬體會 push／pop return address |
| call／ret | call：ra ← PC + 4，PC ← label；ret：PC ← ra |
| 慣例 | r0、r1 傳參數；r14 = sp，r15 = ra |
| 遞迴階乘 | sp −8；存 r0、ra；遞迴；讀回；相乘；sp +8；ret |
| nop | 什麼都不做；pipeline 時才知道為什麼要（課本 Ch10） |
| opcode | 5 位元；分支的最高位是 1 |
| branch format | op(5) ＋ offset(27)；word offset：PC + offset × 4 |
| register format | op(5) I=0 rd(4) rs1(4) rs2(4) |
| immediate format | op(5) I=1 rd(4) rs1(4) mod(2) imm(16)；mod：00 預設、01 u、10 h |
| 2-address | 沿用 3-address 格式，空出不用的欄位 |
| st 的例外 | st rd, imm[rs1]：要存的暫存器放在 rd 欄位 |
| 設計原則 | 少數幾種格式，解碼器才簡單；優雅 vs 效率的取捨 |

---

## 21. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| ARM 的 lr（return address）、bl 指令、stack | **第 9–10 支**：Ch4 ARM |
| x86 的 call／ret／push／pop、rsp | **第 11–14 支**：Ch5 x86 |
| RISC-V 的 jal／jalr、ra、sp、呼叫慣例 | **第 15–16 支**：Ch6 RISC-V |
| 解碼器、用 opcode 判斷格式 | **第 27–29 支**：課本 Ch9 處理器設計 |
| nop 為什麼需要 | **第 30–33 支**：課本 Ch10 pipeline |
| Kernel stack、context switch 時存暫存器 | [`os/linux-os` 第 8 支](../../os/linux-os/lecture08_note.md)、[第 12 支](../../os/linux-os/lecture12_note.md) |

---

## 附：本講一句話總結

> **函式是一段連續的指令，呼叫它要跳到第一條指令並記住 return address（call 指令的下一條，P + 4）；只用暫存器傳參數會遇到 space problem（參數太多）和 overwrite problem（callee 蓋掉 caller 的暫存器，例如遞迴），解法是 register spilling（caller saved 或 callee saved）加上 activation block（參數、return address、spill 區、區域變數）。因為函式呼叫是後進先出，所有 activation block 存成一個往下長的 stack，用 sp（r14）一條 sub／add 就能配置或釋放；call 把 PC + 4 存進 ra（r15）再跳，ret 把 ra 寫回 PC。遞迴階乘每層把 r0 和 ra 存到 8 byte 的 activation block（實測 10! 用到 9 層、72 byte；不存 ra 就無限迴圈）。編碼上，21 條指令用 5 位元 opcode，分為 branch format（27 位元 word offset，PC + offset × 4）、register format（I = 0、rd、rs1、rs2）和 immediate format（I = 1、rd、rs1、2 位元 modifier ＋ 16 位元常數），2-address 指令沿用 3-address 格式、空出欄位，st 則把要存的暫存器放在 rd 欄位以免多一種格式（實測自寫 assembler 編出課本的 0x0C480003、0x7E080014）。x86-64 Linux 則由硬體把 return address push 到 stack，前 6 個參數用暫存器、多的放 stack，rbx 等是 callee saved。課本說「看 opcode 最高三位」判斷分支，其實只要看最高一位，nop 也不在其中。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 3: Assembly Language, Part 3/3 錄影逐字稿（YouTube 自動字幕，1:16:36）、Sarangi *Basic Computer Architecture* v3.09 Ch3 §3.3.10–3.3.14、§3.4（p.115–134）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
