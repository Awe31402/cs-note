# 讀書筆記計劃書

> 狀態：第 0～8 批全部完成（2026-09-28）。

## 目標
為**面試準備**，整理兩本書的資料結構與演算法，每個主題附 C 實作與測試。

- **CLRS** — *Introduction to Algorithms*, 4th ed.（`algorithms.pdf`，虛擬碼）
- **Thareja** — *Data Structures Using C*, 2nd ed.（`ds-in-c.pdf`，C 程式）

## 決策
| 項目 | 決定 |
|---|---|
| 組織 | 按主題分資料夾，兩書合併，標註出處（書／章／頁） |
| 撰寫 | Claude 寫筆記與程式，使用者審閱修改 |
| 語言 | 繁體中文，關鍵術語附英文 |
| 格式 | Markdown，git 版控 |
| C | C17、`int` 元素、`gcc -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all`、`assert` 測試、Makefile |
| 紅黑樹／B-Tree | 插入、刪除都實作 |
| NP | 只有筆記，無主程式（LeetCode 仍附解法，示範 NP-hard 問題在 n 很小時的解法） |
| Segment Tree | 兩書皆無，出處標「無」 |
| LeetCode | **每個主題**都挑 3–5 題對應的免費題，附 C 解法（含本機測試）與說明；題號一律上網核對。題目太少的主題就在筆記裡說明原因（25 Bellman-Ford、31 B-Tree 各只有 2 題）；LeetCode 沒有直接實作題的主題（18 紅黑樹、31 B-Tree）改用「需要有序集合」的題目並內嵌實作 |
| 交付 | 分批；每批使用者審閱通過後才 commit |

## 目錄結構
```
ds-algo-c/
  Makefile            # make test / make test T=05-linked-list
  README.md           # 總目錄 + 進度表
  NN-topic/
    README.md         # 筆記
    topic.h  topic.c  # 實作
    test_topic.c      # 測試
    leetcode/
      README.md       # 每題：題意、思路、複雜度、其他解法、陷阱
      NNNN-slug.c     # 單檔解法 + 本機測試（make test 會一起跑）
tools/
  lc_check.py         # 核對 LeetCode 題號：python3 tools/lc_check.py 215 703
```

## 筆記模板（每個主題）
1. 一句話直覺
2. ASCII 圖示
3. 操作複雜度表
4. 面試陷阱／常考點
5. 練習題（LeetCode 3–5 題）：表格列題號、題名連結、難度，由易到難；只選免費題；每題都對照 LeetCode 官方題目清單核對（`tools/lc_check.py`）
   - 解法放 `leetcode/NNNN-slug.c`：單檔、LeetCode 原簽名，「提交範圍」可直接貼上，下方 `main` 為本機測試
   - 說明放 `leetcode/README.md`：題意、思路、複雜度、其他解法、陷阱
6. 出處（CLRS 第 X 章／Thareja 第 Y 章、頁碼）

## 主題與出處
| # | 主題 | CLRS | Thareja |
|---|---|---|---|
| 01 | 複雜度 Big-O/Ω/Θ | 3 | 2 |
| 02 | 遞迴 | 4 | 7.7.4 |
| 03 | 陣列 Array | 10.1 | 3 |
| 04 | 字串 String | — | 4 |
| 05 | Linked List | 10.2 | 6 |
| 06 | Stack | 10.1 | 7 |
| 07 | Queue / Deque | 10.1 | 8 |
| 08 | Hash Table | 11 | 15 |
| 09 | Binary Search | 2.3 習題 | 14 |
| 10 | Insertion Sort | 2.1 | 14 |
| 11 | Merge Sort | 2.3 | 14 |
| 12 | Quick Sort | 7 | 14 |
| 13 | Heap / Priority Queue / Heapsort | 6 | 12 |
| 14 | Counting / Radix Sort | 8 | 14 |
| 15 | Binary Tree 走訪 | 10.3 | 9 |
| 16 | BST | 12 | 10 |
| 17 | AVL Tree | 13 問題 | 10 |
| 18 | 紅黑樹 Red-Black Tree | 13 | 10 |
| 19 | Trie | 問題 12-2 | 11 |
| 20 | BFS / DFS | 20 | 13 |
| 21 | 拓撲排序 Topological Sort | 20.4 | 13 |
| 22 | Union-Find | 19 | — |
| 23 | MST (Kruskal / Prim) | 21 | 13 |
| 24 | Dijkstra | 22.3 | 13 |
| 25 | Bellman-Ford | 22.1 | — |
| 26 | Floyd-Warshall | 23.2 | 13 |
| 27 | Divide & Conquer | 4 | — |
| 28 | 動態規劃 DP | 14 | — |
| 29 | Greedy | 15 | — |
| 30 | KMP | 32.4 | — |
| 31 | B-Tree | 18 | 11 |
| 32 | Segment Tree | —（相關：17 章） | — |
| 33 | Max Flow (Edmonds-Karp) | 24 | — |
| 34 | NP 觀念（僅筆記） | 34 | — |

頁碼已在各主題 README 的「出處」查證補上。

## 分批
| 批 | 內容 |
|---|---|
| 0 | 骨架（Makefile、README、git init）+ 13-Heap 範例 → 確認格式 |
| 1 | 01–02 基礎 |
| 2 | 03–07 線性結構 |
| 3 | 08–09 查找 |
| 4 | 10–12、14 排序 |
| 5 | 15–19 樹 |
| 6 | 20–26 圖 |
| 7 | 27–30 演算法設計 + 字串 |
| 8 | 31–34 進階 |

## 不做
CLRS 第 5、16、17、25–33、35 章與附錄；Thareja 第 1、5、16 章與附錄。
