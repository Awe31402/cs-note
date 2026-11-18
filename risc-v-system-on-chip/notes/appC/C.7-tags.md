# C.7 Tags

> 原書 p.784.e178（Appendix C: Version Control Using Git）

## 這節在講什麼

一頁的小節。tag 是「給某個快照取一個固定的名字」，方便日後回去找。

和 C.6 的 branch 對照著看最清楚：**branch 是會移動的名字，tag 是釘死的名字**。

## 關鍵點

### 為什麼需要 tag

> "Sometimes it is important to name snapshots for easy access in the future. For example, to name the current snapshot as v2.0: $ git tag -a v2.0 -m 'Version 2.0 Released by Ben Bitdiddle 30 November 2025'"
> — p.784.e178

中文解釋：**hash 是 40 位十六進位數，人記不住也看不出意義**。`v2.0` 就清楚多了。

`-a` 建立 annotated tag（帶作者、日期、訊息），`-m` 給訊息。

**典型用途**：發布版本、論文投稿時的程式碼狀態、demo 前的已知良好版本、tape-out（送去製造）的那一版 RTL。最後一個對這本書的情境特別重要——**晶片送製造之後，那一版 RTL 必須永遠能找回來**。

### 列出所有 tag

> "$ git tag / v0.1 / v1.0 / v2.0"
> — p.784.e178

中文解釋：不帶參數的 `git tag` 就是列出全部。

### tag 預設不會推上去

> "By default, tags are private to the user and aren't pushed into the remote repository. To push a tag, list its name: $ git push v2.0"
> — p.784.e178

中文解釋：**這是最容易漏掉的一點**。`git push` 推 commit，**但不推 tag**。打了 tag 卻忘了單獨推，別人就看不到。

（想一次推所有 tag 可以用 `git push --tags`。）

### 取出一個 tag

> "A user can check out a tagged snapshot with: $ git checkout v2.0"
> — p.784.e178

中文解釋：這會把工作目錄變回那個版本的狀態。

**一個要知道的細節**（書上沒提但實務上會遇到）：`git checkout <tag>` 之後會進入 **detached HEAD** 狀態——因為 HEAD 直接指向一個快照，而不是指向某個分支。這時可以看、可以編譯、可以測試，但**如果在這裡提交，那個 commit 不屬於任何分支，之後很容易找不到**。要在舊版本上開發，應該從 tag 開一個分支：`git checkout -b fix-v2.0 v2.0`。

### tag vs branch

| | branch | tag |
| --- | --- | --- |
| 會移動嗎 | **會**（在該分支提交時跟著前進） | **不會**（永遠指向同一個快照） |
| 用途 | 進行中的開發線 | 歷史上的重要時刻 |
| 預設會 push 嗎 | 會（設過 `-u` 之後） | **不會** |
| 命名慣例 | `main`、`muldiv`、`fix-xyz` | `v1.0`、`v2.0`、`tapeout-2025` |

**兩者在底層是同一種東西**——都是「指向快照的名字」，差別只在 Git 會不會自動移動它。

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| tag | 釘在某個快照上的固定名字 |
| annotated tag (`-a`) | 帶作者、日期、訊息的 tag |
| `git tag` | 列出所有 tag |
| `git push <tagname>` | 推送單一 tag（預設不會隨 `git push` 送出） |
| detached HEAD | HEAD 直接指向快照而非分支的狀態 |
| tape-out | 晶片設計定案送製造 |

## 所以呢

三件事：**tag 是釘死的名字，branch 是會移動的名字**；**tag 預設不會被 `git push` 推上去，要單獨推**；**在 tag 上開發前要先開分支**（否則進入 detached HEAD，提交容易遺失）。

下一節 C.8 是這個附錄最實用的部分——三種檔案狀態，以及各種「反悔」的方法。
