# C.1 Gitting Started

> 原書 p.784.e170–784.e172（Appendix C: Version Control Using Git）

## 這節在講什麼

附錄 C 的開場加上初次設定。開場那段（784.e170–e171）比 C.1 本身更重要——它用一個**所有人都經歷過的慘劇**說明為什麼需要版本控制，並且定義了整個附錄會用到的術語。

## 關鍵點

### 沒有版本控制的下場

> "For example, you might save a copy of your project report as project_old.docx before making revisions. The next night, you might save the final copy as project_final.docx. Then your lab partner might make some more edits and save it as project_final_ah.doc. Then you might fix a bug and resave it as project_final_notkiddingthistime.docx."
> — p.784.e170

中文解釋：這段自嘲式的舉例點出三個真實問題：**搞不清楚哪個版本最新**、**兩人同時改會互相覆蓋且看不出誰改了什麼**、**硬碟壞了全部消失**。

### Git 的核心模型

> "The general idea of Git is to maintain a repository of all the files in a project with snapshots every time a user checks in changes. A Git project has at least one remote repository, often hosted on a website such as github.com where collaborators can easily access it. Each user clones the entire repository to their own working copy ... also called a local repository."
> — p.784.e170

中文解釋：**Git 是分散式的**——每個人手上都有一份完整的倉庫（含全部歷史），不是只有一份中央副本。這也是它取代 CVS/Subversion 的主因：

> "Git has largely supplanted older version control tools such as Concurrent Versions System (CVS) and Subversion because it is more flexible, is a distributed tool resistant to failures of a centralized server, and offers easy worldwide collaboration through sites such as GitHub."
> — p.784.e170（邊欄）

中文解釋：三個理由——彈性、**不怕中央伺服器掛掉**、方便全球協作。

（Git 是 Linus Torvalds 在 2005 年開發的，就是寫 Linux 那位——他為了支援 Linux 的開發流程而做了它。）

### 一次改動的生命週期

> "The user then stages modified files for inclusion in the next snapshot and then commits the staged files to create a new snapshot. Each snapshot is associated with a hash to identify the snapshot and ensure its integrity."
> — p.784.e170

中文解釋：**modify → stage → commit → push** 是核心流程。「stage（暫存）」這一步是 Git 特有的——它讓你能**只提交部分改動**，而不是把工作目錄裡所有東西一次送出。

`hash` 是 40 位十六進位數，同時當識別碼與完整性檢查。

### Git 不適合什麼

> "Git is designed and optimized for managing text-based source code files of small to medium size. It is not good for huge files (10s of megabytes or more) or binary files. Don't place executable files in a Git repository; instead, keep the source code and the Makefile necessary to regenerate the executable."
> — p.784.e171

中文解釋：**這是很實際的限制**。原因是 Git 存的是「每次快照的完整檔案」（靠壓縮與去重省空間），對二進位檔它算不出有意義的差異，每改一次就多存一份完整副本，倉庫會迅速膨脹。

（這本書的專案恰好是個反例——57 MB 的 PDF 被放進 Git 裡。）

「不要放執行檔，放原始碼和 Makefile」正好呼應附錄 B.5.4。

### fork / pull request 工作流程

> "In a larger project with many contributors, a central upstream repository is maintained by committers, who review and approve contributions. A user who simply wants to clone the code can clone the upstream repository. A user who wants to contribute code makes a fork of the upstream repository into her own GitHub account. She then makes changes and tests them in her own fork. When they are complete, she makes a pull request, asking the committers to incorporate the new or modified code from the fork into the upstream repository."
> — p.784.e171

中文解釋：**這是開源專案的標準模式**，也是附錄 A.4 講的提交流程：

```
upstream (openhwgroup/cvw)  ← committers 守門
    ↓ fork
fork (你的帳號/cvw)          ← 遠端倉庫
    ↓ clone
local (你的電腦/cvw)         ← 本地倉庫
```

**關鍵是「三層」而不是兩層**——你沒有權限直接改 upstream，所以要先 fork 一份自己能寫的。

> "She should periodically sync her fork with the upstream repository to see the latest contributions from others and to avoid merge conflict surprises."
> — p.784.e171

中文解釋：**定期同步是避免合併衝突的關鍵**。拖太久才同步，別人已經改了同一批檔案，衝突會很難解。

（邊欄提醒：GitHub 的「project boards」是追蹤 issue 和 PR 的工具，和這裡說的 repository/repo/project 是完全不同的概念。）

### 四行初始設定

> "$ git config --global user.name 'Ben Bitdiddle' / $ git config --global user.email 'ben_bitdiddle@wally.edu' / $ git config --global pull.rebase false / $ git config --global credential.helper 'store --file ~/.my-credentials'"
> — p.784.e171

中文解釋：四行各做一件事：
1. **`user.name`、`user.email`** — 會寫進每個 commit。email 要用 GitHub 帳號上的那個，不然貢獻不會被算到你頭上
2. **`pull.rebase false`** — 明確選擇 pull 時用 merge 而非 rebase，避免 Git 每次都警告
3. **`credential.helper`** — 存認證資訊，不用每次輸入

### personal access token 取代密碼

> "Then set up a personal access token to access your repositories via HTTPS. ... Set the expiration date to Custom and choose a date a year away. Check the repo box under Select scopes, then scroll to the bottom and click Generate token. Copy your token and save it some place secure on your computer because you will never be able to see it again on GitHub. It should be of the form ghp_<long string of random letters and numbers>."
> — p.784.e171–e172

中文解釋：**GitHub 已經不接受密碼認證了**。要產生一個 token 當密碼用。兩個實務要點：
- **token 只會顯示一次**，關掉頁面就再也看不到（只能重新產生）
- **要設過期日和權限範圍（scope）**——這是 token 比密碼安全的地方：可以限制權限、可以單獨撤銷

> "Git's credential helper will store your authentication information, so you do not need to reenter it in the future. Unfortunately, it is presently stored in unencrypted format."
> — p.784.e172

中文解釋：**明文儲存是一個要知道的風險**。共用伺服器上要特別注意那個 credential 檔的權限（附錄 B.2 的 `chmod 600`）。

（邊欄：GitHub 的認證流程一直在變，如果這個做法失效，查 docs.github.com。）

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| repository / repo / project | 倉庫，此附錄中三詞同義 |
| snapshot | 快照，一次 commit 的完整狀態 |
| hash | 40 位十六進位的快照識別碼 |
| clone | 把遠端倉庫複製成本地倉庫 |
| working copy / local repository | 你電腦上的那一份 |
| remote repository | 遠端倉庫（GitHub 上的） |
| upstream | 上游主倉庫（如 `openhwgroup/cvw`） |
| fork | 在 GitHub 上複製到自己帳號的遠端倉庫 |
| committer | 有權限合併 PR 的維護者 |
| pull request | 請求把 fork 的改動合併進 upstream |
| stage | 把改動放進待提交區 |
| merge conflict | 兩人改同一處造成的衝突 |
| personal access token | 取代密碼的存取權杖，可設範圍與期限 |
| credential helper | 儲存認證資訊的機制（明文） |
| CVS / Subversion | Git 之前的集中式版本控制工具 |

## 所以呢

三個要點：**Git 是分散式的**（每人一份完整歷史）、**開源協作是 upstream → fork → local 三層**（因為你沒權限寫 upstream）、**不要把大檔案和執行檔放進 Git**。

設定只有四行，但 personal access token 那段值得記——**token 只顯示一次，而且本機是明文儲存**。

下一節 C.2 講怎麼建立或取得一個倉庫。
