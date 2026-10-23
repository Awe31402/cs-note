# Operating Systems: A Linux Kernel-Oriented Approach 第四講完整筆記：靜態與動態連結（Chapter 2, Part III）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 4（Chapter 2, Part III）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（39:54，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）附錄 A.2 末（記憶體運算元）與附錄 B（編譯、連結、載入，p.467–476）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
> **標記說明**：
> - `【口頭】`：教授在課堂上口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與教授回答
> - `【課本補充】`：課本有、但這堂課沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（`檔名:行號`），用來對照觀念
> - `⚠️ 逐字稿缺字`：自動字幕在這裡掉字或聽不清，內容依上下文或課本補上
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 這堂課要做什麼](#0-這堂課要做什麼)
- [1. x86 記憶體定址模式](#1-x86-記憶體定址模式)
- [2. 範例：用組合語言算階乘](#2-範例用組合語言算階乘)
- [3. 大型專案怎麼變成一個執行檔](#3-大型專案怎麼變成一個執行檔)
  - [3.1 每個 `.c` 先變 `.o`](#31-每個-c-先變-o)
  - [3.2 【Q&A】組合語言檔在哪裡？編譯器的各個階段](#32-qa組合語言檔在哪裡編譯器的各個階段)
  - [3.3 【Q&A】順序重要嗎？](#33-qa順序重要嗎)
- [4. `.o` 檔裡有什麼](#4-o-檔裡有什麼)
  - [4.1 Section](#41-section)
  - [4.2 Symbol table 與 relocation table](#42-symbol-table-與-relocation-table)
- [5. Header 檔](#5-header-檔)
  - [5.1 為什麼需要函式簽名](#51-為什麼需要函式簽名)
  - [5.2 壞方法 vs 好方法](#52-壞方法-vs-好方法)
  - [5.3 前置處理器與巨集](#53-前置處理器與巨集)
  - [5.4 【原始碼】kernel 的 include guard 與巨集](#54-原始碼kernel-的-include-guard-與巨集)
- [6. 靜態連結](#6-靜態連結)
  - [6.1 標準函式庫也要連進來](#61-標準函式庫也要連進來)
  - [6.2 靜態連結的問題](#62-靜態連結的問題)
  - [6.3 【課本補充】連結器的兩個 pass](#63-課本補充連結器的兩個-pass)
- [7. Demo：實際跑一次](#7-demo實際跑一次)
  - [7.1 三個檔案](#71-三個檔案)
  - [7.2 一般編譯與靜態編譯：大小差多少](#72-一般編譯與靜態編譯大小差多少)
  - [7.3 靜態函式庫 `.a`](#73-靜態函式庫-a)
  - [7.4 用 `nm` 看符號](#74-用-nm-看符號)
  - [7.5 動態函式庫 `.so`](#75-動態函式庫-so)
- [8. 動態連結的原理：stub 函式](#8-動態連結的原理stub-函式)
- [9. Make：只重新編譯改過的檔](#9-make只重新編譯改過的檔)
- [10. 【課本補充】ELF 與 Loader](#10-課本補充elf-與-loader)
- [11. 【原始碼】kernel 自己怎麼連結](#11-原始碼kernel-自己怎麼連結)
- [12. 一頁速查表](#12-一頁速查表)
- [13. 與後續講次的連結](#13-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

| # | 主題 | 對應課本 |
|---|---|---|
| 1 | x86 的記憶體定址、一個階乘範例 | 附錄 A.2 |
| 2 | **編譯 → 連結**：`.c` → `.o` → 執行檔 | 附錄 B.1–B.2 |
| 3 | **靜態連結 vs 動態連結**，含現場 demo | 附錄 B.2 |

---

## 1. x86 記憶體定址模式

`[00:32]` x86 是 CISC，定址模式**不太單純**。一個記憶體運算元最多有四個部分：

```
  位址 = base + index × scale + displacement
```

| 部分 | 意思 | 例子 |
|---|---|---|
| **base** | 基底暫存器 | 陣列 `a` 的起始位址 |
| **index** | 索引暫存器 | `a[i]` 的 `i` |
| **scale** | 每個元素幾個 byte，只能是 **1、2、4、8** | `char` = 1、`int` = 4、`double` = 8 |
| **displacement** | 固定位移（常數） | 從 base 再偏移多少 |

`[01:32]` **不是每一欄都要寫**。只寫 displacement 的話，就變成**絕對位址**。

AT&T 格式的寫法：`displacement(base, index, scale)`。課本的例子：

| 寫法 | 有哪些欄位 | 位址 |
|---|---|---|
| `-32(%eax,%ebx,0x4)` | 全部四個 | `eax + ebx×4 − 32` |
| `(%eax,%ebx)` | base + index | `eax + ebx` |
| `4(%esp)` | displacement + base | `esp + 4` |

`[02:30]` 好處：編譯器能產生很有效率的程式碼。讀索引、乘上資料型別大小、加上基底——**全部在一條指令裡完成**，不用另外寫指令算位址。

【課本補充】課本說 displacement 對**存取 stack 上的變數**特別重要，因為那些變數在離某個區域起點固定偏移的位置。

---

## 2. 範例：用組合語言算階乘

`[03:31]` 教授在投影片上的程式，計算 **10!**（依逐字稿重建）：

```asm
	movl	$1, %edx        # edx = 1  (乘積 product)
	movl	$1, %eax        # eax = 1  (計數器 i)
.L1:
	imull	%eax, %edx      # edx = edx * eax   (product = product * i)
	incl	%eax            # eax = eax + 1     (i++)
	cmpl	$11, %eax       # 比較 eax 和 11
	jne	.L1             # 不相等就跳回去
```

| 行 | 意思 |
|---|---|
| 1–2 | `edx`（乘積）和 `eax`（i）都設成 1 |
| 4 | **目的地在右邊**：`edx = edx × eax`，就是 `product = product * i` |
| 5 | `eax++`。來源和目的地是同一個 |
| 6 | 比較 `eax` 和 11（要算到 10，所以 i 到 11 就停） |
| 7 | 不相等就跳回 `.L1` |

【口頭】`[04:33]` 唯一要注意的是**目的地在最右邊**：

> 「就像你在靠左行駛的國家長大，然後去美國那種靠右行駛的國家。只要克服這點不習慣，其他就沒問題了。」

我在本機驗證過（加上 `.globl main`、最後把 `edx` 搬到 `eax` 當回傳值）。因為 10! = 3628800，取 exit code 的低 8 位元剛好是 0，看不出對錯；改成 `cmpl $6, %eax`（算 5!）後，執行結果的 exit code 是 **120** ✓。

---

## 3. 大型專案怎麼變成一個執行檔

`[05:00]` 現在換到附錄 B：編譯和連結。

### 3.1 每個 `.c` 先變 `.o`

`[05:32]` 大型專案有**幾百個** C/C++ 檔：

- 很多開發者，每人寫自己的檔案，並**匯出函式**給別人用。
- 例子：我寫 `factorial` → 我是**生產者**；你用 `factorial` → 你是**消費者**。
- 最後要做成**一個大程式**，叫 **executable（執行檔）** 或 **binary**，兩個詞通用。Linux 上預設叫 **`a.out`**（歷史原因）。

`[06:30]` 可以直接 `gcc *.c`，但**很沒效率**：

> 【口頭】「你試著編譯 Linux kernel，每次要一個小時。你只改了一個檔，不想每次都等一小時，所以要**漸進式**地做。」

做法（課本 Fig B.1）：

```
  gcc -c x.c  →  x.o  ┐
  gcc -c y.c  →  y.o  ├──  連結（ld）  ──►  a.out
  gcc -c z.c  →  z.o  ┘
```

- **`.o` 檔（object file，目的檔）**：一個 C 檔編譯後的機器碼表示。
- `gcc -c`：只做到 `.o` 為止，不連結。
- 【口頭】GCC = **GNU Compiler Collection**；GNU 又是遞迴的：GNU is Not Unix。

`[07:33]` **重點**：不是每個函式都定義在同一個 `.o` 裡。例如 `factorial` 定義在 `y.o`，`x.o` 只是**呼叫**它。

`[08:01]` **連結器（linker）** 的工作：

1. 把所有 `.o` 合成一個大執行檔。
2. 執行檔有**記憶體映像（memory map）**，分成很多 section。所有程式碼放在 **text section**，連結器把它們**串起來**。
3. 編譯 `x.o` 時，`factorial` 的位址還不知道（它在別處）。串起來之後，所有函式都在了，就可以**填空**——在用到 `factorial` 的地方填上真正的位址。
4. 得到一個**完整、自給自足**的執行檔。

### 3.2 【Q&A】組合語言檔在哪裡？編譯器的各個階段

`[09:00]`

**學生問**：組合語言檔在哪裡？

**教授答**：在**中間**。用 **`gcc -S`** 可以看到組合語言。`gcc -c` 時，編譯會經過很多個階段（pass）：

```
 .c 檔
  │ 解析（parse）：檢查語法（文法）
  ▼
 Parse tree（剖析樹）
  │
  ▼
 AST（抽象語法樹）：用樹狀結構表示整個程式
  │ 多個 pass：一步步接近機器碼，每個 pass 做不同最佳化
  ▼
 中間表示（IR）：很接近組合語言
  │
  ▼
 組合語言（gcc -S 看得到）
  │ 組譯器（assembler）
  ▼
 .o 檔（機器碼，可能再最佳化）
```

`[09:30]` 很多現代編譯器**不一定真的產生組合語言**，而是轉成很接近組語的**中間表示（intermediate representation）**；但很多也確實會轉成組語，再用**組譯器**轉成 `.o`。這些全都包在 `gcc` 裡。

【課本補充】課本把編譯器分成兩半（p.467–468）：

| 部分 | 做什麼 |
|---|---|
| **前端（frontend）** | 讀 C 檔 → **詞法分析**（拆成 token）→ 建 **parse tree**（常用 yacc、bison）→ 簡化成 **AST** |
| **後端（backend）** | 一連串 pass，每個 pass 讀 AST、產生語意相同但更好的版本 → 高階 IR → 中階、低階 IR → **程式碼產生** |

常見的 pass：

- **死碼移除（dead code removal）**：永遠不會執行的程式碼刪掉。
- **常數摺疊（constant folding）**：`int a = 5 + 3 + 9;` 直接變成 `a = 17`。
- **窺孔最佳化（peephole optimization）**：在接近機器碼的低階 IR 上做指令層級的小最佳化。

`-O1`、`-O2`、`-O3` 控制最佳化的積極程度。但最佳化太多可能**互相抵銷**，甚至變慢。

### 3.3 【Q&A】順序重要嗎？

`[10:31]`

**學生問**：編譯時指定的順序重要嗎？（⚠️ 逐字稿問題不完整）

**教授答**：「**是也不是**，之後會看到。」但一般來說：**編譯一次只編一個 C 檔**，`gcc -c x.c` 永遠只處理一個檔。它可能會遇到很多**沒定義**的函式，所以需要函式的**簽名（signature）**——這就是 header 檔重要的原因。

> 「定義不在沒關係，但**至少給我看簽名**，我才能產生程式碼。簽名就像一份**合約**。」

`[11:32]` 等到所有 `.o` 都有了、要連結成一個大執行檔時，就需要**所有東西的定義**。（教授說這句話「之後會稍微放寬」——就是動態連結。）

---

## 4. `.o` 檔裡有什麼

### 4.1 Section

`[12:00]` 用 **`objdump`** 可以看 `.o` 檔的內容，它分成很多 **section**：

| Section | 內容 |
|---|---|
| **`.text`** | 機器指令（C 程式碼編譯後的結果） |
| **`.data`** | **有初始值**的全域變數、static 變數 |
| **`.bss`** | **沒有初始值**的全域變數 |
| **`.rodata`** | **唯讀資料**：常數。例如 `printf("The value is %d")` 裡的那串字串，要存在某處，就放這裡 |
| 註解等 | |

我用 `objdump -h prog.o` 實際看到的 section：`.text`、`.data`、`.bss`、`.rodata`、`.comment`、`.note.GNU-stack`、`.note.gnu.property`、`.eh_frame`。

### 4.2 Symbol table 與 relocation table

`[13:31]` `.o` 檔裡有兩張**非常重要的表**：

| 表 | 內容 | 怎麼看 |
|---|---|---|
| **Symbol table（符號表）** | 所有符號（函式名稱、全域變數）。有些**定義在這個檔**，有些**被參考但沒定義**（標 **`U` = undefined**） | `nm x.o` 或 `objdump --syms x.o` |
| **Relocation table（重定位表）** | 符號表的**子集**：那些沒定義的 | `objdump -r x.o` |

`[14:00]` relocation table 是**給連結器的訊息**：

> 「我呼叫了 `factorial`，但這個 C 檔裡沒有 `factorial`。我有它的簽名，但沒有函式本身。連結時請看我的 relocation table：我參考了 `factorial`，請你找到它在哪，把位址填到我用它的地方。」

`[15:00]` 連結器讀其他 `.o` 的**符號表**，找到 `factorial` 在哪。串接之後 `factorial` 就有位址了，填進去。

`[20:31]` 【口頭】教授特別糾正用詞：

> 「說 undefined 不太好，應該說 **unresolved symbols（未解析符號）**。連結時，所有未解析符號——可能是全域變數或函式——會被**解析（resolved）**。這才是正確的術語。」

我實際跑出來的 relocation table（`objdump -r prog.o`）：

```text
000000000000000e R_X86_64_PLT32    factorial-0x0000000000000004
0000000000000024 R_X86_64_PLT32    printf-0x0000000000000004
```

- 第一欄：要填位址的**位置**（在 `.text` 裡的偏移）。
- `R_X86_64_PLT32`：填法的類型（PLT 跟動態連結有關，見 [§8](#8-動態連結的原理stub-函式)）。
- 最後一欄：要填**誰**的位址。`factorial` 和 `printf` 都在這裡。

---

## 5. Header 檔

### 5.1 為什麼需要函式簽名

`[15:30]` `prog.c` 用 `factorial`，`factorial.c` 定義 `factorial`。編譯器**一次看一個 C 檔**，所以 `prog.c` 只需要**簽名**：

```c
int factorial(int);      // 不需要參數名稱
```

`[16:01]` 為什麼簽名重要？**型別轉換**：

- 你傳一個 `float` 給 `factorial`。C 會**自動轉型**成 `int`。
- 但要轉型，就要**塞一段轉換程式碼**進去。編譯器得知道函式**期待什麼**，才能把參數排好、轉好型別、正確呼叫。
- **回傳值**也一樣：函式回傳 `float`，你存進 `int`，也要轉型。

【課本補充】課本區分兩個詞（p.469）：

| 詞 | 意思 | 例子 |
|---|---|---|
| **宣告（declaration）** | 只寫簽名 | `int foo(int, char, double);` |
| **定義（definition）** | 函式真正的程式碼 | `int foo(int x, char y, double z) { ... }` |

如果函式在第 19 行被呼叫、第 300 行才定義，就要在第 19 行之前**先宣告**。

### 5.2 壞方法 vs 好方法

`[17:01]` **壞方法**（大一作業常這樣做）：

```c
int factorial(int);           // 自己在檔案最上面手寫簽名
// 或
extern int factorial(int);    // extern = 定義在別的檔案
```

問題：

- 簽名**寫死**了，改不了。不知道誰用的是哪個版本的簽名。
- **到處重複**。如果給 `factorial` 多加一個參數，要去**很多地方**改。

**好方法**：一個 **`.h` 檔**，集中放相關函式和全域變數的簽名。

```c
#include "factorial.h"
```

### 5.3 前置處理器與巨集

`[18:01]` `#include` 是誰處理的？**前置處理器（preprocessor）**：

- 在編譯**之前**執行，像一個「前置編譯器」。
- 做的事很簡單：**複製貼上**。把 `.h` 的內容整個貼進 C 檔。
- 用 **`gcc -E`** 可以看到貼完的結果。

我實際跑 `gcc -E prog.c`：結果有 **747 行**（因為 `stdio.h` 又引入很多其他 header），最後幾行是：

```c
extern int factorial(int);
# 3 "prog.c" 2
int main() {
 printf("%d\n", factorial(3));
}
```

`factorial.h` 的簽名就這樣「貼」進來了。

`[19:01]` 前置處理器還能做：

| 指令 | 作用 |
|---|---|
| `#define` | 定義前置處理器變數 |
| **巨集（macro）** | 像前置處理器的「函式」。kernel 用它做**相當複雜**的事 |

【口頭】`[19:01]`

> 「你可以看 Linux 開發者的訪談，他們說：『我們把 C 改得太多，**它看起來都不像 C 了**。』因為 kernel 程式碼裡有超多巨集和捷徑——**整個 linked list 的走訪是一個巨集**。他們讓自己的生活變得很輕鬆，而且很優雅。」

### 5.4 【原始碼】kernel 的 include guard 與巨集

**Include guard**：教授在 demo 裡講的「防止被引入兩次」，kernel 每個 header 都這樣寫：

```c
// include/linux/jiffies.h:1-3
/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_JIFFIES_H
#define _LINUX_JIFFIES_H
```

（檔案最後有對應的 `#endif`。）跟課本的 `factorial.h` 完全同一招：沒定義過 `_LINUX_JIFFIES_H` → 定義它、放內容；第二次引入時已經定義了 → 整段跳過。

**「整個 linked list 走訪是一個巨集」**：

```c
// include/linux/list.h:667-676
/**
 * list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_head within the struct.
 */
#define list_for_each_entry(pos, head, member)				\
	for (pos = list_first_entry(head, typeof(*pos), member);	\
	     !list_entry_is_head(pos, head, member);			\
	     pos = list_next_entry(pos, member))
```

| 部分 | 意思 |
|---|---|
| `pos` | 迴圈游標，指向目前的元素 |
| `head` | 串列的頭 |
| `member` | 結構裡那個 `list_head` 欄位的名字 |
| 整個巨集 | 展開成一個完整的 `for` 迴圈：從第一個元素開始，走到繞回頭為止，每次走到下一個 |
| 行尾 `\` | 巨集要寫在**一行**，`\` 讓它可以跨行 |
| `typeof(*pos)` | GCC 擴充：取得 `*pos` 的型別。這種用法就是「看起來不像 C」的原因之一 |

用起來就像這樣（示意）：

```c
struct task_struct *p;
list_for_each_entry(p, &some_list, tasks) {
	/* 對每個 p 做事 */
}
```

看起來像語言內建的 `foreach`，其實是前置處理器幫你展開的。（kernel 的 linked list 在課本附錄 C.1，第 3 章開始會大量出現。）

---

## 6. 靜態連結

### 6.1 標準函式庫也要連進來

`[20:02]` 目前的狀態：

- `factorial.c` 的符號表說：**我匯出 `factorial`**。
- `prog.c` 引入 `factorial.h` 拿到簽名；`prog.o` 的 relocation table 說：**`factorial` 未解析**。
- 連結時，整個程式碼排好，所有未解析符號被解析。

`[21:02]` 連結有兩種：**靜態連結（static linking）** 和 **動態連結（dynamic linking）**。

`[21:02]` 那 **`printf`** 呢？

- 它幫你**印到終端機**，但不是你寫的，你只是透過 `stdio.h` 拿到它的**簽名**。
- 做執行檔時，`printf` 的**程式碼也要進來**。
- **函式庫（library）**：一堆**編譯好的函式**的集合。把一堆 `.o` 包成一個 library。
- library **自己不能執行**。
- **C 標準函式庫**要跟你的 `.o` 一起連結，做成一個**巨大的執行檔**。

【課本補充】Definition B.2.1：**標準 C 函式庫**是一堆編譯好的函式，讓使用者程式能存取讀寫檔案、送網路訊息等系統服務。

### 6.2 靜態連結的問題

`[22:31]` 兩個問題：

| 問題 | 說明 |
|---|---|
| **① 太大** | 你的程式裡**提到**的每個標準函式庫函式，程式碼全都要放進來。可能提到 100 個，但實際只呼叫 3 個 |
| **② 不能共用** | `printf` 的機器碼其實被**很多個 process 共用**，大家都在用。何必每個執行檔都複製一份？之後講虛擬記憶體會看到：程式碼放一個地方，大家**共享** |

【課本補充】課本把問題 ① 講得更精確（p.473）：

- 執行檔大，不只是因為你的程式碼。編譯器還會加很多函式來**準備記憶體區域、載入常數、建立執行環境**。
- 第一個被呼叫的函式是 **`_start`**，它呼叫 **`__libc_start_main`**，最後才呼叫 `main`。所以 **`main` 不是第一個被執行的函式**。`main` 回傳後，也還有一連串函式負責釋放資源（開著的檔案、網路連線）。
- 但**最大的來源**還是 C 函式庫的程式碼：`printf` 本身，加上 `printf` 呼叫的所有函式、那些函式再呼叫的函式……這叫 `printf` 的 **backward slice**，正式名稱是**自反遞移閉包（reflexive-transitive closure）**。
- 編譯時**不知道**哪些函式真的會被呼叫，只能保守地全部放進來。例子：原始碼提到 100 個函式庫呼叫，實際只用 25 個 → 大小多了 4 倍。

我的 `nm dyn` 結果裡就看得到：`U __libc_start_main@GLIBC_2.34`——印證 `main` 前面還有 `__libc_start_main`。

### 6.3 【課本補充】連結器的兩個 pass

教授沒細講，課本 p.472 說連結器分兩個 pass（Fig B.3）：

| Pass | 做什麼 |
|---|---|
| **Pass 1** | 掃過所有 `.o`，**串接**所有 text、全域變數、函式、常數 section；列出所有定義過的符號。這樣就能算出每個 section（text、data、bss、常數）的最終大小，以及**每個變數和函式的最終位址**。但這時被參考的外部符號位址都**先設成 0** |
| **Pass 2** | 把那些設成 0 的位址，**換成 Pass 1 算出來的真正位址** |

---

## 7. Demo：實際跑一次

`[24:02]` 教授在課堂上現場 demo。以下我在本機（Ubuntu、GCC）**照做一遍**，數字是我機器上的結果。

### 7.1 三個檔案

`[24:32]`（課本 Listing B.1–B.3）

```c
// factorial.h
#ifndef FACTORIAL_H
#define FACTORIAL_H
extern int factorial(int);
#endif
```

`[24:32]` 【口頭】「如果 `FACTORIAL_H` 沒定義過就定義它，**防止被引入兩次**。已經引入過一次，何必再引入？這是標準寫法。」`factorial.c` 引入兩次也不會怎樣。

```c
// factorial.c
#include "factorial.h"
int factorial(int val) {
	int i, prod = 1;
	for (i = 1; i <= val; i++) prod *= i;
	return prod;
}
```

```c
// prog.c
#include <stdio.h>
#include "factorial.h"
int main() {
	printf("%d\n", factorial(3));
}
```

【課本補充】`#include "..."`（雙引號）通常表示 header 在**同一個目錄**；`#include <...>`（角括號）表示在 **include path** 裡找。`-I` 可以把目錄加進 include path。

**簽名和實作分開的好處**（課本 p.471）：只要簽名不變，實作怎麼改都行，外界不受影響。各組開發者只要**談好匯出函式的簽名**，就能各自獨立工作。

### 7.2 一般編譯與靜態編譯：大小差多少

`[25:32]`（課本 Fig B.2、B.4）

```bash
gcc prog.c factorial.c          # 方法 1：直接編
./a.out                          # 6

gcc -c prog.c                    # 方法 2：分開編成 .o
gcc -c factorial.c
gcc prog.o factorial.o           # 這一步就是連結
./a.out                          # 6

gcc -static prog.o factorial.o   # 靜態連結
ldd a.out                        # not a dynamic executable
```

**大小比較**：

| 版本 | 教授課堂 | 課本 | 我的機器 |
|---|---|---|---|
| 靜態（`-static`） | **768 KB** | **892 KB** | 900,408 bytes（**880 KB**） |
| 一般（動態） | **16 KB** | 約 **12 KB** | 16,032 bytes（**16 KB**） |

`[27:00]` 【口頭】「768 KB，它**幾乎什麼都沒做**——就算個階乘、印出來。根本不需要這麼大。」

`[27:31]` 一般的 `gcc` 不是靜態執行檔：**標準函式庫的程式碼不在執行檔裡**，放在外面。用 `ldd` 可以看到它指向哪個標準函式庫。這就是**動態連結**。

### 7.3 靜態函式庫 `.a`

`[28:31]` 如果我寫了一堆函式要給你，給你一堆 `.o` 很麻煩。不如**包成一個 library**：

```bash
ar -crs factorial.a factorial.o   # ar = archive（封存）
gcc prog.o factorial.a            # 跟 library 一起連結
./a.out                           # 6
```

- `ar`：archive 指令。`c` = create（建立）；`r` = 加進 library；`s` = 建索引。
- `factorial.a` 就是一個**封存檔**，裡面裝 `.o`。
- 這是**靜態函式庫**，連結方式跟一般 `.o` 一樣（程式碼會被複製進執行檔）。

### 7.4 用 `nm` 看符號

`[29:30]` 我機器上的結果：

```text
$ nm prog.o
                 U factorial
0000000000000000 T main
                 U printf

$ nm factorial.o
0000000000000000 T factorial
```

| 標記 | 意思 |
|---|---|
| **`T`** | 定義在 **text section**（是這個檔的函式） |
| **`U`** | **undefined**（未解析，別人定義的） |

`[30:00]` 教授的解讀：

- `prog.o`：「我知道 `main`（在我的 text section），我**不知道** `factorial` 和 `printf`。」
- `factorial.o`：「我知道 `factorial`。」

`[30:30]` 連結之後看 `a.out`：

```text
$ nm a.out | grep -E 'factorial|main|printf'
0000000000001178 T factorial
                 U __libc_start_main@GLIBC_2.34
0000000000001149 T main
                 U printf@GLIBC_2.2.5
```

- `factorial` 和 `main` 都是 `T`，**有位址了**。
- **`printf` 還是 `U`**！但程式照樣能跑。這是因為**動態連結**：`printf` 要到**執行時**才去找。
- `@GLIBC_2.2.5`：要的是 glibc 的哪個版本的 `printf`。

### 7.5 動態函式庫 `.so`

`[31:30]`（課本 Fig B.6）步驟比較多：

```bash
gcc -c -fpic -o factorial.o factorial.c     # ① 產生位置無關的程式碼
gcc -shared -o libfactorial.so factorial.o  # ② 做成共享函式庫
gcc -L. prog.c -lfactorial                  # ③ 編譯並參考這個函式庫
export LD_LIBRARY_PATH=`pwd`                # ④ 告訴系統去哪裡找
./a.out                                     # 6
```

| 步驟 | 說明 |
|---|---|
| ① `-fpic` | **PIC = position independent code（位置無關程式碼）**。程式碼可以被放在**任何位址**，所有位址都相對於一個基底 |
| ② `-shared` | 做成**共享函式庫**，也就是**動態連結函式庫**。【口頭】「它是一個 DLL」 |
| ③ `-L.` | **大寫 L**：library 的**搜尋路徑**。`.` = 先找目前目錄，再找系統目錄 |
| ③ `-lfactorial` | **小寫 l**：library 的**名字**。會去找 `libfactorial.so`（自動加 `lib` 前綴和 `.so` 副檔名）。`-l` 和名字之間**沒有空格** |
| ④ `LD_LIBRARY_PATH` | **執行時**要去哪裡找 library 的環境變數 |

`[33:30]` 第 ③ 步完成後，`ldd a.out` 顯示：

```text
	libfactorial.so => not found
```

執行檔**知道**它需要 `libfactorial.so`，但**不知道在哪**。設了 `LD_LIBRARY_PATH` 之後再 `ldd`：

```text
	libfactorial.so => /.../link/libfactorial.so (0x00007f3cecbac000)
```

找到了，就能跑，答案是 6，而且**大小很小**。

`[34:31]` 教授的總結：

1. 可以建一個**動態函式庫**（library 就是編譯好的函式集合），編譯選項不同：要**位置無關**，要編成 **lib….so**。
2. 建 `a.out` 時要給兩個資訊：**搜尋路徑**（`-L`）和**名字**（`-l`）。
3. 但這樣還不能跑：執行時要告訴 runtime 除了 `/lib`、`/usr/lib` 等標準位置之外去哪裡找，所以設 `LD_LIBRARY_PATH`。

【課本補充】Linux 的 `.so` 和 Windows 的 **DLL**（dynamically linked library）概念上是一樣的。

---

## 8. 動態連結的原理：stub 函式

`[30:30]`、`[35:30]`（課本 Fig B.5）

```
                      第一次
  printf  ──►  Stub 函式  ────────►  在 library 裡找到函式
                  │                        │
                  │ 之後                    ▼
                  │                 把函式複製到（或映射到）
                  │                 這個 process 的位址空間
                  │                        │
                  │                        ▼
                  │                 把函式的位址存起來
                  ▼
           直接用存好的位址呼叫
```

1. `printf` 被連到一個 **stub 函式**（stub = 替身，一個假的 `printf`）。
2. **第一次**呼叫 `printf`：真的進到 stub。stub 照優先順序搜尋 library 路徑（你指定的、標準的），找到 `printf` 所在的 library（例如 `libc.so`）。
3. 把 `printf` 的程式碼**複製到你的位址空間**。
4. 但如果它**已經在別的 process 的記憶體裡**了，就不浪費記憶體——OS 調整**映射（mapping）**，讓你從你的虛擬位址空間也能存取它。
5. 把位址存起來。
6. **之後**呼叫 `printf`：不用再走一遍，直接跳到那個位址。「這像一條**捷徑**。」

`[37:31]` 動態連結把程式從 **768 KB 縮到 16 KB**，「**巨大的縮減**」。細節講虛擬記憶體時再看。

【課本補充】課本補了幾個細節（p.474–475）：

- 存位址的地方叫 **jump table（跳躍表）**。第一次呼叫慢，之後很快。
- stub 呼叫 **`dlopen`** 來找 library。搜尋順序：**目前目錄 → `LD_LIBRARY_PATH` → `/lib`、`/usr/lib` 等系統位置**。順序很重要，因為常常有好幾份同一個 library，要拿到對的那份。
- 每個 library 都有**符號表**，列出它匯出的符號，這樣才能找到函式在 library 裡的位址。
- 好處：一份共享 library 放在實體記憶體裡，各個 process 只要把**虛擬位址空間**映射過去就好。這是**今天的事實標準**。
- 如果不確定目標系統有沒有某個共享 library：軟體可以把 library 一起打包，或先裝好。Linux 的**套件管理器**（Ubuntu、Red Hat）會計算一個套件的 backward slice，把缺的依賴套件先裝起來。

> 補充（不是教授講的）：上面 relocation table 裡的 `R_X86_64_PLT32`，**PLT = Procedure Linkage Table**，就是實作「stub」的那張表；課本說的 jump table 在 Linux 上對應的是 **GOT（Global Offset Table）**。這兩個名詞課本沒提，先記下名稱。

---

## 9. Make：只重新編譯改過的檔

`[38:01]` 編譯的步驟很多，所以有 **make**、**make file**（⚠️ 逐字稿還提到「cake / Cake files」，推測是 **CMake**）：

- 把這些步驟寫下來。
- 實際專案有**上千個** `.o`，不用全部重編，只要**重編幾個、再重新連結**。
- make 根據**檔案修改時間**，自動找出哪些 C 檔改過 → 重建那些 `.o`（可能只有一兩三個）→ 重新連結。

> 【口頭】「make file 的偉大之處：它根據檔案修改時間，自動算出你**上次建置之後改了什麼**，只重建那些 `.o`，重新連結，就完成了。這就是現代大型 C/C++ 專案的運作方式。」

`[39:00]` 【口頭】**作業**：已經看過 `ldd`、`objdump`、`nm`。請自己去看另外三個：

| 工具 | 做什麼 |
|---|---|
| **`readelf`** | 比 `objdump` 表達力更強一點 |
| **`strip`** | **刪掉符號表** |
| **`ranlib`** | 符號很多時，幫它**建索引** |

下一講：虛擬記憶體。

---

## 10. 【課本補充】ELF 與 Loader

教授這一講沒講到，課本 B.2.3、B.3 有：

### ELF 格式

**ELF（Executable and Linkable Format）** 是最流行的執行檔與共享函式庫格式：

- 開頭是一個 **header**：描述檔案結構、指令集、機器相容性。
- 分成很多 **section**（虛擬位址空間裡存同類資訊的連續區域）。section 再組成 **segment**。
- 有兩張表：**program header table**（segment 列表）和 **section header table**（section 列表），記錄每個 section / segment 的起始位址。
- 常見 section：text、data、bss、symbol table、relocation table，還有動態連結用的 `dynamic` section。

### Loader（載入器）

**Loader** 是 OS 負責**執行程式**的元件：

1. 你在終端機執行程式 → 產生一個新 process，跑 loader 的程式碼。
2. loader 從檔案系統讀執行檔，解析 ELF，**擺進主記憶體**：
   - 為各 section 配空間、把常數載入記憶體。
   - 配置 **stack、heap、data/bss**（static 和全域變數）區域。
   - 複製指令到記憶體；如果**已經在記憶體裡**，就直接**映射**到新 process 的虛擬記憶體，不重複複製。
   - 動態連結的資訊放在 relocation table 和 dynamic section；loader **初始化 jump table**。
3. 設定執行環境：環境變數、命令列參數、exception handler。
4. **安全**：可以把 stack 和 heap 的起始位址**隨機化**，讓攻擊者猜不到執行時的位址。
5. 最後發一個 **system call**，抹掉 loader 自己的記憶體狀態，從 text section 的第一個位址開始執行。process 就「活」了。

（loader 和 `exec` system call 在第 11 講「process 建立」會再遇到。）

---

## 11. 【原始碼】kernel 自己怎麼連結

kernel 也是一個大 C 專案，但它很特別：**它不能用標準 C 函式庫**。原因很簡單——標準函式庫裡的 `printf`、`malloc` 最後都要發 **system call** 請 kernel 幫忙，而 kernel 不能請自己幫忙。

kernel 編譯時加了這個參數：

```makefile
# Makefile:1027-1028
# arch Makefile may override CC so keep this after arch Makefile is included
NOSTDINC_FLAGS += -nostdinc
```

- **`-nostdinc`**：不要去系統的標準 include 目錄（`/usr/include`）找 header。
- 所以 kernel 不能 `#include <stdio.h>`，它用自己的 `include/linux/...`。kernel 印訊息用 **`printk`**，不是 `printf`。

**kernel 的 section 是誰排的？** kernel 有自己的 **linker script**（第 3 講看過 `jiffies = jiffies_64;` 就在這個檔）：

```text
// arch/x86/kernel/vmlinux.lds.S:124-127
	/* Text and read-only data */
	.text :  AT(ADDR(.text) - LOAD_OFFSET) {
		_text = .;
		_stext = .;
```

```text
// arch/x86/kernel/vmlinux.lds.S:391-401
	/* BSS */
	. = ALIGN(PAGE_SIZE);
	.bss : AT(ADDR(.bss) - LOAD_OFFSET) {
		__bss_start = .;
		*(.bss..page_aligned)
		. = ALIGN(PAGE_SIZE);
		*(BSS_MAIN)
		BSS_DECRYPTED
		. = ALIGN(PAGE_SIZE);
		__bss_stop = .;
	}
```

| 部分 | 意思 |
|---|---|
| `.text : { ... }` | 定義輸出檔的 `.text` section，裡面放所有 `.o` 的程式碼 |
| `_text = .;` | `.` 代表「目前位址」。這行定義一個符號 `_text` = text section 的開頭 |
| `.bss : { ... }` | 輸出檔的 `.bss` section：沒初始值的全域變數 |
| `__bss_start`、`__bss_stop` | 標出 bss 的頭尾。開機時 kernel 用這兩個符號把整段 bss **清成 0**（C 規定沒初始值的全域變數是 0） |
| `ALIGN(PAGE_SIZE)` | 對齊到 page 邊界（4 KB） |

一般程式的這些工作由 GCC 預設的 linker script 和 `_start` 幫你做好；kernel 沒有人幫它，所以全部自己寫。

**kernel 也有「匯出符號」**：kernel 是靜態連結成一個 `vmlinux`，但 **kernel module**（可以執行時載入的 driver）需要用 kernel 裡的函式，就像動態連結。kernel 用一個巨集標記「這個符號可以給 module 用」：

```c
// include/linux/export.h:150-151
#define EXPORT_SYMBOL(sym)		_EXPORT_SYMBOL(sym, "")
#define EXPORT_SYMBOL_GPL(sym)		_EXPORT_SYMBOL(sym, "_gpl")
```

```c
// kernel/time/timer.c:62
EXPORT_SYMBOL(jiffies_64);
```

- `EXPORT_SYMBOL(jiffies_64)`：把第 3 講看過的 `jiffies_64` 匯出，driver module 就能讀它。這就像 kernel 版的「符號表裡標 `T`、給外人用」。
- `EXPORT_SYMBOL_GPL`：**只給 GPL 授權的 module 用**——跟第 1 講的 GPL 授權接起來了。

（module 的載入在課本第 7 章 §7.4.2「Drivers and Modules」。）

---

## 12. 一頁速查表

### 12.1 x86 定址

| 項目 | 內容 |
|---|---|
| 公式 | `base + index × scale + displacement` |
| scale | 1、2、4、8 |
| AT&T 寫法 | `disp(base, index, scale)`，例如 `-32(%eax,%ebx,4)` |

### 12.2 編譯流程

```
 .c ──(前置處理 gcc -E)──► 展開的 .c ──(編譯 gcc -S)──► .s ──(組譯)──► .o ──(連結 ld)──► a.out
```

| 指令 | 停在哪裡 |
|---|---|
| `gcc -E` | 前置處理完（看 `#include` 貼了什麼） |
| `gcc -S` | 組合語言 `.s` |
| `gcc -c` | 目的檔 `.o` |
| `gcc` | 完整執行檔 |

### 12.3 `.o` 檔

| 項目 | 內容 |
|---|---|
| `.text` | 機器碼 |
| `.data` | 有初值的全域 / static 變數 |
| `.bss` | 沒初值的全域變數 |
| `.rodata` | 唯讀常數（字串等） |
| Symbol table | 所有符號；`T` = 定義在這、`U` = 未解析 |
| Relocation table | 未解析符號的清單，給連結器填位址 |
| 正確用詞 | **unresolved**（未解析）→ 連結時 **resolved** |

### 12.4 靜態 vs 動態

| | 靜態連結 | 動態連結 |
|---|---|---|
| 指令 | `gcc -static` | `gcc`（預設） |
| library 副檔名 | `.a`（`ar -crs`） | `.so`（`-fpic` + `-shared`） |
| library 程式碼 | 複製進執行檔 | 留在外面，執行時才找 |
| 大小（本機） | 880 KB | 16 KB |
| 記憶體共享 | 不行 | 一份 library，大家映射過去 |
| `ldd` 顯示 | not a dynamic executable | 列出依賴的 `.so` |
| 原理 | 連結時填好所有位址 | stub 函式：第一次去找、之後走捷徑 |

### 12.5 動態函式庫的參數

| 參數 | 意思 |
|---|---|
| `-fpic` | 位置無關程式碼 |
| `-shared` | 做成 `.so` |
| `-L目錄` | 編譯時的 library 搜尋路徑（大寫） |
| `-l名字` | 連結 `lib名字.so`（小寫，不加空格） |
| `LD_LIBRARY_PATH` | 執行時的 library 搜尋路徑 |

### 12.6 工具

| 工具 | 用途 |
|---|---|
| `nm` | 看符號表 |
| `objdump -h / -r / --syms` | 看 section / relocation / 符號 |
| `ldd` | 看依賴哪些動態 library |
| `ar` | 做靜態 library |
| `readelf` | 比 `objdump` 更詳細（作業） |
| `strip` | 刪符號表（作業） |
| `ranlib` | 幫 library 建索引（作業） |
| `make` | 依修改時間只重編改過的檔 |

### 12.7 本講看過的原始碼位置

| 檔案:行號 | 看什麼 |
|---|---|
| `include/linux/jiffies.h:1-3` | include guard |
| `include/linux/list.h:667-676` | `list_for_each_entry`：整個串列走訪是一個巨集 |
| `Makefile:1027-1028` | `-nostdinc`：kernel 不用標準 header |
| `arch/x86/kernel/vmlinux.lds.S:124-127` | kernel 的 `.text` section |
| `arch/x86/kernel/vmlinux.lds.S:391-401` | kernel 的 `.bss` section 與 `__bss_start` / `__bss_stop` |
| `include/linux/export.h:150-151` | `EXPORT_SYMBOL`、`EXPORT_SYMBOL_GPL` |
| `kernel/time/timer.c:62` | `EXPORT_SYMBOL(jiffies_64)` |

---

## 13. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **共享 library 只放一份、各 process 映射過去** | **第 5 講**：虛擬記憶體（下一講） |
| **記憶體映像、text / data / bss / stack / heap** | **第 5 講**：process 的 memory map（課本 §2.2.1） |
| **位置無關程式碼、位址隨機化** | **第 5–6 講**：虛擬位址、segmentation |
| **`list_for_each_entry`、kernel 的 linked list** | **第 7–9 講**：process 描述子 `task_struct`、`mm_struct`；課本附錄 C.1 |
| **Loader、`exec`** | **第 11 講**：process 建立（fork 與 exec） |
| **library call 包裝 system call** | **第 14 講**：library call 與 system call |
| **`EXPORT_SYMBOL`、kernel module** | 課本 §7.4.2（Drivers and Modules）。對應講次**待補** |

---

## 附：本講一句話總結

> **大型 C 專案是每個 `.c` 各自編成 `.o`（靠 header 給的簽名先過編譯），再由連結器根據符號表和 relocation table 把未解析的符號填上位址；靜態連結把整個函式庫塞進執行檔（本機 880 KB），動態連結則只留一個 stub、執行時才去找共享函式庫（16 KB），還能讓所有 process 共用同一份程式碼。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 4（Chapter 2, Part III）錄影逐字稿（YouTube 自動字幕，39:54）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）附錄 A.2 與附錄 B，以及 Linux kernel v6.2.12 原始碼整理。Demo 數字為本機（Ubuntu、kernel 6.5.0、GCC）實測。*
