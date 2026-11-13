# 19 · Trie 字典樹

## 一句話
**Trie（字典樹、前綴樹 prefix tree，唸作 "try"）** 把字串一個字元一個字元拆開存在樹上：從根往下走的路徑就是一個字串，**共同的前綴共用同一段路**。查一個長度 L 的字串只要 O(L)，跟字典裡有多少個字無關。

## 圖示
```
插入 app, apple, apply, ape, bat, bath：

            (root)
           /      \
          a        b
          |        |
          p        a
        /   \      |
       e*    p*    t*
            |      |
            l      h*
           / \
          e*  y*          * = 單字結尾 (end)
```
- `app` 是單字，所以第二個 p 有 `*`；`ap` 不是單字。
- `apple` 和 `apply` 共用 `a-p-p-l` 這段路。

### 每個節點存什麼
```c
struct TrieNode {
    TrieNode *child[26];  // 每個字母一個小孩指標
    int pass;             // 有幾個單字經過這裡 → 前綴計數 O(|p|)
    int end;              // 是不是單字結尾
};
```

### 刪除
沿路把 `pass` 減 1；某個節點的 `pass` 變成 0，代表沒有其他單字經過了，整條剪掉。
```
刪 bath：h 的 pass 1 → 0，剪掉 h；bat 還在
刪 bat ：b 的 pass 1 → 0，整條 b-a-t 剪掉
刪 app ：p 的 pass 還有 apple、apply 經過，只把 end 設成 0
```

### 自動完成 (autocomplete)
先走到前綴的節點，再從那裡做 DFS。依照 a → z 的順序走，結果剛好是**字典順序**。

## 複雜度
L = 字串長度，Σ = 字母表大小（這裡是 26），N = 總字元數。
| 操作 | 時間 |
|---|---|
| 插入 / 查詢 / 刪除 | O(L) |
| 前綴計數 `count_prefix` | O(L) |
| 自動完成 | O(L + 結果的總長度) |
| 空間 | O(N · Σ)，每個節點都有 26 個指標 |

### Trie vs Hash Table
| | Trie | Hash Table |
|---|---|---|
| 查整個字 | O(L) | 平均 O(L)（算雜湊也要讀完字串） |
| **前綴查詢** | **O(L)** | 做不到，要掃全部 |
| 依字典順序列出 | 可以（DFS） | 做不到 |
| 空間 | 大（每節點 26 個指標） | 小 |

## 面試陷阱／常考點
1. **「走得到」≠「是單字」**：`search("app")` 要看 `end` 旗標；`startsWith("app")` 只要走得到就好（LeetCode 208）。
2. **空間很大**：每個節點 26 個指標，在 64 位元系統上就是 208 bytes。字母表很大（例如 Unicode）時，改用 hash map 或排序陣列存小孩。
3. **壓縮 trie (radix tree / Patricia tree)**：把只有一個小孩的路徑合併成一條邊。CLRS 問題 12-2 的 radix tree 講的是 0/1 字串版本。
4. **萬用字元 `.`**：遇到 `.` 就把所有小孩都試一遍，DFS（LeetCode 211）。
5. **Trie + DFS/回溯**：Word Search II（212）是經典難題，把單字表建成 trie，在棋盤上 DFS 時同步走 trie，可以大量剪枝。
6. **XOR 最大值**：把整數的二進位（Σ = 2）存進 trie，就能找「跟 x XOR 最大的數」（LeetCode 421）。
7. **最短字根替換**（648）：沿著 trie 走，**第一次**碰到 end 就停。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 14 | [Longest Common Prefix](https://leetcode.com/problems/longest-common-prefix/) | Easy | 共同前綴 |
| 208 | [Implement Trie (Prefix Tree)](https://leetcode.com/problems/implement-trie-prefix-tree/) | Medium | 基本實作 |
| 720 | [Longest Word in Dictionary](https://leetcode.com/problems/longest-word-in-dictionary/) | Medium | 只走 end 節點的 DFS |
| 211 | [Design Add and Search Words Data Structure](https://leetcode.com/problems/design-add-and-search-words-data-structure/) | Medium | 萬用字元 DFS |
| 648 | [Replace Words](https://leetcode.com/problems/replace-words/) | Medium | 最短前綴 |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `trie.h` / `.c` | a-z 字典樹：插入、查詢、前綴計數（`pass` 欄位）、刪除（沒人經過就剪枝）、依字典順序的自動完成 |
| `test_trie.c` | 固定例子（刪除時的三種剪枝情況）、2 萬次隨機插入刪除，每一步都用暴力法對照前綴計數 |

執行：`make test T=19-trie`

## 出處
- **CLRS** 問題 12-2 Radix trees · PDF p.437（用 0/1 字串的 radix tree 做字典排序）
- **Thareja** 11.5 Trie · p.358
