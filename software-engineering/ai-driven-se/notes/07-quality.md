# 07 軟體品質管理與保證

> 李允中《軟體工程》第 7 章（僅作目錄）。主要依據：Sommerville 9e Ch.24、Ch.15 §15.1、Pressman 9e Ch.15–17、Ch.23、《Software Engineering at Google》Ch.8–9、Ch.20。

---

## ① 核心原則

| 原則 | 說明 | 出處 |
|---|---|---|
| 品質管理涵蓋標準、審查、度量 | 品質不是測出來的，而是由流程與標準建立的 | Sommerville Ch.24 §24.1–24.4 |
| 審查與檢視 | 由人檢查程式碼與文件，找出測試找不到的問題 | Sommerville Ch.24 §24.3；Pressman Ch.16 |
| 靜態分析 | 不執行程式就找出錯誤 | Sommerville Ch.15 §15.1 |
| SQA 是系統化的活動 | 有計畫、有目標、有度量 | Pressman Ch.17 |
| 度量 | 產品度量、流程度量、品質度量 | Pressman Ch.23；Sommerville Ch.24 §24.4 |
| 規則要有理由，且盡量自動執行 | 風格指南的規則要說明為什麼存在，並盡可能由工具強制 | Google Ch.8 |
| 靜態分析要整合進開發流程 | 分析結果要在開發者工作的地方出現（例如 code review），而且誤報率要低，否則會被忽略 | Google Ch.20（介紹 Tricorder） |

---

## ② AI 開發中如何落地

**核心轉變：品質保證從「人檢查」轉向「機器檢查」，人只處理機器無法判斷的部分。**

這正是倒金字塔的精神（詳見 [`quality-strategy.md`](../quality-strategy.md)）：

| 傳統品質活動 | AI 開發中的對應 |
|---|---|
| 人工 code review 逐行讀 | 機器檢查先過濾；人看驗證產物與設計決策 |
| 風格指南寫在 wiki | 寫成 lint 規則，由 CI 強制 |
| 審查時糾正錯誤 | **把糾正轉成檢查**（lint 規則紀錄簿） |
| 定期品質稽核 | 每個 PR 自動執行 |

**Google Ch.20 的提醒同樣適用於 AI**：誤報多的檢查會被忽略。對 AI 來說更嚴重，因為 AI 會想辦法繞過它，例如加 `NOLINT`。所以每條規則都要精準。

**人工審查的重點**（單人版）
- 設計是否符合 ADR 與架構邊界？
- 測試是否真的驗證需求？（AI 最容易在這裡偷懶）
- 有沒有新增抑制註解（`NOLINT`、`eslint-disable`）？

可直接使用的 Claude Code skill：`code-review`（依標準與規格兩個面向審查）。

---

## ③ 對應的自動化檢查

| 檢查 | 工具 |
|---|---|
| 編譯器警告視為錯誤 | GCC / Clang |
| 靜態分析 | clang-tidy（內建加自訂）、ESLint |
| 執行期錯誤偵測 | ASan、UBSan、TSan |
| 格式 | clang-format、Prettier |
| 抑制註解偵測 | CI 腳本 |
| **品質度量** | revert 率、CI 失敗原因統計、correction log 條數（[10 流程改善](./10-process-improvement.md)） |

---

## ④ 專案連結

- **P0**：建立所有靜態分析與 CI 關卡
- **P1**：第一批自訂 lint 規則（[`phase-1-mvp.md#第一批-lint-規則候選`](../project/phase-1-mvp.md#第一批-lint-規則候選)）
- **P5**：LLM 輸出品質的評估集
- **全階段**：lint 規則紀錄簿（[`quality-strategy.md#六lint-規則紀錄簿correction-log`](../quality-strategy.md#六lint-規則紀錄簿correction-log)）
