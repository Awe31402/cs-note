# 設計自動化（一）隨堂筆記：任務定義

## Purpose
為 YouTube 播放清單「【11402李炯霆教授｜設計自動化一】」（NTHU OCW）每支影片寫一份隨堂筆記 `lectureXX_note.md`，跟著影片走、並對照課本 Wang, Chang, Cheng（eds.）*Electronic Design Automation: Synthesis, Verification, and Test*（Morgan Kaufmann, 2009）。風格對齊 `../computer-architecture/basic/`（繁體中文、`[mm:ss]`、標記系統）。

## Situation
- 讀者：會 C／Python、懂資料結構與演算法、看過 `../computer-architecture/` 的數位邏輯與 CMOS 章節。
- 檔名編號 = 播放清單順序（`playlist.txt`），不是章節編號。每份筆記標頭寫明影片的章節名。
- 影片用的是**老師自己的投影片章號**，和課本章號不同：

  | 播放清單 | 影片章節 | 課本章節 |
  |---|---|---|
  | 1 | ch1 Introduction | Ch1 |
  | 2 | ch1 intro + transistor design | Ch1、Ch2 |
  | 3 | ch2 complexity | Ch4 |
  | 4 | ch2 Python 範例 | Ch4 |
  | 5 | ch2 Steiner tree | Ch4、Ch12 §12.4.3 |
  | 6 | ch10 synthesis | Ch6（待依逐字稿確認是否含 Ch5） |
  | 7 | ch3 compaction | 課本沒有 layout compaction（Ch3 的 compaction 是測試的 response compaction，不同東西） |
  | 8 | ch4 partitioning | Ch11 §11.3（課本沒有獨立的 partitioning 章） |
  | 9 | ch5 floorplanning | Ch10 |
  | 10 | ch6 placement | Ch11 |
  | 11–12 | ch7 routing | Ch12 |
  | 13 | ch5 pin assignment | 課本沒有專節（待確認） |
  | 14 | ch8 channel routing、clock、P/G | Ch12 §12.5.1、Ch13 |
  | 15 | ch8 clock tree synthesis | Ch13 §13.3 |

- 影片與課本章號、內容不一致時用 ⚠️ 標出。

## Inputs（優先順序）
1. 逐字稿 `transcripts/lectureXX.txt`。影片沒有任何字幕，全部用 whisper `large-v3-turbo`（`--language zh`）轉錄成 `[mm:ss] 文字`。中文語音辨識會錯字（尤其專有名詞），讀的時候要依上下文和課本判斷；看不懂的地方寫 ⚠️，不猜。
2. 投影片：影片是全螢幕投影片＋老師講話。從影片每 3 秒取一張、去掉重複，得到投影片截圖與時間點（只當工作素材，不放進 repo）。投影片上的字、圖、例子以截圖為準；口語細節以逐字稿為準。
3. 課本 `../books/eda/eda.pdf`。有文字層，可 pdftotext。**PDF 頁碼 = 印刷頁碼 + 37**。筆記引用印刷頁碼。
4. 本機實測（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10、gcc 11.4）：演算法（KL／FM partitioning、Steiner tree、maze routing、left-edge channel routing、slicing floorplan 等）用 Python 小程式跑課本或影片的例子驗證。實測程式放在 `experiments/lectureXX/`，筆記用 `experiments/lectureXX/檔名:行號` 引用。
5. 不用外部筆記或網站。查不到的寫「待補」，不捏造。

## Limits
- 只寫該講內容；不整章抄課本；不捏造 Q&A；不提供習題解答。
- 程式碼節錄 ≤ 30 行一段，附 `檔名:行號`。
- 只動 `electronic-design-automation/` 底下的檔案。
- 不 commit，除非被要求。
- 影片、課本、實測不一致時兩者並陳並註明來源（⚠️），不默默選一邊。

## Done-when
每講一份 `lectureXX_note.md`：標頭（課程／講者／來源／標記說明）、目錄、帶 `[mm:ss]` 的章節、`【口頭】【Q&A】【課本補充】【實測】` 標記、⚠️、一頁速查表、與後續講次的連結、一句話總結、斜體頁尾。

檢查點：先交 `BRIEF.md`、`README.md`、`lecture01_note.md`、`lecture02_note.md` 給使用者看，通過後再依序寫完其餘講次。

整門課完成 = 每支影片都有筆記、`README.md` 索引全部連到、沒有壞連結。
