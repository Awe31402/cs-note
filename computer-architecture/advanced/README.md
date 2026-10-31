# Advanced Computer Architecture（Smruti R. Sarangi）隨堂筆記索引

YouTube 播放清單「Advanced Computer Architecture -- Smruti R. Sarangi」的隨堂筆記。課本是 `../../books/computer-arch/nextgen-comp-arch.pdf`（*Next-Gen Computer Architecture: Till the End of Silicon* v3.2），影片章號 = 課本章號。規則見 [BRIEF.md](BRIEF.md)。先讀 [Basic 課程](../basic/README.md)。

第 1 支只是網址連結，不寫筆記；第 2 支和 Basic 第 47 支是同一支影片。

| # | 筆記 | 課本 | 長度 | 原影片標題 |
|---|---|---|---|---|
| 1 | （只有網址連結，不寫筆記） | — | 47 | Link to the Homepage of Advanced Computer Architecture |
| 2 | [計算機結構的過去、現在與未來](lecture02_note.md) | — | 1:04:12 | Computer Architecture: Past, Present, and Future |
| 3 | [導論：為什麼要 out-of-order、multicore 與這本書的地圖](lecture03_note.md) | Ch1 | 24:12 | Chapter 1: Introduction |
| 4 | [In-order pipeline 的快速複習：五級 pipeline、hazard、interlock、forwarding](lecture04_note.md) | Ch2 | 58:14 | Chapter 2: Out-of-order Pipelines: Part I/III (Overview) |
| 5 | [效能模型：performance equation、頻率的極限、multi-issue 與 OOO 的動機](lecture05_note.md) | Ch2 | 1:08:07 | Chapter 2: Out-of-order Pipelines: Part II/ III  (Performance Models) |
| 6 | [相依的種類、register renaming 與 precise exception](lecture06_note.md) | Ch2 | 52:01 | Chapter 2: Out-of-order Pipelines: Part III/III (Hazards and Exceptions) |
| 7 | [Fetch 邏輯：一次抓多條、multi-way branch 與 IST](lecture07_note.md) | Ch3 | 42:36 | Chapter 3: The Fetch and Decode Stages - Part I/III (fetch logic) |
| 8 | [Branch prediction：bimodal、saturating counter、GAg／GAp／PAg／PAp、gshare、tournament、BTB 與 RAS](lecture08_note.md) | Ch3 | 1:20:44 | Chapter 3: The Fetch and Decode Stages: Part II / III (Branch prediction) |
| 9 | [Decode 時的最佳化：CISC → micro-op、predecode、stack pointer、指令壓縮](lecture09_note.md) | Ch3 | 45:04 | Chapter 3: The fetch and decode stages: Part III/III (Decode time optimizations) |
| 10 | [Register renaming 的硬體：physical register file、RAT、free list、dependence check logic](lecture10_note.md) | Ch4 | 54:23 | Chapter 4:  The Issue, Execute, and Commit Stages - Part 1/4 (Instruction renaming) |
| 11 | [Dispatch、wakeup、select、broadcast 與 early broadcast](lecture11_note.md) | Ch4 | 1:01:40 | Chapter 4: The Issue, Execute and Commit Stages - Part 2/4 (Wakeup, Select, and Broadcast) |
| 12 | [Bypass、漏掉 broadcast 與 load-store queue](lecture12_note.md) | Ch4 | 1:01:29 | Chapter 4: The Issue, Execute, and Commit Stages - Part 3/4 (double broadcast, the load-store queue) |
| 13 | [Instruction commit、ROB 與四種狀態回復](lecture13_note.md) | Ch4 | 1:16:43 | Chapter 4: The issue, execute, and commit stages - part 4/4 (Instruction commit) |
| 14 | [Aggressive speculation：位址、相依、延遲與值的預測](lecture14_note.md) | Ch5 | 56:03 | Chapter 5: Alternative Approaches to Issue and Commit - Part I (Aggressive Speculation) |
| 15 | [Instruction replay：forward slice、non-selective、delayed selective 與 token](lecture15_note.md) | Ch5 | 1:14:23 | Chapter 5: Alternative Approaches to Issue and Commit (Part II) (Instruction Replay) |
| 16 | [沒有 PRF 的簡單 OOO、compiler 最佳化與 software pipelining](lecture16_note.md) | Ch5 | 1:30:46 | Chapter 5: Alternative Approaches to Issue and Commit (Part III) -- Compiler Techniques |
| 17 | [VLIW、EPIC 與 Intel Itanium](lecture17_note.md) | Ch5 | 59:26 | Chapter 5: Alternative Approaches to Issue and Commit (Part IV) --  VLIW and EPIC Processors |
| 18 | [ASIC、FPGA 與傳統的 graphics pipeline](lecture18_note.md) | Ch6 | 56:20 | Chapter 6: GPUs (Part I) -- Traditional Graphics Pipeline |
| 19 | [CUDA：kernel、thread／block／grid、記憶體、stream、graph 與 event](lecture19_note.md) | Ch6 | 55:31 | Chapter 6: GPUs (Part II) -- CUDA Programming Language |
| 20 | [GPGPU 的設計：GPC、SM、warp、SIMT 與 register file](lecture20_note.md) | Ch6 | 51:48 | Chapter 6: GPUs (Part III) -- Design of GPGPUs |
| 21 | [Cache 的基本設計：locality、direct mapped、fully／set associative、寫入與替換](lecture21_note.md) | Ch7 | 1:01:25 | Chapter 7: Caches (Part I) --  Cache design |
| 22 | [AMAT、三個 C、簡單的 cache 最佳化與 virtual memory](lecture22_note.md) | Ch7 | 50:58 | Chapter 7: Caches (Part II) - Optimizations and Virtual Memory |
| 23 | [SRAM 與 CAM 陣列：6T cell、precharge、sense amplifier、10T CAM cell](lecture23_note.md) | Ch7 | 46:05 | Chapter 7: Caches (Part III) -- SRAM and CAM arrays |
| 24 | [CACTI、多 port 與 bank、array 的階層與 Elmore delay model](lecture24_note.md) | Ch7 | 1:15:38 | Chapter 7: Caches (Part IV) -- Cacti, Cache Design, Elmore Delay |
| 25 | [進階的 cache 設計：pipelined、non-blocking（MSHR）、skewed associative、way prediction、loop tiling、VIPT](lecture25_note.md) | Ch7 | 56:00 | Chapter 7: Caches (Part V) -- Advanced Cache Optimizations |
| 26 | [Trace cache、instruction prefetching、data prefetching 與 runahead execution](lecture26_note.md) | Ch7 | 1:10:49 | Chapter 7: Caches (Part VI/VI) -- Trace caches, instruction and data prefetching |
| 27 | [On-chip network（一）：節點、router、buffered wire、bisection bandwidth、diameter 與各種 topology](lecture27_note.md) | Ch8 | 59:49 | Chapter 8: The On-chip Network (Part I/V) -- Network Topologies and Basic Concepts |
| 28 | [On-chip network（二）：flit 與 phit、credit 與 on/off flow control、circuit／packet／wormhole／virtual channel](lecture28_note.md) | Ch8 | 1:03:54 | Chapter 8: The On-chip Network (Part II/V) -- Flow Control and Flit/Packet Level Switching |
| 29 | [On-chip network（三）：routing、deadlock、turn model 與 router 的 5 級 pipeline](lecture29_note.md) | Ch8 | 1:07:49 | Chapter 8: The On-chip Network (Part III/V) -- Routing Algorithms and 5-Stage Router Pipeline |
| 30 | [On-chip network（四）：switch 的設計、arbiter、allocator 與 router pipeline 的最佳化](lecture30_note.md) | Ch8 | 1:03:40 | Chapter 8: The On-chip Network (Part IV/V) -- Arbiters and Allocators, Pipeline Optimizations |
| 31 | [On-chip network（五）：NUCA 與 synthetic traffic](lecture31_note.md) | Ch8 | 38:28 | Chapter 8: The On-chip Network (Part V/V) -- Non-Uniform Caches, Synthetic Traffic |
| 32 | [Multicore（一）：平行程式、Amdahl 與 Gustafson、Flynn 分類與 hardware thread](lecture32_note.md) | Ch9 | 1:18:12 | Chapter 9:  Multicore Systems(Part I) -- Parallel Programming, Hardware Threads |
| 33 | [Multicore（二）：distributed cache、coherence 與 consistency、legal sequential execution](lecture33_note.md) | Ch9 | 1:13:50 | Chapter 9: Multicore Systems (Part II) -- Overview of coherence and consistency |
| 34 | [Multicore（三）：sequential consistency、PLSC、coherence 的兩個公理與 fence](lecture34_note.md) | Ch9 | 1:11:55 | Chapter 9: Multicore Systems (Part III) -- Sequential consistency, PLSC, Coherence |
| 35 | [Multicore（四）：execution witness、access graph、causal graph 與 thin air read](lecture35_note.md) | Ch9 | 1:01:30 | Chapter 9: Multicore Systems (Part IV) -- Execution Witnesses |
| 36 | [Multicore（五）：snoopy 協定（write-update、MSI、MESI、MOESI）與 directory 協定](lecture36_note.md) | Ch9 | 1:05:28 | Chapter 9: Multicore Systems (Part V) -- Coherence protocols, Snoopy and Directory based schemes |
| 37 | [Multicore（六）：directory 的優化、false sharing、atomic 指令、spin lock 與 lock-free](lecture37_note.md) | Ch9 | 59:55 | Chapter 9: Multicore Systems (Part VI) -- Optimized directories and atomic instructions |
| 38 | [Multicore（七）：memory model 的分類與 data race](lecture38_note.md) | Ch9 | 1:16:37 | Chapter 9: Multicore Systems (Part VII) -- Memory Models and Data Races |
| 39 | [Multicore（八）：data race 偵測：lock set 演算法與 vector clock](lecture39_note.md) | Ch9 | 46:37 | Chapter 9: Multicore Systems (Part VIII) -- Data Race Detection Algorithms |
| 40 | [Multicore（九）：transactional memory：ACID、版本管理、Bartok、TL2 與硬體 TM](lecture40_note.md) | Ch9 | 1:21:54 | Chapter 9: Multicore Systems (Part IX) -- Transactional Memory |
| 41 | [主記憶體（一）：DRAM cell、bit line 架構、sense amplifier 與 channel／DIMM／rank／bank](lecture41_note.md) | Ch10 | 1:01:50 | Chapter 10: Main Memory (DRAM devices, arrays and architectures) -- Part I |
| 42 | [主記憶體（二）：非同步、同步、DDR 的協定，prefetch length 與 buffered DIMM](lecture42_note.md) | Ch10 | 56:15 | Chapter 10: Main Memory (asynchronous, synchronous and buffered communication) -- Part II |
| 43 | [主記憶體（三）：DDR4 的狀態圖、時序參數與 memory controller](lecture43_note.md) | Ch10 | 1:07:58 | Chapter 10: Main Memory (DDR4 timing and the memory controller) -- Part III |
| 44 | [主記憶體（四）：非揮發記憶體、flash（NOR、NAND、P/E cycle、wear leveling）與 FeRAM](lecture44_note.md) | Ch10 | 57:56 | Chapter 10: Main Memory (Flash and FeRAM) -- Part IV |
| 45 | [主記憶體（五）：STT-MRAM、PCM、ReRAM、3D 記憶體與 roofline model](lecture45_note.md) | Ch10 | 42:36 | Chapter 10: Main Memory (MRAMs, PCM, ReRAM, Roofline model) -- Part V |
| 46 | [功率與溫度（一）：動態功率、ED² 指標、短路功率與六種漏電機制](lecture46_note.md) | Ch11 | 47:40 | Chapter 11:  Power and Temperature (dynamic and leakage power) --- Part I/III |
| 47 | [功率與溫度（二）：晶片散熱封裝、Fourier 熱傳導方程、FDM／電路類比、FEM 與 Green's function](lecture47_note.md) | Ch11 | 52:00 | Chapter 11:  Power and Temperature (temperature modeling) --- Part II/III |
| 48 | [功率與溫度（三）：DVFS、clock gating、power gating、drowsy cache 與溫度／排程管理](lecture48_note.md) | Ch11 | 42:23 | Chapter 11:  Power and Temperature (power and temperature management) --- Part III/III |
| 49 | [可靠度（一）：fault／error／failure、軟錯誤物理（Hazucha–Svensson）、遮罩機制、DIVA checker 與電感雜訊](lecture49_note.md) | Ch12 | 1:01:10 | Chapter 12: Reliability (Soft errors and inductive noise) -- Part I/III |
| 50 | [可靠度（二）：固有非確定性、確定性匯流排、設計缺陷（errata）、形式驗證與矽後除錯](lecture50_note.md) | Ch12 | 43:28 | Chapter 12: Reliability (non-determinism and design faults) -- Part II/III |
| 51 | [可靠度（三）：光微影與參數變異（Varius、SSTA、OPC／PSM）、電晶體老化（NBTI／HCI）、硬錯誤（RAMP）與浴缸曲線](lecture51_note.md) | Ch12 | 1:18:33 | Chapter 12: Reliability (Variation and Hard Errors) -- Part III/III |
| 52 | [安全處理器（一）：密碼學基礎、AES 區塊加密、CTR 模式、RC4、RSA 與 Diffie–Hellman](lecture52_note.md) | Ch13 | 1:10:14 | Chapter 13:  Secure Processor Architectures (Part I) -- Cryptographic Fundamentals and Encryption |
| 53 | [安全處理器（二）：SHA 雜湊、MAC、緩衝區溢位、TCB／TPM、記憶體加密引擎（MEE）與 Bonsai Merkle tree](lecture53_note.md) | Ch13 | 1:24:53 | Chapter 13: Security (Part II/III) -- Secure Processors |
| 54 | [安全處理器（三）：Intel SGX 飛地生命週期、反向分頁表（IPT）、Path ORAM 與旁道攻擊](lecture54_note.md) | Ch13 | 1:07:26 | Chapter 13: Secure Processor Architectures (Side channel attacks and ORAM) -- Part III/III |
| 55 | [機器學習硬體（一）：機器學習基礎、非線性活化（Sigmoid／tanh／ReLU）與卷積神經網路（CNN）](lecture55_note.md) | Ch14 | 1:14:37 | Chapter 14: Architectures for Machine Learning (Part I) -- Basic ML Concepts |
| 56 | [機器學習硬體（二）：七重迴圈數學表示法、loop tiling、halo pixels 與四種 stationarity 映射](lecture56_note.md) | Ch14 | 1:06:22 | Chapter 14: Architectures for Machine Learning (Part II) -- Mathematical representations |
| 57 | [機器學習硬體（三）：空間重用硬體資料流、脈動陣列（systolic arrays）、Retiming Lemma 與矩陣乘法轉換](lecture57_note.md) | Ch14 | 1:06:44 | Chapter 14: Architectures for Machine Learning (Part III) -- Hardware Architectures |
| 58 | [機器學習硬體（四）：推論最佳化、位元串列乘法、記憶體系統與記憶體內類比運算（PIM／ReRAM）](lecture58_note.md) | Ch14 | 38:54 | Chapter 14: Architectures for Machine Learning (Part IV) -- Optimizations and Memory Systems |
