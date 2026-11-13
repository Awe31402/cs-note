# 22 · Union-Find 並查集

## 一句話
**Union-Find（並查集，也叫 Disjoint Set Union, DSU）** 管理一堆「互不重疊的集合」，只支援兩個操作，而且都幾乎是 O(1)：
- **Find**：x 屬於哪一個集合？（回傳這個集合的「代表」）
- **Union**：把 a、b 所在的兩個集合合併。

最常用來回答「這兩個點連通嗎？」，而且邊會一直增加。

## 圖示：用森林表示
每個集合是一棵樹，**根就是代表**。`parent[x]` 指向父節點，根指向自己。
```
集合 {0,1,2,3}   集合 {4,5}
      0             4
     / \            |
    1   2           5
        |
        3
find(3)：3 → 2 → 0，根是 0
union(3, 5)：find(3) = 0、find(5) = 4 → 把 4 接到 0 底下
```

### 優化 1：依秩合併 (union by rank)
**矮的樹接到高的樹下面**，高度才不會長。`rank` 是樹高的上界，只有兩棵一樣高時，合併後才 +1。
→ rank 為 r 的樹至少有 2ʳ 個節點，所以高度 ≤ log₂n（本專案的測試有驗證）。

### 優化 2：路徑壓縮 (path compression)
find 的時候，把路上經過的每個節點**直接指向根**。下次再 find 就一步到位。
```
find(3) 之前         find(3) 之後
    0                    0
    |                  / | \
    1                 1  2  3
    |
    2
    |
    3
```
LeetCode 題解裡常用的簡化版是**路徑減半 (path halving)**：`p[x] = p[p[x]]`，一邊走一邊讓節點指向祖父。只要一趟，效果也很好。

### 兩個優化一起用
每個操作的攤銷時間是 **O(α(n))**。α 是反 Ackermann 函數，長得極慢，宇宙中所有原子的數量代進去都還不到 5，所以實務上可以當成 O(1)（CLRS 19.4）。

## 複雜度
| 版本 | find / union |
|---|---|
| 什麼優化都沒有 | 最壞 O(n)，樹會變成一條直線 |
| 只有依秩合併 | O(log n) |
| 只有路徑壓縮 | 攤銷 O(log n) |
| **兩個都有** | **攤銷 O(α(n)) ≈ O(1)** |

空間 O(n)。

## 面試陷阱／常考點
1. **union 之前一定要先 find 到根**，然後合併**根**。寫成 `parent[a] = b`（沒有先 find）是錯的。
2. **find 用迴圈寫**：還沒壓縮過的長鏈，遞迴可能爆堆疊（本專案測試了 100 萬個元素連成一條）。
3. **「數連通元件」**：集合數從 n 開始，每成功 union 一次就 -1（LeetCode 547）。
4. **判斷無向圖有沒有環**：加邊時兩端已經在同一組 → 這條邊會形成環（684，也是 Kruskal 的核心）。
5. **Union-Find 只能合併，不能拆開。** 需要「刪邊」時，常見的技巧是把操作**倒過來做**（從最後的狀態開始，把刪除改成加入）。
6. **先處理所有「相等」，再檢查「不相等」**（990）：相等有遞移性，順序錯了會漏判。
7. **應用**：Kruskal 最小生成樹（第 23 主題）、網格連通、朋友圈、帳號合併（721）、影像分割。
8. **加權並查集**：在邊上存「到父節點的比值或差值」，可以解 399 Evaluate Division（第 26 主題用 Floyd 解同一題）。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 1971 | [Find if Path Exists in Graph](https://leetcode.com/problems/find-if-path-exists-in-graph/) | Easy | 連通判斷 |
| 547 | [Number of Provinces](https://leetcode.com/problems/number-of-provinces/) | Medium | 數集合 |
| 684 | [Redundant Connection](https://leetcode.com/problems/redundant-connection/) | Medium | 找形成環的邊 |
| 990 | [Satisfiability of Equality Equations](https://leetcode.com/problems/satisfiability-of-equality-equations/) | Medium | 先合併、再檢查 |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `dsu.h` / `.c` | 依秩合併 + 路徑壓縮（兩趟迴圈版），維護集合大小與集合數 |
| `test_dsu.c` | 基本操作、6 萬次隨機操作對照「重新標記」的暴力法、rank 上界 2ʳ ≤ size、100 萬個元素連成一條 |

執行：`make test T=22-union-find`

## 出處
- **CLRS** 第 19 章 Data Structures for Disjoint Sets · PDF p.683（19.1 操作與連通元件的應用、19.2 串列表示法、19.3 森林表示法 p.692、19.4 α(n) 的分析）
- **Thareja**：沒有專門章節。13.8.3 Kruskal 演算法 p.409 有用到「判斷兩個頂點是否在同一棵樹」的想法。
