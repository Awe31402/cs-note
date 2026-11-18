# C.11 Summary

> 原書 p.784.e182–784.e184（Appendix C: Version Control Using Git）

## 這節在講什麼

附錄 C 的總結，核心是 **Fig. C.4 的「資料層」圖**——它把前面十節的所有指令放進一張圖，標出每個指令在哪兩層之間搬資料。

**這張圖是理解 Git 的最佳工具**。書上自己也這麼說：

> "To use Git effectively, you need a mental model of its operations."
> — p.784.e182

## 關鍵點

### 六層資料儲存

Fig. C.4 由左到右是六個「層」：

```
Stash │ Workspace │ Staging Area │ Local      │ Remote Fork │ Remote Upstream
      │           │ (Index)      │ Repository │ Repository  │ Repository
      └───────── /home/ben/cvw ──────────────┘ github.com/   github.com/
                                                benbitdiddle/  openhwgroup/
                                                cvw            cvw
```

**前四層在你的電腦上，後兩層在 GitHub 上**——這就是 C.1 講的三層架構（local / fork / upstream），只是把本地再細分成三格（工作目錄、暫存區、本地倉庫）加一個 stash。

### 往右的流程（貢獻程式碼）

> "Ben now edits, adds, or removes files. These changes are part of Ben's workspace but are not yet in the local repository. As Ben becomes satisfied with changes, he stages them using git add. When he has completed a group of changes, he commits the staged changes to the local repository using git commit. Alternatively, he can add and commit all changes in one fell swoop with git commit -a. When he has an internet connection, he uploads them back to his remote fork with git push. ... If he wants his changes to go into the remote upstream repository, he makes a pull request (through GitHub). An OpenHW Foundation committer reviews the pull request and may ask for changes or may merge it into the remote upstream repo."
> — p.784.e182

中文解釋：

| 指令 | 從 | 到 |
| --- | --- | --- |
| `git add` | Workspace | Staging Area |
| `git commit` | Staging Area | Local Repository |
| **`git commit -a`** | Workspace | **Local Repository（跳過暫存區）** |
| `git push` | Local Repository | Remote Fork |
| **Pull Request** | Remote Fork | Remote Upstream |

**注意 Pull Request 不是 Git 指令**——它是 GitHub 的功能（圖上那一段沒有 `git` 開頭）。這是 C.1 講的「committer 守門」機制。

### 往左的流程（取得與撤銷）

> "If Ben is unsatisfied with a group of unstaged changes, he could discard them all using git checkout. If the changes have been staged, he can still discard them using git checkout HEAD to throw away the staged changes and roll the workspace back to the HEAD of the local repository. If Ben is only unsatisfied with changes to a specific unstaged file, he can discard those with git restore. If the file has already been staged, he can unstage it with git restore --staged."
> — p.784.e182

中文解釋：**這段把 C.8 的撤銷指令對應到圖上的層**：

| 指令 | 從 | 到 | 撤銷什麼 |
| --- | --- | --- | --- |
| `git restore` | Staging Area | Workspace | 一個未暫存檔案的改動 |
| `git restore --staged` | Local Repo | Staging Area | 取消暫存（保留改動） |
| `git checkout` | Local Repo | Workspace | 未暫存的改動 |
| **`git checkout HEAD`** | Local Repo | Workspace | **連已暫存的一起丟掉** |

**`git checkout` 和 `git checkout HEAD` 的差別**就是「要不要連暫存區的一起清掉」——這是 Fig. C.4 上兩條不同長度的箭頭。

### 取得別人的改動：pull vs fetch

> "When others make changes to the remote upstream repository, Ben has several ways to get them. The simplest is to git pull upstream main, which fetches these changes from the upstream repository into the local repository and the workspace. If the remote upstream repository and the workspace both have changes to a particular file, Git will attempt to merge the changes. However, if a particular line is changed in both places, a merge conflict occurs."
> — p.784.e183

> "Alternatively, Ben can sync his fork to the upstream remote on GitHub, then git fetch the changes into his local repository without applying them to the workspace yet."
> — p.784.e183

中文解釋：**這是 `pull` 和 `fetch` 的關鍵區別**，Fig. C.4 上畫得很清楚：

| 指令 | 箭頭到哪裡 | 意思 |
| --- | --- | --- |
| `git pull upstream main` | 一路到 **Workspace** | 下載**並且**套用到工作目錄 |
| `git fetch` | 只到 **Local Repository** | 只下載，工作目錄不動 |

**`fetch` 比較安全**——先拿下來看看別人改了什麼（`git diff`、`git log`），確認沒問題再合併。`pull` 等於 `fetch` + `merge` 一次做完，可能立刻撞上衝突。

（另外注意「Sync Fork」也不是 Git 指令——那是 GitHub 網頁上的按鈕，把 upstream 的改動同步到你的 fork。）

### stash 在圖上的位置

> "git stash restores the workspace to match the local repo (like git checkout HEAD) but keeps a copy of the staged and unstaged changes. This is helpful to avoid merge conflicts or having to commit incomplete code before temporarily changing to a branch. git stash pop reapplies the stashed changes."
> — p.784.e183

中文解釋：**這句話把 stash 定義得比 C.10 更精確**——`git stash` 的效果**等同 `git checkout HEAD`**（把工作目錄清乾淨），差別只在它**留了一份副本**在 Stash 那一層。

所以 Fig. C.4 最左邊那一格的存在理由就是：**「我想要 `git checkout HEAD` 的效果，但不想真的丟掉東西」**。

### 圖上沒有畫的：分支

> "Branching is not shown in this diagram. git branch creates a new branch. git checkout <branchname> checks out the branch out into the workspace. Be sure to commit or stash any changes in the workspace before checking out a different branch. git checkout main returns to the main branch. git merge merges changes from another branch into the current branch."
> — p.784.e184

中文解釋：**為什麼分支畫不進去**——Fig. C.4 是「資料在哪幾層之間流動」的圖，而分支是**同一層內部的多條平行線**（C.6 的 Fig. C.2 才是描述分支的正確圖）。兩張圖是互補的視角。

「切分支前記得先 commit 或 stash」正是 C.10 講 stash 時的那個情境。

### 只想看不想貢獻

> "If Ben just wants to look at the remote upstream repository but not make contributions, he can simply clone the remote upstream repo instead of making a fork. However, he will not have permission to push commits from the clone directly to the remote upstream repository."
> — p.784.e184

中文解釋：**只是想讀程式碼、跑模擬的話，直接 clone upstream 就好，不用 fork**。C.2 講的三層結構只在「要貢獻」時才需要。

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| Workspace | 工作目錄，你實際編輯的檔案 |
| Staging Area / Index | 暫存區 |
| Local Repository | 本地倉庫（`.git/` 裡的歷史） |
| Remote Fork / Remote Upstream | 你的 fork ／ 主倉庫 |
| Stash | 暫存起來的未完成改動 |
| `git fetch` | 只下載到本地倉庫，不動工作目錄 |
| `git pull` | `fetch` + 套用到工作目錄 |
| Sync Fork | GitHub 網頁功能，把 upstream 同步到 fork |
| `git checkout HEAD` | 丟掉工作目錄與暫存區的所有改動 |

## 所以呢

**Fig. C.4 是整個附錄的濃縮**——六層資料、每個指令是層與層之間的一支箭頭。記住這張圖，就不需要死背指令。

三個從圖上看出來的重點：
1. **`git commit -a` 跳過暫存區**（所以不能挑選要提交哪些檔案）
2. **`fetch` 只到本地倉庫，`pull` 一路到工作目錄**——不確定別人改了什麼時，用 `fetch` 比較安全
3. **`git stash` = `git checkout HEAD` + 留一份副本**

附錄 C 到此結束。下一個附錄 D 講 Tcl——EDA 工具（第 6 章的合成、第 23 章的 Vivado）的通用腳本語言。
