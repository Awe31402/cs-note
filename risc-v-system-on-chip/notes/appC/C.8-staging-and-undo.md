# C.8 Staging and Undo

> 原書 p.784.e179–784.e180（Appendix C: Version Control Using Git）

## 這節在講什麼

整個附錄最實用的一節。**三種檔案狀態（committed / modified / staged）加上狀態之間的轉換指令**——Fig. C.3 把這張圖畫出來了。

理解這張圖，就不會再對「`git restore`、`git reset`、`git checkout`、`git revert` 到底差在哪」感到困惑。

## 關鍵點

### 三種狀態

> "Each file in a Git repository exists in one of three states: committed, modified, or staged. Understanding the meaning of these states and how to move between these states is essential to using Git effectively."
> — p.784.e179

中文解釋：

> "When a repo is first cloned, all files in the working directory will be in the committed state. If you modify a file either by changing the contents of the file or its meta data such as permissions, the file moves into the modified state. git add moves modified files into the staging area (also called the index). git commit creates a new commit from only the files in the staging area, and now the files are in the committed state."
> — p.784.e179

中文解釋：Fig. C.3 的循環：

```
  Committed ──[編輯檔案]──> Modified
      ↑                        │
      │                    git add
      │                        ↓
      └──[git commit]───── Staged
```

**注意「改權限也算修改」**——`chmod` 過的檔案也會進入 modified 狀態。這在 shell 腳本加執行權限時會遇到（附錄 B.5）。

### staging area 存在的理由

> "If multiple files are modified, it is possible to move only a subset of the files to the staging area and make a commit of just some files."
> — p.784.e179

中文解釋：**這就是 staging area 的唯一目的——讓你能「只提交一部分」。**

實務上的價值：你在改一個 bug 時順手修了三個 typo。這應該是兩個 commit（一個 bug fix、三個 typo 各自獨立），而不是一個混在一起的大 commit。staging area 讓你能分次提交。

### 四種「反悔」

書上按「改動走到哪一步」分別給出對應的指令：

**(1) 改了但還沒 `git add`**

> "If you have modified code in a file but have not yet staged (git add) or committed (git commit) it, you can throw it away and go back to the last committed version using: $ git checkout <file>"
> — p.784.e179

中文解釋：Modified → Committed。**改動直接消失，不可逆**（C.3 提過的 `git restore <file>` 是同樣效果的較新寫法）。

**(2) 已經 `git add` 但還沒 commit**

> "If you have staged a file but not committed it, you can remove it from the staging area (but not discard the changes), leaving the file in the modified state: $ git reset <file>"
> — p.784.e179

中文解釋：Staged → Modified。**只取消暫存，改動還在**。這是安全的操作。（C.3 的 `git restore --staged <file>` 是同樣效果。）

**(3) 已經 commit 但還沒 push**

> "If you have committed a set of staged changes but not yet pushed it to the repo, reset with --hard: $ git reset --hard hash#"
> — p.784.e179

> "--hard specifically tells Git to remove changes to both the staging area and the working directory. Any files tracked by Git will be set to the committed state of the hash. A common hash is origin/main, as it reverts the file back to the state of the main branch. --hard can only be used for the whole working directory. To revert a single file use git checkout <file>."
> — p.784.e179

中文解釋：**`--hard` 是最危險的指令**——它同時清掉暫存區和工作目錄，**所有未提交的改動全部消失**。

`git reset --hard origin/main` 的意思是「把我的本地狀態完全變回遠端 main 的樣子」。當本地搞得一團亂時這很有用，但要確定沒有想保留的東西。

**書上用粗體標了一個警告**：

> "IMPORTANT: Never perform git reset if commits are already pushed. Rewriting published history creates problems for other users."
> — p.784.e179（邊欄）

中文解釋：**這是 Git 最重要的一條規則。**

原因：`reset` 是「假裝那些 commit 從來沒發生過」。但別人已經把它們拉下去、可能已經在上面繼續工作了。你把它們從歷史裡抹掉，別人下次 pull 時歷史對不上，會陷入一團混亂。

**已經 push 的東西，只能用下一個方法處理。**

**(4) 已經 push 了**

> "If you have committed some changes any time in the history of a project and wish to roll back those changes and create a new snapshot without those changes but with everything else the same: $ git revert <hash>"
> — p.784.e179

> "<hash> is the snapshot to undo. This is effectively the opposite of applying a patch. Typically, the resulting files will be in a committed state; however, it is possible to have a conflict. You will need to resolve the conflict, stage the changes, and then commit."
> — p.784.e180

中文解釋：**`revert` 和 `reset` 的差別是整節的核心**：

| | `git reset --hard` | `git revert` |
| --- | --- | --- |
| 做什麼 | **刪掉**那些 commit | **新增一個 commit**，內容是「把那次改動反過來做一遍」 |
| 歷史 | 被改寫 | 保留完整（多一筆） |
| 已 push 能用嗎 | **絕對不行** | **可以** |
| 比喻 | 把日記那一頁撕掉 | 在日記後面寫「昨天那件事我做錯了，改回來」 |

**`revert` 是唯一對協作安全的撤銷方式**，因為它不動歷史，只是往前加一筆。

### 完整的狀態轉換圖（Fig. C.3）

```
                  編輯檔案
      Committed ──────────> Modified
          ↑  ↑                  │
          │  │  git checkout    │
          │  │  git reset --hard│
          │  └──────────────────┤
          │                 git add
    git reset --hard            │
          │                     ↓
          │                  Staged ──git reset──> Modified
          │                     │
          └──── git commit ─────┘
```

**這張圖值得記住**——遇到「我剛剛做錯了」的時候，先問自己「現在在哪個狀態」，答案就在圖上。

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| committed / modified / staged | 三種檔案狀態 |
| staging area / index | 暫存區，`git add` 的目的地 |
| `git checkout <file>` | 丟棄未暫存的改動（不可逆） |
| `git reset <file>` | 取消暫存，保留改動 |
| `git reset --hard <hash>` | 把整個工作目錄退回某快照，**改寫歷史** |
| `git revert <hash>` | 新增一個「反做」的 commit，**不改歷史** |
| `origin/main` | 遠端 main 分支的本地參照 |
| rewriting published history | 改寫已推送的歷史，會害到其他人 |

## 所以呢

一句話記住：**沒 push 用 `reset`，已 push 用 `revert`。**

三個狀態的轉換圖（Fig. C.3）是這節的核心——出錯時先確認「改動走到哪一步」，就知道該用哪個指令。

最危險的是 **`git reset --hard`**：它會清掉所有未提交的工作，而且用在已 push 的 commit 上會害到整個團隊。

下一節 C.9 講 submodule——這對 `cvw` 特別重要，因為它有十幾個。
