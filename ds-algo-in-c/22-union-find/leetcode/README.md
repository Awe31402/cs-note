# Union-Find · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

LeetCode 上的寫法通常很精簡：只用一個 `parent` 陣列，`find` 用**路徑減半**（`p[x] = p[p[x]]`），不做依秩合併。這樣已經很快，而且只要幾行。

執行：`make test T=22-union-find`

---

## 1971 · Find if Path Exists in Graph（Easy）
[題目](https://leetcode.com/problems/find-if-path-exists-in-graph/) · [程式](1971-find-if-path-exists-in-graph.c)

**題意**：無向圖中，source 和 destination 之間有沒有路？

**思路**：每條邊都把兩端 union 起來，最後看 source 和 destination 的根是不是同一個。

**複雜度**：O(E · α(V))。BFS/DFS 也是 O(V + E)，但要先建鄰接串列。只問一次的話兩者差不多；如果要問很多次、邊又會一直增加，Union-Find 比較好。

---

## 547 · Number of Provinces（Medium）
[題目](https://leetcode.com/problems/number-of-provinces/) · [程式](0547-number-of-provinces.c)

**題意**：用鄰接矩陣表示城市之間的直接連線。直接或間接相連的城市算同一個省，總共有幾個省？

**思路**：集合數一開始是 n，每**成功**合併一次（兩個原本不同組）就 -1。

**複雜度**：O(n² · α(n))，要看完整個矩陣。

**陷阱**：矩陣是對稱的，只看上三角（`j > i`）就好。對角線一定是 1，不用管。

---

## 684 · Redundant Connection（Medium）
[題目](https://leetcode.com/problems/redundant-connection/) · [程式](0684-redundant-connection.c)

**題意**：一棵 n 個節點的樹，多加了一條邊。找出拿掉之後會變回一棵樹的那條邊；有多個答案就回傳輸入中**最後**出現的。

**思路**：依序加入每條邊。**第一條「兩端已經相連」的邊**就是形成環的那條，也就是答案。
```
[1,2] [1,3] [2,3]
加 [1,2]：1、2 合併
加 [1,3]：1、3 合併
加 [2,3]：2 和 3 已經相連了 → 答案 [2,3]
```

**為什麼剛好是「最後出現」的那條**：樹加上一條邊只會有**一個**環。依序加邊時，環上前面的邊都能成功合併，只有環上最後出現的那條會發現兩端已經相連。

**複雜度**：O(n · α(n))。

**陷阱**：節點編號是 1..n，陣列要開 n + 1 格。

---

## 990 · Satisfiability of Equality Equations（Medium）
[題目](https://leetcode.com/problems/satisfiability-of-equality-equations/) · [程式](0990-satisfiability-of-equality-equations.c)

**題意**：一堆 `"a==b"` 和 `"b!=c"` 形式的方程式，能不能同時成立？

**思路：兩趟**
1. 先處理所有 `==`：把兩邊 union 起來。
2. 再檢查所有 `!=`：兩邊在同一組 → 矛盾，回傳 false。

**為什麼一定要分兩趟**：`["a!=c", "a==b", "b==c"]`，如果照順序處理，看到 `a!=c` 時 a、c 還沒相連，就漏掉了。相等關係有**遞移性**，要全部合併完，才知道誰跟誰相等。

**複雜度**：O(n · α(26))，變數只有 26 個字母。

**陷阱**：`"a!=a"` 本身就矛盾。第二趟會發現 a 和 a 在同一組，所以會正確回傳 false。
