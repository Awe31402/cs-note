# Real Time Systems 隨堂筆記：任務定義

## Purpose
為 YouTube 播放清單「Real Time Systems」（Smruti R. Sarangi, IIT Delhi；投影片與課本為 Rajib Mall, IIT Kharagpur）每支影片寫一份隨堂筆記 `lectureXX_note.md`，風格對齊 `../linux-os/` 與 `../../math-note/cryptography-note/`。在觀念適用的地方，節錄 Linux kernel v6.2.12 原始碼（`../linux-src/`）或在本機實測，做觀念對照。

## Situation
- 讀者：會 C、懂基本計算機結構，修過（或正在看）同一位老師的 OS 課（`../linux-os/`）。
- 檔名編號 = 播放清單順序（`playlist.txt`），不是官方講次編號。對應：
  - 第 1–7 支 = Lecture 1–7
  - 第 8 支 = Lectures 8 and 9（同一支影片）
  - 第 9–28 支 = Lecture 10–29
  - 第 29 支 = Lecture 30 Part I、第 30 支 = Lecture 30 Part II
  - 第 31 支 = Lecture 31
  每份筆記標頭寫明官方講次。

## Inputs（優先順序）
1. 逐字稿 `transcripts/lectureXX.txt`（YouTube 自動字幕；第 4 支只有印地語自動字幕，改用 whisper medium 轉錄英文音訊）。
2. 課本 PDF `../../books/os/real-time-systems-rajib-mall-pearson-education-india-2007.pdf`。實為 IIT Kharagpur「Version 2 CSE」講義（依 Mall 的書編寫，190 頁）。PDF 文字層亂碼，無法 pdftotext，改以頁面影像閱讀。章節對應（PDF 頁碼）：
   - Ch1 Introduction：1–30
   - Ch2 Real-Time Task Scheduling：31–73
   - Ch3 Handling Resource Sharing and Dependencies：74–96
   - Ch4 Multiprocessor and Distributed Real-Time Systems：97–107
   - Ch5 Real-Time Operating Systems：108–138
   - Ch6 Real-Time Communication：139–177
   - Ch7 Real-Time Databases：178–190
3. Linux kernel v6.2.12 原始碼（只在有關的講次：排程、POSIX、Linux-based RTOS、計時器、鎖與優先權繼承等）。
4. 不用外部筆記。查不到的寫「待補」，不捏造；課堂、課本、原始碼不一致時兩者並陳並註明來源（⚠️）。

## Limits
- 只寫該講內容；不整本抄課本；不捏造 Q&A；不提供習題解答。
- 原始碼節錄 ≤ 30 行一段，僅限 v6.2.12，附 `檔名:行號`。

## Done-when
每講一份 `lectureXX_note.md`：標頭（課程／講者／來源／標記說明）、目錄、帶 `[mm:ss]` 的章節、`【口頭】【Q&A】【課本補充】【原始碼】【實測】` 標記、⚠️、一頁速查表、與後續講次的連結、一句話總結、斜體頁尾。
