# 讀書筆記製作 Brief

> 這份 brief 定義 `reading-notes.md` 的製作規格。照 `../linux-ai-guide/NOTES-BRIEF.md` 的模子改寫。

## 1. Purpose（目的）

這份筆記要**代替讀原書**。讀者讀完筆記後，應該能說出每章在講什麼、核心觀點是什麼、書中主張的做法是什麼——不需要再翻 500 多頁的原文。

不是查詢用的手冊，所以不追求程式碼完整可編譯；但概念要夠完整，讓讀者能判斷「這個 pattern 我用不用得上」，也能在 code review 時叫得出 anti-pattern 的名字。

## 2. Situation（背景）

- **書**：《Dependency Injection Principles, Practices, and Patterns》，Steven van Deursen & Mark Seemann，Manning，2019，ISBN 978-1-61729-473-0，英文。範例全部是 C#／.NET Core
- **讀者**：C／Linux／嵌入式底子熟，C#／.NET 不熟
  - → C#／.NET 名詞（interface、generic、attribute、ASP.NET Core、Entity Framework、`IDisposable`…）第一次出現時用白話解釋，必要時對照 C 的寫法（函式指標表、ops struct）
  - → 一般程式名詞（module、library、unit test、mock）不用解釋
  - → 書中的 pattern／anti-pattern 名稱保留英文（書中用小型大寫標示的術語，如 Composition Root、Control Freak），第一次出現附中文
- **筆記語言**：中文（繁體）
- **引用語言**：英文原文照抄，附頁碼

## 3. Inputs（來源）

- **唯一來源**：`dependency-injection.pdf`（用 `pdftotext -layout` 抽成 `txt/book.txt`，不進 git）
- **不引入外部資料**。書中沒寫的，標「書中未提」，不補充、不推測
- 引用必須是原文逐字
- 頁碼是書本頁碼（頁眉／頁腳數字）。**PDF 頁＝書本頁＋31**

## 4. Limits（界線）

- 15 章全做。第 13～15 章（Autofac、Simple Injector、MS.DI 三個容器各一章）結構重複，每章可縮短到約 200～300 行，重點放在「這個容器跟別人不一樣的地方」
- 跳過 preface、acknowledgments、about the authors、glossary、index
- 不寫成教學文，不評論書寫得好不好，不比較其他書
- 程式碼只留關鍵骨架（C#），不貼完整 listing；長 listing 用文字描述

## 5. Done-when（完成定義）

每章包含以下區塊，順序固定：

1. `## 第 N 章：英文原標題（中文標題）` + 對應原書頁碼範圍
2. `### 這章在幹嘛` — 3–5 句，講這章的動機和骨架
3. `### 重點 N：<標題>` — 每章 8–14 個，每個重點包含：
   - 中文說明（白話，短句）
   - `> **原文引用（p.XX）**` 區塊，1–3 句英文原文
   - 需要時附程式碼骨架、表格、名詞解釋
   - `**要記住的觀點**：` 一句話收尾（用於關鍵重點）
4. `### 本章 Summary（作者原話）` — 引用書中章末 Summary 的條列
5. `### 我的整理：本章帶走的 N 句話` — 3–5 句，我自己的濃縮

（這本書沒有練習題、章末延伸資源，所以不列這兩節。）

**每章長度**：約 300–450 行（第 13～15 章 200–300 行）

**格式規則**：
- 每章之間用 `---` 分隔
- 引用一律用 blockquote，標題行為 `**原文引用（p.XX）**`
- 程式碼用 fenced code block，標語言（csharp / text）
- 表格照抄書中表格，欄位名保留英文並附中文

**檔案**：`reading-notes.md`（單一檔案，依章節順序累積）＋`reading-notes.html`（用 `../linux-ai-guide/reading-notes.html` 同一套版面產生）
