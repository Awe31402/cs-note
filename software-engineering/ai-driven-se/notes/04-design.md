# 04 軟體設計

> 李允中《軟體工程》第 4 章（僅作目錄）。主要依據：Sommerville 9e Ch.6–7、Pressman 9e Ch.9–11、Farley《Modern Software Engineering》Part III、Meyer《Object-Oriented Software Construction》2e、Gaczkowski《Software Architecture with C++》2e。

---

## ① 核心原則

| 原則 | 說明 | 出處 |
|---|---|---|
| 架構決策要明確記錄 | 架構是一連串的決策，要有理由 | Sommerville Ch.6 §6.1 |
| 架構模式 | 分層、MVC、儲存庫、管線等，各有適用情境 | Sommerville Ch.6 §6.3 |
| 模組化 | 把系統切成可以獨立理解與修改的部分 | Farley Ch.9；Meyer Ch.3〈Modularity〉 |
| 高內聚 | 相關的東西放在一起 | Farley Ch.10 |
| 關注點分離 | 每個模組只處理一件事 | Farley Ch.11 |
| 資訊隱藏與抽象 | 隱藏實作細節，只暴露必要介面 | Farley Ch.12；Pressman Ch.9 |
| 管理耦合 | 降低模組之間的依賴 | Farley Ch.13 |
| 契約式設計（DbC） | 以前置條件、後置條件、不變式明確定義元件責任 | Meyer Ch.11〈Design by Contract〉 |
| 設計模式 | 可重用的設計解法 | Sommerville Ch.7 §7.2；Pressman Ch.14 |

---

## ② AI 開發中如何落地

**「程式碼庫就是最大的 prompt。」**（Lauren Tan，第三方摘要）AI 會模仿它看到的結構。好的設計不只是為了人，更是為了讓 AI 每次只需要看一小部分程式碼就能正確修改。

| 設計原則 | 對 AI 的意義 |
|---|---|
| 模組化與資訊隱藏 | AI 只要讀介面就能使用模組，不必讀實作 → **節省 context** |
| 低耦合 | AI 改 A 時不會意外影響 B |
| 契約式設計 | 前置與後置條件寫成 assert 或型別，AI 違反時立即失敗 |
| 架構決策記錄 | AI 不會「好心」把你刻意的設計改回常見寫法 |

**本專案的關鍵設計：Ports & Adapters**
- C++ 核心不做 I/O，不知道自己在 WASM 還是 server 中執行（見 [`overview.md`](../project/overview.md#c--ts-邊界設計)）。
- 好處：核心可以完全用單元測試驗證；同一份核心同時用在客戶端與 server。

**讓錯誤寫法寫不出來**（倒金字塔第 1 層）
- 強型別 ID、`std::chrono` 時間、concepts 限制模板（見 [`quality-strategy.md`](../quality-strategy.md#二第-1-層程式碼本身)）。

可直接使用的 Claude Code skill：`codebase-design`（深模組設計詞彙）。

---

## ③ 對應的自動化檢查

| 檢查 | 做法 |
|---|---|
| **CMake target 依賴規則** | `core` 不得連結外層 target，違反時建置失敗（[`overview.md`](../project/overview.md#專案目錄結構monorepo初稿)） |
| `#include` 方向檢查 | 自訂 clang-tidy 檢查或腳本：core 不得 include 平台標頭 |
| 契約 | 前置與後置條件用 assert 或型別表達，debug 建置與測試中啟用 |
| TS 架構邊界 | ESLint 規則限制 UI 目錄可以 import 的模組 |

---

## ④ 專案連結

- **P0**：CMake target 劃分、第一條自訂邊界檢查
- **P1**：強型別領域模型、儲存 port（[`phase-1-mvp.md`](../project/phase-1-mvp.md)）
- **P2**：API 設計、server 架構
- **P3**：同步引擎與核心的介面
- **P6**：客戶端擴充時維持 UI 與核心的邊界
