# RISC-V Instruction Set Summary（書末參考卡）

> 原書 p.815–823（Bibliography 785、Index 799 之後的最後一節）
> 涵蓋 Fig. I.1–I.2 與 Table I.1–I.27

## 這節在講什麼

全書最後九頁是**參考卡**——把 RISC-V 的指令編碼、暫存器、CSR、頁表格式全部濃縮成 27 張表。前面 23 章反覆出現的「見書末 Table I.24」指的就是這裡。

這一節沒有敘述文字可以引用，所以下面不列「關鍵點 + 原文」，改成**一張索引**：想查什麼，翻哪一張表。

## 索引：27 張表在哪裡

### 指令格式與基礎指令（p.815–816）

| 圖表 | 內容 | 對應章節 |
| --- | --- | --- |
| **Fig. I.1** | 七種 32 位元指令格式（R/I/S/B/U/J/R4-Type）的欄位配置 | 第 2 章 |
| **Table I.1** | RV32/64I 整數指令（load/store、算術、邏輯、分支、跳躍） | 第 2、7 章 |
| **Table I.2** | RV64I 額外的整數指令（`ld`、`addw`、`sllw` 等 W-type） | 第 2 章 |

**Fig. I.1 值得先看**——它定義了後面所有表格的欄位名（`funct7`、`rs2`、`rs1`、`funct3`、`rd`、`op`），也解釋了為什麼第 19.5 節說「RISC-V 編碼裡沒空間放第三個來源暫存器」（只有 R4-Type 有 `fs3`，那是給 FMA 用的）。

### 浮點（p.816）

| 表 | 內容 | 對應章節 |
| --- | --- | --- |
| **Table I.3** | RVF/D/Q/Zfh 全部浮點指令，**含五個旗標欄位（NV DZ OF UF NX）** | 第 16 章 |

**這張表的旗標欄位很實用**——一眼就能看出哪些指令會設哪些旗標。例如 `fdiv` 是 `x x x x x`（五個都可能），`fsgnj` 是 `- - - - -`（一個都不會）。

### 暫存器與其他擴充（p.817）

| 表 | 內容 | 對應章節 |
| --- | --- | --- |
| **Table I.4** | 32 個整數暫存器的名稱與用途（`zero`、`ra`、`sp`、`a0-a7`…） | 第 2 章 |
| **Table I.5** | RVM 乘除指令 | 第 15 章 |
| **Fig. I.2** | 九種 16 位元壓縮指令格式（CR/CI/CSS/CIW/CL/CS/CA/CB/CJ） | 第 14 章 |
| **Table I.6** | RVC/Zca 壓縮整數指令與它們的 32 位元等價形式 | 第 14 章 |

### 偽指令與特權（p.818）

| 表 | 內容 | 對應章節 |
| --- | --- | --- |
| **Table I.7** | 常用偽指令（`li`、`mv`、`nop`、`ret`、`call`、`la`…）與它們展開成什麼 | 第 2 章 |
| **Table I.8** | 特權/CSR/fence 指令（`ecall`、`mret`、`sret`、`wfi`、`fence`、`csrrw`…） | 第 8 章 |

**Table I.7 是讀組語時最常翻的一張**——它說明 `nop` 其實是 `addi x0, x0, 0`、`ret` 是 `jalr x0, 0(ra)`、`mv` 是 `addi rd, rs1, 0`。

### 特權架構（p.819–820）

| 表 | 內容 | 對應章節 |
| --- | --- | --- |
| **Table I.9** | 所有 CSR 的名稱、位址、大小、讀寫權限 | 第 8 章 |
| **Table I.10** | CSR 的位元欄位配置（`mstatus`、`misa`、`menvcfg`、`fcsr`、`mie`/`mip`…） | 第 8、16、19 章 |
| **Table I.11** | 上表所有縮寫的完整意義（SD、TSR、MXR、SUM、MPRV、FIOM、CBZE…） | 第 8、19 章 |
| **Table I.12** | `misa` 的位元對應（bit 0 = A、1 = B、2 = C、3 = D…） | 第 8 章 |
| **Table I.13** | trap 的 cause 編碼（中斷與例外各 16 種） | 第 8 章 |

**Table I.11 特別有用**——它把 CSR 裡那些神秘的縮寫展開，而且用粗體標出縮寫的來源字母（**S**tate **D**irty、**T**rap **SR**ET、**M**ake e**X**ecutable **R**eadable…）。第 8 章讀不懂某個位元時翻這裡最快。

**Table I.13 的 cause 表**驗證了第 17 章說的「AMO 的例外歸類為 store」——編碼 6、7、15 都寫的是「Store/AMO」。

### 虛擬記憶體（p.820）

| 表 | 內容 | 對應章節 |
| --- | --- | --- |
| **Table I.14** | `satp` 的欄位（Mode、ASID、PPN） | 第 11 章 |
| **Table I.15** | Sv32/39/48/57 的虛擬位址切分 | 第 11 章 |
| **Table I.16** | XWR 編碼（`000` = 指向下一層頁表、`010` = 唯讀…） | 第 11 章 |
| **Table I.17** | Sv32/39/48/57 的實體位址切分 | 第 11 章 |
| **Table I.18** | 頁表項（PTE）的完整位元配置，含 NAPOT、PBMT、RSW、D、A、G、U、X、W、R、V | 第 11、19 章 |

### PMP（p.821）

| 表 | 內容 | 對應章節 |
| --- | --- | --- |
| **Table I.19** | `pmpaddr0-63` 的位元配置 | 第 11 章 |
| **Table I.20** | `pmpcfg0-15` 的位元配置（Locked、Alignment = OFF/TOR/NA4/NAPOT、X/W/R） | 第 11、22 章 |

**Table I.20 解釋了第 22.2.6.3 節那個 `pmpcfg0 = 0x1f18`**——`0x18` 是 region 0（NAPOT、權限全 0），`0x1f` 是 region 1（NAPOT、XWR 全開）。

### 後續擴充（p.821–823）

| 表 | 內容 | 對應章節 |
| --- | --- | --- |
| **Table I.21** | Zicbom/Zicboz/Zicbop 的 CMO 指令 | 第 19.1 章 |
| **Table I.22** | Zcb/Zcf/Zcd 額外的壓縮指令 | 第 14 章 |
| **Table I.23** | Zba/Zbb/Zbc/Zbs 全部位元操作指令 | 第 18 章 |
| **Table I.24** | A 擴充的 `lr`/`sc` 與九個 AMO 指令 | 第 17 章 |
| **Table I.25** | Zicond 的 `czero.eqz`/`czero.nez` | 第 19.5 章 |
| **Table I.26** | Zfa 的額外浮點指令 | 第 16.4.5 章 |
| **Table I.27** | Zbk*/Zkn* 全部密碼學指令 | 第 18 章 |

**Table I.27 值得細看**——它的 Operation 欄位直接給出每條密碼學指令的語意，例如：
- `aes64esm rd, rs1, rs2` → `rd = mixcols(sbox(shiftrows(rs2, rs1)))`
- `sha256sig0 rd, rs1` → `rd = (rs1 ror 7) ^ (rs1 ror 18) ^ (rs1 >> 3)`

這正好對應第 18.2.7 與 18.2.8 節的說明，而且比文字精確得多。

**Table I.23 的兩個註腳也值得注意**：
- `*` 表示 RV32 與 RV64 的立即值欄位寬度不同（5 位元 vs 6 位元）
- 第二個註腳說 `rev8` 和 `zext.h` 其實是更通用的 GREV、PACK、PACKW 指令的特例，**未來的位元操作規格可能會加入完整版**

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| R/I/S/B/U/J/R4-Type | 七種 32 位元指令格式 |
| CR/CI/CSS/CIW/CL/CS/CA/CB/CJ | 九種 16 位元壓縮指令格式 |
| `SignExt` / `ZeroExt` | 符號擴展／零擴展 |
| BTA / JTA | branch target address ／ jump target address |
| `uimm` / `upimm` | 無號立即值／32 位元值的高 20 位元 |
| pseudoinstruction | 偽指令，組譯器展開成一或多條真實指令 |
| MRW / SRW / URW / MRO / URO | CSR 的讀寫權限（Machine/Supervisor/User、Read-Write/Read-Only） |
| `h` address (RV32) | RV32 上 64 位元 CSR 的高 32 位元所在的伴隨位址 |
| XWR | 頁表項的 execute/write/read 權限位元 |
| TOR / NA4 / NAPOT | PMP 的三種位址對齊模式 |
| `bs` / `rnum` | AES 指令的 2 位元 byte select ／ 4 位元 round number |

## 所以呢

這九頁是整本書的濃縮版參考卡。實務上的三種用法：

1. **讀組語或反組譯輸出時** → Table I.1、I.2、I.7（偽指令展開）
2. **除錯特權相關問題時** → Table I.9（CSR 位址）、I.10/I.11（位元欄位與意義）、I.13（cause 編碼）
3. **實作或驗證某個擴充時** → Table I.21–I.27（那個擴充的完整編碼與語意）

到這裡，《RISC-V System-on-Chip Design》的全書筆記完成——正文第 1–23 章、附錄 A/B/C/D/F，以及這份書末參考卡。
