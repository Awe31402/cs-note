# A.1 Wally Repository

> 原書 p.784.e106–784.e107（Appendix A: Wally Synopsis）

## 這節在講什麼

附錄 A 是整本書的**速查表**——把散落在 23 章裡的 Wally 設定、記憶體映射、參數、測試套件、方塊圖集中在一起。A.1 只有一句話加一張目錄樹圖，但那張圖值得花時間看：**它是整本書內容的一張地圖**。

## 關鍵點

### 附錄 A 提供什麼

> "This appendix provides a summary of the Wally implementation, including the following: Organization of the Wally repository; Configurations, memory map, configuration parameters, and profiles; Regression test suites; Key microarchitectural diagrams"
> — p.784.e106

中文解釋：四類資訊。書上也說明了為什麼要重複：

> "The microarchitectural diagrams in this appendix are found in the chapters, but they are also duplicated here for convenience."
> — p.784.e106

中文解釋：Fig. A.2–A.32 是各章方塊圖的彙整，**方便對照查閱**。

### 倉庫結構

> "Fig. A.1 lists the overall organization of the CORE-V Wally repository (cvw). The repository is described further in Section 3.2."
> — p.784.e106

中文解釋：Fig. A.1 的目錄樹可以分成五類讀：

**外部相依（`addins/`）**——全部是 git submodule，對應書裡各章用到的工具：
- `berkeley-softfloat-3` / `berkeley-testfloat-3` → 第 16 章的浮點參考模型
- `branch-predictor-simulator` → 第 21.3 章的 `sim_bp`
- `coremark` / `embench-iot` → 第 21 章的兩個 benchmark
- `riscv-arch-test` / `cvw-riscv-arch-test` / `cvw-arch-verif` → 第 13 章的架構測試
- `riscv-dv` → 隨機指令產生器
- `verilog-ethernet` / `vivado-boards` → 第 23 章的 FPGA 支援

**設定（`config/`）**——六個基礎組態：`rv32e`、`rv32i`、`rv32imc`、`rv32gc`、`rv64i`、`rv64gc`，加一個 `shared`。這正是 A.2 那張 Table A.1 的來源。

**RTL 原始碼（`src/`）**——**這個目錄的結構就是整本書的章節順序**：
| 目錄 | 對應章節 |
| --- | --- |
| `ifu` | 第 7、13、14 章（取指、分支預測、壓縮指令） |
| `ieu` | 第 7、18 章（整數執行、位元操作） |
| `mdu` | 第 15 章（乘除法） |
| `fpu` | 第 16 章、附錄 F（浮點） |
| `lsu` | 第 12、17 章（載入儲存、原子操作） |
| `cache` | 第 10 章 |
| `mmu` | 第 11 章（虛擬記憶體、PMA、PMP） |
| `privileged` | 第 8 章（CSR、trap） |
| `hazard` | 第 7 章 |
| `ebu` / `uncore` | 第 9、20 章（匯流排、周邊） |
| `rvvi` | 驗證介面（第 5 章的 lockstep） |
| `generic` | 通用元件（flop、mux、adder） |
| `wally` | 頂層 |

**流程與工具**——`sim/`（五種模擬器各一個目錄）、`synthDC/`（第 6 章的合成）、`fpga/`（第 23 章）、`linux/`（第 22 章）、`benchmarks/`（第 21 章）、`bin/`（安裝與腳本）。

**測試（`tests/`、`testbench/`）**——`riscof`、`wally-riscv-arch-test`、`custom`、`fp`、`coverage`、`breker`。

### 一個實用的觀察

**`src/` 底下沒有 `chapterN` 這種名字，但幾乎每個目錄都能對到一章。** 這不是巧合——這本書的組織方式就是「跟著 RTL 的模組結構走」。所以讀完書之後，倉庫本身就是最好的複習索引：想不起某個模組在講什麼，看目錄名就能回想到章節。

（Wally 的 GitHub 在 github.com/openhwgroup/cvw。）

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| cvw | CORE-V Wally 倉庫名 |
| `addins/` | git submodule 形式的外部相依 |
| `src/` | SystemVerilog RTL 原始碼，按模組分目錄 |
| `synthDC/` | Synopsys Design Compiler 的合成流程（第 6 章） |
| `testbench/` | 模擬用的 testbench 與支援檔 |
| `studies/` | 研究用腳本（如 PPA 掃描） |

## 所以呢

倉庫的目錄結構本身就是這本書的目錄。接下來 A.2 把六個基礎組態、記憶體映射、所有設定參數、以及 RISC-V 擴充與 profile 的對照表整理出來——那是全書最密集的參考資料。
