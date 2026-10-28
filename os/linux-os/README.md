# Operating Systems（Smruti R. Sarangi）隨堂筆記索引

依播放清單順序。每份筆記含原始碼節錄（Linux v6.2.12，`../linux-src/`）與本機實測。任務定義見 `BRIEF.md`。

| # | 筆記 | 長度 | 原影片標題 |
|---|---|---|---|
| 1 | [導論（Introduction）](lecture01_note.md) | 49:20 | Operating Systems (Smruti R. Sarangi): Introduction |
| 2 | [特權暫存器與 Ring Level（Chapter 2, Part I）](lecture02_note.md) | 59:19 | Operating Systems (Smruti R. Sarangi): Privileged registers, ring levels (Chap 2, Part I) |
| 3 | [Timer Interrupt 與 x86（Chapter 2, Part II）](lecture03_note.md) | 34:58 | Operating Systems (Smruti R. Sarangi): Timer interrupts and x86 (Chap 2, Part II) |
| 4 | [靜態與動態連結（Chapter 2, Part III）](lecture04_note.md) | 39:54 | Operating Systems (Smruti R. Sarangi): Static and Dynamic Linking (Chap 2, Part III) |
| 5 | [虛擬記憶體（Chapter 2, Part IV）](lecture05_note.md) | 46:56 | Operating Systems (Smruti R. Sarangi): Virtual Memory (Chap 2, Part IV) |
| 6 | [分段記憶體與 I/O（Chapter 2, Part V）](lecture06_note.md) | 1:03:41 | Operating Systems (Smruti R. Sarangi): Segmentation and I/O (Chap 2, Part V) |
| 7 | [Linux 的 Process 狀態（Chapter 3, Part I）](lecture07_note.md) | 43:15 | Operating Systems (Smruti R. Sarangi): Process States in Linux (Chap 3, Part I) |
| 8 | [Kernel Stack 與優先權（Chapter 3, Part II）](lecture08_note.md) | 43:12 | Operating Systems (Smruti R. Sarangi): Kernel Stack and Priorities (Chap 3, Part II) |
| 9 | [`mm_struct` 與 Maple Tree（Chapter 3, Part III）](lecture09_note.md) | 51:36 | Operating Systems (Smruti R. Sarangi): mm_struct and maple trees (Chap 3, Part III) |
| 10 | [PID 與 Namespace（Chapter 3, Part IV）](lecture10_note.md) | 1:10:04 | Operating Systems (Smruti R. Sarangi): PIDs and Namespaces (Chap 3, Part IV) |
| 11 | [Process 的建立（Chapter 3, Part V）](lecture11_note.md) | 36:54 | Operating Systems (Smruti R. Sarangi): Process creation (Chap 3, Part V) |
| 12 | [Context Switch（Chapter 3, Part VI）](lecture12_note.md) | 37:51 | Operating Systems (Smruti R. Sarangi): Context Switch (Chap 3, Part VI) |
| 13 | [Context Switch 細節（Chapter 3, Part VII）](lecture13_note.md) | 44:16 | Operating Systems (Smruti R. Sarangi): Context Switch Details-II (Chap 3, Part VII) |
| 14 | [Library Call 與 System Call（Chapter 4, Part I）](lecture14_note.md) | 41:55 | Operating Systems (Smruti R. Sarangi): Library calls and  System calls (Chap 4, Part I) |
| 15 | [x86 的中斷架構（Chapter 4, Part II）](lecture15_note.md) | 1:07:33 | Operating Systems (Smruti R. Sarangi): Interrupt Architecture in x86 (Chap 4, Part II) |
| 16 | [Exception、softirq、threaded IRQ、work queue（Chapter 4, Part III）](lecture16_note.md) | 57:17 | Operating Systems (Smruti R. Sarangi): Exceptions, softirqs, threaded IRQs, wk qs (Chap 4, Part III) |
| 17 | [Signal（Chapter 4, Part IV）](lecture17_note.md) | 54:37 | Operating Systems: A Linux Kernel-Oriented Approach (Smruti R. Sarangi) Signals (Chap 4, Part IV) |
| 18 | [多執行緒與同步基礎（Chapter 5, Part I）](lecture18_note.md) | 1:08:58 | Operating Systems:A Linux Kernel-Oriented Approach (Smruti Sarangi) Multithreading (Chap 5, Part I) |
| 19 | [記憶體模型（Chapter 5, Part II）](lecture19_note.md) | 51:13 | Operating Systems:A Linux Kernel-Oriented Approach (S. R. Sarangi) Memory models (Chap 5, Part II) |
| 20 | [進度保證、Semaphore、Reader-Writer Lock 與 Wait-Free Queue（Chapter 5, Part III）](lecture20_note.md) | 42:32 | Operating Systems:A Linux Kernel-Oriented Approach, RW Locks and queues (Chap 5, Part III) |
| 21 | [並行 Queue、Condition Variable、Barrier（Chapter 5, Part IV）](lecture21_note.md) | 35:48 | Operating Systems:A Linux Kernel-Oriented Approach, Concurrent queues (Chap 5, Part IV) |
| 22 | [Kernel 裡的並行（Chapter 5, Part V）](lecture22_note.md) | 57:37 | Operating Systems:A Linux Kernel-Oriented Approach, Concurrency in the kernel (Chap 5, Part V) |
| 23 | [RCU（Chapter 5, Part VI）](lecture23_note.md) | 46:40 | OS: A Linux Kernel-Oriented Approach, RCU (Chapter 5, Part VI) |
| 24 | [排程（Chapter 5, Part VII）](lecture24_note.md) | 41:21 | Operating Systems:A Linux Kernel-Oriented Approach, Scheduling (Chap 5, Part VII) |
| 25 | [多處理器排程與 Banker's Algorithm（Chapter 5, Part VIII）](lecture25_note.md) | 39:53 | Operating Systems: A Linux Kernel-Oriented Approach,  Multiprocessor scheduling (Chap 5, Part VIII) |
| 26 | [Linux 的排程與 CFS（Chapter 5, Part IX）](lecture26_note.md) | 56:13 | Operating Systems: A Linux Kernel-Oriented Approach,  Linux scheduling (Chap 5, Part IX) |
| 27 | [即時排程（Chapter 5, Part X）](lecture27_note.md) | 48:35 | OS: A Linux Kernel-Oriented Approach, Real-time Schedulers (Chap 5, Part X) |
| 28 | [即時系統的資源分配（Chapter 5, Part XI）](lecture28_note.md) | 53:16 | OS: A Linux Kernel-Oriented Approach, Real-time Resource Allocation (Chap 5, Part XI) |
| 29 | [傳統的頁面置換演算法（Chapter 6, Part I）](lecture29_note.md) | 42:34 | OS: A Linux Kernel-Oriented Approach, Traditional Page Replacement Algorithms (Chap 6, Part I) |
| 30 | [Kernel 的記憶體架構（Chapter 6, Part II）](lecture30_note.md) | 57:58 | OS: A Linux Kernel-Oriented Approach, Memory architecture of the kernel (Chap 6, Part II) |
| 31 | [Reverse Mapping（Chapter 6, Part III）](lecture31_note.md) | 47:05 | OS: A Linux Kernel-Oriented Approach, Reverse Mapping (Chap 6, Part III) |
| 32 | [MGLRU 與 Thrashing（Chapter 6, Part IV）](lecture32_note.md) | 1:00:06 | OS: A Linux Kernel-Oriented Approach, MGLRU Algorithm (Chap 6, Part IV) |
| 33 | [Kernel 的記憶體配置（Chapter 6, Part V）](lecture33_note.md) | 50:42 | OS: A Linux Kernel-Oriented Approach, Kernel Memory Allocation (Chap 6, Part V) |
