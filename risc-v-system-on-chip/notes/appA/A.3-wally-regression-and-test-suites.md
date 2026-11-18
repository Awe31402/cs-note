# A.3 Wally Regression and Test Suites

> 原書 p.784.e109、784.e125–784.e126（Appendix A: Wally Synopsis）

## 這節在講什麼

三張表整理 Wally 的測試：哪個組態跑哪些套件（Table A.7）、每個套件對應哪個擴充和哪一章（Table A.8）、以及一個獨立的浮點 testbench（Table A.9）。

**Table A.8 特別值得看**——它把「指令集擴充」「章節」「測試套件名稱」三者對在一起，是回頭找東西時最快的索引。

## 關鍵點

### 回歸測試怎麼分配（Table A.7）

> "Section 5.11 describes running regression, applying each relevant test suite to each configuration. Table A.7 lists the regression test suites run on each configuration. The riscv-arch-test suite is steadily growing, and the latest list of regression tests can be found in bin/wally-regression."
> — p.784.e109

中文解釋：Table A.7 的重點是**測試量隨組態複雜度暴增**：

| 組態 | 測試套件 |
| --- | --- |
| `rv32e` | `arch32e`（1 個） |
| `rv32i` | `arch32i`（1 個） |
| `rv32imc` | `arch32i`、`arch32c`、`arch32m`、`wally32periph`（4 個） |
| `rv32gc` | 21 個 |
| `rv64i` | `arch64i`（1 個） |
| `rv64gc` | **26 個** |
| `buildroot` | `buildroot`（1 個，但要跑 20 小時） |

`rv64gc` 的清單完整展現了這本書的範圍：`arch64i/c/m/f/d/zfh` + 各自的 `_fma` 與 `_divsqrt` 變體 + `arch64zfa` + `arch64zcb` + `arch64priv` + `arch64a_amo` + `arch64zb*` + `arch64zk*` + `arch64zifencei` + `arch64zicond` + `arch64pmp` + **`ahb64_ram_{0/1/2}_{0/1}_rv64gc`**（第 23.4.1 章那個 burst bug 的回歸測試）+ `wally64a_lrsc` + `wally64priv` + `wally64periph` + `coverage64gc`。

注意 `rv32gc` 有 `arch32vm_sv32`（Sv32 虛擬記憶體），`rv64gc` 沒有對應的——因為 RV64 的虛擬記憶體測試被歸在 `arch64priv` 裡。

### 測試套件的來源分兩種（Table A.8）

> "The test suites used for the regression tests are listed in Table A.8. They are run using testbench.sv in the $RISCV/testbench folder."
> — p.784.e109

中文解釋：Table A.8 分成四欄——RV32 與 RV64 各有 `riscv-arch-test`（官方）和 `wally-riscv-arch-test`（Wally 自製）兩類。

**哪些東西官方沒有測試、要 Wally 自己補？**
- **A 擴充的 lr-sc**（`wally32a_lrsc` / `wally64a_lrsc`）——第 17.3 章講過官方只有 AMO 測試
- **Privileged**（`wally32priv` / `wally64priv`）——特權架構的行為
- **Peripherals**（`wally32periph` / `wally64periph`）——第 20 章的周邊
- **CoreMark / Embench**——第 21 章的 benchmark

**這個分工很合理**：官方測試管「ISA 行為是否符合規格」，自製測試管「規格沒定義或不涵蓋的實作細節」（周邊是平台相關的、特權架構的細節、以及官方還沒補上的部分）。

### 浮點另有專屬 testbench（Table A.9）

> "Another testbench (testbench_fp.sv) exercises floating-point units in isolation for better performance, as listed in Table A.9."
> — p.784.e109

中文解釋：Table A.9 只有一列——`testbench-fp.sv`，第 16.5.3 章，「Applies TestFloat vectors to FPU」。

**「for better performance」是關鍵**。第 16.5 章講過原因：TestFloat 的向量數量是千萬等級（光 `fmadd` 就有 6,133,248 個），走完整的取指-解碼-執行流程太慢。這個 testbench 直接把向量灌進 FPU 模組，一個 cycle 測一個案例。

**這是驗證策略的一個通則**：當某個單元的狀態空間遠大於其他部分時，值得為它做一個繞過管線的專用 testbench。

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| regression test | 回歸測試，改動後重跑確認沒弄壞既有功能 |
| `bin/wally-regression` | 定義各組態要跑哪些套件的腳本 |
| `riscv-arch-test` | RISC-V 官方架構相容性測試 |
| `wally-riscv-arch-test` | Wally 自製、補官方沒涵蓋的測試 |
| `testbench.sv` | 一般測試用的 testbench（跑真正的 RISC-V 程式） |
| `testbench-fp.sv` | 直接把向量灌進 FPU 模組的專用 testbench |
| `ahb64_ram_{0/1/2}_{0/1}` | RAM 延遲 × burst 開關的組合測試 |
| `coverage64gc` | 覆蓋率測試組態 |

## 所以呢

三個實用結論：

1. **測試量隨功能暴增**——從 1 個套件（`rv32i`）到 26 個（`rv64gc`）。這是「加功能的隱藏成本」的另一種量化，和 A.2 的面積數字互補。
2. **官方測試不夠**——lr-sc、特權架構細節、周邊、benchmark 都要自己補。
3. **狀態空間特別大的單元值得專用 testbench**——浮點的千萬級測試向量走不了正常管線。

下一節 A.4 是常用指令速查。
