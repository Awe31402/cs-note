# BST · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

LeetCode 的 `TreeNode` 沒有 parent 指標，所以這裡的寫法跟 `bst.c`（CLRS 版）不太一樣，多半用遞迴。

執行：`make test T=16-bst`

---

## 700 · Search in a Binary Search Tree（Easy）
[題目](https://leetcode.com/problems/search-in-a-binary-search-tree/) · [程式](0700-search-in-a-binary-search-tree.c)

**題意**：在 BST 裡找值為 val 的節點，回傳那棵子樹。

**思路**：比較小往左、比較大往右，找到或走到 NULL 為止。用迴圈寫，不用堆疊。

**複雜度**：O(h)。

---

## 98 · Validate Binary Search Tree（Medium）
[題目](https://leetcode.com/problems/validate-binary-search-tree/) · [程式](0098-validate-binary-search-tree.c)

**題意**：判斷一棵二元樹是不是合法的 BST（嚴格小於／大於，不能相等）。

**思路**：每個節點都有一個合法的範圍 `(lo, hi)`：
- 往左走：上界變成目前節點的值。
- 往右走：下界變成目前節點的值。
```
      5          (−∞, +∞)
     / \
    1   6        1: (−∞, 5)   6: (5, +∞)
       / \
      3   7      3: (5, 6) ← 3 不在範圍內 ❌
```
另一個做法：中序走訪，檢查是不是嚴格遞增。

**複雜度**：O(n)。

**陷阱**：
- **只跟父節點比是錯的**：上面的例子裡，3 < 6 看起來沒問題，但 3 在 5 的右子樹裡，不合法。
- **節點值可能剛好是 `INT_MIN` / `INT_MAX`**：用 `int` 當邊界會誤判，要用 `long long`。
- 值相等也不合法。

---

## 230 · Kth Smallest Element in a BST（Medium）
[題目](https://leetcode.com/problems/kth-smallest-element-in-a-bst/) · [程式](0230-kth-smallest-element-in-a-bst.c)

**題意**：找 BST 中第 k 小的值。

**思路**：中序走訪是遞增的，所以用迴圈版中序（15 主題的 94 題），數到第 k 個就停。

**複雜度**：O(h + k)，不用走完整棵樹。

**延伸題**：如果樹會一直被修改，又要一直查第 k 小？在每個節點多存「子樹大小」(size)，就能 O(h) 找到第 k 小。這就是 CLRS 17.1 的「順序統計樹 (order-statistic tree)」。

---

## 235 · Lowest Common Ancestor of a Binary Search Tree（Medium）
[題目](https://leetcode.com/problems/lowest-common-ancestor-of-a-binary-search-tree/) · [程式](0235-lowest-common-ancestor-of-a-binary-search-tree.c)

**題意**：找 p、q 兩個節點最低的共同祖先 (LCA)。節點自己也可以是自己的祖先。

**思路**：從根往下走：
- p、q 都比目前小 → LCA 在左邊。
- p、q 都比目前大 → LCA 在右邊。
- 否則（一個在左、一個在右，或其中一個就是目前節點）→ 這裡就是分岔點，也就是 LCA。

**複雜度**：O(h)，額外空間 O(1)。

**陷阱**：這個方法只適用於 **BST**。一般二元樹（236）沒有大小關係可以用，要用後序遞迴：左右兩邊都找到就是自己。

---

## 450 · Delete Node in a BST（Medium）
[題目](https://leetcode.com/problems/delete-node-in-a-bst/) · [程式](0450-delete-node-in-a-bst.c)

**題意**：刪除 key，回傳新的根。

**思路（遞迴，每次回傳「新的子樹根」）**：
1. 先往左或往右找到那個節點。
2. 找到之後：
   - 只有一個小孩或沒有小孩 → 回傳那個小孩（或 NULL）頂替自己。
   - 兩個小孩都有 → 找右子樹最小的值（後繼），**把值搬上來**，再到右子樹裡刪掉那個後繼。

**複雜度**：O(h)。

**陷阱**：
- 遞迴回傳新的根，並寫成 `root->left = deleteNode(root->left, key)`，這樣就不用記 parent。
- 「搬值」的做法很簡單，但如果節點上還有其他資料（或外面有指標指著它），就不能只搬 key。這時要用 CLRS 的 TRANSPLANT，真的移動節點（見 `bst.c`）。
- key 不存在時，要原封不動回傳。
