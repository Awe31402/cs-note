# Basic Computer Architecture 第十二支影片完整筆記：x86 的整數、分支與函式指令（Chapter 5, Part 2/4）

> **課程**：Basic Computer Architecture — Chapt 5: x86 assembly, Part 2/4（播放清單第 12 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（1:29:15，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 Ch5 §5.3.1–5.3.3（p.185–203）
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、NASM）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. mov：一條抵三條](#1-mov一條抵三條)
- [2. movsx 與 movzx](#2-movsx-與-movzx)
- [3. xchg](#3-xchg)
- [4. push 與 pop](#4-push-與-pop)
- [5. 記憶體運算元的大小：byte、word、dword、qword](#5-記憶體運算元的大小byteworddwordqword)
- [6. 例子：push dword [esp]](#6-例子push-dword-esp)
- [7. 加減法：add、sub、adc、sbb](#7-加減法addsubadcsbb)
- [8. inc、dec、neg](#8-incdecneg)
- [9. cmp](#9-cmp)
- [10. 乘法：imul 的三種形式](#10-乘法imul-的三種形式)
- [11. 除法：idiv 與 edx 的陷阱](#11-除法idiv-與-edx-的陷阱)
- [12. 邏輯與位移指令](#12-邏輯與位移指令)
- [13. 分支：jmp 與 jcc](#13-分支jmp-與-jcc)
- [14. 例子：判斷質數](#14-例子判斷質數)
- [15. call 與 ret](#15-call-與-ret)
- [16. 遞迴的階乘：四種寫法](#16-遞迴的階乘四種寫法)
- [17. enter 與 leave](#17-enter-與-leave)
- [18. 【實測】](#18-實測)
- [19. 一頁速查表](#19-一頁速查表)
- [20. 與後續講次的連結](#20-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

上一講講了 x86 的機器模型。這一講是 x86 的「基本指令」：

```
資料搬移：mov（= RISC 的 mov + load + store）、movsx、movzx、xchg、push、pop
ALU：add sub adc sbb、inc dec neg、cmp、imul idiv、and or xor not、sar shr sal/shl
分支：jmp、j<條件>（je、jne、jl、jg、jb…）
函式：call（push return address）、ret（pop）、ebp、enter、leave
```

**x86 和 RISC 最大的不同**：硬體**知道 stack 的存在**（push、pop、call、ret 都會自動改 esp），而且大部分指令可以直接存取記憶體。

---

## 1. mov：一條抵三條

[00:35]–[05:49]　課本 Table 5.3、Example 55–57（p.185–186）

```
mov (reg/mem), (reg/mem/imm)
```

- 第一個運算元是目的地（暫存器或記憶體），第二個是來源（暫存器、記憶體或立即值）。
- 【口頭】「mov 可以說是 x86 最萬能的指令」，它等於 RISC 的**三條**指令：

| x86 | 等於 RISC 的 |
|---|---|
| `mov eax, ebx` | mov（暫存器 → 暫存器） |
| `mov eax, 3` | mov（立即值 → 暫存器） |
| `mov eax, [esp + eax*4 + 8]` | **load**（記憶體 → 暫存器） |
| `mov [esp + eax*4 + 8], ebx` | **store**（暫存器 → 記憶體） |

- 只要會 mov，就能寫出很多組合語言程式。
- 規則照舊：**兩個運算元不能都是記憶體**，所以不能用 mov 在兩個記憶體位置之間搬資料。
- 記憶體運算元用方括號：base 暫存器、scaled index、offset（x86 叫 **displacement**）。
- ⚠️ 逐字稿 [03:27] 的記憶體位址例子聽不清楚（「num uh something of this type」），只聽得出用到了 esp 和 eax；上表的 `[esp + eax*4 + 8]` 是示意。

---

## 2. movsx 與 movzx

[05:49]–[08:47]　課本 Table 5.4（p.186）

| 語意 | 例子 | 意思 |
|---|---|---|
| `movsx reg, (reg/mem)` | `movsx eax, bx` | eax ← **sign extend**(bx) |
| `movzx reg, (reg/mem)` | `movzx eax, bx` | eax ← **zero extend**(bx) |

- 普通的 mov 假設來源和目的地**一樣大**；movsx、movzx 是「從小搬到大」：來源 8 或 16 位元，目的地比較大。
- 第一個運算元只能是**暫存器**；第二個是暫存器或記憶體。
- 多出來的高位放什麼？
  - **movsx**（S = sign）：放**符號位**，所以值不變（有號數）。
  - **movzx**（Z = zero）：放 **0**（當成無號數）。
- ⚠️ 影片口誤說「`movsx ax, bx`」，接著說「EAX 是 32 位元暫存器」。目的地應該是 **eax**（ax 和 bx 一樣大，不需要擴展）。
- [第 4 支 §13.2](lecture04_note.md#132-硬體怎麼做-sign-extension) 實測過：gcc 把 `signed char` 轉 `int` 編成 movsx，`unsigned char` 編成 movzx。

---

## 3. xchg

[08:47]–[10:18]　課本 Table 5.5（p.187）

| 語意 | 例子 | 意思 |
|---|---|---|
| `xchg (reg/mem), (reg/mem)` | `xchg eax, [eax + edi]` | **交換**兩者的內容 |

- 兩個運算元都是來源也都是目的地；不能兩個都是記憶體。
- 【口頭】xchg 非常重要，**在寫平行程式時很有用**，但要到 Chapter 11 才能理解為什麼，而且那裡也只會講一點。⚠️ 課本 v3.09 是 **Ch12**（多處理器）。
- 【課本補充】xchg 和記憶體交換時是**原子的**（atomic），所以可以用來實作 lock。（OS 那邊的同步見 [`os/linux-os` 第 18 支](../../os/linux-os/lecture18_note.md)。）

---

## 4. push 與 pop

[10:18]–[13:27]　課本 Table 5.6（p.188）

x86 處理器**明確知道 stack**，知道 stack pointer 在 **esp**。所以有兩條專門的指令：

| 語意 | 例子 | 等於 |
|---|---|---|
| `push (reg/mem/imm)` | `push ecx` | temp ← ecx；**esp ← esp − 4**；[esp] ← temp |
| `pop (reg/mem)` | `pop ecx` | temp ← [esp]；**esp ← esp + 4**；ecx ← temp |

- stack 往下長：push 時 esp **減** 4，pop 時 esp **加** 4。
- 【口頭】這就是 CISC：一條指令做了四、五條 RISC 指令的事。
- **為什麼要先讀到 temp？** 這個順序很重要：push 先讀運算元的值，**再**改 esp。如果運算元本身用到 esp（例如 `push dword [esp]`），就會讀到**舊的** esp 指的東西。§6 的例子。
- 【口頭】不用 temp 也能寫成兩步，但「老師故意寫出 temp，表示處理器內部的動作順序」。

---

## 5. 記憶體運算元的大小：byte、word、dword、qword

[13:27]–[17:08]　課本 Table 5.7（p.188）

- 暫存器運算元的大小從**名字**就知道：eax 32 位元、ax 16 位元、rax 64 位元。
- **記憶體運算元**就不知道了：`push [esp]` 要 push 幾個 byte？2？4？8？
- 所以要加 **modifier**（大小修飾詞）：

| Modifier | 大小 |
|---|---|
| `byte` | 8 位元（1 byte） |
| `word` | **16 位元**（2 byte） |
| `dword` | 32 位元（4 byte，double word） |
| `qword` | 64 位元（8 byte，quad word） |

- 例：`push dword [esp]`：push 4 個 byte。
- ⚠️ **x86 的 word 是 16 位元**！SimpleRisc 和 ARM 的 word 是 32 位元（4 byte）。這是**歷史原因**：8086 是 16 位元處理器。
- 【課本補充】NASM 在 32 位元模式下，不寫 modifier 時預設是 dword。

---

## 6. 例子：push dword [esp]

[17:08]–[21:25]　課本 Example 59–61（p.188–189）

【口頭】「我故意選了一個稍微複雜的例子。」

```
mov eax, 10
mov [esp], eax         ; stack 頂端 = 10
push dword [esp]       ; 讀 [esp]（= 10），esp − 4，再把 10 放到新的頂端
mov ebx, [esp]         ; ebx = 新的頂端 = 10
```

答案：**ebx = 10**。

- 關鍵：push 的運算元 `[esp]` 是**先**讀出來的（讀到 10），之後 esp 才減 4，最後 10 放到新的位置。
- ⚠️ 影片 [19:47] 說「假設之前 stack pointer 是 1,000，現在的位址是 **1,004**」。這不對：stack 往下長，push 之後 esp **減** 4，應該是 **996**。§18.1 實測：push 之後 esp 少了 4。

【課本補充】課本的 Example 60 寫法稍微不同，用 ebp 記住原本的 esp：

```
mov ebp, esp
mov eax, 10
mov [esp], eax
push dword [esp]
mov ebx, [ebp-4]       ; push 之後的頂端 = 原本的 esp − 4
```

課本還有 Example 59（push eax 再 mov ebx, [esp] → 10）和 Example 61（push 17、`pop dword [esp]` 再讀 → 17）。

---

## 7. 加減法：add、sub、adc、sbb

[22:33]–[26:22]　課本 Table 5.8、Example 62–63（p.189–190）

| 語意 | 例子 | 意思 |
|---|---|---|
| `add (reg/mem), (reg/mem/imm)` | `add eax, ebx` | eax ← eax + ebx |
| `sub (reg/mem), (reg/mem/imm)` | `sub eax, ebx` | eax ← eax − ebx |
| `adc (reg/mem), (reg/mem/imm)` | `adc eax, ebx` | eax ← eax + ebx + **CF** |
| `sbb (reg/mem), (reg/mem/imm)` | `sbb eax, ebx` | eax ← eax − ebx − **CF** |

- 2-address：第一個運算元是第一個來源，也是目的地。兩個不能都是記憶體。
- adc、sbb 和 ARM 一樣用來做**多倍精度**運算：先算低位，把 carry 存起來，用它算高位。
- **x86 的 carry 和 ARM 的意思相反**（[25:07]）：
  - ARM：減法**借位時 C = 0**，所以 sbc 減掉 NOT(C)。
  - **x86：減法借位時 CF = 1**，所以 sbb 直接減掉 CF。CF 就是借位位元。
  - ⚠️ 影片這句話逐字稿是「unlike ARM and x86, the carry bit and the borrow bit are the same」，應該是「和 ARM 不同，x86 的 carry bit 就是 borrow bit」。§18.1 實測：0 − 5 之後 x86 的 CF = 1（[第 9 支](lecture09_note.md#122-nzcv-flags-與-64-位元加減法) 實測 ARM 的 C = 0）。

【課本補充】Example 62、63：64 位元數存在 ebx:eax 和 edx:ecx

```
add eax, ecx       ; 低 32 位
adc ebx, edx       ; 高 32 位 + carry

sub eax, ecx
sbb ebx, edx       ; 高 32 位 − borrow
```

---

## 8. inc、dec、neg

[26:22]–[27:42]　課本 Table 5.9、Example 64（p.190–191）

| 語意 | 例子 | 意思 |
|---|---|---|
| `inc (reg/mem)` | `inc edx` | edx ← edx + 1 |
| `dec (reg/mem)` | `dec edx` | edx ← edx − 1 |
| `neg (reg/mem)` | `neg edx` | edx ← −1 × edx |

- 1-address：唯一的運算元同時是來源和目的地。

**Example 64**：eax = −1 × (eax + 1)

```
inc eax
neg eax
```

---

## 9. cmp

[27:42]–[28:58]　課本 Table 5.10（p.191）

| 語意 | 例子 | 意思 |
|---|---|---|
| `cmp (reg/mem), (reg/mem/imm)` | `cmp eax, [ebx + 4]` | 比較 eax 和記憶體 [ebx + 4]，設定 flags |
| | `cmp ecx, 10` | 比較 ecx 和 10 |

- 結果存在 flags（flags／eflags／rflags）。
- 【課本補充】做法是**第一個減第二個**：「概念上就是一個沒有目的地的 sub 指令」。
- ⚠️ 影片 [27:59] 說「第一個運算元可以是暫存器、記憶體**或立即值**」。課本的表和 x86 的實際規則是：第一個運算元只能是**暫存器或記憶體**，立即值只能放第二個。

---

## 10. 乘法：imul 的三種形式

[28:58]–[35:07]　課本 Table 5.11、Example 65–66（p.191–193）

| 形式 | 例子 | 意思 |
|---|---|---|
| **1 個運算元** | `imul ecx` | **edx:eax** ← eax × ecx（64 位元結果） |
| **2 個運算元** | `imul ecx, [eax + 4]` | ecx ← ecx × [eax + 4]（截成 32 位元） |
| **3 個運算元** | `imul ecx, [eax + 4], 5` | ecx ← [eax + 4] × 5 |

**1 個運算元**：

- 預設**和 eax 相乘**。
- 兩個 32 位元有號數相乘最多要 63 位元（(−2³¹) × (−2³¹) = 2⁶²）。【口頭】「63 是個爛數字，就用 64 位元吧，2 的冪次。」
- 結果放兩個暫存器：**edx 放高 32 位，eax 放低 32 位**。
- 例：eax = 17、ecx = 1000，`imul ecx` → edx:eax = 17000。

**2 個運算元**：和 add、sub 一樣的普通指令。結果只留 32 位元；overflow 時會設定 **overflow flag**。

**3 個運算元**：第一個是目的地（暫存器），第二個是暫存器或記憶體，第三個是**立即值**。

- 【口頭】這三種形式都是「仔細考慮過」才設計的：
  1. 需要 64 位元乘積時（ARM 也有類似的 smull）
  2. 像 add、sub 一樣的普通指令
  3. 「暫存器或記憶體乘以常數，存到另一個暫存器」很常見
- 【口頭】CISC 指令集可以「放心地有很多種格式，什麼適合就用什麼」。

【課本補充】

- **imul／idiv 是有號的**；**mul／div 是無號的**，語意一樣。
- Example 65：3 × (−17) → `mov ebx, 3` ＋ `imul eax, ebx, -17`。
- Example 66：k³（k 在 ecx）→ `mov eax, ecx` ＋ `imul ecx` ＋ `imul ecx`。

---

## 11. 除法：idiv 與 edx 的陷阱

[35:07]–[43:07]　課本 Table 5.11、Example 67（p.192–193）

```
idiv (reg/mem)       ; edx:eax ÷ 運算元 → eax = 商，edx = 餘數
```

- 【口頭】idiv「當然很複雜」。這也是早期 ARM 指令集**沒有除法指令**的原因之一（後來的版本才有）。
- **被除數是 64 位元**：放在 **edx:eax**（edx 高 32 位、eax 低 32 位）。這正好是 1 個運算元 imul 的反向。
- **除數**是運算元（暫存器或記憶體）。
- 除法的難處：會產生**兩個**結果。例：10 ÷ 7 → 商 1、餘 3；15 ÷ 7 → 商 2、餘 1。
- 所以結束後：**eax = 商**、**edx = 餘數**。【口頭】「edx 和 eax 是一對，會很常一起出現。」
- 【課本補充】**餘數的符號和被除數一樣**。

**最重要的陷阱**（[39:43]，老師打了兩顆星、還寫上 important）：

- 被除數只有 32 位元（放在 eax）時，**edx 要設成 eax 的 sign extension**：
  - eax 是正數 → edx = 0（全 0）
  - eax 是**負數** → edx = **全 1**（`mov edx, -1`）
- 例：eax = −10，edx 一定要是全 1。如果把 edx 設成 0，edx:eax 就變成一個很大的正數，答案全錯。「這是 idiv 很大的 bug 來源。」
- §18.2 實測：gcc 用一條專門的指令 **`cdq`**（convert doubleword to quadword）做這件事，把 eax 的符號位複製到整個 edx。

**Example 67**：−50 ÷ 3，商放 eax、餘數放 edx

```
mov edx, -1
mov eax, -50
mov ebx, 3
idiv ebx
```

- 答案：**eax = −16**，**edx = −2**。
- ⚠️ 影片 [43:07] 說「eax 會是商，這裡是 **−3**」。這是口誤：−50 ÷ 3 的商是 **−16**（−16 × 3 = −48，−50 − (−48) = −2）。課本寫的是 −16，§18.1 實測也是 −16。

---

## 12. 邏輯與位移指令

[43:07]–[49:09]　課本 Table 5.12–5.13、Example 68–70（p.193–194）

**邏輯**：

| 語意 | 例子 | 意思 |
|---|---|---|
| `and (reg/mem), (reg/mem/imm)` | `and eax, ebx` | eax ← eax AND ebx |
| `or (reg/mem), (reg/mem/imm)` | `or eax, ebx` | eax ← eax OR ebx |
| `xor (reg/mem), (reg/mem/imm)` | `xor eax, ebx` | eax ← eax XOR ebx |
| `not (reg/mem)` | `not eax` | eax ← ~eax |

- and、or、xor 的格式和 add、sub 一樣；not 和 inc、dec 一樣（1-address）。

**位移**：

| 語意 | 例子 | 意思 |
|---|---|---|
| `sar (reg/mem), imm` | `sar eax, 3` | 算術右移（複製符號位）＝ 有號數 ÷ 2ⁿ |
| `shr (reg/mem), imm` | `shr eax, 3` | 邏輯右移（補 0）＝ 無號數 ÷ 2ⁿ |
| `sal`／`shl (reg/mem), imm` | `sal eax, 2` | 左移（sal 和 shl 是**同一條**指令） |

- 【口頭】「我故意講快一點」，因為位移在第 2、3 章講過了。
- 沒有算術左移和邏輯左移的分別，所以 sal 和 shl 是同一條指令的兩個名字。
- ⚠️ 位移量其實也可以放在 **cl** 暫存器（例如 `shl eax, cl`），不只是立即值。課本的表只列了立即值。

**例子**：`0xdeadc0de`

```
mov eax, 0xdeadc0de
sar eax, 4          ; 0xfdeadc0d
shr eax, 4          ; 0x0deadc0d
sal eax, 4          ; 0xeadc0de0
```

- 右移 4 位 = 移掉一個十六進位數字。最高位是 d = `1101`，MSB 是 1。
  - sar：複製 1 → 補上 `1111` = **f**。
  - shr：補 0 → 0x**0**deadc0d。
- ⚠️ 逐字稿把 `0xdeadc0de` 聽成「dead feet」「dead fee」。課本的例子是 `0xdeadc0de`（「dead code」）。

---

## 13. 分支：jmp 與 jcc

[49:09]–[55:59]　課本 Table 5.14–5.15（p.194–195）

- x86 的分支叫 **jump**。【口頭】「如莎士比亞所說，玫瑰換個名字還是玫瑰，分支換個名字還是分支。」

| 語意 | 例子 | 意思 |
|---|---|---|
| `jmp <label>` | `jmp .foo` | 無條件跳到 .foo |
| `j<條件> <label>` | `je .foo` | 條件成立就跳 |

- 和 ARM 的 b ＋ 條件碼一樣，只是 **b 換成 j**。
- 條件看**最後一個設定 flags 的指令**（例如 cmp）。中間可以有很多不設定 flags 的指令。
- 條件不成立就「掉下去」（fall through），執行下一條。

**條件碼**（課本 Table 5.15）：

| 條件碼 | 意思 |
|---|---|
| o | overflow |
| no | 沒有 overflow |
| **b** | **below**（無號小於） |
| nb | not below（無號大於或等於） |
| **e／z** | equal 或 zero（兩個是同一個條件） |
| ne／nz | 不相等或不是 0 |
| be | below or equal（無號小於或等於） |
| s | 符號位是 1（負數） |
| ns | 符號位是 0（0 或正數） |
| **l** | less（有號小於） |
| le | 有號小於或等於 |
| **g** | greater（有號大於） |
| ge | 有號大於或等於 |

- **為什麼 e 和 z 一樣？** 比較 A 和 B 就是算 A − B；A == B 等於 A − B == 0。
- **無號用 below／above，有號用 less／greater**。
- 【口頭】s／ns 就像 ARM 的 mi／pl。「指令集之間的差異很多只是表面的；底層的機制，例如分支和條件碼，大多一樣，而且很多受到更早的設計啟發。」
- ⚠️ 影片 [50:50] 說「JEQ」，x86 的寫法是 **je**（沒有 jeq）；課本寫的是 je。

【課本補充】Example 71：加總 1 到 10

```
	mov eax, 0      ; sum = 0
	mov ebx, 1      ; idx = 1
.loop:
	add eax, ebx    ; sum += idx
	inc ebx         ; idx ++
	cmp ebx, 10     ; compare idx and 10
	jle .loop       ; jump if idx <= 10
```

---

## 14. 例子：判斷質數

[55:59]–[61:38]　課本 Example 72（p.196）

判斷 eax 是不是質數（假設 > 10），結果放 eax（1 = 質數，0 = 不是）：

```
	mov ebx, 2        ; starting index
	mov ecx, eax      ; ecx contains the original number
.loop:
	mov edx, 0        ; required for correct division
	idiv ebx
	cmp edx, 0        ; compare the remainder
	je .notprime      ; number is composite
	inc ebx
	mov eax, ecx      ; set the value of eax again
	cmp ebx, eax      ; compare the index and the number
	jl .loop
	; end of the loop
	mov eax, 1        ; number is prime
	jmp .exit
.notprime:
	mov eax, 0
.exit:
```

- 演算法：用 2、3、…、N − 1 去除 N。有任何一個餘數是 0，就有 1 和 N 以外的因數，不是質數。
- **mov edx, 0**：idiv 要 64 位元的 edx:eax。數字是正的，所以 edx = 0（§11 的 sign extension）。
- 餘數在 edx，商在 eax。**idiv 會蓋掉 eax**，所以每一輪都要從 ecx 把原來的數放回 eax。
- 【口頭】「13 條指令，不難。但問題是：聽課的同學能不能**從零**寫出這種程式？這不是聽課或讀書能學會的，**唯一的方法是盡量多練習**，寫幾百個組合語言程式。」
- §18.1 實測：97 → 1、91 → 0、11 → 1。

【課本補充】Example 73：迴圈版的階乘（結果放 ecx）

```
	mov ebx, 2        ; idx = 2
	mov ecx, 1        ; prod = 1
.loop:
	imul ecx, ebx     ; prod *= idx
	inc ebx           ; idx++
	cmp ebx, eax      ; compare num (number) and idx
	jle .loop         ; jump to .loop if idx <= num
```

---

## 15. call 與 ret

[61:38]–[64:23]　課本 Table 5.16（p.197）

| 語意 | 例子 | 意思 |
|---|---|---|
| `call <label>` | `call .foo` | **把 return address push 到 stack**，跳到 .foo |
| `ret` | `ret` | 跳到 stack 頂端存的位址，**pop** 掉它 |

- 和 SimpleRisc、ARM 一樣的地方：`call label`；不只是跳躍，還要處理 return address。
- 不一樣的地方：SimpleRisc 和 ARM 把 return address 放在 **return address 暫存器**；**x86 沒有這個暫存器**，完全靠 **stack**：硬體知道 esp，直接把 return address push 上去。
- ret 是 0-address 指令。

一個典型的 x86 函式：

1. 呼叫前：把參數放在 stack 或暫存器
2. 函式開始：從 stack 拿出參數；在 stack 上建立自己的 activation block；需要的話 spill 暫存器（caller saved／callee saved）
3. 做自己的事、呼叫其他函式
4. 把回傳值放到指定的記憶體位置或暫存器
5. **恢復 stack pointer**，return

---

## 16. 遞迴的階乘：四種寫法

[64:23]–[82:38]　課本 Example 74–77（p.198–201）

【口頭】「為什麼總是用遞迴函式當例子？因為我們要給**最難**的例子，希望激發讀者深入研究。」

題目：eax（≥ 1）的階乘，結果放 ebx。

### 寫法 1：參數用暫存器，push／pop 保存（Example 74）

```
factorial:
	mov ebx, 1          ; default return value（0! = 1! = 1）
	cmp eax, 1          ; compare num (input) with 1
	je .return          ; return if input is equal to 1
	; recursive step
	push eax            ; save input on the stack
	dec eax             ; num--
	call factorial      ; recursive call（之後 ebx = (num−1)!）
	pop eax             ; retrieve input
	imul ebx, eax       ; prod = prod * num
.return:
	ret
```

- 用 je 或 jz 都可以（同一個條件）。
- 為什麼 push eax？因為要改 eax 傳給下一層，回來後還要用原本的值。「只有一個值，所以叫它 caller saved 或 callee saved 都可以。」
- **ret 能正確回去，是因為每個 push 都有一個對應的 pop**，stack 的大小沒變，ret 會在頂端找到 return address。
- 【口頭】9 條指令，用 C 寫也要 3、4 行，「所以 x86 組合語言其實沒有那麼沒效率」。

### 寫法 2：參數用 stack（Example 75，課本補充）

```
factorial:
	mov eax, [esp+4]    ; get the value of eax from the stack
	...
	push eax            ; save eax on the stack
	dec eax
	push eax            ; push the argument
	call factorial
	pop eax             ; pop the argument
	pop eax             ; retrieve the value of eax
	...
```

**為什麼參數在 [esp+4]**（[73:31]）：

```
            ┌─────────────────┐
   esp+4 →  │ 參數（num）       │ ← caller 在 call 之前 push 的
   esp   →  │ return address  │ ← call 指令 push 的
            └─────────────────┘
```

- 進入函式時，stack 頂端（[esp]）是 **return address**，因為 call 最後才 push 它。
- caller 在 call **之前** push 的參數就在它上面一格：**[esp+4]**（stack 往下長，所以「上面」是位址比較大的地方）。

### 寫法 3：不用 push／pop，直接管理 stack（Example 76）

```
	sub esp, 8          ; create space on the stack
	mov [esp+4], eax    ; save the input eax on the stack（spill）
	dec eax             ; num--
	mov [esp], eax      ; push the argument（給下一層的參數）
	call factorial
	mov eax, [esp+4]    ; retrieve eax
	imul ebx, eax
	add esp, 8          ; restore the stack pointer
```

- 一次在 stack 上建立 **8 byte 的 activation block**：
  - [esp+4]：spill 的 eax（num）
  - [esp]：傳給下一層的參數（num − 1）
- 下一層進去後，[esp] 是 return address，[esp+4] 正好是我們放的 num − 1。
- 【課本補充】參數很多時，push、pop 的順序容易搞錯、引入 bug；**一次減掉 activation block 的大小、用 mov 存取**比較好。
- 【口頭】效能上**不會比較好**：push、pop 本身就包含很多功能。

### 寫法 4：用 ebp 記住 esp（Example 77）

**問題**（[70:23]）：C++ 這種語言可以在函式中間**動態宣告區域變數**，activation block 的大小連 compile time 都不知道，執行時才會變。那函式結束時，要加多少回 esp？

**解法**：用 **ebp**（base pointer，也叫 **frame pointer**）。

```
factorial:
	mov eax, [esp+4]    ; get the value of eax from the stack
	push ebp            ; save ebp
	mov ebp, esp        ; save the stack pointer
	mov ebx, 1
	cmp eax, 1
	je .return
	sub esp, 8
	mov [esp+4], eax
	dec eax
	mov [esp], eax
	call factorial
	mov eax, [esp+4]
	imul ebx, eax
.return:
	mov esp, ebp        ; restore the stack pointer
	pop ebp             ; restore ebp
	ret
```

- 函式一開始：**ebp ← esp**（記住開始時的 esp）。中間 stack 怎麼長都沒關係。
- 函式結束：**esp ← ebp**，一步就恢復。「**刪掉 activation block = 恢復 stack pointer**。」
- 被呼叫的函式也會用 ebp，所以 ebp 要先 push 起來、最後 pop 回去。
- 【課本補充】所以 **ebp 是 callee saved** 暫存器：函式如果改了 ebp，回傳前一定要恢復。

---

## 17. enter 與 leave

[82:38]–[89:04]　課本 Table 5.17、Example 78（p.201–202）

寫法 4 多出來的那幾行幾乎每個大函式都有，所以 x86 專門設計了兩條指令：

| 語意 | 例子 | 等於 |
|---|---|---|
| `enter imm, 0` | `enter 32, 0` | push ebp；mov ebp, esp；esp ← esp − 32 |
| `leave` | `leave` | mov esp, ebp；pop ebp |

- **enter**「三合一」：保存 ebp、ebp ← esp、建立 activation block（第一個參數是大小）。
- 第二個參數是函式的**巢狀層級**（nesting level），比較複雜；實務上一律用 **0**。
- **leave**：恢復 esp 和 ebp。要在 **ret 前一條**用。

**Example 78**：

```
factorial:
	mov eax, [esp+4]    ; get the value of eax from the stack
	enter 8, 0          ; save ebp and esp, decrement esp by 8
	mov ebx, 1
	cmp eax, 1
	je .return
	mov [esp+4], eax    ; save input on the stack
	dec eax
	mov [esp], eax      ; push the argument
	call factorial
	mov eax, [esp+4]
	imul ebx, eax
.return:
	leave               ; load esp and ebp
	ret
```

**什麼時候用哪一種**：

- 小而簡單的函式：push、pop 很好用。但 **push 和 pop 必須完全配對**：n 個 push 就要 n 個 pop，否則 stack 不對，ret 會跳錯地方。在複雜的函式裡很難追蹤。
- 大函式：一開始 **enter**，最後 ret 之前 **leave**。中間 activation block 可能有 100 個整數或更多、大小隨執行路徑改變都沒關係，leave 會把它整個刪掉。【口頭】「這是好的 x86 程式設計習慣。」
- 【課本補充】x86 也有 **nop**：除了 Ch10 會講的正確性用途，也用來把程式碼**對齊**到 16 或 64 byte 的邊界。

【口頭】x86 也有像 ARM 的**條件指令**，叫 **cmov**（conditional move）。本講不討論；x86 手冊有幾百條指令，很多都有很酷的功能。

⚠️ 現代 compiler 通常**不用 enter**：它比 `push ebp; mov ebp, esp; sub esp, n` 這三條還慢。leave 則還很常見。§18.3。

---

## 18. 【實測】

本機：Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、NASM。把課本的例子用 NASM 寫成 32 位元的函式，從 C 呼叫（`gcc -m32`）。程式在 `experiments/lecture12/`。

### 18.1 課本的例子都跑得出正確答案

```
$ nasm -f elf32 ex.asm -o ex.o && gcc -m32 -O2 -o main main.c ex.o && ./main
Ex60: ebx = 10, esp moved by 4 bytes
Ex67: -50 / 3 -> quotient -16, remainder -2
Ex68-70: sal 0xeadc0de0  sar 0xfdeadc0d  shr 0x0deadc0d
Ex72: prime(97)=1 prime(91)=0 prime(11)=1
Ex74: 10! = 3628800   Ex78: 10! = 3628800
x86: CF after (0 - 5) = 1
```

- **push dword [esp]**：ebx = 10；push 之後 esp **少了** 4 byte（往低位址），不是影片說的變成 1004。
- **−50 ÷ 3**：商 **−16**、餘數 **−2**（影片口誤說 −3）。
- **0xdeadc0de 的位移**：和課本 Example 68–70 一模一樣。
- **質數**：97、11 是質數，91 = 7 × 13 不是。
- **遞迴階乘**：push／pop 版（Ex74）和 enter／leave 版（Ex78）都得到 10! = 3628800。
- **0 − 5 之後 CF = 1**：x86 借位時 CF = 1，和 ARM（C = 0）相反。

### 18.2 gcc 怎麼做除法：cdq

```
$ gcc -m32 -O2 -S -masm=intel div.c
q:	mov	eax, DWORD PTR 4[esp]
	cdq                           ← edx = eax 的 sign extension
	idiv	DWORD PTR 8[esp]
	ret                           ← 商在 eax
r:	mov	eax, DWORD PTR 4[esp]
	cdq
	idiv	DWORD PTR 8[esp]
	mov	eax, edx                  ← 餘數在 edx
	ret
uq:	mov	eax, DWORD PTR 4[esp]
	xor	edx, edx                  ← 無號：edx = 0
	div	DWORD PTR 8[esp]
	ret
```

- **有號除法**：gcc 用 **`cdq`** 把 eax 的符號位複製到整個 edx，一條指令就做完 §11 說的「正數補 0、負數補全 1」，不用自己判斷正負。
- **無號除法**：用 **div**，edx 一律清成 0（`xor edx, edx` 是清 0 的慣用寫法，比 `mov edx, 0` 短）。
- 參數在 `4[esp]`、`8[esp]`：32 位元的 C 呼叫慣例（cdecl）就是用 stack 傳參數，[esp] 是 return address，和 §16 一樣。

```
$ ./cdiv
C: -50/3=-16  -50%3=-2
```

C 的整數除法和 idiv 一樣：往 0 捨入，餘數和被除數同號。

### 18.3 enter 與 leave

一個有 8 個區域變數的函式（`fr.c`），加 `-fno-omit-frame-pointer` 讓 gcc 用 ebp：

```
$ gcc -m32 -O0 -fno-omit-frame-pointer -S -masm=intel fr.c     （-O2 也一樣）
	push	ebp
	mov	ebp, esp
	sub	esp, 56
	...
	leave
	ret
```

- gcc 不管 `-O0` 還是 `-O2`，開頭都**不用 `enter`**，而是拆成 `push ebp; mov ebp, esp; sub esp, 56` 三條；但結尾**會用 `leave`**。原因是 enter 在現代處理器上用微碼（microcode）實作，比拆開的三條指令慢；leave 則很快。
- 課本 Example 78 用 enter 8, 0，在 §18.1 實測也正確，只是一般 compiler 不這樣寫。

---

## 19. 一頁速查表

| 主題 | 重點 |
|---|---|
| mov | = RISC 的 mov ＋ load ＋ store；不能兩個記憶體 |
| movsx／movzx | 小 → 大；sign extend／zero extend；目的地是暫存器 |
| xchg | 交換；平行程式（lock）用 |
| push／pop | 先讀運算元 → esp ∓ 4 → 寫；硬體知道 stack |
| 大小 | byte 8、**word 16**、dword 32、qword 64（x86 的 word 是 16 位元） |
| add／sub／adc／sbb | 2-address；**x86 借位時 CF = 1**（和 ARM 相反） |
| inc／dec／neg | 1-address |
| cmp | 第一個 − 第二個，只設 flags；第一個不能是立即值 |
| imul | 1 個：edx:eax ← eax × op；2 個：r ← r × op；3 個：r ← op × imm |
| idiv | edx:eax ÷ op → eax 商、edx 餘；**負數要 edx = −1（cdq）** |
| mul／div | 無號版本 |
| 邏輯 | and、or、xor、not |
| 位移 | sar（算術）、shr（邏輯）、sal = shl |
| 分支 | jmp；j ＋ o／no／b／nb／e(z)／ne(nz)／be／s／ns／l／le／g／ge |
| 無號／有號 | below／above vs less／greater |
| call／ret | call：push return address ＋ 跳；ret：pop ＋ 跳 |
| 參數 | 用 stack 時在 [esp+4]（[esp] 是 return address） |
| ebp | frame pointer；開始 ebp ← esp，結束 esp ← ebp；callee saved |
| enter／leave | enter n, 0 = push ebp; mov ebp, esp; sub esp, n；leave = mov esp, ebp; pop ebp |
| 好習慣 | 大函式用 enter … leave ＋ ret；push／pop 一定要配對 |

---

## 20. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| si／di、rep、字串指令、lea | [第 13 支](lecture13_note.md)：x86 進階記憶體指令 |
| 浮點的 fld、fadd… | [第 13](lecture13_note.md)–[14 支](lecture14_note.md)：x86 浮點指令 |
| 指令怎麼編碼成 1～15 byte | [第 14 支](lecture14_note.md)：x86 編碼 |
| RISC-V 的 jal／jalr、用暫存器存 return address | [第 15](lecture15_note.md)–[16 支](lecture16_note.md)：Ch6 RISC-V |
| 乘法、除法的硬體為什麼慢 | [第 21](lecture21_note.md)–[26 支](lecture26_note.md)：課本 Ch8 計算機算術 |
| xchg 與 lock、原子指令 | [第 38](lecture38_note.md)–[40 支](lecture40_note.md)：課本 Ch12 多處理器 |
| 64 位元 Linux 的呼叫慣例（暫存器傳參數） | [第 8 支 §19.3](lecture08_note.md#193-x86-64-linux-的-stack-與呼叫慣例) |

---

## 附：本講一句話總結

> **x86 的 mov 一條就等於 RISC 的 mov、load 和 store（兩個運算元不能都是記憶體），movsx／movzx 做 sign／zero extension，xchg 交換兩個位置，push／pop 讓硬體直接管理 esp（先讀運算元，再改 esp，所以 push dword [esp] 推的是舊頂端的值），記憶體運算元的大小用 byte／word（16 位元！）／dword／qword 標明；ALU 指令都是 2-address（add、sub、adc、sbb、and、or、xor、sar、shr、sal），inc／dec／neg／not 是 1-address，cmp 只設 flags，而 x86 減法借位時 CF = 1，和 ARM 相反。imul 有三種形式（1 個運算元時結果放 edx:eax），idiv 把 edx:eax 除以運算元、商放 eax、餘數放 edx，被除數是負數時 edx 一定要是全 1（gcc 用 cdq；實測 −50 ÷ 3 的商是 −16，影片口誤說 −3）。分支叫 jump：jmp 和 j ＋ 條件碼（無號用 below、有號用 less／greater）。call 把 return address push 到 stack、ret 再 pop 回來，參數用 stack 傳時在 [esp+4]；遞迴階乘可以用 push／pop、sub esp 加 mov、或用 ebp 記住開始時的 esp（ebp 是 callee saved），enter n, 0 和 leave 把這些開頭和結尾的動作各合成一條指令，大函式用 enter … leave ＋ ret 是好習慣（實測四種寫法都算出 10! = 3628800）。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 5: x86 assembly, Part 2/4 錄影逐字稿（YouTube 自動字幕，1:29:15）、Sarangi *Basic Computer Architecture* v3.09 Ch5 §5.3.1–5.3.3（p.185–203）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、NASM）結果。*
