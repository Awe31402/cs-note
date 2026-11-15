# DRAM 系列（Computer Science Lessons）隨堂筆記：任務定義

## Purpose
為 YouTube 播放清單「Dynamic Random Access Memory (DRAM)」（頻道 Computer Science Lessons，7 支，共約 59 分鐘）每支影片寫一份隨堂筆記 `lectureXX_note.md`。風格對齊 `../../../cs-note/computer-architecture/basic/`、`../../../cs-note/computer-architecture/advanced/`（繁體中文、`[mm:ss]`、標記系統）。這個系列是入門動畫，很多觀念在 `../../../cs-note/computer-architecture/basic/lecture20_note.md`、`../../../cs-note/computer-architecture/advanced/lecture41_note.md`～`lecture43_note.md` 講得更深，筆記在適當處連過去。

## Situation
- 讀者：看過 `../../../cs-note/computer-architecture/basic/`、`../../../cs-note/computer-architecture/advanced/` 的 DRAM 章節，想用動畫把「一個 bit 怎麼從 cell 走到 data bus」串起來。
- 檔名編號 = 播放清單順序（`playlist.txt`），和影片的 Part 編號一樣。
- 影片是投影片動畫＋旁白，沒有老師、沒有 Q&A、沒有課本。

## Inputs（優先順序）
1. 逐字稿 `transcripts/lectureXX.txt`。YouTube 抓字幕被擋（HTTP 429），改用 whisper `large-v3-turbo`（`--language en`）轉，片尾音樂被辨識成亂碼的部分已刪掉。
2. 投影片：每 3 秒取一張、去掉重複，當工作素材（不進 repo）。圖上的數字（位址、cell 內容、時序圖）以截圖為準。
3. 本 repo 其他筆記（`../../../cs-note/computer-architecture/basic/`、`../../../cs-note/computer-architecture/advanced/`）與 JEDEC 世代的通用常識（DDR4 BL8、DDR5 BL16 等），標 `【補充】`。
4. 本機實測（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）：影片的模型用 Python 小程式重跑，放在 `experiments/lectureXX/`，輸出存成同名 `.out`。本機的 row conflict、頻寬已在 `../../../cs-note/computer-architecture/advanced/lecture41_note.md`、`lecture42_note.md` 量過，這裡不重複，只連過去。
5. 不用外部筆記或網站。查不到的寫「待補」，不捏造。

## Limits
- 只寫該支影片的內容；不捏造 Q&A。
- 程式碼節錄 ≤ 30 行一段，附 `檔名:行號`。
- 只動 `notes/dram-notes/cs-lessons/` 底下的檔案。
- 不 commit，除非被要求。
- 影片說法跟實際 DRAM 不同時並陳並標 ⚠️。

## Done-when
每支影片一份 `lectureXX_note.md`：標頭、目錄、帶 `[mm:ss]` 的章節、`【口頭】【補充】【實測】` 標記、⚠️、一頁速查表、與其他講次的連結、一句話總結、斜體頁尾。`README.md` 索引全部連到、沒有壞連結。
