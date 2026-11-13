# 資料結構與演算法 · 讀書筆記（C 實作）

面試準備用。整合兩本書，按主題整理，每個主題附 C 實作與測試。

- **CLRS** — *Introduction to Algorithms*, 4th ed.
- **Thareja** — *Data Structures Using C*, 2nd ed.

計劃與決策見 [PLAN.md](PLAN.md)。

## 使用
```sh
make test              # 跑全部測試
make test T=13-heap    # 只跑一個主題
make list              # 列出有測試的主題
make clean
```
編譯參數：C17、`-Wall -Wextra -Werror`、AddressSanitizer + UBSan（遇到錯誤直接讓測試失敗）。

## 目錄與進度
✅ 完成 · 🚧 進行中 · ⬜ 未開始

| # | 主題 | 批次 | 狀態 |
|---|---|---|---|
| 01 | [複雜度 Big-O/Ω/Θ](01-complexity/) | 1 | ✅ |
| 02 | [遞迴 Recursion](02-recursion/) | 1 | ✅ |
| 03 | [陣列 Array](03-array/) | 2 | ✅ |
| 04 | [字串 String](04-string/) | 2 | ✅ |
| 05 | [Linked List](05-linked-list/) | 2 | ✅ |
| 06 | [Stack](06-stack/) | 2 | ✅ |
| 07 | [Queue / Deque](07-queue/) | 2 | ✅ |
| 08 | [Hash Table](08-hash-table/) | 3 | ✅ |
| 09 | [Binary Search](09-binary-search/) | 3 | ✅ |
| 10 | [Insertion Sort](10-insertion-sort/) | 4 | ✅ |
| 11 | [Merge Sort](11-merge-sort/) | 4 | ✅ |
| 12 | [Quick Sort](12-quick-sort/) | 4 | ✅ |
| 13 | [Heap / Priority Queue / Heapsort](13-heap/) | 0 | ✅ |
| 14 | [Counting / Radix Sort](14-linear-sort/) | 4 | ✅ |
| 15 | [Binary Tree 走訪](15-binary-tree/) | 5 | ✅ |
| 16 | [BST](16-bst/) | 5 | ✅ |
| 17 | [AVL Tree](17-avl-tree/) | 5 | ✅ |
| 18 | [紅黑樹 Red-Black Tree](18-red-black-tree/) | 5 | ✅ |
| 19 | [Trie](19-trie/) | 5 | ✅ |
| 20 | [BFS / DFS](20-bfs-dfs/) | 6 | ✅ |
| 21 | [拓撲排序 Topological Sort](21-topological-sort/) | 6 | ✅ |
| 22 | [Union-Find](22-union-find/) | 6 | ✅ |
| 23 | [MST (Kruskal / Prim)](23-mst/) | 6 | ✅ |
| 24 | [Dijkstra](24-dijkstra/) | 6 | ✅ |
| 25 | [Bellman-Ford](25-bellman-ford/) | 6 | ✅ |
| 26 | [Floyd-Warshall](26-floyd-warshall/) | 6 | ✅ |
| 27 | [Divide & Conquer](27-divide-and-conquer/) | 7 | ✅ |
| 28 | [動態規劃 DP](28-dynamic-programming/) | 7 | ✅ |
| 29 | [Greedy](29-greedy/) | 7 | ✅ |
| 30 | [KMP](30-kmp/) | 7 | ✅ |
| 31 | [B-Tree](31-b-tree/) | 8 | ✅ |
| 32 | [Segment Tree](32-segment-tree/) | 8 | ✅ |
| 33 | [Max Flow (Edmonds-Karp)](33-max-flow/) | 8 | ✅ |
| 34 | [NP 觀念](34-np/) | 8 | ✅ |
