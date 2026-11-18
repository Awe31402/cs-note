# C.4 Snapshots and HEAD

> 原書 p.784.e175（Appendix C: Version Control Using Git）

## 這節在講什麼

短但關鍵的一節。**HEAD 是理解 Git 的核心概念**——它只是一個指標，指向「我現在看的是哪個快照」。懂了 HEAD，後面的 branch（C.6）和 undo（C.8）就都通了。

Fig. C.1 用 Ben 和 Alyssa 兩人並行工作的例子，四張圖走完一次「分岔 → 合併」。

## 關鍵點

### 快照是鏈狀的

> "Each commit creates a new snapshot of the entire state of the repository linked back to all previous snapshots. Every user has a HEAD, which points to the snapshot that user is presently viewing. Most commonly, HEAD points to the snapshot of the main branch, but later we will discuss how to check out an old snapshot or make a different branch for exploratory work."
> — p.784.e175

中文解釋：三個要點：
1. **每個快照是「整個倉庫的狀態」**，不是「這次改了什麼」（雖然實際儲存時會做去重與壓縮）
2. **快照往回連到之前的快照**——所以歷史是一條鏈（合併時會變成有分岔又匯合的圖）
3. **HEAD 是每個使用者自己的指標**，指向「我現在在哪」

（邊欄：Git 可以用 hash 的**前 7 位**識別快照，叫 short hash。Fig. C.1 為了畫圖只用前 3 位。）

### Fig. C.1 的四步

> "For example, suppose Ben and Alyssa are both working on a project. They both pull a snapshot of the main branch with the hash 8a9. Now both of their HEADs are pointing to that 8a9 snapshot, as shown in Fig. C.1(a)."
> — p.784.e175

中文解釋：

**(a) 起點**：兩人的 HEAD 都指向 `8a9`

```
        8a9
       ↑   ↑
  HEAD(ben) HEAD(alyssa)
```

**(b) Ben 提交**：產生 `3fd`，Ben 的 HEAD 前進，**Alyssa 還停在 `8a9`**

> "Next Ben makes a change and commits it, creating a new snapshot with hash 3fd, and advancing his HEAD to 3fd (Fig. C.1(b))."
> — p.784.e175

**(c) Alyssa 也提交**：產生 `925`，**也是接在 `8a9` 後面**

> "Meanwhile, Alyssa makes a change and commits it, creating a new snapshot with hash 925 (Fig. C.1(c)). Both of their snapshots are based on the original 8a9 snapshot."
> — p.784.e175

中文解釋：**這就是「分岔」**——`3fd` 和 `925` 都以 `8a9` 為父節點。注意這**不是**故意開 branch，只是兩人同時工作的自然結果。

**(d) 合併**：

> "Now Alyssa pushes her change to the remote repository. Next, Ben attempts to push but gets an error because 'the remote contains work that you do not have locally.' He must pull again, causing his changes to be merged with Alyssa's, creating a new snapshot ea1 (Fig. C.1(d)). He can then push ea1 to the remote repository."
> — p.784.e175

中文解釋：**這解釋了 C.3 那句「push 被拒絕就先 pull」的原理**。

`ea1` 是一個 **merge commit**——它有**兩個父節點**（`3fd` 和 `925`），這是 Fig. C.1(d) 裡箭頭匯合的意思。

```
        3fd ←┐
  8a9 ←      ├← ea1 ← HEAD(ben)
        925 ←┘
```

**誰先 push 誰輕鬆**——Alyssa 先推所以什麼都不用做，Ben 後推就得負責合併。這在團隊裡是隨機的，所以每個人都要會處理。

### 為什麼這節重要

這個模型解釋了三件之後會遇到的事：

1. **「push 被拒絕」不是錯誤，是資訊**——遠端有你的 HEAD 還沒包含的快照。
2. **merge 是自動的，除非改到同一處**——Git 看得出 `3fd` 和 `925` 各自從 `8a9` 改了什麼，只要改的不是同一行就能自動併。改到同一行就是 C.5 的 merge conflict。
3. **branch 不是什麼特別的東西**——Fig. C.1(c) 那個分岔在結構上和 C.6 的 branch 一模一樣，差別只是 branch 有名字。

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| snapshot | 快照，一次 commit 記錄的完整倉庫狀態 |
| HEAD | 指向「目前所在快照」的指標，每個使用者各自有一個 |
| short hash | hash 的前 7 位，通常已足以唯一識別 |
| merge commit | 有兩個父節點的快照，由合併產生 |
| main branch | 預設分支 |

## 所以呢

**HEAD 只是一個指標，快照是一張往回連的圖。** 這兩句話就是 Git 的全部。

從這個模型可以直接推出：兩人同時工作必然分岔、後 push 的人負責合併、改到同一行才會衝突。下一節 C.5 就處理那個衝突。
