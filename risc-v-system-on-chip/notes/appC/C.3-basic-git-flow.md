# C.3 Basic Git Flow

> 原書 p.784.e172–784.e175（Appendix C: Version Control Using Git）

## 這節在講什麼

一次工作循環從頭到尾：拉最新的 → 改 → 檢查 → 暫存 → 提交 → 推送 → 發 PR。這是每天都會重複的流程。

## 關鍵點

### 開工前先拉

> "Before starting a work session, it's a good idea to make sure you have the latest version of the repository: $ git pull"
> — p.784.e172

> "If you are working on a forked repository, you can pull in changes from the upstream repo using: $ git pull upstream main"
> — p.784.e173

中文解釋：**兩種 pull 要分清楚**：
- `git pull` — 從 `origin`（自己的 fork）拉。多台電腦工作時用
- `git pull upstream main` — 從主倉庫拉別人的改動。**這是避免衝突的關鍵習慣**（C.1 講的「定期同步」）

### `git status` 是最常打的指令

> "Check that you don't have any modified files from a previous session: $ git status"
> — p.784.e173

中文解釋：**開工前打一次、提交前打一次**。它會把檔案分成三類：
- **Changes not staged for commit** — 改過但還沒 `git add`
- **Untracked files** — 新檔案，Git 還不認識它
- **Changes to be committed** — 已 `git add`，等著 commit

### 改檔案：用 `git mv` 而不是 `mv`

> "To move or rename a file, use: $ git mv <old name> <new name>"
> — p.784.e173

中文解釋：**用一般的 `mv` 會讓 Git 以為「刪了一個檔、加了一個新檔」**，歷史就斷了。`git mv` 讓 Git 知道這是同一個檔案。

但即使如此還是有限制：

> "However, when you ask Git for the history of this file using git log or git diff, it will only show the history up to the last move. To get the full history, use: $ git log --follow <new name>"
> — p.784.e173

中文解釋：**`--follow` 才能看到改名前的歷史**。這是一個很容易漏掉的旗標。

### 提交前先看差異

> "You can list the changes in a file using: $ git diff <file>"
> — p.784.e173

> "If any of the modifications are bad, you can roll back to the version in the repository using: $ git restore <file>"
> — p.784.e173

中文解釋：**`git diff` 是提交前的最後一道防線**——常常會發現自己留了 debug 用的 `$display` 或註解掉的程式碼。

`git restore <file>` 丟掉一個檔案的所有未暫存改動。**這個操作不可逆**（改動沒有被記錄在任何地方）。

### 暫存、刪除、反悔

> "When you are satisfied, stage the files to be committed: $ git add <file>"
> — p.784.e173

> "If you've deleted any files from the project, remove them from the repository as well. This only removes them from future checkpoints; you can always roll back to an earlier version where the file existed. $ git rm <file>"
> — p.784.e174

中文解釋：**`git rm` 只影響「未來的快照」，歷史裡的檔案永遠還在**——這正是 B.2 說的「Linux 沒有還原指令，除非你用版本控制」。

> "If you change your mind after staging a file, you can take it out using: $ git restore --staged <file>"
> — p.784.e174

中文解釋：`git restore --staged` 把檔案移出暫存區但**保留改動**（回到 modified 狀態）。C.8 會有完整的狀態圖。

### 提交：訊息要有意義

> "$ git commit -m 'Fixed undeclared mmu/PhysAdr signal causing X in simulation'"
> — p.784.e174

中文解釋：**這個範例訊息本身就是好示範**——說明了「改了什麼」和「為什麼」（哪個訊號、造成什麼問題）。對照 `git log` 的輸出：

```
commit 14d3059433e212205ebf30b64ffe71d467dabb94
Author: David Harris <david_harris@hmc.edu>
Date:   Fri Jan 21 00:12:14 2022 +0000

    Fixed path to riscvOVPsimPlus
```

> "The commit creates a snapshot of the repository at that instant. The commit is identified by a hash, which is a unique 40-digit hexadecimal number."
> — p.784.e174

中文解釋：注意 commit 記錄的三樣東西——**hash、作者與時間、訊息**。作者資訊來自 C.1 設的 `user.name`/`user.email`。

**一個省時間的捷徑**：

> "Sometimes it is tedious to git add files one-by-one. git commit -a -m 'message' will add all the tracked files and then commit in one fell swoop."
> — p.784.e174（邊欄）

中文解釋：**`-a` 只會加「已追蹤」的檔案**——新檔案（untracked）還是要手動 `git add`。這個區別常被誤解。

### 推送前先測試

> "Always test your code before returning it to the remote repository so you don't break things for anyone else. Then push it: $ git push"
> — p.784.e174

中文解釋：**「不要弄壞別人的東西」**——在共用專案裡這是基本禮貌。對 Wally 來說，`regression-wally`（附錄 A.4）就是這個測試。

> "If anyone else has made changes, you'll have to pull their changes again before you can push. If you're working from your own fork, nobody else will be making changes to your fork, but it is prudent to do a git pull upstream main before pushing to collect any changes from the upstream repository. Git will automatically merge the changes if possible."
> — p.784.e174

中文解釋：**push 被拒絕 = 遠端有你沒有的 commit**。解法永遠是先 pull 再 push。

### 發 pull request

> "To get your commits back into the upstream repository, make a pull request. Only make pull requests for changes that are important for all users to receive in the upstream repository."
> — p.784.e175

中文解釋：**「只為所有人都需要的改動發 PR」**——自己實驗用的修改留在 fork 裡就好。

網頁操作：到自己的 fork → 通常會看到 Compare & pull request 按鈕；沒有的話走 Pull Requests → New pull request → base 選 `openhwgroup/cvw`、HEAD 選自己的 fork → 寫標題與說明 → Create pull request。

> "A committer will review your request and approve it, request changes, or deny and close the request."
> — p.784.e175

中文解釋：三種結果。被要求修改是常態，不是失敗。

### 完整流程速記

```bash
git pull upstream main       # 1. 同步主倉庫
git status                   # 2. 確認乾淨
# ... 編輯、測試 ...
git status                   # 3. 看改了什麼
git diff <file>              # 4. 逐檔檢查
git add <file>               # 5. 暫存
git commit -m "說明"          # 6. 提交
# ... 跑回歸測試 ...
git pull upstream main       # 7. 再同步一次
git push                     # 8. 推到自己的 fork
# 9. 在網頁上發 PR
```

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| `git pull` | 從遠端取得並合併改動 |
| `git status` | 顯示工作目錄與暫存區的狀態 |
| `git mv` | 移動/重新命名並讓 Git 知道 |
| `git log --follow` | 追溯改名前的歷史 |
| `git diff` | 顯示未暫存的改動 |
| `git restore <file>` | 丟棄未暫存的改動（不可逆） |
| `git restore --staged` | 取消暫存但保留改動 |
| `git add` | 暫存 |
| `git rm` | 從未來的快照中移除檔案 |
| `git commit -a` | 自動暫存所有**已追蹤**檔案並提交 |
| tracked / untracked | Git 已認識／尚未認識的檔案 |
| `git push` | 推送本地 commit 到遠端 |

## 所以呢

日常流程的三個習慣值得養成：**開工前 `git pull upstream main`**（避免衝突）、**提交前 `git diff`**（抓出忘了刪的 debug 程式碼）、**push 前跑測試**（不弄壞別人的東西）。

兩個容易踩的坑：**用 `git mv` 而不是 `mv`**（否則歷史斷掉，而且看歷史要加 `--follow`）、**`git commit -a` 不會加新檔案**。

下一節 C.4 解釋快照與 HEAD 的關係，這是理解後面 branch 與 undo 的基礎。
