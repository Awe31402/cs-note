# DRAM 系列（Computer Science Lessons）隨堂筆記索引

依播放清單順序（`playlist.txt`）。YouTube 頻道 Computer Science Lessons 的「Dynamic Random Access Memory (DRAM)」7 支動畫，從一個 cell 講到 DIMM、burst、bank interleaving 和 address mapping。沒有課本；逐字稿是 whisper 轉的（`transcripts/`）。實測程式在 `experiments/`。任務定義見 `BRIEF.md`。

同一個主題講得更深的地方：`../../../cs-note/computer-architecture/basic/lecture20_note.md`（SRAM／DRAM cell）、`../../../cs-note/computer-architecture/advanced/lecture41_note.md`～`lecture43_note.md`（DRAM 陣列、DDR 協定、時序與 memory controller）。

| # | 筆記 | 長度 | 原影片標題 |
|---|---|---|---|
| 1 | [DRAM cell 與陣列：電晶體＋電容、precharge、sense amplifier、refresh](lecture01_note.md) | 10:28 | Dynamic Random Access Memory (DRAM). Part 1: Memory Cell Arrays |
| 2 | [讀與寫的週期：位址多工、RAS#／CAS#／WE#、時序圖](lecture02_note.md) | 10:36 | Dynamic Random Access Memory (DRAM). Part 2: Read and Write Cycles |
| 3 | [Binary decoder：3-to-8、enable、用兩顆組 4-to-16](lecture03_note.md) | 3:50 | Dynamic Random Access Memory (DRAM). Part 3: Binary Decoders |
| 4 | [Multiplexer 與 demultiplexer：TDM、閘級電路、在 DRAM 裡的用途](lecture04_note.md) | 8:41 | Dynamic Random Access Memory (DRAM). Part 4: Multiplexers and Demultiplexers |
| 5 | [DIMM 的組織：長方形 array、bank、chip、rank、channel](lecture05_note.md) | 7:38 | Dynamic Random Access Memory (DRAM). Part 5: DIMM Organisation |
| 6 | [Burst mode 與 bank interleaving：channel 的線、DDR、prefetch、bank 數的取捨](lecture06_note.md) | 12:10 | Dynamic Random Access Memory (DRAM). Part 6: Burst Mode and Bank Interleaving |
| 7 | [Memory address mapping：把位址 bit 切成 row／column／bank](lecture07_note.md) | 5:51 | Dynamic Random Access Memory (DRAM). Part 7: Memory Address Mapping |
