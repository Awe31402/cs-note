# C.2 Setting Up a Repository

> 原書 p.784.e172（Appendix C: Version Control Using Git）

## 這節在講什麼

兩種起點：**從零開始建一個新專案**，或**加入一個既有的開源專案**（例如 Wally 的 `cvw`）。兩者的指令不同，關鍵差別在於要不要 fork。

## 關鍵點

### 從零開始

> "To start an entirely new project, create a new directory, initialize a repository in that directory, and optionally associate it with a remote repository."
> — p.784.e172

中文解釋：

```bash
$ mkdir ~/my_project
$ cd ~/my_project
$ git init
$ git remote add origin https://github.com/<yourgithubid>/my_project
```

**`git init` 做的事**就是在目錄下建一個 `.git/` 子目錄——從那一刻起這個目錄就是一個 Git 倉庫。

**`git remote add origin <URL>`** 是幫遠端倉庫取名。`origin` 是慣例名稱，指「我自己的那個遠端」。注意這一步是**可選的**——完全在本機用 Git 也完全可行（只是沒有異地備份）。

### 加入既有專案：先 fork 再 clone

> "To begin working with an existing GitHub repository, such as Wally's cvw, fork the upstream repository to your own GitHub account and then clone your fork. This will create a working copy in cvw within your current working directory."
> — p.784.e172

中文解釋：**順序是「先在網頁上 fork，再在命令列 clone」**。

網頁操作：到 github.com/openhwgroup/cvw，按 Fork → Create Fork，得到 github.com/`<你的帳號>`/cvw。

然後：

```bash
$ git clone --recurse-submodules https://github.com/<yourgithubid>/cvw
$ cd cvw
$ git remote add upstream https://github.com/openhwgroup/cvw
```

**三行各有用意**：
1. **clone 自己的 fork**（不是 upstream）——因為你要能推上去
2. **`--recurse-submodules`** 一併取得所有 submodule（C.9 會講）
3. **`git remote add upstream`** 讓本地倉庫也知道主倉庫在哪——這樣才能 `git pull upstream main` 同步別人的改動

> "The --recurse-submodules flag recursively checks out any other Git repositories that are submodules within the main repo."
> — p.784.e172（邊欄）

中文解釋：**漏掉這個旗標是新手最常見的錯誤**。附錄 A.1 列過 `cvw/addins/` 底下十幾個 submodule（SoftFloat、CoreMark、Embench、riscv-arch-test…），漏了就什麼測試都跑不了。

### 兩個 remote 的分工

clone 完之後本地倉庫有**兩個遠端**：

| 名稱 | 指向 | 你能做什麼 |
| --- | --- | --- |
| `origin` | 你的 fork | **讀 + 寫**（`git push`） |
| `upstream` | 主倉庫 | **只能讀**（`git pull upstream main`） |

這就是 C.1 講的三層結構在命令列上的樣子。附錄 A.4 那個提交流程（`git pull upstream main` → `git add` → `git commit` → `git push`）現在完全說得通了——**先從 upstream 拉最新的，再推到 origin，最後在網頁上發 PR**。

### 私有與公開

> "A GitHub repository can be private or public. Typically, a team starts with a private repository and adds collaborators manually. If the team wants to release the project, the owner changes the repo to public."
> — p.784.e172

中文解釋：**先私有、完成後轉公開**是常見做法。要注意的是：**轉成公開之後，整個歷史也會公開**——如果早期 commit 裡不小心存過密碼或金鑰，轉公開前必須清乾淨（改掉密碼比清歷史容易得多）。

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| `git init` | 在當前目錄建立新的 Git 倉庫（產生 `.git/`） |
| `git remote add <name> <URL>` | 幫一個遠端倉庫取名 |
| `origin` | 慣例名稱，指自己的遠端倉庫（fork） |
| `upstream` | 慣例名稱，指主倉庫 |
| `git clone` | 複製遠端倉庫到本機 |
| `--recurse-submodules` | 一併取得所有 submodule |
| private / public repo | 私有／公開倉庫 |
| collaborator | 被授權存取私有倉庫的人 |

## 所以呢

兩種起點：新專案用 `git init`，加入既有專案用 **fork → clone → add upstream** 三步。

最容易踩的坑是 **`--recurse-submodules`**——對 `cvw` 這種有十幾個 submodule 的專案，漏了就等於什麼都沒拿到。

下一節 C.3 是日常工作循環。
