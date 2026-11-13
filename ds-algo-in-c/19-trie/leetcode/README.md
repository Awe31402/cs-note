# Trie · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

執行：`make test T=19-trie`

---

## 14 · Longest Common Prefix（Easy）
[題目](https://leetcode.com/problems/longest-common-prefix/) · [程式](0014-longest-common-prefix.c)

**題意**：一組字串的最長共同前綴。

**思路（垂直掃描，本檔）**：一欄一欄比。第 i 欄只要有任何一個字串不一樣、或已經結束，就停下來。
```
f l o w e r
f l o w
f l i g h t
    ↑ 第 2 欄不同 → 答案 "fl"
```

**Trie 的做法**：全部插進 trie，從根往下走，直到某個節點「有兩個以上的小孩」或「是某個單字的結尾」。

**複雜度**：垂直掃描 O(S)，S 是所有字元總數，額外空間 O(1)。Trie 也是 O(S)，但要 O(S) 空間。只問一次的話，垂直掃描比較好；如果之後還要一直查詢，trie 比較划算。

**陷阱**：
- 有空字串時，答案就是空字串。
- 短的字串讀到 `'\0'` 時一定跟別人不相等，所以不用另外判斷長度。

---

## 208 · Implement Trie (Prefix Tree)（Medium）
[題目](https://leetcode.com/problems/implement-trie-prefix-tree/) · [程式](0208-implement-trie-prefix-tree.c)

**題意**：實作 `insert`、`search`、`startsWith`。

**思路**：每個節點有 26 個小孩指標，加上一個 `end` 旗標。
- `insert`：沒有路就建新節點，最後把 `end` 設成 true。
- `search`：走得到，**而且** `end` 是 true。
- `startsWith`：走得到就好。

**複雜度**：每個操作 O(L)。

**陷阱**：`search("app")` 在只插入過 `"apple"` 時要回傳 **false**。這是最常見的錯誤。

---

## 720 · Longest Word in Dictionary（Medium）
[題目](https://leetcode.com/problems/longest-word-in-dictionary/) · [程式](0720-longest-word-in-dictionary.c)

**題意**：找最長的單字，而且它的**每一個前綴**都要在字典裡（例如 "world" 需要 "w"、"wo"、"wor"、"worl" 都在）。一樣長的話，取字典順序最小的。

**思路**：
1. 全部插進 trie。
2. 從根開始 DFS，**只能走進 `end == true` 的小孩**（代表那個前綴也是單字）。
3. 走得最深的就是最長的。

**同樣長度取字典順序最小的**：DFS 依照 a → z 的順序走，而且只有在「**更長**」時才更新答案。同樣長度時，先遇到的一定是字典順序比較小的。

**複雜度**：O(S)。另一個做法：排序後用 hash set 檢查，O(S log n)。

**陷阱**：根節點本身不是單字，但要從它出發。單字 "abc" 如果字典裡沒有 "a"，就一步都走不出去，答案是空字串。

---

## 211 · Design Add and Search Words Data Structure（Medium）
[題目](https://leetcode.com/problems/design-add-and-search-words-data-structure/) · [程式](0211-design-add-and-search-words-data-structure.c)

**題意**：加入單字；查詢時 `.` 可以代表任何一個字母。

**思路**：trie + DFS。
- 一般字母：只走那一條路。
- `.`：26 個小孩都試一遍，**有任何一條成功**就回傳 true。

```
加入 bad, dad, mad
search(".ad")：'.' → 試 b、d、m 三條，每條再比 'a'、'd' → 找到
```

**複雜度**：沒有 `.` 時 O(L)。最壞（全部都是 `.`）要走整棵樹，O(節點數)。

**陷阱**：
- 走到字串結尾時，要檢查 `end`，不是「走得到就算」。`search("..")` 在只有三個字母的單字時要回傳 false。
- 題目限制查詢最多 2 個 `.`，所以實際上很快。

---

## 648 · Replace Words（Medium）
[題目](https://leetcode.com/problems/replace-words/) · [程式](0648-replace-words.c)

**題意**：句子裡每個字，如果有字根 (root) 是它的前綴，就換成**最短**的那個字根。

**思路**：
1. 所有字根插進 trie。
2. 句子裡每個字從 trie 根往下走：
   - **第一次**碰到 `end` → 那就是最短字根，截斷在這裡。
   - 走不下去（沒有這條路） → 沒有字根，保留整個字。

```
字根 {cat, bat, rat}
"cattle"  → c-a-t 碰到 end → "cat"
"the"     → t 沒有路 → 保留 "the"
```

**複雜度**：O(字典總長 + 句子長度)。暴力做法是每個字都跟每個字根比，O(字數 × 字根數 × L)。

**陷阱**：
- 要**最短**的字根，所以碰到第一個 `end` 就要停，不能繼續往下找。
- 替換只會讓字串變短，所以輸出開跟輸入一樣大的空間就夠了。
