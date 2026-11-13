# 13 · Heap / Priority Queue / Heapsort

## 一句話
**Heap（堆積）** 是一棵用陣列存的「幾乎完全二元樹」(nearly complete binary tree)。每個節點都 ≥ 它的子節點（max-heap），所以**最大值永遠在根**。

它是 **Priority Queue（優先佇列）** 最常見的實作：隨時可以用 O(log n) 放進去一個值，或拿出最大值。

## 圖示
同一個 max-heap 的兩種樣子（CLRS Figure 6.1 的資料，這裡改成 0-based 編號）：

```
            [0]16
          /       \
      [1]14        [2]10
      /   \        /   \
   [3]8  [4]7   [5]9  [6]3
   / \    /
[7]2 [8]4 [9]1

index:  0   1   2   3   4   5   6   7   8   9
value: 16  14  10   8   7   9   3   2   4   1
```

不需要指標，靠 index 的算術就能找到親子關係：

| | 0-based（本專案） | 1-based（兩本書） |
|---|---|---|
| parent(i) | `(i-1)/2` | `i/2` |
| left(i) | `2i+1` | `2i` |
| right(i) | `2i+2` | `2i+1` |

### 兩個核心動作
- **sift up（往上浮）**：新值放在陣列尾端，比父節點大就交換，一直往上。→ 用在 `push`
- **sift down（往下沉）**：值比子節點小，就跟**較大的**子節點交換，一直往下。→ 用在 `pop`、`build`、`heap_sort`。CLRS 叫它 `MAX-HEAPIFY`。

```
pop：拿走根，把最後一個搬到根，再往下沉
     16                1                14
    /  \      →       /  \      →      /  \      → ...
  14    10          14    10          1    10
```

## 複雜度
| 操作 | 時間 | 說明 |
|---|---|---|
| `peek` 看最大值 | O(1) | 就是 `data[0]` |
| `push` 插入 | O(log n) | 最多往上浮樹高 |
| `pop` 取出最大值 | O(log n) | 最多往下沉樹高 |
| `build` 由 n 個值建 heap | **O(n)** | 見下方陷阱 1 |
| 找任意值 | O(n) | heap 不是排序好的，只能整個掃 |
| `heap_sort` | O(n log n) | 原地排序，額外空間 O(1)，**不穩定** |

## 面試陷阱／常考點
1. **Build heap 是 O(n)，不是 O(n log n)。**
   從最後一個非葉節點 `n/2-1` 往回做 sift down。大部分節點靠近底部，只需要往下沉很短。加總起來是 O(n)（CLRS 6.3 有證明）。
   Thareja 12.1.1 用「一個一個 push」來建 heap，那樣才是 O(n log n)。
2. **書上是 1-based，C 是 0-based。** 公式搬錯是最常見的 bug。
3. **sift down 要跟「較大的」子節點換。** 換成較小的那個，heap 性質就壞了。
4. **Heapsort 不穩定 (not stable)。** 值一樣的元素，排序後相對順序可能會變。
5. **Top-K 問題：找前 k 大，要用大小為 k 的 *min*-heap。** 堆頂是目前第 k 大；新值比堆頂大，就取代堆頂。時間 O(n log k)。
6. **Heapsort 最壞也是 O(n log n)**，quicksort 最壞是 O(n²)。但實務上 quicksort 通常比較快，因為 cache 比較友善。
7. **increase-key / decrease-key**（CLRS 6.5）：改一個值後，往上浮或往下沉就好。Dijkstra（第 24 主題）會用到。前提是你知道那個元素在陣列的哪個 index。
8. C 標準函式庫沒有 heap，面試時要能自己寫出 sift up／sift down。

## 練習題（LeetCode）
| # | 題目 | 難度 |
|---|---|---|
| 703 | [Kth Largest Element in a Stream](https://leetcode.com/problems/kth-largest-element-in-a-stream/) | Easy |
| 215 | [Kth Largest Element in an Array](https://leetcode.com/problems/kth-largest-element-in-an-array/) | Medium |
| 347 | [Top K Frequent Elements](https://leetcode.com/problems/top-k-frequent-elements/) | Medium |
| 23 | [Merge k Sorted Lists](https://leetcode.com/problems/merge-k-sorted-lists/) | Hard |
| 295 | [Find Median from Data Stream](https://leetcode.com/problems/find-median-from-data-stream/) | Hard |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `heap.h` | 介面：`heap_init/free/push/pop/peek/build`、`heap_sort` |
| `heap.c` | 實作 |
| `test_heap.c` | 測試：Thareja Example 12.2、CLRS Figure 6.3、重複值與負數、200 組隨機資料對照 `qsort` |

執行：`make test T=13-heap`

## 出處
- **CLRS** 第 6 章 Heapsort（6.1 Heaps ～ 6.5 Priority queues）· PDF p.229 起
- **Thareja** 12.1 Binary Heaps · p.361–365
- **Thareja** 14.13 Heap Sort · p.454
- **Thareja** 8.4.3 Priority Queues · p.268

> 頁碼說明：Thareja 用書上印的頁碼（PDF 頁 = 書頁 + 14）。CLRS 的 PDF 沒有印頁碼，所以用 PDF 頁。
