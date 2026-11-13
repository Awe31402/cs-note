# 16 · BST 二元搜尋樹

## 一句話
**BST（Binary Search Tree，二元搜尋樹）** 是一棵二元樹，每個節點都滿足：**左子樹全部比它小、右子樹全部比它大**。所以搜尋時每一步都能丟掉一邊，就像在樹上做二分搜尋。

## 圖示
```
CLRS Figure 12.2 的樹（本專案測試用）：
              15
           /      \
          6        18
        /   \     /  \
       3     7   17   20
      / \     \
     2   4     13
               /
              9
中序走訪：2 3 4 6 7 9 13 15 17 18 20   ← 一定是遞增的
```

### 搜尋 / 插入
從根開始：比較小往左、比較大往右。**插入**就是搜尋走到 NULL 的那個位置。

### 後繼 (successor)：中序的下一個
```
有右子樹  → 右子樹的最小值（往右一步，再一直往左）      15 的後繼 = 17
沒有右子樹 → 往上走，直到遇到「從左邊上來」的祖先       13 的後繼 = 15
```

### 刪除的三種情況（CLRS 12.3）
```
1. 沒有小孩：直接刪              2. 只有一個小孩：小孩頂上來
      5                               5
     / \     刪 3                    / \      刪 3
    3   8    →   5                  3   8     →    5
                  \                  \              / \
                   8                  4            4   8

3. 兩個小孩：用後繼（右子樹最小值）取代
      5                     6
     / \      刪 5         / \
    3   8     →           3   8
       / \                     \
      6   9                     9
   （後繼 6 一定沒有左小孩，把它搬上來很簡單）
```
CLRS 用 `TRANSPLANT(u, v)`：讓子樹 v 取代子樹 u 在父節點底下的位置。

### 為什麼需要「平衡」
```
依序插入 1 2 3 4 5：
1
 \
  2
   \
    3       ← 變成一條串列，高度 n，每個操作都是 O(n)
     \
      4
```
所以才有 AVL 樹（第 17 主題）和紅黑樹（第 18 主題），保證高度是 O(log n)。

## 複雜度
h = 樹高。平均（隨機插入）h ≈ O(log n)，最壞 h = n。
| 操作 | 時間 |
|---|---|
| 搜尋 / 插入 / 刪除 | O(h) |
| min / max | O(h) |
| 後繼 / 前驅 | O(h) |
| floor / ceil | O(h) |
| 中序走訪 | O(n) |
| 從最小值一路用後繼走完 | O(n)（攤銷，每條邊最多走兩次） |

## 面試陷阱／常考點
1. **驗證 BST 不能只跟父節點比**：右子樹的**每一個**都要比祖先大。要傳一個範圍 (lo, hi) 下去（LeetCode 98）。
2. **邊界值**：節點值可能就是 `INT_MIN` 或 `INT_MAX`，範圍要用 `long long` 或 NULL 指標表示「沒有邊界」。
3. **中序 = 排好的順序**：第 k 小（230）、驗證 BST、把 BST 轉成排序陣列，都用中序。
4. **刪除有兩個小孩的節點**：用後繼（或前驅）取代。後繼一定沒有左小孩，所以刪它很簡單。
5. **LCA 在 BST 上很簡單**：兩個都比目前小往左、都大往右，否則目前節點就是答案（235）。一般二元樹的 LCA（236）要用後序遞迴。
6. **已排序的資料插入 BST 會退化成串列。** 本專案的 `test_degenerate` 驗證了高度真的會變成 n。
7. **重複 key 的處理**：本專案不允許重複。如果要允許，就統一放到某一邊，或在節點上加一個計數欄位。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 700 | [Search in a Binary Search Tree](https://leetcode.com/problems/search-in-a-binary-search-tree/) | Easy | 基本搜尋 |
| 98 | [Validate Binary Search Tree](https://leetcode.com/problems/validate-binary-search-tree/) | Medium | 傳範圍下去 |
| 230 | [Kth Smallest Element in a BST](https://leetcode.com/problems/kth-smallest-element-in-a-bst/) | Medium | 中序走訪 |
| 235 | [Lowest Common Ancestor of a Binary Search Tree](https://leetcode.com/problems/lowest-common-ancestor-of-a-binary-search-tree/) | Medium | 分岔點 |
| 450 | [Delete Node in a BST](https://leetcode.com/problems/delete-node-in-a-bst/) | Medium | 刪除三種情況 |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `bst.h` / `.c` | 帶 parent 指標的 BST：插入、搜尋、刪除（CLRS TRANSPLANT）、min/max、後繼/前驅、floor/ceil、中序、高度、完整性檢查 |
| `test_bst.c` | CLRS Figure 12.2 的例子（刪除的每種情況）、5 萬次隨機插入刪除對照陣列（含 floor/ceil 與後繼走訪）、已排序輸入退化成直線 |

執行：`make test T=16-bst`

## 出處
- **CLRS** 第 12 章 Binary Search Trees · PDF p.418（12.1 定義與中序走訪 p.418、12.2 搜尋／min／max／後繼 p.423、12.3 插入與刪除 p.429）
- **Thareja** 10.1 Binary Search Trees p.298、10.2 BST 的操作 p.300–306（搜尋、插入、刪除、高度、節點數、鏡像、最小、最大）
