# 任務說明：Operating Systems: A Linux Kernel-Oriented Approach 課堂筆記

> 確認日期：2026-10-22

## 範圍

- 課程 A：Smruti R. Sarangi 的 YouTube 播放清單（33 講）
  https://www.youtube.com/playlist?list=PL1iLu2CSC9EW0VVEBN0XhTMoMVMW4Xd42
  ＋ 課本 `books/os/osbook-v0.96.pdf`
- 課程 B（之後）：Rajib Mall《Real Time Systems》（31 講）
  https://www.youtube.com/playlist?list=PL1iLu2CSC9EU4mMByEhBp9CcYgAIiDs_v
  ＋ 課本 `books/os/real-time-systems-rajib-mall-pearson-education-india-2007.pdf`
- 切法：**一部影片 = 一份筆記**（`lectureXX_note.md`）
- 順序：**先做 A 第 1 講**當樣品 → 確認 OK 再做 A 其他 32 講 → 最後做 B

## 1. Purpose（用途）

給自己自學、複習用。看完筆記就懂這一講，不用重看影片。
教授口頭多講、課本沒有的內容，要特別標出來（`【口頭】`）。
Sarangi 會講很多 Linux 核心原始碼（`task_struct`、`mm_struct`…），筆記要附
**kernel 原始碼節錄**與觀念對照，讓我之後能自己翻 kernel 原始碼對得上。

## 2. Situation（背景）

- 讀者會 **C 語言和基本計組**，但**沒讀過 kernel 原始碼** → 原始碼要逐行解釋，不能只貼。
- **繁體中文**，專有名詞保留英文。
- 影片是 Sarangi 配合他自己的書上課；標題標有「Chap N, Part M」，對應章節明確。
- 輸出位置：課程 A → `os/linux-os/`；課程 B → `os/real-time-systems/`。

## 3. Inputs（資料）

1. **主要**：該講影片逐字稿（用 `yt-dlp` 抓 YouTube 字幕，放 `transcripts/`）。筆記骨架照影片順序。
2. **次要**：課本對應章節，補定義、圖、例子。
3. **原始碼**：Linux **v6.2.12**（課本指定版本），放 `os/linux-src/`。
   節錄要標 **檔名:行號**，例如 `include/linux/sched.h:737`。
4. **不用**：網路上其他筆記或文章。
5. 抓不到、查不到 → 寫「待補」，**不亂編**。
6. 課本、影片、原始碼不一致 → 都寫，標明出處。

## 4. Limits（不能做）

- 只寫該講講到的內容；最多在最後一節列「後面哪一講會用到」。
- 不整段抄課本，用自己的話重寫；公式、定義可照用。
- 不編 `【Q&A】`：逐字稿裡真的有學生提問才寫。
- 不寫課本習題答案。
- 原始碼只節錄跟觀念有關的部分，**一段 30 行內**；只用 v6.2.12，不拿新版改動來講（除非教授自己提到）。

## 5. Done-when（完成條件）

- **檢查點**：先交目錄大綱（每節一行）→ 確認 OK 才寫全文。
- **完成**：`lectureXX_note.md`，結構照 `~/disk/math-note/cryptography-notes/lecture01_note.md`：
  - 開頭資訊（課程、講者、來源、標記說明）
  - 目錄
  - 各節內容（含 `【口頭】`、`【Q&A】`、`【原始碼】`）
  - 一頁速查表
  - 與後續講次的連結
  - 一句話總結
- `【原始碼】`：kernel 節錄（檔名:行號 ＋ 逐行解說）。
