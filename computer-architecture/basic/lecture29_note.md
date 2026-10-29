# Basic Computer Architecture 第二十九支影片完整筆記：微碼的 branch、call、ret，shared bus 與 vertical／horizontal microprogramming（Chapter 8, Part 3/3 → 課本 Ch9）

> **課程**：Basic Computer Architecture — Chapt 8: Processor Design, Part 3/3（播放清單第 29 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（33:20，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 **Ch9** §9.6.8–9.9（p.396–408）
> **章號**：影片的 Chapter 8 是課本 v3.09 的 **Chapter 9**。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ **這支的字幕很怪**：看起來是先翻成別的語言再翻回英文，很多名詞被換掉了。對照課本，本筆記一律用課本的名詞：

| 字幕裡的字 | 課本的名詞 |
|---|---|
| transition、transition goal | branch、branch target |
| firmware memory | microprogram memory |
| MBQ、M add、MUI | `mbeq`、`madd`、`mmovi` |
| red source、reg val、Regal | `regSrc`、`regVal` |
| transmit／transfer multiplexer | transfer multiplexer |
| switch | switch unit（處理 `mswitch`） |
| ADC | ALU |

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 複習：preamble 與 8 條微指令](#1-複習preamble-與-8-條微指令)
- [2. beq 與 bgt](#2-beq-與-bgt)
- [3. call](#3-call)
- [4. ret](#4-ret)
- [5. 改用 stack 的 call](#5-改用-stack-的-call)
- [6. 微碼的好處：改指令的行為](#6-微碼的好處改指令的行為)
- [7. Shared bus：write bus、read bus、transfer multiplexer](#7-shared-buswrite-busread-bustransfer-multiplexer)
- [8. Vertical microprogramming：45 bits](#8-vertical-microprogramming45-bits)
- [9. Horizontal microprogramming：65 bits](#9-horizontal-microprogramming65-bits)
- [10. 兩種的取捨](#10-兩種的取捨)
- [11. µcontrol unit 的電路](#11-µcontrol-unit-的電路)
- [12. 【原始碼】Linux 的 microcode 更新](#12-原始碼linux-的-microcode-更新)
- [13. 【實測】](#13-實測)
- [14. 一頁速查表](#14-一頁速查表)
- [15. 與後續講次的連結](#15-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

第 8 章（課本 Ch9）的最後一講，接著上一講（[第 28 支](lecture28_note.md)）的 microprogrammed processor：

1. 用微碼寫完剩下的 branch 指令：`beq`、`bgt`、`call`、`ret`，再示範一個改用 stack 的 `call`。
2. 微碼在硬體上怎麼跑：shared bus、µcontrol unit。
3. 微指令怎麼編碼：**vertical**（45 bits，要 decode）和 **horizontal**（65 bits，不用 decode）。

---

## 1. 複習：preamble 與 8 條微指令

這一段影片沒有重講（上一講的內容），但後面全部都會用到。【課本補充】課本 p.390 的 **preamble**（每條指令一開始都要跑的 4 條微指令）：

```
.begin:
mloadIR          把 pc 指的指令讀進 ir
mdecode          等 1 個 cycle，decode 暫存器（rd、rs1、rs2、immx、branchTarget…）填好
madd pc, 4       pc ← pc + 4（每條指令都先加，taken branch 之後再蓋掉）
mswitch          跳到這條指令自己的微碼
```

【課本補充】課本 p.387–389 的 8 條微指令：`mloadIR`、`mdecode`、`mswitch`、`mmov`、`mmovi`、`madd`、`mb`、`mbeq`。名字前面的 `m` 表示 micro，跟一般的程式指令分開。後面可以加 `<args>`，叫某個單元做事，例如 register file 的 `<read>`／`<write>`、記憶體的 `<load>`／`<store>`、ALU 的 `<add>`、`<cmp>`。

---

## 2. beq 與 bgt

[00:35] `beq`：上一次比較的結果是「相等」（flag E = 1）就跳。`bgt`：上一次比較時第一個數比較大（flag GT = 1）就跳。

課本 p.397 的微碼（影片一樣）：

```
mbeq flags.E, 1, .branch     /* flags.E 也是接在 shared bus 上的微暫存器 */
mb   .begin                  /* 不相等：等於 nop，回 preamble 做下一條 */
.branch:
mmov pc, branchTarget        /* 相等：pc ← branch target */
mb   .begin
```

- [01:50] 不跳的時候，這條指令就等於 `nop`。preamble 已經把 pc 加 4 了，直接回 `.begin` 就是下一條。
- [02:12] 跳的時候，branch target **在 decode 時就算好了**，放在 `branchTarget` 微暫存器。不是 branch 的指令，這裡面是垃圾，但 branch 指令裡是對的。
- [02:37] 把它搬進 pc，回到 preamble。這次 preamble 看到的 pc 是 branch target，不是舊的 pc + 4。
- [02:55] `bgt` 一模一樣，只是把 `flags.E` 換成 `flags.GT`。

【課本補充】無條件的 `b` 更簡單（p.397）：`mmov pc, branchTarget` 然後 `mb .begin`。

---

## 3. call

[03:19] `call` 跟 `b` 很像，只多一件事：把**下一條指令的位址**存進 return address 暫存器（r15）。

```
mmov  regData, pc            /* pc 在 preamble 已經是 pc + 4 了 */
mmovi regSrc, 15, <write>    /* 15 = ra；同時叫 register file 寫入 */
mmov  pc, branchTarget       /* 跳到函式 */
mb    .begin
```

- [03:50] preamble 第三行已經把 pc 加 4，所以現在的 pc 就是「舊 pc + 4」，**不用再加**。
- [04:16] 為什麼是 15？因為 r15 就是 return address 暫存器（ra）。
- [04:28] `mmovi` 把 15 放進 `regSrc`，**同時**送出 `<write>`：register file 把 `regData` 寫進 r15。

---

## 4. ret

[05:18] `ret` 反過來：讀 r15，放進 pc。

```
mmovi regSrc, 15, <read>     /* 讀 r15，結果出現在 regVal */
mmov  pc, regVal
mb    .begin
```

---

## 5. 改用 stack 的 call

[06:01] 例子：讓 `call` 把 return address 存到 **stack** 上，不存在 ra。只寫微碼，preamble 不寫（每條指令都一樣）。

[06:31] 要做三件事：讀 stack pointer、把它減 4、把 return address 存進去。課本 Example 132（p.397–398）：

```
mmovi regSrc, 14, <read>       /* r14 = sp；讀出來放在 regVal */
madd  regVal, -4               /* sp - 4：在 stack 上空出一格 */
mmov  mar, regVal              /* mar = 新的 sp（等一下要寫的位址） */
mmov  regData, regVal, <write> /* 寫回 r14；regSrc 還是 14，沒動過 */
mmov  mdr, pc, <store>         /* 把 pc（已經是 pc + 4）存到 mem[mar] */
mmov  pc, branchTarget
mb    .begin
```

- [07:21] 減 4：32 位元系統裡一個 int 是 4 byte，所以往下空出 4 byte。
- [08:49] 寫回時寫到哪個暫存器？寫到 `regSrc` 裡的那個。`regSrc` 一開始放了 14，之後沒改，所以還是 sp。
- [09:52] return address 就在 pc 裡，因為 preamble 已經加過 4 了。

【課本補充】課本 Example 133（p.398）是對應的 stack 版 `ret`：讀 sp → `mmov mar, regVal, <load>` → `mmov pc, ldResult` → `madd regVal, 4` → 寫回 sp。⚠️ 課本 Example 133–136 的程式框標題都印成「stack-based call instruction」，但其實分別是 ret、階乘、Armstrong number、質數，是標題複製錯了。

---

## 6. 微碼的好處：改指令的行為

[11:29] 這個例子說明：用微碼可以做很多最佳化、很多複雜的指令，也可以**改現有指令的行為**。例如把 `call` 改成存在 stack，`ret` 也跟著改，而且很容易。

[12:17] 重點：知道處理器內部的結構，就能加新指令、讓舊指令換一種做法，替處理器加很多很複雜的機制。

【課本補充】課本 p.397：微碼一定比 hardwired 的 data path 慢，但換到很大的彈性。課本還有用一條指令算階乘（Example 134）、判斷 Armstrong number（Example 135）、判斷質數（Example 136）的例子（p.398–401）。

---

## 7. Shared bus：write bus、read bus、transfer multiplexer

[12:43] 到目前為止，shared bus 被當成「一組平行的銅線」。現在看它真正的結構：**兩條 bus**。

| 部分 | 接了誰 | 做什麼 |
|---|---|---|
| **Write bus** | 所有**會輸出**值的微暫存器：decode 暫存器（ir、rs1、rs2、rd…）、pc、register file／ALU／記憶體單元的暫存器 | 把值倒上 bus |
| **Transfer multiplexer** | 三個輸入：write bus、µimm（微指令裡的常數）、µadder 的結果 | 挑一個送到 read bus |
| **Read bus** | 所有**會接收**值的微暫存器：pc、mdr、regVal、ALU 的暫存器… | 把值讀進來 |

- [13:46] 同一時間**只能有一個**暫存器寫到 write bus，所以每個暫存器有自己的 enable 訊號。
- [14:14] 叫 transfer multiplexer，只是因為它把資料從「寫的人」傳給「讀的人」，沒有別的意思。
- [14:29] **µadder**：給 `madd` 用。例如 `madd pc, 4`：pc 倒到 write bus，加上微常數 4，結果是 transfer multiplexer 的第二個輸入。
- [15:26] 常數本身也是一個輸入，給 `mmovi` 用（例如 `mmovi regSrc, 14`）。
- [16:19] 還有一個**相等比較電路**：比較微常數和 bus 上的值，產生 `isMBranch` 訊號，給 `mbeq` 用。

【課本補充】課本 §9.7.1（p.402–403）：寫到 write bus 的暫存器各有一個 `xxx_out` 訊號（例如 `aluResult_out`），從 read bus 讀的各有一個 `xxx_in`（例如 `mar_in`）。pc 兩個都有。一次只能一個寫，但可以好幾個一起讀。

【課本補充】課本 §9.7.2（p.403）：`<args>` 走在 read bus 上，10 bits：**3 bits 單元 ID**（例如 00 ALU、01 register file、10 記憶體）＋ **7 bits opcode**（每個單元最多 128 種操作）；0 表示什麼都不做。影片 [19:27] 也說了 3 ＋ 7，並說這有點多，但留點空間方便以後擴充。

---

## 8. Vertical microprogramming：45 bits

[18:16] 第一種編碼：像一般 RISC 指令那樣分欄位。

| 欄位 | bits | 為什麼 |
|---|---|---|
| type | 3 | 8 種微指令 |
| src | 5 | 微暫存器很多（19 個） |
| dest | 5 | 同上 |
| immediate | 12 | 微常數 |
| branchTarget | 10 | microprogram memory 的位址 |
| args | 10 | 3 bits 單元 ＋ 7 bits 操作 |
| **合計** | **45** | |

- [18:55] Branch target 只要 10 bits：microprogram memory 不大，而且**每一格放一條微指令**，下一條是「加 1」，不是加 4。10 bits = 1024 條微指令，是所有指令的微碼加起來的上限。
- [20:24] 要跑它，需要一個至少 **3 級**的小處理器：抓微指令、**decode**（把 45 bits 拆開、產生控制訊號）、執行。老師說細節在課本，跟上一講的處理器設計重複，不再講。

【課本補充】課本 p.389：SimpleRisc 有 5 bits opcode，最多 32 條指令；假設最壞一條要 20 條微指令，共 640 條，所以 10 bits（1024）夠用。「硬體設計多留一點不是壞事，因為做出來就不能改了。」

---

## 9. Horizontal microprogramming：65 bits

[21:00] 第二種：**把 decode 這一步拿掉**。

[21:11] decode 給我們的是控制訊號。既然每條微指令的控制訊號都算得出來，乾脆把**控制訊號本身**直接放進編碼裡。例如 `madd pc, 4`：

- vertical：分別編「madd」「pc」「4」。
- horizontal：4 照樣編成常數，其他的直接用一串 0 和 1 表示「電路哪些部分要開、read／write bus 怎麼接、transfer multiplexer 選哪個」。

| 欄位 | bits |
|---|---|
| control signals | 33 |
| immediate | 12 |
| branchTarget | 10 |
| args | 10 |
| **合計** | **65** |

[23:14] 沒有 decode，只要 **fetch、execute 兩級**。邏輯比較簡單、比較快。

【課本補充】課本 p.405 解釋 33 是怎麼來的：19 個微暫存器裡，有 9 個只接一條 bus（ir、flags.E、flags.GT、I、rd、rs1、rs2、branchTarget、immx），各 1 個訊號；另外 10 個兩條都接，各 2 個訊號 → 9 ＋ 20 = **29**；再加 transfer multiplexer 2 個、µfetch multiplexer 2 個 → **33**。

---

## 10. 兩種的取捨

[23:32]

| | Horizontal | Vertical |
|---|---|---|
| 指令大小 | ❌ 大（65 bits） | ✅ 小（45 bits） |
| 速度、複雜度 | ✅ 不用 decode，快、簡單 | ❌ 要 decode 電路 |
| 改硬體的彈性 | ❌ 幾乎沒有 | ✅ 有一些 |

[24:02] 老師說 horizontal 更大的問題，是要把**硬體的控制訊號全部露給微碼（firmware）**：

- 【口頭】從安全的角度，露太多了。
- 硬體稍微改一下，控制訊號就變，**微碼就全部不能用**。
- Vertical 只露微暫存器的名字（pc、ir、rs1、rs2…）。硬體內部改了，只要名字不變，decode 階段會產生新的控制訊號，微碼照樣能用。

[26:20] 沒有哪一種一定比較好，要記住這個取捨。

【課本補充】課本 p.403、p.406：horizontal「比較常見，也比較有效率」；多出來的儲存空間在 microprogrammed 處理器裡不是問題。它的缺點是微組譯器（microassembler）會綁死在某一顆處理器上。課本最後的結論（p.407）：微碼很強，能做出很有表現力的指令集，但**不適合**拿來實作高效能處理器裡**所有**的指令（尤其是常用的）。

---

## 11. µcontrol unit 的電路

### 11.1 Vertical

[26:28] 很簡單：

1. **µpc** 去讀 microprogram memory。沒有跳的話就一直加 1。
2. 微指令送到 **decode unit**，產生控制訊號。
3. **execute unit** 用控制訊號操控 data path、shared bus 和 transfer multiplexer。
4. data path 把 opcode 送到 **switch unit**，算出這條指令的微碼在 microprogram memory 的哪裡（給 `mswitch` 用）。
5. **µfetch multiplexer** 三選一：下一個 µpc（＋1）、微 branch target、switch unit 的位址。

[28:44] ⚠️ 老師特別澄清：圖上的 branch target 是**微碼的** branch target（`mb`、`mbeq` 用的），**不是** SimpleRisc 程式的 branch target。他說會在網站上的新版投影片改。

【課本補充】課本 Table 9.9（p.405）的規則：`mswitch` → switch unit 的輸出；`mb` → branch target；`mbeq` → `isMBranch` = 1 時 branch target，否則下一個 µpc；其他 → 下一個 µpc。

### 11.2 Horizontal

[29:26] 幾乎一樣，只是少了 decode。控制訊號已經在微指令裡，直接送到 data path。

[30:04] 圖上有**兩個** multiplexer：

- **M1**：`mbeq` 用。用 `isMBranch` 在「微 branch target」和「下一個 µpc」之間二選一。
- **µfetch multiplexer**：四選一：switch unit、微 branch target（給 `mb`）、M1 的輸出、下一個 µpc。

[32:14] 老師說 M1 理論上可以拿掉，只是那樣控制會比較複雜；這樣畫比較好設計、好理解，沒有別的理由。

[33:06] 下一章：pipelining。

---

## 12. 【原始碼】Linux 的 microcode 更新

本機的 Intel 處理器真的有微碼，而且**可以更新**（修 bug、修安全漏洞）。這呼應影片說的「不改硬體，也能改指令的行為」。Linux 開機早期載入新微碼後會印一行訊息。`arch/x86/kernel/cpu/microcode/intel.c:311-319`：

```c
static void
print_ucode_info(struct ucode_cpu_info *uci, unsigned int date)
{
	pr_info_once("microcode updated early to revision 0x%x, date = %04x-%02x-%02x\n",
		     uci->cpu_sig.rev,
		     date & 0xffff,
		     date >> 24,
		     (date >> 16) & 0xff);
}
```

微碼檔案的表頭（`arch/x86/include/asm/microcode_intel.h:7-19`）有 revision、日期、處理器簽章、checksum、大小：

```c
struct microcode_header_intel {
	unsigned int            hdrver;
	unsigned int            rev;
	unsigned int            date;
	unsigned int            sig;
	unsigned int            cksum;
	unsigned int            ldrver;
	unsigned int            pf;
	unsigned int            datasize;
	unsigned int            totalsize;
	unsigned int            metasize;
	unsigned int            reserved[2];
};
```

⚠️ 表頭後面的微碼內容格式 Intel 沒有公開，跟課本的 45／65-bit 微指令有沒有關係，**待補**。

---

## 13. 【實測】

程式在 `experiments/lecture29/`。

### 13.1 用課本的微碼跑一個小程式

`microsim.py` 照課本做了一個 microprogrammed SimpleRisc：微暫存器、register file、記憶體、preamble、`mswitch`，以及 `<read>`／`<write>`／`<load>`／`<store>`／`<add>`／`<cmp>` 這些 args。`b`、`beq`、`call`、`ret` 和 stack 版的 `call`／`ret` **一字不改**照抄課本 p.397–398；`mov`、`add`、`cmp` 課本這一段沒有，是我照同樣的風格寫的。

測試程式：r1 從 0 數到 3，每次用 `call` 呼叫一個把 r1 加 1 的函式。

```
$ python3 microsim.py
課本 call/ret（存在 r15）：r1 = 3，r15 = 20，sp(r14) = 1000，記憶體 {}
   每條指令幾條微指令（含 4 條 preamble）：addi 10.0，b 6.0，beq 6.2，call 8.0，cmpi 8.0，movi 7.0，ret 7.0；總共 22 條指令、164 條微指令
stack 版 call/ret：r1 = 3，r15 = 0，sp(r14) = 1000，記憶體 {996: 20}
   每條指令幾條微指令（含 4 條 preamble）：addi 10.0，b 6.0，beq 6.2，call 11.0，cmpi 8.0，movi 7.0，ret 10.0；總共 22 條指令、182 條微指令
```

- 兩個版本結果都對：r1 = 3。
- 課本版把 return address 20（`call` 下一條的位址）放在 r15；stack 版放在記憶體 996（= 1000 − 4），r15 沒用到，sp 最後回到 1000。
- `beq` 平均 6.2：3 次不跳要 4 ＋ 2 = 6 條，1 次跳要 4 ＋ 3 = 7 條 → (3×6 ＋ 7) / 4 = 6.25。
- 改成 stack 版，`call` 從 8 條變 11 條、`ret` 從 7 條變 10 條，整個程式多了 11%（164 → 182 條微指令）。**沒有動到任何「硬體」**（模擬器的 data path），只換了微碼。這就是影片 [12:05] 說的彈性。
- 每條 SimpleRisc 指令要 6–11 個微指令，也就是 6–11 個 cycle。這就是課本說「微碼比 hardwired 慢」的原因。

### 13.2 本機也有微碼

```
$ grep microcode /proc/cpuinfo | sort | uniq -c
     20 microcode	: 0x133
$ journalctl -k -b | grep -i microcode
kernel: microcode: Microcode Update Driver: v2.2.
```

- 20 個 hardware thread 的微碼版本都是 **0x133**。
- 這台沒有裝 `/lib/firmware/intel-ucode`，所以開機時 Linux 沒有更新微碼（沒有 §12 那行「updated early」）；0x133 是 BIOS 載入的版本。

---

## 14. 一頁速查表

| 主題 | 重點 |
|---|---|
| Preamble | `mloadIR`、`mdecode`、`madd pc, 4`、`mswitch` |
| beq／bgt | `mbeq flags.E／GT, 1, .branch`；不跳 = nop；跳 = `mmov pc, branchTarget` |
| call | `mmov regData, pc` ＋ `mmovi regSrc, 15, <write>`，再跳；pc 已經 + 4 |
| ret | `mmovi regSrc, 15, <read>` ＋ `mmov pc, regVal` |
| stack 版 call | 讀 sp → −4 → mar → 寫回 sp → `mmov mdr, pc, <store>` → 跳 |
| Shared bus | write bus ＋ transfer multiplexer（write bus／µimm／µadder）＋ read bus |
| 一次 | 只能一個暫存器寫 write bus；可以多個讀 read bus |
| isMBranch | 比較 µimm 和 bus 上的值，給 `mbeq` |
| args | 10 bits = 3 bits 單元 ＋ 7 bits 操作 |
| Vertical | 3 ＋ 5 ＋ 5 ＋ 12 ＋ 10 ＋ 10 = 45 bits；fetch、decode、execute |
| Horizontal | 33 控制訊號 ＋ 12 ＋ 10 ＋ 10 = 65 bits；fetch、execute |
| 33 怎麼來 | 9 個單向 × 1 ＋ 10 個雙向 × 2 ＋ 2 ＋ 2 |
| 取捨 | horizontal：大、快、沒彈性、露出控制訊號；vertical：小、要 decode、硬體可改 |
| µfetch mux | 下一個 µpc／微 branch target／switch unit（horizontal 再加 M1） |
| Branch target | 圖上的是**微碼的** branch target，不是程式的 |
| 實測 | 照抄課本微碼跑通；stack 版多 3 條微指令；每條指令 6–11 個微指令；本機微碼 0x133 |

---

## 15. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Hardwired 處理器、data path、control unit | [第 27 支](lecture27_note.md) |
| Microprogrammed data path、8 條微指令、ALU／ld／st 的微碼 | [第 28 支](lecture28_note.md) |
| call／ret、return address、stack | [第 8 支](lecture08_note.md) |
| Pipelining | [第 30 支](lecture30_note.md) |
| 現代 x86 的 decode、微指令（micro-op） | [Advanced 第 9 支](../advanced/lecture09_note.md)（Ch3 decode） |

---

## 附：本講一句話總結

> **Microprogrammed 處理器裡，每條程式指令都先跑 preamble（`mloadIR`、`mdecode`、`madd pc, 4`、`mswitch`），再跑自己的微碼：`beq`／`bgt` 用 `mbeq` 看 flag，跳就把 `branchTarget` 搬進 pc，不跳就等於 nop；`call` 把已經加過 4 的 pc 寫進 r15 再跳，`ret` 讀 r15 放回 pc；只要換微碼，就能讓 `call` 改把 return address 存到 stack 上，不用改硬體。這些微指令跑在兩條 bus 上：write bus 收所有輸出、transfer multiplexer 在 write bus、微常數和 µadder 之間選一個、read bus 送給所有輸入，`isMBranch` 比較電路給 `mbeq` 用；微指令可以 vertical 編碼（45 bits，要 decode，三級）或 horizontal 編碼（33 個控制訊號直接放進去，65 bits，只要 fetch 和 execute 兩級），horizontal 比較快、比較簡單，但比較大，而且把控制訊號全部露出來，硬體一改微碼就不能用。本機實測：照抄課本的微碼跑通一個小程式，每條指令要 6–11 條微指令，換成 stack 版 call／ret 只多 3 條；這台 Intel 處理器本身的微碼版本是 0x133。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 8: Processor Design, Part 3/3 錄影逐字稿（YouTube 自動字幕，33:20）、Sarangi *Basic Computer Architecture* v3.09 Ch9 §9.6.8–9.9（p.396–408）、Linux v6.2.12 `arch/x86/kernel/cpu/microcode/intel.c`、`arch/x86/include/asm/microcode_intel.h` 整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
