# RISC-V System-on-Chip Design 讀書筆記

《*RISC-V System-on-Chip Design*》（Harris, Stine, Thompson & Harris，Morgan Kaufmann，1115 頁）的中文讀書筆記。

**167 篇，一小節一篇**，涵蓋正文 23 章、附錄 A/B/C/D/F，以及書末的指令集參考卡。

📖 **原書 PDF**：[Google Drive](https://drive.google.com/file/d/1wn-wSSfJ6YGXWHLN30J2oSlzc8cV9ceb/view?usp=drive_link)（不放進 repo，筆記裡的頁碼都對應這個檔）

---

## 怎麼讀

每篇筆記獨立成篇，照書的順序讀最順，但也可以當參考資料直接跳著查。

三個建議的入口：

| 你想做什麼 | 從哪裡開始 |
| --- | --- |
| 從頭學 RISC-V SoC 設計 | [1.1 Moore's Law and Beyond](notes/ch01/1.1-moores-law-and-beyond.md) 一路往下 |
| 只想懂某個子系統 | 直接看下面目錄，例如快取就跳 [第 10 章](notes/ch10/) |
| 查某條指令的編碼 | [書末指令集參考卡](notes/ref/I-risc-v-instruction-set-summary.md) |

---

## 筆記長什麼樣

每篇固定五個區塊：

| 區塊 | 內容 |
| --- | --- |
| **這節在講什麼** | 3~6 句話交代這節的主題與重點 |
| **關鍵點** | 每個重點都附上**書上英文原文 1~3 句 + 頁碼**，再用中文解釋 |
| **名詞表** | 這節出現的術語對照 |
| **所以呢** | 收尾，通常帶到下一節 |

**每個知識點都可以回查原書。** 原文照抄不改寫，頁碼是書本頁碼（不是 PDF 頁碼）。圖只用文字描述並標出圖號（例如「Fig. 7.3」），不畫 ASCII 圖。

完整的筆記規格在 [`notes/BRIEF.md`](notes/BRIEF.md)。

---

# 目錄

## Part 1 — Introduction：大局觀

從電腦發展史切入，再把 RISC-V 指令集與 Wally 這顆 SoC 的全貌交代清楚。

### Chapter 1: A Brief History of Computer Design

- [1.1 Moore's Law and Beyond](notes/ch01/1.1-moores-law-and-beyond.md)
- [1.2 System-on-Chip](notes/ch01/1.2-system-on-chip.md)
- [1.3 Birth of Computing](notes/ch01/1.3-birth-of-computing.md)
- [1.4 Mainframes and Minicomputers](notes/ch01/1.4-mainframes-and-minicomputers.md)
- [1.5 Microprocessors](notes/ch01/1.5-microprocessors.md)
- [1.6 CISC and RISC](notes/ch01/1.6-cisc-and-risc.md)
- [1.7 RISC-V](notes/ch01/1.7-risc-v.md)
- [1.8 International Economic and Security Competition](notes/ch01/1.8-international-economic-and-security-competition.md)
- [1.9 Summary](notes/ch01/1.9-summary.md)

### Chapter 2: Introduction to RISC-V

- [2.1 RISC-V Assembly Language](notes/ch02/2.1-risc-v-assembly-language.md)
- [2.2 RISC-V Machine Language](notes/ch02/2.2-risc-v-machine-language.md)
- [2.3 Have a Hart](notes/ch02/2.3-have-a-hart.md)
- [2.4 Memory Map](notes/ch02/2.4-memory-map.md)
- [2.5 RISC-V Extensions and Profiles](notes/ch02/2.5-risc-v-extensions-and-profiles.md)
- [2.6 Comparison with Other Architectures](notes/ch02/2.6-comparison-with-other-architectures.md)
- [2.7 RISC-V Microarchitecture](notes/ch02/2.7-risc-v-microarchitecture.md)
- [2.8 Wally System-on-Chip](notes/ch02/2.8-wally-system-on-chip.md)
- [2.9 RISC-V Market](notes/ch02/2.9-risc-v-market.md)
- [2.10 Summary and a Look Ahead](notes/ch02/2.10-summary-and-a-look-ahead.md)

## Part 2 — Tools：工具鏈

編譯器、模擬器、SystemVerilog 寫法、驗證方法、邏輯合成。做晶片之前要先會用的東西。

### Chapter 3: RISC-V Software Tool Flow

- [3.1 Platform Requirements and Tool Overview](notes/ch03/3.1-platform-requirements-and-tool-overview.md)
- [3.2 Getting Started](notes/ch03/3.2-getting-started.md)
- [3.3 GCC Assembler and Spike Simulator](notes/ch03/3.3-gcc-assembler-and-spike-simulator.md)
- [3.4 GCC GNU C Compiler](notes/ch03/3.4-gcc-gnu-c-compiler.md)
- [3.5 Sail Simulation](notes/ch03/3.5-sail-simulation.md)
- [3.6 QEMU Simulation](notes/ch03/3.6-qemu-simulation.md)
- [3.7 Test Suites](notes/ch03/3.7-test-suites.md)
- [3.8 Summary](notes/ch03/3.8-summary.md)

### Chapter 4: HDL Design Practices

- [4.1 HDL Design Methodology](notes/ch04/4.1-hdl-design-methodology.md)
- [4.2 Hardware Design with SystemVerilog](notes/ch04/4.2-hardware-design-with-systemverilog.md)
- [4.3 Wally Style Guidelines](notes/ch04/4.3-wally-style-guidelines.md)
- [4.4 Wally Configuration](notes/ch04/4.4-wally-configuration.md)
- [4.5 Summary](notes/ch04/4.5-summary.md)

### Chapter 5: Design Verification

- [5.1 Verification Strategies](notes/ch05/5.1-verification-strategies.md)
- [5.2 Design Verification Plan](notes/ch05/5.2-design-verification-plan.md)
- [5.3 Lint](notes/ch05/5.3-lint.md)
- [5.4 Testbenches](notes/ch05/5.4-testbenches.md)
- [5.5 Logic Simulators](notes/ch05/5.5-logic-simulators.md)
- [5.6 Lock-Step-Compare](notes/ch05/5.6-lock-step-compare.md)
- [5.7 Assertions](notes/ch05/5.7-assertions.md)
- [5.8 Code Coverage](notes/ch05/5.8-code-coverage.md)
- [5.9 Functional Coverage](notes/ch05/5.9-functional-coverage.md)
- [5.10 Formal Verification](notes/ch05/5.10-formal-verification.md)
- [5.11 Wally Verification](notes/ch05/5.11-wally-verification.md)
- [5.12 Summary](notes/ch05/5.12-summary.md)

### Chapter 6: Logic Synthesis

- [6.1 Performance, Power, and Area (PPA)](notes/ch06/6.1-performance-power-and-area.md)
- [6.2 Fabrication Technologies](notes/ch06/6.2-fabrication-technologies.md)
- [6.3 Running Synthesis](notes/ch06/6.3-running-synthesis.md)
- [6.4 Reviewing Log Files](notes/ch06/6.4-reviewing-log-files.md)
- [6.5 Wally Synthesis Results](notes/ch06/6.5-wally-synthesis-results.md)
- [6.6 Summary](notes/ch06/6.6-summary.md)

## Part 3 — RISC-V Microarchitecture：微架構

本書的主體。五級管線起步，逐章加上特權模式、匯流排、快取、虛擬記憶體、分支預測，以及各個 ISA 擴充。

### Chapter 7: Pipelined Core

- [7.1 Single-Cycle Core](notes/ch07/7.1-single-cycle-core.md)
- [7.2 Pipelining the RV32I Core](notes/ch07/7.2-pipelining-the-rv32i-core.md)
- [7.3 SystemVerilog](notes/ch07/7.3-systemverilog.md)
- [7.4 Test Plan](notes/ch07/7.4-test-plan.md)
- [7.5 Summary](notes/ch07/7.5-summary.md)

### Chapter 8: Privileged Operations

- [8.1 Principles](notes/ch08/8.1-principles.md)
- [8.2 RISC-V Practices](notes/ch08/8.2-risc-v-practices.md)
- [8.3 Test Plan](notes/ch08/8.3-test-plan.md)
- [8.4 Wally Implementation](notes/ch08/8.4-wally-implementation.md)
- [8.5 Summary](notes/ch08/8.5-summary.md)

### Chapter 9: Bus Interface

- [9.1 Principles](notes/ch09/9.1-principles.md)
- [9.2 AMBA Buses](notes/ch09/9.2-amba-buses.md)
- [9.3 Other SoC Buses](notes/ch09/9.3-other-soc-buses.md)
- [9.4 Test Plan](notes/ch09/9.4-test-plan.md)
- [9.5 Wally Implementation](notes/ch09/9.5-wally-implementation.md)
- [9.6 Summary](notes/ch09/9.6-summary.md)

### Chapter 10: Caches

- [10.1 Principles](notes/ch10/10.1-principles.md)
- [10.2 RISC-V Practices](notes/ch10/10.2-risc-v-practices.md)
- [10.3 Test Plan](notes/ch10/10.3-test-plan.md)
- [10.4 Wally Implementation](notes/ch10/10.4-wally-implementation.md)
- [10.5 Summary](notes/ch10/10.5-summary.md)

### Chapter 11: Memory Management Unit

- [11.1 Principles](notes/ch11/11.1-principles.md)
- [11.2 RISC-V Practices](notes/ch11/11.2-risc-v-practices.md)
- [11.3 Test Plan](notes/ch11/11.3-test-plan.md)
- [11.4 Wally Implementation](notes/ch11/11.4-wally-implementation.md)
- [11.5 Summary](notes/ch11/11.5-summary.md)

### Chapter 12: Load/Store Unit

- [12.1 Wally Load/Store Unit Overview](notes/ch12/12.1-wally-load-store-unit-overview.md)
- [12.2 Data Cache Integration](notes/ch12/12.2-data-cache-integration.md)
- [12.3 Memory Management and HPTW Integration](notes/ch12/12.3-memory-management-and-hptw-integration.md)
- [12.4 Stalls, Flushes, and Traps](notes/ch12/12.4-stalls-flushes-and-traps.md)
- [12.5 Examples](notes/ch12/12.5-examples.md)
- [12.6 Test Plan](notes/ch12/12.6-test-plan.md)
- [12.7 Summary](notes/ch12/12.7-summary.md)

### Chapter 13: Instruction Fetch Unit

- [13.1 Branch Prediction Principles](notes/ch13/13.1-branch-prediction-principles.md)
- [13.2 Test Plan](notes/ch13/13.2-test-plan.md)
- [13.3 Wally Implementation](notes/ch13/13.3-wally-implementation.md)
- [13.4 Summary](notes/ch13/13.4-summary.md)

### Chapter 14: Extensions: C (Compressed)

- [14.1 RISC-V Practices](notes/ch14/14.1-risc-v-practices.md)
- [14.2 Test Plan](notes/ch14/14.2-test-plan.md)
- [14.3 Wally Implementation](notes/ch14/14.3-wally-implementation.md)
- [14.4 Summary](notes/ch14/14.4-summary.md)

### Chapter 15: Extensions: M (Multiply and Divide)

- [15.1 Principles](notes/ch15/15.1-principles.md)
- [15.2 RISC-V Practices](notes/ch15/15.2-risc-v-practices.md)
- [15.3 Test Plan](notes/ch15/15.3-test-plan.md)
- [15.4 Wally Implementation](notes/ch15/15.4-wally-implementation.md)
- [15.5 Summary](notes/ch15/15.5-summary.md)

### Chapter 16: Extensions: F/D/Q/Zfh/Zfa (Floating-Point)

- [16.1 Principles](notes/ch16/16.1-principles.md)
- [16.2 Floating-Point Unit Design](notes/ch16/16.2-fpu-design.md)
- [16.3 SoftFloat](notes/ch16/16.3-softfloat.md)
- [16.4 RISC-V Practices](notes/ch16/16.4-risc-v-practices.md)
- [16.5 Test Plan](notes/ch16/16.5-test-plan.md)
- [16.6 Wally Implementation](notes/ch16/16.6-wally-implementation.md)
- [16.7 Summary](notes/ch16/16.7-summary.md)

### Chapter 17: Extensions: A (Atomic)

- [17.1 Principles](notes/ch17/17.1-principles.md)
- [17.2 RISC-V Practices](notes/ch17/17.2-risc-v-practices.md)
- [17.3 Test Plan](notes/ch17/17.3-test-plan.md)
- [17.4 Wally Implementation](notes/ch17/17.4-wally-implementation.md)
- [17.5 Summary](notes/ch17/17.5-summary.md)

### Chapter 18: Extensions: Zb* & Zk* (Bit Manipulation and Cryptography)

- [18.1 Cryptography Principles](notes/ch18/18.1-cryptography-principles.md)
- [18.2 RISC-V Practices](notes/ch18/18.2-risc-v-practices.md)
- [18.3 Test Plan](notes/ch18/18.3-test-plan.md)
- [18.4 Wally Implementation](notes/ch18/18.4-wally-implementation.md)
- [18.5 Summary](notes/ch18/18.5-summary.md)

### Chapter 19: Other Extensions

- [19.1 Cache Management: Zicbom, Zicboz, Zicbop](notes/ch19/19.1-cache-management.md)
- [19.2 Misaligned Loads and Stores: Zicclsm](notes/ch19/19.2-misaligned-loads-and-stores.md)
- [19.3 TLB Management: Svinval](notes/ch19/19.3-tlb-management-svinval.md)
- [19.4 Pause Hint: Zihintpause](notes/ch19/19.4-pause-hint-zihintpause.md)
- [19.5 Conditional Instructions: Zicond](notes/ch19/19.5-conditional-instructions-zicond.md)
- [19.6 Custom Extensions](notes/ch19/19.6-custom-extensions.md)
- [19.7 Summary](notes/ch19/19.7-summary.md)

## Part 4 — Implementation：兜成一台電腦

周邊、效能量測、開機跑 Linux、燒進 FPGA。

### Chapter 20: Peripherals

- [20.1 GPIO](notes/ch20/20.1-gpio.md)
- [20.2 CLINT](notes/ch20/20.2-clint.md)
- [20.3 PLIC](notes/ch20/20.3-plic.md)
- [20.4 UART](notes/ch20/20.4-uart.md)
- [20.5 SPI](notes/ch20/20.5-spi.md)
- [20.6 Summary](notes/ch20/20.6-summary.md)

### Chapter 21: Benchmarking

- [21.1 CoreMark](notes/ch21/21.1-coremark.md)
- [21.2 Embench](notes/ch21/21.2-embench.md)
- [21.3 Performance Verification](notes/ch21/21.3-performance-verification.md)
- [21.4 Summary](notes/ch21/21.4-summary.md)

### Chapter 22: Linux

- [22.1 Principles](notes/ch22/22.1-principles.md)
- [22.2 Buildroot Linux on Wally](notes/ch22/22.2-buildroot-linux-on-wally.md)
- [22.3 Booting Linux](notes/ch22/22.3-booting-linux.md)
- [22.4 Summary](notes/ch22/22.4-summary.md)

### Chapter 23: FPGA Implementation

- [23.1 FPGA Principles](notes/ch23/23.1-fpga-principles.md)
- [23.2 Wally FPGA Implementation](notes/ch23/23.2-wally-fpga-implementation.md)
- [23.3 FPGA Results](notes/ch23/23.3-fpga-results.md)
- [23.4 Interesting Bugs](notes/ch23/23.4-interesting-bugs.md)
- [23.5 Summary](notes/ch23/23.5-summary.md)

## 附錄

附錄 A 是全書速查表；B/C/D 是 Linux、Git、Tcl 入門；F 是第 16 章浮點的完整實作細節。

### Appendix A: Wally Synopsis

- [A.1 Wally Repository](notes/appA/A.1-wally-repository.md)
- [A.2 Wally Configurations](notes/appA/A.2-wally-configurations.md)
- [A.3 Wally Regression and Test Suites](notes/appA/A.3-wally-regression-and-test-suites.md)
- [A.4 Key Wally Commands](notes/appA/A.4-key-wally-commands.md)
- [A.5 Wally Diagrams](notes/appA/A.5-wally-diagrams.md)

### Appendix B: Hitchhiker's Guide to Linux

- [B.1 Connecting to a Linux Server](notes/appB/B.1-connecting-to-a-linux-server.md)
- [B.2 Working With Files](notes/appB/B.2-working-with-files.md)
- [B.3 More Handy Commands](notes/appB/B.3-more-handy-commands.md)
- [B.4 Linux Productivity](notes/appB/B.4-linux-productivity.md)
- [B.5 Scripting and Programming](notes/appB/B.5-scripting-and-programming.md)

### Appendix C: Version Control Using Git

- [C.1 Gitting Started](notes/appC/C.1-gitting-started.md)
- [C.2 Setting Up a Repository](notes/appC/C.2-setting-up-a-repository.md)
- [C.3 Basic Git Flow](notes/appC/C.3-basic-git-flow.md)
- [C.4 Snapshots and HEAD](notes/appC/C.4-snapshots-and-head.md)
- [C.5 Merge Conflicts](notes/appC/C.5-merge-conflicts.md)
- [C.6 Branching and Merging](notes/appC/C.6-branching-and-merging.md)
- [C.7 Tags](notes/appC/C.7-tags.md)
- [C.8 Staging and Undo](notes/appC/C.8-staging-and-undo.md)
- [C.9 Submodules](notes/appC/C.9-submodules.md)
- [C.10 Other Git Capabilities](notes/appC/C.10-other-git-capabilities.md)
- [C.11 Summary](notes/appC/C.11-summary.md)

### Appendix D: Tcl Book of Armaments

- [D.1 Tcl Usage](notes/appD/D.1-tcl-usage.md)
- [D.2 Additional Commands](notes/appD/D.2-additional-commands.md)

### Appendix F: Floating-Point Implementation

- [F.1 Fused Multiply-Add](notes/appF/F.1-fused-multiply-add.md)
- [F.2 Floating-Point Division and Square Root](notes/appF/F.2-floating-point-division-and-square-root.md)
- [F.3 Conversion](notes/appF/F.3-conversion.md)
- [F.4 Postprocessing](notes/appF/F.4-postprocessing.md)

## 參考

- [RISC-V Instruction Set Summary（書末參考卡）](notes/ref/I-risc-v-instruction-set-summary.md) — 指令編碼、暫存器、CSR、頁表格式，27 張表的索引

---

## 幾個讀這本書會撞到的坑

- **本書沒有附錄 E。** 前言明載附錄只有 A–D 和 F。附錄 F 內文的算式編號還殘留舊的 `(E.35)` 之類，是改版沒改乾淨。
- **第 20–23 章與附錄是線上章節**，頁碼格式是 `784.e1` 這種，不是一般頁碼。筆記裡的引用照書標。
- **PDF 頁碼和書本頁碼不一樣**，而且偏移逐章遞減（每章開頭常少一張空白頁）。筆記一律標書本頁碼。
- **第 23 章正文把「Interesting Bugs」誤編為 23.3**（章首目錄作 23.4），Summary 同樣差一號。筆記照目錄編。

## 關於這本書

- 作者：David Harris、James Stine、Rose Thompson、Sarah Harris（Morgan Kaufmann，1115 頁）
- 主角是 **CORE-V Wally**（`cvw`）——OpenHW Foundation 的開源可設定 RISC-V 核心，從 RV32E 微控制器到能開 Linux 的 RV64GC 都涵蓋
- Wally 原始碼：<https://github.com/openhwgroup/cvw>
- 原書 PDF：[Google Drive](https://drive.google.com/file/d/1wn-wSSfJ6YGXWHLN30J2oSlzc8cV9ceb/view?usp=drive_link)

> PDF 有版權，不放進 repo。要對照原文請自行從上面的連結取得。
