# 品質策略：倒金字塔在本專案的具體化

> 決策背景見 [D-006](./decisions.md#d-006-品質策略倒金字塔單人縮小版)。本文件說明每一層在 C++ 與 TypeScript 上**具體用什麼工具、檢查什麼、在哪個階段導入**。
>
> 標記說明：【待決】＝工具尚未選定，於指定階段決定；【待查】＝事實尚未查證。

---

## 一、Lauren Tan 的方法摘要

> ⚠️ 來源：第三方摘要（[TeamDay](https://www.teamday.ai/ai/lauren-tan-spacexsi-grok-bot-11000-agent-prs-lint-rules)、[AI Engineer Guide](https://aiengineerguide.com/til/all-code-is-slop-except-mine-by-lauren-tan/)），未看過演講原始內容。

她把工程時間分配畫成**倒金字塔**，越上層投入越多：

```
┌───────────────────────────────────────────┐
│ 1. 程式碼本身：架構、API、型別、邊界       │  ← 投入最多
├───────────────────────────────────────┤
│ 2. 靜態分析：lint、型別、測試、CI      │
├───────────────────────────────────┤
│ 3. 驗證：runtime、property、formal │
├───────────────────────────────┤
│ 4. 文字規範：AGENTS.md、skill  │  ← 投入最少
└───────────────────────────────┘
```

三個核心觀念：
1. **每次糾正 AI，就把那次糾正變成一個確定性的檢查。** lint 規則或 API 修改可以重現；AGENTS.md 多一行不能。更好的做法是**從設計上讓錯誤的寫法根本寫不出來**。
2. **程式碼庫就是最大的 prompt。** AI 會模仿它在程式碼庫裡看到的模式，包括錯誤的模式，所以舊的壞模式也要清掉。
3. **人工審查轉向驗證產物。** 人不再逐行讀程式碼，而是看錄影、截圖、圖表、驗證結果。

---

## 二、第 1 層：程式碼本身

目標：**讓錯誤的寫法在結構上不可能，或在編譯期就失敗。**

### C++
| 手段 | 用途 | 範例 |
|---|---|---|
| **強型別取代原始型別** | 避免參數搞混 | `CardId`、`DeckId` 用不同型別，不都用 `int64_t`；時間間隔用 `std::chrono::duration` 而非 `double` |
| **concepts（C++20）** | 模板誤用在編譯期失敗 | 排程演算法要求參數符合 `Scheduler` concept |
| **不變式封裝在建構函式** | 無效狀態無法被建立 | `Rating` 只能由 `Again / Hard / Good / Easy` 建立；難度值的範圍由型別保證 |
| **值語意與 `const` 預設** | 降低共享可變狀態 | 領域物件預設不可變，修改產生新值 |
| **錯誤處理型別化** | 錯誤不能被默默忽略 | 用 `std::expected`（C++23）或自訂 `Result` 型別，【待決】於 P0；搭配 `[[nodiscard]]` |
| **CMake target 劃分邊界** | 模組依賴方向由建置系統強制 | `core` 不得連結 `server`、`wasm` 等外層 target（見 [`project/overview.md`](./project/overview.md)） |

### TypeScript
| 手段 | 用途 |
|---|---|
| `strict: true` 與 `noUncheckedIndexedAccess` | 抓出 `undefined` 相關錯誤 |
| 判別聯集（discriminated union） | UI 狀態（載入中 / 成功 / 失敗）不會出現不可能的組合 |
| UI 層不含商業邏輯 | 排程、統計一律呼叫 C++ 核心，TS 只負責顯示，降低 TS 需要測試的範圍 |

### C++ ↔ TS 邊界
- 跨越 WASM 邊界的型別保持**簡單且穩定**（純資料結構），細節見 [`project/overview.md`](./project/overview.md#c--ts-邊界設計)。
- 邊界介面由 C++ 端定義，TS 型別定義**自動產生**或由測試檢查一致性（方案【待決】於 P0）。

---

## 三、第 2 層：靜態分析與測試

### C++
| 工具 | 設定方向 | 導入階段 |
|---|---|---|
| 編譯器警告 | 高警告等級（例如 `-Wall -Wextra -Wpedantic` 等），**警告視為錯誤** | P0 |
| **clang-tidy** | 以 `cppcoreguidelines-*`、`bugprone-*`、`modernize-*`、`performance-*` 等檢查群組為基線，逐條評估是否保留 | P0 |
| **clang-tidy 自訂檢查** | 專案特有規則（例如核心不得 include 平台標頭）；以 clang-query 先驗證 AST matcher，再寫成外掛模組（載入方式與版本支援【待查】） | P0 起逐步累積 |
| clang-format | 格式統一，CI 檢查 | P0 |
| **Sanitizer** | ASan（記憶體錯誤）、UBSan（未定義行為）、TSan（資料競爭，P2 後端起） | P0 / P2 |
| 依賴邊界檢查 | CMake target 之外，再用腳本或工具檢查 `#include` 方向（工具【待決】） | P0 |

### TypeScript
| 工具 | 設定方向 | 導入階段 |
|---|---|---|
| `tsc --noEmit` | 型別檢查 | P0 |
| **ESLint** | 推薦規則集加型別感知規則；專案自訂規則（例如 UI 元件不得直接操作儲存層） | P0 起逐步累積 |
| Prettier | 格式統一 | P0 |

### 測試
各層測試負責的範圍：

| 測試類型 | 負責什麼 | C++ 工具 | TS 工具 |
|---|---|---|---|
| 單元測試 | 單一函式或類別的行為 | GoogleTest 或 Catch2【待決】 | Vitest【待決】 |
| **property-based 測試** | 對任意輸入都成立的性質（排程、同步的核心） | RapidCheck【待決】 | fast-check【待決】 |
| golden test | 固定輸入對應固定輸出（統計、匯入） | 同單元測試框架 | — |
| 對照測試 | 與參考實作比對（FSRS） | 同單元測試框架 | — |
| E2E 測試 | 使用者主流程 | — | Playwright |
| mutation testing | 檢查測試本身是否夠強 | Mull【待查可用性】 | Stryker |

詳見 [06 軟體測試](./notes/06-testing.md)。

### CI 關卡（會擋下合併的檢查）
```
PR → 格式檢查 → 建置（警告即錯誤）→ clang-tidy / ESLint → 依賴邊界檢查
   → 單元 + property + golden 測試 → sanitizer 建置的測試 → E2E 測試 → 可合併
```
- 每個關卡失敗都會擋下合併，**沒有「先合併再修」的例外**。
- CI 平台【待決】於 P0（專案 repo 放在哪裡會影響選擇）。

---

## 四、第 3 層：驗證

| 手段 | 驗證什麼 | 導入階段 |
|---|---|---|
| **AI 操作 app 並錄影**（Playwright） | 主流程在真實瀏覽器中是否正常；人看錄影而不是讀程式碼 | P1 |
| 截圖比對 | UI 意外變動 | P1 或 P6【待決】 |
| **TLA+ 模型檢查** | 同步協定的安全性與收斂性 | P3 |
| 效能預算測試 | 啟動時間、換卡延遲不退步 | P6 |
| LLM 輸出評估集 | AI 製卡的品質 | P5 |

詳見 [09 正規方法](./notes/09-formal-methods.md)。

---

## 五、第 4 層：文字規範

**原則：CLAUDE.md 只放無法機器檢查的判斷。**

| 放進 CLAUDE.md | 不放進 CLAUDE.md（改用檢查） |
|---|---|
| 專案目的與架構概觀（一段話加連結） | 命名規則 → clang-tidy `readability-identifier-naming` |
| 「不確定時先問，不要猜」這類工作方式 | 禁止某些 API → 自訂 lint 規則 |
| 何時要新增 ADR | 模組依賴方向 → CMake 加依賴檢查 |
| 不得複製 Anki 程式碼（[D-007](./decisions.md#d-007-不複製-anki-原始碼)） | 格式 → clang-format / Prettier |

**控制篇幅的方法**
- 每新增一行之前先問：這能不能變成檢查？
- 每個階段回顧時檢視 CLAUDE.md，把能轉成檢查的移出去。
- 可重用的多步驟工作流整理成 skill，放在 [`skill/`](./skill/README.md)（[D-008](./decisions.md#d-008-可重用工作流做成-skill以外掛形式載入)）。

---

## 六、lint 規則紀錄簿（Correction Log）

每次糾正 AI 都要記錄。這是第 10 章「流程改善」的原始資料。

**位置**：專案 repo 的 `docs/correction-log.md`（建立於 P0）。

**格式**
```markdown
## CL-001 YYYY-MM-DD
- **AI 犯的錯**：在 core 裡 include 了 <emscripten.h>
- **為什麼會犯**：程式碼庫裡已有一處類似寫法被它模仿
- **轉成的檢查**：clang-tidy 自訂檢查 `project-core-no-platform-headers`
- **規則位置**：tools/clang-tidy/CoreNoPlatformHeaders.cpp
- **同時清理**：移除既有的違規寫法 2 處
- **無法轉成檢查時**：寫明原因，並記錄放進了 CLAUDE.md 的哪一行
```

**每階段回顧時統計**：新增幾條？哪一類錯誤最多？有幾條只能放進 CLAUDE.md？

---

## 七、防止 AI 作弊

AI 在 CI 失敗時，可能選擇讓檢查通過，而不是修正問題。常見手法與對策：

| 作弊手法 | 偵測或阻止方式 |
|---|---|
| 修改或刪除測試，讓測試通過 | Claude Code `PreToolUse` hook 攔截對測試目錄的修改（需要我明確同意才放行）；CI 比對測試數量不得減少 |
| 加 `// NOLINT`、`eslint-disable` | CI 偵測新增的抑制註解，要求附理由，並在 PR 中標出供人審查 |
| 修改 lint 或編譯器設定，關閉檢查 | 設定檔列為受保護檔案，修改時由 hook 攔截 |
| 降低覆蓋率門檻 | 門檻寫在 CI 設定中，同樣受保護 |
| 用 `try/catch` 或預設值吞掉錯誤 | 自訂 lint 規則禁止空的 catch；`[[nodiscard]]` 搭配 `Result` 型別 |
| 寫只驗證「有執行」不驗證結果的測試 | mutation testing（P1 起）；code review 檢查 |
| 把失敗的測試標為 skip | CI 偵測 skip 標記數量增加 |

> hook 的具體寫法屬於專案程式碼，於 P0 實作（見 [`project/roadmap.md`](./project/roadmap.md#p0環境與護欄)）。

---

## 相關筆記
- [07 品質管理與保證](./notes/07-quality.md)：本策略的理論基礎
- [06 軟體測試](./notes/06-testing.md)：測試層級與 TDD
- [04 軟體設計](./notes/04-design.md)：第 1 層的設計原則
