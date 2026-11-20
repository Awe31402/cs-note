# 06 軟體測試

> 李允中《軟體工程》第 6 章（僅作目錄）。主要依據：Sommerville 9e Ch.8、Pressman 9e Ch.19–20、《Software Engineering at Google》Ch.11–14、Ammann & Offutt《Introduction to Software Testing》。

---

## ① 核心原則

| 原則 | 說明 | 出處 |
|---|---|---|
| 測試層級 | 開發測試（單元、元件、系統）、發布測試、使用者測試 | Sommerville Ch.8 §8.1、§8.3、§8.4 |
| 測試驅動開發（TDD） | 先寫失敗的測試，再寫剛好讓它通過的程式碼，然後重構 | Sommerville Ch.8 §8.2 |
| 測試規模與金字塔 | 多數是小而快的測試，少數是大而慢的測試 | Google Ch.11 |
| 單元測試要測行為，不要測實作 | 測試應透過公開介面驗證行為，重構時才不會大量失敗 | Google Ch.12 |
| 測試替身要謹慎使用 | 能用真實實作就用真實實作；過度 mock 會讓測試失去意義 | Google Ch.13 |
| 覆蓋率準則 | 圖形覆蓋、邏輯覆蓋、輸入空間切分、語法式測試 | Ammann & Offutt Ch.2–5 |
| Mutation testing | 故意在程式中植入小錯誤，檢查測試能否抓到，用來衡量測試本身的強度 | Ammann & Offutt §5.1.2 |
| AI 與回歸測試 | Pressman 9e 已有專節討論 AI 用於回歸測試 | Pressman Ch.20 §20.3 |

---

## ①-1 原文引用

### 測試驅動開發（TDD）
> **原文引用（Sommerville, Software Engineering 9e, Ch.8 §8.2, p.222）**
> "You then run the test, along with all other tests that have been implemented. Initially, you have not implemented the functionality so the new test will fail. This is deliberate as it shows that the test adds something to the test set. 4. You then implement the functionality and re-run the test. This may involve refactoring existing code to improve it and add new code to what's already there."

→ 先跑一個必定失敗的測試（證明它真的測到新東西），再實作讓它通過，過程中可能伴隨重構。

### 測試規模與金字塔
> **原文引用（Winters, Manshreck & Wright, Software Engineering at Google, Ch.11, p.220）**
> "As a very rough guideline, we tend to aim to have a mix of around 80% of our tests being narrow-scoped unit tests that validate the majority of our business logic; 15% medium-scoped integration tests that validate the interactions between two or more components; and 5% end-to-end tests that validate the entire system."

→ Google 的經驗比例約為 80% 單元測試、15% 整合測試、5% 端對端測試，形成金字塔。

### 單元測試要測行為，不要測實作
> **原文引用（Winters, Manshreck & Wright, Software Engineering at Google, Ch.12, p.234）**
> "By far the most important way to ensure this is to write tests that invoke the system being tested in the same way its users would; that is, make calls against its public API rather than its implementation details. If tests work the same way as the system's users, by definition, change that breaks a test might also break a user."

→ 測試要像使用者一樣透過公開 API 呼叫系統；這樣測試壞掉時，代表使用者也可能受影響，而不是因為內部重構。

### 測試替身要謹慎使用
> **原文引用（Winters, Manshreck & Wright, Software Engineering at Google, Ch.13, p.265）**
> "At Google, the preference for real implementations developed over time as we saw that overuse of mocking frameworks had a tendency to pollute tests with repetitive code that got out of sync with the real implementation and made refactoring difficult."

→ Google 偏好真實實作，因為過度使用 mock 會讓測試充滿與真實實作脫節的重複程式碼，反而讓重構更困難。

### 覆蓋率準則
> **原文引用（Ammann & Offutt, Introduction to Software Testing 1e, Ch.1, p.4）**
> "Much to our surprise, we have found that a small number of abstract models suffice: graphs, logical expressions, input domain characterizations, and syntactic descriptions. The main contribution of this book is to simplify testing by classifying coverage criteria into these four categories, and this is why Part II of this book has exactly four chapters."

→ 作者把所有覆蓋率準則歸入四種抽象模型，也就是 Part II 的 Ch.2–5：圖形、邏輯運算式、輸入領域、語法。

### Mutation testing
> **原文引用（Ammann & Offutt, Introduction to Software Testing 1e, Ch.5 §5.1.2, p.175）**
> "Thus, coverage in mutation equates to killing the mutants. The amount of coverage is usually written as a percent of mutants killed and called the mutation score."

→ 突變測試的覆蓋率就是「殺死」突變體的比例（mutation score）：測試能讓多少個植入錯誤的版本產生不同輸出。

### AI 與回歸測試
> **原文引用（Pressman & Maxim, Software Engineering 9e, Ch.20 §20.3, p.403）**
> "Yoo and Harman [Yoo13] write about potential uses of artificial intelligence (AI) in identifying test cases for use in regression test suites. ... This work is promising, but still requires significant human interaction to review the test cases and the recommended order for executing them."

→ Pressman 提到 AI 可以用來挑選或產生回歸測試案例，但也指出這仍需要大量人工審查；書中對此著墨只有一段。

---

## ② AI 開發中如何落地

**測試是 AI 唯一可靠的回饋迴路。** AI 寫完程式碼後跑測試，通過就繼續，失敗就修正。測試的品質直接決定 AI 產出的品質。

### TDD 對 AI 特別重要

![AI 時代的 TDD](../diagrams/tdd-ai.png)

<sub>圖源：[`diagrams/src/tdd-ai.puml`](../diagrams/src/tdd-ai.puml)</sub>

1. **我（或 AI 在我審查下）先寫失敗的測試**，測試本身就是規格。
2. AI 實作，直到測試通過。
3. AI 重構，測試保證行為不變。

如果讓 AI 同時寫程式和測試，它很容易寫出「配合實作」的測試，而不是驗證需求的測試。

可直接使用的 Claude Code skill：`tdd`（red-green-refactor 流程）。

### 本專案各類測試的分工
| 測試類型 | 適用子系統 | 理由 |
|---|---|---|
| property-based | 排程（1）、同步（4） | 這兩者的正確性是「對任何輸入都成立的性質」 |
| 對照測試 | 排程（1） | 有 FSRS 參考實作可以比對 |
| golden test | 分析（3）、匯入（5）、語音（6） | 固定輸入有確定輸出 |
| E2E 加錄影 | UI（8） | 主觀部分交給人看錄影 |
| fuzzing | 匯入（5）、API（9） | 處理外部輸入，要能承受任何格式錯誤的資料 |

### 防止 AI 對測試作弊
AI 可能修改測試讓它通過、把測試標為 skip，或寫只檢查「有執行」的測試。對策見 [`quality-strategy.md#七防止-ai-作弊`](../quality-strategy.md#七防止-ai-作弊)。**Mutation testing 是抓出弱測試的客觀方法。**

---

## ③ 對應的自動化檢查

| 檢查 | 做法 |
|---|---|
| 所有測試在 CI 中執行 | 失敗就擋下合併 |
| 覆蓋率門檻 | 核心模組設定門檻（數值於 P1 依實際情況決定），只能升不能降 |
| **mutation testing** | C++ 用 Mull（可用性【待查】）、TS 用 Stryker；每階段記錄分數 |
| 測試檔保護 | hook 攔截 AI 修改既有測試；CI 檢查測試數量不得減少、skip 不得增加 |
| 測試不得依賴真實時間或亂數 | lint 規則（[`phase-1-mvp.md#第一批-lint-規則候選`](../project/phase-1-mvp.md#第一批-lint-規則候選)） |

---

## ④ 專案連結

- **P1**：排程的 property-based 與對照測試、E2E 主流程（[`phase-1-mvp.md`](../project/phase-1-mvp.md)）
- **P2**：API contract 測試、fuzzing
- **P3**：同步的 property-based 測試與故障注入
- **P4–P6**：golden 音檔、LLM 回應錄製、效能預算測試
- 各階段驗收清單：[`roadmap.md`](../project/roadmap.md)
