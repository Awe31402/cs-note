# Advanced Computer Architecture 隨堂筆記：任務定義

和 `../basic/BRIEF.md` 一樣的規則；這裡只寫不一樣的地方。

## Purpose
為 YouTube 播放清單「Advanced Computer Architecture -- Smruti R. Sarangi」每支影片寫一份隨堂筆記 `lectureXX_note.md`，跟著影片走、對照 Sarangi 的第二本課本。接在 `../basic/` 之後讀。

## Situation
- 讀者：看完 `../basic/` 的人。基礎觀念（pipeline、cache、coherence…）直接連回 Basic 的筆記，不重講。
- 檔名編號 = 播放清單順序（`playlist.txt`）。
- 課本 *Next-Gen Computer Architecture: Till the End of Silicon* **Version 3.2**。**影片章號 = 課本章號**：

  | 播放清單 | 影片章節 | 課本章節（v3.2） |
  |---|---|---|
  | 1 | 首頁連結（47 秒） | 跳過，不寫筆記 |
  | 2 | Past, Present, and Future | 同一支影片（`N9rbSzW9CRE`）已寫在 [Basic 第 47 支](../basic/lecture47_note.md)，這裡只放指向它的短筆記 |
  | 3 | Chapter 1 | Ch1 Introduction（p.13） |
  | 4–6 | Chapter 2 | Ch2 Out-of-order Pipelines（p.25） |
  | 7–9 | Chapter 3 | Ch3 The Fetch and Decode Stages（p.61） |
  | 10–13 | Chapter 4 | Ch4 The Issue, Execute, and Commit Stages（p.95） |
  | 14–17 | Chapter 5 | Ch5 Alternative Approaches to Issue and Commit（p.151） |
  | 18–20 | Chapter 6 | Ch6 Graphics Processors（p.213） |
  | 21–26 | Chapter 7 | Ch7 Caches（p.253） |
  | 27–31 | Chapter 8 | Ch8 The On-chip Network（p.335） |
  | 32–40 | Chapter 9 | Ch9 Multicore Systems（p.415） |
  | 41–45 | Chapter 10 | Ch10 Main Memory（p.535） |
  | 46–48 | Chapter 11 | Ch11 Power and Temperature（p.607） |
  | 49–51 | Chapter 12 | Ch12 Reliability（p.637） |
  | 52–54 | Chapter 13 | Ch13 Secure Processor Architectures（p.671） |
  | 55–58 | Chapter 14 | Ch14 Architectures for Machine Learning（p.703） |

## Inputs（優先順序）
1. 逐字稿 `transcripts/lectureXX.txt`。YouTube 字幕缺的、或辨識成錯的語言的，改用本機 whisper（`large-v3-turbo`，GPU）轉錄；這些檔案旁邊有一個空的 `lectureXX.whisper` 標記檔，筆記標頭也會寫「whisper 轉錄」。
2. 課本 `../../books/computer-arch/nextgen-comp-arch.pdf`（v3.2）。**PDF 頁碼 = 印刷頁碼 + 2**。筆記引用印刷頁碼。
3. Basic 課本 `archbook.pdf` 與四本輔助課本（同 Basic）。
4. 本機實測（同 Basic；另有 RTX 4060 Laptop GPU、CUDA 12.4）。Linux v6.2.12 原始碼只在真正相關時用。
5. 不用外部筆記或網站。查不到的寫「待補」。

## Limits
同 `../basic/BRIEF.md`。

## Done-when
同 `../basic/BRIEF.md`：每支要寫的影片都有筆記、`README.md` 索引全部連到、沒有壞連結。
