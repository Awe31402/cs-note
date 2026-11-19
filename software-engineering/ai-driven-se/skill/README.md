# ai-driven-se 外掛（Skill 收納處）

在單字卡專案開發過程中，**值得重複使用的工作流**會整理成 Claude Code skill 放在這裡，以外掛（plugin）的形式載入。決策背景見 [D-008](../decisions.md#d-008-可重用工作流做成-skill以外掛形式載入)。

## 目錄結構

```
skill/
├── .claude-plugin/
│   ├── plugin.json        # 外掛描述檔（名稱：ai-driven-se）
│   └── marketplace.json   # 本機 marketplace（名稱：ai-driven-se-local），用於長期安裝
├── skills/
│   └── <skill-name>/
│       └── SKILL.md       # 一個工作流一個資料夾
└── README.md
```

載入後，skill 會以 `ai-driven-se:<skill-name>` 的名稱出現。

## 載入方式

**方式一：只在本次 session 載入（試用新 skill 時）**
```bash
claude --plugin-dir <path>/ai-driven-se/skill
```

**方式二：長期安裝（透過本機 marketplace）**
```bash
claude plugin marketplace add <path>/ai-driven-se/skill
claude plugin install ai-driven-se@ai-driven-se-local
```
修改 skill 之後，執行 `claude plugin marketplace update ai-driven-se-local` 與 `claude plugin update ai-driven-se@ai-driven-se-local`，並重新啟動 Claude Code。

> 專案程式碼會放在另一個 repo（x86 開發機），`<path>` 指向這個筆記 repo 的位置即可。

**驗證格式**：新增或修改 skill 後執行 `claude plugin validate <path>/ai-driven-se/skill`。

## 什麼樣的工作流值得做成 skill

符合以下**全部**條件才收進來：
1. **重複出現**：同樣的流程已經做過至少 3 次。
2. **多步驟**：不是一句話能交代完的指令。
3. **有驗收方式**：skill 內寫明怎樣算做完、用什麼檢查確認。
4. **無法改成確定性檢查**：如果能寫成 lint 規則、hook 或 CI 檢查，就優先那樣做（倒金字塔原則：skill 屬於第 4 層「文字規範」，投入順序最後）。

候選來源：每個階段結束時的回顧（見 [`project/roadmap.md`](../project/roadmap.md)）。

## Skill 清單

| Skill | 用途 | 來自哪個階段 | 加入日期 |
|---|---|---|---|
| （尚無） | | | |
