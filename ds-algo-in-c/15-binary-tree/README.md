# 15 · Binary Tree 二元樹走訪

## 一句話
**Binary tree（二元樹）** 的每個節點最多有兩個小孩：左 (left) 和右 (right)。**走訪 (traversal)** 就是用固定的順序把每個節點都拜訪一次。

## 名詞
```
            1          ← 根 (root)，深度 0
          /   \
         2     3       ← 2 是 4、5 的父節點 (parent)
        / \     \
       4   5     6     ← 4、6、7 是葉子 (leaf)
          /
         7             ← 深度 3；整棵樹高度 4（以節點數算）
```
| 名詞 | 意思 |
|---|---|
| 深度 (depth) | 從根走到它經過幾條邊 |
| 高度 (height) | 從它往下到最遠的葉子。本專案用「節點數」算：空樹 0、只有根 1 |
| 完全二元樹 (complete) | 除了最後一層都填滿，最後一層從左邊開始填，heap 就是這種 |
| 滿二元樹 (full) | 每個節點都是 0 或 2 個小孩 |

## 四種走訪
以上面那棵樹為例：
| 走訪 | 順序 | 結果 | 用途 |
|---|---|---|---|
| **前序 (preorder)** | 根 → 左 → 右 | 1 2 4 5 7 3 6 | 複製樹、序列化 |
| **中序 (inorder)** | 左 → 根 → 右 | 4 2 7 5 1 3 6 | **BST 會得到排好的順序** |
| **後序 (postorder)** | 左 → 右 → 根 | 4 7 5 2 6 3 1 | 釋放記憶體、算子樹的值 |
| **層序 (level order)** | 一層一層，由左到右 | 1 2 3 4 5 6 7 | BFS、找最短深度 |

記法：「前、中、後」說的是**根**在什麼時候被拜訪。

### 不用遞迴怎麼寫
- **前序**：stack 裡先放右、再放左，左邊就會先被拿出來。
- **中序**：一路往左走到底並沿路 push；pop 出一個就輸出，然後轉去它的右子樹。
- **後序**：用「根 右 左」的順序走（前序把左右對調），最後整個反轉，就是「左 右 根」。
- **層序**：用 queue（BFS）。每一輪先記下 queue 的長度，就能把每一層分開（LeetCode 102）。

### 由前序 + 中序重建樹（Thareja 9.4.5）
```
前序 [3 9 20 15 7]   → 第一個 3 是根
中序 [9 | 3 | 15 20 7] → 3 左邊的 [9] 是左子樹，右邊的 [15 20 7] 是右子樹
遞迴下去
```
只有前序 + 後序**不能**唯一決定一棵樹（除非是滿二元樹），一定要有中序。

## 複雜度
n = 節點數，h = 高度。
| 操作 | 時間 | 額外空間 |
|---|---|---|
| 前／中／後序（遞迴或 stack） | O(n) | O(h)：平衡時 O(log n)，一條直線時 O(n) |
| 層序 | O(n) | O(寬度)，最多約 n/2 |
| `tree_size`、`tree_height` | O(n) | O(h) |
| `build_pre_in` | O(n²)；用 hash 找根的位置就是 O(n) | O(n) |

## 面試陷阱／常考點
1. **遞迴的三個問題**：base case（`root == NULL`）、要回傳什麼、怎麼把左右子樹的答案組合起來。大部分樹的題目都是這個模式（104、226）。
2. **樹的遞迴深度就是高度**：一條直線的樹（像串列）遞迴深度會到 n，可能爆堆疊。
3. **釋放記憶體要用後序**：先釋放小孩，才能釋放自己。
4. **分層的 BFS**：記下 `width = tail - head`，這一輪只處理 width 個。
5. **前序 + 中序**、**後序 + 中序**都可以重建樹；前序 + 後序不行。
6. **Morris 走訪**可以做到 O(1) 額外空間（暫時修改指標），面試偶爾會追問。
7. LeetCode 用陣列表示樹，例如 `[3,9,20,null,null,15,7]`，是**層序**的格式。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 94 | [Binary Tree Inorder Traversal](https://leetcode.com/problems/binary-tree-inorder-traversal/) | Easy | 迴圈版中序 |
| 104 | [Maximum Depth of Binary Tree](https://leetcode.com/problems/maximum-depth-of-binary-tree/) | Easy | 基本遞迴 |
| 226 | [Invert Binary Tree](https://leetcode.com/problems/invert-binary-tree/) | Easy | 鏡像 |
| 102 | [Binary Tree Level Order Traversal](https://leetcode.com/problems/binary-tree-level-order-traversal/) | Medium | 分層 BFS |
| 105 | [Construct Binary Tree from Preorder and Inorder Traversal](https://leetcode.com/problems/construct-binary-tree-from-preorder-and-inorder-traversal/) | Medium | 重建樹 |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `binary_tree.h` / `.c` | 節點、由層序陣列建樹、四種走訪（遞迴版與迴圈版）、大小、高度、前序 + 中序重建 |
| `test_binary_tree.c` | 固定例子的每種走訪、500 棵隨機形狀的樹：迴圈版對照遞迴版、重建後走訪結果一致 |

執行：`make test T=15-binary-tree`

## 出處
- **CLRS** 10.3 Representing rooted trees · PDF p.359；附錄 B.5 Trees · PDF p.1503；12.1 INORDER-TREE-WALK · PDF p.418
- **Thareja** 第 9 章 Trees · p.279–297（9.1 名詞 p.279、9.2 樹的種類 p.280、9.3 一般樹轉二元樹 p.286、9.4 走訪 p.287–290、9.4.5 由走訪結果重建 p.290、9.5 Huffman 樹 p.290）
