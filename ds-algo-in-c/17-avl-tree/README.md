# 17 · AVL Tree

## 一句話
**AVL 樹**是會自動保持平衡的 BST：每個節點的**左右子樹高度最多差 1**。插入或刪除後，如果某個節點失衡，就用**旋轉 (rotation)** 修正。所以高度永遠是 O(log n)。

## 平衡因子 (balance factor)
`bf(x) = 左子樹高度 − 右子樹高度`，只能是 **−1、0、+1**。變成 ±2 就要旋轉。

## 旋轉
```
右旋 rotate_right(x)：                 左旋 rotate_left(x)：
        x              y                   x                  y
       / \            / \                 / \                / \
      y   C   →      A   x               A   y      →       x   C
     / \                / \                 / \            / \
    A   B              B   C               B   C          A   B
```
- 旋轉後**中序順序不變**（A y B x C），所以還是合法的 BST。
- 只改 O(1) 個指標。
- 重點：y 原本的子樹 B 要**過繼**給 x。

## 四種失衡情況
x 是第一個失衡的節點（從新節點往上找）：
```
LL：左小孩的左邊太高       RR：右小孩的右邊太高
      3                      1
     /                        \
    2      → 右旋 3            2      → 左旋 1
   /                            \
  1                              3

LR：左小孩的右邊太高       RL：右小孩的左邊太高
    3                        1
   /                          \
  1     → 先左旋 1            3     → 先右旋 3
   \      再右旋 3           /        再左旋 1
    2                       2
```
四種情況的結果都是：
```
    2
   / \
  1   3
```
判斷方法：看 x 的 bf 是 +2 還是 −2，再看**那一邊小孩**的 bf 符號。符號相反就是 LR 或 RL，要轉兩次。

## 插入與刪除
- **插入**：照普通 BST 插入，然後沿著路徑往上，每一層都更新高度、檢查平衡。插入時**最多只要修一個地方**（一次單旋或雙旋）。
- **刪除**：照普通 BST 刪除，然後一樣沿路往上修。但刪除可能要修 **O(log n) 個地方**，一路旋轉到根。

## 為什麼高度是 O(log n)
高度 h 的 AVL 樹，最少節點數 N(h) = N(h−1) + N(h−2) + 1，跟 Fibonacci 數列一樣是指數成長。
所以 h < 1.44 log₂(n + 2)。本專案的測試有驗證這個上界。

## 複雜度
| 操作 | 時間 |
|---|---|
| 搜尋 | O(log n) |
| 插入 | O(log n)，最多 2 次旋轉 |
| 刪除 | O(log n)，最多 O(log n) 次旋轉 |
| 空間 | 每個節點多存一個高度（或平衡因子） |

## AVL vs 紅黑樹
| | AVL | 紅黑樹（第 18 主題） |
|---|---|---|
| 平衡程度 | 比較嚴格，高度 ≤ 1.44 log n | 比較寬鬆，高度 ≤ 2 log n |
| 搜尋 | 稍快 | 稍慢 |
| 插入／刪除 | 旋轉可能比較多 | 旋轉比較少（插入 ≤ 2、刪除 ≤ 3） |
| 常見用途 | 查詢很多、修改很少 | 一般用途：C++ `std::map`、Java `TreeMap`、Linux 核心 |

## 面試陷阱／常考點
1. **旋轉後要先更新下面那個節點的高度，再更新上面的**（x 變成 y 的小孩，所以先算 x）。
2. **LR / RL 要轉兩次**：只轉一次的話，樹還是不平衡，只是換了一邊歪。
3. **判斷 LR：看的是「失衡節點的左小孩」的 bf**，不是新插入節點的位置。刪除時沒有「新節點」可以看，用 bf 判斷才通用。
4. **遞迴寫法最容易寫對**：`insert` 回傳新的子樹根，回來的路上每一層都呼叫 `rebalance`。
5. **已排序的資料**：普通 BST 高度 n，AVL 高度 log n（測試：插入 1..1023，高度剛好 10）。
6. **把不平衡的 BST 變平衡**（LeetCode 1382）：面試時用「中序 + 重建」O(n) 就好，不需要真的寫 AVL。
7. 面試很少要你手寫完整的 AVL，但**旋轉**和**四種情況**一定要能畫出來。

## 練習題（LeetCode）
LeetCode 沒有要求實作 AVL 的題目。這幾題練的是「平衡」的概念：
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 110 | [Balanced Binary Tree](https://leetcode.com/problems/balanced-binary-tree/) | Easy | 高度平衡的定義，O(n) 檢查 |
| 108 | [Convert Sorted Array to Binary Search Tree](https://leetcode.com/problems/convert-sorted-array-to-binary-search-tree/) | Easy | 取中間當根 |
| 1382 | [Balance a Binary Search Tree](https://leetcode.com/problems/balance-a-binary-search-tree/) | Medium | 中序 + 重建 |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `avl.h` / `.c` | 遞迴版 AVL：左右旋轉、四種情況的 `rebalance`、插入、刪除、搜尋、完整性檢查（BST 性質、高度欄位、平衡） |
| `test_avl.c` | 四種失衡情況各自的旋轉次數、已排序輸入 1..1023 高度剛好 10、10 萬次隨機插入刪除（檢查高度上界 1.44 log n） |

執行：`make test T=17-avl-tree`

## 出處
- **CLRS** 問題 13-3 AVL trees · PDF p.475；13.2 Rotations · PDF p.447
- **Thareja** 10.4 AVL Trees · p.316–327（10.4.1 搜尋、插入、刪除與旋轉）
