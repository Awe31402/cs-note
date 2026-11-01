# 讀書筆記製作 Brief

> 這份 brief 定義 `reading-notes.md` 的製作規格與章節標準。

## 1. Purpose（目的）

這份筆記要**代替讀原書**。讀者讀完筆記後，應該能說出每章在講什麼、核心觀點是什麼、書中主張的做法是什麼——不需要再翻 250 頁的原文。

不是查詢用的操作手冊，所以不追求每個指令完整可貼上執行；但架構與概念的完整度要夠，能讓嵌入式開發者與資安架構師判斷「這套安全開機與加密架構在我們的產品中該如何落地實作」。

## 2. Situation（背景）

- **書**：《Secure Boot Encryption with Linux: Implementation for Embedded Developers》，Rodolfo Giometti，Apress (Apress Pocket Guides)，2026 年，252 頁，英文
- **讀者**：嵌入式 Linux 開發者、韌體工程師、系統架構師與資安人員
  - → 密碼學與硬體安全名詞（Hardware Root-of-Trust、eFuse、CAAM、Black Key、Keyring、AF_ALG、fitImage、switch_root、RPMB、Tamper Detection…）深入淺出，並輔以架構圖與表格解析
  - → Linux 基礎概念（kernel、initramfs、device mapper、ext4）帶過即可
- **筆記語言**：中文（繁體）
- **引用語言**：英文原文照抄，附原書印刷頁碼

## 3. Inputs（來源）

- **唯一來源**：`secure-boot-linux.pdf`（`books/secure-boot/secure-boot-linux.pdf`）
- **不引入外部資料**。書中沒寫的，標註「書中未提」，不補充、不推測、不用常識填空
- 引用必須是原文逐字，不改寫、不翻譯後再回譯
- 頁碼取自原書印刷頁碼（書本頁眉／頁腳數字，非 PDF 檔案頁次）

## 4. Limits（界線）

- 全面涵蓋第 1 章至第 4 章，並納入極具實務價值的「附錄 A（防篡改偵測）」與「附錄 B（跨階段機密防護）」
- 不寫成空泛介紹文，保留關鍵指令骨架與 C 語言核心函數邏輯
- 嚴格遵守客觀技術記錄，不偏袒或評論不同晶片廠商之優劣

## 5. Done-when（完成定義）

每章包含以下區塊，順序固定：

1. `## 第 N 章：英文原標題（中文標題）` + 對應原書頁碼範圍
2. `### 這章在幹嘛` — 3–5 句，講這章的動機和骨架
3. `### 重點 N：<標題>` — 每章 8–14 個重點，每個重點包含：
   - 中文說明（白話、精確、短句）
   - `> **原文引用（p.XX）**` 區塊，1–3 句英文原文逐字照錄
   - 需要時附程式碼骨架、表格、架構 ASCII 圖、指令示範
   - `**要記住的觀點**：` 一句話收尾（用於關鍵重點）
4. `### 本章 Summary（作者原話）` — 引用書中 Summary 段落
5. `### 思考與實務檢驗` — 針對該章核心設計與常見坑洞進行反思
6. `### 我的整理：本章帶走的五句話` — 5 句核心精華濃縮
7. `### 延伸資源` — 原書提及之開源專案與工具

**檔案產出**：
- `/home/awe/disk/cs-note/secure-boot/reading-notes.md`（完整導讀摘要，約 1300 行 Markdown）
- `/home/awe/disk/cs-note/secure-boot/NOTES-BRIEF.md`（規格與製作準則）
