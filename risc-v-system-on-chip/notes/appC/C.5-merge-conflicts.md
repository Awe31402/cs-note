# C.5 Merge Conflicts

> 原書 p.784.e175–784.e176（Appendix C: Version Control Using Git）

## 這節在講什麼

C.4 說「改到同一行才會衝突」，這節就處理那個情況。重點是**看懂衝突標記的五行格式**——理解之後，解衝突就只是編輯文字檔而已，不神秘。

## 關鍵點

### 衝突怎麼發生

> "Suppose Ben and Alyssa both modified hello.c to add more enthusiasm. Ben capitalized WORLD in snapshot 3fd, and Alyssa added a bunch of exclamation points in snapshot 925. Now Alyssa pushes her changes and then Ben attempts to pull, a merge conflict occurs because Git does not know whose changes to accept."
> — p.784.e175

中文解釋：**接續 C.4 的 Fig. C.1 例子**。兩人都改了 `hello.c` 的**同一行**：
- Ben（`3fd`）：`printf("Hello WORLD!");`
- Alyssa（`925`）：`printf("Hello World!!!!!");`

Git 沒有辦法判斷該保留誰的，所以停下來問人。

**關鍵是「同一行」**——如果 Ben 改第 5 行、Alyssa 改第 20 行，Git 會自動合併，完全不會問。

### Git 的回報

> "CONFLICT (content): Merge conflict in hello.c / Automatic merge failed; fix conflicts and then commit the result."
> — p.784.e175

中文解釋：訊息說得很清楚——**「自動合併失敗，解決衝突後提交結果」**。此時 merge 處於「進行中」的狀態，還沒完成。

### 衝突標記的五行格式

> "<<<<<<< HEAD / printf('Hello WORLD!'); / ======= / printf('Hello World!!!!!'); / >>>>>>> 0b2d6c97f4265819281bf1a8d77698eb9ff30925"
> — p.784.e176

中文解釋：**Git 把兩個版本都留在檔案裡，用三個標記分隔**：

```
<<<<<<< HEAD
printf("Hello WORLD!");          ← 我這邊（HEAD）的版本
=======
printf("Hello World!!!!!");      ← 對方的版本
>>>>>>> 0b2d6c97f426...          ← 對方那個 commit 的完整 hash
```

三個標記的意思：
| 標記 | 意思 |
| --- | --- |
| `<<<<<<< HEAD` | 從這裡開始是**我的**版本 |
| `=======` | 分隔線 |
| `>>>>>>> <hash>` | 到這裡結束，前面那段是**對方**的版本 |

**注意 `>>>>>>>` 後面是完整的 40 位 hash**（不是 short hash）——這樣你能用 `git show <hash>` 去查對方那次改動的完整內容和訊息。

### 怎麼解

> "To fix the merge conflict, Ben must edit these five lines and pick one or the other or a hybrid of the two printf statements and delete the other four lines of the message about the conflict, leaving a single line such as: printf('Hello Alyssa!!!!');"
> — p.784.e176

中文解釋：解衝突就是**編輯這五行，留下你要的一行，刪掉其他四行**（包括三個標記）。

書上這個例子選了**混合版本**（Alyssa 的驚嘆號 + 一個新的名字），這很重要——**你不一定要二選一**，常常正確答案是兩邊的改動都要保留。

例如 RTL 的情境：Ben 在 `always_comb` 裡加了一個 case 分支、Alyssa 在同一個 block 加了另一個——正確的解法是**兩個都留**，而不是選一個。

> "Then he can commit the new hello.c and push it to the remote repo."
> — p.784.e176

中文解釋：解完之後 `git add hello.c` → `git commit` → `git push`。commit 會產生 C.4 講的那個 merge commit（有兩個父節點）。

### 實務要點

**一定要搜一遍 `<<<<<<<`**。如果漏掉一處沒解，那五行就會原封不動被提交進去——SystemVerilog 編譯器會報一個完全看不懂的語法錯誤（因為 `<<<<<<<` 不是合法的語法）。提交前用 `grep -r '<<<<<<<'`（附錄 B.3）掃一次是好習慣。

**衝突越晚處理越難解**。這就是 C.1 和 C.3 反覆強調「定期 `git pull upstream main`」的原因——同步間隔越長，兩邊的改動堆積越多，衝突的範圍越大、越難判斷該留什麼。

**衝突不是錯誤**。它只表示 Git 不敢替你做決定。真正該擔心的反而是「自動合併成功但語意上錯了」——例如 Ben 改了一個函式的名字、Alyssa 在別處新增了對舊名字的呼叫，兩處不衝突所以自動合併，但編譯不過。**這就是為什麼 C.3 說「push 前要跑測試」。**

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| merge conflict | 合併衝突，兩邊改到同一處 |
| `<<<<<<< HEAD` | 衝突標記的開頭，以下是我方版本 |
| `=======` | 衝突標記的分隔線 |
| `>>>>>>> <hash>` | 衝突標記的結尾，前面是對方版本 |
| hybrid | 混合兩邊改動的解法 |

## 所以呢

三件事：**衝突標記是五行，解法是編輯它、留下想要的、刪掉標記**；**常常正確答案是「兩邊都留」而不是二選一**；**提交前用 `grep` 掃一遍 `<<<<<<<` 確認沒漏**。

還有一個更隱蔽的風險——**自動合併成功不代表語意正確**，所以測試不能省。

下一節 C.6 講 branch，也就是「刻意製造分岔」。
