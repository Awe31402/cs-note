# Basic Computer Architecture 第十三支影片完整筆記：x86 的字串指令、rep 與 x87 浮點指令（Chapter 5, Part 3/4）

> **課程**：Basic Computer Architecture — Chapt 5: x86 assembly, Part 3/4（播放清單第 13 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕一直下載不到）（1:14:33）＋ 課本 *Basic Computer Architecture* v3.09 **Ch5** §5.3.4–5.4.5（p.203–214）
> **章號**：影片 Chapter 5 = 課本 Ch5。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、GNU as）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 把指令名稱聽成「STOST」「LODST」「moveST」，本筆記一律寫成課本的 `stosd`、`lodsd`、`movsd`。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. CISC 的 C 在哪裡](#1-cisc-的-c-在哪裡)
- [2. lea：只算位址、不碰記憶體](#2-lea只算位址不碰記憶體)
- [3. stos、lods、movs](#3-stoslodsmovs)
- [4. Direction flag：std、cld](#4-direction-flagstdcld)
- [5. 例子：movsd](#5-例子movsd)
- [6. 用 movsd 複製陣列](#6-用-movsd-複製陣列)
- [7. rep：一條指令就是一個迴圈](#7-rep一條指令就是一個迴圈)
- [8. x87 浮點的機器模型](#8-x87-浮點的機器模型)
- [9. fld、fild](#9-fldfild)
- [10. 浮點常數怎麼放進去](#10-浮點常數怎麼放進去)
- [11. fxch、fst、fstp、fist](#11-fxchfstfstpfist)
- [12. fadd 與其他算術](#12-fadd-與其他算術)
- [13. 例子：算術平均與幾何平均](#13-例子算術平均與幾何平均)
- [14. 特殊函數](#14-特殊函數)
- [15. fcomi：要用 unsigned 的條件碼](#15-fcomi要用-unsigned-的條件碼)
- [16. 例子：sin 2θ = 2 sin θ cos θ](#16-例子sin-2θ--2-sin-θ-cos-θ)
- [17. 清理 FP stack：ffree、finit](#17-清理-fp-stackffreefinit)
- [18. 後記：x87 之後](#18-後記x87-之後)
- [19. 【原始碼】Linux 的 memcpy 用 rep movsb](#19-原始碼linux-的-memcpy-用-rep-movsb)
- [20. 【實測】](#20-實測)
- [21. 一頁速查表](#21-一頁速查表)
- [22. 與後續講次的連結](#22-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

x86 的第 3 講。前一講（第 12 支）講完一般的整數、分支、函式指令，這一講是 x86 比較「CISC」的部分：

| 部分 | 指令 |
|---|---|
| 進階記憶體指令 | `lea`；字串指令 `stos`、`lods`、`movs`；`std`、`cld`；`rep` prefix |
| x87 浮點指令 | `fld`、`fild`、`fxch`、`fst(p)`、`fist(p)`、`fadd`／`fsub`／`fmul`／`fdiv`、`fabs`、`fsqrt`、`fsin`、`fcos`、`fcomi(p)`、`ffree`、`finit` |

---

## 1. CISC 的 C 在哪裡

[00:00] 複雜指令集（CISC）總要有些讓人生活輕鬆很多的指令，才對得起 CISC 的 C。

[01:20] 常見的程式模式：把一大串 byte 從記憶體 A 搬到記憶體 B。專門做這件事的指令叫**字串指令（string instructions）**。

[01:45] 字串指令特別用到兩個暫存器（第 11 支講過）：

| 暫存器 | 預設的角色 |
|---|---|
| `esi` | 來源（source index） |
| `edi` | 目的地（destination index） |

---

## 2. lea：只算位址、不碰記憶體

[02:19] **lea**（load effective address）是所有字串指令的基礎：把一個位址放進暫存器，最常是 `edi`、`esi`。

```
lea ebx, [esi + edi*2 + 10]      ; ebx ← esi + edi*2 + 10
```

[03:18] 跟 `mov` 的差別：`lea` **不存取記憶體**。它只把記憶體運算元的**位址算出來**，把位址（不是內容）放進暫存器。【口頭】老師說它其實應該叫「compute effective address」。

---

## 3. stos、lods、movs

課本 Table 5.18（p.203）：

| 指令 | 做什麼 |
|---|---|
| `stosd` | [edi] ← eax；edi ← edi + 4 × (−1)^DF |
| `lodsd` | eax ← [esi]；esi ← esi + 4 × (−1)^DF |
| `movsd` | [edi] ← [esi]；esi、edi 都 ± 4 |

- [04:26] 三個都**沒有運算元**，全部用預設的暫存器。
- [06:17] 像 **post-indexed** 定址：先存取記憶體，再改 edi／esi。
- [06:47] 為什麼是 4？因為 `d` 是 4 byte（32 位元）。
- [09:21] 後綴決定大小：

| 後綴 | 大小 |
|---|---|
| `b` | byte，1 byte |
| `w` | word，2 byte |
| `d` | double word，4 byte |
| `q` | quad word，8 byte（64 位元才有） |

- [12:15] `movsd` 就像 `lodsd` 加 `stosd` 合成一條（但不經過 eax）。

【課本補充】課本 Trivia 2（p.204）：`si` 是 source index、`di` 是 destination index。

⚠️ 在 GNU as 的 Intel 語法裡，**沒有運算元的 `movsd` 是字串指令**；有兩個運算元的 `movsd xmm0, xmm1` 是 SSE 的「搬一個 double」，名字一樣、完全不同的指令（本機實測用的是前者）。

---

## 4. Direction flag：std、cld

[05:14] flags 暫存器裡有一個 **DF**（direction flag）bit：

| DF | 字串指令做完後 |
|---|---|
| 0 | edi／esi **加** 4（往高位址走） |
| 1 | edi／esi **減** 4（往低位址走） |

[12:41] 兩條指令設定它：

| 指令 | 做什麼 |
|---|---|
| `std` | DF ← 1 |
| `cld` | DF ← 0 |

（本機用 `std` 做往回搬的實測見 [§20.1](#201-字串指令與-rep)。）

---

## 5. 例子：movsd

[13:14] 課本 Example 81（p.205）：eax 最後是多少？

```
mov dword [esp+4], 192
lea esi, [esp+4]
lea edi, [esp+8]
movsd
mov eax, [esp+8]
```

假設 esp = 1000：

1. [esp+4]（1004）放 192。
2. esi = 1004，edi = 1008。
3. `movsd`：把 1004 的 4 byte 搬到 1008。
4. 讀 1008 → **eax = 192**。

【課本補充】Example 79、80（p.204）分別示範 `stosd`（ebx = 21）和 `lodsd`（eax = 19）。

---

## 6. 用 movsd 複製陣列

[16:17] 光是 load、store 的捷徑沒什麼了不起。看真的例子：10 個 int 的陣列，來源位址在 esi、目的地在 edi，要複製過去。

課本 Example 83（p.205–206）：

```
cld                 ; DF = 0：往上走
mov ebx, 0          ; 迴圈的 index
.loop:
    movsd           ; [edi] ← [esi]，兩個都自動 + 4
    inc ebx
    cmp ebx, 10
    jne .loop
```

[17:42] `movsd` 自帶 post-increment，不用自己去加 edi、esi。

【課本補充】課本 Example 82（p.205）是不用字串指令的版本：迴圈裡 `mov edx, [esi+ebx*4]`、`mov [edi+ebx*4], edx`、`inc`、`cmp`、`jne`，5 條；Example 83 的迴圈裡是 4 條。影片說「6 條」是連迴圈外的 `cld`、`mov ebx, 0` 一起算。

---

## 7. rep：一條指令就是一個迴圈

[18:53] **rep prefix**：寫在指令前面。

```
rep movsd           ; 看 ecx 的值 val，把 movsd 做 val 次；每做一次 ecx − 1，最後 ecx = 0
```

[19:28] 「非常、非常強大」：**一條指令裡面包了一個迴圈**。

[20:18] 課本 Example 84（p.206）：

```
cld                 ; DF = 0
mov ecx, 10         ; 次數
rep movsd           ; 做 10 次 movsd
```

[20:42] 6 條變 3 條。把一大塊資料從 A 搬到 B 不是什麼大事，三條指令就好。

[22:06] 加上前一講的分支指令，整數指令到這裡大致講完了。

【課本補充】課本 p.207：rep 還有兩個變體 **repe**（ZF 變成 0 也停）和 **repne**（ZF 變成 1 也停）。

（本機實測：rep movsd 和 memcpy、一般迴圈比速度，見 [§20.2](#202-rep-movsd-快不快)。）

---

## 8. x87 浮點的機器模型

[22:39] 【口頭】學生都說 x86 的浮點指令很複雜、不想學。老師說：一點都不複雜，只要懂背後的機器模型。

[23:02] 機器模型：

| 可以 | 不行 |
|---|---|
| 常數 → 記憶體 | 常數 → 浮點暫存器（**沒有浮點 immediate**） |
| 常數 → 整數暫存器 | 整數暫存器 ↔ 浮點暫存器（**沒有直接連線**） |
| 整數暫存器 ↔ 記憶體 | |
| 浮點暫存器 ↔ 記憶體 | |

[23:43] 浮點暫存器「人脈很弱」：**只能跟記憶體講話**。

[25:18] 8 個浮點暫存器組成一個 **stack**：

- **st0** 永遠是 stack 頂端，st7 是底。
- 名字是「指向 stack 的指標」，push 之後大家的名字都會變。
- 例子：st0 = 4.5，push 3.5 → st0 = 3.5、st1 = 4.5。

【課本補充】課本 p.207：8086 一直到 486 之前，處理器本身沒有浮點單元，要接一顆另外的 **x87 協同處理器**晶片。486 之後浮點單元才整合進來。很多怪特性（沒有直接連線、不能載入 immediate）都是那個年代的遺跡：浮點指令本來就是「送一個訊息給外面的晶片」。

---

## 9. fld、fild

[24:50]

| 指令 | 例子 | 做什麼 |
|---|---|---|
| `fld mem` | `fld dword [eax]` | 把記憶體裡的浮點數 push 到 FP stack |
| `fld reg` | `fld st1` | 把 st1 複製一份 push 上去 |
| `fild mem` | `fild dword [eax]` | 把記憶體裡的**整數**轉成浮點數再 push |

- [26:10] 記憶體運算元要寫大小：`dword` = 4 byte（32 位元的 float）。
- [27:09] `fld st1` 的 st1 是**浮點**暫存器，不是整數暫存器。
- [27:54] `fild` 的 i = integer：做 int → float 的轉換。

---

## 10. 浮點常數怎麼放進去

[28:53] 兩種方法：

1. 程式設計師自己把數字換成 IEEE 754 的 hex，存到記憶體，再 `fld`。
2. 讓**組譯器**做：NASM 在 `section .data` 宣告

```
section .data
num: dd 2.392          ; 32 位元浮點常數

...
fld dword [num]        ; num 會被換成那個位址
```

[31:26] 組譯器把 2.392 轉成 hex、放在記憶體某處，並把標籤 `num` 對應到那個位址。兩種方法本質一樣，只是誰來做轉換。

---

## 11. fxch、fst、fstp、fist

[32:29]

| 指令 | 做什麼 |
|---|---|
| `fxch st3` | st0 ↔ st3 |
| `fxch` | st0 ↔ st1 |
| `fst dword [eax]` | [eax] ← st0 |
| `fst st4` | st4 ← st0（像浮點暫存器之間的 mov） |
| `fstp …` | 跟 `fst` 一樣，做完再 **pop** |
| `fist dword [eax]` | 把 st0 轉成整數存到 [eax] |
| `fistp …` | 同上，再 pop |

- [34:57] `fst` 的來源永遠是 **st0**。
- [36:46] 沒寫的話，預設的來源或目的地幾乎都是 st0，它是很特別的位置。
- [37:51] 課本 Example 86（p.210）：把 st0 轉成整數放進 eax。因為浮點暫存器不能直接到整數暫存器，要經過記憶體：

```
fist dword [esp]
mov eax, [esp]
```

⚠️ **fist 是截斷還是捨入？** 影片 [35:38] 說 `fist`「把小數點後面全部去掉（truncate）」；課本 p.209 說 `fist`「**捨入（rounding）**成有號整數」。本機實測：`fist` 照目前的捨入模式（預設是四捨六入五成雙），2.7 → 3、2.5 → 2、3.5 → 4；**課本對**。真的要截斷要用 SSE3 加的 `fisttp`。見 [§20.3](#203-fist-是捨入不是截斷)。

---

## 12. fadd 與其他算術

[40:17]

| 形式 | 例子 | 做什麼 |
|---|---|---|
| 一個記憶體運算元 | `fadd dword [eax]` | st0 ← st0 + [eax] |
| 兩個暫存器 | `fadd st0, st1` | st0 ← st0 + st1（其中一個必須是 st0） |
| 加 p：pop | `faddp st1` | st1 ← st0 + st1，再 pop |
| 加 i：整數 | `fiadd dword [eax]` | st0 ← st0 + float([eax]) |
| NASM 的一個運算元版 | `fadd st1` | st0 ← st0 + st1 |

- [42:26] `faddp` 為什麼結果放在 st1？因為 st0 等一下要被 pop 掉，放 st0 就沒了。pop 之後原本的 st1 變成新的 st0。
- [44:09] ⚠️ 老師提醒：`fadd st1` 這種一個暫存器運算元的寫法是 **NASM 自己的**，不是 Intel 手冊裡的一般格式，別的組譯器不一定吃。
- [46:02] `fsub`、`fmul`、`fdiv` 有一樣的所有變體（記憶體、兩個暫存器、p、i）。

---

## 13. 例子：算術平均與幾何平均

[48:00] eax、ebx 是兩個整數，算平均，存成 **64 位元**（double）放在 [esp+4]。假設記憶體標籤 `two` 放著 2.0。

```
mov [esp], eax          ; 整數暫存器不能直接到 FP stack，先放記憶體
mov [esp+4], ebx
fild dword [esp]        ; 轉成浮點、push
fild dword [esp+4]
fadd st0, st1           ; st0 = 和
fdiv dword [two]        ; st0 = 和 / 2
fstp qword [esp+4]      ; 存 64 位元；用 fstp 順便清 stack（不是必要的）
```

- [50:05] eax = 8、ebx = 9 → **8.5**。
- [51:08] `qword` = quad word = 64 位元。
- [53:42] 幾何平均只改中間：`fmul st0, st1` 再 `fsqrt`。

（本機實測見 [§20.4](#204-平均與-sin-2θ)。）

---

## 14. 特殊函數

[52:36] CISC 的好處就是有這些：

| 指令 | 做什麼 |
|---|---|
| `fabs` | st0 ← \|st0\| |
| `fsqrt` | st0 ← √st0 |
| `fcos` | st0 ← cos(st0) |
| `fsin` | st0 ← sin(st0) |

[53:08] 還有其他超越函數、對數等，請查 Intel 手冊。

---

## 15. fcomi：要用 unsigned 的條件碼

[54:58]

| 指令 | 做什麼 |
|---|---|
| `fcomi st0, st1` | 比較 st0 和 st1，設 eflags（第一個必須是 st0） |
| `fcomip st0, st1` | 同上，做完再 pop |

[56:21] **非常常見的 bug**：`fcomi` 設的 flag 是給 **unsigned** 比較用的。

| 要用 | 不要用 |
|---|---|
| `ja`（above）、`jb`（below）、`jae`、`jbe`、`jna`、`jnb` | `jg`、`jl`、`jge`、`jle`（signed 的） |

[57:35] 學生常用 signed 的條件碼，得到錯的答案。

【課本補充】課本 p.213 一樣強調這點：用 `l`、`le`、`g`、`ge` 會得到錯的結果，要用 `a` 和 `b`。（本機實測把 `ja` 換成 `jg`，真的會錯，見 [§20.4](#204-平均與-sin-2θ)。）

---

## 16. 例子：sin 2θ = 2 sin θ cos θ

[58:18] 驗證這個三角恆等式。θ 放在 `.data` 的標籤 `theta`，門檻放在 `threshold`，相等就 eax = 1。

[58:56] **怎麼比較兩個浮點數？** 浮點數不精確。3.0 和 3.0 + 10⁻³⁰ 理論上不相等，但可能沒有足夠的 bit 表示差別，硬體會算成相等。標準做法：

```
|f1 − f2| < ε        ε 視需要，例如 10⁻⁵ 或 10⁻¹⁰
```

課本 Example 89（p.213–214），影片一樣：

```
; sin(2θ) → [esp]
fld  dword [theta]
fadd st0                 ; st0 = 2θ（NASM 寫法）
fsin
fstp dword [esp]
; 2 sinθ cosθ
fld  dword [theta]
fst  st1                 ; st1 = θ
fsin                     ; st0 = sinθ
fxch                     ; st0 = θ，st1 = sinθ
fcos                     ; st0 = cosθ
fmul st1                 ; st0 = sinθ cosθ
fadd st0                 ; st0 = 2 sinθ cosθ
; |差|
fld  dword [esp]         ; st0 = sin 2θ，st1 = 2 sinθ cosθ
fsub st0, st1
fabs
; 比較
fld  dword [threshold]   ; st0 = ε，st1 = |差|
fcomi st0, st1
ja .equal                ; ε 在「上面」：差 < ε
mov eax, 0
jmp .exit
.equal:
mov eax, 1
.exit:
```

- [65:35] 每次 `fld` 都會 push：原本的 st0 變 st1，st4 變 st5……
- [68:27] 這是恆等式，所以答案永遠是 1，除非程式有 bug。【口頭】老師說可以直接複製到 NASM 跑。

[69:19] 寫 x87 程式只要懂三件事：

1. FP stack：push 之後其他的往下移一格。
2. 指令把 st0 當成特別的位置。
3. 比較之後一定用 above／below 的條件碼。

---

## 17. 清理 FP stack：ffree、finit

[70:23]

| 指令 | 做什麼 |
|---|---|
| `ffree st4` | 把 st4 標成空的 |
| `finit` | 重設整個浮點單元：stack、暫存器、狀態全部清空 |

[70:46] 好習慣：程式（或某些段落）結束時清理 FP stack。因為 FP stack 是**程式狀態**的一部分：程式被暫停、換別的程式跑時，整個 FP stack 都要存起來再還原。不用浮點時讓它保持乾淨比較好。

【課本補充】課本 p.214：要清整個 stack，用 `ffree` 得一個一個清；`finit` 比較乾淨，讓下一個不相干的函式從乾淨的狀態開始。

---

## 18. 後記：x87 之後

[72:14] x86 的版本：

| 版本 | 現況 |
|---|---|
| 16 位元（8086） | 現在很少用，也許某些嵌入式 |
| 32 位元（x86、x86-32、i386） | 慢慢消失，嵌入式還有 |
| 64 位元（x86-64） | 現在主流，會用很久 |

[73:00] 老師說「據我所知」FP stack 大約從 **1987** 年、搭配 387 協同處理器就有了。這套架構有點老、有點原始。

[73:27] 之後 Intel 加了更多浮點暫存器：**MMX**、**SSE**、最近的 **AVX**（XMM、YMM 暫存器）。用 XMM 暫存器，很多程式碼會簡單很多。這本書沒講，有興趣自己去讀。

⚠️ 兩個小地方：

- 年份：課本 p.207 只說 8086 到 486 之前都要另外接 x87 晶片，沒有給年份；影片的「1987、387」是老師自己說「據我所知」。
- 影片 [73:52] 說「MMX 定義了 XMM 暫存器」。對照本機 gcc 的標頭檔：MMX 的型別是 64 位元的 `__m64`（`mmintrin.h:44`），128 位元的 `__m128`（XMM）在 SSE 的 `xmmintrin.h:69`。所以 XMM 是 **SSE** 的，不是 MMX 的。SSE 見 [第 41 支](lecture41_note.md)。

---

## 19. 【原始碼】Linux 的 memcpy 用 rep movsb

`rep movs` 不只是課本的玩具：Linux kernel 的 `memcpy` 在支援 **ERMS**（Enhanced REP MOVSB/STOSB）的 CPU 上，整個函式就是一條 `rep movsb`。`arch/x86/lib/memcpy_64.S:12-17`：

```
/*
 * We build a jump to memcpy_orig by default which gets NOPped out on
 * the majority of x86 CPUs which set REP_GOOD. In addition, CPUs which
 * have the enhanced REP MOVSB/STOSB feature (ERMS), change those NOPs
 * to a jmp to memcpy_erms which does the REP; MOVSB mem copy.
 */
```

`arch/x86/lib/memcpy_64.S:31-41`（一般的 `REP_GOOD` 版本：先 8 byte 一次用 `rep movsq`，剩下的用 `rep movsb`）：

```
SYM_TYPED_FUNC_START(__memcpy)
	ALTERNATIVE_2 "jmp memcpy_orig", "", X86_FEATURE_REP_GOOD, \
		      "jmp memcpy_erms", X86_FEATURE_ERMS

	movq %rdi, %rax
	movq %rdx, %rcx
	shrq $3, %rcx
	andl $7, %edx
	rep movsq
	movl %edx, %ecx
	rep movsb
```

`arch/x86/lib/memcpy_64.S:53-58`（ERMS 版本）：

```
SYM_FUNC_START_LOCAL(memcpy_erms)
	movq %rdi, %rax
	movq %rdx, %rcx
	rep movsb
	RET
SYM_FUNC_END(memcpy_erms)
```

- `rdi` 是目的地、`rsi` 是來源、`rcx` 是次數，就是課本的 edi、esi、ecx（64 位元版）。
- `ALTERNATIVE_2`：開機時看 CPU 有什麼功能，把開頭那條 `jmp` 換掉。本機 `/proc/cpuinfo` 有 `erms` 和 `fsrm`（fast short rep movsb），所以 kernel 用的是 `memcpy_erms`。

---

## 20. 【實測】

程式在 `experiments/lecture13/`：`x87.S` 是 Intel 語法的組合語言函式（x86-64，System V 呼叫慣例），`main.c` 呼叫它們。

```
$ gcc -O2 -o x87 main.c x87.S -lm && ./x87
```

### 20.1 字串指令與 rep

```
Example 81：eax = 192
rep movsd：dst = 0 1 2 3 4 5 6 7 8 9 ，之後 ecx = 0
std ＋ rep movsd（重疊、往後搬 2 格）：0 1 0 1 2 3 4 5 6 7
```

- 課本 Example 81 照抄（esp → rsp、esi → rsi）：192。
- Example 84 的 `cld; mov rcx, 10; rep movsd`：複製正確，做完 **ecx = 0**，跟影片說的一樣。
- **DF = 1 的用處**：把陣列在**自己裡面往後搬 2 格**（來源和目的地重疊）。從前面開始搬會蓋掉還沒搬的資料，所以 `copy_back` 讓 esi、edi 指向最後一個，`std` 後再 `rep movsd`，從尾巴往前搬，結果正確。做完要 `cld`：x86-64 的呼叫慣例規定函式回去時 DF 必須是 0。

### 20.2 rep movsd 快不快

`bench.c`：複製 64 MB，綁在 P-core 跑 3 次：

```
loop 10.8 GB/s | rep movsd 11.6 GB/s | memcpy 18.5 GB/s
loop 10.9 GB/s | rep movsd 12.5 GB/s | memcpy 18.6 GB/s
loop 10.8 GB/s | rep movsd 12.4 GB/s | memcpy 17.4 GB/s
```

- `loop` 用 `objdump` 確認過，正好是課本 Example 82：`mov` 讀、`mov` 寫、`add`、`cmp`、`jne`，一次 4 byte。
- `rep movsd` 一條指令就比 5 條的迴圈快一點（12 vs 11 GB/s）。
- glibc 的 `memcpy` 最快（約 18 GB/s），它會用更寬的 SIMD 暫存器或 `rep movsb`，細節**待補**。
- 這裡 64 MB 遠大於 L3（24 MB），量到的主要是記憶體頻寬，所以差距不大。

### 20.3 fist 是捨入，不是截斷

```
fist( 2.7) =  3   fisttp( 2.7) =  2
fist( 2.5) =  2   fisttp( 2.5) =  2
fist( 3.5) =  4   fisttp( 3.5) =  3
fist(-2.5) = -2   fisttp(-2.5) = -2
fist(-2.7) = -3   fisttp(-2.7) = -2
```

- `fist`：2.7 → 3（不是 2），所以**不是截斷**。2.5 → 2、3.5 → 4：剛好一半時捨到**偶數**（round half to even），這是 x87 預設的捨入模式。課本「rounding」正確，影片「truncating」是口誤。
- `fisttp`（SSE3 才加的）：一律往 0 截斷，就是 C 語言 `(int)x` 的行為。

### 20.4 平均與 sin 2θ

```
amean(8, 9) = 8.5
gmean(8, 9) = 8.485281（sqrt(72) = 8.485281）
sin 2θ vs 2 sinθ cosθ，θ=0.7，門檻 1e-5：ja → 1，jg → 1
門檻 −1（門檻比差小，應該是 0）：         ja → 0，jg → 1
```

- 影片的算術平均：8.5；幾何平均 √72 = 8.485281，和 C 的 `sqrt` 一樣。
- Example 89 照抄（`fld`、`fadd`、`fsin`、`fxch`、`fcos`、`fmul`、`fabs`、`fcomi`），θ = 0.7、門檻 10⁻⁵：`ja` 得 1，恆等式成立。
- **把 `ja` 換成 `jg`**：門檻是 −1（比差小），正確答案是 0。`ja` 給 0（對），`jg` 給 **1（錯）**。原因：`fcomi` 只設 ZF、PF、CF，把 SF、OF 清成 0。`jg` 看的是「ZF = 0 且 SF = OF」，SF 和 OF 永遠相等，所以只要不相等 `jg` 就成立，大小完全沒看。這就是 [§15](#15-fcomi要用-unsigned-的條件碼) 的 bug。

⚠️ 寫這個測試時我自己踩了兩個坑：(1) 在 `fcomi` 和 `ja` 中間放了一條 `test`，把 flag 蓋掉了，結果全錯；改成 `fcomi` 之後立刻 `seta`／`setg` 存起來。(2) 函式是 `sin_check(float, float, int)`，兩個 float 走 `xmm0`、`xmm1`，所以 int 是在 `edi`，不是我以為的 `edx`。

---

## 21. 一頁速查表

| 主題 | 重點 |
|---|---|
| lea | 只算位址放進暫存器，不讀記憶體 |
| stosd | [edi] ← eax；edi ± 4 |
| lodsd | eax ← [esi]；esi ± 4 |
| movsd | [edi] ← [esi]；兩個都 ± 4 |
| 後綴 | b 1、w 2、d 4、q 8 byte |
| DF | 0：加；1：減；`cld`、`std` |
| rep | 做 ecx 次，ecx 最後是 0；`repe`、`repne` 多一個 ZF 條件 |
| 複製陣列 | `cld` ＋ `mov ecx, n` ＋ `rep movsd` |
| x87 模型 | 8 個暫存器的 stack（st0 是頂）；只能跟記憶體交換；沒有浮點 immediate |
| fld／fild | push 浮點數／整數轉浮點再 push |
| 常數 | 放在 `.data`（`num: dd 2.392`），再 `fld dword [num]` |
| fst／fstp／fist | 存 st0／存了再 pop／轉整數（**捨入**）再存 |
| fadd 系列 | 記憶體、兩個暫存器（含 st0）、p（pop，結果放 st_i）、i（整數） |
| 特殊函數 | fabs、fsqrt、fsin、fcos |
| fcomi | 設 eflags；用 **ja／jb**，不能用 jg／jl |
| 浮點相等 | \|f1 − f2\| < ε |
| 清理 | ffree st_i、finit |
| 後來 | MMX、SSE（XMM）、AVX（YMM） |
| 實測 | rep movsd 後 ecx = 0；std 做重疊往後搬；fist 2.5 → 2、2.7 → 3；把 ja 換 jg 會錯；Linux memcpy = rep movsb |

---

## 22. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| x86 暫存器、esi／edi、定址模式 | [第 11 支](lecture11_note.md) |
| mov、cmp、jcc（含 above／below 和 greater／less 的分別） | [第 12 支](lecture12_note.md) |
| x86 指令怎麼編碼（prefix、ModR/M、SIB） | [第 14 支](lecture14_note.md) |
| IEEE 754、浮點數不精確 | [第 5 支](lecture05_note.md) |
| 浮點加法的硬體、捨入 | [第 25 支](lecture25_note.md) |
| SSE、AVX、XMM 暫存器 | [第 41 支](lecture41_note.md) |
| 程式狀態（context）要存什麼 | [Linux OS 第 2 講 §7](../../os/linux-os/lecture02_note.md#7-context要存哪些東西) |

---

## 附：本講一句話總結

> **x86 的 CISC 特色在進階記憶體指令：lea 只算位址不碰記憶體，字串指令 stosd（[edi] ← eax）、lodsd（eax ← [esi]）、movsd（[edi] ← [esi]）做完會依 direction flag（cld 設 0 往上、std 設 1 往下）自動把 edi／esi 加減 4，再加上 rep prefix 就能把整個迴圈塞進一條指令（cld、mov ecx, 10、rep movsd 就複製完 10 個 int）；x87 浮點則是一個 8 格的暫存器 stack（st0 在頂端），只能經過記憶體和外界交換資料、沒有浮點 immediate，用 fld／fild push、fst／fstp／fist 存回（fist 會捨入）、fadd／fsub／fmul／fdiv 及 p、i 變體做運算、fsqrt／fsin／fcos 做特殊函數，比較用 fcomi 而且一定要接 ja／jb 這類 unsigned 條件碼，浮點相等要用 |差| < ε 判斷，最後用 ffree／finit 清理。本機實測：rep movsd 做完 ecx = 0、fist 把 2.5 捨成 2 而不是截斷、把 ja 換成 jg 真的會算錯，而 Linux kernel 的 memcpy 在這台 CPU 上就是一條 rep movsb。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 5: x86 assembly, Part 3/4 的 whisper 轉錄稿（本機 `large-v3-turbo`，1:14:33）、Sarangi *Basic Computer Architecture* v3.09 Ch5 §5.3.4–5.4.5（p.203–214）、Linux v6.2.12 `arch/x86/lib/memcpy_64.S` 整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、GNU as）結果。*
