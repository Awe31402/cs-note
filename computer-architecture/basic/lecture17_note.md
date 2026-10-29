# Basic Computer Architecture 第十七支影片完整筆記：RISC-V 的浮點指令與編碼（RISC-V Chapter, Part III/III）

> **課程**：Basic Computer Architecture — RISC-V Chapter: Floating point instructions and encoding (Part III/III)（播放清單第 17 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（50:36，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 Ch6 §6.4–6.6（p.244–257）
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、LLVM 14 的 `llvm-mc`／`ld.lld`、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 浮點暫存器 f0–f31](#1-浮點暫存器-f0f31)
- [2. fcsr：例外旗標與捨入模式](#2-fcsr例外旗標與捨入模式)
- [3. 浮點的 load 與 store](#3-浮點的-load-與-store)
- [4. 載入浮點常數：la ＋ flw](#4-載入浮點常數la--flw)
- [5. 浮點算術指令](#5-浮點算術指令)
- [6. Fused multiply-add](#6-fused-multiply-add)
- [7. 整數與浮點的轉換：fcvt](#7-整數與浮點的轉換fcvt)
- [8. 浮點比較](#8-浮點比較)
- [9. 編碼：R、I、S、U 四種格式](#9-編碼risu-四種格式)
- [10. 編碼：B 與 J 格式](#10-編碼b-與-j-格式)
- [11. 浮點指令的編碼](#11-浮點指令的編碼)
- [12. 【課本補充】Summary 6 裡的幾個小錯](#12-課本補充summary-6-裡的幾個小錯)
- [13. 【實測】](#13-實測)
- [14. 一頁速查表](#14-一頁速查表)
- [15. 與後續講次的連結](#15-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

RISC-V 的最後一講。【口頭】前兩講已經講了 ISA、整數指令、記憶體指令和分支，「我們目前破解過最難的一顆堅果是**遞迴函式呼叫**」，它用到了整數運算、記憶體、函式呼叫、stack 管理。

這一講：

```
浮點：f0–f31（沒有 zero 暫存器）、fcsr（5 個例外旗標 ＋ 捨入模式）
      flw／fsw、la ＋ flw 載入常數、fadd.s fsub.s fmul.s fdiv.s fmin.s fmax.s fsqrt.s
      fmadd.s／fmsub.s（MAC）、fcvt.s.w／fcvt.w.s、flt.s fle.s feq.s → 整數暫存器
編碼：R I S U（算術、記憶體）＋ B J（分支）＋ 浮點的變體
      原則：opcode、rd、rs1、rs2 永遠在同樣的位置
```

---

## 1. 浮點暫存器 f0–f31

[02:00]–[04:48]　課本 §6.4.1（p.244–245）

- 和整數暫存器一樣有 **32 個**：f0～f31。
- **沒有 hardwired 成 0 的浮點暫存器**。整數的 x0 也不能直接拿來當浮點數用（要先轉換）。
  - 【口頭】為什麼不需要？因為把數值載入浮點暫存器不像整數那麼直接。浮點格式比較複雜（1 ＋ 8 ＋ 23、biased exponent、sign bit），**最好先存在記憶體裡再 load**。
- 也有 mnemonic，**應該使用**：

| 暫存器 | Mnemonic | 說明 |
|---|---|---|
| f0–7 | ft0–7 | 暫存（不保存） |
| f8–9 | fs0–1 | saved |
| f10–11 | fa0–1 | 參數／**回傳值** |
| f12–17 | fa2–7 | 參數 |
| f18–27 | fs2–11 | saved |
| f28–31 | ft8–11 | 暫存 |

- 慣例和整數一樣，只是前面加 f。
- 【口頭】只有**三類**（參數／回傳值、saved、temporary），因為不需要 stack pointer、frame pointer、return address 這些特殊暫存器，所以每一類的暫存器比較多。
- 【課本補充】F（單精度）和 D（雙精度）擴充是 **2017 年**加入的。這部分「相當傳統，和其他 RISC ISA 很像」，浮點數用標準的 IEEE 754 格式（[第 5 支](lecture05_note.md)）。

---

## 2. fcsr：例外旗標與捨入模式

[04:48]–[10:32]　課本 Figure 6.1、Table 6.12–6.13（p.245–246）

所有浮點 ISA 都有一個特殊暫存器。RISC-V 叫 **fcsr**（floating point control and status register）。

```
 31          8 7     5 4    3    2    1    0
┌─────────────┬───────┬────┬────┬────┬────┬────┐
│  reserved   │  frm  │ NV │ DZ │ OF │ UF │ NX │
└─────────────┴───────┴────┴────┴────┴────┴────┘
                捨入模式      fflags（accrued exceptions）
```

**fflags**：最低 5 位，記錄**自從上次清除以來**遇過的例外狀況（accrued exception flags）

| 旗標 | 意思 |
|---|---|
| **NV** | invalid operation（無效運算，例如負數開根號） |
| **DZ** | divide by zero |
| **OF** | overflow：結果超出浮點數的範圍 |
| **UF** | underflow |
| **NX** | **inexact**：結果不能精確表示，有做捨入 |

- 【口頭】浮點運算很複雜，不是每個運算都能得到乾淨、甚至正確的值，所以要追蹤這些狀況。

**捨入模式**：

- 【口頭】為什麼要捨入：例如兩個浮點數相加，一個很大、一個很小（例如 2.36 × 2⁻²³），mantissa 只有 23 位，加完後有些位元會「掉出去」，就會損失精度，要決定怎麼捨入。

| 編碼 | Mnemonic | 意思 |
|---|---|---|
| 000 | **RNE** | 捨入到最近的；剛好在中間時，選 **LSB 是偶數**的（預設） |
| 001 | **RTZ** | 往 0 捨入（= 截斷） |
| 010 | **RDN** | 往 −∞（floor） |
| 011 | **RUP** | 往 +∞（ceiling） |
| 100 | **RMM** | 捨入到最近的；剛好在中間時，選**絕對值大**的（遠離 0） |
| 101、110 | — | 保留 |
| 111 | **DYN** | 動態：用 fcsr 的 **frm** 欄位 |

口頭的十進位例子（保留小數點後一位）：

| 模式 | 例子 |
|---|---|
| RNE | 3.46 → 3.5；**3.45 → 3.4**（4 是偶數）；**3.55 → 3.6**（6 是偶數）；3.54 → 3.5；3.56 → 3.6 |
| RTZ | 3.46 → 3.4；−3.46 → −3.4 |
| RMM | 3.45 → 3.5；−3.45 → −3.5（遠離 0，「RTZ 的相反」） |

- **靜態 vs 動態**：指令裡可以直接寫捨入模式（static），也可以寫 DYN，執行時才用 fcsr 的 frm 決定。【口頭】有時候不想在編譯時、甚至程式開始時就決定捨入模式。
- 【課本補充】預設的捨入模式是 **RNE**。§13.2 實測了五種模式。

---

## 3. 浮點的 load 與 store

[10:32]–[11:57]　課本 Table 6.14（p.247）

| 語意 | 例子 | 解釋 |
|---|---|---|
| `flw rd, imm(rs1)` | `flw f1, 48(sp)` | f1 ← mem[48 + sp] |
| `fsw rs2, imm(rs1)` | `fsw f1, 48(sp)` | mem[48 + sp] ← f1 |

- 和整數的 lw／sw 非常像。位址用的是**整數**暫存器（sp）。
- **不做任何轉換**：記憶體裡的 4 byte 原封不動搬進暫存器，因為它本來就是浮點格式；送到 ALU 時才被當成浮點數解讀。
- ⚠️ 投影片上 store 的例子寫成 x1，老師口頭更正：「應該是 **F1**，不是 X1。」

---

## 4. 載入浮點常數：la ＋ flw

[11:57]–[13:41]　課本 Example 115（p.247）

- **沒有**帶浮點立即值的指令：浮點格式很長，每個立即值都要 32 位元，塞不進 12 位元的立即值欄位。
- 做法：把常數**放在記憶體裡、給它一個 label**，用 **la** 取得位址，再用 **flw** 載入。

```
val:	.float 3.14
.main:
	la  a1, val          # a1 = val 的位址
	flw fs1, 0(a1)       # fs1 = 3.14
```

- **la**（load address）是 pseudo instruction（上一講介紹過），把 label 的位址放進整數暫存器。
- 【課本補充】課本說這和 **x86 一樣**：浮點暫存器只能從記憶體載入值。「這部分的 ISA 比整數那邊弱。」但實務上不太影響效能，因為除了 π、e 這些內建常數，很少需要載入浮點立即值。

---

## 5. 浮點算術指令

[13:41]–[17:41]　課本 Table 6.15、Example 116（p.248）

| 語意 | 例子 | 解釋 |
|---|---|---|
| `fadd.s rd, rs1, rs2` | `fadd.s f1, f2, f3` | f1 ← f2 + f3 |
| `fsub.s` | `fsub.s f1, f2, f3` | f1 ← f2 − f3 |
| `fmul.s` | `fmul.s f1, f2, f3` | f1 ← f2 × f3 |
| `fdiv.s` | `fdiv.s f1, f2, f3` | f1 ← f2 ÷ f3 |
| `fmin.s`／`fmax.s` | `fmin.s f1, f2, f3` | 最小／最大 |
| `fsqrt.s rd, rs1` | `fsqrt.s f1, f2` | f1 ← √f2 |

- **`.s`** = single precision（32 位元）；**`.d`** = double precision（64 位元）。
- 格式和整數一樣：`rd, rs1, rs2`。但**沒有立即值的版本**。
- 【口頭】和整數不同，浮點的乘法和除法結果**還是 32 位元**：整數乘法要 64 位元、整數除法有商和餘數，但浮點數「放不下的位元由捨入模式處理，也沒有浮點餘數」。真的放不下才會 overflow／underflow，很少見。
- fsqrt 只有一個來源。

**Example 116**：√(π + e + π × e)，結果放 fa0（回傳值的暫存器）

```
pi:	.float 3.14
e:	.float 2.72
.main:
	la      a1, pi
	flw     fs1, 0(a1)
	la      a2, e
	flw     fs2, 0(a2)
	fadd.s  ft1, fs1, fs2      # pi + e
	fmul.s  ft2, fs1, fs2      # pi * e
	fadd.s  ft3, ft1, ft2      # pi + e + pi * e
	fsqrt.s fa0, ft3           # sqrt(pi + e + pi * e)
```

- 【口頭】「和整數非常像，沒有驚喜。」只是少了 zero 暫存器，「它在整數那邊扮演了很重要的角色」。
- §13.1 實測：fa0 = **3.79484**。

---

## 6. Fused multiply-add

[17:41]–[19:10]　課本 Table 6.16（p.249）

| 語意 | 例子 | 解釋 |
|---|---|---|
| `fmadd.s rd, rs1, rs2, rs3` | `fmadd.s f1, f2, f3, f4` | f1 ← f2 × f3 + f4 |
| `fmsub.s rd, rs1, rs2, rs3` | `fmsub.s f1, f2, f3, f4` | f1 ← f2 × f3 − f4 |

- 有趣的新指令：**三個來源暫存器**。
- 用途：**內積**（dot product）、**矩陣乘法**，以及**現代的神經網路**：一直把兩個數相乘、再累加起來。
- 這種指令叫 **MAC**（multiply and accumulate），在神經網路的世界非常流行。【口頭】「這是給現代工作負載非常強大的運算。」
- 和 ARM 的 mla 是同樣的想法（[第 9 支](lecture09_note.md#6-乘法mulmlasmullumull)），但這裡是浮點數。
- ⚠️ 影片和課本都沒強調「**fused**」的意思：乘完**不先捨入**，加完才**捨入一次**。所以 fmadd 的結果可能和「fmul 再 fadd」（捨入兩次）差一點點，通常更準。

---

## 7. 整數與浮點的轉換：fcvt

[19:10]–[24:03]　課本 Table 6.17、Example 117（p.249–250）

| 語意 | 例子 | 解釋 |
|---|---|---|
| `fcvt.s.w rd, rs1` | `fcvt.s.w f1, x5` | f1 ← (float) x5：**整數 → 浮點** |
| `fcvt.w.s rd, rs1` | `fcvt.w.s x5, f1` | x5 ← (int) f1：**浮點 → 整數** |

- 讀法：`fcvt.目的地.來源`。s = single，w = word（32 位元整數）。
- 很常需要，因為很難把立即值讀進浮點程式。
- 內部要做真正的**型別轉換**：整數和浮點數的表示法完全不同。
- 【口頭】可以用它**造出一個「浮點的 0」**：把整數 0 轉成浮點數。雖然沒有 hardwired 的浮點 0 暫存器，但這樣就有了。
- 例：想要 3.0 而不是 3，就用 fcvt.s.w。

**Example 117**：π × e + 4，結果放 fa0；再轉成整數放 a0

```
pi:	.float 3.14
e:	.float 2.72
.main:
	la      a1, pi             # load pi
	flw     fs1, 0(a1)
	la      a2, e              # load e
	flw     fs2, 0(a2)
	addi    t1, zero, 4        # load 4.0 in a register
	fcvt.s.w ft1, t1
	fmadd.s fa0, fs1, fs2, ft1 # pi * e + 4
	fcvt.w.s a0, fa0           # convert to int
```

- 技巧：4 不能直接用，要變成 **4.0**。先用 addi ＋ zero 把 4 放進**整數**暫存器，再用 fcvt.s.w 轉成浮點數。
- 然後用 **fmadd** 一條指令算完 π × e + 4.0。
- 最後 fcvt.w.s 轉回整數。
- ⚠️ 老師口頭說要把結果改放 ft0（「let's change that to ft0」），課本和投影片放的是 fa0。
- ⚠️ **fcvt.w.s 用哪種捨入？** 組合語言沒寫捨入模式時是 **DYN**，用 fcsr 的 frm（預設 RNE）。π × e + 4 ≈ 12.54，**RNE 得到 13**；但 C 的 `(int)` 是往 0 捨入，會得到 **12**。要和 C 一樣，要寫 `fcvt.w.s a0, fa0, rtz`。§13.1 實測：a0 = 13、加 rtz 的版本 = 12。

---

## 8. 浮點比較

[24:03]–[28:50]　課本 §6.4.5、Table 6.18、Example 118（p.250）

| 語意 | 例子 | 解釋 |
|---|---|---|
| `flt.s rd, rs1, rs2` | `flt.s s1, f2, f3` | f2 < f3 就把 s1 設成 1 |
| `fle.s rd, rs1, rs2` | `fle.s s1, f2, f3` | f2 ≤ f3 就設成 1 |
| `feq.s rd, rs1, rs2` | `feq.s s1, f2, f3` | f2 == f3 就設成 1 |

- **結果放在整數暫存器**，這點很重要。
- 【課本補充】浮點數**不能直接當成條件分支的參數**。比較的結果要存進整數暫存器，再用一般的條件分支和 zero 暫存器比較。

**浮點數的相等很麻煩**（[15:16]、[24:03]）：

- 【口頭】理論上兩個實數除非完全一樣，否則不會相等。例如 4 和 4 + 10⁻¹⁵：如果你的表示法能表示 10⁻¹⁵，它們就不相等；不能的話，它們就相等。
- **正確的做法**：相減、取絕對值、看是不是**小於某個門檻**（epsilon）。小於就當作相等。但這會增加指令數，要斟酌。
- 影片說 feq「是逐位元的比較：第 17 位要和第 17 位一樣、第 1 位和第 1 位一樣……」。
  - ⚠️ 這不完全對。IEEE 754 的相等**不是**逐位元比較：**+0 和 −0 位元不同，但 feq 說相等**；**NaN 和自己位元相同，但 feq 說不相等**。[第 5 支 §14.1](lecture05_note.md#141-ieee-754-的位元長什麼樣) 在 x86 上實測過：`-0 == +0` 是 1、`nan == nan` 是 0；RISC-V 遵守同樣的 IEEE 754 規則。

**Example 118**：先 a0 = 0；如果 e < π，就把 a0 設成 17

```
pi:	.float 3.14
e:	.float 2.72
.main:
	la    a1, pi           # load pi
	flw   fs1, 0(a1)
	la    a2, e            # load e
	flw   fs2, 0(a2)
	add   a0, zero, zero   # a0 = 0
	flt.s t0, fs2, fs1     # compare pi and e
	beq   t0, zero, .end   # if (t0 == 0) jump to .end
	addi  a0, zero, 17     # a0 = 17 because t0 == 1
.end:
```

- `add a0, zero, zero`：「很棒的技巧」，0 ＋ 0 放進 a0。
- e < π 成立，所以 t0 = 1，beq 不跳，a0 被設成 17。
- 【口頭】這樣就做到了：比較浮點數，**依結果做決定**。

---

## 9. 編碼：R、I、S、U 四種格式

[28:50]–[39:36]　課本 §6.5.1、Figure 6.2、Table 6.19（p.251–252）

【口頭】SimpleRisc 的格式少得多；RISC-V「有相當多種」。

```
 31        25 24   20 19   15 14  12 11        7 6      0
┌────────────┬───────┬───────┬──────┬───────────┬────────┐
│  funct7    │  rs2  │  rs1  │funct3│    rd     │ opcode │  R
├────────────┴───────┼───────┼──────┼───────────┼────────┤
│    imm[11:0]       │  rs1  │funct3│    rd     │ opcode │  I
├────────────┬───────┼───────┼──────┼───────────┼────────┤
│ imm[11:5]  │  rs2  │  rs1  │funct3│ imm[4:0]  │ opcode │  S
├────────────┴───────┴───────┴──────┼───────────┼────────┤
│            imm[31:12]             │    rd     │ opcode │  U
└───────────────────────────────────┴───────────┴────────┘
     7          5       5       3         5         7
```

| 格式 | 結構 | 指令 |
|---|---|---|
| **R**（register） | rd, rs1, rs2 | add、sub、mul、div、rem、and、or、xor、sll、srl、sra、slt、sltu |
| **I**（immediate） | rd, rs1, imm／rd, imm(rs1) | addi、andi、ori、xori、slli、srli、srai、slti、sltiu、**lw**、**jalr** |
| **S**（store） | rs2, imm(rs1) | sw |
| **U**（upper） | rd, imm | lui |

- 【口頭】記法：**R** for register、**I** for immediate、**S** for store、**U** for upper immediate。

**R 格式**：

- **opcode 7 位元**：最多 128 個 opcode。但因為 RISC-V 擴充很多，opcode 的資訊**也放在其他欄位**。
- 接著 5 位元的 **rd**（32 個暫存器要 5 位元）。
- **funct3**：opcode 的**擴充器**。例：**add、slt、xor 的 opcode 一樣**（`0110011`），用 funct3 區分。
- rs1、rs2 各 5 位元。
- **funct7**：再一層擴充器。例：**add 和 sub** 的 opcode 和 funct3 都一樣，只有 funct7 不同（`0000000` vs `0100000`）。
  - 【口頭】為什麼 add 和 sub 在最底層才分開？因為它們非常相似：減法就是加上 2's complement。add 和 xor 只是「有點相關」，所以在 funct3 就分開了。
- **為什麼這樣設計**（[33:13]）：建立**階層**。opcode 一樣的指令在處理器裡走同一條路、用同樣的方式處理，**之後**才做區分。
  - opcode：高層的區分
  - funct3：中層
  - funct7：最底層
- 【口頭】這種**階層式管理指令**是 RISC-V 的特徵，也是擴充和群組擴充的目的。

**I 格式**：

- opcode 7、rd 5、funct3、**rs1 5**、**12 位元有號立即值**（就是 R 格式 rs2 ＋ funct7 的位置：7 ＋ 5 = 12）。
- 為什麼 rs1 放在同一個位置？因為 **rs2 可以被立即值取代**，rs1 不動。解碼器在這裡才分辨。【口頭】「不同格式之間**重疊越多越好**。」
- load 和 jalr 也是 I 格式：雖然寫法不同（`lw a0, 8(sp)`），但都是 rd、rs1、imm 三個參數。
  - jalr：有一個 return address 的目的暫存器、一個 base 暫存器和一個立即值。

**S 格式**：

- 【口頭】store 一直是個難題。SimpleRisc 沒有設計新格式，而是挪用既有的格式（[第 8 支 §16](lecture08_note.md#16-ld-與-stst-的例外)）；RISC-V 的 ISA 比較有表現力，**真的設計了一個新格式**，因為 store 有兩個來源暫存器和一個立即值，三個都要放。
- 但「也不算很不一樣」：**不要搬動欄位**，否則解碼器會變複雜。
  - store 沒有目的暫存器，所以把 **rd 的 5 個位元拿來放立即值的低 5 位**。
  - funct3、rs1、rs2 都**留在原位**。
  - 剩下的 7 位（R 格式 funct7 的位置）放立即值的**高 7 位**。

**U 格式**：

- opcode 7、rd 5（lui 要把立即值放進目的暫存器），剩下 **20 位元**全部放立即值。
- 【口頭】「每樣東西都很舒服地塞進去，而且大部分時間我們都**重用欄位**，不改欄位的邊界。」例如本來可以把 rs1、rs2 往右移，但沒有這麼做。

【課本補充】所有立即值在硬體使用前都會 **sign-extend**，除非指令有 u 後綴（unsigned）。opcode 在所有格式都在同樣的位置，所以很容易找到指令的類型；**降低解碼器的複雜度**是 ISA 設計的關鍵目標。

§13.3 實測把每一種格式的指令拆成欄位。

---

## 10. 編碼：B 與 J 格式

[39:36]–[46:15]　課本 §6.5.2、Figure 6.3（p.252–253）

【口頭】「四種格式玩得很開心，那就再加兩種吧。」

```
 31   30      25 24  20 19  15 14 12 11     8   7    6    0
┌────┬──────────┬──────┬──────┬─────┬────────┬────┬────────┐
│[12]│ [10:5]   │ rs2  │ rs1  │ f3  │ [4:1]  │[11]│ opcode │  B
├────┼──────────┴─┬────┼──────┴─────┴────────┴────┼────────┤
│[20]│  [10:1]    │[11]│     [19:12]   │    rd     │ opcode │  J
└────┴────────────┴────┴───────────────┴───────────┴────────┘
```

**B 格式**（條件分支：beq、bne、blt、bge、bltu、bgeu）：

- 直接比較兩個暫存器，成功就跳到 label。label 是 **PC-relative offset**，所以最後變成一個立即值，加到目前的 PC 上。
- 結構：rs1、rs2、imm，**沒有目的暫存器**。
- 【口頭】「指令集的編碼充滿了技巧：看得見的技巧、看不見的技巧，到處都是技巧。」
- **技巧 1**：PC-relative offset 的 **LSB 一定是 0**（假設至少對齊到 2 byte 的邊界），所以**不存 bit 0**。
- **技巧 2**：位元的位置要和**其他格式一致**：
  - **bit 4:1** 放在 S 格式 imm[4:0] 的同一個位置（bit 0 的位置空出來）。
  - 空出來的 bit 0 位置拿來放 **bit 11**。
  - **bit 10:5** 放在高位（和 S 格式、J 格式同一個位置）。
  - 剩下一個位元放 **bit 12**（最高位，也就是符號位）。
- 總共存 12 個位元，加上隱含的 0，是 **13 位元**的立即值：約 **±2¹² byte = ±4 KB**。
  - 【口頭】大概是「一個 page 之內」：往前一個 page、往後一個 page。更遠的話要用間接分支（jalr，可以跳到任意位置）。
- 【課本補充】課本說 offset 的範圍是 −4096 到 4095；因為 LSB 一定是 0，實際能跳的是 −4096 到 4094 的偶數。

**J 格式**（jal）：

- 立即值大得多：**21 位元**（存 20 個、隱含 1 個 0）。
- Return address 存在 rd；rd 是 zero 的話就**不存**（等於普通的跳躍）。
- 位元的順序（從高到低）：**bit 20、bit 10:1、bit 11、bit 19:12**。
  - bit 10:1 放在和 B 格式 bit 10:5 對齊的位置。
  - **bit 20（符號位）放在最高位**：【口頭】「如果是有號數，可以很快拿到立即值的符號位。」
- 範圍約 **±1 MB**。
- 【口頭】B 和 J 格式會在 pipeline 的同一個部分、用類似的方式處理，所以位元位置要有一致性。

⚠️ 為什麼對齊到 **2 byte** 而不是 4 byte？影片和課本只說「limitation of the ISA」。原因是 **C 擴充**：壓縮指令只有 16 位元，所以指令可以從 2 的倍數開始。

---

## 11. 浮點指令的編碼

[46:15]–[50:12]　課本 §6.5.3、Figure 6.4–6.6（p.253–255）

**flw**（I 格式的變體）：

```
┌──────────────┬───────┬───────┬───────┬────────┐
│  imm[11:0]   │  rs1  │ width │  rd   │ opcode │
└──────────────┴───────┴───────┴───────┴────────┘
```

- funct3 的位置改成 **width**：要載入多少資料（4 byte 的 float？8 byte 的 double？）。
- rs1 是**整數**暫存器（存位址）；rd 是**浮點**暫存器。

**fsw**（S 格式的變體）：立即值一樣拆成 5 ＋ 7 位；width 在 funct3 的位置。

**浮點算術**（R 格式的變體）：

```
┌────────┬─────┬───────┬───────┬──────┬───────┬────────┐
│ funct5 │ fmt │  rs2  │  rs1  │  rm  │  rd   │ opcode │
└────────┴─────┴───────┴───────┴──────┴───────┴────────┘
    5       2      5       5      3      5        7
```

- funct3 的位置改成 **rm**（rounding mode）：用動態捨入時就寫在這裡。
- **fmt**：精度（32、64、16、128 位元）。
- **funct5**：區分哪一種浮點運算（整數用 funct3 ＋ funct7，浮點用 funct5）。
- 三個暫存器都是浮點暫存器。
- 【課本補充】fsqrt 沒有第二個來源，rs2 設成 0。fcvt.w.s、fcvt.s.w 和比較指令也用這個格式；比較指令的 rm 欄位放比較條件（EQ、LT、LE）。

**fmadd／fmsub**（R4 格式）：funct5 的位置改放**第三個來源暫存器 rs3**，其他一樣。

**本章結束**（[50:12]）：【口頭】「我們總是很高興看到這張投影片，表示這章結束了。」下一章：數位設計、pipeline 設計等。

---

## 12. 【課本補充】Summary 6 裡的幾個小錯

課本 §6.6.1（p.255–257）的摘要大致和內文一致，但有幾點和內文（以及 RISC-V 的規格）不同：

| 摘要第幾點 | 摘要寫的 | 應該是 |
|---|---|---|
| 7 | saved 暫存器是 **s0 − 12** | **s0–s11**（12 個） |
| 9 | 分支指令用 **20 位元**立即值 | 條件分支是 **12 位元**（＋ 隱含的 0 = 13 位元）；20 位元的是 **jal** 和 lui（內文 §6.5.2 和摘要第 23 點都寫對了） |
| 13 | jal、jalr 把 return address 存在**第一個來源暫存器** | 存在**目的暫存器 rd** |
| 15 | 浮點 saved 暫存器是 **fs0 − 12** | **fs0–fs11** |

---

## 13. 【實測】

本機：Intel i7-13650HX、Ubuntu、kernel 6.5.0、LLVM 14。上一講的 Python 模擬器 `experiments/riscv/rv32sim.py` 加上了 F 擴充的一部分（flw、fsw、fadd.s、fsub.s、fmul.s、fdiv.s、fsqrt.s、fmin／fmax、fmadd.s、fmsub.s、flt／fle／feq、fcvt.w.s 的五種捨入、fcvt.s.w），用 Python 的 `struct` 把每個結果捨入成單精度。`build.sh` 改成先用 `ld.lld` 連結到位址 0（這樣 `la` 和 `.data` 的位址才能解析）。本講的程式在 `experiments/riscv/l17/`。

### 13.1 課本 Example 116、117、118

```
$ ../build.sh fp.s && python3 ../rv32sim.py fp.bin
fp.bin: 20 instructions; t0=1 t1=4 a0=13 a1=4176 a2=4180 a3=12 a4=17 | f1=5.86 f2=8.5408 f3=14.4008 f4=4 f9=3.14 f10=3.79484 f11=12.5408 f18=2.72
```

（f9 = fs1、f18 = fs2、f10 = fa0、f11 = fa1；a1、a2 是 la 算出來的 pi、e 的位址）

- **Example 116**：π + e = 5.86（f1 = ft1）、π × e = 8.5408（f2 = ft2）、和 = 14.4008（f3 = ft3）、**√ = 3.79484**（fa0）。
- **Example 117**：fcvt.s.w 把 4 變成 **4.0**（f4 = ft4）；fmadd.s 算出 π × e + 4 = **12.5408**（fa1）。
  - `fcvt.w.s a0, fa1`（預設 DYN → RNE）→ **a0 = 13**
  - `fcvt.w.s a3, fa1, rtz` → **a3 = 12**，這才和 C 的 `(int)` 一樣
- **Example 118**：e < π，所以 t0 = 1、**a4 = 17**。

### 13.2 五種捨入模式

```
$ ../build.sh rm.s && python3 ../rv32sim.py rm.bin
rm.bin: 15 instructions; s0=2 s1=4 s2=-2 s3=2 s4=-2 s5=-3 s6=-2 s7=3 s8=-3 | f0=2.5 f1=3.5 f2=-2.5
```

| 輸入 | RNE | RTZ | RDN | RUP | RMM |
|---|---|---|---|---|---|
| 2.5 | **2**（s0） | 2（s3） | | | **3**（s7） |
| 3.5 | **4**（s1） | | | | |
| −2.5 | **−2**（s2） | −2（s4） | **−3**（s5） | −2（s6） | **−3**（s8） |

- **RNE**：2.5 → 2、3.5 → 4，都選**偶數**；這就是「ties to even」，平均下來不會一直往上偏。
- **RMM**：2.5 → 3、−2.5 → −3，都選**絕對值大的**。
- **RDN／RUP**：−2.5 往 −∞ 是 −3、往 +∞ 是 −2。
- **RTZ**：直接截斷。

### 13.3 六種格式拆開來看

```
$ python3 decode.py enc.bin '...'
006283b3 add t2,t0,t1        R: funct7=0000000 rs2= 6 rs1= 5 f3=000 rd= 7 op=0110011
406283b3 sub t2,t0,t1        R: funct7=0100000 rs2= 6 rs1= 5 f3=000 rd= 7 op=0110011
0062c3b3 xor t2,t0,t1        R: funct7=0000000 rs2= 6 rs1= 5 f3=100 rd= 7 op=0110011
fed28313 addi t1,t0,-19      I: imm=  -19 rs1= 5 f3=000 rd= 6 op=0010011
00812503 lw a0,8(sp)         I: imm=    8 rs1= 2 f3=010 rd=10 op=0000011
00008067 jalr zero,0(ra)     I: imm=    0 rs1= 1 f3=000 rd= 0 op=1100111
00a12423 sw a0,8(sp)         S: imm[11:5]=0000000 rs2=10 rs1= 2 f3=010 imm[4:0]=01000 op=0100011 → imm=8
000642b7 lui t0,100          U: imm=100 rd= 5 op=0110111
00628463 beq t0,t1,.L2 (+8)  B: [12]=0 [10:5]=000000 rs2= 6 rs1= 5 f3=000 [4:1]=0100 [11]=0 → offset=8
00000013 nop                 I: imm=    0 rs1= 0 f3=000 rd= 0 op=0010011
ff9ff0ef jal ra,.L1 (-8)     J: [20]=1 [10:1]=1111111100 [11]=1 [19:12]=11111111 rd= 1 → offset=-8
0020f053 fadd.s ft0,ft1,ft2  R: funct7=0000000 rs2= 2 rs1= 1 f3=111 rd= 0 op=1010011
2124f543 fmadd.s fa0,fs1,fs2,ft4  R4: rs3= 4 fmt=00 rs2=18 rs1= 9 rm=111 rd=10 op=1000011
0045a007 flw ft0,4(a1)       I: imm=    4 rs1=11 f3=010 rd= 0 op=0000111
```

- **add、sub、xor 的 opcode 都是 `0110011`**。add 和 xor 差在 **funct3**（000 vs 100）；add 和 sub 差在 **funct7**（0000000 vs 0100000）。這就是 §9 的階層。
- **rs1 永遠在第 19–15 位、rs2 在第 24–20 位、rd 在第 11–7 位**：add 的 rs1 = 5（t0）、sw 的 rs1 = 2（sp）、beq 的 rs1 = 5，位置都一樣。
- **sw**：沒有 rd，那 5 個位元放的是立即值的低 5 位 `01000` = 8。
- **lw、jalr、flw** 都是 I 格式，只是 opcode 不同。`ret` 就是 `jalr zero, 0(ra)`；`nop` 就是 `addi zero, zero, 0`。
- **beq**：offset = 8，存的是 [4:1] = `0100`（= 4，左移 1 = 8），bit 0 沒存。
- **jal**：offset = −8，符號位 bit 20 = 1 在最高位。
- **fadd.s** 的 f3（rm）是 `111` = **DYN**：沒寫捨入模式時，assembler 預設用動態捨入。
- **fmadd.s**：rs3 = 4（ft4）放在 funct5 的位置，是 R4 格式。

---

## 14. 一頁速查表

| 主題 | 重點 |
|---|---|
| 浮點暫存器 | f0–f31；沒有 zero；ft（暫存）、fs（saved）、fa（參數，fa0／fa1 回傳） |
| fcsr | fflags（NV DZ OF UF NX）＋ frm（捨入模式） |
| 捨入模式 | RNE（預設，ties to even）、RTZ、RDN、RUP、RMM（ties away）、DYN |
| load／store | flw rd, imm(rs1)；fsw rs2, imm(rs1)；不做轉換 |
| 常數 | 沒有浮點立即值：`la a1, val` ＋ `flw fs1, 0(a1)` |
| 算術 | fadd.s fsub.s fmul.s fdiv.s fmin.s fmax.s fsqrt.s（.d 是雙精度）；結果仍是 32 位元 |
| MAC | fmadd.s rd, rs1, rs2, rs3 = rs1 × rs2 + rs3（只捨入一次）；fmsub.s |
| 轉換 | fcvt.s.w（整數 → 浮點）；fcvt.w.s（浮點 → 整數，預設用 frm；要截斷寫 rtz） |
| 比較 | flt.s／fle.s／feq.s，結果放**整數**暫存器；再用 beq／bne 分支 |
| 相等 | 用 \|a − b\| < ε；feq 不是逐位元（+0 == −0、NaN ≠ NaN） |
| R | funct7 rs2 rs1 funct3 rd opcode |
| I | imm[11:0] rs1 funct3 rd opcode（addi、lw、jalr） |
| S | imm[11:5] rs2 rs1 funct3 imm[4:0] opcode（sw） |
| U | imm[31:12] rd opcode（lui） |
| B | [12] [10:5] rs2 rs1 f3 [4:1] [11] opcode；13 位元，±4 KB |
| J | [20] [10:1] [11] [19:12] rd opcode；21 位元，±1 MB |
| 階層 | opcode → funct3 → funct7；add／xor 差 funct3，add／sub 差 funct7 |
| 原則 | opcode、rd、rs1、rs2 永遠在同樣的位置；重疊越多，解碼器越簡單 |
| FP 編碼 | flw：I（width 取代 funct3）；fsw：S；算術：funct5 fmt rs2 rs1 rm rd op；fmadd：rs3 取代 funct5 |

---

## 15. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| 邏輯閘、多工器、解碼器（從固定位置取出欄位） | [第 18](lecture18_note.md)–[20 支](lecture20_note.md)：課本 Ch7 數位邏輯 |
| 浮點加減乘除的硬體與捨入 | [第 24](lecture24_note.md)–[26 支](lecture26_note.md)：課本 Ch8 §8.4–8.6 |
| 解碼器與指令格式的關係 | [第 27](lecture27_note.md)–[29 支](lecture29_note.md)：課本 Ch9 處理器設計 |
| B、J 在 pipeline 的同一部分處理 | [第 30](lecture30_note.md)–[33 支](lecture33_note.md)：課本 Ch10 pipeline |
| MAC、向量擴充（V）、神經網路 | [第 41 支](lecture41_note.md)：Ch12 向量處理；Advanced 課程 Ch14 機器學習的架構 |
| IEEE 754、+0／−0、NaN | [第 5 支](lecture05_note.md) |

---

## 附：本講一句話總結

> **RISC-V 的 F／D 擴充有 32 個浮點暫存器 f0～f31（ft、fs、fa 三類，沒有 zero 暫存器），fcsr 記錄 NV、DZ、OF、UF、NX 五個例外旗標和捨入模式（RNE 預設、RTZ、RDN、RUP、RMM、DYN；實測 2.5 在 RNE 是 2、在 RMM 是 3）；浮點沒有立即值，常數要放在記憶體用 la ＋ flw 載入，算術指令 fadd.s／fsub.s／fmul.s／fdiv.s／fsqrt.s 和整數版很像，fmadd.s 一條算完乘加（MAC，神經網路必備），fcvt.s.w／fcvt.w.s 在整數和浮點之間轉換（實測課本 Example 117 的 fcvt.w.s 用預設的 RNE 得到 13，C 的截斷要加 rtz 才是 12），比較指令 flt／fle／feq 把結果放在整數暫存器，相等最好用 |a − b| < ε 判斷（feq 也不是逐位元比較）。編碼有 R、I、S、U、B、J 六種格式，opcode 7 位元、funct3／funct7 形成指令的階層（add／xor 差在 funct3、add／sub 差在 funct7），而且 opcode、rd、rs1、rs2 永遠在同樣的位置：S 格式把立即值拆到 rd 的位置，B 格式省略一定是 0 的 bit 0、把 bit 11 塞進那個空位，得到 ±4 KB 的範圍，J 格式得到 ±1 MB；浮點指令是這些格式的變體（rm 取代 funct3、funct5 ＋ fmt、R4 格式放 rs3）。課本 Summary 6 有幾處小錯（s0–12 應為 s0–s11、分支立即值不是 20 位元、return address 存在 rd）。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」RISC-V Chapter: Floating point instructions and encoding (Part III/III) 錄影逐字稿（YouTube 自動字幕，50:36，2024 年錄製）、Sarangi *Basic Computer Architecture* v3.09 Ch6 §6.4–6.6（p.244–257）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、LLVM 14 `llvm-mc`／`ld.lld`、Python 3.10）結果。*
