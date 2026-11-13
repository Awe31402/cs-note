# 32 · Segment Tree 線段樹

## 一句話
**線段樹 (segment tree)** 用一棵二元樹管理一個陣列的**區間**：每個節點記錄一段區間的彙總值（總和、最大值…）。**區間查詢**和**修改**都是 O(log n)。

## 為什麼需要它
| 方法 | 修改一格 | 查詢區間和 |
|---|---|---|
| 直接用陣列 | O(1) | O(n) |
| 前綴和 | **O(n)**（後面全部要重算） | O(1) |
| **線段樹** | **O(log n)** | **O(log n)** |
| 線段樹 + 懶標記 | 區間修改也是 O(log n) | O(log n) |

修改和查詢都很多時，線段樹最平衡。

## 圖示
```
a = [5, 8, 6, 3, 2, 7, 2, 6]

                   [0,7] 39
              /                \
        [0,3] 22              [4,7] 17
        /      \              /      \
   [0,1] 13  [2,3] 9     [4,5] 9   [6,7] 8
    /  \      /  \        /  \      /  \
   5    8    6    3      2    7    2    6
```
- 節點 k 的左右小孩是 2k、2k+1（跟 heap 一樣），根是 1。陣列開 **4n** 格一定夠。
- 查 [2, 5] 的和：只要用到 `[2,3] 9` 和 `[4,5] 9` 兩個節點 → 18。
- 任何區間最多被拆成 O(log n) 個「完全包含」的節點。

## 懶標記 (lazy propagation)
**區間修改**（例如 a[l..r] 全部 +v）如果每一格都去改，就是 O(n)。
懶標記的做法：遇到**完全被包含**的節點，只更新它自己的總和，並在它身上記一筆「**欠小孩的加值**」，**不再往下走**。之後真的需要往下走時，再把欠的傳給兩個小孩（`push_down`）。
```
對 [0,3] 全部 +1：
[0,3] 的 sum 22 → 26，lazy = 1      ← 在這裡就停了
之後查 [2,3]：經過 [0,3] 時先 push_down，小孩 [0,1]、[2,3] 各自加上欠的值
```

## Fenwick Tree（樹狀陣列 / Binary Indexed Tree）
只支援「**單點加值 + 前綴和**」，但程式只有幾行，常數也很小：
```c
void add(i, v) { for (i++; i <= n; i += i & -i) bit[i] += v; }
long long prefix(i) { long long s = 0; for (i++; i > 0; i -= i & -i) s += bit[i]; return s; }
```
`i & -i` 是 i 二進位中最低的那個 1（lowbit）。`bit[i]` 管 `(i − lowbit(i), i]` 這一段。
區間和 = `prefix(r) − prefix(l−1)`。

| | 線段樹 | Fenwick |
|---|---|---|
| 程式長度 | 長 | **很短** |
| 單點修改 + 區間和 | ✅ | ✅ |
| 區間修改 | ✅（懶標記） | 要用兩個 BIT 的技巧 |
| 最大值／最小值 | ✅ | 不好做 |
| 空間 | 4n | n + 1 |

## 複雜度
| 操作 | 時間 |
|---|---|
| 建樹 | O(n) |
| 單點修改 | O(log n) |
| 區間修改（懶標記） | O(log n) |
| 區間查詢 | O(log n) |
| 空間 | O(n)（4n 格） |

## 面試陷阱／常考點
1. **陣列不會變就不要用線段樹**，前綴和就夠了（LeetCode 303）。
2. **只有單點修改 + 區間和，就用 Fenwick**（307），比較短、比較不容易寫錯。
3. **陣列大小開 4n**：2n 在 n 不是 2 的次方時不夠。
4. **懶標記一定要在往下走之前 push_down**（查詢和修改都要），否則小孩的值是舊的。
5. **座標很大（例如 10⁹）但點很少**：先做**座標離散化 (coordinate compression)**，把用到的座標排序、去重，換成 0..m−1 的編號（699）。
6. **總和要用 `long long`**：本專案的測試對 10 萬個元素加了 10 萬次 10⁶，總和是 10¹⁶。
7. **常見變化**：區間最大值／最小值、區間設值（懶標記存「要設成多少」）、數逆序對（315 可以用 Fenwick 解）。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 303 | [Range Sum Query - Immutable](https://leetcode.com/problems/range-sum-query-immutable/) | Easy | 不會變 → 前綴和就好 |
| 307 | [Range Sum Query - Mutable](https://leetcode.com/problems/range-sum-query-mutable/) | Medium | 單點修改 → Fenwick |
| 699 | [Falling Squares](https://leetcode.com/problems/falling-squares/) | Hard | 區間最大值 + 區間設值 + 離散化 |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `segtree.h` / `.c` | 線段樹（區間加值 + 區間求和，懶標記）、單點設值、Fenwick Tree |
| `test_segtree.c` | 200 組隨機陣列 × 500 次混合操作對照暴力法、所有區間 Fenwick 和線段樹都對照、10 萬次整段加值（總和 10¹⁶） |

執行：`make test T=32-segment-tree`

## 出處
- **CLRS**：沒有線段樹。最相關的是第 17 章 Augmenting Data Structures · PDF p.633（在樹的節點上多存子樹的資訊；17.3 Interval trees p.644）
- **Thareja**：沒有
