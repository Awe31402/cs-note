# Basic Computer Architecture 隨堂筆記：任務定義

## Purpose
為 YouTube 播放清單「Basic Computer Architecture -- Smruti R. Sarangi -- Lecture Videos」每支影片寫一份隨堂筆記 `lectureXX_note.md`，跟著影片走、並對照 Sarangi 本人的課本。風格對齊 `../../os/real-time-systems/`（繁體中文、`[mm:ss]`、標記系統）。在觀念適用的地方連到 `../../os/linux-os/` 與 `../../os/real-time-systems/`。完成後接著做 `../advanced/`（Advanced Computer Architecture）。

## Situation
- 讀者：會 C、懂基本 OS，看過同一位老師的 OS 與 Real Time Systems 課。
- 檔名編號 = 播放清單順序（`playlist.txt`），不是章節編號。每份筆記標頭寫明影片章節與 Part。
- 影片錄於 2015 年 McGraw-Hill 版；手上課本是開放版 v3.09（2025），多了一章 RISC-V（Ch6），所以**影片 Ch6 之後的章號 = 課本章號 − 1**：

  | 播放清單 | 影片章節 | 課本章節（v3.09） |
  |---|---|---|
  | 1 | 書的介紹 | 前言／目錄 |
  | 2 | Chapt 1 | Ch1 |
  | 3–5 | Chapt 2 | Ch2 |
  | 6–8 | Chapt 3 | Ch3 |
  | 9–10 | Chapt 4 | Ch4 |
  | 11–14 | Chapt 5 | Ch5 |
  | 15–17 | RISC-V Chapter（無章號） | Ch6 |
  | 18–20 | Chapt 6 Digital Logic | Ch7 |
  | 21–26 | Chapt 7 Computer Arithmetic | Ch8 |
  | 27–29 | Chapt 8 Processor Design | Ch9 |
  | 30–33 | Chapt 9 Pipelining | Ch10 |
  | 34–37 | Chapt 10 Memory System | Ch11 |
  | 38–42 | Chapt 11 Multiprocessors | Ch12 |
  | 43–46 | Chapt 12 I/O | Ch13 |
  | 47 | Past, Present, and Future | 待確認 |
  | 48 | 首頁連結（1 分鐘） | 跳過，不寫筆記 |

- 第 47 支「Past, Present, and Future」也出現在 Advanced 播放清單（第 2 支）；只在這裡寫一次，Advanced 那邊指過來。
- 影片與課本章號、內容不一致時用 ⚠️ 標出。

## Inputs（優先順序）
1. 逐字稿 `transcripts/lectureXX.txt`（YouTube 自動字幕轉成 `[mm:ss] 文字`）；沒有英文字幕的改用 whisper 轉錄。
2. 課本 `../../books/computer-arch/archbook.pdf`（*Basic Computer Architecture* v3.09）。有文字層，可 pdftotext。**PDF 頁碼 = 印刷頁碼 + 3**。筆記引用印刷頁碼。
3. 輔助課本（只在有幫助時引用，註明書名＋頁碼）：
   - Patterson & Hennessy, *Computer Organization and Design RISC-V Edition*
   - Harris & Harris, *Digital Design and Computer Architecture RISC-V Edition*
   - Harris et al., *RISC-V System-on-Chip Design*
   - 奔跑吧 Linux 社區，《RISC-V 體系結構編程與實踐》
4. 本機實測（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、x86-64；另有 `aarch64-linux-gnu-gcc`）：組合語言章用 `gcc -S`／`objdump`，cache 章用 `lscpu`／`perf` 等。Linux v6.2.12 原始碼（`../../os/linux-src/`）只在真正相關時用（atomic、memory ordering 等）。實測程式放在 `experiments/lectureXX/`，筆記用 `experiments/lectureXX/檔名:行號` 引用。
5. 不用外部筆記或網站。查不到的寫「待補」，不捏造。

## Limits
- 只寫該講內容；不整章抄課本；不捏造 Q&A；不提供習題解答。
- 程式碼節錄 ≤ 30 行一段，附 `檔名:行號`；Linux 原始碼僅限 v6.2.12。
- 只動 `computer-architecture/` 底下的檔案（其他筆記只加連結）。
- 不 commit，除非被要求。
- 影片、課本、實測不一致時兩者並陳並註明來源（⚠️），不默默選一邊。

## Done-when
每講一份 `lectureXX_note.md`：標頭（課程／講者／來源／標記說明）、目錄、帶 `[mm:ss]` 的章節、`【口頭】【Q&A】【課本補充】【原始碼】【實測】` 標記、⚠️、一頁速查表、與後續講次的連結、一句話總結、斜體頁尾。

檢查點：先交 `BRIEF.md`、`README.md`、`lecture01_note.md`、`lecture02_note.md` 給使用者看，通過後再依序寫完 Basic，再做 Advanced（同樣先交前兩份）。

整門課完成 = 每支（要寫的）影片都有筆記、`README.md` 索引全部連到、沒有壞連結。
