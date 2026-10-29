# Basic Computer Architecture 第十四支影片完整筆記：x86 指令的編碼：prefix、opcode、ModR/M、SIB（Chapter 5, Part 4/4）

> **課程**：Basic Computer Architecture — Chapt 5: x86 assembly, Part 4/4（播放清單第 14 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕一直下載不到）（25:24）＋ 課本 *Basic Computer Architecture* v3.09 **Ch5** §5.5–5.6（p.214–219）
> **章號**：影片 Chapter 5 = 課本 Ch5。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、GNU as／objdump、gcc 11.4）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 把「ModR/M」聽成「mod r by m」、「SIB」聽成「SIP」；[13:46] 附近有一長串重複的「0, 0, 0」，是轉錄的幻覺，內容照課本 Table 5.28 補。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 相容性](#1-相容性)
- [2. 整體格式：1 到 17 byte](#2-整體格式1-到-17-byte)
- [3. Prefix](#3-prefix)
- [4. Opcode 與 d bit](#4-opcode-與-d-bit)
- [5. ModR/M byte](#5-modrm-byte)
- [6. 暫存器編號](#6-暫存器編號)
- [7. 特例一：R/M = 100 → 後面有 SIB](#7-特例一rm--100--後面有-sib)
- [8. SIB byte](#8-sib-byte)
- [9. 特例二：Mod = 00、R/M = 101 → 記憶體直接定址](#9-特例二mod--00rm--101--記憶體直接定址)
- [10. 64 位元的更正](#10-64-位元的更正)
- [11. 例子：add ebx, [edx + ecx*2 + 32]](#11-例子add-ebx-edx--ecx2--32)
- [12. x86 這一章的結尾](#12-x86-這一章的結尾)
- [13. 【原始碼】Linux 的 15 byte 上限](#13-原始碼linux-的-15-byte-上限)
- [14. 【實測】](#14-實測)
- [15. 一頁速查表](#15-一頁速查表)
- [16. 與後續講次的連結](#16-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

x86 這一章的最後一講：x86 指令怎麼變成一串 byte。

```
[Prefix 0–4 byte] [Opcode 1–3 byte] [ModR/M 0–1] [SIB 0–1] [Displacement 0/1/2/4] [Immediate 0/1/2/4]
                                     Mod(2) Reg(3) R/M(3)   Scale(2) Index(3) Base(3)
```

重點是 ModR/M 和 SIB 這兩個 byte，以及兩個特例（R/M = 100、Mod = 00 且 R/M = 101）。

---

## 1. 相容性

[01:09] x86 存在很久了，很重視**相容性**：今天的程式要能在昨天的處理器上跑、昨天的程式要能在今天的處理器上跑。

- 不是 100% 都做得到：例如用了 MMX 暫存器的程式，在更老的處理器上就跑不了。
- 但至少 10 年前能跑的程式，今天的處理器一定要能跑。【口頭】不然是很糟的商業決策。

---

## 2. 整體格式：1 到 17 byte

[01:57] CISC 指令集，**指令長度不固定**，而且差很多。這會讓處理器設計很困難（之後再談）。

課本 Figure 5.4（p.215），影片 [02:23]：

| 欄位 | 大小 | 必要嗎 |
|---|---|---|
| Prefix | 1–4 byte | 選用 |
| Opcode | 1–3 byte | 必要 |
| ModR/M | 1 byte | 選用 |
| SIB | 1 byte | 選用 |
| Displacement（offset） | 1／2／4 byte | 選用 |
| Immediate | 1／2／4 byte | 選用 |

[05:50] 最短 **1 byte**（只有 1 byte 的 opcode）；最長「理論上」4 + 3 + 1 + 1 + 4 + 4 = **17 byte**。

⚠️ 本機實測：同一條指令塞到 **16 byte**，CPU 就拒絕執行；15 byte 可以。Linux 的指令解碼器也寫死 `MAX_INSN_SIZE = 15`。17 是把每個欄位的最大值加起來，實際的上限是 15。見 [§13](#13-原始碼linux-的-15-byte-上限)、[§14.3](#143-最長幾-byte)。

---

## 3. Prefix

[02:43] Prefix 指定指令的一些行為：

| Prefix | 用途 |
|---|---|
| `rep` | 第 13 支看過：重複執行（像迴圈） |
| 資料大小 | 例如在 64 位元指令集裡表示用一般暫存器還是 64 位元暫存器 |
| `lock` | 在多處理器系統裡讓指令 **atomic**（第 11 章，課本 Ch12） |
| 其他 | 很多 ISA 擴充都靠 prefix |

[03:39] **Atomic**：指令好像在一瞬間做完，沒人看得到它做到一半的狀態。【口頭】「其實比這複雜得多」，第 11 章再講。

[04:04] 重點：prefix 這個**欄位**是選用的，但裡面的**資訊**一點都不是可有可無的。

（本機看到的 prefix byte：`lock` = F0、`rep` = F3、16 位元運算元 = 66、64 位元 REX = 48，見 [§14.1](#141-每個欄位長什麼樣)。）

---

## 4. Opcode 與 d bit

[04:27] Opcode 1–3 byte。每種 add、sub、mul、div 都有自己的 opcode。

[08:33] 同一種 add 有兩個方向：

```
add ebx, [eax]      ; 暫存器 ← 暫存器 + 記憶體
add [eax], ebx      ; 記憶體 ← 記憶體 + 暫存器
```

兩條的 ModR/M byte **一樣**。差別在 opcode 裡的 **d bit**（direction）：表示資料往哪邊流。

【課本補充】課本 p.215：opcode 也記錄了運算元的格式，例如 add 的記憶體運算元是第一個還是第二個。（本機：`03` 是「暫存器 ← 記憶體」、`01` 是「記憶體 ← 暫存器」，見 [§14.1](#141-每個欄位長什麼樣)。）

---

## 5. ModR/M byte

[07:08] 一個 byte 分成三個欄位（課本 Figure 5.5，p.215）：

```
 7 6   5 4 3   2 1 0
[Mod] [ Reg ] [ R/M ]
  2      3       3
```

[07:29] **Mod** 決定定址模式（課本 Table 5.28，p.216）：

| Mod | 意思 | 例子 |
|---|---|---|
| **00** | register indirect，**沒有** displacement | `add ebx, [eax]` |
| **01** | register indirect ＋ **1 byte** 有號 displacement | `add ebx, [eax + 8]` |
| **10** | register indirect ＋ **4 byte** 有號 displacement | `add ebx, [eax + 0x1000]` |
| **11** | register direct：**沒有記憶體**，兩個都是暫存器 | `add ebx, ecx` |

- [10:13] 前三種表示有一個運算元是記憶體。
- [10:40] **R/M** 放可能是記憶體的那個運算元（例如 base 暫存器），**Reg** 放另一個，**一定是暫存器**。
- [10:13] 這就是為什麼 x86 一條指令**最多只能有一個記憶體運算元**：bit 不夠。
- [11:20] 只有一個運算元的指令（例如 `inc ecx`、`not edx`），那個運算元放在 R/M（老師叫它 source 1，主要的來源）。

【課本補充】課本 p.216：浮點指令預設的暫存器運算元永遠是 st0；有第二個 FP 暫存器時用 Mod = 11，R/M 放它的編號（st0 = 000、st6 = 110）。

---

## 6. 暫存器編號

[15:44] 課本 Table 5.29（p.216）：

| 暫存器 | 編號 |
|---|---|
| eax | 000 |
| ecx | 001 |
| edx | 010 |
| ebx | 011 |
| esp | 100 |
| ebp | 101 |
| esi | 110 |
| edi | 111 |

8 個暫存器，3 bit。注意順序是 a、c、d、b，不是 a、b、c、d。

---

## 7. 特例一：R/M = 100 → 後面有 SIB

[12:08] 如果是記憶體模式（Mod ≠ 11）而且 **R/M = 100**（本來是 esp 的編號）：

- 不是「用 esp」，而是告訴硬體「**後面還有一個 SIB byte**」。
- R/M 的值不拿來定址，改從 SIB 讀 scale、index、base。
- 依 Mod，後面可能還有 displacement。

---

## 8. SIB byte

[13:15] 給 `[base + index × scale]` 這種位址用（課本 Figure 5.6，p.217）：

```
 7 6     5 4 3    2 1 0
[Scale] [Index] [ Base ]
   2       3        3
```

| Scale | 乘幾 |
|---|---|
| 00 | 1 |
| 01 | 2 |
| 10 | 4 |
| 11 | 8 |

[14:39] 規則：

1. **esp 不能當 index**。【口頭】「誰會想拿 stack pointer 當 index？當 base 還可以理解。」
2. 位址裡的常數要放在 **displacement** 欄位。

---

## 9. 特例二：Mod = 00、R/M = 101 → 記憶體直接定址

[17:04] Mod = 00（沒有 displacement 的 register indirect）而且 R/M = 101（本來是 ebp）：

- 照理說是 `[ebp]`，但**不是**。
- 這個組合被拿來表示**記憶體直接定址（memory direct）**：不用任何暫存器，後面的 32 位元 displacement **就是位址**。

```
add eax, [0x12345678]       ; 直接寫位址
```

[19:01] 學生常問：那 `[ebp]` 要怎麼寫？老師的答案：R/M 設 100（用 SIB），SIB 裡 base = ebp、index = esp。esp 不能當 index，硬體會忽略它，只剩 base = ebp。

⚠️ 本機 GNU as 對 `[ebp]` 選的是另一個做法：**Mod = 01、displacement = 0**（`03 5d 00`，3 byte），等於 `[ebp + 0]`。老師說的 SIB 做法也合法，只是要 3 byte 加上 SIB，一樣長。見 [§14.1](#141-每個欄位長什麼樣)。

[20:24] 【口頭】x86 常要靠這種特例才能把所有想要的東西編進去。禁止 esp 當 index 很合理，因為幾乎沒有理由那樣用。

【課本補充】課本 p.217 的規則還多一條：Mod = 00 時，**SIB 裡的 base 也不能是 ebp**；寫了 ebp 的話，位址只用 scale 和 index 算（加上 displacement）。

⚠️ 課本 p.217 寫「要用 SIB byte，必須把 ModR/M 的 **Mod** 欄位設成 100」，應該是 **R/M** 欄位（Mod 只有 2 bit，不可能是 100；Table 5.28 和影片都說是 R/M = 100）。

---

## 10. 64 位元的更正

[20:24] 老師更正自己前面的話：他說過 64 位元的 displacement 和 immediate 可以更大。**不對**。看最新的 Intel 手冊，x86-64 的 displacement 和 immediate 還是最多 **4 byte**，硬體會把 4 byte 做 sign extension 變成 8 byte。

⚠️ 有一個例外：`mov rax, 0x1122334455667788`（組譯器寫成 `movabs`）可以放 **8 byte** 的 immediate（本機組譯出來是 `48 b8` 加 8 byte，10 byte 長）。一般指令像 `add rax, imm` 就只能 4 byte。見 [§14.2](#142-64-位元模式)。

---

## 11. 例子：add ebx, [edx + ecx*2 + 32]

[21:05] 課本 Example 90（p.217），假設 add 的 opcode 是 0x03：

1. **ModR/M**：
   - displacement 32 放得進 8 bit → **Mod = 01**。
   - 有 scale 和 index，要用 SIB → **R/M = 100**。
   - 目的暫存器 ebx = 011 → **Reg = 011**（放中間）。
   - 01 011 100 = **0x5C**。
2. **SIB**：
   - scale = 2 → 01。
   - index = ecx → 001。
   - base = edx → 010。
   - 01 001 010 = **0x4A**。
3. **Displacement**：32 = **0x20**。

```
03 5C 4A 20
```

4 byte。（本機 `as` 組譯出來一模一樣，見 [§14.1](#141-每個欄位長什麼樣)。）

---

## 12. x86 這一章的結尾

[23:50] x86 有 16、32、64 位元三種，指令集非常大、有很多額外功能。

[24:27] 這一章沒講的一大塊：**向量指令集**，主要是 MMX 和 SSE，第 11 章（課本 Ch12）會講一點，但書裡講得不多，要看網路上或 Intel 手冊。不過 x86 ISA 的核心已經講完了。

[24:59] 下一章（第 6 章，課本 Ch7）：數位電子學的基礎。

【課本補充】課本 Summary 5（p.218–219）回顧整章：16 位元的 8086、80 年代中期改 32 位元、2003 年後高階處理器改 64 位元；暫存器前綴 e（32 位元）、r（64 位元）；2-address 格式、最多一個記憶體運算元；segment 記憶體模型；字串指令和 rep；FP stack；編碼結構很規則。

---

## 13. 【原始碼】Linux 的 15 byte 上限

Linux 有自己的 x86 指令解碼器（給 kprobe、alternative 等用），上限寫死 15 byte。`arch/x86/include/asm/insn.h:105`：

```c
#define MAX_INSN_SIZE	15
```

使用者程式執行超過 15 byte 的指令時，CPU 產生 **#GP**（general protection fault），kernel 把它變成 `SIGSEGV`。`arch/x86/kernel/traps.c:719-725`：

```c
static void gp_user_force_sig_segv(struct pt_regs *regs, int trapnr,
				   unsigned long error_code, const char *str)
{
	current->thread.error_code = error_code;
	current->thread.trap_nr = trapnr;
	show_signal(current, SIGSEGV, "", str, regs, error_code);
	force_sig(SIGSEGV);
}
```

`exc_general_protection`（`traps.c:728` 起）在使用者模式時就呼叫它（`traps.c:758`）。

---

## 14. 【實測】

程式在 `experiments/lecture14/`。用 GNU as 組譯、objdump 看 byte。

### 14.1 每個欄位長什麼樣

`enc32.s`（`as --32`，32 位元模式）：

```
03 5c 4a 20          add ebx,[edx+ecx*2+0x20]     ← 課本 Example 90
01 5c 4a 20          add [edx+ecx*2+0x20],ebx     ← 反方向：opcode 01（d bit 不同），其他都一樣
01 cb                add ebx,ecx                   ← ModR/M CB = 11 001 011：Mod 11、兩個都是暫存器
03 19                add ebx,[ecx]                 ← 19 = 00 011 001：Mod 00
03 59 08             add ebx,[ecx+0x8]             ← 59 = 01 011 001：Mod 01、1 byte disp
03 99 00 10 00 00    add ebx,[ecx+0x1000]          ← 99 = 10 011 001：Mod 10、4 byte disp（little endian）
03 1c 24             add ebx,[esp]                 ← R/M = 100，所以 esp 只能用 SIB（24 = 00 100 100）
03 5d 00             add ebx,[ebp+0x0]             ← [ebp] 變成 Mod 01、disp 0
03 1d 78 56 34 12    add ebx,ds:0x12345678         ← 1D = 00 011 101：記憶體直接定址
41                   inc ecx                       ← 1 byte
f0 01 19             lock add [ecx],ebx            ← lock prefix F0
f3 a5                rep movs [edi],[esi]          ← rep prefix F3
66 01 cb             add bx,cx                     ← 66：運算元改成 16 位元
```

- **Example 90 完全一樣**：`03 5C 4A 20`。
- **d bit**：`03` 和 `01` 只差一個 bit（0000 0011 vs 0000 0001），ModR/M、SIB、displacement 全部一樣。
- **Mod 的四種**：19、59、99、CB 的最高兩 bit 分別是 00、01、10、11。
- **[esp]**：R/M = 100 已經被拿去表示「有 SIB」，所以要多一個 SIB byte `24`：index = 100（esp，表示「沒有 index」）、base = 100（esp）。
- **[ebp]**：Mod 00 + R/M 101 已經被拿去表示「直接位址」，所以 as 改用 Mod 01 加一個 0 的 displacement。
- `add ebx, ecx` 用的是 opcode `01`（Reg = ecx、R/M = ebx），寫成 `03 d9` 也對：暫存器對暫存器有兩種編法，組譯器挑一種。
- 指令長度：1 byte（`inc ecx`）到 6 byte，各不相同。

### 14.2 64 位元模式

`enc64.s`（`as`，64 位元模式）：

```
48 03 5c 4a 20             add rbx,[rdx+rcx*2+0x20]   ← 多了 REX prefix 48（W bit：64 位元運算元）
03 1d 00 01 00 00          add ebx,[rip+0x100]        ← Mod 00、R/M 101 在 64 位元模式變成 rip 相對
03 1c 25 78 56 34 12       add ebx,ds:0x12345678      ← 絕對位址只好改用 SIB（base 101、index 100）
48 b8 88 77 66 55 44 33 22 11   movabs rax,0x1122334455667788   ← 8 byte immediate
48 05 44 33 22 11          add rax,0x11223344         ← 一般指令：4 byte immediate，sign extend
```

- 同一個 Example 90 在 64 位元只多一個 prefix `48`，後面的 byte 完全一樣。這就是 [§1](#1-相容性) 說的相容性：舊的格式原封不動，用 prefix 擴充。
- **特例二在 64 位元模式意思不同**：Mod = 00、R/M = 101 變成「rip + 32 位元 displacement」（rip 相對定址，跟 PC 相對有關的程式碼要靠它）。要真的寫絕對位址，as 改用 SIB 的「沒有 base、沒有 index」組合：`1c 25`。
- **8 byte immediate**：只有 `mov r64, imm64`（`movabs`）有，10 byte 長。`add rax, imm` 最多 4 byte。這是 [§10](#10-64-位元的更正) 老師更正的例外。

### 14.3 最長幾 byte

`maxlen.c`：拿一條 12 byte 的指令

```
48 81 84 D8 44 33 22 11 44 33 22 11     add qword [rax + rbx*8 + 0x11223344], 0x11223344
REX op  ModR/M SIB  ── disp32 ──  ── imm32 ──
```

前面再塞 0–5 個沒有作用的 prefix `2E`（CS segment override，64 位元模式下會被忽略），放進可執行的記憶體裡呼叫它：

```
12 byte：執行成功，buf = 0x11223344
13 byte：執行成功，buf = 0x11223344
14 byte：執行成功，buf = 0x11223344
15 byte：執行成功，buf = 0x11223344
16 byte：SIGSEGV
17 byte：SIGSEGV
```

- 15 byte 以內：CPU 照常執行，`buf` 被加了 0x11223344。
- **16 byte 以上**：CPU 不執行，產生 #GP，Linux 送 `SIGSEGV`（[§13](#13-原始碼linux-的-15-byte-上限)）。
- 所以影片和課本「最多 17 byte」只是把每個欄位的最大值加起來；這顆 CPU 實際的上限是 **15 byte**。

---

## 15. 一頁速查表

| 主題 | 重點 |
|---|---|
| 相容性 | 舊程式要能在新處理器上跑 |
| 格式 | prefix 0–4、opcode 1–3、ModR/M 0–1、SIB 0–1、disp 0/1/2/4、imm 0/1/2/4 |
| 長度 | 最短 1 byte；欄位加起來 17；⚠️ 實際上限 15 |
| Prefix | rep（F3）、lock（F0）、運算元大小（66）、64 位元 REX（48） |
| d bit | opcode 裡；決定「暫存器 ← 記憶體」還是「記憶體 ← 暫存器」（03 vs 01） |
| ModR/M | Mod(2) Reg(3) R/M(3) |
| Mod | 00 無 disp、01 1 byte disp、10 4 byte disp、11 暫存器 |
| 一個記憶體運算元 | R/M 放可能是記憶體的、Reg 一定是暫存器 |
| 暫存器編號 | eax 0、ecx 1、edx 2、ebx 3、esp 4、ebp 5、esi 6、edi 7 |
| R/M = 100 | 後面有 SIB（所以 [esp] 要用 SIB） |
| SIB | Scale(2) Index(3) Base(3)；scale 1/2/4/8；esp 不能當 index |
| Mod 00 ＋ R/M 101 | 32 位元：直接位址；64 位元：rip 相對 |
| [ebp] | 老師：SIB；as：Mod 01 ＋ disp 0 |
| Example 90 | add ebx, [edx + ecx*2 + 32] = 03 5C 4A 20 |
| 64 位元 | disp、imm 最多 4 byte（sign extend）；例外 movabs 有 8 byte imm |
| 實測 | as 組出 03 5C 4A 20；16 byte 的指令 → SIGSEGV（#GP） |

---

## 16. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| x86 暫存器、定址模式、segment | [第 11 支](lecture11_note.md) |
| add、inc、not 這些指令 | [第 12 支](lecture12_note.md) |
| rep、字串指令、x87 | [第 13 支](lecture13_note.md) |
| SimpleRisc、ARM 的固定長度編碼（對照） | [第 8 支](lecture08_note.md)、[第 10 支](lecture10_note.md) |
| RISC-V 的編碼 | [第 17 支](lecture17_note.md) |
| lock prefix、atomic 指令 | [第 40 支](lecture40_note.md) |
| MMX、SSE 向量指令 | [第 41 支](lecture41_note.md) |
| 長度不固定讓 decode 很難（predecode、micro-op） | [Advanced 第 9 支](../advanced/lecture09_note.md) |
| Little endian | [第 11 支](lecture11_note.md) |

---

## 附：本講一句話總結

> **x86 是長度不固定的 CISC 指令集，為了相容性一直用同一套格式擴充：0–4 byte 的 prefix（rep、lock、資料大小、64 位元的 REX）、1–3 byte 的 opcode（含決定資料方向的 d bit）、選用的 ModR/M byte（Mod 2 bit 表示無 disp／1 byte disp／4 byte disp／純暫存器，Reg 放一定是暫存器的那個運算元，R/M 放可能是記憶體的那個，所以一條指令最多一個記憶體運算元）、選用的 SIB byte（scale 1/2/4/8、index、base，esp 不能當 index）、以及 displacement 和 immediate；兩個特例是 R/M = 100 表示「後面有 SIB」、Mod = 00 且 R/M = 101 表示「只有 32 位元位址」，課本 Example 90 的 add ebx, [edx + ecx*2 + 32] 編成 03 5C 4A 20。本機實測：GNU as 組出一模一樣的 byte；64 位元模式只多一個 48 prefix，而且 Mod 00／R/M 101 變成 rip 相對；欄位加起來最多 17 byte，但 CPU 實際上只接受 15 byte，第 16 byte 就產生 #GP。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 5: x86 assembly, Part 4/4 的 whisper 轉錄稿（本機 `large-v3-turbo`，25:24）、Sarangi *Basic Computer Architecture* v3.09 Ch5 §5.5–5.6（p.214–219）、Linux v6.2.12 `arch/x86/include/asm/insn.h`、`arch/x86/kernel/traps.c` 整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、GNU as／objdump、gcc 11.4）結果。*
