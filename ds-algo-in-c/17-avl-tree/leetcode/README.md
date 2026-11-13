# AVL Tree · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

執行：`make test T=17-avl-tree`

---

## 110 · Balanced Binary Tree（Easy）
[題目](https://leetcode.com/problems/balanced-binary-tree/) · [程式](0110-balanced-binary-tree.c)

**題意**：判斷一棵樹是不是「高度平衡」：**每個**節點的左右子樹高度差都 ≤ 1。這就是 AVL 的條件。

**思路：後序遞迴，一次做兩件事**
- `check(t)` 平衡的話回傳高度，不平衡就回傳 −1。
- 小孩回傳 −1，就直接一路往上傳 −1，不用再算下去。

**複雜度**：O(n)。

**陷阱**：
- **直覺寫法是 O(n²)**：每個節點都呼叫一次 `height()`，而 `height()` 本身是 O(子樹大小)。樹是一條直線時就是 O(n²)。測試裡的 `balanced_slow` 就是這個版本，只拿來對照答案。
- **只檢查根是不夠的**：測試中有一棵樹，根的左右高度一樣，但下面某個節點不平衡。

---

## 108 · Convert Sorted Array to Binary Search Tree（Easy）
[題目](https://leetcode.com/problems/convert-sorted-array-to-binary-search-tree/) · [程式](0108-convert-sorted-array-to-binary-search-tree.c)

**題意**：把排好序的陣列轉成高度平衡的 BST。

**思路**：中間的元素當根，左半邊遞迴建左子樹，右半邊遞迴建右子樹。
```
[-10 -3 0 5 9]
        ↑ 根 0
[-10 -3]   [5 9]
    ↑         ↑
```
左右兩半的大小最多差 1，所以一定平衡，而且高度是最矮的 ⌈log₂(n+1)⌉。

**複雜度**：O(n)。一個一個插進 AVL 也可以，但那是 O(n log n)。

**陷阱**：中間取 `lo + (hi - lo) / 2` 或 `+1` 都可以，答案不唯一。

---

## 1382 · Balance a Binary Search Tree（Medium）
[題目](https://leetcode.com/problems/balance-a-binary-search-tree/) · [程式](1382-balance-a-binary-search-tree.c)

**題意**：給一棵 BST（可能很歪），回傳一棵節點相同、但高度平衡的 BST。

**思路**：
1. 中序走訪，把**節點指標**依序存進陣列（BST 的中序是排好的）。
2. 用 108 的方法重建：中間當根，左右遞迴。

直接重用原本的節點，只改左右指標，不用 malloc 新節點。

**複雜度**：O(n) 時間、O(n) 空間。

**陷阱**：
- 重建時每個節點的 `left`、`right` 都會被重新設定（包含葉子設成 NULL），所以不會留下舊的指標。
- 面試官可能會追問：「能不能用 AVL 旋轉來做？」可以，但要 O(n log n)，而且程式長很多。這題的重點是看出「中序 + 重建」就夠了。
- 進階：**DSW 演算法**可以用 O(1) 額外空間做到，先把樹轉成一條直線，再用旋轉壓平。
