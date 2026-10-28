# Basic Computer Architecture 第六支影片完整筆記：組合語言基礎（Chapter 3, Part 1/3）

> **課程**：Basic Computer Architecture — Chapt 3: Assembly Language, Part 1/3（播放清單第 6 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（46:02，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 Ch3 §3.1–3.2（p.91–106）
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 什麼是組合語言](#1-什麼是組合語言)
- [2. 為什麼要學組合語言](#2-為什麼要學組合語言)
- [3. Assembler 與 gcc -S](#3-assembler-與-gcc--s)
- [4. 機器模型：暫存器](#4-機器模型暫存器)
- [5. 記憶體：一個很大的 byte 陣列](#5-記憶體一個很大的-byte-陣列)
- [6. Little endian 與 big endian](#6-little-endian-與-big-endian)
- [7. 陣列：row major 與 column major](#7-陣列row-major-與-column-major)
- [8. 組合語言檔案的結構](#8-組合語言檔案的結構)
- [9. 組合語言敘述的結構](#9-組合語言敘述的結構)
- [10. 指令的種類](#10-指令的種類)
- [11. n-address format](#11-n-address-format)
- [12. Register transfer notation](#12-register-transfer-notation)
- [13. 定址模式](#13-定址模式)
- [14. 【實測】](#14-實測)
- [15. 一頁速查表](#15-一頁速查表)
- [16. 與後續講次的連結](#16-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

Chapter 3 的大綱（[01:02]）：

1. 組合語言概觀 ← **本講**
2. 組合語言語法 ← **本講**
3. 自己設計一套組合語言 **SimpleRisc** ← 第 7 支
4. 函式與 stack ← 第 7–8 支
5. 指令編碼 ← 第 8 支

這一講的重點：組合語言看到的**機器長什麼樣**（暫存器、記憶體），以及**指令怎麼寫、運算元怎麼指定**（定址模式）。

---

## 1. 什麼是組合語言

[01:16]–[05:37]　課本 §3.1.1（p.91–95）

| | 高階語言（C、Java） | 低階語言（組合語言） |
|---|---|---|
| 一行敘述對應 | **很多條**機器指令 | 通常**一條**機器指令 |
| 和 ISA 的關係 | 跟處理器無關，到處都能跑 | **只適用某個處理器**（或某一族處理器） |
| 抽象程度 | 很高 | 很接近硬體 |

- 【口頭】例：同一個 C 程式可以在 Intel 和 ARM 上跑，但 Intel 有自己的組合語言語法，ARM 也有自己的。
- 組合語言**不是一種語言**，是一**族**語言：每個 ISA 都有自己的組合語言（Intel 組合語言、ARM 組合語言……）。
- 一行組合語言敘述有兩部分：
  1. **指令碼**（instruction code，助憶碼 mnemonic）：例如 add、sub、mul
  2. **運算元列表**（list of operands）

【課本補充】課本 §3.1.1 還講了幾件影片沒提的事：

- **雞生蛋問題**：第一個 compiler 怎麼寫的？最早的程式設計師直接用機器指令寫出簡陋的 compiler，再用它編譯用高階語言寫的、更好的 compiler，一代一代下來。
- **Cross-compiler**（Definition 25）：在機器 A 上跑、產生機器 B 的機器碼。新處理器還沒做好時就靠它。§14 實測用的 `aarch64-linux-gnu-gcc` 就是一個 cross-compiler（在 x86 上產生 ARM 的程式）。
- 什麼時候要自己寫組合語言：
  1. **效率**：compiler 的分析有限（不能編譯太久），也不知道程式的整體模式。聰明的程式設計師有時能贏過 compiler。
  2. **額外功能**：處理器新增的指令，舊 compiler 用不到；OS 和 device driver 需要 gcc 不會產生的特殊指令。
- 實務上，比較常見的是**在 C 裡嵌入一小段組合語言**（inline assembly），而不是整個程式用組合語言寫。
- 例子：3D 遊戲的核心引擎；模擬原子核結構的超級電腦程式，把核心的 1000 行函式改用組合語言寫，跑出破紀錄的速度，後來還用它算出質子和中子的質量。

---

## 2. 為什麼要學組合語言

[05:37]–[07:41]，[10:40]–[11:08]　課本 §3.1.2（p.95）

- 【口頭】老師打開一個程式語言排行榜（2016 年 7 月）：前三名是 Java、C、C++，**組合語言排第 10**（把所有處理器的組合語言算在一起）。
- 所以學生至少要熟悉**一種**組合語言，才能：
  - 更好地寫硬體程式
  - 理解 compiler 和 assembler 怎麼運作
- 【口頭】組合語言流行的一個原因：現在到處都是**小型控制器**。例如有人在房間就開燈、沒人就關燈的智慧燈。這些小硬體為了效率，常用組合語言寫。
- **從硬體設計者的角度**：組合語言的每一行對應一條指令，所以學會組合語言 = 學會 ISA 的細節。硬體設計者知道組合語言的形式，就知道**要做什麼**。

---

## 3. Assembler 與 gcc -S

[07:41]–[10:40]　課本 Definition 27（p.94）

- **Assembler（組譯器）**：把組合語言程式轉成機器碼（一串 0 和 1，也就是 binary）的程式。
- x86 常見的 assembler：**NASM**、**TASM**、**MASM**。ARM 也有對應的 assembler。
  - ⚠️ 逐字稿聽成「nasum uh tum and Mum」，應該是 NASM、TASM、MASM。
- 在 Linux 上：

```
gcc -S filename.c    → filename.s（組合語言檔，可以用文字編輯器打開看）
gcc filename.s       → a.out（binary）
./a.out              → 執行
```

```
filename.c ──gcc -S──▶ filename.s ──gcc──▶ a.out ──▶ 處理器執行
  （C）                 （組合語言）           （0 和 1）
```

- 【口頭】Mac OS X 也一樣；Windows 上用 Code::Blocks 的話指令不同。

---

## 4. 機器模型：暫存器

[11:08]–[13:00]　課本 §3.2.1（p.96–97）

組合語言假設的機器 = **Chapter 1 的 Von Neumann 機器 ＋ 暫存器**：

```
         ┌────────── CPU ──────────┐
記憶體 ◀─▶│ 暫存器 │ ALU │ 控制單元 │◀─▶ I/O 裝置
（指令＋資料）└─────────────────────────┘
```

- 【課本補充】組合語言**不區分**指令記憶體和資料記憶體（不是 Harvard）。

**暫存器**：有名字的儲存位置。

| ISA | 一般用途暫存器 |
|---|---|
| ARM | r0、r1、…、r15 |
| x86（32 位元） | eax、ebx、ecx、edx、esi、edi（課本另外列了 ebp、esp，共 8 個） |
| x86-64 | 16 個（課本補充） |

其他種類的暫存器：

- **Machine specific register（MSR）**：可以細部存取處理器內部，例如風扇轉速、電源、溫度控制。只屬於某顆處理器，不一定整個家族都有。
- 有特殊功能的暫存器：
  - **Stack pointer**（之後講）
  - **Program counter（PC）**：指向目前正在執行的指令的位址
  - **Return address register**（之後講）

【課本補充】

- **Return address register**：呼叫函式前，先把「回來的位址」存在這裡；return 就把它複製回 PC。ARM 和 MIPS 的程式設計師看得到這個暫存器；**x86 沒有**，改用 stack（§3.3.10）。
- ARM 的 PC 是程式設計師看得到的 **r15**，可以讀也可以寫（寫它就是跳躍）。x86 的 PC 是**隱含的**，看不到。

---

## 5. 記憶體：一個很大的 byte 陣列

[13:00]–[15:15]　課本 §3.2.2（p.97）

- 所有程式（包括組合語言）都把記憶體看成**一個很大的 byte 陣列**。
- 每個 byte 有一個**位址**：第一個 byte 是 0，第二個是 1，第 n 個是 n − 1。
- 程式放在記憶體的某一部分，資料放在另一部分。PC 存著目前指令的位址。
- 【口頭】每個 binary 都**假設整個記憶體都是它自己的**。同時跑好幾個程式會怎樣？之後再講（虛擬記憶體，課本 Ch11；也見 [`os/linux-os` 第 5 支](../../os/linux-os/lecture05_note.md)）。
- 【課本補充】最小的存取單位是 **byte**，沒辦法直接定址某一個 bit。32 位元機器的位址是 32 位元無號數，64 位元機器是 64 位元。

C 和 Java 的資料型別大小（[14:43]）：

| 型別 | 大小 |
|---|---|
| char | 1 byte |
| short | 2 byte |
| int | 4 byte |
| long | 8 byte |

⚠️ 「long 是 8 byte」在 Java 永遠成立；在 C 要看平台。x86-64 Linux 上是 8 byte，但 32 位元系統和 64 位元 Windows 上 `long` 是 4 byte。

---

## 6. Little endian 與 big endian

[15:15]–[17:51]　課本 p.97–98

一個 4 byte 的整數存在**連續的 4 個位置** i、i+1、i+2、i+3。但 4 個 byte 的**順序**有兩種：

例：`0x87654321` 拆成 4 個 byte：87、65、43、21。

| 位址 | 0 | 1 | 2 | 3 |
|---|---|---|---|---|
| **Big endian**（最高位的 byte 放最低位址） | 87 | 65 | 43 | 21 |
| **Little endian**（最低位的 byte 放最低位址） | 21 | 43 | 65 | 87 |

- 【口頭】**沒有誰比較好**，只是慣例。但硬體、組合語言程式設計師、assembler 和 compiler 的作者都要知道用的是哪一種。
- 影片：**ARM 和 x86** 用 little endian；**Sun SPARC 和 IBM PowerPC** 用 big endian。
- 【課本補充】ARM 早期是 little endian，**現在是 bi-endian**（兩種都可以，看使用者的設定）。
- §14.2 實測：本機（x86）把 `0x87654321` 存成 `21 43 65 87`。

---

## 7. 陣列：row major 與 column major

[17:51]–[21:34]　課本 p.98–99

**一維陣列**：

- `int a[100]`：一個 int 4 byte，整個陣列 **400 byte**，存在連續的位置。
- 【課本補充】a[i] 存在 (loc + 4i) 到 (loc + 4i + 3)。每個整數照處理器的 endian 存。

**二維陣列**（矩陣）有兩種存法：

```
矩陣：         row major：              column major：
a b c          a b c d e f g h i        a d g b e h c f i
d e f          （先存第一列，再第二列）    （先存第一行，再第二行）
g h i
```

| 方式 | 誰用 |
|---|---|
| **Row major** | C、Python |
| **Column major** | Fortran、MATLAB |

- 【課本補充】Example 22：`a[100][100]` 用 row major 存進一維的 `b[10000]`，(i, j) 對到 b[100 × i + j]。
- 【口頭】兩種方式各有取捨，「要看你怎麼寫程式」。影片說 Chapter 10 再討論，⚠️ 在課本 v3.09 是 **Ch11**（記憶體系統）。§14.3 實測先看一下差多少：在 C 裡照 column 順序走，比照 row 順序慢 **20 倍**。

**本節重點**：處理器和所有程式都假設記憶體是一個很大的 byte 陣列。4 byte 整數占 4 個連續位置（big 或 little endian）；陣列可以用 row major 或 column major 存。

---

## 8. 組合語言檔案的結構

[21:34]–[23:47]　課本 §3.2.3（p.99–100）

- 不同 ISA 的組合語言格式不同，有時差很多。這裡講 **GNU assembler**（Linux、Mac OS 常用）的格式。其他系統（例如 Windows）的結構大致類似。

```
組合語言檔（.s）
├── .file   原始 C 檔的名字等 meta 資訊
├── .text   所有指令（程式本身）
└── .data   程式用到的資料
```

- 每個 section 用一個以 `.` 開頭的識別字標出來。
- ⚠️ 逐字稿 [23:30] 說「.text section 包含唯讀變數和常數的值」，這句聽起來像口誤，前面 [22:54] 他才說 .text 是指令、.data 是資料。課本也說 .text 是程式本身。實測（§14.1）：gcc 把**唯讀常數**放在 `.rodata`，**有初值的變數**放在 `.data`，**初值是 0 的變數**放在 `.bss`。
- 【課本補充】課本列了三種 section：**text**（程式）、**data**（有初值的資料）、**bss**（初值是 0 的共用資料）。text 是必要的，其他可有可無；也可以用 `.section` 定義新的 section。

---

## 9. 組合語言敘述的結構

[23:47]–[28:32]　課本 p.100–102

**基本敘述**：

```
指令   運算元1, 運算元2, …, 運算元n
```

運算元可以是：

- **立即值（immediate）**：常數，例如 3、4、5，或字元、字串常數（Definition 29）
- **暫存器**
- **記憶體位置**

例：

```
sub r3, r1, r2     ; r3 = r1 − r2
mul r3, r1, r2     ; r3 = r1 × r2
```

- 【口頭】像高階語言，只是語法不同。**第一個運算元通常是目的地**（destination），後面兩個是來源（source）。
- ⚠️ 這是 ARM 和 Intel 語法的慣例。GNU 的 x86 組合語言預設用 **AT&T 語法**，順序剛好**相反**（目的地在最後）。§14.1 實測比較兩種語法。

**一般敘述的結構**（課本 Figure 3.6）：

```
[label:]  [指令 或 指示詞(directive) 或 常數]  [註解]
```

- 三個欄位都**可有可無**，但至少要有一個。
- **註解**：兩種寫法。
  - `@` 後面到行尾（ARM）
  - C 風格的 `/* … */`
  - 註解可以放在敘述後面，也可以自己一整行。
- **Label**：一個敘述的**唯一識別字**，像 C 裡 goto 用的 label。寫成 `label:`，之後「goto label」就會跳回這裡。
  - 【課本補充】同一個檔案裡 label 不能重複；可以用英數字和 `.`、`_`、`$`，通常不能以數字開頭。只有 label 沒有內容的行，label 指向下一個有內容的敘述。
- **Directive（指示詞）**：給 **assembler** 的命令，要它做特別的事，例如宣告函式、開始一個新的 section。以 `.` 開頭。

```
label1: add r1, r2, r3   @ Add the values in r2 and r3
        add r5, r6, r7   /* Add the values in r6 and r7 */
```

---

## 10. 指令的種類

[28:32]–[30:49]　課本 §3.2.4（p.103）

| 種類 | 做什麼 |
|---|---|
| **Data processing（資料處理）** | 算術（加減乘除）、比較、邏輯運算（AND、OR、NOT） |
| **Data transfer（資料搬移）** | 暫存器 ↔ 記憶體、暫存器 ↔ 暫存器 |
| **Branch（分支）** | 跳到某個 label；實作 for、while、if |
| **特殊指令** | 和周邊裝置、其他程式互動；設定機器參數；存取高階語言碰不到的部分 |

- ⚠️ 課本的第 4 類叫 **exception generating instructions**：把控制權從使用者程式交給**作業系統**（system call 就是這種）。影片說的「特殊指令」範圍比較廣。課本說本書只講前三類。

---

## 11. n-address format

[30:49]–[31:33]　課本 Definition 31（p.103–104）

> 指令需要 n 個運算元（包括來源和目的地），就叫做 **n-address format**。

- 例：`add r1, r2, r3` 有 3 個運算元 → **3-address format**。
- 也有 0、1、2、4-address format。

【課本補充】ARM 的資料處理指令大多是 3-address，資料搬移是 2-address；**x86 大多是 2-address**：

```
add r3, r1, r2      ; ARM：r3 = r1 + r2
add edx, eax        ; x86：edx = edx + eax（目的地 = 第一個來源）
```

取捨：運算元越多，指令要越多位元、越占空間；但也越有彈性，compiler 和組合語言程式設計師越好用。

---

## 12. Register transfer notation

[31:33]–[34:57]　課本 p.104–105

用來精確描述指令語意的寫法：

| 寫法 | 意思 |
|---|---|
| `r1 ← r2` | 把 r2 的內容搬到 r1（箭頭是資料流動的方向） |
| `r1 ← r2 + 4` | r2 加立即值 4，存到 r1 |
| `r1 ← r2 + r3` | r2 加 r3，存到 r1 |
| `r1 ← [r2]` | 用 r2 的內容當**位址**去記憶體讀，存到 r1 |
| `r1 ← [r2 + 4]` | 位址 = r2 + 4 |

- `[ ]` 的意思：裡面是一個**位址**，要拿它去存取記憶體。
- 例：r2 = 8，`r1 ← [r2]` 會讀位址 **8、9、10、11** 這 4 個 byte（一個 int），存到 r1。
- 從記憶體讀資料到暫存器叫做 **load**（memory load）。

---

## 13. 定址模式

[34:57]–[45:48]　課本 p.105–106

> **Addressing mode（定址模式）**：在組合語言敘述裡，指定和存取一個運算元的方法（Definition 32）。

令 V 是運算元的值：

| 定址模式 | V = | 例子 | 用途 |
|---|---|---|---|
| **Immediate** | imm | `add r1, r2, 4` 的 4；48、0x13、−3 | 初始化、和常數運算 |
| **Register (direct)** | r1 | `add r1, r2, r3` | 最常用 |
| **Register indirect** | [r1] | r1 = 32 → 讀位址 32 起的 4 byte | 指標 |
| **Base-offset** | [r1 + offset] | `20[r1]`：r1 = 8 → 讀位址 28～31 | **陣列**、struct |
| **Base-index** | [r1 + r2] | | |
| **Base-index-offset** | [r1 + r2 + offset] | `100[r1, r2]` | x86 很好用 |
| **Memory direct** | [addr] | 位址直接寫在指令裡 | 大多數 ISA 沒有，x86 有 |
| **Memory indirect** | [[r1]] | （課本補充） | |
| **PC-relative** | [PC + offset] | | **分支**：if、for、while |

- **Effective address（有效位址）**：算出來、拿去存取記憶體的那個位址。例如 base-offset 的 r1 + offset（Definition 33）。
  - 【課本補充】offset 也叫 **displacement（位移）**。
- **Base-offset** 的圖解（[39:47]）：用 r1 讀 register file → 加上 offset → 得到位址 → 存取記憶體 → 讀出 4 byte。
- 【口頭】base-index 和 base-index-offset「不是很常用，但常見到值得講；等我們看 x86 的時候會發現它們非常有用」。
- **PC-relative**（[43:47]）：把 PC 當成一個暫存器，加上 offset 得到位址。
  - 那個位址通常指向**一條指令**（在 32 位元系統裡，一條指令 4 byte，本書預設 32 位元）。
  - 用途：改變 PC，跳到程式的不同位置，就是 if、for、while 在做的事。之後細講。
- ⚠️ 影片 [36:20] 舉的例子「`add r2, r2, r3, r1`（r1 是記憶體位址）」逐字稿不太完整；意思是「用 r1 的內容當位址讀記憶體，加上 r3，存到 r2」。真實的 ISA 寫法各不相同。
- 【課本補充】**Memory indirect** [[r1]]：r1 存的是 M 的位址，M 裡又存著真正資料的位址。要存取記憶體兩次。影片沒講。

---

## 14. 【實測】

本機：Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4（x86-64）、aarch64-linux-gnu-gcc。程式在 `experiments/lecture06/`。

### 14.1 gcc -S：section、label、directive、定址模式

```c
int a[100];
int b = 0x87654321;
const char msg[] = "hi";
int get(int i, int j) { return a[i + j + 5]; }
```

```
$ gcc -O2 -S -fno-asynchronous-unwind-tables -fcf-protection=none test.c -o -
	.file	"test.c"
	.text
	.globl	get
	.type	get, @function
get:
	leal	5(%rdi,%rsi), %eax          ← base-index-offset：rdi + rsi + 5
	leaq	a(%rip), %rdx               ← PC-relative：a 的位址 = rip + offset
	cltq                                ← 把 eax sign-extend 成 rax
	movl	(%rdx,%rax,4), %eax         ← base-index（index × 4）：讀 a[...]
	ret
	.section	.rodata
msg:	.string	"hi"
	.data
b:	.long	-2023406815
	.bss
a:	.zero	400
```

（輸出已刪掉部分 directive）

- 看得到課本的結構：`.file` → `.text`（程式）→ 資料 section。
- **資料放在三種 section**：
  - 唯讀常數 `msg` → `.rodata`
  - 有初值的 `b` → `.data`（0x87654321 當成有號數印成 −2023406815）
  - 初值是 0 的 `a` → `.bss`，`.zero 400`：**100 個 int = 400 byte**，和影片說的一樣
- `get:` 是 **label**；`.globl`、`.type`、`.long` 這些以 `.` 開頭的是 **directive**。
- 定址模式：
  - `5(%rdi,%rsi)`：AT&T 語法的 **base-index-offset**，= rdi + rsi + 5
  - `a(%rip)`：**PC-relative**。x86-64 的 PC 叫 rip。資料的位址用「離目前指令多遠」表示。
  - `(%rdx,%rax,4)`：base-index，而且 index 會乘上 **scale** 4（一個 int 4 byte）。這是 x86 特有的，課本 Ch5 會講。

**兩種 x86 語法**：

```
$ gcc -O2 -S -masm=intel ...
get:
	lea	eax, 5[rdi+rsi]       ← Intel 語法：目的地在前面
	lea	rdx, a[rip]
```

| 語法 | 寫法 | 目的地 |
|---|---|---|
| AT&T（GNU 預設） | `leal 5(%rdi,%rsi), %eax` | **最後** |
| Intel | `lea eax, 5[rdi+rsi]` | **最前**（和影片、ARM 一樣） |

**ARM 版本**（cross-compiler）：

```
$ aarch64-linux-gnu-gcc -O2 -S test.c -o -
get:
	add	w0, w0, w1                ← 3-address
	adrp	x1, .LANCHOR0
	add	w0, w0, 5
	add	x1, x1, :lo12:.LANCHOR0
	ldr	w0, [x1, w0, sxtw 2]      ← base-index：x1 + (w0 sign-extend 後 × 4)
	ret
```

- ARM 是 **3-address**（`add w0, w0, w1`），x86 的 `lea` 用定址模式一次做完 rdi + rsi + 5。

### 14.2 Endianness

```
$ ./endian
addr+0..3: 21 43 65 87
bswap: 21436587
$ lscpu | grep "Byte Order"
Byte Order:                         Little Endian
```

- `0x87654321` 在記憶體裡從低位址往高位址是 `21 43 65 87`：x86 是 **little endian**，和課本 Figure 3.3 一樣。
- `__builtin_bswap32` 把 byte 順序反過來（x86 的 `bswap` 指令）。網路協定用 big endian（network byte order），所以 `htonl()` 這類函式在 x86 上就是做 bswap。

### 14.3 Row major：照順序走快 20 倍

```
$ gcc -O1 -o rowcol rowcol.c && ./rowcol
&m[0][1] - &m[0][0] = 4 bytes, &m[1][0] - &m[0][0] = 16384 bytes
row order:    5.4 ms
column order: 112.8 ms
```

- C 是 row major：`m[0][1]` 緊接在 `m[0][0]` 後面（差 4 byte），`m[1][0]` 隔了一整列（4096 × 4 = 16384 byte）。
- 對 4096 × 4096 的 int 陣列：**照 row 順序走 5.4 ms，照 column 順序走 113 ms**，慢了約 20 倍。
- 原因是 cache：記憶體一次搬一整塊相鄰的資料進來。照 row 走，搬進來的都用得到；照 column 走，每次都跳 16 KB，搬進來的幾乎都浪費。這就是影片說「trade-off 留到記憶體那章」的內容（課本 Ch11，第 34–37 支）。

---

## 15. 一頁速查表

| 主題 | 重點 |
|---|---|
| 組合語言 | 低階語言；一行 ≈ 一條機器指令；每個 ISA 各有一種 |
| 敘述 | 指令碼（助憶碼）＋ 運算元列表 |
| Assembler | 組合語言 → 機器碼；x86：NASM、TASM、MASM |
| gcc | `gcc -S x.c` → x.s；`gcc x.s` → a.out |
| 機器模型 | Von Neumann ＋ 暫存器；不分指令／資料記憶體 |
| 暫存器 | ARM r0–r15；x86 eax…edi；MSR；SP、PC、return address |
| 記憶體 | 一個大 byte 陣列；位址從 0 開始；byte 是最小單位 |
| 型別 | char 1、short 2、int 4、long 8（C 看平台） |
| Endian | big：MSB 放低位址（SPARC、POWER）；little：LSB 放低位址（x86；ARM 是 bi-endian） |
| 陣列 | a[i] 在 loc + 4i；row major（C、Python）／column major（Fortran、MATLAB） |
| 檔案結構 | .file、.text（指令）、.data（有初值）、.bss（初值 0）、.rodata（唯讀） |
| 敘述結構 | label: 指令／directive ＠註解 或 /* */ |
| 指令種類 | data processing、data transfer、branch、特殊（exception generating） |
| n-address | ARM 多為 3-address；x86 多為 2-address |
| RTN | `r1 ← [r2 + 4]`：[ ] 表示存取記憶體 |
| 定址模式 | immediate、register、register indirect、base-offset、base-index、base-index-offset、memory direct、memory indirect、PC-relative |
| Effective address | 算出來拿去存取記憶體的位址 |
| AT&T vs Intel | AT&T 目的地在最後；Intel 在最前 |

---

## 16. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| 自己設計的 ISA | **第 7 支**：SimpleRisc 的指令 |
| Return address、stack pointer | **第 7–8 支**：函式與 stack |
| 指令怎麼變成 0 和 1 | **第 8 支**：SimpleRisc 的編碼 |
| ARM 的 r0–r15、3-address | **第 9–10 支**：Ch4 ARM |
| x86 的暫存器、2-address、base-index-offset、memory direct | **第 11–14 支**：Ch5 x86 |
| PC-relative | **第 16 支**：RISC-V 的 branch 與記憶體指令 |
| Row major vs column major 的取捨、cache | **第 34–37 支**：課本 Ch11 記憶體系統 |
| 每個程式以為記憶體都是自己的 | 課本 Ch11 虛擬記憶體；[`os/linux-os` 第 5 支](../../os/linux-os/lecture05_note.md) |

---

## 附：本講一句話總結

> **組合語言是一族低階語言，每個 ISA 各有一種，一行通常對應一條機器指令（指令碼 ＋ 運算元），用 assembler（NASM、TASM、MASM）或 gcc -S／gcc 在 C、組合語言、binary 之間轉換；它看到的機器是 Von Neumann 加暫存器（ARM 的 r0–r15、x86 的 eax 等，還有 MSR、stack pointer、PC、return address），記憶體是從 0 開始編號的大 byte 陣列，多 byte 的資料用 little endian（x86，實測 0x87654321 存成 21 43 65 87）或 big endian（SPARC、POWER）存，二維陣列用 row major（C）或 column major（Fortran）存（實測照 column 走慢 20 倍）。GNU 組合語言檔分成 .file、.text、.data（實測還有 .rodata、.bss），敘述由 label、指令或 directive、註解組成；指令分為資料處理、資料搬移、分支和特殊指令，依運算元個數叫 n-address format（ARM 多 3 個、x86 多 2 個）；運算元用 register transfer notation 描述，指定方式叫定址模式：immediate、register、register indirect、base-offset（陣列）、base-index（-offset）、memory direct、PC-relative（分支）等。注意 GNU 的 x86 預設是 AT&T 語法，目的地在最後，和影片說的「目的地在最前」相反。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 3: Assembly Language, Part 1/3 錄影逐字稿（YouTube 自動字幕，46:02）、Sarangi *Basic Computer Architecture* v3.09 Ch3 §3.1–3.2（p.91–106）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、aarch64-linux-gnu-gcc）結果。*
