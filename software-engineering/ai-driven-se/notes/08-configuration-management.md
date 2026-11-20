# 08 軟體建構管理

> 李允中《軟體工程》第 8 章（僅作目錄）。主要依據：Sommerville 9e Ch.25、Pressman 9e Ch.22、《Software Engineering at Google》Ch.16、18、21、23、24。

---

## ① 核心原則

| 原則 | 說明 | 出處 |
|---|---|---|
| 建構管理四大活動 | 變更管理、版本管理、系統建置、發布管理 | Sommerville Ch.25 §25.1–25.4 |
| 變更管理流程 | 變更要經過評估、核准、追蹤 | Sommerville Ch.25 §25.1；Pressman Ch.22 §22.5 |
| 版本控制 | 所有產出物都受版本控制 | Pressman Ch.22 §22.3；Google Ch.16 |
| 持續整合 | 頻繁整合並自動驗證 | Pressman Ch.22 §22.4；Google Ch.23 |
| 可重現的建置 | 相同輸入一定產生相同輸出 | Google Ch.18 |
| 依賴管理 | 外部依賴要有明確版本與更新策略 | Google Ch.21 |
| 持續交付 | 隨時可以發布 | Google Ch.24 |

---

## ①-1 原文引用

### 變更管理流程
> **原文引用（Sommerville, Software Engineering 9e, Ch.25 §25.1, p.685）**
> "The change management process is concerned with analyzing the costs and benefits of proposed changes, approving those changes that are worthwhile, and tracking which components in the system have been changed."

→ 變更管理的三個動作正是：分析成本效益（評估）、核准值得做的變更、追蹤哪些元件被改過。

### 版本控制
> **原文引用（Pressman & Maxim, Software Engineering 9e, Ch.22 §22.3, p.445）**
> "Version control combines procedures and tools to manage different versions of configuration objects that are created during the software process. A version control system implements or is directly integrated with four major capabilities: (1) a project database (repository) that stores all relevant configuration objects, (2) a version management capability that stores all versions of a configuration object ..."

→ 版本控制管理的是軟體流程中產生的所有「組態物件」，不只是程式碼，而且保存每個物件的所有版本。

### 持續整合
> **原文引用（Pressman & Maxim, Software Engineering 9e, Ch.22 §22.4, p.446）**
> "CI also adds value to SCM by ensuring that each change is promptly integrated into the project source code, compiled, and tested automatically."

→ 每個變更都立即整合、自動編譯並測試，這就是「頻繁整合並自動驗證」。

### 可重現的建置
> **原文引用（Winters, Manshreck & Wright, Software Engineering at Google, Ch.18, p.387）**
> "For a remote caching system to work, the build system must guarantee that builds are completely reproducible. That is, for any build target, it must be possible to determine the set of inputs to that target such that the same set of inputs will produce exactly the same output on any machine."

→ 可重現的定義：任何建置目標都能確定其完整輸入，而且相同輸入在任何機器上都產生完全相同的輸出。

### 依賴管理
> **原文引用（Winters, Manshreck & Wright, Software Engineering at Google, Ch.21, p.429）**
> "Dependency management—the management of networks of libraries, packages, and dependencies that we don't control—is one of the least understood and most challenging problems in software engineering. Dependency management focuses on questions like: how do we update between versions of external dependencies? How do we describe versions, for that matter?"

→ 依賴管理處理的是「我們無法控制的程式碼」，核心問題就是如何描述版本、如何在版本之間更新。

### 持續交付
> **原文引用（Winters, Manshreck & Wright, Software Engineering at Google, Ch.24, p.507）**
> "In the limit, with CD, every change goes through the QA pipeline and is automatically deployed into production. This is often not a practical reality for many teams, and so there is often work of culture change toward CD as an intermediate step, during which teams can build their readiness to deploy at any time without actually doing so, building up their confidence to release more frequently in the future."

→ CD 的極致是每個變更都自動部署；在做到之前，先培養「隨時可以部署」的能力，即使實際上還沒這樣做。

---

## ② AI 開發中如何落地

| 原則 | AI 開發中的做法 |
|---|---|
| 變更管理 | **一個任務一個分支（或 git worktree）**，AI 的修改範圍被限制在一個 PR 內 |
| 小變更 | PR 越小，自動化檢查越容易定位問題，人審查也越快 |
| 版本控制一切 | 不只程式碼：需求、ADR、CLAUDE.md、lint 規則、TLA+ 規格、skill 都在 git 中 |
| 可重現建置 | AI 在任何時候執行建置都得到相同結果，CI 失敗時才能確定是程式碼的問題 |
| 依賴管理 | 依賴版本鎖定；AI 不得自行新增依賴（需要我同意） |

![一個任務一個分支](../diagrams/branch-per-task.png)

<sub>圖源：[`diagrams/src/branch-per-task.puml`](../diagrams/src/branch-per-task.puml)</sub>

**單人版的變更管理**：不需要 CCB（變更控制委員會），但需要：
- 需求變更走 PR，並更新需求文件。
- 架構變更要新增 ADR（[`decisions.md`](../decisions.md)）。

**防止破壞性操作**：AI 可能執行 `git push --force`、`git reset --hard` 等指令。可直接使用的 Claude Code skill：`git-guardrails-claude-code`（以 hook 阻擋危險的 git 指令）。

---

## ③ 對應的自動化檢查

| 檢查 | 做法 |
|---|---|
| CI | 每個 PR 都跑完整檢查（[`quality-strategy.md#ci-關卡會擋下合併的檢查`](../quality-strategy.md#ci-關卡會擋下合併的檢查)） |
| pre-commit hook | 本機提交前先跑格式檢查與快速測試（可參考 `setup-pre-commit` skill 的做法，但本專案是 C++ 加 TS，需調整） |
| git guardrails | Claude Code hook 阻擋危險 git 指令 |
| 依賴變更偵測 | CI 標記修改依賴清單的 PR，要求人審查 |
| 受保護檔案 | lint 設定、CI 設定、hook 本身的修改需要我明確同意 |

---

## ④ 專案連結

- **P0**：repo、CI、hooks、依賴管理工具（[`roadmap.md#p0`](../project/roadmap.md#p0環境與護欄)）
- **P2**：server 部署
- **P6**：發布流程
- skill 外掛也有版本號：[D-008](../decisions.md#d-008-可重用工作流做成-skill以外掛形式載入)
