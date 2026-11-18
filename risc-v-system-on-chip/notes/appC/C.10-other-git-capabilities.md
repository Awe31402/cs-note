# C.10 Other Git Capabilities

> 原書 p.784.e181（Appendix C: Version Control Using Git）

## 這節在講什麼

三個零散但常用的功能：`git stash`（暫時收起未完成的工作）、`.gitignore`（排除不該追蹤的檔案）、`git diff` 比較兩個快照。

其中 **`.gitignore` 對硬體專案特別重要**——模擬和合成會產生大量中間檔案，不排除掉倉庫會變得無法使用。

## 關鍵點

### `git stash`：半成品的暫存處

> "Sometimes you are working on a branch in a messy state and need to switch to another branch for a while but don't want to commit your incomplete work. You can use the git stash command to stash your changes, then change branches, do your other work, and then git stash apply to restore your in-progress work."
> — p.784.e181

中文解釋：**這解決了一個實際的兩難**。C.6 講過 `git checkout <branch>` 會改變工作目錄的內容——但如果現在有未提交的改動，Git 會拒絕切換（怕蓋掉你的工作）。

三個選項：
1. 提交一個半成品 commit（污染歷史）
2. 丟掉改動（損失工作）
3. **`git stash`**（收起來，之後拿回）

典型情境：正在改 FPU，突然有人回報一個緊急 bug 要修。`git stash` → `git checkout main` → 修 bug → 推上去 → `git checkout fpu-work` → `git stash apply` → 繼續。

（`git stash list` 看有哪些暫存、`git stash pop` 是 apply 之後順便刪掉。stash 可以堆疊多層，但堆太多很容易忘記裡面是什麼。）

### `.gitignore`：硬體專案的必需品

> "Often, local repositories include files you don't want added to the repo, such as object files. To have Git ignore such files, add them and their path to the .gitignore file. Usually, .gitignore is in the root directory of the repo, but additional .gitignore files may be in subdirectories. Wildcards using '*' can be used to match many different files or even whole directories."
> — p.784.e181

中文解釋：Code Example C.1 的內容：

```
*.o
*.objdump
examples/C/sum/sum
examples/C/fir/fir
```

> "The .gitignore file in Code Example C.1 ignores all C object files and object dumps in any directory, as well as the sum and fir compiled binary files in certain examples directories."
> — p.784.e181

中文解釋：注意兩種寫法——**萬用字元（任何目錄下的 `*.o`）** 和 **明確路徑（那兩個編譯出來的執行檔）**。後者是因為它們沒有可辨識的副檔名，只能一個一個列。

**為什麼這對硬體專案特別重要**：一次模擬或合成會產生大量中間檔案——`work/` 目錄、`transcript`、`*.wlf` 波形檔、Vivado 的 `.runs/`/`.cache/`（附錄 23.2 提過 Vivado 要 90 GiB 磁碟空間）、`.log`、`.objdump`。

這些檔案的共同特徵是：**體積大、是二進位、每次都不一樣、而且可以從原始碼重新產生**——正好命中 C.1 說的「Git 不適合處理的東西」。沒有 `.gitignore` 的話，`git status` 會被幾千個雜訊檔淹沒，倉庫也會迅速膨脹。

**這也呼應附錄 B.5.4 的原則**：「不要放執行檔，放原始碼和 Makefile」。

### `git diff` 比較兩個快照

> "To show the changes between two snapshots, use git diff. If you only specify one snapshot, it will be compared to your HEAD. For example, if the current HEAD is 2d2 (see Fig. C.1), to see what changed relative to snapshot c30, use: $ git diff c30"
> — p.784.e181

中文解釋：`git diff` 有好幾種用法，值得整理：

| 用法 | 比較什麼 |
| --- | --- |
| `git diff` | 工作目錄 vs 暫存區（未 `git add` 的改動） |
| `git diff HEAD` | 工作目錄 vs 最後一次 commit（**包含已暫存的**） |
| `git diff <hash>` | 工作目錄 vs 指定快照 |
| `git diff <hash1> <hash2>` | 兩個快照之間 |

**`git diff` 和 `git diff HEAD` 的差別是實務上最常搞混的**——前者看不到已經 `git add` 過的改動。C.11 的總結會再提一次。

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| `git stash` | 把未完成的改動收起來 |
| `git stash apply` | 取回收起來的改動 |
| `.gitignore` | 列出不要追蹤的檔案路徑（支援 `*` 萬用字元） |
| object file | 編譯產生的中間檔（`.o`） |
| objdump | 反組譯輸出 |

## 所以呢

三個功能各解決一個實際問題：**`git stash`** 讓你能在半成品狀態下切換工作、**`.gitignore`** 讓倉庫不被模擬與合成的中間產物淹沒、**`git diff <hash>`** 讓你能比較任意兩個時間點。

其中 `.gitignore` 對硬體專案是必需品——模擬與合成產生的檔案又大又多又是二進位，正是 Git 最不擅長的東西。

下一節 C.11 是總結，並附上一張把所有指令串起來的資料流圖。
