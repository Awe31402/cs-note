# 大綱（檢查點 1）

> 依據 [`BRIEF.md`](./BRIEF.md)。每個檔案列出標題層級，每節附一句話說明。確認後才開始寫全文。

---

## `README.md`：總覽與導航
- **這是什麼**：一句話說明目的，連到 BRIEF。
- **核心問題**：為什麼十萬行會讓軟體工程從「建議」變成「必要」。
- **決策摘要**：專案、技術棧、品質策略各一行，連到 `decisions.md`。
- **閱讀順序**：建議先讀 quality-strategy → roadmap → 依階段讀對應章節。
- **章節 × 階段對照表**：十章 × 各階段的矩陣，標出每個階段練到哪幾章。
- **進度追蹤**：各檔案的撰寫狀態（草稿 / 完成）。

## `decisions.md`：決策紀錄（類 ADR）
每條決策固定格式：背景 → 選項 → 決定 → 理由 → 代價。
- **D-001 專案類型選 E（產品型）**：對比 A 資料庫、B 編譯器等選項；驗證疑慮如何用倒金字塔化解。
- **D-002 題目：英文單字卡**：自己就是真實使用者，需求會自然演進。
- **D-003 九個子系統分階段**：為什麼不刪減，以及 MVP 的切法。
- **D-004 技術棧 B+（C++20 + TS UI）**：對比 A 全 Qt、B C++ 核心、C 全 TS；目標是把 C++ 練好。
- **D-005 C++20**：concepts、ranges、coroutines 的用途；為什麼不選 C++23。
- **D-006 品質策略：倒金字塔單人版**：保留什麼、縮小什麼、延後什麼。
- **D-007 不複製 Anki 原始碼**：授權考量與替代做法。
- **決策範本**：之後新增決策時使用。

## `quality-strategy.md`：倒金字塔在本專案的具體化
- **Lauren Tan 的方法摘要**：四層架構與「糾正 → 檢查」原則（標明來源為第三方摘要）。
- **第 1 層：程式碼本身**：C++ 用型別與 concepts 讓錯誤寫法寫不出來；CMake target 強制模組邊界；C++ 核心與 TS 外殼的介面契約。
- **第 2 層：靜態分析**
  - C++：編譯器警告等級、clang-tidy（內建檢查加自訂檢查）、sanitizer（ASan / UBSan / TSan）、clang-format。
  - TS：`strict` 模式、ESLint 自訂規則、Prettier。
  - 測試：單元、property-based、golden、E2E 各自負責什麼。
  - CI 關卡：哪些檢查會擋下合併。
- **第 3 層：驗證**：AI 操作 app 並錄影或截圖（Playwright）；TLA+ 延到第三階段（同步）。
- **第 4 層：文字規範**：CLAUDE.md / AGENTS.md 只放無法機器檢查的判斷，並說明如何控制它的篇幅。
- **lint 規則紀錄簿（Correction Log）**：每次糾正 AI 的紀錄格式：問題 → 轉成哪條規則 → 規則檔位置。
- **防止 AI 作弊**：AI 改測試讓它通過、關掉 lint 規則、加 `NOLINT`；用 hook 和 CI 偵測。

---

## `notes/`：軟體工程十章筆記

每章固定四段：**① 核心原則**（出處：Sommerville / Pressman / 其他書，找不到標【待查】）→ **② AI 開發中如何落地** → **③ 對應的自動化檢查** → **④ 專案連結**（連到 roadmap 的哪個階段、哪個子系統）。

| 檔案 | ① 核心原則 | ② AI 開發中的落地重點 | ③ 自動化檢查範例 | ④ 主要對應階段 |
|---|---|---|---|---|
| `01-process.md` 軟體危機與流程 | 生命週期模型、增量與迭代、敏捷 | agent 迴圈就是微型迭代；人在迴圈中 vs. 無人監督的分界 | CI pipeline 作為流程本身 | 第 0 階段 |
| `02-requirements.md` 需求工程 | 需求萃取、功能 / 非功能需求、驗收條件、追溯性 | 需求寫成可執行規格（驗收測試），AI 才不會用猜的 | 需求 ID ↔ 測試的追溯檢查腳本 | 第 1 階段 |
| `03-object-oriented.md` 物件導向開發 | OOA → OOD、領域模型、GRASP（Larman） | 領域模型與詞彙表作為單一事實來源，沿用 `uml-and-ai-workflow.md` | 命名必須符合詞彙表；Mermaid 圖與程式碼同步 | 第 1 階段 |
| `04-design.md` 軟體設計 | 模組化、深模組、SOLID、契約式設計（Meyer） | 「程式碼庫就是最大的 prompt」；邊界要能讓機器檢查 | CMake target 依賴規則、include 限制、clang-tidy 自訂規則 | 第 0–1 階段 |
| `05-project-management.md` 專案計畫與管理 | 工作分解、估算、風險管理 | 單人加 AI 版：任務切到 agent 一次做得完的大小；context 預算 | Definition of Done 檢查清單、issue 範本 | 全階段 |
| `06-testing.md` 軟體測試 | 測試層級、TDD、覆蓋率、property-based 測試 | 測試是 AI 的回饋迴路；防止 AI 修改測試來讓它通過 | 覆蓋率門檻、mutation testing、保護測試檔的 hook | 第 1 階段起 |
| `07-quality.md` 品質管理與保證 | SQA、度量、審查、靜態分析 | 倒金字塔；把糾正轉成檢查；`code-review` skill | clang-tidy、sanitizer、ESLint、revert 率追蹤 | 全階段 |
| `08-configuration-management.md` 建構管理 | 版本控制、建構、CI/CD、發布 | 一個任務一個分支或 worktree、小 PR、可重現建構 | pre-commit hook、CI、git guardrails | 第 0 階段 |
| `09-formal-methods.md` 正規方法 | 形式規格、契約、模型檢查 | AI 讓正規方法的成本降低（Lauren Tan 觀點） | assert / 契約、TLA+ 模型檢查放進 CI | 第 3 階段（同步） |
| `10-process-improvement.md` 流程改善 | CMMI 成熟度、度量驅動改善 | 單人版：每階段回顧、lint 規則紀錄簿、改善指標 | 度量儀表（revert 率、CI 失敗原因統計） | 每階段結束 |

---

## `project/overview.md`：專案架構總覽
- **產品願景**：一段話說明我要用它做什麼（每天背英文、看到自己的進步）。
- **九個子系統**：每個子系統的職責、語言、預估行數（標明為估計）、主要驗證方式。
- **架構圖**（Mermaid）：C++ 核心、WASM 綁定、TS UI、C++ 後端之間的關係與依賴方向。
- **C++ / TS 邊界設計**：Ports & Adapters；哪些型別會跨越 WASM 邊界，以及跨越的規則。
- **專案目錄結構（monorepo）**：各目錄與 CMake target 的對應。
- **技術選型清單**：建構工具、測試框架、Web server 函式庫等，每項附選擇理由；尚未決定的標【待決】。

## `project/roadmap.md`：分階段路線圖
每個階段固定格式：目標 → 包含的子系統 → 預估行數 → 練到的章節 → **驗收機制** → 結束時的回顧項目。
- **第 0 階段：環境與護欄**：CMake、CI、clang-tidy 基線、TS 工具鏈、WASM hello world、Claude Code 設定（hooks、CLAUDE.md）。
- **第 1 階段：MVP，每天能用**：子系統 1、2、3（基本版）、8（最小 Web UI）；先只做本機儲存。
- **第 2 階段：後端與匯入**：子系統 9、5。
- **第 3 階段：多裝置同步**：子系統 4，導入 TLA+。
- **第 4 階段：語音與發音**：子系統 6。
- **第 5 階段：AI 製卡與進階分析**：子系統 7、3（進階版）。
- **第 6 階段：多平台客戶端與打磨**：子系統 8 擴充。
- **時程估算**：以每週 8 小時換算，並說明估算方法與不確定性。
- **風險清單**：例如 WASM 綁定複雜度、clang-tidy 自訂規則的學習曲線。

## `project/phase-1-mvp.md`：第一階段詳細計畫
- **MVP 範圍**：要做什麼，以及刻意不做什麼。
- **需求與驗收條件**：使用者故事加上可執行的驗收條件（連到 `notes/02`）。
- **領域模型初稿**：Card、Deck、Review、Schedule 等（Mermaid 類別圖，連到 `notes/03`）。
- **逐週計畫**：每週的任務、要學的技術（C++20 功能、TS 入門）、驗收方式。
- **第一週任務清單**：具體到可以直接開工。
- **第一批 lint 規則候選**：預期 AI 常犯的錯誤，以及對應的檢查。
- **階段結束的回顧問題**：連到 `notes/10`。
