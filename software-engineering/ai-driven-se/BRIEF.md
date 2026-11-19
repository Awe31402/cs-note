# 任務說明（Brief）：AI 自動化開發 × 軟體工程

> 2026-11-19 透過 `/define-task` 逐欄確認。本文件是後續所有產出（筆記、計畫）與交給 AI agent 的 prompt 的依據。

---

## 1. Purpose（目的）

建立一份**「軟體工程知識筆記 × 專案實戰計畫」**（同一套檔案、互相連結），讓我能用 AI agent 自動化開發一個**約十萬行的英文單字卡平台**，並學會：

> **當專案大到 AI 無法一次掌握時，如何用軟體工程原則讓 AI 持續可靠地開發。**

「十萬行」不是目標本身，而是代表一個規模：AI 只能看到局部，所以模組邊界、測試、需求文件、審查關卡都會變得必要。

**成功標準**：第一階段結束時，我每天真的在用這個 app 背英文，而且能說出每一條軟體工程原則在專案裡由哪一個自動化檢查守住。

---

## 2. Situation（情境）

### 開發者
- 單人開發，搭配 AI agent（Claude Code）。李允中第 5 章（專案管理）、第 10 章（CMMI）要轉換成**單人加 AI** 的版本。
- 背景偏嵌入式與系統（OS、RISC-V、secure boot、ISO 26262、C）。
- **主要學習目標之一是把 C++ 練好**。Web 經驗不多，TypeScript 和前端從基礎學起，但只學到 UI 層夠用的程度。
- 每週投入約 **8 小時**。十萬行以年為單位，第一階段 MVP 預估約 3 個月。

### 已定案的決策
| 項目 | 決策 |
|---|---|
| 專案類型 | E 類產品型應用：英文數位單字卡，並追蹤學習進度 |
| 範圍 | 九個子系統全部保留，分階段開發 |
| 技術棧（方案 B+） | **C++20** 負責 UI 以外的所有部分（約 70–80%）；**TypeScript** 只負責 UI，透過 WASM 呼叫 C++ 核心 |
| 品質策略 | Lauren Tan 倒金字塔（程式碼設計 → lint、型別、測試 → 驗證 → 文字規範），採單人縮小版 |
| 開發平台 | x86 電腦（不需要考慮編譯效能） |

### 九個子系統
1. 間隔重複排程引擎（FSRS / SM-2）
2. 卡片與牌組領域模型
3. 學習進度追蹤與分析
4. 多裝置離線同步（CRDT / 事件溯源）
5. 內容匯入匯出（Anki `.apkg`、CSV、字典）
6. 語音與發音（TTS、錄音、發音比對）
7. AI 輔助製卡（LLM 產生例句、解釋）
8. 客戶端 UI（TypeScript）
9. 後端 API、帳號、儲存

### 既有基礎（沿用，不要重做）
- [`../recommended-books.md`](../recommended-books.md)：以李允中十章為骨架，對照經典英文書。
- [`../uml-and-ai-workflow.md`](../uml-and-ai-workflow.md)：OOAD 推導流程，以 StarUML 和 Mermaid 作為約束 AI 的單一事實來源。
- Claude Code skill：`tdd`、`code-review`、`domain-modeling`、`codebase-design`。
- 筆記使用繁體中文 Markdown。

### 方法論前提
- Lauren Tan 的成果是**一個團隊花六個月**建出來的。單人版要縮小規模：lint 規則（C++ 用 clang-tidy，TS 用 ESLint）從第一天開始累積；TLA+ 等正規驗證留到同步子系統再導入。
- 核心原則：**每次糾正 AI，就把那次糾正變成一個確定性的檢查**，不要只在 AGENTS.md 多加一行。

---

## 3. Inputs（資料來源）

### 一級來源
1. **李允中《軟體工程》十章架構**：**只當目錄骨架**（手上沒有這本書）。各章內容以 Sommerville、Pressman 為主要依據。
2. **`../books/` 本地 PDF**：依照 `recommended-books.md` 的章節對應引用。
3. **既有筆記**：只能連結，不重寫重複內容。
4. **官方文件**：cppreference、C++ Core Guidelines、clang-tidy、CMake、Emscripten、FSRS（open-spaced-repetition）、CRDT 論文 / Automerge、TLA+、Claude Code 官方文件。

### 二級來源（參考，不當作定論）
- Lauren Tan〈All Code Is Slop (Except Mine)〉：目前只有第三方摘要（[TeamDay](https://www.teamday.ai/ai/lauren-tan-spacexsi-grok-bot-11000-agent-prs-lint-rules)、[AI Engineer Guide](https://aiengineerguide.com/til/all-code-is-slop-except-mine-by-lauren-tan/)）。取得逐字稿後升級為一級來源。
- 業界部落格（例如 [Factory.ai: using linters to direct agents](https://www.factory.ai/using-linters-to-direct-agents)）。引用其中的數據時要標明「來自第三方，未經查證」。

### 禁止
- **不得複製 Anki 原始碼**（AGPL 授權）。可以參考 `.apkg` 檔案格式做相容匯入；FSRS 參考實作是 MIT 授權，參考時要註明出處。
- 不得只憑 AI 記憶寫出書籍頁碼、章節編號、演算法參數。

### 找不到資料時
- **留空並標記 `【待查】`，不要編造。**

---

## 4. Limits（範圍界線）

- **不要碰**：既有筆記內容（只能連結或加反向連結）、`books/`、其他領域的資料夾。
- **不要假設**：熟悉 Web；有團隊；AI 產出一定正確（每個階段都必須有自動化驗收機制）。
- **不要提議**：重新討論已定案的決策；換技術棧（發現重大問題時標記出來讓我決定）；第一階段就導入正規驗證；商業化與營運。
- **這份產出不包含**：專案程式碼；各本書的完整讀書心得。

---

## 5. Done-when（完成條件）

### 產出位置與結構
```
ai-driven-se/
├── BRIEF.md                   # 本文件
├── README.md
├── decisions.md
├── quality-strategy.md
├── notes/01-…10-*.md          # 十章筆記
├── project/
│   ├── overview.md
│   ├── roadmap.md
│   └── phase-1-mvp.md
└── skill/                     # Claude Code 外掛：開發中提煉出的可重用工作流（2026-11-19 追加）
```

### 追加要求（2026-11-19）
- 開發過程中值得做成 skill 的工作流，放在 `skill/` 底下，以 Claude Code **外掛（plugin）**形式載入。詳見 [D-008](./decisions.md#d-008-可重用工作流做成-skill以外掛形式載入) 與 [`skill/README.md`](./skill/README.md)。

### 完成的判斷標準
1. 每章筆記都包含：**原則 → AI 開發中如何落地 → 對應的自動化檢查 → 專案連結**。
2. 每個專案階段都寫明自動化驗收機制，沒有任何一步只寫「請 AI 實作」。
3. 筆記與專案計畫雙向連結。
4. 所有引用都有出處；找不到出處的地方標記【待查】。
5. `phase-1-mvp.md` 詳細到讀完就知道第一週要做什麼。

### 檢查點
1. **大綱**（[`OUTLINE.md`](./OUTLINE.md)）：目錄結構、每個檔案的標題層級、每節一句話說明
2. **核心三檔**：`README.md`、`decisions.md`、`project/roadmap.md`
3. **其餘全部**
