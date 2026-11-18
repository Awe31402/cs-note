# C.6 Branching and Merging

> 原書 p.784.e176–784.e178（Appendix C: Version Control Using Git）

## 這節在講什麼

C.4 的分岔是「兩人同時工作」意外造成的，branch 則是**刻意製造分岔**——為了讓半成品的開發不干擾主線。

Fig. C.2 的六張圖走完一個完整的 branch 生命週期：建立 → 切換 → 開發 → 切回 → 合併 → 刪除。

## 關鍵點

### 為什麼要開分支

> "By default, Git keeps your work in the main branch. When multiple developers are working concurrently, it is sometimes helpful to create another branch to write and debug code before integrating back into main so that instabilities in the development branch don't interfere with progress in the main branch."
> — p.784.e176

中文解釋：**核心理由是「隔離不穩定的東西」**。

對 Wally 這種專案特別有意義——主線必須隨時能通過回歸測試，但開發一個新擴充（例如第 18 章的 Zk*）中間會有好幾天是編不過或測不過的狀態。放在分支上就不會拖累別人。

### 六個步驟

**(a) 建立分支**

```bash
$ git branch muldiv
```

> "Now Ben's main and muldiv both point to snapshot ea1 in Fig. C.2(a), and Ben's HEAD still points to his main."
> — p.784.e176

中文解釋：**關鍵理解——分支只是一個指向某快照的名字（標籤）**。`git branch` 建立分支**不會切換過去**，HEAD 還在 `main`。這時 `main` 和 `muldiv` 指向同一個快照 `ea1`。

**(b) 切換並開發**

```bash
$ git checkout muldiv
$ git commit -a -m "New muldiv branch changes"
$ git push -u origin muldiv
```

> "Now Ben can make changes and commit them and push them. They will affect the muldiv branch but not main (Fig. C.2(b)). The first time pushing to a new branch, Ben must also set the origin."
> — p.784.e176

中文解釋：`git checkout muldiv` 把 HEAD 移到 `muldiv`。提交後產生 `bb8`，**`muldiv` 前進到 `bb8`，`main` 還在 `ea1`**。

**`-u`（`--set-upstream`）只有第一次要打**——它告訴 Git「這個本地分支對應遠端的哪個分支」，之後直接 `git push` 就好。

**(c)(d) 切回主線繼續做別的**

```bash
$ git checkout main
```

> "Now Ben's HEAD points to main (Fig. C.2(c)). He can make more changes and commit them (Fig. C.2(d))."
> — p.784.e176

中文解釋：HEAD 回到 `main`，**工作目錄的檔案內容也會跟著變回 `main` 的版本**。再提交產生 `c30`。

現在真正分岔了：

```
              c30 ← main ← HEAD
  ea1 ←
              bb8 ← muldiv
```

**(e) 合併回來**

```bash
$ git merge muldiv
```

> "If the same piece of code received different modifications in different branches, he may have to resolve the merge conflicts manually."
> — p.784.e178

中文解釋：產生 merge commit `2d2`，有兩個父節點（`c30` 和 `bb8`）。**和 C.4 的 `ea1` 是完全一樣的機制**——衝突的處理方式也和 C.5 完全一樣。

**注意合併的方向**：`git merge muldiv` 是在 `main` 上執行，把 `muldiv` 併進來。所以要先 `git checkout main`。

**(f) 刪除分支**

```bash
$ git branch          # 列出所有分支
$ git branch -d muldiv
```

> "If the commits in the branch have not been merged, Git will print a warning and not delete anything. To force the delete use -D."
> — p.784.e178

中文解釋：**`-d` 是安全的刪除——沒合併過就拒絕**。這是一個防呆機制：避免不小心丟掉還沒整合的工作。

**`-D` 是強制刪除**，用在「這個分支的實驗失敗了，不要了」的情況。

刪掉遠端分支：
```bash
$ git push -d origin <branch name>
```

**刪除分支不會刪掉快照**——`bb8` 還在歷史裡（因為 `2d2` 連到它）。刪的只是那個名字。這也是為什麼 Fig. C.2(f) 裡 `bb8` 還在圖上，只是沒有 `muldiv` 標籤了。

### 分支到底是什麼

把六張圖串起來看，結論很簡單：

**分支 = 一個指向某個快照的可移動名字。**

- `git branch <name>` 在當前位置放一個名字
- `git checkout <name>` 把 HEAD 移到那個名字上（工作目錄跟著變）
- 在某個分支上 commit，**那個名字會跟著往前移**
- `git merge` 把兩個名字指向的歷史合起來
- `git branch -d` 只是拿掉名字，快照還在

**`main` 也只是一個分支名**，沒有任何特殊之處——它的特殊性完全來自團隊的約定（「main 上的東西必須能通過測試」）。

### 實務建議

**什麼時候該開分支？** 判斷標準是「這個工作會不會讓主線有一段時間是壞的」：
- 改一個 typo、修一個 bug → 不用開
- 加一個新的 ISA 擴充、重構一個模組 → 開

**分支不要活太久**。開太久的分支和主線差異越來越大，最後合併時會是一場災難（C.5 講的「衝突越晚處理越難解」）。定期把 `main` 併進分支（`git checkout muldiv && git merge main`）可以緩解。

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| branch | 分支，指向某快照的可移動名字 |
| `main` | 預設的主分支 |
| `git branch <name>` | 建立分支（不切換） |
| `git branch` | 列出所有分支 |
| `git checkout <name>` | 把 HEAD 切到該分支 |
| `git merge <name>` | 把指定分支合併進當前分支 |
| `git push -u origin <name>` | 首次推送新分支並建立對應關係 |
| `git branch -d` / `-D` | 安全刪除（未合併會拒絕）／強制刪除 |
| `git push -d origin <name>` | 刪除遠端分支 |

## 所以呢

**分支只是一個會移動的名字**——理解這點，六個指令就都不需要死記了。

兩個實務要點：**`-d` 的防呆機制**（未合併不讓刪）、**分支不要活太久**（否則合併會變成災難）。

下一節 C.7 講 tag——另一種「給快照取名字」的方式，差別在 tag 不會移動。
