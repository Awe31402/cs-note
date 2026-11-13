# Binary Tree · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試（包含把 LeetCode 層序陣列轉成樹的小工具）。

執行：`make test T=15-binary-tree`

---

## 94 · Binary Tree Inorder Traversal（Easy）
[題目](https://leetcode.com/problems/binary-tree-inorder-traversal/) · [程式](0094-binary-tree-inorder-traversal.c)

**題意**：回傳中序走訪的結果。進階要求：不要用遞迴。

**思路（迴圈 + stack）**：
```
cur = root
重複：
  1. 一路往左走，沿路 push
  2. pop 一個 → 輸出它（它的左邊都處理完了）
  3. cur = 它的右小孩
直到 cur 是 NULL 而且 stack 空了
```

**複雜度**：O(n) 時間，O(h) 空間。

**陷阱**：迴圈條件是 `cur || top > 0`，兩個都要檢查。只看 stack 的話，剛轉去右子樹時 stack 可能是空的，迴圈就提早結束了。

---

## 104 · Maximum Depth of Binary Tree（Easy）
[題目](https://leetcode.com/problems/maximum-depth-of-binary-tree/) · [程式](0104-maximum-depth-of-binary-tree.c)

**題意**：樹的最大深度（根到最遠葉子的節點數）。

**思路**：`depth(root) = 1 + max(depth(left), depth(right))`，空樹是 0。

**複雜度**：O(n) 時間，O(h) 空間。

**陷阱**：延伸題 111「**最小**深度」不能直接把 max 改成 min。只有一邊有小孩時，空的那邊不是葉子，不能算進來。

---

## 226 · Invert Binary Tree（Easy）
[題目](https://leetcode.com/problems/invert-binary-tree/) · [程式](0226-invert-binary-tree.c)

**題意**：把樹左右翻轉（鏡像）。

**思路**：每個節點都交換左右小孩，再遞迴處理兩邊。

**複雜度**：O(n)。

**陷阱**：寫成 `root->left = invert(root->right); root->right = invert(root->left);` 是錯的：第二行拿到的 `root->left` 已經被改掉了。要先把其中一個存起來。

---

## 102 · Binary Tree Level Order Traversal（Medium）
[題目](https://leetcode.com/problems/binary-tree-level-order-traversal/) · [程式](0102-binary-tree-level-order-traversal.c)

**題意**：一層一層回傳節點值，每層是一個陣列。

**思路**：BFS 加上「分層」的技巧：
```
queue: [3]           width = 1 → 第 0 層 [3]，放入 9, 20
queue: [9 20]        width = 2 → 第 1 層 [9 20]，放入 15, 7
queue: [15 7]        width = 2 → 第 2 層 [15 7]
```
每一輪開始時，queue 裡剛好是一整層，所以先記下 `width`，這一輪只處理 width 個。

**複雜度**：O(n)。

**陷阱**：C 要回傳 `int**` 和每層的長度 `returnColumnSizes`。題目最多 2000 個節點，所以最多 2000 層，直接開這麼大就好。

---

## 105 · Construct Binary Tree from Preorder and Inorder Traversal（Medium）
[題目](https://leetcode.com/problems/construct-binary-tree-from-preorder-and-inorder-traversal/) · [程式](0105-construct-binary-tree-from-preorder-and-inorder-traversal.c)

**題意**：給前序和中序（值不重複），重建二元樹。

**思路**：
```
前序 [3 9 20 15 7]    中序 [9 3 15 20 7]
根 = 3（前序第一個）
中序裡 3 的左邊 [9] → 左子樹；右邊 [15 20 7] → 右子樹
遞迴
```
- 用陣列 `pos[值]` 記住每個值在中序的位置，找根就是 O(1)。
- 用一個全域的 `pre_i` 依序讀前序：因為前序是「根 左 右」，只要**先建左子樹**，讀的順序就剛好對。

**複雜度**：O(n)。每次都在中序裡線性搜尋根的話是 O(n²)。

**陷阱**：
- 一定要先遞迴左邊、再遞迴右邊，順序反了 `pre_i` 就錯了。
- 題目保證值不重複。有重複值的話，中序裡找不到唯一的根，整個方法就不成立（本機測試一開始就踩到這個坑）。
- LeetCode 會在同一個程式裡多次呼叫 `buildTree`，所以 `pre_i` 每次都要重設成 0。
